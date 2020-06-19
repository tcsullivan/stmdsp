#ifndef STMDSP_HPP_
#define STMDSP_HPP_

#include <cstdint>
#include <list>
#include <serial/serial.h>
#include <string>

namespace stmdsp
{
    class scanner
    {
    private:
        constexpr static const char *STMDSP_USB_ID = "USB VID:PID=0483:5740";

    public:
        std::list<std::string>& scan();
        auto& devices() {
            return m_available_devices;
        }

    private:
        std::list<std::string> m_available_devices;
    };

    using adcsample_t = uint16_t;

    class device
    {
    public:
        device(const std::string& file) :
            m_serial(file, 115200, serial::Timeout::simpleTimeout(1000)) {}

        ~device() {
            m_serial.close();
        }

        bool connected() {
            return m_serial.isOpen() && (m_serial.write("i"), m_serial.read(6) == "stmdsp");
        }

        std::vector<adcsample_t> sample(unsigned long int count = 1) {
            if (connected()) {
                m_serial.write(std::vector<uint8_t> {'r',
                                                     static_cast<uint8_t>(count),
                                                     static_cast<uint8_t>(count >> 8)});
                std::vector<adcsample_t> data (count);
                m_serial.read(reinterpret_cast<uint8_t *>(data.data()),
                              data.size() * sizeof(adcsample_t));
                return data;
            } else {
                return {};
            }
        }

    private:
        serial::Serial m_serial;
    };
}

#endif // STMDSP_HPP_

