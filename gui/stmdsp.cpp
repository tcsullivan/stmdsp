#include "stmdsp.hpp"

#include <serial/serial.h>

namespace stmdsp
{
    std::list<std::string>& scanner::scan()
    {
        auto devices = serial::list_ports();
        for (auto& device : devices) {
            if (device.hardware_id.find(STMDSP_USB_ID) != std::string::npos)
                m_available_devices.emplace_front(device.port);
        }

        return m_available_devices;
    }
}
