#include "scene.hpp"

#include <cstring>
#include <glm/gtc/matrix_transform.hpp>

void GpuBuffers::init(VkSmol &engine, size_t _objectSize, size_t _baseSize) {
    count = 0;
    capacity = 2;
    objectSize = _objectSize;
    baseSize = _baseSize;

    bufferList = engine.initBufferList(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, baseSize + objectSize * capacity);
}

void GpuBuffers::destroy(VkSmol &engine) {
    engine.destroyBufferList(bufferList);
}

void GpuBuffers::addElement(VkSmol &engine) {
    if (count >= capacity) resize(engine);
    count++;
}

void GpuBuffers::removeElement() {
    count--;
}

void GpuBuffers::resize(VkSmol &engine) {
    engine.waitIdle();
    engine.destroyBufferList(bufferList);
    capacity *= 2;
    bufferList = engine.initBufferList(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, baseSize + objectSize * capacity);
}


constexpr size_t OBJECT_HEADER_SIZE = sizeof(unsigned int) + sizeof(int);

void Scene::init(VkSmol &engine) {
    sphereBuffers.init(engine, sizeof(GpuSphere));
    planeBuffers.init(engine, sizeof(GpuPlane));
    boxBuffers.init(engine, sizeof(GpuBox));
    objectBuffers.init(engine, sizeof(GpuObject), OBJECT_HEADER_SIZE);
}

void Scene::destroy(VkSmol &engine) {
    sphereBuffers.destroy(engine);
    planeBuffers.destroy(engine);
    boxBuffers.destroy(engine);
    objectBuffers.destroy(engine);
}

void Scene::pushSphere(VkSmol &engine, std::string name, glm::vec3 center, float radius, Material mat) {
    sphereBuffers.addElement(engine);
    objectBuffers.addElement(engine);
    objects.push_back(new Sphere(name, center, radius, mat));
}

void Scene::pushPlane(VkSmol &engine, std::string name, glm::vec3 point, glm::vec3 normal, Material mat) {
    planeBuffers.addElement(engine);
    objectBuffers.addElement(engine);
    objects.push_back(new Plane(name, point, normal, mat));
}

void Scene::pushBox(VkSmol &engine, std::string name, glm::vec3 cornerMin, glm::vec3 cornerMax, Material mat) {
    boxBuffers.addElement(engine);
    objectBuffers.addElement(engine);
    objects.push_back(new Box(name, cornerMin, cornerMax, mat));
}

void Scene::fillBuffers(VkSmol &engine) {
    // TODO: Only refill them after an update (not every frame)
    std::vector<GpuSphere> spheres(sphereBuffers.getCapacity());
    std::vector<GpuPlane> planes(planeBuffers.getCapacity());
    std::vector<GpuBox> boxes(boxBuffers.getCapacity());
    std::vector<GpuObject> objects_data(objectBuffers.getCapacity());

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

    engine.fillBuffer(engine.getBuffer(sphereBuffers.getBufferList()), spheres.data());
    engine.fillBuffer(engine.getBuffer(planeBuffers.getBufferList()), planes.data());
    engine.fillBuffer(engine.getBuffer(boxBuffers.getBufferList()), boxes.data());

    std::vector<char> objectData(OBJECT_HEADER_SIZE + sizeof(GpuObject) * objectBuffers.getCapacity(), 0);
    size_t offset = 0;
    memcpy(objectData.data() + offset, &objectCount, sizeof(objectCount));
    offset += sizeof(objectCount);
    memcpy(objectData.data() + offset, &selected, sizeof(selected));
    offset += sizeof(selected);
    if (objectCount > 0)
        memcpy(objectData.data() + offset, objects_data.data(), sizeof(GpuObject) * objectCount);

    engine.fillBuffer(engine.getBuffer(objectBuffers.getBufferList()), objectData.data());
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
            pushSphere(
                engine,
                "New Sphere",
                glm::vec3(0.0, 0.0, 0.0),
                1.0,
                Material {
                    .type = Lambertian,
                    .albedo = { 1.0, 0.0, 1.0 },
                }
            );
            selectedObjectId = objects.size() - 1;
            updated = true;
            ImGui::CloseCurrentPopup();
        }
        if (ImGui::Button("Plane", { 100, 0 })) {
            pushPlane(
                engine,
                "New Plane",
                glm::vec3( 0.0, 0.0, 0.0),
                glm::vec3( 0.0, 1.0, 0.0),
                Material {
                    .type = Lambertian,
                    .albedo = { 1.0, 0.0, 1.0 },
                }
            );
            selectedObjectId = objects.size() - 1;
            updated = true;
            ImGui::CloseCurrentPopup();
        }
        if (ImGui::Button("Box", { 100, 0 })) {
            pushBox(
                engine,
                "New Box",
                glm::vec3(-1.0,-1.0,-1.0),
                glm::vec3( 1.0, 1.0, 1.0),
                Material {
                    .type = Lambertian,
                    .albedo = { 1.0, 0.0, 1.0 },
                }
            );
            selectedObjectId = objects.size() - 1;
            updated = true;
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
                case ObjectType::Sphere: sphereBuffers.removeElement();
                case ObjectType::Plane:  planeBuffers.removeElement();
                case ObjectType::Box:    boxBuffers.removeElement();
                default: break;
            }
            objectBuffers.removeElement();

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

std::vector<bufferList_t> Scene::getBufferLists() {
    std::vector<bufferList_t> bufferLists = {
        sphereBuffers.getBufferList(),
        planeBuffers.getBufferList(),
        boxBuffers.getBufferList(),
        objectBuffers.getBufferList(),
    };


    return bufferLists;
}

bool Scene::wasUpdated() {
    if (updated) {
        updated = false;
        return true;
    }
    return false;
}
