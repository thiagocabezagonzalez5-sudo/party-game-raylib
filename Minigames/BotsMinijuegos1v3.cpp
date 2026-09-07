#include "Minigames/BotsMinijuegos1v3.h"

#include <cmath>


void ReiniciarBotRefugioPinchos(
    EstadoBotRefugioPinchos& bot
)
{
    bot = {};
}


InputMinijuegoParticipante CrearEntradaBotHaciaObjetivo1v3(
    Vector3 posicionActual,
    Vector3 objetivo,
    float tolerancia
)
{
    InputMinijuegoParticipante entrada{};

    float dx = objetivo.x - posicionActual.x;
    float dz = objetivo.z - posicionActual.z;

    if (dx < -tolerancia)
    {
        entrada.izquierda = true;
    }
    else if (dx > tolerancia)
    {
        entrada.derecha = true;
    }

    if (dz < -tolerancia)
    {
        entrada.adelante = true;
    }
    else if (dz > tolerancia)
    {
        entrada.atras = true;
    }

    return entrada;
}


int ElegirDireccionAtaqueBotPinchos()
{
    return GetRandomValue(0, 3);
}


void ReiniciarBotMiradas(
    EstadoBotMiradas& bot,
    float demoraMinima,
    float demoraMaxima
)
{
    bot = {};

    if (demoraMaxima < demoraMinima)
    {
        float temporal = demoraMinima;
        demoraMinima = demoraMaxima;
        demoraMaxima = temporal;
    }

    int minimoMs = (int)std::lround(demoraMinima * 1000.0f);
    int maximoMs = (int)std::lround(demoraMaxima * 1000.0f);

    if (minimoMs < 0) minimoMs = 0;
    if (maximoMs < minimoMs) maximoMs = minimoMs;

    bot.tiempoDecision =
        (float)GetRandomValue(minimoMs, maximoMs) /
        1000.0f;
}


bool ActualizarDecisionBotMiradas(
    EstadoBotMiradas& bot,
    float deltaTime,
    int cantidadDirecciones
)
{
    if (bot.decisionTomada)
    {
        return false;
    }

    bot.tiempoDecision -= deltaTime;

    if (bot.tiempoDecision > 0.0f)
    {
        return false;
    }

    int limite = cantidadDirecciones > 0
        ? cantidadDirecciones - 1
        : 0;

    bot.direccionElegida = GetRandomValue(0, limite);
    bot.decisionTomada = true;
    bot.tiempoDecision = 0.0f;

    return true;
}
