#ifndef ADMIN_OPTIONS_H
#define ADMIN_OPTIONS_H


#include <cstdint>
#include <getopt.h>
#include <cstdlib>
#include <iostream>

class AdminOptions
{
public:
    AdminOptions()
    {
    }
    void init(int argc, char **argv)
    {
        static struct option long_options[] = {
            {"pins", required_argument, 0, 'p'},
            {"outdated", required_argument, 0, 'd'},
            {"offline", required_argument, 0, 'f'},
            {0, 0, 0, 0}};

        const char *usage =
            "Usage: IoTeyeServer [options]\n"
            "\n"
            "Options:\n"
            "  -p value, --pins=value           Maximum value of server pins (default: 255)\n"
            "  -d delay, --outdated=delay       Time required for device information to be considered out of date (default: 500)\n"
            "  -f delay,  --offline=delay       Time required to consider the device disabled (default: 1000)\n"
            "\n";

        int result;
        while ((result = getopt_long(argc, argv, "t:s:p:", long_options, nullptr)) != -1)
        {
            switch (result)
            {
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
    inline uint16_t getOutdatedDelay() { return m_outdatedDelayOpt; }
    inline uint16_t getOfflineDelay() { return m_offlineDelayOpt; }
    inline uint16_t getMaxPins() { return m_maxPinsOpt; }

private:
    uint16_t m_maxPinsOpt = 255;
    uint16_t m_outdatedDelayOpt = 500;
    uint16_t m_offlineDelayOpt = 1000;
};

extern AdminOptions *OPTIONS;

#endif // !ADMIN_OPTIONS_H