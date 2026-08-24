#pragma once

#include "raylib.h"
#include "Systems/Lighting.h"

struct Plataforma
{
    Vector3 posicion;
    Vector3 tamano;
    
    Model modelo;

    float ObtenerAlturaSuperior() const;

    BoundingBox ObtenerHitbox() const;

    void Dibujar() const;
};