#pragma once

#include "Core/Participante.h"
#include "Core/ResultadoMinijuego.h"
#include "Systems/Audio.h"
#include "raylib.h"


inline constexpr int MAX_HUELLAS_TRAZO_PERFECTO = 240;


enum FormaTrazoPerfecto
{
    FORMA_TRAZO_CRISTAL = 0,
    FORMA_TRAZO_RAYO,
    FORMA_TRAZO_COCHETE,
    CANTIDAD_FORMAS_TRAZO
};


enum FaseTrazoPerfecto
{
    FASE_TRAZO_PREPARACION = 0,
    FASE_TRAZO_DIBUJANDO,
    FASE_TRAZO_TERMINADO
};


struct EstadoJugadorTrazoPerfecto
{
    Vector2 cursor{};
    Vector2 huellas[MAX_HUELLAS_TRAZO_PERFECTO];

    int cantidadHuellas = 0;
    int puntuacionFinal = 0;

    float precisionAcumulada = 0.0f;
    float tiempoEvaluado = 0.0f;
    float tiempoNuevaHuella = 0.0f;
    float desfaseBot = 0.0f;
    float errorBot = 0.0f;
    float retardoBot = 0.2f;
    Vector2 objetivoSeguido{};
    bool seguimientoIniciado = false;

    // Solo visual: esquinas acertadas, salto del pintor, goteo del pincel.
    int aciertos = 0;
    int tipoTexto = 0;
    float textoFlotante = 0.0f;
    float destello = 0.0f;
    float sacudida = 0.0f;
    float salto = 0.0f;
    float calidad = 0.0f;           // cercania a la estrella, suavizada
    float fueraDeLinea = 0.0f;      // segundos lejos del trazo
    float goteo = 0.0f;
};


// Efectos del atelier (solo visuales).
struct ParticulaTrazoPerfecto
{
    bool activa = false;
    bool brillo = false;            // se dibuja en la pasada aditiva
    bool mancha = false;            // al tocar el piso deja una mancha
    Vector3 posicion{};
    Vector3 velocidad{};
    Color color{};
    float vida = 0.0f;
    float vidaMaxima = 1.0f;
    float tamano = 0.1f;
    float gravedad = 0.0f;
};


// Mancha de pintura que cayo al piso durante la partida.
struct ManchaTrazoPerfecto
{
    bool activa = false;
    Vector3 posicion{};
    float radio = 0.1f;
    int forma = 0;
    Color color{};
};


inline constexpr int MAX_PARTICULAS_TRAZO = 240;
inline constexpr int MAX_MANCHAS_TRAZO = 72;


struct MinijuegoTrazoPerfecto
{
    ResultadoMinijuego resultado{};
    AudioJuego* audio = nullptr;
    EstadoJugadorTrazoPerfecto jugadores[MAX_PARTICIPANTES];

    FaseTrazoPerfecto fase = FASE_TRAZO_PREPARACION;
    FormaTrazoPerfecto forma = FORMA_TRAZO_CRISTAL;

    float tiempoPreparacion = 0.0f;
    float tiempoTrazo = 0.0f;
    float tiempoAnimacion = 0.0f;
    float progresoObjetivo = 0.0f;

    // Solo visual: nada de esto toca la logica ni consume el
    // GetRandomValue de los bots.
    unsigned int semillaVisual = 2654435761u;
    float progresoVisto = 0.0f;     // esquinas ya evaluadas
    bool finVisto = false;
    int siguienteMancha = 0;
    ParticulaTrazoPerfecto particulas[MAX_PARTICULAS_TRAZO];
    ManchaTrazoPerfecto manchas[MAX_MANCHAS_TRAZO];

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
