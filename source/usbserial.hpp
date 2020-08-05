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

#include "hal.h"

void usbserial_init();
bool usbserial_is_active();
size_t usbserial_read(void *buffer, size_t count);
size_t usbserial_write(const void *buffer, size_t count);

#endif // STMDSP_USBSERIAL_HPP_

