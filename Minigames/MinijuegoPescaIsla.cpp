#include "Minigames/MinijuegoPescaIsla.h"

#include "Minigames/AudioMinijuegos.h"
#include "Minigames/MecanicasJugador.h"
#include "Minigames/ModelosEscenariosRetro3D.h"
#include "Minigames/UtilidadesMinijuegos.h"
#include "Systems/Input.h"

#include "raylib.h"

#include <cmath>


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


static float HashPesca(int k, int semilla)
{
    float v = std::sin((float)k * 12.9898f + (float)semilla * 78.233f) * 43758.5453f;
    return v - std::floor(v);
}


static int LimitePesca(int cantidadMaxima)
{
    return cantidadMaxima < MAX_PARTICIPANTES
        ? cantidadMaxima
        : MAX_PARTICIPANTES;
}


static const char* NombreJugadorPesca(const Participante& participante, int indice)
{
    return TextFormat(
        "J%d%s",
        participante.numeroJugador > 0 ? participante.numeroJugador : indice + 1,
        participante.esBot ? " BOT" : ""
    );
}


static const char* TextoGolpePesca(const Participante& participante)
{
    if (participante.esBot) return "BOT";
    if (participante.control == CONTROL_GAMEPAD) return "B";
    if (participante.control == CONTROL_TECLADO_FLECHAS) return "SHIFT DER";
    return "E";
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

    // Camara desplazada al sur para que los muelles queden sobre las tarjetas del HUD.
    camara.position = { 0.0f, 24.0f, 6.7f };
    camara.target = { 0.0f, 0.0f, 1.7f };
    camara.up = { 0.0f, 1.0f, 0.0f };
    camara.fovy = 64.0f;
    camara.projection = CAMERA_PERSPECTIVE;

    fase = FASE_PESCA_PREPARACION;
    tiempoPreparacion = DURACION_PREPARACION_PESCA;
    tiempoRestante = DURACION_PARTIDA_PESCA;
    tiempoAnimacion = 0.0f;
}


void MinijuegoPescaIsla::Reiniciar(
    JugadorPrueba jugadores[],
    Participante participantes[],
    int cantidadMaxima
)
{
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

    CargarPaquetePescaIslenaRetro3D();
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

void MinijuegoPescaIsla::Actualizar(
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
// VISUAL: ISLA TROPICAL (solo decoracion, la logica no depende de esto)
//==================================================

// GLB compartidos; cada pieza conserva sus primitivas si falla la carga.
// Los parentesis en fallbacks evitan sombras automaticas en Y=0 para
// arrecife, peces sumergidos, humo y gaviotas de SombrasRetro.h.

static float GiroMuellePesca(int muelle)
{
    const float giros[] = {0,180,-90,90};
    return giros[muelle % 4];
}

static void DibujarEntornoPesca(const MinijuegoPescaIsla& m)
{
    float t = m.tiempoAnimacion;

    // Oceano, arena y fondo de la laguna.
    if (!DibujarModeloPescaIslenaRetro3D(MODELO_PESCA_OCEANO,{0,0,0}))
        DrawPlane({ 0.0f, -0.08f, 0.0f }, { 120.0f, 120.0f }, Color{ 20, 110, 150, 255 });
    if (!DibujarModeloPescaIslenaRetro3D(MODELO_PESCA_ISLA,{0,0,0}))
    {
        (DrawCylinder)({ 0.0f, -0.06f, 0.0f }, 15.0f, 15.0f, 0.08f, 40, Color{ 238, 222, 170, 255 });
        (DrawCylinder)({ 0.0f, PROFUNDIDAD_PESCA, 0.0f }, RADIO_LAGUNA_PESCA, RADIO_LAGUNA_PESCA, 0.05f, 36, Color{ 214, 196, 150, 255 });
        (DrawCylinder)({ 0.0f, PROFUNDIDAD_PESCA, 0.0f }, RADIO_LAGUNA_PESCA + 0.01f, RADIO_LAGUNA_PESCA + 0.01f, 1.8f, 36, Color{ 30, 120, 140, 255 });
    }

    // Arrecife de coral visible bajo el agua.
    for (int k = 0; k < 16; k++)
    {
        float angulo = HashPesca(k, 1) * 2.0f * PI;
        float radio = 2.0f + HashPesca(k, 2) * 5.8f;
        float x = std::cos(angulo) * radio;
        float z = std::sin(angulo) * radio;
        Color color = ColorFromHSV(HashPesca(k, 3) * 60.0f + (k % 2 == 0 ? 300.0f : 0.0f), 0.65f, 0.95f);
        float altura = 0.5f + HashPesca(k, 4) * 0.6f;

        if (DibujarModeloPescaIslenaRetro3D(k % 2 == 0 ? MODELO_PESCA_CORAL_1 : MODELO_PESCA_CORAL_2,
            {x,PROFUNDIDAD_PESCA,z},0,{0,1,0},{1,altura/.91f,1})) continue;

        (DrawCylinder)({ x, PROFUNDIDAD_PESCA, z }, 0.12f, 0.2f, altura, 6, color);
        DrawSphereEx({ x, PROFUNDIDAD_PESCA + altura, z }, 0.28f, 6, 6, color);
        (DrawCylinder)({ x + 0.3f, PROFUNDIDAD_PESCA, z + 0.2f }, 0.08f, 0.14f, altura * 0.7f, 6, color);
    }

    // Muelles de bambu.
    for (int d = 0; d < 4; d++)
    {
        Vector3 p = PosicionMuellePesca(d);
        if (DibujarModeloPescaIslenaRetro3D(MODELO_PESCA_MUELLE,{p.x,0,p.z},GiroMuellePesca(d))) continue;
        bool lateral = d >= 2;
        float ancho = lateral ? 3.0f : 3.4f;
        float largo = lateral ? 3.4f : 3.0f;

        (DrawCube)({ p.x, ALTURA_MUELLE_PESCA - 0.1f, p.z }, ancho, 0.2f, largo, Color{ 214, 180, 90, 255 });

        for (int tabla = 0; tabla < 6; tabla++)
        {
            float desplazamiento = ((float)tabla - 2.5f) * 0.55f;
            Vector3 centro = { p.x + (lateral ? 0.0f : desplazamiento), ALTURA_MUELLE_PESCA + 0.02f, p.z + (lateral ? desplazamiento : 0.0f) };
            (DrawCube)(centro, lateral ? ancho : 0.5f, 0.05f, lateral ? 0.5f : largo, (tabla % 2 == 0) ? Color{ 230, 200, 110, 255 } : Color{ 200, 165, 80, 255 });
        }

        for (int sx = -1; sx <= 1; sx += 2)
        {
            for (int sz = -1; sz <= 1; sz += 2)
            {
                (DrawCylinder)({ p.x + (float)sx * ancho * 0.45f, -1.0f, p.z + (float)sz * largo * 0.45f }, 0.12f, 0.12f, 1.6f, 6, Color{ 150, 160, 70, 255 });
            }
        }
    }

    // Palmeras.
    for (int k = 0; k < 8; k++)
    {
        float angulo = ((float)k * 45.0f + 22.5f) * DEG2RAD;
        float x = std::cos(angulo) * 13.2f;
        float z = std::sin(angulo) * 13.2f;
        float balanceo = std::sin(t * 1.2f + (float)k) * 0.1f;
        Vector3 copa = { x + balanceo, 4.2f, z };

        if (DibujarModeloPescaIslenaRetro3D(MODELO_PESCA_PALMERA,{x,0,z},
            -std::atan2(balanceo,4.2f)*RAD2DEG,{0,0,1})) continue;

        DrawCylinderEx({ x, 0.0f, z }, copa, 0.28f, 0.18f, 6, Color{ 130, 92, 56, 255 });

        for (int h = 0; h < 5; h++)
        {
            float a = (float)h * 2.0f * PI / 5.0f + (float)k;
            DrawCylinderEx(copa, { copa.x + std::cos(a) * 1.9f, copa.y - 0.6f, copa.z + std::sin(a) * 1.9f }, 0.18f, 0.02f, 4, Color{ 50, 160, 70, 255 });
        }

        DrawSphereEx({ copa.x, copa.y - 0.2f, copa.z }, 0.2f, 5, 5, Color{ 110, 70, 40, 255 });
    }

    // Barca anclada.
    if (!DibujarModeloPescaIslenaRetro3D(MODELO_PESCA_BARCA,{-12,0,-11}))
    {
        (DrawCube)({ -12.0f, 0.1f, -11.0f }, 3.2f, 0.6f, 1.2f, Color{ 160, 90, 50, 255 });
        (DrawCylinder)({ -12.0f, 0.3f, -11.0f }, 0.06f, 0.06f, 2.6f, 5, Color{ 110, 80, 50, 255 });
        (DrawCube)({ -11.7f, 1.7f, -11.0f }, 1.3f, 1.5f, 0.05f, Color{ 250, 245, 230, 255 });
    }

    // Volcan lejano con humo.
    if (!DibujarModeloPescaIslenaRetro3D(MODELO_PESCA_VOLCAN,{0,0,-21}))
    {
        (DrawCylinder)({ 0.0f, 0.0f, -21.0f }, 1.6f, 8.0f, 7.0f, 14, Color{ 90, 70, 60, 255 });
        DrawSphereEx({ 0.0f, 7.0f, -21.0f }, 1.3f, 8, 8, Color{ 235, 90, 30, 255 });
    }

    if (!DibujarModeloAnimadoPescaIslenaRetro3D(MODELO_PESCA_HUMO,{0,7,-21},0,t))
    {
        for (int h = 0; h < 4; h++)
        {
            float fase = std::fmod(t * 0.25f + (float)h * 0.25f, 1.0f);
            DrawSphereEx({ 0.5f * std::sin(t + (float)h), 7.8f + fase * 4.0f, -21.0f }, 0.6f + fase * 0.9f, 6, 6, Fade(Color{ 120, 120, 130, 255 }, 0.7f * (1.0f - fase)));
        }
    }

    // Gaviotas.
    for (int k = 0; k < 4; k++)
    {
        float angulo = t * (0.3f + 0.06f * (float)k) + (float)k * 1.6f;
        Vector3 p = { std::cos(angulo) * 11.0f, 5.5f + (float)k * 0.6f + std::sin(t * 2.0f + (float)k) * 0.3f, std::sin(angulo) * 11.0f };
        float aleteo = std::sin(t * 8.0f + (float)k) * 0.3f;

        if (DibujarModeloAnimadoPescaIslenaRetro3D(MODELO_PESCA_GAVIOTA,p,
            180-angulo*RAD2DEG,aleteo)) continue;

        DrawLine3D({ p.x - 0.45f, p.y + aleteo, p.z }, p, WHITE);
        DrawLine3D(p, { p.x + 0.45f, p.y + aleteo, p.z }, WHITE);
    }
}


static void DibujarPezPesca(const PezPesca& pez, float tiempo)
{
    float y = -0.9f;
    Color sombra = Fade(Color{ 10, 40, 60, 255 }, 0.75f);
    float radio = 0.28f;
    bool modelo = pez.tipo == PEZ_PESCA_BOTA
        ? DibujarModeloPescaIslenaRetro3D(MODELO_PESCA_BOTA,{pez.x,y,pez.z},-std::atan2(pez.dirZ,pez.dirX)*RAD2DEG)
        : DibujarModeloAnimadoPescaIslenaRetro3D((ModeloPescaIslena3D)(MODELO_PESCA_PEQUENO+pez.tipo),
            {pez.x,y,pez.z},-std::atan2(pez.dirZ,pez.dirX)*RAD2DEG,std::sin(tiempo*10+pez.semilla)*.15f);

    if (!modelo)
    {
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

        // Cola en abanico (ambas caras para evitar el descarte de caras traseras).
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

    if (pez.tipo == PEZ_PESCA_RARO)
    {
        for (int s = 0; s < 3; s++)
        {
            float angulo = tiempo * 3.0f + (float)s * 2.1f;
            DrawSphereEx({ pez.x + std::cos(angulo) * 0.7f, y + 0.1f, pez.z + std::sin(angulo) * 0.7f }, 0.06f, 4, 4, Color{ 255, 240, 140, 255 });
        }
    }
}


static void DibujarAguaPesca(const MinijuegoPescaIsla& m)
{
    // Superficie translucida: se dibuja despues de lo que hay debajo.
    // El GLB tiene fondo y ondas, sin tapa opaca que esconda los peces.
    if (!DibujarModeloPescaIslenaRetro3D(MODELO_PESCA_AGUA,{0,0,0}))
        (DrawCylinder)({ 0.0f, 0.0f, 0.0f }, RADIO_LAGUNA_PESCA, RADIO_LAGUNA_PESCA, 0.05f, 40, Fade(Color{ 60, 210, 215, 255 }, 0.55f));

    for (int k = 0; k < 3; k++)
    {
        float fase = std::fmod(m.tiempoAnimacion * 0.4f + (float)k * 0.33f, 1.0f);
        DrawCircle3D({ 0.0f, 0.07f, 0.0f }, 2.0f + fase * 6.0f, { 1.0f, 0.0f, 0.0f }, 90.0f, Fade(WHITE, 0.25f * (1.0f - fase)));
    }
}


static void DibujarLineasPesca(
    const MinijuegoPescaIsla& m,
    int limite
)
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

        // Escala Y explicita: punta GLB (0,1.6,-1.4) conecta con PuntaCanaPesca.
        if (!DibujarModeloPescaIslenaRetro3D(MODELO_PESCA_CANA,{mano.x,mano.y+1.1f,mano.z},
            GiroMuellePesca(m.muelleDe[i]),{0,1,0},{1,1.25f/1.6f,1}))
            DrawCylinderEx({ mano.x, mano.y + 1.1f, mano.z }, punta, 0.06f, 0.03f, 5, Color{ 120, 84, 50, 255 });

        if (estado.estado == PESCA_LIBRE)
        {
            float pulso = 0.5f + 0.5f * std::sin(m.tiempoAnimacion * 6.0f);
            float escala = (.45f+.1f*pulso)/.48f;
            if (DibujarModeloPescaIslenaRetro3D(MODELO_PESCA_CURSOR,{estado.cursorX,.09f,estado.cursorZ},
                0,{0,1,0},{escala,1,escala},color)) continue;
            DrawCircle3D({ estado.cursorX, 0.09f, estado.cursorZ }, 0.45f + 0.1f * pulso, { 1.0f, 0.0f, 0.0f }, 90.0f, color);
            DrawCircle3D({ estado.cursorX, 0.09f, estado.cursorZ }, 0.25f, { 1.0f, 0.0f, 0.0f }, 90.0f, WHITE);
            DrawLine3D({ estado.cursorX - 0.7f, 0.09f, estado.cursorZ }, { estado.cursorX + 0.7f, 0.09f, estado.cursorZ }, Fade(color, 0.7f));
            DrawLine3D({ estado.cursorX, 0.09f, estado.cursorZ - 0.7f }, { estado.cursorX, 0.09f, estado.cursorZ + 0.7f }, Fade(color, 0.7f));
            continue;
        }

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
            DrawCircle3D({ anzuelo.x, 0.08f, anzuelo.z }, 0.3f + 0.15f * std::sin(m.tiempoAnimacion * 2.0f), { 1.0f, 0.0f, 0.0f }, 90.0f, Fade(WHITE, 0.6f));
        }

        DrawLine3D(punta, anzuelo, Fade(WHITE, 0.9f));
        if (!DibujarModeloPescaIslenaRetro3D(MODELO_PESCA_CORCHO,anzuelo,0,{0,1,0},{1,1,1},color))
        {
            DrawSphereEx(anzuelo, 0.17f, 6, 6, Color{ 240, 240, 240, 255 });
            DrawSphereEx({ anzuelo.x, anzuelo.y + 0.12f, anzuelo.z }, 0.13f, 6, 6, color);
        }

        if (estado.estado == PESCA_PICADA && std::fmod(m.tiempoAnimacion * 12.0f, 1.0f) < 0.7f)
        {
            (DrawCylinder)({ anzuelo.x, anzuelo.y + 0.7f, anzuelo.z }, 0.1f, 0.1f, 0.7f, 6, RED);
            DrawSphereEx({ anzuelo.x, anzuelo.y + 0.45f, anzuelo.z }, 0.12f, 6, 6, RED);
        }
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
    int anchoPantalla = GetScreenWidth();
    int altoPantalla = GetScreenHeight();

    ClearBackground(Color{ 130, 205, 240, 255 });
    BeginMode3D(camara);

    DibujarEntornoPesca(*this);

    for (int f = 0; f < MAX_PECES_PESCA; f++)
    {
        if (peces[f].estado != ESTADO_PEZ_AUSENTE)
        {
            DibujarPezPesca(peces[f], tiempoAnimacion);
        }
    }

    DibujarAguaPesca(*this);
    DibujarLineasPesca(*this, limite);

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

    EndMode3D();

    // Etiquetas y avisos sobre los jugadores.
    for (int i = 0; i < limite; i++)
    {
        if (!participa[i])
        {
            continue;
        }

        Vector2 pantalla = GetWorldToScreen(
            { jugadores[i].posicion.x, jugadores[i].posicion.y + 1.35f, jugadores[i].posicion.z },
            camara
        );
        const char* nombre = NombreJugadorPesca(participantes[i], i);
        int ancho = MeasureText(nombre, 18);

        // La etiqueta se corre hacia el lago para no tapar al jugador (vista cenital).
        Vector2 centroPantalla = GetWorldToScreen({ 0.0f, 0.0f, 0.0f }, camara);
        Vector2 haciaCentro = { centroPantalla.x - pantalla.x, centroPantalla.y - pantalla.y };
        float largoCentro = std::sqrt(haciaCentro.x * haciaCentro.x + haciaCentro.y * haciaCentro.y);

        if (largoCentro > 1.0f)
        {
            pantalla.x += haciaCentro.x / largoCentro * 52.0f;
            pantalla.y += haciaCentro.y / largoCentro * 40.0f - 10.0f;
        }

        DrawRectangle((int)pantalla.x - ancho / 2 - 4, (int)pantalla.y - 2, ancho + 8, 22, Fade(BLACK, 0.55f));
        DrawText(nombre, (int)pantalla.x - ancho / 2, (int)pantalla.y, 18, coloresJugadores[i]);

        if (estados[i].popupTiempo > 0.0f && estados[i].popupValor >= 0)
        {
            const char* texto = TextFormat("+%d", estados[i].popupValor);
            DrawText(texto, (int)pantalla.x - MeasureText(texto, 30) / 2, (int)pantalla.y - 34, 30, estados[i].popupValor >= 5 ? GOLD : LIME);
        }
    }

    // Cabecera.
    DrawRectangle(14, 12, 650, 66, Fade(BLACK, 0.74f));
    DrawText("PESCA ISLENA", 28, 18, 28, GOLD);
    DrawText("APUNTA: WASD/FLECHAS/STICK   ACCION: LANZAR / ENGANCHAR / ALTERNAR TIRON", 28, 52, 14, RAYWHITE);

    if (fase == FASE_PESCA_JUGANDO)
    {
        DrawRectangle(anchoPantalla - 214, 12, 200, 44, Fade(BLACK, 0.74f));
        DrawText(
            TextFormat("TIEMPO %.0f", tiempoRestante > 0.0f ? tiempoRestante : 0.0f),
            anchoPantalla - 200,
            22,
            26,
            tiempoRestante <= 6.0f ? RED : GOLD
        );
    }

    // Tarjetas inferiores.
    int activos = 0;

    for (int i = 0; i < limite; i++)
    {
        if (participa[i]) activos++;
    }

    if (activos > 0)
    {
        int separacion = 10;
        int anchoTarjeta = (anchoPantalla - 28 - separacion * (activos - 1)) / activos;

        if (anchoTarjeta > 300) anchoTarjeta = 300;

        int xInicial = (anchoPantalla - (anchoTarjeta * activos + separacion * (activos - 1))) / 2;
        int k = 0;

        for (int i = 0; i < limite; i++)
        {
            if (!participa[i])
            {
                continue;
            }

            int x = xInicial + k * (anchoTarjeta + separacion);
            int y = altoPantalla - 86;
            const EstadoJugadorPesca& estado = estados[i];

            DrawRectangle(x, y, anchoTarjeta, 76, Fade(BLACK, 0.78f));
            DrawRectangle(x, y, 8, 76, coloresJugadores[i]);
            DrawText(NombreJugadorPesca(participantes[i], i), x + 18, y + 6, 18, coloresJugadores[i]);
            DrawText(TextFormat("%d PTS", estado.puntos), x + anchoTarjeta - 90, y + 6, 20, RAYWHITE);

            const char* texto = "APUNTA Y LANZA";
            Color colorTexto = LIGHTGRAY;

            if (estado.bloqueo > 0.0f && estado.estado == PESCA_LIBRE)
            {
                texto = TextFormat("SIN CARNADA %.1f", estado.bloqueo);
                colorTexto = ORANGE;
            }
            else if (estado.estado == PESCA_LANZANDO)
            {
                texto = "LANZANDO";
            }
            else if (estado.estado == PESCA_ESPERANDO)
            {
                texto = "ESPERA EL PIQUE...";
                colorTexto = SKYBLUE;
            }
            else if (estado.estado == PESCA_PICADA)
            {
                texto = "PICO! ACCION YA!";
                colorTexto = RED;
            }
            else if (estado.estado == PESCA_TENSION)
            {
                texto = estado.recogiendo ? "RECOGIENDO (ACCION: SOLTAR)" : "AFLOJANDO (ACCION: RECOGER)";
                colorTexto = estado.recogiendo ? YELLOW : LIME;
            }

            DrawText(texto, x + 18, y + 30, 14, colorTexto);

            if (estado.estado == PESCA_TENSION)
            {
                int barra = anchoTarjeta - 36;
                DrawRectangle(x + 18, y + 50, barra, 8, Fade(WHITE, 0.2f));
                DrawRectangle(x + 18, y + 50, (int)((float)barra * LimitarPesca(estado.tension, 0.0f, 1.0f)), 8, estado.tension > 0.8f ? RED : ORANGE);
                DrawRectangle(x + 18, y + 62, barra, 6, Fade(WHITE, 0.2f));
                DrawRectangle(x + 18, y + 62, (int)((float)barra * estado.progreso), 6, LIME);
            }
            else if (!participantes[i].esBot)
            {
                DrawText(
                    TextFormat("ACCION: %s / %s", ObtenerTextoBotonPrincipal(participantes[i]), TextoGolpePesca(participantes[i])),
                    x + 18,
                    y + 54,
                    14,
                    LIGHTGRAY
                );
            }

            k++;
        }
    }

    if (fase == FASE_PESCA_PREPARACION)
    {
        int numero = (int)std::ceil(tiempoPreparacion);

        if (numero < 1) numero = 1;

        const char* texto = TextFormat("%d", numero);
        DrawText(texto, anchoPantalla / 2 - MeasureText(texto, 96) / 2, altoPantalla / 2 - 150, 96, GOLD);

        const char* ayuda = "LANZA, ESPERA EL '!' Y ENGANCHA. DORADO = 5. CUIDADO CON LAS BOTAS!";
        DrawText(ayuda, anchoPantalla / 2 - MeasureText(ayuda, 20) / 2, altoPantalla / 2 - 40, 20, RAYWHITE);
    }
    else if (
        fase == FASE_PESCA_TERMINADO &&
        resultado.estado == RESULTADO_MINIJUEGO_FINALIZADO
    )
    {
        int panelAncho = 560;
        int panelAlto = 130 + 30 * activos;
        int px = anchoPantalla / 2 - panelAncho / 2;
        int py = altoPantalla / 2 - panelAlto / 2 - 40;

        DrawRectangle(px, py, panelAncho, panelAlto, Fade(BLACK, 0.9f));

        int ganadores[MAX_PARTICIPANTES]{};
        int cantidadGanadores = ObtenerIndicesGanadores(resultado, ganadores, MAX_PARTICIPANTES);
        const char* titulo = "EMPATE";

        if (resultado.desenlace == DESENLACE_CON_GANADOR && cantidadGanadores == 1)
        {
            titulo = TextFormat("GANA %s", NombreJugadorPesca(participantes[ganadores[0]], ganadores[0]));
        }

        DrawText(titulo, anchoPantalla / 2 - MeasureText(titulo, 32) / 2, py + 14, 32, GOLD);

        int fila = 0;

        for (int posicion = 1; posicion <= MAX_PARTICIPANTES; posicion++)
        {
            for (int i = 0; i < limite; i++)
            {
                if (!participa[i] || resultado.participantes[i].posicionFinal != posicion)
                {
                    continue;
                }

                DrawText(
                    TextFormat(
                        "%d.  %s   %d pts  (%d peces, mejor %d)",
                        posicion,
                        NombreJugadorPesca(participantes[i], i),
                        estados[i].puntos,
                        estados[i].peces,
                        estados[i].mejorValor
                    ),
                    px + 30,
                    py + 62 + fila * 30,
                    20,
                    coloresJugadores[i]
                );
                fila++;
            }
        }

        DrawText(
            TextoReinicioMinijuego(),
            anchoPantalla / 2 - MeasureText(TextoReinicioMinijuego(), 18) / 2,
            py + panelAlto - 30,
            18,
            RAYWHITE
        );
    }
}


const ResultadoMinijuego& MinijuegoPescaIsla::ObtenerResultado() const
{
    return resultado;
}
