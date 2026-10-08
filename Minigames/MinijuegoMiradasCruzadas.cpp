#include "Minigames/MinijuegoMiradasCruzadas.h"

#include "Minigames/AudioMinijuegos.h"
#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include "Systems/Input.h"

#include <cmath>


static const float DURACION_PREPARACION_MIRADAS = 2.5f;
static const float DURACION_ELECCION_MIRADAS = 1.65f;
static const float DURACION_RESOLUCION_MIRADAS = 1.15f;
static const int MAX_RONDAS_MIRADAS = 5;


static const char* NombreDireccionMirada(
    DireccionMiradaCruzada direccion
)
{
    switch (direccion)
    {
        case MIRADA_FRENTE: return "FRENTE";
        case MIRADA_IZQUIERDA: return "IZQUIERDA";
        case MIRADA_DERECHA: return "DERECHA";
        case MIRADA_ARRIBA: return "ARRIBA";
        case MIRADA_ABAJO: return "ABAJO";
        case CANTIDAD_DIRECCIONES_MIRADA: break;
    }

    return "?";
}


static int ContarEquipoVivoMiradas(
    const MinijuegoMiradasCruzadas& minijuego
)
{
    int vivos = 0;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (
            i != minijuego.indiceSolo &&
            minijuego.resultado.participantes[i].participo &&
            !minijuego.jugadores[i].eliminado
        )
        {
            vivos++;
        }
    }

    return vivos;
}


static bool LeerDireccionMiradaHumana(
    const Participante& participante,
    DireccionMiradaCruzada& direccion
)
{
    InputSeleccionParticipante entrada =
        LeerInputSeleccionParticipante(participante);

    if (entrada.izquierda)
    {
        direccion = MIRADA_IZQUIERDA;
        return true;
    }

    if (entrada.derecha)
    {
        direccion = MIRADA_DERECHA;
        return true;
    }

    if (entrada.arriba)
    {
        direccion = MIRADA_ARRIBA;
        return true;
    }

    if (entrada.abajo)
    {
        direccion = MIRADA_ABAJO;
        return true;
    }

    return false;
}


static void PrepararRondaMiradas(
    MinijuegoMiradasCruzadas& minijuego,
    const Participante participantes[]
)
{
    minijuego.numeroRonda++;
    minijuego.fase = FASE_MIRADAS_ELECCION;
    minijuego.tiempoFase = DURACION_ELECCION_MIRADAS;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!minijuego.resultado.participantes[i].participo)
        {
            continue;
        }

        minijuego.jugadores[i].direccion = MIRADA_FRENTE;
        minijuego.jugadores[i].eligioDireccion = false;

        if (
            (participantes[i].esBot || !participantes[i].conectado) &&
            !minijuego.jugadores[i].eliminado
        )
        {
            ReiniciarBotMiradas(
                minijuego.bots[i],
                0.32f,
                1.05f
            );
        }
    }
}


static void FinalizarMiradas(
    MinijuegoMiradasCruzadas& minijuego,
    bool ganaSolo
)
{
    minijuego.resultado.estado = RESULTADO_MINIJUEGO_FINALIZADO;
    minijuego.resultado.desenlace = DESENLACE_CON_GANADOR;
    minijuego.resultado.cantidadEquipos = 2;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        ResultadoParticipante& resultadoJugador =
            minijuego.resultado.participantes[i];

        if (!resultadoJugador.participo)
        {
            continue;
        }

        bool esSolo = i == minijuego.indiceSolo;
        bool ganador = esSolo == ganaSolo;

        resultadoJugador.numeroEquipo = esSolo ? 0 : 1;
        resultadoJugador.posicionFinal = ganador ? 1 : 2;
        resultadoJugador.puntuacionMinijuego =
            esSolo
            ? 3 - ContarEquipoVivoMiradas(minijuego)
            : (!minijuego.jugadores[i].eliminado ? 1 : 0);
        resultadoJugador.puntosObtenidos = 0;
    }

    minijuego.fase = FASE_MIRADAS_TERMINADO;

    ReproducirSonidoMinijuego(minijuego.audio, SONIDO_RESULTADO);
}


static void ResolverRondaMiradas(
    MinijuegoMiradasCruzadas& minijuego
)
{
    DireccionMiradaCruzada direccionSolo =
        minijuego.jugadores[minijuego.indiceSolo].direccion;

    int vivosAntes = ContarEquipoVivoMiradas(minijuego);
    int eliminados = 0;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (
            i == minijuego.indiceSolo ||
            !minijuego.resultado.participantes[i].participo ||
            minijuego.jugadores[i].eliminado
        )
        {
            continue;
        }

        if (minijuego.jugadores[i].direccion == direccionSolo)
        {
            minijuego.jugadores[i].eliminado = true;
            eliminados++;
        }
    }

    ReproducirSonidoMinijuego(
        minijuego.audio,
        eliminados > 0 ? SONIDO_ELIMINADO : SONIDO_ACIERTO
    );

    int posicion = vivosAntes - eliminados + 1;
    if (posicion < 2) posicion = 2;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (
            i != minijuego.indiceSolo &&
            minijuego.jugadores[i].eliminado &&
            minijuego.jugadores[i].posicionFinal == 0
        )
        {
            minijuego.jugadores[i].posicionFinal = posicion;
        }
    }

    if (ContarEquipoVivoMiradas(minijuego) <= 0)
    {
        FinalizarMiradas(minijuego, true);
        return;
    }

    if (minijuego.numeroRonda >= MAX_RONDAS_MIRADAS)
    {
        FinalizarMiradas(minijuego, false);
        return;
    }

    minijuego.fase = FASE_MIRADAS_RESOLUCION;
    minijuego.tiempoFase = DURACION_RESOLUCION_MIRADAS;
}


void MinijuegoMiradasCruzadas::Inicializar()
{
    resultado = {};
    resultado.formato = FORMATO_MINIJUEGO_EQUIPOS;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        jugadores[i] = {};
        bots[i] = {};
    }

    fase = FASE_MIRADAS_PREPARACION;
    indiceSolo = -1;
    numeroRonda = 0;
    tiempoPreparacion = DURACION_PREPARACION_MIRADAS;
    tiempoFase = 0.0f;
}


void MinijuegoMiradasCruzadas::Reiniciar(
    Participante participantes[]
)
{
    Inicializar();

    InicializarResultadoMinijuego(
        resultado,
        participantes,
        FORMATO_MINIJUEGO_EQUIPOS
    );

    int indices[MAX_PARTICIPANTES]{};
    int cantidad =
        ObtenerIndicesParticipantesActivos(
            participantes,
            indices,
            MAX_PARTICIPANTES
        );

    if (cantidad < 2)
    {
        resultado.estado = RESULTADO_MINIJUEGO_CANCELADO;
        fase = FASE_MIRADAS_TERMINADO;
        return;
    }

    indiceSolo = indices[GetRandomValue(0, cantidad - 1)];
    resultado.cantidadEquipos = 2;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!resultado.participantes[i].participo)
        {
            continue;
        }

        resultado.participantes[i].numeroEquipo =
            i == indiceSolo ? 0 : 1;
    }
}


void MinijuegoMiradasCruzadas::Actualizar(
    float deltaTime,
    Participante participantes[]
)
{
    tiempoAnimacion += deltaTime;

    if (
        fase == FASE_MIRADAS_TERMINADO ||
        resultado.estado == RESULTADO_MINIJUEGO_CANCELADO
    )
    {
        return;
    }

    if (fase == FASE_MIRADAS_PREPARACION)
    {
        float preparacionAntes = tiempoPreparacion;
        tiempoPreparacion -= deltaTime;
        ActualizarAudioCuentaRegresiva(audio, preparacionAntes, tiempoPreparacion);

        if (tiempoPreparacion <= 0.0f)
        {
            tiempoPreparacion = 0.0f;
            PrepararRondaMiradas(*this, participantes);
        }

        return;
    }

    if (fase == FASE_MIRADAS_ELECCION)
    {
        for (int i = 0; i < MAX_PARTICIPANTES; i++)
        {
            if (
                !resultado.participantes[i].participo ||
                (i != indiceSolo && jugadores[i].eliminado)
            )
            {
                continue;
            }

            if (participantes[i].esBot || !participantes[i].conectado)
            {
                if (
                    ActualizarDecisionBotMiradas(
                        bots[i],
                        deltaTime,
                        CANTIDAD_DIRECCIONES_MIRADA
                    )
                )
                {
                    jugadores[i].direccion =
                        (DireccionMiradaCruzada)bots[i].direccionElegida;
                    jugadores[i].eligioDireccion = true;
                }

                continue;
            }

            if (!participantes[i].conectado)
            {
                continue;
            }

            DireccionMiradaCruzada nueva = jugadores[i].direccion;

            if (LeerDireccionMiradaHumana(participantes[i], nueva))
            {
                jugadores[i].direccion = nueva;
                jugadores[i].eligioDireccion = true;
                ReproducirSonidoMinijuego(audio, SONIDO_BOTON);
            }
        }

        tiempoFase -= deltaTime;

        if (tiempoFase <= 0.0f)
        {
            ResolverRondaMiradas(*this);
        }

        return;
    }

    if (fase == FASE_MIRADAS_RESOLUCION)
    {
        tiempoFase -= deltaTime;

        if (tiempoFase <= 0.0f)
        {
            PrepararRondaMiradas(*this, participantes);
        }
    }
}


//==================================================
// VISUAL 3D (independiente de la logica)
//==================================================
//
// Escenario: plaza con arcoiris. El jugador solitario esta en un podio
// alto al fondo y el equipo en pedestales al frente. Todos son cabezas
// grandes que giran hacia la direccion elegida (izquierda / derecha son
// las de la pantalla) y solo se revelan al resolver la ronda.
//
// MODELO FUTURO: reemplazar por GLB el podio y los pedestales, las
// cabezas / personajes con animacion de giro, el arcoiris, las nubes y
// los arboles de la plaza.
//==================================================


static const float Z_SOLO_MIRADAS = -4.5f;
static const float Z_EQUIPO_MIRADAS = 2.0f;
static const float ALTURA_PODIO_SOLO_MIRADAS = 3.4f;
static const float ALTURA_PEDESTAL_EQUIPO_MIRADAS = 1.0f;
static const float RADIO_CABEZA_SOLO_MIRADAS = 1.3f;
static const float RADIO_CABEZA_EQUIPO_MIRADAS = 1.1f;


static Camera3D ObtenerCamaraMiradas()
{
    Camera3D camara{};
    camara.position = { 0.0f, 6.5f, 16.0f };
    camara.target = { 0.0f, 3.6f, 0.0f };
    camara.up = { 0.0f, 1.0f, 0.0f };
    camara.fovy = 45.0f;
    camara.projection = CAMERA_PERSPECTIVE;
    return camara;
}


// Giro de la cabeza (grados) para cada direccion de la pantalla.
static void AngulosMiradaCruzada(
    DireccionMiradaCruzada direccion,
    float& giro,
    float& cabeceo
)
{
    giro = 0.0f;
    cabeceo = 0.0f;

    switch (direccion)
    {
        case MIRADA_IZQUIERDA: giro = -55.0f; break;
        case MIRADA_DERECHA: giro = 55.0f; break;
        case MIRADA_ARRIBA: cabeceo = -35.0f; break;
        case MIRADA_ABAJO: cabeceo = 35.0f; break;
        case MIRADA_FRENTE:
        case CANTIDAD_DIRECCIONES_MIRADA:
            break;
    }
}


static void DibujarEscenarioMiradas(float tiempo)
{
    // Cielo (color de fondo), pasto y plaza circular.
    DrawCube({ 0.0f, -0.3f, 0.0f }, 80.0f, 0.4f, 60.0f, Color{ 116, 187, 111, 255 });
    DrawCylinder({ 0.0f, -0.12f, -1.0f }, 11.0f, 11.0f, 0.12f, 40, Color{ 214, 196, 160, 255 });
    DrawCircle3D({ 0.0f, 0.02f, -1.0f }, 10.0f, { 1.0f, 0.0f, 0.0f }, 90.0f, Fade(Color{ 150, 120, 90, 255 }, 0.6f));
    DrawCircle3D({ 0.0f, 0.02f, -1.0f }, 6.0f, { 1.0f, 0.0f, 0.0f }, 90.0f, Fade(Color{ 150, 120, 90, 255 }, 0.4f));

    // Arcoiris: 7 bandas hechas con tramos de cilindro.
    Color colores[7] = { RED, ORANGE, YELLOW, GREEN, SKYBLUE, BLUE, PURPLE };

    for (int banda = 0; banda < 7; banda++)
    {
        float radio = 13.0f - banda * 0.38f;

        for (int tramo = 0; tramo < 20; tramo++)
        {
            float a0 = PI * tramo / 20.0f;
            float a1 = PI * (tramo + 1) / 20.0f;
            DrawCylinderEx(
                { std::cos(a0) * radio, std::sin(a0) * radio - 1.0f, -14.0f },
                { std::cos(a1) * radio, std::sin(a1) * radio - 1.0f, -14.0f },
                0.2f,
                0.2f,
                4,
                Fade(colores[banda], 0.85f)
            );
        }
    }

    // Nubes (grupos de esferas) que se mecen despacio.
    for (int n = 0; n < 6; n++)
    {
        float x = -17.0f + n * 7.0f + std::sin(tiempo * 0.15f + n) * 1.2f;
        float y = 9.0f + (n % 3) * 1.6f;
        DrawSphereEx({ x, y, -15.0f }, 1.3f, 8, 8, Fade(WHITE, 0.9f));
        DrawSphereEx({ x + 1.4f, y - 0.2f, -15.0f }, 1.0f, 8, 8, Fade(WHITE, 0.9f));
        DrawSphereEx({ x - 1.3f, y - 0.3f, -15.0f }, 0.9f, 8, 8, Fade(WHITE, 0.9f));
    }

    // Arboles de caramelo a los lados.
    for (int lado = -1; lado <= 1; lado += 2)
    {
        for (int k = 0; k < 3; k++)
        {
            float x = lado * (9.0f + k * 2.6f);
            float z = -6.0f - k * 2.0f;
            DrawCylinder({ x, 0.0f, z }, 0.18f, 0.25f, 2.0f, 6, Color{ 130, 90, 60, 255 });
            DrawSphereEx({ x, 2.6f, z }, 1.1f, 8, 8, k % 2 == 0 ? Color{ 255, 140, 190, 255 } : Color{ 120, 215, 140, 255 });
        }
    }
}


static void DibujarCabezaMiradas3D(
    Vector3 basePedestal,
    float alturaPedestal,
    float radioPedestal,
    float radioCabeza,
    Color color,
    DireccionMiradaCruzada direccion,
    float factorGiro,
    bool eliminado,
    bool decisionBloqueada,
    float tiempo
)
{
    // Pedestal con una luz que confirma que ya eligio (sin revelar a donde).
    DrawCylinder(basePedestal, radioPedestal, radioPedestal + 0.2f, alturaPedestal, 20, Color{ 90, 84, 104, 255 });
    DrawCylinder({ basePedestal.x, alturaPedestal, basePedestal.z }, radioPedestal, radioPedestal, 0.12f, 20, color);
    DrawSphereEx(
        { basePedestal.x, alturaPedestal * 0.5f, basePedestal.z + radioPedestal + 0.15f },
        0.14f,
        6,
        6,
        decisionBloqueada ? LIME : Fade(GRAY, 0.6f)
    );

    float escala = eliminado ? 0.58f : 1.0f;
    float radio = radioCabeza * escala;
    Vector3 centro =
    {
        basePedestal.x,
        alturaPedestal + 0.12f + radio * 1.05f,
        basePedestal.z
    };

    float giro = 0.0f;
    float cabeceo = 0.0f;
    AngulosMiradaCruzada(direccion, giro, cabeceo);
    giro *= factorGiro;
    cabeceo *= factorGiro;

    // Balanceo suave de reposo antes de revelar.
    giro += std::sin(tiempo * 1.7f + centro.x) * 3.0f * (1.0f - factorGiro);

    Color piel = eliminado ? Fade(color, 0.45f) : color;

    DrawCylinder(
        { centro.x, alturaPedestal + 0.1f, centro.z },
        radio * 0.35f,
        radio * 0.45f,
        radio * 0.3f,
        10,
        ColorLerp(color, BLACK, 0.4f)
    );

    rlPushMatrix();
    rlTranslatef(centro.x, centro.y, centro.z);
    rlRotatef(giro, 0.0f, 1.0f, 0.0f);
    rlRotatef(cabeceo, 1.0f, 0.0f, 0.0f);

    DrawSphereEx({ 0.0f, 0.0f, 0.0f }, radio, 16, 16, piel);
    DrawSphereEx({ -radio * 0.98f, 0.0f, 0.0f }, radio * 0.18f, 6, 6, piel);
    DrawSphereEx({ radio * 0.98f, 0.0f, 0.0f }, radio * 0.18f, 6, 6, piel);

    // Ojos grandes con pupilas, nariz y ceja.
    for (int lado = -1; lado <= 1; lado += 2)
    {
        DrawSphereEx({ lado * radio * 0.34f, radio * 0.16f, radio * 0.84f }, radio * 0.2f, 8, 8, RAYWHITE);
        DrawSphereEx({ lado * radio * 0.34f, radio * 0.16f, radio * 0.99f }, radio * 0.1f, 6, 6, BLACK);
    }

    DrawSphereEx({ 0.0f, -radio * 0.08f, radio * 1.0f }, radio * 0.13f, 6, 6, ColorLerp(color, BLACK, 0.45f));
    DrawCube({ 0.0f, -radio * 0.4f, radio * 0.92f }, radio * 0.4f, radio * 0.06f, radio * 0.06f, ColorLerp(color, BLACK, 0.6f));

    rlPopMatrix();

    // Rayo de mirada: solo cuando ya se reviso la direccion.
    if (factorGiro > 0.5f && direccion != MIRADA_FRENTE && !eliminado)
    {
        float th = giro * DEG2RAD;
        float ph = cabeceo * DEG2RAD;
        Vector3 mirada =
        {
            std::cos(ph) * std::sin(th),
            -std::sin(ph),
            std::cos(ph) * std::cos(th)
        };
        Vector3 inicio = { centro.x + mirada.x * radio, centro.y + mirada.y * radio, centro.z + mirada.z * radio };
        Vector3 fin = { centro.x + mirada.x * (radio + 2.2f), centro.y + mirada.y * (radio + 2.2f), centro.z + mirada.z * (radio + 2.2f) };
        DrawCylinderEx(inicio, fin, 0.05f, 0.05f, 4, Fade(color, 0.7f));
        DrawSphereEx(fin, 0.14f, 6, 6, color);
    }
}


void MinijuegoMiradasCruzadas::Dibujar(
    const Participante participantes[]
) const
{
    ClearBackground(Color{ 102, 154, 219, 255 });

    int ancho = GetScreenWidth();
    int alto = GetScreenHeight();


    if (indiceSolo < 0)
    {
        return;
    }

    bool revelar =
        fase == FASE_MIRADAS_RESOLUCION ||
        fase == FASE_MIRADAS_TERMINADO;

    // Giro progresivo al revelar (0 a 1 en un cuarto de segundo).
    float factorGiro = 0.0f;

    if (fase == FASE_MIRADAS_TERMINADO)
    {
        factorGiro = 1.0f;
    }
    else if (fase == FASE_MIRADAS_RESOLUCION)
    {
        factorGiro = Clamp((DURACION_RESOLUCION_MIRADAS - tiempoFase) / 0.25f, 0.0f, 1.0f);
    }

    Camera3D camara = ObtenerCamaraMiradas();
    Vector2 etiquetas[MAX_PARTICIPANTES]{};
    Vector2 marcasEliminado[MAX_PARTICIPANTES]{};

    BeginMode3D(camara);

    DibujarEscenarioMiradas(tiempoAnimacion);

    // Jugador solitario en el podio.
    {
        Vector3 base = { 0.0f, 0.0f, Z_SOLO_MIRADAS };
        DibujarCabezaMiradas3D(
            base,
            ALTURA_PODIO_SOLO_MIRADAS,
            1.9f,
            RADIO_CABEZA_SOLO_MIRADAS,
            participantes[indiceSolo].color,
            jugadores[indiceSolo].direccion,
            revelar ? factorGiro : 0.0f,
            false,
            fase == FASE_MIRADAS_ELECCION && jugadores[indiceSolo].eligioDireccion,
            tiempoAnimacion
        );

        etiquetas[indiceSolo] = GetWorldToScreen({ 0.0f, ALTURA_PODIO_SOLO_MIRADAS + 3.2f, Z_SOLO_MIRADAS }, camara);
    }

    // Equipo en pedestales frente al solitario.
    static const float posicionesX[3] = { -4.6f, 0.0f, 4.6f };
    int cursor = 0;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (
            i == indiceSolo ||
            !resultado.participantes[i].participo ||
            cursor >= 3
        )
        {
            continue;
        }

        Vector3 base = { posicionesX[cursor], 0.0f, Z_EQUIPO_MIRADAS };

        DibujarCabezaMiradas3D(
            base,
            ALTURA_PEDESTAL_EQUIPO_MIRADAS,
            1.3f,
            RADIO_CABEZA_EQUIPO_MIRADAS,
            participantes[i].color,
            jugadores[i].direccion,
            revelar ? factorGiro : 0.0f,
            jugadores[i].eliminado,
            fase == FASE_MIRADAS_ELECCION && jugadores[i].eligioDireccion,
            tiempoAnimacion
        );

        if (jugadores[i].eliminado && fase == FASE_MIRADAS_RESOLUCION)
        {
            float p = 1.0f - tiempoFase / DURACION_RESOLUCION_MIRADAS;
            DrawCircle3D(
                { base.x, ALTURA_PEDESTAL_EQUIPO_MIRADAS + 0.2f, base.z },
                1.4f + p * 2.0f,
                { 1.0f, 0.0f, 0.0f },
                90.0f,
                Fade(RED, 1.0f - p)
            );
        }

        etiquetas[i] = GetWorldToScreen({ base.x, 0.0f, base.z + 1.9f }, camara);
        marcasEliminado[i] = GetWorldToScreen({ base.x, ALTURA_PEDESTAL_EQUIPO_MIRADAS + 3.0f, base.z }, camara);
        cursor++;
    }

    EndMode3D();

    // Encabezado despues de la escena 3D: antes quedaba tapado por ella.
    DrawRectangle(18, 14, 690, 98, Fade(BLACK, 0.72f));
    DrawText("MIRADAS CRUZADAS - 1 VS 3", 24, 20, 30, RAYWHITE);
    DrawText(
        "El equipo debe mirar a un lugar distinto del jugador solitario.",
        24,
        58,
        18,
        RAYWHITE
    );
    DrawText(
        "Direcciones: izquierda / derecha / arriba / abajo. Sin tocar nada = frente.",
        24,
        84,
        17,
        LIGHTGRAY
    );

    // Etiquetas de cada jugador.
    cursor = 0;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!resultado.participantes[i].participo)
        {
            continue;
        }

        const char* texto = i == indiceSolo
            ? TextFormat("J%d SOLO%s", participantes[i].numeroJugador, participantes[i].esBot ? " BOT" : "")
            : TextFormat("J%d%s", participantes[i].numeroJugador, participantes[i].esBot ? " BOT" : "");

        DrawText(
            texto,
            (int)etiquetas[i].x - MeasureText(texto, 20) / 2,
            (int)etiquetas[i].y,
            20,
            RAYWHITE
        );

        if (i != indiceSolo && jugadores[i].eliminado)
        {
            DrawText(
                "X",
                (int)marcasEliminado[i].x - MeasureText("X", 34) / 2,
                (int)marcasEliminado[i].y - 34,
                34,
                RED
            );
        }
    }

    int baseTexto = alto - 118;

    if (fase == FASE_MIRADAS_PREPARACION)
    {
        int numero = (int)std::ceil(tiempoPreparacion);
        if (numero < 1) numero = 1;

        const char* texto = TextFormat("%d", numero);
        DrawText(
            texto,
            ancho / 2 - MeasureText(texto, 76) / 2,
            baseTexto - 20,
            76,
            GOLD
        );
    }
    else if (fase == FASE_MIRADAS_ELECCION)
    {
        // Arriba a la derecha (cielo libre): abajo pisaba la etiqueta de J3.
        const char* ronda = TextFormat("RONDA %d / %d - ELIGE", numeroRonda, MAX_RONDAS_MIRADAS);
        int xPanel = ancho - 384;

        DrawRectangle(xPanel, 14, 366, 76, Fade(BLACK, 0.70f));
        DrawText(ronda, xPanel + 183 - MeasureText(ronda, 28) / 2, 24, 28, GOLD);

        DrawRectangle(xPanel + 13, 64, 340, 14, Fade(WHITE, 0.25f));

        float porcentaje = tiempoFase / DURACION_ELECCION_MIRADAS;
        if (porcentaje < 0.0f) porcentaje = 0.0f;

        DrawRectangle(
            xPanel + 13,
            64,
            (int)(340.0f * porcentaje),
            14,
            porcentaje < 0.25f ? RED : YELLOW
        );
    }
    else if (fase == FASE_MIRADAS_RESOLUCION)
    {
        const char* texto =
            TextFormat(
                "SOLO MIRO: %s",
                NombreDireccionMirada(jugadores[indiceSolo].direccion)
            );

        DrawText(texto, ancho / 2 - MeasureText(texto, 28) / 2, baseTexto, 28, GOLD);
    }
    else if (fase == FASE_MIRADAS_TERMINADO)
    {
        bool ganaSolo = ContarEquipoVivoMiradas(*this) <= 0;
        const char* titulo = ganaSolo
            ? "GANA EL JUGADOR SOLITARIO"
            : "GANA EL EQUIPO";

        const int py = 16;
        const int cx = ancho - 300;
        DrawRectangle(cx - 280, py, 560, 110, Fade(BLACK, 0.88f));
        DrawText(titulo, cx - MeasureText(titulo, 32) / 2, py + 16, 32, GOLD);
        DrawText(
            TextoReinicioMinijuego(),
            cx - MeasureText(TextoReinicioMinijuego(), 19) / 2,
            py + 62,
            19,
            RAYWHITE
        );
    }
}


const ResultadoMinijuego&
MinijuegoMiradasCruzadas::ObtenerResultado() const
{
    return resultado;
}
