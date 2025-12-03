#include "scene.hpp"

void Scene::init(VkSmol &engine) {
    sphereBuffers = engine.initBufferList(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, sizeof(Sphere) * MAX_CAPACITY);
    boxBuffers = engine.initBufferList(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, sizeof(Box) * MAX_CAPACITY);
    objectBuffers = engine.initBufferList(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, sizeof(unsigned int) + sizeof(int) + sizeof(Object) * MAX_CAPACITY * 2);
}

void Scene::destroy(VkSmol &engine) {
    engine.destroyBufferList(sphereBuffers);
    engine.destroyBufferList(boxBuffers);
    engine.destroyBufferList(objectBuffers);
}

void Scene::pushSphere(Sphere sphere, std::string name) {
    if (spheres.size() >= MAX_CAPACITY)
        throw std::runtime_error("Sphere overflow");

    objects.push_back({
        .type = ObjectType::SPHERE,
        .id = static_cast<int>(spheres.size())
    });
    spheres.push_back(sphere);
    sphereNames.push_back(name);
}

void Scene::pushBox(Box box, std::string name) {
    if (boxes.size() >= MAX_CAPACITY)
        throw std::runtime_error("Scene overflow");

    objects.push_back({
        .type = ObjectType::BOX,
        .id = static_cast<int>(boxes.size())
    });
    boxes.push_back(box);
    boxNames.push_back(name);
}

void Scene::fillBuffers(VkSmol &engine) {
    void *sphereData = malloc(sizeof(Sphere) * spheres.size());
    void *boxData = malloc(sizeof(Box) * spheres.size());
    void *objectData = malloc(sizeof(unsigned int) + sizeof(int) + sizeof(Object) * objects.size());

    memcpy(sphereData, spheres.data(), sizeof(Sphere) * spheres.size());
    memcpy(boxData, boxes.data(), sizeof(Box) * boxes.size());
    
    unsigned int count = static_cast<unsigned int>(objects.size());
    int selected = static_cast<int>(selectedObjectId);  // should be the same as the selectedObjectId (only spheres for now)
    int size = 0;
    memcpy(static_cast<char*>(objectData) + size, &count, sizeof(count));
    size += sizeof(count);
    memcpy(static_cast<char*>(objectData) + size, &selected, sizeof(selected));
    size += sizeof(selected);
    memcpy(static_cast<char*>(objectData) + size, objects.data(), sizeof(Object) * objects.size());
    
    engine.fillBuffer(engine.getBuffer(sphereBuffers), sphereData);
    engine.fillBuffer(engine.getBuffer(boxBuffers), boxData);
    engine.fillBuffer(engine.getBuffer(objectBuffers), objectData);
    
    free(sphereData);
    free(boxData);
    free(objectData);
}

Object* Scene::getSelectedObject() {
    if (selectedObjectId < 0) return nullptr;
    return &objects[selectedObjectId];
}


void Scene::drawSphereGuizmo(int &frameCount, const glm::mat4 &view, const glm::mat4 &proj, const int &sphereId) {
    glm::mat4 model = glm::translate(glm::mat4(1.0), spheres[sphereId].center);
    model = glm::scale(model, glm::vec3(spheres[sphereId].radius));

    if (ImGuizmo::Manipulate(
        glm::value_ptr(view),
        glm::value_ptr(proj),
        ImGuizmo::OPERATION::SCALE_X | ImGuizmo::OPERATION::TRANSLATE,
        ImGuizmo::MODE::WORLD, 
        glm::value_ptr(model)
    )) {
        glm::vec3 translation, rotation, scale;
        ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(model), glm::value_ptr(translation), glm::value_ptr(rotation), glm::value_ptr(scale));
        spheres[sphereId].center = translation;
        spheres[sphereId].radius = glm::max(scale.x, 0.1f);

        frameCount = 0;
    }
}

void Scene::drawBoxGuizmo(int &frameCount, const glm::mat4 &view, const glm::mat4 &proj, const int &boxId) {
    glm::vec3 center = (boxes[boxId].cornerMax + boxes[boxId].cornerMin) * glm::vec3(0.5);
    glm::mat4 model = glm::translate(glm::mat4(1.0), center);
    model = glm::scale(model, glm::vec3(boxes[boxId].cornerMax - boxes[boxId].cornerMin));

    if (ImGuizmo::Manipulate(
        glm::value_ptr(view),
        glm::value_ptr(proj),
        ImGuizmo::OPERATION::SCALE | ImGuizmo::OPERATION::TRANSLATE,
        ImGuizmo::MODE::WORLD, 
        glm::value_ptr(model)
    )) {
        glm::vec3 translation, rotation, scale;
        ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(model), glm::value_ptr(translation), glm::value_ptr(rotation), glm::value_ptr(scale));
        const glm::vec3 halfSize = glm::max(scale, glm::vec3(0.1)) * 0.5f;
        boxes[boxId].cornerMin = translation - halfSize;
        boxes[boxId].cornerMax = translation + halfSize;
        
        frameCount = 0;
    }
}

void Scene::drawGuizmo(int &frameCount, const glm::mat4 &view, const glm::mat4 &proj) {
    if (selectedObjectId < 0) return;

    Object obj = objects[selectedObjectId];
    switch (obj.type) {
        case ObjectType::SPHERE: drawSphereGuizmo(frameCount, view, proj, obj.id); break;
        case ObjectType::BOX: drawBoxGuizmo(frameCount, view, proj, obj.id); break;
        default: break;
    }
}

void Scene::drawMaterialUI(int &frameCount, Material &mat) {
    ImGui::SeparatorText("Material");
    const char *types[4] = { "Lambertian", "Metal", "Dielectric", "Emissive" };
    if (ImGui::Combo("##Mat-type", (int*)&mat.type, types, IM_ARRAYSIZE(types))) frameCount = 0;
    
    ImGui::Text("Albedo:");
    if (ImGui::ColorEdit3("##Mat-albedo", glm::value_ptr(mat.albedo))) frameCount = 0;
    
    switch (mat.type) {
        case lambertian: break;
        case metal: {
            ImGui::Text("Fuzz:");
            if (ImGui::DragFloat("##Mat-fuzz", &mat.fuzz, 0.01, 0.0, 1.0)) frameCount = 0;
        } break;
        case dielectric: {
            ImGui::Text("Refraction Index:");
            if (ImGui::DragFloat("##Mat-index", &mat.refraction_index, 0.001, 0.01, 10.0, "%.3f")) frameCount = 0;
        } break;
        case emissive: {
            ImGui::Text("Intensity:");
            if (ImGui::DragFloat("##Mat-intensity", &mat.intensity, 0.1, 0.0, 100.0)) frameCount = 0;
        } break;
        default: break;
    }
}

void Scene::drawSphereUI(int &frameCount, const int &sphereId) {
    char name[128];
    memcpy(name, sphereNames[sphereId].data(), sphereNames[sphereId].size());
    ImGui::Text("Name:");
    ImGui::InputText("##Name", name, 128);
    sphereNames[sphereId] = std::string(name);
    
    ImGui::Text("Position:");
    ImGui::PushItemWidth(-FLT_MIN);
    if (ImGui::DragFloat3("##Position", glm::value_ptr(spheres[sphereId].center), 0.01)) frameCount = 0;
    ImGui::PopItemWidth();
    
    ImGui::Text("Radius:");
    ImGui::PushItemWidth(-FLT_MIN);
    if (ImGui::DragFloat("##Radius", &spheres[sphereId].radius, 0.01, 0.0)) frameCount = 0;
    ImGui::PopItemWidth();
    
    drawMaterialUI(frameCount, spheres[sphereId].mat);
}

void Scene::drawBoxUI(int &frameCount, const int &boxId) {
    char name[128];
    memcpy(name, boxNames[boxId].data(), boxNames[boxId].size());
    ImGui::Text("Name:");
    ImGui::InputText("##Name", name, 128);
    boxNames[boxId] = std::string(name);
    
    ImGui::Text("Corner Max:");
    ImGui::PushItemWidth(-FLT_MIN);
    if (ImGui::DragFloat3("##CornerMax", glm::value_ptr(boxes[boxId].cornerMax), 0.01)) frameCount = 0;
    ImGui::PopItemWidth();

    ImGui::Text("Corner Min:");
    ImGui::PushItemWidth(-FLT_MIN);
    if (ImGui::DragFloat3("##CornerMin", glm::value_ptr(boxes[boxId].cornerMin), 0.01)) frameCount = 0;
    ImGui::PopItemWidth();
    
    drawMaterialUI(frameCount, boxes[boxId].mat);
}

void Scene::drawInformationUI(int &frameCount) {
    ImGui::SeparatorText("Scene");

    if (ImGui::Button("Add sphere", { -FLT_MIN, 0 }) && !ImGui::IsPopupOpen("New sphere")) {
        pushSphere({
            .center = { 0.0, 0.0, 0.0 },
            .radius = 1.0,
            .mat.type = lambertian,
            .mat.albedo = { 1.0, 0.0, 1.0 },
        });
        ImGui::OpenPopup("New sphere");
    }

    if (ImGui::BeginPopupModal("New sphere", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) {
        ImGui::SeparatorText("Sphere parameters");
        
        drawSphereUI(frameCount, spheres.size()-1);

        ImGui::Separator();
        if (ImGui::Button("Save sphere", { -FLT_MIN, 0 })) {
            frameCount = 0;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

void Scene::drawSelectedUI(int &frameCount) {
    if (selectedObjectId < 0) return;

    // TODO: open popup modal to make sure
    switch (objects[selectedObjectId].type) {
        case ObjectType::SPHERE: {
            ImGui::Begin("Sphere", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
            drawSphereUI(frameCount, objects[selectedObjectId].id);

            ImGui::Separator();
            ImGui::PushStyleColor(ImGuiCol_Button, { 1.0, 0.03, 0.0, 1.0 });
            if (ImGui::Button("Delete Sphere", { -FLT_MIN, 0 })) {
                spheres.erase(std::next(spheres.begin(), objects[selectedObjectId].id));
                sphereNames.erase(std::next(sphereNames.begin(), objects[selectedObjectId].id));
                objects.erase(std::next(objects.begin(), selectedObjectId));
                selectedObjectId = -1;
                frameCount = 0;
            }
            ImGui::PopStyleColor();
        } break;
        case ObjectType::BOX: {
            ImGui::Begin("Box", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
            drawBoxUI(frameCount, objects[selectedObjectId].id);

            ImGui::Separator();
            ImGui::PushStyleColor(ImGuiCol_Button, { 1.0, 0.03, 0.0, 1.0 });
            if (ImGui::Button("Delete Box", { -FLT_MIN, 0 })) {
                boxes.erase(std::next(boxes.begin(), objects[selectedObjectId].id));
                boxNames.erase(std::next(boxNames.begin(), objects[selectedObjectId].id));
                objects.erase(std::next(objects.begin(), selectedObjectId));
                selectedObjectId = -1;
                frameCount = 0;
            }
            ImGui::PopStyleColor();
        } break;
        default: {
            ImGui::Begin("Not implemented", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
        } break;
    }

    ImGui::End();
}
