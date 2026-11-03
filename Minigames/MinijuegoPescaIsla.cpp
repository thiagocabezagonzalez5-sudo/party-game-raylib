#include "Minigames/MinijuegoPescaIsla.h"

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
#include "raymath.h"
#include "rlgl.h"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>


//==================================================
// CONSTANTES DE GAMEPLAY
//==================================================

static const float DURACION_PREPARACION_PESCA = 3.0f;
static const float DURACION_PARTIDA_PESCA = 50.0f;

// Laguna y muelles.
static const float RADIO_LAGUNA_PESCA = 8.5f;
static const float RADIO_NADO_PESCA = 7.2f;
static const float DISTANCIA_MUELLE_PESCA = 10.5f;
static const float ALTURA_MUELLE_PESCA = 0.35f;
static const float PROFUNDIDAD_PESCA = -1.8f;

// Pesca.
static const float VELOCIDAD_CURSOR_PESCA = 5.5f;
static const float DURACION_LANZAMIENTO_PESCA = 0.45f;
static const float VENTANA_PIQUE_PESCA = 0.45f;
static const float ESPERA_MAXIMA_PESCA = 12.0f;
static const float TIEMPO_MAXIMO_PELEA_PESCA = 8.0f;
static const float RADIO_ATRAER_PESCA = 3.2f;
static const float RADIO_PIQUE_PESCA = 1.4f;
static const float BLOQUEO_BOTA_PESCA = 2.0f;
static const float BLOQUEO_PERDIDA_PESCA = 2.0f;
static const float BLOQUEO_FALLO_PESCA = 1.0f;

static const int VALOR_PEZ_PESCA[4] = { 1, 2, 5, 0 };
static const float DURACION_PELEA_PESCA[4] = { 1.5f, 2.2f, 3.0f, 0.0f };
static const float TENSION_PESCA[4] = { 0.55f, 0.65f, 0.80f, 0.0f };
static const float VELOCIDAD_PEZ_PESCA[4] = { 1.8f, 1.4f, 1.0f, 0.6f };
static const TipoPezPesca TIPOS_INICIALES_PESCA[MAX_PECES_PESCA] =
{
    PEZ_PESCA_PEQUENO, PEZ_PESCA_PEQUENO, PEZ_PESCA_PEQUENO,
    PEZ_PESCA_PEQUENO, PEZ_PESCA_PEQUENO, PEZ_PESCA_PEQUENO,
    PEZ_PESCA_MEDIANO, PEZ_PESCA_MEDIANO, PEZ_PESCA_MEDIANO, PEZ_PESCA_MEDIANO,
    PEZ_PESCA_RARO, PEZ_PESCA_RARO,
    PEZ_PESCA_BOTA, PEZ_PESCA_BOTA
};


//==================================================
// UTILIDADES
//==================================================

static float LimitarPesca(float valor, float minimo, float maximo)
{
    if (valor < minimo) return minimo;
    if (valor > maximo) return maximo;
    return valor;
}


static float AleatorioPesca(float minimo, float maximo)
{
    return minimo + (float)GetRandomValue(0, 1000) / 1000.0f * (maximo - minimo);
}


static int LimitePesca(int cantidadMaxima)
{
    return cantidadMaxima < MAX_PARTICIPANTES
        ? cantidadMaxima
        : MAX_PARTICIPANTES;
}


static void AsignarColoresPesca(
    MinijuegoPescaIsla& m,
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

                if (
                    otro.r == respaldo[c].r &&
                    otro.g == respaldo[c].g &&
                    otro.b == respaldo[c].b
                )
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


//==================================================
// LOGICA: MUELLES Y PECES (datos logicos, sin dependencia visual)
//==================================================

// Muelles: sur, norte, oeste, este. Con 2 jugadores quedan enfrentados.
static Vector3 PosicionMuellePesca(int muelle)
{
    switch (muelle % 4)
    {
        case 0: return { 0.0f, ALTURA_MUELLE_PESCA, DISTANCIA_MUELLE_PESCA };
        case 1: return { 0.0f, ALTURA_MUELLE_PESCA, -DISTANCIA_MUELLE_PESCA };
        case 2: return { -DISTANCIA_MUELLE_PESCA, ALTURA_MUELLE_PESCA, 0.0f };
        default: return { DISTANCIA_MUELLE_PESCA, ALTURA_MUELLE_PESCA, 0.0f };
    }
}


static Vector3 PuntaCanaPesca(int muelle)
{
    Vector3 p = PosicionMuellePesca(muelle);
    float longitud = std::sqrt(p.x * p.x + p.z * p.z);

    return { p.x - p.x / longitud * 1.4f, 2.7f, p.z - p.z / longitud * 1.4f };
}


static void AparecerPezPesca(PezPesca& pez, TipoPezPesca tipo)
{
    float angulo = AleatorioPesca(0.0f, 2.0f * PI);
    float radio = AleatorioPesca(0.0f, 6.2f);
    float rumbo = AleatorioPesca(0.0f, 2.0f * PI);

    pez = {};
    pez.estado = ESTADO_PEZ_NADANDO;
    pez.tipo = tipo;
    pez.x = std::cos(angulo) * radio;
    pez.z = std::sin(angulo) * radio;
    pez.dirX = std::cos(rumbo);
    pez.dirZ = std::sin(rumbo);
    pez.velocidad = VELOCIDAD_PEZ_PESCA[tipo];
    pez.tiempoCambio = AleatorioPesca(1.0f, 3.0f);
    pez.temporizadorMorder = -1.0f;
    pez.semilla = AleatorioPesca(0.0f, 6.0f);
}


static void MoverPezPesca(PezPesca& pez, float velocidad, float deltaTime)
{
    float nx = pez.x + pez.dirX * velocidad * deltaTime;
    float nz = pez.z + pez.dirZ * velocidad * deltaTime;
    float radio = std::sqrt(nx * nx + nz * nz);

    if (radio > RADIO_NADO_PESCA)
    {
        pez.dirX = -pez.x / (radio > 0.01f ? radio : 1.0f);
        pez.dirZ = -pez.z / (radio > 0.01f ? radio : 1.0f);
        return;
    }

    pez.x = nx;
    pez.z = nz;
}


static float DistanciaPesca(float ax, float az, float bx, float bz)
{
    float dx = ax - bx;
    float dz = az - bz;
    return std::sqrt(dx * dx + dz * dz);
}


static void LiberarJugadorPesca(
    EstadoJugadorPesca& estado,
    float bloqueo
)
{
    estado.estado = PESCA_LIBRE;
    estado.pez = -1;
    estado.bloqueo = bloqueo;
    estado.tension = 0.0f;
    estado.progreso = 0.0f;
    estado.cursorX = estado.anzueloX;
    estado.cursorZ = estado.anzueloZ;
}


static void MorderPezPesca(MinijuegoPescaIsla& m, int indicePez, int limite)
{
    PezPesca& pez = m.peces[indicePez];
    bool alguno = false;

    for (int i = 0; i < limite; i++)
    {
        EstadoJugadorPesca& estado = m.estados[i];

        if (
            m.participa[i] &&
            estado.estado == PESCA_ESPERANDO &&
            DistanciaPesca(estado.anzueloX, estado.anzueloZ, pez.x, pez.z) <= RADIO_PIQUE_PESCA
        )
        {
            estado.estado = PESCA_PICADA;
            estado.pez = indicePez;
            estado.tiempoEstado = 0.0f;
            estado.botReaccion = AleatorioPesca(0.15f, 0.40f);
            estado.botFalla = GetRandomValue(0, 99) < 6;
            alguno = true;
        }
    }

    if (!alguno)
    {
        pez.temporizadorMorder = -1.0f;
        return;
    }

    pez.estado = ESTADO_PEZ_MORDIENDO;
    pez.ventana = VENTANA_PIQUE_PESCA;
    ReproducirSonidoMinijuego(m.audio, SONIDO_PLATAFORMA);
}


static void ActualizarPecesPesca(MinijuegoPescaIsla& m, float deltaTime, int limite)
{
    for (int f = 0; f < MAX_PECES_PESCA; f++)
    {
        PezPesca& pez = m.peces[f];

        if (pez.estado == ESTADO_PEZ_AUSENTE)
        {
            pez.ausente -= deltaTime;

            if (pez.ausente <= 0.0f)
            {
                AparecerPezPesca(pez, pez.tipo);
            }
        }
        else if (pez.estado == ESTADO_PEZ_NADANDO)
        {
            pez.ignorar -= deltaTime;

            int atractor = -1;
            float mejorDistancia = RADIO_ATRAER_PESCA;

            if (pez.ignorar <= 0.0f)
            {
                for (int i = 0; i < limite; i++)
                {
                    const EstadoJugadorPesca& estado = m.estados[i];

                    if (!m.participa[i] || estado.estado != PESCA_ESPERANDO || estado.tiempoEstado < 0.4f)
                    {
                        continue;
                    }

                    float d = DistanciaPesca(estado.anzueloX, estado.anzueloZ, pez.x, pez.z);

                    if (d < mejorDistancia)
                    {
                        mejorDistancia = d;
                        atractor = i;
                    }
                }
            }

            if (atractor >= 0)
            {
                const EstadoJugadorPesca& estado = m.estados[atractor];
                float dx = estado.anzueloX - pez.x;
                float dz = estado.anzueloZ - pez.z;
                float d = mejorDistancia > 0.01f ? mejorDistancia : 0.01f;

                pez.dirX += (dx / d - pez.dirX) * LimitarPesca(deltaTime * 6.0f, 0.0f, 1.0f);
                pez.dirZ += (dz / d - pez.dirZ) * LimitarPesca(deltaTime * 6.0f, 0.0f, 1.0f);

                float largo = std::sqrt(pez.dirX * pez.dirX + pez.dirZ * pez.dirZ);

                if (largo > 0.01f)
                {
                    pez.dirX /= largo;
                    pez.dirZ /= largo;
                }

                if (d > 0.6f)
                {
                    MoverPezPesca(pez, pez.velocidad * 1.2f, deltaTime);
                    pez.temporizadorMorder = -1.0f;
                }
                else
                {
                    if (pez.temporizadorMorder < 0.0f)
                    {
                        pez.temporizadorMorder = AleatorioPesca(0.5f, 1.8f);
                    }

                    pez.temporizadorMorder -= deltaTime;

                    if (pez.temporizadorMorder <= 0.0f)
                    {
                        MorderPezPesca(m, f, limite);
                    }
                }
            }
            else
            {
                pez.temporizadorMorder = -1.0f;
                pez.tiempoCambio -= deltaTime;

                if (pez.tiempoCambio <= 0.0f)
                {
                    float angulo = std::atan2(pez.dirZ, pez.dirX) + AleatorioPesca(-1.2f, 1.2f);
                    pez.dirX = std::cos(angulo);
                    pez.dirZ = std::sin(angulo);
                    pez.tiempoCambio = AleatorioPesca(1.0f, 3.0f);
                }

                MoverPezPesca(pez, pez.velocidad, deltaTime);
            }
        }
        else if (pez.estado == ESTADO_PEZ_MORDIENDO)
        {
            pez.ventana -= deltaTime;

            if (pez.ventana <= 0.0f)
            {
                bool huboAfectados = false;

                for (int i = 0; i < limite; i++)
                {
                    if (m.participa[i] && m.estados[i].estado == PESCA_PICADA && m.estados[i].pez == f)
                    {
                        LiberarJugadorPesca(m.estados[i], BLOQUEO_FALLO_PESCA);
                        huboAfectados = true;
                    }
                }

                if (huboAfectados)
                {
                    ReproducirSonidoMinijuego(m.audio, SONIDO_ERROR);
                }

                float angulo = AleatorioPesca(0.0f, 2.0f * PI);
                pez.estado = ESTADO_PEZ_NADANDO;
                pez.ignorar = 3.0f;
                pez.dirX = std::cos(angulo);
                pez.dirZ = std::sin(angulo);
                pez.temporizadorMorder = -1.0f;
            }
        }
    }
}


static void EngancharPesca(MinijuegoPescaIsla& m, int indice, int limite)
{
    EstadoJugadorPesca& estado = m.estados[indice];
    int indicePez = estado.pez;
    PezPesca& pez = m.peces[indicePez];

    // Los demas anzuelos que rodeaban al mismo pez pierden la carnada.
    bool perdedores = false;

    for (int j = 0; j < limite; j++)
    {
        if (j != indice && m.participa[j] && m.estados[j].estado == PESCA_PICADA && m.estados[j].pez == indicePez)
        {
            LiberarJugadorPesca(m.estados[j], BLOQUEO_PERDIDA_PESCA);
            perdedores = true;
        }
    }

    if (perdedores)
    {
        ReproducirSonidoMinijuego(m.audio, SONIDO_ERROR);
    }

    if (pez.tipo == PEZ_PESCA_BOTA)
    {
        LiberarJugadorPesca(estado, BLOQUEO_BOTA_PESCA);
        pez.estado = ESTADO_PEZ_AUSENTE;
        pez.ausente = 4.0f;
        ReproducirSonidoMinijuego(m.audio, SONIDO_ERROR);
        return;
    }

    pez.estado = ESTADO_PEZ_ENGANCHADO;
    pez.jugador = indice;
    estado.estado = PESCA_TENSION;
    estado.tension = 0.15f;
    estado.progreso = 0.0f;
    estado.tiempoPelea = 0.0f;
    estado.recogiendo = true;
}


static void ActualizarTensionPesca(
    MinijuegoPescaIsla& m,
    int indice,
    bool alternar,
    float deltaTime
)
{
    EstadoJugadorPesca& estado = m.estados[indice];
    PezPesca& pez = m.peces[estado.pez];

    if (alternar)
    {
        estado.recogiendo = !estado.recogiendo;
    }

    float lucha = 1.0f + 0.4f * std::sin(m.tiempoAnimacion * 3.0f + pez.semilla);

    if (estado.recogiendo)
    {
        estado.tension += TENSION_PESCA[pez.tipo] * lucha * deltaTime;
        estado.progreso += deltaTime / DURACION_PELEA_PESCA[pez.tipo];
    }
    else
    {
        estado.tension -= 0.9f * deltaTime;
        estado.progreso -= 0.12f * deltaTime;
    }

    estado.tension = LimitarPesca(estado.tension, 0.0f, 1.2f);
    estado.progreso = LimitarPesca(estado.progreso, 0.0f, 1.0f);
    estado.tiempoPelea += deltaTime;

    if (estado.tension >= 1.0f || estado.tiempoPelea > TIEMPO_MAXIMO_PELEA_PESCA)
    {
        float angulo = AleatorioPesca(0.0f, 2.0f * PI);
        pez.estado = ESTADO_PEZ_NADANDO;
        pez.jugador = -1;
        pez.x = estado.anzueloX;
        pez.z = estado.anzueloZ;
        pez.ignorar = 3.0f;
        pez.dirX = std::cos(angulo);
        pez.dirZ = std::sin(angulo);
        pez.temporizadorMorder = -1.0f;
        LiberarJugadorPesca(estado, BLOQUEO_BOTA_PESCA);
        ReproducirSonidoMinijuego(m.audio, SONIDO_ERROR);
        return;
    }

    if (estado.progreso >= 1.0f)
    {
        int valor = VALOR_PEZ_PESCA[pez.tipo];

        estado.puntos += valor;
        estado.peces++;

        if (valor > estado.mejorValor)
        {
            estado.mejorValor = valor;
        }

        estado.popupValor = valor;
        estado.popupTiempo = 1.6f;
        pez.estado = ESTADO_PEZ_AUSENTE;
        pez.jugador = -1;
        pez.ausente = 3.0f;
        LiberarJugadorPesca(estado, 0.6f);
        ReproducirSonidoMinijuego(
            m.audio,
            pez.tipo == PEZ_PESCA_RARO ? SONIDO_RECOGER_NUCLEO_ESPECIAL : SONIDO_ACIERTO
        );
    }
}


static void FinalizarPesca(MinijuegoPescaIsla& m, int limite)
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

            const EstadoJugadorPesca& a = m.estados[j];
            const EstadoJugadorPesca& b = m.estados[i];

            if (a.puntos > b.puntos || (a.puntos == b.puntos && a.mejorValor > b.mejorValor))
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
    m.fase = FASE_PESCA_TERMINADO;
    ReproducirSonidoMinijuego(m.audio, SONIDO_RESULTADO);
}


//==================================================
// IA DE BOTS
//==================================================

static void DirigirCursorPesca(InputMinijuegoParticipante& entrada, float dx, float dz)
{
    float longitud = std::sqrt(dx * dx + dz * dz);

    if (longitud < 0.25f)
    {
        return;
    }

    const float umbral = 0.38f;
    entrada.derecha = dx / longitud > umbral;
    entrada.izquierda = dx / longitud < -umbral;
    entrada.atras = dz / longitud > umbral;
    entrada.adelante = dz / longitud < -umbral;
}


static InputMinijuegoParticipante CrearEntradaBotPesca(
    MinijuegoPescaIsla& m,
    int indice,
    float deltaTime
)
{
    InputMinijuegoParticipante entrada{};
    EstadoJugadorPesca& estado = m.estados[indice];

    if (estado.estado == PESCA_LIBRE)
    {
        estado.botReevaluar -= deltaTime;
        estado.botEspera -= deltaTime;

        if (estado.botReevaluar <= 0.0f)
        {
            estado.botReevaluar = 0.5f;
            int mejor = -1;
            float mejorPuntaje = -1.0f;

            for (int f = 0; f < MAX_PECES_PESCA; f++)
            {
                const PezPesca& pez = m.peces[f];

                if (pez.estado != ESTADO_PEZ_NADANDO || pez.tipo == PEZ_PESCA_BOTA || pez.ignorar > 0.0f)
                {
                    continue;
                }

                float distancia = DistanciaPesca(pez.x, pez.z, estado.cursorX, estado.cursorZ);
                float puntaje = (float)VALOR_PEZ_PESCA[pez.tipo] / (1.0f + distancia * 0.2f);

                if (puntaje > mejorPuntaje)
                {
                    mejorPuntaje = puntaje;
                    mejor = f;
                }
            }

            estado.botTieneObjetivo = mejor >= 0;

            if (mejor >= 0)
            {
                const PezPesca& pez = m.peces[mejor];
                estado.botObjetivoX = pez.x + pez.dirX * pez.velocidad * 0.7f + AleatorioPesca(-0.8f, 0.8f);
                estado.botObjetivoZ = pez.z + pez.dirZ * pez.velocidad * 0.7f + AleatorioPesca(-0.8f, 0.8f);
            }
        }

        if (estado.botTieneObjetivo)
        {
            float dx = estado.botObjetivoX - estado.cursorX;
            float dz = estado.botObjetivoZ - estado.cursorZ;
            DirigirCursorPesca(entrada, dx, dz);

            if (std::sqrt(dx * dx + dz * dz) < 0.6f && estado.bloqueo <= 0.0f && estado.botEspera <= 0.0f)
            {
                entrada.golpear = true;
                estado.botEspera = AleatorioPesca(0.3f, 1.0f);
            }
        }
    }
    else if (estado.estado == PESCA_ESPERANDO)
    {
        if (estado.tiempoEstado > 7.0f)
        {
            entrada.golpear = true;
        }
    }
    else if (estado.estado == PESCA_PICADA)
    {
        estado.botReaccion -= deltaTime;

        if (estado.botReaccion <= 0.0f && !estado.botFalla)
        {
            entrada.golpear = true;
        }
    }
    else if (estado.estado == PESCA_TENSION)
    {
        if (estado.recogiendo && estado.tension > 0.82f)
        {
            entrada.golpear = true;
        }
        else if (!estado.recogiendo && estado.tension < 0.35f)
        {
            entrada.golpear = true;
        }
    }

    return entrada;
}


//==================================================
// CICLO DE VIDA
//==================================================

// Mallas horneadas del decorado (seccion visual).
static void LiberarMallasPesca();


void MinijuegoPescaIsla::Inicializar()
{
    resultado = {};
    resultado.formato = FORMATO_MINIJUEGO_INDIVIDUAL;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        estados[i] = {};
        participa[i] = false;
        muelleDe[i] = 0;
        coloresJugadores[i] = LIGHTGRAY;
    }

    for (int f = 0; f < MAX_PECES_PESCA; f++)
    {
        AparecerPezPesca(peces[f], TIPOS_INICIALES_PESCA[f]);
    }

    // Vista de tres cuartos desde el sur: los cuatro muelles entran entre
    // el tendedero del HUD y la franja de ayuda (solo visual).
    camara.position = { 0.0f, 16.0f, 21.0f };
    camara.target = { 0.0f, 0.0f, 0.5f };
    camara.up = { 0.0f, 1.0f, 0.0f };
    camara.fovy = 44.0f;
    camara.projection = CAMERA_PERSPECTIVE;

    fase = FASE_PESCA_PREPARACION;
    tiempoPreparacion = DURACION_PREPARACION_PESCA;
    tiempoRestante = DURACION_PARTIDA_PESCA;
    tiempoAnimacion = 0.0f;

    // Estado visual.
    for (int k = 0; k < MAX_PARTICULAS_PESCA; k++)
    {
        particulas[k] = {};
    }

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        vueloTiempo[i] = -1.0f;
        vueloTipo[i] = 0;
        vueloDesde[i] = { 0.0f, 0.0f, 0.0f };
        avisoTipo[i] = AVISO_PESCA_NINGUNO;
        avisoTiempo[i] = 0.0f;
        avisoPosicion[i] = { 0.0f, 0.0f, 0.0f };
        destelloJugador[i] = 0.0f;
        chapoteo[i] = 0.0f;
    }

    semillaVisual = 0x5EA1u;
    anuncioDorado = 99.0f;
    jugadorDorado = -1;
    tiempoFinVisual = 0.0f;
}


void MinijuegoPescaIsla::Reiniciar(
    JugadorPrueba jugadores[],
    Participante participantes[],
    int cantidadMaxima
)
{
    // Cada partida vuelve a hornear la isla: no quedan mallas de la anterior.
    LiberarMallasPesca();
    Inicializar();
    InicializarResultadoMinijuego(
        resultado,
        participantes,
        FORMATO_MINIJUEGO_INDIVIDUAL
    );

    AsignarColoresPesca(*this, participantes);

    int indices[MAX_PARTICIPANTES]{};
    int cantidad = ObtenerIndicesParticipantesActivos(
        participantes,
        indices,
        MAX_PARTICIPANTES
    );

    if (cantidad < 2)
    {
        resultado.estado = RESULTADO_MINIJUEGO_CANCELADO;
        fase = FASE_PESCA_TERMINADO;
        return;
    }

    CargarPiezasJugablesPescaIsla();
    int limite = LimitePesca(cantidadMaxima);

    for (int k = 0; k < cantidad; k++)
    {
        int i = indices[k];

        if (i >= limite)
        {
            continue;
        }

        Vector3 muelle = PosicionMuellePesca(k);
        Vector3 spawn = { muelle.x, ALTURA_MUELLE_PESCA + 0.72f, muelle.z };
        float longitud = std::sqrt(muelle.x * muelle.x + muelle.z * muelle.z);

        ConfigurarJugadorMinijuegoEstandar(jugadores[i], spawn);
        jugadores[i].direccionMirada = { -muelle.x / longitud, 0.0f, -muelle.z / longitud };
        participa[i] = true;
        muelleDe[i] = k;
        estados[i].cursorX = muelle.x * 0.4f;
        estados[i].cursorZ = muelle.z * 0.4f;
        estados[i].botEspera = AleatorioPesca(0.5f, 1.5f);
    }
}


//==================================================
// ACTUALIZACION
//==================================================

void MinijuegoPescaIsla::ActualizarPartida(
    float deltaTime,
    JugadorPrueba jugadores[],
    int cantidadMaxima,
    Participante participantes[]
)
{
    (void)jugadores;
    tiempoAnimacion += deltaTime;

    if (
        fase == FASE_PESCA_TERMINADO ||
        resultado.estado == RESULTADO_MINIJUEGO_CANCELADO
    )
    {
        return;
    }

    if (fase == FASE_PESCA_PREPARACION)
    {
        float preparacionAntes = tiempoPreparacion;
        tiempoPreparacion -= deltaTime;
        ActualizarAudioCuentaRegresiva(audio, preparacionAntes, tiempoPreparacion);

        if (tiempoPreparacion <= 0.0f)
        {
            tiempoPreparacion = 0.0f;
            fase = FASE_PESCA_JUGANDO;
        }

        return;
    }

    int limite = LimitePesca(cantidadMaxima);

    float restanteAntes = tiempoRestante;
    tiempoRestante -= deltaTime;
    ActualizarAudioAlertaTiempo(audio, restanteAntes, tiempoRestante);

    ActualizarPecesPesca(*this, deltaTime, limite);

    for (int i = 0; i < limite; i++)
    {
        if (!participa[i])
        {
            continue;
        }

        EstadoJugadorPesca& estado = estados[i];
        InputMinijuegoParticipante entrada{};

        if (participantes[i].esBot || !participantes[i].conectado)
        {
            entrada = CrearEntradaBotPesca(*this, i, deltaTime);
        }
        else
        {
            entrada = LeerInputMinijuegoParticipante(participantes[i]);
        }

        bool accion = entrada.golpear || entrada.saltar;

        estado.tiempoEstado += deltaTime;

        if (estado.popupTiempo > 0.0f)
        {
            estado.popupTiempo -= deltaTime;
        }

        if (estado.bloqueo > 0.0f)
        {
            estado.bloqueo -= deltaTime;

            if (estado.bloqueo < 0.0f)
            {
                estado.bloqueo = 0.0f;
            }
        }

        switch (estado.estado)
        {
            case PESCA_LIBRE:
            {
                float dx = (entrada.derecha ? 1.0f : 0.0f) - (entrada.izquierda ? 1.0f : 0.0f);
                float dz = (entrada.atras ? 1.0f : 0.0f) - (entrada.adelante ? 1.0f : 0.0f);
                float longitud = std::sqrt(dx * dx + dz * dz);

                if (longitud > 0.0f)
                {
                    estado.cursorX += dx / longitud * VELOCIDAD_CURSOR_PESCA * deltaTime;
                    estado.cursorZ += dz / longitud * VELOCIDAD_CURSOR_PESCA * deltaTime;
                }

                float radio = std::sqrt(estado.cursorX * estado.cursorX + estado.cursorZ * estado.cursorZ);

                if (radio > RADIO_NADO_PESCA)
                {
                    estado.cursorX *= RADIO_NADO_PESCA / radio;
                    estado.cursorZ *= RADIO_NADO_PESCA / radio;
                }

                if (accion && estado.bloqueo <= 0.0f)
                {
                    estado.estado = PESCA_LANZANDO;
                    estado.tiempoEstado = 0.0f;
                    estado.anzueloX = estado.cursorX;
                    estado.anzueloZ = estado.cursorZ;
                    ReproducirSonidoMinijuego(audio, SONIDO_DISPARO);
                }

                break;
            }

            case PESCA_LANZANDO:
            {
                if (estado.tiempoEstado >= DURACION_LANZAMIENTO_PESCA)
                {
                    estado.estado = PESCA_ESPERANDO;
                    estado.tiempoEstado = 0.0f;
                }

                break;
            }

            case PESCA_ESPERANDO:
            {
                if (accion || estado.tiempoEstado > ESPERA_MAXIMA_PESCA)
                {
                    LiberarJugadorPesca(estado, 0.5f);
                }

                break;
            }

            case PESCA_PICADA:
            {
                if (accion)
                {
                    EngancharPesca(*this, i, limite);
                }

                break;
            }

            case PESCA_TENSION:
            {
                ActualizarTensionPesca(*this, i, accion, deltaTime);
                break;
            }
        }
    }

    // Los peces enganchados siguen al anzuelo.
    for (int f = 0; f < MAX_PECES_PESCA; f++)
    {
        PezPesca& pez = peces[f];

        if (pez.estado == ESTADO_PEZ_ENGANCHADO && pez.jugador >= 0)
        {
            pez.x = estados[pez.jugador].anzueloX + std::sin(tiempoAnimacion * 9.0f + pez.semilla) * 0.25f;
            pez.z = estados[pez.jugador].anzueloZ + std::cos(tiempoAnimacion * 7.0f + pez.semilla) * 0.25f;
        }
    }

    if (tiempoRestante <= 0.0f)
    {
        tiempoRestante = 0.0f;
        FinalizarPesca(*this, limite);
    }
}



//==================================================
// VISUAL: LAGUNA TROPICAL AL ATARDECER
//==================================================
//
// Capas, de adelante hacia atras: la laguna calma de agua turquesa
// transparente (arrecife, pastos marinos, estrellas de mar y los peces
// visibles debajo, con la cuerda de boyas que marca la zona de pesca) y
// los cuatro muelles de bambu de los jugadores (farol, banderin de su
// color, balde con lo pescado); un anillo de playa con palmeras,
// antorchas tiki, cabanas de paja, barca, canoa con balancin, redes,
// cajones, barriles, rocas y matas; la selva y las faldas del volcan;
// el mar abierto con islotes y un velero; y el cielo de atardecer con el
// sol bajo, nubes y gaviotas. Todo con primitivas agrupadas por rlgl: los
// unicos GLB son las piezas jugables (cana, corcho, peces, bota y mira).
// El azar visual sale de semillaVisual o de un hash por indice; nunca de
// GetRandomValue.
//==================================================


static const Color COLOR_CIELO_ALTO_PESCA = { 44, 42, 98, 255 };
static const Color COLOR_CIELO_MEDIO_PESCA = { 176, 92, 140, 255 };
static const Color COLOR_HORIZONTE_PESCA = { 255, 170, 108, 255 };
static const Color COLOR_NIEBLA_PESCA = { 228, 150, 140, 255 };
static const Color COLOR_SOL_PESCA = { 255, 222, 150, 255 };
static const Color COLOR_ARENA_PESCA = { 238, 208, 152, 255 };
static const Color COLOR_ARENA_MOJADA_PESCA = { 196, 158, 116, 255 };
static const Color COLOR_ARENA_FONDO_PESCA = { 226, 212, 168, 255 };
static const Color COLOR_PASTO_PESCA = { 104, 156, 74, 255 };
static const Color COLOR_SELVA_PESCA = { 48, 100, 64, 255 };
static const Color COLOR_HOJA_PESCA = { 74, 150, 72, 255 };
static const Color COLOR_BAMBU_PESCA = { 216, 186, 104, 255 };
static const Color COLOR_BAMBU_OSCURO_PESCA = { 160, 128, 62, 255 };
static const Color COLOR_PAJA_PESCA = { 210, 164, 94, 255 };
static const Color COLOR_MADERA_PESCA = { 130, 86, 56, 255 };
static const Color COLOR_MADERA_OSCURA_PESCA = { 86, 58, 44, 255 };
static const Color COLOR_ROCA_PESCA = { 84, 70, 80, 255 };
static const Color COLOR_AGUA_PESCA = { 40, 186, 196, 255 };
static const Color COLOR_AGUA_HONDA_PESCA = { 16, 98, 136, 255 };
static const Color COLOR_MAR_PESCA = { 30, 74, 116, 255 };
static const Color COLOR_LUZ_PESCA = { 255, 178, 96, 255 };
static const Color COLOR_LAVA_PESCA = { 255, 112, 42, 255 };
static const Color COLOR_DORADO_PESCA = { 255, 206, 70, 255 };

// Vista de juego (igual a la camara de Inicializar) y toma de entrada.
static const Vector3 OJO_INTRO_PESCA = { -3.0f, 4.4f, 37.0f };
static const Vector3 MIRA_INTRO_PESCA = { 2.0f, 6.2f, -20.0f };

// Sol bajo al oeste, sobre el mar.
static const Vector3 DIRECCION_SOL_PESCA = { -0.8200f, 0.1000f, -0.5634f };

static const Vector3 CENTRO_VOLCAN_PESCA = { 9.0f, -1.0f, -58.0f };
static const float RADIO_VOLCAN_PESCA = 32.0f;
static const float ALTURA_VOLCAN_PESCA = 26.0f;

static const float Y_MAR_PESCA = -0.12f;
static const float DURACION_VUELO_PESCA = 0.75f;
static const float DURACION_AVISO_PESCA = 1.4f;

// Ojo y ejes de la vista del frame (para niebla y carteles de cara).
static Vector3 ojoPesca = { 0.0f, 16.0f, 21.0f };
static Vector3 derechaVistaPesca = { 1.0f, 0.0f, 0.0f };
static Vector3 arribaVistaPesca = { 0.0f, 1.0f, 0.0f };


//--------------------------------------------------
// Utilidades de color y geometria
//--------------------------------------------------

static float Limitar01Pesca(float valor)
{
    if (valor < 0.0f) return 0.0f;
    if (valor > 1.0f) return 1.0f;
    return valor;
}


static unsigned char CanalPesca(float valor)
{
    if (valor < 0.0f) return 0;
    if (valor > 255.0f) return 255;
    return (unsigned char)valor;
}


static Color TonoPesca(Color color, float factor)
{
    return Color{
        CanalPesca((float)color.r * factor),
        CanalPesca((float)color.g * factor),
        CanalPesca((float)color.b * factor),
        color.a
    };
}


static Color MezclaPesca(Color a, Color b, float t)
{
    t = Limitar01Pesca(t);

    return Color{
        CanalPesca((float)a.r + ((float)b.r - (float)a.r) * t),
        CanalPesca((float)a.g + ((float)b.g - (float)a.g) * t),
        CanalPesca((float)a.b + ((float)b.b - (float)a.b) * t),
        CanalPesca((float)a.a + ((float)b.a - (float)a.a) * t)
    };
}


static Color AlfaPesca(Color color, float alfa)
{
    color.a = CanalPesca(255.0f * alfa);
    return color;
}


static float RuidoPesca(int indice, int semilla)
{
    unsigned int h =
        (unsigned int)indice * 374761393u +
        (unsigned int)semilla * 668265263u;

    h = (h ^ (h >> 13)) * 1274126177u;
    h = h ^ (h >> 16);

    return (float)(h & 0xFFFFu) / 65535.0f;
}


static float SuavePesca(float u)
{
    u = Limitar01Pesca(u);
    return u * u * (3.0f - 2.0f * u);
}


static Vector3 SumaPesca(Vector3 a, Vector3 b) { return { a.x + b.x, a.y + b.y, a.z + b.z }; }
static Vector3 RestaPesca(Vector3 a, Vector3 b) { return { a.x - b.x, a.y - b.y, a.z - b.z }; }
static Vector3 EscalaPesca(Vector3 a, float s) { return { a.x * s, a.y * s, a.z * s }; }


static Vector3 LerpPesca(Vector3 a, Vector3 b, float t)
{
    return { a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t };
}


static Vector3 NormalizarPesca(Vector3 a)
{
    float l = std::sqrt(a.x * a.x + a.y * a.y + a.z * a.z);
    if (l < 0.0001f) return { 0.0f, 1.0f, 0.0f };
    return { a.x / l, a.y / l, a.z / l };
}


static Vector3 CruzPesca(Vector3 a, Vector3 b)
{
    return { a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x };
}


// Horneado: lo estatico de la isla se graba una vez en mallas (la niebla y
// la caustica del fondo las pone el shader por pixel). Mientras se graba,
// VerticePesca junta triangulos (con la matriz de la pila propia) en lugar
// de mandarlos a rlgl.
struct GrabacionPesca
{
    bool activa = false;
    int modo = RL_TRIANGLES;
    int enQuad = 0;
    Vector3 quad[4]{};
    Color quadColor[4]{};
    float quadCaustica[4]{};
    float caustica = 0.0f;  // peso de la caustica del proximo vertice
    float modoShader = 0.0f; // shader: 0 normal, 1 sin niebla (cielo), 2 agua con ondas
    Matrix matriz = MatrixIdentity();
    Matrix pila[8]{};
    int profundidad = 0;
    std::vector<float> posiciones;
    std::vector<unsigned char> colores;
    std::vector<float> causticas;
};

static GrabacionPesca g_grabacionPesca;

// Que partes dibujan las funciones del decorado: todo (sin horno), solo lo
// estatico (mientras se graba) o solo lo animado (con el horno listo).
enum PartePesca { PARTE_PESCA_TODO = 0, PARTE_PESCA_ESTATICA, PARTE_PESCA_ANIMADA };
static PartePesca g_partePesca = PARTE_PESCA_TODO;

static bool EstaticoPesca() { return g_partePesca != PARTE_PESCA_ANIMADA; }
static bool AnimadoPesca() { return g_partePesca != PARTE_PESCA_ESTATICA; }


static void BeginPesca(int modo)
{
    if (g_grabacionPesca.activa)
    {
        g_grabacionPesca.modo = modo;
        g_grabacionPesca.enQuad = 0;
        return;
    }

    rlBegin(modo);
}


static void EndPesca()
{
    if (!g_grabacionPesca.activa)
    {
        rlEnd();
    }
}


static void EmpujarPesca()
{
    GrabacionPesca& g = g_grabacionPesca;

    if (!g.activa)
    {
        rlPushMatrix();
        return;
    }

    if (g.profundidad < 8) g.pila[g.profundidad] = g.matriz;
    g.profundidad++;
}


static void SoltarPesca()
{
    GrabacionPesca& g = g_grabacionPesca;

    if (!g.activa)
    {
        rlPopMatrix();
        return;
    }

    if (g.profundidad > 0) g.profundidad--;
    if (g.profundidad < 8) g.matriz = g.pila[g.profundidad];
}


// Mismo orden que rlgl: la transformacion nueva se aplica primero.
static void TrasladarPesca(float x, float y, float z)
{
    if (!g_grabacionPesca.activa)
    {
        rlTranslatef(x, y, z);
        return;
    }

    g_grabacionPesca.matriz = MatrixMultiply(MatrixTranslate(x, y, z), g_grabacionPesca.matriz);
}


static void RotarPesca(float grados, float x, float y, float z)
{
    if (!g_grabacionPesca.activa)
    {
        rlRotatef(grados, x, y, z);
        return;
    }

    g_grabacionPesca.matriz = MatrixMultiply(MatrixRotate({ x, y, z }, grados * DEG2RAD), g_grabacionPesca.matriz);
}


static void GrabarVerticePesca(Vector3 p, Color c, float caustica)
{
    GrabacionPesca& g = g_grabacionPesca;
    p = Vector3Transform(p, g.matriz);
    g.posiciones.push_back(p.x);
    g.posiciones.push_back(p.y);
    g.posiciones.push_back(p.z);
    g.colores.push_back(c.r);
    g.colores.push_back(c.g);
    g.colores.push_back(c.b);
    g.colores.push_back(c.a);
    g.causticas.push_back(caustica);
}


// Bruma tibia del atardecer: lo lejano se funde con el aire rosado (al
// grabar la pone el shader por pixel).
static float CantidadNieblaPesca(Vector3 p)
{
    if (g_grabacionPesca.activa)
    {
        return 0.0f;
    }

    float dx = p.x - ojoPesca.x;
    float dy = p.y - ojoPesca.y;
    float dz = p.z - ojoPesca.z;
    float d = std::sqrt(dx * dx + dy * dy + dz * dz);
    float n = (d - 32.0f) / 120.0f;

    return n < 0.0f ? 0.0f : (n > 0.72f ? 0.72f : n);
}


static Color TintePesca(Color color, Vector3 p, float emisivo = 0.0f)
{
    Color niebla = COLOR_NIEBLA_PESCA;
    niebla.a = color.a;
    return MezclaPesca(color, niebla, CantidadNieblaPesca(p) * (1.0f - 0.65f * emisivo));
}


static void VerticePesca(Vector3 p, Color color)
{
    GrabacionPesca& g = g_grabacionPesca;

    if (g.activa)
    {
        float caustica = g.caustica;
        g.caustica = 0.0f;

        if (g.modo == RL_QUADS)
        {
            g.quad[g.enQuad] = p;
            g.quadColor[g.enQuad] = color;
            g.quadCaustica[g.enQuad] = caustica;

            if (++g.enQuad == 4)
            {
                g.enQuad = 0;
                static const int ORDEN[6] = { 0, 1, 2, 0, 2, 3 };
                for (int k : ORDEN) GrabarVerticePesca(g.quad[k], g.quadColor[k], g.quadCaustica[k]);
            }
        }
        else if (g.modo == RL_TRIANGLES)
        {
            GrabarVerticePesca(p, color, caustica);
        }

        return;
    }

    rlColor4ub(color.r, color.g, color.b, color.a);
    rlVertex3f(p.x, p.y, p.z);
}


static void QuadPesca(Vector3 a, Vector3 b, Vector3 c, Vector3 d, Color color)
{
    VerticePesca(a, color);
    VerticePesca(b, color);
    VerticePesca(c, color);
    VerticePesca(d, color);
}


// Luz de la escena: sol bajo desde la izquierda y algo de frente.
static float LuzNormalPesca(Vector3 n)
{
    const float LX = -0.45f, LY = 0.80f, LZ = 0.40f;
    float d = n.x * LX + n.y * LY + n.z * LZ;
    return 0.55f + 0.45f * Limitar01Pesca(d);
}


// Caja con caras sombreadas (arriba 1.0, frente 0.78, izquierda 0.70,
// derecha 0.62, fondo 0.55), sin niebla. Respeta la matriz de rlgl.
static void CuboCrudoPesca(Vector3 c, Vector3 t, Color color)
{
    float x0 = c.x - t.x * 0.5f, x1 = c.x + t.x * 0.5f;
    float y0 = c.y - t.y * 0.5f, y1 = c.y + t.y * 0.5f;
    float z0 = c.z - t.z * 0.5f, z1 = c.z + t.z * 0.5f;

    BeginPesca(RL_QUADS);
    QuadPesca({ x0, y1, z1 }, { x1, y1, z1 }, { x1, y1, z0 }, { x0, y1, z0 }, color);
    QuadPesca({ x0, y0, z1 }, { x1, y0, z1 }, { x1, y1, z1 }, { x0, y1, z1 }, TonoPesca(color, 0.78f));
    QuadPesca({ x1, y0, z0 }, { x0, y0, z0 }, { x0, y1, z0 }, { x1, y1, z0 }, TonoPesca(color, 0.55f));
    QuadPesca({ x1, y0, z1 }, { x1, y0, z0 }, { x1, y1, z0 }, { x1, y1, z1 }, TonoPesca(color, 0.62f));
    QuadPesca({ x0, y0, z0 }, { x0, y0, z1 }, { x0, y1, z1 }, { x0, y1, z0 }, TonoPesca(color, 0.70f));
    QuadPesca({ x0, y0, z0 }, { x1, y0, z0 }, { x1, y0, z1 }, { x0, y0, z1 }, TonoPesca(color, 0.45f));
    EndPesca();
}


static void CuboPesca(Vector3 c, Vector3 t, Color color)
{
    CuboCrudoPesca(c, t, TintePesca(color, c));
}


static void CuboGiradoPesca(Vector3 c, Vector3 t, float giroGrados, Color color)
{
    Color tenido = TintePesca(color, c);
    EmpujarPesca();
    TrasladarPesca(c.x, c.y, c.z);
    RotarPesca(giroGrados, 0.0f, 1.0f, 0.0f);
    CuboCrudoPesca({ 0.0f, 0.0f, 0.0f }, t, tenido);
    SoltarPesca();
}


// Prisma (o tronco de cono) entre dos puntos, con niebla por vertice.
static void PrismaPesca(
    Vector3 a,
    Vector3 b,
    float r0,
    float r1,
    int lados,
    Color color,
    float emisivo = 0.0f,
    bool tapa = true
)
{
    Vector3 eje = NormalizarPesca(RestaPesca(b, a));
    Vector3 apoyo = std::fabs(eje.y) < 0.9f ? Vector3{ 0.0f, 1.0f, 0.0f } : Vector3{ 1.0f, 0.0f, 0.0f };
    Vector3 u = NormalizarPesca(CruzPesca(eje, apoyo));
    Vector3 v = CruzPesca(eje, u);
    float paso = 6.2832f / (float)lados;
    float luzTapa = LuzNormalPesca(eje);
    Color colorTapa = TintePesca(TonoPesca(color, luzTapa + (1.0f - luzTapa) * emisivo), b, emisivo);

    BeginPesca(RL_QUADS);

    for (int k = 0; k < lados; k++)
    {
        float a0 = paso * (float)k;
        float a1 = a0 + paso;
        float am = a0 + paso * 0.5f;
        Vector3 d0 = SumaPesca(EscalaPesca(u, std::cos(a0)), EscalaPesca(v, std::sin(a0)));
        Vector3 d1 = SumaPesca(EscalaPesca(u, std::cos(a1)), EscalaPesca(v, std::sin(a1)));
        Vector3 dm = SumaPesca(EscalaPesca(u, std::cos(am)), EscalaPesca(v, std::sin(am)));

        float f = LuzNormalPesca(dm);
        f = f + (1.0f - f) * emisivo;
        if (k & 1) f *= 0.94f;
        Color cara = TonoPesca(color, f);

        Vector3 p0 = SumaPesca(a, EscalaPesca(d0, r0));
        Vector3 p1 = SumaPesca(a, EscalaPesca(d1, r0));
        Vector3 p2 = SumaPesca(b, EscalaPesca(d1, r1));
        Vector3 p3 = SumaPesca(b, EscalaPesca(d0, r1));

        VerticePesca(p0, TintePesca(cara, p0, emisivo));
        VerticePesca(p1, TintePesca(cara, p1, emisivo));
        VerticePesca(p2, TintePesca(cara, p2, emisivo));
        VerticePesca(p3, TintePesca(cara, p3, emisivo));

        if (tapa && r1 > 0.001f)
        {
            QuadPesca(b, p2, p3, b, colorTapa);
        }
    }

    EndPesca();
}


// Roca, mata o coral redondo: esfera de pocos lados con radios irregulares.
static void RocaPesca(Vector3 c, Vector3 r, int semilla, Color color, float irregular = 0.14f, float emisivo = 0.0f)
{
    const int ANILLOS = 5;
    const int LADOS = 8;

    BeginPesca(RL_QUADS);

    for (int a = 0; a < ANILLOS; a++)
    {
        for (int k = 0; k < LADOS; k++)
        {
            int aa[4] = { a, a, a + 1, a + 1 };
            int kk[4] = { k, k + 1, k + 1, k };

            for (int q = 0; q < 4; q++)
            {
                float th = -1.5708f + 3.1416f * (float)aa[q] / (float)ANILLOS;
                float ph = 6.2832f * (float)kk[q] / (float)LADOS + (float)semilla;
                float f = 1.0f;

                if (aa[q] != 0 && aa[q] != ANILLOS)
                {
                    f = 1.0f - irregular + 2.0f * irregular * RuidoPesca(aa[q] * 31 + (kk[q] % LADOS), semilla);
                }

                Vector3 n = { std::cos(th) * std::cos(ph), std::sin(th), std::cos(th) * std::sin(ph) };
                Vector3 p = { c.x + n.x * r.x * f, c.y + n.y * r.y * f, c.z + n.z * r.z * f };
                float luz = LuzNormalPesca(n);
                luz = luz + (1.0f - luz) * emisivo;
                Color cv = TonoPesca(color, luz * (0.93f + 0.12f * RuidoPesca(aa[q] * 7 + kk[q], semilla + 5)));
                VerticePesca(p, TintePesca(cv, p, emisivo));
            }
        }
    }

    EndPesca();
}


// Corona horizontal (ondas, bordes, anillos de luz).
static void AnilloPesca(Vector3 c, float r0, float r1, Color color, int lados = 24)
{
    BeginPesca(RL_QUADS);

    for (int k = 0; k < lados; k++)
    {
        float a0 = 6.2832f * (float)k / (float)lados;
        float a1 = 6.2832f * (float)(k + 1) / (float)lados;

        QuadPesca(
            { c.x + std::cos(a0) * r1, c.y, c.z + std::sin(a0) * r1 },
            { c.x + std::cos(a0) * r0, c.y, c.z + std::sin(a0) * r0 },
            { c.x + std::cos(a1) * r0, c.y, c.z + std::sin(a1) * r0 },
            { c.x + std::cos(a1) * r1, c.y, c.z + std::sin(a1) * r1 },
            color
        );
    }

    EndPesca();
}


// Disco con borde que se apaga (u y v definen el plano).
static void ResplandorPesca(Vector3 centro, Vector3 u, Vector3 v, float radio, Color color, float estirado = 1.0f)
{
    const int LADOS = 18;
    BeginPesca(RL_TRIANGLES);

    for (int k = 0; k < LADOS; k++)
    {
        float a0 = 6.2832f * (float)k / (float)LADOS;
        float a1 = 6.2832f * (float)(k + 1) / (float)LADOS;
        float c0 = std::cos(a0) * radio * estirado, s0 = std::sin(a0) * radio;
        float c1 = std::cos(a1) * radio * estirado, s1 = std::sin(a1) * radio;
        Color borde = { color.r, color.g, color.b, 0 };

        VerticePesca(centro, color);
        VerticePesca({ centro.x + u.x * c0 + v.x * s0, centro.y + u.y * c0 + v.y * s0, centro.z + u.z * c0 + v.z * s0 }, borde);
        VerticePesca({ centro.x + u.x * c1 + v.x * s1, centro.y + u.y * c1 + v.y * s1, centro.z + u.z * c1 + v.z * s1 }, borde);
    }

    EndPesca();
}


static void ResplandorSueloPesca(Vector3 c, float radio, Color color, float estirado = 1.0f)
{
    ResplandorPesca(c, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, radio, color, estirado);
}


static void ResplandorFrentePesca(Vector3 c, float radio, Color color, float estirado = 1.0f)
{
    ResplandorPesca(c, derechaVistaPesca, arribaVistaPesca, radio, color, estirado);
}


static void HaloPesca(Vector3 p, float radio, Color exterior, Color interior)
{
    ResplandorFrentePesca(p, radio, exterior);
    ResplandorFrentePesca(p, radio * 0.42f, interior);
}


// Cono de luz (aditivo): del foco hacia el charco, se apaga en la base.
static void ConoLuzPesca(Vector3 foco, Vector3 baseCentro, float radio, Color color)
{
    const int LADOS = 16;
    Vector3 eje = NormalizarPesca(RestaPesca(baseCentro, foco));
    Vector3 apoyo = std::fabs(eje.y) < 0.9f ? Vector3{ 0.0f, 1.0f, 0.0f } : Vector3{ 1.0f, 0.0f, 0.0f };
    Vector3 u = NormalizarPesca(CruzPesca(eje, apoyo));
    Vector3 v = CruzPesca(eje, u);

    BeginPesca(RL_TRIANGLES);
    for (int k = 0; k < LADOS; k++)
    {
        float a0 = 6.2832f * (float)k / (float)LADOS;
        float a1 = 6.2832f * (float)(k + 1) / (float)LADOS;
        Vector3 p0 = SumaPesca(baseCentro, SumaPesca(EscalaPesca(u, std::cos(a0) * radio), EscalaPesca(v, std::sin(a0) * radio)));
        Vector3 p1 = SumaPesca(baseCentro, SumaPesca(EscalaPesca(u, std::cos(a1) * radio), EscalaPesca(v, std::sin(a1) * radio)));

        rlColor4ub(color.r, color.g, color.b, color.a);
        rlVertex3f(foco.x, foco.y, foco.z);
        rlColor4ub(color.r, color.g, color.b, 0);
        rlVertex3f(p0.x, p0.y, p0.z);
        rlColor4ub(color.r, color.g, color.b, 0);
        rlVertex3f(p1.x, p1.y, p1.z);
    }
    EndPesca();
}


// Charco de luz: el mismo perfil que tres discos concentricos tenues
// (radios 0.55, 1 y 1.45; alfas 0.11, 0.08 y 0.05) en una sola pasada,
// para no pintar tres veces cada pixel.
static void CharcoLuzPesca(Vector3 c, float radio, Color color, float intensidad)
{
    const int LADOS = 18;
    const float radios[3] = { 0.55f, 1.0f, 1.45f };
    const float alfas[3] = { 0.11f, 0.08f, 0.05f };
    float paradas[4] = { 0.0f, 0.55f, 1.0f, 1.45f };
    Color tonos[4];

    for (int s = 0; s < 4; s++)
    {
        float a = 0.0f;
        for (int r = 0; r < 3; r++) a += alfas[r] * std::fmax(0.0f, 1.0f - paradas[s] / radios[r]);
        tonos[s] = AlfaPesca(color, a * intensidad);
        paradas[s] *= radio;
    }

    BeginPesca(RL_TRIANGLES);

    for (int k = 0; k < LADOS; k++)
    {
        float a0 = 6.2832f * (float)k / (float)LADOS;
        float a1 = 6.2832f * (float)(k + 1) / (float)LADOS;
        float c0 = std::cos(a0), s0 = std::sin(a0);
        float c1 = std::cos(a1), s1 = std::sin(a1);

        for (int s = 0; s < 3; s++)
        {
            float r0 = paradas[s], r1 = paradas[s + 1];
            Vector3 p00 = { c.x + c0 * r0, c.y, c.z + s0 * r0 };
            Vector3 p01 = { c.x + c1 * r0, c.y, c.z + s1 * r0 };
            Vector3 p10 = { c.x + c0 * r1, c.y, c.z + s0 * r1 };
            Vector3 p11 = { c.x + c1 * r1, c.y, c.z + s1 * r1 };

            if (s > 0)
            {
                VerticePesca(p00, tonos[s]);
                VerticePesca(p01, tonos[s]);
                VerticePesca(p11, tonos[s + 1]);
            }

            VerticePesca(p00, tonos[s]);
            VerticePesca(p11, tonos[s + 1]);
            VerticePesca(p10, tonos[s + 1]);
        }
    }

    EndPesca();
}


// Cartel plano de cara a la camara.
static void CartelPesca(Vector3 c, float ancho, float alto, Color color)
{
    Vector3 dx = EscalaPesca(derechaVistaPesca, ancho * 0.5f);
    Vector3 dy = EscalaPesca(arribaVistaPesca, alto * 0.5f);

    BeginPesca(RL_QUADS);
    QuadPesca(
        RestaPesca(RestaPesca(c, dx), dy),
        RestaPesca(SumaPesca(c, dx), dy),
        SumaPesca(SumaPesca(c, dx), dy),
        SumaPesca(RestaPesca(c, dx), dy),
        color
    );
    EndPesca();
}


// Cinta (linea gruesa) entre dos puntos, de cara a la camara.
static void CintaPesca(Vector3 a, Vector3 b, float ancho, Color ca, Color cb)
{
    Vector3 dir = RestaPesca(b, a);
    Vector3 haciaOjo = RestaPesca(ojoPesca, LerpPesca(a, b, 0.5f));
    Vector3 lado = EscalaPesca(NormalizarPesca(CruzPesca(dir, haciaOjo)), ancho * 0.5f);

    BeginPesca(RL_QUADS);
    VerticePesca(RestaPesca(a, lado), ca);
    VerticePesca(SumaPesca(a, lado), ca);
    VerticePesca(SumaPesca(b, lado), cb);
    VerticePesca(RestaPesca(b, lado), cb);
    EndPesca();
}


//--------------------------------------------------
// Geometria visual de la isla (coincide con la logica)
//--------------------------------------------------

// Orilla de la laguna (radio del agua) segun el angulo.
static float OrillaLagunaPesca(float a)
{
    return 12.6f + 0.45f * std::sin(3.0f * a + 1.0f) + 0.3f * std::sin(7.0f * a + 2.1f);
}


// Costa exterior de la isla; al norte se ensancha hacia el volcan.
static float CostaIslaPesca(float a)
{
    float norte = std::fmax(0.0f, -std::sin(a));
    return 22.6f + 1.8f * std::sin(2.0f * a + 0.5f) + 1.1f * std::sin(5.0f * a + 1.3f) + 9.0f * norte * norte;
}


static float TramoPesca(float r, float r0, float r1, float y0, float y1)
{
    float u = SuavePesca((r - r0) / (r1 - r0));
    return y0 + (y1 - y0) * u;
}


// Altura del terreno (fondo de la laguna, playa, meseta y costa).
static float AlturaTerrenoPesca(float r, float a)
{
    float rl = OrillaLagunaPesca(a);
    float rc = CostaIslaPesca(a);
    float x = std::cos(a) * r;
    float z = std::sin(a) * r;
    float meseta = 0.92f + 0.22f * std::sin(x * 0.7f + 1.3f) * std::sin(z * 0.6f + 0.4f) + 0.12f * std::sin(x * 1.9f - z * 1.4f);

    if (r <= 6.6f) return -1.8f + 0.05f * std::sin(x * 1.3f) * std::sin(z * 1.1f);
    if (r <= 7.7f) return TramoPesca(r, 6.6f, 7.7f, -1.8f, -1.06f);
    if (r <= 8.9f) return -1.06f + 0.1f * std::sin(a * 9.0f) + 0.06f * std::sin(a * 23.0f);
    if (r <= rl - 0.7f) return TramoPesca(r, 8.9f, rl - 0.7f, -1.0f, -0.4f);
    if (r <= rl + 0.5f) return TramoPesca(r, rl - 0.7f, rl + 0.5f, -0.4f, 0.06f);
    if (r <= rl + 3.5f) return TramoPesca(r, rl + 0.5f, rl + 3.5f, 0.06f, 0.5f);
    if (r <= rc - 3.0f) return TramoPesca(r, rl + 3.5f, rl + 5.5f, 0.5f, meseta);
    if (r <= rc) return TramoPesca(r, rc - 3.0f, rc, meseta, -0.06f);
    return TramoPesca(r, rc, rc + 6.0f, -0.06f, -1.7f);
}


static float AlturaEnPesca(float x, float z)
{
    return AlturaTerrenoPesca(std::sqrt(x * x + z * z), std::atan2(z, x));
}


// Marco local de cada muelle: u hacia afuera de la laguna, v lateral.
static Vector3 NormalMuellePesca(int muelle)
{
    Vector3 p = PosicionMuellePesca(muelle);
    float l = std::sqrt(p.x * p.x + p.z * p.z);
    return { p.x / l, 0.0f, p.z / l };
}


static Vector3 PuntoMuellePesca(int muelle, float u, float v, float y)
{
    Vector3 p = PosicionMuellePesca(muelle);
    Vector3 n = NormalMuellePesca(muelle);
    return { p.x + n.x * u - n.z * v, y, p.z + n.z * u + n.x * v };
}


static float GiroMuellePesca(int muelle)
{
    const float giros[] = { 0.0f, 180.0f, -90.0f, 90.0f };
    return giros[muelle % 4];
}


static float LargoPasarelaPesca(int muelle)
{
    Vector3 n = NormalMuellePesca(muelle);
    float a = std::atan2(n.z, n.x);
    return OrillaLagunaPesca(a) + 1.5f - DISTANCIA_MUELLE_PESCA;
}


// Balde donde cae lo pescado.
static Vector3 CestaPesca(int muelle)
{
    return PuntoMuellePesca(muelle, 0.55f, -0.9f, ALTURA_MUELLE_PESCA + 0.32f);
}


// Posicion del corcho: la misma que usa la logica para cada estado.
static Vector3 PosicionCorchoPesca(const MinijuegoPescaIsla& m, int i)
{
    const EstadoJugadorPesca& estado = m.estados[i];
    Vector3 punta = PuntaCanaPesca(m.muelleDe[i]);
    Vector3 anzuelo = { estado.anzueloX, 0.12f, estado.anzueloZ };

    if (estado.estado == PESCA_LANZANDO)
    {
        float u = LimitarPesca(estado.tiempoEstado / DURACION_LANZAMIENTO_PESCA, 0.0f, 1.0f);
        anzuelo =
        {
            punta.x + (estado.anzueloX - punta.x) * u,
            punta.y * (1.0f - u) + 0.12f * u + std::sin(PI * u) * 1.5f,
            punta.z + (estado.anzueloZ - punta.z) * u
        };
    }
    else if (estado.estado == PESCA_PICADA)
    {
        anzuelo.y = -0.08f;
    }
    else if (estado.estado == PESCA_TENSION)
    {
        float acercamiento = estado.progreso * 0.5f;
        anzuelo.x += (punta.x - anzuelo.x) * acercamiento;
        anzuelo.z += (punta.z - anzuelo.z) * acercamiento;
        anzuelo.y = 0.05f;
    }
    else
    {
        anzuelo.y = 0.12f + 0.05f * std::sin(m.tiempoAnimacion * 3.0f + (float)i);
    }

    return anzuelo;
}


//--------------------------------------------------
// Carga de las piezas jugables
//--------------------------------------------------

static_assert(MODELO_PESCA_CANA + 1 == MODELO_PESCA_CORCHO,
    "Cana y corcho deben ir seguidos en el paquete Pesca");
static_assert(MODELO_PESCA_PEQUENO + 4 == MODELO_PESCA_CURSOR,
    "Peces, bota y mira deben ir seguidos en el paquete Pesca");

void CargarPiezasJugablesPescaIsla()
{
    // Mismas primitives por GLB que exige CargarPaquetePescaIslenaRetro3D.
    static const int mallasEsperadas[CANTIDAD_MODELOS_PESCA_ISLENA_3D] =
        { 2,5,2,4,4,3,5,3,2,3,1,3,3,4,4,4,3,2 };
    const int tramos[2][2] =
    {
        { MODELO_PESCA_CANA, MODELO_PESCA_CORCHO },
        { MODELO_PESCA_PEQUENO, MODELO_PESCA_CURSOR }
    };
    PaqueteModelosEscenarioRetro3D paquete = ObtenerPaquetePescaIslenaRetro3D();

    for (int t = 0; t < 2; t++)
    {
        PaqueteModelosEscenarioRetro3D tramo = paquete;
        tramo.recursos += tramos[t][0];
        tramo.definiciones += tramos[t][0];
        tramo.cantidad = tramos[t][1] - tramos[t][0] + 1;
        CargarPaqueteModelosEscenarioRetro3D(tramo);

        for (int i = tramos[t][0]; i <= tramos[t][1]; i++)
        {
            RecursoModeloEscenarioRetro3D& r = paquete.recursos[i];
            if (!r.cargado || r.modelo.meshCount == mallasEsperadas[i]) continue;
            UnloadModel(r.modelo);
            r.modelo = {};
            r.cargado = false;
            TraceLog(LOG_WARNING, "Primitives inesperadas en Pesca; se usan primitivas: %s", r.ruta);
        }
    }

    // Colas de los tres peces (las animaciones 0 y 1 son de la gaviota y
    // el humo, que ya no se usan).
    for (int i = 2; i < 5; i++)
    {
        AnimacionMallaPescaRetro3D& a = ObtenerModelosEscenariosRetro3D().animacionesPesca[i];
        RecursoModeloEscenarioRetro3D& r = paquete.recursos[MODELO_PESCA_PEQUENO + i - 2];
        if (a.intentada || !r.cargado) continue;
        a.intentada = true;
        Mesh& malla = r.modelo.meshes[1];
        bool valida = malla.normals && malla.colors && malla.vboId && malla.vboId[0] && malla.vboId[2];
        unsigned int bytes = (unsigned int)malla.vertexCount * 3 * sizeof(float);

        if (valida)
        {
            a.vertices = (float*)MemAlloc(bytes);
            a.normales = (float*)MemAlloc(bytes);
            valida = a.vertices && a.normales;
        }

        if (valida)
        {
            std::memcpy(a.vertices, malla.vertices, bytes);
            std::memcpy(a.normales, malla.normals, bytes);
        }
        else
        {
            MemFree(a.vertices);
            MemFree(a.normales);
            a = {};
            a.intentada = true;
            UnloadModel(r.modelo);
            r.modelo = {};
            r.cargado = false;
            TraceLog(LOG_WARNING, "No se pudo preparar la animacion de Pesca; se usan primitivas: %s", r.ruta);
        }
    }
}


//--------------------------------------------------
// Estado visual: particulas y eventos
//--------------------------------------------------

struct InstantaneaPesca
{
    FasePesca fase = FASE_PESCA_PREPARACION;
    EstadoPescaJugador estado[MAX_PARTICIPANTES] = {};
    int puntos[MAX_PARTICIPANTES] = {};
    int pez[MAX_PARTICIPANTES] = {};
    Vector3 corcho[MAX_PARTICIPANTES] = {};
    EstadoPezPesca estadoPez[MAX_PECES_PESCA] = {};
};


static void TomarInstantaneaPesca(const MinijuegoPescaIsla& m, InstantaneaPesca& s)
{
    s.fase = m.fase;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        s.estado[i] = m.estados[i].estado;
        s.puntos[i] = m.estados[i].puntos;
        s.pez[i] = m.estados[i].pez;
        s.corcho[i] = m.participa[i] ? PosicionCorchoPesca(m, i) : Vector3{ 0.0f, 0.0f, 0.0f };
    }

    for (int f = 0; f < MAX_PECES_PESCA; f++)
    {
        s.estadoPez[f] = m.peces[f].estado;
    }
}


static float AzarVisualPesca(MinijuegoPescaIsla& m, float minimo, float maximo)
{
    m.semillaVisual = m.semillaVisual * 1664525u + 1013904223u;
    float u = (float)((m.semillaVisual >> 8) & 0xFFFFu) / 65535.0f;
    return minimo + (maximo - minimo) * u;
}


static int CantidadParticulasPesca(int base)
{
    int n = (int)((float)base * FactorCalidadGrafica(CalidadParticulas()) + 0.5f);
    return n < 1 ? 1 : n;
}


static void EmitirParticulaPesca(
    MinijuegoPescaIsla& m,
    int tipo,
    Vector3 posicion,
    Vector3 velocidad,
    float vida,
    float tamano,
    Color color
)
{
    int elegida = 0;
    float menorVida = 1e9f;

    for (int i = 0; i < MAX_PARTICULAS_PESCA; i++)
    {
        if (!m.particulas[i].activa)
        {
            elegida = i;
            break;
        }

        if (m.particulas[i].vida < menorVida)
        {
            menorVida = m.particulas[i].vida;
            elegida = i;
        }
    }

    ParticulaPesca& p = m.particulas[elegida];
    p.activa = true;
    p.tipo = tipo;
    p.posicion = posicion;
    p.velocidad = velocidad;
    p.vida = vida;
    p.vidaMaxima = vida;
    p.tamano = tamano;
    p.giro = AzarVisualPesca(m, 0.0f, 360.0f);
    p.color = color;
}


// Salpicadura sobre el agua: gotas que saltan y ondas que se abren.
static void SalpicarPesca(MinijuegoPescaIsla& m, Vector3 c, int gotas, int ondas, float fuerza, Color color)
{
    int n = CantidadParticulasPesca(gotas);

    for (int k = 0; k < n; k++)
    {
        float a = AzarVisualPesca(m, 0.0f, 6.2832f);
        float r = AzarVisualPesca(m, 0.4f, 1.4f) * fuerza;
        EmitirParticulaPesca(m, PARTICULA_PESCA_GOTA,
            { c.x + std::cos(a) * 0.15f, 0.05f, c.z + std::sin(a) * 0.15f },
            { std::cos(a) * r, AzarVisualPesca(m, 2.0f, 3.6f) * fuerza, std::sin(a) * r },
            AzarVisualPesca(m, 0.5f, 0.9f), AzarVisualPesca(m, 0.05f, 0.09f), color);
    }

    for (int k = 0; k < ondas; k++)
    {
        EmitirParticulaPesca(m, PARTICULA_PESCA_ONDA, { c.x, 0.035f, c.z },
            { 0.0f, 0.0f, 0.0f }, 0.9f + 0.35f * (float)k, 0.18f + 0.2f * (float)k * fuerza, WHITE);
    }
}


static void ChispasPesca(MinijuegoPescaIsla& m, Vector3 c, int cantidad, Color color, Color secundario)
{
    int n = CantidadParticulasPesca(cantidad);

    for (int k = 0; k < n; k++)
    {
        float a = AzarVisualPesca(m, 0.0f, 6.2832f);
        float r = AzarVisualPesca(m, 0.6f, 2.0f);
        EmitirParticulaPesca(m, PARTICULA_PESCA_CHISPA, c,
            { std::cos(a) * r, AzarVisualPesca(m, 1.8f, 3.8f), std::sin(a) * r },
            AzarVisualPesca(m, 0.5f, 0.9f), AzarVisualPesca(m, 0.06f, 0.12f), k % 3 == 0 ? secundario : color);
    }
}


static void ActualizarParticulasPesca(MinijuegoPescaIsla& m, float dt)
{
    for (int i = 0; i < MAX_PARTICULAS_PESCA; i++)
    {
        ParticulaPesca& p = m.particulas[i];
        if (!p.activa) continue;

        p.vida -= dt;
        if (p.vida <= 0.0f)
        {
            p.activa = false;
            continue;
        }

        switch (p.tipo)
        {
        case PARTICULA_PESCA_GOTA:
        case PARTICULA_PESCA_BARRO:
            p.velocidad.y -= 9.5f * dt;
            p.giro += 300.0f * dt;
            break;
        case PARTICULA_PESCA_ONDA:
            p.tamano += 1.3f * dt;
            break;
        case PARTICULA_PESCA_CHISPA:
            p.velocidad = EscalaPesca(p.velocidad, 1.0f - 1.2f * dt);
            p.velocidad.y -= 2.6f * dt;
            break;
        case PARTICULA_PESCA_CONFETI:
            p.velocidad.y -= 5.0f * dt;
            if (p.velocidad.y < -1.6f) p.velocidad.y = -1.6f;
            p.velocidad.x += std::sin(p.vida * 5.0f + p.giro) * 2.0f * dt;
            p.giro += 360.0f * dt;
            break;
        default:
            break;
        }

        p.posicion = SumaPesca(p.posicion, EscalaPesca(p.velocidad, dt));

        // Las gotas que vuelven al agua se apagan.
        if ((p.tipo == PARTICULA_PESCA_GOTA || p.tipo == PARTICULA_PESCA_BARRO) && p.posicion.y < 0.0f && p.velocidad.y < 0.0f)
        {
            p.activa = false;
        }
        else if (p.tipo == PARTICULA_PESCA_CONFETI && p.posicion.y < -0.2f)
        {
            p.activa = false;
        }
    }
}


static void ActualizarVisualPesca(MinijuegoPescaIsla& m, const InstantaneaPesca& s, float dt)
{
    ActualizarParticulasPesca(m, dt);

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!m.participa[i])
        {
            continue;
        }

        Color color = m.coloresJugadores[i];
        m.destelloJugador[i] = std::fmax(0.0f, m.destelloJugador[i] - dt * 1.6f);

        if (m.avisoTipo[i] != AVISO_PESCA_NINGUNO)
        {
            m.avisoTiempo[i] += dt;

            if (m.avisoTiempo[i] > DURACION_AVISO_PESCA)
            {
                m.avisoTipo[i] = AVISO_PESCA_NINGUNO;
            }
        }

        // Pez (o bota) en el aire hacia el balde del muelle.
        if (m.vueloTiempo[i] >= 0.0f)
        {
            float antes = m.vueloTiempo[i];
            m.vueloTiempo[i] += dt;

            if (antes < DURACION_VUELO_PESCA && m.vueloTiempo[i] >= DURACION_VUELO_PESCA)
            {
                Vector3 cesta = CestaPesca(m.muelleDe[i]);

                if (m.vueloTipo[i] == PEZ_PESCA_BOTA)
                {
                    for (int k = 0; k < CantidadParticulasPesca(8); k++)
                    {
                        float a = AzarVisualPesca(m, 0.0f, 6.2832f);
                        EmitirParticulaPesca(m, PARTICULA_PESCA_BARRO, cesta,
                            { std::cos(a) * 1.2f, AzarVisualPesca(m, 1.5f, 2.8f), std::sin(a) * 1.2f },
                            0.6f, AzarVisualPesca(m, 0.06f, 0.1f), Color{ 96, 84, 60, 255 });
                    }
                }
                else
                {
                    bool dorado = m.vueloTipo[i] == PEZ_PESCA_RARO;
                    ChispasPesca(m, cesta, dorado ? 22 : 12, dorado ? COLOR_DORADO_PESCA : color, WHITE);
                    m.destelloJugador[i] = 1.0f;
                }
            }

            if (m.vueloTiempo[i] > DURACION_VUELO_PESCA + 0.05f)
            {
                m.vueloTiempo[i] = -1.0f;
            }
        }

        EstadoPescaJugador a = s.estado[i];
        EstadoPescaJugador b = m.estados[i].estado;

        if (a == PESCA_LANZANDO && b == PESCA_ESPERANDO)
        {
            SalpicarPesca(m, PosicionCorchoPesca(m, i), 5, 1, 0.6f, Color{ 220, 250, 250, 255 });
        }
        else if (a == PESCA_PICADA && b == PESCA_TENSION)
        {
            SalpicarPesca(m, s.corcho[i], 12, 2, 1.0f, Color{ 230, 252, 252, 255 });
        }
        else if (a == PESCA_PICADA && b == PESCA_LIBRE)
        {
            int f = s.pez[i];
            bool bota = f >= 0 && f < MAX_PECES_PESCA &&
                m.peces[f].tipo == PEZ_PESCA_BOTA &&
                s.estadoPez[f] != ESTADO_PEZ_AUSENTE &&
                m.peces[f].estado == ESTADO_PEZ_AUSENTE;

            m.avisoTiempo[i] = 0.0f;
            m.avisoPosicion[i] = s.corcho[i];

            if (bota)
            {
                m.avisoTipo[i] = AVISO_PESCA_BOTA;
                m.vueloTiempo[i] = 0.0f;
                m.vueloTipo[i] = PEZ_PESCA_BOTA;
                m.vueloDesde[i] = { s.corcho[i].x, 0.0f, s.corcho[i].z };
                SalpicarPesca(m, s.corcho[i], 8, 1, 0.8f, Color{ 170, 180, 150, 255 });
            }
            else
            {
                m.avisoTipo[i] = AVISO_PESCA_ESCAPO;
                SalpicarPesca(m, s.corcho[i], 3, 1, 0.5f, Color{ 220, 250, 250, 255 });
            }
        }
        else if (a == PESCA_TENSION && b == PESCA_LIBRE)
        {
            int f = s.pez[i];

            if (m.estados[i].puntos > s.puntos[i] && f >= 0 && f < MAX_PECES_PESCA)
            {
                m.vueloTiempo[i] = 0.0f;
                m.vueloTipo[i] = m.peces[f].tipo;
                m.vueloDesde[i] = { s.corcho[i].x, 0.0f, s.corcho[i].z };
                SalpicarPesca(m, s.corcho[i], 14, 2, 1.1f, Color{ 230, 252, 252, 255 });

                if (m.peces[f].tipo == PEZ_PESCA_RARO)
                {
                    m.anuncioDorado = 0.0f;
                    m.jugadorDorado = i;
                }
            }
            else
            {
                m.avisoTipo[i] = AVISO_PESCA_CORTE;
                m.avisoTiempo[i] = 0.0f;
                m.avisoPosicion[i] = s.corcho[i];
                SalpicarPesca(m, s.corcho[i], 6, 1, 0.7f, Color{ 190, 200, 205, 255 });
            }
        }

        // El pez enganchado chapotea mientras dura el tira y afloja.
        if (b == PESCA_TENSION && m.estados[i].pez >= 0)
        {
            m.chapoteo[i] -= dt;

            if (m.chapoteo[i] <= 0.0f)
            {
                const PezPesca& pez = m.peces[m.estados[i].pez];
                m.chapoteo[i] = AzarVisualPesca(m, 0.12f, 0.22f);
                SalpicarPesca(m, { pez.x, 0.0f, pez.z }, 2, 1, 0.55f, Color{ 220, 250, 250, 255 });
            }
        }
    }

    m.anuncioDorado += dt;

    // Final: confeti sobre los muelles de quien gano.
    if (m.fase == FASE_PESCA_TERMINADO && m.resultado.estado == RESULTADO_MINIJUEGO_FINALIZADO)
    {
        m.tiempoFinVisual += dt;

        if (m.tiempoFinVisual < 3.2f)
        {
            const Color tonos[5] =
            {
                COLOR_DORADO_PESCA, Color{ 255, 255, 255, 255 }, Color{ 255, 130, 170, 255 },
                Color{ 90, 220, 220, 255 }, Color{ 255, 160, 70, 255 }
            };

            for (int i = 0; i < MAX_PARTICIPANTES; i++)
            {
                if (!m.participa[i] || m.resultado.participantes[i].posicionFinal != 1)
                {
                    continue;
                }

                float cantidad = dt * 34.0f * FactorCalidadGrafica(CalidadParticulas()) + AzarVisualPesca(m, 0.0f, 1.0f);

                for (int k = 0; k < (int)cantidad; k++)
                {
                    Vector3 base = PuntoMuellePesca(m.muelleDe[i], AzarVisualPesca(m, -1.4f, 1.4f), AzarVisualPesca(m, -1.4f, 1.4f), 4.6f);
                    int tono = (int)AzarVisualPesca(m, 0.0f, 4.99f);
                    EmitirParticulaPesca(m, PARTICULA_PESCA_CONFETI, base,
                        { AzarVisualPesca(m, -0.6f, 0.6f), AzarVisualPesca(m, 0.5f, 1.6f), AzarVisualPesca(m, -0.6f, 0.6f) },
                        AzarVisualPesca(m, 2.0f, 3.0f), AzarVisualPesca(m, 0.1f, 0.16f), k % 3 == 0 ? m.coloresJugadores[i] : tonos[tono]);
                }
            }
        }
    }
}


void MinijuegoPescaIsla::Actualizar(
    float deltaTime,
    JugadorPrueba jugadores[],
    int cantidadMaxima,
    Participante participantes[]
)
{
    InstantaneaPesca antes;
    TomarInstantaneaPesca(*this, antes);

    ActualizarPartida(deltaTime, jugadores, cantidadMaxima, participantes);

    if (resultado.estado != RESULTADO_MINIJUEGO_CANCELADO)
    {
        ActualizarVisualPesca(*this, antes, deltaTime);
    }
}


//--------------------------------------------------
// Cielo de atardecer: cupula, sol y nubes
//--------------------------------------------------

static Color ColorCieloPesca(Vector3 dir)
{
    float e = dir.y;
    Color c;

    if (e <= 0.0f)
    {
        c = COLOR_HORIZONTE_PESCA;
    }
    else if (e <= 0.14f)
    {
        c = MezclaPesca(COLOR_HORIZONTE_PESCA, COLOR_CIELO_MEDIO_PESCA, e / 0.14f);
    }
    else
    {
        c = MezclaPesca(COLOR_CIELO_MEDIO_PESCA, COLOR_CIELO_ALTO_PESCA, (e - 0.14f) / 0.5f);
    }

    float g = std::fmax(0.0f, dir.x * DIRECCION_SOL_PESCA.x + dir.y * DIRECCION_SOL_PESCA.y + dir.z * DIRECCION_SOL_PESCA.z);
    float g2 = g * g;
    float g8 = g2 * g2 * g2 * g2;
    return MezclaPesca(c, COLOR_SOL_PESCA, g8 * 0.75f + g2 * 0.12f);
}


static Vector3 DireccionCupulaPesca(float elevacionGrados, float azimut)
{
    float e = elevacionGrados * DEG2RAD;
    return { std::cos(e) * std::cos(azimut), std::sin(e), std::cos(e) * std::sin(azimut) };
}


static void DibujarCupulaPesca()
{
    const float elevaciones[] = { -12.0f, 0.0f, 3.0f, 7.0f, 12.0f, 20.0f, 32.0f, 48.0f, 68.0f, 90.0f };
    const int CANTIDAD = (int)(sizeof(elevaciones) / sizeof(elevaciones[0]));
    const int SECTORES = 40;
    const float RADIO = 420.0f;

    BeginPesca(RL_QUADS);

    for (int e = 0; e < CANTIDAD - 1; e++)
    {
        for (int k = 0; k < SECTORES; k++)
        {
            float a0 = 6.2832f * (float)k / (float)SECTORES;
            float a1 = 6.2832f * (float)(k + 1) / (float)SECTORES;
            Vector3 d00 = DireccionCupulaPesca(elevaciones[e], a0);
            Vector3 d01 = DireccionCupulaPesca(elevaciones[e], a1);
            Vector3 d11 = DireccionCupulaPesca(elevaciones[e + 1], a1);
            Vector3 d10 = DireccionCupulaPesca(elevaciones[e + 1], a0);

            VerticePesca(SumaPesca(ojoPesca, EscalaPesca(d00, RADIO)), ColorCieloPesca(d00));
            VerticePesca(SumaPesca(ojoPesca, EscalaPesca(d01, RADIO)), ColorCieloPesca(d01));
            VerticePesca(SumaPesca(ojoPesca, EscalaPesca(d11, RADIO)), ColorCieloPesca(d11));
            VerticePesca(SumaPesca(ojoPesca, EscalaPesca(d10, RADIO)), ColorCieloPesca(d10));
        }
    }

    EndPesca();
}


static Vector3 PosicionSolPesca()
{
    return SumaPesca(ojoPesca, EscalaPesca(DIRECCION_SOL_PESCA, 390.0f));
}


static void DibujarSolPesca()
{
    Vector3 c = PosicionSolPesca();
    const int LADOS = 28;
    const float RADIO = 15.0f;

    BeginPesca(RL_TRIANGLES);
    for (int k = 0; k < LADOS; k++)
    {
        float a0 = 6.2832f * (float)k / (float)LADOS;
        float a1 = 6.2832f * (float)(k + 1) / (float)LADOS;
        Vector3 p0 = SumaPesca(c, SumaPesca(EscalaPesca(derechaVistaPesca, std::cos(a0) * RADIO), EscalaPesca(arribaVistaPesca, std::sin(a0) * RADIO)));
        Vector3 p1 = SumaPesca(c, SumaPesca(EscalaPesca(derechaVistaPesca, std::cos(a1) * RADIO), EscalaPesca(arribaVistaPesca, std::sin(a1) * RADIO)));
        VerticePesca(c, Color{ 255, 246, 214, 255 });
        VerticePesca(p0, Color{ 255, 214, 140, 255 });
        VerticePesca(p1, Color{ 255, 214, 140, 255 });
    }
    EndPesca();
}


// Bancos de nubes bajas: manchas planas iluminadas del lado del sol.
static void DibujarNubesPesca(float t, bool efectos, int paso)
{
    const int NUBES = 16;

    for (int k = 0; k < NUBES; k += paso)
    {
        float deriva = efectos ? t * 0.0025f : 0.0f;
        float azimut = (-172.0f + 150.0f * RuidoPesca(k, 3101)) * DEG2RAD + deriva;
        float elevacion = 2.5f + 11.0f * RuidoPesca(k, 3102);
        Vector3 dir = DireccionCupulaPesca(elevacion, azimut);
        Vector3 centro = SumaPesca(ojoPesca, EscalaPesca(dir, 380.0f));
        float g = std::fmax(0.0f, dir.x * DIRECCION_SOL_PESCA.x + dir.z * DIRECCION_SOL_PESCA.z);
        Color luz = MezclaPesca(Color{ 168, 120, 168, 255 }, Color{ 255, 196, 150, 255 }, g * g);
        Color sombra = MezclaPesca(Color{ 120, 84, 132, 255 }, Color{ 214, 128, 120, 255 }, g);
        float ancho = 26.0f + 30.0f * RuidoPesca(k, 3103);

        for (int b = 0; b < 4; b++)
        {
            float dx = (RuidoPesca(k * 7 + b, 3104) - 0.5f) * ancho * 1.4f;
            float dy = (RuidoPesca(k * 7 + b, 3105) - 0.3f) * 5.0f;
            Vector3 p = SumaPesca(centro, SumaPesca(EscalaPesca(derechaVistaPesca, dx), EscalaPesca(arribaVistaPesca, dy)));
            float r = ancho * (0.32f + 0.22f * RuidoPesca(k * 7 + b, 3106));
            ResplandorPesca(SumaPesca(p, EscalaPesca(arribaVistaPesca, -r * 0.12f)), derechaVistaPesca, arribaVistaPesca, r, AlfaPesca(sombra, 0.75f), 2.6f);
            ResplandorPesca(SumaPesca(p, EscalaPesca(arribaVistaPesca, r * 0.1f)), derechaVistaPesca, arribaVistaPesca, r * 0.8f, AlfaPesca(luz, 0.8f), 2.4f);
        }
    }
}


//--------------------------------------------------
// Mar abierto, islotes y velero
//--------------------------------------------------

static Color ColorMarPesca(float x, float z, float t, bool efectos)
{
    float r = std::sqrt(x * x + z * z);
    float a = std::atan2(z, x);
    float costa = r - CostaIslaPesca(a);
    Color c = MezclaPesca(Color{ 46, 170, 178, 255 }, COLOR_MAR_PESCA, SuavePesca((costa - 0.5f) / 9.0f));

    Vector3 p = { x, Y_MAR_PESCA, z };
    Vector3 d = NormalizarPesca(RestaPesca(p, ojoPesca));
    float rasante = 1.0f - std::fabs(d.y);
    float fres = rasante * rasante * rasante;
    c = MezclaPesca(c, ColorCieloPesca({ d.x, -d.y, d.z }), 0.15f + 0.8f * fres);

    if (efectos)
    {
        float ola = 0.5f + 0.25f * std::sin(x * 0.33f + z * 0.21f + t * 0.7f) + 0.25f * std::sin(x * 0.12f - z * 0.37f - t * 0.5f);
        c = TonoPesca(c, 0.9f + 0.18f * ola);
    }

    return TintePesca(c, p);
}


static void DibujarMarPesca(float t, bool efectos)
{
    const float radios[] = { 17.0f, 22.0f, 26.0f, 30.0f, 35.0f, 42.0f, 52.0f, 66.0f, 86.0f, 115.0f, 160.0f, 240.0f, 360.0f };
    const int CANTIDAD = (int)(sizeof(radios) / sizeof(radios[0]));
    const int SECTORES = 64;

    BeginPesca(RL_QUADS);

    for (int e = 0; e < CANTIDAD - 1; e++)
    {
        for (int k = 0; k < SECTORES; k++)
        {
            float a0 = 6.2832f * (float)k / (float)SECTORES;
            float a1 = 6.2832f * (float)(k + 1) / (float)SECTORES;
            float r0 = radios[e], r1 = radios[e + 1];
            float xs[4] = { std::cos(a0) * r0, std::cos(a1) * r0, std::cos(a1) * r1, std::cos(a0) * r1 };
            float zs[4] = { std::sin(a0) * r0, std::sin(a1) * r0, std::sin(a1) * r1, std::sin(a0) * r1 };

            for (int q = 0; q < 4; q++)
            {
                VerticePesca({ xs[q], Y_MAR_PESCA, zs[q] }, ColorMarPesca(xs[q], zs[q], t, efectos));
            }
        }
    }

    EndPesca();
}


static void PalmeraSimplePesca(Vector3 base, float alto, float inclina, Color tronco, Color hoja)
{
    Vector3 copa = { base.x + inclina, base.y + alto, base.z };
    PrismaPesca(base, copa, alto * 0.05f, alto * 0.035f, 5, tronco);
    RocaPesca(copa, { alto * 0.42f, alto * 0.12f, alto * 0.42f }, (int)(base.x * 3.0f), hoja, 0.3f);
}


static void DibujarIslotesPesca(float t, bool efectos)
{
    struct Islote { float x, z, radio, alto; };
    const Islote islotes[] =
    {
        { -84.0f, -96.0f, 10.0f, 3.2f },
        { -40.0f, -134.0f, 7.0f, 2.4f },
        { 66.0f, -112.0f, 9.0f, 4.0f },
        { 112.0f, -64.0f, 12.0f, 3.0f },
        { -128.0f, -38.0f, 9.0f, 2.6f },
        { 40.0f, -170.0f, 14.0f, 6.0f }
    };

    for (int k = 0; EstaticoPesca() && k < (int)(sizeof(islotes) / sizeof(islotes[0])); k++)
    {
        const Islote& isla = islotes[k];
        Vector3 c = { isla.x, Y_MAR_PESCA, isla.z };
        AnilloPesca({ c.x, Y_MAR_PESCA + 0.05f, c.z }, isla.radio * 0.8f, isla.radio * 1.15f, TintePesca(COLOR_ARENA_PESCA, c), 18);
        RocaPesca(c, { isla.radio * 0.85f, isla.alto, isla.radio * 0.75f }, 400 + k, COLOR_SELVA_PESCA, 0.22f);

        for (int p = 0; p < 3; p++)
        {
            float a = RuidoPesca(k * 5 + p, 3201) * 6.2832f;
            float r = isla.radio * (0.2f + 0.45f * RuidoPesca(k * 5 + p, 3202));
            Vector3 base = { c.x + std::cos(a) * r, Y_MAR_PESCA + isla.alto * 0.6f, c.z + std::sin(a) * r * 0.6f };
            PalmeraSimplePesca(base, 6.0f + 3.0f * RuidoPesca(k * 5 + p, 3203), 1.4f, COLOR_MADERA_PESCA, COLOR_HOJA_PESCA);
        }
    }

    // Velero lejano que se mece.
    if (!AnimadoPesca()) return;
    float vaiven = efectos ? std::sin(t * 0.9f) * 3.0f : 0.0f;
    Vector3 v = { -36.0f, Y_MAR_PESCA, -66.0f };
    EmpujarPesca();
    TrasladarPesca(v.x, v.y, v.z);
    RotarPesca(vaiven, 1.0f, 0.0f, 0.0f);
    RotarPesca(20.0f, 0.0f, 1.0f, 0.0f);
    CuboCrudoPesca({ 0.0f, 0.35f, 0.0f }, { 4.4f, 0.7f, 1.3f }, TintePesca(Color{ 240, 236, 226, 255 }, v));
    CuboCrudoPesca({ 0.0f, 0.75f, 0.0f }, { 4.6f, 0.12f, 1.4f }, TintePesca(COLOR_MADERA_PESCA, v));
    SoltarPesca();
    PrismaPesca({ v.x, v.y + 0.7f, v.z }, { v.x, v.y + 6.2f, v.z }, 0.08f, 0.06f, 5, COLOR_MADERA_OSCURA_PESCA);
    BeginPesca(RL_TRIANGLES);
    VerticePesca({ v.x + 0.1f, v.y + 6.0f, v.z }, TintePesca(Color{ 255, 226, 200, 255 }, v));
    VerticePesca({ v.x + 0.1f, v.y + 1.1f, v.z }, TintePesca(Color{ 250, 206, 186, 255 }, v));
    VerticePesca({ v.x + 2.6f, v.y + 1.2f, v.z + 0.6f }, TintePesca(Color{ 236, 176, 166, 255 }, v));
    VerticePesca({ v.x - 0.1f, v.y + 5.4f, v.z }, TintePesca(Color{ 240, 200, 190, 255 }, v));
    VerticePesca({ v.x - 1.8f, v.y + 1.2f, v.z - 0.4f }, TintePesca(Color{ 214, 160, 160, 255 }, v));
    VerticePesca({ v.x - 0.1f, v.y + 1.1f, v.z }, TintePesca(Color{ 230, 186, 176, 255 }, v));
    EndPesca();
}


//--------------------------------------------------
// Volcan del fondo
//--------------------------------------------------

static float AlturaVolcanPesca(float d, float angulo)
{
    float arrugas = 1.0f + 0.07f * std::sin(angulo * 7.0f + 1.0f) + 0.05f * std::sin(angulo * 13.0f + 2.0f) + 0.03f * std::sin(angulo * 29.0f);

    if (d < 4.0f)
    {
        float u = d / 4.0f;
        return CENTRO_VOLCAN_PESCA.y + 22.8f + 3.2f * u * u;
    }

    float u = Limitar01Pesca((d * arrugas - 4.0f) / (RADIO_VOLCAN_PESCA - 4.0f));
    float h = ALTURA_VOLCAN_PESCA * std::pow(1.0f - u, 1.7f);
    h *= 1.0f + 0.05f * std::sin(angulo * 11.0f) * Limitar01Pesca(u * 4.0f);
    return CENTRO_VOLCAN_PESCA.y + h;
}


static Vector3 PuntoVolcanPesca(float d, float angulo)
{
    return
    {
        CENTRO_VOLCAN_PESCA.x + std::cos(angulo) * d,
        AlturaVolcanPesca(d, angulo),
        CENTRO_VOLCAN_PESCA.z + std::sin(angulo) * d
    };
}


static Color ColorVolcanPesca(Vector3 p, Vector3 normal)
{
    float h = p.y - CENTRO_VOLCAN_PESCA.y;
    Color basalto = { 74, 58, 66, 255 };
    Color ladera = { 108, 94, 66, 255 };
    Color c;

    if (h > 15.0f) c = basalto;
    else if (h > 7.0f) c = MezclaPesca(ladera, basalto, (h - 7.0f) / 8.0f);
    else if (h > 2.0f) c = MezclaPesca(COLOR_SELVA_PESCA, ladera, (h - 2.0f) / 5.0f);
    else c = MezclaPesca(Color{ 150, 128, 104, 255 }, COLOR_SELVA_PESCA, h / 2.0f);

    c = TonoPesca(c, LuzNormalPesca(normal) * (0.94f + 0.1f * RuidoPesca((int)(p.x * 2.0f) * 53 + (int)(p.z * 2.0f), 3301)));
    return TintePesca(c, p);
}


static void DibujarVolcanPesca(float t, bool efectos, int sectores)
{
    const float anillos[] = { 0.0f, 2.5f, 4.0f, 5.5f, 8.0f, 11.5f, 15.5f, 20.0f, 25.0f, 32.0f };
    const int CANTIDAD = (int)(sizeof(anillos) / sizeof(anillos[0]));

    BeginPesca(RL_QUADS);

    for (int e = 0; EstaticoPesca() && e < CANTIDAD - 1; e++)
    {
        for (int k = 0; k < sectores; k++)
        {
            float a0 = 6.2832f * (float)k / (float)sectores;
            float a1 = 6.2832f * (float)(k + 1) / (float)sectores;
            Vector3 p[4] =
            {
                PuntoVolcanPesca(anillos[e], a0), PuntoVolcanPesca(anillos[e], a1),
                PuntoVolcanPesca(anillos[e + 1], a1), PuntoVolcanPesca(anillos[e + 1], a0)
            };
            Vector3 n = NormalizarPesca(CruzPesca(RestaPesca(p[2], p[0]), RestaPesca(p[1], p[3])));
            if (n.y < 0.0f) n = EscalaPesca(n, -1.0f);

            for (int q = 0; q < 4; q++)
            {
                VerticePesca(p[q], ColorVolcanPesca(p[q], n));
            }
        }
    }

    EndPesca();

    // Lava del crater y tres coladas que bajan hacia la isla.
    if (!AnimadoPesca()) return;
    float latido = efectos ? 0.85f + 0.15f * std::sin(t * 2.1f) : 0.9f;
    Vector3 crater = { CENTRO_VOLCAN_PESCA.x, CENTRO_VOLCAN_PESCA.y + 23.2f, CENTRO_VOLCAN_PESCA.z };
    BeginPesca(RL_TRIANGLES);
    for (int k = 0; k < 16; k++)
    {
        float a0 = 6.2832f * (float)k / 16.0f;
        float a1 = 6.2832f * (float)(k + 1) / 16.0f;
        VerticePesca(crater, TonoPesca(Color{ 255, 220, 120, 255 }, latido));
        VerticePesca({ crater.x + std::cos(a1) * 3.0f, crater.y + 0.4f, crater.z + std::sin(a1) * 3.0f }, TonoPesca(COLOR_LAVA_PESCA, latido));
        VerticePesca({ crater.x + std::cos(a0) * 3.0f, crater.y + 0.4f, crater.z + std::sin(a0) * 3.0f }, TonoPesca(COLOR_LAVA_PESCA, latido));
    }
    EndPesca();

    const float coladas[3] = { 1.42f, 1.95f, 2.55f };

    for (int c = 0; c < 3; c++)
    {
        BeginPesca(RL_QUADS);
        for (int s = 0; s < 12; s++)
        {
            float d0 = 4.2f + (float)s * 1.35f;
            float d1 = d0 + 1.35f;
            float g0 = coladas[c] + 0.09f * std::sin(d0 * 0.45f + (float)c);
            float g1 = coladas[c] + 0.09f * std::sin(d1 * 0.45f + (float)c);
            float w0 = 0.55f + 0.05f * d0;
            float w1 = 0.55f + 0.05f * d1;
            Vector3 eje0 = { -std::sin(g0), 0.0f, std::cos(g0) };
            Vector3 eje1 = { -std::sin(g1), 0.0f, std::cos(g1) };
            Vector3 p0 = PuntoVolcanPesca(d0, g0);
            Vector3 p1 = PuntoVolcanPesca(d1, g1);
            p0.y += 0.2f;
            p1.y += 0.2f;
            float pulso0 = efectos ? 0.75f + 0.25f * std::sin(t * 3.0f - d0 * 0.6f + (float)c) : 0.85f;
            float pulso1 = efectos ? 0.75f + 0.25f * std::sin(t * 3.0f - d1 * 0.6f + (float)c) : 0.85f;
            float frio0 = Limitar01Pesca((d0 - 8.0f) / 10.0f);
            float frio1 = Limitar01Pesca((d1 - 8.0f) / 10.0f);
            Color c0 = TintePesca(TonoPesca(MezclaPesca(Color{ 255, 190, 90, 255 }, Color{ 190, 50, 30, 255 }, frio0), pulso0), p0, 0.8f);
            Color c1 = TintePesca(TonoPesca(MezclaPesca(Color{ 255, 190, 90, 255 }, Color{ 190, 50, 30, 255 }, frio1), pulso1), p1, 0.8f);
            VerticePesca(SumaPesca(p0, EscalaPesca(eje0, w0)), c0);
            VerticePesca(SumaPesca(p0, EscalaPesca(eje0, -w0)), c0);
            VerticePesca(SumaPesca(p1, EscalaPesca(eje1, -w1)), c1);
            VerticePesca(SumaPesca(p1, EscalaPesca(eje1, w1)), c1);
        }
        EndPesca();
    }
}


static Vector3 PuntoHumoPesca(int k, float t, bool efectos, float& radio, float& alfa)
{
    float fase = efectos ? std::fmod(t * 0.045f + (float)k / 10.0f, 1.0f) : (float)k / 10.0f;
    radio = 2.2f + 8.0f * fase;
    alfa = 0.55f * (1.0f - fase) * Limitar01Pesca(fase * 6.0f);
    return
    {
        CENTRO_VOLCAN_PESCA.x + fase * 22.0f + std::sin(fase * 5.0f + (float)k) * 1.5f,
        CENTRO_VOLCAN_PESCA.y + 26.0f + fase * 26.0f,
        CENTRO_VOLCAN_PESCA.z - fase * 6.0f
    };
}


static void DibujarHumoVolcanPesca(float t, bool efectos)
{
    for (int k = 0; k < 10; k++)
    {
        float radio = 0.0f, alfa = 0.0f;
        Vector3 p = PuntoHumoPesca(k, t, efectos, radio, alfa);
        ResplandorFrentePesca(SumaPesca(p, EscalaPesca(arribaVistaPesca, -radio * 0.12f)), radio, AlfaPesca(TintePesca(Color{ 120, 94, 120, 255 }, p), alfa));
        ResplandorFrentePesca(SumaPesca(p, EscalaPesca(derechaVistaPesca, -radio * 0.18f)), radio * 0.7f, AlfaPesca(TintePesca(Color{ 236, 170, 160, 255 }, p), alfa * 0.8f));
    }
}


//--------------------------------------------------
// Isla: fondo de la laguna, playa, meseta y costa
//--------------------------------------------------

static float CausticaPesca(float x, float z, float t)
{
    float c = std::sin(x * 1.6f + t * 1.1f) + std::sin(z * 1.8f - t * 0.9f) +
        std::sin((x - z) * 1.2f + t * 1.4f) + std::sin((x + z) * 0.85f - t * 0.7f);
    c = 1.0f - std::fabs(c * 0.25f) * 2.2f;
    c = Limitar01Pesca(c);
    return c * c;
}


static float RadioAnilloTerrenoPesca(int k, float a)
{
    const float fijos[] = { 0.0f, 1.6f, 3.2f, 4.8f, 6.0f, 6.6f, 7.15f, 7.7f, 8.3f, 8.9f };
    float rl = OrillaLagunaPesca(a);
    float rc = CostaIslaPesca(a);
    float inicioMeseta = rl + 5.5f;
    float finMeseta = std::fmax(rc - 3.0f, inicioMeseta + 0.6f);

    if (k < 10) return fijos[k];

    switch (k)
    {
        case 10: return 8.9f + (rl - 0.7f - 8.9f) / 3.0f;
        case 11: return 8.9f + (rl - 0.7f - 8.9f) * 2.0f / 3.0f;
        case 12: return rl - 0.7f;
        case 13: return rl - 0.25f;
        case 14: return rl + 0.1f;
        case 15: return rl + 0.5f;
        case 16: return rl + 1.5f;
        case 17: return rl + 2.5f;
        case 18: return rl + 3.5f;
        case 19: return rl + 4.5f;
        case 20: return inicioMeseta;
        case 21: return (inicioMeseta + finMeseta) * 0.5f;
        case 22: return finMeseta;
        case 23: return std::fmax(rc - 1.8f, finMeseta + 0.3f);
        case 24: return std::fmax(rc - 0.8f, finMeseta + 0.6f);
        case 25: return std::fmax(rc, finMeseta + 0.9f);
        case 26: return std::fmax(rc + 1.5f, finMeseta + 2.0f);
        case 27: return std::fmax(rc + 3.5f, finMeseta + 4.0f);
        default: return std::fmax(rc + 6.0f, finMeseta + 6.5f);
    }
}


static Color ColorTerrenoPesca(float r, float a, float y, float t, bool efectos)
{
    float rl = OrillaLagunaPesca(a);
    float rc = CostaIslaPesca(a);
    float x = std::cos(a) * r;
    float z = std::sin(a) * r;
    float n = 0.5f + 0.5f * std::sin(x * 2.3f + z * 1.7f) * std::sin(x * 1.1f - z * 2.9f);
    Color c;

    if (r < rl + 0.12f)
    {
        // Bajo el agua: arena clara, cascajo de coral y caustica.
        float arrecife = Limitar01Pesca(1.0f - std::fabs(r - 8.3f) / 0.9f);
        Color base = MezclaPesca(COLOR_ARENA_FONDO_PESCA, Color{ 200, 150, 136, 255 }, arrecife * 0.7f);
        float prof = Limitar01Pesca(-y / 1.9f);
        c = MezclaPesca(base, COLOR_AGUA_PESCA, 0.22f + 0.18f * prof);
        c = MezclaPesca(c, COLOR_AGUA_HONDA_PESCA, prof * 0.42f);

        if (efectos && g_grabacionPesca.activa)
        {
            // Horneado: la caustica animada la dibuja el shader por pixel.
            g_grabacionPesca.caustica = 0.34f * (1.0f - 0.35f * prof);
        }
        else if (efectos)
        {
            float caustica = CausticaPesca(x, z, t);
            c = MezclaPesca(c, Color{ 228, 255, 246, 255 }, caustica * 0.34f * (1.0f - 0.35f * prof));
        }

        c = MezclaPesca(c, COLOR_ARENA_MOJADA_PESCA, Limitar01Pesca((r - (rl - 0.6f)) / 0.7f) * 0.6f);
    }
    else if (r < rl + 0.9f)
    {
        c = MezclaPesca(COLOR_ARENA_MOJADA_PESCA, COLOR_ARENA_PESCA, (r - rl) / 0.9f);
    }
    else if (r < rl + 3.4f)
    {
        c = COLOR_ARENA_PESCA;
    }
    else if (r < rl + 4.6f)
    {
        c = MezclaPesca(COLOR_ARENA_PESCA, COLOR_PASTO_PESCA, (r - rl - 3.4f) / 1.2f);
    }
    else if (r < rc - 2.6f)
    {
        c = MezclaPesca(COLOR_PASTO_PESCA, COLOR_SELVA_PESCA, n * 0.8f);
    }
    else if (r < rc - 1.6f)
    {
        c = MezclaPesca(COLOR_PASTO_PESCA, COLOR_ARENA_PESCA, (r - (rc - 2.6f)) / 1.0f);
    }
    else if (r < rc - 0.3f)
    {
        c = COLOR_ARENA_PESCA;
    }
    else
    {
        c = MezclaPesca(COLOR_ARENA_MOJADA_PESCA, Color{ 50, 150, 160, 255 }, Limitar01Pesca((r - rc) / 4.0f));
    }

    c = TonoPesca(c, 0.9f + 0.12f * n);
    return TintePesca(c, { x, y, z });
}


static void DibujarTerrenoPesca(float t, bool efectos, int sectores)
{
    const int ANILLOS = 29;

    for (int k = 0; k < sectores; k++)
    {
        float a0 = 6.2832f * (float)k / (float)sectores;
        float a1 = 6.2832f * (float)(k + 1) / (float)sectores;

        BeginPesca(RL_QUADS);

        for (int e = 0; e < ANILLOS - 1; e++)
        {
            float rs[4] =
            {
                RadioAnilloTerrenoPesca(e, a0), RadioAnilloTerrenoPesca(e, a1),
                RadioAnilloTerrenoPesca(e + 1, a1), RadioAnilloTerrenoPesca(e + 1, a0)
            };
            float as[4] = { a0, a1, a1, a0 };

            for (int q = 0; q < 4; q++)
            {
                float y = AlturaTerrenoPesca(rs[q], as[q]);
                VerticePesca({ std::cos(as[q]) * rs[q], y, std::sin(as[q]) * rs[q] }, ColorTerrenoPesca(rs[q], as[q], y, t, efectos));
            }
        }

        EndPesca();
    }
}


//--------------------------------------------------
// Superficie de la laguna (translucida)
//--------------------------------------------------

static void DibujarSuperficieLagunaPesca(float t, bool efectos)
{
    const float fijos[] = { 0.0f, 1.2f, 2.4f, 3.6f, 4.8f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f };
    const int FIJOS = (int)(sizeof(fijos) / sizeof(fijos[0]));
    const int ANILLOS = FIJOS + 4;
    const int SECTORES = 72;

    BeginPesca(RL_QUADS);

    for (int k = 0; k < SECTORES; k++)
    {
        float a0 = 6.2832f * (float)k / (float)SECTORES;
        float a1 = 6.2832f * (float)(k + 1) / (float)SECTORES;

        for (int e = 0; e < ANILLOS - 1; e++)
        {
            float as[4] = { a0, a1, a1, a0 };
            int es[4] = { e, e, e + 1, e + 1 };

            for (int q = 0; q < 4; q++)
            {
                float a = as[q];
                float rl = OrillaLagunaPesca(a);
                int j = es[q];
                float r = j < FIJOS ? fijos[j] : (j == FIJOS ? rl - 0.9f : (j == FIJOS + 1 ? rl - 0.3f : (j == FIJOS + 2 ? rl + 0.3f : rl + 0.9f)));
                float x = std::cos(a) * r;
                float z = std::sin(a) * r;
                Vector3 p = { x, 0.0f, z };
                float prof = Limitar01Pesca(-AlturaTerrenoPesca(r, a) / 1.8f);
                Color c = MezclaPesca(Color{ 96, 222, 220, 255 }, COLOR_AGUA_HONDA_PESCA, prof * 0.6f);
                float alfa = 0.16f + 0.3f * prof;

                Vector3 d = NormalizarPesca(RestaPesca(p, ojoPesca));
                float rasante = 1.0f - std::fabs(d.y);
                float fres = rasante * rasante * rasante;
                c = MezclaPesca(c, ColorCieloPesca({ d.x, -d.y, d.z }), 0.12f + 0.9f * fres);
                alfa += 0.5f * fres;

                if (efectos)
                {
                    float onda = std::sin(x * 0.9f + t * 1.2f) * std::sin(z * 0.8f - t * 0.9f);
                    c = TonoPesca(c, 1.0f + 0.07f * onda);
                    alfa += 0.03f * onda;
                }

                c.a = CanalPesca(255.0f * Limitar01Pesca(alfa));
                VerticePesca(p, c);
            }
        }
    }

    EndPesca();
}


// Espuma que lame la orilla de la laguna y olas en la costa exterior.
static void DibujarEspumaPesca(float t, bool efectos, bool decoracion)
{
    const int SECTORES = 96;
    float vaiven = efectos ? t : 0.0f;

    BeginPesca(RL_QUADS);
    for (int k = 0; k < SECTORES; k++)
    {
        float a0 = 6.2832f * (float)k / (float)SECTORES;
        float a1 = 6.2832f * (float)(k + 1) / (float)SECTORES;
        float l0 = 0.12f * std::sin(vaiven * 1.3f + a0 * 5.0f);
        float l1 = 0.12f * std::sin(vaiven * 1.3f + a1 * 5.0f);
        float r0 = OrillaLagunaPesca(a0) - 0.25f + l0;
        float r1 = OrillaLagunaPesca(a1) - 0.25f + l1;
        Color fuerte = AlfaPesca(Color{ 250, 252, 246, 255 }, 0.55f);
        Color nada = AlfaPesca(Color{ 250, 252, 246, 255 }, 0.0f);
        VerticePesca({ std::cos(a0) * (r0 - 0.35f), 0.012f, std::sin(a0) * (r0 - 0.35f) }, nada);
        VerticePesca({ std::cos(a1) * (r1 - 0.35f), 0.012f, std::sin(a1) * (r1 - 0.35f) }, nada);
        VerticePesca({ std::cos(a1) * r1, 0.012f, std::sin(a1) * r1 }, fuerte);
        VerticePesca({ std::cos(a0) * r0, 0.012f, std::sin(a0) * r0 }, fuerte);
    }
    EndPesca();

    if (!decoracion)
    {
        return;
    }

    // Olas que avanzan hacia la costa exterior y rompen.
    for (int ola = 0; ola < 2; ola++)
    {
        float fase = efectos ? std::fmod(t * 0.22f + (float)ola * 0.5f, 1.0f) : 0.3f + 0.4f * (float)ola;
        float alfa = 0.5f * std::sin(fase * 3.1416f);

        BeginPesca(RL_QUADS);
        for (int k = 0; k < SECTORES; k++)
        {
            float a0 = 6.2832f * (float)k / (float)SECTORES;
            float a1 = 6.2832f * (float)(k + 1) / (float)SECTORES;
            float r0 = CostaIslaPesca(a0) + 3.2f - fase * 3.0f;
            float r1 = CostaIslaPesca(a1) + 3.2f - fase * 3.0f;
            Vector3 p = { std::cos(a0) * r0, Y_MAR_PESCA + 0.03f, std::sin(a0) * r0 };
            Color c = TintePesca(AlfaPesca(Color{ 250, 246, 240, 255 }, alfa), p);
            Color nada = AlfaPesca(c, 0.0f);
            VerticePesca({ std::cos(a0) * (r0 + 0.7f), Y_MAR_PESCA + 0.03f, std::sin(a0) * (r0 + 0.7f) }, nada);
            VerticePesca({ std::cos(a1) * (r1 + 0.7f), Y_MAR_PESCA + 0.03f, std::sin(a1) * (r1 + 0.7f) }, nada);
            VerticePesca({ std::cos(a1) * r1, Y_MAR_PESCA + 0.03f, std::sin(a1) * r1 }, c);
            VerticePesca({ std::cos(a0) * r0, Y_MAR_PESCA + 0.03f, std::sin(a0) * r0 }, c);
        }
        EndPesca();
    }
}


//--------------------------------------------------
// Palmeras, matas y selva
//--------------------------------------------------

struct PalmeraPescaDatos
{
    float angulo;     // grados, alrededor de la laguna
    float radio;
    float alto;
    float inclina;    // hacia afuera (+) o hacia la laguna (-)
};


static const PalmeraPescaDatos PALMERAS_PESCA[] =
{
    { 32.0f, 15.6f, 5.4f, 1.3f }, { 58.0f, 18.4f, 6.0f, 0.9f },
    { 122.0f, 18.0f, 5.8f, 1.0f }, { 148.0f, 15.4f, 5.2f, 1.4f },
    { 172.0f, 18.2f, 6.2f, 0.8f }, { 196.0f, 15.2f, 5.0f, 1.5f },
    { 218.0f, 17.6f, 6.4f, 0.7f }, { 242.0f, 15.6f, 5.6f, 1.1f },
    { 262.0f, 19.6f, 6.8f, 0.6f }, { 284.0f, 17.4f, 6.0f, 1.0f },
    { 302.0f, 15.0f, 5.2f, 1.4f }, { 322.0f, 18.0f, 6.2f, 0.9f },
    { 344.0f, 15.4f, 5.4f, 1.3f }, { 8.0f, 18.6f, 6.6f, 0.8f },
    { 250.0f, 22.0f, 7.2f, 0.4f }, { 290.0f, 22.6f, 7.0f, 0.5f }
};

static const int CANTIDAD_PALMERAS_PESCA = (int)(sizeof(PALMERAS_PESCA) / sizeof(PALMERAS_PESCA[0]));


static Vector3 BezierPesca(Vector3 a, Vector3 b, Vector3 c, float u)
{
    float v = 1.0f - u;
    return {
        a.x * v * v + 2.0f * b.x * u * v + c.x * u * u,
        a.y * v * v + 2.0f * b.y * u * v + c.y * u * u,
        a.z * v * v + 2.0f * b.z * u * v + c.z * u * u
    };
}


static void PalmeraPesca(const PalmeraPescaDatos& datos, int semilla, float t, bool efectos)
{
    float a = datos.angulo * DEG2RAD;
    Vector3 afuera = { std::cos(a), 0.0f, std::sin(a) };
    Vector3 base = { afuera.x * datos.radio, 0.0f, afuera.z * datos.radio };
    base.y = AlturaEnPesca(base.x, base.z) - 0.05f;
    float alto = datos.alto;
    float balanceo = efectos ? std::sin(t * 0.9f + (float)semilla) * 0.05f * alto : 0.0f;
    Vector3 copa = { base.x + afuera.x * datos.inclina + balanceo, base.y + alto, base.z + afuera.z * datos.inclina };
    Vector3 control = { base.x + afuera.x * datos.inclina * 0.1f, base.y + alto * 0.55f, base.z + afuera.z * datos.inclina * 0.1f };

    // Tronco con anillos.
    const int SEGMENTOS = 7;
    Vector3 anterior = base;
    float escala = alto / 5.5f;

    for (int s = 1; s <= SEGMENTOS; s++)
    {
        float u = (float)s / (float)SEGMENTOS;
        Vector3 p = BezierPesca(base, control, copa, u);
        float r0 = (0.22f - 0.08f * (float)(s - 1) / (float)SEGMENTOS) * escala;
        float r1 = (0.22f - 0.08f * u) * escala;
        Color c = (s & 1) ? Color{ 158, 118, 78, 255 } : Color{ 132, 96, 64, 255 };
        PrismaPesca(anterior, p, r0, r1, 6, c, 0.0f, s == SEGMENTOS);
        anterior = p;
    }

    // Cocos.
    for (int k = 0; k < 3; k++)
    {
        float g = (float)k * 2.1f + (float)semilla;
        RocaPesca({ copa.x + std::cos(g) * 0.22f * escala, copa.y - 0.22f * escala, copa.z + std::sin(g) * 0.22f * escala },
            { 0.16f * escala, 0.15f * escala, 0.16f * escala }, semilla * 3 + k, Color{ 112, 78, 46, 255 }, 0.08f);
    }

    // Hojas: abanicos que caen en V y aletean.
    const int HOJAS = 9;
    const Vector3 LUZ = { -0.75f, 0.0f, 0.66f };

    for (int f = 0; f < HOJAS; f++)
    {
        float g = 6.2832f * (float)f / (float)HOJAS + (float)semilla * 0.7f;
        Vector3 dir = { std::cos(g), 0.0f, std::sin(g) };
        Vector3 lado = { -dir.z, 0.0f, dir.x };
        float largo = alto * 0.52f * (0.85f + 0.3f * RuidoPesca(f, semilla + 61));
        float caida = 0.9f + 0.4f * RuidoPesca(f, semilla + 62);
        float aleteo = efectos ? std::sin(t * 2.3f + (float)f * 1.3f + (float)semilla) * 0.09f : 0.0f;
        float luz = 0.78f + 0.22f * Limitar01Pesca(dir.x * LUZ.x + dir.z * LUZ.z);
        Color claro = TonoPesca(COLOR_HOJA_PESCA, luz);
        Color oscuro = TonoPesca(Color{ 52, 118, 60, 255 }, luz);
        const int TRAMOS = 5;

        BeginPesca(RL_QUADS);
        for (int s = 0; s < TRAMOS; s++)
        {
            float u0 = (float)s / (float)TRAMOS;
            float u1 = (float)(s + 1) / (float)TRAMOS;
            Vector3 p0 = { copa.x + dir.x * largo * u0, copa.y + largo * (0.32f * u0 - (caida + aleteo) * u0 * u0), copa.z + dir.z * largo * u0 };
            Vector3 p1 = { copa.x + dir.x * largo * u1, copa.y + largo * (0.32f * u1 - (caida + aleteo) * u1 * u1), copa.z + dir.z * largo * u1 };
            float w0 = largo * 0.17f * std::sin(3.1416f * (u0 * 0.85f + 0.1f));
            float w1 = largo * 0.17f * std::sin(3.1416f * (u1 * 0.85f + 0.1f));
            Vector3 b0 = { p0.x - lado.x * w0, p0.y - w0 * 0.45f, p0.z - lado.z * w0 };
            Vector3 b1 = { p1.x - lado.x * w1, p1.y - w1 * 0.45f, p1.z - lado.z * w1 };
            Vector3 c0 = { p0.x + lado.x * w0, p0.y - w0 * 0.45f, p0.z + lado.z * w0 };
            Vector3 c1 = { p1.x + lado.x * w1, p1.y - w1 * 0.45f, p1.z + lado.z * w1 };
            VerticePesca(b0, TintePesca(oscuro, b0));
            VerticePesca(p0, TintePesca(claro, p0));
            VerticePesca(p1, TintePesca(claro, p1));
            VerticePesca(b1, TintePesca(oscuro, b1));
            VerticePesca(p0, TintePesca(claro, p0));
            VerticePesca(c0, TintePesca(TonoPesca(claro, 0.86f), c0));
            VerticePesca(c1, TintePesca(TonoPesca(claro, 0.86f), c1));
            VerticePesca(p1, TintePesca(claro, p1));
        }
        EndPesca();
    }
}


static void MataPesca(Vector3 base, float escala, int semilla, bool flores)
{
    const Color verdes[3] = { COLOR_SELVA_PESCA, Color{ 70, 130, 66, 255 }, Color{ 96, 150, 70, 255 } };

    for (int b = 0; b < 3; b++)
    {
        float g = (float)b * 2.2f + (float)semilla;
        float r = (0.55f + 0.25f * RuidoPesca(b, semilla)) * escala;
        Vector3 c = { base.x + std::cos(g) * 0.35f * escala, base.y + r * 0.55f, base.z + std::sin(g) * 0.3f * escala };
        RocaPesca(c, { r, r * 0.85f, r }, semilla * 5 + b, verdes[(semilla + b) % 3], 0.22f);
    }

    if (flores)
    {
        for (int k = 0; k < 4; k++)
        {
            float g = (float)k * 1.7f + (float)semilla * 0.3f;
            Vector3 p = { base.x + std::cos(g) * 0.55f * escala, base.y + (0.75f + 0.2f * RuidoPesca(k, semilla + 3)) * escala, base.z + std::sin(g) * 0.45f * escala };
            CuboPesca(p, { 0.12f, 0.1f, 0.12f }, k % 2 ? Color{ 255, 150, 196, 255 } : Color{ 255, 240, 230, 255 });
        }
    }
}


static void DibujarSelvaPesca(int paso, bool decoracion)
{
    // Matas sobre la meseta, entre la playa y la costa exterior.
    const int MATAS = 46;

    for (int k = 0; k < MATAS; k += paso)
    {
        float a = 6.2832f * ((float)k + RuidoPesca(k, 701) * 0.6f) / (float)MATAS;
        float rl = OrillaLagunaPesca(a);
        float rc = CostaIslaPesca(a);
        float r = rl + 4.8f + (rc - 3.2f - rl - 4.8f) * RuidoPesca(k, 702);
        Vector3 base = { std::cos(a) * r, 0.0f, std::sin(a) * r };
        base.y = AlturaEnPesca(base.x, base.z) - 0.05f;
        MataPesca(base, 0.8f + 0.6f * RuidoPesca(k, 703), k, decoracion && (k % 3 == 0));
    }

    // Selva densa al pie del volcan (borde superior de la vista).
    if (decoracion)
    {
        for (int k = 0; k < 22; k += paso)
        {
            float x = -26.0f + 52.0f * ((float)k + RuidoPesca(k, 711)) / 22.0f;
            float z = -21.0f - 7.0f * RuidoPesca(k, 712);
            float y = AlturaEnPesca(x, z);
            float r = 1.6f + 1.4f * RuidoPesca(k, 713);
            RocaPesca({ x, y + r * 0.6f, z }, { r, r * 1.05f, r * 0.9f }, 720 + k, k % 2 ? COLOR_SELVA_PESCA : Color{ 62, 118, 64, 255 }, 0.25f);
        }
    }
}


//--------------------------------------------------
// Cabanas, antorchas, redes, cajones y barriles
//--------------------------------------------------

struct CabanaPescaDatos { float x, z, giro; };

static const CabanaPescaDatos CABANAS_PESCA[] =
{
    { 8.6f, -15.6f, -18.0f },
    { -9.2f, -15.8f, 22.0f },
    { -17.6f, 4.6f, 84.0f }
};


static Vector3 VentanaCabanaPesca(const CabanaPescaDatos& c)
{
    float g = c.giro * DEG2RAD;
    // Ventana en la cara +X local, a media altura.
    Vector3 local = { 1.33f, 1.55f, 0.1f };
    float y = AlturaEnPesca(c.x, c.z);
    return { c.x + local.x * std::cos(g) + local.z * std::sin(g), y + local.y, c.z - local.x * std::sin(g) + local.z * std::cos(g) };
}


static void TrianguloSombreadoPesca(Vector3 a, Vector3 b, Vector3 c, Color color)
{
    Vector3 n = NormalizarPesca(CruzPesca(RestaPesca(b, a), RestaPesca(c, a)));
    if (n.y < 0.0f) n = EscalaPesca(n, -1.0f);
    Color tono = TonoPesca(color, LuzNormalPesca(n));
    VerticePesca(a, tono);
    VerticePesca(b, tono);
    VerticePesca(c, tono);
}


static void CabanaPesca(const CabanaPescaDatos& datos, float t, bool efectos)
{
    Vector3 base = { datos.x, AlturaEnPesca(datos.x, datos.z), datos.z };
    float niebla = CantidadNieblaPesca(base);
    auto tinte = [&](Color c) { Color n = COLOR_NIEBLA_PESCA; n.a = c.a; return MezclaPesca(c, n, niebla); };

    EmpujarPesca();
    TrasladarPesca(base.x, base.y, base.z);
    RotarPesca(datos.giro, 0.0f, 1.0f, 0.0f);

    // Ventana encendida (emisiva, titila).
    if (AnimadoPesca())
    {
        float titila = efectos ? 0.92f + 0.08f * std::sin(t * 7.0f + datos.x) : 1.0f;
        CuboCrudoPesca({ 1.31f, 1.55f, 0.1f }, { 0.04f, 0.5f, 0.6f }, TonoPesca(Color{ 255, 206, 120, 255 }, titila));
    }

    if (EstaticoPesca())
    {
    // Pilotes y piso.
    for (int k = 0; k < 4; k++)
    {
        float sx = (k & 1) ? 1.3f : -1.3f;
        float sz = (k & 2) ? 1.1f : -1.1f;
        CuboCrudoPesca({ sx, 0.25f, sz }, { 0.16f, 0.6f, 0.16f }, tinte(COLOR_MADERA_PESCA));
    }

    CuboCrudoPesca({ 0.0f, 0.6f, 0.0f }, { 3.1f, 0.18f, 2.7f }, tinte(COLOR_BAMBU_OSCURO_PESCA));
    CuboCrudoPesca({ 0.0f, 0.45f, 1.7f }, { 0.9f, 0.1f, 0.6f }, tinte(COLOR_MADERA_PESCA));

    // Paredes de cana con listones verticales.
    CuboCrudoPesca({ 0.0f, 1.45f, 0.0f }, { 2.6f, 1.55f, 2.2f }, tinte(COLOR_BAMBU_PESCA));

    for (int k = 0; k < 9; k++)
    {
        float x = -1.2f + 0.3f * (float)k;
        CuboCrudoPesca({ x, 1.45f, 1.105f }, { 0.07f, 1.55f, 0.03f }, tinte(COLOR_BAMBU_OSCURO_PESCA));
        CuboCrudoPesca({ x, 1.45f, -1.105f }, { 0.07f, 1.55f, 0.03f }, tinte(COLOR_BAMBU_OSCURO_PESCA));
    }

    CuboCrudoPesca({ 0.0f, 1.3f, 1.12f }, { 0.72f, 1.2f, 0.04f }, tinte(COLOR_MADERA_OSCURA_PESCA));

    // Marco de la ventana.
    CuboCrudoPesca({ 1.33f, 1.55f, 0.1f }, { 0.05f, 0.06f, 0.66f }, tinte(COLOR_MADERA_OSCURA_PESCA));

    // Techo de paja en dos capas.
    BeginPesca(RL_TRIANGLES);
    for (int capa = 0; capa < 2; capa++)
    {
        float ex = capa == 0 ? 1.85f : 1.55f;
        float ez = capa == 0 ? 1.65f : 1.38f;
        float ya = capa == 0 ? 2.15f : 2.55f;
        Vector3 cima = { 0.0f, 3.55f, 0.0f };
        Vector3 e[4] = { { -ex, ya, ez }, { ex, ya, ez }, { ex, ya, -ez }, { -ex, ya, -ez } };
        Color paja = tinte(capa == 0 ? COLOR_PAJA_PESCA : Color{ 222, 182, 110, 255 });

        for (int k = 0; k < 4; k++)
        {
            TrianguloSombreadoPesca(e[k], e[(k + 1) % 4], cima, paja);
        }
    }
    EndPesca();

    // Flecos del alero.
    for (int k = 0; k < 12; k++)
    {
        float x = -1.75f + 3.5f * (float)k / 11.0f;
        CuboCrudoPesca({ x, 2.05f, 1.66f }, { 0.22f, 0.2f, 0.04f }, tinte(TonoPesca(COLOR_PAJA_PESCA, 0.85f + 0.15f * (float)(k & 1))));
    }

    // Tabla de surf apoyada en la pared.
    EmpujarPesca();
    TrasladarPesca(-1.45f, 1.3f, 0.75f);
    RotarPesca(-12.0f, 0.0f, 0.0f, 1.0f);
    CuboCrudoPesca({ 0.0f, 0.0f, 0.0f }, { 0.08f, 2.0f, 0.5f }, tinte(Color{ 76, 200, 196, 255 }));
    CuboCrudoPesca({ -0.01f, 0.0f, 0.0f }, { 0.09f, 2.02f, 0.1f }, tinte(Color{ 250, 244, 226, 255 }));
    SoltarPesca();
    }

    SoltarPesca();
}


struct AntorchaPescaDatos { float angulo, radio; };

static const AntorchaPescaDatos ANTORCHAS_PESCA[] =
{
    { -78.0f, 14.6f }, { -102.0f, 14.6f },
    { 168.0f, 14.6f }, { 192.0f, 14.6f },
    { 12.0f, 14.6f }, { -12.0f, 14.6f },
    { 78.0f, 14.6f }, { 102.0f, 14.6f }
};

static const int CANTIDAD_ANTORCHAS_PESCA = (int)(sizeof(ANTORCHAS_PESCA) / sizeof(ANTORCHAS_PESCA[0]));


static Vector3 BaseAntorchaPesca(int k)
{
    float a = ANTORCHAS_PESCA[k].angulo * DEG2RAD;
    float r = OrillaLagunaPesca(a) + 1.8f;
    Vector3 p = { std::cos(a) * r, 0.0f, std::sin(a) * r };
    p.y = AlturaEnPesca(p.x, p.z) - 0.05f;
    return p;
}


static Vector3 LlamaAntorchaPesca(int k, float t, bool efectos)
{
    Vector3 b = BaseAntorchaPesca(k);
    float vaiven = efectos ? std::sin(t * 9.0f + (float)k * 2.0f) * 0.03f : 0.0f;
    return { b.x + vaiven, b.y + 2.12f, b.z };
}


static void AntorchaPesca(int k)
{
    Vector3 b = BaseAntorchaPesca(k);
    PrismaPesca(b, { b.x, b.y + 1.85f, b.z }, 0.07f, 0.06f, 6, COLOR_BAMBU_OSCURO_PESCA);

    for (int n = 0; n < 3; n++)
    {
        float y = b.y + 0.5f + 0.5f * (float)n;
        PrismaPesca({ b.x, y, b.z }, { b.x, y + 0.05f, b.z }, 0.08f, 0.08f, 6, COLOR_MADERA_OSCURA_PESCA);
    }

    PrismaPesca({ b.x, b.y + 1.78f, b.z }, { b.x, b.y + 2.02f, b.z }, 0.1f, 0.17f, 6, COLOR_PAJA_PESCA);
}


static void LlamaVisiblePesca(Vector3 c, float t, int k, bool efectos, float escala)
{
    float f = efectos ? 0.85f + 0.15f * std::sin(t * 13.0f + (float)k * 1.7f) + 0.08f * std::sin(t * 29.0f + (float)k) : 1.0f;
    CartelPesca({ c.x, c.y + 0.16f * escala * f, c.z }, 0.3f * escala, 0.5f * escala * f, Color{ 255, 120, 40, 230 });
    CartelPesca({ c.x, c.y + 0.1f * escala * f, c.z }, 0.18f * escala, 0.3f * escala * f, Color{ 255, 220, 120, 250 });
}


static void RedPesca(Vector3 a, Vector3 b, float t, bool efectos)
{
    PrismaPesca(a, { a.x, a.y + 2.2f, a.z }, 0.06f, 0.05f, 5, COLOR_MADERA_PESCA);
    PrismaPesca(b, { b.x, b.y + 2.2f, b.z }, 0.06f, 0.05f, 5, COLOR_MADERA_PESCA);
    float viento = efectos ? std::sin(t * 1.6f) * 0.08f : 0.0f;
    Color hilo = TintePesca(Color{ 110, 86, 62, 200 }, a);

    BeginPesca(RL_LINES);
    for (int f = 0; f <= 6; f++)
    {
        float y = 0.7f + 0.2f * (float)f;
        for (int c = 0; c < 10; c++)
        {
            float u0 = (float)c / 10.0f;
            float u1 = (float)(c + 1) / 10.0f;
            float s0 = std::sin(u0 * 3.1416f) * (0.35f - 0.04f * (float)f);
            float s1 = std::sin(u1 * 3.1416f) * (0.35f - 0.04f * (float)f);
            Vector3 p0 = LerpPesca(a, b, u0);
            Vector3 p1 = LerpPesca(a, b, u1);
            VerticePesca({ p0.x, a.y + y - s0, p0.z + viento * s0 * 3.0f }, hilo);
            VerticePesca({ p1.x, a.y + y - s1, p1.z + viento * s1 * 3.0f }, hilo);
        }
    }
    for (int c = 0; c <= 10; c++)
    {
        float u = (float)c / 10.0f;
        float s = std::sin(u * 3.1416f) * 0.35f;
        Vector3 p = LerpPesca(a, b, u);
        VerticePesca({ p.x, a.y + 0.7f - s, p.z + viento * s * 3.0f }, hilo);
        VerticePesca({ p.x, a.y + 1.9f - s * 0.3f, p.z + viento * s }, hilo);
    }
    EndPesca();

    for (int k = 1; k < 5; k++)
    {
        float u = (float)k / 5.0f;
        Vector3 p = LerpPesca(a, b, u);
        float s = std::sin(u * 3.1416f) * 0.35f;
        RocaPesca({ p.x, a.y + 0.62f - s, p.z + viento * s * 3.0f }, { 0.09f, 0.09f, 0.09f }, 900 + k, k % 2 ? Color{ 255, 150, 60, 255 } : Color{ 246, 240, 226, 255 }, 0.05f);
    }
}


static void BarrilPesca(Vector3 base)
{
    PrismaPesca(base, { base.x, base.y + 0.8f, base.z }, 0.3f, 0.3f, 8, COLOR_MADERA_PESCA);
    for (int k = 0; k < 2; k++)
    {
        float y = base.y + 0.18f + 0.44f * (float)k;
        PrismaPesca({ base.x, y, base.z }, { base.x, y + 0.06f, base.z }, 0.315f, 0.315f, 8, Color{ 70, 64, 66, 255 }, 0.0f, false);
    }
}


static void CajonPesca(Vector3 base, float giro, float lado)
{
    Vector3 c = { base.x, base.y + lado * 0.5f, base.z };
    CuboGiradoPesca(c, { lado, lado, lado }, giro, Color{ 176, 130, 80, 255 });
    CuboGiradoPesca({ c.x, c.y + lado * 0.2f, c.z }, { lado * 1.02f, lado * 0.08f, lado * 1.02f }, giro, COLOR_MADERA_OSCURA_PESCA);
    CuboGiradoPesca({ c.x, c.y - lado * 0.2f, c.z }, { lado * 1.02f, lado * 0.08f, lado * 1.02f }, giro, COLOR_MADERA_OSCURA_PESCA);
}


//--------------------------------------------------
// Barca en la playa y canoa con balancin
//--------------------------------------------------

static void CascoPesca(float largo, float ancho, float alto, Color exterior, Color franja, Color interior, Color borde)
{
    const int ESTACIONES = 8;

    BeginPesca(RL_QUADS);
    for (int s = 0; s < ESTACIONES; s++)
    {
        float u0 = (float)s / (float)ESTACIONES;
        float u1 = (float)(s + 1) / (float)ESTACIONES;
        float x0 = (u0 - 0.5f) * largo;
        float x1 = (u1 - 0.5f) * largo;
        float w0 = ancho * std::pow(std::sin(3.1416f * u0), 0.6f);
        float w1 = ancho * std::pow(std::sin(3.1416f * u1), 0.6f);
        float q0 = alto * (0.12f + 0.55f * std::fabs(u0 - 0.5f));
        float q1 = alto * (0.12f + 0.55f * std::fabs(u1 - 0.5f));

        for (int l = -1; l <= 1; l += 2)
        {
            float f = (float)l;
            Color e = TonoPesca(exterior, l < 0 ? 0.92f : 0.7f);
            Color fr = TonoPesca(franja, l < 0 ? 0.95f : 0.72f);
            // Casco exterior (dos bandas de color).
            QuadPesca({ x0, q0, f * w0 * 0.3f }, { x1, q1, f * w1 * 0.3f }, { x1, alto * 0.6f, f * w1 * 0.9f }, { x0, alto * 0.6f, f * w0 * 0.9f }, e);
            QuadPesca({ x0, alto * 0.6f, f * w0 * 0.9f }, { x1, alto * 0.6f, f * w1 * 0.9f }, { x1, alto, f * w1 }, { x0, alto, f * w0 }, fr);
            // Interior.
            QuadPesca({ x0, q0 + 0.06f, f * w0 * 0.25f }, { x1, q1 + 0.06f, f * w1 * 0.25f }, { x1, alto - 0.03f, f * (w1 - 0.06f) }, { x0, alto - 0.03f, f * (w0 - 0.06f) }, interior);
            // Borda.
            QuadPesca({ x0, alto, f * w0 }, { x1, alto, f * w1 }, { x1, alto + 0.05f, f * (w1 - 0.06f) }, { x0, alto + 0.05f, f * (w0 - 0.06f) }, borde);
        }
    }
    EndPesca();
}


static void BarcaPesca(Vector3 base, float giro)
{
    float niebla = CantidadNieblaPesca(base);
    auto tinte = [&](Color c) { Color n = COLOR_NIEBLA_PESCA; n.a = c.a; return MezclaPesca(c, n, niebla); };

    EmpujarPesca();
    TrasladarPesca(base.x, base.y, base.z);
    RotarPesca(giro, 0.0f, 1.0f, 0.0f);
    RotarPesca(-6.0f, 1.0f, 0.0f, 0.0f);
    CascoPesca(3.4f, 0.66f, 0.62f, tinte(Color{ 56, 126, 150, 255 }), tinte(Color{ 240, 230, 206, 255 }), tinte(Color{ 150, 108, 70, 255 }), tinte(COLOR_MADERA_PESCA));
    CuboCrudoPesca({ -0.5f, 0.42f, 0.0f }, { 0.3f, 0.06f, 1.0f }, tinte(COLOR_MADERA_PESCA));
    CuboCrudoPesca({ 0.6f, 0.42f, 0.0f }, { 0.3f, 0.06f, 0.95f }, tinte(COLOR_MADERA_PESCA));
    // Remos cruzados.
    EmpujarPesca();
    RotarPesca(18.0f, 0.0f, 1.0f, 0.0f);
    CuboCrudoPesca({ 0.0f, 0.7f, 0.0f }, { 2.6f, 0.05f, 0.08f }, tinte(Color{ 190, 150, 96, 255 }));
    CuboCrudoPesca({ 1.25f, 0.7f, 0.0f }, { 0.5f, 0.04f, 0.22f }, tinte(Color{ 190, 150, 96, 255 }));
    SoltarPesca();
    SoltarPesca();
}


static void CanoaPesca(Vector3 base, float giro, float t, bool efectos)
{
    float vaiven = efectos ? std::sin(t * 1.1f) * 3.0f : 0.0f;
    float sube = efectos ? std::sin(t * 1.4f + 1.0f) * 0.04f : 0.0f;
    float niebla = CantidadNieblaPesca(base);
    auto tinte = [&](Color c) { Color n = COLOR_NIEBLA_PESCA; n.a = c.a; return MezclaPesca(c, n, niebla); };

    EmpujarPesca();
    TrasladarPesca(base.x, base.y + sube, base.z);
    RotarPesca(giro, 0.0f, 1.0f, 0.0f);
    RotarPesca(vaiven, 1.0f, 0.0f, 0.0f);
    CascoPesca(3.6f, 0.42f, 0.4f, tinte(Color{ 150, 96, 60, 255 }), tinte(Color{ 236, 196, 110, 255 }), tinte(Color{ 110, 74, 50, 255 }), tinte(COLOR_MADERA_OSCURA_PESCA));
    // Balancin: tronco paralelo unido por dos varas.
    CuboCrudoPesca({ 0.0f, 0.1f, 1.25f }, { 2.4f, 0.16f, 0.18f }, tinte(Color{ 168, 124, 80, 255 }));
    CuboCrudoPesca({ -0.7f, 0.42f, 0.62f }, { 0.07f, 0.07f, 1.4f }, tinte(COLOR_BAMBU_OSCURO_PESCA));
    CuboCrudoPesca({ 0.7f, 0.42f, 0.62f }, { 0.07f, 0.07f, 1.4f }, tinte(COLOR_BAMBU_OSCURO_PESCA));
    CuboCrudoPesca({ 0.2f, 0.3f, 0.0f }, { 1.4f, 0.04f, 0.12f }, tinte(Color{ 200, 160, 100, 255 }));
    SoltarPesca();
}


//--------------------------------------------------
// Muelles de bambu de los jugadores
//--------------------------------------------------

static void TablaBambuPesca(Vector3 a, Vector3 b, float radio, Color color, float nudoA, float nudoB)
{
    PrismaPesca(a, b, radio, radio, 6, color);
    Color nudo = TonoPesca(color, 0.72f);
    Vector3 n0 = LerpPesca(a, b, nudoA);
    Vector3 n1 = LerpPesca(a, b, nudoA + 0.025f);
    Vector3 m0 = LerpPesca(a, b, nudoB);
    Vector3 m1 = LerpPesca(a, b, nudoB + 0.025f);
    PrismaPesca(n0, n1, radio * 1.1f, radio * 1.1f, 6, nudo, 0.0f, false);
    PrismaPesca(m0, m1, radio * 1.1f, radio * 1.1f, 6, nudo, 0.0f, false);
}


static Vector3 FarolMuellePesca(int muelle)
{
    return PuntoMuellePesca(muelle, -1.05f, -1.02f, ALTURA_MUELLE_PESCA + 1.3f);
}


static void DibujarMuellePesca(int d, bool ocupado, Color color, int peces, int mejor, float t, bool efectos)
{
    const float Y = ALTURA_MUELLE_PESCA;
    float largo = LargoPasarelaPesca(d);
    auto P = [d](float u, float v, float y) { return PuntoMuellePesca(d, u, v, y); };
    bool estatico = EstaticoPesca();
    bool animado = AnimadoPesca();
    Vector3 poste = P(-1.05f, -1.3f, Y);
    Vector3 farol = FarolMuellePesca(d);
    Vector3 mastil = P(1.15f, 1.3f, Y);
    Vector3 balde = CestaPesca(d);
    Vector3 pie = { balde.x, Y, balde.z };

    if (estatico)
    {
    // Pilotes de la plataforma y de la pasarela (bajan hasta el fondo).
    const float pilotesU[3] = { -1.12f, 0.0f, 1.12f };

    for (int k = 0; k < 3; k++)
    {
        for (int s = -1; s <= 1; s += 2)
        {
            float alto = k == 0 ? Y + 0.32f : Y - 0.05f;
            Vector3 pie = P(pilotesU[k], (float)s * 1.24f, -1.4f);
            PrismaPesca(pie, P(pilotesU[k], (float)s * 1.24f, alto), 0.11f, 0.1f, 6, COLOR_BAMBU_OSCURO_PESCA);
        }
    }

    for (float u = 2.2f; u < largo - 0.3f; u += 1.25f)
    {
        for (int s = -1; s <= 1; s += 2)
        {
            PrismaPesca(P(u, (float)s * 0.62f, -1.0f), P(u, (float)s * 0.62f, Y - 0.05f), 0.09f, 0.09f, 6, COLOR_BAMBU_OSCURO_PESCA);
        }
    }

    // Vigas bajo las tablas.
    for (int s = -1; s <= 1; s += 2)
    {
        PrismaPesca(P(-1.25f, (float)s * 1.12f, Y - 0.2f), P(1.25f, (float)s * 1.12f, Y - 0.2f), 0.08f, 0.08f, 6, COLOR_MADERA_PESCA);
        PrismaPesca(P(1.2f, (float)s * 0.52f, Y - 0.2f), P(largo, (float)s * 0.52f, Y - 0.2f), 0.07f, 0.07f, 6, COLOR_MADERA_PESCA);
    }

    // Tablas de bambu: plataforma ancha y pasarela hasta la playa.
    for (int k = 0; k < 12; k++)
    {
        float u = -1.25f + 0.104f + 0.208f * (float)k;
        Color c = MezclaPesca(COLOR_BAMBU_PESCA, COLOR_BAMBU_OSCURO_PESCA, 0.12f + 0.45f * RuidoPesca(d * 40 + k, 501));
        TablaBambuPesca(P(u, -1.38f, Y - 0.09f), P(u, 1.38f, Y - 0.09f), 0.095f, c, 0.22f + 0.1f * RuidoPesca(k, 502), 0.7f);
    }

    int n = 0;
    for (float u = 1.25f + 0.104f; u < largo; u += 0.208f, n++)
    {
        Color c = MezclaPesca(COLOR_BAMBU_PESCA, COLOR_BAMBU_OSCURO_PESCA, 0.12f + 0.45f * RuidoPesca(d * 40 + n, 503));
        TablaBambuPesca(P(u, -0.7f, Y - 0.1f), P(u, 0.7f, Y - 0.1f), 0.09f, c, 0.3f, 0.66f);
    }

    // Barandas laterales bajas (el frente queda libre para pescar).
    for (int s = -1; s <= 1; s += 2)
    {
        float v = (float)s * 1.3f;
        for (int k = 0; k < 3; k++)
        {
            float u = -0.4f + 0.8f * (float)k;
            PrismaPesca(P(u, v, Y), P(u, v, Y + 0.72f), 0.06f, 0.055f, 6, COLOR_BAMBU_OSCURO_PESCA);
        }
        PrismaPesca(P(-0.45f, v, Y + 0.68f), P(1.25f, v, Y + 0.68f), 0.055f, 0.055f, 6, COLOR_BAMBU_PESCA);
        PrismaPesca(P(1.25f, (float)s * 0.68f, Y + 0.62f), P(largo - 0.2f, (float)s * 0.68f, Y + 0.62f), 0.05f, 0.05f, 6, COLOR_BAMBU_PESCA);
    }

    // Bitas del frente con soga enrollada.
    for (int s = -1; s <= 1; s += 2)
    {
        Vector3 bita = P(-1.12f, (float)s * 1.24f, Y + 0.3f);
        PrismaPesca({ bita.x, bita.y - 0.04f, bita.z }, { bita.x, bita.y + 0.05f, bita.z }, 0.13f, 0.13f, 6, Color{ 214, 196, 150, 255 }, 0.0f, false);
    }

    // Farol en poste del frente.
    PrismaPesca(poste, { poste.x, Y + 1.75f, poste.z }, 0.05f, 0.045f, 6, COLOR_MADERA_OSCURA_PESCA);
    PrismaPesca({ poste.x, Y + 1.7f, poste.z }, { farol.x, Y + 1.7f, farol.z }, 0.03f, 0.03f, 4, COLOR_MADERA_OSCURA_PESCA, 0.0f, false);
    CuboPesca({ farol.x, farol.y + 0.2f, farol.z }, { 0.04f, 0.2f, 0.04f }, COLOR_MADERA_OSCURA_PESCA);
    CuboPesca({ farol.x, farol.y + 0.17f, farol.z }, { 0.3f, 0.06f, 0.3f }, COLOR_MADERA_OSCURA_PESCA);
    CuboPesca({ farol.x, farol.y - 0.16f, farol.z }, { 0.28f, 0.05f, 0.28f }, COLOR_MADERA_OSCURA_PESCA);

    // Mastil del banderin, balde y caja de aparejos, rollo de soga.
    PrismaPesca(mastil, { mastil.x, Y + 2.75f, mastil.z }, 0.05f, 0.04f, 6, COLOR_BAMBU_OSCURO_PESCA);
    PrismaPesca(pie, { pie.x, Y + 0.34f, pie.z }, 0.2f, 0.25f, 8, Color{ 150, 104, 66, 255 });
    PrismaPesca({ pie.x, Y + 0.08f, pie.z }, { pie.x, Y + 0.12f, pie.z }, 0.215f, 0.22f, 8, Color{ 80, 76, 80, 255 }, 0.0f, false);
    PrismaPesca({ pie.x, Y + 0.33f, pie.z }, { pie.x, Y + 0.35f, pie.z }, 0.25f, 0.25f, 8, Color{ 40, 64, 80, 255 });

    Vector3 caja = P(0.75f, 0.88f, Y + 0.13f);
    CuboGiradoPesca(caja, { 0.42f, 0.24f, 0.3f }, GiroMuellePesca(d) + 8.0f, Color{ 70, 150, 146, 255 });
    CuboGiradoPesca({ caja.x, caja.y + 0.13f, caja.z }, { 0.44f, 0.04f, 0.32f }, GiroMuellePesca(d) + 8.0f, Color{ 52, 116, 114, 255 });

    Vector3 soga = P(1.6f, -0.32f, Y + 0.03f);
    AnilloPesca(soga, 0.1f, 0.2f, TintePesca(Color{ 214, 190, 140, 255 }, soga), 12);
    AnilloPesca({ soga.x, soga.y + 0.04f, soga.z }, 0.08f, 0.17f, TintePesca(Color{ 196, 170, 120, 255 }, soga), 12);
    }

    if (!animado)
    {
        return;
    }

    // Luz del farol (titila).
    float titila = efectos ? 0.9f + 0.1f * std::sin(t * 8.0f + (float)d * 1.9f) : 1.0f;
    CuboPesca(farol, { 0.24f, 0.28f, 0.24f }, TonoPesca(Color{ 255, 210, 130, 255 }, titila));

    // Banderin del color del jugador, que flamea con el viento del oeste.
    Color tela = ocupado ? color : Color{ 236, 226, 206, 255 };
    const int TRAMOS = 5;
    BeginPesca(RL_QUADS);
    for (int s = 0; s < TRAMOS; s++)
    {
        float u0 = (float)s / (float)TRAMOS;
        float u1 = (float)(s + 1) / (float)TRAMOS;
        float o0 = efectos ? std::sin(t * 5.0f - u0 * 4.0f + (float)d) * 0.12f * u0 : 0.0f;
        float o1 = efectos ? std::sin(t * 5.0f - u1 * 4.0f + (float)d) * 0.12f * u1 : 0.0f;
        float h0 = 0.24f * (1.0f - u0);
        float h1 = 0.24f * (1.0f - u1);
        float yc = Y + 2.48f;
        Color c0 = TonoPesca(tela, 0.82f + 0.18f * (0.5f + 0.5f * std::sin(t * 5.0f - u0 * 4.0f)));
        Color c1 = TonoPesca(tela, 0.82f + 0.18f * (0.5f + 0.5f * std::sin(t * 5.0f - u1 * 4.0f)));
        VerticePesca({ mastil.x + u0 * 1.1f, yc - h0, mastil.z + o0 }, c0);
        VerticePesca({ mastil.x + u1 * 1.1f, yc - h1, mastil.z + o1 }, c1);
        VerticePesca({ mastil.x + u1 * 1.1f, yc + h1, mastil.z + o1 }, c1);
        VerticePesca({ mastil.x + u0 * 1.1f, yc + h0, mastil.z + o0 }, c0);
    }
    EndPesca();

    // Colas de lo pescado asomando del balde.
    int colas = peces < 5 ? peces : 5;
    BeginPesca(RL_TRIANGLES);
    for (int k = 0; k < colas; k++)
    {
        float g = (float)k * 1.3f + (float)d;
        Vector3 c = { pie.x + std::cos(g) * 0.1f, Y + 0.36f, pie.z + std::sin(g) * 0.1f };
        Color aleta = (k == 0 && mejor >= 5) ? COLOR_DORADO_PESCA : Color{ 150, 200, 214, 255 };
        Vector3 lado = { std::cos(g + 1.57f) * 0.1f, 0.0f, std::sin(g + 1.57f) * 0.1f };
        VerticePesca(c, aleta);
        VerticePesca({ c.x + lado.x, c.y + 0.2f, c.z + lado.z }, TonoPesca(aleta, 0.8f));
        VerticePesca({ c.x - lado.x, c.y + 0.2f, c.z - lado.z }, TonoPesca(aleta, 0.8f));
    }
    EndPesca();
}


//--------------------------------------------------
// Arrecife, pastos marinos, estrellas de mar y boyas
//--------------------------------------------------

static void DibujarArrecifePesca(float t, bool efectos, int paso)
{
    const Color rosas[4] =
    {
        { 240, 124, 152, 255 }, { 248, 156, 90, 255 }, { 180, 120, 214, 255 }, { 246, 206, 104, 255 }
    };
    const int CORALES = 52;

    for (int k = 0; k < CORALES; k += paso)
    {
        float a = 6.2832f * ((float)k + 0.6f * RuidoPesca(k, 801)) / (float)CORALES;
        float r = 7.75f + 1.1f * RuidoPesca(k, 802);
        float x = std::cos(a) * r;
        float z = std::sin(a) * r;
        float y = AlturaEnPesca(x, z);
        int tipo = (int)(RuidoPesca(k, 803) * 4.0f) % 4;
        Color color = rosas[(k + tipo) % 4];

        // Los abanicos se mecen; el resto del arrecife queda quieto.
        if (tipo == 2 ? !AnimadoPesca() : !EstaticoPesca())
        {
            continue;
        }

        if (tipo == 0)
        {
            // Coral ramificado.
            for (int b = 0; b < 4; b++)
            {
                float g = (float)b * 1.6f + a;
                float alto = 0.35f + 0.3f * RuidoPesca(k * 4 + b, 804);
                Vector3 pie = { x + std::cos(g) * 0.08f, y, z + std::sin(g) * 0.08f };
                Vector3 punta = { x + std::cos(g) * 0.28f, y + alto, z + std::sin(g) * 0.28f };
                PrismaPesca(pie, punta, 0.06f, 0.035f, 5, color);
                RocaPesca(punta, { 0.07f, 0.07f, 0.07f }, k + b, TonoPesca(color, 1.15f), 0.1f);
            }
        }
        else if (tipo == 1)
        {
            // Coral cerebro.
            float rr = 0.28f + 0.18f * RuidoPesca(k, 805);
            RocaPesca({ x, y + rr * 0.3f, z }, { rr, rr * 0.7f, rr }, 820 + k, MezclaPesca(Color{ 196, 190, 110, 255 }, Color{ 150, 196, 132, 255 }, RuidoPesca(k, 806)), 0.12f);
        }
        else if (tipo == 2)
        {
            // Abanico de mar que se mece.
            float mece = efectos ? std::sin(t * 1.3f + (float)k) * 0.12f : 0.0f;
            Vector3 lado = { -std::sin(a), 0.0f, std::cos(a) };
            Vector3 pie = { x, y, z };
            Color c = TintePesca(Color{ 168, 92, 196, 255 }, pie);
            BeginPesca(RL_TRIANGLES);
            for (int s = 0; s < 6; s++)
            {
                float g0 = -1.1f + 2.2f * (float)s / 6.0f;
                float g1 = -1.1f + 2.2f * (float)(s + 1) / 6.0f;
                float h = 0.62f;
                VerticePesca(pie, c);
                VerticePesca({ x + lado.x * std::sin(g0) * h + std::cos(a) * mece, y + std::cos(g0) * h, z + lado.z * std::sin(g0) * h + std::sin(a) * mece }, TonoPesca(c, (s & 1) ? 1.0f : 0.85f));
                VerticePesca({ x + lado.x * std::sin(g1) * h + std::cos(a) * mece, y + std::cos(g1) * h, z + lado.z * std::sin(g1) * h + std::sin(a) * mece }, TonoPesca(c, (s & 1) ? 1.0f : 0.85f));
            }
            EndPesca();
        }
        else
        {
            // Esponjas de tubo.
            for (int b = 0; b < 3; b++)
            {
                float g = (float)b * 2.1f + a;
                Vector3 pie = { x + std::cos(g) * 0.16f, y, z + std::sin(g) * 0.16f };
                float alto = 0.3f + 0.25f * RuidoPesca(k * 3 + b, 807);
                PrismaPesca(pie, { pie.x, y + alto, pie.z }, 0.09f, 0.1f, 6, Color{ 240, 170, 74, 255 });
            }
        }
    }

    if (!EstaticoPesca())
    {
        return;
    }

    // Rocas con coral en el fondo hondo, bajo los peces.
    for (int k = 0; k < 9; k += paso)
    {
        float a = 6.2832f * RuidoPesca(k, 811);
        float r = 1.6f + 4.4f * RuidoPesca(k, 812);
        float x = std::cos(a) * r;
        float z = std::sin(a) * r;
        float y = AlturaEnPesca(x, z);
        RocaPesca({ x, y + 0.08f, z }, { 0.45f, 0.25f, 0.38f }, 830 + k, Color{ 150, 140, 130, 255 }, 0.2f);
        RocaPesca({ x + 0.12f, y + 0.28f, z }, { 0.12f, 0.1f, 0.12f }, 840 + k, rosas[k % 4], 0.1f);
    }

    // Estrellas de mar en la arena del fondo.
    for (int k = 0; k < 10; k += paso)
    {
        float a = 6.2832f * RuidoPesca(k, 851);
        float r = 2.0f + 9.5f * RuidoPesca(k, 852);
        if (r > 6.4f && r < 9.0f) r += 2.6f;
        float x = std::cos(a) * r;
        float z = std::sin(a) * r;
        float y = AlturaEnPesca(x, z) + 0.03f;
        Color c = TintePesca(k % 2 ? Color{ 250, 140, 80, 255 } : Color{ 236, 96, 110, 255 }, { x, y, z });
        float giro = RuidoPesca(k, 853) * 6.2832f;
        BeginPesca(RL_TRIANGLES);
        for (int b = 0; b < 5; b++)
        {
            float g = giro + 6.2832f * (float)b / 5.0f;
            VerticePesca({ x + std::cos(g + 0.5f) * 0.08f, y, z + std::sin(g + 0.5f) * 0.08f }, c);
            VerticePesca({ x + std::cos(g) * 0.3f, y, z + std::sin(g) * 0.3f }, TonoPesca(c, 1.1f));
            VerticePesca({ x + std::cos(g - 0.5f) * 0.08f, y, z + std::sin(g - 0.5f) * 0.08f }, c);
            VerticePesca({ x, y, z }, c);
            VerticePesca({ x + std::cos(g + 0.5f) * 0.08f, y, z + std::sin(g + 0.5f) * 0.08f }, c);
            VerticePesca({ x + std::cos(g - 0.5f) * 0.08f, y, z + std::sin(g - 0.5f) * 0.08f }, c);
        }
        EndPesca();
    }
}


static void DibujarPastosMarinosPesca(float t, bool efectos, int paso)
{
    const int MATAS = 70;

    BeginPesca(RL_TRIANGLES);
    for (int k = 0; k < MATAS; k += paso)
    {
        float a = 6.2832f * ((float)k + RuidoPesca(k, 861)) / (float)MATAS;
        float r = 9.3f + (OrillaLagunaPesca(a) - 10.4f) * RuidoPesca(k, 862);
        float x = std::cos(a) * r;
        float z = std::sin(a) * r;
        float y = AlturaEnPesca(x, z);

        for (int b = 0; b < 3; b++)
        {
            float g = (float)b * 2.0f + (float)k;
            float bx = x + std::cos(g) * 0.12f;
            float bz = z + std::sin(g) * 0.12f;
            float alto = 0.35f + 0.25f * RuidoPesca(k * 3 + b, 863);
            float mece = efectos ? std::sin(t * 1.5f + (float)k * 0.7f + (float)b) * 0.12f : 0.0f;
            Vector3 pie = { bx, y, bz };
            Color c = TintePesca(Color{ 74, 150, 92, 255 }, pie);
            VerticePesca({ bx - 0.04f, y, bz }, TonoPesca(c, 0.8f));
            VerticePesca({ bx + 0.04f, y, bz }, TonoPesca(c, 0.8f));
            VerticePesca({ bx + mece, y + alto, bz + mece * 0.5f }, TonoPesca(c, 1.15f));
        }
    }
    EndPesca();
}


static Vector3 BoyaPesca(int k, float t, bool efectos)
{
    float a = 6.2832f * ((float)k + 0.5f) / 28.0f;
    float sube = efectos ? std::sin(t * 1.6f + (float)k * 0.9f) * 0.03f : 0.0f;
    return { std::cos(a) * 7.6f, 0.05f + sube, std::sin(a) * 7.6f };
}


// Cuerda de boyas: marca el borde de la zona de pesca.
static void DibujarBoyasPesca(float t, bool efectos)
{
    const int BOYAS = 28;

    for (int k = 0; k < BOYAS; k++)
    {
        Vector3 b = BoyaPesca(k, t, efectos);
        Vector3 s = BoyaPesca((k + 1) % BOYAS, t, efectos);
        CintaPesca({ b.x, b.y - 0.02f, b.z }, { s.x, s.y - 0.02f, s.z }, 0.035f, Color{ 232, 214, 170, 255 }, Color{ 232, 214, 170, 255 });
        RocaPesca(b, { 0.14f, 0.11f, 0.14f }, 870 + k, Color{ 248, 246, 238, 255 }, 0.04f);
        PrismaPesca({ b.x, b.y - 0.02f, b.z }, { b.x, b.y + 0.03f, b.z }, 0.145f, 0.145f, 8, Color{ 40, 120, 140, 255 }, 0.0f, false);
    }
}


//--------------------------------------------------
// Rocas, conchas y gaviotas
//--------------------------------------------------

static void DibujarRocasPlayaPesca(int paso)
{
    for (int k = 0; k < 18; k += paso)
    {
        float a = 6.2832f * ((float)k + RuidoPesca(k, 901)) / 18.0f;
        float r = CostaIslaPesca(a) - 0.6f + 1.4f * RuidoPesca(k, 902);
        float x = std::cos(a) * r;
        float z = std::sin(a) * r;
        float y = AlturaEnPesca(x, z);
        float s = 0.6f + 0.8f * RuidoPesca(k, 903);
        RocaPesca({ x, y + s * 0.25f, z }, { s, s * 0.6f, s * 0.8f }, 910 + k, COLOR_ROCA_PESCA, 0.24f);
    }

    // Conchas y piedritas en la playa de la laguna.
    for (int k = 0; k < 16; k += paso)
    {
        float a = 6.2832f * ((float)k + RuidoPesca(k, 921)) / 16.0f;
        float r = OrillaLagunaPesca(a) + 1.0f + 2.0f * RuidoPesca(k, 922);
        float x = std::cos(a) * r;
        float z = std::sin(a) * r;
        float y = AlturaEnPesca(x, z);
        RocaPesca({ x, y + 0.03f, z }, { 0.14f, 0.06f, 0.11f }, 930 + k, k % 2 ? Color{ 250, 214, 214, 255 } : Color{ 236, 236, 226, 255 }, 0.15f);
    }
}


static void DibujarGaviotasPesca(float t, bool efectos)
{
    for (int k = 0; k < 5; k++)
    {
        float vel = 0.22f + 0.04f * (float)k;
        float g = (efectos ? t * vel : 0.0f) + (float)k * 1.3f;
        float radio = 9.0f + 2.5f * (float)(k % 3);
        Vector3 c = { -2.0f + std::cos(g) * radio, 6.8f + 0.7f * (float)k + (efectos ? std::sin(t * 1.3f + (float)k) * 0.3f : 0.0f), -2.0f + std::sin(g) * radio * 0.7f };
        Vector3 frente = NormalizarPesca({ -std::sin(g), 0.0f, std::cos(g) * 0.7f });
        Vector3 lado = { -frente.z, 0.0f, frente.x };
        float aleteo = efectos ? std::sin(t * 7.0f + (float)k * 1.9f) * 0.32f : 0.1f;
        Color blanco = TintePesca(Color{ 250, 244, 238, 255 }, c);
        Color gris = TintePesca(Color{ 150, 150, 170, 255 }, c);

        BeginPesca(RL_TRIANGLES);
        for (int s = -1; s <= 1; s += 2)
        {
            Vector3 codo = { c.x + lado.x * 0.32f * (float)s, c.y + aleteo * 0.5f, c.z + lado.z * 0.32f * (float)s };
            Vector3 punta = { c.x + lado.x * 0.7f * (float)s, c.y + aleteo, c.z + lado.z * 0.7f * (float)s };
            VerticePesca({ c.x + frente.x * 0.12f, c.y, c.z + frente.z * 0.12f }, blanco);
            VerticePesca(codo, blanco);
            VerticePesca({ c.x - frente.x * 0.1f, c.y, c.z - frente.z * 0.1f }, blanco);
            VerticePesca(codo, blanco);
            VerticePesca(punta, gris);
            VerticePesca({ codo.x - frente.x * 0.12f, codo.y, codo.z - frente.z * 0.12f }, gris);
        }
        EndPesca();
        CuboCrudoPesca(c, { 0.12f, 0.1f, 0.12f }, blanco);
    }
}


//--------------------------------------------------
// Piezas jugables: peces, botas, canas, corchos y mira
//--------------------------------------------------

static float RadioPezPesca(TipoPezPesca tipo)
{
    switch (tipo)
    {
        case PEZ_PESCA_MEDIANO: return 0.42f;
        case PEZ_PESCA_RARO: return 0.5f;
        case PEZ_PESCA_BOTA: return 0.36f;
        default: return 0.28f;
    }
}


static void DibujarPezPesca(const PezPesca& pez, float tiempo)
{
    float y = -0.9f;
    Color sombra = Fade(Color{ 10, 40, 60, 255 }, 0.75f);
    float radio = 0.28f;
    bool modelo = pez.tipo == PEZ_PESCA_BOTA
        ? DibujarModeloPescaIslenaRetro3D(MODELO_PESCA_BOTA, { pez.x, y, pez.z }, -std::atan2(pez.dirZ, pez.dirX) * RAD2DEG)
        : DibujarModeloAnimadoPescaIslenaRetro3D((ModeloPescaIslena3D)(MODELO_PESCA_PEQUENO + pez.tipo),
            { pez.x, y, pez.z }, -std::atan2(pez.dirZ, pez.dirX) * RAD2DEG, std::sin(tiempo * 10 + pez.semilla) * .15f);

    if (!modelo)
    {
        // Respaldo con primitivas si falta el GLB de la pieza.
        if (pez.tipo == PEZ_PESCA_MEDIANO)
        {
            radio = 0.42f;
            sombra = Fade(Color{ 10, 30, 70, 255 }, 0.8f);
        }
        else if (pez.tipo == PEZ_PESCA_RARO)
        {
            radio = 0.5f;
            sombra = Fade(Color{ 255, 205, 40, 255 }, 0.9f);
        }
        else if (pez.tipo == PEZ_PESCA_BOTA)
        {
            (DrawCube)({ pez.x, y, pez.z }, 0.3f, 0.12f, 0.8f, Color{ 120, 80, 50, 255 });
            (DrawCube)({ pez.x + 0.25f, y, pez.z + 0.3f }, 0.5f, 0.12f, 0.25f, Color{ 100, 66, 40, 255 });
            return;
        }

        (DrawCylinder)({ pez.x, y, pez.z }, radio, radio, 0.06f, 10, sombra);
        (DrawCylinder)({ pez.x + pez.dirX * radio * 0.8f, y, pez.z + pez.dirZ * radio * 0.8f }, radio * 0.7f, radio * 0.7f, 0.06f, 8, sombra);

        float colaX = pez.x - pez.dirX * radio * 1.3f;
        float colaZ = pez.z - pez.dirZ * radio * 1.3f;
        float lateralX = -pez.dirZ * radio * 0.6f;
        float lateralZ = pez.dirX * radio * 0.6f;
        float coleo = std::sin(tiempo * 10.0f + pez.semilla) * 0.15f;
        Vector3 a = { pez.x - pez.dirX * radio * 0.6f, y + 0.03f, pez.z - pez.dirZ * radio * 0.6f };
        Vector3 b = { colaX + lateralX + coleo, y + 0.03f, colaZ + lateralZ };
        Vector3 c = { colaX - lateralX + coleo, y + 0.03f, colaZ - lateralZ };

        DrawTriangle3D(a, b, c, sombra);
        DrawTriangle3D(a, c, b, sombra);
    }

    // El dorado deja tres destellos que giran a su alrededor.
    if (pez.tipo == PEZ_PESCA_RARO)
    {
        for (int s = 0; s < 3; s++)
        {
            float angulo = tiempo * 3.0f + (float)s * 2.1f;
            DrawSphereEx({ pez.x + std::cos(angulo) * 0.7f, y + 0.1f, pez.z + std::sin(angulo) * 0.7f }, 0.06f, 4, 4, Color{ 255, 240, 140, 255 });
        }
    }
}


static void DibujarCanasCorchosPesca(const MinijuegoPescaIsla& m, int limite)
{
    for (int i = 0; i < limite; i++)
    {
        if (!m.participa[i])
        {
            continue;
        }

        const EstadoJugadorPesca& estado = m.estados[i];
        Color color = m.coloresJugadores[i];
        Vector3 punta = PuntaCanaPesca(m.muelleDe[i]);
        Vector3 mano = PosicionMuellePesca(m.muelleDe[i]);

        // Escala Y explicita: la punta del GLB (0,1.6,-1.4) llega a PuntaCanaPesca.
        if (!DibujarModeloPescaIslenaRetro3D(MODELO_PESCA_CANA, { mano.x, mano.y + 1.1f, mano.z },
            GiroMuellePesca(m.muelleDe[i]), { 0, 1, 0 }, { 1, 1.25f / 1.6f, 1 }))
            DrawCylinderEx({ mano.x, mano.y + 1.1f, mano.z }, punta, 0.06f, 0.03f, 5, Color{ 120, 84, 50, 255 });

        if (estado.estado == PESCA_LIBRE)
        {
            float pulso = 0.5f + 0.5f * std::sin(m.tiempoAnimacion * 6.0f);
            float escala = (.45f + .1f * pulso) / .48f;
            if (DibujarModeloPescaIslenaRetro3D(MODELO_PESCA_CURSOR, { estado.cursorX, .09f, estado.cursorZ },
                0, { 0, 1, 0 }, { escala, 1, escala }, color)) continue;
            DrawCircle3D({ estado.cursorX, 0.09f, estado.cursorZ }, 0.45f + 0.1f * pulso, { 1.0f, 0.0f, 0.0f }, 90.0f, color);
            DrawCircle3D({ estado.cursorX, 0.09f, estado.cursorZ }, 0.25f, { 1.0f, 0.0f, 0.0f }, 90.0f, WHITE);
            DrawLine3D({ estado.cursorX - 0.7f, 0.09f, estado.cursorZ }, { estado.cursorX + 0.7f, 0.09f, estado.cursorZ }, Fade(color, 0.7f));
            DrawLine3D({ estado.cursorX, 0.09f, estado.cursorZ - 0.7f }, { estado.cursorX, 0.09f, estado.cursorZ + 0.7f }, Fade(color, 0.7f));
            continue;
        }

        Vector3 corcho = PosicionCorchoPesca(m, i);

        if (!DibujarModeloPescaIslenaRetro3D(MODELO_PESCA_CORCHO, corcho, 0, { 0, 1, 0 }, { 1, 1, 1 }, color))
        {
            DrawSphereEx(corcho, 0.17f, 6, 6, Color{ 240, 240, 240, 255 });
            DrawSphereEx({ corcho.x, corcho.y + 0.12f, corcho.z }, 0.13f, 6, 6, color);
        }
    }
}


// Sedal: cuelga un poco mientras espera, se tensa y vibra en la pelea.
static void DibujarSedalesPesca(const MinijuegoPescaIsla& m, int limite, float t)
{
    for (int i = 0; i < limite; i++)
    {
        if (!m.participa[i] || m.estados[i].estado == PESCA_LIBRE)
        {
            continue;
        }

        const EstadoJugadorPesca& estado = m.estados[i];
        Vector3 punta = PuntaCanaPesca(m.muelleDe[i]);
        Vector3 corcho = PosicionCorchoPesca(m, i);
        corcho.y += 0.12f;
        float comba = 0.0f;
        float vibra = 0.0f;
        Color color = MezclaPesca(Color{ 250, 250, 244, 255 }, m.coloresJugadores[i], 0.3f);

        if (estado.estado == PESCA_ESPERANDO) comba = 0.45f;
        else if (estado.estado == PESCA_PICADA) comba = 0.1f;
        else if (estado.estado == PESCA_TENSION)
        {
            vibra = 0.02f + 0.06f * Limitar01Pesca(estado.tension);
            color = MezclaPesca(color, Color{ 255, 120, 90, 255 }, Limitar01Pesca((estado.tension - 0.6f) / 0.4f));
        }

        const int TRAMOS = 12;
        Vector3 anterior = punta;

        for (int s = 1; s <= TRAMOS; s++)
        {
            float u = (float)s / (float)TRAMOS;
            Vector3 p = LerpPesca(punta, corcho, u);
            float panza = 4.0f * u * (1.0f - u);
            p.y -= comba * panza;
            p.x += std::sin(t * 47.0f + u * 9.0f) * vibra * panza;
            p.z += std::cos(t * 41.0f + u * 7.0f) * vibra * panza;
            CintaPesca(anterior, p, 0.035f, color, color);
            anterior = p;
        }
    }
}


// Pez (o bota) que vuela del agua al balde despues de la captura.
static void DibujarVuelosPesca(const MinijuegoPescaIsla& m, int limite)
{
    for (int i = 0; i < limite; i++)
    {
        if (!m.participa[i] || m.vueloTiempo[i] < 0.0f || m.vueloTiempo[i] > DURACION_VUELO_PESCA)
        {
            continue;
        }

        float u = Limitar01Pesca(m.vueloTiempo[i] / DURACION_VUELO_PESCA);
        Vector3 desde = m.vueloDesde[i];
        Vector3 hasta = CestaPesca(m.muelleDe[i]);
        Vector3 p = LerpPesca(desde, hasta, u);
        p.y += std::sin(u * 3.1416f) * 2.6f + 0.2f * (1.0f - u);
        float giro = std::atan2(hasta.z - desde.z, hasta.x - desde.x) * RAD2DEG;
        float escala = 1.0f - 0.35f * u;

        EmpujarPesca();
        TrasladarPesca(p.x, p.y, p.z);
        RotarPesca(-giro, 0.0f, 1.0f, 0.0f);
        RotarPesca(u * 540.0f, 0.0f, 0.0f, 1.0f);
        rlScalef(escala, escala, escala);

        if (m.vueloTipo[i] == PEZ_PESCA_BOTA)
        {
            CuboCrudoPesca({ 0.0f, 0.0f, 0.0f }, { 0.22f, 0.55f, 0.24f }, Color{ 112, 78, 50, 255 });
            CuboCrudoPesca({ 0.16f, -0.22f, 0.0f }, { 0.4f, 0.16f, 0.26f }, Color{ 96, 66, 42, 255 });
            CuboCrudoPesca({ 0.0f, 0.28f, 0.0f }, { 0.26f, 0.06f, 0.28f }, Color{ 70, 52, 40, 255 });
        }
        else
        {
            float largo = RadioPezPesca((TipoPezPesca)m.vueloTipo[i]) * 1.4f;
            Color cuerpo = m.vueloTipo[i] == PEZ_PESCA_RARO ? COLOR_DORADO_PESCA
                : (m.vueloTipo[i] == PEZ_PESCA_MEDIANO ? Color{ 96, 148, 226, 255 } : Color{ 120, 206, 206, 255 });
            RocaPesca({ 0.0f, 0.0f, 0.0f }, { largo * 0.5f, largo * 0.24f, largo * 0.16f }, 3, cuerpo, 0.04f, 0.2f);
            BeginPesca(RL_TRIANGLES);
            VerticePesca({ -largo * 0.42f, 0.0f, 0.0f }, TonoPesca(cuerpo, 0.8f));
            VerticePesca({ -largo * 0.78f, largo * 0.24f, 0.0f }, TonoPesca(cuerpo, 0.9f));
            VerticePesca({ -largo * 0.78f, -largo * 0.24f, 0.0f }, TonoPesca(cuerpo, 0.9f));
            VerticePesca({ 0.0f, largo * 0.2f, 0.0f }, TonoPesca(cuerpo, 0.75f));
            VerticePesca({ -largo * 0.25f, largo * 0.36f, 0.0f }, TonoPesca(cuerpo, 0.75f));
            VerticePesca({ -largo * 0.2f, largo * 0.16f, 0.0f }, TonoPesca(cuerpo, 0.75f));
            EndPesca();
            CuboCrudoPesca({ largo * 0.3f, largo * 0.06f, largo * 0.15f }, { 0.05f, 0.05f, 0.03f }, Color{ 30, 30, 40, 255 });
            CuboCrudoPesca({ largo * 0.3f, largo * 0.06f, -largo * 0.15f }, { 0.05f, 0.05f, 0.03f }, Color{ 30, 30, 40, 255 });
        }

        SoltarPesca();
    }
}


static void DibujarParticulasSolidasPesca(const MinijuegoPescaIsla& m)
{
    for (int i = 0; i < MAX_PARTICULAS_PESCA; i++)
    {
        const ParticulaPesca& p = m.particulas[i];

        if (!p.activa || p.tipo == PARTICULA_PESCA_ONDA)
        {
            continue;
        }

        float vida = p.vida / p.vidaMaxima;
        float s = p.tamano;

        switch (p.tipo)
        {
        case PARTICULA_PESCA_GOTA:
        case PARTICULA_PESCA_BARRO:
        case PARTICULA_PESCA_CHISPA:
            s *= 0.45f + 0.55f * vida;
            CuboCrudoPesca(p.posicion, { s, s, s }, p.color);
            break;
        case PARTICULA_PESCA_CONFETI:
            EmpujarPesca();
            TrasladarPesca(p.posicion.x, p.posicion.y, p.posicion.z);
            RotarPesca(p.giro, 0.3f, 1.0f, 0.4f);
            CuboCrudoPesca({ 0.0f, 0.0f, 0.0f }, { s, s * 0.6f, 0.02f }, p.color);
            SoltarPesca();
            break;
        default:
            break;
        }
    }
}


//--------------------------------------------------
// Translucidos: superficie, sombras, ondas, punteria, llamas y humo
//--------------------------------------------------

static void PuntoArcoPesca(const MinijuegoPescaIsla& m, int i, float u, Vector3& p)
{
    const EstadoJugadorPesca& e = m.estados[i];
    Vector3 punta = PuntaCanaPesca(m.muelleDe[i]);
    p =
    {
        punta.x + (e.cursorX - punta.x) * u,
        punta.y * (1.0f - u) + 0.12f * u + std::sin(PI * u) * 1.5f,
        punta.z + (e.cursorZ - punta.z) * u
    };
}


// Superficie horneada (seccion de horneado); false si no hay horno.
static bool DibujarAguaHorneadaPesca(float t, bool efectos);


static void DibujarTranslucidosPesca(const MinijuegoPescaIsla& m, int limite, float t, bool efectos, bool decoracion)
{
    rlDrawRenderBatchActive();
    rlDisableDepthMask();

    // Sombra de cada pez sobre el fondo: da la profundidad del agua.
    for (int f = 0; f < MAX_PECES_PESCA; f++)
    {
        const PezPesca& pez = m.peces[f];
        if (pez.estado == ESTADO_PEZ_AUSENTE) continue;
        float y = AlturaEnPesca(pez.x, pez.z) + 0.03f;
        ResplandorSueloPesca({ pez.x + 0.25f, y, pez.z + 0.2f }, RadioPezPesca(pez.tipo) * 1.7f, Color{ 6, 40, 64, 110 }, 1.3f);
    }

    // Burbujas que suben del arrecife.
    if (efectos)
    {
        for (int k = 0; k < 16; k++)
        {
            float a = 6.2832f * RuidoPesca(k, 1301);
            float r = 7.8f + 1.0f * RuidoPesca(k, 1302);
            float fase = std::fmod(t * (0.3f + 0.2f * RuidoPesca(k, 1303)) + RuidoPesca(k, 1304), 1.0f);
            float x = std::cos(a) * r + std::sin(t * 2.0f + (float)k) * 0.06f;
            float z = std::sin(a) * r;
            float suelo = AlturaEnPesca(x, z);
            Vector3 p = { x, suelo + (-0.05f - suelo) * fase, z };
            ResplandorFrentePesca(p, 0.06f + 0.03f * fase, AlfaPesca(Color{ 230, 255, 255, 255 }, 0.6f * (1.0f - fase * 0.6f)));
        }
    }

    if (!DibujarAguaHorneadaPesca(t, efectos))
    {
        DibujarSuperficieLagunaPesca(t, efectos);
    }

    DibujarEspumaPesca(t, efectos, decoracion);

    // Sombra de los muelles sobre el agua.
    for (int d = 0; d < 4; d++)
    {
        float largo = LargoPasarelaPesca(d);
        Color oscuro = Color{ 10, 40, 50, 70 };
        BeginPesca(RL_QUADS);
        QuadPesca(PuntoMuellePesca(d, -1.2f, -1.5f, 0.01f), PuntoMuellePesca(d, 1.45f, -1.5f, 0.01f),
            PuntoMuellePesca(d, 1.45f, 1.1f, 0.01f), PuntoMuellePesca(d, -1.2f, 1.1f, 0.01f), oscuro);
        QuadPesca(PuntoMuellePesca(d, 1.45f, -0.8f, 0.01f), PuntoMuellePesca(d, largo, -0.8f, 0.01f),
            PuntoMuellePesca(d, largo, 0.5f, 0.01f), PuntoMuellePesca(d, 1.45f, 0.5f, 0.01f), oscuro);
        EndPesca();

        // Ondas alrededor de los pilotes.
        if (efectos)
        {
            for (int s = -1; s <= 1; s += 2)
            {
                for (int k = 0; k < 2; k++)
                {
                    Vector3 c = PuntoMuellePesca(d, k == 0 ? -1.12f : 0.0f, (float)s * 1.24f, 0.02f);
                    float fase = std::fmod(t * 0.5f + (float)(d * 3 + k) * 0.31f + (float)s * 0.2f, 1.0f);
                    float radio = 0.16f + 0.4f * fase;
                    AnilloPesca(c, radio, radio + 0.04f, AlfaPesca(WHITE, 0.4f * (1.0f - fase)), 16);
                }
            }
        }
    }

    // Boyas: reflejo claro alrededor.
    for (int k = 0; k < 28; k++)
    {
        Vector3 b = BoyaPesca(k, t, efectos);
        AnilloPesca({ b.x, 0.02f, b.z }, 0.15f, 0.24f, AlfaPesca(WHITE, 0.22f), 12);
    }

    for (int i = 0; i < limite; i++)
    {
        if (!m.participa[i]) continue;

        const EstadoJugadorPesca& e = m.estados[i];
        Color color = m.coloresJugadores[i];
        int d = m.muelleDe[i];

        // Anillo del color del jugador sobre su plataforma.
        AnilloPesca(PuntoMuellePesca(d, 0.0f, 0.0f, ALTURA_MUELLE_PESCA + 0.015f), 0.64f, 0.78f, AlfaPesca(color, 0.8f), 28);

        if (e.estado == PESCA_LIBRE)
        {
            // Arco punteado del lanzamiento (si hay carnada).
            if (e.bloqueo <= 0.0f)
            {
                for (int s = 1; s < 14; s++)
                {
                    Vector3 p;
                    float u = ((float)s + std::fmod(t * 2.0f, 1.0f)) / 14.0f;
                    PuntoArcoPesca(m, i, u, p);
                    CartelPesca(p, 0.09f, 0.09f, AlfaPesca(MezclaPesca(color, WHITE, 0.35f), 0.75f));
                }
            }
            continue;
        }

        Vector3 corcho = PosicionCorchoPesca(m, i);

        if (e.estado == PESCA_ESPERANDO)
        {
            for (int k = 0; k < 2; k++)
            {
                float fase = std::fmod(t * 0.7f + (float)k * 0.5f + (float)i * 0.17f, 1.0f);
                float radio = 0.22f + 0.65f * fase;
                AnilloPesca({ corcho.x, 0.025f, corcho.z }, radio, radio + 0.05f, AlfaPesca(WHITE, 0.5f * (1.0f - fase)), 24);
            }
        }
        else if (e.estado == PESCA_PICADA)
        {
            for (int k = 0; k < 3; k++)
            {
                float fase = std::fmod(t * 3.2f + (float)k / 3.0f, 1.0f);
                float radio = 0.25f + 1.1f * fase;
                AnilloPesca({ corcho.x, 0.03f, corcho.z }, radio, radio + 0.09f, AlfaPesca(Color{ 255, 250, 220, 255 }, 0.85f * (1.0f - fase)), 28);
            }
        }
        else if (e.estado == PESCA_TENSION && e.pez >= 0)
        {
            const PezPesca& pez = m.peces[e.pez];
            for (int k = 0; k < 2; k++)
            {
                float fase = std::fmod(t * 2.2f + (float)k * 0.5f, 1.0f);
                float radio = 0.3f + 0.8f * fase;
                AnilloPesca({ pez.x, 0.03f, pez.z }, radio, radio + 0.07f, AlfaPesca(WHITE, 0.6f * (1.0f - fase)), 24);
            }
        }
    }

    // Ondas de las salpicaduras.
    for (int k = 0; k < MAX_PARTICULAS_PESCA; k++)
    {
        const ParticulaPesca& p = m.particulas[k];
        if (!p.activa || p.tipo != PARTICULA_PESCA_ONDA) continue;
        float vida = p.vida / p.vidaMaxima;
        AnilloPesca(p.posicion, p.tamano, p.tamano + 0.08f, AlfaPesca(p.color, 0.65f * vida), 24);
    }

    // Llamas de las antorchas.
    for (int k = 0; k < CANTIDAD_ANTORCHAS_PESCA; k++)
    {
        LlamaVisiblePesca(LlamaAntorchaPesca(k, t, efectos), t, k, efectos, 1.0f);
    }

    if (decoracion)
    {
        DibujarHumoVolcanPesca(t, efectos);
    }

    rlDrawRenderBatchActive();
    rlEnableDepthMask();
}


//--------------------------------------------------
// Luces aditivas
//--------------------------------------------------

static void DibujarLucesPesca(const MinijuegoPescaIsla& m, int limite, float t, bool decoracion)
{
    rlDrawRenderBatchActive();
    rlDisableDepthMask();
    BeginBlendMode(BLEND_ADDITIVE);

    // Sol bajo y su camino de brillos sobre el mar.
    Vector3 sol = PosicionSolPesca();
    HaloPesca(sol, 70.0f, Color{ 255, 150, 90, 70 }, Color{ 255, 230, 170, 120 });
    Vector3 haciaSol = NormalizarPesca({ DIRECCION_SOL_PESCA.x, 0.0f, DIRECCION_SOL_PESCA.z });

    for (int k = 0; k < 30; k++)
    {
        float dist = 24.0f + (float)(k * k) * 0.32f;
        float lado = (RuidoPesca(k, 1401) - 0.5f) * dist * 0.08f;
        Vector3 p = { ojoPesca.x + haciaSol.x * dist - haciaSol.z * lado, Y_MAR_PESCA + 0.04f, ojoPesca.z + haciaSol.z * dist + haciaSol.x * lado };
        if (p.x * p.x + p.z * p.z < 900.0f) continue;
        float brillo = 0.5f + 0.5f * std::sin(t * 3.0f + (float)k * 1.7f);
        ResplandorPesca(p, { haciaSol.z, 0.0f, -haciaSol.x }, haciaSol, 1.0f + dist * 0.03f, AlfaPesca(Color{ 255, 200, 130, 255 }, 0.16f + 0.14f * brillo), 2.6f);
    }

    // Antorchas tiki: halo, charco en la arena, reflejo en la laguna y brasas.
    for (int k = 0; k < CANTIDAD_ANTORCHAS_PESCA; k++)
    {
        Vector3 l = LlamaAntorchaPesca(k, t, true);
        Vector3 b = BaseAntorchaPesca(k);
        float p = 0.88f + 0.08f * std::sin(t * 11.0f + (float)k * 2.0f) + 0.04f * std::sin(t * 29.0f + (float)k);
        HaloPesca({ l.x, l.y + 0.15f, l.z }, 1.5f * p, Color{ 255, 150, 70, 90 }, Color{ 255, 226, 150, 170 });
        CharcoLuzPesca({ b.x, b.y + 0.06f, b.z }, 3.4f * p, COLOR_LUZ_PESCA, 1.3f);
        Vector3 reflejo = { b.x * 0.9f, 0.02f, b.z * 0.9f };
        ResplandorPesca(reflejo, { b.x / 14.0f, 0.0f, b.z / 14.0f }, { -b.z / 14.0f, 0.0f, b.x / 14.0f }, 0.9f, AlfaPesca(COLOR_LUZ_PESCA, 0.22f * p), 2.2f);

        if (decoracion)
        {
            for (int e = 0; e < 3; e++)
            {
                float fase = std::fmod(t * 0.55f + (float)e / 3.0f + (float)k * 0.13f, 1.0f);
                Vector3 brasa = { l.x + std::sin(t * 2.0f + (float)(e + k)) * 0.25f * fase, l.y + 0.3f + fase * 1.6f, l.z + std::cos(t * 1.7f + (float)e) * 0.2f * fase };
                ResplandorFrentePesca(brasa, 0.07f, AlfaPesca(Color{ 255, 170, 80, 255 }, 0.8f * (1.0f - fase)));
            }
        }
    }

    // Faroles de los muelles: halo, charco en las tablas y reflejo en el agua.
    for (int d = 0; d < 4; d++)
    {
        Vector3 f = FarolMuellePesca(d);
        float p = 0.9f + 0.1f * std::sin(t * 8.0f + (float)d * 1.9f);
        HaloPesca(f, 1.1f * p, Color{ 255, 170, 90, 90 }, Color{ 255, 236, 180, 170 });
        CharcoLuzPesca({ f.x, ALTURA_MUELLE_PESCA + 0.02f, f.z }, 1.7f * p, COLOR_LUZ_PESCA, 1.2f);
        Vector3 n = NormalMuellePesca(d);
        Vector3 reflejo = PuntoMuellePesca(d, -1.75f, -1.0f, 0.02f);
        ResplandorPesca(reflejo, n, { -n.z, 0.0f, n.x }, 0.7f, AlfaPesca(COLOR_LUZ_PESCA, 0.26f * p), 2.4f);
    }

    // Ventanas de las cabanas.
    if (decoracion)
    {
        for (int k = 0; k < (int)(sizeof(CABANAS_PESCA) / sizeof(CABANAS_PESCA[0])); k++)
        {
            Vector3 v = VentanaCabanaPesca(CABANAS_PESCA[k]);
            HaloPesca(v, 0.9f, Color{ 255, 180, 100, 70 }, Color{ 255, 230, 170, 110 });
            CharcoLuzPesca({ v.x, AlturaEnPesca(v.x, v.z) + 0.05f, v.z }, 2.2f, COLOR_LUZ_PESCA, 0.8f);
        }
    }

    // Volcan: resplandor del crater y de las coladas.
    {
        float latido = 0.85f + 0.15f * std::sin(t * 2.1f);
        Vector3 crater = { CENTRO_VOLCAN_PESCA.x, CENTRO_VOLCAN_PESCA.y + 25.0f, CENTRO_VOLCAN_PESCA.z };
        HaloPesca(crater, 9.0f * latido, Color{ 255, 110, 40, 60 }, Color{ 255, 190, 100, 70 });
        const float coladas[3] = { 1.42f, 1.95f, 2.55f };
        for (int c = 0; c < 3; c++)
        {
            for (int s = 0; s < 4; s++)
            {
                float dd = 5.0f + 3.6f * (float)s;
                Vector3 p = PuntoVolcanPesca(dd, coladas[c] + 0.09f * std::sin(dd * 0.45f + (float)c));
                ResplandorFrentePesca({ p.x, p.y + 0.6f, p.z }, 2.4f, AlfaPesca(COLOR_LAVA_PESCA, 0.18f * latido));
            }
        }
    }

    // Peces dorados: brillo bajo el agua.
    for (int f = 0; f < MAX_PECES_PESCA; f++)
    {
        const PezPesca& pez = m.peces[f];
        if (pez.estado == ESTADO_PEZ_AUSENTE || pez.tipo != PEZ_PESCA_RARO) continue;
        float pulso = 0.75f + 0.25f * std::sin(t * 4.0f + pez.semilla);
        ResplandorSueloPesca({ pez.x, -0.88f, pez.z }, 1.2f, AlfaPesca(COLOR_DORADO_PESCA, 0.4f * pulso));
        ResplandorSueloPesca({ pez.x, 0.015f, pez.z }, 0.9f, AlfaPesca(COLOR_DORADO_PESCA, 0.14f * pulso));
    }

    for (int i = 0; i < limite; i++)
    {
        if (!m.participa[i]) continue;

        const EstadoJugadorPesca& e = m.estados[i];
        Color color = m.coloresJugadores[i];

        // Mira: charco del color del jugador (apagado sin carnada).
        if (e.estado == PESCA_LIBRE)
        {
            CharcoLuzPesca({ e.cursorX, 0.03f, e.cursorZ }, 1.1f, color, e.bloqueo > 0.0f ? 0.5f : 1.8f);
        }
        else if (e.estado == PESCA_PICADA)
        {
            // La picada: destello fuerte sobre el corcho.
            Vector3 c = PosicionCorchoPesca(m, i);
            float pulso = 0.7f + 0.3f * std::sin(t * 30.0f);
            HaloPesca({ c.x, c.y + 0.35f, c.z }, 1.8f * pulso, Color{ 255, 230, 140, 140 }, Color{ 255, 255, 230, 200 });
            CharcoLuzPesca({ c.x, 0.03f, c.z }, 1.8f, Color{ 255, 236, 160, 255 }, 2.0f);
        }

        if (m.destelloJugador[i] > 0.0f)
        {
            float d = m.destelloJugador[i];
            Vector3 c = PuntoMuellePesca(m.muelleDe[i], 0.0f, 0.0f, ALTURA_MUELLE_PESCA + 1.1f);
            HaloPesca(c, 2.0f * d + 0.4f, AlfaPesca(color, 0.45f * d), AlfaPesca(WHITE, 0.35f * d));
        }
    }

    // Chispas con brillo.
    for (int i = 0; i < MAX_PARTICULAS_PESCA; i++)
    {
        const ParticulaPesca& p = m.particulas[i];
        if (!p.activa || p.tipo != PARTICULA_PESCA_CHISPA) continue;
        float vida = p.vida / p.vidaMaxima;
        ResplandorFrentePesca(p.posicion, p.tamano * 3.2f, AlfaPesca(p.color, 0.55f * vida));
    }

    // Luciernagas en el borde de la selva.
    if (decoracion)
    {
        int cantidad = 8 + (int)(26.0f * FactorCalidadGrafica(CalidadParticulas()));
        for (int k = 0; k < cantidad; k++)
        {
            float a = 6.2832f * RuidoPesca(k, 1501);
            float r = OrillaLagunaPesca(a) + 3.8f + 3.5f * RuidoPesca(k, 1502);
            float x = std::cos(a) * r + std::sin(t * 0.6f + (float)k) * 0.7f;
            float z = std::sin(a) * r + std::cos(t * 0.5f + (float)k * 1.3f) * 0.7f;
            float y = AlturaEnPesca(x, z) + 0.8f + 0.5f * std::sin(t * 1.1f + (float)k * 2.0f);
            float brillo = Limitar01Pesca(0.5f + 0.7f * std::sin(t * 2.6f + (float)k * 1.7f));
            ResplandorFrentePesca({ x, y, z }, 0.16f, AlfaPesca(Color{ 220, 255, 140, 255 }, 0.7f * brillo));
        }
    }

    EndBlendMode();
    rlDrawRenderBatchActive();
    rlEnableDepthMask();
}


//--------------------------------------------------
// Avisos 2D sobre el mundo
//--------------------------------------------------

static float AlfaAvisoPesca(float u)
{
    if (u < 0.0f || u > 1.0f) return 0.0f;
    if (u < 0.12f) return u / 0.12f;
    if (u > 0.7f) return (1.0f - u) / 0.3f;
    return 1.0f;
}


static void DibujarMedidorTensionPesca(Vector2 centro, const EstadoJugadorPesca& e, float t)
{
    float ancho = Px(92.0f);
    float alto = Px(13.0f);
    float borde = Px(3.0f);
    float tension = Limitar01Pesca(e.tension);
    Rectangle marco = { centro.x - ancho * 0.5f - borde, centro.y - alto * 0.5f - borde, ancho + borde * 2.0f, alto + borde * 2.0f + Px(8.0f) };
    Rectangle pista = { centro.x - ancho * 0.5f, centro.y - alto * 0.5f, ancho, alto };

    DrawRectangleRounded(marco, 0.4f, 6, UI_MADERA_OSCURA);
    DrawRectangleRounded(pista, 0.45f, 6, UI_PLACA_CLARA);
    // Zona de corte: desde el 80 % la linea esta por romperse.
    DrawRectangleRec({ pista.x + ancho * 0.8f, pista.y, ancho * 0.2f, alto }, Color{ 236, 150, 130, 255 });

    Color relleno = tension < 0.55f ? UI_POSITIVO : (tension < 0.8f ? Color{ 246, 186, 64, 255 } : UI_PELIGRO);
    if (tension >= 0.8f && std::fmod(t * 8.0f, 1.0f) < 0.5f) relleno = Color{ 255, 236, 220, 255 };
    if (tension > 0.01f)
    {
        DrawRectangleRounded({ pista.x, pista.y, ancho * tension, alto }, 0.45f, 6, relleno);
    }

    DrawRectangleRec({ pista.x + ancho * 0.8f - Px(1.0f), pista.y - Px(2.0f), Px(2.0f), alto + Px(4.0f) }, UI_TINTA);

    // Avance de la captura (barrita inferior).
    Rectangle avance = { pista.x, pista.y + alto + Px(3.0f), ancho, Px(5.0f) };
    DrawRectangleRounded(avance, 0.5f, 4, Color{ 60, 44, 40, 255 });
    if (e.progreso > 0.01f)
    {
        DrawRectangleRounded({ avance.x, avance.y, ancho * Limitar01Pesca(e.progreso), avance.height }, 0.5f, 4, Color{ 120, 214, 255, 255 });
    }

    // Indicacion breve de que hacer.
    const char* pista2 = nullptr;
    Color colorPista = UI_TEXTO_MUNDO;
    if (e.recogiendo && e.tension > 0.72f) { pista2 = "¡AFLOJÁ!"; colorPista = Color{ 255, 150, 120, 255 }; }
    else if (!e.recogiendo && e.tension < 0.3f) { pista2 = "¡RECOGÉ!"; colorPista = Color{ 160, 240, 160, 255 }; }

    if (pista2)
    {
        // Muelle del fondo: el medidor queda cerca de las tarjetas del HUD,
        // asi que la indicacion va debajo para no tocarlas.
        float alfa = 0.7f + 0.3f * std::sin(t * 12.0f);
        float y = centro.y < Px(185.0f) ? marco.y + marco.height + Px(14.0f) : marco.y - Px(20.0f);
        DibujarTextoMundoContornoUI(pista2, centro.x, y, UI_TEXTO_PEQUENO, colorPista, alfa);
    }
}


static void DibujarAvisosPesca(
    const MinijuegoPescaIsla& m,
    const JugadorPrueba jugadores[],
    const Participante participantes[],
    int limite,
    const Camera3D& vista
)
{
    float t = m.tiempoAnimacion;

    for (int i = 0; i < limite; i++)
    {
        if (!m.participa[i])
        {
            continue;
        }

        const EstadoJugadorPesca& e = m.estados[i];
        Color color = m.coloresJugadores[i];
        Vector3 cabeza = { jugadores[i].posicion.x, jugadores[i].posicion.y + jugadores[i].tamano.y * 0.5f + 0.55f, jugadores[i].posicion.z };
        Vector2 p = GetWorldToScreen(cabeza, vista);

        // Cartelito J#.
        char rotulo[16];
        std::snprintf(rotulo, sizeof(rotulo), "J%d", participantes[i].numeroJugador > 0 ? participantes[i].numeroJugador : i + 1);
        OpcionesEtiquetaUI op;
        op.borde = color;
        op.pico = true;
        Rectangle placa = DibujarEtiquetaUI(rotulo, p, 0.0f, op);

        // Medidor del tira y afloja al lado del cartel.
        if (e.estado == PESCA_TENSION && m.fase == FASE_PESCA_JUGANDO)
        {
            DibujarMedidorTensionPesca({ placa.x + placa.width + Px(56.0f), placa.y + placa.height * 0.5f }, e, t);
        }

        // ¡Picó! sobre el corcho.
        if (e.estado == PESCA_PICADA)
        {
            Vector3 c = PosicionCorchoPesca(m, i);
            Vector2 s = GetWorldToScreen({ c.x, c.y + 1.0f, c.z }, vista);
            float salto = std::sin(Limitar01Pesca(e.tiempoEstado / 0.12f) * 1.5708f);
            Color tono = std::fmod(t * 10.0f, 1.0f) < 0.5f ? Color{ 255, 226, 80, 255 } : Color{ 255, 255, 255, 255 };
            DibujarTextoMundoContornoUI("¡!", s.x, s.y - Px(10.0f) * salto, UI_TEXTO_TITULO, tono, 1.0f);
        }

        // Aviso breve donde estaba el corcho.
        if (m.avisoTipo[i] != AVISO_PESCA_NINGUNO)
        {
            float u = m.avisoTiempo[i] / DURACION_AVISO_PESCA;
            float alfa = AlfaAvisoPesca(u);
            Vector3 a = m.avisoPosicion[i];
            Vector2 s = GetWorldToScreen({ a.x, 0.9f + u * 0.8f, a.z }, vista);
            const char* texto = "¡SE ESCAPÓ!";
            Color tono = Color{ 214, 236, 255, 255 };
            int token = UI_TEXTO;

            if (m.avisoTipo[i] == AVISO_PESCA_BOTA)
            {
                texto = "¡UNA BOTA!";
                tono = Color{ 236, 206, 160, 255 };
                token = UI_TEXTO_GRANDE;
            }
            else if (m.avisoTipo[i] == AVISO_PESCA_CORTE)
            {
                texto = "¡SE CORTÓ!";
                tono = Color{ 255, 150, 130, 255 };
                token = UI_TEXTO_GRANDE;
            }

            DibujarTextoMundoContornoUI(texto, s.x, s.y, token, tono, alfa);
        }

        // «+N» sobre quien pesco.
        if (e.popupTiempo > 0.0f && e.popupValor >= 0)
        {
            float u = 1.0f - e.popupTiempo / 1.6f;
            Vector2 s = GetWorldToScreen({ cabeza.x, cabeza.y + 0.6f + u * 0.9f, cabeza.z }, vista);
            char texto[8];
            std::snprintf(texto, sizeof(texto), "+%d", e.popupValor);
            Color tono = e.popupValor >= 5 ? COLOR_DORADO_PESCA : Color{ 210, 255, 200, 255 };
            float x = s.x;
            float y = s.y - Px(26.0f);

            // Muelle del fondo: al costado del cartel, sin subir hasta las
            // tarjetas del HUD.
            if (y < Px(150.0f))
            {
                x = placa.x - Px(34.0f);
                y = placa.y + placa.height * 0.5f - u * Px(14.0f);
            }

            DibujarTextoMundoContornoUI(texto, x, y, UI_TEXTO_GRANDE, tono, AlfaAvisoPesca(u));
        }
    }

    // Anuncio del pez dorado.
    if (m.jugadorDorado >= 0 && m.anuncioDorado < 1.8f)
    {
        float u = m.anuncioDorado / 1.8f;
        float alfa = AlfaAvisoPesca(u);
        float x = (float)GetScreenWidth() * 0.5f;
        float y = (float)GetScreenHeight() * 0.3f - u * Px(12.0f);
        char linea[40];
        std::snprintf(linea, sizeof(linea), "J%d SUMA 5 PUNTOS", participantes[m.jugadorDorado].numeroJugador > 0 ? participantes[m.jugadorDorado].numeroJugador : m.jugadorDorado + 1);
        DibujarTextoMundoContornoUI("¡PEZ DORADO!", x, y, UI_TEXTO_TITULO, COLOR_DORADO_PESCA, alfa);
        DibujarTextoMundoContornoUI(linea, x, y + Px(62.0f), UI_TEXTO_GRANDE, UI_TEXTO_MUNDO, alfa);
    }
}


//==================================================
// DECORADO Y HORNEADO
//==================================================

// Muelles, boyas, arrecife, pastos, utileria, antorchas, cabanas, selva,
// isla, volcan, islotes y mar: de cerca a lejos, para que la prueba de
// profundidad descarte lo tapado. Segun g_partePesca dibuja todo, solo lo
// estatico (al grabar el horno) o solo lo animado (con el horno listo).
// Las palmeras van aparte: cada una tiene su malla y se mece entera.
static void DibujarDecoradoPesca(const MinijuegoPescaIsla& m, int limite, float t, bool efectos, bool decoracion, int paso)
{
    bool estatico = EstaticoPesca();
    bool animado = AnimadoPesca();

    // Muelles de bambu (los que no usa nadie quedan con banderin crudo).
    for (int d = 0; d < 4; d++)
    {
        int duenio = -1;

        for (int i = 0; i < limite; i++)
        {
            if (m.participa[i] && m.muelleDe[i] == d) duenio = i;
        }

        DibujarMuellePesca(
            d,
            duenio >= 0,
            duenio >= 0 ? m.coloresJugadores[duenio] : WHITE,
            duenio >= 0 ? m.estados[duenio].peces : 0,
            duenio >= 0 ? m.estados[duenio].mejorValor : 0,
            t,
            efectos
        );
    }

    // Laguna: la cuerda de boyas, el arrecife y los pastos.
    if (estatico) DibujarBoyasPesca(t, efectos);
    DibujarArrecifePesca(t, efectos, decoracion ? paso : 3);
    if (animado && decoracion) DibujarPastosMarinosPesca(t, efectos, paso);

    if (g_partePesca == PARTE_PESCA_TODO)
    {
        for (int k = 0; k < CANTIDAD_PALMERAS_PESCA; k += (decoracion ? paso : 3))
        {
            PalmeraPesca(PALMERAS_PESCA[k], 40 + k, t, efectos);
        }
    }

    for (int k = 0; k < (int)(sizeof(CABANAS_PESCA) / sizeof(CABANAS_PESCA[0])); k += (decoracion ? 1 : 2))
    {
        CabanaPesca(CABANAS_PESCA[k], t, efectos);
    }

    if (estatico)
    {
        for (int k = 0; k < CANTIDAD_ANTORCHAS_PESCA; k++)
        {
            AntorchaPesca(k);
        }
    }

    if (decoracion)
    {
        if (estatico)
        {
            float aBarca = 22.0f * DEG2RAD;
            float rBarca = OrillaLagunaPesca(aBarca) + 2.3f;
            Vector3 barca = { std::cos(aBarca) * rBarca, 0.0f, std::sin(aBarca) * rBarca };
            barca.y = AlturaEnPesca(barca.x, barca.z) - 0.05f;
            BarcaPesca(barca, -64.0f);

            Vector3 pila[4] = { { 11.4f, 0.0f, -13.4f }, { 12.4f, 0.0f, -12.8f }, { -11.6f, 0.0f, -13.0f }, { -16.2f, 0.0f, 7.6f } };
            for (int k = 0; k < 4; k++) pila[k].y = AlturaEnPesca(pila[k].x, pila[k].z) - 0.04f;
            CajonPesca(pila[0], 12.0f, 0.7f);
            CajonPesca({ pila[0].x + 0.1f, pila[0].y + 0.7f, pila[0].z }, 30.0f, 0.5f);
            BarrilPesca(pila[1]);
            BarrilPesca(pila[2]);
            CajonPesca({ pila[2].x + 0.9f, pila[2].y, pila[2].z + 0.3f }, -20.0f, 0.6f);
            BarrilPesca(pila[3]);

            DibujarRocasPlayaPesca(paso);
        }

        if (animado)
        {
            CanoaPesca({ -8.3f, -0.12f, -7.0f }, 132.0f, t, efectos);

            float aRed = -26.0f * DEG2RAD;
            float rRed = OrillaLagunaPesca(aRed) + 2.6f;
            Vector3 redA = { std::cos(aRed) * rRed - 1.1f, 0.0f, std::sin(aRed) * rRed - 0.8f };
            Vector3 redB = { std::cos(aRed) * rRed + 1.1f, 0.0f, std::sin(aRed) * rRed + 0.8f };
            redA.y = AlturaEnPesca(redA.x, redA.z) - 0.05f;
            redB.y = AlturaEnPesca(redB.x, redB.z) - 0.05f;
            RedPesca(redA, redB, t, efectos);
        }
    }

    // Isla: selva, fondo de la laguna, playa y meseta.
    if (estatico)
    {
        DibujarSelvaPesca(decoracion ? paso : 3, decoracion);
        DibujarTerrenoPesca(t, efectos, decoracion ? 96 : 72);
    }

    // Horizonte: volcan, islotes, velero y mar.
    DibujarVolcanPesca(t, efectos, decoracion ? 48 : 28);
    if (decoracion) DibujarIslotesPesca(t, efectos);
    if (estatico) DibujarMarPesca(t, efectos);
}


static const char* VS_HORNO_PESCA =
    "#version 330\n"
    "in vec3 vertexPosition;\n"
    "in vec2 vertexTexCoord;\n"
    "in vec4 vertexColor;\n"
    "uniform mat4 mvp;\n"
    "uniform mat4 matModel;\n"
    "out vec4 fragColor;\n"
    "out vec3 fragPos;\n"
    "out float fragCaustica;\n"
    "out float fragModo;\n"
    "void main()\n"
    "{\n"
    "    fragColor = vertexColor;\n"
    "    fragPos = (matModel * vec4(vertexPosition, 1.0)).xyz;\n"
    "    fragCaustica = vertexTexCoord.x;\n"
    "    fragModo = vertexTexCoord.y;\n"
    "    gl_Position = mvp * vec4(vertexPosition, 1.0);\n"
    "}\n";

// Niebla del atardecer, caustica del fondo y ondas de la laguna (las mismas
// formulas que CantidadNieblaPesca, CausticaPesca y la superficie). Modo:
// 0 normal, 1 cielo sin niebla, 2 agua con ondas.
static const char* FS_HORNO_PESCA =
    "#version 330\n"
    "in vec4 fragColor;\n"
    "in vec3 fragPos;\n"
    "in float fragCaustica;\n"
    "in float fragModo;\n"
    "uniform vec3 ojo;\n"
    "uniform vec4 niebla;\n"
    "uniform float tiempo;\n"
    "uniform float ondas;\n"
    "out vec4 finalColor;\n"
    "void main()\n"
    "{\n"
    "    vec3 c = fragColor.rgb;\n"
    "    float a = fragColor.a;\n"
    "    if (fragModo > 0.5 && fragModo < 1.5)\n"
    "    {\n"
    "        finalColor = fragColor;\n"
    "        return;\n"
    "    }\n"
    "    if (fragModo > 1.5 && ondas > 0.5)\n"
    "    {\n"
    "        float o = sin(fragPos.x * 0.9 + tiempo * 1.2) * sin(fragPos.z * 0.8 - tiempo * 0.9);\n"
    "        c = min(c * (1.0 + 0.07 * o), vec3(1.0));\n"
    "        a = clamp(a + 0.03 * o, 0.0, 1.0);\n"
    "    }\n"
    "    if (fragCaustica > 0.001)\n"
    "    {\n"
    "        float x = fragPos.x, z = fragPos.z;\n"
    "        float k = sin(x * 1.6 + tiempo * 1.1) + sin(z * 1.8 - tiempo * 0.9) +\n"
    "            sin((x - z) * 1.2 + tiempo * 1.4) + sin((x + z) * 0.85 - tiempo * 0.7);\n"
    "        k = clamp(1.0 - abs(k * 0.25) * 2.2, 0.0, 1.0);\n"
    "        c = mix(c, vec3(0.894, 1.0, 0.965), k * k * fragCaustica);\n"
    "    }\n"
    "    float n = clamp((distance(fragPos, ojo) - 32.0) / 120.0, 0.0, 0.72);\n"
    "    finalColor = vec4(mix(c, niebla.rgb, n), a);\n"
    "}\n";


struct PalmeraHornoPesca
{
    Mesh malla{};
    bool valida = false;
    Vector3 base{};
    float alto = 0.0f;
    int semilla = 0;
};

struct HornoPesca
{
    bool listo = false;
    int clave = -1;
    bool shaderListo = false;
    Shader shader{};
    Material material{};
    int locOjo = -1;
    int locNiebla = -1;
    int locTiempo = -1;
    int locOndas = -1;
    Mesh decorado{};
    bool decoradoValido = false;
    Mesh cielo{};           // cupula y nubes, relativas al ojo
    bool cieloValido = false;
    Mesh nubes{};
    bool nubesValidas = false;
    Mesh agua{};            // superficie translucida de la laguna
    bool aguaValida = false;
    PalmeraHornoPesca palmeras[CANTIDAD_PALMERAS_PESCA];
};

static HornoPesca g_hornoPesca;


static void EmpezarGrabacionPesca()
{
    GrabacionPesca& g = g_grabacionPesca;
    g.activa = true;
    g.enQuad = 0;
    g.caustica = 0.0f;
    g.modoShader = 0.0f;
    g.matriz = MatrixIdentity();
    g.profundidad = 0;
    g.posiciones.clear();
    g.colores.clear();
    g.causticas.clear();
}


// Cierra la grabacion y sube la malla (sin indices: cabe cualquier tamano).
static bool TerminarGrabacionPesca(Mesh& malla)
{
    GrabacionPesca& g = g_grabacionPesca;
    g.activa = false;
    int n = (int)(g.posiciones.size() / 3);
    n -= n % 3;

    if (n < 3)
    {
        return false;
    }

    malla = {};
    malla.vertexCount = n;
    malla.triangleCount = n / 3;
    malla.vertices = (float*)MemAlloc((unsigned int)(n * 3 * (int)sizeof(float)));
    malla.texcoords = (float*)MemAlloc((unsigned int)(n * 2 * (int)sizeof(float)));
    malla.colors = (unsigned char*)MemAlloc((unsigned int)(n * 4));

    std::memcpy(malla.vertices, g.posiciones.data(), (size_t)n * 3 * sizeof(float));
    std::memcpy(malla.colors, g.colores.data(), (size_t)n * 4);

    for (int i = 0; i < n; i++)
    {
        malla.texcoords[i * 2] = g.causticas[(size_t)i];
        malla.texcoords[i * 2 + 1] = g.modoShader;
    }

    UploadMesh(&malla, false);
    g.posiciones.clear();
    g.colores.clear();
    g.causticas.clear();
    return true;
}


static void LiberarMallaHornoPesca(Mesh& malla, bool& valida)
{
    if (valida) UnloadMesh(malla);
    valida = false;
    malla = {};
}


// Libera las mallas horneadas (se vuelven a grabar al dibujar).
static void LiberarMallasPesca()
{
    LiberarMallaHornoPesca(g_hornoPesca.decorado, g_hornoPesca.decoradoValido);
    LiberarMallaHornoPesca(g_hornoPesca.cielo, g_hornoPesca.cieloValido);
    LiberarMallaHornoPesca(g_hornoPesca.nubes, g_hornoPesca.nubesValidas);
    LiberarMallaHornoPesca(g_hornoPesca.agua, g_hornoPesca.aguaValida);

    for (int k = 0; k < CANTIDAD_PALMERAS_PESCA; k++)
    {
        LiberarMallaHornoPesca(g_hornoPesca.palmeras[k].malla, g_hornoPesca.palmeras[k].valida);
    }

    g_hornoPesca.listo = false;
    g_hornoPesca.clave = -1;
}


static int ClaveHornoPesca()
{
    return (int)CalidadDecoracion() | (CalidadEfectos() != CALIDAD_BAJA ? 8 : 0);
}


// Graba lo estatico para la calidad actual (solo si cambio). El brillo del
// mar se calcula desde la camara de juego.
static void HornearPesca(const MinijuegoPescaIsla& m, int limite)
{
    int clave = ClaveHornoPesca();

    if (g_hornoPesca.listo && g_hornoPesca.clave == clave)
    {
        return;
    }

    if (!IsWindowReady())
    {
        return;
    }

    LiberarMallasPesca();

    if (!g_hornoPesca.shaderListo)
    {
        g_hornoPesca.shader = LoadShaderFromMemory(VS_HORNO_PESCA, FS_HORNO_PESCA);
        g_hornoPesca.locOjo = GetShaderLocation(g_hornoPesca.shader, "ojo");
        g_hornoPesca.locNiebla = GetShaderLocation(g_hornoPesca.shader, "niebla");
        g_hornoPesca.locTiempo = GetShaderLocation(g_hornoPesca.shader, "tiempo");
        g_hornoPesca.locOndas = GetShaderLocation(g_hornoPesca.shader, "ondas");
        g_hornoPesca.material = LoadMaterialDefault();
        g_hornoPesca.material.shader = g_hornoPesca.shader;
        g_hornoPesca.shaderListo = true;
    }

    if (!IsShaderValid(g_hornoPesca.shader))
    {
        return;
    }

    bool decoracion = CalidadDecoracion() != CALIDAD_BAJA;
    bool efectos = CalidadEfectos() != CALIDAD_BAJA;
    int paso = CalidadDecoracion() == CALIDAD_ALTA ? 1 : 2;
    Vector3 ojo = ojoPesca;
    ojoPesca = m.camara.position;
    g_partePesca = PARTE_PESCA_ESTATICA;

    EmpezarGrabacionPesca();
    DibujarDecoradoPesca(m, limite, 0.0f, efectos, decoracion, paso);
    g_hornoPesca.decoradoValido = TerminarGrabacionPesca(g_hornoPesca.decorado);

    // Cada palmera quieta (sin balanceo): el viento la inclina entera.
    for (int k = 0; k < CANTIDAD_PALMERAS_PESCA; k += (decoracion ? paso : 3))
    {
        PalmeraHornoPesca& p = g_hornoPesca.palmeras[k];
        const PalmeraPescaDatos& datos = PALMERAS_PESCA[k];
        float a = datos.angulo * DEG2RAD;
        p.base = { std::cos(a) * datos.radio, 0.0f, std::sin(a) * datos.radio };
        p.base.y = AlturaEnPesca(p.base.x, p.base.z) - 0.05f;
        p.alto = datos.alto;
        p.semilla = 40 + k;
        EmpezarGrabacionPesca();
        PalmeraPesca(datos, p.semilla, 0.0f, false);
        p.valida = TerminarGrabacionPesca(p.malla);
    }

    // Superficie de la laguna: brillo desde la camara de juego; las ondas
    // las anima el shader.
    EmpezarGrabacionPesca();
    g_grabacionPesca.modoShader = 2.0f;
    DibujarSuperficieLagunaPesca(0.0f, false);
    g_hornoPesca.aguaValida = TerminarGrabacionPesca(g_hornoPesca.agua);

    // Cielo y nubes relativos al ojo (se dibujan trasladados a el), con las
    // nubes de cara a la vista de la entrada, que es la que mira el cielo.
    Vector3 derecha = derechaVistaPesca;
    Vector3 arriba = arribaVistaPesca;
    Vector3 frente = NormalizarPesca(RestaPesca(MIRA_INTRO_PESCA, OJO_INTRO_PESCA));
    derechaVistaPesca = NormalizarPesca(CruzPesca(frente, { 0.0f, 1.0f, 0.0f }));
    arribaVistaPesca = CruzPesca(derechaVistaPesca, frente);
    ojoPesca = { 0.0f, 0.0f, 0.0f };

    EmpezarGrabacionPesca();
    g_grabacionPesca.modoShader = 1.0f;
    DibujarCupulaPesca();
    g_hornoPesca.cieloValido = TerminarGrabacionPesca(g_hornoPesca.cielo);

    if (decoracion)
    {
        EmpezarGrabacionPesca();
        g_grabacionPesca.modoShader = 1.0f;
        DibujarNubesPesca(0.0f, false, paso);
        g_hornoPesca.nubesValidas = TerminarGrabacionPesca(g_hornoPesca.nubes);
    }

    derechaVistaPesca = derecha;
    arribaVistaPesca = arriba;
    g_partePesca = PARTE_PESCA_TODO;
    ojoPesca = ojo;
    g_hornoPesca.clave = clave;
    g_hornoPesca.listo = true;
}


// Vacia lo pendiente de rlgl (para respetar el orden) y fija los uniformes.
static void UsarShaderHornoPesca(float t, bool efectos)
{
    rlDrawRenderBatchActive();
    float ojo[3] = { ojoPesca.x, ojoPesca.y, ojoPesca.z };
    float niebla[4] = { COLOR_NIEBLA_PESCA.r / 255.0f, COLOR_NIEBLA_PESCA.g / 255.0f, COLOR_NIEBLA_PESCA.b / 255.0f, 1.0f };
    float tiempo = efectos ? t : 0.0f;
    float ondas = efectos ? 1.0f : 0.0f;
    SetShaderValue(g_hornoPesca.shader, g_hornoPesca.locOjo, ojo, SHADER_UNIFORM_VEC3);
    SetShaderValue(g_hornoPesca.shader, g_hornoPesca.locNiebla, niebla, SHADER_UNIFORM_VEC4);
    SetShaderValue(g_hornoPesca.shader, g_hornoPesca.locTiempo, &tiempo, SHADER_UNIFORM_FLOAT);
    SetShaderValue(g_hornoPesca.shader, g_hornoPesca.locOndas, &ondas, SHADER_UNIFORM_FLOAT);
}


// Cupula, sol y nubes (sin escribir profundidad).
static void DibujarCieloHornoPesca(float t, bool efectos)
{
    Matrix enOjo = MatrixTranslate(ojoPesca.x, ojoPesca.y, ojoPesca.z);
    UsarShaderHornoPesca(t, efectos);
    if (g_hornoPesca.cieloValido) DrawMesh(g_hornoPesca.cielo, g_hornoPesca.material, enOjo);
    DibujarSolPesca();
    UsarShaderHornoPesca(t, efectos);
    if (g_hornoPesca.nubesValidas) DrawMesh(g_hornoPesca.nubes, g_hornoPesca.material, enOjo);
}


static bool DibujarAguaHorneadaPesca(float t, bool efectos)
{
    if (!g_hornoPesca.listo || !g_hornoPesca.aguaValida)
    {
        return false;
    }

    UsarShaderHornoPesca(t, efectos);
    DrawMesh(g_hornoPesca.agua, g_hornoPesca.material, MatrixIdentity());
    return true;
}


static void DibujarHornoPesca(float t, bool efectos)
{
    UsarShaderHornoPesca(t, efectos);

    // Palmeras primero: tapan parte de la isla y la prueba de profundidad
    // descarta lo que queda detras.
    for (int k = 0; k < CANTIDAD_PALMERAS_PESCA; k++)
    {
        const PalmeraHornoPesca& p = g_hornoPesca.palmeras[k];
        if (!p.valida) continue;

        // Mismo vaiven que la copa de PalmeraPesca, como giro desde el pie.
        float balanceo = efectos ? std::sin(t * 0.9f + (float)p.semilla) * 0.05f : 0.0f;
        Matrix giro = MatrixMultiply(
            MatrixMultiply(MatrixTranslate(-p.base.x, -p.base.y, -p.base.z), MatrixRotateZ(-std::asin(balanceo))),
            MatrixTranslate(p.base.x, p.base.y, p.base.z));
        DrawMesh(p.malla, g_hornoPesca.material, giro);
    }

    if (g_hornoPesca.decoradoValido)
    {
        DrawMesh(g_hornoPesca.decorado, g_hornoPesca.material, MatrixIdentity());
    }
}



//==================================================
// DIBUJO
//==================================================

void MinijuegoPescaIsla::Dibujar(
    const JugadorPrueba jugadores[],
    int cantidadMaxima,
    const Participante participantes[],
    bool mostrarDebug
) const
{
    int limite = LimitePesca(cantidadMaxima);
    const float t = tiempoAnimacion;
    bool decoracion = CalidadDecoracion() != CALIDAD_BAJA;
    bool efectos = CalidadEfectos() != CALIDAD_BAJA;
    int paso = CalidadDecoracion() == CALIDAD_ALTA ? 1 : 2;

    SeleccionarTemaVisualMinijuego(TEMA_VISUAL_NINGUNO);

    // Entrada: durante la cuenta la camara baja del horizonte al juego.
    Camera3D vista = camara;

    if (fase == FASE_PESCA_PREPARACION && resultado.estado == RESULTADO_MINIJUEGO_EN_CURSO)
    {
        float u = Limitar01Pesca((tiempoPreparacion - 0.5f) / (DURACION_PREPARACION_PESCA - 0.5f));
        float s = SuavePesca(u) * ObtenerFactorMovimientoCamaraMinijuegos();
        vista.position = LerpPesca(camara.position, OJO_INTRO_PESCA, s);
        vista.target = LerpPesca(camara.target, MIRA_INTRO_PESCA, s);
        vista.fovy = camara.fovy + (52.0f - camara.fovy) * s;
    }

    ojoPesca = vista.position;
    Vector3 frente = NormalizarPesca(RestaPesca(vista.target, vista.position));
    derechaVistaPesca = NormalizarPesca(CruzPesca(frente, { 0.0f, 1.0f, 0.0f }));
    arribaVistaPesca = CruzPesca(derechaVistaPesca, frente);

    // Lo estatico se graba una vez por calidad (mallas con niebla por pixel).
    HornearPesca(*this, limite);

    // El bucle del juego no limpia la pantalla: limpiar antes del cielo.
    ClearBackground(COLOR_HORIZONTE_PESCA);

    BeginMode3D(vista);
    rlDisableBackfaceCulling();


    // Decorado: horneado + lo animado, o todo inmediato si no hay horno.
    if (g_hornoPesca.listo)
    {
        DibujarHornoPesca(t, efectos);
        g_partePesca = PARTE_PESCA_ANIMADA;
        DibujarDecoradoPesca(*this, limite, t, efectos, decoracion, paso);
        g_partePesca = PARTE_PESCA_TODO;
    }
    else
    {
        DibujarDecoradoPesca(*this, limite, t, efectos, decoracion, paso);
    }

    // Peces y botas bajo el agua.
    for (int f = 0; f < MAX_PECES_PESCA; f++)
    {
        if (peces[f].estado != ESTADO_PEZ_AUSENTE)
        {
            DibujarPezPesca(peces[f], t);
        }
    }

    // Pescadores, canas, corchos, mira y sedales.
    for (int i = 0; i < limite; i++)
    {
        if (!participa[i])
        {
            continue;
        }

        Participante visual = participantes[i];
        visual.conectado = true;
        visual.color = coloresJugadores[i];
        DibujarJugadorCuboPrueba(jugadores[i], visual);

        if (mostrarDebug)
        {
            DrawBoundingBox(CrearHitboxJugadorPrueba(jugadores[i]), LIME);
            DrawCircle3D({ estados[i].anzueloX, 0.2f, estados[i].anzueloZ }, RADIO_PIQUE_PESCA, { 1.0f, 0.0f, 0.0f }, 90.0f, LIME);
        }
    }

    DibujarCanasCorchosPesca(*this, limite);
    DibujarSedalesPesca(*this, limite, t);
    DibujarVuelosPesca(*this, limite);
    DibujarParticulasSolidasPesca(*this);

    if (decoracion)
    {
        DibujarGaviotasPesca(t, efectos);
    }

    // Cielo despues de lo opaco y antes de lo translucido: la prueba de
    // profundidad descarta lo que tapan la isla y el mar (sin escribirla).
    rlDrawRenderBatchActive();
    rlDisableDepthMask();
    if (g_hornoPesca.listo)
    {
        DibujarCieloHornoPesca(t, efectos);
    }
    else
    {
        DibujarCupulaPesca();
        DibujarSolPesca();
        if (decoracion) DibujarNubesPesca(t, efectos, paso);
    }
    rlDrawRenderBatchActive();
    rlEnableDepthMask();

    DibujarTranslucidosPesca(*this, limite, t, efectos, decoracion);

    if (efectos)
    {
        DibujarLucesPesca(*this, limite, t, decoracion);
    }

    rlEnableBackfaceCulling();
    EndMode3D();

    DibujarAvisosPesca(*this, jugadores, participantes, limite, vista);

    if (resultado.estado == RESULTADO_MINIJUEGO_CANCELADO)
    {
        DibujarTextoMundoContornoUI("SE NECESITAN AL MENOS 2 JUGADORES", (float)GetScreenWidth() * 0.5f, (float)GetScreenHeight() * 0.42f, UI_TEXTO_GRANDE, Color{ 255, 120, 110, 255 });
    }

    // HUD comun.
    DatosHudMinijuego hud;
    PrepararDatosHudMinijuego(
        hud,
        "PESCA ISLEÑA",
        "LANZÁ, ENGANCHÁ AL PICAR Y NO CORTES LA LÍNEA. ¡OJO CON LAS BOTAS!",
        resultado
    );

    hud.fase = fase == FASE_PESCA_PREPARACION
        ? HUD_MINIJUEGO_CUENTA
        : (fase == FASE_PESCA_TERMINADO ? HUD_MINIJUEGO_FIN : HUD_MINIJUEGO_JUGANDO);
    hud.cuentaRestante = tiempoPreparacion;
    hud.reloj = HUD_RELOJ_CUENTA_ATRAS;
    hud.tiempoRestante = tiempoRestante;
    hud.tiempoTotal = DURACION_PARTIDA_PESCA;
    hud.segundosAlerta = 10.0f;
    hud.finPorTiempo = true;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        JugadorHudMinijuego& h = hud.jugadores[i];

        if (!h.visible || i >= limite)
        {
            continue;
        }

        const EstadoJugadorPesca& e = estados[i];

        std::snprintf(h.valor, sizeof(h.valor), e.puntos == 1 ? "%d PT" : "%d PTS", e.puntos);
        h.valorNumerico = e.puntos;
        std::snprintf(h.detalleFinal, sizeof(h.detalleFinal), e.puntos == 1 ? "%d PUNTO" : "%d PUNTOS", e.puntos);
        std::snprintf(h.detalleDesempate, sizeof(h.detalleDesempate), "MEJOR PEZ: %d", e.mejorValor);

        if (fase == FASE_PESCA_JUGANDO)
        {
            const char* texto = "APUNTÁ Y LANZÁ";

            if (e.estado == PESCA_LIBRE && e.bloqueo > 0.0f) texto = "SIN CARNADA";
            else if (e.estado == PESCA_LANZANDO) texto = "LANZANDO";
            else if (e.estado == PESCA_ESPERANDO) texto = "ESPERÁ EL PIQUE";
            else if (e.estado == PESCA_PICADA) texto = "¡PICÓ! ¡YA!";
            else if (e.estado == PESCA_TENSION)
            {
                if (e.recogiendo) texto = e.tension > 0.72f ? "¡AFLOJÁ!" : "RECOGIENDO";
                else texto = e.tension < 0.3f ? "¡RECOGÉ!" : "AFLOJANDO";
                h.progreso = Limitar01Pesca(e.progreso);
            }

            std::snprintf(h.estado, sizeof(h.estado), "%s", texto);
        }

        if (!participantes[i].esBot && participantes[i].conectado)
        {
            std::snprintf(
                h.controles,
                sizeof(h.controles),
                "%s: APUNTAR\n%s: LANZAR / TIRAR",
                TextoMoverHud(participantes[i]),
                TextoBotonGolpeHud(participantes[i])
            );
        }
    }

    DibujarHudMinijuego(hud, participantes);
}


const ResultadoMinijuego& MinijuegoPescaIsla::ObtenerResultado() const
{
    return resultado;
}
