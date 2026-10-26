#pragma once

#include "Core/Participante.h"
#include "Core/ResultadoMinijuego.h"
#include "Systems/Audio.h"
#include "Minigames/BotsMinijuegos1v3.h"
#include "raylib.h"


enum DireccionMiradaCruzada
{
    MIRADA_FRENTE = 0,
    MIRADA_IZQUIERDA,
    MIRADA_DERECHA,
    MIRADA_ARRIBA,
    MIRADA_ABAJO,
    CANTIDAD_DIRECCIONES_MIRADA
};


enum FaseMiradasCruzadas
{
    FASE_MIRADAS_PREPARACION = 0,
    FASE_MIRADAS_ELECCION,
    FASE_MIRADAS_RESOLUCION,
    FASE_MIRADAS_TERMINADO
};


struct EstadoJugadorMiradasCruzadas
{
    bool eliminado = false;
    bool eligioDireccion = false;
    DireccionMiradaCruzada direccion = MIRADA_FRENTE;
    int posicionFinal = 0;
    int rondaEliminacion = 0;   // solo informativo (tablon final)
};


// Solo visual: giro animado de la cabeza-titere, caida por la trampilla
// y destellos. No influye en la logica.
struct VisualJugadorMiradas
{
    float giro = 0.0f;              // grados, + hacia la derecha de pantalla
    float cabeceo = 0.0f;           // grados, - hacia arriba
    float velocidadGiro = 0.0f;
    float velocidadCabeceo = 0.0f;
    float tiempoAtrapado = -1.0f;   // >= 0: atrapado (espera y cae)
    bool cayo = false;              // ya sono la trampilla
    float destello = 0.0f;          // brillo al salvarse / atrapar (1 -> 0)
    float pulsoListo = 0.0f;        // salto del farolito al elegir (1 -> 0)
    bool eligioAntes = false;
};


// Particula visual (chispas, polvo de la trampilla y confeti).
struct ParticulaMiradas
{
    bool activa = false;
    Vector3 posicion = { 0.0f, 0.0f, 0.0f };
    Vector3 velocidad = { 0.0f, 0.0f, 0.0f };
    float vida = 0.0f;
    float vidaMaxima = 1.0f;
    float tamano = 0.1f;
    float gravedad = 0.0f;
    Color color = { 255, 255, 255, 255 };
};


static const int MAX_PARTICULAS_MIRADAS = 220;


struct MinijuegoMiradasCruzadas
{
    ResultadoMinijuego resultado{};
    AudioJuego* audio = nullptr;

    EstadoJugadorMiradasCruzadas jugadores[MAX_PARTICIPANTES];
    EstadoBotMiradas bots[MAX_PARTICIPANTES];

    FaseMiradasCruzadas fase = FASE_MIRADAS_PREPARACION;

    int indiceSolo = -1;
    int numeroRonda = 0;

    float tiempoPreparacion = 2.5f;
    float tiempoFase = 0.0f;
    float tiempoAnimacion = 0.0f;

    // Solo visual: azar propio (no consume el GetRandomValue de la
    // partida), estado animado de cada titere y particulas.
    unsigned int semillaVisual = 0x2545F491u;
    int rondaRevelada = 0;
    float tiempoRevelado = 0.0f;
    bool confetiLanzado = false;
    VisualJugadorMiradas visual[MAX_PARTICIPANTES];
    ParticulaMiradas particulas[MAX_PARTICULAS_MIRADAS];

    void Inicializar();

    void Reiniciar(
        Participante participantes[]
    );

    void Actualizar(
        float deltaTime,
        Participante participantes[]
    );

    void Dibujar(
        const Participante participantes[]
    ) const;

    const ResultadoMinijuego& ObtenerResultado() const;
};
