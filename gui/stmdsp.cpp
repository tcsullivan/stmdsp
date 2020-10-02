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
        m_serial(file, 1000000/*230400*/, serial::Timeout::simpleTimeout(50))
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

    void device::continuous_start_measure() {
        if (connected())
            m_serial.write("M");
    }

    uint32_t device::continuous_start_get_measurement() {
        uint32_t count = 0;
        if (connected()) {
            m_serial.write("m");
            m_serial.read(reinterpret_cast<uint8_t *>(&count), sizeof(uint32_t));
        }

        return count;
    }

    std::vector<adcsample_t> device::continuous_read() {
        if (connected()) {
            m_serial.write("s");
            std::vector<adcsample_t> data (2048);
            m_serial.read(reinterpret_cast<uint8_t *>(data.data()), 2048 * sizeof(adcsample_t));
            return data;
        }

        return {};
    }

    void device::continuous_stop() {
        if (connected())
            m_serial.write("S");
    }

    void device::upload_filter(unsigned char *buffer, size_t size) {
        if (connected()) {
            uint8_t request[3] = {
                'e',
                static_cast<uint8_t>(size),
                static_cast<uint8_t>(size >> 8)
            };
            m_serial.write(request, 3);

            m_serial.write(buffer, size);
        }
    }
}
