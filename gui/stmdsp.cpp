#include "stmdsp.hpp"

#include <serial/serial.h>

namespace stmdsp
{
    std::list<std::string>& scanner::scan()
    {
        auto serialDevices = serial::list_ports();
        for (auto& device : serialDevices) {
            if (device.hardware_id.find(STMDSP_USB_ID) != std::string::npos)
                m_devices.emplace_front(device.port);
        }

        return m_devices;
    }
}
