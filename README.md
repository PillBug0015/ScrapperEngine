# MyMudGame

raylib 기반 크로스플랫폼(Mac / Windows) 프로젝트. 의존성은 [vcpkg](https://github.com/microsoft/vcpkg) manifest(`vcpkg.json`)로 관리하고, 빌드는 CMake Presets로 합니다.

## 사전 준비

1. **CMake** (3.15+) 와 C++17 컴파일러
   - Mac: Xcode Command Line Tools (`xcode-select --install`)
   - Windows: Visual Studio 2022 (Desktop development with C++)
2. **vcpkg** 설치 후 `VCPKG_ROOT` 환경변수 설정 — 이게 빠지면 CMake가 라이브러리를 못 찾습니다.

### `VCPKG_ROOT` 설정

**Mac (zsh)** — `~/.zshrc`에 추가:
```bash
export VCPKG_ROOT="$HOME/vcpkg"
```
> VS Code는 **터미널에서 `code .`로 열어야** 이 변수를 상속받습니다. (Dock 아이콘으로 열면 IntelliSense가 vcpkg를 못 찾습니다.)

**Windows** — 시스템 환경변수로 등록 (GUI 앱도 자동 상속):
```
VCPKG_ROOT = C:\vcpkg
```

## 빌드

```bash
# 구성 (의존성 자동 설치됨)
cmake --preset MacOS      # Windows: cmake --preset Windows

# 빌드
cmake --build --preset MacOS   # Windows: cmake --build --preset Windows
```

결과물: `build/<OS>/bin/` 아래에 실행 파일 생성.

## VS Code

`.vscode/settings.json`이 CMake Presets를 쓰도록 설정돼 있어, 권장 확장(CMake Tools, C/C++)만 설치하면 IntelliSense·빌드·디버그가 자동으로 동작합니다. 디버깅은 하단 상태바의 CMake **Debug** 버튼을 사용하세요 (`launch.json` 불필요).
