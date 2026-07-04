#version 330 core

in vec3 FragPos;
in vec3 Normal;

out vec4 FragColor;

uniform vec3 lightPos;
uniform vec3 viewPos;
uniform vec3 objectColor;

void main() {
    // Ambient
    vec3 ambient = 0.18 * objectColor;

    // Diffuse
    vec3 norm     = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff    = max(dot(norm, lightDir), 0.0);
    vec3 diffuse  = diff * objectColor;

    // Specular (Blinn-Phong)
    vec3  viewDir    = normalize(viewPos - FragPos);
    vec3  halfDir    = normalize(lightDir + viewDir);
    float spec       = pow(max(dot(norm, halfDir), 0.0), 64.0);
    vec3  specular   = spec * vec3(0.6);

    // Edge darkening for a subtle voxel feel
    float rim = 1.0 - max(dot(viewDir, norm), 0.0);
    rim       = pow(rim, 3.0) * 0.25;

    vec3 result = ambient + diffuse + specular + rim * objectColor;
    FragColor   = vec4(result, 1.0);
}
