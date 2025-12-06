#include "scene.hpp"

void Scene::init(VkSmol &engine) {
    sphereBuffers = engine.initBufferList(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, sizeof(Sphere) * MAX_CAPACITY);
    boxBuffers = engine.initBufferList(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, sizeof(Box) * MAX_CAPACITY);
    planeBuffers = engine.initBufferList(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, sizeof(Plane) * MAX_CAPACITY);
    objectBuffers = engine.initBufferList(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, sizeof(unsigned int) + sizeof(int) + sizeof(Object) * MAX_CAPACITY * 2);
}

void Scene::destroy(VkSmol &engine) {
    engine.destroyBufferList(sphereBuffers);
    engine.destroyBufferList(boxBuffers);
    engine.destroyBufferList(planeBuffers);
    engine.destroyBufferList(objectBuffers);
}

void Scene::pushSphere(std::string name, glm::vec3 center, float radius, Material mat) {
    if (objects.size() >= MAX_CAPACITY)
        throw std::runtime_error("Object overflow");
    objects.push_back(new Sphere(name, center, radius, mat));
}

void Scene::pushPlane(std::string name, glm::vec3 point, glm::vec3 normal, Material mat) {
    if (objects.size() >= MAX_CAPACITY)
        throw std::runtime_error("Object overflow");
    objects.push_back(new Plane(name, point, normal, mat));
}

void Scene::pushBox(std::string name, glm::vec3 cornerMin, glm::vec3 cornerMax, Material mat) {
    if (objects.size() >= MAX_CAPACITY)
        throw std::runtime_error("Object overflow");
    objects.push_back(new Box(name, cornerMin, cornerMax, mat));
}

void Scene::fillBuffers(VkSmol &engine) {
    // TODO: Only refill them after an update (not every frame)
    std::vector<GpuSphere> spheres;
    std::vector<GpuPlane> planes;
    std::vector<GpuBox> boxes;
    std::vector<GpuObject> objects_data;

    int sphereId = 0;
    int planeId = 0;
    int boxId = 0;
    for (Object *p_object : objects) {
        switch(p_object->getType()) {
            case ObjectType::SPHERE: {
                spheres.push_back(static_cast<Sphere*>(p_object)->getStruct());
                objects_data.push_back({ .type=ObjectType::SPHERE, .id=sphereId });
                sphereId++;
            } break;
            case ObjectType::PLANE: {
                planes.push_back(static_cast<Plane*>(p_object)->getStruct());
                objects_data.push_back({ .type=ObjectType::PLANE, .id=planeId });
                planeId++;
            } break;
            case ObjectType::BOX: {
                boxes.push_back(static_cast<Box*>(p_object)->getStruct());
                objects_data.push_back({ .type=ObjectType::BOX, .id=boxId });
                boxId++;
            } break;
            default: break;
        }
    }

    void *sphereData = malloc(sizeof(GpuSphere) * spheres.size());
    void *planeData = malloc(sizeof(GpuPlane) * planes.size());
    void *boxData = malloc(sizeof(GpuBox) * boxes.size());
    void *objectData = malloc(sizeof(unsigned int) + sizeof(int) + sizeof(GpuObject) * objects_data.size());
    
    memcpy(sphereData, spheres.data(), sizeof(GpuSphere) * spheres.size());
    memcpy(planeData, planes.data(), sizeof(GpuPlane) * planes.size());
    memcpy(boxData, boxes.data(), sizeof(GpuBox) * boxes.size());
    
    unsigned int count = static_cast<unsigned int>(objects_data.size());
    int selected = static_cast<int>(selectedObjectId);  // should be the same as the selectedObjectId (only spheres for now)
    int size = 0;
    memcpy(static_cast<char*>(objectData) + size, &count, sizeof(count));
    size += sizeof(count);
    memcpy(static_cast<char*>(objectData) + size, &selected, sizeof(selected));
    size += sizeof(selected);
    memcpy(static_cast<char*>(objectData) + size, objects_data.data(), sizeof(GpuObject) * objects_data.size());
    
    engine.fillBuffer(engine.getBuffer(sphereBuffers), sphereData);
    engine.fillBuffer(engine.getBuffer(planeBuffers), planeData);
    engine.fillBuffer(engine.getBuffer(boxBuffers), boxData);
    engine.fillBuffer(engine.getBuffer(objectBuffers), objectData);
    
    free(sphereData);
    free(planeData);
    free(boxData);
    free(objectData);
}

void Scene::drawGuizmo(const glm::mat4 &view, const glm::mat4 &proj) {
    if (selectedObjectId < 0) return;
    updated |= objects[selectedObjectId]->drawGuizmo(view, proj);
}


void Scene::drawNewObjectUI() {
    ImGui::SeparatorText("Scene");

    if (ImGui::Button("Add object", { -FLT_MIN, 0 }) && !ImGui::IsPopupOpen("New Object")) {
        ImGui::OpenPopup("New Object");
    }

    if (ImGui::BeginPopupModal("New Object", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) {
        if (ImGui::Button("Sphere", { 100, 0 })) {
            selectedObjectId = objects.size();
            updated = true;
            pushSphere(
                "New Sphere",
                glm::vec3(0.0, 0.0, 0.0),
                1.0,
                Material {
                    .type = LAMBERTIAN,
                    .albedo = { 1.0, 0.0, 1.0 },
                }
            );
            ImGui::CloseCurrentPopup();
        }
        if (ImGui::Button("Plane", { 100, 0 })) {
            selectedObjectId = objects.size();
            updated = true;
            pushPlane(
                "New Plane",
                glm::vec3( 0.0, 0.0, 0.0),
                glm::vec3( 0.0, 1.0, 0.0),
                Material {
                    .type = LAMBERTIAN,
                    .albedo = { 1.0, 0.0, 1.0 },
                }
            );
            ImGui::CloseCurrentPopup();
        }
        if (ImGui::Button("Box", { 100, 0 })) {
            selectedObjectId = objects.size();
            updated = true;
            pushBox(
                "New Box",
                glm::vec3(-1.0,-1.0,-1.0),
                glm::vec3( 1.0, 1.0, 1.0),
                Material {
                    .type = LAMBERTIAN,
                    .albedo = { 1.0, 0.0, 1.0 },
                }
            );
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

void Scene::drawSelectedUI() {
    if (selectedObjectId < 0) return;

    std::string typeName;
    switch (objects[selectedObjectId]->getType()) {
        case ObjectType::SPHERE:    typeName = "Sphere"; break;
        case ObjectType::PLANE:     typeName = "Plane"; break;
        case ObjectType::BOX:       typeName = "Box"; break;
        default: break; // UNREACHABLE
    }

    ImGui::Begin(
        typeName.c_str(),
        nullptr,
        ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings
    );
    {
        objects[selectedObjectId]->drawUI();
    
        ImGui::Separator();
        ImGui::PushStyleColor(ImGuiCol_Button, { 1.0, 0.03, 0.0, 1.0 });
        if (ImGui::Button("Delete", { -FLT_MIN, 0 })) {
            objects.erase(std::next(objects.begin(), selectedObjectId));
            selectedObjectId = -1;
            updated = true;
        }
        ImGui::PopStyleColor();
    }
    ImGui::End();
}

bool Scene::raycast(const glm::vec2 &screenPos, const glm::vec2 &screenSize, const Camera &camera) {
    Ray ray = getRay(screenPos, screenSize, camera);
    float closest_t = std::numeric_limits<float>::infinity();
    int closest_id = -1;

    float t;
    int i = 0;
    for (Object *p_object : objects) {
        t = p_object->rayIntersection(ray);
        if (t >= 0.0f && t < closest_t) {
            closest_t = t;
            closest_id = i;
        }
        i++;
    }

    selectedObjectId = closest_id;
    return closest_id >= 0;
}

bool Scene::isUpdated() {
    if (updated) {
        updated = false;
        return true;
    }
    return false;
}