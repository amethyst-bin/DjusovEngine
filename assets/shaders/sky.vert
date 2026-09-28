#version 330 core

layout (location = 0) in vec3 aPos;

out vec3 TexCoords;

uniform mat4 uView;
uniform mat4 uProjection;

void main() {
    TexCoords = aPos;
    // Remove translation from view matrix so skybox stays centered on camera
    mat4 staticView = mat4(mat3(uView));
    vec4 pos = uProjection * staticView * vec4(aPos, 1.0);
    gl_Position = pos.xyww; // Force z = w to render at depth 1.0
}
