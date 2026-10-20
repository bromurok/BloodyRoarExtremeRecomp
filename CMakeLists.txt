# Bloody Roar Extreme (original Xbox), statically recompiled

cmake_minimum_required(VERSION 3.25)
project(BloodyRoar LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# the SDK: sdk/ here, two folders up inside the SDK's templates, a checkout beside this repository, else downloaded
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/sdk/cmake/XbeRecomp.cmake)
    set(SDK ${CMAKE_CURRENT_SOURCE_DIR}/sdk)
elseif(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/../XboxXBERecomp-main/XboxXBERecomp-main/cmake/XbeRecomp.cmake)
    set(SDK ${CMAKE_CURRENT_SOURCE_DIR}/../XboxXBERecomp-main/XboxXBERecomp-main)
elseif(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/XbeRecomp.cmake)
    set(SDK ${CMAKE_CURRENT_SOURCE_DIR}/../..)
elseif(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/../XboxXBE-Recomp/cmake/XbeRecomp.cmake)
    set(SDK ${CMAKE_CURRENT_SOURCE_DIR}/../XboxXBE-Recomp)
else()
    include(FetchContent)
    FetchContent_Declare(xbe_sdk GIT_REPOSITORY https://github.com/TGP482/XboxXBE-Recomp.git GIT_TAG main)
    FetchContent_MakeAvailable(xbe_sdk)
    set(SDK ${xbe_sdk_SOURCE_DIR})
endif()
include(${SDK}/cmake/XbeRecomp.cmake)

# Replace default SDK sources with our optimized implementations
list(FILTER XBE_RUNTIME_SOURCES EXCLUDE REGEX "/(xdk/dsound|audio/xaudio2|kernel/files|gpu/vulkan|app/settings|app/game_files|kernel/xboxkrnl)\\.cpp$")

file(GLOB XBE_HOST_SOURCES CONFIGURE_DEPENDS src/*/*.cpp)
list(FILTER XBE_HOST_SOURCES EXCLUDE REGEX "/src/game.*")

# partitions as xbe_symbols wrote them into config/game.toml
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS config/game.toml)
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/config/game.toml)
    file(STRINGS config/game.toml GAME_PARTITIONS REGEX "^partitions *=")
    string(REGEX MATCH "[0-9]+" GAME_PARTITIONS "${GAME_PARTITIONS}")
endif()
# no config yet: build.bat writes it with xbe_symbols, the change reconfigures
if(NOT GAME_PARTITIONS)
    set(GAME_PARTITIONS 8)
endif()

# Target USA (default)
add_xbe_image(BloodyRoarExtreme default.xbe game.toml ${GAME_PARTITIONS} game)

# Target Europe (PAL)
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/assets/default_eur.xbe AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/config/game_eur.toml)
    add_xbe_image(BloodyRoarExtreme_EUR default_eur.xbe game_eur.toml 32 game_eur)
endif()

# Target Japan (NTSC-J)
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/assets/default_jpn.xbe AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/config/game_jpn.toml)
    add_xbe_image(BloodyRoarExtreme_JPN default_jpn.xbe game_jpn.toml 32 game_jpn)
endif()

if(MINGW OR (CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND NOT MSVC))
    target_link_options(BloodyRoarExtreme PRIVATE -static -static-libgcc)
    if(TARGET BloodyRoarExtreme_EUR)
        target_link_options(BloodyRoarExtreme_EUR PRIVATE -static -static-libgcc)
    endif()
    if(TARGET BloodyRoarExtreme_JPN)
        target_link_options(BloodyRoarExtreme_JPN PRIVATE -static -static-libgcc)
    endif()
endif()
