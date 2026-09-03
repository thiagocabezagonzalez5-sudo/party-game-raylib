#include "Minigames/Minijuego67.h"

#include "Systems/Input.h"

#include "raymath.h"

#include <cmath>
#include <cstring>


//==================================================
// CONSTANTES
//==================================================

static const float DURACION_PREPARACION_67 = 2.5f;
static const float DURACION_PARTIDA_67 = 30.0f;

static const float INICIO_ZONA_RECOGIDA_67 = 0.68f;
static const float FIN_ZONA_RECOGIDA_67 = 0.88f;

static const float X_INICIO_CINTA_67 = -5.6f;
static const float LARGO_CINTA_67 = 7.4f;
static const float X_JUGADOR_67 = 2.35f;
static const float X_MESA_67 = 2.70f;

static const float Z_CARRIL_SUPERIOR_67 = -1.55f;
static const float Z_CARRIL_INFERIOR_67 = 1.55f;

static const float DEMORA_REAPARICION_FALLO_67 = 0.26f;
static const float DEMORA_REAPARICION_AGARRE_67 = 0.52f;
static const float DURACION_GIRO_67 = 0.28f;
static const float DURACION_STUN_67 = 0.46f;
static const float DURACION_MESA_COMPLETA_67 = 0.48f;
static const float DURACION_FEEDBACK_67 = 0.55f;


//==================================================
// UTILIDADES GENERALES
//==================================================

static int ContarParticipantesActivos67(
    const Participante participantes[],
    int cantidadMaxima
)
{
    int cantidad = 0;

    for (int i = 0; i < cantidadMaxima; i++)
    {
        if (participantes[i].activo)
        {
            cantidad++;
        }
    }

    return cantidad;
}


static int ContarParticipantesDisponibles67(
    const Participante participantes[],
    int cantidadMaxima
)
{
    int cantidad = 0;

    for (int i = 0; i < cantidadMaxima; i++)
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


static int CrearMascaraParticipantes67(
    const Participante participantes[],
    int cantidadMaxima
)
{
    int mascara = 0;

    for (int i = 0; i < cantidadMaxima; i++)
    {
        if (participantes[i].activo)
        {
            mascara |= 1 << i;

            if (participantes[i].conectado)
            {
                mascara |=
                    1 << (i + MAX_JUGADORES_PRUEBA);
            }
        }
    }

    return mascara;
}


static bool NombreEsIdle67(const char* nombre)
{
    if (nombre == nullptr)
    {
        return false;
    }

    return
        std::strstr(nombre, "Idle") != nullptr ||
        std::strstr(nombre, "idle") != nullptr ||
        std::strstr(nombre, "IDLE") != nullptr;
}


static Color ObtenerColorEquipo67(int equipo)
{
    return equipo == 0
        ? Color{ 238, 55, 66, 255 }
        : Color{ 40, 159, 224, 255 };
}


static float ObtenerZCarrilEquipo67(
    const Minijuego67& minijuego,
    int indiceJugador
)
{
    int equipo = minijuego.equipoPorJugador[indiceJugador];

    if (equipo < 0 || equipo > 1)
    {
        return 0.0f;
    }

    if (minijuego.cantidadJugadoresEquipo[equipo] <= 1)
    {
        return 0.0f;
    }

    return
        minijuego.ordenEnEquipoPorJugador[indiceJugador] == 0
        ? Z_CARRIL_SUPERIOR_67
        : Z_CARRIL_INFERIOR_67;
}


static float ObtenerXObjeto67(float progreso)
{
    return
        X_INICIO_CINTA_67 +
        progreso * LARGO_CINTA_67;
}


static void ReiniciarObjetoCinta67(
    EstadoJugador67& estado,
    float demora
)
{
    estado.objetoActivo = false;
    estado.tiempoReaparicion = demora;
    estado.progresoObjeto = -0.06f;
}


static int BuscarJugadorUnicoEquipo67(
    const Minijuego67& minijuego,
    int equipo,
    int cantidadMaxima
)
{
    for (int i = 0; i < cantidadMaxima; i++)
    {
        if (minijuego.equipoPorJugador[i] == equipo)
        {
            return i;
        }
    }

    return -1;
}


static void ActualizarTipoPiezaModoPrueba67(
    Minijuego67& minijuego,
    int equipo,
    int cantidadMaxima
)
{
    if (
        equipo < 0 ||
        equipo > 1 ||
        minijuego.cantidadJugadoresEquipo[equipo] != 1
    )
    {
        return;
    }

    int indiceJugador =
        BuscarJugadorUnicoEquipo67(
            minijuego,
            equipo,
            cantidadMaxima
        );

    if (indiceJugador < 0)
    {
        return;
    }

    minijuego.estadosJugadores[indiceJugador].tipoPieza =
        minijuego.equipos[equipo].mesa == MESA_67_CON_6
        ? PIEZA_NUMERO_7
        : PIEZA_NUMERO_6;
}


//==================================================
// RENDER TEXTURES PARA PANTALLA DIVIDIDA
//==================================================

static void DescargarVistasEquipo67(
    Minijuego67& minijuego
)
{
    for (int equipo = 0; equipo < 2; equipo++)
    {
        if (minijuego.vistasEquipo[equipo].id != 0)
        {
            UnloadRenderTexture(
                minijuego.vistasEquipo[equipo]
            );

            minijuego.vistasEquipo[equipo] = {};
        }
    }

    minijuego.vistasEquipoCargadas = false;
    minijuego.anchoVistaEquipos = 0;
    minijuego.altoVistaEquipos = 0;
}


static void AsegurarVistasEquipo67(
    Minijuego67& minijuego
)
{
    int ancho = GetScreenWidth() / 2;
    int alto = GetScreenHeight();

    if (ancho <= 0 || alto <= 0)
    {
        return;
    }

    if (
        minijuego.vistasEquipoCargadas &&
        minijuego.anchoVistaEquipos == ancho &&
        minijuego.altoVistaEquipos == alto
    )
    {
        return;
    }

    DescargarVistasEquipo67(minijuego);

    minijuego.vistasEquipo[0] =
        LoadRenderTexture(ancho, alto);

    minijuego.vistasEquipo[1] =
        LoadRenderTexture(ancho, alto);

    minijuego.vistasEquipoCargadas =
        minijuego.vistasEquipo[0].id != 0 &&
        minijuego.vistasEquipo[1].id != 0;

    if (!minijuego.vistasEquipoCargadas)
    {
        DescargarVistasEquipo67(minijuego);
        return;
    }

    minijuego.anchoVistaEquipos = ancho;
    minijuego.altoVistaEquipos = alto;
}


//==================================================
// RESULTADO POR EQUIPOS
//==================================================

static void FinalizarResultado67(
    Minijuego67& minijuego,
    int cantidadMaxima
)
{
    if (
        minijuego.resultado.estado !=
        RESULTADO_MINIJUEGO_EN_CURSO ||
        !minijuego.partidaValida
    )
    {
        return;
    }

    int puntosEquipo0 = minijuego.equipos[0].puntos;
    int puntosEquipo1 = minijuego.equipos[1].puntos;

    minijuego.empate =
        puntosEquipo0 == puntosEquipo1;

    minijuego.equipoGanador = -1;

    if (!minijuego.empate)
    {
        minijuego.equipoGanador =
            puntosEquipo0 > puntosEquipo1
            ? 0
            : 1;
    }

    minijuego.resultado.estado =
        RESULTADO_MINIJUEGO_FINALIZADO;

    minijuego.resultado.desenlace =
        minijuego.empate
        ? DESENLACE_EMPATE
        : DESENLACE_CON_GANADOR;

    minijuego.resultado.cantidadEquipos = 2;

    for (int i = 0; i < cantidadMaxima; i++)
    {
        ResultadoParticipante& participanteResultado =
            minijuego.resultado.participantes[i];

        if (!participanteResultado.participo)
        {
            continue;
        }

        int equipo = minijuego.equipoPorJugador[i];

        participanteResultado.numeroEquipo = equipo;
        participanteResultado.puntuacionMinijuego =
            equipo >= 0 && equipo < 2
            ? minijuego.equipos[equipo].puntos
            : 0;
        participanteResultado.puntosObtenidos = 0;

        if (minijuego.empate)
        {
            participanteResultado.posicionFinal = 1;
        }
        else
        {
            participanteResultado.posicionFinal =
                equipo == minijuego.equipoGanador
                ? 1
                : 2;
        }
    }
}


//==================================================
// LOGICA DE COLOCACION
//==================================================

static void ColocarPieza67(
    Minijuego67& minijuego,
    int indiceJugador,
    int cantidadMaxima
)
{
    int equipo =
        minijuego.equipoPorJugador[indiceJugador];

    if (equipo < 0 || equipo > 1)
    {
        return;
    }

    EstadoJugador67& estadoJugador =
        minijuego.estadosJugadores[indiceJugador];

    EstadoEquipo67& estadoEquipo =
        minijuego.equipos[equipo];

    bool colocacionCorrecta = false;

    if (
        estadoJugador.tipoPieza == PIEZA_NUMERO_6 &&
        estadoEquipo.mesa == MESA_67_VACIA
    )
    {
        estadoEquipo.mesa = MESA_67_CON_6;
        colocacionCorrecta = true;
    }
    else if (
        estadoJugador.tipoPieza == PIEZA_NUMERO_7 &&
        estadoEquipo.mesa == MESA_67_CON_6
    )
    {
        estadoEquipo.mesa = MESA_67_COMPLETA;
        estadoEquipo.tiempoMesaCompleta =
            DURACION_MESA_COMPLETA_67;
        estadoEquipo.puntos++;
        colocacionCorrecta = true;
    }

    estadoEquipo.ultimoAcierto = colocacionCorrecta;
    estadoEquipo.tiempoFeedback = DURACION_FEEDBACK_67;

    if (!colocacionCorrecta)
    {
        estadoJugador.tiempoStun = DURACION_STUN_67;
    }

    estadoJugador.llevaPieza = false;
    estadoJugador.mirandoMesa = false;
    estadoJugador.tiempoGiro = 0.0f;

    if (
        colocacionCorrecta &&
        minijuego.cantidadJugadoresEquipo[equipo] == 1
    )
    {
        ActualizarTipoPiezaModoPrueba67(
            minijuego,
            equipo,
            cantidadMaxima
        );
    }
}


//==================================================
// DIBUJO 3D
//==================================================

static void DibujarSegmentoNumero67(
    Vector3 posicion,
    bool horizontal,
    float escala,
    Color color
)
{
    float ancho = horizontal ? 0.62f : 0.13f;
    float alto = horizontal ? 0.13f : 0.52f;

    DrawCube(
        posicion,
        ancho * escala,
        alto * escala,
        0.20f * escala,
        color
    );

    DrawCubeWires(
        posicion,
        ancho * escala,
        alto * escala,
        0.20f * escala,
        Fade(BLACK, 0.62f)
    );
}


static void DibujarNumero3D67(
    int numero,
    Vector3 centro,
    float escala,
    Color color
)
{
    const float X_LADO = 0.31f * escala;
    const float Y_EXTREMO = 0.55f * escala;
    const float Y_MEDIO_LADO = 0.275f * escala;

    bool segmentos[7] = {};

    if (numero == 6)
    {
        segmentos[0] = true;
        segmentos[2] = true;
        segmentos[3] = true;
        segmentos[4] = true;
        segmentos[5] = true;
        segmentos[6] = true;
    }
    else
    {
        segmentos[0] = true;
        segmentos[1] = true;
        segmentos[2] = true;
    }

    if (segmentos[0])
    {
        DibujarSegmentoNumero67(
            { centro.x, centro.y + Y_EXTREMO, centro.z },
            true,
            escala,
            color
        );
    }

    if (segmentos[1])
    {
        DibujarSegmentoNumero67(
            { centro.x + X_LADO, centro.y + Y_MEDIO_LADO, centro.z },
            false,
            escala,
            color
        );
    }

    if (segmentos[2])
    {
        DibujarSegmentoNumero67(
            { centro.x + X_LADO, centro.y - Y_MEDIO_LADO, centro.z },
            false,
            escala,
            color
        );
    }

    if (segmentos[3])
    {
        DibujarSegmentoNumero67(
            { centro.x, centro.y - Y_EXTREMO, centro.z },
            true,
            escala,
            color
        );
    }

    if (segmentos[4])
    {
        DibujarSegmentoNumero67(
            { centro.x - X_LADO, centro.y - Y_MEDIO_LADO, centro.z },
            false,
            escala,
            color
        );
    }

    if (segmentos[5])
    {
        DibujarSegmentoNumero67(
            { centro.x - X_LADO, centro.y + Y_MEDIO_LADO, centro.z },
            false,
            escala,
            color
        );
    }

    if (segmentos[6])
    {
        DibujarSegmentoNumero67(
            centro,
            true,
            escala,
            color
        );
    }
}


static void DibujarCinta67(
    float z,
    float desplazamiento,
    Color colorEquipo
)
{
    float centroX =
        X_INICIO_CINTA_67 +
        LARGO_CINTA_67 / 2.0f;

    DrawCube(
        { centroX, 0.22f, z },
        LARGO_CINTA_67 + 0.40f,
        0.44f,
        1.22f,
        Color{ 49, 54, 62, 255 }
    );

    DrawCubeWires(
        { centroX, 0.22f, z },
        LARGO_CINTA_67 + 0.40f,
        0.44f,
        1.22f,
        BLACK
    );

    DrawCube(
        { centroX, 0.48f, z },
        LARGO_CINTA_67,
        0.08f,
        1.02f,
        Color{ 83, 91, 101, 255 }
    );

    for (int i = 0; i < 13; i++)
    {
        float avance =
            std::fmod(
                i * 0.61f + desplazamiento,
                LARGO_CINTA_67
            );

        float x = X_INICIO_CINTA_67 + avance;

        DrawCube(
            { x, 0.535f, z },
            0.07f,
            0.035f,
            0.98f,
            Color{ 176, 184, 193, 255 }
        );
    }

    float xZonaInicio =
        ObtenerXObjeto67(INICIO_ZONA_RECOGIDA_67);

    float xZonaFin =
        ObtenerXObjeto67(FIN_ZONA_RECOGIDA_67);

    DrawCube(
        {
            (xZonaInicio + xZonaFin) / 2.0f,
            0.58f,
            z
        },
        xZonaFin - xZonaInicio,
        0.04f,
        1.06f,
        Fade(YELLOW, 0.42f)
    );

    for (int lado = -1; lado <= 1; lado += 2)
    {
        DrawCube(
            { centroX, 0.66f, z + lado * 0.61f },
            LARGO_CINTA_67 + 0.45f,
            0.18f,
            0.10f,
            Color{ 42, 46, 53, 255 }
        );
    }

    for (int pata = 0; pata < 3; pata++)
    {
        float xPata =
            X_INICIO_CINTA_67 +
            0.8f +
            pata * 2.95f;

        DrawCube(
            { xPata, -0.02f, z },
            0.18f,
            0.55f,
            0.84f,
            Color{ 48, 52, 58, 255 }
        );
    }

    DrawCube(
        { X_INICIO_CINTA_67 - 0.46f, 1.15f, z },
        0.72f,
        2.15f,
        1.58f,
        Color{ 58, 63, 71, 255 }
    );

    DrawCube(
        { X_INICIO_CINTA_67 - 0.08f, 1.22f, z },
        0.08f,
        1.35f,
        1.16f,
        Fade(colorEquipo, 0.78f)
    );
}


static void DibujarMesa67(
    EstadoMesa67 estado,
    float tiempoCompleta,
    Color colorEquipo
)
{
    Color colorBase =
        tiempoCompleta > 0.0f
        ? Color{ 72, 188, 99, 255 }
        : Color{ 48, 51, 58, 255 };

    // Plataforma central de armado: queda entre los dos
    // jugadores del equipo, como en el boceto de referencia.
    DrawCylinder(
        { X_MESA_67, 0.13f, 0.0f },
        1.18f,
        1.18f,
        0.26f,
        32,
        Color{ 24, 26, 31, 255 }
    );

    DrawCylinder(
        { X_MESA_67, 0.30f, 0.0f },
        1.00f,
        1.00f,
        0.12f,
        32,
        colorBase
    );

    DrawCylinder(
        { X_MESA_67, 0.39f, 0.0f },
        0.84f,
        0.84f,
        0.05f,
        32,
        Fade(colorEquipo, 0.58f)
    );

    // Marca tenue 67 incluso cuando la plataforma esta vacia.
    DibujarNumero3D67(
        6,
        { X_MESA_67 - 0.34f, 0.82f, 0.0f },
        0.47f,
        estado == MESA_67_VACIA
        ? Fade(RAYWHITE, 0.28f)
        : ORANGE
    );

    DibujarNumero3D67(
        7,
        { X_MESA_67 + 0.34f, 0.82f, 0.0f },
        0.47f,
        estado == MESA_67_COMPLETA
        ? SKYBLUE
        : Fade(RAYWHITE, 0.28f)
    );

    if (estado == MESA_67_COMPLETA)
    {
        DrawCylinder(
            { X_MESA_67, 0.47f, 0.0f },
            0.92f,
            0.92f,
            0.04f,
            32,
            Fade(LIME, 0.48f)
        );
    }
}


static void DibujarJugador3D67(
    const Minijuego67& minijuego,
    const Participante& participante,
    const EstadoJugador67& estado,
    float z
)
{
    float giro = -90.0f;

    if (estado.llevaPieza)
    {
        float progresoGiro =
            1.0f -
            estado.tiempoGiro / DURACION_GIRO_67;

        if (progresoGiro < 0.0f)
        {
            progresoGiro = 0.0f;
        }

        if (progresoGiro > 1.0f)
        {
            progresoGiro = 1.0f;
        }

        giro = -90.0f + 180.0f * progresoGiro;
    }

    float alturaBase =
        estado.tiempoStun > 0.0f
        ? 0.03f
        : 0.16f;

    Color colorModelo =
        estado.tiempoStun > 0.0f
        ? Fade(RED, 0.72f)
        : participante.color;

    DrawCylinder(
        { X_JUGADOR_67, 0.08f, z },
        0.60f,
        0.60f,
        0.16f,
        24,
        Fade(participante.color, 0.70f)
    );

    if (minijuego.modeloJugadorCargado)
    {
        DrawModelEx(
            minijuego.modeloJugador,
            { X_JUGADOR_67, alturaBase, z },
            { 0.0f, 1.0f, 0.0f },
            giro,
            { 0.25f, 0.25f, 0.25f },
            colorModelo
        );
    }
    else
    {
        DrawCube(
            { X_JUGADOR_67, 0.90f, z },
            0.72f,
            1.55f,
            0.72f,
            colorModelo
        );

        DrawCubeWires(
            { X_JUGADOR_67, 0.90f, z },
            0.72f,
            1.55f,
            0.72f,
            BLACK
        );
    }

    if (estado.llevaPieza)
    {
        Color colorPieza =
            estado.tipoPieza == PIEZA_NUMERO_6
            ? ORANGE
            : SKYBLUE;

        DibujarNumero3D67(
            estado.tipoPieza == PIEZA_NUMERO_6
            ? 6
            : 7,
            { X_JUGADOR_67, 2.18f, z },
            0.62f,
            colorPieza
        );
    }
}


static void DibujarFabricaEquipo67(
    int equipo
)
{
    Color colorEquipo = ObtenerColorEquipo67(equipo);

    DrawPlane(
        { 0.0f, -0.31f, 0.0f },
        { 18.0f, 10.0f },
        equipo == 0
        ? Color{ 79, 45, 48, 255 }
        : Color{ 38, 61, 77, 255 }
    );

    // Pared posterior neutra con una franja del color del equipo.
    DrawCube(
        { 0.0f, 2.45f, -4.45f },
        16.8f,
        5.5f,
        0.25f,
        Color{ 66, 72, 80, 255 }
    );

    DrawCube(
        { 0.0f, 4.65f, -4.28f },
        16.2f,
        0.55f,
        0.10f,
        Fade(colorEquipo, 0.88f)
    );

    DrawCube(
        { -7.35f, 2.45f, 0.0f },
        0.35f,
        5.5f,
        8.8f,
        Color{ 57, 63, 70, 255 }
    );

    // Dos paneles grandes ayudan a separar visualmente el
    // carril del 6 y el carril del 7.
    DrawCube(
        { -1.15f, 0.02f, Z_CARRIL_SUPERIOR_67 },
        11.8f,
        0.05f,
        2.10f,
        Fade(colorEquipo, 0.16f)
    );

    DrawCube(
        { -1.15f, 0.02f, Z_CARRIL_INFERIOR_67 },
        11.8f,
        0.05f,
        2.10f,
        Fade(colorEquipo, 0.16f)
    );

    for (int i = 0; i < 5; i++)
    {
        DrawCube(
            { -5.8f + i * 2.85f, 3.92f, -4.18f },
            1.45f,
            1.10f,
            0.08f,
            Fade(colorEquipo, 0.52f)
        );

        DrawCubeWires(
            { -5.8f + i * 2.85f, 3.92f, -4.18f },
            1.45f,
            1.10f,
            0.08f,
            BLACK
        );
    }

    for (int i = 0; i < 4; i++)
    {
        DrawCube(
            { -5.8f + i * 3.8f, 2.1f, -4.05f },
            0.30f,
            4.1f,
            0.30f,
            Color{ 49, 54, 61, 255 }
        );
    }
}


//==================================================
// HUD DE CADA EQUIPO
//==================================================

static const char* ObtenerTextoMesa67(
    EstadoMesa67 mesa
)
{
    if (mesa == MESA_67_VACIA)
    {
        return "MESA: FALTA EL 6";
    }

    if (mesa == MESA_67_CON_6)
    {
        return "MESA: FALTA EL 7";
    }

    return "MESA COMPLETA +1";
}


static void DibujarHudEquipo67(
    const Minijuego67& minijuego,
    int equipo,
    int cantidadMaxima,
    const Participante participantes[]
)
{
    int ancho = minijuego.anchoVistaEquipos;
    int alto = minijuego.altoVistaEquipos;

    if (ancho <= 0 || alto <= 0)
    {
        return;
    }

    Color colorEquipo = ObtenerColorEquipo67(equipo);
    const EstadoEquipo67& estadoEquipo =
        minijuego.equipos[equipo];

    DrawRectangle(
        12,
        10,
        ancho - 24,
        82,
        Fade(BLACK, 0.74f)
    );

    DrawRectangle(
        12,
        10,
        8,
        82,
        colorEquipo
    );

    DrawText(
        TextFormat("EQUIPO %d", equipo + 1),
        30,
        18,
        25,
        RAYWHITE
    );

    DrawText(
        TextFormat("PUNTOS %d", estadoEquipo.puntos),
        30,
        52,
        19,
        LIME
    );

    const char* textoTiempo =
        TextFormat("%.1f s", minijuego.tiempoPartida);

    DrawText(
        textoTiempo,
        ancho - MeasureText(textoTiempo, 22) - 24,
        22,
        22,
        minijuego.tiempoPartida <= 7.0f
        ? RED
        : SKYBLUE
    );

    const char* textoMesa =
        ObtenerTextoMesa67(estadoEquipo.mesa);

    DrawText(
        textoMesa,
        ancho - MeasureText(textoMesa, 14) - 24,
        56,
        14,
        LIGHTGRAY
    );

    int indiceRol6 = -1;
    int indiceRol7 = -1;
    int indiceUnico = -1;

    for (int i = 0; i < cantidadMaxima; i++)
    {
        if (
            minijuego.equipoPorJugador[i] != equipo ||
            !participantes[i].activo ||
            !participantes[i].conectado
        )
        {
            continue;
        }

        indiceUnico = i;

        if (
            minijuego.estadosJugadores[i].tipoPieza ==
            PIEZA_NUMERO_6
        )
        {
            indiceRol6 = i;
        }
        else
        {
            indiceRol7 = i;
        }
    }

    if (minijuego.cantidadJugadoresEquipo[equipo] == 1)
    {
        indiceRol6 = indiceUnico;
        indiceRol7 = -1;
    }

    int anchoTarjeta = ancho < 640 ? 185 : 215;
    int xTarjeta = ancho - anchoTarjeta - 18;

    auto DibujarTarjetaRol =
        [&](int indiceJugador, int numero, int y)
        {
            if (indiceJugador < 0)
            {
                return;
            }

            const EstadoJugador67& estado =
                minijuego.estadosJugadores[indiceJugador];

            DrawRectangle(
                xTarjeta,
                y,
                anchoTarjeta,
                62,
                Fade(BLACK, 0.80f)
            );

            DrawRectangleLinesEx(
                Rectangle{
                    (float)xTarjeta,
                    (float)y,
                    (float)anchoTarjeta,
                    62.0f
                },
                2.0f,
                participantes[indiceJugador].color
            );

            DrawText(
                TextFormat(
                    "J%d  ->  %d",
                    participantes[indiceJugador].numeroJugador,
                    numero
                ),
                xTarjeta + 9,
                y + 7,
                19,
                numero == 6 ? ORANGE : SKYBLUE
            );

            const char* accion =
                estado.tiempoStun > 0.0f
                ? "FALLO"
                : (
                    estado.llevaPieza
                    ? (
                        estado.mirandoMesa
                        ? "COLOCAR"
                        : "GIRANDO"
                    )
                    : "AGARRAR"
                );

            DrawText(
                TextFormat(
                    "[%s] %s",
                    ObtenerTextoBotonPrincipal(
                        participantes[indiceJugador]
                    ),
                    accion
                ),
                xTarjeta + 9,
                y + 36,
                14,
                estado.tiempoStun > 0.0f
                ? RED
                : RAYWHITE
            );
        };

    if (minijuego.cantidadJugadoresEquipo[equipo] == 1)
    {
        if (indiceUnico >= 0)
        {
            int numeroActual =
                minijuego.estadosJugadores[indiceUnico].tipoPieza ==
                PIEZA_NUMERO_6
                ? 6
                : 7;

            DibujarTarjetaRol(
                indiceUnico,
                numeroActual,
                alto / 2 - 31
            );
        }
    }
    else
    {
        DibujarTarjetaRol(
            indiceRol6,
            6,
            alto / 2 - 155
        );

        DibujarTarjetaRol(
            indiceRol7,
            7,
            alto / 2 + 93
        );
    }

    if (estadoEquipo.tiempoFeedback > 0.0f)
    {
        const char* textoFeedback =
            estadoEquipo.ultimoAcierto
            ? "BIEN!"
            : "ORDEN INCORRECTO";

        DrawRectangle(
            18,
            alto / 2 - 28,
            MeasureText(textoFeedback, 20) + 24,
            42,
            Fade(BLACK, 0.78f)
        );

        DrawText(
            textoFeedback,
            30,
            alto / 2 - 18,
            20,
            estadoEquipo.ultimoAcierto
            ? LIME
            : RED
        );
    }
}


static void DibujarVistaEquipo67(
    const Minijuego67& minijuego,
    int equipo,
    int cantidadMaxima,
    const Participante participantes[]
)
{
    BeginTextureMode(
        minijuego.vistasEquipo[equipo]
    );

    Color colorEquipo = ObtenerColorEquipo67(equipo);

    ClearBackground(
        equipo == 0
        ? Color{ 95, 28, 34, 255 }
        : Color{ 21, 72, 103, 255 }
    );

    BeginMode3D(
        minijuego.camarasEquipo[equipo]
    );

    DibujarFabricaEquipo67(equipo);

    for (int i = 0; i < cantidadMaxima; i++)
    {
        if (
            minijuego.equipoPorJugador[i] != equipo ||
            !participantes[i].activo ||
            !participantes[i].conectado
        )
        {
            continue;
        }

        float z =
            ObtenerZCarrilEquipo67(
                minijuego,
                i
            );

        DibujarCinta67(
            z,
            minijuego.desplazamientoVisualCintas,
            colorEquipo
        );

        const EstadoJugador67& estado =
            minijuego.estadosJugadores[i];

        if (estado.objetoActivo)
        {
            Color colorPieza =
                estado.tipoPieza == PIEZA_NUMERO_6
                ? ORANGE
                : SKYBLUE;

            DibujarNumero3D67(
                estado.tipoPieza == PIEZA_NUMERO_6
                ? 6
                : 7,
                {
                    ObtenerXObjeto67(
                        estado.progresoObjeto
                    ),
                    0.94f,
                    z
                },
                0.62f,
                colorPieza
            );
        }

        DibujarJugador3D67(
            minijuego,
            participantes[i],
            estado,
            z
        );
    }

    DibujarMesa67(
        minijuego.equipos[equipo].mesa,
        minijuego.equipos[equipo].tiempoMesaCompleta,
        colorEquipo
    );

    EndMode3D();

    DibujarHudEquipo67(
        minijuego,
        equipo,
        cantidadMaxima,
        participantes
    );

    EndTextureMode();
}


//==================================================
// INICIALIZAR / REINICIAR
//==================================================

void Minijuego67::Inicializar()
{
    tiempoPreparacion = DURACION_PREPARACION_67;
    tiempoPartida = DURACION_PARTIDA_67;
    velocidadCintas = 0.24f;
    desplazamientoVisualCintas = 0.0f;
    jugadoresEnPartida = 0;
    mascaraJugadoresEnPartida = 0;
    equipoGanador = -1;
    partidaValida = false;
    empate = false;
    estadoPartida = FABRICA_67_ESPERANDO_JUGADORES;

    for (int equipo = 0; equipo < 2; equipo++)
    {
        camarasEquipo[equipo].position =
            { 7.6f, 10.8f, 11.8f };

        camarasEquipo[equipo].target =
            { -1.15f, 0.55f, 0.0f };

        camarasEquipo[equipo].up =
            { 0.0f, 1.0f, 0.0f };

        // La vista ortografica conserva el esquema del boceto:
        // dos cintas horizontales y la plataforma 67 al centro.
        camarasEquipo[equipo].fovy = 11.8f;
        camarasEquipo[equipo].projection = CAMERA_ORTHOGRAPHIC;
    }

    AsegurarVistasEquipo67(*this);

    if (modeloJugadorCargado)
    {
        return;
    }

    if (!FileExists(rutaModeloJugador))
    {
        TraceLog(
            LOG_WARNING,
            "No se encontro el modelo para Fabrica 67: %s",
            rutaModeloJugador
        );

        return;
    }

    modeloJugador = LoadModel(rutaModeloJugador);
    modeloJugadorCargado = modeloJugador.meshCount > 0;

    if (!modeloJugadorCargado)
    {
        TraceLog(
            LOG_WARNING,
            "No se pudo cargar el modelo para Fabrica 67"
        );

        return;
    }

    modeloJugador.transform =
        MatrixRotateX(90.0f * DEG2RAD);

    animacionesJugador =
        LoadModelAnimations(
            rutaModeloJugador,
            &cantidadAnimacionesJugador
        );

    indiceAnimacionIdle = -1;

    for (
        int i = 0;
        animacionesJugador != nullptr &&
        i < cantidadAnimacionesJugador;
        i++
    )
    {
        if (NombreEsIdle67(animacionesJugador[i].name))
        {
            indiceAnimacionIdle = i;
            break;
        }
    }

    if (
        indiceAnimacionIdle < 0 &&
        animacionesJugador != nullptr &&
        cantidadAnimacionesJugador == 1
    )
    {
        indiceAnimacionIdle = 0;
    }

    animacionIdleActiva =
        animacionesJugador != nullptr &&
        indiceAnimacionIdle >= 0 &&
        IsModelAnimationValid(
            modeloJugador,
            animacionesJugador[indiceAnimacionIdle]
        ) &&
        animacionesJugador[indiceAnimacionIdle]
            .keyframeCount > 0;
}


void Minijuego67::PrepararEquipos(
    const Participante participantes[],
    int cantidadMaxima
)
{
    for (int i = 0; i < cantidadMaxima; i++)
    {
        equipoPorJugador[i] = -1;
        ordenEnEquipoPorJugador[i] = -1;
    }

    cantidadJugadoresEquipo[0] = 0;
    cantidadJugadoresEquipo[1] = 0;

    int cantidadActivos =
        ContarParticipantesActivos67(
            participantes,
            cantidadMaxima
        );

    int cantidadDisponibles =
        ContarParticipantesDisponibles67(
            participantes,
            cantidadMaxima
        );

    partidaValida =
        cantidadActivos == cantidadDisponibles &&
        (
            cantidadActivos == 2 ||
            cantidadActivos == 4
        );

    jugadoresEnPartida = cantidadDisponibles;
    mascaraJugadoresEnPartida =
        CrearMascaraParticipantes67(
            participantes,
            cantidadMaxima
        );

    resultado.cantidadEquipos =
        partidaValida
        ? 2
        : 0;

    if (!partidaValida)
    {
        estadoPartida =
            FABRICA_67_ESPERANDO_JUGADORES;

        return;
    }

    int indicesActivos[MAX_PARTICIPANTES]{};
    int cantidadIndices = 0;

    for (int i = 0; i < cantidadMaxima; i++)
    {
        if (
            participantes[i].activo &&
            participantes[i].conectado &&
            cantidadIndices < MAX_PARTICIPANTES
        )
        {
            indicesActivos[cantidadIndices] = i;
            cantidadIndices++;
        }
    }

    // Sorteo Fisher-Yates: cada vez que comienza la partida
    // se vuelven a formar los equipos y tambien se sortean
    // los roles 6 y 7 dentro de cada pareja.
    for (int i = cantidadIndices - 1; i > 0; i--)
    {
        int otroIndice = GetRandomValue(0, i);

        int temporal = indicesActivos[i];
        indicesActivos[i] = indicesActivos[otroIndice];
        indicesActivos[otroIndice] = temporal;
    }

    if (cantidadActivos == 4)
    {
        for (int orden = 0; orden < 4; orden++)
        {
            int indiceJugador = indicesActivos[orden];
            int equipo = orden < 2 ? 0 : 1;
            int ordenEquipo = orden % 2;

            equipoPorJugador[indiceJugador] = equipo;
            ordenEnEquipoPorJugador[indiceJugador] = ordenEquipo;
            cantidadJugadoresEquipo[equipo]++;

            resultado.participantes[indiceJugador]
                .numeroEquipo = equipo;
        }
    }
    else
    {
        for (int orden = 0; orden < 2; orden++)
        {
            int indiceJugador = indicesActivos[orden];
            int equipo = orden;

            equipoPorJugador[indiceJugador] = equipo;
            ordenEnEquipoPorJugador[indiceJugador] = 0;
            cantidadJugadoresEquipo[equipo] = 1;

            resultado.participantes[indiceJugador]
                .numeroEquipo = equipo;
        }
    }

    estadoPartida = FABRICA_67_PREPARANDO;
}


void Minijuego67::Reiniciar(
    const Participante participantes[],
    int cantidadMaxima
)
{
    InicializarResultadoMinijuego(
        resultado,
        participantes,
        FORMATO_MINIJUEGO_EQUIPOS
    );

    tiempoPreparacion = DURACION_PREPARACION_67;
    tiempoPartida = DURACION_PARTIDA_67;
    velocidadCintas = 0.24f;
    desplazamientoVisualCintas = 0.0f;
    fotogramaAnimacionIdle = 0.0f;
    equipoGanador = -1;
    empate = false;

    for (int equipo = 0; equipo < 2; equipo++)
    {
        equipos[equipo] = {};
    }

    for (int i = 0; i < cantidadMaxima; i++)
    {
        estadosJugadores[i] = {};

        estadosJugadores[i].progresoObjeto =
            -0.06f + 0.08f * (float)i;
    }

    PrepararEquipos(
        participantes,
        cantidadMaxima
    );

    if (!partidaValida)
    {
        return;
    }

    for (int i = 0; i < cantidadMaxima; i++)
    {
        int equipo = equipoPorJugador[i];

        if (equipo < 0)
        {
            continue;
        }

        if (cantidadJugadoresEquipo[equipo] == 1)
        {
            estadosJugadores[i].tipoPieza =
                PIEZA_NUMERO_6;
        }
        else
        {
            estadosJugadores[i].tipoPieza =
                ordenEnEquipoPorJugador[i] == 0
                ? PIEZA_NUMERO_6
                : PIEZA_NUMERO_7;
        }
    }
}


//==================================================
// ACTUALIZAR
//==================================================

void Minijuego67::Actualizar(
    float deltaTime,
    int cantidadMaxima,
    const Participante participantes[]
)
{
    AsegurarVistasEquipo67(*this);

    desplazamientoVisualCintas +=
        velocidadCintas * 8.5f * deltaTime;

    if (desplazamientoVisualCintas >= LARGO_CINTA_67)
    {
        desplazamientoVisualCintas =
            std::fmod(
                desplazamientoVisualCintas,
                LARGO_CINTA_67
            );
    }

    if (animacionIdleActiva)
    {
        fotogramaAnimacionIdle += 30.0f * deltaTime;

        int cantidadFotogramas =
            animacionesJugador[indiceAnimacionIdle]
                .keyframeCount;

        if (cantidadFotogramas > 0)
        {
            int fotogramaActual =
                (int)fotogramaAnimacionIdle %
                cantidadFotogramas;

            UpdateModelAnimation(
                modeloJugador,
                animacionesJugador[indiceAnimacionIdle],
                fotogramaActual
            );
        }
    }

    int mascaraActual =
        CrearMascaraParticipantes67(
            participantes,
            cantidadMaxima
        );

    if (mascaraActual != mascaraJugadoresEnPartida)
    {
        Reiniciar(
            participantes,
            cantidadMaxima
        );

        return;
    }

    if (!partidaValida)
    {
        return;
    }

    if (estadoPartida == FABRICA_67_PREPARANDO)
    {
        tiempoPreparacion -= deltaTime;

        if (tiempoPreparacion <= 0.0f)
        {
            tiempoPreparacion = 0.0f;
            estadoPartida = FABRICA_67_JUGANDO;
        }

        return;
    }

    if (estadoPartida != FABRICA_67_JUGANDO)
    {
        return;
    }

    tiempoPartida -= deltaTime;

    if (tiempoPartida <= 0.0f)
    {
        tiempoPartida = 0.0f;
        estadoPartida = FABRICA_67_FINALIZADO;

        FinalizarResultado67(
            *this,
            cantidadMaxima
        );

        return;
    }

    float porcentajeTiempo =
        1.0f -
        tiempoPartida / DURACION_PARTIDA_67;

    velocidadCintas =
        0.24f + porcentajeTiempo * 0.22f;

    for (int equipo = 0; equipo < 2; equipo++)
    {
        EstadoEquipo67& estadoEquipo =
            equipos[equipo];

        if (estadoEquipo.tiempoFeedback > 0.0f)
        {
            estadoEquipo.tiempoFeedback -= deltaTime;

            if (estadoEquipo.tiempoFeedback < 0.0f)
            {
                estadoEquipo.tiempoFeedback = 0.0f;
            }
        }

        if (estadoEquipo.tiempoMesaCompleta > 0.0f)
        {
            estadoEquipo.tiempoMesaCompleta -= deltaTime;

            if (estadoEquipo.tiempoMesaCompleta <= 0.0f)
            {
                estadoEquipo.tiempoMesaCompleta = 0.0f;
                estadoEquipo.mesa = MESA_67_VACIA;

                ActualizarTipoPiezaModoPrueba67(
                    *this,
                    equipo,
                    cantidadMaxima
                );
            }
        }
    }

    for (int i = 0; i < cantidadMaxima; i++)
    {
        if (
            equipoPorJugador[i] < 0 ||
            !participantes[i].activo ||
            !participantes[i].conectado
        )
        {
            continue;
        }

        EstadoJugador67& estado =
            estadosJugadores[i];

        if (estado.tiempoStun > 0.0f)
        {
            estado.tiempoStun -= deltaTime;

            if (estado.tiempoStun < 0.0f)
            {
                estado.tiempoStun = 0.0f;
            }
        }

        if (
            estado.llevaPieza &&
            estado.tiempoGiro > 0.0f
        )
        {
            estado.tiempoGiro -= deltaTime;

            if (estado.tiempoGiro <= 0.0f)
            {
                estado.tiempoGiro = 0.0f;
                estado.mirandoMesa = true;
            }
        }

        if (estado.objetoActivo)
        {
            estado.progresoObjeto +=
                velocidadCintas * deltaTime;

            if (estado.progresoObjeto > 1.06f)
            {
                ReiniciarObjetoCinta67(
                    estado,
                    DEMORA_REAPARICION_FALLO_67
                );
            }
        }
        else
        {
            estado.tiempoReaparicion -= deltaTime;

            if (estado.tiempoReaparicion <= 0.0f)
            {
                estado.objetoActivo = true;
                estado.progresoObjeto = -0.06f;
                estado.tiempoReaparicion = 0.0f;
            }
        }

        if (estado.tiempoStun > 0.0f)
        {
            continue;
        }

        InputMinijuegoParticipante entrada =
            LeerInputMinijuegoParticipante(
                participantes[i]
            );

        if (!entrada.saltar)
        {
            continue;
        }

        if (estado.llevaPieza)
        {
            if (estado.mirandoMesa)
            {
                ColocarPieza67(
                    *this,
                    i,
                    cantidadMaxima
                );
            }

            continue;
        }

        bool objetoEnFrente =
            estado.objetoActivo &&
            estado.progresoObjeto >=
                INICIO_ZONA_RECOGIDA_67 &&
            estado.progresoObjeto <=
                FIN_ZONA_RECOGIDA_67;

        if (objetoEnFrente)
        {
            estado.llevaPieza = true;
            estado.mirandoMesa = false;
            estado.tiempoGiro = DURACION_GIRO_67;

            ReiniciarObjetoCinta67(
                estado,
                DEMORA_REAPARICION_AGARRE_67
            );
        }
        else
        {
            int equipo = equipoPorJugador[i];

            estado.tiempoStun = 0.28f;
            equipos[equipo].ultimoAcierto = false;
            equipos[equipo].tiempoFeedback = 0.38f;
        }
    }
}


//==================================================
// DIBUJAR
//==================================================

void Minijuego67::Dibujar(
    int cantidadMaxima,
    const Participante participantes[]
) const
{
    ClearBackground(BLACK);

    if (!vistasEquipoCargadas)
    {
        const char* texto =
            "NO SE PUDO CREAR LA PANTALLA DIVIDIDA";

        DrawText(
            texto,
            GetScreenWidth() / 2 -
                MeasureText(texto, 26) / 2,
            GetScreenHeight() / 2 - 13,
            26,
            RED
        );

        return;
    }

    for (int equipo = 0; equipo < 2; equipo++)
    {
        DibujarVistaEquipo67(
            *this,
            equipo,
            cantidadMaxima,
            participantes
        );
    }

    int anchoPantalla = GetScreenWidth();
    int altoPantalla = GetScreenHeight();
    int anchoMitad = anchoPantalla / 2;

    Rectangle origen =
    {
        0.0f,
        0.0f,
        (float)anchoVistaEquipos,
        -(float)altoVistaEquipos
    };

    Rectangle destinoIzquierdo =
    {
        0.0f,
        0.0f,
        (float)anchoMitad,
        (float)altoPantalla
    };

    Rectangle destinoDerecho =
    {
        (float)anchoMitad,
        0.0f,
        (float)(anchoPantalla - anchoMitad),
        (float)altoPantalla
    };

    DrawTexturePro(
        vistasEquipo[0].texture,
        origen,
        destinoIzquierdo,
        { 0.0f, 0.0f },
        0.0f,
        WHITE
    );

    DrawTexturePro(
        vistasEquipo[1].texture,
        origen,
        destinoDerecho,
        { 0.0f, 0.0f },
        0.0f,
        WHITE
    );

    DrawRectangle(
        anchoMitad - 3,
        0,
        6,
        altoPantalla,
        BLACK
    );

    DrawRectangle(
        anchoMitad - 44,
        altoPantalla / 2 - 24,
        88,
        48,
        Fade(BLACK, 0.90f)
    );

    const char* textoVersus =
        jugadoresEnPartida == 4
        ? "VS"
        : "1V1";

    DrawText(
        textoVersus,
        anchoMitad -
            MeasureText(textoVersus, 22) / 2,
        altoPantalla / 2 - 11,
        22,
        RAYWHITE
    );

    if (estadoPartida == FABRICA_67_ESPERANDO_JUGADORES)
    {
        DrawRectangle(
            anchoPantalla / 2 - 330,
            altoPantalla / 2 - 68,
            660,
            136,
            Fade(BLACK, 0.91f)
        );

        const char* titulo =
            "FABRICA 67 - EQUIPOS";

        const char* detalle =
            "SE NECESITAN 4 JUGADORES PARA 2 VS 2";

        const char* detallePrueba =
            "CON 2 JUGADORES SE ACTIVA EL MODO DE PRUEBA 1 VS 1";

        DrawText(
            titulo,
            anchoPantalla / 2 -
                MeasureText(titulo, 30) / 2,
            altoPantalla / 2 - 48,
            30,
            ORANGE
        );

        DrawText(
            detalle,
            anchoPantalla / 2 -
                MeasureText(detalle, 19) / 2,
            altoPantalla / 2 - 2,
            19,
            RAYWHITE
        );

        DrawText(
            detallePrueba,
            anchoPantalla / 2 -
                MeasureText(detallePrueba, 16) / 2,
            altoPantalla / 2 + 30,
            16,
            LIGHTGRAY
        );

        return;
    }

    if (estadoPartida == FABRICA_67_PREPARANDO)
    {
        int numeroCuenta =
            (int)std::ceil(tiempoPreparacion);

        if (numeroCuenta < 1)
        {
            numeroCuenta = 1;
        }

        const char* texto =
            TextFormat("%d", numeroCuenta);

        DrawCircle(
            anchoPantalla / 2,
            altoPantalla / 2,
            58.0f,
            Fade(BLACK, 0.86f)
        );

        DrawText(
            texto,
            anchoPantalla / 2 -
                MeasureText(texto, 58) / 2,
            altoPantalla / 2 - 30,
            58,
            YELLOW
        );

        return;
    }

    if (estadoPartida == FABRICA_67_FINALIZADO)
    {
        DrawRectangle(
            anchoPantalla / 2 - 330,
            altoPantalla / 2 - 78,
            660,
            156,
            Fade(BLACK, 0.92f)
        );

        const char* tituloFinal = nullptr;
        Color colorTitulo = RAYWHITE;

        if (empate)
        {
            tituloFinal = "EMPATE";
            colorTitulo = YELLOW;
        }
        else
        {
            tituloFinal =
                TextFormat(
                    "GANA EL EQUIPO %d",
                    equipoGanador + 1
                );

            colorTitulo =
                ObtenerColorEquipo67(equipoGanador);
        }

        DrawText(
            tituloFinal,
            anchoPantalla / 2 -
                MeasureText(tituloFinal, 34) / 2,
            altoPantalla / 2 - 52,
            34,
            colorTitulo
        );

        const char* marcador =
            TextFormat(
                "%d  -  %d",
                equipos[0].puntos,
                equipos[1].puntos
            );

        DrawText(
            marcador,
            anchoPantalla / 2 -
                MeasureText(marcador, 30) / 2,
            altoPantalla / 2 + 4,
            30,
            RAYWHITE
        );

        DrawText(
            "R PARA REINICIAR EN ZONA DE PRUEBAS",
            anchoPantalla / 2 -
                MeasureText(
                    "R PARA REINICIAR EN ZONA DE PRUEBAS",
                    16
                ) / 2,
            altoPantalla / 2 + 48,
            16,
            LIGHTGRAY
        );
    }
}


const ResultadoMinijuego& Minijuego67::ObtenerResultado() const
{
    return resultado;
}


//==================================================
// DESCARGAR
//==================================================

void Minijuego67::Descargar()
{
    DescargarVistasEquipo67(*this);

    if (animacionesJugador != nullptr)
    {
        UnloadModelAnimations(
            animacionesJugador,
            cantidadAnimacionesJugador
        );

        animacionesJugador = nullptr;
        cantidadAnimacionesJugador = 0;
        indiceAnimacionIdle = -1;
        animacionIdleActiva = false;
    }

    if (modeloJugadorCargado)
    {
        UnloadModel(modeloJugador);

        modeloJugador = {};
        modeloJugadorCargado = false;
    }
}