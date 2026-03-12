#include <iris/nodes/encoder.hpp>

#include <gtest/gtest.h>

using namespace iris;

// Helper: create a properly-sized filter response (16x256 CV_64FC2).
// All values default to zero; test code sets specific positions.
static IrisFilterResponse make_response() {
    IrisFilterResponse response;
    IrisFilterResponse::Response resp;
    constexpr int kRows = 16;
    constexpr int kCols = 256;
    resp.data = cv::Mat(kRows, kCols, CV_64FC2, cv::Scalar(0.0, 0.0));
    resp.mask = cv::Mat(kRows, kCols, CV_64FC2, cv::Scalar(0.95, 0.95));
    response.responses.push_back(resp);
    response.iris_code_version = "v2.0";
    return response;
}

TEST(IrisEncoder, BinarizesRealPositive) {
    auto response = make_response();
    auto& resp = response.responses[0];

    // Set known values at specific positions
    resp.data.at<cv::Vec2d>(0, 0) = {1.0, 1.0};    // code: 1, 1
    resp.data.at<cv::Vec2d>(0, 1) = {-1.0, 1.0};   // code: 0, 1
    resp.data.at<cv::Vec2d>(0, 2) = {1.0, -1.0};    // code: 1, 0
    resp.data.at<cv::Vec2d>(1, 0) = {-1.0, -1.0};   // code: 0, 0
    resp.data.at<cv::Vec2d>(1, 1) = {0.5, -0.5};    // code: 1, 0
    resp.data.at<cv::Vec2d>(1, 2) = {0.0, 0.0};     // code: 0, 0 (zero is NOT > 0)

    IrisEncoder encoder;
    auto result = encoder.run(response);
    ASSERT_TRUE(result.has_value()) << result.error().message;

    auto& tmpl = result.value();
    ASSERT_EQ(tmpl.iris_codes.size(), 1u);

    // Unpack and verify bits at known positions
    auto [code_mat, mask_mat] = tmpl.iris_codes[0].to_mat();

    // Row 0, col 0: {1.0, 1.0} -> bits at (0, 0) and (0, 1) = 1, 1
    EXPECT_EQ(code_mat.at<uint8_t>(0, 0), 1);  // real(1.0) > 0
    EXPECT_EQ(code_mat.at<uint8_t>(0, 1), 1);  // imag(1.0) > 0

    // Row 0, col 1: {-1.0, 1.0} -> bits at (0, 2) and (0, 3) = 0, 1
    EXPECT_EQ(code_mat.at<uint8_t>(0, 2), 0);  // real(-1.0) > 0 = false
    EXPECT_EQ(code_mat.at<uint8_t>(0, 3), 1);  // imag(1.0) > 0

    // Row 0, col 2: {1.0, -1.0} -> bits at (0, 4) and (0, 5) = 1, 0
    EXPECT_EQ(code_mat.at<uint8_t>(0, 4), 1);  // real(1.0) > 0
    EXPECT_EQ(code_mat.at<uint8_t>(0, 5), 0);  // imag(-1.0) > 0 = false

    // Row 1, col 0: {-1.0, -1.0} -> bits at (1, 0) and (1, 1) = 0, 0
    EXPECT_EQ(code_mat.at<uint8_t>(1, 0), 0);  // real(-1.0) > 0 = false
    EXPECT_EQ(code_mat.at<uint8_t>(1, 1), 0);  // imag(-1.0) > 0 = false

    // Row 1, col 1: {0.5, -0.5} -> bits at (1, 2) and (1, 3) = 1, 0
    EXPECT_EQ(code_mat.at<uint8_t>(1, 2), 1);  // real(0.5) > 0
    EXPECT_EQ(code_mat.at<uint8_t>(1, 3), 0);  // imag(-0.5) > 0 = false

    // Row 1, col 2: {0.0, 0.0} -> bits at (1, 4) and (1, 5) = 0, 0
    EXPECT_EQ(code_mat.at<uint8_t>(1, 4), 0);  // real(0.0) > 0 = false
    EXPECT_EQ(code_mat.at<uint8_t>(1, 5), 0);  // imag(0.0) > 0 = false

    // All zero positions should remain zero
    EXPECT_EQ(code_mat.at<uint8_t>(5, 100), 0);
    EXPECT_EQ(code_mat.at<uint8_t>(15, 511), 0);
}

TEST(IrisEncoder, MaskThreshold) {
    auto response = make_response();
    auto& resp = response.responses[0];

    // Set data to positive so code bits are all 1
    resp.data.at<cv::Vec2d>(0, 0) = {1.0, 1.0};
    resp.data.at<cv::Vec2d>(0, 1) = {1.0, 1.0};

    // First pixel: mask above threshold, second: below
    resp.mask.at<cv::Vec2d>(0, 0) = {0.95, 0.95};  // >= 0.93 -> valid
    resp.mask.at<cv::Vec2d>(0, 1) = {0.90, 0.92};  // < 0.93 -> invalid

    IrisEncoder encoder(IrisEncoder::Params{.mask_threshold = 0.93});
    auto result = encoder.run(response);
    ASSERT_TRUE(result.has_value());

    auto [_, mask_mat] = result.value().mask_codes[0].to_mat();

    EXPECT_EQ(mask_mat.at<uint8_t>(0, 0), 1);  // 0.95 >= 0.93
    EXPECT_EQ(mask_mat.at<uint8_t>(0, 1), 1);  // 0.95 >= 0.93
    EXPECT_EQ(mask_mat.at<uint8_t>(0, 2), 0);  // 0.90 < 0.93
    EXPECT_EQ(mask_mat.at<uint8_t>(0, 3), 0);  // 0.92 < 0.93
}

TEST(IrisEncoder, EmptyResponseFails) {
    IrisFilterResponse response;
    IrisEncoder encoder;
    auto result = encoder.run(response);
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, ErrorCode::EncodingFailed);
}

TEST(IrisEncoder, NodeParamsConstruction) {
    std::unordered_map<std::string, std::string> params;
    params["mask_threshold"] = "0.85";
    IrisEncoder encoder(params);

    // Use proper 16x256 response with mask value between 0.85 and 0.93
    auto response = make_response();
    auto& resp = response.responses[0];
    resp.data = cv::Mat(16, 256, CV_64FC2, cv::Scalar(1.0, 1.0));
    resp.mask = cv::Mat(16, 256, CV_64FC2, cv::Scalar(0.88, 0.88));

    auto result = encoder.run(response);
    ASSERT_TRUE(result.has_value());

    auto [code_ignore, mask_mat] = result.value().mask_codes[0].to_mat();
    // 0.88 >= 0.85 with custom threshold -> should be valid
    EXPECT_EQ(mask_mat.at<uint8_t>(0, 0), 1);
    EXPECT_EQ(mask_mat.at<uint8_t>(0, 1), 1);
}
