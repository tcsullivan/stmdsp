/**
 * @file adc.cpp
 * @brief Manages signal reading through the ADC.
 *
 * Copyright (C) 2020 Clyne Sullivan
 *
 * Distributed under the GNU GPL v3 or later. You should have received a copy of
 * the GNU General Public License along with this program.
 * If not, see <https://www.gnu.org/licenses/>.
 */

#include "adc.hpp"

constexpr static const auto adcd = &ADCD1;
constexpr static const auto gptd = &GPTD4;

constexpr static const ADCConfig adc_config = {
    .difsel = 0
};

static void adc_read_callback(ADCDriver *);

static ADCConversionGroup adc_group_config = {
    .circular = false,
    .num_channels = 1,
    .end_cb = adc_read_callback,
    .error_cb = nullptr,
    .cfgr = ADC_CFGR_EXTEN_RISING | ADC_CFGR_EXTSEL_SRC(12),  /* TIM4_TRGO */
    .cfgr2 = 0,
    .tr1 = ADC_TR(0, 4095),
    .smpr = {
        ADC_SMPR1_SMP_AN5(ADC_SMPR_SMP_12P5), 0
    },
    .sqr = {
        ADC_SQR1_SQ1_N(ADC_CHANNEL_IN5),
        0, 0, 0
    }
};

constexpr static const GPTConfig gpt_config = {
    .frequency = 2400000,
    .callback = nullptr,
    .cr2 = TIM_CR2_MMS_1, /* TRGO */
    .dier = 0
};

static bool adc_is_read_finished = false;
static adcsample_t *adc_current_buffer = nullptr;
static size_t adc_current_buffer_size = 0;
static adc::operation_t adc_operation_func = nullptr;

namespace adc
{
    void init()
    {
        palSetPadMode(GPIOA, 0, PAL_MODE_INPUT_ANALOG);
    
        gptStart(gptd, &gpt_config);
        adcStart(adcd, &adc_config);
        adcSTM32EnableVREF(adcd);
    }
    
    adcsample_t *read(adcsample_t *buffer, size_t count)
    {
        adc_is_read_finished = false;
        adc_group_config.circular = false;
        adcStartConversion(adcd, &adc_group_config, buffer, count);
        gptStartContinuous(gptd, 25);
        while (!adc_is_read_finished);
        return buffer;
    }

    void read_start(operation_t operation_func, adcsample_t *buffer, size_t count)
    {
        adc_current_buffer = buffer;
        adc_current_buffer_size = count;
        adc_operation_func = operation_func;
        adc_group_config.circular = true;
        adcStartConversion(adcd, &adc_group_config, buffer, count);
        gptStartContinuous(gptd, 25);
    }

    void read_set_operation_func(operation_t operation_func)
    {
        adc_operation_func = operation_func;
    }
    
    void read_stop()
    {
        gptStopTimer(gptd);
        adcStopConversion(adcd);
        adc_group_config.circular = false;
        adc_current_buffer = nullptr;
        adc_current_buffer_size = 0;
        adc_operation_func = nullptr;
    }
 
    void set_rate(rate r)
    {
        uint32_t val = 0;
    
        switch (r) {
        case rate::R2P5:
            val = ADC_SMPR1_SMP_AN5(ADC_SMPR_SMP_2P5);
            break;
        case rate::R6P5:
            val = ADC_SMPR1_SMP_AN5(ADC_SMPR_SMP_6P5);
            break;
        case rate::R12P5:
            val = ADC_SMPR1_SMP_AN5(ADC_SMPR_SMP_12P5);
            break;
        case rate::R24P5:
            val = ADC_SMPR1_SMP_AN5(ADC_SMPR_SMP_24P5);
            break;
        case rate::R47P5:
            val = ADC_SMPR1_SMP_AN5(ADC_SMPR_SMP_47P5);
            break;
        case rate::R92P5:
            val = ADC_SMPR1_SMP_AN5(ADC_SMPR_SMP_92P5);
            break;
        case rate::R247P5:
            val = ADC_SMPR1_SMP_AN5(ADC_SMPR_SMP_247P5);
            break;
        case rate::R640P5:
            val = ADC_SMPR1_SMP_AN5(ADC_SMPR_SMP_640P5);
            break;
        }
    
        adc_group_config.smpr[0] = val;
    }
}

void adc_read_callback(ADCDriver *driver)
{
    if (adc_group_config.circular) {
        if (adc_operation_func != nullptr) {
            auto half_size = adc_current_buffer_size / 2;
            if (adcIsBufferComplete(driver))
                adc_operation_func(adc_current_buffer + half_size, half_size);
            else
                adc_operation_func(adc_current_buffer, half_size);
        }
    } else {
        gptStopTimer(gptd);
        adc_is_read_finished = true;
    }
}

