#include "dac.hpp"

//static const DACConversionGroup dacGroupConfig = {
//  .num_channels = 1,
//  .end_cb       = NULL,
//  .error_cb     = NULL,
//  .trigger      = DAC_TRG(0)
//};

void DACd::init()
{
    initPins();
    dacStart(m_driver, &m_config);
}

void DACd::write(unsigned int channel, uint16_t value)
{
    if (channel < 2)
        dacPutChannelX(m_driver, channel, value);
}

void DACd::initPins()
{
    palSetPadMode(GPIOA, 4, PAL_MODE_INPUT_ANALOG);     // DAC out1, out2
    palSetPadMode(GPIOA, 5, PAL_MODE_INPUT_ANALOG);
}

