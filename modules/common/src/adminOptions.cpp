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

#include <common/adminOptions.hpp>

namespace ioteye {

void AdminOptions::handleMaxPins(uint16_t value) {
    m_maxPinsOpt = value;
}
void AdminOptions::handleOutdatedDelay(uint16_t value) {
    m_outdatedDelayOpt = value;
}
void AdminOptions::handleOfflineDelay(uint16_t value) {
    m_offlineDelayOpt = value;
}
void AdminOptions::handleDeadDelay(uint16_t value) {
    m_deadDelayOpt = value;
}

uint16_t AdminOptions::getOutdatedDelay() const {
    return m_outdatedDelayOpt;
}

uint16_t AdminOptions::getOfflineDelay() const {
    return m_offlineDelayOpt;
}
uint16_t AdminOptions::getDeadDelay() const {
    return m_deadDelayOpt;
}
uint16_t AdminOptions::getMaxPins() const {
    return m_maxPinsOpt;
}

AdminOptions::AdminOptions(int argc, char** argv) : m_argc(argc), m_argv(argv) {
    std::vector<std::string> options = {"maxPins", "outdated", "offline",
                                        "dead", "help"};
    po::options_description desc("Usage: IoTeyeServer [options]");
    desc.add_options()("help,h", "Produce help message")(
        "maxPins,p", po::value<uint16_t>(),
        "Maximum value of server pins (max: 65556, default: 255)")(
        "outdated,o", po::value<uint16_t>(),
        "Time required for device information to be considered "
        "out of date (default: 500)")(
        "offline,f", po::value<uint16_t>(),
        "Time required to consider the device disabled (default: 1000)")(
        "dead,d", po::value<uint16_t>(),
        "Time required to disable device monitoring (default: 10000)");
    po::variables_map vm;
    po::store(po::parse_command_line(m_argc, m_argv, desc), vm);
    po::notify(vm);
    std::unordered_map<std::string, std::function<void()>> handlers = {
        {options[MAXPINS], [&vm, opt = options[MAXPINS],
                            this]() { handleMaxPins(vm[opt].as<uint16_t>()); }},
        {options[OUTDATED],
         [&vm, opt = options[OUTDATED], this]() {
             handleOutdatedDelay(vm[opt].as<uint16_t>());
         }},
        {options[OFFLINE],
         [&vm, opt = options[OFFLINE], this]() {
             handleOfflineDelay(vm[opt].as<uint16_t>());
         }},
        {options[DEADDELAY],
         [&vm, opt = options[DEADDELAY], this]() {
             handleDeadDelay(vm[opt].as<uint16_t>());
         }},
        {options[HELP], [&desc]() { std::cout << desc << std::endl; }},
    };

    // Handle Help
    if (vm.count(options[HELP])) {
        handlers[options[HELP]]();
        exit(0);
    }
    for (const auto& opt : handlers) {
        if (vm.count(opt.first))
            opt.second();
    }
}

AdminOptions& AdminOptions::getOptions(int argc, char** argv) {
    static AdminOptions instance(argc, argv);
    return instance;
}
AdminOptions& AdminOptions::getOptions() {
    return getOptions(0, {});
}

}  // namespace ioteye