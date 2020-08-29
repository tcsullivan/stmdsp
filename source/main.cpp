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
static uint8_t elf_exec_store[2048];
static elf::entry_t elf_entry = nullptr;

static volatile bool signal_operate_done = false;

static void signal_operate(adcsample_t *buffer, size_t count);

int main()
{
    halInit();
    chSysInit();

    palSetPadMode(GPIOA, 5,  PAL_MODE_OUTPUT_PUSHPULL); // LED

    adc::init();
    dac::init();
    usbserial::init();

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
                    dac_samples.fill(0);
                    adc::read_start(signal_operate, &adc_samples[0], adc_samples.size());
                    dac::write_start(&dac_samples[0], dac_samples.size());
                    break;
                case 's':
                    while (!signal_operate_done);
                    usbserial::write(dac_samples.data(), dac_samples.size() * sizeof(adcsample_t));
                    break;
                case 'S':
                    dac::write_stop();
                    adc::read_stop();
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
                default:
                    break;
                }
            }
        }

		chThdSleepMilliseconds(1);
	}
}

void quick_freeall();

void signal_operate(adcsample_t *buffer, size_t count)
{
    if (elf_entry) {
        elf_entry(buffer, count);
        quick_freeall();
    }

    auto dac_buffer = &dac_samples[buffer == &adc_samples[0] ? 0 : 1024];
    std::copy(buffer, buffer + count, dac_buffer);
    signal_operate_done = buffer == &adc_samples[1024];
}

// Dynamic memory allocation below

uint8_t quick_malloc_heap[8192];
uint8_t *quick_malloc_next = quick_malloc_heap;

void *quick_malloc(unsigned int size)
{
    if (auto free = std::distance(quick_malloc_next, quick_malloc_heap + 8192); free < 0 || size > static_cast<unsigned int>(free))
        return nullptr;

    auto ptr = quick_malloc_next;
    quick_malloc_next += size;
    return ptr;
}

void quick_freeall()
{
    if (quick_malloc_next != quick_malloc_heap)
        quick_malloc_next = quick_malloc_heap;
}

void port_syscall(struct port_extctx *ctxp, uint32_t n)
{
    switch (n) {
    case 0:
        *reinterpret_cast<void **>(ctxp->r0) = quick_malloc(ctxp->r1);
        break;
    case 1:
        quick_freeall();
        break;
    }

    chSysHalt("svc");
}

