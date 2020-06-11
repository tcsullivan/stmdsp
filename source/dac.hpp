#ifndef STMDSP_DAC_HPP_
#define STMDSP_DAC_HPP_

#include "hal.h"

class DACd
{
public:
    constexpr explicit DACd(DACDriver& driver, const DACConfig& config) :
        m_driver(&driver), m_config(config) {}

    void init();

    void write(unsigned int channel, uint16_t value);

private:
    DACDriver *m_driver;
    DACConfig m_config;

    void initPins();
};

#endif // STMDSP_DAC_HPP_

