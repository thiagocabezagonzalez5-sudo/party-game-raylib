#pragma once

#include "Core/Participante.h"
#include "Core/ResultadoMinijuego.h"
#include "Systems/Audio.h"
#include "raylib.h"


inline constexpr int MAX_DRONES_CONTEO_EXPLOSIVO = 22;
inline constexpr int MAX_PARTICULAS_CONTEO_EXPLOSIVO = 200;


enum FaseConteoExplosivo
{
    FASE_CONTEO_PREPARACION = 0,
    FASE_CONTEO_OBSERVACION,
    FASE_CONTEO_RESPUESTA,
    FASE_CONTEO_TERMINADO
};


// Todos los drones son identicos (mismo modelo, color y luces): su
// aspecto no debe codificar el indice ni el total.
struct DronConteoExplosivo
{
    Vector3 posicion{};
    Vector3 velocidad{};
    float faseFlotacion = 0.0f;
    float faseHelice = 0.0f;

    // Inclinacion visual suavizada hacia donde avanza (solo visual).
    float inclinacionX = 0.0f;
    float inclinacionZ = 0.0f;
};


enum TipoParticulaConteoExplosivo
{
    PARTICULA_CONTEO_CHISPA = 0,
    PARTICULA_CONTEO_CONFETI,
    PARTICULA_CONTEO_HUMO,
    PARTICULA_CONTEO_POLVO
};


struct ParticulaConteoExplosivo
{
    bool activa = false;
    TipoParticulaConteoExplosivo tipo = PARTICULA_CONTEO_CHISPA;
    Vector3 posicion{};
    Vector3 velocidad{};
    float vida = 0.0f;
    float vidaMaxima = 1.0f;
    float tamano = 0.05f;
    float giro = 0.0f;
    Color color = WHITE;
};


struct EstadoJugadorConteoExplosivo
{
    int respuesta = 10;
    int diferencia = 0;
    bool botRespondio = false;
    float retrasoBot = 0.0f;

    // Seleccion rapida: mantener una direccion repite y acelera.
    int direccionMantenida = 0;
    float tiempoMantenido = 0.0f;
    float proximoPaso = 0.0f;
};


struct MinijuegoConteoExplosivo
{
    ResultadoMinijuego resultado{};
    AudioJuego* audio = nullptr;

    DronConteoExplosivo drones[MAX_DRONES_CONTEO_EXPLOSIVO];
    EstadoJugadorConteoExplosivo jugadores[MAX_PARTICIPANTES];
    ParticulaConteoExplosivo particulas[MAX_PARTICULAS_CONTEO_EXPLOSIVO];

    Camera3D camara{};
    FaseConteoExplosivo fase = FASE_CONTEO_PREPARACION;

    int cantidadDrones = 0;
    float tiempoFase = 0.0f;        // en TERMINADO cuenta hacia arriba
    float tiempoAnimacion = 0.0f;

    // Revelacion (solo visual: el resultado ya quedo cerrado al terminar
    // el tiempo de respuesta).
    int dronesRevelados = 0;
    bool persianaAsentada = false;
    bool totalRevelado = false;
    bool resultadoAnunciado = false;

    void Inicializar();

    void Reiniciar(
        Participante participantes[]
    );

    void Actualizar(
        float deltaTime,
        Participante participantes[]
    );

    void Dibujar(
        const Participante participantes[]
    ) const;

    const ResultadoMinijuego& ObtenerResultado() const;
};
