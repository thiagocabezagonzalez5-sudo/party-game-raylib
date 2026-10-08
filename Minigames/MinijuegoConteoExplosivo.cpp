#include "Minigames/MinijuegoConteoExplosivo.h"

#include "Minigames/AudioMinijuegos.h"
#include "Minigames/TiposMinijuegos.h"
#include "Systems/Input.h"

#include <cmath>


static const float DURACION_PREPARACION_CONTEO = 2.5f;
static const float DURACION_OBSERVACION_CONTEO = 8.0f;
static const float DURACION_RESPUESTA_CONTEO = 7.0f;


static Color ColorDronConteo(int indice)
{
    const Color colores[] =
    {
        SKYBLUE,
        ORANGE,
        LIME,
        VIOLET,
        GOLD
    };

    return colores[indice % 5];
}


static void PrepararDronesConteo(
    MinijuegoConteoExplosivo& minijuego
)
{
    minijuego.cantidadDrones = GetRandomValue(
        12,
        MAX_DRONES_CONTEO_EXPLOSIVO
    );

    for (int i = 0; i < minijuego.cantidadDrones; i++)
    {
        DronConteoExplosivo& dron = minijuego.drones[i];

        dron.posicion =
        {
            GetRandomValue(-540, 540) / 100.0f,
            GetRandomValue(90, 250) / 100.0f,
            GetRandomValue(-330, 330) / 100.0f
        };

        float velocidadX = GetRandomValue(-120, 120) / 100.0f;
        float velocidadZ = GetRandomValue(-105, 105) / 100.0f;

        if (std::fabs(velocidadX) < 0.38f)
        {
            velocidadX = velocidadX < 0.0f ? -0.62f : 0.62f;
        }

        if (std::fabs(velocidadZ) < 0.32f)
        {
            velocidadZ = velocidadZ < 0.0f ? -0.54f : 0.54f;
        }

        dron.velocidad = { velocidadX, 0.0f, velocidadZ };
        dron.color = ColorDronConteo(i);
        dron.faseFlotacion = GetRandomValue(0, 628) / 100.0f;
    }
}


static void ActualizarDronesConteo(
    MinijuegoConteoExplosivo& minijuego,
    float deltaTime
)
{
    for (int i = 0; i < minijuego.cantidadDrones; i++)
    {
        DronConteoExplosivo& dron = minijuego.drones[i];

        dron.posicion.x += dron.velocidad.x * deltaTime;
        dron.posicion.z += dron.velocidad.z * deltaTime;

        if (dron.posicion.x < -5.7f || dron.posicion.x > 5.7f)
        {
            dron.posicion.x = Clamp(dron.posicion.x, -5.7f, 5.7f);
            dron.velocidad.x *= -1.0f;
        }

        if (dron.posicion.z < -3.5f || dron.posicion.z > 3.5f)
        {
            dron.posicion.z = Clamp(dron.posicion.z, -3.5f, 3.5f);
            dron.velocidad.z *= -1.0f;
        }
    }
}


static void FinalizarConteo(
    MinijuegoConteoExplosivo& minijuego
)
{
    int mejorDiferencia = 1000;
    int cantidadGanadores = 0;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!minijuego.resultado.participantes[i].participo)
        {
            continue;
        }

        EstadoJugadorConteoExplosivo& jugador = minijuego.jugadores[i];
        jugador.diferencia = std::abs(
            jugador.respuesta - minijuego.cantidadDrones
        );

        if (jugador.diferencia < mejorDiferencia)
        {
            mejorDiferencia = jugador.diferencia;
            cantidadGanadores = 1;
        }
        else if (jugador.diferencia == mejorDiferencia)
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
                minijuego.jugadores[j].diferencia <
                    minijuego.jugadores[i].diferencia
            )
            {
                posicion++;
            }
        }

        resultadoJugador.posicionFinal = posicion;
        resultadoJugador.puntuacionMinijuego =
            100 - minijuego.jugadores[i].diferencia * 15;

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

    minijuego.fase = FASE_CONTEO_TERMINADO;

    ReproducirSonidoMinijuego(minijuego.audio, SONIDO_RESULTADO);
    minijuego.tiempoFase = 0.0f;
}


static void DibujarDronConteo(
    const DronConteoExplosivo& dron,
    float tiempoAnimacion
)
{
    Vector3 posicion = dron.posicion;
    posicion.y += std::sin(
        tiempoAnimacion * 2.8f + dron.faseFlotacion
    ) * 0.14f;

    DrawSphere(posicion, 0.24f, dron.color);
    DrawSphereWires(posicion, 0.25f, 6, 8, Fade(RAYWHITE, 0.8f));

    DrawCube(
        { posicion.x - 0.35f, posicion.y, posicion.z },
        0.34f,
        0.06f,
        0.12f,
        LIGHTGRAY
    );

    DrawCube(
        { posicion.x + 0.35f, posicion.y, posicion.z },
        0.34f,
        0.06f,
        0.12f,
        LIGHTGRAY
    );
}


static const char* TextoFaseConteo(
    FaseConteoExplosivo fase
)
{
    switch (fase)
    {
        case FASE_CONTEO_PREPARACION: return "PREPARATE";
        case FASE_CONTEO_OBSERVACION: return "CUENTA LOS DRONES";
        case FASE_CONTEO_RESPUESTA: return "CUANTOS VISTE?";
        case FASE_CONTEO_TERMINADO: return "RESULTADOS";
    }

    return "";
}


void MinijuegoConteoExplosivo::Inicializar()
{
    resultado = {};
    resultado.formato = FORMATO_MINIJUEGO_INDIVIDUAL;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        jugadores[i] = {};
    }

    camara.position = { 0.0f, 9.0f, 11.0f };
    camara.target = { 0.0f, 1.1f, 0.0f };
    camara.up = { 0.0f, 1.0f, 0.0f };
    camara.fovy = 48.0f;
    camara.projection = CAMERA_PERSPECTIVE;

    fase = FASE_CONTEO_PREPARACION;
    cantidadDrones = 0;
    tiempoFase = DURACION_PREPARACION_CONTEO;
    tiempoAnimacion = 0.0f;
}


void MinijuegoConteoExplosivo::Reiniciar(
    Participante participantes[]
)
{
    Inicializar();
    InicializarResultadoMinijuego(
        resultado,
        participantes,
        FORMATO_MINIJUEGO_INDIVIDUAL
    );

    PrepararDronesConteo(*this);

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        jugadores[i].respuesta = 17;
        jugadores[i].retrasoBot = GetRandomValue(35, 160) / 100.0f;
    }
}


void MinijuegoConteoExplosivo::Actualizar(
    float deltaTime,
    Participante participantes[]
)
{
    tiempoAnimacion += deltaTime;

    if (
        fase == FASE_CONTEO_PREPARACION ||
        fase == FASE_CONTEO_OBSERVACION
    )
    {
        ActualizarDronesConteo(*this, deltaTime);
    }

    if (fase == FASE_CONTEO_TERMINADO)
    {
        return;
    }

    float faseAntes = tiempoFase;
    tiempoFase -= deltaTime;

    if (fase == FASE_CONTEO_PREPARACION)
    {
        ActualizarAudioCuentaRegresiva(audio, faseAntes, tiempoFase);
    }
    else if (fase == FASE_CONTEO_RESPUESTA)
    {
        ActualizarAudioAlertaTiempo(audio, faseAntes, tiempoFase, 3.0f);
    }

    if (fase == FASE_CONTEO_PREPARACION)
    {
        if (tiempoFase <= 0.0f)
        {
            fase = FASE_CONTEO_OBSERVACION;
            tiempoFase = DURACION_OBSERVACION_CONTEO;
        }

        return;
    }

    if (fase == FASE_CONTEO_OBSERVACION)
    {
        if (tiempoFase <= 0.0f)
        {
            fase = FASE_CONTEO_RESPUESTA;
            ReproducirSonidoMinijuego(audio, SONIDO_INICIO_MINIJUEGO);
            tiempoFase = DURACION_RESPUESTA_CONTEO;
        }

        return;
    }

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!resultado.participantes[i].participo)
        {
            continue;
        }

        EstadoJugadorConteoExplosivo& jugador = jugadores[i];

        if (participantes[i].esBot || !participantes[i].conectado)
        {
            jugador.retrasoBot -= deltaTime;

            if (!jugador.botRespondio && jugador.retrasoBot <= 0.0f)
            {
                int margen = (int)std::ceil((float)cantidadDrones * 0.3f);
                if (margen < 2) margen = 2;

                // 10% exacto; el resto falla por 1..margen (nunca 0).
                int error = 0;
                if (GetRandomValue(0, 99) >= 10)
                {
                    error = GetRandomValue(1, margen);
                    if (GetRandomValue(0, 1) == 0) error = -error;
                }

                jugador.respuesta = cantidadDrones + error;
                jugador.respuesta = Clamp(jugador.respuesta, 0, 30);
                jugador.botRespondio = true;
            }

            continue;
        }

        InputMinijuegoParticipante entrada =
            LeerInputMinijuegoParticipante(participantes[i]);

        if (entrada.golpear)
        {
            jugador.respuesta++;
            ReproducirSonidoMinijuego(audio, SONIDO_BOTON);
        }

        if (entrada.saltar)
        {
            jugador.respuesta--;
            ReproducirSonidoMinijuego(audio, SONIDO_BOTON);
        }

        // Direcciones: derecha/izquierda +-1 (mantener acelera), arriba/abajo +-5.
        int direccion = 0;
        if (entrada.derecha) direccion = 1;
        else if (entrada.izquierda) direccion = -1;
        else if (entrada.adelante) direccion = 5;
        else if (entrada.atras) direccion = -5;

        if (direccion == 0)
        {
            jugador.direccionMantenida = 0;
            jugador.tiempoMantenido = 0.0f;
        }
        else
        {
            if (direccion != jugador.direccionMantenida)
            {
                jugador.direccionMantenida = direccion;
                jugador.tiempoMantenido = 0.0f;
                jugador.proximoPaso = 0.0f;
            }

            jugador.tiempoMantenido += deltaTime;
            jugador.proximoPaso -= deltaTime;

            if (jugador.proximoPaso <= 0.0f)
            {
                jugador.respuesta += direccion;
                ReproducirSonidoMinijuego(audio, SONIDO_BOTON);

                float espera = 0.30f - jugador.tiempoMantenido * 0.12f;
                if (espera < 0.07f) espera = 0.07f;
                jugador.proximoPaso = jugador.tiempoMantenido <= 0.0f ? 0.35f : espera;
            }
        }

        jugador.respuesta = Clamp(jugador.respuesta, 0, 30);
    }

    if (tiempoFase <= 0.0f)
    {
        FinalizarConteo(*this);
    }
}


//==================================================
// ESCENARIO: hangar de pruebas de drones (solo visual)
//==================================================
//
// MODELO FUTURO: reemplazar por GLB el hangar, los pilares con
// reflectores, la torre de control, el skyline y la jaula de pruebas.
//==================================================
static void DibujarHangarConteo(float tiempo)
{
    // Suelo del hangar y plataforma de pruebas.
    DrawCube({ 0.0f, -0.3f, 0.0f }, 40.0f, 0.4f, 26.0f, Color{ 22, 28, 44, 255 });
    DrawCube({ 0.0f, -0.05f, 0.0f }, 13.0f, 0.1f, 8.5f, Color{ 31, 47, 72, 255 });

    for (int g = -6; g <= 6; g++)
    {
        DrawLine3D({ (float)g, 0.02f, -4.2f }, { (float)g, 0.02f, 4.2f }, Fade(SKYBLUE, 0.14f));
    }

    for (int g = -4; g <= 4; g++)
    {
        DrawLine3D({ -6.5f, 0.02f, (float)g }, { 6.5f, 0.02f, (float)g }, Fade(SKYBLUE, 0.14f));
    }

    // Muro trasero con skyline nocturno y ventanas encendidas.
    DrawCube({ 0.0f, 5.0f, -9.0f }, 40.0f, 10.0f, 0.6f, Color{ 14, 18, 34, 255 });

    for (int e = 0; e < 14; e++)
    {
        float alto = 2.5f + std::fmod(e * 1.7f, 4.0f);
        float x = -17.0f + e * 2.6f;
        DrawCube({ x, alto * 0.5f, -8.5f }, 2.0f, alto, 0.5f, Color{ 24, 30, 54, 255 });

        for (int v = 0; v < 3; v++)
        {
            if ((e * 3 + v) % 2 == 0)
            {
                DrawCube({ x, 1.0f + v * 1.1f, -8.2f }, 0.5f, 0.3f, 0.05f, Fade(GOLD, 0.7f));
            }
        }
    }

    // Pilares laterales con reflectores y haz de luz.
    for (int lado = -1; lado <= 1; lado += 2)
    {
        for (int k = 0; k < 2; k++)
        {
            float x = lado * 8.2f;
            float z = -3.0f + k * 6.0f;
            DrawCube({ x, 3.0f, z }, 0.7f, 6.0f, 0.7f, Color{ 52, 60, 84, 255 });
            DrawSphereEx({ x, 6.1f, z }, 0.3f, 8, 8, Fade(SKYBLUE, 0.7f + 0.3f * std::sin(tiempo * 3.0f + k)));
            DrawCylinderEx({ x, 6.0f, z }, { x * 0.35f, 0.0f, 0.0f }, 0.04f, 0.5f, 8, Fade(SKYBLUE, 0.05f));
        }
    }

    // Torre de control a un lado.
    DrawCube({ 12.0f, 2.5f, -5.0f }, 2.0f, 5.0f, 2.0f, Color{ 38, 46, 70, 255 });
    DrawCube({ 12.0f, 5.4f, -5.0f }, 3.0f, 0.9f, 3.0f, Color{ 60, 90, 130, 255 });
    DrawSphereEx({ 12.0f, 6.2f, -5.0f }, 0.25f, 6, 6, Fade(RED, 0.5f + 0.5f * std::sin(tiempo * 5.0f)));
}


void MinijuegoConteoExplosivo::Dibujar(
    const Participante participantes[]
) const
{
    ClearBackground(Color{ 17, 24, 43, 255 });

    BeginMode3D(camara);
    DibujarHangarConteo(tiempoAnimacion);


    DrawCubeWires(
        { 0.0f, 2.0f, 0.0f },
        12.5f,
        4.0f,
        8.0f,
        Fade(SKYBLUE, 0.38f)
    );

    if (
        fase == FASE_CONTEO_PREPARACION ||
        fase == FASE_CONTEO_OBSERVACION ||
        fase == FASE_CONTEO_TERMINADO
    )
    {
        for (int i = 0; i < cantidadDrones; i++)
        {
            DibujarDronConteo(drones[i], tiempoAnimacion);
        }
    }
    else
    {
        // Los drones se ocultan, pero el hangar sigue visible (atenuado).
        DrawCube(
            { 0.0f, 2.2f, 0.0f },
            12.7f,
            4.5f,
            8.2f,
            Fade(Color{ 11, 16, 29, 255 }, 0.30f)
        );

        DrawCubeWires(
            { 0.0f, 2.2f, 0.0f },
            12.7f,
            4.5f,
            8.2f,
            MAGENTA
        );
    }

    EndMode3D();

    DrawRectangle(18, 16, 520, 112, Fade(BLACK, 0.76f));
    DrawText("CONTEO EXPLOSIVO", 32, 28, 30, GOLD);
    DrawText(TextoFaseConteo(fase), 32, 66, 22, RAYWHITE);

    if (fase != FASE_CONTEO_TERMINADO)
    {
        DrawText(
            TextFormat("TIEMPO: %.1f", tiempoFase > 0.0f ? tiempoFase : 0.0f),
            32,
            96,
            18,
            LIGHTGRAY
        );
    }

    if (
        fase == FASE_CONTEO_RESPUESTA ||
        fase == FASE_CONTEO_TERMINADO
    )
    {
        int anchoPanel = 210;
        int separacion = 14;
        int cantidadActivos = resultado.cantidadParticipantes;
        int anchoTotal = cantidadActivos * anchoPanel +
            (cantidadActivos - 1) * separacion;
        int x = (GetScreenWidth() - anchoTotal) / 2;
        int orden = 0;

        for (int i = 0; i < MAX_PARTICIPANTES; i++)
        {
            if (!resultado.participantes[i].participo)
            {
                continue;
            }

            int panelX = x + orden * (anchoPanel + separacion);
            Color color = participantes[i].color;
            bool esHumano = !(participantes[i].esBot || !participantes[i].conectado);

            DrawRectangle(
                panelX,
                GetScreenHeight() - 190,
                anchoPanel,
                116,
                Fade(esHumano ? Color{ 40, 40, 20, 255 } : BLACK, 0.82f)
            );

            DrawRectangleLinesEx(
                {
                    (float)panelX,
                    (float)GetScreenHeight() - 190.0f,
                    (float)anchoPanel,
                    116.0f
                },
                esHumano ? 5.0f : 2.0f,
                color
            );

            if (esHumano && fase == FASE_CONTEO_RESPUESTA)
            {
                DrawText("TU RESPUESTA", panelX + 14, GetScreenHeight() - 112, 16, GOLD);
            }

            DrawText(
                TextFormat("J%d", participantes[i].numeroJugador),
                panelX + 14,
                GetScreenHeight() - 176,
                20,
                color
            );

            DrawText(
                TextFormat("%02d", jugadores[i].respuesta),
                panelX + 76,
                GetScreenHeight() - 166,
                42,
                RAYWHITE
            );

            if (fase == FASE_CONTEO_TERMINADO)
            {
                int signado = jugadores[i].respuesta - cantidadDrones;
                const char* error = signado == 0
                    ? "EXACTO"
                    : TextFormat("%+d", signado);
                DrawText(
                    error,
                    panelX + anchoPanel - MeasureText(error, 30) - 14,
                    GetScreenHeight() - 168,
                    30,
                    signado == 0 ? LIME : ORANGE
                );

                const char* texto =
                    resultado.participantes[i].posicionFinal == 1
                    ? "GANADOR"
                    : TextFormat("PUESTO %d", resultado.participantes[i].posicionFinal);

                DrawText(
                    texto,
                    panelX + 14,
                    GetScreenHeight() - 112,
                    18,
                    resultado.participantes[i].posicionFinal == 1
                        ? GOLD
                        : LIGHTGRAY
                );
            }

            orden++;
        }
    }

    if (fase == FASE_CONTEO_RESPUESTA)
    {
        DrawText(
            "GOLPEAR +1  SALTAR -1  |  DERECHA/IZQ: MANTEN PARA ACELERAR  |  ARRIBA/ABAJO: +-5",
            GetScreenWidth() / 2 - 380,
            148,
            19,
            RAYWHITE
        );
    }
    else if (fase == FASE_CONTEO_TERMINADO)
    {
        DrawText(
            TextFormat("HABIA %d DRONES", cantidadDrones),
            GetScreenWidth() / 2 - 128,
            148,
            26,
            GOLD
        );
    }
}


const ResultadoMinijuego&
MinijuegoConteoExplosivo::ObtenerResultado() const
{
    return resultado;
}
