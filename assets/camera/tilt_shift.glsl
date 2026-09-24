// --------------- INPUTS ---------------
// vec2 ndcPos
// CameraPose pose
// float U
// float V
// RngState rng

// --------------- OUTPUTS ---------------
// LensSample result

#camera_lens: version(1)

#param vec3 planePosition = vec3(0.0)
#param vec3 planeRotation = vec3(0.0)
#param float shiftX = 0.0
#param float shiftY = 0.0

void main() {
    float lensRadius = ubo.camera.thinLens.lensRadius;
    float focalDistance = ubo.camera.thinLens.focusDistance;

    vec3 worldNormal = quatToMat3(eulerToQuat(radians(planeRotation))) * vec3(0.0, 0.0, 1.0);

    vec3 diff = planePosition - pose.eye;
    vec3 center = vec3(dot(diff, pose.right), dot(diff, pose.up), dot(diff, pose.dir));
    vec3 normal = normalize(vec3(dot(worldNormal, pose.right), dot(worldNormal, pose.up), dot(worldNormal, pose.dir)));

    vec3 arbUp = abs(dot(normal, vec3(0.0, 1.0, 0.0))) < 0.99 ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0);
    vec3 right2 = normalize(cross(normal, arbUp));
    vec3 upOnPlane = cross(normal, right2);

    vec3 focusA = center + right2;
    vec3 focusB = center - right2;
    vec3 focusC = center + upOnPlane;

    vec3 d = vec3(ndcPos.x * U + shiftX, -ndcPos.y * V + shiftY, 1.0);
    vec3 ab = focusB - focusA;
    vec3 ac = focusC - focusA;
    vec3 planeNormal = normalize(cross(ab, ac));
    float dDotN = dot(d, planeNormal);

    vec3 focalPointCam;
    if (abs(dDotN) > 1e-6) {
        float t = dot(focusA, planeNormal) / dDotN;
        focalPointCam = t * d;
    } else {
        focalPointCam = d * focalDistance;
    }
    vec3 focalPoint = pose.eye + focalPointCam.x * pose.right + focalPointCam.y * pose.up + focalPointCam.z * pose.dir;

    vec3 offset = vec3(0.0);
    if (lensRadius > 0.0) {
        vec2 p = sampleLens(rng);
        offset = lensRadius * (pose.right * p.x + pose.up * p.y);
    }

    result.origin = pose.eye + offset;
    result.direction = focalPoint - result.origin;
}
