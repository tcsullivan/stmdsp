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

const ADCConfig ADC::m_config = {
    .difsel = 0,
    .calibration = 0,
};

const ADCConversionGroup ADC::m_group_config = {
    .circular = true,
    .num_channels = 1,
    .end_cb = ADC::conversionCallback,
    .error_cb = nullptr,
    .cfgr = ADC_CFGR_EXTEN_RISING | ADC_CFGR_EXTSEL_SRC(13),  /* TIM6_TRGO */
    .cfgr2 = ADC_CFGR2_ROVSE | ADC_CFGR2_OVSR_0 | ADC_CFGR2_OVSS_1, // Oversampling 2x
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

std::array<std::array<uint32_t, 2>, 6> ADC::m_rate_presets = {{
     // Rate   PLL N  PLL P
    {/* 8k  */ 80,    20},
    {/* 16k */ 80,    10},
    {/* 20k */ 80,    8},
    {/* 32k */ 80,    5},
    {/* 48k */ 96,    4},
    {/* 96k */ 96,    2}
}};

adcsample_t *ADC::m_current_buffer = nullptr;
size_t ADC::m_current_buffer_size = 0;
ADC::Operation ADC::m_operation = nullptr;

void ADC::begin()
{
    palSetPadMode(GPIOF, 3, PAL_MODE_INPUT_ANALOG);

    adcStart(m_driver, &m_config);
    adcSTM32EnableVREF(m_driver);

    setRate(SClock::Rate::R32K);
}

void ADC::start(adcsample_t *buffer, size_t count, Operation operation)
{
    m_current_buffer = buffer;
    m_current_buffer_size = count;
    m_operation = operation;

    adcStartConversion(m_driver, &m_group_config, buffer, count);
    SClock::start();
}

void ADC::stop()
{
    SClock::stop();
    adcStopConversion(m_driver);

    m_current_buffer = nullptr;
    m_current_buffer_size = 0;
    m_operation = nullptr;
}

void ADC::setRate(SClock::Rate rate)
{
    auto& preset = m_rate_presets[static_cast<unsigned int>(rate)];
    auto pllbits = (preset[0] << RCC_PLL2DIVR_N2_Pos) |
                   (preset[1] << RCC_PLL2DIVR_P2_Pos);

    adcStop(m_driver);

    // Adjust PLL2
    RCC->CR &= ~(RCC_CR_PLL2ON);
    while ((RCC->CR & RCC_CR_PLL2RDY) == RCC_CR_PLL2RDY);
    auto pll2divr = RCC->PLL2DIVR &
                    ~(RCC_PLL2DIVR_N2_Msk | RCC_PLL2DIVR_P2_Msk);
    pll2divr |= pllbits;
    RCC->PLL2DIVR = pll2divr;
    RCC->CR |= RCC_CR_PLL2ON;
    while ((RCC->CR & RCC_CR_PLL2RDY) != RCC_CR_PLL2RDY);

    adcStart(m_driver, &m_config);
}

void ADC::setOperation(ADC::Operation operation)
{
    m_operation = operation;
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

