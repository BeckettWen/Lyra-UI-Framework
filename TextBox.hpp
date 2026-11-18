#pragma once

#include "Rectangle.hpp"
#include "Text.hpp"

#include <array>
#include <cstddef>
#include <string>
#include <vector>

class TextBox {
public:
    TextBox(float x, float y, float width, float height);

    float x = 0.0f;
    float y = 0.0f;
    float width = 200.0f;
    float height = 40.0f;
    float outlineThickness = 2.0f;
    bool enabled = true;
    bool visible = true;

    std::array<Rectangle, 4> outline{};
    TextLabel label;

    bool contains(double cursorX, double cursorY) const;
    bool focused() const { return m_focused; }
    void setFocused(bool focused);

    void appendCodepoint(unsigned int codepoint);
    void backspace();
    void clear();
    void setText(const std::string& text);
    void refresh();
    const std::vector<char>& get_textbox_value() const { return m_value; }
    std::size_t maxCharacters() const { return m_maxCharacters; }
    void setMaxCharacters(std::size_t maxCharacters);

private:
    void syncVisuals();

    std::vector<char> m_value;
    std::size_t m_maxCharacters = 256;
    bool m_focused = false;
};
