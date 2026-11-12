#pragma once

#include "Core/Participante.h"
#include "Core/ResultadoMinijuego.h"
#include "Minigames/TiposMinijuegos.h"
#include "Systems/Audio.h"


// Referencia de reglas: Mario Party 4 - Long Claw of the Law (garra que baja
// para atrapar objetos). Aqui cada jugador maneja una garra magnetica en un
// pozo de chatarra compartido y lleva lo atrapado a la tolva de su color.


inline constexpr int MAX_OBJETOS_GRUA = 30;
inline constexpr int MAX_PARTICULAS_GRUA = 220;


enum FaseGruaChatarra
{
    FASE_GRUA_PREPARACION = 0,
    FASE_GRUA_JUGANDO,
    FASE_GRUA_TERMINADO
};


enum TipoObjetoGrua
{
    OBJETO_GRUA_TUERCA = 0,
    OBJETO_GRUA_ENGRANAJE,
    OBJETO_GRUA_MOTOR,
    OBJETO_GRUA_BATERIA,
    OBJETO_GRUA_CARTUCHO,
    CANTIDAD_TIPOS_OBJETO_GRUA
};


enum EstadoGarraGrua
{
    GARRA_GRUA_LIBRE = 0,
    GARRA_GRUA_ACCION
};


struct ObjetoGrua
{
    bool activo = false;
    int tipo = OBJETO_GRUA_TUERCA;
    float x = 0.0f;
    float z = 0.0f;
    float giro = 0.0f;
    float tiempoReaparicion = 0.0f;
};


struct EstadoJugadorGrua
{
    bool participa = false;
    int puesto = 0;

    // Garra: posicion en el plano e inercia.
    float x = 0.0f;
    float z = 0.0f;
    float velocidadX = 0.0f;
    float velocidadZ = 0.0f;

    // Secuencia de bajada / captura / entrega.
    EstadoGarraGrua estado = GARRA_GRUA_LIBRE;
    float tiempo = 0.0f;
    float duracion = 0.0f;
    float origenX = 0.0f;
    float origenZ = 0.0f;
    bool resuelto = false;
    bool exito = false;
    bool depositado = false;
    bool disputaPerdida = false;
    int tipoCapturado = -1;
    float bloqueo = 0.0f;

    int puntos = 0;
    int mejorObjeto = 0;

    // Mensaje breve sobre la garra.
    float tiempoMensaje = 0.0f;
    int mensaje = 0;
    int valorMensaje = 0;

    // Inteligencia de bot.
    float tiempoDecision = 0.0f;
    int objetivo = -1;
    float errorX = 0.0f;
    float errorZ = 0.0f;
    float esperaBajar = 0.0f;
};


// Particula puramente visual (polvo, chispas, piezas, confeti).
struct ParticulaGrua
{
    bool activa = false;
    int tipo = 0;
    Vector3 posicion{};
    Vector3 velocidad{};
    float vida = 0.0f;
    float vidaMaxima = 1.0f;
    float tamano = 0.1f;
    Color color = WHITE;
};


struct MinijuegoGruaChatarra
{
    ResultadoMinijuego resultado{};
    AudioJuego* audio = nullptr;   // lo asigna GestorMinijuegos; puede ser nullptr

    EstadoJugadorGrua estadosJugadores[MAX_PARTICIPANTES];
    ObjetoGrua objetos[MAX_OBJETOS_GRUA];

    // Estado visual: lo escribe solo Actualizar a partir de los cambios de
    // la logica y usa su propia semilla (nunca GetRandomValue).
    ParticulaGrua particulas[MAX_PARTICULAS_GRUA];
    unsigned int semillaVisual = 0;
    float destelloTolva[MAX_PARTICIPANTES] = {};
    float sacudidaGarra[MAX_PARTICIPANTES] = {};
    float tiempoChispasSoldadura = 0.0f;
    float tiempoFinVisual = -1.0f;

    Camera3D camara{};
    FaseGruaChatarra fase = FASE_GRUA_PREPARACION;

    int cantidadPuestos = 0;
    bool partidaValida = false;
    bool empate = false;

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

    // Reglas de la partida (Actualizar la llama y despues anima lo visual).
    void ActualizarLogica(
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

    // Libera lo horneado en la GPU (mallas, material y shader); se rehace
    // solo la proxima vez que se dibuje.
    void Descargar();
};


// Carga del paquete GLB solo las piezas que muestran el estado del juego
// (tolva, iman, pinzas, carro, marca y las cinco piezas de chatarra). El
// desguace se construye con primitivas propias horneadas en mallas.
void CargarPiezasJugablesGruaChatarra();
