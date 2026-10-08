#pragma once

#include "Core/Participante.h"
#include "Core/ResultadoMinijuego.h"
#include "Minigames/TiposMinijuegos.h"
#include "Systems/Audio.h"


inline constexpr int MAX_BLOQUES_BOLAS_AZUCAR = 10;
inline constexpr int MAX_PARTICULAS_BOLAS_AZUCAR = 64;
inline constexpr int VIDAS_BOLAS_AZUCAR = 3;


enum FaseBolasAzucar
{
    FASE_BOLAS_PREPARACION = 0,
    FASE_BOLAS_JUGANDO,
    FASE_BOLAS_TERMINADO
};


enum EstadoBolaAzucar
{
    BOLA_AZUCAR_EMPUJADA = 0,
    BOLA_AZUCAR_LANZADA,
    BOLA_AZUCAR_REPOSO
};


struct BolaAzucar
{
    bool activa = false;
    EstadoBolaAzucar estado = BOLA_AZUCAR_EMPUJADA;
    float x = 0.0f;
    float z = 0.0f;
    float velocidadX = 0.0f;
    float velocidadZ = 0.0f;
    float radio = 0.3f;
    float giro = 0.0f;
    float inactiva = 0.0f;
};


struct EstadoJugadorBolasAzucar
{
    bool vivo = false;
    bool accionPrevia = false;
    bool botNota = true;
    int vidas = VIDAS_BOLAS_AZUCAR;
    int posicionEliminacion = 0;
    float aturdido = 0.0f;
    float creando = 0.0f;
    float inmunidad = 0.0f;
    float empujeX = 0.0f;
    float empujeZ = 0.0f;
    float tiempoEmpujando = 0.0f;
    float botReaccion = 0.0f;
    int botFallos = 0;
    int botVidasRef = -1;
    int botRivalExcluido = -1;
    float botExcluidoTiempo = 0.0f;
    float botDesplazamiento = 0.0f;
    float botDesplazaX = 0.0f;
    float botDesplazaZ = 0.0f;
};


struct MinijuegoBolasAzucar
{
    ResultadoMinijuego resultado{};
    AudioJuego* audio = nullptr;
    EstadoJugadorBolasAzucar estadosJugadores[MAX_PARTICIPANTES];
    BolaAzucar bolas[MAX_PARTICIPANTES];
    BloquePrueba bloques[MAX_BLOQUES_BOLAS_AZUCAR];
    ParticulaTierra particulas[MAX_PARTICULAS_BOLAS_AZUCAR];
    int cantidadBloques = 0;

    Camera3D camara{};
    FaseBolasAzucar fase = FASE_BOLAS_PREPARACION;

    int mensajeJugador = -1;
    int mensajeTipo = 0;
    float mensajeTiempo = 0.0f;

    float tiempoRestante = 0.0f;
    float tiempoPreparacion = 0.0f;
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
