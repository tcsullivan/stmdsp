#ifndef STMDSP_HPP_
#define STMDSP_HPP_

#include <fstream>
#include <set>
#include <string>

namespace stmdsp
{
    class device
    {
    public:
        device(const std::string& path) :
            m_path(path) {}

        bool open() {
            m_stream.open(m_path, std::ios_base::in | std::ios_base::out | std::ios_base::binary);
            return m_stream.is_open();
        }

        std::size_t read(char *buffer, std::size_t count) {
            return m_stream.readsome(buffer, count);
        }

        std::size_t write(const char *buffer, std::size_t count) {
            m_stream.write(buffer, count);
            return m_stream.good() ? count : 0;
        }

        const std::string& path() const {
            return m_path;
        }

        auto operator<=>(const device& other) const {
            return m_path <=> other.m_path;
        }

    private:
        std::string m_path;
        std::fstream m_stream;
    };

    class scanner
    {
    private:
        constexpr static unsigned int STMDSP_VENDOR_ID = 0x0483;
        constexpr static unsigned int STMDSP_DEVICE_ID = 0x5740;

    public:
        void scan();
        const auto& devices() const {
            return m_devices;
        }

    private:
        std::set<device> m_devices;
    };
}

#endif // STMDSP_HPP_

