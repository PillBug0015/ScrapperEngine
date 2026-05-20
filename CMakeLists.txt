if(NOT CMAKE_TOOLCHAIN_FILE AND DEFINED ENV{VCPKG_ROOT})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
endif()
cmake_minimum_required(VERSION 3.15)
project(MyMudGame VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


file(GLOB_RECURSE HEADERS CONFIGURE_DEPENDS "header/*.h" "header/*.hpp")
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "source/*.cpp")


if(WIN32)
    file(GLOB_RECURSE WIN_SOURCES CONFIGURE_DEPENDS "source/windows/*.cpp")
    list(APPEND SOURCES ${WIN_SOURCES})
    
    add_compile_definitions(USING_WINDOWS)
elseif(APPLE)
    file(GLOB_RECURSE MAC_SOURCES CONFIGURE_DEPENDS "source/mac/*.cpp")
    list(APPEND SOURCES ${MAC_SOURCES})
    
    add_compile_definitions(USING_MAC)
endif()

find_package(fmt CONFIG REQUIRED)


add_executable(MyMudGame ${SOURCES} ${HEADERS})
target_include_directories(MyMudGame PRIVATE header)


if(MSVC)
    target_compile_options(MyMudGame PRIVATE /W4) 
else()
    target_compile_options(MyMudGame PRIVATE -Wall -Wextra -Wpedantic) 
endif()