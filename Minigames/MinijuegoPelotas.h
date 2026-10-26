#pragma once

#include "Core/Participante.h"
#include "Core/ResultadoMinijuego.h"
#include "Minigames/TiposMinijuegos.h"
#include "Systems/Audio.h"


enum FaseMinijuegoPelotas
{
    FASE_PELOTAS_PREPARACION = 0,
    FASE_PELOTAS_JUGANDO,
    FASE_PELOTAS_TERMINADO
};


struct EstadoJugadorPelotas
{
    bool eliminado = false;
    int posicionFinal = 0;
    int tiempoSobrevividoMs = 0;

    // Solo visual: no cambian reglas ni resultado.
    bool fueraDeCima = false;          // ya paso el borde (polvo emitido)
    Vector3 puntoCaida{};              // borde por donde cayo
    float tiempoEstela = 0.0f;         // acumulador de huellas y polvo
    float destelloChoque = 0.0f;       // 1 -> 0 tras un choque
    float distanciaCentroFinal = 0.0f; // desempate por tiempo (tablon)
};


// Parametros aleatorios por partida de cada bot (no dependen del indice).
struct EstadoBotPelotas
{
    float faseX = 0.0f;
    float faseZ = 0.0f;
    float faseDuda = 0.0f;
    float amplitudError = 0.9f;
    float retardoBorde = 0.0f;
    float esperaBorde = 0.0f;
};


// Efectos locales de la cumbre (pools fijos, escalados por calidad).
inline constexpr int MAX_PARTICULAS_PELOTAS = 280;
inline constexpr int MAX_COPOS_PELOTAS = 170;
inline constexpr int MAX_ONDAS_PELOTAS = 14;
inline constexpr int MAX_HUELLAS_PELOTAS = 140;
inline constexpr int MAX_AVISOS_PELOTAS = 4;


enum TipoParticulaPelotas
{
    PARTICULA_PELOTAS_NIEVE = 0,  // polvo que flota y se frena
    PARTICULA_PELOTAS_CHISPA,     // esquirla de hielo con gravedad y rebote
    PARTICULA_PELOTAS_TROZO,      // bloque de cornisa que cae al vacio
    PARTICULA_PELOTAS_HUMO        // chimenea del refugio
};


struct ParticulaPelotas
{
    bool activa = false;
    TipoParticulaPelotas tipo = PARTICULA_PELOTAS_NIEVE;
    Vector3 posicion{};
    Vector3 velocidad{};
    float vida = 0.0f;
    float vidaMaxima = 1.0f;
    float tamano = 0.1f;
    Color color{ 255, 255, 255, 255 };
};


struct CopoNievePelotas
{
    Vector3 posicion{};
    float fase = 0.0f;
    float caida = 1.0f;
    float tamano = 0.06f;
};


struct OndaPelotas
{
    bool activa = false;
    Vector3 centro{};
    float vida = 0.0f;
    float vidaMaxima = 0.5f;
    float radioMaximo = 1.5f;
    Color color{ 255, 255, 255, 255 };
};


struct HuellaPelotas
{
    bool activa = false;
    Vector3 posicion{};
    float angulo = 0.0f;
    float vida = 0.0f;
    float ancho = 0.3f;
};


struct AvisoPelotas
{
    bool activo = false;
    Vector3 posicion{};
    float vida = 0.0f;
    int jugador = 0;
};


struct MinijuegoPelotas
{
    ResultadoMinijuego resultado;
    AudioJuego* audio = nullptr;

    EstadoJugadorPelotas estadosJugadores[
        MAX_JUGADORES_PRUEBA
    ];

    EstadoBotPelotas bots[MAX_JUGADORES_PRUEBA];

    BloquePrueba bloques[1];
    int cantidadBloques = 0;

    Camera3D camara{};

    FaseMinijuegoPelotas fase =
        FASE_PELOTAS_PREPARACION;

    float tiempoPreparacion = 3.0f;
    float tiempoRestante = 60.0f;
    float tiempoJugado = 0.0f;
    bool terminoPorTiempo = false;

    // Estado visual (solo lo lee Dibujar).
    float tiempoAnimacion = 0.0f;
    float esperaSonidoChoque = 0.0f;
    float esperaHumo = 0.0f;
    float esperaFestejo = 0.0f;
    int siguienteHuella = 0;
    ParticulaPelotas particulasNieve[MAX_PARTICULAS_PELOTAS];
    CopoNievePelotas copos[MAX_COPOS_PELOTAS];
    OndaPelotas ondas[MAX_ONDAS_PELOTAS];
    HuellaPelotas huellas[MAX_HUELLAS_PELOTAS];
    AvisoPelotas avisos[MAX_AVISOS_PELOTAS];

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
        bool mostrarDebug
    ) const;

    const ResultadoMinijuego& ObtenerResultado() const;
};
