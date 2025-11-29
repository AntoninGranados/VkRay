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
    memcpy(data, &count, sizeof(count));
    memcpy(static_cast<char*>(data) + offset, spheres.data(), sizeof(Sphere) * spheres.size());
    
    engine.fillBuffer(engine.getBuffer(storageBuffers), data);
    
    free(data);
}

Sphere* Scene::getSelectedSphere() {
    if (selectedSphere < 0) return nullptr;
    return &spheres[selectedSphere];
}

void Scene::sphereUI(int &frameCount, Sphere &sphere) {

    ImGui::Text("Position:");
    ImGui::PushItemWidth(-FLT_MIN);
    if (ImGui::DragFloat3("##Position", glm::value_ptr(sphere.center), 0.01)) frameCount = 0;
    ImGui::PopItemWidth();
    
    ImGui::Text("Radius:");
    ImGui::PushItemWidth(-FLT_MIN);
    if (ImGui::DragFloat("##Radius", &sphere.radius, 0.01, 0.0)) frameCount = 0;
    ImGui::PopItemWidth();
    
    // Material parameters
    if (ImGui::TreeNode("Material")) {
        const char *types[4] = { "Lambertian", "Metal", "Dielectric", "Emissive" };
        if (ImGui::Combo("##Mat-type", (int*)&sphere.mat.type, types, IM_ARRAYSIZE(types))) frameCount = 0;
        
        ImGui::Text("Albedo:");
        if (ImGui::ColorEdit3("##Mat-albedo", glm::value_ptr(sphere.mat.albedo))) frameCount = 0;
        
        switch (sphere.mat.type) {
            case lambertian: break;
            case metal: {
                ImGui::Text("Fuzz:");
                if (ImGui::DragFloat("##Mat-fuzz", &sphere.mat.fuzz, 0.01, 0.0, 1.0)) frameCount = 0;
            } break;
            case dielectric: {
                ImGui::Text("Refraction Index:");
                if (ImGui::DragFloat("##Mat-index", &sphere.mat.refraction_index, 0.1, 0.01, 10000.0, "%.3f", ImGuiSliderFlags_Logarithmic)) frameCount = 0;
            } break;
            case emissive: {
                ImGui::Text("Intensity:");
                if (ImGui::DragFloat("##Mat-intensity", &sphere.mat.intensity, 0.1, 0.0, 100.0)) frameCount = 0;
            } break;
            default: break;
        }
        
        ImGui::TreePop();
    }
}

void Scene::drawUI(int &frameCount) {
    char buff[128];

    ImGui::SeparatorText("Scene");

    if (!ImGui::CollapsingHeader("Objects", ImGuiTreeNodeFlags_DefaultOpen))
        return;

    selectedSphere = -1;
    for (size_t i = 0; i < spheres.size(); i++) {
        if (sphereNames[i].length() > 0)
            std::snprintf(buff, sizeof(buff), "%s", sphereNames[i].c_str());
        else
            std::snprintf(buff, sizeof(buff), "Sphere %zu", i);

        ImGui::TextDisabled("%zu", i);
        ImGui::SameLine();
        if (ImGui::TreeNode(buff)) {
            sphereUI(frameCount, spheres[i]);
            selectedSphere = i;
            ImGui::TreePop();
        }
    }

    if (ImGui::Button("Add sphere", { -FLT_MIN, 0 }) && !ImGui::IsPopupOpen("New sphere")) {
        ImGui::OpenPopup("New sphere");

        pushSphere({
            .center = { 0.0, 0.0, 0.0 },
            .radius = 1.0,
            .mat.type = lambertian,
            .mat.albedo = { 1.0, 0.0, 1.0 },
        });
    }

    if (ImGui::BeginPopupModal("New sphere", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) {
        ImGui::SeparatorText("Sphere parameters");

        char name[128];
        memcpy(name, sphereNames.back().data(), sphereNames.back().size());
        ImGui::InputText("##Name", name, 128);
        sphereNames.back() = std::string(name);

        sphereUI(frameCount, spheres.back());

        ImGui::Separator();
        if (ImGui::Button("Save sphere", { -FLT_MIN, 0 }))
            ImGui::CloseCurrentPopup();

        ImGui::EndPopup();
    }
}
