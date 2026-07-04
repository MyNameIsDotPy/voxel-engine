#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aUV;
layout (location = 3) in vec4 aColor;

out vec3 FragPos;
out vec3 Normal;
out vec4 VertColor;
out float FragDist;

uniform mat4  model;
uniform mat4  view;
uniform mat4  projection;
uniform vec3  viewPos;
uniform float time;

void main() {
    vec3 pos = aPos;

    // Water wave animation — only on transparent (water) vertices
    if (aColor.a < 0.99) {
        float wave = sin(time * 1.8 + pos.x * 1.2 + pos.z * 0.9) * 0.055
                   + cos(time * 1.3 + pos.z * 1.5 - pos.x * 0.7) * 0.035;
        pos.y += wave;
    }

    FragPos    = vec3(model * vec4(pos, 1.0));
    Normal     = mat3(transpose(inverse(model))) * aNormal;
    VertColor  = aColor;
    FragDist   = length(FragPos - viewPos);

    gl_Position = projection * view * vec4(FragPos, 1.0);
}
