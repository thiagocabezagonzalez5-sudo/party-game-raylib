#pragma once

#include "Minigames/TiposMinijuegos.h"
#include "Systems/Input.h"


// Estos estados y funciones solo se usan en los dos minijuegos 1v3
// que tienen bots activos. El resto del proyecto sigue dejando a los
// bots inmoviles hasta que se implemente una IA especifica para ellos.

struct EstadoBotRefugioPinchos
{
    Vector3 objetivo{};
    float tiempoRecalculo = 0.0f;
};


struct EstadoBotMiradas
{
    int direccionElegida = 0;
    float tiempoDecision = 0.0f;
    bool decisionTomada = false;
};


void ReiniciarBotRefugioPinchos(
    EstadoBotRefugioPinchos& bot
);


InputMinijuegoParticipante CrearEntradaBotHaciaObjetivo1v3(
    Vector3 posicionActual,
    Vector3 objetivo,
    float tolerancia = 0.20f
);


int ElegirDireccionAtaqueBotPinchos();


void ReiniciarBotMiradas(
    EstadoBotMiradas& bot,
    float demoraMinima,
    float demoraMaxima
);


bool ActualizarDecisionBotMiradas(
    EstadoBotMiradas& bot,
    float deltaTime,
    int cantidadDirecciones
);
