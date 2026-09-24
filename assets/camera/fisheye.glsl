#camera_lens: version(1)

#param float fov = 180.0: min(1.0), max(220.0)

void main() {
    vec2 p = vec2(ndcPos.x * U, -ndcPos.y * V);
    float r = length(p) / max(U, V);
    float theta = r * radians(fov) * 0.5;
    float phi = atan(p.y, p.x);

    result.origin = pose.eye;
    result.direction = pose.dir * cos(theta) + (pose.right * cos(phi) + pose.up * sin(phi)) * sin(theta);
}
