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
#include "sclock.hpp"

#include <array>

class ADC
{
public:
    using Operation = void (*)(adcsample_t *buffer, size_t count);

    static void begin();

    static void start(adcsample_t *buffer, size_t count, Operation operation);
    static void stop();

    static void setRate(SClock::Rate rate);
    static void setOperation(Operation operation);

private:
    static ADCDriver *m_driver;

    static const ADCConfig m_config;
    static ADCConversionGroup m_group_config;

    static std::array<std::array<uint32_t, 2>, 6> m_rate_presets;

    static adcsample_t *m_current_buffer;
    static size_t m_current_buffer_size;
    static Operation m_operation;

public:
    static void conversionCallback(ADCDriver *);
};

#endif // STMDSP_ADC_HPP_

