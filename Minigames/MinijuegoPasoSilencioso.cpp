#include "Minigames/MinijuegoPasoSilencioso.h"

#include "Minigames/AudioMinijuegos.h"
#include "Minigames/EfectosVisualesMinijuegos.h"
#include "Minigames/HudMinijuego.h"
#include "Minigames/ModeloJugadorCompartido.h"
#include "Minigames/SombrasRetro.h"
#include "Minigames/TiposMinijuegos.h"
#include "Systems/CalidadGrafica.h"
#include "Systems/Input.h"
#include "UI/EstiloUI.h"

#include "rlgl.h"

#include <cmath>
#include <cstdio>

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


// Efectos solo visuales / de audio (definidos con el dibujo, al final).
static void ActualizarVisualPaso(
    MinijuegoPasoSilencioso& minijuego,
    const Participante participantes[],
    float deltaTime
);


void MinijuegoPasoSilencioso::Inicializar()
{
    resultado = {};
    resultado.formato = FORMATO_MINIJUEGO_INDIVIDUAL;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        jugadores[i] = {};
    }

    // Vista desde la entrada del salon: los cuatro pasillos de losas y,
    // al fondo, el gigante sentado frente a la logia abierta a la noche.
    camara.position = { 0.0f, 11.0f, 20.0f };
    camara.target = { 0.0f, 2.16f, -2.0f };
    camara.up = { 0.0f, 1.0f, 0.0f };
    camara.fovy = 46.0f;
    camara.projection = CAMERA_PERSPECTIVE;

    fase = FASE_PASO_PREPARACION;
    centinelaAlerta = false;
    tiempoPreparacion = DURACION_PREPARACION_PASO;
    tiempoEstadoCentinela = NuevaDuracionEstadoCentinela(false);
    tiempoCarrera = DURACION_CARRERA_PASO;
    tiempoAnimacion = 0.0f;

    despertar = 0.0f;
    giroCabeza = 0.0f;
    tiempoAnuncio = -1.0f;
    tipoAnuncio = 0;
    alertaVista = false;
    avisoVisto = false;
    finVisto = false;

    for (int i = 0; i < MAX_PARTICULAS_PASO; i++)
    {
        particulas[i] = {};
    }
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
    ActualizarVisualPaso(*this, participantes, deltaTime);

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


//==================================================
// EFECTOS VISUALES (sin efecto en la logica)
//==================================================

static const float DURACION_ANUNCIO_PASO = 1.6f;

enum AnuncioPaso
{
    ANUNCIO_PASO_NINGUNO = 0,
    ANUNCIO_PASO_DESPIERTA,
    ANUNCIO_PASO_QUIETOS,
    ANUNCIO_PASO_AVANZA
};

// El gigante se sienta en el estrado del fondo, detras de la meta.
static const float Z_GIGANTE_PASO = -13.0f;
static const float Y_ESTRADO_PASO = 0.6f;

// Cuello del gigante, relativo a su asiento: pivote de la cabeza.
static const Vector3 CUELLO_GIGANTE_PASO = { 0.0f, 3.75f, 0.8f };

// Medidas del salon (solo dibujo; los pasillos coinciden con los carriles).
static const float NAVE_X_PASO = 5.3f;          // borde de la nave elevada
static const float NAVE_Z_FRENTE_PASO = 12.0f;
static const float NAVE_Z_FONDO_PASO = -8.6f;
static const float NIVEL_LATERAL_PASO = -0.4f;  // suelo de las naves laterales
static const float ESTRADO_Z_PASO = -9.8f;      // frente del estrado
static const float MURO_X_PASO = 12.6f;
static const float LOGIA_Z_PASO = -17.0f;       // cara de la logia del fondo
static const float COLUMNAS_X_PASO = 7.0f;
static const float MEDIO_PASILLO_PASO = 1.0f;
static const float Z_META_PASO = -8.05f;
static const float Z_SALIDA_PASO = 7.6f;


static float Limitar01Paso(float valor)
{
    if (valor < 0.0f) return 0.0f;
    if (valor > 1.0f) return 1.0f;
    return valor;
}


// Hash determinista para la decoracion (misma escena en cada partida).
static float Hash01Paso(int a, int b)
{
    unsigned int h = (unsigned int)a * 374761393u + (unsigned int)b * 668265263u;
    h = (h ^ (h >> 13)) * 1274126177u;
    h ^= h >> 16;
    return (float)(h & 0xFFFFu) / 65535.0f;
}


// Azar propio para particulas: no consume el GetRandomValue de los bots.
static float AzarPaso(MinijuegoPasoSilencioso& minijuego, float minimo, float maximo)
{
    unsigned int& s = minijuego.semillaVisual;
    s ^= s << 13;
    s ^= s >> 17;
    s ^= s << 5;
    return minimo + (maximo - minimo) * (float)(s & 0xFFFFFFu) / 16777215.0f;
}


static int CantidadParticulasPaso(int base)
{
    int cantidad = (int)std::lround((float)base * FactorCalidadGrafica(CalidadParticulas()));
    return cantidad < 1 ? 1 : cantidad;
}


static bool EsHumanoPaso(const Participante& participante)
{
    return !(participante.esBot || !participante.conectado);
}


// Pies del jugador en su pasillo (posicion dibujada, suavizada).
static Vector3 PiesJugadorPaso(const MinijuegoPasoSilencioso& minijuego, int indice)
{
    return {
        PosicionXCarrilPaso(indice),
        0.0f,
        PosicionZProgresoPaso(minijuego.jugadores[indice].progresoVisual)
    };
}


static void EmitirParticulaPaso(
    MinijuegoPasoSilencioso& minijuego,
    Vector3 posicion,
    Vector3 velocidad,
    float vida,
    float tamano,
    float gravedad,
    Color color,
    bool brillo
)
{
    for (int i = 0; i < MAX_PARTICULAS_PASO; i++)
    {
        ParticulaPasoSilencioso& p = minijuego.particulas[i];

        if (p.activa)
        {
            continue;
        }

        p.activa = true;
        p.brillo = brillo;
        p.posicion = posicion;
        p.velocidad = velocidad;
        p.vida = vida;
        p.vidaMaxima = vida;
        p.tamano = tamano;
        p.gravedad = gravedad;
        p.color = color;
        return;
    }
}


// Lo vio moverse: polvo de las losas y chispas rojas de la mirada.
static void EfectoCastigoPaso(MinijuegoPasoSilencioso& minijuego, int indice)
{
    Vector3 pies = PiesJugadorPaso(minijuego, indice);
    int polvo = CantidadParticulasPaso(16);

    for (int k = 0; k < polvo; k++)
    {
        float angulo = AzarPaso(minijuego, 0.0f, 6.2832f);
        float rapidez = AzarPaso(minijuego, 1.0f, 2.6f);

        EmitirParticulaPaso(
            minijuego,
            { pies.x + std::cos(angulo) * 0.3f, 0.12f, pies.z + std::sin(angulo) * 0.3f },
            { std::cos(angulo) * rapidez, AzarPaso(minijuego, 0.8f, 2.2f), std::sin(angulo) * rapidez },
            AzarPaso(minijuego, 0.45f, 0.85f),
            AzarPaso(minijuego, 0.06f, 0.13f),
            6.0f,
            Color{ 152, 148, 164, 255 },
            false
        );
    }

    int chispas = CantidadParticulasPaso(12);

    for (int k = 0; k < chispas; k++)
    {
        float angulo = AzarPaso(minijuego, 0.0f, 6.2832f);

        EmitirParticulaPaso(
            minijuego,
            { pies.x, AzarPaso(minijuego, 0.5f, 1.3f), pies.z },
            { std::cos(angulo) * 2.2f, AzarPaso(minijuego, 0.5f, 2.5f), std::sin(angulo) * 2.2f },
            AzarPaso(minijuego, 0.35f, 0.6f),
            AzarPaso(minijuego, 0.08f, 0.14f),
            3.0f,
            Color{ 255, 86, 64, 255 },
            true
        );
    }

    minijuego.jugadores[indice].sacudida = 0.45f;
}


// Llego al pie del gigante: chispas doradas que suben.
static void EfectoMetaPaso(MinijuegoPasoSilencioso& minijuego, int indice)
{
    Vector3 pies = PiesJugadorPaso(minijuego, indice);
    int cantidad = CantidadParticulasPaso(26);

    for (int k = 0; k < cantidad; k++)
    {
        float angulo = AzarPaso(minijuego, 0.0f, 6.2832f);
        float radio = AzarPaso(minijuego, 0.2f, 0.7f);

        EmitirParticulaPaso(
            minijuego,
            { pies.x + std::cos(angulo) * radio, AzarPaso(minijuego, 0.1f, 0.6f), pies.z + std::sin(angulo) * radio },
            { std::cos(angulo) * 0.6f, AzarPaso(minijuego, 1.8f, 3.6f), std::sin(angulo) * 0.6f },
            AzarPaso(minijuego, 0.7f, 1.3f),
            AzarPaso(minijuego, 0.08f, 0.16f),
            -0.4f,
            Color{ 255, 214, 110, 255 },
            true
        );
    }
}


// Polvo y piedritas que caen de los hombros cuando el gigante se mueve.
static void EmitirPolvoGigantePaso(MinijuegoPasoSilencioso& minijuego, int base)
{
    int cantidad = CantidadParticulasPaso(base);

    for (int k = 0; k < cantidad; k++)
    {
        float lado = AzarPaso(minijuego, 0.0f, 1.0f) < 0.5f ? -1.0f : 1.0f;

        EmitirParticulaPaso(
            minijuego,
            {
                lado * AzarPaso(minijuego, 1.9f, 3.9f),
                Y_ESTRADO_PASO + AzarPaso(minijuego, 4.4f, 5.2f),
                Z_GIGANTE_PASO + AzarPaso(minijuego, -0.9f, 0.6f)
            },
            { lado * AzarPaso(minijuego, 0.1f, 0.6f), AzarPaso(minijuego, -0.2f, 0.6f), AzarPaso(minijuego, 0.0f, 0.5f) },
            AzarPaso(minijuego, 0.8f, 1.4f),
            AzarPaso(minijuego, 0.05f, 0.12f),
            7.0f,
            Color{ 132, 136, 128, 255 },
            false
        );
    }
}


static void EmitirConfetiPaso(
    MinijuegoPasoSilencioso& minijuego,
    const Participante participantes[],
    int indice,
    int base
)
{
    Vector3 pies = PiesJugadorPaso(minijuego, indice);
    int cantidad = CantidadParticulasPaso(base);
    const Color colores[3] =
    {
        participantes[indice].color,
        Color{ 255, 214, 110, 255 },
        Color{ 236, 240, 255, 255 }
    };

    for (int k = 0; k < cantidad; k++)
    {
        float angulo = AzarPaso(minijuego, 0.0f, 6.2832f);
        float rapidez = AzarPaso(minijuego, 0.6f, 2.0f);

        EmitirParticulaPaso(
            minijuego,
            { pies.x, 2.2f, pies.z },
            { std::cos(angulo) * rapidez, AzarPaso(minijuego, 2.5f, 4.5f), std::sin(angulo) * rapidez },
            AzarPaso(minijuego, 1.4f, 2.2f),
            AzarPaso(minijuego, 0.07f, 0.12f),
            4.0f,
            colores[k % 3],
            false
        );
    }
}


static void ActualizarVisualPaso(
    MinijuegoPasoSilencioso& minijuego,
    const Participante participantes[],
    float deltaTime
)
{
    MinijuegoPasoSilencioso& m = minijuego;
    float dt = deltaTime;

    // El gigante se despierta rapido y se vuelve a dormir despacio.
    float objetivo = m.centinelaAlerta ? 1.0f : (m.avisoPrevio ? 0.38f : 0.0f);
    float rapidez = m.centinelaAlerta ? 7.0f : (m.avisoPrevio ? 4.0f : 2.0f);
    m.despertar += (objetivo - m.despertar) * Limitar01Paso(dt * rapidez);

    float giroObjetivo = m.centinelaAlerta
        ? std::sin(m.tiempoAnimacion * 2.3f) * 24.0f
        : 0.0f;
    m.giroCabeza += (giroObjetivo - m.giroCabeza) * Limitar01Paso(dt * 5.0f);

    // Cambios del centinela: anuncio, polvo y un temblor sordo.
    if (m.fase == FASE_PASO_CARRERA)
    {
        if (m.centinelaAlerta && !m.alertaVista)
        {
            m.tipoAnuncio = ANUNCIO_PASO_QUIETOS;
            m.tiempoAnuncio = 0.0f;
            EmitirPolvoGigantePaso(m, 18);
            ActivarTemblorCamaraGeneral(0.07f, 0.28f);
        }
        else if (!m.centinelaAlerta && m.alertaVista)
        {
            m.tipoAnuncio = ANUNCIO_PASO_AVANZA;
            m.tiempoAnuncio = 0.0f;
        }
        else if (m.avisoPrevio && !m.avisoVisto)
        {
            m.tipoAnuncio = ANUNCIO_PASO_DESPIERTA;
            m.tiempoAnuncio = 0.0f;
            EmitirPolvoGigantePaso(m, 6);
        }
    }

    m.alertaVista = m.centinelaAlerta;
    m.avisoVisto = m.avisoPrevio;

    if (m.tiempoAnuncio >= 0.0f)
    {
        m.tiempoAnuncio += dt;

        if (m.tiempoAnuncio > DURACION_ANUNCIO_PASO || m.fase == FASE_PASO_TERMINADO)
        {
            m.tiempoAnuncio = -1.0f;
        }
    }

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!m.resultado.participantes[i].participo)
        {
            continue;
        }

        EstadoJugadorPasoSilencioso& j = m.jugadores[i];

        // Andar en puntas de pie mientras el progreso sube.
        bool avanza = j.progreso > j.progresoAnterior + 0.00001f;
        j.marcha += ((avanza ? 1.0f : 0.0f) - j.marcha) * Limitar01Paso(dt * 10.0f);
        j.fasePaso += dt * 9.0f * j.marcha;
        j.progresoAnterior = j.progreso;

        // Quien ya llego (o al terminar) sigue acomodando su posicion.
        if (j.llegoMeta || m.fase == FASE_PASO_TERMINADO)
        {
            j.progresoVisual += (j.progreso - j.progresoVisual) * Limitar01Paso(dt * 9.0f);
        }

        j.sacudida = j.sacudida > dt ? j.sacudida - dt : 0.0f;
        j.destelloMeta = j.destelloMeta > dt ? j.destelloMeta - dt : 0.0f;

        if (j.penalizaciones > j.penalizacionesVistas)
        {
            j.penalizacionesVistas = j.penalizaciones;
            EfectoCastigoPaso(m, i);

            if (EsHumanoPaso(participantes[i]))
            {
                ActivarTemblorCamaraGeneral(0.12f, 0.22f);
            }
        }

        if (j.llegoMeta && !j.metaVista)
        {
            j.metaVista = true;
            j.destelloMeta = 1.2f;
            EfectoMetaPaso(m, i);
            ReproducirSonidoMinijuego(m.audio, SONIDO_ACIERTO);
        }
    }

    // Fin: confeti sobre el o los ganadores, en tandas.
    if (m.fase == FASE_PASO_TERMINADO)
    {
        bool primeraVez = !m.finVisto;
        m.finVisto = true;

        bool tanda = primeraVez || std::fmod(m.tiempoAnimacion, 0.5f) < dt;

        if (tanda)
        {
            for (int i = 0; i < MAX_PARTICIPANTES; i++)
            {
                if (
                    m.resultado.participantes[i].participo &&
                    m.resultado.participantes[i].posicionFinal == 1
                )
                {
                    EmitirConfetiPaso(m, participantes, i, primeraVez ? 36 : 8);
                }
            }
        }
    }

    for (int i = 0; i < MAX_PARTICULAS_PASO; i++)
    {
        ParticulaPasoSilencioso& p = m.particulas[i];

        if (!p.activa)
        {
            continue;
        }

        p.vida -= dt;

        if (p.vida <= 0.0f)
        {
            p.activa = false;
            continue;
        }

        float freno = 1.0f - Limitar01Paso(dt * 1.4f);
        p.velocidad.x *= freno;
        p.velocidad.z *= freno;
        p.velocidad.y -= p.gravedad * dt;
        p.posicion.x += p.velocidad.x * dt;
        p.posicion.y += p.velocidad.y * dt;
        p.posicion.z += p.velocidad.z * dt;

        // Lo que cae rebota apenas en las losas (o en el estrado).
        float suelo = p.posicion.z < ESTRADO_Z_PASO ? Y_ESTRADO_PASO + 0.03f : 0.03f;

        if (p.posicion.y < suelo && p.velocidad.y < 0.0f && !p.brillo)
        {
            p.posicion.y = suelo;
            p.velocidad.y *= -0.25f;
        }
    }
}


//==================================================
// PALETA, NIEBLA Y PRIMITIVAS
//==================================================
//
// Templo de noche: piedra gris violacea, bronce viejo, tapices granate e
// indigo y una luz de luna azulada que entra por los ventanales. El fuego
// de los braseros pone el calido. Acento: el oro de la meta. Los ojos del
// gigante usan el color del estado (verde dormido, ambar aviso, rojo).

static const Color COLOR_NOCHE_PASO = { 22, 24, 46, 255 };        // niebla interior
static const Color COLOR_BRUMA_PASO = { 74, 88, 140, 255 };       // bruma de luna (afuera)
static const Color COLOR_LOSA_PASO = { 122, 118, 140, 255 };
static const Color COLOR_LOSA_LATERAL_PASO = { 96, 94, 116, 255 };
static const Color COLOR_LOSA_ESTRADO_PASO = { 112, 104, 120, 255 };
static const Color COLOR_JUNTA_PASO = { 46, 44, 62, 255 };
static const Color COLOR_PIEDRA_PASO = { 108, 104, 126, 255 };
static const Color COLOR_PIEDRA_OSCURA_PASO = { 68, 66, 88, 255 };
static const Color COLOR_PIEDRA_CLARA_PASO = { 156, 152, 170, 255 };
static const Color COLOR_GIGANTE_PASO = { 148, 154, 142, 255 };
static const Color COLOR_GIGANTE_OSCURO_PASO = { 100, 106, 100, 255 };
static const Color COLOR_MUSGO_PASO = { 72, 108, 70, 255 };
static const Color COLOR_BRONCE_PASO = { 170, 120, 62, 255 };
static const Color COLOR_BRONCE_OSCURO_PASO = { 104, 72, 44, 255 };
static const Color COLOR_MADERA_PASO = { 92, 58, 40, 255 };
static const Color COLOR_TAPIZ_PASO = { 132, 34, 48, 255 };
static const Color COLOR_TAPIZ_AZUL_PASO = { 42, 50, 112, 255 };
static const Color COLOR_ORO_PASO = { 222, 176, 80, 255 };
static const Color COLOR_LUNA_PASO = { 186, 210, 255, 255 };
static const Color COLOR_ACENTO_PASO = { 255, 206, 96, 255 };

// Ojo de referencia para la niebla (la camara de juego).
static const Vector3 OJO_NIEBLA_PASO = { 0.0f, 11.0f, 20.0f };


static unsigned char CanalPaso(float v)
{
    if (v < 0.0f) return 0;
    if (v > 255.0f) return 255;
    return (unsigned char)v;
}


static Color TonoPaso(Color color, float factor)
{
    return Color{
        CanalPaso((float)color.r * factor),
        CanalPaso((float)color.g * factor),
        CanalPaso((float)color.b * factor),
        color.a
    };
}


// Caras de arriba con luz de luna fria; las laterales, sombra violacea.
static Color SombraPaso(Color color, float factor, bool arriba)
{
    if (arriba)
    {
        return Color{
            CanalPaso((float)color.r * factor * 0.96f),
            CanalPaso((float)color.g * factor * 1.0f),
            CanalPaso((float)color.b * factor * 1.08f),
            color.a
        };
    }

    return Color{
        CanalPaso((float)color.r * factor * 0.94f),
        CanalPaso((float)color.g * factor * 0.94f),
        CanalPaso((float)color.b * factor * 1.04f),
        color.a
    };
}


static Color MezclaPaso(Color a, Color b, float t)
{
    t = Limitar01Paso(t);

    return Color{
        CanalPaso((float)a.r + ((float)b.r - (float)a.r) * t),
        CanalPaso((float)a.g + ((float)b.g - (float)a.g) * t),
        CanalPaso((float)a.b + ((float)b.b - (float)a.b) * t),
        CanalPaso((float)a.a + ((float)b.a - (float)a.a) * t)
    };
}


static float DistanciaOjoPaso(Vector3 p)
{
    float dx = p.x - OJO_NIEBLA_PASO.x;
    float dy = p.y - OJO_NIEBLA_PASO.y;
    float dz = p.z - OJO_NIEBLA_PASO.z;
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}


// Penumbra del salon: lo lejano se hunde en el azul de la noche.
static Color TintePaso(Color color, Vector3 p)
{
    float n = (DistanciaOjoPaso(p) - 15.0f) / 44.0f;
    Color niebla = COLOR_NOCHE_PASO;
    niebla.a = color.a;
    return MezclaPaso(color, niebla, n < 0.0f ? 0.0f : (n > 0.5f ? 0.5f : n));
}


// Afuera: montanas y nubes se aclaran hacia la bruma de la luna.
static Color TinteLejosPaso(Color color, Vector3 p)
{
    float n = (DistanciaOjoPaso(p) - 60.0f) / 260.0f;
    Color bruma = COLOR_BRUMA_PASO;
    bruma.a = color.a;
    return MezclaPaso(color, bruma, n < 0.0f ? 0.0f : (n > 0.6f ? 0.6f : n));
}


static void VerticePaso(Vector3 p, Color color)
{
    rlColor4ub(color.r, color.g, color.b, color.a);
    rlVertex3f(p.x, p.y, p.z);
}


static void QuadPaso(Vector3 a, Vector3 b, Vector3 c, Vector3 d, Color color)
{
    VerticePaso(a, color);
    VerticePaso(b, color);
    VerticePaso(c, color);
    VerticePaso(d, color);
}


// Quad con la penumbra calculada por vertice (suelos y muros grandes).
static void QuadNieblaPaso(Vector3 a, Vector3 b, Vector3 c, Vector3 d, Color color)
{
    VerticePaso(a, TintePaso(color, a));
    VerticePaso(b, TintePaso(color, b));
    VerticePaso(c, TintePaso(color, c));
    VerticePaso(d, TintePaso(color, d));
}


// Caja con caras sombreadas (arriba 1.0, frente 0.78, izquierda 0.70,
// derecha 0.62, fondo 0.54). Respeta la matriz actual.
static void CuboPaso(Vector3 c, Vector3 t, Color color)
{
    float x0 = c.x - t.x * 0.5f, x1 = c.x + t.x * 0.5f;
    float y0 = c.y - t.y * 0.5f, y1 = c.y + t.y * 0.5f;
    float z0 = c.z - t.z * 0.5f, z1 = c.z + t.z * 0.5f;

    rlBegin(RL_QUADS);
    QuadPaso({ x0, y1, z1 }, { x1, y1, z1 }, { x1, y1, z0 }, { x0, y1, z0 }, SombraPaso(color, 1.0f, true));
    QuadPaso({ x0, y0, z1 }, { x1, y0, z1 }, { x1, y1, z1 }, { x0, y1, z1 }, SombraPaso(color, 0.78f, false));
    QuadPaso({ x1, y0, z0 }, { x0, y0, z0 }, { x0, y1, z0 }, { x1, y1, z0 }, SombraPaso(color, 0.54f, false));
    QuadPaso({ x1, y0, z1 }, { x1, y0, z0 }, { x1, y1, z0 }, { x1, y1, z1 }, SombraPaso(color, 0.62f, false));
    QuadPaso({ x0, y0, z0 }, { x0, y0, z1 }, { x0, y1, z1 }, { x0, y1, z0 }, SombraPaso(color, 0.70f, false));
    QuadPaso({ x0, y0, z0 }, { x1, y0, z0 }, { x1, y0, z1 }, { x0, y0, z1 }, SombraPaso(color, 0.45f, false));
    rlEnd();
}


// Igual que CuboPaso pero con la penumbra del salon.
static void CuboNieblaPaso(Vector3 c, Vector3 t, Color color)
{
    CuboPaso(c, t, TintePaso(color, c));
}


// Luz falsa de las caras laterales segun su orientacion (angulo en XZ:
// 0 = +X, 90 = +Z hacia la camara). La luna entra desde la izquierda.
static float LuzLateralPaso(float angulo)
{
    return 0.68f + 0.12f * std::sin(angulo) - 0.10f * std::cos(angulo);
}


// Prisma o tronco de cono de base.y (radio r0) a base.y + alto (r1).
static void PrismaPaso(
    Vector3 base,
    float r0,
    float r1,
    float alto,
    int lados,
    Color color,
    bool estriado = false
)
{
    float yArriba = base.y + alto;
    float paso = 6.2832f / (float)lados;
    Color tapaArriba = SombraPaso(color, 1.0f, true);
    Color tapaAbajo = SombraPaso(color, 0.45f, false);

    rlBegin(RL_QUADS);

    for (int k = 0; k < lados; k++)
    {
        float a0 = paso * (float)k;
        float a1 = a0 + paso;
        float f = LuzLateralPaso(a0 + paso * 0.5f);
        if (estriado && (k & 1)) f *= 0.88f;
        Color cara = SombraPaso(color, f, false);

        float c0 = std::cos(a0), s0 = std::sin(a0);
        float c1 = std::cos(a1), s1 = std::sin(a1);

        QuadPaso(
            { base.x + c0 * r0, base.y, base.z + s0 * r0 },
            { base.x + c1 * r0, base.y, base.z + s1 * r0 },
            { base.x + c1 * r1, yArriba, base.z + s1 * r1 },
            { base.x + c0 * r1, yArriba, base.z + s0 * r1 },
            cara
        );

        if (r1 > 0.001f)
        {
            Vector3 centro = { base.x, yArriba, base.z };
            QuadPaso(centro, { base.x + c1 * r1, yArriba, base.z + s1 * r1 }, { base.x + c0 * r1, yArriba, base.z + s0 * r1 }, centro, tapaArriba);
        }

        if (r0 > 0.001f)
        {
            Vector3 centro = { base.x, base.y, base.z };
            QuadPaso(centro, { base.x + c0 * r0, base.y, base.z + s0 * r0 }, { base.x + c1 * r0, base.y, base.z + s1 * r0 }, centro, tapaAbajo);
        }
    }

    rlEnd();
}


// Corona plana (r0..r1) a la altura c.y.
static void AnilloPaso(Vector3 c, float r0, float r1, Color color, int lados = 28)
{
    rlBegin(RL_QUADS);

    for (int k = 0; k < lados; k++)
    {
        float a0 = 6.2832f * (float)k / (float)lados;
        float a1 = 6.2832f * (float)(k + 1) / (float)lados;

        QuadPaso(
            { c.x + std::cos(a0) * r1, c.y, c.z + std::sin(a0) * r1 },
            { c.x + std::cos(a0) * r0, c.y, c.z + std::sin(a0) * r0 },
            { c.x + std::cos(a1) * r0, c.y, c.z + std::sin(a1) * r0 },
            { c.x + std::cos(a1) * r1, c.y, c.z + std::sin(a1) * r1 },
            color
        );
    }

    rlEnd();
}


// Corona en un plano vertical (mirando a +Z): relieves y discos.
static void AnilloFrentePaso(Vector3 c, float r0, float r1, Color color, int lados = 28)
{
    rlBegin(RL_QUADS);

    for (int k = 0; k < lados; k++)
    {
        float a0 = 6.2832f * (float)k / (float)lados;
        float a1 = 6.2832f * (float)(k + 1) / (float)lados;

        QuadPaso(
            { c.x + std::cos(a0) * r1, c.y + std::sin(a0) * r1, c.z },
            { c.x + std::cos(a0) * r0, c.y + std::sin(a0) * r0, c.z },
            { c.x + std::cos(a1) * r0, c.y + std::sin(a1) * r0, c.z },
            { c.x + std::cos(a1) * r1, c.y + std::sin(a1) * r1, c.z },
            color
        );
    }

    rlEnd();
}


// Disco con borde que se apaga (u y v definen el plano).
static void ResplandorPaso(Vector3 centro, Vector3 u, Vector3 v, float radio, Color color, float estirado = 1.0f)
{
    const int LADOS = 18;
    rlBegin(RL_TRIANGLES);

    for (int k = 0; k < LADOS; k++)
    {
        float a0 = 6.2832f * (float)k / (float)LADOS;
        float a1 = 6.2832f * (float)(k + 1) / (float)LADOS;
        float c0 = std::cos(a0) * radio * estirado, s0 = std::sin(a0) * radio;
        float c1 = std::cos(a1) * radio * estirado, s1 = std::sin(a1) * radio;

        rlColor4ub(color.r, color.g, color.b, color.a);
        rlVertex3f(centro.x, centro.y, centro.z);
        rlColor4ub(color.r, color.g, color.b, 0);
        rlVertex3f(centro.x + u.x * c0 + v.x * s0, centro.y + u.y * c0 + v.y * s0, centro.z + u.z * c0 + v.z * s0);
        rlColor4ub(color.r, color.g, color.b, 0);
        rlVertex3f(centro.x + u.x * c1 + v.x * s1, centro.y + u.y * c1 + v.y * s1, centro.z + u.z * c1 + v.z * s1);
    }

    rlEnd();
}


static void ResplandorSueloPaso(Vector3 c, float radio, Color color, float estirado = 1.0f)
{
    ResplandorPaso(c, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, radio, color, estirado);
}


// Halo que mira a la camara (plano inclinado como la vista).
static void ResplandorFrentePaso(Vector3 c, float radio, Color color)
{
    ResplandorPaso(c, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.91f, 0.41f }, radio, color);
}


static Color ConAlfaPaso(Color color, float alfa)
{
    color.a = CanalPaso(alfa);
    return color;
}


//==================================================
// CIELO Y HORIZONTE (lo que se ve por la logia)
//==================================================

// Cielo nocturno (2D, antes del 3D): indigo arriba, bruma de luna en el
// horizonte, estrellas que titilan y una luna llena con nubes finas.
static void DibujarCieloPaso(const Camera3D& vista, float t, bool detalle)
{
    int ancho = GetScreenWidth();
    int alto = GetScreenHeight();

    float horizonte = GetWorldToScreen({ 0.0f, -18.0f, -150.0f }, vista).y;
    if (horizonte < (float)alto * 0.12f) horizonte = (float)alto * 0.12f;
    if (horizonte > (float)alto * 0.60f) horizonte = (float)alto * 0.60f;

    int h1 = (int)(horizonte * 0.45f);
    int h2 = (int)horizonte;

    DrawRectangleGradientV(0, 0, ancho, h1, Color{ 8, 10, 30, 255 }, Color{ 22, 28, 66, 255 });
    DrawRectangleGradientV(0, h1, ancho, h2 - h1, Color{ 22, 28, 66, 255 }, Color{ 76, 92, 146, 255 });
    DrawRectangleGradientV(0, h2, ancho, alto - h2, Color{ 76, 92, 146, 255 }, Color{ 28, 30, 58, 255 });

    if (!detalle)
    {
        return;
    }

    // Estrellas: mas densas arriba, titilan cada una a su ritmo.
    for (int k = 0; k < 90; k++)
    {
        float x = Hash01Paso(k, 1) * (float)ancho;
        float y = Hash01Paso(k, 2) * Hash01Paso(k, 2) * horizonte * 0.92f;
        float brillo = 0.45f + 0.55f * (0.5f + 0.5f * std::sin(t * (1.2f + 2.0f * Hash01Paso(k, 3)) + (float)k));
        float r = Hash01Paso(k, 4) < 0.15f ? 1.8f : 1.1f;
        DrawCircleV({ x, y }, r, Fade(Color{ 220, 230, 255, 255 }, brillo * (1.0f - y / (horizonte + 1.0f) * 0.6f)));
    }

    // Luna llena, baja sobre las montanas de la izquierda.
    Vector2 luna = GetWorldToScreen({ 15.0f, -9.0f, -150.0f }, vista);
    float radio = (float)alto * 0.05f;
    float pulso = 1.0f + 0.02f * std::sin(t * 0.7f);

    DrawCircleGradient(luna, radio * 6.0f * pulso, Fade(Color{ 150, 176, 240, 255 }, 0.22f), Fade(Color{ 120, 140, 210, 255 }, 0.0f));
    DrawCircleGradient(luna, radio * 2.2f, Fade(Color{ 214, 228, 255, 255 }, 0.45f), Fade(Color{ 190, 210, 255, 255 }, 0.0f));
    DrawCircleV(luna, radio, Color{ 236, 240, 252, 255 });
    DrawCircleV({ luna.x - radio * 0.32f, luna.y - radio * 0.18f }, radio * 0.26f, Color{ 206, 214, 236, 255 });
    DrawCircleV({ luna.x + radio * 0.30f, luna.y + radio * 0.28f }, radio * 0.20f, Color{ 212, 220, 240, 255 });
    DrawCircleV({ luna.x + radio * 0.18f, luna.y - radio * 0.42f }, radio * 0.12f, Color{ 214, 222, 242, 255 });
    DrawCircleV({ luna.x - radio * 0.42f, luna.y + radio * 0.36f }, radio * 0.10f, Color{ 216, 224, 244, 255 });

    // Velos de nube que cruzan delante de la luna.
    for (int k = 0; k < 6; k++)
    {
        float vel = 6.0f + 5.0f * Hash01Paso(k, 20);
        float x = std::fmod(Hash01Paso(k, 21) * (float)(ancho + 700) + t * vel, (float)(ancho + 700)) - 350.0f;
        float y = luna.y + (Hash01Paso(k, 22) - 0.5f) * radio * 5.0f;
        float largo = 140.0f + 200.0f * Hash01Paso(k, 23);
        DrawEllipse((int)x, (int)y, largo, 5.0f + 4.0f * Hash01Paso(k, 24), Fade(Color{ 96, 112, 170, 255 }, 0.55f));
        DrawEllipse((int)(x + largo * 0.25f), (int)(y - 4.0f), largo * 0.5f, 3.0f, Fade(Color{ 150, 166, 220, 255 }, 0.35f));
    }
}


// Cordillera como silueta en un plano Z: pie oscuro, cumbre iluminada
// por la luna (la ladera que mira a la izquierda queda mas clara).
static void CordilleraPaso(
    float z,
    float x0,
    float x1,
    float yBase,
    float yMin,
    float yMax,
    int puntos,
    int semilla,
    Color pie,
    Color cumbre
)
{
    float ancho = (x1 - x0) / (float)puntos;

    rlBegin(RL_QUADS);

    for (int k = 0; k < puntos; k++)
    {
        float ha = yMin + (yMax - yMin) * (0.55f * Hash01Paso(k, semilla) + 0.45f * (0.5f + 0.5f * std::sin((float)k * 0.9f + (float)semilla)));
        float hb = yMin + (yMax - yMin) * (0.55f * Hash01Paso(k + 1, semilla) + 0.45f * (0.5f + 0.5f * std::sin((float)(k + 1) * 0.9f + (float)semilla)));
        float xa = x0 + ancho * (float)k;
        float xb = xa + ancho;

        // La ladera que baja hacia la derecha mira a la luna.
        Color cima = hb < ha ? cumbre : MezclaPaso(cumbre, pie, 0.35f);

        Vector3 a = { xa, yBase, z };
        Vector3 b = { xb, yBase, z };
        Vector3 c = { xb, hb, z };
        Vector3 d = { xa, ha, z };

        VerticePaso(a, TinteLejosPaso(pie, a));
        VerticePaso(b, TinteLejosPaso(pie, b));
        VerticePaso(c, TinteLejosPaso(cima, c));
        VerticePaso(d, TinteLejosPaso(cima, d));
    }

    rlEnd();
}


// Banco de nubes: lomos redondeados en un plano Z que se mecen despacio.
static void BancoNubesPaso(float z, float yLomo, float x0, float x1, float radio, Color color, float t, int semilla)
{
    Color claro = TinteLejosPaso(MezclaPaso(color, COLOR_LUNA_PASO, 0.25f), { 0.0f, yLomo, z });
    Color oscuro = TinteLejosPaso(TonoPaso(color, 0.7f), { 0.0f, yLomo - 20.0f, z });
    float paso = radio * 1.3f;
    int cantidad = (int)((x1 - x0) / paso) + 1;

    for (int k = 0; k < cantidad; k++)
    {
        float r = radio * (0.7f + 0.6f * Hash01Paso(k, semilla));
        float cx = x0 + paso * (float)k + std::sin(t * 0.07f + (float)k * 1.3f) * radio * 0.25f;
        float cy = yLomo + (Hash01Paso(k, semilla + 1) - 0.5f) * radio * 0.4f;

        // Lomo: abanico de media luna.
        rlBegin(RL_TRIANGLES);
        const int LADOS = 10;
        for (int s = 0; s < LADOS; s++)
        {
            float a0 = 3.1416f * (float)s / (float)LADOS;
            float a1 = 3.1416f * (float)(s + 1) / (float)LADOS;
            VerticePaso({ cx, cy, z }, MezclaPaso(claro, oscuro, 0.35f));
            VerticePaso({ cx + std::cos(a1) * r, cy + std::sin(a1) * r * 0.7f, z }, claro);
            VerticePaso({ cx + std::cos(a0) * r, cy + std::sin(a0) * r * 0.7f, z }, claro);
        }
        rlEnd();

        // Cuerpo de la nube hasta muy abajo.
        rlBegin(RL_QUADS);
        VerticePaso({ cx - r, cy, z }, MezclaPaso(claro, oscuro, 0.35f));
        VerticePaso({ cx + r, cy, z }, MezclaPaso(claro, oscuro, 0.35f));
        VerticePaso({ cx + r, cy - 30.0f, z }, oscuro);
        VerticePaso({ cx - r, cy - 30.0f, z }, oscuro);
        rlEnd();
    }
}


// Pagoda lejana en una cumbre, con una ventana encendida.
static void PagodaLejanaPaso(Vector3 base, float escala)
{
    Color muro = TinteLejosPaso(Color{ 40, 40, 66, 255 }, base);
    Color techo = TinteLejosPaso(Color{ 28, 28, 50, 255 }, base);

    // Meseta de roca que sostiene la pagoda.
    CuboPaso({ base.x, base.y - 8.0f, base.z + 0.4f }, { 7.0f * escala, 16.0f, 1.5f }, TinteLejosPaso(Color{ 30, 34, 60, 255 }, base));

    for (int piso = 0; piso < 3; piso++)
    {
        float ancho = (2.6f - 0.6f * (float)piso) * escala;
        float y = base.y + 1.6f * escala * (float)piso;
        CuboPaso({ base.x, y + 0.6f * escala, base.z }, { ancho * 0.7f, 1.2f * escala, 0.5f }, muro);
        PrismaPaso({ base.x, y + 1.2f * escala, base.z }, ancho * 0.75f, ancho * 0.2f, 0.45f * escala, 4, techo);
    }

    PrismaPaso({ base.x, base.y + 4.8f * escala, base.z }, 0.12f * escala, 0.0f, 1.0f * escala, 4, techo);
}


static void DibujarExteriorPaso(float t, int paso)
{
    // Montanas en dos planos: la de atras mas clara por la bruma.
    CordilleraPaso(-150.0f, -120.0f, 120.0f, -60.0f, -20.0f, -11.0f, 26 / paso, 7, Color{ 58, 66, 112, 255 }, Color{ 116, 130, 186, 255 });
    BancoNubesPaso(-120.0f, -17.0f, -110.0f, 110.0f, 9.0f, Color{ 70, 80, 130, 255 }, t, 30);
    CordilleraPaso(-82.0f, -70.0f, 70.0f, -40.0f, -10.0f, -5.0f, 22 / paso, 3, Color{ 26, 30, 58, 255 }, Color{ 84, 96, 150, 255 });

    if (paso == 1)
    {
        PagodaLejanaPaso({ -22.6f, -6.5f, -82.5f }, 1.0f);
    }

    BancoNubesPaso(-60.0f, -6.3f, -50.0f, 50.0f, 5.0f, Color{ 56, 64, 108, 255 }, t * 1.4f, 50);
}


//==================================================
// SALON: SUELOS, NAVE Y ESTRADO
//==================================================

// Losas de piedra en una grilla (filas alternadas desplazadas media losa).
// Primero la junta, despues cada losa con su variacion y alguna grieta.
static void LosasPaso(
    float x0,
    float x1,
    float z0,
    float z1,
    float y,
    float anchoLosa,
    float largoLosa,
    Color color,
    int semilla
)
{
    rlBegin(RL_QUADS);
    QuadNieblaPaso({ x0, y, z1 }, { x1, y, z1 }, { x1, y, z0 }, { x0, y, z0 }, COLOR_JUNTA_PASO);

    const float JUNTA = 0.035f;
    float yLosa = y + 0.008f;
    int filas = (int)std::ceil((z1 - z0) / largoLosa);

    for (int f = 0; f < filas; f++)
    {
        float za = z0 + largoLosa * (float)f;
        float zb = za + largoLosa > z1 ? z1 : za + largoLosa;
        float desplazamiento = (f & 1) ? anchoLosa * 0.5f : 0.0f;
        float xa = x0;
        int c = 0;

        while (xa < x1 - 0.01f)
        {
            float xb = (c == 0 && desplazamiento > 0.0f) ? xa + desplazamiento : xa + anchoLosa;
            if (xb > x1) xb = x1;

            float h = Hash01Paso(f * 37 + c, semilla);
            float brillo = 0.90f + 0.18f * h;
            Color losa = SombraPaso(TonoPaso(color, brillo), 1.0f, true);

            Vector3 a = { xa + JUNTA, yLosa, zb - JUNTA };
            Vector3 b = { xb - JUNTA, yLosa, zb - JUNTA };
            Vector3 cc = { xb - JUNTA, yLosa, za + JUNTA };
            Vector3 d = { xa + JUNTA, yLosa, za + JUNTA };
            QuadNieblaPaso(a, b, cc, d, losa);

            // Filo claro del lado de la luna y grietas finas en algunas.
            QuadNieblaPaso(
                { xa + JUNTA, yLosa + 0.002f, zb - JUNTA },
                { xa + JUNTA + 0.06f, yLosa + 0.002f, zb - JUNTA },
                { xa + JUNTA + 0.06f, yLosa + 0.002f, za + JUNTA },
                { xa + JUNTA, yLosa + 0.002f, za + JUNTA },
                TonoPaso(losa, 1.12f)
            );

            if (Hash01Paso(f * 37 + c, semilla + 9) < 0.16f)
            {
                float gx = xa + (xb - xa) * (0.3f + 0.4f * h);
                Color grieta = TonoPaso(losa, 0.62f);
                QuadNieblaPaso(
                    { gx - 0.02f, yLosa + 0.003f, za + JUNTA + 0.1f },
                    { gx + 0.02f, yLosa + 0.003f, za + JUNTA + 0.1f },
                    { gx + 0.18f + 0.02f, yLosa + 0.003f, zb - JUNTA - 0.1f },
                    { gx + 0.18f - 0.02f, yLosa + 0.003f, zb - JUNTA - 0.1f },
                    grieta
                );
            }

            xa = xb;
            c++;
        }
    }

    rlEnd();
}


// Nave elevada: cuatro pasillos de losas (uno por carril), divisorias
// con tachas de bronce, bordillo con grecas y el escalon a las naves
// laterales. Los pasillos coinciden con los carriles reales.
static void DibujarNavePaso(const MinijuegoPasoSilencioso& m, const Participante participantes[])
{
    float zMedio = (NAVE_Z_FRENTE_PASO + NAVE_Z_FONDO_PASO) * 0.5f;
    float largo = NAVE_Z_FRENTE_PASO - NAVE_Z_FONDO_PASO;

    // Cuerpo de la nave (sus caras laterales son el escalon).
    CuboNieblaPaso({ 0.0f, NIVEL_LATERAL_PASO * 0.5f, zMedio }, { NAVE_X_PASO * 2.0f, -NIVEL_LATERAL_PASO, largo }, COLOR_PIEDRA_PASO);

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        float xc = PosicionXCarrilPaso(i);
        LosasPaso(xc - MEDIO_PASILLO_PASO, xc + MEDIO_PASILLO_PASO, NAVE_Z_FONDO_PASO, NAVE_Z_FRENTE_PASO, 0.0f, 1.0f, 1.0f, COLOR_LOSA_PASO, 11 + i * 5);
    }

    // Divisorias entre pasillos con tachas de bronce.
    for (int d = 0; d < 3; d++)
    {
        float xd = (PosicionXCarrilPaso(d) + PosicionXCarrilPaso(d + 1)) * 0.5f;
        CuboNieblaPaso({ xd, 0.025f, zMedio }, { 0.4f, 0.05f, largo }, COLOR_PIEDRA_OSCURA_PASO);

        for (float z = NAVE_Z_FONDO_PASO + 0.75f; z < NAVE_Z_FRENTE_PASO; z += 1.5f)
        {
            CuboNieblaPaso({ xd, 0.065f, z }, { 0.13f, 0.035f, 0.13f }, COLOR_BRONCE_PASO);
        }
    }

    // Bordillo de piedra clara con grecas y moldura hacia afuera.
    for (int lado = -1; lado <= 1; lado += 2)
    {
        float s = (float)lado;
        float xb = s * (NAVE_X_PASO - 0.35f);
        CuboNieblaPaso({ xb, -0.165f, zMedio }, { 0.7f, 0.47f, largo }, COLOR_PIEDRA_CLARA_PASO);
        CuboNieblaPaso({ s * (NAVE_X_PASO + 0.05f), -0.08f, zMedio }, { 0.14f, 0.12f, largo }, COLOR_PIEDRA_PASO);

        rlBegin(RL_QUADS);
        for (float z = NAVE_Z_FONDO_PASO + 0.2f; z < NAVE_Z_FRENTE_PASO - 0.3f; z += 0.7f)
        {
            Color greca = TonoPaso(COLOR_PIEDRA_CLARA_PASO, 0.66f);
            QuadNieblaPaso({ xb - 0.18f, 0.072f, z + 0.42f }, { xb + 0.18f, 0.072f, z + 0.42f }, { xb + 0.18f, 0.072f, z + 0.34f }, { xb - 0.18f, 0.072f, z + 0.34f }, greca);
            QuadNieblaPaso({ xb - 0.18f, 0.072f, z + 0.34f }, { xb - 0.1f, 0.072f, z + 0.34f }, { xb - 0.1f, 0.072f, z }, { xb - 0.18f, 0.072f, z }, greca);
            QuadNieblaPaso({ xb + 0.1f, 0.072f, z + 0.42f }, { xb + 0.18f, 0.072f, z + 0.42f }, { xb + 0.18f, 0.072f, z + 0.12f }, { xb + 0.1f, 0.072f, z + 0.12f }, greca);
        }
        rlEnd();
    }

    // Marcas de avance (un cuarto, la mitad, tres cuartos): surcos con runas.
    for (int q = 1; q <= 3; q++)
    {
        float z = PosicionZProgresoPaso((float)q * 0.25f);

        for (int i = 0; i < MAX_PARTICIPANTES; i++)
        {
            float xc = PosicionXCarrilPaso(i);
            rlBegin(RL_QUADS);
            QuadNieblaPaso({ xc - 0.9f, 0.014f, z + 0.05f }, { xc + 0.9f, 0.014f, z + 0.05f }, { xc + 0.9f, 0.014f, z - 0.05f }, { xc - 0.9f, 0.014f, z - 0.05f }, COLOR_JUNTA_PASO);
            for (int r = 0; r < q; r++)
            {
                float xr = xc + 0.55f + 0.14f * (float)r;
                QuadNieblaPaso({ xr - 0.04f, 0.016f, z + 0.2f }, { xr + 0.04f, 0.016f, z + 0.2f }, { xr + 0.04f, 0.016f, z + 0.08f }, { xr - 0.04f, 0.016f, z + 0.08f }, COLOR_BRONCE_PASO);
            }
            rlEnd();
        }
    }

    // Linea de salida de plata y placas con el color de cada jugador.
    CuboNieblaPaso({ 0.0f, 0.02f, Z_SALIDA_PASO }, { 9.2f, 0.03f, 0.14f }, Color{ 182, 190, 210, 255 });

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        float xc = PosicionXCarrilPaso(i);
        CuboNieblaPaso({ xc, 0.03f, Z_SALIDA_PASO + 0.55f }, { 1.4f, 0.05f, 0.5f }, COLOR_PIEDRA_OSCURA_PASO);

        if (m.resultado.participantes[i].participo)
        {
            CuboNieblaPaso({ xc, 0.05f, Z_SALIDA_PASO + 0.55f }, { 1.1f, 0.04f, 0.26f }, TonoPaso(participantes[i].color, 0.85f));
        }
    }

    // Meta: banda de oro incrustada al pie del estrado (el acento).
    CuboNieblaPaso({ 0.0f, 0.02f, Z_META_PASO }, { 9.4f, 0.04f, 0.32f }, COLOR_ACENTO_PASO);
    rlBegin(RL_QUADS);
    for (float x = -4.5f; x < 4.6f; x += 0.6f)
    {
        QuadNieblaPaso({ x, 0.043f, Z_META_PASO + 0.06f }, { x + 0.3f, 0.043f, Z_META_PASO + 0.06f }, { x + 0.3f, 0.043f, Z_META_PASO - 0.06f }, { x, 0.043f, Z_META_PASO - 0.06f }, COLOR_BRONCE_PASO);
    }
    rlEnd();
}


// Naves laterales (mas bajas y oscuras) y el estrado del gigante con sus
// dos escalones.
static void DibujarSuelosPaso()
{
    for (int lado = -1; lado <= 1; lado += 2)
    {
        float s = (float)lado;
        float xa = s < 0.0f ? -MURO_X_PASO : NAVE_X_PASO;
        float xb = s < 0.0f ? -NAVE_X_PASO : MURO_X_PASO;
        LosasPaso(xa, xb, ESTRADO_Z_PASO, NAVE_Z_FRENTE_PASO, NIVEL_LATERAL_PASO, 1.4f, 1.4f, COLOR_LOSA_LATERAL_PASO, 40 + lado);
    }

    // Escalones del estrado (solo frente a la nave).
    CuboNieblaPaso({ 0.0f, 0.1f, NAVE_Z_FONDO_PASO - 0.3f }, { NAVE_X_PASO * 2.0f, 0.2f, 0.6f }, COLOR_PIEDRA_CLARA_PASO);
    CuboNieblaPaso({ 0.0f, 0.2f, NAVE_Z_FONDO_PASO - 0.9f }, { NAVE_X_PASO * 2.0f, 0.4f, 0.6f }, COLOR_PIEDRA_CLARA_PASO);
    CuboNieblaPaso({ 0.0f, 0.205f, NAVE_Z_FONDO_PASO - 0.62f }, { NAVE_X_PASO * 2.0f, 0.03f, 0.06f }, COLOR_BRONCE_OSCURO_PASO);

    // Estrado a lo ancho del salon.
    float zFondo = LOGIA_Z_PASO - 1.2f;
    float largo = ESTRADO_Z_PASO - zFondo;
    CuboNieblaPaso(
        { 0.0f, (NIVEL_LATERAL_PASO + Y_ESTRADO_PASO) * 0.5f, (ESTRADO_Z_PASO + zFondo) * 0.5f },
        { MURO_X_PASO * 2.0f, Y_ESTRADO_PASO - NIVEL_LATERAL_PASO, largo },
        COLOR_PIEDRA_PASO
    );
    CuboNieblaPaso({ 0.0f, Y_ESTRADO_PASO - 0.06f, ESTRADO_Z_PASO + 0.04f }, { MURO_X_PASO * 2.0f, 0.12f, 0.1f }, COLOR_PIEDRA_CLARA_PASO);
    LosasPaso(-MURO_X_PASO, MURO_X_PASO, zFondo, ESTRADO_Z_PASO, Y_ESTRADO_PASO, 1.6f, 1.6f, COLOR_LOSA_ESTRADO_PASO, 77);
}


//==================================================
// SALON: MUROS, VENTANALES, COLUMNAS Y LOGIA
//==================================================

// Ventanal en arco de un muro lateral: vidrio con la luz de la luna,
// celosia de piedra, marco y alfeizar. Mira hacia el centro del salon.
static void VentanalPaso(float lado, float zc, bool detalle)
{
    const float MEDIO = 1.1f;
    const float Y0 = 1.8f;
    const float Y1 = 6.8f;
    float x = lado * (MURO_X_PASO - 0.03f);
    bool lunar = lado < 0.0f;
    Color arriba = lunar ? Color{ 176, 200, 246, 255 } : Color{ 92, 108, 160, 255 };
    Color abajo = lunar ? Color{ 102, 124, 188, 255 } : Color{ 56, 66, 112, 255 };

    rlBegin(RL_QUADS);
    VerticePaso({ x, Y0, zc - MEDIO }, TintePaso(abajo, { x, Y0, zc }));
    VerticePaso({ x, Y0, zc + MEDIO }, TintePaso(abajo, { x, Y0, zc }));
    VerticePaso({ x, Y1, zc + MEDIO }, TintePaso(arriba, { x, Y1, zc }));
    VerticePaso({ x, Y1, zc - MEDIO }, TintePaso(arriba, { x, Y1, zc }));
    rlEnd();

    rlBegin(RL_TRIANGLES);
    for (int k = 0; k < 10; k++)
    {
        float a0 = 3.1416f * (float)k / 10.0f;
        float a1 = 3.1416f * (float)(k + 1) / 10.0f;
        Color c = TintePaso(arriba, { x, Y1, zc });
        VerticePaso({ x, Y1, zc }, c);
        VerticePaso({ x, Y1 + std::sin(a0) * MEDIO, zc + std::cos(a0) * MEDIO }, c);
        VerticePaso({ x, Y1 + std::sin(a1) * MEDIO, zc + std::cos(a1) * MEDIO }, c);
    }
    rlEnd();

    Color piedra = COLOR_PIEDRA_CLARA_PASO;
    float xm = lado * (MURO_X_PASO - 0.1f);

    // Marco y alfeizar.
    CuboNieblaPaso({ xm, Y0 - 0.12f, zc }, { 0.4f, 0.24f, MEDIO * 2.0f + 0.5f }, piedra);
    CuboNieblaPaso({ xm, (Y0 + Y1) * 0.5f, zc - MEDIO - 0.12f }, { 0.3f, Y1 - Y0, 0.24f }, piedra);
    CuboNieblaPaso({ xm, (Y0 + Y1) * 0.5f, zc + MEDIO + 0.12f }, { 0.3f, Y1 - Y0, 0.24f }, piedra);

    for (int k = 0; k < 7; k++)
    {
        float a = 3.1416f * ((float)k + 0.5f) / 7.0f;
        CuboNieblaPaso({ xm, Y1 + std::sin(a) * (MEDIO + 0.12f), zc + std::cos(a) * (MEDIO + 0.12f) }, { 0.3f, 0.34f, 0.34f }, piedra);
    }

    if (!detalle)
    {
        return;
    }

    // Celosia: parteluces, travesanos y una roseta arriba.
    Color celosia = COLOR_PIEDRA_OSCURA_PASO;
    float xc = lado * (MURO_X_PASO - 0.06f);
    CuboNieblaPaso({ xc, (Y0 + Y1) * 0.5f, zc - 0.37f }, { 0.08f, Y1 - Y0, 0.08f }, celosia);
    CuboNieblaPaso({ xc, (Y0 + Y1) * 0.5f, zc + 0.37f }, { 0.08f, Y1 - Y0, 0.08f }, celosia);

    for (int k = 1; k <= 3; k++)
    {
        CuboNieblaPaso({ xc, Y0 + (Y1 - Y0) * (float)k / 4.0f, zc }, { 0.08f, 0.08f, MEDIO * 2.0f }, celosia);
    }

    CuboNieblaPaso({ xc, Y1 + 0.45f, zc }, { 0.08f, 0.5f, 0.08f }, celosia);
    CuboNieblaPaso({ xc, Y1 + 0.45f, zc }, { 0.08f, 0.08f, 0.5f }, celosia);
}


// Muro lateral con hiladas de sillares y ventanales entre columnas.
static void MuroLateralPaso(float lado, bool detalle)
{
    float s = lado;
    float zFrente = NAVE_Z_FRENTE_PASO;
    float zFondo = LOGIA_Z_PASO - 1.2f;
    float yBase = NIVEL_LATERAL_PASO;
    float yTope = 13.0f;

    CuboNieblaPaso({ s * (MURO_X_PASO + 0.4f), (yBase + yTope) * 0.5f, (zFrente + zFondo) * 0.5f }, { 0.8f, yTope - yBase, zFrente - zFondo }, COLOR_PIEDRA_OSCURA_PASO);

    // Zocalo y juntas horizontales de los sillares.
    CuboNieblaPaso({ s * (MURO_X_PASO - 0.05f), yBase + 0.4f, (zFrente + zFondo) * 0.5f }, { 0.2f, 0.8f, zFrente - zFondo }, COLOR_PIEDRA_PASO);

    if (detalle)
    {
        rlBegin(RL_QUADS);
        float x = s * (MURO_X_PASO - 0.005f);
        for (float y = 1.4f; y < yTope; y += 0.9f)
        {
            QuadNieblaPaso({ x, y, zFrente }, { x, y, zFondo }, { x, y + 0.05f, zFondo }, { x, y + 0.05f, zFrente }, TonoPaso(COLOR_PIEDRA_OSCURA_PASO, 0.7f));
        }
        rlEnd();
    }

    const float ventanas[4] = { 5.2f, 0.8f, -3.6f, -8.0f };
    for (int k = 0; k < 4; k++)
    {
        VentanalPaso(s, ventanas[k], detalle);
    }

    // Friso pintado (granate con guardas doradas) y oculos de luna arriba.
    float xf = s * (MURO_X_PASO - 0.04f);
    CuboNieblaPaso({ xf, 9.9f, (zFrente + zFondo) * 0.5f }, { 0.1f, 0.8f, zFrente - zFondo }, COLOR_TAPIZ_PASO);
    CuboNieblaPaso({ xf, 10.34f, (zFrente + zFondo) * 0.5f }, { 0.14f, 0.1f, zFrente - zFondo }, COLOR_ORO_PASO);
    CuboNieblaPaso({ xf, 9.46f, (zFrente + zFondo) * 0.5f }, { 0.14f, 0.1f, zFrente - zFondo }, COLOR_ORO_PASO);

    if (detalle)
    {
        for (float z = zFondo + 1.0f; z < zFrente; z += 1.1f)
        {
            CuboNieblaPaso({ xf, 9.9f, z }, { 0.14f, 0.34f, 0.34f }, COLOR_ORO_PASO);
        }
    }

    Color vidrio = s < 0.0f ? Color{ 168, 194, 244, 255 } : Color{ 86, 100, 154, 255 };
    for (int k = 0; k < 4; k++)
    {
        float z = ventanas[k];
        Vector3 c = { s * (MURO_X_PASO - 0.03f), 11.6f, z };

        rlBegin(RL_TRIANGLES);
        for (int n = 0; n < 16; n++)
        {
            float a0 = 6.2832f * (float)n / 16.0f;
            float a1 = 6.2832f * (float)(n + 1) / 16.0f;
            VerticePaso(c, TintePaso(vidrio, c));
            VerticePaso({ c.x, c.y + std::sin(a0) * 0.7f, c.z + std::cos(a0) * 0.7f }, TintePaso(vidrio, c));
            VerticePaso({ c.x, c.y + std::sin(a1) * 0.7f, c.z + std::cos(a1) * 0.7f }, TintePaso(vidrio, c));
        }
        rlEnd();

        for (int n = 0; n < 10; n++)
        {
            float a = 6.2832f * (float)n / 10.0f;
            CuboNieblaPaso({ s * (MURO_X_PASO - 0.1f), c.y + std::sin(a) * 0.82f, z + std::cos(a) * 0.82f }, { 0.2f, 0.3f, 0.3f }, COLOR_PIEDRA_CLARA_PASO);
        }

        CuboNieblaPaso({ s * (MURO_X_PASO - 0.06f), c.y, z }, { 0.06f, 1.4f, 0.07f }, COLOR_PIEDRA_OSCURA_PASO);
        CuboNieblaPaso({ s * (MURO_X_PASO - 0.06f), c.y, z }, { 0.06f, 0.07f, 1.4f }, COLOR_PIEDRA_OSCURA_PASO);
    }
}


// Columna de tambores con base, fuste estriado, collarino y capitel.
static void ColumnaPaso(Vector3 base, float yTope)
{
    Color piedra = TintePaso(COLOR_PIEDRA_CLARA_PASO, { base.x, 3.0f, base.z });
    Color oscura = TintePaso(COLOR_PIEDRA_PASO, { base.x, 3.0f, base.z });

    CuboPaso({ base.x, base.y + 0.25f, base.z }, { 1.6f, 0.5f, 1.6f }, oscura);
    PrismaPaso({ base.x, base.y + 0.5f, base.z }, 0.74f, 0.64f, 0.3f, 16, piedra);

    float yFuste = base.y + 0.8f;
    float altoFuste = yTope - 0.85f - yFuste;
    PrismaPaso({ base.x, yFuste, base.z }, 0.58f, 0.52f, altoFuste, 16, piedra, true);
    PrismaPaso({ base.x, yFuste + altoFuste, base.z }, 0.6f, 0.6f, 0.12f, 16, oscura);
    PrismaPaso({ base.x, yFuste + altoFuste + 0.12f, base.z }, 0.56f, 0.86f, 0.4f, 16, piedra);
    CuboPaso({ base.x, yTope - 0.17f, base.z }, { 1.9f, 0.34f, 1.9f }, oscura);
}


// Columnatas que separan la nave de las naves laterales, con su
// arquitrabe y friso encima.
static void DibujarColumnatasPaso(int paso)
{
    const float YTOPE = 7.4f;
    const float zs[4] = { 7.4f, 3.0f, -1.4f, -5.8f };

    for (int lado = -1; lado <= 1; lado += 2)
    {
        float x = (float)lado * COLUMNAS_X_PASO;

        for (int k = 0; k < 4; k++)
        {
            if (paso == 2 && (k == 1 || k == 3))
            {
                continue;
            }

            float y = zs[k] < ESTRADO_Z_PASO ? Y_ESTRADO_PASO : NIVEL_LATERAL_PASO;
            ColumnaPaso({ x, y, zs[k] }, YTOPE);
        }

        float zFin = LOGIA_Z_PASO;
        float zMedio = (NAVE_Z_FRENTE_PASO + zFin) * 0.5f;
        float largo = NAVE_Z_FRENTE_PASO - zFin;
        CuboNieblaPaso({ x, YTOPE + 0.4f, zMedio }, { 1.5f, 0.8f, largo }, COLOR_PIEDRA_PASO);
        CuboNieblaPaso({ x, YTOPE + 0.95f, zMedio }, { 1.7f, 0.3f, largo }, COLOR_PIEDRA_CLARA_PASO);
        CuboNieblaPaso({ x, YTOPE + 1.8f, zMedio }, { 1.2f, 1.4f, largo }, COLOR_PIEDRA_OSCURA_PASO);
    }
}


// Relleno sobre un arco de medio punto (enjuta) en rebanadas, con la
// rosca de dovelas claras. El hueco queda entre x0 y x1.
static void ArcoLogiaPaso(float x0, float x1, float yArranque, float yTope, float z, float grosor)
{
    float centro = (x0 + x1) * 0.5f;
    float radio = (x1 - x0) * 0.5f;
    const int REBANADAS = 14;

    for (int k = 0; k < REBANADAS; k++)
    {
        float xa = x0 + (x1 - x0) * (float)k / (float)REBANADAS;
        float xb = x0 + (x1 - x0) * (float)(k + 1) / (float)REBANADAS;
        float xm = (xa + xb) * 0.5f;
        float dx = (xm - centro) / radio;
        float yCurva = yArranque + radio * std::sqrt(Limitar01Paso(1.0f - dx * dx));

        if (yCurva < yTope)
        {
            CuboNieblaPaso({ xm, (yCurva + yTope) * 0.5f, z }, { xb - xa + 0.01f, yTope - yCurva, grosor }, COLOR_PIEDRA_OSCURA_PASO);
        }

        Color dovela = (k & 1) ? COLOR_PIEDRA_CLARA_PASO : TonoPaso(COLOR_PIEDRA_CLARA_PASO, 0.9f);
        CuboNieblaPaso({ xm, yCurva + 0.2f, z + grosor * 0.5f + 0.05f }, { xb - xa + 0.02f, 0.4f, 0.12f }, dovela);
    }
}


// Logia del fondo: tres arcos abiertos a la noche, pilares con pilastras
// y una balaustrada. MODELO FUTURO: logia del templo en GLB.
static void DibujarLogiaPaso(int paso, bool detalle)
{
    const float Z = LOGIA_Z_PASO - 0.6f;
    const float GROSOR = 1.2f;
    const float YTOPE = 14.0f;
    const float pilares[4][2] = { { -MURO_X_PASO, -11.2f }, { -6.4f, -5.0f }, { 5.0f, 6.4f }, { 11.2f, MURO_X_PASO } };

    for (int k = 0; k < 4; k++)
    {
        float xa = pilares[k][0];
        float xb = pilares[k][1];
        float xm = (xa + xb) * 0.5f;
        float ancho = xb - xa;

        CuboNieblaPaso({ xm, (Y_ESTRADO_PASO + YTOPE) * 0.5f, Z }, { ancho, YTOPE - Y_ESTRADO_PASO, GROSOR }, COLOR_PIEDRA_PASO);
        CuboNieblaPaso({ xm, Y_ESTRADO_PASO + 0.35f, Z + 0.1f }, { ancho + 0.3f, 0.7f, GROSOR + 0.2f }, COLOR_PIEDRA_OSCURA_PASO);
        CuboNieblaPaso({ xm, 7.6f, Z + 0.1f }, { ancho + 0.3f, 0.4f, GROSOR + 0.2f }, COLOR_PIEDRA_CLARA_PASO);

        // Pilastra al frente.
        CuboNieblaPaso({ xm, (Y_ESTRADO_PASO + 7.4f) * 0.5f, Z + GROSOR * 0.5f + 0.1f }, { ancho * 0.55f, 7.4f - Y_ESTRADO_PASO, 0.2f }, COLOR_PIEDRA_CLARA_PASO);
    }

    ArcoLogiaPaso(-5.0f, 5.0f, 7.8f, YTOPE, Z, GROSOR);
    ArcoLogiaPaso(-11.2f, -6.4f, 7.8f, YTOPE, Z, GROSOR);
    ArcoLogiaPaso(6.4f, 11.2f, 7.8f, YTOPE, Z, GROSOR);

    if (!detalle)
    {
        return;
    }

    // Balaustrada en los tres huecos.
    const float huecos[3][2] = { { -11.2f, -6.4f }, { -5.0f, 5.0f }, { 6.4f, 11.2f } };
    float zb = Z + 0.2f;

    for (int h = 0; h < 3; h++)
    {
        float xa = huecos[h][0];
        float xb = huecos[h][1];
        CuboNieblaPaso({ (xa + xb) * 0.5f, Y_ESTRADO_PASO + 0.1f, zb }, { xb - xa, 0.2f, 0.5f }, COLOR_PIEDRA_PASO);
        CuboNieblaPaso({ (xa + xb) * 0.5f, Y_ESTRADO_PASO + 1.15f, zb }, { xb - xa, 0.18f, 0.55f }, COLOR_PIEDRA_CLARA_PASO);

        Color balaustre = TintePaso(COLOR_PIEDRA_CLARA_PASO, { 0.0f, 1.0f, zb });
        for (float x = xa + 0.3f; x < xb - 0.1f; x += 0.45f * (float)paso)
        {
            PrismaPaso({ x, Y_ESTRADO_PASO + 0.2f, zb }, 0.09f, 0.16f, 0.45f, 6, balaustre);
            PrismaPaso({ x, Y_ESTRADO_PASO + 0.65f, zb }, 0.16f, 0.08f, 0.41f, 6, balaustre);
        }
    }
}


// Tapiz colgante en un plano Z que ondea con la corriente del salon:
// tela con guardas doradas, un emblema de luna y la punta en V abajo.
static void TapizPaso(
    Vector3 arriba,
    float ancho,
    float largo,
    float t,
    float fase,
    Color tela,
    Color guarda
)
{
    const int COLUMNAS = 6;
    const int FILAS = 10;
    Color telaN = TintePaso(tela, arriba);
    Color guardaN = TintePaso(guarda, arriba);
    float paso = largo / (float)FILAS;

    // Barra de bronce de la que cuelga.
    CuboPaso({ arriba.x, arriba.y + 0.06f, arriba.z }, { ancho + 0.4f, 0.12f, 0.12f }, TintePaso(COLOR_BRONCE_PASO, arriba));

    rlBegin(RL_QUADS);

    for (int f = 0; f < FILAS; f++)
    {
        for (int c = 0; c < COLUMNAS; c++)
        {
            float u0 = (float)c / (float)COLUMNAS;
            float u1 = (float)(c + 1) / (float)COLUMNAS;
            float v0 = (float)f / (float)FILAS;
            float v1 = (float)(f + 1) / (float)FILAS;

            Vector3 q[4];
            float uu[4] = { u0, u1, u1, u0 };
            float vv[4] = { v0, v0, v1, v1 };

            for (int n = 0; n < 4; n++)
            {
                float onda = std::sin(uu[n] * 4.0f + t * 1.5f + fase) * 0.12f + std::sin(vv[n] * 5.0f - t * 1.1f + fase) * 0.06f;
                float y = arriba.y - vv[n] * largo;

                // Punta en V: la ultima fila baja hacia el centro.
                if (f == FILAS - 1 && n >= 2)
                {
                    y -= paso * (1.0f - std::fabs(uu[n] - 0.5f) * 2.0f) * 1.6f;
                }

                q[n] = { arriba.x - ancho * 0.5f + uu[n] * ancho, y, arriba.z + onda * vv[n] * 1.4f };
            }

            float uc = (u0 + u1) * 0.5f;
            float vc = (v0 + v1) * 0.5f;
            bool borde = c == 0 || c == COLUMNAS - 1 || f == 0 || f == FILAS - 1;
            float du = (uc - 0.5f) * ancho;
            float dv = (vc - 0.45f) * largo;
            float r = std::sqrt(du * du + dv * dv);
            bool emblema = r < ancho * 0.28f && r > ancho * 0.18f;
            bool lunaLlena = r < ancho * 0.12f;

            Color color = borde || emblema || lunaLlena ? guardaN : telaN;
            float pliegue = 0.84f + 0.16f * std::sin(uc * 4.0f + t * 1.5f + fase + 1.2f);
            QuadPaso(q[0], q[1], q[2], q[3], TonoPaso(color, pliegue));
        }
    }

    rlEnd();
}


static void DibujarTapicesPaso(float t, int paso)
{
    // En la logia, sobre los pilares del arco central.
    TapizPaso({ -5.7f, 7.3f, LOGIA_Z_PASO + 0.25f }, 1.2f, 4.6f, t, 0.0f, COLOR_TAPIZ_PASO, COLOR_ORO_PASO);
    TapizPaso({ 5.7f, 7.3f, LOGIA_Z_PASO + 0.25f }, 1.2f, 4.6f, t, 1.7f, COLOR_TAPIZ_PASO, COLOR_ORO_PASO);

    // Estandartes que cuelgan del techo sobre las naves laterales.
    const Vector3 estandartes[4] =
    {
        { -9.8f, 7.0f, 0.8f }, { 9.8f, 7.0f, 0.8f },
        { -9.8f, 7.0f, -8.0f }, { 9.8f, 7.0f, -8.0f }
    };

    for (int k = 0; k < 4; k += paso)
    {
        Color tela = k < 2 ? COLOR_TAPIZ_AZUL_PASO : COLOR_TAPIZ_PASO;
        TapizPaso(estandartes[k], 2.0f, 4.2f, t, (float)k * 1.3f, tela, COLOR_ORO_PASO);

        // Cadenas al techo.
        for (int s = -1; s <= 1; s += 2)
        {
            Vector3 p = { estandartes[k].x + (float)s * 1.1f, estandartes[k].y + 1.7f, estandartes[k].z };
            CuboNieblaPaso(p, { 0.06f, 3.4f, 0.06f }, COLOR_BRONCE_OSCURO_PASO);
        }
    }
}


//==================================================
// ANILLO CERCANO: UTILERIA DEL TEMPLO
//==================================================

struct BraseroPaso
{
    Vector3 base;
    float escala;
};

static const BraseroPaso BRASEROS_PASO[6] =
{
    { { -6.2f, NIVEL_LATERAL_PASO, 2.4f }, 1.0f },
    { { 6.2f, NIVEL_LATERAL_PASO, 2.4f }, 1.0f },
    { { -6.2f, NIVEL_LATERAL_PASO, -5.2f }, 1.0f },
    { { 6.2f, NIVEL_LATERAL_PASO, -5.2f }, 1.0f },
    { { -5.6f, Y_ESTRADO_PASO, -11.4f }, 1.3f },
    { { 5.6f, Y_ESTRADO_PASO, -11.4f }, 1.3f }
};

// Postes de la meta, sobre el bordillo, con llama dorada.
static const Vector3 POSTES_META_PASO[2] =
{
    { -4.95f, 0.07f, Z_META_PASO },
    { 4.95f, 0.07f, Z_META_PASO }
};

static const Vector3 VELAS_PASO[6] =
{
    { -6.0f, NIVEL_LATERAL_PASO, -8.9f },
    { 6.0f, NIVEL_LATERAL_PASO, -8.9f },
    { -4.2f, Y_ESTRADO_PASO, -14.0f },
    { 4.2f, Y_ESTRADO_PASO, -14.0f },
    { -9.2f, NIVEL_LATERAL_PASO, 5.6f },
    { 9.6f, NIVEL_LATERAL_PASO, -2.6f }
};

static const Vector3 INCENSARIOS_PASO[2] =
{
    { -4.1f, Y_ESTRADO_PASO, -10.4f },
    { 4.1f, Y_ESTRADO_PASO, -10.4f }
};


// Altura de la llama numero k de un fuego (la misma en el dibujo y la luz).
static float AlturaLlamaPaso(float t, int k, int semilla)
{
    return 0.5f + 0.22f * std::sin(t * 9.0f + (float)k * 1.7f + (float)semilla) + 0.1f * std::sin(t * 23.0f + (float)k * 2.3f);
}


static void FuegoPaso(Vector3 base, float escala, float t, int semilla, bool dorado)
{
    Color exterior = dorado ? Color{ 255, 196, 70, 255 } : Color{ 255, 118, 40, 255 };
    Color interior = dorado ? Color{ 255, 244, 170, 255 } : Color{ 255, 212, 108, 255 };

    for (int k = 0; k < 5; k++)
    {
        float a = (float)k * 1.2566f + (float)semilla;
        float r = k == 0 ? 0.0f : 0.2f * escala;
        Vector3 p = { base.x + std::cos(a) * r, base.y, base.z + std::sin(a) * r };
        float h = AlturaLlamaPaso(t, k, semilla) * escala * (k == 0 ? 1.4f : 1.0f);
        PrismaPaso(p, 0.16f * escala, 0.0f, h, 5, exterior);
        PrismaPaso(p, 0.09f * escala, 0.0f, h * 0.6f, 5, interior);
    }
}


// Brasero de bronce sobre pie torneado, con brasas y fuego.
// MODELO FUTURO: brasero ceremonial GLB.
static void DibujarBraseroPaso(Vector3 base, float e, float t, int semilla)
{
    Color bronce = TintePaso(COLOR_BRONCE_PASO, base);
    Color oscuro = TintePaso(COLOR_BRONCE_OSCURO_PASO, base);

    PrismaPaso(base, 0.46f * e, 0.36f * e, 0.14f * e, 8, oscuro);
    PrismaPaso({ base.x, base.y + 0.14f * e, base.z }, 0.12f * e, 0.1f * e, 0.8f * e, 8, bronce);
    PrismaPaso({ base.x, base.y + 0.5f * e, base.z }, 0.17f * e, 0.17f * e, 0.12f * e, 8, oscuro);
    PrismaPaso({ base.x, base.y + 0.94f * e, base.z }, 0.2f * e, 0.56f * e, 0.34f * e, 12, bronce);

    Vector3 boca = { base.x, base.y + 1.28f * e, base.z };
    AnilloPaso(boca, 0.48f * e, 0.6f * e, TintePaso(COLOR_ORO_PASO, base), 18);
    AnilloPaso({ boca.x, boca.y - 0.02f, boca.z }, 0.0f, 0.5f * e, Color{ 70, 24, 16, 255 }, 14);
    FuegoPaso({ boca.x, boca.y - 0.04f, boca.z }, e, t, semilla, false);
}


static void DibujarPosteMetaPaso(Vector3 base, float t, int semilla)
{
    Color bronce = TintePaso(COLOR_BRONCE_PASO, base);
    PrismaPaso(base, 0.2f, 0.16f, 0.1f, 8, TintePaso(COLOR_BRONCE_OSCURO_PASO, base));
    PrismaPaso({ base.x, base.y + 0.1f, base.z }, 0.07f, 0.06f, 1.1f, 8, bronce);
    PrismaPaso({ base.x, base.y + 1.2f, base.z }, 0.08f, 0.24f, 0.16f, 10, TintePaso(COLOR_ORO_PASO, base));
    FuegoPaso({ base.x, base.y + 1.34f, base.z }, 0.55f, t, semilla, true);
}


// Grupo de velas de distintas alturas sobre un plato de bronce.
static void PosicionVelaPaso(Vector3 grupo, int g, int k, Vector3& base, float& alto)
{
    float a = 6.2832f * Hash01Paso(g * 11 + k, 300);
    float r = k == 0 ? 0.0f : 0.14f + 0.2f * Hash01Paso(g * 11 + k, 301);
    base = { grupo.x + std::cos(a) * r, grupo.y + 0.04f, grupo.z + std::sin(a) * r };
    alto = 0.22f + 0.4f * Hash01Paso(g * 11 + k, 302) + (k == 0 ? 0.2f : 0.0f);
}


static void DibujarVelasPaso(Vector3 grupo, int g, float t)
{
    AnilloPaso({ grupo.x, grupo.y + 0.03f, grupo.z }, 0.0f, 0.48f, TintePaso(COLOR_BRONCE_OSCURO_PASO, grupo), 16);
    Color cera = TintePaso(Color{ 228, 214, 180, 255 }, grupo);

    for (int k = 0; k < 5; k++)
    {
        Vector3 b;
        float alto;
        PosicionVelaPaso(grupo, g, k, b, alto);
        PrismaPaso(b, 0.06f, 0.055f, alto, 6, cera);
        float h = 0.1f + 0.03f * std::sin(t * 11.0f + (float)(g * 5 + k));
        PrismaPaso({ b.x, b.y + alto + 0.02f, b.z }, 0.035f, 0.0f, h, 4, Color{ 255, 214, 120, 255 });
    }
}


// Urna ceremonial (perfil por anillos) con bandas de bronce.
// MODELO FUTURO: urna GLB.
static void DibujarUrnaPaso(Vector3 base, float e, Color barro)
{
    Color c = TintePaso(barro, base);
    Color banda = TintePaso(COLOR_BRONCE_PASO, base);
    PrismaPaso(base, 0.22f * e, 0.26f * e, 0.1f * e, 12, TonoPaso(c, 0.8f));
    PrismaPaso({ base.x, base.y + 0.1f * e, base.z }, 0.26f * e, 0.46f * e, 0.42f * e, 12, c);
    PrismaPaso({ base.x, base.y + 0.52f * e, base.z }, 0.47f * e, 0.47f * e, 0.08f * e, 12, banda);
    PrismaPaso({ base.x, base.y + 0.6f * e, base.z }, 0.46f * e, 0.2f * e, 0.32f * e, 12, c);
    PrismaPaso({ base.x, base.y + 0.92f * e, base.z }, 0.18f * e, 0.2f * e, 0.18f * e, 12, c);
    PrismaPaso({ base.x, base.y + 1.1f * e, base.z }, 0.26f * e, 0.26f * e, 0.05f * e, 12, banda);
}


// Perro-leon guardian de piedra sentado sobre su plinto.
// MODELO FUTURO: estatua guardiana GLB.
static void DibujarGuardianPaso(Vector3 base, float giro)
{
    Color piedra = TintePaso(COLOR_GIGANTE_PASO, base);
    Color oscura = TintePaso(COLOR_GIGANTE_OSCURO_PASO, base);

    CuboPaso({ base.x, base.y + 0.4f, base.z }, { 1.3f, 0.8f, 1.5f }, TintePaso(COLOR_PIEDRA_PASO, base));
    CuboPaso({ base.x, base.y + 0.84f, base.z }, { 1.45f, 0.1f, 1.65f }, TintePaso(COLOR_PIEDRA_CLARA_PASO, base));

    rlPushMatrix();
    rlTranslatef(base.x, base.y + 0.9f, base.z);
    rlRotatef(giro, 0.0f, 1.0f, 0.0f);

    CuboPaso({ 0.0f, 0.35f, -0.3f }, { 0.8f, 0.7f, 0.9f }, piedra);           // ancas
    CuboPaso({ 0.0f, 0.75f, 0.15f }, { 0.75f, 1.0f, 0.7f }, piedra);          // pecho
    CuboPaso({ -0.24f, 0.25f, 0.45f }, { 0.22f, 0.5f, 0.24f }, oscura);       // patas
    CuboPaso({ 0.24f, 0.25f, 0.45f }, { 0.22f, 0.5f, 0.24f }, oscura);
    CuboPaso({ 0.0f, 1.45f, 0.25f }, { 0.95f, 0.7f, 0.75f }, oscura);         // melena
    CuboPaso({ 0.0f, 1.45f, 0.55f }, { 0.62f, 0.52f, 0.5f }, piedra);         // cara
    CuboPaso({ 0.0f, 1.32f, 0.84f }, { 0.36f, 0.24f, 0.18f }, piedra);        // hocico
    CuboPaso({ -0.18f, 1.58f, 0.81f }, { 0.12f, 0.06f, 0.03f }, Color{ 30, 32, 40, 255 });
    CuboPaso({ 0.18f, 1.58f, 0.81f }, { 0.12f, 0.06f, 0.03f }, Color{ 30, 32, 40, 255 });
    CuboPaso({ -0.3f, 1.86f, 0.25f }, { 0.16f, 0.2f, 0.16f }, oscura);        // orejas
    CuboPaso({ 0.3f, 1.86f, 0.25f }, { 0.16f, 0.2f, 0.16f }, oscura);
    CuboPaso({ 0.0f, 0.65f, -0.72f }, { 0.2f, 0.5f, 0.2f }, oscura);          // cola
    CuboPaso({ 0.0f, 1.06f, 0.52f }, { 0.4f, 0.08f, 0.06f }, TintePaso(COLOR_MUSGO_PASO, base));

    rlPopMatrix();
}


// Gong de bronce en su marco de madera: se mece apenas con la corriente.
// MODELO FUTURO: gong GLB.
static void DibujarGongPaso(Vector3 base, float t)
{
    Color madera = TintePaso(COLOR_MADERA_PASO, base);
    Color bronce = TintePaso(COLOR_BRONCE_PASO, base);

    CuboPaso({ base.x - 1.3f, base.y + 1.6f, base.z }, { 0.22f, 3.2f, 0.22f }, madera);
    CuboPaso({ base.x + 1.3f, base.y + 1.6f, base.z }, { 0.22f, 3.2f, 0.22f }, madera);
    CuboPaso({ base.x, base.y + 3.2f, base.z }, { 3.2f, 0.24f, 0.26f }, madera);
    CuboPaso({ base.x - 1.55f, base.y + 3.35f, base.z }, { 0.3f, 0.2f, 0.28f }, madera);
    CuboPaso({ base.x + 1.55f, base.y + 3.35f, base.z }, { 0.3f, 0.2f, 0.28f }, madera);
    CuboPaso({ base.x - 1.3f, base.y + 0.08f, base.z }, { 0.6f, 0.16f, 0.7f }, madera);
    CuboPaso({ base.x + 1.3f, base.y + 0.08f, base.z }, { 0.6f, 0.16f, 0.7f }, madera);

    float vaiven = std::sin(t * 0.9f) * 3.0f;

    rlPushMatrix();
    rlTranslatef(base.x, base.y + 3.1f, base.z);
    rlRotatef(vaiven, 0.0f, 0.0f, 1.0f);
    CuboPaso({ -0.5f, -0.25f, 0.0f }, { 0.04f, 0.5f, 0.04f }, TonoPaso(madera, 0.7f));
    CuboPaso({ 0.5f, -0.25f, 0.0f }, { 0.04f, 0.5f, 0.04f }, TonoPaso(madera, 0.7f));
    rlTranslatef(0.0f, -1.55f, 0.0f);
    rlRotatef(90.0f, 1.0f, 0.0f, 0.0f);
    PrismaPaso({ 0.0f, -0.05f, 0.0f }, 1.05f, 1.05f, 0.1f, 24, bronce);
    PrismaPaso({ 0.0f, -0.08f, 0.0f }, 0.35f, 0.35f, 0.16f, 16, TonoPaso(bronce, 1.12f));
    rlPopMatrix();

    // Mazo apoyado en el pie.
    CuboPaso({ base.x + 1.0f, base.y + 0.55f, base.z + 0.4f }, { 0.08f, 1.0f, 0.08f }, madera);
    CuboPaso({ base.x + 1.0f, base.y + 1.1f, base.z + 0.4f }, { 0.26f, 0.26f, 0.26f }, Color{ 140, 40, 40, 255 });
}


// Bloque caido o tambor de columna roto, apenas girado.
static void DibujarEscombroPaso(Vector3 p, Vector3 tamano, float giro, bool tambor)
{
    Color piedra = TintePaso(COLOR_PIEDRA_PASO, p);

    rlPushMatrix();
    rlTranslatef(p.x, p.y, p.z);
    rlRotatef(giro, 0.0f, 1.0f, 0.0f);

    if (tambor)
    {
        rlRotatef(90.0f, 0.0f, 0.0f, 1.0f);
        PrismaPaso({ 0.0f, -tamano.x * 0.5f, 0.0f }, tamano.y, tamano.y, tamano.x, 14, TintePaso(COLOR_PIEDRA_CLARA_PASO, p), true);
    }
    else
    {
        CuboPaso({ 0.0f, tamano.y * 0.5f, 0.0f }, tamano, piedra);
        CuboPaso({ 0.0f, tamano.y + 0.02f, 0.0f }, { tamano.x * 0.6f, 0.04f, tamano.z * 0.5f }, TintePaso(COLOR_MUSGO_PASO, p));
    }

    rlPopMatrix();
}


static void DibujarIncensarioPaso(Vector3 base)
{
    Color bronce = TintePaso(COLOR_BRONCE_PASO, base);
    PrismaPaso(base, 0.3f, 0.24f, 0.12f, 10, TintePaso(COLOR_BRONCE_OSCURO_PASO, base));
    PrismaPaso({ base.x, base.y + 0.12f, base.z }, 0.26f, 0.36f, 0.3f, 10, bronce);
    PrismaPaso({ base.x, base.y + 0.42f, base.z }, 0.36f, 0.12f, 0.22f, 10, bronce);
    PrismaPaso({ base.x, base.y + 0.64f, base.z }, 0.06f, 0.06f, 0.14f, 6, TintePaso(COLOR_ORO_PASO, base));
}


// Lampara colgante que se mece desde el arquitrabe.
static Vector3 PosicionLamparaPaso(int k, float t, float& angulo)
{
    const Vector3 anclas[4] =
    {
        { -6.2f, 7.4f, 5.2f }, { 6.2f, 7.4f, 5.2f },
        { -6.2f, 7.4f, -3.6f }, { 6.2f, 7.4f, -3.6f }
    };
    const float LARGO = 1.9f;

    angulo = std::sin(t * 0.8f + (float)k * 1.9f) * 6.0f;
    float a = angulo * DEG2RAD;
    Vector3 ancla = anclas[k];
    return { ancla.x + std::sin(a) * LARGO, ancla.y - std::cos(a) * LARGO, ancla.z };
}


static void DibujarLamparaPaso(int k, float t)
{
    float angulo = 0.0f;
    Vector3 p = PosicionLamparaPaso(k, t, angulo);
    Color bronce = TintePaso(COLOR_BRONCE_PASO, p);

    rlPushMatrix();
    rlTranslatef(p.x, p.y, p.z);
    rlRotatef(angulo, 0.0f, 0.0f, 1.0f);
    CuboPaso({ 0.0f, 1.0f, 0.0f }, { 0.05f, 1.9f, 0.05f }, TintePaso(COLOR_BRONCE_OSCURO_PASO, p));
    PrismaPaso({ 0.0f, 0.0f, 0.0f }, 0.34f, 0.1f, 0.22f, 6, bronce);
    PrismaPaso({ 0.0f, -0.55f, 0.0f }, 0.24f, 0.28f, 0.55f, 6, Color{ 255, 206, 132, 255 });
    PrismaPaso({ 0.0f, -0.65f, 0.0f }, 0.3f, 0.26f, 0.1f, 6, bronce);
    PrismaPaso({ 0.0f, -0.85f, 0.0f }, 0.0f, 0.12f, 0.2f, 6, bronce);
    rlPopMatrix();
}


static void DibujarUtileriaPaso(float t, int paso)
{
    for (int k = 0; k < 6; k++)
    {
        DibujarBraseroPaso(BRASEROS_PASO[k].base, BRASEROS_PASO[k].escala, t, k * 3);
    }

    for (int k = 0; k < 2; k++)
    {
        DibujarPosteMetaPaso(POSTES_META_PASO[k], t, 20 + k);
        DibujarIncensarioPaso(INCENSARIOS_PASO[k]);
    }

    for (int k = 0; k < 6; k += paso)
    {
        DibujarVelasPaso(VELAS_PASO[k], k, t);
    }

    for (int k = 0; k < 4; k++)
    {
        DibujarLamparaPaso(k, t);
    }

    // Guardianes que flanquean la entrada de los pasillos.
    DibujarGuardianPaso({ -6.5f, NIVEL_LATERAL_PASO, 6.8f }, 25.0f);
    DibujarGuardianPaso({ 6.5f, NIVEL_LATERAL_PASO, 6.8f }, -25.0f);

    DibujarGongPaso({ 9.8f, NIVEL_LATERAL_PASO, 0.4f }, t);

    struct UrnaPaso { Vector3 p; float e; int tipo; };
    const UrnaPaso urnas[8] =
    {
        { { -9.4f, NIVEL_LATERAL_PASO, 8.0f }, 1.0f, 0 },
        { { -8.6f, NIVEL_LATERAL_PASO, 8.6f }, 0.7f, 1 },
        { { 10.8f, NIVEL_LATERAL_PASO, 5.0f }, 1.1f, 2 },
        { { 10.0f, NIVEL_LATERAL_PASO, 5.8f }, 0.75f, 0 },
        { { -11.0f, NIVEL_LATERAL_PASO, -2.8f }, 1.2f, 1 },
        { { 9.2f, NIVEL_LATERAL_PASO, -8.6f }, 1.0f, 2 },
        { { -9.0f, Y_ESTRADO_PASO, -15.2f }, 1.1f, 0 },
        { { 9.0f, Y_ESTRADO_PASO, -15.4f }, 1.0f, 1 }
    };
    const Color barros[3] = { Color{ 150, 82, 58, 255 }, Color{ 64, 92, 112, 255 }, Color{ 168, 140, 96, 255 } };

    for (int k = 0; k < 8; k += paso)
    {
        DibujarUrnaPaso(urnas[k].p, urnas[k].e, barros[urnas[k].tipo]);
    }

    for (int k = 0; k < 8; k += paso)
    {
        float s = k % 2 == 0 ? -1.0f : 1.0f;
        Vector3 p = { s * (10.4f + 1.4f * Hash01Paso(k, 140)), NIVEL_LATERAL_PASO, -8.0f + 17.0f * Hash01Paso(k, 141) };
        Vector3 tamano = { 0.6f + 0.5f * Hash01Paso(k, 142), 0.35f + 0.3f * Hash01Paso(k, 143), 0.5f + 0.3f * Hash01Paso(k, 144) };
        DibujarEscombroPaso(p, tamano, 70.0f * Hash01Paso(k, 145), k == 3);
    }

    DibujarEscombroPaso({ -10.6f, NIVEL_LATERAL_PASO + 0.55f, 1.6f }, { 1.6f, 0.55f, 0.55f }, 20.0f, true);
}


//==================================================
// EL GIGANTE DE PIEDRA
//==================================================
//
// Un golem sentado en su trono, con los punos apoyados en el estrado.
// Dormido: la cabeza cae sobre el pecho, respira despacio y las runas del
// pecho laten en verde. Aviso: tiembla, levanta un poco la cabeza y
// entreabre los ojos en ambar. Alerta: cabeza erguida, ojos rojos y una
// mirada que barre los pasillos. Todo se dibuja en coordenadas del
// asiento y se escala. MODELO FUTURO: gigante y trono en GLB.

static const float ESCALA_GIGANTE_PASO = 1.18f;

struct PoseGigantePaso
{
    float respiro = 0.0f;
    float alzada = 0.0f;
    float cabeceo = 0.0f;           // grados; positivo = menton abajo
    float giro = 0.0f;              // grados; positivo = mira a +X
};


static PoseGigantePaso CalcularPoseGigantePaso(const MinijuegoPasoSilencioso& m)
{
    PoseGigantePaso pose;
    float t = m.tiempoAnimacion;
    float dormido = 1.0f - m.despertar;
    bool aviso = m.avisoPrevio && !m.centinelaAlerta;

    pose.respiro = std::sin(t * 1.25f) * dormido;
    pose.alzada = 0.4f * m.despertar + 0.05f * pose.respiro;
    pose.cabeceo = 32.0f * dormido - 6.0f * m.despertar;
    pose.giro = m.giroCabeza + (aviso ? std::sin(t * 38.0f) * 3.5f : 0.0f);
    return pose;
}


// Punto del cuerpo (coordenadas del asiento) a mundo.
static Vector3 PuntoGigantePaso(Vector3 local)
{
    return {
        local.x * ESCALA_GIGANTE_PASO,
        Y_ESTRADO_PASO + local.y * ESCALA_GIGANTE_PASO,
        Z_GIGANTE_PASO + local.z * ESCALA_GIGANTE_PASO
    };
}


// Punto en coordenadas de la cabeza (pivote en el cuello) a mundo.
static Vector3 PuntoCabezaGigantePaso(const PoseGigantePaso& pose, Vector3 local)
{
    float a = pose.cabeceo * DEG2RAD;
    float g = pose.giro * DEG2RAD;
    float y1 = local.y * std::cos(a) - local.z * std::sin(a);
    float z1 = local.y * std::sin(a) + local.z * std::cos(a);
    float x2 = local.x * std::cos(g) + z1 * std::sin(g);
    float z2 = -local.x * std::sin(g) + z1 * std::cos(g);

    return PuntoGigantePaso({
        CUELLO_GIGANTE_PASO.x + x2,
        CUELLO_GIGANTE_PASO.y + pose.alzada + y1,
        CUELLO_GIGANTE_PASO.z + z2
    });
}


// Ojos en coordenadas de la cabeza.
static const Vector3 OJO_IZQ_GIGANTE_PASO = { -0.5f, 0.98f, 1.27f };
static const Vector3 OJO_DER_GIGANTE_PASO = { 0.5f, 0.98f, 1.27f };


// Color de la mirada: verde dormido, ambar en el aviso, rojo en alerta.
static Color ColorMiradaPaso(const MinijuegoPasoSilencioso& m)
{
    if (m.centinelaAlerta) return Color{ 255, 62, 50, 255 };
    if (m.avisoPrevio) return Color{ 255, 172, 52, 255 };
    return Color{ 110, 236, 160, 255 };
}


// El gigante es lo que hay que mirar: casi sin penumbra.
static Color TinteGigantePaso(Color color)
{
    return TintePaso(color, { 0.0f, 3.0f, -2.0f });
}


// Asiento bajo de piedra: el respaldo no tapa la logia, asi el gigante
// se recorta contra el cielo de la noche.
static void DibujarTronoPaso()
{
    Color piedra = TinteGigantePaso(COLOR_PIEDRA_PASO);
    Color oscura = TinteGigantePaso(COLOR_PIEDRA_OSCURA_PASO);
    Color clara = TinteGigantePaso(COLOR_PIEDRA_CLARA_PASO);
    Color bronce = TinteGigantePaso(COLOR_BRONCE_PASO);

    rlPushMatrix();
    rlTranslatef(0.0f, Y_ESTRADO_PASO, Z_GIGANTE_PASO);
    rlScalef(ESCALA_GIGANTE_PASO, ESCALA_GIGANTE_PASO, ESCALA_GIGANTE_PASO);

    CuboPaso({ 0.0f, 0.9f, -1.7f }, { 5.8f, 1.8f, 2.6f }, oscura);
    CuboPaso({ 0.0f, 1.84f, -1.7f }, { 6.0f, 0.12f, 2.8f }, clara);
    CuboPaso({ 0.0f, 2.7f, -2.7f }, { 5.0f, 1.8f, 0.8f }, piedra);
    CuboPaso({ 0.0f, 3.66f, -2.7f }, { 5.4f, 0.16f, 1.0f }, clara);

    for (int s = -1; s <= 1; s += 2)
    {
        float x = (float)s * 3.3f;
        CuboPaso({ x, 1.5f, -1.4f }, { 0.9f, 3.0f, 2.4f }, piedra);
        CuboPaso({ x, 3.06f, -1.4f }, { 1.1f, 0.16f, 2.6f }, clara);
        PrismaPaso({ x, 3.14f, -0.8f }, 0.3f, 0.22f, 0.3f, 8, bronce);
        PrismaPaso({ x, 3.44f, -0.8f }, 0.22f, 0.0f, 0.5f, 8, bronce);
    }

    rlPopMatrix();
}


// Se dibuja dentro de la matriz del asiento (ya escalada).
static void DibujarCabezaGigantePaso(const MinijuegoPasoSilencioso& m, const PoseGigantePaso& pose)
{
    Color piedra = TinteGigantePaso(COLOR_GIGANTE_PASO);
    Color oscura = TinteGigantePaso(COLOR_GIGANTE_OSCURO_PASO);
    Color musgo = TinteGigantePaso(COLOR_MUSGO_PASO);
    Color mirada = ColorMiradaPaso(m);

    rlPushMatrix();
    rlTranslatef(CUELLO_GIGANTE_PASO.x, CUELLO_GIGANTE_PASO.y + pose.alzada, CUELLO_GIGANTE_PASO.z);
    rlRotatef(pose.giro, 0.0f, 1.0f, 0.0f);
    rlRotatef(pose.cabeceo, 1.0f, 0.0f, 0.0f);

    CuboPaso({ 0.0f, 0.85f, 0.45f }, { 2.2f, 1.8f, 1.6f }, piedra);           // craneo
    CuboPaso({ 0.0f, 1.32f, 1.22f }, { 2.35f, 0.34f, 0.5f }, oscura);         // ceja
    CuboPaso({ -0.95f, 0.55f, 0.95f }, { 0.5f, 0.8f, 0.8f }, piedra);         // pomulos
    CuboPaso({ 0.95f, 0.55f, 0.95f }, { 0.5f, 0.8f, 0.8f }, piedra);
    CuboPaso({ 0.0f, 0.72f, 1.36f }, { 0.42f, 0.6f, 0.36f }, piedra);         // nariz
    CuboPaso({ 0.0f, 0.08f, 0.75f }, { 1.9f, 0.5f, 1.3f }, oscura);           // mandibula
    CuboPaso({ 0.0f, -0.22f, 1.05f }, { 1.3f, 0.5f, 0.5f }, musgo);           // barba de musgo
    CuboPaso({ -1.2f, 0.8f, 0.35f }, { 0.3f, 0.7f, 0.6f }, oscura);           // orejas
    CuboPaso({ 1.2f, 0.8f, 0.35f }, { 0.3f, 0.7f, 0.6f }, oscura);
    CuboPaso({ 0.0f, 1.86f, 0.3f }, { 1.5f, 0.28f, 1.1f }, oscura);           // cresta
    CuboPaso({ 0.3f, 1.78f, -0.1f }, { 1.2f, 0.08f, 0.9f }, musgo);
    CuboPaso({ 0.0f, 0.55f, 1.255f }, { 1.0f, 0.06f, 0.04f }, TonoPaso(oscura, 0.6f));  // boca

    // Ojos: rendija cerrada, entreabiertos o abiertos del todo.
    float abertura = m.centinelaAlerta ? 0.26f : (m.avisoPrevio ? 0.13f : 0.05f);
    Color ojo = m.centinelaAlerta || m.avisoPrevio ? mirada : TonoPaso(mirada, 0.45f);

    for (int s = -1; s <= 1; s += 2)
    {
        float x = (float)s * 0.5f;
        CuboPaso({ x, 0.98f, 1.255f }, { 0.62f, 0.34f, 0.04f }, Color{ 26, 28, 34, 255 });
        CuboPaso({ x, 0.98f, 1.275f }, { 0.5f, abertura, 0.03f }, ojo);
    }

    // Runa de la frente.
    CuboPaso({ 0.0f, 1.62f, 1.27f }, { 0.16f, 0.16f, 0.03f }, TonoPaso(mirada, 0.8f));

    rlPopMatrix();
}


static void DibujarGigantePaso(const MinijuegoPasoSilencioso& m)
{
    PoseGigantePaso pose = CalcularPoseGigantePaso(m);
    Color piedra = TinteGigantePaso(COLOR_GIGANTE_PASO);
    Color oscura = TinteGigantePaso(COLOR_GIGANTE_OSCURO_PASO);
    Color musgo = TinteGigantePaso(COLOR_MUSGO_PASO);
    Color bronce = TinteGigantePaso(COLOR_BRONCE_PASO);
    Color mirada = ColorMiradaPaso(m);
    float r = pose.respiro * 0.05f;
    float hombros = 0.25f * m.despertar;

    rlPushMatrix();
    rlTranslatef(0.0f, Y_ESTRADO_PASO, Z_GIGANTE_PASO);
    rlScalef(ESCALA_GIGANTE_PASO, ESCALA_GIGANTE_PASO, ESCALA_GIGANTE_PASO);

    // Piernas sentadas y pies.
    for (int s = -1; s <= 1; s += 2)
    {
        float x = (float)s;
        CuboPaso({ x * 1.2f, 0.85f, 0.75f }, { 1.4f, 1.2f, 2.3f }, piedra);
        CuboPaso({ x * 1.25f, 0.4f, 1.95f }, { 1.5f, 0.8f, 1.3f }, oscura);
    }

    // Cadera, vientre y pecho (respira).
    CuboPaso({ 0.0f, 1.0f, -0.4f }, { 3.2f, 1.2f, 2.0f }, oscura);
    CuboPaso({ 0.0f, 2.2f + r, -0.4f }, { 3.6f, 1.6f, 2.2f }, piedra);
    CuboPaso({ 0.0f, 3.3f + r * 1.4f + hombros, -0.15f }, { 4.2f, 1.2f, 2.4f }, piedra);
    CuboPaso({ 0.0f, 3.95f + r * 1.4f + hombros, -0.7f }, { 3.0f, 0.4f, 1.4f }, musgo);

    // Grietas del pecho y relieve circular de las runas.
    CuboPaso({ 0.7f, 2.1f + r, 0.71f }, { 0.06f, 0.9f, 0.02f }, TonoPaso(oscura, 0.6f));
    CuboPaso({ -1.2f, 2.6f + r, 0.71f }, { 0.5f, 0.05f, 0.02f }, TonoPaso(oscura, 0.6f));
    AnilloFrentePaso({ 0.0f, 2.2f + r, 0.72f }, 0.42f, 0.56f, TonoPaso(oscura, 0.8f), 20);
    AnilloFrentePaso({ 0.0f, 2.2f + r, 0.73f }, 0.46f, 0.52f, TonoPaso(mirada, 0.7f), 20);
    CuboPaso({ 0.0f, 2.2f + r, 0.73f }, { 0.08f, 0.6f, 0.02f }, TonoPaso(mirada, 0.7f));
    CuboPaso({ 0.0f, 2.2f + r, 0.73f }, { 0.6f, 0.08f, 0.02f }, TonoPaso(mirada, 0.7f));

    // Cuello.
    CuboPaso({ 0.0f, CUELLO_GIGANTE_PASO.y + pose.alzada * 0.6f, 0.5f }, { 1.2f, 0.8f, 1.0f }, oscura);

    // Hombros, brazos, antebrazos con brazaletes y punos en el estrado.
    for (int s = -1; s <= 1; s += 2)
    {
        float x = (float)s;
        float yh = 3.55f + r * 1.4f + hombros;
        CuboPaso({ x * 2.55f, yh, -0.2f }, { 1.7f, 1.5f, 2.0f }, piedra);
        CuboPaso({ x * 2.55f, yh + 0.78f, -0.3f }, { 1.3f, 0.08f, 1.4f }, musgo);
        CuboPaso({ x * 2.95f, 2.35f + hombros * 0.5f, 0.25f }, { 1.2f, 1.9f, 1.3f }, oscura);
        CuboPaso({ x * 2.85f, 1.0f, 1.25f }, { 1.3f, 1.0f, 2.0f }, piedra);
        CuboPaso({ x * 2.85f, 1.0f, 0.6f }, { 1.4f, 1.1f, 0.26f }, bronce);
        CuboPaso({ x * 2.75f, 0.55f, 2.2f }, { 1.55f, 1.1f, 1.3f }, oscura);

        for (int d = 0; d < 3; d++)
        {
            CuboPaso({ x * 2.75f + (float)(d - 1) * 0.42f, 0.9f, 2.86f }, { 0.06f, 0.4f, 0.02f }, TonoPaso(oscura, 0.6f));
        }
    }

    DibujarCabezaGigantePaso(m, pose);
    rlPopMatrix();
}


//==================================================
// JUGADORES, TRANSLUCIDOS Y PARTICULAS
//==================================================

// Pies dibujados: sacudida del castigo y saltito de cada paso.
static Vector3 PiesDibujoPaso(const MinijuegoPasoSilencioso& m, int i)
{
    const EstadoJugadorPasoSilencioso& j = m.jugadores[i];
    Vector3 pies = PiesJugadorPaso(m, i);
    pies.x += std::sin(m.tiempoAnimacion * 60.0f) * 0.08f * (j.sacudida / 0.45f);
    pies.y = 0.04f + std::fabs(std::sin(j.fasePaso)) * 0.07f * j.marcha;
    return pies;
}


static void DibujarJugadoresPaso(const MinijuegoPasoSilencioso& m, const Participante participantes[])
{
    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!m.resultado.participantes[i].participo)
        {
            continue;
        }

        Vector3 pies = PiesDibujoPaso(m, i);
        DibujarSombraRetroCircular({ pies.x, 0.0f, pies.z }, 0.42f, 0.42f, 0.02f, 0.32f, 10);

        // Avanzan de espaldas hacia el gigante; al terminar se dan vuelta.
        float angulo = m.fase == FASE_PASO_TERMINADO ? 0.0f : 180.0f;
        DibujarModeloJugadorEnPosicion(pies, angulo, participantes[i].color);
    }
}


// Anillos de cada jugador y humo de los incensarios (alfa normal).
static void DibujarTranslucidosPaso(const MinijuegoPasoSilencioso& m, const Participante participantes[], bool decoracion)
{
    float t = m.tiempoAnimacion;

    rlDrawRenderBatchActive();
    rlDisableDepthMask();

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!m.resultado.participantes[i].participo)
        {
            continue;
        }

        const EstadoJugadorPasoSilencioso& j = m.jugadores[i];
        Vector3 pies = PiesJugadorPaso(m, i);
        Color color = participantes[i].color;

        AnilloPaso({ pies.x, 0.035f, pies.z }, 0.56f, 0.7f, ConAlfaPaso(color, 190.0f), 28);

        // Castigo: onda roja que se abre en las losas.
        if (j.flashCastigo > 0.0f)
        {
            float u = 1.0f - j.flashCastigo / 0.5f;
            float r = 0.6f + u * 1.8f;
            AnilloPaso({ pies.x, 0.04f, pies.z }, r, r + 0.18f, ConAlfaPaso(Color{ 255, 70, 56, 255 }, 230.0f * (1.0f - u)), 28);
        }
    }

    if (decoracion)
    {
        // Humo de incienso: volutas que suben, se ensanchan y se desvanecen.
        for (int n = 0; n < 2; n++)
        {
            Vector3 b = INCENSARIOS_PASO[n];

            for (int k = 0; k < 7; k++)
            {
                float u = std::fmod(t * 0.22f + (float)k / 7.0f + (float)n * 0.37f, 1.0f);
                Vector3 p =
                {
                    b.x + std::sin(t * 0.9f + (float)k * 1.3f) * 0.25f * u,
                    b.y + 0.8f + u * 3.2f,
                    b.z + std::cos(t * 0.7f + (float)k) * 0.15f * u
                };
                float alfa = 70.0f * std::sin(u * 3.1416f);
                ResplandorFrentePaso(p, 0.18f + u * 0.55f, ConAlfaPaso(Color{ 190, 196, 214, 255 }, alfa));
            }
        }
    }

    rlDrawRenderBatchActive();
    rlEnableDepthMask();
}


static void DibujarParticulasPaso(const MinijuegoPasoSilencioso& m)
{
    for (int i = 0; i < MAX_PARTICULAS_PASO; i++)
    {
        const ParticulaPasoSilencioso& p = m.particulas[i];

        if (!p.activa || p.brillo)
        {
            continue;
        }

        float vida = p.vida / p.vidaMaxima;
        float s = p.tamano * (0.4f + 0.6f * vida);
        CuboPaso(p.posicion, { s, s, s }, p.color);
    }
}


//==================================================
// LUCES FALSAS
//==================================================

// Haz de luna que entra por un ventanal izquierdo y su charco en el suelo.
static void HazLunaPaso(float zVentana, float t, int k)
{
    float vaiven = 0.85f + 0.15f * std::sin(t * 0.35f + (float)k * 1.9f);
    Color c = { 150, 180, 255, (unsigned char)(26.0f * vaiven) };
    Color c0 = c;
    c0.a = 0;

    float x0 = -MURO_X_PASO + 0.05f;
    float xSuelo = -2.2f;
    float zDesvio = 2.4f;

    rlBegin(RL_QUADS);
    VerticePaso({ x0, 6.8f, zVentana - 1.0f }, c);
    VerticePaso({ x0, 6.8f, zVentana + 1.0f }, c);
    VerticePaso({ xSuelo, 0.05f, zVentana + 1.0f + zDesvio }, c0);
    VerticePaso({ xSuelo, 0.05f, zVentana - 1.0f + zDesvio }, c0);
    rlEnd();

    // Charco de luna: cruza la nave lateral y entra en los pasillos.
    Color charco = { 140, 170, 255, (unsigned char)(34.0f * vaiven) };
    Color charco0 = charco;
    charco0.a = 0;
    float yLateral = NIVEL_LATERAL_PASO + 0.03f;

    rlBegin(RL_QUADS);
    VerticePaso({ -MURO_X_PASO, yLateral, zVentana - 0.9f }, charco);
    VerticePaso({ -MURO_X_PASO, yLateral, zVentana + 0.9f }, charco);
    VerticePaso({ -NAVE_X_PASO - 0.05f, yLateral, zVentana + 0.9f + 1.4f }, charco);
    VerticePaso({ -NAVE_X_PASO - 0.05f, yLateral, zVentana - 0.9f + 1.4f }, charco);

    VerticePaso({ -NAVE_X_PASO, 0.08f, zVentana - 0.9f + 1.4f }, charco);
    VerticePaso({ -NAVE_X_PASO, 0.08f, zVentana + 0.9f + 1.4f }, charco);
    VerticePaso({ -1.0f, 0.03f, zVentana + 0.9f + 2.6f }, charco0);
    VerticePaso({ -1.0f, 0.03f, zVentana - 0.9f + 2.6f }, charco0);
    rlEnd();
}


static void DibujarLucesPaso(const MinijuegoPasoSilencioso& m, const Participante participantes[], bool decoracion)
{
    if (CalidadEfectos() == CALIDAD_BAJA)
    {
        return;
    }

    float t = m.tiempoAnimacion;

    rlDrawRenderBatchActive();
    rlDisableDepthMask();
    BeginBlendMode(BLEND_ADDITIVE);

    // Luna por los ventanales de la izquierda y por la logia.
    const float ventanas[4] = { 5.2f, 0.8f, -3.6f, -8.0f };
    for (int k = 0; k < 4; k++)
    {
        HazLunaPaso(ventanas[k], t, k);
        ResplandorPaso({ -MURO_X_PASO + 0.1f, 4.6f, ventanas[k] }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f, 0.0f }, 2.4f, Color{ 150, 180, 255, 40 });
    }

    for (int k = -1; k <= 1; k++)
    {
        float x = (float)k * 8.0f;
        ResplandorSueloPaso({ x * 0.9f, Y_ESTRADO_PASO + 0.03f, LOGIA_Z_PASO + 2.2f }, 3.2f, Color{ 140, 170, 255, 30 }, 1.3f);
    }

    // Fuego de los braseros: halo en la llama y charco calido en el suelo.
    if (decoracion)
    {
        for (int i = 0; i < 6; i++)
        {
            Vector3 b = BRASEROS_PASO[i].base;
            float e = BRASEROS_PASO[i].escala;
            float pulso = 0.85f + 0.15f * std::sin(t * 9.0f + (float)i * 2.0f) + 0.05f * std::sin(t * 23.0f);
            Vector3 llama = { b.x, b.y + 1.55f * e, b.z };

            ResplandorFrentePaso(llama, 1.5f * e * pulso, Color{ 255, 140, 60, 110 });
            ResplandorFrentePaso(llama, 0.6f * e * pulso, Color{ 255, 220, 140, 150 });

            for (int r = 0; r < 3; r++)
            {
                ResplandorSueloPaso({ b.x, b.y + 0.03f + 0.002f * (float)r, b.z }, (1.6f + 0.9f * (float)r) * e * pulso, Color{ 255, 140, 64, 30 });
            }

            // Cuando el brasero esta junto a la nave, el calor tambien toca los pasillos.
            if (b.y < 0.0f)
            {
                float lado = b.x < 0.0f ? 1.0f : -1.0f;
                ResplandorSueloPaso({ b.x + lado * 1.6f, 0.03f, b.z }, 2.2f * pulso, Color{ 255, 140, 64, 22 });
            }

            for (int k = 0; k < 6; k++)
            {
                float ciclo = 1.1f + 0.6f * Hash01Paso(k, 210 + i);
                float u = std::fmod(t / ciclo + Hash01Paso(k, 212 + i), 1.0f);
                Vector3 p = { llama.x + std::sin(t * 3.0f + (float)k) * 0.25f * u, llama.y + 2.0f * u, llama.z };
                ResplandorFrentePaso(p, 0.07f, Color{ 255, 190, 90, (unsigned char)(220.0f * (1.0f - u)) });
            }
        }

        // Velas y lamparas colgantes.
        for (int g = 0; g < 6; g++)
        {
            Vector3 b;
            float alto;
            PosicionVelaPaso(VELAS_PASO[g], g, 0, b, alto);
            float pulso = 0.9f + 0.1f * std::sin(t * 11.0f + (float)g);
            ResplandorFrentePaso({ b.x, b.y + alto + 0.1f, b.z }, 0.7f * pulso, Color{ 255, 200, 120, 70 });
            ResplandorSueloPaso({ b.x, b.y + 0.04f, b.z }, 1.2f * pulso, Color{ 255, 170, 90, 26 });
        }

        for (int k = 0; k < 4; k++)
        {
            float angulo = 0.0f;
            Vector3 p = PosicionLamparaPaso(k, t, angulo);
            ResplandorFrentePaso({ p.x, p.y - 0.3f, p.z }, 1.0f, Color{ 255, 196, 120, 80 });
            ResplandorSueloPaso({ p.x, NIVEL_LATERAL_PASO + 0.04f, p.z }, 2.2f, Color{ 255, 180, 100, 18 });
        }

        // Ventana encendida de la pagoda lejana.
        ResplandorFrentePaso({ -22.6f, -5.8f, -82.0f }, 0.9f, Color{ 255, 190, 110, 120 });
    }

    // Postes de la meta (oro) y la banda que brilla.
    for (int k = 0; k < 2; k++)
    {
        Vector3 p = POSTES_META_PASO[k];
        float pulso = 0.85f + 0.15f * std::sin(t * 7.0f + (float)k);
        ResplandorFrentePaso({ p.x, p.y + 1.6f, p.z }, 0.9f * pulso, Color{ 255, 206, 96, 120 });
        ResplandorSueloPaso({ p.x, 0.08f, p.z }, 1.6f * pulso, Color{ 255, 200, 90, 30 });
    }

    float pulsoMeta = 0.75f + 0.25f * std::sin(t * 2.4f);
    ResplandorSueloPaso({ 0.0f, 0.05f, Z_META_PASO }, 5.2f, Color{ 255, 200, 90, (unsigned char)(28.0f * pulsoMeta) }, 1.0f);
    rlBegin(RL_QUADS);
    Color oro = { 255, 206, 96, (unsigned char)(70.0f * pulsoMeta) };
    Color oro0 = oro;
    oro0.a = 0;
    VerticePaso({ -4.7f, 0.05f, Z_META_PASO + 0.6f }, oro0);
    VerticePaso({ 4.7f, 0.05f, Z_META_PASO + 0.6f }, oro0);
    VerticePaso({ 4.7f, 0.05f, Z_META_PASO }, oro);
    VerticePaso({ -4.7f, 0.05f, Z_META_PASO }, oro);
    rlEnd();

    // El gigante: runas del pecho, ojos y, en alerta, la mirada que barre.
    PoseGigantePaso pose = CalcularPoseGigantePaso(m);
    Color mirada = ColorMiradaPaso(m);
    float latido = m.centinelaAlerta
        ? 1.0f
        : (m.avisoPrevio ? 0.7f + 0.3f * std::sin(t * 30.0f) : 0.55f + 0.25f * std::sin(t * 1.25f));

    Vector3 runas = PuntoGigantePaso({ 0.0f, 2.2f + pose.respiro * 0.05f, 0.76f });
    ResplandorPaso(runas, { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, 1.5f * latido, ConAlfaPaso(mirada, 120.0f * latido));

    Vector3 ojoI = PuntoCabezaGigantePaso(pose, OJO_IZQ_GIGANTE_PASO);
    Vector3 ojoD = PuntoCabezaGigantePaso(pose, OJO_DER_GIGANTE_PASO);
    float intensidadOjos = m.centinelaAlerta ? 1.0f : (m.avisoPrevio ? 0.6f * latido : 0.0f);

    if (intensidadOjos > 0.01f)
    {
        for (int s = 0; s < 2; s++)
        {
            Vector3 ojo = s == 0 ? ojoI : ojoD;
            Vector3 frente = { ojo.x, ojo.y, ojo.z + 0.1f };
            ResplandorFrentePaso(frente, 1.1f * intensidadOjos, ConAlfaPaso(mirada, 150.0f * intensidadOjos));
            ResplandorFrentePaso(frente, 0.4f * intensidadOjos, ConAlfaPaso(Color{ 255, 236, 220, 255 }, 200.0f * intensidadOjos));
        }
    }

    if (m.centinelaAlerta)
    {
        float fuerza = m.despertar;
        Vector3 centroOjos = { (ojoI.x + ojoD.x) * 0.5f, (ojoI.y + ojoD.y) * 0.5f, (ojoI.z + ojoD.z) * 0.5f };
        float g = pose.giro * DEG2RAD;
        Vector3 foco = { std::sin(g) * 9.0f, 0.06f, -1.5f };
        const int LADOS = 24;
        Color vertice = ConAlfaPaso(Color{ 255, 70, 56, 255 }, 70.0f * fuerza);
        Color borde = ConAlfaPaso(Color{ 255, 70, 56, 255 }, 0.0f);

        rlBegin(RL_TRIANGLES);
        for (int k = 0; k < LADOS; k++)
        {
            float a0 = 6.2832f * (float)k / (float)LADOS;
            float a1 = 6.2832f * (float)(k + 1) / (float)LADOS;
            VerticePaso(centroOjos, vertice);
            VerticePaso({ foco.x + std::cos(a0) * 5.0f, foco.y, foco.z + std::sin(a0) * 7.5f }, borde);
            VerticePaso({ foco.x + std::cos(a1) * 5.0f, foco.y, foco.z + std::sin(a1) * 7.5f }, borde);
        }
        rlEnd();

        ResplandorSueloPaso(foco, 7.0f, ConAlfaPaso(Color{ 255, 60, 48, 255 }, 46.0f * fuerza), 0.75f);
        ResplandorSueloPaso({ foco.x, 0.07f, foco.z }, 3.2f, ConAlfaPaso(Color{ 255, 90, 70, 255 }, 40.0f * fuerza));
    }

    // Jugadores: destello rojo del castigo y aura dorada al llegar.
    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!m.resultado.participantes[i].participo)
        {
            continue;
        }

        const EstadoJugadorPasoSilencioso& j = m.jugadores[i];
        Vector3 p = PiesJugadorPaso(m, i);

        if (j.flashCastigo > 0.0f)
        {
            float u = j.flashCastigo / 0.5f;
            ResplandorFrentePaso({ p.x, 0.9f, p.z + 0.3f }, 1.4f * u + 0.3f, ConAlfaPaso(Color{ 255, 70, 56, 255 }, 170.0f * u));
        }

        if (j.llegoMeta)
        {
            float u = 0.5f + 0.5f * Limitar01Paso(j.destelloMeta);
            ResplandorSueloPaso({ p.x, 0.06f, p.z }, 1.4f * u, ConAlfaPaso(participantes[i].color, 70.0f));
            ResplandorFrentePaso({ p.x, 0.9f, p.z + 0.3f }, 1.2f * u, Color{ 255, 214, 120, (unsigned char)(90.0f * u) });
        }
    }

    for (int i = 0; i < MAX_PARTICULAS_PASO; i++)
    {
        const ParticulaPasoSilencioso& p = m.particulas[i];

        if (!p.activa || !p.brillo)
        {
            continue;
        }

        float vida = p.vida / p.vidaMaxima;
        ResplandorFrentePaso(p.posicion, p.tamano * 1.6f, ConAlfaPaso(p.color, 230.0f * vida));
    }

    // Motas de polvo que brillan al cruzar la luz de la luna.
    int motas = (int)(46.0f * FactorCalidadGrafica(CalidadParticulas()));

    for (int k = 0; k < motas; k++)
    {
        float ciclo = 14.0f + 8.0f * Hash01Paso(k, 220);
        float u = std::fmod(t / ciclo + Hash01Paso(k, 221), 1.0f);
        Vector3 p =
        {
            -11.0f + 14.0f * Hash01Paso(k, 224) + std::sin(t * 0.4f + (float)k) * 0.5f,
            0.4f + 6.0f * u,
            -10.0f + 18.0f * Hash01Paso(k, 223) + std::cos(t * 0.3f + (float)k) * 0.4f
        };
        float alfa = std::sin(u * 3.1416f);
        ResplandorFrentePaso(p, 0.05f, Color{ 200, 220, 255, (unsigned char)(170.0f * alfa) });
    }

    EndBlendMode();
    rlEnableDepthMask();
}


//==================================================
// DIBUJAR
//==================================================

static void DibujarEscenarioPaso(const MinijuegoPasoSilencioso& m, const Participante participantes[])
{
    float t = m.tiempoAnimacion;
    bool decoracion = CalidadDecoracion() != CALIDAD_BAJA;
    int paso = CalidadDecoracion() == CALIDAD_ALTA ? 1 : 2;

    // Horizonte por la logia; muros, columnas y logia.
    DibujarExteriorPaso(t, decoracion ? paso : 2);
    DibujarLogiaPaso(paso, decoracion);

    for (int lado = -1; lado <= 1; lado += 2)
    {
        MuroLateralPaso((float)lado, decoracion);
    }

    DibujarColumnatasPaso(decoracion ? paso : 2);

    // Suelos, nave con los pasillos y el estrado.
    DibujarSuelosPaso();
    DibujarNavePaso(m, participantes);

    // El trono y el gigante van siempre: son la senal del juego.
    DibujarTronoPaso();
    DibujarGigantePaso(m);

    if (!decoracion)
    {
        return;
    }

    DibujarTapicesPaso(t, paso);
    DibujarUtileriaPaso(t, paso);
}


static const char* TextoAdelantePaso(const Participante& participante)
{
    switch (participante.control)
    {
        case CONTROL_GAMEPAD:
            return "STICK ARRIBA";

        case CONTROL_TECLADO_FLECHAS:
            return "FLECHA ARRIBA";

        case CONTROL_TECLADO_WASD:
            return "W";

        case CONTROL_TECLADO_COMPLETO:
            return "W / ARRIBA";

        case CONTROL_NINGUNO:
            break;
    }

    return "";
}


void MinijuegoPasoSilencioso::Dibujar(
    const Participante participantes[]
) const
{

    const float t = tiempoAnimacion;
    int ancho = GetScreenWidth();
    int alto = GetScreenHeight();
    bool decoracion = CalidadDecoracion() != CALIDAD_BAJA;

    // Entrada: durante la cuenta la camara baja desde lo alto del salon.
    Camera3D vista = camara;

    if (fase == FASE_PASO_PREPARACION)
    {
        float u = Limitar01Paso(tiempoPreparacion / DURACION_PREPARACION_PASO);
        float suave = u * u * (3.0f - 2.0f * u) * ObtenerFactorMovimientoCamaraMinijuegos();
        vista.position.y += 3.2f * suave;
        vista.position.z += 4.5f * suave;
        vista.target.y += 1.2f * suave;
    }

    ClearBackground(COLOR_NOCHE_PASO);
    DibujarCieloPaso(vista, t, decoracion);

    BeginMode3D(vista);
    rlDisableBackfaceCulling();

    DibujarEscenarioPaso(*this, participantes);
    DibujarJugadoresPaso(*this, participantes);
    DibujarParticulasPaso(*this);
    DibujarTranslucidosPaso(*this, participantes, decoracion);
    DibujarLucesPaso(*this, participantes, decoracion);

    rlEnableBackfaceCulling();
    EndMode3D();

    // Zzz del gigante dormido (y nada mientras mira).
    PoseGigantePaso pose = CalcularPoseGigantePaso(*this);

    if (fase != FASE_PASO_TERMINADO && !centinelaAlerta && !avisoPrevio && despertar < 0.2f)
    {
        Vector3 cabeza = PuntoCabezaGigantePaso(pose, { 0.9f, 1.6f, 0.4f });
        Vector2 s = GetWorldToScreen(cabeza, vista);

        for (int k = 0; k < 3; k++)
        {
            float u = std::fmod(t * 0.45f + (float)k / 3.0f, 1.0f);
            float alfa = std::sin(u * 3.1416f) * (1.0f - despertar * 5.0f);
            int token = k == 0 ? UI_TEXTO_PEQUENO : (k == 1 ? UI_TEXTO : UI_TEXTO_GRANDE);
            float x = s.x + Px(18.0f) + u * Px(46.0f) + std::sin(t * 2.0f + (float)k) * Px(5.0f);
            float y = s.y - u * Px(46.0f);
            DibujarTextoMundoContornoUI(u < 0.33f ? "z" : "Z", x, y, token, Color{ 176, 236, 210, 255 }, alfa);
        }
    }

    // Rotulos J1..J4 y la perdida flotante sobre quien fue castigado.
    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!resultado.participantes[i].participo || !participantes[i].activo)
        {
            continue;
        }

        const EstadoJugadorPasoSilencioso& j = jugadores[i];
        Vector3 pies = PiesDibujoPaso(*this, i);
        Vector2 p = GetWorldToScreen({ pies.x, pies.y + 2.25f, pies.z }, vista);

        char rotulo[8];
        std::snprintf(rotulo, sizeof(rotulo), "J%d", participantes[i].numeroJugador);

        OpcionesEtiquetaUI op;
        op.borde = participantes[i].color;
        op.pico = true;
        DibujarEtiquetaUI(rotulo, p, 0.0f, op);

        if (j.perdidaFlotante > 0.0f)
        {
            float subida = (1.4f - j.perdidaFlotante) * Px(34.0f);
            float alfa = Limitar01Paso(j.perdidaFlotante);
            char texto[16];
            std::snprintf(texto, sizeof(texto), "-%d %%", (int)(RETROCESO_ALERTA_PASO * 100.0f + 0.5f));
            DibujarTextoMundoContornoUI(texto, p.x, p.y - Px(34.0f) - subida, UI_TEXTO_GRANDE, Color{ 255, 96, 80, 255 }, alfa);
        }
    }

    // Anuncios del centinela, sobre el pecho del gigante.
    if (fase == FASE_PASO_CARRERA && tiempoAnuncio >= 0.0f)
    {
        float u = tiempoAnuncio;
        float alfa = u < 0.12f ? u / 0.12f : (u > DURACION_ANUNCIO_PASO - 0.4f ? (DURACION_ANUNCIO_PASO - u) / 0.4f : 1.0f);
        float yAnuncio = (float)alto * 0.40f;

        if (tipoAnuncio == ANUNCIO_PASO_QUIETOS)
        {
            DibujarTextoMundoContornoUI("¡QUIETOS!", (float)ancho * 0.5f, yAnuncio, UI_TEXTO_TITULO, Color{ 255, 104, 86, 255 }, alfa);
            DibujarTextoMundoContornoUI("EL GIGANTE ESTÁ MIRANDO", (float)ancho * 0.5f, yAnuncio + Px(48.0f), UI_TEXTO, UI_TEXTO_MUNDO, alfa);
        }
        else if (tipoAnuncio == ANUNCIO_PASO_DESPIERTA)
        {
            DibujarTextoMundoContornoUI("SE ESTÁ DESPERTANDO...", (float)ancho * 0.5f, yAnuncio, UI_TEXTO_GRANDE, Color{ 255, 192, 84, 255 }, alfa);
        }
        else if (tipoAnuncio == ANUNCIO_PASO_AVANZA)
        {
            DibujarTextoMundoContornoUI("¡SE DURMIÓ: AVANZÁ!", (float)ancho * 0.5f, yAnuncio, UI_TEXTO_GRANDE, Color{ 150, 240, 190, 255 }, alfa);
        }
    }

    // HUD comun.
    DatosHudMinijuego hud;
    PrepararDatosHudMinijuego(
        hud,
        "PASO SILENCIOSO",
        "AVANZÁ MIENTRAS EL GIGANTE DUERME. SI ABRE LOS OJOS, ¡QUEDATE QUIETO!",
        resultado
    );

    hud.fase = fase == FASE_PASO_PREPARACION
        ? HUD_MINIJUEGO_CUENTA
        : (fase == FASE_PASO_TERMINADO ? HUD_MINIJUEGO_FIN : HUD_MINIJUEGO_JUGANDO);
    hud.cuentaRestante = tiempoPreparacion;
    hud.reloj = HUD_RELOJ_CUENTA_ATRAS;
    hud.tiempoRestante = fase == FASE_PASO_PREPARACION ? DURACION_CARRERA_PASO : tiempoCarrera;
    hud.tiempoTotal = DURACION_CARRERA_PASO;

    bool alguienLlego = false;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        JugadorHudMinijuego& h = hud.jugadores[i];
        if (!h.visible) continue;

        const EstadoJugadorPasoSilencioso& j = jugadores[i];
        int porcentaje = (int)(Limitar01Paso(j.progreso) * 100.0f + 0.5f);
        alguienLlego = alguienLlego || j.llegoMeta;

        std::snprintf(h.valor, sizeof(h.valor), "%d %%", porcentaje);
        h.valorNumerico = j.penalizaciones;
        h.progreso = Limitar01Paso(j.progreso);

        if (j.llegoMeta)
        {
            std::snprintf(h.estado, sizeof(h.estado), "¡LLEGÓ!");
        }
        else if (fase == FASE_PASO_CARRERA && j.perdidaFlotante > 0.0f)
        {
            std::snprintf(h.estado, sizeof(h.estado), "¡TE VIO!");
        }

        std::snprintf(h.detalleFinal, sizeof(h.detalleFinal), "%d %% DEL SALÓN", porcentaje);

        if (j.penalizaciones == 0)
        {
            std::snprintf(h.detalleDesempate, sizeof(h.detalleDesempate), "SIN CASTIGOS");
        }
        else
        {
            std::snprintf(h.detalleDesempate, sizeof(h.detalleDesempate), "%d CASTIGO%s", j.penalizaciones, j.penalizaciones == 1 ? "" : "S");
        }

        if (EsHumanoPaso(participantes[i]))
        {
            std::snprintf(
                h.controles,
                sizeof(h.controles),
                "AVANZÁ: %s\nSI TE MIRA, SOLTÁ",
                TextoAdelantePaso(participantes[i])
            );
        }
    }

    if (fase == FASE_PASO_TERMINADO && resultado.estado == RESULTADO_MINIJUEGO_FINALIZADO)
    {
        hud.finPorTiempo = !alguienLlego;

        if (!alguienLlego)
        {
            hud.notaFinal = "NADIE LLEGÓ AL GIGANTE: GANA QUIEN MÁS AVANZÓ";
        }
    }

    DibujarHudMinijuego(hud, participantes);
}


const ResultadoMinijuego&
MinijuegoPasoSilencioso::ObtenerResultado() const
{
    return resultado;
}
