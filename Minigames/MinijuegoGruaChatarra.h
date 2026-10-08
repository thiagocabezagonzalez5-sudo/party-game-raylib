#pragma once

#include "Core/Participante.h"
#include "Core/ResultadoMinijuego.h"
#include "Minigames/TiposMinijuegos.h"
#include "Systems/Audio.h"


// Referencia de reglas: Mario Party 4 - Long Claw of the Law (garra que baja
// para atrapar objetos). Aqui cada jugador maneja una garra magnetica en un
// pozo de chatarra compartido y lleva lo atrapado a la tolva de su color.


inline constexpr int MAX_OBJETOS_GRUA = 30;


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


struct MinijuegoGruaChatarra
{
    ResultadoMinijuego resultado{};
    AudioJuego* audio = nullptr;   // lo asigna GestorMinijuegos; puede ser nullptr

    EstadoJugadorGrua estadosJugadores[MAX_PARTICIPANTES];
    ObjetoGrua objetos[MAX_OBJETOS_GRUA];

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

    void Dibujar(
        const JugadorPrueba jugadores[],
        int cantidadMaxima,
        const Participante participantes[],
        bool mostrarDebug
    ) const;

    const ResultadoMinijuego& ObtenerResultado() const;
};
