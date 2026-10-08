#include "Gameplay/PartidaTablero.h"

#include "Minigames/EfectosVisualesMinijuegos.h"
#include "Minigames/ModeloJugadorCompartido.h"
#include "Systems/Input.h"

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
    partida.textoEvento[0] = '\0';

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
        jugador.monedas += 3;
        partida.variacionMonedasEvento = 3;
        ReproducirSonidoTablero(partida, SONIDO_CASILLA_POSITIVA);
    }
    else if (casilla->tipo == CASILLA_NEGATIVA)
    {
        int perdida = jugador.monedas < 3
            ? jugador.monedas
            : 3;

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
                SONIDO_DADO
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
                "J%d: RECONECTA TU CONTROL (JUEGA UN BOT EN %d s)",
                indiceParticipante + 1,
                (int)std::ceil(std::fmax(0.0f, 4.0f - partida.tiempoFase))
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
        const EstadoJugadorPartidaTablero& jugadorRuta =
            partida.jugadores[indiceParticipante];

        if (
            partida.definicion != nullptr &&
            partida.definicion->DescribirOpcionRuta != nullptr
        )
        {
            const char* descripcion =
                partida.definicion->DescribirOpcionRuta(
                    partida,
                    jugadorRuta.casillaActual,
                    partida.opcionRuta
                );

            if (descripcion != nullptr)
            {
                return TextFormat(
                    "J%d: RUTA %d - %s",
                    indiceParticipante + 1,
                    partida.opcionRuta + 1,
                    descripcion
                );
            }
        }

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
        // Evento resuelto por el gimmick del tablero.
        if (partida.textoEvento[0] != '\0')
        {
            return partida.textoEvento;
        }

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


//==================================================
// HUD COMPACTO
//==================================================

static float EscalaHudTablero()
{
    float escala = GetScreenHeight() / 720.0f;

    if (escala < 0.9f) escala = 0.9f;
    if (escala > 1.8f) escala = 1.8f;

    return escala;
}


// Texto con sombra para que se lea sobre el mapa sin necesitar un panel.
static void DibujarTextoHudTablero(
    const char* texto,
    int x,
    int y,
    int tamano,
    Color color
)
{
    DrawText(texto, x + 1, y + 1, tamano, Fade(BLACK, 0.75f));
    DrawText(texto, x, y, tamano, color);
}


static void DibujarMonedaHudTablero(int x, int y, int radio)
{
    DrawCircle(x, y, (float)radio, Color{ 255, 205, 50, 255 });
    DrawCircle(x, y, radio * 0.62f, Color{ 214, 150, 20, 255 });
    DrawCircle(x, y, radio * 0.40f, Color{ 255, 226, 110, 255 });
}


static void DibujarTrofeoHudTablero(int x, int y, int tamano)
{
    Color oro = Color{ 255, 205, 50, 255 };

    DrawRectangle(x - tamano / 2, y - tamano / 2, tamano, tamano / 2, oro);
    DrawCircle(x, y, tamano * 0.5f, oro);
    DrawRectangle(x - tamano / 8, y, tamano / 4, tamano / 2, oro);
    DrawRectangle(x - tamano / 2, y + tamano / 2, tamano, tamano / 6, Color{ 214, 150, 20, 255 });
}


static void DibujarMarcadorTableroFinal(
    const PartidaTablero& partida
)
{
    float esc = EscalaHudTablero();

    int margen = (int)(14 * esc);
    int separacion = (int)(8 * esc);
    int anchoTarjeta = (int)(176 * esc);
    int altoTarjeta = (int)(54 * esc);

    int participanteTurno =
        ObtenerParticipanteTurnoTableroFinal(partida);

    // Si no caben en la parte izquierda, se encogen.
    int maximo = (int)(GetScreenWidth() * 0.66f) - margen;
    int necesario =
        partida.cantidadJugadores * (anchoTarjeta + separacion);

    if (necesario > maximo && partida.cantidadJugadores > 0)
    {
        anchoTarjeta = maximo / partida.cantidadJugadores - separacion;
    }

    for (int i = 0; i < partida.cantidadJugadores; i++)
    {
        int indice = partida.ordenParticipantes[i];
        bool activo = indice == participanteTurno;

        int x = margen + i * (anchoTarjeta + separacion);
        int y = margen - (activo ? (int)(3 * esc) : 0);

        Color color = partida.participantes[indice].color;

        Rectangle caja =
        {
            (float)x, (float)y, (float)anchoTarjeta, (float)altoTarjeta
        };

        DrawRectangleRounded(caja, 0.25f, 6, Fade(Color{ 18, 22, 30, 255 }, activo ? 0.82f : 0.58f));

        // Banda con el color del jugador.
        DrawRectangleRounded(
            Rectangle{ caja.x, caja.y, 8.0f * esc, caja.height },
            0.6f, 4, color
        );

        if (activo)
        {
            DrawRectangleRoundedLinesEx(caja, 0.25f, 6, 2.5f * esc, YELLOW);
        }

        int t1 = (int)(17 * esc);
        int t2 = (int)(20 * esc);

        DibujarTextoHudTablero(
            TextFormat("J%d", indice + 1),
            x + (int)(18 * esc), y + (int)(6 * esc), t1, color
        );

        const char* control =
            partida.participantes[indice].esBot
            ? "BOT"
            : ObtenerNombreControlParticipante(partida.participantes[indice]);

        int tc = (int)(11 * esc);
        DrawText(
            control,
            x + anchoTarjeta - MeasureText(control, tc) - (int)(8 * esc),
            y + (int)(7 * esc), tc, Fade(RAYWHITE, 0.6f)
        );

        int yFila = y + (int)(33 * esc);
        int icono = (int)(7 * esc);

        DibujarTrofeoHudTablero(x + (int)(26 * esc), yFila, icono * 2 - 2);
        DibujarTextoHudTablero(
            TextFormat("%d", partida.jugadores[indice].trofeos),
            x + (int)(38 * esc), yFila - (int)(9 * esc), t2, RAYWHITE
        );

        DibujarMonedaHudTablero(x + (int)(anchoTarjeta * 0.52f), yFila, icono);
        DibujarTextoHudTablero(
            TextFormat("%d", partida.jugadores[indice].monedas),
            x + (int)(anchoTarjeta * 0.52f) + (int)(12 * esc),
            yFila - (int)(9 * esc), t2, RAYWHITE
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
        DrawText(
            "ERROR: RECORRIDO DE TABLERO INVALIDO",
            32,
            32,
            28,
            RED
        );

        return;
    }

    float esc = EscalaHudTablero();
    int ancho = GetScreenWidth();
    int alto = GetScreenHeight();
    int margen = (int)(14 * esc);

    DibujarMarcadorTableroFinal(*this);

    // Panel superior derecho: ronda, tablero y estado del gimmick.
    {
        int tRonda = (int)(24 * esc);
        int tPeq = (int)(14 * esc);
        int tGim = tPeq;
        int anchoMaxPanel = (int)(ancho * 0.34f);

        if (
            estadoGimmick[0] != '\0' &&
            MeasureText(estadoGimmick, tGim) + (int)(28 * esc) > anchoMaxPanel
        )
        {
            tGim = (int)(tGim * (float)(anchoMaxPanel - (int)(28 * esc)) / MeasureText(estadoGimmick, tGim));
        }

        const char* textoRonda =
            TextFormat("RONDA %d / %d", rondaActual, cantidadRondas);

        int anchoPanel = (int)(300 * esc);

        if (estadoGimmick[0] != '\0')
        {
            int anchoGimmick = MeasureText(estadoGimmick, tGim) + (int)(28 * esc);
            if (anchoGimmick > anchoPanel) anchoPanel = anchoGimmick;
        }

        int altoPanel = (int)((estadoGimmick[0] != '\0' ? 78 : 56) * esc);
        int x = ancho - anchoPanel - margen;

        Rectangle caja = { (float)x, (float)margen, (float)anchoPanel, (float)altoPanel };

        DrawRectangleRounded(caja, 0.2f, 6, Fade(Color{ 18, 22, 30, 255 }, 0.66f));
        DrawRectangleRoundedLinesEx(caja, 0.2f, 6, 2.0f, definicion->colorTema);

        DibujarTextoHudTablero(textoRonda, x + (int)(14 * esc), margen + (int)(7 * esc), tRonda, RAYWHITE);

        // Nombre del tablero y zona donde esta el jugador activo.
        const char* textoTablero = definicion->nombre;
        int turnoHud = ObtenerParticipanteTurnoTableroFinal(*this);

        if (
            turnoHud >= 0 &&
            definicion->nombresZonas != nullptr
        )
        {
            const Casilla* casillaHud =
                tablero.ObtenerCasilla(jugadores[turnoHud].casillaActual);

            if (
                casillaHud != nullptr &&
                casillaHud->zona >= 0 &&
                casillaHud->zona < definicion->cantidadZonas
            )
            {
                textoTablero =
                    TextFormat(
                        "%s - %s",
                        definicion->nombre,
                        definicion->nombresZonas[casillaHud->zona]
                    );
            }
        }

        DibujarTextoHudTablero(
            textoTablero,
            x + anchoPanel - MeasureText(textoTablero, tPeq) - (int)(12 * esc),
            margen + (int)(34 * esc), tPeq, ColorBrightness(definicion->colorTema, 0.5f)
        );

        if (estadoGimmick[0] != '\0')
        {
            DibujarTextoHudTablero(
                estadoGimmick,
                x + (int)(14 * esc), margen + (int)(55 * esc), tGim,
                Color{ 255, 226, 150, 255 }
            );
        }
    }

    // Aviso temporal del gimmick (centro, bajo el mapa).
    if (tiempoMensajeGimmick > 0.0f)
    {
        int tamano = (int)(22 * esc);
        int anchoAviso = MeasureText(mensajeGimmick, tamano) + (int)(44 * esc);
        int altoAviso = (int)(40 * esc);
        int xAviso = ancho / 2 - anchoAviso / 2;
        int yAviso = alto - (int)(130 * esc);

        Rectangle caja = { (float)xAviso, (float)yAviso, (float)anchoAviso, (float)altoAviso };

        DrawRectangleRounded(caja, 0.4f, 6, Fade(BLACK, 0.74f));
        DrawRectangleRoundedLinesEx(caja, 0.4f, 6, 2.0f, definicion->colorTema);

        DrawText(
            mensajeGimmick,
            xAviso + (int)(22 * esc),
            yAviso + (int)(9 * esc),
            tamano,
            RAYWHITE
        );
    }

    // Instruccion de fase: banda inferior centrada.
    {
        const char* fraseFase = ObtenerTextoFaseTableroFinal(*this);

        int tamano = (int)(20 * esc);
        int anchoTexto = MeasureText(fraseFase, tamano);
        int maximoAncho = ancho - (int)(60 * esc);

        if (anchoTexto > maximoAncho)
        {
            tamano = (int)(tamano * (float)maximoAncho / anchoTexto);
            anchoTexto = MeasureText(fraseFase, tamano);
        }

        int anchoBanda = anchoTexto + (int)(44 * esc);
        int altoBanda = (int)(40 * esc);
        int x = ancho / 2 - anchoBanda / 2;
        int y = alto - altoBanda - (int)(30 * esc);

        Rectangle caja = { (float)x, (float)y, (float)anchoBanda, (float)altoBanda };

        DrawRectangleRounded(caja, 0.4f, 6, Fade(Color{ 18, 22, 30, 255 }, 0.74f));

        DrawText(
            fraseFase,
            x + (int)(22 * esc),
            y + (altoBanda - tamano) / 2,
            tamano,
            RAYWHITE
        );
    }

    // Dado.
    if (valorDado > 0)
    {
        int lado = (int)(76 * esc);
        int x = ancho - lado - margen;
        int y = (int)(100 * esc) + (estadoGimmick[0] != '\0' ? (int)(18 * esc) : 0);

        Rectangle caja = { (float)x, (float)y, (float)lado, (float)lado };

        DrawRectangleRounded(caja, 0.2f, 6, RAYWHITE);
        DrawRectangleRoundedLinesEx(caja, 0.2f, 6, 3.0f, Color{ 30, 30, 40, 255 });

        const char* textoDado = TextFormat("%d", valorDado);
        int tamano = (int)(46 * esc);

        DrawText(
            textoDado,
            x + lado / 2 - MeasureText(textoDado, tamano) / 2,
            y + (lado - tamano) / 2,
            tamano,
            BLACK
        );
    }

    // Leyenda de casillas (esquina inferior derecha) y ayuda de pausa.
    {
        int tamano = (int)(13 * esc);
        int y = alto - (int)(22 * esc);
        int x = ancho - margen;

        const char* leyenda =
            TextFormat("TROFEO %d MONEDAS", costoTrofeo);

        x -= MeasureText(leyenda, tamano);
        DibujarTextoHudTablero(leyenda, x, y, tamano, Color{ 255, 226, 120, 255 });

        const TipoCasilla tipos[3] = { CASILLA_POSITIVA, CASILLA_NEGATIVA, CASILLA_ESPECIAL };
        const char* nombres[3] = { "+3", "-3", "EVENTO" };

        for (int i = 2; i >= 0; i--)
        {
            x -= (int)(14 * esc);
            x -= MeasureText(nombres[i], tamano);
            DibujarTextoHudTablero(nombres[i], x, y, tamano, RAYWHITE);
            x -= (int)(14 * esc);
            DrawCircle(x + (int)(5 * esc), y + (int)(7 * esc), 5.0f * esc, ObtenerColorCasilla(tipos[i]));
        }

        DibujarTextoHudTablero(
            "ESC / START: PAUSA Y SALIR",
            margen, y, tamano, Fade(RAYWHITE, 0.8f)
        );
    }

    DibujarResultadoFinalTablero(*this);
}
