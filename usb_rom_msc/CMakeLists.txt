
cmake_minimum_required(VERSION 3.5.0 FATAL_ERROR)

set(CMAKE_FILES ${CMAKE_SOURCE_DIR}/../cmake)
set(CMAKE_TOOLCHAIN_FILE    ${CMAKE_FILES}/toolchain-gcc-arm-embedded.cmake)

project(USB_ROM_MSC)

include(${CMAKE_FILES}/CPM_setup.cmake)


#-----------------------------------------------------------------------
# Build settings
#-----------------------------------------------------------------------

set(EXE_NAME                USB_ROM_MSC)
set(FLASH_ADDR              0x00000000)
set(FLASH_CFG               lpc11uxx)
set(DEBUG_BREAKPOINT_LIMIT  4)
set(DEBUG_WATCHPOINT_LIMIT  2)


# default settings
set(OPTIMIZE s)
set(BLACKMAGIC_DEV /dev/ttyBmpGdb)
set(POWER_TARGET "no")

# Include custom settings
# (if this file does not exist, copy it from config.cmake.example)
include(${CMAKE_SOURCE_DIR}/config.cmake)

message(STATUS "Config OPTIMIZE: ${OPTIMIZE}")
message(STATUS "Config BLACKMAGIC_DEV: ${BLACKMAGIC_DEV}")
message(STATUS "Config POWER_TARGET: ${POWER_TARGET}")

set(SYSTEM_LIBRARIES    m c gcc)

set(FLAGS_M0 "-mcpu=cortex-m0")

set(C_FLAGS "-O${OPTIMIZE} -g3 -c -fmessage-length=80 -fno-builtin   \
    -ffunction-sections -fdata-sections -std=gnu99 -mthumb      \
    -fdiagnostics-color=auto")
set(C_FLAGS_WARN "-Wall -Wextra -Wno-unused-parameter           \
    -Wshadow -Wpointer-arith -Winit-self -Wstrict-overflow=2")

set(L_FLAGS "-fmessage-length=80 -nostdlib -specs=nano.specs \
    -mthumb -Wl,--gc-sections")

set(MCU_PLATFORM    11uxx)

add_definitions("${FLAGS_M0} ${C_FLAGS} ${C_FLAGS_WARN}")
add_definitions(-DCORE_M0 -DCHIP_LPC11UXX -DMCU_PLATFORM_${MCU_PLATFORM})

# lpc_usb_lib settings
add_definitions(-D__LPC11U1X)


set(ELF_PATH            "${CMAKE_CURRENT_BINARY_DIR}/${EXE_NAME}")
set(EXE_PATH            "${ELF_PATH}.bin")
set(FLASH_FILE          ${PROJECT_BINARY_DIR}/flash.cfg)

#------------------------------------------------------------------------------
# CPM Modules
#------------------------------------------------------------------------------

# ---- #
# chip_lpc1xxx: use our own src/usb/app_usbd_cfg.h
set(USBD_CFG_INCLUDE "<${CMAKE_CURRENT_SOURCE_DIR}/src/usb/app_usbd_cfg.h>")
add_definitions("-DCHIP_LPC11UXX_USBD_CONFIG_FILE=${USBD_CFG_INCLUDE}")
# --- #


CPM_AddModule("startup_lpc11xxx"
    GIT_REPOSITORY "https://github.com/JitterCompany/startup_lpc11xxx.git"
    GIT_TAG "1.2")

CPM_AddModule("lpc_tools"
    GIT_REPOSITORY "https://github.com/JitterCompany/lpc_tools.git"
    GIT_TAG "2.8")

CPM_AddModule("chip_lpc11xxx"
    GIT_REPOSITORY "https://github.com/JitterCompany/chip_lpc11xxx.git"
    GIT_TAG "1.4")

CPM_AddModule("mcu_timing"
    GIT_REPOSITORY "https://github.com/JitterCompany/mcu_timing.git"
    GIT_TAG "1.5.7")

CPM_AddModule("c_utils"
    GIT_REPOSITORY "https://github.com/JitterCompany/c_utils.git"
    GIT_TAG "1.4.5")

CPM_AddModule("mcu_debug"
    GIT_REPOSITORY "https://github.com/JitterCompany/mcu_debug.git"
    GIT_TAG "2.1")

CPM_Finish()


get_property(startup_linker GLOBAL PROPERTY startup_linker)
message(STATUS "Startup_linker: ${startup_linker}")

set(LINKER_FILES "-L .. -T ${startup_linker}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${L_FLAGS} \
${LINKER_FILES} ${FLAGS_M0}")


#-----------------------------------------------------------------------
# Setup source
#-----------------------------------------------------------------------

include_directories("src", "src/usb")

file(GLOB SOURCES
"src/*.c",
"src/usb/*.c"
)

set(CMAKE_SYSTEM_NAME Generic)




#-----------------------------------------------------------------------
# Setup executable
#-----------------------------------------------------------------------
add_executable(${EXE_NAME} ${SOURCES})
target_link_libraries(${EXE_NAME} ${CPM_LIBRARIES})
target_link_libraries(${EXE_NAME} ${SYSTEM_LIBRARIES})

add_custom_target(bin
    # empty flash file
    COMMAND > "${FLASH_FILE}"

    DEPENDS ${EXE_NAME}
    COMMAND ${CMAKE_OBJCOPY} -O binary ${EXE_NAME} ${EXE_NAME}.bin

    # append flash file
    COMMAND echo "${PROJECT_BINARY_DIR}/${EXE_NAME}.bin ${FLASH_ADDR} ${FLASH_CFG}" >> "${PROJECT_BINARY_DIR}/flash.cfg"
    )

add_dependencies(flash bin)
add_dependencies(debug bin)
