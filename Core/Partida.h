#pragma once

#include "raylib.h"
#include "Entities/Jugador.h"
//#include "Minigames/Minijuegos.h"

struct Partida{
    Jugador jugadores [4];

    int cantidadJugadores;

    int rondaActual;
    int cantidadRondas;

    //TipoMinijuego minijuegoActual;

    void Inicializar(
        int cantidadJugadores
    );

    void Actualizar(float deltaTime);

    void ActualizarMinijuego(float deltaTime);

    void TerminarMinijuego();

    void Dibujar() const;

    void Reiniciar();
};