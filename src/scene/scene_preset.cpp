#include "scene_preset.hpp"
#include "scene/object/material.hpp"

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
        .type = MaterialType::Principled,
        .albedo = glm::vec3(1.0f, 0.05f, 0.10f),
        .roughness = 0.2,
        .ior = 1.3
    };
    cubeMaterial.roughness = 0.02f;
    cubeMaterial.ior = 1.5f;

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
            .emissionStrength = 10.0f,
        }
    );
    
    float v = 4.0f;
    for (int j = 0; j <= v; j++) {
        for (int i = 0; i <= v; i++) {
            float rough = i/v * 0.7f;
            float ior = 1.0f + j/v;
            MaterialHandle temp = scene.pushMaterial(
                Material {
                    .name = "Temp_" + std::to_string(i) + std::to_string(j),
                    .type = MaterialType::GgxGlossy,
                    .albedo = glm::vec3(0.0, 1.0, 0.0) * 0.8f,
                    .roughness = rough,
                    .ior = ior,
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
        .roughness = 0.1f,
        .ior = 4.0f,
    });
    glm::mat4 transform(1.0f);
    transform = glm::translate(transform, glm::vec3(0.0f, 1.2f, 0.0f));
    transform = glm::scale(transform, glm::vec3(4.7f));
    transform = glm::rotate(transform, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
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
        .emissionStrength = 50.0f,
    });
    scene.pushBox(
        "Light",
        glm::vec3(-2.0f, 14.99f, -2.0f),
        glm::vec3( 2.0f, 15.01f,  2.0f),
        lightHandle
    );
    
    const MaterialHandle largeLightHandle = scene.pushMaterial(Material{
        .name = "Large Light",
        .type = MaterialType::Emissive,
        .albedo = glm::vec3(0.6f, 0.6f, 0.9f),
        .emissionStrength = 5.0f,
    });
    scene.pushBox(
        "Large Light",
        glm::vec3(-20.0f, 49.99f, -20.0f),
        glm::vec3( 20.0f, 50.01f,  20.0f),
        largeLightHandle
    );
    
    const MaterialHandle baseHandle = scene.pushMaterial(Material{
        .name = "Base",
        .type = MaterialType::GgxGlossy,
        .albedo = glm::vec3(1.0f),
        .roughness = 0.05f,
        .ior = 1.5f,
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
        glm::scale(glm::mat4(1.0f), glm::vec3(0.04f)),
        sponzaHandle
    );
}

void initCornellBox(Scene& scene, LightMode& lightMode) {
    scene.clear();

    lightMode = LightMode::Empty;

    const float R = 4.0f;

    auto quad = [&](const std::string& name, Material mat, glm::vec3 center, glm::vec3 normal, glm::vec2 size) {
        scene.pushQuad(name, center, normal, size, 0.0f, scene.pushMaterial(mat));
    };

    const MaterialHandle objectHandle = scene.pushMaterial(Material{
        .name = "Object",
        .type = MaterialType::Dielectric,
        .albedo = { 1.0f, 0.5f, 0.5f },
        .roughness = 0.1f,
        .ior = 1.3f,
    });
    glm::mat4 transform(1.0f);
    transform = glm::translate(transform, glm::vec3(0.0, -2.2, 0.0));
    transform = glm::scale(transform, glm::vec3(5.0f));
    transform = glm::rotate(transform, glm::radians(200.0f), glm::vec3(0.0, 1.0, 0.0));
    scene.pushMesh("Object", "./res/model/dragon.obj", transform, objectHandle);

    const glm::vec2 wallSize(2.0f * R, 2.0f * R);
    quad("Left",   {.name="Left",   .type=MaterialType::Programmable, .albedo={1.0f,0.1f,0.1f}}, {-R, 0, 0}, { 1, 0, 0}, wallSize);
    quad("Right",  {.name="Right",  .type=MaterialType::Programmable, .albedo={0.1f,1.0f,0.1f}}, { R, 0, 0}, {-1, 0, 0}, wallSize);
    quad("Top",    {.name="Top",    .type=MaterialType::Programmable, .albedo={1.0f,1.0f,1.0f}}, {0,  R, 0}, { 0,-1, 0}, wallSize);
    quad("Bottom", {.name="Bottom", .type=MaterialType::Programmable, .albedo={1.0f,1.0f,1.0f}}, {0, -R, 0}, { 0, 1, 0}, wallSize);
    quad("Back",   {.name="Back",   .type=MaterialType::Programmable, .albedo={0.2f,0.2f,0.6f}}, {0, 0,  R}, { 0, 0,-1}, wallSize);
    quad("Front",  {.name="Front",  .type=MaterialType::Programmable, .albedo={1.0f,1.0f,1.0f}}, {0, 0, -R}, { 0, 0, 1}, wallSize);

    const MaterialHandle lightHandle = scene.pushMaterial(Material{
        .name = "Light",
        .type = MaterialType::Emissive,
        .albedo = { 1.0f, 0.7f, 0.5f },
        .emissionStrength = 30.0f,
    });
    scene.pushBox("Light", {-1.0f, R - 0.1f, -1.0f}, {1.0f, R, 1.0f}, lightHandle);
}


void initReferenceScene(Scene& scene, LightMode& lightMode) {
    scene.clear();
    lightMode = LightMode::Empty;

    const float R = 4.0f;

    // Camera: slightly further back, FOV tuned so the box front corners fill the frame
    scene.getCamera().setPosition({0.0f, 0.0f, -15.0f});
    scene.getCamera().setTarget({0.0f, 0.0f, 0.0f});
    scene.getCamera().setFov(40.0f);

    auto box = [&](const std::string& name, Material mat, glm::vec3 mn, glm::vec3 mx) {
        scene.pushBox(name, mn, mx, scene.pushMaterial(mat));
    };
    auto quad = [&](const std::string& name, Material mat, glm::vec3 center, glm::vec3 normal, glm::vec2 size) {
        scene.pushQuad(name, center, normal, size, 0.0f, scene.pushMaterial(mat));
    };
    auto sphere = [&](const std::string& name, Material mat, glm::vec3 pos, float r) {
        scene.pushSphere(name, pos, r, scene.pushMaterial(mat));
    };

    const MaterialHandle floorHandle = scene.pushMaterial({
        .name="Floor", .type=MaterialType::Programmable, .albedo={0.32f,0.32f,0.32f}
    });
    const glm::vec2 wallSize(2.0f * R, 2.0f * R);
    scene.pushQuad("Bottom", {0, -R, 0}, {0, 1, 0}, wallSize, 0.0f, floorHandle);
    quad("Top",   {.name="Top",   .type=MaterialType::Lambertian, .albedo={0.90f,0.90f,0.90f}}, {0,  R, 0}, { 0,-1, 0}, wallSize);
    quad("Back",  {.name="Back",  .type=MaterialType::GgxGlossy,  .albedo={0.10f,0.18f,0.72f}, .roughness=0.20f, .ior=1.5f}, {0, 0,  R}, { 0, 0,-1}, wallSize);
    quad("Left",  {.name="Left",  .type=MaterialType::GgxGlossy,  .albedo={0.06f,0.65f,0.08f}, .roughness=0.20f, .ior=1.5f}, {-R, 0, 0}, { 1, 0, 0}, wallSize);
    quad("Right", {.name="Right", .type=MaterialType::GgxGlossy,  .albedo={0.82f,0.06f,0.04f}, .roughness=0.20f, .ior=1.5f}, { R, 0, 0}, {-1, 0, 0}, wallSize);
    quad("Front", {.name="Front", .type=MaterialType::Lambertian,  .albedo={0.08f,0.08f,0.08f}}, {0, 0, -R}, { 0, 0, 1}, wallSize);

    box("Light", {.name="Light", .type=MaterialType::Emissive,
                  .albedo={1.0f,0.72f,0.38f}, .emissionStrength=400.0f},
        {-0.3f, R-0.02f, -0.3f}, {0.3f, R, 0.3f});
    
    box("Smoke", {.name="Smoke", .type=MaterialType::Volume,
                  .albedo={0.6f, 0.6f, 0.6f}, .density=0.03f, .anisotropic=0.0f},
        {-4.01f, -4.01f, -4.01f}, {4.01f, 4.01f, 4.01f});

    {
        constexpr float dragonScale      = 5.2f;
        constexpr float dragonTranslateY = -1.4f;
        constexpr float dragonFootY      = -0.33f; // model-space foot position
        const float pedestalTop = dragonTranslateY + dragonScale * dragonFootY;

        const MaterialHandle h = scene.pushMaterial({
            .name="DragonGlass", .type=MaterialType::Dielectric,
            .albedo={0.88f,0.38f,0.82f}, .roughness=0.10f, .ior=1.45f, .density=1.0f,
        });
        glm::mat4 t(1.0f);
        t = glm::translate(t, {0.0f, dragonTranslateY, 1.0f});
        t = glm::scale(t, glm::vec3(dragonScale));
        t = glm::rotate(t, glm::radians(205.0f), {0.0f, 1.0f, 0.0f});
        scene.pushMesh("Dragon", "./res/model/dragon.obj", t, h);

        box("Pedestal", {.name="Pedestal", .type=MaterialType::GgxGlossy,
                         .albedo={1.0f,1.0f,1.0f}, .roughness=0.0f, .ior=12.0f},
            {-3.25f, -R, -1.6f}, {3.25f, pedestalTop, 3.2f});
        scene.pushBox("PedestalBorder", {-3.29f, -R, -1.64f}, {3.29f, pedestalTop - 0.02f, 3.24f}, floorHandle);
    }

    const float sr   = 0.85f;
    const float sy   = -R + sr + 0.8f;
    const float sz   = -2.0f;
    const float sgap = (2.0f * R - 4.0f * 2.0f * sr) / 5.0f;
    const float step = 2.0f * sr + sgap;
    const float sx0  = -R + sgap + sr;
    sphere("Metal", {.name="Metal", .type=MaterialType::GgxMetal,
                    .albedo={1.0f,1.0f,1.0f}, .roughness=0.10f},
           {sx0, sy, sz}, sr);

    sphere("Matte", {.name="Matte", .type=MaterialType::Lambertian,
                      .albedo={1.0f,1.0f,1.0f}},
           {sx0 + step, sy, sz}, sr);

    sphere("FrostedGlass", {.name="FrostedGlass", .type=MaterialType::Dielectric,
                             .albedo={1.0f,1.0f,1.0f}, .roughness=0.08f, .ior=1.45f},
           {sx0 + 2.0f * step, sy, sz}, sr);

    sphere("Glossy", {.name="Glossy", .type=MaterialType::GgxGlossy,
                      .albedo={1.0f,1.0f,1.0f}, .roughness=0.04f, .ior=2.0f},
           {sx0 + 3.0f * step, sy, sz}, sr);
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
    lightMat.emissionStrength = 15.0f;
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
