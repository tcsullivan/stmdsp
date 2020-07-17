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
        device(const std::string& file);

        ~device() {
            m_serial.close();
        }

        bool connected() {
            return m_serial.isOpen() && (m_serial.write("i"), m_serial.read(6) == "stmdsp");
        }

        std::vector<adcsample_t> sample(unsigned long int count = 1);

    private:
        serial::Serial m_serial;
    };
}

#endif // STMDSP_HPP_

