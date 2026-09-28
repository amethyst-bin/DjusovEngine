#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in ivec4 aBoneIds;
layout (location = 5) in vec4 aWeights;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;
out vec4 FragPosLightSpace;
out vec4 ClipPos;
out mat3 TBN;

const int MAX_BONES = 64;
uniform mat4 uBones[MAX_BONES];
uniform bool uHasSkinning;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;
uniform mat4 uLightSpaceMatrix;

void main() {
    vec4 totalPosition = vec4(0.0);
    vec3 totalNormal = vec3(0.0);
    vec3 totalTangent = vec3(0.0);

    if (uHasSkinning) {
        float weightSum = aWeights.x + aWeights.y + aWeights.z + aWeights.w;
        if (weightSum > 0.001) {
            for (int i = 0; i < 4; ++i) {
                int boneId = aBoneIds[i];
                float weight = aWeights[i];
                if (boneId >= 0 && boneId < MAX_BONES && weight > 0.0) {
                    mat4 boneMatrix = uBones[boneId];
                    totalPosition += (boneMatrix * vec4(aPos, 1.0)) * weight;
                    totalNormal += (mat3(boneMatrix) * aNormal) * weight;
                    totalTangent += (mat3(boneMatrix) * aTangent) * weight;
                }
            }
        } else {
            totalPosition = vec4(aPos, 1.0);
            totalNormal = aNormal;
            totalTangent = aTangent;
        }
    } else {
        totalPosition = vec4(aPos, 1.0);
        totalNormal = aNormal;
        totalTangent = aTangent;
    }

    vec4 worldPos = uModel * totalPosition;
    FragPos = worldPos.xyz;

    mat3 normalMatrix = transpose(inverse(mat3(uModel)));
    vec3 N = normalize(normalMatrix * totalNormal);
    vec3 T = normalize(normalMatrix * totalTangent);
    T = normalize(T - dot(T, N) * N);
    vec3 B = cross(N, T);
    TBN = mat3(T, B, N);

    Normal = N;
    TexCoords = aTexCoords;
    FragPosLightSpace = uLightSpaceMatrix * worldPos;

    ClipPos = uProjection * uView * worldPos;
    gl_Position = ClipPos;
}
