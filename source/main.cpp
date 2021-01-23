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

static_assert(sizeof(adcsample_t) == sizeof(uint16_t));
static_assert(sizeof(dacsample_t) == sizeof(uint16_t));

#include "common.hpp"
#include "error.hpp"

#include "adc.hpp"
#include "dac.hpp"
#include "elf_load.hpp"
#include "sclock.hpp"
#include "usbserial.hpp"

#include <array>

constexpr unsigned int MAX_ELF_FILE_SIZE = 8 * 1024;

enum class RunStatus : char
{
    Idle = '1',
    Running
};
static RunStatus run_status = RunStatus::Idle;

#define MSG_CONVFIRST          (1)
#define MSG_CONVSECOND         (2)
#define MSG_CONVFIRST_MEASURE  (3)
#define MSG_CONVSECOND_MEASURE (4)

#define MSG_FOR_FIRST(m)   (m & 1)
#define MSG_FOR_MEASURE(m) (m > 2)

static msg_t conversionMBBuffer[4];
static MAILBOX_DECL(conversionMB, conversionMBBuffer, 4);

static THD_WORKING_AREA(conversionThreadWA, 2048);
static THD_FUNCTION(conversionThread, arg);

static time_measurement_t conversion_time_measurement;

static ErrorManager EM;

static SampleBuffer samplesIn  (reinterpret_cast<Sample *>(0x38000000)); // 16k
static SampleBuffer samplesOut (reinterpret_cast<Sample *>(0x30004000)); // 16k
static SampleBuffer samplesSigGen (reinterpret_cast<Sample *>(0x30000000)); // 16k

static unsigned char elf_file_store[MAX_ELF_FILE_SIZE];
static ELF::Entry elf_entry = nullptr;

static void signal_operate(adcsample_t *buffer, size_t count);
static void signal_operate_measure(adcsample_t *buffer, size_t count);
static void main_loop();

static THD_WORKING_AREA(waThread1, 128);
static THD_FUNCTION(Thread1, arg);

int main()
{
    // Initialize the RTOS
    halInit();
    chSysInit();

    // Enable FPU
    SCB->CPACR |= 0xF << 20;

    ADC::begin();
    DAC::begin();
    SClock::begin();
    USBSerial::begin();

    SClock::setRate(SClock::Rate::R32K);
    ADC::setRate(SClock::Rate::R32K);

    chTMObjectInit(&conversion_time_measurement);
    chThdCreateStatic(waThread1, sizeof(waThread1), NORMALPRIO, Thread1, nullptr);
    chThdCreateStatic(conversionThreadWA, sizeof(conversionThreadWA),
                      NORMALPRIO, conversionThread, nullptr);

    main_loop();
}

void main_loop()
{

	while (1) {
        if (USBSerial::isActive()) {
            // Attempt to receive a command packet
            if (unsigned char cmd[3]; USBSerial::read(&cmd[0], 1) > 0) {
                // Packet received, first byte represents the desired command/action
                switch (cmd[0]) {

                case 'a':
                    USBSerial::write(samplesIn.bytedata(), samplesIn.bytesize());
                    break;
                case 'A':
                    USBSerial::read(samplesIn.bytedata(), samplesIn.bytesize());
                    break;

                case 'B':
                    if (EM.assert(run_status == RunStatus::Idle, Error::NotIdle) &&
                        EM.assert(USBSerial::read(&cmd[1], 2) == 2, Error::BadParamSize))
                    {
                        unsigned int count = (cmd[1] | (cmd[2] << 8)) * 2;
                        if (EM.assert(count <= MAX_SAMPLE_BUFFER_SIZE, Error::BadParam)) {
                            samplesIn.setSize(count);
                            samplesOut.setSize(count);
                        }
                    }
                    break;

                case 'd':
                    USBSerial::write(samplesOut.bytedata(), samplesOut.bytesize());
                    break;
                case 'D':
                    if (EM.assert(USBSerial::read(&cmd[1], 2) == 2, Error::BadParamSize)) {
                        unsigned int count = cmd[1] | (cmd[2] << 8);
                        if (EM.assert(count <= MAX_SAMPLE_BUFFER_SIZE, Error::BadParam)) {
                            samplesSigGen.setSize(count);
                            USBSerial::read(samplesSigGen.bytedata(), samplesSigGen.bytesize());
                        }
                    }
                    break;

                // 'E' - Reads in and loads the compiled conversion code binary from USB.
                case 'E':
                    if (EM.assert(run_status == RunStatus::Idle, Error::NotIdle) &&
                        EM.assert(USBSerial::read(&cmd[1], 2) == 2, Error::BadParamSize))
                    {
                        // Only load the binary if it can fit in the memory reserved for it.
                        unsigned int size = cmd[1] | (cmd[2] << 8);
                        if (EM.assert(size < sizeof(elf_file_store), Error::BadUserCodeSize)) {
                            USBSerial::read(elf_file_store, size);
                            elf_entry = ELF::load(elf_file_store);

                            EM.assert(elf_entry != nullptr, Error::BadUserCodeLoad);
                        }
                    }
                    break;

                // 'e' - Unloads the currently loaded conversion code
                case 'e':
                    elf_entry = nullptr;
                    break;

                // 'i' - Sends an identifying string to confirm that this is the stmdsp device.
                case 'i':
                    USBSerial::write(reinterpret_cast<const uint8_t *>("stmdsp"), 6);
                    break;

                // 'I' - Sends the current run status.
                case 'I':
                    {
                        unsigned char buf[2] = {
                            static_cast<unsigned char>(run_status),
                            static_cast<unsigned char>(EM.pop())
                        };
                        USBSerial::write(buf, sizeof(buf));
                    }
                    break;

                // 'M' - Begins continuous sampling, but measures the execution time of the first
                //       sample processing. This duration can be later read through 'm'.
                case 'M':
                    if (EM.assert(run_status == RunStatus::Idle, Error::NotIdle)) {
                        run_status = RunStatus::Running;
                        samplesOut.clear();
                        ADC::start(samplesIn.data(), samplesIn.size(), signal_operate_measure);
                        DAC::start(0, samplesOut.data(), samplesOut.size());
                    }
                    break;

                // 'm' - Returns the last measured sample processing time, presumably in processor
                //       ticks.
                case 'm':
                    USBSerial::write(reinterpret_cast<uint8_t *>(&conversion_time_measurement.last),
                                     sizeof(rtcnt_t));
                    break;

                // 'R' - Begin continuous sampling/conversion of the ADC. Samples will go through
                //       the conversion code, and will be sent out over the DAC.
                case 'R':
                    if (EM.assert(run_status == RunStatus::Idle, Error::NotIdle)) {
                        run_status = RunStatus::Running;
                        samplesOut.clear();
                        ADC::start(samplesIn.data(), samplesIn.size(), signal_operate);
                        DAC::start(0, samplesOut.data(), samplesOut.size());
                    }
                    break;

                case 'r':
                    if (EM.assert(USBSerial::read(&cmd[1], 1) == 1, Error::BadParamSize)) {
                        if (cmd[1] == 0xFF) {
                            unsigned char r = SClock::getRate();
                            USBSerial::write(&r, 1);
                        } else {
                            auto r = static_cast<SClock::Rate>(cmd[1]);
                            SClock::setRate(r);
                            ADC::setRate(r);
                        }
                    }
                    break;

                // 'S' - Stops the continuous sampling/conversion.
                case 'S':
                    if (run_status == RunStatus::Running) {
                        DAC::stop(0);
                        ADC::stop();
                        run_status = RunStatus::Idle;
                    }
                    break;

                case 's':
                    if (auto samps = samplesOut.modified(); samps != nullptr) {
                        unsigned char buf[2] = {
                            static_cast<unsigned char>(samplesOut.size() / 2 & 0xFF),
                            static_cast<unsigned char>(((samplesOut.size() / 2) >> 8) & 0xFF)
                        };
                        USBSerial::write(buf, 2);
                        unsigned int total = samplesOut.bytesize() / 2;
                        unsigned int offset = 0;
                        unsigned char unused;
                        while (total > 512) {
                            USBSerial::write(reinterpret_cast<uint8_t *>(samps) + offset, 512);
                            while (USBSerial::read(&unused, 1) == 0);
                            offset += 512;
                            total -= 512;
                        }
                        USBSerial::write(reinterpret_cast<uint8_t *>(samps) + offset, total);
                        while (USBSerial::read(&unused, 1) == 0);
                    } else {
                        USBSerial::write(reinterpret_cast<const uint8_t *>("\0\0"), 2);
                    }
                    break;

                case 'W':
                    DAC::start(1, samplesSigGen.data(), samplesSigGen.size());
                    break;
                case 'w':
                    DAC::stop(1);
                    break;

                default:
                    break;
                }
            }
        }

		chThdSleepMicroseconds(100);
	}
}

void conversion_abort()
{
    elf_entry = nullptr;
    DAC::stop(0);
    ADC::stop();
    EM.add(Error::ConversionAborted);
}

THD_FUNCTION(conversionThread, arg)
{
    (void)arg;

    while (1) {
        msg_t message;
        if (chMBFetchTimeout(&conversionMB, &message, TIME_INFINITE) == MSG_OK) {
            auto samples = MSG_FOR_FIRST(message) ? samplesIn.data() : samplesIn.middata();
            auto size = samplesIn.size() / 2;

            if (elf_entry) {
                if (!MSG_FOR_MEASURE(message)) {
                    samples = elf_entry(samples, size);
                } else {
                    chTMStartMeasurementX(&conversion_time_measurement);
                    samples = elf_entry(samples, size);
                    chTMStopMeasurementX(&conversion_time_measurement);
                } 
            }

            if (MSG_FOR_FIRST(message))
                samplesOut.modify(samples, size); 
            else
                samplesOut.midmodify(samples, size); 
        }
    }
}

void signal_operate(adcsample_t *buffer, size_t)
{
    chSysLockFromISR();

    if (chMBGetUsedCountI(&conversionMB) > 1) {
        chSysUnlockFromISR();
        conversion_abort();
    } else {
        chMBPostI(&conversionMB, buffer == samplesIn.data() ? MSG_CONVFIRST : MSG_CONVSECOND);
        chSysUnlockFromISR();
    }
}

void signal_operate_measure(adcsample_t *buffer, [[maybe_unused]] size_t count)
{
    chSysLockFromISR();
    chMBPostI(&conversionMB, buffer == samplesIn.data() ? MSG_CONVFIRST_MEASURE : MSG_CONVSECOND_MEASURE);
    chSysUnlockFromISR();

    ADC::setOperation(signal_operate);
}

THD_FUNCTION(Thread1, arg)
{
    (void)arg;

    bool erroron = false;
    while (1) {
        bool isidle = run_status == RunStatus::Idle;
        auto led = isidle ? LINE_LED_GREEN : LINE_LED_YELLOW;
        auto delay = isidle ? 500 : 250;

        palSetLine(led);
        chThdSleepMilliseconds(delay);
        palClearLine(led);
        chThdSleepMilliseconds(delay);

        if (auto err = EM.hasError(); err ^ erroron) {
            erroron = err;
            if (err)
                palSetLine(LINE_LED_RED);
            else
                palClearLine(LINE_LED_RED);
        }
    }
}

extern "C" {

__attribute__((naked))
void HardFault_Handler()
{
    while (1);
//    //asm("push {lr}");
//
//    uint32_t *stack;
//    uint32_t lr;
//	asm("\
//		tst lr, #4; \
//		ite eq; \
//		mrseq %0, msp; \
//		mrsne %0, psp; \
//        mov %1, lr; \
//	" : "=r" (stack), "=r" (lr));
//    //stack++;
//    stack[7] |= (1 << 24); // Keep Thumb mode enabled
//
//    conversion_abort();
//
//    // TODO test lr and decide how to recover
//
//    //if (run_status == RunStatus::Converting) {
//        stack[6] = stack[5];   // Escape from elf_entry code
//    //} else /*if (run_status == RunStatus::Recovered)*/ {
//    //    stack[6] = (uint32_t)main_loop & ~1; // Return to safety
//    //}
//
//    //asm("pop {lr}; bx lr");
//    asm("bx lr");
}

} // extern "C"

