#pragma once

#include "Core/ConfiguracionJuego.h"
#include "Minigames/TiposMinijuegos.h"


int CantidadJugadoresTeclado(
    ModoTeclado modoTeclado
);


int ObtenerIndiceGamepadParaJugador(
    int indiceJugador,
    ModoTeclado modoTeclado
);


EntradaJugadorPrueba LeerEntradaJugadorPrueba(
    int indiceJugador,
    const JugadorPrueba& jugador,
    ModoTeclado modoTeclado
);


void ActualizarJugadoresConectadosPrueba(
    JugadorPrueba jugadores[],
    int cantidadMaxima,
    int& cantidadActivos,
    ModoTeclado modoTeclado
);


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
    const EntradaJugadorPrueba& entrada,
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
    int cantidadMaxima
);


void ResolverColisionesPelotas(
    JugadorPrueba jugadores[],
    int cantidadMaxima
);


bool ResolverGolpesSuelo(
    JugadorPrueba jugadores[],
    int cantidadMaxima
);


void ResolverGolpesJugadores(
    JugadorPrueba jugadores[],
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
    const JugadorPrueba& jugador
);


void DibujarJugadorPelotaPrueba(
    const JugadorPrueba& jugador
);
