#ifndef ELF_LOAD_HPP_
#define ELF_LOAD_HPP_

#include <cstddef>
#include <cstdint>

namespace elf
{
    using entry_t = uint16_t *(*)(uint16_t *, size_t);

    entry_t load(void *elf_data);
}

#endif // ELF_LOAD_HPP_

