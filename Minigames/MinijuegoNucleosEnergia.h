#pragma once

#include "Core/Participante.h"
#include "Core/ResultadoMinijuego.h"
#include "Minigames/TiposMinijuegos.h"
#include "Systems/Audio.h"
#include "raylib.h"


const int MAX_NUCLEOS_ENERGIA = 6;
const int MAX_BLOQUES_NUCLEOS = 5;


enum FaseMinijuegoNucleos
{
    FASE_NUCLEOS_PREPARACION = 0,
    FASE_NUCLEOS_JUGANDO,
    FASE_NUCLEOS_TERMINADO
};


struct NucleoEnergia
{
    Vector3 posicion{};

    bool activo = false;
    bool especial = false;

    float tiempoReaparicion = 0.0f;
    float faseFlotacion = 0.0f;
};


struct MinijuegoNucleosEnergia
{
    Camera3D camara{};

    BloquePrueba bloques[MAX_BLOQUES_NUCLEOS];
    int cantidadBloques = 0;

    NucleoEnergia nucleos[MAX_NUCLEOS_ENERGIA];

    int puntuaciones[MAX_PARTICIPANTES]{};

    ResultadoMinijuego resultado{};

    FaseMinijuegoNucleos fase =
        FASE_NUCLEOS_PREPARACION;

    float tiempoPreparacion = 3.0f;
    float tiempoRestante = 35.0f;
    float tiempoAnimacion = 0.0f;

    int ultimoNumeroCuenta = -1;
    int ultimoNumeroAlerta = -1;

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
        int cantidadParticulas,
        AudioJuego* audio
    );

    void Dibujar(
        const JugadorPrueba jugadores[],
        int cantidadMaxima,
        const Participante participantes[],
        bool mostrarDebug
    ) const;
};
