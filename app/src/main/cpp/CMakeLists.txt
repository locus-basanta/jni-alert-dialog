# CMakeLists.txt — builds the native-alert shared library (.so)
#
# Android Studio's externalNativeBuild block points here.
# The resulting libnative-alert.so is loaded by MainActivity at runtime.

cmake_minimum_required(VERSION 3.22)

project(native-alert LANGUAGES C)

# Build our single C source file into a shared library.
add_library(native-alert SHARED native-alert.c)

# Find Android's built-in log library (for __android_log_print).
find_library(log-lib log)

# Link the log library so we can use LOGI / LOGE macros.
target_link_libraries(native-alert ${log-lib})
