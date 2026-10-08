#pragma once

#include "Core/Participante.h"
#include "Core/ResultadoMinijuego.h"
#include "Minigames/TiposMinijuegos.h"
#include "Systems/Audio.h"


enum FaseMinijuegoPelotas
{
    FASE_PELOTAS_PREPARACION = 0,
    FASE_PELOTAS_JUGANDO,
    FASE_PELOTAS_TERMINADO
};


struct EstadoJugadorPelotas
{
    bool eliminado = false;
    int posicionFinal = 0;
    int tiempoSobrevividoMs = 0;
};


// Parametros aleatorios por partida de cada bot (no dependen del indice).
struct EstadoBotPelotas
{
    float faseX = 0.0f;
    float faseZ = 0.0f;
    float faseDuda = 0.0f;
    float amplitudError = 0.9f;
    float retardoBorde = 0.0f;
    float esperaBorde = 0.0f;
};


struct MinijuegoPelotas
{
    ResultadoMinijuego resultado;
    AudioJuego* audio = nullptr;

    EstadoJugadorPelotas estadosJugadores[
        MAX_JUGADORES_PRUEBA
    ];

    EstadoBotPelotas bots[MAX_JUGADORES_PRUEBA];

    BloquePrueba bloques[1];
    int cantidadBloques = 0;

    Camera3D camara{};

    FaseMinijuegoPelotas fase =
        FASE_PELOTAS_PREPARACION;

    float tiempoPreparacion = 3.0f;
    float tiempoRestante = 60.0f;
    float tiempoJugado = 0.0f;
    bool terminoPorTiempo = false;

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
        bool mostrarDebug
    ) const;

    const ResultadoMinijuego& ObtenerResultado() const;
};
