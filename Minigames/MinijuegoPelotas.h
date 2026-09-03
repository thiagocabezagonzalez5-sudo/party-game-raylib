#pragma once

#include "Core/Participante.h"
#include "Minigames/TiposMinijuegos.h"


struct MinijuegoPelotas
{
    BloquePrueba bloques[1];
    int cantidadBloques = 0;

    Camera3D camara{};

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
};
