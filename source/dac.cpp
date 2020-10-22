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

constexpr static const auto dacd = &DACD1;
constexpr static const auto dacd2 = &DACD2;
constexpr static const auto gptd = &GPTD7;

constexpr static const DACConfig dac_config = {
    .init = 0,
    .datamode = DAC_DHRM_12BIT_RIGHT,
    .cr = 0
};

constexpr static const DACConversionGroup dac_group_config = {
  .num_channels = 1,
  .end_cb = nullptr,
  .error_cb = nullptr,
  .trigger = DAC_TRG(2)
};

constexpr static const GPTConfig gpt_config = {
  .frequency = 2400000,
  .callback = nullptr,
  .cr2 = TIM_CR2_MMS_1, /* TRGO */
  .dier = 0
};

static unsigned int dacs_running = 0;

namespace dac
{
    void init()
    {
        palSetPadMode(GPIOA, 4, PAL_MODE_INPUT_ANALOG);
        palSetPadMode(GPIOA, 5, PAL_MODE_INPUT_ANALOG);
    
        dacStart(dacd, &dac_config);
        dacStart(dacd2, &dac_config);
        gptStart(gptd, &gpt_config);
    }

    void write_start(unsigned int channel, dacsample_t *buffer, size_t count)
    {
        if (channel < 2) {
            dacStartConversion(channel == 0 ? dacd : dacd2, &dac_group_config, buffer, count);

            if (dacs_running == 0)
                gptStartContinuous(gptd, 25);
            dacs_running |= 1 << channel;
        }
    }
    
    void write_stop(unsigned int channel)
    {
        if (channel < 2) {
            dacStopConversion(channel == 0 ? dacd : dacd2);

            dacs_running &= ~(1 << channel);
            if (dacs_running == 0)
                gptStopTimer(gptd);
        }
    }
}

