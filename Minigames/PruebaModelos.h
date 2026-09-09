#pragma once

#include "raylib.h"
#include "Core/RecursosJuego.h"

#define SOMBRAS_RETRO_AUTOMATICAS
#include "Minigames/SombrasRetro.h"


struct PruebaModelos
{
    Model modelo{};

    ModelAnimation* animaciones =
        nullptr;

    int cantidadAnimaciones =
        0;

    int indiceAnimacionIdle =
        -1;

    float fotogramaAnimacionIdle =
        0.0f;

    bool animacionIdleActiva =
        false;

    bool modeloCargado = false;

    // La ruta real vive en Core/RecursosJuego.h.
    const char* rutaModelo =
        RUTA_MODELO_JUGADOR_3D;

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
