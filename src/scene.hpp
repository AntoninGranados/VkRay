#pragma once

#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "./engine/engine.hpp"
#include "./imgui/ImGuizmo.h"

enum MaterialType {
    lambertian = 0,
    metal,
    dielectric,
    emissive,
    animated,
};

struct Material {
    MaterialType type;
    alignas(16) glm::vec3 albedo;
    float fuzz;
    float refraction_index;
    float intensity;
};

struct Sphere {
    alignas(16) glm::vec3 center;
    float radius;
    Material mat;
};

#define MAX_CAPACITY 8  // TODO: realloc buffer instead of using a fixed max capacity

class Scene {
public:
    void init(VkSmol &engine);
    void destroy(VkSmol &engine);

    void pushSphere(Sphere sphere, std::string name = "");
    void fillBuffer(VkSmol &engine);
    
    void sphereUI(int &frameCount, Sphere &sphere);
    void drawUI(int &frameCount);

    Sphere* getSelectedSphere();
    int getSelectedSphereId() { return selectedSphereId; }
    int getSpheresCount() { return spheres.size(); }

    bufferList_t getBufferList() { return storageBuffers; }

    private:
    bufferList_t storageBuffers;
    
    std::vector<Sphere> spheres;
    std::vector<std::string> sphereNames;
    int selectedSphereId = -1;
};
