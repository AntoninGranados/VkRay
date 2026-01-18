// 000101001
// 001000000

#include "scene.hpp"

#include <iostream>
#include <cstring>
#include <glm/gtc/matrix_transform.hpp>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

constexpr size_t OBJECT_HEADER_SIZE = sizeof(unsigned int) + sizeof(int);
constexpr size_t LIGHT_HEADER_SIZE = sizeof(float);

void Scene::init(VkSmol &engine) {
    sphereBuffers.init(engine, sizeof(GpuSphere));
    planeBuffers.init(engine, sizeof(GpuPlane));
    boxBuffers.init(engine, sizeof(GpuBox));
    vertexBuffers.init(engine, sizeof(Vertex));
    indexBuffers.init(engine, sizeof(unsigned int));
    bvhBuffers.init(engine, sizeof(GpuBvhNode));
    meshBuffers.init(engine, sizeof(GpuMesh));

    materialBuffers.init(engine, sizeof(Material));
    objectBuffers.init(engine, sizeof(ObjectHandle), OBJECT_HEADER_SIZE);
    lightBuffers.init(engine, sizeof(GpuLight), LIGHT_HEADER_SIZE);
}

void Scene::destroy(VkSmol &engine) {
    sphereBuffers.destroy(engine);
    planeBuffers.destroy(engine);
    boxBuffers.destroy(engine);
    vertexBuffers.destroy(engine);
    indexBuffers.destroy(engine);
    bvhBuffers.destroy(engine);
    meshBuffers.destroy(engine);

    materialBuffers.destroy(engine);
    objectBuffers.destroy(engine);
    lightBuffers.destroy(engine);
}

void Scene::clear(VkSmol &engine) {
    engine.waitIdle();
    
    sphereBuffers.clear(engine);
    planeBuffers.clear(engine);
    boxBuffers.clear(engine);
    vertexBuffers.clear(engine);
    indexBuffers.clear(engine);
    bvhBuffers.clear(engine);
    meshBuffers.clear(engine);

    materialBuffers.clear(engine);
    objectBuffers.clear(engine);
    lightBuffers.clear(engine);

    objects.clear();
    materials.clear();
    selectedObjectId = -1;
    bufferUpdated = true;
}


void Scene::pushSphere(VkSmol &engine, std::string name, glm::vec3 center, float radius, Material mat) {
    bufferUpdated |= sphereBuffers.addElement(engine);
    bufferUpdated |= materialBuffers.addElement(engine);
    bufferUpdated |= objectBuffers.addElement(engine);

    objects.push_back(new Sphere(name, center, radius, static_cast<int>(materials.size())));
    materials.push_back(mat);
}

void Scene::pushPlane(VkSmol &engine, std::string name, glm::vec3 point, glm::vec3 normal, Material mat) {
    bufferUpdated |= planeBuffers.addElement(engine);
    bufferUpdated |= materialBuffers.addElement(engine);
    bufferUpdated |= objectBuffers.addElement(engine);

    objects.push_back(new Plane(name, point, normal, static_cast<int>(materials.size())));
    materials.push_back(mat);
}

void Scene::pushBox(VkSmol &engine, std::string name, glm::vec3 cornerMin, glm::vec3 cornerMax, Material mat) {
    glm::vec3 center = (cornerMin + cornerMax) * 0.5f;
    glm::vec3 halfExtents = (cornerMax - cornerMin) * 0.5f;
    glm::mat4 transform = glm::translate(glm::mat4(1.0f), center);
    transform = glm::scale(transform, halfExtents);
    pushBoxTransform(engine, name, transform, mat);
}

void Scene::pushBoxTransform(VkSmol &engine, std::string name, const glm::mat4 &transform, Material mat) {
    bufferUpdated |= boxBuffers.addElement(engine);
    bufferUpdated |= materialBuffers.addElement(engine);
    bufferUpdated |= objectBuffers.addElement(engine);

    objects.push_back(new Box(name, transform, static_cast<int>(materials.size())));
    materials.push_back(mat);
}

void Scene::pushMesh(VkSmol &engine, std::string name, std::vector<Vertex> vertices, std::vector<unsigned int> indices, glm::mat4 transform, Material mat) {
    bufferUpdated |= meshBuffers.addElement(engine);
    bufferUpdated |= materialBuffers.addElement(engine);
    bufferUpdated |= objectBuffers.addElement(engine);

    objects.push_back(new Mesh(name, std::move(vertices), std::move(indices), transform, static_cast<int>(materials.size())));
    materials.push_back(mat);
}

bool Scene::pushMeshFromObj(VkSmol &engine, const std::string &name, const std::string &path, Material mat, const glm::mat4 &transform) {
    std::string baseDir = "./";
    size_t slash = path.find_last_of("/\\");
    if (slash != std::string::npos) {
        baseDir = path.substr(0, slash + 1);
    }

    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn;
    std::string err;
    bool loaded = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path.c_str(), baseDir.c_str(), true);
    if (!warn.empty()) {
        std::cerr << "[WARN] OBJ load: " << warn << std::endl;
    }
    if (!err.empty()) {
        std::cerr << "[ERROR] OBJ load: " << err << std::endl;
    }
    if (!loaded) {
        std::cerr << "[ERROR] Failed to load OBJ: " << path << std::endl;
        return false;
    }

    std::vector<Vertex> meshVertices;
    meshVertices.reserve(attrib.vertices.size() / 3);
    for (size_t i = 0; i + 2 < attrib.vertices.size(); i += 3) {
        meshVertices.push_back(Vertex{
            .position = glm::vec3(attrib.vertices[i + 0], attrib.vertices[i + 1], attrib.vertices[i + 2]),
        });
    }

    std::vector<unsigned int> meshIndices;
    for (const tinyobj::shape_t &shape : shapes) {
        size_t indexOffset = 0;
        for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++) {
            int fv = shape.mesh.num_face_vertices[f];
            if (fv != 3) {
                indexOffset += fv;
                continue;
            }

            for (int v = 0; v < fv; v++) {
                const tinyobj::index_t idx = shape.mesh.indices[indexOffset + v];
                if (idx.vertex_index < 0) continue;
                meshIndices.push_back(static_cast<unsigned int>(idx.vertex_index));
            }
            indexOffset += fv;
        }
    }

    pushMesh(engine, name, std::move(meshVertices), std::move(meshIndices), transform, mat);
    return true;
}

// TODO refactor this
inline void addLight(const Material &mat, const float &area, const int &objectId, int &lightCount, std::vector<GpuLight> &lights, float &totalLightArea) {
    if (mat.type == MaterialType::Emissive) {
        lightCount++;
        lights.push_back(GpuLight{
            .objectId = objectId,
            .area = area,
            .pdfA = 1.0f/area,
        });
        totalLightArea += area;
    }
};

// TODO: Only refill them after an update (not every frame)
void Scene::fillBuffers(VkSmol &engine) {
    size_t totalVertices = 0;
    size_t totalIndices = 0;
    size_t totalBvhNodes = 0;
    for (Object *object : objects) {
        if (object->getType() == ObjectType::Mesh) {
            Mesh *mesh = static_cast<Mesh*>(object);
            totalVertices += mesh->getVertices().size();
            totalIndices += mesh->getIndices().size();
            totalBvhNodes += mesh->getBvhNodes().size();
        }
    }

    bufferUpdated |= vertexBuffers.setElementCount(engine, totalVertices);
    bufferUpdated |= indexBuffers.setElementCount(engine, totalIndices);
    bufferUpdated |= bvhBuffers.setElementCount(engine, totalBvhNodes);

    std::vector<GpuSphere> spheres(sphereBuffers.getCapacity());
    std::vector<GpuPlane> planes(planeBuffers.getCapacity());
    std::vector<GpuBox> boxes(boxBuffers.getCapacity());
    std::vector<Vertex> vertices(vertexBuffers.getCapacity());
    std::vector<uint32_t> indices(indexBuffers.getCapacity());
    std::vector<GpuBvhNode> bvhNodes(bvhBuffers.getCapacity());
    std::vector<GpuMesh> meshes(meshBuffers.getCapacity());
    std::vector<Material> materialData(materialBuffers.getCapacity());
    std::vector<ObjectHandle> objectHandles(objectBuffers.getCapacity());
    std::vector<GpuLight> lights;

    int sphereId = 0;
    int planeId = 0;
    int boxId = 0;
    int meshId = 0;
    uint32_t objectCount = 0;
    int lightCount = 0;
    float totalLightArea = 0;
    uint32_t vertexOffset = 0;
    uint32_t indexOffset = 0;
    uint32_t bvhOffset = 0;
    
    for (Object *object : objects) {
        switch(object->getType()) {
            case ObjectType::Sphere: {
                spheres[sphereId] = static_cast<Sphere*>(object)->getStruct();
                addLight(materials[spheres[sphereId].materialHandle], object->getArea(), objectCount, lightCount, lights, totalLightArea);
                objectHandles[objectCount] = { .type=ObjectType::Sphere, .id=sphereId };
                sphereId++;
                objectCount++;
            } break;
            case ObjectType::Plane: {
                planes[planeId] = static_cast<Plane*>(object)->getStruct();
                // addLight(materials[planes[planeId].materialHandle], object->getArea(), objectCount, lightCount, lights, totalLightArea);    //! should not be counted as it can't be used for importance sampling
                objectHandles[objectCount] = { .type=ObjectType::Plane, .id=planeId };
                planeId++;
                objectCount++;
            } break;
            case ObjectType::Box: {
                boxes[boxId] = static_cast<Box*>(object)->getStruct();
                addLight(materials[boxes[boxId].materialHandle], object->getArea(), objectCount, lightCount, lights, totalLightArea);
                objectHandles[objectCount] = { .type=ObjectType::Box, .id=boxId };
                boxId++;
                objectCount++;
            } break;
            case ObjectType::Mesh: {
                Mesh *mesh = static_cast<Mesh*>(object);
                const std::vector<Vertex> &meshVerts = mesh->getVertices();
                const std::vector<uint32_t> &meshIndices = mesh->getIndices();
                const std::vector<GpuBvhNode> &meshBvhNodes = mesh->getBvhNodes();

                for (size_t i = 0; i < meshVerts.size(); i++) {
                    vertices[vertexOffset + i] = meshVerts[i];
                }
                for (size_t i = 0; i < meshIndices.size(); i++) {
                    indices[indexOffset + i] = meshIndices[i] + vertexOffset;
                }
                for (size_t i = 0; i < meshBvhNodes.size(); i++) {
                    GpuBvhNode node = meshBvhNodes[i];
                    if (node.isLeaf != 0) {
                        node.data0 = static_cast<size_t>(node.data0 + (indexOffset / 3));
                    } else {
                        node.data0 = static_cast<size_t>(node.data0 + bvhOffset);
                        node.data1 = static_cast<size_t>(node.data1 + bvhOffset);
                    }
                    bvhNodes[bvhOffset + i] = node;
                }

                GpuMesh meshStruct = mesh->getStruct();
                meshStruct.indexOffset = indexOffset;
                meshStruct.bvhOffset = bvhOffset;
                meshes[meshId] = meshStruct;

                addLight(materials[meshStruct.materialHandle], object->getArea(), objectCount, lightCount, lights, totalLightArea);
                objectHandles[objectCount] = { .type=ObjectType::Mesh, .id=meshId };

                vertexOffset += static_cast<uint32_t>(meshVerts.size());
                indexOffset += static_cast<uint32_t>(meshIndices.size());
                bvhOffset += static_cast<uint32_t>(meshBvhNodes.size());
                meshId++;
                objectCount++;
            } break;
            default: break;
        }
    }
    bufferUpdated |= lightBuffers.setElementCount(engine, lightCount);

    for (size_t i = 0; i < materials.size() && i < materialData.size(); i++) {
        materialData[i] = materials[i];
    }

    int selected = static_cast<int>(selectedObjectId);

    // Fill the buffers
    sphereBuffers.fill(engine, spheres.data());
    planeBuffers.fill(engine, planes.data());
    boxBuffers.fill(engine, boxes.data());
    materialBuffers.fill(engine, materialData.data());
    vertexBuffers.fill(engine, vertices.data());
    indexBuffers.fill(engine, indices.data());
    meshBuffers.fill(engine, meshes.data());
    bvhBuffers.fill(engine, bvhNodes.data());

    size_t offset;
    
    std::vector<char> objectData(OBJECT_HEADER_SIZE + sizeof(ObjectHandle) * objectBuffers.getCapacity(), 0);
    offset = 0;
    memcpy(objectData.data() + offset, &objectCount, sizeof(objectCount));
    offset += sizeof(objectCount);
    memcpy(objectData.data() + offset, &selected, sizeof(selected));
    offset += sizeof(selected);
    if (objectCount > 0)
        memcpy(objectData.data() + offset, objectHandles.data(), objectHandles.size() * sizeof(ObjectHandle));

    objectBuffers.fill(engine, objectData.data());

    std::vector<char> lightData(LIGHT_HEADER_SIZE + sizeof(GpuLight) * lightBuffers.getCapacity(), 0);
    offset = 0;
    memcpy(lightData.data() + offset, &totalLightArea, sizeof(totalLightArea));
    offset += sizeof(totalLightArea);
    memcpy(lightData.data() + offset, lights.data(), lights.size() * sizeof(GpuLight));
        
    lightBuffers.fill(engine, lightData.data());
}


void Scene::drawGuizmo(const glm::mat4 &view, const glm::mat4 &proj) {
    if (selectedObjectId < 0) return;
    ImGuizmo::PushID(selectedObjectId); // To isolate the state of the gizmo
    updated |= objects[selectedObjectId]->drawGuizmo(view, proj);
    ImGuizmo::PopID();
}

void Scene::drawUI(VkSmol &engine) {
    if (ImGui::Button("Add object", { -FLT_MIN, 0 }) && !ImGui::IsPopupOpen("New Object")) {
        ImGui::OpenPopup("New Object");
    }

    for (size_t i = 0; i < objects.size(); i++) {
        switch (objects[i]->getType()) {
            case ObjectType::Sphere: ImGui::TextDisabled("Sph"); break;
            case ObjectType::Plane:  ImGui::TextDisabled("Pln"); break;
            case ObjectType::Box:    ImGui::TextDisabled("Box"); break;
            case ObjectType::Mesh:   ImGui::TextDisabled("Msh"); break;
            default: ImGui::TextDisabled("???");; break;
        }
        ImGui::SameLine();

        bool value = i == selectedObjectId;
        std::string displayName = objects[i]->getName().length() > 0 ? objects[i]->getName() : "???";
        if (ImGui::Selectable(displayName.c_str(), value, ImGuiSelectableFlags_AllowDoubleClick)) {
            if (ImGui::IsMouseDoubleClicked(0))
                selectedObjectId = i;
        }
    }

    drawNewObjectPopUp(engine);
}

void Scene::drawNewObjectPopUp(VkSmol &engine) {
    if (!ImGui::BeginPopupModal("New Object", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove))
        return;
    
    if (ImGui::Button("Sphere", { 100, 0 })) {
        pushSphere(
            engine,
            "Sphere-" + std::to_string(objectId++),
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
            "Plane-" + std::to_string(objectId++),
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
            "Box-" + std::to_string(objectId++),
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

void Scene::drawSelectedUI(VkSmol &engine) {
    if (selectedObjectId < 0) return;

    std::string typeName;
    switch (objects[selectedObjectId]->getType()) {
        case ObjectType::Sphere:    typeName = "Sphere"; break;
        case ObjectType::Plane:     typeName = "Plane"; break;
        case ObjectType::Box:       typeName = "Box"; break;
        case ObjectType::Mesh:      typeName = "Mesh"; break;
        default: break; // UNREACHABLE
    }

    ImGui::SetNextWindowBgAlpha(0.6f);
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
        
        updated |= objects[selectedObjectId]->drawUI(materials);
        
        ImGui::Separator();
        if (ImGui::Button("Clone", { -FLT_MIN, 0 })) {
            switch(objects[selectedObjectId]->getType()) {
                case ObjectType::Sphere: {
                    Sphere *sphere = static_cast<Sphere*>(objects[selectedObjectId]);
                    pushSphere(
                        engine,
                        sphere->getName() + "-copy",
                        sphere->getStruct().center,
                        sphere->getStruct().radius,
                        materials[sphere->getStruct().materialHandle]
                    ); 
                } break;
                case ObjectType::Plane: {
                    Plane *plane = static_cast<Plane*>(objects[selectedObjectId]);
                    pushPlane(
                        engine,
                        plane->getName() + "-copy",
                        plane->getStruct().point,
                        plane->getStruct().normal,
                        materials[plane->getStruct().materialHandle]
                    ); 
                } break;
                case ObjectType::Box: {
                    Box *box = static_cast<Box*>(objects[selectedObjectId]);
                    pushBoxTransform(
                        engine,
                        box->getName() + "-copy",
                        box->getTransform(),
                        materials[box->getStruct().materialHandle]
                    );
                } break;
                case ObjectType::Mesh: {
                    Mesh *mesh = static_cast<Mesh*>(objects[selectedObjectId]);
                    pushMesh(
                        engine,
                        mesh->getName() + "-copy",
                        mesh->getVertices(),
                        mesh->getIndices(),
                        mesh->getTransform(),
                        materials[mesh->getStruct().materialHandle]
                    );
                } break;
                default: break;
            }

            selectedObjectId = objects.size()-1;
            updated = true;
        }

        ImGui::Separator();
        ImGui::PushStyleColor(ImGuiCol_Button, { 1.0, 0.03, 0.0, 1.0 });
        if (ImGui::Button("Delete", { -FLT_MIN, 0 })) {
            switch(objects[selectedObjectId]->getType()) {
                case ObjectType::Sphere: sphereBuffers.removeElement(); break;
                case ObjectType::Plane:  planeBuffers.removeElement(); break;
                case ObjectType::Box:    boxBuffers.removeElement(); break;
                case ObjectType::Mesh:   meshBuffers.removeElement(); break;
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


bool Scene::raycast(const glm::vec2 &screenPos, const glm::vec2 &screenSize, const Camera &camera, float &dist, glm::vec3 &p, bool select) {
    Ray ray = getRay(screenPos, screenSize, camera);
    float tClosest = std::numeric_limits<float>::infinity();
    int idClosest = -1;

    float t;
    int i = 0;
    for (Object *object : objects) {
        t = object->rayIntersection(ray);
        if (t >= 0.0f && t < tClosest) {
            tClosest = t;
            idClosest = i;
        }
        i++;
    }

    if (select) selectedObjectId = idClosest;
    dist = tClosest;
    p = ray.origin + dist * ray.dir;
    return idClosest >= 0;
}

std::vector<bufferList_t> Scene::getBufferLists() {
    std::vector<bufferList_t> bufferLists = {
        sphereBuffers.getBufferList(),
        planeBuffers.getBufferList(),
        boxBuffers.getBufferList(),
        vertexBuffers.getBufferList(),
        indexBuffers.getBufferList(),
        bvhBuffers.getBufferList(),
        meshBuffers.getBufferList(),
        materialBuffers.getBufferList(),
        objectBuffers.getBufferList(),
        lightBuffers.getBufferList(),
    };


    return bufferLists;
}

bool Scene::checkUpdate() {
    if (updated) {
        updated = false;
        return true;
    }
    return false;
}

bool Scene::checkBufferUpdate() {
    if (bufferUpdated) {
        bufferUpdated = false;
        return true;
    }
    return false;
}
