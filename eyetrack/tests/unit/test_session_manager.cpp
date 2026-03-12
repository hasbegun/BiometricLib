#include <gtest/gtest.h>

#include <thread>
#include <vector>

#include <eyetrack/comm/session_manager.hpp>

TEST(SessionManager, add_client_session) {
    eyetrack::SessionManager mgr;
    auto result = mgr.add("client_1", eyetrack::Protocol::Grpc);
    ASSERT_TRUE(result.has_value()) << result.error().message;
    EXPECT_TRUE(mgr.has("client_1"));
    EXPECT_EQ(mgr.count(), 1U);
}

TEST(SessionManager, remove_client_session) {
    eyetrack::SessionManager mgr;
    (void)mgr.add("client_1", eyetrack::Protocol::Grpc);
    EXPECT_TRUE(mgr.has("client_1"));

    mgr.remove("client_1");
    EXPECT_FALSE(mgr.has("client_1"));
    EXPECT_EQ(mgr.count(), 0U);
}

TEST(SessionManager, get_client_protocol) {
    eyetrack::SessionManager mgr;
    (void)mgr.add("client_1", eyetrack::Protocol::Grpc);

    auto session = mgr.get("client_1");
    ASSERT_TRUE(session.has_value());
    EXPECT_EQ(session->protocol, eyetrack::Protocol::Grpc);
    EXPECT_EQ(session->client_id, "client_1");
}

TEST(SessionManager, get_calibration_state) {
    eyetrack::SessionManager mgr;
    (void)mgr.add("client_1", eyetrack::Protocol::WebSocket);

    auto session = mgr.get("client_1");
    ASSERT_TRUE(session.has_value());
    EXPECT_EQ(session->calibration, eyetrack::CalibrationState::NotCalibrated);

    auto result = mgr.set_calibration("client_1",
                                       eyetrack::CalibrationState::Calibrated);
    ASSERT_TRUE(result.has_value()) << result.error().message;

    session = mgr.get("client_1");
    EXPECT_EQ(session->calibration, eyetrack::CalibrationState::Calibrated);
}

TEST(SessionManager, multiple_clients_tracked) {
    eyetrack::SessionManager mgr;
    for (int i = 0; i < 5; ++i) {
        (void)mgr.add("client_" + std::to_string(i), eyetrack::Protocol::Mqtt);
    }
    EXPECT_EQ(mgr.count(), 5U);

    for (int i = 0; i < 5; ++i) {
        EXPECT_TRUE(mgr.has("client_" + std::to_string(i)));
    }
}

TEST(SessionManager, unknown_client_returns_error) {
    eyetrack::SessionManager mgr;
    auto session = mgr.get("nonexistent");
    EXPECT_FALSE(session.has_value());

    auto result = mgr.set_calibration("nonexistent",
                                       eyetrack::CalibrationState::Calibrated);
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, eyetrack::ErrorCode::InvalidInput);
}

TEST(SessionManager, concurrent_add_remove) {
    eyetrack::SessionManager mgr;
    constexpr int kThreads = 8;
    constexpr int kOpsPerThread = 100;

    std::vector<std::thread> threads;
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&mgr, t] {
            for (int i = 0; i < kOpsPerThread; ++i) {
                std::string id = "t";
                id += std::to_string(t);
                id += "_";
                id += std::to_string(i);
                (void)mgr.add(id, eyetrack::Protocol::WebSocket);
                (void)mgr.get(id);
                mgr.remove(id);
            }
        });
    }
    for (auto& th : threads) {
        th.join();
    }
    // All temporary clients removed
    EXPECT_EQ(mgr.count(), 0U);
}
