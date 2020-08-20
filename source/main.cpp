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

static_assert(sizeof(adcsample_t) == sizeof(uint16_t));

#if CACHE_LINE_SIZE > 0
CC_ALIGN(CACHE_LINE_SIZE)
#endif
static std::array<adcsample_t, CACHE_SIZE_ALIGN(adcsample_t, 2048)> adc_samples;
static std::array<dacsample_t, CACHE_SIZE_ALIGN(dacsample_t, 2048)> dac_samples;

static void signal_operate(adcsample_t *buffer, size_t count);

int main()
{
    halInit();
    chSysInit();

    palSetPadMode(GPIOA, 5,  PAL_MODE_OUTPUT_PUSHPULL); // LED

    adc::init();
    dac::init();
    usbserial::init();

    static unsigned int dac_sample_count = 2048;
	while (true) {
        if (usbserial::is_active()) {
            // Expect to receive a byte command 'packet'.
            if (char cmd[3]; usbserial::read(&cmd, 1) > 0) {
                switch (cmd[0]) {
                case 'r': // Read in analog signal
                    if (usbserial::read(&cmd[1], 2) < 2)
                        break;
                    if (auto count = std::min(static_cast<unsigned int>(cmd[1] | (cmd[2] << 8)), adc_samples.size()); count > 0) {
                        adc::read(&adc_samples[0], count);
                        usbserial::write(adc_samples.data(), count * sizeof(adcsample_t));
                    }
                    break;
                case 'R':
                    adc::read_start(signal_operate, &adc_samples[0], adc_samples.size() * sizeof(adcsample_t));
                    break;
                case 'S':
                    adc::read_stop();
                    break;
                case 'W':
                    if (usbserial::read(&cmd[1], 2) < 2)
                        break;
                    if (auto count = std::min(static_cast<unsigned int>(cmd[1] | (cmd[2] << 8)), dac_samples.size()); count > 0)
                        dac_sample_count = count;
                    else
                        dac::write_stop();
                    break;
                case 'w':
                    if (usbserial::read(&dac_samples[0], 2 * dac_sample_count) != 2 * dac_sample_count)
                        break;
                    dac::write_start(&dac_samples[0], dac_sample_count);
                    break;
                case 'i': // Identify ourself as an stmdsp device
                    usbserial::write("stmdsp", 6);
                    break;
                default:
                    break;
                }
            }
        }

		chThdSleepMilliseconds(1);
	}
}

void signal_operate([[maybe_unused]] adcsample_t *buffer, [[maybe_unused]] size_t count)
{

}

