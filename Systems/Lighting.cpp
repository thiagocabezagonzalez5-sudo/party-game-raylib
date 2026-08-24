#include "Systems/Lighting.h"

bool InicializarIluminacion(LightingState& lighting)
{
    const char* vsCode =
        "#version 330\n"
        "in vec3 vertexPosition;\n"
        "in vec2 vertexTexCoord;\n"
        "in vec3 vertexNormal;\n"
        "in vec4 vertexColor;\n"
        "uniform mat4 mvp;\n"
        "uniform mat4 matModel;\n"
        "out vec2 fragTexCoord;\n"
        "out vec4 fragColor;\n"
        "out vec3 fragNormal;\n"
        "void main()\n"
        "{\n"
        "    fragTexCoord = vertexTexCoord;\n"
        "    fragColor = vertexColor;\n"
        "    fragNormal = normalize(mat3(matModel) * vertexNormal);\n"
        "    gl_Position = mvp * vec4(vertexPosition, 1.0);\n"
        "}\n";

    const char* fsCode =
        "#version 330\n"
        "in vec2 fragTexCoord;\n"
        "in vec4 fragColor;\n"
        "in vec3 fragNormal;\n"
        "uniform sampler2D texture0;\n"
        "uniform vec4 colDiffuse;\n"
        "uniform vec3 lightDir;\n"
        "uniform vec4 lightColor;\n"
        "uniform vec4 ambientColor;\n"
        "out vec4 finalColor;\n"
        "void main()\n"
        "{\n"
        "    vec4 texel = texture(texture0, fragTexCoord);\n"
        "    vec3 N = normalize(fragNormal);\n"
        "    vec3 L = normalize(-lightDir);\n"
        "    float diff = max(dot(N, L), 0.0);\n"
        "    vec4 base = texel * colDiffuse * fragColor;\n"
        "    vec3 iluminacion = ambientColor.rgb + lightColor.rgb * diff;\n"
        "    iluminacion = clamp(iluminacion, 0.0, 1.0);\n"
        "    finalColor = vec4(base.rgb * iluminacion, base.a);\n"
        "}\n";

    lighting.shader = LoadShaderFromMemory(vsCode, fsCode);

    if (lighting.shader.id <= 0)
    {
        return false;
    }

    lighting.locLightDir =
        GetShaderLocation(lighting.shader, "lightDir");

    lighting.locLightColor =
        GetShaderLocation(lighting.shader, "lightColor");

    lighting.locAmbientColor =
        GetShaderLocation(lighting.shader, "ambientColor");

    ActualizarIluminacion(lighting);

    return true;
}

void ActualizarIluminacion(const LightingState& lighting)
{
    if (lighting.shader.id <= 0)
    {
        return;
    }

    SetShaderValue(
        lighting.shader,
        lighting.locLightDir,
        (void*)&lighting.lightDir,
        SHADER_UNIFORM_VEC3
    );

    SetShaderValue(
        lighting.shader,
        lighting.locLightColor,
        (void*)lighting.lightColor,
        SHADER_UNIFORM_VEC4
    );

    SetShaderValue(
        lighting.shader,
        lighting.locAmbientColor,
        (void*)lighting.ambientColor,
        SHADER_UNIFORM_VEC4
    );
}

void AplicarShaderAlModelo(
    Model& model,
    const LightingState& lighting
)
{
    for (int i = 0; i < model.materialCount; i++)
    {
        model.materials[i].shader = lighting.shader;
    }
}

void DibujarDecoracionCielo(
    const LightingState& lighting
)
{
    DrawSphere(lighting.posicionSol, 8.0f, Color{ 255, 245, 180, 255 });
}

void DescargarIluminacion(LightingState& lighting)
{
    if (lighting.shader.id > 0)
    {
        UnloadShader(lighting.shader);
        lighting.shader.id = 0;
    }
}