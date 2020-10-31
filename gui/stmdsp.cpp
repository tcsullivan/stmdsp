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

    /*std::vector<adcsample_t> device::sample(unsigned long int count) {
        if (connected()) {
            uint8_t request[3] = {
                'd',
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
    }*/

    void device::continuous_set_buffer_size(unsigned int size) {
        if (connected()) {
            uint8_t request[3] = {
                'B',
                static_cast<uint8_t>(size),
                static_cast<uint8_t>(size >> 8)
            };
            m_serial.write(request, 3);
        }
    }

    void device::set_sample_rate(unsigned int id) {
        if (connected()) {
            uint8_t request[2] = {
                'r',
                static_cast<uint8_t>(id)
            };
            m_serial.write(request, 2);
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

        return count / 2;
    }

    std::vector<adcsample_t> device::continuous_read() {
        if (connected()) {
            m_serial.write("s");
            unsigned char sizebytes[2];
            m_serial.read(sizebytes, 2);
            unsigned int size = sizebytes[0] | (sizebytes[1] << 8);
            if (size > 0) {
                std::vector<adcsample_t> data (size);
                unsigned int total = size * sizeof(adcsample_t);
                unsigned int offset = 0;

                while (total > 512) {
                    m_serial.read(reinterpret_cast<uint8_t *>(&data[0]) + offset, 512);
                    m_serial.write("n");
                    offset += 512;
                    total -= 512;
                }
                m_serial.read(reinterpret_cast<uint8_t *>(&data[0]) + offset, total);
                m_serial.write("n");
                return data;

            }
        }

        return {};
    }

    void device::continuous_stop() {
        if (connected())
            m_serial.write("S");
    }

    void device::siggen_upload(dacsample_t *buffer, unsigned int size) {
        if (connected()) {
            uint8_t request[3] = {
                'D',
                static_cast<uint8_t>(size),
                static_cast<uint8_t>(size >> 8)
            };
            m_serial.write(request, 3);

            m_serial.write((uint8_t *)buffer, size * sizeof(dacsample_t));
        }
    }

    void device::siggen_start() {
        if (connected())
            m_serial.write("W");
    }

    void device::siggen_stop() {
        if (connected())
            m_serial.write("w");
    }

    void device::upload_filter(unsigned char *buffer, size_t size) {
        if (connected()) {
            uint8_t request[3] = {
                'E',
                static_cast<uint8_t>(size),
                static_cast<uint8_t>(size >> 8)
            };
            m_serial.write(request, 3);

            m_serial.write(buffer, size);
        }
    }

    void device::unload_filter() {
        if (connected())
            m_serial.write("e");
    }
}
