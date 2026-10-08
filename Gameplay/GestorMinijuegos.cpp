#include "Gameplay/GestorMinijuegos.h"

#include "Minigames/EfectosVisualesMinijuegos.h"
#include "Minigames/UtilidadesMinijuegos.h"


void GestorMinijuegos::Inicializar()
{
    minijuegoColor.Inicializar();
    minijuegoPelotas.Inicializar();
    minijuegoTronco.Inicializar();
    minijuego67.Inicializar();
    minijuegoIslaFuego.Inicializar();
    minijuegoCapitanManda.Inicializar();
    minijuegoBarraGiratoria.Inicializar();
    minijuegoNucleosEnergia.Inicializar();
    minijuegoRefugioPinchos.Inicializar();
    minijuegoMiradasCruzadas.Inicializar();
    minijuegoMurosLocos.Inicializar();
    minijuegoTormentaMagnetica.Inicializar();
    minijuegoConteoExplosivo.Inicializar();
    minijuegoPasoSilencioso.Inicializar();
    minijuegoCircuitoVoltaje.Inicializar();
    minijuegoTrazoPerfecto.Inicializar();
    minijuegoCargaInestable.Inicializar();
    minijuegoSecuenciaNeon.Inicializar();
    minijuegoInterruptoresCaos.Inicializar();
    minijuegoTanquesPlasma.Inicializar();
    minijuegoPasarelasVacio.Inicializar();
    minijuegoCanteraFuga.Inicializar();
    minijuegoTerritorioConquista.Inicializar();
    minijuegoDefensaNucleo.Inicializar();
    minijuegoLluviaApilada.Inicializar();
    minijuegoCuerdaAcantilado.Inicializar();
    minijuegoUltimoAsiento.Inicializar();
    minijuegoCajasPuerto.Inicializar();
    minijuegoLaberintoInclinado.Inicializar();
    minijuegoVetaCristal.Inicializar();
    minijuegoCapsulasBarajadas.Inicializar();
    minijuegoBateoMeteorico.Inicializar();
    minijuegoRacimoToxico.Inicializar();
    minijuegoTesoreroAcorralado.Inicializar();
    minijuegoDescensoNubes.Inicializar();
    minijuegoVoleaMagma.Inicializar();
    minijuegoParejasGlaciar.Inicializar();
    minijuegoEsferasCanon.Inicializar();
    minijuegoPescaIsla.Inicializar();
    minijuegoRodillosNeon.Inicializar();
    minijuegoBolasAzucar.Inicializar();
    minijuegoGruaChatarra.Inicializar();
    minijuegoPisotonPlagas.Inicializar();
    minijuegoBalsasRapido.Inicializar();
    minijuegoAutosGlobo.Inicializar();
    minijuegoSenderoInvisible.Inicializar();
    minijuegoBanqueteTurbo.Inicializar();
    minijuegoTuberiasDesierto.Inicializar();
    minijuegoTrepaMastil.Inicializar();
    minijuegoGuardianRuinas.Inicializar();
}


void GestorMinijuegos::ActivarMinijuego(
    IdMinijuego id,
    ContextoMinijuego& contexto
)
{
    if (!EsIdMinijuegoValido(id))
    {
        return;
    }

    minijuegoActivo = id;

    // Los minijuegos con audio propio reciben el sistema compartido.
    minijuegoTerritorioConquista.audio = contexto.audio;
    minijuegoDefensaNucleo.audio = contexto.audio;
    minijuegoLluviaApilada.audio = contexto.audio;
    minijuegoCuerdaAcantilado.audio = contexto.audio;
    minijuegoUltimoAsiento.audio = contexto.audio;
    minijuegoCajasPuerto.audio = contexto.audio;
    minijuegoLaberintoInclinado.audio = contexto.audio;
    minijuegoVetaCristal.audio = contexto.audio;
    minijuegoCapsulasBarajadas.audio = contexto.audio;
    minijuegoBateoMeteorico.audio = contexto.audio;
    minijuegoRacimoToxico.audio = contexto.audio;
    minijuegoTesoreroAcorralado.audio = contexto.audio;
    minijuegoDescensoNubes.audio = contexto.audio;
    minijuegoVoleaMagma.audio = contexto.audio;
    minijuegoParejasGlaciar.audio = contexto.audio;
    minijuegoEsferasCanon.audio = contexto.audio;
    minijuegoPescaIsla.audio = contexto.audio;
    minijuegoRodillosNeon.audio = contexto.audio;
    minijuegoBolasAzucar.audio = contexto.audio;
    minijuegoGruaChatarra.audio = contexto.audio;
    minijuegoTanquesPlasma.audio = contexto.audio;
    minijuegoCircuitoVoltaje.audio = contexto.audio;
    minijuegoCargaInestable.audio = contexto.audio;
    minijuegoSecuenciaNeon.audio = contexto.audio;
    minijuegoPisotonPlagas.audio = contexto.audio;
    minijuegoBalsasRapido.audio = contexto.audio;
    minijuegoTrazoPerfecto.audio = contexto.audio;
    minijuegoInterruptoresCaos.audio = contexto.audio;
    minijuegoMiradasCruzadas.audio = contexto.audio;
    minijuegoAutosGlobo.audio = contexto.audio;
    minijuegoSenderoInvisible.audio = contexto.audio;
    minijuegoPelotas.audio = contexto.audio;
    minijuegoCapitanManda.audio = contexto.audio;
    minijuegoConteoExplosivo.audio = contexto.audio;
    minijuegoPasoSilencioso.audio = contexto.audio;
    minijuegoBanqueteTurbo.audio = contexto.audio;
    minijuegoTuberiasDesierto.audio = contexto.audio;
    minijuegoTrepaMastil.audio = contexto.audio;
    minijuegoGuardianRuinas.audio = contexto.audio;
    minijuegoTormentaMagnetica.audio = contexto.audio;
    minijuegoRefugioPinchos.audio = contexto.audio;

    switch (minijuegoActivo)
    {
        case MINIJUEGO_COLOR_SEGURO:
            minijuegoColor.Inicializar();
            minijuegoColor.ConfigurarJugadores(
                contexto.jugadores,
                contexto.cantidadJugadores
            );
            break;

        case MINIJUEGO_PELOTAS:
            minijuegoPelotas.Inicializar();
            minijuegoPelotas.ConfigurarJugadores(
                contexto.jugadores,
                contexto.cantidadJugadores
            );
            break;

        case MINIJUEGO_TRONCO:
            minijuegoTronco.Inicializar();
            minijuegoTronco.ConfigurarJugadores(
                contexto.jugadores,
                contexto.cantidadJugadores
            );
            break;

        case MINIJUEGO_FABRICA_67:
            minijuego67.Inicializar();
            break;

        case MINIJUEGO_ISLA_FUEGO:
            minijuegoIslaFuego.Inicializar();
            minijuegoIslaFuego.ConfigurarJugadores(
                contexto.jugadores,
                contexto.cantidadJugadores
            );
            break;

        case MINIJUEGO_CAPITAN_MANDA:
            minijuegoCapitanManda.Reiniciar(
                contexto.participantes
            );
            break;

        case MINIJUEGO_BARRA_GIRATORIA:
            minijuegoBarraGiratoria.Inicializar();
            minijuegoBarraGiratoria.ConfigurarJugadores(
                contexto.jugadores,
                contexto.cantidadJugadores
            );
            break;

        case MINIJUEGO_NUCLEOS_ENERGIA:
            minijuegoNucleosEnergia.Inicializar();
            minijuegoNucleosEnergia.ConfigurarJugadores(
                contexto.jugadores,
                contexto.cantidadJugadores
            );
            break;

        case MINIJUEGO_REFUGIO_PINCHOS:
            minijuegoRefugioPinchos.Inicializar();
            break;

        case MINIJUEGO_MIRADAS_CRUZADAS:
            minijuegoMiradasCruzadas.Inicializar();
            break;

        case MINIJUEGO_MUROS_LOCOS:
            minijuegoMurosLocos.Inicializar();
            minijuegoMurosLocos.ConfigurarJugadores(
                contexto.jugadores,
                contexto.cantidadJugadores
            );
            break;

        case MINIJUEGO_TORMENTA_MAGNETICA:
            minijuegoTormentaMagnetica.Inicializar();
            minijuegoTormentaMagnetica.ConfigurarJugadores(
                contexto.jugadores,
                contexto.cantidadJugadores
            );
            break;

        case MINIJUEGO_CONTEO_EXPLOSIVO:
            minijuegoConteoExplosivo.Reiniciar(
                contexto.participantes
            );
            break;

        case MINIJUEGO_PASO_SILENCIOSO:
            minijuegoPasoSilencioso.Reiniciar(
                contexto.participantes
            );
            break;

        case MINIJUEGO_CIRCUITO_VOLTAJE:
            minijuegoCircuitoVoltaje.Reiniciar(
                contexto.participantes
            );
            break;

        case MINIJUEGO_TRAZO_PERFECTO:
            minijuegoTrazoPerfecto.Reiniciar(
                contexto.participantes
            );
            break;

        case MINIJUEGO_CARGA_INESTABLE:
            minijuegoCargaInestable.Reiniciar(
                contexto.participantes
            );
            break;

        case MINIJUEGO_SECUENCIA_NEON:
            minijuegoSecuenciaNeon.Reiniciar(
                contexto.participantes
            );
            break;

        case MINIJUEGO_INTERRUPTORES_CAOS:
            minijuegoInterruptoresCaos.Reiniciar(
                contexto.participantes
            );
            break;

        case MINIJUEGO_TANQUES_PLASMA:
            minijuegoTanquesPlasma.Reiniciar(
                contexto.participantes
            );
            break;

        case MINIJUEGO_PASARELAS_VACIO:
        case MINIJUEGO_CANTERA_FUGA:
            break;

        case MINIJUEGO_TERRITORIO_CONQUISTA:
            minijuegoTerritorioConquista.Inicializar();
            break;

        case MINIJUEGO_DEFENSA_NUCLEO:
            minijuegoDefensaNucleo.Inicializar();
            break;

        case MINIJUEGO_LLUVIA_APILADA:
            minijuegoLluviaApilada.Inicializar();
            break;

        case MINIJUEGO_CUERDA_ACANTILADO:
            minijuegoCuerdaAcantilado.Inicializar();
            break;

        case MINIJUEGO_ULTIMO_ASIENTO:
            minijuegoUltimoAsiento.Inicializar();
            break;

        case MINIJUEGO_CAJAS_PUERTO:
            minijuegoCajasPuerto.Inicializar();
            break;

        case MINIJUEGO_LABERINTO_INCLINADO:
            minijuegoLaberintoInclinado.Inicializar();
            break;

        case MINIJUEGO_VETA_CRISTAL:
            minijuegoVetaCristal.Inicializar();
            break;

        case MINIJUEGO_CAPSULAS_BARAJADAS:
            minijuegoCapsulasBarajadas.Inicializar();
            break;

        case MINIJUEGO_BATEO_METEORICO:
            minijuegoBateoMeteorico.Inicializar();
            break;

        case MINIJUEGO_RACIMO_TOXICO:
            minijuegoRacimoToxico.Inicializar();
            break;

        case MINIJUEGO_TESORERO_ACORRALADO:
            minijuegoTesoreroAcorralado.Inicializar();
            break;

        case MINIJUEGO_DESCENSO_NUBES:
            minijuegoDescensoNubes.Inicializar();
            break;

        case MINIJUEGO_VOLEA_MAGMA:
            minijuegoVoleaMagma.Inicializar();
            break;

        case MINIJUEGO_PAREJAS_GLACIAR:
            minijuegoParejasGlaciar.Inicializar();
            break;

        case MINIJUEGO_ESFERAS_CANON:
            minijuegoEsferasCanon.Inicializar();
            break;

        case MINIJUEGO_PESCA_ISLA:
            minijuegoPescaIsla.Inicializar();
            break;

        case MINIJUEGO_RODILLOS_NEON:
            minijuegoRodillosNeon.Inicializar();
            break;

        case MINIJUEGO_BOLAS_AZUCAR:
            minijuegoBolasAzucar.Inicializar();
            break;

        case MINIJUEGO_GRUA_CHATARRA:
            minijuegoGruaChatarra.Inicializar();
            break;

        case MINIJUEGO_PISOTON_PLAGAS:
            minijuegoPisotonPlagas.Inicializar();
            break;

        case MINIJUEGO_BALSAS_RAPIDO:
            minijuegoBalsasRapido.Inicializar();
            break;

        case MINIJUEGO_AUTOS_GLOBO:
            minijuegoAutosGlobo.Inicializar();
            break;

        case MINIJUEGO_SENDERO_INVISIBLE:
            minijuegoSenderoInvisible.Inicializar();
            break;

        case MINIJUEGO_BANQUETE_TURBO:
            minijuegoBanqueteTurbo.Inicializar();
            break;

        case MINIJUEGO_TUBERIAS_DESIERTO:
            minijuegoTuberiasDesierto.Inicializar();
            break;

        case MINIJUEGO_TREPA_MASTIL:
            minijuegoTrepaMastil.Inicializar();
            break;

        case MINIJUEGO_GUARDIAN_RUINAS:
            minijuegoGuardianRuinas.Inicializar();
            break;

        case CANTIDAD_MINIJUEGOS:
            break;
    }

    ReiniciarJugadoresCompartidos(contexto);

    switch (minijuegoActivo)
    {
        case MINIJUEGO_TRONCO:
            minijuegoTronco.Reiniciar(
                contexto.participantes,
                contexto.cantidadJugadores
            );
            break;

        case MINIJUEGO_FABRICA_67:
            minijuego67.Reiniciar(
                contexto.participantes,
                contexto.cantidadJugadores
            );
            break;

        case MINIJUEGO_REFUGIO_PINCHOS:
            minijuegoRefugioPinchos.Reiniciar(
                contexto.jugadores,
                contexto.participantes,
                contexto.cantidadJugadores
            );
            break;

        case MINIJUEGO_MIRADAS_CRUZADAS:
            minijuegoMiradasCruzadas.Reiniciar(
                contexto.participantes
            );
            break;

        case MINIJUEGO_PASARELAS_VACIO:
            minijuegoPasarelasVacio.Reiniciar(
                contexto.jugadores,
                contexto.participantes,
                contexto.cantidadJugadores
            );
            break;

        case MINIJUEGO_CANTERA_FUGA:
            minijuegoCanteraFuga.Reiniciar(
                contexto.jugadores,
                contexto.participantes,
                contexto.cantidadJugadores
            );
            break;

        case MINIJUEGO_TERRITORIO_CONQUISTA:
            minijuegoTerritorioConquista.Reiniciar(
                contexto.jugadores,
                contexto.participantes,
                contexto.cantidadJugadores
            );
            break;

        case MINIJUEGO_DEFENSA_NUCLEO:
            minijuegoDefensaNucleo.Reiniciar(
                contexto.jugadores,
                contexto.participantes,
                contexto.cantidadJugadores
            );
            break;

        case MINIJUEGO_LLUVIA_APILADA:
            minijuegoLluviaApilada.Reiniciar(
                contexto.jugadores,
                contexto.participantes,
                contexto.cantidadJugadores
            );
            break;

        case MINIJUEGO_CUERDA_ACANTILADO:
            minijuegoCuerdaAcantilado.Reiniciar(
                contexto.jugadores,
                contexto.participantes,
                contexto.cantidadJugadores
            );
            break;

        case MINIJUEGO_ULTIMO_ASIENTO:
            minijuegoUltimoAsiento.Reiniciar(
                contexto.jugadores,
                contexto.participantes,
                contexto.cantidadJugadores
            );
            break;

        case MINIJUEGO_CAJAS_PUERTO:
            minijuegoCajasPuerto.Reiniciar(
                contexto.jugadores,
                contexto.participantes,
                contexto.cantidadJugadores
            );
            break;

        case MINIJUEGO_LABERINTO_INCLINADO:
            minijuegoLaberintoInclinado.Reiniciar(
                contexto.jugadores,
                contexto.participantes,
                contexto.cantidadJugadores
            );
            break;

        case MINIJUEGO_VETA_CRISTAL:
            minijuegoVetaCristal.Reiniciar(
                contexto.jugadores,
                contexto.participantes,
                contexto.cantidadJugadores
            );
            break;

        case MINIJUEGO_CAPSULAS_BARAJADAS:
            minijuegoCapsulasBarajadas.Reiniciar(
                contexto.jugadores,
                contexto.participantes,
                contexto.cantidadJugadores
            );
            break;

        case MINIJUEGO_BATEO_METEORICO:
            minijuegoBateoMeteorico.Reiniciar(
                contexto.jugadores,
                contexto.participantes,
                contexto.cantidadJugadores
            );
            break;

        case MINIJUEGO_RACIMO_TOXICO:
            minijuegoRacimoToxico.Reiniciar(
                contexto.jugadores,
                contexto.participantes,
                contexto.cantidadJugadores
            );
            break;

        case MINIJUEGO_TESORERO_ACORRALADO:
            minijuegoTesoreroAcorralado.Reiniciar(
                contexto.jugadores,
                contexto.participantes,
                contexto.cantidadJugadores
            );
            break;

        case MINIJUEGO_DESCENSO_NUBES:
            minijuegoDescensoNubes.Reiniciar(
                contexto.jugadores,
                contexto.participantes,
                contexto.cantidadJugadores
            );
            break;

        case MINIJUEGO_VOLEA_MAGMA:
            minijuegoVoleaMagma.Reiniciar(
                contexto.jugadores,
                contexto.participantes,
                contexto.cantidadJugadores
            );
            break;

        case MINIJUEGO_PAREJAS_GLACIAR:
            minijuegoParejasGlaciar.Reiniciar(
                contexto.jugadores,
                contexto.participantes,
                contexto.cantidadJugadores
            );
            break;

        case MINIJUEGO_ESFERAS_CANON:
            minijuegoEsferasCanon.Reiniciar(
                contexto.jugadores,
                contexto.participantes,
                contexto.cantidadJugadores
            );
            break;

        case MINIJUEGO_PESCA_ISLA:
            minijuegoPescaIsla.Reiniciar(
                contexto.jugadores,
                contexto.participantes,
                contexto.cantidadJugadores
            );
            break;

        case MINIJUEGO_RODILLOS_NEON:
            minijuegoRodillosNeon.Reiniciar(
                contexto.jugadores,
                contexto.participantes,
                contexto.cantidadJugadores
            );
            break;

        case MINIJUEGO_BOLAS_AZUCAR:
            minijuegoBolasAzucar.Reiniciar(
                contexto.jugadores,
                contexto.participantes,
                contexto.cantidadJugadores
            );
            break;

        case MINIJUEGO_GRUA_CHATARRA:
            minijuegoGruaChatarra.Reiniciar(
                contexto.jugadores,
                contexto.participantes,
                contexto.cantidadJugadores
            );
            break;

        case MINIJUEGO_PISOTON_PLAGAS:
            minijuegoPisotonPlagas.Reiniciar(
                contexto.jugadores,
                contexto.participantes,
                contexto.cantidadJugadores
            );
            break;

        case MINIJUEGO_BALSAS_RAPIDO:
            minijuegoBalsasRapido.Reiniciar(
                contexto.jugadores,
                contexto.participantes,
                contexto.cantidadJugadores
            );
            break;

        case MINIJUEGO_AUTOS_GLOBO:
            minijuegoAutosGlobo.Reiniciar(
                contexto.jugadores,
                contexto.participantes,
                contexto.cantidadJugadores
            );
            break;

        case MINIJUEGO_SENDERO_INVISIBLE:
            minijuegoSenderoInvisible.Reiniciar(
                contexto.jugadores,
                contexto.participantes,
                contexto.cantidadJugadores
            );
            break;

        case MINIJUEGO_BANQUETE_TURBO:
            minijuegoBanqueteTurbo.Reiniciar(
                contexto.jugadores,
                contexto.participantes,
                contexto.cantidadJugadores
            );
            break;

        case MINIJUEGO_TUBERIAS_DESIERTO:
            minijuegoTuberiasDesierto.Reiniciar(
                contexto.jugadores,
                contexto.participantes,
                contexto.cantidadJugadores
            );
            break;

        case MINIJUEGO_TREPA_MASTIL:
            minijuegoTrepaMastil.Reiniciar(
                contexto.jugadores,
                contexto.participantes,
                contexto.cantidadJugadores
            );
            break;

        case MINIJUEGO_GUARDIAN_RUINAS:
            minijuegoGuardianRuinas.Reiniciar(
                contexto.jugadores,
                contexto.participantes,
                contexto.cantidadJugadores
            );
            break;

        default:
            break;
    }
}


void GestorMinijuegos::ReiniciarActivo(
    ContextoMinijuego& contexto
)
{
    switch (minijuegoActivo)
    {
        case MINIJUEGO_COLOR_SEGURO:
            minijuegoColor.Reiniciar(
                contexto.jugadores,
                contexto.cantidadJugadores
            );
            break;

        case MINIJUEGO_PELOTAS:
            minijuegoPelotas.Reiniciar(
                contexto.jugadores,
                contexto.cantidadJugadores
            );
            break;

        case MINIJUEGO_TRONCO:
            minijuegoTronco.Reiniciar(
                contexto.participantes,
                contexto.cantidadJugadores
            );
            break;

        case MINIJUEGO_FABRICA_67:
            minijuego67.Reiniciar(
                contexto.participantes,
                contexto.cantidadJugadores
            );
            break;

        case MINIJUEGO_ISLA_FUEGO:
            minijuegoIslaFuego.Reiniciar(
                contexto.jugadores,
                contexto.cantidadJugadores
            );
            break;

        case MINIJUEGO_CAPITAN_MANDA:
            minijuegoCapitanManda.Reiniciar(
                contexto.participantes
            );
            break;

        case MINIJUEGO_BARRA_GIRATORIA:
            minijuegoBarraGiratoria.Reiniciar(
                contexto.jugadores,
                contexto.cantidadJugadores
            );
            break;

        case MINIJUEGO_NUCLEOS_ENERGIA:
            minijuegoNucleosEnergia.Reiniciar(
                contexto.jugadores,
                contexto.cantidadJugadores
            );
            break;

        case MINIJUEGO_REFUGIO_PINCHOS:
            minijuegoRefugioPinchos.Reiniciar(
                contexto.jugadores,
                contexto.participantes,
                contexto.cantidadJugadores
            );
            break;

        case MINIJUEGO_MIRADAS_CRUZADAS:
            minijuegoMiradasCruzadas.Reiniciar(
                contexto.participantes
            );
            break;

        case MINIJUEGO_MUROS_LOCOS:
            minijuegoMurosLocos.Reiniciar(
                contexto.jugadores,
                contexto.cantidadJugadores
            );
            break;

        case MINIJUEGO_TORMENTA_MAGNETICA:
            minijuegoTormentaMagnetica.Reiniciar(
                contexto.jugadores,
                contexto.cantidadJugadores
            );
            break;

        case MINIJUEGO_CONTEO_EXPLOSIVO:
            minijuegoConteoExplosivo.Reiniciar(
                contexto.participantes
            );
            break;

        case MINIJUEGO_PASO_SILENCIOSO:
            minijuegoPasoSilencioso.Reiniciar(
                contexto.participantes
            );
            break;

        case MINIJUEGO_CIRCUITO_VOLTAJE:
            minijuegoCircuitoVoltaje.Reiniciar(
                contexto.participantes
            );
            break;

        case MINIJUEGO_TRAZO_PERFECTO:
            minijuegoTrazoPerfecto.Reiniciar(
                contexto.participantes
            );
            break;

        case MINIJUEGO_CARGA_INESTABLE:
            minijuegoCargaInestable.Reiniciar(
                contexto.participantes
            );
            break;

        case MINIJUEGO_SECUENCIA_NEON:
            minijuegoSecuenciaNeon.Reiniciar(
                contexto.participantes
            );
            break;

        case MINIJUEGO_INTERRUPTORES_CAOS:
            minijuegoInterruptoresCaos.Reiniciar(
                contexto.participantes
            );
            break;

        case MINIJUEGO_TANQUES_PLASMA:
            minijuegoTanquesPlasma.Reiniciar(
                contexto.participantes
            );
            break;

        case MINIJUEGO_PASARELAS_VACIO:
            minijuegoPasarelasVacio.Reiniciar(
                contexto.jugadores,
                contexto.participantes,
                contexto.cantidadJugadores
            );
            break;

        case MINIJUEGO_CANTERA_FUGA:
            minijuegoCanteraFuga.Reiniciar(
                contexto.jugadores,
                contexto.participantes,
                contexto.cantidadJugadores
            );
            break;

        case MINIJUEGO_TERRITORIO_CONQUISTA:
            minijuegoTerritorioConquista.Reiniciar(
                contexto.jugadores,
                contexto.participantes,
                contexto.cantidadJugadores
            );
            break;

        case MINIJUEGO_DEFENSA_NUCLEO:
            minijuegoDefensaNucleo.Reiniciar(
                contexto.jugadores,
                contexto.participantes,
                contexto.cantidadJugadores
            );
            break;

        case MINIJUEGO_LLUVIA_APILADA:
            minijuegoLluviaApilada.Reiniciar(
                contexto.jugadores,
                contexto.participantes,
                contexto.cantidadJugadores
            );
            break;

        case MINIJUEGO_CUERDA_ACANTILADO:
            minijuegoCuerdaAcantilado.Reiniciar(
                contexto.jugadores,
                contexto.participantes,
                contexto.cantidadJugadores
            );
            break;

        case MINIJUEGO_ULTIMO_ASIENTO:
            minijuegoUltimoAsiento.Reiniciar(
                contexto.jugadores,
                contexto.participantes,
                contexto.cantidadJugadores
            );
            break;

        case MINIJUEGO_CAJAS_PUERTO:
            minijuegoCajasPuerto.Reiniciar(
                contexto.jugadores,
                contexto.participantes,
                contexto.cantidadJugadores
            );
            break;

        case MINIJUEGO_LABERINTO_INCLINADO:
            minijuegoLaberintoInclinado.Reiniciar(
                contexto.jugadores,
                contexto.participantes,
                contexto.cantidadJugadores
            );
            break;

        case MINIJUEGO_VETA_CRISTAL:
            minijuegoVetaCristal.Reiniciar(
                contexto.jugadores,
                contexto.participantes,
                contexto.cantidadJugadores
            );
            break;

        case MINIJUEGO_CAPSULAS_BARAJADAS:
            minijuegoCapsulasBarajadas.Reiniciar(
                contexto.jugadores,
                contexto.participantes,
                contexto.cantidadJugadores
            );
            break;

        case MINIJUEGO_BATEO_METEORICO:
            minijuegoBateoMeteorico.Reiniciar(
                contexto.jugadores,
                contexto.participantes,
                contexto.cantidadJugadores
            );
            break;

        case MINIJUEGO_RACIMO_TOXICO:
            minijuegoRacimoToxico.Reiniciar(
                contexto.jugadores,
                contexto.participantes,
                contexto.cantidadJugadores
            );
            break;

        case MINIJUEGO_TESORERO_ACORRALADO:
            minijuegoTesoreroAcorralado.Reiniciar(
                contexto.jugadores,
                contexto.participantes,
                contexto.cantidadJugadores
            );
            break;

        case MINIJUEGO_DESCENSO_NUBES:
            minijuegoDescensoNubes.Reiniciar(
                contexto.jugadores,
                contexto.participantes,
                contexto.cantidadJugadores
            );
            break;

        case MINIJUEGO_VOLEA_MAGMA:
            minijuegoVoleaMagma.Reiniciar(
                contexto.jugadores,
                contexto.participantes,
                contexto.cantidadJugadores
            );
            break;

        case MINIJUEGO_PAREJAS_GLACIAR:
            minijuegoParejasGlaciar.Reiniciar(
                contexto.jugadores,
                contexto.participantes,
                contexto.cantidadJugadores
            );
            break;

        case MINIJUEGO_ESFERAS_CANON:
            minijuegoEsferasCanon.Reiniciar(
                contexto.jugadores,
                contexto.participantes,
                contexto.cantidadJugadores
            );
            break;

        case MINIJUEGO_PESCA_ISLA:
            minijuegoPescaIsla.Reiniciar(
                contexto.jugadores,
                contexto.participantes,
                contexto.cantidadJugadores
            );
            break;

        case MINIJUEGO_RODILLOS_NEON:
            minijuegoRodillosNeon.Reiniciar(
                contexto.jugadores,
                contexto.participantes,
                contexto.cantidadJugadores
            );
            break;

        case MINIJUEGO_BOLAS_AZUCAR:
            minijuegoBolasAzucar.Reiniciar(
                contexto.jugadores,
                contexto.participantes,
                contexto.cantidadJugadores
            );
            break;

        case MINIJUEGO_GRUA_CHATARRA:
            minijuegoGruaChatarra.Reiniciar(
                contexto.jugadores,
                contexto.participantes,
                contexto.cantidadJugadores
            );
            break;

        case MINIJUEGO_PISOTON_PLAGAS:
            minijuegoPisotonPlagas.Reiniciar(
                contexto.jugadores,
                contexto.participantes,
                contexto.cantidadJugadores
            );
            break;

        case MINIJUEGO_BALSAS_RAPIDO:
            minijuegoBalsasRapido.Reiniciar(
                contexto.jugadores,
                contexto.participantes,
                contexto.cantidadJugadores
            );
            break;

        case MINIJUEGO_AUTOS_GLOBO:
            minijuegoAutosGlobo.Reiniciar(
                contexto.jugadores,
                contexto.participantes,
                contexto.cantidadJugadores
            );
            break;

        case MINIJUEGO_SENDERO_INVISIBLE:
            minijuegoSenderoInvisible.Reiniciar(
                contexto.jugadores,
                contexto.participantes,
                contexto.cantidadJugadores
            );
            break;

        case MINIJUEGO_BANQUETE_TURBO:
            minijuegoBanqueteTurbo.Reiniciar(
                contexto.jugadores,
                contexto.participantes,
                contexto.cantidadJugadores
            );
            break;

        case MINIJUEGO_TUBERIAS_DESIERTO:
            minijuegoTuberiasDesierto.Reiniciar(
                contexto.jugadores,
                contexto.participantes,
                contexto.cantidadJugadores
            );
            break;

        case MINIJUEGO_TREPA_MASTIL:
            minijuegoTrepaMastil.Reiniciar(
                contexto.jugadores,
                contexto.participantes,
                contexto.cantidadJugadores
            );
            break;

        case MINIJUEGO_GUARDIAN_RUINAS:
            minijuegoGuardianRuinas.Reiniciar(
                contexto.jugadores,
                contexto.participantes,
                contexto.cantidadJugadores
            );
            break;

        case CANTIDAD_MINIJUEGOS:
            break;
    }
}


void GestorMinijuegos::ActualizarActivo(
    float deltaTime,
    ContextoMinijuego& contexto
)
{
    switch (minijuegoActivo)
    {
        case MINIJUEGO_COLOR_SEGURO:
            minijuegoColor.Actualizar(
                deltaTime,
                contexto.jugadores,
                contexto.cantidadJugadores,
                contexto.participantes,
                contexto.particulas,
                contexto.cantidadParticulas
            );
            break;

        case MINIJUEGO_PELOTAS:
            minijuegoPelotas.Actualizar(
                deltaTime,
                contexto.jugadores,
                contexto.cantidadJugadores,
                contexto.participantes,
                contexto.particulas,
                contexto.cantidadParticulas
            );
            break;

        case MINIJUEGO_TRONCO:
            minijuegoTronco.Actualizar(
                deltaTime,
                contexto.cantidadJugadores,
                contexto.participantes
            );
            break;

        case MINIJUEGO_FABRICA_67:
            minijuego67.Actualizar(
                deltaTime,
                contexto.cantidadJugadores,
                contexto.participantes
            );
            break;

        case MINIJUEGO_ISLA_FUEGO:
            minijuegoIslaFuego.Actualizar(
                deltaTime,
                contexto.jugadores,
                contexto.cantidadJugadores,
                contexto.participantes,
                contexto.particulas,
                contexto.cantidadParticulas
            );
            break;

        case MINIJUEGO_CAPITAN_MANDA:
            minijuegoCapitanManda.Actualizar(
                deltaTime,
                contexto.participantes
            );
            break;

        case MINIJUEGO_BARRA_GIRATORIA:
            minijuegoBarraGiratoria.Actualizar(
                deltaTime,
                contexto.jugadores,
                contexto.cantidadJugadores,
                contexto.participantes,
                contexto.particulas,
                contexto.cantidadParticulas
            );
            break;

        case MINIJUEGO_NUCLEOS_ENERGIA:
            minijuegoNucleosEnergia.Actualizar(
                deltaTime,
                contexto.jugadores,
                contexto.cantidadJugadores,
                contexto.participantes,
                contexto.particulas,
                contexto.cantidadParticulas,
                contexto.audio
            );
            break;

        case MINIJUEGO_REFUGIO_PINCHOS:
            minijuegoRefugioPinchos.Actualizar(
                deltaTime,
                contexto.jugadores,
                contexto.cantidadJugadores,
                contexto.participantes,
                contexto.particulas,
                contexto.cantidadParticulas
            );
            break;

        case MINIJUEGO_MIRADAS_CRUZADAS:
            minijuegoMiradasCruzadas.Actualizar(
                deltaTime,
                contexto.participantes
            );
            break;

        case MINIJUEGO_MUROS_LOCOS:
            minijuegoMurosLocos.Actualizar(
                deltaTime,
                contexto.jugadores,
                contexto.cantidadJugadores,
                contexto.participantes,
                contexto.particulas,
                contexto.cantidadParticulas
            );
            break;

        case MINIJUEGO_TORMENTA_MAGNETICA:
            minijuegoTormentaMagnetica.Actualizar(
                deltaTime,
                contexto.jugadores,
                contexto.cantidadJugadores,
                contexto.participantes,
                contexto.particulas,
                contexto.cantidadParticulas
            );
            break;

        case MINIJUEGO_CONTEO_EXPLOSIVO:
            minijuegoConteoExplosivo.Actualizar(
                deltaTime,
                contexto.participantes
            );
            break;

        case MINIJUEGO_PASO_SILENCIOSO:
            minijuegoPasoSilencioso.Actualizar(
                deltaTime,
                contexto.participantes
            );
            break;

        case MINIJUEGO_CIRCUITO_VOLTAJE:
            minijuegoCircuitoVoltaje.Actualizar(
                deltaTime,
                contexto.participantes
            );
            break;

        case MINIJUEGO_TRAZO_PERFECTO:
            minijuegoTrazoPerfecto.Actualizar(
                deltaTime,
                contexto.participantes
            );
            break;

        case MINIJUEGO_CARGA_INESTABLE:
            minijuegoCargaInestable.Actualizar(
                deltaTime,
                contexto.participantes
            );
            break;

        case MINIJUEGO_SECUENCIA_NEON:
            minijuegoSecuenciaNeon.Actualizar(
                deltaTime,
                contexto.participantes
            );
            break;

        case MINIJUEGO_INTERRUPTORES_CAOS:
            minijuegoInterruptoresCaos.Actualizar(
                deltaTime,
                contexto.participantes
            );
            break;

        case MINIJUEGO_TANQUES_PLASMA:
            minijuegoTanquesPlasma.Actualizar(
                deltaTime,
                contexto.participantes
            );
            break;

        case MINIJUEGO_PASARELAS_VACIO:
            minijuegoPasarelasVacio.Actualizar(
                deltaTime,
                contexto.jugadores,
                contexto.cantidadJugadores,
                contexto.participantes,
                contexto.particulas,
                contexto.cantidadParticulas
            );
            break;

        case MINIJUEGO_CANTERA_FUGA:
            minijuegoCanteraFuga.Actualizar(
                deltaTime,
                contexto.jugadores,
                contexto.cantidadJugadores,
                contexto.participantes
            );
            break;

        case MINIJUEGO_TERRITORIO_CONQUISTA:
            minijuegoTerritorioConquista.Actualizar(
                deltaTime,
                contexto.jugadores,
                contexto.cantidadJugadores,
                contexto.participantes
            );
            break;

        case MINIJUEGO_DEFENSA_NUCLEO:
            minijuegoDefensaNucleo.Actualizar(
                deltaTime,
                contexto.jugadores,
                contexto.cantidadJugadores,
                contexto.participantes
            );
            break;

        case MINIJUEGO_LLUVIA_APILADA:
            minijuegoLluviaApilada.Actualizar(
                deltaTime,
                contexto.jugadores,
                contexto.cantidadJugadores,
                contexto.participantes
            );
            break;

        case MINIJUEGO_CUERDA_ACANTILADO:
            minijuegoCuerdaAcantilado.Actualizar(
                deltaTime,
                contexto.jugadores,
                contexto.cantidadJugadores,
                contexto.participantes
            );
            break;

        case MINIJUEGO_ULTIMO_ASIENTO:
            minijuegoUltimoAsiento.Actualizar(
                deltaTime,
                contexto.jugadores,
                contexto.cantidadJugadores,
                contexto.participantes
            );
            break;

        case MINIJUEGO_CAJAS_PUERTO:
            minijuegoCajasPuerto.Actualizar(
                deltaTime,
                contexto.jugadores,
                contexto.cantidadJugadores,
                contexto.participantes
            );
            break;

        case MINIJUEGO_LABERINTO_INCLINADO:
            minijuegoLaberintoInclinado.Actualizar(
                deltaTime,
                contexto.jugadores,
                contexto.cantidadJugadores,
                contexto.participantes
            );
            break;

        case MINIJUEGO_VETA_CRISTAL:
            minijuegoVetaCristal.Actualizar(
                deltaTime,
                contexto.jugadores,
                contexto.cantidadJugadores,
                contexto.participantes
            );
            break;

        case MINIJUEGO_CAPSULAS_BARAJADAS:
            minijuegoCapsulasBarajadas.Actualizar(
                deltaTime,
                contexto.jugadores,
                contexto.cantidadJugadores,
                contexto.participantes
            );
            break;

        case MINIJUEGO_BATEO_METEORICO:
            minijuegoBateoMeteorico.Actualizar(
                deltaTime,
                contexto.jugadores,
                contexto.cantidadJugadores,
                contexto.participantes
            );
            break;

        case MINIJUEGO_RACIMO_TOXICO:
            minijuegoRacimoToxico.Actualizar(
                deltaTime,
                contexto.jugadores,
                contexto.cantidadJugadores,
                contexto.participantes
            );
            break;

        case MINIJUEGO_TESORERO_ACORRALADO:
            minijuegoTesoreroAcorralado.Actualizar(
                deltaTime,
                contexto.jugadores,
                contexto.cantidadJugadores,
                contexto.participantes
            );
            break;

        case MINIJUEGO_DESCENSO_NUBES:
            minijuegoDescensoNubes.Actualizar(
                deltaTime,
                contexto.jugadores,
                contexto.cantidadJugadores,
                contexto.participantes
            );
            break;

        case MINIJUEGO_VOLEA_MAGMA:
            minijuegoVoleaMagma.Actualizar(
                deltaTime,
                contexto.jugadores,
                contexto.cantidadJugadores,
                contexto.participantes
            );
            break;

        case MINIJUEGO_PAREJAS_GLACIAR:
            minijuegoParejasGlaciar.Actualizar(
                deltaTime,
                contexto.jugadores,
                contexto.cantidadJugadores,
                contexto.participantes
            );
            break;

        case MINIJUEGO_ESFERAS_CANON:
            minijuegoEsferasCanon.Actualizar(
                deltaTime,
                contexto.jugadores,
                contexto.cantidadJugadores,
                contexto.participantes
            );
            break;

        case MINIJUEGO_PESCA_ISLA:
            minijuegoPescaIsla.Actualizar(
                deltaTime,
                contexto.jugadores,
                contexto.cantidadJugadores,
                contexto.participantes
            );
            break;

        case MINIJUEGO_RODILLOS_NEON:
            minijuegoRodillosNeon.Actualizar(
                deltaTime,
                contexto.jugadores,
                contexto.cantidadJugadores,
                contexto.participantes
            );
            break;

        case MINIJUEGO_BOLAS_AZUCAR:
            minijuegoBolasAzucar.Actualizar(
                deltaTime,
                contexto.jugadores,
                contexto.cantidadJugadores,
                contexto.participantes
            );
            break;

        case MINIJUEGO_GRUA_CHATARRA:
            minijuegoGruaChatarra.Actualizar(
                deltaTime,
                contexto.jugadores,
                contexto.cantidadJugadores,
                contexto.participantes
            );
            break;

        case MINIJUEGO_PISOTON_PLAGAS:
            minijuegoPisotonPlagas.Actualizar(
                deltaTime,
                contexto.jugadores,
                contexto.cantidadJugadores,
                contexto.participantes
            );
            break;

        case MINIJUEGO_BALSAS_RAPIDO:
            minijuegoBalsasRapido.Actualizar(
                deltaTime,
                contexto.jugadores,
                contexto.cantidadJugadores,
                contexto.participantes
            );
            break;

        case MINIJUEGO_AUTOS_GLOBO:
            minijuegoAutosGlobo.Actualizar(
                deltaTime,
                contexto.jugadores,
                contexto.cantidadJugadores,
                contexto.participantes
            );
            break;

        case MINIJUEGO_SENDERO_INVISIBLE:
            minijuegoSenderoInvisible.Actualizar(
                deltaTime,
                contexto.jugadores,
                contexto.cantidadJugadores,
                contexto.participantes
            );
            break;

        case MINIJUEGO_BANQUETE_TURBO:
            minijuegoBanqueteTurbo.Actualizar(
                deltaTime,
                contexto.jugadores,
                contexto.cantidadJugadores,
                contexto.participantes
            );
            break;

        case MINIJUEGO_TUBERIAS_DESIERTO:
            minijuegoTuberiasDesierto.Actualizar(
                deltaTime,
                contexto.jugadores,
                contexto.cantidadJugadores,
                contexto.participantes
            );
            break;

        case MINIJUEGO_TREPA_MASTIL:
            minijuegoTrepaMastil.Actualizar(
                deltaTime,
                contexto.jugadores,
                contexto.cantidadJugadores,
                contexto.participantes
            );
            break;

        case MINIJUEGO_GUARDIAN_RUINAS:
            minijuegoGuardianRuinas.Actualizar(
                deltaTime,
                contexto.jugadores,
                contexto.cantidadJugadores,
                contexto.participantes
            );
            break;

        case CANTIDAD_MINIJUEGOS:
            break;
    }
}


void GestorMinijuegos::DibujarActivo(
    const ContextoMinijuego& contexto,
    bool mostrarDebug
) const
{
    switch (minijuegoActivo)
    {
        case MINIJUEGO_COLOR_SEGURO:
            minijuegoColor.Dibujar(
                contexto.jugadores,
                contexto.cantidadJugadores,
                contexto.participantes,
                contexto.particulas,
                contexto.cantidadParticulas,
                mostrarDebug
            );
            break;

        case MINIJUEGO_PELOTAS:
            minijuegoPelotas.Dibujar(
                contexto.jugadores,
                contexto.cantidadJugadores,
                contexto.participantes,
                mostrarDebug
            );
            break;

        case MINIJUEGO_TRONCO:
            minijuegoTronco.Dibujar(
                contexto.jugadores,
                contexto.cantidadJugadores,
                contexto.participantes
            );
            break;

        case MINIJUEGO_FABRICA_67:
            minijuego67.Dibujar(
                contexto.cantidadJugadores,
                contexto.participantes
            );
            break;

        case MINIJUEGO_ISLA_FUEGO:
            minijuegoIslaFuego.Dibujar(
                contexto.jugadores,
                contexto.cantidadJugadores,
                contexto.participantes,
                contexto.particulas,
                contexto.cantidadParticulas,
                mostrarDebug
            );
            break;

        case MINIJUEGO_CAPITAN_MANDA:
            minijuegoCapitanManda.Dibujar(
                contexto.participantes
            );
            break;

        case MINIJUEGO_BARRA_GIRATORIA:
            minijuegoBarraGiratoria.Dibujar(
                contexto.jugadores,
                contexto.cantidadJugadores,
                contexto.participantes,
                contexto.particulas,
                contexto.cantidadParticulas,
                mostrarDebug
            );
            break;

        case MINIJUEGO_NUCLEOS_ENERGIA:
            minijuegoNucleosEnergia.Dibujar(
                contexto.jugadores,
                contexto.cantidadJugadores,
                contexto.participantes,
                mostrarDebug
            );
            break;

        case MINIJUEGO_REFUGIO_PINCHOS:
            minijuegoRefugioPinchos.Dibujar(
                contexto.jugadores,
                contexto.cantidadJugadores,
                contexto.participantes,
                mostrarDebug
            );
            break;

        case MINIJUEGO_MIRADAS_CRUZADAS:
            minijuegoMiradasCruzadas.Dibujar(
                contexto.participantes
            );
            break;

        case MINIJUEGO_MUROS_LOCOS:
            minijuegoMurosLocos.Dibujar(
                contexto.jugadores,
                contexto.cantidadJugadores,
                contexto.participantes,
                contexto.particulas,
                contexto.cantidadParticulas,
                mostrarDebug
            );
            break;

        case MINIJUEGO_TORMENTA_MAGNETICA:
            minijuegoTormentaMagnetica.Dibujar(
                contexto.jugadores,
                contexto.cantidadJugadores,
                contexto.participantes,
                contexto.particulas,
                contexto.cantidadParticulas,
                mostrarDebug
            );
            break;

        case MINIJUEGO_CONTEO_EXPLOSIVO:
            minijuegoConteoExplosivo.Dibujar(
                contexto.participantes
            );
            break;

        case MINIJUEGO_PASO_SILENCIOSO:
            minijuegoPasoSilencioso.Dibujar(
                contexto.participantes
            );
            break;

        case MINIJUEGO_CIRCUITO_VOLTAJE:
            minijuegoCircuitoVoltaje.Dibujar(
                contexto.participantes
            );
            break;

        case MINIJUEGO_TRAZO_PERFECTO:
            minijuegoTrazoPerfecto.Dibujar(
                contexto.participantes
            );
            break;

        case MINIJUEGO_CARGA_INESTABLE:
            minijuegoCargaInestable.Dibujar(
                contexto.participantes
            );
            break;

        case MINIJUEGO_SECUENCIA_NEON:
            minijuegoSecuenciaNeon.Dibujar(
                contexto.participantes
            );
            break;

        case MINIJUEGO_INTERRUPTORES_CAOS:
            minijuegoInterruptoresCaos.Dibujar(
                contexto.participantes
            );
            break;

        case MINIJUEGO_TANQUES_PLASMA:
            minijuegoTanquesPlasma.Dibujar(
                contexto.participantes
            );
            break;

        case MINIJUEGO_PASARELAS_VACIO:
            minijuegoPasarelasVacio.Dibujar(
                contexto.jugadores,
                contexto.cantidadJugadores,
                contexto.participantes,
                contexto.particulas,
                contexto.cantidadParticulas,
                mostrarDebug
            );
            break;

        case MINIJUEGO_CANTERA_FUGA:
            minijuegoCanteraFuga.Dibujar(
                contexto.jugadores,
                contexto.cantidadJugadores,
                contexto.participantes,
                mostrarDebug
            );
            break;

        case MINIJUEGO_TERRITORIO_CONQUISTA:
            minijuegoTerritorioConquista.Dibujar(
                contexto.jugadores,
                contexto.cantidadJugadores,
                contexto.participantes,
                mostrarDebug
            );
            break;

        case MINIJUEGO_DEFENSA_NUCLEO:
            minijuegoDefensaNucleo.Dibujar(
                contexto.jugadores,
                contexto.cantidadJugadores,
                contexto.participantes,
                mostrarDebug
            );
            break;

        case MINIJUEGO_LLUVIA_APILADA:
            minijuegoLluviaApilada.Dibujar(
                contexto.jugadores,
                contexto.cantidadJugadores,
                contexto.participantes,
                mostrarDebug
            );
            break;

        case MINIJUEGO_CUERDA_ACANTILADO:
            minijuegoCuerdaAcantilado.Dibujar(
                contexto.jugadores,
                contexto.cantidadJugadores,
                contexto.participantes,
                mostrarDebug
            );
            break;

        case MINIJUEGO_ULTIMO_ASIENTO:
            minijuegoUltimoAsiento.Dibujar(
                contexto.jugadores,
                contexto.cantidadJugadores,
                contexto.participantes,
                mostrarDebug
            );
            break;

        case MINIJUEGO_CAJAS_PUERTO:
            minijuegoCajasPuerto.Dibujar(
                contexto.jugadores,
                contexto.cantidadJugadores,
                contexto.participantes,
                mostrarDebug
            );
            break;

        case MINIJUEGO_LABERINTO_INCLINADO:
            minijuegoLaberintoInclinado.Dibujar(
                contexto.jugadores,
                contexto.cantidadJugadores,
                contexto.participantes,
                mostrarDebug
            );
            break;

        case MINIJUEGO_VETA_CRISTAL:
            minijuegoVetaCristal.Dibujar(
                contexto.jugadores,
                contexto.cantidadJugadores,
                contexto.participantes,
                mostrarDebug
            );
            break;

        case MINIJUEGO_CAPSULAS_BARAJADAS:
            minijuegoCapsulasBarajadas.Dibujar(
                contexto.jugadores,
                contexto.cantidadJugadores,
                contexto.participantes,
                mostrarDebug
            );
            break;

        case MINIJUEGO_BATEO_METEORICO:
            minijuegoBateoMeteorico.Dibujar(
                contexto.jugadores,
                contexto.cantidadJugadores,
                contexto.participantes,
                mostrarDebug
            );
            break;

        case MINIJUEGO_RACIMO_TOXICO:
            minijuegoRacimoToxico.Dibujar(
                contexto.jugadores,
                contexto.cantidadJugadores,
                contexto.participantes,
                mostrarDebug
            );
            break;

        case MINIJUEGO_TESORERO_ACORRALADO:
            minijuegoTesoreroAcorralado.Dibujar(
                contexto.jugadores,
                contexto.cantidadJugadores,
                contexto.participantes,
                mostrarDebug
            );
            break;

        case MINIJUEGO_DESCENSO_NUBES:
            minijuegoDescensoNubes.Dibujar(
                contexto.jugadores,
                contexto.cantidadJugadores,
                contexto.participantes,
                mostrarDebug
            );
            break;

        case MINIJUEGO_VOLEA_MAGMA:
            minijuegoVoleaMagma.Dibujar(
                contexto.jugadores,
                contexto.cantidadJugadores,
                contexto.participantes,
                mostrarDebug
            );
            break;

        case MINIJUEGO_PAREJAS_GLACIAR:
            minijuegoParejasGlaciar.Dibujar(
                contexto.jugadores,
                contexto.cantidadJugadores,
                contexto.participantes,
                mostrarDebug
            );
            break;

        case MINIJUEGO_ESFERAS_CANON:
            minijuegoEsferasCanon.Dibujar(
                contexto.jugadores,
                contexto.cantidadJugadores,
                contexto.participantes,
                mostrarDebug
            );
            break;

        case MINIJUEGO_PESCA_ISLA:
            minijuegoPescaIsla.Dibujar(
                contexto.jugadores,
                contexto.cantidadJugadores,
                contexto.participantes,
                mostrarDebug
            );
            break;

        case MINIJUEGO_RODILLOS_NEON:
            minijuegoRodillosNeon.Dibujar(
                contexto.jugadores,
                contexto.cantidadJugadores,
                contexto.participantes,
                mostrarDebug
            );
            break;

        case MINIJUEGO_BOLAS_AZUCAR:
            minijuegoBolasAzucar.Dibujar(
                contexto.jugadores,
                contexto.cantidadJugadores,
                contexto.participantes,
                mostrarDebug
            );
            break;

        case MINIJUEGO_GRUA_CHATARRA:
            minijuegoGruaChatarra.Dibujar(
                contexto.jugadores,
                contexto.cantidadJugadores,
                contexto.participantes,
                mostrarDebug
            );
            break;

        case MINIJUEGO_PISOTON_PLAGAS:
            minijuegoPisotonPlagas.Dibujar(
                contexto.jugadores,
                contexto.cantidadJugadores,
                contexto.participantes,
                mostrarDebug
            );
            break;

        case MINIJUEGO_BALSAS_RAPIDO:
            minijuegoBalsasRapido.Dibujar(
                contexto.jugadores,
                contexto.cantidadJugadores,
                contexto.participantes,
                mostrarDebug
            );
            break;

        case MINIJUEGO_AUTOS_GLOBO:
            minijuegoAutosGlobo.Dibujar(
                contexto.jugadores,
                contexto.cantidadJugadores,
                contexto.participantes,
                mostrarDebug
            );
            break;

        case MINIJUEGO_SENDERO_INVISIBLE:
            minijuegoSenderoInvisible.Dibujar(
                contexto.jugadores,
                contexto.cantidadJugadores,
                contexto.participantes,
                mostrarDebug
            );
            break;

        case MINIJUEGO_BANQUETE_TURBO:
            minijuegoBanqueteTurbo.Dibujar(
                contexto.jugadores,
                contexto.cantidadJugadores,
                contexto.participantes,
                mostrarDebug
            );
            break;

        case MINIJUEGO_TUBERIAS_DESIERTO:
            minijuegoTuberiasDesierto.Dibujar(
                contexto.jugadores,
                contexto.cantidadJugadores,
                contexto.participantes,
                mostrarDebug
            );
            break;

        case MINIJUEGO_TREPA_MASTIL:
            minijuegoTrepaMastil.Dibujar(
                contexto.jugadores,
                contexto.cantidadJugadores,
                contexto.participantes,
                mostrarDebug
            );
            break;

        case MINIJUEGO_GUARDIAN_RUINAS:
            minijuegoGuardianRuinas.Dibujar(
                contexto.jugadores,
                contexto.cantidadJugadores,
                contexto.participantes,
                mostrarDebug
            );
            break;

        case CANTIDAD_MINIJUEGOS:
            break;
    }
}


const ResultadoMinijuego*
GestorMinijuegos::ObtenerResultadoActivo() const
{
    switch (minijuegoActivo)
    {
        case MINIJUEGO_COLOR_SEGURO:
            return &minijuegoColor.ObtenerResultado();
        case MINIJUEGO_PELOTAS:
            return &minijuegoPelotas.ObtenerResultado();
        case MINIJUEGO_TRONCO:
            return &minijuegoTronco.ObtenerResultado();
        case MINIJUEGO_FABRICA_67:
            return &minijuego67.ObtenerResultado();
        case MINIJUEGO_ISLA_FUEGO:
            return &minijuegoIslaFuego.ObtenerResultado();
        case MINIJUEGO_CAPITAN_MANDA:
            return &minijuegoCapitanManda.ObtenerResultado();
        case MINIJUEGO_BARRA_GIRATORIA:
            return &minijuegoBarraGiratoria.ObtenerResultado();
        case MINIJUEGO_NUCLEOS_ENERGIA:
            return &minijuegoNucleosEnergia.ObtenerResultado();
        case MINIJUEGO_REFUGIO_PINCHOS:
            return &minijuegoRefugioPinchos.ObtenerResultado();
        case MINIJUEGO_MIRADAS_CRUZADAS:
            return &minijuegoMiradasCruzadas.ObtenerResultado();
        case MINIJUEGO_MUROS_LOCOS:
            return &minijuegoMurosLocos.ObtenerResultado();
        case MINIJUEGO_TORMENTA_MAGNETICA:
            return &minijuegoTormentaMagnetica.ObtenerResultado();
        case MINIJUEGO_CONTEO_EXPLOSIVO:
            return &minijuegoConteoExplosivo.ObtenerResultado();
        case MINIJUEGO_PASO_SILENCIOSO:
            return &minijuegoPasoSilencioso.ObtenerResultado();
        case MINIJUEGO_CIRCUITO_VOLTAJE:
            return &minijuegoCircuitoVoltaje.ObtenerResultado();
        case MINIJUEGO_TRAZO_PERFECTO:
            return &minijuegoTrazoPerfecto.ObtenerResultado();
        case MINIJUEGO_CARGA_INESTABLE:
            return &minijuegoCargaInestable.ObtenerResultado();
        case MINIJUEGO_SECUENCIA_NEON:
            return &minijuegoSecuenciaNeon.ObtenerResultado();
        case MINIJUEGO_INTERRUPTORES_CAOS:
            return &minijuegoInterruptoresCaos.ObtenerResultado();
        case MINIJUEGO_TANQUES_PLASMA:
            return &minijuegoTanquesPlasma.ObtenerResultado();
        case MINIJUEGO_PASARELAS_VACIO:
            return &minijuegoPasarelasVacio.ObtenerResultado();
        case MINIJUEGO_CANTERA_FUGA:
            return &minijuegoCanteraFuga.ObtenerResultado();
        case MINIJUEGO_TERRITORIO_CONQUISTA:
            return &minijuegoTerritorioConquista.ObtenerResultado();
        case MINIJUEGO_DEFENSA_NUCLEO:
            return &minijuegoDefensaNucleo.ObtenerResultado();
        case MINIJUEGO_LLUVIA_APILADA:
            return &minijuegoLluviaApilada.ObtenerResultado();
        case MINIJUEGO_CUERDA_ACANTILADO:
            return &minijuegoCuerdaAcantilado.ObtenerResultado();
        case MINIJUEGO_ULTIMO_ASIENTO:
            return &minijuegoUltimoAsiento.ObtenerResultado();
        case MINIJUEGO_CAJAS_PUERTO:
            return &minijuegoCajasPuerto.ObtenerResultado();
        case MINIJUEGO_LABERINTO_INCLINADO:
            return &minijuegoLaberintoInclinado.ObtenerResultado();
        case MINIJUEGO_VETA_CRISTAL:
            return &minijuegoVetaCristal.ObtenerResultado();
        case MINIJUEGO_CAPSULAS_BARAJADAS:
            return &minijuegoCapsulasBarajadas.ObtenerResultado();
        case MINIJUEGO_BATEO_METEORICO:
            return &minijuegoBateoMeteorico.ObtenerResultado();
        case MINIJUEGO_RACIMO_TOXICO:
            return &minijuegoRacimoToxico.ObtenerResultado();
        case MINIJUEGO_TESORERO_ACORRALADO:
            return &minijuegoTesoreroAcorralado.ObtenerResultado();
        case MINIJUEGO_DESCENSO_NUBES:
            return &minijuegoDescensoNubes.ObtenerResultado();
        case MINIJUEGO_VOLEA_MAGMA:
            return &minijuegoVoleaMagma.ObtenerResultado();
        case MINIJUEGO_PAREJAS_GLACIAR:
            return &minijuegoParejasGlaciar.ObtenerResultado();
        case MINIJUEGO_ESFERAS_CANON:
            return &minijuegoEsferasCanon.ObtenerResultado();
        case MINIJUEGO_PESCA_ISLA:
            return &minijuegoPescaIsla.ObtenerResultado();
        case MINIJUEGO_RODILLOS_NEON:
            return &minijuegoRodillosNeon.ObtenerResultado();
        case MINIJUEGO_BOLAS_AZUCAR:
            return &minijuegoBolasAzucar.ObtenerResultado();
        case MINIJUEGO_GRUA_CHATARRA:
            return &minijuegoGruaChatarra.ObtenerResultado();
        case MINIJUEGO_PISOTON_PLAGAS:
            return &minijuegoPisotonPlagas.ObtenerResultado();
        case MINIJUEGO_BALSAS_RAPIDO:
            return &minijuegoBalsasRapido.ObtenerResultado();
        case MINIJUEGO_AUTOS_GLOBO:
            return &minijuegoAutosGlobo.ObtenerResultado();
        case MINIJUEGO_SENDERO_INVISIBLE:
            return &minijuegoSenderoInvisible.ObtenerResultado();
        case MINIJUEGO_BANQUETE_TURBO:
            return &minijuegoBanqueteTurbo.ObtenerResultado();
        case MINIJUEGO_TUBERIAS_DESIERTO:
            return &minijuegoTuberiasDesierto.ObtenerResultado();
        case MINIJUEGO_TREPA_MASTIL:
            return &minijuegoTrepaMastil.ObtenerResultado();
        case MINIJUEGO_GUARDIAN_RUINAS:
            return &minijuegoGuardianRuinas.ObtenerResultado();
        case CANTIDAD_MINIJUEGOS:
            break;
    }

    return nullptr;
}


IdMinijuego GestorMinijuegos::ObtenerIdActivo() const
{
    return minijuegoActivo;
}


void GestorMinijuegos::PrepararTemaVisualActivo() const
{
    if (minijuegoActivo == MINIJUEGO_COLOR_SEGURO)
    {
        SeleccionarTemaVisualMinijuego(TEMA_VISUAL_LAVA);
        return;
    }

    if (minijuegoActivo == MINIJUEGO_PELOTAS)
    {
        SeleccionarTemaVisualMinijuego(TEMA_VISUAL_NIEVE);
        return;
    }

    if (minijuegoActivo == MINIJUEGO_TORMENTA_MAGNETICA)
    {
        SeleccionarTemaVisualMinijuego(TEMA_VISUAL_MAGNETICO);
        return;
    }

    if (minijuegoActivo == MINIJUEGO_REFUGIO_PINCHOS)
    {
        SeleccionarTemaVisualMinijuego(TEMA_VISUAL_CUEVA);
        minijuegoRefugioPinchos.ConfigurarTaladrosVisuales();
    }
}


bool GestorMinijuegos::MantieneControlIAEnReconexion() const
{
    return
        minijuegoActivo == MINIJUEGO_PASARELAS_VACIO ||
        minijuegoActivo == MINIJUEGO_CANTERA_FUGA ||
        minijuegoActivo == MINIJUEGO_TERRITORIO_CONQUISTA ||
        minijuegoActivo == MINIJUEGO_DEFENSA_NUCLEO ||
        minijuegoActivo == MINIJUEGO_LLUVIA_APILADA ||
        minijuegoActivo == MINIJUEGO_CUERDA_ACANTILADO ||
        minijuegoActivo == MINIJUEGO_ULTIMO_ASIENTO ||
        minijuegoActivo == MINIJUEGO_CAJAS_PUERTO ||
        minijuegoActivo == MINIJUEGO_LABERINTO_INCLINADO ||
        minijuegoActivo == MINIJUEGO_VETA_CRISTAL ||
        minijuegoActivo == MINIJUEGO_CAPSULAS_BARAJADAS ||
        minijuegoActivo == MINIJUEGO_BATEO_METEORICO ||
        minijuegoActivo == MINIJUEGO_RACIMO_TOXICO ||
        minijuegoActivo == MINIJUEGO_TESORERO_ACORRALADO ||
        minijuegoActivo == MINIJUEGO_DESCENSO_NUBES ||
        minijuegoActivo == MINIJUEGO_VOLEA_MAGMA ||
        minijuegoActivo == MINIJUEGO_PAREJAS_GLACIAR ||
        minijuegoActivo == MINIJUEGO_ESFERAS_CANON ||
        minijuegoActivo == MINIJUEGO_PESCA_ISLA ||
        minijuegoActivo == MINIJUEGO_RODILLOS_NEON ||
        minijuegoActivo == MINIJUEGO_BOLAS_AZUCAR ||
        minijuegoActivo == MINIJUEGO_GRUA_CHATARRA ||
        minijuegoActivo == MINIJUEGO_PISOTON_PLAGAS ||
        minijuegoActivo == MINIJUEGO_BALSAS_RAPIDO ||
        minijuegoActivo == MINIJUEGO_AUTOS_GLOBO ||
        minijuegoActivo == MINIJUEGO_SENDERO_INVISIBLE ||
        minijuegoActivo == MINIJUEGO_BANQUETE_TURBO ||
        minijuegoActivo == MINIJUEGO_TUBERIAS_DESIERTO ||
        minijuegoActivo == MINIJUEGO_TREPA_MASTIL ||
        minijuegoActivo == MINIJUEGO_GUARDIAN_RUINAS;
}


void GestorMinijuegos::ReiniciarJugadorCompartido(
    ContextoMinijuego& contexto,
    int indice
) const
{
    if (
        indice < 0 ||
        indice >= contexto.cantidadJugadores
    )
    {
        return;
    }

    ReiniciarJugadorPrueba(
        contexto.jugadores[indice]
    );
}


void GestorMinijuegos::ReiniciarJugadoresCompartidos(
    ContextoMinijuego& contexto
) const
{
    for (int i = 0; i < contexto.cantidadJugadores; i++)
    {
        ReiniciarJugadorCompartido(
            contexto,
            i
        );
    }
}


void GestorMinijuegos::Descargar()
{
    minijuego67.Descargar();
}
