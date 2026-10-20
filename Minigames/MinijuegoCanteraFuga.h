#pragma once

#include "Core/Participante.h"
#include "Core/ResultadoMinijuego.h"
#include "Minigames/TiposMinijuegos.h"
#include "Systems/Audio.h"


inline constexpr int MAX_ROCAS_CANTERA_FUGA = 12;
inline constexpr int MAX_PARTICULAS_CANTERA_FUGA = 240;
inline constexpr int MAX_DESTELLOS_CANTERA_FUGA = 6;


enum FaseCanteraFuga
{
    FASE_CANTERA_PREPARACION = 0,
    FASE_CANTERA_JUGANDO,
    FASE_CANTERA_TERMINADO
};


struct EstadoJugadorCanteraFuga
{
    float progresoMaximo = 0.0f;
    float tiempoAturdido = 0.0f;
    float impulsoLateral = 0.0f;
    int impactosRecibidos = 0;
};


struct RocaCanteraFuga
{
    bool activa = false;
    Vector3 posicion{};
    float velocidadX = 0.0f;
    float velocidadZ = 0.0f;
    float radio = 0.72f;

    // Solo visual: giro al rodar, forma facetada y polvo que levanta.
    float giro = 0.0f;
    float giroLateral = 0.0f;
    float temporizadorPolvo = 0.0f;
    int semilla = 0;
};


// Polvo, escombros, chispas y confeti (pool fijo, solo visual).
enum TipoParticulaCanteraFuga
{
    PARTICULA_CANTERA_POLVO = 0,
    PARTICULA_CANTERA_ESCOMBRO,
    PARTICULA_CANTERA_CHISPA,
    PARTICULA_CANTERA_CONFETI
};


struct ParticulaCanteraFuga
{
    bool activa = false;
    TipoParticulaCanteraFuga tipo = PARTICULA_CANTERA_POLVO;
    Vector3 posicion{};
    Vector3 velocidad{};
    float vida = 0.0f;
    float vidaMaxima = 0.0f;
    float tamano = 0.1f;
    Color color = { 200, 170, 128, 255 };
};


// Anillo que se expande donde una roca golpea a un escalador.
struct DestelloCanteraFuga
{
    bool activo = false;
    Vector3 posicion{};
    float tiempo = 0.0f;
};


struct MinijuegoCanteraFuga
{
    ResultadoMinijuego resultado{};
    AudioJuego* audio = nullptr;

    EstadoJugadorCanteraFuga estadosJugadores[MAX_PARTICIPANTES];
    RocaCanteraFuga rocas[MAX_ROCAS_CANTERA_FUGA];

    Camera3D camara{};
    FaseCanteraFuga fase = FASE_CANTERA_PREPARACION;

    int indiceSolo = -1;
    int impactosSolo = 0;

    float posicionTolvaX = 0.0f;
    float cooldownRoca = 0.0f;
    float tiempoDecisionBot = 0.0f;
    float tiempoPreparacion = 0.0f;
    float tiempoRestante = 0.0f;
    float tiempoAnimacion = 0.0f;

    // Estado solo visual / sonoro (no afecta reglas, tiempos ni IA).
    ParticulaCanteraFuga particulas[MAX_PARTICULAS_CANTERA_FUGA];
    DestelloCanteraFuga destellos[MAX_DESTELLOS_CANTERA_FUGA];
    unsigned int semillaVisual = 0x9E3779B9u;
    int contadorRocas = 0;
    float retrocesoTolva = 0.0f;
    float destelloOperador = 0.0f;
    float esperaSonidoLanzar = 0.0f;
    float esperaSonidoRoca = 0.0f;
    float temporizadorAmbiente = 0.0f;

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
