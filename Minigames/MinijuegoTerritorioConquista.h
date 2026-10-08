#pragma once

#include "Core/Participante.h"
#include "Core/ResultadoMinijuego.h"
#include "Minigames/TiposMinijuegos.h"
#include "Systems/Audio.h"


//==================================================
// TERRITORIO EN CONQUISTA
//==================================================
//
// Todos contra todos sobre una arena de baldosas. Caminar marca poco a poco
// la baldosa que se pisa; el ground pound (salto + salto en el aire) reclama
// de golpe un area y aturde a los rivales cercanos. Cada cierto tiempo un
// sector de la arena se sella y no puede reclamarse. Gana quien controle mas
// baldosas cuando se acaba el tiempo.
//==================================================

inline constexpr int COLUMNAS_TERRITORIO = 10;
inline constexpr int FILAS_TERRITORIO = 10;
inline constexpr int MAX_BALDOSAS_TERRITORIO =
    COLUMNAS_TERRITORIO * FILAS_TERRITORIO;
inline constexpr int MAX_BLOQUES_TERRITORIO = 8;
inline constexpr int MAX_PARTICULAS_TERRITORIO = MAX_PARTICULAS_TIERRA;
inline constexpr int MAX_ONDAS_TERRITORIO = 8;


enum FaseTerritorio
{
    FASE_TERRITORIO_PREPARACION = 0,
    FASE_TERRITORIO_JUGANDO,
    FASE_TERRITORIO_TERMINADO
};


// Banco tematico de este minijuego. Solo cambia la presentacion y el obstaculo
// central; las reglas son iguales en todos los temas.
enum TemaTerritorio
{
    TEMA_TERRITORIO_VOLCAN = 0,
    TEMA_TERRITORIO_CYBER,
    TEMA_TERRITORIO_DULCE,
    TEMA_TERRITORIO_JARDIN,
    TEMA_TERRITORIO_TOXICO,
    CANTIDAD_TEMAS_TERRITORIO
};


enum EstadoSectorTerritorio
{
    SECTOR_TERRITORIO_LIBRE = 0,
    SECTOR_TERRITORIO_AVISO,
    SECTOR_TERRITORIO_SELLADO
};


struct BaldosaTerritorio
{
    int dueno = -1;
    int candidato = -1;
    float progreso = 0.0f;
    float destello = 0.0f;
    bool obstaculo = false;
    bool tocadaEsteFrame = false;
};


struct EstadoJugadorTerritorio
{
    float recargaPound = 0.0f;
    int baldosas = 0;

    // Estado interno de la IA (solo se usa en bots o jugadores desconectados).
    int objetivoColumna = -1;
    int objetivoFila = -1;
    float tiempoReevaluar = 0.0f;
    int faseSalto = 0;
    float tiempoSalto = 0.0f;
    float recargaBot = 0.0f;
};


struct OndaTerritorio
{
    bool activa = false;
    Vector3 posicion{};
    float tiempo = 0.0f;
    Color color = WHITE;
};


struct MinijuegoTerritorioConquista
{
    ResultadoMinijuego resultado{};
    AudioJuego* audio = nullptr;   // lo asigna GestorMinijuegos; puede ser nullptr
    EstadoJugadorTerritorio estadosJugadores[MAX_PARTICIPANTES];
    BaldosaTerritorio baldosas[MAX_BALDOSAS_TERRITORIO];
    BloquePrueba bloques[MAX_BLOQUES_TERRITORIO];
    ParticulaTierra particulas[MAX_PARTICULAS_TERRITORIO];
    OndaTerritorio ondas[MAX_ONDAS_TERRITORIO];
    Color coloresJugadores[MAX_PARTICIPANTES];

    Camera3D camara{};
    FaseTerritorio fase = FASE_TERRITORIO_PREPARACION;
    TemaTerritorio tema = TEMA_TERRITORIO_VOLCAN;

    int cantidadBloques = 0;
    int totalBaldosasJugables = 0;

    EstadoSectorTerritorio estadoSector = SECTOR_TERRITORIO_LIBRE;
    int sectorActual = -1;
    int ultimoSector = -1;
    float tiempoSector = 0.0f;

    float tiempoPreparacion = 0.0f;
    float tiempoRestante = 0.0f;
    float tiempoAnimacion = 0.0f;

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
