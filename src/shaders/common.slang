float luma(float3 c) {
    return dot(c, float3(0.2126, 0.7152, 0.0722));
}

struct Material {
    int  type;
    uint base;
};

static const int mat_Principled = 0;
static const int mat_Emissive = 1;
static const int mat_Diffuse = 2;
static const int mat_Metal = 3;
static const int mat_Glossy = 4;
static const int mat_Dielectric = 5;
static const int mat_Volume = 6;

struct AOVBuffer {
    uint  hitValid;
    float3  positionW;
    float3  position;
    float3  normalW;
    float2  normal;
    float3  albedo;
    float roughness;
    uint  matType;
    uint  skyMask;
};

struct PixelInfo {
    AOVBuffer aov;
    float mean;
    float m2;
    int   count;
    uint  bounces;
    uint  bvhChecks;
    uint  triangleChecks;
    float varianceProba;
};

static const int debug_None = 0;
static const int debug_PositionW = 1;
static const int debug_Position = 2;
static const int debug_NormalW = 3;
static const int debug_Normal = 4;
static const int debug_Albedo = 5;
static const int debug_Roughness = 6;
static const int debug_MatType = 7;
static const int debug_Bounces = 8;
static const int debug_HitChecks = 9;
static const int debug_Variance = 10;
static const int debug_SkyMask = 11;
