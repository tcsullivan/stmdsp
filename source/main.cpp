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

#include "adc.hpp"
#include "cordic.hpp"
#include "dac.hpp"
#include "elf_load.hpp"
#include "error.hpp"
#include "samplebuffer.hpp"
#include "sclock.hpp"
#include "usbserial.hpp"

#include <array>

constexpr unsigned int MAX_ELF_FILE_SIZE = 16 * 1024;

enum class RunStatus : char
{
    Idle = '1',
    Running,
    Recovering
};
static RunStatus run_status = RunStatus::Idle;

#define MSG_CONVFIRST          (1)
#define MSG_CONVSECOND         (2)
#define MSG_CONVFIRST_MEASURE  (3)
#define MSG_CONVSECOND_MEASURE (4)

#define MSG_FOR_FIRST(m)   (m & 1)
#define MSG_FOR_MEASURE(m) (m > 2)

static ErrorManager EM;

static msg_t conversionMBBuffer[2];
static MAILBOX_DECL(conversionMB, conversionMBBuffer, 2);

// Thread for LED status and wakeup hold
#if defined(TARGET_PLATFORM_H7)
__attribute__((section(".stacks")))
static THD_WORKING_AREA(monitorThreadWA, 1024);
static THD_FUNCTION(monitorThread, arg);
#endif

// Thread for managing the conversion task
__attribute__((section(".stacks")))
static THD_WORKING_AREA(conversionThreadMonitorWA, 1024);
static THD_FUNCTION(conversionThreadMonitor, arg);
static thread_t *conversionThreadHandle = nullptr;

// Thread for unprivileged algorithm execution
__attribute__((section(".stacks")))
static THD_WORKING_AREA(conversionThreadWA, 128); // All we do is enter unprivileged mode.
static THD_FUNCTION(conversionThread, arg);
constexpr unsigned int conversionThreadUPWASize = 
#if defined(TARGET_PLATFORM_H7)
                                                  62 * 1024;
#else
                                                  15 * 1024;
#endif
__attribute__((section(".convdata")))
static THD_WORKING_AREA(conversionThreadUPWA, conversionThreadUPWASize);
__attribute__((section(".convdata")))
static thread_t *conversionThreadMonitorHandle = nullptr;

// Thread for USB monitoring
__attribute__((section(".stacks")))
static THD_WORKING_AREA(communicationThreadWA, 4096);
static THD_FUNCTION(communicationThread, arg);

static time_measurement_t conversion_time_measurement;
#if defined(TARGET_PLATFORM_H7)
__attribute__((section(".convdata")))
static SampleBuffer samplesIn  (reinterpret_cast<Sample *>(0x38000000)); // 16k
__attribute__((section(".convdata")))
static SampleBuffer samplesOut (reinterpret_cast<Sample *>(0x30004000)); // 16k
static SampleBuffer samplesSigGen (reinterpret_cast<Sample *>(0x30000000)); // 16k
#else
__attribute__((section(".convdata")))
static SampleBuffer samplesIn  (reinterpret_cast<Sample *>(0x20008000)); // 16k
__attribute__((section(".convdata")))
static SampleBuffer samplesOut (reinterpret_cast<Sample *>(0x2000C000)); // 16k
static SampleBuffer samplesSigGen (reinterpret_cast<Sample *>(0x20010000)); // 16k
#endif

static unsigned char elf_file_store[MAX_ELF_FILE_SIZE];
__attribute__((section(".convdata")))
static ELF::Entry elf_entry = nullptr;

__attribute__((section(".convcode")))
static void conversion_unprivileged_main();

static void mpu_setup();
static void conversion_abort();
static void signal_operate(adcsample_t *buffer, size_t count);
static void signal_operate_measure(adcsample_t *buffer, size_t count);

int main()
{
    // Initialize the RTOS
    halInit();
    chSysInit();

    SCB->CPACR |= 0xF << 20; // Enable FPU
    mpu_setup();

    palSetLineMode(LINE_BUTTON, PAL_MODE_INPUT);
    ADC::begin();
    DAC::begin();
    SClock::begin();
    USBSerial::begin();
    cordic::init();

    SClock::setRate(SClock::Rate::R32K);
    ADC::setRate(SClock::Rate::R32K);

    chTMObjectInit(&conversion_time_measurement);
#if defined(TARGET_PLATFORM_H7)
    chThdCreateStatic(
        monitorThreadWA, sizeof(monitorThreadWA),
        LOWPRIO,
        monitorThread, nullptr);
#endif
    conversionThreadMonitorHandle = chThdCreateStatic(
        conversionThreadMonitorWA, sizeof(conversionThreadMonitorWA),
        NORMALPRIO + 1,
        conversionThreadMonitor, nullptr);
    conversionThreadHandle = chThdCreateStatic(
        conversionThreadWA, sizeof(conversionThreadWA),
        HIGHPRIO,
        conversionThread,
        reinterpret_cast<void *>(reinterpret_cast<uint32_t>(conversionThreadUPWA) +
                                 conversionThreadUPWASize));
    chThdCreateStatic(
        communicationThreadWA, sizeof(communicationThreadWA),
        NORMALPRIO,
        communicationThread, nullptr);

    chThdExit(0);
    return 0;
}

THD_FUNCTION(communicationThread, arg)
{
    (void)arg;
	while (1) {
        if (USBSerial::isActive()) {
            // Attempt to receive a command packet
            if (unsigned char cmd[3]; USBSerial::read(&cmd[0], 1) > 0) {
                // Packet received, first byte represents the desired command/action
                switch (cmd[0]) {

                // 'a' - Read contents of ADC buffer.
                // 'A' - Write contents of ADC buffer.
                // 'B' - Set ADC/DAC buffer size.
                // 'd' - Read contents of DAC buffer.
                // 'D' - Set siggen size and write to its buffer.
                // 'E' - Load algorithm binary.
                // 'e' - Unload algorithm.
                // 'i' - Read "stmdsp" identifier string.
                // 'I' - Read status information.
                // 'M' - Begin conversion, measure algorithm execution time.
                // 'm' - Read last algorithm execution time.
                // 'R' - Begin conversion.
                // 'r' - Read or write sample rate.
                // 'S' - Stop conversion.
                // 's' - Get latest block of conversion results.
                // 't' - Get latest block of conversion input.
                // 'W' - Start signal generator (siggen).
                // 'w' - Stop siggen.

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
                        // count is multiplied by two since this command receives size of buffer
                        // for each algorithm application.
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
#if defined(TARGET_PLATFORM_H7)
                    USBSerial::write(reinterpret_cast<const uint8_t *>("stmdsph"), 7);
#else
                    USBSerial::write(reinterpret_cast<const uint8_t *>("stmdspl"), 7);
#endif
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
                case 't':
                    if (auto samps = samplesIn.modified(); samps != nullptr) {
                        unsigned char buf[2] = {
                            static_cast<unsigned char>(samplesIn.size() / 2 & 0xFF),
                            static_cast<unsigned char>(((samplesIn.size() / 2) >> 8) & 0xFF)
                        };
                        USBSerial::write(buf, 2);
                        unsigned int total = samplesIn.bytesize() / 2;
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

THD_FUNCTION(conversionThreadMonitor, arg)
{
    (void)arg;
    while (1) {
        // Recover from algorithm fault if necessary
        //if (run_status == RunStatus::Recovering)
        //    conversion_abort();

        msg_t message;
        if (chMBFetchTimeout(&conversionMB, &message, TIME_INFINITE) == MSG_OK)
            chMsgSend(conversionThreadHandle, message);
    }
}

THD_FUNCTION(conversionThread, stack)
{
    elf_entry = nullptr;
    port_unprivileged_jump(reinterpret_cast<uint32_t>(conversion_unprivileged_main),
                           reinterpret_cast<uint32_t>(stack));
}

#if defined(TARGET_PLATFORM_H7)
THD_FUNCTION(monitorThread, arg)
{
    (void)arg;

    palSetLineMode(LINE_BUTTON, PAL_MODE_INPUT_PULLUP);

    while (1) {
        bool isidle = run_status == RunStatus::Idle;
        auto led = isidle ? LINE_LED_GREEN : LINE_LED_YELLOW;
        auto delay = isidle ? 500 : 250;

        palSetLine(led);
        chThdSleepMilliseconds(delay);
        palClearLine(led);
        chThdSleepMilliseconds(delay);

        if (run_status == RunStatus::Idle && palReadLine(LINE_BUTTON)) {
            palSetLine(LINE_LED_RED);
            palSetLine(LINE_LED_YELLOW);
            chSysLock();
            while (palReadLine(LINE_BUTTON))
                asm("nop");
            while (!palReadLine(LINE_BUTTON))
                asm("nop");
            chSysUnlock();
            palClearLine(LINE_LED_RED);
            palClearLine(LINE_LED_YELLOW);
            chThdSleepMilliseconds(500);
        }

        static bool erroron = false;
        if (auto err = EM.hasError(); err ^ erroron) {
            erroron = err;
            if (err)
                palSetLine(LINE_LED_RED);
            else
                palClearLine(LINE_LED_RED);
        }
    }
}
#endif

void conversion_unprivileged_main()
{
    while (1) {
        msg_t message;
        asm("svc 0; mov %0, r0" : "=r" (message)); // sleep until next message
        if (message != 0) {
            auto samples = MSG_FOR_FIRST(message) ? samplesIn.data() : samplesIn.middata();
            auto size = samplesIn.size() / 2;

            if (elf_entry) {
                if (!MSG_FOR_MEASURE(message)) {
                    samples = elf_entry(samples, size);
                } else {
                    asm("eor r0, r0; svc 2"); // start measurement
                    samples = elf_entry(samples, size);
                    asm("mov r0, #1; svc 2"); // stop measurement
                } 
            }

            if (MSG_FOR_FIRST(message))
                samplesOut.modify(samples, size); 
            else
                samplesOut.midmodify(samples, size); 
        }
    }
}

void mpu_setup()
{
    // Set up MPU for user algorithm
#if defined(TARGET_PLATFORM_H7)
    // Region 2: Data for algorithm thread
    // Region 3: Code for algorithm thread
    // Region 4: User algorithm code
    mpuConfigureRegion(MPU_REGION_2,
                       0x20000000,
                       MPU_RASR_ATTR_AP_RW_RW | MPU_RASR_ATTR_NON_CACHEABLE |
                       MPU_RASR_SIZE_64K |
                       MPU_RASR_ENABLE);
    mpuConfigureRegion(MPU_REGION_3,
                       0x0807F800,
                       MPU_RASR_ATTR_AP_RO_RO | MPU_RASR_ATTR_NON_CACHEABLE |
                       MPU_RASR_SIZE_2K |
                       MPU_RASR_ENABLE);
    mpuConfigureRegion(MPU_REGION_4,
                       0x00000000,
                       MPU_RASR_ATTR_AP_RW_RW | MPU_RASR_ATTR_NON_CACHEABLE |
                       MPU_RASR_SIZE_64K |
                       MPU_RASR_ENABLE);
#else
    // Region 2: Data for algorithm thread and ADC/DAC buffers
    // Region 3: Code for algorithm thread
    // Region 4: User algorithm code
    mpuConfigureRegion(MPU_REGION_2,
                       0x20008000,
                       MPU_RASR_ATTR_AP_RW_RW | MPU_RASR_ATTR_NON_CACHEABLE |
                       MPU_RASR_SIZE_128K|
                       MPU_RASR_ENABLE);
    mpuConfigureRegion(MPU_REGION_3,
                       0x0807F800,
                       MPU_RASR_ATTR_AP_RO_RO | MPU_RASR_ATTR_NON_CACHEABLE |
                       MPU_RASR_SIZE_2K |
                       MPU_RASR_ENABLE);
    mpuConfigureRegion(MPU_REGION_4,
                       0x10000000,
                       MPU_RASR_ATTR_AP_RW_RW | MPU_RASR_ATTR_NON_CACHEABLE |
                       MPU_RASR_SIZE_32K |
                       MPU_RASR_ENABLE);
#endif
}

void conversion_abort()
{
    elf_entry = nullptr;
    DAC::stop(0);
    ADC::stop();
    EM.add(Error::ConversionAborted);

    chMBReset(&conversionMB);
    run_status = RunStatus::Idle;
}

void signal_operate(adcsample_t *buffer, size_t)
{
    chSysLockFromISR();

    if (chMBGetUsedCountI(&conversionMB) > 1) {
        chSysUnlockFromISR();
        conversion_abort();
    } else {
        if (buffer == samplesIn.data()) {
            samplesIn.setModified();
            chMBPostI(&conversionMB, MSG_CONVFIRST);
        } else {
            samplesIn.setMidmodified();
            chMBPostI(&conversionMB, MSG_CONVSECOND);
        }
        chSysUnlockFromISR();
    }
}

void signal_operate_measure(adcsample_t *buffer, [[maybe_unused]] size_t count)
{
    chSysLockFromISR();
    if (buffer == samplesIn.data()) {
        samplesIn.setModified();
        chMBPostI(&conversionMB, MSG_CONVFIRST_MEASURE);
    } else {
        samplesIn.setMidmodified();
        chMBPostI(&conversionMB, MSG_CONVSECOND_MEASURE);
    }
    chSysUnlockFromISR();

    ADC::setOperation(signal_operate);
}

extern "C" {

__attribute__((naked))
void port_syscall(struct port_extctx *ctxp, uint32_t n)
{
    switch (n) {
    case 0:
        {
            chSysLock();
            chMsgWaitS();
            auto msg = chMsgGet(conversionThreadMonitorHandle);
            chMsgReleaseS(conversionThreadMonitorHandle, MSG_OK);
            chSysUnlock();
            ctxp->r0 = msg;
        }
        break;
    case 1:
        {
            using mathcall = void (*)();
            static mathcall funcs[3] = {
                reinterpret_cast<mathcall>(cordic::sin),
                reinterpret_cast<mathcall>(cordic::cos),
                reinterpret_cast<mathcall>(cordic::tan),
            };
#if defined(PLATFORM_H7)
            asm("vmov.f64 d0, %0, %1" :: "r" (ctxp->r1), "r" (ctxp->r2));
            if (ctxp->r0 < 3) {
                funcs[ctxp->r0]();
                asm("vmov.f64 %0, %1, d0" : "=r" (ctxp->r1), "=r" (ctxp->r2));
            } else {
                asm("eor r0, r0; vmov.f64 d0, r0, r0");
            }
#else
            asm("vmov.f32 s0, %0" :: "r" (ctxp->r1));
            if (ctxp->r0 < 3) {
                funcs[ctxp->r0]();
                asm("vmov.f32 %0, s0" : "=r" (ctxp->r1));
            } else {
                asm("eor r0, r0; vmov.f32 s0, r0");
            }
#endif
        }
        break;
    case 2:
        if (ctxp->r0 == 0) {
            chTMStartMeasurementX(&conversion_time_measurement);
        } else {
            chTMStopMeasurementX(&conversion_time_measurement);
            // Subtract measurement overhead from the result.
            // Running an empty algorithm ("bx lr") takes 196 cycles as of 2/4/21.
            // Only measures algorithm code time (loading args/storing result takes 9 cycles).
            constexpr rtcnt_t measurement_overhead = 196 - 1;
            if (conversion_time_measurement.last > measurement_overhead)
                conversion_time_measurement.last -= measurement_overhead;
        }
        break;
    case 3:
        ctxp->r0 = ADC::readAlt(0);
        break;
    default:
        while (1);
        break;
    }

    asm("svc 0");
    while (1);
}

__attribute__((naked))
void MemManage_Handler()
{
    while (1);
}

__attribute__((naked))
void HardFault_Handler()
{
    // Below not working (yet)
    while (1);

    // 1. Get the stack pointer
    uint32_t *stack;
    uint32_t lr;
	asm("\
		tst lr, #4; \
		ite eq; \
		mrseq %0, msp; \
		mrsne %0, psp; \
        mov %1, lr; \
	" : "=r" (stack), "=r" (lr));

    // 2. Only attempt to recover from failed algorithm code
    if ((lr & 4) == 0 || run_status != RunStatus::Running)
        while (1);

    // 3. Post the failure and unload algorithm
    elf_entry = nullptr;
    EM.add(Error::ConversionAborted);
    run_status = RunStatus::Recovering;

    // 4. Make this exception return to point after algorithm exec.
    stack[6] = stack[5];
    stack[7] |= (1 << 24); // Ensure Thumb mode stays enabled

    asm("mov lr, %0; bx lr" :: "r" (lr));
}

} // extern "C"

