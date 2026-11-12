#include "Minigames/MinijuegoPisotonPlagas.h"

#include "Minigames/AudioMinijuegos.h"
#include "Minigames/EfectosVisualesMinijuegos.h"
#include "Minigames/HorneadoMallas.h"
#include "Minigames/HudMinijuego.h"
#include "Minigames/MecanicasJugador.h"
#include "Minigames/ModeloJugadorCompartido.h"
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

static const float DURACION_PREPARACION_PISOTON = 3.0f;
static const float DURACION_PARTIDA_PISOTON = 45.0f;

// Arena rectangular (el suelo fisico es un bloque; los limites se aplican a mano).
static const float LIMITE_X_PISOTON = 9.0f;
static const float LIMITE_Z_PISOTON = 5.6f;
static const float SUELO_PISOTON = -0.05f;

// Plagas.
static const float RADIO_POUND_PISOTON = 1.25f;
static const float RADIO_PISADA_PISOTON = 0.85f;
static const float ALTURA_PLAGA_PISOTON = 0.5f;
static const float DURACION_ATURDIMIENTO_PISOTON = 2.4f;
static const float DURACION_AVISO_MADRIGUERA_PISOTON = 1.25f;
static const float SEPARACION_SEGMENTOS_PISOTON = 0.55f;
static const int MAXIMO_PLAGAS_VIVAS_PISOTON = 9;

static const int PUNTOS_ESCARABAJO_PISOTON = 1;
static const int PUNTOS_ORUGA_SEGMENTO_PISOTON = 2;
static const int PUNTOS_BABOSA_PISOTON = 5;

// Avispas.
static const float RADIO_AVISPA_PISOTON = 0.7f;
static const int PENALIZACION_AVISPA_PISOTON = 2;
static const float RALENTIZACION_AVISPA_PISOTON = 2.5f;
static const float RADIO_ESPANTO_PISOTON = 1.7f;

// Flor gigante.
static const float INTERVALO_FLOR_PISOTON = 15.0f;
static const float PRIMERA_FLOR_PISOTON = 12.0f;
static const float RADIO_FLOR_PISOTON = 1.7f;
static const int PUNTOS_FLOR_PISOTON = 3;
static const float AVISO_FLOR_PISOTON = 6.0f;

// Posiciones de las madrigueras.
static const float MADRIGUERAS_X_PISOTON[MAX_MADRIGUERAS_PISOTON] = { -7.2f, 0.0f, 7.2f, -7.2f, 0.0f, 7.2f };
static const float MADRIGUERAS_Z_PISOTON[MAX_MADRIGUERAS_PISOTON] = { -4.3f, -4.7f, -4.3f, 4.3f, 4.7f, 4.3f };


//==================================================
// UTILIDADES
//==================================================

static float LimitarPisoton(float valor, float minimo, float maximo)
{
    if (valor < minimo) return minimo;
    if (valor > maximo) return maximo;
    return valor;
}


static float AleatorioPisoton(float minimo, float maximo)
{
    return minimo + (float)GetRandomValue(0, 1000) / 1000.0f * (maximo - minimo);
}


static int LimitePisoton(int cantidadMaxima)
{
    return cantidadMaxima < MAX_PARTICIPANTES
        ? cantidadMaxima
        : MAX_PARTICIPANTES;
}


static float DistanciaPisoton(float ax, float az, float bx, float bz)
{
    float dx = ax - bx;
    float dz = az - bz;
    return std::sqrt(dx * dx + dz * dz);
}


static void AgregarParticulaPisoton(
    MinijuegoPisotonPlagas& m,
    Vector3 posicion,
    Vector3 velocidad,
    float vida,
    float tamano,
    Color color
)
{
    for (int i = 0; i < MAX_PARTICULAS_PISOTON; i++)
    {
        if (m.particulas[i].activa)
        {
            continue;
        }

        m.particulas[i].activa = true;
        m.particulas[i].posicion = posicion;
        m.particulas[i].velocidad = velocidad;
        m.particulas[i].vida = vida;
        m.particulas[i].vidaMaxima = vida;
        m.particulas[i].tamano = tamano;
        m.particulas[i].color = color;
        return;
    }
}


static void ExplosionPisoton(
    MinijuegoPisotonPlagas& m,
    float x,
    float z,
    Color color,
    int cantidad
)
{
    for (int k = 0; k < cantidad; k++)
    {
        float angulo = AleatorioPisoton(0.0f, 2.0f * PI);
        float fuerza = AleatorioPisoton(1.0f, 3.2f);

        AgregarParticulaPisoton(
            m,
            { x, 0.15f, z },
            { std::cos(angulo) * fuerza, AleatorioPisoton(1.5f, 3.5f), std::sin(angulo) * fuerza },
            AleatorioPisoton(0.35f, 0.7f),
            AleatorioPisoton(0.07f, 0.13f),
            color
        );
    }
}


static void CrearPopupPisoton(
    MinijuegoPisotonPlagas& m,
    float x,
    float z,
    int valor,
    Color color
)
{
    for (int i = 0; i < MAX_POPUPS_PISOTON; i++)
    {
        if (m.popups[i].activo)
        {
            continue;
        }

        m.popups[i].activo = true;
        m.popups[i].posicion = { x, 0.6f, z };
        m.popups[i].valor = valor;
        m.popups[i].vida = 1.1f;
        m.popups[i].color = color;
        return;
    }
}


static void ActualizarPopupsPisoton(MinijuegoPisotonPlagas& m, float deltaTime)
{
    for (int i = 0; i < MAX_POPUPS_PISOTON; i++)
    {
        if (!m.popups[i].activo)
        {
            continue;
        }

        m.popups[i].vida -= deltaTime;
        m.popups[i].posicion.y += deltaTime * 1.4f;

        if (m.popups[i].vida <= 0.0f)
        {
            m.popups[i].activo = false;
        }
    }
}


//==================================================
// LOGICA: PLAGAS
//==================================================

static void PosicionPlagaPisoton(const PlagaPisoton& plaga, float& x, float& z)
{
    float sumaX = 0.0f;
    float sumaZ = 0.0f;
    int cantidad = 0;

    for (int s = 0; s < MAX_SEGMENTOS_PISOTON; s++)
    {
        if (plaga.segVivo[s])
        {
            sumaX += plaga.segX[s];
            sumaZ += plaga.segZ[s];
            cantidad++;
        }
    }

    if (cantidad == 0)
    {
        x = plaga.segX[0];
        z = plaga.segZ[0];
        return;
    }

    x = sumaX / (float)cantidad;
    z = sumaZ / (float)cantidad;
}


static int ValorPlagaPisoton(const PlagaPisoton& plaga)
{
    if (plaga.tipo == PLAGA_PISOTON_BABOSA)
    {
        return PUNTOS_BABOSA_PISOTON;
    }

    if (plaga.tipo == PLAGA_PISOTON_ORUGA)
    {
        int vivos = 0;

        for (int s = 0; s < MAX_SEGMENTOS_PISOTON; s++)
        {
            if (plaga.segVivo[s])
            {
                vivos++;
            }
        }

        return vivos * PUNTOS_ORUGA_SEGMENTO_PISOTON;
    }

    return PUNTOS_ESCARABAJO_PISOTON;
}


static float VelocidadPlagaPisoton(TipoPlagaPisoton tipo)
{
    if (tipo == PLAGA_PISOTON_BABOSA) return 1.1f;
    if (tipo == PLAGA_PISOTON_ORUGA) return 1.5f;
    return 2.1f;
}


static int ContarPlagasVivasPisoton(const MinijuegoPisotonPlagas& m)
{
    int cantidad = 0;

    for (int p = 0; p < MAX_PLAGAS_PISOTON; p++)
    {
        if (m.plagas[p].activa && !m.plagas[p].escapando)
        {
            cantidad++;
        }
    }

    return cantidad;
}


static void ElegirDestinoPlagaPisoton(PlagaPisoton& plaga)
{
    plaga.destinoX = AleatorioPisoton(-LIMITE_X_PISOTON + 0.8f, LIMITE_X_PISOTON - 0.8f);
    plaga.destinoZ = AleatorioPisoton(-LIMITE_Z_PISOTON + 0.6f, LIMITE_Z_PISOTON - 0.6f);
    plaga.cambioRumbo = AleatorioPisoton(1.5f, 3.2f);
}


static void CrearPlagaPisoton(MinijuegoPisotonPlagas& m, float x, float z, TipoPlagaPisoton tipo)
{
    for (int p = 0; p < MAX_PLAGAS_PISOTON; p++)
    {
        if (m.plagas[p].activa)
        {
            continue;
        }

        PlagaPisoton& plaga = m.plagas[p];
        plaga = {};
        plaga.activa = true;
        plaga.tipo = tipo;
        plaga.id = m.siguienteId++;
        plaga.vidaMaxima = tipo == PLAGA_PISOTON_BABOSA ? 11.0f : AleatorioPisoton(12.0f, 16.0f);

        int segmentos = tipo == PLAGA_PISOTON_ORUGA ? MAX_SEGMENTOS_PISOTON : 1;

        for (int s = 0; s < segmentos; s++)
        {
            plaga.segX[s] = x;
            plaga.segZ[s] = z;
            plaga.segVivo[s] = true;
        }

        ElegirDestinoPlagaPisoton(plaga);

        float dx = plaga.destinoX - x;
        float dz = plaga.destinoZ - z;
        float longitud = std::sqrt(dx * dx + dz * dz);

        if (longitud > 0.001f)
        {
            plaga.dirX = dx / longitud;
            plaga.dirZ = dz / longitud;
        }

        ExplosionPisoton(m, x, z, Color{ 120, 84, 48, 255 }, 8);
        return;
    }
}


static void IniciarAvisoMadriguera(MinijuegoPisotonPlagas& m, float transcurrido)
{
    int libres[MAX_MADRIGUERAS_PISOTON]{};
    int cantidadLibres = 0;

    for (int k = 0; k < MAX_MADRIGUERAS_PISOTON; k++)
    {
        if (m.madrigueras[k].aviso <= 0.0f)
        {
            libres[cantidadLibres++] = k;
        }
    }

    if (cantidadLibres == 0)
    {
        return;
    }

    MadriguerasPisoton& madriguera = m.madrigueras[libres[GetRandomValue(0, cantidadLibres - 1)]];

    // Siempre hay doradas: una garantizada hacia la mitad y otra al final.
    bool forzarDorada =
        (m.doradasGeneradas == 0 && transcurrido > 14.0f) ||
        (m.doradasGeneradas == 1 && transcurrido > 30.0f);

    int dado = GetRandomValue(0, 99);

    if (forzarDorada || dado < 12)
    {
        madriguera.tipoPendiente = PLAGA_PISOTON_BABOSA;
        m.doradasGeneradas++;
    }
    else if (dado < 52)
    {
        madriguera.tipoPendiente = PLAGA_PISOTON_ESCARABAJO;
    }
    else
    {
        madriguera.tipoPendiente = PLAGA_PISOTON_ORUGA;
    }

    madriguera.aviso = DURACION_AVISO_MADRIGUERA_PISOTON;
}


static void ActualizarMadriguerasPisoton(
    MinijuegoPisotonPlagas& m,
    float deltaTime,
    float transcurrido
)
{
    m.proximaAparicion -= deltaTime;

    if (m.proximaAparicion <= 0.0f)
    {
        if (ContarPlagasVivasPisoton(m) < MAXIMO_PLAGAS_VIVAS_PISOTON)
        {
            IniciarAvisoMadriguera(m, transcurrido);
        }

        // El ritmo sube a medida que avanza la ronda.
        float ritmo = 1.5f - 0.6f * LimitarPisoton(transcurrido / DURACION_PARTIDA_PISOTON, 0.0f, 1.0f);
        m.proximaAparicion = AleatorioPisoton(ritmo * 0.7f, ritmo * 1.2f);
    }

    for (int k = 0; k < MAX_MADRIGUERAS_PISOTON; k++)
    {
        MadriguerasPisoton& madriguera = m.madrigueras[k];

        if (madriguera.aviso <= 0.0f)
        {
            continue;
        }

        madriguera.aviso -= deltaTime;

        // La tierra se mueve: polvo que salta de vez en cuando.
        if (GetRandomValue(0, 100) < 22)
        {
            AgregarParticulaPisoton(
                m,
                { madriguera.x + AleatorioPisoton(-0.35f, 0.35f), 0.1f, madriguera.z + AleatorioPisoton(-0.35f, 0.35f) },
                { AleatorioPisoton(-0.4f, 0.4f), AleatorioPisoton(1.0f, 2.0f), AleatorioPisoton(-0.4f, 0.4f) },
                0.4f,
                0.08f,
                Color{ 130, 92, 56, 255 }
            );
        }

        if (madriguera.aviso <= 0.0f)
        {
            madriguera.aviso = 0.0f;
            CrearPlagaPisoton(m, madriguera.x, madriguera.z, madriguera.tipoPendiente);
        }
    }
}


static void ActualizarPlagasPisoton(MinijuegoPisotonPlagas& m, float deltaTime)
{
    for (int p = 0; p < MAX_PLAGAS_PISOTON; p++)
    {
        PlagaPisoton& plaga = m.plagas[p];

        if (!plaga.activa)
        {
            continue;
        }

        plaga.edad += deltaTime;

        if (plaga.aparicion < 1.0f)
        {
            plaga.aparicion = LimitarPisoton(plaga.aparicion + deltaTime * 2.0f, 0.0f, 1.0f);
        }

        if (plaga.proteccion > 0.0f)
        {
            plaga.proteccion -= deltaTime;
        }

        if (plaga.escapando)
        {
            plaga.escala -= deltaTime * 2.0f;

            if (plaga.escala <= 0.0f)
            {
                plaga.activa = false;
            }

            continue;
        }

        if (plaga.edad > plaga.vidaMaxima)
        {
            plaga.escapando = true;
            continue;
        }

        if (plaga.aturdida > 0.0f)
        {
            plaga.aturdida -= deltaTime;
            continue;
        }

        if (plaga.aparicion < 0.5f)
        {
            continue;
        }

        // Cabeza: primer segmento vivo.
        int cabeza = 0;

        while (cabeza < MAX_SEGMENTOS_PISOTON - 1 && !plaga.segVivo[cabeza])
        {
            cabeza++;
        }

        plaga.cambioRumbo -= deltaTime;

        if (
            plaga.cambioRumbo <= 0.0f ||
            DistanciaPisoton(plaga.segX[cabeza], plaga.segZ[cabeza], plaga.destinoX, plaga.destinoZ) < 0.4f
        )
        {
            ElegirDestinoPlagaPisoton(plaga);
        }

        float dx = plaga.destinoX - plaga.segX[cabeza];
        float dz = plaga.destinoZ - plaga.segZ[cabeza];
        float longitud = std::sqrt(dx * dx + dz * dz);

        if (longitud > 0.001f)
        {
            // Giro suave para que el rumbo no cambie de golpe.
            float giro = LimitarPisoton(deltaTime * 4.0f, 0.0f, 1.0f);
            plaga.dirX += (dx / longitud - plaga.dirX) * giro;
            plaga.dirZ += (dz / longitud - plaga.dirZ) * giro;

            float norma = std::sqrt(plaga.dirX * plaga.dirX + plaga.dirZ * plaga.dirZ);

            if (norma > 0.001f)
            {
                plaga.dirX /= norma;
                plaga.dirZ /= norma;
            }
        }

        float velocidad = VelocidadPlagaPisoton(plaga.tipo);
        plaga.segX[cabeza] = LimitarPisoton(
            plaga.segX[cabeza] + plaga.dirX * velocidad * deltaTime,
            -LIMITE_X_PISOTON, LIMITE_X_PISOTON
        );
        plaga.segZ[cabeza] = LimitarPisoton(
            plaga.segZ[cabeza] + plaga.dirZ * velocidad * deltaTime,
            -LIMITE_Z_PISOTON, LIMITE_Z_PISOTON
        );

        // El resto de segmentos vivos sigue al anterior vivo.
        int anterior = cabeza;

        for (int s = cabeza + 1; s < MAX_SEGMENTOS_PISOTON; s++)
        {
            if (!plaga.segVivo[s])
            {
                continue;
            }

            float sx = plaga.segX[s] - plaga.segX[anterior];
            float sz = plaga.segZ[s] - plaga.segZ[anterior];
            float distancia = std::sqrt(sx * sx + sz * sz);

            if (distancia > SEPARACION_SEGMENTOS_PISOTON && distancia > 0.001f)
            {
                float exceso = distancia - SEPARACION_SEGMENTOS_PISOTON;
                plaga.segX[s] -= sx / distancia * exceso;
                plaga.segZ[s] -= sz / distancia * exceso;
            }

            anterior = s;
        }
    }
}


static void SumarPuntosPisoton(
    MinijuegoPisotonPlagas& m,
    int jugador,
    int valor,
    float x,
    float z,
    Color color
)
{
    m.puntos[jugador] += valor;
    CrearPopupPisoton(m, x, z, valor, color);
}


// Aplasta una plaga entera (escarabajo/babosa) o los segmentos de oruga que
// queden dentro del radio. Devuelve true si algo murio.
static bool AplastarPlagaPisoton(
    MinijuegoPisotonPlagas& m,
    PlagaPisoton& plaga,
    int jugador,
    float x,
    float z,
    float radio
)
{
    bool murioAlgo = false;
    int segmentosHit = 0;
    float impactoX = 0.0f;
    float impactoZ = 0.0f;

    for (int s = 0; s < MAX_SEGMENTOS_PISOTON; s++)
    {
        if (!plaga.segVivo[s] || DistanciaPisoton(plaga.segX[s], plaga.segZ[s], x, z) > radio)
        {
            continue;
        }

        plaga.segVivo[s] = false;
        segmentosHit++;
        murioAlgo = true;
        impactoX = plaga.segX[s];
        impactoZ = plaga.segZ[s];

        Color sangre = Color{ 150, 200, 60, 255 };

        if (plaga.tipo == PLAGA_PISOTON_ESCARABAJO) sangre = Color{ 190, 50, 40, 255 };
        if (plaga.tipo == PLAGA_PISOTON_BABOSA) sangre = Color{ 255, 220, 70, 255 };

        ExplosionPisoton(m, plaga.segX[s], plaga.segZ[s], sangre, 10);
    }

    if (!murioAlgo)
    {
        return false;
    }

    int valor = 0;

    if (plaga.tipo == PLAGA_PISOTON_BABOSA)
    {
        valor = PUNTOS_BABOSA_PISOTON;
        m.doradas[jugador]++;
    }
    else if (plaga.tipo == PLAGA_PISOTON_ORUGA)
    {
        valor = PUNTOS_ORUGA_SEGMENTO_PISOTON * segmentosHit;
    }
    else
    {
        valor = PUNTOS_ESCARABAJO_PISOTON;
    }

    m.aplastadas[jugador]++;
    SumarPuntosPisoton(
        m, jugador, valor, impactoX, impactoZ,
        plaga.tipo == PLAGA_PISOTON_BABOSA ? GOLD : RAYWHITE
    );

    bool quedan = false;

    for (int s = 0; s < MAX_SEGMENTOS_PISOTON; s++)
    {
        if (plaga.segVivo[s])
        {
            quedan = true;
        }
    }

    if (!quedan)
    {
        plaga.activa = false;
    }

    ReproducirSonidoMinijuego(
        m.audio,
        plaga.tipo == PLAGA_PISOTON_BABOSA
            ? SONIDO_RECOGER_NUCLEO_ESPECIAL
            : SONIDO_RECOGER_OBJETO
    );

    return true;
}


// Debe llamarse ANTES de la resolucion compartida de ground pound, que
// consume impactoGolpeSuelo.
static void ResolverPisotonesPisoton(
    MinijuegoPisotonPlagas& m,
    JugadorPrueba jugadores[],
    int limite
)
{
    for (int i = 0; i < limite; i++)
    {
        if (!m.resultado.participantes[i].participo)
        {
            continue;
        }

        JugadorPrueba& jugador = jugadores[i];

        if (jugador.cayendo || jugador.aplastado)
        {
            continue;
        }

        if (jugador.impactoGolpeSuelo)
        {
            for (int p = 0; p < MAX_PLAGAS_PISOTON; p++)
            {
                PlagaPisoton& plaga = m.plagas[p];

                if (!plaga.activa || plaga.escapando || plaga.aparicion < 0.5f)
                {
                    continue;
                }

                AplastarPlagaPisoton(m, plaga, i, jugador.posicion.x, jugador.posicion.z, RADIO_POUND_PISOTON);
            }

            continue;
        }

        bool descendiendo =
            !jugador.enSuelo &&
            jugador.velocidad.y < 0.0f &&
            !jugador.golpeSueloActivo &&
            !jugador.preparandoGolpeSuelo;

        if (!descendiendo)
        {
            continue;
        }

        float pies = jugador.posicion.y - jugador.tamano.y * 0.5f;

        if (pies > SUELO_PISOTON + ALTURA_PLAGA_PISOTON + 0.15f)
        {
            continue;
        }

        for (int p = 0; p < MAX_PLAGAS_PISOTON; p++)
        {
            PlagaPisoton& plaga = m.plagas[p];

            if (!plaga.activa || plaga.escapando || plaga.aparicion < 0.5f || plaga.proteccion > 0.0f)
            {
                continue;
            }

            bool pisada = false;

            for (int s = 0; s < MAX_SEGMENTOS_PISOTON; s++)
            {
                if (
                    plaga.segVivo[s] &&
                    DistanciaPisoton(plaga.segX[s], plaga.segZ[s], jugador.posicion.x, jugador.posicion.z) < RADIO_PISADA_PISOTON
                )
                {
                    pisada = true;
                }
            }

            if (!pisada)
            {
                continue;
            }

            // Salto normal: solo aturde y rebota al jugador.
            plaga.aturdida = DURACION_ATURDIMIENTO_PISOTON;
            plaga.proteccion = 0.45f;
            jugador.velocidad.y = 6.0f;
            jugador.enSuelo = false;

            float px = 0.0f;
            float pz = 0.0f;
            PosicionPlagaPisoton(plaga, px, pz);
            ExplosionPisoton(m, px, pz, Color{ 255, 230, 90, 255 }, 5);
            ReproducirSonidoMinijuego(m.audio, SONIDO_GOLPE);
            break;
        }
    }
}


//==================================================
// LOGICA: AVISPAS
//==================================================

static void ReiniciarAvispaPisoton(AvispaPisoton& avispa)
{
    // Entra por un borde aleatorio.
    bool lateral = GetRandomValue(0, 1) == 0;

    if (lateral)
    {
        avispa.x = GetRandomValue(0, 1) == 0 ? -LIMITE_X_PISOTON - 1.5f : LIMITE_X_PISOTON + 1.5f;
        avispa.z = AleatorioPisoton(-LIMITE_Z_PISOTON, LIMITE_Z_PISOTON);
    }
    else
    {
        avispa.x = AleatorioPisoton(-LIMITE_X_PISOTON, LIMITE_X_PISOTON);
        avispa.z = -LIMITE_Z_PISOTON - 1.5f;
    }

    avispa.dirX = avispa.x > 0.0f ? -1.0f : 1.0f;
    avispa.dirZ = 0.0f;
    avispa.huida = 0.0f;
    avispa.reposo = 0.0f;
    avispa.objetivo = -1;
    avispa.tiempoObjetivo = 0.0f;
}


static void ActualizarAvispasPisoton(
    MinijuegoPisotonPlagas& m,
    JugadorPrueba jugadores[],
    const Participante participantes[],
    int limite,
    float deltaTime
)
{
    for (int i = 0; i < limite; i++)
    {
        if (m.inmunidadAvispa[i] > 0.0f)
        {
            m.inmunidadAvispa[i] -= deltaTime;
        }
    }

    for (int a = 0; a < MAX_AVISPAS_PISOTON; a++)
    {
        AvispaPisoton& avispa = m.avispas[a];

        if (!avispa.activa)
        {
            avispa.retrasoAparicion -= deltaTime;

            if (avispa.retrasoAparicion <= 0.0f)
            {
                avispa.activa = true;
                ReiniciarAvispaPisoton(avispa);
            }

            continue;
        }

        avispa.fase += deltaTime;
        avispa.tiempoObjetivo -= deltaTime;

        if (avispa.tiempoObjetivo <= 0.0f)
        {
            int candidatos[MAX_PARTICIPANTES]{};
            int cantidad = 0;

            for (int i = 0; i < limite; i++)
            {
                if (m.resultado.participantes[i].participo)
                {
                    candidatos[cantidad++] = i;
                }
            }

            avispa.objetivo = cantidad > 0 ? candidatos[GetRandomValue(0, cantidad - 1)] : -1;
            avispa.tiempoObjetivo = AleatorioPisoton(2.0f, 3.5f);
        }

        float velocidad = 2.5f;
        float deseadoX = avispa.dirX;
        float deseadoZ = avispa.dirZ;

        if (avispa.huida > 0.0f)
        {
            avispa.huida -= deltaTime;
            velocidad = 6.0f;
        }
        else if (avispa.reposo > 0.0f)
        {
            avispa.reposo -= deltaTime;
            velocidad = 3.0f;
        }
        else if (avispa.objetivo >= 0)
        {
            deseadoX = jugadores[avispa.objetivo].posicion.x - avispa.x;
            deseadoZ = jugadores[avispa.objetivo].posicion.z - avispa.z;
            float norma = std::sqrt(deseadoX * deseadoX + deseadoZ * deseadoZ);

            if (norma > 0.001f)
            {
                deseadoX /= norma;
                deseadoZ /= norma;
            }

            // Zigzag para que no sea un misil.
            deseadoX += std::sin(avispa.fase * 3.1f) * 0.45f;
            deseadoZ += std::cos(avispa.fase * 2.7f) * 0.45f;
        }

        float giro = LimitarPisoton(deltaTime * (avispa.huida > 0.0f ? 8.0f : 2.2f), 0.0f, 1.0f);
        avispa.dirX += (deseadoX - avispa.dirX) * giro;
        avispa.dirZ += (deseadoZ - avispa.dirZ) * giro;

        float norma = std::sqrt(avispa.dirX * avispa.dirX + avispa.dirZ * avispa.dirZ);

        if (norma > 0.001f)
        {
            avispa.dirX /= norma;
            avispa.dirZ /= norma;
        }

        avispa.x = LimitarPisoton(avispa.x + avispa.dirX * velocidad * deltaTime, -LIMITE_X_PISOTON - 2.5f, LIMITE_X_PISOTON + 2.5f);
        avispa.z = LimitarPisoton(avispa.z + avispa.dirZ * velocidad * deltaTime, -LIMITE_Z_PISOTON - 2.5f, LIMITE_Z_PISOTON + 2.5f);
        avispa.altura = 0.95f + std::sin(avispa.fase * 6.0f) * 0.12f;

        for (int i = 0; i < limite; i++)
        {
            if (!m.resultado.participantes[i].participo)
            {
                continue;
            }

            JugadorPrueba& jugador = jugadores[i];

            if (jugador.cayendo)
            {
                continue;
            }

            float distancia = DistanciaPisoton(avispa.x, avispa.z, jugador.posicion.x, jugador.posicion.z);
            float altoRelativo = std::fabs(avispa.altura - jugador.posicion.y);

            // Un golpe cuerpo a cuerpo la espanta.
            if (
                jugador.golpeando &&
                avispa.huida <= 0.0f &&
                distancia < RADIO_ESPANTO_PISOTON &&
                altoRelativo < 1.6f
            )
            {
                float dx = avispa.x - jugador.posicion.x;
                float dz = avispa.z - jugador.posicion.z;
                float frente = distancia > 0.001f
                    ? (dx * jugador.direccionMirada.x + dz * jugador.direccionMirada.z) / distancia
                    : 1.0f;

                if (frente > 0.0f)
                {
                    avispa.huida = 2.5f;
                    avispa.dirX = distancia > 0.001f ? dx / distancia : 1.0f;
                    avispa.dirZ = distancia > 0.001f ? dz / distancia : 0.0f;
                    ExplosionPisoton(m, avispa.x, avispa.z, Color{ 255, 230, 80, 255 }, 6);
                    ReproducirSonidoMinijuego(m.audio, SONIDO_GOLPE);
                    continue;
                }
            }

            if (
                avispa.huida > 0.0f ||
                avispa.reposo > 0.0f ||
                m.inmunidadAvispa[i] > 0.0f ||
                distancia > RADIO_AVISPA_PISOTON ||
                altoRelativo > 0.85f
            )
            {
                continue;
            }

            int perdida = m.puntos[i] >= PENALIZACION_AVISPA_PISOTON ? PENALIZACION_AVISPA_PISOTON : m.puntos[i];
            m.puntos[i] -= perdida;
            m.inmunidadAvispa[i] = 2.0f;
            avispa.reposo = 1.6f;
            jugador.tiempoRalentizado = RALENTIZACION_AVISPA_PISOTON;
            jugador.multiplicadorRalentizacion = 0.45f;

            CrearPopupPisoton(m, jugador.posicion.x, jugador.posicion.z, -PENALIZACION_AVISPA_PISOTON, RED);
            ExplosionPisoton(m, jugador.posicion.x, jugador.posicion.z, Color{ 255, 90, 60, 255 }, 7);
            ReproducirSonidoMinijuego(m.audio, SONIDO_IMPACTO);
        }
    }

    (void)participantes;
}


//==================================================
// LOGICA: FLOR GIGANTE
//==================================================

static void ElegirPosicionFlorPisoton(MinijuegoPisotonPlagas& m)
{
    m.florX = AleatorioPisoton(-5.8f, 5.8f);
    m.florZ = AleatorioPisoton(-3.0f, 3.0f);
}


static void ActualizarFlorPisoton(
    MinijuegoPisotonPlagas& m,
    const JugadorPrueba jugadores[],
    int limite,
    float deltaTime
)
{
    if (m.florAbierta > 0.0f)
    {
        m.florAbierta -= deltaTime;
    }

    m.tiempoFlor -= deltaTime;

    if (m.tiempoFlor > 0.0f)
    {
        return;
    }

    // Floracion: gana quien este mas cerca del centro dentro del radio.
    int ganador = -1;
    float mejor = RADIO_FLOR_PISOTON;

    for (int i = 0; i < limite; i++)
    {
        if (!m.resultado.participantes[i].participo || jugadores[i].cayendo)
        {
            continue;
        }

        float distancia = DistanciaPisoton(m.florX, m.florZ, jugadores[i].posicion.x, jugadores[i].posicion.z);

        if (distancia < mejor)
        {
            mejor = distancia;
            ganador = i;
        }
    }

    m.florGanador = ganador;
    m.florAbierta = 2.0f;

    if (ganador >= 0)
    {
        SumarPuntosPisoton(m, ganador, PUNTOS_FLOR_PISOTON, m.florX, m.florZ, Color{ 255, 150, 220, 255 });
        ReproducirSonidoMinijuego(m.audio, SONIDO_ACIERTO);
    }

    for (int k = 0; k < 14; k++)
    {
        float angulo = AleatorioPisoton(0.0f, 2.0f * PI);
        AgregarParticulaPisoton(
            m,
            { m.florX, 1.2f, m.florZ },
            { std::cos(angulo) * 2.4f, AleatorioPisoton(1.0f, 3.0f), std::sin(angulo) * 2.4f },
            0.8f,
            0.1f,
            Color{ 255, 170, 220, 255 }
        );
    }

    m.tiempoFlor = INTERVALO_FLOR_PISOTON;
    ElegirPosicionFlorPisoton(m);
}


//==================================================
// LOGICA: FIN DE PARTIDA
//==================================================

static void FinalizarPartidaPisoton(MinijuegoPisotonPlagas& m, int limite)
{
    for (int i = 0; i < limite; i++)
    {
        ResultadoParticipante& r = m.resultado.participantes[i];

        if (!r.participo)
        {
            continue;
        }

        // Puntos y, en empate, mas babosas doradas.
        int posicion = 1;

        for (int j = 0; j < limite; j++)
        {
            if (j == i || !m.resultado.participantes[j].participo)
            {
                continue;
            }

            if (
                m.puntos[j] > m.puntos[i] ||
                (m.puntos[j] == m.puntos[i] && m.doradas[j] > m.doradas[i])
            )
            {
                posicion++;
            }
        }

        r.posicionFinal = posicion;
        r.puntuacionMinijuego = m.puntos[i];
    }

    int ganadores = 0;

    for (int i = 0; i < limite; i++)
    {
        if (m.resultado.participantes[i].participo && m.resultado.participantes[i].posicionFinal == 1)
        {
            ganadores++;
        }
    }

    m.resultado.estado = RESULTADO_MINIJUEGO_FINALIZADO;
    m.resultado.desenlace = ganadores == 1 ? DESENLACE_CON_GANADOR : DESENLACE_EMPATE;
    m.fase = FASE_PISOTON_TERMINADO;
    ReproducirSonidoMinijuego(m.audio, SONIDO_RESULTADO);
}


//==================================================
// BOTS
//==================================================

static void DirigirEntradaPisoton(
    InputMinijuegoParticipante& entrada,
    float dx,
    float dz
)
{
    float longitud = std::sqrt(dx * dx + dz * dz);

    if (longitud < 0.001f)
    {
        return;
    }

    dx /= longitud;
    dz /= longitud;

    const float umbral = 0.38f;
    entrada.derecha = dx > umbral;
    entrada.izquierda = dx < -umbral;
    entrada.atras = dz > umbral;
    entrada.adelante = dz < -umbral;
}


static int ElegirPlagaBotPisoton(
    const MinijuegoPisotonPlagas& m,
    const JugadorPrueba& jugador
)
{
    int mejor = -1;
    float mejorPuntaje = 0.0f;

    for (int p = 0; p < MAX_PLAGAS_PISOTON; p++)
    {
        const PlagaPisoton& plaga = m.plagas[p];

        if (!plaga.activa || plaga.escapando || plaga.aparicion < 0.5f)
        {
            continue;
        }

        float px = 0.0f;
        float pz = 0.0f;
        PosicionPlagaPisoton(plaga, px, pz);

        float distancia = DistanciaPisoton(px, pz, jugador.posicion.x, jugador.posicion.z);
        float puntaje = (float)ValorPlagaPisoton(plaga) / (1.0f + distancia * 0.25f);

        // Las avispas cerca de la plaga la hacen poco atractiva.
        for (int a = 0; a < MAX_AVISPAS_PISOTON; a++)
        {
            const AvispaPisoton& avispa = m.avispas[a];

            if (avispa.activa && avispa.huida <= 0.0f && DistanciaPisoton(avispa.x, avispa.z, px, pz) < 2.0f)
            {
                puntaje *= 0.3f;
            }
        }

        puntaje += AleatorioPisoton(0.0f, 0.35f);

        if (puntaje > mejorPuntaje)
        {
            mejorPuntaje = puntaje;
            mejor = p;
        }
    }

    return mejor;
}


static InputMinijuegoParticipante CrearEntradaBotPisoton(
    MinijuegoPisotonPlagas& m,
    int indice,
    const JugadorPrueba jugadores[],
    int limite,
    float deltaTime
)
{
    InputMinijuegoParticipante entrada{};
    EstadoBotPisoton& bot = m.bots[indice];
    const JugadorPrueba& jugador = jugadores[indice];

    bot.cooldownGolpe -= deltaTime;
    bot.cooldownSalto -= deltaTime;
    bot.reevaluar -= deltaTime;
    bot.cambioModo -= deltaTime;

    if (jugador.preparandoGolpeSuelo || jugador.golpeSueloActivo || jugador.aplastado)
    {
        return entrada;
    }

    if (bot.cambioModo <= 0.0f)
    {
        bot.modoAvispa = GetRandomValue(0, 1);
        bot.cambioModo = AleatorioPisoton(2.0f, 4.0f);
    }

    if (bot.reevaluar <= 0.0f)
    {
        bot.objetivo = ElegirPlagaBotPisoton(m, jugador);
        bot.objetivoId = bot.objetivo >= 0 ? m.plagas[bot.objetivo].id : -1;
        bot.reevaluar = AleatorioPisoton(0.32f, 0.5f);
        bot.errorAlcance = AleatorioPisoton(-0.2f, 0.3f);
    }

    float destinoX = 0.0f;
    float destinoZ = 0.0f;
    float parada = 0.2f;
    bool hayDestino = false;

    // 1. Avispa cercana: atacarla o apartarse.
    int avispaCercana = -1;
    float distanciaAvispa = 2.4f;

    for (int a = 0; a < MAX_AVISPAS_PISOTON; a++)
    {
        const AvispaPisoton& avispa = m.avispas[a];

        if (!avispa.activa || avispa.huida > 0.0f || avispa.reposo > 0.0f)
        {
            continue;
        }

        float d = DistanciaPisoton(avispa.x, avispa.z, jugador.posicion.x, jugador.posicion.z);

        if (d < distanciaAvispa)
        {
            distanciaAvispa = d;
            avispaCercana = a;
        }
    }

    if (avispaCercana >= 0)
    {
        const AvispaPisoton& avispa = m.avispas[avispaCercana];

        if (bot.modoAvispa == 1)
        {
            destinoX = avispa.x;
            destinoZ = avispa.z;
            parada = 0.4f;
            hayDestino = true;

            if (distanciaAvispa < 1.35f && bot.cooldownGolpe <= 0.0f)
            {
                entrada.golpear = true;
                bot.cooldownGolpe = AleatorioPisoton(0.6f, 1.0f);
            }
        }
        else
        {
            destinoX = jugador.posicion.x + (jugador.posicion.x - avispa.x);
            destinoZ = jugador.posicion.z + (jugador.posicion.z - avispa.z);
            parada = 0.0f;
            hayDestino = true;
        }
    }

    // 2. Flor a punto de florecer.
    if (!hayDestino && m.tiempoFlor < 3.5f)
    {
        float distanciaFlor = DistanciaPisoton(m.florX, m.florZ, jugador.posicion.x, jugador.posicion.z);

        if (distanciaFlor / 5.0f < m.tiempoFlor + 0.8f)
        {
            destinoX = m.florX;
            destinoZ = m.florZ;
            parada = 0.35f;
            hayDestino = true;

            // Disputa: empujar a un rival que esta mas cerca del centro.
            for (int j = 0; j < limite; j++)
            {
                if (j == indice || !m.resultado.participantes[j].participo)
                {
                    continue;
                }

                float dRival = DistanciaPisoton(m.florX, m.florZ, jugadores[j].posicion.x, jugadores[j].posicion.z);
                float dEntre = DistanciaPisoton(jugador.posicion.x, jugador.posicion.z, jugadores[j].posicion.x, jugadores[j].posicion.z);

                if (dRival < RADIO_FLOR_PISOTON && dRival < distanciaFlor && dEntre < 2.0f)
                {
                    destinoX = jugadores[j].posicion.x;
                    destinoZ = jugadores[j].posicion.z;
                    parada = 0.5f;

                    if (dEntre < 1.3f && bot.cooldownGolpe <= 0.0f)
                    {
                        entrada.golpear = true;
                        bot.cooldownGolpe = AleatorioPisoton(0.6f, 1.1f);
                    }

                    break;
                }
            }
        }
    }

    // 3. Plaga objetivo.
    if (!hayDestino && bot.objetivo >= 0)
    {
        const PlagaPisoton& plaga = m.plagas[bot.objetivo];

        if (plaga.activa && plaga.id == bot.objetivoId && !plaga.escapando)
        {
            float px = 0.0f;
            float pz = 0.0f;
            PosicionPlagaPisoton(plaga, px, pz);

            // Se adelanta un poco al movimiento de la plaga.
            float adelanto = plaga.aturdida > 0.0f ? 0.0f : 0.25f;
            destinoX = px + plaga.dirX * adelanto;
            destinoZ = pz + plaga.dirZ * adelanto;
            parada = 0.25f;
            hayDestino = true;

            float distancia = DistanciaPisoton(px, pz, jugador.posicion.x, jugador.posicion.z);

            if (!jugador.enSuelo)
            {
                // En el aire: segundo salto (ground pound) cuando esta encima.
                if (distancia < 0.95f + bot.errorAlcance && jugador.velocidad.y < 3.0f)
                {
                    entrada.saltar = true;
                }
            }
            else if (distancia < 1.5f + bot.errorAlcance && bot.cooldownSalto <= 0.0f)
            {
                entrada.saltar = true;
                bot.cooldownSalto = AleatorioPisoton(0.8f, 1.3f);
            }
        }
        else
        {
            bot.objetivo = -1;
            bot.reevaluar = 0.0f;
        }
    }

    // 4. Sin nada que hacer: merodear por el jardin.
    if (!hayDestino)
    {
        bot.tiempoVagar -= deltaTime;

        if (
            bot.tiempoVagar <= 0.0f ||
            DistanciaPisoton(bot.vagarX, bot.vagarZ, jugador.posicion.x, jugador.posicion.z) < 0.6f
        )
        {
            bot.vagarX = AleatorioPisoton(-LIMITE_X_PISOTON + 1.0f, LIMITE_X_PISOTON - 1.0f);
            bot.vagarZ = AleatorioPisoton(-LIMITE_Z_PISOTON + 1.0f, LIMITE_Z_PISOTON - 1.0f);
            bot.tiempoVagar = AleatorioPisoton(1.2f, 2.4f);
        }

        destinoX = bot.vagarX;
        destinoZ = bot.vagarZ;
        parada = 0.4f;
        hayDestino = true;
    }

    if (hayDestino && DistanciaPisoton(destinoX, destinoZ, jugador.posicion.x, jugador.posicion.z) > parada)
    {
        DirigirEntradaPisoton(entrada, destinoX - jugador.posicion.x, destinoZ - jugador.posicion.z);
    }

    return entrada;
}


//==================================================
// VISUAL: ESTADO (manchas, ondas y fin)
//==================================================
// Lo escribe Actualizar despues de la logica comparando una instantanea:
// nunca toca reglas ni consume GetRandomValue.

struct InstantaneaPisoton
{
    int puntos[MAX_PARTICIPANTES]{};
    bool activa[MAX_PLAGAS_PISOTON]{};
    int id[MAX_PLAGAS_PISOTON]{};
    bool vivo[MAX_PLAGAS_PISOTON][MAX_SEGMENTOS_PISOTON]{};
};


static void TomarInstantaneaPisoton(const MinijuegoPisotonPlagas& m, InstantaneaPisoton& s)
{
    for (int i = 0; i < MAX_PARTICIPANTES; i++) s.puntos[i] = m.puntos[i];

    for (int p = 0; p < MAX_PLAGAS_PISOTON; p++)
    {
        s.activa[p] = m.plagas[p].activa;
        s.id[p] = m.plagas[p].id;

        for (int k = 0; k < MAX_SEGMENTOS_PISOTON; k++) s.vivo[p][k] = m.plagas[p].segVivo[k];
    }
}


static void DecaerVisualPisoton(VisualPisoton& v, float dt)
{
    for (ManchaPisoton& mancha : v.manchas)
    {
        if (mancha.vida > 0.0f) mancha.vida = mancha.vida - dt > 0.0f ? mancha.vida - dt : 0.0f;
    }

    for (OndaPisoton& onda : v.ondas)
    {
        if (onda.vida > 0.0f) onda.vida = onda.vida - dt > 0.0f ? onda.vida - dt : 0.0f;
    }

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        v.picado[i] = v.picado[i] - dt > 0.0f ? v.picado[i] - dt : 0.0f;
    }
}


static void CompararVisualPisoton(
    MinijuegoPisotonPlagas& m,
    const InstantaneaPisoton& antes,
    const JugadorPrueba jugadores[],
    int limite
)
{
    VisualPisoton& v = m.visual;

    // Segmentos aplastados: vivos antes, muertos ahora (escapar no los mata).
    for (int p = 0; p < MAX_PLAGAS_PISOTON; p++)
    {
        const PlagaPisoton& plaga = m.plagas[p];

        if (!antes.activa[p] || plaga.id != antes.id[p])
        {
            continue;
        }

        for (int k = 0; k < MAX_SEGMENTOS_PISOTON; k++)
        {
            if (!antes.vivo[p][k] || plaga.segVivo[k])
            {
                continue;
            }

            ManchaPisoton& mancha = v.manchas[v.siguienteMancha];
            v.siguienteMancha = (v.siguienteMancha + 1) % MAX_MANCHAS_PISOTON;
            mancha.x = plaga.segX[k];
            mancha.z = plaga.segZ[k];
            mancha.vida = 6.0f;
            mancha.giro = (float)((plaga.id * 7 + k * 3) % 12) * 30.0f;
            mancha.tipo = (int)plaga.tipo;
        }
    }

    // Puntos: onda del color del jugador; picadura: destello rojo.
    for (int i = 0; i < limite && i < MAX_PARTICIPANTES; i++)
    {
        if (m.puntos[i] == antes.puntos[i])
        {
            continue;
        }

        bool suma = m.puntos[i] > antes.puntos[i];
        OndaPisoton& onda = v.ondas[v.siguienteOnda];
        v.siguienteOnda = (v.siguienteOnda + 1) % MAX_ONDAS_PISOTON;
        onda.x = jugadores[i].posicion.x;
        onda.z = jugadores[i].posicion.z;
        onda.vida = 0.7f;
        onda.jugador = i;
        onda.picadura = !suma;

        if (!suma) v.picado[i] = 0.6f;
    }

    if (m.fase == FASE_PISOTON_TERMINADO && v.tiempoFin < 0.0f)
    {
        v.tiempoFin = m.tiempoAnimacion;
    }
}


//==================================================
// CICLO DE VIDA
//==================================================

static void LiberarMallasPisoton();


void MinijuegoPisotonPlagas::Inicializar()
{
    resultado = {};
    visual = {};
    resultado.formato = FORMATO_MINIJUEGO_INDIVIDUAL;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        puntos[i] = 0;
        doradas[i] = 0;
        aplastadas[i] = 0;
        inmunidadAvispa[i] = 0.0f;
        bots[i] = {};
    }

    for (int i = 0; i < MAX_PARTICULAS_PISOTON; i++)
    {
        particulas[i] = {};
    }

    for (int p = 0; p < MAX_PLAGAS_PISOTON; p++)
    {
        plagas[p] = {};
    }

    for (int k = 0; k < MAX_POPUPS_PISOTON; k++)
    {
        popups[k] = {};
    }

    for (int k = 0; k < MAX_MADRIGUERAS_PISOTON; k++)
    {
        madrigueras[k] = {};
        madrigueras[k].x = MADRIGUERAS_X_PISOTON[k];
        madrigueras[k].z = MADRIGUERAS_Z_PISOTON[k];
    }

    for (int a = 0; a < MAX_AVISPAS_PISOTON; a++)
    {
        avispas[a] = {};
        avispas[a].retrasoAparicion = a == 0 ? 4.0f : 16.0f;
    }

    // Suelo fisico: bloque ancho bajo el cesped.
    cantidadBloques = 0;
    AgregarBloquePrueba(
        bloques,
        cantidadBloques,
        MAX_BLOQUES_PISOTON,
        { 0.0f, SUELO_PISOTON - 0.5f, 0.0f },
        { LIMITE_X_PISOTON * 2.0f + 3.0f, 1.0f, LIMITE_Z_PISOTON * 2.0f + 3.0f },
        Color{ 84, 150, 60, 255 }
    );

    camara.position = { 0.0f, 15.0f, 10.5f };
    camara.target = { 0.0f, 0.0f, 0.6f };
    camara.up = { 0.0f, 1.0f, 0.0f };
    camara.fovy = 50.0f;
    camara.projection = CAMERA_PERSPECTIVE;

    fase = FASE_PISOTON_PREPARACION;
    siguienteId = 1;
    doradasGeneradas = 0;
    tiempoPreparacion = DURACION_PREPARACION_PISOTON;
    tiempoRestante = DURACION_PARTIDA_PISOTON;
    tiempoAnimacion = 0.0f;
    proximaAparicion = 0.8f;
    florAbierta = 0.0f;
    florGanador = -1;
    tiempoFlor = PRIMERA_FLOR_PISOTON;
    ElegirPosicionFlorPisoton(*this);
}


void MinijuegoPisotonPlagas::Reiniciar(
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

    int indices[MAX_PARTICIPANTES]{};
    int cantidad = ObtenerIndicesParticipantesActivos(
        participantes,
        indices,
        MAX_PARTICIPANTES
    );

    if (cantidad < 2)
    {
        resultado.estado = RESULTADO_MINIJUEGO_CANCELADO;
        fase = FASE_PISOTON_TERMINADO;
        return;
    }

    CargarPiezasJugablesPisotonPlagas();
    LiberarMallasPisoton();
    int limite = LimitePisoton(cantidadMaxima);

    for (int k = 0; k < cantidad; k++)
    {
        int i = indices[k];

        if (i >= limite)
        {
            continue;
        }

        // Reparto en fila central, separados de las madrigueras.
        float fraccion = cantidad > 1 ? (float)k / (float)(cantidad - 1) : 0.5f;
        Vector3 spawn =
        {
            -4.5f + fraccion * 9.0f,
            SUELO_PISOTON + 0.72f,
            (k % 2 == 0) ? 1.2f : -0.2f
        };

        ConfigurarJugadorMinijuegoEstandar(jugadores[i], spawn);
        jugadores[i].direccionMirada = { 0.0f, 0.0f, -1.0f };
    }
}


//==================================================
// ACTUALIZACION
//==================================================

void MinijuegoPisotonPlagas::Actualizar(
    float deltaTime,
    JugadorPrueba jugadores[],
    int cantidadMaxima,
    Participante participantes[]
)
{
    tiempoAnimacion += deltaTime;
    DecaerVisualPisoton(visual, deltaTime);
    ActualizarParticulasTierra(particulas, MAX_PARTICULAS_PISOTON, deltaTime);
    ActualizarPopupsPisoton(*this, deltaTime);

    if (
        fase == FASE_PISOTON_TERMINADO ||
        resultado.estado == RESULTADO_MINIJUEGO_CANCELADO
    )
    {
        return;
    }

    if (fase == FASE_PISOTON_PREPARACION)
    {
        float preparacionAntes = tiempoPreparacion;
        tiempoPreparacion -= deltaTime;
        ActualizarAudioCuentaRegresiva(audio, preparacionAntes, tiempoPreparacion);

        if (tiempoPreparacion <= 0.0f)
        {
            tiempoPreparacion = 0.0f;
            fase = FASE_PISOTON_JUGANDO;
        }

        return;
    }

    int limite = LimitePisoton(cantidadMaxima);
    InstantaneaPisoton antesVisual;
    TomarInstantaneaPisoton(*this, antesVisual);

    float restanteAntes = tiempoRestante;
    tiempoRestante -= deltaTime;
    ActualizarAudioAlertaTiempo(audio, restanteAntes, tiempoRestante);

    float transcurrido = DURACION_PARTIDA_PISOTON - tiempoRestante;

    // Los humanos desconectados se tratan como bots.
    Participante efectivos[MAX_PARTICIPANTES];

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        efectivos[i] = participantes[i];
        efectivos[i].conectado = true;
    }

    // 1. Movimiento y fisica estandar.
    for (int i = 0; i < limite; i++)
    {
        if (!participantes[i].activo)
        {
            continue;
        }

        JugadorPrueba& jugador = jugadores[i];
        InputMinijuegoParticipante entrada{};

        if (participantes[i].esBot || !participantes[i].conectado)
        {
            entrada = CrearEntradaBotPisoton(*this, i, jugadores, limite, deltaTime);
        }
        else
        {
            entrada = LeerInputMinijuegoParticipante(participantes[i]);
        }

        ActualizarJugadorPruebaNormal(
            jugador,
            entrada,
            bloques,
            cantidadBloques,
            particulas,
            MAX_PARTICULAS_PISOTON,
            true,
            true,
            deltaTime
        );

        jugador.posicion.x = LimitarPisoton(jugador.posicion.x, -LIMITE_X_PISOTON, LIMITE_X_PISOTON);
        jugador.posicion.z = LimitarPisoton(jugador.posicion.z, -LIMITE_Z_PISOTON, LIMITE_Z_PISOTON);

        if (jugador.cayendo || jugador.posicion.y < -3.0f)
        {
            ReiniciarJugadorPrueba(jugador);
        }
    }

    // 2. Plagas, madrigueras, avispas y flor.
    ActualizarMadriguerasPisoton(*this, deltaTime, transcurrido);
    ActualizarPlagasPisoton(*this, deltaTime);

    // 3. Pisotones (antes de que la resolucion compartida consuma el impacto).
    ResolverPisotonesPisoton(*this, jugadores, limite);

    // 4. Golpes, ground pound entre jugadores y colisiones (el sonido del
    // ground pound lo reproduce la fisica compartida).
    float ralentizacionAntes[MAX_PARTICIPANTES]{};

    for (int i = 0; i < limite; i++)
    {
        ralentizacionAntes[i] = jugadores[i].tiempoRalentizado;
    }

    ResolverInteraccionesJugadoresMinijuegoEstandar(
        jugadores,
        efectivos,
        limite,
        particulas,
        MAX_PARTICULAS_PISOTON
    );

    for (int i = 0; i < limite; i++)
    {
        if (jugadores[i].tiempoRalentizado > ralentizacionAntes[i] + 0.2f)
        {
            ReproducirSonidoMinijuego(audio, SONIDO_GOLPE);
            break;
        }
    }

    ActualizarAvispasPisoton(*this, jugadores, participantes, limite, deltaTime);
    ActualizarFlorPisoton(*this, jugadores, limite, deltaTime);

    // 5. Tiempo limite duro.
    if (tiempoRestante <= 0.0f)
    {
        tiempoRestante = 0.0f;
        FinalizarPartidaPisoton(*this, limite);
    }

    CompararVisualPisoton(*this, antesVisual, jugadores, limite);
}


//==================================================
// VISUAL: JARDIN A ESCALA DE BICHO
//==================================================
// Manana soleada con rocio vista desde el tamano de un bicho: la arena es
// un claro de tierra apisonada con musgo, bordeado de guijarros; las
// briznas de pasto son arboles, hay setas, treboles, una chapa de botella,
// bellotas y hojas caidas; mas atras margaritas, tulipanes y un diente de
// leon; al fondo una cerca de madera colosal, una regadera gigante que
// gotea, una maceta y el cerco de ligustro bajo el cielo de la manana.
// Lo estatico se hornea en mallas; solo se dibuja por frame lo que se
// mueve. La logica no depende de nada de esta seccion.

// Paleta: cielo celeste de manana, tierra tibia, verdes del pasto, madera
// gastada y un unico acento (rosa fuerte) reservado a la flor disputada.
static const Color COLOR_CIELO_ALTO_P = { 84, 150, 222, 255 };
static const Color COLOR_CIELO_MEDIO_P = { 140, 192, 238, 255 };
static const Color COLOR_CIELO_BAJO_P = { 206, 228, 240, 255 };
static const Color COLOR_HORIZONTE_P = { 238, 238, 214, 255 };
static const Color COLOR_NIEBLA_P = { 204, 226, 222, 255 };
static const Color COLOR_SOL_P = { 255, 246, 206, 255 };
static const Color COLOR_TINTE_SOL_P = { 255, 238, 176, 255 };
static const Color COLOR_TIERRA_P = { 134, 100, 68, 255 };
static const Color COLOR_TIERRA_OSCURA_P = { 88, 62, 42, 255 };
static const Color COLOR_MUSGO_P = { 108, 132, 64, 255 };
static const Color COLOR_PASTO_P = { 74, 142, 56, 255 };
static const Color COLOR_PASTO_CLARO_P = { 140, 192, 82, 255 };
static const Color COLOR_PASTO_OSCURO_P = { 40, 94, 44, 255 };
static const Color COLOR_MADERA_P = { 172, 136, 98, 255 };
static const Color COLOR_PIEDRA_P = { 158, 152, 140, 255 };
static const Color COLOR_ACENTO_P = { 255, 92, 176, 255 };
static const Color COLOR_PELIGRO_P = { 255, 72, 52, 255 };

// Camara de la vista: algo baja para que el jardin del fondo se vea alto.
static const Vector3 OJO_PISOTON = { 0.0f, 10.8f, 14.6f };
static const Vector3 MIRA_PISOTON = { 0.0f, 0.6f, -1.8f };
static const float FOV_PISOTON = 48.0f;

// Borde visible de la arena (guijarros justo fuera del limite real).
static const float BORDE_X_PISOTON = LIMITE_X_PISOTON + 0.55f;
static const float BORDE_Z_PISOTON = LIMITE_Z_PISOTON + 0.55f;


static float Ruido01P(int indice, int semilla)
{
    unsigned int h = (unsigned int)indice * 374761393u + (unsigned int)semilla * 668265263u;
    h = (h ^ (h >> 13)) * 1274126177u;
    h = h ^ (h >> 16);
    return (float)(h & 0xFFFFu) / 65535.0f;
}


// Ruido suave por interpolacion de una reja de hashes (manchas del suelo).
static float RuidoSuaveP(float x, float z, float escala, int semilla)
{
    float fx = x / escala, fz = z / escala;
    int ix = (int)std::floor(fx), iz = (int)std::floor(fz);
    float tx = fx - (float)ix, tz = fz - (float)iz;
    tx = tx * tx * (3.0f - 2.0f * tx);
    tz = tz * tz * (3.0f - 2.0f * tz);
    auto H = [&](int a, int b) { return Ruido01P(a * 7919 + b * 104729 + 50000, semilla); };
    float a = H(ix, iz) + (H(ix + 1, iz) - H(ix, iz)) * tx;
    float b = H(ix, iz + 1) + (H(ix + 1, iz + 1) - H(ix, iz + 1)) * tx;
    return a + (b - a) * tz;
}


static GrabadorMalla g_grabP;
static Vector3 g_ojoP = OJO_PISOTON;      // bruma: siempre desde la camara final
static Vector3 g_camP = OJO_PISOTON;      // camara actual (halos y particulas)
static Vector3 g_derP = { 1.0f, 0.0f, 0.0f };
static Vector3 g_arribaP = { 0.0f, 1.0f, 0.0f };
static float g_emisP = 0.0f;              // 1 = superficie luminosa: casi sin bruma


static float Lim01P(float v)
{
    return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
}


static unsigned char CanalP(float v)
{
    return (unsigned char)(v < 0.0f ? 0.0f : (v > 255.0f ? 255.0f : v));
}


static Color TonoP(Color c, float f)
{
    return { CanalP((float)c.r * f), CanalP((float)c.g * f), CanalP((float)c.b * f), c.a };
}


static Color MezclaP(Color a, Color b, float t)
{
    t = Lim01P(t);
    return {
        CanalP((float)a.r + ((float)b.r - (float)a.r) * t),
        CanalP((float)a.g + ((float)b.g - (float)a.g) * t),
        CanalP((float)a.b + ((float)b.b - (float)a.b) * t),
        CanalP((float)a.a + ((float)b.a - (float)a.a) * t)
    };
}


static Color AlfaP(Color c, float alfa)
{
    c.a = CanalP((float)c.a * Lim01P(alfa));
    return c;
}


static Color VariarP(Color c, int indice, int semilla, float amplitud)
{
    return TonoP(c, 1.0f - amplitud + 2.0f * amplitud * Ruido01P(indice, semilla));
}


// Niebla de color: lo lejano se funde con la bruma clara de la manana.
static Color NieblaP(Color c, Vector3 p)
{
    float dx = p.x - g_ojoP.x, dy = p.y - g_ojoP.y, dz = p.z - g_ojoP.z;
    float d = std::sqrt(dx * dx + dy * dy + dz * dz);
    float n = Lim01P((d - 22.0f) / 48.0f) * 0.78f * (1.0f - 0.7f * g_emisP);
    Color r = MezclaP(c, COLOR_NIEBLA_P, n);
    r.a = c.a;
    return r;
}


static void VP(Vector3 p, Color c)
{
    if (g_grabP.activa)
    {
        GrabarVerticeMalla(g_grabP, p, c);
        return;
    }

    rlColor4ub(c.r, c.g, c.b, c.a);
    rlVertex3f(p.x, p.y, p.z);
}


static void LoteP()
{
    if (!g_grabP.activa) rlBegin(RL_TRIANGLES);
}


static void FinLoteP()
{
    if (!g_grabP.activa) rlEnd();
}


static void TriP(Vector3 a, Vector3 b, Vector3 c, Color color)
{
    VP(a, NieblaP(color, a));
    VP(b, NieblaP(color, b));
    VP(c, NieblaP(color, c));
}


static void QuadP(Vector3 a, Vector3 b, Vector3 c, Vector3 d, Color color)
{
    TriP(a, b, c, color);
    TriP(a, c, d, color);
}


// Quad con un color por vertice (con bruma).
static void Quad4P(Vector3 a, Vector3 b, Vector3 c, Vector3 d, Color ca, Color cb, Color cc, Color cd)
{
    VP(a, NieblaP(ca, a)); VP(b, NieblaP(cb, b)); VP(c, NieblaP(cc, c));
    VP(a, NieblaP(ca, a)); VP(c, NieblaP(cc, c)); VP(d, NieblaP(cd, d));
}


// Quad sin bruma con un color por vertice (luces aditivas y sombras).
static void QuadColoresP(Vector3 a, Vector3 b, Vector3 c, Vector3 d, Color ca, Color cb, Color cc, Color cd)
{
    VP(a, ca); VP(b, cb); VP(c, cc);
    VP(a, ca); VP(c, cc); VP(d, cd);
}


// Sombreado por caras de la guia (arriba 1.0, frente 0.78, izquierda 0.70,
// derecha 0.62) interpolado para normales cualesquiera; el sol de la
// manana entra por la derecha y desde atras.
static float LuzP(Vector3 n)
{
    float ax = std::fabs(n.x), ay = std::fabs(n.y), az = std::fabs(n.z);
    float suma = ax + ay + az;

    if (suma < 0.0001f) return 1.0f;

    float f =
        (n.y > 0.0f ? ay * 1.0f : ay * 0.46f) +
        (n.z > 0.0f ? az * 0.78f : az * 0.58f) +
        (n.x < 0.0f ? ax * 0.70f : ax * 0.62f);

    return f / suma;
}


static Color CaraP(Color c, Vector3 n)
{
    Color r = TonoP(c, LuzP(n));

    if (n.y > 0.3f) r = MezclaP(r, COLOR_TINTE_SOL_P, 0.12f * n.y);

    return r;
}


// Caja con medio-ejes arbitrarios: caras sombreadas por normal.
static void CajaEjesP(Vector3 c, Vector3 ex, Vector3 ey, Vector3 ez, Color color, bool fondo = false)
{
    auto P = [&](float sx, float sy, float sz)
    {
        return Vector3{
            c.x + ex.x * sx + ey.x * sy + ez.x * sz,
            c.y + ex.y * sx + ey.y * sy + ez.y * sz,
            c.z + ex.z * sx + ey.z * sy + ez.z * sz
        };
    };

    Vector3 nx = Vector3Normalize(ex), ny = Vector3Normalize(ey), nz = Vector3Normalize(ez);

    QuadP(P(-1, 1, -1), P(-1, 1, 1), P(1, 1, 1), P(1, 1, -1), CaraP(color, ny));
    QuadP(P(-1, -1, 1), P(1, -1, 1), P(1, 1, 1), P(-1, 1, 1), CaraP(color, nz));
    QuadP(P(1, -1, -1), P(-1, -1, -1), P(-1, 1, -1), P(1, 1, -1), CaraP(color, Vector3Negate(nz)));
    QuadP(P(-1, -1, -1), P(-1, -1, 1), P(-1, 1, 1), P(-1, 1, -1), CaraP(color, Vector3Negate(nx)));
    QuadP(P(1, -1, 1), P(1, -1, -1), P(1, 1, -1), P(1, 1, 1), CaraP(color, nx));

    if (fondo)
    {
        QuadP(P(-1, -1, -1), P(1, -1, -1), P(1, -1, 1), P(-1, -1, 1), CaraP(color, Vector3Negate(ny)));
    }
}


static void CajaP(Vector3 c, Vector3 t, Color color)
{
    CajaEjesP(c, { t.x * 0.5f, 0.0f, 0.0f }, { 0.0f, t.y * 0.5f, 0.0f }, { 0.0f, 0.0f, t.z * 0.5f }, color, c.y - t.y * 0.5f > 0.05f);
}


// Caja girada en Y (grados) y ladeada sobre su eje X local (grados).
static void CajaGiradaP(Vector3 c, Vector3 t, float giroY, Color color, float ladeo = 0.0f)
{
    float a = giroY * DEG2RAD;
    float l = ladeo * DEG2RAD;
    Vector3 ux = { std::cos(a), 0.0f, -std::sin(a) };
    Vector3 uz0 = { std::sin(a), 0.0f, std::cos(a) };
    Vector3 uy = { -uz0.x * std::sin(l), std::cos(l), -uz0.z * std::sin(l) };
    Vector3 uz = { uz0.x * std::cos(l), std::sin(l), uz0.z * std::cos(l) };

    CajaEjesP(c, Vector3Scale(ux, t.x * 0.5f), Vector3Scale(uy, t.y * 0.5f), Vector3Scale(uz, t.z * 0.5f), color, true);
}


// Viga de seccion cuadrada entre dos puntos.
static void VigaP(Vector3 a, Vector3 b, float grosor, Color color)
{
    Vector3 eje = Vector3Subtract(b, a);
    float largo = Vector3Length(eje);

    if (largo < 0.0001f) return;

    Vector3 u = Vector3Scale(eje, 1.0f / largo);
    Vector3 ref = std::fabs(u.y) < 0.9f ? Vector3{ 0.0f, 1.0f, 0.0f } : Vector3{ 1.0f, 0.0f, 0.0f };
    Vector3 v = Vector3Normalize(Vector3CrossProduct(u, ref));
    Vector3 w = Vector3CrossProduct(v, u);
    Vector3 centro = Vector3Scale(Vector3Add(a, b), 0.5f);

    CajaEjesP(centro, Vector3Scale(v, grosor * 0.5f), Vector3Scale(w, grosor * 0.5f), Vector3Scale(u, largo * 0.5f), color, true);
}


// Cilindro (o tronco de cono) entre dos puntos, caras sombreadas.
static void CilP(Vector3 a, Vector3 b, float ra, float rb, int lados, Color color, bool tapas = true)
{
    Vector3 eje = Vector3Subtract(b, a);
    float largo = Vector3Length(eje);

    if (largo < 0.0001f) return;

    eje = Vector3Scale(eje, 1.0f / largo);
    Vector3 ref = std::fabs(eje.y) < 0.9f ? Vector3{ 0.0f, 1.0f, 0.0f } : Vector3{ 1.0f, 0.0f, 0.0f };
    Vector3 u = Vector3Normalize(Vector3CrossProduct(eje, ref));
    Vector3 v = Vector3CrossProduct(eje, u);

    for (int k = 0; k < lados; k++)
    {
        float a0 = 2.0f * PI * (float)k / (float)lados;
        float a1 = 2.0f * PI * (float)(k + 1) / (float)lados;
        float am = 0.5f * (a0 + a1);
        Vector3 n0 = Vector3Add(Vector3Scale(u, std::cos(a0)), Vector3Scale(v, std::sin(a0)));
        Vector3 n1 = Vector3Add(Vector3Scale(u, std::cos(a1)), Vector3Scale(v, std::sin(a1)));
        Vector3 nm = Vector3Add(Vector3Scale(u, std::cos(am)), Vector3Scale(v, std::sin(am)));

        QuadP(
            Vector3Add(a, Vector3Scale(n0, ra)),
            Vector3Add(a, Vector3Scale(n1, ra)),
            Vector3Add(b, Vector3Scale(n1, rb)),
            Vector3Add(b, Vector3Scale(n0, rb)),
            CaraP(color, nm)
        );

        if (tapas)
        {
            TriP(b, Vector3Add(b, Vector3Scale(n0, rb)), Vector3Add(b, Vector3Scale(n1, rb)), CaraP(color, eje));
            TriP(a, Vector3Add(a, Vector3Scale(n1, ra)), Vector3Add(a, Vector3Scale(n0, ra)), CaraP(color, Vector3Negate(eje)));
        }
    }
}


// Elipsoide (o media cupula con desde = 0) con caras sombreadas.
static void BolaP(Vector3 c, Vector3 r, Color color, int anillos = 4, int lados = 8, float desde = -0.5f * PI)
{
    for (int i = 0; i < anillos; i++)
    {
        float la0 = desde + (0.5f * PI - desde) * (float)i / (float)anillos;
        float la1 = desde + (0.5f * PI - desde) * (float)(i + 1) / (float)anillos;

        for (int k = 0; k < lados; k++)
        {
            float lo0 = 2.0f * PI * (float)k / (float)lados;
            float lo1 = 2.0f * PI * (float)(k + 1) / (float)lados;
            float lam = 0.5f * (la0 + la1), lom = 0.5f * (lo0 + lo1);
            Vector3 n = { std::cos(lam) * std::cos(lom), std::sin(lam), std::cos(lam) * std::sin(lom) };
            auto P = [&](float la, float lo)
            {
                return Vector3{ c.x + r.x * std::cos(la) * std::cos(lo), c.y + r.y * std::sin(la), c.z + r.z * std::cos(la) * std::sin(lo) };
            };
            QuadP(P(la0, lo0), P(la0, lo1), P(la1, lo1), P(la1, lo0), CaraP(color, n));
        }
    }
}


// Disco horizontal con color de centro y de borde.
static void DiscoSueloP(Vector3 c, float rx, float rz, int lados, Color centro, Color borde, float giro = 0.0f)
{
    for (int k = 0; k < lados; k++)
    {
        float a0 = giro + 2.0f * PI * (float)k / (float)lados;
        float a1 = giro + 2.0f * PI * (float)(k + 1) / (float)lados;
        Vector3 p0 = { c.x + std::cos(a0) * rx, c.y, c.z + std::sin(a0) * rz };
        Vector3 p1 = { c.x + std::cos(a1) * rx, c.y, c.z + std::sin(a1) * rz };
        VP(c, NieblaP(centro, c)); VP(p0, NieblaP(borde, p0)); VP(p1, NieblaP(borde, p1));
    }
}


// Mancha irregular (borde con radio variable por hash).
static void ManchaSueloP(Vector3 c, float r, int lados, Color centro, Color borde, int semilla)
{
    for (int k = 0; k < lados; k++)
    {
        float a0 = 2.0f * PI * (float)k / (float)lados;
        float a1 = 2.0f * PI * (float)(k + 1) / (float)lados;
        float r0 = r * (0.7f + 0.5f * Ruido01P(k % lados, semilla));
        float r1 = r * (0.7f + 0.5f * Ruido01P((k + 1) % lados, semilla));
        Vector3 p0 = { c.x + std::cos(a0) * r0, c.y, c.z + std::sin(a0) * r0 * 0.8f };
        Vector3 p1 = { c.x + std::cos(a1) * r1, c.y, c.z + std::sin(a1) * r1 * 0.8f };
        VP(c, NieblaP(centro, c)); VP(p0, NieblaP(borde, p0)); VP(p1, NieblaP(borde, p1));
    }
}


static void AnilloSueloP(Vector3 c, float r0, float r1, int lados, Color color, Color colorFuera)
{
    for (int k = 0; k < lados; k++)
    {
        float a0 = 2.0f * PI * (float)k / (float)lados;
        float a1 = 2.0f * PI * (float)(k + 1) / (float)lados;
        Vector3 i0 = { c.x + std::cos(a0) * r0, c.y, c.z + std::sin(a0) * r0 };
        Vector3 i1 = { c.x + std::cos(a1) * r0, c.y, c.z + std::sin(a1) * r0 };
        Vector3 o0 = { c.x + std::cos(a0) * r1, c.y, c.z + std::sin(a0) * r1 };
        Vector3 o1 = { c.x + std::cos(a1) * r1, c.y, c.z + std::sin(a1) * r1 };
        QuadColoresP(i0, o0, o1, i1, color, colorFuera, colorFuera, color);
    }
}


// Disco que mira a la camara con centro y borde de distinto color (halos).
static void HaloP(Vector3 c, float radio, Color centro, Color borde, int lados = 14)
{
    for (int k = 0; k < lados; k++)
    {
        float a0 = 2.0f * PI * (float)k / (float)lados;
        float a1 = 2.0f * PI * (float)(k + 1) / (float)lados;
        Vector3 p0 = Vector3Add(c, Vector3Add(Vector3Scale(g_derP, std::cos(a0) * radio), Vector3Scale(g_arribaP, std::sin(a0) * radio)));
        Vector3 p1 = Vector3Add(c, Vector3Add(Vector3Scale(g_derP, std::cos(a1) * radio), Vector3Scale(g_arribaP, std::sin(a1) * radio)));
        VP(c, centro); VP(p0, borde); VP(p1, borde);
    }
}


// Cuadradito que mira a la camara (particulas).
static void PuntoP(Vector3 c, float r, Color color)
{
    Vector3 d = Vector3Scale(g_derP, r), u = Vector3Scale(g_arribaP, r);
    QuadColoresP(
        Vector3Subtract(Vector3Subtract(c, d), u), Vector3Subtract(Vector3Add(c, d), u),
        Vector3Add(Vector3Add(c, d), u), Vector3Add(Vector3Subtract(c, d), u),
        color, color, color, color);
}


// Brizna de pasto: tira afinada que se curva hacia "giro"; la doblez del
// nervio central deja una mitad clara y otra oscura.
static void BriznaP(Vector3 base, float alto, float ancho, float giro, float curva, Color abajo, Color arriba, int segmentos = 4)
{
    float a = giro * DEG2RAD;
    Vector3 lado = { std::cos(a), 0.0f, -std::sin(a) };
    Vector3 frente = { std::sin(a), 0.0f, std::cos(a) };
    float luzFrente = 0.84f + 0.16f * frente.z;

    auto Eje = [&](float t)
    {
        float doblez = curva * alto * t * t;
        return Vector3{ base.x + frente.x * doblez, base.y + alto * t * (1.0f - 0.18f * curva * t), base.z + frente.z * doblez };
    };
    auto Ancho = [&](float t) { return ancho * (1.0f - t) * (0.9f + 0.25f * (1.0f - t)); };

    for (int k = 0; k < segmentos; k++)
    {
        float t0 = (float)k / (float)segmentos;
        float t1 = (float)(k + 1) / (float)segmentos;
        Vector3 e0 = Eje(t0), e1 = Eje(t1);
        // Nervio adelantado: la hoja queda en V.
        Vector3 n0 = Vector3Add(e0, Vector3Scale(frente, Ancho(t0) * 0.35f));
        Vector3 n1 = Vector3Add(e1, Vector3Scale(frente, Ancho(t1) * 0.35f));
        Vector3 i0 = Vector3Subtract(e0, Vector3Scale(lado, Ancho(t0)));
        Vector3 i1 = Vector3Subtract(e1, Vector3Scale(lado, Ancho(t1)));
        Vector3 d0 = Vector3Add(e0, Vector3Scale(lado, Ancho(t0)));
        Vector3 d1 = Vector3Add(e1, Vector3Scale(lado, Ancho(t1)));
        Color c0 = TonoP(MezclaP(abajo, arriba, t0), luzFrente);
        Color c1 = TonoP(MezclaP(abajo, arriba, t1), luzFrente);

        Quad4P(i0, n0, n1, i1, c0, c0, c1, c1);
        Quad4P(n0, d0, d1, n1, TonoP(c0, 0.8f), TonoP(c0, 0.8f), TonoP(c1, 0.8f), TonoP(c1, 0.8f));
    }
}


// Hoja plana (trebol, hojas caidas, petalos): elipse con nervio, inclinada.
// eje = direccion de la punta (en el plano de la hoja), normal = cara.
static void HojaP(Vector3 base, Vector3 eje, Vector3 normal, float largo, float ancho, Color color, float curva = 0.0f, bool nervio = true)
{
    Vector3 u = Vector3Normalize(eje);
    Vector3 n = Vector3Normalize(normal);
    Vector3 l = Vector3Normalize(Vector3CrossProduct(n, u));
    const int seg = 5;
    Color cara = CaraP(color, n);
    Color borde = TonoP(cara, 0.9f);
    Color centro = MezclaP(cara, COLOR_TINTE_SOL_P, 0.12f);

    for (int k = 0; k < seg; k++)
    {
        float t0 = (float)k / (float)seg, t1 = (float)(k + 1) / (float)seg;
        float w0 = ancho * std::sin(PI * t0) * (t0 < 0.5f ? 1.0f : 0.92f);
        float w1 = ancho * std::sin(PI * t1) * (t1 < 0.5f ? 1.0f : 0.92f);
        float c0 = curva * largo * (t0 * t0), c1 = curva * largo * (t1 * t1);
        Vector3 m0 = Vector3Add(Vector3Add(base, Vector3Scale(u, largo * t0)), Vector3Scale(n, c0));
        Vector3 m1 = Vector3Add(Vector3Add(base, Vector3Scale(u, largo * t1)), Vector3Scale(n, c1));
        Vector3 a0 = Vector3Add(m0, Vector3Add(Vector3Scale(l, w0), Vector3Scale(n, w0 * 0.25f)));
        Vector3 a1 = Vector3Add(m1, Vector3Add(Vector3Scale(l, w1), Vector3Scale(n, w1 * 0.25f)));
        Vector3 b0 = Vector3Add(m0, Vector3Add(Vector3Scale(l, -w0), Vector3Scale(n, w0 * 0.25f)));
        Vector3 b1 = Vector3Add(m1, Vector3Add(Vector3Scale(l, -w1), Vector3Scale(n, w1 * 0.25f)));

        Quad4P(m0, a0, a1, m1, centro, borde, borde, centro);
        Quad4P(b0, m0, m1, b1, TonoP(borde, 0.9f), TonoP(centro, 0.92f), TonoP(centro, 0.92f), TonoP(borde, 0.9f));

        if (nervio)
        {
            Vector3 e = Vector3Scale(n, 0.012f);
            Vector3 g0 = Vector3Scale(l, ancho * 0.05f), g1 = Vector3Scale(l, ancho * 0.04f);
            QuadP(Vector3Add(Vector3Subtract(m0, g0), e), Vector3Add(Vector3Add(m0, g0), e),
                Vector3Add(Vector3Add(m1, g1), e), Vector3Add(Vector3Subtract(m1, g1), e), TonoP(cara, 1.12f));
        }
    }
}


//--------------------------------------------------
// Piezas GLB jugables
//--------------------------------------------------
// Del paquete solo se cargan las piezas que muestran el estado del juego:
// brote, flor y aro de la flor; madriguera y aro de aviso; plagas y
// avispas. El jardin se construye con primitivas propias horneadas.

static_assert(MODELO_PISOTON_BROTE + 2 == MODELO_PISOTON_ARO_FLOR &&
    MODELO_PISOTON_MADRIGUERA + 7 == MODELO_PISOTON_ALA,
    "Flor y madriguera..ala deben ir seguidas en el paquete Pisoton de Plagas");

void CargarPiezasJugablesPisotonPlagas()
{
    // Mismas primitives por GLB que exige CargarPaquetePisotonPlagasRetro3D.
    static const int mallasEsperadas[CANTIDAD_MODELOS_PISOTON_PLAGAS_3D] =
        { 5,2,5,4,5,2,4,2,3,3,2,3,3,3,1,5,5,2,5,3,2,2 };

    PaqueteModelosEscenarioRetro3D paquete = ObtenerPaquetePisotonPlagasRetro3D();
    const int desde[2] = { MODELO_PISOTON_BROTE, MODELO_PISOTON_MADRIGUERA };
    const int hasta[2] = { MODELO_PISOTON_ARO_FLOR, MODELO_PISOTON_ALA };

    for (int t = 0; t < 2; t++)
    {
        PaqueteModelosEscenarioRetro3D tramo = paquete;
        tramo.recursos += desde[t];
        tramo.definiciones += desde[t];
        tramo.cantidad = hasta[t] - desde[t] + 1;
        CargarPaqueteModelosEscenarioRetro3D(tramo);

        for (int i = desde[t]; i <= hasta[t]; i++)
        {
            RecursoModeloEscenarioRetro3D& r = paquete.recursos[i];
            if (!r.cargado || r.modelo.meshCount == mallasEsperadas[i]) continue;
            UnloadModel(r.modelo);
            r.modelo = {};
            r.cargado = false;
            TraceLog(LOG_WARNING, "Paquete Pisoton de Plagas incompatible; se usan primitivas: %s", r.ruta);
        }
    }

    // Reposos CPU de las mallas que se deforman (patas, cola y petalos),
    // igual que el cargador del paquete completo.
    const int piezas[4] = { MODELO_PISOTON_ESCARABAJO, MODELO_PISOTON_BABOSA,
        MODELO_PISOTON_FLOR_ABIERTA, MODELO_PISOTON_FLOR_ABIERTA };
    const int mallas[4] = { 3, 0, 1, 2 };
    const int vertices[4] = { 876, 618, 1152, 1152 };

    for (int i = 0; i < 4; i++)
    {
        RecursoModeloEscenarioRetro3D& r = paquete.recursos[piezas[i]];
        ReposoMallaPisotonRetro3D& reposo = ObtenerModelosEscenariosRetro3D().animacionesPisoton[i];

        if (!r.cargado || reposo.vertices) continue;

        Mesh& malla = r.modelo.meshes[mallas[i]];
        bool valido = malla.vertexCount == vertices[i] && malla.normals;
        const int bytes = malla.vertexCount * 3 * (int)sizeof(float);

        if (valido)
        {
            reposo.vertices = static_cast<float*>(MemAlloc((unsigned int)bytes));
            reposo.normales = static_cast<float*>(MemAlloc((unsigned int)bytes));
            valido = reposo.vertices && reposo.normales;
        }

        if (valido)
        {
            std::memcpy(reposo.vertices, malla.vertices, (size_t)bytes);
            std::memcpy(reposo.normales, malla.normals, (size_t)bytes);
        }
        else
        {
            MemFree(reposo.vertices);
            MemFree(reposo.normales);
            reposo = {};
            UnloadModel(r.modelo);
            r.modelo = {};
            r.cargado = false;
            TraceLog(LOG_WARNING, "No se pudo preparar la animacion de Pisoton de Plagas: %s", r.ruta);
        }
    }
}


//--------------------------------------------------
// Horno: lo estatico se graba una vez en dos mallas
//--------------------------------------------------

struct HornoPisoton
{
    bool listo = false;
    int clave = -1;
    Mesh suelo{};
    bool sueloValido = false;
    Mesh escena{};
    bool escenaValida = false;
    bool materialListo = false;
    Material material{};
};

static HornoPisoton g_hornoP;


static void LiberarMallasPisoton()
{
    LiberarMallaHorneada(g_hornoP.suelo, g_hornoP.sueloValido);
    LiberarMallaHorneada(g_hornoP.escena, g_hornoP.escenaValida);
    g_hornoP.listo = false;
    g_hornoP.clave = -1;
}


static void SueloEstaticoP(bool decoracion, bool sombras);
static void EscenaEstaticaP(bool decoracion, int paso);


static int ClaveHornoP()
{
    return (int)CalidadDecoracion() * 2 + (CalidadSombras() != CALIDAD_BAJA ? 1 : 0);
}


static void HornearPisoton()
{
    int clave = ClaveHornoP();

    if ((g_hornoP.listo && g_hornoP.clave == clave) || !IsWindowReady())
    {
        return;
    }

    LiberarMallasPisoton();

    AsegurarMaterialHorneado(g_hornoP.material, g_hornoP.materialListo);

    bool decoracion = CalidadDecoracion() != CALIDAD_BAJA;
    bool sombras = CalidadSombras() != CALIDAD_BAJA;
    int paso = CalidadDecoracion() == CALIDAD_ALTA ? 1 : 2;
    Vector3 ojo = g_ojoP;
    g_ojoP = OJO_PISOTON;

    EmpezarGrabacionMalla(g_grabP);
    SueloEstaticoP(decoracion, sombras);
    g_hornoP.sueloValido = TerminarGrabacionMalla(g_grabP, g_hornoP.suelo);

    EmpezarGrabacionMalla(g_grabP);
    EscenaEstaticaP(decoracion, paso);
    g_hornoP.escenaValida = TerminarGrabacionMalla(g_grabP, g_hornoP.escena);

    g_ojoP = ojo;
    g_hornoP.clave = clave;
    g_hornoP.listo = true;
}


// El suelo va sin prueba de profundidad: tierra, musgo, grietas y sombras
// se pintan en orden, sin parpadeos entre capas coplanares.
static void DibujarHornoSueloP()
{
    if (!g_hornoP.listo || !g_hornoP.sueloValido) return;

    rlDrawRenderBatchActive();
    rlDisableDepthTest();
    DrawMesh(g_hornoP.suelo, g_hornoP.material, MatrixIdentity());
    rlDrawRenderBatchActive();
    rlEnableDepthTest();
}


static void DibujarHornoEscenaP()
{
    if (!g_hornoP.listo || !g_hornoP.escenaValida) return;

    rlDrawRenderBatchActive();
    DrawMesh(g_hornoP.escena, g_hornoP.material, MatrixIdentity());
}


//--------------------------------------------------
// Suelo: claro de tierra con musgo, cesped espeso afuera
//--------------------------------------------------

static bool DentroArenaP(float x, float z, float margen = 0.0f)
{
    return std::fabs(x) <= BORDE_X_PISOTON + margen && std::fabs(z) <= BORDE_Z_PISOTON + margen;
}


static Color ColorTierraArenaP(float x, float z)
{
    float n1 = RuidoSuaveP(x, z, 2.4f, 11);
    float n2 = RuidoSuaveP(x, z, 0.7f, 12);
    Color c = MezclaP(COLOR_TIERRA_P, COLOR_MUSGO_P, Lim01P((n1 - 0.42f) * 2.4f));
    c = TonoP(c, 0.9f + 0.18f * n2);

    // El borde se oscurece un poco: la sombra de los guijarros y del pasto.
    float dx = BORDE_X_PISOTON - std::fabs(x), dz = BORDE_Z_PISOTON - std::fabs(z);
    float borde = Lim01P(1.0f - (dx < dz ? dx : dz) / 0.9f);
    return TonoP(c, 1.0f - 0.16f * borde);
}


static Color ColorCespedP(float x, float z)
{
    float n1 = RuidoSuaveP(x, z, 3.5f, 21);
    float n2 = RuidoSuaveP(x, z, 1.1f, 22);
    Color c = MezclaP(COLOR_PASTO_OSCURO_P, COLOR_PASTO_P, 0.25f + 0.5f * n1);
    return TonoP(c, 0.9f + 0.16f * n2);
}


static void GrietaP(float x, float z, int semilla)
{
    float a = Ruido01P(semilla, 31) * 2.0f * PI;
    float px = x, pz = z;

    for (int k = 0; k < 4; k++)
    {
        a += (Ruido01P(semilla * 5 + k, 32) - 0.5f) * 1.4f;
        float l = 0.35f + 0.35f * Ruido01P(semilla * 5 + k, 33);
        float nx = px + std::cos(a) * l, nz = pz + std::sin(a) * l;
        float w = 0.035f * (1.0f - (float)k * 0.2f);
        Vector3 d = { -std::sin(a) * w, 0.0f, std::cos(a) * w };
        Color c = AlfaP(COLOR_TIERRA_OSCURA_P, 0.75f);
        QuadP({ px - d.x, SUELO_PISOTON, pz - d.z }, { px + d.x, SUELO_PISOTON, pz + d.z },
            { nx + d.x * 0.7f, SUELO_PISOTON, nz + d.z * 0.7f }, { nx - d.x * 0.7f, SUELO_PISOTON, nz - d.z * 0.7f }, c);
        px = nx;
        pz = nz;
    }
}


static void SueloEstaticoP(bool decoracion, bool sombras)
{
    const float y = SUELO_PISOTON;

    // Cesped lejano (celdas grandes) hasta el seto del fondo.
    for (float z = -26.0f; z < 14.0f; z += 2.0f)
    {
        for (float x = -44.0f; x < 44.0f; x += 2.0f)
        {
            Quad4P({ x, y, z }, { x, y, z + 2.0f }, { x + 2.0f, y, z + 2.0f }, { x + 2.0f, y, z },
                ColorCespedP(x, z), ColorCespedP(x, z + 2.0f), ColorCespedP(x + 2.0f, z + 2.0f), ColorCespedP(x + 2.0f, z));
        }
    }

    // Claro de la arena: reja fina con color por vertice (tierra y musgo).
    const float paso = 0.45f;
    const float x0 = -BORDE_X_PISOTON - 0.3f, x1 = BORDE_X_PISOTON + 0.3f;
    const float z0 = -BORDE_Z_PISOTON - 0.3f, z1 = BORDE_Z_PISOTON + 0.3f;

    for (float z = z0; z < z1 - 0.001f; z += paso)
    {
        float zb = z + paso < z1 ? z + paso : z1;

        for (float x = x0; x < x1 - 0.001f; x += paso)
        {
            float xb = x + paso < x1 ? x + paso : x1;
            Quad4P({ x, y, z }, { x, y, zb }, { xb, y, zb }, { xb, y, z },
                ColorTierraArenaP(x, z), ColorTierraArenaP(x, zb), ColorTierraArenaP(xb, zb), ColorTierraArenaP(xb, z));
        }
    }

    // Manchas de musgo claro y de tierra mojada por el rocio.
    int manchas = decoracion ? 16 : 8;

    for (int k = 0; k < manchas; k++)
    {
        float x = (Ruido01P(k, 41) * 2.0f - 1.0f) * (LIMITE_X_PISOTON - 0.6f);
        float z = (Ruido01P(k, 42) * 2.0f - 1.0f) * (LIMITE_Z_PISOTON - 0.6f);
        float r = 0.5f + 0.8f * Ruido01P(k, 43);
        Color c = k % 3 == 0 ? TonoP(COLOR_TIERRA_P, 0.82f) : MezclaP(COLOR_MUSGO_P, COLOR_PASTO_CLARO_P, 0.25f);
        ManchaSueloP({ x, y, z }, r, 9, AlfaP(c, 0.75f), AlfaP(c, 0.0f), 400 + k);
    }

    // Tierra removida alrededor de cada madriguera.
    for (int k = 0; k < MAX_MADRIGUERAS_PISOTON; k++)
    {
        Vector3 c = { MADRIGUERAS_X_PISOTON[k], y, MADRIGUERAS_Z_PISOTON[k] };
        ManchaSueloP(c, 1.25f, 12, AlfaP(COLOR_TIERRA_OSCURA_P, 0.85f), AlfaP(COLOR_TIERRA_OSCURA_P, 0.0f), 500 + k);
        ManchaSueloP(c, 0.95f, 10, AlfaP(TonoP(COLOR_TIERRA_P, 0.86f), 0.9f), AlfaP(COLOR_TIERRA_P, 0.0f), 520 + k);
    }

    if (decoracion)
    {
        for (int k = 0; k < 14; k++)
        {
            float x = (Ruido01P(k, 51) * 2.0f - 1.0f) * (LIMITE_X_PISOTON - 0.5f);
            float z = (Ruido01P(k, 52) * 2.0f - 1.0f) * (LIMITE_Z_PISOTON - 0.5f);
            GrietaP(x, z, k);
        }

        // Petalos caidos (blancos y lilas) chatos sobre la tierra.
        for (int k = 0; k < 12; k++)
        {
            float x = (Ruido01P(k, 61) * 2.0f - 1.0f) * (LIMITE_X_PISOTON - 0.4f);
            float z = (Ruido01P(k, 62) * 2.0f - 1.0f) * (LIMITE_Z_PISOTON - 0.4f);
            Color c = k % 3 == 0 ? Color{ 214, 196, 236, 255 } : Color{ 246, 242, 230, 255 };
            DiscoSueloP({ x, y, z }, 0.26f, 0.12f, 7, c, TonoP(c, 0.86f), Ruido01P(k, 63) * PI);
        }
    }

    // Sombra del pasto alto del fondo y de los costados (el sol entra por
    // atras y a la derecha: la sombra se estira hacia la camara).
    if (sombras)
    {
        Color s0 = Color{ 20, 34, 18, 110 }, s1 = Color{ 20, 34, 18, 0 };
        QuadColoresP({ -BORDE_X_PISOTON - 3.0f, y, -BORDE_Z_PISOTON - 0.4f }, { -BORDE_X_PISOTON - 3.0f, y, -BORDE_Z_PISOTON + 1.6f },
            { BORDE_X_PISOTON + 3.0f, y, -BORDE_Z_PISOTON + 1.6f }, { BORDE_X_PISOTON + 3.0f, y, -BORDE_Z_PISOTON - 0.4f }, s0, s1, s1, s0);
        QuadColoresP({ -BORDE_X_PISOTON - 0.4f, y, -BORDE_Z_PISOTON }, { -BORDE_X_PISOTON - 0.4f, y, BORDE_Z_PISOTON },
            { -BORDE_X_PISOTON + 1.2f, y, BORDE_Z_PISOTON }, { -BORDE_X_PISOTON + 1.2f, y, -BORDE_Z_PISOTON }, s0, s0, s1, s1);
    }
}


//--------------------------------------------------
// Utileria del jardin (todo con primitivas)
//--------------------------------------------------

// Gotas de rocio grabadas: el brillo que titila se agrega por frame.
static const int MAX_ROCIO_P = 48;
static Vector3 g_rocioP[MAX_ROCIO_P];
static float g_rocioRadioP[MAX_ROCIO_P];
static int g_cantidadRocioP = 0;


static void GotaRocioP(Vector3 c, float r)
{
    BolaP({ c.x, c.y + r * 0.7f, c.z }, { r, r * 0.8f, r }, Color{ 168, 214, 236, 255 }, 3, 7);
    BolaP({ c.x - r * 0.3f, c.y + r * 1.05f, c.z + r * 0.35f }, { r * 0.3f, r * 0.24f, r * 0.3f }, Color{ 255, 255, 255, 255 }, 2, 5);

    if (g_cantidadRocioP < MAX_ROCIO_P)
    {
        g_rocioP[g_cantidadRocioP] = { c.x - r * 0.3f, c.y + r * 1.05f, c.z + r * 0.35f };
        g_rocioRadioP[g_cantidadRocioP] = r;
        g_cantidadRocioP++;
    }
}


// Elipse con ejes arbitrarios (nudos de la madera, agujeros, motas).
static void ElipseP(Vector3 c, Vector3 u, Vector3 v, int lados, Color centro, Color borde)
{
    for (int k = 0; k < lados; k++)
    {
        float a0 = 2.0f * PI * (float)k / (float)lados;
        float a1 = 2.0f * PI * (float)(k + 1) / (float)lados;
        Vector3 p0 = Vector3Add(c, Vector3Add(Vector3Scale(u, std::cos(a0)), Vector3Scale(v, std::sin(a0))));
        Vector3 p1 = Vector3Add(c, Vector3Add(Vector3Scale(u, std::cos(a1)), Vector3Scale(v, std::sin(a1))));
        VP(c, NieblaP(centro, c)); VP(p0, NieblaP(borde, p0)); VP(p1, NieblaP(borde, p1));
    }
}


static void GuijarroP(float x, float z, float r, int semilla, float altura = 0.55f)
{
    float h = Ruido01P(semilla, 71);
    Color c = MezclaP(COLOR_PIEDRA_P, Color{ 176, 160, 132, 255 }, Ruido01P(semilla, 72));
    c = VariarP(c, semilla, 73, 0.1f);
    float rx = r * (0.85f + 0.35f * h), rz = r * (0.8f + 0.35f * Ruido01P(semilla, 74));
    BolaP({ x, SUELO_PISOTON + r * altura * 0.55f, z }, { rx, r * altura, rz }, c, 3, 7);

    // Algunos con musgo encima.
    if (Ruido01P(semilla, 75) < 0.22f)
    {
        BolaP({ x - rx * 0.15f, SUELO_PISOTON + r * altura * 1.25f, z - rz * 0.1f }, { rx * 0.6f, r * altura * 0.25f, rz * 0.55f }, COLOR_MUSGO_P, 2, 6);
    }
}


static void BordeGuijarrosP(bool decoracion)
{
    // Lomo de tierra bajo los guijarros: el limite tiene grosor.
    Color lomo = TonoP(COLOR_TIERRA_OSCURA_P, 1.1f);
    CajaP({ 0.0f, SUELO_PISOTON + 0.06f, -BORDE_Z_PISOTON - 0.15f }, { BORDE_X_PISOTON * 2.0f + 1.4f, 0.12f, 0.75f }, lomo);
    CajaP({ 0.0f, SUELO_PISOTON + 0.04f, BORDE_Z_PISOTON + 0.15f }, { BORDE_X_PISOTON * 2.0f + 1.4f, 0.08f, 0.6f }, lomo);
    CajaP({ -BORDE_X_PISOTON - 0.15f, SUELO_PISOTON + 0.06f, 0.0f }, { 0.75f, 0.12f, BORDE_Z_PISOTON * 2.0f + 0.6f }, lomo);
    CajaP({ BORDE_X_PISOTON + 0.15f, SUELO_PISOTON + 0.06f, 0.0f }, { 0.75f, 0.12f, BORDE_Z_PISOTON * 2.0f + 0.6f }, lomo);

    int semilla = 1000;
    float paso = decoracion ? 0.4f : 0.55f;

    // Atras y costados: guijarros mas grandes; adelante bajitos.
    for (int lado = 0; lado < 4; lado++)
    {
        bool horizontal = lado < 2;
        float largo = horizontal ? BORDE_X_PISOTON + 0.4f : BORDE_Z_PISOTON;
        float frente = lado == 1;

        for (float t = -largo; t <= largo; t += paso)
        {
            semilla++;
            float j = (Ruido01P(semilla, 76) - 0.5f) * 0.18f;
            float r = (frente ? 0.17f : 0.22f) + (frente ? 0.07f : 0.12f) * Ruido01P(semilla, 77);
            float x = horizontal ? t + j : (lado == 2 ? -BORDE_X_PISOTON - 0.1f + j : BORDE_X_PISOTON + 0.1f + j);
            float z = horizontal ? (lado == 0 ? -BORDE_Z_PISOTON - 0.1f + j : BORDE_Z_PISOTON + 0.1f + j) : t + j;
            GuijarroP(x, z, r, semilla, frente ? 0.45f : 0.6f);
        }
    }
}


static void SetaP(Vector3 base, float alto, float radio, Color sombrero, int semilla, bool lunares = true)
{
    Color crema = Color{ 238, 228, 206, 255 };
    Vector3 tope = { base.x, base.y + alto, base.z };
    CilP(base, tope, radio * 0.26f, radio * 0.2f, 8, crema, false);
    CilP({ base.x, base.y + alto * 0.68f, base.z }, { base.x, base.y + alto * 0.62f, base.z }, radio * 0.24f, radio * 0.36f, 8, TonoP(crema, 0.94f), false);

    // Laminas bajo el sombrero y sombrero en cupula.
    DiscoSueloP(tope, radio * 0.97f, radio * 0.97f, 12, TonoP(crema, 0.7f), TonoP(crema, 0.82f));
    BolaP(tope, { radio, radio * 0.58f, radio }, sombrero, 3, 12, 0.0f);

    if (!lunares) return;

    for (int k = 0; k < 9; k++)
    {
        float lo = 2.0f * PI * Ruido01P(semilla * 9 + k, 81);
        float la = 0.25f + 1.05f * Ruido01P(semilla * 9 + k, 82);
        float r = radio * (0.09f + 0.06f * Ruido01P(semilla * 9 + k, 83));
        Vector3 p = { tope.x + std::cos(la) * std::cos(lo) * radio * 0.98f, tope.y + std::sin(la) * radio * 0.58f * 0.98f, tope.z + std::cos(la) * std::sin(lo) * radio * 0.98f };
        BolaP(p, { r, r * 0.45f, r }, Color{ 252, 248, 238, 255 }, 2, 6);
    }
}


// Chapa de botella tirada boca abajo: dientes plegados y aro pintado.
static void ChapaP(Vector3 c, float r)
{
    Color rojo = Color{ 196, 46, 44, 255 };
    float alto = r * 0.28f;
    CilP(c, { c.x, c.y + alto, c.z }, r, r * 0.97f, 22, rojo);

    for (int k = 0; k < 21; k++)
    {
        float a = 2.0f * PI * (float)k / 21.0f;
        Vector3 p = { c.x + std::cos(a) * (r + 0.05f), c.y + alto * 0.4f, c.z + std::sin(a) * (r + 0.05f) };
        CajaGiradaP(p, { r * 0.2f, alto * 0.8f, r * 0.1f }, 90.0f - a * RAD2DEG, TonoP(rojo, 0.92f), -18.0f);
    }

    // Aro blanco y circulo dorado impresos arriba.
    Vector3 t = { c.x, c.y + alto + 0.01f, c.z };
    for (int k = 0; k < 22; k++)
    {
        float a0 = 2.0f * PI * (float)k / 22.0f, a1 = 2.0f * PI * (float)(k + 1) / 22.0f;
        float r0 = r * 0.62f, r1 = r * 0.74f;
        QuadP({ t.x + std::cos(a0) * r0, t.y, t.z + std::sin(a0) * r0 }, { t.x + std::cos(a0) * r1, t.y, t.z + std::sin(a0) * r1 },
            { t.x + std::cos(a1) * r1, t.y, t.z + std::sin(a1) * r1 }, { t.x + std::cos(a1) * r0, t.y, t.z + std::sin(a1) * r0 }, Color{ 248, 240, 226, 255 });
    }
    DiscoSueloP({ t.x, t.y + 0.005f, t.z }, r * 0.42f, r * 0.42f, 14, Color{ 240, 200, 90, 255 }, Color{ 214, 168, 70, 255 });
}


// Bellota acostada a lo largo de X (o de Z).
static void BellotaP(Vector3 c, float s, bool largoX)
{
    Color cuerpo = Color{ 168, 120, 62, 255 };
    Color cupula = Color{ 118, 94, 64, 255 };
    Vector3 rc = largoX ? Vector3{ 0.56f * s, 0.4f * s, 0.4f * s } : Vector3{ 0.4f * s, 0.4f * s, 0.56f * s };
    Vector3 eje = largoX ? Vector3{ 1.0f, 0.0f, 0.0f } : Vector3{ 0.0f, 0.0f, 1.0f };
    Vector3 centro = { c.x, c.y + 0.38f * s, c.z };
    BolaP(centro, rc, cuerpo, 4, 10);
    BolaP(Vector3Add(centro, Vector3Scale(eje, 0.52f * s)), { 0.1f * s, 0.1f * s, 0.1f * s }, TonoP(cuerpo, 0.7f), 2, 6);

    Vector3 rcu = largoX ? Vector3{ 0.26f * s, 0.45f * s, 0.45f * s } : Vector3{ 0.45f * s, 0.45f * s, 0.26f * s };
    Vector3 pc = Vector3Add(centro, Vector3Scale(eje, -0.3f * s));
    BolaP(pc, rcu, cupula, 4, 10);

    // Escamas de la cupula: motas claras en anillos.
    for (int k = 0; k < 10; k++)
    {
        float a = 2.0f * PI * (float)k / 10.0f;
        Vector3 off = largoX ? Vector3{ -0.12f * s, std::sin(a) * 0.43f * s, std::cos(a) * 0.43f * s } : Vector3{ std::cos(a) * 0.43f * s, std::sin(a) * 0.43f * s, -0.12f * s };
        BolaP(Vector3Add(pc, off), { 0.07f * s, 0.07f * s, 0.07f * s }, TonoP(cupula, 1.25f), 2, 5);
    }

    CilP(Vector3Add(pc, Vector3Scale(eje, -0.24f * s)), Vector3Add(Vector3Add(pc, Vector3Scale(eje, -0.5f * s)), { 0.0f, 0.12f * s, 0.0f }), 0.06f * s, 0.05f * s, 5, TonoP(cupula, 0.8f));
}


static void TrebolP(Vector3 base, float alto, float giro, int semilla)
{
    Color tallo = Color{ 92, 150, 70, 255 };
    Color hoja = VariarP(Color{ 62, 132, 60, 255 }, semilla, 91, 0.08f);
    float inclX = (Ruido01P(semilla, 92) - 0.5f) * 0.3f * alto;
    float inclZ = (Ruido01P(semilla, 93) - 0.5f) * 0.3f * alto;
    Vector3 p0 = base;
    Vector3 p1 = { base.x + inclX * 0.3f, base.y + alto * 0.5f, base.z + inclZ * 0.3f };
    Vector3 p2 = { base.x + inclX, base.y + alto, base.z + inclZ };
    float rt = 0.05f + 0.025f * alto;
    CilP(p0, p1, rt, rt * 0.9f, 5, tallo, false);
    CilP(p1, p2, rt * 0.9f, rt * 0.8f, 5, tallo, false);

    float largo = 0.5f + 0.2f * alto;

    for (int k = 0; k < 3; k++)
    {
        float a = (giro + (float)k * 120.0f) * DEG2RAD;

        // Cada foliolo es un corazon: dos lobulos y una V clara.
        for (int l = -1; l <= 1; l += 2)
        {
            float al = a + (float)l * 0.32f;
            Vector3 eje = Vector3Normalize({ std::cos(al), 0.28f, std::sin(al) });
            Vector3 normal = Vector3Normalize({ -std::cos(al) * 0.3f, 1.0f, -std::sin(al) * 0.3f });
            HojaP(p2, eje, normal, largo, largo * 0.36f, hoja, 0.05f, false);
        }

        Vector3 eje = Vector3Normalize({ std::cos(a), 0.3f, std::sin(a) });
        Vector3 normal = Vector3Normalize({ -std::cos(a) * 0.3f, 1.0f, -std::sin(a) * 0.3f });
        HojaP(Vector3Add(p2, Vector3Scale(normal, 0.03f)), eje, normal, largo * 0.55f, largo * 0.18f, MezclaP(hoja, Color{ 200, 230, 170, 255 }, 0.45f), 0.0f, false);
    }

    if (Ruido01P(semilla, 94) < 0.6f)
    {
        float a = (giro + 40.0f) * DEG2RAD;
        GotaRocioP({ p2.x + std::cos(a) * largo * 0.5f, p2.y + largo * 0.12f, p2.z + std::sin(a) * largo * 0.5f }, 0.07f + 0.03f * alto);
    }
}


static void HojaCaidaP(Vector3 base, float giro, float largo, float ancho, Color color, float levante = 0.0f)
{
    float a = giro * DEG2RAD;
    Vector3 eje = Vector3Normalize({ std::cos(a), levante, std::sin(a) });
    Vector3 normal = Vector3Normalize({ -std::cos(a) * levante, 1.0f, -std::sin(a) * levante });
    HojaP({ base.x, base.y + 0.05f, base.z }, eje, normal, largo, ancho, color, 0.1f, true);

    // Nervaduras laterales.
    Vector3 l = Vector3Normalize(Vector3CrossProduct(normal, eje));
    for (int k = 1; k < 5; k++)
    {
        float t = (float)k / 5.0f;
        Vector3 m = Vector3Add({ base.x, base.y + 0.07f + 0.1f * largo * t * t, base.z }, Vector3Scale(eje, largo * t));
        for (int s = -1; s <= 1; s += 2)
        {
            Vector3 f = Vector3Add(m, Vector3Add(Vector3Scale(l, (float)s * ancho * 0.7f * std::sin(PI * t)), Vector3Scale(eje, largo * 0.1f)));
            VigaP(m, Vector3Add(f, Vector3Scale(normal, 0.02f)), 0.025f, TonoP(color, 1.12f));
        }
    }
}


static void RamitaP(Vector3 a, Vector3 b, float r)
{
    Color corteza = Color{ 120, 92, 64, 255 };
    CilP(a, b, r, r * 0.7f, 6, corteza);
    Vector3 m = Vector3Lerp(a, b, 0.55f);
    Vector3 d = Vector3Subtract(b, a);
    Vector3 rama = Vector3Add(m, { -d.z * 0.3f, 0.35f, d.x * 0.3f });
    CilP(m, rama, r * 0.6f, r * 0.35f, 5, corteza);
    BolaP(b, { r * 0.75f, r * 0.75f, r * 0.75f }, TonoP(corteza, 1.3f), 2, 6);
}


static void MargaritaP(Vector3 base, float alto, float radio, int semilla)
{
    Color tallo = Color{ 86, 146, 66, 255 };
    Vector3 h = { base.x, base.y + alto, base.z + 0.1f * alto };
    Vector3 m = { base.x + 0.12f * alto * (Ruido01P(semilla, 101) - 0.5f), base.y + alto * 0.5f, base.z };
    CilP(base, m, 0.13f + 0.02f * alto, 0.12f + 0.015f * alto, 6, tallo, false);
    CilP(m, h, 0.12f + 0.015f * alto, 0.1f + 0.01f * alto, 6, tallo, false);

    for (int s = -1; s <= 1; s += 2)
    {
        HojaP({ base.x, base.y + 0.1f, base.z }, Vector3Normalize({ (float)s * 0.8f, 0.9f, 0.25f }), Vector3Normalize({ -(float)s * 0.6f, 0.3f, 0.75f }), alto * 0.35f, alto * 0.07f, TonoP(tallo, 0.9f), -0.12f, true);
    }

    // Cara inclinada hacia la camara y hacia arriba.
    Vector3 n = Vector3Normalize({ 0.0f, 0.78f, 0.62f });
    Vector3 l1 = { 1.0f, 0.0f, 0.0f };
    Vector3 l2 = Vector3Normalize(Vector3CrossProduct(n, l1));
    int petalos = 16;

    for (int k = 0; k < petalos; k++)
    {
        float a = 2.0f * PI * ((float)k + 0.5f * Ruido01P(semilla * 31 + k, 102)) / (float)petalos;
        Vector3 eje = Vector3Add(Vector3Scale(l1, std::cos(a)), Vector3Scale(l2, std::sin(a)));
        Color blanco = VariarP(Color{ 250, 248, 240, 255 }, semilla * 31 + k, 103, 0.04f);
        HojaP(Vector3Add(h, Vector3Scale(n, 0.02f * (float)(k % 2))), eje, n, radio, radio * 0.17f, blanco, -0.06f, false);
    }

    BolaP(Vector3Add(h, Vector3Scale(n, 0.08f)), { radio * 0.33f, radio * 0.2f, radio * 0.33f }, Color{ 250, 196, 44, 255 }, 3, 10);
}


static void TulipanP(Vector3 base, float alto, float radio, Color color)
{
    Color tallo = Color{ 92, 150, 72, 255 };
    Vector3 tope = { base.x, base.y + alto, base.z };
    CilP(base, tope, 0.13f + 0.015f * alto, 0.1f, 6, tallo, false);

    for (int s = -1; s <= 1; s += 2)
    {
        HojaP({ base.x, base.y + 0.1f, base.z }, Vector3Normalize({ (float)s * 0.35f, 1.0f, 0.15f }), Vector3Normalize({ -(float)s, 0.2f, 0.4f }), alto * 0.6f, radio * 0.45f, TonoP(tallo, 0.92f), -0.1f, true);
    }

    for (int k = 0; k < 6; k++)
    {
        float a = 2.0f * PI * (float)k / 6.0f + (k % 2 ? 0.3f : 0.0f);
        Vector3 eje = Vector3Normalize({ std::cos(a) * 0.32f, 1.0f, std::sin(a) * 0.32f });
        Vector3 normal = Vector3Normalize({ std::cos(a), -0.3f, std::sin(a) });
        float r = k % 2 ? radio * 0.92f : radio;
        HojaP(Vector3Add(tope, { std::cos(a) * radio * 0.18f, -0.1f, std::sin(a) * radio * 0.18f }), eje, normal, r * 1.55f, r * 0.62f, k % 2 ? TonoP(color, 0.9f) : color, -0.22f, false);
    }
}


static void DienteLeonP(Vector3 base, float alto, float radio, int semilla)
{
    Color tallo = Color{ 140, 168, 104, 255 };
    Vector3 m = { base.x + 0.25f, base.y + alto * 0.55f, base.z };
    Vector3 c = { base.x + 0.1f, base.y + alto, base.z };
    CilP(base, m, 0.1f, 0.09f, 6, tallo, false);
    CilP(m, c, 0.09f, 0.08f, 6, tallo, false);

    // Esfera de vilanos: nucleo y cientos de penachos (motas claras).
    BolaP(c, { radio * 0.25f, radio * 0.25f, radio * 0.25f }, Color{ 150, 128, 90, 255 }, 2, 6);
    BolaP(c, { radio * 0.86f, radio * 0.86f, radio * 0.86f }, Color{ 234, 236, 228, 255 }, 5, 12);

    for (int k = 0; k < 34; k++)
    {
        float lo = 2.0f * PI * Ruido01P(semilla * 40 + k, 111);
        float la = std::asin(Ruido01P(semilla * 40 + k, 112) * 2.0f - 1.0f);
        Vector3 d = { std::cos(la) * std::cos(lo), std::sin(la), std::cos(la) * std::sin(lo) };
        BolaP(Vector3Add(c, Vector3Scale(d, radio * 0.95f)), { radio * 0.1f, radio * 0.1f, radio * 0.1f }, Color{ 250, 250, 246, 255 }, 2, 5);
    }
}


// Regadera de chapa verde, gigante, olvidada contra la cerca.
static const Vector3 BASE_REGADERA_P = { -18.0f, 0.0f, -16.5f };
static const Vector3 ROSETA_REGADERA_P = { -8.6f, 4.3f, -16.5f };

static void RegaderaP(bool decoracion)
{
    Vector3 b = BASE_REGADERA_P;
    Color chapa = Color{ 74, 150, 118, 255 };
    Color oscura = TonoP(chapa, 0.72f);
    float r = 3.2f, alto = 5.0f;

    CilP(b, { b.x, b.y + alto, b.z }, r, r * 0.94f, 20, chapa, false);
    CilP({ b.x, b.y + 0.1f, b.z }, { b.x, b.y + 0.55f, b.z }, r + 0.08f, r + 0.08f, 20, oscura, false);
    CilP({ b.x, b.y + alto - 0.4f, b.z }, { b.x, b.y + alto, b.z }, r * 0.94f + 0.08f, r * 0.94f + 0.08f, 20, oscura, false);
    BolaP({ b.x, b.y + alto, b.z }, { r * 0.94f, 1.1f, r * 0.94f }, TonoP(chapa, 1.05f), 3, 20, 0.0f);

    // Boca de llenado y asa (el asa sale del cuadro: es enorme).
    CilP({ b.x - 1.2f, b.y + alto + 0.7f, b.z - 0.6f }, { b.x - 1.3f, b.y + alto + 1.5f, b.z - 0.6f }, 0.9f, 1.0f, 12, oscura);
    Vector3 asa[5] = { { b.x - 2.5f, b.y + alto - 0.2f, b.z }, { b.x - 3.3f, b.y + alto + 2.4f, b.z }, { b.x - 1.8f, b.y + alto + 4.6f, b.z }, { b.x + 0.8f, b.y + alto + 4.9f, b.z }, { b.x + 2.2f, b.y + alto + 1.2f, b.z } };
    for (int k = 0; k < 4; k++) CilP(asa[k], asa[k + 1], 0.32f, 0.32f, 8, chapa);

    // Pico largo y roseta (de ahi caen las gotas).
    Vector3 p0 = { b.x + r * 0.8f, b.y + 1.0f, b.z + 0.3f };
    Vector3 dir = Vector3Normalize(Vector3Subtract(ROSETA_REGADERA_P, p0));
    Vector3 p1 = Vector3Subtract(ROSETA_REGADERA_P, Vector3Scale(dir, 0.9f));
    CilP(p0, p1, 0.6f, 0.36f, 10, chapa);
    CilP(p1, ROSETA_REGADERA_P, 0.5f, 1.05f, 14, TonoP(chapa, 1.08f));
    ElipseP(Vector3Add(ROSETA_REGADERA_P, Vector3Scale(dir, 0.02f)), { 0.0f, 1.0f, 0.0f }, Vector3Normalize(Vector3CrossProduct(dir, { 0.0f, 1.0f, 0.0f })), 14, TonoP(chapa, 0.85f), TonoP(chapa, 0.85f));

    if (decoracion)
    {
        // Remaches y una abolladura iluminada.
        for (int k = 0; k < 12; k++)
        {
            float a = PI * 0.15f + PI * 0.7f * (float)k / 11.0f;
            BolaP({ b.x + std::cos(a) * (r + 0.06f), b.y + 0.33f, b.z + std::sin(a) * (r + 0.06f) }, { 0.1f, 0.1f, 0.1f }, TonoP(chapa, 1.2f), 2, 5);
            BolaP({ b.x + std::cos(a) * (r * 0.94f + 0.12f), b.y + alto - 0.2f, b.z + std::sin(a) * (r * 0.94f + 0.12f) }, { 0.1f, 0.1f, 0.1f }, TonoP(chapa, 1.2f), 2, 5);
        }

        ElipseP({ b.x + 1.3f, b.y + 2.7f, b.z + r * 0.96f }, { 0.7f, 0.0f, 0.0f }, { 0.0f, 0.5f, 0.0f }, 10, TonoP(chapa, 1.25f), TonoP(chapa, 1.0f));
    }
}


// Cerca de madera colosal: tablas gastadas, nudos y manchas de humedad.
static void CercaP(bool decoracion)
{
    const float z = -21.5f;
    int k = 0;

    for (float x = -46.0f; x < 5.0f; x += 2.45f, k++)
    {
        Color c = VariarP(MezclaP(COLOR_MADERA_P, Color{ 150, 126, 102, 255 }, Ruido01P(k, 121)), k, 122, 0.08f);
        CajaP({ x + 1.1f, 7.0f, z }, { 2.2f, 14.0f, 0.5f }, c);

        // Humedad abajo y vetas.
        Color mancha = AlfaP(TonoP(c, 0.62f), 0.8f);
        Quad4P({ x, 0.0f, z + 0.26f }, { x + 2.2f, 0.0f, z + 0.26f }, { x + 2.2f, 1.6f, z + 0.26f }, { x, 1.6f, z + 0.26f }, mancha, mancha, AlfaP(mancha, 0.0f), AlfaP(mancha, 0.0f));

        if (decoracion)
        {
            for (int v = 0; v < 2; v++)
            {
                float vx = x + 0.5f + 1.2f * Ruido01P(k * 3 + v, 123);
                QuadP({ vx, 0.4f, z + 0.27f }, { vx + 0.06f, 0.4f, z + 0.27f }, { vx + 0.1f, 9.0f, z + 0.27f }, { vx + 0.04f, 9.0f, z + 0.27f }, TonoP(c, 0.86f));
            }

            float ny = 2.0f + 3.0f * Ruido01P(k, 124);
            ElipseP({ x + 0.4f + 1.4f * Ruido01P(k, 125), ny, z + 0.28f }, { 0.22f, 0.0f, 0.0f }, { 0.0f, 0.32f, 0.0f }, 8, TonoP(c, 0.55f), TonoP(c, 0.75f));
        }
    }

    // Travesano de nuestro lado y poste de esquina; la cerca dobla hacia atras.
    CajaP({ -20.5f, 2.6f, z + 0.55f }, { 51.0f, 0.9f, 0.6f }, TonoP(COLOR_MADERA_P, 0.85f));
    CajaP({ 5.4f, 7.0f, z + 0.1f }, { 1.1f, 14.0f, 1.1f }, TonoP(COLOR_MADERA_P, 0.78f));

    Vector3 a = { 6.0f, 0.0f, z - 0.3f }, d = Vector3Normalize({ 0.42f, 0.0f, -1.0f });
    for (int j = 0; j < 7; j++)
    {
        Vector3 c = Vector3Add(a, Vector3Scale(d, 1.2f + (float)j * 2.45f));
        CajaGiradaP({ c.x, 7.0f, c.z }, { 0.5f, 14.0f, 2.2f }, std::atan2(d.x, d.z) * RAD2DEG, VariarP(COLOR_MADERA_P, j + 50, 122, 0.08f));
    }
}


// Seto de ligustro donde termina el cesped (a la derecha de la cerca).
static void SetoP(bool decoracion)
{
    int k = 0;

    for (float x = 6.0f; x < 50.0f; x += decoracion ? 1.7f : 2.6f, k++)
    {
        float r = 1.6f + 0.9f * Ruido01P(k, 131);
        float z = -22.8f - 0.8f * Ruido01P(k, 132);
        Color c = VariarP(Color{ 58, 112, 60, 255 }, k, 133, 0.1f);
        BolaP({ x, r * 0.55f, z }, { r * 1.1f, r * 0.95f, r }, c, 3, 8);
    }
}


static void MacetaP(bool decoracion)
{
    Vector3 b = { 17.5f, 0.0f, -15.0f };
    Color barro = Color{ 198, 108, 68, 255 };
    CilP(b, { b.x, 4.0f, b.z }, 2.9f, 3.5f, 18, barro, false);
    CilP({ b.x, 3.9f, b.z }, { b.x, 4.9f, b.z }, 3.85f, 3.9f, 18, TonoP(barro, 1.06f));
    DiscoSueloP({ b.x, 4.92f, b.z }, 3.4f, 3.4f, 18, COLOR_TIERRA_OSCURA_P, TonoP(COLOR_TIERRA_OSCURA_P, 0.8f));

    // Moho verde abajo y planta de hojas anchas que sale del cuadro.
    Color moho = AlfaP(COLOR_MUSGO_P, 0.7f);
    for (int k = 0; k < 18; k++)
    {
        float a0 = 2.0f * PI * (float)k / 18.0f, a1 = 2.0f * PI * (float)(k + 1) / 18.0f;
        Vector3 i0 = { b.x + std::cos(a0) * 2.93f, 0.0f, b.z + std::sin(a0) * 2.93f }, i1 = { b.x + std::cos(a1) * 2.93f, 0.0f, b.z + std::sin(a1) * 2.93f };
        Vector3 o0 = { b.x + std::cos(a0) * 3.1f, 0.9f, b.z + std::sin(a0) * 3.1f }, o1 = { b.x + std::cos(a1) * 3.1f, 0.9f, b.z + std::sin(a1) * 3.1f };
        Quad4P(i0, i1, o1, o0, moho, moho, AlfaP(moho, 0.0f), AlfaP(moho, 0.0f));
    }

    if (!decoracion) return;

    for (int k = 0; k < 6; k++)
    {
        float a = 2.0f * PI * (float)k / 6.0f + 0.4f;
        Vector3 eje = Vector3Normalize({ std::cos(a) * 0.45f, 1.0f, std::sin(a) * 0.45f });
        Vector3 normal = Vector3Normalize({ std::cos(a), -0.1f, std::sin(a) });
        HojaP({ b.x + std::cos(a) * 0.4f, 4.9f, b.z + std::sin(a) * 0.4f }, eje, normal, 6.0f, 1.3f, VariarP(Color{ 70, 146, 64, 255 }, k, 141, 0.1f), 0.12f, true);
    }
}


// Huecos de la utileria: el pasto no crece encima.
struct HuecoP { float x, z, r; };
static const HuecoP HUECOS_P[] =
{
    { -11.8f, -7.4f, 1.8f }, { -13.6f, -2.4f, 1.6f }, { -12.6f, 1.6f, 1.4f }, { -11.8f, 5.4f, 1.6f },
    { 12.2f, -4.6f, 1.4f }, { 11.6f, 4.4f, 1.4f }, { 12.9f, 1.0f, 1.3f }, { 13.6f, -2.2f, 1.4f },
    { -18.0f, -16.5f, 4.0f }, { 17.5f, -15.0f, 4.2f }, { -6.0f, -13.5f, 1.0f }, { 1.5f, -15.0f, 1.0f },
    { 5.5f, -13.0f, 1.0f }, { 8.0f, -16.0f, 1.0f }, { 12.5f, -12.0f, 1.0f }, { -9.5f, -15.0f, 1.0f },
};


static bool EnHuecoP(float x, float z)
{
    for (const HuecoP& h : HUECOS_P)
    {
        float dx = x - h.x, dz = z - h.z;
        if (dx * dx + dz * dz < h.r * h.r) return true;
    }

    return DentroArenaP(x, z, 0.35f);
}


// Bosque de briznas: anillo cercano, costados y media distancia.
static void PastoGiganteP(bool decoracion, int paso)
{
    int semilla = 2000;
    int intentos = decoracion ? (paso == 1 ? 520 : 300) : 170;

    for (int k = 0; k < intentos; k++)
    {
        semilla++;
        float u = Ruido01P(semilla, 151), v = Ruido01P(semilla, 152);
        float x = 0.0f, z = 0.0f, alto = 1.0f;
        int zona = k % 6;

        if (zona <= 1)          // atras, pegado al borde
        {
            x = -17.0f + 34.0f * u;
            z = -BORDE_Z_PISOTON - 0.5f - 4.5f * v;
            alto = 1.3f + 2.6f * Ruido01P(semilla, 153) + 0.25f * (-z - 6.0f);
        }
        else if (zona <= 3)     // costados
        {
            x = (k % 2 ? 1.0f : -1.0f) * (BORDE_X_PISOTON + 0.5f + 7.0f * u);
            z = -10.0f + 16.8f * v;
            alto = 0.9f + 2.4f * Ruido01P(semilla, 153);
            if (z > 2.0f) alto *= 0.6f;
        }
        else if (zona == 4)     // media distancia
        {
            if (!decoracion && k % 2) continue;
            x = -24.0f + 46.0f * u;
            z = -11.0f - 9.0f * v;
            alto = 3.5f + 3.5f * Ruido01P(semilla, 153);
        }
        else                    // adelante: matitas bajas
        {
            x = -13.0f + 26.0f * u;
            z = BORDE_Z_PISOTON + 0.45f + 2.4f * v;
            alto = 0.35f + 0.7f * Ruido01P(semilla, 153);
            if (std::fabs(x) > BORDE_X_PISOTON + 0.5f) alto *= 1.8f;
        }

        if (EnHuecoP(x, z)) continue;

        float giro = -60.0f + 120.0f * Ruido01P(semilla, 154);
        float curva = 0.1f + 0.35f * Ruido01P(semilla, 155);
        float ancho = 0.12f + 0.05f * alto;
        Color abajo = VariarP(COLOR_PASTO_OSCURO_P, semilla, 156, 0.12f);
        Color arriba = MezclaP(COLOR_PASTO_P, COLOR_PASTO_CLARO_P, Ruido01P(semilla, 157));
        if (Ruido01P(semilla, 158) < 0.12f) arriba = MezclaP(arriba, Color{ 196, 196, 104, 255 }, 0.5f);

        BriznaP({ x, SUELO_PISOTON, z }, alto, ancho, giro, curva, abajo, arriba, alto > 2.5f ? 5 : 3);

        // Matas: dos o tres briznas juntas.
        if (zona != 4 && Ruido01P(semilla, 159) < 0.55f)
        {
            BriznaP({ x + 0.18f, SUELO_PISOTON, z + 0.1f }, alto * 0.72f, ancho * 0.9f, giro + 70.0f, curva * 1.3f, abajo, TonoP(arriba, 0.92f), 3);
        }

        // Rocio en la punta de algunas.
        if (decoracion && zona != 4 && Ruido01P(semilla, 160) < 0.08f)
        {
            float a = giro * DEG2RAD;
            GotaRocioP({ x + std::sin(a) * curva * alto * 0.6f, SUELO_PISOTON + alto * 0.62f, z + std::cos(a) * curva * alto * 0.6f }, 0.07f);
        }
    }
}


// Matas cortitas dentro del claro: bajas y apagadas, no compiten con nada.
static void MatasClaroP()
{
    for (int k = 0; k < 34; k++)
    {
        float x = (Ruido01P(k, 171) * 2.0f - 1.0f) * (LIMITE_X_PISOTON - 0.3f);
        float z = (Ruido01P(k, 172) * 2.0f - 1.0f) * (LIMITE_Z_PISOTON - 0.3f);
        bool cerca = false;

        for (int m = 0; m < MAX_MADRIGUERAS_PISOTON; m++)
        {
            float dx = x - MADRIGUERAS_X_PISOTON[m], dz = z - MADRIGUERAS_Z_PISOTON[m];
            if (dx * dx + dz * dz < 1.6f) cerca = true;
        }

        if (cerca) continue;

        for (int b = 0; b < 4; b++)
        {
            float a = (float)b * 1.7f + Ruido01P(k, 173) * 3.0f;
            BriznaP({ x + std::cos(a) * 0.08f, SUELO_PISOTON, z + std::sin(a) * 0.08f }, 0.16f + 0.14f * Ruido01P(k * 4 + b, 174), 0.035f,
                a * RAD2DEG, 0.5f, TonoP(COLOR_MUSGO_P, 0.8f), MezclaP(COLOR_MUSGO_P, COLOR_PASTO_CLARO_P, 0.4f), 2);
        }
    }
}


static void EscenaEstaticaP(bool decoracion, int paso)
{
    g_cantidadRocioP = 0;

    BordeGuijarrosP(decoracion);
    PastoGiganteP(decoracion, paso);
    CercaP(decoracion);
    SetoP(decoracion);

    if (!decoracion)
    {
        return;
    }

    MatasClaroP();
    RegaderaP(paso == 1);
    MacetaP(paso == 1);

    // Anillo cercano: setas, piedras, chapa, bellotas, hojas, ramita y treboles.
    SetaP({ -11.8f, 0.0f, -7.6f }, 1.8f, 1.3f, Color{ 212, 56, 46, 255 }, 1);
    SetaP({ -10.6f, 0.0f, -6.9f }, 1.15f, 0.85f, Color{ 222, 70, 52, 255 }, 2);
    SetaP({ -12.9f, 0.0f, -6.6f }, 0.75f, 0.55f, Color{ 204, 60, 50, 255 }, 3);
    SetaP({ 12.7f, 0.0f, 0.8f }, 1.3f, 0.95f, Color{ 186, 136, 92, 255 }, 4, false);
    SetaP({ 13.7f, 0.0f, 1.8f }, 0.8f, 0.55f, Color{ 196, 146, 100, 255 }, 5, false);

    GuijarroP(-13.6f, -2.4f, 1.4f, 61, 0.7f);
    GuijarroP(12.2f, -4.6f, 1.1f, 62, 0.75f);
    GuijarroP(11.2f, -5.6f, 0.6f, 63, 0.7f);
    ChapaP({ -12.6f, SUELO_PISOTON, 1.6f }, 1.15f);
    BellotaP({ 11.3f, SUELO_PISOTON, 4.8f }, 1.0f, true);
    BellotaP({ 12.5f, SUELO_PISOTON, 3.5f }, 0.85f, false);
    HojaCaidaP({ -12.8f, SUELO_PISOTON, 6.2f }, -20.0f, 2.6f, 0.95f, Color{ 120, 160, 70, 255 }, 0.05f);
    HojaCaidaP({ 12.6f, SUELO_PISOTON, -1.4f }, 200.0f, 2.2f, 0.85f, Color{ 178, 128, 70, 255 }, 0.18f);
    RamitaP({ -14.5f, SUELO_PISOTON + 0.18f, 4.2f }, { -10.4f, SUELO_PISOTON + 0.15f, 6.9f }, 0.17f);

    if (paso == 1)
    {
        GuijarroP(-4.4f, -7.6f, 0.7f, 64, 0.7f);
        SetaP({ 5.8f, 0.0f, -7.9f }, 1.0f, 0.7f, Color{ 212, 62, 50, 255 }, 6);
        TrebolP({ 2.2f, 0.0f, -8.6f }, 1.6f, 20.0f, 7);
        TrebolP({ -15.5f, 0.0f, -5.0f }, 2.6f, 60.0f, 8);
        TrebolP({ 15.6f, 0.0f, -6.0f }, 2.2f, 10.0f, 9);
        TrebolP({ -11.2f, 0.0f, 3.6f }, 1.2f, 100.0f, 10);
    }

    // Media distancia: margaritas, tulipanes, diente de leon y treboles altos.
    MargaritaP({ -6.0f, 0.0f, -13.5f }, 4.2f, 1.5f, 1);
    MargaritaP({ 12.5f, 0.0f, -12.0f }, 3.4f, 1.3f, 2);
    TulipanP({ 5.5f, 0.0f, -13.0f }, 3.6f, 0.9f, Color{ 198, 150, 226, 255 });
    DienteLeonP({ 1.5f, 0.0f, -15.0f }, 4.9f, 1.15f, 3);
    TrebolP({ -2.8f, 0.0f, -11.4f }, 2.2f, 30.0f, 11);
    TrebolP({ 10.4f, 0.0f, -9.8f }, 2.0f, 80.0f, 12);

    if (paso == 1)
    {
        MargaritaP({ -9.5f, 0.0f, -15.0f }, 3.6f, 1.25f, 4);
        TulipanP({ 8.0f, 0.0f, -16.0f }, 4.4f, 1.0f, Color{ 250, 222, 120, 255 });
        TrebolP({ -13.2f, 0.0f, -10.6f }, 2.4f, 140.0f, 13);
    }

    // Rocio sobre el claro (fuera del camino de los jugadores no importa:
    // es decoracion chata) y sobre la utileria.
    for (int k = 0; k < (paso == 1 ? 10 : 5); k++)
    {
        float x = (Ruido01P(k, 181) * 2.0f - 1.0f) * (LIMITE_X_PISOTON - 0.5f);
        float z = (Ruido01P(k, 182) * 2.0f - 1.0f) * (LIMITE_Z_PISOTON - 0.5f);
        GotaRocioP({ x, SUELO_PISOTON, z }, 0.06f + 0.03f * Ruido01P(k, 183));
    }

    GotaRocioP({ -12.4f, 0.45f, 6.0f }, 0.12f);
    GotaRocioP({ -12.3f, SUELO_PISOTON + 0.33f, 1.4f }, 0.1f);
}


//--------------------------------------------------
// Camara y cielo
//--------------------------------------------------

static Camera3D CamaraVistaP(const MinijuegoPisotonPlagas& m)
{
    Camera3D c = m.camara;
    c.position = OJO_PISOTON;
    c.target = MIRA_PISOTON;
    c.up = { 0.0f, 1.0f, 0.0f };
    c.fovy = FOV_PISOTON;
    c.projection = CAMERA_PERSPECTIVE;

    // Entrada: la camara llega desde atras, como si bajara volando entre
    // las briznas, durante la cuenta regresiva.
    if (m.fase == FASE_PISOTON_PREPARACION && m.resultado.estado != RESULTADO_MINIJUEGO_CANCELADO)
    {
        float u = Lim01P(1.0f - m.tiempoPreparacion / DURACION_PREPARACION_PISOTON);
        float e = 1.0f - u;
        e = e * e * (3.0f - 2.0f * e) * ObtenerFactorMovimientoCamaraMinijuegos();
        Vector3 atras = Vector3Normalize(Vector3Subtract(OJO_PISOTON, MIRA_PISOTON));
        c.position = Vector3Add(c.position, Vector3Scale(atras, 6.5f * e));
        c.position.x += 3.5f * e;
        c.position.y += 1.5f * e;
        c.target.x += 1.0f * e;
    }

    return c;
}


// Punto del cielo segun su elevacion (grados, negativa = bajo el horizonte
// del ojo): el degrade sigue a la camara durante la entrada.
static Vector2 PuntoCieloP(const Camera3D& vista, float x, float elevacion)
{
    const float z = -60.0f;
    float d = vista.position.z - z;
    return GetWorldToScreen({ x, vista.position.y + std::tan(elevacion * DEG2RAD) * d, z }, vista);
}


static void NubeP(float x, float y, float largo, float alto, Color cuerpo, Color luz)
{
    DrawEllipse((int)x, (int)y, largo * 0.5f, alto, cuerpo);
    DrawEllipse((int)(x - largo * 0.18f), (int)(y - alto * 0.55f), largo * 0.24f, alto * 0.95f, cuerpo);
    DrawEllipse((int)(x + largo * 0.12f), (int)(y - alto * 0.8f), largo * 0.2f, alto * 1.1f, cuerpo);
    DrawEllipse((int)(x + largo * 0.08f), (int)(y - alto * 0.95f), largo * 0.14f, alto * 0.7f, luz);
}


static void DibujarCieloP(const Camera3D& vista, float t, bool decoracion)
{
    int w = GetScreenWidth();
    int h = GetScreenHeight();
    float s = (float)h / 720.0f;

    // Degrade por elevacion: celeste arriba y bruma calida sobre el seto.
    const float elev[5] = { 2.0f, -6.0f, -10.0f, -13.0f, -15.5f };
    const Color col[5] = { COLOR_CIELO_ALTO_P, COLOR_CIELO_MEDIO_P, COLOR_CIELO_BAJO_P, COLOR_HORIZONTE_P, COLOR_HORIZONTE_P };
    float ys[5];

    for (int k = 0; k < 5; k++) ys[k] = PuntoCieloP(vista, 0.0f, elev[k]).y;

    if (ys[0] > 0.0f) DrawRectangle(0, 0, w, (int)ys[0] + 1, col[0]);

    for (int k = 0; k < 4; k++)
    {
        int y0 = (int)ys[k], y1 = (int)ys[k + 1];
        if (y1 > y0 && y1 > 0 && y0 < h) DrawRectangleGradientV(0, y0, w, y1 - y0 + 1, col[k], col[k + 1]);
    }

    if (ys[4] < (float)h) DrawRectangle(0, (int)ys[4], w, h - (int)ys[4], col[4]);

    // Sol de la manana arriba a la derecha, con resplandor y rayos lentos.
    Vector2 sol = PuntoCieloP(vista, 34.0f, -4.5f);
    float radioSol = 26.0f * s;
    BeginBlendMode(BLEND_ADDITIVE);
    DrawCircleGradient(sol, radioSol * 8.0f, Fade(Color{ 255, 236, 180, 255 }, 0.3f), Fade(Color{ 255, 236, 180, 255 }, 0.0f));
    DrawCircleGradient(sol, radioSol * 3.0f, Fade(COLOR_SOL_P, 0.45f), Fade(COLOR_SOL_P, 0.0f));

    if (decoracion)
    {
        for (int k = 0; k < 10; k++)
        {
            float a = (float)k / 10.0f * 2.0f * PI + t * 0.025f;
            float largo = radioSol * (6.0f + 3.0f * Ruido01P(k, 901));
            float abre = 0.045f;
            Vector2 p1 = { sol.x + std::cos(a - abre) * largo, sol.y + std::sin(a - abre) * largo };
            Vector2 p2 = { sol.x + std::cos(a + abre) * largo, sol.y + std::sin(a + abre) * largo };
            DrawTriangle(sol, p2, p1, Fade(COLOR_SOL_P, 0.08f));
            DrawTriangle(sol, p1, p2, Fade(COLOR_SOL_P, 0.08f));
        }
    }

    EndBlendMode();
    DrawCircleV(sol, radioSol, Color{ 255, 250, 228, 255 });
    DrawCircleV(sol, radioSol * 0.82f, Color{ 255, 255, 244, 255 });

    // Nubes de algodon que cruzan despacio.
    int nubes = decoracion ? 6 : 3;

    for (int i = 0; i < nubes; i++)
    {
        float e = 1.0f - 9.0f * Ruido01P(i, 911);
        float y = PuntoCieloP(vista, 0.0f, e).y;
        float largo = (120.0f + 140.0f * Ruido01P(i, 912)) * s;
        float alto = (12.0f + 9.0f * Ruido01P(i, 913)) * s;
        float recorrido = (float)w + largo * 2.0f;
        float x = std::fmod(Ruido01P(i, 914) * recorrido + t * (5.0f + 6.0f * Ruido01P(i, 915)) * s, recorrido) - largo;
        float lejos = Lim01P((-e + 1.0f) / 9.0f);
        Color cuerpo = MezclaP(Color{ 252, 252, 250, 255 }, COLOR_CIELO_BAJO_P, lejos * 0.6f);
        Color sombra = MezclaP(cuerpo, COLOR_CIELO_MEDIO_P, 0.35f);
        DrawEllipse((int)x, (int)(y + alto * 0.35f), largo * 0.46f, alto * 0.6f, sombra);
        NubeP(x, y, largo, alto, cuerpo, Color{ 255, 255, 255, 255 });
    }

    // Horizonte en dos planos detras del seto: arboleda lejana (palida) y
    // arboles mas cercanos con el tejado de un galpon.
    float yH = GetWorldToScreen({ 20.0f, 1.2f, -23.5f }, vista).y;

    if (yH > 0.0f && yH < (float)h)
    {
        Color lejos = MezclaP(Color{ 140, 176, 168, 255 }, COLOR_HORIZONTE_P, 0.55f);
        Color cerca = MezclaP(Color{ 96, 142, 112, 255 }, COLOR_HORIZONTE_P, 0.3f);

        for (int k = 0; k < 18; k++)
        {
            float x = (float)w * (0.3f + 0.75f * (float)k / 17.0f);
            float r = (26.0f + 22.0f * Ruido01P(k, 921)) * s;
            DrawCircleV({ x, yH - r * 0.45f }, r, lejos);
        }

        DrawRectangle(0, (int)(yH - 8.0f * s), w, (int)(30.0f * s), lejos);

        // Galpon con tejado a dos aguas.
        float gx = (float)w * 0.74f, gw = 120.0f * s, gh = 46.0f * s;
        DrawRectangle((int)(gx - gw * 0.5f), (int)(yH - gh), (int)gw, (int)(gh + 10.0f * s), MezclaP(Color{ 196, 182, 160, 255 }, COLOR_HORIZONTE_P, 0.35f));
        DrawTriangle({ gx - gw * 0.62f, yH - gh }, { gx + gw * 0.62f, yH - gh }, { gx, yH - gh - 34.0f * s }, MezclaP(Color{ 168, 96, 80, 255 }, COLOR_HORIZONTE_P, 0.35f));
        DrawRectangle((int)(gx - 12.0f * s), (int)(yH - gh * 0.6f), (int)(18.0f * s), (int)(gh * 0.6f), MezclaP(Color{ 110, 96, 84, 255 }, COLOR_HORIZONTE_P, 0.35f));

        for (int k = 0; k < 9; k++)
        {
            float x = (float)w * (0.36f + 0.7f * (float)k / 8.0f) + 30.0f * s * Ruido01P(k, 922);
            if (std::fabs(x - gx) < gw * 0.6f) continue;
            float r = (20.0f + 18.0f * Ruido01P(k, 923)) * s;
            DrawRectangle((int)(x - 3.0f * s), (int)(yH - r), (int)(6.0f * s), (int)r, cerca);
            DrawCircleV({ x, yH - r * 1.3f }, r, cerca);
            DrawCircleV({ x - r * 0.5f, yH - r * 0.9f }, r * 0.7f, cerca);
        }
    }
}


//--------------------------------------------------
// Marcas del suelo (sin profundidad, despues del suelo horneado)
//--------------------------------------------------

static void SombraP(float x, float z, float rx, float rz, float alfa)
{
    DiscoSueloP({ x, SUELO_PISOTON + 0.01f, z }, rx, rz, 12, Color{ 24, 30, 16, (unsigned char)(255.0f * alfa) }, Color{ 24, 30, 16, 0 });
}


// Sector de disco (relleno de la flor que crece).
static void SectorP(Vector3 c, float r, float fraccion, Color color)
{
    int lados = 28;
    int hasta = (int)((float)lados * Lim01P(fraccion) + 0.5f);

    for (int k = 0; k < hasta; k++)
    {
        float a0 = -0.5f * PI + 2.0f * PI * (float)k / (float)lados;
        float a1 = -0.5f * PI + 2.0f * PI * (float)(k + 1) / (float)lados;
        VP(c, color);
        VP({ c.x + std::cos(a0) * r, c.y, c.z + std::sin(a0) * r }, color);
        VP({ c.x + std::cos(a1) * r, c.y, c.z + std::sin(a1) * r }, color);
    }
}


static void DibujarMarcasSueloP(
    const MinijuegoPisotonPlagas& m,
    const JugadorPrueba jugadores[],
    const Participante participantes[],
    int limite
)
{
    const float y = SUELO_PISOTON + 0.012f;
    float t = m.tiempoAnimacion;

    rlDrawRenderBatchActive();
    rlDisableDepthTest();
    LoteP();

    // Manchas de lo aplastado (se desvanecen).
    for (const ManchaPisoton& mancha : m.visual.manchas)
    {
        if (mancha.vida <= 0.0f) continue;

        float a = Lim01P(mancha.vida / 1.5f);
        Color c = mancha.tipo == PLAGA_PISOTON_BABOSA ? Color{ 246, 200, 60, 255 }
            : (mancha.tipo == PLAGA_PISOTON_ORUGA ? Color{ 120, 176, 56, 255 } : Color{ 120, 40, 34, 255 });
        int semilla = (int)(mancha.giro * 3.0f) + (int)(mancha.x * 10.0f);
        ManchaSueloP({ mancha.x, y, mancha.z }, 0.42f, 10, AlfaP(c, 0.8f * a), AlfaP(c, 0.35f * a), semilla);

        for (int k = 0; k < 5; k++)
        {
            float ang = mancha.giro * DEG2RAD + (float)k * 1.3f;
            float d = 0.5f + 0.25f * Ruido01P(semilla + k, 191);
            DiscoSueloP({ mancha.x + std::cos(ang) * d, y, mancha.z + std::sin(ang) * d }, 0.07f, 0.07f, 6, AlfaP(c, 0.8f * a), AlfaP(c, 0.6f * a));
        }
    }

    // Zona de la flor: se llena mientras crece y late al abrir.
    bool aviso = m.tiempoFlor <= AVISO_FLOR_PISOTON;

    if (aviso || m.florAbierta > 0.0f)
    {
        Vector3 c = { m.florX, y, m.florZ };
        float crecimiento = m.florAbierta > 0.0f ? 1.0f : Lim01P(1.0f - m.tiempoFlor / AVISO_FLOR_PISOTON);
        float pulso = 0.5f + 0.5f * std::sin(t * (4.0f + 8.0f * crecimiento));
        DiscoSueloP(c, RADIO_FLOR_PISOTON, RADIO_FLOR_PISOTON, 28, AlfaP(COLOR_ACENTO_P, 0.16f), AlfaP(COLOR_ACENTO_P, 0.3f));
        SectorP(c, RADIO_FLOR_PISOTON * 0.96f, crecimiento, AlfaP(COLOR_ACENTO_P, 0.22f + 0.12f * pulso));
        AnilloSueloP(c, RADIO_FLOR_PISOTON - 0.1f, RADIO_FLOR_PISOTON + 0.04f, 32, AlfaP(COLOR_ACENTO_P, 0.75f + 0.25f * pulso), AlfaP(Color{ 255, 200, 230, 255 }, 0.9f));
    }

    // Madrigueras que avisan: tierra que se oscurece y grietas que se abren.
    for (int k = 0; k < MAX_MADRIGUERAS_PISOTON; k++)
    {
        const MadriguerasPisoton& h = m.madrigueras[k];
        if (h.aviso <= 0.0f) continue;

        float u = Lim01P(1.0f - h.aviso / DURACION_AVISO_MADRIGUERA_PISOTON);
        bool dorada = h.tipoPendiente == PLAGA_PISOTON_BABOSA;
        DiscoSueloP({ h.x, y, h.z }, 0.9f + 0.35f * u, 0.9f + 0.35f * u, 14, AlfaP(COLOR_TIERRA_OSCURA_P, 0.6f), AlfaP(COLOR_TIERRA_OSCURA_P, 0.0f));

        for (int g = 0; g < 6; g++)
        {
            float a = (float)g * 1.047f + (float)k;
            float l = 0.55f + 0.55f * u;
            Vector3 d = { -std::sin(a) * 0.04f, 0.0f, std::cos(a) * 0.04f };
            Vector3 p0 = { h.x + std::cos(a) * 0.5f, y, h.z + std::sin(a) * 0.5f };
            Vector3 p1 = { h.x + std::cos(a + 0.2f) * (0.5f + l), y, h.z + std::sin(a + 0.2f) * (0.5f + l) };
            Color c = dorada ? AlfaP(Color{ 255, 210, 80, 255 }, 0.8f) : AlfaP(Color{ 50, 30, 20, 255 }, 0.8f);
            QuadColoresP(Vector3Subtract(p0, d), Vector3Add(p0, d), p1, p1, c, c, AlfaP(c, 0.0f), AlfaP(c, 0.0f));
        }
    }

    // Sombras de plagas y avispas.
    for (const PlagaPisoton& p : m.plagas)
    {
        if (!p.activa) continue;
        float e = Lim01P(p.aparicion) * Lim01P(p.escala);
        if (e <= 0.02f) continue;

        for (int s = 0; s < MAX_SEGMENTOS_PISOTON; s++)
        {
            if (!p.segVivo[s]) continue;
            float r = (p.tipo == PLAGA_PISOTON_BABOSA ? 0.5f : (p.tipo == PLAGA_PISOTON_ORUGA ? 0.32f : 0.4f)) * e;
            SombraP(p.segX[s] - p.dirX * (p.tipo == PLAGA_PISOTON_BABOSA ? 0.2f : 0.0f), p.segZ[s] + 0.06f, r, r * 0.85f, 0.5f);
        }
    }

    for (const AvispaPisoton& a : m.avispas)
    {
        if (!a.activa) continue;
        bool peligro = a.huida <= 0.0f && a.reposo <= 0.0f;
        SombraP(a.x, a.z + 0.1f, 0.28f, 0.22f, 0.35f);

        // Aviso de peligro: anillo rojo del radio exacto de la picadura.
        float pulso = 0.5f + 0.5f * std::sin(t * 9.0f + a.fase);
        Color rojo = peligro ? AlfaP(COLOR_PELIGRO_P, 0.55f + 0.35f * pulso) : AlfaP(Color{ 255, 200, 170, 255 }, 0.3f);
        AnilloSueloP({ a.x, y, a.z }, RADIO_AVISPA_PISOTON - 0.07f, RADIO_AVISPA_PISOTON, 24, rojo, rojo);
        if (peligro) DiscoSueloP({ a.x, y, a.z }, RADIO_AVISPA_PISOTON, RADIO_AVISPA_PISOTON, 20, AlfaP(COLOR_PELIGRO_P, 0.18f * pulso), AlfaP(COLOR_PELIGRO_P, 0.06f));
    }

    // Anillo del color de cada jugador.
    for (int i = 0; i < limite; i++)
    {
        if (!m.resultado.participantes[i].participo) continue;
        const JugadorPrueba& j = jugadores[i];
        float alto = Lim01P((j.posicion.y - 0.7f) / 2.5f);
        Color c = participantes[i].color;
        AnilloSueloP({ j.posicion.x, y, j.posicion.z }, 0.5f, 0.62f - 0.08f * alto, 24, AlfaP(c, 0.9f - 0.4f * alto), AlfaP(c, 0.9f - 0.4f * alto));
        AnilloSueloP({ j.posicion.x, y, j.posicion.z }, 0.44f, 0.5f, 24, AlfaP(WHITE, 0.6f - 0.3f * alto), AlfaP(WHITE, 0.6f - 0.3f * alto));
    }

    // Ondas al sumar (color del jugador) o al recibir una picadura (rojo).
    for (const OndaPisoton& o : m.visual.ondas)
    {
        if (o.vida <= 0.0f || o.jugador < 0) continue;
        float u = 1.0f - o.vida / 0.7f;
        float r = 0.5f + 1.6f * u;
        Color c = o.picadura ? COLOR_PELIGRO_P : participantes[o.jugador].color;
        AnilloSueloP({ o.x, y, o.z }, r - 0.16f * (1.0f - u), r, 28, AlfaP(c, 0.0f), AlfaP(c, 0.85f * (1.0f - u)));
    }

    FinLoteP();
    rlDrawRenderBatchActive();
    rlEnableDepthTest();
}


//--------------------------------------------------
// Vida del jardin (por frame)
//--------------------------------------------------

static void MariposaP(Vector3 c, float rumbo, float aleteo, Color ala, Color borde)
{
    Vector3 f = { std::cos(rumbo), 0.0f, std::sin(rumbo) };
    Vector3 l = { -f.z, 0.0f, f.x };
    float a = aleteo;

    for (int s = -1; s <= 1; s += 2)
    {
        Vector3 dir = Vector3Normalize(Vector3Add(Vector3Scale(l, (float)s * std::cos(a)), { 0.0f, std::sin(a), 0.0f }));
        Vector3 p1 = Vector3Add(c, Vector3Add(Vector3Scale(dir, 0.55f), Vector3Scale(f, 0.3f)));
        Vector3 p2 = Vector3Add(c, Vector3Add(Vector3Scale(dir, 0.6f), Vector3Scale(f, -0.15f)));
        Vector3 p3 = Vector3Add(c, Vector3Add(Vector3Scale(dir, 0.38f), Vector3Scale(f, -0.42f)));
        TriP(c, p1, p2, ala);
        TriP(c, p2, p3, TonoP(ala, 0.92f));
        TriP(Vector3Add(c, Vector3Scale(dir, 0.02f)), Vector3Add(Vector3Scale(Vector3Subtract(p1, c), 0.4f), p1), p1, borde);
    }

    VigaP(Vector3Add(c, Vector3Scale(f, 0.22f)), Vector3Add(c, Vector3Scale(f, -0.25f)), 0.06f, Color{ 60, 50, 46, 255 });
}


static void DibujarAnimadoP(const MinijuegoPisotonPlagas& m, bool decoracion, bool efectos)
{
    float t = m.tiempoAnimacion;
    LoteP();

    // Gotas que caen de la roseta de la regadera y salpican.
    if (decoracion)
    {
        for (int k = 0; k < 3; k++)
        {
            float ciclo = 1.6f;
            float u = std::fmod(t + (float)k * 0.53f, ciclo);
            float caida = 0.5f * 9.0f * u * u;
            Vector3 p = { ROSETA_REGADERA_P.x + 0.3f + 0.25f * (float)(k - 1), ROSETA_REGADERA_P.y - 0.9f - caida, ROSETA_REGADERA_P.z + 0.2f * (float)k };

            if (p.y > 0.05f)
            {
                BolaP(p, { 0.13f, 0.2f, 0.13f }, Color{ 176, 220, 240, 255 }, 2, 6);
            }
            else
            {
                float tChoque = std::sqrt(2.0f * (ROSETA_REGADERA_P.y - 0.95f) / 9.0f);
                float v = Lim01P((u - tChoque) / 0.35f);
                Color c = AlfaP(Color{ 210, 236, 250, 255 }, 0.8f * (1.0f - v));
                for (int a = 0; a < 10; a++)
                {
                    float a0 = (float)a * 0.628f, a1 = a0 + 0.628f;
                    float r0 = 0.2f + 0.7f * v, r1 = r0 + 0.08f;
                    QuadColoresP({ p.x + std::cos(a0) * r0, 0.04f, p.z + std::sin(a0) * r0 }, { p.x + std::cos(a0) * r1, 0.04f, p.z + std::sin(a0) * r1 },
                        { p.x + std::cos(a1) * r1, 0.04f, p.z + std::sin(a1) * r1 }, { p.x + std::cos(a1) * r0, 0.04f, p.z + std::sin(a1) * r0 }, c, c, c, c);
                }
            }
        }

        // Matas del frente que se mecen con la brisa.
        for (int k = 0; k < 14; k++)
        {
            float x = -12.0f + 24.0f * Ruido01P(k, 951);
            float z = BORDE_Z_PISOTON + 0.9f + 1.6f * Ruido01P(k, 952);
            float alto = 0.7f + 0.6f * Ruido01P(k, 953);
            if (std::fabs(x) < BORDE_X_PISOTON) alto *= 0.75f;
            float mecer = std::sin(t * 1.4f + (float)k * 0.8f) * 0.18f;
            BriznaP({ x, SUELO_PISOTON, z }, alto, 0.1f, -30.0f + 60.0f * Ruido01P(k, 954), 0.25f + mecer,
                COLOR_PASTO_OSCURO_P, MezclaP(COLOR_PASTO_P, COLOR_PASTO_CLARO_P, Ruido01P(k, 955)), 3);
        }

        // Mariposas que revolotean sobre el pasto del fondo.
        const Color alas[2] = { Color{ 252, 250, 238, 255 }, Color{ 206, 176, 240, 255 } };
        const Color bordes[2] = { Color{ 120, 120, 130, 255 }, Color{ 120, 84, 170, 255 } };

        for (int b = 0; b < 2; b++)
        {
            float f = t * (0.22f + 0.07f * (float)b) + (float)b * 2.4f;
            Vector3 c = { (b == 0 ? -4.0f : 9.0f) + 6.0f * std::sin(f), 3.4f + 0.8f * std::sin(f * 2.3f) + 0.3f * std::sin(t * 7.0f), -10.5f - 2.0f * std::cos(f * 1.3f) };
            float rumbo = std::atan2(-2.6f * std::sin(f * 1.3f) * 1.3f, 6.0f * std::cos(f));
            MariposaP(c, rumbo, std::sin(t * 16.0f + (float)b) * 0.9f, alas[b], bordes[b]);
        }

        // Vilanos que se sueltan del diente de leon.
        for (int k = 0; k < 7; k++)
        {
            float ciclo = 9.0f;
            float u = std::fmod(t + (float)k * 1.3f, ciclo) / ciclo;
            Vector3 p = { 2.4f + 9.0f * u + 0.5f * std::sin(t * 1.5f + (float)k), 4.9f + 1.4f * u + 0.3f * std::sin(t * 2.0f + (float)k * 2.0f), -15.0f + 3.0f * u };
            Color c = AlfaP(Color{ 255, 255, 252, 255 }, 1.0f - u);
            PuntoP(p, 0.09f, c);
            VigaP(p, { p.x - 0.05f, p.y - 0.25f, p.z }, 0.025f, AlfaP(Color{ 200, 190, 160, 255 }, 1.0f - u));
        }
    }

    FinLoteP();
    (void)efectos;
}


//--------------------------------------------------
// Piezas jugables: madrigueras, flor, plagas y avispas
//--------------------------------------------------

static void DibujarMadriguerasP(const MinijuegoPisotonPlagas& m)
{
    for (int k = 0; k < MAX_MADRIGUERAS_PISOTON; k++)
    {
        const MadriguerasPisoton& madriguera = m.madrigueras[k];
        float temblor = 0.0f;
        float elevacion = 0.0f;

        if (madriguera.aviso > 0.0f)
        {
            temblor = std::sin(m.tiempoAnimacion * 60.0f + (float)k) * 0.06f * ObtenerFactorMovimientoCamaraMinijuegos();
            elevacion = 0.15f * (1.0f - madriguera.aviso / DURACION_AVISO_MADRIGUERA_PISOTON);
        }

        float altura = (0.12f + elevacion) / 0.12f;

        if (!DibujarModeloPisotonPlagasRetro3D(MODELO_PISOTON_MADRIGUERA,
            { madriguera.x + temblor, SUELO_PISOTON + 0.045f * altura, madriguera.z }, 0, { 1, altura, 1 }))
        {
            (DrawCylinder)({ madriguera.x + temblor, SUELO_PISOTON, madriguera.z }, 0.35f + elevacion, 0.7f, 0.12f + elevacion, 12, Color{ 118, 82, 50, 255 });
            (DrawCylinder)({ madriguera.x, SUELO_PISOTON + 0.115f + elevacion, madriguera.z }, 0.3f, 0.3f, 0.01f, 12, Color{ 40, 26, 18, 255 });
        }

        if (madriguera.aviso > 0.0f)
        {
            float pulso = 0.5f + 0.5f * std::sin(m.tiempoAnimacion * 14.0f);
            if (!DibujarModeloPisotonPlagasRetro3D(MODELO_PISOTON_ARO_AVISO,
                { madriguera.x, SUELO_PISOTON, madriguera.z }, 0, { 1, 1, 1 }, Fade(ORANGE, 0.5f + 0.4f * pulso)))
                DrawCircle3D({ madriguera.x, SUELO_PISOTON + 0.03f, madriguera.z }, 0.85f, { 1.0f, 0.0f, 0.0f }, 90.0f, Fade(ORANGE, 0.5f + 0.4f * pulso));
        }
    }

    // Terrones que saltan de las madrigueras que avisan.
    LoteP();

    for (int k = 0; k < MAX_MADRIGUERAS_PISOTON; k++)
    {
        const MadriguerasPisoton& h = m.madrigueras[k];
        if (h.aviso <= 0.0f) continue;

        for (int c = 0; c < 5; c++)
        {
            float u = std::fmod(m.tiempoAnimacion * 2.2f + (float)c * 0.21f + (float)k * 0.37f, 1.0f);
            float a = (float)c * 1.26f + (float)k;
            float d = 0.25f + 0.5f * u;
            float alto = 0.9f * u * (1.0f - u) * 4.0f * 0.4f;
            Color tierra = h.tipoPendiente == PLAGA_PISOTON_BABOSA && c % 2 ? Color{ 250, 206, 70, 255 } : Color{ 116, 82, 52, 255 };
            BolaP({ h.x + std::cos(a) * d, SUELO_PISOTON + 0.15f + alto, h.z + std::sin(a) * d }, { 0.07f, 0.06f, 0.07f }, tierra, 2, 5);
        }
    }

    FinLoteP();
}


static void DibujarFlorP(const MinijuegoPisotonPlagas& m)
{
    bool aviso = m.tiempoFlor <= AVISO_FLOR_PISOTON;

    if (!aviso && m.florAbierta <= 0.0f) return;

    float x = m.florX;
    float z = m.florZ;
    float crecimiento = m.florAbierta > 0.0f ? 1.0f : LimitarPisoton(1.0f - m.tiempoFlor / AVISO_FLOR_PISOTON, 0.0f, 1.0f);
    float abierta = m.florAbierta > 0.0f ? 1.0f : LimitarPisoton((crecimiento - 0.6f) / 0.4f, 0.0f, 1.0f);
    float alto = 0.4f + 2.2f * crecimiento;
    float pulso = 0.5f + 0.5f * std::sin(m.tiempoAnimacion * (4.0f + 10.0f * crecimiento));

    // Abierta: el aro del GLB marca la zona (fallback local si falta).
    if (m.florAbierta > 0.0f && !DibujarModeloPisotonPlagasRetro3D(MODELO_PISOTON_ARO_FLOR,
        { x, SUELO_PISOTON + 0.03f, z }, 0, { 1, 1, 1 }, WHITE, 0.7f + 0.3f * pulso))
    {
        (DrawCylinder)({ x, SUELO_PISOTON + 0.01f, z }, RADIO_FLOR_PISOTON, RADIO_FLOR_PISOTON, 0.015f, 28, Fade(Color{ 255, 150, 210, 255 }, 0.25f + 0.2f * pulso));
        DrawCircle3D({ x, SUELO_PISOTON + 0.05f, z }, RADIO_FLOR_PISOTON, { 1.0f, 0.0f, 0.0f }, 90.0f, Fade(Color{ 255, 110, 190, 255 }, 0.7f + 0.3f * pulso));
    }

    if (abierta > 0.0f)
    {
        if (DibujarFlorAbiertaPisotonRetro3D({ x, SUELO_PISOTON, z }, alto, abierta, m.tiempoAnimacion)) return;
    }
    else if (DibujarModeloPisotonPlagasRetro3D(MODELO_PISOTON_BROTE,
        { x, SUELO_PISOTON, z }, 0, { (0.22f + 0.2f * crecimiento) / 0.38f, alto / 2.6f, (0.22f + 0.2f * crecimiento) / 0.38f }))
    {
        return;
    }

    (DrawCylinder)({ x, SUELO_PISOTON, z }, 0.08f, 0.12f, alto, 6, Color{ 70, 150, 56, 255 });

    if (abierta <= 0.0f)
    {
        (DrawSphere)({ x, alto + 0.12f, z }, 0.22f + 0.2f * crecimiento, Color{ 100, 180, 70, 255 });
        return;
    }

    for (int p = 0; p < 8; p++)
    {
        float angulo = (float)p * 2.0f * PI / 8.0f + m.tiempoAnimacion * 0.4f;
        (DrawSphere)({ x + std::cos(angulo) * 0.75f * abierta, alto + 0.05f, z + std::sin(angulo) * 0.75f * abierta }, 0.38f,
            (p % 2 == 0) ? Color{ 255, 120, 190, 255 } : Color{ 255, 160, 215, 255 });
    }

    (DrawSphere)({ x, alto + 0.12f, z }, 0.4f, Color{ 255, 210, 60, 255 });
}


static void DibujarPlagaP(const PlagaPisoton& plaga, float tiempo)
{
    if (!plaga.activa) return;

    float escala = LimitarPisoton(plaga.aparicion, 0.0f, 1.0f) * LimitarPisoton(plaga.escala, 0.0f, 1.0f);

    if (escala <= 0.02f) return;

    bool aturdida = plaga.aturdida > 0.0f;
    float fx = plaga.dirX, fz = plaga.dirZ;
    float sx = -fz, sz = fx;
    float oscila = aturdida ? 0.0f : std::sin(plaga.edad * 14.0f);
    // Los modelos miran hacia +X; raylib gira +X hacia -Z en un giro Y positivo.
    float giro = -std::atan2(fz, fx) * RAD2DEG;

    if (plaga.tipo == PLAGA_PISOTON_ESCARABAJO)
    {
        float x = plaga.segX[0], z = plaga.segZ[0];

        if (!DibujarPlagaAnimadaPisotonRetro3D(MODELO_PISOTON_ESCARABAJO, { x, 0, z }, giro, escala, oscila))
        {
            Color cuerpo = Color{ 150, 38, 30, 255 };
            (DrawSphere)({ x, 0.27f * escala, z }, 0.3f * escala, cuerpo);
            (DrawSphere)({ x + fx * 0.3f * escala, 0.24f * escala, z + fz * 0.3f * escala }, 0.16f * escala, Color{ 30, 20, 20, 255 });
            (DrawSphere)({ x - fx * 0.05f + sx * 0.12f, 0.5f * escala, z - fz * 0.05f + sz * 0.12f }, 0.06f * escala, BLACK);
            (DrawSphere)({ x - fx * 0.05f - sx * 0.12f, 0.5f * escala, z - fz * 0.05f - sz * 0.12f }, 0.06f * escala, BLACK);

            for (int p = -1; p <= 1; p++)
            {
                float pata = (float)p * 0.2f;
                float meneo = oscila * 0.08f * (float)(p == 0 ? -1 : 1);
                for (int lado = -1; lado <= 1; lado += 2)
                {
                    DrawLine3D({ x + fx * pata, 0.15f * escala, z + fz * pata },
                        { x + fx * (pata + meneo) + sx * 0.42f * (float)lado * escala, 0.03f, z + fz * (pata + meneo) + sz * 0.42f * (float)lado * escala },
                        Color{ 40, 24, 20, 255 });
                }
            }
        }
    }
    else if (plaga.tipo == PLAGA_PISOTON_ORUGA)
    {
        int cabeza = 0;
        while (cabeza < MAX_SEGMENTOS_PISOTON - 1 && !plaga.segVivo[cabeza]) cabeza++;

        for (int s = MAX_SEGMENTOS_PISOTON - 1; s >= 0; s--)
        {
            if (!plaga.segVivo[s]) continue;

            float x = plaga.segX[s], z = plaga.segZ[s];
            float bote = aturdida ? 0.0f : std::fabs(std::sin(plaga.edad * 9.0f + (float)s * 1.2f)) * 0.07f;
            bool modelo = DibujarModeloPisotonPlagasRetro3D(s == cabeza ? MODELO_PISOTON_ORUGA_CABEZA : MODELO_PISOTON_ORUGA_SEGMENTO,
                { x, bote * escala, z }, giro, { escala, escala, escala });

            if (modelo) continue;

            Color verde = (s % 2 == 0) ? Color{ 120, 200, 70, 255 } : Color{ 90, 170, 56, 255 };
            (DrawSphere)({ x, (0.27f + bote) * escala, z }, 0.27f * escala, verde);

            if (s == cabeza)
            {
                (DrawSphere)({ x + fx * 0.2f + sx * 0.11f, 0.4f * escala, z + fz * 0.2f + sz * 0.11f }, 0.07f * escala, WHITE);
                (DrawSphere)({ x + fx * 0.2f - sx * 0.11f, 0.4f * escala, z + fz * 0.2f - sz * 0.11f }, 0.07f * escala, WHITE);
            }
        }
    }
    else
    {
        float x = plaga.segX[0], z = plaga.segZ[0];
        float estira = aturdida ? 0.0f : std::sin(plaga.edad * 5.0f) * 0.04f;

        if (!DibujarPlagaAnimadaPisotonRetro3D(MODELO_PISOTON_BABOSA, { x, 0, z }, giro, escala, estira / 0.04f))
        {
            Color oro = Color{ 255, 205, 40, 255 };
            (DrawSphere)({ x, 0.3f * escala, z }, 0.3f * escala, oro);
            (DrawSphere)({ x - fx * (0.3f + estira), 0.25f * escala, z - fz * (0.3f + estira) }, 0.26f * escala, oro);
            (DrawSphere)({ x - fx * 0.55f, 0.19f * escala, z - fz * 0.55f }, 0.2f * escala, Color{ 240, 180, 30, 255 });
            (DrawSphere)({ x + fx * 0.05f, 0.45f * escala, z + fz * 0.05f }, 0.12f * escala, Color{ 255, 240, 160, 255 });
        }
    }

    (void)tiempo;
}


static void DibujarAvispaP(const AvispaPisoton& avispa, float tiempo)
{
    if (!avispa.activa) return;

    float fx = avispa.dirX, fz = avispa.dirZ;
    float sx = -fz, sz = fx;
    float aleteo = std::sin(tiempo * 55.0f) * 0.12f;
    float giro = -std::atan2(fz, fx) * RAD2DEG;

    if (!DibujarModeloPisotonPlagasRetro3D(MODELO_PISOTON_AVISPA, { avispa.x, avispa.altura, avispa.z }, giro))
    {
        Color amarillo = avispa.huida > 0.0f ? Color{ 255, 150, 90, 255 } : Color{ 255, 214, 40, 255 };
        (DrawSphere)({ avispa.x + fx * 0.24f, avispa.altura, avispa.z + fz * 0.24f }, 0.13f, Color{ 30, 24, 20, 255 });
        (DrawSphere)({ avispa.x, avispa.altura, avispa.z }, 0.17f, Color{ 30, 24, 20, 255 });
        (DrawSphere)({ avispa.x - fx * 0.25f, avispa.altura, avispa.z - fz * 0.25f }, 0.2f, amarillo);
        (DrawSphere)({ avispa.x - fx * 0.3f, avispa.altura, avispa.z - fz * 0.3f }, 0.2f, amarillo);
    }

    for (int lado = -1; lado <= 1; lado += 2)
    {
        Vector3 posicion = { avispa.x + sx * 0.22f * (float)lado, avispa.altura + 0.17f + aleteo, avispa.z + sz * 0.22f * (float)lado };

        if (DibujarModeloPisotonPlagasRetro3D(MODELO_PISOTON_ALA, posicion, giro + (lado < 0 ? 180 : 0))) continue;

        (DrawCube)(posicion, 0.3f, 0.02f, 0.18f, Fade(Color{ 220, 240, 255, 255 }, 0.7f));
    }
}


//--------------------------------------------------
// Efectos: particulas, estrellas de aturdido y confeti
//--------------------------------------------------

static void EstrellaP(Vector3 c, float r, float giro, Color color)
{
    for (int k = 0; k < 4; k++)
    {
        float a = giro + (float)k * 0.5f * PI;
        Vector3 punta = Vector3Add(c, Vector3Add(Vector3Scale(g_derP, std::cos(a) * r), Vector3Scale(g_arribaP, std::sin(a) * r)));
        Vector3 l0 = Vector3Add(c, Vector3Add(Vector3Scale(g_derP, std::cos(a + 0.785f) * r * 0.32f), Vector3Scale(g_arribaP, std::sin(a + 0.785f) * r * 0.32f)));
        Vector3 l1 = Vector3Add(c, Vector3Add(Vector3Scale(g_derP, std::cos(a - 0.785f) * r * 0.32f), Vector3Scale(g_arribaP, std::sin(a - 0.785f) * r * 0.32f)));
        VP(c, color); VP(l1, color); VP(punta, color);
        VP(c, color); VP(punta, color); VP(l0, color);
    }
}


static void DibujarEfectosP(
    const MinijuegoPisotonPlagas& m,
    const JugadorPrueba jugadores[],
    const Participante participantes[],
    int limite
)
{
    float t = m.tiempoAnimacion;
    LoteP();

    // Particulas de la logica (tierra, jugo de plaga, polen de la flor).
    for (const ParticulaTierra& p : m.particulas)
    {
        if (!p.activa) continue;
        float u = p.vidaMaxima > 0.0f ? Lim01P(p.vida / p.vidaMaxima) : 0.0f;
        PuntoP(p.posicion, p.tamano * (0.5f + 0.5f * u), AlfaP(p.color, 0.4f + 0.6f * u));
    }

    // Estrellitas que giran sobre las plagas aturdidas.
    for (const PlagaPisoton& p : m.plagas)
    {
        if (!p.activa || p.aturdida <= 0.0f) continue;

        int s = 0;
        while (s < MAX_SEGMENTOS_PISOTON - 1 && !p.segVivo[s]) s++;
        float altura = p.tipo == PLAGA_PISOTON_BABOSA ? 0.95f : 0.82f;

        for (int k = 0; k < 3; k++)
        {
            float a = t * 6.0f + (float)k * 2.0944f;
            EstrellaP({ p.segX[s] + std::cos(a) * 0.36f, altura + 0.05f * std::sin(t * 9.0f + (float)k), p.segZ[s] + std::sin(a) * 0.36f },
                0.12f, t * 4.0f, Color{ 255, 236, 110, 255 });
        }
    }

    // Confeti sobre el ganador.
    if (m.fase == FASE_PISOTON_TERMINADO && m.visual.tiempoFin >= 0.0f && m.resultado.estado == RESULTADO_MINIJUEGO_FINALIZADO)
    {
        int ganadores[MAX_PARTICIPANTES]{};
        int cantidad = ObtenerIndicesGanadores(m.resultado, ganadores, MAX_PARTICIPANTES);
        float te = t - m.visual.tiempoFin;

        for (int g = 0; g < cantidad && g < limite; g++)
        {
            Vector3 c = jugadores[ganadores[g]].posicion;
            Color propio = participantes[ganadores[g]].color;

            for (int k = 0; k < 46; k++)
            {
                float ciclo = 2.6f;
                float u = std::fmod(te + Ruido01P(k, 961) * ciclo, ciclo);
                float a = 2.0f * PI * Ruido01P(k, 962);
                float v = 1.2f + 1.6f * Ruido01P(k, 963);
                Vector3 p = { c.x + std::cos(a) * v * u * 0.6f, c.y + 2.6f + 3.2f * u - 2.6f * u * u, c.z + std::sin(a) * v * u * 0.6f };
                if (p.y < 0.05f) p.y = 0.05f;
                const Color paleta[4] = { propio, Color{ 255, 236, 120, 255 }, Color{ 255, 255, 255, 255 }, COLOR_ACENTO_P };
                Color col = AlfaP(paleta[k % 4], u > ciclo - 0.4f ? (ciclo - u) / 0.4f : 1.0f);
                float giro = t * (3.0f + 4.0f * Ruido01P(k, 964)) + (float)k;
                Vector3 d = Vector3Scale(g_derP, std::cos(giro) * 0.09f);
                Vector3 e = Vector3Scale(g_arribaP, 0.06f);
                QuadColoresP(Vector3Subtract(Vector3Subtract(p, d), e), Vector3Subtract(Vector3Add(p, d), e), Vector3Add(Vector3Add(p, d), e), Vector3Add(Vector3Subtract(p, d), e), col, col, col, col);
            }
        }
    }

    FinLoteP();
}


//--------------------------------------------------
// Luces falsas aditivas: haces de sol, charco de luz, rocio y polen
//--------------------------------------------------

// Tira que mira a la camara entre dos puntos (haces de luz).
static void TiraP(Vector3 a, Vector3 b, float anchoA, float anchoB, Color ca, Color cb)
{
    Vector3 eje = Vector3Subtract(b, a);
    Vector3 vista = Vector3Subtract(Vector3Scale(Vector3Add(a, b), 0.5f), g_camP);
    Vector3 lado = Vector3CrossProduct(eje, vista);
    float l = Vector3Length(lado);

    if (l < 0.0001f) return;

    lado = Vector3Scale(lado, 1.0f / l);
    Vector3 la = Vector3Scale(lado, anchoA * 0.5f), lb = Vector3Scale(lado, anchoB * 0.5f);
    QuadColoresP(Vector3Subtract(a, la), Vector3Add(a, la), Vector3Add(b, lb), Vector3Subtract(b, lb), ca, ca, cb, cb);
}


static void DibujarLucesP(
    const MinijuegoPisotonPlagas& m,
    const JugadorPrueba jugadores[],
    int limite,
    bool decoracion
)
{
    float t = m.tiempoAnimacion;

    rlDrawRenderBatchActive();
    rlDisableDepthMask();
    BeginBlendMode(BLEND_ADDITIVE);
    LoteP();

    // Haces de sol que entran entre el pasto, de atras a la derecha.
    const Vector3 origen = { 22.0f, 26.0f, -30.0f };
    const Vector3 destinos[4] = { { 3.5f, 0.0f, -1.0f }, { -3.0f, 0.0f, 1.5f }, { 8.0f, 0.0f, 2.0f }, { -7.5f, 0.0f, -2.5f } };
    int haces = decoracion ? 4 : 2;

    for (int k = 0; k < haces; k++)
    {
        float respira = 0.75f + 0.25f * std::sin(t * 0.5f + (float)k * 1.7f);
        Color arriba = AlfaP(Color{ 255, 240, 190, 255 }, 0.0f);
        Color abajo = AlfaP(Color{ 255, 236, 180, 255 }, 0.07f * respira);
        Vector3 a = Vector3Lerp(origen, destinos[k], 0.25f);
        TiraP(a, destinos[k], 1.2f, 2.6f + 0.6f * (float)k, arriba, abajo);
    }

    // Charco de luz del sol sobre el claro.
    for (int k = 0; k < 4; k++)
    {
        float r = 2.2f + 1.4f * (float)k;
        DiscoSueloP({ 2.5f, SUELO_PISOTON + 0.02f, 0.2f }, r * 1.35f, r, 24, Color{ 255, 236, 180, 12 }, Color{ 255, 236, 180, 0 });
    }

    // Brillo del rocio: cada gota titila a su ritmo.
    for (int k = 0; k < g_cantidadRocioP; k++)
    {
        float b = 0.5f + 0.5f * std::sin(t * (2.0f + 0.6f * Ruido01P(k, 971)) + (float)k * 1.9f);
        b = b * b * b;
        float r = g_rocioRadioP[k];
        HaloP(g_rocioP[k], r * (2.2f + 2.0f * b), Color{ 255, 255, 255, (unsigned char)(70.0f + 140.0f * b) }, Color{ 200, 240, 255, 0 }, 10);
        if (b > 0.6f) EstrellaP(g_rocioP[k], r * 4.5f * b, 0.785f, Color{ 255, 255, 255, (unsigned char)(160.0f * b) });
    }

    // Polen y polvo que flotan dentro de los haces.
    int motas = (int)(36.0f * FactorCalidadGrafica(CalidadParticulas()) + 0.5f);

    for (int k = 0; k < motas; k++)
    {
        float u = std::fmod(t * (0.03f + 0.03f * Ruido01P(k, 981)) + Ruido01P(k, 982), 1.0f);
        Vector3 d = destinos[k % haces];
        Vector3 p = Vector3Lerp(Vector3Lerp(origen, d, 0.55f), d, u);
        p.x += 1.2f * std::sin(t * 0.7f + (float)k) + (Ruido01P(k, 983) - 0.5f) * 2.0f;
        p.y = 0.4f + 3.5f * Ruido01P(k, 984) + 0.3f * std::sin(t + (float)k);
        float a = std::sin(PI * u);
        PuntoP(p, 0.045f, Color{ 255, 246, 200, (unsigned char)(150.0f * a) });
    }

    // Halo de la flor abierta (el acento de la escena).
    if (m.florAbierta > 0.0f)
    {
        float pulso = 0.5f + 0.5f * std::sin(t * 6.0f);
        HaloP({ m.florX, 2.7f, m.florZ }, 1.6f + 0.3f * pulso, AlfaP(COLOR_ACENTO_P, 0.35f), AlfaP(COLOR_ACENTO_P, 0.0f), 18);
        DiscoSueloP({ m.florX, SUELO_PISOTON + 0.03f, m.florZ }, RADIO_FLOR_PISOTON * 1.2f, RADIO_FLOR_PISOTON * 1.2f, 24, AlfaP(COLOR_ACENTO_P, 0.14f), AlfaP(COLOR_ACENTO_P, 0.0f));
    }

    // Destello dorado de la babosa y aura de las plagas aturdidas.
    for (const PlagaPisoton& p : m.plagas)
    {
        if (!p.activa || p.aparicion < 0.3f) continue;

        if (p.tipo == PLAGA_PISOTON_BABOSA)
        {
            for (int k = 0; k < 2; k++)
            {
                float a = t * 3.0f + (float)k * PI;
                EstrellaP({ p.segX[0] + std::cos(a) * 0.55f, 0.6f + 0.12f * std::sin(t * 8.0f + (float)k), p.segZ[0] + std::sin(a) * 0.55f }, 0.14f, t * 2.0f, Color{ 255, 240, 170, 220 });
            }
            HaloP({ p.segX[0], 0.35f, p.segZ[0] }, 0.8f, Color{ 255, 210, 80, 60 }, Color{ 255, 210, 80, 0 }, 14);
        }
    }

    // Destello rojo sobre el jugador picado.
    for (int i = 0; i < limite; i++)
    {
        if (m.visual.picado[i] <= 0.0f) continue;
        float a = m.visual.picado[i] / 0.6f;
        Vector3 c = jugadores[i].posicion;
        HaloP(c, 1.0f + 0.4f * (1.0f - a), Color{ 255, 80, 60, (unsigned char)(120.0f * a) }, Color{ 255, 80, 60, 0 }, 16);
    }

    FinLoteP();
    rlDrawRenderBatchActive();
    EndBlendMode();
    rlEnableDepthMask();
}


//==================================================
// VISUAL: ANUNCIOS Y HUD
//==================================================

static void DibujarAnunciosP(
    const MinijuegoPisotonPlagas& m,
    const JugadorPrueba jugadores[],
    const Participante participantes[],
    int limite,
    const Camera3D& vista
)
{
    float t = m.tiempoAnimacion;

    // Valor de cada plaga sobre su cabeza (tipo por color).
    for (const PlagaPisoton& p : m.plagas)
    {
        if (!p.activa || p.escapando || p.aparicion < 0.6f) continue;

        int s = 0;
        while (s < MAX_SEGMENTOS_PISOTON - 1 && !p.segVivo[s]) s++;
        Color c = p.tipo == PLAGA_PISOTON_BABOSA ? Color{ 255, 214, 70, 255 }
            : (p.tipo == PLAGA_PISOTON_ORUGA ? Color{ 176, 236, 120, 255 } : Color{ 255, 236, 226, 255 });
        float altura = p.tipo == PLAGA_PISOTON_BABOSA ? 1.15f : 0.95f;
        Vector2 q = GetWorldToScreen({ p.segX[s], altura, p.segZ[s] }, vista);
        DibujarTextoMundoContornoUI(TextFormat("+%d", ValorPlagaPisoton(p)), q.x, q.y, UI_TEXTO_PEQUENO, c, 0.9f);
    }

    // Peligro sobre cada avispa que acecha.
    for (const AvispaPisoton& a : m.avispas)
    {
        if (!a.activa || a.huida > 0.0f || a.reposo > 0.0f) continue;
        Vector2 q = GetWorldToScreen({ a.x, a.altura + 0.75f + 0.08f * std::sin(t * 6.0f), a.z }, vista);
        DibujarTextoMundoContornoUI("¡!", q.x, q.y, UI_TEXTO, COLOR_PELIGRO_P);
    }

    // Babosa dorada a punto de salir.
    for (const MadriguerasPisoton& h : m.madrigueras)
    {
        if (h.aviso <= 0.0f || h.tipoPendiente != PLAGA_PISOTON_BABOSA) continue;
        float salto = 1.0f + 0.08f * std::sin(t * 12.0f);
        Vector2 q = GetWorldToScreen({ h.x, 1.2f, h.z }, vista);
        DibujarTextoMundoContornoUI("¡BABOSA DORADA!", q.x, q.y - 6.0f * salto * EscalaUI(), UI_TEXTO, Color{ 255, 214, 70, 255 });
    }

    // Flor: cuenta para florecer.
    if (m.fase == FASE_PISOTON_JUGANDO && m.tiempoFlor <= AVISO_FLOR_PISOTON && m.florAbierta <= 0.0f)
    {
        Vector2 q = GetWorldToScreen({ m.florX, 3.0f, m.florZ }, vista);
        int segundos = (int)std::ceil(m.tiempoFlor > 0.0f ? m.tiempoFlor : 0.0f);
        DibujarTextoMundoContornoUI(TextFormat("¡FLOR +%d!", PUNTOS_FLOR_PISOTON), q.x, q.y, UI_TEXTO, Color{ 255, 150, 210, 255 });
        if (segundos > 0) DibujarTextoMundoContornoUI(TextFormat("%d", segundos), q.x, q.y + 24.0f * EscalaUI(), UI_TEXTO_GRANDE, Color{ 255, 236, 246, 255 });
    }

    // Cartelito J# sobre cada jugador.
    for (int i = 0; i < limite; i++)
    {
        if (!m.resultado.participantes[i].participo) continue;

        Vector3 cabeza = jugadores[i].posicion;
        cabeza.y += jugadores[i].tamano.y * 0.5f + 0.5f;
        Vector2 q = GetWorldToScreen(cabeza, vista);
        OpcionesEtiquetaUI opciones;
        opciones.borde = participantes[i].color;
        opciones.pico = true;
        int numero = participantes[i].numeroJugador > 0 ? participantes[i].numeroJugador : i + 1;
        DibujarEtiquetaUI(TextFormat("J%d", numero), q, 0.0f, opciones);
    }

    // Puntos que suben desde donde se ganaron o perdieron.
    for (const PopupPisoton& p : m.popups)
    {
        if (!p.activo) continue;

        float vida = 1.0f - p.vida / 1.1f;
        float alfa = Lim01P(p.vida * 2.5f);
        float escala = vida < 0.12f ? 0.7f + vida * 2.5f : 1.0f;
        Vector2 q = GetWorldToScreen(p.posicion, vista);
        // Rojo: picadura; dorado: babosa; claro: el resto (flor incluida).
        Color c = p.valor < 0 ? Color{ 255, 110, 96, 255 }
            : (ColorIsEqual(p.color, GOLD) ? Color{ 255, 214, 70, 255 } : Color{ 240, 248, 226, 255 });
        DibujarTextoMundoContornoUI(p.valor >= 0 ? TextFormat("+%d", p.valor) : TextFormat("%d", p.valor), q.x, q.y - (1.0f - escala) * 12.0f, UI_TEXTO_GRANDE, c, alfa);
    }
}


static void PrepararHudP(
    const MinijuegoPisotonPlagas& m,
    const JugadorPrueba jugadores[],
    const Participante participantes[],
    int limite,
    DatosHudMinijuego& hud
)
{
    PrepararDatosHudMinijuego(
        hud,
        "PISOTÓN DE PLAGAS",
        "SALTÁ Y SALTÁ DE NUEVO EN EL AIRE PARA APLASTAR: ESCARABAJO 1, ORUGA 2 POR ANILLO, BABOSA DORADA 5. ¡LAS AVISPAS RESTAN 2 Y LA FLOR DA 3!",
        m.resultado
    );

    hud.fase = m.fase == FASE_PISOTON_PREPARACION
        ? HUD_MINIJUEGO_CUENTA
        : (m.fase == FASE_PISOTON_TERMINADO ? HUD_MINIJUEGO_FIN : HUD_MINIJUEGO_JUGANDO);
    hud.cuentaRestante = m.tiempoPreparacion;
    hud.reloj = HUD_RELOJ_CUENTA_ATRAS;
    hud.tiempoRestante = m.tiempoRestante;
    hud.tiempoTotal = DURACION_PARTIDA_PISOTON;
    hud.segundosAlerta = 10.0f;
    hud.finPorTiempo = true;

    for (int i = 0; i < limite; i++)
    {
        if (!m.resultado.participantes[i].participo) continue;

        JugadorHudMinijuego& jugador = hud.jugadores[i];
        const Participante& participante = participantes[i];

        jugador.visible = true;
        jugador.valorNumerico = m.puntos[i];
        std::snprintf(jugador.valor, sizeof(jugador.valor), "%d PTS", m.puntos[i]);
        std::snprintf(jugador.detalleFinal, sizeof(jugador.detalleFinal), "%d PTS", m.puntos[i]);
        std::snprintf(jugador.detalleDesempate, sizeof(jugador.detalleDesempate), "%d DORADAS", m.doradas[i]);

        if (m.fase == FASE_PISOTON_JUGANDO)
        {
            if (m.visual.picado[i] > 0.0f || m.inmunidadAvispa[i] > 0.5f)
                std::snprintf(jugador.estado, sizeof(jugador.estado), "¡PICADO! -%d", PENALIZACION_AVISPA_PISOTON);
            else if (jugadores[i].tiempoRalentizado > 0.0f)
                std::snprintf(jugador.estado, sizeof(jugador.estado), "LENTO");
            else if (m.florGanador == i && m.florAbierta > 0.0f)
                std::snprintf(jugador.estado, sizeof(jugador.estado), "¡FLOR +%d!", PUNTOS_FLOR_PISOTON);
        }

        if (!participante.esBot && participante.conectado)
        {
            std::snprintf(
                jugador.controles,
                sizeof(jugador.controles),
                "MOVÉ: %s\nPISOTÓN: %s x2 · GOLPE: %s",
                TextoMoverHud(participante),
                ObtenerTextoBotonPrincipal(participante),
                TextoBotonGolpeHud(participante)
            );
        }
    }
}


//==================================================
// DIBUJAR
//==================================================

void MinijuegoPisotonPlagas::Dibujar(
    const JugadorPrueba jugadores[],
    int cantidadMaxima,
    const Participante participantes[],
    bool mostrarDebug
) const
{
    int limite = LimitePisoton(cantidadMaxima);
    bool decoracion = CalidadDecoracion() != CALIDAD_BAJA;
    bool efectos = CalidadEfectos() != CALIDAD_BAJA;

    // El bucle del juego no limpia la pantalla: fondo propio siempre.
    ClearBackground(COLOR_HORIZONTE_P);

    Camera3D vista = CamaraVistaP(*this);
    g_ojoP = OJO_PISOTON;
    g_camP = vista.position;
    Vector3 frente = Vector3Normalize(Vector3Subtract(vista.target, vista.position));
    g_derP = Vector3Normalize(Vector3CrossProduct(frente, vista.up));
    g_arribaP = Vector3CrossProduct(g_derP, frente);

    DibujarCieloP(vista, tiempoAnimacion, decoracion);
    HornearPisoton();

    BeginMode3D(vista);

    // El horneado lleva la bruma calculada desde la camara final.
    rlDisableBackfaceCulling();
    DibujarHornoSueloP();
    DibujarMarcasSueloP(*this, jugadores, participantes, limite);
    DibujarHornoEscenaP();
    DibujarAnimadoP(*this, decoracion, efectos);
    rlDrawRenderBatchActive();
    rlEnableBackfaceCulling();

    DibujarMadriguerasP(*this);
    DibujarFlorP(*this);

    for (int p = 0; p < MAX_PLAGAS_PISOTON; p++)
    {
        DibujarPlagaP(plagas[p], tiempoAnimacion);
    }

    for (int i = 0; i < limite; i++)
    {
        if (!resultado.participantes[i].participo) continue;

        Participante visual = participantes[i];
        visual.conectado = true;
        DibujarJugadorModeloCompartido(jugadores[i], visual);

        if (mostrarDebug)
        {
            DrawBoundingBox(CrearHitboxJugadorPrueba(jugadores[i]), LIME);
        }
    }

    for (int a = 0; a < MAX_AVISPAS_PISOTON; a++)
    {
        DibujarAvispaP(avispas[a], tiempoAnimacion);
    }

    rlDisableBackfaceCulling();
    DibujarEfectosP(*this, jugadores, participantes, limite);
    rlDrawRenderBatchActive();

    if (efectos)
    {
        DibujarLucesP(*this, jugadores, limite, decoracion);
    }

    rlEnableBackfaceCulling();

    if (mostrarDebug)
    {
        for (int i = 0; i < cantidadBloques; i++)
        {
            DrawBoundingBox(CrearHitboxBloquePrueba(bloques[i]), YELLOW);
        }

        for (int p = 0; p < MAX_PLAGAS_PISOTON; p++)
        {
            if (!plagas[p].activa) continue;

            for (int s = 0; s < MAX_SEGMENTOS_PISOTON; s++)
            {
                if (plagas[p].segVivo[s])
                {
                    DrawCircle3D({ plagas[p].segX[s], 0.1f, plagas[p].segZ[s] }, RADIO_POUND_PISOTON, { 1.0f, 0.0f, 0.0f }, 90.0f, LIME);
                }
            }
        }
    }

    EndMode3D();

    DibujarAnunciosP(*this, jugadores, participantes, limite, vista);

    if (resultado.estado == RESULTADO_MINIJUEGO_CANCELADO)
    {
        DibujarTextoMundoContornoUI("SE NECESITAN AL MENOS 2 JUGADORES", (float)GetScreenWidth() * 0.5f, (float)GetScreenHeight() * 0.42f, UI_TEXTO_GRANDE, Color{ 255, 120, 110, 255 });
    }

    DatosHudMinijuego hud;
    PrepararHudP(*this, jugadores, participantes, limite, hud);
    DibujarHudMinijuego(hud, participantes);
}


const ResultadoMinijuego& MinijuegoPisotonPlagas::ObtenerResultado() const
{
    return resultado;
}



void MinijuegoPisotonPlagas::Descargar()
{
    LiberarMallasPisoton();
    LiberarMaterialHorneado(g_hornoP.material, g_hornoP.materialListo);
}
