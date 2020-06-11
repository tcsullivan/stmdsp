/**
 * @file main.cpp
 * @brief Program entry point.
 *
 * Copyright (C) 2020 Clyne Sullivan
 *
 * Distributed under the GNU GPL v3 or later. You should have received a copy of
 * the GNU General Public License along with this program.
 * If not, see <https://www.gnu.org/licenses/>.
 */

#include "ch.h"
#include "hal.h"

#include "adc.hpp"
#include "dac.hpp"
#include "usbserial.hpp"

#include <array>

#if CACHE_LINE_SIZE > 0
CC_ALIGN(CACHE_LINE_SIZE)
#endif
static std::array<adcsample_t, CACHE_SIZE_ALIGN(adcsample_t, 100)> adc_samples;

int main()
{
    halInit();
    chSysInit();

    palSetPadMode(GPIOA, 5,  PAL_MODE_OUTPUT_PUSHPULL); // LED

    ADCd adc (ADCD1, GPTD4);
    adc.start();

    //DACd dac (DACD1, {
    //    .init         = 0,
    //    .datamode     = DAC_DHRM_12BIT_RIGHT,
    //    .cr           = 0
    //});
    //dac.start();
    //dac.write(0, 1024);

    USBSeriald usbd (SDU1);
    usbd.start();

	while (true) {
        if (usbd.active()) {
            // Expect to receive a byte command 'packet'.
            if (char cmd; usbd.read(&cmd) > 0) {
                switch (cmd) {
                case 'r': // Read in analog signal
                    adc.getSamples(&adc_samples[0], adc_samples.size());
                    usbd.write(adc_samples.data(), adc_samples.size());
                    break;
                case 'i': // Identify ourself as an stmdsp device
                    usbd.write("stmdsp", 6);
                    break;
                default:
                    break;
                }
            }
        }

		chThdSleepMilliseconds(250);
	}
}

