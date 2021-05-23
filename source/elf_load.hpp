/**
 * @file elf_load.hpp
 * @brief Loads ELF binary data into memory for execution.
 *
 * Copyright (C) 2021 Clyne Sullivan
 *
 * Distributed under the GNU GPL v3 or later. You should have received a copy of
 * the GNU General Public License along with this program.
 * If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef ELF_LOAD_HPP_
#define ELF_LOAD_HPP_

#include <cstddef>
#include <cstdint>

namespace ELF
{
    using Entry = uint16_t *(*)(uint16_t *, size_t);

    Entry load(void *elf_data);
}

#endif // ELF_LOAD_HPP_

