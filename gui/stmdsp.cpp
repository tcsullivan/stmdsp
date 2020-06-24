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

    device::device(const std::string& file) :
        m_serial(file, 230400, serial::Timeout::simpleTimeout(50)) {}

    std::vector<adcsample_t> device::sample(unsigned long int count) {
        if (connected()) {
            uint8_t request[3] = {
                'r',
                static_cast<uint8_t>(count),
                static_cast<uint8_t>(count >> 8)
            };
            m_serial.write(request, 3);
            std::vector<adcsample_t> data (count);
            m_serial.read(reinterpret_cast<uint8_t *>(data.data()),
                          data.size() * sizeof(adcsample_t));
            return data;
        } else {
            return {};
        }
    }
}
