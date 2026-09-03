#include "Minigames/MinijuegoTronco.h"

#include "Minigames/UtilidadesMinijuegos.h"


//==================================================
// CONSTANTES
//==================================================

static const float DURACION_PREPARACION_TRONCO =
    2.5f;

static const float DURACION_PARTIDA_TRONCO =
    60.0f;

static const float VENTANA_COORDINACION_TRONCO =
    0.34f;

static const float BLOQUEO_FALLO_TRONCO =
    0.26f;

static const float AVANCE_CORTE_TRONCO =
    0.065f;

static const float VELOCIDAD_SIERRA_VISUAL_TRONCO =
    5.4f;

static const float CENTRO_EQUIPO_TRONCO[2] =
{
    -3.9f,
    3.9f
};


//==================================================
// UTILIDADES DE ESTADO
//==================================================

static int LimitarCantidadTronco(
    int cantidadMaxima
)
{
    return cantidadMaxima < MAX_JUGADORES_PRUEBA
        ? cantidadMaxima
        : MAX_JUGADORES_PRUEBA;
}


static int ContarParticipantesActivosTronco(
    const Participante participantes[],
    int cantidadMaxima
)
{
    int cantidad = 0;
    int limite = LimitarCantidadTronco(cantidadMaxima);

    for (int i = 0; i < limite; i++)
    {
        if (participantes[i].activo)
        {
            cantidad++;
        }
    }

    return cantidad;
}


static int ContarJugadoresConectadosTronco(
    const Participante participantes[],
    int cantidadMaxima
)
{
    int cantidad = 0;
    int limite = LimitarCantidadTronco(cantidadMaxima);

    for (int i = 0; i < limite; i++)
    {
        if (
            participantes[i].activo &&
            participantes[i].conectado
        )
        {
            cantidad++;
        }
    }

    return cantidad;
}


static AccionDireccionalControl ObtenerAccionControlTronco(
    AccionTronco accion
)
{
    return accion == ACCION_TRONCO_TIRAR
        ? CONTROL_DIRECCION_IZQUIERDA
        : CONTROL_DIRECCION_DERECHA;
}


static bool AccionJugadorPresionadaTronco(
    const Participante& participante,
    AccionTronco accion
)
{
    return AccionDireccionalControlPresionada(
        participante,
        ObtenerAccionControlTronco(accion)
    );
}


static bool OtraAccionJugadorPresionadaTronco(
    const Participante& participante,
    AccionTronco accionEsperada
)
{
    AccionTronco otraAccion =
        accionEsperada == ACCION_TRONCO_TIRAR
        ? ACCION_TRONCO_EMPUJAR
        : ACCION_TRONCO_TIRAR;

    return AccionJugadorPresionadaTronco(
        participante,
        otraAccion
    );
}


static const char* TextoAccionTronco(
    const Participante& participante,
    AccionTronco accion
)
{
    return ObtenerTextoAccionDireccionalControl(
        participante,
        ObtenerAccionControlTronco(accion)
    );
}


static const char* NombreAccionTronco(
    AccionTronco accion
)
{
    return accion == ACCION_TRONCO_TIRAR
        ? "TIRAR"
        : "EMPUJAR";
}


static const char* TextoDispositivoTronco(
    const Participante& participante
)
{
    return ObtenerNombreControlParticipante(
        participante
    );
}


static AccionTronco ObtenerAccionEsperadaTronco(
    const MinijuegoTronco& minijuego,
    int indiceJugador
)
{
    int equipo =
        minijuego.equipoPorJugador[indiceJugador];

    int orden =
        minijuego.ordenEnEquipoPorJugador[indiceJugador];

    if (
        equipo < 0 ||
        equipo >= 2 ||
        orden < 0
    )
    {
        return ACCION_TRONCO_TIRAR;
    }

    int direccion =
        minijuego.equipos[equipo].direccionSierra;

    if (minijuego.cantidadJugadoresEquipo[equipo] <= 1)
    {
        return direccion > 0
            ? ACCION_TRONCO_EMPUJAR
            : ACCION_TRONCO_TIRAR;
    }

    if (direccion > 0)
    {
        return orden == 0
            ? ACCION_TRONCO_EMPUJAR
            : ACCION_TRONCO_TIRAR;
    }

    return orden == 0
        ? ACCION_TRONCO_TIRAR
        : ACCION_TRONCO_EMPUJAR;
}


static void LimpiarIntentoEquipoTronco(
    MinijuegoTronco& minijuego,
    int equipo
)
{
    int limite = MAX_JUGADORES_PRUEBA;

    for (int i = 0; i < limite; i++)
    {
        if (minijuego.equipoPorJugador[i] != equipo)
        {
            continue;
        }

        minijuego.estadosJugadores[i].respondio = false;
        minijuego.estadosJugadores[i].acerto = false;
    }

    minijuego.equipos[equipo].golpeEnCurso = false;
    minijuego.equipos[equipo].tiempoCoordinacion = 0.0f;
}


static void FallarGolpeEquipoTronco(
    MinijuegoTronco& minijuego,
    int equipo
)
{
    EstadoEquipoTronco& estadoEquipo =
        minijuego.equipos[equipo];

    estadoEquipo.ultimoGolpeCorrecto = false;
    estadoEquipo.tiempoFeedback = 0.38f;
    estadoEquipo.tiempoBloqueo = BLOQUEO_FALLO_TRONCO;

    LimpiarIntentoEquipoTronco(
        minijuego,
        equipo
    );
}


static void CompletarGolpeEquipoTronco(
    MinijuegoTronco& minijuego,
    int equipo
)
{
    EstadoEquipoTronco& estadoEquipo =
        minijuego.equipos[equipo];

    int direccionAnterior =
        estadoEquipo.direccionSierra;

    estadoEquipo.progresoCorte +=
        AVANCE_CORTE_TRONCO;

    if (estadoEquipo.progresoCorte > 1.0f)
    {
        estadoEquipo.progresoCorte = 1.0f;
    }

    estadoEquipo.objetivoSierra =
        direccionAnterior > 0
        ? 0.72f
        : -0.72f;

    estadoEquipo.direccionSierra =
        -direccionAnterior;

    estadoEquipo.ultimoGolpeCorrecto = true;
    estadoEquipo.tiempoFeedback = 0.28f;
    estadoEquipo.tiempoBloqueo = 0.06f;

    LimpiarIntentoEquipoTronco(
        minijuego,
        equipo
    );
}


static void FinalizarResultadoTronco(
    MinijuegoTronco& minijuego,
    int equipoGanador,
    bool empate
)
{
    if (
        minijuego.resultado.estado !=
        RESULTADO_MINIJUEGO_EN_CURSO
    )
    {
        return;
    }

    minijuego.estado =
        TRONCO_FINALIZADO;

    minijuego.equipoGanador =
        equipoGanador;

    minijuego.empate =
        empate;

    minijuego.resultado.estado =
        RESULTADO_MINIJUEGO_FINALIZADO;

    minijuego.resultado.desenlace =
        empate
        ? DESENLACE_EMPATE
        : DESENLACE_CON_GANADOR;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        ResultadoParticipante& participante =
            minijuego.resultado.participantes[i];

        if (!participante.participo)
        {
            continue;
        }

        int equipo =
            i < MAX_JUGADORES_PRUEBA
            ? minijuego.equipoPorJugador[i]
            : -1;

        participante.numeroEquipo =
            equipo;

        if (equipo >= 0 && equipo < 2)
        {
            participante.puntuacionMinijuego =
                (int)(
                    minijuego.equipos[equipo]
                        .progresoCorte *
                    1000.0f
                );

            participante.posicionFinal =
                empate || equipo == equipoGanador
                ? 1
                : 2;
        }
        else
        {
            participante.puntuacionMinijuego = 0;
            participante.posicionFinal = 0;
        }

        participante.puntosObtenidos = 0;
    }
}


//==================================================
// INICIALIZAR
//==================================================

void MinijuegoTronco::Inicializar()
{
    camara.position =
    {
        0.0f,
        6.8f,
        13.2f
    };

    camara.target =
    {
        0.0f,
        1.15f,
        -0.35f
    };

    camara.up =
    {
        0.0f,
        1.0f,
        0.0f
    };

    camara.fovy =
        52.0f;

    camara.projection =
        CAMERA_PERSPECTIVE;
}


void MinijuegoTronco::ConfigurarJugadores(
    JugadorPrueba jugadores[],
    int cantidadMaxima
) const
{
    Vector3 spawns[MAX_JUGADORES_PRUEBA] =
    {
        { -5.65f, 0.75f, 0.15f },
        { -2.15f, 0.75f, 0.15f },
        { 2.15f, 0.75f, 0.15f },
        { 5.65f, 0.75f, 0.15f }
    };

    int limite = LimitarCantidadTronco(cantidadMaxima);

    for (int i = 0; i < limite; i++)
    {
        jugadores[i].posicionSpawn =
            spawns[i];

        jugadores[i].tamano =
        {
            0.78f,
            1.45f,
            0.78f
        };

        ReiniciarJugadorPrueba(
            jugadores[i]
        );
    }
}


void MinijuegoTronco::PrepararEquipos(
    const Participante participantes[],
    int cantidadMaxima
)
{
    for (int i = 0; i < MAX_JUGADORES_PRUEBA; i++)
    {
        equipoPorJugador[i] = -1;
        ordenEnEquipoPorJugador[i] = -1;
    }

    cantidadJugadoresEquipo[0] = 0;
    cantidadJugadoresEquipo[1] = 0;

    int cantidadActivos =
        ContarParticipantesActivosTronco(
            participantes,
            cantidadMaxima
        );

    int ordenActivo = 0;
    int limite = LimitarCantidadTronco(cantidadMaxima);

    for (int i = 0; i < limite; i++)
    {
        if (!participantes[i].activo)
        {
            continue;
        }

        int equipo = 0;

        if (cantidadActivos == 2)
        {
            equipo = ordenActivo;
        }
        else
        {
            equipo = ordenActivo < 2
                ? 0
                : 1;
        }

        int ordenEquipo =
            cantidadJugadoresEquipo[equipo];

        equipoPorJugador[i] = equipo;
        ordenEnEquipoPorJugador[i] = ordenEquipo;
        cantidadJugadoresEquipo[equipo]++;

        resultado.participantes[i].numeroEquipo =
            equipo;

        ordenActivo++;
    }
}


void MinijuegoTronco::Reiniciar(
    const Participante participantes[],
    int cantidadMaxima
)
{
    InicializarResultadoMinijuego(
        resultado,
        participantes,
        FORMATO_MINIJUEGO_EQUIPOS
    );

    for (int i = 0; i < MAX_JUGADORES_PRUEBA; i++)
    {
        estadosJugadores[i] =
            EstadoJugadorTronco{};

        equipoPorJugador[i] = -1;
        ordenEnEquipoPorJugador[i] = -1;
    }

    for (int equipo = 0; equipo < 2; equipo++)
    {
        equipos[equipo] =
            EstadoEquipoTronco{};

        cantidadJugadoresEquipo[equipo] = 0;
    }

    int cantidadActivos =
        ContarParticipantesActivosTronco(
            participantes,
            cantidadMaxima
        );

    int cantidadConectados =
        ContarJugadoresConectadosTronco(
            participantes,
            cantidadMaxima
        );

    jugadoresEnPartida =
        cantidadConectados;

    partidaValida =
        (
            cantidadActivos == 2 ||
            cantidadActivos == 4
        ) &&
        cantidadConectados == cantidadActivos;

    equipoGanador = -1;
    empate = false;

    tiempoPreparacion =
        DURACION_PREPARACION_TRONCO;

    tiempoPartida =
        DURACION_PARTIDA_TRONCO;

    if (!partidaValida)
    {
        estado =
            TRONCO_ESPERANDO_JUGADORES;

        resultado.cantidadEquipos = 0;
        return;
    }

    resultado.cantidadEquipos = 2;

    PrepararEquipos(
        participantes,
        cantidadMaxima
    );

    estado =
        TRONCO_PREPARANDO;
}


//==================================================
// ACTUALIZAR
//==================================================

void MinijuegoTronco::Actualizar(
    float deltaTime,
    int cantidadMaxima,
    const Participante participantes[]
)
{
    int cantidadConectados =
        ContarJugadoresConectadosTronco(
            participantes,
            cantidadMaxima
        );

    if (cantidadConectados != jugadoresEnPartida)
    {
        Reiniciar(
            participantes,
            cantidadMaxima
        );

        return;
    }

    for (int i = 0; i < MAX_JUGADORES_PRUEBA; i++)
    {
        if (estadosJugadores[i].animacionGolpe > 0.0f)
        {
            estadosJugadores[i].animacionGolpe -=
                deltaTime;

            if (estadosJugadores[i].animacionGolpe < 0.0f)
            {
                estadosJugadores[i].animacionGolpe = 0.0f;
            }
        }
    }

    for (int equipo = 0; equipo < 2; equipo++)
    {
        EstadoEquipoTronco& estadoEquipo =
            equipos[equipo];

        float diferencia =
            estadoEquipo.objetivoSierra -
            estadoEquipo.posicionSierra;

        float paso =
            VELOCIDAD_SIERRA_VISUAL_TRONCO *
            deltaTime;

        float distancia =
            diferencia < 0.0f
            ? -diferencia
            : diferencia;

        if (distancia <= paso)
        {
            estadoEquipo.posicionSierra =
                estadoEquipo.objetivoSierra;
        }
        else
        {
            estadoEquipo.posicionSierra +=
                diferencia < 0.0f
                ? -paso
                : paso;
        }

        if (estadoEquipo.tiempoFeedback > 0.0f)
        {
            estadoEquipo.tiempoFeedback -=
                deltaTime;

            if (estadoEquipo.tiempoFeedback < 0.0f)
            {
                estadoEquipo.tiempoFeedback = 0.0f;
            }
        }

        if (estadoEquipo.tiempoBloqueo > 0.0f)
        {
            estadoEquipo.tiempoBloqueo -=
                deltaTime;

            if (estadoEquipo.tiempoBloqueo < 0.0f)
            {
                estadoEquipo.tiempoBloqueo = 0.0f;
            }
        }
    }

    if (!partidaValida)
    {
        return;
    }

    if (estado == TRONCO_FINALIZADO)
    {
        return;
    }

    if (estado == TRONCO_PREPARANDO)
    {
        tiempoPreparacion -=
            deltaTime;

        if (tiempoPreparacion <= 0.0f)
        {
            tiempoPreparacion = 0.0f;
            estado = TRONCO_JUGANDO;
        }

        return;
    }

    if (estado != TRONCO_JUGANDO)
    {
        return;
    }

    tiempoPartida -=
        deltaTime;

    if (tiempoPartida <= 0.0f)
    {
        tiempoPartida = 0.0f;

        FinalizarResultadoTronco(
            *this,
            -1,
            true
        );

        return;
    }

    for (int equipo = 0; equipo < 2; equipo++)
    {
        EstadoEquipoTronco& estadoEquipo =
            equipos[equipo];

        if (estadoEquipo.tiempoBloqueo > 0.0f)
        {
            continue;
        }

        bool fallo = false;
        int respuestasCorrectas = 0;
        int limite = LimitarCantidadTronco(cantidadMaxima);

        for (int i = 0; i < limite; i++)
        {
            if (equipoPorJugador[i] != equipo)
            {
                continue;
            }

            EstadoJugadorTronco& estadoJugador =
                estadosJugadores[i];

            if (!estadoJugador.respondio)
            {
                AccionTronco accionEsperada =
                    ObtenerAccionEsperadaTronco(
                        *this,
                        i
                    );

                bool accionCorrecta =
                    AccionJugadorPresionadaTronco(
                        participantes[i],
                        accionEsperada
                    );

                bool accionIncorrecta =
                    OtraAccionJugadorPresionadaTronco(
                        participantes[i],
                        accionEsperada
                    );

                if (accionCorrecta && !accionIncorrecta)
                {
                    estadoJugador.respondio = true;
                    estadoJugador.acerto = true;
                    estadoJugador.animacionGolpe = 0.22f;
                }
                else if (accionIncorrecta)
                {
                    estadoJugador.respondio = true;
                    estadoJugador.acerto = false;
                    fallo = true;
                }
            }

            if (
                estadoJugador.respondio &&
                estadoJugador.acerto
            )
            {
                respuestasCorrectas++;
            }

            if (
                estadoJugador.respondio &&
                !estadoJugador.acerto
            )
            {
                fallo = true;
            }
        }

        if (fallo)
        {
            FallarGolpeEquipoTronco(
                *this,
                equipo
            );

            continue;
        }

        int requeridas =
            cantidadJugadoresEquipo[equipo];

        if (
            requeridas > 0 &&
            respuestasCorrectas >= requeridas
        )
        {
            CompletarGolpeEquipoTronco(
                *this,
                equipo
            );

            if (equipos[equipo].progresoCorte >= 1.0f)
            {
                FinalizarResultadoTronco(
                    *this,
                    equipo,
                    false
                );

                return;
            }

            continue;
        }

        if (respuestasCorrectas > 0)
        {
            if (!estadoEquipo.golpeEnCurso)
            {
                estadoEquipo.golpeEnCurso = true;
                estadoEquipo.tiempoCoordinacion =
                    VENTANA_COORDINACION_TRONCO;
            }

            estadoEquipo.tiempoCoordinacion -=
                deltaTime;

            if (estadoEquipo.tiempoCoordinacion <= 0.0f)
            {
                FallarGolpeEquipoTronco(
                    *this,
                    equipo
                );
            }
        }
    }
}


//==================================================
// DIBUJO 3D
//==================================================

static Color ObtenerColorEquipoTronco(
    const MinijuegoTronco& minijuego,
    const Participante participantes[],
    int equipo
)
{
    for (int i = 0; i < MAX_JUGADORES_PRUEBA; i++)
    {
        if (minijuego.equipoPorJugador[i] == equipo)
        {
            return participantes[i].color;
        }
    }

    return equipo == 0
        ? ORANGE
        : SKYBLUE;
}


static Vector3 ObtenerPosicionJugadorTronco(
    int equipo,
    int ordenEnEquipo,
    int cantidadJugadoresEquipo
)
{
    float centroX =
        CENTRO_EQUIPO_TRONCO[equipo];

    if (cantidadJugadoresEquipo <= 1)
    {
        return
        {
            centroX - 1.85f,
            0.76f,
            0.15f
        };
    }

    return
    {
        centroX +
            (ordenEnEquipo == 0 ? -1.85f : 1.85f),
        0.76f,
        0.15f
    };
}


static void DibujarFondoEquipoTronco(
    int equipo,
    Color colorEquipo
)
{
    float centroX =
        CENTRO_EQUIPO_TRONCO[equipo];

    Color colorPasto =
        equipo == 0
        ? Color{ 91, 150, 75, 255 }
        : Color{ 80, 143, 86, 255 };

    DrawCube(
        { centroX, -0.16f, 0.0f },
        7.45f,
        0.32f,
        9.0f,
        colorPasto
    );

    DrawCube(
        { centroX, 1.85f, -3.55f },
        6.35f,
        3.85f,
        0.28f,
        Color{ 151, 103, 60, 255 }
    );

    DrawCube(
        { centroX, 3.85f, -3.48f },
        6.75f,
        0.34f,
        0.42f,
        Color{ 95, 56, 40, 255 }
    );

    DrawCube(
        { centroX - 1.65f, 2.15f, -3.38f },
        1.25f,
        1.15f,
        0.10f,
        Color{ 61, 94, 114, 255 }
    );

    DrawCubeWires(
        { centroX - 1.65f, 2.15f, -3.38f },
        1.25f,
        1.15f,
        0.10f,
        BLACK
    );

    DrawCube(
        { centroX + 1.55f, 1.55f, -3.36f },
        1.20f,
        2.05f,
        0.12f,
        Color{ 91, 58, 40, 255 }
    );

    DrawCube(
        { centroX, 0.02f, 3.55f },
        5.6f,
        0.12f,
        0.16f,
        Fade(colorEquipo, 0.78f)
    );
}


static void DibujarTroncoEquipo(
    const MinijuegoTronco& minijuego,
    int equipo
)
{
    float centroX =
        CENTRO_EQUIPO_TRONCO[equipo];

    float progreso =
        minijuego.equipos[equipo].progresoCorte;

    DrawCube(
        { centroX, 0.42f, -1.12f },
        0.42f,
        0.82f,
        0.42f,
        Color{ 80, 54, 35, 255 }
    );

    DrawCube(
        { centroX, 0.42f, 1.12f },
        0.42f,
        0.82f,
        0.42f,
        Color{ 80, 54, 35, 255 }
    );

    DrawCylinderEx(
        { centroX, 1.08f, -1.65f },
        { centroX, 1.08f, 1.65f },
        0.72f,
        0.72f,
        24,
        Color{ 130, 78, 42, 255 }
    );

    DrawCylinderEx(
        { centroX, 1.08f, 1.63f },
        { centroX, 1.08f, 1.69f },
        0.63f,
        0.63f,
        24,
        Color{ 210, 157, 88, 255 }
    );

    float altoCorte =
        0.05f +
        progreso * 1.12f;

    DrawCube(
        {
            centroX,
            1.72f - altoCorte * 0.46f,
            0.0f
        },
        1.52f,
        altoCorte,
        0.11f,
        Color{ 70, 39, 24, 255 }
    );
}


static void DibujarSierraEquipo(
    const MinijuegoTronco& minijuego,
    int equipo
)
{
    float centroX =
        CENTRO_EQUIPO_TRONCO[equipo];

    float desplazamiento =
        minijuego.equipos[equipo]
            .posicionSierra *
        0.42f;

    float centroSierra =
        centroX + desplazamiento;

    DrawCube(
        { centroSierra, 1.32f, 0.0f },
        3.05f,
        0.12f,
        0.14f,
        Color{ 196, 201, 205, 255 }
    );

    for (int diente = -5; diente <= 5; diente++)
    {
        DrawCube(
            {
                centroSierra + diente * 0.26f,
                1.245f,
                0.0f
            },
            0.11f,
            0.09f,
            0.15f,
            Color{ 115, 121, 127, 255 }
        );
    }

    DrawCube(
        { centroSierra - 1.68f, 1.38f, 0.0f },
        0.36f,
        0.34f,
        0.28f,
        Color{ 112, 69, 37, 255 }
    );

    DrawCube(
        { centroSierra + 1.68f, 1.38f, 0.0f },
        0.36f,
        0.34f,
        0.28f,
        Color{ 112, 69, 37, 255 }
    );
}


static void DibujarJugadoresEquipoTronco(
    const MinijuegoTronco& minijuego,
    const JugadorPrueba jugadores[],
    const Participante participantes[],
    int cantidadMaxima,
    int equipo
)
{
    int limite = LimitarCantidadTronco(cantidadMaxima);

    for (int i = 0; i < limite; i++)
    {
        if (
            minijuego.equipoPorJugador[i] != equipo ||
            !participantes[i].activo ||
            !participantes[i].conectado
        )
        {
            continue;
        }

        Vector3 posicion =
            ObtenerPosicionJugadorTronco(
                equipo,
                minijuego.ordenEnEquipoPorJugador[i],
                minijuego.cantidadJugadoresEquipo[equipo]
            );

        float direccionCentro =
            posicion.x < CENTRO_EQUIPO_TRONCO[equipo]
            ? 1.0f
            : -1.0f;

        if (minijuego.estadosJugadores[i].animacionGolpe > 0.0f)
        {
            posicion.x +=
                direccionCentro * 0.16f;
        }

        DrawCylinder(
            { posicion.x, 0.08f, posicion.z },
            0.48f,
            0.48f,
            0.14f,
            20,
            Fade(participantes[i].color, 0.70f)
        );

        DrawCube(
            posicion,
            jugadores[i].tamano.x,
            jugadores[i].tamano.y,
            jugadores[i].tamano.z,
            participantes[i].color
        );

        DrawCubeWires(
            posicion,
            jugadores[i].tamano.x,
            jugadores[i].tamano.y,
            jugadores[i].tamano.z,
            BLACK
        );
    }
}


static void DibujarEscenarioTronco(
    const MinijuegoTronco& minijuego,
    const JugadorPrueba jugadores[],
    const Participante participantes[],
    int cantidadMaxima
)
{
    BeginMode3D(
        minijuego.camara
    );

    for (int equipo = 0; equipo < 2; equipo++)
    {
        Color colorEquipo =
            ObtenerColorEquipoTronco(
                minijuego,
                participantes,
                equipo
            );

        DibujarFondoEquipoTronco(
            equipo,
            colorEquipo
        );

        DibujarTroncoEquipo(
            minijuego,
            equipo
        );

        DibujarSierraEquipo(
            minijuego,
            equipo
        );

        DibujarJugadoresEquipoTronco(
            minijuego,
            jugadores,
            participantes,
            cantidadMaxima,
            equipo
        );
    }

    EndMode3D();
}


//==================================================
// DIBUJO 2D
//==================================================

static void DibujarBarraEquipoTronco(
    const MinijuegoTronco& minijuego,
    const Participante participantes[],
    int equipo,
    int xInicio,
    int anchoMitad
)
{
    Color colorEquipo =
        ObtenerColorEquipoTronco(
            minijuego,
            participantes,
            equipo
        );

    const int margen = 30;
    int x = xInicio + margen;
    int ancho = anchoMitad - margen * 2;

    DrawText(
        TextFormat(
            "EQUIPO %d",
            equipo + 1
        ),
        x,
        96,
        24,
        colorEquipo
    );

    DrawRectangle(
        x,
        128,
        ancho,
        24,
        Fade(BLACK, 0.62f)
    );

    int anchoProgreso =
        (int)(
            (ancho - 6) *
            minijuego.equipos[equipo].progresoCorte
        );

    DrawRectangle(
        x + 3,
        131,
        anchoProgreso,
        18,
        colorEquipo
    );

    DrawText(
        TextFormat(
            "CORTE %d%%",
            (int)(
                minijuego.equipos[equipo]
                    .progresoCorte *
                100.0f
            )
        ),
        x + ancho - 112,
        99,
        18,
        RAYWHITE
    );

    if (minijuego.equipos[equipo].tiempoFeedback > 0.0f)
    {
        const char* texto =
            minijuego.equipos[equipo]
                .ultimoGolpeCorrecto
            ? "CORTE!"
            : "DESCOORDINADOS";

        int tamano = 21;

        DrawText(
            texto,
            x + ancho / 2 -
                MeasureText(texto, tamano) / 2,
            164,
            tamano,
            minijuego.equipos[equipo]
                .ultimoGolpeCorrecto
            ? LIME
            : RED
        );
    }
}


static void DibujarTarjetasEquipoTronco(
    const MinijuegoTronco& minijuego,
    const Participante participantes[],
    int cantidadMaxima,
    int equipo,
    int xInicio,
    int anchoMitad
)
{
    int cantidadEquipo =
        minijuego.cantidadJugadoresEquipo[equipo];

    if (cantidadEquipo <= 0)
    {
        return;
    }

    int anchoTarjeta =
        cantidadEquipo == 1
        ? 260
        : (anchoMitad - 54) / 2;

    if (anchoTarjeta > 270)
    {
        anchoTarjeta = 270;
    }

    int separacion = 12;

    int anchoTotal =
        cantidadEquipo * anchoTarjeta +
        (cantidadEquipo - 1) * separacion;

    int x =
        xInicio +
        anchoMitad / 2 -
        anchoTotal / 2;

    int y =
        GetScreenHeight() - 132;

    int limite = LimitarCantidadTronco(cantidadMaxima);

    for (int i = 0; i < limite; i++)
    {
        if (minijuego.equipoPorJugador[i] != equipo)
        {
            continue;
        }

        AccionTronco accionEsperada =
            ObtenerAccionEsperadaTronco(
                minijuego,
                i
            );

        Color fondo =
            minijuego.estadosJugadores[i].respondio
            ? Fade(LIME, 0.78f)
            : Fade(BLACK, 0.78f);

        DrawRectangle(
            x,
            y,
            anchoTarjeta,
            104,
            fondo
        );

        DrawRectangleLinesEx(
            Rectangle{
                (float)x,
                (float)y,
                (float)anchoTarjeta,
                104.0f
            },
            3.0f,
            participantes[i].color
        );

        DrawText(
            TextFormat(
                "J%d  %s",
                participantes[i].numeroJugador,
                TextoDispositivoTronco(
                    participantes[i]
                )
            ),
            x + 10,
            y + 8,
            15,
            RAYWHITE
        );

        const char* nombreAccion =
            minijuego.estadosJugadores[i].respondio
            ? "LISTO"
            : NombreAccionTronco(
                accionEsperada
            );

        DrawText(
            nombreAccion,
            x + 10,
            y + 39,
            22,
            minijuego.estadosJugadores[i].respondio
            ? DARKGREEN
            : ORANGE
        );

        const char* boton =
            TextoAccionTronco(
                participantes[i],
                accionEsperada
            );

        const char* textoBoton =
            TextFormat(
                "[%s]",
                boton
            );

        DrawText(
            textoBoton,
            x + anchoTarjeta -
                MeasureText(
                    textoBoton,
                    26
                ) - 12,
            y + 35,
            26,
            RAYWHITE
        );

        if (
            minijuego.equipos[equipo].golpeEnCurso &&
            minijuego.estadosJugadores[i].respondio
        )
        {
            DrawText(
                "ESPERANDO COMPANERO",
                x + 10,
                y + 74,
                14,
                YELLOW
            );
        }
        else
        {
            DrawText(
                "TIRAR/EMPUJAR EN PAREJA",
                x + 10,
                y + 74,
                13,
                LIGHTGRAY
            );
        }

        x +=
            anchoTarjeta +
            separacion;
    }
}


void MinijuegoTronco::Dibujar(
    const JugadorPrueba jugadores[],
    int cantidadMaxima,
    const Participante participantes[]
) const
{
    ClearBackground(
        Color{ 135, 196, 225, 255 }
    );

    DibujarEscenarioTronco(
        *this,
        jugadores,
        participantes,
        cantidadMaxima
    );

    int anchoPantalla =
        GetScreenWidth();

    int altoPantalla =
        GetScreenHeight();

    int mitad =
        anchoPantalla / 2;

    DrawRectangle(
        0,
        0,
        anchoPantalla,
        84,
        Fade(BLACK, 0.82f)
    );

    DrawText(
        "TRONCO - CARRERA POR EQUIPOS",
        24,
        20,
        27,
        RAYWHITE
    );

    DrawText(
        cantidadJugadoresEquipo[0] == 2 &&
        cantidadJugadoresEquipo[1] == 2
        ? "2 VS 2"
        : "MODO DE PRUEBA 1 VS 1",
        25,
        53,
        16,
        LIGHTGRAY
    );

    DrawText(
        TextFormat(
            "TIEMPO %.1f",
            tiempoPartida
        ),
        anchoPantalla / 2 - 68,
        27,
        23,
        tiempoPartida <= 10.0f
        ? RED
        : SKYBLUE
    );

    DrawRectangle(
        mitad - 3,
        84,
        6,
        altoPantalla - 84,
        Fade(BLACK, 0.88f)
    );

    DrawRectangle(
        mitad - 1,
        84,
        2,
        altoPantalla - 84,
        ORANGE
    );

    if (!partidaValida)
    {
        const char* texto =
            "ESPERANDO 2 O 4 JUGADORES ACTIVOS Y CONECTADOS";

        DrawRectangle(
            anchoPantalla / 2 - 360,
            altoPantalla / 2 - 58,
            720,
            116,
            Fade(BLACK, 0.90f)
        );

        DrawText(
            texto,
            anchoPantalla / 2 -
                MeasureText(texto, 24) / 2,
            altoPantalla / 2 - 12,
            24,
            ORANGE
        );

        return;
    }

    DibujarBarraEquipoTronco(
        *this,
        participantes,
        0,
        0,
        mitad
    );

    DibujarBarraEquipoTronco(
        *this,
        participantes,
        1,
        mitad,
        anchoPantalla - mitad
    );

    DibujarTarjetasEquipoTronco(
        *this,
        participantes,
        cantidadMaxima,
        0,
        0,
        mitad
    );

    DibujarTarjetasEquipoTronco(
        *this,
        participantes,
        cantidadMaxima,
        1,
        mitad,
        anchoPantalla - mitad
    );

    if (estado == TRONCO_PREPARANDO)
    {
        int numero =
            (int)tiempoPreparacion + 1;

        const char* texto =
            numero > 1
            ? TextFormat("%d", numero)
            : "YA!";

        int tamano = 58;

        DrawRectangle(
            anchoPantalla / 2 - 90,
            200,
            180,
            86,
            Fade(BLACK, 0.72f)
        );

        DrawText(
            texto,
            anchoPantalla / 2 -
                MeasureText(texto, tamano) / 2,
            214,
            tamano,
            YELLOW
        );
    }

    if (estado == TRONCO_FINALIZADO)
    {
        DrawRectangle(
            anchoPantalla / 2 - 320,
            altoPantalla / 2 - 72,
            640,
            144,
            Fade(BLACK, 0.92f)
        );

        const char* texto =
            empate
            ? "EMPATE - SE TERMINO EL TIEMPO"
            : TextFormat(
                "EQUIPO %d GANA!",
                equipoGanador + 1
            );

        int tamano =
            empate
            ? 30
            : 40;

        DrawText(
            texto,
            anchoPantalla / 2 -
                MeasureText(texto, tamano) / 2,
            altoPantalla / 2 - 20,
            tamano,
            empate
            ? ORANGE
            : LIME
        );
    }
}


const ResultadoMinijuego& MinijuegoTronco::ObtenerResultado() const
{
    return resultado;
}
