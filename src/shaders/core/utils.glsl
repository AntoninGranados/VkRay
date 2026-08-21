#ifndef UTILS_GLSL
#define UTILS_GLSL

#include "../common.glsl"

#define TRI_EPS 1e-8
#define EPS 1e-4
#define EPS_HIGH 1e-3   // @note arbitrary, but EPS is too small for GGX alpha clamping
#define PI 3.14159265
#define INFINITY abs(1.0/0.0)

#define Enum int

#define MaterialHandle int

// ============== OBJECTS  ==============
#define obj_None    Enum(0)
#define obj_Sphere  Enum(1)
#define obj_Plane   Enum(2)
#define obj_Box     Enum(3)
#define obj_Quad    Enum(4)
#define obj_Mesh    Enum(5)
#define obj_Aabb    Enum(6) // NOTE: only used internally


struct Object {
    Enum type;
    uint id;
};

#define OBJECT_NONE Object(obj_None, -1)
#define OBJECT_AABB Object(obj_Aabb, -1)

struct Sphere {
    vec3 center;
    float radius;
    MaterialHandle materialHandle;
};

struct Plane {
    vec3 point;
    vec3 normal;
    MaterialHandle materialHandle;
};

struct Box {
    mat4 modelMatrix;
    mat4 invModelMatrix;
    MaterialHandle materialHandle;
};

struct Quad {
    vec3 point;
    vec3 u;
    vec3 v;
    vec3 normal;
    MaterialHandle materialHandle;
};

struct Vertex {
    vec3 position;
    vec3 normal;
    vec3 color;
};

struct BvhChild {
    float minX, minY, minZ;
    float maxX, maxY, maxZ;
    uint index;
};

struct BvhNode {
    BvhChild children[2];
    uint firstTriangle;
    uint triangleCount;
};
#define BVH_STACK_SIZE 64

struct Mesh {
    mat4 modelMatrix;
    mat4 invModelMatrix;
    uint indexOffset;
    uint triangleCount;
    uint bvhOffset;
    uint bvhNodeCount;
    float aabbMinX, aabbMinY, aabbMinZ;
    float aabbMaxX, aabbMaxY, aabbMaxZ;
    MaterialHandle materialHandle;
    uint smoothShading;
    uint hasVertexColor;
};

// ============== PATH-TRACING  ==============
struct Camera {
    vec3 eye;
    vec3 U;
    vec3 V;
    vec3 W;
};

struct Ray {
    vec3 origin;
    vec3 dir;
};

struct Statistics {
    uint bvhChecks;
    uint triangleChecks;
};

struct Hit {
    vec3 p;
    vec3 normal;
    float t;
    bool frontFace;
    Object object;
    vec3 vertexColor;
};
#define NO_HIT Hit(vec3(0), vec3(0), INFINITY, true, OBJECT_NONE, vec3(1.0))
#define foundIntersection(h) ((h).object.type != obj_None)

// ============== LIGHTS ==============
#define lightMode_Day    Enum(0)
#define lightMode_Sunset Enum(1)
#define lightMode_Night  Enum(2)
#define lightMode_Empty  Enum(3)
#define lightMode_Studio Enum(4)

struct Light {
    int objectId;
    float area;
    float pdfA;     // 1 / area
};

struct SurfaceSample {
    vec3 p;
    vec3 normal;
};

// ============== MIS ==============
float powerHeuristic(float fPdf, float gPdf) {
    float f2 = fPdf * fPdf;
    float g2 = gPdf * gPdf;
    return (f2 + g2 > 0.0) ? f2 / (f2 + g2) : 0.0;
}

float balanceHeuristic(float fPdf, float gPdf) {
    float sum = fPdf + gPdf;
    return (sum > 0.0) ? fPdf / sum : 0.0;
}

#endif
