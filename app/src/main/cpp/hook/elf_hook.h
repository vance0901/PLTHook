//
// Created by Administrator on 2023/8/29/029.
//

#ifndef NATIVEDEMO_ELF_HOOK_H
#define NATIVEDEMO_ELF_HOOK_H

void elf_hook_test(const char* soName,const char* symbol,void *new_func, void  **old_func);

#endif //NATIVEDEMO_ELF_HOOK_H
