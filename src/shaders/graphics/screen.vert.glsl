#version 460

layout (location = 0) out vec2 frag;

layout (std140, set = 1, binding = 0) uniform Camera {
    mat4 orthographic;
    mat4 view;
};

void main() {
    vec2 position = vec2[](
        vec2(-1.0, -1.0),
        vec2(-1.0,  1.0),
        vec2( 1.0, -1.0),
        vec2( 1.0,  1.0)
    )[gl_VertexIndex];
    gl_Position = vec4(position, 0.0, 1.0);
    frag = (inverse(view) * inverse(orthographic) * gl_Position).xy;
}
