#include "Minigames/MinijuegoCargaInestable.h"

#include "Minigames/AudioMinijuegos.h"
#include "Minigames/EfectosVisualesMinijuegos.h"
#include "Minigames/HudMinijuego.h"
#include "Minigames/UtilidadesMinijuegos.h"
#include "Systems/CalidadGrafica.h"
#include "Systems/Input.h"
#include "UI/EstiloUI.h"
#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"

#include <cmath>
#include <cstdio>


//==================================================
// CARGA INESTABLE - SALA DE CALDERAS 3D
//==================================================
//
// Logica: los jugadores se numeran en orden alrededor de un anillo y la
// carga pasa al siguiente vivo (golpear = horario, saltar = antihorario).
// El fusible es aleatorio y oculto. Lo unico que se ve es el CALOR por
// escalones (TIBIA, CALIENTE, AL ROJO, CRITICA) cuyos umbrales se sortean
// en cada carga y no dependen de la duracion: el escalon no dice cuanto
// falta. El tic y el pulso aceleran por escalon con ritmo irregular.
// Visual: funciones Dibujar*Carga, separadas de la logica.
//==================================================


static const float DURACION_PREPARACION_CARGA = 2.5f;
static const float DURACION_EXPLOSION_CARGA = 1.35f;
static const float DURACION_VUELO_CARGA = 0.40f;
// Tras recibirla hay que esperar el vuelo mas un instante de agarre.
static const float BLOQUEO_RECEPCION_CARGA = 0.78f;
// Cada pase recalienta la carga y le quita mecha.
static const float RECALENTAMIENTO_PASE_CARGA = 0.2f;

// Anillo de plataformas.
static const float RADIO_ANILLO_CARGA = 6.6f;
static const float RADIO_PLATAFORMA_CARGA = 1.7f;
static const float ALTURA_PLATAFORMA_CARGA = 0.8f;

// Periodo base del tic por escalon de calor (se le suma ruido en cada tic).
static const float PERIODO_TIC_CARGA[4] = { 0.66f, 0.48f, 0.34f, 0.24f };
// Humo por segundo que suelta la carga en cada escalon.
static const float HUMO_NIVEL_CARGA[4] = { 0.0f, 3.0f, 8.0f, 15.0f };

static const char* const NOMBRE_NIVEL_CARGA[4] =
{
    "TIBIA", "CALIENTE", "AL ROJO", "CRÍTICA"
};


static float AleatorioCarga(float minimo, float maximo)
{
    return minimo + (maximo - minimo) * (float)GetRandomValue(0, 1000) / 1000.0f;
}


static int CantidadParticulasCarga(int base)
{
    long cantidad = std::lround(base * FactorCalidadGrafica(CalidadParticulas()));
    return cantidad < 1 ? 1 : (int)cantidad;
}


static int ContarVivosCarga(
    const MinijuegoCargaInestable& minijuego
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


static int ElegirVivoAleatorioCarga(
    const MinijuegoCargaInestable& minijuego
)
{
    int vivos[MAX_PARTICIPANTES]{};
    int cantidad = 0;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (
            minijuego.resultado.participantes[i].participo &&
            !minijuego.jugadores[i].eliminado
        )
        {
            vivos[cantidad] = i;
            cantidad++;
        }
    }

    if (cantidad <= 0)
    {
        return -1;
    }

    return vivos[GetRandomValue(0, cantidad - 1)];
}


static int BuscarSiguienteVivoCarga(
    const MinijuegoCargaInestable& minijuego,
    int origen,
    int direccion
)
{
    int indice = origen;

    for (int intento = 0; intento < MAX_PARTICIPANTES; intento++)
    {
        indice += direccion;

        if (indice < 0) indice = MAX_PARTICIPANTES - 1;
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


// Posicion del participante en el anillo (0 = arriba, sentido horario).
static int OrdenCarga(
    const MinijuegoCargaInestable& minijuego,
    int indice
)
{
    int orden = 0;

    for (int i = 0; i < indice; i++)
    {
        if (minijuego.resultado.participantes[i].participo)
        {
            orden++;
        }
    }

    return orden;
}


static int CantidadActivosCarga(
    const MinijuegoCargaInestable& minijuego
)
{
    int cantidad = 0;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (minijuego.resultado.participantes[i].participo)
        {
            cantidad++;
        }
    }

    return cantidad < 1 ? 1 : cantidad;
}


static float AnguloPlataformaCarga(
    const MinijuegoCargaInestable& minijuego,
    int indice
)
{
    return
        -PI / 2.0f +
        OrdenCarga(minijuego, indice) * PI * 2.0f /
        CantidadActivosCarga(minijuego);
}


// Centro de la plataforma (a ras del suelo).
static Vector3 PosicionPlataformaCarga(
    const MinijuegoCargaInestable& minijuego,
    int indice
)
{
    float angulo = AnguloPlataformaCarga(minijuego, indice);

    return Vector3
    {
        std::cos(angulo) * RADIO_ANILLO_CARGA,
        0.0f,
        std::sin(angulo) * RADIO_ANILLO_CARGA
    };
}


// Donde flota la carga cuando un jugador la sostiene (entre el y la caldera).
static Vector3 PosicionCargaSostenida(
    const MinijuegoCargaInestable& minijuego,
    int indice,
    float tiempo
)
{
    Vector3 plataforma = PosicionPlataformaCarga(minijuego, indice);

    return Vector3
    {
        plataforma.x * 0.72f,
        2.3f + std::sin(tiempo * 6.0f) * 0.12f,
        plataforma.z * 0.72f
    };
}


// Donde se ve la carga ahora: dormida en la caldera, en vuelo con arco o
// sostenida. Devuelve false si no hay carga visible.
static bool PosicionCargaActual(
    const MinijuegoCargaInestable& minijuego,
    float tiempo,
    float retrasoVuelo,
    Vector3& posicion
)
{
    if (minijuego.fase == FASE_CARGA_PREPARACION)
    {
        posicion = { 0.0f, 2.4f + std::sin(tiempo * 3.0f) * 0.15f, 0.0f };
        return true;
    }

    if (minijuego.fase != FASE_CARGA_ACTIVA || minijuego.portador < 0)
    {
        return false;
    }

    Vector3 destino = PosicionCargaSostenida(minijuego, minijuego.portador, tiempo);
    float t = minijuego.progresoPase - retrasoVuelo;

    if (t >= 1.0f)
    {
        posicion = destino;
        return true;
    }

    if (t < 0.0f) t = 0.0f;

    Vector3 origen = minijuego.origenPase >= 0
        ? PosicionCargaSostenida(minijuego, minijuego.origenPase, tiempo)
        : Vector3{ 0.0f, 1.4f, 0.0f };

    posicion = Vector3Lerp(origen, destino, t);
    posicion.y += std::sin(t * PI) * 2.2f;
    return true;
}


// Escalon de calor segun la mecha consumida (con los recalentamientos),
// comparada con umbrales sorteados: no depende de la duracion total.
static int CalcularNivelCalorCarga(
    const MinijuegoCargaInestable& minijuego
)
{
    float consumido = minijuego.duracionCarga - minijuego.tiempoCarga;
    int nivel = 0;

    for (int k = 0; k < 3; k++)
    {
        if (consumido >= minijuego.umbralCalor[k])
        {
            nivel = k + 1;
        }
    }

    return nivel;
}


static void EmitirParticulaCarga(
    MinijuegoCargaInestable& minijuego,
    TipoParticulaCarga tipo,
    Vector3 posicion,
    Vector3 velocidad,
    float vida,
    float tamano,
    Color color
)
{
    for (int i = 0; i < MAX_PARTICULAS_CARGA; i++)
    {
        ParticulaCarga& particula = minijuego.particulas[i];

        if (particula.activa)
        {
            continue;
        }

        particula.activa = true;
        particula.tipo = tipo;
        particula.posicion = posicion;
        particula.velocidad = velocidad;
        particula.vidaMaxima = vida;
        particula.vida = vida;
        particula.tamano = tamano;
        particula.color = color;
        return;
    }
}


static Vector3 DireccionAleatoriaCarga(float subidaMinima, float subidaMaxima)
{
    float angulo = AleatorioCarga(0.0f, 2.0f * PI);
    return { std::cos(angulo), AleatorioCarga(subidaMinima, subidaMaxima), std::sin(angulo) };
}


static void EmitirChispasCarga(
    MinijuegoCargaInestable& minijuego,
    Vector3 centro,
    int base,
    float fuerza
)
{
    int cantidad = CantidadParticulasCarga(base);

    for (int k = 0; k < cantidad; k++)
    {
        Vector3 d = DireccionAleatoriaCarga(0.3f, 1.4f);
        float v = AleatorioCarga(0.4f, 1.0f) * fuerza;
        int tono = GetRandomValue(0, 2);

        EmitirParticulaCarga(
            minijuego, PARTICULA_CARGA_CHISPA, centro,
            { d.x * v, d.y * v, d.z * v },
            AleatorioCarga(0.3f, 0.7f), AleatorioCarga(0.05f, 0.09f),
            tono == 0 ? Color{ 255, 236, 150, 255 }
            : (tono == 1 ? Color{ 255, 176, 70, 255 } : Color{ 255, 120, 50, 255 })
        );
    }
}


static void CrearExplosionCarga(
    MinijuegoCargaInestable& minijuego,
    Vector3 centro
)
{
    // Chispas que saltan y caen.
    EmitirChispasCarga(minijuego, centro, 70, 9.0f);

    // Lenguas de fuego que se abren y suben.
    int fuego = CantidadParticulasCarga(26);

    for (int k = 0; k < fuego; k++)
    {
        Vector3 d = DireccionAleatoriaCarga(-0.2f, 0.9f);
        float v = AleatorioCarga(1.2f, 3.6f);
        EmitirParticulaCarga(
            minijuego, PARTICULA_CARGA_FUEGO, centro,
            { d.x * v, d.y * v + 1.0f, d.z * v },
            AleatorioCarga(0.45f, 0.85f), AleatorioCarga(0.35f, 0.6f),
            GetRandomValue(0, 1) == 0 ? Color{ 255, 170, 60, 255 } : Color{ 255, 112, 40, 255 }
        );
    }

    // Humo negro que queda flotando.
    int humo = CantidadParticulasCarga(28);

    for (int k = 0; k < humo; k++)
    {
        Vector3 d = DireccionAleatoriaCarga(0.1f, 0.8f);
        float v = AleatorioCarga(0.6f, 2.2f);
        unsigned char gris = (unsigned char)GetRandomValue(38, 70);
        EmitirParticulaCarga(
            minijuego, PARTICULA_CARGA_HUMO, centro,
            { d.x * v, d.y * v + 0.8f, d.z * v },
            AleatorioCarga(1.4f, 2.4f), AleatorioCarga(0.35f, 0.6f),
            Color{ gris, (unsigned char)(gris - 6), (unsigned char)(gris - 8), 255 }
        );
    }

    // Trozos de la carcasa.
    int escombros = CantidadParticulasCarga(18);

    for (int k = 0; k < escombros; k++)
    {
        Vector3 d = DireccionAleatoriaCarga(0.5f, 1.5f);
        float v = AleatorioCarga(3.0f, 7.0f);
        EmitirParticulaCarga(
            minijuego, PARTICULA_CARGA_ESCOMBRO, centro,
            { d.x * v, d.y * v, d.z * v },
            AleatorioCarga(1.2f, 2.0f), AleatorioCarga(0.09f, 0.18f),
            GetRandomValue(0, 2) == 0 ? Color{ 150, 92, 52, 255 } : Color{ 62, 58, 62, 255 }
        );
    }
}


static void PrepararCargaNueva(
    MinijuegoCargaInestable& minijuego,
    int portadorInicial
)
{
    minijuego.numeroRonda++;
    minijuego.portador = portadorInicial;
    minijuego.portadorExplosion = -1;
    minijuego.duracionCarga = GetRandomValue(590, 980) / 100.0f;
    minijuego.tiempoCarga = minijuego.duracionCarga;
    minijuego.bloqueoPase = 0.48f;
    minijuego.tiempoExplosion = 0.0f;
    minijuego.fase = FASE_CARGA_ACTIVA;
    minijuego.origenPase = -1;
    minijuego.progresoPase = 0.0f;
    minijuego.tiempoTick = 0.6f;

    // Umbrales de calor propios de esta carga, independientes de su
    // duracion: el escalon CRITICA puede llegar mucho antes o no llegar.
    minijuego.umbralCalor[0] = AleatorioCarga(1.5f, 2.9f);
    minijuego.umbralCalor[1] = minijuego.umbralCalor[0] + AleatorioCarga(1.5f, 2.6f);
    minijuego.umbralCalor[2] = minijuego.umbralCalor[1] + AleatorioCarga(1.3f, 2.4f);
    minijuego.nivelCalor = 0;
    minijuego.nivelVisual = 0.0f;
    minijuego.destelloTick = 0.0f;
    minijuego.acumuladorHumo = 0.0f;
    minijuego.tiempoRonda = 0.0f;

    if (portadorInicial >= 0)
    {
        minijuego.jugadores[portadorInicial].tiempoDecisionBot =
            GetRandomValue(32, 105) / 100.0f;
    }
}


static void PasarCarga(
    MinijuegoCargaInestable& minijuego,
    int direccion
)
{
    if (minijuego.portador < 0 || minijuego.bloqueoPase > 0.0f)
    {
        return;
    }

    int anterior = minijuego.portador;
    int siguiente = BuscarSiguienteVivoCarga(
        minijuego,
        anterior,
        direccion
    );

    if (siguiente == anterior)
    {
        return;
    }

    minijuego.jugadores[anterior].cantidadPases++;
    minijuego.portador = siguiente;
    minijuego.bloqueoPase = BLOQUEO_RECEPCION_CARGA;
    minijuego.tiempoCarga -= RECALENTAMIENTO_PASE_CARGA;
    minijuego.origenPase = anterior;
    minijuego.progresoPase = 0.0f;
    minijuego.jugadores[siguiente].tiempoDecisionBot =
        GetRandomValue(5, 70) / 100.0f +
        (GetRandomValue(1, 100) <= 12 ? GetRandomValue(50, 100) / 100.0f : 0.0f);

    // Chispazo al lanzarla.
    EmitirChispasCarga(
        minijuego,
        PosicionCargaSostenida(minijuego, anterior, minijuego.tiempoAnimacion),
        10,
        3.5f
    );

    ReproducirSonidoMinijuego(minijuego.audio, SONIDO_BOTON);
}


static void FinalizarCargaInestable(
    MinijuegoCargaInestable& minijuego
)
{
    int ganador = ElegirVivoAleatorioCarga(minijuego);

    if (ganador >= 0)
    {
        minijuego.jugadores[ganador].posicionFinal = 1;
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
            minijuego.jugadores[i].cantidadPases * 5 +
            (MAX_PARTICIPANTES + 1 - resultadoJugador.posicionFinal) * 20;
    }

    minijuego.resultado.estado = RESULTADO_MINIJUEGO_FINALIZADO;
    minijuego.resultado.desenlace = DESENLACE_CON_GANADOR;
    minijuego.fase = FASE_CARGA_TERMINADO;
    minijuego.portador = ganador;

    ReproducirSonidoMinijuego(minijuego.audio, SONIDO_RESULTADO);
}


void MinijuegoCargaInestable::Inicializar()
{
    resultado = {};
    resultado.formato = FORMATO_MINIJUEGO_INDIVIDUAL;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        jugadores[i] = {};
    }

    for (int i = 0; i < MAX_PARTICULAS_CARGA; i++)
    {
        particulas[i] = {};
    }

    fase = FASE_CARGA_PREPARACION;
    portador = -1;
    portadorExplosion = -1;
    numeroRonda = 0;
    origenPase = -1;
    progresoPase = 1.0f;
    tiempoTick = 0.0f;
    umbralCalor[0] = umbralCalor[1] = umbralCalor[2] = 0.0f;
    nivelCalor = 0;
    nivelVisual = 0.0f;
    destelloTick = 0.0f;
    destelloCaptura = 0.0f;
    acumuladorHumo = 0.0f;
    tiempoRonda = 0.0f;
    posicionExplosion = { 0.0f, 0.0f, 0.0f };
    tiempoPreparacion = DURACION_PREPARACION_CARGA;
    tiempoCarga = 0.0f;
    duracionCarga = 0.0f;
    bloqueoPase = 0.0f;
    tiempoExplosion = 0.0f;
    tiempoAnimacion = 0.0f;
}


void MinijuegoCargaInestable::Reiniciar(
    Participante participantes[]
)
{
    Inicializar();
    InicializarResultadoMinijuego(
        resultado,
        participantes,
        FORMATO_MINIJUEGO_INDIVIDUAL
    );

    portador = ElegirVivoAleatorioCarga(*this);
}


static void ActualizarParticulasCarga(
    MinijuegoCargaInestable& minijuego,
    float deltaTime
)
{
    for (int i = 0; i < MAX_PARTICULAS_CARGA; i++)
    {
        ParticulaCarga& p = minijuego.particulas[i];

        if (!p.activa)
        {
            continue;
        }

        switch (p.tipo)
        {
            case PARTICULA_CARGA_CHISPA:
                p.velocidad.y -= 9.0f * deltaTime;
                break;

            case PARTICULA_CARGA_ESCOMBRO:
                p.velocidad.y -= 13.0f * deltaTime;

                // Rebota en el piso y se frena.
                if (p.posicion.y < 0.12f && p.velocidad.y < 0.0f)
                {
                    p.posicion.y = 0.12f;
                    p.velocidad.y *= -0.32f;
                    p.velocidad.x *= 0.55f;
                    p.velocidad.z *= 0.55f;
                }
                break;

            case PARTICULA_CARGA_FUEGO:
                p.velocidad = Vector3Scale(p.velocidad, std::exp(-3.0f * deltaTime));
                p.velocidad.y += 2.0f * deltaTime;
                p.tamano += 0.9f * deltaTime;
                break;

            case PARTICULA_CARGA_HUMO:
            case PARTICULA_CARGA_VAPOR:
                p.velocidad = Vector3Scale(p.velocidad, std::exp(-1.6f * deltaTime));
                p.velocidad.y += 0.9f * deltaTime;
                p.tamano += (p.tipo == PARTICULA_CARGA_HUMO ? 0.55f : 0.8f) * deltaTime;
                break;
        }

        p.posicion = Vector3Add(p.posicion, Vector3Scale(p.velocidad, deltaTime));
        p.vida -= deltaTime;

        if (p.vida <= 0.0f || (p.tipo == PARTICULA_CARGA_CHISPA && p.posicion.y < 0.0f))
        {
            p.activa = false;
        }
    }
}


// Humo, chispas y estela que suelta la carga mientras esta en juego.
static void ActualizarEmisionCarga(
    MinijuegoCargaInestable& minijuego,
    float deltaTime
)
{
    Vector3 posicion{};

    if (
        minijuego.fase != FASE_CARGA_ACTIVA ||
        !PosicionCargaActual(minijuego, minijuego.tiempoAnimacion, 0.0f, posicion)
    )
    {
        return;
    }

    float factor = FactorCalidadGrafica(CalidadParticulas());
    float tasa = HUMO_NIVEL_CARGA[minijuego.nivelCalor] * factor;

    // En vuelo deja una estela de chispas.
    if (minijuego.progresoPase < 1.0f)
    {
        tasa += 34.0f * factor;
    }

    minijuego.acumuladorHumo += tasa * deltaTime;

    while (minijuego.acumuladorHumo >= 1.0f)
    {
        minijuego.acumuladorHumo -= 1.0f;

        Vector3 desvio = {
            AleatorioCarga(-0.25f, 0.25f),
            AleatorioCarga(0.2f, 0.5f),
            AleatorioCarga(-0.25f, 0.25f)
        };

        if (minijuego.progresoPase < 1.0f)
        {
            EmitirParticulaCarga(
                minijuego, PARTICULA_CARGA_CHISPA, Vector3Add(posicion, Vector3Scale(desvio, 0.5f)),
                { desvio.x * 2.0f, AleatorioCarga(-0.5f, 1.2f), desvio.z * 2.0f },
                AleatorioCarga(0.25f, 0.5f), AleatorioCarga(0.05f, 0.08f),
                GetRandomValue(0, 1) == 0 ? Color{ 255, 220, 130, 255 } : Color{ 255, 150, 60, 255 }
            );
            continue;
        }

        unsigned char gris = (unsigned char)GetRandomValue(70, 110);
        EmitirParticulaCarga(
            minijuego, PARTICULA_CARGA_HUMO, Vector3Add(posicion, desvio),
            { desvio.x * 0.8f, AleatorioCarga(0.6f, 1.2f), desvio.z * 0.8f },
            AleatorioCarga(0.9f, 1.5f), AleatorioCarga(0.14f, 0.24f),
            Color{ gris, (unsigned char)(gris - 8), (unsigned char)(gris - 10), 255 }
        );

        // Al rojo y critica: alguna chispa suelta.
        if (minijuego.nivelCalor >= 2 && GetRandomValue(0, 2) == 0)
        {
            EmitirChispasCarga(minijuego, posicion, 1, 2.5f);
        }
    }
}


void MinijuegoCargaInestable::Actualizar(
    float deltaTime,
    Participante participantes[]
)
{
    tiempoAnimacion += deltaTime;
    destelloTick *= std::exp(-7.0f * deltaTime);
    destelloCaptura *= std::exp(-5.0f * deltaTime);
    nivelVisual += (nivelCalor - nivelVisual) * (1.0f - std::exp(-5.0f * deltaTime));

    float progresoAntes = progresoPase;
    progresoPase += deltaTime / DURACION_VUELO_CARGA;
    if (progresoPase > 1.0f) progresoPase = 1.0f;

    // La recibe: chispazo en las manos del nuevo portador.
    if (fase == FASE_CARGA_ACTIVA && progresoAntes < 1.0f && progresoPase >= 1.0f && portador >= 0)
    {
        destelloCaptura = 1.0f;
        EmitirChispasCarga(*this, PosicionCargaSostenida(*this, portador, tiempoAnimacion), 8, 2.8f);
    }

    ActualizarParticulasCarga(*this, deltaTime);
    ActualizarEmisionCarga(*this, deltaTime);

    if (fase == FASE_CARGA_TERMINADO)
    {
        return;
    }

    if (fase == FASE_CARGA_PREPARACION)
    {
        float preparacionAntes = tiempoPreparacion;
        tiempoPreparacion -= deltaTime;
        ActualizarAudioCuentaRegresiva(audio, preparacionAntes, tiempoPreparacion);

        if (tiempoPreparacion <= 0.0f)
        {
            tiempoPreparacion = 0.0f;
            PrepararCargaNueva(*this, portador);
        }

        return;
    }

    if (fase == FASE_CARGA_EXPLOSION)
    {
        tiempoExplosion -= deltaTime;

        if (tiempoExplosion <= 0.0f)
        {
            if (ContarVivosCarga(*this) <= 1)
            {
                FinalizarCargaInestable(*this);
            }
            else
            {
                PrepararCargaNueva(
                    *this,
                    ElegirVivoAleatorioCarga(*this)
                );
            }
        }

        return;
    }

    bloqueoPase -= deltaTime;
    if (bloqueoPase < 0.0f) bloqueoPase = 0.0f;

    tiempoCarga -= deltaTime;
    tiempoRonda += deltaTime;

    // Sube de escalon: resoplido de vapor y destello.
    int nivelNuevo = CalcularNivelCalorCarga(*this);

    if (nivelNuevo > nivelCalor)
    {
        nivelCalor = nivelNuevo;
        destelloTick = 1.0f;

        Vector3 posicion{};
        if (PosicionCargaActual(*this, tiempoAnimacion, 0.0f, posicion))
        {
            int cantidad = CantidadParticulasCarga(10);

            for (int k = 0; k < cantidad; k++)
            {
                Vector3 d = DireccionAleatoriaCarga(-0.1f, 0.6f);
                EmitirParticulaCarga(
                    *this, PARTICULA_CARGA_VAPOR, posicion,
                    { d.x * 2.4f, d.y * 2.4f, d.z * 2.4f },
                    AleatorioCarga(0.5f, 0.9f), AleatorioCarga(0.18f, 0.3f),
                    Color{ 226, 222, 216, 255 }
                );
            }
        }
    }

    // Tic que acelera por escalon, con ruido en cada intervalo: su ritmo
    // transmite tension pero no permite calcular cuanto falta.
    tiempoTick -= deltaTime;

    if (tiempoTick <= 0.0f)
    {
        ReproducirSonidoMinijuego(audio, SONIDO_ALERTA_TIEMPO);
        tiempoTick = PERIODO_TIC_CARGA[nivelCalor] * AleatorioCarga(0.72f, 1.32f);
        destelloTick = 1.0f;
    }

    if (tiempoCarga <= 0.0f)
    {
        int vivosAntes = ContarVivosCarga(*this);

        if (!PosicionCargaActual(*this, tiempoAnimacion, 0.0f, posicionExplosion))
        {
            posicionExplosion = { 0.0f, 2.3f, 0.0f };
        }

        if (portador >= 0)
        {
            jugadores[portador].eliminado = true;
            jugadores[portador].posicionFinal = vivosAntes;
            jugadores[portador].rondaEliminacion = numeroRonda;
            CrearExplosionCarga(*this, posicionExplosion);
        }

        portadorExplosion = portador;
        portador = -1;
        fase = FASE_CARGA_EXPLOSION;
        tiempoExplosion = DURACION_EXPLOSION_CARGA;

        // Ya respeta "reducir movimiento".
        ActivarTemblorCamaraGeneral(0.55f, 0.75f);

        ReproducirSonidoMinijuego(audio, SONIDO_EXPLOSION);
        ReproducirSonidoMinijuego(audio, SONIDO_ELIMINADO);
        return;
    }

    if (portador < 0 || bloqueoPase > 0.0f)
    {
        return;
    }

    if (participantes[portador].esBot || !participantes[portador].conectado)
    {
        // Como un humano: reacciona mas rapido cuanto mas caliente esta.
        float calor = duracionCarga > 0.0f
            ? 1.0f - Clamp(tiempoCarga / duracionCarga, 0.0f, 1.0f)
            : 0.0f;
        jugadores[portador].tiempoDecisionBot -= deltaTime * (1.0f + 1.5f * calor);

        if (jugadores[portador].tiempoDecisionBot <= 0.0f)
        {
            int direccion = GetRandomValue(0, 1) == 0 ? -1 : 1;

            // Con tres o mas vivos no se la devuelve a quien se la acaba de dar.
            if (
                ContarVivosCarga(*this) >= 3 &&
                BuscarSiguienteVivoCarga(*this, portador, direccion) == origenPase
            )
            {
                direccion = -direccion;
            }

            PasarCarga(*this, direccion);
        }

        return;
    }

    InputMinijuegoParticipante entrada =
        LeerInputMinijuegoParticipante(participantes[portador]);

    if (entrada.golpear)
    {
        PasarCarga(*this, 1);
    }
    else if (entrada.saltar)
    {
        PasarCarga(*this, -1);
    }
}


//==================================================
// VISUAL 3D (independiente de la logica)
//==================================================
//
// Sala de calderas: piso de chapas, foso de metal fundido alrededor de la
// caldera central, pared de ladrillo con hornos, tuberias con valvulas y
// manometros, tanques laterales, lamparas colgantes y vapor. La carga es
// una esfera blindada cuyo brillo y humo siguen el escalon de calor.
//
// MODELO FUTURO: reemplazar por GLB la caldera central, los hornos y la
// carga blindada.
//==================================================


static unsigned char CanalCarga(float valor)
{
    return (unsigned char)(valor < 0.0f ? 0.0f : (valor > 255.0f ? 255.0f : valor));
}


static Color TinteCarga(Color color, float factor)
{
    return Color{
        CanalCarga(color.r * factor),
        CanalCarga(color.g * factor),
        CanalCarga(color.b * factor),
        color.a
    };
}


static Color AlfaCarga(Color color, float alfa)
{
    return Color{ color.r, color.g, color.b, CanalCarga(255.0f * Clamp(alfa, 0.0f, 1.0f)) };
}


static float HashCarga(int a, int b)
{
    unsigned int h = ((unsigned int)a * 73856093u) ^ ((unsigned int)b * 19349663u);
    h = (h ^ (h >> 13)) * 1274126177u;
    h ^= h >> 16;
    return (float)(h & 0xFFFF) / 65535.0f;
}


// Color de la carga segun el escalon (interpolado para que no salte).
static Color ColorNivelCarga(float nivel)
{
    static const Color COLORES[4] =
    {
        Color{ 110, 200, 255, 255 },
        Color{ 255, 192, 84, 255 },
        Color{ 255, 124, 44, 255 },
        Color{ 255, 56, 40, 255 }
    };

    nivel = Clamp(nivel, 0.0f, 3.0f);
    int i = (int)nivel;
    if (i >= 3) return COLORES[3];
    return ColorLerp(COLORES[i], COLORES[i + 1], nivel - i);
}


// Caja con caras sombreadas (volumen sin luces reales).
static void CajaFacetadaCarga(Vector3 centro, Vector3 tamano, Color color, float brilloTapa = 1.22f)
{
    float x0 = centro.x - tamano.x * 0.5f;
    float x1 = centro.x + tamano.x * 0.5f;
    float y0 = centro.y - tamano.y * 0.5f;
    float y1 = centro.y + tamano.y * 0.5f;
    float z0 = centro.z - tamano.z * 0.5f;
    float z1 = centro.z + tamano.z * 0.5f;

    Color tapa = TinteCarga(color, brilloTapa);
    Color frente = TinteCarga(color, 0.84f);
    Color atras = TinteCarga(color, 0.58f);
    Color derecha = TinteCarga(color, 0.64f);
    Color izquierda = TinteCarga(color, 0.74f);

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


// Disco de luz que se desvanece hacia el borde (usar en modo aditivo).
static void ResplandorCarga(Vector3 centro, Vector3 u, Vector3 v, float radio, Color color)
{
    const int LADOS = 16;
    rlBegin(RL_TRIANGLES);

    for (int k = 0; k < LADOS; k++)
    {
        float a0 = 2.0f * PI * (float)k / (float)LADOS;
        float a1 = 2.0f * PI * (float)(k + 1) / (float)LADOS;
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


static void ResplandorSueloCarga(float x, float y, float z, float radio, Color color)
{
    ResplandorCarga({ x, y, z }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, radio, color);
}


static void ResplandorMuroCarga(float x, float y, float z, float radio, Color color)
{
    ResplandorCarga({ x, y, z }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, radio, color);
}


static const float Z_PARED_CARGA = -13.6f;
static const float X_HORNOS_CARGA[5] = { -12.0f, -6.0f, 0.0f, 6.0f, 12.0f };


static Camera3D ObtenerCamaraCarga()
{
    Camera3D camara{};
    camara.position = { 0.0f, 16.0f, 12.5f };
    camara.target = { 0.0f, 0.8f, 0.8f };
    camara.up = { 0.0f, 1.0f, 0.0f };
    camara.fovy = 46.0f;
    camara.projection = CAMERA_PERSPECTIVE;
    return camara;
}


static void DibujarSueloCarga(float tiempo)
{
    // Base oscura: se ve en las juntas entre chapas.
    (DrawCube)({ 0.0f, -0.32f, 0.0f }, 50.0f, 0.4f, 36.0f, Color{ 18, 14, 16, 255 });

    // Chapas remachadas con variacion de tono.
    const float LADO = 3.0f;

    for (int fila = 0; fila < 10; fila++)
    {
        for (int col = 0; col < 16; col++)
        {
            float x = -24.0f + LADO * 0.5f + col * LADO;
            float z = -14.5f + LADO * 0.5f + fila * LADO;

            // Dentro del anillo va la cubierta circular.
            if (x * x + z * z < 10.2f * 10.2f) continue;

            float h = HashCarga(col, fila);
            Color chapa = TinteCarga(Color{ 58, 50, 52, 255 }, 0.82f + 0.3f * h);
            (DrawCube)({ x, -0.1f, z }, LADO - 0.08f, 0.04f, LADO - 0.08f, chapa);

            // Mancha de hollin en algunas chapas.
            if (h > 0.78f)
            {
                (DrawCube)({ x + 0.3f, -0.075f, z - 0.2f }, 1.2f, 0.01f, 0.9f, Color{ 36, 30, 32, 255 });
            }
        }
    }

    // Cubierta circular del anillo, con aros de chapa.
    (DrawCylinder)({ 0.0f, -0.12f, 0.0f }, 10.4f, 10.6f, 0.22f, 48, Color{ 70, 60, 64, 255 });
    (DrawCylinder)({ 0.0f, 0.09f, 0.0f }, 10.4f, 10.4f, 0.03f, 48, Color{ 82, 72, 74, 255 });
    DrawCircle3D({ 0.0f, 0.13f, 0.0f }, 10.0f, { 1.0f, 0.0f, 0.0f }, 90.0f, Color{ 150, 96, 52, 255 });
    DrawCircle3D({ 0.0f, 0.13f, 0.0f }, 8.6f, { 1.0f, 0.0f, 0.0f }, 90.0f, Fade(Color{ 30, 24, 26, 255 }, 0.8f));

    // Franjas de peligro en el borde del anillo.
    if (CalidadDecoracion() != CALIDAD_BAJA)
    {
        for (int k = 0; k < 40; k++)
        {
            if (k % 2 != 0) continue;

            float a = k * 2.0f * PI / 40.0f;
            rlPushMatrix();
            rlTranslatef(std::cos(a) * 10.2f, 0.115f, std::sin(a) * 10.2f);
            rlRotatef(-a * RAD2DEG + 35.0f, 0.0f, 1.0f, 0.0f);
            (DrawCube)({ 0.0f, 0.0f, 0.0f }, 0.28f, 0.02f, 0.75f, Color{ 214, 170, 70, 255 });
            rlPopMatrix();
        }
    }

    // Foso de metal fundido entre la caldera y las plataformas.
    float latido = 0.86f + 0.14f * std::sin(tiempo * 2.2f);
    (DrawCylinder)({ 0.0f, 0.10f, 0.0f }, 5.0f, 5.0f, 0.04f, 40, Color{ 40, 22, 18, 255 });
    (DrawCylinder)(
        { 0.0f, 0.12f, 0.0f }, 4.75f, 4.75f, 0.04f, 40,
        Color{ CanalCarga(240 * latido), CanalCarga(96 * latido), 28, 255 }
    );

    // Costras oscuras que derivan sobre el metal fundido.
    for (int k = 0; k < 9; k++)
    {
        float a = k * 0.7f + tiempo * 0.05f * (k % 2 == 0 ? 1.0f : -1.0f);
        float r = 3.75f + 0.55f * HashCarga(k, 3);
        (DrawCube)(
            { std::cos(a) * r, 0.155f, std::sin(a) * r },
            0.5f + 0.4f * HashCarga(k, 7), 0.02f, 0.4f + 0.3f * HashCarga(k, 9),
            Color{ 92, 40, 24, 255 }
        );
    }
}


static void DibujarHornoCarga(float x, int indice, float tiempo)
{
    float z = Z_PARED_CARGA + 1.1f;
    float parpadeo = 0.72f + 0.18f * std::sin(tiempo * 5.0f + indice * 1.7f) + 0.10f * std::sin(tiempo * 13.0f + indice);

    // Cuerpo del horno con zocalo y corona.
    CajaFacetadaCarga({ x, 2.2f, z }, { 3.6f, 4.4f, 2.2f }, Color{ 64, 52, 54, 255 });
    CajaFacetadaCarga({ x, 0.25f, z + 0.1f }, { 3.9f, 0.5f, 2.5f }, Color{ 44, 36, 38, 255 });
    CajaFacetadaCarga({ x, 4.5f, z }, { 3.9f, 0.3f, 2.4f }, Color{ 112, 74, 50, 255 });

    // Boca encendida con rejilla.
    CajaFacetadaCarga({ x, 1.7f, z + 1.06f }, { 2.4f, 1.9f, 0.1f }, Color{ 28, 22, 24, 255 });
    (DrawCube)(
        { x, 1.6f, z + 1.12f }, 2.0f, 1.5f, 0.04f,
        Color{ CanalCarga(255 * parpadeo), CanalCarga(132 * parpadeo), CanalCarga(36 * parpadeo), 255 }
    );

    for (int b = 0; b < 5; b++)
    {
        (DrawCube)({ x - 0.8f + b * 0.4f, 1.6f, z + 1.17f }, 0.08f, 1.6f, 0.06f, Color{ 34, 28, 30, 255 });
    }

    // Manometro: aguja que tiembla (solo decorado, no mide nada del juego).
    Vector3 dial = { x + 1.2f, 3.5f, z + 1.14f };
    (DrawCylinder)({ dial.x, dial.y, dial.z - 0.05f }, 0.0f, 0.0f, 0.0f, 3, BLANK);
    rlPushMatrix();
    rlTranslatef(dial.x, dial.y, dial.z);
    rlRotatef(90.0f, 1.0f, 0.0f, 0.0f);
    (DrawCylinder)({ 0.0f, 0.0f, 0.0f }, 0.42f, 0.42f, 0.08f, 14, Color{ 150, 110, 60, 255 });
    (DrawCylinder)({ 0.0f, 0.05f, 0.0f }, 0.34f, 0.34f, 0.04f, 14, Color{ 226, 216, 190, 255 });
    rlPopMatrix();

    float aguja = 0.6f + 0.5f * std::sin(tiempo * 1.3f + indice * 2.1f) + 0.06f * std::sin(tiempo * 23.0f + indice);
    DrawLine3D(
        { dial.x, dial.y, dial.z + 0.1f },
        { dial.x + std::cos(PI - aguja) * 0.28f, dial.y + std::sin(PI - aguja) * 0.28f, dial.z + 0.1f },
        Color{ 180, 40, 30, 255 }
    );

    // Chimenea hacia el techo.
    (DrawCylinder)({ x, 4.6f, z - 0.3f }, 0.55f, 0.65f, 6.0f, 10, Color{ 56, 46, 48, 255 });
    (DrawCylinder)({ x, 6.4f, z - 0.3f }, 0.72f, 0.72f, 0.25f, 10, Color{ 120, 84, 54, 255 });
}


static void DibujarParedFondoCarga(float tiempo)
{
    // Pared de ladrillo.
    CajaFacetadaCarga({ 0.0f, 6.0f, Z_PARED_CARGA - 0.5f }, { 52.0f, 13.0f, 1.0f }, Color{ 74, 42, 36, 255 }, 1.0f);

    NivelCalidadGrafica calidad = CalidadDecoracion();

    if (calidad != CALIDAD_BAJA)
    {
        // Ladrillos sueltos con otro tono y juntas horizontales.
        int paso = calidad == CALIDAD_ALTA ? 1 : 2;

        for (int fila = 0; fila < 20; fila += paso)
        {
            float y = 0.3f + fila * 0.6f;
            DrawLine3D({ -26.0f, y, Z_PARED_CARGA + 0.01f }, { 26.0f, y, Z_PARED_CARGA + 0.01f }, Color{ 44, 26, 24, 255 });

            for (int col = 0; col < 34; col++)
            {
                float h = HashCarga(col, fila);
                if (h < 0.62f) continue;

                float x = -25.5f + col * 1.5f + (fila % 2 == 0 ? 0.0f : 0.75f);
                (DrawCube)(
                    { x, y + 0.3f, Z_PARED_CARGA + 0.02f }, 1.36f, 0.5f, 0.04f,
                    TinteCarga(Color{ 96, 52, 42, 255 }, 0.8f + 0.4f * HashCarga(fila, col))
                );
            }
        }
    }

    for (int i = 0; i < 5; i++)
    {
        DibujarHornoCarga(X_HORNOS_CARGA[i], i, tiempo);
    }

    // Tuberias horizontales con bridas y valvulas.
    Color cobre = Color{ 150, 92, 56, 255 };
    Color hierro = Color{ 82, 76, 80, 255 };
    float zTubo = Z_PARED_CARGA + 0.45f;

    DrawCylinderEx({ -26.0f, 7.6f, zTubo }, { 26.0f, 7.6f, zTubo }, 0.32f, 0.32f, 10, cobre);
    DrawCylinderEx({ -26.0f, 8.6f, zTubo }, { 26.0f, 8.6f, zTubo }, 0.22f, 0.22f, 8, hierro);

    for (int k = 0; k < 13; k++)
    {
        float x = -24.0f + k * 4.0f;
        DrawCylinderEx({ x - 0.1f, 7.6f, zTubo }, { x + 0.1f, 7.6f, zTubo }, 0.42f, 0.42f, 10, TinteCarga(cobre, 0.7f));
    }

    // Bajadas entre hornos con rueda de valvula.
    for (int k = 0; k < 4; k++)
    {
        float x = (X_HORNOS_CARGA[k] + X_HORNOS_CARGA[k + 1]) * 0.5f;
        DrawCylinderEx({ x, 0.0f, zTubo + 0.2f }, { x, 7.6f, zTubo + 0.2f }, 0.24f, 0.24f, 8, cobre);

        Vector3 rueda = { x, 3.0f, zTubo + 0.55f };
        DrawCircle3D(rueda, 0.42f, { 0.0f, 1.0f, 0.0f }, 0.0f, Color{ 190, 50, 40, 255 });
        DrawCircle3D(rueda, 0.38f, { 0.0f, 1.0f, 0.0f }, 0.0f, Color{ 190, 50, 40, 255 });
        DrawLine3D({ x - 0.4f, 3.0f, rueda.z }, { x + 0.4f, 3.0f, rueda.z }, Color{ 190, 50, 40, 255 });
        DrawLine3D({ x, 2.6f, rueda.z }, { x, 3.4f, rueda.z }, Color{ 190, 50, 40, 255 });
    }
}


static void DibujarLateralesCarga()
{
    if (CalidadDecoracion() == CALIDAD_BAJA)
    {
        return;
    }

    // Tanques de presion acostados a los costados, con zunchos y patas.
    for (int lado = -1; lado <= 1; lado += 2)
    {
        float x = lado * 16.5f;
        Color tanque = Color{ 92, 84, 88, 255 };

        DrawCylinderEx({ x, 1.9f, -8.0f }, { x, 1.9f, 3.5f }, 1.7f, 1.7f, 16, tanque);
        DrawSphereEx({ x, 1.9f, -8.0f }, 1.7f, 10, 10, TinteCarga(tanque, 0.8f));
        DrawSphereEx({ x, 1.9f, 3.5f }, 1.7f, 10, 10, TinteCarga(tanque, 0.9f));

        for (int b = 0; b < 4; b++)
        {
            float z = -6.8f + b * 3.0f;
            DrawCylinderEx({ x, 1.9f, z - 0.12f }, { x, 1.9f, z + 0.12f }, 1.78f, 1.78f, 16, Color{ 140, 92, 54, 255 });
            CajaFacetadaCarga({ x, 0.3f, z }, { 2.6f, 0.6f, 0.4f }, Color{ 50, 42, 44, 255 });
        }

        // Pilas de carbon junto a los tanques.
        for (int p = 0; p < 6; p++)
        {
            float h = HashCarga(p, lado + 5);
            CajaFacetadaCarga(
                { lado * (12.4f + h * 1.4f), 0.2f + 0.15f * HashCarga(p, 2), 5.5f + p * 0.6f - h },
                { 0.7f + h * 0.4f, 0.5f + 0.4f * h, 0.7f }, Color{ 34, 32, 36, 255 }, 1.4f
            );
        }
    }

    // Lamparas colgantes fuera del anillo.
    for (int k = 0; k < 4; k++)
    {
        float x = k < 2 ? -12.0f : 12.0f;
        float z = (k % 2 == 0) ? -6.5f : 3.0f;

        DrawLine3D({ x, 13.0f, z }, { x, 6.3f, z }, Color{ 40, 36, 38, 255 });
        (DrawCylinder)({ x, 5.6f, z }, 0.18f, 0.75f, 0.6f, 10, Color{ 60, 70, 60, 255 });
        DrawSphereEx({ x, 5.6f, z }, 0.22f, 8, 8, Color{ 255, 224, 150, 255 });
    }
}


static void DibujarCalderaCentralCarga(float tiempo)
{
    // Cuerpo de hierro con zunchos y remaches.
    (DrawCylinder)({ 0.0f, 0.0f, 0.0f }, 3.15f, 3.4f, 0.95f, 20, Color{ 66, 58, 62, 255 });
    (DrawCylinder)({ 0.0f, 0.82f, 0.0f }, 3.3f, 3.3f, 0.18f, 20, Color{ 132, 88, 54, 255 });
    (DrawCylinder)({ 0.0f, 0.25f, 0.0f }, 3.36f, 3.36f, 0.12f, 20, Color{ 46, 40, 44, 255 });

    if (CalidadDecoracion() != CALIDAD_BAJA)
    {
        for (int k = 0; k < 20; k++)
        {
            float a = k * 2.0f * PI / 20.0f;
            DrawSphereEx({ std::cos(a) * 3.32f, 0.55f, std::sin(a) * 3.32f }, 0.07f, 4, 4, Color{ 170, 130, 90, 255 });
        }
    }

    // Metal fundido burbujeante.
    float brillo = 0.82f + 0.18f * std::sin(tiempo * 3.0f);
    (DrawCylinder)(
        { 0.0f, 0.9f, 0.0f }, 2.8f, 2.8f, 0.08f, 24,
        Color{ CanalCarga(255 * brillo), CanalCarga(120 * brillo), 34, 255 }
    );
    (DrawCylinder)({ 0.0f, 0.95f, 0.0f }, 1.3f, 1.3f, 0.04f, 18, Color{ 255, 214, 120, 255 });

    for (int b = 0; b < 6; b++)
    {
        float fase = std::fmod(tiempo * 0.8f + b * 0.19f, 1.0f);
        DrawSphereEx(
            { std::cos(b * 1.9f) * 1.8f, 0.95f + fase * 0.22f, std::sin(b * 1.9f) * 1.8f },
            0.2f * (1.0f - fase * 0.5f), 6, 6,
            Fade(Color{ 255, 220, 120, 255 }, 1.0f - fase)
        );
    }

    // Soportes en cruz que sostienen la carga dormida.
    for (int k = 0; k < 4; k++)
    {
        float a = k * PI * 0.5f + PI * 0.25f;
        DrawCylinderEx(
            { std::cos(a) * 2.9f, 0.9f, std::sin(a) * 2.9f },
            { std::cos(a) * 0.9f, 1.75f, std::sin(a) * 0.9f },
            0.1f, 0.1f, 6, Color{ 70, 62, 66, 255 }
        );
    }
}


static void DibujarPuentePlataformaCarga(float angulo, float tiempo, int indice)
{
    float radioMedio = (3.2f + RADIO_ANILLO_CARGA - RADIO_PLATAFORMA_CARGA) * 0.5f;
    float largo = RADIO_ANILLO_CARGA - RADIO_PLATAFORMA_CARGA - 3.2f + 0.6f;

    rlPushMatrix();
    rlTranslatef(std::cos(angulo) * radioMedio, 0.0f, std::sin(angulo) * radioMedio);
    rlRotatef(-angulo * RAD2DEG, 0.0f, 1.0f, 0.0f);

    CajaFacetadaCarga({ 0.0f, 0.55f, 0.0f }, { largo, 0.16f, 1.0f }, Color{ 92, 86, 92, 255 });

    // Rejilla y barandas.
    for (int k = 0; k < 5; k++)
    {
        float x = -largo * 0.5f + 0.25f + k * (largo - 0.5f) / 4.0f;
        (DrawCube)({ x, 0.64f, 0.0f }, 0.05f, 0.02f, 0.9f, Color{ 52, 48, 52, 255 });
    }

    for (int lado = -1; lado <= 1; lado += 2)
    {
        (DrawCube)({ 0.0f, 0.95f, lado * 0.48f }, largo, 0.06f, 0.06f, Color{ 150, 100, 60, 255 });
        (DrawCube)({ -largo * 0.5f + 0.1f, 0.78f, lado * 0.48f }, 0.07f, 0.36f, 0.07f, Color{ 70, 62, 66, 255 });
        (DrawCube)({ largo * 0.5f - 0.1f, 0.78f, lado * 0.48f }, 0.07f, 0.36f, 0.07f, Color{ 70, 62, 66, 255 });

        // Luces de borde que laten suave.
        float latido = 0.6f + 0.4f * std::sin(tiempo * 2.5f + indice + lado);
        (DrawCube)({ 0.0f, 0.65f, lado * 0.5f }, largo * 0.9f, 0.04f, 0.04f, AlfaCarga(Color{ 255, 150, 60, 255 }, 0.5f + 0.4f * latido));
    }

    rlPopMatrix();
}


static void DibujarPlataformasCarga(
    const MinijuegoCargaInestable& minijuego,
    const Participante participantes[]
)
{
    float tiempo = minijuego.tiempoAnimacion;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!minijuego.resultado.participantes[i].participo)
        {
            continue;
        }

        const EstadoJugadorCargaInestable& estado = minijuego.jugadores[i];
        Vector3 centro = PosicionPlataformaCarga(minijuego, i);
        float angulo = AnguloPlataformaCarga(minijuego, i);
        Color color = participantes[i].color;
        float altura = estado.eliminado ? 0.5f : ALTURA_PLATAFORMA_CARGA;

        DibujarPuentePlataformaCarga(angulo, tiempo, i);

        // Pedestal: zocalo, cuerpo y aro del color del jugador.
        Color metal = estado.eliminado ? Color{ 34, 28, 30, 255 } : Color{ 104, 96, 104, 255 };
        (DrawCylinder)({ centro.x, 0.0f, centro.z }, RADIO_PLATAFORMA_CARGA + 0.3f, RADIO_PLATAFORMA_CARGA + 0.4f, 0.3f, 20, Color{ 42, 36, 40, 255 });
        (DrawCylinder)(centro, RADIO_PLATAFORMA_CARGA, RADIO_PLATAFORMA_CARGA + 0.15f, altura, 20, metal);
        (DrawCylinder)(
            { centro.x, altura - 0.12f, centro.z },
            RADIO_PLATAFORMA_CARGA + 0.04f, RADIO_PLATAFORMA_CARGA + 0.04f, 0.1f, 20,
            estado.eliminado ? Color{ 60, 40, 36, 255 } : color
        );
        (DrawCylinder)(
            { centro.x, altura, centro.z },
            RADIO_PLATAFORMA_CARGA * 0.78f, RADIO_PLATAFORMA_CARGA * 0.78f, 0.02f, 20,
            estado.eliminado ? Color{ 24, 20, 22, 255 } : TinteCarga(metal, 1.15f)
        );
        DrawCircle3D(
            { centro.x, altura + 0.03f, centro.z }, RADIO_PLATAFORMA_CARGA * 0.62f,
            { 1.0f, 0.0f, 0.0f }, 90.0f,
            estado.eliminado ? Fade(Color{ 255, 110, 50, 255 }, 0.45f) : Fade(color, 0.8f)
        );

        // Jugador sobre la plataforma (aplastado si exploto).
        JugadorPrueba figura{};
        figura.posicion = { centro.x, altura + figura.tamano.y * 0.5f, centro.z };
        figura.direccionMirada = Vector3Normalize({ -centro.x, 0.0f, -centro.z });
        figura.enSuelo = true;
        figura.aplastado = estado.eliminado;

        Participante visual = participantes[i];
        visual.activo = true;
        visual.conectado = true;
        DibujarJugadorCuboPrueba(figura, visual);

        // Chevrones animados hacia donde puede pasar el portador:
        // dorado = horario (golpe), celeste = antihorario (salto).
        if (minijuego.fase == FASE_CARGA_ACTIVA && i == minijuego.portador && minijuego.bloqueoPase <= 0.0f)
        {
            Vector3 tangente = { -std::sin(angulo), 0.0f, std::cos(angulo) };
            Vector3 radial = { std::cos(angulo), 0.0f, std::sin(angulo) };

            for (int sentido = -1; sentido <= 1; sentido += 2)
            {
                Color colorFlecha = sentido > 0 ? Color{ 255, 206, 96, 255 } : Color{ 120, 214, 255, 255 };

                for (int k = 0; k < 3; k++)
                {
                    float onda = std::fmod(tiempo * 2.5f - k * 0.33f + 3.0f, 1.0f);
                    float distancia = RADIO_PLATAFORMA_CARGA + 0.45f + k * 0.45f;
                    Vector3 punta = {
                        centro.x + tangente.x * sentido * (distancia + 0.22f),
                        altura + 0.06f,
                        centro.z + tangente.z * sentido * (distancia + 0.22f)
                    };
                    Vector3 base = {
                        centro.x + tangente.x * sentido * distancia,
                        altura + 0.06f,
                        centro.z + tangente.z * sentido * distancia
                    };
                    Vector3 a = Vector3Add(base, Vector3Scale(radial, 0.22f));
                    Vector3 b = Vector3Subtract(base, Vector3Scale(radial, 0.22f));
                    Color c = AlfaCarga(colorFlecha, 0.35f + 0.65f * (1.0f - onda));

                    DrawTriangle3D(a, punta, b, c);
                    DrawTriangle3D(b, punta, a, c);
                }
            }
        }
    }
}


// Carga blindada: nucleo que brilla segun el escalon, zunchos de hierro,
// valvula con mecha y grietas que se encienden en los escalones altos.
static void DibujarCarga3D(
    Vector3 posicion,
    float nivel,
    float destello,
    float tiempo,
    bool inerte
)
{
    float r = 0.44f + 0.05f * nivel + 0.05f * destello + 0.012f * std::sin(tiempo * 7.0f);
    Color nucleo = inerte ? Color{ 110, 200, 255, 255 } : ColorNivelCarga(nivel);
    Color brillante = ColorLerp(nucleo, WHITE, 0.25f + 0.35f * destello);

    DrawSphereEx(posicion, r, 14, 14, brillante);

    // Zunchos: ecuador y dos meridianos que giran despacio.
    Color hierro = Color{ 58, 52, 56, 255 };
    rlPushMatrix();
    rlTranslatef(posicion.x, posicion.y, posicion.z);
    rlRotatef(tiempo * 40.0f, 0.0f, 1.0f, 0.0f);
    (DrawCylinder)({ 0.0f, -0.07f, 0.0f }, r * 1.04f, r * 1.04f, 0.14f, 16, hierro);

    for (int m = 0; m < 2; m++)
    {
        rlPushMatrix();
        rlRotatef(m * 90.0f, 0.0f, 1.0f, 0.0f);
        rlRotatef(90.0f, 1.0f, 0.0f, 0.0f);
        (DrawCylinder)({ 0.0f, -0.05f, 0.0f }, r * 1.03f, r * 1.03f, 0.1f, 16, TinteCarga(hierro, 1.2f));
        rlPopMatrix();
    }

    // Remaches del ecuador.
    for (int k = 0; k < 8; k++)
    {
        float a = k * PI * 0.25f;
        DrawCubeV({ std::cos(a) * r * 1.06f, 0.0f, std::sin(a) * r * 1.06f }, { 0.06f, 0.06f, 0.06f }, Color{ 170, 130, 90, 255 });
    }

    rlPopMatrix();

    // Valvula y mecha arriba.
    (DrawCylinder)({ posicion.x, posicion.y + r * 0.85f, posicion.z }, 0.1f, 0.13f, 0.2f, 8, Color{ 150, 100, 60, 255 });
    DrawLine3D(
        { posicion.x, posicion.y + r * 0.85f + 0.2f, posicion.z },
        { posicion.x + 0.08f, posicion.y + r * 0.85f + 0.42f, posicion.z },
        Color{ 40, 34, 30, 255 }
    );

    // Grietas encendidas desde AL ROJO.
    if (!inerte && nivel > 1.4f && CalidadDecoracion() != CALIDAD_BAJA)
    {
        float alfa = Clamp((nivel - 1.4f) / 1.2f, 0.0f, 1.0f);
        Color grieta = AlfaCarga(Color{ 255, 236, 170, 255 }, alfa);

        for (int g = 0; g < 6; g++)
        {
            Vector3 d = Vector3Normalize({ HashCarga(g, 1) - 0.5f, HashCarga(g, 2) - 0.5f, HashCarga(g, 3) - 0.5f + 0.6f });
            Vector3 lateral = Vector3Normalize(Vector3CrossProduct(d, { 0.0f, 1.0f, 0.0f }));
            Vector3 p0 = Vector3Add(posicion, Vector3Scale(d, r * 1.01f));
            Vector3 p1 = Vector3Add(p0, Vector3Scale(lateral, 0.12f));
            p1 = Vector3Add(posicion, Vector3Scale(Vector3Normalize(Vector3Subtract(p1, posicion)), r * 1.01f));
            Vector3 p2 = Vector3Add(p1, { 0.0f, -0.12f, 0.0f });
            p2 = Vector3Add(posicion, Vector3Scale(Vector3Normalize(Vector3Subtract(p2, posicion)), r * 1.01f));
            DrawLine3D(p0, p1, grieta);
            DrawLine3D(p1, p2, grieta);
        }
    }
}


// Halo y chispa de la mecha (modo aditivo).
static void DibujarLuzCarga3D(
    Vector3 posicion,
    float nivel,
    float destello,
    float captura,
    float tiempo,
    bool inerte,
    bool sostenida
)
{
    Color nucleo = inerte ? Color{ 110, 200, 255, 255 } : ColorNivelCarga(nivel);
    float r = 0.44f + 0.05f * nivel;

    DrawSphereEx(posicion, r * (1.7f + 0.25f * nivel), 12, 12, AlfaCarga(nucleo, 0.10f + 0.05f * nivel + 0.12f * destello));
    DrawSphereEx(posicion, r * 1.25f, 10, 10, AlfaCarga(nucleo, 0.16f + 0.18f * destello));

    // Chispa de la mecha: titila siempre igual, no mide nada.
    float titileo = 0.6f + 0.4f * std::sin(tiempo * 31.0f);
    DrawSphereEx(
        { posicion.x + 0.08f, posicion.y + r * 0.85f + 0.44f, posicion.z },
        0.08f + 0.04f * titileo, 6, 6, AlfaCarga(Color{ 255, 220, 140, 255 }, 0.8f)
    );

    if (captura > 0.02f)
    {
        DrawSphereEx(posicion, r * (1.3f + 1.4f * (1.0f - captura)), 10, 10, AlfaCarga(Color{ 255, 240, 200, 255 }, 0.35f * captura));
    }

    // Charco de luz sobre el puente cuando alguien la sostiene.
    if (sostenida)
    {
        ResplandorSueloCarga(posicion.x, 0.67f, posicion.z, 1.6f + 0.3f * nivel, AlfaCarga(nucleo, 0.22f + 0.1f * destello));
    }
}


// Vapor ambiental de las tuberias y humo de las plataformas chamuscadas.
static void DibujarVaporAmbienteCarga(
    const MinijuegoCargaInestable& minijuego,
    float tiempo
)
{
    NivelCalidadGrafica calidad = CalidadDecoracion();

    if (calidad == CALIDAD_BAJA)
    {
        return;
    }

    int puffs = calidad == CALIDAD_ALTA ? 5 : 3;

    // Vapor de la caldera.
    for (int k = 0; k < (calidad == CALIDAD_ALTA ? 7 : 4); k++)
    {
        float f = std::fmod(tiempo * 0.32f + k * 0.15f, 1.0f);
        DrawSphereEx(
            { std::cos(k * 1.05f) * 1.9f + f * 0.4f, 1.1f + f * 4.0f, std::sin(k * 1.05f) * 1.9f },
            0.3f + f * 0.6f, 6, 6,
            Fade(Color{ 210, 204, 200, 255 }, 0.22f * (1.0f - f))
        );
    }

    // Fugas de vapor en las bridas y en las chimeneas.
    for (int fuga = 0; fuga < 4; fuga++)
    {
        float x = -18.0f + fuga * 12.0f;

        for (int k = 0; k < puffs; k++)
        {
            float f = std::fmod(tiempo * 0.45f + k / (float)puffs + fuga * 0.27f, 1.0f);
            DrawSphereEx(
                { x + f * 1.2f, 7.6f + f * 2.6f, Z_PARED_CARGA + 0.9f + f * 0.8f },
                0.2f + f * 0.7f, 6, 6,
                Fade(Color{ 220, 216, 212, 255 }, 0.3f * (1.0f - f))
            );
        }
    }

    for (int i = 0; i < 5; i++)
    {
        for (int k = 0; k < 2; k++)
        {
            float f = std::fmod(tiempo * 0.25f + k * 0.5f + i * 0.21f, 1.0f);
            DrawSphereEx(
                { X_HORNOS_CARGA[i] + std::sin(f * 4.0f + i) * 0.3f, 10.6f + f * 2.4f, Z_PARED_CARGA + 0.8f },
                0.5f + f * 0.8f, 6, 6,
                Fade(Color{ 90, 84, 86, 255 }, 0.35f * (1.0f - f))
            );
        }
    }

    // Humo que sigue saliendo de las plataformas de los eliminados.
    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!minijuego.resultado.participantes[i].participo || !minijuego.jugadores[i].eliminado)
        {
            continue;
        }

        Vector3 centro = PosicionPlataformaCarga(minijuego, i);

        for (int k = 0; k < 3; k++)
        {
            float f = std::fmod(tiempo * 0.4f + k / 3.0f + i * 0.3f, 1.0f);
            DrawSphereEx(
                { centro.x + std::sin(f * 5.0f + k) * 0.4f, 1.0f + f * 2.6f, centro.z + std::cos(f * 4.0f + k) * 0.3f },
                0.22f + f * 0.45f, 6, 6,
                Fade(Color{ 60, 54, 56, 255 }, 0.45f * (1.0f - f))
            );
        }
    }
}


static void DibujarParticulasCarga(
    const MinijuegoCargaInestable& minijuego,
    bool aditivas
)
{
    for (int i = 0; i < MAX_PARTICULAS_CARGA; i++)
    {
        const ParticulaCarga& p = minijuego.particulas[i];

        if (!p.activa)
        {
            continue;
        }

        float vida = Clamp(p.vida / p.vidaMaxima, 0.0f, 1.0f);
        bool esAditiva = p.tipo == PARTICULA_CARGA_CHISPA || p.tipo == PARTICULA_CARGA_FUEGO;

        if (esAditiva != aditivas)
        {
            continue;
        }

        switch (p.tipo)
        {
            case PARTICULA_CARGA_CHISPA:
            {
                float lado = p.tamano * (0.5f + 0.5f * vida);
                DrawCubeV(p.posicion, { lado, lado, lado }, AlfaCarga(p.color, vida));
                break;
            }

            case PARTICULA_CARGA_FUEGO:
            {
                Color c = ColorLerp(Color{ 120, 40, 20, 255 }, p.color, vida);
                DrawSphereEx(p.posicion, p.tamano, 6, 6, AlfaCarga(c, 0.75f * vida));
                break;
            }

            case PARTICULA_CARGA_HUMO:
            case PARTICULA_CARGA_VAPOR:
            {
                float alfa = (p.tipo == PARTICULA_CARGA_HUMO ? 0.55f : 0.45f) * vida;
                DrawSphereEx(p.posicion, p.tamano, 6, 6, AlfaCarga(p.color, alfa));
                break;
            }

            case PARTICULA_CARGA_ESCOMBRO:
            {
                float lado = p.tamano;
                DrawCubeV(p.posicion, { lado, lado * 0.6f, lado }, AlfaCarga(p.color, Clamp(vida * 2.5f, 0.0f, 1.0f)));
                break;
            }
        }
    }
}


static float SuavizarCarga(float t)
{
    t = Clamp(t, 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}


// Bola de fuego, onda expansiva y destello (translucidos).
static void DibujarExplosionCarga(
    const MinijuegoCargaInestable& minijuego,
    float progreso,
    bool aditiva
)
{
    Vector3 c = minijuego.posicionExplosion;
    float p = progreso;
    float abre = 1.0f - (1.0f - p) * (1.0f - p);

    if (!aditiva)
    {
        // Bola que pasa de fuego a humo negro.
        Color bola = ColorLerp(Color{ 255, 150, 50, 255 }, Color{ 50, 42, 44, 255 }, SuavizarCarga(p * 1.6f));
        DrawSphereEx(c, 0.7f + 2.6f * abre, 14, 14, AlfaCarga(bola, 0.65f * (1.0f - p)));
        DrawSphereEx({ c.x, c.y + 0.8f * p, c.z }, 0.5f + 2.0f * abre, 12, 12, AlfaCarga(Color{ 40, 34, 36, 255 }, 0.5f * p * (1.0f - p) * 2.0f));
        return;
    }

    float resto = 1.0f - p;
    DrawSphereEx(c, 0.4f + 1.8f * abre, 12, 12, AlfaCarga(Color{ 255, 236, 170, 255 }, 0.85f * resto * resto));

    // Onda sobre el piso y luz del fogonazo.
    ResplandorSueloCarga(c.x, 0.2f, c.z, 2.0f + 8.0f * abre, AlfaCarga(Color{ 255, 140, 50, 255 }, 0.6f * resto));
    DrawCircle3D({ c.x, 0.22f, c.z }, 1.0f + 6.5f * abre, { 1.0f, 0.0f, 0.0f }, 90.0f, AlfaCarga(Color{ 255, 206, 96, 255 }, resto));
    DrawCircle3D({ c.x, 0.22f, c.z }, 0.6f + 5.0f * abre, { 1.0f, 0.0f, 0.0f }, 90.0f, AlfaCarga(Color{ 255, 90, 50, 255 }, resto));
    ResplandorMuroCarga(0.0f, 4.0f, Z_PARED_CARGA + 0.05f, 10.0f, AlfaCarga(Color{ 255, 130, 50, 255 }, 0.25f * resto));
}


// Luces falsas de la sala (modo aditivo).
static void DibujarLucesSalaCarga(float tiempo)
{
    float latido = 0.85f + 0.15f * std::sin(tiempo * 2.2f);

    // Foso y caldera.
    ResplandorSueloCarga(0.0f, 0.16f, 0.0f, 6.4f * latido, Color{ 255, 120, 40, 46 });
    ResplandorSueloCarga(0.0f, 0.99f, 0.0f, 3.0f, Color{ 255, 190, 90, 60 });

    // Bocas de los hornos sobre el piso y la pared.
    for (int i = 0; i < 5; i++)
    {
        float parpadeo = 0.75f + 0.25f * std::sin(tiempo * 5.0f + i * 1.7f);
        float x = X_HORNOS_CARGA[i];
        ResplandorSueloCarga(x, -0.06f, Z_PARED_CARGA + 3.4f, 3.2f * parpadeo, Color{ 255, 130, 45, 70 });
        ResplandorMuroCarga(x, 1.7f, Z_PARED_CARGA + 2.25f, 1.8f * parpadeo, Color{ 255, 170, 70, 80 });
    }

    // Lamparas colgantes.
    for (int k = 0; k < 4; k++)
    {
        float x = k < 2 ? -12.0f : 12.0f;
        float z = (k % 2 == 0) ? -6.5f : 3.0f;
        ResplandorSueloCarga(x, -0.06f, z, 3.6f, Color{ 255, 214, 140, 46 });
        DrawSphereEx({ x, 5.6f, z }, 0.55f, 8, 8, Color{ 255, 214, 140, 40 });
    }
}


void MinijuegoCargaInestable::Dibujar(
    const Participante participantes[]
) const
{
    ClearBackground(Color{ 18, 12, 14, 255 });

    float progresoExplosion = 1.0f - Clamp(
        tiempoExplosion / DURACION_EXPLOSION_CARGA,
        0.0f,
        1.0f
    );

    // Intro de la cuenta: la camara baja hasta su lugar fijo.
    Camera3D camara = ObtenerCamaraCarga();

    if (fase == FASE_CARGA_PREPARACION)
    {
        float u = 1.0f - tiempoPreparacion / DURACION_PREPARACION_CARGA;
        float resto = (1.0f - SuavizarCarga(u * 1.2f)) * ObtenerFactorMovimientoCamaraMinijuegos();
        camara.position.y += 3.0f * resto;
        camara.position.z += 3.5f * resto;
    }

    BeginMode3D(camara);

    // Escenario hecho a mano: triangulos sin cara trasera definida.
    rlDrawRenderBatchActive();
    rlDisableBackfaceCulling();

    DibujarSueloCarga(tiempoAnimacion);
    DibujarParedFondoCarga(tiempoAnimacion);
    DibujarLateralesCarga();
    DibujarCalderaCentralCarga(tiempoAnimacion);
    DibujarPlataformasCarga(*this, participantes);

    Vector3 posicionCarga{};
    bool hayCarga = PosicionCargaActual(*this, tiempoAnimacion, 0.0f, posicionCarga);
    bool inerte = fase == FASE_CARGA_PREPARACION;
    float nivel = inerte ? 0.0f : nivelVisual;

    if (hayCarga)
    {
        DibujarCarga3D(posicionCarga, nivel, destelloTick, tiempoAnimacion, inerte);
    }

    DibujarParticulasCarga(*this, false);
    rlDrawRenderBatchActive();
    rlEnableBackfaceCulling();

    // Translucidos sin escribir profundidad.
    rlDisableDepthMask();
    DibujarVaporAmbienteCarga(*this, tiempoAnimacion);

    // Estela del pase: copias de la carga que se desvanecen sobre el arco.
    if (hayCarga && fase == FASE_CARGA_ACTIVA && progresoPase < 1.0f)
    {
        Color estela = ColorNivelCarga(nivel);

        for (int k = 1; k <= 6; k++)
        {
            Vector3 atras{};
            PosicionCargaActual(*this, tiempoAnimacion, k * 0.05f, atras);
            DrawSphereEx(atras, 0.4f - k * 0.04f, 8, 8, AlfaCarga(estela, 0.4f - k * 0.055f));
        }
    }

    if (fase == FASE_CARGA_EXPLOSION && portadorExplosion >= 0)
    {
        DibujarExplosionCarga(*this, progresoExplosion, false);
    }

    rlDrawRenderBatchActive();

    // Luz calida falsa (aditiva).
    if (FactorCalidadGrafica(CalidadEfectos()) >= 0.5f)
    {
        BeginBlendMode(BLEND_ADDITIVE);
        DibujarLucesSalaCarga(tiempoAnimacion);

        if (hayCarga)
        {
            DibujarLuzCarga3D(
                posicionCarga, nivel, destelloTick, destelloCaptura, tiempoAnimacion, inerte,
                fase == FASE_CARGA_ACTIVA && progresoPase >= 1.0f
            );
        }

        if (fase == FASE_CARGA_EXPLOSION && portadorExplosion >= 0)
        {
            DibujarExplosionCarga(*this, progresoExplosion, true);
        }

        DibujarParticulasCarga(*this, true);
        rlDrawRenderBatchActive();
        EndBlendMode();
    }
    else
    {
        // Calidad baja: sin resplandores, pero chispas y fuego se ven.
        DibujarParticulasCarga(*this, true);
        rlDrawRenderBatchActive();
    }

    rlEnableDepthMask();

    EndMode3D();

    // Fogonazo al estallar.
    if (fase == FASE_CARGA_EXPLOSION && progresoExplosion < 0.25f)
    {
        DibujarVeloUI(0.55f * (1.0f - progresoExplosion / 0.25f), Color{ 255, 236, 200, 255 });
    }

    // Rotulos sobre cada jugador.
    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!resultado.participantes[i].participo)
        {
            continue;
        }

        Vector3 centro = PosicionPlataformaCarga(*this, i);
        Vector2 pantalla = GetWorldToScreen({ centro.x, 3.3f, centro.z }, camara);
        char texto[8];
        std::snprintf(texto, sizeof(texto), "J%d", participantes[i].numeroJugador);

        OpcionesEtiquetaUI opciones;
        opciones.borde = jugadores[i].eliminado ? Color{ 110, 100, 100, 255 } : participantes[i].color;
        opciones.pico = true;
        opciones.alfa = jugadores[i].eliminado ? 0.6f : 1.0f;
        DibujarEtiquetaUI(texto, pantalla, jugadores[i].eliminado ? -8.0f : -5.0f, opciones);
    }

    // Aviso al humano que la tiene.
    if (
        fase == FASE_CARGA_ACTIVA && portador >= 0 && progresoPase >= 1.0f &&
        !participantes[portador].esBot && participantes[portador].conectado
    )
    {
        Vector2 pantalla = GetWorldToScreen(
            { posicionCarga.x, posicionCarga.y + 1.3f, posicionCarga.z }, camara
        );
        float alfa = 0.75f + 0.25f * std::sin(tiempoAnimacion * 8.0f);
        DibujarTextoMundoContornoUI("¡PASALA!", pantalla.x, pantalla.y - Px(20.0f), UI_TEXTO, UI_FOCO, alfa);
    }

    // Estallido sobre quien la tenia.
    if (fase == FASE_CARGA_EXPLOSION && portadorExplosion >= 0)
    {
        Vector2 pantalla = GetWorldToScreen(
            { posicionExplosion.x, posicionExplosion.y + 1.6f, posicionExplosion.z }, camara
        );
        float alfa = progresoExplosion < 0.75f ? 1.0f : (1.0f - progresoExplosion) / 0.25f;
        DibujarTextoMundoContornoUI(
            "¡BOOM!", pantalla.x, pantalla.y - Px(30.0f + 40.0f * progresoExplosion),
            UI_TEXTO_TITULO, UI_PELIGRO, alfa
        );
    }

    // Anuncio de carga nueva desde la segunda ronda.
    if (fase == FASE_CARGA_ACTIVA && numeroRonda >= 2 && tiempoRonda < 1.3f)
    {
        float alfa = tiempoRonda < 1.0f ? 1.0f : (1.3f - tiempoRonda) / 0.3f;
        DibujarTextoMundoContornoUI(
            "¡NUEVA CARGA!", GetScreenWidth() * 0.5f, GetScreenHeight() * 0.24f,
            UI_TEXTO_GRANDE, UI_TEXTO_MUNDO, alfa
        );
    }

    // HUD comun (Minigames/HudMinijuego): solo lee el estado.
    DatosHudMinijuego hud;
    PrepararDatosHudMinijuego(
        hud,
        "CARGA INESTABLE",
        "PASÁ LA CARGA ANTES DE QUE EXPLOTE. SI TE EXPLOTA EN LAS MANOS, QUEDÁS FUERA.",
        resultado
    );

    hud.fase = fase == FASE_CARGA_PREPARACION
        ? HUD_MINIJUEGO_CUENTA
        : (fase == FASE_CARGA_TERMINADO ? HUD_MINIJUEGO_FIN : HUD_MINIJUEGO_JUGANDO);
    hud.cuentaRestante = tiempoPreparacion;

    // Sin reloj: el cartel central muestra el escalon de calor (no el tiempo).
    char etiquetaMedidor[32];
    std::snprintf(
        etiquetaMedidor, sizeof(etiquetaMedidor), "CALOR - RONDA %d",
        numeroRonda > 0 ? numeroRonda : 1
    );
    hud.reloj = HUD_RELOJ_MEDIDOR;
    hud.etiquetaMedidor = etiquetaMedidor;

    if (fase == FASE_CARGA_EXPLOSION)
    {
        std::snprintf(hud.valorMedidor, sizeof(hud.valorMedidor), "¡BOOM!");
        hud.progresoMedidor = 1.0f;
    }
    else if (fase == FASE_CARGA_ACTIVA)
    {
        std::snprintf(hud.valorMedidor, sizeof(hud.valorMedidor), "%s", NOMBRE_NIVEL_CARGA[nivelCalor]);
        hud.progresoMedidor = (nivelCalor + 1) / 4.0f;
    }
    else
    {
        std::snprintf(hud.valorMedidor, sizeof(hud.valorMedidor), "FRÍA");
        hud.progresoMedidor = 0.0f;
    }

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        JugadorHudMinijuego& jugador = hud.jugadores[i];
        if (!jugador.visible) continue;

        const EstadoJugadorCargaInestable& estado = jugadores[i];
        jugador.eliminado = estado.eliminado;
        jugador.valorNumerico = estado.cantidadPases;
        std::snprintf(
            jugador.valor, sizeof(jugador.valor),
            estado.cantidadPases == 1 ? "%d PASE" : "%d PASES", estado.cantidadPases
        );

        if (fase == FASE_CARGA_ACTIVA && i == portador)
        {
            std::snprintf(jugador.estado, sizeof(jugador.estado), "¡LA TENÉS!");
        }

        if (estado.eliminado)
        {
            std::snprintf(jugador.detalleFinal, sizeof(jugador.detalleFinal), "EXPLOTÓ EN RONDA %d", estado.rondaEliminacion);
        }
        else
        {
            std::snprintf(jugador.detalleFinal, sizeof(jugador.detalleFinal), "ÚLTIMO EN PIE");
        }

        if (!participantes[i].esBot)
        {
            std::snprintf(
                jugador.controles, sizeof(jugador.controles), "%s: PASÁ HORARIO\n%s: ANTIHORARIO",
                TextoBotonGolpeHud(participantes[i]), ObtenerTextoBotonPrincipal(participantes[i])
            );
        }
    }

    // Sin empates: cada carga elimina a uno solo.
    hud.notaFinal = "ORDEN: CUANTO MÁS TARDE TE EXPLOTA, MÁS ARRIBA QUEDÁS";

    DibujarHudMinijuego(hud, participantes);
}


const ResultadoMinijuego&
MinijuegoCargaInestable::ObtenerResultado() const
{
    return resultado;
}
