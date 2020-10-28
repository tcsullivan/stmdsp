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
        R3125 = 0,
        R3750,
        R6250,
        R7500,
        R12K5,
        R15K,
        R25K,
        R30K,
        R40K,
        R50K,
        R60K,
        R80K,
        R96K,
        R100K,
        R120K,
        R160K,
        R200K,
        R240K,
        R400K,
        R480K,
        R800K,
        R960K,
        R1M6
    };
    
    void init();
    void set_rate(rate new_rate);
    unsigned int get_gpt_divisor();
    adcsample_t *read(adcsample_t *buffer, size_t count);
    void read_start(operation_t operation_func, adcsample_t *buffer, size_t count);
    void read_set_operation_func(operation_t operation_func);
    void read_stop();
}

#endif // STMDSP_ADC_HPP_

