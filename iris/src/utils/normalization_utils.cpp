#include <iris/utils/normalization_utils.hpp>

#include <algorithm>
#include <cmath>
#include <numbers>

#include <opencv2/imgproc.hpp>

#include <iris/utils/geometry_utils.hpp>

namespace iris {

std::pair<Contour, Contour> correct_orientation(
    const Contour& pupil_points, const Contour& iris_points,
    double eye_orientation_radians) {
    const auto n = static_cast<int>(pupil_points.points.size());
    if (n == 0) return {pupil_points, iris_points};

    const double angle_degrees =
        eye_orientation_radians * 180.0 / std::numbers::pi;
    // Matches Python: num_rotations = -round(angle_degrees * n / 360.0)
    const int num_rotations =
        -static_cast<int>(std::round(angle_degrees * static_cast<double>(n) / 360.0));

    // np.roll(arr, k) places arr[(i - k) % n] at position i.
    // Our roll lambda places c[(i + shift) % n] at position i.
    // So shift = -num_rotations (mod n).
    int shift = (-num_rotations) % n;
    if (shift < 0) shift += n;

    auto roll = [](const Contour& c, int s) -> Contour {
        Contour result;
        const auto sz = c.points.size();
        result.points.reserve(sz);
        for (size_t i = 0; i < sz; ++i) {
            result.points.push_back(
                c.points[(i + static_cast<size_t>(s)) % sz]);
        }
        return result;
    };

    return {roll(pupil_points, shift), roll(iris_points, shift)};
}

cv::Mat generate_iris_mask(const GeometryPolygons& contours,
                           const cv::Mat& noise_mask) {
    const cv::Size sz = noise_mask.size();

    cv::Mat iris_mask = contour_to_filled_mask(contours.iris, sz);
    cv::Mat eyeball_mask = contour_to_filled_mask(contours.eyeball, sz);

    // iris_mask = iris_mask & eyeball_mask
    cv::bitwise_and(iris_mask, eyeball_mask, iris_mask);

    // iris_mask = ~(iris_mask & noise_mask) & iris_mask
    // i.e., remove noise pixels from iris mask
    cv::Mat noise_in_iris;
    cv::bitwise_and(iris_mask, noise_mask, noise_in_iris);
    cv::Mat not_noise_in_iris;
    cv::bitwise_not(noise_in_iris, not_noise_in_iris);
    cv::bitwise_and(iris_mask, not_noise_in_iris, iris_mask);

    return iris_mask;
}

std::pair<cv::Mat, cv::Mat> normalize_all(
    const cv::Mat& image, const cv::Mat& iris_mask,
    const std::vector<std::vector<cv::Point>>& src_points) {
    if (src_points.empty()) {
        return {cv::Mat(), cv::Mat()};
    }

    const auto rows = static_cast<int>(src_points.size());
    const auto cols = static_cast<int>(src_points[0].size());
    const int img_h = image.rows;
    const int img_w = image.cols;

    cv::Mat normalized_image = cv::Mat::zeros(rows, cols, CV_64FC1);
    cv::Mat normalized_mask = cv::Mat::zeros(rows, cols, CV_8UC1);

    for (int r = 0; r < rows; ++r) {
        auto* img_row = normalized_image.ptr<double>(r);
        auto* mask_row = normalized_mask.ptr<uint8_t>(r);
        for (int c = 0; c < cols; ++c) {
            const int px = src_points[static_cast<size_t>(r)]
                                     [static_cast<size_t>(c)].x;
            const int py = src_points[static_cast<size_t>(r)]
                                     [static_cast<size_t>(c)].y;
            if (px >= 0 && px < img_w && py >= 0 && py < img_h) {
                img_row[c] = static_cast<double>(
                    image.at<uint8_t>(py, px));
                mask_row[c] = iris_mask.at<uint8_t>(py, px);
            }
            // else: stays 0 / false
        }
    }

    return {normalized_image, normalized_mask};
}

double get_pixel_or_default(const cv::Mat& image, int x, int y,
                            double default_val) {
    if (x >= 0 && x < image.cols && y >= 0 && y < image.rows) {
        return static_cast<double>(image.at<uint8_t>(y, x));
    }
    return default_val;
}

double interpolate_pixel_intensity(const cv::Mat& image, double pixel_x,
                                   double pixel_y) {
    double xmin = std::floor(pixel_x);
    double ymin = std::floor(pixel_y);
    double xmax = std::ceil(pixel_x);
    double ymax = std::ceil(pixel_y);

    const int img_h = image.rows;
    const int img_w = image.cols;

    if (xmin == xmax) {
        if (static_cast<int>(xmax) != img_w - 1)
            xmax += 1.0;
        else
            xmin -= 1.0;
    }
    if (ymin == ymax) {
        if (static_cast<int>(ymax) != img_h - 1)
            ymax += 1.0;
        else
            ymin -= 1.0;
    }

    const double ll = get_pixel_or_default(
        image, static_cast<int>(xmin), static_cast<int>(ymax), 0.0);
    const double lr = get_pixel_or_default(
        image, static_cast<int>(xmax), static_cast<int>(ymax), 0.0);
    const double ul = get_pixel_or_default(
        image, static_cast<int>(xmin), static_cast<int>(ymin), 0.0);
    const double ur = get_pixel_or_default(
        image, static_cast<int>(xmax), static_cast<int>(ymin), 0.0);

    // Bilinear: xs_diff . pixel_matrix . ys_diff
    const double dx1 = xmax - pixel_x;
    const double dx2 = pixel_x - xmin;
    const double dy1 = pixel_y - ymin;
    const double dy2 = ymax - pixel_y;

    // [dx1, dx2] . [[ll, ul], [lr, ur]] . [[dy1], [dy2]]
    const double r1 = dx1 * ll + dx2 * lr;
    const double r2 = dx1 * ul + dx2 * ur;
    return r1 * dy1 + r2 * dy2;
}

}  // namespace iris
