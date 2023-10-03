//
// Created by Administrator on 2023/8/29/029.
//

#ifndef NATIVEDEMO_ELF_UTIL_H
#define NATIVEDEMO_ELF_UTIL_H
#include <link.h>
#include <android/log.h>
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "lance-elf-test", __VA_ARGS__)

#if defined(__LP64__)
#define ELF_R_SYM(info)  ELF64_R_SYM(info)
#define ELF_R_TYPE(info) ELF64_R_TYPE(info)
#else
#define ELF_R_SYM(info)  ELF32_R_SYM(info)
#define ELF_R_TYPE(info) ELF32_R_TYPE(info)
#endif

#define PAGE_START(addr) ((addr) & PAGE_MASK)
#define PAGE_END(addr)   (PAGE_START(addr) + PAGE_SIZE)

typedef struct
{
    ElfW(Addr)  base_addr;
    ElfW(Addr)  bias_addr;

    ElfW(Ehdr) *ehdr;
    ElfW(Phdr) *phdr;

    ElfW(Dyn)  *dyn; //.dynamic
    ElfW(Word)  dyn_sz;

    const char *strtab; //.dynstr (string-table)
    ElfW(Sym)  *symtab; //.dynsym (symbol-index to string-table's offset)

    ElfW(Addr)  relplt; //.rel.plt or .rela.plt
    ElfW(Word)  relplt_sz;

    ElfW(Addr)  reldyn; //.rel.dyn or .rela.dyn
    ElfW(Word)  reldyn_sz;

    ElfW(Addr)  relandroid; //android compressed rel or rela
    ElfW(Word)  relandroid_sz;

    //for ELF hash
    uint32_t   *bucket;
    uint32_t    bucket_cnt;
    uint32_t   *chain;
    uint32_t    chain_cnt; //invalid for GNU hash

    //append for GNU hash
    uint32_t    symoffset;
    ElfW(Addr) *bloom;
    uint32_t    bloom_sz;
    uint32_t    bloom_shift;

    int         is_use_rela;
    int         is_use_gnu_hash;
} elf_t;
elf_t* elf_init(const char *name);
 ElfW(Addr) get_so_base(const char *name);
 ElfW(Phdr) *get_first_load_segment(elf_t *elf, ElfW(Word) type, ElfW(Off) offset);
 ElfW(Phdr) *get_segment_by_type(elf_t *self, ElfW(Word) type);
 void init_section_from_dynamic(elf_t *elf);
 int find_symidx_by_name(elf_t *self, const char *symbol, uint32_t *symidx);

#endif //NATIVEDEMO_ELF_UTIL_H
