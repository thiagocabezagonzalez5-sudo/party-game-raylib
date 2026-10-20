#include "Minigames/MinijuegoCapsulasBarajadas.h"

#include "Minigames/AudioMinijuegos.h"
#include "Minigames/EfectosVisualesMinijuegos.h"
#include "Minigames/HudMinijuego.h"
#include "Minigames/MecanicasJugador.h"
#include "Minigames/ModelosEscenariosRetro3D.h"
#include "Minigames/UtilidadesMinijuegos.h"
#include "Systems/CalidadGrafica.h"
#include "Systems/Input.h"
#include "UI/EstiloUI.h"

#include "raylib.h"
#include "rlgl.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>


//==================================================
// CONSTANTES DE GAMEPLAY
//==================================================

static const float DURACION_PREPARACION_CAPSULAS = 3.0f;
static const float DURACION_MOSTRAR_CAPSULAS = 2.4f;
static const float PAUSA_INICIAL_BARAJADO_CAPSULAS = 0.5f;
static const float DURACION_ELEGIR_CAPSULAS = 4.0f;
static const float DURACION_REVELAR_CAPSULAS = 2.6f;

// Apuesta doble: confirmar dentro de este tiempo vale 2 puntos.
static const float VENTANA_APUESTA_DOBLE_CAPSULAS = 1.0f;

static const int CAPSULAS_POR_RONDA[RONDAS_CAPSULAS] = { 3, 3, 4, 4, 5 };

// Geometria de la mesa y la pasarela.
static const float ALTURA_MESA_CAPSULAS = 1.01f;
static const float ALTURA_PASARELA_CAPSULAS = 0.3f;
static const float Z_PASARELA_CAPSULAS = 4.8f;
static const float Z_MARCADOR_CAPSULAS = 1.5f;


//==================================================
// UTILIDADES
//==================================================

static float LimitarCapsulas(float valor, float minimo, float maximo)
{
    if (valor < minimo) return minimo;
    if (valor > maximo) return maximo;
    return valor;
}


static float SuavizadoCapsulas(float t)
{
    t = LimitarCapsulas(t, 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}


static float AleatorioCapsulas(float minimo, float maximo)
{
    return minimo + (float)GetRandomValue(0, 1000) / 1000.0f * (maximo - minimo);
}


static float SeparacionSlotCapsulas(int cantidad)
{
    if (cantidad <= 3) return 2.8f;
    if (cantidad == 4) return 2.5f;
    return 2.1f;
}


static float XSlotCapsulas(int slot, int cantidad)
{
    return ((float)slot - (float)(cantidad - 1) * 0.5f) * SeparacionSlotCapsulas(cantidad);
}


static float DuracionIntercambioCapsulas(int ronda)
{
    float duracion = 0.62f - 0.08f * (float)ronda;
    return duracion < 0.28f ? 0.28f : duracion;
}


static int LimiteCapsulas(int cantidadMaxima)
{
    return cantidadMaxima < MAX_PARTICIPANTES
        ? cantidadMaxima
        : MAX_PARTICIPANTES;
}


static int NumeroJugadorCapsulas(const Participante& participante, int indice)
{
    return participante.numeroJugador > 0 ? participante.numeroJugador : indice + 1;
}


static bool AutomaticoCapsulas(const Participante& participante)
{
    return participante.esBot || !participante.conectado;
}


static void AsignarColoresCapsulas(
    MinijuegoCapsulasBarajadas& m,
    const Participante participantes[]
)
{
    const Color respaldo[] =
    {
        Color{ 232, 62, 62, 255 },
        Color{ 62, 124, 238, 255 },
        Color{ 66, 202, 96, 255 },
        Color{ 246, 206, 52, 255 },
        Color{ 232, 92, 204, 255 },
        Color{ 250, 144, 44, 255 }
    };
    const int cantidadRespaldo = (int)(sizeof(respaldo) / sizeof(respaldo[0]));

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        m.coloresJugadores[i] = participantes[i].color;
    }

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!participantes[i].activo || !participantes[i].esBot)
        {
            continue;
        }

        for (int c = 0; c < cantidadRespaldo; c++)
        {
            bool repetido = false;

            for (int j = 0; j < MAX_PARTICIPANTES; j++)
            {
                if (j == i || !participantes[j].activo)
                {
                    continue;
                }

                const Color& otro = (!participantes[j].esBot || j < i)
                    ? m.coloresJugadores[j]
                    : participantes[j].color;

                // Colores parecidos cuentan como repetidos (un rojo humano
                // 230,60,60 y el rojo de respaldo no se distinguen).
                int diferencia =
                    std::abs((int)otro.r - (int)respaldo[c].r) +
                    std::abs((int)otro.g - (int)respaldo[c].g) +
                    std::abs((int)otro.b - (int)respaldo[c].b);

                if (diferencia < 90)
                {
                    repetido = true;
                }
            }

            if (!repetido)
            {
                m.coloresJugadores[i] = respaldo[c];
                break;
            }
        }
    }
}


static int IndiceVisualCapsulas(
    const MinijuegoCapsulasBarajadas& m,
    int indice,
    int& total
)
{
    int posicion = 0;
    total = 0;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!m.participa[i])
        {
            continue;
        }

        if (i == indice)
        {
            posicion = total;
        }

        total++;
    }

    return posicion;
}


//==================================================
// LOGICA (datos logicos, sin dependencia visual)
//==================================================

static int CapsulaEnSlotCapsulas(const MinijuegoCapsulasBarajadas& m, int slot)
{
    for (int c = 0; c < m.cantidadCapsulas; c++)
    {
        if (m.slotDe[c] == slot)
        {
            return c;
        }
    }

    return 0;
}


static void IniciarRondaCapsulas(MinijuegoCapsulasBarajadas& m)
{
    m.cantidadCapsulas = CAPSULAS_POR_RONDA[m.ronda];
    m.subfase = SUBFASE_CAPSULAS_MOSTRAR;
    m.tiempoSubfase = 0.0f;
    m.revelado = 0.0f;
    m.premio = GetRandomValue(0, m.cantidadCapsulas - 1);
    m.swapsRestantes = 5 + m.ronda * 2;
    m.swapA = -1;
    m.swapB = -1;
    m.swapTiempo = 0.0f;
    m.pausaBarajado = PAUSA_INICIAL_BARAJADO_CAPSULAS;
    m.ultimoParSlotA = -1;
    m.ultimoParSlotB = -1;
    m.efectosRevelado = false;
    m.tiempoEfectosRevelado = 0.0f;

    for (int c = 0; c < MAX_CAPSULAS; c++)
    {
        m.slotDe[c] = c;
    }

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        EstadoJugadorCapsulas& estado = m.estados[i];
        estado.marcador = m.cantidadCapsulas / 2;
        estado.confirmado = false;
        estado.tiempoConfirmacion = 0.0f;
        estado.slotElegido = -1;
        estado.acerto = false;
        estado.puntosRonda = 0;
    }
}


static void IniciarEleccionCapsulas(MinijuegoCapsulasBarajadas& m)
{
    m.subfase = SUBFASE_CAPSULAS_ELEGIR;
    m.tiempoSubfase = 0.0f;

    int slotCorrecto = m.slotDe[m.premio];
    int probabilidad = 60 - 8 * m.ronda;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        EstadoJugadorCapsulas& estado = m.estados[i];
        estado.botPaso = 0.0f;
        estado.botConfirmar = AleatorioCapsulas(0.6f, 2.5f);

        if (GetRandomValue(0, 99) < probabilidad)
        {
            estado.botObjetivo = slotCorrecto;
        }
        else
        {
            int incorrecto = GetRandomValue(0, m.cantidadCapsulas - 2);

            if (incorrecto >= slotCorrecto)
            {
                incorrecto++;
            }

            // Error humano plausible: suele elegir una capsula contigua a la correcta.
            if (GetRandomValue(0, 99) < 70)
            {
                int lado = GetRandomValue(0, 1) == 0 ? -1 : 1;
                int vecino = slotCorrecto + lado;

                if (vecino < 0 || vecino >= m.cantidadCapsulas)
                {
                    vecino = slotCorrecto - lado;
                }

                if (vecino >= 0 && vecino < m.cantidadCapsulas && vecino != slotCorrecto)
                {
                    incorrecto = vecino;
                }
            }

            estado.botObjetivo = incorrecto;
        }
    }
}


static void ConfirmarCapsulas(MinijuegoCapsulasBarajadas& m, int indice)
{
    EstadoJugadorCapsulas& estado = m.estados[indice];

    estado.confirmado = true;
    estado.slotElegido = estado.marcador;
    estado.tiempoConfirmacion = m.tiempoSubfase;
}


static void IniciarRevelacionCapsulas(MinijuegoCapsulasBarajadas& m)
{
    m.subfase = SUBFASE_CAPSULAS_REVELAR;
    m.tiempoSubfase = 0.0f;

    int slotCorrecto = m.slotDe[m.premio];
    bool algunAcierto = false;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!m.participa[i])
        {
            continue;
        }

        EstadoJugadorCapsulas& estado = m.estados[i];

        if (estado.confirmado && estado.slotElegido == slotCorrecto)
        {
            estado.acerto = true;
            estado.puntosRonda =
                estado.tiempoConfirmacion <= VENTANA_APUESTA_DOBLE_CAPSULAS ? 2 : 1;
            estado.puntos += estado.puntosRonda;
            estado.aciertos++;
            estado.tiempoAciertos += estado.tiempoConfirmacion;
            algunAcierto = true;
        }
    }

    ReproducirSonidoMinijuego(m.audio, algunAcierto ? SONIDO_ACIERTO : SONIDO_ERROR);
}


static void FinalizarCapsulas(MinijuegoCapsulasBarajadas& m, int limite)
{
    int mejores = 0;

    for (int i = 0; i < limite; i++)
    {
        if (!m.participa[i])
        {
            continue;
        }

        int posicion = 1;

        for (int j = 0; j < limite; j++)
        {
            if (j == i || !m.participa[j])
            {
                continue;
            }

            const EstadoJugadorCapsulas& a = m.estados[j];
            const EstadoJugadorCapsulas& b = m.estados[i];

            if (
                a.puntos > b.puntos ||
                (a.puntos == b.puntos && a.tiempoAciertos < b.tiempoAciertos - 0.001f)
            )
            {
                posicion++;
            }
        }

        m.resultado.participantes[i].posicionFinal = posicion;
        m.resultado.participantes[i].puntuacionMinijuego = m.estados[i].puntos;

        if (posicion == 1)
        {
            mejores++;
        }
    }

    m.resultado.estado = RESULTADO_MINIJUEGO_FINALIZADO;
    m.resultado.desenlace = mejores == 1
        ? DESENLACE_CON_GANADOR
        : DESENLACE_EMPATE;
    m.fase = FASE_CAPSULAS_TERMINADO;
    ReproducirSonidoMinijuego(m.audio, SONIDO_RESULTADO);
}


static void ActualizarBarajadoCapsulas(MinijuegoCapsulasBarajadas& m, float deltaTime)
{
    if (m.pausaBarajado > 0.0f)
    {
        m.pausaBarajado -= deltaTime;
        return;
    }

    if (m.swapA < 0)
    {
        if (m.swapsRestantes <= 0)
        {
            IniciarEleccionCapsulas(m);
            return;
        }

        int slotA = 0;
        int slotB = 0;

        for (int intento = 0; intento < 20; intento++)
        {
            slotA = GetRandomValue(0, m.cantidadCapsulas - 1);
            slotB = GetRandomValue(0, m.cantidadCapsulas - 2);

            if (slotB >= slotA)
            {
                slotB++;
            }

            bool repetido =
                (slotA == m.ultimoParSlotA && slotB == m.ultimoParSlotB) ||
                (slotA == m.ultimoParSlotB && slotB == m.ultimoParSlotA);

            if (!repetido)
            {
                break;
            }
        }

        m.ultimoParSlotA = slotA;
        m.ultimoParSlotB = slotB;
        m.swapA = CapsulaEnSlotCapsulas(m, slotA);
        m.swapB = CapsulaEnSlotCapsulas(m, slotB);
        m.swapTiempo = 0.0f;
        m.swapsRestantes--;
        ReproducirSonidoMinijuego(m.audio, SONIDO_PLATAFORMA);
        return;
    }

    m.swapTiempo += deltaTime;

    if (m.swapTiempo >= DuracionIntercambioCapsulas(m.ronda))
    {
        int auxiliar = m.slotDe[m.swapA];
        m.slotDe[m.swapA] = m.slotDe[m.swapB];
        m.slotDe[m.swapB] = auxiliar;
        m.swapA = -1;
        m.swapB = -1;
        m.swapTiempo = 0.0f;
    }
}


static void ActualizarEleccionJugadorCapsulas(
    MinijuegoCapsulasBarajadas& m,
    int indice,
    const Participante& participante,
    float deltaTime
)
{
    EstadoJugadorCapsulas& estado = m.estados[indice];
    bool automatico = participante.esBot || !participante.conectado;
    InputMinijuegoParticipante entrada{};

    if (!automatico)
    {
        entrada = LeerInputMinijuegoParticipante(participante);
    }

    bool izquierdaNueva = entrada.izquierda && !estado.izquierdaPrevia;
    bool derechaNueva = entrada.derecha && !estado.derechaPrevia;
    estado.izquierdaPrevia = entrada.izquierda;
    estado.derechaPrevia = entrada.derecha;

    if (estado.confirmado)
    {
        return;
    }

    if (automatico)
    {
        estado.botPaso -= deltaTime;

        if (estado.marcador != estado.botObjetivo && estado.botPaso <= 0.0f)
        {
            estado.marcador += estado.botObjetivo > estado.marcador ? 1 : -1;
            estado.botPaso = 0.12f;
        }

        if (
            m.tiempoSubfase >= estado.botConfirmar &&
            estado.marcador == estado.botObjetivo
        )
        {
            ConfirmarCapsulas(m, indice);
        }

        return;
    }

    int anterior = estado.marcador;

    if (izquierdaNueva && estado.marcador > 0)
    {
        estado.marcador--;
    }

    if (derechaNueva && estado.marcador < m.cantidadCapsulas - 1)
    {
        estado.marcador++;
    }

    if (estado.marcador != anterior)
    {
        ReproducirSonidoMinijuego(m.audio, SONIDO_BOTON);
    }

    if (entrada.golpear || entrada.saltar)
    {
        ConfirmarCapsulas(m, indice);
    }
}


//==================================================
// EFECTOS (solo visuales; la logica nunca los lee)
//==================================================

static const Color COLOR_NUCLEO_CAPSULAS = { 120, 255, 170, 255 };
static const Color COLOR_ORO_CAPSULAS = { 255, 226, 96, 255 };
static const Color COLOR_HUMO_CAPSULAS = { 128, 120, 130, 255 };
static const Color COLOR_ERROR_CAPSULAS = { 214, 78, 66, 255 };

// Azar propio para lo visual: no consume GetRandomValue, asi las
// particulas (que dependen de la calidad grafica) no cambian el sorteo del
// premio, del barajado ni de la IA.
static unsigned int semillaVisualCapsulas = 0x2F6B4A1Du;


static float AzarVisualCapsulas(float minimo, float maximo)
{
    semillaVisualCapsulas = semillaVisualCapsulas * 1664525u + 1013904223u;
    float u = (float)((semillaVisualCapsulas >> 8) & 0xFFFFu) / 65535.0f;
    return minimo + u * (maximo - minimo);
}


static void EmitirParticulasCapsulas(
    MinijuegoCapsulasBarajadas& m,
    Vector3 origen,
    int cantidadBase,
    Color color,
    float velocidad,
    float subida,
    float vida,
    float tamano,
    float gravedad,
    float dispersion
)
{
    int cantidad = (int)std::lround((float)cantidadBase * FactorCalidadGrafica(CalidadParticulas()));
    if (cantidad < 1) cantidad = 1;

    int libre = 0;

    for (int n = 0; n < cantidad; n++)
    {
        while (libre < MAX_PARTICULAS_CAPSULAS && m.particulas[libre].activa) libre++;
        if (libre >= MAX_PARTICULAS_CAPSULAS) return;

        ParticulaCapsulas& p = m.particulas[libre];
        float angulo = AzarVisualCapsulas(0.0f, 2.0f * PI);
        float radial = AzarVisualCapsulas(0.3f, 1.0f) * velocidad;

        p.activa = true;
        p.posicion = {
            origen.x + AzarVisualCapsulas(-dispersion, dispersion),
            origen.y + AzarVisualCapsulas(-dispersion, dispersion) * 0.5f,
            origen.z + AzarVisualCapsulas(-dispersion, dispersion)
        };
        p.velocidad = {
            std::cos(angulo) * radial,
            subida * AzarVisualCapsulas(0.6f, 1.2f),
            std::sin(angulo) * radial
        };
        p.vidaMaxima = vida * AzarVisualCapsulas(0.7f, 1.2f);
        p.vida = p.vidaMaxima;
        p.tamano = tamano * AzarVisualCapsulas(0.7f, 1.3f);
        p.gravedad = gravedad;
        p.color = color;
    }
}


// Altura de apoyo aproximada: mesa, pasarela o suelo.
static float SueloParticulaCapsulas(Vector3 p)
{
    if (std::fabs(p.x) < 5.7f && std::fabs(p.z) < 1.8f) return ALTURA_MESA_CAPSULAS + 0.02f;
    if (std::fabs(p.x) < 5.5f && p.z > Z_PASARELA_CAPSULAS - 1.3f && p.z < Z_PASARELA_CAPSULAS + 1.3f)
        return ALTURA_PASARELA_CAPSULAS + 0.02f;
    return 0.02f;
}


static void ActualizarParticulasCapsulas(MinijuegoCapsulasBarajadas& m, float deltaTime)
{
    float freno = std::exp(-1.8f * deltaTime);

    for (int k = 0; k < MAX_PARTICULAS_CAPSULAS; k++)
    {
        ParticulaCapsulas& p = m.particulas[k];
        if (!p.activa) continue;

        p.vida -= deltaTime;
        if (p.vida <= 0.0f)
        {
            p.activa = false;
            continue;
        }

        p.velocidad.y -= p.gravedad * deltaTime;
        p.velocidad.x *= freno;
        p.velocidad.z *= freno;
        p.posicion.x += p.velocidad.x * deltaTime;
        p.posicion.y += p.velocidad.y * deltaTime;
        p.posicion.z += p.velocidad.z * deltaTime;

        float suelo = SueloParticulaCapsulas(p.posicion);

        if (p.posicion.y < suelo && p.velocidad.y < 0.0f)
        {
            p.posicion.y = suelo;
            p.velocidad.y *= -0.3f;
        }
    }

    // Motas de polvo flotando en los focos (decoracion).
    if (CalidadDecoracion() == CALIDAD_BAJA) return;

    m.acumuladorAmbiente += deltaTime * 9.0f * FactorCalidadGrafica(CalidadDecoracion());

    while (m.acumuladorAmbiente >= 1.0f)
    {
        m.acumuladorAmbiente -= 1.0f;
        Vector3 origen = {
            AzarVisualCapsulas(-5.2f, 5.2f),
            AzarVisualCapsulas(1.6f, 5.6f),
            AzarVisualCapsulas(-1.0f, 1.2f)
        };
        EmitirParticulasCapsulas(m, origen, 1, Color{ 210, 236, 255, 255 }, 0.12f, 0.12f, 2.6f, 0.035f, 0.0f, 0.0f);
    }
}


// Polvo al apoyar un par barajado: siempre igual en las dos posiciones.
static void EmitirPolvoIntercambioCapsulas(MinijuegoCapsulasBarajadas& m, int slotA, int slotB)
{
    int slots[2] = { slotA, slotB };

    for (int k = 0; k < 2; k++)
    {
        Vector3 base = { XSlotCapsulas(slots[k], m.cantidadCapsulas), ALTURA_MESA_CAPSULAS + 0.05f, 0.0f };
        EmitirParticulasCapsulas(m, base, 6, Color{ 196, 208, 220, 255 }, 1.5f, 0.4f, 0.4f, 0.06f, 1.5f, 0.55f);
    }
}


// Al levantarse las capsulas: chispas del nucleo, humo en las vacias,
// confeti sobre quien acerto y chispazo rojo sobre quien fallo.
static void EmitirEfectosRevelacionCapsulas(
    MinijuegoCapsulasBarajadas& m,
    const JugadorPrueba jugadores[],
    int limite
)
{
    int slotCorrecto = m.slotDe[m.premio];

    for (int s = 0; s < m.cantidadCapsulas; s++)
    {
        Vector3 base = { XSlotCapsulas(s, m.cantidadCapsulas), ALTURA_MESA_CAPSULAS + 0.4f, 0.0f };

        if (s == slotCorrecto)
        {
            EmitirParticulasCapsulas(m, base, 34, COLOR_NUCLEO_CAPSULAS, 3.0f, 4.6f, 0.9f, 0.09f, 7.0f, 0.3f);
            EmitirParticulasCapsulas(m, base, 18, COLOR_ORO_CAPSULAS, 2.2f, 5.6f, 1.0f, 0.07f, 7.0f, 0.2f);
        }
        else
        {
            EmitirParticulasCapsulas(m, base, 8, COLOR_HUMO_CAPSULAS, 0.8f, 0.9f, 0.8f, 0.15f, -0.4f, 0.45f);
        }
    }

    for (int i = 0; i < limite; i++)
    {
        if (!m.participa[i]) continue;

        Vector3 cabeza = { jugadores[i].posicion.x, jugadores[i].posicion.y + 2.0f, jugadores[i].posicion.z };
        m.reaccionRevelado[i] = 1.0f;

        if (m.estados[i].acerto)
        {
            EmitirParticulasCapsulas(m, cabeza, 22, m.coloresJugadores[i], 2.4f, 4.2f, 1.2f, 0.1f, 7.5f, 0.3f);
            EmitirParticulasCapsulas(m, cabeza, 10, COLOR_ORO_CAPSULAS, 1.8f, 4.8f, 1.1f, 0.08f, 7.5f, 0.2f);
        }
        else
        {
            EmitirParticulasCapsulas(m, cabeza, 10, COLOR_ERROR_CAPSULAS, 1.4f, 1.4f, 0.55f, 0.08f, 4.0f, 0.25f);
            EmitirParticulasCapsulas(m, cabeza, 5, COLOR_HUMO_CAPSULAS, 0.5f, 0.8f, 0.8f, 0.14f, -0.3f, 0.25f);
        }
    }

    ActivarTemblorCamaraGeneral(0.07f, 0.24f);
}


//==================================================
// CICLO DE VIDA
//==================================================

void MinijuegoCapsulasBarajadas::Inicializar()
{
    resultado = {};
    resultado.formato = FORMATO_MINIJUEGO_INDIVIDUAL;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        estados[i] = {};
        participa[i] = false;
        coloresJugadores[i] = LIGHTGRAY;
        destelloConfirmar[i] = 0.0f;
        reaccionRevelado[i] = 0.0f;
    }

    for (int k = 0; k < MAX_PARTICULAS_CAPSULAS; k++)
    {
        particulas[k] = {};
    }

    acumuladorAmbiente = 0.0f;

    // Camara alta: la mesa y las tapas abiertas quedan bajo el tendedero
    // del HUD y los jugadores no tapan las capsulas.
    camara.position = { 0.0f, 9.0f, 13.0f };
    camara.target = { 0.0f, 1.6f, 0.9f };
    camara.up = { 0.0f, 1.0f, 0.0f };
    camara.fovy = 44.0f;
    camara.projection = CAMERA_PERSPECTIVE;

    fase = FASE_CAPSULAS_PREPARACION;
    ronda = 0;
    tiempoPreparacion = DURACION_PREPARACION_CAPSULAS;
    tiempoAnimacion = 0.0f;
    IniciarRondaCapsulas(*this);
}


void MinijuegoCapsulasBarajadas::Reiniciar(
    JugadorPrueba jugadores[],
    Participante participantes[],
    int cantidadMaxima
)
{
    Inicializar();
    // Inicializar se llama para todos los minijuegos al arrancar. Cargar
    // solo al activar este laboratorio; la cache sobrevive a nuevas rondas.
    CargarPaqueteModelosEscenarioRetro3D(ObtenerPaqueteCapsulasBarajadasRetro3D());
    InicializarResultadoMinijuego(
        resultado,
        participantes,
        FORMATO_MINIJUEGO_INDIVIDUAL
    );

    AsignarColoresCapsulas(*this, participantes);

    int indices[MAX_PARTICIPANTES]{};
    int cantidad = ObtenerIndicesParticipantesActivos(
        participantes,
        indices,
        MAX_PARTICIPANTES
    );

    if (cantidad < 2)
    {
        resultado.estado = RESULTADO_MINIJUEGO_CANCELADO;
        fase = FASE_CAPSULAS_TERMINADO;
        return;
    }

    int limite = LimiteCapsulas(cantidadMaxima);

    for (int k = 0; k < cantidad; k++)
    {
        int i = indices[k];

        if (i >= limite)
        {
            continue;
        }

        Vector3 spawn =
        {
            ((float)k - (float)(cantidad - 1) * 0.5f) * 2.4f,
            ALTURA_PASARELA_CAPSULAS + 0.72f,
            Z_PASARELA_CAPSULAS
        };

        ConfigurarJugadorMinijuegoEstandar(jugadores[i], spawn);
        jugadores[i].direccionMirada = { 0.0f, 0.0f, -1.0f };
        participa[i] = true;
    }
}


//==================================================
// ACTUALIZACION
//==================================================

void MinijuegoCapsulasBarajadas::Actualizar(
    float deltaTime,
    JugadorPrueba jugadores[],
    int cantidadMaxima,
    Participante participantes[]
)
{
    tiempoAnimacion += deltaTime;

    // Efectos: siguen corriendo tambien en el final (confeti que cae).
    ActualizarParticulasCapsulas(*this, deltaTime);

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        destelloConfirmar[i] = LimitarCapsulas(destelloConfirmar[i] - deltaTime * 2.2f, 0.0f, 1.0f);
        reaccionRevelado[i] = LimitarCapsulas(reaccionRevelado[i] - deltaTime * 1.6f, 0.0f, 1.0f);
    }

    if (
        fase == FASE_CAPSULAS_TERMINADO ||
        resultado.estado == RESULTADO_MINIJUEGO_CANCELADO
    )
    {
        return;
    }

    if (fase == FASE_CAPSULAS_PREPARACION)
    {
        float preparacionAntes = tiempoPreparacion;
        tiempoPreparacion -= deltaTime;
        ActualizarAudioCuentaRegresiva(audio, preparacionAntes, tiempoPreparacion);

        if (tiempoPreparacion <= 0.0f)
        {
            tiempoPreparacion = 0.0f;
            fase = FASE_CAPSULAS_JUGANDO;
        }

        return;
    }

    int limite = LimiteCapsulas(cantidadMaxima);
    float tiempoAntes = tiempoSubfase;
    tiempoSubfase += deltaTime;

    if (subfase == SUBFASE_CAPSULAS_MOSTRAR)
    {
        if (tiempoSubfase >= DURACION_MOSTRAR_CAPSULAS)
        {
            subfase = SUBFASE_CAPSULAS_BARAJAR;
            tiempoSubfase = 0.0f;
        }
    }
    else if (subfase == SUBFASE_CAPSULAS_BARAJAR)
    {
        int capsulaA = swapA;
        int capsulaB = swapB;

        ActualizarBarajadoCapsulas(*this, deltaTime);

        // Par apoyado: polvo identico en las dos posiciones.
        if (capsulaA >= 0 && swapA < 0)
        {
            EmitirPolvoIntercambioCapsulas(*this, slotDe[capsulaA], slotDe[capsulaB]);
        }
    }
    else if (subfase == SUBFASE_CAPSULAS_ELEGIR)
    {
        bool todosConfirmaron = true;

        for (int i = 0; i < limite; i++)
        {
            if (!participa[i])
            {
                continue;
            }

            bool yaConfirmado = estados[i].confirmado;
            ActualizarEleccionJugadorCapsulas(*this, i, participantes[i], deltaTime);

            // Confirmar: destello y chispas a los pies, iguales para
            // cualquier capsula (no revelan cual eligio).
            if (!yaConfirmado && estados[i].confirmado)
            {
                destelloConfirmar[i] = 1.0f;
                EmitirParticulasCapsulas(
                    *this,
                    { jugadores[i].posicion.x, ALTURA_PASARELA_CAPSULAS + 0.1f, jugadores[i].posicion.z },
                    12, coloresJugadores[i], 1.8f, 1.6f, 0.5f, 0.08f, 5.0f, 0.35f
                );

                if (!AutomaticoCapsulas(participantes[i]))
                {
                    ReproducirSonidoMinijuego(audio, SONIDO_UI_CONFIRMAR);
                }
            }

            if (!estados[i].confirmado)
            {
                todosConfirmaron = false;
            }
        }

        ActualizarAudioAlertaTiempo(
            audio,
            DURACION_ELEGIR_CAPSULAS - tiempoAntes,
            DURACION_ELEGIR_CAPSULAS - tiempoSubfase,
            3.0f
        );

        if (todosConfirmaron || tiempoSubfase >= DURACION_ELEGIR_CAPSULAS)
        {
            IniciarRevelacionCapsulas(*this);
        }
    }
    else
    {
        revelado = LimitarCapsulas(revelado + deltaTime * 2.0f, 0.0f, 1.0f);

        if (efectosRevelado)
        {
            tiempoEfectosRevelado += deltaTime;
        }
        else if (revelado >= 0.45f)
        {
            efectosRevelado = true;
            tiempoEfectosRevelado = 0.0f;
            EmitirEfectosRevelacionCapsulas(*this, jugadores, limite);
        }

        if (tiempoSubfase >= DURACION_REVELAR_CAPSULAS)
        {
            ronda++;

            if (ronda >= RONDAS_CAPSULAS)
            {
                FinalizarCapsulas(*this, limite);
            }
            else
            {
                IniciarRondaCapsulas(*this);
            }
        }
    }

    // Los jugadores no se mueven: solo celebran un acierto con saltitos.
    for (int i = 0; i < limite; i++)
    {
        if (!participa[i])
        {
            continue;
        }

        float rebote = 0.0f;

        if (subfase == SUBFASE_CAPSULAS_REVELAR && estados[i].acerto)
        {
            rebote = std::fabs(std::sin(tiempoSubfase * 9.0f)) * 0.35f;
        }

        jugadores[i].posicion.y = ALTURA_PASARELA_CAPSULAS + 0.72f + rebote;
    }
}


//==================================================
// VISUAL: LABORATORIO (solo decoracion, la logica no depende de esto)
//==================================================

// Piezas GLB del paquete original con fallback individual. Solo dibujo:
// los slots, tiempos, seleccion y puntuacion no dependen de estas mallas.

static Vector3 PosicionCapsulaCapsulas(const MinijuegoCapsulasBarajadas& m, int c)
{
    float x = XSlotCapsulas(m.slotDe[c], m.cantidadCapsulas);
    float z = 0.0f;

    if (m.swapA >= 0 && (c == m.swapA || c == m.swapB))
    {
        int otra = c == m.swapA ? m.swapB : m.swapA;
        float destino = XSlotCapsulas(m.slotDe[otra], m.cantidadCapsulas);
        float s = SuavizadoCapsulas(m.swapTiempo / DuracionIntercambioCapsulas(m.ronda));
        float arco = std::sin(PI * s) * 1.3f;

        x = x + (destino - x) * s;
        z = c == m.swapA ? arco : -arco;
    }

    return { x, ALTURA_MESA_CAPSULAS, z };
}


static Vector3 PosicionManoCapsulas(const MinijuegoCapsulasBarajadas& m, bool& llevaNucleo)
{
    const Vector3 reposo = { 5.6f, 4.6f, -0.8f };
    llevaNucleo = false;

    if (m.fase != FASE_CAPSULAS_JUGANDO || m.subfase != SUBFASE_CAPSULAS_MOSTRAR)
    {
        return reposo;
    }

    float u = m.tiempoSubfase / DURACION_MOSTRAR_CAPSULAS;
    float xp = XSlotCapsulas(m.premio, m.cantidadCapsulas);
    Vector3 sobre = { xp, 4.0f, 0.0f };
    Vector3 dentro = { xp, 2.9f, 0.0f };

    llevaNucleo = u < 0.6f;

    auto mezclar = [](Vector3 a, Vector3 b, float t)
    {
        return Vector3{ a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t };
    };

    if (u < 0.35f) return mezclar(reposo, sobre, SuavizadoCapsulas(u / 0.35f));
    if (u < 0.60f) return mezclar(sobre, dentro, SuavizadoCapsulas((u - 0.35f) / 0.25f));
    if (u < 0.75f) return dentro;
    return mezclar(dentro, reposo, SuavizadoCapsulas((u - 0.75f) / 0.25f));
}


static float TapaAbiertaCapsulas(const MinijuegoCapsulasBarajadas& m)
{
    if (m.fase != FASE_CAPSULAS_JUGANDO || m.subfase != SUBFASE_CAPSULAS_MOSTRAR)
    {
        return 0.0f;
    }

    float u = m.tiempoSubfase / DURACION_MOSTRAR_CAPSULAS;

    if (u < 0.8f) return 1.0f;
    return 1.0f - LimitarCapsulas((u - 0.8f) / 0.2f, 0.0f, 1.0f);
}


// Todas las capsulas se dibujan IGUALES (modelo, color, brillo, altura,
// animacion y sombra) mientras se muestran, se barajan y se eligen: nada
// sigue a la capsula logica salvo su posicion. Solo al revelar cambian.
static const Color COLOR_BANDA_CAPSULAS = { 70, 196, 232, 255 };
static const Color COLOR_VACIA_CAPSULAS = { 168, 64, 58, 255 };
static const Color COLOR_FOCO_CAPSULAS = { 196, 226, 255, 255 };


static Color EscalarColorCapsulas(Color c, float k)
{
    float r = (float)c.r * k;
    float g = (float)c.g * k;
    float b = (float)c.b * k;
    return Color{
        (unsigned char)LimitarCapsulas(r, 0.0f, 255.0f),
        (unsigned char)LimitarCapsulas(g, 0.0f, 255.0f),
        (unsigned char)LimitarCapsulas(b, 0.0f, 255.0f),
        c.a
    };
}


static Color ConAlfaCapsulas(Color c, float alfa)
{
    c.a = (unsigned char)(LimitarCapsulas(alfa, 0.0f, 1.0f) * 255.0f);
    return c;
}


// Camara de la escena: durante la cuenta se acerca desde un costado
// (respeta "reducir movimiento").
static Camera3D CamaraVisualCapsulas(const MinijuegoCapsulasBarajadas& m)
{
    Camera3D c = m.camara;

    if (m.fase != FASE_CAPSULAS_PREPARACION)
    {
        return c;
    }

    float u = 1.0f - m.tiempoPreparacion / DURACION_PREPARACION_CAPSULAS;
    float k = (1.0f - SuavizadoCapsulas(u)) * ObtenerFactorMovimientoCamaraMinijuegos();

    c.position.x += 2.4f * k;
    c.position.y += 1.2f * k;
    c.position.z += 2.6f * k;
    return c;
}


// Caja con caras sombreadas: tapa clara, cuerpo medio, borde oscuro.
static void CajaSombreadaCapsulas(Vector3 c, Vector3 t, Color color)
{
    (DrawCube)(c, t.x, t.y, t.z, EscalarColorCapsulas(color, 0.74f));
    (DrawCube)({ c.x, c.y + t.y * 0.5f, c.z }, t.x + 0.02f, 0.05f, t.z + 0.02f, color);
    (DrawCube)({ c.x - t.x * 0.5f, c.y, c.z }, 0.03f, t.y, t.z + 0.01f, EscalarColorCapsulas(color, 0.6f));
    DrawCubeWires(c, t.x, t.y, t.z, Fade(BLACK, 0.28f));
}


static float HashCapsulas(int a, int b)
{
    unsigned int h = (unsigned int)(a * 73856093) ^ (unsigned int)(b * 19349663);
    h = (h ^ (h >> 13)) * 1274126177u;
    return (float)(h & 0xFFFFu) / 65535.0f;
}


// Racks de servidores con luces y cables por el suelo (decoracion pura).
static void DibujarRacksCapsulas(const MinijuegoCapsulasBarajadas& m)
{
    if (CalidadDecoracion() == CALIDAD_BAJA) return;

    int paso = CalidadDecoracion() == CALIDAD_ALTA ? 1 : 2;
    const Color carcasa = { 58, 66, 80, 255 };
    const Color cable = { 30, 34, 42, 255 };

    for (int lado = -1; lado <= 1; lado += 2)
    {
        for (int r = 0; r < 2; r++)
        {
            float x = (float)lado * (7.6f + 0.15f * (float)r);
            float z = -2.6f + (float)r * 1.5f;
            CajaSombreadaCapsulas({ x, 1.55f, z }, { 1.2f, 3.1f, 1.1f }, carcasa);

            // Frente con ranuras y LEDs que parpadean (variacion por hash).
            for (int fila = 0; fila < 9; fila += paso)
            {
                float y = 0.45f + (float)fila * 0.3f;
                (DrawCube)({ x, y, z + 0.56f }, 0.95f, 0.08f, 0.02f, Color{ 28, 32, 40, 255 });

                for (int led = 0; led < 3; led++)
                {
                    float h = HashCapsulas(fila + r * 17 + (lado + 1) * 31, led);
                    bool encendido = std::fmod(m.tiempoAnimacion * (0.7f + h * 2.3f) + h * 5.0f, 1.0f) < 0.6f;
                    Color color = h < 0.45f ? Color{ 80, 255, 170, 255 } : (h < 0.8f ? Color{ 90, 190, 255, 255 } : Color{ 255, 170, 60, 255 });
                    (DrawCube)({ x - 0.36f + (float)led * 0.12f, y, z + 0.575f }, 0.06f, 0.05f, 0.02f, encendido ? color : EscalarColorCapsulas(color, 0.25f));
                }
            }
        }

        // Mazo de cables desde el rack hasta la pata de la mesa.
        for (int k = 0; k < 3; k++)
        {
            float z = -1.9f + (float)k * 0.22f;
            (DrawCylinderEx)({ (float)lado * 7.0f, 0.06f, z }, { (float)lado * 5.4f, 0.06f, z + 0.9f }, 0.06f, 0.06f, 6, cable);
        }
    }
}


static void DibujarSalaCapsulas(const MinijuegoCapsulasBarajadas& m)
{
    bool detalle = CalidadDecoracion() != CALIDAD_BAJA;

    // Suelo y marcas.
    if (!DibujarModeloCapsulasBarajadasRetro3D(MODELO_CAPSULAS_SALA, { 0.0f, 0.0f, 0.0f }))
    {
        (DrawPlane)({ 0.0f, 0.0f, 0.0f }, { 40.0f, 40.0f }, Color{ 150, 160, 172, 255 });

        for (int k = -6; k <= 6; k++)
            (DrawCube)({ (float)k * 2.0f, 0.005f, 0.0f }, 0.05f, 0.01f, 30.0f, Color{ 118, 128, 140, 255 });
        for (int k = -4; k <= 6; k++)
            (DrawCube)({ 0.0f, 0.005f, (float)k * 2.0f }, 26.0f, 0.01f, 0.05f, Color{ 118, 128, 140, 255 });
        (DrawCube)({ 0.0f, 4.0f, -8.4f }, 26.0f, 8.0f, 0.4f, Color{ 52, 70, 84, 255 });
    }

    // Pared del fondo con monitores.
    for (int k = 0; k < 4; k++)
    {
        float x = -7.5f + (float)k * 5.0f;
        // Primitive 2 (menta) contiene solo las seis barras fijas del GLB.
        // Se conserva marco/pantalla; las barras siguen el seno original.
        if (!DibujarModeloCapsulasBarajadasRetro3D(MODELO_CAPSULAS_MONITOR,
            { x, 4.6f, -8.1f }, 0.0f, { 0.0f, 1.0f, 0.0f },
            { 1.0f, 1.0f, 1.0f }, WHITE, 2))
        {
            (DrawCube)({ x, 4.6f, -8.1f }, 3.4f, 2.0f, 0.15f, Color{ 24, 28, 34, 255 });
            (DrawCube)({ x, 4.6f, -8.0f }, 3.1f, 1.7f, 0.05f, Color{ 10, 40, 36, 255 });
        }

        for (int b = 0; b < 6; b++)
        {
            float altura = 0.3f + 0.6f * (0.5f + 0.5f * std::sin(m.tiempoAnimacion * 2.0f + (float)(b + k * 3)));
            // Decoracion sobre la pared: no proyecta sombras en el suelo.
            (DrawCube)({ x - 1.2f + (float)b * 0.48f, 4.6f - 0.8f + altura * 0.5f, -7.96f }, 0.3f, altura, 0.04f, Color{ 80, 255, 170, 255 });
        }
    }

    // Mostrador con tubos de ensayo burbujeantes.
    if (!DibujarModeloCapsulasBarajadasRetro3D(MODELO_CAPSULAS_MOSTRADOR, { 0.0f, 0.0f, -6.4f }))
        CajaSombreadaCapsulas({ 0.0f, 0.5f, -6.4f }, { 22.0f, 1.0f, 1.4f }, Color{ 110, 122, 134, 255 });

    for (int k = 0; k < 8; k++)
    {
        float x = -8.4f + (float)k * 2.4f;
        Color liquido = ColorFromHSV((float)k * 47.0f + 100.0f, 0.7f, 0.95f);

        // Primitive 4 (BOMBILLAS) son burbujas fijas. Omitirlas y conservar
        // las dos que suben con tiempoAnimacion, sin cargas adicionales.
        if (!DibujarModeloCapsulasBarajadasRetro3D(MODELO_CAPSULAS_TUBO,
            { x, 1.0f, -6.4f }, 0.0f, { 0.0f, 1.0f, 0.0f },
            { 1.0f, 1.0f, 1.0f }, Fade(liquido, 0.85f), 4))
        {
            (DrawCylinder)({ x, 1.0f, -6.4f }, 0.3f, 0.3f, 1.5f, 10, Fade(WHITE, 0.3f));
            (DrawCylinder)({ x, 1.0f, -6.4f }, 0.24f, 0.24f, 0.9f, 10, Fade(liquido, 0.85f));
            DrawCylinderWires({ x, 1.0f, -6.4f }, 0.3f, 0.3f, 1.5f, 8, Color{ 220, 235, 245, 255 });
        }

        if (!detalle) continue;

        for (int b = 0; b < 2; b++)
        {
            float fase = std::fmod(m.tiempoAnimacion * 0.6f + (float)b * 0.5f + (float)k * 0.13f, 1.0f);
            DrawSphereEx({ x + (b == 0 ? 0.06f : -0.06f), 1.1f + fase * 0.8f, -6.4f }, 0.07f, 4, 4, Fade(WHITE, 0.8f));
        }
    }

    DibujarRacksCapsulas(m);

    // Balizas de alerta en las esquinas.
    bool alerta = m.fase == FASE_CAPSULAS_JUGANDO && m.subfase == SUBFASE_CAPSULAS_BARAJAR;
    bool elegir = m.fase == FASE_CAPSULAS_JUGANDO && m.subfase == SUBFASE_CAPSULAS_ELEGIR;
    Color luz = Color{ 70, 70, 80, 255 };

    if (alerta && std::fmod(m.tiempoAnimacion * 4.0f, 1.0f) < 0.5f)
    {
        luz = Color{ 255, 150, 30, 255 };
    }
    else if (elegir)
    {
        luz = Color{ 80, 255, 140, 255 };
    }

    for (int lado = -1; lado <= 1; lado += 2)
    {
        if (!DibujarModeloCapsulasBarajadasRetro3D(MODELO_CAPSULAS_BALIZA,
            { (float)lado * 10.5f, 5.6f, -7.8f }, 0.0f,
            { 0.0f, 1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, luz))
        {
            (DrawCylinder)({ (float)lado * 10.5f, 5.6f, -7.8f }, 0.3f, 0.3f, 0.5f, 8, Color{ 50, 54, 62, 255 });
            DrawSphereEx({ (float)lado * 10.5f, 6.3f, -7.8f }, 0.35f, 8, 8, luz);
        }
    }
}


static bool DibujarSegmentoBrazoCapsulas(Vector3 inicio, Vector3 fin)
{
    Vector3 direccion = Vector3Subtract(fin, inicio);
    float longitud = Vector3Length(direccion);
    if (longitud < 0.001f) return false;
    Vector3 unidad = Vector3Scale(direccion, 1.0f / longitud);
    Vector3 eje = Vector3CrossProduct({ 0.0f, 1.0f, 0.0f }, unidad);
    float coseno = LimitarCapsulas(unidad.y, -1.0f, 1.0f);
    float angulo = std::acos(coseno) * RAD2DEG;
    // Incluye los casos paralelo y antiparalelo a +Y.
    if (Vector3Length(eje) < 0.001f) eje = { 1.0f, 0.0f, 0.0f };
    else eje = Vector3Normalize(eje);
    return DibujarModeloCapsulasBarajadasRetro3D(MODELO_CAPSULAS_BRAZO_SEGMENTO,
        inicio, angulo, eje, { 1.0f, longitud, 1.0f });
}


// El nucleo solo se dibuja cuando el juego lo muestra a proposito: en la
// pinza (cuenta y principio de MOSTRAR), dentro de su capsula mientras la
// tapa sigue abierta y al revelar. Durante BARAJAR y ELEGIR no existe en
// pantalla (ni el modelo ni su luz), asi no puede verse ni por transparencia.
static bool NucleoVisibleCapsulas(const MinijuegoCapsulasBarajadas& m)
{
    if (m.resultado.estado == RESULTADO_MINIJUEGO_CANCELADO) return false;
    if (m.fase == FASE_CAPSULAS_PREPARACION) return true;
    return m.subfase == SUBFASE_CAPSULAS_MOSTRAR || m.subfase == SUBFASE_CAPSULAS_REVELAR;
}


static Vector3 PosicionNucleoCapsulas(const MinijuegoCapsulasBarajadas& m, bool& enPinza)
{
    bool llevaNucleo = false;
    Vector3 mano = PosicionManoCapsulas(m, llevaNucleo);
    enPinza = llevaNucleo || m.fase == FASE_CAPSULAS_PREPARACION;

    if (enPinza)
    {
        return { mano.x, mano.y - 0.5f, mano.z };
    }

    Vector3 p = PosicionCapsulaCapsulas(m, m.premio);
    float subida = 0.0f;

    if (m.subfase == SUBFASE_CAPSULAS_REVELAR)
    {
        float e = SuavizadoCapsulas(m.revelado);
        subida = e * (0.35f + 0.06f * std::sin(m.tiempoAnimacion * 4.0f));
    }

    return { p.x, p.y + 0.4f + subida, p.z };
}


static void DibujarMesaCapsulas(const MinijuegoCapsulasBarajadas& m)
{
    // Mesa de acero.
    if (!DibujarModeloCapsulasBarajadasRetro3D(MODELO_CAPSULAS_MESA, { 0.0f, 0.0f, 0.0f }))
    {
        CajaSombreadaCapsulas({ 0.0f, ALTURA_MESA_CAPSULAS - 0.06f, 0.0f }, { 11.4f, 0.12f, 3.6f }, Color{ 172, 182, 194, 255 });

        for (int sx = -1; sx <= 1; sx += 2)
        {
            for (int sz = -1; sz <= 1; sz += 2)
            {
                (DrawCylinder)({ (float)sx * 5.3f, 0.0f, (float)sz * 1.5f }, 0.14f, 0.14f, ALTURA_MESA_CAPSULAS - 0.12f, 8, Color{ 120, 128, 140, 255 });
            }
        }
    }

    // Pasarela del publico con borde de precaucion.
    if (!DibujarModeloCapsulasBarajadasRetro3D(MODELO_CAPSULAS_PASARELA,
        { 0.0f, 0.0f, Z_PASARELA_CAPSULAS }))
    {
        CajaSombreadaCapsulas({ 0.0f, ALTURA_PASARELA_CAPSULAS * 0.5f, Z_PASARELA_CAPSULAS }, { 11.0f, ALTURA_PASARELA_CAPSULAS, 2.6f }, Color{ 96, 104, 116, 255 });

        for (int k = 0; k < 22; k++)
        {
            Color franja = (k % 2 == 0) ? Color{ 230, 196, 60, 255 } : Color{ 30, 30, 34, 255 };
            (DrawCube)({ -5.25f + (float)k * 0.5f, ALTURA_PASARELA_CAPSULAS + 0.03f, Z_PASARELA_CAPSULAS - 1.2f }, 0.5f, 0.012f, 0.2f, franja);
        }
    }

    // Brazo robotico.
    bool llevaNucleo = false;
    Vector3 mano = PosicionManoCapsulas(m, llevaNucleo);
    Vector3 hombro = { 0.0f, 4.2f, -3.4f };
    Vector3 codo =
    {
        (hombro.x + mano.x) * 0.5f,
        (hombro.y + mano.y) * 0.5f + 1.3f,
        (hombro.z + mano.z) * 0.5f - 0.3f
    };

    if (!DibujarModeloCapsulasBarajadasRetro3D(MODELO_CAPSULAS_BRAZO_BASE, { 0.0f, 0.0f, -3.4f }))
    {
        DrawCylinder({ 0.0f, 0.0f, -3.4f }, 0.6f, 0.8f, 3.6f, 12, Color{ 80, 90, 104, 255 });
        DrawSphereEx(hombro, 0.45f, 10, 10, Color{ 230, 140, 52, 255 });
    }
    if (!DibujarSegmentoBrazoCapsulas(hombro, codo))
        DrawCylinderEx(hombro, codo, 0.24f, 0.2f, 8, Color{ 230, 140, 52, 255 });
    if (!DibujarModeloCapsulasBarajadasRetro3D(MODELO_CAPSULAS_BRAZO_ARTICULACION, codo))
        DrawSphereEx(codo, 0.3f, 8, 8, Color{ 60, 66, 78, 255 });
    if (!DibujarSegmentoBrazoCapsulas(codo, mano))
        DrawCylinderEx(codo, mano, 0.2f, 0.13f, 8, Color{ 230, 140, 52, 255 });
    if (!DibujarModeloCapsulasBarajadasRetro3D(MODELO_CAPSULAS_BRAZO_PINZA, mano))
    {
        (DrawCube)({ mano.x - 0.2f, mano.y - 0.15f, mano.z }, 0.08f, 0.4f, 0.2f, Color{ 60, 66, 78, 255 });
        (DrawCube)({ mano.x + 0.2f, mano.y - 0.15f, mano.z }, 0.08f, 0.4f, 0.2f, Color{ 60, 66, 78, 255 });
    }

    // Capsulas. Mismo color, mismo pulso y mismo LED para todas; el indice
    // logico c solo se usa para la posicion y, al revelar, para el color.
    float tapa = TapaAbiertaCapsulas(m);
    float elevacion = SuavizadoCapsulas(m.revelado);
    bool revelando = m.subfase == SUBFASE_CAPSULAS_REVELAR;
    float pulso = 0.86f + 0.14f * std::sin(m.tiempoAnimacion * 3.2f);
    Color bandaComun = EscalarColorCapsulas(COLOR_BANDA_CAPSULAS, pulso);
    bool ledEncendido = std::fmod(m.tiempoAnimacion * 1.4f, 1.0f) < 0.55f;

    // Temblor del revelado: el mismo en todas mientras se levantan.
    float temblor = 0.0f;

    if (revelando && m.fase == FASE_CAPSULAS_JUGANDO)
    {
        temblor = std::sin(m.tiempoAnimacion * 47.0f) * 0.05f * (1.0f - elevacion)
            * ObtenerFactorMovimientoCamaraMinijuegos();
    }

    for (int c = 0; c < m.cantidadCapsulas; c++)
    {
        Vector3 p = PosicionCapsulaCapsulas(m, c);
        float x = p.x + temblor;
        float y0 = p.y + elevacion * 1.7f;
        Color banda = bandaComun;
        bool led = ledEncendido;

        if (revelando)
        {
            banda = ColorLerp(bandaComun, c == m.premio ? COLOR_NUCLEO_CAPSULAS : COLOR_VACIA_CAPSULAS, elevacion);
            led = c == m.premio || ledEncendido;
        }

        if (!DibujarModeloCapsulasBarajadasRetro3D(MODELO_CAPSULAS_CUERPO, { x, y0, p.z }))
        {
            (DrawCylinder)({ x, y0, p.z }, 0.8f, 0.8f, 1.5f, 18, Color{ 190, 198, 208, 255 });
            DrawCylinderWires({ x, y0, p.z }, 0.8f, 0.8f, 1.5f, 12, Color{ 80, 90, 104, 255 });
        }
        if (!DibujarModeloCapsulasBarajadasRetro3D(MODELO_CAPSULAS_BANDA,
            { x, y0, p.z }, 0.0f, { 0.0f, 1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, banda))
            (DrawCylinder)({ x, y0 + 0.45f, p.z }, 0.83f, 0.83f, 0.28f, 18, banda);
        if (!DibujarModeloCapsulasBarajadasRetro3D(MODELO_CAPSULAS_TAPA,
            { x, y0 + 1.5f + tapa * 1.3f, p.z }, 0.0f,
            { 0.0f, 1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, banda))
        {
            (DrawCylinder)({ x, y0 + 1.5f + tapa * 1.3f, p.z }, 0.62f, 0.82f, 0.22f, 18, Color{ 150, 160, 174, 255 });
            DrawSphereEx({ x, y0 + 1.75f + tapa * 1.3f, p.z }, 0.14f, 6, 6, banda);
        }

        // LED de estado al frente (parpadeo comun).
        DrawSphereEx({ x, y0 + 1.05f, p.z + 0.84f }, 0.075f, 6, 6, led ? EscalarColorCapsulas(banda, 1.25f) : EscalarColorCapsulas(banda, 0.3f));

        // Una sombra sobre la mesa, tambien en fallback. No proyectar las
        // tres piezas por separado sobre el suelo debajo de la mesa.
        DibujarSombraRetroCircular({ x, y0, p.z }, 0.76f, 0.76f, 1.06f);
    }

    // Nucleo brillante: en la pinza, en su capsula o sobre la mesa al revelar.
    if (NucleoVisibleCapsulas(m))
    {
        bool enPinza = false;
        Vector3 nucleo = PosicionNucleoCapsulas(m, enPinza);
        float pulsoNucleo = 1.0f + 0.12f * std::sin(m.tiempoAnimacion * 8.0f);

        if (!DibujarModeloCapsulasBarajadasRetro3D(MODELO_CAPSULAS_NUCLEO,
            nucleo, m.tiempoAnimacion * 60.0f, { 0.0f, 1.0f, 0.0f }, { pulsoNucleo, pulsoNucleo, pulsoNucleo }))
        {
            DrawSphereEx(nucleo, 0.32f * pulsoNucleo, 10, 10, COLOR_NUCLEO_CAPSULAS);
            DrawSphereWires(nucleo, 0.46f * pulsoNucleo, 6, 6, Fade(Color{ 160, 255, 210, 255 }, 0.8f));
        }
    }

    // Destello al soltar el nucleo en su capsula (aviso intencional).
    if (m.subfase == SUBFASE_CAPSULAS_MOSTRAR && m.fase == FASE_CAPSULAS_JUGANDO)
    {
        float u = m.tiempoSubfase / DURACION_MOSTRAR_CAPSULAS;

        if (u >= 0.6f && u < 0.8f)
        {
            Vector3 p = PosicionCapsulaCapsulas(m, m.premio);
            DrawSphereEx({ p.x, p.y + 1.6f, p.z }, 0.6f * (1.0f - (u - 0.6f) / 0.2f), 8, 8, Fade(Color{ 160, 255, 210, 255 }, 0.6f));
        }
    }
}


static int ContarHumanosCapsulas(
    const MinijuegoCapsulasBarajadas& m,
    const Participante participantes[],
    int limite
)
{
    int humanos = 0;

    for (int i = 0; i < limite; i++)
    {
        if (m.participa[i] && !AutomaticoCapsulas(participantes[i])) humanos++;
    }

    return humanos;
}


// Marcadores: nadie ve donde apunta otro antes del revelado.
//  - Bots: su marcador recien aparece al revelar.
//  - Humanos: ven su cursor mientras eligen (lo necesitan para jugar). Si
//    hay varios humanos en la misma pantalla, al confirmar el marcador se
//    oculta para que no se pueda copiar una eleccion ya cerrada.
static bool MarcadorVisibleCapsulas(
    const MinijuegoCapsulasBarajadas& m,
    const Participante participantes[],
    int indice,
    int humanos
)
{
    if (m.fase != FASE_CAPSULAS_JUGANDO) return false;

    const EstadoJugadorCapsulas& estado = m.estados[indice];

    if (m.subfase == SUBFASE_CAPSULAS_REVELAR) return estado.slotElegido >= 0;
    if (m.subfase != SUBFASE_CAPSULAS_ELEGIR) return false;
    if (AutomaticoCapsulas(participantes[indice])) return false;

    return !estado.confirmado || humanos <= 1;
}


static Vector3 PosicionMarcadorCapsulas(const MinijuegoCapsulasBarajadas& m, int indice)
{
    const EstadoJugadorCapsulas& estado = m.estados[indice];
    int slot = m.subfase == SUBFASE_CAPSULAS_REVELAR ? estado.slotElegido : estado.marcador;
    int total = 0;
    int k = IndiceVisualCapsulas(m, indice, total);
    float x = XSlotCapsulas(slot, m.cantidadCapsulas) + ((float)k - (float)(total - 1) * 0.5f) * 0.42f;
    float y = 3.5f + 0.08f * std::sin(m.tiempoAnimacion * 4.0f + (float)k);
    return { x, y, Z_MARCADOR_CAPSULAS };
}


static void DibujarMarcadoresCapsulas(
    const MinijuegoCapsulasBarajadas& m,
    const JugadorPrueba jugadores[],
    const Participante participantes[],
    int limite
)
{
    int humanos = ContarHumanosCapsulas(m, participantes, limite);

    for (int i = 0; i < limite; i++)
    {
        if (!m.participa[i] || !MarcadorVisibleCapsulas(m, participantes, i, humanos))
        {
            continue;
        }

        const EstadoJugadorCapsulas& estado = m.estados[i];
        Vector3 p = PosicionMarcadorCapsulas(m, i);
        Color color = m.coloresJugadores[i];

        if (m.subfase == SUBFASE_CAPSULAS_REVELAR)
        {
            color = estado.acerto ? COLOR_NUCLEO_CAPSULAS : COLOR_ERROR_CAPSULAS;
        }

        if (!DibujarModeloCapsulasBarajadasRetro3D(MODELO_CAPSULAS_MARCADOR,
            p, 0.0f, { 0.0f, 1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, color))
        {
            (DrawCylinder)(p, 0.24f, 0.0f, 0.55f, 10, color);
            DrawSphereEx({ p.x, p.y + 0.72f, p.z }, 0.2f, 8, 8, m.coloresJugadores[i]);
        }

        if (estado.confirmado)
        {
            DrawSphereWires({ p.x, p.y + 0.72f, p.z }, 0.32f, 6, 6, Fade(WHITE, 0.8f));
        }

        DrawLine3D(
            { jugadores[i].posicion.x, jugadores[i].posicion.y + 0.8f, jugadores[i].posicion.z },
            { p.x, p.y + 0.72f, p.z },
            Fade(m.coloresJugadores[i], estado.confirmado ? 0.9f : 0.45f)
        );
    }
}


static void DibujarParticulasCapsulas(const MinijuegoCapsulasBarajadas& m)
{
    for (int k = 0; k < MAX_PARTICULAS_CAPSULAS; k++)
    {
        const ParticulaCapsulas& p = m.particulas[k];
        if (!p.activa) continue;

        float vida = p.vida / p.vidaMaxima;
        float alfa = vida < 0.4f ? vida / 0.4f : 1.0f;

        // Las motas de ambiente (sin gravedad, chicas) entran y salen suaves.
        if (p.gravedad == 0.0f)
        {
            alfa = 0.45f * std::sin(PI * vida);
        }

        float s = p.tamano * (0.6f + 0.4f * vida);
        (DrawCube)(p.posicion, s, s, s, ConAlfaCapsulas(p.color, alfa));
    }
}


//==================================================
// LUZ FALSA (mezcla aditiva, sin escribir profundidad)
//==================================================

// Disco de luz: centro opaco, borde transparente.
static void DiscoLuzCapsulas(Vector3 centro, Vector3 u, Vector3 v, float radio, Color color)
{
    const int LADOS = 18;
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


static void LuzSueloCapsulas(float x, float y, float z, float radio, Color color)
{
    DiscoLuzCapsulas({ x, y, z }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, radio, color);
}


// Disco mirando a la camara fija (direccion de vista ~(0, -0.52, -0.85)).
static void HaloCapsulas(Vector3 centro, float radio, Color color)
{
    DiscoLuzCapsulas(centro, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.853f, -0.522f }, radio, color);
}


// Haz conico entre dos discos horizontales (alfa por extremo).
static void HazLuzCapsulas(
    Vector3 a, float radioA, unsigned char alfaA,
    Vector3 b, float radioB, unsigned char alfaB,
    Color color
)
{
    const int LADOS = 16;
    rlBegin(RL_TRIANGLES);

    for (int k = 0; k < LADOS; k++)
    {
        float a0 = 2.0f * PI * (float)k / (float)LADOS;
        float a1 = 2.0f * PI * (float)(k + 1) / (float)LADOS;
        Vector3 a0v = { a.x + std::cos(a0) * radioA, a.y, a.z + std::sin(a0) * radioA };
        Vector3 a1v = { a.x + std::cos(a1) * radioA, a.y, a.z + std::sin(a1) * radioA };
        Vector3 b0v = { b.x + std::cos(a0) * radioB, b.y, b.z + std::sin(a0) * radioB };
        Vector3 b1v = { b.x + std::cos(a1) * radioB, b.y, b.z + std::sin(a1) * radioB };

        rlColor4ub(color.r, color.g, color.b, alfaA); rlVertex3f(a0v.x, a0v.y, a0v.z);
        rlColor4ub(color.r, color.g, color.b, alfaB); rlVertex3f(b0v.x, b0v.y, b0v.z);
        rlColor4ub(color.r, color.g, color.b, alfaB); rlVertex3f(b1v.x, b1v.y, b1v.z);

        rlColor4ub(color.r, color.g, color.b, alfaA); rlVertex3f(a0v.x, a0v.y, a0v.z);
        rlColor4ub(color.r, color.g, color.b, alfaB); rlVertex3f(b1v.x, b1v.y, b1v.z);
        rlColor4ub(color.r, color.g, color.b, alfaA); rlVertex3f(a1v.x, a1v.y, a1v.z);
    }

    rlEnd();
}


static unsigned char AlfaCapsulas(float valor)
{
    return (unsigned char)LimitarCapsulas(valor, 0.0f, 255.0f);
}


static void DibujarLucesCapsulas(
    const MinijuegoCapsulasBarajadas& m,
    const JugadorPrueba jugadores[],
    const Participante participantes[],
    int limite
)
{
    if (FactorCalidadGrafica(CalidadEfectos()) < 0.5f) return;

    float t = m.tiempoAnimacion;
    bool revelando = m.subfase == SUBFASE_CAPSULAS_REVELAR;
    float elevacion = SuavizadoCapsulas(m.revelado);
    int slotCorrecto = m.slotDe[m.premio];

    rlDrawRenderBatchActive();
    rlDisableDepthMask();
    rlDisableBackfaceCulling();
    BeginBlendMode(BLEND_ADDITIVE);

    // Focos del techo: uno por POSICION de la mesa (no por capsula), todos
    // iguales hasta el revelado. Ahi el de la capsula premiada se vuelve
    // verde y fuerte y los demas se apagan en rojo.
    for (int s = 0; s < m.cantidadCapsulas; s++)
    {
        float x = XSlotCapsulas(s, m.cantidadCapsulas);
        Color foco = COLOR_FOCO_CAPSULAS;
        float intensidad = 1.0f;

        if (revelando)
        {
            bool premiado = s == slotCorrecto;
            foco = ColorLerp(foco, premiado ? COLOR_NUCLEO_CAPSULAS : COLOR_VACIA_CAPSULAS, elevacion);
            intensidad = premiado ? 1.0f + 1.1f * elevacion : 1.0f - 0.45f * elevacion;
        }

        HazLuzCapsulas(
            { x, 8.4f, -0.4f }, 0.3f, AlfaCapsulas(30.0f * intensidad),
            { x, ALTURA_MESA_CAPSULAS + 0.02f, 0.0f }, 1.2f, AlfaCapsulas(9.0f * intensidad),
            foco
        );
        LuzSueloCapsulas(x, ALTURA_MESA_CAPSULAS + 0.025f, 0.0f, 1.55f, ConAlfaCapsulas(foco, 0.32f * intensidad));
    }

    // Columna de luz sobre el nucleo revelado.
    if (revelando && elevacion > 0.01f)
    {
        float x = XSlotCapsulas(slotCorrecto, m.cantidadCapsulas);
        float latido = 0.85f + 0.15f * std::sin(t * 9.0f);
        HazLuzCapsulas(
            { x, ALTURA_MESA_CAPSULAS + 0.05f, 0.0f }, 0.75f, AlfaCapsulas(70.0f * elevacion * latido),
            { x, 6.2f, 0.0f }, 0.35f, 0,
            COLOR_NUCLEO_CAPSULAS
        );
    }

    // Nucleo: halo y charco de luz solo mientras se ve a proposito.
    if (NucleoVisibleCapsulas(m))
    {
        bool enPinza = false;
        Vector3 n = PosicionNucleoCapsulas(m, enPinza);
        float pulso = 1.0f + 0.12f * std::sin(t * 8.0f);
        float alfa = 1.0f;

        if (!enPinza && m.subfase == SUBFASE_CAPSULAS_MOSTRAR)
        {
            float u = m.tiempoSubfase / DURACION_MOSTRAR_CAPSULAS;
            alfa = 1.0f - LimitarCapsulas((u - 0.6f) / 0.2f, 0.0f, 1.0f);
        }
        else if (revelando)
        {
            alfa = elevacion;
        }

        if (alfa > 0.01f)
        {
            HaloCapsulas(n, 1.15f * pulso, ConAlfaCapsulas(COLOR_NUCLEO_CAPSULAS, 0.5f * alfa));
            HaloCapsulas(n, 0.55f * pulso, Color{ 230, 255, 240, AlfaCapsulas(110.0f * alfa) });

            if (!enPinza)
            {
                LuzSueloCapsulas(n.x, ALTURA_MESA_CAPSULAS + 0.03f, n.z, 1.7f, ConAlfaCapsulas(COLOR_NUCLEO_CAPSULAS, 0.4f * alfa));
            }
        }
    }

    // Pantallas del fondo y balizas.
    for (int k = 0; k < 4; k++)
    {
        HaloCapsulas({ -7.5f + (float)k * 5.0f, 4.6f, -7.7f }, 2.4f, Color{ 60, 255, 170, 26 });
    }

    bool alerta = m.fase == FASE_CAPSULAS_JUGANDO && m.subfase == SUBFASE_CAPSULAS_BARAJAR
        && std::fmod(t * 4.0f, 1.0f) < 0.5f;
    bool elegir = m.fase == FASE_CAPSULAS_JUGANDO && m.subfase == SUBFASE_CAPSULAS_ELEGIR;

    if (alerta || elegir)
    {
        Color baliza = alerta ? Color{ 255, 150, 30, 90 } : Color{ 80, 255, 140, 70 };

        for (int lado = -1; lado <= 1; lado += 2)
        {
            HaloCapsulas({ (float)lado * 10.5f, 6.3f, -7.6f }, 1.6f, baliza);
        }
    }

    // Luz bajo cada jugador (su color = pertenencia). Al confirmar, destello;
    // al revelar, verde-oro si acerto o rojo si fallo.
    int humanos = ContarHumanosCapsulas(m, participantes, limite);

    for (int i = 0; i < limite; i++)
    {
        if (!m.participa[i]) continue;

        Color color = m.coloresJugadores[i];
        float fuerza = 0.28f + 0.5f * m.destelloConfirmar[i];

        if (revelando && m.efectosRevelado)
        {
            color = ColorLerp(color, m.estados[i].acerto ? COLOR_ORO_CAPSULAS : COLOR_ERROR_CAPSULAS, 0.6f + 0.4f * m.reaccionRevelado[i]);
            fuerza = 0.3f + 0.5f * m.reaccionRevelado[i];
        }

        LuzSueloCapsulas(
            jugadores[i].posicion.x, ALTURA_PASARELA_CAPSULAS + 0.04f, jugadores[i].posicion.z,
            1.25f + 0.35f * m.destelloConfirmar[i],
            ConAlfaCapsulas(color, fuerza)
        );

        if (MarcadorVisibleCapsulas(m, participantes, i, humanos))
        {
            Vector3 p = PosicionMarcadorCapsulas(m, i);
            HaloCapsulas({ p.x, p.y + 0.72f, p.z + 0.1f }, 0.6f, ConAlfaCapsulas(m.coloresJugadores[i], 0.35f));
        }
    }

    EndBlendMode();
    rlDrawRenderBatchActive();
    rlEnableBackfaceCulling();
    rlEnableDepthMask();
}


//==================================================
// DIBUJO
//==================================================

void MinijuegoCapsulasBarajadas::Dibujar(
    const JugadorPrueba jugadores[],
    int cantidadMaxima,
    const Participante participantes[],
    bool mostrarDebug
) const
{
    int limite = LimiteCapsulas(cantidadMaxima);
    Camera3D vista = CamaraVisualCapsulas(*this);
    bool jugando = fase == FASE_CAPSULAS_JUGANDO;
    bool revelando = jugando && subfase == SUBFASE_CAPSULAS_REVELAR;

    SeleccionarTemaVisualMinijuego(TEMA_VISUAL_NINGUNO);
    ClearBackground(Color{ 16, 22, 30, 255 });
    BeginMode3D(vista);

    DibujarSalaCapsulas(*this);
    DibujarMesaCapsulas(*this);

    for (int i = 0; i < limite; i++)
    {
        if (!participa[i])
        {
            continue;
        }

        Participante visual = participantes[i];
        visual.conectado = true;
        visual.color = coloresJugadores[i];

        // Quien fallo se sacude un instante al revelar.
        JugadorPrueba cuerpo = jugadores[i];

        if (revelando && !estados[i].acerto)
        {
            cuerpo.posicion.x += std::sin(tiempoAnimacion * 55.0f) * 0.09f
                * reaccionRevelado[i] * ObtenerFactorMovimientoCamaraMinijuegos();
        }

        // Anillo del color del jugador (pertenencia) sobre la pasarela.
        DrawCircle3D(
            { cuerpo.posicion.x, ALTURA_PASARELA_CAPSULAS + 0.035f, cuerpo.posicion.z },
            0.95f,
            { 1.0f, 0.0f, 0.0f },
            90.0f,
            coloresJugadores[i]
        );
        DrawCircle3D(
            { cuerpo.posicion.x, ALTURA_PASARELA_CAPSULAS + 0.035f, cuerpo.posicion.z },
            0.88f,
            { 1.0f, 0.0f, 0.0f },
            90.0f,
            Fade(coloresJugadores[i], 0.6f)
        );
        DibujarJugadorCuboPrueba(cuerpo, visual);

        if (mostrarDebug)
        {
            DrawBoundingBox(CrearHitboxJugadorPrueba(jugadores[i]), LIME);
        }
    }

    DibujarMarcadoresCapsulas(*this, jugadores, participantes, limite);

    if (mostrarDebug)
    {
        for (int c = 0; c < cantidadCapsulas; c++)
        {
            Vector3 p = PosicionCapsulaCapsulas(*this, c);
            DrawCubeWires({ p.x, p.y + 0.75f, p.z }, 1.6f, 1.5f, 1.6f, YELLOW);
        }
    }

    DibujarParticulasCapsulas(*this);
    DibujarLucesCapsulas(*this, jugadores, participantes, limite);

    EndMode3D();

    // Rotulos sobre los jugadores y puntos del revelado.
    for (int i = 0; i < limite; i++)
    {
        if (!participa[i])
        {
            continue;
        }

        Vector2 cabeza = GetWorldToScreen(
            { jugadores[i].posicion.x, jugadores[i].posicion.y + 2.25f, jugadores[i].posicion.z },
            vista
        );
        char nombre[8];
        std::snprintf(nombre, sizeof(nombre), "J%d", NumeroJugadorCapsulas(participantes[i], i));

        OpcionesEtiquetaUI opciones;
        opciones.borde = coloresJugadores[i];
        opciones.pico = true;
        DibujarEtiquetaUI(nombre, cabeza, -4.0f, opciones);

        if (revelando && efectosRevelado)
        {
            float u = tiempoEfectosRevelado;
            float alfa = LimitarCapsulas(u / 0.15f, 0.0f, 1.0f)
                * LimitarCapsulas((DURACION_REVELAR_CAPSULAS - 0.45f - u) / 0.4f, 0.0f, 1.0f);
            Vector2 aviso = GetWorldToScreen(
                { jugadores[i].posicion.x, jugadores[i].posicion.y + 2.25f, jugadores[i].posicion.z },
                vista
            );
            char texto[20];

            if (estados[i].acerto)
            {
                std::snprintf(texto, sizeof(texto), "%s", estados[i].puntosRonda == 2 ? "+2 ¡DOBLE!" : "+1");
            }
            else
            {
                std::snprintf(texto, sizeof(texto), "%s", estados[i].confirmado ? "FALLÓ" : "SIN ELEGIR");
            }

            DibujarTextoMundoContornoUI(
                texto,
                aviso.x,
                aviso.y - Px(64.0f) - Px(26.0f) * SuavizadoCapsulas(u / 0.8f),
                estados[i].acerto ? UI_TEXTO_GRANDE : UI_TEXTO,
                estados[i].acerto ? (estados[i].puntosRonda == 2 ? UI_FOCO : Color{ 150, 240, 160, 255 }) : UI_PELIGRO,
                alfa
            );
        }
    }

    // Anuncio de la etapa de la ronda, bajo el tendedero.
    if (jugando && subfase != SUBFASE_CAPSULAS_REVELAR)
    {
        const char* anuncio = "OBSERVÁ DÓNDE VA EL NÚCLEO";
        Color color = UI_TEXTO_MUNDO;

        if (subfase == SUBFASE_CAPSULAS_BARAJAR)
        {
            anuncio = "¡SEGUI LA CÁPSULA!";
            color = Color{ 255, 176, 80, 255 };
        }
        else if (subfase == SUBFASE_CAPSULAS_ELEGIR)
        {
            anuncio = "¡ELEGI!";
            color = Color{ 140, 240, 170, 255 };
        }

        float alfa = LimitarCapsulas(tiempoSubfase / 0.25f, 0.0f, 1.0f);
        float yAnuncio = (float)GetScreenHeight() * 0.205f;
        DibujarTextoMundoContornoUI(anuncio, (float)GetScreenWidth() * 0.5f, yAnuncio, UI_TEXTO_GRANDE, color, alfa);

        if (subfase == SUBFASE_CAPSULAS_ELEGIR && tiempoSubfase <= VENTANA_APUESTA_DOBLE_CAPSULAS)
        {
            float latido = 0.75f + 0.25f * std::sin(tiempoAnimacion * 14.0f);
            DibujarTextoMundoContornoUI(
                "APUESTA DOBLE: ¡CONFIRMÁ YA!",
                (float)GetScreenWidth() * 0.5f,
                yAnuncio + Px(38.0f),
                UI_TEXTO,
                UI_FOCO,
                latido
            );
        }
    }

    // HUD comun (Minigames/HudMinijuego): solo lee el estado.
    DatosHudMinijuego hud;
    PrepararDatosHudMinijuego(
        hud,
        "CÁPSULAS BARAJADAS",
        "SEGUI LA CÁPSULA DEL NÚCLEO Y ELEGILA. SI CONFIRMÁS EN EL PRIMER SEGUNDO, VALE DOBLE.",
        resultado
    );

    hud.fase = fase == FASE_CAPSULAS_PREPARACION
        ? HUD_MINIJUEGO_CUENTA
        : (fase == FASE_CAPSULAS_JUGANDO ? HUD_MINIJUEGO_JUGANDO : HUD_MINIJUEGO_FIN);
    hud.cuentaRestante = tiempoPreparacion;
    hud.finPorTiempo = false;

    int rondaVisible = ronda < RONDAS_CAPSULAS ? ronda + 1 : RONDAS_CAPSULAS;

    if (jugando && subfase == SUBFASE_CAPSULAS_ELEGIR)
    {
        // Mientras se elige, el cartel central es la cuenta de 4 s.
        float restante = DURACION_ELEGIR_CAPSULAS - tiempoSubfase;
        hud.reloj = HUD_RELOJ_CUENTA_ATRAS;
        hud.tiempoRestante = restante > 0.0f ? restante : 0.0f;
        hud.tiempoTotal = DURACION_ELEGIR_CAPSULAS;
        hud.segundosAlerta = 1.5f;
    }
    else
    {
        hud.reloj = HUD_RELOJ_MEDIDOR;
        hud.etiquetaMedidor = "RONDA";
        std::snprintf(hud.valorMedidor, sizeof(hud.valorMedidor), "%d/%d", rondaVisible, RONDAS_CAPSULAS);
        hud.progresoMedidor = (float)rondaVisible / (float)RONDAS_CAPSULAS;
    }

    bool hayEmpateEnPuntos = false;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        JugadorHudMinijuego& jugador = hud.jugadores[i];
        if (!jugador.visible) continue;

        const EstadoJugadorCapsulas& estado = estados[i];

        std::snprintf(jugador.valor, sizeof(jugador.valor), estado.puntos == 1 ? "%d PT" : "%d PTS", estado.puntos);
        jugador.valorNumerico = estado.puntos;
        std::snprintf(jugador.detalleFinal, sizeof(jugador.detalleFinal), estado.puntos == 1 ? "%d PUNTO" : "%d PUNTOS", estado.puntos);

        // Estado: "LISTO" es neutro (no dice que eligio ni si acerto).
        if (jugando && subfase == SUBFASE_CAPSULAS_ELEGIR)
        {
            if (estado.confirmado) std::snprintf(jugador.estado, sizeof(jugador.estado), "LISTO");
        }
        else if (jugando && subfase == SUBFASE_CAPSULAS_REVELAR)
        {
            if (estado.acerto)
                std::snprintf(jugador.estado, sizeof(jugador.estado), "%s", estado.puntosRonda == 2 ? "¡ACIERTO DOBLE!" : "¡ACIERTO!");
            else
                std::snprintf(jugador.estado, sizeof(jugador.estado), "%s", estado.confirmado ? "FALLÓ" : "SIN ELEGIR");
        }

        // Empate en puntos: desempata el menor tiempo total de aciertos.
        for (int j = 0; j < MAX_PARTICIPANTES; j++)
        {
            if (j == i || !hud.jugadores[j].visible) continue;

            if (estados[j].puntos == estado.puntos && estado.aciertos > 0)
            {
                int centesimas = (int)std::lround(estado.tiempoAciertos * 100.0f);
                std::snprintf(
                    jugador.detalleDesempate,
                    sizeof(jugador.detalleDesempate),
                    "%d,%02d S",
                    centesimas / 100,
                    centesimas % 100
                );
                hayEmpateEnPuntos = true;
                break;
            }
        }

        if (!participantes[i].esBot)
        {
            std::snprintf(
                jugador.controles,
                sizeof(jugador.controles),
                "%s: ELEGIR CÁPSULA\n%s: CONFIRMAR",
                TextoMoverHud(participantes[i]),
                ObtenerTextoBotonPrincipal(participantes[i])
            );
        }
    }

    if (hayEmpateEnPuntos)
    {
        hud.notaFinal = "EMPATE EN PUNTOS: QUEDA ARRIBA QUIEN TARDÓ MENOS EN CONFIRMAR SUS ACIERTOS";
    }

    // Banderines con los mismos colores que se ven en la escena.
    Participante colores[MAX_PARTICIPANTES];

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        colores[i] = participantes[i];

        if (participa[i])
        {
            colores[i].color = coloresJugadores[i];
        }
    }

    DibujarHudMinijuego(hud, colores);
}



const ResultadoMinijuego& MinijuegoCapsulasBarajadas::ObtenerResultado() const
{
    return resultado;
}
