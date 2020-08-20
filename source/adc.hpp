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

enum class ADCRate {
    R2P5,
    R6P5,
    R12P5,
    R24P5,
    R47P5,
    R92P5,
    R247P5,
    R640P5
};

using adc_operation_t = void (*)(adcsample_t *buffer, size_t count);

void adc_init();
adcsample_t *adc_read(adcsample_t *buffer, size_t count);
void adc_read_start(adc_operation_t operation_func, adcsample_t *buffer, size_t count);
void adc_read_stop();
void adc_set_rate(ADCRate rate);

#endif // STMDSP_ADC_HPP_

