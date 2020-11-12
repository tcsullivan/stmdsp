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

ADCDriver *ADC::m_driver = &ADCD1;
GPTDriver *ADC::m_timer = &GPTD6;

const ADCConfig ADC::m_config = {
    .difsel = 0
};

ADCConversionGroup ADC::m_group_config = {
    .circular = true,
    .num_channels = 1,
    .end_cb = ADC::conversionCallback,
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

const GPTConfig ADC::m_timer_config = {
    .frequency = 36000000,
    .callback = nullptr,
    .cr2 = TIM_CR2_MMS_1, /* TRGO */
    .dier = 0
};

std::array<std::array<uint32_t, 4>, 6> ADC::m_rate_presets = {{
     // Rate    PLLSAI2N  ADC_PRESC            ADC_SMPR           GPT_DIV
    {/* 16k  */ 8,        ADC_CCR_PRESC_DIV10, ADC_SMPR_SMP_12P5, 2250},
    {/* 20k  */ 10,       ADC_CCR_PRESC_DIV10, ADC_SMPR_SMP_12P5, 1800},
    {/* 32k  */ 16,       ADC_CCR_PRESC_DIV10, ADC_SMPR_SMP_12P5, 1125},
    {/* 48k  */ 24,       ADC_CCR_PRESC_DIV10, ADC_SMPR_SMP_12P5, 750},
    {/* 60k  */ 30,       ADC_CCR_PRESC_DIV10, ADC_SMPR_SMP_12P5, 600},
    {/* 96k  */ 48,       ADC_CCR_PRESC_DIV10, ADC_SMPR_SMP_12P5, 375}
}};

adcsample_t *ADC::m_current_buffer = nullptr;
size_t ADC::m_current_buffer_size = 0;
ADC::Operation ADC::m_operation = nullptr;

unsigned int ADC::m_timer_divisor = 2;

void ADC::begin()
{
    palSetPadMode(GPIOA, 0, PAL_MODE_INPUT_ANALOG);

    adcStart(m_driver, &m_config);
    adcSTM32EnableVREF(m_driver);
    gptStart(m_timer, &m_timer_config);

    setRate(Rate::R96K);
}

void ADC::start(adcsample_t *buffer, size_t count, Operation operation)
{
    m_current_buffer = buffer;
    m_current_buffer_size = count;
    m_operation = operation;

    adcStartConversion(m_driver, &m_group_config, buffer, count);
    gptStartContinuous(m_timer, m_timer_divisor);
}

void ADC::stop()
{
    gptStopTimer(m_timer);
    adcStopConversion(m_driver);

    m_current_buffer = nullptr;
    m_current_buffer_size = 0;
    m_operation = nullptr;
}

void ADC::setRate(ADC::Rate rate)
{
    auto& preset = m_rate_presets[static_cast<int>(rate)];
    auto plln = preset[0] << RCC_PLLSAI2CFGR_PLLSAI2N_Pos;
    auto presc = preset[1] << ADC_CCR_PRESC_Pos;
    auto smp = preset[2];
    m_timer_divisor = preset[3];

    adcStop(m_driver);

    // Adjust PLLSAI2
    RCC->CR &= ~(RCC_CR_PLLSAI2ON);
    while ((RCC->CR & RCC_CR_PLLSAI2RDY) == RCC_CR_PLLSAI2RDY);
    RCC->PLLSAI2CFGR = (RCC->PLLSAI2CFGR & ~(RCC_PLLSAI2CFGR_PLLSAI2N_Msk)) | plln;
    RCC->CR |= RCC_CR_PLLSAI2ON;
    // Set ADC prescaler
    m_driver->adcc->CCR = (m_driver->adcc->CCR & ~(ADC_CCR_PRESC_Msk)) | presc;
    // Set sampling time
    m_group_config.smpr[0] = ADC_SMPR1_SMP_AN5(smp);

    adcStart(m_driver, &m_config);
}

void ADC::setOperation(ADC::Operation operation)
{
    m_operation = operation;
}

int ADC::getRate()
{
    for (unsigned int i = 0; i < m_rate_presets.size(); i++) {
        if (m_timer_divisor == m_rate_presets[i][3])
            return i;
    }

    return -1;
}

unsigned int ADC::getTimerDivisor()
{
    return m_timer_divisor;
}
 
void ADC::conversionCallback(ADCDriver *driver)
{
    if (m_operation != nullptr) {
        auto half_size = m_current_buffer_size / 2;
        if (adcIsBufferComplete(driver))
            m_operation(m_current_buffer + half_size, half_size);
        else
            m_operation(m_current_buffer, half_size);
    }
}

