#pragma once

#include "Core/Participante.h"
#include "Core/ResultadoMinijuego.h"
#include "Minigames/TiposMinijuegos.h"


enum FaseMurosLocos
{
    FASE_MUROS_PREPARACION = 0,
    FASE_MUROS_JUGANDO,
    FASE_MUROS_TERMINADO
};


struct EstadoJugadorMurosLocos
{
    bool eliminado = false;
    int posicionFinal = 0;
    int tiempoSobrevividoMs = 0;
    float cooldownImpacto = 0.0f;
};


struct MuroLoco
{
    float z = -7.2f;
    float velocidad = 4.2f;
    float anchoHueco = 2.5f;

    int cantidadHuecos = 1;
    float centrosHueco[2] = { 0.0f, 0.0f };
};


struct MinijuegoMurosLocos
{
    ResultadoMinijuego resultado;

    EstadoJugadorMurosLocos estadosJugadores[
        MAX_JUGADORES_PRUEBA
    ];

    BloquePrueba suelo;
    MuroLoco muro;
    Camera3D camara{};

    FaseMurosLocos fase = FASE_MUROS_PREPARACION;

    float tiempoPreparacion = 3.0f;
    float tiempoRestante = 35.0f;
    float tiempoJugado = 0.0f;
    float tiempoEntreMuros = 0.0f;

    int numeroMuro = 0;

    void Inicializar();

    void ConfigurarJugadores(
        JugadorPrueba jugadores[],
        int cantidadMaxima
    ) const;

    void Reiniciar(
        JugadorPrueba jugadores[],
        int cantidadMaxima
    );

    void Actualizar(
        float deltaTime,
        JugadorPrueba jugadores[],
        int cantidadMaxima,
        Participante participantes[],
        ParticulaTierra particulas[],
        int cantidadParticulas
    );

    void Dibujar(
        const JugadorPrueba jugadores[],
        int cantidadMaxima,
        const Participante participantes[],
        const ParticulaTierra particulas[],
        int cantidadParticulas,
        bool mostrarDebug
    ) const;

    const ResultadoMinijuego& ObtenerResultado() const;
};
