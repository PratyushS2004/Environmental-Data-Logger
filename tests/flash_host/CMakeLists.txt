
cmake_minimum_required(VERSION 3.20)

project(flash_host_tests LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_executable(flash_host_tests
    test_flash.c
    mock_hal.c
    ../../App/Src/w25q64.c
)

target_include_directories(flash_host_tests PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/fake_hal"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../App/Inc"
)

target_compile_options(flash_host_tests PRIVATE
    -Wall
    -Wextra
    -Werror
)
