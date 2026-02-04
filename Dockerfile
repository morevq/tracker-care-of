# Stage 1: Build
FROM ubuntu:22.04 AS builder

# Disable interactive prompts
ENV DEBIAN_FRONTEND=noninteractive

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    git \
    curl \
    zip \
    unzip \
    tar \
    pkg-config \
    ninja-build \
    python3 \
    python3-pip \
    linux-libc-dev \
    bison \
    flex \
    autoconf \
    automake \
    libtool \
    && rm -rf /var/lib/apt/lists/*

# Install modern CMake via pip
RUN pip3 install cmake --upgrade

# Set up vcpkg
ENV VCPKG_ROOT=/opt/vcpkg
ENV PATH="${VCPKG_ROOT}:${PATH}"

RUN git clone https://github.com/microsoft/vcpkg.git ${VCPKG_ROOT} \
    && ${VCPKG_ROOT}/bootstrap-vcpkg.sh

WORKDIR /app

# Copy vcpkg manifest to cache dependencies
COPY vcpkg.json .

# Install dependencies (Manifest mode)
RUN vcpkg install

# Copy source code
COPY . .

# Configure and Build
RUN cmake -B build -S . \
    -DCMAKE_TOOLCHAIN_FILE=${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake \
    -DCMAKE_BUILD_TYPE=Release \
    && cmake --build build --config Release --target tracker_api

# Stage 2: Runtime (API)
FROM ubuntu:22.04 AS api

WORKDIR /app

# Install runtime dependencies
# libpq5: for PostgreSQL
# libcurl4, libssl3: for CPR/network
RUN apt-get update && apt-get install -y \
    libpq5 \
    libcurl4 \
    libssl3 \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# Create directory for Swagger UI assets
RUN mkdir -p apps/api

# Copy artifacts
COPY --from=builder /app/build/tracker_api ./tracker_api
COPY --from=builder /app/apps/api/swagger.json ./apps/api/swagger.json
COPY .env.example .env.example

# Expose API port
EXPOSE 8080

CMD ["./tracker_api"]