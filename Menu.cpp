#include "Menu.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

Menu::Menu(float menuX, float menuY, float menuWidth, float menuRowHeight)
    : x(menuX), y(menuY), width(menuWidth), rowHeight(menuRowHeight) {
    refresh();
}

std::size_t Menu::addItem(const std::string& text,
                          std::function<void()> onClick) {
    Item item;
    item.text = text;
    item.onClick = std::move(onClick);
    m_items.push_back(std::move(item));
    refresh();
    return m_items.size() - 1;
}

bool Menu::removeItem(std::size_t index) {
    if (index >= m_items.size()) return false;
    m_items.erase(m_items.begin() + static_cast<std::ptrdiff_t>(index));
    refresh();
    return true;
}

bool Menu::setItemText(std::size_t index, const std::string& text) {
    if (index >= m_items.size()) return false;
    m_items[index].text = text;
    refresh();
    return true;
}

bool Menu::setItemCallback(std::size_t index,
                           std::function<void()> onClick) {
    if (index >= m_items.size()) return false;
    m_items[index].onClick = std::move(onClick);
    refresh();
    return true;
}

bool Menu::setItemEnabled(std::size_t index, bool enabled) {
    if (index >= m_items.size()) return false;
    m_items[index].enabled = enabled;
    refresh();
    return true;
}

bool Menu::setItemVisible(std::size_t index, bool visible) {
    if (index >= m_items.size()) return false;
    m_items[index].visible = visible;
    refresh();
    return true;
}

void Menu::setLineSpacing(float spacing) {
    lineSpacing = std::isfinite(spacing)
        ? std::max(0.0f, spacing)
        : 0.0f;
    refresh();
}

void Menu::clear() {
    m_items.clear();
    refresh();
}

void Menu::refresh() {
    ++m_revision;
    for (std::size_t index = 0; index < m_items.size(); ++index) {
        Item& item = m_items[index];
        const float itemY =
            y + static_cast<float>(index) * (rowHeight + lineSpacing);

        item.background = Rectangle(
            x, itemY, width, rowHeight, backgroundColor);
        item.background.enabled = enabled && item.enabled && item.visible;
        item.background.visible = visible && item.visible;
        item.background.textureLayer = -1;
        item.background.onClick = [this, index]() {
            if (index >= m_items.size()) return;
            Item& current = m_items[index];
            if (enabled && visible && current.enabled && current.visible &&
                current.onClick) {
                current.onClick();
            }
        };

        item.spacingBelow = Rectangle(
            x,
            itemY + rowHeight,
            width,
            lineSpacing,
            backgroundColor);
        item.spacingBelow.enabled = false;
        item.spacingBelow.visible =
            visible && item.visible && lineSpacing > 0.0f;
        item.spacingBelow.textureLayer = -1;

        item.label = TextLabel{
            item.text,
            x + textPadding,
            itemY + rowHeight * 0.67f,
            textScale,
            textColor,
            visible && item.visible
        };
    }
}

bool Menu::animateTo(float targetX, float targetY, float durationSeconds,
                     uint32_t repeatCount) {
    Animation animation = Animation::linear(
        {x, y}, {targetX, targetY}, durationSeconds, repeatCount);
    if (!animation.active()) return false;
    m_positionAnimation = std::move(animation);
    return true;
}

bool Menu::animateBezierTo(float targetX, float targetY,
                           float control1X, float control1Y,
                           float control2X, float control2Y,
                           float durationSeconds, uint32_t repeatCount) {
    Animation animation = Animation::bezier(
        {x, y}, {control1X, control1Y}, {control2X, control2Y},
        {targetX, targetY}, durationSeconds, repeatCount);
    if (!animation.active()) return false;
    m_positionAnimation = std::move(animation);
    return true;
}

void Menu::setAnimationRepeatCount(uint32_t repeatCount) {
    if (m_positionAnimation)
        m_positionAnimation->setRepeatCount(repeatCount);
}

void Menu::cancelAnimation() {
    m_positionAnimation.reset();
}

bool Menu::advanceAnimation(float deltaSeconds) {
    if (!m_positionAnimation) return false;

    const float oldX = x;
    const float oldY = y;
    AnimationPoint position{};
    const bool stillActive =
        m_positionAnimation->advance(deltaSeconds, position);
    x = position.x;
    y = position.y;
    refresh();

    if (!stillActive)
        m_positionAnimation.reset();
    return oldX != x || oldY != y;
}

bool Menu::animationActive() const {
    return m_positionAnimation && m_positionAnimation->active();
}
