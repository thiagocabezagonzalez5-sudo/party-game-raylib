#include "Minigames/MinijuegoInterruptoresCaos.h"

#include "Minigames/AudioMinijuegos.h"
#include "Minigames/UtilidadesMinijuegos.h"
#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include "Systems/Input.h"

#include <cmath>


static const float DURACION_PREPARACION_INTERRUPTORES = 2.5f;
static const float DURACION_TURNO_INTERRUPTORES = 10.0f;
static const float DURACION_SEGURO_INTERRUPTORES = 0.8f;
static const float DURACION_EXPLOSION_INTERRUPTORES = 1.45f;


static Color ColorInterruptorCaos(int indice)
{
    const Color colores[MAX_INTERRUPTORES_CAOS] =
    {
        Color{ 238, 72, 82, 255 },
        Color{ 76, 161, 238, 255 },
        Color{ 246, 197, 68, 255 },
        Color{ 85, 203, 116, 255 },
        Color{ 177, 95, 226, 255 }
    };

    if (indice < 0 || indice >= MAX_INTERRUPTORES_CAOS)
    {
        return GRAY;
    }

    return colores[indice];
}


static int ContarVivosInterruptores(
    const MinijuegoInterruptoresCaos& minijuego
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


static int BuscarSiguienteVivoInterruptores(
    const MinijuegoInterruptoresCaos& minijuego,
    int origen
)
{
    int indice = origen;

    for (int intento = 0; intento < MAX_PARTICIPANTES; intento++)
    {
        indice++;
        if (indice >= MAX_PARTICIPANTES) indice = 0;

        if (
            minijuego.resultado.participantes[indice].participo &&
            !minijuego.jugadores[indice].eliminado
        )
        {
            return indice;
        }
    }

    return origen;
}


static int ElegirInterruptorDisponible(
    const MinijuegoInterruptoresCaos& minijuego
)
{
    int disponibles[MAX_INTERRUPTORES_CAOS]{};
    int cantidad = 0;

    for (int i = 0; i < minijuego.cantidadInterruptores; i++)
    {
        if (!minijuego.interruptoresUsados[i])
        {
            disponibles[cantidad] = i;
            cantidad++;
        }
    }

    if (cantidad <= 0)
    {
        return 0;
    }

    return disponibles[GetRandomValue(0, cantidad - 1)];
}


static int MoverSeleccionInterruptor(
    const MinijuegoInterruptoresCaos& minijuego,
    int origen,
    int direccion
)
{
    int indice = origen;

    for (int intento = 0; intento < minijuego.cantidadInterruptores; intento++)
    {
        indice += direccion;

        if (indice < 0) indice = minijuego.cantidadInterruptores - 1;
        if (indice >= minijuego.cantidadInterruptores) indice = 0;

        if (!minijuego.interruptoresUsados[indice])
        {
            return indice;
        }
    }

    return origen;
}


static void PrepararRondaInterruptores(
    MinijuegoInterruptoresCaos& minijuego
)
{
    int eliminados =
        minijuego.resultado.cantidadParticipantes -
        ContarVivosInterruptores(minijuego);

    minijuego.cantidadInterruptores = MAX_INTERRUPTORES_CAOS - eliminados;
    if (minijuego.cantidadInterruptores < 3)
    {
        minijuego.cantidadInterruptores = 3;
    }

    for (int i = 0; i < MAX_INTERRUPTORES_CAOS; i++)
    {
        minijuego.interruptoresUsados[i] = false;
    }

    minijuego.interruptorPeligroso = GetRandomValue(
        0,
        minijuego.cantidadInterruptores - 1
    );
}


static void PrepararTurnoInterruptores(
    MinijuegoInterruptoresCaos& minijuego,
    int jugador
)
{
    minijuego.jugadorTurno = jugador;
    minijuego.jugadorExplosion = -1;
    minijuego.numeroTurno++;
    minijuego.tiempoTurno = DURACION_TURNO_INTERRUPTORES;
    minijuego.interruptorSeleccionado =
        ElegirInterruptorDisponible(minijuego);
    minijuego.fase = FASE_INTERRUPTORES_ELECCION;

    if (jugador >= 0)
    {
        minijuego.jugadores[jugador].tiempoDecisionBot =
            GetRandomValue(75, 190) / 100.0f;
    }
}


static void FinalizarInterruptores(
    MinijuegoInterruptoresCaos& minijuego
)
{
    int ganador = -1;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (
            minijuego.resultado.participantes[i].participo &&
            !minijuego.jugadores[i].eliminado
        )
        {
            ganador = i;
            minijuego.jugadores[i].posicionFinal = 1;
            break;
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

        resultadoJugador.posicionFinal =
            minijuego.jugadores[i].posicionFinal;

        resultadoJugador.puntuacionMinijuego =
            (minijuego.resultado.cantidadParticipantes + 1 -
                resultadoJugador.posicionFinal) * 50 +
            minijuego.jugadores[i].turnosSuperados * 10;
    }

    minijuego.resultado.estado = RESULTADO_MINIJUEGO_FINALIZADO;
    minijuego.resultado.desenlace =
        ganador >= 0 ? DESENLACE_CON_GANADOR : DESENLACE_EMPATE;
    minijuego.fase = FASE_INTERRUPTORES_TERMINADO;

    ReproducirSonidoMinijuego(minijuego.audio, SONIDO_RESULTADO);
    minijuego.jugadorTurno = ganador;
}


static void ActivarInterruptor(
    MinijuegoInterruptoresCaos& minijuego
)
{
    int indice = minijuego.interruptorSeleccionado;

    if (
        indice < 0 ||
        indice >= minijuego.cantidadInterruptores ||
        minijuego.interruptoresUsados[indice]
    )
    {
        return;
    }

    if (indice == minijuego.interruptorPeligroso)
    {
        int vivosAntes = ContarVivosInterruptores(minijuego);
        int eliminado = minijuego.jugadorTurno;

        minijuego.jugadores[eliminado].eliminado = true;
        minijuego.jugadores[eliminado].posicionFinal = vivosAntes;
        minijuego.jugadorExplosion = eliminado;
        minijuego.jugadorTurno = -1;
        minijuego.fase = FASE_INTERRUPTORES_EXPLOSION;
        ReproducirSonidoMinijuego(minijuego.audio, SONIDO_EXPLOSION);
        ReproducirSonidoMinijuego(minijuego.audio, SONIDO_ELIMINADO);
        minijuego.tiempoResolucion = DURACION_EXPLOSION_INTERRUPTORES;
        return;
    }

    minijuego.interruptoresUsados[indice] = true;
    minijuego.jugadores[minijuego.jugadorTurno].turnosSuperados++;
    minijuego.fase = FASE_INTERRUPTORES_SEGURO;
    ReproducirSonidoMinijuego(minijuego.audio, SONIDO_ACIERTO);
    minijuego.tiempoResolucion = DURACION_SEGURO_INTERRUPTORES;
}


void MinijuegoInterruptoresCaos::Inicializar()
{
    resultado = {};
    resultado.formato = FORMATO_MINIJUEGO_INDIVIDUAL;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        jugadores[i] = {};
    }

    for (int i = 0; i < MAX_INTERRUPTORES_CAOS; i++)
    {
        interruptoresUsados[i] = false;
    }

    cantidadInterruptores = MAX_INTERRUPTORES_CAOS;
    interruptorPeligroso = 0;
    interruptorSeleccionado = 0;
    jugadorTurno = -1;
    jugadorExplosion = -1;
    numeroTurno = 0;
    fase = FASE_INTERRUPTORES_PREPARACION;
    tiempoPreparacion = DURACION_PREPARACION_INTERRUPTORES;
    tiempoTurno = DURACION_TURNO_INTERRUPTORES;
    tiempoResolucion = 0.0f;
    tiempoAnimacion = 0.0f;
}


void MinijuegoInterruptoresCaos::Reiniciar(
    Participante participantes[]
)
{
    Inicializar();
    InicializarResultadoMinijuego(
        resultado,
        participantes,
        FORMATO_MINIJUEGO_INDIVIDUAL
    );

    PrepararRondaInterruptores(*this);

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (resultado.participantes[i].participo)
        {
            jugadorTurno = i;
            break;
        }
    }
}


void MinijuegoInterruptoresCaos::Actualizar(
    float deltaTime,
    Participante participantes[]
)
{
    tiempoAnimacion += deltaTime;

    if (fase == FASE_INTERRUPTORES_TERMINADO)
    {
        return;
    }

    if (fase == FASE_INTERRUPTORES_PREPARACION)
    {
        float preparacionAntes = tiempoPreparacion;
        tiempoPreparacion -= deltaTime;
        ActualizarAudioCuentaRegresiva(audio, preparacionAntes, tiempoPreparacion);

        if (tiempoPreparacion <= 0.0f)
        {
            PrepararTurnoInterruptores(*this, jugadorTurno);
        }

        return;
    }

    if (fase == FASE_INTERRUPTORES_SEGURO)
    {
        tiempoResolucion -= deltaTime;

        if (tiempoResolucion <= 0.0f)
        {
            PrepararTurnoInterruptores(
                *this,
                BuscarSiguienteVivoInterruptores(*this, jugadorTurno)
            );
        }

        return;
    }

    if (fase == FASE_INTERRUPTORES_EXPLOSION)
    {
        tiempoResolucion -= deltaTime;

        if (tiempoResolucion <= 0.0f)
        {
            if (ContarVivosInterruptores(*this) <= 1)
            {
                FinalizarInterruptores(*this);
            }
            else
            {
                int siguiente = BuscarSiguienteVivoInterruptores(
                    *this,
                    jugadorExplosion
                );
                PrepararRondaInterruptores(*this);
                PrepararTurnoInterruptores(*this, siguiente);
            }
        }

        return;
    }

    float turnoAntes = tiempoTurno;
    tiempoTurno -= deltaTime;
    ActualizarAudioAlertaTiempo(audio, turnoAntes, tiempoTurno, 3.0f);

    if (jugadorTurno < 0)
    {
        return;
    }

    bool confirmar = tiempoTurno <= 0.0f;

    if (participantes[jugadorTurno].esBot || !participantes[jugadorTurno].conectado)
    {
        jugadores[jugadorTurno].tiempoDecisionBot -= deltaTime;

        if (jugadores[jugadorTurno].tiempoDecisionBot <= 0.0f)
        {
            interruptorSeleccionado = ElegirInterruptorDisponible(*this);
            confirmar = true;
        }
    }
    else
    {
        int seleccionAntes = interruptorSeleccionado;

        if (AccionDireccionalControlPresionada(
            participantes[jugadorTurno],
            CONTROL_DIRECCION_IZQUIERDA
        ))
        {
            interruptorSeleccionado = MoverSeleccionInterruptor(
                *this,
                interruptorSeleccionado,
                -1
            );
        }

        if (AccionDireccionalControlPresionada(
            participantes[jugadorTurno],
            CONTROL_DIRECCION_DERECHA
        ))
        {
            interruptorSeleccionado = MoverSeleccionInterruptor(
                *this,
                interruptorSeleccionado,
                1
            );
        }

        if (seleccionAntes != interruptorSeleccionado)
        {
            ReproducirSonidoMinijuego(audio, SONIDO_BOTON);
        }

        InputMinijuegoParticipante entrada =
            LeerInputMinijuegoParticipante(participantes[jugadorTurno]);

        confirmar = confirmar || entrada.golpear;
    }

    if (confirmar)
    {
        ActivarInterruptor(*this);
    }
}


//==================================================
// VISUAL 3D (independiente de la logica)
//==================================================
//
// MODELO FUTURO: reemplazar por GLB la consola con sus palancas, el
// reactor de cristal con su nucleo, los pilones Tesla, las pasarelas
// de acero y las tuberias / cables de la central electrica.
//==================================================


static const float Z_PASARELA_INTERRUPTORES = 4.6f;
static const float ALTURA_PASARELA_INTERRUPTORES = 0.6f;
static const float Z_PALANCAS_INTERRUPTORES = -3.2f;
static const float ALTURA_BASE_PALANCA = 1.45f;
static const Vector3 CORONA_REACTOR_INTERRUPTORES = { 0.0f, 8.4f, -9.0f };


static Camera3D ObtenerCamaraInterruptores(float temblor)
{
    Camera3D camara{};
    camara.position = { temblor, 10.0f, 14.5f };
    camara.target = { 0.0f, 2.6f, -2.0f };
    camara.up = { 0.0f, 1.0f, 0.0f };
    camara.fovy = 48.0f;
    camara.projection = CAMERA_PERSPECTIVE;
    return camara;
}


// Posicion de la pasarela del participante (segun su orden entre los activos).
static Vector3 PosicionPasarelaInterruptores(
    const MinijuegoInterruptoresCaos& minijuego,
    int indice
)
{
    int orden = 0;
    int cantidad = 0;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (minijuego.resultado.participantes[i].participo)
        {
            if (i < indice) orden++;
            cantidad++;
        }
    }

    if (cantidad < 1) cantidad = 1;

    return Vector3
    {
        (orden - (cantidad - 1) * 0.5f) * 4.2f,
        ALTURA_PASARELA_INTERRUPTORES,
        Z_PASARELA_INTERRUPTORES
    };
}


static float PosicionXPalanca(const MinijuegoInterruptoresCaos& minijuego, int indice)
{
    return (indice - (minijuego.cantidadInterruptores - 1) * 0.5f) * 3.1f;
}


// Rayo con zigzag entre dos puntos (cambia rapido con el tiempo).
static void DibujarRayoInterruptores(Vector3 origen, Vector3 destino, float tiempo, float semilla, Color color)
{
    Vector3 anterior = origen;

    for (int tramo = 1; tramo <= 8; tramo++)
    {
        float f = tramo / 8.0f;
        float ruido = tramo == 8 ? 0.0f : 0.55f;
        Vector3 siguiente =
        {
            origen.x + (destino.x - origen.x) * f + std::sin(tiempo * 53.0f + semilla + tramo * 2.3f) * ruido,
            origen.y + (destino.y - origen.y) * f + std::sin(tiempo * 47.0f + semilla * 1.7f + tramo * 1.1f) * ruido,
            origen.z + (destino.z - origen.z) * f + std::sin(tiempo * 61.0f + semilla * 0.6f + tramo * 3.7f) * ruido
        };

        DrawLine3D(anterior, siguiente, color);
        DrawLine3D(
            { anterior.x + 0.04f, anterior.y, anterior.z },
            { siguiente.x + 0.04f, siguiente.y, siguiente.z },
            Fade(color, 0.6f)
        );
        anterior = siguiente;
    }
}


static void DibujarCentralInterruptores(float tiempo, float tension, bool sobrecarga)
{
    // Suelo industrial y muro trasero.
    DrawCube({ 0.0f, -0.2f, 0.0f }, 52.0f, 0.3f, 38.0f, Color{ 24, 22, 32, 255 });
    DrawCube({ 0.0f, 7.0f, -12.0f }, 52.0f, 16.0f, 0.8f, Color{ 30, 26, 42, 255 });

    for (int x = -24; x <= 24; x += 4)
    {
        DrawLine3D({ (float)x, 0.0f, -11.0f }, { (float)x, 0.0f, 12.0f }, Fade(Color{ 120, 90, 200, 255 }, 0.25f));
    }

    // Reactor de cristal con nucleo: se calienta con cada palanca segura.
    float latido = std::sin(tiempo * (3.0f + tension * 9.0f));
    Color nucleo = sobrecarga
        ? Color{ 255, 60, 50, 255 }
        : ColorLerp(Color{ 106, 232, 245, 255 }, Color{ 255, 190, 70, 255 }, tension);

    DrawCylinder({ 0.0f, 0.0f, -9.0f }, 2.6f, 2.8f, 0.8f, 20, Color{ 60, 56, 74, 255 });
    DrawCylinder({ 0.0f, 0.8f, -9.0f }, 2.0f, 2.0f, 7.0f, 20, Fade(nucleo, 0.18f));
    DrawCylinder({ 0.0f, 7.8f, -9.0f }, 2.4f, 2.4f, 0.6f, 20, Color{ 60, 56, 74, 255 });
    DrawSphereEx({ 0.0f, 4.2f, -9.0f }, 1.1f + latido * 0.12f, 14, 14, Fade(nucleo, 0.8f));
    DrawSphereEx({ 0.0f, 4.2f, -9.0f }, 0.6f, 10, 10, WHITE);

    for (int anillo = 0; anillo < 3; anillo++)
    {
        DrawCircle3D(
            { 0.0f, 2.0f + anillo * 2.2f, -9.0f },
            2.25f,
            { 1.0f, 0.0f, 0.0f },
            90.0f,
            Fade(nucleo, 0.7f)
        );
    }

    // Pilones Tesla laterales.
    for (int lado = -1; lado <= 1; lado += 2)
    {
        float x = lado * 7.5f;
        DrawCylinder({ x, 0.0f, -8.5f }, 0.5f, 0.8f, 6.0f, 8, Color{ 80, 76, 100, 255 });
        DrawSphereEx({ x, 6.4f, -8.5f }, 0.6f + 0.08f * latido, 10, 10, Fade(nucleo, 0.9f));
        DrawLine3D({ x, 6.4f, -8.5f }, { 0.0f, 4.2f, -9.0f }, Fade(nucleo, 0.35f + 0.3f * tension));
    }

    // Tuberias en el muro.
    DrawCylinderEx({ -22.0f, 9.0f, -11.4f }, { 22.0f, 9.0f, -11.4f }, 0.4f, 0.4f, 8, Color{ 110, 84, 60, 255 });
    DrawCylinderEx({ -22.0f, 11.0f, -11.4f }, { 22.0f, 11.0f, -11.4f }, 0.3f, 0.3f, 8, Color{ 90, 100, 130, 255 });

    // Consola de palancas.
    DrawCube({ 0.0f, 0.7f, Z_PALANCAS_INTERRUPTORES - 0.6f }, 17.0f, 1.4f, 2.2f, Color{ 52, 48, 66, 255 });
    DrawCube({ 0.0f, 1.42f, Z_PALANCAS_INTERRUPTORES - 0.6f }, 17.0f, 0.06f, 2.2f, Color{ 96, 92, 112, 255 });
}


static void DibujarPalanca3D(
    float x,
    Color color,
    float anguloGrados,
    bool seleccionada,
    bool usada,
    bool peligrosaRevelada,
    float tiempo
)
{
    // Placa base y lampara superior.
    DrawCube({ x, ALTURA_BASE_PALANCA, Z_PALANCAS_INTERRUPTORES }, 1.5f, 0.12f, 1.3f, usada ? Color{ 40, 38, 48, 255 } : Color{ 92, 88, 104, 255 });
    DrawCube({ x, 3.6f, Z_PALANCAS_INTERRUPTORES - 1.6f }, 1.0f, 1.0f, 0.3f, Color{ 24, 22, 34, 255 });
    DrawSphereEx(
        { x, 3.6f, Z_PALANCAS_INTERRUPTORES - 1.4f },
        0.32f,
        8,
        8,
        usada ? Fade(LIME, 0.8f) : Fade(color, 0.9f)
    );

    if (seleccionada)
    {
        DrawCircle3D(
            { x, ALTURA_BASE_PALANCA + 0.1f, Z_PALANCAS_INTERRUPTORES },
            0.95f + 0.06f * std::sin(tiempo * 8.0f),
            { 1.0f, 0.0f, 0.0f },
            90.0f,
            RAYWHITE
        );
        DrawCylinder({ x, 4.9f + 0.15f * std::sin(tiempo * 6.0f), Z_PALANCAS_INTERRUPTORES + 0.4f }, 0.4f, 0.0f, 0.8f, 6, GOLD);
    }

    // Mango: gira alrededor del eje X desde la placa; hacia el jugador al jalar.
    rlPushMatrix();
    rlTranslatef(x, ALTURA_BASE_PALANCA + 0.1f, Z_PALANCAS_INTERRUPTORES);
    rlRotatef(anguloGrados, 1.0f, 0.0f, 0.0f);
    DrawCylinder({ 0.0f, 0.0f, 0.0f }, 0.1f, 0.14f, 2.0f, 8, Color{ 170, 170, 182, 255 });

    Color perilla = usada ? Fade(color, 0.35f) : color;

    if (peligrosaRevelada)
    {
        perilla = ColorLerp(RED, WHITE, 0.5f + 0.5f * std::sin(tiempo * 30.0f));
    }

    DrawSphereEx({ 0.0f, 2.15f, 0.0f }, 0.34f, 10, 10, perilla);
    rlPopMatrix();
}


void MinijuegoInterruptoresCaos::Dibujar(
    const Participante participantes[]
) const
{
    ClearBackground(Color{ 14, 10, 22, 255 });

    int ancho = GetScreenWidth();
    int alto = GetScreenHeight();

    int usados = 0;

    for (int i = 0; i < cantidadInterruptores; i++)
    {
        if (interruptoresUsados[i]) usados++;
    }

    float tension = cantidadInterruptores > 1
        ? Clamp((float)usados / (cantidadInterruptores - 1), 0.0f, 1.0f)
        : 1.0f;
    bool sobrecarga = fase == FASE_INTERRUPTORES_EXPLOSION;
    float progresoExplosion = sobrecarga
        ? 1.0f - Clamp(tiempoResolucion / DURACION_EXPLOSION_INTERRUPTORES, 0.0f, 1.0f)
        : 0.0f;
    float temblor = sobrecarga
        ? std::sin(tiempoAnimacion * 70.0f) * 0.3f * (1.0f - progresoExplosion)
        : 0.0f;

    Camera3D camara = ObtenerCamaraInterruptores(temblor);
    BeginMode3D(camara);

    DibujarCentralInterruptores(tiempoAnimacion, tension, sobrecarga);

    // Palancas: la actual se anima; las usadas quedan jaladas.
    for (int i = 0; i < cantidadInterruptores; i++)
    {
        bool actual =
            i == interruptorSeleccionado &&
            (fase == FASE_INTERRUPTORES_SEGURO || fase == FASE_INTERRUPTORES_EXPLOSION);
        float angulo = 0.0f;

        if (actual)
        {
            float duracion = fase == FASE_INTERRUPTORES_SEGURO
                ? DURACION_SEGURO_INTERRUPTORES
                : DURACION_EXPLOSION_INTERRUPTORES;
            float progreso = 1.0f - Clamp(tiempoResolucion / duracion, 0.0f, 1.0f);
            angulo = 55.0f * Clamp(progreso * 4.0f, 0.0f, 1.0f);
        }
        else if (interruptoresUsados[i])
        {
            angulo = 55.0f;
        }
        else if (fase == FASE_INTERRUPTORES_ELECCION && i == interruptorSeleccionado)
        {
            angulo = std::sin(tiempoAnimacion * 10.0f) * 3.0f;
        }

        DibujarPalanca3D(
            PosicionXPalanca(*this, i),
            ColorInterruptorCaos(i),
            angulo,
            fase == FASE_INTERRUPTORES_ELECCION && i == interruptorSeleccionado,
            interruptoresUsados[i],
            actual && fase == FASE_INTERRUPTORES_EXPLOSION,
            tiempoAnimacion
        );
    }

    // Pasarelas con los jugadores.
    Vector2 etiquetas[MAX_PARTICIPANTES]{};

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!resultado.participantes[i].participo)
        {
            continue;
        }

        Vector3 base = PosicionPasarelaInterruptores(*this, i);
        bool eliminado = jugadores[i].eliminado;
        bool enTurno = fase == FASE_INTERRUPTORES_ELECCION && i == jugadorTurno;

        DrawCube(
            { base.x, base.y - 0.15f, base.z },
            3.2f,
            0.3f,
            2.4f,
            eliminado ? Color{ 28, 22, 24, 255 } : Color{ 62, 60, 74, 255 }
        );

        // Franja de precaucion en el borde frontal.
        for (int franja = 0; franja < 8; franja++)
        {
            DrawCube(
                { base.x - 1.4f + franja * 0.4f, base.y + 0.02f, base.z + 1.15f },
                0.4f,
                0.04f,
                0.1f,
                franja % 2 == 0 ? Color{ 240, 200, 40, 255 } : Color{ 20, 20, 24, 255 }
            );
        }

        DrawCircle3D(
            { base.x, base.y + 0.03f, base.z },
            1.0f,
            { 1.0f, 0.0f, 0.0f },
            90.0f,
            eliminado ? Fade(RED, 0.5f) : (enTurno ? WHITE : participantes[i].color)
        );

        JugadorPrueba figura{};
        figura.posicion = { base.x, base.y + figura.tamano.y * 0.5f, base.z };
        figura.direccionMirada = { 0.0f, 0.0f, -1.0f };
        figura.enSuelo = true;
        figura.aplastado = eliminado;

        Participante visual = participantes[i];
        visual.activo = true;
        visual.conectado = true;
        DibujarJugadorCuboPrueba(figura, visual);

        if (enTurno)
        {
            DrawCylinder(
                { base.x, base.y + 2.5f + 0.15f * std::sin(tiempoAnimacion * 6.0f), base.z },
                0.35f,
                0.0f,
                0.6f,
                6,
                GOLD
            );

            // Hilo de luz entre el jugador y la palanca elegida.
            DrawLine3D(
                { base.x, base.y + 1.0f, base.z },
                { PosicionXPalanca(*this, interruptorSeleccionado), ALTURA_BASE_PALANCA + 1.0f, Z_PALANCAS_INTERRUPTORES },
                Fade(ColorInterruptorCaos(interruptorSeleccionado), 0.8f)
            );
        }

        etiquetas[i] = GetWorldToScreen({ base.x, base.y + 2.2f, base.z }, camara);
    }

    // Descarga electrica sobre el jugador que activo la sobrecarga.
    if (sobrecarga && jugadorExplosion >= 0)
    {
        Vector3 victima = PosicionPasarelaInterruptores(*this, jugadorExplosion);
        victima.y += 1.0f;
        float intensidad = 1.0f - progresoExplosion;

        for (int rayo = 0; rayo < 4; rayo++)
        {
            DibujarRayoInterruptores(
                CORONA_REACTOR_INTERRUPTORES,
                victima,
                tiempoAnimacion,
                rayo * 5.0f,
                Fade(rayo % 2 == 0 ? WHITE : Color{ 120, 230, 255, 255 }, 0.5f + 0.5f * intensidad)
            );
        }

        DrawSphereEx(victima, 0.6f + progresoExplosion * 2.5f, 12, 12, Fade(ORANGE, 0.5f * intensidad));
        DrawCircle3D(
            { victima.x, ALTURA_PASARELA_INTERRUPTORES + 0.05f, victima.z },
            1.0f + progresoExplosion * 5.0f,
            { 1.0f, 0.0f, 0.0f },
            90.0f,
            Fade(GOLD, intensidad)
        );
    }

    EndMode3D();

    if (sobrecarga && progresoExplosion < 0.25f)
    {
        DrawRectangle(0, 0, ancho, alto, Fade(WHITE, 0.45f * (1.0f - progresoExplosion / 0.25f)));
    }

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!resultado.participantes[i].participo)
        {
            continue;
        }

        const char* texto = jugadores[i].eliminado
            ? TextFormat("J%d  X", participantes[i].numeroJugador)
            : TextFormat("J%d", participantes[i].numeroJugador);
        DrawText(
            texto,
            (int)etiquetas[i].x - MeasureText(texto, 22) / 2,
            (int)etiquetas[i].y,
            22,
            jugadores[i].eliminado ? RED : participantes[i].color
        );
    }

    DrawRectangle(18, 16, 560, 116, Fade(BLACK, 0.80f));
    DrawText("INTERRUPTORES DEL CAOS", 32, 28, 30, GOLD);

    if (fase == FASE_INTERRUPTORES_PREPARACION)
    {
        DrawText(TextFormat("PREPARATE  %.1f", tiempoPreparacion), 32, 77, 23, RAYWHITE);

        const char* numero = TextFormat("%d", (int)std::ceil(tiempoPreparacion));
        DrawText(numero, ancho / 2 - MeasureText(numero, 120) / 2, alto / 2 - 60, 120, Fade(GOLD, 0.9f));
    }
    else if (fase == FASE_INTERRUPTORES_ELECCION)
    {
        DrawText(
            TextFormat(
                "TURNO J%d  |  TIEMPO %.1f",
                participantes[jugadorTurno].numeroJugador,
                tiempoTurno
            ),
            32,
            70,
            22,
            participantes[jugadorTurno].color
        );
        DrawText("IZQ/DER ELEGIR  |  GOLPEAR ACTIVAR", 32, 101, 17, LIGHTGRAY);
    }
    else if (fase == FASE_INTERRUPTORES_SEGURO)
    {
        DrawText("SEGURO! PASA EL TURNO", 32, 76, 22, LIME);
    }
    else if (fase == FASE_INTERRUPTORES_EXPLOSION)
    {
        DrawText(
            TextFormat("J%d ACTIVO LA SOBRECARGA", participantes[jugadorExplosion].numeroJugador),
            32,
            76,
            22,
            RED
        );
    }
    else
    {
        DrawText(TextFormat("ULTIMO EN PIE  %s", TextoReinicioMinijuego()), 32, 76, 20, RAYWHITE);

        if (jugadorTurno >= 0)
        {
            const char* titulo = TextFormat("GANADOR: J%d", participantes[jugadorTurno].numeroJugador);
            DrawRectangle(ancho / 2 - 240, alto / 2 - 50, 480, 90, Fade(BLACK, 0.75f));
            DrawText(titulo, ancho / 2 - MeasureText(titulo, 44) / 2, alto / 2 - 30, 44, participantes[jugadorTurno].color);
        }
    }

    int tarjetaX = ancho - 236;
    int tarjetaY = 22;
    DrawRectangle(tarjetaX - 12, tarjetaY - 8, 222, 40 + resultado.cantidadParticipantes * 42, Fade(BLACK, 0.76f));
    DrawText("ORDEN", tarjetaX, tarjetaY, 18, LIGHTGRAY);

    int fila = 0;
    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!resultado.participantes[i].participo) continue;

        int y = tarjetaY + 31 + fila * 42;
        Color color = jugadores[i].eliminado ? Fade(participantes[i].color, 0.28f) : participantes[i].color;
        DrawCircle(tarjetaX + 14, y + 11, 12.0f, color);
        DrawText(TextFormat("J%d", participantes[i].numeroJugador), tarjetaX + 35, y, 21, color);

        if (jugadores[i].eliminado)
        {
            DrawText(TextFormat("#%d", jugadores[i].posicionFinal), tarjetaX + 122, y + 2, 18, RED);
        }
        else if (i == jugadorTurno)
        {
            DrawText("TURNO", tarjetaX + 112, y + 2, 17, GOLD);
        }
        else
        {
            DrawText("ESPERA", tarjetaX + 106, y + 2, 16, LIGHTGRAY);
        }

        fila++;
    }
}


const ResultadoMinijuego&
MinijuegoInterruptoresCaos::ObtenerResultado() const
{
    return resultado;
}
