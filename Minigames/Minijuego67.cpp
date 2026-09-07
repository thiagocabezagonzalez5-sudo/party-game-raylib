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

static const float INICIO_ZONA_RECOGIDA_67 = 0.67f;
static const float FIN_ZONA_RECOGIDA_67 = 0.86f;
static const float CENTRO_ZONA_RECOGIDA_67 =
    (INICIO_ZONA_RECOGIDA_67 + FIN_ZONA_RECOGIDA_67) / 2.0f;

static const float X_INICIO_CINTA_67 = -6.3f;
static const float LARGO_CINTA_67 = 9.0f;
static const float X_JUGADOR_67 = 1.45f;
static const float X_MESA_67 = 3.35f;

// Las dos cintas quedan a los lados y los jugadores trabajan
// dentro del pasillo central, en vez de estar parados sobre ellas.
static const float Z_CINTA_6_67 = -2.35f;
static const float Z_CINTA_7_67 = 2.35f;
static const float Z_JUGADOR_6_67 = -0.72f;
static const float Z_JUGADOR_7_67 = 0.72f;

static const int CANTIDAD_PIEZAS_VISUALES_67 = 7;
static const float DESFASE_CARRIL_7_67 =
    0.5f / (float)CANTIDAD_PIEZAS_VISUALES_67;

static const float DURACION_GIRO_67 = 0.24f;
static const float DURACION_STUN_67 = 0.42f;
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
    return
        X_INICIO_CINTA_67 +
        progreso * LARGO_CINTA_67;
}


static float ObtenerZCintaPorTipo67(
    TipoPieza67 tipo
)
{
    return tipo == PIEZA_NUMERO_6
        ? Z_CINTA_6_67
        : Z_CINTA_7_67;
}


static float ObtenerZJugadorEquipo67(
    const Minijuego67& minijuego,
    int indiceJugador
)
{
    int equipo =
        minijuego.equipoPorJugador[indiceJugador];

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


static float ObtenerProgresoPiezaVisual67(
    const EstadoJugador67& estado,
    TipoPieza67 tipo,
    int indicePieza
)
{
    float separacion =
        1.0f /
        (float)CANTIDAD_PIEZAS_VISUALES_67;

    float desfaseTipo =
        tipo == PIEZA_NUMERO_7
        ? DESFASE_CARRIL_7_67
        : 0.0f;

    return NormalizarProgreso67(
        estado.progresoObjeto +
        separacion * (float)indicePieza +
        desfaseTipo
    );
}


static bool HayPiezaAlAlcance67(
    const EstadoJugador67& estado,
    TipoPieza67 tipo
)
{
    float mejorDistancia = 1000.0f;

    for (
        int i = 0;
        i < CANTIDAD_PIEZAS_VISUALES_67;
        i++
    )
    {
        float progreso =
            ObtenerProgresoPiezaVisual67(
                estado,
                tipo,
                i
            );

        if (
            progreso < INICIO_ZONA_RECOGIDA_67 ||
            progreso > FIN_ZONA_RECOGIDA_67
        )
        {
            continue;
        }

        float distancia =
            std::fabs(
                progreso -
                CENTRO_ZONA_RECOGIDA_67
            );

        if (distancia < mejorDistancia)
        {
            mejorDistancia = distancia;
        }
    }

    return mejorDistancia < 1000.0f;
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


static int BuscarJugadorRol67(
    const Minijuego67& minijuego,
    int equipo,
    TipoPieza67 tipo,
    int cantidadMaxima
)
{
    for (int i = 0; i < cantidadMaxima; i++)
    {
        if (
            minijuego.equipoPorJugador[i] == equipo &&
            minijuego.estadosJugadores[i].tipoPieza == tipo
        )
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
// DIBUJO DE NUMEROS
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
        DibujarSegmentoNumero67(
            { centro.x, centro.y + Y_EXTREMO, centro.z },
            true, escala, color
        );

    if (segmentos[1])
        DibujarSegmentoNumero67(
            { centro.x + X_LADO, centro.y + Y_MEDIO_LADO, centro.z },
            false, escala, color
        );

    if (segmentos[2])
        DibujarSegmentoNumero67(
            { centro.x + X_LADO, centro.y - Y_MEDIO_LADO, centro.z },
            false, escala, color
        );

    if (segmentos[3])
        DibujarSegmentoNumero67(
            { centro.x, centro.y - Y_EXTREMO, centro.z },
            true, escala, color
        );

    if (segmentos[4])
        DibujarSegmentoNumero67(
            { centro.x - X_LADO, centro.y - Y_MEDIO_LADO, centro.z },
            false, escala, color
        );

    if (segmentos[5])
        DibujarSegmentoNumero67(
            { centro.x - X_LADO, centro.y + Y_MEDIO_LADO, centro.z },
            false, escala, color
        );

    if (segmentos[6])
        DibujarSegmentoNumero67(
            centro,
            true, escala, color
        );
}


//==================================================
// DIBUJO DE CINTAS Y PIEZAS
//==================================================

static void DibujarCinta67(
    float z,
    float desplazamiento,
    Color colorEquipo,
    TipoPieza67 tipo
)
{
    float centroX =
        X_INICIO_CINTA_67 +
        LARGO_CINTA_67 / 2.0f;

    DrawCube(
        { centroX, 0.20f, z },
        LARGO_CINTA_67 + 0.45f,
        0.40f,
        1.25f,
        Color{ 48, 52, 60, 255 }
    );

    DrawCubeWires(
        { centroX, 0.20f, z },
        LARGO_CINTA_67 + 0.45f,
        0.40f,
        1.25f,
        BLACK
    );

    DrawCube(
        { centroX, 0.44f, z },
        LARGO_CINTA_67,
        0.08f,
        1.02f,
        tipo == PIEZA_NUMERO_6
        ? Color{ 111, 88, 68, 255 }
        : Color{ 71, 91, 114, 255 }
    );

    for (int i = 0; i < 16; i++)
    {
        float avance =
            std::fmod(
                i * 0.58f + desplazamiento,
                LARGO_CINTA_67
            );

        float x = X_INICIO_CINTA_67 + avance;

        DrawCube(
            { x, 0.50f, z },
            0.065f,
            0.035f,
            0.98f,
            Color{ 194, 196, 201, 255 }
        );
    }

    float xZonaInicio =
        ObtenerXObjeto67(INICIO_ZONA_RECOGIDA_67);

    float xZonaFin =
        ObtenerXObjeto67(FIN_ZONA_RECOGIDA_67);

    DrawCube(
        {
            (xZonaInicio + xZonaFin) / 2.0f,
            0.535f,
            z
        },
        xZonaFin - xZonaInicio,
        0.035f,
        1.06f,
        Fade(YELLOW, 0.34f)
    );

    for (int lado = -1; lado <= 1; lado += 2)
    {
        DrawCube(
            { centroX, 0.60f, z + lado * 0.62f },
            LARGO_CINTA_67 + 0.45f,
            0.16f,
            0.10f,
            Color{ 42, 46, 53, 255 }
        );
    }

    for (int pata = 0; pata < 4; pata++)
    {
        float xPata =
            X_INICIO_CINTA_67 +
            0.7f +
            pata * 2.55f;

        DrawCube(
            { xPata, -0.02f, z },
            0.18f,
            0.52f,
            0.84f,
            Color{ 48, 52, 58, 255 }
        );
    }

    DrawCube(
        { X_INICIO_CINTA_67 - 0.44f, 1.10f, z },
        0.70f,
        2.05f,
        1.54f,
        Color{ 58, 63, 71, 255 }
    );

    DrawCube(
        { X_INICIO_CINTA_67 - 0.05f, 1.16f, z },
        0.08f,
        1.28f,
        1.12f,
        Fade(colorEquipo, 0.72f)
    );
}


static void DibujarFlujoPiezas67(
    const EstadoJugador67& estado,
    TipoPieza67 tipo,
    float z
)
{
    Color color =
        tipo == PIEZA_NUMERO_6
        ? ORANGE
        : SKYBLUE;

    for (
        int i = 0;
        i < CANTIDAD_PIEZAS_VISUALES_67;
        i++
    )
    {
        float progreso =
            ObtenerProgresoPiezaVisual67(
                estado,
                tipo,
                i
            );

        DibujarNumero3D67(
            tipo == PIEZA_NUMERO_6 ? 6 : 7,
            {
                ObtenerXObjeto67(progreso),
                0.91f,
                z
            },
            0.56f,
            color
        );
    }
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
        : Color{ 54, 57, 65, 255 };

    DrawCylinder(
        { X_MESA_67, 0.12f, 0.0f },
        1.12f,
        1.12f,
        0.24f,
        32,
        Color{ 27, 29, 34, 255 }
    );

    DrawCylinder(
        { X_MESA_67, 0.28f, 0.0f },
        0.96f,
        0.96f,
        0.11f,
        32,
        colorBase
    );

    DrawCylinder(
        { X_MESA_67, 0.37f, 0.0f },
        0.80f,
        0.80f,
        0.045f,
        32,
        Fade(colorEquipo, 0.58f)
    );

    DibujarNumero3D67(
        6,
        { X_MESA_67 - 0.33f, 0.79f, 0.0f },
        0.45f,
        estado == MESA_67_VACIA
        ? Fade(RAYWHITE, 0.25f)
        : ORANGE
    );

    DibujarNumero3D67(
        7,
        { X_MESA_67 + 0.33f, 0.79f, 0.0f },
        0.45f,
        estado == MESA_67_COMPLETA
        ? SKYBLUE
        : Fade(RAYWHITE, 0.25f)
    );

    if (estado == MESA_67_COMPLETA)
    {
        DrawCylinder(
            { X_MESA_67, 0.45f, 0.0f },
            0.88f,
            0.88f,
            0.035f,
            32,
            Fade(LIME, 0.45f)
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
    // Mirando a su cinta cuando espera y a la mesa cuando lleva pieza.
    float giroCinta =
        estado.tipoPieza == PIEZA_NUMERO_6
        ? 180.0f
        : 0.0f;

    float giroMesa = 90.0f;
    float giro = giroCinta;

    if (estado.llevaPieza)
    {
        float progresoGiro =
            1.0f -
            estado.tiempoGiro / DURACION_GIRO_67;

        if (progresoGiro < 0.0f) progresoGiro = 0.0f;
        if (progresoGiro > 1.0f) progresoGiro = 1.0f;

        giro =
            giroCinta +
            (giroMesa - giroCinta) *
            progresoGiro;
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
        { X_JUGADOR_67, 0.06f, z },
        0.56f,
        0.56f,
        0.12f,
        24,
        Fade(participante.color, 0.56f)
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
            { X_JUGADOR_67, 2.16f, z },
            0.60f,
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
        { 0.0f, -0.30f, 0.0f },
        { 19.0f, 11.0f },
        equipo == 0
        ? Color{ 95, 58, 62, 255 }
        : Color{ 51, 74, 91, 255 }
    );

    DrawCube(
        { 0.0f, 2.45f, -4.85f },
        18.0f,
        5.5f,
        0.25f,
        Color{ 70, 75, 84, 255 }
    );

    DrawCube(
        { 0.0f, 4.65f, -4.68f },
        17.4f,
        0.55f,
        0.10f,
        Fade(colorEquipo, 0.86f)
    );

    // Pasillo central: esta zona deja claro que los jugadores estan
    // entre las dos cintas y no encima de ellas.
    DrawCube(
        { -0.80f, -0.20f, 0.0f },
        13.4f,
        0.08f,
        2.05f,
        Color{ 129, 132, 139, 255 }
    );

    DrawCubeWires(
        { -0.80f, -0.20f, 0.0f },
        13.4f,
        0.08f,
        2.05f,
        Fade(BLACK, 0.42f)
    );

    // Carteles de los dos carriles.
    DrawCube(
        { -4.95f, 3.55f, -4.55f },
        2.1f,
        1.25f,
        0.08f,
        Fade(ORANGE, 0.58f)
    );

    DrawCube(
        { -1.95f, 3.55f, -4.55f },
        2.1f,
        1.25f,
        0.08f,
        Fade(SKYBLUE, 0.58f)
    );

    DibujarNumero3D67(
        6,
        { -4.95f, 3.55f, -4.40f },
        0.75f,
        ORANGE
    );

    DibujarNumero3D67(
        7,
        { -1.95f, 3.55f, -4.40f },
        0.75f,
        SKYBLUE
    );

    for (int i = 0; i < 4; i++)
    {
        DrawCube(
            { -6.0f + i * 3.85f, 2.0f, -4.50f },
            0.26f,
            4.0f,
            0.26f,
            Color{ 51, 56, 64, 255 }
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

    int indiceRol6 =
        BuscarJugadorRol67(
            minijuego,
            equipo,
            PIEZA_NUMERO_6,
            cantidadMaxima
        );

    int indiceRol7 =
        BuscarJugadorRol67(
            minijuego,
            equipo,
            PIEZA_NUMERO_7,
            cantidadMaxima
        );

    int indiceUnico =
        BuscarJugadorUnicoEquipo67(
            minijuego,
            equipo,
            cantidadMaxima
        );

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
            alto / 2 - 150
        );

        DibujarTarjetaRol(
            indiceRol7,
            7,
            alto / 2 + 88
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
        ? Color{ 98, 38, 44, 255 }
        : Color{ 27, 73, 102, 255 }
    );

    BeginMode3D(
        minijuego.camarasEquipo[equipo]
    );

    DibujarFabricaEquipo67(equipo);

    // Las dos cintas siempre existen y los objetos estan desfasados.
    DibujarCinta67(
        Z_CINTA_6_67,
        minijuego.desplazamientoVisualCintas,
        colorEquipo,
        PIEZA_NUMERO_6
    );

    DibujarCinta67(
        Z_CINTA_7_67,
        minijuego.desplazamientoVisualCintas + 0.31f,
        colorEquipo,
        PIEZA_NUMERO_7
    );

    int indice6 =
        BuscarJugadorRol67(
            minijuego,
            equipo,
            PIEZA_NUMERO_6,
            cantidadMaxima
        );

    int indice7 =
        BuscarJugadorRol67(
            minijuego,
            equipo,
            PIEZA_NUMERO_7,
            cantidadMaxima
        );

    int indiceUnico =
        BuscarJugadorUnicoEquipo67(
            minijuego,
            equipo,
            cantidadMaxima
        );

    if (minijuego.cantidadJugadoresEquipo[equipo] == 1)
    {
        if (indiceUnico >= 0)
        {
            const EstadoJugador67& estado =
                minijuego.estadosJugadores[indiceUnico];

            // En 1v1 se muestran ambos flujos; el rol del jugador cambia
            // segun lo que necesita la mesa.
            DibujarFlujoPiezas67(
                estado,
                PIEZA_NUMERO_6,
                Z_CINTA_6_67
            );

            DibujarFlujoPiezas67(
                estado,
                PIEZA_NUMERO_7,
                Z_CINTA_7_67
            );
        }
    }
    else
    {
        if (indice6 >= 0)
        {
            DibujarFlujoPiezas67(
                minijuego.estadosJugadores[indice6],
                PIEZA_NUMERO_6,
                Z_CINTA_6_67
            );
        }

        if (indice7 >= 0)
        {
            DibujarFlujoPiezas67(
                minijuego.estadosJugadores[indice7],
                PIEZA_NUMERO_7,
                Z_CINTA_7_67
            );
        }
    }

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

        float zJugador =
            ObtenerZJugadorEquipo67(
                minijuego,
                i
            );

        DibujarJugador3D67(
            minijuego,
            participantes[i],
            minijuego.estadosJugadores[i],
            zJugador
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
    velocidadCintas = 0.22f;
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
            { 8.4f, 10.6f, 12.8f };

        camarasEquipo[equipo].target =
            { -1.15f, 0.60f, 0.0f };

        camarasEquipo[equipo].up =
            { 0.0f, 1.0f, 0.0f };

        camarasEquipo[equipo].fovy = 12.4f;
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
    velocidadCintas = 0.22f;
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

        // Cada jugador parte en una fase distinta. Ademas el carril 7
        // suma su propio desfase, por lo que 6 y 7 nunca quedan alineados.
        estadosJugadores[i].progresoObjeto =
            NormalizarProgreso67(
                0.037f * (float)i
            );
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
        velocidadCintas * 9.0f * deltaTime;

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
        0.22f + porcentajeTiempo * 0.22f;

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

        // La cinta nunca se detiene cuando alguien toma una pieza.
        // Hay una secuencia continua de varios 6/7 pasando.
        estado.progresoObjeto =
            NormalizarProgreso67(
                estado.progresoObjeto +
                velocidadCintas * deltaTime
            );

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

        bool piezaEnLaMano =
            HayPiezaAlAlcance67(
                estado,
                estado.tipoPieza
            );

        if (piezaEnLaMano)
        {
            estado.llevaPieza = true;
            estado.mirandoMesa = false;
            estado.tiempoGiro = DURACION_GIRO_67;

            // Pequenio corrimiento para evitar volver a capturar el mismo
            // elemento del flujo inmediatamente despues de colocarlo.
            estado.progresoObjeto =
                NormalizarProgreso67(
                    estado.progresoObjeto +
                    0.035f
                );
        }
        else
        {
            int equipo = equipoPorJugador[i];

            estado.tiempoStun = 0.26f;
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
