#include <iris/nodes/perspective_normalization.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <string>

#include <opencv2/calib3d.hpp>
#include <opencv2/imgproc.hpp>

#include <iris/core/node_registry.hpp>
#include <iris/utils/normalization_utils.hpp>

namespace iris {

PerspectiveNormalization::PerspectiveNormalization() {
    // Default intermediate_radiuses: linspace(0, 1, 10)
    params_.intermediate_radiuses.resize(10);
    for (int i = 0; i < 10; ++i) {
        params_.intermediate_radiuses[static_cast<size_t>(i)] =
            static_cast<double>(i) / 9.0;
    }
}

PerspectiveNormalization::PerspectiveNormalization(
    const std::unordered_map<std::string, std::string>& node_params)
    : PerspectiveNormalization() {
    auto it = node_params.find("res_in_phi");
    if (it != node_params.end()) params_.res_in_phi = std::stoi(it->second);

    it = node_params.find("res_in_r");
    if (it != node_params.end()) params_.res_in_r = std::stoi(it->second);

    it = node_params.find("skip_boundary_points");
    if (it != node_params.end()) params_.skip_boundary_points = std::stoi(it->second);

    it = node_params.find("oversat_threshold");
    if (it != node_params.end()) params_.oversat_threshold = std::stoi(it->second);
}

std::pair<std::vector<std::vector<cv::Point2f>>,
          std::vector<std::vector<cv::Point2f>>>
PerspectiveNormalization::generate_correspondences(
    const Contour& pupil_points, const Contour& iris_points) const {
    const int skip = params_.skip_boundary_points;
    const auto orig_n = static_cast<int>(pupil_points.points.size());

    // Subsample boundary points
    std::vector<cv::Point2f> sub_pupil;
    std::vector<cv::Point2f> sub_iris;
    for (int i = 0; i < orig_n; i += skip) {
        sub_pupil.push_back(pupil_points.points[static_cast<size_t>(i)]);
        sub_iris.push_back(iris_points.points[static_cast<size_t>(i)]);
    }

    const auto num_boundary = static_cast<int>(sub_pupil.size());
    const auto num_rings = static_cast<int>(params_.intermediate_radiuses.size());

    // Generate src_points: for each ring, interpolate between pupil and iris
    // Each ring wraps around (append first point again)
    std::vector<std::vector<cv::Point2f>> src_points(static_cast<size_t>(num_rings));
    for (int r = 0; r < num_rings; ++r) {
        const double t = params_.intermediate_radiuses[static_cast<size_t>(r)];
        auto& ring = src_points[static_cast<size_t>(r)];
        ring.reserve(static_cast<size_t>(num_boundary + 1));

        for (int i = 0; i < num_boundary; ++i) {
            const float px = sub_pupil[static_cast<size_t>(i)].x;
            const float py = sub_pupil[static_cast<size_t>(i)].y;
            const float ix = sub_iris[static_cast<size_t>(i)].x;
            const float iy = sub_iris[static_cast<size_t>(i)].y;
            ring.emplace_back(
                px + static_cast<float>(t) * (ix - px),
                py + static_cast<float>(t) * (iy - py));
        }
        // Wrap around
        ring.push_back(ring[0]);
    }

    const int num_ring_pts = num_boundary + 1;

    // Generate dst_points: meshgrid in normalized image coordinates
    // x = linspace(0, res_in_phi, num_ring_pts)
    // y = linspace(0, res_in_r, num_rings)
    std::vector<std::vector<cv::Point2f>> dst_points(static_cast<size_t>(num_rings));
    for (int r = 0; r < num_rings; ++r) {
        const float dy = static_cast<float>(
            static_cast<double>(r) * static_cast<double>(params_.res_in_r) /
            static_cast<double>(num_rings - 1));
        auto& row = dst_points[static_cast<size_t>(r)];
        row.reserve(static_cast<size_t>(num_ring_pts));

        for (int c = 0; c < num_ring_pts; ++c) {
            const float dx = static_cast<float>(
                static_cast<double>(c) * static_cast<double>(params_.res_in_phi) /
                static_cast<double>(num_ring_pts - 1));
            row.emplace_back(dx, dy);
        }
    }

    return {src_points, dst_points};
}

Result<NormalizedIris> PerspectiveNormalization::run(
    const IRImage& image,
    const NoiseMask& noise_mask,
    const GeometryPolygons& extrapolated_contours,
    const EyeOrientation& eye_orientation) const {
    if (extrapolated_contours.pupil.points.size() !=
        extrapolated_contours.iris.points.size()) {
        return make_error(ErrorCode::NormalizationFailed,
                          "Pupil and iris point counts differ");
    }
    if (extrapolated_contours.pupil.points.empty()) {
        return make_error(ErrorCode::NormalizationFailed,
                          "Empty contour points");
    }

    auto [pupil_pts, iris_pts] = correct_orientation(
        extrapolated_contours.pupil, extrapolated_contours.iris,
        eye_orientation.angle);

    cv::Mat iris_mask = generate_iris_mask(extrapolated_contours, noise_mask.mask);

    for (int y = 0; y < image.data.rows; ++y) {
        const auto* img_row = image.data.ptr<uint8_t>(y);
        auto* mask_row = iris_mask.ptr<uint8_t>(y);
        for (int x = 0; x < image.data.cols; ++x) {
            if (img_row[x] >= static_cast<uint8_t>(params_.oversat_threshold)) {
                mask_row[x] = 0;
            }
        }
    }

    auto [src_points, dst_points] =
        generate_correspondences(pupil_pts, iris_pts);

    const int out_h = params_.res_in_r;
    const int out_w = params_.res_in_phi;

    cv::Mat norm_image = cv::Mat::zeros(out_h, out_w, CV_64FC1);
    cv::Mat norm_mask = cv::Mat::zeros(out_h, out_w, CV_8UC1);

    const auto num_rings = static_cast<int>(src_points.size());
    const auto num_angle_pts = static_cast<int>(src_points[0].size());

    // For each quad (ring_idx, angle_idx), compute perspective transform
    for (int angle_idx = 0; angle_idx < num_angle_pts - 1; ++angle_idx) {
        for (int ring_idx = 0; ring_idx < num_rings - 1; ++ring_idx) {
            // 4 corners of the quad in src and dst
            const cv::Point2f src_quad[4] = {
                src_points[static_cast<size_t>(ring_idx)]
                          [static_cast<size_t>(angle_idx)],
                src_points[static_cast<size_t>(ring_idx)]
                          [static_cast<size_t>(angle_idx + 1)],
                src_points[static_cast<size_t>(ring_idx + 1)]
                          [static_cast<size_t>(angle_idx + 1)],
                src_points[static_cast<size_t>(ring_idx + 1)]
                          [static_cast<size_t>(angle_idx)],
            };
            const cv::Point2f dst_quad[4] = {
                dst_points[static_cast<size_t>(ring_idx)]
                          [static_cast<size_t>(angle_idx)],
                dst_points[static_cast<size_t>(ring_idx)]
                          [static_cast<size_t>(angle_idx + 1)],
                dst_points[static_cast<size_t>(ring_idx + 1)]
                          [static_cast<size_t>(angle_idx + 1)],
                dst_points[static_cast<size_t>(ring_idx + 1)]
                          [static_cast<size_t>(angle_idx)],
            };

            // Bounding box of dst quad
            const int xmin = static_cast<int>(std::floor(std::min(
                {static_cast<double>(dst_quad[0].x),
                 static_cast<double>(dst_quad[1].x),
                 static_cast<double>(dst_quad[2].x),
                 static_cast<double>(dst_quad[3].x)})));
            const int ymin = static_cast<int>(std::floor(std::min(
                {static_cast<double>(dst_quad[0].y),
                 static_cast<double>(dst_quad[1].y),
                 static_cast<double>(dst_quad[2].y),
                 static_cast<double>(dst_quad[3].y)})));
            const int xmax = static_cast<int>(std::ceil(std::max(
                {static_cast<double>(dst_quad[0].x),
                 static_cast<double>(dst_quad[1].x),
                 static_cast<double>(dst_quad[2].x),
                 static_cast<double>(dst_quad[3].x)})));
            const int ymax = static_cast<int>(std::ceil(std::max(
                {static_cast<double>(dst_quad[0].y),
                 static_cast<double>(dst_quad[1].y),
                 static_cast<double>(dst_quad[2].y),
                 static_cast<double>(dst_quad[3].y)})));

            if (xmax <= xmin || ymax <= ymin) continue;

            // Perspective transform: dst -> src
            cv::Mat persp = cv::getPerspectiveTransform(dst_quad, src_quad);

            // For each pixel in the dst bounding box, map back to src
            for (int ny = ymin; ny < ymax && ny < out_h; ++ny) {
                if (ny < 0) continue;
                for (int nx = xmin; nx < xmax && nx < out_w; ++nx) {
                    if (nx < 0) continue;

                    // Apply perspective transform
                    const double w = persp.at<double>(2, 0) * static_cast<double>(nx) +
                                     persp.at<double>(2, 1) * static_cast<double>(ny) +
                                     persp.at<double>(2, 2);
                    if (std::abs(w) < 1e-10) continue;

                    const double sx = (persp.at<double>(0, 0) * static_cast<double>(nx) +
                                       persp.at<double>(0, 1) * static_cast<double>(ny) +
                                       persp.at<double>(0, 2)) / w;
                    const double sy = (persp.at<double>(1, 0) * static_cast<double>(nx) +
                                       persp.at<double>(1, 1) * static_cast<double>(ny) +
                                       persp.at<double>(1, 2)) / w;

                    // Bilinear interpolation for image
                    const double intensity =
                        interpolate_pixel_intensity(image.data, sx, sy);
                    norm_image.at<double>(ny, nx) = intensity;

                    // Nearest-neighbor for mask
                    const int mx = static_cast<int>(std::round(sx));
                    const int my = static_cast<int>(std::round(sy));
                    if (mx >= 0 && mx < iris_mask.cols &&
                        my >= 0 && my < iris_mask.rows) {
                        norm_mask.at<uint8_t>(ny, nx) =
                            iris_mask.at<uint8_t>(my, mx);
                    }
                }
            }
        }
    }

    return NormalizedIris{
        .normalized_image = std::move(norm_image),
        .normalized_mask = std::move(norm_mask),
    };
}

IRIS_REGISTER_NODE("iris.PerspectiveNormalization", PerspectiveNormalization);

}  // namespace iris
