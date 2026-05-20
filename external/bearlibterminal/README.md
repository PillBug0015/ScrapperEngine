# BearLibTerminal

This project vendors BearLibTerminal for C/C++ builds.

- `include/BearLibTerminal.h`: C/C++ API header from the upstream project.
- `lib/macos-arm64/libBearLibTerminal.dylib`: local macOS arm64 build.

The root `CMakeLists.txt` imports this dylib and copies it beside the
`MyMudGame` executable after each build so the runtime loader can find it.
