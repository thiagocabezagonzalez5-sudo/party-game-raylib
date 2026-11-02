#pragma once

#include "Core/Participante.h"
#include "Core/ResultadoMinijuego.h"
#include "Minigames/TiposMinijuegos.h"
#include "Systems/Audio.h"


inline constexpr int CANTIDAD_BLOQUES_GLACIAR = 16;
inline constexpr int COLUMNAS_GLACIAR = 4;
inline constexpr int SIMBOLO_AURORA_GLACIAR = 7;
inline constexpr int MAX_PARTICULAS_GLACIAR = 200;


enum FaseParejasGlaciar
{
    FASE_GLACIAR_PREPARACION = 0,
    FASE_GLACIAR_ELEGIR,
    FASE_GLACIAR_REVELANDO,
    FASE_GLACIAR_CRUJIDO,
    FASE_GLACIAR_TERMINADO
};


enum EventoParejasGlaciar
{
    EVENTO_GLACIAR_NINGUNO = 0,
    EVENTO_GLACIAR_PAREJA,
    EVENTO_GLACIAR_AURORA,
    EVENTO_GLACIAR_FALLO,
    EVENTO_GLACIAR_CRUJE
};


struct BloqueGlaciar
{
    int simbolo = 0;
    int duenio = -1;
    bool revelado = false;
    bool emparejado = false;
    float derretido = 0.0f;
    float temblor = 0.0f;
};


struct EstadoJugadorParejasGlaciar
{
    int puntos = 0;
    int parejas = 0;
    int precisionMemoria = 70;
};


// Particulas de efecto (solo visual): esquirlas de hielo, vapor del
// deshielo, chispas de pareja y confeti final.
enum TipoParticulaGlaciar
{
    PARTICULA_GLACIAR_ESQUIRLA = 0,
    PARTICULA_GLACIAR_VAPOR,
    PARTICULA_GLACIAR_CHISPA,
    PARTICULA_GLACIAR_CONFETI
};


struct ParticulaGlaciar
{
    bool activa = false;
    int tipo = PARTICULA_GLACIAR_ESQUIRLA;
    Vector3 posicion{};
    Vector3 velocidad{};
    float vida = 0.0f;
    float vidaMaxima = 1.0f;
    float tamano = 0.1f;
    float giro = 0.0f;
    Color color{};
};


struct MinijuegoParejasGlaciar
{
    ResultadoMinijuego resultado{};
    AudioJuego* audio = nullptr;
    EstadoJugadorParejasGlaciar estadosJugadores[MAX_PARTICIPANTES];
    BloqueGlaciar bloques[CANTIDAD_BLOQUES_GLACIAR];
    int memoriaBots[MAX_PARTICIPANTES][CANTIDAD_BLOQUES_GLACIAR]{};
    float posicionTempanoX[MAX_PARTICIPANTES]{};
    float posicionTempanoZ[MAX_PARTICIPANTES]{};

    Camera3D camara{};
    FaseParejasGlaciar fase = FASE_GLACIAR_PREPARACION;
    EventoParejasGlaciar ultimoEvento = EVENTO_GLACIAR_NINGUNO;

    int turno = -1;
    int ultimoJugador = -1;
    int cursor = 5;
    int primera = -1;
    int segunda = -1;
    int fallosSeguidos = 0;
    int direccionPrevia = 0;
    int botObjetivo = -1;
    int crujidoA = -1;
    int crujidoB = -1;
    bool aciertoActual = false;
    bool accionPrevia = false;
    bool crujidoIntercambiado = false;

    float repeticionCursor = 0.0f;
    float botTemporizador = 0.0f;
    float tiempoTurno = 0.0f;
    float tiempoPausa = 0.0f;
    float tiempoRestante = 0.0f;
    float tiempoPreparacion = 0.0f;
    float tiempoAnimacion = 0.0f;

    // Estado solo visual: lo escribe Actualizar despues de la logica
    // comparando el antes y el despues; el azar sale de semillaVisual.
    ParticulaGlaciar particulas[MAX_PARTICULAS_GLACIAR];
    unsigned int semillaVisual = 1u;
    float destelloBloque[CANTIDAD_BLOQUES_GLACIAR] = {};
    float destelloJugador[MAX_PARTICIPANTES] = {};
    float brilloAurora = 0.0f;
    float tiempoAnuncioTurno = 99.0f;
    float tiempoSuma = 0.0f;
    int sumaVisual = 0;
    int jugadorSuma = -1;
    float tiempoFinVisual = 0.0f;
    float confetiPendiente = 0.0f;
    float sacudidaBloque[CANTIDAD_BLOQUES_GLACIAR] = {};
    float tiempoCrujidoVisual = 99.0f;
    bool turnoRepetido = false;
    Color colorJugador[MAX_PARTICIPANTES]{};

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
