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

    bool iniciarPartida =
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
