#pragma once

#include "Core/Participante.h"
#include "Core/ResultadoMinijuego.h"
#include "Minigames/TiposMinijuegos.h"
#include "Systems/Audio.h"
#include "raylib.h"


const int MAX_NUCLEOS_ENERGIA = 6;
const int MAX_BLOQUES_NUCLEOS = 5;
const int MAX_HISTORIAL_NUCLEOS = 512;
const int MAX_NUCLEOS_CAIDOS = 32;
const int MAX_PARTICULAS_NUCLEOS = 220;
const int MAX_DESTELLOS_NUCLEOS = 20;
const int MAX_AVISOS_NUCLEOS = 12;


enum FaseMinijuegoNucleos
{
    FASE_NUCLEOS_PREPARACION = 0,
    FASE_NUCLEOS_JUGANDO,
    FASE_NUCLEOS_TERMINADO
};


struct NucleoEnergia
{
    Vector3 posicion{};

    bool activo = false;
    bool especial = false;

    float tiempoReaparicion = 0.0f;
    float faseFlotacion = 0.0f;

    // Solo visual: momento (tiempoAnimacion) en que aparecio, para la
    // animacion de entrada. No afecta la recogida.
    float tiempoAparicion = -10.0f;
};


struct NucleoEnergiaCaido
{
    Vector3 posicion{};
    Vector3 velocidad{};

    bool activo = false;
    bool especial = false;

    // Durante unos instantes el jugador golpeado no puede volver
    // a recoger exactamente los mismos nucleos que acaba de perder.
    int jugadorBloqueado = -1;
    float tiempoBloqueo = 0.0f;
    float tiempoVida = 0.0f;
};


// Particula propia (chispas del color del nucleo, motas de ambiente).
struct ParticulaNucleos
{
    bool activa = false;
    bool gravedad = true;
    Vector3 posicion{};
    Vector3 velocidad{};
    float vida = 0.0f;
    float vidaMaxima = 0.0f;
    float tamano = 0.06f;
    Color color = WHITE;
};


// Onda de luz breve: recogida, aparicion de nucleo o golpe.
struct DestelloNucleos
{
    bool activo = false;
    Vector3 posicion{};
    float vida = 0.0f;
    float vidaMaxima = 0.0f;
    float radio = 1.0f;
    Color color = WHITE;
};


// Numero flotante sobre el mundo ("+3", "-2").
struct AvisoNucleos
{
    bool activo = false;
    Vector3 posicion{};
    float vida = 0.0f;
    int valor = 0;
    bool especial = false;
};


struct HistorialNucleosJugador
{
    bool especiales[MAX_HISTORIAL_NUCLEOS]{};
    int cantidad = 0;
};


struct MinijuegoNucleosEnergia
{
    Camera3D camara{};

    BloquePrueba bloques[MAX_BLOQUES_NUCLEOS];
    int cantidadBloques = 0;

    NucleoEnergia nucleos[MAX_NUCLEOS_ENERGIA];
    NucleoEnergiaCaido nucleosCaidos[MAX_NUCLEOS_CAIDOS];
    HistorialNucleosJugador historial[MAX_PARTICIPANTES];

    int puntuaciones[MAX_PARTICIPANTES]{};

    // Para desempatar si dos llegan a la meta en el mismo frame.
    int puntuacionesPrevias[MAX_PARTICIPANTES]{};
    int golpesRecibidos[MAX_PARTICIPANTES]{};

    ResultadoMinijuego resultado{};

    FaseMinijuegoNucleos fase =
        FASE_NUCLEOS_PREPARACION;

    float tiempoPreparacion = 3.0f;
    float tiempoRestante = 35.0f;
    float tiempoAnimacion = 0.0f;

    // Solo visual.
    ParticulaNucleos particulasLocales[MAX_PARTICULAS_NUCLEOS];
    DestelloNucleos destellos[MAX_DESTELLOS_NUCLEOS];
    AvisoNucleos avisos[MAX_AVISOS_NUCLEOS];
    float temporizadorAmbiente = 0.0f;

    int ultimoNumeroCuenta = -1;
    int ultimoNumeroAlerta = -1;

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
        int cantidadParticulas,
        AudioJuego* audio
    );

    void Dibujar(
        const JugadorPrueba jugadores[],
        int cantidadMaxima,
        const Participante participantes[],
        bool mostrarDebug
    ) const;

    const ResultadoMinijuego& ObtenerResultado() const;
};
