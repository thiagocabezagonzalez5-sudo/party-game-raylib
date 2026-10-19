#pragma once

#include "Core/CatalogoMinijuegos.h"
#include "Core/ResultadoMinijuego.h"
#include "Systems/Input.h"

#include "raylib.h"


struct PartidaTablero;


//==================================================
// DUELO DEL TABLERO
//==================================================
//
// La casilla DUELO abre un evento interactivo (EVENTO_INTERACTIVO_DUELO,
// ver EventosCasillas.h) donde quien cayo ELIGE a un rival y ELIGE la
// apuesta. Despues PartidaTablero pasa a FASE_..._DUELO_PENDIENTE: Juego
// sortea uno de los minijuegos esDuelo (Core/CatalogoMinijuegos), activa
// solo a los dos duelistas y devuelve el resultado con la misma pareja
// AplicarResultadoMinijuego / ContinuarTrasMinijuego de la ronda normal
// (PartidaTablero sabe que es un duelo y no entrega los premios de ronda).
//
// Reglas decididas:
//  - Apuesta: 5, 10 o 20 monedas limitadas por lo que tenga el mas pobre de
//    los dos, o 1 trofeo si ambos tienen al menos uno. Valores repetidos
//    tras el tope se muestran una sola vez.
//  - El ganador se lleva la apuesta del perdedor. Empate o ronda cancelada
//    o fallida: nadie paga.
//  - El ESCUDO NO protege: el duelo es una apuesta voluntaria.
//  - Sin rival valido (nadie puede apostar nada) o si el humano cancela: el
//    duelo se cancela, nadie pierde y el turno sigue.
//  - Bot: desafia al lider (mas trofeos, luego monedas); si el lider es el
//    mismo, al mas parejo en monedas. Apuesta el 40 % de sus monedas (o 1
//    trofeo si va por detras en trofeos y todavia no alcanza para uno).

inline constexpr int MAX_OPCIONES_APUESTA_DUELO = 4;

enum TipoApuestaDuelo
{
    APUESTA_DUELO_MONEDAS = 0,
    APUESTA_DUELO_TROFEO
};


struct OpcionApuestaDuelo
{
    TipoApuestaDuelo tipo = APUESTA_DUELO_MONEDAS;
    int cantidad = 0;

    // true si el tope del mas pobre recorto la cantidad pedida.
    bool limitada = false;
};


// Pasos de la seleccion.
enum PasoDuelo
{
    PASO_DUELO_RIVAL = 0,
    PASO_DUELO_APUESTA,
    PASO_DUELO_ANUNCIO,
    PASO_DUELO_CANCELADO
};


struct EstadoDueloSeleccion
{
    int retador = -1;
    int paso = PASO_DUELO_RIVAL;
    float inicioPaso = 0.0f;

    int rivales[3] = { -1, -1, -1 };
    int cantidadRivales = 0;
    int rivalSeleccion = 0;

    OpcionApuestaDuelo opciones[MAX_OPCIONES_APUESTA_DUELO];
    int cantidadOpciones = 0;
    int opcionSeleccion = 0;

    float proximaAccionBot = 1.0f;
    float tiempoSinAccion = 0.0f;
    float reloj = 0.0f;

    bool aceptado = false;
    char resumen[112] = {};
};


// Duelo aceptado (vive en PartidaTablero mientras se juega y se paga).
struct DueloTablero
{
    bool activo = false;
    int retador = -1;
    int rival = -1;
    TipoApuestaDuelo apuesta = APUESTA_DUELO_MONEDAS;
    int cantidad = 0;
    IdMinijuego minijuego = MINIJUEGO_COLOR_SEGURO;

    // Resultado.
    int ganador = -1;
    int perdedor = -1;
    int pagado = 0;
    bool empate = false;
    bool cancelado = false;
    float tiempo = 0.0f;
    char resumen[112] = {};
};


//--------------------------------------------------
// REGLAS
//--------------------------------------------------

// Opciones de apuesta entre a y b. Devuelve cuantas hay (0 = no pueden
// apostar nada).
int ListarOpcionesApuestaDuelo(
    const PartidaTablero& partida,
    int a,
    int b,
    OpcionApuestaDuelo opciones[]
);

// Rivales con los que se puede apostar algo.
int ListarRivalesDuelo(
    const PartidaTablero& partida,
    int retador,
    int rivales[]
);

int DecidirRivalDueloBot(
    const PartidaTablero& partida,
    int retador,
    const int rivales[],
    int cantidad
);

int DecidirApuestaDueloBot(
    const PartidaTablero& partida,
    int retador,
    int rival,
    const OpcionApuestaDuelo opciones[],
    int cantidad
);

// Deja un duelo aceptado y pasa a FASE_..._DUELO_PENDIENTE.
void IniciarDueloTablero(
    PartidaTablero& partida,
    int retador,
    int rival,
    OpcionApuestaDuelo apuesta
);

// Paga la apuesta segun el resultado (nullptr = cancelado: no se paga) y
// pasa a FASE_..._DUELO_RESULTADO. Nadie queda en negativo.
void ResolverDueloTablero(
    PartidaTablero& partida,
    const ResultadoMinijuego* resultado
);


//--------------------------------------------------
// SELECCION (la llama EventosCasillas)
//--------------------------------------------------

void IniciarSeleccionDuelo(
    PartidaTablero& partida,
    int retador,
    EstadoDueloSeleccion& duelo
);

// true cuando la seleccion termino (aceptada o cancelada).
bool ActualizarSeleccionDuelo(
    PartidaTablero& partida,
    EstadoDueloSeleccion& duelo,
    float deltaTime,
    bool esAutomatico,
    bool puedeDecidir,
    const InputSeleccionParticipante& entrada
);

void DibujarSeleccionDuelo2D(
    const PartidaTablero& partida,
    const EstadoDueloSeleccion& duelo,
    float tiempoTotal
);

void TextoSeleccionDuelo(
    const PartidaTablero& partida,
    const EstadoDueloSeleccion& duelo,
    char* destino,
    int capacidad
);


//--------------------------------------------------
// FASES DE PARTIDA
//--------------------------------------------------

void ActualizarDueloResultado(
    PartidaTablero& partida,
    float deltaTime,
    bool esAutomatico,
    bool puedeDecidir,
    const InputSeleccionParticipante& entrada
);

void DibujarDueloResultado2D(const PartidaTablero& partida);

void TextoDuelo(
    const PartidaTablero& partida,
    char* destino,
    int capacidad
);

// Texto del duelo para un jugador: "J1 vs J3 por 10 monedas".
void DescribirApuestaDuelo(
    const DueloTablero& duelo,
    char* destino,
    int capacidad
);
