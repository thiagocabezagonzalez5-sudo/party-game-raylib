#include "Minigames/MinijuegoPasoSilencioso.h"

#include "Minigames/AudioMinijuegos.h"
#include "Minigames/EfectosVisualesMinijuegos.h"
#include "Minigames/ModeloJugadorCompartido.h"
#include "Minigames/TiposMinijuegos.h"
#include "Systems/Input.h"

#include <cmath>


static const float DURACION_PREPARACION_PASO = 2.5f;
static const float DURACION_CARRERA_PASO = 35.0f;
static const float VELOCIDAD_AVANCE_PASO = 0.105f;
static const float RETROCESO_ALERTA_PASO = 0.17f;
static const float GRACIA_ALERTA_PASO = 0.3f;
static const float AVISO_PREVIO_PASO = 0.4f;


static float PosicionXCarrilPaso(int indiceParticipante)
{
    static const float posiciones[MAX_PARTICIPANTES] =
    {
        -3.6f,
        -1.2f,
         1.2f,
         3.6f
    };

    return posiciones[indiceParticipante];
}


static float PosicionZProgresoPaso(float progreso)
{
    return 7.0f - Clamp(progreso, 0.0f, 1.0f) * 15.0f;
}


static float NuevaDuracionEstadoCentinela(bool alerta)
{
    if (alerta)
    {
        return GetRandomValue(90, 175) / 100.0f;
    }

    return GetRandomValue(220, 390) / 100.0f;
}


static void CambiarEstadoCentinela(
    MinijuegoPasoSilencioso& minijuego,
    const Participante participantes[]
)
{
    minijuego.centinelaAlerta = !minijuego.centinelaAlerta;
    minijuego.avisoPrevio = false;
    minijuego.tiempoEnAlerta = 0.0f;
    minijuego.tiempoEstadoCentinela =
        NuevaDuracionEstadoCentinela(minijuego.centinelaAlerta);

    if (minijuego.centinelaAlerta)
    {
        ReproducirSonidoMinijuego(minijuego.audio, SONIDO_IMPACTO);
    }

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        EstadoJugadorPasoSilencioso& jugador = minijuego.jugadores[i];
        jugador.castigadoEnAlerta = false;

        if (
            !minijuego.resultado.participantes[i].participo ||
            !(participantes[i].esBot || !participantes[i].conectado)
        )
        {
            continue;
        }

        if (minijuego.centinelaAlerta)
        {
            // Quien no se anticipo al aviso reacciona en 0.18-0.45 s.
            jugador.botAvanzando = !jugador.botAnticipo;
            jugador.reaccionBot = jugador.botAnticipo
                ? 0.0f
                : GetRandomValue(18, 45) / 100.0f;
        }
        else
        {
            jugador.botAnticipo = false;
            jugador.botAvanzando = false;
            jugador.reaccionBot = GetRandomValue(18, 45) / 100.0f;
        }
    }
}


static int PuntuacionCarreraPaso(
    const EstadoJugadorPasoSilencioso& jugador
)
{
    return (int)(Clamp(jugador.progreso, 0.0f, 1.0f) * 10000.0f + 0.5f);
}


static void FinalizarPasoSilencioso(
    MinijuegoPasoSilencioso& minijuego
)
{
    int mejorPuntuacion = -1;
    int cantidadGanadores = 0;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!minijuego.resultado.participantes[i].participo)
        {
            continue;
        }

        int puntuacion = PuntuacionCarreraPaso(minijuego.jugadores[i]);

        if (puntuacion > mejorPuntuacion)
        {
            mejorPuntuacion = puntuacion;
            cantidadGanadores = 1;
        }
        else if (puntuacion == mejorPuntuacion)
        {
            cantidadGanadores++;
        }
    }

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        ResultadoParticipante& resultadoJugador =
            minijuego.resultado.participantes[i];

        if (!resultadoJugador.participo)
        {
            continue;
        }

        int puntuacionCarrera = PuntuacionCarreraPaso(minijuego.jugadores[i]);
        int posicion = 1;

        for (int j = 0; j < MAX_PARTICIPANTES; j++)
        {
            if (
                minijuego.resultado.participantes[j].participo &&
                PuntuacionCarreraPaso(minijuego.jugadores[j]) > puntuacionCarrera
            )
            {
                posicion++;
            }
        }

        resultadoJugador.posicionFinal = posicion;
        resultadoJugador.puntuacionMinijuego =
            puntuacionCarrera / 10 -
            minijuego.jugadores[i].penalizaciones * 20;

        if (resultadoJugador.puntuacionMinijuego < 0)
        {
            resultadoJugador.puntuacionMinijuego = 0;
        }
    }

    minijuego.resultado.estado = RESULTADO_MINIJUEGO_FINALIZADO;
    minijuego.resultado.desenlace =
        cantidadGanadores == 1
        ? DESENLACE_CON_GANADOR
        : DESENLACE_EMPATE;

    minijuego.fase = FASE_PASO_TERMINADO;

    ReproducirSonidoMinijuego(minijuego.audio, SONIDO_RESULTADO);
}


static void DibujarCentinelaPaso(
    const MinijuegoPasoSilencioso& minijuego
)
{
    Color colorLuz = minijuego.centinelaAlerta
        ? RED
        : (minijuego.avisoPrevio ? ORANGE : LIME);
    float giro = minijuego.centinelaAlerta
        ? std::sin(minijuego.tiempoAnimacion * 8.0f) * 18.0f
        : (minijuego.avisoPrevio
            ? 180.0f + std::sin(minijuego.tiempoAnimacion * 45.0f) * 40.0f
            : 180.0f);

    // Pedestal y brazos de piedra de la estatua.
    DrawCube({ 0.0f, 0.2f, -9.2f }, 3.6f, 0.4f, 3.4f, Color{ 84, 86, 104, 255 });
    DrawCube({ -1.7f, 1.7f, -9.2f }, 0.5f, 0.5f, 1.2f, Color{ 64, 70, 82, 255 });
    DrawCube({ 1.7f, 1.7f, -9.2f }, 0.5f, 0.5f, 1.2f, Color{ 64, 70, 82, 255 });

    DrawCylinder(
        { 0.0f, 1.15f, -9.2f },
        1.15f,
        1.45f,
        2.3f,
        12,
        Color{ 64, 70, 82, 255 }
    );

    DrawCylinderWires(
        { 0.0f, 1.15f, -9.2f },
        1.15f,
        1.45f,
        2.3f,
        12,
        LIGHTGRAY
    );

    Vector3 cabeza = { 0.0f, 3.25f, -9.2f };
    DrawSphere(cabeza, 0.82f, Color{ 45, 48, 58, 255 });

    float desplazamientoX = std::sin(giro * DEG2RAD) * 0.38f;
    DrawSphere(
        { cabeza.x + desplazamientoX, cabeza.y + 0.08f, cabeza.z + 0.66f },
        0.19f,
        colorLuz
    );

    if (minijuego.centinelaAlerta)
    {
        Vector3 origenVision =
        {
            cabeza.x + desplazamientoX,
            cabeza.y,
            cabeza.z + 0.65f
        };

        Vector3 bordeIzquierdo = { -5.0f, 0.08f, 7.8f };
        Vector3 bordeDerecho = { 5.0f, 0.08f, 7.8f };

        DrawTriangle3D(
            origenVision,
            bordeIzquierdo,
            bordeDerecho,
            Fade(RED, 0.10f)
        );

        DrawTriangle3D(
            origenVision,
            bordeDerecho,
            bordeIzquierdo,
            Fade(RED, 0.10f)
        );
    }
}


//==================================================
// ESCENARIO: TEMPLO NOCTURNO DEL CENTINELA (solo visual)
//==================================================
//
// MODELO FUTURO: reemplazar por GLB las columnas con capitel, el muro del
// fondo con su arco, la estatua del centinela con su pedestal, las antorchas
// y la alfombra ceremonial. La logica (carriles, meta, estados) no depende
// de nada de esto.
//==================================================

static void DibujarEscenarioTemploPaso(
    const MinijuegoPasoSilencioso& minijuego
)
{
    bool alerta = minijuego.centinelaAlerta;
    Color luz = alerta ? Color{ 255, 70, 60, 255 } : Color{ 110, 235, 130, 255 };

    // Suelo de piedra y alfombra ceremonial bajo los carriles.
    DrawPlane({ 0.0f, -0.09f, -2.0f }, { 40.0f, 40.0f }, Color{ 22, 24, 36, 255 });
    DrawCube({ 0.0f, -0.03f, -0.5f }, 10.6f, 0.04f, 17.6f, Color{ 96, 28, 40, 255 });
    DrawCube({ -5.2f, 0.0f, -0.5f }, 0.14f, 0.05f, 17.6f, GOLD);
    DrawCube({ 5.2f, 0.0f, -0.5f }, 0.14f, 0.05f, 17.6f, GOLD);

    // Columnas a ambos lados con antorchas que cambian de color.
    for (int i = 0; i < 5; i++)
    {
        float z = 7.0f - (float)i * 4.2f;

        for (int lado = -1; lado <= 1; lado += 2)
        {
            float x = (float)lado * 8.0f;

            DrawCube({ x, 0.25f, z }, 1.5f, 0.5f, 1.5f, Color{ 70, 72, 88, 255 });
            DrawCylinder({ x, 0.5f, z }, 0.55f, 0.48f, 2.8f, 10, Color{ 120, 122, 140, 255 });
            DrawCube({ x, 3.5f, z }, 1.5f, 0.45f, 1.5f, Color{ 84, 86, 104, 255 });
            DrawSphere({ x - (float)lado * 0.95f, 2.1f, z }, 0.2f, luz);
            DrawCircle3D(
                { x - (float)lado * 0.95f, 0.01f, z },
                1.1f,
                { 1.0f, 0.0f, 0.0f },
                90.0f,
                Fade(luz, 0.35f)
            );
        }
    }

    // Muro del fondo con arco hacia la meta.
    DrawCube({ 0.0f, 3.0f, -13.0f }, 20.0f, 6.0f, 0.8f, Color{ 48, 50, 68, 255 });
    DrawCube({ 0.0f, 2.2f, -12.55f }, 5.0f, 4.4f, 0.2f, Color{ 14, 16, 26, 255 });
    DrawCube({ -2.7f, 2.4f, -12.5f }, 0.5f, 4.8f, 0.5f, Color{ 120, 122, 140, 255 });
    DrawCube({ 2.7f, 2.4f, -12.5f }, 0.5f, 4.8f, 0.5f, Color{ 120, 122, 140, 255 });
    DrawCube({ 0.0f, 5.0f, -12.5f }, 6.0f, 0.6f, 0.6f, Color{ 120, 122, 140, 255 });

    // Meta: arco dorado sobre la linea de llegada.
    DrawCube({ -5.2f, 1.0f, -8.05f }, 0.25f, 2.0f, 0.25f, GOLD);
    DrawCube({ 5.2f, 1.0f, -8.05f }, 0.25f, 2.0f, 0.25f, GOLD);
    DrawCube({ 0.0f, 2.0f, -8.05f }, 10.65f, 0.25f, 0.25f, GOLD);

    // Pedestales de salida bajo cada carril.
    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        DrawCube(
            { PosicionXCarrilPaso(i), 0.02f, 7.75f },
            1.9f, 0.16f, 0.5f,
            Color{ 150, 150, 168, 255 }
        );
    }

    // Reflector del suelo frente a la estatua: verde dormido, rojo alerta.
    DrawCircle3D(
        { 0.0f, 0.02f, -8.6f },
        3.2f,
        { 1.0f, 0.0f, 0.0f },
        90.0f,
        Fade(luz, 0.45f)
    );
}


void MinijuegoPasoSilencioso::Inicializar()
{
    resultado = {};
    resultado.formato = FORMATO_MINIJUEGO_INDIVIDUAL;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        jugadores[i] = {};
    }

    camara.position = { 0.0f, 10.8f, 14.5f };
    camara.target = { 0.0f, 0.7f, -1.2f };
    camara.up = { 0.0f, 1.0f, 0.0f };
    camara.fovy = 46.0f;
    camara.projection = CAMERA_PERSPECTIVE;

    fase = FASE_PASO_PREPARACION;
    centinelaAlerta = false;
    tiempoPreparacion = DURACION_PREPARACION_PASO;
    tiempoEstadoCentinela = NuevaDuracionEstadoCentinela(false);
    tiempoCarrera = DURACION_CARRERA_PASO;
    tiempoAnimacion = 0.0f;
}


void MinijuegoPasoSilencioso::Reiniciar(
    Participante participantes[]
)
{
    Inicializar();
    InicializarResultadoMinijuego(
        resultado,
        participantes,
        FORMATO_MINIJUEGO_INDIVIDUAL
    );

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        jugadores[i].reaccionBot = GetRandomValue(18, 45) / 100.0f;
    }
}


void MinijuegoPasoSilencioso::Actualizar(
    float deltaTime,
    Participante participantes[]
)
{
    tiempoAnimacion += deltaTime;

    if (fase == FASE_PASO_TERMINADO)
    {
        return;
    }

    if (fase == FASE_PASO_PREPARACION)
    {
        float preparacionAntes = tiempoPreparacion;
        tiempoPreparacion -= deltaTime;
        ActualizarAudioCuentaRegresiva(audio, preparacionAntes, tiempoPreparacion);

        if (tiempoPreparacion <= 0.0f)
        {
            fase = FASE_PASO_CARRERA;
            tiempoPreparacion = 0.0f;
        }

        return;
    }

    float carreraAntes = tiempoCarrera;
    tiempoCarrera -= deltaTime;
    ActualizarAudioAlertaTiempo(audio, carreraAntes, tiempoCarrera);
    tiempoEstadoCentinela -= deltaTime;

    if (centinelaAlerta)
    {
        tiempoEnAlerta += deltaTime;
    }
    else if (!avisoPrevio && tiempoEstadoCentinela <= AVISO_PREVIO_PASO)
    {
        // Aviso: el centinela tiembla antes de girar la cabeza.
        avisoPrevio = true;
        ReproducirSonidoMinijuego(audio, SONIDO_ALERTA_TIEMPO);

        for (int i = 0; i < MAX_PARTICIPANTES; i++)
        {
            jugadores[i].botAnticipo =
                resultado.participantes[i].participo &&
                (participantes[i].esBot || !participantes[i].conectado) &&
                GetRandomValue(0, 99) < 30;
        }
    }

    if (tiempoEstadoCentinela <= 0.0f)
    {
        CambiarEstadoCentinela(*this, participantes);
    }

    bool alguienLlego = false;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (
            !resultado.participantes[i].participo ||
            jugadores[i].llegoMeta
        )
        {
            continue;
        }

        EstadoJugadorPasoSilencioso& jugador = jugadores[i];
        bool avanzando = false;

        jugador.flashCastigo = Clamp(jugador.flashCastigo - deltaTime, 0.0f, 10.0f);
        jugador.perdidaFlotante = Clamp(jugador.perdidaFlotante - deltaTime, 0.0f, 10.0f);
        jugador.progresoVisual += (jugador.progreso - jugador.progresoVisual) *
            Clamp(deltaTime * 9.0f, 0.0f, 1.0f);

        if (participantes[i].esBot || !participantes[i].conectado)
        {
            jugador.reaccionBot -= deltaTime;

            if (jugador.botAnticipo)
            {
                jugador.botAvanzando = false;
            }
            else if (jugador.reaccionBot <= 0.0f)
            {
                jugador.botAvanzando = !centinelaAlerta;
            }

            avanzando = jugador.botAvanzando;
        }
        else
        {
            InputMinijuegoParticipante entrada =
                LeerInputMinijuegoParticipante(participantes[i]);

            avanzando = entrada.adelante;
        }

        if (centinelaAlerta)
        {
            if (
                avanzando &&
                !jugador.castigadoEnAlerta &&
                tiempoEnAlerta > GRACIA_ALERTA_PASO
            )
            {
                float antes = jugador.progreso;
                jugador.progreso -= RETROCESO_ALERTA_PASO;
                jugador.progreso = Clamp(jugador.progreso, 0.0f, 1.0f);
                jugador.perdidaFlotante = 1.4f;
                jugador.flashCastigo = 0.5f;
                jugador.progresoVisual = antes;
                jugador.penalizaciones++;
                jugador.castigadoEnAlerta = true;
                ReproducirSonidoMinijuego(audio, SONIDO_GOLPE);
            }

            continue;
        }

        if (avanzando)
        {
            jugador.progreso += VELOCIDAD_AVANCE_PASO * deltaTime;
            jugador.progreso = Clamp(jugador.progreso, 0.0f, 1.0f);

            if (jugador.progreso >= 1.0f)
            {
                jugador.llegoMeta = true;
                alguienLlego = true;
            }
        }
    }

    if (alguienLlego || tiempoCarrera <= 0.0f)
    {
        tiempoCarrera = tiempoCarrera < 0.0f ? 0.0f : tiempoCarrera;
        FinalizarPasoSilencioso(*this);
    }
}


void MinijuegoPasoSilencioso::Dibujar(
    const Participante participantes[]
) const
{
    ClearBackground(Color{ 14, 16, 27, 255 });

    BeginMode3D(camara);

    DibujarEscenarioTemploPaso(*this);

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        float x = PosicionXCarrilPaso(i);
        DrawCube(
            { x, 0.0f, -0.5f },
            1.85f,
            0.10f,
            17.0f,
            i % 2 == 0
                ? Color{ 53, 60, 78, 255 }
                : Color{ 43, 50, 68, 255 }
        );

        DrawLine3D(
            { x - 0.95f, 0.07f, 7.9f },
            { x - 0.95f, 0.07f, -8.9f },
            Fade(RAYWHITE, 0.25f)
        );
    }

    DrawCube(
        { 0.0f, 0.08f, -8.05f },
        10.2f,
        0.16f,
        0.28f,
        GOLD
    );

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!resultado.participantes[i].participo)
        {
            continue;
        }

        float x = PosicionXCarrilPaso(i);
        float z = PosicionZProgresoPaso(jugadores[i].progresoVisual);

        DrawCircle3D(
            { x, 0.03f, z },
            0.52f,
            { 1.0f, 0.0f, 0.0f },
            90.0f,
            Fade(BLACK, 0.36f)
        );

        if (jugadores[i].flashCastigo > 0.0f)
        {
            DrawCircle3D(
                { x, 0.05f, z },
                0.55f + (0.5f - jugadores[i].flashCastigo) * 1.6f,
                { 1.0f, 0.0f, 0.0f },
                90.0f,
                Fade(RED, jugadores[i].flashCastigo * 1.6f)
            );
        }

        DibujarModeloJugadorEnPosicion(
            { x, 0.04f, z },
            180.0f,
            participantes[i].color
        );
    }

    DibujarCentinelaPaso(*this);

    EndMode3D();

    // "-X%" flotante sobre quien fue castigado y destello si es un humano.
    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!resultado.participantes[i].participo)
        {
            continue;
        }

        bool humano = !(participantes[i].esBot || !participantes[i].conectado);

        if (humano && jugadores[i].flashCastigo > 0.0f)
        {
            DrawRectangleLinesEx(
                { 0.0f, 0.0f, (float)GetScreenWidth(), (float)GetScreenHeight() },
                14.0f,
                Fade(RED, jugadores[i].flashCastigo)
            );
        }

        if (jugadores[i].perdidaFlotante <= 0.0f)
        {
            continue;
        }

        float subida = (1.4f - jugadores[i].perdidaFlotante) * 1.2f;
        Vector2 pantalla = GetWorldToScreen(
            { PosicionXCarrilPaso(i), 2.4f + subida, PosicionZProgresoPaso(jugadores[i].progresoVisual) },
            camara
        );
        const char* texto = TextFormat("-%d%%", (int)(RETROCESO_ALERTA_PASO * 100.0f + 0.5f));
        int ancho = MeasureText(texto, 30);

        DrawText(texto, (int)pantalla.x - ancho / 2 + 2, (int)pantalla.y + 2, 30, Fade(BLACK, 0.7f));
        DrawText(texto, (int)pantalla.x - ancho / 2, (int)pantalla.y, 30,
            Fade(RED, Clamp(jugadores[i].perdidaFlotante, 0.0f, 1.0f)));
    }

    DrawRectangle(18, 16, 515, 126, Fade(BLACK, 0.78f));
    DrawText("PASO SILENCIOSO", 32, 28, 30, GOLD);

    if (fase == FASE_PASO_PREPARACION)
    {
        DrawText("PREPARATE", 32, 68, 23, RAYWHITE);
        DrawText(
            TextFormat("EMPIEZA EN %.1f", tiempoPreparacion),
            32,
            101,
            18,
            LIGHTGRAY
        );
    }
    else if (fase == FASE_PASO_CARRERA)
    {
        DrawText(
            centinelaAlerta
                ? "ALTO! TE ESTA MIRANDO"
                : (avisoPrevio ? "CUIDADO... VA A MIRAR" : "AVANZA! ESTA DORMIDO"),
            32,
            68,
            23,
            centinelaAlerta ? RED : (avisoPrevio ? ORANGE : LIME)
        );

        DrawText(
            TextFormat("TIEMPO: %.1f", tiempoCarrera),
            32,
            101,
            18,
            LIGHTGRAY
        );
    }
    else
    {
        DrawText("RESULTADOS", 32, 68, 23, RAYWHITE);
        DrawText(TextoReinicioMinijuego(), 32, 101, 18, LIGHTGRAY);
    }

    int yInicial = GetScreenHeight() - 174;
    int anchoBarra = 240;
    int separacion = 16;
    int cantidadActivos = resultado.cantidadParticipantes;
    int anchoTotal = cantidadActivos * anchoBarra +
        (cantidadActivos - 1) * separacion;
    int xInicial = (GetScreenWidth() - anchoTotal) / 2;
    int orden = 0;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!resultado.participantes[i].participo)
        {
            continue;
        }

        int x = xInicial + orden * (anchoBarra + separacion);
        float progreso = Clamp(jugadores[i].progreso, 0.0f, 1.0f);

        DrawRectangle(x, yInicial, anchoBarra, 88, Fade(BLACK, 0.78f));
        DrawText(
            TextFormat("J%d", participantes[i].numeroJugador),
            x + 12,
            yInicial + 10,
            19,
            participantes[i].color
        );

        DrawRectangle(x + 12, yInicial + 39, anchoBarra - 24, 18, DARKGRAY);
        DrawRectangle(
            x + 12,
            yInicial + 39,
            (int)((anchoBarra - 24) * progreso),
            18,
            participantes[i].color
        );

        if (fase == FASE_PASO_TERMINADO)
        {
            DrawText(
                resultado.participantes[i].posicionFinal == 1
                    ? "GANADOR"
                    : TextFormat("PUESTO %d", resultado.participantes[i].posicionFinal),
                x + 12,
                yInicial + 63,
                16,
                resultado.participantes[i].posicionFinal == 1
                    ? GOLD
                    : LIGHTGRAY
            );
        }
        else
        {
            DrawText(
                TextFormat("CASTIGOS: %d", jugadores[i].penalizaciones),
                x + 12,
                yInicial + 63,
                15,
                LIGHTGRAY
            );
        }

        orden++;
    }

    if (fase != FASE_PASO_TERMINADO)
    {
        DrawRectangle(
            GetScreenWidth() / 2 - 350,
            154,
            700,
            28,
            Fade(BLACK, 0.6f)
        );

        DrawText(
            "MANTEN ADELANTE PARA AVANZAR. SUELTA CUANDO SE PONGA ROJO.",
            GetScreenWidth() / 2 - 338,
            158,
            18,
            RAYWHITE
        );
    }
}


const ResultadoMinijuego&
MinijuegoPasoSilencioso::ObtenerResultado() const
{
    return resultado;
}
