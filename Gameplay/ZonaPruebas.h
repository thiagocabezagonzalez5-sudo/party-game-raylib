#pragma once

#include "Core/CatalogoMinijuegos.h"
#include "Core/Participante.h"
#include "Gameplay/ContextoMinijuego.h"
#include "Gameplay/GestorMinijuegos.h"
#include "Gameplay/PrototipoTablero.h"
#include "Minigames/MinijuegoBarraGiratoria.h"
#include "Minigames/MinijuegoCanteraFuga.h"
#include "Minigames/MinijuegoCargaInestable.h"
#include "Minigames/MinijuegoCircuitoVoltaje.h"
#include "Minigames/MinijuegoConteoExplosivo.h"
#include "Minigames/Minijuego67.h"
#include "Minigames/MinijuegoIslaFuego.h"
#include "Minigames/MinijuegoInterruptoresCaos.h"
#include "Minigames/MinijuegoMiradasCruzadas.h"
#include "Minigames/MinijuegoMurosLocos.h"
#include "Minigames/MinijuegoNucleosEnergia.h"
#include "Minigames/MinijuegoPasarelasVacio.h"
#include "Minigames/MinijuegoPelotas.h"
#include "Minigames/MinijuegoPasoSilencioso.h"
#include "Minigames/MinijuegoRefugioPinchos.h"
#include "Minigames/MinijuegoSecuenciaNeon.h"
#include "Minigames/MinijuegoTanquesPlasma.h"
#include "Minigames/MinijuegoTormentaMagnetica.h"
#include "Minigames/MinijuegoTrazoPerfecto.h"
#include "Minigames/MinijuegoTronco.h"
#include "Minigames/PruebaModelos.h"
#include "Minigames/TiposMinijuegos.h"
#include "Systems/Audio.h"


enum ModoZonaPruebas
{
    PRUEBA_ZONA_PRINCIPAL = -4,
    PRUEBA_MODELOS = -3,
    PRUEBA_TABLERO = -2,
    PRUEBA_MINIJUEGO = -1
};


struct ZonaPruebas
{
    ModoZonaPruebas modoActual = PRUEBA_ZONA_PRINCIPAL;
    IdMinijuego minijuegoActual = MINIJUEGO_COLOR_SEGURO;

    JugadorPrueba jugadores[MAX_JUGADORES_PRUEBA];

    Participante* participantes = nullptr;
    int cantidadParticipantes = 0;

    AudioJuego* audio = nullptr;

    ParticulaTierra particulas[MAX_PARTICULAS_TIERRA];

    BloquePrueba bloquesPrincipal[MAX_BLOQUES_PRUEBA];
    int cantidadBloquesPrincipal = 0;

    Camera3D camaraPrincipal{};

    ContextoMinijuego contextoMinijuego;
    GestorMinijuegos gestorMinijuegos;
    MinijuegoPelotas minijuegoPelotas;
    PruebaModelos pruebaModelos;
    MinijuegoTronco minijuegoTronco;
    Minijuego67 minijuego67;
    PrototipoTablero prototipoTablero;
    MinijuegoIslaFuego minijuegoIslaFuego;
    MinijuegoBarraGiratoria minijuegoBarraGiratoria;
    MinijuegoNucleosEnergia minijuegoNucleosEnergia;
    MinijuegoRefugioPinchos minijuegoRefugioPinchos;
    MinijuegoMiradasCruzadas minijuegoMiradasCruzadas;
    MinijuegoMurosLocos minijuegoMurosLocos;
    MinijuegoTormentaMagnetica minijuegoTormentaMagnetica;
    MinijuegoConteoExplosivo minijuegoConteoExplosivo;
    MinijuegoPasoSilencioso minijuegoPasoSilencioso;
    MinijuegoCircuitoVoltaje minijuegoCircuitoVoltaje;
    MinijuegoTrazoPerfecto minijuegoTrazoPerfecto;
    MinijuegoCargaInestable minijuegoCargaInestable;
    MinijuegoSecuenciaNeon minijuegoSecuenciaNeon;
    MinijuegoInterruptoresCaos minijuegoInterruptoresCaos;
    MinijuegoTanquesPlasma minijuegoTanquesPlasma;
    MinijuegoPasarelasVacio minijuegoPasarelasVacio;
    MinijuegoCanteraFuga minijuegoCanteraFuga;

    bool modoCatalogo = false;
    bool mostrarDebug = false;
    bool volverAlMenu = false;

    void Inicializar(
        Participante participantesJuego[],
        int cantidadParticipantesJuego,
        AudioJuego* audioJuego = nullptr
    );

    void CambiarModo(
        ModoZonaPruebas nuevoModo
    );

    void CambiarMinijuego(
        IdMinijuego nuevoMinijuego
    );

    void Actualizar(
        float deltaTime
    );

    void Dibujar() const;

    void ReiniciarJugador(
        int indice
    );

    void ReiniciarJugadores();

    const ResultadoMinijuego* ObtenerResultadoMinijuego() const;

    void Descargar();
};
