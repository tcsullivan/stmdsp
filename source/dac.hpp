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

namespace dac
{
    void init();
    void write_start(unsigned int channel, dacsample_t *buffer, size_t count);
    void write_stop(unsigned int channel);
}

#endif // STMDSP_DAC_HPP_

