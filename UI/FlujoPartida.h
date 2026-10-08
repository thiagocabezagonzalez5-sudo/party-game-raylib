#pragma once

#include "Board/CatalogoTableros.h"
#include "Core/Participante.h"
#include "Gameplay/PartidaTablero.h"
#include "Systems/Audio.h"

#include "raylib.h"


//==================================================
// FLUJO DE UNA PARTIDA DE TABLERO
//==================================================
//
// Pantallas que rodean a PartidaTablero:
//
//   jugadores -> SeleccionTablero -> ConfiguracionPartida -> IntroTablero
//   -> OrdenTurno -> partida -> ResultadosPartida
//
// Todas comparten las mismas reglas de entrada:
//   - Se manejan con cualquier humano activo y conectado (teclado o mando).
//   - Ignoran la entrada los primeros BLOQUEO_ENTRADA_FLUJO segundos, para
//     que la pulsacion que abrio la pantalla no la confirme sola.
//   - El stick/cruceta del mando solo cuenta al INICIAR el movimiento.
//   - Cancelar (ESC / B / Backspace / Shift) vuelve a la pantalla anterior.

inline constexpr float BLOQUEO_ENTRADA_FLUJO = 0.30f;

inline constexpr int CANTIDAD_OPCIONES_RONDAS = 3;
inline constexpr int OPCIONES_RONDAS_PARTIDA[CANTIDAD_OPCIONES_RONDAS] =
{
    5,
    10,
    15
};


//==================================================
// ENTRADA COMUN
//==================================================

struct EntradaFlujo
{
    bool izquierda = false;
    bool derecha = false;
    bool arriba = false;
    bool abajo = false;
    bool confirmar = false;
    bool cancelar = false;
};


// Une la entrada de todos los humanos activos y detecta el flanco
// de subida de los sticks. Llamar SIEMPRE cada frame, incluso durante el
// bloqueo de entrada, para que un stick mantenido no cuente al soltar el
// bloqueo.
struct LectorEntradaFlujo
{
    bool nivelPrevio[4] = {};

    void Reiniciar();

    EntradaFlujo Leer(
        const Participante participantes[],
        int cantidad
    );
};


//==================================================
// SELECCION DE TABLERO
//==================================================

struct SeleccionTablero
{
    int indice = 0;
    float tiempo = 0.0f;

    bool confirmado = false;
    bool volver = false;

    LectorEntradaFlujo lector;

    void Inicializar();

    void Actualizar(
        float deltaTime,
        const Participante participantes[],
        int cantidad,
        AudioJuego& audio
    );

    void Dibujar() const;

    IdTablero ObtenerIdElegido() const;
};


//==================================================
// CONFIGURACION DE PARTIDA
//==================================================

struct ConfiguracionPartida
{
    // Se conserva entre partidas: la revancha y la nueva partida
    // proponen la misma duracion.
    int opcionRondas = 0;

    IdTablero tablero = TABLERO_ISLA_ARBOLEDA;
    float tiempo = 0.0f;

    bool confirmado = false;
    bool volver = false;

    LectorEntradaFlujo lector;

    void Inicializar(
        IdTablero idTablero
    );

    void Actualizar(
        float deltaTime,
        const Participante participantes[],
        int cantidad,
        AudioJuego& audio
    );

    void Dibujar(
        const Participante participantes[],
        int cantidad
    ) const;

    int ObtenerRondas() const;
};


//==================================================
// INTRO DEL TABLERO
//==================================================

struct IntroTablero
{
    static constexpr float DURACION = 5.5f;

    IdTablero tablero = TABLERO_ISLA_ARBOLEDA;
    int rondas = 5;

    float tiempo = 0.0f;
    bool terminada = false;

    LectorEntradaFlujo lector;

    void Inicializar(
        IdTablero idTablero,
        int cantidadRondas
    );

    void Actualizar(
        float deltaTime,
        const Participante participantes[],
        int cantidad
    );

    void Dibujar() const;
};


//==================================================
// ORDEN DE TURNO
//==================================================
//
// El orden real ya lo decidio PartidaTablero al inicializarse. Esta
// pantalla lo presenta como una tirada de dados: reparte valores
// distintos de forma que el mayor dado coincida con el primer turno.

struct OrdenTurno
{
    int orden[MAX_PARTICIPANTES] = { -1, -1, -1, -1 };
    int dado[MAX_PARTICIPANTES] = {};
    bool dadoSonado[MAX_PARTICIPANTES] = {};

    // Participantes que tiran, por numero de jugador.
    int tiradores[MAX_PARTICIPANTES] = { -1, -1, -1, -1 };

    int cantidad = 0;
    float tiempo = 0.0f;
    bool revelado = false;
    bool terminado = false;

    IdTablero tablero = TABLERO_ISLA_ARBOLEDA;

    LectorEntradaFlujo lector;

    void Inicializar(
        const PartidaTablero& partida
    );

    void Actualizar(
        float deltaTime,
        const Participante participantes[],
        int cantidad,
        AudioJuego& audio
    );

    void Dibujar(
        const Participante participantes[]
    ) const;
};


//==================================================
// RESULTADOS DE LA PARTIDA
//==================================================

enum AccionResultados
{
    RESULTADOS_NINGUNA = 0,
    RESULTADOS_REVANCHA,
    RESULTADOS_NUEVA_PARTIDA,
    RESULTADOS_VOLVER_HUB
};


struct ResultadosPartida
{
    static constexpr int CANTIDAD_OPCIONES = 3;

    // Clasificacion: ranking[0] es el ganador. puesto[k] repite el valor
    // en caso de empate.
    int ranking[MAX_PARTICIPANTES] = { -1, -1, -1, -1 };
    int puesto[MAX_PARTICIPANTES] = {};
    int trofeos[MAX_PARTICIPANTES] = {};
    int monedas[MAX_PARTICIPANTES] = {};
    int cantidad = 0;

    IdTablero tablero = TABLERO_ISLA_ARBOLEDA;
    int rondas = 0;

    float tiempo = 0.0f;
    int sonidosReproducidos = 0;
    int opcion = 0;

    AccionResultados accion = RESULTADOS_NINGUNA;

    LectorEntradaFlujo lector;

    void Inicializar(
        const PartidaTablero& partida
    );

    void Actualizar(
        float deltaTime,
        const Participante participantes[],
        int cantidadParticipantes,
        AudioJuego& audio
    );

    void Dibujar(
        const Participante participantes[]
    ) const;
};
