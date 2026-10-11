// --------------- INPUTS ---------------
// float2 ndcPos
// CameraPose pose
// float U
// float V
// float lensRadius
// float focalDistance
// RngState rng

// --------------- OUTPUTS ---------------
// LensSample result

#camera_lens: version(1)

#param float3 planePosition = float3(0.0)
#param float3 planeRotation = float3(0.0)
#param float shiftX = 0.0
#param float shiftY = 0.0

void main() {
    float3 worldNormal = mul(float3(0.0, 0.0, 1.0), quatToMat3(eulerToQuat(radians(planeRotation))));

    float3 diff = planePosition - pose.eye;
    float3 center = float3(dot(diff, pose.right), dot(diff, pose.up), dot(diff, pose.dir));
    float3 normal = normalize(float3(dot(worldNormal, pose.right), dot(worldNormal, pose.up), dot(worldNormal, pose.dir)));

    float3 arbUp = abs(dot(normal, float3(0.0, 1.0, 0.0))) < 0.99 ? float3(0.0, 1.0, 0.0) : float3(1.0, 0.0, 0.0);
    float3 right2 = normalize(cross(normal, arbUp));
    float3 upOnPlane = cross(normal, right2);

    float3 focusA = center + right2;
    float3 focusB = center - right2;
    float3 focusC = center + upOnPlane;

    float3 d = float3(ndcPos.x * U + shiftX, -ndcPos.y * V + shiftY, 1.0);
    float3 ab = focusB - focusA;
    float3 ac = focusC - focusA;
    float3 planeNormal = normalize(cross(ab, ac));
    float dDotN = dot(d, planeNormal);

    float3 focalPointCam;
    if (abs(dDotN) > 1e-6) {
        float t = dot(focusA, planeNormal) / dDotN;
        focalPointCam = t * d;
    } else {
        focalPointCam = d * focalDistance;
    }
    float3 focalPoint = pose.eye + focalPointCam.x * pose.right + focalPointCam.y * pose.up + focalPointCam.z * pose.dir;

    float3 offset = float3(0.0);
    if (lensRadius > 0.0) {
        float2 p = sampleLens(rng);
        offset = lensRadius * (pose.right * p.x + pose.up * p.y);
    }

    result.origin = pose.eye + offset;
    result.direction = focalPoint - result.origin;
}
