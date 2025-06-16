/*# MIT License

# Copyright (c) 2025 Shults Bogdan aka K1joL

# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:

# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.

# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
*/

#include <condition_variable>
#include <ctime>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

#include "client.hpp"
using ms = std::chrono::milliseconds;

void performMeasurements(std::string server, std::string port,
                         int numMeasurements, std::mutex &mtx,
                         int &totalTimeWrite) {
    ioteye::Client ioteye(server, port);
    std::string token = ioteye.registerNewDevice();
    ioteye.createVirtualPin(token, "1", "int", "0");
    ms totalTime = ms(0);

    for (int i = 0; i < numMeasurements; ++i) {
        std::string value = std::to_string(i);
        auto start = std::chrono::high_resolution_clock::now();
        ioteye.writeVirtualPin(token, "1", value);
        auto end = std::chrono::high_resolution_clock::now();
        totalTime += std::chrono::duration_cast<ms>(end - start);
        std::this_thread::sleep_for(ms(150));
    }

    std::lock_guard<std::mutex> lock(mtx);
    totalTimeWrite += totalTime.count();
    ioteye.deleteDevice(token);
}

void runTest(std::string server, std::string port, int numThreads) {
    const int numMeasurements = 50;
    int totalTimeWrite = 0;
    std::mutex mtx;
    std::vector<std::thread> threads;

    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back(performMeasurements, server, port, numMeasurements,
                             std::ref(mtx), std::ref(totalTimeWrite));
        std::this_thread::sleep_for(ms(10));
    }

    for (auto &thread : threads) {
        thread.join();
    }
    float averageTimeWrite =
        static_cast<float>(totalTimeWrite) / (numThreads * numMeasurements);
    std::cout << "Average time for writeVirtualPin: ";
    std::cout << averageTimeWrite << " ms" << std::endl;
}

int main() {
    std::string port = "8080";
    std::string server = "192.168.0.101";

    const int startNumThreads = 10;
    const int numTests = 5;
    const int scaleThreads = 2;
    int numThreads = startNumThreads;
    for (int test = 0; test < numTests; ++test) {
        std::cout << "Test with " << numThreads << " threads" << std::endl;
        runTest(server, port, numThreads);
        numThreads *= scaleThreads;
    }

    return 0;
}