/**
 * @file dac.cpp
 * @brief Manages signal creation using the DAC.
 *
 * Copyright (C) 2020 Clyne Sullivan
 *
 * Distributed under the GNU GPL v3 or later. You should have received a copy of
 * the GNU General Public License along with this program.
 * If not, see <https://www.gnu.org/licenses/>.
 */

#include "dac.hpp"
#include "sclock.hpp"

DACDriver *DAC::m_driver[2] = {
    &DACD1, &DACD2
};

const DACConfig DAC::m_config = {
    .init = 0,
    .datamode = DAC_DHRM_12BIT_RIGHT,
    .cr = 0
};

const DACConversionGroup DAC::m_group_config = {
    .num_channels = 1,
    .end_cb = nullptr,
    .error_cb = nullptr,
#if defined(TARGET_PLATFORM_H7)
    .trigger = 5 // TIM6_TRGO
#elif defined(TARGET_PLATFORM_L4)
    .trigger = 0 // TIM6_TRGO
#endif
};

void DAC::begin()
{
    palSetPadMode(GPIOA, 4, PAL_STM32_MODE_ANALOG);
    palSetPadMode(GPIOA, 5, PAL_STM32_MODE_ANALOG);

    dacStart(m_driver[0], &m_config);
    dacStart(m_driver[1], &m_config);
}

void DAC::start(int channel, dacsample_t *buffer, size_t count)
{
    if (channel >= 0 && channel < 2) {
        dacStartConversion(m_driver[channel], &m_group_config, buffer, count);
        SClock::start();
    }
}

void DAC::stop(int channel)
{
    if (channel >= 0 && channel < 2) {
        dacStopConversion(m_driver[channel]);
        SClock::stop();
    }
}

