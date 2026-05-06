# IoTeye Server App

IoTeye Server App is an advanced usage example of the [IoTeye Server Library](https://github.com/K1joL/IoTeyeHttpServer). This application provides a robust and efficient server implementation for handling HTTP requests, suitable for embedded systems and IoT devices.

## Getting Started

### Prerequisites

- C++17 or later
- CMake 3.18 or later
- ASIO 1.30.2 or later
- [IoTeye Server Library](https://github.com/K1joL/IoTeyeHttpServer)

### Installation

1. **Clone the repository**:
   ```sh
   git clone https://github.com/K1joL/IoTeyeServerApp
   cd IoTeyeServerApp
   ```

2. **Build the project**:
   ```sh
   mkdir build
   cd build
   cmake .. && cmake --build .
   ```
    ---
    **Build Options**

    The project supports several build options that can be configured via CMake:

    - **CMAKE_BUILD_TYPE**: Specifies the build type. The default is `Release`. When set to `Debug`, it will output debug information.
    ```sh
    cmake -DCMAKE_BUILD_TYPE=Debug ..
    ```
     - **IOTEYE_BUILD_CLIENT**: Specifies whether to build the client. The default is `OFF`.
    ```sh
    cmake -DIOTEYE_BUILD_CLIENT=ON ..
    ```
    ---
3. **Run the Server**:
   ```sh
   ./main
   ```
4. **Run the Client (optional)**:
    ```sh
    ./client/ioteyeClient
    ```
    
## Docker


### 1. Build the Image

```bash
docker build -t ioteye-server .
```

> **Note:** The first build will take several minutes because it downloads and compiles Boost and the C++ libraries from source. Subsequent builds will be much faster.

### 2. Run the Server
```bash
docker run -d \
  -p 8080:8080 \
  -p 8081:8081 \
  --name ioteye-instance \
  ioteye-server \
  --maxPins 100 --outdated 1000 --offline 5000
```


## License
This project is licensed under the MIT License - see the [COPYING](COPYING) file for details.
