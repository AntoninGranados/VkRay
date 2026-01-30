#pragma once

#include <functional>
#include <vector>
#include <unordered_map>

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
#include "../ecs/system_scheduler.hpp"
#include "../ecs/systems/transform_system.hpp"
#include "../ecs/systems/gpu_packing_system.hpp"

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

struct PackingMaps {
    std::unordered_map<ecs::Entity, int> sphereId;
    std::unordered_map<ecs::Entity, int> planeId;
    std::unordered_map<ecs::Entity, int> boxId;
    std::unordered_map<ecs::Entity, int> meshId;
};

class Scene {
public:
    void init();
    void destroy();
    void clear();

    void setPreviewCameraCallback(std::function<void(const CameraHandle&)> callback) {
        previewCameraCallback = std::move(callback);
    }
    void setContext(const AppContext& context) { ctx = &context; }

    MaterialHandle pushMaterial(const Material &mat);
    void pushSphere(std::string name, glm::vec3 center, float radius, MaterialHandle materialHandle = 0);
    void pushPlane(std::string name, glm::vec3 point, glm::vec3 normal, MaterialHandle materialHandle = 0);
    void pushBox(std::string name, glm::vec3 cornerMin, glm::vec3 cornerMax, MaterialHandle materialHandle = 0);
    void pushMesh(std::string name, const std::string &path, const glm::mat4 &transform, MaterialHandle materialHandle = 0);
    void pushMesh(std::string name, MeshHandle meshHandle, const glm::mat4 &transform, MaterialHandle materialHandle = 0);
    void pushCameraHandle(std::string name, glm::vec3 position, glm::vec3 direction, float fov);
    
    void drawGuizmo(const glm::mat4 &view, const glm::mat4 &proj);
    void drawUI();
    void drawNewObjectPopUp();
    void drawSelectedEntityUI();
    void drawSelectedMaterialUI();
    void drawSelectedMeshAssetUI();
    void runSystems(AppContext& ctx) { scheduler.run(registry, ctx); }
    void gpuPacking(AppContext& ctx) { packingScheduler.run(registry, ctx); }
    LightMode loadPreset(ScenePreset preset);
    CameraHandle* getFirstCameraHandle() const;

    const int getSelectionId() { return selectedEntity; }
    void clearSelection() { selectedEntity = -1; }
    const ecs::Entity* getSelectedEntity() const;
    bool raycast(const glm::vec2 &screenPos, const glm::vec2 &screenSize, const Camera &camera, float &dist, glm::vec3 &p, bool select = false, bool includeCameras = true);
    bool containsObject(const Object *object) const;

    std::vector<bufferList_t> getBufferLists();
    ObjectBuffers& getSphereBuffers() { return sphereBuffers; };
    ObjectBuffers& getPlaneBuffers() { return planeBuffers; };
    ObjectBuffers& getBoxBuffers() { return boxBuffers; };
    ObjectBuffers& getVertexBuffers() { return vertexBuffers; };
    ObjectBuffers& getIndexBuffers() { return indexBuffers; };
    ObjectBuffers& getBvhBuffers() { return bvhBuffers; };
    ObjectBuffers& getMeshBuffers() { return meshBuffers; };
    ObjectBuffers& getMaterialBuffers() { return materialBuffers; };
    ObjectBuffers& getObjectBuffers() { return objectBuffers; };
    ObjectBuffers& getLightBuffers() { return lightBuffers; };
    std::vector<Material>& getMaterials() { return materials; };
    std::vector<MeshAsset>& getMeshAssets() { return meshAssets; };
    PackingMaps& getPackingMaps() { return packingMaps; }

    // Returns true if the scene have been updated since the last call of this function
    bool checkUpdate();
    bool checkBufferUpdate();
private:
    ObjectBuffers sphereBuffers, planeBuffers, boxBuffers, vertexBuffers, indexBuffers, bvhBuffers, meshBuffers;
    ObjectBuffers materialBuffers, objectBuffers, lightBuffers;

    PackingMaps packingMaps;
    
    ecs::Registry registry;
    ecs::SystemScheduler scheduler, packingScheduler;
    
    int selectedEntity = -1;
    std::vector<ecs::Entity> entities;
    int entityN = 0;
    
    MaterialHandle selectedMaterial = -1;
    std::vector<Material> materials;
    int materialN = 0;
    
    MeshHandle selectedMeshAsset = -1;
    std::vector<MeshAsset> meshAssets;

    std::vector<Object*> objects;

    bool updated = false;
    bool bufferUpdated = false;

    std::function<void(const CameraHandle&)> previewCameraCallback;
    const AppContext* ctx = nullptr;

    void initSystems();

};
