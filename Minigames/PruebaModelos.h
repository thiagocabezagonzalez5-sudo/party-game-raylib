#pragma once

#include "raylib.h"
#include "Core/RecursosJuego.h"

#define SOMBRAS_RETRO_AUTOMATICAS
#include "Minigames/SombrasRetro.h"


struct PruebaModelos
{
    float fotogramaAnimacionIdle =
        0.0f;

    Camera3D camara{};

    float rotacion = 0.0f;
    float escala = ESCALA_MODELO_JUGADOR_3D;

    bool rotacionAutomatica = true;

    void Inicializar();

    void Reiniciar();

    void Actualizar(
        float deltaTime
    );

    void Dibujar() const;

    void Descargar();
};
