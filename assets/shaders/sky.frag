#version 330 core

out vec4 FragColor;

in vec3 TexCoords;

uniform vec3 uSunDir;
uniform vec3 uSunColor;

void main() {
    vec3 dir = normalize(TexCoords);

    // Smooth atmospheric sky gradient
    vec3 zenithColor = vec3(0.18, 0.42, 0.82);
    vec3 horizonColor = vec3(0.70, 0.82, 0.95);
    vec3 groundColor = vec3(0.20, 0.22, 0.25);

    float h = dir.y;
    vec3 sky;
    if (h > 0.0) {
        sky = mix(horizonColor, zenithColor, pow(h, 0.6));
    } else {
        sky = mix(horizonColor, groundColor, pow(clamp(-h * 4.0, 0.0, 1.0), 0.5));
    }

    // Sun disc
    float sunDot = max(dot(dir, normalize(uSunDir)), 0.0);
    float sunDisc = smoothstep(0.998, 0.9995, sunDot);
    float sunGlow = pow(sunDot, 16.0) * 0.4;

    vec3 finalColor = sky + (uSunColor * sunDisc * 5.0) + (uSunColor * sunGlow);
    FragColor = vec4(finalColor, 1.0);
}
