#pragma once

#include "Rectangle.hpp"
#include "Text.hpp"

#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <string>
#include <utility>
#include <vector>

// A vertical menu whose items are individually clickable text rows.
// Menu owns the visual Rectangle/TextLabel parts and layout; callers own the
// Menu and keep it alive while it is registered with a Renderer.
class Menu {
public:
    struct Item {
        std::string text;
        std::function<void()> onClick;
        bool enabled = true;
        bool visible = true;

        Rectangle background;
        Rectangle spacingBelow;
        TextLabel label;
    };

    Menu(float x, float y, float width, float rowHeight);

    float x = 0.0f;
    float y = 0.0f;
    float width = 200.0f;
    float rowHeight = 40.0f;
    float lineSpacing = 0.0f;
    float textScale = 0.32f;
    float textPadding = 10.0f;
    std::array<float, 4> backgroundColor = {0.18f, 0.22f, 0.28f, 1.0f};
    std::array<float, 4> textColor = {1.0f, 1.0f, 1.0f, 1.0f};
    bool enabled = true;
    bool visible = true;

    std::size_t addItem(const std::string& text,
                        std::function<void()> onClick = {});
    bool removeItem(std::size_t index);
    bool setItemText(std::size_t index, const std::string& text);
    bool setItemCallback(std::size_t index,
                         std::function<void()> onClick);
    bool setItemEnabled(std::size_t index, bool enabled);
    bool setItemVisible(std::size_t index, bool visible);
    void setLineSpacing(float spacing);
    float getLineSpacing() const { return lineSpacing; }
    void clear();
    void refresh();
    std::uint64_t revision() const { return m_revision; }

    std::size_t itemCount() const { return m_items.size(); }
    const std::deque<Item>& items() const { return m_items; }
    std::deque<Item>& items() { return m_items; }

private:
    std::deque<Item> m_items;
    std::uint64_t m_revision = 1;
};
