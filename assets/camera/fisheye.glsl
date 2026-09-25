#camera_lens: version(1)

#param float fov = 180.0: min(1.0), max(220.0)

void main() {
    vec2 p = vec2(ndcPos.x * U, -ndcPos.y * V);
    float r = length(p) / max(U, V);
    float theta = r * radians(fov) * 0.5;
    float phi = atan(p.y, p.x);

    vec3 dir = pose.dir * cos(theta) + (pose.right * cos(phi) + pose.up * sin(phi)) * sin(theta);
    vec3 focalPoint = pose.eye + dir * ubo.camera.thinLens.focusDistance;

    vec3 offset = vec3(0.0);
    if (ubo.camera.thinLens.lensRadius > 0.0) {
        vec2 lensP = sampleLens(rng);
        offset = ubo.camera.thinLens.lensRadius * (pose.right * lensP.x + pose.up * lensP.y);
    }

    result.origin = pose.eye + offset;
    result.direction = focalPoint - result.origin;
}
