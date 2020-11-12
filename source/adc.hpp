/**
 * @file adc.hpp
 * @brief Manages signal reading through the ADC.
 *
 * Copyright (C) 2020 Clyne Sullivan
 *
 * Distributed under the GNU GPL v3 or later. You should have received a copy of
 * the GNU General Public License along with this program.
 * If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef STMDSP_ADC_HPP_
#define STMDSP_ADC_HPP_

#include "hal.h"

#include <array>

class ADC
{
public:
    using Operation = void (*)(adcsample_t *buffer, size_t count);

    enum class Rate : int {
        //R8K = 0,
        R16K = 0,
        R20K,
        R32K,
        R48K,
        R60K,
        R96K
    };
    
    static void begin();

    static void start(adcsample_t *buffer, size_t count, Operation operation);
    static void stop();

    static void setRate(Rate rate);
    static void setOperation(Operation operation);

    static int getRate();
    static unsigned int getTimerDivisor();

private:
    static ADCDriver *m_driver;
    static GPTDriver *m_timer;

    static const ADCConfig m_config;
    static /*const*/ ADCConversionGroup m_group_config;
    static const GPTConfig m_timer_config;

    static std::array<std::array<uint32_t, 4>, 6> m_rate_presets;

    static adcsample_t *m_current_buffer;
    static size_t m_current_buffer_size;
    static Operation m_operation;

    static unsigned int m_timer_divisor;

    static void conversionCallback(ADCDriver *);
};

#endif // STMDSP_ADC_HPP_

