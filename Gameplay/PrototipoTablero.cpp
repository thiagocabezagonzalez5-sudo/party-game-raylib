#include "Gameplay/PrototipoTablero.h"

#include "Systems/Input.h"

#include <cmath>


//==================================================
// CONSTANTES DEL PROTOTIPO
//==================================================

static const float VELOCIDAD_MOVIMIENTO_TABLERO =
    2.2f;

static const float ALTURA_SALTO_FICHA =
    0.75f;

static const float PI_TABLERO =
    3.14159265f;


//==================================================
// UTILIDADES DE POSICION
//==================================================

static Vector3 InterpolarPosicion(
    Vector3 origen,
    Vector3 destino,
    float progreso
)
{
    return Vector3{
        origen.x +
            (destino.x - origen.x) * progreso,

        origen.y +
            (destino.y - origen.y) * progreso,

        origen.z +
            (destino.z - origen.z) * progreso
    };
}


static Vector3 ObtenerDesplazamientoFicha(
    int indiceParticipante
)
{
    const Vector3 desplazamientos[
        MAX_PARTICIPANTES
    ] =
    {
        { -0.32f, 0.0f, -0.32f },
        {  0.32f, 0.0f, -0.32f },
        { -0.32f, 0.0f,  0.32f },
        {  0.32f, 0.0f,  0.32f }
    };

    if (
        indiceParticipante < 0 ||
        indiceParticipante >= MAX_PARTICIPANTES
    )
    {
        return Vector3{};
    }

    return desplazamientos[
        indiceParticipante
    ];
}


static Vector3 ObtenerPosicionFichaEnCasilla(
    const Tablero& tablero,
    int indiceCasilla,
    int indiceParticipante
)
{
    const Casilla* casilla =
        tablero.ObtenerCasilla(
            indiceCasilla
        );

    if (casilla == nullptr)
    {
        return Vector3{};
    }

    Vector3 posicion =
        casilla->posicion;

    Vector3 desplazamiento =
        ObtenerDesplazamientoFicha(
            indiceParticipante
        );

    posicion.x +=
        desplazamiento.x;

    posicion.y +=
        0.75f;

    posicion.z +=
        desplazamiento.z;

    return posicion;
}


//==================================================
// TURNO ACTUAL
//==================================================

static int ObtenerParticipanteTurno(
    const PrototipoTablero& prototipo
)
{
    if (
        prototipo.cantidadJugadores <= 0 ||
        prototipo.indiceOrdenTurno < 0 ||
        prototipo.indiceOrdenTurno >= prototipo.cantidadJugadores
    )
    {
        return -1;
    }

    return prototipo
        .ordenParticipantes[
            prototipo.indiceOrdenTurno
        ];
}


static bool HayControlConectado(
    const PrototipoTablero& prototipo
)
{
    if (prototipo.participantes == nullptr)
    {
        return false;
    }

    for (
        int i = 0;
        i < prototipo.cantidadJugadores;
        i++
    )
    {
        int indiceParticipante =
            prototipo.ordenParticipantes[i];

        if (
            prototipo.participantes[indiceParticipante].activo &&
            prototipo.participantes[indiceParticipante].conectado
        )
        {
            return true;
        }
    }

    return false;
}


static void AvanzarTurno(
    PrototipoTablero& prototipo
)
{
    if (prototipo.cantidadJugadores <= 0)
    {
        return;
    }

    prototipo.indiceOrdenTurno++;

    if (
        prototipo.indiceOrdenTurno >=
        prototipo.cantidadJugadores
    )
    {
        prototipo.indiceOrdenTurno =
            0;

        prototipo.rondaActual++;

        if (
            prototipo.rondaActual >
            prototipo.cantidadRondas
        )
        {
            prototipo.fase =
                FASE_TABLERO_PARTIDA_TERMINADA;

            prototipo.tiempoFase =
                0.0f;

            return;
        }
    }

    prototipo.valorDado =
        0;

    prototipo.pasosPendientes =
        0;

    prototipo.opcionRuta =
        0;

    prototipo.direccionRutaBloqueada =
        false;

    prototipo.fase =
        FASE_TABLERO_ESPERANDO_DADO;

    prototipo.tiempoFase =
        0.0f;
}


static void OmitirControlesDesconectados(
    PrototipoTablero& prototipo
)
{
    if (
        prototipo.participantes == nullptr ||
        !HayControlConectado(prototipo)
    )
    {
        return;
    }

    for (
        int intento = 0;
        intento < prototipo.cantidadJugadores;
        intento++
    )
    {
        int indiceParticipante =
            ObtenerParticipanteTurno(
                prototipo
            );

        if (
            indiceParticipante >= 0 &&
            prototipo.participantes[indiceParticipante].conectado
        )
        {
            return;
        }

        AvanzarTurno(
            prototipo
        );

        if (
            prototipo.fase ==
            FASE_TABLERO_PARTIDA_TERMINADA
        )
        {
            return;
        }
    }
}


//==================================================
// MOVIMIENTO
//==================================================

static void IniciarMovimientoHacia(
    PrototipoTablero& prototipo,
    int indiceDestino
)
{
    int indiceParticipante =
        ObtenerParticipanteTurno(
            prototipo
        );

    const Casilla* destino =
        prototipo.tablero.ObtenerCasilla(
            indiceDestino
        );

    if (
        indiceParticipante < 0 ||
        destino == nullptr
    )
    {
        prototipo.fase =
            FASE_TABLERO_FIN_TURNO;

        prototipo.tiempoFase =
            0.0f;

        return;
    }

    EstadoJugadorTablero& jugador =
        prototipo.jugadores[
            indiceParticipante
        ];

    prototipo.casillaDestinoMovimiento =
        indiceDestino;

    prototipo.posicionInicioMovimiento =
        jugador.posicionVisual;

    prototipo.posicionFinMovimiento =
        ObtenerPosicionFichaEnCasilla(
            prototipo.tablero,
            indiceDestino,
            indiceParticipante
        );

    prototipo.progresoMovimiento =
        0.0f;

    prototipo.fase =
        FASE_TABLERO_MOVIENDO;

    prototipo.tiempoFase =
        0.0f;
}


static void PrepararSiguientePaso(
    PrototipoTablero& prototipo
)
{
    int indiceParticipante =
        ObtenerParticipanteTurno(
            prototipo
        );

    if (indiceParticipante < 0)
    {
        return;
    }

    EstadoJugadorTablero& jugador =
        prototipo.jugadores[
            indiceParticipante
        ];

    const Casilla* casilla =
        prototipo.tablero.ObtenerCasilla(
            jugador.casillaActual
        );

    if (
        casilla == nullptr ||
        casilla->cantidadConexiones <= 0
    )
    {
        prototipo.fase =
            FASE_TABLERO_FIN_TURNO;

        prototipo.tiempoFase =
            0.0f;

        return;
    }

    if (casilla->cantidadConexiones > 1)
    {
        prototipo.opcionRuta =
            0;

        prototipo.direccionRutaBloqueada =
            false;

        prototipo.fase =
            FASE_TABLERO_ELIGIENDO_RUTA;

        prototipo.tiempoFase =
            0.0f;

        return;
    }

    IniciarMovimientoHacia(
        prototipo,
        casilla->conexiones[0].destino
    );
}


static void ActualizarMovimiento(
    PrototipoTablero& prototipo,
    float deltaTime
)
{
    int indiceParticipante =
        ObtenerParticipanteTurno(
            prototipo
        );

    if (indiceParticipante < 0)
    {
        return;
    }

    EstadoJugadorTablero& jugador =
        prototipo.jugadores[
            indiceParticipante
        ];

    prototipo.progresoMovimiento +=
        VELOCIDAD_MOVIMIENTO_TABLERO *
        deltaTime;

    float progreso =
        prototipo.progresoMovimiento;

    if (progreso > 1.0f)
    {
        progreso =
            1.0f;
    }

    float progresoSuave =
        progreso * progreso *
        (3.0f - 2.0f * progreso);

    jugador.posicionVisual =
        InterpolarPosicion(
            prototipo.posicionInicioMovimiento,
            prototipo.posicionFinMovimiento,
            progresoSuave
        );

    jugador.posicionVisual.y +=
        std::sin(
            progreso * PI_TABLERO
        ) * ALTURA_SALTO_FICHA;

    if (progreso < 1.0f)
    {
        return;
    }

    jugador.casillaActual =
        prototipo.casillaDestinoMovimiento;

    jugador.posicionVisual =
        prototipo.posicionFinMovimiento;

    prototipo.pasosPendientes--;

    if (prototipo.pasosPendientes > 0)
    {
        PrepararSiguientePaso(
            prototipo
        );

        return;
    }

    prototipo.fase =
        FASE_TABLERO_EVENTO_CASILLA;

    prototipo.tiempoFase =
        0.0f;

    prototipo.eventoCasillaResuelto =
        false;
}


//==================================================
// EVENTO DE CASILLA
//==================================================

static void ResolverEventoCasilla(
    PrototipoTablero& prototipo
)
{
    int indiceParticipante =
        ObtenerParticipanteTurno(
            prototipo
        );

    if (indiceParticipante < 0)
    {
        return;
    }

    EstadoJugadorTablero& jugador =
        prototipo.jugadores[
            indiceParticipante
        ];

    const Casilla* casilla =
        prototipo.tablero.ObtenerCasilla(
            jugador.casillaActual
        );

    if (casilla == nullptr)
    {
        return;
    }

    if (casilla->tipo == CASILLA_POSITIVA)
    {
        jugador.puntos +=
            3;
    }
    else if (casilla->tipo == CASILLA_NEGATIVA)
    {
        jugador.puntos -=
            2;

        if (jugador.puntos < 0)
        {
            jugador.puntos =
                0;
        }
    }
}


//==================================================
// INICIALIZAR
//==================================================

void PrototipoTablero::Inicializar(
    Participante participantesJuego[],
    int cantidadParticipantesJuego
)
{
    participantes =
        participantesJuego;

    tablero.InicializarPrototipo();

    camara.position =
    {
        0.0f,
        17.0f,
        18.0f
    };

    camara.target =
    {
        0.0f,
        0.0f,
        0.0f
    };

    camara.up =
    {
        0.0f,
        1.0f,
        0.0f
    };

    camara.fovy =
        48.0f;

    camara.projection =
        CAMERA_PERSPECTIVE;

    (void)cantidadParticipantesJuego;

    inicializado =
        true;

    Reiniciar();
}


//==================================================
// REINICIAR
//==================================================

void PrototipoTablero::Reiniciar()
{
    if (!inicializado)
    {
        return;
    }

    cantidadJugadores =
        0;

    indiceOrdenTurno =
        0;

    rondaActual =
        1;

    cantidadRondas =
        5;

    valorDado =
        0;

    pasosPendientes =
        0;

    opcionRuta =
        0;

    direccionRutaBloqueada =
        false;

    casillaDestinoMovimiento =
        -1;

    progresoMovimiento =
        0.0f;

    tiempoFase =
        0.0f;

    eventoCasillaResuelto =
        false;

    fase =
        FASE_TABLERO_ESPERANDO_DADO;

    for (
        int i = 0;
        i < MAX_PARTICIPANTES;
        i++
    )
    {
        ordenParticipantes[i] =
            -1;

        jugadores[i] =
            EstadoJugadorTablero{};
    }

    if (participantes == nullptr)
    {
        return;
    }

    cantidadJugadores =
        ObtenerIndicesParticipantesActivos(
            participantes,
            ordenParticipantes,
            MAX_PARTICIPANTES
        );

    for (
        int i = 0;
        i < cantidadJugadores;
        i++
    )
    {
        int indiceParticipante =
            ordenParticipantes[i];

        EstadoJugadorTablero& jugador =
            jugadores[indiceParticipante];

        jugador.participa =
            true;

        jugador.casillaActual =
            0;

        jugador.puntos =
            0;

        jugador.posicionVisual =
            ObtenerPosicionFichaEnCasilla(
                tablero,
                0,
                indiceParticipante
            );
    }

    if (
        cantidadJugadores <= 0 ||
        !tablero.recorridoValido
    )
    {
        fase =
            FASE_TABLERO_PARTIDA_TERMINADA;
    }
}


//==================================================
// ACTUALIZAR
//==================================================

void PrototipoTablero::Actualizar(
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

    tiempoFase +=
        deltaTime;

    if (
        fase ==
        FASE_TABLERO_PARTIDA_TERMINADA
    )
    {
        return;
    }

    int indiceParticipante =
        ObtenerParticipanteTurno(
            *this
        );

    if (indiceParticipante < 0)
    {
        return;
    }

    bool controlConectado =
        participantes[
            indiceParticipante
        ]
        .conectado;

    if (
        fase == FASE_TABLERO_ESPERANDO_DADO &&
        !controlConectado
    )
    {
        OmitirControlesDesconectados(
            *this
        );

        return;
    }

    if (
        fase == FASE_TABLERO_ESPERANDO_DADO
    )
    {
        InputSeleccionParticipante entrada =
            LeerInputSeleccionParticipante(
                participantes[indiceParticipante]
            );

        if (entrada.confirmar)
        {
            valorDado =
                GetRandomValue(
                    1,
                    10
                );

            pasosPendientes =
                valorDado;

            fase =
                FASE_TABLERO_MOSTRANDO_DADO;

            tiempoFase =
                0.0f;
        }
    }
    else if (
        fase == FASE_TABLERO_MOSTRANDO_DADO
    )
    {
        if (tiempoFase >= 0.75f)
        {
            PrepararSiguientePaso(
                *this
            );
        }
    }
    else if (
        fase == FASE_TABLERO_ELIGIENDO_RUTA
    )
    {
        EstadoJugadorTablero& jugador =
            jugadores[indiceParticipante];

        const Casilla* casilla =
            tablero.ObtenerCasilla(
                jugador.casillaActual
            );

        if (
            casilla == nullptr ||
            casilla->cantidadConexiones <= 1
        )
        {
            PrepararSiguientePaso(
                *this
            );

            return;
        }

        if (!controlConectado)
        {
            opcionRuta =
                0;

            IniciarMovimientoHacia(
                *this,
                casilla->conexiones[0].destino
            );

            return;
        }

        InputSeleccionParticipante entrada =
            LeerInputSeleccionParticipante(
                participantes[indiceParticipante]
            );

        bool direccionPresionada =
            entrada.izquierda ||
            entrada.derecha ||
            entrada.arriba ||
            entrada.abajo;

        if (
            !direccionRutaBloqueada &&
            (
                entrada.izquierda ||
                entrada.arriba
            )
        )
        {
            opcionRuta--;
        }

        if (
            !direccionRutaBloqueada &&
            (
                entrada.derecha ||
                entrada.abajo
            )
        )
        {
            opcionRuta++;
        }

        direccionRutaBloqueada =
            direccionPresionada;

        if (opcionRuta < 0)
        {
            opcionRuta =
                casilla->cantidadConexiones - 1;
        }

        if (
            opcionRuta >=
            casilla->cantidadConexiones
        )
        {
            opcionRuta =
                0;
        }

        if (entrada.confirmar)
        {
            IniciarMovimientoHacia(
                *this,
                casilla
                    ->conexiones[opcionRuta]
                    .destino
            );
        }
    }
    else if (
        fase == FASE_TABLERO_MOVIENDO
    )
    {
        ActualizarMovimiento(
            *this,
            deltaTime
        );
    }
    else if (
        fase == FASE_TABLERO_EVENTO_CASILLA
    )
    {
        if (!eventoCasillaResuelto)
        {
            ResolverEventoCasilla(
                *this
            );

            eventoCasillaResuelto =
                true;
        }

        if (tiempoFase >= 1.20f)
        {
            fase =
                FASE_TABLERO_FIN_TURNO;

            tiempoFase =
                0.0f;
        }
    }
    else if (
        fase == FASE_TABLERO_FIN_TURNO
    )
    {
        if (tiempoFase >= 0.35f)
        {
            AvanzarTurno(
                *this
            );
        }
    }
}


//==================================================
// DIBUJAR FICHAS
//==================================================

static void DibujarFichas(
    const PrototipoTablero& prototipo
)
{
    int participanteTurno =
        ObtenerParticipanteTurno(
            prototipo
        );

    for (
        int i = 0;
        i < prototipo.cantidadJugadores;
        i++
    )
    {
        int indiceParticipante =
            prototipo.ordenParticipantes[i];

        const EstadoJugadorTablero& jugador =
            prototipo.jugadores[
                indiceParticipante
            ];

        Color color =
            prototipo
                .participantes[indiceParticipante]
                .color;

        if (
            !prototipo
                .participantes[indiceParticipante]
                .conectado
        )
        {
            color =
                Fade(
                    color,
                    0.45f
                );
        }

        Vector3 cuerpo =
            jugador.posicionVisual;

        DrawCube(
            cuerpo,
            0.48f,
            0.62f,
            0.48f,
            color
        );

        DrawCubeWires(
            cuerpo,
            0.48f,
            0.62f,
            0.48f,
            BLACK
        );

        Vector3 cabeza =
            cuerpo;

        cabeza.y +=
            0.48f;

        DrawSphere(
            cabeza,
            0.26f,
            color
        );

        if (
            indiceParticipante == participanteTurno &&
            prototipo.fase != FASE_TABLERO_PARTIDA_TERMINADA
        )
        {
            DrawSphereWires(
                cuerpo,
                0.55f,
                8,
                8,
                YELLOW
            );
        }
    }
}


//==================================================
// DIBUJAR RUTA SELECCIONADA
//==================================================

static void DibujarSeleccionRuta(
    const PrototipoTablero& prototipo
)
{
    if (
        prototipo.fase !=
        FASE_TABLERO_ELIGIENDO_RUTA
    )
    {
        return;
    }

    int indiceParticipante =
        ObtenerParticipanteTurno(
            prototipo
        );

    if (indiceParticipante < 0)
    {
        return;
    }

    const EstadoJugadorTablero& jugador =
        prototipo.jugadores[
            indiceParticipante
        ];

    const Casilla* origen =
        prototipo.tablero.ObtenerCasilla(
            jugador.casillaActual
        );

    if (
        origen == nullptr ||
        prototipo.opcionRuta < 0 ||
        prototipo.opcionRuta >= origen->cantidadConexiones
    )
    {
        return;
    }

    const Casilla* destino =
        prototipo.tablero.ObtenerCasilla(
            origen
                ->conexiones[prototipo.opcionRuta]
                .destino
        );

    if (destino == nullptr)
    {
        return;
    }

    Vector3 marcador =
        destino->posicion;

    marcador.y +=
        0.14f;

    DrawCubeWires(
        marcador,
        1.85f,
        0.75f,
        1.85f,
        YELLOW
    );
}


//==================================================
// TEXTO DE LA FASE
//==================================================

static int ObtenerGanadorUnico(
    const PrototipoTablero& prototipo
)
{
    int ganador =
        -1;

    int mejorPuntaje =
        -1;

    bool hayEmpate =
        false;

    for (
        int i = 0;
        i < prototipo.cantidadJugadores;
        i++
    )
    {
        int indiceParticipante =
            prototipo.ordenParticipantes[i];

        int puntos =
            prototipo
                .jugadores[indiceParticipante]
                .puntos;

        if (puntos > mejorPuntaje)
        {
            mejorPuntaje =
                puntos;

            ganador =
                indiceParticipante;

            hayEmpate =
                false;
        }
        else if (puntos == mejorPuntaje)
        {
            hayEmpate =
                true;
        }
    }

    return hayEmpate
        ? -1
        : ganador;
}


static const char* ObtenerTextoFase(
    const PrototipoTablero& prototipo
)
{
    int indiceParticipante =
        ObtenerParticipanteTurno(
            prototipo
        );

    if (
        prototipo.fase ==
        FASE_TABLERO_PARTIDA_TERMINADA
    )
    {
        int ganador =
            ObtenerGanadorUnico(
                prototipo
            );

        if (ganador >= 0)
        {
            return TextFormat(
                "GANA JUGADOR %d CON %d PUNTOS - R REINICIA",
                ganador + 1,
                prototipo.jugadores[ganador].puntos
            );
        }

        return "EMPATE - R PARA REINICIAR";
    }

    if (
        indiceParticipante < 0 ||
        prototipo.participantes == nullptr
    )
    {
        return "NO HAY JUGADORES ACTIVOS";
    }

    if (!HayControlConectado(prototipo))
    {
        return "ESPERANDO QUE SE RECONECTE UN CONTROL";
    }

    if (
        prototipo.fase ==
        FASE_TABLERO_ESPERANDO_DADO
    )
    {
        return TextFormat(
            "JUGADOR %d: PRESIONA %s PARA LANZAR",
            indiceParticipante + 1,
            ObtenerTextoBotonPrincipal(
                prototipo.participantes[indiceParticipante]
            )
        );
    }

    if (
        prototipo.fase ==
        FASE_TABLERO_MOSTRANDO_DADO
    )
    {
        return TextFormat(
            "JUGADOR %d SACO %d",
            indiceParticipante + 1,
            prototipo.valorDado
        );
    }

    if (
        prototipo.fase ==
        FASE_TABLERO_ELIGIENDO_RUTA
    )
    {
        return TextFormat(
            "JUGADOR %d: ELEGIR RUTA %d",
            indiceParticipante + 1,
            prototipo.opcionRuta + 1
        );
    }

    if (
        prototipo.fase ==
        FASE_TABLERO_MOVIENDO
    )
    {
        return TextFormat(
            "MOVIENDO... %d PASOS RESTANTES",
            prototipo.pasosPendientes
        );
    }

    if (
        prototipo.fase ==
        FASE_TABLERO_EVENTO_CASILLA
    )
    {
        const EstadoJugadorTablero& jugador =
            prototipo.jugadores[
                indiceParticipante
            ];

        const Casilla* casilla =
            prototipo.tablero.ObtenerCasilla(
                jugador.casillaActual
            );

        if (casilla != nullptr)
        {
            if (casilla->tipo == CASILLA_POSITIVA)
            {
                return "CASILLA POSITIVA: +3 PUNTOS";
            }

            if (casilla->tipo == CASILLA_NEGATIVA)
            {
                return "CASILLA NEGATIVA: -2 PUNTOS";
            }

            if (casilla->tipo == CASILLA_ESPECIAL)
            {
                return "CASILLA ESPECIAL: EVENTO RESERVADO";
            }

            return "CASILLA NEUTRA: SIN CAMBIOS";
        }
    }

    return "FINALIZANDO TURNO...";
}


//==================================================
// DIBUJAR MARCADOR
//==================================================

static void DibujarMarcador(
    const PrototipoTablero& prototipo
)
{
    int margen =
        18;

    int separacion =
        10;

    int anchoDisponible =
        GetScreenWidth() -
        margen * 2 -
        separacion *
        (prototipo.cantidadJugadores - 1);

    int anchoTarjeta =
        prototipo.cantidadJugadores > 0
        ? anchoDisponible /
            prototipo.cantidadJugadores
        : anchoDisponible;

    int participanteTurno =
        ObtenerParticipanteTurno(
            prototipo
        );

    for (
        int i = 0;
        i < prototipo.cantidadJugadores;
        i++
    )
    {
        int indiceParticipante =
            prototipo.ordenParticipantes[i];

        int x =
            margen +
            i *
            (anchoTarjeta + separacion);

        Color color =
            prototipo
                .participantes[indiceParticipante]
                .color;

        DrawRectangle(
            x,
            16,
            anchoTarjeta,
            64,
            Fade(
                color,
                indiceParticipante == participanteTurno
                ? 0.92f
                : 0.68f
            )
        );

        DrawRectangleLinesEx(
            Rectangle{
                static_cast<float>(x),
                16.0f,
                static_cast<float>(anchoTarjeta),
                64.0f
            },
            indiceParticipante == participanteTurno
            ? 4.0f
            : 2.0f,
            indiceParticipante == participanteTurno
            ? YELLOW
            : BLACK
        );

        DrawText(
            TextFormat(
                "J%d   %d PTS",
                indiceParticipante + 1,
                prototipo
                    .jugadores[indiceParticipante]
                    .puntos
            ),
            x + 12,
            26,
            22,
            BLACK
        );

        DrawText(
            prototipo
                .participantes[indiceParticipante]
                .conectado
            ? ObtenerNombreControlParticipante(
                prototipo.participantes[indiceParticipante]
            )
            : "CONTROL DESCONECTADO",
            x + 12,
            52,
            14,
            DARKGRAY
        );
    }
}


//==================================================
// DIBUJAR
//==================================================

void PrototipoTablero::Dibujar(
    bool mostrarDebug
) const
{
    ClearBackground(
        Color{
            112,
            178,
            202,
            255
        }
    );

    BeginMode3D(
        camara
    );

    tablero.Dibujar();

    DibujarSeleccionRuta(
        *this
    );

    if (
        participantes != nullptr &&
        cantidadJugadores > 0
    )
    {
        DibujarFichas(
            *this
        );
    }

    if (mostrarDebug)
    {
        DrawGrid(
            30,
            1.0f
        );
    }

    EndMode3D();

    if (
        !inicializado ||
        !tablero.recorridoValido
    )
    {
        DrawRectangle(
            20,
            20,
            610,
            90,
            Fade(
                MAROON,
                0.90f
            )
        );

        DrawText(
            "ERROR: EL RECORRIDO DEL TABLERO NO ES VALIDO",
            38,
            48,
            22,
            RAYWHITE
        );

        return;
    }

    DibujarMarcador(
        *this
    );

    DrawRectangle(
        18,
        94,
        525,
        112,
        Fade(
            RAYWHITE,
            0.91f
        )
    );

    DrawRectangleLinesEx(
        Rectangle{
            18.0f,
            94.0f,
            525.0f,
            112.0f
        },
        2.0f,
        DARKBLUE
    );

    DrawText(
        TextFormat(
            "RONDA %d / %d",
            rondaActual > cantidadRondas
            ? cantidadRondas
            : rondaActual,
            cantidadRondas
        ),
        34,
        108,
        23,
        DARKBLUE
    );

    DrawText(
        ObtenerTextoFase(
            *this
        ),
        34,
        140,
        19,
        BLACK
    );

    if (
        fase ==
        FASE_TABLERO_ELIGIENDO_RUTA
    )
    {
        DrawText(
            "IZQUIERDA/DERECHA + CONFIRMAR",
            34,
            173,
            17,
            DARKGRAY
        );
    }
    else
    {
        DrawText(
            "VERDE +3   ROJA -2   AZUL NEUTRA",
            34,
            173,
            17,
            DARKGRAY
        );
    }

    if (valorDado > 0)
    {
        int tamanoDado =
            104;

        int xDado =
            GetScreenWidth() -
            tamanoDado -
            24;

        DrawRectangle(
            xDado,
            96,
            tamanoDado,
            tamanoDado,
            RAYWHITE
        );

        DrawRectangleLinesEx(
            Rectangle{
                static_cast<float>(xDado),
                96.0f,
                static_cast<float>(tamanoDado),
                static_cast<float>(tamanoDado)
            },
            4.0f,
            BLACK
        );

        const char* textoDado =
            TextFormat(
                "%d",
                valorDado
            );

        int anchoTexto =
            MeasureText(
                textoDado,
                58
            );

        DrawText(
            textoDado,
            xDado +
                tamanoDado / 2 -
                anchoTexto / 2,
            116,
            58,
            BLACK
        );
    }

    if (mostrarDebug)
    {
        DrawText(
            TextFormat(
                "DEBUG: %d CASILLAS - RECORRIDO %s",
                tablero.cantidadCasillas,
                tablero.recorridoValido
                ? "VALIDO"
                : "INVALIDO"
            ),
            24,
            220,
            18,
            YELLOW
        );
    }
}
