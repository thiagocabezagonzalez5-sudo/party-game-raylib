#pragma once

#include "Core/Participante.h"
#include "Core/ResultadoMinijuego.h"
#include "Minigames/TiposMinijuegos.h"


inline constexpr int MAX_ROCAS_CANTERA_FUGA = 12;


enum FaseCanteraFuga
{
    FASE_CANTERA_PREPARACION = 0,
    FASE_CANTERA_JUGANDO,
    FASE_CANTERA_TERMINADO
};


struct EstadoJugadorCanteraFuga
{
    float progresoMaximo = 0.0f;
    float tiempoAturdido = 0.0f;
    float impulsoLateral = 0.0f;
    int impactosRecibidos = 0;
};


struct RocaCanteraFuga
{
    bool activa = false;
    Vector3 posicion{};
    float velocidadX = 0.0f;
    float velocidadZ = 0.0f;
    float radio = 0.72f;
};


struct MinijuegoCanteraFuga
{
    ResultadoMinijuego resultado{};
    EstadoJugadorCanteraFuga estadosJugadores[MAX_PARTICIPANTES];
    RocaCanteraFuga rocas[MAX_ROCAS_CANTERA_FUGA];

    Camera3D camara{};
    FaseCanteraFuga fase = FASE_CANTERA_PREPARACION;

    int indiceSolo = -1;
    int impactosSolo = 0;

    float posicionTolvaX = 0.0f;
    float cooldownRoca = 0.0f;
    float tiempoDecisionBot = 0.0f;
    float tiempoPreparacion = 0.0f;
    float tiempoRestante = 0.0f;
    float tiempoAnimacion = 0.0f;

    void Inicializar();

    void Reiniciar(
        JugadorPrueba jugadores[],
        Participante participantes[],
        int cantidadMaxima
    );

    void Actualizar(
        float deltaTime,
        JugadorPrueba jugadores[],
        int cantidadMaxima,
        Participante participantes[]
    );

    void Dibujar(
        const JugadorPrueba jugadores[],
        int cantidadMaxima,
        const Participante participantes[],
        bool mostrarDebug
    ) const;

    const ResultadoMinijuego& ObtenerResultado() const;
};
