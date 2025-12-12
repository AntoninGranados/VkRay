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
#include "../notification.hpp"

class GpuBuffers {
public:
    void init(VkSmol &engine, size_t objectSize, size_t baseSize = 0);
    void destroy(VkSmol &engine);

    void addElement(VkSmol &engine);
    void removeElement();
    void resize(VkSmol &engine);

    size_t getCapacity() { return capacity; }
    size_t getCount() { return count; }
    bufferList_t getBufferList() { return bufferList; }

private:
    bufferList_t bufferList;
    size_t capacity;
    size_t count;

    size_t objectSize;
    size_t baseSize;
};

class Scene {
public:
    void init(VkSmol &engine);
    void destroy(VkSmol &engine);
    void setMessageCallback(void (*messageCallback_)(NotificationType, std::string)) {
        messageCallback = messageCallback_;
    }

    // The engine is needed in case we have to resize a buffer
    void pushSphere(VkSmol &engine, std::string name, glm::vec3 center, float radius, Material mat);
    void pushPlane(VkSmol &engine, std::string name, glm::vec3 point, glm::vec3 normal, Material mat);
    void pushBox(VkSmol &engine, std::string name, glm::vec3 cornerMin, glm::vec3 cornerMax, Material mat);

    void fillBuffers(VkSmol &engine);
    
    void drawGuizmo(const glm::mat4 &view, const glm::mat4 &proj);
    void drawUI(VkSmol &engine);   // The engine is needed in case we have to resize a buffer
    void drawNewObjectPopUp(VkSmol &engine);
    void drawSelectedUI(VkSmol &engine);

    void clearSelection() { selectedObjectId = -1; }
    bool raycast(const glm::vec2 &screenPos, const glm::vec2 &screenSize, const Camera &camera);

    std::vector<bufferList_t> getBufferLists();

    // Returns true if the scene have been updated this the last call of this function
    bool wasUpdated();

private:
    GpuBuffers sphereBuffers, planeBuffers, boxBuffers, objectBuffers;
    
    int selectedObjectId = -1;
    int objectId = 0;   // Used for unique object naming
    std::vector<Object*> objects;

    bool updated = false;

    void (*messageCallback)(NotificationType, std::string) = nullptr;
};
