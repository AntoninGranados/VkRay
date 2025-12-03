#include "scene.hpp"

void Scene::init(VkSmol &engine) {
    storageBuffers = engine.initBufferList(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, 16 + sizeof(Sphere) * MAX_CAPACITY);
}

void Scene::destroy(VkSmol &engine) {
    engine.destroyBufferList(storageBuffers);
}

void Scene::pushSphere(Sphere sphere, std::string name) {
    if (spheres.size() >= MAX_CAPACITY)
        throw std::runtime_error("Scene object overflow");

    spheres.push_back(sphere);
    sphereNames.push_back(name);
}

void Scene::fillBuffer(VkSmol &engine) {
    // FIXME: compute the offset dynamically using the size of all previous elements
    int offset = 16;
    void *data = malloc(offset + sizeof(Sphere) * MAX_CAPACITY);
    
    int count = static_cast<int>(spheres.size());
    int selected = static_cast<int>(selectedSphereId);
    memcpy(static_cast<char*>(data) + 0, &count, sizeof(count));
    memcpy(static_cast<char*>(data) + sizeof(count), &selected, sizeof(selected));
    memcpy(static_cast<char*>(data) + offset, spheres.data(), sizeof(Sphere) * spheres.size());
    
    engine.fillBuffer(engine.getBuffer(storageBuffers), data);
    
    free(data);
}

Sphere* Scene::getSelectedSphere() {
    if (selectedSphereId < 0) return nullptr;
    return &spheres[selectedSphereId];
}


void Scene::sphereUI(int &frameCount, const int &sphereId) {
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
    
    // Material parameters
    ImGui::SeparatorText("Material");
    const char *types[4] = { "Lambertian", "Metal", "Dielectric", "Emissive" };
    if (ImGui::Combo("##Mat-type", (int*)&spheres[sphereId].mat.type, types, IM_ARRAYSIZE(types))) frameCount = 0;
    
    ImGui::Text("Albedo:");
    if (ImGui::ColorEdit3("##Mat-albedo", glm::value_ptr(spheres[sphereId].mat.albedo))) frameCount = 0;
    
    switch (spheres[sphereId].mat.type) {
        case lambertian: break;
        case metal: {
            ImGui::Text("Fuzz:");
            if (ImGui::DragFloat("##Mat-fuzz", &spheres[sphereId].mat.fuzz, 0.01, 0.0, 1.0)) frameCount = 0;
        } break;
        case dielectric: {
            ImGui::Text("Refraction Index:");
            if (ImGui::DragFloat("##Mat-index", &spheres[sphereId].mat.refraction_index, 0.001, 0.01, 10.0, "%.3f")) frameCount = 0;
        } break;
        case emissive: {
            ImGui::Text("Intensity:");
            if (ImGui::DragFloat("##Mat-intensity", &spheres[sphereId].mat.intensity, 0.1, 0.0, 100.0)) frameCount = 0;
        } break;
        default: break;
    }
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
        
        sphereUI(frameCount, spheres.size()-1);

        ImGui::Separator();
        if (ImGui::Button("Save sphere", { -FLT_MIN, 0 })) {
            frameCount = 0;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

void Scene::drawSelectedUI(int &frameCount) {
    if (selectedSphereId >= 0) {
        ImGui::Begin("Sphere", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
        sphereUI(frameCount, selectedSphereId);

        // TODO: open popup modal to make sure
        ImGui::Separator();
        ImGui::PushStyleColor(ImGuiCol_Button, { 1.0, 0.03, 0.0, 1.0 });
        if (ImGui::Button("Delete sphere", { -FLT_MIN, 0 })) {
            frameCount = 0;
            spheres.erase(std::next(spheres.begin(), selectedSphereId));
            sphereNames.erase(std::next(sphereNames.begin(), selectedSphereId));
            selectedSphereId = -1;
        }
        ImGui::PopStyleColor();
        
        ImGui::End();
    }
}
