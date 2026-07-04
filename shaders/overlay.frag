#version 330 core

out vec4 FragColor;

uniform vec4  overlayColor;
uniform float waterLineNDC;   // NDC Y [-1,1] of water surface; >1 = full screen; <-1 = none
uniform float screenHeight;

void main() {
    // Convert fragment window-Y to NDC Y  (gl_FragCoord.y = 0 at bottom)
    float ndcY = (gl_FragCoord.y / screenHeight) * 2.0 - 1.0;

    // Only tint the portion of the screen that is below the water surface
    if (ndcY > waterLineNDC) discard;

    FragColor = overlayColor;
}
