#pragma once

#include "Core/ConfiguracionJuego.h"
#include "Minigames/TiposMinijuegos.h"


enum TipoPieza67
{
    PIEZA_NUMERO_6 = 0,
    PIEZA_NUMERO_7
};


enum EstadoMesa67
{
    MESA_67_VACIA = 0,
    MESA_67_CON_6,
    MESA_67_COMPLETA
};


struct EstadoJugador67
{
    TipoPieza67 tipoPieza =
        PIEZA_NUMERO_6;

    float progresoObjeto = 0.0f;
    bool objetoActivo = true;
    float tiempoReaparicion = 0.0f;

    bool llevaPieza = false;
    bool mirandoMesa = false;
    float tiempoGiro = 0.0f;

    float tiempoStun = 0.0f;
};


struct Minijuego67
{
    EstadoJugador67 estadosJugadores[
        MAX_JUGADORES_PRUEBA
    ];

    EstadoMesa67 mesas[2] =
    {
        MESA_67_VACIA,
        MESA_67_VACIA
    };

    float tiempoMesaCompleta[2] =
    {
        0.0f,
        0.0f
    };

    float tiempoPartida = 30.0f;
    float velocidadCintas = 0.24f;
    float desplazamientoVisualCintas = 0.0f;

    int puntos = 0;
    int jugadoresEnPartida = 0;

    bool terminado = false;

    Camera3D camara{};

    Model modeloJugador{};
    bool modeloJugadorCargado = false;

    ModelAnimation* animacionesJugador = nullptr;
    int cantidadAnimacionesJugador = 0;
    int indiceAnimacionIdle = -1;

    float fotogramaAnimacionIdle = 0.0f;
    bool animacionIdleActiva = false;

    const char* rutaModeloJugador =
        "Assets/Modelos/Jugador_Raylib_Normalizado.glb";

    void Inicializar();

    void ConfigurarJugadores(
        JugadorPrueba jugadores[],
        int cantidadMaxima
    ) const;

    void Reiniciar(
        const JugadorPrueba jugadores[],
        int cantidadMaxima
    );

    void Actualizar(
        float deltaTime,
        const JugadorPrueba jugadores[],
        int cantidadMaxima,
        ModoTeclado modoTeclado
    );

    void Dibujar(
        const JugadorPrueba jugadores[],
        int cantidadMaxima,
        ModoTeclado modoTeclado
    ) const;

    void Descargar();
};
