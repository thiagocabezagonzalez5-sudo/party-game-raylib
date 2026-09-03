#pragma once

#include "Core/Participante.h"


enum FormatoMinijuego
{
    FORMATO_MINIJUEGO_INDIVIDUAL = 0,
    FORMATO_MINIJUEGO_EQUIPOS,
    FORMATO_MINIJUEGO_COOPERATIVO
};


enum EstadoResultadoMinijuego
{
    RESULTADO_MINIJUEGO_EN_CURSO = 0,
    RESULTADO_MINIJUEGO_FINALIZADO,
    RESULTADO_MINIJUEGO_CANCELADO
};


enum DesenlaceMinijuego
{
    DESENLACE_SIN_DEFINIR = 0,
    DESENLACE_CON_GANADOR,
    DESENLACE_EMPATE,
    DESENLACE_VICTORIA_COOPERATIVA,
    DESENLACE_DERROTA_COOPERATIVA,
    DESENLACE_COMPLETADO
};


struct ResultadoParticipante
{
    bool participo = false;

    // Uno representa el primer puesto. Cero indica que
    // el minijuego no asigno una posicion final.
    int posicionFinal = 0;

    // Solo se utiliza para equipos rivales.
    int numeroEquipo = -1;

    // Desempeno producido dentro del minijuego.
    int puntuacionMinijuego = 0;

    // La recompensa la asignara Partida o una regla general.
    int puntosObtenidos = 0;
};


struct ResultadoMinijuego
{
    EstadoResultadoMinijuego estado =
        RESULTADO_MINIJUEGO_EN_CURSO;

    FormatoMinijuego formato =
        FORMATO_MINIJUEGO_INDIVIDUAL;

    DesenlaceMinijuego desenlace =
        DESENLACE_SIN_DEFINIR;

    ResultadoParticipante participantes[
        MAX_PARTICIPANTES
    ];

    // Es una cantidad, no un limite para recorrer el arreglo.
    int cantidadParticipantes = 0;

    // Debe ser cero salvo en FORMATO_MINIJUEGO_EQUIPOS.
    int cantidadEquipos = 0;
};


void InicializarResultadoMinijuego(
    ResultadoMinijuego& resultado,
    const Participante participantes[],
    FormatoMinijuego formato
);


bool ResultadoMinijuegoFinalizado(
    const ResultadoMinijuego& resultado
);


bool ParticipanteEsGanador(
    const ResultadoMinijuego& resultado,
    int indiceParticipante
);


int ObtenerIndicesGanadores(
    const ResultadoMinijuego& resultado,
    int indicesGanadores[],
    int capacidad
);


bool ValidarResultadoMinijuego(
    const ResultadoMinijuego& resultado,
    const Participante participantes[]
);
