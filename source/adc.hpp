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

    enum class rate {
        R2P5,
        R6P5,
        R12P5,
        R24P5,
        R47P5,
        R92P5,
        R247P5,
        R640P5
    };
    
    void init();
    adcsample_t *read(adcsample_t *buffer, size_t count);
    void read_start(operation_t operation_func, adcsample_t *buffer, size_t count);
    void read_stop();
    void set_rate(rate r);
}

#endif // STMDSP_ADC_HPP_

