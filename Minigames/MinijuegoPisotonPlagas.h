#pragma once

#include "Core/Participante.h"
#include "Core/ResultadoMinijuego.h"
#include "Minigames/TiposMinijuegos.h"
#include "Systems/Audio.h"


//==================================================
// PISOTON DE PLAGAS
//==================================================
//
// Todos contra todos en un jardin gigante. Escarabajos, orugas y babosas
// doradas salen de madrigueras (la tierra se mueve antes de que asomen) y
// caminan por el cesped. Un ground pound (salto + salto en el aire) sobre
// una plaga la aplasta y suma puntos; un salto normal sobre ella solo la
// aturde para que alguien la remate. Las avispas vuelan bajo: si te tocan
// pierdes puntos y quedas lento, pero un golpe las espanta. Cada 15 s una
// flor gigante florece y quien este debajo gana puntos extra.
//==================================================

inline constexpr int MAX_BLOQUES_PISOTON = 2;
inline constexpr int MAX_PARTICULAS_PISOTON = MAX_PARTICULAS_TIERRA;
inline constexpr int MAX_PLAGAS_PISOTON = 12;
inline constexpr int MAX_SEGMENTOS_PISOTON = 3;
inline constexpr int MAX_MADRIGUERAS_PISOTON = 6;
inline constexpr int MAX_AVISPAS_PISOTON = 2;
inline constexpr int MAX_POPUPS_PISOTON = 14;


enum FasePisoton
{
    FASE_PISOTON_PREPARACION = 0,
    FASE_PISOTON_JUGANDO,
    FASE_PISOTON_TERMINADO
};


enum TipoPlagaPisoton
{
    PLAGA_PISOTON_ESCARABAJO = 0,
    PLAGA_PISOTON_ORUGA,
    PLAGA_PISOTON_BABOSA
};


struct PlagaPisoton
{
    bool activa = false;
    TipoPlagaPisoton tipo = PLAGA_PISOTON_ESCARABAJO;
    int id = 0;                       // distingue plagas que reutilizan la ranura

    // Escarabajo y babosa usan solo el segmento 0; la oruga usa los tres.
    float segX[MAX_SEGMENTOS_PISOTON]{};
    float segZ[MAX_SEGMENTOS_PISOTON]{};
    bool segVivo[MAX_SEGMENTOS_PISOTON]{};

    float dirX = 1.0f;
    float dirZ = 0.0f;
    float destinoX = 0.0f;
    float destinoZ = 0.0f;
    float cambioRumbo = 0.0f;

    float aparicion = 0.0f;           // 0..1, no se puede pisar hasta que asoma
    float aturdida = 0.0f;
    float proteccion = 0.0f;          // evita repetir el pisoton en el mismo salto
    float edad = 0.0f;
    float vidaMaxima = 14.0f;
    bool escapando = false;
    float escala = 1.0f;
};


struct MadriguerasPisoton
{
    float x = 0.0f;
    float z = 0.0f;
    float aviso = 0.0f;               // > 0 mientras la tierra se mueve
    TipoPlagaPisoton tipoPendiente = PLAGA_PISOTON_ESCARABAJO;
};


struct AvispaPisoton
{
    bool activa = false;
    float retrasoAparicion = 0.0f;
    float x = 0.0f;
    float z = 0.0f;
    float altura = 0.95f;
    float dirX = 1.0f;
    float dirZ = 0.0f;
    int objetivo = -1;
    float tiempoObjetivo = 0.0f;
    float huida = 0.0f;               // espantada por un golpe
    float reposo = 0.0f;              // se aleja tras picar
    float fase = 0.0f;
};


struct PopupPisoton
{
    bool activo = false;
    Vector3 posicion{};
    int valor = 0;
    float vida = 0.0f;
    Color color = WHITE;
};


struct EstadoBotPisoton
{
    int objetivo = -1;
    int objetivoId = -1;
    float reevaluar = 0.0f;
    float errorAlcance = 0.0f;
    int modoAvispa = 0;               // 0 evitar, 1 atacar
    float cambioModo = 0.0f;
    float cooldownGolpe = 0.0f;
    float cooldownSalto = 0.0f;
    float tiempoVagar = 0.0f;
    float vagarX = 0.0f;
    float vagarZ = 0.0f;
};


struct MinijuegoPisotonPlagas
{
    ResultadoMinijuego resultado{};
    AudioJuego* audio = nullptr;   // lo asigna GestorMinijuegos; puede ser nullptr

    BloquePrueba bloques[MAX_BLOQUES_PISOTON];
    ParticulaTierra particulas[MAX_PARTICULAS_PISOTON];
    PlagaPisoton plagas[MAX_PLAGAS_PISOTON];
    MadriguerasPisoton madrigueras[MAX_MADRIGUERAS_PISOTON];
    AvispaPisoton avispas[MAX_AVISPAS_PISOTON];
    PopupPisoton popups[MAX_POPUPS_PISOTON];
    EstadoBotPisoton bots[MAX_PARTICIPANTES];

    int puntos[MAX_PARTICIPANTES]{};
    int doradas[MAX_PARTICIPANTES]{};
    int aplastadas[MAX_PARTICIPANTES]{};
    float inmunidadAvispa[MAX_PARTICIPANTES]{};

    Camera3D camara{};
    FasePisoton fase = FASE_PISOTON_PREPARACION;

    int cantidadBloques = 0;
    int siguienteId = 1;
    int doradasGeneradas = 0;

    float tiempoPreparacion = 0.0f;
    float tiempoRestante = 0.0f;
    float tiempoAnimacion = 0.0f;
    float proximaAparicion = 0.0f;

    // Flor gigante: se mueve a un punto nuevo tras cada floracion.
    float florX = 0.0f;
    float florZ = 0.0f;
    float tiempoFlor = 0.0f;
    float florAbierta = 0.0f;
    int florGanador = -1;

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
