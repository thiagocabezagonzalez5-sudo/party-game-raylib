#pragma once

#include "Core/Participante.h"
#include "Core/ResultadoMinijuego.h"
#include "Minigames/TiposMinijuegos.h"


const int MAX_PINCHOS_TORMENTA_MAGNETICA = 12;


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


struct PinchoTormentaMagnetica
{
    bool activo = false;
    Vector3 posicion{};
    Vector3 velocidad{};
    float tiempoVida = 0.0f;
};


struct MinijuegoTormentaMagnetica
{
    ResultadoMinijuego resultado;

    EstadoJugadorTormentaMagnetica estadosJugadores[
        MAX_JUGADORES_PRUEBA
    ];

    PinchoTormentaMagnetica pinchos[
        MAX_PINCHOS_TORMENTA_MAGNETICA
    ];

    BloquePrueba suelo;
    Camera3D camara{};

    Vector3 posicionNucleo{};
    bool campoAtrae = true;

    FaseTormentaMagnetica fase = FASE_MAGNETICA_PREPARACION;

    float tiempoPreparacion = 3.0f;
    float tiempoRestante = 0.0f;
    float tiempoJugado = 0.0f;
    float tiempoHastaCambioCampo = 3.0f;
    float tiempoHastaPincho = 1.2f;
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
