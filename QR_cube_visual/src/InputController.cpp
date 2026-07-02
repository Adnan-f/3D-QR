// InputController.cpp
#include "InputController.hpp"

#include <cstdio>
#include <string>

namespace qrcube {

namespace {
// Maps a GLFW key to its base (non-primed) move name.
const char* keyToMove(int key) {
    switch (key) {
        case GLFW_KEY_U: return "U";
        case GLFW_KEY_D: return "D";
        case GLFW_KEY_L: return "L";
        case GLFW_KEY_R: return "R";
        case GLFW_KEY_F: return "F";
        case GLFW_KEY_B: return "B";
        default: return nullptr;
    }
}
} // namespace

InputController::InputController(GLFWwindow* window, Camera& camera, RubiksCube& cube)
    : window_(window), camera_(camera), cube_(cube) {
    glfwSetWindowUserPointer(window_, this);
    glfwSetKeyCallback(window_, keyCallback);
    glfwSetMouseButtonCallback(window_, mouseButtonCallback);
    glfwSetCursorPosCallback(window_, cursorPosCallback);
    glfwSetScrollCallback(window_, scrollCallback);
}

void InputController::update() {
    // Event-driven; nothing to poll per-frame currently.
}

void InputController::keyCallback(GLFWwindow* window, int key, int /*scancode*/, int action, int mods) {
    auto* self = static_cast<InputController*>(glfwGetWindowUserPointer(window));
    if (self) self->handleKey(key, action, mods);
}

void InputController::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    auto* self = static_cast<InputController*>(glfwGetWindowUserPointer(window));
    if (self) self->handleMouseButton(button, action, mods);
}

void InputController::cursorPosCallback(GLFWwindow* window, double x, double y) {
    auto* self = static_cast<InputController*>(glfwGetWindowUserPointer(window));
    if (self) self->handleCursorPos(x, y);
}

void InputController::scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    auto* self = static_cast<InputController*>(glfwGetWindowUserPointer(window));
    if (self) self->handleScroll(xoffset, yoffset);
}

void InputController::handleKey(int key, int action, int mods) {
    if (action != GLFW_PRESS && action != GLFW_REPEAT) return;
    if (action == GLFW_REPEAT) return; // only trigger on fresh key-down

    if (key == GLFW_KEY_ESCAPE) {
        glfwSetWindowShouldClose(window_, GLFW_TRUE);
        return;
    }

    const char* base = keyToMove(key);
    if (!base) return;

    bool prime = (mods & GLFW_MOD_SHIFT) != 0;
    std::string moveName = base;
    if (prime) moveName += "'";

    auto req = lookupMove(moveName);
    if (!req.has_value()) {
        std::fprintf(stderr, "InputController: unknown move '%s'\n", moveName.c_str());
        return;
    }
    bool started = cube_.requestTurn(*req);
    if (!started) {
        // A turn is already animating; per documented policy we simply
        // drop this input rather than queueing it.
    }
}

void InputController::handleMouseButton(int button, int action, int /*mods*/) {
    if (button != GLFW_MOUSE_BUTTON_LEFT) return;
    if (action == GLFW_PRESS) {
        dragging_ = true;
        glfwGetCursorPos(window_, &lastMouseX_, &lastMouseY_);
    } else if (action == GLFW_RELEASE) {
        dragging_ = false;
    }
}

void InputController::handleCursorPos(double x, double y) {
    if (!dragging_) return;
    double dx = x - lastMouseX_;
    double dy = y - lastMouseY_;
    lastMouseX_ = x;
    lastMouseY_ = y;

    const float orbitSpeed = 0.25f; // degrees per pixel of drag
    camera_.orbit(static_cast<float>(-dx) * orbitSpeed, static_cast<float>(-dy) * orbitSpeed);
}

void InputController::handleScroll(double /*xoffset*/, double yoffset) {
    const float zoomSpeed = 0.5f; // distance units per scroll notch
    camera_.zoom(static_cast<float>(yoffset) * zoomSpeed);
}

} // namespace qrcube
