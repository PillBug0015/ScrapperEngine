cmake_minimum_required(VERSION 3.10)

project(MyMudGame VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB_RECURSE SOURCE_FILES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp"
)

add_executable(MyMudGame MACOSX_BUNDLE ${SOURCE_FILES})

target_include_directories(MyMudGame PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/header"
)

set(BEARLIBTERMINAL_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/external/bearlibterminal")
set(BEARLIBTERMINAL_INCLUDE_DIR "${BEARLIBTERMINAL_ROOT}/include")

if(APPLE AND CMAKE_SYSTEM_PROCESSOR MATCHES "^(arm64|aarch64)$")
    set(BEARLIBTERMINAL_LIBRARY_DIR "${BEARLIBTERMINAL_ROOT}/lib/macos-arm64")
else()
    message(FATAL_ERROR "BearLibTerminal is currently vendored only for macOS arm64 in this project.")
endif()

file(GLOB BEARLIBTERMINAL_LIBRARIES CONFIGURE_DEPENDS
    "${BEARLIBTERMINAL_LIBRARY_DIR}/*.a"
    "${BEARLIBTERMINAL_LIBRARY_DIR}/*.dylib"
)

file(GLOB BEARLIBTERMINAL_RUNTIME_LIBRARIES CONFIGURE_DEPENDS
    "${BEARLIBTERMINAL_LIBRARY_DIR}/*.dylib"
)

target_include_directories(MyMudGame PRIVATE
    "${BEARLIBTERMINAL_INCLUDE_DIR}"
)
target_link_directories(MyMudGame PRIVATE
    "${BEARLIBTERMINAL_LIBRARY_DIR}"
)
target_link_libraries(MyMudGame PRIVATE
    ${BEARLIBTERMINAL_LIBRARIES}
)
add_custom_command(TARGET MyMudGame POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${BEARLIBTERMINAL_RUNTIME_LIBRARIES}
        "$<TARGET_FILE_DIR:MyMudGame>"
)

include(FetchContent)
FetchContent_Declare(
    json
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
)
FetchContent_MakeAvailable(json)
target_link_libraries(MyMudGame PRIVATE nlohmann_json::nlohmann_json)