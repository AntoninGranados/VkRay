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
    void pushBox(Box box, std::string name = "No Name");
    void fillBuffers(VkSmol &engine);
    
    void drawSphereGuizmo(int &frameCount, const glm::mat4 &view, const glm::mat4 &proj, const int &sphereId);
    void drawBoxGuizmo(int &frameCount, const glm::mat4 &view, const glm::mat4 &proj, const int &boxId);
    void drawGuizmo(int &frameCount, const glm::mat4 &view, const glm::mat4 &proj);

    void drawMaterialUI(int &frameCount, Material &mat);
    void drawSphereUI(int &frameCount, const int &sphereId);
    void drawBoxUI(int &frameCount, const int &boxId);
    void drawInformationUI(int &frameCount);
    void drawSelectedUI(int &frameCount);

    Object* getSelectedObject();
    void setSelectedObject(int id) { selectedObjectId = id; };
    int getSelectedObjectId() { return selectedObjectId; }
    int getSpheresCount() { return spheres.size(); }
    const std::vector<Sphere>& getSpheres() const { return spheres; }
    const std::vector<Plane>& getPlanes() const { return planes; }
    const std::vector<Box>& getBoxes() const { return boxes; }
    const std::vector<Object>& getObjects() const { return objects; }

    std::vector<bufferList_t> getBufferLists() { return { sphereBuffers, boxBuffers, objectBuffers }; }

private:
    bufferList_t sphereBuffers, boxBuffers, objectBuffers;
    
    std::vector<Sphere> spheres;
    std::vector<std::string> sphereNames;
    std::vector<Plane> planes;
    std::vector<std::string> planeNames;
    std::vector<Box> boxes;
    std::vector<std::string> boxNames;
    
    int selectedObjectId = -1;
    std::vector<Object> objects;
};
