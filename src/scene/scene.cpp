#include "scene.hpp"

#include <cstring>
#include <glm/gtc/matrix_transform.hpp>

constexpr size_t OBJECT_HEADER_SIZE = sizeof(unsigned int) + sizeof(int);

void Scene::init(VkSmol &engine) {
    sphereBuffers = engine.initBufferList(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, sizeof(GpuSphere) * sphereBuffersCapacity);
    planeBuffers = engine.initBufferList(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, sizeof(GpuPlane) * planeBuffersCapacity);
    boxBuffers = engine.initBufferList(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, sizeof(GpuBox) * boxBuffersCapacity);
    objectBuffers = engine.initBufferList(
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        OBJECT_HEADER_SIZE + sizeof(GpuObject) * objectBuffersCapacity
    );
}

void Scene::destroy(VkSmol &engine) {
    engine.destroyBufferList(sphereBuffers);
    engine.destroyBufferList(planeBuffers);
    engine.destroyBufferList(boxBuffers);
    engine.destroyBufferList(objectBuffers);
}

void Scene::resizeBuffer(VkSmol &engine, bufferList_t &bufferList, size_t &capacity, size_t objectSize, size_t baseSize) {
    engine.waitIdle();

    char buff[128];
    snprintf(buff, 128, "Resize buffer %zu -> %zu", capacity, capacity*2);
    messageCallback(NotificationType::Debug, buff);

    engine.destroyBufferList(bufferList);
    capacity *= 2;
    bufferList = engine.initBufferList(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, baseSize + objectSize * capacity);
}

bool Scene::pushSphere(VkSmol &engine, std::string name, glm::vec3 center, float radius, Material mat) {
    if (sphereBuffersSize >= sphereBuffersCapacity) {
        resizeBuffer(engine, sphereBuffers, sphereBuffersCapacity, sizeof(GpuSphere));
    }
    if (objectBuffersSize >= objectBuffersCapacity) {
        resizeBuffer(engine, objectBuffers, objectBuffersCapacity, sizeof(GpuObject), OBJECT_HEADER_SIZE);
    }
    objects.push_back(new Sphere(name, center, radius, mat));
    
    sphereBuffersSize++;
    objectBuffersSize++;

    return true;
}

bool Scene::pushPlane(VkSmol &engine, std::string name, glm::vec3 point, glm::vec3 normal, Material mat) {
    if (planeBuffersSize >= planeBuffersCapacity) {
        resizeBuffer(engine, planeBuffers, planeBuffersCapacity, sizeof(GpuPlane));
    }
    if (objectBuffersSize >= objectBuffersCapacity) {
        resizeBuffer(engine, objectBuffers, objectBuffersCapacity, sizeof(GpuObject), OBJECT_HEADER_SIZE);
    }
    objects.push_back(new Plane(name, point, normal, mat));
    
    planeBuffersSize++;
    objectBuffersSize++;

    return true;
}

bool Scene::pushBox(VkSmol &engine, std::string name, glm::vec3 cornerMin, glm::vec3 cornerMax, Material mat) {
    if (boxBuffersSize >= boxBuffersCapacity) {
        resizeBuffer(engine, boxBuffers, boxBuffersCapacity, sizeof(GpuBox));
    }
    if (objectBuffersSize >= objectBuffersCapacity) {
        resizeBuffer(engine, objectBuffers, objectBuffersCapacity, sizeof(GpuObject), OBJECT_HEADER_SIZE);
    }
    objects.push_back(new Box(name, cornerMin, cornerMax, mat));
    
    boxBuffersSize++;
    objectBuffersSize++;

    return true;
}

void Scene::fillBuffers(VkSmol &engine) {
    // TODO: Only refill them after an update (not every frame)
    std::vector<GpuSphere> spheres(sphereBuffersCapacity);
    std::vector<GpuPlane> planes(planeBuffersCapacity);
    std::vector<GpuBox> boxes(boxBuffersCapacity);
    std::vector<GpuObject> objects_data(objectBuffersCapacity);

    int sphereId = 0;
    int planeId = 0;
    int boxId = 0;
    unsigned int objectCount = 0;
    for (Object *p_object : objects) {
        switch(p_object->getType()) {
            case ObjectType::Sphere: {
                spheres[sphereId] = static_cast<Sphere*>(p_object)->getStruct();
                objects_data[objectCount] = { .type=ObjectType::Sphere, .id=sphereId };
                sphereId++;
                objectCount++;
            } break;
            case ObjectType::Plane: {
                planes[planeId] = static_cast<Plane*>(p_object)->getStruct();
                objects_data[objectCount] = { .type=ObjectType::Plane, .id=planeId };
                planeId++;
                objectCount++;
            } break;
            case ObjectType::Box: {
                boxes[boxId] = static_cast<Box*>(p_object)->getStruct();
                objects_data[objectCount] = { .type=ObjectType::Box, .id=boxId };
                boxId++;
                objectCount++;
            } break;
            default: break;
        }
    }

    int selected = static_cast<int>(selectedObjectId);  // should be the same as the selectedObjectId (only spheres for now)

    engine.fillBuffer(engine.getBuffer(sphereBuffers), spheres.data());
    engine.fillBuffer(engine.getBuffer(planeBuffers), planes.data());
    engine.fillBuffer(engine.getBuffer(boxBuffers), boxes.data());

    std::vector<char> objectData(OBJECT_HEADER_SIZE + sizeof(GpuObject) * objectBuffersCapacity, 0);
    size_t offset = 0;
    memcpy(objectData.data() + offset, &objectCount, sizeof(objectCount));
    offset += sizeof(objectCount);
    memcpy(objectData.data() + offset, &selected, sizeof(selected));
    offset += sizeof(selected);
    if (objectCount > 0)
        memcpy(objectData.data() + offset, objects_data.data(), sizeof(GpuObject) * objectCount);

    engine.fillBuffer(engine.getBuffer(objectBuffers), objectData.data());
}

void Scene::drawGuizmo(const glm::mat4 &view, const glm::mat4 &proj) {
    if (selectedObjectId < 0) return;
    ImGuizmo::PushID(selectedObjectId); // To isolate the state of the gizmo
    updated |= objects[selectedObjectId]->drawGuizmo(view, proj);
    ImGuizmo::PopID();
}


void Scene::drawNewObjectUI(VkSmol &engine) {
    ImGui::SeparatorText("Scene");

    if (ImGui::Button("Add object", { -FLT_MIN, 0 }) && !ImGui::IsPopupOpen("New Object")) {
        ImGui::OpenPopup("New Object");
    }

    if (ImGui::BeginPopupModal("New Object", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) {
        if (ImGui::Button("Sphere", { 100, 0 })) {
            if (pushSphere(
                engine,
                "New Sphere",
                glm::vec3(0.0, 0.0, 0.0),
                1.0,
                Material {
                    .type = Lambertian,
                    .albedo = { 1.0, 0.0, 1.0 },
                }
            )) {
                selectedObjectId = objects.size() - 1;
                updated = true;
                std::cout << selectedObjectId << std::endl;
            }
            ImGui::CloseCurrentPopup();
        }
        if (ImGui::Button("Plane", { 100, 0 })) {
            if (pushPlane(
                engine,
                "New Plane",
                glm::vec3( 0.0, 0.0, 0.0),
                glm::vec3( 0.0, 1.0, 0.0),
                Material {
                    .type = Lambertian,
                    .albedo = { 1.0, 0.0, 1.0 },
                }
            )) {
                selectedObjectId = objects.size() - 1;
                updated = true;
            }
            ImGui::CloseCurrentPopup();
        }
        if (ImGui::Button("Box", { 100, 0 })) {
            if (pushBox(
                engine,
                "New Box",
                glm::vec3(-1.0,-1.0,-1.0),
                glm::vec3( 1.0, 1.0, 1.0),
                Material {
                    .type = Lambertian,
                    .albedo = { 1.0, 0.0, 1.0 },
                }
            )) {
                selectedObjectId = objects.size() - 1;
                updated = true;
            }
            ImGui::CloseCurrentPopup();
        }
        ImGui::PushStyleColor(ImGuiCol_Button, { 1.0, 0.03, 0.0, 1.0 });
        if (ImGui::Button("Cancel", { 100, 0 })) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::PopStyleColor();

        ImGui::EndPopup();
    }
}

void Scene::drawSelectedUI() {
    if (selectedObjectId < 0) return;

    std::string typeName;
    switch (objects[selectedObjectId]->getType()) {
        case ObjectType::Sphere:    typeName = "Sphere"; break;
        case ObjectType::Plane:     typeName = "Plane"; break;
        case ObjectType::Box:       typeName = "Box"; break;
        default: break; // UNREACHABLE
    }

    ImGui::SetNextWindowBgAlpha(0.8f);
    ImGui::SetNextWindowSize({ 200, 0 });
    ImGui::Begin(
        typeName.c_str(),
        nullptr,
        ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing
    );
    {
        char buff[128] = {};
        const std::string &name = objects[selectedObjectId]->getName();
        std::strncpy(buff, name.c_str(), sizeof(buff) - 1);
        ImGui::Text("Name:");
        ImGui::PushItemWidth(-FLT_MIN);
        ImGui::InputText("##Name", buff, 128);
        ImGui::PopItemWidth();
        objects[selectedObjectId]->setName(std::string(buff));
        
        updated |= objects[selectedObjectId]->drawUI();
    
        ImGui::Separator();
        ImGui::PushStyleColor(ImGuiCol_Button, { 1.0, 0.03, 0.0, 1.0 });
        if (ImGui::Button("Delete", { -FLT_MIN, 0 })) {
            switch(objects[selectedObjectId]->getType()) {
                case ObjectType::Sphere: sphereBuffersSize--;
                case ObjectType::Plane:  planeBuffersSize--;
                case ObjectType::Box:    boxBuffersSize--;
                default: break;
            }
            objectBuffersSize--;

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

bool Scene::wasUpdated() {
    if (updated) {
        updated = false;
        return true;
    }
    return false;
}
