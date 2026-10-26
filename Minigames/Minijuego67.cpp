#include "Minigames/Minijuego67.h"
#include "Minigames/AudioMinijuegos.h"
#include "Minigames/HudMinijuego.h"
#include "Minigames/ModeloJugadorCompartido.h"

#include "Systems/CalidadGrafica.h"
#include "Systems/Input.h"
#include "UI/EstiloUI.h"

#include "raymath.h"
#include "rlgl.h"

#include <cmath>
#include <cstdio>

//==================================================
// CONSTANTES
//==================================================

static const float DURACION_PREPARACION_67 = 2.5f;
static const float DURACION_PARTIDA_67 = 30.0f;

// La ventana anterior era demasiado ancha. Ahora hay que esperar
// a que la pieza quede realmente a mano del jugador.
static const float INICIO_ZONA_RECOGIDA_67 = 0.755f;
static const float FIN_ZONA_RECOGIDA_67 = 0.815f;

static const float X_INICIO_CINTA_67 = -6.3f;
static const float LARGO_CINTA_67 = 9.0f;
static const float X_JUGADOR_67 = 1.35f;
static const float X_MESA_67 = 3.45f;

static const float Z_CINTA_6_67 = -2.35f;
static const float Z_CINTA_7_67 = 2.35f;
static const float Z_JUGADOR_6_67 = -0.72f;
static const float Z_JUGADOR_7_67 = 0.72f;

// Cuatro piezas por cinta dejan espacio visual. El 7 queda atrasado respecto
// del 6, por lo que el 6 cruza primero la zona de recogida en cada ciclo.
static const int CANTIDAD_PIEZAS_CINTA_67 = 4;
static const float DESFASE_CINTA_7_67 = -0.150f;

static const float DURACION_GIRO_67 = 0.24f;
static const float DURACION_STUN_67 = 0.42f;
static const float DURACION_MESA_COMPLETA_67 = 0.48f;
static const float DURACION_FEEDBACK_67 = 0.55f;
static const float COOLDOWN_TRAS_AGARRAR_67 = 0.30f;
static const float COOLDOWN_TRAS_COLOCAR_67 = 0.38f;

static const int PROBABILIDAD_AGARRE_BOT_67 = 50;


//==================================================
// UTILIDADES
//==================================================

static int ContarParticipantesActivos67(
    const Participante participantes[],
    int cantidadMaxima
)
{
    int cantidad = 0;

    for (int i = 0; i < cantidadMaxima; i++)
    {
        if (participantes[i].activo)
        {
            cantidad++;
        }
    }

    return cantidad;
}


static int ContarParticipantesDisponibles67(
    const Participante participantes[],
    int cantidadMaxima
)
{
    int cantidad = 0;

    for (int i = 0; i < cantidadMaxima; i++)
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


static int CrearMascaraParticipantes67(
    const Participante participantes[],
    int cantidadMaxima
)
{
    int mascara = 0;

    for (int i = 0; i < cantidadMaxima; i++)
    {
        if (participantes[i].activo)
        {
            mascara |= 1 << i;

            if (participantes[i].conectado)
            {
                mascara |= 1 << (i + MAX_JUGADORES_PRUEBA);
            }
        }
    }

    return mascara;
}


static float NormalizarProgreso67(float progreso)
{
    progreso = std::fmod(progreso, 1.0f);

    if (progreso < 0.0f)
    {
        progreso += 1.0f;
    }

    return progreso;
}


static float ObtenerXObjeto67(float progreso)
{
    return X_INICIO_CINTA_67 + progreso * LARGO_CINTA_67;
}


static float ObtenerProgresoPieza67(
    float progresoBase,
    int indicePieza
)
{
    float separacion =
        1.0f / (float)CANTIDAD_PIEZAS_CINTA_67;

    return NormalizarProgreso67(
        progresoBase +
        separacion * (float)indicePieza
    );
}


static bool HayPiezaAlAlcance67(
    const EstadoEquipo67& equipo,
    TipoPieza67 tipo
)
{
    float progresoBase =
        tipo == PIEZA_NUMERO_6
        ? equipo.progresoCinta6
        : equipo.progresoCinta7;

    for (int i = 0; i < CANTIDAD_PIEZAS_CINTA_67; i++)
    {
        float progreso =
            ObtenerProgresoPieza67(
                progresoBase,
                i
            );

        if (
            progreso >= INICIO_ZONA_RECOGIDA_67 &&
            progreso <= FIN_ZONA_RECOGIDA_67
        )
        {
            return true;
        }
    }

    return false;
}


static Color ObtenerColorEquipo67(int equipo)
{
    return equipo == 0
        ? Color{ 238, 55, 66, 255 }
        : Color{ 40, 159, 224, 255 };
}


static int BuscarJugadorUnicoEquipo67(
    const Minijuego67& minijuego,
    int equipo,
    int cantidadMaxima
)
{
    for (int i = 0; i < cantidadMaxima; i++)
    {
        if (minijuego.equipoPorJugador[i] == equipo)
        {
            return i;
        }
    }

    return -1;
}


static void ActualizarRolJugadorUnico67(
    Minijuego67& minijuego,
    int equipo,
    int cantidadMaxima
)
{
    if (
        equipo < 0 ||
        equipo > 1 ||
        minijuego.cantidadJugadoresEquipo[equipo] != 1
    )
    {
        return;
    }

    int indice =
        BuscarJugadorUnicoEquipo67(
            minijuego,
            equipo,
            cantidadMaxima
        );

    if (indice < 0)
    {
        return;
    }

    minijuego.estadosJugadores[indice].tipoPieza =
        minijuego.equipos[equipo].mesa == MESA_67_CON_6
        ? PIEZA_NUMERO_7
        : PIEZA_NUMERO_6;
}


static float ObtenerZJugador67(
    const Minijuego67& minijuego,
    int indiceJugador
)
{
    int equipo = minijuego.equipoPorJugador[indiceJugador];

    if (equipo < 0 || equipo > 1)
    {
        return 0.0f;
    }

    if (minijuego.cantidadJugadoresEquipo[equipo] <= 1)
    {
        return 0.0f;
    }

    return
        minijuego.ordenEnEquipoPorJugador[indiceJugador] == 0
        ? Z_JUGADOR_6_67
        : Z_JUGADOR_7_67;
}


//==================================================
// EFECTOS (solo visuales: no tocan la logica)
//==================================================

// Altura de la superficie de las cintas.
static const float Y_CINTA_67 = 0.70f;

static const Color NIEBLA_67 = { 30, 34, 44, 255 };
static const Color COLOR_6_67 = { 255, 150, 52, 255 };
static const Color COLOR_7_67 = { 86, 196, 255, 255 };
static const Color ACERO_67 = { 84, 90, 102, 255 };
static const Color ACERO_OSCURO_67 = { 50, 54, 64, 255 };
static const Color AMARILLO_67 = { 236, 186, 54, 255 };
static const Color LUZ_CALIDA_67 = { 255, 200, 130, 255 };
static const Color DESTELLO_67 = { 255, 244, 190, 255 };

// Brazos mecanicos (detras de la cinta del 6) y valvulas de vapor.
static const Vector3 BASE_BRAZOS_67[2] = { { -4.7f, 0.0f, -3.95f }, { 1.75f, 0.0f, -3.95f } };
static const float FASE_BRAZOS_67[2] = { 0.0f, 2.1f };
static const Vector3 VALVULAS_67[2] = { { -5.4f, 3.05f, -4.45f }, { -8.2f, 2.65f, -4.45f } };

static const float ALTO_BASE_BRAZO_67 = 0.42f;
static const float LARGO_BRAZO_67 = 1.15f;
static const float LARGO_ANTEBRAZO_67 = 0.85f;
static const float LARGO_HERRAMIENTA_67 = 0.28f;


// Azar propio para efectos: no consume el GetRandomValue de la logica.
static float Azar67(Minijuego67& minijuego)
{
    minijuego.semillaEfectos = minijuego.semillaEfectos * 1664525u + 1013904223u;
    return (float)((minijuego.semillaEfectos >> 8) & 0xFFFFu) / 65535.0f;
}


static float AzarRango67(Minijuego67& minijuego, float minimo, float maximo)
{
    return minimo + (maximo - minimo) * Azar67(minijuego);
}


static int CantidadEfecto67(int cantidad)
{
    int n = (int)std::lround((float)cantidad * FactorCalidadGrafica(CalidadParticulas()));
    return n < 1 ? 1 : n;
}


static void EmitirParticula67(
    Minijuego67& minijuego,
    int equipo,
    TipoParticula67 tipo,
    Vector3 posicion,
    Vector3 velocidad,
    float vida,
    float tamano,
    Color color
)
{
    for (int i = 0; i < MAX_PARTICULAS_67; i++)
    {
        Particula67& p = minijuego.particulas[i];

        if (p.activa)
        {
            continue;
        }

        p.activa = true;
        p.equipo = equipo;
        p.tipo = tipo;
        p.posicion = posicion;
        p.velocidad = velocidad;
        p.vida = vida;
        p.vidaMaxima = vida;
        p.tamano = tamano;
        p.giro = Azar67(minijuego) * 360.0f;
        p.color = color;
        return;
    }
}


static void Chispas67(
    Minijuego67& minijuego,
    int equipo,
    Vector3 posicion,
    int cantidad,
    Color color,
    float fuerza
)
{
    int n = CantidadEfecto67(cantidad);

    for (int i = 0; i < n; i++)
    {
        float angulo = Azar67(minijuego) * 6.2832f;
        float rapidez = fuerza * AzarRango67(minijuego, 0.35f, 1.0f);
        Vector3 velocidad =
        {
            std::cos(angulo) * rapidez,
            AzarRango67(minijuego, 1.2f, 3.2f) * fuerza * 0.7f,
            std::sin(angulo) * rapidez
        };

        EmitirParticula67(
            minijuego,
            equipo,
            PARTICULA_67_CHISPA,
            posicion,
            velocidad,
            AzarRango67(minijuego, 0.35f, 0.75f),
            AzarRango67(minijuego, 0.05f, 0.09f),
            Azar67(minijuego) < 0.4f ? DESTELLO_67 : color
        );
    }
}


static void Humo67(
    Minijuego67& minijuego,
    int equipo,
    Vector3 posicion,
    int cantidad
)
{
    int n = CantidadEfecto67(cantidad);

    for (int i = 0; i < n; i++)
    {
        Vector3 inicio =
        {
            posicion.x + AzarRango67(minijuego, -0.25f, 0.25f),
            posicion.y + AzarRango67(minijuego, -0.1f, 0.15f),
            posicion.z + AzarRango67(minijuego, -0.25f, 0.25f)
        };

        Vector3 velocidad =
        {
            AzarRango67(minijuego, -0.5f, 0.5f),
            AzarRango67(minijuego, 0.6f, 1.3f),
            AzarRango67(minijuego, -0.5f, 0.5f)
        };

        unsigned char gris = (unsigned char)AzarRango67(minijuego, 62.0f, 96.0f);

        EmitirParticula67(
            minijuego,
            equipo,
            PARTICULA_67_HUMO,
            inicio,
            velocidad,
            AzarRango67(minijuego, 0.8f, 1.3f),
            AzarRango67(minijuego, 0.16f, 0.28f),
            Color{ gris, gris, (unsigned char)(gris + 6), 255 }
        );
    }
}


static void Confeti67(
    Minijuego67& minijuego,
    int equipo,
    int cantidad,
    Color colorEquipo
)
{
    static const Color COLORES[4] =
    {
        { 255, 214, 90, 255 },
        { 255, 250, 240, 255 },
        { 110, 220, 150, 255 },
        { 255, 140, 200, 255 }
    };

    int n = CantidadEfecto67(cantidad);

    for (int i = 0; i < n; i++)
    {
        Vector3 posicion =
        {
            AzarRango67(minijuego, -7.5f, 4.5f),
            AzarRango67(minijuego, 4.8f, 7.6f),
            AzarRango67(minijuego, -3.5f, 5.0f)
        };

        Vector3 velocidad =
        {
            AzarRango67(minijuego, -0.6f, 0.6f),
            AzarRango67(minijuego, -0.6f, 0.0f),
            AzarRango67(minijuego, -0.6f, 0.6f)
        };

        int indiceColor = (int)(Azar67(minijuego) * 5.0f);
        Color color = indiceColor >= 4 ? colorEquipo : COLORES[indiceColor];

        EmitirParticula67(
            minijuego,
            equipo,
            PARTICULA_67_CONFETI,
            posicion,
            velocidad,
            AzarRango67(minijuego, 2.8f, 4.0f),
            AzarRango67(minijuego, 0.12f, 0.18f),
            color
        );
    }
}


static void PonerAviso67(
    EstadoEquipo67& equipo,
    TipoAviso67 tipo,
    Vector3 posicion
)
{
    int elegido = 0;

    for (int i = 0; i < MAX_AVISOS_67; i++)
    {
        if (equipo.avisos[i].tiempo <= 0.0f)
        {
            elegido = i;
            break;
        }

        if (equipo.avisos[i].tiempo < equipo.avisos[elegido].tiempo)
        {
            elegido = i;
        }
    }

    equipo.avisos[elegido].tipo = tipo;
    equipo.avisos[elegido].tiempo = 1.1f;
    equipo.avisos[elegido].posicion = posicion;
}


// Pose de un brazo mecanico (grados). Un solo ciclo: sube, gira y baja
// la herramienta sobre su banco; al bajar del todo "suelda".
struct PoseBrazo67
{
    float giro = 0.0f;
    float hombro = 0.0f;
    float codo = 0.0f;
    bool soldando = false;
};


static PoseBrazo67 CalcularPoseBrazo67(float tiempo, float fase)
{
    float f = tiempo * 0.85f + fase;
    PoseBrazo67 pose;
    pose.giro = 28.0f * std::sin(f);
    pose.hombro = -38.0f - 12.0f * std::sin(f * 2.0f + 0.6f);
    pose.codo = 128.0f + 18.0f * std::sin(f * 2.0f);
    pose.soldando = std::sin(f * 2.0f) > 0.8f;
    return pose;
}


static Vector3 PuntaBrazo67(Vector3 base, const PoseBrazo67& pose)
{
    float hombro = pose.hombro * DEG2RAD;
    float total = (pose.hombro + pose.codo) * DEG2RAD;
    float largo = LARGO_ANTEBRAZO_67 + LARGO_HERRAMIENTA_67;

    float lx = -std::sin(hombro) * LARGO_BRAZO_67 - std::sin(total) * largo;
    float ly = std::cos(hombro) * LARGO_BRAZO_67 + std::cos(total) * largo;
    float giro = pose.giro * DEG2RAD;

    return
    {
        base.x + lx * std::cos(giro),
        base.y + ALTO_BASE_BRAZO_67 + ly,
        base.z - lx * std::sin(giro)
    };
}


static void ActualizarEfectos67(
    Minijuego67& minijuego,
    float deltaTime
)
{
    for (int i = 0; i < MAX_PARTICULAS_67; i++)
    {
        Particula67& p = minijuego.particulas[i];

        if (!p.activa)
        {
            continue;
        }

        p.vida -= deltaTime;

        if (p.vida <= 0.0f)
        {
            p.activa = false;
            continue;
        }

        switch (p.tipo)
        {
            case PARTICULA_67_CHISPA:
                p.velocidad.y -= 9.0f * deltaTime;
                p.velocidad = Vector3Scale(p.velocidad, std::exp(-1.4f * deltaTime));

                if (p.posicion.y < 0.06f && p.velocidad.y < 0.0f)
                {
                    p.velocidad.y = -p.velocidad.y * 0.3f;
                }
                break;

            case PARTICULA_67_HUMO:
            case PARTICULA_67_VAPOR:
                p.velocidad = Vector3Scale(p.velocidad, std::exp(-1.1f * deltaTime));
                p.velocidad.y += 0.25f * deltaTime;
                p.tamano += (p.tipo == PARTICULA_67_VAPOR ? 0.55f : 0.40f) * deltaTime;
                break;

            case PARTICULA_67_CONFETI:
                p.velocidad.y -= 2.2f * deltaTime;
                if (p.velocidad.y < -1.9f) p.velocidad.y = -1.9f;
                p.velocidad.x += std::sin(minijuego.tiempoAnimacion * 5.0f + (float)i) * 2.0f * deltaTime;
                p.giro += 260.0f * deltaTime;

                if (p.posicion.y < 0.04f)
                {
                    p.posicion.y = 0.04f;
                    p.velocidad = { 0.0f, 0.0f, 0.0f };
                }
                break;
        }

        p.posicion = Vector3Add(p.posicion, Vector3Scale(p.velocidad, deltaTime));
    }

    for (int equipo = 0; equipo < 2; equipo++)
    {
        EstadoEquipo67& estado = minijuego.equipos[equipo];

        estado.sacudida -= deltaTime * 2.4f;
        if (estado.sacudida < 0.0f) estado.sacudida = 0.0f;

        estado.destello6 -= deltaTime * 1.6f;
        if (estado.destello6 < 0.0f) estado.destello6 = 0.0f;

        estado.destello7 -= deltaTime * 1.1f;
        if (estado.destello7 < 0.0f) estado.destello7 = 0.0f;

        for (int i = 0; i < MAX_AVISOS_67; i++)
        {
            if (estado.avisos[i].tiempo > 0.0f)
            {
                estado.avisos[i].tiempo -= deltaTime;
            }
        }
    }

    // Ambiente: vapor de las valvulas y chispas de soldadura. La escena
    // es la misma en las dos vistas, asi que se emite para ambas.
    float factor = FactorCalidadGrafica(CalidadParticulas());

    if (CalidadDecoracion() != CALIDAD_BAJA)
    {
        minijuego.acumuladorVapor += deltaTime * 5.0f * factor;

        while (minijuego.acumuladorVapor >= 1.0f)
        {
            minijuego.acumuladorVapor -= 1.0f;
            Vector3 valvula = VALVULAS_67[Azar67(minijuego) < 0.6f ? 0 : 1];

            for (int equipo = 0; equipo < 2; equipo++)
            {
                EmitirParticula67(
                    minijuego,
                    equipo,
                    PARTICULA_67_VAPOR,
                    { valvula.x + AzarRango67(minijuego, -0.08f, 0.08f), valvula.y, valvula.z + 0.2f },
                    { AzarRango67(minijuego, -0.2f, 0.2f), AzarRango67(minijuego, 0.5f, 0.9f), AzarRango67(minijuego, 0.2f, 0.45f) },
                    AzarRango67(minijuego, 1.1f, 1.7f),
                    AzarRango67(minijuego, 0.10f, 0.18f),
                    Color{ 214, 220, 228, 255 }
                );
            }
        }

        for (int b = 0; b < 2; b++)
        {
            PoseBrazo67 pose = CalcularPoseBrazo67(minijuego.tiempoAnimacion, FASE_BRAZOS_67[b]);

            if (!pose.soldando)
            {
                continue;
            }

            minijuego.acumuladorSoldadura += deltaTime * 16.0f * factor;

            while (minijuego.acumuladorSoldadura >= 1.0f)
            {
                minijuego.acumuladorSoldadura -= 1.0f;
                Vector3 punta = PuntaBrazo67(BASE_BRAZOS_67[b], pose);

                for (int equipo = 0; equipo < 2; equipo++)
                {
                    EmitirParticula67(
                        minijuego,
                        equipo,
                        PARTICULA_67_CHISPA,
                        punta,
                        { AzarRango67(minijuego, -1.2f, 1.2f), AzarRango67(minijuego, 0.4f, 1.8f), AzarRango67(minijuego, -0.6f, 1.2f) },
                        AzarRango67(minijuego, 0.25f, 0.5f),
                        0.045f,
                        Azar67(minijuego) < 0.5f ? Color{ 170, 220, 255, 255 } : DESTELLO_67
                    );
                }
            }
        }
    }

    // Lluvia de confeti sobre la vista del equipo ganador.
    if (minijuego.tiempoConfeti > 0.0f && minijuego.equipoGanador >= 0)
    {
        minijuego.tiempoConfeti -= deltaTime;

        if (Azar67(minijuego) < deltaTime * 40.0f)
        {
            Confeti67(
                minijuego,
                minijuego.equipoGanador,
                2,
                ObtenerColorEquipo67(minijuego.equipoGanador)
            );
        }
    }
}


//==================================================
// RESULTADO
//==================================================

static void FinalizarResultado67(
    Minijuego67& minijuego,
    int cantidadMaxima
)
{
    if (
        minijuego.resultado.estado != RESULTADO_MINIJUEGO_EN_CURSO ||
        !minijuego.partidaValida
    )
    {
        return;
    }

    int puntos0 = minijuego.equipos[0].puntos;
    int puntos1 = minijuego.equipos[1].puntos;

    minijuego.empate = puntos0 == puntos1;
    minijuego.equipoGanador = -1;

    if (!minijuego.empate)
    {
        minijuego.equipoGanador = puntos0 > puntos1 ? 0 : 1;
    }

    minijuego.resultado.estado = RESULTADO_MINIJUEGO_FINALIZADO;
    minijuego.resultado.desenlace =
        minijuego.empate
        ? DESENLACE_EMPATE
        : DESENLACE_CON_GANADOR;
    minijuego.resultado.cantidadEquipos = 2;

    for (int i = 0; i < cantidadMaxima; i++)
    {
        ResultadoParticipante& resultadoJugador =
            minijuego.resultado.participantes[i];

        if (!resultadoJugador.participo)
        {
            continue;
        }

        int equipo = minijuego.equipoPorJugador[i];
        resultadoJugador.numeroEquipo = equipo;
        resultadoJugador.puntuacionMinijuego =
            equipo >= 0 && equipo < 2
            ? minijuego.equipos[equipo].puntos
            : 0;
        resultadoJugador.puntosObtenidos = 0;

        if (minijuego.empate)
        {
            resultadoJugador.posicionFinal = 1;
        }
        else
        {
            resultadoJugador.posicionFinal =
                equipo == minijuego.equipoGanador ? 1 : 2;
        }
    }
}


//==================================================
// LOGICA DE PIEZAS
//==================================================

static void ColocarPieza67(
    Minijuego67& minijuego,
    int indiceJugador,
    int cantidadMaxima
)
{
    int equipo = minijuego.equipoPorJugador[indiceJugador];

    if (equipo < 0 || equipo > 1)
    {
        return;
    }

    EstadoJugador67& jugador =
        minijuego.estadosJugadores[indiceJugador];

    EstadoEquipo67& estadoEquipo =
        minijuego.equipos[equipo];

    bool correcto = false;

    if (
        jugador.tipoPieza == PIEZA_NUMERO_6 &&
        estadoEquipo.mesa == MESA_67_VACIA
    )
    {
        estadoEquipo.mesa = MESA_67_CON_6;
        correcto = true;
    }
    else if (
        jugador.tipoPieza == PIEZA_NUMERO_7 &&
        estadoEquipo.mesa == MESA_67_CON_6
    )
    {
        estadoEquipo.mesa = MESA_67_COMPLETA;
        estadoEquipo.tiempoMesaCompleta = DURACION_MESA_COMPLETA_67;
        estadoEquipo.puntos++;
        correcto = true;
    }

    estadoEquipo.ultimoAcierto = correcto;
    estadoEquipo.tiempoFeedback = DURACION_FEEDBACK_67;

    jugador.llevaPieza = false;
    jugador.mirandoMesa = false;
    jugador.tiempoGiro = 0.0f;
    jugador.tiempoCooldownInteraccion = COOLDOWN_TRAS_COLOCAR_67;

    if (!correcto)
    {
        jugador.tiempoStun = DURACION_STUN_67;
    }

    // Feedback: chispas en la cifra colocada; error con humo y sacudida.
    Vector3 mesa = { X_MESA_67, 1.35f, 0.05f };

    if (correcto && estadoEquipo.mesa == MESA_67_CON_6)
    {
        estadoEquipo.destello6 = 1.0f;
        Chispas67(minijuego, equipo, { X_MESA_67 - 0.32f, 1.35f, 0.1f }, 14, COLOR_6_67, 1.6f);
        ReproducirSonidoMinijuego(minijuego.audio, SONIDO_ACIERTO);
    }
    else if (correcto)
    {
        estadoEquipo.destello7 = 1.0f;
        Chispas67(minijuego, equipo, { X_MESA_67 + 0.32f, 1.35f, 0.1f }, 14, COLOR_7_67, 1.6f);
        Chispas67(minijuego, equipo, mesa, 22, Color{ 255, 214, 90, 255 }, 2.4f);
        PonerAviso67(estadoEquipo, AVISO_67_COMPLETO, { X_MESA_67, 2.3f, 0.0f });
        ReproducirSonidoMinijuego(minijuego.audio, SONIDO_RECOGER_NUCLEO_ESPECIAL);
    }
    else
    {
        estadoEquipo.sacudida = 1.0f;
        Humo67(minijuego, equipo, mesa, 12);
        Chispas67(minijuego, equipo, mesa, 6, Color{ 255, 110, 70, 255 }, 1.2f);
        PonerAviso67(estadoEquipo, AVISO_67_ORDEN, { X_MESA_67, 2.3f, 0.0f });
        ReproducirSonidoMinijuego(minijuego.audio, SONIDO_ERROR);
    }

    if (
        correcto &&
        minijuego.cantidadJugadoresEquipo[equipo] == 1
    )
    {
        ActualizarRolJugadorUnico67(
            minijuego,
            equipo,
            cantidadMaxima
        );
    }
}


// Devuelve si el bot "presiona" el boton principal este fotograma.
// Solo coloca cuando el orden de la mesa es correcto, asi un bot
// nunca castiga a su companero humano con un 67 mal armado.
static bool DecidirPulsacionBot67(
    Minijuego67& minijuego,
    int indiceJugador,
    int equipo,
    float deltaTime
)
{
    EstadoJugador67& jugador =
        minijuego.estadosJugadores[indiceJugador];

    const EstadoEquipo67& estadoEquipo =
        minijuego.equipos[equipo];

    if (jugador.llevaPieza)
    {
        jugador.botEvaluoPieza = false;

        bool colocacionCorrecta =
            (
                jugador.tipoPieza == PIEZA_NUMERO_6 &&
                estadoEquipo.mesa == MESA_67_VACIA
            ) ||
            (
                jugador.tipoPieza == PIEZA_NUMERO_7 &&
                estadoEquipo.mesa == MESA_67_CON_6
            );

        if (!jugador.mirandoMesa || !colocacionCorrecta)
        {
            jugador.tiempoDecisionBot =
                GetRandomValue(10, 28) / 100.0f;

            return false;
        }

        jugador.tiempoDecisionBot -= deltaTime;

        return jugador.tiempoDecisionBot <= 0.0f;
    }

    if (!HayPiezaAlAlcance67(estadoEquipo, jugador.tipoPieza))
    {
        jugador.botEvaluoPieza = false;
        return false;
    }

    if (!jugador.botEvaluoPieza)
    {
        jugador.botEvaluoPieza = true;
        jugador.botAgarraPieza =
            GetRandomValue(1, 100) <= PROBABILIDAD_AGARRE_BOT_67;
    }

    return jugador.botAgarraPieza;
}


//==================================================
// DIBUJO: HERRAMIENTAS
//==================================================

// Contexto de una vista (una mitad de pantalla).
struct Vista67
{
    const Minijuego67* minijuego = nullptr;
    int equipo = 0;
    float tiempo = 0.0f;
    Vector3 posicionCamara{};
    Vector3 direccionCamara{};
    Color colorEquipo{};
    NivelCalidadGrafica decoracion = CALIDAD_ALTA;
};


static float Hash67(int a, int b)
{
    unsigned int h = (unsigned int)a * 374761393u + (unsigned int)b * 668265263u;
    h = (h ^ (h >> 13)) * 1274126177u;
    h ^= h >> 16;
    return (float)(h & 0xFFFFu) / 65535.0f;
}


static Color Sombrear67(Color c, float f)
{
    return Color
    {
        (unsigned char)std::fmin(255.0f, (float)c.r * f),
        (unsigned char)std::fmin(255.0f, (float)c.g * f),
        (unsigned char)std::fmin(255.0f, (float)c.b * f),
        c.a
    };
}


static Color Mezclar67(Color a, Color b, float t)
{
    t = Clamp(t, 0.0f, 1.0f);

    return Color
    {
        (unsigned char)((float)a.r + ((float)b.r - (float)a.r) * t),
        (unsigned char)((float)a.g + ((float)b.g - (float)a.g) * t),
        (unsigned char)((float)a.b + ((float)b.b - (float)a.b) * t),
        a.a
    };
}


static Color ConAlfa67(Color c, float alfa)
{
    c.a = (unsigned char)(255.0f * Clamp(alfa, 0.0f, 1.0f));
    return c;
}


// Niebla de profundidad: lo lejano se funde con el aire de la nave.
static Color Niebla67(const Vista67& v, Color color, Vector3 punto)
{
    float profundidad =
        (punto.x - v.posicionCamara.x) * v.direccionCamara.x +
        (punto.y - v.posicionCamara.y) * v.direccionCamara.y +
        (punto.z - v.posicionCamara.z) * v.direccionCamara.z;

    float f = Clamp((profundidad - 15.5f) / 10.0f, 0.0f, 1.0f) * 0.62f;
    Color niebla = Mezclar67(NIEBLA_67, v.colorEquipo, 0.10f);
    return Mezclar67(color, niebla, f);
}


// Cubo con caras sombreadas; respeta la matriz actual (piezas que giran).
static void Cubo67(Vector3 c, Vector3 t, Color color)
{
    float x0 = c.x - t.x * 0.5f, x1 = c.x + t.x * 0.5f;
    float y0 = c.y - t.y * 0.5f, y1 = c.y + t.y * 0.5f;
    float z0 = c.z - t.z * 0.5f, z1 = c.z + t.z * 0.5f;

    Color arriba = color;
    Color frente = Sombrear67(color, 0.78f);
    Color derecha = Sombrear67(color, 0.62f);
    Color izquierda = Sombrear67(color, 0.70f);
    Color atras = Sombrear67(color, 0.58f);

    rlBegin(RL_TRIANGLES);

    rlColor4ub(arriba.r, arriba.g, arriba.b, arriba.a);
    rlVertex3f(x0, y1, z0); rlVertex3f(x0, y1, z1); rlVertex3f(x1, y1, z1);
    rlVertex3f(x1, y1, z0); rlVertex3f(x0, y1, z0); rlVertex3f(x1, y1, z1);

    rlColor4ub(frente.r, frente.g, frente.b, frente.a);
    rlVertex3f(x0, y0, z1); rlVertex3f(x1, y0, z1); rlVertex3f(x0, y1, z1);
    rlVertex3f(x1, y1, z1); rlVertex3f(x0, y1, z1); rlVertex3f(x1, y0, z1);

    rlColor4ub(derecha.r, derecha.g, derecha.b, derecha.a);
    rlVertex3f(x1, y0, z0); rlVertex3f(x1, y1, z0); rlVertex3f(x1, y1, z1);
    rlVertex3f(x1, y0, z1); rlVertex3f(x1, y0, z0); rlVertex3f(x1, y1, z1);

    rlColor4ub(izquierda.r, izquierda.g, izquierda.b, izquierda.a);
    rlVertex3f(x0, y0, z0); rlVertex3f(x0, y1, z1); rlVertex3f(x0, y1, z0);
    rlVertex3f(x0, y0, z1); rlVertex3f(x0, y1, z1); rlVertex3f(x0, y0, z0);

    rlColor4ub(atras.r, atras.g, atras.b, atras.a);
    rlVertex3f(x1, y0, z0); rlVertex3f(x0, y0, z0); rlVertex3f(x0, y1, z0);
    rlVertex3f(x1, y0, z0); rlVertex3f(x0, y1, z0); rlVertex3f(x1, y1, z0);

    rlEnd();
}


// Cubo con niebla segun su centro.
static void CuboN67(const Vista67& v, Vector3 c, Vector3 t, Color color)
{
    Cubo67(c, t, Niebla67(v, color, c));
}


// Tronco de piramide cuadrada (tolvas y pantallas de lampara).
static void Tronco67(
    Vector3 base,
    float medioAbajo,
    float medioArriba,
    float alto,
    Color color
)
{
    float x = base.x, z = base.z;
    float y0 = base.y, y1 = base.y + alto;
    float a = medioAbajo, b = medioArriba;

    Color frente = Sombrear67(color, 0.82f);
    Color derecha = Sombrear67(color, 0.64f);
    Color izquierda = Sombrear67(color, 0.72f);
    Color atras = Sombrear67(color, 0.58f);
    Color tapa = Sombrear67(color, 0.32f);

    rlBegin(RL_TRIANGLES);

    rlColor4ub(frente.r, frente.g, frente.b, frente.a);
    rlVertex3f(x - a, y0, z + a); rlVertex3f(x + a, y0, z + a); rlVertex3f(x + b, y1, z + b);
    rlVertex3f(x - a, y0, z + a); rlVertex3f(x + b, y1, z + b); rlVertex3f(x - b, y1, z + b);

    rlColor4ub(derecha.r, derecha.g, derecha.b, derecha.a);
    rlVertex3f(x + a, y0, z + a); rlVertex3f(x + a, y0, z - a); rlVertex3f(x + b, y1, z - b);
    rlVertex3f(x + a, y0, z + a); rlVertex3f(x + b, y1, z - b); rlVertex3f(x + b, y1, z + b);

    rlColor4ub(izquierda.r, izquierda.g, izquierda.b, izquierda.a);
    rlVertex3f(x - a, y0, z - a); rlVertex3f(x - a, y0, z + a); rlVertex3f(x - b, y1, z + b);
    rlVertex3f(x - a, y0, z - a); rlVertex3f(x - b, y1, z + b); rlVertex3f(x - b, y1, z - b);

    rlColor4ub(atras.r, atras.g, atras.b, atras.a);
    rlVertex3f(x + a, y0, z - a); rlVertex3f(x - a, y0, z - a); rlVertex3f(x - b, y1, z - b);
    rlVertex3f(x + a, y0, z - a); rlVertex3f(x - b, y1, z - b); rlVertex3f(x + b, y1, z - b);

    // Boca (interior oscuro), apenas por debajo del borde.
    float yb = y1 - 0.02f;
    rlColor4ub(tapa.r, tapa.g, tapa.b, tapa.a);
    rlVertex3f(x - b, yb, z + b); rlVertex3f(x + b, yb, z + b); rlVertex3f(x + b, yb, z - b);
    rlVertex3f(x - b, yb, z + b); rlVertex3f(x + b, yb, z - b); rlVertex3f(x - b, yb, z - b);

    rlEnd();
}


// Charco de luz en el suelo (solo dentro del pase aditivo).
static void Resplandor67(float x, float y, float z, float radio, Color color)
{
    for (int r = 0; r < 4; r++)
    {
        float radioCapa = radio * (1.0f - 0.2f * (float)r);
        (DrawCylinder)({ x, y + 0.002f * (float)r, z }, radioCapa, radioCapa, 0.002f, 24, color);
    }
}


//==================================================
// DIBUJO: CIFRAS DE SIETE SEGMENTOS
//==================================================

static int SegmentosNumero67(
    int numero,
    Vector3 centro,
    float escala,
    Vector3 posiciones[7],
    bool horizontales[7]
)
{
    const float xLado = 0.31f * escala;
    const float yExtremo = 0.55f * escala;
    const float yMedio = 0.275f * escala;

    int n = 0;

    auto agregar = [&](float dx, float dy, bool horizontal)
    {
        posiciones[n] = { centro.x + dx, centro.y + dy, centro.z };
        horizontales[n] = horizontal;
        n++;
    };

    agregar(0.0f, yExtremo, true);

    if (numero == 7)
    {
        agregar(xLado, yMedio, false);
        agregar(xLado, -yMedio, false);
    }
    else
    {
        agregar(xLado, -yMedio, false);
        agregar(0.0f, -yExtremo, true);
        agregar(-xLado, -yMedio, false);
        agregar(-xLado, yMedio, false);
        agregar(0.0f, 0.0f, true);
    }

    return n;
}


static Vector3 TamanoSegmento67(bool horizontal, float escala)
{
    return
    {
        (horizontal ? 0.62f : 0.13f) * escala,
        (horizontal ? 0.13f : 0.52f) * escala,
        0.20f * escala
    };
}


static void DibujarNumero3D67(
    int numero,
    Vector3 centro,
    float escala,
    Color color
)
{
    Vector3 posiciones[7];
    bool horizontales[7];
    int n = SegmentosNumero67(numero, centro, escala, posiciones, horizontales);

    for (int i = 0; i < n; i++)
    {
        Cubo67(posiciones[i], TamanoSegmento67(horizontales[i], escala), color);
    }
}


// Halo de neon alrededor de una cifra (solo dentro del pase aditivo).
static void DibujarHaloNumero67(
    int numero,
    Vector3 centro,
    float escala,
    Color color,
    float intensidad
)
{
    Vector3 posiciones[7];
    bool horizontales[7];
    int n = SegmentosNumero67(numero, centro, escala, posiciones, horizontales);

    for (int i = 0; i < n; i++)
    {
        Vector3 t = TamanoSegmento67(horizontales[i], escala);
        float borde = 0.16f * escala;
        (DrawCube)(posiciones[i], t.x + borde, t.y + borde, t.z + borde, Fade(color, 0.26f * intensidad));
        borde = 0.42f * escala;
        (DrawCube)(posiciones[i], t.x + borde, t.y + borde, t.z * 0.5f, Fade(color, 0.09f * intensidad));
    }
}


//==================================================
// DIBUJO: NAVE INDUSTRIAL
//==================================================

static const float Z_PARED_67 = -4.8f;
static const float X_PILARES_67[5] = { -12.0f, -7.6f, -3.2f, 1.2f, 5.6f };
static const float X_LAMPARAS_67[2] = { -3.6f, 0.3f };
static const float Y_LAMPARAS_67 = 5.1f;
static const Vector3 FOCOS_OBRA_67[2] = { { -0.8f, 0.0f, 5.4f }, { -3.3f, 0.0f, 5.0f } };


static void DibujarSuelo67(const Vista67& v)
{
    // Losa base: lo que asoma entre baldosas son las juntas.
    Cubo67({ -2.0f, -0.25f, 3.5f }, { 28.0f, 0.5f, 18.0f }, Niebla67(v, Color{ 36, 38, 44, 255 }, { -2.0f, 0.0f, 0.0f }));

    int paso = v.decoracion == CALIDAD_ALTA ? 1 : 2;
    float lado = 2.0f * (float)paso;

    // Baldosas de hormigon con variacion.
    for (int i = 0; i * lado < 26.0f; i++)
    {
        for (int j = 0; j * lado < 18.0f; j++)
        {
            float cx = -15.0f + lado * 0.5f + (float)i * lado;
            float cz = -4.8f + lado * 0.5f + (float)j * lado;
            float h = Hash67(i * 3 + 11, j * 7 + 5);
            Color base = Mezclar67(Color{ 66, 68, 74, 255 }, Color{ 82, 80, 76, 255 }, h);
            Cubo67({ cx, 0.0f, cz }, { lado - 0.07f, 0.02f, lado - 0.07f }, Niebla67(v, base, { cx, 0.0f, cz }));
        }
    }

    // Lineas de circulacion pintadas a los lados de cada cinta.
    Color linea = Niebla67(v, Color{ 196, 160, 56, 255 }, { -2.0f, 0.0f, 0.0f });
    for (int k = 0; k < 2; k++)
    {
        float zc = k == 0 ? Z_CINTA_6_67 : Z_CINTA_7_67;
        for (int lado2 = -1; lado2 <= 1; lado2 += 2)
        {
            Cubo67({ -1.8f, 0.012f, zc + (float)lado2 * 0.95f }, { 10.6f, 0.006f, 0.07f }, linea);
        }
    }

    // Pasarela de chapa del puesto de trabajo (jugadores y mesa).
    Color chapa = Color{ 96, 102, 114, 255 };
    Cubo67({ 2.25f, 0.02f, 0.0f }, { 4.7f, 0.04f, 2.9f }, chapa);

    if (v.decoracion != CALIDAD_BAJA)
    {
        // Relieve de chapa semillada.
        Color relieve = Sombrear67(chapa, 1.14f);
        for (int i = 0; i < 14; i += paso)
        {
            for (int j = 0; j < 9; j += paso)
            {
                bool girado = ((i + j) & 1) != 0;
                Cubo67(
                    { -0.0f + 0.16f + (float)i * 0.33f, 0.045f, -1.3f + (float)j * 0.32f },
                    { girado ? 0.04f : 0.13f, 0.01f, girado ? 0.13f : 0.04f },
                    relieve
                );
            }
        }
    }

    // Franjas de seguridad amarillas y negras en los bordes de la pasarela.
    for (int i = 0; i < 16; i++)
    {
        Color c = (i & 1) ? Color{ 34, 34, 38, 255 } : AMARILLO_67;
        for (int lado2 = -1; lado2 <= 1; lado2 += 2)
        {
            Cubo67({ -0.05f + 0.15f + (float)i * 0.3f, 0.046f, (float)lado2 * 1.40f }, { 0.3f, 0.012f, 0.1f }, c);
        }
    }

    if (v.decoracion == CALIDAD_BAJA)
    {
        return;
    }

    // Manchas de aceite y una rejilla de desague.
    for (int i = 0; i < 9; i++)
    {
        float x = -9.0f + Hash67(i, 3) * 15.0f;
        float z = -3.6f + Hash67(i, 9) * 12.0f;

        if (z > -1.6f && z < 1.6f && x > -0.4f)
        {
            continue;
        }

        float r = 0.25f + Hash67(i, 17) * 0.45f;
        (DrawCylinder)({ x, 0.013f, z }, r, r * 0.9f, 0.004f, 12, Fade(Color{ 14, 14, 18, 255 }, 0.45f));
    }

    Cubo67({ -1.8f, 0.014f, 0.0f }, { 1.4f, 0.01f, 0.7f }, Niebla67(v, Color{ 40, 42, 48, 255 }, { -1.8f, 0.0f, 0.0f }));
    for (int i = 0; i < 6; i++)
    {
        Cubo67({ -2.4f + (float)i * 0.24f, 0.022f, 0.0f }, { 0.06f, 0.01f, 0.62f }, Niebla67(v, Color{ 88, 92, 100, 255 }, { -1.8f, 0.0f, 0.0f }));
    }
}


static void DibujarParedFondo67(const Vista67& v)
{
    // Zocalo de hormigon.
    CuboN67(v, { -4.5f, 0.6f, Z_PARED_67 }, { 22.0f, 1.2f, 0.3f }, Color{ 74, 76, 82, 255 });

    // Franja del equipo sobre el zocalo.
    CuboN67(v, { -4.5f, 1.27f, Z_PARED_67 + 0.12f }, { 22.0f, 0.14f, 0.06f }, Mezclar67(v.colorEquipo, Color{ 70, 70, 76, 255 }, 0.3f));

    // Chapa acanalada.
    int paso = v.decoracion == CALIDAD_ALTA ? 1 : 2;
    float anchoPanel = 0.5f * (float)paso;

    for (int i = 0; i * anchoPanel < 22.0f; i++)
    {
        float x = -15.5f + anchoPanel * 0.5f + (float)i * anchoPanel;
        float h = Hash67(i, 41) * 0.08f;
        Color c = (i & 1) ? Color{ 64, 72, 86, 255 } : Color{ 56, 63, 76, 255 };
        CuboN67(v, { x, 3.95f, Z_PARED_67 - ((i & 1) ? 0.0f : 0.05f) }, { anchoPanel, 5.3f, 0.3f }, Sombrear67(c, 0.96f + h));
    }

    // Pilares de acero con base pintada.
    for (int i = 0; i < 5; i++)
    {
        float x = X_PILARES_67[i];
        CuboN67(v, { x, 3.3f, Z_PARED_67 + 0.3f }, { 0.55f, 6.6f, 0.36f }, Color{ 92, 98, 110, 255 });
        CuboN67(v, { x, 3.3f, Z_PARED_67 + 0.5f }, { 0.18f, 6.6f, 0.06f }, Color{ 78, 84, 96, 255 });

        for (int s = 0; s < 4; s++)
        {
            Color c = (s & 1) ? Color{ 34, 34, 38, 255 } : AMARILLO_67;
            CuboN67(v, { x, 0.12f + (float)s * 0.22f, Z_PARED_67 + 0.5f }, { 0.58f, 0.22f, 0.04f }, c);
        }
    }

    // Carteles de neon "6" y "7": se encienden cuando esa cifra esta en
    // la mesa del equipo (lo mismo que ya muestra la mesa).
    Vector3 cartel = { -1.4f, 2.2f, Z_PARED_67 + 0.22f };
    CuboN67(v, cartel, { 2.5f, 1.6f, 0.12f }, Color{ 22, 24, 30, 255 });
    CuboN67(v, { cartel.x, cartel.y + 0.83f, cartel.z + 0.02f }, { 2.6f, 0.08f, 0.16f }, v.colorEquipo);
    CuboN67(v, { cartel.x, cartel.y - 0.83f, cartel.z + 0.02f }, { 2.6f, 0.08f, 0.16f }, v.colorEquipo);

    const EstadoEquipo67& eq = v.minijuego->equipos[v.equipo];
    bool hay6 = eq.mesa != MESA_67_VACIA;
    bool hay7 = eq.mesa == MESA_67_COMPLETA;
    float parpadeo = 0.85f + 0.15f * std::sin(v.tiempo * 23.0f) * std::sin(v.tiempo * 7.0f);

    DibujarNumero3D67(
        6,
        { cartel.x - 0.55f, cartel.y, cartel.z + 0.1f },
        0.95f,
        hay6 ? Mezclar67(COLOR_6_67, DESTELLO_67, eq.destello6 * 0.6f) : Sombrear67(COLOR_6_67, 0.32f * parpadeo)
    );
    DibujarNumero3D67(
        7,
        { cartel.x + 0.55f, cartel.y, cartel.z + 0.1f },
        0.95f,
        hay7 ? Mezclar67(COLOR_7_67, DESTELLO_67, eq.destello7 * 0.6f) : Sombrear67(COLOR_7_67, 0.32f * parpadeo)
    );

    // Tuberias con soportes, codos y valvulas.
    Color cobre = Niebla67(v, Color{ 168, 104, 66, 255 }, { -4.0f, 3.0f, Z_PARED_67 });
    Color gris = Niebla67(v, Color{ 108, 120, 130, 255 }, { -4.0f, 3.0f, Z_PARED_67 });
    (DrawCylinderEx)({ -15.5f, 3.05f, -4.45f }, { 6.0f, 3.05f, -4.45f }, 0.14f, 0.14f, 10, cobre);
    (DrawCylinderEx)({ -15.5f, 3.45f, -4.5f }, { 6.0f, 3.45f, -4.5f }, 0.10f, 0.10f, 8, gris);
    (DrawCylinderEx)({ -8.2f, 3.05f, -4.45f }, { -8.2f, 1.25f, -4.45f }, 0.12f, 0.12f, 10, cobre);
    (DrawCylinderEx)({ 3.4f, 3.45f, -4.5f }, { 3.4f, 0.0f, -4.5f }, 0.09f, 0.09f, 8, gris);

    if (v.decoracion != CALIDAD_BAJA)
    {
        for (int i = 0; i < 11; i++)
        {
            float x = -14.0f + (float)i * 2.0f;
            CuboN67(v, { x, 3.25f, -4.55f }, { 0.12f, 0.62f, 0.2f }, Color{ 60, 64, 72, 255 });
        }

        for (int k = 0; k < 2; k++)
        {
            Vector3 p = VALVULAS_67[k];
            Color rojo = Niebla67(v, Color{ 200, 62, 52, 255 }, p);
            (DrawCylinderEx)({ p.x, p.y, p.z - 0.05f }, { p.x, p.y, p.z + 0.12f }, 0.06f, 0.06f, 8, gris);
            (DrawCylinderEx)({ p.x, p.y, p.z + 0.12f }, { p.x, p.y, p.z + 0.16f }, 0.2f, 0.2f, 10, rojo);
            // Manometro.
            (DrawCylinderEx)({ p.x + 0.45f, p.y - 0.1f, p.z }, { p.x + 0.45f, p.y - 0.1f, p.z + 0.1f }, 0.13f, 0.13f, 12, Niebla67(v, Color{ 226, 226, 220, 255 }, p));
            float aguja = 0.6f + 0.25f * std::sin(v.tiempo * 1.7f + (float)k * 2.0f);
            Cubo67({ p.x + 0.45f + std::cos(aguja) * 0.05f, p.y - 0.1f + std::sin(aguja) * 0.05f, p.z + 0.11f }, { 0.1f, 0.02f, 0.01f }, Color{ 180, 40, 36, 255 });
        }

        // Porton enrollable a la izquierda.
        Vector3 porton = { -10.2f, 1.8f, Z_PARED_67 + 0.18f };
        CuboN67(v, porton, { 3.4f, 3.6f, 0.06f }, Color{ 96, 100, 104, 255 });
        for (int i = 0; i < 14; i++)
        {
            CuboN67(v, { porton.x, 0.12f + (float)i * 0.25f, porton.z + 0.04f }, { 3.3f, 0.04f, 0.04f }, Color{ 70, 74, 80, 255 });
        }
        CuboN67(v, { porton.x, 3.7f, porton.z + 0.1f }, { 3.7f, 0.3f, 0.28f }, Color{ 60, 64, 72, 255 });
        for (int s = 0; s < 8; s++)
        {
            Color c = (s & 1) ? Color{ 34, 34, 38, 255 } : AMARILLO_67;
            CuboN67(v, { porton.x - 1.78f, 0.22f + (float)s * 0.45f, porton.z + 0.06f }, { 0.12f, 0.45f, 0.1f }, c);
            CuboN67(v, { porton.x + 1.78f, 0.22f + (float)s * 0.45f, porton.z + 0.06f }, { 0.12f, 0.45f, 0.1f }, c);
        }

        // Tablero electrico con leds que titilan.
        Vector3 tablero = { -6.0f, 1.75f, Z_PARED_67 + 0.25f };
        CuboN67(v, tablero, { 0.9f, 1.1f, 0.2f }, Color{ 150, 156, 150, 255 });
        CuboN67(v, { tablero.x, tablero.y + 0.2f, tablero.z + 0.11f }, { 0.4f, 0.3f, 0.02f }, Color{ 230, 196, 56, 255 });
        for (int i = 0; i < 4; i++)
        {
            bool prendido = std::sin(v.tiempo * (2.0f + (float)i * 1.3f) + (float)i) > 0.0f;
            Color led = i == 0 ? Color{ 110, 230, 120, 255 } : (i == 3 ? Color{ 255, 90, 70, 255 } : Color{ 255, 200, 80, 255 });
            Cubo67({ tablero.x - 0.24f + (float)i * 0.16f, tablero.y - 0.25f, tablero.z + 0.11f }, { 0.07f, 0.07f, 0.03f }, prendido ? led : Sombrear67(led, 0.3f));
        }

        // Matafuego en un pilar.
        Vector3 matafuego = { X_PILARES_67[2] + 0.42f, 0.0f, Z_PARED_67 + 0.6f };
        (DrawCylinder)({ matafuego.x, 0.95f, matafuego.z }, 0.11f, 0.11f, 0.55f, 10, Niebla67(v, Color{ 196, 46, 40, 255 }, matafuego));
        CuboN67(v, { matafuego.x, 1.55f, matafuego.z }, { 0.08f, 0.1f, 0.08f }, Color{ 40, 40, 44, 255 });
    }
}


static void DibujarBrazo67(const Vista67& v, int indice)
{
    Vector3 base = BASE_BRAZOS_67[indice];
    PoseBrazo67 pose = CalcularPoseBrazo67(v.tiempo, FASE_BRAZOS_67[indice]);

    Color pintura = Niebla67(v, Color{ 228, 150, 46, 255 }, base);
    Color oscuro = Niebla67(v, Color{ 58, 60, 70, 255 }, base);

    // Banco con una pieza en bruto.
    CuboN67(v, { base.x - 0.35f, 0.42f, base.z + 0.1f }, { 1.1f, 0.84f, 0.7f }, Color{ 74, 80, 92, 255 });
    CuboN67(v, { base.x - 0.35f, 0.9f, base.z + 0.1f }, { 0.6f, 0.12f, 0.36f }, Color{ 150, 156, 166, 255 });

    // Base y torreta.
    Cubo67({ base.x, 0.08f, base.z }, { 0.8f, 0.16f, 0.8f }, oscuro);
    (DrawCylinder)({ base.x, 0.16f, base.z }, 0.30f, 0.34f, 0.26f, 12, pintura);

    rlPushMatrix();
    rlTranslatef(base.x, base.y + ALTO_BASE_BRAZO_67, base.z);
    rlRotatef(pose.giro, 0.0f, 1.0f, 0.0f);
    Cubo67({ 0.0f, 0.02f, 0.0f }, { 0.5f, 0.2f, 0.46f }, pintura);

    rlRotatef(pose.hombro, 0.0f, 0.0f, 1.0f);
    Cubo67({ 0.0f, LARGO_BRAZO_67 * 0.5f, 0.0f }, { 0.2f, LARGO_BRAZO_67, 0.22f }, pintura);
    Cubo67({ 0.0f, LARGO_BRAZO_67 * 0.5f, 0.13f }, { 0.07f, LARGO_BRAZO_67 * 0.8f, 0.04f }, oscuro);

    rlTranslatef(0.0f, LARGO_BRAZO_67, 0.0f);
    Cubo67({ 0.0f, 0.0f, 0.0f }, { 0.27f, 0.27f, 0.3f }, oscuro);

    rlRotatef(pose.codo, 0.0f, 0.0f, 1.0f);
    Cubo67({ 0.0f, LARGO_ANTEBRAZO_67 * 0.5f, 0.0f }, { 0.16f, LARGO_ANTEBRAZO_67, 0.18f }, pintura);

    rlTranslatef(0.0f, LARGO_ANTEBRAZO_67, 0.0f);
    Cubo67({ 0.0f, 0.03f, 0.0f }, { 0.24f, 0.1f, 0.24f }, oscuro);
    Cubo67({ 0.0f, LARGO_HERRAMIENTA_67 * 0.5f, 0.0f }, { 0.08f, LARGO_HERRAMIENTA_67, 0.08f }, Color{ 150, 154, 164, 255 });
    rlPopMatrix();
}


static void DibujarLamparaColgante67(const Vista67& v, Vector3 foco)
{
    Cubo67({ foco.x, foco.y + 1.7f, foco.z }, { 0.03f, 3.0f, 0.03f }, Niebla67(v, Color{ 26, 28, 32, 255 }, foco));
    Tronco67({ foco.x, foco.y + 0.02f, foco.z }, 0.44f, 0.14f, 0.32f, Niebla67(v, Color{ 70, 92, 84, 255 }, foco));
    (DrawSphere)({ foco.x, foco.y, foco.z }, 0.13f, Color{ 255, 238, 196, 255 });
}


static void DibujarFocoObra67(const Vista67& v, Vector3 base)
{
    Color patas = Niebla67(v, Color{ 46, 48, 54, 255 }, base);

    for (int i = 0; i < 3; i++)
    {
        float a = (float)i * 2.094f + 0.5f;
        (DrawCylinderEx)({ base.x + std::cos(a) * 0.42f, 0.0f, base.z + std::sin(a) * 0.42f }, { base.x, 1.0f, base.z }, 0.03f, 0.03f, 5, patas);
    }

    (DrawCylinderEx)({ base.x, 1.0f, base.z }, { base.x, 1.4f, base.z }, 0.045f, 0.045f, 6, patas);

    rlPushMatrix();
    rlTranslatef(base.x, 1.5f, base.z);
    rlRotatef(140.0f, 0.0f, 1.0f, 0.0f);
    rlRotatef(-18.0f, 1.0f, 0.0f, 0.0f);
    Cubo67({ 0.0f, 0.0f, 0.0f }, { 0.56f, 0.4f, 0.22f }, Niebla67(v, Color{ 226, 172, 46, 255 }, base));
    Cubo67({ 0.0f, 0.0f, 0.12f }, { 0.46f, 0.3f, 0.03f }, Color{ 255, 246, 214, 255 });
    rlPopMatrix();

    // Cable por el suelo.
    Cubo67({ base.x - 0.9f, 0.02f, base.z + 0.3f }, { 1.8f, 0.02f, 0.03f }, Color{ 24, 24, 28, 255 });
}


static void DibujarEstante67(const Vista67& v, Vector3 base, float ancho, int niveles, int semilla)
{
    const float alto = 0.72f;
    const float fondo = 0.8f;
    Color montante = Niebla67(v, Color{ 52, 92, 150, 255 }, base);
    Color larguero = Niebla67(v, Color{ 222, 120, 44, 255 }, base);
    Color tabla = Niebla67(v, Color{ 132, 112, 88, 255 }, base);

    for (int sx = -1; sx <= 1; sx += 2)
    {
        for (int sz = -1; sz <= 1; sz += 2)
        {
            Cubo67({ base.x + (float)sx * ancho * 0.5f, alto * (float)niveles * 0.5f + 0.05f, base.z + (float)sz * fondo * 0.5f }, { 0.08f, alto * (float)niveles + 0.1f, 0.08f }, montante);
        }
    }

    static const Color CARGAS[4] =
    {
        { 176, 132, 82, 255 },
        { 150, 62, 52, 255 },
        { 64, 120, 160, 255 },
        { 110, 150, 84, 255 }
    };

    for (int nivel = 0; nivel < niveles; nivel++)
    {
        float y = 0.12f + (float)nivel * alto;
        Cubo67({ base.x, y, base.z + fondo * 0.5f }, { ancho, 0.09f, 0.07f }, larguero);
        Cubo67({ base.x, y, base.z - fondo * 0.5f }, { ancho, 0.09f, 0.07f }, larguero);
        Cubo67({ base.x, y + 0.06f, base.z }, { ancho - 0.05f, 0.03f, fondo }, tabla);

        int cantidad = 3;
        for (int k = 0; k < cantidad; k++)
        {
            float h = Hash67(semilla + nivel * 7, k);
            float x = base.x - ancho * 0.5f + ancho * ((float)k + 0.5f) / (float)cantidad;
            Color c = Niebla67(v, CARGAS[(int)(h * 4.0f) & 3], base);

            if (h < 0.35f)
            {
                (DrawCylinder)({ x, y + 0.08f, base.z }, 0.22f, 0.22f, 0.5f, 10, c);
                (DrawCylinder)({ x, y + 0.36f, base.z }, 0.23f, 0.23f, 0.04f, 10, Sombrear67(c, 0.7f));
            }
            else
            {
                float alto2 = 0.3f + h * 0.25f;
                Cubo67({ x, y + 0.08f + alto2 * 0.5f, base.z }, { 0.55f, alto2, 0.55f }, c);
                Cubo67({ x, y + 0.08f + alto2 * 0.5f, base.z + 0.28f }, { 0.55f, 0.06f, 0.01f }, Sombrear67(c, 0.6f));
            }
        }
    }
}


static void DibujarUtileria67(const Vista67& v)
{
    DibujarBrazo67(v, 0);
    DibujarBrazo67(v, 1);

    for (int i = 0; i < 2; i++)
    {
        DibujarLamparaColgante67(v, { X_LAMPARAS_67[i], Y_LAMPARAS_67, Z_CINTA_6_67 });
    }

    if (v.decoracion == CALIDAD_BAJA)
    {
        return;
    }

    for (int i = 0; i < 2; i++)
    {
        DibujarFocoObra67(v, FOCOS_OBRA_67[i]);
    }

    // Estante y tambores en primer plano (abajo a la izquierda).
    DibujarEstante67(v, { -3.9f, 0.0f, 6.7f }, 2.4f, 2, 3);

    Color tambor = Color{ 46, 104, 150, 255 };
    Cubo67({ -0.2f, 0.07f, 7.6f }, { 1.5f, 0.14f, 1.1f }, Color{ 140, 104, 66, 255 });
    (DrawCylinder)({ -0.55f, 0.14f, 7.4f }, 0.32f, 0.32f, 0.85f, 12, tambor);
    (DrawCylinder)({ 0.15f, 0.14f, 7.45f }, 0.32f, 0.32f, 0.85f, 12, Color{ 176, 64, 48, 255 });
    (DrawCylinder)({ -0.2f, 0.14f, 7.95f }, 0.32f, 0.32f, 0.85f, 12, tambor);

    // Palet con cajas de 67 terminados.
    Cubo67({ 2.4f, 0.07f, 6.0f }, { 1.6f, 0.14f, 1.2f }, Color{ 140, 104, 66, 255 });
    for (int k = 0; k < 3; k++)
    {
        float x = 2.0f + (float)(k % 2) * 0.8f;
        float y = 0.14f + 0.25f + (float)(k / 2) * 0.5f;
        Cubo67({ x, y, 6.0f }, { 0.74f, 0.5f, 1.0f }, Color{ 176, 134, 86, 255 });
        Cubo67({ x, y, 6.51f }, { 0.5f, 0.06f, 0.01f }, Color{ 120, 84, 52, 255 });
    }
    DibujarNumero3D67(6, { 1.84f, 1.34f, 6.05f }, 0.3f, COLOR_6_67);
    DibujarNumero3D67(7, { 2.16f, 1.34f, 6.05f }, 0.3f, COLOR_7_67);

    // Bolardos de seguridad delante de la cinta del 7.
    for (int i = 0; i < 3; i++)
    {
        float x = -0.4f + (float)i * 1.6f;
        (DrawCylinder)({ x, 0.0f, 4.3f }, 0.1f, 0.1f, 0.7f, 8, AMARILLO_67);
        (DrawCylinder)({ x, 0.42f, 4.3f }, 0.105f, 0.105f, 0.1f, 8, Color{ 30, 30, 34, 255 });
    }
}


//==================================================
// DIBUJO: CINTAS, PIEZAS Y MESA
//==================================================

static void DibujarCinta67(
    const Vista67& v,
    float z,
    float progresoBase,
    int numero
)
{
    float centroX = X_INICIO_CINTA_67 + LARGO_CINTA_67 * 0.5f;
    float xFinal = X_INICIO_CINTA_67 + LARGO_CINTA_67;
    Vector3 centro = { centroX, 0.5f, z };

    Color acero = Niebla67(v, ACERO_67, centro);
    Color oscuro = Niebla67(v, ACERO_OSCURO_67, centro);
    Color goma = Niebla67(v, Color{ 36, 38, 44, 255 }, centro);
    Color listones = Niebla67(v, Color{ 72, 76, 86, 255 }, centro);
    Color rodillo = Niebla67(v, Color{ 132, 138, 150, 255 }, centro);
    Color colorPieza = numero == 6 ? COLOR_6_67 : COLOR_7_67;

    // Patas, travesanos y bandeja inferior.
    for (float x = X_INICIO_CINTA_67 + 0.3f; x <= xFinal - 0.1f; x += 1.4f)
    {
        for (int lado = -1; lado <= 1; lado += 2)
        {
            Cubo67({ x, 0.3f, z + (float)lado * 0.5f }, { 0.12f, 0.6f, 0.12f }, oscuro);
        }

        Cubo67({ x, 0.14f, z }, { 0.08f, 0.08f, 0.96f }, oscuro);
    }

    Cubo67({ centroX, 0.3f, z }, { LARGO_CINTA_67 + 0.3f, 0.04f, 0.96f }, oscuro);

    // Cama de goma y listones que avanzan al ritmo de las piezas.
    Cubo67({ centroX, Y_CINTA_67 - 0.05f, z }, { LARGO_CINTA_67 + 0.2f, 0.1f, 1.02f }, goma);

    float desplazamiento = progresoBase * LARGO_CINTA_67;
    const float separacion = 0.45f;
    int cantidad = (int)(LARGO_CINTA_67 / separacion);

    for (int i = 0; i < cantidad; i++)
    {
        float x = X_INICIO_CINTA_67 + std::fmod(desplazamiento + (float)i * separacion, LARGO_CINTA_67);
        Cubo67({ x, Y_CINTA_67 + 0.004f, z }, { 0.06f, 0.012f, 0.98f }, listones);
    }

    // Barandas laterales.
    for (int lado = -1; lado <= 1; lado += 2)
    {
        Cubo67({ centroX, Y_CINTA_67 - 0.01f, z + (float)lado * 0.58f }, { LARGO_CINTA_67 + 0.5f, 0.18f, 0.1f }, acero);
    }

    // Franja de recogida: mismo tramo de siempre, pintado en la cinta y
    // con bloques de peligro en las barandas.
    float xInicio = ObtenerXObjeto67(INICIO_ZONA_RECOGIDA_67);
    float xFin = ObtenerXObjeto67(FIN_ZONA_RECOGIDA_67);
    float anchoZona = xFin - xInicio;

    for (int lado = -1; lado <= 1; lado += 2)
    {
        for (int s = 0; s < 4; s++)
        {
            Color c = (s & 1) ? Color{ 30, 30, 34, 255 } : AMARILLO_67;
            Cubo67({ xInicio + anchoZona * ((float)s + 0.5f) / 4.0f, Y_CINTA_67 + 0.085f, z + (float)lado * 0.58f }, { anchoZona / 4.0f, 0.02f, 0.11f }, c);
        }
    }

    (DrawCube)({ (xInicio + xFin) * 0.5f, Y_CINTA_67 + 0.025f, z }, anchoZona, 0.02f, 1.04f, Fade(AMARILLO_67, 0.5f));

    // Rodillos de los extremos y del costado (giran con la cinta).
    float giro = -desplazamiento / 0.14f;

    for (int extremo = 0; extremo < 2; extremo++)
    {
        float x = extremo == 0 ? X_INICIO_CINTA_67 - 0.06f : xFinal + 0.06f;
        float y = Y_CINTA_67 - 0.08f;
        (DrawCylinderEx)({ x, y, z - 0.56f }, { x, y, z + 0.56f }, 0.14f, 0.14f, 12, rodillo);
        Cubo67({ x + std::cos(giro) * 0.09f, y + std::sin(giro) * 0.09f, z + 0.57f }, { 0.05f, 0.05f, 0.02f }, AMARILLO_67);
    }

    if (v.decoracion != CALIDAD_BAJA)
    {
        for (float x = X_INICIO_CINTA_67 + 0.65f; x < xFinal - 0.2f; x += 0.7f)
        {
            float y = 0.5f;
            (DrawCylinderEx)({ x, y, z + 0.5f }, { x, y, z + 0.6f }, 0.08f, 0.08f, 10, rodillo);
            rlPushMatrix();
            rlTranslatef(x, y, z + 0.605f);
            rlRotatef(giro * RAD2DEG * 1.75f, 0.0f, 0.0f, 1.0f);
            Cubo67({ 0.0f, 0.0f, 0.0f }, { 0.14f, 0.03f, 0.01f }, oscuro);
            rlPopMatrix();
        }
    }

    // Tolva de entrada: de aca salen las piezas.
    float xTolva = X_INICIO_CINTA_67 - 0.62f;
    Vector3 tolva = { xTolva, 0.75f, z };
    CuboN67(v, tolva, { 1.0f, 1.5f, 1.4f }, Color{ 72, 78, 92, 255 });
    CuboN67(v, { xTolva, 1.3f, z + 0.71f }, { 1.0f, 0.12f, 0.03f }, v.colorEquipo);
    Cubo67({ xTolva + 0.5f, Y_CINTA_67 + 0.3f, z }, { 0.03f, 0.5f, 0.9f }, Color{ 16, 18, 22, 255 });
    Tronco67({ xTolva, 1.5f, z }, 0.5f, 0.85f, 0.95f, Niebla67(v, Color{ 100, 106, 120, 255 }, tolva));

    // Cifras amontonadas asomando por la boca.
    for (int k = 0; k < 3; k++)
    {
        rlPushMatrix();
        rlTranslatef(xTolva - 0.3f + (float)k * 0.3f, 2.38f, z - 0.15f + (float)(k % 2) * 0.3f);
        rlRotatef(-25.0f + (float)k * 22.0f, 0.0f, 0.0f, 1.0f);
        DibujarNumero3D67(numero, { 0.0f, 0.0f, 0.0f }, 0.34f, Niebla67(v, Sombrear67(colorPieza, 0.8f), tolva));
        rlPopMatrix();
    }

    // Cartel con la cifra de la cinta en el frente de la tolva.
    Cubo67({ xTolva, 0.68f, z + 0.72f }, { 0.66f, 0.8f, 0.03f }, Color{ 20, 22, 28, 255 });
    DibujarNumero3D67(numero, { xTolva, 0.68f, z + 0.76f }, 0.5f, colorPieza);

    // Baliza.
    Cubo67({ xTolva + 0.32f, 2.5f, z - 0.55f }, { 0.12f, 0.12f, 0.12f }, Color{ 40, 40, 46, 255 });
    (DrawSphere)({ xTolva + 0.32f, 2.62f, z - 0.55f }, 0.09f, Color{ 255, 170, 60, 255 });

    // Contenedor de descarte al final.
    float xCaja = xFinal + 0.5f;
    Color verde = Niebla67(v, Color{ 58, 104, 76, 255 }, { xCaja, 0.3f, z });
    Cubo67({ xCaja, 0.06f, z }, { 0.8f, 0.04f, 1.1f }, Color{ 22, 26, 24, 255 });
    Cubo67({ xCaja, 0.3f, z + 0.55f }, { 0.8f, 0.6f, 0.06f }, verde);
    Cubo67({ xCaja, 0.3f, z - 0.55f }, { 0.8f, 0.6f, 0.06f }, verde);
    Cubo67({ xCaja + 0.4f, 0.3f, z }, { 0.06f, 0.6f, 1.16f }, verde);
    Cubo67({ xCaja - 0.4f, 0.3f, z }, { 0.06f, 0.6f, 1.16f }, verde);
    Cubo67({ xCaja, 0.12f, z - 0.15f }, { 0.4f, 0.1f, 0.3f }, Sombrear67(colorPieza, 0.55f));
    Cubo67({ xCaja + 0.1f, 0.12f, z + 0.25f }, { 0.3f, 0.1f, 0.3f }, Sombrear67(colorPieza, 0.45f));
}


static void DibujarPieza67(int numero, float progreso, float z)
{
    float x = ObtenerXObjeto67(progreso);
    float yBase = Y_CINTA_67 + 0.01f;
    float escala = 0.56f;

    // Sale de la tolva creciendo y al final cae al contenedor. Solo cambia
    // fuera de la franja de recogida: el momento de agarrar no se altera.
    if (progreso < 0.05f)
    {
        escala *= 0.55f + 0.45f * (progreso / 0.05f);
    }

    if (progreso > 0.96f)
    {
        float caida = (progreso - 0.96f) / 0.04f;
        x += caida * 0.5f;
        yBase -= caida * caida * 0.62f;
    }

    Color color = numero == 6 ? COLOR_6_67 : COLOR_7_67;

    Cubo67({ x, yBase + 0.02f, z }, { 0.6f * escala / 0.56f, 0.04f, 0.42f }, Color{ 46, 48, 56, 255 });
    DibujarNumero3D67(numero, { x, yBase + 0.04f + 0.615f * escala, z }, escala, color);
}


static void DibujarFlujoPiezas67(const EstadoEquipo67& equipo)
{
    for (int i = 0; i < CANTIDAD_PIEZAS_CINTA_67; i++)
    {
        DibujarPieza67(6, ObtenerProgresoPieza67(equipo.progresoCinta6, i), Z_CINTA_6_67);
        DibujarPieza67(7, ObtenerProgresoPieza67(equipo.progresoCinta7, i), Z_CINTA_7_67);
    }
}


static Vector3 PosicionCifraMesa67(int numero)
{
    return { X_MESA_67 + (numero == 6 ? -0.32f : 0.32f), 1.33f, 0.04f };
}


static void DibujarMesa67(const Vista67& v, const EstadoEquipo67& equipo)
{
    Color patas = ACERO_OSCURO_67;
    Color tapa = Color{ 108, 114, 126, 255 };

    for (int sx = -1; sx <= 1; sx += 2)
    {
        for (int sz = -1; sz <= 1; sz += 2)
        {
            Cubo67({ X_MESA_67 + (float)sx * 0.68f, 0.42f, (float)sz * 0.82f }, { 0.12f, 0.84f, 0.12f }, patas);
        }
    }

    Cubo67({ X_MESA_67, 0.24f, 0.0f }, { 1.4f, 0.05f, 1.7f }, patas);
    Cubo67({ X_MESA_67 - 0.3f, 0.39f, 0.35f }, { 0.5f, 0.24f, 0.3f }, Color{ 190, 62, 50, 255 });
    Cubo67({ X_MESA_67 + 0.3f, 0.36f, -0.2f }, { 0.36f, 0.18f, 0.36f }, Color{ 150, 120, 80, 255 });

    Cubo67({ X_MESA_67, 0.89f, 0.0f }, { 1.6f, 0.1f, 2.0f }, tapa);
    Cubo67({ X_MESA_67, 0.89f, 1.02f }, { 1.62f, 0.1f, 0.04f }, v.colorEquipo);
    Cubo67({ X_MESA_67 + 0.82f, 0.89f, 0.0f }, { 0.04f, 0.1f, 2.04f }, v.colorEquipo);

    // Molde y respaldo con marco del equipo.
    Cubo67({ X_MESA_67, 0.97f, 0.05f }, { 1.3f, 0.06f, 0.6f }, Color{ 38, 42, 50, 255 });
    Cubo67({ X_MESA_67, 1.45f, -0.36f }, { 1.45f, 1.0f, 0.08f }, Color{ 30, 32, 40, 255 });
    Cubo67({ X_MESA_67, 1.98f, -0.36f }, { 1.55f, 0.07f, 0.12f }, v.colorEquipo);
    Cubo67({ X_MESA_67 - 0.76f, 1.45f, -0.36f }, { 0.07f, 1.1f, 0.12f }, v.colorEquipo);
    Cubo67({ X_MESA_67 + 0.76f, 1.45f, -0.36f }, { 0.07f, 1.1f, 0.12f }, v.colorEquipo);

    // Cifras: apagadas en el molde hasta que se colocan; al completar el
    // 67 laten hacia el dorado mientras dura la mesa completa.
    Color molde = Color{ 62, 66, 78, 255 };
    float completo = equipo.tiempoMesaCompleta > 0.0f
        ? 0.5f + 0.5f * std::sin(v.tiempo * 24.0f)
        : 0.0f;

    Color cifra6 = equipo.mesa != MESA_67_VACIA
        ? Mezclar67(Mezclar67(COLOR_6_67, DESTELLO_67, equipo.destello6 * 0.75f), Color{ 255, 226, 110, 255 }, completo * 0.6f)
        : molde;
    Color cifra7 = equipo.mesa == MESA_67_COMPLETA
        ? Mezclar67(Mezclar67(COLOR_7_67, DESTELLO_67, equipo.destello7 * 0.75f), Color{ 255, 226, 110, 255 }, completo * 0.6f)
        : molde;

    DibujarNumero3D67(6, PosicionCifraMesa67(6), 0.42f, cifra6);
    DibujarNumero3D67(7, PosicionCifraMesa67(7), 0.42f, cifra7);
}


// El tinte puro multiplica la textura oscura del modelo: se aclara hacia el
// blanco para que el color del jugador se reconozca en la vista del equipo.
static Color TinteJugador67(Color color)
{
    const float mezcla = 0.62f;

    return Color
    {
        (unsigned char)(color.r + (255 - color.r) * mezcla),
        (unsigned char)(color.g + (255 - color.g) * mezcla),
        (unsigned char)(color.b + (255 - color.b) * mezcla),
        color.a
    };
}


static void DibujarJugador67(
    const Vista67& v,
    int indice,
    const Participante& participante
)
{
    const Minijuego67& minijuego = *v.minijuego;
    const EstadoJugador67& estado = minijuego.estadosJugadores[indice];
    float z = ObtenerZJugador67(minijuego, indice);

    // Al fallar: tinte rojizo y un temblor corto.
    float aturdido = Clamp(estado.tiempoStun / DURACION_STUN_67, 0.0f, 1.0f);
    float x = X_JUGADOR_67 + std::sin(v.tiempo * 70.0f) * 0.07f * aturdido * ObtenerFactorMovimientoCamaraMinijuegos();

    Color color = Mezclar67(TinteJugador67(participante.color), Color{ 255, 96, 76, 255 }, aturdido * 0.75f);

    float giro = -90.0f;

    if (estado.llevaPieza)
    {
        float progreso = 1.0f;

        if (estado.tiempoGiro > 0.0f)
        {
            progreso =
                1.0f - estado.tiempoGiro / DURACION_GIRO_67;
        }

        progreso = Clamp(progreso, 0.0f, 1.0f);
        giro = -90.0f + 180.0f * progreso;
    }

    // Sombra y anillo del color del jugador.
    (DrawCylinder)({ x, 0.052f, z }, 0.44f, 0.44f, 0.004f, 16, Fade(BLACK, 0.4f));
    (DrawCylinder)({ x, 0.058f, z }, 0.56f, 0.56f, 0.012f, 24, Fade(participante.color, 0.55f));

    // Flecha del color del jugador sobre la cabeza: el modelo es oscuro.
    float rebote = std::sin(v.tiempo * 5.0f + (float)indice) * 0.06f * ObtenerFactorMovimientoCamaraMinijuegos();
    DrawCylinderEx(
        { x, 2.30f + rebote, z },
        { x, 2.62f + rebote, z },
        0.0f,
        0.30f,
        10,
        participante.color
    );

    const Model* modeloJugador =
        ObtenerModeloJugadorCompartidoRender();

    if (modeloJugador != nullptr)
    {
        (DrawModelEx)(
            *modeloJugador,
            { x, 0.06f, z },
            { 0.0f, 1.0f, 0.0f },
            giro,
            { 0.25f, 0.25f, 0.25f },
            color
        );
    }
    else
    {
        Cubo67({ x, 0.84f, z }, { 0.72f, 1.55f, 0.72f }, color);
    }

    if (estado.llevaPieza)
    {
        bool es6 = estado.tipoPieza == PIEZA_NUMERO_6;

        DibujarNumero3D67(
            es6 ? 6 : 7,
            { x, 2.0f, z },
            0.5f,
            es6 ? COLOR_6_67 : COLOR_7_67
        );
    }
}


static void DibujarParticulas67(const Minijuego67& minijuego, int equipo, bool translucidas)
{
    for (int i = 0; i < MAX_PARTICULAS_67; i++)
    {
        const Particula67& p = minijuego.particulas[i];

        if (!p.activa || p.equipo != equipo)
        {
            continue;
        }

        bool esHumo = p.tipo == PARTICULA_67_HUMO || p.tipo == PARTICULA_67_VAPOR;

        if (esHumo != translucidas)
        {
            continue;
        }

        float f = p.vida / p.vidaMaxima;

        if (p.tipo == PARTICULA_67_CHISPA)
        {
            Cubo67(p.posicion, { p.tamano, p.tamano, p.tamano }, Mezclar67(DESTELLO_67, p.color, 1.0f - f * 0.6f));
        }
        else if (p.tipo == PARTICULA_67_CONFETI)
        {
            rlPushMatrix();
            rlTranslatef(p.posicion.x, p.posicion.y, p.posicion.z);
            rlRotatef(p.giro, 0.6f, 0.3f, 0.75f);
            Cubo67({ 0.0f, 0.0f, 0.0f }, { p.tamano, p.tamano * 0.12f, p.tamano * 0.6f }, ConAlfa67(p.color, Clamp(f * 3.0f, 0.0f, 1.0f)));
            rlPopMatrix();
        }
        else
        {
            float alfa = (p.tipo == PARTICULA_67_VAPOR ? 0.34f : 0.62f) * f;
            Cubo67(p.posicion, { p.tamano, p.tamano, p.tamano }, ConAlfa67(p.color, alfa));
        }
    }
}


//==================================================
// DIBUJO: LUCES (pase aditivo, al final de la escena)
//==================================================

static void DibujarLuces67(const Vista67& v, const EstadoEquipo67& equipo)
{
    if (FactorCalidadGrafica(CalidadEfectos()) < 0.5f)
    {
        return;
    }

    float t = v.tiempo;

    rlDrawRenderBatchActive();
    rlDisableDepthMask();
    BeginBlendMode(BLEND_ADDITIVE);

    // Lamparas colgantes: cono de luz, halo y charco sobre la cinta del 6.
    for (int i = 0; i < 2; i++)
    {
        float x = X_LAMPARAS_67[i];
        float z = Z_CINTA_6_67;
        float latido = 0.92f + 0.08f * std::sin(t * 3.1f + (float)i * 1.7f);

        (DrawCylinderEx)({ x, Y_LAMPARAS_67 - 0.05f, z }, { x, Y_CINTA_67 + 0.05f, z }, 0.3f, 1.25f, 16, Fade(LUZ_CALIDA_67, 0.035f * latido));
        (DrawSphere)({ x, Y_LAMPARAS_67, z }, 0.26f, Fade(LUZ_CALIDA_67, 0.32f * latido));
        (DrawSphere)({ x, Y_LAMPARAS_67, z }, 0.55f, Fade(LUZ_CALIDA_67, 0.10f * latido));
        Resplandor67(x, Y_CINTA_67 + 0.03f, z, 1.3f, Fade(LUZ_CALIDA_67, 0.05f * latido));
        Resplandor67(x + 0.4f, 0.06f, z + 1.3f, 1.4f, Fade(LUZ_CALIDA_67, 0.035f * latido));
    }

    // Focos de obra: halo en el cabezal y charco sobre la cinta del 7.
    if (v.decoracion != CALIDAD_BAJA)
    {
        for (int i = 0; i < 2; i++)
        {
            Vector3 f = FOCOS_OBRA_67[i];
            (DrawSphere)({ f.x + 0.08f, 1.5f, f.z - 0.08f }, 0.32f, Fade(Color{ 255, 240, 200, 255 }, 0.22f));
            (DrawSphere)({ f.x + 0.08f, 1.5f, f.z - 0.08f }, 0.7f, Fade(Color{ 255, 230, 180, 255 }, 0.07f));
            Resplandor67(f.x + 1.1f, Y_CINTA_67 + 0.03f, Z_CINTA_7_67, 1.2f, Fade(Color{ 255, 236, 200, 255 }, 0.04f));
            Resplandor67(f.x + 0.6f, 0.06f, f.z - 1.3f, 1.5f, Fade(Color{ 255, 236, 200, 255 }, 0.035f));
        }
    }

    // Balizas ambar de las tolvas (giran).
    for (int k = 0; k < 2; k++)
    {
        float z = k == 0 ? Z_CINTA_6_67 : Z_CINTA_7_67;
        float giro = 0.5f + 0.5f * std::sin(t * 6.0f + (float)k * 1.4f);
        Vector3 b = { X_INICIO_CINTA_67 - 0.62f + 0.32f, 2.62f, z - 0.55f };
        (DrawSphere)(b, 0.22f, Fade(Color{ 255, 160, 50, 255 }, 0.35f * giro));
        (DrawSphere)(b, 0.5f, Fade(Color{ 255, 140, 40, 255 }, 0.10f * giro));
    }

    // Cartel de neon de la pared (lo apagado brilla poco).
    Vector3 cartel = { -1.4f, 2.2f, Z_PARED_67 + 0.32f };
    bool hay6 = equipo.mesa != MESA_67_VACIA;
    bool hay7 = equipo.mesa == MESA_67_COMPLETA;
    DibujarHaloNumero67(6, { cartel.x - 0.55f, cartel.y, cartel.z }, 0.95f, COLOR_6_67, hay6 ? 1.0f + equipo.destello6 : 0.18f);
    DibujarHaloNumero67(7, { cartel.x + 0.55f, cartel.y, cartel.z }, 0.95f, COLOR_7_67, hay7 ? 1.0f + equipo.destello7 : 0.18f);

    // Carteles de las tolvas.
    for (int k = 0; k < 2; k++)
    {
        float z = k == 0 ? Z_CINTA_6_67 : Z_CINTA_7_67;
        DibujarHaloNumero67(k == 0 ? 6 : 7, { X_INICIO_CINTA_67 - 0.62f, 0.68f, z + 0.76f }, 0.5f, k == 0 ? COLOR_6_67 : COLOR_7_67, 0.6f);
    }

    // Mesa: la cifra colocada brilla; el 67 completo ilumina la mesa.
    if (hay6)
    {
        DibujarHaloNumero67(6, PosicionCifraMesa67(6), 0.42f, COLOR_6_67, 0.8f + equipo.destello6 * 1.5f);
    }

    if (hay7)
    {
        DibujarHaloNumero67(7, PosicionCifraMesa67(7), 0.42f, COLOR_7_67, 0.8f + equipo.destello7 * 1.5f);
    }

    float brilloMesa = equipo.destello7;

    if (brilloMesa > 0.01f)
    {
        Resplandor67(X_MESA_67, 0.95f, 0.0f, 1.3f, Fade(Color{ 255, 226, 110, 255 }, 0.09f * brilloMesa));
        Resplandor67(X_MESA_67, 0.07f, 0.4f, 2.4f, Fade(Color{ 255, 210, 90, 255 }, 0.06f * brilloMesa));
        (DrawSphere)({ X_MESA_67, 1.4f, 0.1f }, 1.0f * (1.0f + 0.3f * (1.0f - brilloMesa)), Fade(Color{ 255, 220, 120, 255 }, 0.12f * brilloMesa));
    }

    // Destello de soldadura en la punta de los brazos.
    for (int b = 0; b < 2; b++)
    {
        PoseBrazo67 pose = CalcularPoseBrazo67(t, FASE_BRAZOS_67[b]);

        if (!pose.soldando)
        {
            continue;
        }

        Vector3 punta = PuntaBrazo67(BASE_BRAZOS_67[b], pose);
        float chisporroteo = 0.6f + 0.4f * std::sin(t * 61.0f + (float)b * 3.0f);
        (DrawSphere)(punta, 0.16f, Fade(Color{ 190, 230, 255, 255 }, 0.7f * chisporroteo));
        (DrawSphere)(punta, 0.5f, Fade(Color{ 120, 180, 255, 255 }, 0.16f * chisporroteo));
        Resplandor67(punta.x, 0.92f, punta.z, 0.7f, Fade(Color{ 150, 200, 255, 255 }, 0.08f * chisporroteo));
    }

    EndBlendMode();
    rlEnableDepthMask();
}


//==================================================
// DIBUJO: VISTA DE UN EQUIPO
//==================================================

static const char* TextoAviso67(TipoAviso67 tipo)
{
    switch (tipo)
    {
        case AVISO_67_COMPLETO: return "¡67! +1";
        case AVISO_67_ORDEN: return "¡ORDEN INCORRECTO!";
        case AVISO_67_DESTIEMPO: return "¡A DESTIEMPO!";
    }

    return "";
}


static Color ColorAviso67(TipoAviso67 tipo)
{
    switch (tipo)
    {
        case AVISO_67_COMPLETO: return Color{ 150, 240, 160, 255 };
        case AVISO_67_ORDEN: return Color{ 255, 130, 110, 255 };
        case AVISO_67_DESTIEMPO: return Color{ 255, 196, 120, 255 };
    }

    return UI_TEXTO_MUNDO;
}


static void DibujarVistaEquipo67(
    const Minijuego67& minijuego,
    int equipo,
    Rectangle vista,
    int cantidadMaxima,
    const Participante participantes[]
)
{
    const EstadoEquipo67& estadoEquipo = minijuego.equipos[equipo];
    float factorMovimiento = ObtenerFactorMovimientoCamaraMinijuegos();
    float t = minijuego.tiempoAnimacion;

    Camera3D camara = minijuego.camarasEquipo[equipo];

    // Durante la cuenta la camara se acerca un poco.
    if (minijuego.estadoPartida == FABRICA_67_PREPARANDO)
    {
        float intro = Clamp(minijuego.tiempoPreparacion / DURACION_PREPARACION_67, 0.0f, 1.0f);
        camara.fovy *= 1.0f + 0.10f * intro * intro * factorMovimiento;
    }

    // Sacudida propia de la vista al equivocarse.
    float amplitud = estadoEquipo.sacudida * estadoEquipo.sacudida * 0.26f * factorMovimiento;
    Vector3 temblor =
    {
        std::sin(t * 63.0f + (float)equipo) * amplitud,
        std::cos(t * 57.0f) * amplitud * 0.6f,
        0.0f
    };
    camara.position = Vector3Add(camara.position, temblor);
    camara.target = Vector3Add(camara.target, temblor);

    Vista67 v;
    v.minijuego = &minijuego;
    v.equipo = equipo;
    v.tiempo = t;
    v.posicionCamara = camara.position;
    v.direccionCamara = Vector3Normalize(Vector3Subtract(camara.target, camara.position));
    v.colorEquipo = ObtenerColorEquipo67(equipo);
    v.decoracion = CalidadDecoracion();

    float escalaX = (float)rlGetFramebufferWidth() / (float)GetScreenWidth();
    float escalaY = (float)rlGetFramebufferHeight() / (float)GetScreenHeight();
    int x0 = (int)vista.x;
    int y0 = (int)vista.y;
    int ancho = (int)vista.width;
    int alto = (int)vista.height;

    BeginScissorMode(x0, y0, ancho, alto);

    Color fondo = Mezclar67(NIEBLA_67, v.colorEquipo, 0.08f);
    DrawRectangleGradientV(x0, y0, ancho, alto, Sombrear67(fondo, 0.55f), fondo);

    BeginMode3D(camara);
    rlViewport(
        (int)(vista.x * escalaX),
        (int)((float)rlGetFramebufferHeight() - (vista.y + vista.height) * escalaY),
        (int)(vista.width * escalaX),
        (int)(vista.height * escalaY)
    );

    double arriba = camara.fovy * 0.5;
    double derecha = arriba * (double)(vista.width / vista.height);
    rlSetMatrixProjection(MatrixOrtho(-derecha, derecha, -arriba, arriba, 0.01, 1000.0));

    DibujarSuelo67(v);
    DibujarParedFondo67(v);
    DibujarUtileria67(v);
    DibujarCinta67(v, Z_CINTA_6_67, estadoEquipo.progresoCinta6, 6);
    DibujarCinta67(v, Z_CINTA_7_67, estadoEquipo.progresoCinta7, 7);
    DibujarFlujoPiezas67(estadoEquipo);
    DibujarMesa67(v, estadoEquipo);

    for (int i = 0; i < cantidadMaxima; i++)
    {
        if (
            minijuego.equipoPorJugador[i] == equipo &&
            participantes[i].activo &&
            participantes[i].conectado
        )
        {
            DibujarJugador67(v, i, participantes[i]);
        }
    }

    DibujarParticulas67(minijuego, equipo, false);
    DibujarLuces67(v, estadoEquipo);

    rlDrawRenderBatchActive();
    rlDisableDepthMask();
    DibujarParticulas67(minijuego, equipo, true);
    rlDrawRenderBatchActive();
    rlEnableDepthMask();

    EndMode3D();
    rlViewport(0, 0, rlGetFramebufferWidth(), rlGetFramebufferHeight());

    // Rotulos de los jugadores (sobre la flecha).
    for (int i = 0; i < cantidadMaxima; i++)
    {
        if (
            minijuego.equipoPorJugador[i] != equipo ||
            !participantes[i].activo ||
            !participantes[i].conectado
        )
        {
            continue;
        }

        Vector2 p = GetWorldToScreenEx(
            { X_JUGADOR_67, 3.05f, ObtenerZJugador67(minijuego, i) },
            camara,
            ancho,
            alto
        );

        char rotulo[8];
        std::snprintf(rotulo, sizeof(rotulo), "J%d", participantes[i].numeroJugador);

        OpcionesEtiquetaUI opciones;
        opciones.borde = participantes[i].color;
        opciones.pico = true;
        DibujarEtiquetaUI(rotulo, { vista.x + p.x, vista.y + p.y }, -5.0f, opciones);
    }

    // Avisos flotantes.
    for (int i = 0; i < MAX_AVISOS_67; i++)
    {
        const AvisoFlotante67& aviso = estadoEquipo.avisos[i];

        if (aviso.tiempo <= 0.0f)
        {
            continue;
        }

        float edad = 1.1f - aviso.tiempo;
        float alfa = Clamp(aviso.tiempo * 3.0f, 0.0f, 1.0f);
        float subida = factorMovimiento > 0.0f ? edad * Px(40.0f) : 0.0f;
        Vector2 p = GetWorldToScreenEx(aviso.posicion, camara, ancho, alto);

        DibujarTextoMundoContornoUI(
            TextoAviso67(aviso.tipo),
            vista.x + p.x,
            vista.y + p.y - subida - (float)i * Px(8.0f),
            UI_TEXTO_GRANDE,
            ColorAviso67(aviso.tipo),
            alfa
        );
    }

    // Vineta roja al equivocarse y dorada al completar un 67.
    float error = estadoEquipo.sacudida;
    float acierto = estadoEquipo.destello7;

    if (error > 0.01f || acierto > 0.01f)
    {
        bool esError = error >= acierto;
        float intensidad = esError ? error : acierto;
        unsigned char a = (unsigned char)((esError ? 110.0f : 70.0f) * intensidad);
        Color borde = esError ? Color{ 255, 50, 36, a } : Color{ 255, 214, 90, a };
        Color nada = { borde.r, borde.g, borde.b, 0 };
        int franja = alto / 5;
        DrawRectangleGradientV(x0, y0, ancho, franja, borde, nada);
        DrawRectangleGradientV(x0, y0 + alto - franja, ancho, franja, nada, borde);
        DrawRectangleGradientH(x0, y0, ancho / 7, alto, borde, nada);
        DrawRectangleGradientH(x0 + ancho - ancho / 7, y0, ancho / 7, alto, nada, borde);
    }

    EndScissorMode();
}


//==================================================
// INICIALIZACION
//==================================================

void Minijuego67::Inicializar()
{
    tiempoPreparacion = DURACION_PREPARACION_67;
    tiempoPartida = DURACION_PARTIDA_67;
    velocidadCintas = 0.30f;
    desplazamientoVisualCintas = 0.0f;
    jugadoresEnPartida = 0;
    mascaraJugadoresEnPartida = 0;
    equipoGanador = -1;
    partidaValida = false;
    empate = false;
    estadoPartida = FABRICA_67_ESPERANDO_JUGADORES;
    tiempoAnimacion = 0.0f;

    // Ortografica, en diagonal desde adelante a la derecha. El objetivo va
    // algo alto para que la nave asome bajo el tendedero del HUD.
    for (int equipo = 0; equipo < 2; equipo++)
    {
        camarasEquipo[equipo].position = { 6.6f, 11.35f, 12.4f };
        camarasEquipo[equipo].target = { -2.2f, 1.2f, 0.0f };
        camarasEquipo[equipo].up = { 0.0f, 1.0f, 0.0f };
        camarasEquipo[equipo].fovy = 14.4f;
        camarasEquipo[equipo].projection = CAMERA_ORTHOGRAPHIC;
    }

    InicializarModeloJugadorCompartido();
}


void Minijuego67::PrepararEquipos(
    const Participante participantes[],
    int cantidadMaxima
)
{
    for (int i = 0; i < cantidadMaxima; i++)
    {
        equipoPorJugador[i] = -1;
        ordenEnEquipoPorJugador[i] = -1;
    }

    cantidadJugadoresEquipo[0] = 0;
    cantidadJugadoresEquipo[1] = 0;

    int activos = ContarParticipantesActivos67(participantes, cantidadMaxima);
    int disponibles = ContarParticipantesDisponibles67(participantes, cantidadMaxima);

    partidaValida =
        activos == disponibles &&
        (activos == 2 || activos == 4);

    jugadoresEnPartida = disponibles;
    mascaraJugadoresEnPartida =
        CrearMascaraParticipantes67(participantes, cantidadMaxima);

    resultado.cantidadEquipos = partidaValida ? 2 : 0;

    if (!partidaValida)
    {
        estadoPartida = FABRICA_67_ESPERANDO_JUGADORES;
        return;
    }

    int indices[MAX_PARTICIPANTES]{};
    int cantidadIndices = 0;

    for (int i = 0; i < cantidadMaxima; i++)
    {
        if (participantes[i].activo && participantes[i].conectado)
        {
            indices[cantidadIndices++] = i;
        }
    }

    for (int i = cantidadIndices - 1; i > 0; i--)
    {
        int otro = GetRandomValue(0, i);
        int temporal = indices[i];
        indices[i] = indices[otro];
        indices[otro] = temporal;
    }

    if (activos == 4)
    {
        for (int orden = 0; orden < 4; orden++)
        {
            int indice = indices[orden];
            int equipo = orden < 2 ? 0 : 1;
            int ordenEquipo = orden % 2;

            equipoPorJugador[indice] = equipo;
            ordenEnEquipoPorJugador[indice] = ordenEquipo;
            cantidadJugadoresEquipo[equipo]++;
            resultado.participantes[indice].numeroEquipo = equipo;
        }
    }
    else
    {
        for (int orden = 0; orden < 2; orden++)
        {
            int indice = indices[orden];
            equipoPorJugador[indice] = orden;
            ordenEnEquipoPorJugador[indice] = 0;
            cantidadJugadoresEquipo[orden] = 1;
            resultado.participantes[indice].numeroEquipo = orden;
        }
    }

    estadoPartida = FABRICA_67_PREPARANDO;
}


void Minijuego67::Reiniciar(
    const Participante participantes[],
    int cantidadMaxima
)
{
    InicializarResultadoMinijuego(
        resultado,
        participantes,
        FORMATO_MINIJUEGO_EQUIPOS
    );

    tiempoPreparacion = DURACION_PREPARACION_67;
    tiempoPartida = DURACION_PARTIDA_67;
    velocidadCintas = 0.30f;
    fotogramaAnimacionIdle = 0.0f;
    equipoGanador = -1;
    empate = false;

    for (int equipo = 0; equipo < 2; equipo++)
    {
        equipos[equipo] = {};
        equipos[equipo].progresoCinta6 =
            equipo == 0 ? 0.02f : 0.11f;
        equipos[equipo].progresoCinta7 =
            NormalizarProgreso67(
                equipos[equipo].progresoCinta6 +
                DESFASE_CINTA_7_67
            );
    }

    for (int i = 0; i < cantidadMaxima; i++)
    {
        estadosJugadores[i] = {};
    }

    for (int i = 0; i < MAX_PARTICULAS_67; i++)
    {
        particulas[i].activa = false;
    }

    tiempoConfeti = 0.0f;
    acumuladorVapor = 0.0f;
    acumuladorSoldadura = 0.0f;

    PrepararEquipos(participantes, cantidadMaxima);

    if (!partidaValida)
    {
        return;
    }

    for (int i = 0; i < cantidadMaxima; i++)
    {
        int equipo = equipoPorJugador[i];

        if (equipo < 0)
        {
            continue;
        }

        if (cantidadJugadoresEquipo[equipo] == 1)
        {
            estadosJugadores[i].tipoPieza = PIEZA_NUMERO_6;
        }
        else
        {
            estadosJugadores[i].tipoPieza =
                ordenEnEquipoPorJugador[i] == 0
                ? PIEZA_NUMERO_6
                : PIEZA_NUMERO_7;
        }
    }
}


//==================================================
// ACTUALIZAR
//==================================================

void Minijuego67::Actualizar(
    float deltaTime,
    int cantidadMaxima,
    const Participante participantes[]
)
{
    tiempoAnimacion += deltaTime;
    ActualizarEfectos67(*this, deltaTime);

    if (AnimacionIdleModeloJugadorCompartidoActiva())
    {
        fotogramaAnimacionIdle +=
            30.0f * deltaTime;

        int cantidadFotogramas =
            ObtenerCantidadFotogramasIdleModeloJugadorCompartido();

        if (cantidadFotogramas > 0)
        {
            AplicarFotogramaIdleModeloJugadorCompartido(
                (int)fotogramaAnimacionIdle %
                cantidadFotogramas
            );
        }
    }

    int mascaraActual =
        CrearMascaraParticipantes67(participantes, cantidadMaxima);

    if (avisoReconfiguracion > 0.0f)
    {
        avisoReconfiguracion -= deltaTime;
        if (avisoReconfiguracion < 0.0f) avisoReconfiguracion = 0.0f;
    }

    if (mascaraActual != mascaraJugadoresEnPartida)
    {
        Reiniciar(participantes, cantidadMaxima);
        avisoReconfiguracion = 4.0f;
        return;
    }

    if (!partidaValida)
    {
        return;
    }

    if (estadoPartida == FABRICA_67_PREPARANDO)
    {
        float antes = tiempoPreparacion;
        tiempoPreparacion -= deltaTime;
        ActualizarAudioCuentaRegresiva(audio, antes, tiempoPreparacion);

        if (tiempoPreparacion <= 0.0f)
        {
            tiempoPreparacion = 0.0f;
            estadoPartida = FABRICA_67_JUGANDO;
        }

        return;
    }

    if (estadoPartida != FABRICA_67_JUGANDO)
    {
        return;
    }

    float tiempoAntes = tiempoPartida;
    tiempoPartida -= deltaTime;
    ActualizarAudioAlertaTiempo(audio, tiempoAntes, tiempoPartida);

    if (tiempoPartida <= 0.0f)
    {
        tiempoPartida = 0.0f;
        estadoPartida = FABRICA_67_FINALIZADO;
        FinalizarResultado67(*this, cantidadMaxima);
        ReproducirSonidoMinijuego(audio, SONIDO_RESULTADO);

        // Confeti sobre la vista del equipo ganador (en empate, nada).
        if (!empate && equipoGanador >= 0)
        {
            tiempoConfeti = 2.6f;
            Confeti67(*this, equipoGanador, 60, ObtenerColorEquipo67(equipoGanador));
        }
        return;
    }

    float progresoPartida =
        1.0f - tiempoPartida / DURACION_PARTIDA_67;

    // Mas rapido desde el inicio y acelera hasta 0.50 al final de los 30 s.
    velocidadCintas = 0.30f + progresoPartida * 0.20f;

    for (int equipo = 0; equipo < 2; equipo++)
    {
        EstadoEquipo67& estadoEquipo = equipos[equipo];

        estadoEquipo.progresoCinta6 =
            NormalizarProgreso67(
                estadoEquipo.progresoCinta6 +
                velocidadCintas * deltaTime
            );

        estadoEquipo.progresoCinta7 =
            NormalizarProgreso67(
                estadoEquipo.progresoCinta7 +
                velocidadCintas * deltaTime
            );

        if (estadoEquipo.tiempoFeedback > 0.0f)
        {
            estadoEquipo.tiempoFeedback -= deltaTime;
            if (estadoEquipo.tiempoFeedback < 0.0f)
                estadoEquipo.tiempoFeedback = 0.0f;
        }

        if (estadoEquipo.tiempoMesaCompleta > 0.0f)
        {
            estadoEquipo.tiempoMesaCompleta -= deltaTime;

            if (estadoEquipo.tiempoMesaCompleta <= 0.0f)
            {
                estadoEquipo.tiempoMesaCompleta = 0.0f;
                estadoEquipo.mesa = MESA_67_VACIA;
                ActualizarRolJugadorUnico67(*this, equipo, cantidadMaxima);
            }
        }
    }

    for (int i = 0; i < cantidadMaxima; i++)
    {
        int equipo = equipoPorJugador[i];

        if (
            equipo < 0 ||
            !participantes[i].activo ||
            !participantes[i].conectado
        )
        {
            continue;
        }

        EstadoJugador67& jugador = estadosJugadores[i];

        if (jugador.tiempoStun > 0.0f)
        {
            jugador.tiempoStun -= deltaTime;
            if (jugador.tiempoStun < 0.0f) jugador.tiempoStun = 0.0f;
        }

        if (jugador.tiempoCooldownInteraccion > 0.0f)
        {
            jugador.tiempoCooldownInteraccion -= deltaTime;
            if (jugador.tiempoCooldownInteraccion < 0.0f)
                jugador.tiempoCooldownInteraccion = 0.0f;
        }

        if (jugador.llevaPieza && jugador.tiempoGiro > 0.0f)
        {
            jugador.tiempoGiro -= deltaTime;

            if (jugador.tiempoGiro <= 0.0f)
            {
                jugador.tiempoGiro = 0.0f;
                jugador.mirandoMesa = true;
            }
        }

        if (
            jugador.tiempoStun > 0.0f ||
            jugador.tiempoCooldownInteraccion > 0.0f
        )
        {
            continue;
        }

        bool presionar = false;

        if (participantes[i].esBot)
        {
            presionar =
                DecidirPulsacionBot67(
                    *this,
                    i,
                    equipo,
                    deltaTime
                );
        }
        else
        {
            presionar =
                LeerInputMinijuegoParticipante(participantes[i]).saltar;
        }

        if (!presionar)
        {
            continue;
        }

        if (jugador.llevaPieza)
        {
            if (jugador.mirandoMesa)
            {
                ColocarPieza67(*this, i, cantidadMaxima);
            }

            continue;
        }

        bool alAlcance =
            HayPiezaAlAlcance67(
                equipos[equipo],
                jugador.tipoPieza
            );

        float zCinta =
            jugador.tipoPieza == PIEZA_NUMERO_6
            ? Z_CINTA_6_67
            : Z_CINTA_7_67;

        float zJugador = ObtenerZJugador67(*this, i);

        if (alAlcance)
        {
            jugador.llevaPieza = true;
            jugador.mirandoMesa = false;
            jugador.tiempoGiro = DURACION_GIRO_67;
            jugador.tiempoCooldownInteraccion = COOLDOWN_TRAS_AGARRAR_67;

            Chispas67(
                *this,
                equipo,
                { X_JUGADOR_67 - 0.6f, Y_CINTA_67 + 0.3f, zCinta },
                5,
                jugador.tipoPieza == PIEZA_NUMERO_6 ? COLOR_6_67 : COLOR_7_67,
                0.8f
            );
            ReproducirSonidoMinijuego(audio, SONIDO_RECOGER_OBJETO);
        }
        else
        {
            jugador.tiempoStun = 0.22f;
            equipos[equipo].ultimoAcierto = false;
            equipos[equipo].tiempoFeedback = 0.34f;

            // Manotazo al aire: humo chico, sacudida leve y aviso.
            equipos[equipo].sacudida = 0.6f;
            Humo67(*this, equipo, { X_JUGADOR_67 - 0.5f, 1.0f, (zJugador + zCinta) * 0.5f }, 5);
            PonerAviso67(equipos[equipo], AVISO_67_DESTIEMPO, { X_JUGADOR_67, 3.2f, zJugador });
            ReproducirSonidoMinijuego(audio, SONIDO_ERROR);
        }
    }
}


//==================================================
// DIBUJAR
//==================================================

void Minijuego67::Dibujar(
    int cantidadMaxima,
    const Participante participantes[]
) const
{
    SeleccionarTemaVisualMinijuego(TEMA_VISUAL_NINGUNO);
    ClearBackground(NIEBLA_67);

    int ancho = GetScreenWidth();
    int alto = GetScreenHeight();
    int mitad = ancho / 2;

    DibujarVistaEquipo67(*this, 0, { 0.0f, 0.0f, (float)(mitad - 3), (float)alto }, cantidadMaxima, participantes);
    DibujarVistaEquipo67(*this, 1, { (float)(mitad + 3), 0.0f, (float)(ancho - mitad - 3), (float)alto }, cantidadMaxima, participantes);

    // Divisoria de acero con un filo del color de cada equipo.
    DrawRectangle(mitad - 3, 0, 6, alto, Color{ 16, 18, 24, 255 });
    DrawRectangle(mitad - 3, 0, 1, alto, Fade(ObtenerColorEquipo67(0), 0.85f));
    DrawRectangle(mitad + 2, 0, 1, alto, Fade(ObtenerColorEquipo67(1), 0.85f));

    if (estadoPartida == FABRICA_67_ESPERANDO_JUGADORES)
    {
        DibujarVeloUI(0.55f);
        DibujarTextoMundoContornoUI("FÁBRICA 67", ancho * 0.5f, alto * 0.36f, UI_TEXTO_TITULO, UI_FOCO);
        DibujarTextoMundoContornoUI("SE NECESITAN 2 O 4 JUGADORES", ancho * 0.5f, alto * 0.36f + Px(76.0f), UI_TEXTO_GRANDE, UI_TEXTO_MUNDO);
        DibujarTextoMundoContornoUI("CON 4: 2 VS 2   |   CON 2: 1 VS 1", ancho * 0.5f, alto * 0.36f + Px(120.0f), UI_TEXTO, UI_TEXTO_MUNDO);
    }

    if (avisoReconfiguracion > 0.0f)
    {
        DibujarTextoMundoContornoUI(
            "SE RECONFIGURARON LOS EQUIPOS",
            ancho * 0.5f,
            alto * 0.24f,
            UI_TEXTO_GRANDE,
            UI_FOCO,
            Clamp(avisoReconfiguracion * 2.0f, 0.0f, 1.0f)
        );
    }

    if (estadoPartida == FABRICA_67_ESPERANDO_JUGADORES)
    {
        return;
    }

    // HUD comun a pantalla completa: equipo 0 a la izquierda del reloj
    // y equipo 1 a la derecha, igual que las vistas.
    DatosHudMinijuego hud;
    PrepararDatosHudMinijuego(
        hud,
        "FÁBRICA 67",
        "ARMÁ MÁS 67 QUE EL RIVAL: AGARRÁ TU CIFRA EN LA FRANJA AMARILLA Y PONÉ PRIMERO EL 6.",
        resultado
    );

    hud.fase = estadoPartida == FABRICA_67_PREPARANDO
        ? HUD_MINIJUEGO_CUENTA
        : (estadoPartida == FABRICA_67_JUGANDO ? HUD_MINIJUEGO_JUGANDO : HUD_MINIJUEGO_FIN);
    hud.cuentaRestante = tiempoPreparacion;
    hud.reloj = HUD_RELOJ_CUENTA_ATRAS;
    hud.tiempoRestante = tiempoPartida > 0.0f ? tiempoPartida : 0.0f;
    hud.tiempoTotal = DURACION_PARTIDA_67;
    hud.segundosAlerta = 10.0f;
    hud.finPorTiempo = true;

    if (estadoPartida == FABRICA_67_FINALIZADO && resultado.estado == RESULTADO_MINIJUEGO_FINALIZADO)
    {
        if (empate)
        {
            hud.tituloFinal = "¡EMPATE!";
            hud.notaFinal = "EMPATE: LOS DOS EQUIPOS ARMARON LA MISMA CANTIDAD DE 67.";
        }
        else
        {
            hud.tituloFinal = equipoGanador == 0
                ? "¡GANA EL EQUIPO ROJO!"
                : "¡GANA EL EQUIPO AZUL!";
        }
    }

    for (int i = 0; i < MAX_PARTICIPANTES && i < cantidadMaxima; i++)
    {
        JugadorHudMinijuego& jugador = hud.jugadores[i];
        int equipo = equipoPorJugador[i];

        if (!jugador.visible || equipo < 0 || equipo > 1)
        {
            continue;
        }

        const EstadoEquipo67& estadoEquipo = equipos[equipo];
        const EstadoJugador67& estado = estadosJugadores[i];
        int puntos = estadoEquipo.puntos;
        bool es6 = estado.tipoPieza == PIEZA_NUMERO_6;

        jugador.equipo = equipo;
        jugador.rol = es6 ? "CIFRA 6" : "CIFRA 7";

        // Valor: 67 armados por el equipo. Barra: el 67 en curso (6 y 7).
        std::snprintf(jugador.valor, sizeof(jugador.valor), "%d PTS", puntos);
        jugador.valorNumerico = puntos * 2 + (estadoEquipo.mesa == MESA_67_CON_6 ? 1 : 0);
        jugador.progreso =
            estadoEquipo.mesa == MESA_67_VACIA ? 0.0f
            : (estadoEquipo.mesa == MESA_67_CON_6 ? 0.5f : 1.0f);

        if (estado.tiempoStun > 0.0f)
        {
            std::snprintf(jugador.estado, sizeof(jugador.estado), "¡ERROR!");
        }
        else if (estado.llevaPieza)
        {
            std::snprintf(jugador.estado, sizeof(jugador.estado), "LLEVA EL %d", es6 ? 6 : 7);
        }

        std::snprintf(
            jugador.detalleFinal,
            sizeof(jugador.detalleFinal),
            puntos == 1 ? "%d PUNTO" : "%d PUNTOS",
            puntos
        );

        if (!participantes[i].esBot)
        {
            std::snprintf(
                jugador.controles,
                sizeof(jugador.controles),
                "%s: AGARRAR Y PONER\nEN LA FRANJA AMARILLA",
                ObtenerTextoBotonPrincipal(participantes[i])
            );
        }
    }

    DibujarHudMinijuego(hud, participantes);
}


const ResultadoMinijuego& Minijuego67::ObtenerResultado() const
{
    return resultado;
}


// Sin recursos propios: la pantalla dividida usa recorte y viewport.
void Minijuego67::Descargar()
{
}
