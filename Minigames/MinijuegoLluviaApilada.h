#pragma once

#include "Core/Participante.h"
#include "Core/ResultadoMinijuego.h"
#include "Minigames/TiposMinijuegos.h"
#include "Systems/Audio.h"


inline constexpr int MAX_PIEZAS_CAYENDO_LLUVIA = 24;
inline constexpr int MAX_PIEZAS_SUELTAS_LLUVIA = 24;
inline constexpr int MAX_PILA_LLUVIA = 12;
inline constexpr int MAX_OBSTACULOS_LLUVIA = 6;
inline constexpr int MAX_BLOQUES_LLUVIA = 12;
inline constexpr int MAX_PARTICULAS_VISUALES_LLUVIA = 200;
inline constexpr int MAX_MARCAS_LLUVIA = 10;


enum FaseLluviaApilada
{
    FASE_LLUVIA_PREPARACION = 0,
    FASE_LLUVIA_JUGANDO,
    FASE_LLUVIA_TERMINADO
};


enum TipoPiezaLluvia
{
    PIEZA_LLUVIA_NORMAL = 0,
    PIEZA_LLUVIA_RARA,
    PIEZA_LLUVIA_PELIGROSA
};


// Pieza que todavia cae. (x, z) es el punto de aterrizaje: ahi se dibuja
// la sombra/marcador que crece a medida que avanza "tiempo".
struct PiezaCayendoLluvia
{
    bool activa = false;
    TipoPiezaLluvia tipo = PIEZA_LLUVIA_NORMAL;
    float x = 0.0f;
    float z = 0.0f;
    float tiempo = 0.0f;
    float duracion = 1.6f;
    float altura = 0.0f;
};


// Pieza que un golpe hizo saltar de una pila. Cualquiera puede recogerla
// (salvo su duenio durante un instante).
struct PiezaSueltaLluvia
{
    bool activa = false;
    bool rara = false;
    Vector3 posicion{};
    Vector3 velocidad{};
    int jugadorBloqueado = -1;
    float tiempoBloqueo = 0.0f;
    float tiempoVida = 0.0f;
};


// Dato de GAMEPLAY de un obstaculo (columna, roca, caja...). La colision
// sale de un BloquePrueba con las mismas medidas; el dibujo es aparte.
struct ObstaculoLluvia
{
    Vector3 posicion{};
    Vector3 tamano{};
    bool redondo = false;
};


struct EstadoJugadorLluvia
{
    int pila = 0;
    bool rara[MAX_PILA_LLUVIA]{};
    int aseguradas = 0;

    // Inercia: direccion suavizada. Cuanta mas pila, mas tarda en cambiar.
    float direccionX = 0.0f;
    float direccionZ = 0.0f;

    float tiempoAvisoDeposito = 0.0f;

    // Estado de la IA de bots.
    float tiempoDecision = 0.0f;
    float objetivoX = 0.0f;
    float objetivoZ = 0.0f;
    bool quiereDepositar = false;
    bool acosaRival = false;
    bool ignoraPeligro = false;
    int umbralDeposito = 5;
};


// Observatorio (solo visual): esquirlas, chispas, brasas y confeti.
struct ParticulaVisualLluvia
{
    bool activa = false;
    bool brillo = false;            // se dibuja en la pasada aditiva
    Vector3 posicion{};
    Vector3 velocidad{};
    Color color{};
    float vida = 0.0f;
    float vidaMaxima = 1.0f;
    float tamano = 0.1f;
    float gravedad = 0.0f;
    float giro = 0.0f;
};


// Quemadura que deja un meteoro en las losas: se enfria y se borra.
struct MarcaImpactoLluvia
{
    float x = 0.0f;
    float z = 0.0f;
    float vida = 0.0f;
    float giro = 0.0f;
};


struct MinijuegoLluviaApilada
{
    ResultadoMinijuego resultado{};
    AudioJuego* audio = nullptr;   // lo asigna GestorMinijuegos; puede ser nullptr
    EstadoJugadorLluvia estadosJugadores[MAX_PARTICIPANTES];
    PiezaCayendoLluvia piezasCayendo[MAX_PIEZAS_CAYENDO_LLUVIA];
    PiezaSueltaLluvia piezasSueltas[MAX_PIEZAS_SUELTAS_LLUVIA];
    ObstaculoLluvia obstaculos[MAX_OBSTACULOS_LLUVIA];
    BloquePrueba bloques[MAX_BLOQUES_LLUVIA];
    ParticulaTierra particulas[MAX_PARTICULAS_TIERRA];

    Camera3D camara{};
    FaseLluviaApilada fase = FASE_LLUVIA_PREPARACION;

    int tema = 0;
    int cantidadObstaculos = 0;
    int cantidadBloques = 0;

    float tiempoPreparacion = 0.0f;
    float tiempoRestante = 0.0f;
    float tiempoAnimacion = 0.0f;
    float tiempoSiguientePieza = 0.0f;
    float tiempoJugado = 0.0f;

    // Solo visual: nada de esto toca la logica ni consume el
    // GetRandomValue de los bots.
    unsigned int semillaVisual = 2654435761u;
    bool caidaVista[MAX_PIEZAS_CAYENDO_LLUVIA];
    int pilaVista[MAX_PARTICIPANTES];
    int aseguradasVista[MAX_PARTICIPANTES];
    float destelloBase[MAX_PARTICIPANTES];
    int sumaBase[MAX_PARTICIPANTES];
    float golpePila[MAX_PARTICIPANTES];
    float esperaSonidoImpacto = 0.0f;
    float tiempoFin = 0.0f;
    bool finVisto = false;
    ParticulaVisualLluvia particulasVisuales[MAX_PARTICULAS_VISUALES_LLUVIA];
    MarcaImpactoLluvia marcas[MAX_MARCAS_LLUVIA];

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
