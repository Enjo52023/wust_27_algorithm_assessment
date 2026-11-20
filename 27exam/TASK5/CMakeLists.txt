






#由于本人还不会写CMakeLists,本文件全权交由dsh大人编写









# ============================================================================
# TASK5：两部电梯调度
#
# 只编译三个源文件：
#   main.cpp          —— 程序入口（题目给定，不得修改）
#   my_scheduler.cpp  —— 你自己实现的调度器
#   simulator.cpp     —— 模拟器（题目给定，不得修改）
#
# 所有头文件都在本目录，因此包含路径就是当前源码目录。
#
# 用法：
#   cmake -S . -B build
#   cmake --build build -j
#   ./build/task5
# ============================================================================

cmake_minimum_required(VERSION 3.16)

project(task5 LANGUAGES CXX)

# ---- C++ 标准 ------------------------------------------------------------
# 代码只用到 C++17 特性；关闭 GCC 扩展，保证可移植性。
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---- 默认构建类型 --------------------------------------------------------
# 不加 -DCMAKE_BUILD_TYPE 时，CMake 默认是“无优化”，
# 一天 3 千多名乘客的模拟会慢一些，所以默认给 RelWithDebInfo（-O2 -g）。
get_property(is_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(NOT is_multi_config AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "构建类型" FORCE)
endif()

# ---- 可执行文件 ----------------------------------------------------------
add_executable(task5
    main.cpp
    my_scheduler.cpp
    simulator.cpp
)

# 头文件都在本目录，显式写出来比依赖默认行为更稳妥
target_include_directories(task5 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# ---- 警告选项 ------------------------------------------------------------
# 只对 GCC / Clang 生效，避免在 MSVC 上报“未知选项”。
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(task5 PRIVATE
        -Wall
        -Wextra
        -Wpedantic
    )
endif()
