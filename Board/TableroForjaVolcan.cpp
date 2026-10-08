#include "Board/CatalogoTableros.h"

#include "Gameplay/PartidaTablero.h"
#include "Systems/CalidadGrafica.h"

#include "rlgl.h"

#include <cmath>
#include <cstdio>


//==================================================
// FORJA DEL VOLCAN (tablero 2)
//==================================================
//
// Un anillo exterior de 26 casillas rodea un lago de lava y un volcan.
// Dos atajos atraviesan el centro y se parten del anillo en una
// bifurcacion:
//
//   - PUENTE DE BASALTO (casilla 6 -> 26..29 -> 15): cruza la lava.
//     Es el GIMMICK principal: se levanta y se baja cada ronda; con el
//     puente levantado la casilla 6 deja de ser bifurcacion.
//   - GALERIA MINERA (casilla 18 -> 30..33 -> 3): siempre abierta,
//     corta y llena de casillas rojas, con un geiser al final.
//
// Segundo gimmick: cada ronda el volcan escupe una llama sobre una
// casilla del anillo; quien termina su turno ahi pierde 5 monedas.
//
// Casillas especiales (eventos propios): veta de oro, duende ladron
// y geiser teletransportador.

static const int CASILLAS_ANILLO = 26;

static const int BIFURCACION_PUENTE = 6;
static const int PUENTE_PRIMERA = 26;
static const int PUENTE_ULTIMA = 29;
static const int UNION_PUENTE = 15;

static const int BIFURCACION_GALERIA = 18;
static const int GALERIA_PRIMERA = 30;
static const int GALERIA_ULTIMA = 33;
static const int UNION_GALERIA = 3;

static const int PERDIDA_ERUPCION = 5;
static const int PREMIO_VETA = 7;
static const int ROBO_DUENDE = 5;

static const float ALTURA_CASILLA_FORJA = 0.25f;
static const float VELOCIDAD_ANIMACION_PUENTE = 1.1f;

static const Vector3 CENTRO_VOLCAN = { -5.6f, 0.0f, 0.0f };

static const int CASILLAS_TROFEO_FORJA[] =
{
    1,
    5,
    9,
    11,
    14,
    20,
    23,
    31
};


//==================================================
// CONSTRUCCION
//==================================================

static Vector3 PosicionAnillo(
    int indice
)
{
    const float y = ALTURA_CASILLA_FORJA;

    if (indice <= 8)
    {
        // Borde inferior, de izquierda a derecha.
        return { -9.0f + 2.25f * indice, y, 6.0f };
    }

    if (indice <= 12)
    {
        // Borde derecho, hacia arriba.
        return { 9.0f, y, 3.6f - 2.4f * (indice - 9) };
    }

    if (indice <= 21)
    {
        // Borde superior, de derecha a izquierda.
        return { 9.0f - 2.25f * (indice - 13), y, -6.0f };
    }

    // Borde izquierdo, hacia abajo.
    return { -9.0f, y, -3.6f + 2.4f * (indice - 22) };
}


static void ConstruirForjaVolcan(
    Tablero& tablero
)
{
    tablero.cantidadCasillas = 0;
    tablero.recorridoValido = false;

    for (int i = 0; i < MAX_CASILLAS_TABLERO; i++)
    {
        tablero.casillas[i] = Casilla{};
    }

    // N neutra, P positiva, G negativa, E especial.
    const char* tiposAnillo = "NPNPGNPNGPENGPNPGNPNGPNEGP";

    for (int i = 0; i < CASILLAS_ANILLO; i++)
    {
        TipoCasilla tipo = CASILLA_NEUTRA;

        if (tiposAnillo[i] == 'P')
        {
            tipo = CASILLA_POSITIVA;
        }
        else if (tiposAnillo[i] == 'G')
        {
            tipo = CASILLA_NEGATIVA;
        }
        else if (tiposAnillo[i] == 'E')
        {
            tipo = CASILLA_ESPECIAL;
        }

        tablero.AgregarCasilla(PosicionAnillo(i), tipo);
    }

    const float y = ALTURA_CASILLA_FORJA;

    // Puente de basalto: 26..29 (columna x = 4.5).
    tablero.AgregarCasilla({ 4.5f, y,  3.6f }, CASILLA_NEGATIVA); // 26
    tablero.AgregarCasilla({ 4.5f, y,  1.2f }, CASILLA_ESPECIAL); // 27
    tablero.AgregarCasilla({ 4.5f, y, -1.2f }, CASILLA_NEGATIVA); // 28
    tablero.AgregarCasilla({ 4.5f, y, -3.6f }, CASILLA_POSITIVA); // 29

    // Galeria minera: 30..33 (columna x = -2.25).
    tablero.AgregarCasilla({ -2.25f, y, -3.6f }, CASILLA_NEGATIVA); // 30
    tablero.AgregarCasilla({ -2.25f, y, -1.2f }, CASILLA_ESPECIAL); // 31
    tablero.AgregarCasilla({ -2.25f, y,  1.2f }, CASILLA_NEGATIVA); // 32
    tablero.AgregarCasilla({ -2.25f, y,  3.6f }, CASILLA_POSITIVA); // 33

    // El anillo es un circuito cerrado.
    for (int i = 0; i < CASILLAS_ANILLO; i++)
    {
        tablero.ConectarCasillas(
            i,
            (i + 1) % CASILLAS_ANILLO
        );
    }

    // Bifurcacion del puente: la conexion 0 (anillo) es la larga y
    // segura; la 1 (puente) es el atajo. El gimmick puede cerrar la 1.
    tablero.ConectarCasillas(BIFURCACION_PUENTE, PUENTE_PRIMERA);

    for (int i = PUENTE_PRIMERA; i < PUENTE_ULTIMA; i++)
    {
        tablero.ConectarCasillas(i, i + 1);
    }

    tablero.ConectarCasillas(PUENTE_ULTIMA, UNION_PUENTE);

    // Bifurcacion de la galeria minera.
    tablero.ConectarCasillas(BIFURCACION_GALERIA, GALERIA_PRIMERA);

    for (int i = GALERIA_PRIMERA; i < GALERIA_ULTIMA; i++)
    {
        tablero.ConectarCasillas(i, i + 1);
    }

    tablero.ConectarCasillas(GALERIA_ULTIMA, UNION_GALERIA);

    // Se valida con todo abierto.
    tablero.recorridoValido =
        tablero.ValidarRecorrido();
}


//==================================================
// UTILIDADES
//==================================================

static float Parpadeo(
    float tiempo,
    float velocidad
)
{
    return 0.5f + 0.5f * std::sin(tiempo * velocidad);
}


static void DibujarCristalForja(
    Vector3 base,
    Color color,
    float altura
)
{
    DrawCylinder(
        base,
        0.0f,
        0.30f,
        altura,
        6,
        color
    );

    Vector3 pequeno = base;
    pequeno.x += 0.32f;
    pequeno.z += 0.12f;

    DrawCylinder(
        pequeno,
        0.0f,
        0.18f,
        altura * 0.6f,
        6,
        color
    );
}


//==================================================
// DECORACION
//==================================================

static void DibujarDecoracionForjaVolcan(
    const PartidaTablero& partida
)
{
    // MODELO FUTURO: caldera volcanica completa como modelo 3D.
    float t = partida.tiempoTablero;

    const Color roca = { 58, 46, 50, 255 };
    const Color rocaOscura = { 34, 26, 30, 255 };

    // Suelo de basalto.
    DrawCube(
        Vector3{ 0.0f, -0.42f, 0.0f },
        22.4f,
        0.65f,
        16.4f,
        roca
    );

    DrawCube(
        Vector3{ 0.0f, -0.55f, 0.0f },
        23.2f,
        0.38f,
        17.2f,
        rocaOscura
    );

    // Muro de roca al fondo.
    for (int i = 0; i < 9; i++)
    {
        float altura = 1.6f + 0.5f * (float)((i * 7) % 4);

        DrawCube(
            Vector3{ -10.0f + 2.5f * i, altura * 0.5f - 0.1f, -8.4f },
            2.7f,
            altura,
            1.6f,
            Color{ 48, 38, 42, 255 }
        );
    }

    // Lago de lava y su borde.
    DrawCube(
        Vector3{ 4.5f, -0.07f, 0.0f },
        6.4f,
        0.06f,
        9.8f,
        Color{ 226, 78, 20, 255 }
    );

    int vetasLava = (int)std::ceil(5.0f * FactorCalidadGrafica(CalidadEfectos()));

    for (int i = 0; i < vetasLava; i++)
    {
        float fase =
            std::fmod(t * 0.35f + i * 0.2f, 1.0f);

        DrawCube(
            Vector3
            {
                4.5f + std::sin(t * 0.8f + i * 1.7f) * 1.6f,
                -0.035f,
                -4.5f + fase * 9.0f
            },
            1.6f,
            0.02f,
            0.32f,
            Color{ 255, 190, 60, 255 }
        );
    }

    DrawCube({ 1.2f, 0.02f, 0.0f }, 0.25f, 0.18f, 10.0f, rocaOscura);
    DrawCube({ 7.8f, 0.02f, 0.0f }, 0.25f, 0.18f, 10.0f, rocaOscura);
    DrawCube({ 4.5f, 0.02f, -5.0f }, 6.6f, 0.18f, 0.25f, rocaOscura);
    DrawCube({ 4.5f, 0.02f, 5.0f }, 6.6f, 0.18f, 0.25f, rocaOscura);

    // Pilares bajo el puente.
    for (int i = PUENTE_PRIMERA; i <= PUENTE_ULTIMA; i++)
    {
        const Casilla* casilla =
            partida.tablero.ObtenerCasilla(i);

        if (casilla == nullptr)
        {
            continue;
        }

        DrawCylinder(
            Vector3
            {
                casilla->posicion.x,
                -0.05f,
                casilla->posicion.z
            },
            0.38f,
            0.46f,
            0.30f,
            8,
            Color{ 96, 82, 86, 255 }
        );
    }

    // Volcan.
    DrawCylinder(
        Vector3{ CENTRO_VOLCAN.x, -0.1f, CENTRO_VOLCAN.z },
        2.0f,
        2.7f,
        1.2f,
        20,
        Color{ 70, 54, 56, 255 }
    );

    DrawCylinder(
        Vector3{ CENTRO_VOLCAN.x, 1.1f, CENTRO_VOLCAN.z },
        0.7f,
        2.0f,
        3.4f,
        20,
        Color{ 92, 68, 62, 255 }
    );

    float brillo = Parpadeo(t, 3.0f);

    DrawCylinder(
        Vector3{ CENTRO_VOLCAN.x, 4.5f, CENTRO_VOLCAN.z },
        0.62f,
        0.62f,
        0.14f,
        16,
        Color
        {
            255,
            (unsigned char)(110 + (int)(brillo * 70.0f)),
            30,
            255
        }
    );

    // Coladas de lava.
    DrawCylinderEx(
        Vector3{ CENTRO_VOLCAN.x + 0.3f, 4.4f, CENTRO_VOLCAN.z + 0.3f },
        Vector3{ CENTRO_VOLCAN.x + 1.7f, 0.2f, CENTRO_VOLCAN.z + 1.1f },
        0.10f,
        0.20f,
        6,
        Color{ 255, 120, 35, 255 }
    );

    DrawCylinderEx(
        Vector3{ CENTRO_VOLCAN.x - 0.3f, 4.4f, CENTRO_VOLCAN.z - 0.2f },
        Vector3{ CENTRO_VOLCAN.x - 1.8f, 0.2f, CENTRO_VOLCAN.z - 0.9f },
        0.10f,
        0.20f,
        6,
        Color{ 255, 120, 35, 255 }
    );

    // Humo y brasas que suben del crater.
    int bocanadas = (int)std::ceil(5.0f * FactorCalidadGrafica(CalidadParticulas()));

    for (int i = 0; i < bocanadas; i++)
    {
        float fase =
            std::fmod(t * 0.30f + i * 0.2f, 1.0f);

        DrawSphere(
            Vector3
            {
                CENTRO_VOLCAN.x + std::sin(i * 2.3f + t) * 0.35f * fase,
                4.7f + fase * 2.6f,
                CENTRO_VOLCAN.z + std::cos(i * 1.9f) * 0.3f * fase
            },
            0.30f + fase * 0.45f,
            Color
            {
                (unsigned char)(120 - (int)(fase * 40.0f)),
                (unsigned char)(104 - (int)(fase * 40.0f)),
                (unsigned char)(104 - (int)(fase * 40.0f)),
                255
            }
        );
    }

    int brasas = (int)std::ceil(6.0f * FactorCalidadGrafica(CalidadParticulas()));

    for (int i = 0; i < brasas; i++)
    {
        float fase =
            std::fmod(t * 0.7f + i * 0.17f, 1.0f);

        DrawSphere(
            Vector3
            {
                CENTRO_VOLCAN.x + std::sin(i * 1.3f) * (0.5f + fase),
                4.6f + fase * 2.0f,
                CENTRO_VOLCAN.z + std::cos(i * 2.9f) * (0.5f + fase)
            },
            0.07f,
            Color{ 255, 170, 50, 255 }
        );
    }

    // Galeria minera: rieles y arcos de madera.
    DrawCubeV({ -2.60f, 0.03f, 0.0f }, { 0.07f, 0.06f, 10.4f }, Color{ 150, 150, 160, 255 });
    DrawCubeV({ -1.90f, 0.03f, 0.0f }, { 0.07f, 0.06f, 10.4f }, Color{ 150, 150, 160, 255 });

    const float arcos[] = { -4.8f, -2.4f, 0.0f, 2.4f };

    for (float z : arcos)
    {
        const Color madera = { 112, 74, 44, 255 };

        DrawCube({ -3.35f, 0.9f, z }, 0.28f, 1.8f, 0.28f, madera);
        DrawCube({ -1.15f, 0.9f, z }, 0.28f, 1.8f, 0.28f, madera);
        DrawCube({ -2.25f, 1.85f, z }, 2.5f, 0.24f, 0.30f, madera);
    }

    // Boca de la galeria: bloques de roca sobre el primer arco.
    DrawCube({ -2.25f, 2.35f, -4.8f }, 3.2f, 0.7f, 0.9f, rocaOscura);

    // Vagoneta con oro junto a las vias.
    DrawCube({ -0.2f, 0.35f, 1.4f }, 0.9f, 0.5f, 0.6f, Color{ 90, 94, 104, 255 });
    DrawSphere({ -0.35f, 0.72f, 1.4f }, 0.22f, GOLD);
    DrawSphere({ -0.05f, 0.70f, 1.45f }, 0.19f, GOLD);

    // Cristales del subsuelo.
    DibujarCristalForja({ 0.2f, 0.0f, -2.6f }, Color{ 90, 210, 235, 255 }, 1.1f);
    DibujarCristalForja({ 0.4f, 0.0f, 3.2f }, Color{ 190, 110, 235, 255 }, 0.9f);
    DibujarCristalForja({ -3.4f, 0.0f, 4.1f }, Color{ 90, 210, 235, 255 }, 0.8f);
    DibujarCristalForja({ -3.5f, 0.0f, -4.0f }, Color{ 190, 110, 235, 255 }, 1.0f);
    DibujarCristalForja({ -8.0f, 0.0f, -2.0f }, Color{ 90, 210, 235, 255 }, 0.7f);
    DibujarCristalForja({ -7.8f, 0.0f, 2.6f }, Color{ 190, 110, 235, 255 }, 0.8f);
}


//==================================================
// GIMMICK
//==================================================

static void DibujarPuenteLevadizo(
    const PartidaTablero& partida
)
{
    const Casilla* base =
        partida.tablero.ObtenerCasilla(BIFURCACION_PUENTE);

    const Casilla* primera =
        partida.tablero.ObtenerCasilla(PUENTE_PRIMERA);

    if (base == nullptr || primera == nullptr)
    {
        return;
    }

    float x = base->posicion.x;
    float zBisagra = base->posicion.z - 0.78f;
    float largo = 2.2f;
    float angulo = partida.gimmickAnim * 78.0f;
    float radianes = angulo * 3.14159265f / 180.0f;

    // Tablon del puente articulado en el borde de la casilla 6.
    rlPushMatrix();
    rlTranslatef(x, 0.47f, zBisagra);
    rlRotatef(angulo, 1.0f, 0.0f, 0.0f);

    DrawCubeV(
        Vector3{ 0.0f, 0.06f, -largo * 0.5f },
        Vector3{ 1.5f, 0.14f, largo },
        Color{ 128, 84, 52, 255 }
    );

    DrawCubeWiresV(
        Vector3{ 0.0f, 0.06f, -largo * 0.5f },
        Vector3{ 1.5f, 0.14f, largo },
        Color{ 50, 30, 20, 255 }
    );

    rlPopMatrix();

    // Torres del mecanismo con luz de estado.
    Vector3 punta =
    {
        x,
        0.47f + largo * std::sin(radianes),
        zBisagra - largo * std::cos(radianes)
    };

    bool cerrado = partida.gimmickAnim > 0.5f;

    Color luz =
        cerrado
        ? Color{ 235, 60, 50, 255 }
        : Color{ 80, 230, 110, 255 };

    for (int lado = -1; lado <= 1; lado += 2)
    {
        Vector3 torre = { x + lado * 1.15f, 1.0f, zBisagra - 0.1f };

        DrawCube(torre, 0.5f, 2.0f, 0.5f, Color{ 86, 78, 84, 255 });

        Vector3 cima = torre;
        cima.y += 1.25f;

        DrawSphere(cima, 0.20f, luz);

        DrawLine3D(cima, punta, Color{ 40, 40, 46, 255 });
    }
}


static void DibujarPeligroForja(
    const PartidaTablero& partida
)
{
    const Casilla* casilla =
        partida.tablero.ObtenerCasilla(partida.casillaPeligro);

    if (casilla == nullptr)
    {
        return;
    }

    float pulso = Parpadeo(partida.tiempoTablero, 7.0f);

    Vector3 base = casilla->posicion;
    base.y += 0.45f;

    DrawCylinder(
        base,
        0.05f,
        0.55f,
        1.5f + 0.5f * pulso,
        10,
        Color{ 255, (unsigned char)(90 + (int)(pulso * 90.0f)), 30, 255 }
    );

    Vector3 punta = base;
    punta.y += 1.6f + 0.5f * pulso;

    DrawSphere(punta, 0.16f, Color{ 255, 220, 90, 255 });

    DrawCubeWires(
        casilla->posicion,
        1.95f,
        0.55f,
        1.95f,
        RED
    );
}


static void DibujarGimmickForjaVolcan(
    const PartidaTablero& partida
)
{
    DibujarPuenteLevadizo(partida);

    if (partida.casillaPeligro >= 0)
    {
        DibujarPeligroForja(partida);
    }
}


static void ActualizarGimmickForjaVolcan(
    PartidaTablero& partida,
    float deltaTime
)
{
    // Solo anima: la logica del puente cambia en AlIniciarRonda.
    float objetivo =
        partida.gimmickActivo
        ? 1.0f
        : 0.0f;

    float paso =
        VELOCIDAD_ANIMACION_PUENTE * deltaTime;

    if (partida.gimmickAnim < objetivo)
    {
        partida.gimmickAnim =
            partida.gimmickAnim + paso > objetivo
            ? objetivo
            : partida.gimmickAnim + paso;
    }
    else if (partida.gimmickAnim > objetivo)
    {
        partida.gimmickAnim =
            partida.gimmickAnim - paso < objetivo
            ? objetivo
            : partida.gimmickAnim - paso;
    }
}


static void ElegirCasillaPeligro(
    PartidaTablero& partida
)
{
    int anterior = partida.casillaPeligro;
    int elegida = -1;

    for (int intento = 0; intento < 30 && elegida < 0; intento++)
    {
        int candidata =
            GetRandomValue(1, CASILLAS_ANILLO - 1);

        const Casilla* casilla =
            partida.tablero.ObtenerCasilla(candidata);

        if (
            casilla != nullptr &&
            casilla->tipo != CASILLA_ESPECIAL &&
            candidata != partida.casillaTrofeo &&
            candidata != anterior
        )
        {
            elegida = candidata;
        }
    }

    partida.casillaPeligro = elegida;
}


static void AlIniciarRondaForjaVolcan(
    PartidaTablero& partida
)
{
    bool levantado =
        partida.rondaActual % 2 == 0;

    partida.gimmickActivo = levantado;

    // Con el puente levantado la casilla 6 solo conserva el anillo.
    // La conexion 1 sigue guardada y se reabre al bajar el puente.
    Casilla& bifurcacion =
        partida.tablero.casillas[BIFURCACION_PUENTE];

    bifurcacion.cantidadConexiones =
        levantado
        ? 1
        : 2;

    ElegirCasillaPeligro(partida);

    std::snprintf(
        partida.estadoGimmick,
        sizeof(partida.estadoGimmick),
        "%s | LLAMA EN EL ANILLO: -%d MONEDAS",
        levantado
            ? "PUENTE LEVANTADO (ATAJO CERRADO)"
            : "PUENTE BAJADO (ATAJO ABIERTO)",
        PERDIDA_ERUPCION
    );

    EstablecerMensajeGimmick(
        partida,
        levantado
            ? "EL PUENTE SE LEVANTA! ATAJO CERRADO"
            : "EL PUENTE BAJA! ATAJO ABIERTO"
    );
}


//==================================================
// EVENTOS DE CASILLA
//==================================================

static int ElegirRivalForja(
    const PartidaTablero& partida,
    int participante
)
{
    int candidatos[MAX_PARTICIPANTES]{};
    int cantidad = 0;

    for (int i = 0; i < partida.cantidadJugadores; i++)
    {
        int indice = partida.ordenParticipantes[i];

        if (
            indice != participante &&
            partida.jugadores[indice].monedas > 0
        )
        {
            candidatos[cantidad] = indice;
            cantidad++;
        }
    }

    if (cantidad <= 0)
    {
        return -1;
    }

    return candidatos[GetRandomValue(0, cantidad - 1)];
}


static void ResolverEventoEspecialForja(
    PartidaTablero& partida,
    int participante
)
{
    EstadoJugadorPartidaTablero& jugador =
        partida.jugadores[participante];

    ReproducirSonidoTablero(partida, SONIDO_EVENTO_TABLERO);

    int evento = GetRandomValue(0, 2);
    int rival = -1;

    if (evento == 1)
    {
        rival = ElegirRivalForja(partida, participante);

        // Sin nadie a quien robar, el duende deja una propina.
        if (rival < 0)
        {
            evento = 0;
        }
    }

    if (evento == 0)
    {
        partida.ultimoEventoEspecial = EVENTO_TABLERO_BONIFICACION;
        partida.variacionMonedasEvento = PREMIO_VETA;
        jugador.monedas += PREMIO_VETA;

        std::snprintf(
            partida.textoEvento,
            sizeof(partida.textoEvento),
            "VETA DE ORO: +%d MONEDAS",
            PREMIO_VETA
        );
    }
    else if (evento == 1)
    {
        EstadoJugadorPartidaTablero& victima =
            partida.jugadores[rival];

        int robo =
            victima.monedas < ROBO_DUENDE
            ? victima.monedas
            : ROBO_DUENDE;

        victima.monedas -= robo;
        jugador.monedas += robo;

        partida.ultimoEventoEspecial = EVENTO_TABLERO_INTERCAMBIO;
        partida.variacionMonedasEvento = robo;
        partida.jugadorIntercambioEvento = rival;

        std::snprintf(
            partida.textoEvento,
            sizeof(partida.textoEvento),
            "DUENDE LADRON: J%d ROBA %d MONEDAS A J%d",
            participante + 1,
            robo,
            rival + 1
        );
    }
    else
    {
        // Geiser: dispara al jugador a otra casilla del anillo.
        int destino = jugador.casillaActual;

        for (int intento = 0; intento < 30; intento++)
        {
            int candidata =
                GetRandomValue(0, CASILLAS_ANILLO - 1);

            if (
                candidata != jugador.casillaActual &&
                candidata != partida.casillaTrofeo
            )
            {
                destino = candidata;
                break;
            }
        }

        TeletransportarJugadorTablero(
            partida,
            participante,
            destino
        );

        std::snprintf(
            partida.textoEvento,
            sizeof(partida.textoEvento),
            "GEISER: J%d SALE DISPARADO A OTRA CASILLA",
            participante + 1
        );
    }
}


static bool ResolverCasillaForjaVolcan(
    PartidaTablero& partida,
    int participante
)
{
    EstadoJugadorPartidaTablero& jugador =
        partida.jugadores[participante];

    const Casilla* casilla =
        partida.tablero.ObtenerCasilla(jugador.casillaActual);

    if (casilla == nullptr)
    {
        return false;
    }

    if (casilla->tipo == CASILLA_ESPECIAL)
    {
        ResolverEventoEspecialForja(partida, participante);
        return true;
    }

    if (jugador.casillaActual == partida.casillaPeligro)
    {
        int perdida =
            jugador.monedas < PERDIDA_ERUPCION
            ? jugador.monedas
            : PERDIDA_ERUPCION;

        jugador.monedas -= perdida;
        partida.variacionMonedasEvento = -perdida;

        std::snprintf(
            partida.textoEvento,
            sizeof(partida.textoEvento),
            "ERUPCION! LA LLAMA TE QUEMA: -%d MONEDAS",
            perdida
        );

        ReproducirSonidoTablero(partida, SONIDO_CASILLA_NEGATIVA);

        return true;
    }

    return false;
}


//==================================================
// BOTS Y TEXTOS DE RUTA
//==================================================

static int DecidirRutaBotForja(
    const PartidaTablero& partida,
    int participante
)
{
    int casilla =
        partida.jugadores[participante].casillaActual;

    int primeraAtajo = 0;
    int ultimaAtajo = 0;
    bool trofeoEnLargo = false;

    int trofeo = partida.casillaTrofeo;

    if (casilla == BIFURCACION_PUENTE)
    {
        primeraAtajo = PUENTE_PRIMERA;
        ultimaAtajo = PUENTE_ULTIMA;
        trofeoEnLargo = trofeo >= 7 && trofeo <= 14;
    }
    else if (casilla == BIFURCACION_GALERIA)
    {
        primeraAtajo = GALERIA_PRIMERA;
        ultimaAtajo = GALERIA_ULTIMA;

        trofeoEnLargo =
            (trofeo >= 19 && trofeo <= 25) ||
            (trofeo >= 0 && trofeo <= 2);
    }
    else
    {
        return -1;
    }

    bool trofeoEnAtajo =
        trofeo >= primeraAtajo &&
        trofeo <= ultimaAtajo;

    int monedas =
        partida.jugadores[participante].monedas;

    // Con dinero para el trofeo, va hacia donde esta.
    if (monedas >= partida.costoTrofeo)
    {
        if (trofeoEnAtajo)
        {
            return 1;
        }

        if (trofeoEnLargo)
        {
            return 0;
        }
    }

    // Pobre: arriesga mas; con dinero: prefiere el anillo.
    int probabilidadAtajo =
        monedas < 8
        ? 65
        : 35;

    return GetRandomValue(0, 99) < probabilidadAtajo
        ? 1
        : 0;
}


static const char* DescribirRutaForja(
    const PartidaTablero& partida,
    int casilla,
    int opcion
)
{
    (void)partida;

    if (casilla == BIFURCACION_PUENTE)
    {
        return opcion == 0
            ? "ANILLO EXTERIOR (LARGO Y SEGURO)"
            : "PUENTE DE BASALTO (CORTO Y RIESGOSO)";
    }

    if (casilla == BIFURCACION_GALERIA)
    {
        return opcion == 0
            ? "ANILLO EXTERIOR (LARGO Y SEGURO)"
            : "GALERIA MINERA (CORTA Y RIESGOSA)";
    }

    return nullptr;
}


//==================================================
// DEFINICION
//==================================================

const DefinicionTablero& ObtenerDefinicionTableroForjaVolcan()
{
    static DefinicionTablero definicion;
    static bool preparada = false;

    if (!preparada)
    {
        definicion.id = TABLERO_FORJA_VOLCAN;
        definicion.nombre = "FORJA DEL VOLCAN";

        definicion.historia =
            "Los mineros funden el Corazon de Magma en la forja del volcan. "
            "Quien paga la fianza de excavacion se lleva un trofeo de oro fundido.";

        definicion.objetivo =
            "Reune mas trofeos (20 monedas cada uno) en 5 rondas.";

        definicion.gimmick =
            "Puente de basalto: sube y baja cada ronda y abre o cierra el atajo.";

        definicion.dificultad = "MEDIA";

        definicion.colorTema = Color{ 235, 100, 35, 255 };
        definicion.colorFondo = Color{ 40, 24, 36, 255 };

        definicion.camaraPosicion = { 0.0f, 17.5f, 18.5f };
        definicion.camaraObjetivo = { 0.0f, 0.0f, 0.0f };
        definicion.camaraFovy = 48.0f;

        definicion.casillasTrofeo = CASILLAS_TROFEO_FORJA;
        definicion.cantidadCasillasTrofeo =
            (int)(sizeof(CASILLAS_TROFEO_FORJA) /
                  sizeof(CASILLAS_TROFEO_FORJA[0]));

        definicion.costoTrofeo = 20;
        definicion.cantidadRondas = 5;

        definicion.Construir = ConstruirForjaVolcan;
        definicion.DibujarDecoracion = DibujarDecoracionForjaVolcan;
        definicion.AlIniciarRonda = AlIniciarRondaForjaVolcan;
        definicion.ResolverCasilla = ResolverCasillaForjaVolcan;
        definicion.ActualizarGimmick = ActualizarGimmickForjaVolcan;
        definicion.DibujarGimmick = DibujarGimmickForjaVolcan;
        definicion.DecidirRutaBot = DecidirRutaBotForja;
        definicion.DescribirOpcionRuta = DescribirRutaForja;

        preparada = true;
    }

    return definicion;
}
