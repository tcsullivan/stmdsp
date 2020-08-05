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

void dac_init();
void dac_write_start(dacsample_t *buffer, size_t count);
void dac_write_stop();

#endif // STMDSP_DAC_HPP_

