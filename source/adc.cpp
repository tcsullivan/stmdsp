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

ADCDriver *ADC::m_driver = &ADCD3;
GPTDriver *ADC::m_timer = &GPTD6;

const ADCConfig ADC::m_config = {
    .difsel = 0,
    .calibration = 0,
};

ADCConversionGroup ADC::m_group_config = {
    .circular = true,
    .num_channels = 1,
    .end_cb = ADC::conversionCallback,
    .error_cb = nullptr,
    .cfgr = ADC_CFGR_EXTEN_RISING | ADC_CFGR_EXTSEL_SRC(13),  /* TIM6_TRGO */
    .cfgr2 = 0,//ADC_CFGR2_ROVSE | ADC_CFGR2_OVSR_0 | ADC_CFGR2_OVSS_1, // Oversampling 2x
    .ccr = 0,
    .pcsel = 0,
    .ltr1 = 0, .htr1 = 0x0FFF,
    .ltr2 = 0, .htr2 = 0x0FFF,
    .ltr3 = 0, .htr3 = 0x0FFF,
    .smpr = {
        ADC_SMPR1_SMP_AN5(ADC_SMPR_SMP_12P5), 0
    },
    .sqr = {
        ADC_SQR1_SQ1_N(ADC_CHANNEL_IN5),
        0, 0, 0
    },
};

const GPTConfig ADC::m_timer_config = {
    .frequency = 4800000,
    .callback = nullptr,
    .cr2 = TIM_CR2_MMS_1, /* TRGO */
    .dier = 0
};

std::array<std::array<uint32_t, 4>, 6> ADC::m_rate_presets = {{
     // Rate   PLLSAI2N  R  OVERSAMPLE 2x?  GPT_DIV
    {/* 8k  */ 16,       3, 1,              4500},
    {/* 16k */ 32,       3, 1,              2250},
    {/* 20k */ 40,       3, 1,              1800},
    {/* 32k */ 64,       3, 1,              1125},
    {/* 48k */ 24,       3, 0,               750},
    {/* 96k */ 48,       3, 0,               375}
}};

adcsample_t *ADC::m_current_buffer = nullptr;
size_t ADC::m_current_buffer_size = 0;
ADC::Operation ADC::m_operation = nullptr;

unsigned int ADC::m_timer_divisor = 50;

void ADC::begin()
{
    palSetPadMode(GPIOF, 3, PAL_MODE_INPUT_ANALOG);

    adcStart(m_driver, &m_config);
    adcSTM32EnableVREF(m_driver);
    gptStart(m_timer, &m_timer_config);

    //setRate(Rate::R32K);
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
//    auto& preset = m_rate_presets[static_cast<int>(rate)];
//    auto pllnr = (preset[0] << RCC_PLLSAI2CFGR_PLLSAI2N_Pos) |
//                 (preset[1] << RCC_PLLSAI2CFGR_PLLSAI2R_Pos);
//    bool oversample = preset[2] != 0;
//    m_timer_divisor = preset[3];

//    adcStop(m_driver);
//
//    // Adjust PLLSAI2
//    RCC->CR &= ~(RCC_CR_PLLSAI2ON);
//    while ((RCC->CR & RCC_CR_PLLSAI2RDY) == RCC_CR_PLLSAI2RDY);
//    RCC->PLLSAI2CFGR = (RCC->PLLSAI2CFGR & ~(RCC_PLLSAI2CFGR_PLLSAI2N_Msk | RCC_PLLSAI2CFGR_PLLSAI2R_Msk)) | pllnr;
//    RCC->CR |= RCC_CR_PLLSAI2ON;
//    while ((RCC->CR & RCC_CR_PLLSAI2RDY) != RCC_CR_PLLSAI2RDY);
//
//    // Set 2x oversampling
//    m_group_config.cfgr2 = oversample ? ADC_CFGR2_ROVSE | ADC_CFGR2_OVSR_0 | ADC_CFGR2_OVSS_1 : 0;
//
//    adcStart(m_driver, &m_config);
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

