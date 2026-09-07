#pragma once

#include "Core/Participante.h"
#include "Core/ResultadoMinijuego.h"
#include "Minigames/BotsMinijuegos1v3.h"


enum DireccionMiradaCruzada
{
    MIRADA_FRENTE = 0,
    MIRADA_IZQUIERDA,
    MIRADA_DERECHA,
    MIRADA_ARRIBA,
    MIRADA_ABAJO,
    CANTIDAD_DIRECCIONES_MIRADA
};


enum FaseMiradasCruzadas
{
    FASE_MIRADAS_PREPARACION = 0,
    FASE_MIRADAS_ELECCION,
    FASE_MIRADAS_RESOLUCION,
    FASE_MIRADAS_TERMINADO
};


struct EstadoJugadorMiradasCruzadas
{
    bool eliminado = false;
    bool eligioDireccion = false;
    DireccionMiradaCruzada direccion = MIRADA_FRENTE;
    int posicionFinal = 0;
};


struct MinijuegoMiradasCruzadas
{
    ResultadoMinijuego resultado{};

    EstadoJugadorMiradasCruzadas jugadores[MAX_PARTICIPANTES];
    EstadoBotMiradas bots[MAX_PARTICIPANTES];

    FaseMiradasCruzadas fase = FASE_MIRADAS_PREPARACION;

    int indiceSolo = -1;
    int numeroRonda = 0;

    float tiempoPreparacion = 2.5f;
    float tiempoFase = 0.0f;

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
