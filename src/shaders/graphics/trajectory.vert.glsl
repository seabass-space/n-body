#version 460

layout (location = 0) out vec4 out_color;

const uint PREDICTION_LENGTH = 2048;
layout (std430, set = 0, binding = 0) readonly buffer Positions { vec2 positions[][PREDICTION_LENGTH]; };
layout (std430, set = 0, binding = 1) readonly buffer Colors { vec4 colors[]; };

layout (std140, set = 1, binding = 0) uniform Camera {
    mat4 orthographic;
    mat4 view;
};

layout (std140, set = 1, binding = 1) uniform Constants {
    vec3 _padding;
    uint target;
    float brightness;
    uint body_count;
};

layout (std140, set = 1, binding = 2) uniform Ghost { vec4 ghost; };

void main() {
    vec2 position = positions[gl_InstanceIndex][gl_VertexIndex];
    if (target != uint(-1)) {
        position += positions[target][0] - positions[target][gl_VertexIndex];
    }

    gl_Position = orthographic * view * vec4(position, 0.0, 1.0);

    vec4 color = (gl_InstanceIndex == body_count) ? ghost : colors[gl_InstanceIndex];
    float alpha = (brightness / 2.0) * (1.0 - float(gl_VertexIndex) / float(PREDICTION_LENGTH));
     out_color = vec4(color.rgb, alpha);
}
