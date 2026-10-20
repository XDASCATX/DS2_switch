# ds2 - host application for the recompiled Dark Souls II: Scholar of the First Sin (Xbox 360, 465307E4).
#
# Unlike the template from `rexglue init`, this file does not include generated/rexglue.cmake: that file
# re-runs codegen with a hard-wired ds2_manifest.toml (the working manifest, which still fails validation)
# and is rewritten by every codegen run. Here the generated directory is chosen explicitly.
#
#   cmake -S app -B out/app-host -G Ninja -DCMAKE_BUILD_TYPE=Release \
#     -DCMAKE_C_COMPILER=clang-20 -DCMAKE_CXX_COMPILER=clang++-20 \
#     -DCMAKE_C_FLAGS=-march=x86-64-v3 -DCMAKE_CXX_FLAGS=-march=x86-64-v3
#   cmake --build out/app-host --target ds2 -j2
#
# Nintendo Switch (devkitA64 + libnx + the Mesa NVK SDK, see docs/handoff/10-switch-build.md):
#   cmake -S app -B out/app-switch -G Ninja -DCMAKE_BUILD_TYPE=Release \
#     -DCMAKE_TOOLCHAIN_FILE=tools/switch/cmake/switch-devkitA64.cmake \
#     -DREXGLUE_SWITCH_NVK_SDK=<mesa-sdk>/opt/devkitpro/portlibs/switch
#   cmake --build out/app-switch --target ds2 -j2        -> out/app-switch/ds2.nro

cmake_minimum_required(VERSION 3.25)
project(ds2 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(REXSDK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../sdk" CACHE PATH "ReXGlue SDK source tree")
set(DS2_GENERATED_DIR "${CMAKE_CURRENT_SOURCE_DIR}/generated/diag-dataonly" CACHE PATH
    "codegen output directory (diagnostic configuration that passes validation)")
if(NOT EXISTS "${DS2_GENERATED_DIR}/sources.cmake")
    message(FATAL_ERROR "No generated code in ${DS2_GENERATED_DIR}. Run codegen first "
                        "(MANIFEST=app/ds2_manifest.diag-dataonly.toml tools/run_codegen_diag.sh <tag>).")
endif()

add_subdirectory("${REXSDK_DIR}" rexglue-sdk)

# Recompiled guest code, with the same options generated/rexglue.cmake would apply.
include("${DS2_GENERATED_DIR}/sources.cmake")
add_library(ds2_recomp OBJECT ${GENERATED_SOURCES})
target_include_directories(ds2_recomp PRIVATE "${DS2_GENERATED_DIR}")
target_link_libraries(ds2_recomp PRIVATE rex::runtime)
target_precompile_headers(ds2_recomp PRIVATE "${DS2_GENERATED_DIR}/ds2_pch.h")
target_compile_options(ds2_recomp PRIVATE $<$<CXX_COMPILER_ID:Clang,AppleClang>:-gline-tables-only>)
rexglue_apply_target_settings(ds2_recomp)

set(DS2_SOURCES
    src/main.cpp
    src/ds2_capture.cpp
    src/ds2_input_script.cpp
    src/ds2_dlc.cpp
    src/ds2_watchdog.cpp
    src/ds2_switch_platform.cpp
)
add_executable(ds2 ${DS2_SOURCES})
target_include_directories(ds2 PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src" "${DS2_GENERATED_DIR}")
target_link_libraries(ds2 PRIVATE ds2_recomp rex::runtime)
if(REXGLUE_PLATFORM_SWITCH)
    # One static executable: the GPU plugin is linked in instead of loaded from a shared library.
    rexglue_configure_target(ds2 GPU_PLUGINS xenos)
    rexglue_switch_add_nro(ds2
        NAME "Dark Souls II SotFS (recomp)"
        AUTHOR "DS2 Switch port"
        VERSION "0.1.0")
else()
    rexglue_configure_target(ds2)
endif()

# rex_app.cpp includes imgui.h, but rexruntime links imgui privately, so the include path does not reach the
# executable when the SDK is a subdirectory (same fix as nfsmw-nx app/CMakeLists.txt; on Switch
# rexglue_switch.cmake does it).
if(NOT REXGLUE_PLATFORM_SWITCH AND TARGET imgui)
    target_include_directories(ds2 PRIVATE $<TARGET_PROPERTY:imgui,INTERFACE_INCLUDE_DIRECTORIES>)
endif()

# Put ds2 next to librexruntime.so (sdk/out/<platform>/) so the $ORIGIN rpath set by rexglue_configure_target finds it.
if(NOT REXGLUE_PLATFORM_SWITCH)
    set_target_properties(ds2 PROPERTIES RUNTIME_OUTPUT_DIRECTORY "$<TARGET_FILE_DIR:rex::runtime>")
endif()
