#pragma once

#include "Core/Participante.h"
#include "Minigames/TiposMinijuegos.h"
#include "Systems/Input.h"

#ifndef UTILIDADES_MINIJUEGOS_IMPLEMENTACION
#include "Minigames/ModeloJugadorCompartido.h"
#endif


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


//==================================================
// RENDER DE JUGADOR EN MINIJUEGOS
//==================================================
//
// La implementacion historica de cubo/esfera se conserva como fallback en
// UtilidadesMinijuegos.cpp, pero todos los minijuegos nuevos y actuales usan
// el GLB compartido. La unidad que implementa este archivo se excluye del
// macro desde CMake para no renombrar sus propias funciones.
//==================================================

#ifndef UTILIDADES_MINIJUEGOS_IMPLEMENTACION
#define DibujarJugadorCuboPrueba DibujarJugadorModeloCompartido
#define DibujarJugadorPelotaPrueba DibujarJugadorModeloCompartido
#endif
