/**
 * @file dac.hpp
 * @brief Wrapper for ChibiOS's DACDriver.
 *
 * Copyright (C) 2020 Clyne Sullivan
 *
 * Distributed under the GNU GPL v3 or later. You should have received a copy of
 * the GNU General Public License along with this program.
 * If not, see <https://www.gnu.org/licenses/>.
 */

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

