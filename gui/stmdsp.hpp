#ifndef STMDSP_HPP_
#define STMDSP_HPP_

#include <fstream>
#include <list>
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
            return m_devices;
        }

    private:
        std::list<std::string> m_devices;
    };
}

#endif // STMDSP_HPP_

