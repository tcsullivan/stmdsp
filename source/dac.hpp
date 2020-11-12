/**
 * @file dac.hpp
 * @brief Manages signal creation using the DAC.
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
#undef DAC

class DAC
{
public:
    static void begin();

    static void start(int channel, dacsample_t *buffer, size_t count);
    static void stop(int channel);

private:
    static DACDriver *m_driver[2];
    static GPTDriver *m_timer;
    static int m_timer_user_count;

    static const DACConfig m_config;
    static const DACConversionGroup m_group_config;
    static const GPTConfig m_timer_config;
};

#endif // STMDSP_DAC_HPP_

