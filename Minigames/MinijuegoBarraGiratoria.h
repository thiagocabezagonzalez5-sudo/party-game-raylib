#pragma once

#include "Core/Participante.h"
#include "Core/ResultadoMinijuego.h"
#include "Minigames/TiposMinijuegos.h"


enum FaseMinijuegoBarraGiratoria
{
    FASE_BARRA_PREPARACION = 0,
    FASE_BARRA_JUGANDO,
    FASE_BARRA_TERMINADO
};


struct EstadoJugadorBarraGiratoria
{
    bool eliminado = false;
    int posicionFinal = 0;
    int tiempoSobrevividoMs = 0;
    float cooldownImpacto = 0.0f;
};


struct MinijuegoBarraGiratoria
{
    ResultadoMinijuego resultado;

    EstadoJugadorBarraGiratoria estadosJugadores[
        MAX_JUGADORES_PRUEBA
    ];

    BloquePrueba suelo;
    Camera3D camara{};

    FaseMinijuegoBarraGiratoria fase =
        FASE_BARRA_PREPARACION;

    float tiempoPreparacion = 3.0f;
    float tiempoRestante = 45.0f;
    float tiempoJugado = 0.0f;

    float anguloBarra = 0.0f;
    float velocidadAngular = 0.9f;

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
