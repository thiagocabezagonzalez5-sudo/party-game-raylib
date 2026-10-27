#pragma once

#include "Core/Participante.h"
#include "Core/ResultadoMinijuego.h"
#include "Systems/Audio.h"


enum FaseCircuitoVoltaje
{
    FASE_CIRCUITO_PREPARACION = 0,
    FASE_CIRCUITO_CARRERA,
    FASE_CIRCUITO_TERMINADO
};


struct EstadoJugadorCircuitoVoltaje
{
    float avance = 0.0f;
    float velocidad = 0.0f;
    float tiempoExcesoCurva = 0.0f;
    float tiempoTrompo = 0.0f;

    // Personalidad del bot, sorteada al reiniciar para que las partidas varien.
    float margenBot = 0.06f;
    float faseErrorBot = 0.0f;

    int trompos = 0;
    bool llegoMeta = false;
};


static const int MAX_PARTICULAS_CIRCUITO_VOLTAJE = 320;
static const int MAX_AVISOS_CIRCUITO_VOLTAJE = 8;


// Solo visual: chispas, humo de trompo, confeti y fuegos artificiales.
struct ParticulaCircuitoVoltaje
{
    Vector3 posicion{};
    Vector3 velocidad{};
    Color color{};
    float vida = 0.0f;
    float duracion = 1.0f;
    float tamano = 0.1f;
    int tipo = 0;
    bool activa = false;
};


// Solo visual: cartel sobre un auto («¡TROMPO!», «¡VUELTA 2!», «¡META!»).
struct AvisoCircuitoVoltaje
{
    bool activo = false;
    int jugador = 0;
    int tipo = 0;
    int valor = 0;
    float vida = 0.0f;
};


struct MinijuegoCircuitoVoltaje
{
    ResultadoMinijuego resultado{};
    AudioJuego* audio = nullptr;
    EstadoJugadorCircuitoVoltaje jugadores[MAX_PARTICIPANTES];

    FaseCircuitoVoltaje fase = FASE_CIRCUITO_PREPARACION;

    float tiempoPreparacion = 0.0f;
    float tiempoCarrera = 0.0f;
    float tiempoAnimacion = 0.0f;

    // Estado solo visual: se deduce de la carrera y no la modifica.
    ParticulaCircuitoVoltaje particulas[MAX_PARTICULAS_CIRCUITO_VOLTAJE];
    AvisoCircuitoVoltaje avisos[MAX_AVISOS_CIRCUITO_VOLTAJE];
    int siguienteParticula = 0;
    unsigned int semillaVisual = 0;
    int vueltaVista[MAX_PARTICIPANTES]{};
    int tromposVistos[MAX_PARTICIPANTES]{};
    bool metaVista[MAX_PARTICIPANTES]{};
    bool excesoVisto[MAX_PARTICIPANTES]{};
    float destelloVuelta[MAX_PARTICIPANTES]{};
    float acumuladorChispas[MAX_PARTICIPANTES]{};
    int vueltaLiderVista = 0;
    float tiempoAnuncioVuelta = 99.0f;
    float tiempoDesdeFin = 0.0f;
    float acumuladorFuegos = 0.0f;

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
