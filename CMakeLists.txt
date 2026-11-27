cmake_minimum_required(VERSION 3.22)

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)


# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Set the project name
set(CMAKE_PROJECT_NAME canpute)

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable CMake support for ASM and C languages
enable_language(C ASM)

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
)

# ev27-can (CAN database, generated pack/unpack code, ev27_node). Built from a
# checkout next to this repo for now; switch to a lib/ev27-can submodule later:
#   cmake --preset Release -DEV27_CAN_DIR=<path>
set(EV27_CAN_DIR "${CMAKE_SOURCE_DIR}/../ev27-can" CACHE PATH "ev27-can checkout")
if(NOT EXISTS "${EV27_CAN_DIR}/generated/c/ev27_main.c")
    message(FATAL_ERROR "ev27-can not found at ${EV27_CAN_DIR}; set -DEV27_CAN_DIR=<path to ev27-can>")
endif()

# Git hash reported in ev27 Status (first 8 hex digits; 0 if git isn't available)
execute_process(
    COMMAND git rev-parse --short=8 HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE CANPUTE_GIT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET)
if(NOT CANPUTE_GIT_HASH MATCHES "^[0-9a-f]+$")
    set(CANPUTE_GIT_HASH "0")
endif()

# Add sources to executable
file(GLOB_RECURSE APP_SOURCES CONFIGURE_DEPENDS "App/*.c")
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user sources here
    ${APP_SOURCES}
    ${EV27_CAN_DIR}/generated/c/ev27_main.c
    ${EV27_CAN_DIR}/node/ev27_node.c
)

# Add include paths
file(GLOB_RECURSE APP_HEADERS CONFIGURE_DEPENDS "App/*.h")
set(APP_INCLUDE_DIRS "")
foreach(HEADER ${APP_HEADERS})
    get_filename_component(DIR ${HEADER} DIRECTORY)
    list(APPEND APP_INCLUDE_DIRS ${DIR})
endforeach()
list(REMOVE_DUPLICATES APP_INCLUDE_DIRS)
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    ${APP_INCLUDE_DIRS}
    ${EV27_CAN_DIR}/generated/c
    ${EV27_CAN_DIR}/node
)

# Add project symbols (macros)
if(BOOTLOADER)
    set(VECT_OFFSET "0x8000U")
else()
    set(VECT_OFFSET "0x0U")
endif()

target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    VECT_TAB_OFFSET=${VECT_OFFSET}
    EV27_GIT_HASH=0x${CANPUTE_GIT_HASH}u
)

# SystemInit() (Core/Src/system_stm32h5xx.c) sets SCB->VTOR from VECT_TAB_OFFSET, and
# it is compiled in the STM32_Drivers object library, not the executable. Without
# this it puts VTOR back to 0x08000000 and the first interrupt (the TIM6 HAL tick)
# lands in the bootloader's default handler: the app hangs right after the jump.
target_compile_definitions(STM32_Drivers PRIVATE
    VECT_TAB_OFFSET=${VECT_OFFSET}
)

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx

    # Add user defined libraries
)

add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary ${CMAKE_PROJECT_NAME}.elf ${CMAKE_PROJECT_NAME}
)

add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_SIZE} --format=berkeley ${CMAKE_PROJECT_NAME}.elf
    COMMENT "Memory Usage Report:"
)