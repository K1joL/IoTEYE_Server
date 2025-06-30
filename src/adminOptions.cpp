#include "adminOptions.h"

std::unique_ptr<ioteye::AdminOptions> OPTIONS = std::make_unique<ioteye::AdminOptions>();
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
void AdminOptions::init(int argc, char** argv) {
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
    po::store(po::parse_command_line(argc, argv, desc), vm);
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

}  // namespace ioteye