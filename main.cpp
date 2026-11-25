// Lyra Phase 3 demo — native Metal on macOS, Vulkan elsewhere.
//
// Displays solid-color and textured rectangles as indexed batch draws,
// loads PPM P6 textures from project assets at runtime, and dispatches
// click callbacks. Top-left-origin coordinates; latest-added rectangle is
// topmost for both drawing and click hit-testing.
//
// Build: ./run.sh (macOS) or configure CMake for the platform backend.
// Run (from project root, so Shaders/ and Assets/ are found):
//   ./run.sh

#include "Window.hpp"
#include "Version.hpp"
#include "Renderer.hpp"
#include "Rectangle.hpp"
#include "Texture.hpp"
#include "Text.hpp"
#include "TextBox.hpp"
#include "View.hpp"
#include "Menu.hpp"

#include <iostream>
#include <memory>
#include <vector>

#ifndef SHADER_PATH
#define SHADER_PATH "Shaders"
#endif

int main(int argc, char** argv) {
    bool textboxDemo = false;
    bool blurDemo = false;
    for (int i = 1; i < argc; ++i) {
        const std::string argument = argv[i];
        if (argument == "--textboxes" || argument == "--textbox-demo")
            textboxDemo = true;
        if (argument == "--blur-demo")
            blurDemo = true;
    }

    std::cout << lyra::version::projectName << " " << lyra::version::string
              << " (" << lyra::version::codename << ")\n";
    if (textboxDemo)
        std::cout << "Demo: click a text box, type text, or press Backspace.\n";
    if (blurDemo)
        std::cout << "Demo: translucent components and text demonstrate backdrop blur.\n";
    if (!glfwInit()) {
        std::cerr << "Demo: failed to initialize GLFW\n";
        return 1;
    }

    const int winW = 800;
    const int winH = 600;

    Window window;
    const std::string windowTitle =
        std::string(lyra::version::projectName) + " " + lyra::version::string +
        " (" + lyra::version::codename + ") — Animation and Input";
    if (!window.create(winW, winH, windowTitle.c_str())) {
        std::cerr << "Demo: failed to create GLFW window\n";
        glfwTerminate();
        return 1;
    }

    Renderer renderer;
    if (!renderer.init(window.handle(),
                       window.fbWidth(), window.fbHeight(),
                       window.width(), window.height(),
                       SHADER_PATH)) {
        std::cerr << "Demo: failed to initialize the renderer\n";
        renderer.destroy();
        glfwTerminate();
        return 1;
    }

    window.setResizeCallback([&renderer](int fbw, int fbh, int ww, int wh) {
        renderer.framebufferResized(fbw, fbh, ww, wh);
    });
    window.setMouseButtonCallback(
        [&renderer](int button, int action, int mods, double x, double y) {
            renderer.dispatchMouseButton(button, action, mods, x, y);
        });
    window.setCharCallback(
        [&renderer](unsigned int codepoint) {
            renderer.dispatchChar(codepoint);
        });
    window.setKeyCallback(
        [&renderer](int key, int scancode, int action, int mods) {
            renderer.dispatchKey(key, scancode, action, mods);
        });

    Menu demoMenu(315.0f, 180.0f, 170.0f, 42.0f);
    demoMenu.setLineSpacing(4.0f);
    demoMenu.addItem("New", []() {
        std::cout << "menu: New\n";
    });
    demoMenu.addItem("Open", []() {
        std::cout << "menu: Open\n";
    });
    demoMenu.addItem("Save", []() {
        std::cout << "menu: Save\n";
    });
    renderer.addMenu(&demoMenu);

    if (!demoMenu.animateBezierTo(
            315.0f, 330.0f,
            500.0f, 150.0f,
            500.0f, 360.0f,
            4.0f, 0)) {
        std::cerr << "Demo: failed to start menu animation\n";
    }

    if (!renderer.uploadTextures()) {
        std::cerr << "Demo: failed to initialize the renderer texture state\n";
        renderer.destroy();
        glfwTerminate();
        return 1;
    }

    while (!window.shouldClose()) {
        window.pollEvents();
        if (!renderer.drawFrame()) break;
    }

    renderer.destroy();
    glfwTerminate();
    return 0;
}
