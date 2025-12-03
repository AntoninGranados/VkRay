#pragma once

#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "objects.hpp"
#include "../engine/engine.hpp"
#include "../imgui/ImGuizmo.h"

#define MAX_CAPACITY 8  // TODO: realloc buffer instead of using a fixed max capacity

class Scene {
public:
    void init(VkSmol &engine);
    void destroy(VkSmol &engine);

    void pushSphere(Sphere sphere, std::string name = "No Name");
    void fillBuffer(VkSmol &engine);
    
    void sphereUI(int &frameCount, const int &sphereId);
    void drawInformationUI(int &frameCount);
    void drawSelectedUI(int &frameCount);

    Sphere* getSelectedSphere();
    int getSelectedSphereId() { return selectedSphereId; }
    int getSpheresCount() { return spheres.size(); }
    std::vector<Sphere> getSpheres() { return spheres; }

    bufferList_t getBufferList() { return storageBuffers; }

    int selectedSphereId = -1;
private:
    bufferList_t storageBuffers;
    
    std::vector<Sphere> spheres;
    std::vector<std::string> sphereNames;
};
