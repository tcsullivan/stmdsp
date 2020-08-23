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

static Elf32_Shdr *find_section(Elf32_Ehdr *ehdr, const char *name);

namespace elf {

entry_t load(void *elf_data, void *elf_load_offset)
{
    auto ehdr = reinterpret_cast<Elf32_Ehdr *>(elf_data);
    if (!std::equal(ehdr->e_ident, ehdr->e_ident + 4, elf_header))
        return nullptr;

    auto phdr = ptr_from_offset<Elf32_Phdr *>(elf_data, ehdr->e_phoff);
    for (Elf32_Half i = 0; i < ehdr->e_phnum; i++) {
        if (phdr->p_type == PT_LOAD) {
            std::memcpy(ptr_from_offset<void *>(elf_load_offset, phdr->p_vaddr),
                        ptr_from_offset<void *>(elf_data, phdr->p_offset),
                        phdr->p_filesz);
            //break;
        }

        phdr = ptr_from_offset<Elf32_Phdr *>(phdr, ehdr->e_phentsize);
    }

    // Zero .bss section
    if (auto bss_section = find_section(ehdr, ".bss"); bss_section) {
        auto bss = ptr_from_offset<uint32_t *>(elf_load_offset, bss_section->sh_addr);
        std::fill(bss, bss + bss_section->sh_size / sizeof(uint32_t), 0);
    }

    // Fix global offset table (GOT) entries
    if (auto got_section = find_section(ehdr, ".got"); got_section) {
        auto got = ptr_from_offset<void **>(elf_load_offset, got_section->sh_addr);
        for (size_t i = 0; i < got_section->sh_size / sizeof(void *); i++)
            got[i] = ptr_from_offset<void *>(got[i], reinterpret_cast<uint32_t>(elf_load_offset));
    }

    //// Run any initial constructors
    //if (auto init_section = find_section(ehdr, ".init_array"); init_section) {
    //    auto init_array = reinterpret_cast<void (**)()>(elf_load_offset + init_section->sh_addr);
    //    std::for_each(init_array, init_array + init_section->sh_size / sizeof(void (*)()),
    //                  [elf_load_offset](auto func) { (func + elf_load_offset)(); });
    //}

    // Find filter code start
    if (auto filter = find_section(ehdr, ".process_data"); filter)
        return ptr_from_offset<entry_t>(elf_load_offset, filter->sh_addr | 1); // OR 1 to enable thumb
    else
        return nullptr;
}

} // namespace elf

Elf32_Shdr *find_section(Elf32_Ehdr *ehdr, const char *name)
{
	auto shdr = ptr_from_offset<Elf32_Shdr *>(ehdr, ehdr->e_shoff);
	auto shdr_str = ptr_from_offset<Elf32_Shdr *>(ehdr,
        ehdr->e_shoff + ehdr->e_shstrndx * ehdr->e_shentsize);

	for (Elf32_Half i = 0; i < ehdr->e_shnum; i++) {
		char *section = ptr_from_offset<char *>(ehdr, shdr_str->sh_offset) + shdr->sh_name;
		if (!strcmp(section, name))
			return shdr;

		shdr = ptr_from_offset<Elf32_Shdr *>(shdr, ehdr->e_shentsize);
	}

	return 0;
}

