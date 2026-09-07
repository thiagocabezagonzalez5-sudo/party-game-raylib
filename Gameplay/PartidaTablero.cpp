#include "Gameplay/PartidaTablero.h"

#include "Systems/Input.h"

#include <cmath>


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


static int ObtenerParticipanteTurnoTableroFinal(
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


static void ReproducirSonidoTablero(
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
    const int candidatas[] =
    {
        2,
        5,
        8,
        11,
        14,
        17
    };

    const int cantidadCandidatas =
        sizeof(candidatas) / sizeof(candidatas[0]);

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
    partida.fase = FASE_PARTIDA_TABLERO_ESPERANDO_DADO;
    partida.tiempoFase = 0.0f;
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
        SONIDO_RESULTADO
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


static int ElegirRivalIntercambio(
    const PartidaTablero& partida,
    int indiceParticipante
)
{
    int candidatos[MAX_PARTICIPANTES]{};
    int cantidad = 0;

    for (int i = 0; i < partida.cantidadJugadores; i++)
    {
        int indice = partida.ordenParticipantes[i];

        if (indice != indiceParticipante)
        {
            candidatos[cantidad] = indice;
            cantidad++;
        }
    }

    if (cantidad <= 0)
    {
        return -1;
    }

    return candidatos[GetRandomValue(0, cantidad - 1)];
}


static void ResolverEventoEspecialTablero(
    PartidaTablero& partida,
    int indiceParticipante
)
{
    EstadoJugadorPartidaTablero& jugador =
        partida.jugadores[indiceParticipante];

    int evento = GetRandomValue(0, 2);

    if (evento == 0)
    {
        partida.ultimoEventoEspecial = EVENTO_TABLERO_BONIFICACION;
        partida.variacionMonedasEvento = 6;
        jugador.monedas += 6;
    }
    else if (evento == 1)
    {
        partida.ultimoEventoEspecial = EVENTO_TABLERO_MULTA;

        int perdida = jugador.monedas < 4
            ? jugador.monedas
            : 4;

        jugador.monedas -= perdida;
        partida.variacionMonedasEvento = -perdida;
    }
    else
    {
        int rival =
            ElegirRivalIntercambio(partida, indiceParticipante);

        if (rival < 0)
        {
            partida.ultimoEventoEspecial = EVENTO_TABLERO_BONIFICACION;
            partida.variacionMonedasEvento = 4;
            jugador.monedas += 4;
            return;
        }

        partida.ultimoEventoEspecial = EVENTO_TABLERO_INTERCAMBIO;
        partida.jugadorIntercambioEvento = rival;

        int temporal = jugador.monedas;
        jugador.monedas = partida.jugadores[rival].monedas;
        partida.jugadores[rival].monedas = temporal;
    }
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

    if (casilla->tipo == CASILLA_POSITIVA)
    {
        jugador.monedas += 3;
        partida.variacionMonedasEvento = 3;
    }
    else if (casilla->tipo == CASILLA_NEGATIVA)
    {
        int perdida = jugador.monedas < 3
            ? jugador.monedas
            : 3;

        jugador.monedas -= perdida;
        partida.variacionMonedasEvento = -perdida;
    }
    else if (casilla->tipo == CASILLA_ESPECIAL)
    {
        ResolverEventoEspecialTablero(
            partida,
            indiceParticipante
        );
    }
}


void PartidaTablero::Inicializar(
    Participante participantesJuego[],
    int cantidadParticipantesJuego,
    AudioJuego* audioJuego
)
{
    participantes = participantesJuego;
    audio = audioJuego;

    tablero.InicializarPrototipo();

    camara.position = { 0.0f, 17.5f, 18.5f };
    camara.target = { 0.0f, 0.0f, 0.0f };
    camara.up = { 0.0f, 1.0f, 0.0f };
    camara.fovy = 48.0f;
    camara.projection = CAMERA_PERSPECTIVE;

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
    cantidadRondas = 5;
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
    minijuegoSolicitado = false;
    salidaSolicitada = false;

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

    PrepararInicioTurno(*this);
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

    if (fase == FASE_PARTIDA_TABLERO_TERMINADA)
    {
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
        IsKeyPressed(KEY_ESCAPE)
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

    bool esBot = participante.esBot;
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
            lanzar = tiempoFase >= 0.70f;
        }
        else if (puedeDecidir)
        {
            InputSeleccionParticipante entrada =
                LeerInputSeleccionParticipante(participante);

            lanzar = entrada.confirmar;
        }

        if (lanzar)
        {
            valorDado = GetRandomValue(1, 10);
            pasosPendientes = valorDado;
            fase = FASE_PARTIDA_TABLERO_MOSTRANDO_DADO;
            tiempoFase = 0.0f;

            ReproducirSonidoTablero(
                *this,
                SONIDO_UI_CONFIRMAR
            );
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
                opcionRuta =
                    GetRandomValue(
                        0,
                        casilla->cantidadConexiones - 1
                    );

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
            LeerInputSeleccionParticipante(participante);

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
            LeerInputSeleccionParticipante(participante);

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
        }

        if (tiempoFase >= 1.35f)
        {
            fase = FASE_PARTIDA_TABLERO_FIN_TURNO;
            tiempoFase = 0.0f;
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
        fase == FASE_PARTIDA_TABLERO_FIN_RONDA &&
        minijuegoSolicitado;
}


void PartidaTablero::AplicarResultadoMinijuego(
    const ResultadoMinijuego& resultado
)
{
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
    minijuegoSolicitado = false;

    if (rondaActual >= cantidadRondas)
    {
        fase = FASE_PARTIDA_TABLERO_TERMINADA;
        tiempoFase = 0.0f;
        return;
    }

    rondaActual++;
    indiceOrdenTurno = 0;
    PrepararInicioTurno(*this);
}


bool PartidaTablero::SolicitaSalida() const
{
    return salidaSolicitada;
}


static void DibujarDecoracionTableroFinal()
{
    DrawCube(
        Vector3{ 0.0f, -0.42f, 0.0f },
        20.5f,
        0.65f,
        14.5f,
        Color{ 62, 118, 84, 255 }
    );

    DrawCube(
        Vector3{ 0.0f, -0.55f, 0.0f },
        21.3f,
        0.38f,
        15.3f,
        Color{ 196, 178, 112, 255 }
    );

    const Vector3 arboles[] =
    {
        { -6.5f, 0.35f,  1.2f },
        { -4.2f, 0.35f, -1.0f },
        {  0.4f, 0.35f,  1.0f },
        {  3.0f, 0.35f, -1.6f },
        {  6.2f, 0.35f,  1.5f }
    };

    for (const Vector3& base : arboles)
    {
        DrawCylinder(
            base,
            0.18f,
            0.24f,
            1.0f,
            8,
            BROWN
        );

        Vector3 copa = base;
        copa.y += 0.90f;

        DrawSphere(
            copa,
            0.56f,
            Color{ 48, 145, 74, 255 }
        );
    }
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

    Vector3 base = casilla->posicion;
    base.y += 0.72f;

    DrawCylinder(
        base,
        0.30f,
        0.22f,
        0.34f,
        12,
        GOLD
    );

    Vector3 copa = base;
    copa.y += 0.38f;

    DrawSphere(copa, 0.34f, YELLOW);
    DrawSphereWires(copa, 0.50f, 10, 10, ORANGE);
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

        DrawCube(cuerpo, 0.50f, 0.62f, 0.50f, color);
        DrawCubeWires(cuerpo, 0.50f, 0.62f, 0.50f, BLACK);

        Vector3 cabeza = cuerpo;
        cabeza.y += 0.48f;
        DrawSphere(cabeza, 0.27f, color);

        if (
            indiceParticipante == participanteTurno &&
            partida.fase != FASE_PARTIDA_TABLERO_TERMINADA
        )
        {
            DrawSphereWires(cuerpo, 0.58f, 8, 8, YELLOW);
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

    Vector3 marcador = destino->posicion;
    marcador.y += 0.15f;

    DrawCubeWires(
        marcador,
        1.92f,
        0.82f,
        1.92f,
        YELLOW
    );
}


static const char* ObtenerTextoFaseTableroFinal(
    const PartidaTablero& partida
)
{
    int indiceParticipante =
        ObtenerParticipanteTurnoTableroFinal(partida);

    if (partida.fase == FASE_PARTIDA_TABLERO_TERMINADA)
    {
        return "PARTIDA TERMINADA";
    }

    if (indiceParticipante < 0 || partida.participantes == nullptr)
    {
        return "SIN JUGADORES";
    }

    const Participante& participante =
        partida.participantes[indiceParticipante];

    if (partida.fase == FASE_PARTIDA_TABLERO_ESPERANDO_DADO)
    {
        if (participante.esBot)
        {
            return TextFormat(
                "TURNO J%d - BOT LANZANDO DADO...",
                indiceParticipante + 1
            );
        }

        if (!participante.conectado)
        {
            return TextFormat(
                "J%d: RECONECTA TU CONTROL",
                indiceParticipante + 1
            );
        }

        return TextFormat(
            "J%d: %s PARA LANZAR EL DADO",
            indiceParticipante + 1,
            ObtenerTextoBotonPrincipal(participante)
        );
    }

    if (partida.fase == FASE_PARTIDA_TABLERO_MOSTRANDO_DADO)
    {
        return TextFormat(
            "J%d SACO %d",
            indiceParticipante + 1,
            partida.valorDado
        );
    }

    if (partida.fase == FASE_PARTIDA_TABLERO_ELIGIENDO_RUTA)
    {
        return TextFormat(
            "J%d: ELEGIR RUTA %d",
            indiceParticipante + 1,
            partida.opcionRuta + 1
        );
    }

    if (partida.fase == FASE_PARTIDA_TABLERO_MOVIENDO)
    {
        return TextFormat(
            "MOVIENDO - %d PASOS RESTANTES",
            partida.pasosPendientes
        );
    }

    if (partida.fase == FASE_PARTIDA_TABLERO_DECISION_TROFEO)
    {
        if (!partida.compraTrofeoDisponible)
        {
            return TextFormat(
                "TROFEO: NECESITAS %d MONEDAS",
                partida.costoTrofeo
            );
        }

        return TextFormat(
            "COMPRAR TROFEO POR %d MONEDAS? CONFIRMAR / CANCELAR",
            partida.costoTrofeo
        );
    }

    if (partida.fase == FASE_PARTIDA_TABLERO_EVENTO_CASILLA)
    {
        const EstadoJugadorPartidaTablero& jugador =
            partida.jugadores[indiceParticipante];

        const Casilla* casilla =
            partida.tablero.ObtenerCasilla(jugador.casillaActual);

        if (casilla == nullptr)
        {
            return "EVENTO DE CASILLA";
        }

        if (casilla->tipo == CASILLA_POSITIVA)
        {
            return "CASILLA VERDE: +3 MONEDAS";
        }

        if (casilla->tipo == CASILLA_NEGATIVA)
        {
            return TextFormat(
                "CASILLA ROJA: %d MONEDAS",
                partida.variacionMonedasEvento
            );
        }

        if (casilla->tipo == CASILLA_ESPECIAL)
        {
            if (
                partida.ultimoEventoEspecial ==
                EVENTO_TABLERO_BONIFICACION
            )
            {
                return TextFormat(
                    "EVENTO: BONIFICACION +%d MONEDAS",
                    partida.variacionMonedasEvento
                );
            }

            if (
                partida.ultimoEventoEspecial ==
                EVENTO_TABLERO_MULTA
            )
            {
                return TextFormat(
                    "EVENTO: MULTA %d MONEDAS",
                    partida.variacionMonedasEvento
                );
            }

            if (
                partida.ultimoEventoEspecial ==
                EVENTO_TABLERO_INTERCAMBIO
            )
            {
                return TextFormat(
                    "EVENTO: J%d INTERCAMBIA MONEDAS CON J%d",
                    indiceParticipante + 1,
                    partida.jugadorIntercambioEvento + 1
                );
            }

            return "EVENTO ESPECIAL";
        }

        return "CASILLA AZUL: SIN CAMBIOS";
    }

    if (partida.fase == FASE_PARTIDA_TABLERO_FIN_RONDA)
    {
        return "FIN DE RONDA - PREPARANDO MINIJUEGO";
    }

    return "FINALIZANDO TURNO...";
}


static void DibujarMarcadorTableroFinal(
    const PartidaTablero& partida
)
{
    const int margen = 16;
    const int separacion = 8;

    int anchoDisponible =
        GetScreenWidth() -
        margen * 2 -
        separacion * (partida.cantidadJugadores - 1);

    int anchoTarjeta =
        partida.cantidadJugadores > 0
        ? anchoDisponible / partida.cantidadJugadores
        : anchoDisponible;

    int participanteTurno =
        ObtenerParticipanteTurnoTableroFinal(partida);

    for (int i = 0; i < partida.cantidadJugadores; i++)
    {
        int indiceParticipante = partida.ordenParticipantes[i];
        int x = margen + i * (anchoTarjeta + separacion);

        Color color =
            partida.participantes[indiceParticipante].color;

        DrawRectangle(
            x,
            14,
            anchoTarjeta,
            70,
            Fade(
                color,
                indiceParticipante == participanteTurno
                ? 0.94f
                : 0.72f
            )
        );

        DrawRectangleLinesEx(
            Rectangle
            {
                (float)x,
                14.0f,
                (float)anchoTarjeta,
                70.0f
            },
            indiceParticipante == participanteTurno
                ? 4.0f
                : 2.0f,
            indiceParticipante == participanteTurno
                ? YELLOW
                : BLACK
        );

        DrawText(
            TextFormat("J%d", indiceParticipante + 1),
            x + 10,
            22,
            21,
            BLACK
        );

        DrawText(
            TextFormat(
                "T:%d  M:%d",
                partida.jugadores[indiceParticipante].trofeos,
                partida.jugadores[indiceParticipante].monedas
            ),
            x + 10,
            47,
            18,
            BLACK
        );

        const char* control =
            partida.participantes[indiceParticipante].esBot
            ? "BOT"
            : ObtenerNombreControlParticipante(
                partida.participantes[indiceParticipante]
            );

        DrawText(
            control,
            x + anchoTarjeta - MeasureText(control, 13) - 8,
            25,
            13,
            DARKGRAY
        );
    }
}


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

    DrawRectangle(
        0,
        0,
        GetScreenWidth(),
        GetScreenHeight(),
        Fade(BLACK, 0.72f)
    );

    Rectangle panel =
    {
        GetScreenWidth() / 2.0f - 330.0f,
        GetScreenHeight() / 2.0f - 235.0f,
        660.0f,
        470.0f
    };

    DrawRectangle(
        (int)panel.x,
        (int)panel.y,
        (int)panel.width,
        (int)panel.height,
        Color{ 24, 27, 35, 248 }
    );

    DrawRectangleLinesEx(panel, 4.0f, GOLD);

    const char* titulo = "RESULTADO FINAL";

    DrawText(
        titulo,
        GetScreenWidth() / 2 - MeasureText(titulo, 38) / 2,
        (int)panel.y + 28,
        38,
        GOLD
    );

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

    const char* ganador =
        empatePrimero
        ? "EMPATE EN EL PRIMER PUESTO"
        : TextFormat("GANA JUGADOR %d", ranking[0] + 1);

    DrawText(
        ganador,
        GetScreenWidth() / 2 - MeasureText(ganador, 25) / 2,
        (int)panel.y + 83,
        25,
        RAYWHITE
    );

    for (int i = 0; i < partida.cantidadJugadores; i++)
    {
        int indice = ranking[i];
        int y = (int)panel.y + 135 + i * 58;

        Color color = partida.participantes[indice].color;

        DrawRectangle(
            (int)panel.x + 46,
            y,
            568,
            46,
            Fade(color, 0.72f)
        );

        DrawText(
            TextFormat(
                "%d. J%d     TROFEOS %d     MONEDAS %d",
                i + 1,
                indice + 1,
                partida.jugadores[indice].trofeos,
                partida.jugadores[indice].monedas
            ),
            (int)panel.x + 62,
            y + 12,
            20,
            BLACK
        );
    }

    const char* ayuda =
        "ENTER / ESC: VOLVER    R: REINICIAR";

    DrawText(
        ayuda,
        GetScreenWidth() / 2 - MeasureText(ayuda, 18) / 2,
        (int)panel.y + 430,
        18,
        LIGHTGRAY
    );
}


void PartidaTablero::Dibujar() const
{
    ClearBackground(Color{ 86, 165, 210, 255 });

    BeginMode3D(camara);

    DibujarDecoracionTableroFinal();
    tablero.Dibujar();
    DibujarTrofeoTableroFinal(*this);
    DibujarRutaSeleccionadaTableroFinal(*this);
    DibujarFichasTableroFinal(*this);

    EndMode3D();

    if (!tablero.recorridoValido)
    {
        DrawText(
            "ERROR: RECORRIDO DE TABLERO INVALIDO",
            32,
            32,
            28,
            RED
        );

        return;
    }

    DibujarMarcadorTableroFinal(*this);

    DrawRectangle(
        18,
        98,
        600,
        118,
        Fade(RAYWHITE, 0.92f)
    );

    DrawRectangleLinesEx(
        Rectangle{ 18.0f, 98.0f, 600.0f, 118.0f },
        2.0f,
        DARKBLUE
    );

    DrawText(
        TextFormat(
            "RONDA %d / %d",
            rondaActual,
            cantidadRondas
        ),
        34,
        111,
        24,
        DARKBLUE
    );

    DrawText(
        ObtenerTextoFaseTableroFinal(*this),
        34,
        145,
        18,
        BLACK
    );

    DrawText(
        "VERDE +3 | ROJA -3 | DORADA EVENTO | TROFEO 20 MONEDAS",
        34,
        181,
        16,
        DARKGRAY
    );

    if (valorDado > 0)
    {
        int tamanoDado = 104;
        int xDado = GetScreenWidth() - tamanoDado - 24;

        DrawRectangle(
            xDado,
            100,
            tamanoDado,
            tamanoDado,
            RAYWHITE
        );

        DrawRectangleLinesEx(
            Rectangle
            {
                (float)xDado,
                100.0f,
                (float)tamanoDado,
                (float)tamanoDado
            },
            4.0f,
            BLACK
        );

        const char* textoDado = TextFormat("%d", valorDado);

        DrawText(
            textoDado,
            xDado + tamanoDado / 2 - MeasureText(textoDado, 58) / 2,
            120,
            58,
            BLACK
        );
    }

    DrawRectangle(
        18,
        GetScreenHeight() - 50,
        510,
        32,
        Fade(BLACK, 0.58f)
    );

    DrawText(
        "ESC: SALIR DEL TABLERO",
        28,
        GetScreenHeight() - 42,
        16,
        RAYWHITE
    );

    DibujarResultadoFinalTablero(*this);
}
