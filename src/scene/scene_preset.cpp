#include "scene_preset.hpp"

void initEmpty(Scene &scene, LightMode &lightMode) {
    scene.clear();

    lightMode = LightMode::Sunset;
}

void initMaterialZoo(Scene &scene, LightMode &lightMode) {
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
                    .albedo = glm::vec3(1.0, 0.1, 0.1) * 0.5f,
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

void initMesh(Scene &scene, LightMode &lightMode) {
    scene.clear();

    lightMode = LightMode::Day;

    // scene.pushMeshFromObj(
    //     engine,
    //     "Suzanne",
    //     "./res/model/suzanne.obj",
    //     Material {
    //         .type = MaterialType::Lambertian,
    //         .albedo = { 1.0f, 0.0f, 1.0f },
    //     },
    //     glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f))
    // );

    const MaterialHandle lucyHandle = scene.pushMaterial(Material{
        .name = "Lucy",
        .type = MaterialType::GgxMetal,
        .albedo = { 0.8f, 0.2f, 0.2f },
        .payload = { 0.4f, 0.0f },
    });
    scene.pushMesh(
        "Lucy",
        "./res/model/lucy.obj",
        glm::mat4(1.0f),
        lucyHandle
    );

    Material floorMat = {
        .type = MaterialType::Lambertian,
        .albedo = { 1.0, 1.0, 1.0 },
        .name = "Floor",
    };
    const MaterialHandle floorHandle = scene.pushMaterial(floorMat);
    scene.pushPlane(
        "Floor",
        glm::vec3(0.0, -2.0, 0.0),
        glm::vec3(0.0,  1.0 , 0.0),
        floorHandle
    );
}

void initSponza(Scene &scene, LightMode &lightMode) {
    scene.clear();

    lightMode = LightMode::Day;

    const MaterialHandle sponzaHandle = scene.pushMaterial(Material{
        .type = MaterialType::Lambertian,
        .albedo = { 1.0f, 1.0f, 1.0f },
        .name = "Sponza",
    });
    scene.pushMesh(
        "Sponza",
        "./res/model/sponza.obj",
        glm::scale(glm::mat4(1.0f), glm::vec3(0.1f)),
        sponzaHandle
    );
}

void initCornellBox(Scene &scene, LightMode &lightMode) {
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

    const MaterialHandle lucyHandle = scene.pushMaterial(Material{
        .name = "Lucy",
        .type = MaterialType::GgxMetal,
        .albedo = { 1.0f, 1.0f, 1.0f },
        .payload = { 3.0f, 0.1f },
    });
    scene.pushMesh(
        "Lucy",
        "./res/model/lucy.obj",
        glm::mat4(1.0f),
        lucyHandle
    );
    
    Material leftMat {
        .type = MaterialType::Lambertian,
        .albedo = { 1.0, 0.1, 0.1 },
        .name = "Left",
    };
    const MaterialHandle leftHandle = scene.pushMaterial(leftMat);
    scene.pushBox(
        "Left",
        glm::vec3(4.0,-4.0,-4.0),
        glm::vec3(4.1, 4.0, 4.0),
        leftHandle
    );
    
    Material rightMat {
        .type = MaterialType::Lambertian,
        .albedo = { 0.1, 1.0, 0.1 },
        .name = "Right",
    };
    const MaterialHandle rightHandle = scene.pushMaterial(rightMat);
    scene.pushBox(
        "Right",
        glm::vec3(-4.1,-4.0,-4.0),
        glm::vec3(-4.0, 4.0, 4.0),
        rightHandle
    );
    
    Material topMat {
        .type = MaterialType::Lambertian,
        .albedo = { 1.0, 1.0, 1.0 },
        .name = "Top",
    };
    const MaterialHandle topHandle = scene.pushMaterial(topMat);
    scene.pushBox(
        "Top",
        glm::vec3(-4.0, 4.0,-4.0),
        glm::vec3( 4.0, 4.1, 4.0),
        topHandle
    );
    
    Material bottomMat {
        .type = MaterialType::Lambertian,
        .albedo = { 1.0, 1.0, 1.0 },
        .name = "Bottom",
    };
    const MaterialHandle bottomHandle = scene.pushMaterial(bottomMat);
    scene.pushBox(
        "Bottom",
        glm::vec3(-4.0,-4.1,-4.0),
        glm::vec3( 4.0,-4.0, 4.0),
        bottomHandle
    );
    
    Material backMat {
        .type = MaterialType::Lambertian,
        .albedo = { 0.2, 0.2, 0.6 },
        .name = "Back",
    };
    const MaterialHandle backHandle = scene.pushMaterial(backMat);
    scene.pushBox(
        "Back",
        glm::vec3(-4.0,-4.0, 4.0),
        glm::vec3( 4.0, 4.0, 4.1),
        backHandle
    );
    
    Material lightBoxMat {
        .type = MaterialType::Emissive,
        .albedo = { 1.0, 0.7, 0.5 },
        .payload = { 30.0, 0.0 },
        .name = "Light",
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
void initRandomSpheres(Scene &scene, LightMode &lightMode) {
    srand(time(nullptr));

    scene.clear();

    lightMode = LightMode::Empty;

    Material floorMat = {
        .type = MaterialType::Lambertian,
        .albedo = { 1.0, 1.0, 1.0 },
        .name = "Floor",
    };
    const MaterialHandle floorHandle = scene.pushMaterial(floorMat);
    scene.pushPlane(
        "Floor",
        glm::vec3(0.0, -1.0, 0.0),
        glm::vec3(0.0,  1.0 , 0.0),
        floorHandle
    );
    
    Material lightMat = {
        .type = MaterialType::Emissive,
        .albedo = { 1.0, 1.0, 1.0 },
        .name = "Light",
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
