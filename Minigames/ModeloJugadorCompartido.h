#pragma once

#include "Core/Participante.h"
#include "Core/RecursosJuego.h"
#include "Minigames/TiposMinijuegos.h"

#include "raylib.h"


// Ownership unico del GLB del jugador y de sus animaciones.
// Los consumidores solo obtienen vistas no propietarias o usan
// las funciones de dibujo compartidas.
void InicializarModeloJugadorCompartido();

bool ModeloJugadorCompartidoCargado();

const Model* ObtenerModeloJugadorCompartidoRender();

int ObtenerCantidadAnimacionesModeloJugadorCompartido();

bool AnimacionIdleModeloJugadorCompartidoActiva();

const char* ObtenerNombreIdleModeloJugadorCompartido();

int ObtenerCantidadFotogramasIdleModeloJugadorCompartido();

void AplicarFotogramaIdleModeloJugadorCompartido(
    int fotograma
);

void ActualizarAnimacionModeloJugadorCompartido();

float ObtenerAnguloModeloJugadorCompartido(
    const JugadorPrueba& jugador
);

void DibujarSombraModeloJugadorCompartido(
    const JugadorPrueba& jugador
);

void DibujarJugadorModeloCompartido(
    const JugadorPrueba& jugador,
    const Participante& participante
);

void DibujarModeloJugadorEnPosicion(
    Vector3 posicionPies,
    float anguloY,
    Color color,
    float escala = ESCALA_MODELO_JUGADOR_3D
);

void DescargarModeloJugadorCompartido();
