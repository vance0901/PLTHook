# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.18.1)

# Declares and names the project.

project("plthook")

add_subdirectory(hook)

add_library(
        plthook
        SHARED
        native-lib.cpp
)

add_library(enjoy_test
        SHARED
        IMPORTED)
set_target_properties(enjoy_test  PROPERTIES  IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/../jniLibs/arm64-v8a/libenjoy_test.so)


#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -fvisibility=hidden")

target_link_libraries(
        plthook
        enjoy_test
        log)

add_subdirectory(xhook)
include_directories(xhook)

add_library(
        memory_hook
        SHARED
        memory_hook_jni.cpp
)
target_link_libraries(
        memory_hook
        xhook
        log)



