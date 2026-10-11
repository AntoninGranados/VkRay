__exported import "../common.slang";

static const float TRI_EPS = 1e-8;
static const float EPS = 1e-4;
static const float EPS_HIGH = 1e-3;  // @note arbitrary, but EPS is too small for GGX alpha clamping
static const float PI = 3.14159265;
static const float INFINITY = 1.0 / 0.0;

typealias Enum = int;

typealias MaterialSlot = int;

float mod(float x, float y) {
    return x - y * floor(x / y);
}

vector<float, N> mod<let N : int>(vector<float, N> x, vector<float, N> y) {
    return x - y * floor(x / y);
}

vector<float, N> mod<let N : int>(vector<float, N> x, float y) {
    return x - y * floor(x / y);
}

int2 textureSize(Sampler2D tex) {
    uint width, height;
    tex.GetDimensions(width, height);
    return int2(width, height);
}

int2 imageSize(RWTexture2D<float4> image) {
    uint width, height;
    image.GetDimensions(width, height);
    return int2(width, height);
}

// ============== OBJECTS  ==============
static const Enum obj_None    = 0;
static const Enum obj_Sphere  = 1;
static const Enum obj_Plane   = 2;
static const Enum obj_Box     = 3;
static const Enum obj_Quad    = 4;
static const Enum obj_Mesh    = 5;
static const Enum obj_Aabb    = 6; // NOTE: only used internally


struct Object {
    Enum type;
    uint id;
    MaterialSlot materialSlot;
    uint motionOffset;
    int lightId;
};

static const Object OBJECT_NONE = Object(obj_None, -1, 0, 0, -1);
static const Object OBJECT_AABB = Object(obj_Aabb, -1, 0, 0, -1);

struct MotionSample {
    float4 rotation;
    float3 translation;
    float3 scale;
};

struct Vertex {
    float3 position;
    float3 normal;
    float3 color;
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
static const int BVH_STACK_SIZE = 64;

struct Mesh {
    uint indexOffset;
    uint triangleCount;
    uint bvhOffset;
    uint bvhNodeCount;
    float aabbMinX, aabbMinY, aabbMinZ;
    float aabbMaxX, aabbMaxY, aabbMaxZ;
    uint smoothShading;
    uint hasVertexColor;
};

// ============== PATH-TRACING  ==============
static const Enum projection_Perspective  = 0;
static const Enum projection_Orthographic = 1;

struct ThinLensUBO {
    float lensRadius;
    float focusDistance;
};

struct CameraUBO {
    float U;
    float V;
    ThinLensUBO thinLens;
    uint motionOffset;
    int lensSlot;
    Enum projection;
    int lensParamsBase;
};

struct CameraPose {
    float3 eye;
    float3 dir;
    float3 right;
    float3 up;
};

struct Ray {
    float3 origin;
    float3 dir;
};

Ray cameraRay(float2 ndc_pos, CameraPose pose, Enum projection, float U, float V) {
    if (projection == projection_Orthographic) {
        float3 origin = pose.eye + ndc_pos.x * U * pose.right - ndc_pos.y * V * pose.up;
        return Ray(origin, pose.dir);
    }
    float3 dir = normalize(ndc_pos.x * pose.right * U - ndc_pos.y * pose.up * V + pose.dir);
    return Ray(pose.eye, dir);
}

struct LensSample {
    float3 origin;
    float3 direction;
};

struct Statistics {
    uint bvhChecks;
    uint triangleChecks;
};

struct Hit {
    float3 p;
    float2 uv;
    float3 normal;
    float t;
    bool frontFace;
    Object object;
    float3 vertexColor;
};
static const Hit NO_HIT = Hit(float3(0), float2(0), float3(0), INFINITY, true, OBJECT_NONE, float3(1.0));
bool foundIntersection(Hit hit) {
    return hit.object.type != obj_None;
}

// ============== LIGHTS ==============
struct Light {
    int objectId;
    float area;
    float selectionProbability;
    float cumulativeProbability;
};

struct SurfaceSample {
    float3 p;
    float3 normal;
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
