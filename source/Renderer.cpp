#include "Renderer.h"

#include <fmt/core.h>

ftxui::Element Renderer::RenderMenu() const {
    using namespace ftxui;

    return vbox({
               text(fmt::format("=== {} ===", "전설의 MUD 게임")) | bold | color(Color::Green) | hcenter,
               separator(),
               text(" [1] 게임 시작 ") | border,
               text(" [3] 게임 종료 ") | border,
               separator(),
               text("숫자 키를 누르세요.") | color(Color::GrayLight) | hcenter,
           }) |
           border;
}

ftxui::Element Renderer::RenderWorld(const World& world) const {
    using namespace ftxui;

    Elements rows;

    for (int y = 0; y < world.Height(); ++y) {
        Elements cells;

        for (int x = 0; x < world.Width(); ++x) {
            cells.push_back(RenderCell(world.GetTile(x, y), world.GetEntityAt(x, y)));
        }

        rows.push_back(hbox(std::move(cells)));
    }

    return vbox({
               text("월드") | bold | color(Color::Green) | hcenter,
               separator(),
               vbox(std::move(rows)) | hcenter,
               separator(),
               hbox({
                   text("@ 플레이어 ") | color(Color::Yellow),
                   text("T 나무 ") | color(Color::Green),
                   text("# 벽 ") | color(Color::GrayDark),
                   text(". 길 ") | color(Color::White),
                   text("~ 물") | color(Color::Blue),
               }) | hcenter,
               text("WASD 또는 방향키로 이동, Q/Esc로 종료") | color(Color::GrayLight) | hcenter,
           }) |
           border;
}

ftxui::Element Renderer::RenderCell(const Tile& tile, const Entity* entity) const {
    if (entity != nullptr) {
        return RenderEntity(*entity);
    }

    return RenderTile(tile);
}

ftxui::Element Renderer::RenderEntity(const Entity& entity) const {
    using namespace ftxui;

    switch (entity.Type()) {
    case EntityType::Player:
        return text(fmt::format(" {} ", entity.Symbol())) | color(Color::Yellow) | bold;
    case EntityType::Tree:
        return text(fmt::format(" {} ", entity.Symbol())) | color(Color::Green);
    default:
        return text(fmt::format(" {} ", entity.Symbol())) | color(Color::White);
    }
}

ftxui::Element Renderer::RenderTile(const Tile& tile) const {
    using namespace ftxui;

    switch (tile.type) {
    case TileType::Wall:
        return text(" # ") | color(Color::GrayDark);
    case TileType::Water:
        return text(" ~ ") | color(Color::Blue);
    case TileType::Floor:
    default:
        return text(" . ") | color(Color::White);
    }
}
