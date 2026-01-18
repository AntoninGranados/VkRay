#ifndef UTILS_GLSL
#define UTILS_GLSL

#define EPS 1e-3
#define PI 3.14159265
#define INFINITY abs(1.0/0.0)

#define Enum int

// ============== MATERIAL ==============
#define mat_Lambertian    Enum(0)
#define mat_Metal         Enum(1)
#define mat_Dielectric    Enum(2)
#define mat_Emissive      Enum(3)
#define mat_Glossy        Enum(4)
#define mat_Checkerboard  Enum(5)

struct Material {
    Enum type;
    vec3 albedo;
    float payload[2];
};

#define MaterialHandle int

// ============== OBJECTS  ==============
#define obj_None    Enum(0)
#define obj_Sphere  Enum(1)
#define obj_Plane   Enum(2)
#define obj_Aabb    Enum(3)
#define obj_Box     Enum(4)
#define obj_Mesh    Enum(5)

// ============== DEBUG VIEW ==============
#define debug_None          Enum(0)
#define debug_Bounces       Enum(1)
#define debug_Normal        Enum(2)
#define debug_SelectionMask Enum(3)

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

struct Vertex {
    vec3 position;
};

struct BvhNode {
    vec3 aabbMin;
    vec3 aabbMax;
    uint data0; // left or first triangle
    uint data1; // right or triangle count
    uint isLeaf;
};

#define BVH_childLeft(node)   (node.data0)
#define BVH_childRight(node)  (node.data1)
#define BVH_firstTriangle(node) (node.data0)
#define BVH_triangleCount(node) (node.data1)

struct Mesh {
    mat4 modelMatrix;
    mat4 invModelMatrix;
    uint indexOffset;
    uint triangleCount;
    uint bvhOffset;
    uint bvhNodeCount;
    MaterialHandle materialHandle;
};

// ============== PATH-TRACING  ==============
struct Camera {
    vec3 pos;
    vec3 dir;
    vec3 up;
};

struct Ray {
    vec3 origin;
    vec3 dir;
};

struct Hit {
    vec3 p;
    vec3 normal;
    float t;
    bool front_face;
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
