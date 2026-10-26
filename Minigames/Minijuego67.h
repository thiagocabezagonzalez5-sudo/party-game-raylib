#pragma once

#include "Core/Participante.h"
#include "Core/ResultadoMinijuego.h"
#include "Minigames/TiposMinijuegos.h"


struct AudioJuego;


enum TipoPieza67
{
    PIEZA_NUMERO_6 = 0,
    PIEZA_NUMERO_7
};


enum EstadoMesa67
{
    MESA_67_VACIA = 0,
    MESA_67_CON_6,
    MESA_67_COMPLETA
};


enum EstadoPartida67
{
    FABRICA_67_ESPERANDO_JUGADORES = 0,
    FABRICA_67_PREPARANDO,
    FABRICA_67_JUGANDO,
    FABRICA_67_FINALIZADO
};


// Efectos de la fabrica (solo visuales). Cada particula pertenece a la
// vista de un equipo: se dibuja unicamente en su mitad de pantalla.
enum TipoParticula67
{
    PARTICULA_67_CHISPA = 0,
    PARTICULA_67_HUMO,
    PARTICULA_67_VAPOR,
    PARTICULA_67_CONFETI
};


struct Particula67
{
    bool activa = false;
    int equipo = 0;
    TipoParticula67 tipo = PARTICULA_67_CHISPA;

    Vector3 posicion{};
    Vector3 velocidad{};

    float vida = 0.0f;
    float vidaMaxima = 1.0f;
    float tamano = 0.1f;
    float giro = 0.0f;

    Color color{};
};


enum TipoAviso67
{
    AVISO_67_COMPLETO = 0,
    AVISO_67_ORDEN,
    AVISO_67_DESTIEMPO
};


// Texto flotante anclado a un punto de la vista del equipo.
struct AvisoFlotante67
{
    TipoAviso67 tipo = AVISO_67_COMPLETO;
    float tiempo = 0.0f;
    Vector3 posicion{};
};


inline constexpr int MAX_PARTICULAS_67 = 260;
inline constexpr int MAX_AVISOS_67 = 3;


struct EstadoJugador67
{
    TipoPieza67 tipoPieza = PIEZA_NUMERO_6;

    bool llevaPieza = false;
    bool mirandoMesa = false;

    float tiempoGiro = 0.0f;
    float tiempoStun = 0.0f;

    // Se aplica tanto despues de agarrar una pieza como despues
    // de colocarla. Evita encadenar dos acciones en el mismo instante.
    float tiempoCooldownInteraccion = 0.0f;

    // Solo para bots. Cada pieza que entra en la zona de recogida
    // se evalua una vez: el bot puede agarrarla o dejarla pasar.
    float tiempoDecisionBot = 0.0f;
    bool botEvaluoPieza = false;
    bool botAgarraPieza = false;
};


struct EstadoEquipo67
{
    EstadoMesa67 mesa = MESA_67_VACIA;

    float tiempoMesaCompleta = 0.0f;
    float tiempoFeedback = 0.0f;

    // Cada cinta tiene un unico flujo por equipo. Asi no se dibuja
    // otra secuencia completa por cada jugador.
    float progresoCinta6 = 0.0f;
    float progresoCinta7 = 0.125f;

    int puntos = 0;
    bool ultimoAcierto = false;

    // Solo visual: temblor de la vista al equivocarse y brillo de las
    // cifras de la mesa al colocarlas (1 -> 0).
    float sacudida = 0.0f;
    float destello6 = 0.0f;
    float destello7 = 0.0f;

    AvisoFlotante67 avisos[MAX_AVISOS_67];
};


struct Minijuego67
{
    ResultadoMinijuego resultado;

    EstadoJugador67 estadosJugadores[MAX_JUGADORES_PRUEBA];
    EstadoEquipo67 equipos[2];

    int equipoPorJugador[MAX_JUGADORES_PRUEBA] = { -1, -1, -1, -1 };
    int ordenEnEquipoPorJugador[MAX_JUGADORES_PRUEBA] = { -1, -1, -1, -1 };

    int cantidadJugadoresEquipo[2] = { 0, 0 };

    EstadoPartida67 estadoPartida = FABRICA_67_ESPERANDO_JUGADORES;

    float tiempoPreparacion = 2.5f;
    float tiempoPartida = 30.0f;
    float velocidadCintas = 0.24f;
    float desplazamientoVisualCintas = 0.0f;

    int jugadoresEnPartida = 0;
    int mascaraJugadoresEnPartida = 0;
    int equipoGanador = -1;

    bool partidaValida = false;
    bool empate = false;

    // Segundos restantes del aviso "SE RECONFIGURARON LOS EQUIPOS".
    float avisoReconfiguracion = 0.0f;

    // Una camara por mitad de pantalla. Cada vista se dibuja con su
    // propio viewport y recorte (como Vagonetas): sin RenderTexture.
    Camera3D camarasEquipo[2]{};

    float fotogramaAnimacionIdle = 0.0f;

    // Reloj de las animaciones (Dibujar es const).
    float tiempoAnimacion = 0.0f;

    Particula67 particulas[MAX_PARTICULAS_67];
    unsigned int semillaEfectos = 12345u;
    float acumuladorVapor = 0.0f;
    float acumuladorSoldadura = 0.0f;
    float tiempoConfeti = 0.0f;

    // Lo asigna GestorMinijuegos; puede ser nullptr.
    AudioJuego* audio = nullptr;

    void Inicializar();

    void Reiniciar(
        const Participante participantes[],
        int cantidadMaxima
    );

    void PrepararEquipos(
        const Participante participantes[],
        int cantidadMaxima
    );

    void Actualizar(
        float deltaTime,
        int cantidadMaxima,
        const Participante participantes[]
    );

    void Dibujar(
        int cantidadMaxima,
        const Participante participantes[]
    ) const;

    const ResultadoMinijuego& ObtenerResultado() const;

    void Descargar();
};
