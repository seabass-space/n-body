#version 460
#extension GL_ARB_shading_language_include : enable
#include "../../../include/constants.h"

layout (location = 0) out vec4 out_color;

layout (std430, set = 0, binding = 0) readonly buffer Positions { vec2 positions[][FIELD_LINE_LENGTH]; };
layout (std430, set = 0, binding = 1) readonly buffer LineIDs { uint id[]; };
layout (std430, set = 0, binding = 2) readonly buffer Colors { vec4 colors[]; };

layout (std140, set = 1, binding = 0) uniform Camera {
    mat4 orthographic;
    mat4 view;
};

layout (std140, set = 1, binding = 1) uniform Constants {
    vec4 _padding;
    float brightness;
};

void main() {
    vec2 position = positions[gl_InstanceIndex][gl_VertexIndex];
    gl_Position = orthographic * view * vec4(position, 0.0, 1.0);
    float alpha = (brightness / 2.0) * (1.0 - float(gl_VertexIndex) / float(FIELD_LINE_LENGTH));
    out_color = vec4(colors[id[gl_InstanceIndex]].rgb, alpha);
}

