#include "Minigames/MinijuegoSecuenciaNeon.h"

#include "Minigames/AudioMinijuegos.h"
#include "Minigames/HudMinijuego.h"
#include "Minigames/UtilidadesMinijuegos.h"
#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include "Systems/CalidadGrafica.h"
#include "Systems/Input.h"
#include "UI/EstiloUI.h"

#include <cmath>
#include <cstdio>


//==================================================
// SECUENCIA NEON - SALA DE SINTETIZADOR 3D
//==================================================
//
// Logica: la secuencia crece cada ronda; los jugadores la repiten con
// las cuatro direcciones y quien falla o se queda sin tiempo cae.
// Visual: cuatro paneles gigantes en cruz sobre el escenario (se
// encienden SOLO al mostrar la secuencia) y una consola por jugador.
//
// Regla de no filtracion: mientras se responde, nada de lo que se ve o
// se oye depende de la direccion pulsada. El panel grande no reacciona a
// las respuestas, las consolas solo cuentan avance (segmentos neutros) y
// todas las pulsaciones suenan igual. Fallar se muestra (consola roja),
// pero sin indicar que direccion se toco ni cual era la correcta.
//==================================================


static const float DURACION_PREPARACION_SECUENCIA = 2.5f;
static const float DURACION_PULSO_VISIBLE = 0.50f;
static const float DURACION_PAUSA_PULSO = 0.20f;
static const float DURACION_RESOLUCION_SECUENCIA = 1.25f;
static const int PULSOS_INICIALES_SECUENCIA = 3;
static const int RONDAS_MAXIMAS_SECUENCIA =
    MAX_PULSOS_SECUENCIA_NEON - PULSOS_INICIALES_SECUENCIA + 1;

// Distribucion de la sala (la usan la logica visual y el dibujo).
static const float Z_PANELES_SECUENCIA = -8.4f;
static const float Z_JUGADORES_SECUENCIA = 3.0f;
static const float Z_CONSOLAS_SECUENCIA = 1.45f;
static const float SEPARACION_JUGADORES_SECUENCIA = 4.4f;
static const float LADO_PANEL_SECUENCIA = 2.3f;

// Paleta de la sala: rosa neon y blanco calido. Nunca los colores de las
// direcciones (celeste, naranja, verde, violeta) ni flechas en la decoracion.
static const Color COLOR_ACENTO_SALA_SECUENCIA = { 255, 64, 168, 255 };
static const Color COLOR_BLANCO_SALA_SECUENCIA = { 255, 232, 218, 255 };
static const Color COLOR_ERROR_SECUENCIA = { 255, 72, 58, 255 };
static const Color COLOR_ORO_SECUENCIA = { 255, 226, 96, 255 };


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
        case PULSO_NEON_ARRIBA: return Color{ 86, 200, 255, 255 };
        case PULSO_NEON_DERECHA: return Color{ 255, 150, 46, 255 };
        case PULSO_NEON_ABAJO: return Color{ 122, 236, 84, 255 };
        case PULSO_NEON_IZQUIERDA: return Color{ 168, 102, 255, 255 };
        case CANTIDAD_PULSOS_NEON: break;
    }

    return RAYWHITE;
}


// X de la consola/pedestal del jugador (mismo orden que el dibujo).
static float XJugadorSecuencia(
    const MinijuegoSecuenciaNeon& minijuego,
    int indice
)
{
    int cantidad = minijuego.resultado.cantidadParticipantes > 0
        ? minijuego.resultado.cantidadParticipantes
        : 1;
    int orden = 0;

    for (int j = 0; j < indice && j < MAX_PARTICIPANTES; j++)
    {
        if (minijuego.resultado.participantes[j].participo) orden++;
    }

    return ((float)orden - (float)(cantidad - 1) * 0.5f) * SEPARACION_JUGADORES_SECUENCIA;
}


//==================================================
// EFECTOS (solo visual)
//==================================================

// Azar propio de los efectos: no consume GetRandomValue, asi la secuencia
// y las respuestas de los bots no cambian por lo visual.
static float AzarSecuencia(float minimo, float maximo)
{
    static unsigned int estado = 0x2545F491u;
    estado = estado * 1664525u + 1013904223u;
    float u = (float)((estado >> 8) & 0xFFFFu) / 65535.0f;
    return minimo + (maximo - minimo) * u;
}


static ParticulaSecuenciaNeon* ReservarParticulaSecuencia(MinijuegoSecuenciaNeon& m)
{
    for (int i = 0; i < MAX_PARTICULAS_SECUENCIA_NEON; i++)
    {
        if (!m.particulas[i].activa) return &m.particulas[i];
    }

    return nullptr;
}


static void EmitirChispasSecuencia(
    MinijuegoSecuenciaNeon& m,
    Vector3 posicion,
    Color color,
    Color colorMezcla,
    int cantidad,
    float fuerza
)
{
    int total = (int)std::lround((float)cantidad * FactorCalidadGrafica(CalidadParticulas()));
    if (total < 1) total = 1;

    for (int i = 0; i < total; i++)
    {
        ParticulaSecuenciaNeon* p = ReservarParticulaSecuencia(m);
        if (p == nullptr) return;

        float angulo = AzarSecuencia(0.0f, 6.2832f);
        float velocidad = AzarSecuencia(0.3f, 1.0f) * fuerza;

        *p = {};
        p->activa = true;
        p->gravedad = true;
        p->posicion =
        {
            posicion.x + AzarSecuencia(-0.5f, 0.5f),
            posicion.y,
            posicion.z + AzarSecuencia(-0.2f, 0.2f)
        };
        p->velocidad =
        {
            std::cos(angulo) * velocidad,
            AzarSecuencia(2.0f, 5.0f),
            std::sin(angulo) * velocidad
        };
        p->vidaMaxima = AzarSecuencia(0.45f, 0.9f);
        p->vida = p->vidaMaxima;
        p->tamano = AzarSecuencia(0.06f, 0.12f);
        p->color = ColorLerp(color, colorMezcla, AzarSecuencia(0.0f, 0.6f));
    }
}


static Vector3 PuntoConsolaSecuencia(const MinijuegoSecuenciaNeon& m, int indice)
{
    return { XJugadorSecuencia(m, indice), 1.25f, Z_CONSOLAS_SECUENCIA };
}


// Motas rosas que suben de las bocinas (lejos de los paneles).
static void ActualizarAmbienteSecuencia(MinijuegoSecuenciaNeon& m, float deltaTime)
{
    if (FactorCalidadGrafica(CalidadParticulas()) < 0.5f) return;

    m.temporizadorAmbiente -= deltaTime;
    if (m.temporizadorAmbiente > 0.0f) return;
    m.temporizadorAmbiente = AzarSecuencia(0.12f, 0.26f);

    ParticulaSecuenciaNeon* p = ReservarParticulaSecuencia(m);
    if (p == nullptr) return;

    float lado = AzarSecuencia(0.0f, 1.0f) < 0.5f ? -1.0f : 1.0f;
    *p = {};
    p->activa = true;
    p->gravedad = false;
    p->posicion = { lado * AzarSecuencia(10.0f, 13.0f), AzarSecuencia(0.4f, 2.0f), AzarSecuencia(-7.0f, -4.5f) };
    p->velocidad = { AzarSecuencia(-0.12f, 0.12f), AzarSecuencia(0.5f, 1.0f), AzarSecuencia(-0.05f, 0.05f) };
    p->vidaMaxima = AzarSecuencia(1.6f, 2.8f);
    p->vida = p->vidaMaxima;
    p->tamano = AzarSecuencia(0.05f, 0.09f);
    p->color = ColorLerp(COLOR_ACENTO_SALA_SECUENCIA, COLOR_BLANCO_SALA_SECUENCIA, AzarSecuencia(0.0f, 0.5f));
    p->color.a = 190;
}


static void ActualizarVisualesSecuencia(MinijuegoSecuenciaNeon& m, float deltaTime)
{
    // El panel encendido es exactamente pulsoVisible (mismos tiempos que
    // antes); al apagarse se desvanece en ~0,1 s, menos que la pausa, asi
    // dos pulsos iguales seguidos siguen viendose separados.
    for (int i = 0; i < CANTIDAD_PULSOS_NEON; i++)
    {
        m.brilloPanel[i] -= deltaTime * 9.0f;
        if (m.brilloPanel[i] < 0.0f) m.brilloPanel[i] = 0.0f;
    }

    if (
        m.fase == FASE_SECUENCIA_MOSTRANDO &&
        m.pulsoVisible &&
        m.indiceMuestra < m.cantidadPulsos
    )
    {
        m.brilloPanel[m.secuencia[m.indiceMuestra]] = 1.0f;
    }

    m.pulsoCamara -= deltaTime * 5.0f;
    if (m.pulsoCamara < 0.0f) m.pulsoCamara = 0.0f;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        EstadoJugadorSecuenciaNeon& j = m.jugadores[i];
        j.destelloPulso = fmaxf(0.0f, j.destelloPulso - deltaTime * 5.0f);
        j.destelloError = fmaxf(0.0f, j.destelloError - deltaTime * 1.4f);
        j.destelloRonda = fmaxf(0.0f, j.destelloRonda - deltaTime * 1.6f);
    }

    ActualizarAmbienteSecuencia(m, deltaTime);

    for (int i = 0; i < MAX_PARTICULAS_SECUENCIA_NEON; i++)
    {
        ParticulaSecuenciaNeon& p = m.particulas[i];
        if (!p.activa) continue;

        p.vida -= deltaTime;
        if (p.vida <= 0.0f)
        {
            p.activa = false;
            continue;
        }

        if (p.gravedad) p.velocidad.y -= 11.0f * deltaTime;

        p.posicion.x += p.velocidad.x * deltaTime;
        p.posicion.y += p.velocidad.y * deltaTime;
        p.posicion.z += p.velocidad.z * deltaTime;

        if (p.gravedad && p.posicion.y < 0.05f)
        {
            p.posicion.y = 0.05f;
            p.velocidad.y *= -0.35f;
            p.velocidad.x *= 0.6f;
            p.velocidad.z *= 0.6f;
        }
    }
}


//==================================================
// LOGICA
//==================================================

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


// Todo el feedback de aqui es identico para cualquier direccion: ni el
// panel compartido ni la consola muestran que boton se pulso.
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

    jugador.destelloPulso = 1.0f;

    if (pulso != minijuego.secuencia[jugador.indiceRespuesta])
    {
        jugador.eliminado = true;
        jugador.falloEstaRonda = true;
        jugador.destelloError = 1.0f;
        EmitirChispasSecuencia(
            minijuego,
            PuntoConsolaSecuencia(minijuego, indiceParticipante),
            COLOR_ERROR_SECUENCIA,
            Color{ 60, 40, 50, 255 },
            18,
            2.2f
        );
        ActivarTemblorCamaraGeneral(0.08f, 0.20f);
        ReproducirSonidoMinijuego(minijuego.audio, SONIDO_ERROR);
        return;
    }

    jugador.indiceRespuesta++;
    jugador.aciertosTotales++;

    if (jugador.indiceRespuesta >= minijuego.cantidadPulsos)
    {
        jugador.completoRonda = true;
        jugador.rondasSuperadas++;
        jugador.destelloRonda = 1.0f;
        EmitirChispasSecuencia(
            minijuego,
            PuntoConsolaSecuencia(minijuego, indiceParticipante),
            COLOR_ORO_SECUENCIA,
            COLOR_BLANCO_SALA_SECUENCIA,
            22,
            2.0f
        );
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
    minijuego.tiempoFase = 0.0f;
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
    minijuego.tiempoFase = 0.0f;
    minijuego.pulsoCamara = 1.0f;

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
    minijuego.tiempoFase = 0.0f;

    // Festejo en la consola de quien gano.
    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (
            minijuego.resultado.participantes[i].participo &&
            minijuego.resultado.participantes[i].posicionFinal == 1
        )
        {
            minijuego.jugadores[i].destelloRonda = 1.0f;
            EmitirChispasSecuencia(
                minijuego,
                PuntoConsolaSecuencia(minijuego, i),
                COLOR_ORO_SECUENCIA,
                COLOR_ACENTO_SALA_SECUENCIA,
                30,
                2.6f
            );
        }
    }

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
            jugador.destelloError = 1.0f;
            EmitirChispasSecuencia(
                minijuego,
                PuntoConsolaSecuencia(minijuego, i),
                COLOR_ERROR_SECUENCIA,
                Color{ 60, 40, 50, 255 },
                14,
                1.8f
            );
            huboEliminadoPorTiempo = true;
        }
    }

    if (huboEliminadoPorTiempo)
    {
        ActivarTemblorCamaraGeneral(0.08f, 0.20f);
        ReproducirSonidoMinijuego(minijuego.audio, SONIDO_ERROR);
    }

    minijuego.fase = FASE_SECUENCIA_RESOLUCION;
    minijuego.tiempoFase = 0.0f;
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

    for (int i = 0; i < MAX_PARTICULAS_SECUENCIA_NEON; i++)
    {
        particulas[i] = {};
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
    pulsoCamara = 0.0f;
    tiempoFase = 0.0f;
    temporizadorAmbiente = 0.0f;
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

    for (int i = 0; i < PULSOS_INICIALES_SECUENCIA; i++)
    {
        secuencia[i] = (PulsoSecuenciaNeon)GetRandomValue(
            0,
            CANTIDAD_PULSOS_NEON - 1
        );
    }

    cantidadPulsos = PULSOS_INICIALES_SECUENCIA;
}


void MinijuegoSecuenciaNeon::Actualizar(
    float deltaTime,
    Participante participantes[]
)
{
    tiempoAnimacion += deltaTime;
    tiempoFase += deltaTime;

    ActualizarVisualesSecuencia(*this, deltaTime);

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
                    pulsoCamara = 1.0f;
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
// VISUAL: PRIMITIVAS
//==================================================

static float Hash01Secuencia(int a, int b)
{
    unsigned int h = (unsigned int)a * 73856093u ^ (unsigned int)b * 19349663u;
    h ^= h >> 13;
    h *= 0x5bd1e995u;
    h ^= h >> 15;
    return (float)(h & 0xFFFFu) / 65535.0f;
}


static Color TinteSecuencia(Color c, float f)
{
    float r = c.r * f;
    float g = c.g * f;
    float b = c.b * f;
    if (r > 255.0f) r = 255.0f;
    if (g > 255.0f) g = 255.0f;
    if (b > 255.0f) b = 255.0f;
    return Color{ (unsigned char)r, (unsigned char)g, (unsigned char)b, c.a };
}


static Color ConAlfaSecuencia(Color c, float alfa)
{
    if (alfa < 0.0f) alfa = 0.0f;
    if (alfa > 1.0f) alfa = 1.0f;
    c.a = (unsigned char)(alfa * 255.0f);
    return c;
}


// Caja con cada cara a un brillo distinto: volumen sin luces reales.
static void CajaFacetadaSecuencia(Vector3 centro, Vector3 tamano, Color color, float brilloTapa)
{
    float x0 = centro.x - tamano.x * 0.5f;
    float x1 = centro.x + tamano.x * 0.5f;
    float y0 = centro.y - tamano.y * 0.5f;
    float y1 = centro.y + tamano.y * 0.5f;
    float z0 = centro.z - tamano.z * 0.5f;
    float z1 = centro.z + tamano.z * 0.5f;

    Color tapa = TinteSecuencia(color, brilloTapa);
    Color frente = TinteSecuencia(color, 0.78f);
    Color atras = TinteSecuencia(color, 0.58f);
    Color derecha = TinteSecuencia(color, 0.62f);
    Color izquierda = TinteSecuencia(color, 0.70f);

    DrawTriangle3D({ x0, y1, z0 }, { x0, y1, z1 }, { x1, y1, z1 }, tapa);
    DrawTriangle3D({ x0, y1, z0 }, { x1, y1, z1 }, { x1, y1, z0 }, tapa);
    DrawTriangle3D({ x0, y0, z1 }, { x1, y0, z1 }, { x1, y1, z1 }, frente);
    DrawTriangle3D({ x0, y0, z1 }, { x1, y1, z1 }, { x0, y1, z1 }, frente);
    DrawTriangle3D({ x0, y0, z0 }, { x0, y1, z0 }, { x1, y1, z0 }, atras);
    DrawTriangle3D({ x0, y0, z0 }, { x1, y1, z0 }, { x1, y0, z0 }, atras);
    DrawTriangle3D({ x1, y0, z0 }, { x1, y1, z0 }, { x1, y1, z1 }, derecha);
    DrawTriangle3D({ x1, y0, z0 }, { x1, y1, z1 }, { x1, y0, z1 }, derecha);
    DrawTriangle3D({ x0, y0, z0 }, { x0, y0, z1 }, { x0, y1, z1 }, izquierda);
    DrawTriangle3D({ x0, y0, z0 }, { x0, y1, z1 }, { x0, y1, z0 }, izquierda);
}


static void CajaMetalSecuencia(Vector3 centro, Vector3 tamano, Color color)
{
    CajaFacetadaSecuencia(centro, tamano, color, 1.3f);
    DrawCubeWiresV(centro, tamano, Fade(BLACK, 0.30f));
}


// Rectangulo vertical mirando a +Z (pared, pantallas).
static void RectParedSecuencia(float x0, float y0, float x1, float y1, float z, Color color)
{
    DrawTriangle3D({ x0, y0, z }, { x1, y0, z }, { x1, y1, z }, color);
    DrawTriangle3D({ x0, y0, z }, { x1, y1, z }, { x0, y1, z }, color);
}


static void RectSueloSecuencia(float x0, float z0, float x1, float z1, float y, Color color)
{
    DrawTriangle3D({ x0, y, z0 }, { x0, y, z1 }, { x1, y, z1 }, color);
    DrawTriangle3D({ x0, y, z0 }, { x1, y, z1 }, { x1, y, z0 }, color);
}


// Disco de luz (centro opaco, borde transparente). Solo con mezcla
// aditiva y sin escribir profundidad.
static void ResplandorSecuencia(Vector3 centro, Vector3 u, Vector3 v, float radio, Color color)
{
    const int LADOS = 18;
    rlBegin(RL_TRIANGLES);

    for (int k = 0; k < LADOS; k++)
    {
        float a0 = 6.2832f * (float)k / (float)LADOS;
        float a1 = 6.2832f * (float)(k + 1) / (float)LADOS;
        float c0 = std::cos(a0) * radio, s0 = std::sin(a0) * radio;
        float c1 = std::cos(a1) * radio, s1 = std::sin(a1) * radio;

        rlColor4ub(color.r, color.g, color.b, color.a);
        rlVertex3f(centro.x, centro.y, centro.z);
        rlColor4ub(color.r, color.g, color.b, 0);
        rlVertex3f(centro.x + u.x * c0 + v.x * s0, centro.y + u.y * c0 + v.y * s0, centro.z + u.z * c0 + v.z * s0);
        rlColor4ub(color.r, color.g, color.b, 0);
        rlVertex3f(centro.x + u.x * c1 + v.x * s1, centro.y + u.y * c1 + v.y * s1, centro.z + u.z * c1 + v.z * s1);
    }

    rlEnd();
}


static void ResplandorParedSecuencia(Vector3 centro, float radio, Color color)
{
    ResplandorSecuencia(centro, { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, radio, color);
}


static void ResplandorSueloSecuencia(float x, float z, float radio, Color color)
{
    ResplandorSecuencia({ x, 0.03f, z }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, radio, color);
}


// Reflejo alargado sobre el piso brillante: opaco junto a la fuente y
// transparente hacia la camara.
static void ReflejoSueloSecuencia(float x, float z0, float z1, float ancho, Color color)
{
    float a = ancho * 0.5f;
    rlBegin(RL_TRIANGLES);
    rlColor4ub(color.r, color.g, color.b, color.a); rlVertex3f(x - a, 0.025f, z0);
    rlColor4ub(color.r, color.g, color.b, 0);       rlVertex3f(x - a * 0.6f, 0.025f, z1);
    rlColor4ub(color.r, color.g, color.b, 0);       rlVertex3f(x + a * 0.6f, 0.025f, z1);
    rlColor4ub(color.r, color.g, color.b, color.a); rlVertex3f(x - a, 0.025f, z0);
    rlColor4ub(color.r, color.g, color.b, 0);       rlVertex3f(x + a * 0.6f, 0.025f, z1);
    rlColor4ub(color.r, color.g, color.b, color.a); rlVertex3f(x + a, 0.025f, z0);
    rlEnd();
}


//==================================================
// VISUAL: PANEL DE SECUENCIA
//==================================================

static Camera3D ObtenerCamaraSecuencia(float intro, float pulso)
{
    float movimiento = ObtenerFactorMovimientoCamaraMinijuegos();

    Camera3D camara{};
    camara.position = { 0.0f, 9.6f + 1.4f * intro * movimiento, 15.4f + 3.0f * intro * movimiento };
    camara.target = { 0.0f, 3.4f, -2.0f };
    camara.up = { 0.0f, 1.0f, 0.0f };
    camara.fovy = 50.0f - 1.3f * pulso * movimiento;
    camara.projection = CAMERA_PERSPECTIVE;
    return camara;
}


// Centro del panel gigante (cruz de direcciones sobre el escenario).
static Vector3 CentroPanelSecuencia(PulsoSecuenciaNeon pulso)
{
    switch (pulso)
    {
        case PULSO_NEON_ARRIBA: return { 0.0f, 7.3f, Z_PANELES_SECUENCIA };
        case PULSO_NEON_DERECHA: return { 2.7f, 4.7f, Z_PANELES_SECUENCIA };
        case PULSO_NEON_ABAJO: return { 0.0f, 2.1f, Z_PANELES_SECUENCIA };
        case PULSO_NEON_IZQUIERDA: return { -2.7f, 4.7f, Z_PANELES_SECUENCIA };
        case CANTIDAD_PULSOS_NEON: break;
    }

    return { 0.0f, 4.7f, Z_PANELES_SECUENCIA };
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


// Flecha plana en el plano del panel (apunta hacia arriba con angulo 0).
static void DibujarFlechaPlanaSecuencia(Vector3 centro, float anguloGrados, float escala, Color color)
{
    static const Vector2 puntos[7] =
    {
        { 0.0f, 0.92f }, { -0.80f, 0.08f }, { 0.80f, 0.08f },
        { -0.30f, 0.08f }, { 0.30f, 0.08f }, { 0.30f, -0.88f }, { -0.30f, -0.88f }
    };

    float a = anguloGrados * DEG2RAD;
    float c = std::cos(a);
    float s = std::sin(a);
    Vector3 p[7];

    for (int i = 0; i < 7; i++)
    {
        float x = puntos[i].x * escala;
        float y = puntos[i].y * escala;
        p[i] = { centro.x + x * c - y * s, centro.y + x * s + y * c, centro.z };
    }

    DrawTriangle3D(p[0], p[1], p[2], color);
    DrawTriangle3D(p[3], p[6], p[5], color);
    DrawTriangle3D(p[3], p[5], p[4], color);
}


static void DibujarPanelNeon3D(PulsoSecuenciaNeon pulso, float luz, float tiempo)
{
    Color dir = ColorPulsoNeon(pulso);
    Vector3 c = CentroPanelSecuencia(pulso);
    float pop = 1.0f + 0.05f * luz;
    float m = LADO_PANEL_SECUENCIA * 0.5f * pop;
    float zFrente = c.z + 0.31f;

    // Carcasa facetada y vidrio de la pantalla.
    CajaMetalSecuencia({ c.x, c.y, c.z }, { LADO_PANEL_SECUENCIA + 0.5f, LADO_PANEL_SECUENCIA + 0.5f, 0.6f }, Color{ 40, 36, 66, 255 });

    Color vidrio = ColorLerp(Color{ 12, 10, 28, 255 }, TinteSecuencia(dir, 0.55f), 0.10f + 0.70f * luz);
    RectParedSecuencia(c.x - m, c.y - m, c.x + m, c.y + m, zFrente, vidrio);

    // Barrido de lineas de pantalla: igual en los cuatro paneles.
    for (int k = 0; k < 3; k++)
    {
        float f = std::fmod(tiempo * 0.35f + (float)k / 3.0f, 1.0f);
        float y = c.y - m + f * 2.0f * m;
        RectParedSecuencia(c.x - m, y, c.x + m, y + 0.04f, zFrente + 0.005f, Fade(WHITE, 0.035f + 0.05f * luz));
    }

    // Tubo de neon del borde.
    Color tubo = ColorLerp(TinteSecuencia(dir, 0.42f), ColorLerp(dir, WHITE, 0.4f), luz);
    float g = 0.07f;
    RectParedSecuencia(c.x - m, c.y + m - g, c.x + m, c.y + m, zFrente + 0.01f, tubo);
    RectParedSecuencia(c.x - m, c.y - m, c.x + m, c.y - m + g, zFrente + 0.01f, tubo);
    RectParedSecuencia(c.x - m, c.y - m, c.x - m + g, c.y + m, zFrente + 0.01f, tubo);
    RectParedSecuencia(c.x + m - g, c.y - m, c.x + m, c.y + m, zFrente + 0.01f, tubo);

    // Flecha: contorno de neon y relleno (blanco caliente al encenderse).
    float angulo = AnguloFlechaSecuencia(pulso);
    Color contorno = ColorLerp(TinteSecuencia(dir, 0.55f), dir, luz);
    Color relleno = ColorLerp(Color{ 16, 14, 32, 255 }, ColorLerp(dir, WHITE, 0.75f), luz);
    DibujarFlechaPlanaSecuencia({ c.x, c.y, zFrente + 0.02f }, angulo, pop, contorno);
    DibujarFlechaPlanaSecuencia({ c.x, c.y, zFrente + 0.03f }, angulo, 0.70f * pop, relleno);
}


static void DibujarCruzPanelesSecuencia(const MinijuegoSecuenciaNeon& m, float t, float luces[CANTIDAD_PULSOS_NEON])
{
    // Respaldo en cruz y tarima del escenario.
    CajaMetalSecuencia({ 0.0f, 4.7f, Z_PANELES_SECUENCIA - 0.45f }, { 3.4f, 8.6f, 0.4f }, Color{ 26, 22, 48, 255 });
    CajaMetalSecuencia({ 0.0f, 4.7f, Z_PANELES_SECUENCIA - 0.45f }, { 8.8f, 3.4f, 0.4f }, Color{ 26, 22, 48, 255 });
    CajaMetalSecuencia({ 0.0f, 0.25f, -7.85f }, { 10.8f, 0.5f, 2.6f }, Color{ 34, 30, 58, 255 });
    (DrawCube)({ 0.0f, 0.47f, -6.54f }, 10.8f, 0.05f, 0.06f, ConAlfaSecuencia(COLOR_ACENTO_SALA_SECUENCIA, 0.65f + 0.2f * std::sin(t * 2.0f)));

    for (int d = 0; d < CANTIDAD_PULSOS_NEON; d++)
    {
        DibujarPanelNeon3D((PulsoSecuenciaNeon)d, luces[d], t);
    }

    // Nucleo central con el largo de la secuencia (el numero va en 2D).
    Vector3 hub = { 0.0f, 4.7f, Z_PANELES_SECUENCIA + 0.05f };
    DrawCylinderEx(hub, { hub.x, hub.y, hub.z + 0.32f }, 0.62f, 0.56f, 20, Color{ 30, 26, 54, 255 });
    float latido = m.fase == FASE_SECUENCIA_MOSTRANDO ? 0.5f + 0.5f * std::sin(t * 9.0f) : 0.5f + 0.5f * std::sin(t * 2.5f);
    DrawCircle3D({ hub.x, hub.y, hub.z + 0.34f }, 0.58f, { 0.0f, 1.0f, 0.0f }, 0.0f, ConAlfaSecuencia(COLOR_ACENTO_SALA_SECUENCIA, 0.6f + 0.4f * latido));
    DrawCircle3D({ hub.x, hub.y, hub.z + 0.34f }, 0.50f, { 0.0f, 1.0f, 0.0f }, 0.0f, ConAlfaSecuencia(COLOR_ACENTO_SALA_SECUENCIA, 0.3f + 0.3f * latido));
}


//==================================================
// VISUAL: SALA
//==================================================

static void DibujarSueloSecuencia(float t)
{
    // Base y losas de vidrio negro con variacion por hash.
    (DrawCube)({ 0.0f, -0.25f, 2.0f }, 60.0f, 0.4f, 34.0f, Color{ 8, 7, 18, 255 });

    for (int ix = -12; ix < 12; ix++)
    {
        for (int iz = -5; iz < 9; iz++)
        {
            float h = Hash01Secuencia(ix, iz);
            float x0 = (float)ix * 2.0f;
            float z0 = (float)iz * 2.0f - 1.0f;
            float cercania = 1.0f - fminf(1.0f, fabsf((float)iz * 2.0f + 4.0f) / 18.0f);
            Color losa =
            {
                (unsigned char)(14 + h * 6.0f + cercania * 6.0f),
                (unsigned char)(12 + h * 5.0f + cercania * 4.0f),
                (unsigned char)(30 + h * 9.0f + cercania * 10.0f),
                255
            };
            RectSueloSecuencia(x0 + 0.04f, z0 + 0.04f, x0 + 1.96f, z0 + 1.96f, 0.0f, losa);

            // Brillo especular en algunas losas (piso pulido).
            if (h > 0.82f)
            {
                RectSueloSecuencia(x0 + 0.3f, z0 + 0.3f, x0 + 0.9f, z0 + 0.5f, 0.004f, Fade(WHITE, 0.035f));
            }
        }
    }

    // Grilla de neon rosa que se apaga con la distancia y respira.
    float respira = 0.75f + 0.25f * std::sin(t * 1.6f);

    for (int ix = -12; ix <= 12; ix++)
    {
        float x = (float)ix * 2.0f;
        float alfa = (0.30f - fabsf(x) * 0.008f) * respira;
        DrawLine3D({ x, 0.008f, -11.0f }, { x, 0.008f, 17.0f }, ConAlfaSecuencia(Color{ 210, 50, 150, 255 }, alfa));
    }

    for (int iz = -5; iz <= 9; iz++)
    {
        float z = (float)iz * 2.0f - 1.0f;
        float alfa = (0.16f + 0.18f * (1.0f - fabsf(z + 4.0f) / 20.0f)) * respira;
        DrawLine3D({ -24.0f, 0.008f, z }, { 24.0f, 0.008f, z }, ConAlfaSecuencia(Color{ 210, 50, 150, 255 }, alfa));
    }
}


static void DibujarParedFondoSecuencia(float t)
{
    // Muro de placas con variacion y pilastras.
    CajaFacetadaSecuencia({ 0.0f, 8.0f, -9.7f }, { 52.0f, 18.0f, 0.6f }, Color{ 20, 16, 38, 255 }, 1.0f);

    for (int ix = -6; ix < 6; ix++)
    {
        for (int iy = 0; iy < 8; iy++)
        {
            float h = Hash01Secuencia(ix + 40, iy + 7);
            float x0 = (float)ix * 4.0f;
            float y0 = (float)iy * 2.0f;
            Color placa = { (unsigned char)(22 + h * 8.0f), (unsigned char)(18 + h * 6.0f), (unsigned char)(42 + h * 12.0f), 255 };
            RectParedSecuencia(x0 + 0.06f, y0 + 0.06f, x0 + 3.94f, y0 + 1.94f, -9.39f, placa);
        }
    }

    for (int k = -3; k <= 3; k++)
    {
        if (k == 0) continue;
        float x = (float)k * 6.6f;
        CajaMetalSecuencia({ x, 8.0f, -9.2f }, { 0.9f, 16.0f, 0.5f }, Color{ 30, 26, 52, 255 });
    }

    // Franjas de neon rosa del muro (zocalo y cornisa).
    float a = 0.6f + 0.2f * std::sin(t * 1.8f);
    (DrawCube)({ 0.0f, 0.55f, -9.30f }, 52.0f, 0.08f, 0.06f, ConAlfaSecuencia(COLOR_ACENTO_SALA_SECUENCIA, a));
    (DrawCube)({ 0.0f, 12.2f, -9.30f }, 52.0f, 0.08f, 0.06f, ConAlfaSecuencia(COLOR_ACENTO_SALA_SECUENCIA, a));
}


static void DibujarBocinasSecuencia(float t, float energia)
{
    for (int lado = -1; lado <= 1; lado += 2)
    {
        float x = (float)lado * 11.5f;
        CajaMetalSecuencia({ x, 3.5f, -6.4f }, { 3.2f, 7.0f, 2.4f }, Color{ 30, 27, 48, 255 });
        CajaMetalSecuencia({ x, 7.4f, -6.4f }, { 2.4f, 0.8f, 1.8f }, Color{ 36, 32, 56, 255 });

        for (int fila = 0; fila < 2; fila++)
        {
            float y = 2.0f + fila * 3.0f;
            float latido = 1.0f + energia * 0.10f * std::sin(t * 18.0f + (float)fila);
            float zf = -5.18f;
            DrawCylinderEx({ x, y, zf }, { x, y, zf + 0.12f }, 1.15f, 1.05f, 18, Color{ 18, 16, 30, 255 });
            DrawCylinderEx({ x, y, zf + 0.12f }, { x, y, zf + 0.12f + 0.25f * latido }, 0.95f * latido, 0.30f, 16, Color{ 46, 42, 68, 255 });
            DrawCircle3D({ x, y, zf + 0.13f }, 1.08f, { 0.0f, 1.0f, 0.0f }, 0.0f, ConAlfaSecuencia(COLOR_ACENTO_SALA_SECUENCIA, 0.45f + 0.5f * energia));
        }
    }
}


// Decoracion condicionada a calidad: columnas LED, letreros abstractos
// (anillo y onda, nunca flechas) y viga con focos. Solo rosa y blanco.
static void DibujarDecoracionSecuencia(float t, float energia)
{
    NivelCalidadGrafica calidad = CalidadDecoracion();
    if (calidad == CALIDAD_BAJA) return;

    int columnas = calidad == CALIDAD_ALTA ? 2 : 1;
    const float xColumnas[2] = { 7.6f, 15.2f };

    for (int k = 0; k < columnas; k++)
    {
        for (int lado = -1; lado <= 1; lado += 2)
        {
            float x = (float)lado * xColumnas[k];
            float z = k == 0 ? -7.9f : -8.6f;
            CajaMetalSecuencia({ x, 3.1f, z }, { 0.9f, 6.2f, 0.7f }, Color{ 26, 22, 44, 255 });

            float nivel = 0.45f + 0.30f * std::sin(t * 2.3f + (float)k * 1.7f + (float)lado) + 0.25f * energia;
            int encendidos = (int)(nivel * 12.0f);

            for (int s = 0; s < 12; s++)
            {
                bool on = s < encendidos;
                Color led = ColorLerp(COLOR_ACENTO_SALA_SECUENCIA, COLOR_BLANCO_SALA_SECUENCIA, (float)s / 11.0f);
                RectParedSecuencia(x - 0.3f, 0.35f + s * 0.48f, x + 0.3f, 0.35f + s * 0.48f + 0.34f, z + 0.36f, on ? led : TinteSecuencia(led, 0.22f));
            }
        }
    }

    // Letreros de tubo de neon: anillo (izquierda) y onda (derecha).
    float parpadeo = (std::fmod(t, 5.3f) < 0.08f) ? 0.35f : 1.0f;
    Color neon = ConAlfaSecuencia(COLOR_ACENTO_SALA_SECUENCIA, 0.9f * parpadeo);
    Color neonBlanco = ConAlfaSecuencia(COLOR_BLANCO_SALA_SECUENCIA, 0.85f);

    for (int r = 0; r < 3; r++)
    {
        DrawCircle3D({ -15.5f, 9.4f, -9.36f }, 1.25f + r * 0.03f, { 0.0f, 1.0f, 0.0f }, 0.0f, neon);
        DrawCircle3D({ -15.5f, 9.4f, -9.36f }, 0.62f + r * 0.03f, { 0.0f, 1.0f, 0.0f }, 0.0f, neonBlanco);
    }

    for (int s = 0; s < 16; s++)
    {
        float x0 = 13.6f + s * 0.25f;
        float x1 = x0 + 0.25f;
        float y0 = 9.4f + std::sin((float)s * 0.8f + t * 2.0f) * 0.55f;
        float y1 = 9.4f + std::sin((float)(s + 1) * 0.8f + t * 2.0f) * 0.55f;
        for (int r = 0; r < 3; r++)
        {
            DrawLine3D({ x0, y0 + r * 0.03f, -9.36f }, { x1, y1 + r * 0.03f, -9.36f }, neon);
        }
    }

    // Viga del techo con focos.
    DrawCylinderEx({ -17.0f, 11.0f, -5.5f }, { 17.0f, 11.0f, -5.5f }, 0.18f, 0.18f, 8, Color{ 62, 60, 84, 255 });

    for (int l = 0; l < 7; l++)
    {
        float x = -12.0f + l * 4.0f;
        Color foco = l % 2 == 0 ? COLOR_ACENTO_SALA_SECUENCIA : COLOR_BLANCO_SALA_SECUENCIA;
        CajaMetalSecuencia({ x, 10.6f, -5.5f }, { 0.55f, 0.6f, 0.55f }, Color{ 44, 42, 62, 255 });
        DrawSphereEx({ x, 10.25f, -5.5f }, 0.18f, 8, 8, ConAlfaSecuencia(foco, 0.6f + 0.4f * std::sin(t * 3.0f + l)));
    }
}


//==================================================
// VISUAL: CONSOLAS
//==================================================

static void DibujarConsolaJugador3D(
    const MinijuegoSecuenciaNeon& m,
    const EstadoJugadorSecuenciaNeon& estado,
    const Participante& participante,
    float x
)
{
    Color color = participante.color;
    bool fallo = estado.eliminado;
    Color metal = fallo ? Color{ 52, 30, 40, 255 } : Color{ 44, 40, 70, 255 };
    float zc = Z_CONSOLAS_SECUENCIA;

    // Pedestal con anillo del color del jugador.
    DrawCylinderEx({ x, 0.0f, Z_JUGADORES_SECUENCIA }, { x, 0.12f, Z_JUGADORES_SECUENCIA }, 1.15f, 1.05f, 22, Color{ 30, 28, 48, 255 });
    DrawCircle3D({ x, 0.13f, Z_JUGADORES_SECUENCIA }, 0.98f, { 1.0f, 0.0f, 0.0f }, 90.0f, fallo ? COLOR_ERROR_SECUENCIA : color);
    DrawCircle3D({ x, 0.13f, Z_JUGADORES_SECUENCIA }, 0.92f, { 1.0f, 0.0f, 0.0f }, 90.0f, fallo ? COLOR_ERROR_SECUENCIA : color);

    // Cuerpo, tapa y moldura del color del jugador.
    DibujarSombraRetroRectangular({ x, 0.0f, zc }, 2.9f, 1.3f);
    CajaMetalSecuencia({ x, 0.45f, zc }, { 2.6f, 0.9f, 1.0f }, metal);
    CajaMetalSecuencia({ x, 0.95f, zc }, { 2.9f, 0.12f, 1.25f }, TinteSecuencia(metal, 1.15f));
    Color moldura = fallo ? TinteSecuencia(COLOR_ERROR_SECUENCIA, 0.7f) : color;
    (DrawCube)({ x, 0.84f, zc + 0.51f }, 2.6f, 0.06f, 0.03f, moldura);

    // Cuatro botones en cruz, todos iguales y quietos: la consola nunca
    // muestra que direccion se pulso. Al pulsar, el destello es parejo.
    static const float dx[CANTIDAD_PULSOS_NEON] = { 0.0f, 0.42f, 0.0f, -0.42f };
    static const float dz[CANTIDAD_PULSOS_NEON] = { -0.3f, 0.0f, 0.3f, 0.0f };
    Color boton = ColorLerp(Color{ 120, 112, 160, 255 }, WHITE, 0.5f * estado.destelloPulso);

    for (int d = 0; d < CANTIDAD_PULSOS_NEON; d++)
    {
        CajaFacetadaSecuencia({ x + 0.55f + dx[d], 1.05f, zc + 0.12f + dz[d] }, { 0.28f, 0.08f, 0.24f }, boton, 1.3f);
    }

    // Pantalla trasera con segmentos de avance (neutros).
    CajaMetalSecuencia({ x - 0.35f, 1.55f, zc - 0.42f }, { 1.9f, 1.0f, 0.14f }, Color{ 34, 30, 56, 255 });
    Color fondoPantalla = ColorLerp(Color{ 10, 10, 22, 255 }, color, 0.12f + 0.18f * estado.destelloPulso);
    if (estado.destelloError > 0.0f) fondoPantalla = ColorLerp(fondoPantalla, COLOR_ERROR_SECUENCIA, 0.6f * estado.destelloError);
    float zp = zc - 0.34f;
    RectParedSecuencia(x - 1.2f, 1.13f, x + 0.5f, 1.97f, zp, fondoPantalla);

    int total = m.cantidadPulsos > 0 ? m.cantidadPulsos : 1;
    int llenos = 0;
    Color colorLleno = color;

    if (m.fase == FASE_SECUENCIA_MOSTRANDO)
    {
        llenos = m.indiceMuestra + (m.pulsoVisible ? 1 : 0);
        colorLleno = COLOR_BLANCO_SALA_SECUENCIA;
    }
    else if (m.fase == FASE_SECUENCIA_RESPONDIENDO || m.fase == FASE_SECUENCIA_RESOLUCION)
    {
        llenos = estado.indiceRespuesta;
        if (estado.completoRonda) colorLleno = ColorLerp(color, WHITE, 0.35f + 0.4f * estado.destelloRonda);
    }
    else if (m.fase == FASE_SECUENCIA_TERMINADO)
    {
        llenos = total;
    }

    if (fallo) { llenos = total; colorLleno = TinteSecuencia(COLOR_ERROR_SECUENCIA, 0.8f); }

    float ancho = 1.5f / (float)total;

    for (int k = 0; k < total; k++)
    {
        float s0 = x - 1.1f + k * ancho + 0.03f;
        Color seg = k < llenos ? colorLleno : TinteSecuencia(color, 0.25f);
        RectParedSecuencia(s0, 1.35f, s0 + ancho - 0.06f, 1.75f, zp + 0.01f, seg);
    }

}


static void DibujarParticulasSecuencia(const MinijuegoSecuenciaNeon& m)
{
    for (int i = 0; i < MAX_PARTICULAS_SECUENCIA_NEON; i++)
    {
        const ParticulaSecuenciaNeon& p = m.particulas[i];
        if (!p.activa || p.vidaMaxima <= 0.0f) continue;

        float vida = p.vida / p.vidaMaxima;
        float tamano = p.tamano * (0.4f + 0.6f * vida);
        Color color = p.color;
        color.a = (unsigned char)((float)p.color.a * (vida > 0.3f ? 1.0f : vida / 0.3f));
        DrawCubeV(p.posicion, { tamano, tamano, tamano }, color);
    }
}


// Iluminacion falsa: SOLO el panel que la fase de MOSTRAR enciende emite
// luz de direccion; el resto es rosa/blanco o del color del jugador.
static void DibujarResplandoresSecuencia(
    float t,
    float energia,
    const float luces[CANTIDAD_PULSOS_NEON]
)
{
    if (FactorCalidadGrafica(CalidadEfectos()) < 0.5f) return;

    rlDrawRenderBatchActive();
    rlDisableDepthMask();
    BeginBlendMode(BLEND_ADDITIVE);

    // Paneles: halo minimo e igual para los cuatro cuando estan apagados.
    for (int d = 0; d < CANTIDAD_PULSOS_NEON; d++)
    {
        Color dir = ColorPulsoNeon((PulsoSecuenciaNeon)d);
        Vector3 c = CentroPanelSecuencia((PulsoSecuenciaNeon)d);
        float luz = luces[d];

        ResplandorParedSecuencia({ c.x, c.y, c.z + 0.36f }, 1.5f, Color{ dir.r, dir.g, dir.b, 16 });
        ReflejoSueloSecuencia(c.x, -6.5f, -1.0f, 1.4f, Color{ dir.r, dir.g, dir.b, 10 });

        if (luz <= 0.01f) continue;

        ResplandorParedSecuencia({ c.x, c.y, c.z + 0.4f }, 2.2f + 0.5f * luz, Color{ dir.r, dir.g, dir.b, (unsigned char)(170.0f * luz) });
        ResplandorParedSecuencia({ c.x, c.y, c.z + 0.2f }, 5.0f, Color{ dir.r, dir.g, dir.b, (unsigned char)(60.0f * luz) });
        ResplandorSueloSecuencia(c.x, -5.2f, 3.4f, Color{ dir.r, dir.g, dir.b, (unsigned char)(80.0f * luz) });
        ReflejoSueloSecuencia(c.x, -6.5f, 2.0f, 2.2f, Color{ dir.r, dir.g, dir.b, (unsigned char)(95.0f * luz) });
    }

    // Nucleo central y franjas del muro.
    ResplandorParedSecuencia({ 0.0f, 4.7f, Z_PANELES_SECUENCIA + 0.4f }, 1.3f, Color{ 255, 64, 168, (unsigned char)(50.0f + 40.0f * energia) });
    for (int k = -4; k <= 4; k++)
    {
        ResplandorParedSecuencia({ (float)k * 5.5f, 0.55f, -9.25f }, 1.3f, Color{ 255, 64, 168, 34 });
    }
    ReflejoSueloSecuencia(0.0f, -6.5f, -2.0f, 10.0f, Color{ 255, 64, 168, 18 });

    // Bocinas.
    for (int lado = -1; lado <= 1; lado += 2)
    {
        for (int fila = 0; fila < 2; fila++)
        {
            ResplandorParedSecuencia({ (float)lado * 11.5f, 2.0f + fila * 3.0f, -5.0f }, 1.6f, Color{ 255, 64, 168, (unsigned char)(26.0f + 50.0f * energia) });
        }
        ReflejoSueloSecuencia((float)lado * 11.5f, -5.1f, 0.5f, 2.6f, Color{ 255, 64, 168, 22 });
    }

    // Focos del techo (decoracion).
    if (CalidadDecoracion() != CALIDAD_BAJA)
    {
        for (int l = 0; l < 7; l++)
        {
            float x = -12.0f + l * 4.0f;
            float bx = x + std::sin(t * 0.55f + (float)l * 1.3f) * 2.4f;
            Color foco = l % 2 == 0 ? COLOR_ACENTO_SALA_SECUENCIA : COLOR_BLANCO_SALA_SECUENCIA;
            DrawCylinderEx({ x, 10.2f, -5.5f }, { bx, 0.05f, -3.5f }, 0.12f, 1.3f, 12, Color{ foco.r, foco.g, foco.b, 10 });
            ResplandorSueloSecuencia(bx, -3.5f, 1.4f, Color{ foco.r, foco.g, foco.b, 26 });
        }

        for (int lado = -1; lado <= 1; lado += 2)
        {
            ResplandorParedSecuencia({ (float)lado * 7.6f, 3.1f, -7.5f }, 1.8f, Color{ 255, 90, 180, (unsigned char)(24.0f + 30.0f * energia) });
        }
    }

    EndBlendMode();
    rlEnableDepthMask();
}


static void DibujarLucesConsolasSecuencia(const MinijuegoSecuenciaNeon& m, const Participante participantes[])
{
    if (FactorCalidadGrafica(CalidadEfectos()) < 0.5f) return;

    rlDrawRenderBatchActive();
    rlDisableDepthMask();
    BeginBlendMode(BLEND_ADDITIVE);

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!m.resultado.participantes[i].participo) continue;

        const EstadoJugadorSecuenciaNeon& j = m.jugadores[i];
        float x = XJugadorSecuencia(m, i);
        Color c = j.eliminado ? COLOR_ERROR_SECUENCIA : participantes[i].color;

        ResplandorSueloSecuencia(x, Z_JUGADORES_SECUENCIA, 1.5f, Color{ c.r, c.g, c.b, (unsigned char)(j.eliminado ? 40 : 55) });
        ResplandorParedSecuencia({ x - 0.35f, 1.55f, Z_CONSOLAS_SECUENCIA - 0.3f }, 1.0f + 0.3f * j.destelloPulso, Color{ c.r, c.g, c.b, (unsigned char)(30.0f + 90.0f * j.destelloPulso) });

        if (j.destelloError > 0.0f)
        {
            ResplandorSueloSecuencia(x, Z_CONSOLAS_SECUENCIA, 2.4f, Color{ 255, 72, 58, (unsigned char)(150.0f * j.destelloError) });
        }

        if (j.destelloRonda > 0.0f)
        {
            ResplandorParedSecuencia({ x, 1.6f, Z_JUGADORES_SECUENCIA }, 1.8f, Color{ 255, 226, 96, (unsigned char)(120.0f * j.destelloRonda) });
            ResplandorSueloSecuencia(x, Z_JUGADORES_SECUENCIA, 2.0f + (1.0f - j.destelloRonda), Color{ c.r, c.g, c.b, (unsigned char)(130.0f * j.destelloRonda) });
        }
    }

    EndBlendMode();
    rlEnableDepthMask();
}


//==================================================
// DIBUJO PRINCIPAL
//==================================================

static void TextoControlesSecuencia(const Participante& participante, char* buffer, int tamano)
{
    const char* teclas = "WASD";
    if (participante.control == CONTROL_GAMEPAD) teclas = "Y B A X";
    else if (participante.control == CONTROL_TECLADO_FLECHAS) teclas = "FLECHAS";

    std::snprintf(buffer, (size_t)tamano, "REPETI CON: %s", teclas);
}


void MinijuegoSecuenciaNeon::Dibujar(
    const Participante participantes[]
) const
{
    float t = tiempoAnimacion;

    // Luz de cada panel: solo la fase de MOSTRAR la enciende.
    float luces[CANTIDAD_PULSOS_NEON]{};
    float energia = 0.15f;

    for (int d = 0; d < CANTIDAD_PULSOS_NEON; d++)
    {
        luces[d] = brilloPanel[d];

        if (
            fase == FASE_SECUENCIA_MOSTRANDO &&
            pulsoVisible &&
            indiceMuestra < cantidadPulsos &&
            (int)secuencia[indiceMuestra] == d
        )
        {
            luces[d] = 1.0f;
        }

        energia = fmaxf(energia, luces[d]);
    }

    float intro = fase == FASE_SECUENCIA_PREPARACION
        ? tiempoPreparacion / DURACION_PREPARACION_SECUENCIA
        : 0.0f;
    Camera3D camara = ObtenerCamaraSecuencia(intro * intro, pulsoCamara);

    SeleccionarTemaVisualMinijuego(TEMA_VISUAL_NINGUNO);
    ClearBackground(Color{ 10, 8, 22, 255 });
    BeginMode3D(camara);

    // Las cajas facetadas son triangulos sueltos: sin descarte de caras.
    rlDrawRenderBatchActive();
    rlDisableBackfaceCulling();

    DibujarSueloSecuencia(t);
    DibujarParedFondoSecuencia(t);
    DibujarDecoracionSecuencia(t, energia);
    DibujarBocinasSecuencia(t, energia);
    DibujarCruzPanelesSecuencia(*this, t, luces);

    Vector2 etiquetas[MAX_PARTICIPANTES]{};

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!resultado.participantes[i].participo) continue;

        const EstadoJugadorSecuenciaNeon& estado = jugadores[i];
        float x = XJugadorSecuencia(*this, i);

        DibujarConsolaJugador3D(*this, estado, participantes[i], x);

        // Pulsar: leve hundida igual para cualquier boton; ronda completa: salto.
        float salto = estado.destelloRonda > 0.0f
            ? std::sin((1.0f - estado.destelloRonda) * PI) * 0.45f
            : 0.0f;
        float hundida = estado.eliminado ? 0.0f : 0.06f * estado.destelloPulso;
        float idle = estado.eliminado ? 0.0f : std::sin(t * 3.0f + (float)i) * 0.03f;

        JugadorPrueba figura{};
        figura.posicion = { x, 0.12f + figura.tamano.y * 0.5f + salto - hundida + idle, Z_JUGADORES_SECUENCIA };
        figura.direccionMirada = { 0.0f, 0.0f, -1.0f };
        figura.enSuelo = salto <= 0.0f;
        figura.aplastado = estado.eliminado;

        Participante visual = participantes[i];
        visual.activo = true;
        visual.conectado = true;
        DibujarJugadorCuboPrueba(figura, visual);

        etiquetas[i] = GetWorldToScreen({ x, 2.75f, Z_JUGADORES_SECUENCIA }, camara);
    }

    DibujarParticulasSecuencia(*this);
    DibujarResplandoresSecuencia(t, energia, luces);
    DibujarLucesConsolasSecuencia(*this, participantes);

    rlDrawRenderBatchActive();
    rlEnableBackfaceCulling();
    EndMode3D();

    // Rotulos sobre cada jugador.
    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!resultado.participantes[i].participo) continue;

        char rotulo[8];
        std::snprintf(rotulo, sizeof(rotulo), "J%d", participantes[i].numeroJugador);
        OpcionesEtiquetaUI op;
        op.borde = jugadores[i].eliminado ? UI_PELIGRO : participantes[i].color;
        op.pico = true;
        op.alfa = jugadores[i].eliminado ? 0.6f : 1.0f;
        DibujarEtiquetaUI(rotulo, etiquetas[i], -4.0f, op);
    }

    // Largo de la secuencia en el nucleo central.
    if (fase != FASE_SECUENCIA_PREPARACION)
    {
        Vector2 centroHub = GetWorldToScreen({ 0.0f, 4.7f, Z_PANELES_SECUENCIA + 0.4f }, camara);
        char largo[8];
        std::snprintf(largo, sizeof(largo), "%d", cantidadPulsos);
        DibujarTextoMundoContornoUI(largo, centroHub.x, centroHub.y - Px(15.0f), UI_TEXTO_GRANDE, UI_TEXTO_MUNDO);
    }

    // Anuncio de fase sobre la tarima (debajo de la cruz de paneles).
    const char* anuncio = nullptr;
    Color colorAnuncio = UI_FOCO;
    float alfaAnuncio = 1.0f;
    char textoAnuncio[24] = "";

    // En la primera ronda espera a que se vaya el «¡YA!» del HUD.
    if (fase == FASE_SECUENCIA_MOSTRANDO && (numeroRonda > 1 || tiempoFase > 1.1f))
    {
        anuncio = "MEMORIZÁ";
        colorAnuncio = UI_TEXTO_MUNDO;
        alfaAnuncio = 0.75f + 0.25f * std::sin(t * 4.0f);
    }
    else if (fase == FASE_SECUENCIA_RESPONDIENDO && tiempoFase < 1.4f)
    {
        anuncio = "¡REPETI!";
        alfaAnuncio = tiempoFase > 1.0f ? (1.4f - tiempoFase) / 0.4f : 1.0f;
    }
    else if (fase == FASE_SECUENCIA_RESOLUCION)
    {
        int vivos = ContarVivosSecuencia(*this);
        std::snprintf(textoAnuncio, sizeof(textoAnuncio), vivos == 1 ? "%d SIGUE EN PIE" : "%d SIGUEN EN PIE", vivos);
        anuncio = textoAnuncio;
        colorAnuncio = vivos > 0 ? UI_POSITIVO : UI_PELIGRO;
    }

    if (anuncio != nullptr)
    {
        Vector2 p = GetWorldToScreen({ 0.0f, 0.6f, -5.6f }, camara);
        DibujarTextoMundoContornoUI(anuncio, p.x, p.y, UI_TEXTO_GRANDE, colorAnuncio, alfaAnuncio);
    }

    // HUD comun (Minigames/HudMinijuego): solo lee el estado.
    DatosHudMinijuego hud;
    PrepararDatosHudMinijuego(
        hud,
        "SECUENCIA NEÓN",
        "MIRÁ LA SECUENCIA DE FLECHAS Y REPETILA EN ORDEN. SI TE EQUIVOCÁS, QUEDÁS FUERA.",
        resultado
    );

    hud.fase = fase == FASE_SECUENCIA_PREPARACION
        ? HUD_MINIJUEGO_CUENTA
        : (fase == FASE_SECUENCIA_TERMINADO ? HUD_MINIJUEGO_FIN : HUD_MINIJUEGO_JUGANDO);
    hud.cuentaRestante = tiempoPreparacion;
    hud.finPorTiempo = false;

    bool respondiendo = fase == FASE_SECUENCIA_RESPONDIENDO;
    bool rondaEnCurso = respondiendo || fase == FASE_SECUENCIA_RESOLUCION;

    if (respondiendo)
    {
        // Reloj de la respuesta de esta ronda.
        hud.reloj = HUD_RELOJ_CUENTA_ATRAS;
        hud.tiempoRestante = tiempoRespuesta > 0.0f ? tiempoRespuesta : 0.0f;
        hud.tiempoTotal = 1.5f + cantidadPulsos * 0.78f;
        hud.segundosAlerta = 3.0f;
    }
    else if (fase == FASE_SECUENCIA_MOSTRANDO)
    {
        int mostrados = indiceMuestra + (pulsoVisible ? 1 : 0);
        hud.reloj = HUD_RELOJ_MEDIDOR;
        hud.etiquetaMedidor = "PULSOS";
        std::snprintf(hud.valorMedidor, sizeof(hud.valorMedidor), "%d/%d", mostrados, cantidadPulsos);
        hud.progresoMedidor = cantidadPulsos > 0 ? (float)mostrados / (float)cantidadPulsos : 0.0f;
    }
    else
    {
        int ronda = numeroRonda > 0 ? numeroRonda : 1;
        hud.reloj = HUD_RELOJ_MEDIDOR;
        hud.etiquetaMedidor = "RONDA";
        std::snprintf(hud.valorMedidor, sizeof(hud.valorMedidor), "%d/%d", ronda, RONDAS_MAXIMAS_SECUENCIA);
        hud.progresoMedidor = (float)ronda / (float)RONDAS_MAXIMAS_SECUENCIA;
    }

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        JugadorHudMinijuego& jugador = hud.jugadores[i];
        if (!jugador.visible) continue;

        const EstadoJugadorSecuenciaNeon& estado = jugadores[i];

        // Quien falla en esta ronda queda «FALLÓ» hasta que empieza la
        // siguiente; ahi la etiqueta se descuelga con «FUERA».
        bool falloAhora = rondaEnCurso && estado.falloEstaRonda;
        jugador.eliminado = estado.eliminado && !falloAhora;

        // Avance neutro de la ronda: cuantos pulsos lleva, nunca cuales.
        // Al terminar, la etiqueta pasa a mostrar las rondas superadas.
        int avance = rondaEnCurso ? estado.indiceRespuesta : 0;
        int total = cantidadPulsos;
        if (fase == FASE_SECUENCIA_TERMINADO)
        {
            avance = estado.rondasSuperadas;
            total = RONDAS_MAXIMAS_SECUENCIA;
        }
        std::snprintf(jugador.valor, sizeof(jugador.valor), "%d/%d", avance, total);
        jugador.valorNumerico = estado.aciertosTotales;
        jugador.progreso = total > 0 ? (float)avance / (float)total : 0.0f;

        if (falloAhora)
        {
            std::snprintf(jugador.estado, sizeof(jugador.estado), "FALLÓ");
        }
        else if (rondaEnCurso && estado.completoRonda)
        {
            std::snprintf(jugador.estado, sizeof(jugador.estado), "LISTO");
        }

        std::snprintf(
            jugador.detalleFinal,
            sizeof(jugador.detalleFinal),
            estado.rondasSuperadas == 1 ? "%d RONDA" : "%d RONDAS",
            estado.rondasSuperadas
        );

        if (!participantes[i].esBot)
        {
            TextoControlesSecuencia(participantes[i], jugador.controles, (int)sizeof(jugador.controles));
        }
    }

    // Tablon final: con las mismas rondas decide quien sigue en pie y
    // despues los aciertos totales (PuntuacionJugadorSecuencia).
    if (hud.fase == HUD_MINIJUEGO_FIN && resultado.estado == RESULTADO_MINIJUEGO_FINALIZADO)
    {
        for (int i = 0; i < MAX_PARTICIPANTES; i++)
        {
            if (!hud.jugadores[i].visible) continue;

            for (int j = 0; j < MAX_PARTICIPANTES; j++)
            {
                if (j == i || !hud.jugadores[j].visible) continue;
                if (jugadores[j].rondasSuperadas != jugadores[i].rondasSuperadas) continue;
                if (resultado.participantes[j].posicionFinal == resultado.participantes[i].posicionFinal) continue;

                if (jugadores[i].eliminado != jugadores[j].eliminado)
                {
                    std::snprintf(
                        hud.jugadores[i].detalleDesempate,
                        sizeof(hud.jugadores[i].detalleDesempate),
                        "%s",
                        jugadores[i].eliminado ? "FUERA" : "EN PIE"
                    );
                    hud.notaFinal = "MISMAS RONDAS: QUEDA ARRIBA QUIEN SIGUE EN PIE";
                }
                else
                {
                    std::snprintf(
                        hud.jugadores[i].detalleDesempate,
                        sizeof(hud.jugadores[i].detalleDesempate),
                        jugadores[i].aciertosTotales == 1 ? "%d ACIERTO" : "%d ACIERTOS",
                        jugadores[i].aciertosTotales
                    );
                    if (hud.notaFinal == nullptr)
                    {
                        hud.notaFinal = "MISMAS RONDAS: DESEMPATAN LOS ACIERTOS TOTALES";
                    }
                }
                break;
            }
        }
    }

    DibujarHudMinijuego(hud, participantes);
}


const ResultadoMinijuego&
MinijuegoSecuenciaNeon::ObtenerResultado() const
{
    return resultado;
}
