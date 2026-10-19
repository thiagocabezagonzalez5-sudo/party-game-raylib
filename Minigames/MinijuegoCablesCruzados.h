#pragma once

#include "Core/Participante.h"
#include "Core/ResultadoMinijuego.h"
#include "Minigames/TiposMinijuegos.h"
#include "Systems/Audio.h"


//==================================================
// CABLES CRUZADOS
//==================================================
//
// Duelo 1 vs 1 de puzzle de circuitos sobre un banco de trabajo. Cada
// jugador tiene su propio panel con las MISMAS piezas de cable (rectas,
// esquinas, T, cruces y algun hueco) en el MISMO desorden: la pila de la
// izquierda tiene que llegar a la bombilla de la derecha. Se mueve el cursor
// con el movimiento y se gira la pieza con golpear (horario) o saltar
// (antihorario); girar no cuesta nada, solo tiempo. Cuando la corriente
// llega a la bombilla se enciende y esa ronda es tuya.
//
// Mejor de 3 rondas (gana quien llegue a 2). Cada ronda trae un circuito
// nuevo (4x4, 5x4 y 5x5), igual para los dos y siempre resoluble. Si se
// acaba el tiempo de la ronda gana quien tenga la cadena energizada mas
// larga, luego mas piezas con corriente, luego quien llego antes a ese
// avance y, por ultimo, una moneda sorteada con la semilla de la partida:
// nunca hay empate ni ventaja de lado.
//
// Las funciones puras de tablero (generar, energizar, resolver) estan
// declaradas aqui para poder verificarlas sin ventana.
//==================================================


inline constexpr int MAX_COLUMNAS_CABLES = 5;
inline constexpr int MAX_FILAS_CABLES = 5;
inline constexpr int MAX_CELDAS_CABLES = MAX_COLUMNAS_CABLES * MAX_FILAS_CABLES;
inline constexpr int MAX_SOLUCIONES_CABLES = 3;
inline constexpr int RONDAS_PARA_GANAR_CABLES = 2;
inline constexpr int MAX_RONDAS_CABLES = 3;
inline constexpr int MAX_PARTICULAS_CABLES = 220;


// Lados de una pieza: bits de la mascara. Giro horario: N -> E -> S -> O.
inline constexpr unsigned char CABLE_NORTE = 1;
inline constexpr unsigned char CABLE_ESTE = 2;
inline constexpr unsigned char CABLE_SUR = 4;
inline constexpr unsigned char CABLE_OESTE = 8;


//==================================================
// TABLERO (logica pura)
//==================================================

struct TableroCables
{
    int columnas = 4;
    int filas = 4;
    int origen = 0;                  // pila (fija, sale por el este)
    int bombilla = 0;                // bombilla (fija, entra por el oeste)

    unsigned char base[MAX_CELDAS_CABLES]{};   // mascara con giro 0
    unsigned char giro[MAX_CELDAS_CABLES]{};   // giro actual 0..3
    bool fija[MAX_CELDAS_CABLES]{};

    int camino[MAX_CELDAS_CABLES]{};           // camino de referencia
    int largoCamino = 0;                       // sin pila ni bombilla
};


struct SolucionCables
{
    int cantidad = 0;                          // piezas del camino
    int celdas[MAX_CELDAS_CABLES]{};
    unsigned char giro[MAX_CELDAS_CABLES]{};   // giro objetivo de cada una
    int clics = 0;                             // giros minimos desde el estado actual
};


// Mascara de una pieza ya girada (giro horario, 0..3 o mas).
unsigned char MascaraGiradaCables(unsigned char base, int giro);

// Crea un tablero siempre resoluble y desordenado (sin bombilla encendida y
// con al menos clicsMin giros para resolverlo). Determinista por semilla.
void GenerarTableroCables(
    unsigned int semilla,
    int columnas,
    int filas,
    int largoMin,
    int largoMax,
    int clicsMin,
    TableroCables& tablero
);

// BFS desde la pila: distancia por celda (-1 sin corriente). Devuelve la
// cantidad de celdas energizadas y, si se pide, la cadena mas larga.
int CalcularEnergiaCables(
    const TableroCables& tablero,
    int distancia[],
    int* profundidad = nullptr
);

bool BombillaEncendidaCables(const TableroCables& tablero);

// Mejores caminos pila -> bombilla (menos giros primero). Devuelve cuantos.
int ResolverTableroCables(
    const TableroCables& tablero,
    SolucionCables soluciones[],
    int maximo
);

void AplicarSolucionCables(
    TableroCables& tablero,
    const SolucionCables& solucion
);


//==================================================
// MINIJUEGO
//==================================================

enum FaseCablesCruzados
{
    FASE_CABLES_PREPARACION = 0,
    FASE_CABLES_INTRO_RONDA,
    FASE_CABLES_JUGANDO,
    FASE_CABLES_FIN_RONDA,
    FASE_CABLES_TERMINADO
};


struct ParticulaCables
{
    bool activa = false;
    Vector3 posicion{};
    Vector3 velocidad{};
    float vida = 0.0f;
    float vidaMaxima = 0.0f;
    float tamano = 0.06f;
    Color color = WHITE;
};


// Bot que resuelve el puzzle como una persona: mira un rato, sigue un camino
// celda a celda, reacciona, duda, y a veces gira de mas y lo corrige.
struct BotCables
{
    float torpeza = 0.5f;          // 0 rapido y preciso .. 1 lento y errante
    float ritmo = 0.3f;            // segundos por accion
    float planificacion = 1.2f;    // mirada inicial
    float probError = 0.08f;
    float probDuda = 0.1f;

    float espera = 0.0f;
    bool planificado = false;
    int rondaPlan = -1;
    SolucionCables plan;
    int paso = 0;
    int celdaReaccion = -1;
    bool reaccionado = false;
};


struct LadoCables
{
    int participante = -1;
    TableroCables tablero;

    int giro[MAX_CELDAS_CABLES]{};             // clics acumulados (animacion)
    float giroVisual[MAX_CELDAS_CABLES]{};     // en vueltas de 90 grados
    float giroVelocidad[MAX_CELDAS_CABLES]{};
    float destello[MAX_CELDAS_CABLES]{};

    int distancia[MAX_CELDAS_CABLES]{};
    int energizadas = 0;
    int profundidad = 0;
    bool encendida = false;

    int cursor = 0;
    int dirPrevia = -1;
    float repeticion = 0.0f;

    int rondasGanadas = 0;
    float brilloBombilla = 0.0f;
    float onda = -1.0f;                        // frente de la descarga final
    float tiempoUltimoAvance = 0.0f;
    float sacudida = 0.0f;                     // cursor sobre pieza fija
    int clics = 0;

    BotCables bot;
};


struct MinijuegoCablesCruzados
{
    ResultadoMinijuego resultado{};
    AudioJuego* audio = nullptr;   // lo asigna GestorMinijuegos; puede ser nullptr

    LadoCables lados[2];
    ParticulaCables particulas[MAX_PARTICULAS_CABLES];

    unsigned int semilla = 1;
    int ronda = 0;                 // 0..2
    int ladoGanadorRonda = -1;
    bool rondaPorTiempo = false;
    int monedaDesempate = 0;       // 0/1 sorteada por ronda
    int ganadorPartida = -1;
    bool partidaValida = false;

    FaseCablesCruzados fase = FASE_CABLES_PREPARACION;
    float tiempoPreparacion = 0.0f;
    float tiempoFase = 0.0f;       // INTRO_RONDA y FIN_RONDA
    float tiempoRonda = 0.0f;      // transcurrido en la ronda
    float tiempoRestante = 0.0f;
    float duracionRonda = 20.0f;
    float tiempoAparicion = 0.0f;  // piezas que aparecen al empezar la ronda
    float tiempoAnimacion = 0.0f;

    Camera3D camara{};

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
