# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0-a4)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico2 CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(pico-sgp30 C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# SGP30 library (from lib/)
add_library(sgp30_lib STATIC lib/sgp30.c)
target_include_directories(sgp30_lib PUBLIC lib)
target_link_libraries(sgp30_lib PUBLIC pico_stdlib hardware_gpio)

# SSD1306 OLED library (from lib/)
add_library(ssd1306_lib STATIC lib/ssd1306.c)
target_include_directories(ssd1306_lib PUBLIC lib)
target_link_libraries(ssd1306_lib PUBLIC pico_stdlib hardware_i2c)

# Add executable. Default name is the project name, version 0.1
add_executable(pico-sgp30 pico-sgp30.c)

pico_set_program_name(pico-sgp30 "pico-sgp30")
pico_set_program_version(pico-sgp30 "0.1")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(pico-sgp30 0)
pico_enable_stdio_usb(pico-sgp30 1)

# Add the standard library to the build
target_link_libraries(pico-sgp30
        pico_stdlib
        sgp30_lib
        ssd1306_lib)

# Add the standard include files to the build
target_include_directories(pico-sgp30 PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/lib
)

pico_add_extra_outputs(pico-sgp30)
