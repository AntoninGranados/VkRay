#include "scene_preset.hpp"

void initEmpty(VkSmol &engine, Scene &scene, LightMode &lightMode) {
    scene.clear(engine);

    lightMode = LightMode::Day;

    scene.pushMeshFromObj(
        engine,
        "Monkey",
        "./res/model/monkey.obj",
        Material {
            .type = MaterialType::Lambertian,
            .albedo = { 0.9f, 0.9f, 0.9f },
        },
        glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f))
    );

    Material floorMat = {};
    floorMat.type = MaterialType::Lambertian;
    floorMat.albedo = { 1.0, 1.0, 1.0 };
    scene.pushPlane(
        engine,
        "Floor",
        glm::vec3(0.0, -1.0, 0.0),
        glm::vec3(0.0,  1.0 , 0.0),
        floorMat
    );
}

void initCornellBox(VkSmol &engine, Scene &scene, LightMode &lightMode) {
    scene.clear(engine);

    lightMode = LightMode::Empty;
    
    scene.pushSphere(
        engine,
        "Glass",
        glm::vec3(-2.0, 0.0, 0.0),
        1.5,
        Material {
            .type = MaterialType::Glossy,
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

    scene.pushBox(
        engine,
        "Left",
        glm::vec3(4.0,-4.0,-4.0),
        glm::vec3(4.1, 4.0, 4.0),
        Material {
            .type = MaterialType::Lambertian,
            .albedo = { 1.0, 0.1, 0.1 },
        }
    );
    
    scene.pushBox(
        engine,
        "Right",
        glm::vec3(-4.1,-4.0,-4.0),
        glm::vec3(-4.0, 4.0, 4.0),
        Material {
            .type = MaterialType::Lambertian,
            .albedo = { 0.1, 1.0, 0.1 },
        }
    );
    
    scene.pushBox(
        engine,
        "Top",
        glm::vec3(-4.0, 4.0,-4.0),
        glm::vec3( 4.0, 4.1, 4.0),
        Material {
            .type = MaterialType::Lambertian,
            .albedo = { 1.0, 1.0, 1.0 },
        }
    );
    
    scene.pushBox(
        engine,
        "Bottom",
        glm::vec3(-4.0,-4.1,-4.0),
        glm::vec3( 4.0,-4.0, 4.0),
        Material {
            .type = MaterialType::Lambertian,
            .albedo = { 1.0, 1.0, 1.0 },
        }
    );
    
    scene.pushBox(
        engine,
        "Back",
        glm::vec3(-4.0,-4.0, 4.0),
        glm::vec3( 4.0, 4.0, 4.1),
        Material {
            .type = MaterialType::Lambertian,
            .albedo = { 0.2, 0.2, 0.6 },
        }
    );
    
    scene.pushBox(
        engine,
        "Light",
        glm::vec3(-2.0, 3.9,-2.0),
        glm::vec3( 2.0, 4.0, 2.0),
        Material {
            .type = MaterialType::Emissive,
            .albedo = { 1.0, 0.7, 0.5 },
            .payload = { 5.0, 0.0 },
        }
    );
}

#define RAND_FLOAT static_cast<float>(rand() % 100000) / 100000.0f
void initRandomSpheres(VkSmol &engine, Scene &scene, LightMode &lightMode) {
    srand(time(nullptr));

    scene.clear(engine);

    lightMode = LightMode::Empty;

    Material floorMat = {};
    floorMat.type = MaterialType::Lambertian;
    floorMat.albedo = { 1.0, 1.0, 1.0 };
    scene.pushPlane(
        engine,
        "Floor",
        glm::vec3(0.0, -1.0, 0.0),
        glm::vec3(0.0,  1.0 , 0.0),
        floorMat
    );
    
    Material lightMat = {};
    lightMat.type = MaterialType::Emissive;
    lightMat.albedo = { 1.0, 1.0, 1.0 };
    emissiveIntensity(lightMat) = 15.0f;
    scene.pushSphere(
        engine,
        "Light",
        glm::vec3(0.0, 15.0, 0.0),
        3.0f,
        lightMat
    );

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
            sphereMat.type = MaterialType::Glossy;
            glossyIoR(sphereMat) = 1.5f;
            glossyFuzz(sphereMat) = 0.0f;
        }

        scene.pushSphere(
            engine,
            std::string("Sphere" + std::to_string(i)),
            pos,
            1.0,
            sphereMat
        );
        i += 1;
    }}
}
