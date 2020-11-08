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

namespace adc
{
    using operation_t = void (*)(adcsample_t *buffer, size_t count);

    enum class rate : unsigned int {
        R16K = 0,
        R48K,
        R96K,
        R100K,
        R400K,
        R1M,
        R2M
    };
    
    void init();
    void set_rate(rate new_rate);
    unsigned int get_rate();
    unsigned int get_gpt_divisor();
    adcsample_t *read(adcsample_t *buffer, size_t count);
    void read_start(operation_t operation_func, adcsample_t *buffer, size_t count);
    void read_set_operation_func(operation_t operation_func);
    void read_stop();
}

#endif // STMDSP_ADC_HPP_

