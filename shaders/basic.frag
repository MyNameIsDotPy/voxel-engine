#version 330 core

in vec3  FragPos;
in vec3  Normal;
in vec4  VertColor;
in float FragDist;

out vec4 FragColor;

uniform vec3  viewPos;
uniform vec3  sunDir;
uniform vec3  sunColor;
uniform vec3  ambientColor;
uniform vec3  fogColor;
uniform float fogDensity;

void main() {
    // ── Face-based ambient shading (Minecraft-style) ──────────────────────────
    // Top faces receive full light, sides slightly less, bottom face is darkest.
    float faceLight;
    if      (Normal.y >  0.5)       faceLight = 1.00;   // top
    else if (Normal.y < -0.5)       faceLight = 0.50;   // bottom
    else if (abs(Normal.z) > 0.5)   faceLight = 0.80;   // north / south
    else                             faceLight = 0.65;   // east  / west

    vec3 base = VertColor.rgb * faceLight;

    // ── Ambient (sky light) ───────────────────────────────────────────────────
    vec3 ambient = ambientColor * base * 1.8;

    // ── Directional diffuse (sun) ─────────────────────────────────────────────
    vec3  N     = normalize(Normal);
    vec3  L     = normalize(sunDir);
    float diff  = max(dot(N, L), 0.0);
    vec3  diffuse = sunColor * diff * base;

    // ── Blinn-Phong specular (subtle) ─────────────────────────────────────────
    vec3  V       = normalize(viewPos - FragPos);
    vec3  H       = normalize(L + V);
    float spec    = pow(max(dot(N, H), 0.0), 48.0) * faceLight;
    vec3  specular = sunColor * spec * 0.12;

    vec3 lit = ambient + diffuse + specular;

    // ── Exponential² fog (blends into horizon / sky colour) ──────────────────
    float fogFactor = exp(-(fogDensity * FragDist) * (fogDensity * FragDist));
    fogFactor = clamp(fogFactor, 0.0, 1.0);
    vec3 result = mix(fogColor, lit, fogFactor);

    FragColor = vec4(result, VertColor.a);
}
