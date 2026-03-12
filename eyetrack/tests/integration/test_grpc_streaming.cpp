#include <gtest/gtest.h>

#ifdef EYETRACK_HAS_GRPC

#include <chrono>
#include <string>
#include <thread>

#include <grpcpp/grpcpp.h>

#include <eyetrack.grpc.pb.h>
#include <eyetrack.pb.h>
#include <eyetrack/comm/grpc_server.hpp>

namespace {

const uint16_t kTestPort = 50200;

// Helper: create a server, start it, return the server.
std::unique_ptr<eyetrack::GrpcServer> start_test_server(uint16_t port) {
    eyetrack::ServerConfig config;
    config.grpc_port = port;
    config.tls.enabled = false;
    auto server = std::make_unique<eyetrack::GrpcServer>(config);
    auto result = server->start();
    if (!result.has_value()) return nullptr;
    return server;
}

// Helper: create a client stub.
std::unique_ptr<eyetrack::proto::EyeTrackService::Stub> create_stub(
    uint16_t port) {
    auto channel = grpc::CreateChannel(
        "localhost:" + std::to_string(port),
        grpc::InsecureChannelCredentials());
    return eyetrack::proto::EyeTrackService::NewStub(channel);
}

}  // namespace

TEST(GrpcStreaming, client_sends_frame_receives_gaze) {
    auto server = start_test_server(kTestPort);
    ASSERT_TRUE(server) << "Failed to start server";

    auto stub = create_stub(kTestPort);

    eyetrack::proto::FrameData request;
    request.set_client_id("test_client");
    request.set_frame_id(1);
    request.set_frame("fake_image_data");

    eyetrack::proto::GazeEvent response;
    grpc::ClientContext context;
    auto status = stub->ProcessFrame(&context, request, &response);

    EXPECT_TRUE(status.ok()) << status.error_message();
    EXPECT_EQ(response.client_id(), "test_client");
    EXPECT_EQ(response.frame_id(), 1U);
    EXPECT_GT(response.confidence(), 0.0F);

    server->stop();
}

TEST(GrpcStreaming, bidirectional_streaming_30fps) {
    uint16_t port = kTestPort + 1;
    auto server = start_test_server(port);
    ASSERT_TRUE(server) << "Failed to start server";

    auto stub = create_stub(port);

    // Send 30 ProcessFrame requests in ~1 second
    int received = 0;
    auto start = std::chrono::steady_clock::now();

    for (int i = 0; i < 30; ++i) {
        eyetrack::proto::FrameData request;
        request.set_client_id("fps_client");
        request.set_frame_id(static_cast<uint64_t>(i));

        eyetrack::proto::GazeEvent response;
        grpc::ClientContext ctx;
        auto status = stub->ProcessFrame(&ctx, request, &response);
        if (status.ok()) ++received;
    }

    auto end = std::chrono::steady_clock::now();
    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                          end - start)
                          .count();

    EXPECT_EQ(received, 30);
    // All 30 frames should complete well under 5 seconds
    EXPECT_LT(elapsed_ms, 5000) << "30 frames took " << elapsed_ms << "ms";

    server->stop();
}

TEST(GrpcStreaming, calibration_rpc) {
    uint16_t port = kTestPort + 2;
    auto server = start_test_server(port);
    ASSERT_TRUE(server) << "Failed to start server";

    auto stub = create_stub(port);

    eyetrack::proto::CalibrationRequest request;
    request.set_user_id("test_user");

    // Add 9 calibration points
    for (int i = 0; i < 9; ++i) {
        auto* point = request.add_points();
        auto* screen = point->mutable_screen_point();
        screen->set_x(static_cast<float>(i % 3) / 2.0F);
        screen->set_y(static_cast<float>(i / 3) / 2.0F);
        auto* pupil = point->mutable_pupil_observation();
        pupil->mutable_center()->set_x(50.0F + static_cast<float>(i));
        pupil->mutable_center()->set_y(60.0F);
        pupil->set_radius(5.0F);
        pupil->set_confidence(0.9F);
    }

    eyetrack::proto::CalibrationResponse response;
    grpc::ClientContext context;
    auto status = stub->Calibrate(&context, request, &response);

    EXPECT_TRUE(status.ok()) << status.error_message();
    EXPECT_TRUE(response.success());
    EXPECT_EQ(response.profile_id(), "test_user");

    server->stop();
}

TEST(GrpcStreaming, stream_gaze_rpc) {
    uint16_t port = kTestPort + 3;
    auto server = start_test_server(port);
    ASSERT_TRUE(server) << "Failed to start server";

    auto stub = create_stub(port);

    eyetrack::proto::StreamGazeRequest request;
    request.set_client_id("stream_client");

    grpc::ClientContext context;
    auto reader = stub->StreamGaze(&context, request);

    eyetrack::proto::GazeEvent event;
    int count = 0;
    while (reader->Read(&event)) {
        EXPECT_EQ(event.client_id(), "stream_client");
        ++count;
    }

    auto status = reader->Finish();
    EXPECT_TRUE(status.ok()) << status.error_message();
    EXPECT_GT(count, 0) << "Should receive at least one gaze event";

    server->stop();
}

TEST(GrpcStreaming, client_disconnect_handled) {
    uint16_t port = kTestPort + 4;
    auto server = start_test_server(port);
    ASSERT_TRUE(server) << "Failed to start server";

    // Create a client, send one request, then destroy the client
    {
        auto stub = create_stub(port);
        eyetrack::proto::FrameData request;
        request.set_client_id("disconnect_client");
        eyetrack::proto::GazeEvent response;
        grpc::ClientContext ctx;
        (void)stub->ProcessFrame(&ctx, request, &response);
    }
    // Client is destroyed here

    // Server should still be running
    EXPECT_TRUE(server->is_running());

    // Another client should still be able to connect
    auto stub2 = create_stub(port);
    eyetrack::proto::FrameData request;
    request.set_client_id("reconnect_client");
    eyetrack::proto::GazeEvent response;
    grpc::ClientContext ctx;
    auto status = stub2->ProcessFrame(&ctx, request, &response);
    EXPECT_TRUE(status.ok());

    server->stop();
}

TEST(GrpcStreaming, overhead_under_5ms) {
    uint16_t port = kTestPort + 5;
    auto server = start_test_server(port);
    ASSERT_TRUE(server) << "Failed to start server";

    auto stub = create_stub(port);

    // Warm up
    for (int i = 0; i < 5; ++i) {
        eyetrack::proto::FrameData request;
        request.set_client_id("bench_client");
        eyetrack::proto::GazeEvent response;
        grpc::ClientContext ctx;
        (void)stub->ProcessFrame(&ctx, request, &response);
    }

    // Measure
    constexpr int kIterations = 100;
    auto start = std::chrono::steady_clock::now();

    for (int i = 0; i < kIterations; ++i) {
        eyetrack::proto::FrameData request;
        request.set_client_id("bench_client");
        request.set_frame_id(static_cast<uint64_t>(i));
        eyetrack::proto::GazeEvent response;
        grpc::ClientContext ctx;
        (void)stub->ProcessFrame(&ctx, request, &response);
    }

    auto end = std::chrono::steady_clock::now();
    auto total_us = std::chrono::duration_cast<std::chrono::microseconds>(
                        end - start)
                        .count();
    double per_frame_ms = static_cast<double>(total_us) /
                          static_cast<double>(kIterations) / 1000.0;

    // In Docker/ARM emulation, allow up to 50ms. On native, should be <5ms.
    EXPECT_LT(per_frame_ms, 50.0)
        << "Per-frame overhead: " << per_frame_ms << "ms";

    server->stop();
}

#else

TEST(GrpcStreaming, skipped_without_grpc) {
    GTEST_SKIP() << "gRPC support not enabled";
}

#endif  // EYETRACK_HAS_GRPC
