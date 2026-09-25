//#shader vertex
#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    FragPos = vec3(model * vec4(aPos, 1.0));
    // It's better to calculate the normal matrix on CPU and pass it as a uniform, 
    // but doing it here for simplicity
    Normal = mat3(transpose(inverse(model))) * aNormal;  
    TexCoords = aTexCoords;
    
    gl_Position = projection * view * vec4(FragPos, 1.0);
}

//#shader fragment
#version 330 core

// ... existing fragment shader setup ...
layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BrightColor;

struct Material {
    float shininess;
    vec3 diffuse_color;
    vec3 specular_color;
}; 

struct DirLight {
    vec3 direction;
	
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct PointLight {
    vec3 position;
    
    float constant;
    float linear;
    float quadratic;
	
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

uniform vec3 viewPos;
uniform DirLight dirLight;
uniform PointLight pointLight;

struct SpotLight {
    vec3 position;
    vec3 direction;
    float cutOff;
    float outerCutOff;
  
    float constant;
    float linear;
    float quadratic;
  
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;       
};
uniform SpotLight spotLight;
uniform Material material;

uniform sampler2D texture_diffuse1;
uniform sampler2D texture_specular1;

uniform samplerCube depthMap;
uniform float far_plane;
uniform bool shadows;
uniform bool shadowsFromWelder;

// Function prototypes
vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir);
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, bool calculateShadow);
vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir, bool calculateShadow);
float ShadowCalculation(vec3 fragPos, vec3 lightPos);

void main()
{    
    // Properties
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);
    
    // Phase 1: directional lighting
    vec3 result = CalcDirLight(dirLight, norm, viewDir);
    
    // Phase 2: lights
    result += CalcPointLight(pointLight, norm, FragPos, viewDir, shadowsFromWelder);    
    result += CalcSpotLight(spotLight, norm, FragPos, viewDir, !shadowsFromWelder);    
    
    FragColor = vec4(result, 1.0);
    
    // check whether result is higher than some threshold, if so, output as bloom threshold color
    float brightness = dot(result, vec3(0.2126, 0.7152, 0.0722));
    if(brightness > 1.0)
        BrightColor = vec4(result, 1.0);
    else
        BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
}

// Calculates the color when using a directional light.
vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir)
{
    vec3 lightDir = normalize(-light.direction);
    
    // Diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    
    // Specular shading (Blinn-Phong)
    vec3 halfwayDir = normalize(lightDir + viewDir);  
    float spec = pow(max(dot(normal, halfwayDir), 0.0), material.shininess);
    
    // Combine results
    vec3 ambient = light.ambient * vec3(texture(texture_diffuse1, TexCoords)) * material.diffuse_color;
    vec3 diffuse = light.diffuse * diff * vec3(texture(texture_diffuse1, TexCoords)) * material.diffuse_color;
    vec3 specular = light.specular * spec * vec3(texture(texture_specular1, TexCoords)) * material.specular_color;
    
    return (ambient + diffuse + specular);
}

// Calculates the color when using a point light.
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, bool calculateShadow)
{
    vec3 lightDir = normalize(light.position - fragPos);
    
    // Diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    
    // Specular shading (Blinn-Phong)
    vec3 halfwayDir = normalize(lightDir + viewDir);  
    float spec = pow(max(dot(normal, halfwayDir), 0.0), material.shininess);
    
    // Attenuation
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));    
    
    // Combine results
    vec3 ambient = light.ambient * vec3(texture(texture_diffuse1, TexCoords)) * material.diffuse_color;
    vec3 diffuse = light.diffuse * diff * vec3(texture(texture_diffuse1, TexCoords)) * material.diffuse_color;
    vec3 specular = light.specular * spec * vec3(texture(texture_specular1, TexCoords)) * material.specular_color;
    
    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;
    
    // Calculate shadow
    float shadow = (calculateShadow && shadows) ? ShadowCalculation(fragPos, light.position) : 0.0;                      
    
    return (ambient + (1.0 - shadow) * (diffuse + specular));
}

// Calculate shadows with PCF
float ShadowCalculation(vec3 fragPos, vec3 lightPos)
{
    vec3 fragToLight = fragPos - lightPos;
    float currentDepth = length(fragToLight);
    
    float shadow = 0.0;
    float bias = 0.15;
    int samples = 20;
    float viewDistance = length(viewPos - fragPos);
    float diskRadius = (1.0 + (viewDistance / far_plane)) / 25.0;
    
    // array of offset directions for sampling
    vec3 sampleOffsetDirections[20] = vec3[]
    (
       vec3( 1,  1,  1), vec3( 1, -1,  1), vec3(-1, -1,  1), vec3(-1,  1,  1), 
       vec3( 1,  1, -1), vec3( 1, -1, -1), vec3(-1, -1, -1), vec3(-1,  1, -1),
       vec3( 1,  1,  0), vec3( 1, -1,  0), vec3(-1, -1,  0), vec3(-1,  1,  0),
       vec3( 1,  0,  1), vec3(-1,  0,  1), vec3( 1,  0, -1), vec3(-1,  0, -1),
       vec3( 0,  1,  1), vec3( 0, -1,  1), vec3( 0, -1, -1), vec3( 0,  1, -1)
    );
    
    for(int i = 0; i < samples; ++i)
    {
        float closestDepth = texture(depthMap, fragToLight + sampleOffsetDirections[i] * diskRadius).r;
        closestDepth *= far_plane;   // undo mapping [0;1]
        if(currentDepth - bias > closestDepth)
            shadow += 1.0;
    }
    shadow /= float(samples);
        
    return shadow;
}

// Calculates the color when using a spotlight.
vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir, bool calculateShadow)
{
    vec3 lightDir = normalize(light.position - fragPos);
    
    // Diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    
    // Specular shading (Blinn-Phong)
    vec3 halfwayDir = normalize(lightDir + viewDir);  
    float spec = pow(max(dot(normal, halfwayDir), 0.0), material.shininess);
    
    // Attenuation
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));    
    
    // Spotlight intensity -> Replaced with Hemispherical downward blocking
    vec3 dirToFrag = fragPos - light.position;
    
    // Smoothly fade out light that goes above the lamp bulb horizon (Y = 0 relative to bulb)
    // This allows it to shine evenly everywhere downwards (360 degrees) but blocks Y > 0
    float intensity = clamp(1.0 - (dirToFrag.y + 1.0) / 2.0, 0.0, 1.0);
    
    // Combine results
    vec3 ambient = light.ambient * vec3(texture(texture_diffuse1, TexCoords)) * material.diffuse_color;
    vec3 diffuse = light.diffuse * diff * vec3(texture(texture_diffuse1, TexCoords)) * material.diffuse_color;
    vec3 specular = light.specular * spec * vec3(texture(texture_specular1, TexCoords)) * material.specular_color;
    
    ambient *= attenuation;
    diffuse *= attenuation * intensity;
    specular *= attenuation * intensity;
    
    // Calculate shadow
    float shadow = (calculateShadow && shadows) ? ShadowCalculation(fragPos, light.position) : 0.0;                      
    
    return (ambient + (1.0 - shadow) * (diffuse + specular));
}
