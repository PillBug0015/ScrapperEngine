#include <iostream>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/screen.hpp>
#include <fmt/core.hpp>

int main() {
    using namespace ftxui;

    // 1. FTXUI로 멋진 UI 요소 그리기
    Element document = vbox({
        text(fmt::format("=== {} ===", "전설의 MUD 게임")) | bold | color(Color::Green) | hcenter,
        hbox({
            text(" [1] 게임 시작 ") | border,
            text(" [2] 이어 하기 ") | border,
            text(" [3] 게임 종료 ") | border,
        }) | hcenter,
    }) | border;

    // 2. 화면 크기 지정 및 렌더링
    auto screen = Screen::Create(Dimension::Full(), Dimension::Fit(document));
    Render(screen, document);
    screen.Print();

    return 0;
}