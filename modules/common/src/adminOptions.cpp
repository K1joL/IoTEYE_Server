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
    m_maxPinsOpt = value > 0 ? value : 255;
}
void AdminOptions::handleOutdatedDelay(uint16_t value) {
    m_outdatedDelayOpt = value > 0 ? value : 500;
}
void AdminOptions::handleOfflineDelay(uint16_t value) {
    m_offlineDelayOpt = value > 0 ? value : 1000;
}
void AdminOptions::handleDeadDelay(uint16_t value) {
    m_deadDelayOpt = value > 0 ? value : 10000;
}

void AdminOptions::handleHistoryInterval(uint32_t value) {
    m_historyIntervalMsOpt = value > 0 ? value : 2000;
}
void AdminOptions::handleHistoryMax(uint32_t value) {
    m_historyMaxSamplesOpt = value > 0 ? value : 5000;
}
void AdminOptions::handleHistoryFile(const std::string& value) {
    if (!value.empty())
        m_historyFileOpt = value;
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

uint32_t AdminOptions::getHistoryIntervalMs() const {
    return m_historyIntervalMsOpt;
}
uint32_t AdminOptions::getHistoryMaxSamples() const {
    return m_historyMaxSamplesOpt;
}
const std::string& AdminOptions::getHistoryFile() const {
    return m_historyFileOpt;
}

AdminOptions::AdminOptions(int argc, char** argv) : m_argc(argc), m_argv(argv) {
    std::vector<std::string> options = {"maxPins",         "outdated",   "offline",     "dead",
                                        "historyInterval", "historyMax", "historyFile", "help"};

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
        "Time required to disable device monitoring (default: 10000)")(
        "historyInterval", po::value<uint32_t>(),
        "Pin history snapshot interval in ms (default: 2000)")(
        "historyMax", po::value<uint32_t>(), "Max lines in pin history file (default: 5000)")(
        "historyFile", po::value<std::string>(),
        "Pin history JSONL file path (default: pin_history.jsonl)");
    po::variables_map vm;
    po::store(po::parse_command_line(m_argc, m_argv, desc), vm);
    po::notify(vm);
    std::unordered_map<std::string, std::function<void()>> handlers = {
        {options[MAXPINS],
         [&vm, opt = options[MAXPINS], this]() { handleMaxPins(vm[opt].as<uint16_t>()); }},
        {options[OUTDATED],
         [&vm, opt = options[OUTDATED], this]() { handleOutdatedDelay(vm[opt].as<uint16_t>()); }},
        {options[OFFLINE],
         [&vm, opt = options[OFFLINE], this]() { handleOfflineDelay(vm[opt].as<uint16_t>()); }},
        {options[DEADDELAY],
         [&vm, opt = options[DEADDELAY], this]() { handleDeadDelay(vm[opt].as<uint16_t>()); }},
        {options[HISTORYINTERVAL], [&vm, opt = options[HISTORYINTERVAL],
                                    this]() { handleHistoryInterval(vm[opt].as<uint32_t>()); }},
        {options[HISTORYMAX],
         [&vm, opt = options[HISTORYMAX], this]() { handleHistoryMax(vm[opt].as<uint32_t>()); }},
        {options[HISTORYFILE], [&vm, opt = options[HISTORYFILE],
                                this]() { handleHistoryFile(vm[opt].as<std::string>()); }},
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