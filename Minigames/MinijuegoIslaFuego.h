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


// Particulas propias (pool fijo): humo, chispas, arena y agua. Solo visuales.
inline constexpr int MAX_PARTICULAS_ISLA = 320;

enum TipoParticulaIslaFuego
{
    PARTICULA_ISLA_HUMO = 0,
    PARTICULA_ISLA_CHISPA,
    PARTICULA_ISLA_ARENA,
    PARTICULA_ISLA_AGUA,
    PARTICULA_ISLA_CONFETI
};


struct ParticulaIslaFuego
{
    bool activa = false;
    TipoParticulaIslaFuego tipo = PARTICULA_ISLA_HUMO;
    Vector3 posicion{};
    Vector3 velocidad{};
    float vida = 0.0f;
    float vidaMaxima = 0.0f;
    float tamano = 0.1f;
    Color color = { 200, 200, 200, 255 };
};


// Estado puramente visual (animaciones y reacciones). No interviene en la
// logica: solo lo leen Dibujar y los emisores de efectos.
struct EfectosIslaFuego
{
    ParticulaIslaFuego particulas[MAX_PARTICULAS_ISLA];

    unsigned int semilla = 12345u;
    float tiempoAnimacion = 0.0f;
    float temporizadorEstela = 0.0f;

    // Disparo del barco (coincide con la aparicion de la bomba).
    bool disparoPendiente = false;
    int canonDisparo = 0;
    float tiempoDesdeDisparo = 99.0f;

    // Ultima explosion.
    Vector3 puntoExplosion{};
    bool explosionEspecial = false;
    float tiempoDesdeExplosion = 99.0f;

    // Reacciones por jugador.
    float tiempoDesdeGolpe[MAX_PARTICIPANTES]{};
    bool golpeDirecto[MAX_PARTICIPANTES]{};
    float temporizadorRastro[MAX_PARTICIPANTES]{};
    bool salpico[MAX_PARTICIPANTES]{};
    Vector3 puntoSalpicadura[MAX_PARTICIPANTES]{};
    float tiempoDesdeSalpicadura[MAX_PARTICIPANTES]{};
    bool eliminadoVisto[MAX_PARTICIPANTES]{};
    Vector3 puntoEliminacion[MAX_PARTICIPANTES]{};
    float tiempoDesdeEliminacion[MAX_PARTICIPANTES]{};
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

    EfectosIslaFuego efectos;

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
