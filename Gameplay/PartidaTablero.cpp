#include "Gameplay/PartidaTablero.h"

#include "Minigames/EfectosVisualesMinijuegos.h"
#include "Minigames/ModeloJugadorCompartido.h"
#include "Systems/Input.h"
#include "UI/EstiloUI.h"
#include "UI/EstiloUIGlobal.h"

#include "rlgl.h"

#include <cmath>
#include <cstdio>


static const float VELOCIDAD_MOVIMIENTO_TABLERO_FINAL = 2.35f;
static const float ALTURA_SALTO_FICHA_TABLERO_FINAL = 0.70f;
static const float PI_TABLERO_FINAL = 3.14159265f;
static const int MONEDAS_INICIALES_TABLERO = 10;
static const int PREMIO_GANADOR_MINIJUEGO = 10;
static const int PREMIO_PARTICIPACION_MINIJUEGO = 3;


static Vector3 InterpolarPosicionTableroFinal(
    Vector3 origen,
    Vector3 destino,
    float progreso
)
{
    return Vector3
    {
        origen.x + (destino.x - origen.x) * progreso,
        origen.y + (destino.y - origen.y) * progreso,
        origen.z + (destino.z - origen.z) * progreso
    };
}


static Vector3 ObtenerDesplazamientoFichaTableroFinal(
    int indiceParticipante
)
{
    const Vector3 desplazamientos[MAX_PARTICIPANTES] =
    {
        { -0.34f, 0.0f, -0.34f },
        {  0.34f, 0.0f, -0.34f },
        { -0.34f, 0.0f,  0.34f },
        {  0.34f, 0.0f,  0.34f }
    };

    if (
        indiceParticipante < 0 ||
        indiceParticipante >= MAX_PARTICIPANTES
    )
    {
        return Vector3{};
    }

    return desplazamientos[indiceParticipante];
}


static Vector3 ObtenerPosicionFichaTableroFinal(
    const PartidaTablero& partida,
    int indiceCasilla,
    int indiceParticipante
)
{
    const Casilla* casilla =
        partida.tablero.ObtenerCasilla(indiceCasilla);

    if (casilla == nullptr)
    {
        return Vector3{};
    }

    Vector3 posicion = casilla->posicion;
    Vector3 desplazamiento =
        ObtenerDesplazamientoFichaTableroFinal(indiceParticipante);

    posicion.x += desplazamiento.x;
    posicion.y += 0.78f;
    posicion.z += desplazamiento.z;

    return posicion;
}


int ObtenerParticipanteTurnoTableroFinal(
    const PartidaTablero& partida
)
{
    if (
        partida.cantidadJugadores <= 0 ||
        partida.indiceOrdenTurno < 0 ||
        partida.indiceOrdenTurno >= partida.cantidadJugadores
    )
    {
        return -1;
    }

    return partida.ordenParticipantes[partida.indiceOrdenTurno];
}


static bool ParticipantePuedeDecidirTablero(
    const PartidaTablero& partida,
    int indiceParticipante
)
{
    if (
        partida.participantes == nullptr ||
        indiceParticipante < 0 ||
        indiceParticipante >= MAX_PARTICIPANTES
    )
    {
        return false;
    }

    const Participante& participante =
        partida.participantes[indiceParticipante];

    return
        participante.activo &&
        !participante.esBot &&
        participante.conectado;
}


void ReproducirSonidoTablero(
    PartidaTablero& partida,
    TipoSonidoJuego tipo
)
{
    if (partida.audio != nullptr)
    {
        partida.audio->ReproducirSonido(tipo);
    }
}


static void MezclarOrdenParticipantes(
    int orden[],
    int cantidad
)
{
    for (int i = cantidad - 1; i > 0; i--)
    {
        int otro = GetRandomValue(0, i);
        int temporal = orden[i];
        orden[i] = orden[otro];
        orden[otro] = temporal;
    }
}


static void ElegirNuevaCasillaTrofeo(
    PartidaTablero& partida
)
{
    // Las candidatas dependen del tablero (DefinicionTablero).
    if (
        partida.definicion == nullptr ||
        partida.definicion->casillasTrofeo == nullptr ||
        partida.definicion->cantidadCasillasTrofeo <= 0
    )
    {
        return;
    }

    const int* candidatas =
        partida.definicion->casillasTrofeo;

    const int cantidadCandidatas =
        partida.definicion->cantidadCasillasTrofeo;

    int anterior = partida.casillaTrofeo;
    int elegida = anterior;

    for (int intento = 0; intento < 12; intento++)
    {
        int candidata =
            candidatas[GetRandomValue(0, cantidadCandidatas - 1)];

        if (
            candidata >= 0 &&
            candidata < partida.tablero.cantidadCasillas &&
            candidata != anterior
        )
        {
            elegida = candidata;
            break;
        }
    }

    if (elegida == anterior)
    {
        for (int i = 0; i < cantidadCandidatas; i++)
        {
            int candidata = candidatas[i];

            if (
                candidata >= 0 &&
                candidata < partida.tablero.cantidadCasillas &&
                candidata != anterior
            )
            {
                elegida = candidata;
                break;
            }
        }
    }

    partida.casillaTrofeo = elegida;
}


static void PrepararInicioTurno(
    PartidaTablero& partida
)
{
    partida.valorDado = 0;
    partida.pasosPendientes = 0;
    partida.opcionRuta = 0;
    partida.direccionRutaBloqueada = false;
    partida.casillaDestinoMovimiento = -1;
    partida.progresoMovimiento = 0.0f;
    partida.compraTrofeoDisponible = false;
    partida.compraTrofeoResuelta = false;
    partida.ultimoEventoEspecial = EVENTO_TABLERO_NINGUNO;
    partida.variacionMonedasEvento = 0;
    partida.jugadorIntercambioEvento = -1;
    partida.textoEvento[0] = '\0';
    partida.evento = EventoInteractivo{};
    partida.movimientoDeEvento = false;
    partida.duelo = DueloTablero{};
    partida.menuObjetos = MenuObjetosTablero{};
    partida.botRevisoObjetos = false;
    partida.detalleDado[0] = '\0';
    partida.fase = FASE_PARTIDA_TABLERO_ESPERANDO_DADO;
    partida.tiempoFase = 0.0f;
}


void TeletransportarJugadorTablero(
    PartidaTablero& partida,
    int participante,
    int casilla
)
{
    if (
        participante < 0 ||
        participante >= MAX_PARTICIPANTES ||
        partida.tablero.ObtenerCasilla(casilla) == nullptr
    )
    {
        return;
    }

    EstadoJugadorPartidaTablero& jugador =
        partida.jugadores[participante];

    jugador.casillaActual = casilla;
    jugador.posicionVisual =
        ObtenerPosicionFichaTableroFinal(
            partida,
            casilla,
            participante
        );
}


void EstablecerMensajeGimmick(
    PartidaTablero& partida,
    const char* texto
)
{
    std::snprintf(
        partida.mensajeGimmick,
        sizeof(partida.mensajeGimmick),
        "%s",
        texto
    );
    partida.tiempoMensajeGimmick = 3.5f;
}


// Avisa al tablero de que empieza una ronda (reglas del gimmick).
static void IniciarRondaTablero(
    PartidaTablero& partida
)
{
    if (
        partida.definicion != nullptr &&
        partida.definicion->AlIniciarRonda != nullptr
    )
    {
        partida.definicion->AlIniciarRonda(partida);
    }
}


static void AvanzarTurnoTableroFinal(
    PartidaTablero& partida
)
{
    partida.indiceOrdenTurno++;

    if (partida.indiceOrdenTurno >= partida.cantidadJugadores)
    {
        partida.indiceOrdenTurno = 0;
        partida.fase = FASE_PARTIDA_TABLERO_FIN_RONDA;
        partida.tiempoFase = 0.0f;
        partida.minijuegoSolicitado = false;
        return;
    }

    PrepararInicioTurno(partida);
}


static void IniciarMovimientoHaciaTableroFinal(
    PartidaTablero& partida,
    int indiceDestino
)
{
    int indiceParticipante =
        ObtenerParticipanteTurnoTableroFinal(partida);

    const Casilla* destino =
        partida.tablero.ObtenerCasilla(indiceDestino);

    if (
        indiceParticipante < 0 ||
        destino == nullptr
    )
    {
        partida.fase = FASE_PARTIDA_TABLERO_FIN_TURNO;
        partida.tiempoFase = 0.0f;
        return;
    }

    EstadoJugadorPartidaTablero& jugador =
        partida.jugadores[indiceParticipante];

    partida.casillaDestinoMovimiento = indiceDestino;
    partida.posicionInicioMovimiento = jugador.posicionVisual;
    partida.posicionFinMovimiento =
        ObtenerPosicionFichaTableroFinal(
            partida,
            indiceDestino,
            indiceParticipante
        );

    partida.progresoMovimiento = 0.0f;
    partida.fase = FASE_PARTIDA_TABLERO_MOVIENDO;
    ReproducirSonidoTablero(partida, SONIDO_PASO_TABLERO);
    partida.tiempoFase = 0.0f;
}


static void PrepararSiguientePasoTableroFinal(
    PartidaTablero& partida
)
{
    int indiceParticipante =
        ObtenerParticipanteTurnoTableroFinal(partida);

    if (indiceParticipante < 0)
    {
        return;
    }

    const EstadoJugadorPartidaTablero& jugador =
        partida.jugadores[indiceParticipante];

    const Casilla* casilla =
        partida.tablero.ObtenerCasilla(jugador.casillaActual);

    if (
        casilla == nullptr ||
        casilla->cantidadConexiones <= 0
    )
    {
        partida.fase = FASE_PARTIDA_TABLERO_FIN_TURNO;
        partida.tiempoFase = 0.0f;
        return;
    }

    if (casilla->cantidadConexiones > 1)
    {
        partida.opcionRuta = 0;
        partida.direccionRutaBloqueada = false;
        partida.fase = FASE_PARTIDA_TABLERO_ELIGIENDO_RUTA;
        partida.tiempoFase = 0.0f;
        return;
    }

    IniciarMovimientoHaciaTableroFinal(
        partida,
        casilla->conexiones[0].destino
    );
}


static void ContinuarTrasDecisionTrofeo(
    PartidaTablero& partida
)
{
    partida.compraTrofeoDisponible = false;
    partida.compraTrofeoResuelta = true;

    if (partida.pasosPendientes > 0)
    {
        PrepararSiguientePasoTableroFinal(partida);
        return;
    }

    partida.fase = FASE_PARTIDA_TABLERO_EVENTO_CASILLA;
    partida.tiempoFase = 0.0f;
}


static void ComprarTrofeo(
    PartidaTablero& partida,
    int indiceParticipante
)
{
    EstadoJugadorPartidaTablero& jugador =
        partida.jugadores[indiceParticipante];

    if (jugador.monedas < partida.costoTrofeo)
    {
        ContinuarTrasDecisionTrofeo(partida);
        return;
    }

    jugador.monedas -= partida.costoTrofeo;
    jugador.trofeos++;

    ReproducirSonidoTablero(
        partida,
        SONIDO_COMPRA
    );

    ElegirNuevaCasillaTrofeo(partida);
    ContinuarTrasDecisionTrofeo(partida);
}


static void AlLlegarACasillaTableroFinal(
    PartidaTablero& partida
)
{
    int indiceParticipante =
        ObtenerParticipanteTurnoTableroFinal(partida);

    if (indiceParticipante < 0)
    {
        return;
    }

    EstadoJugadorPartidaTablero& jugador =
        partida.jugadores[indiceParticipante];

    jugador.casillaActual = partida.casillaDestinoMovimiento;
    jugador.posicionVisual = partida.posicionFinMovimiento;
    partida.pasosPendientes--;

    if (jugador.casillaActual == partida.casillaTrofeo)
    {
        partida.compraTrofeoDisponible =
            jugador.monedas >= partida.costoTrofeo;

        partida.compraTrofeoResuelta = false;
        partida.fase = FASE_PARTIDA_TABLERO_DECISION_TROFEO;
        partida.tiempoFase = 0.0f;
        return;
    }

    if (partida.pasosPendientes > 0)
    {
        PrepararSiguientePasoTableroFinal(partida);
        return;
    }

    partida.fase = FASE_PARTIDA_TABLERO_EVENTO_CASILLA;
    partida.tiempoFase = 0.0f;
}


static void ActualizarMovimientoTableroFinal(
    PartidaTablero& partida,
    float deltaTime
)
{
    int indiceParticipante =
        ObtenerParticipanteTurnoTableroFinal(partida);

    if (indiceParticipante < 0)
    {
        return;
    }

    EstadoJugadorPartidaTablero& jugador =
        partida.jugadores[indiceParticipante];

    partida.progresoMovimiento +=
        VELOCIDAD_MOVIMIENTO_TABLERO_FINAL * deltaTime;

    float progreso = partida.progresoMovimiento;

    if (progreso > 1.0f)
    {
        progreso = 1.0f;
    }

    float progresoSuave =
        progreso * progreso * (3.0f - 2.0f * progreso);

    jugador.posicionVisual =
        InterpolarPosicionTableroFinal(
            partida.posicionInicioMovimiento,
            partida.posicionFinMovimiento,
            progresoSuave
        );

    jugador.posicionVisual.y +=
        std::sin(progreso * PI_TABLERO_FINAL) *
        ALTURA_SALTO_FICHA_TABLERO_FINAL;

    if (progreso >= 1.0f)
    {
        AlLlegarACasillaTableroFinal(partida);
    }
}


// Texto del evento cuando el escudo bloquea la perdida de la casilla.
static void EscribirTextoEscudo(
    PartidaTablero& partida,
    int indiceParticipante
)
{
    std::snprintf(
        partida.textoEvento,
        sizeof(partida.textoEvento),
        "¡EL ESCUDO DE J%d BLOQUEA LA PÉRDIDA!",
        indiceParticipante + 1
    );
}


// ESPECIAL: bonificacion o multa (el intercambio de monedas ahora es de
// la casilla INTERCAMBIO).
static void ResolverEventoEspecialTablero(
    PartidaTablero& partida,
    int indiceParticipante
)
{
    EstadoJugadorPartidaTablero& jugador =
        partida.jugadores[indiceParticipante];

    if (GetRandomValue(0, 1) == 0)
    {
        partida.ultimoEventoEspecial = EVENTO_TABLERO_BONIFICACION;
        partida.variacionMonedasEvento = MONEDAS_ESPECIAL_BONIFICACION;
        jugador.monedas += MONEDAS_ESPECIAL_BONIFICACION;
    }
    else
    {
        partida.ultimoEventoEspecial = EVENTO_TABLERO_MULTA;

        int perdida = jugador.monedas < MONEDAS_ESPECIAL_MULTA
            ? jugador.monedas
            : MONEDAS_ESPECIAL_MULTA;

        if (perdida > 0 && EscudoAbsorbePerdida(partida, indiceParticipante))
        {
            perdida = 0;
            EscribirTextoEscudo(partida, indiceParticipante);
        }

        jugador.monedas -= perdida;
        partida.variacionMonedasEvento = -perdida;
    }
}


// REGALO: lo que entrega la casilla. Tabla de premios: monedas (la mitad
// de las veces) u objeto barato (botas o escudo). Con el inventario lleno
// el objeto se convierte en monedas.
struct PremioRegalo
{
    int monedas;
    int objeto;     // OBJETO_VACIO = solo monedas
};

static const PremioRegalo PREMIOS_REGALO[] =
{
    { MONEDAS_REGALO, OBJETO_VACIO },
    { MONEDAS_REGALO, OBJETO_VACIO },
    { 0, OBJETO_BOTAS },
    { 0, OBJETO_ESCUDO }
};

static void ResolverRegaloCasilla(
    PartidaTablero& partida,
    int indiceParticipante
)
{
    EstadoJugadorPartidaTablero& jugador =
        partida.jugadores[indiceParticipante];

    const int cantidadPremios =
        (int)(sizeof(PREMIOS_REGALO) / sizeof(PREMIOS_REGALO[0]));

    PremioRegalo premio =
        PREMIOS_REGALO[GetRandomValue(0, cantidadPremios - 1)];

    // Mochila llena: el objeto se cambia por monedas.
    if (
        premio.objeto != OBJETO_VACIO &&
        !AgregarObjetoInventario(jugador.inventario, premio.objeto)
    )
    {
        premio = PremioRegalo{ MONEDAS_REGALO, OBJETO_VACIO };
    }

    if (premio.objeto != OBJETO_VACIO)
    {
        std::snprintf(
            partida.textoEvento,
            sizeof(partida.textoEvento),
            "¡REGALO! J%d RECIBE: %s",
            indiceParticipante + 1,
            ObtenerDefinicionObjeto(premio.objeto).nombre
        );

        ReproducirSonidoTablero(partida, SONIDO_RECOGER_OBJETO);
        return;
    }

    jugador.monedas += premio.monedas;
    partida.variacionMonedasEvento = premio.monedas;

    std::snprintf(
        partida.textoEvento,
        sizeof(partida.textoEvento),
        "¡REGALO! J%d RECIBE %d MONEDAS",
        indiceParticipante + 1,
        premio.monedas
    );

    ReproducirSonidoTablero(partida, SONIDO_RECOGER_OBJETO);
    ReproducirSonidoTablero(partida, SONIDO_MONEDA);
}


static void ResolverEventoCasillaTableroFinal(
    PartidaTablero& partida
)
{
    int indiceParticipante =
        ObtenerParticipanteTurnoTableroFinal(partida);

    if (indiceParticipante < 0)
    {
        return;
    }

    EstadoJugadorPartidaTablero& jugador =
        partida.jugadores[indiceParticipante];

    const Casilla* casilla =
        partida.tablero.ObtenerCasilla(jugador.casillaActual);

    if (casilla == nullptr)
    {
        return;
    }

    partida.ultimoEventoEspecial = EVENTO_TABLERO_NINGUNO;
    partida.variacionMonedasEvento = 0;
    partida.jugadorIntercambioEvento = -1;
    partida.textoEvento[0] = '\0';
    partida.evento = EventoInteractivo{};

    // Tras un movimiento causado por un evento no se encadena otro evento
    // de tablero: la casilla EVENTO queda sin efecto esa vez.
    if (
        partida.movimientoDeEvento &&
        casilla->tipo == CASILLA_EVENTO
    )
    {
        std::snprintf(
            partida.textoEvento,
            sizeof(partida.textoEvento),
            "CASILLA DE EVENTO: SIN EFECTO TRAS UN EVENTO"
        );
        return;
    }

    // Reglas propias del tablero (gimmick) antes de la regla comun.
    if (
        partida.definicion != nullptr &&
        partida.definicion->ResolverCasilla != nullptr &&
        partida.definicion->ResolverCasilla(
            partida,
            indiceParticipante
        )
    )
    {
        return;
    }

    if (casilla->tipo == CASILLA_POSITIVA)
    {
        jugador.monedas += MONEDAS_CASILLA_POSITIVA;
        partida.variacionMonedasEvento = MONEDAS_CASILLA_POSITIVA;
        ReproducirSonidoTablero(partida, SONIDO_CASILLA_POSITIVA);
    }
    else if (casilla->tipo == CASILLA_NEGATIVA)
    {
        int perdida = jugador.monedas < MONEDAS_CASILLA_NEGATIVA
            ? jugador.monedas
            : MONEDAS_CASILLA_NEGATIVA;

        if (perdida > 0 && EscudoAbsorbePerdida(partida, indiceParticipante))
        {
            perdida = 0;
            EscribirTextoEscudo(partida, indiceParticipante);
        }

        jugador.monedas -= perdida;
        partida.variacionMonedasEvento = -perdida;
        ReproducirSonidoTablero(partida, SONIDO_CASILLA_NEGATIVA);
    }
    else if (casilla->tipo == CASILLA_ESPECIAL)
    {
        ReproducirSonidoTablero(partida, SONIDO_EVENTO_TABLERO);
        ResolverEventoEspecialTablero(
            partida,
            indiceParticipante
        );
    }
    else if (casilla->tipo == CASILLA_REGALO)
    {
        ResolverRegaloCasilla(partida, indiceParticipante);
    }
    else if (casilla->tipo == CASILLA_DUELO)
    {
        IniciarEventoInteractivo(
            partida,
            indiceParticipante,
            EVENTO_INTERACTIVO_DUELO
        );
    }
    else if (casilla->tipo == CASILLA_TIENDA)
    {
        IniciarEventoInteractivo(
            partida,
            indiceParticipante,
            EVENTO_INTERACTIVO_TIENDA
        );
    }
    else if (casilla->tipo == CASILLA_CAOS)
    {
        IniciarEventoInteractivo(
            partida,
            indiceParticipante,
            EVENTO_INTERACTIVO_CAOS
        );
    }
    else if (casilla->tipo == CASILLA_INTERCAMBIO)
    {
        IniciarEventoInteractivo(
            partida,
            indiceParticipante,
            EVENTO_INTERACTIVO_INTERCAMBIO
        );
    }
    else if (casilla->tipo == CASILLA_EVENTO)
    {
        // El hook del tablero no resolvio el evento: este tablero no
        // define uno propio.
        std::snprintf(
            partida.textoEvento,
            sizeof(partida.textoEvento),
            "EVENTO: ESTE TABLERO NO TIENE EVENTO PROPIO"
        );
    }
}


// Cierra el evento interactivo: avanza las casillas que haya ganado o
// termina el turno.
static void ContinuarTrasEventoInteractivo(
    PartidaTablero& partida
)
{
    int pasos = partida.evento.pasosExtra;

    // Duelo aceptado: pasa a la fase de espera (Juego sortea y juega el
    // minijuego 1v1 y devuelve el resultado).
    if (
        partida.evento.tipo == EVENTO_INTERACTIVO_DUELO &&
        partida.evento.dueloSel.aceptado
    )
    {
        const EstadoDueloSeleccion& seleccion = partida.evento.dueloSel;
        int retador = seleccion.retador;
        int rival = seleccion.rivales[seleccion.rivalSeleccion];
        OpcionApuestaDuelo apuesta = seleccion.opciones[seleccion.opcionSeleccion];

        partida.evento = EventoInteractivo{};
        IniciarDueloTablero(partida, retador, rival, apuesta);
        return;
    }

    partida.evento = EventoInteractivo{};

    if (pasos > 0)
    {
        // Usa el movimiento normal: bifurcaciones, trofeo y efecto de la
        // casilla de llegada se resuelven como en cualquier tirada.
        partida.movimientoDeEvento = true;
        partida.pasosPendientes = pasos;
        PrepararSiguientePasoTableroFinal(partida);
        return;
    }

    partida.fase = FASE_PARTIDA_TABLERO_FIN_TURNO;
    partida.tiempoFase = 0.0f;
}


void PartidaTablero::Inicializar(
    Participante participantesJuego[],
    int cantidadParticipantesJuego,
    AudioJuego* audioJuego,
    IdTablero id
)
{
    participantes = participantesJuego;
    audio = audioJuego;

    if (
        id < 0 ||
        id >= CANTIDAD_TABLEROS
    )
    {
        id = TABLERO_ISLA_ARBOLEDA;
    }

    idTablero = id;
    definicion = &ObtenerDefinicionTablero(id);

    camara.position = definicion->camaraPosicion;
    camara.target = definicion->camaraObjetivo;
    camara.up = { 0.0f, 1.0f, 0.0f };
    camara.fovy = definicion->camaraFovy;
    camara.projection = CAMERA_PERSPECTIVE;

    camaraFoco = definicion->camaraObjetivo;
    camaraZoom = 1.0f;

    (void)cantidadParticipantesJuego;

    inicializado = true;
    Reiniciar();
}


void PartidaTablero::Reiniciar()
{
    if (!inicializado)
    {
        return;
    }

    cantidadJugadores = 0;
    indiceOrdenTurno = 0;
    rondaActual = 1;
    cantidadRondas = definicion->cantidadRondas;
    costoTrofeo = definicion->costoTrofeo;
    valorDado = 0;
    pasosPendientes = 0;
    opcionRuta = 0;
    direccionRutaBloqueada = false;
    casillaDestinoMovimiento = -1;
    progresoMovimiento = 0.0f;
    tiempoFase = 0.0f;
    compraTrofeoDisponible = false;
    compraTrofeoResuelta = false;
    ultimoEventoEspecial = EVENTO_TABLERO_NINGUNO;
    variacionMonedasEvento = 0;
    jugadorIntercambioEvento = -1;
    evento = EventoInteractivo{};
    movimientoDeEvento = false;
    duelo = DueloTablero{};
    menuObjetos = MenuObjetosTablero{};
    botRevisoObjetos = false;
    detalleDado[0] = '\0';
    avisoObjeto[0] = '\0';
    tiempoAvisoObjeto = 0.0f;
    minijuegoSolicitado = false;
    salidaSolicitada = false;

    // Reconstruye el tablero: deja cerradas/abiertas las conexiones
    // opcionales y el gimmick como al principio de la partida.
    tablero.recorridoValido = false;
    definicion->Construir(tablero);

    tiempoTablero = 0.0f;
    gimmickActivo = false;
    gimmickAnim = 0.0f;
    casillaPeligro = -1;
    textoEvento[0] = '\0';
    estadoGimmick[0] = '\0';
    mensajeGimmick[0] = '\0';
    tiempoMensajeGimmick = 0.0f;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        ordenParticipantes[i] = -1;
        jugadores[i] = EstadoJugadorPartidaTablero{};
    }

    if (participantes == nullptr)
    {
        fase = FASE_PARTIDA_TABLERO_TERMINADA;
        return;
    }

    cantidadJugadores =
        ObtenerIndicesParticipantesActivos(
            participantes,
            ordenParticipantes,
            MAX_PARTICIPANTES
        );

    MezclarOrdenParticipantes(
        ordenParticipantes,
        cantidadJugadores
    );

    for (int i = 0; i < cantidadJugadores; i++)
    {
        int indiceParticipante = ordenParticipantes[i];
        EstadoJugadorPartidaTablero& jugador =
            jugadores[indiceParticipante];

        jugador.participa = true;
        jugador.casillaActual = 0;
        jugador.monedas = MONEDAS_INICIALES_TABLERO;
        jugador.trofeos = 0;
        jugador.premioUltimoMinijuego = 0;
        jugador.posicionVisual =
            ObtenerPosicionFichaTableroFinal(
                *this,
                0,
                indiceParticipante
            );
    }

    casillaTrofeo = -1;
    ElegirNuevaCasillaTrofeo(*this);

    if (
        cantidadJugadores <= 0 ||
        !tablero.recorridoValido
    )
    {
        fase = FASE_PARTIDA_TABLERO_TERMINADA;
        return;
    }

    IniciarRondaTablero(*this);

    // La primera ronda arranca con la animacion ya en su estado final.
    gimmickAnim = gimmickActivo ? 1.0f : 0.0f;

    PrepararInicioTurno(*this);
}


// ESC o START de cualquier mando conectado abre la confirmacion de salida
// (la gestiona Juego): quien juega solo con mando tambien puede pausar.
static bool SolicitaPausaTablero()
{
    if (IsKeyPressed(KEY_ESCAPE))
    {
        return true;
    }

    for (int mando = 0; mando < MAX_PARTICIPANTES; mando++)
    {
        if (
            IsGamepadAvailable(mando) &&
            IsGamepadButtonPressed(mando, GAMEPAD_BUTTON_MIDDLE_RIGHT)
        )
        {
            return true;
        }
    }

    return false;
}


// Caja que encuadra varios puntos (para bifurcaciones y vistas generales).
struct CajaCamaraTablero
{
    float minX = 1.0e9f;
    float maxX = -1.0e9f;
    float minZ = 1.0e9f;
    float maxZ = -1.0e9f;

    void Incluir(float x, float z)
    {
        if (x < minX) minX = x;
        if (x > maxX) maxX = x;
        if (z < minZ) minZ = z;
        if (z > maxZ) maxZ = z;
    }
};


// Sigue una cadena de casillas con una sola salida y devuelve la que
// queda a "pasos" de distancia (se detiene en la proxima bifurcacion).
static const Casilla* AvanzarCadenaTablero(
    const PartidaTablero& partida,
    int indice,
    int pasos
)
{
    const Casilla* casilla = partida.tablero.ObtenerCasilla(indice);

    for (int i = 0; i < pasos && casilla != nullptr; i++)
    {
        if (casilla->cantidadConexiones != 1)
        {
            break;
        }

        const Casilla* siguiente =
            partida.tablero.ObtenerCasilla(casilla->conexiones[0].destino);

        if (siguiente == nullptr)
        {
            break;
        }

        casilla = siguiente;
    }

    return casilla;
}


// Camara diorama: sigue al jugador activo con suavizado, da una vista
// general breve al empezar la ronda, encuadra ambas rutas en las
// bifurcaciones y se acerca a los eventos importantes (trofeo, gimmick).
static void ActualizarCamaraTablero(
    PartidaTablero& partida,
    float deltaTime
)
{
    const DefinicionTablero& def = *partida.definicion;

    // Temporizadores de enfoque.
    if (partida.rondaActual != partida.camaraRondaVista)
    {
        partida.camaraRondaVista = partida.rondaActual;
        partida.camaraTiempoVista = 1.8f;
    }

    if (partida.casillaTrofeo != partida.camaraTrofeoVisto)
    {
        partida.camaraTrofeoVisto = partida.casillaTrofeo;
        partida.camaraTiempoTrofeo = 2.0f;
    }

    if (partida.camaraTiempoVista > 0.0f)
    {
        partida.camaraTiempoVista -= deltaTime;
    }
    else if (partida.camaraTiempoTrofeo > 0.0f)
    {
        partida.camaraTiempoTrofeo -= deltaTime;
    }

    Vector3 centro = def.camaraObjetivo;
    Vector3 foco = centro;
    float zoom = 1.0f;

    bool vistaGeneral =
        def.camaraZoomGeneral > 1.05f &&
        (
            partida.camaraTiempoVista > 0.0f ||
            partida.fase == FASE_PARTIDA_TABLERO_TERMINADA
        );

    int turno = ObtenerParticipanteTurnoTableroFinal(partida);

    if (vistaGeneral)
    {
        zoom = def.camaraZoomGeneral;
    }
    else if (turno >= 0)
    {
        Vector3 ficha = partida.jugadores[turno].posicionVisual;
        ficha.y = 0.4f;
        ficha.z += 1.6f;

        float k = def.camaraSeguimiento;

        foco =
        {
            centro.x + (ficha.x - centro.x) * k,
            centro.y + (ficha.y - centro.y) * k,
            centro.z + (ficha.z - centro.z) * k
        };

        const Casilla* trofeo =
            partida.tablero.ObtenerCasilla(partida.casillaTrofeo);

        const Casilla* actual =
            partida.tablero.ObtenerCasilla(
                partida.jugadores[turno].casillaActual
            );

        if (
            partida.fase == FASE_PARTIDA_TABLERO_ELIGIENDO_RUTA &&
            actual != nullptr &&
            actual->cantidadConexiones > 1
        )
        {
            // Ambas rutas: unos pasos de cada una mas la ficha.
            CajaCamaraTablero caja;
            caja.Incluir(ficha.x, ficha.z);

            for (int o = 0; o < actual->cantidadConexiones; o++)
            {
                int destino = actual->conexiones[o].destino;

                const Casilla* cerca = AvanzarCadenaTablero(partida, destino, 0);
                const Casilla* lejos = AvanzarCadenaTablero(partida, destino, 6);

                if (cerca != nullptr)
                {
                    caja.Incluir(cerca->posicion.x, cerca->posicion.z);
                }

                if (lejos != nullptr)
                {
                    caja.Incluir(lejos->posicion.x, lejos->posicion.z);
                }
            }

            foco.x = (caja.minX + caja.maxX) * 0.5f;
            foco.y = 0.4f;
            foco.z = (caja.minZ + caja.maxZ) * 0.5f + 1.2f;

            float mitad = (caja.maxX - caja.minX) * 0.5f;
            float mitadZ = (caja.maxZ - caja.minZ) * 0.75f;

            if (mitadZ > mitad)
            {
                mitad = mitadZ;
            }

            zoom = mitad / 8.0f;

            if (zoom < 1.2f) zoom = 1.2f;
            if (zoom > 2.0f) zoom = 2.0f;
        }
        else if (
            partida.fase == FASE_PARTIDA_TABLERO_DECISION_TROFEO &&
            trofeo != nullptr
        )
        {
            foco.x = (ficha.x + trofeo->posicion.x) * 0.5f;
            foco.y = 0.4f;
            foco.z = (ficha.z + trofeo->posicion.z) * 0.5f;
            zoom = 0.78f;
        }
        else if (
            partida.camaraTiempoTrofeo > 0.0f &&
            partida.camaraTiempoVista <= 0.0f &&
            partida.fase != FASE_PARTIDA_TABLERO_MOVIENDO &&
            partida.fase != FASE_PARTIDA_TABLERO_ELIGIENDO_RUTA &&
            partida.fase != FASE_PARTIDA_TABLERO_TERMINADA &&
            trofeo != nullptr
        )
        {
            // Nueva ubicacion del trofeo: ficha y trofeo en pantalla.
            foco.x = (ficha.x + trofeo->posicion.x * 2.0f) / 3.0f;
            foco.y = 0.4f;
            foco.z = (ficha.z + trofeo->posicion.z * 2.0f) / 3.0f;

            float dx = ficha.x - trofeo->posicion.x;
            float dz = ficha.z - trofeo->posicion.z;
            float d = std::sqrt(dx * dx + dz * dz);

            zoom = 0.9f + d / 28.0f;

            if (zoom > 2.0f) zoom = 2.0f;
        }
        else if (
            partida.gimmickActivo &&
            def.tieneLandmark &&
            partida.tiempoMensajeGimmick > 2.4f
        )
        {
            foco = def.camaraLandmark;
            zoom = 1.08f;
        }
        else if (partida.fase == FASE_PARTIDA_TABLERO_MOVIENDO)
        {
            zoom = 0.94f;
        }
    }

    float suavizado = 1.0f - std::exp(-3.0f * deltaTime);

    partida.camaraFoco.x += (foco.x - partida.camaraFoco.x) * suavizado;
    partida.camaraFoco.y += (foco.y - partida.camaraFoco.y) * suavizado;
    partida.camaraFoco.z += (foco.z - partida.camaraFoco.z) * suavizado;
    partida.camaraZoom += (zoom - partida.camaraZoom) * suavizado;

    Vector3 desfase =
    {
        def.camaraPosicion.x - def.camaraObjetivo.x,
        def.camaraPosicion.y - def.camaraObjetivo.y,
        def.camaraPosicion.z - def.camaraObjetivo.z
    };

    // Giro muy leve segun la posicion lateral (paralaje sin marear).
    float giro = partida.camaraFoco.x * 0.003f;
    float c = std::cos(giro);
    float s = std::sin(giro);

    partida.camara.target = partida.camaraFoco;
    partida.camara.position =
    {
        partida.camaraFoco.x + (desfase.x * c + desfase.z * s) * partida.camaraZoom,
        partida.camaraFoco.y + desfase.y * partida.camaraZoom,
        partida.camaraFoco.z + (desfase.z * c - desfase.x * s) * partida.camaraZoom
    };
}


void PartidaTablero::Actualizar(
    float deltaTime
)
{
    if (
        !inicializado ||
        participantes == nullptr ||
        cantidadJugadores <= 0 ||
        !tablero.recorridoValido
    )
    {
        return;
    }

    tiempoFase += deltaTime;
    tiempoTablero += deltaTime;

    if (tiempoMensajeGimmick > 0.0f)
    {
        tiempoMensajeGimmick -= deltaTime;
    }

    if (tiempoAvisoObjeto > 0.0f)
    {
        tiempoAvisoObjeto -= deltaTime;
    }

    if (definicion->ActualizarGimmick != nullptr)
    {
        definicion->ActualizarGimmick(*this, deltaTime);
    }

    ActualizarCamaraTablero(*this, deltaTime);

    if (fase == FASE_PARTIDA_TABLERO_TERMINADA)
    {
        // Ignora la pulsacion con la que se confirmo el ultimo minijuego.
        if (tiempoFase < 0.5f)
        {
            return;
        }

        if (IsKeyPressed(KEY_R))
        {
            Reiniciar();
            return;
        }

        if (
            IsKeyPressed(KEY_ESCAPE) ||
            IsKeyPressed(KEY_ENTER)
        )
        {
            salidaSolicitada = true;
        }

        return;
    }

    if (
        fase != FASE_PARTIDA_TABLERO_DECISION_TROFEO &&
        SolicitaPausaTablero()
    )
    {
        salidaSolicitada = true;
        return;
    }

    int indiceParticipante =
        ObtenerParticipanteTurnoTableroFinal(*this);

    if (indiceParticipante < 0)
    {
        return;
    }

    const Participante& participante =
        participantes[indiceParticipante];

    // Un humano desconectado no bloquea la partida: tras 4 s de espera
    // en su fase, la IA juega por el hasta que reconecte.
    bool esBot =
        participante.esBot ||
        (!participante.conectado && tiempoFase >= 4.0f);
    bool puedeDecidir =
        ParticipantePuedeDecidirTablero(
            *this,
            indiceParticipante
        );

    if (fase == FASE_PARTIDA_TABLERO_ESPERANDO_DADO)
    {
        bool lanzar = false;

        if (esBot)
        {
            // Antes de tirar, el bot decide si usa un objeto (una vez por
            // turno) y espera un instante para que se vea el aviso.
            if (!botRevisoObjetos && tiempoFase >= 0.35f)
            {
                botRevisoObjetos = true;

                int rival = -1;
                int espacio = DecidirObjetoBot(*this, indiceParticipante, rival);

                if (espacio >= 0)
                {
                    char resumen[96];

                    UsarObjeto(*this, indiceParticipante, espacio, rival, resumen, (int)sizeof(resumen));
                    tiempoFase = 0.0f;
                }
            }

            lanzar = tiempoFase >= 0.70f;
        }
        else if (puedeDecidir)
        {
            InputSeleccionParticipante entrada =
                lectorEntrada(participante);

            lanzar = entrada.confirmar;

            // Cancelar abre el menu de objetos (si hay algo para usar).
            if (
                !lanzar &&
                entrada.cancelar &&
                HayObjetoUsableAhora(*this, indiceParticipante)
            )
            {
                ReproducirSonidoTablero(*this, SONIDO_UI_CONFIRMAR);
                IniciarMenuObjetos(*this, indiceParticipante);
                fase = FASE_PARTIDA_TABLERO_MENU_OBJETOS;
                tiempoFase = 0.0f;
                return;
            }
        }

        if (lanzar)
        {
            valorDado = TirarDadoDeJugador(
                *this, indiceParticipante,
                detalleDado, (int)sizeof(detalleDado)
            );
            pasosPendientes = valorDado;
            fase = FASE_PARTIDA_TABLERO_MOSTRANDO_DADO;
            tiempoFase = 0.0f;

            ReproducirSonidoTablero(
                *this,
                SONIDO_DADO
            );
        }
    }
    else if (fase == FASE_PARTIDA_TABLERO_DUELO_RESULTADO)
    {
        InputSeleccionParticipante entrada{};

        if (puedeDecidir)
        {
            entrada = lectorEntrada(participante);
        }

        ActualizarDueloResultado(*this, deltaTime, esBot, puedeDecidir, entrada);
    }
    // DUELO_PENDIENTE: Juego juega el minijuego 1v1 y devuelve el resultado.
    else if (fase == FASE_PARTIDA_TABLERO_MENU_OBJETOS)
    {
        InputSeleccionParticipante entrada{};

        if (puedeDecidir)
        {
            entrada = lectorEntrada(participante);
        }

        ActualizarMenuObjetos(
            *this,
            deltaTime,
            esBot,
            puedeDecidir,
            entrada
        );

        if (menuObjetos.terminado || !menuObjetos.activo)
        {
            menuObjetos = MenuObjetosTablero{};
            fase = FASE_PARTIDA_TABLERO_ESPERANDO_DADO;

            // El humano no pierde su turno por usar un objeto: la espera
            // del dado vuelve a empezar (sin reloj de bot).
            tiempoFase = 0.0f;
            botRevisoObjetos = true;
        }
    }
    else if (fase == FASE_PARTIDA_TABLERO_MOSTRANDO_DADO)
    {
        if (tiempoFase >= 0.85f)
        {
            PrepararSiguientePasoTableroFinal(*this);
        }
    }
    else if (fase == FASE_PARTIDA_TABLERO_ELIGIENDO_RUTA)
    {
        const EstadoJugadorPartidaTablero& jugador =
            jugadores[indiceParticipante];

        const Casilla* casilla =
            tablero.ObtenerCasilla(jugador.casillaActual);

        if (
            casilla == nullptr ||
            casilla->cantidadConexiones <= 1
        )
        {
            PrepararSiguientePasoTableroFinal(*this);
            return;
        }

        if (esBot)
        {
            if (tiempoFase >= 0.55f)
            {
                // El tablero puede dar criterio al bot; si no, al azar.
                int decision = -1;

                if (definicion->DecidirRutaBot != nullptr)
                {
                    decision =
                        definicion->DecidirRutaBot(
                            *this,
                            indiceParticipante
                        );
                }

                if (
                    decision < 0 ||
                    decision >= casilla->cantidadConexiones
                )
                {
                    decision =
                        GetRandomValue(
                            0,
                            casilla->cantidadConexiones - 1
                        );
                }

                opcionRuta = decision;

                IniciarMovimientoHaciaTableroFinal(
                    *this,
                    casilla->conexiones[opcionRuta].destino
                );
            }

            return;
        }

        if (!puedeDecidir)
        {
            return;
        }

        InputSeleccionParticipante entrada =
            lectorEntrada(participante);

        bool direccionPresionada =
            entrada.izquierda ||
            entrada.derecha ||
            entrada.arriba ||
            entrada.abajo;

        int opcionAnterior = opcionRuta;

        if (
            !direccionRutaBloqueada &&
            (entrada.izquierda || entrada.arriba)
        )
        {
            opcionRuta--;
        }

        if (
            !direccionRutaBloqueada &&
            (entrada.derecha || entrada.abajo)
        )
        {
            opcionRuta++;
        }

        direccionRutaBloqueada = direccionPresionada;

        if (opcionRuta < 0)
        {
            opcionRuta = casilla->cantidadConexiones - 1;
        }

        if (opcionRuta >= casilla->cantidadConexiones)
        {
            opcionRuta = 0;
        }

        if (opcionRuta != opcionAnterior)
        {
            ReproducirSonidoTablero(
                *this,
                SONIDO_UI_MOVER
            );
        }

        if (entrada.confirmar)
        {
            ReproducirSonidoTablero(
                *this,
                SONIDO_UI_CONFIRMAR
            );

            IniciarMovimientoHaciaTableroFinal(
                *this,
                casilla->conexiones[opcionRuta].destino
            );
        }
    }
    else if (fase == FASE_PARTIDA_TABLERO_MOVIENDO)
    {
        ActualizarMovimientoTableroFinal(
            *this,
            deltaTime
        );
    }
    else if (fase == FASE_PARTIDA_TABLERO_DECISION_TROFEO)
    {
        if (!compraTrofeoDisponible)
        {
            if (tiempoFase >= 0.75f)
            {
                ContinuarTrasDecisionTrofeo(*this);
            }

            return;
        }

        if (esBot)
        {
            if (tiempoFase >= 0.55f)
            {
                ComprarTrofeo(*this, indiceParticipante);
            }

            return;
        }

        if (!puedeDecidir)
        {
            return;
        }

        InputSeleccionParticipante entrada =
            lectorEntrada(participante);

        if (entrada.confirmar || IsKeyPressed(KEY_Y))
        {
            ReproducirSonidoTablero(
                *this,
                SONIDO_UI_CONFIRMAR
            );

            ComprarTrofeo(*this, indiceParticipante);
        }
        else if (
            entrada.cancelar ||
            IsKeyPressed(KEY_N) ||
            IsKeyPressed(KEY_ESCAPE)
        )
        {
            ContinuarTrasDecisionTrofeo(*this);
        }
    }
    else if (fase == FASE_PARTIDA_TABLERO_EVENTO_CASILLA)
    {
        if (tiempoFase <= deltaTime + 0.0001f)
        {
            ResolverEventoCasillaTableroFinal(*this);

            // CAOS, INTERCAMBIO y los eventos del tablero siguen en su
            // propia fase con presentacion e interaccion.
            if (evento.activo)
            {
                fase = FASE_PARTIDA_TABLERO_EVENTO_INTERACTIVO;
                tiempoFase = 0.0f;
                return;
            }
        }

        if (tiempoFase >= 1.35f)
        {
            fase = FASE_PARTIDA_TABLERO_FIN_TURNO;
            tiempoFase = 0.0f;
        }
    }
    else if (fase == FASE_PARTIDA_TABLERO_EVENTO_INTERACTIVO)
    {
        InputSeleccionParticipante entrada{};

        if (puedeDecidir)
        {
            entrada = lectorEntrada(participante);
        }

        ActualizarEventoInteractivo(
            *this,
            deltaTime,
            esBot,
            puedeDecidir,
            entrada
        );

        if (evento.terminado || !evento.activo)
        {
            ContinuarTrasEventoInteractivo(*this);
        }
    }
    else if (fase == FASE_PARTIDA_TABLERO_FIN_TURNO)
    {
        if (tiempoFase >= 0.40f)
        {
            AvanzarTurnoTableroFinal(*this);
        }
    }
    else if (fase == FASE_PARTIDA_TABLERO_FIN_RONDA)
    {
        if (
            !minijuegoSolicitado &&
            tiempoFase >= 1.0f
        )
        {
            minijuegoSolicitado = true;
        }
    }
}


bool PartidaTablero::SolicitaMinijuego() const
{
    return
        (
            fase == FASE_PARTIDA_TABLERO_FIN_RONDA &&
            minijuegoSolicitado
        ) ||
        EsDueloPendiente();
}


bool PartidaTablero::EsDueloPendiente() const
{
    return fase == FASE_PARTIDA_TABLERO_DUELO_PENDIENTE;
}


void PartidaTablero::AplicarResultadoMinijuego(
    const ResultadoMinijuego& resultado
)
{
    // Un duelo paga solo la apuesta: sin los premios de ronda.
    if (EsDueloPendiente())
    {
        ResolverDueloTablero(*this, &resultado);
        return;
    }

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        jugadores[i].premioUltimoMinijuego = 0;

        if (!jugadores[i].participa)
        {
            continue;
        }

        int premio =
            ParticipanteEsGanador(resultado, i)
            ? PREMIO_GANADOR_MINIJUEGO
            : PREMIO_PARTICIPACION_MINIJUEGO;

        jugadores[i].monedas += premio;
        jugadores[i].premioUltimoMinijuego = premio;
    }

    ReproducirSonidoTablero(
        *this,
        SONIDO_RESULTADO
    );
}


void PartidaTablero::ContinuarTrasMinijuego()
{
    // Duelo sin resultado (ronda cancelada o fallida): se devuelve la
    // apuesta. Con el resultado ya aplicado no hay nada mas que hacer: la
    // partida sigue en su fase de resultado del duelo.
    if (EsDueloPendiente())
    {
        ResolverDueloTablero(*this, nullptr);
        return;
    }

    if (fase == FASE_PARTIDA_TABLERO_DUELO_RESULTADO)
    {
        return;
    }

    minijuegoSolicitado = false;

    if (rondaActual >= cantidadRondas)
    {
        fase = FASE_PARTIDA_TABLERO_TERMINADA;
        tiempoFase = 0.0f;
        return;
    }

    rondaActual++;
    indiceOrdenTurno = 0;
    IniciarRondaTablero(*this);
    PrepararInicioTurno(*this);
}


bool PartidaTablero::SolicitaSalida() const
{
    return salidaSolicitada;
}


static void DibujarTrofeoTableroFinal(
    const PartidaTablero& partida
)
{
    const Casilla* casilla =
        partida.tablero.ObtenerCasilla(partida.casillaTrofeo);

    if (casilla == nullptr)
    {
        return;
    }

    // MODELO FUTURO: pedestal y trofeo como modelo GLB.
    float t = partida.tiempoTablero;
    Vector3 p = casilla->posicion;
    float techo = p.y + 0.22f;

    // Pedestal de piedra en dos escalones.
    DrawCylinderEx(
        Vector3{ p.x, techo, p.z },
        Vector3{ p.x, techo + 0.20f, p.z },
        0.52f, 0.46f, 10,
        Color{ 120, 116, 124, 255 }
    );

    DrawCylinderEx(
        Vector3{ p.x, techo + 0.20f, p.z },
        Vector3{ p.x, techo + 0.50f, p.z },
        0.34f, 0.28f, 10,
        Color{ 168, 162, 172, 255 }
    );

    // Copa dorada flotando y girando sobre el pedestal.
    float flotar = 0.12f * std::sin(t * 2.2f);
    float y0 = techo + 0.78f + flotar;

    rlPushMatrix();
    rlTranslatef(p.x, y0, p.z);
    rlRotatef(t * 70.0f, 0.0f, 1.0f, 0.0f);

    DrawCylinderEx(Vector3{ 0.0f, -0.28f, 0.0f }, Vector3{ 0.0f, -0.12f, 0.0f },
        0.22f, 0.10f, 10, GOLD);
    DrawCylinderEx(Vector3{ 0.0f, -0.12f, 0.0f }, Vector3{ 0.0f, 0.34f, 0.0f },
        0.07f, 0.36f, 10, Color{ 255, 214, 60, 255 });
    DrawCylinderEx(Vector3{ 0.0f, 0.30f, 0.0f }, Vector3{ 0.0f, 0.36f, 0.0f },
        0.36f, 0.36f, 10, Color{ 255, 240, 150, 255 });
    DrawCubeV(Vector3{ 0.40f, 0.14f, 0.0f }, Vector3{ 0.08f, 0.26f, 0.08f }, GOLD);
    DrawCubeV(Vector3{ -0.40f, 0.14f, 0.0f }, Vector3{ 0.08f, 0.26f, 0.08f }, GOLD);

    rlPopMatrix();
}


// Haz de luz y chispas del trofeo (translucidos: se dibujan al final).
static void DibujarEfectosTrofeoTableroFinal(
    const PartidaTablero& partida
)
{
    const Casilla* casilla =
        partida.tablero.ObtenerCasilla(partida.casillaTrofeo);

    if (casilla == nullptr)
    {
        return;
    }

    float t = partida.tiempoTablero;
    Vector3 p = casilla->posicion;
    float pulso = 0.5f + 0.5f * std::sin(t * 3.0f);

    DrawCylinderEx(
        Vector3{ p.x, p.y + 0.2f, p.z },
        Vector3{ p.x, p.y + 9.0f, p.z },
        0.55f + 0.1f * pulso, 0.18f, 10,
        Color{ 255, 226, 110, (unsigned char)(34 + 22 * pulso) }
    );

    DrawCylinderEx(
        Vector3{ p.x, p.y + 0.2f, p.z },
        Vector3{ p.x, p.y + 0.24f, p.z },
        1.15f + 0.15f * pulso, 1.15f + 0.15f * pulso, 16,
        Color{ 255, 226, 110, 60 }
    );

    for (int i = 0; i < 12; i++)
    {
        float fase = std::fmod(t * 0.45f + i * 0.083f, 1.0f);
        float ang = t * 1.2f + i * 0.9f;
        float radio = 0.55f + 0.25f * std::sin(i * 1.7f);

        DrawSphereEx(
            Vector3
            {
                p.x + std::cos(ang) * radio,
                p.y + 0.5f + fase * 3.2f,
                p.z + std::sin(ang) * radio
            },
            0.06f * (1.0f - fase * 0.6f), 3, 4,
            Color{ 255, 240, 160, (unsigned char)(230 * (1.0f - fase)) }
        );
    }
}


static void DibujarFichasTableroFinal(
    const PartidaTablero& partida
)
{
    int participanteTurno =
        ObtenerParticipanteTurnoTableroFinal(partida);

    for (int i = 0; i < partida.cantidadJugadores; i++)
    {
        int indiceParticipante = partida.ordenParticipantes[i];
        const EstadoJugadorPartidaTablero& jugador =
            partida.jugadores[indiceParticipante];

        Color color =
            partida.participantes[indiceParticipante].color;

        Vector3 cuerpo = jugador.posicionVisual;

        Vector3 posicionPies = cuerpo;
        posicionPies.y -= 0.31f;

        // Sombra blanda sobre la losa, mas pequena cuanto mas alto salta.
        {
            float alturaSalto = cuerpo.y - 1.03f;
            if (alturaSalto < 0.0f) alturaSalto = 0.0f;

            DrawCylinderEx(
                Vector3{ cuerpo.x, 0.475f, cuerpo.z },
                Vector3{ cuerpo.x, 0.480f, cuerpo.z },
                0.26f / (1.0f + alturaSalto), 0.26f / (1.0f + alturaSalto), 10,
                Color{ 20, 20, 30, 110 }
            );
        }

        DibujarModeloJugadorEnPosicion(
            posicionPies,
            0.0f,
            color,
            0.18f
        );

        if (
            indiceParticipante == participanteTurno &&
            partida.fase != FASE_PARTIDA_TABLERO_TERMINADA
        )
        {
            // Flecha amarilla que rebota sobre la ficha activa.
            float rebote = 0.12f * std::sin(partida.tiempoTablero * 5.0f);
            Vector3 cima = { cuerpo.x, cuerpo.y + 1.25f + rebote, cuerpo.z };

            DrawCylinderEx(
                cima,
                Vector3{ cima.x, cima.y - 0.34f, cima.z },
                0.24f, 0.0f, 4,
                Color{ 255, 220, 40, 255 }
            );

            DrawCylinderEx(
                Vector3{ cuerpo.x, 0.48f, cuerpo.z },
                Vector3{ cuerpo.x, 0.50f, cuerpo.z },
                0.46f, 0.46f, 18,
                Color{ 255, 230, 80, 90 }
            );
        }
    }
}


static void DibujarRutaSeleccionadaTableroFinal(
    const PartidaTablero& partida
)
{
    if (partida.fase != FASE_PARTIDA_TABLERO_ELIGIENDO_RUTA)
    {
        return;
    }

    int indiceParticipante =
        ObtenerParticipanteTurnoTableroFinal(partida);

    if (indiceParticipante < 0)
    {
        return;
    }

    const EstadoJugadorPartidaTablero& jugador =
        partida.jugadores[indiceParticipante];

    const Casilla* origen =
        partida.tablero.ObtenerCasilla(jugador.casillaActual);

    if (
        origen == nullptr ||
        partida.opcionRuta < 0 ||
        partida.opcionRuta >= origen->cantidadConexiones
    )
    {
        return;
    }

    const Casilla* destino =
        partida.tablero.ObtenerCasilla(
            origen->conexiones[partida.opcionRuta].destino
        );

    if (destino == nullptr)
    {
        return;
    }

    // Anillo pulsante y flecha sobre la casilla de la ruta elegida.
    float pulso = 0.5f + 0.5f * std::sin(partida.tiempoTablero * 6.0f);
    Vector3 m = destino->posicion;

    DrawCylinderEx(
        Vector3{ m.x, m.y + 0.23f, m.z },
        Vector3{ m.x, m.y + 0.26f, m.z },
        1.0f + 0.1f * pulso, 1.0f + 0.1f * pulso, 18,
        Color{ 255, 230, 60, (unsigned char)(90 + 80 * pulso) }
    );

    DrawCylinderEx(
        Vector3{ m.x, m.y + 1.7f + 0.15f * pulso, m.z },
        Vector3{ m.x, m.y + 1.2f + 0.15f * pulso, m.z },
        0.34f, 0.0f, 4,
        YELLOW
    );
}


//==================================================
// HUD DEL TABLERO: «CARTELES DE PLAYA»
//==================================================
//
// Misma familia que el HUD de minijuegos (Minigames/HudMinijuego.cpp):
//  - arriba cruza una soga de la que cuelgan, con pinzas, las etiquetas
//    de los jugadores en orden de turno (trofeos y monedas) y, en el
//    medio, el cartel de la ronda. Del cartel de ronda cuelga una tira
//    con el tablero, la zona y el estado del gimmick; mientras dura un
//    aviso del gimmick, la tira se cambia por un cartel mas grande;
//  - sobre la ficha activa flota el dado: gira mientras se espera la
//    tirada, muestra el resultado y despues los pasos que quedan;
//  - en una bifurcacion, etiquetas «RUTA 1 / RUTA 2» sobre cada camino;
//  - abajo, el cartel del turno (quien juega y que pasa) y la ayuda con
//    las teclas o botones reales del jugador que decide;
//  - la compra del trofeo y el resultado final tienen tablon propio.
// Los anuncios grandes («RONDA 2», «¡FIN DE LA RONDA!», el +3 de una
// casilla) van sobre el mundo, en texto claro con contorno.
//
// El HUD SOLO LEE la partida. Como Dibujar es const, su estado de
// animacion vive en este .cpp y avanza con el reloj del EstiloUI
// compartido; respeta reducirMovimiento. Medidas en unidades de 1280x720.
//==================================================

static const float ANCHO_ETIQUETA_HUD = 168.0f;
static const float ALTO_ETIQUETA_HUD = 64.0f;
static const float SEPARACION_ETIQUETAS_HUD = 10.0f;
static const float ANCHO_RONDA_HUD = 128.0f;
static const float ALTO_RONDA_HUD = 76.0f;
static const float SEPARACION_RONDA_HUD = 14.0f;

static const float Y_SOGA_HUD = 14.0f;          // altura de la soga en los extremos
static const float COMBA_SOGA_HUD = 6.0f;       // cuanto baja en el centro
static const float SOGA_SOBRANTE_HUD = 30.0f;   // soga a cada lado de la fila
static const float BAJADA_TURNO_HUD = 12.0f;    // la etiqueta del turno cuelga mas abajo

static const float ANCHO_CARTEL_TURNO = 540.0f;
static const float ALTO_CARTEL_TURNO = 56.0f;
static const float Y_CARTEL_TURNO = 610.0f;     // anclado abajo (720 = borde)
static const float ANCHO_TABLON_TROFEO = 560.0f;
static const float ALTO_TABLON_TROFEO = 150.0f;
static const float LADO_DADO_HUD = 56.0f;

// Tiempos (segundos).
static const float T_ANUNCIO_RONDA = 1.8f;
static const float T_ANUNCIO_TROFEO = 1.8f;
static const float T_SALTO_HUD = 0.28f;
static const float T_DESTELLO_HUD = 0.6f;
static const float T_DELTA_MONEDAS = 1.6f;
static const float T_ENTRADA_TABLON = 0.30f;
static const float T_EVENTO_CASILLA = 1.35f;    // duracion de la fase (ver Actualizar)

// Colores locales. Las tintas van sobre placa clara (contraste >= 4.5:1).
static const Color COLOR_TINTA_POSITIVA_HUD = { 20, 92, 48, 255 };
static const Color COLOR_TINTA_ALERTA_HUD = { 160, 30, 24, 255 };
static const Color COLOR_PLACA_ALERTA_HUD = { 255, 190, 170, 255 };
static const Color COLOR_CARA_DADO_HUD = { 255, 248, 232, 255 };
static const Color COLOR_MUNDO_POSITIVO_HUD = { 130, 235, 150, 255 };
static const Color COLOR_MUNDO_NEGATIVO_HUD = { 255, 120, 100, 255 };


//==================================================
// ESTADO DE ANIMACION DEL HUD
//==================================================

struct EstadoHudTablero
{
    bool iniciado = false;
    const PartidaTablero* partida = nullptr;
    float reloj = 0.0f;                 // EstiloUI::tiempo del ultimo dibujo

    int ronda = 0;
    float tiempoRonda = 99.0f;          // desde que empezo la ronda (anuncio)

    int monedas[MAX_PARTICIPANTES] = {};
    int trofeos[MAX_PARTICIPANTES] = {};
    int deltaMonedas[MAX_PARTICIPANTES] = {};
    float tiempoMonedas[MAX_PARTICIPANTES] = {};
    float tiempoTrofeo[MAX_PARTICIPANTES] = {};
    int ultimoTrofeo = -1;              // quien consiguio el ultimo trofeo

    int valorDado = -1;                 // numero que muestra el dado
    float tiempoDado = 99.0f;           // desde que cambio (salto)

    float tiempoMensaje = 0.0f;         // tiempoMensajeGimmick del frame anterior
    ResorteUI caidaMensaje;             // caida del cartel de aviso

    ColgadoUI colgado[MAX_PARTICIPANTES];   // balanceo al recibir el turno
    ResorteUI bajada[MAX_PARTICIPANTES];    // la del turno cuelga mas abajo
};


static EstadoHudTablero estadoHudTablero;


static float Limitar01Hud(float v)
{
    return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
}


// Rebote al aparecer: 1 + extra al principio, baja a 1 en `duracion`.
static float EscalaReboteHud(float t, float duracion, float extra, bool reducir)
{
    if (reducir)
    {
        return 1.0f;
    }

    float resto = 1.0f - Limitar01Hud(t / duracion);

    return 1.0f + extra * resto * resto * resto;
}


// Lo que muestra el dado sobre la ficha: -1 oculto, 0 girando (se espera
// la tirada), 1..10 un numero (resultado o pasos que quedan).
static int ValorDadoHud(
    const PartidaTablero& partida
)
{
    int turno = ObtenerParticipanteTurnoTableroFinal(partida);

    if (turno < 0 || partida.participantes == nullptr)
    {
        return -1;
    }

    switch (partida.fase)
    {
        case FASE_PARTIDA_TABLERO_ESPERANDO_DADO:
        {
            const Participante& participante = partida.participantes[turno];

            // Humano sin mando: hasta que juegue el bot no hay tirada.
            if (
                !participante.esBot &&
                !participante.conectado &&
                partida.tiempoFase < 4.0f
            )
            {
                return -1;
            }

            return 0;
        }

        case FASE_PARTIDA_TABLERO_MOSTRANDO_DADO:
            return partida.valorDado;

        // En la bifurcacion el dado taparia las etiquetas de ruta (las
        // casillas estan pegadas a la ficha): los pasos van al cartel.
        case FASE_PARTIDA_TABLERO_MOVIENDO:
        case FASE_PARTIDA_TABLERO_DECISION_TROFEO:
            return partida.pasosPendientes > 0 ? partida.pasosPendientes : -1;

        default:
            break;
    }

    return -1;
}


// Jugador cuya etiqueta se destaca: nadie al terminar la ronda (el
// indice ya apunta al primero de la ronda siguiente) ni la partida.
static int TurnoVisibleHud(const PartidaTablero& partida)
{
    if (
        partida.fase == FASE_PARTIDA_TABLERO_FIN_RONDA ||
        partida.fase == FASE_PARTIDA_TABLERO_TERMINADA
    )
    {
        return -1;
    }

    return ObtenerParticipanteTurnoTableroFinal(partida);
}


static void SincronizarEstadoHudTablero(
    const PartidaTablero& partida
)
{
    float reloj = estadoHudTablero.reloj;

    estadoHudTablero = EstadoHudTablero{};
    estadoHudTablero.iniciado = true;
    estadoHudTablero.partida = &partida;
    estadoHudTablero.reloj = reloj;
    estadoHudTablero.ronda = partida.rondaActual;
    estadoHudTablero.tiempoRonda = 0.0f;
    estadoHudTablero.valorDado = ValorDadoHud(partida);
    estadoHudTablero.tiempoMensaje = partida.tiempoMensajeGimmick;

    int turno = TurnoVisibleHud(partida);

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        estadoHudTablero.monedas[i] = partida.jugadores[i].monedas;
        estadoHudTablero.trofeos[i] = partida.jugadores[i].trofeos;
        estadoHudTablero.tiempoMonedas[i] = 99.0f;
        estadoHudTablero.tiempoTrofeo[i] = 99.0f;

        estadoHudTablero.colgado[i].Iniciar(0.0f, 0.0f, 0.0f, 0.6f);
        estadoHudTablero.colgado[i].aterrizo = true;
        estadoHudTablero.colgado[i].focoPrevio = i == turno;

        estadoHudTablero.bajada[i].valor =
            i == turno ? BAJADA_TURNO_HUD : 0.0f;
    }
}


static void ActualizarEstadoHudTablero(
    const PartidaTablero& partida,
    const EstiloUI& estilo
)
{
    EstadoHudTablero& e = estadoHudTablero;
    bool reducir = estilo.reducirMovimiento;

    // Reloj del EstiloUI compartido (Juego lo avanza cada frame).
    float dt = estilo.tiempo - e.reloj;
    e.reloj = estilo.tiempo;

    // Un hueco largo significa que el tablero no se dibujo un rato
    // (minijuego, menus). Si al volver no es la ronda siguiente, es otra
    // partida: se copia el estado sin animar los cambios.
    bool hueco = !(dt > 0.0f && dt < 0.5f);

    if (hueco)
    {
        dt = GetFrameTime();
        if (dt > 0.1f) dt = 0.1f;
    }

    if (
        !e.iniciado ||
        e.partida != &partida ||
        partida.rondaActual < e.ronda ||
        (hueco && partida.rondaActual != e.ronda + 1)
    )
    {
        SincronizarEstadoHudTablero(partida);
    }

    e.tiempoRonda += dt;

    if (partida.rondaActual != e.ronda)
    {
        e.ronda = partida.rondaActual;
        e.tiempoRonda = 0.0f;
    }

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        const EstadoJugadorPartidaTablero& jugador = partida.jugadores[i];

        if (jugador.monedas != e.monedas[i])
        {
            e.deltaMonedas[i] = jugador.monedas - e.monedas[i];
            e.monedas[i] = jugador.monedas;
            e.tiempoMonedas[i] = 0.0f;
        }
        else
        {
            e.tiempoMonedas[i] += dt;
        }

        if (jugador.trofeos > e.trofeos[i])
        {
            e.tiempoTrofeo[i] = 0.0f;
            e.ultimoTrofeo = i;
        }
        else
        {
            e.tiempoTrofeo[i] += dt;
        }

        e.trofeos[i] = jugador.trofeos;
    }

    int turno = TurnoVisibleHud(partida);

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        bool activo = i == turno;
        float objetivo = activo ? BAJADA_TURNO_HUD : 0.0f;

        e.colgado[i].Actualizar(estilo, dt, activo);

        if (reducir)
        {
            e.bajada[i].valor = objetivo;
            e.bajada[i].velocidad = 0.0f;
        }
        else
        {
            e.bajada[i].Actualizar(objetivo, dt, 160.0f, 14.0f);
        }
    }

    int valorDado = ValorDadoHud(partida);

    if (valorDado != e.valorDado)
    {
        e.valorDado = valorDado;
        e.tiempoDado = 0.0f;
    }
    else
    {
        e.tiempoDado += dt;
    }

    // Aviso nuevo del gimmick: su contador vuelve a subir.
    if (partida.tiempoMensajeGimmick > e.tiempoMensaje + 0.01f)
    {
        e.caidaMensaje.valor = reducir ? 0.0f : -90.0f;
        e.caidaMensaje.velocidad = 0.0f;
    }

    e.tiempoMensaje = partida.tiempoMensajeGimmick;

    if (reducir)
    {
        e.caidaMensaje = ResorteUI{};
    }
    else
    {
        e.caidaMensaje.Actualizar(0.0f, dt, 90.0f, 9.0f);
    }
}


//==================================================
// LIENZO: unidades de referencia ancladas arriba o abajo
//==================================================

static float XHud(float x)
{
    float escala = EscalaUI();

    return ((float)GetScreenWidth() - UI_REF_ANCHO * escala) * 0.5f + x * escala;
}


static float YArribaHud(float y)
{
    return y * EscalaUI();
}


static float YAbajoHud(float y)
{
    return (float)GetScreenHeight() - (UI_REF_ALTO - y) * EscalaUI();
}


// Altura de la soga (unidades ref) en x, con una comba suave.
static float AlturaSogaHud(float x, float inicio, float fin)
{
    float centro = (inicio + fin) * 0.5f;
    float medio = (fin - inicio) * 0.5f;

    if (medio <= 0.0f)
    {
        return Y_SOGA_HUD;
    }

    float u = (x - centro) / medio;

    return Y_SOGA_HUD + COMBA_SOGA_HUD * (1.0f - u * u);
}


static float GrosorLineaHud(float unidades)
{
    float grosor = Px(unidades);

    return grosor < 1.0f ? 1.0f : grosor;
}


//==================================================
// PIEZAS DE DIBUJO
//==================================================

// Tablon grande: madera gruesa y placa clara, con sombra proyectada.
static Rectangle DibujarTablonHud(const EstiloUI& estilo, Rectangle rect)
{
    DrawRectangleRec(
        Rectangle{ rect.x + Px(6.0f), rect.y + Px(8.0f), rect.width, rect.height },
        Fade(BLACK, 0.30f)
    );
    DibujarNueveCortes(estilo.madera, 12, rect, EscalaUI(), WHITE);

    float margen = Px(14.0f);
    Rectangle placa = {
        rect.x + margen, rect.y + margen,
        rect.width - margen * 2.0f, rect.height - margen * 2.0f
    };

    DibujarNueveCortes(estilo.placa, 8, placa, EscalaUI(), WHITE);

    return placa;
}


//==================================================
// TEXTOS DE CONTROLES (fieles a Systems/Input.cpp)
//==================================================

// Confirmar: LeerInputSeleccionParticipante. Con un solo teclado
// (modo completo) confirman ESPACIO y ENTER.
static const char* TextoConfirmarHud(const Participante& participante)
{
    switch (participante.control)
    {
        case CONTROL_GAMEPAD:
            return "A";

        case CONTROL_TECLADO_FLECHAS:
            return "ENTER";

        case CONTROL_TECLADO_WASD:
            return "ESPACIO";

        case CONTROL_TECLADO_COMPLETO:
            return "ESPACIO / ENTER";

        case CONTROL_NINGUNO:
            break;
    }

    return "";
}


// Cambiar de ruta: cualquier direccion (izq/arriba = anterior,
// der/abajo = siguiente).
static const char* TextoElegirHud(const Participante& participante)
{
    switch (participante.control)
    {
        case CONTROL_GAMEPAD:
            return "CRUCETA / STICK";

        case CONTROL_TECLADO_FLECHAS:
            return "FLECHAS";

        case CONTROL_TECLADO_WASD:
            return "WASD";

        case CONTROL_TECLADO_COMPLETO:
            return "WASD / FLECHAS";

        case CONTROL_NINGUNO:
            break;
    }

    return "";
}


// Cancelar (fase de objetos, tienda): abre el menu de objetos antes de
// tirar y sale de los menus. Fiel a Systems/Input.cpp.
static const char* TextoCancelarHud(const Participante& participante)
{
    switch (participante.control)
    {
        case CONTROL_GAMEPAD:
            return "B";

        case CONTROL_TECLADO_FLECHAS:
            return "SHIFT DER";

        case CONTROL_TECLADO_WASD:
        case CONTROL_TECLADO_COMPLETO:
            return "RETROCESO";

        case CONTROL_NINGUNO:
            break;
    }

    return "";
}


// Compra del trofeo: confirmar o Y compran; cancelar, N o ESC no.
// Y, N y ESC son del teclado: solo se anuncian a quien juega con teclado.
static void TextosCompraTrofeoHud(
    const Participante& participante,
    const char*& comprar,
    const char*& noComprar
)
{
    switch (participante.control)
    {
        case CONTROL_GAMEPAD:
            comprar = "A";
            noComprar = "B";
            return;

        case CONTROL_TECLADO_FLECHAS:
            comprar = "ENTER / Y";
            noComprar = "SHIFT DER / N / ESC";
            return;

        case CONTROL_TECLADO_WASD:
            comprar = "ESPACIO / Y";
            noComprar = "RETROCESO / N / ESC";
            return;

        case CONTROL_TECLADO_COMPLETO:
            comprar = "ESPACIO / ENTER / Y";
            noComprar = "RETROCESO / N / ESC";
            return;

        case CONTROL_NINGUNO:
            break;
    }

    comprar = "";
    noComprar = "";
}


// ESC siempre pausa; START de un mando tambien (SolicitaPausaTablero).
static const char* TextoPausaHud(const PartidaTablero& partida)
{
    for (int i = 0; i < partida.cantidadJugadores; i++)
    {
        const Participante& participante =
            partida.participantes[partida.ordenParticipantes[i]];

        if (!participante.esBot && participante.control == CONTROL_GAMEPAD)
        {
            return "ESC / START: PAUSA Y SALIR";
        }
    }

    return "ESC: PAUSA Y SALIR";
}


// Nombre corto del control para la etiqueta del jugador.
static const char* NombreControlHud(const Participante& participante)
{
    if (participante.esBot)
    {
        return "BOT";
    }

    switch (participante.control)
    {
        case CONTROL_TECLADO_COMPLETO:
            return "TECLADO";

        case CONTROL_TECLADO_WASD:
            return "WASD";

        case CONTROL_TECLADO_FLECHAS:
            return "FLECHAS";

        case CONTROL_GAMEPAD:
            return TextFormat("MANDO %d", participante.indiceGamepad + 1);

        case CONTROL_NINGUNO:
            break;
    }

    return "";
}


// Humano que decide en esta fase (no bot y con su control conectado).
static bool HumanoDecideHud(const PartidaTablero& partida, int turno)
{
    return ParticipantePuedeDecidirTablero(partida, turno);
}


//==================================================
// TEXTOS DE FASE
//==================================================

// Texto del evento de la casilla (despues de resolverse). Vacio si la
// fase no es la del evento o aun no se resolvio.
static void TextoEventoHud(
    const PartidaTablero& partida,
    int turno,
    char* destino,
    int capacidad
)
{
    destino[0] = '\0';

    if (
        partida.fase != FASE_PARTIDA_TABLERO_EVENTO_CASILLA ||
        partida.tiempoFase <= 0.0f
    )
    {
        return;
    }

    // Evento resuelto por el gimmick del tablero.
    if (partida.textoEvento[0] != '\0')
    {
        std::snprintf(destino, capacidad, "%s", partida.textoEvento);
        return;
    }

    const Casilla* casilla =
        partida.tablero.ObtenerCasilla(partida.jugadores[turno].casillaActual);

    if (casilla == nullptr)
    {
        return;
    }

    if (casilla->tipo == CASILLA_POSITIVA)
    {
        std::snprintf(destino, capacidad, "CASILLA VERDE: %+d MONEDAS", partida.variacionMonedasEvento);
    }
    else if (casilla->tipo == CASILLA_NEGATIVA)
    {
        std::snprintf(destino, capacidad, "CASILLA ROJA: %d MONEDAS", partida.variacionMonedasEvento);
    }
    else if (casilla->tipo == CASILLA_ESPECIAL)
    {
        if (partida.ultimoEventoEspecial == EVENTO_TABLERO_BONIFICACION)
        {
            std::snprintf(destino, capacidad, "EVENTO: BONIFICACIÓN DE %+d MONEDAS", partida.variacionMonedasEvento);
        }
        else if (partida.ultimoEventoEspecial == EVENTO_TABLERO_MULTA)
        {
            std::snprintf(destino, capacidad, "EVENTO: MULTA DE %d MONEDAS", partida.variacionMonedasEvento);
        }
        else if (partida.ultimoEventoEspecial == EVENTO_TABLERO_INTERCAMBIO)
        {
            std::snprintf(
                destino, capacidad,
                "EVENTO: J%d Y J%d CAMBIAN SUS MONEDAS",
                turno + 1,
                partida.jugadorIntercambioEvento + 1
            );
        }
        else
        {
            std::snprintf(destino, capacidad, "EVENTO ESPECIAL");
        }
    }
    else if (casilla->tipo == CASILLA_NEUTRA)
    {
        std::snprintf(destino, capacidad, "CASILLA AZUL: SIN CAMBIOS");
    }
    else
    {
        std::snprintf(destino, capacidad, "ESTA CASILLA NO CAMBIA NADA");
    }
}


// Segunda linea del cartel del turno: que esta pasando.
static void TextoFaseHud(
    const PartidaTablero& partida,
    int turno,
    char* destino,
    int capacidad
)
{
    const Participante& participante = partida.participantes[turno];
    bool humano = !participante.esBot;
    bool sinMando = humano && !participante.conectado;
    int segundosBot = (int)std::ceil(std::fmax(0.0f, 4.0f - partida.tiempoFase));

    // Humano sin mando en una fase que pide decidir: avisa del bot.
    bool fasePideDecidir =
        partida.fase == FASE_PARTIDA_TABLERO_ESPERANDO_DADO ||
        partida.fase == FASE_PARTIDA_TABLERO_MENU_OBJETOS ||
        partida.fase == FASE_PARTIDA_TABLERO_DUELO_RESULTADO ||
        partida.fase == FASE_PARTIDA_TABLERO_ELIGIENDO_RUTA ||
        (
            partida.fase == FASE_PARTIDA_TABLERO_DECISION_TROFEO &&
            partida.compraTrofeoDisponible
        ) ||
        (
            partida.fase == FASE_PARTIDA_TABLERO_EVENTO_INTERACTIVO &&
            PedidoDelEventoInteractivo(partida) != PEDIDO_EVENTO_NADA
        );

    if (sinMando && fasePideDecidir)
    {
        if (partida.tiempoFase < 4.0f)
        {
            std::snprintf(destino, capacidad, "RECONECTÁ TU MANDO: SI NO, JUEGA UN BOT EN %d s", segundosBot);
        }
        else
        {
            std::snprintf(destino, capacidad, "SIN MANDO: JUEGA UN BOT POR VOS");
        }

        return;
    }

    switch (partida.fase)
    {
        case FASE_PARTIDA_TABLERO_ESPERANDO_DADO:
            if (partida.jugadores[turno].botasActivas || partida.jugadores[turno].dadoDobleActivo)
            {
                std::snprintf(
                    destino, capacidad, "%s: %s%s%s",
                    humano ? "TIRÁ EL DADO" : "EL BOT TIRA EL DADO...",
                    partida.jugadores[turno].botasActivas ? "BOTAS +3" : "",
                    partida.jugadores[turno].botasActivas && partida.jugadores[turno].dadoDobleActivo ? " Y " : "",
                    partida.jugadores[turno].dadoDobleActivo ? "DADO DOBLE" : ""
                );
            }
            else
            {
                std::snprintf(destino, capacidad, "%s", humano ? "TIRÁ EL DADO" : "EL BOT TIRA EL DADO...");
            }
            return;

        case FASE_PARTIDA_TABLERO_MENU_OBJETOS:
            TextoMenuObjetos(partida, destino, capacidad);
            return;

        case FASE_PARTIDA_TABLERO_DUELO_PENDIENTE:
        case FASE_PARTIDA_TABLERO_DUELO_RESULTADO:
            TextoDuelo(partida, destino, capacidad);
            return;

        case FASE_PARTIDA_TABLERO_MOSTRANDO_DADO:
            if (partida.detalleDado[0] != '\0')
            {
                std::snprintf(destino, capacidad, humano ? "¡SACASTE %d! (%s)" : "¡SACÓ %d! (%s)", partida.valorDado, partida.detalleDado);
            }
            else
            {
                std::snprintf(destino, capacidad, humano ? "¡SACASTE %d!" : "¡SACÓ %d!", partida.valorDado);
            }
            return;

        case FASE_PARTIDA_TABLERO_ELIGIENDO_RUTA:
            // El humano ve el selector de ruta en su lugar.
            std::snprintf(destino, capacidad, "%s", humano ? "ELEGI POR DÓNDE SEGUIR" : "EL BOT ELIGE EL CAMINO...");
            return;

        case FASE_PARTIDA_TABLERO_MOVIENDO:
            if (partida.pasosPendientes == 1)
            {
                std::snprintf(destino, capacidad, "AVANZA: QUEDA 1 PASO");
            }
            else
            {
                std::snprintf(destino, capacidad, "AVANZA: QUEDAN %d PASOS", partida.pasosPendientes);
            }
            return;

        case FASE_PARTIDA_TABLERO_DECISION_TROFEO:
            std::snprintf(destino, capacidad, "¡LLEGÓ AL TROFEO!");
            return;

        case FASE_PARTIDA_TABLERO_EVENTO_CASILLA:
            TextoEventoHud(partida, turno, destino, capacidad);
            return;

        case FASE_PARTIDA_TABLERO_EVENTO_INTERACTIVO:
            TextoEventoInteractivo(partida, destino, capacidad);
            return;

        case FASE_PARTIDA_TABLERO_FIN_TURNO:
            std::snprintf(destino, capacidad, "FIN DEL TURNO");
            return;

        case FASE_PARTIDA_TABLERO_FIN_RONDA:
            std::snprintf(destino, capacidad, "¡SE VIENE EL MINIJUEGO!");
            return;

        default:
            break;
    }

    destino[0] = '\0';
}


// Ayuda de controles de la fase (vacia si no hay nada que pulsar).
static void TextoAyudaHud(
    const PartidaTablero& partida,
    int turno,
    char* destino,
    int capacidad
)
{
    const Participante& participante = partida.participantes[turno];
    const char* pausa = TextoPausaHud(partida);
    bool decide = HumanoDecideHud(partida, turno);

    if (partida.fase == FASE_PARTIDA_TABLERO_DECISION_TROFEO)
    {
        // En la compra no hay pausa: ESC responde «no comprar».
        destino[0] = '\0';

        if (decide && partida.compraTrofeoDisponible)
        {
            const char* comprar = "";
            const char* noComprar = "";
            TextosCompraTrofeoHud(participante, comprar, noComprar);

            std::snprintf(destino, capacidad, "%s: COMPRAR   |   %s: NO COMPRAR", comprar, noComprar);
        }

        return;
    }

    if (partida.fase == FASE_PARTIDA_TABLERO_ESPERANDO_DADO && decide)
    {
        if (HayObjetoUsableAhora(partida, turno))
        {
            std::snprintf(
                destino, capacidad, "%s: TIRAR EL DADO   |   %s: OBJETOS   |   %s",
                TextoConfirmarHud(participante), TextoCancelarHud(participante), pausa
            );
        }
        else
        {
            std::snprintf(destino, capacidad, "%s: TIRAR EL DADO   |   %s", TextoConfirmarHud(participante), pausa);
        }

        return;
    }

    if (partida.fase == FASE_PARTIDA_TABLERO_MENU_OBJETOS && decide)
    {
        std::snprintf(
            destino, capacidad, "%s: ELEGIR   |   %s: USAR   |   %s: CERRAR",
            TextoElegirHud(participante), TextoConfirmarHud(participante), TextoCancelarHud(participante)
        );
        return;
    }

    if (
        partida.fase == FASE_PARTIDA_TABLERO_EVENTO_INTERACTIVO &&
        decide &&
        PedidoDelEventoInteractivo(partida) == PEDIDO_EVENTO_DUELO
    )
    {
        std::snprintf(
            destino, capacidad, "%s: ELEGIR   |   %s: CONFIRMAR   |   %s: NO DESAFIAR",
            TextoElegirHud(participante), TextoConfirmarHud(participante), TextoCancelarHud(participante)
        );
        return;
    }

    if (partida.fase == FASE_PARTIDA_TABLERO_DUELO_RESULTADO && decide)
    {
        std::snprintf(destino, capacidad, "%s: CONTINUAR   |   %s", TextoConfirmarHud(participante), pausa);
        return;
    }

    if (
        partida.fase == FASE_PARTIDA_TABLERO_EVENTO_INTERACTIVO &&
        decide &&
        PedidoDelEventoInteractivo(partida) == PEDIDO_EVENTO_TIENDA
    )
    {
        std::snprintf(
            destino, capacidad, "%s: ELEGIR   |   %s: COMPRAR O SALIR   |   %s: SALIR",
            TextoElegirHud(participante), TextoConfirmarHud(participante), TextoCancelarHud(participante)
        );
        return;
    }

    if (partida.fase == FASE_PARTIDA_TABLERO_ELIGIENDO_RUTA && decide)
    {
        std::snprintf(
            destino, capacidad,
            "%s: CAMBIAR DE RUTA   |   %s: SEGUIR POR ESA   |   %s",
            TextoElegirHud(participante),
            TextoConfirmarHud(participante),
            pausa
        );
        return;
    }

    if (partida.fase == FASE_PARTIDA_TABLERO_EVENTO_INTERACTIVO && decide)
    {
        switch (PedidoDelEventoInteractivo(partida))
        {
            case PEDIDO_EVENTO_GIRAR:
                std::snprintf(destino, capacidad, "%s: GIRAR LA RULETA   |   %s", TextoConfirmarHud(participante), pausa);
                return;

            case PEDIDO_EVENTO_DADOS:
                std::snprintf(destino, capacidad, "%s: TIRAR LOS DADOS   |   %s", TextoConfirmarHud(participante), pausa);
                return;

            case PEDIDO_EVENTO_ELEGIR_OBJETO:
                std::snprintf(
                    destino, capacidad,
                    "%s: ELEGIR OBJETO   |   %s: ATRAPAR   |   %s",
                    TextoElegirHud(participante),
                    TextoConfirmarHud(participante),
                    pausa
                );
                return;

            case PEDIDO_EVENTO_CONTINUAR:
                std::snprintf(destino, capacidad, "%s: CONTINUAR   |   %s", TextoConfirmarHud(participante), pausa);
                return;

            case PEDIDO_EVENTO_DUELO:
            case PEDIDO_EVENTO_TIENDA:
            case PEDIDO_EVENTO_NADA:
                break;
        }
    }

    std::snprintf(destino, capacidad, "%s", pausa);
}


//==================================================
// TENDEDERO: ETIQUETAS DE JUGADORES Y CARTEL DE RONDA
//==================================================

static void DibujarEtiquetaJugadorHud(
    const EstiloUI& estilo,
    const PartidaTablero& partida,
    int indice,
    float xRef,
    float ySogaRef,
    bool activo
)
{
    const EstadoHudTablero& e = estadoHudTablero;
    const Participante& participante = partida.participantes[indice];
    const EstadoJugadorPartidaTablero& jugador = partida.jugadores[indice];
    bool reducir = estilo.reducirMovimiento;

    float ySoga = YArribaHud(ySogaRef);
    float bajada = e.bajada[indice].valor;

    Rectangle rect = {
        XHud(xRef),
        YArribaHud(ySogaRef + 8.0f + bajada),
        Px(ANCHO_ETIQUETA_HUD),
        Px(ALTO_ETIQUETA_HUD)
    };

    float pinzaIzq = rect.x + Px(22.0f);
    float pinzaDer = rect.x + rect.width - Px(22.0f);
    Vector2 pivote = { rect.x + rect.width * 0.5f, ySoga };

    // Salto corto cuando cambian las monedas o los trofeos.
    float tCambio = std::fmin(e.tiempoMonedas[indice], e.tiempoTrofeo[indice]);

    if (!reducir && tCambio < T_SALTO_HUD)
    {
        rect.y -= Px(7.0f) * std::sin(PI_TABLERO_FINAL * tCambio / T_SALTO_HUD);
    }

    // Si cuelga mas abajo (tiene el turno), dos tramos de soga hasta la
    // etiqueta. Se dibujan antes, por detras.
    if (bajada > 1.0f)
    {
        DibujarCuerdaUI(estilo, Vector2{ pinzaIzq, ySoga }, Vector2{ pinzaIzq, rect.y + Px(4.0f) });
        DibujarCuerdaUI(estilo, Vector2{ pinzaDer, ySoga }, Vector2{ pinzaDer, rect.y + Px(4.0f) });
    }

    rlPushMatrix();
    rlTranslatef(pivote.x, pivote.y, 0.0f);
    rlRotatef(e.colgado[indice].angulo, 0.0f, 0.0f, 1.0f);
    rlTranslatef(-pivote.x, -pivote.y, 0.0f);

    Rectangle placa = DibujarPlacaConMarcoUI(estilo, rect, 6.0f);
    (void)placa;

    DibujarBanderinColganteUI(Vector2{ rect.x + Px(12.0f), rect.y + Px(11.0f) }, participante.color, 20.0f, 36.0f);

    // Linea 1: J1 y el control (o BOT).
    float xTexto = rect.x + Px(40.0f);
    float derecha = rect.x + rect.width - Px(12.0f);
    float yLinea1 = rect.y + Px(9.0f);

    const char* nombre = TextFormat("J%d", indice + 1);
    int tamanoNombre = TamanoTexto(UI_TEXTO);
    DrawText(nombre, (int)xTexto, (int)yLinea1, tamanoNombre, UI_TINTA);

    float xControl = xTexto + MeasureText(nombre, tamanoNombre) + Px(7.0f);
    bool sinMando = !participante.esBot && !participante.conectado;

    DibujarTextoAjustadoUI(
        sinMando ? "SIN MANDO" : NombreControlHud(participante),
        xControl,
        yLinea1 + Px(3.0f),
        UI_TEXTO_PEQUENO,
        derecha - xControl,
        sinMando ? COLOR_TINTA_ALERTA_HUD : UI_TINTA_SUAVE,
        false
    );

    // Linea 2: trofeos y monedas.
    float yIconos = rect.y + Px(46.0f);
    float yValores = yIconos - TamanoTexto(UI_TEXTO) * 0.5f + Px(1.0f);
    float xTrofeo = rect.x + Px(50.0f);
    float xMoneda = rect.x + Px(100.0f);

    // Destello detras del valor que cambio.
    if (e.tiempoTrofeo[indice] < T_DESTELLO_HUD)
    {
        float k = 1.0f - e.tiempoTrofeo[indice] / T_DESTELLO_HUD;
        DrawRectangleRec(
            Rectangle{ xTrofeo - Px(12.0f), yIconos - Px(13.0f), Px(44.0f), Px(26.0f) },
            Fade(UI_FOCO, 0.75f * k)
        );
    }

    if (e.tiempoMonedas[indice] < T_DESTELLO_HUD)
    {
        float k = 1.0f - e.tiempoMonedas[indice] / T_DESTELLO_HUD;
        DrawRectangleRec(
            Rectangle{ xMoneda - Px(12.0f), yIconos - Px(13.0f), derecha - xMoneda + Px(16.0f), Px(26.0f) },
            Fade(UI_FOCO, 0.75f * k)
        );
    }

    DibujarTrofeoUI(Vector2{ xTrofeo, yIconos }, 18.0f);
    DrawText(
        TextFormat("%d", jugador.trofeos),
        (int)(xTrofeo + Px(13.0f)), (int)yValores, TamanoTexto(UI_TEXTO), UI_TINTA
    );

    DibujarMonedaUI(Vector2{ xMoneda, yIconos }, 8.0f);
    DrawText(
        TextFormat("%d", jugador.monedas),
        (int)(xMoneda + Px(14.0f)), (int)yValores, TamanoTexto(UI_TEXTO), UI_TINTA
    );

    // Cuanto cambio: «+3» / «-3» a la derecha, un rato.
    int delta = e.deltaMonedas[indice];

    if (e.tiempoMonedas[indice] < T_DELTA_MONEDAS && delta != 0)
    {
        DibujarTextoDerechaUI(
            TextFormat("%+d", delta),
            derecha,
            yIconos - TamanoTexto(UI_TEXTO_PEQUENO) * 0.5f + Px(1.0f),
            UI_TEXTO_PEQUENO,
            Px(40.0f),
            delta > 0 ? COLOR_TINTA_POSITIVA_HUD : COLOR_TINTA_ALERTA_HUD
        );
    }

    if (activo)
    {
        DibujarMarcoFoco(rect);
    }

    rlPopMatrix();

    DibujarPinzaUI(Vector2{ pinzaIzq, ySoga });
    DibujarPinzaUI(Vector2{ pinzaDer, ySoga });
}


static void DibujarCartelRondaHud(
    const EstiloUI& estilo,
    const PartidaTablero& partida,
    float xRef,
    float ySogaRef
)
{
    Rectangle rect = {
        XHud(xRef),
        YArribaHud(ySogaRef + 8.0f),
        Px(ANCHO_RONDA_HUD),
        Px(ALTO_RONDA_HUD)
    };

    bool ultima =
        partida.rondaActual >= partida.cantidadRondas &&
        partida.fase != FASE_PARTIDA_TABLERO_TERMINADA;

    DrawRectangleRec(
        Rectangle{ rect.x + Px(4.0f), rect.y + Px(5.0f), rect.width, rect.height },
        Fade(BLACK, 0.28f)
    );
    DibujarNueveCortes(estilo.cartel, 10, rect, EscalaUI(), WHITE);

    Rectangle cara = {
        rect.x + Px(9.0f), rect.y + Px(9.0f),
        rect.width - Px(18.0f), rect.height - Px(18.0f)
    };

    if (ultima)
    {
        DrawRectangleRec(cara, Fade(COLOR_PLACA_ALERTA_HUD, 0.85f));
    }

    float cx = rect.x + rect.width * 0.5f;
    Color tinta = ultima ? COLOR_TINTA_ALERTA_HUD : UI_TINTA;

    DibujarTextoAjustadoUI(
        ultima ? "ÚLTIMA RONDA" : "RONDA",
        cx, rect.y + Px(12.0f), UI_TEXTO_PEQUENO - 2,
        cara.width - Px(6.0f), ultima ? COLOR_TINTA_ALERTA_HUD : UI_TINTA_SUAVE, true
    );

    DibujarTextoAjustadoUI(
        TextFormat("%d/%d", partida.rondaActual, partida.cantidadRondas),
        cx, rect.y + Px(27.0f), UI_TEXTO_GRANDE,
        cara.width - Px(6.0f), tinta, true
    );

    // Progreso: un punto por ronda (o una barra si son muchas).
    float x0 = rect.x + Px(18.0f);
    float x1 = rect.x + rect.width - Px(18.0f);
    float y = rect.y + rect.height - Px(16.0f);
    int total = partida.cantidadRondas > 0 ? partida.cantidadRondas : 1;

    if (total <= 10)
    {
        float paso = total > 1 ? (x1 - x0) / (total - 1) : 0.0f;

        for (int r = 0; r < total; r++)
        {
            float x = total > 1 ? x0 + paso * r : (x0 + x1) * 0.5f;
            bool jugada = r < partida.rondaActual;

            DrawCircleV(Vector2{ x, y }, Px(4.0f), UI_TINTA);
            DrawCircleV(
                Vector2{ x, y }, Px(2.8f),
                jugada ? (ultima ? Color{ 226, 104, 46, 255 } : UI_FOCO) : UI_PLACA_CLARA
            );
        }
    }
    else
    {
        Rectangle riel = { x0, y - Px(3.0f), x1 - x0, Px(6.0f) };
        DrawRectangleRec(riel, Fade(UI_MADERA_OSCURA, 0.30f));
        riel.width *= Limitar01Hud((float)partida.rondaActual / total);
        DrawRectangleRec(riel, UI_FOCO);
    }

    float ySoga = YArribaHud(ySogaRef);
    DibujarPinzaUI(Vector2{ rect.x + Px(26.0f), ySoga });
    DibujarPinzaUI(Vector2{ rect.x + rect.width - Px(26.0f), ySoga });
}


// Dos sogas desde un cartel de arriba hasta otro de abajo (pixeles).
static void DibujarSogasColgantesHud(const EstiloUI& estilo, float cx, float separacion, float yArriba, float yAbajo)
{
    if (yAbajo <= yArriba)
    {
        return;
    }

    DibujarCuerdaUI(estilo, Vector2{ cx - separacion, yArriba }, Vector2{ cx - separacion, yAbajo + Px(4.0f) });
    DibujarCuerdaUI(estilo, Vector2{ cx + separacion, yArriba }, Vector2{ cx + separacion, yAbajo + Px(4.0f) });
}


// Debajo del cartel de ronda: tablero, zona y estado del gimmick; o el
// aviso del gimmick mientras dura.
static void DibujarTiraTableroHud(
    const EstiloUI& estilo,
    const PartidaTablero& partida,
    float cxRef,
    float yArribaRef
)
{
    const EstadoHudTablero& e = estadoHudTablero;
    float cx = XHud(cxRef);
    float yArriba = YArribaHud(yArribaRef);

    // Aviso del gimmick: cartel mas grande que cae, y sube al terminar.
    if (partida.tiempoMensajeGimmick > 0.0f && partida.mensajeGimmick[0] != '\0')
    {
        float subida = 0.0f;

        if (!estilo.reducirMovimiento && partida.tiempoMensajeGimmick < 0.3f)
        {
            float k = 1.0f - partida.tiempoMensajeGimmick / 0.3f;
            subida = 90.0f * k * k;
        }

        const float ancho = 470.0f;
        float anchoTexto = Px(ancho - 40.0f);
        MedidaParrafoUI medida = MedirParrafoUI(partida.mensajeGimmick, UI_TEXTO, anchoTexto, Px(52.0f));
        float alto = 26.0f + medida.alto / EscalaUI();

        Rectangle rect = {
            XHud(cxRef - ancho * 0.5f),
            YArribaHud(yArribaRef + 10.0f + e.caidaMensaje.valor - subida),
            Px(ancho),
            Px(alto)
        };

        DibujarSogasColgantesHud(estilo, cx, Px(40.0f), yArriba - Px(4.0f), rect.y);
        Rectangle placa = DibujarPlacaConMarcoUI(estilo, rect, 7.0f);

        DibujarParrafoUI(
            partida.mensajeGimmick,
            placa.x + Px(13.0f),
            placa.y + (placa.height - medida.alto) * 0.5f,
            UI_TEXTO,
            anchoTexto,
            UI_TINTA,
            UI_ALINEAR_CENTRO,
            Px(52.0f)
        );

        return;
    }

    // Tira fija: tablero y zona del jugador activo + estado del gimmick.
    char lineaTablero[128] = "";
    std::snprintf(lineaTablero, sizeof(lineaTablero), "%s", partida.definicion->nombre);

    int turno = ObtenerParticipanteTurnoTableroFinal(partida);

    if (turno >= 0 && partida.definicion->nombresZonas != nullptr)
    {
        const Casilla* casilla =
            partida.tablero.ObtenerCasilla(partida.jugadores[turno].casillaActual);

        if (
            casilla != nullptr &&
            casilla->zona >= 0 &&
            casilla->zona < partida.definicion->cantidadZonas
        )
        {
            std::snprintf(
                lineaTablero, sizeof(lineaTablero), "%s - %s",
                partida.definicion->nombre,
                partida.definicion->nombresZonas[casilla->zona]
            );
        }
    }

    bool conGimmick = partida.estadoGimmick[0] != '\0';
    int tamano = TamanoTexto(UI_TEXTO_PEQUENO);
    float anchoTexto = (float)MeasureText(lineaTablero, tamano);

    if (conGimmick)
    {
        anchoTexto = std::fmax(anchoTexto, (float)MeasureText(partida.estadoGimmick, tamano));
    }

    float ancho = anchoTexto / EscalaUI() + 28.0f;
    if (ancho < 260.0f) ancho = 260.0f;
    if (ancho > 560.0f) ancho = 560.0f;

    float alto = conGimmick ? 48.0f : 28.0f;

    Rectangle rect = {
        XHud(cxRef - ancho * 0.5f),
        YArribaHud(yArribaRef + 10.0f),
        Px(ancho),
        Px(alto)
    };

    DibujarSogasColgantesHud(estilo, cx, Px(40.0f), yArriba - Px(4.0f), rect.y);

    DrawRectangleRec(
        Rectangle{ rect.x + Px(2.0f), rect.y + Px(3.0f), rect.width, rect.height },
        Fade(BLACK, 0.22f)
    );
    DibujarNueveCortes(estilo.placa, 8, rect, EscalaUI(), WHITE);
    DrawRectangleLinesEx(rect, GrosorLineaHud(1.0f), Fade(UI_MADERA_OSCURA, 0.6f));

    float anchoUtil = rect.width - Px(16.0f);

    DibujarTextoAjustadoUI(
        lineaTablero, cx, rect.y + Px(6.0f), UI_TEXTO_PEQUENO,
        anchoUtil, UI_TINTA_SUAVE, true
    );

    if (conGimmick)
    {
        DibujarTextoAjustadoUI(
            partida.estadoGimmick, cx, rect.y + Px(26.0f), UI_TEXTO_PEQUENO,
            anchoUtil, UI_TINTA, true
        );
    }
}


static void DibujarTendederoHud(
    const EstiloUI& estilo,
    const PartidaTablero& partida
)
{
    int cantidad = partida.cantidadJugadores;

    if (cantidad <= 0)
    {
        return;
    }

    // Orden de turno: la primera mitad a la izquierda del cartel de ronda.
    int cantidadIzq = (cantidad + 1) / 2;
    int cantidadDer = cantidad - cantidadIzq;

    float anchoIzq = cantidadIzq * ANCHO_ETIQUETA_HUD + (cantidadIzq - 1) * SEPARACION_ETIQUETAS_HUD;
    float anchoDer = cantidadDer > 0 ? cantidadDer * ANCHO_ETIQUETA_HUD + (cantidadDer - 1) * SEPARACION_ETIQUETAS_HUD : 0.0f;
    float total = anchoIzq + SEPARACION_RONDA_HUD + ANCHO_RONDA_HUD + SEPARACION_RONDA_HUD + anchoDer;
    float inicio = UI_REF_ANCHO * 0.5f - total * 0.5f;
    float xRonda = inicio + anchoIzq + SEPARACION_RONDA_HUD;
    float cxRonda = xRonda + ANCHO_RONDA_HUD * 0.5f;

    float sogaInicio = inicio - SOGA_SOBRANTE_HUD;
    float sogaFin = inicio + total + SOGA_SOBRANTE_HUD;

    // 1) Lo que cuelga del cartel de ronda (pasa por detras de la fila).
    float ySogaRonda = AlturaSogaHud(cxRonda, sogaInicio, sogaFin);
    DibujarTiraTableroHud(estilo, partida, cxRonda, ySogaRonda + 8.0f + ALTO_RONDA_HUD - 4.0f);

    // 2) Soga: dos tramos verticales desde el borde y la comba.
    DibujarCuerdaUI(estilo, Vector2{ XHud(sogaInicio), 0.0f }, Vector2{ XHud(sogaInicio), YArribaHud(Y_SOGA_HUD) });
    DibujarCuerdaUI(estilo, Vector2{ XHud(sogaFin), 0.0f }, Vector2{ XHud(sogaFin), YArribaHud(Y_SOGA_HUD) });

    const int tramos = 12;

    for (int k = 0; k < tramos; k++)
    {
        float xa = sogaInicio + (sogaFin - sogaInicio) * k / tramos;
        float xb = sogaInicio + (sogaFin - sogaInicio) * (k + 1) / tramos;

        DibujarCuerdaUI(
            estilo,
            Vector2{ XHud(xa), YArribaHud(AlturaSogaHud(xa, sogaInicio, sogaFin)) },
            Vector2{ XHud(xb), YArribaHud(AlturaSogaHud(xb, sogaInicio, sogaFin)) }
        );
    }

    DrawCircleV(Vector2{ XHud(sogaInicio), YArribaHud(Y_SOGA_HUD) }, Px(4.5f), UI_MADERA_OSCURA);
    DrawCircleV(Vector2{ XHud(sogaFin), YArribaHud(Y_SOGA_HUD) }, Px(4.5f), UI_MADERA_OSCURA);

    // 3) Etiquetas y cartel de ronda.
    int turno = TurnoVisibleHud(partida);

    for (int k = 0; k < cantidad; k++)
    {
        int indice = partida.ordenParticipantes[k];
        float x = k < cantidadIzq
            ? inicio + k * (ANCHO_ETIQUETA_HUD + SEPARACION_ETIQUETAS_HUD)
            : xRonda + ANCHO_RONDA_HUD + SEPARACION_RONDA_HUD +
              (k - cantidadIzq) * (ANCHO_ETIQUETA_HUD + SEPARACION_ETIQUETAS_HUD);
        float centro = x + ANCHO_ETIQUETA_HUD * 0.5f;

        DibujarEtiquetaJugadorHud(
            estilo, partida, indice, x,
            AlturaSogaHud(centro, sogaInicio, sogaFin),
            indice == turno
        );
    }

    DibujarCartelRondaHud(estilo, partida, xRonda, ySogaRonda);
}


//==================================================
// DADO SOBRE LA FICHA Y ETIQUETAS DE RUTA
//==================================================

// Punto de pantalla sobre la ficha del jugador del turno.
static Vector2 PuntoSobreFichaHud(const PartidaTablero& partida, int turno, float alturaMundo)
{
    Vector3 p = partida.jugadores[turno].posicionVisual;
    p.y += alturaMundo;

    return GetWorldToScreen(p, partida.camara);
}


static void DibujarDadoHud(
    const EstiloUI& estilo,
    const PartidaTablero& partida
)
{
    const EstadoHudTablero& e = estadoHudTablero;
    int valor = e.valorDado;
    int turno = ObtenerParticipanteTurnoTableroFinal(partida);

    if (valor < 0 || turno < 0)
    {
        return;
    }

    bool reducir = estilo.reducirMovimiento;
    bool girando = valor == 0;
    bool resultado = partida.fase == FASE_PARTIDA_TABLERO_MOSTRANDO_DADO;
    float t = estilo.tiempo;

    // Encima de la flecha amarilla que marca la ficha activa.
    Vector2 base = PuntoSobreFichaHud(partida, turno, 2.2f);
    float lado = Px(LADO_DADO_HUD);

    float escala = 1.0f;
    float angulo = 0.0f;

    if (girando)
    {
        if (!reducir)
        {
            base.y -= Px(4.0f) * std::sin(t * 4.0f);
            angulo = 7.0f * std::sin(t * 9.0f);
        }
    }
    else if (resultado)
    {
        escala = EscalaReboteHud(e.tiempoDado, 0.30f, 0.55f, reducir);
    }
    else
    {
        escala = EscalaReboteHud(e.tiempoDado, 0.20f, 0.22f, reducir);
    }

    lado *= escala;

    // Dentro de la pantalla, libre del tendedero y del cartel de abajo.
    float ancho = (float)GetScreenWidth();
    float cx = std::fmax(Px(60.0f), std::fmin(ancho - Px(60.0f), base.x));
    float cy = base.y - Px(LADO_DADO_HUD) * 0.5f - Px(6.0f);
    cy = std::fmax(YArribaHud(150.0f), std::fmin(YAbajoHud(560.0f), cy));

    char texto[8] = "";

    if (girando)
    {
        // Caras al azar mientras se espera; quieto, un «?».
        if (reducir)
        {
            std::snprintf(texto, sizeof(texto), "?");
        }
        else
        {
            std::snprintf(texto, sizeof(texto), "%d", 1 + (int)(t * 12.0f) % 10);
        }
    }
    else
    {
        std::snprintf(texto, sizeof(texto), "%d", valor);
    }

    rlPushMatrix();
    rlTranslatef(cx, cy, 0.0f);
    rlRotatef(angulo, 0.0f, 0.0f, 1.0f);

    float borde = GrosorLineaHud(3.0f);
    Rectangle cara = { -lado * 0.5f, -lado * 0.5f, lado, lado };

    DrawRectangleRounded(
        Rectangle{ cara.x - borde + Px(3.0f), cara.y - borde + Px(5.0f), cara.width + borde * 2.0f, cara.height + borde * 2.0f },
        0.24f, 6, Fade(BLACK, 0.30f)
    );
    DrawRectangleRounded(
        Rectangle{ cara.x - borde, cara.y - borde, cara.width + borde * 2.0f, cara.height + borde * 2.0f },
        0.24f, 6, UI_TINTA
    );
    DrawRectangleRounded(cara, 0.22f, 6, COLOR_CARA_DADO_HUD);

    // Canto inferior algo mas oscuro: volumen de dado.
    DrawRectangleRec(
        Rectangle{ cara.x + lado * 0.12f, cara.y + lado * 0.84f, lado * 0.76f, lado * 0.08f },
        Fade(UI_MADERA_MEDIA, 0.35f)
    );

    if (resultado)
    {
        float halo = Px(5.0f);
        DrawRectangleRoundedLinesEx(
            Rectangle{ cara.x - halo, cara.y - halo, cara.width + halo * 2.0f, cara.height + halo * 2.0f },
            0.26f, 6, Px(UI_BORDE_FOCO), UI_FOCO
        );
    }

    int tamano = (int)(TamanoTexto(UI_TEXTO_GRANDE) * escala);
    int anchoTexto = MeasureText(texto, tamano);
    DrawText(texto, -anchoTexto / 2, -tamano / 2, tamano, UI_TINTA);

    rlPopMatrix();

    // Mientras se espera la tirada de un humano: el boton, al costado.
    if (girando && HumanoDecideHud(partida, turno))
    {
        const char* boton = TextoConfirmarHud(partida.participantes[turno]);
        int tamanoBoton = TamanoTexto(UI_TEXTO_PEQUENO);
        float medio = (MeasureText(boton, tamanoBoton) + Px(21.0f)) * 0.5f;

        OpcionesEtiquetaUI opciones;
        opciones.borde = partida.participantes[turno].color;

        float xEtiqueta = cx + Px(LADO_DADO_HUD) * 0.5f + Px(12.0f) + medio;

        // Si no entra a la derecha, va a la izquierda.
        if (xEtiqueta + medio > ancho - Px(8.0f))
        {
            xEtiqueta = cx - Px(LADO_DADO_HUD) * 0.5f - Px(12.0f) - medio;
        }

        DibujarEtiquetaUI(boton, Vector2{ xEtiqueta, cy }, reducir ? 0.0f : -3.0f, opciones);
    }
}


static void DibujarEtiquetasRutaHud(
    const EstiloUI& estilo,
    const PartidaTablero& partida
)
{
    if (partida.fase != FASE_PARTIDA_TABLERO_ELIGIENDO_RUTA)
    {
        return;
    }

    int turno = ObtenerParticipanteTurnoTableroFinal(partida);

    if (turno < 0)
    {
        return;
    }

    const Casilla* origen =
        partida.tablero.ObtenerCasilla(partida.jugadores[turno].casillaActual);

    if (origen == nullptr || origen->cantidadConexiones <= 1)
    {
        return;
    }

    // Primero las no elegidas, para que la elegida quede encima.
    for (int pasada = 0; pasada < 2; pasada++)
    {
        for (int o = 0; o < origen->cantidadConexiones; o++)
        {
            bool elegida = o == partida.opcionRuta;

            if (elegida != (pasada == 1))
            {
                continue;
            }

            const Casilla* destino =
                partida.tablero.ObtenerCasilla(origen->conexiones[o].destino);

            if (destino == nullptr)
            {
                continue;
            }

            // Por encima de la flecha 3D de la ruta elegida.
            Vector3 punto = destino->posicion;
            punto.y += 2.3f;
            Vector2 pantalla = GetWorldToScreen(punto, partida.camara);

            if (elegida && !estilo.reducirMovimiento)
            {
                pantalla.y -= Px(4.0f) * std::sin(estilo.tiempo * 5.0f);
            }

            OpcionesEtiquetaUI opciones;
            opciones.pico = true;
            opciones.borde = elegida ? UI_FOCO : UI_MADERA_MEDIA;
            opciones.tokenTexto = elegida ? UI_TEXTO : UI_TEXTO_PEQUENO;
            opciones.alfa = elegida ? 1.0f : 0.85f;

            DibujarEtiquetaUI(
                TextFormat("RUTA %d", o + 1),
                pantalla,
                elegida ? 0.0f : (o == 0 ? -4.0f : 4.0f),
                opciones
            );
        }
    }
}


//==================================================
// CARTEL DEL TURNO (abajo) Y LEYENDA
//==================================================

// Sello del evento de la casilla: texto y color segun lo que paso.
static bool SelloEventoHud(const PartidaTablero& partida, char* texto, int capacidad, Color& color)
{
    if (
        partida.fase != FASE_PARTIDA_TABLERO_EVENTO_CASILLA ||
        partida.tiempoFase <= 0.0f
    )
    {
        return false;
    }

    int variacion = partida.variacionMonedasEvento;

    if (variacion == 0)
    {
        return false;
    }

    std::snprintf(texto, capacidad, "%+d", variacion);
    color = variacion > 0 ? UI_POSITIVO : UI_PELIGRO;

    return true;
}


static void DibujarCartelTurnoHud(
    const EstiloUI& estilo,
    const PartidaTablero& partida,
    int turno
)
{
    const Participante& participante = partida.participantes[turno];

    Rectangle rect = {
        XHud(UI_REF_ANCHO * 0.5f - ANCHO_CARTEL_TURNO * 0.5f),
        YAbajoHud(Y_CARTEL_TURNO),
        Px(ANCHO_CARTEL_TURNO),
        Px(ALTO_CARTEL_TURNO)
    };

    Rectangle placa = DibujarPlacaConMarcoUI(estilo, rect, 6.0f);
    (void)placa;

    bool finRonda = partida.fase == FASE_PARTIDA_TABLERO_FIN_RONDA;

    DibujarBanderinColganteUI(
        Vector2{ rect.x + Px(14.0f), rect.y + Px(10.0f) },
        finRonda ? UI_MADERA_MEDIA : participante.color,
        18.0f, 34.0f
    );

    float xTexto = rect.x + Px(44.0f);
    float derecha = rect.x + rect.width - Px(14.0f);

    // Sello del evento a la derecha (reserva su lugar).
    char textoSello[8] = "";
    Color colorSello = UI_POSITIVO;
    bool conSello = SelloEventoHud(partida, textoSello, (int)sizeof(textoSello), colorSello);

    if (conSello)
    {
        derecha -= Px(44.0f);
    }

    // Linea 1: de quien es el turno.
    const char* titulo =
        finRonda
        ? TextFormat("RONDA %d TERMINADA", partida.rondaActual)
        : TextFormat("TURNO DE J%d", turno + 1);

    int tamanoTitulo = TamanoTexto(UI_TEXTO);
    DrawText(titulo, (int)xTexto, (int)(rect.y + Px(9.0f)), tamanoTitulo, UI_TINTA);

    if (participante.esBot && !finRonda)
    {
        DrawText(
            "BOT",
            (int)(xTexto + MeasureText(titulo, tamanoTitulo) + Px(7.0f)),
            (int)(rect.y + Px(12.0f)),
            TamanoTexto(UI_TEXTO_PEQUENO),
            UI_TINTA_SUAVE
        );
    }

    // En la bifurcacion no hay dado sobre la ficha: los pasos van aca.
    if (
        partida.fase == FASE_PARTIDA_TABLERO_ELIGIENDO_RUTA &&
        partida.pasosPendientes > 0
    )
    {
        DibujarTextoDerechaUI(
            partida.pasosPendientes == 1 ? "QUEDA 1 PASO" : TextFormat("QUEDAN %d PASOS", partida.pasosPendientes),
            derecha,
            rect.y + Px(12.0f),
            UI_TEXTO_PEQUENO,
            Px(150.0f),
            UI_TINTA_SUAVE
        );
    }

    // Linea 2: que esta pasando. Al elegir ruta, el humano ve un selector
    // «< RUTA 1: ... >».
    float yLinea2 = rect.y + Px(32.0f);
    bool selector =
        partida.fase == FASE_PARTIDA_TABLERO_ELIGIENDO_RUTA &&
        HumanoDecideHud(partida, turno);

    if (selector)
    {
        const char* descripcion = nullptr;

        if (partida.definicion->DescribirOpcionRuta != nullptr)
        {
            descripcion =
                partida.definicion->DescribirOpcionRuta(
                    partida,
                    partida.jugadores[turno].casillaActual,
                    partida.opcionRuta
                );
        }

        char linea[128] = "";

        if (descripcion != nullptr)
        {
            std::snprintf(linea, sizeof(linea), "RUTA %d: %s", partida.opcionRuta + 1, descripcion);
        }
        else
        {
            std::snprintf(linea, sizeof(linea), "RUTA %d", partida.opcionRuta + 1);
        }

        float medio = Px(6.0f);
        float yFlecha = yLinea2 + TamanoTexto(UI_TEXTO_PEQUENO) * 0.5f;

        DibujarFlechaUI(xTexto + medio, yFlecha, medio, -1, UI_TINTA);
        DibujarFlechaUI(derecha - medio, yFlecha, medio, 1, UI_TINTA);

        float izquierda = xTexto + medio * 2.0f + Px(8.0f);
        float anchoLinea = derecha - medio * 2.0f - Px(8.0f) - izquierda;

        DibujarTextoAjustadoUI(
            linea, izquierda + anchoLinea * 0.5f, yLinea2, UI_TEXTO_PEQUENO,
            anchoLinea, UI_TINTA, true
        );
    }
    else
    {
        char linea[128] = "";
        TextoFaseHud(partida, turno, linea, (int)sizeof(linea));

        bool alerta =
            !participante.esBot &&
            !participante.conectado;

        DibujarTextoAjustadoUI(
            linea, xTexto, yLinea2, UI_TEXTO_PEQUENO,
            derecha - xTexto, alerta ? COLOR_TINTA_ALERTA_HUD : UI_TINTA_SUAVE, false
        );
    }

    if (conSello)
    {
        DibujarSelloUI(
            textoSello,
            Vector2{ rect.x + rect.width - Px(34.0f), rect.y + rect.height * 0.5f },
            22.0f,
            UI_SELLO_ESTRELLA,
            colorSello,
            -8.0f,
            1.0f
        );
    }
}


// Tablon de la compra del trofeo (reemplaza al cartel del turno).
static void DibujarTablonTrofeoHud(
    const EstiloUI& estilo,
    const PartidaTablero& partida,
    int turno
)
{
    const Participante& participante = partida.participantes[turno];
    const EstadoJugadorPartidaTablero& jugador = partida.jugadores[turno];
    bool humano = !participante.esBot;
    bool decide = HumanoDecideHud(partida, turno);
    bool alcanza = partida.compraTrofeoDisponible;

    // Entra desde abajo.
    float subida = 0.0f;

    if (!estilo.reducirMovimiento)
    {
        float resto = 1.0f - Limitar01Hud(partida.tiempoFase / T_ENTRADA_TABLON);
        subida = (ALTO_TABLON_TROFEO + 60.0f) * resto * resto * resto;
    }

    // Con botones (humano que decide), con linea de estado (bot o sin
    // mando) o solo titulo y precio (no le alcanza).
    bool conBotones = decide && alcanza;
    bool conEstado = alcanza && !decide;
    float alto =
        conBotones ? ALTO_TABLON_TROFEO :
        (conEstado ? ALTO_TABLON_TROFEO - 30.0f : ALTO_TABLON_TROFEO - 52.0f);

    Rectangle rect = {
        XHud(UI_REF_ANCHO * 0.5f - ANCHO_TABLON_TROFEO * 0.5f),
        YAbajoHud(Y_CARTEL_TURNO + ALTO_CARTEL_TURNO - alto + subida),
        Px(ANCHO_TABLON_TROFEO),
        Px(alto)
    };

    Rectangle placa = DibujarTablonHud(estilo, rect);

    // Copa grande a la izquierda (el alto se pasa en unidades de referencia).
    float xCopa = placa.x + Px(44.0f);
    DibujarTrofeoUI(
        Vector2{ xCopa, placa.y + std::fmin(Px(46.0f), placa.height * 0.5f) },
        std::fmin(Px(54.0f), placa.height - Px(16.0f)) / EscalaUI()
    );

    float xTexto = placa.x + Px(88.0f);
    float anchoTexto = placa.x + placa.width - Px(14.0f) - xTexto;

    // Titulo.
    char titulo[64] = "";

    if (alcanza)
    {
        std::snprintf(titulo, sizeof(titulo), humano ? "¿COMPRÁS EL TROFEO?" : "¿J%d COMPRA EL TROFEO?", turno + 1);
    }
    else
    {
        std::snprintf(titulo, sizeof(titulo), "%s", humano ? "NO TE ALCANZA PARA EL TROFEO" : "NO LE ALCANZA PARA EL TROFEO");
    }

    DibujarTextoAjustadoUI(titulo, xTexto, placa.y + Px(10.0f), UI_TEXTO_GRANDE - 4, anchoTexto, UI_TINTA, false);

    // Precio y monedas del jugador.
    float yValores = placa.y + Px(48.0f);
    int tamano = TamanoTexto(UI_TEXTO);
    float x = xTexto;

    DrawText("CUESTA", (int)x, (int)yValores, tamano, UI_TINTA_SUAVE);
    x += MeasureText("CUESTA", tamano) + Px(16.0f);
    DibujarMonedaUI(Vector2{ x, yValores + tamano * 0.5f }, 8.0f);
    x += Px(14.0f);
    const char* costo = TextFormat("%d", partida.costoTrofeo);
    DrawText(costo, (int)x, (int)yValores, tamano, UI_TINTA);
    x += MeasureText(costo, tamano) + Px(30.0f);

    if (alcanza)
    {
        const char* tiene = humano ? "TENÉS" : "TIENE";
        DrawText(tiene, (int)x, (int)yValores, tamano, UI_TINTA_SUAVE);
        x += MeasureText(tiene, tamano) + Px(16.0f);
        DibujarMonedaUI(Vector2{ x, yValores + tamano * 0.5f }, 8.0f);
        x += Px(14.0f);
        DrawText(TextFormat("%d", jugador.monedas), (int)x, (int)yValores, tamano, UI_TINTA);
    }
    else
    {
        const char* falta = TextFormat(
            "FALTAN %d",
            partida.costoTrofeo - jugador.monedas
        );
        DrawText(falta, (int)x, (int)yValores, tamano, COLOR_TINTA_ALERTA_HUD);
    }

    if (!conBotones)
    {
        // Bot pensando o humano sin mando: una linea de estado.
        if (conEstado)
        {
            char linea[96] = "";
            TextoFaseHud(partida, turno, linea, (int)sizeof(linea));

            if (participante.esBot)
            {
                std::snprintf(linea, sizeof(linea), "EL BOT DECIDE...");
            }

            DibujarTextoAjustadoUI(
                linea, xTexto, placa.y + Px(76.0f), UI_TEXTO_PEQUENO,
                anchoTexto, participante.esBot ? UI_TINTA_SUAVE : COLOR_TINTA_ALERTA_HUD, false
            );
        }

        return;
    }

    // Dos carteles con la accion y sus teclas (no hay foco: cada uno
    // tiene su propia tecla).
    const char* teclasComprar = "";
    const char* teclasNo = "";
    TextosCompraTrofeoHud(participante, teclasComprar, teclasNo);

    const char* acciones[2] = { "COMPRAR", "NO COMPRAR" };
    const char* teclas[2] = { teclasComprar, teclasNo };
    float anchoBoton = (placa.width - Px(88.0f) - Px(14.0f) - Px(12.0f)) * 0.5f;

    for (int b = 0; b < 2; b++)
    {
        Rectangle boton = {
            xTexto + b * (anchoBoton + Px(12.0f)),
            placa.y + placa.height - Px(54.0f),
            anchoBoton,
            Px(48.0f)
        };

        DrawRectangleRec(
            Rectangle{ boton.x + Px(3.0f), boton.y + Px(4.0f), boton.width, boton.height },
            Fade(BLACK, 0.22f)
        );
        DibujarNueveCortes(estilo.cartel, 10, boton, EscalaUI() * 0.6f, WHITE);

        if (b == 0)
        {
            DrawRectangleLinesEx(boton, GrosorLineaHud(2.0f), UI_POSITIVO);
        }

        float cx = boton.x + boton.width * 0.5f;

        DibujarTextoAjustadoUI(
            acciones[b], cx, boton.y + Px(7.0f), UI_TEXTO,
            boton.width - Px(16.0f), b == 0 ? COLOR_TINTA_POSITIVA_HUD : UI_TINTA, true
        );
        DibujarTextoAjustadoUI(
            teclas[b], cx, boton.y + Px(29.0f), UI_TEXTO_PEQUENO - 2,
            boton.width - Px(16.0f), UI_TINTA_SUAVE, true
        );
    }
}


// Texto de la leyenda de un tipo de casilla con su valor REAL: primero lo
// que diga el tablero (p. ej. la floracion) y, si no, el valor comun.
static void TextoLeyendaCasillaHud(
    const PartidaTablero& partida,
    TipoCasilla tipo,
    char* texto,
    int capacidad
)
{
    if (
        partida.definicion != nullptr &&
        partida.definicion->TextoLeyendaCasilla != nullptr &&
        partida.definicion->TextoLeyendaCasilla(partida, tipo, texto, capacidad)
    )
    {
        return;
    }

    switch (tipo)
    {
        case CASILLA_POSITIVA:
            std::snprintf(texto, capacidad, "+%d", MONEDAS_CASILLA_POSITIVA);
            return;

        case CASILLA_NEGATIVA:
            std::snprintf(texto, capacidad, "-%d", MONEDAS_CASILLA_NEGATIVA);
            return;

        case CASILLA_ESPECIAL:
            // Un tablero con regla propia para ESPECIAL (ResolverCasilla)
            // debe describirla con TextoLeyendaCasilla; si no, solo "AZAR".
            if (
                partida.definicion == nullptr ||
                partida.definicion->ResolverCasilla == nullptr ||
                partida.definicion->TextoLeyendaCasilla != nullptr
            )
            {
                std::snprintf(texto, capacidad, "AZAR +%d/-%d", MONEDAS_ESPECIAL_BONIFICACION, MONEDAS_ESPECIAL_MULTA);
            }
            else
            {
                std::snprintf(texto, capacidad, "AZAR");
            }
            return;

        case CASILLA_REGALO:
            std::snprintf(texto, capacidad, "REGALO +%d", MONEDAS_REGALO);
            return;

        case CASILLA_CAOS:
            std::snprintf(texto, capacidad, "CAOS");
            return;

        case CASILLA_INTERCAMBIO:
            std::snprintf(texto, capacidad, "TRUEQUE");
            return;

        case CASILLA_DUELO:
            std::snprintf(texto, capacidad, "DUELO");
            return;

        case CASILLA_TIENDA:
            std::snprintf(texto, capacidad, "TIENDA");
            return;

        case CASILLA_EVENTO:
            std::snprintf(texto, capacidad, "EVENTO");
            return;

        case CASILLA_NEUTRA:
        case CANTIDAD_TIPOS_CASILLA:
            break;
    }

    texto[0] = '\0';
}


// Leyenda de las casillas PRESENTES en este tablero y precio del trofeo
// (abajo a la derecha). Las entradas se acomodan en filas de ancho
// limitado para quedar a la derecha del cartel del turno sin tapar el
// tablero.
static void DibujarLeyendaHud(
    const EstiloUI& estilo,
    const PartidaTablero& partida
)
{
    const TipoCasilla orden[] =
    {
        CASILLA_POSITIVA, CASILLA_NEGATIVA, CASILLA_ESPECIAL, CASILLA_REGALO,
        CASILLA_CAOS, CASILLA_INTERCAMBIO, CASILLA_DUELO, CASILLA_TIENDA,
        CASILLA_EVENTO
    };

    const int cantidadOrden = (int)(sizeof(orden) / sizeof(orden[0]));

    bool presentes[CANTIDAD_TIPOS_CASILLA] = {};

    for (int i = 0; i < partida.tablero.cantidadCasillas; i++)
    {
        int tipo = (int)partida.tablero.casillas[i].tipo;

        if (tipo >= 0 && tipo < CANTIDAD_TIPOS_CASILLA)
        {
            presentes[tipo] = true;
        }
    }

    // Entradas: tipos presentes y, al final, el precio del trofeo.
    struct EntradaLeyenda
    {
        TipoCasilla tipo;
        char texto[24];
        float ancho;
    };

    EntradaLeyenda entradas[CANTIDAD_TIPOS_CASILLA + 1] = {};
    int cantidad = 0;
    int tamano = TamanoTexto(UI_TEXTO_PEQUENO - 2);

    for (int i = 0; i < cantidadOrden; i++)
    {
        if (!presentes[orden[i]])
        {
            continue;
        }

        EntradaLeyenda& entrada = entradas[cantidad++];

        entrada.tipo = orden[i];
        TextoLeyendaCasillaHud(partida, orden[i], entrada.texto, (int)sizeof(entrada.texto));
        entrada.ancho = Px(16.0f) + (float)MeasureText(entrada.texto, tamano) + Px(12.0f);
    }

    EntradaLeyenda& trofeo = entradas[cantidad++];

    trofeo.tipo = CASILLA_NEUTRA;
    std::snprintf(trofeo.texto, sizeof(trofeo.texto), "%d", partida.costoTrofeo);
    trofeo.ancho = Px(18.0f) + (float)MeasureText(trofeo.texto, tamano) + Px(12.0f);

    // Reparto en filas: el cartel del turno termina en x = 910 (ref), asi
    // que la leyenda ocupa a lo sumo los 340 de la derecha.
    const float anchoMaximo = Px(340.0f) - Px(12.0f);
    const float altoFila = Px(20.0f);

    int filaDe[CANTIDAD_TIPOS_CASILLA + 1] = {};
    float anchoFila[CANTIDAD_TIPOS_CASILLA + 1] = {};
    int filas = 1;

    for (int i = 0; i < cantidad; i++)
    {
        if (anchoFila[filas - 1] > 0.0f && anchoFila[filas - 1] + entradas[i].ancho > anchoMaximo)
        {
            filas++;
        }

        filaDe[i] = filas - 1;
        anchoFila[filas - 1] += entradas[i].ancho;
    }

    float ancho = 0.0f;

    for (int f = 0; f < filas; f++)
    {
        if (anchoFila[f] > ancho)
        {
            ancho = anchoFila[f];
        }
    }

    ancho += Px(12.0f);

    float alto = altoFila * (float)filas + Px(8.0f);

    Rectangle rect = {
        XHud(UI_REF_ANCHO - 14.0f) - ancho,
        YAbajoHud(664.0f) - alto,
        ancho,
        alto
    };

    DrawRectangleRec(
        Rectangle{ rect.x + Px(2.0f), rect.y + Px(3.0f), rect.width, rect.height },
        Fade(BLACK, 0.22f)
    );
    DibujarNueveCortes(estilo.placa, 8, rect, EscalaUI(), WHITE);
    DrawRectangleLinesEx(rect, GrosorLineaHud(1.0f), Fade(UI_MADERA_OSCURA, 0.6f));

    float xFila[CANTIDAD_TIPOS_CASILLA + 1] = {};

    for (int f = 0; f < filas; f++)
    {
        xFila[f] = rect.x + Px(12.0f);
    }

    for (int i = 0; i < cantidad; i++)
    {
        float x = xFila[filaDe[i]];
        float cy = rect.y + Px(4.0f) + altoFila * ((float)filaDe[i] + 0.5f);
        bool esTrofeo = i == cantidad - 1;

        if (esTrofeo)
        {
            DibujarTrofeoUI(Vector2{ x + Px(6.0f), cy }, 14.0f);
            x += Px(18.0f);
        }
        else
        {
            DrawCircleV(Vector2{ x + Px(5.0f), cy }, Px(6.0f), UI_TINTA);
            DrawCircleV(Vector2{ x + Px(5.0f), cy }, Px(5.0f), ObtenerColorCasilla(entradas[i].tipo));
            x += Px(16.0f);
        }

        DrawText(entradas[i].texto, (int)x, (int)(cy - (float)tamano * 0.5f), tamano, UI_TINTA);
        xFila[filaDe[i]] += entradas[i].ancho;
    }
}


//==================================================
// ANUNCIOS SOBRE EL MUNDO
//==================================================

static float AlfaAnuncioHud(float t, float duracion)
{
    if (t > duracion - 0.35f)
    {
        return Limitar01Hud((duracion - t) / 0.35f);
    }

    return 1.0f;
}


static void DibujarAnunciosHud(
    const EstiloUI& estilo,
    const PartidaTablero& partida
)
{
    const EstadoHudTablero& e = estadoHudTablero;
    bool reducir = estilo.reducirMovimiento;
    float cx = (float)GetScreenWidth() * 0.5f;
    float yCentro = (float)GetScreenHeight() * 0.40f;
    int turno = ObtenerParticipanteTurnoTableroFinal(partida);

    // +3 / -3 de la casilla: sube desde la ficha y se desvanece.
    char textoSello[8] = "";
    Color colorSello = UI_POSITIVO;

    if (
        turno >= 0 &&
        SelloEventoHud(partida, textoSello, (int)sizeof(textoSello), colorSello) &&
        partida.variacionMonedasEvento != 0
    )
    {
        float t = partida.tiempoFase;
        Vector2 ficha = PuntoSobreFichaHud(partida, turno, 1.6f);
        float subida = reducir ? 0.0f : Px(36.0f) * Limitar01Hud(t / 0.8f);
        int token = (int)(UI_TEXTO_TITULO * EscalaReboteHud(t, 0.25f, 0.4f, reducir));

        DibujarTextoMundoContornoUI(
            textoSello,
            ficha.x,
            ficha.y - Px(70.0f) - subida,
            token,
            partida.variacionMonedasEvento > 0 ? COLOR_MUNDO_POSITIVO_HUD : COLOR_MUNDO_NEGATIVO_HUD,
            AlfaAnuncioHud(t, T_EVENTO_CASILLA)
        );
    }

    // Trofeo recien comprado.
    if (
        e.ultimoTrofeo >= 0 &&
        e.tiempoTrofeo[e.ultimoTrofeo] < T_ANUNCIO_TROFEO
    )
    {
        float t = e.tiempoTrofeo[e.ultimoTrofeo];
        int token = (int)(UI_TEXTO_TITULO * EscalaReboteHud(t, 0.25f, 0.35f, reducir));

        DibujarTextoMundoContornoUI(
            TextFormat("¡J%d CONSIGUE UN TROFEO!", e.ultimoTrofeo + 1),
            cx, yCentro - TamanoTexto(UI_TEXTO_TITULO) * 0.5f,
            token, UI_FOCO, AlfaAnuncioHud(t, T_ANUNCIO_TROFEO)
        );

        return;
    }

    if (partida.fase == FASE_PARTIDA_TABLERO_FIN_RONDA)
    {
        int token = (int)(UI_TEXTO_TITULO * EscalaReboteHud(partida.tiempoFase, 0.25f, 0.35f, reducir));

        DibujarTextoMundoContornoUI(
            "¡FIN DE LA RONDA!",
            cx, yCentro - TamanoTexto(UI_TEXTO_TITULO) * 0.5f,
            token, UI_FOCO, 1.0f
        );

        return;
    }

    // Comienzo de ronda (no tapa el resultado de un duelo).
    if (
        e.tiempoRonda < T_ANUNCIO_RONDA &&
        partida.fase != FASE_PARTIDA_TABLERO_TERMINADA &&
        partida.fase != FASE_PARTIDA_TABLERO_DUELO_RESULTADO
    )
    {
        bool ultima = partida.rondaActual >= partida.cantidadRondas && partida.cantidadRondas > 1;
        int token = (int)(UI_TEXTO_TITULO * EscalaReboteHud(e.tiempoRonda, 0.25f, 0.35f, reducir));

        DibujarTextoMundoContornoUI(
            ultima ? "¡ÚLTIMA RONDA!" : TextFormat("RONDA %d", partida.rondaActual),
            cx, yCentro - TamanoTexto(UI_TEXTO_TITULO) * 0.5f,
            token, ultima ? COLOR_MUNDO_NEGATIVO_HUD : UI_TEXTO_MUNDO,
            AlfaAnuncioHud(e.tiempoRonda, T_ANUNCIO_RONDA)
        );
    }
}


//==================================================
// RESULTADO FINAL
//==================================================

static int CompararClasificacionTablero(
    const PartidaTablero& partida,
    int a,
    int b
)
{
    const EstadoJugadorPartidaTablero& jugadorA = partida.jugadores[a];
    const EstadoJugadorPartidaTablero& jugadorB = partida.jugadores[b];

    if (jugadorA.trofeos != jugadorB.trofeos)
    {
        return jugadorA.trofeos > jugadorB.trofeos ? -1 : 1;
    }

    if (jugadorA.monedas != jugadorB.monedas)
    {
        return jugadorA.monedas > jugadorB.monedas ? -1 : 1;
    }

    return 0;
}


static void DibujarResultadoFinalTablero(
    const PartidaTablero& partida
)
{
    if (partida.fase != FASE_PARTIDA_TABLERO_TERMINADA)
    {
        return;
    }

    const EstiloUI& estilo = ObtenerEstiloUI();

    int ranking[MAX_PARTICIPANTES]{};

    for (int i = 0; i < partida.cantidadJugadores; i++)
    {
        ranking[i] = partida.ordenParticipantes[i];
    }

    for (int i = 0; i < partida.cantidadJugadores - 1; i++)
    {
        for (int j = i + 1; j < partida.cantidadJugadores; j++)
        {
            if (
                CompararClasificacionTablero(
                    partida,
                    ranking[j],
                    ranking[i]
                ) < 0
            )
            {
                int temporal = ranking[i];
                ranking[i] = ranking[j];
                ranking[j] = temporal;
            }
        }
    }

    bool empatePrimero =
        partida.cantidadJugadores > 1 &&
        CompararClasificacionTablero(
            partida,
            ranking[0],
            ranking[1]
        ) == 0;

    // Un solo velo y el tablon, que entra cayendo.
    float aparicion = estilo.reducirMovimiento ? 1.0f : Limitar01Hud(partida.tiempoFase / 0.25f);
    DibujarVeloUI(0.55f * aparicion);

    const float ancho = 560.0f;
    const float altoFila = 40.0f;
    float alto = 14.0f + 56.0f + partida.cantidadJugadores * altoFila + 44.0f + 14.0f;
    float caida = 0.0f;

    if (!estilo.reducirMovimiento)
    {
        float resto = 1.0f - Limitar01Hud(partida.tiempoFase / 0.35f);
        caida = -(alto + 120.0f) * resto * resto * resto;
    }

    Rectangle rect = RectRef(
        UI_REF_ANCHO * 0.5f - ancho * 0.5f,
        UI_REF_ALTO * 0.5f - alto * 0.5f + caida,
        ancho,
        alto
    );

    Rectangle placa = DibujarTablonHud(estilo, rect);
    float cx = rect.x + rect.width * 0.5f;

    const char* titulo =
        empatePrimero
        ? "¡EMPATE EN EL PRIMER PUESTO!"
        : TextFormat("¡GANA J%d!", ranking[0] + 1);

    DibujarTextoAjustadoUI(titulo, cx, placa.y + Px(12.0f), UI_TEXTO_GRANDE, placa.width - Px(24.0f), UI_TINTA, true);

    DrawRectangleRec(
        Rectangle{ placa.x + Px(16.0f), placa.y + Px(50.0f), placa.width - Px(32.0f), Px(2.0f) },
        Fade(UI_MADERA_OSCURA, 0.45f)
    );

    static const Color medallas[4] = {
        { 255, 200, 70, 255 },      // oro (UI_FOCO)
        { 206, 208, 216, 255 },     // plata
        { 220, 156, 104, 255 },     // bronce
        { 196, 170, 130, 255 }      // madera clara
    };

    int puesto = 1;

    for (int f = 0; f < partida.cantidadJugadores; f++)
    {
        int indice = ranking[f];
        const Participante& participante = partida.participantes[indice];

        // Empatados comparten puesto.
        if (f > 0 && CompararClasificacionTablero(partida, ranking[f - 1], indice) != 0)
        {
            puesto = f + 1;
        }

        float y = placa.y + Px(58.0f + f * altoFila);
        float centroY = y + Px(altoFila * 0.5f);

        if (puesto == 1)
        {
            DrawRectangleRec(
                Rectangle{ placa.x + Px(8.0f), y + Px(2.0f), placa.width - Px(16.0f), Px(altoFila - 4.0f) },
                Fade(UI_FOCO, 0.30f)
            );
        }

        float xMedalla = placa.x + Px(30.0f);
        DrawCircleV(Vector2{ xMedalla, centroY }, Px(13.5f), UI_TINTA);
        DrawCircleV(Vector2{ xMedalla, centroY }, Px(12.0f), medallas[puesto <= 3 ? puesto - 1 : 3]);

        const char* textoPuesto = TextFormat("%d", puesto);
        int tamanoPuesto = TamanoTexto(UI_TEXTO_PEQUENO);
        DrawText(textoPuesto, (int)(xMedalla - MeasureText(textoPuesto, tamanoPuesto) * 0.5f), (int)(centroY - tamanoPuesto * 0.5f), tamanoPuesto, UI_TINTA);

        DibujarBanderinColganteUI(Vector2{ placa.x + Px(56.0f), centroY - Px(12.0f) }, participante.color, 14.0f, 24.0f);

        int tamanoNombre = TamanoTexto(UI_TEXTO);
        const char* nombre = TextFormat("J%d", indice + 1);
        float xNombre = placa.x + Px(80.0f);
        DrawText(nombre, (int)xNombre, (int)(centroY - tamanoNombre * 0.5f), tamanoNombre, UI_TINTA);

        if (participante.esBot)
        {
            DrawText(
                "BOT",
                (int)(xNombre + MeasureText(nombre, tamanoNombre) + Px(8.0f)),
                (int)(centroY - TamanoTexto(UI_TEXTO_PEQUENO) * 0.5f + Px(1.0f)),
                TamanoTexto(UI_TEXTO_PEQUENO),
                UI_TINTA_SUAVE
            );
        }

        // Trofeos y monedas a la derecha.
        float derecha = placa.x + placa.width - Px(18.0f);
        const char* monedas = TextFormat("%d", partida.jugadores[indice].monedas);
        float xMonedas = derecha - MeasureText(monedas, tamanoNombre);
        DrawText(monedas, (int)xMonedas, (int)(centroY - tamanoNombre * 0.5f), tamanoNombre, UI_TINTA);
        DibujarMonedaUI(Vector2{ xMonedas - Px(14.0f), centroY }, 8.0f);

        const char* trofeos = TextFormat("%d", partida.jugadores[indice].trofeos);
        float xTrofeos = xMonedas - Px(86.0f);
        DrawText(trofeos, (int)xTrofeos, (int)(centroY - tamanoNombre * 0.5f), tamanoNombre, UI_TINTA);
        DibujarTrofeoUI(Vector2{ xTrofeos - Px(14.0f), centroY }, 20.0f);
    }

    DibujarTextoAjustadoUI(
        "ENTER / ESC: VOLVER   |   R: REINICIAR",
        cx, placa.y + placa.height - Px(28.0f), UI_TEXTO_PEQUENO,
        placa.width - Px(24.0f), UI_TINTA_SUAVE, true
    );
}


//==================================================
// HUD COMPLETO
//==================================================

static void DibujarHudTablero(
    const PartidaTablero& partida
)
{
    const EstiloUI& estilo = ObtenerEstiloUI();

    if (
        !estilo.cargado ||
        partida.participantes == nullptr ||
        partida.cantidadJugadores <= 0
    )
    {
        return;
    }

    ActualizarEstadoHudTablero(partida, estilo);

    if (partida.fase == FASE_PARTIDA_TABLERO_TERMINADA)
    {
        DibujarTendederoHud(estilo, partida);
        DibujarResultadoFinalTablero(partida);
        return;
    }

    int turno = ObtenerParticipanteTurnoTableroFinal(partida);

    // Sobre el mundo, lo que esta pegado a fichas y casillas.
    DibujarEtiquetasRutaHud(estilo, partida);
    DibujarDadoHud(estilo, partida);

    // Eventos interactivos (ruleta, dados, objetos): velo y escena propia,
    // bajo el tendedero para que las monedas sigan a la vista.
    DibujarEventoInteractivo2D(partida);
    DibujarMenuObjetos2D(partida);
    DibujarDueloResultado2D(partida);

    DibujarTendederoHud(estilo, partida);

    if (turno >= 0)
    {
        if (partida.fase == FASE_PARTIDA_TABLERO_DECISION_TROFEO)
        {
            DibujarTablonTrofeoHud(estilo, partida, turno);
        }
        else
        {
            DibujarCartelTurnoHud(estilo, partida, turno);
        }

        char ayuda[160] = "";
        TextoAyudaHud(partida, turno, ayuda, (int)sizeof(ayuda));

        if (ayuda[0] != '\0')
        {
            DibujarAyudaUI(estilo, ayuda);
        }
    }

    DibujarLeyendaHud(estilo, partida);
    DibujarInventariosHud(partida);
    DibujarAnunciosHud(estilo, partida);
    DibujarAvisoObjeto(partida);
}


void DibujarEscenaTablero3D(
    const PartidaTablero& partida
)
{
    if (partida.definicion == nullptr)
    {
        return;
    }

    if (partida.definicion->DibujarDecoracion != nullptr)
    {
        partida.definicion->DibujarDecoracion(partida);
    }

    partida.tablero.DibujarRuta(
        partida.definicion->estiloCasilla,
        partida.tiempoTablero
    );

    if (partida.definicion->DibujarGimmick != nullptr)
    {
        partida.definicion->DibujarGimmick(partida);
    }

    DibujarTrofeoTableroFinal(partida);
    DibujarRutaSeleccionadaTableroFinal(partida);
    DibujarFichasTableroFinal(partida);
    DibujarEfectosTrofeoTableroFinal(partida);
    DibujarEventosCasillas3D(partida);
}


void PartidaTablero::Dibujar() const
{
    ClearBackground(definicion->colorFondo);

    // BeginMode3D esta redefinido por EfectosVisualesMinijuegos: el tablero
    // nunca debe heredar el tema visual del ultimo minijuego.
    SeleccionarTemaVisualMinijuego(TEMA_VISUAL_NINGUNO);

    BeginMode3D(camara);

    DibujarEscenaTablero3D(*this);

    EndMode3D();

    if (definicion->DibujarCapaPantalla != nullptr)
    {
        definicion->DibujarCapaPantalla(*this);
    }

    if (!tablero.recorridoValido)
    {
        DibujarTextoMundoContornoUI(
            "ERROR: RECORRIDO DE TABLERO INVALIDO",
            GetScreenWidth() * 0.5f,
            Px(40.0f),
            UI_TEXTO_GRANDE,
            COLOR_MUNDO_NEGATIVO_HUD
        );

        return;
    }

    DibujarHudTablero(*this);
}
