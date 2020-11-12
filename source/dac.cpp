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

#include "adc.hpp" // ADC::getTimerDivisor
#include "dac.hpp"

DACDriver *DAC::m_driver[2] = {
    &DACD1, &DACD2
};
GPTDriver *DAC::m_timer = &GPTD7;
int DAC::m_timer_user_count = 0;

const DACConfig DAC::m_config = {
    .init = 0,
    .datamode = DAC_DHRM_12BIT_RIGHT,
    .cr = 0
};

const DACConversionGroup DAC::m_group_config = {
    .num_channels = 1,
    .end_cb = nullptr,
    .error_cb = nullptr,
    .trigger = DAC_TRG(2)
};

const GPTConfig DAC::m_timer_config = {
    .frequency = 36000000,
    .callback = nullptr,
    .cr2 = TIM_CR2_MMS_1, /* TRGO */
    .dier = 0
};

void DAC::begin()
{
    palSetPadMode(GPIOA, 4, PAL_MODE_INPUT_ANALOG);
    palSetPadMode(GPIOA, 5, PAL_MODE_INPUT_ANALOG);

    dacStart(m_driver[0], &m_config);
    dacStart(m_driver[1], &m_config);
    gptStart(m_timer, &m_timer_config);
}

void DAC::start(int channel, dacsample_t *buffer, size_t count)
{
    if (channel >= 0 && channel < 2) {
        dacStartConversion(m_driver[channel], &m_group_config, buffer, count);

        if (m_timer_user_count == 0)
            gptStartContinuous(m_timer, ADC::getTimerDivisor());
        m_timer_user_count++;
    }
}

void DAC::stop(int channel)
{
    if (channel >= 0 && channel < 2) {
        dacStopConversion(m_driver[channel]);

        if (--m_timer_user_count == 0)
            gptStopTimer(m_timer);
    }
}

