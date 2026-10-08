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
#include "Minigames/MinijuegoCuerdaAcantilado.h"
#include "Minigames/MinijuegoDefensaNucleo.h"
#include "Minigames/Minijuego67.h"
#include "Minigames/MinijuegoInterruptoresCaos.h"
#include "Minigames/MinijuegoIslaFuego.h"
#include "Minigames/MinijuegoLluviaApilada.h"
#include "Minigames/MinijuegoMiradasCruzadas.h"
#include "Minigames/MinijuegoMurosLocos.h"
#include "Minigames/MinijuegoNucleosEnergia.h"
#include "Minigames/MinijuegoPasarelasVacio.h"
#include "Minigames/MinijuegoPelotas.h"
#include "Minigames/MinijuegoPasoSilencioso.h"
#include "Minigames/MinijuegoRefugioPinchos.h"
#include "Minigames/MinijuegoSecuenciaNeon.h"
#include "Minigames/MinijuegoTanquesPlasma.h"
#include "Minigames/MinijuegoTerritorioConquista.h"
#include "Minigames/MinijuegoTormentaMagnetica.h"
#include "Minigames/MinijuegoTrazoPerfecto.h"
#include "Minigames/MinijuegoTronco.h"
#include "Minigames/MinijuegoUltimoAsiento.h"
#include "Minigames/MinijuegoCajasPuerto.h"
#include "Minigames/MinijuegoLaberintoInclinado.h"
#include "Minigames/MinijuegoVetaCristal.h"
#include "Minigames/MinijuegoCapsulasBarajadas.h"
#include "Minigames/MinijuegoBateoMeteorico.h"
#include "Minigames/MinijuegoRacimoToxico.h"
#include "Minigames/MinijuegoTesoreroAcorralado.h"
#include "Minigames/MinijuegoDescensoNubes.h"
#include "Minigames/MinijuegoVoleaMagma.h"
#include "Minigames/MinijuegoParejasGlaciar.h"
#include "Minigames/MinijuegoEsferasCanon.h"
#include "Minigames/MinijuegoPescaIsla.h"
#include "Minigames/MinijuegoRodillosNeon.h"
#include "Minigames/MinijuegoBolasAzucar.h"
#include "Minigames/MinijuegoGruaChatarra.h"
#include "Minigames/MinijuegoPisotonPlagas.h"
#include "Minigames/MinijuegoBalsasRapido.h"
#include "Minigames/MinijuegoAutosGlobo.h"
#include "Minigames/MinijuegoSenderoInvisible.h"
#include "Minigames/MinijuegoBanqueteTurbo.h"
#include "Minigames/MinijuegoTuberiasDesierto.h"
#include "Minigames/MinijuegoTrepaMastil.h"
#include "Minigames/MinijuegoGuardianRuinas.h"


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
    MinijuegoTerritorioConquista minijuegoTerritorioConquista;
    MinijuegoDefensaNucleo minijuegoDefensaNucleo;
    MinijuegoLluviaApilada minijuegoLluviaApilada;
    MinijuegoCuerdaAcantilado minijuegoCuerdaAcantilado;
    MinijuegoUltimoAsiento minijuegoUltimoAsiento;
    MinijuegoCajasPuerto minijuegoCajasPuerto;
    MinijuegoLaberintoInclinado minijuegoLaberintoInclinado;
    MinijuegoVetaCristal minijuegoVetaCristal;
    MinijuegoCapsulasBarajadas minijuegoCapsulasBarajadas;
    MinijuegoBateoMeteorico minijuegoBateoMeteorico;
    MinijuegoRacimoToxico minijuegoRacimoToxico;
    MinijuegoTesoreroAcorralado minijuegoTesoreroAcorralado;
    MinijuegoDescensoNubes minijuegoDescensoNubes;
    MinijuegoVoleaMagma minijuegoVoleaMagma;
    MinijuegoParejasGlaciar minijuegoParejasGlaciar;
    MinijuegoEsferasCanon minijuegoEsferasCanon;
    MinijuegoPescaIsla minijuegoPescaIsla;
    MinijuegoRodillosNeon minijuegoRodillosNeon;
    MinijuegoBolasAzucar minijuegoBolasAzucar;
    MinijuegoGruaChatarra minijuegoGruaChatarra;
    MinijuegoPisotonPlagas minijuegoPisotonPlagas;
    MinijuegoBalsasRapido minijuegoBalsasRapido;
    MinijuegoAutosGlobo minijuegoAutosGlobo;
    MinijuegoSenderoInvisible minijuegoSenderoInvisible;
    MinijuegoBanqueteTurbo minijuegoBanqueteTurbo;
    MinijuegoTuberiasDesierto minijuegoTuberiasDesierto;
    MinijuegoTrepaMastil minijuegoTrepaMastil;
    MinijuegoGuardianRuinas minijuegoGuardianRuinas;

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
