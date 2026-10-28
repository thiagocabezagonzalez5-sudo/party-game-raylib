#pragma once

#include "Core/Participante.h"
#include "Core/ResultadoMinijuego.h"
#include "Minigames/TiposMinijuegos.h"
#include "Systems/Audio.h"


//==================================================
// LABERINTO INCLINADO
//==================================================
//
// Todos contra todos (2-4). Cada participante tiene su propia losa de piedra
// con el mismo laberinto, dentro de un templo antiguo. El jugador no camina:
// inclina su losa y una esfera de jade rueda por ella. Hay agujeros (la
// esfera vuelve al ultimo checkpoint), trampas de dardos que empujan la
// esfera por un pasillo y un altar dorado como meta. Gana quien llegue
// primero; si se acaba el tiempo se ordena por progreso por el camino.
//==================================================

inline constexpr int COLUMNAS_LABERINTO = 11;
inline constexpr int FILAS_LABERINTO = 11;
inline constexpr int MAX_PUNTOS_CONTROL_LABERINTO = 2;
inline constexpr int MAX_TRAMPAS_LABERINTO = 2;
inline constexpr int CANTIDAD_DISENOS_LABERINTO = 3;


enum FaseLaberintoInclinado
{
    FASE_LABERINTO_PREPARACION = 0,
    FASE_LABERINTO_JUGANDO,
    FASE_LABERINTO_TERMINADO
};


enum TipoCeldaLaberinto
{
    CELDA_LABERINTO_LIBRE = 0,
    CELDA_LABERINTO_MURO,
    CELDA_LABERINTO_AGUJERO
};


// Region rectangular de celdas (inclusive) donde los dardos empujan la
// esfera con una direccion fija. Es parte del diseno logico del laberinto.
struct TrampaDardosLaberinto
{
    int columnaInicio = 0;
    int filaInicio = 0;
    int columnaFin = 0;
    int filaFin = 0;
    float direccionX = 1.0f;
    float direccionZ = 0.0f;
    float desfase = 0.0f;
};


struct EstadoJugadorLaberinto
{
    // Posicion en coordenadas de rejilla: la celda (c, r) ocupa [c, c+1].
    float x = 0.0f;
    float z = 0.0f;
    float velocidadX = 0.0f;
    float velocidadZ = 0.0f;

    // Inclinacion actual y deseada, de -1 a 1 por eje.
    float inclinacionX = 0.0f;
    float inclinacionZ = 0.0f;
    float objetivoX = 0.0f;
    float objetivoZ = 0.0f;

    int tablero = -1;
    int puntosControl = 0;
    float puntoRespawnX = 0.0f;
    float puntoRespawnZ = 0.0f;

    bool llego = false;
    float tiempoLlegada = 0.0f;
    float progreso = 0.0f;

    // Mayor que cero mientras la esfera se hunde en un agujero.
    float tiempoCaida = 0.0f;
    float agujeroX = 0.0f;
    float agujeroZ = 0.0f;
    int caidas = 0;

    // Estado interno de la IA (bots y humanos desconectados).
    float habilidad = 1.0f;
    float ruidoX = 0.0f;
    float ruidoZ = 0.0f;
    float tiempoRuido = 0.0f;
    float mejorProgresoBot = 0.0f;
    float tiempoSinProgreso = 0.0f;
    float tiempoSacudida = 0.0f;
    float sacudidaX = 0.0f;
    float sacudidaZ = 0.0f;
};


// ---- Solo visual (no interviene en la partida) ----

inline constexpr int MAX_PARTICULAS_LABERINTO = 320;


struct ParticulaLaberinto
{
    bool activa = false;
    bool brillo = false;            // se dibuja con mezcla aditiva
    Vector3 posicion{};
    Vector3 velocidad{};
    float vida = 0.0f;
    float vidaMaxima = 0.0f;
    float tamano = 0.1f;
    float gravedad = 0.0f;
    Color color{};
};


// Lo que el dibujo necesita recordar de cada jugador para detectar
// eventos (checkpoint, caida, llegada, golpe) y animar su respuesta.
struct VisualJugadorLaberinto
{
    int puntosControlPrevios = 0;
    int caidasPrevias = 0;
    bool llegoPrevio = false;
    float velocidadXPrevia = 0.0f;
    float velocidadZPrevia = 0.0f;

    float destelloControl = 0.0f;
    float destelloCaida = 0.0f;
    float destelloMeta = 0.0f;
    float destelloGolpe = 0.0f;

    float tiempoTexto = 0.0f;       // texto flotante sobre la losa
    int tipoTexto = 0;              // 1 control, 2 pozo
};


struct MinijuegoLaberintoInclinado
{
    ResultadoMinijuego resultado{};
    AudioJuego* audio = nullptr;   // lo asigna GestorMinijuegos; puede ser nullptr

    // Datos LOGICOS del laberinto (iguales para todos los tableros).
    TipoCeldaLaberinto celdas[FILAS_LABERINTO][COLUMNAS_LABERINTO];
    int distanciaMeta[FILAS_LABERINTO][COLUMNAS_LABERINTO];
    int distanciaInicio = 1;
    int inicioColumna = 1;
    int inicioFila = 1;
    int metaColumna = 1;
    int metaFila = 1;
    int controlColumna[MAX_PUNTOS_CONTROL_LABERINTO];
    int controlFila[MAX_PUNTOS_CONTROL_LABERINTO];
    TrampaDardosLaberinto trampas[MAX_TRAMPAS_LABERINTO];
    int cantidadTrampas = 0;
    bool pulsoPrevio[MAX_TRAMPAS_LABERINTO];

    EstadoJugadorLaberinto estadosJugadores[MAX_PARTICIPANTES];

    // Disposicion de las losas en la escena.
    Vector3 centrosTableros[MAX_PARTICIPANTES];
    int cantidadTableros = 0;

    Camera3D camara{};
    FaseLaberintoInclinado fase = FASE_LABERINTO_PREPARACION;

    int disenoActual = 0;
    int ultimoDiseno = -1;
    int llegadas = 0;
    bool plazoAcortado = false;

    float tiempoPreparacion = 0.0f;
    float tiempoRestante = 0.0f;
    float tiempoJuego = 0.0f;
    float tiempoAnimacion = 0.0f;
    float enfriamientoImpacto = 0.0f;

    // Estado visual: particulas, destellos, anuncios. Azar propio.
    ParticulaLaberinto particulas[MAX_PARTICULAS_LABERINTO];
    VisualJugadorLaberinto visuales[MAX_PARTICIPANTES];
    bool pulsoVisualPrevio[MAX_TRAMPAS_LABERINTO];
    unsigned int semillaVisual = 0x2F6E2B1u;
    float relojVisual = 0.0f;
    float tiempoAnuncio = -1.0f;
    int jugadorAnuncio = -1;
    bool anuncioConPlazo = false;
    bool confetiLanzado = false;

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
