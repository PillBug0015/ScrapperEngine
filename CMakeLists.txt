cmake_minimum_required(VERSION 3.15)

# set up vcpkg 
if(NOT CMAKE_TOOLCHAIN_FILE AND DEFINED ENV{VCPKG_ROOT})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
endif()

# achitecture settings
# complier won't shut up.
if(APPLE)
    set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "" FORCE)
    set(VCPKG_TARGET_TRIPLET "arm64-osx" CACHE STRING "" FORCE)
endif()

project(MyMudGame VERSION 0.0.1 LANGUAGES C CXX)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 1. load packages
find_package(raylib CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(EnTT CONFIG REQUIRED)
find_package(Jolt CONFIG REQUIRED)
find_package(unofficial-enet CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)

function(scrapper_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /utf-8)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endfunction()

# 2. engine modules: one static library per folder under engine/, public headers in include/
function(scrapper_module name dir)
    file(GLOB_RECURSE sources CONFIGURE_DEPENDS "${dir}/src/*.cpp" "${dir}/src/*.h" "${dir}/include/*.h")
    add_library(${name} STATIC ${sources})
    target_include_directories(${name} PUBLIC ${dir}/include PRIVATE ${dir}/src)
    scrapper_warnings(${name})
endfunction()

scrapper_module(Scrapper.Core engine/Core)
target_link_libraries(Scrapper.Core PUBLIC EnTT::EnTT)
# Core uses raylib's math structs and header-only raymath, never raylib's functions.
target_include_directories(Scrapper.Core PUBLIC $<TARGET_PROPERTY:raylib,INTERFACE_INCLUDE_DIRECTORIES>)

scrapper_module(Scrapper.Assets engine/Assets)
# cgltf's implementation comes from raylib: vcpkg builds raylib against vcpkg's cgltf, the same header
# included here. Compiling CGLTF_IMPLEMENTATION again would clash with raylib's copy at link time.
target_link_libraries(Scrapper.Assets PUBLIC Scrapper.Core PRIVATE raylib)

scrapper_module(Scrapper.Raylib engine/Raylib)
target_link_libraries(Scrapper.Raylib PUBLIC Scrapper.Core Scrapper.Assets raylib glfw)
if(APPLE)
    target_link_libraries(Scrapper.Raylib PUBLIC
        "-framework Cocoa"
        "-framework IOKit"
        "-framework CoreVideo"
        "-framework OpenGL"
        "-framework CoreAudio"
        "-framework AudioToolbox"
    )
endif()

scrapper_module(Scrapper.Jolt engine/Jolt)
target_link_libraries(Scrapper.Jolt PUBLIC Scrapper.Core Scrapper.Assets Jolt::Jolt)

# ENet stays private: nothing outside Scrapper.Net can include it (it drags windows.h, which breaks raylib).
scrapper_module(Scrapper.Net engine/Net)
target_link_libraries(Scrapper.Net PUBLIC Scrapper.Core PRIVATE unofficial::enet::enet)
if(WIN32)
    # ENet's own CMake links these only for MinGW, so MSVC builds need them here.
    target_link_libraries(Scrapper.Net PRIVATE ws2_32 winmm)
endif()

scrapper_module(Scrapper.ScriptEngine engine/ScriptEngine)
target_link_libraries(Scrapper.ScriptEngine PUBLIC Scrapper.Core)

scrapper_module(Scrapper.ScriptEngine.ForJolt engine/ScriptEngine.ForJolt)
target_link_libraries(Scrapper.ScriptEngine.ForJolt PUBLIC Scrapper.ScriptEngine Scrapper.Jolt)

# 3. game
file(GLOB_RECURSE GAME_SOURCES CONFIGURE_DEPENDS "Assets/source/*.cpp" "Assets/source/*.h")
add_executable(MyMudGame ${GAME_SOURCES})
scrapper_warnings(MyMudGame)
target_link_libraries(MyMudGame PRIVATE
    Scrapper.Raylib
    Scrapper.Jolt
    Scrapper.Net
    Scrapper.ScriptEngine
    Scrapper.ScriptEngine.ForJolt
    nlohmann_json::nlohmann_json
    fmt::fmt
)

if(WIN32)
    target_compile_definitions(MyMudGame PRIVATE USING_WINDOWS)
elseif(APPLE)
    target_compile_definitions(MyMudGame PRIVATE USING_MAC)
endif()

# 4. models: every Assets/Models/*.blend is exported to a .glb beside it. The .glb is committed,
#    so machines without Blender build with the last exported version.
find_program(BLENDER_EXECUTABLE NAMES Blender blender
    PATHS
        "$ENV{HOME}/Library/Application Support/Steam/steamapps/common/Blender/Blender.app/Contents/MacOS"
        "/Applications/Blender.app/Contents/MacOS"
        "C:/Program Files (x86)/Steam/steamapps/common/Blender"
    DOC "Blender used to export Assets/Models/*.blend")
set(MODEL_EXPORTER "${CMAKE_CURRENT_SOURCE_DIR}/tools/blender/export_glb.py")
file(GLOB MODEL_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/Assets/Models/*.blend")
set(MODEL_OUTPUTS)
foreach(blend ${MODEL_SOURCES})
    get_filename_component(model_name ${blend} NAME_WE)
    set(glb "${CMAKE_CURRENT_SOURCE_DIR}/Assets/Models/${model_name}.glb")
    if(BLENDER_EXECUTABLE)
        add_custom_command(OUTPUT ${glb}
            COMMAND ${BLENDER_EXECUTABLE} -b --factory-startup ${blend} --python ${MODEL_EXPORTER} -- ${glb}
            DEPENDS ${blend} ${MODEL_EXPORTER}
            COMMENT "Exporting ${model_name}.blend")
    endif()
    list(APPEND MODEL_OUTPUTS ${glb})
endforeach()
if(NOT BLENDER_EXECUTABLE)
    message(STATUS "Blender not found: using the committed Assets/Models/*.glb")
endif()
add_custom_target(ScrapperModels ALL DEPENDS ${MODEL_OUTPUTS})
add_dependencies(MyMudGame ScrapperModels)

# 5. copy font and models next to the executable
add_custom_command(TARGET MyMudGame POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/Fonts"
        "$<TARGET_FILE_DIR:MyMudGame>/Fonts"
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:MyMudGame>/Models"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${MODEL_OUTPUTS} "$<TARGET_FILE_DIR:MyMudGame>/Models"
    COMMENT "Deploying fonts and models to the build output folder..."
)
