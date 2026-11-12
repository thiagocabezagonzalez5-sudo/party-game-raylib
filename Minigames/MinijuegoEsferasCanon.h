#pragma once

#include "Core/Participante.h"
#include "Core/ResultadoMinijuego.h"
#include "Minigames/TiposMinijuegos.h"
#include "Systems/Audio.h"


// Referencia de reglas: Mario Party 3 - Boulder Ball (carrera cuesta abajo).
// Aqui se transforma en un canon desertico: cada jugador rueda sobre una
// esfera de piedra por una pista serpenteante, con choques entre esferas,
// obstaculos, grietas, arena suelta y una rampa de atajo.


inline constexpr int PUNTOS_PISTA_ESFERAS = 53;
inline constexpr int MAX_OBSTACULOS_ESFERAS = 24;
inline constexpr int MAX_PARTICULAS_ESFERAS = 220;


// Particula puramente visual (polvo, esquirlas, confeti). No afecta reglas.
struct ParticulaEsferas
{
    bool activa = false;
    Vector3 posicion{};
    Vector3 velocidad{};
    Color color{};
    float vida = 0.0f;
    float vidaMaxima = 1.0f;
    float tamano = 0.1f;
    float gravedad = 0.0f;
    bool brilla = false;
    bool polvo = false;
};


// Estado visual por jugador: lo que se vio el cuadro anterior para
// detectar eventos (choque, caida, salto, control, meta) sin tocar reglas.
struct VisualJugadorEsferas
{
    bool enAire = false;
    bool cayendo = false;
    bool terminado = false;
    int checkpoints = 0;
    float velocidadX = 0.0f;
    float velocidadZ = 0.0f;
    float recarga = 0.0f;

    float destello = 0.0f;
    float avisoControl = 0.0f;
    float avisoAtajo = 0.0f;
    float avisoCaida = 0.0f;
    float avisoChoque = 0.0f;
    float polvoArena = 0.0f;
};


enum FaseEsferasCanon
{
    FASE_ESFERAS_PREPARACION = 0,
    FASE_ESFERAS_JUGANDO,
    FASE_ESFERAS_TERMINADO
};


struct EstadoJugadorEsferas
{
    bool participa = false;

    // Fisica de la esfera sobre la pista (plano XZ).
    float x = 0.0f;
    float z = 0.0f;
    float velocidadX = 0.0f;
    float velocidadZ = 0.0f;

    // Progreso a lo largo de la pista y desplazamiento lateral.
    float s = 0.0f;
    float lateral = 0.0f;
    float progresoMaximo = 0.0f;

    float tiempoAire = 0.0f;
    float recargaImpulso = 0.0f;
    float penalizacion = 0.0f;
    float caidaX = 0.0f;
    float caidaZ = 0.0f;
    int checkpoints = 0;

    bool terminado = false;
    float tiempoMeta = 0.0f;

    // Giro visual de la esfera.
    float giro = 0.0f;
    float ejeX = 1.0f;
    float ejeZ = 0.0f;

    // Inteligencia de bot.
    float tiempoDecision = 0.0f;
    float latObjetivo = 0.0f;
    float latPreferida = 0.0f;
    float objetivoX = 0.0f;
    float objetivoZ = 0.0f;
    bool arriesgado = false;
    bool impulsoPendiente = false;
    float atascoTiempo = 0.0f;
    float atascoReferencia = 0.0f;
    float marchaAtras = 0.0f;
};


struct ObstaculoEsferas
{
    float x = 0.0f;
    float z = 0.0f;
    float s = 0.0f;
    float lateral = 0.0f;
    float radio = 0.9f;
    bool cactus = false;
};


struct MinijuegoEsferasCanon
{
    ResultadoMinijuego resultado{};
    AudioJuego* audio = nullptr;   // lo asigna GestorMinijuegos; puede ser nullptr

    EstadoJugadorEsferas estadosJugadores[MAX_PARTICIPANTES];

    // Linea central de la pista (datos logicos).
    float puntosX[PUNTOS_PISTA_ESFERAS]{};
    float puntosZ[PUNTOS_PISTA_ESFERAS]{};
    float puntosS[PUNTOS_PISTA_ESFERAS]{};
    float longitudPista = 0.0f;

    ObstaculoEsferas obstaculos[MAX_OBSTACULOS_ESFERAS];
    int cantidadObstaculos = 0;

    Camera3D camara{};
    FaseEsferasCanon fase = FASE_ESFERAS_PREPARACION;

    bool partidaValida = false;
    bool hayGanadorPorMeta = false;

    float tiempoPreparacion = 0.0f;
    float tiempoRestante = 0.0f;
    float tiempoCarrera = 0.0f;
    float tiempoCierre = -1.0f;
    float tiempoAnimacion = 0.0f;
    float cooldownSonidoChoque = 0.0f;

    // Solo visual: particulas, avisos y anuncios (semilla propia).
    ParticulaEsferas particulas[MAX_PARTICULAS_ESFERAS];
    VisualJugadorEsferas visualJugadores[MAX_PARTICIPANTES];
    unsigned int semillaVisual = 0x5EEDu;
    float tiempoVisual = 0.0f;
    float anuncioInicio = -1.0f;
    float anuncioMeta = -1.0f;
    int jugadorMeta = -1;
    bool confetiFinal = false;

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

    // Reglas de la carrera (sin cambios); Actualizar la envuelve y despues
    // actualiza solo lo visual.
    void ActualizarReglas(
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

    // Libera lo horneado en la GPU (mallas, material y shader); se rehace
    // solo la proxima vez que se dibuje.
    void Descargar();
};
