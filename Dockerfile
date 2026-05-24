# ==========================================
# STAGE 1: Build Environment
# ==========================================
FROM ubuntu:24.04 AS builder

# Improve apt reliability from Docker
RUN printf 'Acquire::Retries "10";\nAcquire::http::Timeout "120";\nAcquire::ForceIPv4 "true";\n' > /etc/apt/apt.conf.d/99docker

# Use kernel.org mirror if archive.ubuntu.com is flaky from your network
RUN sed -i 's|http://archive.ubuntu.com/ubuntu|http://mirrors.edge.kernel.org/ubuntu|g' /etc/apt/sources.list.d/ubuntu.sources

# Install build dependencies (CMake, compiler, Git)
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
	wget \
    nlohmann-json3-dev \
    libgtest-dev \
    libssl-dev \
    libboost-program-options-dev \
    git \
    && mkdir -p /mark && touch /mark/deps-ready \
    && rm -rf /var/lib/apt/lists/*

# Set up the working directory
WORKDIR /src

# Copy the local source code into the container
COPY . /src/IoTeyeServerApp

# Build the server application
# -DFETCH_LIBIOTEYESERVER=ON will trigger FetchContent for ioteyeserver
# jwt-cpp is also handled automatically by FetchContent in CMakeLists.txt
WORKDIR /src/IoTeyeServerApp/build
RUN cmake -DCMAKE_BUILD_TYPE=Debug -DFETCH_LIBIOTEYESERVER=ON -DFETCH_JWT_CPP=ON .. && \
    make -j$(nproc)
    
# ==========================================
# STAGE 2: Minimal Runtime Environment
# ==========================================
FROM ubuntu:24.04 AS runtime

# Serialize apt: avoid parallel apt-get with builder (both hammer Ubuntu mirrors at once under BuildKit)
COPY --from=builder /mark/deps-ready /tmp/.builder-apt-done

RUN printf 'Acquire::Retries "10";\nAcquire::http::Timeout "120";\nAcquire::ForceIPv4 "true";\n' > /etc/apt/apt.conf.d/99docker
RUN sed -i 's|http://archive.ubuntu.com/ubuntu|http://mirrors.edge.kernel.org/ubuntu|g' /etc/apt/sources.list.d/ubuntu.sources

# Install only runtime dependencies
RUN apt-get update && apt-get install -y \
    libstdc++6 \
    libssl3 \
    ca-certificates \
    libboost-program-options1.83.0 \
    && rm -rf /var/lib/apt/lists/*

RUN useradd -m appuser

# Copy the built binary from the builder stage
COPY --from=builder /src/IoTeyeServerApp/build/app/ioteye /usr/local/bin/ioteye
RUN chmod 755 /usr/local/bin/ioteye

USER appuser
WORKDIR /home/appuser

EXPOSE 8080
EXPOSE 8081

# Set the binary as the main executable for the container
ENTRYPOINT ["/usr/local/bin/ioteye"]

# Provide default arguments
CMD ["--historyInterval", "2000", "--historyMax", "5000", "--historyFile", "pin_history.jsonl"]