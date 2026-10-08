#pragma once

#include "Core/Participante.h"
#include "Core/ResultadoMinijuego.h"
#include "Minigames/TiposMinijuegos.h"


enum FaseMinijuegoIslaFuego
{
    FASE_ISLA_FUEGO_PREPARACION = 0,
    FASE_ISLA_FUEGO_JUGANDO,
    FASE_ISLA_FUEGO_TERMINADO
};


struct EstadoJugadorIslaFuego
{
    bool eliminado = false;
    int posicionFinal = 0;
    int tiempoSobrevividoMs = 0;
    float tiempoAturdido = 0.0f;

    // Tras un aturdimiento ninguna bomba puede alcanzar al jugador hasta
    // 1.2 s despues de que recupere el control (evita cadenas inevitables).
    float proteccionBomba = 0.0f;

    // Un impacto directo con el cuerpo de la bomba no es
    // un simple empujon de area: lanza al jugador y, tras
    // una fraccion de segundo visible, lo elimina.
    bool impactoDirecto = false;
    float tiempoHastaEliminacionDirecta = 0.0f;
};


struct ProyectilIslaFuego
{
    bool activo = false;
    bool especial = false;

    Vector3 puntoImpacto{};

    float tiempoHastaImpacto = 0.0f;
    float duracionAviso = 1.25f;
    float radioExplosion = 2.0f;
};


// IA de bots: ven la bomba con retardo, la ubican con error y a veces
// se distraen. Nada de esto depende del indice del jugador.
struct EstadoBotIslaFuego
{
    bool vioProyectil = false;
    bool distraido = false;
    float retardo = 0.0f;
    Vector3 puntoPercibido{};
    Vector3 destino{};
    float tiempoDeriva = 0.0f;
};


struct MinijuegoIslaFuego
{
    ResultadoMinijuego resultado;

    EstadoJugadorIslaFuego estadosJugadores[
        MAX_JUGADORES_PRUEBA
    ];

    EstadoBotIslaFuego bots[MAX_JUGADORES_PRUEBA];

    BloquePrueba suelo;

    Camera3D camara{};

    FaseMinijuegoIslaFuego fase =
        FASE_ISLA_FUEGO_PREPARACION;

    ProyectilIslaFuego proyectil;

    float tiempoPreparacion = 3.0f;
    float tiempoRestante = 30.0f;
    float tiempoJugado = 0.0f;
    float tiempoHastaSiguienteDisparo = 0.8f;

    bool disparoFinalRealizado = false;

    void Inicializar();

    void ConfigurarJugadores(
        JugadorPrueba jugadores[],
        int cantidadMaxima
    ) const;

    void Reiniciar(
        JugadorPrueba jugadores[],
        int cantidadMaxima
    );

    void Actualizar(
        float deltaTime,
        JugadorPrueba jugadores[],
        int cantidadMaxima,
        Participante participantes[],
        ParticulaTierra particulas[],
        int cantidadParticulas
    );

    void Dibujar(
        const JugadorPrueba jugadores[],
        int cantidadMaxima,
        const Participante participantes[],
        const ParticulaTierra particulas[],
        int cantidadParticulas,
        bool mostrarDebug
    ) const;

    const ResultadoMinijuego& ObtenerResultado() const;
};
