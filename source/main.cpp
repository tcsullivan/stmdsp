#include "ch.h"
#include "hal.h"
#include "usbcfg.h"

class dDAC
{
public:
    constexpr dDAC(DACDriver& driver, const DACConfig& config) :
        m_driver(&driver), m_config(config) {}

    void init() {
        dacStart(m_driver, &m_config);
    }

    void writeX(unsigned int channel, uint16_t value) {
        if (channel < 2)
            dacPutChannelX(m_driver, channel, value);
    }

private:
    DACDriver *m_driver;
    DACConfig m_config;
};

//static const DACConversionGroup dacGroupConfig = {
//  .num_channels = 1,
//  .end_cb       = NULL,
//  .error_cb     = NULL,
//  .trigger      = DAC_TRG(0)
//};

class dGPT {
public:
    constexpr dGPT(GPTDriver& driver, const GPTConfig& config) :
        m_driver(&driver), m_config(config) {}

    void init() {
        gptStart(m_driver, &m_config);
    }

    void startContinuous(unsigned int interval) {
        gptStartContinuous(m_driver, interval);
    }

    void stop() {
        gptStopTimer(m_driver);
    }

private:
    GPTDriver *m_driver;
    GPTConfig m_config;
};

static dGPT gpt (GPTD4, {
  .frequency    =  1000000,
  .callback     =  NULL,
  .cr2          =  TIM_CR2_MMS_1,   /* MMS = 010 = TRGO on Update Event.    */
  .dier         =  0
});

static const ADCConfig adcConfig = {
  .difsel = 0
};

volatile bool adcFinished = false;
void adcEndCallback(ADCDriver *adcd)
{
    (void)adcd;
    gpt.stop();
    adcFinished = true;
}

static const ADCConversionGroup adcGroupConfig = {
  .circular     = false,
  .num_channels = 1,
  .end_cb       = adcEndCallback,
  .error_cb     = NULL,
  .cfgr         = ADC_CFGR_EXTEN_RISING |
                  ADC_CFGR_EXTSEL_SRC(12),  /* TIM4_TRGO */
  .cfgr2        = 0,
  .tr1          = ADC_TR(0, 4095),
  .smpr         = {
    ADC_SMPR1_SMP_AN5(ADC_SMPR_SMP_247P5), 0
  },
  .sqr          = {
    ADC_SQR1_SQ1_N(ADC_CHANNEL_IN5),
    0, 0, 0
  }
};

#if CACHE_LINE_SIZE > 0
CC_ALIGN(CACHE_LINE_SIZE)
#endif
adcsample_t samples[CACHE_SIZE_ALIGN(adcsample_t, 10)];

int main(void) {
	halInit();
	chSysInit();

	palSetPadMode(GPIOA, 5,  PAL_MODE_OUTPUT_PUSHPULL); // LED
	palSetPadMode(GPIOA, 11, PAL_MODE_ALTERNATE(10));   // USB
	palSetPadMode(GPIOA, 12, PAL_MODE_ALTERNATE(10));
    palSetPadMode(GPIOA, 0,  PAL_MODE_INPUT_ANALOG);    // Channel A in (1in5)

    palSetPadMode(GPIOA, 4, PAL_MODE_INPUT_ANALOG);     // DAC out1, out2
    palSetPadMode(GPIOA, 5, PAL_MODE_INPUT_ANALOG);

    gpt.init();

    //dDAC dac (DACD1, {
    //    .init         = 0,
    //    .datamode     = DAC_DHRM_12BIT_RIGHT,
    //    .cr           = 0
    //});

    //dac.init();
    //dac.writeX(0, 1024);

    adcStart(&ADCD1, &adcConfig);
    adcSTM32EnableVREF(&ADCD1);

	sduObjectInit(&SDU1);
	sduStart(&SDU1, &serusbcfg);
	usbDisconnectBus(serusbcfg.usbp);
	chThdSleepMilliseconds(1500);
	usbStart(serusbcfg.usbp, &usbcfg);
	usbConnectBus(serusbcfg.usbp);

	while (true) {
        if (SDU1.config->usbp->state == USB_ACTIVE) {
            BaseSequentialStream *bss = (BaseSequentialStream *)&SDU1;
            char c = 0;
            if (streamRead(bss, (uint8_t *)&c, 1) > 0 && c == 's') {
                adcFinished = false;
                adcStartConversion(&ADCD1, &adcGroupConfig, samples, 10);
                gpt.startContinuous(100);
                while (!adcFinished);
                for (int i = 0; i < 10; i++) {
                    uint8_t str[5] = {
                        static_cast<uint8_t>(samples[i] / 1000 % 10 + '0'),
                        static_cast<uint8_t>(samples[i] / 100 % 10 + '0'),
                        static_cast<uint8_t>(samples[i] / 10 % 10 + '0'),
                        static_cast<uint8_t>(samples[i] % 10 + '0'),
                        ' '
                    };
                    streamWrite(bss, str, 5);
                }
                streamWrite(bss, (uint8_t *)"\r\n", 2);
            }
        }
		chThdSleepMilliseconds(250);
	}
}

