#camera_lens: version(1)

#param float k1 = 0.0: min(-1.0), max(1.0)
#param float k2 = 0.0: min(-1.0), max(1.0)

void main() {
    float r2 = dot(ndcPos, ndcPos);
    vec2 distorted = ndcPos * (1.0 + k1 * r2 + k2 * r2 * r2);

    vec3 right = pose.right * U;
    vec3 up = pose.up * V;
    vec3 focalPoint = pose.eye + (distorted.x * right - distorted.y * up + pose.dir) * focalDistance;

    vec3 offset = vec3(0.0);
    if (lensRadius > 0.0) {
        vec2 p = sampleLens(rng);
        offset = lensRadius * (pose.right * p.x + pose.up * p.y);
    }

    result.origin = pose.eye + offset;
    result.direction = focalPoint - result.origin;
}
