#version 330 core

out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D uSceneTexture;
uniform sampler2D uBloomTexture;
uniform bool uBloomEnabled;
uniform float uBloomIntensity;
uniform float uADSAmount; // 0.0 = not aiming, 1.0 = fully aiming down sights
uniform float uExposure;

// ACES Tone Mapping (Academy Color Encoding System - standard in Unreal Engine 5)
vec3 ACESFilm(vec3 x) {
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

void main() {
    vec2 center = vec2(0.5, 0.5);
    vec2 uv = TexCoords;
    float distFromCenter = length(uv - center);

    vec3 sceneColor = texture(uSceneTexture, uv).rgb;

    // ================= Dynamic ADS Scope Peripheral Blur =================
    if (uADSAmount > 0.01) {
        // Blur mask: center circle remains razor sharp, outer ring smoothly blurs
        float innerRadius = 0.18; // Crisp scope center
        float outerRadius = 0.55;
        float blurFactor = smoothstep(innerRadius, outerRadius, distFromCenter) * uADSAmount;

        if (blurFactor > 0.01) {
            vec3 blurred = vec3(0.0);
            float totalWeight = 0.0;
            // 8-tap radial sample
            vec2 dir = normalize(uv - center + 0.0001);
            float blurScale = blurFactor * 0.015;

            for (float i = -4.0; i <= 4.0; i += 1.0) {
                float weight = exp(-0.5 * (i * i) / 4.0);
                vec2 sampleUV = clamp(uv + dir * i * blurScale, 0.0, 1.0);
                blurred += texture(uSceneTexture, sampleUV).rgb * weight;
                totalWeight += weight;
            }
            blurred /= totalWeight;
            sceneColor = mix(sceneColor, blurred, blurFactor);

            // Subtle dark vignette at extreme edges when scoping
            float vignette = smoothstep(0.75, 0.35, distFromCenter);
            sceneColor *= mix(1.0, vignette, uADSAmount * 0.4);
        }
    }

    // ================= Bloom blending =================
    if (uBloomEnabled) {
        vec3 bloom = texture(uBloomTexture, uv).rgb;
        sceneColor += bloom * uBloomIntensity;
    }

    // Exposure
    sceneColor *= uExposure;

    // ACES Film Tonemapping
    vec3 mapped = ACESFilm(sceneColor);

    // Gamma correction
    const float gamma = 2.2;
    mapped = pow(mapped, vec3(1.0 / gamma));

    FragColor = vec4(mapped, 1.0);
}
