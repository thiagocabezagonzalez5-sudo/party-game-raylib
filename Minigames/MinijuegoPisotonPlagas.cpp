#include "Minigames/MinijuegoPisotonPlagas.h"

#include "Minigames/AudioMinijuegos.h"
#include "Minigames/MecanicasJugador.h"
#include "Minigames/UtilidadesMinijuegos.h"
#include "Systems/Input.h"

#include "raylib.h"

#include <cmath>
#include <cstdio>


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


static const char* NombreJugadorPisoton(const Participante& participante, int indice)
{
    return TextFormat(
        "J%d%s",
        participante.numeroJugador > 0 ? participante.numeroJugador : indice + 1,
        participante.esBot ? " BOT" : ""
    );
}


static const char* TextoGolpePisoton(const Participante& participante)
{
    if (participante.control == CONTROL_GAMEPAD)
    {
        return "B";
    }

    if (participante.control == CONTROL_TECLADO_FLECHAS)
    {
        return "SHIFT DER";
    }

    return "E";
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
// CICLO DE VIDA
//==================================================

void MinijuegoPisotonPlagas::Inicializar()
{
    resultado = {};
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
}


//==================================================
// VISUAL: ESCENARIO (solo decoracion, la logica no depende de esto)
//==================================================

// MODELO FUTURO: briznas de pasto gigantes, flores del fondo y flor de
// floracion, setas, regadera, piedras, gotas de rocio, cerca de madera,
// madrigueras (monticulos), escarabajo, oruga, babosa dorada y avispa:
// reemplazar cada uno por un GLB.

static float HashPisoton(int semilla)
{
    unsigned int x = (unsigned int)semilla * 2654435761u;
    x ^= x >> 15;
    x *= 2246822519u;
    x ^= x >> 13;
    return (float)(x & 0xFFFFu) / 65535.0f;
}


static void DibujarSueloPisoton(const MinijuegoPisotonPlagas& m)
{
    // Cesped lejano y cesped de la arena.
    DrawPlane({ 0.0f, -0.12f, 0.0f }, { 90.0f, 90.0f }, Color{ 58, 118, 46, 255 });

    for (int i = 0; i < m.cantidadBloques; i++)
    {
        DrawCube(m.bloques[i].posicion, m.bloques[i].tamano.x, m.bloques[i].tamano.y, m.bloques[i].tamano.z, m.bloques[i].color);
    }

    // Parches de cesped para dar textura.
    for (int k = 0; k < 18; k++)
    {
        float x = (HashPisoton(k * 3 + 1) * 2.0f - 1.0f) * LIMITE_X_PISOTON;
        float z = (HashPisoton(k * 3 + 2) * 2.0f - 1.0f) * LIMITE_Z_PISOTON;
        float ancho = 1.2f + HashPisoton(k * 3 + 3) * 2.2f;
        Color verde = (k % 2 == 0) ? Color{ 98, 168, 70, 255 } : Color{ 70, 132, 52, 255 };

        DrawCube({ x, SUELO_PISOTON + 0.005f, z }, ancho, 0.01f, ancho * 0.7f, verde);
    }

    // Mechones cortos de pasto dentro de la arena.
    for (int k = 0; k < 26; k++)
    {
        float x = (HashPisoton(k * 5 + 40) * 2.0f - 1.0f) * (LIMITE_X_PISOTON + 0.5f);
        float z = (HashPisoton(k * 5 + 41) * 2.0f - 1.0f) * (LIMITE_Z_PISOTON + 0.3f);
        DrawCylinder({ x, SUELO_PISOTON, z }, 0.0f, 0.07f, 0.3f + HashPisoton(k) * 0.2f, 4, Color{ 110, 184, 76, 255 });
    }

    // Gotas de rocio.
    for (int k = 0; k < 10; k++)
    {
        float x = (HashPisoton(k * 7 + 90) * 2.0f - 1.0f) * LIMITE_X_PISOTON;
        float z = (HashPisoton(k * 7 + 91) * 2.0f - 1.0f) * LIMITE_Z_PISOTON;
        float brillo = 0.5f + 0.5f * std::sin(m.tiempoAnimacion * 2.0f + (float)k);
        DrawSphere({ x, SUELO_PISOTON + 0.09f, z }, 0.09f, Fade(Color{ 190, 230, 255, 255 }, 0.55f + 0.3f * brillo));
    }
}


static void DibujarMadriguerasPisoton(const MinijuegoPisotonPlagas& m)
{
    for (int k = 0; k < MAX_MADRIGUERAS_PISOTON; k++)
    {
        const MadriguerasPisoton& madriguera = m.madrigueras[k];
        float temblor = 0.0f;
        float elevacion = 0.0f;

        if (madriguera.aviso > 0.0f)
        {
            temblor = std::sin(m.tiempoAnimacion * 60.0f + (float)k) * 0.06f;
            elevacion = 0.15f * (1.0f - madriguera.aviso / DURACION_AVISO_MADRIGUERA_PISOTON);
        }

        DrawCylinder({ madriguera.x + temblor, SUELO_PISOTON, madriguera.z }, 0.35f + elevacion, 0.7f, 0.12f + elevacion, 12, Color{ 118, 82, 50, 255 });
        DrawCylinder({ madriguera.x, SUELO_PISOTON + 0.115f + elevacion, madriguera.z }, 0.3f, 0.3f, 0.01f, 12, Color{ 40, 26, 18, 255 });

        if (madriguera.aviso > 0.0f)
        {
            float pulso = 0.5f + 0.5f * std::sin(m.tiempoAnimacion * 14.0f);
            DrawCircle3D({ madriguera.x, SUELO_PISOTON + 0.03f, madriguera.z }, 0.85f, { 1.0f, 0.0f, 0.0f }, 90.0f, Fade(ORANGE, 0.5f + 0.4f * pulso));
        }
    }
}


static void DibujarCercaPisoton()
{
    const float zFondo = -LIMITE_Z_PISOTON - 1.3f;

    // Fondo.
    for (int k = -10; k <= 10; k++)
    {
        float x = (float)k * 1.05f;
        DrawCube({ x, 0.75f, zFondo }, 0.22f, 1.5f, 0.14f, Color{ 160, 112, 66, 255 });
        DrawCylinder({ x, 1.5f, zFondo }, 0.0f, 0.15f, 0.2f, 4, Color{ 160, 112, 66, 255 });
    }

    DrawCube({ 0.0f, 1.15f, zFondo }, 22.0f, 0.18f, 0.1f, Color{ 190, 138, 84, 255 });
    DrawCube({ 0.0f, 0.55f, zFondo }, 22.0f, 0.18f, 0.1f, Color{ 190, 138, 84, 255 });

    // Laterales.
    for (int lado = -1; lado <= 1; lado += 2)
    {
        float x = (float)lado * (LIMITE_X_PISOTON + 1.4f);

        for (int k = -5; k <= 5; k++)
        {
            DrawCube({ x, 0.6f, (float)k * 1.05f }, 0.14f, 1.2f, 0.22f, Color{ 160, 112, 66, 255 });
        }

        DrawCube({ x, 0.95f, 0.0f }, 0.1f, 0.16f, 11.5f, Color{ 190, 138, 84, 255 });
        DrawCube({ x, 0.45f, 0.0f }, 0.1f, 0.16f, 11.5f, Color{ 190, 138, 84, 255 });
    }
}


static void DibujarVegetacionPisoton(const MinijuegoPisotonPlagas& m)
{
    // Briznas gigantes tras la cerca y a los lados.
    for (int k = 0; k < 34; k++)
    {
        float lado = HashPisoton(k * 11 + 5);
        float x = 0.0f;
        float z = 0.0f;

        if (k < 22)
        {
            x = (float)(k - 11) * 1.9f + HashPisoton(k * 11 + 6);
            z = -LIMITE_Z_PISOTON - 2.4f - lado * 2.0f;
        }
        else
        {
            x = (k % 2 == 0 ? -1.0f : 1.0f) * (LIMITE_X_PISOTON + 2.4f + lado * 2.5f);
            z = (HashPisoton(k * 11 + 7) * 2.0f - 1.0f) * 7.0f;
        }

        float alto = 3.0f + HashPisoton(k * 11 + 8) * 3.2f;
        float mecer = std::sin(m.tiempoAnimacion * 1.2f + (float)k) * 0.15f;
        Color verde = (k % 3 == 0) ? Color{ 52, 130, 50, 255 } : ((k % 3 == 1) ? Color{ 70, 150, 58, 255 } : Color{ 40, 110, 44, 255 });

        DrawCylinder({ x + mecer, 0.0f, z }, 0.0f, 0.45f, alto, 5, verde);
    }

    // Flores gigantes del fondo.
    const float floresX[3] = { -7.0f, 1.5f, 8.0f };
    const Color petalos[3] = { Color{ 240, 90, 130, 255 }, Color{ 250, 200, 60, 255 }, Color{ 150, 110, 230, 255 } };

    for (int f = 0; f < 3; f++)
    {
        float x = floresX[f];
        float z = -LIMITE_Z_PISOTON - 3.8f - (float)(f % 2) * 1.2f;
        float alto = 5.0f + (float)f * 0.6f;

        DrawCylinder({ x, 0.0f, z }, 0.14f, 0.2f, alto, 6, Color{ 60, 140, 52, 255 });

        for (int p = 0; p < 8; p++)
        {
            float angulo = (float)p * 2.0f * PI / 8.0f;
            DrawSphere({ x + std::cos(angulo) * 1.15f, alto, z + std::sin(angulo) * 0.35f + 0.3f }, 0.62f, petalos[f]);
        }

        DrawSphere({ x, alto, z + 0.35f }, 0.6f, Color{ 120, 70, 30, 255 });
    }

    // Setas.
    const float setasX[3] = { -10.8f, 10.9f, 10.6f };
    const float setasZ[3] = { 3.8f, 2.6f, -1.2f };

    for (int s = 0; s < 3; s++)
    {
        DrawCylinder({ setasX[s], 0.0f, setasZ[s] }, 0.2f, 0.25f, 0.7f, 8, Color{ 235, 225, 205, 255 });
        DrawCylinder({ setasX[s], 0.7f, setasZ[s] }, 0.0f, 0.75f, 0.45f, 10, Color{ 210, 60, 52, 255 });
        DrawSphere({ setasX[s] + 0.25f, 0.95f, setasZ[s] + 0.2f }, 0.1f, WHITE);
        DrawSphere({ setasX[s] - 0.2f, 0.9f, setasZ[s] - 0.15f }, 0.08f, WHITE);
    }

    // Regadera olvidada.
    DrawCube({ -11.2f, 0.5f, -3.0f }, 1.1f, 1.0f, 0.8f, Color{ 70, 130, 200, 255 });
    DrawCylinderEx({ -10.6f, 0.7f, -3.0f }, { -9.9f, 1.3f, -3.0f }, 0.08f, 0.14f, 6, Color{ 60, 116, 180, 255 });
    DrawCylinderEx({ -11.8f, 0.9f, -3.0f }, { -11.2f, 1.25f, -3.0f }, 0.06f, 0.06f, 6, Color{ 60, 116, 180, 255 });

    // Piedras.
    const float piedrasX[5] = { 10.4f, -10.5f, 6.5f, -5.5f, 11.4f };
    const float piedrasZ[5] = { 5.2f, -0.2f, 7.2f, 7.4f, -4.5f };

    for (int r = 0; r < 5; r++)
    {
        DrawSphere({ piedrasX[r], 0.2f, piedrasZ[r] }, 0.55f - (float)(r % 2) * 0.12f, Color{ 128, 128, 134, 255 });
    }
}


//==================================================
// VISUAL: ENTIDADES
//==================================================

static void DibujarEstrellasAturdidoPisoton(float x, float z, float altura, float tiempo)
{
    for (int k = 0; k < 3; k++)
    {
        float angulo = tiempo * 6.0f + (float)k * 2.0944f;
        DrawSphere({ x + std::cos(angulo) * 0.35f, altura, z + std::sin(angulo) * 0.35f }, 0.07f, YELLOW);
    }
}


static void DibujarPlagaPisoton(const PlagaPisoton& plaga, float tiempo)
{
    if (!plaga.activa)
    {
        return;
    }

    float escala = LimitarPisoton(plaga.aparicion, 0.0f, 1.0f) * LimitarPisoton(plaga.escala, 0.0f, 1.0f);

    if (escala <= 0.02f)
    {
        return;
    }

    bool aturdida = plaga.aturdida > 0.0f;
    bool destello = aturdida && std::fmod(tiempo * 10.0f, 2.0f) < 1.0f;
    float fx = plaga.dirX;
    float fz = plaga.dirZ;
    float sx = -fz;
    float sz = fx;
    float px = 0.0f;
    float pz = 0.0f;
    PosicionPlagaPisoton(plaga, px, pz);

    float oscila = aturdida ? 0.0f : std::sin(plaga.edad * 14.0f);

    if (plaga.tipo == PLAGA_PISOTON_ESCARABAJO)
    {
        float x = plaga.segX[0];
        float z = plaga.segZ[0];
        Color cuerpo = destello ? Color{ 255, 230, 200, 255 } : Color{ 150, 38, 30, 255 };

        DrawCircle3D({ x, SUELO_PISOTON + 0.02f, z }, 0.38f * escala, { 1.0f, 0.0f, 0.0f }, 90.0f, Fade(BLACK, 0.3f));
        DrawSphere({ x, 0.27f * escala, z }, 0.3f * escala, cuerpo);
        DrawSphere({ x + fx * 0.3f * escala, 0.24f * escala, z + fz * 0.3f * escala }, 0.16f * escala, Color{ 30, 20, 20, 255 });
        DrawSphere({ x - fx * 0.05f + sx * 0.12f, 0.5f * escala, z - fz * 0.05f + sz * 0.12f }, 0.06f * escala, BLACK);
        DrawSphere({ x - fx * 0.05f - sx * 0.12f, 0.5f * escala, z - fz * 0.05f - sz * 0.12f }, 0.06f * escala, BLACK);
        DrawLine3D({ x, 0.45f * escala, z }, { x - fx * 0.28f, 0.2f * escala, z - fz * 0.28f }, Color{ 90, 20, 16, 255 });

        for (int p = -1; p <= 1; p++)
        {
            float pata = (float)p * 0.2f;
            float meneo = oscila * 0.08f * (float)(p == 0 ? -1 : 1);

            for (int lado = -1; lado <= 1; lado += 2)
            {
                DrawLine3D(
                    { x + fx * pata, 0.15f * escala, z + fz * pata },
                    { x + fx * (pata + meneo) + sx * 0.42f * (float)lado * escala, 0.03f, z + fz * (pata + meneo) + sz * 0.42f * (float)lado * escala },
                    Color{ 40, 24, 20, 255 }
                );
            }
        }

        // Cuernos.
        DrawLine3D(
            { x + fx * 0.4f * escala, 0.28f * escala, z + fz * 0.4f * escala },
            { x + fx * 0.62f * escala + sx * 0.1f, 0.4f * escala, z + fz * 0.62f * escala + sz * 0.1f },
            Color{ 40, 24, 20, 255 }
        );
        DrawLine3D(
            { x + fx * 0.4f * escala, 0.28f * escala, z + fz * 0.4f * escala },
            { x + fx * 0.62f * escala - sx * 0.1f, 0.4f * escala, z + fz * 0.62f * escala - sz * 0.1f },
            Color{ 40, 24, 20, 255 }
        );

        if (aturdida)
        {
            DibujarEstrellasAturdidoPisoton(x, z, 0.85f, tiempo);
        }
    }
    else if (plaga.tipo == PLAGA_PISOTON_ORUGA)
    {
        for (int s = MAX_SEGMENTOS_PISOTON - 1; s >= 0; s--)
        {
            if (!plaga.segVivo[s])
            {
                continue;
            }

            float x = plaga.segX[s];
            float z = plaga.segZ[s];
            float bote = aturdida ? 0.0f : std::fabs(std::sin(plaga.edad * 9.0f + (float)s * 1.2f)) * 0.07f;
            Color verde = (s % 2 == 0) ? Color{ 120, 200, 70, 255 } : Color{ 90, 170, 56, 255 };

            if (destello)
            {
                verde = Color{ 230, 255, 200, 255 };
            }

            DrawCircle3D({ x, SUELO_PISOTON + 0.02f, z }, 0.3f * escala, { 1.0f, 0.0f, 0.0f }, 90.0f, Fade(BLACK, 0.3f));
            DrawSphere({ x, (0.27f + bote) * escala, z }, 0.27f * escala, verde);
        }

        // Cara en el primer segmento vivo.
        for (int s = 0; s < MAX_SEGMENTOS_PISOTON; s++)
        {
            if (!plaga.segVivo[s])
            {
                continue;
            }

            float x = plaga.segX[s];
            float z = plaga.segZ[s];
            DrawSphere({ x + fx * 0.2f + sx * 0.11f, 0.4f * escala, z + fz * 0.2f + sz * 0.11f }, 0.07f * escala, WHITE);
            DrawSphere({ x + fx * 0.2f - sx * 0.11f, 0.4f * escala, z + fz * 0.2f - sz * 0.11f }, 0.07f * escala, WHITE);
            DrawLine3D({ x, 0.5f * escala, z }, { x + fx * 0.25f + sx * 0.12f, 0.75f * escala, z + fz * 0.25f + sz * 0.12f }, Color{ 50, 100, 40, 255 });
            DrawLine3D({ x, 0.5f * escala, z }, { x + fx * 0.25f - sx * 0.12f, 0.75f * escala, z + fz * 0.25f - sz * 0.12f }, Color{ 50, 100, 40, 255 });

            if (aturdida)
            {
                DibujarEstrellasAturdidoPisoton(x, z, 0.8f, tiempo);
            }

            break;
        }
    }
    else
    {
        float x = plaga.segX[0];
        float z = plaga.segZ[0];
        Color oro = destello ? Color{ 255, 255, 220, 255 } : Color{ 255, 205, 40, 255 };
        float estira = aturdida ? 0.0f : std::sin(plaga.edad * 5.0f) * 0.04f;

        DrawCircle3D({ x - fx * 0.2f, SUELO_PISOTON + 0.02f, z - fz * 0.2f }, 0.5f * escala, { 1.0f, 0.0f, 0.0f }, 90.0f, Fade(BLACK, 0.3f));
        DrawSphere({ x, 0.3f * escala, z }, 0.3f * escala, oro);
        DrawSphere({ x - fx * (0.3f + estira), 0.25f * escala, z - fz * (0.3f + estira) }, 0.26f * escala, oro);
        DrawSphere({ x - fx * 0.55f, 0.19f * escala, z - fz * 0.55f }, 0.2f * escala, Color{ 240, 180, 30, 255 });
        DrawSphere({ x + fx * 0.05f, 0.45f * escala, z + fz * 0.05f }, 0.12f * escala, Color{ 255, 240, 160, 255 });

        // Tentaculos con ojos.
        for (int lado = -1; lado <= 1; lado += 2)
        {
            float ex = x + fx * 0.28f + sx * 0.12f * (float)lado;
            float ez = z + fz * 0.28f + sz * 0.12f * (float)lado;
            DrawLine3D({ x + fx * 0.2f, 0.4f * escala, z + fz * 0.2f }, { ex, 0.75f * escala, ez }, Color{ 200, 140, 20, 255 });
            DrawSphere({ ex, 0.78f * escala, ez }, 0.06f * escala, BLACK);
        }

        // Destellos de brillo.
        for (int k = 0; k < 2; k++)
        {
            float angulo = tiempo * 3.0f + (float)k * PI;
            DrawSphere({ x + std::cos(angulo) * 0.6f, 0.6f + 0.1f * std::sin(tiempo * 8.0f + (float)k), z + std::sin(angulo) * 0.6f }, 0.05f, WHITE);
        }

        if (aturdida)
        {
            DibujarEstrellasAturdidoPisoton(x, z, 0.95f, tiempo);
        }
    }

    (void)px;
    (void)pz;
}


static void DibujarAvispaPisoton(const AvispaPisoton& avispa, float tiempo)
{
    if (!avispa.activa)
    {
        return;
    }

    float fx = avispa.dirX;
    float fz = avispa.dirZ;
    float sx = -fz;
    float sz = fx;
    float aleteo = std::sin(tiempo * 55.0f) * 0.12f;
    bool asustada = avispa.huida > 0.0f;
    Color amarillo = asustada ? Color{ 255, 150, 90, 255 } : Color{ 255, 214, 40, 255 };

    DrawCircle3D({ avispa.x, SUELO_PISOTON + 0.02f, avispa.z }, 0.3f, { 1.0f, 0.0f, 0.0f }, 90.0f, Fade(BLACK, 0.3f));
    DrawSphere({ avispa.x + fx * 0.24f, avispa.altura, avispa.z + fz * 0.24f }, 0.13f, Color{ 30, 24, 20, 255 });
    DrawSphere({ avispa.x, avispa.altura, avispa.z }, 0.17f, Color{ 30, 24, 20, 255 });
    DrawSphere({ avispa.x - fx * 0.25f, avispa.altura, avispa.z - fz * 0.25f }, 0.2f, amarillo);
    DrawSphere({ avispa.x - fx * 0.2f, avispa.altura, avispa.z - fz * 0.2f }, 0.205f, Color{ 30, 24, 20, 255 });
    DrawSphere({ avispa.x - fx * 0.3f, avispa.altura, avispa.z - fz * 0.3f }, 0.2f, amarillo);
    DrawLine3D(
        { avispa.x - fx * 0.44f, avispa.altura, avispa.z - fz * 0.44f },
        { avispa.x - fx * 0.62f, avispa.altura - 0.05f, avispa.z - fz * 0.62f },
        Color{ 40, 20, 16, 255 }
    );

    for (int lado = -1; lado <= 1; lado += 2)
    {
        DrawCube(
            { avispa.x + sx * 0.22f * (float)lado, avispa.altura + 0.17f + aleteo, avispa.z + sz * 0.22f * (float)lado },
            0.3f, 0.02f, 0.18f,
            Fade(Color{ 220, 240, 255, 255 }, 0.7f)
        );
    }

    // Marca de peligro sobre la avispa.
    if (!asustada)
    {
        DrawSphere({ avispa.x, avispa.altura + 0.5f, avispa.z }, 0.07f, RED);
    }
}


static void DibujarFlorPisoton(const MinijuegoPisotonPlagas& m)
{
    bool aviso = m.tiempoFlor <= AVISO_FLOR_PISOTON;

    if (!aviso && m.florAbierta <= 0.0f)
    {
        return;
    }

    float x = m.florX;
    float z = m.florZ;
    float crecimiento = m.florAbierta > 0.0f
        ? 1.0f
        : LimitarPisoton(1.0f - m.tiempoFlor / AVISO_FLOR_PISOTON, 0.0f, 1.0f);
    float abierta = m.florAbierta > 0.0f
        ? 1.0f
        : LimitarPisoton((crecimiento - 0.6f) / 0.4f, 0.0f, 1.0f);
    float alto = 0.4f + 2.2f * crecimiento;

    // Zona de disputa en el suelo.
    float pulso = 0.5f + 0.5f * std::sin(m.tiempoAnimacion * (4.0f + 10.0f * crecimiento));
    DrawCylinder({ x, SUELO_PISOTON + 0.01f, z }, RADIO_FLOR_PISOTON, RADIO_FLOR_PISOTON, 0.015f, 28, Fade(Color{ 255, 150, 210, 255 }, 0.25f + 0.2f * pulso));
    DrawCircle3D({ x, SUELO_PISOTON + 0.05f, z }, RADIO_FLOR_PISOTON, { 1.0f, 0.0f, 0.0f }, 90.0f, Fade(Color{ 255, 110, 190, 255 }, 0.7f + 0.3f * pulso));

    DrawCylinder({ x, SUELO_PISOTON, z }, 0.08f, 0.12f, alto, 6, Color{ 70, 150, 56, 255 });

    if (abierta <= 0.0f)
    {
        DrawSphere({ x, alto + 0.12f, z }, 0.22f + 0.2f * crecimiento, Color{ 100, 180, 70, 255 });
        return;
    }

    for (int p = 0; p < 8; p++)
    {
        float angulo = (float)p * 2.0f * PI / 8.0f + m.tiempoAnimacion * 0.4f;
        DrawSphere(
            { x + std::cos(angulo) * 0.75f * abierta, alto + 0.05f, z + std::sin(angulo) * 0.75f * abierta },
            0.38f,
            (p % 2 == 0) ? Color{ 255, 120, 190, 255 } : Color{ 255, 160, 215, 255 }
        );
    }

    DrawSphere({ x, alto + 0.12f, z }, 0.4f, Color{ 255, 210, 60, 255 });
}


//==================================================
// DIBUJO
//==================================================

void MinijuegoPisotonPlagas::Dibujar(
    const JugadorPrueba jugadores[],
    int cantidadMaxima,
    const Participante participantes[],
    bool mostrarDebug
) const
{
    int limite = LimitePisoton(cantidadMaxima);
    int anchoPantalla = GetScreenWidth();
    int altoPantalla = GetScreenHeight();

    ClearBackground(Color{ 150, 205, 235, 255 });
    BeginMode3D(camara);

    DibujarSueloPisoton(*this);
    DibujarMadriguerasPisoton(*this);
    DibujarCercaPisoton();
    DibujarVegetacionPisoton(*this);
    DibujarFlorPisoton(*this);

    for (int p = 0; p < MAX_PLAGAS_PISOTON; p++)
    {
        DibujarPlagaPisoton(plagas[p], tiempoAnimacion);
    }

    for (int i = 0; i < limite; i++)
    {
        if (!resultado.participantes[i].participo)
        {
            continue;
        }

        const JugadorPrueba& jugador = jugadores[i];

        DrawCircle3D({ jugador.posicion.x, 0.03f, jugador.posicion.z }, 0.62f, { 1.0f, 0.0f, 0.0f }, 90.0f, participantes[i].color);
        DrawCircle3D({ jugador.posicion.x, 0.03f, jugador.posicion.z }, 0.52f, { 1.0f, 0.0f, 0.0f }, 90.0f, WHITE);

        Participante visual = participantes[i];
        visual.conectado = true;
        DibujarJugadorCuboPrueba(jugador, visual);

        if (mostrarDebug)
        {
            DrawBoundingBox(CrearHitboxJugadorPrueba(jugador), LIME);
        }
    }

    for (int a = 0; a < MAX_AVISPAS_PISOTON; a++)
    {
        DibujarAvispaPisoton(avispas[a], tiempoAnimacion);
    }

    DibujarParticulasTierra(particulas, MAX_PARTICULAS_PISOTON);

    if (mostrarDebug)
    {
        for (int i = 0; i < cantidadBloques; i++)
        {
            DrawBoundingBox(CrearHitboxBloquePrueba(bloques[i]), YELLOW);
        }

        for (int p = 0; p < MAX_PLAGAS_PISOTON; p++)
        {
            if (!plagas[p].activa)
            {
                continue;
            }

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

    // Etiquetas flotantes de jugadores.
    for (int i = 0; i < limite; i++)
    {
        if (!resultado.participantes[i].participo)
        {
            continue;
        }

        Vector2 pantalla = GetWorldToScreen(
            { jugadores[i].posicion.x, jugadores[i].posicion.y + 1.35f, jugadores[i].posicion.z },
            camara
        );
        const char* nombre = NombreJugadorPisoton(participantes[i], i);
        int ancho = MeasureText(nombre, 18);

        DrawRectangle((int)pantalla.x - ancho / 2 - 4, (int)pantalla.y - 2, ancho + 8, 22, Fade(BLACK, 0.55f));
        DrawText(nombre, (int)pantalla.x - ancho / 2, (int)pantalla.y, 18, participantes[i].color);
    }

    // Puntos flotantes.
    for (int k = 0; k < MAX_POPUPS_PISOTON; k++)
    {
        if (!popups[k].activo)
        {
            continue;
        }

        Vector2 pantalla = GetWorldToScreen(popups[k].posicion, camara);
        const char* texto = popups[k].valor >= 0
            ? TextFormat("+%d", popups[k].valor)
            : TextFormat("%d", popups[k].valor);
        int ancho = MeasureText(texto, 30);

        DrawText(texto, (int)pantalla.x - ancho / 2 + 2, (int)pantalla.y + 2, 30, Fade(BLACK, 0.6f));
        DrawText(texto, (int)pantalla.x - ancho / 2, (int)pantalla.y, 30, Fade(popups[k].color, LimitarPisoton(popups[k].vida * 2.0f, 0.0f, 1.0f)));
    }

    // Cuenta atras de la flor sobre la flor.
    if (fase == FASE_PISOTON_JUGANDO && tiempoFlor <= AVISO_FLOR_PISOTON)
    {
        Vector2 pantalla = GetWorldToScreen({ florX, 3.2f, florZ }, camara);
        const char* texto = TextFormat("FLOR +%d  %.1f", PUNTOS_FLOR_PISOTON, tiempoFlor > 0.0f ? tiempoFlor : 0.0f);
        int ancho = MeasureText(texto, 20);

        DrawRectangle((int)pantalla.x - ancho / 2 - 6, (int)pantalla.y - 3, ancho + 12, 26, Fade(BLACK, 0.65f));
        DrawText(texto, (int)pantalla.x - ancho / 2, (int)pantalla.y, 20, Color{ 255, 160, 220, 255 });
    }

    // Cabecera.
    DrawRectangle(14, 12, 520, 92, Fade(BLACK, 0.74f));
    DrawText("PISOTON DE PLAGAS", 28, 20, 28, GOLD);
    DrawText("SALTO + SALTO EN EL AIRE = POUND   GOLPE ESPANTA AVISPAS", 28, 54, 15, RAYWHITE);
    DrawText("MOVER: WASD/FLECHAS/STICK   CUIDADO CON LAS AVISPAS (-2)", 28, 76, 14, LIGHTGRAY);

    if (fase == FASE_PISOTON_JUGANDO)
    {
        DrawRectangle(anchoPantalla - 214, 12, 200, 44, Fade(BLACK, 0.74f));
        DrawText(
            TextFormat("TIEMPO %.1f", tiempoRestante),
            anchoPantalla - 200,
            22,
            26,
            tiempoRestante <= 6.0f ? RED : GOLD
        );

        const char* aviso = nullptr;
        Color colorAviso = RAYWHITE;

        if (tiempoFlor <= AVISO_FLOR_PISOTON && tiempoFlor > 0.0f)
        {
            aviso = "LA FLOR VA A FLORECER: PARATE DEBAJO!";
            colorAviso = Color{ 255, 160, 220, 255 };
        }
        else
        {
            for (int k = 0; k < MAX_MADRIGUERAS_PISOTON; k++)
            {
                if (madrigueras[k].aviso > 0.0f && madrigueras[k].tipoPendiente == PLAGA_PISOTON_BABOSA)
                {
                    aviso = "SE MUEVE LA TIERRA... BABOSA DORADA!";
                    colorAviso = GOLD;
                }
            }
        }

        if (aviso != nullptr)
        {
            int a = MeasureText(aviso, 22);
            DrawRectangle(anchoPantalla / 2 - a / 2 - 14, 112, a + 28, 36, Fade(BLACK, 0.8f));
            DrawText(aviso, anchoPantalla / 2 - a / 2, 119, 22, colorAviso);
        }
    }

    // Tarjetas inferiores.
    int activos = 0;

    for (int i = 0; i < limite; i++)
    {
        if (resultado.participantes[i].participo) activos++;
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
            if (!resultado.participantes[i].participo)
            {
                continue;
            }

            int x = xInicial + k * (anchoTarjeta + separacion);
            int y = altoPantalla - 92;

            DrawRectangle(x, y, anchoTarjeta, 80, Fade(BLACK, 0.78f));
            DrawRectangle(x, y, 8, 80, participantes[i].color);
            DrawText(NombreJugadorPisoton(participantes[i], i), x + 18, y + 8, 20, participantes[i].color);
            DrawText(TextFormat("%d PTS", puntos[i]), x + 18, y + 32, 22, RAYWHITE);
            DrawText(TextFormat("DORADAS %d", doradas[i]), x + anchoTarjeta - 110, y + 36, 14, GOLD);

            if (inmunidadAvispa[i] > 0.5f)
            {
                DrawText("PICADO!", x + anchoTarjeta - 80, y + 10, 16, RED);
            }

            if (participantes[i].esBot)
            {
                DrawText("CONTROL: BOT", x + 18, y + 60, 14, LIGHTGRAY);
            }
            else
            {
                DrawText(
                    TextFormat("GOLPE: %s   SALTO: %s", TextoGolpePisoton(participantes[i]), ObtenerTextoBotonPrincipal(participantes[i])),
                    x + 18,
                    y + 60,
                    14,
                    LIGHTGRAY
                );
            }

            k++;
        }
    }

    if (fase == FASE_PISOTON_PREPARACION)
    {
        int numero = (int)std::ceil(tiempoPreparacion);

        if (numero < 1) numero = 1;

        const char* texto = TextFormat("%d", numero);
        DrawText(texto, anchoPantalla / 2 - MeasureText(texto, 96) / 2, altoPantalla / 2 - 90, 96, GOLD);

        const char* ayuda = "PISA PLAGAS CON GROUND POUND: SALTA Y SALTA DE NUEVO EN EL AIRE";
        DrawText(ayuda, anchoPantalla / 2 - MeasureText(ayuda, 20) / 2, altoPantalla / 2 + 20, 20, RAYWHITE);

        const char* ayuda2 = "ESCARABAJO 1 / ORUGA 2 POR SEGMENTO / BABOSA DORADA 5";
        DrawText(ayuda2, anchoPantalla / 2 - MeasureText(ayuda2, 18) / 2, altoPantalla / 2 + 48, 18, LIGHTGRAY);
    }
    else if (
        fase == FASE_PISOTON_TERMINADO &&
        resultado.estado == RESULTADO_MINIJUEGO_FINALIZADO
    )
    {
        int panelAncho = 560;
        int panelAlto = 130 + 30 * activos;
        int px = anchoPantalla / 2 - panelAncho / 2;
        int py = altoPantalla / 2 - panelAlto / 2 - 20;

        DrawRectangle(px, py, panelAncho, panelAlto, Fade(BLACK, 0.9f));

        int ganadores[MAX_PARTICIPANTES]{};
        int cantidadGanadores = ObtenerIndicesGanadores(resultado, ganadores, MAX_PARTICIPANTES);
        const char* titulo = "EMPATE";

        if (resultado.desenlace == DESENLACE_CON_GANADOR && cantidadGanadores == 1)
        {
            titulo = TextFormat("GANA %s", NombreJugadorPisoton(participantes[ganadores[0]], ganadores[0]));
        }

        DrawText(titulo, anchoPantalla / 2 - MeasureText(titulo, 32) / 2, py + 14, 32, GOLD);

        int fila = 0;

        for (int posicion = 1; posicion <= MAX_PARTICIPANTES; posicion++)
        {
            for (int i = 0; i < limite; i++)
            {
                if (
                    !resultado.participantes[i].participo ||
                    resultado.participantes[i].posicionFinal != posicion
                )
                {
                    continue;
                }

                DrawText(
                    TextFormat(
                        "%d.  %s   %d PTS   (%d DORADAS)",
                        posicion,
                        NombreJugadorPisoton(participantes[i], i),
                        puntos[i],
                        doradas[i]
                    ),
                    px + 40,
                    py + 62 + fila * 30,
                    22,
                    participantes[i].color
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


const ResultadoMinijuego& MinijuegoPisotonPlagas::ObtenerResultado() const
{
    return resultado;
}
