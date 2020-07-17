/**
 * @file adc.hpp
 * @brief Wrapper for ChibiOS's ADCDriver.
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

class ADCd;

struct ADCdConfig : public ADCConfig
{
    ADCd *adcdinst;
};

enum class ADCdRate : unsigned int {
    R2P5,
    R6P5,
    R12P5,
    R24P5,
    R47P5,
    R92P5,
    R247P5,
    R640P5
};

class ADCd
{
public:
    constexpr static const unsigned int CLOCK_RATE = 40000000;
    constexpr static unsigned int SAMPLES_PER_SECOND(ADCdRate rate) {
        unsigned int sps = 0;
        switch (rate) {
        case ADCdRate::R2P5:
            sps = 15;
            break;
        case ADCdRate::R6P5:
            sps = 19;
            break;
        case ADCdRate::R12P5:
            sps = 25;
            break;
        case ADCdRate::R24P5:
            sps = 37;
            break;
        case ADCdRate::R47P5:
            sps = 60;
            break;
        case ADCdRate::R92P5:
            sps = 105;
            break;
        case ADCdRate::R247P5:
            sps = 260;
            break;
        case ADCdRate::R640P5:
            sps = 653;
            break;
        }

        return static_cast<unsigned int>(1.f / (sps / static_cast<float>(CLOCK_RATE)));
    }

    constexpr explicit ADCd(ADCDriver& adcd, GPTDriver& gptd) :
        m_adcd(&adcd), m_gptd(&gptd), m_adc_config{},
        m_adc_group_config(ADC_GROUP_CONFIG),
        m_is_adc_finished(false) {}

    void start();
    adcsample_t *getSamples(adcsample_t *buffer, size_t count);
    void setSampleRate(ADCdRate rate);

private:
    static const GPTConfig m_gpt_config;

    ADCDriver *m_adcd;
    GPTDriver *m_gptd;
    ADCdConfig m_adc_config;
    ADCConversionGroup m_adc_group_config;

    bool m_is_adc_finished;

    void initPins();
    //void selectPins(bool a0, bool a1);

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

