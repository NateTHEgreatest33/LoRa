cmake_minimum_required(VERSION 3.13)
project(loraAPI C ASM)

set(CMAKE_C_STANDARD 11)

pico_sdk_init()

add_library( loraAPI
    LoraAPI.h
    LoraAPI.c
    )

target_link_libraries( loraAPI
  pico_stdlib 
  hardware_spi
  # Using Pico W
  pico_cyw43_arch_none

)

target_include_directories(loraAPI PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")