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
        minijuegoActivo == MINIJUEGO_CANTERA_FUGA;
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
