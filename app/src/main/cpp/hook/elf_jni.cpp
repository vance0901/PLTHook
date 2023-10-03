//
// Created by Administrator on 2023/8/29/029.
//
#include <jni.h>
#include <string>
#include "elf_hook.h"
#include "elf_util.h"



void *my_malloc(size_t size)
{
    LOGI("哈哈哈，申请内存啦！");
    return malloc(size);
}


extern "C"
JNIEXPORT void JNICALL
Java_com_enjoy_plthook_PLTHook_hookElf(JNIEnv *env, jclass clazz) {
    elf_hook_test("libenjoy_test.so","malloc", (void*)my_malloc,0);
}
void a(){
    void * a = malloc(100);
    malloc(100);

    free(a);

    char * b = new char[100];
    delete b;
}
extern "C"
JNIEXPORT void JNICALL
Java_com_enjoy_plthook_PLTHook_testMemory2(JNIEnv *env, jclass clazz) {
    a();
}