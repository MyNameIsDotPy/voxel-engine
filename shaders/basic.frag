#version 330 core

in vec3 FragPos;
in vec3 Normal;
in vec4 VertColor;

out vec4 FragColor;

uniform vec3 lightPos;
uniform vec3 viewPos;

void main() {
    // Ambient
    vec3 ambient = 0.18 * VertColor.rgb;

    // Diffuse
    vec3 norm     = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff    = max(dot(norm, lightDir), 0.0);
    vec3 diffuse  = diff * VertColor.rgb;

    // Specular (Blinn-Phong)
    vec3  viewDir  = normalize(viewPos - FragPos);
    vec3  halfDir  = normalize(lightDir + viewDir);
    float spec     = pow(max(dot(norm, halfDir), 0.0), 64.0);
    vec3  specular = spec * vec3(0.4);

    // Subtle rim
    float rim = pow(1.0 - max(dot(viewDir, norm), 0.0), 3.0) * 0.2;

    vec3 result = ambient + diffuse + specular + rim * VertColor.rgb;
    FragColor   = vec4(result, VertColor.a);
}
