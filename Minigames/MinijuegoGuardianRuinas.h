#pragma once

#include "Core/Participante.h"
#include "Core/ResultadoMinijuego.h"
#include "Minigames/TiposMinijuegos.h"
#include "Systems/Audio.h"


//==================================================
// GUARDIAN DE RUINAS
//==================================================
//
// 1 contra 3 (con 3 jugadores 1 vs 2 y con 2, 1 vs 1) en la plaza de una
// ciudad antigua cubierta de vegetacion. El solitario es el guardian del
// portal de piedra: se desplaza lateralmente frente a el con un escudo y
// puede embestir (accion) para devolver los orbes con mas fuerza. Los
// atacantes se mueven por la plaza y lanzan orbes de energia hacia el
// portal (accion; izquierda/derecha al lanzar desvian el tiro, recarga
// 1.2 s). Los orbes rebotan en las columnas y paredes. Los atacantes
// ganan si meten 10 orbes (6 con dos atacantes, 3 con uno) antes de 45 s;
// si no, gana el guardian. Cada 15 s cae una columna (con aviso previo) y
// cambian los angulos de tiro.
//==================================================

inline constexpr int MAX_ORBES_GUARDIAN = 14;
inline constexpr int MAX_COLUMNAS_GUARDIAN = 6;
inline constexpr int MAX_PARTICULAS_GUARDIAN = MAX_PARTICULAS_TIERRA;


enum FaseGuardianRuinas
{
    FASE_GUARDIAN_PREPARACION = 0,
    FASE_GUARDIAN_JUGANDO,
    FASE_GUARDIAN_TERMINADO
};


enum EstadoColumnaGuardian
{
    COLUMNA_GUARDIAN_ENTERA = 0,
    COLUMNA_GUARDIAN_AVISO,
    COLUMNA_GUARDIAN_CAIDA
};


struct ColumnaGuardian
{
    EstadoColumnaGuardian estado = COLUMNA_GUARDIAN_ENTERA;
    float x = 0.0f;
    float z = 0.0f;
    float tiempo = 0.0f;
};


struct OrbeGuardian
{
    bool activo = false;
    bool devuelto = false;
    bool fuerte = false;
    float x = 0.0f;
    float z = 0.0f;
    float vx = 0.0f;
    float vz = 0.0f;
    float vida = 0.0f;
    int dueno = -1;
};


struct EstadoJugadorGuardian
{
    int equipo = -1;               // 0 guardian, 1 atacante
    float recarga = 0.0f;
    float aturdimiento = 0.0f;
    float apuntarX = 0.0f;
    int lanzados = 0;

    // IA (bots y humanos desconectados).
    float temporizador = 0.0f;
    float objetivoX = 0.0f;
    float objetivoZ = 4.0f;
    float finta = 0.0f;
    float direccionFinta = 1.0f;
    float ruido = 0.0f;
};


struct MinijuegoGuardianRuinas
{
    ResultadoMinijuego resultado{};
    AudioJuego* audio = nullptr;   // lo asigna GestorMinijuegos; puede ser nullptr

    EstadoJugadorGuardian estadosJugadores[MAX_PARTICIPANTES];
    int indiceGuardian = -1;
    int cantidadAtacantes = 0;
    int metaOrbes = 10;

    // Guardian.
    float guardianX = 0.0f;
    float embestidaTiempo = 0.0f;
    float embestidaRecarga = 0.0f;
    float botObjetivoX = 0.0f;
    float botRetardo = 0.0f;
    float botRuido = 0.0f;
    int botAmenaza = -1;
    bool botEmbestir = false;

    OrbeGuardian orbes[MAX_ORBES_GUARDIAN];
    ColumnaGuardian columnas[MAX_COLUMNAS_GUARDIAN];
    BloquePrueba bloques[1 + MAX_COLUMNAS_GUARDIAN];
    ParticulaTierra particulas[MAX_PARTICULAS_GUARDIAN];

    int goles = 0;
    int bloqueos = 0;
    int equipoGanador = -1;
    bool partidaValida = false;
    int proximaColumna = 0;
    float tiempoSonidoRebote = 0.0f;

    Camera3D camara{};
    FaseGuardianRuinas fase = FASE_GUARDIAN_PREPARACION;

    float tiempoPreparacion = 0.0f;
    float tiempoRestante = 0.0f;
    float tiempoJugado = 0.0f;
    float tiempoAnimacion = 0.0f;
    float tiempoGol = 0.0f;

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
