#version 460
#extension GL_ARB_shading_language_include : enable
#include "../../include/constants.h"

layout (std430, set = 0, binding = 0) writeonly buffer Trails { vec2 trails[][TRAIL_LENGTH]; };
layout (std430, set = 0, binding = 1) readonly buffer Positions { vec2 positions[]; };
layout (std140, set = 2, binding = 0) uniform Frame { uint frame; };

layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
void main() {
    uint i = gl_GlobalInvocationID.x;
    trails[i][frame] = positions[i];
}
