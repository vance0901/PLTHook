# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.18.1)

# Declares and names the project.

project("xhook")


add_library(
        xhook

        SHARED

        xhook.c
        xh_core.c
        xh_elf.c
        xh_log.c
        xh_util.c
        xh_version.c)



target_link_libraries(
        xhook
        log)