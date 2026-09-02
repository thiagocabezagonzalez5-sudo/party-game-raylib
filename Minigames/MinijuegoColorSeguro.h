#pragma once

#include "Core/ConfiguracionJuego.h"
#include "Minigames/TiposMinijuegos.h"


const int CANTIDAD_PLATAFORMAS_COLOR = 7;


enum FaseMinijuegoColor
{
    FASE_ELEGIR_PLATAFORMA = 0,
    FASE_CAIDA_PLATAFORMAS
};


struct MinijuegoColorSeguro
{
    BloquePrueba plataformas[CANTIDAD_PLATAFORMAS_COLOR];
    int cantidadPlataformas = 0;

    Camera3D camara{};

    Vector3 posicionCamaraBase{};
    Vector3 objetivoCamaraBase{};

    float tiempoTemblorCamara = 0.0f;
    float intensidadTemblorCamara = 0.0f;

    FaseMinijuegoColor fase =
        FASE_ELEGIR_PLATAFORMA;

    int indicePlataformaSegura = 0;
    int numeroRonda = 1;

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
        ModoTeclado modoTeclado,
        ParticulaTierra particulas[],
        int cantidadParticulas
    );

    void Dibujar(
        const JugadorPrueba jugadores[],
        int cantidadMaxima,
        const ParticulaTierra particulas[],
        int cantidadParticulas,
        bool mostrarDebug
    ) const;
};
