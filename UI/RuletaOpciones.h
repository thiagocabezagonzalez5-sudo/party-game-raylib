#pragma once

#include "Systems/Audio.h"
#include "raylib.h"


//==================================================
// RULETA DE OPCIONES
//==================================================
//
// Ruleta circular reutilizable: reparte el circulo en sectores de color
// con un texto corto y un marcador fijo arriba. Igual que
// RuletaMinijuegos, el RESULTADO SE RECIBE YA DECIDIDO en Iniciar(): la
// animacion solo lo muestra y siempre frena con el marcador dentro del
// sector ganador (nunca en el borde). Acelerar solo adelanta el reloj de
// la animacion; no cambia donde frena.
//
// Flujo: Iniciar (queda ESPERANDO, quieta) -> Girar -> GIRANDO ->
// DETENIDA. Quien la use decide cuando llamar a Girar() (un bot casi al
// instante; un humano al confirmar).

inline constexpr int MAX_SECTORES_RULETA_OPCIONES = 12;


enum EstadoRuletaOpciones
{
    RULETA_OPC_INACTIVA = 0,
    RULETA_OPC_ESPERANDO,
    RULETA_OPC_GIRANDO,
    RULETA_OPC_DETENIDA
};


struct SectorRuletaOpciones
{
    char texto[24] = {};
    Color color = GRAY;
};


struct RuletaOpciones
{
    SectorRuletaOpciones sectores[MAX_SECTORES_RULETA_OPCIONES]{};
    int cantidad = 0;

    // Sector donde frena (decidido antes de girar).
    int sectorGanador = 0;

    EstadoRuletaOpciones estado = RULETA_OPC_INACTIVA;

    // Angulo del primer sector en grados (sentido horario en pantalla).
    float angulo = 0.0f;
    float anguloInicial = 0.0f;
    float anguloFinal = 0.0f;

    float tiempo = 0.0f;
    float tiempoDetenida = 0.0f;
    bool acelerada = false;

    int ultimoSector = -1;

    AudioJuego* audio = nullptr;

    void Iniciar(
        const SectorRuletaOpciones sectoresNuevos[],
        int cantidadSectores,
        int sectorElegido,
        AudioJuego* audioJuego
    );

    // Empieza el giro (ignorado si no esta ESPERANDO).
    void Girar();

    void Actualizar(
        float deltaTime,
        bool acelerar
    );

    bool EstaGirando() const;
    bool Termino() const;

    // Sector bajo el marcador en este instante.
    int SectorBajoMarcador() const;

    // centro y radio en PIXELES. tiempoReloj solo anima el destello del
    // sector ganador.
    void Dibujar(
        Vector2 centro,
        float radio,
        float tiempoReloj
    ) const;
};
