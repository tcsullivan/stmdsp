#ifndef SCLOCK_HPP_
#define SCLOCK_HPP_

#include "hal.h"

#include <array>

class SClock
{
public:
    enum class Rate : unsigned int {
        R8K = 0,
        R16K,
        R20K,
        R32K,
        R48K,
        R96K
    };

    static void begin();
    static void start();
    static void stop();

    static void setRate(Rate rate);
    static unsigned int getRate();

private:
    static GPTDriver *m_timer;
    static unsigned int m_div;
    static unsigned int m_runcount;
    static const GPTConfig m_timer_config;
    static const std::array<unsigned int, 6> m_rate_divs;
};

#endif // SCLOCK_HPP_

