#include "Menu.hpp"

#include <cassert>
#include <iostream>

int main() {
    Menu menu(20.0f, 30.0f, 180.0f, 44.0f);
    int clicks = 0;

    assert(menu.itemCount() == 0);
    assert(menu.addItem("New") == 0);
    assert(menu.addItem("Open", [&clicks]() { ++clicks; }) == 1);
    assert(menu.itemCount() == 2);

    assert(menu.items()[0].text == "New");
    assert(menu.items()[0].label.text == "New");
    assert(menu.items()[1].background.contains(30.0, 75.0));
    menu.items()[1].background.onClick();
    assert(clicks == 1);

    assert(menu.setItemText(0, "Create"));
    assert(menu.items()[0].label.text == "Create");
    assert(menu.removeItem(0));
    assert(menu.itemCount() == 1);
    assert(menu.items()[0].background.y == 30.0f);
    assert(menu.setItemEnabled(0, false));
    assert(!menu.items()[0].background.enabled);
    assert(menu.setItemVisible(0, false));
    assert(!menu.items()[0].background.visible);
    menu.setLineSpacing(12.0f);
    assert(menu.getLineSpacing() == 12.0f);
    assert(menu.items()[0].spacingBelow.height == 12.0f);
    assert(menu.items()[0].spacingBelow.y == 74.0f);

    menu.clear();
    assert(menu.itemCount() == 0);

    std::cout << "Menu item layout and callbacks passed\n";
    return 0;
}
