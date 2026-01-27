#pragma once

#include <functional>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "../engine/engine.hpp"
#include "../camera.hpp"

#include "object/object_buffers.hpp"
#include "object/object.hpp"
#include "asset/mesh.hpp"
#include "camera_handle.hpp"
#include "raycast.hpp"

#include "../ecs/registry.hpp"
#include "../ecs/entity.hpp"
#include "../ecs/components.hpp"
#include "../ecs/component_ui_registry.hpp"

#include "imgui/ImGuizmo.h"
#include "imgui/imgui.h"

struct AppContext;

enum LightMode : int {
    Day,
    Sunset,
    Night,
    Empty,
};

enum class ScenePreset : int;

class Scene {
public:
    void init();
    void destroy();
    void clear();

    void setPreviewCameraCallback(std::function<void(const CameraHandle&)> callback) {
        previewCameraCallback = std::move(callback);
    }
    void setContext(const AppContext& context) { ctx = &context; }

    // The engine is needed in case we have to resize a buffer
    MaterialHandle pushMaterial(const Material &mat);
    void pushSphere(std::string name, glm::vec3 center, float radius, MaterialHandle materialHandle);
    void pushPlane(std::string name, glm::vec3 point, glm::vec3 normal, MaterialHandle materialHandle);
    void pushBox(std::string name, glm::vec3 cornerMin, glm::vec3 cornerMax, MaterialHandle materialHandle);
    void pushMesh(std::string name, const std::string &path, const glm::mat4 &transform, MaterialHandle materialHandle);
    void pushCameraHandle(std::string name, glm::vec3 position, glm::vec3 direction, float fov);

    void fillBuffers();
    
    void drawGuizmo(const glm::mat4 &view, const glm::mat4 &proj);
    void drawUI();
    void drawNewObjectPopUp();
    void drawSelectedMaterialUI();
    void drawSelectedEntityUI();
    LightMode loadPreset(ScenePreset preset);
    CameraHandle* getFirstCameraHandle() const;

    void clearSelection() { selectedEntity = -1; }
    bool raycast(const glm::vec2 &screenPos, const glm::vec2 &screenSize, const Camera &camera, float &dist, glm::vec3 &p, bool select = false, bool includeCameras = true);
    bool containsObject(const Object *object) const;

    std::vector<bufferList_t> getBufferLists();

    // Returns true if the scene have been updated since the last call of this function
    bool checkUpdate();
    bool checkBufferUpdate();
private:
    ObjectBuffers sphereBuffers, planeBuffers, boxBuffers, vertexBuffers, indexBuffers, bvhBuffers, meshBuffers;
    ObjectBuffers materialBuffers, objectBuffers, lightBuffers;
    
    int selectedEntity = -1;
    MaterialHandle selectedMaterial = -1;
    ecs::Registry registry;
    std::vector<ecs::Entity> entities;
    int entityN = 0;
    std::vector<Material> materials;
    int materialN = 0;
    std::vector<MeshAsset> meshAssets;

    std::vector<Object*> objects;

    bool updated = false;
    bool bufferUpdated = false;

    std::function<void(const CameraHandle&)> previewCameraCallback;
    const AppContext* ctx = nullptr;

};
