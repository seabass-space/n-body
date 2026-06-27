#version 460

#define TAU 6.2831

const uint LINE_LENGTH = 1024;
const float LINE_VOLUME = 5.0;
const float LINE_OFFSET = 1.0;
const float LINE_STEP = 0.5;

layout (std430, set = 0, binding = 0) buffer FieldLinePositions { vec2 r[][LINE_LENGTH]; };
layout (std430, set = 0, binding = 1) readonly buffer FieldLineIDs { uint line_id[]; };
layout (std430, set = 0, binding = 2) readonly buffer SimulationPositons { vec2 r_0[]; };
layout (std430, set = 0, binding = 3) readonly buffer SimulationMasses { float m[]; };

layout (std140, set = 2, binding = 0) uniform Constants {
    uint body_count;
    float G;
    float ee;
};

layout (std140, set = 2, binding = 1) uniform Frame { uint frame; };

vec2 gravity(vec2 position) {
    vec2 net_a = vec2(0.0);
    for (uint i = 0; i < body_count; i++) {
        vec2 R = r_0[i] - position;
        float R2 = dot(R, R) + ee * ee;
        net_a += (G * m[i] / R2) * normalize(R);
    }

    return net_a;
}

layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
void main() {
    uint i = gl_GlobalInvocationID.x;
    if (frame == 0) {
        uint body = line_id[i];
        uint line_count = int(m[body] / LINE_VOLUME);
        float theta = TAU / line_count * i;
        r[i][0] = r_0[body] + LINE_OFFSET * vec2(cos(theta), sin(theta));
    } else {
        vec2 y = r[i][frame - 1];
        vec2 k_1 = normalize(gravity(y));
        vec2 k_2 = normalize(gravity(y - 0.5 * LINE_STEP * k_1));
        r[i][frame] = y - LINE_STEP * k_2;
    }
}
