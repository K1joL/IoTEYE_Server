# ==========================================
# STAGE 1: Build Environment
# ==========================================
FROM ubuntu:24.04 AS builder

# Install build dependencies (CMake, compiler, Git)
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
	wget \
    nlohmann-json3-dev \
    libgtest-dev \
    libssl-dev \
    git \
    && rm -rf /var/lib/apt/lists/*

# Build Boost 1.88.0 from source (Only program_options to save time)
WORKDIR /tmp
RUN wget https://archives.boost.io/release/1.88.0/source/boost_1_88_0.tar.gz && \
    tar -xzf boost_1_88_0.tar.gz && \
    cd boost_1_88_0 && \
    ./bootstrap.sh --prefix=/usr/local && \
    ./b2 --with-program_options -j$(nproc) install && \
    rm -rf /tmp/boost*
	
# Set up the working directory
WORKDIR /src

# Add a dummy argument to bust the cache here
ARG CACHEBUST=1

# Create a dedicated, clean directory for the library install outside the build tree
RUN mkdir -p /src/ioteye_lib

# Clone the public library and server app repositories
RUN git clone -b develop https://github.com/K1joL/IoTeyeHttpServer.git
RUN git clone -b develop https://github.com/K1joL/IoTeyeServerApp.git

# ----------------------------------------------------
# 1. Build the library
# ----------------------------------------------------
WORKDIR /src/IoTeyeHttpServer/build
RUN cmake -DCMAKE_BUILD_TYPE=Debug -DIOTEYE_USE_BOOST_ASIO=ON .. && \
    make -j$(nproc) && \
    make install

# ----------------------------------------------------
# 1.5 Install jwt-cpp (Header-only library)
# ----------------------------------------------------
WORKDIR /src
RUN git clone https://github.com/Thalhammer/jwt-cpp.git && \
    cd jwt-cpp && \
    mkdir build && cd build && \
    cmake -DCMAKE_INSTALL_PREFIX=/usr/local .. && \
    make install

# ----------------------------------------------------
# 2. Build the server application
# ----------------------------------------------------
WORKDIR /src/IoTeyeServerApp/build
RUN cmake -DCMAKE_BUILD_TYPE=Debug .. && \
    make -j$(nproc)
	
# ==========================================
# STAGE 2: Minimal Runtime Environment
# ==========================================
FROM ubuntu:24.04 AS runtime

RUN apt-get update && apt-get install -y \
    libstdc++6 \
    libssl3 \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

COPY --from=builder /usr/local/lib/libboost_program_options.so.1.88.0 /usr/local/lib/
RUN ldconfig

RUN useradd -m appuser
USER appuser
WORKDIR /home/appuser

COPY --from=builder /src/IoTeyeServerApp/build/app/ioteye ./ioteye

EXPOSE 8080
EXPOSE 8081

# Run the copied file
CMD ["./ioteye"]