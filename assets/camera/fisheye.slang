#camera_lens: version(1)

#param float fov = 180.0: min(1.0), max(220.0)

void main() {
    float2 p = float2(ndcPos.x * U, -ndcPos.y * V);
    float r = length(p) / max(U, V);
    float theta = r * radians(fov) * 0.5;
    float phi = atan2(p.y, p.x);

    float3 dir = pose.dir * cos(theta) + (pose.right * cos(phi) + pose.up * sin(phi)) * sin(theta);
    float3 focalPoint = pose.eye + dir * focalDistance;

    float3 offset = float3(0.0);
    if (lensRadius > 0.0) {
        float2 lensP = sampleLens(rng);
        offset = lensRadius * (pose.right * lensP.x + pose.up * lensP.y);
    }

    result.origin = pose.eye + offset;
    result.direction = focalPoint - result.origin;
}
