#camera_lens: version(1)

#param float astigmatism = 1.0: min(0.0), max(4.0)

void main() {
    vec3 right = pose.right * U;
    vec3 up = pose.up * V;
    vec3 focalPoint = pose.eye + (ndcPos.x * right - ndcPos.y * up + pose.dir) * focalDistance;

    vec3 offset = vec3(0.0);
    if (lensRadius > 0.0) {
        vec2 fieldPos = vec2(ndcPos.x * U, -ndcPos.y * V);
        float fieldHeight = length(fieldPos);
        vec2 radialDir = fieldHeight > 1e-6 ? fieldPos / fieldHeight : vec2(1.0, 0.0);
        vec2 tangentialDir = vec2(-radialDir.y, radialDir.x);

        vec2 p = sampleLens(rng);
        vec2 pLocal = vec2(dot(p, radialDir), dot(p, tangentialDir));
        pLocal.y *= 1.0 + astigmatism * fieldHeight * fieldHeight;
        vec2 pWorld = pLocal.x * radialDir + pLocal.y * tangentialDir;

        offset = lensRadius * (pose.right * pWorld.x + pose.up * pWorld.y);
    }

    result.origin = pose.eye + offset;
    result.direction = focalPoint - result.origin;
}
