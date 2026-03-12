# =============================================================================
# Stage 1: base-deps — OS + all third-party libraries
# =============================================================================
FROM debian:bookworm-slim AS base-deps

ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=UTC

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    g++ \
    cmake \
    ninja-build \
    ccache \
    git \
    curl \
    ca-certificates \
    pkg-config \
    wget \
    # OpenCV
    libopencv-dev \
    # yaml-cpp
    libyaml-cpp-dev \
    # spdlog
    libspdlog-dev \
    # nlohmann-json
    nlohmann-json3-dev \
    # Eigen
    libeigen3-dev \
    # GTest + GMock
    libgtest-dev \
    libgmock-dev \
    # Google Benchmark
    libbenchmark-dev \
    # Boost (for Beast WebSocket)
    libboost-system-dev \
    libboost-thread-dev \
    # Protobuf + gRPC
    protobuf-compiler \
    libprotobuf-dev \
    libgrpc++-dev \
    libgrpc-dev \
    protobuf-compiler-grpc \
    # SSL/TLS
    libssl-dev \
    # ARM64 cross-compiler (for smoke test)
    g++-aarch64-linux-gnu \
    file \
    bzip2 \
    && rm -rf /var/lib/apt/lists/*

# --- ONNX Runtime (prebuilt) ---
ARG ONNXRUNTIME_VERSION=1.17.3
RUN ARCH=$(dpkg --print-architecture) && \
    if [ "$ARCH" = "amd64" ]; then ORT_ARCH="x64"; else ORT_ARCH="aarch64"; fi && \
    wget -q "https://github.com/microsoft/onnxruntime/releases/download/v${ONNXRUNTIME_VERSION}/onnxruntime-linux-${ORT_ARCH}-${ONNXRUNTIME_VERSION}.tgz" \
         -O /tmp/ort.tgz && \
    tar xzf /tmp/ort.tgz -C /opt/ && \
    mv /opt/onnxruntime-linux-${ORT_ARCH}-${ONNXRUNTIME_VERSION} /opt/onnxruntime && \
    rm /tmp/ort.tgz

ENV ONNXRUNTIME_ROOT=/opt/onnxruntime

# --- dlib (built from source for C++23 compat) ---
ARG DLIB_VERSION=19.24.4
RUN git clone --depth 1 --branch v${DLIB_VERSION} \
    https://github.com/davisking/dlib.git /tmp/dlib && \
    cmake -S /tmp/dlib -B /tmp/dlib/build \
        -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=/usr/local \
        -DDLIB_NO_GUI_SUPPORT=ON \
        -DDLIB_USE_CUDA=OFF \
        -DBUILD_SHARED_LIBS=ON && \
    cmake --build /tmp/dlib/build --parallel $(nproc) && \
    cmake --install /tmp/dlib/build && \
    ldconfig && \
    rm -rf /tmp/dlib

# --- Eclipse Paho MQTT C client ---
ARG PAHO_MQTT_C_VERSION=1.3.13
RUN git clone --depth 1 --branch v${PAHO_MQTT_C_VERSION} \
    https://github.com/eclipse/paho.mqtt.c.git /tmp/paho-c && \
    cmake -S /tmp/paho-c -B /tmp/paho-c/build \
        -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=/usr/local \
        -DPAHO_WITH_SSL=ON \
        -DPAHO_BUILD_SHARED=ON \
        -DPAHO_BUILD_STATIC=OFF \
        -DPAHO_BUILD_SAMPLES=OFF \
        -DPAHO_BUILD_DOCUMENTATION=OFF && \
    cmake --build /tmp/paho-c/build --parallel $(nproc) && \
    cmake --install /tmp/paho-c/build && \
    rm -rf /tmp/paho-c

# --- Eclipse Paho MQTT C++ client ---
ARG PAHO_MQTT_CPP_VERSION=1.4.0
RUN git clone --depth 1 --branch v${PAHO_MQTT_CPP_VERSION} \
    https://github.com/eclipse/paho.mqtt.cpp.git /tmp/paho-cpp && \
    cmake -S /tmp/paho-cpp -B /tmp/paho-cpp/build \
        -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=/usr/local \
        -DPAHO_BUILD_SHARED=ON \
        -DPAHO_BUILD_STATIC=OFF \
        -DPAHO_BUILD_SAMPLES=OFF \
        -DPAHO_BUILD_DOCUMENTATION=OFF && \
    cmake --build /tmp/paho-cpp/build --parallel $(nproc) && \
    cmake --install /tmp/paho-cpp/build && \
    ldconfig && \
    rm -rf /tmp/paho-cpp

# =============================================================================
# Stage 2: build — compile the eyetrack library and run tests
# =============================================================================
FROM base-deps AS build

WORKDIR /src
COPY . .

# Configure
RUN cmake -S . -B build \
    -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_FLAGS="-march=native" \
    -DEYETRACK_ENABLE_TESTS=ON \
    -DEYETRACK_ENABLE_BENCH=OFF \
    -DEYETRACK_ENABLE_GRPC=ON \
    -DEYETRACK_ENABLE_MQTT=ON \
    -DONNXRUNTIME_ROOT=/opt/onnxruntime

# Build
RUN cmake --build build --parallel $(nproc)

# Generate eye test fixtures (synthetic + real from face fixtures)
RUN build/generate_eye_fixtures /src

# =============================================================================
# Stage 3: runtime — minimal production image
# =============================================================================
FROM debian:bookworm-slim AS runtime

RUN apt-get update && apt-get install -y --no-install-recommends \
    libopencv-core406 \
    libopencv-imgproc406 \
    libopencv-imgcodecs406 \
    libyaml-cpp0.8 \
    libspdlog1.10 \
    libboost-system1.81.0 \
    libssl3 \
    && rm -rf /var/lib/apt/lists/* \
    && useradd -r -s /usr/sbin/nologin eyetrack

COPY --from=build /opt/onnxruntime/lib/libonnxruntime.so* /usr/lib/
COPY --from=build /usr/local/lib/libpaho*.so* /usr/lib/
# Library and binaries will be copied as they are built in later phases
# COPY --from=build /src/build/libeyetrack.* /usr/lib/

USER eyetrack
WORKDIR /opt/eyetrack
