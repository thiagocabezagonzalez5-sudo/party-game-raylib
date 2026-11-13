#pragma once

// Lista unica de minijuegos del GestorMinijuegos (X-macro).
// Cada linea genera el miembro del gestor y su caso en cada dispatch de
// GestorMinijuegos.cpp. Agregar un minijuego = incluir su header en
// GestorMinijuegos.h + una linea aca (en el orden de IdMinijuego).
//
// X(id, Tipo, miembro, activar, reiniciar, actualizar, dibujar, audio, iaReconexion, descarga)
//
// Letras de las firmas: J = contexto.jugadores, C = contexto.cantidadJugadores,
// P = contexto.participantes, X = contexto.particulas + contexto.cantidadParticulas,
// A = contexto.audio, D = mostrarDebug. Actualizar recibe ademas deltaTime primero.
//
// reiniciar  : argumentos de Reiniciar (JC, PC, P, JPC).
// actualizar : argumentos de Actualizar (P, CP, JCP, JCPX, JCPXA).
// dibujar    : argumentos de Dibujar (P, CP, JCP, JCPD, JCPXD).
// activar    : que hace ActivarMinijuego, antes y despues de
//              ReiniciarJugadoresCompartidos:
//   INICIAR_CONFIGURAR           antes Inicializar + ConfigurarJugadores(J, C).
//   INICIAR_CONFIGURAR_REINICIAR lo mismo y despues Reiniciar.
//   REINICIAR_ANTES              antes Reiniciar; nada despues.
//   INICIAR_REINICIAR            antes Inicializar; despues Reiniciar.
//   REINICIAR_DESPUES            nada antes; despues Reiniciar.
// audio        : SI = recibe contexto.audio en su miembro .audio al activar.
// iaReconexion : SI = MantieneControlIAEnReconexion devuelve true.
// descarga     : SI = Descargar al cambiar de minijuego (DescargarMinijuego);
//                SOLO_AL_CERRAR = solo en GestorMinijuegos::Descargar.

#define LISTA_MINIJUEGOS(X) \
    X(MINIJUEGO_COLOR_SEGURO,         MinijuegoColorSeguro,        minijuegoColor,               INICIAR_CONFIGURAR,           JC,  JCPX,  JCPXD, SI, NO, NO) \
    X(MINIJUEGO_PELOTAS,              MinijuegoPelotas,            minijuegoPelotas,             INICIAR_CONFIGURAR,           JC,  JCPX,  JCPD,  SI, NO, NO) \
    X(MINIJUEGO_TRONCO,               MinijuegoTronco,             minijuegoTronco,              INICIAR_CONFIGURAR_REINICIAR, PC,  CP,    JCP,   SI, NO, NO) \
    X(MINIJUEGO_FABRICA_67,           Minijuego67,                 minijuego67,                  INICIAR_REINICIAR,            PC,  CP,    CP,    SI, NO, SOLO_AL_CERRAR) \
    X(MINIJUEGO_ISLA_FUEGO,           MinijuegoIslaFuego,          minijuegoIslaFuego,           INICIAR_CONFIGURAR,           JC,  JCPX,  JCPXD, NO, NO, NO) \
    X(MINIJUEGO_CAPITAN_MANDA,        MinijuegoCapitanManda,       minijuegoCapitanManda,        REINICIAR_ANTES,              P,   P,     P,     SI, NO, NO) \
    X(MINIJUEGO_BARRA_GIRATORIA,      MinijuegoBarraGiratoria,     minijuegoBarraGiratoria,      INICIAR_CONFIGURAR,           JC,  JCPX,  JCPXD, SI, NO, NO) \
    X(MINIJUEGO_NUCLEOS_ENERGIA,      MinijuegoNucleosEnergia,     minijuegoNucleosEnergia,      INICIAR_CONFIGURAR,           JC,  JCPXA, JCPD,  NO, NO, NO) \
    X(MINIJUEGO_REFUGIO_PINCHOS,      MinijuegoRefugioPinchos,     minijuegoRefugioPinchos,      INICIAR_REINICIAR,            JPC, JCPX,  JCPD,  SI, NO, NO) \
    X(MINIJUEGO_MIRADAS_CRUZADAS,     MinijuegoMiradasCruzadas,    minijuegoMiradasCruzadas,     INICIAR_REINICIAR,            P,   P,     P,     SI, NO, NO) \
    X(MINIJUEGO_MUROS_LOCOS,          MinijuegoMurosLocos,         minijuegoMurosLocos,          INICIAR_CONFIGURAR,           JC,  JCPX,  JCPXD, SI, NO, NO) \
    X(MINIJUEGO_TORMENTA_MAGNETICA,   MinijuegoTormentaMagnetica,  minijuegoTormentaMagnetica,   INICIAR_CONFIGURAR,           JC,  JCPX,  JCPXD, SI, NO, NO) \
    X(MINIJUEGO_CONTEO_EXPLOSIVO,     MinijuegoConteoExplosivo,    minijuegoConteoExplosivo,     REINICIAR_ANTES,              P,   P,     P,     SI, NO, NO) \
    X(MINIJUEGO_PASO_SILENCIOSO,      MinijuegoPasoSilencioso,     minijuegoPasoSilencioso,      REINICIAR_ANTES,              P,   P,     P,     SI, NO, NO) \
    X(MINIJUEGO_CIRCUITO_VOLTAJE,     MinijuegoCircuitoVoltaje,    minijuegoCircuitoVoltaje,     REINICIAR_ANTES,              P,   P,     P,     SI, NO, NO) \
    X(MINIJUEGO_TRAZO_PERFECTO,       MinijuegoTrazoPerfecto,      minijuegoTrazoPerfecto,       REINICIAR_ANTES,              P,   P,     P,     SI, NO, NO) \
    X(MINIJUEGO_CARGA_INESTABLE,      MinijuegoCargaInestable,     minijuegoCargaInestable,      REINICIAR_ANTES,              P,   P,     P,     SI, NO, NO) \
    X(MINIJUEGO_SECUENCIA_NEON,       MinijuegoSecuenciaNeon,      minijuegoSecuenciaNeon,       REINICIAR_ANTES,              P,   P,     P,     SI, NO, NO) \
    X(MINIJUEGO_INTERRUPTORES_CAOS,   MinijuegoInterruptoresCaos,  minijuegoInterruptoresCaos,   REINICIAR_ANTES,              P,   P,     P,     SI, NO, NO) \
    X(MINIJUEGO_TANQUES_PLASMA,       MinijuegoTanquesPlasma,      minijuegoTanquesPlasma,       REINICIAR_ANTES,              P,   P,     P,     SI, NO, NO) \
    X(MINIJUEGO_PASARELAS_VACIO,      MinijuegoPasarelasVacio,     minijuegoPasarelasVacio,      REINICIAR_DESPUES,            JPC, JCPX,  JCPXD, SI, SI, NO) \
    X(MINIJUEGO_CANTERA_FUGA,         MinijuegoCanteraFuga,        minijuegoCanteraFuga,         REINICIAR_DESPUES,            JPC, JCP,   JCPD,  SI, SI, NO) \
    X(MINIJUEGO_TERRITORIO_CONQUISTA, MinijuegoTerritorioConquista, minijuegoTerritorioConquista, INICIAR_REINICIAR,           JPC, JCP,   JCPD,  SI, SI, NO) \
    X(MINIJUEGO_DEFENSA_NUCLEO,       MinijuegoDefensaNucleo,      minijuegoDefensaNucleo,       INICIAR_REINICIAR,            JPC, JCP,   JCPD,  SI, SI, NO) \
    X(MINIJUEGO_LLUVIA_APILADA,       MinijuegoLluviaApilada,      minijuegoLluviaApilada,       INICIAR_REINICIAR,            JPC, JCP,   JCPD,  SI, SI, NO) \
    X(MINIJUEGO_CUERDA_ACANTILADO,    MinijuegoCuerdaAcantilado,   minijuegoCuerdaAcantilado,    INICIAR_REINICIAR,            JPC, JCP,   JCPD,  SI, SI, NO) \
    X(MINIJUEGO_ULTIMO_ASIENTO,       MinijuegoUltimoAsiento,      minijuegoUltimoAsiento,       INICIAR_REINICIAR,            JPC, JCP,   JCPD,  SI, SI, NO) \
    X(MINIJUEGO_CAJAS_PUERTO,         MinijuegoCajasPuerto,        minijuegoCajasPuerto,         INICIAR_REINICIAR,            JPC, JCP,   JCPD,  SI, SI, NO) \
    X(MINIJUEGO_LABERINTO_INCLINADO,  MinijuegoLaberintoInclinado, minijuegoLaberintoInclinado,  INICIAR_REINICIAR,            JPC, JCP,   JCPD,  SI, SI, NO) \
    X(MINIJUEGO_VETA_CRISTAL,         MinijuegoVetaCristal,        minijuegoVetaCristal,         INICIAR_REINICIAR,            JPC, JCP,   JCPD,  SI, SI, NO) \
    X(MINIJUEGO_CAPSULAS_BARAJADAS,   MinijuegoCapsulasBarajadas,  minijuegoCapsulasBarajadas,   INICIAR_REINICIAR,            JPC, JCP,   JCPD,  SI, SI, NO) \
    X(MINIJUEGO_BATEO_METEORICO,      MinijuegoBateoMeteorico,     minijuegoBateoMeteorico,      INICIAR_REINICIAR,            JPC, JCP,   JCPD,  SI, SI, NO) \
    X(MINIJUEGO_RACIMO_TOXICO,        MinijuegoRacimoToxico,       minijuegoRacimoToxico,        INICIAR_REINICIAR,            JPC, JCP,   JCPD,  SI, SI, NO) \
    X(MINIJUEGO_TESORERO_ACORRALADO,  MinijuegoTesoreroAcorralado, minijuegoTesoreroAcorralado,  INICIAR_REINICIAR,            JPC, JCP,   JCPD,  SI, SI, NO) \
    X(MINIJUEGO_DESCENSO_NUBES,       MinijuegoDescensoNubes,      minijuegoDescensoNubes,       INICIAR_REINICIAR,            JPC, JCP,   JCPD,  SI, SI, NO) \
    X(MINIJUEGO_VOLEA_MAGMA,          MinijuegoVoleaMagma,         minijuegoVoleaMagma,          INICIAR_REINICIAR,            JPC, JCP,   JCPD,  SI, SI, NO) \
    X(MINIJUEGO_PAREJAS_GLACIAR,      MinijuegoParejasGlaciar,     minijuegoParejasGlaciar,      INICIAR_REINICIAR,            JPC, JCP,   JCPD,  SI, SI, NO) \
    X(MINIJUEGO_ESFERAS_CANON,        MinijuegoEsferasCanon,       minijuegoEsferasCanon,        INICIAR_REINICIAR,            JPC, JCP,   JCPD,  SI, SI, SI) \
    X(MINIJUEGO_PESCA_ISLA,           MinijuegoPescaIsla,          minijuegoPescaIsla,           INICIAR_REINICIAR,            JPC, JCP,   JCPD,  SI, SI, SI) \
    X(MINIJUEGO_RODILLOS_NEON,        MinijuegoRodillosNeon,       minijuegoRodillosNeon,        INICIAR_REINICIAR,            JPC, JCP,   JCPD,  SI, SI, SI) \
    X(MINIJUEGO_BOLAS_AZUCAR,         MinijuegoBolasAzucar,        minijuegoBolasAzucar,         INICIAR_REINICIAR,            JPC, JCP,   JCPD,  SI, SI, SI) \
    X(MINIJUEGO_GRUA_CHATARRA,        MinijuegoGruaChatarra,       minijuegoGruaChatarra,        INICIAR_REINICIAR,            JPC, JCP,   JCPD,  SI, SI, SI) \
    X(MINIJUEGO_PISOTON_PLAGAS,       MinijuegoPisotonPlagas,      minijuegoPisotonPlagas,       INICIAR_REINICIAR,            JPC, JCP,   JCPD,  SI, SI, SI) \
    X(MINIJUEGO_BALSAS_RAPIDO,        MinijuegoBalsasRapido,       minijuegoBalsasRapido,        INICIAR_REINICIAR,            JPC, JCP,   JCPD,  SI, SI, SI) \
    X(MINIJUEGO_AUTOS_GLOBO,          MinijuegoAutosGlobo,         minijuegoAutosGlobo,          INICIAR_REINICIAR,            JPC, JCP,   JCPD,  SI, SI, SI) \
    X(MINIJUEGO_SENDERO_INVISIBLE,    MinijuegoSenderoInvisible,   minijuegoSenderoInvisible,    INICIAR_REINICIAR,            JPC, JCP,   JCPD,  SI, SI, SI) \
    X(MINIJUEGO_BANQUETE_TURBO,       MinijuegoBanqueteTurbo,      minijuegoBanqueteTurbo,       INICIAR_REINICIAR,            JPC, JCP,   JCPD,  SI, SI, SI) \
    X(MINIJUEGO_TUBERIAS_DESIERTO,    MinijuegoTuberiasDesierto,   minijuegoTuberiasDesierto,    INICIAR_REINICIAR,            JPC, JCP,   JCPD,  SI, SI, SI) \
    X(MINIJUEGO_TREPA_MASTIL,         MinijuegoTrepaMastil,        minijuegoTrepaMastil,         INICIAR_REINICIAR,            JPC, JCP,   JCPD,  SI, SI, SI) \
    X(MINIJUEGO_GUARDIAN_RUINAS,      MinijuegoGuardianRuinas,     minijuegoGuardianRuinas,      INICIAR_REINICIAR,            JPC, JCP,   JCPD,  SI, SI, SI) \
    X(MINIJUEGO_CABLES_CRUZADOS,      MinijuegoCablesCruzados,     minijuegoCablesCruzados,      INICIAR_REINICIAR,            JPC, JCP,   JCPD,  SI, SI, NO) \
    X(MINIJUEGO_VAGONETAS_DUELO,      MinijuegoVagonetasDuelo,     minijuegoVagonetasDuelo,      INICIAR_REINICIAR,            JPC, JCP,   JCPD,  SI, SI, NO) \
    X(MINIJUEGO_GEMAS_DESLIZANTES,    MinijuegoGemasDeslizantes,   minijuegoGemasDeslizantes,    INICIAR_REINICIAR,            JPC, JCP,   JCPD,  SI, SI, NO)
