#pragma once

#include <vector>
#include <unordered_map>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "../engine/engine.hpp"
#include "../camera.hpp"

#include "object/object_buffers.hpp"
#include "object/object.hpp"
#include "asset/mesh.hpp"
#include "raycast.hpp"

#include "../ecs/registry.hpp"
#include "../ecs/entity.hpp"
#include "../ecs/component_ui_registry.hpp"
#include "../ecs/system_scheduler.hpp"

struct AppContext;

enum LightMode : int {
    Day,
    Sunset,
    Night,
    Empty,
};

enum class ScenePreset : int;
class SceneEditorUI;

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

    void setContext(AppContext& context) { ctx = &context; }

    MaterialHandle pushMaterial(const Material &mat);
    void pushSphere(std::string name, glm::vec3 center, float radius, MaterialHandle materialHandle = 0);
    void pushPlane(std::string name, glm::vec3 point, glm::vec3 normal, MaterialHandle materialHandle = 0);
    void pushBox(std::string name, glm::vec3 cornerMin, glm::vec3 cornerMax, MaterialHandle materialHandle = 0);
    void pushMesh(std::string name, const std::string &path, const glm::mat4 &transform, MaterialHandle materialHandle = 0);
    void pushMesh(std::string name, MeshHandle meshHandle, const glm::mat4 &transform, MaterialHandle materialHandle = 0);
    void pushCamera(std::string name, const glm::mat4 &transform);
    
    void drawGuizmo(const glm::mat4 &view, const glm::mat4 &proj);
    void drawUI();
    void drawNewObjectPopUp();
    void drawSelectedEntityUI();
    void drawSelectedMaterialUI();
    void drawSelectedMeshAssetUI();
    void bakePhysics();
    bool isPhysicsBakeInProgress() const;
    int getPhysicsBakeCurrentFrame() const;
    int getPhysicsBakeTotalFrames() const;
    
    bool raycast(const glm::vec2 &screenPos, const glm::vec2 &screenSize, float &dist, glm::vec3 &p, bool select = false, bool includeCameras = true);
    
    void runPreUpdate(AppContext& ctx) { preUpdateScheduler.run(registry, ctx); }
    void runOnRender(AppContext& ctx) { onRenderScheduler.run(registry, ctx); }
    void runOnUi(AppContext& ctx) { onUiScheduler.run(registry, ctx); }
    void runPostUpdate(AppContext& ctx) { postUpdateScheduler.run(registry, ctx); }
    
    const int getSelectionId() { return selectedEntity; }
    const ecs::Entity* getSelectedEntity() const;
    void clearSelection() { selectedEntity = -1; }
    ecs::Registry& getRegistry() { return registry; }
    const ecs::Registry& getRegistry() const { return registry; }
    Camera& getCamera() { return camera; }
    bool isPreviewingCamera();
    
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
    void markBufferUpdated() { bufferUpdated = true; }

    // Returns true if the scene have been updated since the last call of this function
    bool checkUpdate();
    bool checkBufferUpdate();
private:
    friend class SceneEditorUI;

    ObjectBuffers sphereBuffers, planeBuffers, boxBuffers, vertexBuffers, indexBuffers, bvhBuffers, meshBuffers;
    ObjectBuffers materialBuffers, objectBuffers, lightBuffers;

    PackingMaps packingMaps;
    
    ecs::Registry registry;
    ecs::SystemScheduler preUpdateScheduler, onRenderScheduler, onUiScheduler, postUpdateScheduler;
    
    int selectedEntity = -1;
    std::vector<ecs::Entity> entities;
    int entityN = 0;
    
    MaterialHandle selectedMaterial = -1;
    std::vector<Material> materials;
    int materialN = 0;
    
    MeshHandle selectedMeshAsset = -1;
    std::vector<MeshAsset> meshAssets;

    Camera camera = Camera(glm::vec3(0.0f, 0.0f, -10.0f));

    bool updated = false;
    bool bufferUpdated = false;

    AppContext* ctx = nullptr;

    void initSystems();
    void initGpuBuffers(VkSmol& engine);
    void clearGpuBuffers(VkSmol& engine);
    void destroyGpuBuffers(VkSmol& engine);

    ecs::Entity createNamedEntity(std::string name);
    void addMaterialRef(ecs::Entity e, MaterialHandle materialHandle);
    ecs::Transform makeTransformFromMatrix(const glm::mat4& transform) const;
    void resetSceneState();
    void ensureDefaultAssets();

};
