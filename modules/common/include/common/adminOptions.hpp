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

#ifndef IOTEYE_COMMON_ADMIN_OPTIONS_HPP
#define IOTEYE_COMMON_ADMIN_OPTIONS_HPP

#include <boost/program_options.hpp>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace po = boost::program_options;
namespace ioteye {
class AdminOptions {
public:
    static AdminOptions& getOptions(int argc, char** argv);
    static AdminOptions& getOptions();
    uint16_t getOutdatedDelay() const;
    uint16_t getOfflineDelay() const;
    uint16_t getDeadDelay() const;
    uint16_t getMaxPins() const;
    uint32_t getHistoryIntervalMs() const;
    uint32_t getHistoryMaxSamples() const;
    const std::string& getHistoryFile() const;

    AdminOptions(AdminOptions& other) = delete;
    void operator=(const AdminOptions&) = delete;

protected:
    AdminOptions(int argc, char** argv);
    ~AdminOptions(){};

private:
    void handleMaxPins(uint16_t value);
    void handleOutdatedDelay(uint16_t value);
    void handleOfflineDelay(uint16_t value);
    void handleDeadDelay(uint16_t value);
    void handleHistoryInterval(uint32_t value);
    void handleHistoryMax(uint32_t value);
    void handleHistoryFile(const std::string& value);

private:
    int m_argc = 0;
    char** m_argv = nullptr;

    uint16_t m_maxPinsOpt = 255;
    uint16_t m_outdatedDelayOpt = 500;
    uint16_t m_offlineDelayOpt = 1000;
    uint16_t m_deadDelayOpt = 10000;
    uint32_t m_historyIntervalMsOpt = 2000;
    uint32_t m_historyMaxSamplesOpt = 5000;
    std::string m_historyFileOpt = "pin_history.jsonl";

    enum OPTIONS {
        MAXPINS,
        OUTDATED,
        OFFLINE,
        DEADDELAY,
        HISTORYINTERVAL,
        HISTORYMAX,
        HISTORYFILE,
        HELP,
        MAXOPTIONS
    };
};

}  // namespace ioteye

#endif  // IOTEYE_COMMON_ADMIN_OPTIONS_HPP