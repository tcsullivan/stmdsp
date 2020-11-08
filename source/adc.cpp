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
constexpr static const auto gptd = &GPTD6;

constexpr static const ADCConfig adc_config = {
    .difsel = 0
};

static void adc_read_callback(ADCDriver *);

static ADCConversionGroup adc_group_config = {
    .circular = false,
    .num_channels = 1,
    .end_cb = adc_read_callback,
    .error_cb = nullptr,
    .cfgr = ADC_CFGR_EXTEN_RISING | ADC_CFGR_EXTSEL_SRC(13),  /* TIM4_TRGO */
    .cfgr2 = 0,//ADC_CFGR2_ROVSE | ADC_CFGR2_OVSR_0 | ADC_CFGR2_OVSS_1, // Oversampling 2x
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
    .frequency = 36000000,
    .callback = nullptr,
    .cr2 = TIM_CR2_MMS_1, /* TRGO */
    .dier = 0
};

#define ADC_CCR_PRESC_DIV1 (0)
#define ADC_SAMPLE_RATE_SETTINGS_COUNT (7)
static uint32_t adc_sample_rate_settings[] = {
    // Rate    PLLSAI2N  ADC_PRESC            ADC_SMPR           GPT_DIV
    /* 16k  */ 8,        ADC_CCR_PRESC_DIV10, ADC_SMPR_SMP_12P5, 2250,
    /* 48k  */ 24,       ADC_CCR_PRESC_DIV10, ADC_SMPR_SMP_12P5, 750,
    /* 96k  */ 48,       ADC_CCR_PRESC_DIV10, ADC_SMPR_SMP_12P5, 375,
    /* 100k */ 40,       ADC_CCR_PRESC_DIV8,  ADC_SMPR_SMP_12P5, 360,
    /* 400k */ 40,       ADC_CCR_PRESC_DIV2,  ADC_SMPR_SMP_12P5, 90,
    /* 1M   */ 38,       ADC_CCR_PRESC_DIV1,  ADC_SMPR_SMP_6P5,  36,
    /* 2M   */ 76,       ADC_CCR_PRESC_DIV1,  ADC_SMPR_SMP_6P5,  18
};

static bool adc_is_read_finished = false;
static adcsample_t *adc_current_buffer = nullptr;
static size_t adc_current_buffer_size = 0;
static adc::operation_t adc_operation_func = nullptr;
static unsigned int adc_gpt_divisor = 1;

namespace adc
{
    void init()
    {
        palSetPadMode(GPIOA, 0, PAL_MODE_INPUT_ANALOG);
    
        gptStart(gptd, &gpt_config);
        adcStart(adcd, &adc_config);
        adcSTM32EnableVREF(adcd);

        set_rate(rate::R96K);
    }

    void set_rate(rate new_rate)
    {
        auto index = static_cast<unsigned int>(new_rate);
        auto plln = adc_sample_rate_settings[index * 4] << RCC_PLLSAI2CFGR_PLLSAI2N_Pos;
        auto presc = adc_sample_rate_settings[index * 4 + 1] << ADC_CCR_PRESC_Pos;
        auto smp = adc_sample_rate_settings[index * 4 + 2];
        adc_gpt_divisor = adc_sample_rate_settings[index * 4 + 3];
    
        adcStop(adcd);

        // Adjust PLLSAI2
        RCC->CR &= ~(RCC_CR_PLLSAI2ON);
        while ((RCC->CR & RCC_CR_PLLSAI2RDY) == RCC_CR_PLLSAI2RDY);
        RCC->PLLSAI2CFGR = (RCC->PLLSAI2CFGR & ~(RCC_PLLSAI2CFGR_PLLSAI2N_Msk)) | plln;
        RCC->CR |= RCC_CR_PLLSAI2ON;

        // Set ADC prescaler
        adcd->adcc->CCR = (adcd->adcc->CCR & ~(ADC_CCR_PRESC_Msk)) | presc;
        // Set sampling time
        adc_group_config.smpr[0] = ADC_SMPR1_SMP_AN5(smp);
        adcStart(adcd, &adc_config);
    }
    
    unsigned int get_rate()
    {
        for (unsigned int i = 0; i < ADC_SAMPLE_RATE_SETTINGS_COUNT; i++) {
            if (adc_gpt_divisor == adc_sample_rate_settings[i * 3 + 3])
                return i;
        }

        return 0xFF;
    }

    unsigned int get_gpt_divisor()
    {
        return adc_gpt_divisor;
    }
    
    adcsample_t *read(adcsample_t *buffer, size_t count)
    {
        adc_is_read_finished = false;
        adc_group_config.circular = false;
        adcStartConversion(adcd, &adc_group_config, buffer, count);
        gptStartContinuous(gptd, adc_gpt_divisor);
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
        gptStartContinuous(gptd, adc_gpt_divisor);
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

