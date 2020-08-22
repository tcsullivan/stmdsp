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
        m_serial(file, 230400, serial::Timeout::simpleTimeout(50))
    {
        if (m_serial.isOpen()) {
           m_serial.write("i");
           if (m_serial.read(6) != "stmdsp")
               m_serial.close();
        }
    }

    std::vector<adcsample_t> device::sample(unsigned long int count) {
        if (connected()) {
            uint8_t request[3] = {
                'r',
                static_cast<uint8_t>(count),
                static_cast<uint8_t>(count >> 8)
            };
            m_serial.write(request, 3);
            std::vector<adcsample_t> data (count);
            m_serial.read(reinterpret_cast<uint8_t *>(data.data()), data.size() * sizeof(adcsample_t));
            return data;
        } else {
            return {};
        }
    }

    void device::continuous_start() {
        if (connected())
            m_serial.write("R");
    }

    std::vector<adcsample_t> device::continuous_read() {
        if (connected()) {
            m_serial.write("s");
            std::vector<adcsample_t> data (2048);
            m_serial.read(reinterpret_cast<uint8_t *>(data.data()), 2048 * sizeof(adcsample_t));
            return data;
        }
    }

    void device::continuous_stop() {
        if (connected())
            m_serial.write("S");
    }
}
