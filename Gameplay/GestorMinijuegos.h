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
#include "Minigames/MinijuegoCablesCruzados.h"
#include "Minigames/MinijuegoVagonetasDuelo.h"
#include "Minigames/MinijuegoGemasDeslizantes.h"

#include "Gameplay/ListaMinijuegos.h"


// Punto unico de propiedad y dispatch de los minijuegos.
// Los miembros y cada dispatch se generan desde LISTA_MINIJUEGOS
// (Gameplay/ListaMinijuegos.h); las firmas distintas estan en esa tabla.
struct GestorMinijuegos
{
    IdMinijuego minijuegoActivo = MINIJUEGO_COLOR_SEGURO;

#define GM_DECLARAR_MIEMBRO(id, Tipo, miembro, activar, reiniciar, actualizar, dibujar, audio, ia, descarga) \
    Tipo miembro;

    LISTA_MINIJUEGOS(GM_DECLARAR_MIEMBRO)

#undef GM_DECLARAR_MIEMBRO

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

    bool MantieneControlIAEnReconexion() const;

    void ReiniciarJugadorCompartido(
        ContextoMinijuego& contexto,
        int indice
    ) const;

    void ReiniciarJugadoresCompartidos(
        ContextoMinijuego& contexto
    ) const;

    // Libera lo horneado en la GPU por ese minijuego (si tiene); se rehace
    // solo la proxima vez que se dibuje.
    void DescargarMinijuego(
        IdMinijuego id
    );

    void Descargar();
};
