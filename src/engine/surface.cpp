#include "surface.hpp"

#include "./window.hpp"
#include "./instance.hpp"
#include "./utils.hpp"

void Surface::init(Instance instance, Window window) {
    vkCheck(glfwCreateWindowSurface(instance.get(), window.get(), nullptr, &surface), "Failed to create window surface");
}

void Surface::destroy(Instance instance) {
    vkDestroySurfaceKHR(instance.get(), surface, nullptr);
}
