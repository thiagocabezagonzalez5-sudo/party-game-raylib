#include "Minigames/MinijuegoCajasPuerto.h"

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


//==================================================
// CAJAS DEL PUERTO
//==================================================
//
// 1 vs 3. El trio se esconde en contenedores numerados mientras el
// solitario (en la cabina de la grua) mira hacia otro lado. Luego la grua
// deja caer cargas "chispa" sobre contenedores elegidos.
//
// Informacion oculta: el gruero no debe saber en que contenedor entro
// cada polizon. Durante ESCONDER la niebla cubre el patio y los polizones
// no se dibujan (ni sombra, ni anillo, ni particulas, ni rotulo). Un
// polizon humano solo ve una marca chica de su color mientras camina; si
// el gruero tambien es humano aparecen marcas falsas identicas que
// deambulan entre contenedores, asi la marca no delata la caja elegida.
//==================================================


static const float DURACION_PREPARACION_CAJAS = 3.0f;
static const float DURACION_ESCONDER_CAJAS = 6.0f;
static const float DURACION_CIERRE_CAJAS = 1.0f;
static const float DURACION_ELEGIR_CAJAS = 5.0f;
static const float DURACION_PASO_RESOLUCION_CAJAS = 1.1f;
static const float DURACION_REVELACION_CAJAS = 2.0f;
static const float ESPACIADO_CAJAS = 2.1f;
static const float Z_CONTENEDORES_CAJAS = -2.5f;
static const float Z_FRENTE_CONTENEDORES_CAJAS = -1.5f;
static const float LIMITE_X_CAJAS = 9.6f;
static const float Z_MINIMO_CAJAS = -1.0f;
static const float Z_MAXIMO_CAJAS = 5.5f;
static const float VELOCIDAD_JUGADOR_CAJAS = 5.0f;
static const float ALTURA_GANCHO_CAJAS = 4.2f;
static const int ELECCIONES_POR_RONDA_CAJAS[RONDAS_CAJAS] = { 2, 2, 3 };


//==================================================
// LOGICA
//==================================================


static float LimitarCajas(float valor, float minimo, float maximo)
{
    if (valor < minimo) return minimo;
    if (valor > maximo) return maximo;
    return valor;
}


static float AcercarCajas(float valor, float objetivo, float paso)
{
    if (valor < objetivo - paso) return valor + paso;
    if (valor > objetivo + paso) return valor - paso;
    return objetivo;
}


static float PosicionXContenedorCajas(int indice)
{
    return ((float)indice - 3.5f) * ESPACIADO_CAJAS;
}


static Color ColorContenedorBaseCajas(int indice)
{
    const Color colores[CANTIDAD_CONTENEDORES_CAJAS] =
    {
        Color{ 168, 62, 52, 255 },
        Color{ 52, 110, 166, 255 },
        Color{ 70, 140, 84, 255 },
        Color{ 196, 150, 56, 255 },
        Color{ 130, 76, 150, 255 },
        Color{ 60, 150, 156, 255 },
        Color{ 190, 98, 52, 255 },
        Color{ 112, 118, 130, 255 }
    };

    return colores[indice % CANTIDAD_CONTENEDORES_CAJAS];
}


static bool EsControlBotCajas(const Participante& participante)
{
    return participante.esBot || !participante.conectado;
}


static int ContarVivosCajas(const MinijuegoCajasPuerto& minijuego)
{
    int vivos = 0;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (
            i != minijuego.indiceSolo &&
            minijuego.resultado.participantes[i].participo &&
            minijuego.estadosJugadores[i].vivo
        )
        {
            vivos++;
        }
    }

    return vivos;
}


//--------------------------------------------------
// Efectos (solo visual). Azar propio para no alterar
// la secuencia de GetRandomValue que usan reglas y bots.
//--------------------------------------------------

static float AzarVisualCajas(MinijuegoCajasPuerto& minijuego, float minimo, float maximo)
{
    minijuego.semillaVisual = minijuego.semillaVisual * 1664525u + 1013904223u;
    float u = (float)((minijuego.semillaVisual >> 8) & 0xFFFFFFu) / 16777215.0f;
    return minimo + (maximo - minimo) * u;
}


static int CantidadParticulasCajas(int base)
{
    long cantidad = std::lround((float)base * FactorCalidadGrafica(CalidadParticulas()));
    return cantidad < 1 ? 1 : (int)cantidad;
}


static void EmitirParticulaCajas(
    MinijuegoCajasPuerto& minijuego,
    Vector3 posicion,
    Vector3 velocidad,
    float vida,
    float tamano,
    float gravedad,
    Color color
)
{
    for (int i = 0; i < MAX_PARTICULAS_CAJAS; i++)
    {
        ParticulaCajasPuerto& p = minijuego.particulas[i];
        if (p.activa) continue;

        p.activa = true;
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


// Astillas de chapa, polvo y (si el ancla protegio) chispas doradas.
static void EmitirImpactoCajas(MinijuegoCajasPuerto& minijuego, int indice, Color colorCaja)
{
    const ContenedorCajasPuerto& contenedor = minijuego.contenedores[indice];
    Vector3 c = contenedor.posicion;

    int astillas = CantidadParticulasCajas(18);
    for (int k = 0; k < astillas; k++)
    {
        float a = AzarVisualCajas(minijuego, 0.0f, 6.2832f);
        float v = AzarVisualCajas(minijuego, 1.5f, 4.2f);
        Color color = k % 3 == 0 ? Color{ 58, 52, 50, 255 } : colorCaja;
        EmitirParticulaCajas(
            minijuego,
            { c.x + std::cos(a) * 0.6f, c.y + 1.15f, c.z + std::sin(a) * 0.6f },
            { std::cos(a) * v, AzarVisualCajas(minijuego, 3.0f, 6.5f), std::sin(a) * v },
            AzarVisualCajas(minijuego, 0.7f, 1.2f),
            AzarVisualCajas(minijuego, 0.07f, 0.16f),
            13.0f,
            color
        );
    }

    int polvo = CantidadParticulasCajas(14);
    for (int k = 0; k < polvo; k++)
    {
        float a = 6.2832f * (float)k / (float)polvo + AzarVisualCajas(minijuego, -0.2f, 0.2f);
        EmitirParticulaCajas(
            minijuego,
            { c.x + std::cos(a) * 1.0f, 0.25f, c.z + 1.05f + std::sin(a) * 0.3f },
            { std::cos(a) * AzarVisualCajas(minijuego, 1.0f, 2.2f), AzarVisualCajas(minijuego, 0.3f, 0.9f), std::sin(a) * 0.8f + 0.6f },
            AzarVisualCajas(minijuego, 0.8f, 1.3f),
            AzarVisualCajas(minijuego, 0.25f, 0.42f),
            -0.4f,
            Color{ 150, 144, 136, 150 }
        );
    }

    if (contenedor.tipoGolpe == 2)
    {
        int chispas = CantidadParticulasCajas(16);
        for (int k = 0; k < chispas; k++)
        {
            float a = AzarVisualCajas(minijuego, 0.0f, 6.2832f);
            EmitirParticulaCajas(
                minijuego,
                { c.x, c.y + 1.4f, c.z },
                { std::cos(a) * 3.2f, AzarVisualCajas(minijuego, 2.5f, 5.0f), std::sin(a) * 3.2f },
                AzarVisualCajas(minijuego, 0.4f, 0.75f),
                0.07f,
                8.0f,
                Color{ 255, 226, 96, 255 }
            );
        }
    }
}


// Salpicadura del agua contra los costados del muelle (ambiente).
static void EmitirSalpicaduraCajas(MinijuegoCajasPuerto& minijuego)
{
    float lado = AzarVisualCajas(minijuego, 0.0f, 1.0f) < 0.5f ? -1.0f : 1.0f;
    Vector3 origen = { lado * 11.25f, -0.85f, AzarVisualCajas(minijuego, -5.5f, 4.5f) };
    int gotas = CantidadParticulasCajas(7);

    for (int k = 0; k < gotas; k++)
    {
        EmitirParticulaCajas(
            minijuego,
            origen,
            { lado * AzarVisualCajas(minijuego, 0.2f, 1.1f), AzarVisualCajas(minijuego, 2.0f, 3.6f), AzarVisualCajas(minijuego, -0.6f, 0.6f) },
            AzarVisualCajas(minijuego, 0.5f, 0.8f),
            AzarVisualCajas(minijuego, 0.06f, 0.12f),
            9.0f,
            Color{ 190, 220, 240, 210 }
        );
    }
}


static void FinalizarCajas(MinijuegoCajasPuerto& minijuego, bool ganaSolo)
{
    if (minijuego.resultado.estado == RESULTADO_MINIJUEGO_FINALIZADO)
    {
        return;
    }

    minijuego.resultado.estado = RESULTADO_MINIJUEGO_FINALIZADO;
    minijuego.resultado.desenlace = DESENLACE_CON_GANADOR;
    minijuego.resultado.cantidadEquipos = 2;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        ResultadoParticipante& resultadoJugador =
            minijuego.resultado.participantes[i];

        if (!resultadoJugador.participo)
        {
            continue;
        }

        bool esSolo = i == minijuego.indiceSolo;
        bool ganador = esSolo == ganaSolo;

        resultadoJugador.numeroEquipo = esSolo ? 0 : 1;
        resultadoJugador.posicionFinal = ganador ? 1 : 2;
        resultadoJugador.puntuacionMinijuego = esSolo
            ? minijuego.eliminadosTotal * 100
            : minijuego.estadosJugadores[i].rondasSobrevividas * 100;
    }

    minijuego.fase = FASE_CAJAS_TERMINADO;
    ReproducirSonidoMinijuego(minijuego.audio, SONIDO_RESULTADO);
}


static void IniciarRondaCajas(
    MinijuegoCajasPuerto& minijuego,
    JugadorPrueba jugadores[]
)
{
    for (int c = 0; c < CANTIDAD_CONTENEDORES_CAJAS; c++)
    {
        ContenedorCajasPuerto& contenedor = minijuego.contenedores[c];
        contenedor.ocupante = -1;
        contenedor.elegido = false;
        contenedor.revelado = false;
        contenedor.reforzado = false;
        contenedor.caida = 0.0f;
        contenedor.humo = 0.0f;
        contenedor.sacudida = 0.0f;
        contenedor.aviso = 0.0f;
        contenedor.tipoGolpe = 0;
    }

    minijuego.contenedores[
        GetRandomValue(0, CANTIDAD_CONTENEDORES_CAJAS - 1)
    ].reforzado = true;

    const float spawnsX[MAX_PARTICIPANTES] = { -4.5f, -1.5f, 1.5f, 4.5f };
    int cursorSpawn = 0;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        EstadoJugadorCajasPuerto& estado = minijuego.estadosJugadores[i];
        estado.contenedor = -1;
        estado.accionPrevia = false;
        estado.objetivoBot = -1;
        estado.retrasoBot = GetRandomValue(80, 430) / 100.0f;

        if (
            i == minijuego.indiceSolo ||
            !minijuego.resultado.participantes[i].participo ||
            !estado.vivo
        )
        {
            continue;
        }

        jugadores[i].posicion =
        {
            spawnsX[cursorSpawn % MAX_PARTICIPANTES],
            jugadores[i].tamano.y * 0.5f,
            4.0f
        };
        jugadores[i].direccionMirada = { 0.0f, 0.0f, -1.0f };
        jugadores[i].enSuelo = true;
        jugadores[i].cayendo = false;
        jugadores[i].aplastado = false;
        cursorSpawn++;

        // Las marcas falsas salen del mismo punto que el polizon real.
        for (int s = 0; s < SENUELOS_POR_JUGADOR_CAJAS; s++)
        {
            SenueloCajasPuerto& senuelo = minijuego.senuelos[i][s];
            senuelo.posicion = jugadores[i].posicion;
            senuelo.objetivoX = PosicionXContenedorCajas(
                (int)AzarVisualCajas(minijuego, 0.0f, (float)CANTIDAD_CONTENEDORES_CAJAS - 0.01f)
            );
            senuelo.espera = AzarVisualCajas(minijuego, 0.0f, 1.0f);
        }
    }

    minijuego.fase = FASE_CAJAS_ESCONDER;
    minijuego.tiempoFase = DURACION_ESCONDER_CAJAS;
    minijuego.cursor = 3;
    minijuego.eleccionesRestantes = ELECCIONES_POR_RONDA_CAJAS[minijuego.ronda - 1];
    minijuego.cantidadElegidos = 0;
    minijuego.eliminadosRonda = 0;
    minijuego.pasoResolucion = 0;
    minijuego.tiempoPaso = 0.0f;
    minijuego.tiempoFinResolucion = 0.0f;
    minijuego.caidaIniciada = false;
    minijuego.detonado = false;
    minijuego.revelacionHecha = false;
    minijuego.botSoloObjetivo = -1;
    minijuego.botSoloTemporizador = 0.6f;
    minijuego.direccionPrevia = 0;
    minijuego.repeticionCursor = 0.0f;
}


static void EntrarContenedorCajas(
    MinijuegoCajasPuerto& minijuego,
    JugadorPrueba jugadores[],
    int indiceJugador,
    int indiceContenedor,
    bool conSonido
)
{
    minijuego.estadosJugadores[indiceJugador].contenedor = indiceContenedor;
    minijuego.contenedores[indiceContenedor].ocupante = indiceJugador;

    Vector3 centro = minijuego.contenedores[indiceContenedor].posicion;
    jugadores[indiceJugador].posicion =
    {
        centro.x,
        jugadores[indiceJugador].tamano.y * 0.5f,
        centro.z
    };

    if (conSonido)
    {
        ReproducirSonidoMinijuego(minijuego.audio, SONIDO_BOTON);
    }
}


static int ContenedorMasCercanoLibreCajas(
    const MinijuegoCajasPuerto& minijuego,
    const JugadorPrueba& jugador
)
{
    int mejor = -1;
    float mejorDistancia = 1000000.0f;

    for (int c = 0; c < CANTIDAD_CONTENEDORES_CAJAS; c++)
    {
        if (minijuego.contenedores[c].ocupante >= 0)
        {
            continue;
        }

        float dx = jugador.posicion.x - minijuego.contenedores[c].posicion.x;
        float dz = jugador.posicion.z - minijuego.contenedores[c].posicion.z;
        float distancia = dx * dx + dz * dz;

        if (distancia < mejorDistancia)
        {
            mejorDistancia = distancia;
            mejor = c;
        }
    }

    return mejor;
}


static void IntentarEntrarCajas(
    MinijuegoCajasPuerto& minijuego,
    JugadorPrueba jugadores[],
    int indiceJugador
)
{
    const JugadorPrueba& jugador = jugadores[indiceJugador];

    int indice = (int)std::lround(jugador.posicion.x / ESPACIADO_CAJAS + 3.5f);
    if (indice < 0) indice = 0;
    if (indice >= CANTIDAD_CONTENEDORES_CAJAS) indice = CANTIDAD_CONTENEDORES_CAJAS - 1;

    float dx = std::fabs(
        jugador.posicion.x - minijuego.contenedores[indice].posicion.x
    );

    if (dx > 1.0f || jugador.posicion.z > 0.9f)
    {
        return;
    }

    if (minijuego.contenedores[indice].ocupante >= 0)
    {
        ReproducirSonidoMinijuego(minijuego.audio, SONIDO_ERROR);
        minijuego.estadosJugadores[indiceJugador].objetivoBot = -1;
        return;
    }

    EntrarContenedorCajas(minijuego, jugadores, indiceJugador, indice, true);
}


static int ElegirContenedorBotCajas(
    const MinijuegoCajasPuerto& minijuego,
    const JugadorPrueba& jugador
)
{
    int libres[CANTIDAD_CONTENEDORES_CAJAS]{};
    int cantidad = 0;

    for (int c = 0; c < CANTIDAD_CONTENEDORES_CAJAS; c++)
    {
        if (minijuego.contenedores[c].ocupante < 0)
        {
            libres[cantidad] = c;
            cantidad++;
        }
    }

    if (cantidad == 0)
    {
        return -1;
    }

    // Sesgo leve a los alejados: de dos candidatos al azar, el mas lejano.
    int a = libres[GetRandomValue(0, cantidad - 1)];
    int b = libres[GetRandomValue(0, cantidad - 1)];
    float distanciaA = std::fabs(jugador.posicion.x - minijuego.contenedores[a].posicion.x);
    float distanciaB = std::fabs(jugador.posicion.x - minijuego.contenedores[b].posicion.x);

    return distanciaA >= distanciaB ? a : b;
}


static InputMinijuegoParticipante CrearEntradaBotEsconderCajas(
    MinijuegoCajasPuerto& minijuego,
    int indiceJugador,
    const JugadorPrueba& jugador
)
{
    InputMinijuegoParticipante entrada{};
    EstadoJugadorCajasPuerto& estado = minijuego.estadosJugadores[indiceJugador];

    if (
        estado.objetivoBot < 0 ||
        minijuego.contenedores[estado.objetivoBot].ocupante >= 0
    )
    {
        estado.objetivoBot = ElegirContenedorBotCajas(minijuego, jugador);
    }

    if (estado.objetivoBot < 0)
    {
        return entrada;
    }

    float objetivoX = minijuego.contenedores[estado.objetivoBot].posicion.x;
    float objetivoZ = 0.2f;

    if (jugador.posicion.x < objetivoX - 0.2f)
        entrada.derecha = true;
    else if (jugador.posicion.x > objetivoX + 0.2f)
        entrada.izquierda = true;

    if (jugador.posicion.z > objetivoZ + 0.15f)
        entrada.adelante = true;

    float transcurrido = DURACION_ESCONDER_CAJAS - minijuego.tiempoFase;

    if (
        !entrada.izquierda &&
        !entrada.derecha &&
        !entrada.adelante &&
        transcurrido >= estado.retrasoBot
    )
    {
        entrada.golpear = true;
    }

    return entrada;
}


static void ActualizarEsconderCajas(
    MinijuegoCajasPuerto& minijuego,
    float deltaTime,
    JugadorPrueba jugadores[],
    Participante participantes[]
)
{
    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        EstadoJugadorCajasPuerto& estado = minijuego.estadosJugadores[i];

        if (
            i == minijuego.indiceSolo ||
            !minijuego.resultado.participantes[i].participo ||
            !estado.vivo ||
            estado.contenedor >= 0
        )
        {
            continue;
        }

        JugadorPrueba& jugador = jugadores[i];
        InputMinijuegoParticipante entrada{};

        if (EsControlBotCajas(participantes[i]))
        {
            entrada = CrearEntradaBotEsconderCajas(minijuego, i, jugador);
        }
        else
        {
            entrada = LeerInputMinijuegoParticipante(participantes[i]);
        }

        float moverX =
            (entrada.derecha ? 1.0f : 0.0f) -
            (entrada.izquierda ? 1.0f : 0.0f);
        float moverZ =
            (entrada.atras ? 1.0f : 0.0f) -
            (entrada.adelante ? 1.0f : 0.0f);
        float longitud = std::sqrt(moverX * moverX + moverZ * moverZ);

        if (longitud > 0.01f)
        {
            moverX /= longitud;
            moverZ /= longitud;
            jugador.posicion.x += moverX * VELOCIDAD_JUGADOR_CAJAS * deltaTime;
            jugador.posicion.z += moverZ * VELOCIDAD_JUGADOR_CAJAS * deltaTime;
            jugador.direccionMirada = { moverX, 0.0f, moverZ };
        }

        jugador.posicion.x = LimitarCajas(jugador.posicion.x, -LIMITE_X_CAJAS, LIMITE_X_CAJAS);
        jugador.posicion.z = LimitarCajas(jugador.posicion.z, Z_MINIMO_CAJAS, Z_MAXIMO_CAJAS);
        jugador.posicion.y = jugador.tamano.y * 0.5f;
        jugador.enSuelo = true;
        jugador.cayendo = false;

        bool accion = entrada.golpear && !estado.accionPrevia;
        estado.accionPrevia = entrada.golpear;

        if (accion)
        {
            IntentarEntrarCajas(minijuego, jugadores, i);
        }
    }

    bool todosEscondidos = true;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (
            i != minijuego.indiceSolo &&
            minijuego.resultado.participantes[i].participo &&
            minijuego.estadosJugadores[i].vivo &&
            minijuego.estadosJugadores[i].contenedor < 0
        )
        {
            todosEscondidos = false;
        }
    }

    float antes = minijuego.tiempoFase;
    minijuego.tiempoFase -= deltaTime;
    ActualizarAudioAlertaTiempo(minijuego.audio, antes, minijuego.tiempoFase);

    if (!todosEscondidos && minijuego.tiempoFase > 0.0f)
    {
        return;
    }

    // Quien no se escondio queda dentro del contenedor libre mas cercano.
    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (
            i == minijuego.indiceSolo ||
            !minijuego.resultado.participantes[i].participo ||
            !minijuego.estadosJugadores[i].vivo ||
            minijuego.estadosJugadores[i].contenedor >= 0
        )
        {
            continue;
        }

        int libre = ContenedorMasCercanoLibreCajas(minijuego, jugadores[i]);

        if (libre >= 0)
        {
            EntrarContenedorCajas(minijuego, jugadores, i, libre, false);
        }
    }

    ReproducirSonidoMinijuego(minijuego.audio, SONIDO_PLATAFORMA);
    minijuego.fase = FASE_CAJAS_CIERRE;
    minijuego.tiempoFase = DURACION_CIERRE_CAJAS;
}


// Marcas falsas (solo visual): caminan como un polizon hasta el frente de
// un contenedor al azar, esperan y eligen otro. Ignoran la ocupacion real.
static void ActualizarSenuelosCajas(MinijuegoCajasPuerto& minijuego, float deltaTime)
{
    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        for (int s = 0; s < SENUELOS_POR_JUGADOR_CAJAS; s++)
        {
            SenueloCajasPuerto& senuelo = minijuego.senuelos[i][s];

            if (senuelo.espera > 0.0f)
            {
                senuelo.espera -= deltaTime;
                continue;
            }

            float dx = senuelo.objetivoX - senuelo.posicion.x;
            float dz = 0.3f - senuelo.posicion.z;
            float distancia = std::sqrt(dx * dx + dz * dz);
            float paso = VELOCIDAD_JUGADOR_CAJAS * deltaTime;

            if (distancia <= paso)
            {
                senuelo.posicion.x = senuelo.objetivoX;
                senuelo.posicion.z = 0.3f;
                senuelo.espera = AzarVisualCajas(minijuego, 0.6f, 2.2f);
                senuelo.objetivoX = PosicionXContenedorCajas(
                    (int)AzarVisualCajas(minijuego, 0.0f, (float)CANTIDAD_CONTENEDORES_CAJAS - 0.01f)
                );
                continue;
            }

            senuelo.posicion.x += dx / distancia * paso;
            senuelo.posicion.z += dz / distancia * paso;
        }
    }
}


static void IniciarResolucionCajas(MinijuegoCajasPuerto& minijuego)
{
    minijuego.fase = FASE_CAJAS_RESOLVER;
    minijuego.pasoResolucion = 0;
    minijuego.tiempoPaso = 0.0f;
    minijuego.tiempoFinResolucion = 0.0f;
    minijuego.caidaIniciada = false;
    minijuego.detonado = false;
    minijuego.revelacionHecha = false;
}


static void ConfirmarEleccionCajas(MinijuegoCajasPuerto& minijuego)
{
    int c = minijuego.cursor;

    if (
        minijuego.eleccionesRestantes <= 0 ||
        minijuego.cantidadElegidos >= MAX_ELECCIONES_CAJAS ||
        minijuego.contenedores[c].elegido
    )
    {
        return;
    }

    minijuego.contenedores[c].elegido = true;
    minijuego.historialElegidos[c] = true;
    minijuego.ordenElegidos[minijuego.cantidadElegidos] = c;
    minijuego.cantidadElegidos++;
    minijuego.eleccionesRestantes--;
    ReproducirSonidoMinijuego(minijuego.audio, SONIDO_BOTON);

    if (minijuego.eleccionesRestantes <= 0)
    {
        IniciarResolucionCajas(minijuego);
    }
}


static void ActualizarSoloBotCajas(MinijuegoCajasPuerto& minijuego, float deltaTime)
{
    if (
        minijuego.botSoloObjetivo < 0 ||
        minijuego.contenedores[minijuego.botSoloObjetivo].elegido
    )
    {
        int candidatos[CANTIDAD_CONTENEDORES_CAJAS]{};
        int cantidad = 0;

        for (int c = 0; c < CANTIDAD_CONTENEDORES_CAJAS; c++)
        {
            if (!minijuego.contenedores[c].elegido && !minijuego.historialElegidos[c])
            {
                candidatos[cantidad] = c;
                cantidad++;
            }
        }

        if (cantidad == 0)
        {
            for (int c = 0; c < CANTIDAD_CONTENEDORES_CAJAS; c++)
            {
                if (!minijuego.contenedores[c].elegido)
                {
                    candidatos[cantidad] = c;
                    cantidad++;
                }
            }
        }

        if (cantidad == 0)
        {
            return;
        }

        minijuego.botSoloObjetivo = candidatos[GetRandomValue(0, cantidad - 1)];
        minijuego.botSoloTemporizador = 0.2f;
    }

    minijuego.botSoloTemporizador -= deltaTime;

    if (minijuego.botSoloTemporizador > 0.0f)
    {
        return;
    }

    if (minijuego.cursor < minijuego.botSoloObjetivo)
    {
        minijuego.cursor++;
        minijuego.botSoloTemporizador = 0.11f;
    }
    else if (minijuego.cursor > minijuego.botSoloObjetivo)
    {
        minijuego.cursor--;
        minijuego.botSoloTemporizador = 0.11f;
    }
    else
    {
        minijuego.botSoloObjetivo = -1;
        ConfirmarEleccionCajas(minijuego);
        minijuego.botSoloTemporizador = 0.25f;
    }
}


static void ActualizarElegirCajas(
    MinijuegoCajasPuerto& minijuego,
    float deltaTime,
    Participante participantes[]
)
{
    float antes = minijuego.tiempoFase;
    minijuego.tiempoFase -= deltaTime;
    ActualizarAudioAlertaTiempo(minijuego.audio, antes, minijuego.tiempoFase);

    const Participante& participanteSolo = participantes[minijuego.indiceSolo];

    if (EsControlBotCajas(participanteSolo))
    {
        ActualizarSoloBotCajas(minijuego, deltaTime);
    }
    else
    {
        InputMinijuegoParticipante entrada =
            LeerInputMinijuegoParticipante(participanteSolo);

        int direccion = (entrada.derecha ? 1 : 0) - (entrada.izquierda ? 1 : 0);

        if (direccion != 0)
        {
            minijuego.repeticionCursor -= deltaTime;

            if (direccion != minijuego.direccionPrevia || minijuego.repeticionCursor <= 0.0f)
            {
                int nuevo = minijuego.cursor + direccion;

                if (nuevo >= 0 && nuevo < CANTIDAD_CONTENEDORES_CAJAS)
                {
                    minijuego.cursor = nuevo;
                }

                minijuego.repeticionCursor =
                    direccion != minijuego.direccionPrevia ? 0.28f : 0.14f;
            }
        }
        else
        {
            minijuego.repeticionCursor = 0.0f;
        }

        minijuego.direccionPrevia = direccion;

        if (entrada.golpear && !minijuego.accionSoloPrevia)
        {
            ConfirmarEleccionCajas(minijuego);
        }

        minijuego.accionSoloPrevia = entrada.golpear;
    }

    if (minijuego.fase != FASE_CAJAS_ELEGIR || minijuego.tiempoFase > 0.0f)
    {
        return;
    }

    // Se acabo el tiempo: la grua completa sus cargas al azar.
    for (int intento = 0; intento < 64 && minijuego.fase == FASE_CAJAS_ELEGIR; intento++)
    {
        int c = GetRandomValue(0, CANTIDAD_CONTENEDORES_CAJAS - 1);

        if (!minijuego.contenedores[c].elegido)
        {
            minijuego.cursor = c;
            ConfirmarEleccionCajas(minijuego);
        }
    }

    if (minijuego.fase == FASE_CAJAS_ELEGIR)
    {
        IniciarResolucionCajas(minijuego);
    }
}


static void DetonarContenedorCajas(MinijuegoCajasPuerto& minijuego, int indice)
{
    ContenedorCajasPuerto& contenedor = minijuego.contenedores[indice];
    contenedor.humo = 1.0f;
    contenedor.marcas++;
    contenedor.revelado = true;

    // Feedback visual: sacudida, cartel flotante, particulas y temblor.
    contenedor.sacudida = 1.0f;
    contenedor.aviso = 1.0f;
    contenedor.tipoGolpe = contenedor.ocupante < 0 ? 0 : (contenedor.reforzado ? 2 : 1);
    EmitirImpactoCajas(minijuego, indice, ColorContenedorBaseCajas(indice));
    ActivarTemblorCamaraGeneral(contenedor.tipoGolpe == 1 ? 0.30f : 0.20f, 0.34f);

    ReproducirSonidoMinijuego(minijuego.audio, SONIDO_EXPLOSION);

    if (contenedor.ocupante < 0)
    {
        ReproducirSonidoMinijuego(minijuego.audio, SONIDO_ACIERTO);
        return;
    }

    if (contenedor.reforzado)
    {
        // El ancla dorada protege al ocupante.
        ReproducirSonidoMinijuego(minijuego.audio, SONIDO_IMPACTO);
        return;
    }

    minijuego.estadosJugadores[contenedor.ocupante].vivo = false;
    minijuego.eliminadosRonda++;
    minijuego.eliminadosTotal++;
    ReproducirSonidoMinijuego(minijuego.audio, SONIDO_ELIMINADO);
}


static void FinalizarRondaCajas(
    MinijuegoCajasPuerto& minijuego,
    JugadorPrueba jugadores[]
)
{
    int vivos = 0;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (
            i != minijuego.indiceSolo &&
            minijuego.resultado.participantes[i].participo &&
            minijuego.estadosJugadores[i].vivo
        )
        {
            minijuego.estadosJugadores[i].rondasSobrevividas++;
            vivos++;
        }
    }

    if (vivos == 0)
    {
        FinalizarCajas(minijuego, true);
    }
    else if (minijuego.ronda >= RONDAS_CAJAS)
    {
        FinalizarCajas(minijuego, false);
    }
    else
    {
        minijuego.ronda++;
        IniciarRondaCajas(minijuego, jugadores);
    }
}


static void ActualizarResolverCajas(
    MinijuegoCajasPuerto& minijuego,
    float deltaTime,
    JugadorPrueba jugadores[]
)
{
    if (minijuego.pasoResolucion < minijuego.cantidadElegidos)
    {
        int c = minijuego.ordenElegidos[minijuego.pasoResolucion];
        minijuego.tiempoPaso += deltaTime;

        if (!minijuego.caidaIniciada && minijuego.tiempoPaso >= 0.35f)
        {
            minijuego.caidaIniciada = true;
            minijuego.contenedores[c].caida = 0.4f;
        }

        if (
            minijuego.caidaIniciada &&
            !minijuego.detonado &&
            minijuego.tiempoPaso >= 0.75f
        )
        {
            minijuego.detonado = true;
            DetonarContenedorCajas(minijuego, c);
        }

        if (minijuego.tiempoPaso >= DURACION_PASO_RESOLUCION_CAJAS)
        {
            minijuego.pasoResolucion++;
            minijuego.tiempoPaso = 0.0f;
            minijuego.caidaIniciada = false;
            minijuego.detonado = false;
        }

        return;
    }

    if (!minijuego.revelacionHecha)
    {
        minijuego.revelacionHecha = true;

        for (int c = 0; c < CANTIDAD_CONTENEDORES_CAJAS; c++)
        {
            minijuego.contenedores[c].revelado = true;
        }
    }

    minijuego.tiempoFinResolucion += deltaTime;

    if (minijuego.tiempoFinResolucion >= DURACION_REVELACION_CAJAS)
    {
        FinalizarRondaCajas(minijuego, jugadores);
    }
}


static void ActualizarEfectosCajas(MinijuegoCajasPuerto& minijuego, float deltaTime)
{
    for (int c = 0; c < CANTIDAD_CONTENEDORES_CAJAS; c++)
    {
        ContenedorCajasPuerto& contenedor = minijuego.contenedores[c];

        bool abierto =
            minijuego.fase == FASE_CAJAS_PREPARACION ||
            minijuego.fase == FASE_CAJAS_ESCONDER ||
            contenedor.revelado;

        contenedor.apertura = AcercarCajas(
            contenedor.apertura,
            abierto ? 1.0f : 0.0f,
            2.2f * deltaTime
        );

        if (contenedor.humo > 0.0f)
        {
            contenedor.humo -= 0.75f * deltaTime;
            if (contenedor.humo < 0.0f) contenedor.humo = 0.0f;
        }

        if (contenedor.caida > 0.0f)
        {
            contenedor.caida -= deltaTime;
            if (contenedor.caida < 0.0f) contenedor.caida = 0.0f;
        }
    }

    float objetivoX = 0.0f;

    if (minijuego.fase == FASE_CAJAS_ELEGIR)
    {
        objetivoX = minijuego.contenedores[minijuego.cursor].posicion.x;
    }
    else if (
        minijuego.fase == FASE_CAJAS_RESOLVER &&
        minijuego.pasoResolucion < minijuego.cantidadElegidos
    )
    {
        objetivoX = minijuego.contenedores[
            minijuego.ordenElegidos[minijuego.pasoResolucion]
        ].posicion.x;
    }

    minijuego.ganchoX = AcercarCajas(minijuego.ganchoX, objetivoX, 16.0f * deltaTime);
}


static void ColocarSoloCajas(MinijuegoCajasPuerto& minijuego, JugadorPrueba jugadores[])
{
    if (minijuego.indiceSolo < 0)
    {
        return;
    }

    JugadorPrueba& jugadorSolo = jugadores[minijuego.indiceSolo];
    jugadorSolo.posicion = { minijuego.ganchoX, 6.5f, -5.0f };

    bool mirandoAfuera =
        minijuego.fase == FASE_CAJAS_PREPARACION ||
        minijuego.fase == FASE_CAJAS_ESCONDER ||
        minijuego.fase == FASE_CAJAS_CIERRE;

    jugadorSolo.direccionMirada = mirandoAfuera
        ? Vector3{ 0.0f, 0.0f, -1.0f }
        : Vector3{ 0.0f, 0.0f, 1.0f };
    jugadorSolo.enSuelo = true;
    jugadorSolo.cayendo = false;
}


// Estado solo visual: corre tambien en FIN para que las particulas y los
// carteles terminen su animacion.
static void ActualizarVisualCajas(MinijuegoCajasPuerto& minijuego, float deltaTime)
{
    if (minijuego.fase != minijuego.faseAnterior)
    {
        minijuego.faseAnterior = minijuego.fase;
        minijuego.tiempoAnuncio = 0.0f;
    }
    minijuego.tiempoAnuncio += deltaTime;

    for (int c = 0; c < CANTIDAD_CONTENEDORES_CAJAS; c++)
    {
        ContenedorCajasPuerto& contenedor = minijuego.contenedores[c];
        contenedor.sacudida = AcercarCajas(contenedor.sacudida, 0.0f, 2.6f * deltaTime);
        contenedor.aviso = AcercarCajas(contenedor.aviso, 0.0f, 0.62f * deltaTime);
    }

    // Niebla: entra al final de la cuenta, cubre todo ESCONDER y se
    // disipa cuando las puertas ya estan cerradas.
    if (minijuego.fase == FASE_CAJAS_ESCONDER)
    {
        minijuego.niebla = AcercarCajas(minijuego.niebla, 1.0f, 3.0f * deltaTime);
    }
    else if (minijuego.fase == FASE_CAJAS_PREPARACION)
    {
        float entrada = (1.0f - minijuego.tiempoPreparacion) / 1.0f;
        minijuego.niebla = LimitarCajas(entrada, 0.0f, 1.0f);
    }
    else
    {
        minijuego.niebla = AcercarCajas(minijuego.niebla, 0.0f, 1.1f * deltaTime);
    }

    if (minijuego.fase == FASE_CAJAS_ESCONDER)
    {
        ActualizarSenuelosCajas(minijuego, deltaTime);
    }

    minijuego.tiempoSalpicadura -= deltaTime;
    if (minijuego.tiempoSalpicadura <= 0.0f)
    {
        minijuego.tiempoSalpicadura = AzarVisualCajas(minijuego, 0.35f, 1.1f);
        if (CalidadDecoracion() != CALIDAD_BAJA)
        {
            EmitirSalpicaduraCajas(minijuego);
        }
    }

    for (int i = 0; i < MAX_PARTICULAS_CAJAS; i++)
    {
        ParticulaCajasPuerto& p = minijuego.particulas[i];
        if (!p.activa) continue;

        p.vida -= deltaTime;
        if (p.vida <= 0.0f)
        {
            p.activa = false;
            continue;
        }

        p.velocidad.y -= p.gravedad * deltaTime;
        float freno = std::exp(-1.6f * deltaTime);
        p.velocidad.x *= freno;
        p.velocidad.z *= freno;
        p.posicion.x += p.velocidad.x * deltaTime;
        p.posicion.y += p.velocidad.y * deltaTime;
        p.posicion.z += p.velocidad.z * deltaTime;

        // Rebote seco sobre el muelle o el agua (astillas).
        float suelo = std::fabs(p.posicion.x) < 11.0f && p.posicion.z > -6.2f && p.posicion.z < 7.7f ? 0.04f : -0.9f;
        if (p.posicion.y < suelo && p.velocidad.y < 0.0f)
        {
            p.posicion.y = suelo;
            p.velocidad.y *= -0.25f;
            p.velocidad.x *= 0.5f;
            p.velocidad.z *= 0.5f;
        }
    }
}


//==================================================
// INTERFAZ
//==================================================


void MinijuegoCajasPuerto::Inicializar()
{
    resultado = {};
    resultado.formato = FORMATO_MINIJUEGO_EQUIPOS;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        estadosJugadores[i] = {};
    }

    for (int c = 0; c < CANTIDAD_CONTENEDORES_CAJAS; c++)
    {
        contenedores[c] = {};
        contenedores[c].posicion =
        {
            PosicionXContenedorCajas(c),
            1.1f,
            Z_CONTENEDORES_CAJAS
        };
        historialElegidos[c] = false;
    }

    for (int i = 0; i < MAX_ELECCIONES_CAJAS; i++)
    {
        ordenElegidos[i] = 0;
    }

    camara.position = { 0.0f, 10.5f, 12.5f };
    camara.target = { 0.0f, 3.2f, -1.5f };
    camara.up = { 0.0f, 1.0f, 0.0f };
    camara.fovy = 54.0f;
    camara.projection = CAMERA_PERSPECTIVE;

    fase = FASE_CAJAS_PREPARACION;
    indiceSolo = -1;
    ronda = 1;
    cursor = 3;
    eleccionesRestantes = 0;
    cantidadElegidos = 0;
    pasoResolucion = 0;
    eliminadosRonda = 0;
    eliminadosTotal = 0;
    direccionPrevia = 0;
    botSoloObjetivo = -1;
    accionSoloPrevia = false;
    caidaIniciada = false;
    detonado = false;
    revelacionHecha = false;
    ganchoX = 0.0f;
    repeticionCursor = 0.0f;
    botSoloTemporizador = 0.0f;
    tiempoPaso = 0.0f;
    tiempoFinResolucion = 0.0f;
    tiempoFase = DURACION_ESCONDER_CAJAS;
    tiempoPreparacion = DURACION_PREPARACION_CAJAS;
    tiempoAnimacion = 0.0f;

    for (int i = 0; i < MAX_PARTICULAS_CAJAS; i++)
    {
        particulas[i] = {};
    }

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        for (int s = 0; s < SENUELOS_POR_JUGADOR_CAJAS; s++)
        {
            senuelos[i][s] = {};
        }
    }

    niebla = 0.0f;
    tiempoAnuncio = 0.0f;
    tiempoSalpicadura = 0.5f;
    semillaVisual = 0x2545F491u;
    faseAnterior = FASE_CAJAS_PREPARACION;
}


void MinijuegoCajasPuerto::Reiniciar(
    JugadorPrueba jugadores[],
    Participante participantes[],
    int cantidadMaxima
)
{
    Inicializar();
    CargarPaqueteModelosEscenarioRetro3D(ObtenerPaqueteCajasPuertoRetro3D());
    InicializarResultadoMinijuego(
        resultado,
        participantes,
        FORMATO_MINIJUEGO_EQUIPOS
    );

    int indices[MAX_PARTICIPANTES]{};
    int cantidad = ObtenerIndicesParticipantesActivos(
        participantes,
        indices,
        MAX_PARTICIPANTES
    );

    if (cantidad < 2)
    {
        resultado.estado = RESULTADO_MINIJUEGO_CANCELADO;
        fase = FASE_CAJAS_TERMINADO;
        return;
    }

    indiceSolo = indices[GetRandomValue(0, cantidad - 1)];
    resultado.cantidadEquipos = 2;

    int limite = cantidadMaxima < MAX_PARTICIPANTES
        ? cantidadMaxima
        : MAX_PARTICIPANTES;

    for (int i = 0; i < limite; i++)
    {
        if (!resultado.participantes[i].participo)
        {
            continue;
        }

        bool esSolo = i == indiceSolo;
        resultado.participantes[i].numeroEquipo = esSolo ? 0 : 1;
        estadosJugadores[i].vivo = !esSolo;

        ConfigurarJugadorMinijuegoEstandar(
            jugadores[i],
            Vector3{ 0.0f, 0.70f, 4.0f }
        );
    }

    IniciarRondaCajas(*this, jugadores);
    fase = FASE_CAJAS_PREPARACION;
    ColocarSoloCajas(*this, jugadores);
}


void MinijuegoCajasPuerto::Actualizar(
    float deltaTime,
    JugadorPrueba jugadores[],
    int cantidadMaxima,
    Participante participantes[]
)
{
    (void)cantidadMaxima;

    tiempoAnimacion += deltaTime;
    ActualizarVisualCajas(*this, deltaTime);

    if (
        fase == FASE_CAJAS_TERMINADO ||
        resultado.estado == RESULTADO_MINIJUEGO_CANCELADO
    )
    {
        return;
    }

    ActualizarEfectosCajas(*this, deltaTime);

    switch (fase)
    {
    case FASE_CAJAS_PREPARACION:
    {
        float preparacionAntes = tiempoPreparacion;
        tiempoPreparacion -= deltaTime;
        ActualizarAudioCuentaRegresiva(audio, preparacionAntes, tiempoPreparacion);

        if (tiempoPreparacion <= 0.0f)
        {
            tiempoPreparacion = 0.0f;
            fase = FASE_CAJAS_ESCONDER;
            tiempoFase = DURACION_ESCONDER_CAJAS;
        }
        break;
    }

    case FASE_CAJAS_ESCONDER:
        ActualizarEsconderCajas(*this, deltaTime, jugadores, participantes);
        break;

    case FASE_CAJAS_CIERRE:
        tiempoFase -= deltaTime;

        if (tiempoFase <= 0.0f)
        {
            fase = FASE_CAJAS_ELEGIR;
            tiempoFase = DURACION_ELEGIR_CAJAS;
            botSoloTemporizador = 0.6f;
            accionSoloPrevia = true;
        }
        break;

    case FASE_CAJAS_ELEGIR:
        ActualizarElegirCajas(*this, deltaTime, participantes);
        break;

    case FASE_CAJAS_RESOLVER:
        ActualizarResolverCajas(*this, deltaTime, jugadores);
        break;

    case FASE_CAJAS_TERMINADO:
        break;
    }

    ColocarSoloCajas(*this, jugadores);
}


//==================================================
// VISUAL
//==================================================
//
// El paquete compartido conserva pivotes y colores. Cada pieza que no pueda
// dibujarse mantiene sus primitivas originales, sin cambiar las reglas.
//==================================================


static Color OscurecerCajas(Color color, float factor)
{
    return Color{
        (unsigned char)((float)color.r * factor),
        (unsigned char)((float)color.g * factor),
        (unsigned char)((float)color.b * factor),
        255
    };
}


static Color ColorContenedorCajas(int indice)
{
    return ColorContenedorBaseCajas(indice);
}


static void DibujarFarolCajas(float x, float z)
{
    if (!DibujarModeloCajasPuertoRetro3D(MODELO_CAJAS_FAROL, { x, 0.0f, z }))
    {
        DrawCylinder({ x, 1.6f, z }, 0.09f, 0.12f, 3.2f, 8, Color{ 52, 54, 60, 255 });
        (DrawSphere)({ x, 3.3f, z }, 0.22f, Color{ 255, 226, 140, 255 });
    }
    // El halo es un efecto: se conserva tambien con el GLB del farol.
    // Sin sombra (parentesis): la luz no proyecta mancha en el muelle.
    (DrawSphere)({ x, 3.3f, z }, 0.5f, Fade(Color{ 255, 220, 120, 255 }, 0.18f));
}


// Agua del puerto: malla con olas que suben y bajan y se aclaran en las
// crestas. Lejos se funde con el fondo (niebla por distancia).
static void DibujarAguaCajas(float t)
{
    const float paso = CalidadDecoracion() == CALIDAD_ALTA ? 3.0f : 5.0f;
    const Color fondo = { 8, 14, 30, 255 };

    rlBegin(RL_QUADS);
    for (float z = -72.0f; z < 16.0f; z += paso)
    {
        for (float x = -66.0f; x < 66.0f; x += paso)
        {
            const float xs[4] = { x, x, x + paso, x + paso };
            const float zs[4] = { z, z + paso, z + paso, z };

            for (int k = 0; k < 4; k++)
            {
                float ola =
                    std::sin(xs[k] * 0.42f + t * 1.1f) +
                    std::sin(zs[k] * 0.57f - t * 1.6f + xs[k] * 0.13f);
                float brillo = 0.5f + 0.25f * ola;
                float lejos = LimitarCajas((-zs[k] - 14.0f) / 52.0f, 0.0f, 1.0f);
                Color agua = ColorLerp(Color{ 10, 30, 60, 255 }, Color{ 34, 78, 118, 255 }, brillo);
                agua = ColorLerp(agua, fondo, lejos * 0.85f);

                rlColor4ub(agua.r, agua.g, agua.b, 255);
                rlVertex3f(xs[k], -0.9f + 0.07f * ola, zs[k]);
            }
        }
    }
    rlEnd();
}


// Fondo animado: remolcador que cruza la bahia y gaviotas en circulo.
static void DibujarFondoCajas(float t)
{
    if (CalidadDecoracion() == CALIDAD_BAJA)
    {
        return;
    }

    // Remolcador: cruza de izquierda a derecha cada 70 s, cabeceando.
    float x = -46.0f + std::fmod(t * 1.3f, 92.0f);
    float cabeceo = std::sin(t * 1.7f) * 0.08f;
    Vector3 casco = { x, -0.55f + cabeceo, -30.0f };
    (DrawCube)(casco, 5.2f, 0.9f, 1.8f, Color{ 120, 42, 40, 255 });
    (DrawCube)({ x, casco.y + 0.5f, casco.z }, 5.4f, 0.14f, 1.9f, Color{ 40, 34, 36, 255 });
    (DrawCube)({ x - 0.6f, casco.y + 1.05f, casco.z }, 1.8f, 1.0f, 1.3f, Color{ 196, 196, 188, 255 });
    (DrawCube)({ x - 0.2f, casco.y + 1.9f, casco.z }, 0.4f, 0.8f, 0.4f, Color{ 60, 60, 66, 255 });
    (DrawSphere)({ x + 2.4f, casco.y + 0.8f, casco.z }, 0.12f, Color{ 120, 255, 140, 255 });
    (DrawSphere)({ x - 0.6f, casco.y + 1.2f, casco.z + 0.68f }, 0.1f, Color{ 255, 226, 140, 255 });

    // Gaviotas: alas en V que aletean (dos triangulos por ala, ambas caras).
    // Vuelan sobre el agua de los costados, por debajo del tendedero.
    const float centrosX[5] = { -19.0f, -15.0f, 15.5f, 19.5f, -24.0f };
    for (int g = 0; g < 5; g++)
    {
        float fase = (float)g * 1.37f;
        float radio = 2.6f + 0.8f * (float)(g % 3);
        float a = t * (0.45f + 0.06f * (float)g) + fase;
        Vector3 c =
        {
            centrosX[g] + std::cos(a) * radio,
            3.6f + 0.5f * (float)(g % 2) + 0.4f * std::sin(t * 0.7f + fase),
            -7.0f - 2.5f * (float)(g % 3) + std::sin(a) * radio * 0.7f
        };
        float aleteo = std::sin(t * 9.0f + fase * 3.0f) * 0.35f;
        float dx = -std::sin(a);
        float dz = std::cos(a) * 0.7f;
        float largo = std::sqrt(dx * dx + dz * dz);
        dx /= largo;
        dz /= largo;
        Vector3 lado = { -dz, 0.0f, dx };
        Color pluma = Color{ 226, 230, 236, 255 };

        for (int s = -1; s <= 1; s += 2)
        {
            Vector3 punta =
            {
                c.x + lado.x * 0.55f * (float)s,
                c.y + 0.18f + aleteo,
                c.z + lado.z * 0.55f * (float)s
            };
            Vector3 frente = { c.x + dx * 0.18f, c.y, c.z + dz * 0.18f };
            Vector3 atras = { c.x - dx * 0.18f, c.y, c.z - dz * 0.18f };
            DrawTriangle3D(frente, punta, atras, pluma);
            DrawTriangle3D(frente, atras, punta, pluma);
        }
    }
}


static void DibujarMuelleCajas()
{
    // Muelle de madera.
    bool muelleGLB = DibujarModeloCajasPuertoRetro3D(MODELO_CAJAS_MUELLE, { 0.0f, 0.0f, 0.75f });
    if (!muelleGLB)
        DrawCube({ 0.0f, -0.3f, 0.75f }, 22.0f, 0.6f, 14.0f, Color{ 86, 62, 42, 255 });

    for (int i = 0; !muelleGLB && i <= 14; i++)
    {
        float z = -6.0f + (float)i;
        DrawLine3D(
            { -11.0f, 0.01f, z },
            { 11.0f, 0.01f, z },
            Fade(Color{ 40, 28, 20, 255 }, 0.8f)
        );
    }

    for (int i = 0; i < 6; i++)
    {
        float x = -10.0f + 4.0f * (float)i;
        if (!DibujarModeloCajasPuertoRetro3D(MODELO_CAJAS_BOLARDO, { x, 0.0f, 7.8f }))
            DrawCylinder({ x, -0.2f, 7.8f }, 0.22f, 0.22f, 0.9f, 8, Color{ 52, 40, 30, 255 });
    }

    // Barco al fondo.
    if (!DibujarModeloCajasPuertoRetro3D(MODELO_CAJAS_BARCO, { 0.0f, -0.9f, -17.0f }))
    {
        DrawCube({ 0.0f, 0.4f, -17.0f }, 34.0f, 3.4f, 6.0f, Color{ 52, 36, 44, 255 });
        DrawCube({ 0.0f, -0.4f, -17.0f }, 34.5f, 1.2f, 6.2f, Color{ 120, 40, 40, 255 });
        DrawCube({ 9.0f, 3.6f, -17.0f }, 6.0f, 3.0f, 4.2f, Color{ 150, 154, 160, 255 });
        DrawCylinder({ 10.5f, 6.4f, -17.0f }, 0.6f, 0.8f, 2.6f, 10, Color{ 190, 70, 50, 255 });

        for (int i = 0; i < 6; i++)
        {
            float x = -11.0f + 3.2f * (float)i;
            DrawCube(
                { x, 2.6f, -16.5f },
                2.4f,
                1.6f,
                1.8f,
                OscurecerCajas(ColorContenedorCajas(i), 0.55f)
            );
        }

        for (int i = 0; i < 4; i++)
        {
            DrawSphere(
                { 6.2f + 1.2f * (float)i, 4.3f, -14.9f },
                0.12f,
                Color{ 255, 232, 150, 255 }
            );
        }
    }

    // Misma pila y colores del dibujo original; una sola malla decorativa.
    const Vector3 posicionesDecoracion[] =
    {
        { -12.5f, 0.0f, -5.0f }, { -12.5f, 2.2f, -5.0f },
        { 12.5f, 0.0f, -5.0f }, { 12.5f, 2.2f, -5.0f }
    };
    const int coloresDecoracion[] = { 2, 0, 1, 3 };
    for (int i = 0; i < 4; i++)
    {
        Vector3 p = posicionesDecoracion[i];
        Color color = OscurecerCajas(ColorContenedorCajas(coloresDecoracion[i]), 0.5f);
        if (!DibujarModeloCajasPuertoRetro3D(MODELO_CAJAS_CONTENEDOR_DECORACION,
            p, { 1.0f, 1.0f, 1.0f }, color))
            DrawCube({ p.x, p.y + 1.1f, p.z }, 1.9f, 2.2f, 4.0f, color);
    }

    DibujarFarolCajas(-10.5f, 1.0f);
    DibujarFarolCajas(10.5f, 1.0f);
    DibujarFarolCajas(-10.5f, 6.5f);
    DibujarFarolCajas(10.5f, 6.5f);
}


static void DibujarContenedorCajas(const ContenedorCajasPuerto& contenedor, int indice)
{
    Color color = ColorContenedorCajas(indice);
    Vector3 p = contenedor.posicion;
    float frente = p.z + 1.0f;

    // Golpe de la carga: la chapa se hunde un poco y rebota (mismo efecto
    // para cualquier contenedor golpeado, ocupado o no).
    float hundido = 1.0f - 0.06f * contenedor.sacudida *
        (0.6f + 0.4f * std::cos(contenedor.sacudida * 18.0f));

    // El estado guarda el centro Y=1.1; el cuerpo GLB guarda el suelo Y=0.
    if (!DibujarModeloCajasPuertoRetro3D(MODELO_CAJAS_CONTENEDOR_CUERPO,
        { p.x, p.y - 1.1f, p.z }, { 1.0f, hundido, 1.0f }, color))
    {
        Vector3 cuerpo = { p.x, 1.1f * hundido, p.z };
        DrawCube(cuerpo, 1.9f, 2.2f * hundido, 2.0f, color);
        DrawCubeWires(cuerpo, 1.9f, 2.2f * hundido, 2.0f, Fade(BLACK, 0.55f));
        // Costillas de chapa (volumen sin luces reales).
        for (int k = -3; k <= 3; k++)
        {
            (DrawCube)({ p.x + 0.26f * (float)k, cuerpo.y, p.z }, 0.05f, 2.0f * hundido, 2.04f, OscurecerCajas(color, 0.78f));
        }
        DrawCube({ p.x, p.y + 1.12f, p.z }, 1.96f, 0.08f, 2.06f, OscurecerCajas(color, 0.7f));
        DrawCube({ p.x, p.y, frente + 0.01f }, 1.7f, 2.0f, 0.02f, Color{ 6, 6, 10, 255 });
    }

    float escalaPuerta = 1.0f - 0.88f * contenedor.apertura;
    float ancho = 0.85f * escalaPuerta;
    Color puerta = OscurecerCajas(color, 0.82f);
    // Los GLB v1 crecen hacia el lado de su nombre. Se usa el que crece +X
    // en la bisagra izquierda y el que crece -X en la derecha, para cerrar
    // hacia el centro. No se recentran ni se reflejan vertices/herrajes.
    if (!DibujarModeloCajasPuertoRetro3D(MODELO_CAJAS_PUERTA_DERECHA,
        { p.x - 0.85f, p.y, frente + 0.08f }, { escalaPuerta, 1.0f, 1.0f }, puerta))
        DrawCube({ p.x - 0.85f + ancho * 0.5f, p.y, frente + 0.06f }, ancho, 2.0f, 0.08f, puerta);
    if (!DibujarModeloCajasPuertoRetro3D(MODELO_CAJAS_PUERTA_IZQUIERDA,
        { p.x + 0.85f, p.y, frente + 0.08f }, { escalaPuerta, 1.0f, 1.0f }, puerta))
        DrawCube({ p.x + 0.85f - ancho * 0.5f, p.y, frente + 0.06f }, ancho, 2.0f, 0.08f, puerta);

    if (contenedor.marcas > 0)
    {
        if (!DibujarModeloCajasPuertoRetro3D(MODELO_CAJAS_MARCA_GOLPE, { p.x, p.y + 1.22f, p.z }))
            DrawCube({ p.x, p.y + 1.17f, p.z }, 1.3f, 0.03f, 1.3f, Color{ 18, 16, 16, 255 });
    }

    if (contenedor.reforzado && !DibujarModeloCajasPuertoRetro3D(
        MODELO_CAJAS_ANCLA, { p.x, p.y + 1.16f, p.z }))
    {
        const Color oro = { 214, 170, 70, 255 };
        Vector3 ancla = { p.x, p.y + 1.15f, p.z };
        DrawCube({ ancla.x, ancla.y + 0.3f, ancla.z }, 0.12f, 0.6f, 0.12f, oro);
        DrawCube({ ancla.x, ancla.y + 0.5f, ancla.z }, 0.5f, 0.1f, 0.12f, oro);
        DrawSphere({ ancla.x, ancla.y + 0.72f, ancla.z }, 0.12f, oro);
        DrawCube({ ancla.x - 0.22f, ancla.y + 0.08f, ancla.z }, 0.2f, 0.12f, 0.12f, oro);
        DrawCube({ ancla.x + 0.22f, ancla.y + 0.08f, ancla.z }, 0.2f, 0.12f, 0.12f, oro);
    }
}


static void DibujarHumoCajas(const ContenedorCajasPuerto& contenedor)
{
    if (contenedor.humo <= 0.0f)
    {
        return;
    }

    float t = 1.0f - contenedor.humo;
    Vector3 base = { contenedor.posicion.x, 2.4f + t * 1.4f, contenedor.posicion.z };

    if (contenedor.humo > 0.8f)
    {
        (DrawSphere)(base, 0.5f + t * 4.0f, Fade(Color{ 255, 190, 70, 255 }, 0.7f));
    }

    (DrawSphere)(base, 0.6f + t * 1.3f, Fade(Color{ 150, 150, 160, 255 }, contenedor.humo * 0.6f));
    (DrawSphere)(
        { base.x - 0.5f, base.y + 0.3f, base.z + 0.2f },
        0.45f + t,
        Fade(Color{ 110, 110, 120, 255 }, contenedor.humo * 0.55f)
    );
    (DrawSphere)(
        { base.x + 0.5f, base.y + 0.2f, base.z - 0.2f },
        0.4f + t,
        Fade(Color{ 130, 130, 140, 255 }, contenedor.humo * 0.55f)
    );
}


static void DibujarGruaCajas(const MinijuegoCajasPuerto& minijuego)
{
    Color acero = Color{ 224, 164, 40, 255 };
    float gx = minijuego.ganchoX;

    bool porticoGLB = DibujarModeloCajasPuertoRetro3D(MODELO_CAJAS_GRUA_PORTICO, { 0.0f, 0.0f, -5.0f });
    for (int lado = -1; !porticoGLB && lado <= 1; lado += 2)
    {
        for (int fondo = 0; fondo < 2; fondo++)
        {
            DrawCube(
                { 10.3f * (float)lado, 2.55f, -5.0f - 1.2f * (float)fondo },
                0.5f,
                5.1f,
                0.5f,
                acero
            );
        }
    }

    if (!porticoGLB)
        DrawCube({ 0.0f, 5.35f, -5.0f }, 21.0f, 0.5f, 0.8f, acero);
    if (!DibujarModeloCajasPuertoRetro3D(MODELO_CAJAS_GRUA_CARRO, { gx, 5.7f, -5.0f }))
        DrawCube({ gx, 5.7f, -5.0f }, 2.4f, 0.2f, 1.8f, Color{ 70, 74, 82, 255 });
    if (!DibujarModeloCajasPuertoRetro3D(MODELO_CAJAS_GRUA_CABINA, { gx, 5.7f, -5.0f }))
        DrawCubeWires({ gx, 7.2f, -5.0f }, 2.4f, 2.8f, 1.8f, Fade(acero, 0.6f));

    // Brazo hacia los contenedores, cable y gancho.
    float bob = std::sin(minijuego.tiempoAnimacion * 3.0f) * 0.06f;
    float yGancho = ALTURA_GANCHO_CAJAS + bob;

    if (!DibujarModeloCajasPuertoRetro3D(MODELO_CAJAS_GRUA_BRAZO, { gx, 5.55f, -5.0f }))
        DrawCube({ gx, 5.55f, -3.75f }, 0.3f, 0.2f, 2.6f, acero);
    // Cable unitario hacia -Y: mantener su diametro y mover solo el extremo
    // inferior mediante escala Y. Su origen sigue unido a la punta del brazo.
    if (!DibujarModeloCajasPuertoRetro3D(MODELO_CAJAS_GRUA_CABLE,
        { gx, 5.55f, -2.5f }, { 1.0f, 5.55f - yGancho, 1.0f }))
        DrawCylinderEx({ gx, 5.55f, -2.5f }, { gx, yGancho, -2.5f }, 0.04f, 0.04f, 6, LIGHTGRAY);
    if (!DibujarModeloCajasPuertoRetro3D(MODELO_CAJAS_GRUA_GANCHO, { gx, yGancho, -2.5f }))
        DrawCube({ gx, yGancho - 0.15f, -2.5f }, 0.4f, 0.3f, 0.4f, Color{ 90, 94, 100, 255 });

    // Baliza ambar sobre la cabina (gira: se dibuja su halo en las luces).
    (DrawCube)({ gx, 8.72f, -5.0f }, 0.22f, 0.16f, 0.22f, Color{ 60, 56, 50, 255 });
    (DrawSphere)({ gx, 8.86f, -5.0f }, 0.12f, Color{ 255, 170, 60, 255 });

    if (minijuego.fase == FASE_CAJAS_ELEGIR)
    {
        float pulso = 1.0f + 0.03f * std::sin(minijuego.tiempoAnimacion * 12.0f);
        DrawCubeWires(
            minijuego.contenedores[minijuego.cursor].posicion,
            2.1f * pulso,
            2.4f * pulso,
            2.2f * pulso,
            Color{ 255, 226, 96, 255 }
        );
    }

    for (int c = 0; c < CANTIDAD_CONTENEDORES_CAJAS; c++)
    {
        const ContenedorCajasPuerto& contenedor = minijuego.contenedores[c];

        if (contenedor.elegido && minijuego.fase == FASE_CAJAS_ELEGIR)
        {
            DrawCubeWires(contenedor.posicion, 2.0f, 2.3f, 2.1f, Color{ 235, 82, 50, 255 });
        }

        // La carga "chispa": bloque de acero con franjas que cae del gancho.
        if (contenedor.caida > 0.0f)
        {
            float t = 1.0f - contenedor.caida / 0.4f;
            float y = yGancho - 0.5f + (2.6f - yGancho + 0.5f) * t * t;
            Vector3 carga = { contenedor.posicion.x, y, contenedor.posicion.z };
            (DrawCube)(carga, 0.9f, 0.6f, 0.9f, Color{ 66, 70, 78, 255 });
            (DrawCube)(carga, 0.92f, 0.14f, 0.92f, Color{ 255, 140, 50, 255 });
            DrawCubeWires(carga, 0.9f, 0.6f, 0.9f, Fade(BLACK, 0.4f));
            DibujarSombraRetroCircular(carga, 0.55f, 0.55f, 2.25f, 0.4f);
        }
    }
}


//--------------------------------------------------
// Luces falsas, particulas, niebla y marcas
//--------------------------------------------------


// Disco de luz con borde que se apaga (u y v definen el plano).
static void ResplandorCajas(Vector3 centro, Vector3 u, Vector3 v, float radio, Color color)
{
    const int LADOS = 14;
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


static void ResplandorSueloCajas(float x, float y, float z, float radio, Color color)
{
    ResplandorCajas({ x, y, z }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, radio, color);
}


static void ResplandorFrenteCajas(Vector3 centro, float radio, Color color)
{
    ResplandorCajas(centro, { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, radio, color);
}


static unsigned char AlfaCajas(float valor)
{
    return (unsigned char)LimitarCajas(valor, 0.0f, 255.0f);
}


static void DibujarLucesCajas(const MinijuegoCajasPuerto& minijuego)
{
    if (FactorCalidadGrafica(CalidadEfectos()) < 0.5f)
    {
        return;
    }

    float t = minijuego.tiempoAnimacion;

    rlDrawRenderBatchActive();
    rlDisableDepthMask();
    BeginBlendMode(BLEND_ADDITIVE);

    // Faroles: charco calido en el muelle, halo y reflejo temblando en
    // el agua junto al borde.
    const float farolX[4] = { -10.5f, 10.5f, -10.5f, 10.5f };
    const float farolZ[4] = { 1.0f, 1.0f, 6.5f, 6.5f };
    for (int k = 0; k < 4; k++)
    {
        float parpadeo = 0.86f + 0.14f * std::sin(t * 9.0f + (float)k * 1.9f);
        float lado = farolX[k] < 0.0f ? -1.0f : 1.0f;
        ResplandorSueloCajas(farolX[k], 0.05f, farolZ[k], 3.4f * parpadeo, Color{ 255, 170, 80, 64 });
        ResplandorFrenteCajas({ farolX[k], 3.3f, farolZ[k] + 0.3f }, 1.3f * parpadeo, Color{ 255, 200, 110, 105 });

        for (int j = 0; j < 4; j++)
        {
            float ancho = 0.55f + 0.22f * std::sin(t * 3.0f + (float)j * 1.3f + (float)k);
            ResplandorSueloCajas(
                lado * (11.8f + 0.95f * (float)j),
                -0.83f,
                farolZ[k] + 0.3f * std::sin(t * 2.0f + (float)j),
                ancho,
                Color{ 255, 190, 100, (unsigned char)(70 - 14 * j) }
            );
        }
    }

    // Luces del barco: columna de reflejos sobre el agua hacia la camara.
    for (int i = 0; i < 4; i++)
    {
        float x = 6.2f + 1.2f * (float)i;
        for (int j = 0; j < 6; j++)
        {
            float ancho = 0.35f + 0.2f * std::sin(t * 2.6f + (float)i + (float)j * 0.9f);
            ResplandorSueloCajas(
                x + 0.15f * std::sin(t * 1.7f + (float)j),
                -0.83f,
                -13.8f + 1.15f * (float)j,
                ancho,
                Color{ 255, 220, 140, (unsigned char)(60 - 8 * j) }
            );
        }
    }

    // Baliza ambar de la grua.
    float giro = 0.5f + 0.5f * std::sin(t * 6.0f);
    ResplandorFrenteCajas(
        { minijuego.ganchoX, 8.86f, -4.85f },
        0.5f + 0.9f * giro,
        Color{ 255, 160, 50, AlfaCajas(80.0f + 100.0f * giro) }
    );

    // Foco de la grua sobre el contenedor apuntado (decision publica).
    if (minijuego.fase == FASE_CAJAS_ELEGIR)
    {
        Vector3 c = minijuego.contenedores[minijuego.cursor].posicion;
        float yGancho = ALTURA_GANCHO_CAJAS + std::sin(t * 3.0f) * 0.06f;
        ResplandorSueloCajas(c.x, 0.06f, c.z + 1.6f, 1.8f, Color{ 255, 226, 96, 90 });
        ResplandorSueloCajas(c.x, 2.27f, c.z, 1.3f, Color{ 255, 226, 96, 70 });
        DrawCylinderEx(
            { minijuego.ganchoX, yGancho - 0.3f, -2.5f },
            { c.x, 2.3f, c.z },
            0.08f,
            0.95f,
            12,
            Color{ 255, 226, 96, 26 }
        );

        for (int k = 0; k < CANTIDAD_CONTENEDORES_CAJAS; k++)
        {
            const ContenedorCajasPuerto& elegido = minijuego.contenedores[k];
            if (!elegido.elegido) continue;
            ResplandorSueloCajas(
                elegido.posicion.x,
                0.06f,
                elegido.posicion.z + 1.6f,
                1.6f,
                Color{ 255, 70, 40, AlfaCajas(70.0f + 40.0f * std::sin(t * 10.0f)) }
            );
        }
    }

    // Destello del golpe: rojo si aplasto, dorado si el ancla salvo,
    // azulado si estaba vacio.
    for (int k = 0; k < CANTIDAD_CONTENEDORES_CAJAS; k++)
    {
        const ContenedorCajasPuerto& c = minijuego.contenedores[k];
        if (c.sacudida <= 0.0f) continue;

        Color color = c.tipoGolpe == 1
            ? Color{ 255, 70, 40, 255 }
            : (c.tipoGolpe == 2 ? Color{ 255, 210, 90, 255 } : Color{ 170, 190, 230, 255 });
        float s = c.sacudida;
        color.a = AlfaCajas(160.0f * s);
        ResplandorSueloCajas(c.posicion.x, 0.06f, c.posicion.z + 1.3f, 2.0f + 1.4f * (1.0f - s), color);
        color.a = AlfaCajas(130.0f * s);
        ResplandorFrenteCajas({ c.posicion.x, 2.4f, c.posicion.z + 1.1f }, 1.4f + 1.2f * (1.0f - s), color);
    }

    // Polizones descubiertos: luz verde si siguen en juego, roja si no.
    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (
            i == minijuego.indiceSolo ||
            !minijuego.resultado.participantes[i].participo ||
            minijuego.estadosJugadores[i].contenedor < 0
        )
        {
            continue;
        }

        const ContenedorCajasPuerto& c = minijuego.contenedores[minijuego.estadosJugadores[i].contenedor];
        if (!c.revelado) continue;

        Color color = minijuego.estadosJugadores[i].vivo
            ? Color{ 90, 230, 120, 80 }
            : Color{ 255, 70, 40, 90 };
        ResplandorSueloCajas(c.posicion.x, 0.06f, c.posicion.z + 1.3f, 1.2f, color);
    }

    rlDrawRenderBatchActive();
    EndBlendMode();
    rlEnableDepthMask();
}


static void DibujarParticulasCajas(const MinijuegoCajasPuerto& minijuego)
{
    for (int i = 0; i < MAX_PARTICULAS_CAJAS; i++)
    {
        const ParticulaCajasPuerto& p = minijuego.particulas[i];
        if (!p.activa) continue;

        float vida = p.vida / p.vidaMaxima;
        bool polvo = p.gravedad < 0.0f;
        float tamano = polvo ? p.tamano * (1.7f - 0.7f * vida) : p.tamano;
        Color color = p.color;
        color.a = (unsigned char)((float)p.color.a * (polvo ? vida : (vida > 0.3f ? 1.0f : vida / 0.3f)));

        DrawCubeV(p.posicion, { tamano, tamano, tamano }, color);
    }
}


// Niebla del muelle: capas translucidas que tapan el patio desde la
// salida hasta el frente de los contenedores, con bancos que derivan.
static void DibujarNieblaCajas(const MinijuegoCajasPuerto& minijuego)
{
    float n = minijuego.niebla;

    if (n <= 0.01f)
    {
        return;
    }

    float t = minijuego.tiempoAnimacion;

    rlDrawRenderBatchActive();
    rlDisableDepthMask();

    const float alfas[4] = { 0.66f, 0.48f, 0.34f, 0.2f };
    for (int k = 0; k < 4; k++)
    {
        (DrawCube)(
            { 0.0f, 0.225f + 0.45f * (float)k, 2.3f },
            22.6f,
            0.45f,
            7.2f,
            Fade(Color{ 56, 72, 96, 255 }, alfas[k] * n)
        );
    }

    if (CalidadDecoracion() != CALIDAD_BAJA)
    {
        int bancos = CalidadDecoracion() == CALIDAD_ALTA ? 16 : 9;
        for (int i = 0; i < bancos; i++)
        {
            float semilla = (float)i * 12.9898f;
            float h = std::sin(semilla) * 43758.5453f;
            h -= std::floor(h);
            float velocidad = 0.25f + 0.35f * h;
            float x = std::fmod(h * 24.0f + t * velocidad, 24.0f) - 12.0f;
            float z = -0.8f + 6.0f * std::fmod(h * 7.31f, 1.0f);
            float radio = 1.0f + 0.7f * std::fmod(h * 3.7f, 1.0f);

            rlPushMatrix();
            rlTranslatef(x, 1.65f + 0.12f * std::sin(t * 0.8f + semilla), z);
            rlScalef(1.0f, 0.32f, 1.0f);
            (DrawSphere)({ 0.0f, 0.0f, 0.0f }, radio, Fade(Color{ 84, 102, 128, 255 }, 0.2f * n));
            rlPopMatrix();
        }
    }

    rlDrawRenderBatchActive();
    rlEnableDepthMask();
}


// Marca minima de un polizon humano sobre la niebla: disco y flecha de su
// color, sin numero ni rotulo. No marca ningun contenedor.
static void DibujarMarcaPolizonCajas(Vector3 posicion, Color color, float t, float fase)
{
    Vector3 base = { posicion.x, 1.96f, posicion.z };
    float rebote = std::sin(t * 5.0f + fase) * 0.06f;

    (DrawCylinder)(base, 0.42f, 0.42f, 0.03f, 16, Fade(color, 0.45f));
    DrawCircle3D({ base.x, base.y + 0.04f, base.z }, 0.42f, { 1.0f, 0.0f, 0.0f }, 90.0f, color);
    DrawCylinderEx(
        { base.x, base.y + 0.2f + rebote, base.z },
        { base.x, base.y + 0.55f + rebote, base.z },
        0.0f,
        0.2f,
        10,
        color
    );
}


void MinijuegoCajasPuerto::Dibujar(
    const JugadorPrueba jugadores[],
    int cantidadMaxima,
    const Participante participantes[],
    bool mostrarDebug
) const
{
    (void)cantidadMaxima;

    ClearBackground(Color{ 8, 14, 30, 255 });
    BeginMode3D(camara);

    DibujarAguaCajas(tiempoAnimacion);
    DibujarFondoCajas(tiempoAnimacion);
    DibujarMuelleCajas();

    for (int c = 0; c < CANTIDAD_CONTENEDORES_CAJAS; c++)
    {
        DibujarContenedorCajas(contenedores[c], c);

        if (mostrarDebug)
        {
            DrawCubeWires(contenedores[c].posicion, 1.9f, 2.2f, 2.0f, LIME);
        }
    }

    DibujarGruaCajas(*this);

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!resultado.participantes[i].participo)
        {
            continue;
        }

        Participante participanteVisual = participantes[i];
        participanteVisual.conectado = true;

        if (i == indiceSolo)
        {
            DibujarJugadorCuboPrueba(jugadores[i], participanteVisual);
            continue;
        }

        const EstadoJugadorCajasPuerto& estado = estadosJugadores[i];
        JugadorPrueba visual = jugadores[i];

        if (estado.contenedor >= 0)
        {
            const ContenedorCajasPuerto& contenedor = contenedores[estado.contenedor];

            if (!contenedor.revelado)
            {
                continue;
            }

            visual.posicion = { contenedor.posicion.x, visual.tamano.y * 0.5f, contenedor.posicion.z + 0.2f };
            visual.aplastado = !estado.vivo;
            visual.tiempoAplastado = 1.0f;
        }
        else if (!estado.vivo)
        {
            continue;
        }
        else if (fase == FASE_CAJAS_ESCONDER || fase == FASE_CAJAS_CIERRE)
        {
            // Dentro de la niebla: ni cuerpo, ni sombra, ni anillo.
            continue;
        }

        DibujarJugadorCuboPrueba(visual, participanteVisual);

        if (mostrarDebug)
        {
            DrawBoundingBox(CrearHitboxJugadorPrueba(visual), LIME);
        }
    }

    for (int c = 0; c < CANTIDAD_CONTENEDORES_CAJAS; c++)
    {
        DibujarHumoCajas(contenedores[c]);
    }

    DibujarParticulasCajas(*this);
    DibujarLucesCajas(*this);
    DibujarNieblaCajas(*this);

    // Marcas propias de los polizones humanos (y las falsas si el gruero
    // tambien es humano), solo mientras caminan en la niebla.
    if (fase == FASE_CAJAS_ESCONDER)
    {
        bool gruaHumana = indiceSolo >= 0 && !EsControlBotCajas(participantes[indiceSolo]);

        for (int i = 0; i < MAX_PARTICIPANTES; i++)
        {
            if (
                i == indiceSolo ||
                !resultado.participantes[i].participo ||
                !estadosJugadores[i].vivo ||
                estadosJugadores[i].contenedor >= 0 ||
                EsControlBotCajas(participantes[i])
            )
            {
                continue;
            }

            DibujarMarcaPolizonCajas(jugadores[i].posicion, participantes[i].color, tiempoAnimacion, (float)i);

            for (int s = 0; gruaHumana && s < SENUELOS_POR_JUGADOR_CAJAS; s++)
            {
                DibujarMarcaPolizonCajas(
                    senuelos[i][s].posicion,
                    participantes[i].color,
                    tiempoAnimacion,
                    (float)i
                );
            }
        }
    }

    EndMode3D();

    // Numeros de contenedor (la eleccion de la grua es publica).
    int tamanoNumero = TamanoTexto(UI_TEXTO_GRANDE);
    for (int c = 0; c < CANTIDAD_CONTENEDORES_CAJAS; c++)
    {
        Vector2 pantalla = GetWorldToScreen(
            { contenedores[c].posicion.x, 2.8f, contenedores[c].posicion.z + 1.0f },
            camara
        );
        char numero[4];
        std::snprintf(numero, sizeof(numero), "%d", c + 1);

        bool apuntado = fase == FASE_CAJAS_ELEGIR && c == cursor;
        bool marcado = contenedores[c].elegido &&
            (fase == FASE_CAJAS_ELEGIR || fase == FASE_CAJAS_RESOLVER);
        Color color = apuntado ? UI_FOCO : (marcado ? UI_PELIGRO : UI_TEXTO_MUNDO);

        DibujarTextoMundoContornoUI(numero, pantalla.x, pantalla.y - (float)tamanoNumero * 0.5f, UI_TEXTO_GRANDE, color);
    }

    // Rotulos J# sobre los polizones ya descubiertos.
    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (
            i == indiceSolo ||
            !resultado.participantes[i].participo ||
            estadosJugadores[i].contenedor < 0 ||
            !contenedores[estadosJugadores[i].contenedor].revelado
        )
        {
            continue;
        }

        const ContenedorCajasPuerto& c = contenedores[estadosJugadores[i].contenedor];
        Vector2 pantalla = GetWorldToScreen({ c.posicion.x, 1.75f, c.posicion.z + 1.1f }, camara);
        char rotulo[8];
        std::snprintf(rotulo, sizeof(rotulo), "J%d", participantes[i].numeroJugador);

        OpcionesEtiquetaUI opciones;
        opciones.borde = participantes[i].color;
        opciones.pico = true;
        opciones.alfa = estadosJugadores[i].vivo ? 1.0f : 0.7f;
        DibujarEtiquetaUI(rotulo, pantalla, -5.0f, opciones);
    }

    // Cartel flotante del golpe.
    for (int c = 0; c < CANTIDAD_CONTENEDORES_CAJAS; c++)
    {
        const ContenedorCajasPuerto& contenedor = contenedores[c];
        if (contenedor.aviso <= 0.0f) continue;

        float subida = (1.0f - contenedor.aviso) * 0.9f;
        Vector2 pantalla = GetWorldToScreen(
            { contenedor.posicion.x, 3.6f + subida, contenedor.posicion.z + 1.0f },
            camara
        );
        const char* texto = contenedor.tipoGolpe == 1 ? "¡APLASTADO!"
            : (contenedor.tipoGolpe == 2 ? "¡A SALVO!" : "VACIA");
        Color color = contenedor.tipoGolpe == 1 ? UI_PELIGRO
            : (contenedor.tipoGolpe == 2 ? UI_FOCO : UI_TEXTO_MUNDO);
        float alfa = contenedor.aviso * 3.0f;

        DibujarTextoMundoContornoUI(texto, pantalla.x, pantalla.y, UI_TEXTO_GRANDE, color, alfa > 1.0f ? 1.0f : alfa);
    }

    // Anuncio de la fase, debajo del tendedero.
    {
        char anuncio[64] = "";
        Color color = UI_FOCO;
        float alfa = 1.0f;

        if (fase == FASE_CAJAS_ESCONDER && tiempoAnuncio < 2.2f)
        {
            std::snprintf(anuncio, sizeof(anuncio), "¡A ESCONDERSE! RONDA %d DE %d", ronda, RONDAS_CAJAS);
            alfa = tiempoAnuncio < 1.7f ? 1.0f : (2.2f - tiempoAnuncio) / 0.5f;
        }
        else if (fase == FASE_CAJAS_CIERRE)
        {
            std::snprintf(anuncio, sizeof(anuncio), "¡PUERTAS CERRADAS!");
            color = UI_TEXTO_MUNDO;
        }
        else if (fase == FASE_CAJAS_ELEGIR)
        {
            if (eleccionesRestantes == 1)
                std::snprintf(anuncio, sizeof(anuncio), "LA GRÚA ELIGE: QUEDA 1 CARGA");
            else
                std::snprintf(anuncio, sizeof(anuncio), "LA GRÚA ELIGE: QUEDAN %d CARGAS", eleccionesRestantes);
        }
        else if (fase == FASE_CAJAS_RESOLVER)
        {
            if (!revelacionHecha)
            {
                int paso = pasoResolucion < cantidadElegidos ? pasoResolucion + 1 : cantidadElegidos;
                std::snprintf(anuncio, sizeof(anuncio), "CARGA %d DE %d", paso, cantidadElegidos);
                color = UI_TEXTO_MUNDO;
            }
            else if (eliminadosRonda == 0)
            {
                std::snprintf(anuncio, sizeof(anuncio), "¡NADIE APLASTADO EN LA RONDA %d!", ronda);
                color = UI_POSITIVO;
            }
            else if (eliminadosRonda == 1)
            {
                std::snprintf(anuncio, sizeof(anuncio), "RONDA %d: 1 POLIZÓN APLASTADO", ronda);
                color = UI_PELIGRO;
            }
            else
            {
                std::snprintf(anuncio, sizeof(anuncio), "RONDA %d: %d POLIZONES APLASTADOS", ronda, eliminadosRonda);
                color = UI_PELIGRO;
            }
        }

        if (anuncio[0] != '\0')
        {
            DibujarTextoMundoContornoUI(anuncio, (float)GetScreenWidth() * 0.5f, Px(150.0f), UI_TEXTO_GRANDE, color, alfa);
        }
    }

    // HUD comun: el gruero a la izquierda del reloj, los polizones a la
    // derecha. Nada del HUD indica en que contenedor esta nadie.
    int polizones = 0;
    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (i != indiceSolo && resultado.participantes[i].participo) polizones++;
    }

    DatosHudMinijuego hud;
    PrepararDatosHudMinijuego(
        hud,
        "CAJAS DEL PUERTO",
        "1 VS 3: LOS POLIZONES SE ESCONDEN Y LA GRÚA APLASTA CONTENEDORES. EL ANCLA DORADA PROTEGE.",
        resultado
    );

    hud.fase = fase == FASE_CAJAS_PREPARACION
        ? HUD_MINIJUEGO_CUENTA
        : (fase == FASE_CAJAS_TERMINADO ? HUD_MINIJUEGO_FIN : HUD_MINIJUEGO_JUGANDO);
    hud.cuentaRestante = tiempoPreparacion;
    hud.finPorTiempo = false;

    if (fase == FASE_CAJAS_ESCONDER || fase == FASE_CAJAS_ELEGIR)
    {
        hud.reloj = HUD_RELOJ_CUENTA_ATRAS;
        hud.tiempoRestante = tiempoFase > 0.0f ? tiempoFase : 0.0f;
        hud.tiempoTotal = fase == FASE_CAJAS_ESCONDER ? DURACION_ESCONDER_CAJAS : DURACION_ELEGIR_CAJAS;
        hud.segundosAlerta = 3.0f;
    }
    else
    {
        hud.reloj = HUD_RELOJ_MEDIDOR;
        hud.etiquetaMedidor = "RONDA";
        std::snprintf(hud.valorMedidor, sizeof(hud.valorMedidor), "%d/%d", ronda, RONDAS_CAJAS);
        hud.progresoMedidor = (float)ronda / (float)RONDAS_CAJAS;
    }

    char nota[64] = "";
    if (fase == FASE_CAJAS_TERMINADO && resultado.estado == RESULTADO_MINIJUEGO_FINALIZADO && indiceSolo >= 0)
    {
        bool ganaSolo = resultado.participantes[indiceSolo].posicionFinal == 1;
        int vivos = ContarVivosCajas(*this);

        if (ganaSolo)
            hud.tituloFinal = "¡GANA EL GRUERO!";
        else if (polizones == 3)
            hud.tituloFinal = "¡GANA EL TRIO!";
        else if (polizones == 1)
            hud.tituloFinal = "¡GANA EL POLIZÓN!";
        else
            hud.tituloFinal = "¡GANAN LOS POLIZONES!";

        if (ganaSolo)
            std::snprintf(nota, sizeof(nota), "LA GRÚA APLASTÓ A TODOS LOS POLIZONES.");
        else if (vivos == 1)
            std::snprintf(nota, sizeof(nota), "1 POLIZÓN SOBREVIVIÓ LAS %d RONDAS.", RONDAS_CAJAS);
        else
            std::snprintf(nota, sizeof(nota), "%d POLIZONES SOBREVIVIERON LAS %d RONDAS.", vivos, RONDAS_CAJAS);
        hud.notaFinal = nota;
    }

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        JugadorHudMinijuego& jugador = hud.jugadores[i];
        if (!jugador.visible) continue;

        const Participante& participante = participantes[i];

        if (i == indiceSolo)
        {
            jugador.equipo = 0;
            jugador.rol = "GRUERO";
            jugador.valorNumerico = eliminadosTotal;
            std::snprintf(jugador.valor, sizeof(jugador.valor), "%d/%d", eliminadosTotal, polizones);
            if (eliminadosTotal == 1)
                std::snprintf(jugador.detalleFinal, sizeof(jugador.detalleFinal), "1 APLASTADO");
            else
                std::snprintf(jugador.detalleFinal, sizeof(jugador.detalleFinal), "%d APLASTADOS", eliminadosTotal);

            if (!participante.esBot)
            {
                // La grua solo va a izquierda/derecha.
                const char* mover =
                    participante.control == CONTROL_GAMEPAD ? "STICK"
                    : (participante.control == CONTROL_TECLADO_FLECHAS ? "FLECHAS"
                    : (participante.control == CONTROL_TECLADO_WASD ? "A D" : "A D / FLECHAS"));

                std::snprintf(
                    jugador.controles,
                    sizeof(jugador.controles),
                    "GRÚA: %s\nSOLTAR: %s",
                    mover,
                    TextoBotonGolpeHud(participante)
                );
            }
            continue;
        }

        // Solo datos que ya son publicos: rondas superadas y si cayo.
        const EstadoJugadorCajasPuerto& estado = estadosJugadores[i];
        jugador.equipo = 1;
        jugador.rol = "POLIZÓN";
        jugador.valorNumerico = estado.rondasSobrevividas;
        jugador.eliminado = !estado.vivo;
        std::snprintf(jugador.valor, sizeof(jugador.valor), "%d/%d", estado.rondasSobrevividas, RONDAS_CAJAS);
        std::snprintf(jugador.detalleFinal, sizeof(jugador.detalleFinal), "%d/%d RONDAS", estado.rondasSobrevividas, RONDAS_CAJAS);

        if (!participante.esBot)
        {
            std::snprintf(
                jugador.controles,
                sizeof(jugador.controles),
                "%s\nENTRAR: %s",
                TextoMoverHud(participante),
                TextoBotonGolpeHud(participante)
            );
        }
    }

    DibujarHudMinijuego(hud, participantes);
}


const ResultadoMinijuego&
MinijuegoCajasPuerto::ObtenerResultado() const
{
    return resultado;
}
