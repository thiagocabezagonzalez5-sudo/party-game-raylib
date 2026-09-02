#pragma once

#include "raylib.h"


//==================================================
// CONSTANTES
//==================================================

const int MAX_JUGADORES_SELECCION =
    4;

const int MAX_PERSONAJES_SELECCION =
    4;


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
    // CONEXION
    //------------------------------

    bool conectado =
        false;

    bool usaTeclado =
        false;

    int indiceGamepad =
        -1;


    //------------------------------
    // JUGADOR
    //------------------------------

    int colorJugador =
        0;


    //------------------------------
    // PERSONAJE
    //------------------------------

    int cursorPersonaje =
        0;

    int personajeConfirmado =
        -1;

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

    void Inicializar();

    void Actualizar(
        float deltaTime
    );

    void Dibujar() const;

    void Descargar();
};