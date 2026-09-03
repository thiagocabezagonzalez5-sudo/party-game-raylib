#pragma once

#include "Core/Participante.h"
#include "Core/ResultadoMinijuego.h"
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


enum EstadoPartida67
{
    FABRICA_67_ESPERANDO_JUGADORES = 0,
    FABRICA_67_PREPARANDO,
    FABRICA_67_JUGANDO,
    FABRICA_67_FINALIZADO
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


struct EstadoEquipo67
{
    EstadoMesa67 mesa =
        MESA_67_VACIA;

    float tiempoMesaCompleta = 0.0f;
    float tiempoFeedback = 0.0f;

    int puntos = 0;

    bool ultimoAcierto = false;
};


struct Minijuego67
{
    ResultadoMinijuego resultado;

    EstadoJugador67 estadosJugadores[
        MAX_JUGADORES_PRUEBA
    ];

    EstadoEquipo67 equipos[2];

    int equipoPorJugador[
        MAX_JUGADORES_PRUEBA
    ] = { -1, -1, -1, -1 };

    int ordenEnEquipoPorJugador[
        MAX_JUGADORES_PRUEBA
    ] = { -1, -1, -1, -1 };

    int cantidadJugadoresEquipo[2] =
    {
        0,
        0
    };

    EstadoPartida67 estadoPartida =
        FABRICA_67_ESPERANDO_JUGADORES;

    float tiempoPreparacion = 2.5f;
    float tiempoPartida = 30.0f;
    float velocidadCintas = 0.24f;
    float desplazamientoVisualCintas = 0.0f;

    int jugadoresEnPartida = 0;
    int mascaraJugadoresEnPartida = 0;
    int equipoGanador = -1;

    bool partidaValida = false;
    bool empate = false;

    Camera3D camarasEquipo[2]{};

    RenderTexture2D vistasEquipo[2]{};
    bool vistasEquipoCargadas = false;
    int anchoVistaEquipos = 0;
    int altoVistaEquipos = 0;

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

    void Reiniciar(
        const Participante participantes[],
        int cantidadMaxima
    );

    void PrepararEquipos(
        const Participante participantes[],
        int cantidadMaxima
    );

    void Actualizar(
        float deltaTime,
        int cantidadMaxima,
        const Participante participantes[]
    );

    void Dibujar(
        int cantidadMaxima,
        const Participante participantes[]
    ) const;

    const ResultadoMinijuego& ObtenerResultado() const;

    void Descargar();
};
