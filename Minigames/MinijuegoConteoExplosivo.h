#pragma once

#include "Core/Participante.h"
#include "Core/ResultadoMinijuego.h"
#include "Systems/Audio.h"
#include "raylib.h"


inline constexpr int MAX_DRONES_CONTEO_EXPLOSIVO = 22;


enum FaseConteoExplosivo
{
    FASE_CONTEO_PREPARACION = 0,
    FASE_CONTEO_OBSERVACION,
    FASE_CONTEO_RESPUESTA,
    FASE_CONTEO_TERMINADO
};


struct DronConteoExplosivo
{
    Vector3 posicion{};
    Vector3 velocidad{};
    Color color = SKYBLUE;
    float faseFlotacion = 0.0f;
};


struct EstadoJugadorConteoExplosivo
{
    int respuesta = 10;
    int diferencia = 0;
    bool botRespondio = false;
    float retrasoBot = 0.0f;

    // Seleccion rapida: mantener una direccion repite y acelera.
    int direccionMantenida = 0;
    float tiempoMantenido = 0.0f;
    float proximoPaso = 0.0f;
};


struct MinijuegoConteoExplosivo
{
    ResultadoMinijuego resultado{};
    AudioJuego* audio = nullptr;

    DronConteoExplosivo drones[MAX_DRONES_CONTEO_EXPLOSIVO];
    EstadoJugadorConteoExplosivo jugadores[MAX_PARTICIPANTES];

    Camera3D camara{};
    FaseConteoExplosivo fase = FASE_CONTEO_PREPARACION;

    int cantidadDrones = 0;
    float tiempoFase = 0.0f;
    float tiempoAnimacion = 0.0f;

    void Inicializar();

    void Reiniciar(
        Participante participantes[]
    );

    void Actualizar(
        float deltaTime,
        Participante participantes[]
    );

    void Dibujar(
        const Participante participantes[]
    ) const;

    const ResultadoMinijuego& ObtenerResultado() const;
};
