#include "ch.h"
#include "hal.h"

#include "adc.hpp"
#include "dac.hpp"
#include "usbserial.hpp"

#if CACHE_LINE_SIZE > 0
CC_ALIGN(CACHE_LINE_SIZE)
#endif
adcsample_t samples[CACHE_SIZE_ALIGN(adcsample_t, 10)];

int main(void) {
    halInit();
    chSysInit();

    palSetPadMode(GPIOA, 5,  PAL_MODE_OUTPUT_PUSHPULL); // LED

    ADCd adc (ADCD1, GPTD4);
    adc.start();

    //DACd dac (DACD1, {
    //    .init         = 0,
    //    .datamode     = DAC_DHRM_12BIT_RIGHT,
    //    .cr           = 0
    //});
    //dac.start();
    //dac.write(0, 1024);

    USBSeriald usbd (SDU1);
    usbd.start();

	while (true) {
        if (usbd.active()) {
            if (char c; usbd.read(&c) > 0 && c == 's') {
                adc.getSamples(samples, 10);
                for (int i = 0; i < 10; i++) {
                    uint8_t str[5] = {
                        static_cast<uint8_t>(samples[i] / 1000 % 10 + '0'),
                        static_cast<uint8_t>(samples[i] / 100 % 10 + '0'),
                        static_cast<uint8_t>(samples[i] / 10 % 10 + '0'),
                        static_cast<uint8_t>(samples[i] % 10 + '0'),
                        ' '
                    };
                    usbd.write(str, 5);
                }
                usbd.write("\r\n", 2);
            }
        }
		chThdSleepMilliseconds(250);
	}
}

