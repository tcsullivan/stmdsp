#include "sclock.hpp"

GPTDriver *SClock::m_timer = &GPTD6;
unsigned int SClock::m_div = 1;
unsigned int SClock::m_runcount = 0;

const GPTConfig SClock::m_timer_config = {
    .frequency = 4800000,
    .callback = nullptr,
    .cr2 = TIM_CR2_MMS_1, /* TRGO */
    .dier = 0
};

const std::array<unsigned int, 6> SClock::m_rate_divs = {{
    /* 8k  */ 600,
    /* 16k */ 300,
    /* 20k */ 240,
    /* 32k */ 150,
    /* 48k */ 100,
    /* 96k */ 50
}};

void SClock::begin()
{
    gptStart(m_timer, &m_timer_config);
}

void SClock::start()
{
    if (m_runcount++ == 0)
        gptStartContinuous(m_timer, m_div);
}

void SClock::stop()
{
    if (--m_runcount == 0)
        gptStopTimer(m_timer);
}

void SClock::setRate(SClock::Rate rate)
{
    m_div = m_rate_divs[static_cast<unsigned int>(rate)];
}

unsigned int SClock::getRate()
{
    for (unsigned int i = 0; i < m_rate_divs.size(); ++i) {
        if (m_rate_divs[i] == m_div)
            return i;
    }

    return static_cast<unsigned int>(-1);
}

