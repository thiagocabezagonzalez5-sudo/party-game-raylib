#include "Board/CatalogoTableros.h"

#include "Gameplay/PartidaTablero.h"

#include "Board/MallaTablero.h"
#include "Systems/CalidadGrafica.h"

#include "rlgl.h"

#include <cmath>
#include <cstdio>


//==================================================
// FORJA DEL VOLCAN (tablero 2)
//==================================================
//
// Un anillo exterior de 26 casillas (ANILLO DE LA FORJA) rodea un lago de
// lava y un volcan. Dos atajos atraviesan el centro y se parten del anillo
// en una bifurcacion (opcion 0 = anillo, largo y seguro; 1 = atajo):
//
//   - PUENTE DE BASALTO (casilla 6 -> 26..29 -> 15): cruza la lava, con
//     CAOS y casillas rojas. Es el GIMMICK principal: se levanta y se baja
//     cada ronda; con el puente levantado la casilla 6 deja de ser
//     bifurcacion y el tramo 6 -> 26 queda cortado sobre la lava.
//   - GALERIA MINERA (casilla 18 -> 30..33 -> 3): siempre abierta, corta,
//     con casillas rojas y un INTERCAMBIO a la salida.
//
// Segundo gimmick: cada ronda el volcan escupe una llama sobre una casilla
// basica del anillo; quien termina su turno ahi pierde 5 monedas (10 en la
// ultima ronda: GRAN ERUPCION). El Escudo la bloquea.
//
// Casillas obligatorias (Docs/ReglasTableros.md): la TIENDA es el puesto
// del herrero junto al inicio; REGALO y DUELO en el anillo; CAOS en el
// puente; INTERCAMBIO en la galeria. Las cuatro casillas EVENTO resuelven
// el evento propio de la Forja: veta de oro, duende ladron (respeta el
// Escudo de la victima) o geiser teletransportador.
//
// Presentacion: el camino de losas de basalto entre casillas se hornea una
// vez en una malla (calzada sobre la lava en el puente y durmientes en la
// galeria); el tramo levadizo lo dibuja el gimmick. Las losas usan un
// estilo de basalto con brasas en el borde.

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
static const int PERDIDA_GRAN_ERUPCION = 10;
static const int PREMIO_VETA = 7;
static const int ROBO_DUENDE = 5;

static const float ALTURA_CASILLA_FORJA = 0.25f;
static const float VELOCIDAD_ANIMACION_PUENTE = 1.1f;

static const Vector3 CENTRO_VOLCAN = { -5.6f, 0.0f, 0.0f };

enum ZonaForja
{
    ZONA_ANILLO_FORJA = 0,
    ZONA_PUENTE_BASALTO,
    ZONA_GALERIA_MINERA,
    CANTIDAD_ZONAS_FORJA
};

static const char* const NOMBRES_ZONAS_FORJA[CANTIDAD_ZONAS_FORJA] =
{
    "ANILLO DE LA FORJA",
    "PUENTE DE BASALTO",
    "GALERIA MINERA"
};

// Candidatas del trofeo: casillas azules/verdes repartidas por el anillo
// (los atajos no: el puente puede estar levantado).
static const int CASILLAS_TROFEO_FORJA[] =
{
    1,
    5,
    7,
    9,
    14,
    19,
    21,
    25
};


// En la ultima ronda la erupcion es mayor.
static bool EsGranErupcion(
    const PartidaTablero& partida
)
{
    return partida.rondaActual >= partida.cantidadRondas;
}


static int PerdidaErupcion(
    const PartidaTablero& partida
)
{
    return EsGranErupcion(partida)
        ? PERDIDA_GRAN_ERUPCION
        : PERDIDA_ERUPCION;
}


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


// Tipo de casilla a partir de su letra en la tabla de diseno.
static TipoCasilla TipoDesdeLetraForja(char letra)
{
    switch (letra)
    {
        case '+': return CASILLA_POSITIVA;
        case '-': return CASILLA_NEGATIVA;
        case 'E': return CASILLA_ESPECIAL;
        case 'R': return CASILLA_REGALO;
        case 'C': return CASILLA_CAOS;
        case 'I': return CASILLA_INTERCAMBIO;
        case 'D': return CASILLA_DUELO;
        case 'T': return CASILLA_TIENDA;
        case 'V': return CASILLA_EVENTO;
        default: break;
    }

    return CASILLA_NEUTRA;
}


static void NodoForja(
    Tablero& tablero,
    ZonaForja zona,
    char tipo,
    Vector3 posicion
)
{
    int indice = tablero.AgregarCasilla(posicion, TipoDesdeLetraForja(tipo));

    if (indice >= 0)
    {
        tablero.casillas[indice].zona = (int)zona;
    }
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

    // Letras: + verde, - roja, n neutra, R regalo, C caos, I intercambio,
    // D duelo, T tienda, V evento de la Forja.
    //
    // Anillo (seguro, mayoria azul/verde): 0 inicio, 2 puesto del herrero
    // (tienda), 11 y 22 regalos, 17 duelo, 10 y 23 eventos.
    const char* tiposAnillo = "n+T+-n+n-+VR-+n+-D+n-+RV-+";

    // Puente de basalto 26..29: roja, evento, CAOS y verde al final.
    const char* tiposPuente = "-VC+";

    // Galeria minera 30..33: roja, evento, roja e intercambio a la salida.
    const char* tiposGaleria = "-V-I";

    for (int i = 0; i < CASILLAS_ANILLO; i++)
    {
        NodoForja(tablero, ZONA_ANILLO_FORJA, tiposAnillo[i], PosicionAnillo(i));
    }

    const float y = ALTURA_CASILLA_FORJA;

    // Puente de basalto: 26..29 (columna x = 4.5, de sur a norte).
    for (int k = 0; k <= PUENTE_ULTIMA - PUENTE_PRIMERA; k++)
    {
        NodoForja(
            tablero, ZONA_PUENTE_BASALTO, tiposPuente[k],
            Vector3{ 4.5f, y, 3.6f - 2.4f * k }
        );
    }

    // Galeria minera: 30..33 (columna x = -2.25, de norte a sur).
    for (int k = 0; k <= GALERIA_ULTIMA - GALERIA_PRIMERA; k++)
    {
        NodoForja(
            tablero, ZONA_GALERIA_MINERA, tiposGaleria[k],
            Vector3{ -2.25f, y, -3.6f + 2.4f * k }
        );
    }

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
// CAMINO DE BASALTO (malla horneada una vez)
//==================================================
//
// Una tira entre cada par de casillas conectadas, segun la zona:
//   - anillo: lecho oscuro con vetas de brasa y losas de basalto encima;
//   - puente: calzada elevada sobre la lava con pretiles;
//   - galeria: lecho de grava con durmientes de madera bajo los rieles.
// El tramo 6 -> 26 no se hornea: es el tablon del puente levadizo, que el
// gimmick dibuja bajado o levantado (camino cortado).

static const float LARGO_LOSA_CAMINO = 0.6f;
static const float RADIO_LOSA_FORJA = 0.84f;

static MallaGpu mallaCaminoForja;


// Tablero completo (todo abierto) solo para el dibujo del camino.
static const Tablero& TableroReferenciaForja()
{
    static Tablero referencia;
    static bool lista = false;

    if (!lista)
    {
        ConstruirForjaVolcan(referencia);
        lista = true;
    }

    return referencia;
}


static unsigned char SumarCanal(
    unsigned char canal,
    int variacion
)
{
    int v = canal + variacion;
    return (unsigned char)(v < 0 ? 0 : (v > 255 ? 255 : v));
}


// Variacion determinista pequena (+-10) para que las losas no se vean
// todas iguales.
static Color VariarTono(
    Color color,
    int semilla
)
{
    int variacion = ((semilla * 37 + 11) % 21) - 10;

    return Color
    {
        SumarCanal(color.r, variacion),
        SumarCanal(color.g, variacion),
        SumarCanal(color.b, variacion),
        color.a
    };
}


static void AgregarTramoCaminoForja(
    ConstructorMalla& m,
    const Casilla& a,
    const Casilla& b,
    int semilla
)
{
    float dx = b.posicion.x - a.posicion.x;
    float dz = b.posicion.z - a.posicion.z;
    float largo = std::sqrt(dx * dx + dz * dz);

    if (largo < 0.001f)
    {
        return;
    }

    // Espacio local del tramo: x avanza de a hacia b, z es el lateral.
    TransformacionMalla t;
    t.tx = a.posicion.x;
    t.tz = a.posicion.z;
    t.giroY = std::atan2(-dz, dx) * RAD2DEG;

    m.EstablecerTransformacion(t);

    bool puente =
        a.zona == ZONA_PUENTE_BASALTO ||
        b.zona == ZONA_PUENTE_BASALTO;

    bool galeria =
        a.zona == ZONA_GALERIA_MINERA ||
        b.zona == ZONA_GALERIA_MINERA;

    float mitad = largo * 0.5f;
    int losas = (int)(largo / LARGO_LOSA_CAMINO);

    if (losas < 1)
    {
        losas = 1;
    }

    float paso = largo / (float)losas;

    if (puente)
    {
        // Calzada elevada: su tapa queda por encima del borde del lago.
        m.Caja({ mitad, 0.0f, 0.0f }, { largo, 0.24f, 1.25f }, Color{ 48, 40, 44, 255 });

        for (int k = 0; k < losas; k++)
        {
            float x = paso * (k + 0.5f);

            m.Caja(
                { x, 0.14f, 0.0f },
                { paso - 0.07f, 0.04f, 1.0f },
                VariarTono(Color{ 104, 90, 92, 255 }, semilla + k)
            );

            // Pretiles solo donde no los tapa la losa de una casilla.
            if (x > RADIO_LOSA_FORJA && x < largo - RADIO_LOSA_FORJA)
            {
                for (int lado = -1; lado <= 1; lado += 2)
                {
                    m.Caja(
                        { x, 0.22f, lado * 0.56f },
                        { paso * 0.7f, 0.12f, 0.12f },
                        Color{ 66, 56, 60, 255 }
                    );
                }
            }
        }
    }
    else if (galeria)
    {
        // Grava y durmientes: los rieles de la decoracion quedan encima.
        m.Caja({ mitad, -0.05f, 0.0f }, { largo, 0.10f, 1.15f }, Color{ 70, 62, 58, 255 });

        int durmientes = (int)(largo / 0.42f);

        for (int k = 0; k < durmientes; k++)
        {
            m.Caja(
                { (largo / durmientes) * (k + 0.5f), 0.02f, 0.0f },
                { 0.16f, 0.04f, 1.05f },
                VariarTono(Color{ 112, 74, 44, 255 }, semilla + k)
            );
        }
    }
    else
    {
        // Lecho con brasas: asoma entre las losas como vetas rojizas.
        m.Caja({ mitad, -0.05f, 0.0f }, { largo, 0.10f, 1.22f }, Color{ 92, 40, 30, 255 });

        for (int k = 0; k < losas; k++)
        {
            m.Caja(
                { paso * (k + 0.5f), 0.02f, 0.0f },
                { paso - 0.08f, 0.04f, 0.96f },
                VariarTono(Color{ 86, 74, 76, 255 }, semilla + k)
            );
        }
    }

    m.QuitarTransformacion();
}


static void ConstruirMallaCaminoForja()
{
    const Tablero& t = TableroReferenciaForja();

    ConstructorMalla m;
    m.Limpiar();

    for (int i = 0; i < t.cantidadCasillas; i++)
    {
        const Casilla& a = t.casillas[i];

        for (int c = 0; c < a.cantidadConexiones; c++)
        {
            int destino = a.conexiones[c].destino;

            // El tramo levadizo lo dibuja el gimmick.
            if (i == BIFURCACION_PUENTE && destino == PUENTE_PRIMERA)
            {
                continue;
            }

            const Casilla* b = t.ObtenerCasilla(destino);

            if (b != nullptr)
            {
                AgregarTramoCaminoForja(m, a, *b, i * 7 + c * 3);
            }
        }
    }

    ActualizarMallaGpu(mallaCaminoForja, m);
}


static void DibujarCaminoForja()
{
    if (!mallaCaminoForja.cargada)
    {
        ConstruirMallaCaminoForja();
    }

    DibujarMallaGpu(mallaCaminoForja);
}


static void DescargarRecursosForjaVolcan()
{
    DescargarMalla(mallaCaminoForja);
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

    // Camino de basalto entre casillas (malla horneada).
    DibujarCaminoForja();

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

    // Gran erupcion (ultima ronda): llama mas alta y ancha.
    float escala =
        EsGranErupcion(partida)
        ? 1.45f
        : 1.0f;

    Vector3 base = casilla->posicion;
    base.y += 0.45f;

    DrawCylinder(
        base,
        0.05f,
        0.55f * escala,
        (1.5f + 0.5f * pulso) * escala,
        10,
        Color{ 255, (unsigned char)(90 + (int)(pulso * 90.0f)), 30, 255 }
    );

    Vector3 punta = base;
    punta.y += (1.6f + 0.5f * pulso) * escala;

    DrawSphere(punta, 0.16f * escala, Color{ 255, 220, 90, 255 });

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

        // Solo casillas basicas: la llama no tapa tiendas, regalos ni
        // eventos (el hook se resuelve antes que la regla comun).
        bool basica =
            casilla != nullptr &&
            (
                casilla->tipo == CASILLA_NEUTRA ||
                casilla->tipo == CASILLA_POSITIVA ||
                casilla->tipo == CASILLA_NEGATIVA
            );

        if (
            basica &&
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

    bool gran = EsGranErupcion(partida);

    std::snprintf(
        partida.estadoGimmick,
        sizeof(partida.estadoGimmick),
        "%s | %s: -%d MONEDAS",
        levantado
            ? "PUENTE LEVANTADO (ATAJO CERRADO)"
            : "PUENTE BAJADO (ATAJO ABIERTO)",
        gran
            ? "GRAN ERUPCIÓN"
            : "LLAMA EN EL ANILLO",
        PerdidaErupcion(partida)
    );

    char mensaje[96];

    if (gran)
    {
        std::snprintf(
            mensaje,
            sizeof(mensaje),
            "¡GRAN ERUPCIÓN! LA LLAMA QUITA %d. %s",
            PERDIDA_GRAN_ERUPCION,
            levantado
                ? "PUENTE LEVANTADO"
                : "PUENTE BAJADO"
        );
    }
    else
    {
        std::snprintf(
            mensaje,
            sizeof(mensaje),
            "%s",
            levantado
                ? "¡EL PUENTE SE LEVANTA! ATAJO CERRADO"
                : "¡EL PUENTE BAJA! ATAJO ABIERTO"
        );
    }

    EstablecerMensajeGimmick(partida, mensaje);
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


// Casilla EVENTO de la Forja: veta de oro, duende ladron o geiser.
static void ResolverEventoForja(
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

        // El Escudo de la victima frena al duende: no roba nada.
        if (robo > 0 && EscudoAbsorbePerdida(partida, rival))
        {
            partida.ultimoEventoEspecial = EVENTO_TABLERO_NINGUNO;
            partida.variacionMonedasEvento = 0;

            std::snprintf(
                partida.textoEvento,
                sizeof(partida.textoEvento),
                "¡EL ESCUDO DE J%d FRENA AL DUENDE LADRÓN!",
                rival + 1
            );

            return;
        }

        victima.monedas -= robo;
        jugador.monedas += robo;

        partida.ultimoEventoEspecial = EVENTO_TABLERO_INTERCAMBIO;
        partida.variacionMonedasEvento = robo;
        partida.jugadorIntercambioEvento = rival;

        std::snprintf(
            partida.textoEvento,
            sizeof(partida.textoEvento),
            "DUENDE LADRÓN: J%d ROBA %d MONEDAS A J%d",
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
            "GÉISER: J%d SALE DISPARADO A OTRA CASILLA",
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

    if (casilla->tipo == CASILLA_EVENTO)
    {
        ResolverEventoForja(partida, participante);
        return true;
    }

    if (jugador.casillaActual == partida.casillaPeligro)
    {
        int quemadura = PerdidaErupcion(partida);

        int perdida =
            jugador.monedas < quemadura
            ? jugador.monedas
            : quemadura;

        if (perdida > 0 && EscudoAbsorbePerdida(partida, participante))
        {
            partida.variacionMonedasEvento = 0;

            std::snprintf(
                partida.textoEvento,
                sizeof(partida.textoEvento),
                "¡EL ESCUDO DE J%d BLOQUEA LA LLAMA!",
                participante + 1
            );

            ReproducirSonidoTablero(partida, SONIDO_EVENTO_TABLERO);

            return true;
        }

        jugador.monedas -= perdida;
        partida.variacionMonedasEvento = -perdida;

        std::snprintf(
            partida.textoEvento,
            sizeof(partida.textoEvento),
            "%s LA LLAMA TE QUEMA: -%d MONEDAS",
            EsGranErupcion(partida)
                ? "¡GRAN ERUPCIÓN!"
                : "¡ERUPCIÓN!",
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

// Valor aproximado de pisar una casilla (para que los bots elijan ruta).
static int ValorCasillaBotForja(
    const PartidaTablero& partida,
    int indice
)
{
    const Casilla* casilla = partida.tablero.ObtenerCasilla(indice);

    if (casilla == nullptr)
    {
        return 0;
    }

    // La llama del volcan manda sobre el tipo de la casilla.
    if (indice == partida.casillaPeligro)
    {
        return -PerdidaErupcion(partida);
    }

    // Sin "default": un tipo nuevo de casilla obliga a puntuarlo aqui.
    switch (casilla->tipo)
    {
        case CASILLA_POSITIVA: return MONEDAS_CASILLA_POSITIVA;
        case CASILLA_NEGATIVA: return -MONEDAS_CASILLA_NEGATIVA;
        case CASILLA_ESPECIAL: return 1;
        case CASILLA_REGALO: return MONEDAS_REGALO;
        case CASILLA_CAOS: return -4;           // ruleta de castigos
        case CASILLA_INTERCAMBIO: return -1;    // azar: algo arriesgada
        case CASILLA_EVENTO: return 3;          // veta, duende o geiser: casi siempre suma
        case CASILLA_DUELO: return 0;           // apuesta voluntaria: ni premio ni castigo
        case CASILLA_TIENDA: return 1;          // compra objetos: algo util, no un premio
        case CASILLA_NEUTRA: return 0;
        case CANTIDAD_TIPOS_CASILLA: return 0;
    }

    return 0;
}


// Valor de los proximos pasos desde una casilla (media de las ramas).
static float ValorRutaBotForja(
    const PartidaTablero& partida,
    int casilla,
    int profundidad
)
{
    const Casilla* actual = partida.tablero.ObtenerCasilla(casilla);

    if (actual == nullptr || profundidad <= 0)
    {
        return 0.0f;
    }

    float resto = 0.0f;

    for (int c = 0; c < actual->cantidadConexiones; c++)
    {
        resto +=
            ValorRutaBotForja(
                partida, actual->conexiones[c].destino, profundidad - 1
            );
    }

    if (actual->cantidadConexiones > 0)
    {
        resto /= (float)actual->cantidadConexiones;
    }

    return (float)ValorCasillaBotForja(partida, casilla) + resto;
}


// Con monedas para el trofeo, el bot toma el camino mas corto hasta el;
// sin ellas elige la rama con mejores casillas por delante. Siempre deja
// algo de azar (y ante un empate elige al azar).
static int DecidirRutaBotForja(
    const PartidaTablero& partida,
    int participante
)
{
    const EstadoJugadorPartidaTablero& jugador =
        partida.jugadores[participante];

    const Casilla* actual =
        partida.tablero.ObtenerCasilla(jugador.casillaActual);

    if (actual == nullptr || actual->cantidadConexiones < 2)
    {
        return -1;
    }

    if (GetRandomValue(0, 99) < 20)
    {
        return -1;
    }

    int mejor = -1;

    if (
        jugador.monedas >= partida.costoTrofeo &&
        partida.casillaTrofeo >= 0
    )
    {
        int mejorPasos = 1000000;

        for (int c = 0; c < actual->cantidadConexiones; c++)
        {
            int pasos =
                DistanciaEntreCasillas(
                    partida,
                    actual->conexiones[c].destino,
                    partida.casillaTrofeo
                );

            if (pasos >= 0 && pasos < mejorPasos)
            {
                mejorPasos = pasos;
                mejor = c;
            }
            else if (pasos >= 0 && pasos == mejorPasos)
            {
                mejor = -1;
            }
        }

        return mejor;
    }

    float mejorValor = -1.0e9f;

    for (int c = 0; c < actual->cantidadConexiones; c++)
    {
        float valor =
            ValorRutaBotForja(
                partida, actual->conexiones[c].destino, 5
            );

        if (valor > mejorValor + 0.01f)
        {
            mejorValor = valor;
            mejor = c;
        }
        else if (valor > mejorValor - 0.01f)
        {
            mejor = -1;
        }
    }

    return mejor;
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
// ESTILO DE LAS LOSAS
//==================================================

// Losas de basalto oscuro con brasas en el borde (en lugar de musgo).
// Mismas proporciones que la piedra comun: el camino horneado cuenta con
// RADIO_LOSA_FORJA.
static EstiloCasilla ObtenerEstiloCasillaBasalto()
{
    EstiloCasilla estilo = ObtenerEstiloCasillaPiedra();

    estilo.piedraLado = Color{ 64, 54, 58, 255 };
    estilo.piedraTapa = Color{ 108, 96, 98, 255 };
    estilo.borde = Color{ 34, 28, 30, 255 };
    estilo.detalle = Color{ 214, 96, 40, 255 };

    estilo.sendero = Color{ 86, 74, 76, 255 };
    estilo.senderoBorde = Color{ 48, 40, 44, 255 };

    return estilo;
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
        definicion.nombre = "FORJA DEL VOLCÁN";

        definicion.historia =
            "Los mineros funden el Corazón de Magma en la forja del volcán. "
            "Quien paga la fianza de excavación se lleva un trofeo de oro fundido.";

        definicion.objetivo =
            "Reuní más trofeos (20 monedas cada uno) en 5 rondas.";

        definicion.gimmick =
            "Puente levadizo: abre o cierra el atajo cada ronda. "
            "La llama del volcán quita 5 monedas (10 en la última ronda).";

        definicion.dificultad = "MEDIA";

        definicion.colorTema = Color{ 235, 100, 35, 255 };
        definicion.colorFondo = Color{ 40, 24, 36, 255 };

        definicion.camaraPosicion = { 0.0f, 17.5f, 18.5f };
        definicion.camaraObjetivo = { 0.0f, 0.0f, 0.0f };
        definicion.camaraFovy = 48.0f;

        // Landmark: el puente levadizo. El sistema comun lo enfoca cuando
        // gimmickActivo (puente levantado) y hay aviso de ronda, asi que la
        // camara muestra justo el mecanismo que acaba de cerrar el atajo.
        definicion.camaraLandmark = { 4.5f, 1.0f, 4.6f };
        definicion.tieneLandmark = true;

        definicion.estiloCasilla = ObtenerEstiloCasillaBasalto();

        definicion.nombresZonas = NOMBRES_ZONAS_FORJA;
        definicion.cantidadZonas = CANTIDAD_ZONAS_FORJA;

        definicion.casillasTrofeo = CASILLAS_TROFEO_FORJA;
        definicion.cantidadCasillasTrofeo =
            (int)(sizeof(CASILLAS_TROFEO_FORJA) /
                  sizeof(CASILLAS_TROFEO_FORJA[0]));

        definicion.costoTrofeo = 20;
        definicion.cantidadRondas = 5;

        definicion.Construir = ConstruirForjaVolcan;
        definicion.DibujarDecoracion = DibujarDecoracionForjaVolcan;
        definicion.DescargarRecursos = DescargarRecursosForjaVolcan;
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
