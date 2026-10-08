#pragma once

#include "Board/CatalogoTableros.h"
#include "Board/Tablero.h"
#include "Core/Participante.h"
#include "Core/ResultadoMinijuego.h"
#include "Systems/Audio.h"

#include "raylib.h"


enum FasePartidaTablero
{
    FASE_PARTIDA_TABLERO_ESPERANDO_DADO = 0,
    FASE_PARTIDA_TABLERO_MOSTRANDO_DADO,
    FASE_PARTIDA_TABLERO_ELIGIENDO_RUTA,
    FASE_PARTIDA_TABLERO_MOVIENDO,
    FASE_PARTIDA_TABLERO_DECISION_TROFEO,
    FASE_PARTIDA_TABLERO_EVENTO_CASILLA,
    FASE_PARTIDA_TABLERO_FIN_TURNO,
    FASE_PARTIDA_TABLERO_FIN_RONDA,
    FASE_PARTIDA_TABLERO_TERMINADA
};


enum EventoEspecialTablero
{
    EVENTO_TABLERO_NINGUNO = 0,
    EVENTO_TABLERO_BONIFICACION,
    EVENTO_TABLERO_MULTA,
    EVENTO_TABLERO_INTERCAMBIO
};


struct EstadoJugadorPartidaTablero
{
    bool participa = false;

    int casillaActual = 0;
    int monedas = 10;
    int trofeos = 0;

    int premioUltimoMinijuego = 0;

    Vector3 posicionVisual{};
};


struct PartidaTablero
{
    Tablero tablero;

    Participante* participantes = nullptr;
    AudioJuego* audio = nullptr;

    EstadoJugadorPartidaTablero jugadores[MAX_PARTICIPANTES];

    int ordenParticipantes[MAX_PARTICIPANTES] =
    {
        -1,
        -1,
        -1,
        -1
    };

    int cantidadJugadores = 0;
    int indiceOrdenTurno = 0;

    int rondaActual = 1;
    int cantidadRondas = 5;

    int valorDado = 0;
    int pasosPendientes = 0;

    int opcionRuta = 0;
    bool direccionRutaBloqueada = false;

    int casillaDestinoMovimiento = -1;

    Vector3 posicionInicioMovimiento{};
    Vector3 posicionFinMovimiento{};

    float progresoMovimiento = 0.0f;
    float tiempoFase = 0.0f;

    int casillaTrofeo = -1;
    int costoTrofeo = 20;

    bool compraTrofeoDisponible = false;
    bool compraTrofeoResuelta = false;

    EventoEspecialTablero ultimoEventoEspecial =
        EVENTO_TABLERO_NINGUNO;

    int variacionMonedasEvento = 0;
    int jugadorIntercambioEvento = -1;

    bool minijuegoSolicitado = false;
    bool salidaSolicitada = false;
    bool inicializado = false;

    FasePartidaTablero fase =
        FASE_PARTIDA_TABLERO_ESPERANDO_DADO;

    Camera3D camara{};

    // Estado visual de la camara tipo diorama (suavizado hacia el foco).
    Vector3 camaraFoco{};
    float camaraZoom = 1.0f;
    int camaraRondaVista = 0;
    float camaraTiempoVista = 0.0f;
    int camaraTrofeoVisto = -1;
    float camaraTiempoTrofeo = 0.0f;

    // --- Tablero elegido y estado de su gimmick ---

    IdTablero idTablero = TABLERO_ISLA_ARBOLEDA;
    const DefinicionTablero* definicion = nullptr;

    float tiempoTablero = 0.0f;

    // Estado generico del gimmick: gimmickActivo es el objetivo
    // logico y gimmickAnim (0..1) su animacion suavizada.
    bool gimmickActivo = false;
    float gimmickAnim = 0.0f;
    int casillaPeligro = -1;

    // Texto del evento de la casilla actual (vacio = texto comun).
    char textoEvento[96] = {};

    // Estado del gimmick para el HUD y aviso temporal de ronda.
    char estadoGimmick[96] = {};
    char mensajeGimmick[96] = {};
    float tiempoMensajeGimmick = 0.0f;

    void Inicializar(
        Participante participantesJuego[],
        int cantidadParticipantesJuego,
        AudioJuego* audioJuego,
        IdTablero id = TABLERO_ISLA_ARBOLEDA
    );

    void Reiniciar();

    void Actualizar(
        float deltaTime
    );

    void Dibujar() const;

    bool SolicitaMinijuego() const;

    void AplicarResultadoMinijuego(
        const ResultadoMinijuego& resultado
    );

    void ContinuarTrasMinijuego();

    bool SolicitaSalida() const;
};


//==================================================
// UTILIDADES PARA LOS TABLEROS (hooks de gimmick)
//==================================================

// Participante (0..3) que tiene el turno, o -1.
int ObtenerParticipanteTurnoTableroFinal(
    const PartidaTablero& partida
);

void ReproducirSonidoTablero(
    PartidaTablero& partida,
    TipoSonidoJuego tipo
);

// Mueve la ficha al instante a otra casilla (teletransporte).
void TeletransportarJugadorTablero(
    PartidaTablero& partida,
    int participante,
    int casilla
);

// Muestra un aviso de gimmick durante unos segundos.
void EstablecerMensajeGimmick(
    PartidaTablero& partida,
    const char* texto
);

// Escena 3D completa (decoracion, ruta, gimmick, trofeo, fichas).
// Debe llamarse dentro de BeginMode3D. Sirve tambien a la vista previa.
void DibujarEscenaTablero3D(
    const PartidaTablero& partida
);
