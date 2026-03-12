// Generates synthetic and real eye fixture images for testing.
// Usage: generate_eye_fixtures <project_root>
//
// Synthetic images: dark background with white circle at known position.
// Filename convention: synthetic_pupil_CX_CY_R.png (center x, y, radius)
//
// Real images: cropped from face fixture images using dlib pipeline.

#include <filesystem>
#include <iostream>
#include <string>

#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include <eyetrack/nodes/eye_extractor.hpp>
#include <eyetrack/nodes/face_detector.hpp>
#include <eyetrack/nodes/preprocessor.hpp>

namespace fs = std::filesystem;

void generate_synthetic(const std::string& output_dir) {
    // Synthetic eye image 1: centered pupil
    {
        cv::Mat img(60, 120, CV_8UC1, cv::Scalar(40));  // dark iris
        cv::circle(img, cv::Point(60, 30), 12, cv::Scalar(10), -1);  // dark pupil
        cv::circle(img, cv::Point(60, 30), 12, cv::Scalar(255), 1);  // highlight ring
        // Add a bright specular reflection
        cv::circle(img, cv::Point(55, 25), 2, cv::Scalar(220), -1);
        cv::imwrite(output_dir + "/synthetic_pupil_60_30_12.png", img);
    }

    // Synthetic eye image 2: off-center pupil (looking right)
    {
        cv::Mat img(60, 120, CV_8UC1, cv::Scalar(40));
        cv::circle(img, cv::Point(80, 28), 10, cv::Scalar(10), -1);
        cv::circle(img, cv::Point(80, 28), 10, cv::Scalar(255), 1);
        cv::circle(img, cv::Point(76, 24), 2, cv::Scalar(220), -1);
        cv::imwrite(output_dir + "/synthetic_pupil_80_28_10.png", img);
    }

    // Synthetic eye image 3: off-center pupil (looking left)
    {
        cv::Mat img(60, 120, CV_8UC1, cv::Scalar(40));
        cv::circle(img, cv::Point(40, 32), 14, cv::Scalar(10), -1);
        cv::circle(img, cv::Point(40, 32), 14, cv::Scalar(255), 1);
        cv::circle(img, cv::Point(36, 27), 2, cv::Scalar(220), -1);
        cv::imwrite(output_dir + "/synthetic_pupil_40_32_14.png", img);
    }

    // Synthetic eye image 4: small pupil (bright light)
    {
        cv::Mat img(60, 120, CV_8UC1, cv::Scalar(50));
        cv::circle(img, cv::Point(58, 30), 6, cv::Scalar(5), -1);
        cv::circle(img, cv::Point(58, 30), 6, cv::Scalar(200), 1);
        cv::imwrite(output_dir + "/synthetic_pupil_58_30_6.png", img);
    }

    std::cout << "Generated 4 synthetic eye fixtures in " << output_dir << "\n";
}

void generate_real(const std::string& project_root, const std::string& output_dir) {
    const std::string fixture_dir = project_root + "/tests/fixtures/face_images";
    const std::string dlib_model =
        project_root + "/models/shape_predictor_68_face_landmarks.dat";

    if (!fs::exists(dlib_model)) {
        std::cerr << "dlib model not found, skipping real eye fixtures\n";
        return;
    }

    eyetrack::FaceDetector::Config fd_config;
    fd_config.backend = eyetrack::FaceDetectorBackend::Dlib;
    fd_config.dlib_shape_predictor_path = dlib_model;
    eyetrack::FaceDetector detector(fd_config);
    eyetrack::EyeExtractor extractor;

    int count = 0;
    for (const auto& entry : fs::directory_iterator(fixture_dir)) {
        if (!entry.path().filename().string().starts_with("frontal_")) continue;

        cv::Mat frame = cv::imread(entry.path().string());
        if (frame.empty()) continue;

        // Resize to standard size for consistent detection
        cv::Mat resized;
        cv::resize(frame, resized, cv::Size(640, 480));

        auto face_result = detector.run(resized);
        if (!face_result.has_value()) continue;

        auto eye_result = extractor.run(face_result.value(), resized);
        if (!eye_result.has_value()) continue;

        std::string stem = entry.path().stem().string();
        if (!eye_result->left_eye_crop.empty()) {
            cv::imwrite(output_dir + "/real_left_" + stem + ".png",
                        eye_result->left_eye_crop);
            ++count;
        }
        if (!eye_result->right_eye_crop.empty()) {
            cv::imwrite(output_dir + "/real_right_" + stem + ".png",
                        eye_result->right_eye_crop);
            ++count;
        }
    }
    std::cout << "Generated " << count << " real eye fixtures in " << output_dir << "\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <project_root>\n";
        return 1;
    }

    std::string project_root = argv[1];
    std::string output_dir = project_root + "/tests/fixtures/eye_images";
    fs::create_directories(output_dir);

    generate_synthetic(output_dir);
    generate_real(project_root, output_dir);

    return 0;
}
