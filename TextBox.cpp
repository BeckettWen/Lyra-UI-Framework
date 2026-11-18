#include "TextBox.hpp"

#include <algorithm>

TextBox::TextBox(float boxX, float boxY, float boxWidth, float boxHeight)
    : x(boxX), y(boxY), width(boxWidth), height(boxHeight),
      label{"", boxX + 10.0f, boxY + boxHeight * 0.67f, 0.32f,
            {1.0f, 1.0f, 1.0f, 1.0f}, true},
      m_value{} {
    for (Rectangle& part : outline) {
        part.enabled = false;
        part.textureLayer = -1;
        part.color = {0.85f, 0.85f, 0.85f, 1.0f};
    }
    syncVisuals();
}

bool TextBox::contains(double cursorX, double cursorY) const {
    return enabled && visible &&
           cursorX >= x && cursorX <= x + width &&
           cursorY >= y && cursorY <= y + height;
}

void TextBox::setFocused(bool focused) {
    m_focused = enabled && visible && focused;
    syncVisuals();
}

void TextBox::appendCodepoint(unsigned int codepoint) {
    if (!enabled || codepoint < 32 || codepoint == 127 ||
        m_value.size() >= m_maxCharacters) {
        return;
    }

    std::string encoded;
    if (codepoint <= 0x7F) {
        encoded.push_back(static_cast<char>(codepoint));
    } else if (codepoint <= 0x7FF) {
        encoded.push_back(static_cast<char>(0xC0 | (codepoint >> 6)));
        encoded.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
    } else if (codepoint <= 0xFFFF) {
        encoded.push_back(static_cast<char>(0xE0 | (codepoint >> 12)));
        encoded.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
        encoded.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
    } else if (codepoint <= 0x10FFFF) {
        encoded.push_back(static_cast<char>(0xF0 | (codepoint >> 18)));
        encoded.push_back(static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F)));
        encoded.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
        encoded.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
    }

    if (!encoded.empty() &&
        m_value.size() + encoded.size() <= m_maxCharacters) {
        m_value.insert(m_value.end(), encoded.begin(), encoded.end());
        syncVisuals();
    }
}

void TextBox::backspace() {
    if (!enabled || m_value.empty()) return;

    const unsigned char last = static_cast<unsigned char>(m_value.back());
    if ((last & 0xC0) == 0x80) {
        while (!m_value.empty() &&
               (static_cast<unsigned char>(m_value.back()) & 0xC0) == 0x80) {
            m_value.pop_back();
        }
        if (!m_value.empty())
            m_value.pop_back();
    } else {
        m_value.pop_back();
    }
    syncVisuals();
}

void TextBox::clear() {
    m_value.clear();
    syncVisuals();
}

void TextBox::setText(const std::string& text) {
    m_value.assign(text.begin(), text.end());
    if (m_value.size() > m_maxCharacters)
        m_value.resize(m_maxCharacters);
    syncVisuals();
}

void TextBox::setMaxCharacters(std::size_t maxCharacters) {
    m_maxCharacters = maxCharacters;
    if (m_value.size() > m_maxCharacters)
        m_value.resize(m_maxCharacters);
    syncVisuals();
}

void TextBox::refresh() {
    syncVisuals();
}

void TextBox::syncVisuals() {
    const float thickness =
        std::clamp(outlineThickness, 1.0f, std::max(1.0f, height * 0.25f));

    outline[0] = Rectangle(x, y, width, thickness,
                           m_focused
                               ? std::array<float, 4>{0.3f, 0.65f, 1.0f, 1.0f}
                               : std::array<float, 4>{0.85f, 0.85f, 0.85f, 1.0f});
    outline[1] = Rectangle(x, y + height - thickness, width, thickness,
                           outline[0].color);
    outline[2] = Rectangle(x, y, thickness, height, outline[0].color);
    outline[3] = Rectangle(x + width - thickness, y, thickness, height,
                           outline[0].color);
    for (Rectangle& part : outline) {
        part.enabled = false;
        part.visible = visible;
        part.textureLayer = -1;
    }

    label.text.assign(m_value.begin(), m_value.end());
    label.x = x + 10.0f;
    label.baselineY = y + height * 0.67f;
    label.visible = visible;
}
