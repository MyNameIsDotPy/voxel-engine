#version 330 core
out vec4 FragColor;

uniform mat4 invProj;
uniform mat4 invView;
uniform vec2 screenSize;

uniform vec3 sunDir;        // normalized, points toward sun
uniform vec3 sunColor;      // current sun light color
uniform vec3 zenithColor;   // top of sky
uniform vec3 horizonColor;  // horizon color

void main() {
    // ── Reconstruct world-space view direction ────────────────────────────────
    vec2 ndc     = (gl_FragCoord.xy / screenSize) * 2.0 - 1.0;
    vec4 clip    = vec4(ndc, -1.0, 1.0);
    vec4 viewPos = invProj * clip;
    vec3 viewDir = normalize(viewPos.xyz / viewPos.w);
    vec3 worldDir = normalize((invView * vec4(viewDir, 0.0)).xyz);

    // ── Sky gradient (horizon → zenith) ───────────────────────────────────────
    float t   = clamp(worldDir.y, 0.0, 1.0);
    t         = pow(t, 0.55);                         // subtle curve for wider horizon band
    vec3 sky  = mix(horizonColor, zenithColor, t);

    // ── Atmospheric glow near sun ─────────────────────────────────────────────
    float sunDot  = max(dot(worldDir, normalize(sunDir)), 0.0);
    float visible = clamp(sunDir.y + 0.15, 0.0, 1.0); // fade glow as sun sets
    sky += sunColor * pow(sunDot, 6.0) * 0.6 * visible;
    sky += sunColor * pow(sunDot, 64.0) * 0.8 * visible; // tighter corona

    // ── Sun disc ─────────────────────────────────────────────────────────────
    float disc = smoothstep(0.9994, 0.9998, sunDot);
    sky = mix(sky, sunColor * 2.2, disc * visible);

    // ── Below horizon: ground fog colour ─────────────────────────────────────
    if (worldDir.y < 0.0) {
        float below = clamp(-worldDir.y * 4.0, 0.0, 1.0);
        sky = mix(sky, horizonColor * 0.6, below);
    }

    FragColor = vec4(sky, 1.0);
}
