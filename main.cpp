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

    // --- Load textures from project assets (PPM P6) ---
    // These are genuinely loaded at runtime via the PPM loader, not baked
    // into vertex colors.
    TextureRecord texRed = loadPPM("Assets/checkerboard.ppm");
    TextureRecord texGrad = loadPPM("Assets/gradient.ppm");

    int layerRed = -1;
    int layerGrad = -1;

    if (texRed.valid()) {
        layerRed = renderer.addTexture(std::move(texRed));
        std::cout << "Demo: checkerboard texture -> layer " << layerRed << "\n";
    } else {
        std::cerr << "Demo: failed to load checkerboard.ppm\n";
    }

    if (texGrad.valid()) {
        layerGrad = renderer.addTexture(std::move(texGrad));
        std::cout << "Demo: gradient texture -> layer " << layerGrad << "\n";
    } else {
        std::cerr << "Demo: failed to load gradient.ppm\n";
    }

    // Upload textures to the selected platform renderer before drawing.
    if (!renderer.uploadTextures()) {
        std::cerr << "Demo: failed to upload textures to the renderer\n";
        renderer.destroy();
        glfwTerminate();
        return 1;
    }

    // --- Rectangles (screen/logical coords, top-left origin) ---
    // Overlap rule: latest-added (topmost) wins clicks.
    std::vector<Rectangle> rects;
    rects.reserve(5);

    // Solid blue rectangle (no texture, layer -1), in the upper-left demo zone.
    rects.emplace_back(50.0f, 150.0f, 160.0f, 100.0f,
                       std::array<float, 4>{0.2f, 0.45f, 1.0f, 1.0f});
    rects.back().textureLayer = -1;
    rects.back().onClick = []() {
        std::cout << "click: solid blue rectangle\n";
    };

    // Blue-tinted checkerboard rectangle (layer 0).
    rects.emplace_back(280.0f, 150.0f, 180.0f, 140.0f,
                       std::array<float, 4>{0.45f, 0.7f, 1.0f, 1.0f});
    rects.back().textureLayer = layerRed;
    rects.back().onClick = []() {
        std::cout << "click: checkerboard rectangle\n";
    };

    // Shaded gradient rectangle (layer 1), in its own lower-right zone.
    rects.emplace_back(520.0f, 300.0f, 220.0f, 170.0f,
                       std::array<float, 4>{0.5f, 0.72f, 1.0f, 0.9f});
    rects.back().textureLayer = layerGrad;
    rects.back().onClick = []() {
        std::cout << "click: shaded gradient rectangle\n";
    };

    // Solid blue rectangle (no texture), reserved as the animation zone.
    rects.emplace_back(50.0f, 300.0f, 150.0f, 130.0f,
                       std::array<float, 4>{0.2f, 0.45f, 1.0f, 1.0f});
    rects.back().textureLayer = -1;
    rects.back().onClick = []() {
        std::cout << "click: animated blue rectangle\n";
    };

    // Translucent blue checkerboard glass over the first checkerboard. The
    // backdrop is blurred within this component's bounds; the checker texture
    // itself remains sharp.
    rects.emplace_back(340.0f, 180.0f, 160.0f, 120.0f,
                       std::array<float, 4>{0.2f, 0.45f, 1.0f, 0.78f});
    rects.back().textureLayer = layerRed;
    rects.back().backdropBlurEnabled = true;
    rects.back().backdropBlurRadius = 12.0f;
    rects.back().onClick = []() {
        std::cout << "click: tinted checkerboard rectangle\n";
    };

    // The blue rectangle follows a cubic Bezier path confined to the
    // lower-left zone (repeatCount 0 = forever).
    if (!rects[3].animateBezierTo(50.0f, 430.0f,
                                  80.0f, 300.0f,
                                  80.0f, 430.0f,
                                  3.0f, 0))
        std::cerr << "Demo: failed to start Bezier rectangle animation\n";

    ViewSpace viewSpace;
    Rectangle verticalViewRectangle(
        0.0f, 0.0f, 180.0f, 55.0f, {0.25f, 0.55f, 0.85f, 1.0f});
    TextLabel verticalViewLabel{"VIEW", 0.0f, 0.0f, 0.5f,
                                {1.0f, 1.0f, 1.0f, 1.0f}};
    Rectangle horizontalViewFirst(
        0.0f, 0.0f, 160.0f, 55.0f, {0.35f, 0.65f, 0.95f, 1.0f});
    Rectangle horizontalViewSecond(
        0.0f, 0.0f, 220.0f, 55.0f, {0.25f, 0.5f, 0.8f, 1.0f});

    auto verticalView = viewSpace.createVerticalView(520.0f, 90.0f, 220.0f, 190.0f);
    auto horizontalView =
        viewSpace.createHorizontalView(230.0f, 500.0f, 520.0f, 75.0f);
    if (!verticalView || !horizontalView) {
        std::cerr << "Demo: failed to create non-overlapping views\n";
    } else {
        const auto rectangleAdded = (*verticalView)->add(verticalViewRectangle);
        const auto labelAdded = (*verticalView)->add(verticalViewLabel, 180.0f, 50.0f);
        const auto firstAdded = (*horizontalView)->add(horizontalViewFirst);
        const auto secondAdded = (*horizontalView)->add(horizontalViewSecond);
        if (!rectangleAdded || !labelAdded || !firstAdded || !secondAdded) {
            std::cerr << "Demo: a view component did not fit its container\n";
        } else {
            renderer.addRectangle(&verticalViewRectangle);
            renderer.addText(&verticalViewLabel);
            renderer.addRectangle(&horizontalViewFirst);
            renderer.addRectangle(&horizontalViewSecond);
        }
    }

    Menu demoMenu(40.0f, 470.0f, 170.0f, 42.0f);
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

    // The renderer stores non-owning pointers; rects must outlive renderer.
    for (auto& r : rects)
        renderer.addRectangle(&r);

    TextBox textBox(280.0f, 55.0f, 260.0f, 44.0f);
    TextBox searchBox(280.0f, 105.0f, 260.0f, 44.0f);
    textBox.setText("");
    textBox.setMaxCharacters(64);
    searchBox.setText("Type here");
    searchBox.setMaxCharacters(64);
    renderer.addTextBox(&textBox);
    renderer.addTextBox(&searchBox);

    // Text uses an independent glyph texture array, descriptor set, pipeline,
    // and vertex/index batch. Coordinates are top-left-origin; y is the font
    // baseline in logical window pixels.
    std::vector<TextLabel> labels;
    labels.reserve(6);
    labels.push_back({"Cloudsdale", 50.0f, 95.0f, 0.8f,
                      {1.0f, 1.0f, 1.0f, 1.0f}, true});
    labels.push_back({"LYRA", 65.0f, 215.0f, 0.42f,
                      {1.0f, 1.0f, 1.0f, 1.0f}, true});
    labels.push_back({"CHECKER", 295.0f, 225.0f, 0.36f,
                      {1.0f, 1.0f, 1.0f, 1.0f}, true});
    labels.back().backdropBlurEnabled = true;
    labels.back().backdropBlurRadius = 5.0f;
    labels.push_back({"SHADED", 545.0f, 390.0f, 0.5f,
                      {1.0f, 1.0f, 1.0f, 1.0f}, true});
    labels.push_back({"TEXT", 180.0f, 88.0f, 0.28f,
                      {0.85f, 0.85f, 0.85f, 1.0f}, true});
    labels.push_back({"SEARCH", 160.0f, 138.0f, 0.28f,
                      {0.85f, 0.85f, 0.85f, 1.0f}, true});
    for (auto& label : labels)
        renderer.addText(&label);

    while (!window.shouldClose()) {
        window.pollEvents();
        if (!renderer.drawFrame()) break;
    }

    renderer.destroy();
    glfwTerminate();
    return 0;
}
