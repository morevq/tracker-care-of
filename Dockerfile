# Stage 1: Build
FROM ghcr.io/userver-framework/ubuntu-22.04-userver-base:latest AS builder

WORKDIR /app

ENV VCPKG_ROOT=/opt/vcpkg
ENV PATH="${VCPKG_ROOT}:${PATH}"

RUN apt-get update && apt-get install -y \
    curl zip unzip tar git \
    && rm -rf /var/lib/apt/lists/*

RUN git clone https://github.com/microsoft/vcpkg.git ${VCPKG_ROOT} \
    && ${VCPKG_ROOT}/bootstrap-vcpkg.sh

# Copy manifests first — so userver FetchContent layer is cached independently of source changes
COPY vcpkg.json .
RUN vcpkg install

# Copy all sources before configure: vcpkg's _add_executable checks file existence at generate time
COPY . .

# cmake configure — Stage 0 check (downloads & builds userver via FetchContent, heavy but cached)
RUN cmake -B build -S . \
    -DCMAKE_TOOLCHAIN_FILE=${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake \
    -DCMAKE_BUILD_TYPE=Release

RUN cmake --build build --target tracker_api -j$(nproc)

# Stage 2: Runtime
FROM ubuntu:22.04 AS api

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

COPY --from=builder /app/build/tracker_api ./tracker_api
COPY --from=builder /app/apps/api/static_config.yaml ./static_config.yaml
COPY --from=builder /app/apps/api/dynamic_config_vars.yaml ./dynamic_config_vars.yaml
COPY --from=builder /app/apps/api/secdist.json /etc/tracker/secdist.json

EXPOSE 8080

CMD ["./tracker_api", \
     "--config", "static_config.yaml", \
     "--config_vars", "dynamic_config_vars.yaml"]
