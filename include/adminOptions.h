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

#include <boost/program_options.hpp>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <unordered_map>
#include <vector>

namespace po = boost::program_options;
namespace ioteye {
class AdminOptions {
public:
    AdminOptions() {
    }
    void init(int argc, char **argv);
    uint16_t getOutdatedDelay() const;
    uint16_t getOfflineDelay() const;
    uint16_t getDeadDelay() const;
    uint16_t getMaxPins() const;

private:
    void handleMaxPins(uint16_t value);
    void handleOutdatedDelay(uint16_t value);
    void handleOfflineDelay(uint16_t value);
    void handleDeadDelay(uint16_t value);

private:
    uint16_t m_maxPinsOpt = 255;
    uint16_t m_outdatedDelayOpt = 500;
    uint16_t m_offlineDelayOpt = 1000;
    uint16_t m_deadDelayOpt = 10000;
    enum OPTIONS { MAXPINS, OUTDATED, OFFLINE, DEADDELAY, HELP, MAXOPTIONS };
};
}  // namespace ioteye
extern std::unique_ptr<ioteye::AdminOptions> OPTIONS;

#endif  // !ADMIN_OPTIONS_H