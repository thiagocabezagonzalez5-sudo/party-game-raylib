#pragma once

#include "Core/Participante.h"
#include "Core/ResultadoMinijuego.h"
#include "Minigames/TiposMinijuegos.h"


inline constexpr int MAX_PLATAFORMAS_PASARELAS = 9;


enum FasePasarelasVacio
{
    FASE_PASARELAS_PREPARACION = 0,
    FASE_PASARELAS_CARRERA,
    FASE_PASARELAS_TERMINADO
};


struct EstadoJugadorPasarelasVacio
{
    bool eliminado = false;
    bool llegoMeta = false;
    int posicionFinal = 0;
    int indiceObjetivoBot = 1;
    float progresoMaximo = 0.0f;
};


struct EstadoPlataformaPasarelas
{
    bool activada = false;
    bool cayendo = false;
    float tiempoDerrumbe = 0.0f;
    float velocidadCaida = 0.0f;
};


struct MinijuegoPasarelasVacio
{
    ResultadoMinijuego resultado{};
    EstadoJugadorPasarelasVacio estadosJugadores[MAX_PARTICIPANTES];
    EstadoPlataformaPasarelas estadosPlataformas[MAX_PLATAFORMAS_PASARELAS];

    BloquePrueba plataformas[MAX_PLATAFORMAS_PASARELAS];
    int cantidadPlataformas = 0;

    Camera3D camara{};
    FasePasarelasVacio fase = FASE_PASARELAS_PREPARACION;

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
