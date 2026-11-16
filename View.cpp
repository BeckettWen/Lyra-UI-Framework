#include "View.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <type_traits>

#if defined(LYRA_USE_METAL)
extern "C" void* lyraViewSpaceCreate();
extern "C" void lyraViewSpaceDestroy(void* space);
extern "C" int32_t lyraViewCreate(void* space, int32_t orientation,
                                  float x, float y, float width, float height,
                                  uint64_t* viewID);
extern "C" int32_t lyraViewRemove(void* space, uint64_t viewID);
extern "C" int32_t lyraViewAddComponent(void* space, uint64_t viewID,
                                        uint64_t componentID,
                                        float width, float height);
extern "C" int32_t lyraViewRemoveComponent(void* space, uint64_t viewID,
                                           uint64_t componentID);
extern "C" int32_t lyraViewGetComponentPosition(void* space, uint64_t viewID,
                                                uint64_t componentID,
                                                float* x, float* y);
#endif

namespace {
bool validBounds(const ViewBounds& bounds) {
    if (!std::isfinite(bounds.x) || !std::isfinite(bounds.y) ||
        !std::isfinite(bounds.width) || !std::isfinite(bounds.height) ||
        bounds.width <= 0.0f || bounds.height <= 0.0f)
        return false;
    return std::isfinite(bounds.x + bounds.width) &&
           std::isfinite(bounds.y + bounds.height);
}

bool overlaps(const ViewBounds& a, const ViewBounds& b) {
    return a.x < b.x + b.width && a.x + a.width > b.x &&
           a.y < b.y + b.height && a.y + a.height > b.y;
}

#if defined(LYRA_USE_METAL)
ViewError viewErrorFromBackend(int32_t error) {
    switch (error) {
    case 1: return ViewError::InvalidBounds;
    case 2: return ViewError::Overlap;
    case 3: return ViewError::ComponentDoesNotFit;
    case 4: return ViewError::ComponentAlreadyAdded;
    default: return ViewError::BackendUnavailable;
    }
}

std::uint64_t componentID(const void* component) {
    return static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(component));
}
#endif
}

View::View(ViewOrientation orientation, ViewBounds bounds,
           void* backendContext, std::uint64_t backendID)
    : m_orientation(orientation),
      m_bounds(bounds),
      m_backendContext(backendContext),
      m_backendID(backendID) {}

bool View::canFit(float width, float height) const {
    if (!std::isfinite(width) || !std::isfinite(height) ||
        width <= 0.0f || height <= 0.0f ||
        width > m_bounds.width || height > m_bounds.height)
        return false;

    float used = 0.0f;
    for (const LayoutItem& item : m_components)
        used += m_orientation == ViewOrientation::Horizontal
            ? item.width : item.height;
    const float available = m_orientation == ViewOrientation::Horizontal
        ? m_bounds.width : m_bounds.height;
    const float requested = m_orientation == ViewOrientation::Horizontal
        ? width : height;
    return requested <= available - used;
}

std::expected<void, ViewError> View::add(Rectangle& rectangle) {
#if defined(LYRA_USE_METAL)
    const int32_t result = lyraViewAddComponent(
        m_backendContext, m_backendID, componentID(&rectangle),
        rectangle.width, rectangle.height);
    if (result != 0) return std::unexpected(viewErrorFromBackend(result));
#else
    if (std::ranges::any_of(m_components, [&rectangle](const LayoutItem& item) {
            const auto* existing = std::get_if<Rectangle*>(&item.component);
            return existing && *existing == &rectangle;
        }))
        return std::unexpected(ViewError::ComponentAlreadyAdded);
    if (!canFit(rectangle.width, rectangle.height))
        return std::unexpected(ViewError::ComponentDoesNotFit);
#endif

    m_components.push_back({&rectangle, rectangle.width, rectangle.height});
    layoutComponents();
    return {};
}

std::expected<void, ViewError> View::add(TextLabel& text,
                                         float layoutWidth,
                                         float layoutHeight) {
#if defined(LYRA_USE_METAL)
    const int32_t result = lyraViewAddComponent(
        m_backendContext, m_backendID, componentID(&text),
        layoutWidth, layoutHeight);
    if (result != 0) return std::unexpected(viewErrorFromBackend(result));
#else
    if (std::ranges::any_of(m_components, [&text](const LayoutItem& item) {
            const auto* existing = std::get_if<TextLabel*>(&item.component);
            return existing && *existing == &text;
        }))
        return std::unexpected(ViewError::ComponentAlreadyAdded);
    if (!canFit(layoutWidth, layoutHeight))
        return std::unexpected(ViewError::ComponentDoesNotFit);
#endif

    m_components.push_back({&text, layoutWidth, layoutHeight});
    layoutComponents();
    return {};
}

bool View::remove(Rectangle& rectangle) {
    const auto it = std::find_if(m_components.begin(), m_components.end(),
        [&rectangle](const LayoutItem& item) {
            const auto* existing = std::get_if<Rectangle*>(&item.component);
            return existing && *existing == &rectangle;
        });
    if (it == m_components.end()) return false;
#if defined(LYRA_USE_METAL)
    if (lyraViewRemoveComponent(m_backendContext, m_backendID,
                                componentID(&rectangle)) != 0)
        return false;
#endif
    m_components.erase(it);
    layoutComponents();
    return true;
}

bool View::remove(TextLabel& text) {
    const auto it = std::find_if(m_components.begin(), m_components.end(),
        [&text](const LayoutItem& item) {
            const auto* existing = std::get_if<TextLabel*>(&item.component);
            return existing && *existing == &text;
        });
    if (it == m_components.end()) return false;
#if defined(LYRA_USE_METAL)
    if (lyraViewRemoveComponent(m_backendContext, m_backendID,
                                componentID(&text)) != 0)
        return false;
#endif
    m_components.erase(it);
    layoutComponents();
    return true;
}

void View::layoutComponents() {
#if defined(LYRA_USE_METAL)
    for (const LayoutItem& item : m_components) {
        std::visit([&](auto* component) {
            float x = 0.0f;
            float y = 0.0f;
            if (lyraViewGetComponentPosition(
                    m_backendContext, m_backendID, componentID(component),
                    &x, &y) != 0) {
                std::cerr << "View: Swift layout backend could not locate a component\n";
                return;
            }
            using ComponentType = std::remove_pointer_t<decltype(component)>;
            component->x = x;
            if constexpr (std::is_same_v<ComponentType, Rectangle>)
                component->y = y;
            else
                component->baselineY = y + item.height;
        }, item.component);
    }
#else
    float cursorX = m_bounds.x;
    float cursorY = m_bounds.y;
    for (const LayoutItem& item : m_components) {
        std::visit([&](auto* component) {
            using ComponentType = std::remove_pointer_t<decltype(component)>;
            if constexpr (std::is_same_v<ComponentType, Rectangle>) {
                component->x = cursorX;
                component->y = cursorY;
            } else {
                component->x = cursorX;
                component->baselineY = cursorY + item.height;
            }
        }, item.component);

        if (m_orientation == ViewOrientation::Horizontal)
            cursorX += item.width;
        else
            cursorY += item.height;
    }
#endif
}

ViewSpace::ViewSpace() {
#if defined(LYRA_USE_METAL)
    m_backendContext = lyraViewSpaceCreate();
#endif
}

ViewSpace::~ViewSpace() {
#if defined(LYRA_USE_METAL)
    m_views.clear();
    if (m_backendContext) lyraViewSpaceDestroy(m_backendContext);
#endif
}

std::expected<View*, ViewError> ViewSpace::createHorizontalView(
    float x, float y, float width, float height) {
    return createView(ViewOrientation::Horizontal, {x, y, width, height});
}

std::expected<View*, ViewError> ViewSpace::createVerticalView(
    float x, float y, float width, float height) {
    return createView(ViewOrientation::Vertical, {x, y, width, height});
}

std::expected<View*, ViewError> ViewSpace::createView(
    ViewOrientation orientation, ViewBounds bounds) {
#if defined(LYRA_USE_METAL)
    if (!m_backendContext) return std::unexpected(ViewError::BackendUnavailable);
    std::uint64_t id = 0;
    const int32_t result = lyraViewCreate(
        m_backendContext, static_cast<int32_t>(orientation), bounds.x, bounds.y,
        bounds.width, bounds.height, &id);
    if (result != 0) return std::unexpected(viewErrorFromBackend(result));

    auto view = std::unique_ptr<View>(
        new View(orientation, bounds, m_backendContext, id));
    View* resultView = view.get();
    m_views.emplace(id, std::move(view));
    return resultView;
#else
    if (!validBounds(bounds)) return std::unexpected(ViewError::InvalidBounds);
    for (const auto& [id, view] : m_views) {
        (void)id;
        if (overlaps(bounds, view->bounds()))
            return std::unexpected(ViewError::Overlap);
    }

    const std::uint64_t id = m_nextId++;
    auto view = std::unique_ptr<View>(
        new View(orientation, bounds, nullptr, id));
    View* result = view.get();
    m_views.emplace(id, std::move(view));
    return result;
#endif
}

bool ViewSpace::removeView(View* view) {
    if (!view) return false;
    const auto it = std::find_if(m_views.begin(), m_views.end(),
        [view](const auto& entry) { return entry.second.get() == view; });
    if (it == m_views.end()) return false;
#if defined(LYRA_USE_METAL)
    if (lyraViewRemove(m_backendContext, it->first) != 0) return false;
#endif
    m_views.erase(it);
    return true;
}
