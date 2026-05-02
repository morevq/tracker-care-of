# syntax=docker/dockerfile:1.7
# Stage 1: Build
FROM ghcr.io/userver-framework/ubuntu-22.04-userver-base:latest AS builder

WORKDIR /app

ENV VCPKG_ROOT=/opt/vcpkg
ENV PATH="${VCPKG_ROOT}:${PATH}"

RUN apt-get update && apt-get install -y \
    curl zip unzip tar git \
    && rm -rf /var/lib/apt/lists/*

RUN git config --global http.version HTTP/1.1 \
 && git config --global http.postBuffer 1048576000 \
 && git config --global http.lowSpeedLimit 0 \
 && git config --global http.lowSpeedTime 999999

RUN git clone --depth 1 https://github.com/microsoft/vcpkg.git ${VCPKG_ROOT} \
    && ${VCPKG_ROOT}/bootstrap-vcpkg.sh -disableMetrics

# Copy all sources before configure: vcpkg's _add_executable checks file existence at generate time
COPY . .

# Configure + build in one RUN with BuildKit cache mounts:
#   /root/.cache/vcpkg     — vcpkg binary cache (restored on subsequent builds, skips compilation)
#   /opt/vcpkg/buildtrees  — vcpkg source/build dirs
#   /root/.ccache          — ccache (huge win for userver rebuilds)
#   /app/build             — CMake build dir; preserves userver FetchContent + object files
# The binary must be copied OUT of /app/build before the RUN ends, because cache mounts
# are not persisted into the image layer.
RUN --mount=type=cache,target=/root/.cache/vcpkg \
    --mount=type=cache,target=/opt/vcpkg/buildtrees \
    --mount=type=cache,target=/root/.ccache \
    --mount=type=cache,target=/app/build,id=tracker-build \
    cmake -B build -S . \
      -DCMAKE_TOOLCHAIN_FILE=${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake \
      -DCMAKE_BUILD_TYPE=Release \
      -DUSERVER_FEATURE_UTEST=OFF \
      -DUSERVER_FEATURE_TESTSUITE=OFF \
 && cmake --build build --target tracker_api -j$(nproc) \
 && cp build/tracker_api /app/tracker_api

# Stage 2: Runtime
FROM --platform=linux/amd64 ubuntu:22.04 AS api

WORKDIR /app

RUN apt-get update && apt-get install -y \
    libssl3 \
    libpq5 \
    libhiredis0.14 \
    libboost-atomic1.74.0 \
    libboost-filesystem1.74.0 \
    libboost-iostreams1.74.0 \
    libboost-program-options1.74.0 \
    libboost-stacktrace1.74.0 \
    libc-ares2 \
    libev4 \
    libnghttp2-14 \
    libcrypto++8 \
    libjemalloc2 \
    libre2-9 \
    libcctz2 \
    libyaml-cpp0.7 \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

COPY --from=builder /app/tracker_api ./tracker_api
COPY --from=builder /app/apps/api/static_config.yaml ./static_config.yaml
COPY --from=builder /app/apps/api/dynamic_config_vars.yaml ./dynamic_config_vars.yaml
COPY --from=builder /app/apps/api/secdist.json /etc/tracker/secdist.json

EXPOSE 8080

CMD ["./tracker_api", \
     "--config", "static_config.yaml", \
     "--config_vars", "dynamic_config_vars.yaml"]
