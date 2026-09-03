#pragma once

#include "Core/Participante.h"
#include "Core/ResultadoMinijuego.h"
#include "Minigames/TiposMinijuegos.h"


const int CANTIDAD_PLATAFORMAS_COLOR = 7;


enum FaseMinijuegoColor
{
    FASE_ELEGIR_PLATAFORMA = 0,
    FASE_CAIDA_PLATAFORMAS
};


enum EstadoPartidaColorSeguro
{
    COLOR_SEGURO_PREPARACION = 0,
    COLOR_SEGURO_JUGANDO,
    COLOR_SEGURO_TERMINADO
};


struct EstadoJugadorColorSeguro
{
    bool eliminado = false;
    int posicionFinal = 0;
    int tiempoSobrevividoMs = 0;
    int rondasSobrevividas = 0;
};


struct MinijuegoColorSeguro
{
    ResultadoMinijuego resultado;

    EstadoJugadorColorSeguro estadosJugadores[
        MAX_JUGADORES_PRUEBA
    ];

    BloquePrueba plataformas[CANTIDAD_PLATAFORMAS_COLOR];
    int cantidadPlataformas = 0;

    Camera3D camara{};

    Vector3 posicionCamaraBase{};
    Vector3 objetivoCamaraBase{};

    float tiempoTemblorCamara = 0.0f;
    float intensidadTemblorCamara = 0.0f;

    EstadoPartidaColorSeguro estado =
        COLOR_SEGURO_PREPARACION;

    FaseMinijuegoColor fase =
        FASE_ELEGIR_PLATAFORMA;

    int indicePlataformaSegura = 0;
    int numeroRonda = 1;

    float tiempoPreparacion = 3.0f;
    float tiempoRestante = 60.0f;
    float tiempoJugado = 0.0f;

    float tiempoFase = 5.0f;
    float duracionElegirPlataforma = 5.0f;
    float duracionCaidaPlataformas = 2.0f;

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
