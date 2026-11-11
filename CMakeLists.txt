# TB Suzz for current Macs: CLAP, VST3 and AU (AUv2),
# universal (Apple Silicon + Intel), macOS 11 and later.
#
# The plugin is written once as a CLAP (Source/Plugin) and clap-wrapper builds
# the VST3 and AU from the same code. Dependencies (CLAP SDK, VST3 SDK, Apple
# AudioUnitSDK) are downloaded from GitHub at configure time.
#
#   cmake -B build -DCMAKE_BUILD_TYPE=Release
#   cmake --build build --config Release --target TBSuzz_all
#
# On Linux only the CLAP and the tests are built (for CI and development).

cmake_minimum_required(VERSION 3.21)

set(CMAKE_OSX_DEPLOYMENT_TARGET 11.0 CACHE STRING "Minimum macOS")
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Universal binary")

project(TBSuzz VERSION 1.0.0 LANGUAGES C CXX)
if (APPLE)
    enable_language(OBJCXX)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)
if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release)
endif()

# ---- clap-wrapper (and through it the CLAP, VST3 and AudioUnit SDKs) ----
set(CLAP_WRAPPER_DOWNLOAD_DEPENDENCIES TRUE CACHE BOOL "Download the plugin SDKs")
set(CLAP_WRAPPER_BUILD_TESTS OFF CACHE BOOL "")
include(FetchContent)
FetchContent_Declare(clap-wrapper
    GIT_REPOSITORY https://github.com/free-audio/clap-wrapper.git
    GIT_TAG        v0.17.0
    GIT_SHALLOW    TRUE)
FetchContent_MakeAvailable(clap-wrapper)

# ---- The plugin itself, as a static library ----
set(DSP_SOURCES
    Source/DSP/Suzz.cpp
    Source/DSP/Netlist.cpp)

add_library(tbsz-impl STATIC
    ${DSP_SOURCES}
    Source/Plugin/Presets.cpp
    Source/Plugin/SuzzClap.cpp)
target_include_directories(tbsz-impl PUBLIC Source/Plugin Source/DSP)
target_link_libraries(tbsz-impl PUBLIC clap)

if (APPLE)
    target_sources(tbsz-impl PRIVATE Source/Plugin/PanelView.mm)
    set_source_files_properties(Source/Plugin/PanelView.mm PROPERTIES
        COMPILE_FLAGS "-fno-objc-arc -Werror=unguarded-availability -Werror=unguarded-availability-new")
    target_link_libraries(tbsz-impl PUBLIC "-framework Cocoa")
endif()

if (APPLE)
    set(TBSZ_FORMATS CLAP VST3 AUV2)
else()
    set(TBSZ_FORMATS CLAP)
endif()

make_clapfirst_plugins(
    TARGET_NAME       TBSuzz
    IMPL_TARGET       tbsz-impl
    OUTPUT_NAME       "TB Suzz"
    ENTRY_SOURCE      Source/Plugin/ClapEntry.cpp
    BUNDLE_IDENTIFIER "com.tonybuzzcock.tb-suzz"
    BUNDLE_VERSION    ${PROJECT_VERSION}
    COPY_AFTER_BUILD  FALSE
    PLUGIN_FORMATS    ${TBSZ_FORMATS}
    ASSET_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/plugins

    AUV2_MANUFACTURER_NAME "TB"
    AUV2_MANUFACTURER_CODE "TBel"
    AUV2_SUBTYPE_CODE      "TBsz"
    AUV2_INSTRUMENT_TYPE   "aufx"
)

# ---- Tests ----
add_executable(suzz_test Tests/suzz_test.cpp ${DSP_SOURCES})
target_include_directories(suzz_test PRIVATE Source/DSP)

add_executable(preset_test Tests/preset_test.cpp Source/Plugin/Presets.cpp)
target_include_directories(preset_test PRIVATE Source/Plugin)

add_executable(clap_host_test Tests/clap_host_test.cpp ${DSP_SOURCES})
target_link_libraries(clap_host_test PRIVATE clap ${CMAKE_DL_LIBS})
target_include_directories(clap_host_test PRIVATE Source/Plugin Source/DSP)

if (APPLE)
    add_executable(panel_test Tests/panel_test.mm)
    target_link_libraries(panel_test PRIVATE clap "-framework Cocoa")
    target_include_directories(panel_test PRIVATE Source/Plugin Source/DSP)
    set_source_files_properties(Tests/panel_test.mm PROPERTIES COMPILE_FLAGS "-fno-objc-arc")
endif()
