//
// Created by Administrator on 2023/8/29/029.
//

#include "elf_util.h"
#include <string>
#include <inttypes.h>

elf_t* elf_init(const char *name){
    elf_t* elf = (elf_t*)malloc(sizeof(elf_t));
    memset(elf, 0, sizeof(elf_t));
    /**
     * 1、获取so地址
     */
    elf->base_addr = get_so_base(name);
    elf->ehdr = ( ElfW(Ehdr) *) (elf->base_addr);
    elf->phdr = ( ElfW(Phdr) *) (elf->base_addr+elf->ehdr->e_phoff);

    /**
     * 2、查找ELF 第一个LOAD段，计算基地址
     */
    //  确保是ELF文件
#if 0
    if (memcmp( elf->ehdr->e_ident, "\177ELF", 4) != 0) {
        return 0;
    }
#endif
    ElfW(Phdr) *phdr0 = get_first_load_segment(elf, PT_LOAD, 0);
    // 基地址
    elf->bias_addr = elf->base_addr - phdr0->p_vaddr;


    /**
     * 3、处理动态链接段dynamic
     */
    ElfW(Phdr) *dhdr = get_segment_by_type(elf, PT_DYNAMIC);
    elf->dyn          = (ElfW(Dyn) *)(elf->bias_addr + dhdr->p_vaddr);
    elf->dyn_sz       = dhdr->p_memsz;
    // 初始化好各个section
    init_section_from_dynamic(elf);
    return elf;
}

ElfW(Addr) get_so_base(const char *name) {
    FILE *fp = NULL;
    char line[1024] = "\n";
    Elf64_Addr base_addr = 0;
    if (NULL == (fp = fopen("/proc/self/maps", "r"))) return 0;
    while (fgets(line, sizeof(line), fp)) {
        if (NULL != strstr(line, name)){
            if (sscanf(line, "%" PRIxPTR, &base_addr) == 1)
            break;
        }
    }
    fclose(fp);
    return  base_addr;
}



 ElfW(Phdr) *get_first_load_segment(elf_t *elf, ElfW(Word) type, ElfW(Off) offset)
{
    ElfW(Phdr) *phdr;

    for(phdr = elf->phdr; phdr < elf->phdr + elf->ehdr->e_phnum; phdr++)
    {
        if(phdr->p_type == type && phdr->p_offset == offset)
        {
            return phdr;
        }
    }
    return NULL;
}

 ElfW(Phdr) *get_segment_by_type(elf_t *self, ElfW(Word) type)
{
    ElfW(Phdr) *phdr;

    for(phdr = self->phdr; phdr < self->phdr + self->ehdr->e_phnum; phdr++)
    {
        if(phdr->p_type == type)
        {
            return phdr;
        }
    }
    return NULL;
}


 void init_section_from_dynamic(elf_t *elf) {
    ElfW(Dyn) *dyn     = elf->dyn;
    ElfW(Dyn) *dyn_end = elf->dyn + (elf->dyn_sz / sizeof(ElfW(Dyn)));
    uint32_t  *raw; // 用于解析hash表
    // 遍历dynamic
    for(; dyn < dyn_end; dyn++){
        switch(dyn->d_tag){
            case DT_NULL:
                //结束
                dyn = dyn_end;
                break;
            case DT_STRTAB:
            {
                //字符串表
                elf->strtab = (const char *)(elf->bias_addr + dyn->d_un.d_ptr);
                break;
            }
            case DT_SYMTAB:
            {
                //符号表
                elf->symtab = (ElfW(Sym) *)(elf->bias_addr + dyn->d_un.d_ptr);
                break;
            }
            case DT_PLTREL:
                //重定位表格式为 rel 还是 rela?
                elf->is_use_rela = (dyn->d_un.d_val == DT_RELA ? 1 : 0);
                break;
            case DT_JMPREL:
            {
                // plt重定位表  因不知道是rel/rela，所以记录地址
                elf->relplt = (ElfW(Addr))(elf->bias_addr + dyn->d_un.d_ptr);
                break;
            }
            case DT_PLTRELSZ:
                // plt重定位表 内存大小
                elf->relplt_sz = dyn->d_un.d_val;
                break;
            case DT_REL:
            case DT_RELA:
            {
                // 重定位表 除了.rel.plt表以外的引用 如变量，函数指针等
                elf->reldyn = (ElfW(Addr))(elf->bias_addr + dyn->d_un.d_ptr);
                break;
            }
            case DT_RELSZ:
            case DT_RELASZ:
                // 重定位表 大小
                elf->reldyn_sz = dyn->d_un.d_val;
                break;
            case DT_ANDROID_REL:
            case DT_ANDROID_RELA:
            {
                // android 压缩的 rel/rela APS2（Android ELF Packed Sections 2）压缩格式
                elf->relandroid = (ElfW(Addr))(elf->bias_addr + dyn->d_un.d_ptr);
                break;
            }
            case DT_ANDROID_RELSZ:
            case DT_ANDROID_RELASZ:
                elf->relandroid_sz = dyn->d_un.d_val;
                break;
            case DT_HASH:
            {
                // 由于历史原因hash表有两种 （https://github.com/bytedance/bhook/blob/main/doc/overview.zh-CN.md#hash-table%E5%93%88%E5%B8%8C%E8%A1%A8）
                if(1 == elf->is_use_gnu_hash) continue;

                raw = (uint32_t *)(elf->bias_addr + dyn->d_un.d_ptr);
                elf->bucket_cnt  = raw[0];
                elf->chain_cnt   = raw[1];
                elf->bucket      = &raw[2];
                elf->chain       = &(elf->bucket[elf->bucket_cnt]);
                break;
            }
            case DT_GNU_HASH:
            {
                raw = (uint32_t *)(elf->bias_addr + dyn->d_un.d_ptr);
                elf->bucket_cnt  = raw[0];
                elf->symoffset   = raw[1];
                elf->bloom_sz    = raw[2];
                elf->bloom_shift = raw[3];
                elf->bloom       = (ElfW(Addr) *)(&raw[4]);
                elf->bucket      = (uint32_t *)(&(elf->bloom[elf->bloom_sz]));
                elf->chain       = (uint32_t *)(&(elf->bucket[elf->bucket_cnt]));
                elf->is_use_gnu_hash = 1;
                break;
            }
            default:
                break;
        }

    }

    if(elf->relandroid){
        const char *rel = (const char *)elf->relandroid;
        if(elf->relandroid_sz < 4 ||
           rel[0] != 'A' ||
           rel[1] != 'P' ||
           rel[2] != 'S' ||
           rel[3] != '2'){
            elf->relandroid = 0;
            elf->relandroid_sz = 0;
        }else {

            elf->relandroid += 4;
            elf->relandroid_sz -= 4;
        }
    }
}


static uint32_t gnu_hash(uint8_t *symbol) {
    uint32_t h = 5381;

    while(*symbol != 0)
    {
        h += (h << 5) + *symbol++;
    }
    return h;
}

static uint32_t elf_hash(const uint8_t *name){
    uint32_t h = 0, g;

    while (*name) {
        h = (h << 4) + *name++;
        g = h & 0xf0000000;
        h ^= g;
        h ^= g >> 24;
    }

    return h;
}

// http://www.linker-aliens.org/blogs/ali/entry/gnu_hash_elf_sections/  Symbol Lookup Using GNU Hash
// 翻译：https://bbs.kanxue.com/thread-223668.htm
// http://www.aospxref.com/android-12.0.0_r3/xref/bionic/linker/linker_gnu_hash.h
static int find_symidx_by_gnu_hash(elf_t *self, const char *symbol, uint32_t *symidx) {
    /**
     * 1、根据hash找
     */
    //生成符号的32位哈希
    uint32_t hash = gnu_hash((uint8_t *)symbol);
    static uint32_t elfclass_bits = sizeof(ElfW(Addr)) * 8;
    size_t word = self->bloom[(hash / elfclass_bits) % self->bloom_sz];
    size_t mask = 0
                  | (size_t)1 << (hash % elfclass_bits)
                  | (size_t)1 << ((hash >> self->bloom_shift) % elfclass_bits);

    //if at least one bit is not set, this symbol is surely missing
    if((word & mask) != mask) return 0;

    //ignore STN_UNDEF
    uint32_t i = self->bucket[hash % self->bucket_cnt];
    if(i < self->symoffset) return 0;

    //loop through the chain
    while(1)
    {
        const char     *symname = self->strtab + self->symtab[i].st_name;
        const uint32_t  symhash = self->chain[i - self->symoffset];

        if((hash | (uint32_t)1) == (symhash | (uint32_t)1) && 0 == strcmp(symbol, symname))
        {
            *symidx = i;
            return 1;
        }

        //chain ends with an element with the lowest bit set to 1
        if(symhash & (uint32_t)1) break;

        i++;
    }
    return 0;
}

static int find_symidx_by_hash(elf_t *self, const char *symbol, uint32_t *symidx) {
    uint32_t    hash = elf_hash((uint8_t *)symbol);
    const char *symbol_cur;
    uint32_t    i;

    for(i = self->bucket[hash % self->bucket_cnt]; 0 != i; i = self->chain[i])
    {
        symbol_cur = self->strtab + self->symtab[i].st_name;

        if(0 == strcmp(symbol, symbol_cur))
        {
            *symidx = i;
            return 1;
        }
    }

    return 0;
}

 int find_symidx_by_name(elf_t *self, const char *symbol, uint32_t *symidx)
{
    int r = 0;
    if(self->is_use_gnu_hash){
        r = find_symidx_by_gnu_hash(self, symbol, symidx);
        if (r) return r;
        /**
        * GNU hash。只包含动态链接符号中的导出符号。所以对于导入符号需要遍历查找
        */
        for(int i = 0; i < self->symoffset; i++)
        {
            const char *symname = self->strtab + self->symtab[i].st_name;
            if(0 == strcmp(symname, symbol))
            {
                *symidx = i;
                return 1;
            }
        }
    }
    else
        return find_symidx_by_hash(self, symbol, symidx);

    return 0;
}