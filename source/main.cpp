#include <iostream>

void InitializeConsole() {
#ifdef USING_WINDOWS
    // 윈도우 전용 콘솔 설정 코드
    std::cout << "윈도우 환경에서 MUD 게임을 시작합니다." << std::endl;
#elif defined(USING_MAC)
    // 맥 전용 터미널 설정 코드
    std::cout << "맥 환경에서 MUD 게임을 시작합니다." << std::endl;
#endif
}