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

# 1. find source
file(GLOB_RECURSE HEADERS CONFIGURE_DEPENDS "header/*.h" "header/*.hpp")
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "source/*.cpp")

# 2. load package
find_package(raylib CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED) 
find_package(nlohmann_json CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)

# 3. executable
add_executable(MyMudGame ${SOURCES} ${HEADERS})

target_include_directories(MyMudGame PRIVATE header ${HEADER_DIRS})

# 4. cross platform
if(WIN32)
    target_compile_definitions(MyMudGame PRIVATE USING_WINDOWS)
elseif(APPLE)
    target_compile_definitions(MyMudGame PRIVATE USING_MAC)
endif()

if(MSVC)
    target_compile_options(MyMudGame PRIVATE /W4) 
else()
    target_compile_options(MyMudGame PRIVATE -Wall -Wextra -Wpedantic) 
endif()

# 5. link libs
target_link_libraries(MyMudGame PRIVATE 
    glfw
    raylib
    nlohmann_json::nlohmann_json
    fmt::fmt
)

if(APPLE)
    target_link_libraries(MyMudGame PRIVATE 
        "-framework Cocoa"
        "-framework IOKit"
        "-framework CoreVideo"
        "-framework OpenGL"
        "-framework CoreAudio"
        "-framework AudioToolbox"
    )
endif()
