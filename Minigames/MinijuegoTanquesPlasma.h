#pragma once

#include "Core/Participante.h"
#include "Core/ResultadoMinijuego.h"
#include "raylib.h"


inline constexpr int MAX_PROYECTILES_TANQUES_PLASMA = 40;


enum FaseTanquesPlasma
{
    FASE_TANQUES_PREPARACION = 0,
    FASE_TANQUES_COMBATE,
    FASE_TANQUES_TERMINADO
};


struct EstadoTanquePlasma
{
    Vector2 posicion{};
    Vector2 direccion = { 0.0f, -1.0f };

    int vidas = 2;
    int impactosAcertados = 0;
    bool eliminado = false;

    float recarga = 0.0f;
    float invulnerabilidad = 0.0f;
    float faseBot = 0.0f;
};


struct ProyectilTanquePlasma
{
    bool activo = false;
    int propietario = -1;
    Vector2 posicion{};
    Vector2 velocidad{};
    float tiempoVida = 0.0f;
};


struct MinijuegoTanquesPlasma
{
    ResultadoMinijuego resultado{};
    EstadoTanquePlasma tanques[MAX_PARTICIPANTES];
    ProyectilTanquePlasma proyectiles[MAX_PROYECTILES_TANQUES_PLASMA];

    FaseTanquesPlasma fase = FASE_TANQUES_PREPARACION;

    float tiempoPreparacion = 0.0f;
    float tiempoCombate = 0.0f;
    float tiempoAnimacion = 0.0f;

    void Inicializar();

    void Reiniciar(
        Participante participantes[]
    );

    void Actualizar(
        float deltaTime,
        Participante participantes[]
    );

    void Dibujar(
        const Participante participantes[]
    ) const;

    const ResultadoMinijuego& ObtenerResultado() const;
};
