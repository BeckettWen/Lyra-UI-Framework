#pragma once

#include "Rectangle.hpp"
#include "Text.hpp"

#include <cstddef>
#include <cstdint>
#include <expected>
#include <map>
#include <memory>
#include <variant>
#include <vector>

enum class ViewOrientation {
    Horizontal,
    Vertical
};

enum class ViewError {
    InvalidBounds,
    Overlap,
    ComponentDoesNotFit,
    ComponentAlreadyAdded,
    BackendUnavailable
};

struct ViewBounds {
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
};

class View {
public:
    ViewOrientation orientation() const { return m_orientation; }
    const ViewBounds& bounds() const { return m_bounds; }

    // Components are laid out in insertion order. Text dimensions are
    // supplied in logical pixels; baselineY is positioned at the item's bottom.
    std::expected<void, ViewError> add(Rectangle& rectangle);
    std::expected<void, ViewError> add(TextLabel& text,
                                       float layoutWidth, float layoutHeight);
    bool remove(Rectangle& rectangle);
    bool remove(TextLabel& text);

    std::size_t componentCount() const { return m_components.size(); }

private:
    friend class ViewSpace;

    using Component = std::variant<Rectangle*, TextLabel*>;
    struct LayoutItem {
        Component component;
        float width = 0.0f;
        float height = 0.0f;
    };

    View(ViewOrientation orientation, ViewBounds bounds,
         void* backendContext, std::uint64_t backendID);
    bool canFit(float width, float height) const;
    void layoutComponents();

    ViewOrientation m_orientation;
    ViewBounds m_bounds;
    void* m_backendContext = nullptr;
    std::uint64_t m_backendID = 0;
    std::vector<LayoutItem> m_components;
};

class ViewSpace {
public:
    ViewSpace();
    ~ViewSpace();

    ViewSpace(const ViewSpace&) = delete;
    ViewSpace& operator=(const ViewSpace&) = delete;

    std::expected<View*, ViewError> createHorizontalView(
        float x, float y, float width, float height);
    std::expected<View*, ViewError> createVerticalView(
        float x, float y, float width, float height);

    bool removeView(View* view);
    std::size_t viewCount() const { return m_views.size(); }

private:
    std::expected<View*, ViewError> createView(
        ViewOrientation orientation, ViewBounds bounds);

    // On macOS these are C++ handles for Swift-owned view records; elsewhere
    // the C++ map also tracks the occupied regions.
    std::map<std::uint64_t, std::unique_ptr<View>> m_views;
    std::size_t m_nextId = 0;
    void* m_backendContext = nullptr;
};
