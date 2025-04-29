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

#ifndef ADMIN_OPTIONS_H
#define ADMIN_OPTIONS_H

#include <getopt.h>

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>

class AdminOptions {
public:
    AdminOptions() {
    }
    void init(int argc, char **argv) {
        static struct option long_options[] = {{"pins", required_argument, 0, 'p'},
                                               {"outdated", required_argument, 0, 'd'},
                                               {"offline", required_argument, 0, 'f'},
                                               {0, 0, 0, 0}};

        const char *usage =
            "Usage: IoTeyeServer [options]\n"
            "\n"
            "Options:\n"
            "  -p value, --pins=value           Maximum value of server pins (max: 65556, default: 255)\n"
            "  -d delay, --outdated=delay       Time required for device information to be considered out of "
            "date (default: 500)\n"
            "  -f delay,  --offline=delay       Time required to consider the device disabled (default: "
            "1000)\n"
            "\n";

        int result;
        while ((result = getopt_long(argc, argv, "p:d:f:", long_options, nullptr)) != -1) {
            switch (result) {
                case 'p':
                    m_maxPinsOpt = atoi(optarg);
                    break;
                case 'd':
                    m_outdatedDelayOpt = atoi(optarg);
                    break;
                case 'f':
                    m_offlineDelayOpt = atoi(optarg);
                    break;
                default:
                    std::cout << usage;
                    exit(1);
            };
        }
    }
    inline uint16_t getOutdatedDelay() {
        return m_outdatedDelayOpt;
    }
    inline uint16_t getOfflineDelay() {
        return m_offlineDelayOpt;
    }
    inline uint16_t getMaxPins() {
        return m_maxPinsOpt;
    }

private:
    uint16_t m_maxPinsOpt = 255;
    uint16_t m_outdatedDelayOpt = 500;
    uint16_t m_offlineDelayOpt = 1000;
};

extern std::unique_ptr<AdminOptions> OPTIONS;

#endif  // !ADMIN_OPTIONS_H