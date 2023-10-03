//
// Created by Administrator on 2023/8/24/024.
//
#include <jni.h>
#include <string>
#include <inttypes.h>
#include <unistd.h>
#include <sys/mman.h>
#include "elf_util.h"

uint32_t
dl_new_hash (const char *s)
{
    uint32_t h = 5381;

    for (unsigned char c = *s; c != '\0'; c = *++s)
        h = h * 33 + c;

    return h;
}
ElfW(Sym)* symhash( elf_t *elf, const char *symname)
{
    /*
     * Hash the name, generate the "second" hash
     * from it for the Bloom filter.
     */
    uint32_t h1 = dl_new_hash(symname);
    uint32_t h2 = h1 >> elf->bloom_shift;
    /* Test against the Bloom filter */
    uint32_t c = sizeof (ElfW(Addr)) * 8;
    uint32_t n = (h1 / c) & (elf->bloom_sz-1);
    size_t bitmask = 0| (size_t)1 << (h1 % c) | (size_t)1 << (h2 % c);
    if ((elf->bloom[n] & bitmask) != bitmask)
        return (NULL);

    /* Locate the hash chain, and corresponding hash value element */
    n = elf->bucket[h1 % elf->bucket_cnt];
    if (n == 0)    /* Empty hash chain, symbol not present */
        return (NULL);
    ElfW(Sym)*  sym = elf->symtab+n;
    uint32_t* hashval = &elf->chain[n - elf->symoffset];


    for (h1 &= ~1; 1; sym++) {
        h2 = *hashval++;

        if ((h1 == (h2 & ~1)) &&
            !strcmp(symname, elf->strtab + sym->st_name))
            return sym;

        /* Done if at end of chain */
        if (h2 & 1)
            break;
    }

    /* This object does not have the desired symbol */
    return (NULL);
}


void elf_hook_test(const char* soName,const char* symbol,void *new_func, void  **old_func){

    elf_t *elf = elf_init(soName);
    /**
     * 4、执行Hook
     */
     //查找符号表下标
    uint32_t symidx;
    find_symidx_by_name(elf, symbol, &symidx);

    // 遍历重定位表
    ElfW(Addr) *relplt = (ElfW(Addr) *)elf->relplt;
    size_t size;
    ElfW(Rel) *  rel=0;
    ElfW(Rela) *  rela=0;
    // 根据类型rel/rela 计算数量及转型
    if (elf->is_use_rela){
        size = elf->relplt_sz / sizeof(ElfW(Rela));
        rela = (ElfW(Rela) *)relplt;
    }else{
        size= elf->relplt_sz / sizeof(ElfW(Rel));
        rel = (ElfW(Rel) *)relplt;
    }
    // 开始遍历
    for (int i = 0; i < size; ++i) {
        ElfW(Addr)     r_offset;
        size_t         r_info;
        if(rela) {
            r_info = rela->r_info;
            r_offset = rela->r_offset;
            rela++;
        } else {
            r_info = rel->r_info;
            r_offset = rel->r_offset;
            rel++;
        }
        size_t r_sym = ELF_R_SYM(r_info);
        // 当前重定位项与查找的符号表 不匹配
        if(r_sym != symidx) continue;
        // todo 还应该判断 ELF_R_TYPE(r_info);

        ElfW(Addr) addr = elf->bias_addr + r_offset;

        // 保存原函数地址
        if(old_func) *old_func = *(void **)addr;
        //调整写权限
        mprotect((void *) PAGE_START(addr), PAGE_SIZE, PROT_READ | PROT_WRITE);
        // 将记录malloc地址的内存位置中的数据改为自己函数 实现hook
        *(void** )addr = (void*)new_func;
        // 清理指令缓存，确保修改生效
        __builtin___clear_cache( (char *)PAGE_START(addr), (char *)PAGE_END(addr));
       return;
    }

    //用一样的逻辑处理：
    /**
     * .rel.dyn
     */

     /**
      *  .rela.android
      */
}


