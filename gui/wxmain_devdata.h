static const std::array<wxString, 6> srateValues {
    "8 kS/s",
    "16 kS/s",
    "20 kS/s",
    "32 kS/s",
    "48 kS/s",
    "96 kS/s"
};
static const std::array<unsigned int, 6> srateNums {
    8000,
    16000,
    20000,
    32000,
    48000,
    96000
};

static const char *makefile_text_h7 = R"make(
all:
	@arm-none-eabi-g++ -x c++ -Os -fno-exceptions -fno-rtti \
                       -mcpu=cortex-m7 -mthumb -mfloat-abi=hard -mfpu=fpv5-d16 -mtune=cortex-m7 \
	                   -nostartfiles \
	                   -Wl,-Ttext-segment=0x00000000 -Wl,-zmax-page-size=512 -Wl,-eprocess_data_entry \
	                   $0 -o $0.o
	@cp $0.o $0.orig.o
	@arm-none-eabi-strip -s -S --strip-unneeded $0.o
	@arm-none-eabi-objcopy --remove-section .ARM.attributes \
                           --remove-section .comment \
                           --remove-section .noinit \
                           $0.o
	arm-none-eabi-size $0.o
)make";
static const char *makefile_text_l4 = R"make(
all:
	@arm-none-eabi-g++ -x c++ -Os -fno-exceptions -fno-rtti \
	                   -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -mtune=cortex-m4 \
	                   -nostartfiles \
	                   -Wl,-Ttext-segment=0x10000000 -Wl,-zmax-page-size=512 -Wl,-eprocess_data_entry \
	                   $0 -o $0.o
	@cp $0.o $0.orig.o
	@arm-none-eabi-strip -s -S --strip-unneeded $0.o
	@arm-none-eabi-objcopy --remove-section .ARM.attributes \
                           --remove-section .comment \
                           --remove-section .noinit \
                           $0.o
	arm-none-eabi-size $0.o
)make";

static const char *file_header_h7 = R"cpp(
#include <cstdint>

using adcsample_t = uint16_t;
constexpr unsigned int SIZE = $0;
adcsample_t *process_data(adcsample_t *samples, unsigned int size);
extern "C" void process_data_entry()
{
    ((void (*)())process_data)();
}

constexpr double PI = 3.14159265358979323846L;
__attribute__((naked))
auto sin(double x) {
asm("vmov.f64 r1, r2, d0;"
    "eor r0, r0;"
    "svc 1;"
    "vmov.f64 d0, r1, r2;"
    "bx lr");
return 0;
}
__attribute__((naked))
auto cos(double x) {
asm("vmov.f64 r1, r2, d0;"
	"mov r0, #1;"
	"svc 1;"
	"vmov.f64 d0, r1, r2;"
	"bx lr");
return 0;
}
__attribute__((naked))
auto tan(double x) {
asm("vmov.f64 r1, r2, d0;"
	"mov r0, #2;"
	"svc 1;"
	"vmov.f64 d0, r1, r2;"
	"bx lr");
return 0;
}
__attribute__((naked))
auto sqrt(double x) {
asm("vsqrt.f64 d0, d0; bx lr");
return 0;
}

auto readalt() {
adcsample_t s;
asm("svc 3; mov %0, r0" : "=&r"(s));
return s;
}

// End stmdspgui header code

)cpp";
static const char *file_header_l4 = R"cpp(
#include <cstdint>

using adcsample_t = uint16_t;
constexpr unsigned int SIZE = $0;
adcsample_t *process_data(adcsample_t *samples, unsigned int size);
extern "C" void process_data_entry()
{
    ((void (*)())process_data)();
}

constexpr float PI = 3.14159265358979L;
__attribute__((naked))
auto sin(float x) {
asm("vmov.f32 r1, s0;"
    "eor r0, r0;"
    "svc 1;"
    "vmov.f32 s0, r1;"
    "bx lr");
return 0;
}
__attribute__((naked))
auto cos(float x) {
asm("vmov.f32 r1, s0;"
	"mov r0, #1;"
	"svc 1;"
	"vmov.f32 s0, r1;"
	"bx lr");
return 0;
}
__attribute__((naked))
auto tan(float x) {
asm("vmov.f32 r1, s0;"
	"mov r0, #2;"
	"svc 1;"
	"vmov.f32 s0, r1;"
	"bx lr");
return 0;
}
__attribute__((naked))
auto sqrt(float) {
asm("vsqrt.f32 s0, s0; bx lr");
return 0;
}

auto readalt() {
adcsample_t s;
asm("push {r4-r6}; svc 3; mov %0, r0; pop {r4-r6}" : "=&r"(s));
return s;
}

// End stmdspgui header code

)cpp";


static const char *file_content = 
R"cpp(adcsample_t *process_data(adcsample_t *samples, unsigned int size)
{
    return samples;
}
)cpp";

