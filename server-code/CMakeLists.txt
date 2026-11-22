cmake_minimum_required(VERSION 3.13)

project(darkbox_server VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_library(darkbox_core
    src/protocol.c
    src/transport.c
)

target_include_directories(darkbox_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_options(darkbox_core PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

target_compile_definitions(darkbox_core PRIVATE
    _DEFAULT_SOURCE
)

add_executable(darkbox_server
    src/main.c
)

target_link_libraries(darkbox_server PRIVATE darkbox_core)

enable_testing()

add_executable(protocol_tests
    tests/test_protocol.c
)

target_link_libraries(protocol_tests PRIVATE darkbox_core)
add_test(NAME protocol_tests COMMAND protocol_tests)
