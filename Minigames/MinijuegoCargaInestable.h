#pragma once

#include "Core/Participante.h"
#include "Core/ResultadoMinijuego.h"
#include "Systems/Audio.h"
#include "raylib.h"


inline constexpr int MAX_PARTICULAS_CARGA = 260;


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
    int rondaEliminacion = 0;
    float tiempoDecisionBot = 0.0f;
};


enum TipoParticulaCarga
{
    PARTICULA_CARGA_CHISPA = 0,
    PARTICULA_CARGA_HUMO,
    PARTICULA_CARGA_VAPOR,
    PARTICULA_CARGA_FUEGO,
    PARTICULA_CARGA_ESCOMBRO
};


// Chispas, humo, vapor, fuego o escombros (solo visual).
struct ParticulaCarga
{
    bool activa = false;
    TipoParticulaCarga tipo = PARTICULA_CARGA_CHISPA;
    Vector3 posicion{};
    Vector3 velocidad{};
    float vida = 0.0f;
    float vidaMaxima = 1.0f;
    float tamano = 0.1f;
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

    // Calor por escalones (0..3). Los umbrales se sortean en cada carga:
    // el aspecto transmite tension sin revelar cuanto falta para explotar.
    float umbralCalor[3] = { 0.0f, 0.0f, 0.0f };
    int nivelCalor = 0;
    float nivelVisual = 0.0f;
    float destelloTick = 0.0f;
    float destelloCaptura = 0.0f;
    float acumuladorHumo = 0.0f;
    float tiempoRonda = 0.0f;
    Vector3 posicionExplosion{};

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
