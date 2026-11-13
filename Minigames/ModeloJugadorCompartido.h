#pragma once

#include "Core/Participante.h"
#include "Core/RecursosJuego.h"
#include "Entities/PersonajesBloque.h"
#include "Minigames/TiposMinijuegos.h"

#include "raylib.h"


// Dibujo compartido del jugador: los cuatro personajes de bloques
// (Entities/PersonajesBloque). Son procedurales, no hay recursos que cargar
// ni descargar. Este modulo traduce el estado de juego (JugadorPrueba,
// Participante) a la pose y al personaje elegido.


// Personaje elegido en la seleccion; si no hay (Zona de Pruebas, partidas
// armadas a mano) cada puesto usa uno distinto segun su numero.
int ObtenerPersonajeParticipante(
    const Participante& participante
);

float ObtenerAnguloModeloJugadorCompartido(
    const JugadorPrueba& jugador
);

// Pose de animacion a partir del estado fisico y de combate del jugador.
PosePersonajeBloque CrearPoseJugadorPrueba(
    const JugadorPrueba& jugador
);

// Pose de reposo animada (respiracion, parpadeo), para dibujos sin estado.
PosePersonajeBloque CrearPoseReposoPersonaje();

void DibujarSombraModeloJugadorCompartido(
    const JugadorPrueba& jugador
);

void DibujarJugadorModeloCompartido(
    const JugadorPrueba& jugador,
    const Participante& participante
);

// Dibuja un personaje en una posicion libre. "personaje" es el indice
// 0..3 (ver ObtenerPersonajeParticipante).
void DibujarModeloJugadorEnPosicion(
    int personaje,
    Vector3 posicionPies,
    float anguloY,
    Color color,
    float escala = ESCALA_MODELO_JUGADOR_3D
);

void DibujarModeloJugadorEnPosicion(
    int personaje,
    Vector3 posicionPies,
    float anguloY,
    Color color,
    float escala,
    const PosePersonajeBloque& pose
);
