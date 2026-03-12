# =============================================================================
# Stage 1: base-deps — OS + all third-party libraries
# =============================================================================
FROM ubuntu:24.04 AS base-deps

ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=UTC

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    g++-13 \
    cmake \
    ninja-build \
    ccache \
    git \
    curl \
    ca-certificates \
    pkg-config \
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
    # GTest
    libgtest-dev \
    libgmock-dev \
    # Google Benchmark
    libbenchmark-dev \
    # ONNX Runtime build deps
    wget \
    # Python (for V&V analysis scripts)
    python3 \
    python3-pip \
    python3-opencv \
    && pip3 install --break-system-packages numpy matplotlib \
    && rm -rf /var/lib/apt/lists/*

# Set GCC 13 as default
RUN update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-13 100 \
    --slave /usr/bin/g++ g++ /usr/bin/g++-13

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

# =============================================================================
# Stage 2: openfhe — build OpenFHE from source
# =============================================================================
FROM base-deps AS openfhe

ARG OPENFHE_VERSION=v1.4.2

RUN git clone --depth 1 --branch ${OPENFHE_VERSION} \
    https://github.com/openfheorg/openfhe-development.git /tmp/openfhe && \
    cmake -S /tmp/openfhe -B /tmp/openfhe/build \
        -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=/opt/openfhe \
        -DBUILD_STATIC=ON \
        -DBUILD_SHARED=OFF \
        -DWITH_BE2=ON \
        -DWITH_BE4=OFF \
        -DWITH_NTL=OFF \
        -DBUILD_UNITTESTS=OFF \
        -DBUILD_EXAMPLES=OFF \
        -DBUILD_BENCHMARKS=OFF \
        -DNATIVE_SIZE=64 && \
    cmake --build /tmp/openfhe/build --parallel $(nproc) && \
    cmake --install /tmp/openfhe/build && \
    rm -rf /tmp/openfhe

# =============================================================================
# Stage 3: build — compile the iris library and run tests
# =============================================================================
FROM base-deps AS build

# Copy OpenFHE from stage 2
COPY --from=openfhe /opt/openfhe /opt/openfhe
ENV OpenFHE_DIR=/opt/openfhe/lib/OpenFHE

WORKDIR /src
COPY . .

# Remove macOS resource fork / AppleDouble files that macOS hosts inject
RUN find . -name '._*' -delete && find . -name '__MACOSX' -exec rm -rf {} + 2>/dev/null || true

# Configure
RUN cmake -S . -B build \
    -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_FLAGS="-march=native" \
    -DIRIS_ENABLE_TESTS=ON \
    -DIRIS_ENABLE_BENCH=ON \
    -DIRIS_ENABLE_FHE=ON \
    -DONNXRUNTIME_ROOT=/opt/onnxruntime

# Build
RUN cmake --build build --parallel $(nproc)

# Test
RUN cd build && ctest --output-on-failure

# =============================================================================
# Stage 4: runtime — minimal production image
# =============================================================================
FROM ubuntu:24.04 AS runtime

RUN apt-get update && apt-get install -y --no-install-recommends \
    libopencv-core4.6d \
    libopencv-imgproc4.6d \
    libopencv-imgcodecs4.6d \
    libyaml-cpp0.8 \
    libspdlog1.12 \
    && rm -rf /var/lib/apt/lists/* \
    && useradd -r -s /usr/sbin/nologin iris

COPY --from=build /opt/onnxruntime/lib/libonnxruntime.so* /usr/lib/
COPY --from=build /src/build/libiris.* /usr/lib/
COPY --from=build /src/models/ /opt/iris/models/

USER iris
WORKDIR /opt/iris
