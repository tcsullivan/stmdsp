/**
 * @file adc.cpp
 * @brief Wrapper for ChibiOS's ADCDriver.
 *
 * Copyright (C) 2020 Clyne Sullivan
 *
 * Distributed under the GNU GPL v3 or later. You should have received a copy of
 * the GNU General Public License along with this program.
 * If not, see <https://www.gnu.org/licenses/>.
 */

#include "adc.hpp"

const GPTConfig ADCd::m_gpt_config = {
  .frequency    =  1000000,
  .callback     =  NULL,
  .cr2          =  TIM_CR2_MMS_1,   /* MMS = 010 = TRGO on Update Event.    */
  .dier         =  0
};

void ADCd::start()
{
    initPins();
    gptStart(m_gptd, &m_gpt_config);

    m_adc_config.difsel = 0;
    m_adc_config.adcdinst = this;

    adcStart(m_adcd, &m_adc_config);
    adcSTM32EnableVREF(m_adcd);
}

adcsample_t *ADCd::getSamples(adcsample_t *buffer, size_t count)
{
    m_is_adc_finished = false;
    adcStartConversion(m_adcd, &m_adc_group_config, buffer, count);
    gptStartContinuous(m_gptd, 100); // 10kHz
    while (!m_is_adc_finished);
    return buffer;
}

void ADCd::setSampleRate(ADCdRate rate)
{
    uint32_t val = 0;

    switch (rate) {
    case ADCdRate::R2P5:
        val = ADC_SMPR1_SMP_AN5(ADC_SMPR_SMP_2P5);
        break;
    case ADCdRate::R6P5:
        val = ADC_SMPR1_SMP_AN5(ADC_SMPR_SMP_6P5);
        break;
    case ADCdRate::R12P5:
        val = ADC_SMPR1_SMP_AN5(ADC_SMPR_SMP_12P5);
        break;
    case ADCdRate::R24P5:
        val = ADC_SMPR1_SMP_AN5(ADC_SMPR_SMP_24P5);
        break;
    case ADCdRate::R47P5:
        val = ADC_SMPR1_SMP_AN5(ADC_SMPR_SMP_47P5);
        break;
    case ADCdRate::R92P5:
        val = ADC_SMPR1_SMP_AN5(ADC_SMPR_SMP_92P5);
        break;
    case ADCdRate::R247P5:
        val = ADC_SMPR1_SMP_AN5(ADC_SMPR_SMP_247P5);
        break;
    case ADCdRate::R640P5:
        val = ADC_SMPR1_SMP_AN5(ADC_SMPR_SMP_640P5);
        break;
    }

    m_adc_group_config.smpr[0] = val;
}

void ADCd::initPins()
{
    palSetPadMode(GPIOA, 0, PAL_MODE_INPUT_ANALOG);
}

void ADCd::adcEndCallback(ADCDriver *adcd)
{
    auto *_this = reinterpret_cast<const ADCdConfig *>(adcd->config)->adcdinst;
    gptStopTimer(_this->m_gptd);
    _this->m_is_adc_finished = true;
}

