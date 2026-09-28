#pragma once

#include "Core/CatalogoMinijuegos.h"
#include "Core/ResultadoMinijuego.h"
#include "Gameplay/ContextoMinijuego.h"

#include "Minigames/MinijuegoBarraGiratoria.h"
#include "Minigames/MinijuegoCanteraFuga.h"
#include "Minigames/MinijuegoCargaInestable.h"
#include "Minigames/MinijuegoCapitanManda.h"
#include "Minigames/MinijuegoCircuitoVoltaje.h"
#include "Minigames/MinijuegoColorSeguro.h"
#include "Minigames/MinijuegoConteoExplosivo.h"
#include "Minigames/Minijuego67.h"
#include "Minigames/MinijuegoInterruptoresCaos.h"
#include "Minigames/MinijuegoIslaFuego.h"
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


// Punto unico de propiedad y dispatch de los minijuegos.
// Las diferencias de firmas permanecen visibles en GestorMinijuegos.cpp.
struct GestorMinijuegos
{
    IdMinijuego minijuegoActivo = MINIJUEGO_COLOR_SEGURO;

    MinijuegoColorSeguro minijuegoColor;
    MinijuegoPelotas minijuegoPelotas;
    MinijuegoTronco minijuegoTronco;
    Minijuego67 minijuego67;
    MinijuegoIslaFuego minijuegoIslaFuego;
    MinijuegoCapitanManda minijuegoCapitanManda;
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

    void Inicializar();

    void ActivarMinijuego(
        IdMinijuego id,
        ContextoMinijuego& contexto
    );

    void ReiniciarActivo(
        ContextoMinijuego& contexto
    );

    void ActualizarActivo(
        float deltaTime,
        ContextoMinijuego& contexto
    );

    void DibujarActivo(
        const ContextoMinijuego& contexto,
        bool mostrarDebug
    ) const;

    const ResultadoMinijuego* ObtenerResultadoActivo() const;

    IdMinijuego ObtenerIdActivo() const;

    void PrepararTemaVisualActivo() const;

    bool MantieneControlIAEnReconexion() const;

    void ReiniciarJugadorCompartido(
        ContextoMinijuego& contexto,
        int indice
    ) const;

    void ReiniciarJugadoresCompartidos(
        ContextoMinijuego& contexto
    ) const;

    void Descargar();
};
