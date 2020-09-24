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

enum class RunStatus : char
{
    Idle = '1',
    Converting,
    Recovered
};
static RunStatus run_status = RunStatus::Idle;

#define MSG_CONVFIRST  (1)
#define MSG_CONVSECOND (2)

static msg_t conversionMBBuffer[8];
static MAILBOX_DECL(conversionMB, conversionMBBuffer, 8);

static THD_WORKING_AREA(conversionThreadWA, 1024);
static THD_FUNCTION(conversionThread, arg);

static_assert(sizeof(adcsample_t) == sizeof(uint16_t));
static_assert(sizeof(dacsample_t) == sizeof(uint16_t));

#if CACHE_LINE_SIZE > 0
CC_ALIGN(CACHE_LINE_SIZE)
#endif
static std::array<adcsample_t, CACHE_SIZE_ALIGN(adcsample_t, 2048)> adc_samples;
#if CACHE_LINE_SIZE > 0
CC_ALIGN(CACHE_LINE_SIZE)
#endif
static std::array<dacsample_t, CACHE_SIZE_ALIGN(dacsample_t, 2048)> dac_samples;

static uint8_t elf_file_store[2048];
static uint8_t elf_exec_store[4096];
static elf::entry_t elf_entry = nullptr;

static void signal_operate(adcsample_t *buffer, size_t count);
static void main_loop();

int main()
{
    halInit();
    chSysInit();

    // Enable FPU
    SCB->CPACR |= 0xF << 20;

    palSetPadMode(GPIOA, 5,  PAL_MODE_OUTPUT_PUSHPULL); // LED

    adc::init();
    dac::init();
    usbserial::init();

    chThdCreateStatic(conversionThreadWA, sizeof(conversionThreadWA),
                      NORMALPRIO,
                      conversionThread, nullptr);
    main_loop();
}

void main_loop()
{
    static unsigned int dac_sample_count = 2048;
	while (true) {
        if (usbserial::is_active()) {
            // Expect to receive a byte command 'packet'.
            if (char cmd[3]; usbserial::read(&cmd, 1) > 0) {
                switch (cmd[0]) {
                case 'r': // Read in analog signal
                    if (usbserial::read(&cmd[1], 2) < 2)
                        break;
                    if (auto count = std::min(static_cast<unsigned int>(cmd[1] | (cmd[2] << 8)), adc_samples.size()); count > 0) {
                        adc::read(&adc_samples[0], count);
                        usbserial::write(adc_samples.data(), count * sizeof(adcsample_t));
                    }
                    break;
                case 'R':
                    run_status = RunStatus::Converting;
                    dac_samples.fill(0);
                    adc::read_start(signal_operate, &adc_samples[0], adc_samples.size());
                    dac::write_start(&dac_samples[0], dac_samples.size());
                    break;
                case 's':
                    usbserial::write(dac_samples.data(), dac_samples.size() * sizeof(adcsample_t));
                    break;
                case 'S':
                    dac::write_stop();
                    adc::read_stop();
                    run_status = RunStatus::Idle;
                    break;
                case 'e':
                    if (usbserial::read(&cmd[1], 2) < 2)
                        break;
                    if (unsigned int count = cmd[1] | (cmd[2] << 8); count < sizeof(elf_file_store)) {
                        usbserial::read(elf_file_store, count);
                        elf_entry = elf::load(elf_file_store, elf_exec_store);
                    }
                    break;
                case 'W':
                    if (usbserial::read(&cmd[1], 2) < 2)
                        break;
                    if (auto count = std::min(static_cast<unsigned int>(cmd[1] | (cmd[2] << 8)), dac_samples.size()); count > 0)
                        dac_sample_count = count;
                    else
                        dac::write_stop();
                    break;
                case 'w':
                    if (usbserial::read(&dac_samples[0], 2 * dac_sample_count) != 2 * dac_sample_count)
                        break;
                    dac::write_start(&dac_samples[0], dac_sample_count);
                    break;
                case 'i': // Identify ourself as an stmdsp device
                    usbserial::write("stmdsp", 6);
                    break;
                case 'I': // Info (i.e. run status)
                    usbserial::write(&run_status, 1);
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
            auto samples = &adc_samples[0];
            auto halfsize = adc_samples.size() / 2;
            if (message == MSG_CONVFIRST) {
                if (elf_entry)
                    elf_entry(samples, halfsize);
                std::copy(samples, samples + halfsize, &dac_samples[0]);
            } else if (message == MSG_CONVSECOND) {
                if (elf_entry)
                    elf_entry(samples + halfsize, halfsize);
                std::copy(samples + halfsize, samples + halfsize * 2, &dac_samples[1024]);
            }
        }
    }
}

void signal_operate(adcsample_t *buffer, size_t count)
{
    if (chMBGetUsedCountI(&conversionMB) > 1)
        conversion_abort();
    else
        chMBPostI(&conversionMB, buffer == &adc_samples[0] ? MSG_CONVFIRST : MSG_CONVSECOND);
}

extern "C" {

__attribute__((naked))
void HardFault_Handler()
{
    asm("push {lr}");

    uint32_t *stack;
    asm("mrs %0, psp" : "=r" (stack));
    //stack++;
    stack[7] |= (1 << 24); // Keep Thumb mode enabled

    conversion_abort();

    //if (run_status == RunStatus::Converting) {
    //    stack[6] = stack[5];   // Escape from elf_entry code
    //} else /*if (run_status == RunStatus::Recovered)*/ {
        stack[6] = (uint32_t)main_loop & ~1; // Return to safety
    //}

    asm("pop {lr}; bx lr");
}

} // extern "C"

