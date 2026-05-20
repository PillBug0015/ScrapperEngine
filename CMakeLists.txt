if(NOT CMAKE_TOOLCHAIN_FILE AND DEFINED ENV{VCPKG_ROOT})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
endif()

cmake_minimum_required(VERSION 3.15)
project(MyMudGame VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 1. file search
file(GLOB_RECURSE HEADERS CONFIGURE_DEPENDS "header/*.h" "header/*.hpp")
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "source/*.cpp")

# 2. find lib
find_package(fmt CONFIG REQUIRED)
find_package(ftxui CONFIG REQUIRED) 
find_package(nlohmann_json CONFIG REQUIRED)


add_executable(MyMudGame ${SOURCES} ${HEADERS})

target_include_directories(MyMudGame PRIVATE header)

if(WIN32)
    target_compile_definitions(MyMudGame PRIVATE USING_WINDOWS)
elseif(APPLE)
    target_compile_definitions(MyMudGame PRIVATE USING_MAC)
endif()

target_link_libraries(MyMudGame PRIVATE 
    fmt::fmt
    ftxui::screen
    ftxui::dom
    ftxui::component
    nlohmann_json::nlohmann_json
)

if(MSVC)
    target_compile_options(MyMudGame PRIVATE /W4) 
else()
    target_compile_options(MyMudGame PRIVATE -Wall -Wextra -Wpedantic) 
endif()