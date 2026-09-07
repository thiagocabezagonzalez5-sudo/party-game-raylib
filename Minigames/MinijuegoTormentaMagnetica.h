#pragma once

#include "Core/Participante.h"
#include "Core/ResultadoMinijuego.h"
#include "Minigames/TiposMinijuegos.h"


enum FaseTormentaMagnetica
{
    FASE_MAGNETICA_PREPARACION = 0,
    FASE_MAGNETICA_JUGANDO,
    FASE_MAGNETICA_TERMINADO
};


struct EstadoJugadorTormentaMagnetica
{
    bool eliminado = false;
    int posicionFinal = 0;
    int tiempoSobrevividoMs = 0;
};


struct MinijuegoTormentaMagnetica
{
    ResultadoMinijuego resultado;

    EstadoJugadorTormentaMagnetica estadosJugadores[
        MAX_JUGADORES_PRUEBA
    ];

    BloquePrueba suelo;
    Camera3D camara{};

    Vector3 posicionNucleo{};
    bool campoAtrae = true;

    FaseTormentaMagnetica fase =
        FASE_MAGNETICA_PREPARACION;

    float tiempoPreparacion = 3.0f;
    float tiempoRestante = 35.0f;
    float tiempoJugado = 0.0f;
    float tiempoHastaCambioCampo = 3.4f;
    float tiempoAnimacion = 0.0f;

    int cambiosCampo = 0;

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
