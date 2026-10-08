#pragma once

#include "Core/Participante.h"
#include "Core/ResultadoMinijuego.h"
#include "Minigames/TiposMinijuegos.h"
#include "Systems/Audio.h"


//==================================================
// ULTIMO ASIENTO
//==================================================
//
// Todos contra todos en un parque de diversiones. Mientras suena el
// organillo los jugadores corren alrededor del carrusel central. Cuando la
// musica se corta se iluminan tantas tazas como jugadores vivos menos uno:
// hay que ocupar una antes de que se cierren. Se puede empujar a quien ya
// la ocupa. Quien queda sin taza es eliminado y mira desde la grada. A veces
// una taza es trampa: parpadea en rojo antes de cerrarse y expulsa a su
// ocupante. Gana quien sobreviva a todas las rondas (o los supervivientes
// al acabarse el tiempo).
//==================================================

inline constexpr int MAX_TAZAS_ULTIMO_ASIENTO = MAX_PARTICIPANTES;
inline constexpr int MAX_PARTICULAS_ULTIMO_ASIENTO = MAX_PARTICULAS_TIERRA;
inline constexpr int MAX_BLOQUES_ULTIMO_ASIENTO = 2;


enum FaseUltimoAsiento
{
    FASE_ULTIMO_ASIENTO_PREPARACION = 0,
    FASE_ULTIMO_ASIENTO_JUGANDO,
    FASE_ULTIMO_ASIENTO_TERMINADO
};


enum SubfaseUltimoAsiento
{
    SUBFASE_ULTIMO_ASIENTO_MUSICA = 0,
    SUBFASE_ULTIMO_ASIENTO_TAZAS,
    SUBFASE_ULTIMO_ASIENTO_RESOLUCION
};


struct TazaUltimoAsiento
{
    bool activa = false;
    bool trampa = false;
    Vector3 posicion{};
    int ocupante = -1;
    float destello = 0.0f;
};


struct EstadoBotUltimoAsiento
{
    float retardoBase = 0.3f;
    float retardo = 0.0f;
    int objetivoTaza = -1;
    float reevaluar = 0.0f;
    float tiempoVagar = 0.0f;
    float vagarX = 0.0f;
    float vagarZ = 0.0f;
    float cooldownGolpe = 0.0f;
};


struct MinijuegoUltimoAsiento
{
    ResultadoMinijuego resultado{};
    AudioJuego* audio = nullptr;   // lo asigna GestorMinijuegos; puede ser nullptr

    BloquePrueba bloques[MAX_BLOQUES_ULTIMO_ASIENTO];
    ParticulaTierra particulas[MAX_PARTICULAS_ULTIMO_ASIENTO];
    TazaUltimoAsiento tazas[MAX_TAZAS_ULTIMO_ASIENTO];
    EstadoBotUltimoAsiento bots[MAX_PARTICIPANTES];
    Color coloresJugadores[MAX_PARTICIPANTES];

    bool vivo[MAX_PARTICIPANTES]{};
    int rondaEliminacion[MAX_PARTICIPANTES]{};
    int tazaDe[MAX_PARTICIPANTES]{};

    Camera3D camara{};
    FaseUltimoAsiento fase = FASE_ULTIMO_ASIENTO_PREPARACION;
    SubfaseUltimoAsiento subfase = SUBFASE_ULTIMO_ASIENTO_MUSICA;

    int cantidadBloques = 0;
    int cantidadTazas = 0;
    int ronda = 0;
    int indiceTrampa = -1;
    bool avisoTrampaDado = false;

    float tiempoPreparacion = 0.0f;
    float tiempoRestante = 0.0f;
    float tiempoSubfase = 0.0f;
    float tiempoAnimacion = 0.0f;
    float anguloCarrusel = 0.0f;
    float velocidadCarrusel = 0.0f;

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
