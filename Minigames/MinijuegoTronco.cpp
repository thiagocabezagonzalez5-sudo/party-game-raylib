#include "Minigames/MinijuegoTronco.h"

#include "Minigames/AudioMinijuegos.h"
#include "Minigames/EfectosVisualesMinijuegos.h"
#include "Minigames/HudMinijuego.h"
#include "Minigames/UtilidadesMinijuegos.h"
#include "Systems/CalidadGrafica.h"
#include "UI/EstiloUI.h"

#include "rlgl.h"

#include <cmath>
#include <cstdio>


static const float DURACION_PREPARACION_TRONCO = 2.5f;

// Tope duro: si ningun equipo termina de cortar, gana el que mas avanzo.
static const float DURACION_MAXIMA_TRONCO = 90.0f;
static const float VENTANA_COORDINACION_TRONCO = 0.34f;
static const float BLOQUEO_FALLO_TRONCO = 0.26f;
static const float AVANCE_CORTE_TRONCO = 0.065f;
static const float VELOCIDAD_SIERRA_VISUAL_TRONCO = 5.4f;

// Debe ser menor que VENTANA_COORDINACION_TRONCO para que un bot
// alcance a acompanar el golpe iniciado por su companero.
static const float REACCION_MAXIMA_BOT_TRONCO = 0.22f;
static const int PROBABILIDAD_ERROR_BOT_TRONCO = 6;

static const float CENTRO_EQUIPO_TRONCO[2] =
{
    -3.9f,
    3.9f
};


//==================================================
// GEOMETRIA Y COLORES DE LAS ESTACIONES (solo visual)
//==================================================

static const float RADIO_TRONCO = 0.52f;
static const float ALTURA_EJE_TRONCO = 0.76f;
static const float MEDIO_LARGO_TRONCO = 1.70f;
static const float ALTURA_TARIMA_TRONCO = 0.06f;
static const float LARGO_SIERRA_TRONCO = 3.10f;
static const float AMPLITUD_SIERRA_TRONCO = 0.50f;
static const float DURACION_CAIDA_TRONCO = 0.55f;

// Cada equipo tiene un color fijo (los bots comparten gris, asi que no
// sirve el color del primer integrante). Se usa solo en lo que es suyo:
// borde de la tarima, bandera, caballetes y empunaduras de la sierra.
static const Color COLOR_EQUIPO_TRONCO[2] =
{
    { 208, 76, 60, 255 },
    { 62, 128, 210, 255 }
};

static const char* const NOMBRE_EQUIPO_TRONCO[2] =
{
    "EQUIPO ROJO",
    "EQUIPO AZUL"
};

static const Color COLOR_ASERRIN_TRONCO = { 226, 190, 128, 255 };
static const Color COLOR_NIEBLA_TRONCO = { 214, 160, 132, 255 };


static float LimitarTronco(float valor, float minimo, float maximo)
{
    return valor < minimo ? minimo : (valor > maximo ? maximo : valor);
}


static float Hash01Tronco(int a, int b)
{
    unsigned int h = (unsigned int)a * 374761393u + (unsigned int)b * 668265263u;
    h = (h ^ (h >> 13)) * 1274126177u;
    h ^= h >> 16;
    return (float)(h & 0xFFFF) / 65535.0f;
}


static float AzarTronco(float minimo, float maximo)
{
    return minimo + (maximo - minimo) * (float)GetRandomValue(0, 10000) / 10000.0f;
}


static Color TinteTronco(Color c, float f)
{
    float r = c.r * f;
    float g = c.g * f;
    float b = c.b * f;
    return Color
    {
        (unsigned char)(r > 255.0f ? 255.0f : r),
        (unsigned char)(g > 255.0f ? 255.0f : g),
        (unsigned char)(b > 255.0f ? 255.0f : b),
        c.a
    };
}


static Color MezclaTronco(Color a, Color b, float t)
{
    t = LimitarTronco(t, 0.0f, 1.0f);
    return Color
    {
        (unsigned char)(a.r + (b.r - a.r) * t),
        (unsigned char)(a.g + (b.g - a.g) * t),
        (unsigned char)(a.b + (b.b - a.b) * t),
        (unsigned char)(a.a + (b.a - a.a) * t)
    };
}


// Bruma de atardecer: lo lejano se funde con el color del horizonte.
static Color NieblaTronco(Color color, float z)
{
    return MezclaTronco(color, COLOR_NIEBLA_TRONCO, LimitarTronco((-z - 6.0f) / 20.0f, 0.0f, 0.80f));
}


// Altura del filo de los dientes: baja desde el lomo del tronco hasta su
// base a medida que avanza el corte.
static float AlturaDientesTronco(float progreso)
{
    return ALTURA_EJE_TRONCO + RADIO_TRONCO - progreso * 2.0f * RADIO_TRONCO;
}


static float DesplazamientoSierraTronco(const EstadoEquipoTronco& estado, float tiempo)
{
    float desplazamiento = estado.posicionSierra * AMPLITUD_SIERRA_TRONCO;

    if (estado.sacudida > 0.0f)
    {
        desplazamiento += std::sin(tiempo * 83.0f) * 0.05f * (estado.sacudida / 0.38f)
            * ObtenerFactorMovimientoCamaraMinijuegos();
    }

    return desplazamiento;
}


static Vector3 ObtenerPosicionJugadorTronco(
    int equipo,
    int ordenEnEquipo,
    int cantidadJugadoresEquipo
)
{
    float centroX = CENTRO_EQUIPO_TRONCO[equipo];

    if (cantidadJugadoresEquipo <= 1)
    {
        return { centroX - 1.85f, 0.76f, 0.15f };
    }

    return
    {
        centroX + (ordenEnEquipo == 0 ? -1.85f : 1.85f),
        0.76f,
        0.15f
    };
}


//==================================================
// PARTICULAS (pool fijo, escalado por calidad)
//==================================================

static int CantidadParticulasTronco(int cantidad)
{
    int resultado = (int)std::lround((float)cantidad * FactorCalidadGrafica(CalidadParticulas()));
    return resultado < 1 ? 1 : resultado;
}


static void EmitirParticulaTronco(
    MinijuegoTronco& minijuego,
    int tipo,
    Vector3 posicion,
    Vector3 velocidad,
    float vida,
    float tamano,
    Color color
)
{
    for (int i = 0; i < MAX_PARTICULAS_TRONCO; i++)
    {
        ParticulaTronco& p = minijuego.particulas[i];
        if (p.activa) continue;

        p.activa = true;
        p.tipo = tipo;
        p.posicion = posicion;
        p.velocidad = velocidad;
        p.vida = vida;
        p.vidaMaxima = vida;
        p.tamano = tamano;
        p.giro = AzarTronco(0.0f, 6.28f);
        p.color = color;
        return;
    }
}


// Aserrin que escupe el corte por los dos costados del tronco.
static void EmitirAserrinTronco(MinijuegoTronco& minijuego, int equipo, int cantidad)
{
    float cx = CENTRO_EQUIPO_TRONCO[equipo];
    float y = AlturaDientesTronco(minijuego.equipos[equipo].progresoCorte);
    if (y < ALTURA_EJE_TRONCO) y = ALTURA_EJE_TRONCO;

    int n = CantidadParticulasTronco(cantidad);
    for (int k = 0; k < n; k++)
    {
        float lado = k % 2 == 0 ? -1.0f : 1.0f;
        EmitirParticulaTronco(
            minijuego,
            PARTICULA_TRONCO_ASERRIN,
            { cx + lado * AzarTronco(0.25f, 0.50f), y + AzarTronco(0.0f, 0.10f), AzarTronco(-0.08f, 0.08f) },
            { lado * AzarTronco(0.5f, 1.7f), AzarTronco(0.3f, 1.5f), AzarTronco(-0.5f, 0.6f) },
            AzarTronco(0.9f, 1.5f),
            AzarTronco(0.035f, 0.07f),
            TinteTronco(COLOR_ASERRIN_TRONCO, AzarTronco(0.85f, 1.08f))
        );
    }
}


// Chispas de la hoja al trabarse la sierra.
static void EmitirChispasTronco(MinijuegoTronco& minijuego, int equipo, int cantidad)
{
    const EstadoEquipoTronco& estado = minijuego.equipos[equipo];
    float cx = CENTRO_EQUIPO_TRONCO[equipo] + estado.posicionSierra * AMPLITUD_SIERRA_TRONCO;
    float y = AlturaDientesTronco(estado.progresoCorte);

    int n = CantidadParticulasTronco(cantidad);
    for (int k = 0; k < n; k++)
    {
        float lado = k % 2 == 0 ? -1.0f : 1.0f;
        EmitirParticulaTronco(
            minijuego,
            PARTICULA_TRONCO_CHISPA,
            { cx + lado * AzarTronco(0.45f, 0.75f), y + 0.05f, AzarTronco(-0.05f, 0.12f) },
            { lado * AzarTronco(1.0f, 3.0f), AzarTronco(1.2f, 3.4f), AzarTronco(-0.8f, 1.2f) },
            AzarTronco(0.25f, 0.5f),
            AzarTronco(0.035f, 0.06f),
            MezclaTronco(Color{ 255, 236, 140, 255 }, Color{ 255, 120, 40, 255 }, AzarTronco(0.0f, 1.0f))
        );
    }
}


// Astillas y corteza que saltan al partirse el tronco.
static void EmitirAstillasTronco(MinijuegoTronco& minijuego, int equipo, int cantidad)
{
    float cx = CENTRO_EQUIPO_TRONCO[equipo];

    int n = CantidadParticulasTronco(cantidad);
    for (int k = 0; k < n; k++)
    {
        float a = AzarTronco(0.0f, 6.28f);
        EmitirParticulaTronco(
            minijuego,
            PARTICULA_TRONCO_ASTILLA,
            { cx + std::cos(a) * 0.3f, ALTURA_EJE_TRONCO + AzarTronco(-0.2f, 0.4f), AzarTronco(-0.1f, 0.1f) },
            { std::cos(a) * AzarTronco(1.0f, 2.6f), AzarTronco(2.0f, 4.0f), AzarTronco(-1.2f, 1.6f) },
            AzarTronco(1.0f, 1.6f),
            AzarTronco(0.06f, 0.12f),
            k % 3 == 0 ? Color{ 104, 70, 44, 255 } : TinteTronco(Color{ 222, 180, 120, 255 }, AzarTronco(0.85f, 1.05f))
        );
    }
}


static void EmitirPolvoTronco(MinijuegoTronco& minijuego, float x, float z, int cantidad)
{
    int n = CantidadParticulasTronco(cantidad);
    for (int k = 0; k < n; k++)
    {
        float a = AzarTronco(0.0f, 6.28f);
        EmitirParticulaTronco(
            minijuego,
            PARTICULA_TRONCO_POLVO,
            { x + std::cos(a) * 0.5f, 0.15f, z + std::sin(a) * 0.4f },
            { std::cos(a) * AzarTronco(0.4f, 1.1f), AzarTronco(0.2f, 0.6f), std::sin(a) * AzarTronco(0.3f, 0.9f) },
            AzarTronco(0.5f, 0.8f),
            AzarTronco(0.14f, 0.24f),
            TinteTronco(COLOR_ASERRIN_TRONCO, AzarTronco(0.80f, 0.95f))
        );
    }
}


static void EmitirConfetiTronco(MinijuegoTronco& minijuego, int equipo, int cantidad)
{
    float cx = CENTRO_EQUIPO_TRONCO[equipo];
    const Color colores[4] =
    {
        COLOR_EQUIPO_TRONCO[equipo],
        TinteTronco(COLOR_EQUIPO_TRONCO[equipo], 1.3f),
        { 255, 214, 90, 255 },
        { 250, 240, 220, 255 }
    };

    int n = CantidadParticulasTronco(cantidad);
    for (int k = 0; k < n; k++)
    {
        EmitirParticulaTronco(
            minijuego,
            PARTICULA_TRONCO_CONFETI,
            { cx + AzarTronco(-2.6f, 2.6f), AzarTronco(3.4f, 4.6f), AzarTronco(-1.2f, 1.6f) },
            { AzarTronco(-0.6f, 0.6f), AzarTronco(-0.2f, 1.2f), AzarTronco(-0.4f, 0.4f) },
            AzarTronco(2.6f, 3.6f),
            AzarTronco(0.10f, 0.15f),
            colores[k % 4]
        );
    }
}


static void ActualizarParticulasTronco(MinijuegoTronco& minijuego, float deltaTime)
{
    float t = minijuego.tiempoAnimacion;

    for (int i = 0; i < MAX_PARTICULAS_TRONCO; i++)
    {
        ParticulaTronco& p = minijuego.particulas[i];
        if (!p.activa) continue;

        p.vida -= deltaTime;
        if (p.vida <= 0.0f)
        {
            p.activa = false;
            continue;
        }

        float suelo = ALTURA_TARIMA_TRONCO;

        switch (p.tipo)
        {
            case PARTICULA_TRONCO_CHISPA:
                p.velocidad.y -= 9.0f * deltaTime;
                break;
            case PARTICULA_TRONCO_CONFETI:
                p.velocidad.y -= 3.0f * deltaTime;
                if (p.velocidad.y < -1.1f) p.velocidad.y = -1.1f;
                p.posicion.x += std::sin(t * 4.0f + p.giro) * 0.6f * deltaTime;
                p.giro += deltaTime * 6.0f;
                break;
            case PARTICULA_TRONCO_HOJA:
                p.posicion.x += std::sin(t * 1.7f + p.giro) * 0.5f * deltaTime;
                p.giro += deltaTime * 2.2f;
                suelo = 0.02f;
                break;
            case PARTICULA_TRONCO_MOTA:
                p.posicion.x += std::sin(t * 1.3f + p.giro) * 0.12f * deltaTime;
                break;
            case PARTICULA_TRONCO_POLVO:
                p.velocidad.x *= std::exp(-3.0f * deltaTime);
                p.velocidad.z *= std::exp(-3.0f * deltaTime);
                break;
            default:
                p.velocidad.y -= (p.tipo == PARTICULA_TRONCO_ASTILLA ? 9.8f : 5.5f) * deltaTime;
                p.velocidad.x *= std::exp(-1.2f * deltaTime);
                p.velocidad.z *= std::exp(-1.2f * deltaTime);
                break;
        }

        p.posicion.x += p.velocidad.x * deltaTime;
        p.posicion.y += p.velocidad.y * deltaTime;
        p.posicion.z += p.velocidad.z * deltaTime;

        if (p.posicion.y < suelo && p.tipo != PARTICULA_TRONCO_MOTA && p.tipo != PARTICULA_TRONCO_POLVO)
        {
            p.posicion.y = suelo;

            if (p.tipo == PARTICULA_TRONCO_ASTILLA && p.velocidad.y < -1.0f)
            {
                p.velocidad.y = -p.velocidad.y * 0.3f;
                p.velocidad.x *= 0.5f;
                p.velocidad.z *= 0.5f;
            }
            else
            {
                p.velocidad = { 0.0f, 0.0f, 0.0f };
            }
        }
    }
}


// Ambiente: hojas que caen del bosque y motas de aserrin al sol.
static void ActualizarAmbienteTronco(MinijuegoTronco& minijuego, float deltaTime)
{
    float factor = FactorCalidadGrafica(CalidadParticulas());
    if (factor < 0.5f) return;

    minijuego.acumuladorAmbiente += deltaTime * 4.0f * factor;

    while (minijuego.acumuladorAmbiente >= 1.0f)
    {
        minijuego.acumuladorAmbiente -= 1.0f;

        if (GetRandomValue(0, 2) == 0)
        {
            float cx = CENTRO_EQUIPO_TRONCO[GetRandomValue(0, 1)];
            EmitirParticulaTronco(
                minijuego,
                PARTICULA_TRONCO_MOTA,
                { cx + AzarTronco(-2.4f, 2.4f), AzarTronco(0.4f, 2.0f), AzarTronco(-1.0f, 1.8f) },
                { AzarTronco(-0.05f, 0.05f), AzarTronco(0.05f, 0.16f), 0.0f },
                AzarTronco(3.0f, 5.0f),
                AzarTronco(0.02f, 0.035f),
                Color{ 255, 228, 168, 220 }
            );
        }
        else
        {
            float tono = AzarTronco(0.0f, 1.0f);
            Color color = tono < 0.45f ? Color{ 214, 120, 48, 255 } : (tono < 0.8f ? Color{ 222, 176, 70, 255 } : Color{ 128, 132, 62, 255 });
            EmitirParticulaTronco(
                minijuego,
                PARTICULA_TRONCO_HOJA,
                { AzarTronco(-10.0f, 10.0f), AzarTronco(4.6f, 5.6f), AzarTronco(-4.0f, 2.6f) },
                { AzarTronco(0.10f, 0.35f), -AzarTronco(0.38f, 0.55f), 0.0f },
                12.0f,
                AzarTronco(0.10f, 0.15f),
                color
            );
        }
    }
}


// Temporizadores visuales, caida del tronco partido, particulas y confeti.
static void ActualizarVisualesTronco(MinijuegoTronco& minijuego, float deltaTime)
{
    minijuego.tiempoAnimacion += deltaTime;

    for (int equipo = 0; equipo < 2; equipo++)
    {
        EstadoEquipoTronco& estado = minijuego.equipos[equipo];

        estado.sacudida = estado.sacudida > deltaTime ? estado.sacudida - deltaTime : 0.0f;
        estado.destello = estado.destello > deltaTime ? estado.destello - deltaTime : 0.0f;
        estado.avisoError = estado.avisoError > deltaTime ? estado.avisoError - deltaTime : 0.0f;
        estado.avisoHito = estado.avisoHito > deltaTime ? estado.avisoHito - deltaTime : 0.0f;

        if (estado.caidaTronco >= 0.0f)
        {
            float antes = estado.caidaTronco;
            estado.caidaTronco += deltaTime;

            if (antes < DURACION_CAIDA_TRONCO && estado.caidaTronco >= DURACION_CAIDA_TRONCO)
            {
                ReproducirSonidoMinijuego(minijuego.audio, SONIDO_CAIDA);
                EmitirPolvoTronco(minijuego, CENTRO_EQUIPO_TRONCO[equipo], 1.0f, 8);
                EmitirPolvoTronco(minijuego, CENTRO_EQUIPO_TRONCO[equipo], -1.0f, 8);
                ActivarTemblorCamaraGeneral(0.10f, 0.22f);
            }
        }
    }

    ActualizarParticulasTronco(minijuego, deltaTime);
    ActualizarAmbienteTronco(minijuego, deltaTime);

    if (minijuego.estado == TRONCO_FINALIZADO)
    {
        float antes = minijuego.tiempoFinalizado;
        minijuego.tiempoFinalizado += deltaTime;

        if (
            !minijuego.empate &&
            minijuego.equipoGanador >= 0 &&
            antes < 0.7f &&
            minijuego.tiempoFinalizado >= 0.7f
        )
        {
            EmitirConfetiTronco(minijuego, minijuego.equipoGanador, 50);
        }
    }
}


static int LimitarCantidadTronco(int cantidadMaxima)
{
    return cantidadMaxima < MAX_JUGADORES_PRUEBA
        ? cantidadMaxima
        : MAX_JUGADORES_PRUEBA;
}


static int ContarParticipantesActivosTronco(
    const Participante participantes[],
    int cantidadMaxima
)
{
    int cantidad = 0;
    int limite = LimitarCantidadTronco(cantidadMaxima);

    for (int i = 0; i < limite; i++)
    {
        if (participantes[i].activo)
        {
            cantidad++;
        }
    }

    return cantidad;
}


static int ContarJugadoresConectadosTronco(
    const Participante participantes[],
    int cantidadMaxima
)
{
    int cantidad = 0;
    int limite = LimitarCantidadTronco(cantidadMaxima);

    for (int i = 0; i < limite; i++)
    {
        if (
            participantes[i].activo &&
            participantes[i].conectado
        )
        {
            cantidad++;
        }
    }

    return cantidad;
}


static AccionDireccionalControl ObtenerAccionControlTronco(
    AccionTronco accion
)
{
    return accion == ACCION_TRONCO_TIRAR
        ? CONTROL_DIRECCION_IZQUIERDA
        : CONTROL_DIRECCION_DERECHA;
}


static bool AccionJugadorPresionadaTronco(
    const Participante& participante,
    AccionTronco accion
)
{
    return AccionDireccionalControlPresionada(
        participante,
        ObtenerAccionControlTronco(accion)
    );
}


static bool OtraAccionJugadorPresionadaTronco(
    const Participante& participante,
    AccionTronco accionEsperada
)
{
    AccionTronco otraAccion =
        accionEsperada == ACCION_TRONCO_TIRAR
        ? ACCION_TRONCO_EMPUJAR
        : ACCION_TRONCO_TIRAR;

    return AccionJugadorPresionadaTronco(
        participante,
        otraAccion
    );
}


static const char* TextoAccionTronco(
    const Participante& participante,
    AccionTronco accion
)
{
    return ObtenerTextoAccionDireccionalControl(
        participante,
        ObtenerAccionControlTronco(accion)
    );
}


static AccionTronco ObtenerAccionEsperadaTronco(
    const MinijuegoTronco& minijuego,
    int indiceJugador
)
{
    int equipo = minijuego.equipoPorJugador[indiceJugador];
    int orden = minijuego.ordenEnEquipoPorJugador[indiceJugador];

    if (equipo < 0 || equipo >= 2 || orden < 0)
    {
        return ACCION_TRONCO_TIRAR;
    }

    int direccion = minijuego.equipos[equipo].direccionSierra;

    if (minijuego.cantidadJugadoresEquipo[equipo] <= 1)
    {
        return direccion > 0
            ? ACCION_TRONCO_EMPUJAR
            : ACCION_TRONCO_TIRAR;
    }

    if (direccion > 0)
    {
        return orden == 0
            ? ACCION_TRONCO_EMPUJAR
            : ACCION_TRONCO_TIRAR;
    }

    return orden == 0
        ? ACCION_TRONCO_TIRAR
        : ACCION_TRONCO_EMPUJAR;
}


static void LimpiarIntentoEquipoTronco(
    MinijuegoTronco& minijuego,
    int equipo
)
{
    for (int i = 0; i < MAX_JUGADORES_PRUEBA; i++)
    {
        if (minijuego.equipoPorJugador[i] != equipo)
        {
            continue;
        }

        minijuego.estadosJugadores[i].respondio = false;
        minijuego.estadosJugadores[i].acerto = false;
        minijuego.estadosJugadores[i].tiempoReaccionBot = -1.0f;
    }

    minijuego.equipos[equipo].golpeEnCurso = false;
    minijuego.equipos[equipo].tiempoCoordinacion = 0.0f;
}


static void FallarGolpeEquipoTronco(
    MinijuegoTronco& minijuego,
    int equipo
)
{
    EstadoEquipoTronco& estadoEquipo = minijuego.equipos[equipo];

    estadoEquipo.ultimoGolpeCorrecto = false;
    estadoEquipo.tiempoFeedback = 0.38f;
    estadoEquipo.tiempoBloqueo = BLOQUEO_FALLO_TRONCO;

    // Feedback: la sierra se traba, salta chispa y la estacion se sacude.
    estadoEquipo.sacudida = 0.38f;
    estadoEquipo.avisoError = 0.85f;
    estadoEquipo.avisoHito = 0.0f;
    EmitirChispasTronco(minijuego, equipo, 14);
    ReproducirSonidoMinijuego(minijuego.audio, SONIDO_ERROR);

    LimpiarIntentoEquipoTronco(minijuego, equipo);
}


static void CompletarGolpeEquipoTronco(
    MinijuegoTronco& minijuego,
    int equipo
)
{
    EstadoEquipoTronco& estadoEquipo = minijuego.equipos[equipo];

    int direccionAnterior = estadoEquipo.direccionSierra;
    float progresoAnterior = estadoEquipo.progresoCorte;

    estadoEquipo.progresoCorte += AVANCE_CORTE_TRONCO;

    if (estadoEquipo.progresoCorte > 1.0f)
    {
        estadoEquipo.progresoCorte = 1.0f;
    }

    // Feedback de la pasada: aserrin, destello y un raspido; cada cuarto
    // del corte, un aviso. Al terminar, el tronco cruje y se parte.
    estadoEquipo.destello = 0.18f;
    EmitirAserrinTronco(minijuego, equipo, 12);

    if (estadoEquipo.progresoCorte >= 1.0f)
    {
        estadoEquipo.caidaTronco = 0.0f;
        estadoEquipo.avisoHito = 0.0f;
        EmitirAstillasTronco(minijuego, equipo, 22);
        ReproducirSonidoMinijuego(minijuego.audio, SONIDO_IMPACTO);
        ActivarTemblorCamaraGeneral(0.14f, 0.28f);
    }
    else if ((int)(estadoEquipo.progresoCorte * 4.0f) != (int)(progresoAnterior * 4.0f))
    {
        estadoEquipo.hito = (int)(estadoEquipo.progresoCorte * 4.0f);
        estadoEquipo.avisoHito = 0.9f;
        ReproducirSonidoMinijuego(minijuego.audio, SONIDO_ACIERTO);
    }
    else
    {
        ReproducirSonidoMinijuego(minijuego.audio, SONIDO_GOLPE);
    }

    estadoEquipo.objetivoSierra =
        direccionAnterior > 0
        ? 0.72f
        : -0.72f;

    estadoEquipo.direccionSierra = -direccionAnterior;
    estadoEquipo.ultimoGolpeCorrecto = true;
    estadoEquipo.tiempoFeedback = 0.28f;
    estadoEquipo.tiempoBloqueo = 0.06f;

    LimpiarIntentoEquipoTronco(minijuego, equipo);
}


// Los bots no leen dispositivos: responden despues de un tiempo de
// reaccion. Si el equipo tiene un humano, el bot espera a que el humano
// inicie el golpe y lo acompana dentro de la ventana de coordinacion.
static void DecidirAccionBotTronco(
    MinijuegoTronco& minijuego,
    const Participante participantes[],
    int cantidadMaxima,
    int indiceJugador,
    float deltaTime,
    bool& accionCorrecta,
    bool& accionIncorrecta
)
{
    accionCorrecta = false;
    accionIncorrecta = false;

    int equipo = minijuego.equipoPorJugador[indiceJugador];
    int limite = LimitarCantidadTronco(cantidadMaxima);
    bool companeroInicio = false;
    bool equipoConHumano = false;

    for (int i = 0; i < limite; i++)
    {
        if (
            i == indiceJugador ||
            minijuego.equipoPorJugador[i] != equipo
        )
        {
            continue;
        }

        if (!participantes[i].esBot)
        {
            equipoConHumano = true;
        }

        if (
            minijuego.estadosJugadores[i].respondio &&
            minijuego.estadosJugadores[i].acerto
        )
        {
            companeroInicio = true;
        }
    }

    EstadoJugadorTronco& estado =
        minijuego.estadosJugadores[indiceJugador];

    if (estado.tiempoReaccionBot < 0.0f)
    {
        if (companeroInicio)
        {
            estado.tiempoReaccionBot =
                GetRandomValue(8, 22) / 100.0f;
        }
        else if (!equipoConHumano)
        {
            estado.tiempoReaccionBot =
                GetRandomValue(30, 58) / 100.0f;
        }
        else
        {
            // El humano marca el ritmo del equipo.
            return;
        }
    }
    else if (
        companeroInicio &&
        estado.tiempoReaccionBot > REACCION_MAXIMA_BOT_TRONCO
    )
    {
        // Ya empezo el golpe: debe responder antes de que se
        // cierre la ventana de coordinacion.
        estado.tiempoReaccionBot = REACCION_MAXIMA_BOT_TRONCO;
    }

    estado.tiempoReaccionBot -= deltaTime;

    if (estado.tiempoReaccionBot > 0.0f)
    {
        return;
    }

    if (GetRandomValue(1, 100) <= PROBABILIDAD_ERROR_BOT_TRONCO)
    {
        accionIncorrecta = true;
    }
    else
    {
        accionCorrecta = true;
    }
}


static void FinalizarResultadoTronco(
    MinijuegoTronco& minijuego,
    int equipoGanador,
    bool empate
)
{
    if (
        minijuego.resultado.estado !=
        RESULTADO_MINIJUEGO_EN_CURSO
    )
    {
        return;
    }

    minijuego.estado = TRONCO_FINALIZADO;
    minijuego.equipoGanador = equipoGanador;
    minijuego.empate = empate;
    minijuego.tiempoFinalizado = 0.0f;

    ReproducirSonidoMinijuego(minijuego.audio, SONIDO_RESULTADO);

    if (!empate && equipoGanador >= 0 && equipoGanador < 2)
    {
        EmitirConfetiTronco(minijuego, equipoGanador, 40);
    }

    minijuego.resultado.estado = RESULTADO_MINIJUEGO_FINALIZADO;
    minijuego.resultado.desenlace =
        empate
        ? DESENLACE_EMPATE
        : DESENLACE_CON_GANADOR;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        ResultadoParticipante& participante =
            minijuego.resultado.participantes[i];

        if (!participante.participo)
        {
            continue;
        }

        int equipo =
            i < MAX_JUGADORES_PRUEBA
            ? minijuego.equipoPorJugador[i]
            : -1;

        participante.numeroEquipo = equipo;

        if (equipo >= 0 && equipo < 2)
        {
            participante.puntuacionMinijuego =
                (int)(
                    minijuego.equipos[equipo].progresoCorte *
                    1000.0f
                );

            participante.posicionFinal =
                empate || equipo == equipoGanador
                ? 1
                : 2;
        }
        else
        {
            participante.puntuacionMinijuego = 0;
            participante.posicionFinal = 0;
        }

        participante.puntosObtenidos = 0;
    }
}


void MinijuegoTronco::Inicializar()
{
    // Mas cerca y mas baja que antes: las dos estaciones ocupan casi todo
    // el ancho y al fondo se ve el galpon y el bosque.
    camara.position = { 0.0f, 5.0f, 10.0f };
    camara.target = { 0.0f, 1.1f, -0.5f };
    camara.up = { 0.0f, 1.0f, 0.0f };
    camara.fovy = 50.0f;
    camara.projection = CAMERA_PERSPECTIVE;
}


void MinijuegoTronco::ConfigurarJugadores(
    JugadorPrueba jugadores[],
    int cantidadMaxima
) const
{
    Vector3 spawns[MAX_JUGADORES_PRUEBA] =
    {
        { -5.65f, 0.75f, 0.15f },
        { -2.15f, 0.75f, 0.15f },
        { 2.15f, 0.75f, 0.15f },
        { 5.65f, 0.75f, 0.15f }
    };

    int limite = LimitarCantidadTronco(cantidadMaxima);

    for (int i = 0; i < limite; i++)
    {
        jugadores[i].posicionSpawn = spawns[i];
        jugadores[i].tamano = { 0.78f, 1.45f, 0.78f };
        ReiniciarJugadorPrueba(jugadores[i]);
    }
}


void MinijuegoTronco::PrepararEquipos(
    const Participante participantes[],
    int cantidadMaxima
)
{
    for (int i = 0; i < MAX_JUGADORES_PRUEBA; i++)
    {
        equipoPorJugador[i] = -1;
        ordenEnEquipoPorJugador[i] = -1;
    }

    cantidadJugadoresEquipo[0] = 0;
    cantidadJugadoresEquipo[1] = 0;

    int cantidadActivos =
        ContarParticipantesActivosTronco(
            participantes,
            cantidadMaxima
        );

    int ordenActivo = 0;
    int limite = LimitarCantidadTronco(cantidadMaxima);

    for (int i = 0; i < limite; i++)
    {
        if (!participantes[i].activo)
        {
            continue;
        }

        int equipo = 0;

        if (cantidadActivos == 2)
        {
            equipo = ordenActivo;
        }
        else
        {
            equipo = ordenActivo < 2 ? 0 : 1;
        }

        int ordenEquipo = cantidadJugadoresEquipo[equipo];

        equipoPorJugador[i] = equipo;
        ordenEnEquipoPorJugador[i] = ordenEquipo;
        cantidadJugadoresEquipo[equipo]++;
        resultado.participantes[i].numeroEquipo = equipo;

        ordenActivo++;
    }
}


void MinijuegoTronco::Reiniciar(
    const Participante participantes[],
    int cantidadMaxima
)
{
    InicializarResultadoMinijuego(
        resultado,
        participantes,
        FORMATO_MINIJUEGO_EQUIPOS
    );

    for (int i = 0; i < MAX_JUGADORES_PRUEBA; i++)
    {
        estadosJugadores[i] = EstadoJugadorTronco{};
        equipoPorJugador[i] = -1;
        ordenEnEquipoPorJugador[i] = -1;
    }

    for (int equipo = 0; equipo < 2; equipo++)
    {
        equipos[equipo] = EstadoEquipoTronco{};
        cantidadJugadoresEquipo[equipo] = 0;
    }

    int cantidadActivos =
        ContarParticipantesActivosTronco(
            participantes,
            cantidadMaxima
        );

    int cantidadConectados =
        ContarJugadoresConectadosTronco(
            participantes,
            cantidadMaxima
        );

    jugadoresEnPartida = cantidadConectados;

    partidaValida =
        (
            cantidadActivos == 2 ||
            cantidadActivos == 4
        ) &&
        cantidadConectados == cantidadActivos;

    for (int i = 0; i < MAX_PARTICULAS_TRONCO; i++)
    {
        particulas[i].activa = false;
    }

    tiempoFinalizado = 0.0f;
    acumuladorAmbiente = 0.0f;

    equipoGanador = -1;
    empate = false;
    tiempoPreparacion = DURACION_PREPARACION_TRONCO;

    // Ahora representa tiempo transcurrido. Ya no existe una cuenta atras
    // que pueda terminar el minijuego en empate.
    tiempoPartida = 0.0f;

    if (!partidaValida)
    {
        estado = TRONCO_ESPERANDO_JUGADORES;
        resultado.cantidadEquipos = 0;
        return;
    }

    resultado.cantidadEquipos = 2;
    PrepararEquipos(participantes, cantidadMaxima);
    estado = TRONCO_PREPARANDO;
}


void MinijuegoTronco::Actualizar(
    float deltaTime,
    int cantidadMaxima,
    const Participante participantes[]
)
{
    int cantidadConectados =
        ContarJugadoresConectadosTronco(
            participantes,
            cantidadMaxima
        );

    if (avisoReconfiguracion > 0.0f)
    {
        avisoReconfiguracion -= deltaTime;
        if (avisoReconfiguracion < 0.0f) avisoReconfiguracion = 0.0f;
    }

    if (cantidadConectados != jugadoresEnPartida)
    {
        Reiniciar(participantes, cantidadMaxima);
        avisoReconfiguracion = 4.0f;
        return;
    }

    ActualizarVisualesTronco(*this, deltaTime);

    for (int i = 0; i < MAX_JUGADORES_PRUEBA; i++)
    {
        if (estadosJugadores[i].animacionGolpe > 0.0f)
        {
            estadosJugadores[i].animacionGolpe -= deltaTime;

            if (estadosJugadores[i].animacionGolpe < 0.0f)
            {
                estadosJugadores[i].animacionGolpe = 0.0f;
            }
        }
    }

    for (int equipo = 0; equipo < 2; equipo++)
    {
        EstadoEquipoTronco& estadoEquipo = equipos[equipo];

        float diferencia =
            estadoEquipo.objetivoSierra -
            estadoEquipo.posicionSierra;

        float paso = VELOCIDAD_SIERRA_VISUAL_TRONCO * deltaTime;
        float distancia = diferencia < 0.0f ? -diferencia : diferencia;

        if (distancia <= paso)
        {
            estadoEquipo.posicionSierra = estadoEquipo.objetivoSierra;
        }
        else
        {
            estadoEquipo.posicionSierra +=
                diferencia < 0.0f ? -paso : paso;
        }

        if (estadoEquipo.tiempoFeedback > 0.0f)
        {
            estadoEquipo.tiempoFeedback -= deltaTime;
            if (estadoEquipo.tiempoFeedback < 0.0f)
            {
                estadoEquipo.tiempoFeedback = 0.0f;
            }
        }

        if (estadoEquipo.tiempoBloqueo > 0.0f)
        {
            estadoEquipo.tiempoBloqueo -= deltaTime;
            if (estadoEquipo.tiempoBloqueo < 0.0f)
            {
                estadoEquipo.tiempoBloqueo = 0.0f;
            }
        }
    }

    if (!partidaValida || estado == TRONCO_FINALIZADO)
    {
        return;
    }

    if (estado == TRONCO_PREPARANDO)
    {
        float preparacionAntes = tiempoPreparacion;
        tiempoPreparacion -= deltaTime;
        ActualizarAudioCuentaRegresiva(audio, preparacionAntes, tiempoPreparacion);

        if (tiempoPreparacion <= 0.0f)
        {
            tiempoPreparacion = 0.0f;
            estado = TRONCO_JUGANDO;
        }

        return;
    }

    if (estado != TRONCO_JUGANDO)
    {
        return;
    }

    float restanteAntes = DURACION_MAXIMA_TRONCO - tiempoPartida;
    tiempoPartida += deltaTime;
    ActualizarAudioAlertaTiempo(audio, restanteAntes, DURACION_MAXIMA_TRONCO - tiempoPartida, 5.0f);

    if (tiempoPartida >= DURACION_MAXIMA_TRONCO)
    {
        float progresoA = equipos[0].progresoCorte;
        float progresoB = equipos[1].progresoCorte;
        bool empateTiempo = std::fabs(progresoA - progresoB) < 0.001f;
        FinalizarResultadoTronco(*this, progresoA > progresoB ? 0 : 1, empateTiempo);
        return;
    }

    for (int equipo = 0; equipo < 2; equipo++)
    {
        EstadoEquipoTronco& estadoEquipo = equipos[equipo];

        if (estadoEquipo.tiempoBloqueo > 0.0f)
        {
            continue;
        }

        bool fallo = false;
        int respuestasCorrectas = 0;
        int limite = LimitarCantidadTronco(cantidadMaxima);

        for (int i = 0; i < limite; i++)
        {
            if (equipoPorJugador[i] != equipo)
            {
                continue;
            }

            EstadoJugadorTronco& estadoJugador = estadosJugadores[i];

            if (!estadoJugador.respondio)
            {
                bool accionCorrecta = false;
                bool accionIncorrecta = false;

                if (participantes[i].esBot)
                {
                    DecidirAccionBotTronco(
                        *this,
                        participantes,
                        cantidadMaxima,
                        i,
                        deltaTime,
                        accionCorrecta,
                        accionIncorrecta
                    );
                }
                else
                {
                    AccionTronco accionEsperada =
                        ObtenerAccionEsperadaTronco(*this, i);

                    accionCorrecta =
                        AccionJugadorPresionadaTronco(
                            participantes[i],
                            accionEsperada
                        );

                    accionIncorrecta =
                        OtraAccionJugadorPresionadaTronco(
                            participantes[i],
                            accionEsperada
                        );
                }

                if (accionCorrecta && !accionIncorrecta)
                {
                    estadoJugador.respondio = true;
                    estadoJugador.acerto = true;
                    estadoJugador.animacionGolpe = 0.22f;
                }
                else if (accionIncorrecta)
                {
                    estadoJugador.respondio = true;
                    estadoJugador.acerto = false;
                    fallo = true;
                }
            }

            if (estadoJugador.respondio && estadoJugador.acerto)
            {
                respuestasCorrectas++;
            }

            if (estadoJugador.respondio && !estadoJugador.acerto)
            {
                fallo = true;
            }
        }

        if (fallo)
        {
            FallarGolpeEquipoTronco(*this, equipo);
            continue;
        }

        int requeridas = cantidadJugadoresEquipo[equipo];

        if (requeridas > 0 && respuestasCorrectas >= requeridas)
        {
            CompletarGolpeEquipoTronco(*this, equipo);

            if (equipos[equipo].progresoCorte >= 1.0f)
            {
                FinalizarResultadoTronco(*this, equipo, false);
                return;
            }

            continue;
        }

        if (respuestasCorrectas > 0)
        {
            if (!estadoEquipo.golpeEnCurso)
            {
                estadoEquipo.golpeEnCurso = true;
                estadoEquipo.tiempoCoordinacion =
                    VENTANA_COORDINACION_TRONCO;
            }

            estadoEquipo.tiempoCoordinacion -= deltaTime;

            if (estadoEquipo.tiempoCoordinacion <= 0.0f)
            {
                FallarGolpeEquipoTronco(*this, equipo);
            }
        }
    }
}


//==================================================
// VISUAL: AYUDAS GEOMETRICAS
//==================================================

static void CulloTronco(bool activo)
{
    rlDrawRenderBatchActive();
    if (activo) rlEnableBackfaceCulling();
    else rlDisableBackfaceCulling();
}


// Caja con cada cara a un brillo distinto (luz de atardecer desde arriba
// y de frente): da volumen sin iluminacion real.
static void CajaFacetadaTronco(Vector3 centro, Vector3 tamano, Color color, float brilloTapa)
{
    float x0 = centro.x - tamano.x * 0.5f;
    float x1 = centro.x + tamano.x * 0.5f;
    float y0 = centro.y - tamano.y * 0.5f;
    float y1 = centro.y + tamano.y * 0.5f;
    float z0 = centro.z - tamano.z * 0.5f;
    float z1 = centro.z + tamano.z * 0.5f;

    Color tapa = TinteTronco(color, brilloTapa);
    Color frente = TinteTronco(color, 0.86f);
    Color atras = TinteTronco(color, 0.58f);
    Color derecha = TinteTronco(color, 0.66f);
    Color izquierda = TinteTronco(color, 0.76f);

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


// Caja facetada con borde oscuro (tablas, cajones, vigas).
static void CajaTronco(Vector3 centro, Vector3 tamano, Color color)
{
    CajaFacetadaTronco(centro, tamano, color, 1.18f);
    DrawCubeWiresV(centro, tamano, Fade(BLACK, 0.22f));
}


static void QuadTronco(Vector3 a, Vector3 b, Vector3 c, Vector3 d, Color color)
{
    DrawTriangle3D(a, b, c, color);
    DrawTriangle3D(a, c, d, color);
}


// Disco de luz suave (centro opaco, borde transparente). Solo con mezcla
// aditiva y sin escribir profundidad.
static void ResplandorTronco(Vector3 centro, Vector3 u, Vector3 v, float radio, Color color)
{
    const int LADOS = 16;
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


static void ResplandorSueloTronco(float x, float z, float radio, Color color)
{
    ResplandorTronco({ x, 0.09f, z }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, radio, color);
}


static void HaloTronco(Vector3 centro, float radio, Color color)
{
    ResplandorTronco(centro, { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, radio, color);
}


// Anillo plano sobre el suelo (pertenencia del jugador).
static void AnilloSueloTronco(float x, float z, float y, float radioInterior, float radioExterior, Color color)
{
    const int LADOS = 26;

    for (int k = 0; k < LADOS; k++)
    {
        float a0 = 6.2832f * (float)k / (float)LADOS;
        float a1 = 6.2832f * (float)(k + 1) / (float)LADOS;
        Vector3 i0 = { x + std::cos(a0) * radioInterior, y, z + std::sin(a0) * radioInterior };
        Vector3 i1 = { x + std::cos(a1) * radioInterior, y, z + std::sin(a1) * radioInterior };
        Vector3 e0 = { x + std::cos(a0) * radioExterior, y, z + std::sin(a0) * radioExterior };
        Vector3 e1 = { x + std::cos(a1) * radioExterior, y, z + std::sin(a1) * radioExterior };
        DrawTriangle3D(i0, e1, e0, color);
        DrawTriangle3D(i0, i1, e1, color);
    }
}


// Plano vertical con degrade de transparencia: bruma entre capas de bosque.
static void VeloNieblaTronco(float z, float medioAncho, float alto, float alfaAbajo, float alfaArriba)
{
    Color c = COLOR_NIEBLA_TRONCO;
    unsigned char a0 = (unsigned char)(255.0f * alfaAbajo);
    unsigned char a1 = (unsigned char)(255.0f * alfaArriba);

    rlBegin(RL_TRIANGLES);
    rlColor4ub(c.r, c.g, c.b, a0); rlVertex3f(-medioAncho, -0.1f, z);
    rlColor4ub(c.r, c.g, c.b, a0); rlVertex3f(medioAncho, -0.1f, z);
    rlColor4ub(c.r, c.g, c.b, a1); rlVertex3f(medioAncho, alto, z);
    rlColor4ub(c.r, c.g, c.b, a0); rlVertex3f(-medioAncho, -0.1f, z);
    rlColor4ub(c.r, c.g, c.b, a1); rlVertex3f(medioAncho, alto, z);
    rlColor4ub(c.r, c.g, c.b, a1); rlVertex3f(-medioAncho, alto, z);
    rlEnd();
}


// Cara de un corte: albura clara, anillos oscuros y medula. Son discos
// apilados hacia "direccion" (+1 mira a la camara).
static void AnillosCorteTronco(Vector3 centro, float radio, float direccion, int lados)
{
    static const float RADIOS[8] = { 0.96f, 0.86f, 0.70f, 0.62f, 0.44f, 0.37f, 0.18f, 0.08f };
    static const Color COLORES[8] =
    {
        { 214, 170, 112, 255 }, { 158, 106, 62, 255 }, { 226, 186, 128, 255 }, { 164, 112, 66, 255 },
        { 222, 180, 120, 255 }, { 156, 104, 60, 255 }, { 214, 168, 108, 255 }, { 118, 76, 44, 255 }
    };
    const float paso = 0.007f;

    for (int k = 0; k < 8; k++)
    {
        float z0 = centro.z + direccion * paso * (float)k;
        float z1 = z0 + direccion * paso;
        DrawCylinderEx({ centro.x, centro.y, z0 }, { centro.x, centro.y, z1 }, radio * RADIOS[k], radio * RADIOS[k], lados, COLORES[k]);
    }

    float zTapa = centro.z + direccion * paso * 8.2f;
    DrawLine3D({ centro.x + radio * 0.10f, centro.y + radio * 0.05f, zTapa }, { centro.x + radio * 0.78f, centro.y + radio * 0.40f, zTapa }, Color{ 96, 60, 34, 255 });
}


// Leno con la cara del corte hacia la camara (pilas de lena).
static void LenoTronco(Vector3 centro, float radio, float largo, int semilla)
{
    Color corteza = TinteTronco(Color{ 98, 66, 42, 255 }, 0.80f + 0.32f * Hash01Tronco(semilla, 3));
    float zFrente = centro.z + largo * 0.5f;

    DrawCylinderEx({ centro.x, centro.y, centro.z - largo * 0.5f }, { centro.x, centro.y, zFrente }, radio, radio * 0.97f, 10, corteza);
    AnillosCorteTronco({ centro.x, centro.y, zFrente }, radio * 0.92f, 1.0f, 10);
}


static void PinoTronco(Vector3 base, float alto, Color follaje, Color corteza)
{
    DrawCylinderEx(base, { base.x, base.y + alto * 0.34f, base.z }, alto * 0.05f, alto * 0.035f, 6, corteza);

    for (int k = 0; k < 3; k++)
    {
        float y0 = base.y + alto * (0.16f + 0.25f * (float)k);
        float radio = alto * (0.29f - 0.07f * (float)k);
        float y1 = y0 + alto * (0.44f - 0.05f * (float)k);
        DrawCylinderEx({ base.x, y0, base.z }, { base.x, y1, base.z }, radio, 0.0f, 8, TinteTronco(follaje, 0.80f + 0.15f * (float)k));
    }
}


static void FilaPinosTronco(float z, float profundidad, int semilla, float altoMin, float altoMax, float separacion, float xMax)
{
    int cantidad = (int)(2.0f * xMax / separacion) + 1;

    for (int k = 0; k < cantidad; k++)
    {
        float px = -xMax + (float)k * separacion + (Hash01Tronco(k, semilla) - 0.5f) * separacion * 0.8f;
        float pz = z + (Hash01Tronco(k, semilla + 1) - 0.5f) * profundidad;
        float alto = altoMin + (altoMax - altoMin) * Hash01Tronco(k, semilla + 2);
        Color follaje = MezclaTronco(Color{ 44, 80, 54, 255 }, Color{ 74, 96, 50, 255 }, Hash01Tronco(k, semilla + 3));

        PinoTronco({ px, 0.0f, pz }, alto, NieblaTronco(follaje, pz), NieblaTronco(Color{ 72, 50, 36, 255 }, pz));
    }
}


//==================================================
// VISUAL: BOSQUE, SUELO Y GALPON
//==================================================

static void DibujarCieloTronco()
{
    int ancho = GetScreenWidth();
    int alto = GetScreenHeight();
    Color cenit = { 88, 74, 126, 255 };
    Color horizonte = { 248, 178, 116, 255 };

    ClearBackground(horizonte);
    DrawRectangleGradientV(0, 0, ancho, (int)(alto * 0.30f), cenit, horizonte);

    // Sol bajo de atardecer: asoma entre los pinos del fondo.
    float sx = ancho * 0.74f;
    float sy = alto * 0.17f;
    DrawCircleGradient({ sx, sy }, alto * 0.26f, Color{ 255, 214, 150, 150 }, Color{ 248, 178, 116, 0 });
    DrawCircle((int)sx, (int)sy, alto * 0.055f, Color{ 255, 236, 194, 255 });
}


static void DibujarBosqueFondoTronco(float factor)
{
    // Suelo del bosque (bajo las baldosas del claro).
    DrawCubeV({ 0.0f, -0.53f, -14.0f }, { 90.0f, 1.0f, 46.0f }, NieblaTronco(Color{ 76, 74, 46, 255 }, -8.0f));

    // Lomas lejanas casi fundidas con la bruma.
    for (int k = 0; k < 7; k++)
    {
        float x = -36.0f + (float)k * 12.0f + 4.0f * Hash01Tronco(k, 61);
        float alto = 6.0f + 4.0f * Hash01Tronco(k, 62);
        DrawCylinderEx({ x, -0.5f, -34.0f }, { x, alto, -34.0f }, 10.0f, 1.5f, 10, NieblaTronco(Color{ 96, 92, 84, 255 }, -40.0f));
    }

    // Capas de pinos de atras hacia adelante, con bruma entre capas.
    rlDrawRenderBatchActive();
    rlDisableDepthMask();
    VeloNieblaTronco(-27.0f, 60.0f, 14.0f, 0.55f, 0.10f);
    rlDrawRenderBatchActive();
    rlEnableDepthMask();

    if (factor >= 0.9f) FilaPinosTronco(-22.5f, 2.0f, 11, 8.5f, 11.5f, 2.3f, 30.0f);

    rlDrawRenderBatchActive();
    rlDisableDepthMask();
    VeloNieblaTronco(-19.5f, 50.0f, 13.0f, 0.42f, 0.06f);
    rlDrawRenderBatchActive();
    rlEnableDepthMask();

    FilaPinosTronco(-16.5f, 2.0f, 23, 7.5f, 10.0f, factor >= 0.5f ? 2.0f : 2.8f, 24.0f);

    rlDrawRenderBatchActive();
    rlDisableDepthMask();
    VeloNieblaTronco(-14.0f, 40.0f, 11.0f, 0.36f, 0.04f);
    rlDrawRenderBatchActive();
    rlEnableDepthMask();

    if (factor >= 0.5f) FilaPinosTronco(-12.0f, 1.6f, 37, 6.5f, 8.8f, 1.9f, 19.0f);

    rlDrawRenderBatchActive();
    rlDisableDepthMask();
    VeloNieblaTronco(-10.4f, 34.0f, 9.0f, 0.26f, 0.02f);
    rlDrawRenderBatchActive();
    rlEnableDepthMask();

    FilaPinosTronco(-9.6f, 1.0f, 49, 5.5f, 7.4f, factor >= 0.5f ? 1.8f : 2.6f, 16.0f);
}


static void DibujarArbolesLateralesTronco(float factor)
{
    int cantidad = factor >= 0.9f ? 16 : (factor >= 0.5f ? 10 : 6);

    for (int k = 0; k < cantidad; k++)
    {
        float lado = k % 2 == 0 ? -1.0f : 1.0f;
        float z = -7.0f + 8.0f * Hash01Tronco(k, 91);
        float x = lado * (8.4f + 4.0f * Hash01Tronco(k, 92) + (z > -2.0f ? (z + 2.0f) * 0.45f : 0.0f));
        float alto = 4.4f + 2.4f * Hash01Tronco(k, 93);
        Color follaje = MezclaTronco(Color{ 46, 84, 56, 255 }, Color{ 66, 96, 52, 255 }, Hash01Tronco(k, 94));
        PinoTronco({ x, 0.0f, z }, alto, follaje, Color{ 78, 54, 36, 255 });
    }

    if (factor < 0.5f) return;

    // Arboles de hoja con copa otonal: dan calidez al borde del claro.
    for (int k = 0; k < 4; k++)
    {
        float lado = k % 2 == 0 ? -1.0f : 1.0f;
        float x = lado * (7.6f + 1.4f * (float)(k / 2));
        float z = -5.6f + 1.8f * (float)(k / 2);
        Color copa = k / 2 == 0 ? Color{ 196, 116, 52, 255 } : Color{ 210, 156, 62, 255 };

        DrawCylinderEx({ x, 0.0f, z }, { x, 2.0f, z }, 0.20f, 0.14f, 7, Color{ 84, 58, 40, 255 });
        DrawSphereEx({ x, 2.6f, z }, 1.05f, 8, 8, TinteTronco(copa, 0.82f));
        DrawSphereEx({ x - 0.55f, 2.3f, z + 0.3f }, 0.72f, 7, 7, copa);
        DrawSphereEx({ x + 0.5f, 2.45f, z + 0.25f }, 0.66f, 7, 7, TinteTronco(copa, 1.10f));
    }
}


// Claro de tierra: baldosas con altura y color variados; aserrin claro
// alrededor de cada estacion y musgo hacia el borde del bosque.
static void DibujarSueloTronco(float factor)
{
    const float paso = factor >= 0.5f ? 0.80f : 1.30f;
    const float xMin = -13.6f, xMax = 13.6f;
    const float zMin = -7.2f, zMax = 5.6f;
    int nx = (int)((xMax - xMin) / paso);
    int nz = (int)((zMax - zMin) / paso);

    for (int iz = 0; iz < nz; iz++)
    {
        for (int ix = 0; ix < nx; ix++)
        {
            float x = xMin + ((float)ix + 0.5f) * paso;
            float z = zMin + ((float)iz + 0.5f) * paso;
            float h = Hash01Tronco(ix, iz);

            Color color = { 116, 86, 58, 255 };

            float musgo = LimitarTronco((std::fabs(x) - 8.0f) / 3.0f, 0.0f, 1.0f)
                + LimitarTronco((-z - 4.0f) / 2.0f, 0.0f, 1.0f) * 0.7f;
            color = MezclaTronco(color, Color{ 86, 98, 56, 255 }, LimitarTronco(musgo, 0.0f, 0.85f) * (0.7f + 0.3f * Hash01Tronco(ix + 3, iz + 17)));

            for (int e = 0; e < 2; e++)
            {
                float dx = (x - CENTRO_EQUIPO_TRONCO[e]) / 3.4f;
                float dz = z / 2.8f;
                float aserrin = LimitarTronco(1.0f - std::sqrt(dx * dx + dz * dz), 0.0f, 1.0f);
                color = MezclaTronco(color, COLOR_ASERRIN_TRONCO, aserrin * 0.55f * (0.55f + 0.45f * Hash01Tronco(ix + 5, iz + 9)));
            }

            // Sendero pisado hacia el galpon.
            if (std::fabs(x) < 1.3f && z < -1.0f)
            {
                color = MezclaTronco(color, Color{ 150, 116, 80, 255 }, 0.45f);
            }

            float variacion = 0.86f + 0.24f * Hash01Tronco(ix + 31, iz + 7);
            DrawCubeV({ x, -0.05f + h * 0.025f, z }, { paso + 0.002f, 0.10f, paso + 0.002f }, TinteTronco(color, variacion));
        }
    }
}


static void DibujarDetallesSueloTronco(float factor)
{
    if (factor < 0.5f) return;

    // Zonas ocupadas (estaciones y pila central): ahi no va decoracion.
    auto libre = [](float x, float z)
    {
        float ax = std::fabs(x);
        if (ax > 0.9f && ax < 7.1f && z > -2.8f && z < 2.7f) return false;
        if (ax < 1.3f && z > -1.6f && z < 2.4f) return false;
        if (z < -6.0f && ax < 5.2f) return false;
        return true;
    };

    // Piedras.
    for (int k = 0; k < 22; k++)
    {
        float x = -12.0f + 24.0f * Hash01Tronco(k, 201);
        float z = -6.6f + 11.4f * Hash01Tronco(k, 202);
        if (!libre(x, z)) continue;
        float s = 0.12f + 0.20f * Hash01Tronco(k, 203);
        Color piedra = TinteTronco(Color{ 128, 120, 110, 255 }, 0.78f + 0.3f * Hash01Tronco(k, 204));
        CajaFacetadaTronco({ x, s * 0.28f, z }, { s, s * 0.6f, s * 0.8f }, piedra, 1.25f);
    }

    // Hojas caidas.
    int hojas = (int)(70.0f * factor);
    for (int k = 0; k < hojas; k++)
    {
        float x = -12.0f + 24.0f * Hash01Tronco(k, 211);
        float z = -6.8f + 12.0f * Hash01Tronco(k, 212);
        if (!libre(x, z)) continue;
        float tono = Hash01Tronco(k, 213);
        Color c = tono < 0.4f ? Color{ 204, 112, 46, 255 } : (tono < 0.75f ? Color{ 214, 168, 66, 255 } : Color{ 150, 70, 40, 255 });
        DrawCubeV({ x, 0.03f, z }, { 0.11f, 0.01f, 0.07f }, c);
    }

    // Matas de pasto.
    int matas = (int)(46.0f * factor);
    for (int k = 0; k < matas; k++)
    {
        float x = -12.5f + 25.0f * Hash01Tronco(k, 221);
        float z = -6.6f + 11.8f * Hash01Tronco(k, 222);
        if (!libre(x, z)) continue;
        Color verde = MezclaTronco(Color{ 92, 128, 58, 255 }, Color{ 150, 150, 64, 255 }, Hash01Tronco(k, 223));

        for (int b = 0; b < 4; b++)
        {
            float off = ((float)b - 1.5f) * 0.05f;
            float inclinacion = ((float)b - 1.5f) * 0.07f;
            float alto = 0.20f + 0.14f * Hash01Tronco(k, 224 + b);
            DrawTriangle3D({ x + off - 0.025f, 0.0f, z }, { x + off + 0.025f, 0.0f, z }, { x + off + inclinacion, alto, z + 0.02f }, TinteTronco(verde, 0.85f + 0.08f * (float)b));
        }
    }

    // Ramas caidas.
    for (int k = 0; k < 6; k++)
    {
        float x = -11.0f + 22.0f * Hash01Tronco(k, 231);
        float z = -6.0f + 10.0f * Hash01Tronco(k, 232);
        if (!libre(x, z)) continue;
        float a = 6.2832f * Hash01Tronco(k, 233);
        DrawCylinderEx({ x, 0.04f, z }, { x + std::cos(a) * 0.7f, 0.04f, z + std::sin(a) * 0.7f }, 0.035f, 0.02f, 5, Color{ 92, 64, 42, 255 });
    }
}


static void DibujarGalponTronco(float factor)
{
    const float zFondo = -8.9f;
    const float zFrente = -6.3f;
    const float medio = 4.6f;
    const Color maderaPared = { 132, 90, 58, 255 };
    const Color maderaViga = { 92, 62, 40, 255 };

    // Piso de tablas.
    CajaFacetadaTronco({ 0.0f, 0.05f, (zFondo + zFrente) * 0.5f }, { medio * 2.0f + 0.3f, 0.10f, zFrente - zFondo + 0.3f }, Color{ 96, 70, 48, 255 }, 1.05f);

    // Pared del fondo: tablones verticales sobre un respaldo oscuro.
    DrawCubeV({ 0.0f, 2.3f, zFondo - 0.09f }, { medio * 2.0f, 4.6f, 0.05f }, Color{ 38, 26, 18, 255 });
    int tablones = (int)(medio * 2.0f / 0.42f);
    for (int i = 0; i < tablones; i++)
    {
        float x = -medio + ((float)i + 0.5f) * 0.42f;
        float alto = 4.5f + 0.12f * Hash01Tronco(i, 71);
        Color c = TinteTronco(maderaPared, 0.80f + 0.30f * Hash01Tronco(i, 77));
        DrawCubeV({ x, alto * 0.5f, zFondo }, { 0.39f, alto, 0.10f }, c);
    }
    DrawCubeV({ 0.0f, 0.42f, zFondo + 0.08f }, { medio * 2.0f, 0.14f, 0.06f }, maderaViga);
    DrawCubeV({ 0.0f, 2.95f, zFondo + 0.08f }, { medio * 2.0f, 0.14f, 0.06f }, maderaViga);

    // Paredes laterales de tablas horizontales.
    for (int lado = -1; lado <= 1; lado += 2)
    {
        for (int k = 0; k < 8; k++)
        {
            float y = 0.24f + 0.46f * (float)k;
            Color c = TinteTronco(maderaPared, 0.70f + 0.24f * Hash01Tronco(k, 80 + lado));
            CajaFacetadaTronco({ (float)lado * medio, y, (zFondo + zFrente) * 0.5f }, { 0.10f, 0.43f, zFrente - zFondo }, c, 1.1f);
        }
    }

    // Postes y viga del frente.
    const float postes[4] = { -medio, -1.55f, 1.55f, medio };
    for (int k = 0; k < 4; k++)
    {
        CajaTronco({ postes[k], 1.72f, zFrente }, { 0.28f, 3.44f, 0.28f }, maderaViga);
    }
    CajaTronco({ 0.0f, 3.46f, zFrente }, { medio * 2.0f + 0.6f, 0.26f, 0.32f }, TinteTronco(maderaViga, 0.9f));

    // Techo a dos aguas: filas de tejas con labio que da escalon.
    Vector3 aleroIzq = { -medio - 0.55f, 3.56f, zFrente + 0.75f };
    Vector3 aleroDer = { medio + 0.55f, 3.56f, zFrente + 0.75f };
    Vector3 cumbreDer = { medio + 0.55f, 4.78f, zFondo - 0.15f };
    Vector3 cumbreIzq = { -medio - 0.55f, 4.78f, zFondo - 0.15f };
    const int FILAS = 6;
    for (int f = 0; f < FILAS; f++)
    {
        float t0 = (float)f / (float)FILAS;
        float t1 = (float)(f + 1) / (float)FILAS;
        auto mezcla = [](Vector3 a, Vector3 b, float t) { return Vector3{ a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t }; };
        Vector3 a = mezcla(aleroIzq, cumbreIzq, t0);
        Vector3 b = mezcla(aleroDer, cumbreDer, t0);
        Vector3 c = mezcla(aleroDer, cumbreDer, t1);
        Vector3 d = mezcla(aleroIzq, cumbreIzq, t1);
        Color teja = f % 2 == 0 ? Color{ 152, 64, 48, 255 } : Color{ 138, 56, 44, 255 };
        QuadTronco(a, b, c, d, teja);
        DrawCubeV({ 0.0f, a.y + 0.03f, a.z }, { medio * 2.0f + 1.1f, 0.07f, 0.10f }, Color{ 104, 42, 34, 255 });
    }
    // Tapas laterales del techo (hastial).
    for (int lado = -1; lado <= 1; lado += 2)
    {
        float x = (float)lado * medio;
        DrawTriangle3D({ x, 3.30f, zFrente }, { x, 4.70f, zFondo }, { x, 3.30f, zFondo }, TinteTronco(maderaPared, 0.62f));
    }

    // Interior: banco de sierra circular.
    CajaTronco({ -2.6f, 0.62f, -7.6f }, { 1.9f, 0.12f, 0.95f }, Color{ 128, 90, 56, 255 });
    for (int k = 0; k < 4; k++)
    {
        float lx = -2.6f + (k % 2 == 0 ? -0.8f : 0.8f);
        float lz = -7.6f + (k < 2 ? -0.38f : 0.38f);
        CajaFacetadaTronco({ lx, 0.30f, lz }, { 0.10f, 0.56f, 0.10f }, maderaViga, 1.1f);
    }
    DrawCylinderEx({ -2.6f, 0.92f, -7.62f }, { -2.6f, 0.92f, -7.58f }, 0.40f, 0.40f, 22, Color{ 182, 188, 196, 255 });
    DrawCylinderEx({ -2.6f, 0.92f, -7.58f }, { -2.6f, 0.92f, -7.55f }, 0.10f, 0.10f, 10, Color{ 92, 96, 104, 255 });
    for (int k = 0; k < 18; k++)
    {
        float a0 = 6.2832f * (float)k / 18.0f;
        float a1 = a0 + 0.20f;
        DrawTriangle3D(
            { -2.6f + std::cos(a0) * 0.40f, 0.92f + std::sin(a0) * 0.40f, -7.6f },
            { -2.6f + std::cos(a1) * 0.40f, 0.92f + std::sin(a1) * 0.40f, -7.6f },
            { -2.6f + std::cos(a0) * 0.47f, 0.92f + std::sin(a0) * 0.47f, -7.6f },
            Color{ 120, 126, 134, 255 });
    }

    // Pila de tablas con separadores.
    for (int capa = 0; capa < 4; capa++)
    {
        for (int t = 0; t < 3; t++)
        {
            Color c = TinteTronco(Color{ 196, 150, 98, 255 }, 0.86f + 0.2f * Hash01Tronco(capa, t + 40));
            CajaTronco({ 2.7f, 0.20f + 0.17f * (float)capa, -7.95f + 0.42f * (float)t }, { 2.4f, 0.12f, 0.38f }, c);
        }
        DrawCubeV({ 2.0f, 0.27f + 0.17f * (float)capa, -7.5f }, { 0.08f, 0.05f, 1.3f }, maderaViga);
        DrawCubeV({ 3.4f, 0.27f + 0.17f * (float)capa, -7.5f }, { 0.08f, 0.05f, 1.3f }, maderaViga);
    }

    if (factor < 0.5f) return;

    // Hoja de sierra grande colgada en la pared.
    DrawCylinderEx({ 0.0f, 2.35f, zFondo + 0.06f }, { 0.0f, 2.35f, zFondo + 0.10f }, 0.62f, 0.62f, 24, Color{ 170, 176, 184, 255 });
    DrawCylinderEx({ 0.0f, 2.35f, zFondo + 0.10f }, { 0.0f, 2.35f, zFondo + 0.12f }, 0.42f, 0.42f, 20, Color{ 150, 110, 80, 255 });
    DrawCylinderEx({ 0.0f, 2.35f, zFondo + 0.12f }, { 0.0f, 2.35f, zFondo + 0.15f }, 0.10f, 0.10f, 10, Color{ 70, 72, 78, 255 });

    // Herramientas: hacha y una sierra de repuesto.
    DrawCylinderEx({ -1.3f, 1.55f, zFondo + 0.12f }, { -1.0f, 2.7f, zFondo + 0.12f }, 0.035f, 0.035f, 6, Color{ 156, 110, 66, 255 });
    CajaFacetadaTronco({ -0.98f, 2.62f, zFondo + 0.13f }, { 0.30f, 0.20f, 0.04f }, Color{ 150, 156, 166, 255 }, 1.2f);
    DrawCubeV({ 2.0f, 3.25f, zFondo + 0.11f }, { 1.8f, 0.16f, 0.02f }, Color{ 168, 174, 182, 255 });
    DrawCylinderEx({ 1.1f, 3.15f, zFondo + 0.12f }, { 1.1f, 3.55f, zFondo + 0.12f }, 0.04f, 0.04f, 6, Color{ 150, 104, 62, 255 });
    DrawCylinderEx({ 2.9f, 3.15f, zFondo + 0.12f }, { 2.9f, 3.55f, zFondo + 0.12f }, 0.04f, 0.04f, 6, Color{ 150, 104, 62, 255 });

    // Barril.
    DrawCylinderEx({ -4.0f, 0.10f, -6.95f }, { -4.0f, 0.95f, -6.95f }, 0.34f, 0.34f, 12, Color{ 116, 78, 48, 255 });
    DrawCylinderEx({ -4.0f, 0.30f, -6.95f }, { -4.0f, 0.36f, -6.95f }, 0.355f, 0.355f, 12, Color{ 60, 58, 60, 255 });
    DrawCylinderEx({ -4.0f, 0.72f, -6.95f }, { -4.0f, 0.78f, -6.95f }, 0.355f, 0.355f, 12, Color{ 60, 58, 60, 255 });
}


// Farol colgante o de poste: marco oscuro y nucleo luminoso.
static void FarolTronco(Vector3 centro, float brillo)
{
    Color marco = { 52, 44, 40, 255 };
    Color nucleo = MezclaTronco(Color{ 230, 160, 80, 255 }, Color{ 255, 236, 170, 255 }, brillo);

    DrawCubeV({ centro.x, centro.y - 0.17f, centro.z }, { 0.24f, 0.05f, 0.24f }, marco);
    DrawCubeV({ centro.x, centro.y + 0.17f, centro.z }, { 0.26f, 0.06f, 0.26f }, marco);
    DrawCylinderEx({ centro.x, centro.y + 0.20f, centro.z }, { centro.x, centro.y + 0.32f, centro.z }, 0.12f, 0.0f, 4, marco);
    DrawCubeV(centro, { 0.15f, 0.28f, 0.15f }, nucleo);

    for (int k = 0; k < 4; k++)
    {
        float dx = k % 2 == 0 ? -0.10f : 0.10f;
        float dz = k < 2 ? -0.10f : 0.10f;
        DrawCubeV({ centro.x + dx, centro.y, centro.z + dz }, { 0.025f, 0.32f, 0.025f }, marco);
    }
}


//==================================================
// VISUAL: ESTACIONES (UNA POR EQUIPO)
//==================================================

static void DibujarTarimaEquipoTronco(int equipo, float factor)
{
    float cx = CENTRO_EQUIPO_TRONCO[equipo];
    Color colorEquipo = COLOR_EQUIPO_TRONCO[equipo];
    const float medioX = 2.85f;
    const float z0 = -1.9f;
    const float z1 = 2.2f;
    const int TABLAS = 12;
    float ancho = medioX * 2.0f / (float)TABLAS;

    DrawCubeV({ cx, ALTURA_TARIMA_TRONCO * 0.5f - 0.01f, (z0 + z1) * 0.5f }, { medioX * 2.0f, ALTURA_TARIMA_TRONCO - 0.004f, z1 - z0 }, Color{ 56, 38, 26, 255 });

    for (int k = 0; k < TABLAS; k++)
    {
        float x = cx - medioX + ((float)k + 0.5f) * ancho;
        Color c = TinteTronco(Color{ 162, 120, 78, 255 }, 0.82f + 0.26f * Hash01Tronco(k, 31 + equipo * 7));
        CajaFacetadaTronco({ x, ALTURA_TARIMA_TRONCO * 0.5f, (z0 + z1) * 0.5f }, { ancho - 0.03f, ALTURA_TARIMA_TRONCO, z1 - z0 }, c, 1.08f);

        if (factor >= 0.5f)
        {
            DrawCubeV({ x, ALTURA_TARIMA_TRONCO + 0.003f, z0 + 0.3f }, { 0.035f, 0.006f, 0.035f }, Color{ 70, 60, 54, 255 });
            DrawCubeV({ x, ALTURA_TARIMA_TRONCO + 0.003f, z1 - 0.3f }, { 0.035f, 0.006f, 0.035f }, Color{ 70, 60, 54, 255 });
        }
    }

    // Borde pintado del color del equipo: marca de quien es la estacion.
    CajaFacetadaTronco({ cx, ALTURA_TARIMA_TRONCO + 0.02f, z1 + 0.06f }, { medioX * 2.0f + 0.24f, 0.11f, 0.12f }, colorEquipo, 1.2f);
    CajaFacetadaTronco({ cx, ALTURA_TARIMA_TRONCO + 0.02f, z0 - 0.06f }, { medioX * 2.0f + 0.24f, 0.11f, 0.12f }, TinteTronco(colorEquipo, 0.85f), 1.2f);
    CajaFacetadaTronco({ cx - medioX - 0.06f, ALTURA_TARIMA_TRONCO + 0.02f, (z0 + z1) * 0.5f }, { 0.12f, 0.11f, z1 - z0 }, colorEquipo, 1.2f);
    CajaFacetadaTronco({ cx + medioX + 0.06f, ALTURA_TARIMA_TRONCO + 0.02f, (z0 + z1) * 0.5f }, { 0.12f, 0.11f, z1 - z0 }, colorEquipo, 1.2f);
}


static void DibujarBanderaEquipoTronco(int equipo, float t)
{
    float px = CENTRO_EQUIPO_TRONCO[equipo];
    const float pz = -2.45f;
    const float alto = 3.3f;
    Color colorEquipo = COLOR_EQUIPO_TRONCO[equipo];

    CajaFacetadaTronco({ px, 0.12f, pz }, { 0.46f, 0.24f, 0.46f }, Color{ 120, 112, 104, 255 }, 1.25f);
    DrawCylinderEx({ px, 0.2f, pz }, { px, alto, pz }, 0.055f, 0.045f, 8, Color{ 96, 66, 42, 255 });
    DrawSphereEx({ px, alto + 0.05f, pz }, 0.09f, 8, 8, Color{ 214, 170, 70, 255 });

    // Pano en franjas que ondean con la brisa.
    const int FRANJAS = 7;
    const float anchoFranja = 0.19f;
    const float altoPano = 0.66f;
    float arriba = alto - 0.06f;

    for (int s = 0; s < FRANJAS; s++)
    {
        float x0 = px + 0.05f + (float)s * anchoFranja;
        float x1 = x0 + anchoFranja;
        float f0 = (float)s / (float)FRANJAS;
        float f1 = (float)(s + 1) / (float)FRANJAS;
        float w0 = std::sin(t * 4.2f - (float)s * 0.9f) * 0.09f * f0;
        float w1 = std::sin(t * 4.2f - (float)(s + 1) * 0.9f) * 0.09f * f1;
        float caida0 = 0.05f * f0;
        float caida1 = 0.05f * f1;
        float luz = 0.88f + 0.18f * std::sin(t * 4.2f - (float)s * 0.9f + 0.8f);
        Color c = TinteTronco(colorEquipo, luz);

        QuadTronco(
            { x0, arriba - caida0, pz + w0 },
            { x1, arriba - caida1, pz + w1 },
            { x1, arriba - altoPano - caida1, pz + w1 },
            { x0, arriba - altoPano - caida0, pz + w0 },
            c
        );

        // Franja clara al centro del pano.
        QuadTronco(
            { x0, arriba - altoPano * 0.42f - caida0, pz + w0 + 0.004f },
            { x1, arriba - altoPano * 0.42f - caida1, pz + w1 + 0.004f },
            { x1, arriba - altoPano * 0.58f - caida1, pz + w1 + 0.004f },
            { x0, arriba - altoPano * 0.58f - caida0, pz + w0 + 0.004f },
            TinteTronco(Color{ 246, 232, 204, 255 }, luz)
        );
    }
}


static void DibujarCaballeteTronco(float cx, float z, Color colorEquipo)
{
    const Color madera = { 126, 88, 54, 255 };

    for (int lado = -1; lado <= 1; lado += 2)
    {
        Vector3 pie = { cx - (float)lado * 0.40f, ALTURA_TARIMA_TRONCO, z };
        Vector3 punta = { cx + (float)lado * 0.70f, 1.02f, z };
        DrawCylinderEx(pie, punta, 0.058f, 0.05f, 6, TinteTronco(madera, lado > 0 ? 1.0f : 0.84f));

        Vector3 b0 = { pie.x + (punta.x - pie.x) * 0.80f, pie.y + (punta.y - pie.y) * 0.80f, z };
        Vector3 b1 = { pie.x + (punta.x - pie.x) * 0.92f, pie.y + (punta.y - pie.y) * 0.92f, z };
        DrawCylinderEx(b0, b1, 0.066f, 0.062f, 6, colorEquipo);
    }
}


// Media mitad del tronco en coordenadas locales: eje en el origen, de z = 0
// a z = signo * MEDIO_LARGO. La mitad de atras muestra su cara de corte
// (hacia la camara) solo cuando el tronco ya se partio.
static void DibujarMitadTronco(float signo, bool caraInterior, int semilla)
{
    const Color corteza = { 104, 70, 44, 255 };
    float largo = MEDIO_LARGO_TRONCO * signo;

    DrawCylinderEx({ 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, largo }, RADIO_TRONCO, RADIO_TRONCO * 0.98f, 20, corteza);

    // Vetas de la corteza.
    for (int k = 0; k < 6; k++)
    {
        float a = (25.0f + 26.0f * (float)k) * DEG2RAD;
        float largoVeta = MEDIO_LARGO_TRONCO * (0.70f + 0.25f * Hash01Tronco(k, semilla));
        DrawCubeV({ std::cos(a) * RADIO_TRONCO, std::sin(a) * RADIO_TRONCO, largo * 0.5f }, { 0.05f, 0.05f, largoVeta }, TinteTronco(corteza, k % 2 == 0 ? 0.62f : 0.78f));
    }

    // Musgo y nudo.
    DrawCubeV({ -0.10f, RADIO_TRONCO + 0.004f, largo * 0.62f }, { 0.30f, 0.03f, 0.50f }, Color{ 98, 122, 58, 255 });
    DrawSphereEx({ std::cos(150.0f * DEG2RAD) * RADIO_TRONCO, std::sin(150.0f * DEG2RAD) * RADIO_TRONCO, largo * 0.35f }, 0.08f, 6, 6, Color{ 74, 50, 32, 255 });

    if (signo > 0.0f)
    {
        AnillosCorteTronco({ 0.0f, 0.0f, largo }, RADIO_TRONCO * 0.95f, 1.0f, 20);
    }

    if (caraInterior)
    {
        AnillosCorteTronco({ 0.0f, 0.0f, 0.0f }, RADIO_TRONCO * 0.97f, 1.0f, 20);
    }
}


// Surco del corte: rebanadas oscuras hasta la profundidad actual, con la
// madera fresca clara a cada lado. Sobresale apenas de la corteza.
static void DibujarSurcoTronco(float cx, float progreso, float temblor)
{
    if (progreso <= 0.001f) return;

    float yTope = ALTURA_EJE_TRONCO + RADIO_TRONCO;
    float yFondo = AlturaDientesTronco(progreso);
    const int REBANADAS = 9;

    for (int k = 0; k < REBANADAS; k++)
    {
        float y0 = yTope - (yTope - yFondo) * (float)k / (float)REBANADAS;
        float y1 = yTope - (yTope - yFondo) * (float)(k + 1) / (float)REBANADAS;
        float yc = (y0 + y1) * 0.5f;
        float dy = yc - ALTURA_EJE_TRONCO;
        float cuerda = RADIO_TRONCO * RADIO_TRONCO - dy * dy;
        float medio = (cuerda > 0.0004f ? std::sqrt(cuerda) : 0.02f) + 0.014f;

        DrawCubeV({ cx + temblor, yc + 0.006f, 0.0f }, { medio * 2.0f, y0 - y1 + 0.012f, 0.13f }, Color{ 46, 28, 18, 255 });
        DrawCubeV({ cx + temblor, yc + 0.004f, 0.095f }, { medio * 2.0f - 0.01f, y0 - y1 + 0.008f, 0.06f }, Color{ 236, 198, 140, 255 });
        DrawCubeV({ cx + temblor, yc + 0.004f, -0.095f }, { medio * 2.0f - 0.01f, y0 - y1 + 0.008f, 0.06f }, Color{ 214, 176, 120, 255 });
    }
}


static void DibujarTroncoEquipo(const MinijuegoTronco& minijuego, int equipo, float factor)
{
    const EstadoEquipoTronco& estado = minijuego.equipos[equipo];
    float cx = CENTRO_EQUIPO_TRONCO[equipo];
    float progreso = estado.progresoCorte;
    float t = minijuego.tiempoAnimacion;
    float temblor = estado.sacudida > 0.0f
        ? std::sin(t * 71.0f) * 0.035f * (estado.sacudida / 0.38f) * ObtenerFactorMovimientoCamaraMinijuegos()
        : 0.0f;

    DibujarCaballeteTronco(cx, -1.12f, COLOR_EQUIPO_TRONCO[equipo]);
    DibujarCaballeteTronco(cx, 1.12f, COLOR_EQUIPO_TRONCO[equipo]);
    DrawCylinderEx({ cx, 0.22f, -1.12f }, { cx, 0.22f, 1.12f }, 0.045f, 0.045f, 6, Color{ 104, 72, 46, 255 });

    // Montones de aserrin a los costados del corte: crecen con el avance.
    for (int lado = -1; lado <= 1; lado += 2)
    {
        float px = cx + (float)lado * 0.80f;
        float alto = 0.02f + 0.22f * progreso;
        float radio = 0.12f + 0.36f * progreso;
        DrawCylinderEx({ px, ALTURA_TARIMA_TRONCO, 0.12f }, { px, ALTURA_TARIMA_TRONCO + alto, 0.12f }, radio, radio * 0.22f, 12, COLOR_ASERRIN_TRONCO);
        DrawCylinderEx({ px + (float)lado * 0.22f, ALTURA_TARIMA_TRONCO, 0.38f }, { px + (float)lado * 0.22f, ALTURA_TARIMA_TRONCO + alto * 0.6f, 0.38f }, radio * 0.7f, radio * 0.15f, 10, TinteTronco(COLOR_ASERRIN_TRONCO, 0.92f));
    }

    if (factor >= 0.5f)
    {
        int escamas = (int)(30.0f * progreso);
        for (int k = 0; k < escamas; k++)
        {
            float a = 6.2832f * Hash01Tronco(k, 300 + equipo);
            float d = 0.35f + 1.15f * Hash01Tronco(k, 310 + equipo);
            DrawCubeV(
                { cx + std::cos(a) * d * 1.3f, ALTURA_TARIMA_TRONCO + 0.008f, 0.12f + std::sin(a) * d * 0.9f },
                { 0.07f, 0.012f, 0.05f },
                TinteTronco(COLOR_ASERRIN_TRONCO, 0.85f + 0.2f * Hash01Tronco(k, 320))
            );
        }
    }

    // Partido: cada mitad cae de los caballetes hacia su lado y rebota.
    float caida = estado.caidaTronco;
    float k = caida >= 0.0f ? LimitarTronco(caida / DURACION_CAIDA_TRONCO, 0.0f, 1.0f) : 0.0f;
    float bajada = (ALTURA_EJE_TRONCO - RADIO_TRONCO - ALTURA_TARIMA_TRONCO) * k * k;
    float rebote = 0.0f;
    if (caida > DURACION_CAIDA_TRONCO)
    {
        float s = caida - DURACION_CAIDA_TRONCO;
        rebote = std::fabs(std::sin(s * 10.0f)) * 0.07f * std::exp(-s * 5.0f);
    }

    for (int mitad = 0; mitad < 2; mitad++)
    {
        float signo = mitad == 0 ? 1.0f : -1.0f;
        float separacion = signo * (0.85f * k + (caida > DURACION_CAIDA_TRONCO ? 0.12f * (1.0f - std::exp(-(caida - DURACION_CAIDA_TRONCO) * 3.0f)) : 0.0f));
        float giro = signo * 14.0f * k;

        rlPushMatrix();
        rlTranslatef(cx + temblor * 0.4f, ALTURA_EJE_TRONCO - bajada + rebote, separacion);
        rlRotatef(giro, 1.0f, 0.0f, 0.0f);
        DibujarMitadTronco(signo, caida >= 0.0f && signo < 0.0f, 400 + equipo * 10 + mitad);
        rlPopMatrix();
    }

    if (caida < 0.0f)
    {
        DibujarSurcoTronco(cx, progreso, temblor * 0.4f);
    }
}


static void DibujarSierraEquipo(const MinijuegoTronco& minijuego, int equipo)
{
    const EstadoEquipoTronco& estado = minijuego.equipos[equipo];
    float t = minijuego.tiempoAnimacion;
    float cx = CENTRO_EQUIPO_TRONCO[equipo] + DesplazamientoSierraTronco(estado, t);
    float yDientes = AlturaDientesTronco(estado.progresoCorte);

    // Al partirse el tronco, la pareja levanta la sierra.
    if (estado.caidaTronco >= 0.0f)
    {
        yDientes += 0.55f * LimitarTronco(estado.caidaTronco / 0.5f, 0.0f, 1.0f);
    }

    const float medio = LARGO_SIERRA_TRONCO * 0.5f;
    const Color acero = { 178, 184, 192, 255 };
    const Color aceroBrillo = { 232, 236, 242, 255 };
    const Color aceroOscuro = { 112, 118, 128, 255 };
    const Color madera = { 156, 108, 64, 255 };
    const Color bronce = { 204, 164, 76, 255 };
    Color colorEquipo = COLOR_EQUIPO_TRONCO[equipo];

    // Hoja con panza: mas ancha al centro, angosta en los mangos.
    const int SEGMENTOS = 12;
    for (int k = 0; k < SEGMENTOS; k++)
    {
        float u0 = (float)k / (float)SEGMENTOS;
        float u1 = (float)(k + 1) / (float)SEGMENTOS;
        float uc = (u0 + u1) * 0.5f;
        float panza = 1.0f - (2.0f * uc - 1.0f) * (2.0f * uc - 1.0f);
        float alto = 0.09f + 0.11f * panza;
        float x0 = cx - medio + u0 * LARGO_SIERRA_TRONCO;
        float x1 = cx - medio + u1 * LARGO_SIERRA_TRONCO;

        DrawCubeV({ (x0 + x1) * 0.5f, yDientes + 0.025f + alto * 0.5f, 0.0f }, { x1 - x0 + 0.003f, alto, 0.024f }, k % 2 == 0 ? acero : TinteTronco(acero, 0.93f));
        DrawCubeV({ (x0 + x1) * 0.5f, yDientes + 0.025f + alto - 0.012f, 0.0f }, { x1 - x0 + 0.003f, 0.024f, 0.030f }, aceroBrillo);
    }

    // Dientes en las dos caras de la hoja.
    const int DIENTES = 26;
    float pasoDiente = (LARGO_SIERRA_TRONCO - 0.16f) / (float)DIENTES;
    for (int k = 0; k < DIENTES; k++)
    {
        float xa = cx - medio + 0.08f + (float)k * pasoDiente;
        float xb = xa + pasoDiente;
        float xm = (xa + xb) * 0.5f;
        DrawTriangle3D({ xa, yDientes + 0.03f, 0.013f }, { xb, yDientes + 0.03f, 0.013f }, { xm, yDientes - 0.035f, 0.013f }, aceroOscuro);
        DrawTriangle3D({ xa, yDientes + 0.03f, -0.013f }, { xb, yDientes + 0.03f, -0.013f }, { xm, yDientes - 0.035f, -0.013f }, aceroOscuro);
    }

    // Mangos de madera con empunadura del color del equipo.
    for (int lado = -1; lado <= 1; lado += 2)
    {
        float xm = cx + (float)lado * (medio + 0.03f);
        DrawCylinderEx({ xm, yDientes - 0.02f, 0.0f }, { xm, yDientes + 0.44f, 0.0f }, 0.056f, 0.050f, 8, madera);
        DrawCylinderEx({ xm, yDientes + 0.14f, 0.0f }, { xm, yDientes + 0.36f, 0.0f }, 0.066f, 0.066f, 8, colorEquipo);
        DrawCylinderEx({ xm, yDientes - 0.02f, 0.0f }, { xm, yDientes + 0.05f, 0.0f }, 0.068f, 0.068f, 8, bronce);
        DrawSphereEx({ xm, yDientes + 0.46f, 0.0f }, 0.07f, 6, 6, TinteTronco(madera, 0.75f));
        DrawCylinderEx({ xm - (float)lado * 0.09f, yDientes + 0.10f, -0.025f }, { xm - (float)lado * 0.09f, yDientes + 0.10f, 0.025f }, 0.025f, 0.025f, 6, bronce);
    }
}


static Vector3 PiesJugadorTronco(
    const MinijuegoTronco& minijuego,
    const JugadorPrueba jugadores[],
    int indice
)
{
    int equipo = minijuego.equipoPorJugador[indice];
    Vector3 posicion = ObtenerPosicionJugadorTronco(
        equipo,
        minijuego.ordenEnEquipoPorJugador[indice],
        minijuego.cantidadJugadoresEquipo[equipo]
    );

    float direccionCentro = posicion.x < CENTRO_EQUIPO_TRONCO[equipo] ? 1.0f : -1.0f;

    // El cuerpo acompana la sierra en cada pasada.
    posicion.x += minijuego.equipos[equipo].posicionSierra * AMPLITUD_SIERRA_TRONCO * 0.45f;

    if (minijuego.estadosJugadores[indice].animacionGolpe > 0.0f)
    {
        posicion.x += direccionCentro * 0.08f;
    }

    posicion.y = posicion.y - jugadores[indice].tamano.y * 0.5f + 0.03f;

    if (
        minijuego.estado == TRONCO_FINALIZADO &&
        !minijuego.empate &&
        equipo == minijuego.equipoGanador
    )
    {
        posicion.y += std::fabs(std::sin(minijuego.tiempoAnimacion * 7.0f + (float)indice)) * 0.28f
            * ObtenerFactorMovimientoCamaraMinijuegos();
    }

    return posicion;
}


static bool JugadorVisibleTronco(
    const MinijuegoTronco& minijuego,
    const Participante participantes[],
    int indice
)
{
    return minijuego.equipoPorJugador[indice] >= 0 &&
        participantes[indice].activo &&
        participantes[indice].conectado;
}


static void DibujarJugadoresEquipoTronco(
    const MinijuegoTronco& minijuego,
    const JugadorPrueba jugadores[],
    const Participante participantes[],
    int cantidadMaxima,
    int equipo
)
{
    int limite = LimitarCantidadTronco(cantidadMaxima);

    for (int i = 0; i < limite; i++)
    {
        if (minijuego.equipoPorJugador[i] != equipo || !JugadorVisibleTronco(minijuego, participantes, i))
        {
            continue;
        }

        Vector3 pies = PiesJugadorTronco(minijuego, jugadores, i);
        Color color = participantes[i].color;

        AnilloSueloTronco(pies.x, pies.z, ALTURA_TARIMA_TRONCO + 0.012f, 0.0f, 0.36f, Fade(color, 0.22f));
        AnilloSueloTronco(pies.x, pies.z, ALTURA_TARIMA_TRONCO + 0.014f, 0.36f, 0.48f, Fade(color, 0.90f));

        float anguloY = pies.x < CENTRO_EQUIPO_TRONCO[equipo] + minijuego.equipos[equipo].posicionSierra * AMPLITUD_SIERRA_TRONCO * 0.45f
            ? 90.0f
            : -90.0f;

        DibujarModeloJugadorEnPosicion(pies, anguloY, color);
    }
}


static void DibujarLenaCentralTronco(float t)
{
    // Pila de lena con la cara de los cortes hacia la camara: separa las
    // dos estaciones.
    const float z = -0.7f;
    const float r = 0.20f;
    int semilla = 0;

    for (int fila = 0; fila < 4; fila++)
    {
        int cantidad = 4 - fila;
        float y = r + 0.02f + (float)fila * r * 1.72f;

        for (int k = 0; k < cantidad; k++)
        {
            float x = ((float)k - (float)(cantidad - 1) * 0.5f) * r * 2.02f;
            LenoTronco({ x, y, z + 0.06f * Hash01Tronco(fila, k) }, r, 1.0f + 0.1f * Hash01Tronco(k, fila + 5), semilla++);
        }
    }

    for (int lado = -1; lado <= 1; lado += 2)
    {
        DrawCylinderEx({ (float)lado * 0.92f, 0.0f, z }, { (float)lado * 0.98f, 1.15f, z }, 0.05f, 0.045f, 6, Color{ 92, 62, 40, 255 });
    }

    // Poste del farol central.
    DrawCylinderEx({ 0.0f, 0.0f, 1.55f }, { 0.0f, 2.75f, 1.55f }, 0.07f, 0.055f, 8, Color{ 78, 54, 38, 255 });
    CajaFacetadaTronco({ 0.0f, 0.12f, 1.55f }, { 0.34f, 0.24f, 0.34f }, Color{ 116, 108, 100, 255 }, 1.25f);
    DrawCylinderEx({ 0.0f, 2.68f, 1.55f }, { 0.0f, 2.68f, 1.95f }, 0.035f, 0.035f, 6, Color{ 60, 50, 44, 255 });
    DrawLine3D({ 0.0f, 2.68f, 1.92f }, { 0.0f, 2.47f, 1.92f }, Color{ 40, 36, 34, 255 });

    float brillo = 0.75f + 0.25f * std::sin(t * 9.0f) * std::sin(t * 3.7f);
    FarolTronco({ 0.0f, 2.28f, 1.92f }, brillo);
}


static void DibujarPrimerPlanoTronco(float factor, float t)
{
    const Color corteza = { 98, 66, 42, 255 };

    // Tocon con hacha clavada (izquierda).
    Vector3 tocon = { -4.7f, 0.0f, 3.75f };
    DrawCylinderEx(tocon, { tocon.x, 0.52f, tocon.z }, 0.50f, 0.45f, 14, corteza);
    for (int k = 0; k < 4; k++)
    {
        float a = 0.6f + 1.57f * (float)k;
        DrawCylinderEx(
            { tocon.x + std::cos(a) * 0.40f, 0.14f, tocon.z + std::sin(a) * 0.40f },
            { tocon.x + std::cos(a) * 0.78f, 0.0f, tocon.z + std::sin(a) * 0.78f },
            0.09f, 0.03f, 6, corteza);
    }
    {
        static const float RADIOS[5] = { 0.42f, 0.34f, 0.31f, 0.20f, 0.06f };
        static const Color COLORES[5] =
        {
            { 214, 170, 112, 255 }, { 160, 108, 64, 255 }, { 222, 180, 120, 255 }, { 158, 106, 62, 255 }, { 118, 76, 44, 255 }
        };
        for (int k = 0; k < 5; k++)
        {
            float y = 0.52f + 0.007f * (float)k;
            DrawCylinderEx({ tocon.x, y, tocon.z }, { tocon.x, y + 0.007f, tocon.z }, RADIOS[k], RADIOS[k], 14, COLORES[k]);
        }
    }
    DrawCylinderEx({ tocon.x + 0.05f, 0.58f, tocon.z }, { tocon.x + 0.55f, 1.15f, tocon.z + 0.18f }, 0.035f, 0.035f, 6, Color{ 160, 112, 66, 255 });
    CajaFacetadaTronco({ tocon.x + 0.02f, 0.60f, tocon.z }, { 0.30f, 0.13f, 0.05f }, Color{ 150, 156, 166, 255 }, 1.25f);

    // Lena partida al pie del tocon.
    for (int k = 0; k < 5; k++)
    {
        rlPushMatrix();
        rlTranslatef(tocon.x + 0.85f + 0.32f * (float)(k % 3), 0.08f + 0.13f * (float)(k / 3), tocon.z - 0.25f + 0.22f * (float)(k % 2));
        rlRotatef(20.0f + 47.0f * Hash01Tronco(k, 501), 0.0f, 1.0f, 0.0f);
        CajaFacetadaTronco({ 0.0f, 0.0f, 0.0f }, { 0.56f, 0.14f, 0.16f }, Color{ 206, 162, 106, 255 }, 1.15f);
        DrawCubeV({ 0.0f, 0.0f, -0.075f }, { 0.56f, 0.15f, 0.03f }, corteza);
        rlPopMatrix();
    }

    // Cajon con linterna y tronco tendido (derecha).
    Vector3 cajon = { 4.9f, 0.34f, 3.7f };
    CajaTronco(cajon, { 0.72f, 0.68f, 0.62f }, Color{ 136, 94, 58, 255 });
    for (int k = 0; k < 3; k++)
    {
        DrawCubeV({ cajon.x, 0.12f + 0.22f * (float)k, cajon.z + 0.315f }, { 0.70f, 0.05f, 0.012f }, Color{ 96, 64, 40, 255 });
    }
    float brillo = 0.75f + 0.25f * std::sin(t * 8.0f + 1.3f);
    FarolTronco({ cajon.x, 0.86f, cajon.z }, brillo);

    DrawCylinderEx({ 2.9f, 0.2f, 4.35f }, { 4.2f, 0.2f, 4.15f }, 0.20f, 0.19f, 10, corteza);

    if (factor < 0.5f) return;

    // Hongos junto al tocon.
    for (int k = 0; k < 3; k++)
    {
        float hx = tocon.x - 0.55f + 0.18f * (float)k;
        float hz = tocon.z + 0.45f - 0.08f * (float)k;
        float alto = 0.10f + 0.04f * (float)k;
        DrawCylinderEx({ hx, 0.0f, hz }, { hx, alto, hz }, 0.025f, 0.02f, 6, Color{ 236, 224, 200, 255 });
        DrawCylinderEx({ hx, alto, hz }, { hx, alto + 0.06f, hz }, 0.08f, 0.0f, 8, Color{ 196, 72, 52, 255 });
    }
}


static void DibujarParticulasTronco(const MinijuegoTronco& minijuego)
{
    for (int i = 0; i < MAX_PARTICULAS_TRONCO; i++)
    {
        const ParticulaTronco& p = minijuego.particulas[i];
        if (!p.activa) continue;

        float vida = p.vida / p.vidaMaxima;
        float alfa = 1.0f;
        Vector3 tamano = { p.tamano, p.tamano, p.tamano };

        switch (p.tipo)
        {
            case PARTICULA_TRONCO_CHISPA:
                alfa = vida;
                tamano = { p.tamano * (0.5f + 0.5f * vida), p.tamano * (0.5f + 0.5f * vida), p.tamano * (0.5f + 0.5f * vida) };
                break;
            case PARTICULA_TRONCO_CONFETI:
                alfa = vida < 0.25f ? vida / 0.25f : 1.0f;
                tamano = { p.tamano * (0.3f + 0.7f * std::fabs(std::sin(p.giro))), p.tamano * 0.18f, p.tamano * (0.3f + 0.7f * std::fabs(std::cos(p.giro))) };
                break;
            case PARTICULA_TRONCO_HOJA:
                alfa = vida < 0.15f ? vida / 0.15f : (vida > 0.94f ? (1.0f - vida) / 0.06f : 1.0f);
                tamano = { p.tamano * (0.5f + 0.5f * std::fabs(std::sin(p.giro))), p.tamano * 0.15f, p.tamano * 0.7f };
                break;
            case PARTICULA_TRONCO_MOTA:
                alfa = std::sin(vida * 3.1416f) * 0.85f;
                break;
            case PARTICULA_TRONCO_POLVO:
                alfa = vida * 0.55f;
                tamano = { p.tamano * (1.9f - 0.9f * vida), p.tamano * (1.2f - 0.4f * vida), p.tamano * (1.9f - 0.9f * vida) };
                break;
            default:
                alfa = vida < 0.35f ? vida / 0.35f : 1.0f;
                break;
        }

        Color color = p.color;
        color.a = (unsigned char)((float)p.color.a * LimitarTronco(alfa, 0.0f, 1.0f));
        DrawCubeV(p.posicion, tamano, color);
    }
}


static void DibujarResplandoresTronco(const MinijuegoTronco& minijuego)
{
    if (FactorCalidadGrafica(CalidadEfectos()) < 0.5f) return;

    float t = minijuego.tiempoAnimacion;

    rlDrawRenderBatchActive();
    rlDisableDepthMask();
    BeginBlendMode(BLEND_ADDITIVE);

    // Sol entre los pinos del fondo.
    HaloTronco({ 9.0f, 4.0f, -25.0f }, 9.0f, Color{ 255, 170, 96, 70 });

    // Farol central, farol del galpon y linterna del cajon.
    float parpadeo = 0.85f + 0.15f * std::sin(t * 9.0f) * std::sin(t * 3.7f);
    HaloTronco({ 0.0f, 2.28f, 1.98f }, 0.95f * parpadeo, Color{ 255, 192, 110, 130 });
    ResplandorSueloTronco(0.0f, 1.8f, 2.7f * parpadeo, Color{ 255, 160, 80, 60 });

    float parpadeoGalpon = 0.85f + 0.15f * std::sin(t * 7.0f + 2.0f);
    HaloTronco({ 0.0f, 2.35f, -8.7f }, 2.4f * parpadeoGalpon, Color{ 255, 150, 70, 55 });
    ResplandorSueloTronco(0.0f, -7.1f, 2.6f * parpadeoGalpon, Color{ 255, 150, 70, 50 });

    float parpadeoCajon = 0.85f + 0.15f * std::sin(t * 8.0f + 1.3f);
    HaloTronco({ 4.9f, 0.88f, 3.78f }, 0.75f * parpadeoCajon, Color{ 255, 192, 110, 120 });
    ResplandorSueloTronco(4.9f, 3.9f, 1.8f * parpadeoCajon, Color{ 255, 160, 80, 55 });

    for (int equipo = 0; equipo < 2; equipo++)
    {
        const EstadoEquipoTronco& estado = minijuego.equipos[equipo];
        float cx = CENTRO_EQUIPO_TRONCO[equipo];

        // Luz calida de atardecer sobre cada estacion.
        ResplandorSueloTronco(cx, 0.2f, 3.6f, Color{ 255, 150, 90, 26 });

        // Destello en el corte tras una pasada buena.
        if (estado.destello > 0.0f)
        {
            float k = estado.destello / 0.18f;
            HaloTronco({ cx, AlturaDientesTronco(estado.progresoCorte) + 0.05f, 0.35f }, 0.55f + 0.25f * k, Color{ 255, 226, 160, (unsigned char)(150.0f * k) });
        }

        // Error: el suelo de la estacion se tine de rojo un instante.
        if (estado.sacudida > 0.0f)
        {
            float k = estado.sacudida / 0.38f;
            ResplandorSueloTronco(cx, 0.15f, 2.8f, Color{ 230, 70, 50, (unsigned char)(110.0f * k) });
        }
    }

    // Equipo ganador: aro dorado que late bajo su estacion.
    if (minijuego.estado == TRONCO_FINALIZADO && !minijuego.empate && minijuego.equipoGanador >= 0)
    {
        float pulso = 0.8f + 0.2f * std::sin(t * 5.0f);
        ResplandorSueloTronco(CENTRO_EQUIPO_TRONCO[minijuego.equipoGanador], 0.2f, 4.0f * pulso, Color{ 255, 214, 96, 70 });
    }

    EndBlendMode();
    rlEnableDepthMask();
}


static void DibujarEscenarioTronco(
    const MinijuegoTronco& minijuego,
    const JugadorPrueba jugadores[],
    const Participante participantes[],
    int cantidadMaxima
)
{
    float decoracion = FactorCalidadGrafica(CalidadDecoracion());
    float t = minijuego.tiempoAnimacion;

    DibujarCieloTronco();

    BeginMode3D(minijuego.camara);
    CulloTronco(false);

    DibujarBosqueFondoTronco(decoracion);
    DibujarSueloTronco(decoracion);
    DibujarGalponTronco(decoracion);

    // Farol colgado de la viga del galpon.
    DrawLine3D({ 0.0f, 3.33f, -6.3f }, { 0.0f, 2.95f, -6.3f }, Color{ 40, 36, 34, 255 });
    FarolTronco({ 0.0f, 2.78f, -6.3f }, 0.8f + 0.2f * std::sin(t * 7.0f + 2.0f));

    DibujarArbolesLateralesTronco(decoracion);
    DibujarDetallesSueloTronco(decoracion);

    for (int equipo = 0; equipo < 2; equipo++)
    {
        DibujarTarimaEquipoTronco(equipo, decoracion);
        DibujarBanderaEquipoTronco(equipo, t);
        DibujarTroncoEquipo(minijuego, equipo, decoracion);

        if (minijuego.cantidadJugadoresEquipo[equipo] > 0)
        {
            DibujarSierraEquipo(minijuego, equipo);
        }

        DibujarJugadoresEquipoTronco(minijuego, jugadores, participantes, cantidadMaxima, equipo);
    }

    DibujarLenaCentralTronco(t);
    DibujarPrimerPlanoTronco(decoracion, t);
    DibujarParticulasTronco(minijuego);
    DibujarResplandoresTronco(minijuego);

    CulloTronco(true);
    EndMode3D();
}


//==================================================
// INTERFAZ: SENALES SOBRE EL MUNDO Y HUD COMUN
//==================================================

// Sobre cada jugador: su rotulo y la accion que le toca (¡TIRÁ! /
// ¡EMPUJÁ!) o ¡LISTO! si ya respondio. Se lee del estado de este frame:
// misma anticipacion que las tarjetas anteriores.
static void DibujarSenalesTronco(
    const MinijuegoTronco& minijuego,
    const JugadorPrueba jugadores[],
    const Participante participantes[],
    int cantidadMaxima
)
{
    bool mostrarAccion =
        minijuego.estado == TRONCO_PREPARANDO ||
        minijuego.estado == TRONCO_JUGANDO;

    int limite = LimitarCantidadTronco(cantidadMaxima);

    for (int i = 0; i < limite; i++)
    {
        if (!JugadorVisibleTronco(minijuego, participantes, i))
        {
            continue;
        }

        Vector3 pies = PiesJugadorTronco(minijuego, jugadores, i);
        Vector2 cabeza = GetWorldToScreen({ pies.x, pies.y + 1.80f, pies.z }, minijuego.camara);

        char rotulo[8];
        std::snprintf(rotulo, sizeof(rotulo), "J%d", participantes[i].numeroJugador);

        OpcionesEtiquetaUI opciones;
        opciones.borde = participantes[i].color;
        opciones.pico = true;
        Rectangle placa = DibujarEtiquetaUI(rotulo, { cabeza.x, cabeza.y - Px(16.0f) }, 0.0f, opciones);

        if (!mostrarAccion)
        {
            continue;
        }

        const EstadoJugadorTronco& estado = minijuego.estadosJugadores[i];
        AccionTronco accion = ObtenerAccionEsperadaTronco(minijuego, i);
        float y = placa.y - Px(4.0f);

        if (!participantes[i].esBot && !estado.respondio)
        {
            char tecla[24];
            std::snprintf(tecla, sizeof(tecla), "[%s]", TextoAccionTronco(participantes[i], accion));
            y -= (float)TamanoTexto(UI_TEXTO_PEQUENO);
            DibujarTextoMundoContornoUI(tecla, cabeza.x, y, UI_TEXTO_PEQUENO, UI_TEXTO_MUNDO);
            y -= Px(3.0f);
        }

        const char* texto = estado.respondio
            ? "¡LISTO!"
            : (accion == ACCION_TRONCO_TIRAR ? "¡TIRÁ!" : "¡EMPUJÁ!");

        y -= (float)TamanoTexto(UI_TEXTO_GRANDE);
        DibujarTextoMundoContornoUI(texto, cabeza.x, y, UI_TEXTO_GRANDE, estado.respondio ? UI_POSITIVO : UI_FOCO);
    }

    // Avisos de cada estacion: suben sobre el tronco y se desvanecen.
    for (int equipo = 0; equipo < 2; equipo++)
    {
        const EstadoEquipoTronco& estado = minijuego.equipos[equipo];
        Vector2 ancla = GetWorldToScreen(
            { CENTRO_EQUIPO_TRONCO[equipo], ALTURA_EJE_TRONCO + RADIO_TRONCO + 0.30f, 0.0f },
            minijuego.camara
        );

        if (estado.caidaTronco >= 0.0f && estado.caidaTronco < 1.6f)
        {
            float k = estado.caidaTronco / 1.6f;
            DibujarTextoMundoContornoUI("¡CRAC!", ancla.x, ancla.y - Px(40.0f) - Px(36.0f) * k, UI_TEXTO_GRANDE, UI_FOCO, LimitarTronco((1.0f - k) * 3.0f, 0.0f, 1.0f));
        }
        else if (estado.avisoError > 0.0f)
        {
            float k = 1.0f - estado.avisoError / 0.85f;
            DibujarTextoMundoContornoUI("¡DESCOORDINADOS!", ancla.x, ancla.y - Px(24.0f) - Px(26.0f) * k, UI_TEXTO, UI_PELIGRO, LimitarTronco(estado.avisoError / 0.25f, 0.0f, 1.0f));
        }
        else if (estado.avisoHito > 0.0f && estado.hito > 0)
        {
            char texto[16];
            std::snprintf(texto, sizeof(texto), "¡%d %%!", estado.hito * 25);
            float k = 1.0f - estado.avisoHito / 0.9f;
            DibujarTextoMundoContornoUI(texto, ancla.x, ancla.y - Px(24.0f) - Px(26.0f) * k, UI_TEXTO, UI_POSITIVO, LimitarTronco(estado.avisoHito / 0.25f, 0.0f, 1.0f));
        }
    }
}


static void DibujarHudTronco(
    const MinijuegoTronco& minijuego,
    const Participante participantes[]
)
{
    bool dosVsDos =
        minijuego.cantidadJugadoresEquipo[0] == 2 &&
        minijuego.cantidadJugadoresEquipo[1] == 2;

    DatosHudMinijuego hud;
    PrepararDatosHudMinijuego(
        hud,
        dosVsDos ? "TRONCO 2 VS 2" : "TRONCO 1 VS 1",
        dosVsDos
            ? "TIRÁ Y EMPUJÁ A LA PAR DE TU COMPAÑERO. EL PRIMER EQUIPO QUE CORTA SU TRONCO GANA."
            : "TIRÁ Y EMPUJÁ LA SIERRA A TIEMPO. EL PRIMERO QUE CORTA SU TRONCO GANA.",
        minijuego.resultado
    );

    hud.fase = minijuego.estado == TRONCO_PREPARANDO
        ? HUD_MINIJUEGO_CUENTA
        : (minijuego.estado == TRONCO_JUGANDO ? HUD_MINIJUEGO_JUGANDO : HUD_MINIJUEGO_FIN);
    hud.cuentaRestante = minijuego.tiempoPreparacion;

    float restante = DURACION_MAXIMA_TRONCO - minijuego.tiempoPartida;
    hud.reloj = HUD_RELOJ_CUENTA_ATRAS;
    hud.tiempoRestante = restante > 0.0f ? restante : 0.0f;
    hud.tiempoTotal = DURACION_MAXIMA_TRONCO;
    hud.segundosAlerta = 10.0f;
    hud.finPorTiempo = restante <= 0.0f;

    if (minijuego.estado == TRONCO_FINALIZADO)
    {
        hud.tituloFinal = minijuego.empate
            ? "¡EMPATE!"
            : (minijuego.equipoGanador == 0 ? "¡GANA EL EQUIPO ROJO!" : "¡GANA EL EQUIPO AZUL!");

        if (hud.finPorTiempo)
        {
            hud.notaFinal = minijuego.empate
                ? "NADIE TERMINÓ Y LOS DOS CORTES QUEDARON IGUALES."
                : "NADIE TERMINÓ EL CORTE: GANA EL EQUIPO QUE MÁS AVANZÓ.";
        }
    }

    for (int i = 0; i < MAX_PARTICIPANTES && i < MAX_JUGADORES_PRUEBA; i++)
    {
        JugadorHudMinijuego& jugador = hud.jugadores[i];
        int equipo = minijuego.equipoPorJugador[i];

        if (!jugador.visible || equipo < 0)
        {
            continue;
        }

        const EstadoEquipoTronco& estadoEquipo = minijuego.equipos[equipo];
        int porcentaje = (int)(estadoEquipo.progresoCorte * 100.0f + 0.5f);

        std::snprintf(jugador.valor, sizeof(jugador.valor), "%d %%", porcentaje);
        jugador.valorNumerico = porcentaje;
        jugador.progreso = estadoEquipo.progresoCorte;
        jugador.equipo = equipo;
        jugador.rol = NOMBRE_EQUIPO_TRONCO[equipo];

        if (minijuego.estado == TRONCO_JUGANDO)
        {
            if (estadoEquipo.avisoError > 0.0f)
            {
                std::snprintf(jugador.estado, sizeof(jugador.estado), "DESCOORDINADOS");
            }
            else if (minijuego.estadosJugadores[i].respondio && minijuego.estadosJugadores[i].acerto)
            {
                std::snprintf(jugador.estado, sizeof(jugador.estado), "¡LISTO!");
            }
        }

        std::snprintf(jugador.detalleFinal, sizeof(jugador.detalleFinal), "CORTE %d %%", porcentaje);

        if (!participantes[i].esBot)
        {
            std::snprintf(
                jugador.controles,
                sizeof(jugador.controles),
                "TIRAR: %s\nEMPUJAR: %s",
                TextoAccionTronco(participantes[i], ACCION_TRONCO_TIRAR),
                TextoAccionTronco(participantes[i], ACCION_TRONCO_EMPUJAR)
            );
        }
    }

    DibujarHudMinijuego(hud, participantes);
}


void MinijuegoTronco::Dibujar(
    const JugadorPrueba jugadores[],
    int cantidadMaxima,
    const Participante participantes[]
) const
{

    DibujarEscenarioTronco(*this, jugadores, participantes, cantidadMaxima);

    float centroX = GetScreenWidth() * 0.5f;

    if (!partidaValida)
    {
        DibujarTextoMundoContornoUI(
            "ESPERANDO 2 O 4 JUGADORES ACTIVOS Y CONECTADOS",
            centroX,
            Px(300.0f),
            UI_TEXTO_GRANDE,
            UI_FOCO
        );
    }
    else
    {
        DibujarSenalesTronco(*this, jugadores, participantes, cantidadMaxima);
    }

    if (avisoReconfiguracion > 0.0f)
    {
        DibujarTextoMundoContornoUI(
            "SE RECONFIGURARON LOS EQUIPOS",
            centroX,
            Px(160.0f),
            UI_TEXTO_GRANDE,
            UI_FOCO,
            LimitarTronco(avisoReconfiguracion / 0.5f, 0.0f, 1.0f)
        );
    }

    if (partidaValida)
    {
        DibujarHudTronco(*this, participantes);
    }
}


const ResultadoMinijuego&
MinijuegoTronco::ObtenerResultado() const
{
    return resultado;
}
