// InputController.hpp
//
// Translates GLFW keyboard/mouse input into RubiksCube turn requests and
// Camera updates.
//
// KEY BINDINGS (documented here and nowhere else, per Part 5):
//   Face turns (clockwise, uppercase letters as physical keys):
//     U -> top face        D -> bottom face
//     L -> left face       R -> right face
//     F -> front face      B -> back face
//   Prime (counter-clockwise) variants: hold LEFT SHIFT while pressing the
//   same key (e.g. Shift+R = R').
//
//   Camera orbit: left-click-drag anywhere on the window orbits the
//   camera around the cube. This is intentionally NOT tied to any
//   modifier key, since face turns are entirely keyboard-driven in this
//   baseline version (no click-to-turn-a-face picking is implemented --
//   that is called out in the spec as a stretch goal). Mouse scroll
//   zooms the camera in/out.
//
// While a face turn is animating, additional turn key-presses are ignored
// (RubiksCube::requestTurn() already enforces this; InputController simply
// forwards every key-press attempt and lets RubiksCube decide).

#pragma once

#include <GLFW/glfw3.h>

#include "Camera.hpp"
#include "RubiksCube.hpp"

namespace qrcube {

class InputController {
public:
    InputController(GLFWwindow* window, Camera& camera, RubiksCube& cube);

    // Call once per frame; not strictly required since most logic is
    // event-driven via GLFW callbacks, but kept for symmetry/extensibility.
    void update();

private:
    GLFWwindow* window_;
    Camera& camera_;
    RubiksCube& cube_;

    bool dragging_ = false;
    double lastMouseX_ = 0.0;
    double lastMouseY_ = 0.0;

    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    static void cursorPosCallback(GLFWwindow* window, double x, double y);
    static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);

    void handleKey(int key, int action, int mods);
    void handleMouseButton(int button, int action, int mods);
    void handleCursorPos(double x, double y);
    void handleScroll(double xoffset, double yoffset);
};

} // namespace qrcube
