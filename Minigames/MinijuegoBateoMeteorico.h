#pragma once

#include "Core/Participante.h"
#include "Core/ResultadoMinijuego.h"
#include "Minigames/TiposMinijuegos.h"
#include "Systems/Audio.h"


//==================================================
// BATEO METEORICO
//==================================================
//
// Todos contra todos (2-4) en una plataforma en orbita junto a un planeta
// anillado. Cada
// jugador tiene su carril frente a un canon que le lanza 5 meteoritos con
// velocidad y efecto variables. Pulsar la accion balancea el bate: cuanto
// mas cerca del punto dulce, mas lejos vuela el meteorito y mejor anillo
// puntuado alcanza. Cada jugador recibe un meteorito dorado (vale doble) y
// uno rojo inestable que NO hay que golpear (resta 50 puntos).
//==================================================

inline constexpr int LANZAMIENTOS_BATEO = 5;
inline constexpr int MAX_CHISPAS_BATEO = 220;
inline constexpr int MAX_AVISOS_BATEO = 16;


enum FaseBateoMeteorico
{
    FASE_BATEO_PREPARACION = 0,
    FASE_BATEO_JUGANDO,
    FASE_BATEO_TERMINADO
};


enum EtapaRondaBateo
{
    ETAPA_BATEO_AVISO = 0,
    ETAPA_BATEO_VUELO,
    ETAPA_BATEO_PAUSA
};


enum TipoMeteoritoBateo
{
    METEORITO_NORMAL = 0,
    METEORITO_DORADO,
    METEORITO_ROJO
};


enum EstadoLanzamientoBateo
{
    LANZAMIENTO_PENDIENTE = 0,
    LANZAMIENTO_EN_VUELO,
    LANZAMIENTO_GOLPEADO,
    LANZAMIENTO_PASADO,
    LANZAMIENTO_ATERRIZADO,
    LANZAMIENTO_EXPLOTADO
};


enum MensajeBateo
{
    MENSAJE_BATEO_NINGUNO = 0,
    MENSAJE_BATEO_PERFECTO,
    MENSAJE_BATEO_BUENO,
    MENSAJE_BATEO_REGULAR,
    MENSAJE_BATEO_FLOJO,
    MENSAJE_BATEO_FALLO,
    MENSAJE_BATEO_SIN_GOLPE,
    MENSAJE_BATEO_ROJO,
    MENSAJE_BATEO_EVITADO
};


// Parametros de un lanzamiento (iguales para todos los jugadores).
struct LanzamientoBateo
{
    float duracion = 1.2f;     // segundos del canon hasta el punto dulce
    float curva = 0.0f;        // desvio lateral durante el vuelo
    float exponente = 1.0f;    // >1 = lento al inicio y rapido al final
};


struct EstadoJugadorBateo
{
    int puntos = 0;
    int mejorGolpe = 0;        // mejor calidad (0-1000), para desempates
    int golpesPerfectos = 0;
    TipoMeteoritoBateo tipos[LANZAMIENTOS_BATEO]{};

    EstadoLanzamientoBateo estado = LANZAMIENTO_PENDIENTE;
    TipoMeteoritoBateo tipoActual = METEORITO_NORMAL;
    float reloj = 0.0f;
    bool yaGolpeo = false;

    // Vuelo del meteorito golpeado.
    float tiempoVuelo = 0.0f;
    float duracionVuelo = 1.0f;
    float distancia = 0.0f;
    float desvio = 0.0f;
    float calidad = 0.0f;
    float tiempoPostEvento = 0.0f;
    Vector3 puntoGolpe{};
    Vector3 puntoAterrizaje{};

    float tiempoSwing = 0.0f;
    float tiempoImpacto = 0.0f;

    // Mensaje y puntos del ultimo lanzamiento.
    MensajeBateo mensaje = MENSAJE_BATEO_NINGUNO;
    int puntosMensaje = 0;
    float tiempoMensaje = 0.0f;

    // IA (bots y humanos desconectados).
    float sigmaTiming = 0.08f;
    float instanteGolpeBot = 0.0f;
    bool botGolpeara = true;

    int carril = -1;
    float carrilX = 0.0f;
};


// Chispa, esquirla, humo o confeti (solo visual).
struct ChispaBateo
{
    bool activa = false;
    Vector3 posicion{};
    Vector3 velocidad{};
    Color color{};
    float vida = 0.0f;
    float vidaMaxima = 1.0f;
    float tamano = 0.1f;
    float gravedad = 0.0f;
    float giro = 0.0f;
    bool brilla = false;
};


// Rotulo que sube sobre el mundo: «+60», «¡PERFECTO!».
struct AvisoBateo
{
    bool activo = false;
    Vector3 posicion{};
    float vida = 0.0f;
    float vidaMaxima = 1.0f;
    char texto[24] = "";
    Color color{};
    bool grande = false;
};


struct MinijuegoBateoMeteorico
{
    ResultadoMinijuego resultado{};
    AudioJuego* audio = nullptr;   // lo asigna GestorMinijuegos; puede ser nullptr

    EstadoJugadorBateo estadosJugadores[MAX_PARTICIPANTES];
    LanzamientoBateo lanzamientos[LANZAMIENTOS_BATEO];

    Camera3D camara{};
    FaseBateoMeteorico fase = FASE_BATEO_PREPARACION;
    EtapaRondaBateo etapa = ETAPA_BATEO_AVISO;

    int ronda = 0;
    int cantidadCarriles = 0;

    float tiempoPreparacion = 0.0f;
    float tiempoRestante = 0.0f;
    float tiempoEtapa = 0.0f;
    float tiempoAnimacion = 0.0f;

    // Solo visual: se actualiza en Actualizar comparando el estado antes y
    // despues de cada paso; el azar sale de semillaVisual.
    ChispaBateo chispas[MAX_CHISPAS_BATEO];
    AvisoBateo avisos[MAX_AVISOS_BATEO];
    unsigned int semillaVisual = 1u;
    float tiempoVisual = 0.0f;
    float tiempoAnuncioInicio = -1.0f;
    float tiempoFin = -1.0f;
    float destelloJugador[MAX_PARTICIPANTES] = {};

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

    // Reglas de la partida (Actualizar le suma los efectos visuales).
    void ActualizarPartida(
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
