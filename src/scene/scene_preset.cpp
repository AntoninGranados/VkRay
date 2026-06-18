#include "scene_preset.hpp"
#include "ecs/components/animation/transform_anim.hpp"
#include "ecs/components/objects/camera_object.hpp"
#include "scene/object/material.hpp"

#include <cmath>

void initEmpty(Scene& scene, LightMode& lightMode) {
    scene.clear();

    lightMode = LightMode::Day;

    const MaterialHandle floorHandle = scene.pushMaterial(Material{
        .name = "Floor",
        .type = MaterialType::Programmable,
        .albedo = glm::vec3(0.75f, 0.75f, 0.78f),
    });
    scene.pushPlane(
        "Floor",
        glm::vec3(0.0f, -1.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f),
        floorHandle
    );

    Material cubeMaterial = Material{
        .name = "Cube",
        .type = MaterialType::GgxGlossy,
        .albedo = glm::vec3(1.0f, 0.05f, 0.10f),
    };
    ggxGlossyRoughness(cubeMaterial) = 0.02;
    ggxGlossyIoR(cubeMaterial) = 0.7;

    const MaterialHandle cubeHandle = scene.pushMaterial(cubeMaterial);
    scene.pushBox(
        "Cube",
        glm::vec3(-1.0f),
        glm::vec3( 1.0f),
        cubeHandle
    );
};

void initPyramid(Scene& scene, LightMode& lightMode) {
    scene.clear();

    lightMode = LightMode::Sunset;

    const MaterialHandle floorHandle = scene.pushMaterial(Material{
        .name = "Floor",
        .type = MaterialType::Lambertian,
        .albedo = glm::vec3(0.75f, 0.75f, 0.78f),
    });
    scene.pushPlane(
        "Floor",
        glm::vec3(0.0f, -1.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f),
        floorHandle
    );

    const MaterialHandle boxHandle = scene.pushMaterial(Material{
        .name = "PyramidBox",
        .type = MaterialType::Lambertian,
        .albedo = glm::vec3(0.75f, 0.35f, 0.25f),
    });

    const int baseCount = 5;
    const float halfExtent = 0.5f;
    const float spacing = halfExtent * 2.0f + 0.02f;
    const float yBase = -1.0f + halfExtent;
    for (int level = 0; level < baseCount; ++level) {
        const int count = baseCount - level;
        const float y = yBase + static_cast<float>(level) * spacing;
        const float xOffset = -0.5f * static_cast<float>(count - 1) * spacing;
        for (int i = 0; i < count; ++i) {
            const float x = xOffset + static_cast<float>(i) * spacing;
            const glm::vec3 center(x, y, 0.0f);
            scene.pushBox(
                "PyramidBox_" + std::to_string(level) + "_" + std::to_string(i),
                center - glm::vec3(halfExtent),
                center + glm::vec3(halfExtent),
                boxHandle
            );
        }
    }

    ecs::Registry& registry = scene.getRegistry();
    auto& boxes = registry.storage<ecs::Box>();
    auto& planes = registry.storage<ecs::Plane>();

    for (const ecs::Entity& e : boxes.entities()) {
        if (!registry.has<ecs::Collider>(e)) registry.add<ecs::Collider>(e, ecs::Collider{});
        if (!registry.has<ecs::RigidBody>(e)) registry.add<ecs::RigidBody>(e, ecs::RigidBody{});
    }

    for (const ecs::Entity& e : planes.entities()) {
        if (!registry.has<ecs::Collider>(e)) {
            ecs::Collider collider {};
            collider.restitution = 0.2f;
            collider.friction = 0.8f;
            registry.add<ecs::Collider>(e, collider);
        }
    }
}

void initMaterialZoo(Scene& scene, LightMode& lightMode) {
    scene.clear();

    lightMode = LightMode::Empty;
    
    MaterialHandle light = scene.pushMaterial(
        Material {
            .name = "Light",
            .type = MaterialType::Emissive,
            .albedo = glm::vec3(1.0),
            .payload = { 10.0, 0.0 }
        }
    );
    
    float v = 4.0f;
    for (int j = 0; j <= v; j++) {
        for (int i = 0; i <= v; i++) {
            float rough = i/v * 0.7f;
            float ior = j/v;
            MaterialHandle temp = scene.pushMaterial(
                Material {
                    .name = "Temp_" + std::to_string(i) + std::to_string(j),
                    .type = MaterialType::GgxGlossy,
                    .albedo = glm::vec3(0.0, 1.0, 0.0) * 0.8f,
                    .payload = { rough, ior }
                }
            );
            scene.pushSphere(
                "Sphere_" + std::to_string(i) + std::to_string(j),
                glm::vec3(i - v*0.5f, 0.0f, j - v*0.5f),
                0.4,
                temp
            );
        }
        scene.pushBox(
            "Light_" + std::to_string(j),
            glm::vec3(-v*0.5f, 2.995f, j - v*0.5f + -0.1f),
            glm::vec3( v*0.5f, 3.005f, j - v*0.5f +  0.1f),
            light
        );
    };
    

    MaterialHandle floor = scene.pushMaterial(
        Material {
            .name = "Floor",
            .type = MaterialType::Programmable,
            .albedo = glm::vec3(0.59, 0.27, 0.09),
            .payload = { 0.0, 0.0 }
        }
    );
    scene.pushPlane(
        "Floor",
        glm::vec3(0.0, -1.0, 0.0),
        glm::vec3(0.0, 1.0, 0.0),
        floor
    );
}

void initMesh(Scene& scene, LightMode& lightMode) {
    scene.clear();

    lightMode = LightMode::Empty;

    const MaterialHandle objectHandle = scene.pushMaterial(Material{
        .name = "Object",
        .type = MaterialType::GgxGlossy,
        .albedo = { 0.1f, 0.3f, 0.6f },
        .payload = { 0.1f, 0.4f },
    });
    glm::mat4 transform(1.0f);
    transform = glm::translate(transform, glm::vec3(0.0f, 1.2f, 0.0f));
    transform = glm::scale(transform, glm::vec3(4.7f));
    transform = glm::rotate(transform, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    // transform = glm::rotate(transform, glm::radians(-40.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    scene.pushMesh(
        "Object",
        "./res/model/dragon.obj",
        transform,  
        objectHandle
    );
    
    const MaterialHandle lightHandle = scene.pushMaterial(Material{
        .name = "Light",
        .type = MaterialType::Emissive,
        .albedo = glm::vec3(1.0f),
        .payload = { 50.0f },
    });
    scene.pushBox(
        "Light",
        glm::vec3(-2.0f, 14.99f, -2.0f),
        glm::vec3( 2.0f, 15.01f,  2.0f),
        lightHandle
    );
    
    const MaterialHandle baseHandle = scene.pushMaterial(Material{
        .name = "Base",
        .type = MaterialType::GgxGlossy,
        .albedo = glm::vec3(1.0f),
        .payload = { 0.05f, 0.1f },
    });
    scene.pushBox(
        "Base",
        glm::vec3(-3.0f,  -3.0f, -2.0f),
        glm::vec3( 3.0f, -0.45f,  2.0f),
        baseHandle
    );
    
    Material floorMat = {
        .name = "Floor",
        .type = MaterialType::Lambertian,
        .albedo = glm::vec3(0.04f),
    };
    const MaterialHandle floorHandle = scene.pushMaterial(floorMat);
    
    scene.pushBox(
        "Base Border",
        glm::vec3(-3.1f, -3.00f, -2.1f),
        glm::vec3( 3.1f, -0.46f,  2.1f),
        floorHandle
    );
    scene.pushPlane(
        "Floor",
        glm::vec3(0.0, -2.0, 0.0),
        glm::vec3(0.0,  1.0 , 0.0),
        floorHandle
    );

    {
        constexpr int   GRID_H    = 24;     // horizontal (azimuth) samples
        constexpr int   GRID_V    = 24;     // vertical (elevation) samples
        constexpr float HALF_SPAN = 30.0f;  // degrees from center to edge
        constexpr float BASE_AZIM = 180.0f; // front-facing direction of the bunny
        constexpr float BASE_ELEV = 10.0f;  // degrees above horizontal (camera slightly above, looking down)
        constexpr float RADIUS    = 8.0f;
        const     glm::vec3 target(0.0f, 1.5f, 0.0f);

        auto spherePose = [&](float azimDeg, float elevDeg) -> std::pair<glm::vec3, glm::quat> {
            const float az = glm::radians(azimDeg);
            const float el = glm::radians(elevDeg);
            const glm::vec3 pos = target + RADIUS * glm::vec3(
                std::cos(el) * std::sin(az),
                std::sin(el),
                std::cos(el) * std::cos(az)
            );
            const glm::vec3 d = glm::normalize(target - pos);
            const glm::vec3 r = glm::normalize(glm::cross(d, glm::vec3(0.0f, 1.0f, 0.0f)));
            const glm::vec3 u = glm::cross(r, d);
            return { pos, glm::quat_cast(glm::mat3(r, u, -d)) };
        };

        auto [initPos, initRot] = spherePose(BASE_AZIM, BASE_ELEV);
        scene.pushCamera("GridCamera", glm::translate(glm::mat4(1.0f), initPos) * glm::mat4_cast(initRot));

        ecs::Entity camEntity = scene.getRegistry().storage<ecs::CameraObject>().entities().back();
        ecs::TransformAnim anim;

        for (int v = 0; v < GRID_V; ++v) {
            const float elev = BASE_ELEV + (-HALF_SPAN + v * (2.0f * HALF_SPAN / (GRID_V - 1)));
            for (int h = 0; h < GRID_H; ++h) {
                const float azim = BASE_AZIM + (-HALF_SPAN + h * (2.0f * HALF_SPAN / (GRID_H - 1)));
                auto [pos, rot] = spherePose(azim, elev);
                anim.insertPositionKeyframe(v * GRID_H + h, pos);
                anim.insertRotationKeyframe(v * GRID_H + h, rot);
            }
        }

        scene.getRegistry().add<ecs::TransformAnim>(camEntity, anim);
    }
}

void initSponza(Scene& scene, LightMode& lightMode) {
    scene.clear();

    lightMode = LightMode::Day;

    const MaterialHandle sponzaHandle = scene.pushMaterial(Material{
        .name = "Sponza",
        .type = MaterialType::Lambertian,
        .albedo = { 1.0f, 1.0f, 1.0f },
    });
    scene.pushMesh(
        "Sponza",
        "./res/model/sponza.obj",
        glm::scale(glm::mat4(1.0f), glm::vec3(0.1f)),
        sponzaHandle
    );
}

void initCornellBox(Scene& scene, LightMode& lightMode) {
    scene.clear();

    lightMode = LightMode::Empty;
    
    /*
    scene.pushSphere(
        engine,
        "GgxMetal",
        glm::vec3(-2.0, 0.0, 0.0),
        1.5,
        Material {
            .type = MaterialType::GgxMetal,
            .albedo = { 1.0, 0.1, 0.9 },
            .payload = { 1.5, 0.0 },
        }
    );
    
    scene.pushSphere(
        engine,
        "Metal",
        glm::vec3( 2.0, 0.0, 0.0),
        1.5,
        Material {
            .type = MaterialType::Metal,
            .albedo = { 0.2, 0.4, 0.8 },
            .payload = { 0.01, 0.0 },
        }
    );
    */

    /*
    scene.pushMeshFromObj(
        engine,
        "Suzanne",
        "./res/model/suzanne.obj",
        Material {
            .type = MaterialType::GgxMetal,
            .albedo = { 1.0f, 1.0f, 1.0f },
            .payload = { 3.0f, 0.1f },
        },
        glm::rotate(glm::scale(glm::mat4(1.0f), glm::vec3(3.0f)), glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f))
    );
    */

    const MaterialHandle objectHandle = scene.pushMaterial(Material{
        .name = "Lucy",
        .type = MaterialType::Dielectric,
        .albedo = { 1.0f, 0.5f, 0.5f },
        .payload = { 1.3f, 0.1f },
    });
    scene.pushMesh(
        "Object",
        "./res/model/lucy.obj",
        glm::scale(glm::mat4(1.0f), glm::vec3(1.0f)),
        // "./res/model/armadillo.obj",
        // glm::scale(glm::mat4(1.0f), glm::vec3(0.04f)),
        objectHandle
    );
    
    Material leftMat {
        .name = "Left",
        .type = MaterialType::Lambertian,
        .albedo = { 1.0, 0.1, 0.1 },
    };
    const MaterialHandle leftHandle = scene.pushMaterial(leftMat);
    scene.pushBox(
        "Left",
        glm::vec3(4.0,-4.0,-4.0),
        glm::vec3(4.1, 4.0, 4.0),
        leftHandle
    );
    
    Material rightMat {
        .name = "Right",
        .type = MaterialType::Lambertian,
        .albedo = { 0.1, 1.0, 0.1 },
    };
    const MaterialHandle rightHandle = scene.pushMaterial(rightMat);
    scene.pushBox(
        "Right",
        glm::vec3(-4.1,-4.0,-4.0),
        glm::vec3(-4.0, 4.0, 4.0),
        rightHandle
    );
    
    Material topMat {
        .name = "Top",
        .type = MaterialType::Lambertian,
        .albedo = { 1.0, 1.0, 1.0 },
    };
    const MaterialHandle topHandle = scene.pushMaterial(topMat);
    scene.pushBox(
        "Top",
        glm::vec3(-4.0, 4.0,-4.0),
        glm::vec3( 4.0, 4.1, 4.0),
        topHandle
    );
    
    Material bottomMat {
        .name = "Bottom",
        .type = MaterialType::Lambertian,
        .albedo = { 1.0, 1.0, 1.0 },
    };
    const MaterialHandle bottomHandle = scene.pushMaterial(bottomMat);
    scene.pushBox(
        "Bottom",
        glm::vec3(-4.0,-4.1,-4.0),
        glm::vec3( 4.0,-4.0, 4.0),
        bottomHandle
    );
    
    Material backMat {
        .name = "Back",
        .type = MaterialType::Lambertian,
        .albedo = { 0.2, 0.2, 0.6 },
    };
    const MaterialHandle backHandle = scene.pushMaterial(backMat);
    scene.pushBox(
        "Back",
        glm::vec3(-4.0,-4.0, 4.0),
        glm::vec3( 4.0, 4.0, 4.1),
        backHandle
    );
    
    Material lightBoxMat {
        .name = "Light",
        .type = MaterialType::Emissive,
        .albedo = { 1.0, 0.7, 0.5 },
        .payload = { 30.0, 0.0 },
    };
    const MaterialHandle lightHandle = scene.pushMaterial(lightBoxMat);
    scene.pushBox(
        "Light",
        glm::vec3(-1.0, 3.9,-1.0),
        glm::vec3( 1.0, 4.0, 1.0),
        lightHandle
    );
}

#define RAND_FLOAT static_cast<float>(rand() % 100000) / 100000.0f
void initRandomSpheres(Scene& scene, LightMode& lightMode) {
    srand(time(nullptr));

    scene.clear();

    lightMode = LightMode::Empty;

    Material floorMat = {
        .name = "Floor",
        .type = MaterialType::Lambertian,
        .albedo = { 1.0, 1.0, 1.0 },
    };
    const MaterialHandle floorHandle = scene.pushMaterial(floorMat);
    scene.pushPlane(
        "Floor",
        glm::vec3(0.0, -1.0, 0.0),
        glm::vec3(0.0,  1.0 , 0.0),
        floorHandle
    );
    
    Material lightMat = {
        .name = "Light",
        .type = MaterialType::Emissive,
        .albedo = { 1.0, 1.0, 1.0 },
    };
    emissiveIntensity(lightMat) = 15.0f;
    const MaterialHandle lightHandle = scene.pushMaterial(lightMat);
    scene.pushSphere(
        "Light",
        glm::vec3(0.0, 15.0, 0.0),
        3.0f,
        lightHandle
    );

    //! update with the new materials
    /*
    Material sphereMat = {};
    int i = 0;
    for (float x = -10.0f; x <= 10.0f; x+=4.0f) {
    for (float y = -10.0f; y <= 10.0f; y+=4.0f) {
        glm::vec3 pos(x, 0.0f, y);
        pos += glm::vec3(RAND_FLOAT - 0.5f, 0.0f, RAND_FLOAT - 0.5f) * 2.0f;
        
        float r = RAND_FLOAT;
        sphereMat = {};
        sphereMat.albedo = { RAND_FLOAT, RAND_FLOAT, RAND_FLOAT };
        if (r <= 0.25f) {
            sphereMat.type = MaterialType::Lambertian;
        } else if (r <= 0.50f) {
            sphereMat.type = MaterialType::Dielectric;
            dielectricIoR(sphereMat) = 1.5f;
        } else if (r <= 0.75f) {
            sphereMat.type = MaterialType::Metal;
            metalFuzz(sphereMat) = RAND_FLOAT;
        } else if (r <= 1.0f) {
            sphereMat.type = MaterialType::GgxMetal;
            GgxMetalIoR(sphereMat) = 1.5f;
            GgxMetalFuzz(sphereMat) = 0.0f;
        }

        const MaterialHandle sphereHandle = scene.pushMaterial(sphereMat);
        sphereMat.name = "Sphere-" + std::to_string(i);
        scene.pushSphere(
            std::string("Sphere" + std::to_string(i)),
            pos,
            1.0,
            sphereHandle
        );
        i += 1;
    }}
    */
}
