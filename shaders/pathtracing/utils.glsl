#ifndef UTILS_GLSL
#define UTILS_GLSL

#define TRI_EPS 1e-8
#define EPS 1e-4
#define EPS_HIGH 1e-3   // @note arbitrary, but EPS is too small for GGX alpha clamping
#define PI 3.14159265
#define INFINITY abs(1.0/0.0)

#define Enum int

// ============== MATERIAL ==============
float luma(vec3 c) {
    return dot(c, vec3(0.2126, 0.7152, 0.0722));
}

#define mat_Principled   Enum(0)
#define mat_Emissive     Enum(1)
#define mat_Lambertian   Enum(2)
#define mat_GgxMetal     Enum(3)
#define mat_GgxGlossy    Enum(4)
#define mat_Dielectric   Enum(5)
#define mat_Programmable Enum(6)

struct Material {
    Enum type;
    vec3 albedo;
    float roughness;
    float metalness;
    float ior;
    float transmission;
    float emissionStrength;
    float density;
};

#define MaterialHandle int

// ============== OBJECTS  ==============
#define obj_None    Enum(0)
#define obj_Sphere  Enum(1)
#define obj_Plane   Enum(2)
#define obj_Box     Enum(3)
#define obj_Quad    Enum(4)
#define obj_Mesh    Enum(5)
#define obj_Aabb    Enum(6) // NOTE: only used internally

// ============== DEBUG VIEW ==============
#define debug_None          Enum(0)
#define debug_Bounces       Enum(1)
#define debug_Normal        Enum(2)
#define debug_Position      Enum(3)
#define debug_Diffuse       Enum(4)
#define debug_SelectionMask Enum(5)
#define debug_Variance      Enum(6)
#define debug_HitChecks     Enum(7)

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
};

// ============== PATH-TRACING  ==============
struct PixelInfo {
    vec4 normal;
    vec4 position;
    vec4 diffuse;
    float mean;
    float m2;
    int count;
    float varianceProba;
    int selectionMask;
};

struct Camera {
    vec3 pos;
    vec3 dir;
    vec3 up;
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
};
#define NO_HIT Hit(vec3(0), vec3(0), INFINITY, true, OBJECT_NONE)
#define foundIntersection(h) ((h).object.type != obj_None)

// ============== LIGHTS ==============
#define lightMode_Day    Enum(0)
#define lightMode_Sunset Enum(1)
#define lightMode_Night  Enum(2)
#define lightMode_Empty  Enum(3)

struct Light {
    int objectId;
    float area;
    float pdfA;     // 1 / area
};

struct SurfaceSample {
    vec3 p;
    vec3 normal;
};

#endif
