#pragma once

#include "Core/Participante.h"
#include "raylib.h"


//==================================================
// CONSTANTES
//==================================================

inline constexpr int MAX_JUGADORES_SELECCION =
    MAX_PARTICIPANTES;

inline constexpr int MAX_PERSONAJES_SELECCION =
    4;


//==================================================
// CONFIRMACION FINAL DE PARTIDA
//==================================================
//
// La seleccion marca a los jugadores como LISTOS al confirmar
// personaje. Eso no debe iniciar la partida automaticamente:
// con 2 jugadores listos todavia tiene que existir tiempo para
// que un tercero o cuarto se una.
//
// Esta solicitud exige una NUEVA pulsacion de confirmar despues
// de que todos los jugadores activos ya estaban listos.
// Soporta ESPACIO, ENTER y A de cualquier gamepad.
//==================================================

struct SolicitudInicioSeleccion
{
    bool valor = false;
    bool habilitada = false;

    double tiempoHabilitada = 0.0;
    double ultimoMomentoPermitido = 0.0;


    SolicitudInicioSeleccion& operator=(
        bool puedeIniciar
    )
    {
        double ahora = GetTime();

        if (!puedeIniciar)
        {
            valor = false;

            // Juego.cpp vuelve a validar los jugadores despues de
            // SeleccionPersonajes::Actualizar(). Para tres jugadores
            // puede existir una asignacion false seguida de una true
            // dentro del mismo frame. Este pequeno margen evita que
            // esa primera asignacion desarme la confirmacion final.
            if (
                habilitada &&
                ahora - ultimoMomentoPermitido > 0.20
            )
            {
                habilitada = false;
                tiempoHabilitada = 0.0;
            }

            return *this;
        }

        ultimoMomentoPermitido =
            ahora;

        if (!habilitada)
        {
            habilitada = true;
            tiempoHabilitada = ahora;
            valor = false;

            return *this;
        }

        // Impide que la misma pulsacion que hizo LISTO al ultimo
        // jugador tambien arranque la partida en ese mismo frame.
        if (
            ahora - tiempoHabilitada < 0.12
        )
        {
            valor = false;
            return *this;
        }

        bool confirmar =
            IsKeyPressed(KEY_SPACE) ||
            IsKeyPressed(KEY_ENTER);

        for (
            int gamepad = 0;
            gamepad < MAX_JUGADORES_SELECCION;
            gamepad++
        )
        {
            if (
                IsGamepadAvailable(gamepad) &&
                IsGamepadButtonPressed(
                    gamepad,
                    GAMEPAD_BUTTON_RIGHT_FACE_DOWN
                )
            )
            {
                confirmar = true;
            }
        }

        valor =
            confirmar;

        return *this;
    }


    operator bool() const
    {
        return valor;
    }
};


//==================================================
// PERSONAJE
//==================================================
//
// El roster son los 4 personajes reales del juego. Se muestran en 3D con
// el modelo compartido (Minigames/ModeloJugadorCompartido); aqui solo
// viven el nombre y el color de cada uno. No se cargan texturas propias.

struct PersonajeSeleccion
{
    const char* nombre =
        "";

    Color color =
        LIGHTGRAY;
};


//==================================================
// JUGADOR
//==================================================

struct JugadorSeleccion
{
    int cursorPersonaje =
        0;

    bool listo =
        false;

    // Bloqueos del stick (un empuje = un movimiento).
    bool bloqueoHorizontal =
        false;

    bool bloqueoVertical =
        false;
};


//==================================================
// SELECCION DE PERSONAJES
//==================================================
//
// Escena 3D (UI/SeleccionPersonajes3D) con un pedestal por personaje y
// un cursor de color por jugador humano. Los puestos sin unirse seran
// bots (los completa Juego al iniciar la partida).

struct SeleccionPersonajes
{
    PersonajeSeleccion personajes[
        MAX_PERSONAJES_SELECCION
    ];

    JugadorSeleccion jugadores[
        MAX_JUGADORES_SELECCION
    ];

    bool recursosCargados =
        false;


    //------------------------------
    // ESTADOS
    //------------------------------

    bool volverAlMenu =
        false;

    bool todosListos =
        false;

    SolicitudInicioSeleccion iniciarPartida;


    //------------------------------
    // TRANSICION
    //------------------------------

    float alphaEntrada =
        0.0f;

    const float DURACION_ENTRADA =
        0.30f;


    //------------------------------
    // ESTADO VISUAL (solo presentacion)
    //------------------------------

    float tiempoEscena =
        0.0f;

    // 0..1 suavizado: personaje apuntado por algun humano.
    float foco[MAX_PERSONAJES_SELECCION] = {};

    // Segundos desde que alguien confirmo ese personaje
    // (>= DURACION_REACCION = sin reaccion).
    float tiempoReaccion[MAX_PERSONAJES_SELECCION] = {};

    // Segundos que el puesto lleva sin humano (escalona los bots).
    float tiempoComoBot[MAX_JUGADORES_SELECCION] = {};

    // Aviso "CONTROL DESCONECTADO" restante, en segundos.
    float avisoDesconexion[MAX_JUGADORES_SELECCION] = {};

    // Segundos con todos los jugadores listos (fiesta de confeti).
    float tiempoTodosListos =
        0.0f;

    const float DURACION_REACCION =
        0.75f;


    //------------------------------
    // EVENTOS DE SONIDO DEL ULTIMO Actualizar()
    //------------------------------
    //
    // La pantalla no recibe AudioJuego: Juego puede leer estas banderas
    // despues de Actualizar() y reproducir el sonido que corresponda.

    bool sonidoMover =
        false;

    bool sonidoConfirmar =
        false;

    bool sonidoCancelar =
        false;


    //------------------------------
    // FUNCIONES
    //------------------------------

    void Inicializar(
        Participante participantes[],
        int cantidadMaxima
    );

    void Actualizar(
        float deltaTime,
        Participante participantes[],
        int cantidadMaxima
    );

    void Dibujar(
        const Participante participantes[],
        int cantidadMaxima
    ) const;

    void Descargar();
};
