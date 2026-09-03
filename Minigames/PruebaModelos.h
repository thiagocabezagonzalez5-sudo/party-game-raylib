#pragma once

#include "raylib.h"


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

    const char* rutaModelo =
        "Assets/Modelos/Jugador_Raylib_Normalizado.glb";

    Camera3D camara{};

    float rotacion = 0.0f;
    float escala = 0.25f;

    bool rotacionAutomatica = true;

    void Inicializar();

    void Reiniciar();

    void Actualizar(
        float deltaTime
    );

    void Dibujar() const;

    void Descargar();
};
