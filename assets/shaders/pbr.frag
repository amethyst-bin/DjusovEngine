#version 330 core

layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BrightColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;
in vec4 FragPosLightSpace;
in vec4 ClipPos;
in mat3 TBN;

// Material properties
uniform vec3 uAlbedo;
uniform float uMetallic;
uniform float uRoughness;
uniform float uAO;
uniform vec3 uEmissive;
uniform float uEmissiveIntensity;
uniform float uTransmission;
uniform float uIOR;
uniform float uAlpha;
uniform bool uIsMirror;
uniform bool uIsGlass;

// Textures
uniform sampler2D uAlbedoMap;
uniform bool uUseAlbedoMap;
uniform sampler2D uNormalMap;
uniform bool uUseNormalMap;
uniform sampler2D uRoughnessMetallicMap;
uniform bool uUseRoughnessMetallicMap;

// Reflection texture for mirrors
uniform sampler2D uReflectionMap;

// Shadow map
uniform sampler2DShadow uShadowMap;
uniform vec3 uSunDir;
uniform vec3 uSunColor;
uniform float uSunIntensity;

// Point lights
struct PointLight {
    vec3 position;
    vec3 color;
    float intensity;
    float radius;
};
#define MAX_POINT_LIGHTS 16
uniform int uNumPointLights;
uniform PointLight uPointLights[MAX_POINT_LIGHTS];

uniform vec3 uCamPos;

const float PI = 3.14159265359;

// GGX Normal Distribution
float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    return a2 / max(denom, 0.0000001);
}

// Schlick-GGX Geometry
float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
    float denom = NdotV * (1.0 - k) + k;
    return NdotV / max(denom, 0.0000001);
}

// Smith Geometry
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);
    return ggx1 * ggx2;
}

// Fresnel Schlick
vec3 FresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// PCF Shadow calculation
float CalculateShadow(vec4 fragPosLightSpace, vec3 N, vec3 L) {
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    if (projCoords.z > 1.0) return 0.0;

    float bias = max(0.005 * (1.0 - dot(N, L)), 0.001);
    float currentDepth = projCoords.z - bias;

    float shadow = 0.0;
    vec2 texelSize = 1.0 / vec2(2048.0, 2048.0);
    for (int x = -2; x <= 2; ++x) {
        for (int y = -2; y <= 2; ++y) {
            vec3 uvDepth = vec3(projCoords.xy + vec2(x, y) * texelSize, currentDepth);
            shadow += texture(uShadowMap, uvDepth);
        }
    }
    shadow /= 25.0;
    return 1.0 - shadow;
}

void main() {
    vec4 albedoSample = uUseAlbedoMap ? texture(uAlbedoMap, TexCoords) : vec4(uAlbedo, uAlpha);
    vec3 baseColor = albedoSample.rgb;
    float alpha = albedoSample.a;

    // Normal mapping
    vec3 N = normalize(Normal);
    if (uUseNormalMap) {
        vec3 normalMap = texture(uNormalMap, TexCoords).rgb * 2.0 - 1.0;
        N = normalize(TBN * normalMap);
    }

    vec3 V = normalize(uCamPos - FragPos);
    float NdotV = max(dot(N, V), 0.0);

    // ================= 1. Real-time Planar Mirror Reflection =================
    if (uIsMirror) {
        vec2 screenUV = (ClipPos.xy / ClipPos.w) * 0.5 + 0.5;
        // Invert Y if needed based on viewport
        vec3 reflectionColor = texture(uReflectionMap, screenUV).rgb;
        vec3 mirrorColor = mix(reflectionColor, baseColor, 0.05);

        FragColor = vec4(mirrorColor, 1.0);
        BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    // ================= 2. Realistic Glass Shading =================
    if (uIsGlass) {
        float ior = (uIOR > 1.0) ? uIOR : 1.52;
        float F0_glass = pow((1.0 - ior) / (1.0 + ior), 2.0);
        float fresnel = F0_glass + (1.0 - F0_glass) * pow(1.0 - NdotV, 5.0);

        vec3 L = normalize(uSunDir);
        vec3 H = normalize(V + L);
        float spec = pow(max(dot(N, H), 0.0), 128.0) * uSunIntensity;

        // Realistic glass: tint on grazing angles, refraction transparency
        vec3 glassTint = vec3(0.85, 0.95, 0.98);
        vec3 glassColor = mix(glassTint * 0.2, vec3(1.0), fresnel) + spec * uSunColor;
        float glassAlpha = clamp(fresnel * 0.8 + 0.15, 0.0, 0.95);

        FragColor = vec4(glassColor, glassAlpha);
        BrightColor = (spec > 1.0) ? vec4(spec * uSunColor, 1.0) : vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    // ================= 3. Cook-Torrance PBR Shading =================
    float metallic = uMetallic;
    float roughness = uRoughness;
    if (uUseRoughnessMetallicMap) {
        vec4 rm = texture(uRoughnessMetallicMap, TexCoords);
        roughness = rm.g;
        metallic = rm.b;
    }
    roughness = clamp(roughness, 0.04, 1.0);

    vec3 F0 = vec3(0.04);
    F0 = mix(F0, baseColor, metallic);

    vec3 Lo = vec3(0.0);

    // --- Direct Sun Light ---
    {
        vec3 L = normalize(uSunDir);
        vec3 H = normalize(V + L);
        vec3 radiance = uSunColor * uSunIntensity;

        float NDF = DistributionGGX(N, H, roughness);
        float G   = GeometrySmith(N, V, L, roughness);
        vec3 F    = FresnelSchlick(max(dot(H, V), 0.0), F0);

        vec3 numerator    = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
        vec3 specular     = numerator / denominator;

        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - metallic;

        float NdotL = max(dot(N, L), 0.0);
        float shadow = CalculateShadow(FragPosLightSpace, N, L);

        Lo += (kD * baseColor / PI + specular) * radiance * NdotL * (1.0 - shadow);
    }

    // --- Point Lights ---
    for (int i = 0; i < uNumPointLights; ++i) {
        vec3 lightDir = uPointLights[i].position - FragPos;
        float distance = length(lightDir);
        if (distance > uPointLights[i].radius) continue;

        vec3 L = normalize(lightDir);
        vec3 H = normalize(V + L);

        // Attenuation with smooth cutoff
        float atten = clamp(1.0 - (distance / uPointLights[i].radius), 0.0, 1.0);
        atten = atten * atten / (distance * distance + 1.0);
        vec3 radiance = uPointLights[i].color * uPointLights[i].intensity * atten;

        float NDF = DistributionGGX(N, H, roughness);
        float G   = GeometrySmith(N, V, L, roughness);
        vec3 F    = FresnelSchlick(max(dot(H, V), 0.0), F0);

        vec3 numerator    = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
        vec3 specular     = numerator / denominator;

        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - metallic;

        float NdotL = max(dot(N, L), 0.0);
        Lo += (kD * baseColor / PI + specular) * radiance * NdotL;
    }

    // Ambient light with directional sky gradient approximation
    vec3 skyColor = vec3(0.5, 0.7, 0.95) * 0.35;
    vec3 groundColor = vec3(0.2, 0.22, 0.25) * 0.25;
    float up = dot(N, vec3(0.0, 1.0, 0.0)) * 0.5 + 0.5;
    vec3 ambient = mix(groundColor, skyColor, up) * baseColor * uAO;

    // Emissive contribution (windows, neon signs, lamps)
    vec3 emissive = uEmissive * uEmissiveIntensity;

    vec3 finalColor = ambient + Lo + emissive;

    FragColor = vec4(finalColor, alpha);

    // Bright pass for Bloom HDR buffer
    float brightness = dot(finalColor, vec3(0.2126, 0.7152, 0.0722));
    if (brightness > 1.0 || length(emissive) > 0.5) {
        BrightColor = vec4(finalColor, 1.0);
    } else {
        BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
    }
}
