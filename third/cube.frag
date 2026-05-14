#version 330 core

in vec3 FragPos;
in vec3 Normal;

out vec4 FragColor;

// Light
uniform bool  lightOn;
uniform vec3  lightPos;
uniform vec3  lightColor;   // yellow: (1,1,0)

// Material
uniform bool  materialOn;
uniform vec3  objectColor;  // magenta: (1,0,1)

void main()
{
    vec3 baseColor = materialOn ? objectColor : vec3(1.0); // white default

    if (lightOn)
    {
        // Ambient
        float ambientStrength = 0.2;
        vec3 ambient = ambientStrength * lightColor;

        // Diffuse
        vec3 norm     = normalize(Normal);
        vec3 lightDir = normalize(lightPos - FragPos);
        float diff    = max(dot(norm, lightDir), 0.0);
        vec3 diffuse  = diff * lightColor;

        vec3 result = (ambient + diffuse) * baseColor;
        FragColor = vec4(result, 1.0);
    }
    else
    {
        FragColor = vec4(baseColor, 1.0);
    }
}
