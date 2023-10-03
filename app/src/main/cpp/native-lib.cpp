#include <jni.h>
#include <string>

#include <android/log.h>
#include <inttypes.h>
#include <unistd.h>
#include <link.h>
#include <sys/mman.h>
#include "test.h"

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "lance-hook-test", __VA_ARGS__)
#define PAGE_START(addr) ((addr) & PAGE_MASK)
#define PAGE_END(addr)   (PAGE_START(addr) + PAGE_SIZE)



void *my_malloc(size_t size)
{
    LOGI("哈哈哈，申请内存啦！");
    return malloc(size);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_enjoy_plthook_PLTHook_testHook1(JNIEnv *env, jclass thiz) {

    test();
    Elf64_Addr addr = 0x1a78;

    // 该内存地址可能没有写入权限，直接对这个地址赋值会引起段错误
    // 修改该内存页为可读可写
    mprotect((void *) PAGE_START(addr), PAGE_SIZE, PROT_READ | PROT_WRITE);

    // 将记录malloc地址的内存位置中的数据改为自己函数 实现hook
    Elf64_Addr* p = (Elf64_Addr *)addr;
    *p = (Elf64_Addr)my_malloc;

    // 编译器内置函数，机器码中插入：清理缓存指令，确保修改生效
    __builtin___clear_cache( (char *)PAGE_START(addr), (char *)PAGE_END(addr));

    test();
}



extern "C"
JNIEXPORT void JNICALL
Java_com_enjoy_plthook_PLTHook_testHook2(JNIEnv *env, jclass thiz) {
    FILE *fp = fopen("/proc/self/maps","r");
    char line[1024];
    // Elf64_Addr,Elf32_Addr
    /**
     * 1、获取so的加载地址：base_addr
     */
    Elf64_Addr base_addr = 0;
    Elf64_Addr bias_addr = 0;
    while (fgets(line,sizeof(line),fp )){
        if (strstr(line,"libenjoy_test.so")){
//            76c3a59000-76c3a5a000 r--p 00000000 fd:06 23306  .../libenjoy_test.so
            if (sscanf(line,"%lx",&base_addr) != -1){
                break;
            }
        }
    }
    fclose(fp);

    /**
     * 2、获取ELF头中的PHT偏移，读取PHT
     */
    Elf64_Ehdr *header = (Elf64_Ehdr*) base_addr;
    Elf64_Phdr *phtable = (Elf64_Phdr *)( base_addr+header->e_phoff);

    /**
     * 3、遍历PHT 查找offset为0的同时类型为PL_LOAD的段，并且读出p_vaddr
     */
    for (int i = 0; i < header->e_phnum; ++i) {
        if (phtable[i].p_offset == 0&& phtable[i].p_type == PT_LOAD){
            /**
             * 4、基地址 = base_addr - p_vaddr
            */
            bias_addr = base_addr - phtable[i].p_vaddr;
            break;
        }

    }



    test();
    Elf64_Addr addr = bias_addr + 0x1a78;

    // 该内存地址可能没有写入权限，直接对这个地址赋值会引起段错误
    // 修改该内存页为可读可写
    mprotect((void *) PAGE_START(addr), PAGE_SIZE, PROT_READ | PROT_WRITE);

    // 将记录malloc地址的内存位置中的数据改为自己函数 实现hook
    Elf64_Addr* p = (Elf64_Addr *)addr;
    *p = (Elf64_Addr)my_malloc;

    // 编译器内置函数，机器码中插入：清理缓存指令，确保修改生效
    __builtin___clear_cache( (char *)PAGE_START(addr), (char *)PAGE_END(addr));

    test();


}

extern "C"
JNIEXPORT void JNICALL
Java_com_enjoy_plthook_PLTHook_testMemory(JNIEnv *env, jclass clazz) {
    void * a = malloc(100);
    malloc(100);

    free(a);

    char * b = new char[100];
    delete b;
}