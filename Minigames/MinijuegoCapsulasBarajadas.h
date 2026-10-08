#pragma once

#include "Core/Participante.h"
#include "Core/ResultadoMinijuego.h"
#include "Minigames/TiposMinijuegos.h"
#include "Systems/Audio.h"


//==================================================
// CAPSULAS BARAJADAS
//==================================================
//
// Todos contra todos en un laboratorio. Un brazo robotico mete un nucleo
// brillante en una capsula, las cierra y las baraja por pares. Despues cada
// jugador mueve su marcador por las capsulas y confirma. Acierto = 1 punto;
// apuesta doble: confirmar en el primer segundo vale 2. Gana quien tenga mas
// puntos tras 5 rondas; el desempate es el menor tiempo total de aciertos.
//==================================================

inline constexpr int RONDAS_CAPSULAS = 5;
inline constexpr int MAX_CAPSULAS = 5;


enum FaseCapsulas
{
    FASE_CAPSULAS_PREPARACION = 0,
    FASE_CAPSULAS_JUGANDO,
    FASE_CAPSULAS_TERMINADO
};


enum SubfaseCapsulas
{
    SUBFASE_CAPSULAS_MOSTRAR = 0,
    SUBFASE_CAPSULAS_BARAJAR,
    SUBFASE_CAPSULAS_ELEGIR,
    SUBFASE_CAPSULAS_REVELAR
};


struct EstadoJugadorCapsulas
{
    int puntos = 0;
    int aciertos = 0;
    float tiempoAciertos = 0.0f;

    int marcador = 0;
    bool confirmado = false;
    float tiempoConfirmacion = 0.0f;
    int slotElegido = -1;

    bool acerto = false;
    int puntosRonda = 0;

    bool izquierdaPrevia = false;
    bool derechaPrevia = false;

    // Estado interno de la IA.
    int botObjetivo = 0;
    float botConfirmar = 0.0f;
    float botPaso = 0.0f;
};


struct MinijuegoCapsulasBarajadas
{
    ResultadoMinijuego resultado{};
    AudioJuego* audio = nullptr;   // lo asigna GestorMinijuegos; puede ser nullptr

    EstadoJugadorCapsulas estados[MAX_PARTICIPANTES];
    Color coloresJugadores[MAX_PARTICIPANTES];
    bool participa[MAX_PARTICIPANTES]{};

    // slotDe[c]: posicion actual (slot) de la capsula c.
    int slotDe[MAX_CAPSULAS]{};

    Camera3D camara{};
    FaseCapsulas fase = FASE_CAPSULAS_PREPARACION;
    SubfaseCapsulas subfase = SUBFASE_CAPSULAS_MOSTRAR;

    int ronda = 0;
    int cantidadCapsulas = 3;
    int premio = 0;

    int swapsRestantes = 0;
    int swapA = -1;
    int swapB = -1;
    float swapTiempo = 0.0f;
    float pausaBarajado = 0.0f;
    int ultimoParSlotA = -1;
    int ultimoParSlotB = -1;

    float tiempoPreparacion = 0.0f;
    float tiempoSubfase = 0.0f;
    float tiempoAnimacion = 0.0f;
    float revelado = 0.0f;

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
