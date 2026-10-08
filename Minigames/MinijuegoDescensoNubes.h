#pragma once

#include "Core/Participante.h"
#include "Core/ResultadoMinijuego.h"
#include "Minigames/TiposMinijuegos.h"
#include "Systems/Audio.h"


//==================================================
// DESCENSO EN NUBES
//==================================================
//
// Todos contra todos. Cada jugador cae planeando desde gran altura dentro de
// un cilindro de caida. Se mueve en el plano X/Z para recoger anillos (1
// punto, dorados 3), evita nubes de tormenta (aturden y quitan 2 puntos) y
// las rafagas de viento. La accion frena la caida durante 1 s (recarga 2 s).
// Al aterrizar en la isla se suman 5 puntos si se cae en el circulo central.
//==================================================

inline constexpr int MAX_ANILLOS_NUBES = 64;
inline constexpr int MAX_TORMENTAS_NUBES = 16;
inline constexpr int MAX_VIENTOS_NUBES = 6;


enum FaseNubes
{
    FASE_NUBES_PREPARACION = 0,
    FASE_NUBES_JUGANDO,
    FASE_NUBES_TERMINADO
};


struct AnilloNubes
{
    Vector3 posicion{};
    int valor = 1;
    unsigned int recogidoPor = 0;
};


struct TormentaNubes
{
    Vector3 posicion{};
    float radio = 1.6f;
    unsigned int golpeadoA = 0;
};


struct VientoNubes
{
    float altura = 0.0f;
    float dirX = 1.0f;
    float dirZ = 0.0f;
};


struct EstadoJugadorNubes
{
    float altura = 0.0f;
    float velX = 0.0f;
    float velZ = 0.0f;

    float aturdido = 0.0f;
    float frenando = 0.0f;
    float recargaFreno = 0.0f;

    int puntos = 0;
    int golpes = 0;
    bool aterrizo = false;
    bool bonusCentro = false;

    // Estado interno de la IA.
    float botReevaluar = 0.0f;
    float botObjetivoX = 0.0f;
    float botObjetivoZ = 0.0f;
    bool botFrenar = false;
};


struct MinijuegoDescensoNubes
{
    ResultadoMinijuego resultado{};
    AudioJuego* audio = nullptr;   // lo asigna GestorMinijuegos; puede ser nullptr

    EstadoJugadorNubes estados[MAX_PARTICIPANTES];
    Color coloresJugadores[MAX_PARTICIPANTES];
    bool participa[MAX_PARTICIPANTES]{};

    AnilloNubes anillos[MAX_ANILLOS_NUBES];
    TormentaNubes tormentas[MAX_TORMENTAS_NUBES];
    VientoNubes vientos[MAX_VIENTOS_NUBES];
    int cantidadAnillos = 0;
    int cantidadTormentas = 0;
    int cantidadVientos = 0;

    Camera3D camara{};
    FaseNubes fase = FASE_NUBES_PREPARACION;

    float alturaCamara = 0.0f;
    float tiempoPreparacion = 0.0f;
    float tiempoJuego = 0.0f;
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
