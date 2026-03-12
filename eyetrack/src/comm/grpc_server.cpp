#ifdef EYETRACK_HAS_GRPC

#include <eyetrack/comm/grpc_server.hpp>

#include <filesystem>
#include <fstream>
#include <sstream>

#include <grpcpp/security/server_credentials.h>

#include <eyetrack.grpc.pb.h>
#include <eyetrack.pb.h>
#include <eyetrack/utils/proto_utils.hpp>

namespace eyetrack {

namespace {

/// gRPC service implementation.
class EyeTrackServiceImpl final : public proto::EyeTrackService::Service {
public:
    explicit EyeTrackServiceImpl(std::atomic<uint32_t>& client_count)
        : client_count_(client_count) {}

    grpc::Status StreamGaze(
        grpc::ServerContext* /*context*/,
        const proto::StreamGazeRequest* request,
        grpc::ServerWriter<proto::GazeEvent>* writer) override {

        client_count_.fetch_add(1, std::memory_order_relaxed);

        // Placeholder: in a real implementation, this would subscribe
        // to the pipeline and stream events. For now, send one event.
        proto::GazeEvent event;
        auto* gaze = event.mutable_gaze_point();
        gaze->set_x(0.5F);
        gaze->set_y(0.5F);
        event.set_confidence(1.0F);
        event.set_blink(proto::BLINK_NONE);
        event.set_client_id(request->client_id());
        writer->Write(event);

        client_count_.fetch_sub(1, std::memory_order_relaxed);
        return grpc::Status::OK;
    }

    grpc::Status ProcessFrame(
        grpc::ServerContext* /*context*/,
        const proto::FrameData* request,
        proto::GazeEvent* response) override {

        // Placeholder: process frame through pipeline
        auto* gaze = response->mutable_gaze_point();
        gaze->set_x(0.5F);
        gaze->set_y(0.5F);
        response->set_confidence(1.0F);
        response->set_blink(proto::BLINK_NONE);
        response->set_client_id(request->client_id());
        response->set_frame_id(request->frame_id());
        return grpc::Status::OK;
    }

    grpc::Status Calibrate(
        grpc::ServerContext* /*context*/,
        const proto::CalibrationRequest* request,
        proto::CalibrationResponse* response) override {

        if (request->points_size() < 3) {
            response->set_success(false);
            response->set_error_message("At least 3 calibration points required");
            return grpc::Status::OK;
        }

        // Placeholder: run calibration
        response->set_success(true);
        response->set_profile_id(request->user_id());
        return grpc::Status::OK;
    }

    grpc::Status GetStatus(
        grpc::ServerContext* /*context*/,
        const proto::StatusRequest* /*request*/,
        proto::StatusResponse* response) override {

        response->set_is_running(true);
        response->set_connected_clients(
            client_count_.load(std::memory_order_relaxed));
        response->set_fps(30.0F);
        response->set_version("1.0.0");
        return grpc::Status::OK;
    }

private:
    std::atomic<uint32_t>& client_count_;
};

std::string read_file(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) return {};
    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

}  // namespace

GrpcServer::GrpcServer(const ServerConfig& config) : config_(config) {}

GrpcServer::~GrpcServer() {
    stop();
}

Result<std::shared_ptr<grpc::ServerCredentials>>
GrpcServer::build_credentials() const {
    if (!config_.tls.enabled) {
        return grpc::InsecureServerCredentials();
    }

    // Validate cert paths exist
    if (!std::filesystem::exists(config_.tls.cert_path)) {
        return make_error(ErrorCode::ConfigInvalid,
                          "TLS cert file not found: " + config_.tls.cert_path);
    }
    if (!std::filesystem::exists(config_.tls.key_path)) {
        return make_error(ErrorCode::ConfigInvalid,
                          "TLS key file not found: " + config_.tls.key_path);
    }

    auto cert = read_file(config_.tls.cert_path);
    auto key = read_file(config_.tls.key_path);

    if (cert.empty() || key.empty()) {
        return make_error(ErrorCode::ConfigInvalid,
                          "Failed to read TLS cert or key file");
    }

    grpc::SslServerCredentialsOptions ssl_opts;
    ssl_opts.pem_key_cert_pairs.push_back({key, cert});

    if (!config_.tls.ca_path.empty() &&
        std::filesystem::exists(config_.tls.ca_path)) {
        ssl_opts.pem_root_certs = read_file(config_.tls.ca_path);
    }

    return grpc::SslServerCredentials(ssl_opts);
}

Result<void> GrpcServer::start() {
    if (running_.load(std::memory_order_acquire)) {
        return make_error(ErrorCode::ConfigInvalid,
                          "Server already running");
    }

    auto creds = build_credentials();
    if (!creds.has_value()) {
        return std::unexpected(creds.error());
    }

    std::string address =
        config_.host + ":" + std::to_string(config_.grpc_port);

    auto service = std::make_unique<EyeTrackServiceImpl>(client_count_);

    grpc::ServerBuilder builder;
    builder.AddListeningPort(address, creds.value());
    builder.RegisterService(service.get());

    server_ = builder.BuildAndStart();
    if (!server_) {
        return make_error(ErrorCode::ConnectionFailed,
                          "Failed to start gRPC server on " + address);
    }

    // Transfer ownership: the server keeps the service alive
    // We store it as a member to prevent destruction
    service.release();  // NOLINT — server owns the service via RegisterService

    running_.store(true, std::memory_order_release);
    return {};
}

void GrpcServer::stop() {
    if (running_.exchange(false, std::memory_order_acq_rel)) {
        if (server_) {
            server_->Shutdown();
            server_.reset();
        }
    }
}

bool GrpcServer::is_running() const noexcept {
    return running_.load(std::memory_order_acquire);
}

uint32_t GrpcServer::connected_clients() const noexcept {
    return client_count_.load(std::memory_order_relaxed);
}

}  // namespace eyetrack

#endif  // EYETRACK_HAS_GRPC
