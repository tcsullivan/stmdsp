#ifndef ELF_LOAD_HPP_
#define ELF_LOAD_HPP_

namespace elf
{
    using entry_t = void (*)();

    entry_t load(void *file_data);
}

#endif // ELF_LOAD_HPP_

