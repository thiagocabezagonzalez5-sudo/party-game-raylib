#pragma once

#include "Core/Participante.h"
#include "Core/ResultadoMinijuego.h"
#include "Minigames/TiposMinijuegos.h"
#include "Systems/Audio.h"


//==================================================
// BALSAS DEL RAPIDO
//==================================================
//
// Carrera de balsas por equipos (2 vs 2; con 3 jugadores 2 vs 1 y con 2,
// 1 vs 1) por un rio de selva. Cada equipo comparte una balsa en su carril
// (dos carriles separados por una isla). Cada jugador rema en SU lado:
// una palada en el lado izquierdo empuja y gira la balsa hacia la derecha
// y viceversa. Paladas de ambos lados casi a la vez (0.25 s) dan un
// bonus y avanzan recto. Rocas y troncos frenan, los remolinos giran la
// balsa y las bananas dan un boost corto. A mitad del recorrido hay una
// bifurcacion: cascada corta con rocas o remanso largo y seguro.
// Con un solo jugador por balsa, ese jugador rema ambos lados: sin
// direccion rema los dos; con IZQ/DER gira hacia ese lado (con ayuda de
// estabilidad). Gana la primera balsa en la meta (120 u) o la mas
// avanzada a los 70 s; empate solo si van exactamente igual.
//==================================================

inline constexpr int MAX_OBSTACULOS_BALSAS = 40;
inline constexpr int MAX_PARTICULAS_BALSAS = MAX_PARTICULAS_TIERRA;


enum FaseBalsasRapido
{
    FASE_BALSAS_PREPARACION = 0,
    FASE_BALSAS_JUGANDO,
    FASE_BALSAS_TERMINADO
};


enum TipoObstaculoBalsas
{
    OBSTACULO_BALSAS_ROCA = 0,
    OBSTACULO_BALSAS_TRONCO,
    OBSTACULO_BALSAS_DIVISOR,     // isla que separa la cascada del remanso
    OBSTACULO_BALSAS_REMOLINO,
    OBSTACULO_BALSAS_BANANA
};


struct ObstaculoBalsas
{
    TipoObstaculoBalsas tipo = OBSTACULO_BALSAS_ROCA;
    float p = 0.0f;         // avance a lo largo del rio
    float desvio = 0.0f;    // posicion lateral respecto al centro del carril
    float a = 0.0f;         // radio (roca/remolino/banana) o medio ancho
    float b = 0.0f;         // medio largo (tronco/divisor)
};


struct EstadoJugadorBalsas
{
    int equipo = -1;
    int lado = 0;                  // 0 izquierdo, 1 derecho, 2 ambos (solo)
    float recarga = 0.0f;

    // IA (bots y humanos desconectados).
    float temporizador = 0.0f;
    float periodo = 0.42f;
};


struct BalsaRio
{
    bool activa = false;
    int equipo = 0;
    int cantidad = 0;              // jugadores en la balsa

    float centroCarril = 0.0f;
    float x = 0.0f;
    float p = 0.0f;
    float rumbo = 0.0f;            // radianes; positivo = hacia +x
    float omega = 0.0f;
    float velocidad = 0.0f;

    float tiempoBoost = 0.0f;
    float tiempoImpacto = 0.0f;
    float tiempoSync = 0.0f;
    float tiempoPalada[2]{ 9.0f, 9.0f };
    bool paladaSinPareja[2]{};

    bool terminada = false;
    int bananas = 0;
    bool recogidas[MAX_OBSTACULOS_BALSAS]{};

    // IA de rumbo del equipo.
    int ruta = 0;                  // 0 remanso seguro, 1 cascada
    float objetivoX = 0.0f;
    float ruido = 0.0f;
    float tiempoRuido = 0.0f;
};


struct MinijuegoBalsasRapido
{
    ResultadoMinijuego resultado{};
    AudioJuego* audio = nullptr;   // lo asigna GestorMinijuegos; puede ser nullptr

    EstadoJugadorBalsas estadosJugadores[MAX_PARTICIPANTES];
    int cantidadJugadoresEquipo[2]{};
    BalsaRio balsas[2];
    ObstaculoBalsas obstaculos[MAX_OBSTACULOS_BALSAS];
    int cantidadObstaculos = 0;
    ParticulaTierra particulas[MAX_PARTICULAS_BALSAS];

    int equipoGanador = -1;
    bool empate = false;
    bool partidaValida = false;

    float tiempoSonidoPalada = 0.0f;

    Camera3D camara{};
    float focoCamara = 0.0f;
    float separacionCamara = 0.0f;

    FaseBalsasRapido fase = FASE_BALSAS_PREPARACION;

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
