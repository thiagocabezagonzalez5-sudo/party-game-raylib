#pragma once

#include "Core/Participante.h"
#include "Core/ResultadoMinijuego.h"
#include "Minigames/TiposMinijuegos.h"


enum FaseMinijuegoIslaFuego
{
    FASE_ISLA_FUEGO_PREPARACION = 0,
    FASE_ISLA_FUEGO_JUGANDO,
    FASE_ISLA_FUEGO_TERMINADO
};


struct EstadoJugadorIslaFuego
{
    bool eliminado = false;
    int posicionFinal = 0;
    int tiempoSobrevividoMs = 0;
    float tiempoAturdido = 0.0f;
};


struct ProyectilIslaFuego
{
    bool activo = false;
    bool especial = false;

    Vector3 puntoImpacto{};

    float tiempoHastaImpacto = 0.0f;
    float duracionAviso = 1.25f;
    float radioExplosion = 2.0f;
};


struct MinijuegoIslaFuego
{
    ResultadoMinijuego resultado;

    EstadoJugadorIslaFuego estadosJugadores[
        MAX_JUGADORES_PRUEBA
    ];

    BloquePrueba suelo;

    Camera3D camara{};

    FaseMinijuegoIslaFuego fase =
        FASE_ISLA_FUEGO_PREPARACION;

    ProyectilIslaFuego proyectil;

    float tiempoPreparacion = 3.0f;
    float tiempoRestante = 30.0f;
    float tiempoJugado = 0.0f;
    float tiempoHastaSiguienteDisparo = 0.8f;

    bool disparoFinalRealizado = false;

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
