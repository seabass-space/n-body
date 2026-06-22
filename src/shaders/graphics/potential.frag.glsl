#version 460

layout (location = 0) in vec2 position;
layout (location = 0) out vec4 color;

layout (std430, set = 2, binding = 0) readonly buffer Positions { vec2 r[]; };
layout (std430, set = 2, binding = 1) readonly buffer Masses { float m[]; };

layout (std140, set = 3, binding = 0) uniform Constants { uint body_count; };

void main() {
    float potential = 0.0;
    for (uint i = 0; i < body_count; i++) {
        float R = length(r[i] - position);
        potential += 0.2 * m[i] / R;
    }

    color = vec4(potential, 0.0, 0.0, 0.5);
}
