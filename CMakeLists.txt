cmake_minimum_required(VERSION 3.16)

# 按依赖分层，每层一个静态库。下层不链接上层。
#   huff_status → huff_tree ────────→ huff_model ─┐
#               → huff_bitstream ─┬→ huff_codec ──┼→ huffman
#                                 └→ huff_listing ┘

project(Huffman
    VERSION 0.1
    DESCRIPTION "Huffman encoder and decoder"
    LANGUAGES C
)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
    message(FATAL_ERROR
        "请使用独立构建目录:\n"
        "  cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug\n"
        "  cmake --build build")
endif()

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
        Debug Release RelWithDebInfo MinSizeRel)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

if(MSVC)
    add_compile_options(/W4 /utf-8)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(-g -O0)
    endif()
endif()

set(HUFF_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

function(huff_add_layer target)
    add_library(${target} STATIC ${ARGN})
    target_include_directories(${target} PUBLIC ${HUFF_INCLUDE_DIR})
endfunction()

huff_add_layer(huff_status src/status.c)

huff_add_layer(huff_tree src/tree.c)
target_link_libraries(huff_tree PUBLIC huff_status)

huff_add_layer(huff_bitstream src/bitstream.c)
target_link_libraries(huff_bitstream PUBLIC huff_status)

huff_add_layer(huff_model src/model.c)
target_link_libraries(huff_model PUBLIC huff_tree)

huff_add_layer(huff_codec src/codec.c)
target_link_libraries(huff_codec PUBLIC huff_tree huff_bitstream)

huff_add_layer(huff_listing src/listing.c)
target_link_libraries(huff_listing PUBLIC huff_tree huff_bitstream)

add_executable(huffman src/main.c)
target_link_libraries(huffman PRIVATE huff_model huff_codec huff_listing)

enable_testing()
