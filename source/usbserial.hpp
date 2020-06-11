/**
 * @file usbserial.hpp
 * @brief Wrapper for ChibiOS's SerialUSBDriver.
 *
 * Copyright (C) 2020 Clyne Sullivan
 *
 * Distributed under the GNU GPL v3 or later. You should have received a copy of
 * the GNU General Public License along with this program.
 * If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef STMDSP_USBSERIAL_HPP_
#define STMDSP_USBSERIAL_HPP_

#include "usbcfg.h"

#include <cstddef>

class USBSeriald
{
public:
    constexpr explicit USBSeriald(SerialUSBDriver& driver) :
        m_driver(&driver) {}

    void start();

    bool active() const;

    std::size_t read(void *buffer, std::size_t count = 1);
    std::size_t write(const void *buffer, std::size_t count = 1);

private:
    SerialUSBDriver *m_driver;

    void initPins();
};

#endif // STMDSP_USBSERIAL_HPP_

