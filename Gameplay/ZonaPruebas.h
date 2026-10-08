#pragma once

#include "Core/CatalogoMinijuegos.h"
#include "Core/Participante.h"
#include "Gameplay/ContextoMinijuego.h"
#include "Gameplay/GestorMinijuegos.h"
#include "Gameplay/PrototipoTablero.h"
#include "Minigames/PruebaModelos.h"
#include "Minigames/TiposMinijuegos.h"
#include "Systems/Audio.h"


enum ModoZonaPruebas
{
    PRUEBA_ZONA_PRINCIPAL = -4,
    PRUEBA_MODELOS = -3,
    PRUEBA_TABLERO = -2,
    PRUEBA_MINIJUEGO = -1
};


struct ZonaPruebas
{
    ModoZonaPruebas modoActual = PRUEBA_ZONA_PRINCIPAL;

    JugadorPrueba jugadores[MAX_JUGADORES_PRUEBA];

    Participante* participantes = nullptr;
    int cantidadParticipantes = 0;

    AudioJuego* audio = nullptr;

    ParticulaTierra particulas[MAX_PARTICULAS_TIERRA];

    BloquePrueba bloquesPrincipal[MAX_BLOQUES_PRUEBA];
    int cantidadBloquesPrincipal = 0;

    Camera3D camaraPrincipal{};

    ContextoMinijuego contextoMinijuego;
    GestorMinijuegos gestorMinijuegos;

    PruebaModelos pruebaModelos;
    PrototipoTablero prototipoTablero;

    bool modoCatalogo = false;

    // Ronda oficial lanzada desde el tablero: no se puede reiniciar (R),
    // abandonar (ESC) ni abrir el debug (F3). El minijuego debe terminar.
    bool modoTablero = false;
    bool mostrarDebug = false;
    bool volverAlMenu = false;

    void Inicializar(
        Participante participantesJuego[],
        int cantidadParticipantesJuego,
        AudioJuego* audioJuego = nullptr
    );

    void CambiarModo(
        ModoZonaPruebas nuevoModo
    );

    void CambiarMinijuego(
        IdMinijuego nuevoMinijuego
    );

    void Actualizar(
        float deltaTime
    );

    void Dibujar() const;

    const ResultadoMinijuego* ObtenerResultadoMinijuego() const;

    void Descargar();
};
