#include "Minigames/Minijuego67.h"

#include "Minigames/UtilidadesMinijuegos.h"

#include "raymath.h"

#include <cmath>
#include <cstring>


//==================================================
// CONSTANTES
//==================================================

static const float DURACION_PARTIDA_67 = 30.0f;
static const float INICIO_ZONA_RECOGIDA_67 = 0.68f;
static const float FIN_ZONA_RECOGIDA_67 = 0.88f;

static const float X_INICIO_CINTA_67 = -5.4f;
static const float LARGO_CINTA_67 = 7.2f;
static const float X_JUGADOR_67 = 2.35f;
static const float X_MESA_67 = 4.45f;


//==================================================
// UTILIDADES DE ESTADO
//==================================================

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


static float ObtenerZCarril67(int indiceJugador)
{
    static const float Z_CARRILES[4] =
    {
        -3.0f,
        -1.0f,
        1.0f,
        3.0f
    };

    if (indiceJugador < 0)
    {
        indiceJugador = 0;
    }

    if (indiceJugador > 3)
    {
        indiceJugador = 3;
    }

    return Z_CARRILES[indiceJugador];
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


static void ColocarPieza67(
    Minijuego67& minijuego,
    int indiceJugador
)
{
    EstadoJugador67& estadoJugador =
        minijuego.estadosJugadores[indiceJugador];

    int ordenActivo =
        minijuego.ordenActivoPorJugador[indiceJugador];

    if (ordenActivo < 0)
    {
        return;
    }

    int indiceMesa = ordenActivo / 2;

    if (indiceMesa < 0 || indiceMesa >= 2)
    {
        return;
    }

    if (estadoJugador.tipoPieza == PIEZA_NUMERO_6)
    {
        if (minijuego.mesas[indiceMesa] == MESA_67_VACIA)
        {
            minijuego.mesas[indiceMesa] =
                MESA_67_CON_6;
        }
        else
        {
            minijuego.mesas[indiceMesa] =
                MESA_67_VACIA;

            estadoJugador.tiempoStun = 0.55f;
        }
    }
    else
    {
        if (minijuego.mesas[indiceMesa] == MESA_67_CON_6)
        {
            minijuego.mesas[indiceMesa] =
                MESA_67_COMPLETA;

            minijuego.tiempoMesaCompleta[indiceMesa] =
                0.55f;

            minijuego.puntos++;
        }
        else
        {
            estadoJugador.tiempoStun = 0.55f;
        }
    }

    estadoJugador.llevaPieza = false;
    estadoJugador.mirandoMesa = false;
    estadoJugador.tiempoGiro = 0.0f;
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
        Fade(BLACK, 0.65f)
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
        DibujarSegmentoNumero67({ centro.x, centro.y + Y_EXTREMO, centro.z }, true, escala, color);

    if (segmentos[1])
        DibujarSegmentoNumero67({ centro.x + X_LADO, centro.y + Y_MEDIO_LADO, centro.z }, false, escala, color);

    if (segmentos[2])
        DibujarSegmentoNumero67({ centro.x + X_LADO, centro.y - Y_MEDIO_LADO, centro.z }, false, escala, color);

    if (segmentos[3])
        DibujarSegmentoNumero67({ centro.x, centro.y - Y_EXTREMO, centro.z }, true, escala, color);

    if (segmentos[4])
        DibujarSegmentoNumero67({ centro.x - X_LADO, centro.y - Y_MEDIO_LADO, centro.z }, false, escala, color);

    if (segmentos[5])
        DibujarSegmentoNumero67({ centro.x - X_LADO, centro.y + Y_MEDIO_LADO, centro.z }, false, escala, color);

    if (segmentos[6])
        DibujarSegmentoNumero67(centro, true, escala, color);
}


static void DibujarCinta67(
    float z,
    float desplazamiento,
    bool activa
)
{
    Color colorBase = activa
        ? Color{ 55, 61, 70, 255 }
        : Color{ 45, 47, 52, 255 };

    Color colorBanda = activa
        ? Color{ 82, 91, 103, 255 }
        : Color{ 60, 63, 69, 255 };

    float centroX =
        X_INICIO_CINTA_67 +
        LARGO_CINTA_67 / 2.0f;

    DrawCube(
        { centroX, 0.23f, z },
        LARGO_CINTA_67 + 0.35f,
        0.46f,
        1.22f,
        colorBase
    );

    DrawCubeWires(
        { centroX, 0.23f, z },
        LARGO_CINTA_67 + 0.35f,
        0.46f,
        1.22f,
        BLACK
    );

    DrawCube(
        { centroX, 0.48f, z },
        LARGO_CINTA_67,
        0.08f,
        1.02f,
        colorBanda
    );

    for (int i = 0; i < 12; i++)
    {
        float avance =
            std::fmod(
                i * 0.65f + desplazamiento,
                LARGO_CINTA_67
            );

        float x = X_INICIO_CINTA_67 + avance;

        DrawCube(
            { x, 0.535f, z },
            0.075f,
            0.035f,
            0.98f,
            activa
            ? Color{ 175, 185, 195, 255 }
            : Color{ 92, 96, 102, 255 }
        );
    }

    float xZonaInicio =
        ObtenerXObjeto67(INICIO_ZONA_RECOGIDA_67);

    float xZonaFin =
        ObtenerXObjeto67(FIN_ZONA_RECOGIDA_67);

    DrawCube(
        {
            (xZonaInicio + xZonaFin) / 2.0f,
            0.57f,
            z
        },
        xZonaFin - xZonaInicio,
        0.035f,
        1.06f,
        Fade(YELLOW, activa ? 0.38f : 0.12f)
    );

    for (int lado = -1; lado <= 1; lado += 2)
    {
        DrawCube(
            { centroX, 0.66f, z + lado * 0.61f },
            LARGO_CINTA_67 + 0.40f,
            0.18f,
            0.10f,
            Color{ 42, 46, 54, 255 }
        );
    }

    for (int pata = 0; pata < 3; pata++)
    {
        float xPata =
            X_INICIO_CINTA_67 +
            0.7f +
            pata * 2.9f;

        DrawCube(
            { xPata, -0.02f, z },
            0.18f,
            0.55f,
            0.85f,
            Color{ 48, 52, 58, 255 }
        );
    }
}


static void DibujarMesa67(
    int indiceMesa,
    EstadoMesa67 estado,
    float tiempoCompleta
)
{
    float z = indiceMesa == 0 ? -2.0f : 2.0f;

    Color colorMesa = tiempoCompleta > 0.0f
        ? Color{ 70, 180, 95, 255 }
        : Color{ 126, 77, 43, 255 };

    DrawCube(
        { X_MESA_67, 0.82f, z },
        1.85f,
        0.22f,
        2.55f,
        colorMesa
    );

    DrawCubeWires(
        { X_MESA_67, 0.82f, z },
        1.85f,
        0.22f,
        2.55f,
        BLACK
    );

    for (int ladoX = -1; ladoX <= 1; ladoX += 2)
    {
        for (int ladoZ = -1; ladoZ <= 1; ladoZ += 2)
        {
            DrawCube(
                {
                    X_MESA_67 + ladoX * 0.67f,
                    0.38f,
                    z + ladoZ * 0.92f
                },
                0.18f,
                0.78f,
                0.18f,
                Color{ 78, 48, 29, 255 }
            );
        }
    }

    if (
        estado == MESA_67_CON_6 ||
        estado == MESA_67_COMPLETA
    )
    {
        DibujarNumero3D67(
            6,
            { X_MESA_67 - 0.36f, 1.40f, z },
            0.72f,
            ORANGE
        );
    }

    if (estado == MESA_67_COMPLETA)
    {
        DibujarNumero3D67(
            7,
            { X_MESA_67 + 0.36f, 1.40f, z },
            0.72f,
            SKYBLUE
        );
    }
}


static void DibujarJugador3D67(
    const Minijuego67& minijuego,
    const Participante& participante,
    const EstadoJugador67& estado,
    int indiceJugador
)
{
    float z = ObtenerZCarril67(indiceJugador);
    float giro = -90.0f;

    if (estado.llevaPieza)
    {
        float progresoGiro =
            1.0f - estado.tiempoGiro / 0.32f;

        if (progresoGiro < 0.0f)
            progresoGiro = 0.0f;

        if (progresoGiro > 1.0f)
            progresoGiro = 1.0f;

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
        0.62f,
        0.62f,
        0.16f,
        24,
        Fade(participante.color, 0.72f)
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
            estado.tipoPieza == PIEZA_NUMERO_6 ? 6 : 7,
            { X_JUGADOR_67, 2.18f, z },
            0.62f,
            colorPieza
        );
    }
}


static void DibujarFabrica67()
{
    DrawPlane(
        { 0.0f, -0.31f, 0.0f },
        { 17.0f, 11.0f },
        Color{ 104, 106, 109, 255 }
    );

    DrawCube(
        { 0.0f, 2.30f, -5.15f },
        16.0f,
        5.2f,
        0.25f,
        Color{ 80, 87, 96, 255 }
    );

    DrawCube(
        { -6.9f, 2.30f, 0.0f },
        0.35f,
        5.2f,
        10.2f,
        Color{ 67, 73, 82, 255 }
    );

    for (int i = 0; i < 5; i++)
    {
        DrawCube(
            { -5.7f + i * 2.8f, 3.85f, -4.96f },
            1.35f,
            1.25f,
            0.08f,
            Color{ 44, 77, 93, 255 }
        );

        DrawCubeWires(
            { -5.7f + i * 2.8f, 3.85f, -4.96f },
            1.35f,
            1.25f,
            0.08f,
            BLACK
        );
    }

    for (int i = 0; i < 4; i++)
    {
        DrawCube(
            { -5.8f + i * 3.8f, 2.1f, -4.82f },
            0.32f,
            4.2f,
            0.32f,
            Color{ 52, 57, 64, 255 }
        );
    }
}


//==================================================
// INICIALIZAR / REINICIAR
//==================================================

void Minijuego67::Inicializar()
{
    tiempoPartida = DURACION_PARTIDA_67;
    velocidadCintas = 0.24f;
    desplazamientoVisualCintas = 0.0f;
    puntos = 0;
    jugadoresEnPartida = 0;
    terminado = false;

    camara.position = { 10.8f, 8.4f, 11.7f };
    camara.target = { 0.0f, 0.85f, 0.0f };
    camara.up = { 0.0f, 1.0f, 0.0f };
    camara.fovy = 50.0f;
    camara.projection = CAMERA_PERSPECTIVE;

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

    // Este GLB usa Z como eje vertical. El giro negativo
    // lo deja de pie y evita que aparezca bajo la plataforma.
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


void Minijuego67::Reiniciar(
    const Participante participantes[],
    int cantidadMaxima
)
{
    tiempoPartida = DURACION_PARTIDA_67;
    velocidadCintas = 0.24f;
    desplazamientoVisualCintas = 0.0f;
    puntos = 0;
    terminado = false;
    fotogramaAnimacionIdle = 0.0f;

    jugadoresEnPartida =
        ContarParticipantesDisponibles67(
            participantes,
            cantidadMaxima
        );

    int indicesActivos[MAX_PARTICIPANTES]{};
    int cantidadActivos =
        ObtenerIndicesParticipantesActivos(
            participantes,
            indicesActivos,
            MAX_PARTICIPANTES
        );

    for (int i = 0; i < cantidadMaxima; i++)
    {
        ordenActivoPorJugador[i] = -1;
    }

    for (int orden = 0; orden < cantidadActivos; orden++)
    {
        ordenActivoPorJugador[indicesActivos[orden]] =
            orden;
    }

    for (int i = 0; i < 2; i++)
    {
        mesas[i] = MESA_67_VACIA;
        tiempoMesaCompleta[i] = 0.0f;
    }

    for (int i = 0; i < cantidadMaxima; i++)
    {
        EstadoJugador67& estado =
            estadosJugadores[i];

        int ordenActivo = ordenActivoPorJugador[i];

        estado.tipoPieza =
            ordenActivo >= 0 && ordenActivo % 2 == 0
            ? PIEZA_NUMERO_6
            : PIEZA_NUMERO_7;

        estado.progresoObjeto =
            -0.06f + 0.08f * (float)i;

        estado.objetoActivo = true;
        estado.tiempoReaparicion = 0.0f;
        estado.llevaPieza = false;
        estado.mirandoMesa = false;
        estado.tiempoGiro = 0.0f;
        estado.tiempoStun = 0.0f;
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

        int fotogramaActual =
            (int)fotogramaAnimacionIdle %
            cantidadFotogramas;

        UpdateModelAnimation(
            modeloJugador,
            animacionesJugador[indiceAnimacionIdle],
            fotogramaActual
        );
    }

    int cantidadActivos =
        ContarParticipantesDisponibles67(
            participantes,
            cantidadMaxima
        );

    if (cantidadActivos != jugadoresEnPartida)
    {
        Reiniciar(participantes, cantidadMaxima);
        return;
    }

    if (terminado || cantidadActivos < 2)
    {
        return;
    }

    tiempoPartida -= deltaTime;

    if (tiempoPartida <= 0.0f)
    {
        tiempoPartida = 0.0f;
        terminado = true;
        return;
    }

    float porcentajeTiempo =
        1.0f -
        tiempoPartida / DURACION_PARTIDA_67;

    velocidadCintas =
        0.24f + porcentajeTiempo * 0.18f;

    for (int i = 0; i < 2; i++)
    {
        if (tiempoMesaCompleta[i] > 0.0f)
        {
            tiempoMesaCompleta[i] -= deltaTime;

            if (tiempoMesaCompleta[i] <= 0.0f)
            {
                tiempoMesaCompleta[i] = 0.0f;
                mesas[i] = MESA_67_VACIA;
            }
        }
    }

    for (int i = 0; i < cantidadMaxima; i++)
    {
        if (
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

            continue;
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
                    0.28f
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
                ColocarPieza67(*this, i);
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
            estado.tiempoGiro = 0.32f;

            ReiniciarObjetoCinta67(
                estado,
                0.58f
            );
        }
        else
        {
            estado.tiempoStun = 0.48f;
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
    ClearBackground(Color{ 112, 153, 177, 255 });

    BeginMode3D(camara);

    DibujarFabrica67();

    for (int i = 0; i < cantidadMaxima; i++)
    {
        bool jugadorActivo =
            participantes[i].activo &&
            participantes[i].conectado;
        float z = ObtenerZCarril67(i);

        DibujarCinta67(
            z,
            desplazamientoVisualCintas,
            jugadorActivo
        );

        if (!jugadorActivo)
        {
            continue;
        }

        const EstadoJugador67& estado =
            estadosJugadores[i];

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
                    0.93f,
                    z
                },
                0.62f,
                colorPieza
            );
        }

        DibujarJugador3D67(
            *this,
            participantes[i],
            estado,
            i
        );
    }

    for (int pareja = 0; pareja < 2; pareja++)
    {
        int indicesActivos[MAX_PARTICIPANTES]{};
        int cantidadActivos =
            ObtenerIndicesParticipantesActivos(
                participantes,
                indicesActivos,
                MAX_PARTICIPANTES
            );

        if (pareja * 2 < cantidadActivos)
        {
            DibujarMesa67(
                pareja,
                mesas[pareja],
                tiempoMesaCompleta[pareja]
            );
        }
    }

    EndMode3D();

    DrawRectangle(
        18,
        15,
        GetScreenWidth() - 36,
        70,
        Fade(BLACK, 0.76f)
    );

    DrawText(
        "FABRICA 67 - COOPERATIVO 3D",
        32,
        26,
        27,
        RAYWHITE
    );

    DrawText(
        TextFormat("PUNTOS: %d", puntos),
        GetScreenWidth() / 2 - 70,
        26,
        26,
        LIME
    );

    DrawText(
        TextFormat("TIEMPO: %.1f", tiempoPartida),
        GetScreenWidth() - 205,
        26,
        24,
        tiempoPartida <= 7.0f ? RED : SKYBLUE
    );

    DrawText(
        "AGARRA EN AMARILLO Y PRESIONA OTRA VEZ AL MIRAR LA MESA",
        32,
        58,
        16,
        LIGHTGRAY
    );

    int cantidadActivos =
        ContarParticipantesDisponibles67(
            participantes,
            cantidadMaxima
        );

    if (cantidadActivos < 2)
    {
        const char* texto =
            "SE NECESITAN AL MENOS 2 JUGADORES";

        DrawRectangle(
            GetScreenWidth() / 2 - 295,
            105,
            590,
            58,
            Fade(BLACK, 0.82f)
        );

        DrawText(
            texto,
            GetScreenWidth() / 2 -
                MeasureText(texto, 27) / 2,
            120,
            27,
            ORANGE
        );
    }

    int anchoTarjeta =
        (GetScreenWidth() - 55) / 4;

    int yTarjetas =
        GetScreenHeight() - 214;

    for (int i = 0; i < cantidadMaxima; i++)
    {
        if (
            !participantes[i].activo ||
            !participantes[i].conectado
        )
        {
            continue;
        }

        const EstadoJugador67& estado =
            estadosJugadores[i];

        int x = 18 + i * (anchoTarjeta + 6);

        DrawRectangle(
            x,
            yTarjetas,
            anchoTarjeta,
            55,
            Fade(BLACK, 0.76f)
        );

        DrawRectangleLinesEx(
            Rectangle{
                (float)x,
                (float)yTarjetas,
                (float)anchoTarjeta,
                55.0f
            },
            2.0f,
            participantes[i].color
        );

        const char* estadoTexto =
            estado.tiempoStun > 0.0f
            ? "FALLO - STUN"
            : (
                estado.llevaPieza
                ? (
                    estado.mirandoMesa
                    ? "DEJAR EN MESA"
                    : "GIRANDO"
                )
                : "AGARRAR PIEZA"
            );

        DrawText(
            TextFormat(
                "J%d  NUMERO %d  [%s]",
                participantes[i].numeroJugador,
                estado.tipoPieza == PIEZA_NUMERO_6
                ? 6
                : 7,
                ObtenerTextoBotonPrincipal(
                    participantes[i]
                )
            ),
            x + 8,
            yTarjetas + 7,
            15,
            RAYWHITE
        );

        DrawText(
            estadoTexto,
            x + 8,
            yTarjetas + 31,
            15,
            estado.tiempoStun > 0.0f
            ? RED
            : SKYBLUE
        );
    }

    if (terminado)
    {
        DrawRectangle(
            GetScreenWidth() / 2 - 315,
            GetScreenHeight() / 2 - 70,
            630,
            140,
            Fade(BLACK, 0.90f)
        );

        const char* texto =
            TextFormat(
                "TIEMPO - PUNTAJE FINAL: %d",
                puntos
            );

        DrawText(
            texto,
            GetScreenWidth() / 2 -
                MeasureText(texto, 32) / 2,
            GetScreenHeight() / 2 - 18,
            32,
            RAYWHITE
        );
    }
}


//==================================================
// DESCARGAR
//==================================================

void Minijuego67::Descargar()
{
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
