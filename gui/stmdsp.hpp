/**
 * @file stmdsp.hpp
 * @brief Interface for communication with stmdsp device over serial.
 *
 * Copyright (C) 2021 Clyne Sullivan
 *
 * Distributed under the GNU GPL v3 or later. You should have received a copy of
 * the GNU General Public License along with this program.
 * If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef STMDSP_HPP_
#define STMDSP_HPP_

#include <cstdint>
#include <list>
#include <serial/serial.h>
#include <string>

namespace stmdsp
{
    constexpr unsigned int SAMPLES_MAX = 3000;

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
    using dacsample_t = uint16_t;

    class device
    {
    public:
        device(const std::string& file);

        ~device() {
            m_serial.close();
        }

        bool connected() {
            return m_serial.isOpen();
        }

        //std::vector<adcsample_t> sample(unsigned long int count = 1);

        void continuous_set_buffer_size(unsigned int size);
        unsigned int get_buffer_size() const { return m_buffer_size; }
        void set_sample_rate(unsigned int id);
        unsigned int get_sample_rate();
        void continuous_start();
        void continuous_start_measure();
        uint32_t continuous_start_get_measurement();
        std::vector<adcsample_t> continuous_read();
        void continuous_stop();

        void siggen_upload(dacsample_t *buffer, unsigned int size);
        void siggen_start();
        void siggen_stop();

        // buffer is ELF binary
        void upload_filter(unsigned char *buffer, size_t size);
        void unload_filter();

    private:
        serial::Serial m_serial;
        unsigned int m_buffer_size = SAMPLES_MAX;
    };
}

#endif // STMDSP_HPP_

