#pragma once

#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "../engine/engine.hpp"
#include "../camera.hpp"
#include "../notification.hpp"

#include "object/object_buffers.hpp"
#include "object/object.hpp"
#include "object/sphere.hpp"
#include "object/plane.hpp"
#include "object/box.hpp"

#include "imgui/ImGuizmo.h"
#include "imgui/imgui.h"

enum LightMode : int {
    Day,
    Sunset,
    Night,
    Empty,
};

class Scene {
public:
    void init(VkSmol &engine);
    void destroy(VkSmol &engine);
    void clear(VkSmol &engine);

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
    bool raycast(const glm::vec2 &screenPos, const glm::vec2 &screenSize, const Camera &camera, float &dist, glm::vec3 &p, bool select = false);

    std::vector<bufferList_t> getBufferLists();

    // Returns true if the scene have been updated since the last call of this function
    bool checkUpdate();
    bool checkBufferUpdate();

private:
    ObjectBuffers sphereBuffers, planeBuffers, boxBuffers, materialBuffers, objectBuffers, lightBuffers;
    
    int selectedObjectId = -1;
    int objectId = 0;   // Used for unique object naming
    std::vector<Object*> objects;
    std::vector<Material> materials;

    bool updated = false;
    bool bufferUpdated = false;

    void (*messageCallback)(NotificationType, std::string) = nullptr;
};
