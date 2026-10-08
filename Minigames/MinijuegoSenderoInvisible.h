#pragma once

#include "Core/Participante.h"
#include "Core/ResultadoMinijuego.h"
#include "Minigames/TiposMinijuegos.h"
#include "Systems/Audio.h"


//==================================================
// SENDERO INVISIBLE
//==================================================
//
// Todos contra todos (2-4) en un cementerio nocturno. Cada jugador tiene
// su propia cuadricula de losas (6 x 10) sobre un abismo de niebla, todas
// con el MISMO trazado seguro oculto. Al inicio unos fuegos fatuos
// recorren la ruta segura durante 3 s para memorizarla. Pisar una losa
// falsa la desmorona: caes y reapareces en la ultima losa segura. Cada
// caida marca con una grieta esa losa falsa en TODAS las cuadriculas. A
// mitad del recorrido hay un farol que ilumina las 2 losas siguientes 3 s.
// Gana quien llega primero a la ultima fila; tope de 60 s: gana el que
// mas ruta segura haya recorrido (empate si van exactamente igual).
//==================================================

inline constexpr int FILAS_SENDERO = 10;
inline constexpr int COLUMNAS_SENDERO = 6;
inline constexpr int LOSAS_SENDERO = FILAS_SENDERO * COLUMNAS_SENDERO;
inline constexpr int MAX_RUTA_SENDERO = 40;
inline constexpr int MAX_PARTICULAS_SENDERO = MAX_PARTICULAS_TIERRA;


enum FaseSenderoInvisible
{
    FASE_SENDERO_PREPARACION = 0,
    FASE_SENDERO_MEMORIZAR,
    FASE_SENDERO_JUGANDO,
    FASE_SENDERO_TERMINADO
};


enum EstadoLosaSendero
{
    LOSA_SENDERO_FIRME = 0,
    LOSA_SENDERO_TEMBLANDO,
    LOSA_SENDERO_CAIDA
};


struct LosaSendero
{
    EstadoLosaSendero estado = LOSA_SENDERO_FIRME;
    float tiempo = 0.0f;
};


struct EstadoJugadorSendero
{
    int carril = -1;
    int progreso = 0;              // mayor tramo seguro alcanzado
    int caidas = 0;
    bool llego = false;
    float tiempoFin = 0.0f;        // tiempo restante al llegar (mayor = antes)

    bool farolRecogido = false;
    float farolTiempo = 0.0f;

    int ultimaFila = 0;
    int ultimaColumna = 0;

    // IA (bots y humanos desconectados).
    float precision = 0.8f;
    bool conocimiento[LOSAS_SENDERO]{};
    int objetivoFila = -1;
    int objetivoColumna = -1;
    int previaFila = -1;
    int previaColumna = -1;
    float espera = 0.0f;
};


struct MinijuegoSenderoInvisible
{
    ResultadoMinijuego resultado{};
    AudioJuego* audio = nullptr;   // lo asigna GestorMinijuegos; puede ser nullptr

    EstadoJugadorSendero estadosJugadores[MAX_PARTICIPANTES];
    int jugadorDeCarril[MAX_PARTICIPANTES]{};
    int cantidadCarriles = 0;

    BloquePrueba bloques[MAX_PARTICIPANTES][LOSAS_SENDERO];
    LosaSendero losas[MAX_PARTICIPANTES][LOSAS_SENDERO];
    ParticulaTierra particulas[MAX_PARTICULAS_SENDERO];

    // Trazado compartido.
    bool segura[LOSAS_SENDERO]{};
    bool grieta[LOSAS_SENDERO]{};
    int indiceProgreso[LOSAS_SENDERO]{};
    int ruta[MAX_RUTA_SENDERO]{};
    int cantidadRuta = 0;
    int secuencia[MAX_RUTA_SENDERO + 2]{};
    int cantidadSecuencia = 0;
    int indiceSecuencia[LOSAS_SENDERO]{};
    int farolIndice = 0;
    int columnaInicio = 0;
    int columnaMeta = 0;

    bool empate = false;
    bool partidaValida = false;

    Camera3D camara{};
    FaseSenderoInvisible fase = FASE_SENDERO_PREPARACION;

    float tiempoPreparacion = 0.0f;
    float tiempoMemorizar = 0.0f;
    float tiempoRestante = 0.0f;
    float tiempoAnimacion = 0.0f;
    float tiempoYa = 0.0f;

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
