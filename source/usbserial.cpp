/**
 * @file usbserial.cpp
 * @brief Wrapper for ChibiOS's SerialUSBDriver.
 *
 * Copyright (C) 2020 Clyne Sullivan
 *
 * Distributed under the GNU GPL v3 or later. You should have received a copy of
 * the GNU General Public License along with this program.
 * If not, see <https://www.gnu.org/licenses/>.
 */

#include "usbserial.hpp"

#include "hal.h"

void USBSeriald::start()
{
    initPins();

    sduObjectInit(m_driver);
    sduStart(m_driver, &serusbcfg);

    // Reconnect bus so device can re-enumerate on reset
    usbDisconnectBus(serusbcfg.usbp);
    chThdSleepMilliseconds(1500);
    usbStart(serusbcfg.usbp, &usbcfg);
    usbConnectBus(serusbcfg.usbp);
}

bool USBSeriald::active() const
{
    return m_driver->config->usbp->state == USB_ACTIVE;
}

std::size_t USBSeriald::read(void *buffer, std::size_t count)
{
    auto *bss = reinterpret_cast<BaseSequentialStream *>(m_driver);
    return streamRead(bss, static_cast<uint8_t *>(buffer), count);
}

std::size_t USBSeriald::write(const void *buffer, std::size_t count)
{
    auto *bss = reinterpret_cast<BaseSequentialStream *>(m_driver);
    return streamWrite(bss, static_cast<const uint8_t *>(buffer), count);
}

void USBSeriald::initPins()
{
    palSetPadMode(GPIOA, 11, PAL_MODE_ALTERNATE(10));
    palSetPadMode(GPIOA, 12, PAL_MODE_ALTERNATE(10));
}

