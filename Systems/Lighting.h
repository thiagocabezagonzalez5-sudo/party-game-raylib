#pragma once

#include "raylib.h"

struct LightingState
{
    Shader shader = { 0 };

    int locLightDir = -1;
    int locLightColor = -1;
    int locAmbientColor = -1;

    Vector3 lightDir = Vector3{ -0.7f, -1.0f, -0.5f };

    float lightColor[4] = { 1.0f, 0.95f, 0.85f, 1.0f };
    float ambientColor[4] = { 0.42f, 0.48f, 0.58f, 1.0f };

    Color colorCielo = Color{ 135, 206, 235, 255 };
    Vector3 posicionSol = Vector3{ 60.0f, 80.0f, -120.0f };
};

bool InicializarIluminacion(LightingState& lighting);

void ActualizarIluminacion(const LightingState& lighting);

void AplicarShaderAlModelo(
    Model& model,
    const LightingState& lighting
);

void DibujarDecoracionCielo(
    const LightingState& lighting
);

void DescargarIluminacion(LightingState& lighting);