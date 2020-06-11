#include "stmdsp.hpp"

#include <chrono>
#include <filesystem>
#include <thread>

using namespace std::chrono_literals;

namespace stmdsp
{
    void scanner::scan()
    {
        std::string path ("/dev/ttyACM0");

        for (unsigned int i = 0; i < 10; i++) {
            path.back() = '0' + i;
            if (std::filesystem::exists(path)) {
                if (device dev (path); dev.open()) {
                    dev.write("i", 1);
                    std::this_thread::sleep_for(1s);
                    char buf[7];
                    if (dev.read(buf, 6) ==	6) {
                        buf[6] = '\0';
                        if (std::string(buf) == "stmdsp")
                            m_devices.emplace(std::move(dev));
                    }
                }
            }
        }
    }
}
