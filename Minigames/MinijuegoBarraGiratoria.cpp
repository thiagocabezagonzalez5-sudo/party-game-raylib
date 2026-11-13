#include "Minigames/MinijuegoBarraGiratoria.h"

#include "Minigames/AudioMinijuegos.h"
#include "Minigames/BotsMinijuegos1v3.h"
#include "Minigames/EfectosVisualesMinijuegos.h"
#include "Minigames/HudMinijuego.h"
#include "Minigames/MecanicasJugador.h"
#include "Minigames/ModeloJugadorCompartido.h"
#include "Minigames/UtilidadesMinijuegos.h"
#include "Systems/CalidadGrafica.h"
#include "UI/EstiloUI.h"

#include "rlgl.h"

#include <cmath>
#include <cstdio>


static const float DURACION_PREPARACION_BARRA = 3.0f;
static const float RADIO_ARENA_BARRA = 5.4f;
static const float LONGITUD_MEDIA_BARRA = 5.65f;
static const float ALTURA_BARRA = 0.82f;
static const float RADIO_COLISION_BARRA = 0.34f;

static const float FUERZA_IMPACTO_BARRA = 12.8f;
static const float FUERZA_IMPACTO_BARRA_SUPERIOR = 14.6f;
static const float IMPULSO_VERTICAL_BARRA = 7.0f;
static const float IMPULSO_VERTICAL_BARRA_SUPERIOR = 7.6f;
static const float COOLDOWN_IMPACTO_BARRA = 0.42f;
static const float DURACION_STUN_BARRA = 0.36f;
static const float DURACION_RALENTIZACION_BARRA = 0.78f;
static const float MULTIPLICADOR_RALENTIZACION_BARRA = 0.68f;

static const float TIEMPO_APARICION_SEGUNDA_BARRA = 10.0f;
static const float ALTURA_INICIAL_SEGUNDA_BARRA = 8.0f;
// La barra superior queda por encima de la cabeza de un jugador parado. Solo
// entra en su hitbox cuando el jugador salta o queda elevado por un impacto.
static const float ALTURA_FINAL_SEGUNDA_BARRA = 2.38f;
static const float VELOCIDAD_CAIDA_SEGUNDA_BARRA = 7.2f;
static const float MULTIPLICADOR_VELOCIDAD_SEGUNDA_BARRA = 1.5f;


static float MagnitudHorizontalBarra(float x, float z)
{
    return std::sqrt(x * x + z * z);
}


static bool JugadorSobreArenaBarra(
    const JugadorPrueba& jugador
)
{
    float distancia =
        MagnitudHorizontalBarra(
            jugador.posicion.x,
            jugador.posicion.z
        );

    float margen = jugador.tamano.x * 0.18f;

    return distancia <= RADIO_ARENA_BARRA - margen;
}


static int ContarVivosBarra(
    const MinijuegoBarraGiratoria& minijuego
)
{
    int vivos = 0;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (
            minijuego.resultado.participantes[i].participo &&
            !minijuego.estadosJugadores[i].eliminado
        )
        {
            vivos++;
        }
    }

    return vivos;
}


static float DistanciaJugadorALineaBarra(
    const JugadorPrueba& jugador,
    float angulo
)
{
    float direccionX = std::cos(angulo);
    float direccionZ = std::sin(angulo);

    float proyeccion =
        jugador.posicion.x * direccionX +
        jugador.posicion.z * direccionZ;

    if (proyeccion < -LONGITUD_MEDIA_BARRA)
        proyeccion = -LONGITUD_MEDIA_BARRA;

    if (proyeccion > LONGITUD_MEDIA_BARRA)
        proyeccion = LONGITUD_MEDIA_BARRA;

    float puntoX = direccionX * proyeccion;
    float puntoZ = direccionZ * proyeccion;

    float dx = jugador.posicion.x - puntoX;
    float dz = jugador.posicion.z - puntoZ;

    return std::sqrt(dx * dx + dz * dz);
}


static bool BarraTocaJugador(
    const JugadorPrueba& jugador,
    float angulo,
    float altura
)
{
    float pies =
        jugador.posicion.y -
        jugador.tamano.y / 2.0f;

    float cabeza =
        jugador.posicion.y +
        jugador.tamano.y / 2.0f;

    bool solapaVertical =
        cabeza >= altura - RADIO_COLISION_BARRA &&
        pies <= altura + RADIO_COLISION_BARRA;

    if (!solapaVertical)
    {
        return false;
    }

    return
        DistanciaJugadorALineaBarra(jugador, angulo) <=
        RADIO_COLISION_BARRA + jugador.tamano.x * 0.34f;
}


static void AplicarImpactoBarra(
    JugadorPrueba& jugador,
    EstadoJugadorBarraGiratoria& estadoJugador,
    float alturaImpacto,
    float fuerza,
    float impulsoVertical,
    ParticulaTierra particulas[],
    int cantidadParticulas
)
{
    float distancia =
        MagnitudHorizontalBarra(
            jugador.posicion.x,
            jugador.posicion.z
        );

    float normalX = 1.0f;
    float normalZ = 0.0f;

    if (distancia > 0.05f)
    {
        normalX = jugador.posicion.x / distancia;
        normalZ = jugador.posicion.z / distancia;
    }

    jugador.empuje.x += normalX * fuerza;
    jugador.empuje.z += normalZ * fuerza;
    jugador.velocidad.y = impulsoVertical;
    jugador.enSuelo = false;

    if (jugador.tiempoRalentizado < DURACION_RALENTIZACION_BARRA)
    {
        jugador.tiempoRalentizado = DURACION_RALENTIZACION_BARRA;
    }

    if (
        jugador.multiplicadorRalentizacion <= 0.0f ||
        jugador.multiplicadorRalentizacion > MULTIPLICADOR_RALENTIZACION_BARRA
    )
    {
        jugador.multiplicadorRalentizacion =
            MULTIPLICADOR_RALENTIZACION_BARRA;
    }

    estadoJugador.tiempoStunBarra = DURACION_STUN_BARRA;
    estadoJugador.cooldownImpacto = COOLDOWN_IMPACTO_BARRA;

    CrearParticulasImpactoGolpe(
        particulas,
        cantidadParticulas,
        {
            jugador.posicion.x,
            alturaImpacto,
            jugador.posicion.z
        }
    );
}


// Tiempo hasta que el borde delantero de una barra alcanza la posicion
// angular del jugador, y duracion del cruce sobre ese punto.
static void CalcularCruceBarraBot(
    float anguloBarra,
    float omega,
    float theta,
    float radioSeguro,
    float radioContacto,
    float& tiempoContacto,
    float& duracionCruce
)
{
    float delta = std::fmod(theta - anguloBarra, PI);
    if (delta < 0.0f) delta += PI;

    float razon = radioContacto / radioSeguro;
    float mitadAngulo = std::asin(razon < 1.0f ? razon : 1.0f);

    tiempoContacto = (delta - mitadAngulo) / omega;
    if (delta > PI - mitadAngulo) tiempoContacto = 0.0f;

    duracionCruce = 2.0f * mitadAngulo / omega;
}


// IA simple: cada bot orbita a un radio propio (lejos del centro, donde la
// barra pasa mas rapido y deja menos tiempo de contacto). Mientras la barra
// es lenta corre por delante de ella; cuando se vuelve rapida salta justo
// antes del cruce. El instante del salto lleva un error aleatorio, asi que
// algunos fallan y la partida siempre termina con un ganador.
static InputMinijuegoParticipante CrearEntradaBotBarra(
    MinijuegoBarraGiratoria& minijuego,
    int indice,
    const JugadorPrueba& jugador,
    float deltaTime
)
{
    EstadoJugadorBarraGiratoria& bot = minijuego.estadosJugadores[indice];
    InputMinijuegoParticipante entrada{};

    if (bot.radioOrbitaBot <= 0.0f)
    {
        bot.radioOrbitaBot = (float)GetRandomValue(390, 450) / 100.0f;
        bot.errorSaltoBot = (float)GetRandomValue(-100, 100) / 1000.0f;
    }

    if (bot.enfriamientoSaltoBot > 0.0f)
    {
        bot.enfriamientoSaltoBot -= deltaTime;
    }

    float x = jugador.posicion.x;
    float z = jugador.posicion.z;
    float radio = MagnitudHorizontalBarra(x, z);
    float theta = std::atan2(z, x);

    float radioSeguro = radio > 0.8f ? radio : 0.8f;
    float radioContacto = RADIO_COLISION_BARRA + jugador.tamano.x * 0.34f;

    float omega = minijuego.velocidadAngular;
    float tiempoContacto = 0.0f;
    float duracionCruce = 0.0f;

    CalcularCruceBarraBot(
        minijuego.anguloBarra,
        omega,
        theta,
        radioSeguro,
        radioContacto,
        tiempoContacto,
        duracionCruce
    );

    bool barraRapida = omega * radioSeguro > 3.9f;

    float anguloObjetivo = theta;

    if (!barraRapida && tiempoContacto < 1.1f)
    {
        anguloObjetivo = theta + 0.40f;
    }

    InputMinijuegoParticipante movimiento =
        CrearEntradaBotHaciaObjetivo1v3(
            jugador.posicion,
            {
                std::cos(anguloObjetivo) * bot.radioOrbitaBot,
                0.0f,
                std::sin(anguloObjetivo) * bot.radioOrbitaBot
            },
            0.16f
        );

    entrada = movimiento;

    // Instante ideal de salto respecto de la barra base.
    float saltoIdeal = tiempoContacto + duracionCruce * 0.5f - 0.40f;
    float desfase = 0.0f;

    // La segunda barra solo golpea a quien esta en el aire (de 0.10 a 0.70 s
    // tras saltar). Si su cruce coincide con ese tramo, el bot adelanta o
    // retrasa el salto dentro de la holgura que deja la barra base.
    if (barraRapida && minijuego.segundaBarraLista)
    {
        float omega2 = minijuego.velocidadAngularSegunda;
        float contacto2 = 0.0f;
        float cruce2 = 0.0f;

        CalcularCruceBarraBot(
            minijuego.anguloSegundaBarra,
            omega2,
            theta,
            radioSeguro,
            radioContacto,
            contacto2,
            cruce2
        );

        float holgura = 0.35f - duracionCruce;
        if (holgura < 0.0f) holgura = 0.0f;

        const float candidatos[3] = { 0.0f, holgura * 0.4f, -holgura * 0.4f };

        for (int c = 0; c < 3; c++)
        {
            float salto = saltoIdeal + candidatos[c];
            if (salto < 0.0f) salto = 0.0f;

            bool choca = false;

            for (int vuelta = 0; vuelta < 3; vuelta++)
            {
                float inicio = contacto2 + (float)vuelta * PI / omega2;

                if (inicio <= salto + 0.70f && inicio + cruce2 >= salto + 0.10f)
                {
                    choca = true;
                    break;
                }
            }

            if (!choca)
            {
                desfase = candidatos[c];
                break;
            }
        }
    }

    if (
        barraRapida &&
        jugador.enSuelo &&
        bot.enfriamientoSaltoBot <= 0.0f &&
        saltoIdeal + desfase + bot.errorSaltoBot <= 0.0f
    )
    {
        entrada.saltar = true;
        bot.enfriamientoSaltoBot = 0.62f;
        bot.errorSaltoBot = (float)GetRandomValue(-100, 100) / 1000.0f;
    }

    return entrada;
}


static void FinalizarBarra(
    MinijuegoBarraGiratoria& minijuego
)
{
    if (
        minijuego.resultado.estado !=
        RESULTADO_MINIJUEGO_EN_CURSO
    )
    {
        return;
    }

    int vivos = ContarVivosBarra(minijuego);

    minijuego.resultado.estado = RESULTADO_MINIJUEGO_FINALIZADO;
    minijuego.resultado.desenlace =
        vivos == 1
            ? DESENLACE_CON_GANADOR
            : DESENLACE_EMPATE;

    int tiempoFinalMs =
        (int)std::lround(
            minijuego.tiempoJugado * 1000.0f
        );

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        ResultadoParticipante& resultadoJugador =
            minijuego.resultado.participantes[i];

        if (!resultadoJugador.participo)
        {
            continue;
        }

        EstadoJugadorBarraGiratoria& estadoJugador =
            minijuego.estadosJugadores[i];

        if (!estadoJugador.eliminado)
        {
            estadoJugador.posicionFinal = 1;
            estadoJugador.tiempoSobrevividoMs = tiempoFinalMs;
        }

        resultadoJugador.posicionFinal = estadoJugador.posicionFinal;
        resultadoJugador.numeroEquipo = -1;
        resultadoJugador.puntuacionMinijuego =
            estadoJugador.tiempoSobrevividoMs;
        resultadoJugador.puntosObtenidos = 0;
    }

    minijuego.fase = FASE_BARRA_TERMINADO;
}


//==================================================
// EFECTOS VISUALES (sin efecto en la logica)
//==================================================

// Altura del mar de nubes sobre el que flota el templo.
static const float NIVEL_NUBES_BARRA = -16.0f;

// Acento de la escena: la energia celeste de cada barra.
static const Color COLOR_ACENTO_BARRA_BASE = { 96, 236, 255, 255 };
static const Color COLOR_ACENTO_BARRA_SUPERIOR = { 255, 112, 214, 255 };

static const float DURACION_AVISO_BARRA = 1.6f;


static float Limitar01Barra(float valor)
{
    if (valor < 0.0f) return 0.0f;
    if (valor > 1.0f) return 1.0f;
    return valor;
}


// Hash determinista para la decoracion (misma escena en cada partida).
static float Hash01Barra(int a, int b)
{
    unsigned int h = (unsigned int)a * 374761393u + (unsigned int)b * 668265263u;
    h = (h ^ (h >> 13)) * 1274126177u;
    h ^= h >> 16;
    return (float)(h & 0xFFFFu) / 65535.0f;
}


// Azar propio para particulas: no consume el GetRandomValue de los bots.
static float AzarBarra(MinijuegoBarraGiratoria& minijuego, float minimo, float maximo)
{
    unsigned int& s = minijuego.semillaVisual;
    s ^= s << 13;
    s ^= s >> 17;
    s ^= s << 5;
    return minimo + (maximo - minimo) * (float)(s & 0xFFFFFFu) / 16777215.0f;
}


static int CantidadParticulasBarra(int base)
{
    int cantidad = (int)std::lround((float)base * FactorCalidadGrafica(CalidadParticulas()));
    return cantidad < 1 ? 1 : cantidad;
}


static void EmitirParticulaBarra(
    MinijuegoBarraGiratoria& minijuego,
    Vector3 posicion,
    Vector3 velocidad,
    float vida,
    float tamano,
    float gravedad,
    float freno,
    Color color
)
{
    for (int i = 0; i < MAX_PARTICULAS_BARRA; i++)
    {
        ParticulaBarraGiratoria& p = minijuego.particulasEfecto[i];

        if (p.activa)
        {
            continue;
        }

        p.activa = true;
        p.posicion = posicion;
        p.velocidad = velocidad;
        p.vida = vida;
        p.vidaMaxima = vida;
        p.tamano = tamano;
        p.gravedad = gravedad;
        p.freno = freno;
        p.color = color;
        return;
    }
}


static void AgregarOndaBarra(
    MinijuegoBarraGiratoria& minijuego,
    Vector3 posicion,
    float radioMaximo,
    float vida,
    Color color
)
{
    for (int i = 0; i < MAX_ONDAS_BARRA; i++)
    {
        OndaBarraGiratoria& onda = minijuego.ondas[i];

        if (onda.activa)
        {
            continue;
        }

        onda.activa = true;
        onda.posicion = posicion;
        onda.radioMaximo = radioMaximo;
        onda.vida = vida;
        onda.vidaMaxima = vida;
        onda.color = color;
        return;
    }
}


// Chispas blancas, del color de la barra y del jugador golpeado.
static void EmitirChispasGolpeBarra(
    MinijuegoBarraGiratoria& minijuego,
    Vector3 centro,
    Color colorJugador,
    Color colorBarra
)
{
    int cantidad = CantidadParticulasBarra(18);

    for (int k = 0; k < cantidad; k++)
    {
        float a = AzarBarra(minijuego, 0.0f, 6.2832f);
        float r = AzarBarra(minijuego, 2.0f, 5.5f);
        Color color = k % 3 == 0
            ? Color{ 255, 250, 230, 255 }
            : (k % 3 == 1 ? colorBarra : colorJugador);

        EmitirParticulaBarra(
            minijuego,
            centro,
            { std::cos(a) * r, AzarBarra(minijuego, 1.5f, 4.8f), std::sin(a) * r },
            AzarBarra(minijuego, 0.3f, 0.6f),
            AzarBarra(minijuego, 0.06f, 0.12f),
            9.0f,
            2.0f,
            color
        );
    }
}


// Polvillo de piedra al aterrizar (y al asentarse la segunda barra).
static void EmitirPolvoBarra(
    MinijuegoBarraGiratoria& minijuego,
    Vector3 pies,
    int base
)
{
    int cantidad = CantidadParticulasBarra(base);

    for (int k = 0; k < cantidad; k++)
    {
        float a = AzarBarra(minijuego, 0.0f, 6.2832f);
        float r = AzarBarra(minijuego, 0.8f, 2.0f);

        EmitirParticulaBarra(
            minijuego,
            { pies.x + std::cos(a) * 0.25f, 0.06f, pies.z + std::sin(a) * 0.25f },
            { std::cos(a) * r, AzarBarra(minijuego, 0.3f, 1.1f), std::sin(a) * r },
            AzarBarra(minijuego, 0.35f, 0.65f),
            AzarBarra(minijuego, 0.08f, 0.15f),
            1.5f,
            3.5f,
            Color{ 236, 220, 200, 255 }
        );
    }
}


// Bocanada de nube cuando un eliminado atraviesa el mar de nubes.
static void EmitirNubeBarra(
    MinijuegoBarraGiratoria& minijuego,
    Vector3 centro
)
{
    int cantidad = CantidadParticulasBarra(16);

    for (int k = 0; k < cantidad; k++)
    {
        float a = AzarBarra(minijuego, 0.0f, 6.2832f);
        float r = AzarBarra(minijuego, 1.0f, 3.2f);
        Color color = k % 2 == 0 ? Color{ 255, 240, 236, 255 } : Color{ 222, 200, 230, 255 };

        EmitirParticulaBarra(
            minijuego,
            { centro.x + std::cos(a) * 0.5f, NIVEL_NUBES_BARRA + 0.6f, centro.z + std::sin(a) * 0.5f },
            { std::cos(a) * r, AzarBarra(minijuego, 1.5f, 3.6f), std::sin(a) * r },
            AzarBarra(minijuego, 0.8f, 1.3f),
            AzarBarra(minijuego, 0.25f, 0.5f),
            2.2f,
            1.6f,
            color
        );
    }
}


static void EmitirConfetiBarra(
    MinijuegoBarraGiratoria& minijuego,
    Vector3 centro
)
{
    const Color colores[6] =
    {
        { 255, 214, 84, 255 }, { 96, 236, 255, 255 }, { 255, 112, 214, 255 },
        { 255, 160, 96, 255 }, { 150, 220, 130, 255 }, { 250, 246, 236, 255 }
    };

    int cantidad = CantidadParticulasBarra(100);

    for (int k = 0; k < cantidad; k++)
    {
        EmitirParticulaBarra(
            minijuego,
            {
                centro.x + AzarBarra(minijuego, -2.6f, 2.6f),
                AzarBarra(minijuego, 3.6f, 5.8f),
                centro.z + AzarBarra(minijuego, -2.0f, 2.0f)
            },
            {
                AzarBarra(minijuego, -0.8f, 0.8f),
                AzarBarra(minijuego, -0.4f, 1.2f),
                AzarBarra(minijuego, -0.6f, 0.6f)
            },
            AzarBarra(minijuego, 2.4f, 3.8f),
            AzarBarra(minijuego, 0.07f, 0.12f),
            1.6f,
            1.2f,
            colores[k % 6]
        );
    }
}


// Golpe de una barra: destello, chispas y sonido.
static void EfectoGolpeBarra(
    MinijuegoBarraGiratoria& minijuego,
    int indice,
    const JugadorPrueba& jugador,
    float altura,
    bool superior,
    Color colorJugador
)
{
    minijuego.estadosJugadores[indice].destelloGolpe = 1.0f;

    EmitirChispasGolpeBarra(
        minijuego,
        { jugador.posicion.x, altura, jugador.posicion.z },
        colorJugador,
        superior ? COLOR_ACENTO_BARRA_SUPERIOR : COLOR_ACENTO_BARRA_BASE
    );

    ReproducirSonidoMinijuego(minijuego.audio, superior ? SONIDO_IMPACTO : SONIDO_GOLPE);
}


// La segunda barra se asienta: onda de polvo en la losa y golpe seco.
static void EfectoAterrizajeSegundaBarra(MinijuegoBarraGiratoria& minijuego)
{
    minijuego.tiempoAterrizajeSegunda = 0.0f;

    AgregarOndaBarra(minijuego, { 0.0f, 0.03f, 0.0f }, 5.6f, 0.8f, Color{ 255, 190, 240, 255 });

    for (int k = 0; k < 6; k++)
    {
        float a = (float)k * 1.0472f;
        EmitirPolvoBarra(minijuego, { std::cos(a) * 0.9f, 0.0f, std::sin(a) * 0.9f }, 4);
    }

    ReproducirSonidoMinijuego(minijuego.audio, SONIDO_GROUND_POUND);
}


// Eliminado: sigue cayendo hacia el mar de nubes (solo dibujo) y aparece
// el aviso «¡J# AFUERA!» en el borde por donde salio.
static void EfectoEliminacionBarra(
    MinijuegoBarraGiratoria& minijuego,
    int indice,
    const JugadorPrueba& jugador
)
{
    EstadoJugadorBarraGiratoria& estado = minijuego.estadosJugadores[indice];

    float distancia = MagnitudHorizontalBarra(jugador.posicion.x, jugador.posicion.z);
    float nx = distancia > 0.05f ? jugador.posicion.x / distancia : 0.0f;
    float nz = distancia > 0.05f ? jugador.posicion.z / distancia : 1.0f;

    estado.tiempoCaida = 0.0f;
    estado.posicionCaida = jugador.posicion;
    estado.velocidadCaida =
    {
        nx * 2.4f,
        jugador.velocidad.y < -2.0f ? jugador.velocidad.y : -2.0f,
        nz * 2.4f
    };
    estado.atravesoNubes = false;

    for (int i = 0; i < MAX_AVISOS_BARRA; i++)
    {
        AvisoBarraGiratoria& aviso = minijuego.avisos[i];

        if (aviso.activo)
        {
            continue;
        }

        aviso.activo = true;
        aviso.jugador = indice;
        aviso.posicion = { nx * RADIO_ARENA_BARRA, 0.6f, nz * RADIO_ARENA_BARRA };
        aviso.vida = DURACION_AVISO_BARRA;
        break;
    }

    ReproducirSonidoMinijuego(minijuego.audio, SONIDO_CAIDA);
}


static void ActualizarVisualBarra(
    MinijuegoBarraGiratoria& minijuego,
    float deltaTime,
    const JugadorPrueba jugadores[]
)
{
    minijuego.tiempoAnimacion += deltaTime;

    if (minijuego.fase == FASE_BARRA_JUGANDO)
    {
        minijuego.giroMecanismo += minijuego.velocidadAngular * deltaTime;
    }

    if (minijuego.tiempoAterrizajeSegunda >= 0.0f)
    {
        minijuego.tiempoAterrizajeSegunda += deltaTime;
    }

    for (int i = 0; i < MAX_PARTICULAS_BARRA; i++)
    {
        ParticulaBarraGiratoria& p = minijuego.particulasEfecto[i];

        if (!p.activa)
        {
            continue;
        }

        p.vida -= deltaTime;

        if (p.vida <= 0.0f || p.posicion.y < NIVEL_NUBES_BARRA - 1.0f)
        {
            p.activa = false;
            continue;
        }

        p.velocidad.y -= p.gravedad * deltaTime;
        float freno = std::exp(-p.freno * deltaTime);
        p.velocidad.x *= freno;
        p.velocidad.z *= freno;
        p.posicion.x += p.velocidad.x * deltaTime;
        p.posicion.y += p.velocidad.y * deltaTime;
        p.posicion.z += p.velocidad.z * deltaTime;
    }

    for (int i = 0; i < MAX_ONDAS_BARRA; i++)
    {
        OndaBarraGiratoria& onda = minijuego.ondas[i];

        if (!onda.activa)
        {
            continue;
        }

        onda.vida -= deltaTime;
        if (onda.vida <= 0.0f) onda.activa = false;
    }

    for (int i = 0; i < MAX_AVISOS_BARRA; i++)
    {
        AvisoBarraGiratoria& aviso = minijuego.avisos[i];

        if (!aviso.activo)
        {
            continue;
        }

        aviso.vida -= deltaTime;
        if (aviso.vida <= 0.0f) aviso.activo = false;
    }

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        EstadoJugadorBarraGiratoria& estado = minijuego.estadosJugadores[i];

        estado.destelloGolpe -= deltaTime * 2.2f;
        if (estado.destelloGolpe < 0.0f) estado.destelloGolpe = 0.0f;

        if (estado.tiempoCaida < 0.0f)
        {
            continue;
        }

        estado.tiempoCaida += deltaTime;
        estado.velocidadCaida.y -= 16.0f * deltaTime;
        estado.posicionCaida.x += estado.velocidadCaida.x * deltaTime;
        estado.posicionCaida.y += estado.velocidadCaida.y * deltaTime;
        estado.posicionCaida.z += estado.velocidadCaida.z * deltaTime;

        if (!estado.atravesoNubes && estado.posicionCaida.y < NIVEL_NUBES_BARRA + 0.4f)
        {
            estado.atravesoNubes = true;
            EmitirNubeBarra(minijuego, estado.posicionCaida);
            AgregarOndaBarra(
                minijuego,
                { estado.posicionCaida.x, NIVEL_NUBES_BARRA + 0.5f, estado.posicionCaida.z },
                2.6f,
                0.9f,
                Color{ 255, 244, 240, 255 }
            );
        }
    }

    // Confeti sobre el ultimo que quedo en la plataforma.
    if (minijuego.fase == FASE_BARRA_TERMINADO && !minijuego.confetiLanzado)
    {
        minijuego.confetiLanzado = true;

        for (int i = 0; i < MAX_PARTICIPANTES; i++)
        {
            if (
                minijuego.resultado.participantes[i].participo &&
                !minijuego.estadosJugadores[i].eliminado
            )
            {
                EmitirConfetiBarra(minijuego, jugadores[i].posicion);
            }
        }
    }
}


void MinijuegoBarraGiratoria::Inicializar()
{
    resultado = {};
    resultado.formato = FORMATO_MINIJUEGO_INDIVIDUAL;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        estadosJugadores[i] = {};
    }

    suelo = {};
    suelo.posicion = { 0.0f, -0.35f, 0.0f };
    suelo.posicionInicial = suelo.posicion;
    suelo.tamano = { 11.6f, 0.70f, 11.6f };
    suelo.color = Color{ 93, 97, 106, 255 };
    suelo.activaColision = true;

    fase = FASE_BARRA_PREPARACION;
    tiempoPreparacion = DURACION_PREPARACION_BARRA;
    tiempoJugado = 0.0f;

    anguloBarra = 0.0f;
    velocidadAngular = 0.9f;

    segundaBarraAparecio = false;
    segundaBarraLista = false;
    alturaSegundaBarra = ALTURA_INICIAL_SEGUNDA_BARRA;
    anguloSegundaBarra = PI / 2.0f;
    velocidadAngularSegunda =
        velocidadAngular * MULTIPLICADOR_VELOCIDAD_SEGUNDA_BARRA;

    // Solo visual.
    tiempoAnimacion = 0.0f;
    giroMecanismo = 0.0f;
    tiempoAterrizajeSegunda = -1.0f;
    semillaVisual = 2463534242u;
    confetiLanzado = false;

    for (int i = 0; i < MAX_PARTICULAS_BARRA; i++) particulasEfecto[i] = {};
    for (int i = 0; i < MAX_ONDAS_BARRA; i++) ondas[i] = {};
    for (int i = 0; i < MAX_AVISOS_BARRA; i++) avisos[i] = {};

    // Camara del templo (solo dibujo): baja la vista lo justo para que
    // detras de la plataforma se vean las islas y el amanecer.
    camara.position = { 0.0f, 6.5f, 11.9f };
    camara.target = { 0.0f, 0.9f, 0.0f };
    camara.up = { 0.0f, 1.0f, 0.0f };
    camara.fovy = 46.0f;
    camara.projection = CAMERA_PERSPECTIVE;
}


void MinijuegoBarraGiratoria::ConfigurarJugadores(
    JugadorPrueba jugadores[],
    int cantidadMaxima
) const
{
    Vector3 spawns[MAX_JUGADORES_PRUEBA] =
    {
        { -2.4f, 1.05f,  2.4f },
        {  2.4f, 1.05f,  2.4f },
        { -2.4f, 1.05f, -2.4f },
        {  2.4f, 1.05f, -2.4f }
    };

    int limite =
        cantidadMaxima < MAX_JUGADORES_PRUEBA
            ? cantidadMaxima
            : MAX_JUGADORES_PRUEBA;

    for (int i = 0; i < limite; i++)
    {
        ConfigurarJugadorMinijuegoEstandar(jugadores[i], spawns[i]);
    }
}


void MinijuegoBarraGiratoria::Reiniciar(
    JugadorPrueba jugadores[],
    int cantidadMaxima
)
{
    bool participaban[MAX_PARTICIPANTES]{};

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        participaban[i] = resultado.participantes[i].participo;
    }

    Inicializar();

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        resultado.participantes[i].participo = participaban[i];

        if (participaban[i])
        {
            resultado.cantidadParticipantes++;
        }
    }

    ConfigurarJugadores(jugadores, cantidadMaxima);
}


void MinijuegoBarraGiratoria::Actualizar(
    float deltaTime,
    JugadorPrueba jugadores[],
    int cantidadMaxima,
    Participante participantes[],
    ParticulaTierra particulas[],
    int cantidadParticulas
)
{
    if (resultado.cantidadParticipantes == 0)
    {
        InicializarResultadoMinijuego(
            resultado,
            participantes,
            FORMATO_MINIJUEGO_INDIVIDUAL
        );
    }

    // Animaciones y particulas siguen tambien con el minijuego terminado.
    ActualizarVisualBarra(*this, deltaTime, jugadores);

    if (fase == FASE_BARRA_TERMINADO)
    {
        return;
    }

    if (fase == FASE_BARRA_PREPARACION)
    {
        for (int i = 0; i < MAX_PARTICIPANTES; i++)
        {
            jugadores[i].velocidad = {};
            jugadores[i].empuje = {};
        }

        float preparacionAntes = tiempoPreparacion;
        tiempoPreparacion -= deltaTime;
        ActualizarAudioCuentaRegresiva(audio, preparacionAntes, tiempoPreparacion);

        if (tiempoPreparacion <= 0.0f)
        {
            tiempoPreparacion = 0.0f;
            fase = FASE_BARRA_JUGANDO;
        }

        return;
    }

    tiempoJugado += deltaTime;

    // La barra base acelera sin depender de un temporizador de fin. Al cabo
    // de suficiente tiempo supera claramente la velocidad lineal estandar de
    // los jugadores incluso cerca del centro de la arena.
    float aumentoVelocidad = tiempoJugado * 0.12f;
    if (aumentoVelocidad > 5.20f)
    {
        aumentoVelocidad = 5.20f;
    }

    velocidadAngular = 0.9f + aumentoVelocidad;
    anguloBarra += velocidadAngular * deltaTime;

    while (anguloBarra > 2.0f * PI)
    {
        anguloBarra -= 2.0f * PI;
    }

    if (
        !segundaBarraAparecio &&
        tiempoJugado >= TIEMPO_APARICION_SEGUNDA_BARRA
    )
    {
        segundaBarraAparecio = true;
        segundaBarraLista = false;
        alturaSegundaBarra = ALTURA_INICIAL_SEGUNDA_BARRA;
        anguloSegundaBarra = anguloBarra + PI / 2.0f;
    }

    if (segundaBarraAparecio)
    {
        velocidadAngularSegunda =
            velocidadAngular * MULTIPLICADOR_VELOCIDAD_SEGUNDA_BARRA;

        anguloSegundaBarra +=
            velocidadAngularSegunda * deltaTime;

        while (anguloSegundaBarra > 2.0f * PI)
        {
            anguloSegundaBarra -= 2.0f * PI;
        }

        if (!segundaBarraLista)
        {
            alturaSegundaBarra -=
                VELOCIDAD_CAIDA_SEGUNDA_BARRA * deltaTime;

            if (alturaSegundaBarra <= ALTURA_FINAL_SEGUNDA_BARRA)
            {
                alturaSegundaBarra = ALTURA_FINAL_SEGUNDA_BARRA;
                segundaBarraLista = true;
                ActivarTemblorCamaraGeneral(0.12f, 0.20f);
                EfectoAterrizajeSegundaBarra(*this);
            }
        }
    }

    int vivosAntes = ContarVivosBarra(*this);

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        JugadorPrueba& jugador = jugadores[i];
        EstadoJugadorBarraGiratoria& estadoJugador =
            estadosJugadores[i];

        if (
            !resultado.participantes[i].participo ||
            estadoJugador.eliminado
        )
        {
            continue;
        }

        if (estadoJugador.cooldownImpacto > 0.0f)
        {
            estadoJugador.cooldownImpacto -= deltaTime;
            if (estadoJugador.cooldownImpacto < 0.0f)
            {
                estadoJugador.cooldownImpacto = 0.0f;
            }
        }

        if (estadoJugador.tiempoStunBarra > 0.0f)
        {
            estadoJugador.tiempoStunBarra -= deltaTime;
            if (estadoJugador.tiempoStunBarra < 0.0f)
            {
                estadoJugador.tiempoStunBarra = 0.0f;
            }
        }

        InputMinijuegoParticipante entrada{};

        if (estadoJugador.tiempoStunBarra <= 0.0f)
        {
            if (participantes[i].esBot || !participantes[i].conectado)
            {
                entrada = CrearEntradaBotBarra(
                    *this,
                    i,
                    jugador,
                    deltaTime
                );
            }
            else
            {
                entrada = LeerInputMinijuegoParticipante(participantes[i]);
            }
        }

        BloquePrueba sueloJugador = suelo;
        sueloJugador.activaColision = JugadorSobreArenaBarra(jugador);

        ActualizarJugadorPruebaNormal(
            jugador,
            entrada,
            &sueloJugador,
            1,
            particulas,
            cantidadParticulas,
            true,
            false,
            deltaTime
        );

        // Solo visual / audio: polvo al aterrizar y sonido de salto.
        if (jugador.enSuelo && !estadoJugador.enSueloAnterior)
        {
            EmitirPolvoBarra(*this, jugador.posicion, 5);
        }
        else if (
            !jugador.enSuelo &&
            estadoJugador.enSueloAnterior &&
            jugador.velocidad.y > 0.5f &&
            estadoJugador.tiempoStunBarra <= 0.0f &&
            !participantes[i].esBot
        )
        {
            ReproducirSonidoMinijuego(audio, SONIDO_SALTO);
        }

        estadoJugador.enSueloAnterior = jugador.enSuelo;

        if (
            estadoJugador.cooldownImpacto <= 0.0f &&
            !jugador.cayendo &&
            BarraTocaJugador(jugador, anguloBarra, ALTURA_BARRA)
        )
        {
            AplicarImpactoBarra(
                jugador,
                estadoJugador,
                ALTURA_BARRA,
                FUERZA_IMPACTO_BARRA,
                IMPULSO_VERTICAL_BARRA,
                particulas,
                cantidadParticulas
            );

            EfectoGolpeBarra(*this, i, jugador, ALTURA_BARRA, false, participantes[i].color);
        }
        else if (
            segundaBarraLista &&
            estadoJugador.cooldownImpacto <= 0.0f &&
            !jugador.cayendo &&
            BarraTocaJugador(
                jugador,
                anguloSegundaBarra,
                ALTURA_FINAL_SEGUNDA_BARRA
            )
        )
        {
            AplicarImpactoBarra(
                jugador,
                estadoJugador,
                ALTURA_FINAL_SEGUNDA_BARRA,
                FUERZA_IMPACTO_BARRA_SUPERIOR,
                IMPULSO_VERTICAL_BARRA_SUPERIOR,
                particulas,
                cantidadParticulas
            );

            EfectoGolpeBarra(*this, i, jugador, ALTURA_FINAL_SEGUNDA_BARRA, true, participantes[i].color);
        }

        if (
            !JugadorSobreArenaBarra(jugador) &&
            jugador.posicion.y < -2.0f
        )
        {
            jugador.cayendo = true;
        }
    }

    ResolverInteraccionesJugadoresMinijuegoEstandar(
        jugadores,
        participantes,
        cantidadMaxima,
        particulas,
        cantidadParticulas
    );

    int eliminados = 0;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (
            resultado.participantes[i].participo &&
            !estadosJugadores[i].eliminado &&
            jugadores[i].cayendo
        )
        {
            eliminados++;
        }
    }

    int posicion = vivosAntes - eliminados + 1;

    int tiempoMs =
        (int)std::lround(
            tiempoJugado * 1000.0f
        );

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (
            resultado.participantes[i].participo &&
            !estadosJugadores[i].eliminado &&
            jugadores[i].cayendo
        )
        {
            estadosJugadores[i].eliminado = true;
            estadosJugadores[i].posicionFinal = posicion;
            estadosJugadores[i].tiempoSobrevividoMs = tiempoMs;

            EfectoEliminacionBarra(*this, i, jugadores[i]);
        }
    }

    int vivosDespues = vivosAntes - eliminados;

    // Tope duro de seguridad: la ronda nunca queda abierta indefinidamente.
    if (vivosDespues <= 1 || tiempoJugado > 150.0f)
    {
        FinalizarBarra(*this);
    }
}


//==================================================
// PALETA Y PRIMITIVAS DE DIBUJO
//==================================================
//
// Templo celeste al amanecer: piedra caliza tallada, bronce, telas
// ciruela y crema, nubes rosadas y bruma melocoton. El acento (celeste
// en la barra base, magenta en la superior) se reserva para las barras.

static const Color COLOR_NIEBLA_BARRA = { 250, 208, 190, 255 };
static const Color COLOR_PIEDRA_BARRA = { 200, 182, 160, 255 };
static const Color COLOR_PIEDRA_CLARA_BARRA = { 230, 216, 194, 255 };
static const Color COLOR_JUNTA_BARRA = { 118, 100, 96, 255 };
static const Color COLOR_BRONCE_BARRA = { 190, 130, 64, 255 };
static const Color COLOR_BRONCE_CLARO_BARRA = { 238, 188, 108, 255 };
static const Color COLOR_BRONCE_OSCURO_BARRA = { 112, 72, 42, 255 };
static const Color COLOR_ORO_BARRA = { 248, 210, 116, 255 };
static const Color COLOR_TELA_BARRA = { 128, 50, 98, 255 };
static const Color COLOR_TELA_CREMA_BARRA = { 244, 232, 210, 255 };
static const Color COLOR_CESPED_BARRA = { 138, 170, 110, 255 };
static const Color COLOR_ROCA_BARRA = { 164, 136, 128, 255 };
static const Color COLOR_NUBE_LUZ_BARRA = { 255, 236, 228, 255 };
static const Color COLOR_NUBE_SOMBRA_BARRA = { 200, 172, 208, 255 };

// Ojo de referencia para la niebla (la camara de juego).
static const Vector3 OJO_NIEBLA_BARRA = { 0.0f, 6.5f, 11.9f };


static unsigned char CanalBarra(float v)
{
    if (v < 0.0f) return 0;
    if (v > 255.0f) return 255;
    return (unsigned char)v;
}


static Color TonoBarra(Color color, float factor)
{
    return Color{
        CanalBarra((float)color.r * factor),
        CanalBarra((float)color.g * factor),
        CanalBarra((float)color.b * factor),
        color.a
    };
}


// Caras de arriba con luz calida de amanecer; las laterales, algo frias.
static Color SombraBarra(Color color, float factor, bool arriba)
{
    if (arriba)
    {
        return Color{
            CanalBarra((float)color.r * factor * 1.05f),
            CanalBarra((float)color.g * factor * 1.0f),
            CanalBarra((float)color.b * factor * 0.93f),
            color.a
        };
    }

    return Color{
        CanalBarra((float)color.r * factor * 0.96f),
        CanalBarra((float)color.g * factor * 0.97f),
        CanalBarra((float)color.b * factor * 1.05f),
        color.a
    };
}


static Color MezclaBarra(Color a, Color b, float t)
{
    t = Limitar01Barra(t);

    return Color{
        CanalBarra((float)a.r + ((float)b.r - (float)a.r) * t),
        CanalBarra((float)a.g + ((float)b.g - (float)a.g) * t),
        CanalBarra((float)a.b + ((float)b.b - (float)a.b) * t),
        CanalBarra((float)a.a + ((float)b.a - (float)a.a) * t)
    };
}


static Color NieblaBarra(Color color, float cantidad)
{
    Color niebla = COLOR_NIEBLA_BARRA;
    niebla.a = color.a;
    return MezclaBarra(color, niebla, cantidad);
}


// Niebla de color por distancia: lo lejano se funde con la bruma rosada.
static float CantidadNieblaBarra(Vector3 p)
{
    float dx = p.x - OJO_NIEBLA_BARRA.x;
    float dy = p.y - OJO_NIEBLA_BARRA.y;
    float dz = p.z - OJO_NIEBLA_BARRA.z;
    float d = std::sqrt(dx * dx + dy * dy + dz * dz);
    float n = (d - 18.0f) / 80.0f;
    return n < 0.0f ? 0.0f : (n > 0.86f ? 0.86f : n);
}


static Color TinteBarra(Color color, Vector3 p)
{
    return NieblaBarra(color, CantidadNieblaBarra(p));
}


static void VerticeBarra(Vector3 p, Color color)
{
    rlColor4ub(color.r, color.g, color.b, color.a);
    rlVertex3f(p.x, p.y, p.z);
}


static void QuadBarra(Vector3 a, Vector3 b, Vector3 c, Vector3 d, Color color)
{
    VerticeBarra(a, color);
    VerticeBarra(b, color);
    VerticeBarra(c, color);
    VerticeBarra(d, color);
}


// Caja con caras sombreadas; respeta la matriz actual (piezas que giran).
static void CuboBarra(Vector3 c, Vector3 t, Color color)
{
    float x0 = c.x - t.x * 0.5f, x1 = c.x + t.x * 0.5f;
    float y0 = c.y - t.y * 0.5f, y1 = c.y + t.y * 0.5f;
    float z0 = c.z - t.z * 0.5f, z1 = c.z + t.z * 0.5f;

    rlBegin(RL_QUADS);
    QuadBarra({ x0, y1, z1 }, { x1, y1, z1 }, { x1, y1, z0 }, { x0, y1, z0 }, SombraBarra(color, 1.0f, true));
    QuadBarra({ x0, y0, z1 }, { x1, y0, z1 }, { x1, y1, z1 }, { x0, y1, z1 }, SombraBarra(color, 0.78f, false));
    QuadBarra({ x1, y0, z0 }, { x0, y0, z0 }, { x0, y1, z0 }, { x1, y1, z0 }, SombraBarra(color, 0.56f, false));
    QuadBarra({ x1, y0, z1 }, { x1, y0, z0 }, { x1, y1, z0 }, { x1, y1, z1 }, SombraBarra(color, 0.62f, false));
    QuadBarra({ x0, y0, z0 }, { x0, y0, z1 }, { x0, y1, z1 }, { x0, y1, z0 }, SombraBarra(color, 0.70f, false));
    QuadBarra({ x0, y0, z0 }, { x1, y0, z0 }, { x1, y0, z1 }, { x0, y0, z1 }, SombraBarra(color, 0.45f, false));
    rlEnd();
}


// Luz falsa de las caras laterales segun su orientacion: el frente (+Z)
// mas claro, la derecha (+X) y el fondo mas oscuros.
static float LuzLateralBarra(float angulo)
{
    return 0.68f + 0.14f * std::sin(angulo) - 0.07f * std::cos(angulo);
}


// Prisma o tronco de cono: de base.y (radio r0) a base.y + alto (radio
// r1). "estriado" oscurece una cara de cada dos (columnas acanaladas).
static void PrismaBarra(
    Vector3 base,
    float r0,
    float r1,
    float alto,
    int lados,
    Color color,
    float giro = 0.0f,
    bool estriado = false
)
{
    float yArriba = base.y + alto;
    float paso = 6.2832f / (float)lados;

    rlBegin(RL_QUADS);

    for (int k = 0; k < lados; k++)
    {
        float a0 = giro + paso * (float)k;
        float a1 = a0 + paso;
        float f = LuzLateralBarra(a0 + paso * 0.5f);
        if (estriado && (k & 1)) f *= 0.9f;
        Color cara = SombraBarra(color, f, false);

        float c0 = std::cos(a0), s0 = std::sin(a0);
        float c1 = std::cos(a1), s1 = std::sin(a1);

        QuadBarra(
            { base.x + c0 * r0, base.y, base.z + s0 * r0 },
            { base.x + c1 * r0, base.y, base.z + s1 * r0 },
            { base.x + c1 * r1, yArriba, base.z + s1 * r1 },
            { base.x + c0 * r1, yArriba, base.z + s0 * r1 },
            cara
        );

        // Tapas (triangulos como quads degenerados).
        Color tapaArriba = alto > 0.0f ? SombraBarra(color, 1.0f, true) : SombraBarra(color, 0.45f, false);
        Color tapaAbajo = alto > 0.0f ? SombraBarra(color, 0.45f, false) : SombraBarra(color, 1.0f, true);

        if (r1 > 0.001f)
        {
            Vector3 centro = { base.x, yArriba, base.z };
            QuadBarra(
                centro,
                { base.x + c1 * r1, yArriba, base.z + s1 * r1 },
                { base.x + c0 * r1, yArriba, base.z + s0 * r1 },
                centro,
                tapaArriba
            );
        }

        if (r0 > 0.001f)
        {
            Vector3 centro = { base.x, base.y, base.z };
            QuadBarra(
                centro,
                { base.x + c0 * r0, base.y, base.z + s0 * r0 },
                { base.x + c1 * r0, base.y, base.z + s1 * r0 },
                centro,
                tapaAbajo
            );
        }
    }

    rlEnd();
}


// Corona plana en el suelo (r0..r1) a la altura y.
static void AnilloBarra(Vector3 c, float r0, float r1, Color color, int lados = 40)
{
    rlBegin(RL_QUADS);

    for (int k = 0; k < lados; k++)
    {
        float a0 = 6.2832f * (float)k / (float)lados;
        float a1 = 6.2832f * (float)(k + 1) / (float)lados;

        QuadBarra(
            { c.x + std::cos(a0) * r1, c.y, c.z + std::sin(a0) * r1 },
            { c.x + std::cos(a0) * r0, c.y, c.z + std::sin(a0) * r0 },
            { c.x + std::cos(a1) * r0, c.y, c.z + std::sin(a1) * r0 },
            { c.x + std::cos(a1) * r1, c.y, c.z + std::sin(a1) * r1 },
            color
        );
    }

    rlEnd();
}


// Disco de luz con borde que se apaga (u y v definen el plano).
static void ResplandorBarra(Vector3 centro, Vector3 u, Vector3 v, float radio, Color color)
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


static void ResplandorSueloBarra(Vector3 c, float radio, Color color)
{
    ResplandorBarra(c, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, radio, color);
}


// Halo que mira a la camara (plano inclinado como la vista).
static void ResplandorFrenteBarra(Vector3 c, float radio, Color color)
{
    ResplandorBarra(c, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.906f, 0.423f }, radio, color);
}


// Engranaje en el plano XZ local, centrado en el origen (la matriz
// actual lo ubica y orienta). MODELO FUTURO: engranaje de bronce GLB.
static void EngranajeBarra(float radio, float grosor, int dientes, Color color)
{
    PrismaBarra({ 0.0f, -grosor * 0.5f, 0.0f }, radio * 0.84f, radio * 0.84f, grosor, dientes * 2, color);

    for (int k = 0; k < dientes; k++)
    {
        float a = 6.2832f * (float)k / (float)dientes;
        rlPushMatrix();
        rlRotatef(-a * RAD2DEG, 0.0f, 1.0f, 0.0f);
        CuboBarra(
            { radio * 0.92f, 0.0f, 0.0f },
            { radio * 0.2f, grosor * 0.92f, 6.2832f * radio / (float)dientes * 0.45f },
            color
        );
        rlPopMatrix();
    }

    // Rebaje interior, rayos y cubo central.
    Color rebaje = TonoBarra(color, 0.72f);
    AnilloBarra({ 0.0f, grosor * 0.5f + 0.004f, 0.0f }, radio * 0.34f, radio * 0.66f, rebaje, 20);

    for (int k = 0; k < 4; k++)
    {
        rlPushMatrix();
        rlRotatef(45.0f * (float)k, 0.0f, 1.0f, 0.0f);
        CuboBarra({ 0.0f, grosor * 0.5f + 0.01f, 0.0f }, { radio * 1.3f, 0.02f, radio * 0.12f }, color);
        rlPopMatrix();
    }

    PrismaBarra({ 0.0f, -grosor * 0.7f, 0.0f }, radio * 0.26f, radio * 0.26f, grosor * 1.4f, 10, TonoBarra(color, 0.85f));
}


//==================================================
// CIELO Y MAR DE NUBES
//==================================================

// Degradado de amanecer (2D, antes del 3D) alineado con el horizonte de
// nubes que proyecta la camara: anil arriba, malva, salmon y oro.
static void DibujarCieloBarra(const Camera3D& vista, float t)
{
    int ancho = GetScreenWidth();
    int alto = GetScreenHeight();

    float horizonte = GetWorldToScreen({ 0.0f, NIVEL_NUBES_BARRA + 2.0f, -62.0f }, vista).y;
    if (horizonte < (float)alto * 0.12f) horizonte = (float)alto * 0.12f;
    if (horizonte > (float)alto * 0.45f) horizonte = (float)alto * 0.45f;

    int h0 = (int)(horizonte * 0.30f);
    int h1 = (int)(horizonte * 0.68f);
    int h2 = (int)horizonte;

    DrawRectangleGradientV(0, 0, ancho, h0, Color{ 54, 58, 122, 255 }, Color{ 122, 96, 162, 255 });
    DrawRectangleGradientV(0, h0, ancho, h1 - h0, Color{ 122, 96, 162, 255 }, Color{ 238, 150, 154, 255 });
    DrawRectangleGradientV(0, h1, ancho, h2 - h1, Color{ 238, 150, 154, 255 }, Color{ 255, 210, 170, 255 });
    DrawRectangleGradientV(0, h2, ancho, alto - h2, Color{ 255, 210, 170, 255 }, COLOR_NIEBLA_BARRA);

    // Ultimas estrellas que se apagan.
    for (int k = 0; k < 28; k++)
    {
        float x = Hash01Barra(k, 1) * (float)ancho;
        float y = Hash01Barra(k, 2) * (float)h1 * 0.8f;
        float brillo = 0.25f + 0.3f * (0.5f + 0.5f * std::sin(t * (1.0f + Hash01Barra(k, 3)) + (float)k));
        brillo *= 1.0f - y / ((float)h1 + 1.0f);
        DrawCircleV({ x, y }, 1.2f + Hash01Barra(k, 4), Fade(Color{ 255, 248, 236, 255 }, brillo));
    }

    // Sol naciente, apoyado sobre el banco de nubes a la derecha.
    Vector2 sol = { (float)ancho * 0.85f, horizonte - (float)alto * 0.13f };
    float pulso = 1.0f + 0.03f * std::sin(t * 0.7f);
    DrawCircleGradient(sol, (float)alto * 0.42f * pulso, Fade(Color{ 255, 200, 150, 255 }, 0.45f), Fade(Color{ 255, 200, 150, 255 }, 0.0f));
    DrawCircleGradient(sol, (float)alto * 0.13f, Fade(Color{ 255, 236, 196, 255 }, 0.8f), Fade(Color{ 255, 220, 170, 255 }, 0.0f));
    DrawCircleV(sol, (float)alto * 0.05f, Color{ 255, 246, 220, 255 });

    // Rayos que giran muy lento.
    if (CalidadEfectos() != CALIDAD_BAJA)
    {
        for (int k = 0; k < 9; k++)
        {
            float a = t * 0.02f + (float)k * 0.698f;
            float largo = (float)alto * (0.55f + 0.25f * Hash01Barra(k, 9));
            Vector2 p1 = { sol.x + std::cos(a - 0.05f) * largo, sol.y + std::sin(a - 0.05f) * largo };
            Vector2 p2 = { sol.x + std::cos(a + 0.05f) * largo, sol.y + std::sin(a + 0.05f) * largo };
            DrawTriangle(sol, p2, p1, Fade(Color{ 255, 226, 180, 255 }, 0.07f));
        }
    }

    // Cirros rosados que cruzan muy alto.
    for (int k = 0; k < 6; k++)
    {
        float vel = 6.0f + 4.0f * Hash01Barra(k, 20);
        float x = std::fmod(Hash01Barra(k, 21) * (float)(ancho + 600) + t * vel, (float)(ancho + 600)) - 300.0f;
        float y = (float)h0 + Hash01Barra(k, 22) * (float)(h2 - h0) * 0.8f;
        float largo = 140.0f + 160.0f * Hash01Barra(k, 23);
        Color color = MezclaBarra(Color{ 255, 190, 190, 255 }, Color{ 255, 226, 200, 255 }, y / (float)(h2 + 1));
        DrawEllipse((int)x, (int)y, largo, 5.0f + 4.0f * Hash01Barra(k, 24), Fade(color, 0.5f));
        DrawEllipse((int)(x + largo * 0.3f), (int)(y - 6.0f), largo * 0.5f, 3.0f, Fade(color, 0.35f));
    }
}


// Bocanada de nube: sombra lila abajo y copete iluminado arriba.
static void BocanadaNubeBarra(Vector3 c, float radio, float aplastado, float niebla)
{
    Color sombra = NieblaBarra(COLOR_NUBE_SOMBRA_BARRA, niebla);
    Color luz = NieblaBarra(COLOR_NUBE_LUZ_BARRA, niebla);

    rlPushMatrix();
    rlTranslatef(c.x, c.y, c.z);
    rlScalef(1.0f, aplastado, 0.8f);
    DrawSphereEx({ 0.0f, 0.0f, 0.0f }, radio, 5, 9, sombra);
    DrawSphereEx({ radio * 0.18f, radio * 0.32f, radio * 0.1f }, radio * 0.82f, 5, 9, luz);
    rlPopMatrix();
}


// Montana con cara iluminada rosada (alpenglow) y nieve en la cumbre.
static void PicoBarra(Vector3 base, float alto, float ancho, float niebla, int semilla)
{
    float desvio = (Hash01Barra(semilla, 1) - 0.5f) * ancho * 0.5f;
    Vector3 p = { base.x + desvio, base.y + alto, base.z };
    Vector3 l = { base.x - ancho, base.y, base.z };
    Vector3 r = { base.x + ancho, base.y, base.z };
    Vector3 f = { base.x + desvio * 0.4f, base.y, base.z + ancho * 0.8f };

    Color rocaLuz = NieblaBarra(Color{ 178, 128, 150, 255 }, niebla);
    Color rocaSombra = NieblaBarra(Color{ 112, 94, 140, 255 }, niebla);
    Color nieveLuz = NieblaBarra(Color{ 255, 220, 214, 255 }, niebla);
    Color nieveSombra = NieblaBarra(Color{ 196, 176, 214, 255 }, niebla);

    rlBegin(RL_TRIANGLES);
    VerticeBarra(l, rocaLuz); VerticeBarra(f, rocaLuz); VerticeBarra(p, rocaLuz);
    VerticeBarra(f, rocaSombra); VerticeBarra(r, rocaSombra); VerticeBarra(p, rocaSombra);

    float s = 0.34f + 0.1f * Hash01Barra(semilla, 2);
    Vector3 nl = { p.x + (l.x - p.x) * s, p.y + (l.y - p.y) * s, p.z + (l.z - p.z) * s };
    Vector3 nf = { p.x + (f.x - p.x) * s * 1.3f, p.y + (f.y - p.y) * s * 1.3f, p.z + (f.z - p.z) * s * 1.3f };
    Vector3 nr = { p.x + (r.x - p.x) * s * 0.9f, p.y + (r.y - p.y) * s * 0.9f, p.z + (r.z - p.z) * s * 0.9f };
    VerticeBarra(nl, nieveLuz); VerticeBarra(nf, nieveLuz); VerticeBarra(p, nieveLuz);
    VerticeBarra(nf, nieveSombra); VerticeBarra(nr, nieveSombra); VerticeBarra(p, nieveSombra);
    rlEnd();
}


// Horizonte: dos cordilleras (la de atras mas fundida en la bruma) que
// asoman de un banco de nubes lejano.
static void DibujarHorizonteBarra(float t)
{
    for (int capa = 0; capa < 2; capa++)
    {
        float z = capa == 0 ? -110.0f : -86.0f;
        float niebla = capa == 0 ? 0.74f : 0.56f;
        int picos = capa == 0 ? 9 : 7;

        for (int k = 0; k < picos; k++)
        {
            float x = -130.0f + 260.0f * ((float)k + 0.5f) / (float)picos + (Hash01Barra(k, 40 + capa) - 0.5f) * 14.0f;
            float alto = (capa == 0 ? 6.0f : 4.0f) + (capa == 0 ? 5.0f : 4.0f) * Hash01Barra(k, 50 + capa);
            float ancho = 9.0f + 7.0f * Hash01Barra(k, 60 + capa);
            PicoBarra({ x, NIVEL_NUBES_BARRA - 3.0f, z }, alto, ancho, niebla, k * 7 + capa * 31);
        }
    }

    if (CalidadDecoracion() == CALIDAD_BAJA)
    {
        return;
    }

    // Banco de nubes del horizonte: tapa el pie de las montanas.
    int cantidad = CalidadDecoracion() == CALIDAD_ALTA ? 22 : 12;

    for (int k = 0; k < cantidad; k++)
    {
        float x = -120.0f + 240.0f * ((float)k + 0.5f) / (float)cantidad + std::sin(t * 0.05f + (float)k) * 2.0f;
        float z = -70.0f - 14.0f * Hash01Barra(k, 70);
        float radio = 7.0f + 5.0f * Hash01Barra(k, 71);
        BocanadaNubeBarra({ x, NIVEL_NUBES_BARRA + 0.5f, z }, radio, 0.42f, 0.55f);
    }

    // Torres de cumulos que suben al cielo, encendidas por el sol.
    const Vector3 torres[4] =
    {
        { -46.0f, NIVEL_NUBES_BARRA, -64.0f }, { 52.0f, NIVEL_NUBES_BARRA, -70.0f },
        { -78.0f, NIVEL_NUBES_BARRA, -84.0f }, { 22.0f, NIVEL_NUBES_BARRA, -88.0f }
    };

    for (int k = 0; k < 4; k++)
    {
        float deriva = std::sin(t * 0.04f + (float)k * 2.0f) * 1.5f;

        for (int nivel = 0; nivel < 4; nivel++)
        {
            float radio = 9.0f - 1.8f * (float)nivel + 1.5f * Hash01Barra(k, 90 + nivel);
            Vector3 c =
            {
                torres[k].x + deriva + (Hash01Barra(k, 95 + nivel) - 0.5f) * 5.0f,
                torres[k].y + 1.5f + 3.2f * (float)nivel,
                torres[k].z
            };
            BocanadaNubeBarra(c, radio, 0.55f, 0.45f - 0.04f * (float)nivel);
        }
    }
}


// Mar de nubes: base lila con bruma por distancia y bocanadas que derivan.
static void DibujarMarNubesBarra(float t)
{
    // Base: corona por corona, cada una con su niebla.
    const int ANILLOS = 6;
    const int LADOS = 36;
    const float radios[ANILLOS + 1] = { 0.0f, 10.0f, 18.0f, 28.0f, 40.0f, 54.0f, 70.0f };
    float y = NIVEL_NUBES_BARRA - 0.4f;

    rlBegin(RL_QUADS);
    for (int a = 0; a < ANILLOS; a++)
    {
        for (int k = 0; k < LADOS; k++)
        {
            float a0 = 6.2832f * (float)k / (float)LADOS;
            float a1 = 6.2832f * (float)(k + 1) / (float)LADOS;
            Vector3 p[4] =
            {
                { std::cos(a0) * radios[a + 1], y, std::sin(a0) * radios[a + 1] },
                { std::cos(a0) * radios[a], y, std::sin(a0) * radios[a] },
                { std::cos(a1) * radios[a], y, std::sin(a1) * radios[a] },
                { std::cos(a1) * radios[a + 1], y, std::sin(a1) * radios[a + 1] }
            };

            for (int v = 0; v < 4; v++)
            {
                float ondulacion = std::sin(p[v].x * 0.11f + t * 0.1f) * std::cos(p[v].z * 0.09f);
                Color base = MezclaBarra(Color{ 176, 150, 198, 255 }, COLOR_NUBE_SOMBRA_BARRA, 0.5f + 0.5f * ondulacion);
                VerticeBarra(p[v], TinteBarra(base, p[v]));
            }
        }
    }
    rlEnd();

    // Bocanadas: corona alrededor del templo (lo de adelante queda fuera
    // de cuadro), derivando lento hacia la derecha.
    int cantidad = CalidadDecoracion() == CALIDAD_ALTA ? 64 : (CalidadDecoracion() == CALIDAD_MEDIA ? 34 : 14);

    for (int k = 0; k < cantidad; k++)
    {
        float r = 8.0f + 50.0f * Hash01Barra(k, 80);
        float a = 3.1416f + 3.1416f * 1.25f * Hash01Barra(k, 81) - 0.4f;
        float x = std::cos(a) * r;
        float z = std::sin(a) * r * 0.85f;
        x = std::fmod(x + 80.0f + t * (0.25f + 0.2f * Hash01Barra(k, 82)), 160.0f) - 80.0f;
        float radio = 3.0f + 4.0f * Hash01Barra(k, 83) + r * 0.05f;
        Vector3 c = { x, NIVEL_NUBES_BARRA + 0.3f * Hash01Barra(k, 84), z };
        BocanadaNubeBarra(c, radio, 0.38f, CantidadNieblaBarra(c));
    }
}


//==================================================
// ISLAS FLOTANTES, TEMPLO, PUERTA Y CASCADAS
//==================================================

// Isla flotante: tapa de cesped con borde y roca colgante irregular en
// estratos que se afina hacia abajo.
static void IslaFlotanteBarra(
    Vector3 c,
    float radio,
    float profundidad,
    int semilla,
    Color tapa,
    float niebla
)
{
    const int LADOS = 14;
    const int NIVELES = 4;
    Vector3 anillos[NIVELES + 1][LADOS];

    for (int l = 0; l <= NIVELES; l++)
    {
        float f = (float)l / (float)NIVELES;
        float y = c.y - 0.22f - profundidad * f;
        float r = radio * std::pow(1.0f - f, 0.85f);

        for (int k = 0; k < LADOS; k++)
        {
            float a = 6.2832f * (float)k / (float)LADOS;
            float irregular = l == 0
                ? 0.9f + 0.14f * Hash01Barra(semilla, k)
                : 0.72f + 0.42f * Hash01Barra(semilla + l * 17, k);
            float rk = r * irregular;
            anillos[l][k] = { c.x + std::cos(a) * rk, y, c.z + std::sin(a) * rk };
        }
    }

    Color roca = NieblaBarra(COLOR_ROCA_BARRA, niebla);
    Color rocaCalida = NieblaBarra(Color{ 186, 146, 124, 255 }, niebla);
    Color cesped = NieblaBarra(tapa, niebla);
    Color borde = NieblaBarra(TonoBarra(tapa, 0.72f), niebla);

    rlBegin(RL_QUADS);

    for (int k = 0; k < LADOS; k++)
    {
        int k1 = (k + 1) % LADOS;
        float am = 6.2832f * ((float)k + 0.5f) / (float)LADOS;
        float luz = LuzLateralBarra(am);

        Vector3 a0 = anillos[0][k];
        Vector3 a1 = anillos[0][k1];

        // Tapa de cesped.
        QuadBarra(c, { a1.x, c.y, a1.z }, { a0.x, c.y, a0.z }, c, SombraBarra(cesped, 0.94f + 0.1f * Hash01Barra(semilla, k + 40), true));

        // Borde de cesped que cae sobre la roca.
        QuadBarra({ a0.x, c.y, a0.z }, { a1.x, c.y, a1.z }, a1, a0, SombraBarra(borde, luz, false));

        for (int l = 0; l < NIVELES; l++)
        {
            Color estrato = (l + semilla) % 2 == 0 ? roca : rocaCalida;
            float oscuro = 1.0f - 0.32f * (float)l / (float)NIVELES;
            QuadBarra(anillos[l][k], anillos[l][k1], anillos[l + 1][k1], anillos[l + 1][k], SombraBarra(estrato, luz * oscuro, false));
        }
    }

    rlEnd();
}


// Arbol de flor rosada (copas de esferas con cara iluminada).
static void ArbolFlorBarra(Vector3 base, float alto, float niebla)
{
    PrismaBarra(base, 0.14f, 0.09f, alto, 6, NieblaBarra(Color{ 110, 74, 62, 255 }, niebla));
    Color sombra = NieblaBarra(Color{ 214, 128, 160, 255 }, niebla);
    Color luz = NieblaBarra(Color{ 252, 190, 206, 255 }, niebla);
    Vector3 copa = { base.x, base.y + alto, base.z };
    DrawSphereEx(copa, alto * 0.42f, 5, 8, sombra);
    DrawSphereEx({ copa.x + alto * 0.22f, copa.y + alto * 0.12f, copa.z + 0.1f }, alto * 0.32f, 5, 8, luz);
    DrawSphereEx({ copa.x - alto * 0.26f, copa.y + alto * 0.06f, copa.z }, alto * 0.28f, 5, 8, luz);
}


// Banderola que ondea, colgada del centro de su borde superior y mirando
// a camara (+Z). Franja de oro arriba, cuerpo ciruela, banda crema y
// cola de golondrina. MODELO FUTURO: estandarte de tela GLB.
static void BanderolaBarra(
    Vector3 arriba,
    float ancho,
    float largo,
    float t,
    float fase,
    float niebla,
    Color tela = COLOR_TELA_BARRA
)
{
    const int TRAMOS = 10;
    float reducir = ObtenerFactorMovimientoCamaraMinijuegos() > 0.0f ? 1.0f : 0.35f;

    Vector3 izq[TRAMOS + 1];
    Vector3 der[TRAMOS + 1];
    float pendiente[TRAMOS + 1];

    for (int k = 0; k <= TRAMOS; k++)
    {
        float v = (float)k / (float)TRAMOS;
        float onda = std::sin(t * 2.4f + fase - v * 4.2f);
        float dz = onda * 0.32f * v * reducir;
        float dx = std::sin(t * 1.5f + fase * 1.3f - v * 2.6f) * 0.12f * v * reducir;
        float y = arriba.y - v * largo;
        float ancho2 = ancho * 0.5f;
        izq[k] = { arriba.x - ancho2 + dx, y, arriba.z + dz };
        der[k] = { arriba.x + ancho2 + dx, y, arriba.z + dz * 0.8f };
        pendiente[k] = std::cos(t * 2.4f + fase - v * 4.2f);
    }

    Color oro = NieblaBarra(COLOR_ORO_BARRA, niebla);
    Color cuerpo = NieblaBarra(tela, niebla);
    Color crema = NieblaBarra(COLOR_TELA_CREMA_BARRA, niebla);

    rlBegin(RL_QUADS);

    for (int k = 0; k < TRAMOS; k++)
    {
        float v = ((float)k + 0.5f) / (float)TRAMOS;
        Color color = v < 0.1f ? oro : ((v > 0.46f && v < 0.6f) ? crema : cuerpo);
        if (v > 0.6f && v < 0.66f) color = oro;
        float luz = 0.82f + 0.16f * pendiente[k];
        QuadBarra(izq[k], der[k], der[k + 1], izq[k + 1], TonoBarra(color, luz));
    }

    rlEnd();

    // Cola de golondrina.
    Vector3 medio = { (izq[TRAMOS].x + der[TRAMOS].x) * 0.5f, izq[TRAMOS].y + largo * 0.08f, (izq[TRAMOS].z + der[TRAMOS].z) * 0.5f };
    Vector3 puntaI = { izq[TRAMOS].x, izq[TRAMOS].y - largo * 0.12f, izq[TRAMOS].z };
    Vector3 puntaD = { der[TRAMOS].x, der[TRAMOS].y - largo * 0.12f, der[TRAMOS].z };
    Color cola = TonoBarra(cuerpo, 0.82f + 0.16f * pendiente[TRAMOS]);

    rlBegin(RL_TRIANGLES);
    VerticeBarra(izq[TRAMOS], cola); VerticeBarra(medio, cola); VerticeBarra(puntaI, cola);
    VerticeBarra(medio, cola); VerticeBarra(der[TRAMOS], cola); VerticeBarra(puntaD, cola);
    rlEnd();
}


// Templo redondo de columnas con cupula de bronce.
// MODELO FUTURO: templete celeste GLB.
static void TemploRedondoBarra(Vector3 base, float escala, float niebla)
{
    Color piedra = NieblaBarra(COLOR_PIEDRA_CLARA_BARRA, niebla);
    Color piedraMedia = NieblaBarra(COLOR_PIEDRA_BARRA, niebla);
    Color bronce = NieblaBarra(COLOR_BRONCE_BARRA, niebla);
    Color oro = NieblaBarra(COLOR_ORO_BARRA, niebla);
    float e = escala;

    PrismaBarra(base, 2.9f * e, 2.9f * e, 0.22f * e, 20, piedraMedia);
    PrismaBarra({ base.x, base.y + 0.22f * e, base.z }, 2.6f * e, 2.6f * e, 0.2f * e, 20, piedra);

    float y0 = base.y + 0.42f * e;

    for (int k = 0; k < 8; k++)
    {
        float a = 6.2832f * ((float)k + 0.5f) / 8.0f;
        Vector3 p = { base.x + std::cos(a) * 2.15f * e, y0, base.z + std::sin(a) * 2.15f * e };
        PrismaBarra(p, 0.2f * e, 0.17f * e, 2.4f * e, 8, piedra, 0.0f, true);
        CuboBarra({ p.x, y0 + 2.45f * e, p.z }, { 0.5f * e, 0.12f * e, 0.5f * e }, piedra);
    }

    // Altar con la llama celeste.
    PrismaBarra({ base.x, y0, base.z }, 0.5f * e, 0.42f * e, 0.8f * e, 8, piedraMedia);

    float yE = y0 + 2.5f * e;
    PrismaBarra({ base.x, yE, base.z }, 2.55f * e, 2.55f * e, 0.38f * e, 20, piedra);
    PrismaBarra({ base.x, yE + 0.38f * e, base.z }, 2.62f * e, 2.62f * e, 0.08f * e, 20, oro);

    // Cupula en anillos.
    float yC = yE + 0.46f * e;
    const int ANILLOS = 5;

    for (int k = 0; k < ANILLOS; k++)
    {
        float f0 = 1.5708f * (float)k / (float)ANILLOS;
        float f1 = 1.5708f * (float)(k + 1) / (float)ANILLOS;
        float r0 = 2.35f * e * std::cos(f0);
        float r1 = 2.35f * e * std::cos(f1);
        float h0 = 1.8f * e * std::sin(f0);
        float h1 = 1.8f * e * std::sin(f1);
        PrismaBarra({ base.x, yC + h0, base.z }, r0, r1, h1 - h0, 20, TonoBarra(bronce, 1.0f + 0.05f * (float)k));
    }

    PrismaBarra({ base.x, yC + 1.8f * e, base.z }, 0.14f * e, 0.0f, 1.0f * e, 6, oro);
}


// Puerta celeste: dos pilares, dintel doble con extremos levantados,
// disco solar de bronce y banderolas colgando. MODELO FUTURO: portal GLB.
static void PuertaCelesteBarra(Vector3 base, float escala, float t, float niebla)
{
    Color piedra = NieblaBarra(COLOR_PIEDRA_CLARA_BARRA, niebla);
    Color piedraMedia = NieblaBarra(COLOR_PIEDRA_BARRA, niebla);
    Color bronce = NieblaBarra(COLOR_BRONCE_BARRA, niebla);
    float e = escala;

    for (int s = -1; s <= 1; s += 2)
    {
        float x = base.x + (float)s * 1.6f * e;
        CuboBarra({ x, base.y + 0.15f * e, base.z }, { 0.8f * e, 0.3f * e, 0.8f * e }, piedraMedia);
        CuboBarra({ x, base.y + 1.85f * e, base.z }, { 0.48f * e, 3.1f * e, 0.48f * e }, piedra);
        CuboBarra({ x, base.y + 3.45f * e, base.z }, { 0.7f * e, 0.18f * e, 0.7f * e }, piedraMedia);
    }

    CuboBarra({ base.x, base.y + 3.0f * e, base.z }, { 3.7f * e, 0.24f * e, 0.36f * e }, piedraMedia);
    CuboBarra({ base.x, base.y + 3.68f * e, base.z }, { 4.6f * e, 0.32f * e, 0.5f * e }, piedra);

    for (int s = -1; s <= 1; s += 2)
    {
        CuboBarra({ base.x + (float)s * 2.4f * e, base.y + 3.86f * e, base.z }, { 0.5f * e, 0.24f * e, 0.5f * e }, piedra);
    }

    // Disco solar de bronce que gira despacio.
    rlPushMatrix();
    rlTranslatef(base.x, base.y + 2.3f * e, base.z + 0.05f);
    rlRotatef(90.0f, 1.0f, 0.0f, 0.0f);
    rlRotatef(t * 12.0f, 0.0f, 1.0f, 0.0f);
    EngranajeBarra(0.62f * e, 0.12f * e, 12, bronce);
    rlPopMatrix();

    for (int s = -1; s <= 1; s += 2)
    {
        BanderolaBarra({ base.x + (float)s * 0.95f * e, base.y + 2.86f * e, base.z + 0.2f * e }, 0.5f * e, 1.7f * e, t, (float)s * 1.3f, niebla);
    }
}


// Cascada que cae de una isla al mar de nubes: franjas que bajan.
// Translucida: dibujar con la mascara de profundidad apagada.
static void CascadaBarra(Vector3 origen, float ancho, float t, float niebla)
{
    const int TRAMOS = 18;
    float caida = origen.y - NIVEL_NUBES_BARRA;
    if (caida < 1.0f) return;

    rlBegin(RL_QUADS);

    for (int k = 0; k < TRAMOS; k++)
    {
        float v0 = (float)k / (float)TRAMOS;
        float v1 = (float)(k + 1) / (float)TRAMOS;
        float y0 = origen.y - caida * v0;
        float y1 = origen.y - caida * v1;
        // Arco de salida: el agua avanza un poco antes de caer recta.
        float z0 = origen.z + 0.7f * std::sqrt(v0);
        float z1 = origen.z + 0.7f * std::sqrt(v1);
        float a0 = ancho * (1.0f + v0 * 0.7f) * 0.5f;
        float a1 = ancho * (1.0f + v1 * 0.7f) * 0.5f;

        float franja0 = 0.5f + 0.5f * std::sin(v0 * caida * 1.6f - t * 7.0f);
        float franja1 = 0.5f + 0.5f * std::sin(v1 * caida * 1.6f - t * 7.0f);
        Color c0 = NieblaBarra(MezclaBarra(Color{ 170, 214, 240, 255 }, Color{ 250, 252, 255, 255 }, franja0), niebla);
        Color c1 = NieblaBarra(MezclaBarra(Color{ 170, 214, 240, 255 }, Color{ 250, 252, 255, 255 }, franja1), niebla);
        c0.a = (unsigned char)(220.0f * (1.0f - v0 * 0.55f));
        c1.a = (unsigned char)(220.0f * (1.0f - v1 * 0.55f));

        VerticeBarra({ origen.x - a0, y0, z0 }, c0);
        VerticeBarra({ origen.x + a0, y0, z0 }, c0);
        VerticeBarra({ origen.x + a1, y1, z1 }, c1);
        VerticeBarra({ origen.x - a1, y1, z1 }, c1);
    }

    rlEnd();
}


// Posiciones de las islas de media distancia (con su vaiven).
static Vector3 PosicionIslaBarra(Vector3 base, int indice, float t)
{
    float vaiven = std::sin(t * 0.5f + (float)indice * 1.7f) * 0.18f;
    return { base.x, base.y + vaiven, base.z };
}


static const Vector3 ISLA_TEMPLO_BARRA = { -17.0f, -4.6f, -22.0f };
static const Vector3 ISLA_CEREZOS_BARRA = { 16.0f, -4.0f, -19.0f };
static const Vector3 ISLA_PUERTA_BARRA = { 3.0f, -6.6f, -40.0f };


static void DibujarIslasLejanasBarra(float t)
{
    bool baja = CalidadDecoracion() == CALIDAD_BAJA;

    // Templo redondo a la izquierda.
    {
        Vector3 c = PosicionIslaBarra(ISLA_TEMPLO_BARRA, 0, t);
        float n = CantidadNieblaBarra(c);
        IslaFlotanteBarra(c, 4.6f, 6.5f, 11, COLOR_CESPED_BARRA, n);

        if (!baja)
        {
            TemploRedondoBarra({ c.x, c.y, c.z }, 0.95f, n);
            ArbolFlorBarra({ c.x + 3.3f, c.y, c.z + 1.6f }, 1.6f, n);
            ArbolFlorBarra({ c.x - 3.4f, c.y, c.z + 1.0f }, 1.3f, n);
        }
    }

    // Isla de los cerezos a la derecha.
    {
        Vector3 c = PosicionIslaBarra(ISLA_CEREZOS_BARRA, 1, t);
        float n = CantidadNieblaBarra(c);
        IslaFlotanteBarra(c, 3.8f, 5.5f, 23, COLOR_CESPED_BARRA, n);

        if (!baja)
        {
            ArbolFlorBarra({ c.x - 1.2f, c.y, c.z - 0.6f }, 2.4f, n);
            ArbolFlorBarra({ c.x + 1.4f, c.y, c.z - 1.4f }, 2.0f, n);
            ArbolFlorBarra({ c.x + 0.3f, c.y, c.z + 1.6f }, 1.5f, n);
            // Farol de piedra.
            PrismaBarra({ c.x - 2.4f, c.y, c.z + 1.4f }, 0.22f, 0.16f, 0.9f, 6, NieblaBarra(COLOR_PIEDRA_CLARA_BARRA, n));
            CuboBarra({ c.x - 2.4f, c.y + 1.05f, c.z + 1.4f }, { 0.5f, 0.3f, 0.5f }, NieblaBarra(Color{ 255, 214, 150, 255 }, n));
            PrismaBarra({ c.x - 2.4f, c.y + 1.2f, c.z + 1.4f }, 0.48f, 0.0f, 0.35f, 4, NieblaBarra(COLOR_PIEDRA_BARRA, n), 0.785f);
        }
    }

    // Puerta celeste al fondo.
    {
        Vector3 c = PosicionIslaBarra(ISLA_PUERTA_BARRA, 2, t);
        float n = CantidadNieblaBarra(c);
        IslaFlotanteBarra(c, 4.0f, 5.0f, 37, COLOR_CESPED_BARRA, n);

        if (!baja)
        {
            PuertaCelesteBarra({ c.x, c.y, c.z }, 1.25f, t, n);
        }
    }

    if (baja)
    {
        return;
    }

    // Peñascos sueltos, de varios tamanos y distancias.
    const Vector3 rocas[6] =
    {
        { -6.5f, -5.6f, -30.0f }, { 9.5f, -7.0f, -34.0f }, { -28.0f, -5.0f, -38.0f },
        { 28.0f, -3.6f, -34.0f }, { -12.0f, -8.2f, -48.0f }, { 20.0f, -7.6f, -52.0f }
    };
    const float tamanos[6] = { 1.3f, 1.7f, 2.6f, 2.3f, 2.0f, 2.8f };
    int paso = CalidadDecoracion() == CALIDAD_ALTA ? 1 : 2;

    for (int k = 0; k < 6; k += paso)
    {
        Vector3 c = PosicionIslaBarra(rocas[k], k + 3, t);
        float n = CantidadNieblaBarra(c);
        IslaFlotanteBarra(c, tamanos[k], tamanos[k] * 1.6f, 50 + k * 7, COLOR_CESPED_BARRA, n);

        if (k == 3)
        {
            // Arco roto sobre el peñasco de la derecha.
            Color piedra = NieblaBarra(COLOR_PIEDRA_CLARA_BARRA, n);
            CuboBarra({ c.x - 0.8f, c.y + 1.0f, c.z }, { 0.4f, 2.0f, 0.4f }, piedra);
            CuboBarra({ c.x + 0.8f, c.y + 0.7f, c.z }, { 0.4f, 1.4f, 0.4f }, piedra);
            CuboBarra({ c.x - 0.2f, c.y + 2.1f, c.z }, { 1.6f, 0.3f, 0.45f }, piedra);
        }
        else if (k == 0 || k == 2)
        {
            ArbolFlorBarra({ c.x, c.y, c.z }, tamanos[k] * 0.9f, n);
        }
    }
}


// Partes translucidas de las islas: cascadas.
static void DibujarCascadasBarra(float t)
{
    {
        Vector3 c = PosicionIslaBarra(ISLA_TEMPLO_BARRA, 0, t);
        Vector3 o = { c.x + 2.2f, c.y - 0.1f, c.z + 4.0f };
        CascadaBarra(o, 0.9f, t, CantidadNieblaBarra(o));
    }

    {
        Vector3 c = PosicionIslaBarra(ISLA_CEREZOS_BARRA, 1, t);
        Vector3 o = { c.x - 1.0f, c.y - 0.1f, c.z + 3.4f };
        CascadaBarra(o, 1.1f, t + 0.7f, CantidadNieblaBarra(o));
        Vector3 o2 = { c.x + 2.0f, c.y - 0.1f, c.z + 2.6f };
        CascadaBarra(o2, 0.5f, t + 1.9f, CantidadNieblaBarra(o2));
    }

    {
        Vector3 c = PosicionIslaBarra(ISLA_PUERTA_BARRA, 2, t);
        Vector3 o = { c.x + 2.6f, c.y - 0.1f, c.z + 2.8f };
        CascadaBarra(o, 0.8f, t + 0.3f, CantidadNieblaBarra(o));
    }
}


// Bandada de aves que vuela en circulos detras del templo y algunas
// lejanas que planean. MODELO FUTURO: ave GLB con aleteo.
static void DibujarAvesBarra(float t)
{
    if (CalidadDecoracion() == CALIDAD_BAJA)
    {
        return;
    }

    int cantidad = CalidadDecoracion() == CALIDAD_ALTA ? 11 : 6;

    for (int g = 0; g < cantidad; g++)
    {
        float fase = (float)g * 1.37f;
        bool lejana = g >= 7;
        float radio = lejana ? 22.0f + 4.0f * (float)(g % 3) : 6.0f + 1.4f * (float)(g % 4);
        float vel = lejana ? 0.08f : 0.32f + 0.03f * (float)(g % 3);
        float a = t * vel + fase;
        Vector3 c =
        {
            std::cos(a) * radio + (lejana ? 0.0f : (g % 2 == 0 ? -4.0f : 4.0f)),
            (lejana ? 0.0f : 2.4f) + 0.7f * (float)(g % 3) + 0.4f * std::sin(t * 0.7f + fase),
            (lejana ? -40.0f : -11.0f) + std::sin(a) * radio * 0.45f
        };
        float aleteo = std::sin(t * 8.0f + fase * 3.0f) * 0.32f;
        float dx = -std::sin(a);
        float dz = std::cos(a) * 0.45f;
        float largo = std::sqrt(dx * dx + dz * dz);
        dx /= largo;
        dz /= largo;
        Vector3 lado = { -dz, 0.0f, dx };
        float escala = lejana ? 1.5f : 0.95f;
        Color pluma = TinteBarra(Color{ 84, 62, 96, 255 }, c);

        rlBegin(RL_TRIANGLES);
        for (int s = -1; s <= 1; s += 2)
        {
            Vector3 extremo =
            {
                c.x + lado.x * 0.55f * escala * (float)s,
                c.y + (0.16f + aleteo) * escala,
                c.z + lado.z * 0.55f * escala * (float)s
            };
            Vector3 frente = { c.x + dx * 0.2f * escala, c.y, c.z + dz * 0.2f * escala };
            Vector3 atras = { c.x - dx * 0.16f * escala, c.y, c.z - dz * 0.16f * escala };
            VerticeBarra(frente, pluma);
            VerticeBarra(extremo, pluma);
            VerticeBarra(atras, pluma);
        }
        rlEnd();
    }
}


//==================================================
// ANILLO CERCANO: ISLOTES CON UTILERIA
//==================================================

// Brasero de bronce sobre pedestal; la llama titila.
// MODELO FUTURO: brasero GLB.
static void BraseroBarra(Vector3 base, float t, int semilla)
{
    PrismaBarra(base, 0.42f, 0.36f, 0.22f, 8, COLOR_PIEDRA_BARRA, 0.39f);
    PrismaBarra({ base.x, base.y + 0.22f, base.z }, 0.2f, 0.16f, 0.7f, 8, COLOR_PIEDRA_CLARA_BARRA, 0.39f, true);
    PrismaBarra({ base.x, base.y + 0.92f, base.z }, 0.2f, 0.5f, 0.32f, 12, COLOR_BRONCE_BARRA);
    AnilloBarra({ base.x, base.y + 1.245f, base.z }, 0.4f, 0.52f, COLOR_BRONCE_CLARO_BARRA, 16);

    // Llama: conos que titilan.
    for (int k = 0; k < 3; k++)
    {
        float a = (float)k * 2.094f + t * 1.3f;
        float titila = 0.75f + 0.25f * std::sin(t * (11.0f + (float)k * 3.0f) + (float)semilla);
        Vector3 pie = { base.x + std::cos(a) * 0.12f, base.y + 1.2f, base.z + std::sin(a) * 0.12f };
        DrawCylinderEx(pie, { pie.x, pie.y + 0.62f * titila, pie.z }, 0.17f, 0.0f, 6, Color{ 255, 128, 52, 255 });
    }

    float alto = 0.8f + 0.15f * std::sin(t * 13.0f + (float)semilla);
    DrawCylinderEx({ base.x, base.y + 1.2f, base.z }, { base.x, base.y + 1.2f + alto, base.z }, 0.16f, 0.0f, 6, Color{ 255, 206, 96, 255 });
}


// Mastil con travesano y banderola larga.
static void MastilBanderolaBarra(Vector3 base, float alto, float t, float fase, Color tela)
{
    PrismaBarra(base, 0.24f, 0.2f, 0.18f, 8, COLOR_PIEDRA_BARRA);
    PrismaBarra({ base.x, base.y + 0.18f, base.z }, 0.07f, 0.06f, alto, 6, COLOR_BRONCE_OSCURO_BARRA);
    CuboBarra({ base.x, base.y + alto + 0.05f, base.z }, { 1.1f, 0.08f, 0.08f }, COLOR_BRONCE_BARRA);
    (DrawSphere)({ base.x, base.y + alto + 0.24f, base.z }, 0.12f, COLOR_ORO_BARRA);
    BanderolaBarra({ base.x, base.y + alto, base.z + 0.06f }, 0.9f, alto * 0.72f, t, fase, 0.0f, tela);
}


// Vasija de piedra con borde de oro.
static void VasijaBarra(Vector3 base, float escala)
{
    float e = escala;
    PrismaBarra(base, 0.18f * e, 0.32f * e, 0.3f * e, 10, COLOR_PIEDRA_CLARA_BARRA);
    PrismaBarra({ base.x, base.y + 0.3f * e, base.z }, 0.32f * e, 0.16f * e, 0.3f * e, 10, COLOR_PIEDRA_CLARA_BARRA);
    PrismaBarra({ base.x, base.y + 0.6f * e, base.z }, 0.16f * e, 0.22f * e, 0.12f * e, 10, COLOR_ORO_BARRA);
}


// Obelisco con piramidion de oro.
static void ObeliscoBarra(Vector3 base, float alto)
{
    CuboBarra({ base.x, base.y + 0.12f, base.z }, { 0.7f, 0.24f, 0.7f }, COLOR_PIEDRA_BARRA);
    PrismaBarra({ base.x, base.y + 0.24f, base.z }, 0.3f, 0.2f, alto, 4, COLOR_PIEDRA_CLARA_BARRA, 0.785f);
    PrismaBarra({ base.x, base.y + 0.24f + alto, base.z }, 0.2f, 0.0f, 0.4f, 4, COLOR_ORO_BARRA, 0.785f);
}


// Arco con campana de bronce que se mece.
static void CampanaBarra(Vector3 base, float t)
{
    for (int s = -1; s <= 1; s += 2)
    {
        CuboBarra({ base.x + (float)s * 0.6f, base.y + 0.8f, base.z }, { 0.22f, 1.6f, 0.22f }, COLOR_PIEDRA_CLARA_BARRA);
    }

    CuboBarra({ base.x, base.y + 1.68f, base.z }, { 1.6f, 0.18f, 0.32f }, COLOR_PIEDRA_BARRA);
    CuboBarra({ base.x, base.y + 1.84f, base.z }, { 1.2f, 0.14f, 0.26f }, COLOR_PIEDRA_CLARA_BARRA);

    float mecer = std::sin(t * 1.6f) * 12.0f * (ObtenerFactorMovimientoCamaraMinijuegos() > 0.0f ? 1.0f : 0.3f);
    rlPushMatrix();
    rlTranslatef(base.x, base.y + 1.58f, base.z);
    rlRotatef(mecer, 0.0f, 0.0f, 1.0f);
    PrismaBarra({ 0.0f, -0.62f, 0.0f }, 0.34f, 0.16f, 0.52f, 12, COLOR_BRONCE_BARRA);
    PrismaBarra({ 0.0f, -0.12f, 0.0f }, 0.16f, 0.05f, 0.12f, 8, COLOR_BRONCE_OSCURO_BARRA);
    (DrawSphere)({ 0.0f, -0.66f, 0.0f }, 0.07f, COLOR_BRONCE_OSCURO_BARRA);
    rlPopMatrix();
}


// Cadena de eslabones de bronce en catenaria entre dos puntos.
static void CadenaBarra(Vector3 a, Vector3 b, float comba, int eslabones)
{
    for (int k = 0; k <= eslabones; k++)
    {
        float u = (float)k / (float)eslabones;
        Vector3 p =
        {
            a.x + (b.x - a.x) * u,
            a.y + (b.y - a.y) * u - comba * 4.0f * u * (1.0f - u),
            a.z + (b.z - a.z) * u
        };
        Vector3 tam = k % 2 == 0 ? Vector3{ 0.13f, 0.08f, 0.06f } : Vector3{ 0.06f, 0.08f, 0.13f };
        CuboBarra(p, tam, COLOR_BRONCE_OSCURO_BARRA);
    }
}


// Islotes que rodean la plataforma (vaiven suave).
static const Vector3 ISLOTES_BARRA[4] =
{
    { -8.7f, -1.5f, -4.2f },
    {  8.7f, -1.5f, -4.2f },
    { -9.4f, -2.6f,  0.9f },
    {  9.4f, -2.6f,  0.9f }
};

static const float RADIO_ISLOTE_BARRA[4] = { 1.8f, 1.8f, 1.6f, 1.6f };


static Vector3 PosicionIsloteBarra(int indice, float t)
{
    float vaiven = std::sin(t * 0.8f + (float)indice * 1.9f) * 0.12f;
    Vector3 p = ISLOTES_BARRA[indice];
    p.y += vaiven;
    return p;
}


// Punta del brasero de cada islote trasero (para las luces).
static Vector3 PosicionBraseroBarra(int indice, float t)
{
    Vector3 p = PosicionIsloteBarra(indice, t);
    float lado = indice == 0 ? 1.0f : -1.0f;
    return { p.x + lado * 0.55f, p.y, p.z + 0.5f };
}


static void DibujarIsletasBarra(float t)
{
    bool media = CalidadDecoracion() != CALIDAD_ALTA;

    for (int i = 0; i < 4; i++)
    {
        Vector3 c = PosicionIsloteBarra(i, t);
        float r = RADIO_ISLOTE_BARRA[i];
        IslaFlotanteBarra(c, r, r * 2.4f, 90 + i * 13, COLOR_CESPED_BARRA, 0.0f);

        // Losas en la cima del islote.
        PrismaBarra({ c.x, c.y, c.z }, r * 0.7f, r * 0.7f, 0.05f, 10, COLOR_PIEDRA_BARRA);

        // Cadena que lo amarra al borde del templo.
        float lado = c.x < 0.0f ? -1.0f : 1.0f;
        Vector3 borde = { lado * 5.0f, -0.62f, c.z * 0.55f };
        CadenaBarra(borde, { c.x - lado * r * 0.8f, c.y - 0.1f, c.z }, 0.9f, media ? 12 : 20);
    }

    // Traseros: brasero, banderola y vasija u obelisco.
    for (int i = 0; i < 2; i++)
    {
        Vector3 c = PosicionIsloteBarra(i, t);
        float lado = i == 0 ? 1.0f : -1.0f;
        BraseroBarra(PosicionBraseroBarra(i, t), t, i);
        MastilBanderolaBarra({ c.x - lado * 0.6f, c.y, c.z - 0.5f }, 4.2f, t, (float)i * 2.1f, COLOR_TELA_BARRA);

        if (!media)
        {
            if (i == 0) VasijaBarra({ c.x - 0.2f, c.y, c.z + 0.9f }, 1.0f);
            else ObeliscoBarra({ c.x + 0.1f, c.y, c.z + 0.6f }, 1.6f);
        }
    }

    // Delanteros: campana, vasijas, obelisco y banderola crema.
    {
        Vector3 c = PosicionIsloteBarra(2, t);
        CampanaBarra({ c.x + 0.2f, c.y, c.z - 0.2f }, t);
        VasijaBarra({ c.x + 0.9f, c.y, c.z + 0.7f }, 0.8f);
        if (!media) VasijaBarra({ c.x - 0.8f, c.y, c.z + 0.6f }, 0.65f);
    }

    {
        Vector3 c = PosicionIsloteBarra(3, t);
        ObeliscoBarra({ c.x - 0.3f, c.y, c.z - 0.3f }, 2.0f);
        MastilBanderolaBarra({ c.x + 0.7f, c.y, c.z - 0.5f }, 3.4f, t, 4.0f, Color{ 104, 72, 138, 255 });
        if (!media) VasijaBarra({ c.x - 0.9f, c.y, c.z + 0.6f }, 0.7f);
    }
}


//==================================================
// PLATAFORMA DE PIEDRA TALLADA
//==================================================

static const float ALTO_CORNISA_BARRA = 0.55f;


// Faroles que cuelgan bajo la cornisa (angulos en radianes).
static const float ANGULOS_FAROLES_BARRA[5] = { 0.35f, 0.95f, 1.5708f, 2.19f, 2.79f };


static Vector3 PosicionFarolBarra(int k, float t)
{
    float a = ANGULOS_FAROLES_BARRA[k];
    float mecer = std::sin(t * 1.4f + (float)k) * 0.04f;
    return { std::cos(a) * 5.62f + mecer, -1.25f, std::sin(a) * 5.62f };
}


// Cuerpo de la plataforma: canto de la cornisa (coincide con el borde
// real de la arena), moldura de bronce y base tallada que se afina hacia
// un cristal colgante. MODELO FUTURO: plataforma del templo GLB.
static void DibujarCuerpoPlataformaBarra(float giro, float t)
{
    // Canto de la cornisa, radio exacto de la arena.
    PrismaBarra({ 0.0f, -ALTO_CORNISA_BARRA, 0.0f }, RADIO_ARENA_BARRA, RADIO_ARENA_BARRA, ALTO_CORNISA_BARRA - 0.005f, 48, COLOR_PIEDRA_CLARA_BARRA);

    // Bloques del canto: juntas verticales.
    for (int k = 0; k < 36; k++)
    {
        float a = 6.2832f * (float)k / 36.0f;
        if (std::sin(a) < -0.3f) continue;
        Vector3 p = { std::cos(a) * (RADIO_ARENA_BARRA + 0.004f), -ALTO_CORNISA_BARRA * 0.5f, std::sin(a) * (RADIO_ARENA_BARRA + 0.004f) };
        rlPushMatrix();
        rlTranslatef(p.x, p.y, p.z);
        rlRotatef(-a * RAD2DEG, 0.0f, 1.0f, 0.0f);
        CuboBarra({ 0.0f, 0.0f, 0.0f }, { 0.01f, ALTO_CORNISA_BARRA * 0.9f, 0.035f }, COLOR_JUNTA_BARRA);
        rlPopMatrix();
    }

    // Moldura de bronce con remaches.
    PrismaBarra({ 0.0f, -ALTO_CORNISA_BARRA - 0.2f, 0.0f }, 5.3f, 5.46f, 0.2f, 48, COLOR_BRONCE_BARRA);

    for (int k = 0; k < 48; k++)
    {
        float a = 6.2832f * ((float)k + 0.5f) / 48.0f;
        if (std::sin(a) < -0.3f) continue;
        (DrawSphere)({ std::cos(a) * 5.42f, -ALTO_CORNISA_BARRA - 0.08f, std::sin(a) * 5.42f }, 0.04f, COLOR_BRONCE_CLARO_BARRA);
    }

    // Base tallada en tres tramos.
    float y = -ALTO_CORNISA_BARRA - 0.2f;
    PrismaBarra({ 0.0f, y - 1.0f, 0.0f }, 4.5f, 5.25f, 1.0f, 40, COLOR_PIEDRA_BARRA);
    PrismaBarra({ 0.0f, y - 1.22f, 0.0f }, 4.4f, 4.5f, 0.22f, 40, COLOR_BRONCE_OSCURO_BARRA);
    PrismaBarra({ 0.0f, y - 3.6f, 0.0f }, 2.3f, 4.4f, 2.38f, 32, TonoBarra(COLOR_PIEDRA_BARRA, 0.9f), 0.0f, true);
    PrismaBarra({ 0.0f, y - 5.2f, 0.0f }, 0.7f, 2.3f, 1.6f, 24, TonoBarra(COLOR_PIEDRA_BARRA, 0.78f));
    PrismaBarra({ 0.0f, y - 6.3f, 0.0f }, 0.0f, 0.7f, 1.1f, 12, COLOR_BRONCE_BARRA);
    (DrawSphere)({ 0.0f, y - 6.5f, 0.0f }, 0.32f, COLOR_ACENTO_BARRA_BASE);

    // Engranajes verticales encastrados en la moldura (giran con la barra).
    if (CalidadDecoracion() != CALIDAD_BAJA)
    {
        const float angulos[2] = { 0.42f, 2.72f };

        for (int k = 0; k < 2; k++)
        {
            float a = angulos[k];
            float sentido = k == 0 ? 1.0f : -1.0f;

            rlPushMatrix();
            rlTranslatef(std::cos(a) * 5.38f, -ALTO_CORNISA_BARRA - 0.62f, std::sin(a) * 5.38f);
            rlRotatef(-a * RAD2DEG, 0.0f, 1.0f, 0.0f);
            rlRotatef(90.0f, 0.0f, 0.0f, 1.0f);
            rlPushMatrix();
            rlRotatef(giro * 70.0f * sentido, 0.0f, 1.0f, 0.0f);
            EngranajeBarra(0.62f, 0.16f, 12, COLOR_BRONCE_BARRA);
            rlPopMatrix();
            rlTranslatef(0.0f, 0.0f, 0.84f);
            rlRotatef(-giro * 130.0f * sentido + 15.0f, 0.0f, 1.0f, 0.0f);
            EngranajeBarra(0.34f, 0.14f, 8, COLOR_BRONCE_CLARO_BARRA);
            rlPopMatrix();
        }
    }

    // Guirnaldas de cadena y faroles colgando de la moldura.
    for (int k = 0; k < 5; k++)
    {
        Vector3 f = PosicionFarolBarra(k, t);
        float a = ANGULOS_FAROLES_BARRA[k];
        Vector3 ancla = { std::cos(a) * 5.4f, -ALTO_CORNISA_BARRA - 0.2f, std::sin(a) * 5.4f };
        CadenaBarra(ancla, { f.x, f.y + 0.3f, f.z }, 0.0f, 4);
        CuboBarra({ f.x, f.y + 0.24f, f.z }, { 0.3f, 0.06f, 0.3f }, COLOR_BRONCE_OSCURO_BARRA);
        CuboBarra(f, { 0.24f, 0.36f, 0.24f }, Color{ 255, 214, 150, 255 });
        CuboBarra({ f.x, f.y - 0.21f, f.z }, { 0.3f, 0.06f, 0.3f }, COLOR_BRONCE_OSCURO_BARRA);

        if (k < 4)
        {
            float b = ANGULOS_FAROLES_BARRA[k + 1];
            Vector3 p0 = { std::cos(a) * 5.47f, -ALTO_CORNISA_BARRA - 0.22f, std::sin(a) * 5.47f };
            Vector3 p1 = { std::cos(b) * 5.47f, -ALTO_CORNISA_BARRA - 0.22f, std::sin(b) * 5.47f };
            CadenaBarra(p0, p1, 0.35f, 14);
        }
    }
}


// Losas talladas en coronas con juntas, medallon solar y runas. Todo a
// y = 0 (la cara superior del suelo de la logica) y dentro del radio real.
static void DibujarSueloPlataformaBarra()
{
    // Mortero bajo las losas.
    AnilloBarra({ 0.0f, -0.012f, 0.0f }, 0.0f, RADIO_ARENA_BARRA, COLOR_JUNTA_BARRA, 48);

    const int CORONAS = 5;
    const float radios[CORONAS + 1] = { 1.15f, 2.3f, 3.45f, 4.3f, 4.98f, RADIO_ARENA_BARRA };
    const int losas[CORONAS] = { 10, 16, 22, 26, 36 };
    const float junta = 0.05f;

    rlBegin(RL_QUADS);

    for (int c = 0; c < CORONAS; c++)
    {
        float r0 = radios[c] + junta * 0.5f;
        float r1 = radios[c + 1] - (c == CORONAS - 1 ? 0.0f : junta * 0.5f);
        float rm = (r0 + r1) * 0.5f;
        float paso = 6.2832f / (float)losas[c];
        float desfase = (c & 1) ? paso * 0.5f : 0.0f;
        float hueco = junta / rm;
        Color base = c == CORONAS - 1 ? COLOR_PIEDRA_CLARA_BARRA : COLOR_PIEDRA_BARRA;

        for (int k = 0; k < losas[c]; k++)
        {
            float a0 = desfase + paso * (float)k + hueco * 0.5f;
            float a1 = desfase + paso * (float)(k + 1) - hueco * 0.5f;
            float am = (a0 + a1) * 0.5f;
            float brillo = 0.9f + 0.18f * Hash01Barra(c * 97 + 5, k);
            // Luz del sol naciente: mas calida hacia el fondo a la derecha.
            brillo *= 1.0f + 0.04f * (std::cos(am) - std::sin(am)) * rm / RADIO_ARENA_BARRA;
            Color color = SombraBarra(base, brillo, true);

            const int SUB = 3;
            for (int s = 0; s < SUB; s++)
            {
                float b0 = a0 + (a1 - a0) * (float)s / (float)SUB;
                float b1 = a0 + (a1 - a0) * (float)(s + 1) / (float)SUB;
                QuadBarra(
                    { std::cos(b0) * r1, 0.0f, std::sin(b0) * r1 },
                    { std::cos(b0) * r0, 0.0f, std::sin(b0) * r0 },
                    { std::cos(b1) * r0, 0.0f, std::sin(b1) * r0 },
                    { std::cos(b1) * r1, 0.0f, std::sin(b1) * r1 },
                    color
                );
            }
        }
    }

    rlEnd();

    // Lineas de oro incrustadas: aro interior y aviso del borde.
    AnilloBarra({ 0.0f, 0.004f, 0.0f }, 2.28f, 2.33f, COLOR_ORO_BARRA, 48);
    AnilloBarra({ 0.0f, 0.004f, 0.0f }, 4.93f, 4.99f, COLOR_ORO_BARRA, 64);
    AnilloBarra({ 0.0f, 0.004f, 0.0f }, RADIO_ARENA_BARRA - 0.05f, RADIO_ARENA_BARRA, TonoBarra(COLOR_PIEDRA_CLARA_BARRA, 1.12f), 64);

    // Rayos de oro entre el medallon y el aro.
    for (int k = 0; k < 8; k++)
    {
        float a = 6.2832f * (float)k / 8.0f + 0.3927f;
        rlPushMatrix();
        rlRotatef(-a * RAD2DEG, 0.0f, 1.0f, 0.0f);
        CuboBarra({ 1.72f, 0.003f, 0.0f }, { 1.08f, 0.006f, 0.05f }, COLOR_ORO_BARRA);
        rlPopMatrix();
    }

    // Medallon solar: disco de bronce con rayos.
    AnilloBarra({ 0.0f, 0.003f, 0.0f }, 0.0f, 1.12f, TonoBarra(COLOR_BRONCE_BARRA, 0.85f), 32);
    AnilloBarra({ 0.0f, 0.005f, 0.0f }, 1.0f, 1.08f, COLOR_ORO_BARRA, 32);

    rlBegin(RL_TRIANGLES);
    for (int k = 0; k < 16; k++)
    {
        float a = 6.2832f * (float)k / 16.0f;
        float largo = k % 2 == 0 ? 0.98f : 0.8f;
        VerticeBarra({ std::cos(a - 0.12f) * 0.62f, 0.006f, std::sin(a - 0.12f) * 0.62f }, COLOR_ORO_BARRA);
        VerticeBarra({ std::cos(a + 0.12f) * 0.62f, 0.006f, std::sin(a + 0.12f) * 0.62f }, COLOR_ORO_BARRA);
        VerticeBarra({ std::cos(a) * largo, 0.006f, std::sin(a) * largo }, COLOR_ORO_BARRA);
    }
    rlEnd();

    // Runas talladas en la tercera corona (marcas hundidas con punto de oro).
    for (int k = 0; k < 12; k++)
    {
        float a = 6.2832f * ((float)k + 0.5f) / 12.0f;
        float r = 3.88f;
        rlPushMatrix();
        rlTranslatef(std::cos(a) * r, 0.0f, std::sin(a) * r);
        rlRotatef(-a * RAD2DEG + 45.0f, 0.0f, 1.0f, 0.0f);
        CuboBarra({ 0.0f, 0.002f, 0.0f }, { 0.34f, 0.004f, 0.34f }, TonoBarra(COLOR_PIEDRA_BARRA, 0.78f));
        CuboBarra({ 0.0f, 0.004f, 0.0f }, { 0.2f, 0.004f, 0.2f }, TonoBarra(COLOR_PIEDRA_BARRA, 1.05f));
        CuboBarra({ 0.0f, 0.006f, 0.0f }, { 0.07f, 0.004f, 0.07f }, COLOR_ORO_BARRA);
        rlPopMatrix();
    }

    // Liquen en algunas losas del borde.
    for (int k = 0; k < 18; k++)
    {
        float a = 6.2832f * Hash01Barra(k, 300);
        float r = 4.4f + 0.9f * Hash01Barra(k, 301);
        float tam = 0.1f + 0.14f * Hash01Barra(k, 302);
        CuboBarra({ std::cos(a) * r, 0.003f, std::sin(a) * r }, { tam, 0.004f, tam * 0.7f }, Color{ 150, 160, 120, 255 });
    }
}


//==================================================
// EJE Y BARRAS
//==================================================

// Altura de la cupula del eje (donde se apoya la varilla de la segunda).
static const float ALTO_EJE_BARRA = 1.18f;


// Barra de bronce con franjas de energia del color de acento. Su largo,
// altura y grosor siguen a la colision (LONGITUD_MEDIA_BARRA, altura,
// RADIO_COLISION_BARRA). MODELO FUTURO: brazo del mecanismo GLB.
static void DibujarBrazoBarra(float angulo, float altura, Color acento, float escalaAparicion)
{
    float largo = LONGITUD_MEDIA_BARRA * escalaAparicion;

    rlPushMatrix();
    rlTranslatef(0.0f, altura, 0.0f);
    rlRotatef(-angulo * RAD2DEG, 0.0f, 1.0f, 0.0f);

    CuboBarra({ 0.0f, 0.0f, 0.0f }, { largo * 2.0f - 0.3f, 0.26f, 0.3f }, COLOR_BRONCE_BARRA);
    CuboBarra({ 0.0f, 0.15f, 0.0f }, { largo * 2.0f - 0.6f, 0.05f, 0.16f }, COLOR_BRONCE_OSCURO_BARRA);

    for (int k = 0; k < 15; k++)
    {
        float x = (-largo + 0.6f) + (largo * 2.0f - 1.2f) * (float)k / 14.0f;
        if (std::fabs(x) < 0.6f) continue;
        CuboBarra({ x, 0.185f, 0.0f }, { 0.07f, 0.03f, 0.07f }, COLOR_BRONCE_CLARO_BARRA);
        CuboBarra({ x, 0.0f, 0.0f }, { 0.06f, 0.28f, 0.32f }, COLOR_BRONCE_OSCURO_BARRA);
    }

    // Franjas de energia a ambos costados (sin sombrear: emiten luz).
    rlBegin(RL_QUADS);
    for (int s = -1; s <= 1; s += 2)
    {
        float z = (float)s * 0.162f;
        QuadBarra({ -largo + 0.35f, -0.045f, z }, { largo - 0.35f, -0.045f, z }, { largo - 0.35f, 0.045f, z }, { -largo + 0.35f, 0.045f, z }, acento);
    }
    rlEnd();

    // Contrapesos de los extremos con gema.
    for (int s = -1; s <= 1; s += 2)
    {
        float x = (float)s * (largo - 0.2f);
        CuboBarra({ x, 0.0f, 0.0f }, { 0.4f, 0.42f, 0.44f }, COLOR_BRONCE_OSCURO_BARRA);
        CuboBarra({ x, 0.0f, 0.0f }, { 0.3f, 0.2f, 0.47f }, acento);
        CuboBarra({ x + (float)s * 0.22f, 0.0f, 0.0f }, { 0.06f, 0.3f, 0.3f }, COLOR_BRONCE_CLARO_BARRA);
    }

    rlPopMatrix();
}


// Eje del mecanismo: pedestal octogonal, fuste acanalado, corona dentada
// que gira con la barra y cupula con el orbe celeste.
static void DibujarEjeBarra(const MinijuegoBarraGiratoria& m)
{
    PrismaBarra({ 0.0f, 0.0f, 0.0f }, 0.8f, 0.74f, 0.1f, 8, COLOR_PIEDRA_CLARA_BARRA, 0.3927f);
    PrismaBarra({ 0.0f, 0.1f, 0.0f }, 0.6f, 0.56f, 0.1f, 8, COLOR_PIEDRA_BARRA, 0.3927f);
    PrismaBarra({ 0.0f, 0.2f, 0.0f }, 0.4f, 0.37f, 0.44f, 12, COLOR_PIEDRA_CLARA_BARRA, 0.0f, true);

    // Corona dentada a la altura de la barra.
    rlPushMatrix();
    rlTranslatef(0.0f, ALTURA_BARRA, 0.0f);
    rlRotatef(-m.anguloBarra * RAD2DEG, 0.0f, 1.0f, 0.0f);
    EngranajeBarra(0.56f, 0.36f, 14, COLOR_BRONCE_BARRA);
    rlPopMatrix();

    PrismaBarra({ 0.0f, 1.0f, 0.0f }, 0.4f, 0.44f, 0.08f, 12, COLOR_PIEDRA_CLARA_BARRA);
    PrismaBarra({ 0.0f, 1.08f, 0.0f }, 0.36f, 0.24f, 0.1f, 12, COLOR_BRONCE_BARRA);
    AnilloBarra({ 0.0f, ALTO_EJE_BARRA, 0.0f }, 0.14f, 0.24f, COLOR_ORO_BARRA, 12);

    // Engranajes pequenos a los costados del fuste (giran al reves).
    for (int s = -1; s <= 1; s += 2)
    {
        rlPushMatrix();
        rlTranslatef((float)s * 0.46f, 0.44f, 0.0f);
        rlRotatef(90.0f, 0.0f, 0.0f, 1.0f);
        rlRotatef(m.giroMecanismo * 160.0f * (float)s, 0.0f, 1.0f, 0.0f);
        EngranajeBarra(0.2f, 0.07f, 8, COLOR_BRONCE_CLARO_BARRA);
        rlPopMatrix();
    }

    float pulso = 1.0f + 0.08f * std::sin(m.tiempoAnimacion * 3.0f);
    (DrawSphere)({ 0.0f, ALTO_EJE_BARRA + 0.12f, 0.0f }, 0.14f * pulso, COLOR_ACENTO_BARRA_BASE);

    if (!m.segundaBarraAparecio)
    {
        return;
    }

    // Segunda barra: baja con su varilla y su propia corona.
    float h = m.alturaSegundaBarra;
    float abajo = h - 1.25f;
    if (abajo < ALTO_EJE_BARRA) abajo = ALTO_EJE_BARRA;

    PrismaBarra({ 0.0f, abajo, 0.0f }, 0.13f, 0.13f, h - abajo + 0.2f, 10, COLOR_BRONCE_OSCURO_BARRA);

    rlPushMatrix();
    rlTranslatef(0.0f, h, 0.0f);
    rlRotatef(-m.anguloSegundaBarra * RAD2DEG, 0.0f, 1.0f, 0.0f);
    EngranajeBarra(0.44f, 0.32f, 12, COLOR_BRONCE_BARRA);
    rlPopMatrix();

    PrismaBarra({ 0.0f, h + 0.18f, 0.0f }, 0.3f, 0.12f, 0.14f, 10, COLOR_BRONCE_BARRA);
    (DrawSphere)({ 0.0f, h + 0.42f, 0.0f }, 0.13f * pulso, COLOR_ACENTO_BARRA_SUPERIOR);
}


static void DibujarBarrasBarra(const MinijuegoBarraGiratoria& m)
{
    // Durante la cuenta, la barra se despliega desde el eje.
    float aparicion = 1.0f;

    if (m.fase == FASE_BARRA_PREPARACION)
    {
        float u = Limitar01Barra((DURACION_PREPARACION_BARRA - m.tiempoPreparacion) / 1.1f);
        float v = u - 1.0f;
        aparicion = 0.08f + 0.92f * (1.0f + 2.70158f * v * v * v + 1.70158f * v * v);
    }

    DibujarBrazoBarra(m.anguloBarra, ALTURA_BARRA, COLOR_ACENTO_BARRA_BASE, aparicion);

    if (m.segundaBarraAparecio)
    {
        DibujarBrazoBarra(m.anguloSegundaBarra, m.alturaSegundaBarra, COLOR_ACENTO_BARRA_SUPERIOR, 1.0f);
    }
}


//==================================================
// JUGADORES, PARTICULAS Y CAPAS TRANSLUCIDAS
//==================================================

// Anillo del color del jugador en la losa (queda en el suelo cuando salta).
static void DibujarAnilloJugadorBarra(const JugadorPrueba& jugador, Color color, float t)
{
    float distancia = MagnitudHorizontalBarra(jugador.posicion.x, jugador.posicion.z);
    if (jugador.cayendo || distancia > RADIO_ARENA_BARRA - 0.1f) return;

    float pies = jugador.posicion.y - jugador.tamano.y * 0.5f;
    float alto = pies > 0.0f ? pies : 0.0f;
    float escala = 1.0f - Limitar01Barra(alto / 4.0f) * 0.35f;
    float pulso = 1.0f + 0.04f * std::sin(t * 5.0f + jugador.posicion.x);

    Color relleno = color;
    relleno.a = 70;
    AnilloBarra({ jugador.posicion.x, 0.02f, jugador.posicion.z }, 0.0f, 0.5f * escala, relleno, 20);
    AnilloBarra({ jugador.posicion.x, 0.024f, jugador.posicion.z }, 0.5f * escala * pulso, 0.62f * escala * pulso, color, 24);
}


// Eliminados: siguen cayendo hacia las nubes dando vueltas.
static void DibujarCaidasBarra(
    const MinijuegoBarraGiratoria& m,
    const Participante participantes[]
)
{
    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        const EstadoJugadorBarraGiratoria& e = m.estadosJugadores[i];

        if (
            !m.resultado.participantes[i].participo ||
            e.tiempoCaida < 0.0f ||
            e.posicionCaida.y < NIVEL_NUBES_BARRA - 1.5f
        )
        {
            continue;
        }

        rlPushMatrix();
        rlTranslatef(e.posicionCaida.x, e.posicionCaida.y, e.posicionCaida.z);
        rlRotatef(e.tiempoCaida * 420.0f, e.velocidadCaida.z, 0.0f, -e.velocidadCaida.x);
        DibujarModeloJugadorEnPosicion(ObtenerPersonajeParticipante(participantes[i]), { 0.0f, -0.7f, 0.0f }, e.tiempoCaida * 90.0f, participantes[i].color);
        rlPopMatrix();
    }
}


static void DibujarParticulasBarra(const MinijuegoBarraGiratoria& m)
{
    for (int i = 0; i < MAX_PARTICULAS_BARRA; i++)
    {
        const ParticulaBarraGiratoria& p = m.particulasEfecto[i];

        if (!p.activa)
        {
            continue;
        }

        float vida = p.vidaMaxima > 0.0f ? p.vida / p.vidaMaxima : 0.0f;
        float tam = p.tamano * (0.5f + 0.5f * vida);
        CuboBarra(p.posicion, { tam, tam, tam }, p.color);
    }
}


// Sombra de una barra sobre las losas: mas difusa cuanto mas alta.
static void SombraBrazoBarra(float angulo, float altura, float alfa)
{
    float dx = std::cos(angulo);
    float dz = std::sin(angulo);
    float ancho = 0.2f + altura * 0.05f;
    float largo = LONGITUD_MEDIA_BARRA < RADIO_ARENA_BARRA ? LONGITUD_MEDIA_BARRA : RADIO_ARENA_BARRA - 0.05f;
    Color color = { 40, 26, 40, (unsigned char)(alfa * 255.0f) };

    rlBegin(RL_QUADS);
    QuadBarra(
        { -dx * largo - dz * ancho, 0.014f, -dz * largo + dx * ancho },
        { dx * largo - dz * ancho, 0.014f, dz * largo + dx * ancho },
        { dx * largo + dz * ancho, 0.014f, dz * largo - dx * ancho },
        { -dx * largo + dz * ancho, 0.014f, -dz * largo - dx * ancho },
        color
    );
    rlEnd();
}


static void DibujarTranslucidosBarra(const MinijuegoBarraGiratoria& m)
{
    float t = m.tiempoAnimacion;

    rlDrawRenderBatchActive();
    rlDisableDepthMask();

    // Sombras de las barras.
    SombraBrazoBarra(m.anguloBarra, ALTURA_BARRA, 0.28f);

    if (m.segundaBarraAparecio)
    {
        float cerca = 1.0f - Limitar01Barra((m.alturaSegundaBarra - ALTURA_FINAL_SEGUNDA_BARRA) / (ALTURA_INICIAL_SEGUNDA_BARRA - ALTURA_FINAL_SEGUNDA_BARRA));
        SombraBrazoBarra(m.anguloSegundaBarra, m.alturaSegundaBarra, 0.08f + 0.14f * cerca);

        // Aviso mientras baja: anillo magenta que late en el borde.
        if (!m.segundaBarraLista)
        {
            float pulso = 0.5f + 0.5f * std::sin(t * 14.0f);
            Color aviso = COLOR_ACENTO_BARRA_SUPERIOR;
            aviso.a = (unsigned char)(60.0f + 120.0f * pulso);
            AnilloBarra({ 0.0f, 0.03f, 0.0f }, LONGITUD_MEDIA_BARRA * 0.9f, LONGITUD_MEDIA_BARRA * 0.95f, aviso, 64);
        }
    }

    // Ondas.
    for (int i = 0; i < MAX_ONDAS_BARRA; i++)
    {
        const OndaBarraGiratoria& onda = m.ondas[i];
        if (!onda.activa) continue;

        float u = 1.0f - onda.vida / onda.vidaMaxima;
        float r = onda.radioMaximo * (0.15f + 0.85f * std::sqrt(u));
        Color c = onda.color;
        c.a = (unsigned char)(200.0f * (1.0f - u));
        AnilloBarra(onda.posicion, r * 0.86f, r, c, 40);
    }

    DibujarCascadasBarra(t);

    // Jirones de nube que cruzan a los costados, a la altura del templo.
    if (CalidadDecoracion() != CALIDAD_BAJA)
    {
        for (int k = 0; k < 6; k++)
        {
            float vel = 0.5f + 0.3f * Hash01Barra(k, 400);
            float x = std::fmod(Hash01Barra(k, 401) * 60.0f + t * vel, 60.0f) - 30.0f;
            if (std::fabs(x) < 7.5f) continue;
            float y = -2.5f - 3.0f * Hash01Barra(k, 402);
            float z = -9.0f + 12.0f * Hash01Barra(k, 403);
            float borde = Limitar01Barra((30.0f - std::fabs(x)) / 6.0f);
            Color c = COLOR_NUBE_LUZ_BARRA;
            c.a = (unsigned char)(110.0f * borde);

            rlPushMatrix();
            rlTranslatef(x, y, z);
            rlScalef(1.0f, 0.32f, 0.6f);
            DrawSphereEx({ 0.0f, 0.0f, 0.0f }, 2.2f + Hash01Barra(k, 404), 5, 9, c);
            rlPopMatrix();
        }
    }

    rlDrawRenderBatchActive();
    rlEnableDepthMask();
}


// Luces falsas aditivas: braseros, faroles, orbes, energia de las barras
// con su estela, destellos de golpe, rayos de sol, motas y farolillos.
static void DibujarLucesBarra(
    const MinijuegoBarraGiratoria& m,
    const JugadorPrueba jugadores[]
)
{
    if (CalidadEfectos() == CALIDAD_BAJA)
    {
        return;
    }

    float t = m.tiempoAnimacion;

    rlDrawRenderBatchActive();
    rlDisableDepthMask();
    BeginBlendMode(BLEND_ADDITIVE);

    // Braseros: halo en la llama y charco calido en el islote.
    for (int i = 0; i < 2; i++)
    {
        Vector3 b = PosicionBraseroBarra(i, t);
        float pulso = 0.85f + 0.15f * std::sin(t * 9.0f + (float)i * 2.0f);
        ResplandorFrenteBarra({ b.x, b.y + 1.65f, b.z + 0.2f }, 1.3f * pulso, Color{ 255, 170, 80, 120 });
        ResplandorFrenteBarra({ b.x, b.y + 1.6f, b.z + 0.2f }, 0.55f * pulso, Color{ 255, 220, 140, 150 });

        for (int r = 0; r < 3; r++)
        {
            ResplandorSueloBarra({ b.x, b.y + 0.06f + 0.002f * (float)r, b.z }, (1.6f + 0.6f * (float)r) * pulso, Color{ 255, 160, 80, 40 });
        }
    }

    // Faroles bajo la cornisa.
    for (int k = 0; k < 5; k++)
    {
        Vector3 f = PosicionFarolBarra(k, t);
        float pulso = 0.9f + 0.1f * std::sin(t * 6.0f + (float)k * 1.3f);
        ResplandorFrenteBarra({ f.x, f.y, f.z + 0.15f }, 0.75f * pulso, Color{ 255, 200, 120, 110 });
    }

    // Orbe del eje: halo y charco celeste sobre las losas.
    float pulsoOrbe = 0.85f + 0.15f * std::sin(t * 3.0f);
    ResplandorFrenteBarra({ 0.0f, ALTO_EJE_BARRA + 0.12f, 0.15f }, 0.7f * pulsoOrbe, Color{ 120, 240, 255, 130 });
    for (int r = 0; r < 3; r++)
    {
        ResplandorSueloBarra({ 0.0f, 0.03f + 0.002f * (float)r, 0.0f }, (1.6f + 0.7f * (float)r) * pulsoOrbe, Color{ 90, 220, 255, 26 });
    }

    // Cristal colgante bajo la plataforma.
    ResplandorFrenteBarra({ 0.0f, -7.25f, 0.4f }, 1.4f * pulsoOrbe, Color{ 110, 230, 255, 110 });

    // Energia de las barras: brillo a lo largo y estela barrida en la losa.
    for (int b = 0; b < 2; b++)
    {
        if (b == 1 && !m.segundaBarraAparecio) break;

        float angulo = b == 0 ? m.anguloBarra : m.anguloSegundaBarra;
        float altura = b == 0 ? ALTURA_BARRA : m.alturaSegundaBarra;
        float omega = b == 0 ? m.velocidadAngular : m.velocidadAngularSegunda;
        Color acento = b == 0 ? COLOR_ACENTO_BARRA_BASE : COLOR_ACENTO_BARRA_SUPERIOR;
        float largo = LONGITUD_MEDIA_BARRA;

        if (b == 0 && m.fase == FASE_BARRA_PREPARACION)
        {
            largo *= Limitar01Barra((DURACION_PREPARACION_BARRA - m.tiempoPreparacion) / 1.1f);
        }

        float dx = std::cos(angulo);
        float dz = std::sin(angulo);

        // Brillo alrededor del brazo.
        rlBegin(RL_QUADS);
        Color brillo = acento;
        brillo.a = 70;
        Color nada = acento;
        nada.a = 0;
        for (int s = -1; s <= 1; s += 2)
        {
            float ox = -dz * 0.42f * (float)s;
            float oz = dx * 0.42f * (float)s;
            VerticeBarra({ -dx * largo, altura, -dz * largo }, brillo);
            VerticeBarra({ dx * largo, altura, dz * largo }, brillo);
            VerticeBarra({ dx * largo + ox, altura, dz * largo + oz }, nada);
            VerticeBarra({ -dx * largo + ox, altura, -dz * largo + oz }, nada);
        }
        rlEnd();

        // Estela en la losa: abanico detras de cada mitad del brazo.
        if (m.fase == FASE_BARRA_JUGANDO)
        {
            float arco = Limitar01Barra(omega * 0.12f) * 0.6f + 0.08f;
            const int PASOS = 6;
            float yEstela = 0.02f + (float)b * 0.004f;
            unsigned char alfa = (unsigned char)(b == 0 ? 46 : 26);

            rlBegin(RL_QUADS);
            for (int mitad = 0; mitad < 2; mitad++)
            {
                float base = angulo + 3.1416f * (float)mitad;

                for (int p = 0; p < PASOS; p++)
                {
                    float a0 = base - arco * (float)p / (float)PASOS;
                    float a1 = base - arco * (float)(p + 1) / (float)PASOS;
                    Color c0 = acento;
                    Color c1 = acento;
                    c0.a = (unsigned char)((float)alfa * (1.0f - (float)p / (float)PASOS));
                    c1.a = (unsigned char)((float)alfa * (1.0f - (float)(p + 1) / (float)PASOS));
                    float rr = RADIO_ARENA_BARRA - 0.06f;
                    VerticeBarra({ std::cos(a0) * 0.8f, yEstela, std::sin(a0) * 0.8f }, c0);
                    VerticeBarra({ std::cos(a0) * rr, yEstela, std::sin(a0) * rr }, c0);
                    VerticeBarra({ std::cos(a1) * rr, yEstela, std::sin(a1) * rr }, c1);
                    VerticeBarra({ std::cos(a1) * 0.8f, yEstela, std::sin(a1) * 0.8f }, c1);
                }
            }
            rlEnd();
        }

        // Gemas de los extremos.
        for (int s = -1; s <= 1; s += 2)
        {
            Vector3 g = { dx * (largo - 0.2f) * (float)s, altura, dz * (largo - 0.2f) * (float)s };
            ResplandorFrenteBarra(g, 0.55f, Color{ acento.r, acento.g, acento.b, 110 });
        }
    }

    // Orbe de la segunda barra.
    if (m.segundaBarraAparecio)
    {
        ResplandorFrenteBarra({ 0.0f, m.alturaSegundaBarra + 0.42f, 0.15f }, 0.6f * pulsoOrbe, Color{ 255, 130, 220, 120 });
    }

    // Destello de golpe sobre cada jugador.
    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        const EstadoJugadorBarraGiratoria& e = m.estadosJugadores[i];
        if (!m.resultado.participantes[i].participo || e.eliminado || e.destelloGolpe <= 0.0f) continue;

        Vector3 p = jugadores[i].posicion;
        ResplandorFrenteBarra({ p.x, p.y + 0.2f, p.z + 0.3f }, 1.4f * e.destelloGolpe + 0.3f, Color{ 255, 240, 200, (unsigned char)(150.0f * e.destelloGolpe) });
    }

    // Rayos de sol que entran en diagonal desde el fondo a la derecha.
    for (int k = 0; k < 3; k++)
    {
        float x = -4.0f + 7.0f * (float)k;
        float vaiven = 0.5f + 0.5f * std::sin(t * 0.3f + (float)k * 2.0f);
        Color c = { 255, 210, 160, (unsigned char)(10.0f + 8.0f * vaiven) };
        Color c0 = c;
        c0.a = 0;

        rlBegin(RL_QUADS);
        VerticeBarra({ x + 16.0f, 14.0f, -30.0f }, c0);
        VerticeBarra({ x + 19.0f, 14.0f, -30.0f }, c0);
        VerticeBarra({ x + 1.5f, -6.0f, 4.0f }, c);
        VerticeBarra({ x - 1.5f, -6.0f, 4.0f }, c);
        rlEnd();
    }

    // Motas doradas que suben alrededor del templo.
    int motas = (int)(46.0f * FactorCalidadGrafica(CalidadParticulas()));

    for (int k = 0; k < motas; k++)
    {
        float a = 6.2832f * Hash01Barra(k, 500) + t * 0.05f;
        float r = 3.0f + 9.0f * Hash01Barra(k, 501);
        float ciclo = 9.0f + 5.0f * Hash01Barra(k, 502);
        float u = std::fmod(t / ciclo + Hash01Barra(k, 503), 1.0f);
        Vector3 p =
        {
            std::cos(a) * r + std::sin(t * 0.8f + (float)k) * 0.3f,
            -4.0f + 10.0f * u,
            std::sin(a) * r * 0.8f - 1.0f
        };
        float alfa = std::sin(u * 3.1416f);
        ResplandorFrenteBarra(p, 0.09f, Color{ 255, 226, 150, (unsigned char)(230.0f * alfa) });
    }

    // Farolillos de papel que suben a lo lejos.
    for (int k = 0; k < 7; k++)
    {
        float ciclo = 40.0f + 10.0f * Hash01Barra(k, 600);
        float u = std::fmod(t / ciclo + Hash01Barra(k, 601), 1.0f);
        Vector3 p =
        {
            -26.0f + 52.0f * Hash01Barra(k, 602) + std::sin(t * 0.3f + (float)k) * 0.8f,
            -7.0f + 16.0f * u,
            -18.0f - 22.0f * Hash01Barra(k, 603)
        };
        float alfa = std::sin(u * 3.1416f);
        ResplandorFrenteBarra(p, 0.9f, Color{ 255, 170, 90, (unsigned char)(90.0f * alfa) });
        ResplandorFrenteBarra(p, 0.28f, Color{ 255, 230, 170, (unsigned char)(220.0f * alfa) });
    }

    EndBlendMode();
    rlEnableDepthMask();
}


// Petalos rosados que cruzan la escena desde la isla de los cerezos.
static void DibujarPetalosBarra(float t)
{
    if (CalidadDecoracion() == CALIDAD_BAJA)
    {
        return;
    }

    int cantidad = (int)(36.0f * FactorCalidadGrafica(CalidadParticulas()));

    rlBegin(RL_QUADS);
    for (int k = 0; k < cantidad; k++)
    {
        float ciclo = 12.0f + 6.0f * Hash01Barra(k, 700);
        float u = std::fmod(t / ciclo + Hash01Barra(k, 701), 1.0f);
        Vector3 p =
        {
            14.0f - 32.0f * u + std::sin(t * 1.3f + (float)k) * 0.6f,
            6.0f - 9.0f * u + std::sin(t * 2.1f + (float)k * 0.7f) * 0.4f,
            -12.0f + 16.0f * Hash01Barra(k, 702)
        };
        float giro = t * (2.0f + Hash01Barra(k, 703) * 3.0f) + (float)k;
        float c = std::cos(giro) * 0.07f;
        float s = std::sin(giro) * 0.07f;
        Color color = TinteBarra(k % 2 == 0 ? Color{ 255, 196, 214, 255 } : Color{ 250, 226, 236, 255 }, p);

        QuadBarra(
            { p.x - c, p.y - s * 0.5f, p.z - s },
            { p.x + s, p.y + c * 0.5f, p.z - c },
            { p.x + c, p.y + s * 0.5f, p.z + s },
            { p.x - s, p.y - c * 0.5f, p.z + c },
            color
        );
    }
    rlEnd();
}


//==================================================
// DIBUJAR
//==================================================

void MinijuegoBarraGiratoria::Dibujar(
    const JugadorPrueba jugadores[],
    int cantidadMaxima,
    const Participante participantes[],
    const ParticulaTierra particulas[],
    int cantidadParticulas,
    bool mostrarDebug
) const
{
    (void)cantidadMaxima;


    const float t = tiempoAnimacion;
    int ancho = GetScreenWidth();
    int alto = GetScreenHeight();

    // Entrada: durante la cuenta la camara baja desde mas alto y lejos.
    Camera3D vista = camara;

    if (fase == FASE_BARRA_PREPARACION)
    {
        float u = Limitar01Barra(tiempoPreparacion / DURACION_PREPARACION_BARRA);
        float suave = u * u * (3.0f - 2.0f * u) * ObtenerFactorMovimientoCamaraMinijuegos();
        vista.position.y += 3.0f * suave;
        vista.position.z += 4.0f * suave;
        vista.target.y += 0.6f * suave;
    }

    ClearBackground(COLOR_NIEBLA_BARRA);
    DibujarCieloBarra(vista, t);

    BeginMode3D(vista);
    rlDisableBackfaceCulling();

    // Horizonte, mar de nubes y media distancia.
    DibujarHorizonteBarra(t);
    DibujarMarNubesBarra(t);
    DibujarIslasLejanasBarra(t);
    DibujarAvesBarra(t);

    // Anillo cercano y plataforma.
    if (CalidadDecoracion() != CALIDAD_BAJA)
    {
        DibujarIsletasBarra(t);
    }

    DibujarCuerpoPlataformaBarra(giroMecanismo, t);
    DibujarSueloPlataformaBarra();
    DibujarEjeBarra(*this);

    // Anillos de los jugadores antes que los modelos.
    rlDrawRenderBatchActive();
    rlDisableDepthMask();
    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!resultado.participantes[i].participo || estadosJugadores[i].eliminado) continue;
        DibujarAnilloJugadorBarra(jugadores[i], participantes[i].color, t);
    }
    rlDrawRenderBatchActive();
    rlEnableDepthMask();

    DibujarBarrasBarra(*this);

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!resultado.participantes[i].participo || estadosJugadores[i].eliminado)
        {
            continue;
        }

        // Sacudida corta al recibir la barra.
        const EstadoJugadorBarraGiratoria& e = estadosJugadores[i];
        if (e.destelloGolpe > 0.0f)
        {
            JugadorPrueba sacudido = jugadores[i];
            sacudido.posicion.x += std::sin(t * 70.0f) * 0.06f * e.destelloGolpe;
            DibujarJugadorModeloCompartido(sacudido, participantes[i]);
        }
        else
        {
            DibujarJugadorModeloCompartido(jugadores[i], participantes[i]);
        }

        if (mostrarDebug)
        {
            DrawBoundingBox(CrearHitboxJugadorPrueba(jugadores[i]), LIME);
        }
    }

    DibujarCaidasBarra(*this, participantes);
    DibujarParticulasTierra(particulas, cantidadParticulas);
    DibujarParticulasBarra(*this);
    DibujarPetalosBarra(t);

    DibujarTranslucidosBarra(*this);
    DibujarLucesBarra(*this, jugadores);

    rlEnableBackfaceCulling();
    EndMode3D();

    // Rotulos J1..J4 sobre los que siguen en la plataforma.
    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (
            !resultado.participantes[i].participo ||
            estadosJugadores[i].eliminado ||
            jugadores[i].cayendo ||
            !participantes[i].activo
        )
        {
            continue;
        }

        Vector3 cabeza = jugadores[i].posicion;
        cabeza.y += jugadores[i].tamano.y * 0.5f + 0.75f;
        Vector2 p = GetWorldToScreen(cabeza, vista);

        char rotulo[8];
        std::snprintf(rotulo, sizeof(rotulo), "J%d", participantes[i].numeroJugador);

        OpcionesEtiquetaUI op;
        op.borde = participantes[i].color;
        op.pico = true;
        DibujarEtiquetaUI(rotulo, p, 0.0f, op);
    }

    // «¡J# AFUERA!» donde cayo cada eliminado.
    for (int i = 0; i < MAX_AVISOS_BARRA; i++)
    {
        const AvisoBarraGiratoria& aviso = avisos[i];
        if (!aviso.activo) continue;

        float u = 1.0f - aviso.vida / DURACION_AVISO_BARRA;
        Vector3 p = { aviso.posicion.x, aviso.posicion.y + 0.6f + u * 1.2f, aviso.posicion.z };
        Vector2 s = GetWorldToScreen(p, vista);

        if (s.y < Px(150.0f)) s.y = Px(150.0f);
        if (s.y > Px(610.0f)) s.y = Px(610.0f);
        if (s.x < Px(150.0f)) s.x = Px(150.0f);
        if (s.x > (float)ancho - Px(150.0f)) s.x = (float)ancho - Px(150.0f);

        float alfa = aviso.vida < 0.45f ? aviso.vida / 0.45f : 1.0f;
        if (u < 0.08f) alfa *= u / 0.08f;

        char texto[32];
        std::snprintf(texto, sizeof(texto), "¡J%d AFUERA!", participantes[aviso.jugador].numeroJugador);
        DibujarTextoMundoContornoUI(texto, s.x, s.y, UI_TEXTO_GRANDE, participantes[aviso.jugador].color, alfa);
    }

    // Anuncios de la segunda barra y de la aceleracion.
    if (fase == FASE_BARRA_JUGANDO)
    {
        float yAnuncio = (float)alto * 0.18f;
        bool cayendoSegunda = segundaBarraAparecio && !segundaBarraLista;
        bool previa = !segundaBarraAparecio && tiempoJugado >= TIEMPO_APARICION_SEGUNDA_BARRA - 1.2f;

        if (previa || cayendoSegunda)
        {
            float pulso = 0.75f + 0.25f * std::sin(t * 12.0f);
            DibujarTextoMundoContornoUI("¡CUIDADO ARRIBA!", (float)ancho * 0.5f, yAnuncio, UI_TEXTO_GRANDE, Color{ 255, 176, 232, 255 }, pulso);
        }
        else if (tiempoAterrizajeSegunda >= 0.0f && tiempoAterrizajeSegunda < 2.4f)
        {
            float u = tiempoAterrizajeSegunda;
            float alfa = u < 0.15f ? u / 0.15f : (u > 1.9f ? (2.4f - u) / 0.5f : 1.0f);
            DibujarTextoMundoContornoUI("¡SEGUNDA BARRA!", (float)ancho * 0.5f, yAnuncio, UI_TEXTO_GRANDE, Color{ 255, 176, 232, 255 }, alfa);
            DibujarTextoMundoContornoUI("SOLO TE PEGA SI SALTÁS", (float)ancho * 0.5f, yAnuncio + Px(40.0f), UI_TEXTO, UI_TEXTO_MUNDO, alfa);
        }
        else if (tiempoJugado >= 20.0f && tiempoJugado < 51.5f && std::fmod(tiempoJugado, 10.0f) < 1.5f)
        {
            float u = std::fmod(tiempoJugado, 10.0f);
            float alfa = u < 0.15f ? u / 0.15f : (u > 1.1f ? (1.5f - u) / 0.4f : 1.0f);
            DibujarTextoMundoContornoUI("¡MÁS RÁPIDO!", (float)ancho * 0.5f, yAnuncio, UI_TEXTO_GRANDE, Color{ 150, 240, 255, 255 }, alfa);
        }
    }

    // HUD comun.
    DatosHudMinijuego hud;
    PrepararDatosHudMinijuego(
        hud,
        "BARRA GIRATORIA",
        "SALTÁ LA BARRA Y NO TE CAIGAS. A LOS 10 S BAJA OTRA QUE SOLO PEGA SI SALTÁS. GANA EL ÚLTIMO EN PIE.",
        resultado
    );

    hud.fase = fase == FASE_BARRA_PREPARACION
        ? HUD_MINIJUEGO_CUENTA
        : (fase == FASE_BARRA_TERMINADO ? HUD_MINIJUEGO_FIN : HUD_MINIJUEGO_JUGANDO);
    hud.cuentaRestante = tiempoPreparacion;

    // Sin limite de tiempo: el medidor muestra cuanto acelero la barra.
    hud.reloj = HUD_RELOJ_MEDIDOR;
    hud.etiquetaMedidor = "VELOCIDAD";
    int decimas = (int)std::lround(velocidadAngular / 0.9f * 10.0f);
    std::snprintf(hud.valorMedidor, sizeof(hud.valorMedidor), "x%d,%d", decimas / 10, decimas % 10);
    hud.progresoMedidor = Limitar01Barra((velocidadAngular - 0.9f) / 5.2f);

    int vivos = ContarVivosBarra(*this);

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        JugadorHudMinijuego& j = hud.jugadores[i];
        if (!j.visible) continue;

        const EstadoJugadorBarraGiratoria& estado = estadosJugadores[i];
        j.eliminado = estado.eliminado;

        int ms = estado.eliminado ? estado.tiempoSobrevividoMs : resultado.participantes[i].puntuacionMinijuego;

        if (estado.eliminado)
        {
            std::snprintf(j.valor, sizeof(j.valor), "%d,%d S", ms / 1000, (ms % 1000) / 100);
        }
        else if (fase == FASE_BARRA_JUGANDO && estado.tiempoStunBarra > 0.0f)
        {
            std::snprintf(j.estado, sizeof(j.estado), "¡GOLPEADO!");
        }

        if (estado.eliminado)
        {
            std::snprintf(j.detalleFinal, sizeof(j.detalleFinal), "CAYÓ A LOS %d,%d S", ms / 1000, (ms % 1000) / 100);
        }
        else
        {
            std::snprintf(j.detalleFinal, sizeof(j.detalleFinal), "EN PIE");
        }

        if (!participantes[i].esBot)
        {
            std::snprintf(
                j.controles,
                sizeof(j.controles),
                "MOVÉ: %s  SALTÁ: %s\nGOLPE: %s",
                TextoMoverHud(participantes[i]),
                ObtenerTextoBotonPrincipal(participantes[i]),
                TextoBotonGolpeHud(participantes[i])
            );
        }
    }

    if (fase == FASE_BARRA_TERMINADO && resultado.estado == RESULTADO_MINIJUEGO_FINALIZADO)
    {
        if (vivos == 0)
        {
            hud.tituloFinal = "¡NADIE QUEDÓ ARRIBA!";
            hud.notaFinal = "CAYERON A LA VEZ: COMPARTEN PUESTO";
        }
        else if (vivos > 1)
        {
            hud.notaFinal = "SIGUEN EN PIE AL CERRAR EL TEMPLO: COMPARTEN PUESTO";
        }
    }

    DibujarHudMinijuego(hud, participantes);
}


const ResultadoMinijuego&
MinijuegoBarraGiratoria::ObtenerResultado() const
{
    return resultado;
}
