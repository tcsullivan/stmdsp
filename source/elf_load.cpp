/**
 * @file elf_load.cpp
 * @brief Loads ELF binary data into memory for execution.
 *
 * Copyright (C) 2020 Clyne Sullivan
 *
 * Distributed under the GNU GPL v3 or later. You should have received a copy of
 * the GNU General Public License along with this program.
 * If not, see <https://www.gnu.org/licenses/>.
 */

#include "elf_load.hpp"
#include "elf_format.hpp"

#include <algorithm>
#include <cstring>

static const unsigned char elf_header[] = { '\177', 'E', 'L', 'F' };

template<typename T>
constexpr static auto ptr_from_offset(void *base, uint32_t offset)
{
    return reinterpret_cast<T>(reinterpret_cast<uint8_t *>(base) + offset);
}

namespace elf {

entry_t load(void *elf_data)
{
    // Check the ELF's header signature
    auto ehdr = reinterpret_cast<Elf32_Ehdr *>(elf_data);
    if (!std::equal(ehdr->e_ident, ehdr->e_ident + 4, elf_header))
        return nullptr;

    // Iterate through program header LOAD sections
    bool loaded = false;
    auto phdr = ptr_from_offset<Elf32_Phdr *>(elf_data, ehdr->e_phoff);
    for (Elf32_Half i = 0; i < ehdr->e_phnum; i++) {
        if (phdr->p_type == PT_LOAD) {
            if (phdr->p_filesz == 0) {
                std::memset(reinterpret_cast<void *>(phdr->p_vaddr),
                            0,
                            phdr->p_memsz);
            } else {
                std::memcpy(reinterpret_cast<void *>(phdr->p_vaddr),
                            ptr_from_offset<void *>(elf_data, phdr->p_offset),
                            phdr->p_filesz);
                if (!loaded)
                    loaded = true;
            }
        }

        phdr = ptr_from_offset<Elf32_Phdr *>(phdr, ehdr->e_phentsize);
    }


    return loaded ? reinterpret_cast<entry_t>(ehdr->e_entry) : nullptr;
}

} // namespace elf

