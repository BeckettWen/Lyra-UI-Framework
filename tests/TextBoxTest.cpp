#include "TextBox.hpp"

#include <cassert>
#include <string>

int main() {
    TextBox textBox(100.0f, 80.0f, 240.0f, 48.0f);
    textBox.setFocused(true);
    textBox.appendCodepoint('A');
    textBox.appendCodepoint('Z');
    assert(textBox.get_textbox_value() == std::vector<char>(
        {'A', 'Z'}));
    assert(textBox.label.text == "AZ");

    textBox.backspace();
    assert(textBox.label.text == "A");
    assert(textBox.outline.size() == 4);
    assert(textBox.contains(120.0, 100.0));

    textBox.setText("Hello");
    assert(textBox.get_textbox_value().size() == 5);
    textBox.clear();
    assert(textBox.get_textbox_value().empty());
    return 0;
}
