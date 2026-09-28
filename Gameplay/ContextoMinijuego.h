#pragma once

#include "Core/Participante.h"
#include "Minigames/TiposMinijuegos.h"
#include "Systems/Audio.h"


// Vista no propietaria de los datos compartidos que ya utiliza ZonaPruebas.
// El gestor no crea ni destruye estos arreglos ni el sistema de audio.
struct ContextoMinijuego
{
    JugadorPrueba* jugadores = nullptr;
    int cantidadJugadores = 0;

    Participante* participantes = nullptr;

    ParticulaTierra* particulas = nullptr;
    int cantidadParticulas = 0;

    AudioJuego* audio = nullptr;
};
