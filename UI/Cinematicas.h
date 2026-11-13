#pragma once

#include "Core/CatalogoMinijuegos.h"
#include "Core/Participante.h"
#include "UI/EstiloUI.h"

#include "raylib.h"


//==================================================
// CINEMATICAS
//==================================================
//
// Presentaciones breves con camara animada, los personajes de bloques y
// franjas de cine. Todas se pueden omitir (Enter/Espacio/Esc o A/B del
// mando) y respetan "reducir movimiento" (camara quieta, sin temblores).
// No cargan recursos: escena procedural + EstiloUI.
//
// - Apertura: al iniciar el juego, los cuatro personajes llegan al
//   escenario y aparece el titulo (antes del menu).
// - Minijuego: entre la ruleta y el minijuego de una ronda de tablero, el
//   nombre del minijuego y quienes lo juegan.
// - Campeon: al terminar la partida, el ganador festeja en el podio antes
//   de la pantalla de resultados.
//==================================================

struct EstadoCinematica
{
    float tiempo = 0.0f;
    float duracion = 1.0f;
    bool terminada = false;
    bool omitida = false;

    LectorEntradaUI lector;
};


// Un participante tal como lo muestra una cinematica.
struct FiguraCinematica
{
    bool activo = false;
    int personaje = 0;
    int numeroJugador = 1;
    bool esBot = false;
    Color color = WHITE;
};


struct CinematicaApertura
{
    static constexpr float DURACION = 7.2f;

    EstadoCinematica estado;

    void Inicializar();
    void Actualizar(float deltaTime);
    void Dibujar() const;
};


struct CinematicaMinijuego
{
    static constexpr float DURACION = 2.8f;

    EstadoCinematica estado;

    IdMinijuego minijuego = MINIJUEGO_COLOR_SEGURO;
    bool duelo = false;

    FiguraCinematica figuras[MAX_PARTICIPANTES];
    int cantidad = 0;

    void Inicializar(
        IdMinijuego id,
        const Participante participantes[],
        bool esDuelo
    );

    void Actualizar(float deltaTime);
    void Dibujar() const;
};


struct CinematicaCampeon
{
    static constexpr float DURACION = 5.6f;

    EstadoCinematica estado;

    // ranking[0] es el ganador; puesto[k] es 1..4 (se repite si empatan).
    FiguraCinematica figuras[MAX_PARTICIPANTES];
    int puesto[MAX_PARTICIPANTES] = {};
    int cantidad = 0;
    bool empateEnPrimero = false;

    void Inicializar(
        const int ranking[],
        const int puestos[],
        int cantidadJugadores,
        const Participante participantes[]
    );

    void Actualizar(float deltaTime);
    void Dibujar() const;
};
