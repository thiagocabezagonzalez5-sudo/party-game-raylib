#pragma once

#include "raylib.h"


const int MAX_JUGADORES_PRUEBA = 4;
const int MAX_BLOQUES_PRUEBA = 8;
const int MAX_PARTICULAS_TIERRA = 160;


struct BloquePrueba
{
    Vector3 posicion{};
    Vector3 posicionInicial{};

    Vector3 tamano =
    {
        1.0f,
        1.0f,
        1.0f
    };

    Color color = GRAY;

    bool activaColision = true;
    bool cayendo = false;

    float velocidadCaida = 0.0f;
};


struct ParticulaTierra
{
    bool activa = false;

    Vector3 posicion{};
    Vector3 velocidad{};

    float vida = 0.0f;
    float vidaMaxima = 0.0f;
    float tamano = 0.08f;

    Color color =
    {
        125,
        84,
        48,
        255
    };
};


struct JugadorPrueba
{
    Vector3 posicion{};
    Vector3 posicionSpawn{};
    Vector3 velocidad{};
    Vector3 empuje{};

    Vector3 direccionMirada =
    {
        0.0f,
        0.0f,
        -1.0f
    };

    Vector3 tamano =
    {
        0.8f,
        1.4f,
        0.8f
    };

    float velocidadMovimiento = 5.0f;
    float fuerzaSalto = 7.2f;
    float gravedad = 18.0f;

    bool enSuelo = false;
    bool cayendo = false;

    float tiempoRespawn = 0.0f;
    float duracionRespawn = 1.2f;

    float cooldownChoque = 0.0f;

    bool golpeSueloActivo = false;
    bool impactoGolpeSuelo = false;

    bool aplastado = false;
    float tiempoAplastado = 0.0f;

    float tiempoInmunidad = 0.0f;

    bool golpeando = false;
    bool golpeYaConecto = false;
    float tiempoGolpe = 0.0f;
    float cooldownGolpe = 0.0f;
};
