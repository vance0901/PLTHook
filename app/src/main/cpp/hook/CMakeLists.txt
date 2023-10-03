# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.18.1)

# Declares and names the project.

project("elf_test")

add_library(
        elf_test
        SHARED
        elf_util.cpp
        elf_hook.cpp
        elf_jni.cpp)



target_link_libraries(
        elf_test
        log)
