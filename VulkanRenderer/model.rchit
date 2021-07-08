#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_shader_explicit_arithmetic_types : enable

layout(set = 0, binding = 0, std140) uniform ub
{
    mat4 projection;
    mat4 modelView;
};

struct Vertex
{
    int16_t positionX_snorm;
    int16_t positionY_snorm;
    int16_t positionZ_snorm;

    int16_t normalX_snorm;
    int16_t normalY_snorm;
    int16_t normalZ_snorm;

    uint8_t colorR_snorm;
    uint8_t colorG_snorm;
    uint8_t colorB_snorm;
};

vec3 getNormal(Vertex vertex)
{
    return vec3(vertex.normalX_snorm/32767.0, vertex.normalY_snorm/32767.0, vertex.normalZ_snorm/32767.0);
}

vec3 getColor(Vertex vertex)
{
    return vec3(vertex.colorR_snorm/255.0, vertex.colorG_snorm/255.0, vertex.colorB_snorm/255.0);
}

layout(set = 0, binding = 3, std430) readonly buffer vb
{
    Vertex vertexBuffer[];
};

layout(set = 0, binding = 4, std430) readonly buffer iv
{
    int indexBuffer[];
};

layout(location = 0) rayPayloadInEXT vec3 outColor;

hitAttributeEXT vec2 baryCoord;

const vec3 lightPosition = vec3(0.0, 0.0, 0.0);
const float shininess = 16.0;
const float specularCoeff = 0.1;
const float diffuseCoeff = 0.9;
const float ambientCoeff = 0.1;

vec3 interpolate(vec3 a, vec3 b, vec3 c)
{
    return (1.0 - baryCoord.x - baryCoord.y) * a + baryCoord.x * b + baryCoord.y * c;
}

void main()
{
    int index0 = indexBuffer[3*gl_PrimitiveID + 0];
    int index1 = indexBuffer[3*gl_PrimitiveID + 1];
    int index2 = indexBuffer[3*gl_PrimitiveID + 2];

    Vertex vertex0 = vertexBuffer[index0];
    Vertex vertex1 = vertexBuffer[index1];
    Vertex vertex2 = vertexBuffer[index2];

    vec3 position = vec3(modelView * vec4(gl_WorldRayOriginEXT + gl_RayTmaxEXT * gl_WorldRayDirectionEXT, 1.0));
    vec3 normal = interpolate(getNormal(vertex0), getNormal(vertex1), getNormal(vertex2));
    vec3 color = interpolate(getColor(vertex0), getColor(vertex1), getColor(vertex2));

    vec3 normalDir = normalize(mat3x3(modelView) * mat3x3(gl_ObjectToWorldEXT) * normalize(normal));
    vec3 lightDir = normalize(lightPosition - position);
    vec3 viewDir = normalize(-position);
    vec3 reflectDir = reflect(-lightDir, normalDir);

    vec3 specular = vec3(specularCoeff * pow(max(dot(reflectDir, viewDir), 0.0), shininess));
    vec3 diffuse = diffuseCoeff * color * max(dot(normalDir, lightDir), 0.0);
    vec3 ambient = ambientCoeff * color;

    outColor = abs(specular + diffuse + ambient);
}
