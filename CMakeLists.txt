cmake_minimum_required(VERSION 3.13)
project(loraAPI CXX ASM)

set(CMAKE_CXX_STANDARD 20)

pico_sdk_init()

add_library( loraAPI
    LoraAPI.hpp
    LoraAPI.cpp
    )

target_link_libraries( loraAPI
  pico_stdlib 
  hardware_spi
  # Using Pico W
  pico_cyw43_arch_none

  consoleAPI
)

target_include_directories(loraAPI PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")