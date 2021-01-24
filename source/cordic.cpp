#include "cordic.hpp"
#include "hal.h"

namespace math {

void init()
{
    RCC->AHB2ENR |= RCC_AHB2ENR_CORDICEN;
}

static void prepare() {
    while (CORDIC->CSR & CORDIC_CSR_RRDY)
        asm("mov r0, %0" :: "r" (CORDIC->RDATA));
}
static uint32_t dtoq(double in) {
    double res = in * 0x7FFFFFFF;
    int32_t resi = res;
    return resi;
}
static double qtod(uint32_t in) {
    int32_t ini = in;
    double res = ini;
    return res / 0x7FFFFFFF;
}

__attribute__((naked))
double mod(double n, double) {
    asm("vdiv.f64   d2, d0, d1;"
        "vrintz.f64 d2;"
        "vmul.f64   d1, d1, d2;"
        "vsub.f64   d0, d0, d1;"
        "bx lr");
    return n;
}

double cos(double x) {
    x = (mod(x, 2 * math::PI) - math::PI) / math::PI;
    prepare();
    CORDIC->CSR = CORDIC_CSR_NARGS | CORDIC_CSR_NRES |
                  (6 << CORDIC_CSR_PRECISION_Pos) |
                  (0 << CORDIC_CSR_FUNC_Pos);

    auto in = dtoq(x);
    CORDIC->WDATA = in;
    CORDIC->WDATA = in & 0x7FFFFFFF;
    while (!(CORDIC->CSR & CORDIC_CSR_RRDY));

    double cosx = qtod(CORDIC->RDATA) / x;
    in = CORDIC->RDATA;
    return cosx;
}

double sin(double x) {
    x = (mod(x, 2 * math::PI) - math::PI) / math::PI;
    prepare();
    CORDIC->CSR = CORDIC_CSR_NARGS | CORDIC_CSR_NRES |
                  (6 << CORDIC_CSR_PRECISION_Pos) |
                  (1 << CORDIC_CSR_FUNC_Pos);

    auto in = dtoq(x);
    CORDIC->WDATA = in;
    CORDIC->WDATA = in & 0x7FFFFFFF;
    while (!(CORDIC->CSR & CORDIC_CSR_RRDY));

    double sinx = qtod(CORDIC->RDATA) / x;
    in = CORDIC->RDATA;
    return sinx;
}

double tan(double x) {
    x = (mod(x, 2 * math::PI) - math::PI) / math::PI;
    prepare();
    CORDIC->CSR = CORDIC_CSR_NARGS | CORDIC_CSR_NRES |
                  (6 << CORDIC_CSR_PRECISION_Pos) |
                  (1 << CORDIC_CSR_FUNC_Pos);

    auto in = dtoq(x);
    CORDIC->WDATA = in;
    CORDIC->WDATA = in & 0x7FFFFFFF;
    while (!(CORDIC->CSR & CORDIC_CSR_RRDY));

    double sinx = qtod(CORDIC->RDATA) / x;
    double tanx = sinx * x / qtod(CORDIC->RDATA);
    return tanx;
}

__attribute__((naked))
double sqrt(double x) {
    asm("vsqrt.f64 d0, d0; bx lr");
    return x;
}

}

