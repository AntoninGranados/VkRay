#pragma once

#include <cstddef>
#include <memory>
#include <unordered_map>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "../registry.hpp"
#include "../components/objects/plane.hpp"
#include "../components/objects/plane_collider.hpp"
#include "../components/transform.hpp"
#include "../../app_context.hpp"
#include "../../scene/asset/mesh.hpp"

namespace ecs {

namespace physics_detail {

struct BodyAttributes {
    BodyAttributes();

    float M;
    glm::mat3 I0, I0inv;
    glm::mat3 Iinv;

    glm::vec3 X;
    glm::mat3 R;
    glm::quat q;
    glm::vec3 P;
    glm::vec3 L;

    glm::vec3 V;
    glm::vec3 omega;

    glm::vec3 F;
    glm::vec3 tau;

    std::vector<glm::vec3> vdata0;
};

struct BodySnapshot {
    glm::vec3 X;
    glm::mat3 R;
    glm::quat q;
    glm::vec3 P;
    glm::vec3 L;
    glm::vec3 V;
    glm::vec3 omega;
    glm::mat3 Iinv;
};

struct FrameSnapshot {
    BodySnapshot body;
    glm::vec3 position;
    glm::quat rotation;
    glm::vec3 linearVelocity;
    glm::vec3 angularVelocity;
};

class RigidBox : public BodyAttributes {
public:
    explicit RigidBox(
        float w = 1.0f, float h = 1.0f, float d = 1.0f, float dens = 50.0f,
        glm::vec3 v0 = glm::vec3(0, 0, 0), glm::vec3 omega0 = glm::vec3(0, 0, 0));

    float width, height, depth;
};

class RigidSolver {
public:
    explicit RigidSolver(BodyAttributes* body0 = nullptr, glm::vec3 g = glm::vec3(0, 0, 0));

    void init(BodyAttributes* body0);
    void setGravity(const glm::vec3& g);
    void step(
        float dt,
        const ComponentStorage<PlaneCollider>& planeColliders,
        const ComponentStorage<Plane>& planes,
        const ComponentStorage<Transform>& transforms
    );

    BodyAttributes* body = nullptr;

private:
    void integrate(float dt);
    void computeForceAndTorque();
    void resolvePlaneCollision(const glm::vec3& p0, const glm::vec3& n, float eps, float mu);

    glm::vec3 gravity;
    std::size_t stepCount;
    float simTime;
    float appTime;

    float simDt = 0.0001f;
};

struct SolverState {
    MeshHandle meshHandle = -1;
    glm::vec3 localCenterScaled { 0.0f, 0.0f, 0.0f };
    std::unique_ptr<RigidBox> body;
    RigidSolver solver;
    int initializedFrame = 0;
    std::unordered_map<int, FrameSnapshot> snapshots;
};

void computeMeshBounds(const MeshAsset& mesh, glm::vec3& outMin, glm::vec3& outMax);
float safeExtent(float v);

} // namespace physics_detail

void physicsSolverSystem(Registry& registry, AppContext& ctx);

} // namespace ecs
