/**
 * @file main.cpp
 * @brief Program entry point.
 *
 * Copyright (C) 2020 Clyne Sullivan
 *
 * Distributed under the GNU GPL v3 or later. You should have received a copy of
 * the GNU General Public License along with this program.
 * If not, see <https://www.gnu.org/licenses/>.
 */

#include "ch.h"
#include "hal.h"

#include "adc.hpp"
#include "dac.hpp"
#include "elf_load.hpp"
#include "usbserial.hpp"

#include <array>

constexpr unsigned int MAX_SAMPLE_BUFFER_SIZE = 2048;

enum class RunStatus : char
{
    Idle = '1',
    Converting,
    Recovered
};
static RunStatus run_status = RunStatus::Idle;

#define MSG_CONVFIRST          (1)
#define MSG_CONVSECOND         (2)
#define MSG_CONVFIRST_MEASURE  (3)
#define MSG_CONVSECOND_MEASURE (4)

static msg_t conversionMBBuffer[8];
static MAILBOX_DECL(conversionMB, conversionMBBuffer, 8);

static THD_WORKING_AREA(conversionThreadWA, 1024);
static THD_FUNCTION(conversionThread, arg);

static time_measurement_t conversion_time_measurement;

static_assert(sizeof(adcsample_t) == sizeof(uint16_t));
static_assert(sizeof(dacsample_t) == sizeof(uint16_t));

#if CACHE_LINE_SIZE > 0
CC_ALIGN(CACHE_LINE_SIZE)
#endif
static std::array<adcsample_t, CACHE_SIZE_ALIGN(adcsample_t, MAX_SAMPLE_BUFFER_SIZE)> adc_samples;
#if CACHE_LINE_SIZE > 0
CC_ALIGN(CACHE_LINE_SIZE)
#endif
static std::array<dacsample_t, CACHE_SIZE_ALIGN(dacsample_t, MAX_SAMPLE_BUFFER_SIZE)> dac_samples;

static uint8_t elf_file_store[8192];
static elf::entry_t elf_entry = nullptr;

static void signal_operate(adcsample_t *buffer, size_t count);
static void signal_operate_measure(adcsample_t *buffer, size_t count);
static void main_loop();

int main()
{
    // Initialize the RTOS
    halInit();
    chSysInit();

    // Enable FPU
    SCB->CPACR |= 0xF << 20;

    // Prepare LED
    palSetPadMode(GPIOA, 5,  PAL_MODE_OUTPUT_PUSHPULL);
    palClearPad(GPIOA, 5);

    adc::init();
    dac::init();
    usbserial::init();

    // Start the conversion manager thread
    chTMObjectInit(&conversion_time_measurement);
    chThdCreateStatic(conversionThreadWA, sizeof(conversionThreadWA),
                      NORMALPRIO,
                      conversionThread, nullptr);

    main_loop();
}

void main_loop()
{
    static unsigned int dac_sample_count = MAX_SAMPLE_BUFFER_SIZE;

	while (1) {
        if (usbserial::is_active()) {
            // Attempt to receive a command packet
            if (char cmd[3]; usbserial::read(&cmd, 1) > 0) {
                // Packet received, first byte represents the desired command/action
                switch (cmd[0]) {

                // 'r' - Conduct a single sample of the ADC, and send the results back over USB.
                case 'r':
                    // Get the next two bytes of the packet to determine the desired sample size
                    if (run_status != RunStatus::Idle || usbserial::read(&cmd[1], 2) < 2)
                        break;
                    if (unsigned int desiredSize = cmd[1] | (cmd[2] << 8); desiredSize <= adc_samples.size()) {
                        adc::read(&adc_samples[0], desiredSize);
                        usbserial::write(adc_samples.data(), desiredSize * sizeof(adcsample_t));
                    }
                    break;

                // 'R' - Begin continuous sampling/conversion of the ADC. Samples will go through
                //       the conversion code, and will be sent out over the DAC.
                case 'R':
                    //if (run_status != RunStatus::Idle)
                    //    break;

                    run_status = RunStatus::Converting;
                    dac_samples.fill(0);
                    adc::read_start(signal_operate, &adc_samples[0], adc_samples.size());
                    dac::write_start(&dac_samples[0], dac_samples.size());
                    break;

                // 'M' - Begins continuous sampling, but measures the execution time of the first
                //       sample processing. This duration can be later read through 'm'.
                case 'M':
                    run_status = RunStatus::Converting;
                    dac_samples.fill(0);
                    adc::read_start(signal_operate_measure, &adc_samples[0], adc_samples.size());
                    dac::write_start(&dac_samples[0], dac_samples.size());
                    break;

                // 'm' - Returns the last measured sample processing time, presumably in processor
                //       ticks.
                case 'm':
                    usbserial::write(&conversion_time_measurement.last, sizeof(rtcnt_t));
                    break;

                // 's' - Sends the current contents of the DAC buffer back over USB.
                case 's':
                    usbserial::write(dac_samples.data(), 1/*dac_samples.size()*/ * sizeof(dacsample_t));
                    break;

                // 'S' - Stops the continuous sampling/conversion.
                case 'S':
                    //if (run_status != RunStatus::Converting)
                    //    break;

                    dac::write_stop();
                    adc::read_stop();
                    run_status = RunStatus::Idle;
                    break;

                // 'e' - Reads in and loads the compiled conversion code binary from USB.
                case 'e':
                    // Get the binary's size
                    if (usbserial::read(&cmd[1], 2) < 2)
                        break;

                    // Only load the binary if it can fit in the memory reserved for it.
                    if (unsigned int binarySize = cmd[1] | (cmd[2] << 8); binarySize < sizeof(elf_file_store)) {
                        usbserial::read(elf_file_store, binarySize);
                        elf_entry = elf::load(elf_file_store);
                    }
                    break;

                // 'E' - Unloads the currently loaded conversion code
                case 'E':
                    elf_entry = nullptr;
                    break;

                // 'W' - Sets the number of samples for DAC writing with command 'w'.
                //       If the provided count is zero, DAC writing is stopped.
                case 'W':
                    if (usbserial::read(&cmd[1], 2) < 2)
                        break;
                    if (unsigned int sampleCount = cmd[1] | (cmd[2] << 8); sampleCount <= dac_samples.size()) {
                        if (sampleCount > 0)
                            dac_sample_count = sampleCount;
                        else
                            dac::write_stop();
                    }
                    break;

                // 'w' - Starts the DAC, looping over the given data (data size set by command 'W').
                case 'w':
                    if (usbserial::read(&dac_samples[0], dac_sample_count * sizeof(dacsample_t) !=
                        dac_sample_count * sizeof(dacsample_t)))
                    {
                        break;
                    } else {
                        dac::write_start(&dac_samples[0], dac_sample_count);
                    }
                    break;

                // 'i' - Sends an identifying string to confirm that this is the stmdsp device.
                case 'i':
                    usbserial::write("stmdsp", 6);
                    break;

                // 'I' - Sends the current run status.
                case 'I':
                    usbserial::write(&run_status, sizeof(run_status));
                    break;

                default:
                    break;
                }
            }
        }

		chThdSleepMilliseconds(1);
	}
}

void conversion_abort()
{
    elf_entry = nullptr;
    dac::write_stop();
    adc::read_stop();
    run_status = RunStatus::Recovered;
}

THD_FUNCTION(conversionThread, arg)
{
    (void)arg;

    while (1) {
        msg_t message;
        if (chMBFetchTimeout(&conversionMB, &message, TIME_INFINITE) == MSG_OK) {
            adcsample_t *samples = nullptr;
            auto halfsize = adc_samples.size() / 2;
            if (message == MSG_CONVFIRST) {
                if (elf_entry)
                    samples = elf_entry(&adc_samples[0], halfsize);
                if (!samples)
                    samples = &adc_samples[0];
                std::copy(samples, samples + halfsize, &dac_samples[0]);
            } else if (message == MSG_CONVSECOND) {
                if (elf_entry)
                    samples = elf_entry(&adc_samples[adc_samples.size() / 2], halfsize);
                if (!samples)
                    samples = &adc_samples[adc_samples.size() / 2];
                std::copy(samples, samples + halfsize, &dac_samples[dac_samples.size() / 2]);
            } else if (message == MSG_CONVFIRST_MEASURE) {
                chTMStartMeasurementX(&conversion_time_measurement);
                if (elf_entry)
                    samples = elf_entry(&adc_samples[adc_samples.size() / 2], halfsize);
                chTMStopMeasurementX(&conversion_time_measurement);
                if (!samples)
                    samples = &adc_samples[adc_samples.size() / 2];
                std::copy(samples, samples + halfsize, &dac_samples[dac_samples.size() / 2]);
            } else if (message == MSG_CONVSECOND_MEASURE) {
                chTMStartMeasurementX(&conversion_time_measurement);
                if (elf_entry)
                    samples = elf_entry(&adc_samples[adc_samples.size() / 2], halfsize);
                chTMStopMeasurementX(&conversion_time_measurement);
                if (!samples)
                    samples = &adc_samples[adc_samples.size() / 2];
                std::copy(samples, samples + halfsize, &dac_samples[dac_samples.size() / 2]);
            }
        }
    }
}

void signal_operate(adcsample_t *buffer, [[maybe_unused]] size_t count)
{
    if (chMBGetUsedCountI(&conversionMB) > 1)
        conversion_abort();
    else
        chMBPostI(&conversionMB, buffer == &adc_samples[0] ? MSG_CONVFIRST : MSG_CONVSECOND);
}

void signal_operate_measure(adcsample_t *buffer, [[maybe_unused]] size_t count)
{
    chMBPostI(&conversionMB, buffer == &adc_samples[0] ? MSG_CONVFIRST_MEASURE : MSG_CONVSECOND_MEASURE);
    adc::read_set_operation_func(signal_operate);
}

extern "C" {

__attribute__((naked))
void HardFault_Handler()
{
    //asm("push {lr}");

    uint32_t *stack;
    uint32_t lr;
	asm("\
		tst lr, #4; \
		ite eq; \
		mrseq %0, msp; \
		mrsne %0, psp; \
        mov %1, lr; \
	" : "=r" (stack), "=r" (lr));
    //stack++;
    stack[7] |= (1 << 24); // Keep Thumb mode enabled

    conversion_abort();

    // TODO test lr and decide how to recover

    //if (run_status == RunStatus::Converting) {
        stack[6] = stack[5];   // Escape from elf_entry code
    //} else /*if (run_status == RunStatus::Recovered)*/ {
    //    stack[6] = (uint32_t)main_loop & ~1; // Return to safety
    //}

    //asm("pop {lr}; bx lr");
    asm("bx lr");
}

} // extern "C"

