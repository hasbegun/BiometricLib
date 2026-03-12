/// iris_comparison — Run the C++ iris pipeline on a directory of images
/// and output iris templates + matching scores for cross-language comparison.
///
/// Usage:
///   iris_comparison <pipeline.yaml> <image_dir> <output_dir>
///
/// Produces:
///   output_dir/
///     templates/{name}_code.npy    # iris code as uint8 (16, 512) one byte per bit
///     templates/{name}_mask.npy    # mask code as uint8 (16, 512) one byte per bit
///     genuine_scores.csv           # same-identity matching scores
///     impostor_scores.csv          # different-identity matching scores
///     timing.csv                   # per-image processing time
///     summary.json                 # aggregate metrics

#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <numeric>
#include <string>
#include <vector>

#include <opencv2/imgcodecs.hpp>

#include <iris/pipeline/iris_pipeline.hpp>
#include <iris/pipeline/pipeline_context.hpp>
#include <iris/nodes/hamming_distance_matcher.hpp>

// NPY writer — minimal, header-only (from tests/comparison/)
#include "../tests/comparison/npy_writer.hpp"

namespace fs = std::filesystem;
using namespace std::chrono;

struct ImageInfo {
    fs::path path;
    std::string filename;  // stem, e.g. "001_1_1"
    std::string subject;   // e.g. "001"
    std::string eye;       // e.g. "1"
    std::string capture;   // e.g. "1"
    std::string identity;  // e.g. "001_1"  (subject + eye = unique iris)
};

struct ProcessedImage {
    ImageInfo info;
    iris::IrisTemplate tmpl;
    double time_ms = 0.0;
};

static std::vector<ImageInfo> discover_images(const fs::path& dir) {
    std::vector<ImageInfo> images;

    for (const auto& subject_entry : fs::directory_iterator(dir)) {
        if (!subject_entry.is_directory()) continue;
        for (const auto& img_entry : fs::directory_iterator(subject_entry.path())) {
            if (img_entry.path().extension() != ".jpg" &&
                img_entry.path().extension() != ".bmp" &&
                img_entry.path().extension() != ".png") continue;

            auto stem = img_entry.path().stem().string();
            // CASIA1 naming: XXX_Y_Z
            auto p1 = stem.find('_');
            auto p2 = stem.find('_', p1 + 1);
            if (p1 == std::string::npos || p2 == std::string::npos) continue;

            ImageInfo info;
            info.path = img_entry.path();
            info.filename = stem;
            info.subject = stem.substr(0, p1);
            info.eye = stem.substr(p1 + 1, p2 - p1 - 1);
            info.capture = stem.substr(p2 + 1);
            info.identity = info.subject + "_" + info.eye;
            images.push_back(info);
        }
    }

    std::sort(images.begin(), images.end(),
              [](const ImageInfo& a, const ImageInfo& b) {
                  return a.filename < b.filename;
              });
    return images;
}

static void save_template_npy(const fs::path& dir, const std::string& name,
                               const iris::IrisTemplate& tmpl) {
    // Save each PackedIrisCode as a (16, 512) uint8 array
    for (size_t i = 0; i < tmpl.iris_codes.size(); ++i) {
        auto [code_mat, mask_mat] = tmpl.iris_codes[i].to_mat();

        auto code_path = dir / (name + "_code_" + std::to_string(i) + ".npy");
        auto mask_path = dir / (name + "_mask_" + std::to_string(i) + ".npy");
        iris::npy::write_npy(code_path, code_mat);
        iris::npy::write_npy(mask_path, mask_mat);
    }
}

static void write_csv_header(std::ofstream& f, const std::vector<std::string>& cols) {
    for (size_t i = 0; i < cols.size(); ++i) {
        if (i > 0) f << ',';
        f << cols[i];
    }
    f << '\n';
}

int main(int argc, char** argv) {
    if (argc < 4) {
        std::cerr << "Usage: iris_comparison <pipeline.yaml> <image_dir> <output_dir>\n";
        return 1;
    }

    const fs::path config_path = argv[1];
    const fs::path image_dir = argv[2];
    const fs::path output_dir = argv[3];

    fs::create_directories(output_dir);
    auto templates_dir = output_dir / "templates";
    fs::create_directories(templates_dir);

    // ---- Load pipeline ----
    std::cout << "Loading pipeline from: " << config_path << "\n";
    auto pipeline_result = iris::IrisPipeline::from_config(config_path);
    if (!pipeline_result) {
        std::cerr << "Failed to load pipeline: " << pipeline_result.error().message << "\n";
        return 1;
    }
    auto pipeline = std::move(*pipeline_result);
    std::cout << "Pipeline loaded OK\n";

    // ---- Discover images ----
    auto images = discover_images(image_dir);
    std::cout << "Found " << images.size() << " images\n";

    // ---- Process images ----
    std::vector<ProcessedImage> results;
    std::vector<std::pair<std::string, std::string>> failures;  // (filename, error)

    std::ofstream timing_csv(output_dir / "timing.csv");
    write_csv_header(timing_csv, {"filename", "subject", "eye", "capture", "time_ms"});

    size_t total = images.size();
    for (size_t idx = 0; idx < total; ++idx) {
        const auto& img_info = images[idx];
        std::cout << "[" << (idx + 1) << "/" << total << "] "
                  << img_info.filename << "... " << std::flush;

        cv::Mat img = cv::imread(img_info.path.string(), cv::IMREAD_GRAYSCALE);
        if (img.empty()) {
            std::cout << "SKIP (cannot read)\n";
            failures.emplace_back(img_info.filename, "cannot read image");
            continue;
        }

        iris::IRImage ir_image;
        ir_image.data = img;
        ir_image.image_id = img_info.filename;
        ir_image.eye_side = iris::EyeSide::Left;

        iris::PipelineContext ctx;
        auto t0 = high_resolution_clock::now();
        auto output = pipeline.run(ir_image, ctx);
        auto t1 = high_resolution_clock::now();
        double elapsed_ms = static_cast<double>(duration_cast<microseconds>(t1 - t0).count()) / 1000.0;

        if (!output) {
            std::cout << "FAIL (" << output.error().message << ")\n";
            failures.emplace_back(img_info.filename, output.error().message);
            timing_csv << img_info.filename << "," << img_info.subject << ","
                       << img_info.eye << "," << img_info.capture << ","
                       << elapsed_ms << "\n";
            continue;
        }

        if (!output->iris_template) {
            std::cout << "FAIL (no template";
            if (output->error) std::cout << ": " << output->error->message;
            std::cout << ")\n";
            failures.emplace_back(img_info.filename,
                                  output->error ? output->error->message : "no template");
            timing_csv << img_info.filename << "," << img_info.subject << ","
                       << img_info.eye << "," << img_info.capture << ","
                       << elapsed_ms << "\n";
            continue;
        }

        // Save template
        save_template_npy(templates_dir, img_info.filename, *output->iris_template);

        // Save normalized image for first 10 successful images
        if (results.size() < 10) {
            auto norm_r = ctx.get<iris::NormalizedIris>("normalization");
            if (norm_r && *norm_r) {
                const auto& norm = **norm_r;
                // Save normalized_image as NPY (CV_64FC1)
                auto ni_path = templates_dir / (img_info.filename + "_norm_image.npy");
                iris::npy::write_npy(ni_path, norm.normalized_image);
                auto nm_path = templates_dir / (img_info.filename + "_norm_mask.npy");
                iris::npy::write_npy(nm_path, norm.normalized_mask);
            }
            // Save geometry contours as npy (float32, N×2)
            auto geom_r = ctx.get<iris::GeometryPolygons>("geometry_estimation");
            if (geom_r && *geom_r) {
                const auto& geom = **geom_r;
                auto save_contour = [&](const iris::Contour& c, const std::string& suffix) {
                    cv::Mat mat(static_cast<int>(c.points.size()), 2, CV_32FC1);
                    for (size_t k = 0; k < c.points.size(); ++k) {
                        mat.at<float>(static_cast<int>(k), 0) = c.points[k].x;
                        mat.at<float>(static_cast<int>(k), 1) = c.points[k].y;
                    }
                    iris::npy::write_npy(templates_dir / (img_info.filename + suffix), mat);
                };
                save_contour(geom.iris, "_geom_iris.npy");
                save_contour(geom.pupil, "_geom_pupil.npy");
                save_contour(geom.eyeball, "_geom_eyeball.npy");
            }
            // Save eye centers
            auto centers_r = ctx.get<iris::EyeCenters>("eye_center_estimation");
            if (centers_r && *centers_r) {
                const auto& c = **centers_r;
                std::ofstream cf(templates_dir / (img_info.filename + "_centers.txt"));
                cf << std::setprecision(12)
                   << "iris_x=" << c.iris_center.x << "\n"
                   << "iris_y=" << c.iris_center.y << "\n"
                   << "pupil_x=" << c.pupil_center.x << "\n"
                   << "pupil_y=" << c.pupil_center.y << "\n";
            }
            // Save pre-geometry contours (from smoothing, before extrapolation)
            auto smooth_r = ctx.get<iris::GeometryPolygons>("smoothing");
            if (smooth_r && *smooth_r) {
                const auto& smooth = **smooth_r;
                auto save_c = [&](const iris::Contour& c, const std::string& suffix) {
                    cv::Mat mat(static_cast<int>(c.points.size()), 2, CV_32FC1);
                    for (size_t k = 0; k < c.points.size(); ++k) {
                        mat.at<float>(static_cast<int>(k), 0) = c.points[k].x;
                        mat.at<float>(static_cast<int>(k), 1) = c.points[k].y;
                    }
                    iris::npy::write_npy(templates_dir / (img_info.filename + suffix), mat);
                };
                save_c(smooth.iris, "_smooth_iris.npy");
                save_c(smooth.pupil, "_smooth_pupil.npy");
            }
            // Save eye orientation
            auto orient_r = ctx.get<iris::EyeOrientation>("eye_orientation");
            if (orient_r && *orient_r) {
                std::ofstream of(templates_dir / (img_info.filename + "_orientation.txt"));
                of << "angle_radians=" << (*orient_r)->angle << "\n";
            }
        }

        ProcessedImage proc;
        proc.info = img_info;
        proc.tmpl = std::move(*output->iris_template);
        proc.time_ms = elapsed_ms;
        results.push_back(std::move(proc));

        timing_csv << img_info.filename << "," << img_info.subject << ","
                   << img_info.eye << "," << img_info.capture << ","
                   << elapsed_ms << "\n";

        std::cout << "OK (" << std::fixed << std::setprecision(1)
                  << elapsed_ms << "ms)\n";
    }
    timing_csv.close();

    std::cout << "\nProcessed: " << results.size() << " OK, "
              << failures.size() << " failed out of " << total << "\n";

    // ---- Compute matching scores ----
    iris::HammingDistanceMatcher matcher(iris::HammingDistanceMatcher::Params{
        .rotation_shift = 15,
        .normalise = true,
        .norm_mean = 0.45,
        .norm_gradient = 0.00005,
        .separate_half_matching = true,
    });

    // Group by identity
    std::map<std::string, std::vector<size_t>> identity_groups;
    for (size_t i = 0; i < results.size(); ++i) {
        identity_groups[results[i].info.identity].push_back(i);
    }

    // Genuine scores
    std::cout << "\nComputing genuine scores (" << identity_groups.size() << " identities)...\n";
    std::ofstream genuine_csv(output_dir / "genuine_scores.csv");
    write_csv_header(genuine_csv, {"image_a", "image_b", "identity", "score"});
    size_t genuine_count = 0;

    for (const auto& [identity, indices] : identity_groups) {
        for (size_t i = 0; i < indices.size(); ++i) {
            for (size_t j = i + 1; j < indices.size(); ++j) {
                auto match = matcher.run(results[indices[i]].tmpl, results[indices[j]].tmpl);
                if (match) {
                    genuine_csv << results[indices[i]].info.filename << ","
                                << results[indices[j]].info.filename << ","
                                << identity << ","
                                << std::fixed << std::setprecision(6)
                                << match->distance << "\n";
                    ++genuine_count;
                }
            }
        }
    }
    genuine_csv.close();

    // Impostor scores (first template per identity)
    std::cout << "Computing impostor scores...\n";
    std::ofstream impostor_csv(output_dir / "impostor_scores.csv");
    write_csv_header(impostor_csv, {"image_a", "image_b", "identity_a", "identity_b", "score"});
    size_t impostor_count = 0;

    std::vector<std::pair<std::string, size_t>> identity_reps;
    for (const auto& [identity, indices] : identity_groups) {
        identity_reps.emplace_back(identity, indices[0]);
    }

    for (size_t i = 0; i < identity_reps.size(); ++i) {
        for (size_t j = i + 1; j < identity_reps.size(); ++j) {
            auto match = matcher.run(results[identity_reps[i].second].tmpl,
                                     results[identity_reps[j].second].tmpl);
            if (match) {
                impostor_csv << results[identity_reps[i].second].info.filename << ","
                             << results[identity_reps[j].second].info.filename << ","
                             << identity_reps[i].first << ","
                             << identity_reps[j].first << ","
                             << std::fixed << std::setprecision(6)
                             << match->distance << "\n";
                ++impostor_count;
            }
        }
    }
    impostor_csv.close();

    // ---- Summary ----
    std::vector<double> times;
    for (const auto& r : results) times.push_back(r.time_ms);

    double mean_time = 0, median_time = 0, std_time = 0, min_time = 0, max_time = 0;
    if (!times.empty()) {
        std::sort(times.begin(), times.end());
        mean_time = std::accumulate(times.begin(), times.end(), 0.0) /
                    static_cast<double>(times.size());
        median_time = times[times.size() / 2];
        min_time = times.front();
        max_time = times.back();
        double sum_sq = 0;
        for (double t : times) sum_sq += (t - mean_time) * (t - mean_time);
        std_time = std::sqrt(sum_sq / static_cast<double>(times.size()));
    }

    // Write summary JSON
    double total_s = std::accumulate(times.begin(), times.end(), 0.0) / 1000.0;
    std::ofstream summary(output_dir / "summary.json");
    summary << std::fixed;
    summary << "{\n";
    summary << "  \"pipeline\": \"C++ iris (libiris)\",\n";
    summary << "  \"dataset\": \"CASIA1\",\n";
    summary << "  \"total_images\": " << total << ",\n";
    summary << "  \"successful\": " << results.size() << ",\n";
    summary << "  \"failed\": " << failures.size() << ",\n";
    summary << "  \"success_rate\": " << std::setprecision(4)
            << (total > 0 ? static_cast<double>(results.size()) / static_cast<double>(total) : 0.0)
            << ",\n";
    summary << "  \"timing\": {\n";
    summary << "    \"mean_ms\": " << std::setprecision(1) << mean_time << ",\n";
    summary << "    \"median_ms\": " << median_time << ",\n";
    summary << "    \"std_ms\": " << std_time << ",\n";
    summary << "    \"min_ms\": " << min_time << ",\n";
    summary << "    \"max_ms\": " << max_time << ",\n";
    summary << "    \"total_s\": " << total_s << "\n";
    summary << "  },\n";
    summary << "  \"genuine_count\": " << genuine_count << ",\n";
    summary << "  \"impostor_count\": " << impostor_count << ",\n";
    summary << "  \"failures\": [\n";

    for (size_t i = 0; i < failures.size(); ++i) {
        summary << "    {\"filename\": \"" << failures[i].first
                << "\", \"error\": \"" << failures[i].second << "\"}";
        if (i + 1 < failures.size()) summary << ",";
        summary << "\n";
    }
    summary << "  ]\n}\n";
    summary.close();

    // ---- Print report ----
    std::cout << "\n"
              << std::string(70, '=') << "\n"
              << "C++ PIPELINE REPORT — CASIA1\n"
              << std::string(70, '=') << "\n"
              << "\nPipeline:  C++ iris (libiris)\n"
              << "Dataset:   CASIA1\n"
              << "Images:    " << total << " total, " << results.size() << " OK, "
              << failures.size() << " failed ("
              << std::setprecision(1)
              << (total > 0 ? 100.0 * static_cast<double>(results.size()) / static_cast<double>(total) : 0.0)
              << "%)\n"
              << "\nTiming:    " << std::setprecision(1) << mean_time
              << " ± " << std_time << " ms/image"
              << " (median " << median_time
              << ", range [" << min_time << ", " << max_time << "])\n"
              << "           Total: " << std::setprecision(1)
              << std::accumulate(times.begin(), times.end(), 0.0) / 1000.0 << "s\n"
              << "\nGenuine:   " << genuine_count << " pairs\n"
              << "Impostor:  " << impostor_count << " pairs\n"
              << "\n" << std::string(70, '=') << "\n"
              << "\nResults saved to: " << output_dir << "\n";

    return 0;
}
