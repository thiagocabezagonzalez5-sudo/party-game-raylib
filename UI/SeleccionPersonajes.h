#pragma once

#include "Core/Participante.h"
#include "raylib.h"


//==================================================
// CONSTANTES
//==================================================

const int MAX_JUGADORES_SELECCION =
    4;

const int MAX_PERSONAJES_SELECCION =
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

struct PersonajeSeleccion
{
    //------------------------------
    // DATOS
    //------------------------------

    const char* nombre =
        "";

    Color color =
        LIGHTGRAY;


    //------------------------------
    // ARCHIVOS
    //------------------------------

    const char* rutaIcono =
        "";

    const char* rutaRetrato =
        "";


    //------------------------------
    // TEXTURAS
    //------------------------------

    Texture2D icono =
        {};

    Texture2D retrato =
        {};



    //------------------------------
    // ESTADOS
    //------------------------------

    bool iconoCargado =
        false;

    bool retratoCargado =
        false;
};


//==================================================
// JUGADOR
//==================================================

struct JugadorSeleccion
{
    //------------------------------
    // PERSONAJE
    //------------------------------

    int cursorPersonaje =
        0;

    bool listo =
        false;


    //------------------------------
    // BLOQUEOS DEL STICK
    //------------------------------

    bool bloqueoHorizontal =
        false;

    bool bloqueoVertical =
        false;
};


//==================================================
// SELECCION DE PERSONAJES
//==================================================

struct SeleccionPersonajes
{
    //------------------------------
    // PERSONAJES
    //------------------------------

    PersonajeSeleccion personajes[
        MAX_PERSONAJES_SELECCION
    ];


    //------------------------------
    // JUGADORES
    //------------------------------

    JugadorSeleccion jugadores[
        MAX_JUGADORES_SELECCION
    ];


    //------------------------------
    // GRID
    //------------------------------

    int columnas =
        2;

    int filas =
        2;


    //------------------------------
    // RECURSOS
    //------------------------------

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
