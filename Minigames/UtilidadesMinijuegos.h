#pragma once

#include "Core/Participante.h"
#include "Minigames/TiposMinijuegos.h"
#include "Systems/Input.h"


void ReiniciarJugadorPrueba(
    JugadorPrueba& jugador
);


void AgregarBloquePrueba(
    BloquePrueba bloques[],
    int& cantidadBloques,
    int cantidadMaxima,
    Vector3 posicion,
    Vector3 tamano,
    Color color
);


void ReiniciarBloquesPrueba(
    BloquePrueba bloques[],
    int cantidadBloques
);


BoundingBox CrearHitboxBloquePrueba(
    const BloquePrueba& bloque
);


BoundingBox CrearHitboxJugadorPrueba(
    const JugadorPrueba& jugador
);


void ActualizarJugadorPrueba(
    JugadorPrueba& jugador,
    const InputMinijuegoParticipante& entrada,
    BloquePrueba bloques[],
    int cantidadBloques,
    ParticulaTierra particulas[],
    int cantidadParticulas,
    bool permitirSalto,
    bool usarEmpuje,
    bool respawnAutomatico,
    float deltaTime
);


void ResolverColisionesJugadoresNormales(
    JugadorPrueba jugadores[],
    const Participante participantes[],
    int cantidadMaxima
);


void ResolverColisionesPelotas(
    JugadorPrueba jugadores[],
    const Participante participantes[],
    int cantidadMaxima
);


bool ResolverGolpesSuelo(
    JugadorPrueba jugadores[],
    const Participante participantes[],
    int cantidadMaxima
);


void ResolverGolpesJugadores(
    JugadorPrueba jugadores[],
    const Participante participantes[],
    int cantidadMaxima
);


void ActualizarParticulasTierra(
    ParticulaTierra particulas[],
    int cantidadMaxima,
    float deltaTime
);


void DibujarParticulasTierra(
    const ParticulaTierra particulas[],
    int cantidadMaxima
);


void DibujarJugadorCuboPrueba(
    const JugadorPrueba& jugador,
    const Participante& participante
);


void DibujarJugadorPelotaPrueba(
    const JugadorPrueba& jugador,
    const Participante& participante
);
