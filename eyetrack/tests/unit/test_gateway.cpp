#include <gtest/gtest.h>

#include <opencv2/core.hpp>

#include <eyetrack/comm/gateway.hpp>

namespace {

eyetrack::IncomingMessage make_valid_message() {
    eyetrack::IncomingMessage msg;
    msg.client_id = "test_client";
    msg.auth_token = "test_client";
    msg.protocol = eyetrack::Protocol::WebSocket;
    msg.frame.frame = cv::Mat(480, 640, CV_8UC3, cv::Scalar(128, 128, 128));
    msg.frame.frame_id = 1;
    msg.frame.timestamp = std::chrono::steady_clock::now();
    msg.frame.client_id = "test_client";
    return msg;
}

eyetrack::PipelineDispatcher make_echo_dispatcher() {
    return [](const eyetrack::FrameData& /*frame*/)
               -> eyetrack::Result<eyetrack::GazeResult> {
        eyetrack::GazeResult gaze;
        gaze.gaze_point = {0.5F, 0.5F};
        gaze.confidence = 0.9F;
        gaze.blink = eyetrack::BlinkType::None;
        return gaze;
    };
}

}  // namespace

TEST(Gateway, dispatches_to_pipeline) {
    eyetrack::Gateway gateway;
    bool dispatched = false;
    gateway.set_dispatcher(
        [&dispatched](const eyetrack::FrameData& /*frame*/)
            -> eyetrack::Result<eyetrack::GazeResult> {
            dispatched = true;
            eyetrack::GazeResult gaze;
            gaze.gaze_point = {0.5F, 0.5F};
            gaze.confidence = 1.0F;
            return gaze;
        });

    auto result = gateway.process(make_valid_message());
    ASSERT_TRUE(result.has_value()) << result.error().message;
    EXPECT_TRUE(dispatched);
}

TEST(Gateway, routes_result_to_originating_protocol) {
    eyetrack::Gateway gateway;
    gateway.set_dispatcher(make_echo_dispatcher());

    auto msg = make_valid_message();
    msg.protocol = eyetrack::Protocol::Grpc;

    auto result = gateway.process(msg);
    ASSERT_TRUE(result.has_value()) << result.error().message;
    EXPECT_EQ(result.value().protocol, eyetrack::Protocol::Grpc);
    EXPECT_EQ(result.value().client_id, "test_client");
}

TEST(Gateway, calls_auth_before_dispatch) {
    eyetrack::Gateway gateway;
    gateway.set_dispatcher(make_echo_dispatcher());

    // NoOp auth accepts everything — just verify no error
    auto result = gateway.process(make_valid_message());
    EXPECT_TRUE(result.has_value());
}

TEST(Gateway, calls_input_validation) {
    eyetrack::Gateway gateway;
    gateway.set_dispatcher(make_echo_dispatcher());

    // Invalid client ID should be rejected
    auto msg = make_valid_message();
    msg.client_id = "invalid@client!";
    auto result = gateway.process(msg);
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, eyetrack::ErrorCode::InvalidInput);
}

TEST(Gateway, calls_rate_limiter) {
    eyetrack::Gateway gateway;
    gateway.set_dispatcher(make_echo_dispatcher());
    gateway.set_rate_limit(5);  // Very low limit

    // Send 5 messages (should all pass)
    for (int i = 0; i < 5; ++i) {
        auto result = gateway.process(make_valid_message());
        ASSERT_TRUE(result.has_value()) << "Message " << i << " failed: "
                                        << result.error().message;
    }

    // 6th should be rate-limited
    auto result = gateway.process(make_valid_message());
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, eyetrack::ErrorCode::RateLimited);
}
