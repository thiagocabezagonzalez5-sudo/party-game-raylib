#include "Minigames/MinijuegoSecuenciaNeon.h"

#include "Minigames/AudioMinijuegos.h"
#include "Minigames/UtilidadesMinijuegos.h"
#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include "Systems/Input.h"

#include <cmath>


//==================================================
// SECUENCIA NEON - SALA DE SINTETIZADOR 3D
//==================================================
//
// Logica: la secuencia crece cada ronda; los jugadores la repiten con
// las cuatro direcciones y quien falla o se queda sin tiempo cae.
// Visual: cuatro paneles gigantes en el muro del fondo (se encienden al
// mostrar la secuencia) y una consola con pedestal para cada jugador.
//==================================================


static const float DURACION_PREPARACION_SECUENCIA = 2.5f;
static const float DURACION_PULSO_VISIBLE = 0.50f;
static const float DURACION_PAUSA_PULSO = 0.20f;
static const float DURACION_RESOLUCION_SECUENCIA = 1.25f;


static int ContarVivosSecuencia(
    const MinijuegoSecuenciaNeon& minijuego
)
{
    int cantidad = 0;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (
            minijuego.resultado.participantes[i].participo &&
            !minijuego.jugadores[i].eliminado
        )
        {
            cantidad++;
        }
    }

    return cantidad;
}


static Color ColorPulsoNeon(PulsoSecuenciaNeon pulso)
{
    switch (pulso)
    {
        case PULSO_NEON_ARRIBA: return SKYBLUE;
        case PULSO_NEON_DERECHA: return ORANGE;
        case PULSO_NEON_ABAJO: return LIME;
        case PULSO_NEON_IZQUIERDA: return VIOLET;
        case CANTIDAD_PULSOS_NEON: break;
    }

    return RAYWHITE;
}


static bool LeerPulsoHumano(
    const Participante& participante,
    PulsoSecuenciaNeon& pulso
)
{
    if (AccionDireccionalControlPresionada(
        participante,
        CONTROL_DIRECCION_ARRIBA
    ))
    {
        pulso = PULSO_NEON_ARRIBA;
        return true;
    }

    if (AccionDireccionalControlPresionada(
        participante,
        CONTROL_DIRECCION_DERECHA
    ))
    {
        pulso = PULSO_NEON_DERECHA;
        return true;
    }

    if (AccionDireccionalControlPresionada(
        participante,
        CONTROL_DIRECCION_ABAJO
    ))
    {
        pulso = PULSO_NEON_ABAJO;
        return true;
    }

    if (AccionDireccionalControlPresionada(
        participante,
        CONTROL_DIRECCION_IZQUIERDA
    ))
    {
        pulso = PULSO_NEON_IZQUIERDA;
        return true;
    }

    return false;
}


static void RegistrarPulsoJugador(
    MinijuegoSecuenciaNeon& minijuego,
    int indiceParticipante,
    PulsoSecuenciaNeon pulso
)
{
    EstadoJugadorSecuenciaNeon& jugador =
        minijuego.jugadores[indiceParticipante];

    if (
        jugador.eliminado ||
        jugador.completoRonda ||
        jugador.indiceRespuesta >= minijuego.cantidadPulsos
    )
    {
        return;
    }

    jugador.ultimoPulso = (int)pulso;
    jugador.tiempoUltimoPulso = 0.28f;

    if (pulso != minijuego.secuencia[jugador.indiceRespuesta])
    {
        jugador.eliminado = true;
        jugador.falloEstaRonda = true;
        jugador.ultimoPulsoCorrecto = false;
        ReproducirSonidoMinijuego(minijuego.audio, SONIDO_ERROR);
        return;
    }

    jugador.ultimoPulsoCorrecto = true;
    jugador.indiceRespuesta++;
    jugador.aciertosTotales++;
    minijuego.brilloPanel[pulso] = 0.6f;

    if (jugador.indiceRespuesta >= minijuego.cantidadPulsos)
    {
        jugador.completoRonda = true;
        jugador.rondasSuperadas++;
        ReproducirSonidoMinijuego(minijuego.audio, SONIDO_ACIERTO);
    }
    else
    {
        ReproducirSonidoMinijuego(minijuego.audio, SONIDO_BOTON);
    }
}


static bool TodosResolvidosSecuencia(
    const MinijuegoSecuenciaNeon& minijuego
)
{
    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (
            minijuego.resultado.participantes[i].participo &&
            !minijuego.jugadores[i].eliminado &&
            !minijuego.jugadores[i].completoRonda
        )
        {
            return false;
        }
    }

    return true;
}


static void ComenzarRespuestaSecuencia(
    MinijuegoSecuenciaNeon& minijuego
)
{
    minijuego.fase = FASE_SECUENCIA_RESPONDIENDO;
    minijuego.pulsoVisible = false;
    minijuego.tiempoRespuesta =
        1.5f + minijuego.cantidadPulsos * 0.78f;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        EstadoJugadorSecuenciaNeon& jugador = minijuego.jugadores[i];

        if (
            !minijuego.resultado.participantes[i].participo ||
            jugador.eliminado
        )
        {
            continue;
        }

        jugador.indiceRespuesta = 0;
        jugador.completoRonda = false;
        jugador.falloEstaRonda = false;
        jugador.tiempoRespuestaBot = GetRandomValue(24, 50) / 100.0f;

        int probabilidadFallo = 5 + minijuego.cantidadPulsos * 3 + i * 2;
        jugador.indiceFalloBot =
            GetRandomValue(0, 99) < probabilidadFallo
            ? GetRandomValue(0, minijuego.cantidadPulsos - 1)
            : -1;
    }
}


static void ComenzarMuestraSecuencia(
    MinijuegoSecuenciaNeon& minijuego,
    bool agregarPulso
)
{
    if (agregarPulso)
    {
        minijuego.secuencia[minijuego.cantidadPulsos] =
            (PulsoSecuenciaNeon)GetRandomValue(
                0,
                CANTIDAD_PULSOS_NEON - 1
            );

        minijuego.cantidadPulsos++;
    }

    minijuego.numeroRonda++;
    minijuego.indiceMuestra = 0;
    minijuego.pulsoVisible = true;
    minijuego.tiempoPasoMuestra = DURACION_PULSO_VISIBLE;
    minijuego.fase = FASE_SECUENCIA_MOSTRANDO;

    ReproducirSonidoMinijuego(minijuego.audio, SONIDO_BOTON);

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        minijuego.jugadores[i].falloEstaRonda = false;
    }
}


static int PuntuacionJugadorSecuencia(
    const EstadoJugadorSecuenciaNeon& jugador
)
{
    return
        jugador.rondasSuperadas * 100 +
        jugador.aciertosTotales +
        (!jugador.eliminado ? 1000 : 0);
}


static void FinalizarSecuenciaNeon(
    MinijuegoSecuenciaNeon& minijuego
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

        int puntuacion = PuntuacionJugadorSecuencia(minijuego.jugadores[i]);
        minijuego.jugadores[i].puntuacionFinal = puntuacion;

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

        int posicion = 1;

        for (int j = 0; j < MAX_PARTICIPANTES; j++)
        {
            if (
                minijuego.resultado.participantes[j].participo &&
                minijuego.jugadores[j].puntuacionFinal >
                    minijuego.jugadores[i].puntuacionFinal
            )
            {
                posicion++;
            }
        }

        resultadoJugador.posicionFinal = posicion;
        resultadoJugador.puntuacionMinijuego =
            minijuego.jugadores[i].rondasSuperadas * 100 +
            minijuego.jugadores[i].aciertosTotales;
    }

    minijuego.resultado.estado = RESULTADO_MINIJUEGO_FINALIZADO;
    minijuego.resultado.desenlace =
        cantidadGanadores == 1
        ? DESENLACE_CON_GANADOR
        : DESENLACE_EMPATE;

    minijuego.fase = FASE_SECUENCIA_TERMINADO;

    ReproducirSonidoMinijuego(minijuego.audio, SONIDO_RESULTADO);
}


static void ComenzarResolucionSecuencia(
    MinijuegoSecuenciaNeon& minijuego
)
{
    bool huboEliminadoPorTiempo = false;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        EstadoJugadorSecuenciaNeon& jugador = minijuego.jugadores[i];

        if (
            minijuego.resultado.participantes[i].participo &&
            !jugador.eliminado &&
            !jugador.completoRonda
        )
        {
            jugador.eliminado = true;
            jugador.falloEstaRonda = true;
            jugador.ultimoPulsoCorrecto = false;
            huboEliminadoPorTiempo = true;
        }
    }

    if (huboEliminadoPorTiempo)
    {
        ReproducirSonidoMinijuego(minijuego.audio, SONIDO_ERROR);
    }

    minijuego.fase = FASE_SECUENCIA_RESOLUCION;
    minijuego.tiempoResolucion = DURACION_RESOLUCION_SECUENCIA;
}


void MinijuegoSecuenciaNeon::Inicializar()
{
    resultado = {};
    resultado.formato = FORMATO_MINIJUEGO_INDIVIDUAL;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        jugadores[i] = {};
    }

    for (int i = 0; i < MAX_PULSOS_SECUENCIA_NEON; i++)
    {
        secuencia[i] = PULSO_NEON_ARRIBA;
    }

    for (int i = 0; i < CANTIDAD_PULSOS_NEON; i++)
    {
        brilloPanel[i] = 0.0f;
    }

    cantidadPulsos = 0;
    indiceMuestra = 0;
    numeroRonda = 0;
    fase = FASE_SECUENCIA_PREPARACION;
    pulsoVisible = false;
    tiempoPreparacion = DURACION_PREPARACION_SECUENCIA;
    tiempoPasoMuestra = 0.0f;
    tiempoRespuesta = 0.0f;
    tiempoResolucion = 0.0f;
    tiempoAnimacion = 0.0f;
}


void MinijuegoSecuenciaNeon::Reiniciar(
    Participante participantes[]
)
{
    Inicializar();
    InicializarResultadoMinijuego(
        resultado,
        participantes,
        FORMATO_MINIJUEGO_INDIVIDUAL
    );

    for (int i = 0; i < 3; i++)
    {
        secuencia[i] = (PulsoSecuenciaNeon)GetRandomValue(
            0,
            CANTIDAD_PULSOS_NEON - 1
        );
    }

    cantidadPulsos = 3;
}


void MinijuegoSecuenciaNeon::Actualizar(
    float deltaTime,
    Participante participantes[]
)
{
    tiempoAnimacion += deltaTime;

    for (int i = 0; i < CANTIDAD_PULSOS_NEON; i++)
    {
        brilloPanel[i] -= deltaTime * 2.5f;
        if (brilloPanel[i] < 0.0f) brilloPanel[i] = 0.0f;
    }

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        jugadores[i].tiempoUltimoPulso -= deltaTime;
        if (jugadores[i].tiempoUltimoPulso < 0.0f)
        {
            jugadores[i].tiempoUltimoPulso = 0.0f;
        }
    }

    if (fase == FASE_SECUENCIA_TERMINADO)
    {
        return;
    }

    if (fase == FASE_SECUENCIA_PREPARACION)
    {
        float preparacionAntes = tiempoPreparacion;
        tiempoPreparacion -= deltaTime;
        ActualizarAudioCuentaRegresiva(audio, preparacionAntes, tiempoPreparacion);

        if (tiempoPreparacion <= 0.0f)
        {
            tiempoPreparacion = 0.0f;
            ComenzarMuestraSecuencia(*this, false);
        }

        return;
    }

    if (fase == FASE_SECUENCIA_MOSTRANDO)
    {
        tiempoPasoMuestra -= deltaTime;

        if (tiempoPasoMuestra <= 0.0f)
        {
            if (pulsoVisible)
            {
                pulsoVisible = false;
                tiempoPasoMuestra = DURACION_PAUSA_PULSO;
            }
            else
            {
                indiceMuestra++;

                if (indiceMuestra >= cantidadPulsos)
                {
                    ComenzarRespuestaSecuencia(*this);
                }
                else
                {
                    pulsoVisible = true;
                    tiempoPasoMuestra = DURACION_PULSO_VISIBLE;
                    ReproducirSonidoMinijuego(audio, SONIDO_BOTON);
                }
            }
        }

        return;
    }

    if (fase == FASE_SECUENCIA_RESPONDIENDO)
    {
        float respuestaAntes = tiempoRespuesta;
        tiempoRespuesta -= deltaTime;
        ActualizarAudioAlertaTiempo(audio, respuestaAntes, tiempoRespuesta, 3.0f);

        for (int i = 0; i < MAX_PARTICIPANTES; i++)
        {
            EstadoJugadorSecuenciaNeon& jugador = jugadores[i];

            if (
                !resultado.participantes[i].participo ||
                jugador.eliminado ||
                jugador.completoRonda
            )
            {
                continue;
            }

            if (participantes[i].esBot || !participantes[i].conectado)
            {
                jugador.tiempoRespuestaBot -= deltaTime;

                if (jugador.tiempoRespuestaBot <= 0.0f)
                {
                    PulsoSecuenciaNeon pulso =
                        secuencia[jugador.indiceRespuesta];

                    if (jugador.indiceRespuesta == jugador.indiceFalloBot)
                    {
                        pulso = (PulsoSecuenciaNeon)(
                            (pulso + 1 + GetRandomValue(0, 2)) %
                            CANTIDAD_PULSOS_NEON
                        );
                    }

                    RegistrarPulsoJugador(*this, i, pulso);
                    jugador.tiempoRespuestaBot =
                        GetRandomValue(24, 50) / 100.0f;
                }
            }
            else
            {
                PulsoSecuenciaNeon pulso = PULSO_NEON_ARRIBA;

                if (LeerPulsoHumano(participantes[i], pulso))
                {
                    RegistrarPulsoJugador(*this, i, pulso);
                }
            }
        }

        if (TodosResolvidosSecuencia(*this) || tiempoRespuesta <= 0.0f)
        {
            ComenzarResolucionSecuencia(*this);
        }

        return;
    }

    tiempoResolucion -= deltaTime;

    if (tiempoResolucion <= 0.0f)
    {
        if (
            ContarVivosSecuencia(*this) <= 1 ||
            cantidadPulsos >= MAX_PULSOS_SECUENCIA_NEON
        )
        {
            FinalizarSecuenciaNeon(*this);
        }
        else
        {
            ComenzarMuestraSecuencia(*this, true);
        }
    }
}


//==================================================
// VISUAL 3D (independiente de la logica)
//==================================================
//
// MODELO FUTURO: reemplazar por GLB los cuatro paneles gigantes con sus
// flechas, las pilas de bocinas, el muro del fondo con su estructura,
// la viga de luces del techo y las consolas / pedestales de jugador.
//==================================================


static const float Z_PANELES_SECUENCIA = -8.4f;
static const float Z_JUGADORES_SECUENCIA = 2.4f;
static const float Z_CONSOLAS_SECUENCIA = 3.6f;
static const float SEPARACION_JUGADORES_SECUENCIA = 4.4f;
static const float LADO_PANEL_SECUENCIA = 2.3f;


static Camera3D ObtenerCamaraSecuencia()
{
    Camera3D camara{};
    camara.position = { 0.0f, 9.5f, 15.0f };
    camara.target = { 0.0f, 3.2f, -2.0f };
    camara.up = { 0.0f, 1.0f, 0.0f };
    camara.fovy = 50.0f;
    camara.projection = CAMERA_PERSPECTIVE;
    return camara;
}


// Centro del panel gigante (cruz de direcciones en el muro del fondo).
static Vector3 CentroPanelSecuencia(PulsoSecuenciaNeon pulso)
{
    switch (pulso)
    {
        case PULSO_NEON_ARRIBA: return { 0.0f, 7.8f, Z_PANELES_SECUENCIA };
        case PULSO_NEON_DERECHA: return { 2.7f, 5.2f, Z_PANELES_SECUENCIA };
        case PULSO_NEON_ABAJO: return { 0.0f, 2.6f, Z_PANELES_SECUENCIA };
        case PULSO_NEON_IZQUIERDA: return { -2.7f, 5.2f, Z_PANELES_SECUENCIA };
        case CANTIDAD_PULSOS_NEON: break;
    }

    return { 0.0f, 5.2f, Z_PANELES_SECUENCIA };
}


static float AnguloFlechaSecuencia(PulsoSecuenciaNeon pulso)
{
    switch (pulso)
    {
        case PULSO_NEON_ARRIBA: return 0.0f;
        case PULSO_NEON_DERECHA: return -90.0f;
        case PULSO_NEON_ABAJO: return 180.0f;
        case PULSO_NEON_IZQUIERDA: return 90.0f;
        case CANTIDAD_PULSOS_NEON: break;
    }

    return 0.0f;
}


// Flecha 3D hecha con un cubo (cuerpo) y un cono de 3 caras (punta).
// Se dibuja apuntando hacia arriba en el sistema local actual.
static void DibujarFlecha3D(float escala, Color color)
{
    DrawCube({ 0.0f, -0.3f * escala, 0.0f }, 0.5f * escala, 1.0f * escala, 0.3f * escala, color);
    DrawCylinder({ 0.0f, 0.2f * escala, 0.0f }, 0.0f, 0.75f * escala, 0.8f * escala, 3, color);
}


static void DibujarPanelNeon3D(
    PulsoSecuenciaNeon pulso,
    float luz,
    float tiempo
)
{
    Color color = ColorPulsoNeon(pulso);
    Vector3 centro = CentroPanelSecuencia(pulso);
    Color cuerpo = ColorLerp(Color{ 22, 24, 44, 255 }, color, 0.18f + 0.62f * luz);
    Color flecha = ColorLerp(Fade(color, 0.5f), WHITE, luz * 0.85f);

    // Marco y placa del panel.
    DrawCube(centro, LADO_PANEL_SECUENCIA + 0.3f, LADO_PANEL_SECUENCIA + 0.3f, 0.45f, Color{ 12, 12, 24, 255 });
    DrawCube({ centro.x, centro.y, centro.z + 0.2f }, LADO_PANEL_SECUENCIA, LADO_PANEL_SECUENCIA, 0.3f, cuerpo);
    DrawCubeWires(centro, LADO_PANEL_SECUENCIA + 0.3f, LADO_PANEL_SECUENCIA + 0.3f, 0.45f, Fade(color, 0.5f + 0.5f * luz));

    // Flecha del panel.
    rlPushMatrix();
    rlTranslatef(centro.x, centro.y, centro.z + 0.42f);
    rlRotatef(AnguloFlechaSecuencia(pulso), 0.0f, 0.0f, 1.0f);
    DibujarFlecha3D(1.0f + 0.08f * luz * std::sin(tiempo * 30.0f), flecha);
    rlPopMatrix();

    if (luz > 0.05f)
    {
        // Halo frontal y luz de escenario sobre el suelo.
        DrawCube(
            { centro.x, centro.y, centro.z + 0.5f },
            LADO_PANEL_SECUENCIA + 0.9f,
            LADO_PANEL_SECUENCIA + 0.9f,
            0.1f,
            Fade(color, 0.22f * luz)
        );
        DrawCube(
            { centro.x, 0.03f, Z_PANELES_SECUENCIA + 3.6f },
            LADO_PANEL_SECUENCIA + 0.4f,
            0.02f,
            6.4f,
            Fade(color, 0.32f * luz)
        );
    }
}


static void DibujarEscenarioSecuencia(float tiempo, float energia)
{
    // Suelo oscuro con rejilla de neon.
    DrawCube({ 0.0f, -0.2f, 0.0f }, 60.0f, 0.3f, 40.0f, Color{ 10, 10, 22, 255 });

    for (int x = -20; x <= 20; x += 2)
    {
        DrawLine3D({ (float)x, 0.0f, -9.0f }, { (float)x, 0.0f, 10.0f }, Fade(Color{ 120, 70, 220, 255 }, 0.35f));
    }

    for (int z = -8; z <= 10; z += 2)
    {
        DrawLine3D({ -20.0f, 0.0f, (float)z }, { 20.0f, 0.0f, (float)z }, Fade(Color{ 120, 70, 220, 255 }, 0.35f));
    }

    // Muro trasero con franjas de neon.
    DrawCube({ 0.0f, 7.0f, -9.1f }, 46.0f, 16.0f, 0.6f, Color{ 18, 14, 36, 255 });

    for (int f = 0; f < 4; f++)
    {
        DrawCube(
            { 0.0f, 1.0f + f * 4.2f, -8.75f },
            46.0f,
            0.06f,
            0.08f,
            Fade(Color{ 200, 90, 255, 255 }, 0.35f + 0.25f * std::sin(tiempo * 2.0f + f))
        );
    }

    // Pilas de bocinas laterales que laten con la energia de la sala.
    for (int lado = -1; lado <= 1; lado += 2)
    {
        float x = lado * 11.5f;
        DrawCube({ x, 3.5f, -6.0f }, 3.2f, 7.0f, 2.4f, Color{ 24, 22, 40, 255 });
        DrawCubeWires({ x, 3.5f, -6.0f }, 3.2f, 7.0f, 2.4f, Fade(Color{ 120, 70, 220, 255 }, 0.6f));

        for (int fila = 0; fila < 2; fila++)
        {
            float y = 2.0f + fila * 3.0f;
            float latido = 1.0f + energia * 0.12f * std::sin(tiempo * 18.0f);

            rlPushMatrix();
            rlTranslatef(x, y, -4.75f);
            rlScalef(1.0f, 1.0f, 0.25f);
            DrawSphereEx({ 0.0f, 0.0f, 0.0f }, 1.0f * latido, 10, 10, Color{ 40, 38, 60, 255 });
            DrawSphereEx({ 0.0f, 0.0f, 0.0f }, 0.45f * latido, 8, 8, Fade(Color{ 90, 220, 255, 255 }, 0.5f + 0.4f * energia));
            rlPopMatrix();
        }
    }

    // Ecualizador: barras que bailan a cada lado de los paneles.
    for (int b = 0; b < 16; b++)
    {
        float x = b < 8 ? -13.5f + b * 1.0f : 6.5f + (b - 8) * 1.0f;
        float altura = 0.4f + (std::sin(tiempo * 3.0f + b * 0.8f) * 0.5f + 0.5f) * (0.8f + energia * 1.8f);
        Color color = b % 2 == 0 ? Color{ 255, 70, 200, 255 } : Color{ 70, 220, 255, 255 };
        DrawCube({ x, altura * 0.5f, -7.6f }, 0.7f, altura, 0.7f, Fade(color, 0.85f));
    }

    // Viga de luces del techo.
    DrawCylinderEx({ -14.0f, 11.5f, -2.0f }, { 14.0f, 11.5f, -2.0f }, 0.2f, 0.2f, 8, Color{ 70, 70, 90, 255 });

    for (int l = 0; l < 8; l++)
    {
        float parpadeo = 0.5f + 0.5f * std::sin(tiempo * 4.0f + l * 1.3f);
        Color color = l % 2 == 0 ? Color{ 255, 70, 200, 255 } : Color{ 70, 220, 255, 255 };
        DrawSphereEx({ -12.0f + l * 3.4f, 11.2f, -2.0f }, 0.3f, 8, 8, Fade(color, 0.4f + 0.6f * parpadeo));
    }
}


static void DibujarConsolaJugador3D(
    const EstadoJugadorSecuenciaNeon& estado,
    const Participante& participante,
    Vector3 posicionJugador,
    int cantidadPulsos,
    bool mostrarProgreso
)
{
    float xBase = posicionJugador.x;
    Color color = participante.color;
    Color metal = estado.eliminado ? Color{ 40, 22, 26, 255 } : Color{ 36, 34, 56, 255 };

    // Pedestal bajo el jugador y consola frente a el.
    DrawCylinder({ xBase, 0.0f, Z_JUGADORES_SECUENCIA }, 1.3f, 1.4f, 0.15f, 20, Color{ 28, 28, 44, 255 });
    DrawCircle3D({ xBase, 0.17f, Z_JUGADORES_SECUENCIA }, 1.15f, { 1.0f, 0.0f, 0.0f }, 90.0f, estado.eliminado ? Fade(RED, 0.5f) : color);
    DrawCube({ xBase, 0.5f, Z_CONSOLAS_SECUENCIA }, 3.4f, 1.0f, 1.5f, metal);
    DrawCube({ xBase, 1.02f, Z_CONSOLAS_SECUENCIA + 0.7f }, 3.4f, 0.06f, 0.1f, estado.eliminado ? RED : color);

    // Cuatro botones en cruz; el ultimo pulsado se ilumina.
    static const float desplazamientoX[CANTIDAD_PULSOS_NEON] = { 0.0f, 0.75f, 0.0f, -0.75f };
    static const float desplazamientoZ[CANTIDAD_PULSOS_NEON] = { -0.4f, 0.0f, 0.4f, 0.0f };

    for (int d = 0; d < CANTIDAD_PULSOS_NEON; d++)
    {
        Color base = ColorPulsoNeon((PulsoSecuenciaNeon)d);
        bool encendido = estado.ultimoPulso == d && estado.tiempoUltimoPulso > 0.0f;
        Color colorBoton = encendido
            ? (estado.ultimoPulsoCorrecto ? WHITE : RED)
            : Fade(base, 0.35f);

        DrawCube(
            { xBase + desplazamientoX[d], 1.07f, Z_CONSOLAS_SECUENCIA + desplazamientoZ[d] },
            0.55f,
            encendido ? 0.14f : 0.08f,
            0.4f,
            colorBoton
        );
    }

    // Indicadores de progreso sobre la cabeza.
    float inicio = xBase - (cantidadPulsos - 1) * 0.17f;

    for (int k = 0; k < cantidadPulsos; k++)
    {
        Color luz = Fade(GRAY, 0.5f);

        if (estado.eliminado)
        {
            luz = Fade(RED, 0.8f);
        }
        else if (estado.completoRonda)
        {
            luz = LIME;
        }
        else if (mostrarProgreso && k < estado.indiceRespuesta)
        {
            luz = color;
        }

        DrawSphereEx({ inicio + k * 0.34f, 3.1f, Z_JUGADORES_SECUENCIA }, 0.11f, 6, 6, luz);
    }
}


void MinijuegoSecuenciaNeon::Dibujar(
    const Participante participantes[]
) const
{
    ClearBackground(Color{ 8, 8, 20, 255 });

    float energia = 0.0f;

    for (int i = 0; i < CANTIDAD_PULSOS_NEON; i++)
    {
        energia = fmaxf(energia, brilloPanel[i]);
    }

    bool mostrarPulso =
        fase == FASE_SECUENCIA_MOSTRANDO &&
        pulsoVisible &&
        indiceMuestra < cantidadPulsos;
    PulsoSecuenciaNeon pulsoEncendido = mostrarPulso
        ? secuencia[indiceMuestra]
        : PULSO_NEON_ARRIBA;

    if (mostrarPulso)
    {
        energia = 1.0f;
    }

    Camera3D camara = ObtenerCamaraSecuencia();
    BeginMode3D(camara);

    DibujarEscenarioSecuencia(tiempoAnimacion, energia);

    for (int d = 0; d < CANTIDAD_PULSOS_NEON; d++)
    {
        float luz = brilloPanel[d];

        if (mostrarPulso && (int)pulsoEncendido == d)
        {
            luz = 1.0f;
        }

        DibujarPanelNeon3D((PulsoSecuenciaNeon)d, luz, tiempoAnimacion);
    }

    // Nucleo central entre los paneles.
    Vector3 hub = { 0.0f, 5.2f, Z_PANELES_SECUENCIA + 0.3f };
    DrawSphereEx(hub, 0.55f + 0.08f * std::sin(tiempoAnimacion * 4.0f), 12, 12, Fade(Color{ 160, 120, 255, 255 }, 0.55f + 0.4f * energia));

    // Jugadores, pedestales y consolas.
    int cantidad = resultado.cantidadParticipantes > 0 ? resultado.cantidadParticipantes : 1;
    int orden = 0;
    Vector2 etiquetas[MAX_PARTICIPANTES]{};

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!resultado.participantes[i].participo)
        {
            continue;
        }

        float x = (orden - (cantidad - 1) * 0.5f) * SEPARACION_JUGADORES_SECUENCIA;
        orden++;

        DibujarConsolaJugador3D(
            jugadores[i],
            participantes[i],
            { x, 0.0f, Z_JUGADORES_SECUENCIA },
            cantidadPulsos,
            fase == FASE_SECUENCIA_RESPONDIENDO || fase == FASE_SECUENCIA_RESOLUCION
        );

        JugadorPrueba figura{};
        figura.posicion = { x, 0.15f + figura.tamano.y * 0.5f, Z_JUGADORES_SECUENCIA };
        figura.direccionMirada = { 0.0f, 0.0f, -1.0f };
        figura.enSuelo = true;
        figura.aplastado = jugadores[i].eliminado;

        Participante visual = participantes[i];
        visual.activo = true;
        visual.conectado = true;
        DibujarJugadorCuboPrueba(figura, visual);

        etiquetas[i] = GetWorldToScreen({ x, 3.7f, Z_JUGADORES_SECUENCIA }, camara);
    }

    EndMode3D();

    // Etiquetas flotantes de cada jugador.
    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!resultado.participantes[i].participo)
        {
            continue;
        }

        const char* texto = TextFormat("J%d", participantes[i].numeroJugador);
        DrawText(
            texto,
            (int)etiquetas[i].x - MeasureText(texto, 22) / 2,
            (int)etiquetas[i].y,
            22,
            jugadores[i].eliminado ? DARKGRAY : participantes[i].color
        );
    }

    // Largo de la secuencia en el nucleo central.
    Vector2 centroHub = GetWorldToScreen(hub, camara);
    const char* largo = TextFormat("%d", cantidadPulsos);
    DrawText(largo, (int)centroHub.x - MeasureText(largo, 30) / 2, (int)centroHub.y - 15, 30, RAYWHITE);

    DrawRectangle(18, 16, 540, 116, Fade(BLACK, 0.76f));
    DrawText("SECUENCIA NEON", 32, 28, 30, GOLD);

    int ancho = GetScreenWidth();
    int alto = GetScreenHeight();

    if (fase == FASE_SECUENCIA_PREPARACION)
    {
        DrawText("PREPARATE", 32, 68, 21, RAYWHITE);
        DrawText(
            TextFormat("EMPIEZA EN %.1f", tiempoPreparacion),
            32,
            99,
            18,
            LIGHTGRAY
        );

        const char* numero = TextFormat("%d", (int)std::ceil(tiempoPreparacion));
        DrawText(numero, ancho / 2 - MeasureText(numero, 120) / 2, alto / 2 - 40, 120, Fade(GOLD, 0.9f));
    }
    else if (fase == FASE_SECUENCIA_MOSTRANDO)
    {
        DrawText("MEMORIZA LOS PULSOS", 32, 68, 21, SKYBLUE);
        DrawText(
            TextFormat("RONDA %d  -  LONGITUD %d", numeroRonda, cantidadPulsos),
            32,
            99,
            18,
            LIGHTGRAY
        );
    }
    else if (fase == FASE_SECUENCIA_RESPONDIENDO)
    {
        DrawText("REPITE LA SECUENCIA CON LAS DIRECCIONES", 32, 68, 19, LIME);
        DrawText(
            TextFormat("TIEMPO: %.1f", tiempoRespuesta),
            32,
            99,
            18,
            tiempoRespuesta <= 3.0f ? ORANGE : LIGHTGRAY
        );
    }
    else if (fase == FASE_SECUENCIA_RESOLUCION)
    {
        DrawText("RESULTADO DE LA RONDA", 32, 74, 21, RAYWHITE);
    }
    else
    {
        DrawText("SECUENCIA TERMINADA", 32, 68, 21, RAYWHITE);
        DrawText(TextoReinicioMinijuego(), 32, 99, 18, LIGHTGRAY);
    }

    int anchoPanel = 240;
    int separacion = 14;
    int anchoTotal = cantidad * anchoPanel + (cantidad - 1) * separacion;
    int xInicial = (ancho - anchoTotal) / 2;
    int altoPanel = 38;
    int y = alto - altoPanel - 8;
    orden = 0;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!resultado.participantes[i].participo)
        {
            continue;
        }

        int x = xInicial + orden * (anchoPanel + separacion);
        const EstadoJugadorSecuenciaNeon& jugador = jugadores[i];
        Color color = jugador.eliminado
            ? Fade(participantes[i].color, 0.30f)
            : participantes[i].color;

        DrawRectangle(x, y, anchoPanel, altoPanel, Fade(BLACK, 0.78f));
        DrawRectangleLinesEx(
            { (float)x, (float)y, (float)anchoPanel, (float)altoPanel },
            3.0f,
            color
        );

        DrawText(
            TextFormat("J%d", participantes[i].numeroJugador),
            x + 12,
            y + 9,
            20,
            color
        );

        if (fase == FASE_SECUENCIA_TERMINADO)
        {
            DrawText(
                resultado.participantes[i].posicionFinal == 1
                    ? "GANADOR"
                    : TextFormat("PUESTO %d", resultado.participantes[i].posicionFinal),
                x + 64,
                y + 10,
                18,
                resultado.participantes[i].posicionFinal == 1
                    ? GOLD
                    : LIGHTGRAY
            );
        }
        else if (jugador.eliminado)
        {
            DrawText(
                jugador.falloEstaRonda ? "ERROR" : "ELIMINADO",
                x + 64,
                y + 10,
                18,
                RED
            );
        }
        else if (fase == FASE_SECUENCIA_RESPONDIENDO)
        {
            DrawText(
                TextFormat("%d / %d", jugador.indiceRespuesta, cantidadPulsos),
                x + 64,
                y + 10,
                18,
                RAYWHITE
            );
        }
        else
        {
            DrawText(
                TextFormat("RONDAS: %d", jugador.rondasSuperadas),
                x + 64,
                y + 10,
                18,
                LIGHTGRAY
            );
        }

        orden++;
    }
}


const ResultadoMinijuego&
MinijuegoSecuenciaNeon::ObtenerResultado() const
{
    return resultado;
}
