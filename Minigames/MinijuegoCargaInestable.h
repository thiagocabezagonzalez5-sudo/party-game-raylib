#pragma once

#include "Core/Participante.h"
#include "Core/ResultadoMinijuego.h"
#include "Systems/Audio.h"
#include "raylib.h"


inline constexpr int MAX_PARTICULAS_CARGA = 80;


enum FaseCargaInestable
{
    FASE_CARGA_PREPARACION = 0,
    FASE_CARGA_ACTIVA,
    FASE_CARGA_EXPLOSION,
    FASE_CARGA_TERMINADO
};


struct EstadoJugadorCargaInestable
{
    bool eliminado = false;
    int posicionFinal = 0;
    int cantidadPases = 0;
    float tiempoDecisionBot = 0.0f;
};


// Chispa de la explosion (solo visual).
struct ParticulaCarga
{
    bool activa = false;
    Vector3 posicion{};
    Vector3 velocidad{};
    float vida = 0.0f;
    float vidaMaxima = 1.0f;
    Color color = WHITE;
};


struct MinijuegoCargaInestable
{
    ResultadoMinijuego resultado{};
    AudioJuego* audio = nullptr;
    EstadoJugadorCargaInestable jugadores[MAX_PARTICIPANTES];
    ParticulaCarga particulas[MAX_PARTICULAS_CARGA];

    FaseCargaInestable fase = FASE_CARGA_PREPARACION;

    int portador = -1;
    int portadorExplosion = -1;
    int numeroRonda = 0;

    // Solo visual: de quien viene la carga en vuelo (-1 = caldera central).
    int origenPase = -1;
    float progresoPase = 1.0f;
    float tiempoTick = 0.0f;

    float tiempoPreparacion = 0.0f;
    float tiempoCarga = 0.0f;
    float duracionCarga = 0.0f;
    float bloqueoPase = 0.0f;
    float tiempoExplosion = 0.0f;
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
