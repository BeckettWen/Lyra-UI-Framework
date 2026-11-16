#include "View.hpp"

#include <cassert>
#include <cmath>
#include <iostream>

namespace {
bool near(float a, float b) {
    return std::fabs(a - b) < 0.001f;
}
}

int main() {
    ViewSpace space;
    auto horizontalResult = space.createHorizontalView(10.0f, 20.0f, 120.0f, 60.0f);
    assert(horizontalResult);
    View* horizontal = *horizontalResult;

    auto overlap = space.createVerticalView(100.0f, 30.0f, 50.0f, 50.0f);
    assert(!overlap && overlap.error() == ViewError::Overlap);
    auto touching = space.createVerticalView(130.0f, 20.0f, 50.0f, 50.0f);
    assert(touching);

    Rectangle first(0.0f, 0.0f, 30.0f, 20.0f, {1, 0, 0, 1});
    Rectangle second(0.0f, 0.0f, 40.0f, 25.0f, {0, 1, 0, 1});
    TextLabel label{"label"};
    assert(horizontal->add(first));
    assert(horizontal->add(second));
    assert(horizontal->add(label, 40.0f, 20.0f));
    assert(near(first.x, 10.0f) && near(first.y, 20.0f));
    assert(near(second.x, 40.0f) && near(second.y, 20.0f));
    assert(near(label.x, 80.0f) && near(label.baselineY, 40.0f));

    auto duplicate = horizontal->add(first);
    assert(!duplicate && duplicate.error() == ViewError::ComponentAlreadyAdded);
    Rectangle tooLargeRectangle(0.0f, 0.0f, 20.0f, 20.0f, {1, 1, 1, 1});
    auto tooLarge = horizontal->add(tooLargeRectangle);
    assert(!tooLarge && tooLarge.error() == ViewError::ComponentDoesNotFit);

    assert(horizontal->remove(second));
    assert(near(label.x, 40.0f));
    assert(space.removeView(*touching));
    auto reused = space.createVerticalView(130.0f, 20.0f, 50.0f, 50.0f);
    assert(reused);

    std::cout << "View collision handling and layout passed\n";
    return 0;
}
