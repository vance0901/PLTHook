#include <jni.h>

//
// Created by Administrator on 2023/9/4/004.
//
#include <jni.h>
#include "xhook.h"
#include <string>
#include <android/log.h>
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "lance-memory-hook", __VA_ARGS__)
#include <stdio.h>
#include <unwind.h>
#include <dlfcn.h>
#include <map>

using namespace std;

#define Addr void*
#define DEBUG 1

using namespace std;

#define Addr void*

map<string, size_t> soMallocMap;
map<string, size_t> soFreeMap;
map<void *, size_t> objMap;

struct backtrace_stack {
    Addr* current;
    Addr* end;
};

static _Unwind_Reason_Code unwind_callback(struct _Unwind_Context *context, void *data) {
    backtrace_stack* stack = (backtrace_stack*)data;
    // 记录调用堆栈的地址=程序计数器（PC）
    uintptr_t pc = _Unwind_GetIP(context);
    if (pc) {
        if (stack->current == stack->end) {
            return _URC_END_OF_STACK;
        } else {
            *(stack->current++) = (void *) (pc);
        }
    }
    return _URC_NO_REASON;
}

static void on_alloc_memory(Addr caller,  size_t byte_count){
    /**
     * dli_fname; 包含符号的库文件路径
     * dli_fbase;  符号所在库文件的加载基地址
     *  dli_sname;  符号名称
     *  dli_saddr;  符号的起始地址
     * */
    Dl_info callerInfo = {};
    //根据内存地址获得 库、符号名等信息
    if (dladdr(caller, &callerInfo)) {
        // 记录 动态库 申请的内存总大小
        auto key = callerInfo.dli_fname;
        auto mallocMemory = soMallocMap.find(key);
        if (mallocMemory != soMallocMap.end()) {
            soMallocMap[key] = byte_count + mallocMemory->second;
        } else {
            soMallocMap[key] = byte_count;
        }
    }
#if DEBUG
    //  获得调用堆栈，5的长度 (删除本方法)
    int maxStackSize = 6;
    Addr buffer[maxStackSize];
    // 将堆栈地址记录在： *stack.current => *stack.end
    backtrace_stack stack = {buffer, buffer + maxStackSize};
    _Unwind_Backtrace(unwind_callback, &stack);
    size_t count = stack.current - buffer;
    LOGE("=====================Memory Alloc==============================");
    for (int i = 1; i < count; ++i) {
        Addr addr = buffer[i];
        Dl_info info = {};
        if (dladdr(addr, &info)) {
            uintptr_t addr_relative =(uintptr_t) addr - (uintptr_t)info.dli_saddr;
            LOGE("#%d : %p : %s(%s+%p)", i, addr, info.dli_fname, info.dli_sname,addr_relative);
        }
    }
    LOGE("===============================================================");
#endif
}

static void on_free_memory(Addr caller,size_t byte_count){
    Dl_info callerInfo = {};
    //根据内存地址获得 库、符号名等信息
    if (dladdr(caller, &callerInfo)) {
        // 记录 动态库 释放的内存总大小
        auto key = callerInfo.dli_fname;
        auto freeMemory = soFreeMap.find(key);
        if (freeMemory != soFreeMap.end()) {
            soFreeMap[key] = byte_count + freeMemory->second;
        } else {
            soFreeMap[key] = byte_count;
        }
    }
}

void* hook_malloc(size_t size){
    void* result = malloc(size);

    objMap[result] = size;
    //内置函数，获得谁调用该方法
    Addr caller = __builtin_return_address(0);
    on_alloc_memory(caller,  size);
    return result;
}

void hook_free(void* __ptr){
    free(__ptr);

    auto obj = objMap.find(__ptr);
    Addr caller = __builtin_return_address(0);
    on_free_memory(caller,obj->second);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_enjoy_plthook_PLTHook_hookMemory(JNIEnv *env, jclass clazz) {
    xhook_register("^/data/.*.so$","malloc",(void*)hook_malloc,NULL);
    xhook_register("^/data/.*.so$","free",(void*)hook_free,NULL);
    xhook_ignore(".*/libmemory_hook.so$",NULL);
    xhook_ignore(".*/libxhook.so$",NULL);
    xhook_refresh(0);

}


extern "C"
JNIEXPORT void JNICALL
Java_com_enjoy_plthook_PLTHook_dumpMemory(JNIEnv *env, jclass clazz) {
    for (auto iter = soMallocMap.begin(); iter != soMallocMap.end(); ++iter) {
        auto freeSize = soFreeMap[iter->first];
        LOGE("So %s allocated %u bytes, freed %u bytes", iter->first.c_str(), iter->second,
             freeSize);
    }
}