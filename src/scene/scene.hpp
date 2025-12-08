#pragma once

#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "../engine/engine.hpp"
#include "../camera.hpp"
#include "../imgui/ImGuizmo.h"
#include "../imgui/imgui.h"

#include "object/object.hpp"
#include "object/sphere.hpp"
#include "object/plane.hpp"
#include "object/box.hpp"

#define MAX_CAPACITY 8  // TODO: realloc buffer instead of using a fixed max capacity

class Scene {
public:
    void init(VkSmol &engine);
    void destroy(VkSmol &engine);

    void pushSphere(std::string name, glm::vec3 center, float radius, Material mat);
    void pushPlane(std::string name, glm::vec3 point, glm::vec3 normal, Material mat);
    void pushBox(std::string name, glm::vec3 cornerMin, glm::vec3 cornerMax, Material mat);
    void fillBuffers(VkSmol &engine);
    
    void drawGuizmo(const glm::mat4 &view, const glm::mat4 &proj);
    void drawNewObjectUI();
    void drawSelectedUI();

    void clearSelection() { selectedObjectId = -1; }
    bool raycast(const glm::vec2 &screenPos, const glm::vec2 &screenSize, const Camera &camera);

    std::vector<bufferList_t> getBufferLists() { return { sphereBuffers, planeBuffers, boxBuffers, objectBuffers }; }

    // Returns true if the scene have been updated this the last call of this function
    bool wasUpdated();

private:
    bufferList_t sphereBuffers, planeBuffers, boxBuffers, objectBuffers;
    size_t sphereBuffersCapacity = 8, planeBuffersCapacity = 8, boxBuffersCapacity = 8, objectBuffersCapacity = 8;
    size_t sphereBuffersSize = 0, planeBuffersSize = 0, boxBuffersSize = 0, objectBuffersSize = 0;
    
    int selectedObjectId = -1;
    std::vector<Object*> objects;

    bool updated = false;
};
