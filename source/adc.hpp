#ifndef STMDSP_ADC_HPP_
#define STMDSP_ADC_HPP_

#include "hal.h"

class ADCd;

struct ADCdConfig : public ADCConfig
{
    ADCd *adcdinst;
};

class ADCd
{
public:
    constexpr explicit ADCd(ADCDriver& adcd, GPTDriver& gptd) :
        m_adcd(&adcd), m_gptd(&gptd), m_adc_config{},
        m_adc_group_config(ADC_GROUP_CONFIG),
        m_is_adc_finished(false) {}

    void start();
    adcsample_t *getSamples(adcsample_t *buffer, size_t count);

private:
    static const GPTConfig m_gpt_config;

    ADCDriver *m_adcd;
    GPTDriver *m_gptd;
    ADCdConfig m_adc_config;
    ADCConversionGroup m_adc_group_config;

    bool m_is_adc_finished;

    void initPins();
    static void adcEndCallback(ADCDriver *adcd);

    constexpr static const ADCConversionGroup ADC_GROUP_CONFIG = {
        .circular = false,
        .num_channels = 1,
        .end_cb = ADCd::adcEndCallback,
        .error_cb = nullptr,
        .cfgr = ADC_CFGR_EXTEN_RISING |
                ADC_CFGR_EXTSEL_SRC(12),  /* TIM4_TRGO */
        .cfgr2 = 0,
        .tr1 = ADC_TR(0, 4095),
        .smpr = {
            ADC_SMPR1_SMP_AN5(ADC_SMPR_SMP_247P5), 0
        },
        .sqr = {
            ADC_SQR1_SQ1_N(ADC_CHANNEL_IN5),
            0, 0, 0
        }
    };
};

#endif // STMDSP_ADC_HPP_

