#version 330 core

out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D uImage;
uniform bool uHorizontal;
uniform float uWeight[5] = float[] (0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);

void main() {
    vec2 texOffset = 1.0 / textureSize(uImage, 0);
    vec3 result = texture(uImage, TexCoords).rgb * uWeight[0];

    if (uHorizontal) {
        for (int i = 1; i < 5; ++i) {
            result += texture(uImage, TexCoords + vec2(texOffset.x * i, 0.0)).rgb * uWeight[i];
            result += texture(uImage, TexCoords - vec2(texOffset.x * i, 0.0)).rgb * uWeight[i];
        }
    } else {
        for (int i = 1; i < 5; ++i) {
            result += texture(uImage, TexCoords + vec2(0.0, texOffset.y * i)).rgb * uWeight[i];
            result += texture(uImage, TexCoords - vec2(0.0, texOffset.y * i)).rgb * uWeight[i];
        }
    }

    FragColor = vec4(result, 1.0);
}
