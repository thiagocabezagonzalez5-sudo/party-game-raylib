#pragma once

#include "Core/Participante.h"
#include "Core/ResultadoMinijuego.h"
#include "Systems/Audio.h"
#include "Systems/Input.h"


enum FaseCapitanManda
{
    FASE_CAPITAN_PREPARACION = 0,
    FASE_CAPITAN_MOSTRANDO,
    FASE_CAPITAN_RESPONDIENDO,
    FASE_CAPITAN_RESOLVIENDO,
    FASE_CAPITAN_TERMINADO
};


struct EstadoJugadorCapitanManda
{
    bool eliminado = false;
    bool respondio = false;
    bool acerto = false;

    int posicionFinal = 0;
    int rondasSuperadas = 0;

    float tiempoFeedback = 0.0f;

    // Bots: cuando reaccionan y si aciertan la direccion.
    float tiempoBot = 0.0f;
    bool botAcertara = true;

    // Solo visual: hacia donde giro (-1 izquierda, 1 derecha, 0 nada o
    // ambas). Se conoce recien cuando la bandera ya subio.
    int direccionRespuesta = 0;
    float giroVisual = 180.0f;
    float velocidadGiro = 0.0f;
    float destello = 0.0f;          // brillo de acierto (1 -> 0)
    float sacudida = 0.0f;          // temblor de error (1 -> 0)
    float tiempoCaida = -1.0f;      // < 0: no esta cayendo al agua
    bool salpico = false;
};


// Particula visual (chispas, salpicaduras, espuma y confeti).
struct ParticulaCapitanManda
{
    bool activa = false;
    Vector3 posicion = { 0.0f, 0.0f, 0.0f };
    Vector3 velocidad = { 0.0f, 0.0f, 0.0f };
    float vida = 0.0f;
    float vidaMaxima = 1.0f;
    float tamano = 0.1f;
    float gravedad = 0.0f;
    Color color = { 255, 255, 255, 255 };
};


static const int MAX_PARTICULAS_CAPITAN = 220;


struct MinijuegoCapitanManda
{
    ResultadoMinijuego resultado;
    AudioJuego* audio = nullptr;

    EstadoJugadorCapitanManda jugadores[
        MAX_PARTICIPANTES
    ];

    FaseCapitanManda fase =
        FASE_CAPITAN_PREPARACION;

    AccionDireccionalControl ordenActual =
        CONTROL_DIRECCION_IZQUIERDA;

    int numeroRonda = 0;

    float tiempoPreparacion = 3.0f;
    float tiempoFase = 0.0f;
    float tiempoRespuesta = 1.55f;

    bool resultadoInicializado = false;

    // Solo visual: reloj de animacion, particulas y azar propio (no toca
    // el GetRandomValue que sortea las ordenes).
    float tiempoAnimacion = 0.0f;
    unsigned int semillaVisual = 12345u;
    bool confetiLanzado = false;
    ParticulaCapitanManda particulas[MAX_PARTICULAS_CAPITAN];

    void Inicializar();

    void Reiniciar(
        const Participante participantes[]
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
