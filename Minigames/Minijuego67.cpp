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

// La ventana anterior era demasiado ancha. Ahora hay que esperar
// a que la pieza quede realmente a mano del jugador.
static const float INICIO_ZONA_RECOGIDA_67 = 0.755f;
static const float FIN_ZONA_RECOGIDA_67 = 0.815f;

static const float X_INICIO_CINTA_67 = -6.3f;
static const float LARGO_CINTA_67 = 9.0f;
static const float X_JUGADOR_67 = 1.35f;
static const float X_MESA_67 = 3.45f;

static const float Z_CINTA_6_67 = -2.35f;
static const float Z_CINTA_7_67 = 2.35f;
static const float Z_JUGADOR_6_67 = -0.72f;
static const float Z_JUGADOR_7_67 = 0.72f;

// Antes se mostraban siete piezas por cinta. Cuatro dejan mas aire
// visual y obligan a leer mejor el ritmo de la cinta.
static const int CANTIDAD_PIEZAS_CINTA_67 = 4;
static const float DESFASE_CINTA_7_67 = 0.125f;

static const float DURACION_GIRO_67 = 0.24f;
static const float DURACION_STUN_67 = 0.42f;
static const float DURACION_MESA_COMPLETA_67 = 0.48f;
static const float DURACION_FEEDBACK_67 = 0.55f;
static const float COOLDOWN_TRAS_AGARRAR_67 = 0.30f;
static const float COOLDOWN_TRAS_COLOCAR_67 = 0.38f;


//==================================================
// UTILIDADES
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
                mascara |= 1 << (i + MAX_JUGADORES_PRUEBA);
            }
        }
    }

    return mascara;
}


static float NormalizarProgreso67(float progreso)
{
    progreso = std::fmod(progreso, 1.0f);

    if (progreso < 0.0f)
    {
        progreso += 1.0f;
    }

    return progreso;
}


static float ObtenerXObjeto67(float progreso)
{
    return X_INICIO_CINTA_67 + progreso * LARGO_CINTA_67;
}


static float ObtenerProgresoPieza67(
    float progresoBase,
    int indicePieza
)
{
    float separacion =
        1.0f / (float)CANTIDAD_PIEZAS_CINTA_67;

    return NormalizarProgreso67(
        progresoBase +
        separacion * (float)indicePieza
    );
}


static bool HayPiezaAlAlcance67(
    const EstadoEquipo67& equipo,
    TipoPieza67 tipo
)
{
    float progresoBase =
        tipo == PIEZA_NUMERO_6
        ? equipo.progresoCinta6
        : equipo.progresoCinta7;

    for (int i = 0; i < CANTIDAD_PIEZAS_CINTA_67; i++)
    {
        float progreso =
            ObtenerProgresoPieza67(
                progresoBase,
                i
            );

        if (
            progreso >= INICIO_ZONA_RECOGIDA_67 &&
            progreso <= FIN_ZONA_RECOGIDA_67
        )
        {
            return true;
        }
    }

    return false;
}


static Color ObtenerColorEquipo67(int equipo)
{
    return equipo == 0
        ? Color{ 238, 55, 66, 255 }
        : Color{ 40, 159, 224, 255 };
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


static void ActualizarRolJugadorUnico67(
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

    int indice =
        BuscarJugadorUnicoEquipo67(
            minijuego,
            equipo,
            cantidadMaxima
        );

    if (indice < 0)
    {
        return;
    }

    minijuego.estadosJugadores[indice].tipoPieza =
        minijuego.equipos[equipo].mesa == MESA_67_CON_6
        ? PIEZA_NUMERO_7
        : PIEZA_NUMERO_6;
}


static float ObtenerZJugador67(
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
        ? Z_JUGADOR_6_67
        : Z_JUGADOR_7_67;
}


//==================================================
// RENDER TEXTURES
//==================================================

static void DescargarVistasEquipo67(
    Minijuego67& minijuego
)
{
    for (int equipo = 0; equipo < 2; equipo++)
    {
        if (minijuego.vistasEquipo[equipo].id != 0)
        {
            UnloadRenderTexture(minijuego.vistasEquipo[equipo]);
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

    minijuego.vistasEquipo[0] = LoadRenderTexture(ancho, alto);
    minijuego.vistasEquipo[1] = LoadRenderTexture(ancho, alto);

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
// RESULTADO
//==================================================

static void FinalizarResultado67(
    Minijuego67& minijuego,
    int cantidadMaxima
)
{
    if (
        minijuego.resultado.estado != RESULTADO_MINIJUEGO_EN_CURSO ||
        !minijuego.partidaValida
    )
    {
        return;
    }

    int puntos0 = minijuego.equipos[0].puntos;
    int puntos1 = minijuego.equipos[1].puntos;

    minijuego.empate = puntos0 == puntos1;
    minijuego.equipoGanador = -1;

    if (!minijuego.empate)
    {
        minijuego.equipoGanador = puntos0 > puntos1 ? 0 : 1;
    }

    minijuego.resultado.estado = RESULTADO_MINIJUEGO_FINALIZADO;
    minijuego.resultado.desenlace =
        minijuego.empate
        ? DESENLACE_EMPATE
        : DESENLACE_CON_GANADOR;
    minijuego.resultado.cantidadEquipos = 2;

    for (int i = 0; i < cantidadMaxima; i++)
    {
        ResultadoParticipante& resultadoJugador =
            minijuego.resultado.participantes[i];

        if (!resultadoJugador.participo)
        {
            continue;
        }

        int equipo = minijuego.equipoPorJugador[i];
        resultadoJugador.numeroEquipo = equipo;
        resultadoJugador.puntuacionMinijuego =
            equipo >= 0 && equipo < 2
            ? minijuego.equipos[equipo].puntos
            : 0;
        resultadoJugador.puntosObtenidos = 0;

        if (minijuego.empate)
        {
            resultadoJugador.posicionFinal = 1;
        }
        else
        {
            resultadoJugador.posicionFinal =
                equipo == minijuego.equipoGanador ? 1 : 2;
        }
    }
}


//==================================================
// LOGICA DE PIEZAS
//==================================================

static void ColocarPieza67(
    Minijuego67& minijuego,
    int indiceJugador,
    int cantidadMaxima
)
{
    int equipo = minijuego.equipoPorJugador[indiceJugador];

    if (equipo < 0 || equipo > 1)
    {
        return;
    }

    EstadoJugador67& jugador =
        minijuego.estadosJugadores[indiceJugador];

    EstadoEquipo67& estadoEquipo =
        minijuego.equipos[equipo];

    bool correcto = false;

    if (
        jugador.tipoPieza == PIEZA_NUMERO_6 &&
        estadoEquipo.mesa == MESA_67_VACIA
    )
    {
        estadoEquipo.mesa = MESA_67_CON_6;
        correcto = true;
    }
    else if (
        jugador.tipoPieza == PIEZA_NUMERO_7 &&
        estadoEquipo.mesa == MESA_67_CON_6
    )
    {
        estadoEquipo.mesa = MESA_67_COMPLETA;
        estadoEquipo.tiempoMesaCompleta = DURACION_MESA_COMPLETA_67;
        estadoEquipo.puntos++;
        correcto = true;
    }

    estadoEquipo.ultimoAcierto = correcto;
    estadoEquipo.tiempoFeedback = DURACION_FEEDBACK_67;

    jugador.llevaPieza = false;
    jugador.mirandoMesa = false;
    jugador.tiempoGiro = 0.0f;
    jugador.tiempoCooldownInteraccion = COOLDOWN_TRAS_COLOCAR_67;

    if (!correcto)
    {
        jugador.tiempoStun = DURACION_STUN_67;
    }

    if (
        correcto &&
        minijuego.cantidadJugadoresEquipo[equipo] == 1
    )
    {
        ActualizarRolJugadorUnico67(
            minijuego,
            equipo,
            cantidadMaxima
        );
    }
}


//==================================================
// DIBUJO DE NUMEROS Y FABRICA
//==================================================

static void DibujarSegmentoNumero67(
    Vector3 posicion,
    bool horizontal,
    float escala,
    Color color
)
{
    DrawCube(
        posicion,
        (horizontal ? 0.62f : 0.13f) * escala,
        (horizontal ? 0.13f : 0.52f) * escala,
        0.20f * escala,
        color
    );
}


static void DibujarNumero3D67(
    int numero,
    Vector3 centro,
    float escala,
    Color color
)
{
    const float xLado = 0.31f * escala;
    const float yExtremo = 0.55f * escala;
    const float yMedio = 0.275f * escala;

    bool segmentos[7]{};

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

    if (segmentos[0]) DibujarSegmentoNumero67({centro.x, centro.y + yExtremo, centro.z}, true, escala, color);
    if (segmentos[1]) DibujarSegmentoNumero67({centro.x + xLado, centro.y + yMedio, centro.z}, false, escala, color);
    if (segmentos[2]) DibujarSegmentoNumero67({centro.x + xLado, centro.y - yMedio, centro.z}, false, escala, color);
    if (segmentos[3]) DibujarSegmentoNumero67({centro.x, centro.y - yExtremo, centro.z}, true, escala, color);
    if (segmentos[4]) DibujarSegmentoNumero67({centro.x - xLado, centro.y - yMedio, centro.z}, false, escala, color);
    if (segmentos[5]) DibujarSegmentoNumero67({centro.x - xLado, centro.y + yMedio, centro.z}, false, escala, color);
    if (segmentos[6]) DibujarSegmentoNumero67(centro, true, escala, color);
}


static void DibujarCinta67(
    float z,
    Color colorEquipo
)
{
    float centroX = X_INICIO_CINTA_67 + LARGO_CINTA_67 / 2.0f;

    DrawCube(
        { centroX, 0.20f, z },
        LARGO_CINTA_67 + 0.40f,
        0.42f,
        1.22f,
        Color{ 45, 49, 56, 255 }
    );

    DrawCube(
        { centroX, 0.46f, z },
        LARGO_CINTA_67,
        0.08f,
        1.02f,
        Color{ 88, 95, 105, 255 }
    );

    for (int i = 0; i < 12; i++)
    {
        float x = X_INICIO_CINTA_67 + 0.35f + i * 0.72f;

        DrawCube(
            { x, 0.52f, z },
            0.06f,
            0.03f,
            0.98f,
            Color{ 177, 184, 192, 255 }
        );
    }

    float xInicio = ObtenerXObjeto67(INICIO_ZONA_RECOGIDA_67);
    float xFin = ObtenerXObjeto67(FIN_ZONA_RECOGIDA_67);

    DrawCube(
        { (xInicio + xFin) / 2.0f, 0.575f, z },
        xFin - xInicio,
        0.035f,
        1.07f,
        Fade(YELLOW, 0.58f)
    );

    for (int lado = -1; lado <= 1; lado += 2)
    {
        DrawCube(
            { centroX, 0.65f, z + lado * 0.61f },
            LARGO_CINTA_67 + 0.45f,
            0.17f,
            0.10f,
            Color{ 39, 43, 49, 255 }
        );
    }

    DrawCube(
        { X_INICIO_CINTA_67 - 0.42f, 1.1f, z },
        0.65f,
        2.0f,
        1.5f,
        Color{ 55, 60, 68, 255 }
    );

    DrawCube(
        { X_INICIO_CINTA_67 - 0.06f, 1.18f, z },
        0.07f,
        1.25f,
        1.08f,
        Fade(colorEquipo, 0.78f)
    );
}


static void DibujarFlujoPiezas67(
    const EstadoEquipo67& equipo
)
{
    for (int i = 0; i < CANTIDAD_PIEZAS_CINTA_67; i++)
    {
        float progreso6 =
            ObtenerProgresoPieza67(equipo.progresoCinta6, i);

        float progreso7 =
            ObtenerProgresoPieza67(equipo.progresoCinta7, i);

        DibujarNumero3D67(
            6,
            { ObtenerXObjeto67(progreso6), 0.96f, Z_CINTA_6_67 },
            0.56f,
            ORANGE
        );

        DibujarNumero3D67(
            7,
            { ObtenerXObjeto67(progreso7), 0.96f, Z_CINTA_7_67 },
            0.56f,
            SKYBLUE
        );
    }
}


static void DibujarMesa67(
    const EstadoEquipo67& equipo,
    Color colorEquipo
)
{
    // Mesa mas chica: la zona de colocacion deja de dominar el pasillo.
    DrawCylinder(
        { X_MESA_67, 0.12f, 0.0f },
        0.88f,
        0.88f,
        0.22f,
        28,
        Color{ 25, 27, 31, 255 }
    );

    DrawCylinder(
        { X_MESA_67, 0.27f, 0.0f },
        0.72f,
        0.72f,
        0.10f,
        28,
        equipo.tiempoMesaCompleta > 0.0f
            ? Color{ 72, 188, 99, 255 }
            : Color{ 49, 52, 58, 255 }
    );

    DrawCylinder(
        { X_MESA_67, 0.35f, 0.0f },
        0.60f,
        0.60f,
        0.04f,
        28,
        Fade(colorEquipo, 0.55f)
    );

    DibujarNumero3D67(
        6,
        { X_MESA_67 - 0.26f, 0.73f, 0.0f },
        0.37f,
        equipo.mesa == MESA_67_VACIA
            ? Fade(RAYWHITE, 0.25f)
            : ORANGE
    );

    DibujarNumero3D67(
        7,
        { X_MESA_67 + 0.27f, 0.73f, 0.0f },
        0.37f,
        equipo.mesa == MESA_67_COMPLETA
            ? SKYBLUE
            : Fade(RAYWHITE, 0.25f)
    );
}


static void DibujarJugador67(
    const Minijuego67& minijuego,
    int indice,
    const Participante& participante
)
{
    const EstadoJugador67& estado = minijuego.estadosJugadores[indice];
    float z = ObtenerZJugador67(minijuego, indice);

    Color color =
        estado.tiempoStun > 0.0f
        ? Fade(RED, 0.72f)
        : participante.color;

    float giro = -90.0f;

    if (estado.llevaPieza)
    {
        float progreso = 1.0f;

        if (estado.tiempoGiro > 0.0f)
        {
            progreso =
                1.0f - estado.tiempoGiro / DURACION_GIRO_67;
        }

        if (progreso < 0.0f) progreso = 0.0f;
        if (progreso > 1.0f) progreso = 1.0f;

        giro = -90.0f + 180.0f * progreso;
    }

    DrawCylinder(
        { X_JUGADOR_67, 0.07f, z },
        0.56f,
        0.56f,
        0.14f,
        24,
        Fade(participante.color, 0.62f)
    );

    if (minijuego.modeloJugadorCargado)
    {
        DrawModelEx(
            minijuego.modeloJugador,
            { X_JUGADOR_67, 0.16f, z },
            { 0.0f, 1.0f, 0.0f },
            giro,
            { 0.25f, 0.25f, 0.25f },
            color
        );
    }
    else
    {
        DrawCube(
            { X_JUGADOR_67, 0.90f, z },
            0.72f,
            1.55f,
            0.72f,
            color
        );
    }

    if (estado.llevaPieza)
    {
        bool es6 = estado.tipoPieza == PIEZA_NUMERO_6;

        DibujarNumero3D67(
            es6 ? 6 : 7,
            { X_JUGADOR_67, 2.10f, z },
            0.52f,
            es6 ? ORANGE : SKYBLUE
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
            ? Color{ 76, 43, 47, 255 }
            : Color{ 36, 59, 75, 255 }
    );

    DrawCube(
        { 0.0f, 2.35f, -4.55f },
        17.0f,
        5.3f,
        0.25f,
        Color{ 63, 68, 76, 255 }
    );

    DrawCube(
        { 0.0f, 4.55f, -4.38f },
        16.2f,
        0.50f,
        0.10f,
        Fade(colorEquipo, 0.82f)
    );

    // Pasillo central donde trabajan ambos jugadores.
    DrawCube(
        { -0.5f, -0.03f, 0.0f },
        12.6f,
        0.05f,
        2.45f,
        Fade(RAYWHITE, 0.12f)
    );
}


static const char* TextoMesa67(EstadoMesa67 mesa)
{
    if (mesa == MESA_67_VACIA) return "FALTA 6";
    if (mesa == MESA_67_CON_6) return "FALTA 7";
    return "67 COMPLETO +1";
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

    DrawRectangle(12, 10, ancho - 24, 76, Fade(BLACK, 0.74f));
    DrawRectangle(12, 10, 7, 76, ObtenerColorEquipo67(equipo));

    DrawText(TextFormat("EQUIPO %d", equipo + 1), 29, 17, 23, RAYWHITE);
    DrawText(TextFormat("PUNTOS %d", minijuego.equipos[equipo].puntos), 29, 48, 18, LIME);

    const char* tiempo = TextFormat("%.1f s", minijuego.tiempoPartida);
    DrawText(
        tiempo,
        ancho - MeasureText(tiempo, 21) - 22,
        19,
        21,
        minijuego.tiempoPartida <= 7.0f ? RED : SKYBLUE
    );

    const char* mesa = TextoMesa67(minijuego.equipos[equipo].mesa);
    DrawText(
        mesa,
        ancho - MeasureText(mesa, 15) - 22,
        50,
        15,
        LIGHTGRAY
    );

    int y = 106;

    for (int i = 0; i < cantidadMaxima; i++)
    {
        if (
            minijuego.equipoPorJugador[i] != equipo ||
            !participantes[i].activo
        )
        {
            continue;
        }

        const EstadoJugador67& jugador = minijuego.estadosJugadores[i];
        int numero = jugador.tipoPieza == PIEZA_NUMERO_6 ? 6 : 7;

        const char* accion =
            jugador.tiempoStun > 0.0f
            ? "FALLO"
            : (
                jugador.tiempoCooldownInteraccion > 0.0f
                ? "ESPERA"
                : (jugador.llevaPieza ? "COLOCAR" : "AGARRAR")
            );

        DrawText(
            TextFormat(
                "J%d %s  %d  [%s] %s",
                participantes[i].numeroJugador,
                participantes[i].esBot ? "BOT" : "",
                numero,
                ObtenerTextoBotonPrincipal(participantes[i]),
                accion
            ),
            25,
            y,
            15,
            jugador.tiempoStun > 0.0f
                ? RED
                : participantes[i].color
        );

        y += 24;
    }

    if (minijuego.equipos[equipo].tiempoFeedback > 0.0f)
    {
        const char* texto =
            minijuego.equipos[equipo].ultimoAcierto
            ? "BIEN!"
            : "ORDEN INCORRECTO";

        DrawText(
            texto,
            25,
            alto - 85,
            22,
            minijuego.equipos[equipo].ultimoAcierto ? LIME : RED
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
    BeginTextureMode(minijuego.vistasEquipo[equipo]);

    ClearBackground(
        equipo == 0
            ? Color{ 92, 28, 34, 255 }
            : Color{ 20, 70, 100, 255 }
    );

    BeginMode3D(minijuego.camarasEquipo[equipo]);

    DibujarFabricaEquipo67(equipo);
    DibujarCinta67(Z_CINTA_6_67, ObtenerColorEquipo67(equipo));
    DibujarCinta67(Z_CINTA_7_67, ObtenerColorEquipo67(equipo));
    DibujarFlujoPiezas67(minijuego.equipos[equipo]);
    DibujarMesa67(minijuego.equipos[equipo], ObtenerColorEquipo67(equipo));

    for (int i = 0; i < cantidadMaxima; i++)
    {
        if (
            minijuego.equipoPorJugador[i] == equipo &&
            participantes[i].activo &&
            participantes[i].conectado
        )
        {
            DibujarJugador67(minijuego, i, participantes[i]);
        }
    }

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
// INICIALIZACION
//==================================================

void Minijuego67::Inicializar()
{
    tiempoPreparacion = DURACION_PREPARACION_67;
    tiempoPartida = DURACION_PARTIDA_67;
    velocidadCintas = 0.23f;
    desplazamientoVisualCintas = 0.0f;
    jugadoresEnPartida = 0;
    mascaraJugadoresEnPartida = 0;
    equipoGanador = -1;
    partidaValida = false;
    empate = false;
    estadoPartida = FABRICA_67_ESPERANDO_JUGADORES;

    for (int equipo = 0; equipo < 2; equipo++)
    {
        camarasEquipo[equipo].position = { 7.8f, 10.7f, 12.4f };
        camarasEquipo[equipo].target = { -1.0f, 0.55f, 0.0f };
        camarasEquipo[equipo].up = { 0.0f, 1.0f, 0.0f };
        camarasEquipo[equipo].fovy = 12.2f;
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
        return;
    }

    modeloJugador.transform = MatrixRotateX(90.0f * DEG2RAD);

    animacionesJugador =
        LoadModelAnimations(
            rutaModeloJugador,
            &cantidadAnimacionesJugador
        );

    indiceAnimacionIdle = -1;

    for (
        int i = 0;
        animacionesJugador != nullptr && i < cantidadAnimacionesJugador;
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
        animacionesJugador[indiceAnimacionIdle].keyframeCount > 0;
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

    int activos = ContarParticipantesActivos67(participantes, cantidadMaxima);
    int disponibles = ContarParticipantesDisponibles67(participantes, cantidadMaxima);

    partidaValida =
        activos == disponibles &&
        (activos == 2 || activos == 4);

    jugadoresEnPartida = disponibles;
    mascaraJugadoresEnPartida =
        CrearMascaraParticipantes67(participantes, cantidadMaxima);

    resultado.cantidadEquipos = partidaValida ? 2 : 0;

    if (!partidaValida)
    {
        estadoPartida = FABRICA_67_ESPERANDO_JUGADORES;
        return;
    }

    int indices[MAX_PARTICIPANTES]{};
    int cantidadIndices = 0;

    for (int i = 0; i < cantidadMaxima; i++)
    {
        if (participantes[i].activo && participantes[i].conectado)
        {
            indices[cantidadIndices++] = i;
        }
    }

    for (int i = cantidadIndices - 1; i > 0; i--)
    {
        int otro = GetRandomValue(0, i);
        int temporal = indices[i];
        indices[i] = indices[otro];
        indices[otro] = temporal;
    }

    if (activos == 4)
    {
        for (int orden = 0; orden < 4; orden++)
        {
            int indice = indices[orden];
            int equipo = orden < 2 ? 0 : 1;
            int ordenEquipo = orden % 2;

            equipoPorJugador[indice] = equipo;
            ordenEnEquipoPorJugador[indice] = ordenEquipo;
            cantidadJugadoresEquipo[equipo]++;
            resultado.participantes[indice].numeroEquipo = equipo;
        }
    }
    else
    {
        for (int orden = 0; orden < 2; orden++)
        {
            int indice = indices[orden];
            equipoPorJugador[indice] = orden;
            ordenEnEquipoPorJugador[indice] = 0;
            cantidadJugadoresEquipo[orden] = 1;
            resultado.participantes[indice].numeroEquipo = orden;
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
    velocidadCintas = 0.23f;
    fotogramaAnimacionIdle = 0.0f;
    equipoGanador = -1;
    empate = false;

    for (int equipo = 0; equipo < 2; equipo++)
    {
        equipos[equipo] = {};
        equipos[equipo].progresoCinta6 =
            equipo == 0 ? 0.02f : 0.11f;
        equipos[equipo].progresoCinta7 =
            NormalizarProgreso67(
                equipos[equipo].progresoCinta6 +
                DESFASE_CINTA_7_67
            );
    }

    for (int i = 0; i < cantidadMaxima; i++)
    {
        estadosJugadores[i] = {};
    }

    PrepararEquipos(participantes, cantidadMaxima);

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
            estadosJugadores[i].tipoPieza = PIEZA_NUMERO_6;
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

    if (animacionIdleActiva)
    {
        fotogramaAnimacionIdle += 30.0f * deltaTime;

        int cantidadFotogramas =
            animacionesJugador[indiceAnimacionIdle].keyframeCount;

        if (cantidadFotogramas > 0)
        {
            UpdateModelAnimation(
                modeloJugador,
                animacionesJugador[indiceAnimacionIdle],
                (int)fotogramaAnimacionIdle % cantidadFotogramas
            );
        }
    }

    int mascaraActual =
        CrearMascaraParticipantes67(participantes, cantidadMaxima);

    if (mascaraActual != mascaraJugadoresEnPartida)
    {
        Reiniciar(participantes, cantidadMaxima);
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
        FinalizarResultado67(*this, cantidadMaxima);
        return;
    }

    float progresoPartida =
        1.0f - tiempoPartida / DURACION_PARTIDA_67;

    velocidadCintas = 0.23f + progresoPartida * 0.16f;

    for (int equipo = 0; equipo < 2; equipo++)
    {
        EstadoEquipo67& estadoEquipo = equipos[equipo];

        estadoEquipo.progresoCinta6 =
            NormalizarProgreso67(
                estadoEquipo.progresoCinta6 +
                velocidadCintas * deltaTime
            );

        estadoEquipo.progresoCinta7 =
            NormalizarProgreso67(
                estadoEquipo.progresoCinta7 +
                velocidadCintas * deltaTime
            );

        if (estadoEquipo.tiempoFeedback > 0.0f)
        {
            estadoEquipo.tiempoFeedback -= deltaTime;
            if (estadoEquipo.tiempoFeedback < 0.0f)
                estadoEquipo.tiempoFeedback = 0.0f;
        }

        if (estadoEquipo.tiempoMesaCompleta > 0.0f)
        {
            estadoEquipo.tiempoMesaCompleta -= deltaTime;

            if (estadoEquipo.tiempoMesaCompleta <= 0.0f)
            {
                estadoEquipo.tiempoMesaCompleta = 0.0f;
                estadoEquipo.mesa = MESA_67_VACIA;
                ActualizarRolJugadorUnico67(*this, equipo, cantidadMaxima);
            }
        }
    }

    for (int i = 0; i < cantidadMaxima; i++)
    {
        int equipo = equipoPorJugador[i];

        if (
            equipo < 0 ||
            !participantes[i].activo ||
            !participantes[i].conectado
        )
        {
            continue;
        }

        EstadoJugador67& jugador = estadosJugadores[i];

        if (jugador.tiempoStun > 0.0f)
        {
            jugador.tiempoStun -= deltaTime;
            if (jugador.tiempoStun < 0.0f) jugador.tiempoStun = 0.0f;
        }

        if (jugador.tiempoCooldownInteraccion > 0.0f)
        {
            jugador.tiempoCooldownInteraccion -= deltaTime;
            if (jugador.tiempoCooldownInteraccion < 0.0f)
                jugador.tiempoCooldownInteraccion = 0.0f;
        }

        if (jugador.llevaPieza && jugador.tiempoGiro > 0.0f)
        {
            jugador.tiempoGiro -= deltaTime;

            if (jugador.tiempoGiro <= 0.0f)
            {
                jugador.tiempoGiro = 0.0f;
                jugador.mirandoMesa = true;
            }
        }

        if (
            jugador.tiempoStun > 0.0f ||
            jugador.tiempoCooldownInteraccion > 0.0f
        )
        {
            continue;
        }

        InputMinijuegoParticipante entrada =
            LeerInputMinijuegoParticipante(participantes[i]);

        if (!entrada.saltar)
        {
            continue;
        }

        if (jugador.llevaPieza)
        {
            if (jugador.mirandoMesa)
            {
                ColocarPieza67(*this, i, cantidadMaxima);
            }

            continue;
        }

        bool alAlcance =
            HayPiezaAlAlcance67(
                equipos[equipo],
                jugador.tipoPieza
            );

        if (alAlcance)
        {
            jugador.llevaPieza = true;
            jugador.mirandoMesa = false;
            jugador.tiempoGiro = DURACION_GIRO_67;
            jugador.tiempoCooldownInteraccion = COOLDOWN_TRAS_AGARRAR_67;
        }
        else
        {
            jugador.tiempoStun = 0.22f;
            equipos[equipo].ultimoAcierto = false;
            equipos[equipo].tiempoFeedback = 0.34f;
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
        const char* texto = "NO SE PUDO CREAR LA PANTALLA DIVIDIDA";
        DrawText(
            texto,
            GetScreenWidth() / 2 - MeasureText(texto, 26) / 2,
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

    int ancho = GetScreenWidth();
    int alto = GetScreenHeight();
    int mitad = ancho / 2;

    Rectangle origen =
    {
        0.0f,
        0.0f,
        (float)anchoVistaEquipos,
        -(float)altoVistaEquipos
    };

    DrawTexturePro(
        vistasEquipo[0].texture,
        origen,
        { 0.0f, 0.0f, (float)mitad, (float)alto },
        { 0.0f, 0.0f },
        0.0f,
        WHITE
    );

    DrawTexturePro(
        vistasEquipo[1].texture,
        origen,
        { (float)mitad, 0.0f, (float)(ancho - mitad), (float)alto },
        { 0.0f, 0.0f },
        0.0f,
        WHITE
    );

    DrawRectangle(mitad - 3, 0, 6, alto, BLACK);

    if (estadoPartida == FABRICA_67_ESPERANDO_JUGADORES)
    {
        DrawRectangle(
            ancho / 2 - 330,
            alto / 2 - 70,
            660,
            140,
            Fade(BLACK, 0.91f)
        );

        DrawText(
            "FABRICA 67 - EQUIPOS",
            ancho / 2 - MeasureText("FABRICA 67 - EQUIPOS", 30) / 2,
            alto / 2 - 48,
            30,
            ORANGE
        );

        DrawText(
            "SE NECESITAN 4 JUGADORES PARA 2 VS 2",
            ancho / 2 - MeasureText("SE NECESITAN 4 JUGADORES PARA 2 VS 2", 18) / 2,
            alto / 2,
            18,
            RAYWHITE
        );

        DrawText(
            "CON 2 JUGADORES: MODO DE PRUEBA 1 VS 1",
            ancho / 2 - MeasureText("CON 2 JUGADORES: MODO DE PRUEBA 1 VS 1", 16) / 2,
            alto / 2 + 32,
            16,
            LIGHTGRAY
        );
        return;
    }

    if (estadoPartida == FABRICA_67_PREPARANDO)
    {
        int numero = (int)std::ceil(tiempoPreparacion);
        if (numero < 1) numero = 1;

        const char* texto = TextFormat("%d", numero);
        DrawCircle(ancho / 2, alto / 2, 58.0f, Fade(BLACK, 0.86f));
        DrawText(
            texto,
            ancho / 2 - MeasureText(texto, 58) / 2,
            alto / 2 - 30,
            58,
            YELLOW
        );
        return;
    }

    if (estadoPartida == FABRICA_67_FINALIZADO)
    {
        DrawRectangle(
            ancho / 2 - 330,
            alto / 2 - 78,
            660,
            156,
            Fade(BLACK, 0.92f)
        );

        const char* titulo =
            empate
            ? "EMPATE"
            : TextFormat("GANA EL EQUIPO %d", equipoGanador + 1);

        DrawText(
            titulo,
            ancho / 2 - MeasureText(titulo, 34) / 2,
            alto / 2 - 52,
            34,
            empate ? YELLOW : ObtenerColorEquipo67(equipoGanador)
        );

        const char* marcador =
            TextFormat("%d  -  %d", equipos[0].puntos, equipos[1].puntos);

        DrawText(
            marcador,
            ancho / 2 - MeasureText(marcador, 30) / 2,
            alto / 2 + 4,
            30,
            RAYWHITE
        );

        DrawText(
            "R PARA REINICIAR",
            ancho / 2 - MeasureText("R PARA REINICIAR", 17) / 2,
            alto / 2 + 49,
            17,
            LIGHTGRAY
        );
    }
}


const ResultadoMinijuego& Minijuego67::ObtenerResultado() const
{
    return resultado;
}


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
