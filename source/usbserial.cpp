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

#include "usbcfg.h"

constexpr static const auto sdud = &SDU1;

namespace usbserial
{
    void init()
    {
        palSetPadMode(GPIOA, 11, PAL_MODE_ALTERNATE(10));
        palSetPadMode(GPIOA, 12, PAL_MODE_ALTERNATE(10));
    
        sduObjectInit(sdud);
        sduStart(sdud, &serusbcfg);
    
        // Reconnect bus so device can re-enumerate on reset
        usbDisconnectBus(serusbcfg.usbp);
        chThdSleepMilliseconds(1500);
        usbStart(serusbcfg.usbp, &usbcfg);
        usbConnectBus(serusbcfg.usbp);
    }
    
    bool is_active()
    {
        return sdud->config->usbp->state == USB_ACTIVE;
    }
    
    size_t read(void *buffer, size_t count)
    {
        auto bss = reinterpret_cast<BaseSequentialStream *>(sdud);
        return streamRead(bss, static_cast<uint8_t *>(buffer), count);
    }
    
    size_t write(const void *buffer, size_t count)
    {
        auto bss = reinterpret_cast<BaseSequentialStream *>(sdud);
        return streamWrite(bss, static_cast<const uint8_t *>(buffer), count);
    }
}

