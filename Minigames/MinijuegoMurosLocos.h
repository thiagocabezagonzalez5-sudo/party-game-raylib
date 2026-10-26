#pragma once

#include "Core/Participante.h"
#include "Core/ResultadoMinijuego.h"
#include "Minigames/TiposMinijuegos.h"
#include "Systems/Audio.h"


enum FaseMurosLocos
{
    FASE_MUROS_PREPARACION = 0,
    FASE_MUROS_JUGANDO,
    FASE_MUROS_TERMINADO
};


struct EstadoJugadorMurosLocos
{
    bool eliminado = false;
    int posicionFinal = 0;
    int tiempoSobrevividoMs = 0;
    float cooldownImpacto = 0.0f;

    // Estado de la IA cuando el participante es bot o esta desconectado.
    int muroBot = -1;
    float retrasoBot = 0.0f;
    float objetivoX = 0.0f;
    float objetivoZ = 0.0f;

    // Solo visual / audio: flanco de suelo, destello del empujon y caida
    // al foso de arena tras la eliminacion.
    bool enSueloAnterior = true;
    float destelloGolpe = 0.0f;
    float tiempoCaida = -1.0f;      // < 0: no esta cayendo
    Vector3 posicionCaida{};
    Vector3 velocidadCaida{};
    bool tocoArena = false;
    float hundimiento = 0.0f;       // 0..1 se hunde en la arena del foso
};


struct MuroLoco
{
    float z = -7.2f;
    float velocidad = 4.2f;
    float anchoHueco = 2.5f;

    int cantidadHuecos = 1;
    float centrosHueco[2] = { 0.0f, 0.0f };
};


// Efectos de las ruinas (solo visuales).
struct ParticulaMurosLocos
{
    bool activa = false;
    Vector3 posicion{};
    Vector3 velocidad{};
    float vida = 0.0f;
    float vidaMaxima = 1.0f;
    float tamano = 0.1f;
    float gravedad = 0.0f;
    float freno = 1.2f;
    Color color{};
};


// «¡J2 AL FOSO!» sobre el borde por donde cayo.
struct AvisoMurosLocos
{
    bool activo = false;
    int jugador = 0;
    Vector3 posicion{};
    float vida = 0.0f;
};


static const int MAX_PARTICULAS_MUROS = 360;
static const int MAX_AVISOS_MUROS = 4;


struct MinijuegoMurosLocos
{
    ResultadoMinijuego resultado;

    EstadoJugadorMurosLocos estadosJugadores[
        MAX_JUGADORES_PRUEBA
    ];

    BloquePrueba suelo;
    MuroLoco muro;
    Camera3D camara{};

    FaseMurosLocos fase = FASE_MUROS_PREPARACION;

    float tiempoPreparacion = 3.0f;
    float tiempoRestante = 35.0f;
    float tiempoJugado = 0.0f;
    float tiempoEntreMuros = 0.0f;

    int numeroMuro = 0;

    // Solo visual: reloj de animacion, efectos y anuncios. Nada de esto
    // toca la logica ni consume el azar de la partida.
    float tiempoAnimacion = 0.0f;
    unsigned int semillaVisual = 2891336453u;
    bool confetiLanzado = false;
    int muroDerrumbado = 0;         // ultimo muro que ya cayo al foso
    int muroAnunciado = 0;
    float tiempoAnuncio = -1.0f;
    int tipoAnuncio = 0;
    bool dobleHuecoVisto = false;

    ParticulaMurosLocos particulasEfecto[MAX_PARTICULAS_MUROS];
    AvisoMurosLocos avisos[MAX_AVISOS_MUROS];

    // Lo conecta GestorMinijuegos; puede ser nullptr.
    AudioJuego* audio = nullptr;

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
