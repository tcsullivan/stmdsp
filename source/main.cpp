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

constexpr unsigned int MAX_ELF_FILE_SIZE = 12 * 1024;
constexpr unsigned int MAX_ERROR_QUEUE_SIZE = 8;
constexpr unsigned int MAX_SAMPLE_BUFFER_SIZE = 8000;

enum class RunStatus : char
{
    Idle = '1',
    Running
};
enum class Error : char
{
    None = 0,
    BadParam,
    BadParamSize,
    BadUserCodeLoad,
    BadUserCodeSize,
    NotIdle,
    ConversionAborted
};

static RunStatus run_status = RunStatus::Idle;
static Error error_queue[MAX_ERROR_QUEUE_SIZE];
static unsigned int error_queue_index = 0;

static void error_queue_add(Error error)
{
    if (error_queue_index < MAX_ERROR_QUEUE_SIZE)
        error_queue[error_queue_index++] = error;
}
static Error error_queue_pop()
{
    return error_queue_index == 0 ? Error::None : error_queue[--error_queue_index];
}

#define MSG_CONVFIRST          (1)
#define MSG_CONVSECOND         (2)
#define MSG_CONVFIRST_MEASURE  (3)
#define MSG_CONVSECOND_MEASURE (4)

static msg_t conversionMBBuffer[4];
static MAILBOX_DECL(conversionMB, conversionMBBuffer, 4);

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
#if CACHE_LINE_SIZE > 0
CC_ALIGN(CACHE_LINE_SIZE)
#endif
static std::array<dacsample_t, CACHE_SIZE_ALIGN(dacsample_t, MAX_SAMPLE_BUFFER_SIZE)> dac2_samples;

static uint8_t elf_file_store[MAX_ELF_FILE_SIZE];
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

static unsigned int dac_sample_count = MAX_SAMPLE_BUFFER_SIZE;
static unsigned int dac2_sample_count = MAX_SAMPLE_BUFFER_SIZE;
static unsigned int adc_sample_count = MAX_SAMPLE_BUFFER_SIZE;
static bool adc_preloaded = false;
static bool dac_preloaded = false;

void main_loop()
{

	while (1) {
        if (usbserial::is_active()) {
            // Attempt to receive a command packet
            if (char cmd[3]; usbserial::read(&cmd, 1) > 0) {
                // Packet received, first byte represents the desired command/action
                switch (cmd[0]) {

                case 'a':
                    usbserial::write(adc_samples.data(), adc_sample_count * sizeof(adcsample_t));
                    break;
                case 'A':
                    usbserial::read(&adc_samples[0], adc_sample_count * sizeof(adcsample_t));
                    break;

                case 'b':
                    break;
                case 'B':
                    if (run_status == RunStatus::Idle) {
                        if (usbserial::read(&cmd[1], 2) == 2) {
                            unsigned int count = cmd[1] | (cmd[2] << 8);
                            if (count <= MAX_SAMPLE_BUFFER_SIZE / 2) {
                                adc_sample_count = count * 2;
                                dac_sample_count = count * 2;
                            } else {
                                error_queue_add(Error::BadParam);
                            }
                        } else {
                            error_queue_add(Error::BadParamSize);
                        }
                    } else {
                        error_queue_add(Error::NotIdle);
                    }
                    break;

                case 'd':
                    usbserial::write(dac_samples.data(), dac_sample_count * sizeof(dacsample_t));
                    break;
                case 'D':
                    if (usbserial::read(&cmd[1], 2) == 2) {
                        unsigned int count = cmd[1] | (cmd[2] << 8);
                        if (count <= MAX_SAMPLE_BUFFER_SIZE / 2) {
                            dac2_sample_count = count;
                            usbserial::read(&dac2_samples[0], dac2_sample_count * sizeof(dacsample_t));
                        } else {
                            error_queue_add(Error::BadParam);
                        }
                    } else {
                        error_queue_add(Error::BadParamSize);
                    }
                    break;

                // 'E' - Reads in and loads the compiled conversion code binary from USB.
                case 'E':
                    if (run_status == RunStatus::Idle) {
                        if (usbserial::read(&cmd[1], 2) == 2) {
                            // Only load the binary if it can fit in the memory reserved for it.
                            unsigned int size = cmd[1] | (cmd[2] << 8);
                            if (size < sizeof(elf_file_store)) {
                                usbserial::read(elf_file_store, size);
                                elf_entry = elf::load(elf_file_store);

                                if (elf_entry == nullptr)
                                    error_queue_add(Error::BadUserCodeLoad);
                            } else {
                                error_queue_add(Error::BadUserCodeSize);
                            }
                        } else {
                            error_queue_add(Error::BadParamSize);
                        }
                    } else {
                        error_queue_add(Error::NotIdle);
                    }
                    break;

                // 'e' - Unloads the currently loaded conversion code
                case 'e':
                    elf_entry = nullptr;
                    break;

                // 'i' - Sends an identifying string to confirm that this is the stmdsp device.
                case 'i':
                    usbserial::write("stmdsp", 6);
                    break;

                // 'I' - Sends the current run status.
                case 'I':
                    {
                        char buf[2] = {
                            static_cast<char>(run_status),
                            static_cast<char>(error_queue_pop())
                        };
                        usbserial::write(buf, sizeof(buf));
                    }
                    break;

                // 'M' - Begins continuous sampling, but measures the execution time of the first
                //       sample processing. This duration can be later read through 'm'.
                case 'M':
                    if (run_status == RunStatus::Idle) {
                        run_status = RunStatus::Running;
                        dac_samples.fill(0);
                        if (!adc_preloaded)
                            adc::read_start(signal_operate_measure, &adc_samples[0], adc_sample_count);
                        if (!dac_preloaded)
                            dac::write_start(0, &dac_samples[0], dac_sample_count);
                    } else {
                        error_queue_add(Error::NotIdle);
                    }
                    break;

                // 'm' - Returns the last measured sample processing time, presumably in processor
                //       ticks.
                case 'm':
                    usbserial::write(&conversion_time_measurement.last, sizeof(rtcnt_t));
                    break;

                // 'R' - Begin continuous sampling/conversion of the ADC. Samples will go through
                //       the conversion code, and will be sent out over the DAC.
                case 'R':
                    if (run_status == RunStatus::Idle) {
                        run_status = RunStatus::Running;
                        dac_samples.fill(0);
                        if (!adc_preloaded)
                            adc::read_start(signal_operate, &adc_samples[0], adc_sample_count);
                        if (!dac_preloaded)
                            dac::write_start(0, &dac_samples[0], dac_sample_count);
                    } else {
                        error_queue_add(Error::NotIdle);
                    }
                    break;

                case 'r':
                    if (usbserial::read(&cmd[1], 1) == 1) {
                        adc_preloaded = cmd[1] & (1 << 0);
                        dac_preloaded = cmd[1] & (1 << 1);
                    } else {
                        error_queue_add(Error::BadParamSize);
                    }
                    break;

                // 'S' - Stops the continuous sampling/conversion.
                case 'S':
                    if (run_status == RunStatus::Running) {
                        if (!dac_preloaded)
                            dac::write_stop(0);
                        if (!adc_preloaded)
                            adc::read_stop();
                        run_status = RunStatus::Idle;
                    }
                    break;

                case 'W':
                    dac::write_start(1, &dac2_samples[0], dac2_sample_count);
                    break;
                case 'w':
                    dac::write_stop(1);
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
    if (!dac_preloaded)
        dac::write_stop(0);
    if (!adc_preloaded)
        adc::read_stop();
    error_queue_add(Error::ConversionAborted);
}

THD_FUNCTION(conversionThread, arg)
{
    (void)arg;

    while (1) {
        msg_t message;
        if (chMBFetchTimeout(&conversionMB, &message, TIME_INFINITE) == MSG_OK) {
            adcsample_t *samples = nullptr;
            auto halfsize = adc_sample_count / 2;
            if (message == MSG_CONVFIRST) {
                if (elf_entry)
                    samples = elf_entry(&adc_samples[0], halfsize);
                if (!samples)
                    samples = &adc_samples[0];
                std::copy(samples, samples + halfsize, &dac_samples[0]);
            } else if (message == MSG_CONVSECOND) {
                if (elf_entry)
                    samples = elf_entry(&adc_samples[halfsize], halfsize);
                if (!samples)
                    samples = &adc_samples[halfsize];
                std::copy(samples, samples + halfsize, &dac_samples[dac_sample_count / 2]);
            } else if (message == MSG_CONVFIRST_MEASURE) {
                chTMStartMeasurementX(&conversion_time_measurement);
                if (elf_entry)
                    samples = elf_entry(&adc_samples[0], halfsize);
                chTMStopMeasurementX(&conversion_time_measurement);
                if (!samples)
                    samples = &adc_samples[0];
                std::copy(samples, samples + halfsize, &dac_samples[0]);
            } else if (message == MSG_CONVSECOND_MEASURE) {
                chTMStartMeasurementX(&conversion_time_measurement);
                if (elf_entry)
                    samples = elf_entry(&adc_samples[halfsize], halfsize);
                chTMStopMeasurementX(&conversion_time_measurement);
                if (!samples)
                    samples = &adc_samples[halfsize];
                std::copy(samples, samples + halfsize, &dac_samples[dac_sample_count / 2]);
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

