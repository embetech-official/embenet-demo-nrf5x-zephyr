cmake_minimum_required(VERSION 3.20.0)

# This sample lives inside the module it demonstrates, so it points Zephyr at it itself and builds
# from any workspace. An application outside this repository adds the module through its own west
# manifest or -DZEPHYR_EXTRA_MODULES instead - see README.md.
cmake_path(SET EMBENET_MODULE_DIR NORMALIZE "${CMAKE_CURRENT_LIST_DIR}/../..")
list(APPEND ZEPHYR_EXTRA_MODULES ${EMBENET_MODULE_DIR})

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(embenet_aes128_test)

target_sources(app PRIVATE src/main.c)
