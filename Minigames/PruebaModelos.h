#pragma once

#include "raylib.h"
#include "Core/RecursosJuego.h"

#define SOMBRAS_RETRO_AUTOMATICAS
#include "Minigames/SombrasRetro.h"


enum AnimacionPruebaModelos
{
    ANIMACION_PRUEBA_REPOSO = 0,
    ANIMACION_PRUEBA_CAMINAR,
    ANIMACION_PRUEBA_GOLPE,
    ANIMACION_PRUEBA_CAIDA,
    ANIMACION_PRUEBA_TAMBALEO,
    ANIMACION_PRUEBA_SALTO,

    CANTIDAD_ANIMACIONES_PRUEBA_MODELOS
};


// Vitrina de los personajes de bloques en Zona de Pruebas.
struct PruebaModelos
{
    Camera3D camara{};

    float rotacion = 0.0f;
    float escala = ESCALA_MODELO_JUGADOR_3D;

    bool rotacionAutomatica = true;

    float tiempo = 0.0f;
    AnimacionPruebaModelos animacion = ANIMACION_PRUEBA_REPOSO;

    void Inicializar();

    void Reiniciar();

    void Actualizar(
        float deltaTime
    );

    void Dibujar() const;

    void Descargar();
};
