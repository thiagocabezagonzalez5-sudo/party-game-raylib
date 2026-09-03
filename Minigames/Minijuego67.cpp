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
static const float X_MESA_67 = 4.55f;

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
        ? Color{ 235, 142, 48, 255 }
        : Color{ 65, 157, 232, 255 };
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
    int ancho = GetScreenWidth();
    int alto = GetScreenHeight() / 2;

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
    Color colorMesa =
        tiempoCompleta > 0.0f
        ? Color{ 76, 183, 97, 255 }
        : Color{ 126, 77, 43, 255 };

    DrawCube(
        { X_MESA_67, 0.80f, 0.0f },
        1.95f,
        0.22f,
        3.25f,
        colorMesa
    );

    DrawCubeWires(
        { X_MESA_67, 0.80f, 0.0f },
        1.95f,
        0.22f,
        3.25f,
        BLACK
    );

    for (int ladoX = -1; ladoX <= 1; ladoX += 2)
    {
        for (int ladoZ = -1; ladoZ <= 1; ladoZ += 2)
        {
            DrawCube(
                {
                    X_MESA_67 + ladoX * 0.70f,
                    0.36f,
                    ladoZ * 1.18f
                },
                0.18f,
                0.78f,
                0.18f,
                Color{ 78, 48, 29, 255 }
            );
        }
    }

    DrawCube(
        { X_MESA_67, 1.02f, 0.0f },
        1.68f,
        0.08f,
        2.92f,
        Fade(colorEquipo, 0.28f)
    );

    if (
        estado == MESA_67_CON_6 ||
        estado == MESA_67_COMPLETA
    )
    {
        DibujarNumero3D67(
            6,
            { X_MESA_67 - 0.38f, 1.48f, 0.0f },
            0.74f,
            ORANGE
        );
    }

    if (estado == MESA_67_COMPLETA)
    {
        DibujarNumero3D67(
            7,
            { X_MESA_67 + 0.38f, 1.48f, 0.0f },
            0.74f,
            SKYBLUE
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
        { 17.5f, 9.5f },
        Color{ 101, 103, 107, 255 }
    );

    DrawCube(
        { 0.0f, 2.45f, -4.35f },
        16.5f,
        5.5f,
        0.25f,
        Color{ 76, 83, 92, 255 }
    );

    DrawCube(
        { -7.2f, 2.45f, 0.0f },
        0.35f,
        5.5f,
        8.6f,
        Color{ 65, 71, 79, 255 }
    );

    for (int i = 0; i < 5; i++)
    {
        DrawCube(
            { -5.8f + i * 2.85f, 4.02f, -4.18f },
            1.45f,
            1.18f,
            0.08f,
            Fade(colorEquipo, 0.70f)
        );

        DrawCubeWires(
            { -5.8f + i * 2.85f, 4.02f, -4.18f },
            1.45f,
            1.18f,
            0.08f,
            BLACK
        );
    }

    for (int i = 0; i < 4; i++)
    {
        DrawCube(
            { -5.8f + i * 3.8f, 2.2f, -4.05f },
            0.32f,
            4.3f,
            0.32f,
            Color{ 51, 56, 63, 255 }
        );
    }

    DrawCube(
        { 5.9f, 3.15f, -4.02f },
        2.2f,
        0.22f,
        0.18f,
        colorEquipo
    );
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
        14,
        12,
        ancho - 28,
        58,
        Fade(BLACK, 0.72f)
    );

    DrawRectangle(
        14,
        12,
        8,
        58,
        colorEquipo
    );

    DrawText(
        TextFormat("EQUIPO %d", equipo + 1),
        32,
        22,
        24,
        RAYWHITE
    );

    DrawText(
        TextFormat("PUNTOS: %d", estadoEquipo.puntos),
        ancho / 2 - 65,
        22,
        24,
        LIME
    );

    DrawText(
        TextFormat("TIEMPO: %.1f", minijuego.tiempoPartida),
        ancho - 178,
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
        32,
        48,
        15,
        LIGHTGRAY
    );

    if (estadoEquipo.tiempoFeedback > 0.0f)
    {
        const char* textoFeedback =
            estadoEquipo.ultimoAcierto
            ? "BIEN!"
            : "ORDEN INCORRECTO";

        DrawText(
            textoFeedback,
            ancho / 2 -
                MeasureText(textoFeedback, 22) / 2,
            80,
            22,
            estadoEquipo.ultimoAcierto
            ? LIME
            : RED
        );
    }

    int indicesEquipo[2] = { -1, -1 };
    int cantidadEquipo = 0;

    for (int i = 0; i < cantidadMaxima; i++)
    {
        if (
            minijuego.equipoPorJugador[i] == equipo &&
            participantes[i].activo &&
            participantes[i].conectado &&
            cantidadEquipo < 2
        )
        {
            indicesEquipo[cantidadEquipo] = i;
            cantidadEquipo++;
        }
    }

    if (cantidadEquipo <= 0)
    {
        return;
    }

    int separacion = 8;
    int margen = 14;
    int anchoTarjeta =
        cantidadEquipo == 1
        ? 360
        : (ancho - margen * 2 - separacion) / 2;

    if (anchoTarjeta > 390)
    {
        anchoTarjeta = 390;
    }

    int anchoTotal =
        anchoTarjeta * cantidadEquipo +
        separacion * (cantidadEquipo - 1);

    int xInicial =
        (ancho - anchoTotal) / 2;

    int yTarjeta = alto - 64;

    for (int orden = 0; orden < cantidadEquipo; orden++)
    {
        int indiceJugador = indicesEquipo[orden];

        if (indiceJugador < 0)
        {
            continue;
        }

        const EstadoJugador67& estado =
            minijuego.estadosJugadores[indiceJugador];

        int x =
            xInicial +
            orden * (anchoTarjeta + separacion);

        DrawRectangle(
            x,
            yTarjeta,
            anchoTarjeta,
            50,
            Fade(BLACK, 0.78f)
        );

        DrawRectangleLinesEx(
            Rectangle{
                (float)x,
                (float)yTarjeta,
                (float)anchoTarjeta,
                50.0f
            },
            2.0f,
            participantes[indiceJugador].color
        );

        const char* estadoTexto =
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
                "J%d  NUM %d  [%s]",
                participantes[indiceJugador].numeroJugador,
                estado.tipoPieza == PIEZA_NUMERO_6 ? 6 : 7,
                ObtenerTextoBotonPrincipal(
                    participantes[indiceJugador]
                )
            ),
            x + 8,
            yTarjeta + 6,
            15,
            RAYWHITE
        );

        DrawText(
            estadoTexto,
            x + 8,
            yTarjeta + 27,
            15,
            estado.tiempoStun > 0.0f
            ? RED
            : SKYBLUE
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
        ? Color{ 112, 148, 169, 255 }
        : Color{ 102, 141, 171, 255 }
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
            { 10.4f, 7.0f, 10.6f };

        camarasEquipo[equipo].target =
            { 0.0f, 0.82f, 0.0f };

        camarasEquipo[equipo].up =
            { 0.0f, 1.0f, 0.0f };

        camarasEquipo[equipo].fovy = 52.0f;
        camarasEquipo[equipo].projection = CAMERA_PERSPECTIVE;
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
    int altoMitad = altoPantalla / 2;

    Rectangle origen =
    {
        0.0f,
        0.0f,
        (float)anchoVistaEquipos,
        -(float)altoVistaEquipos
    };

    Rectangle destinoSuperior =
    {
        0.0f,
        0.0f,
        (float)anchoPantalla,
        (float)altoMitad
    };

    Rectangle destinoInferior =
    {
        0.0f,
        (float)altoMitad,
        (float)anchoPantalla,
        (float)(altoPantalla - altoMitad)
    };

    DrawTexturePro(
        vistasEquipo[0].texture,
        origen,
        destinoSuperior,
        { 0.0f, 0.0f },
        0.0f,
        WHITE
    );

    DrawTexturePro(
        vistasEquipo[1].texture,
        origen,
        destinoInferior,
        { 0.0f, 0.0f },
        0.0f,
        WHITE
    );

    DrawRectangle(
        0,
        altoMitad - 3,
        anchoPantalla,
        6,
        BLACK
    );

    DrawRectangle(
        anchoPantalla / 2 - 58,
        altoMitad - 18,
        116,
        36,
        Fade(BLACK, 0.90f)
    );

    const char* textoVersus =
        jugadoresEnPartida == 4
        ? "2 VS 2"
        : "1 VS 1";

    DrawText(
        textoVersus,
        anchoPantalla / 2 -
            MeasureText(textoVersus, 20) / 2,
        altoMitad - 10,
        20,
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
