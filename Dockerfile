# Multi-Stage Build & Sandboxed Runtime Container for Gravity VM (Luna Language)
# Containerized Execution Sandbox

# Stage 1: Build stage
FROM ubuntu:24.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    clang-18 \
    llvm-18 \
    cmake \
    ninja-build \
    git \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . /app

RUN cmake -B build/release -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_COMPILER=clang++-18 \
    -DCMAKE_C_COMPILER=clang-18

RUN cmake --build build/release --target luna luna_tests luna_bench

RUN ctest --test-dir build/release --output-on-failure

# Stage 2: Minimal runtime image
FROM ubuntu:24.04 AS runtime

RUN useradd -m -s /bin/bash luna && \
    mkdir -p /sandbox && \
    chown -R luna:luna /sandbox

COPY --from=builder /app/build/release/src/luna /usr/local/bin/luna
COPY --from=builder /app/build/release/tests/luna_tests /usr/local/bin/luna_tests
COPY --from=builder /app/build/release/bench/luna_bench /usr/local/bin/luna_bench

USER luna
WORKDIR /sandbox

ENTRYPOINT ["luna"]
CMD ["--help"]
