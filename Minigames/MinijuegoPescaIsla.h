#pragma once

#include "Core/Participante.h"
#include "Core/ResultadoMinijuego.h"
#include "Minigames/TiposMinijuegos.h"
#include "Systems/Audio.h"


//==================================================
// PESCA ISLENA
//==================================================
//
// Todos contra todos en una laguna tropical. Cada jugador pesca desde su
// muelle: apunta, lanza, espera el pique ("!") y engancha en el momento
// justo; despues un tira y afloja con la tension de la linea. Si varios
// anzuelos rodean al mismo pez, solo el primero en engancharlo se lo queda y
// los demas pierden la carnada. Las botas viejas bloquean el lanzamiento.
//==================================================

inline constexpr int MAX_PECES_PESCA = 14;


enum FasePesca
{
    FASE_PESCA_PREPARACION = 0,
    FASE_PESCA_JUGANDO,
    FASE_PESCA_TERMINADO
};


enum TipoPezPesca
{
    PEZ_PESCA_PEQUENO = 0,
    PEZ_PESCA_MEDIANO,
    PEZ_PESCA_RARO,
    PEZ_PESCA_BOTA
};


enum EstadoPezPesca
{
    ESTADO_PEZ_NADANDO = 0,
    ESTADO_PEZ_MORDIENDO,
    ESTADO_PEZ_ENGANCHADO,
    ESTADO_PEZ_AUSENTE
};


enum EstadoPescaJugador
{
    PESCA_LIBRE = 0,
    PESCA_LANZANDO,
    PESCA_ESPERANDO,
    PESCA_PICADA,
    PESCA_TENSION
};


struct PezPesca
{
    EstadoPezPesca estado = ESTADO_PEZ_AUSENTE;
    TipoPezPesca tipo = PEZ_PESCA_PEQUENO;

    float x = 0.0f;
    float z = 0.0f;
    float dirX = 1.0f;
    float dirZ = 0.0f;
    float velocidad = 1.5f;
    float tiempoCambio = 0.0f;
    float temporizadorMorder = -1.0f;
    float ventana = 0.0f;
    float ignorar = 0.0f;
    float ausente = 0.0f;
    int jugador = -1;
    float semilla = 0.0f;
};


struct EstadoJugadorPesca
{
    EstadoPescaJugador estado = PESCA_LIBRE;

    float cursorX = 0.0f;
    float cursorZ = 0.0f;
    float anzueloX = 0.0f;
    float anzueloZ = 0.0f;
    float tiempoEstado = 0.0f;
    float bloqueo = 0.0f;

    int pez = -1;
    float tension = 0.0f;
    float progreso = 0.0f;
    float tiempoPelea = 0.0f;
    bool recogiendo = false;

    int puntos = 0;
    int peces = 0;
    int mejorValor = 0;

    int popupValor = -1;
    float popupTiempo = 0.0f;

    // Estado interno de la IA.
    float botReevaluar = 0.0f;
    float botObjetivoX = 0.0f;
    float botObjetivoZ = 0.0f;
    bool botTieneObjetivo = false;
    float botReaccion = 0.0f;
    bool botFalla = false;
    float botEspera = 0.0f;
};


struct MinijuegoPescaIsla
{
    ResultadoMinijuego resultado{};
    AudioJuego* audio = nullptr;   // lo asigna GestorMinijuegos; puede ser nullptr

    EstadoJugadorPesca estados[MAX_PARTICIPANTES];
    PezPesca peces[MAX_PECES_PESCA];
    Color coloresJugadores[MAX_PARTICIPANTES];
    bool participa[MAX_PARTICIPANTES]{};
    int muelleDe[MAX_PARTICIPANTES]{};

    Camera3D camara{};
    FasePesca fase = FASE_PESCA_PREPARACION;

    float tiempoPreparacion = 0.0f;
    float tiempoRestante = 0.0f;
    float tiempoAnimacion = 0.0f;

    void Inicializar();

    void Reiniciar(
        JugadorPrueba jugadores[],
        Participante participantes[],
        int cantidadMaxima
    );

    void Actualizar(
        float deltaTime,
        JugadorPrueba jugadores[],
        int cantidadMaxima,
        Participante participantes[]
    );

    void Dibujar(
        const JugadorPrueba jugadores[],
        int cantidadMaxima,
        const Participante participantes[],
        bool mostrarDebug
    ) const;

    const ResultadoMinijuego& ObtenerResultado() const;
};
