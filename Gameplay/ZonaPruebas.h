#pragma once

#include "Core/Participante.h"
#include "Minigames/MinijuegoColorSeguro.h"
#include "Minigames/Minijuego67.h"
#include "Minigames/MinijuegoPelotas.h"
#include "Minigames/MinijuegoTronco.h"
#include "Minigames/PruebaModelos.h"
#include "Minigames/TiposMinijuegos.h"


enum ModoZonaPruebas
{
    PRUEBA_ZONA_PRINCIPAL = 1,
    PRUEBA_COLOR_SEGURO = 2,
    PRUEBA_PELOTAS_EMPUJON = 3,
    PRUEBA_MODELOS = 4,
    PRUEBA_TRONCO_COORDINADO = 5,
    PRUEBA_FABRICA_67 = 6
};


struct ZonaPruebas
{
    ModoZonaPruebas modoActual =
        PRUEBA_ZONA_PRINCIPAL;

    JugadorPrueba jugadores[MAX_JUGADORES_PRUEBA];

    Participante* participantes = nullptr;
    int cantidadParticipantes = 0;

    ParticulaTierra particulas[MAX_PARTICULAS_TIERRA];

    BloquePrueba bloquesPrincipal[MAX_BLOQUES_PRUEBA];
    int cantidadBloquesPrincipal = 0;

    Camera3D camaraPrincipal{};

    MinijuegoColorSeguro minijuegoColor;
    MinijuegoPelotas minijuegoPelotas;
    PruebaModelos pruebaModelos;
    MinijuegoTronco minijuegoTronco;
    Minijuego67 minijuego67;

    bool mostrarDebug = false;
    bool volverAlMenu = false;

    void Inicializar(
        Participante participantesJuego[],
        int cantidadParticipantesJuego
    );

    void CambiarModo(
        ModoZonaPruebas nuevoModo
    );

    void Actualizar(
        float deltaTime
    );

    void Dibujar() const;

    void ReiniciarJugador(
        int indice
    );

    void ReiniciarJugadores();

    void Descargar();
};
