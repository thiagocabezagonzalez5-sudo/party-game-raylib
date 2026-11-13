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
// Mapa de 57 casillas en seis zonas alrededor de un gran lago de lava, al
// pie de un volcan (ver RUTA DEL TABLERO). En cada bifurcacion la opcion 0
// es la ruta larga y la 1 el atajo:
//
//   - ANILLO DE LA FORJA: circuito de 18 casillas que rodea el lago.
//   - CAMPAMENTO MINERO (oeste): lobulo con el inicio y el puesto del
//     herrero (tienda); el anillo lo acorta por la orilla.
//   - PUENTE DE BASALTO: atajo levadizo que cruza el lago de sur a norte.
//     Es el GIMMICK principal: se levanta en las rondas pares y su
//     bifurcacion pierde la conexion 1 (el tablon queda alzado sobre la
//     lava).
//   - LAS FUMAROLAS (norte): lobulo arriesgado al pie del volcan, con el
//     CAOS y mas casillas rojas; dentro se parte otra vez (al pie del
//     volcan o entre las fumarolas).
//   - MINA DE CRISTAL (este): lobulo con cristales gigantes, la segunda
//     tienda y un duelo; tambien se parte (vetas o caverna).
//   - GALERIA MINERA: el tramo este del anillo corre bajo la cresta, sobre
//     rieles; es el atajo que evita la mina.
//
// Segundo gimmick: cada ronda el volcan escupe DOS llamas, una en cada
// mitad del mapa, siempre sobre casillas basicas. Quien termina su turno
// en una pierde 5 monedas (10 en la ultima ronda: GRAN ERUPCION). El
// Escudo la bloquea. La llama oeste usa partida.casillaPeligro; la este,
// un estado propio que se reinicia al construir el tablero.
//
// Las casillas EVENTO resuelven el evento propio de la Forja: veta de oro,
// duende ladron (respeta el Escudo de la victima) o geiser (lanza al
// jugador a una casilla basica cualquiera).
//
// PRESENTACION (diorama volcanico): terreno de basalto horneado con el
// volcan, el lago hundido, mesetas, acantilados y grietas incandescentes;
// mar de lava y cordillera en el horizonte; sendero de losas horneado por
// zona; props horneados (rocas, escoria, cristales, andamios, vagonetas,
// forjas, carpas...) y en modo inmediato solo lo animado (lava, humo,
// brasas, llamas y el puente levadizo).

static const int PERDIDA_ERUPCION = 5;
static const int PERDIDA_GRAN_ERUPCION = 10;
static const int PREMIO_VETA = 7;
static const int ROBO_DUENDE = 5;

static const float ALTURA_CASILLA_FORJA = 0.25f;
static const float VELOCIDAD_ANIMACION_PUENTE = 1.1f;

// Centro de la caja de casillas: divide el mapa en mitad oeste y este
// (una llama en cada una) y es el objetivo de la camara.
static const float CENTRO_CASILLAS_X = 1.1f;
static const float CENTRO_CASILLAS_Z = -7.8f;


//==================================================
// UTILIDADES MATEMATICAS Y DE COLOR
//==================================================

static float Limitar01(float t)
{
    if (t < 0.0f) return 0.0f;
    if (t > 1.0f) return 1.0f;
    return t;
}


static float Suave01(float t)
{
    t = Limitar01(t);
    return t * t * (3.0f - 2.0f * t);
}


static Color Mezcla(Color a, Color b, float t)
{
    t = Limitar01(t);

    return Color
    {
        (unsigned char)(a.r + (b.r - a.r) * t),
        (unsigned char)(a.g + (b.g - a.g) * t),
        (unsigned char)(a.b + (b.b - a.b) * t),
        255
    };
}


static Color Oscurecer(Color c, float factor)
{
    float r = c.r * factor;
    float g = c.g * factor;
    float b = c.b * factor;

    return Color
    {
        (unsigned char)(r > 255.0f ? 255.0f : r),
        (unsigned char)(g > 255.0f ? 255.0f : g),
        (unsigned char)(b > 255.0f ? 255.0f : b),
        c.a
    };
}


static Color ConAlfa(Color c, float alfa)
{
    c.a = (unsigned char)(255.0f * Limitar01(alfa));
    return c;
}


static float Hash2(int a, int b)
{
    unsigned int h =
        ((unsigned int)a * 374761393u + (unsigned int)b * 668265263u);

    h = (h ^ (h >> 13)) * 1274126177u;

    return (float)((h ^ (h >> 16)) & 0xFFFF) / 65535.0f;
}


static float Parpadeo(
    float tiempo,
    float velocidad
)
{
    return 0.5f + 0.5f * std::sin(tiempo * velocidad);
}


//==================================================
// RUTA DEL TABLERO (datos de la logica)
//==================================================
//
// Las posiciones salen de un diseno validado (distancias 3.0-4.2 entre
// casillas conectadas, sin caminos que se toquen y nunca mas de 5 casillas
// seguidas sin especial). Los indices que el gimmick y el dibujo necesitan
// se guardan en nodosForja al construir: nada depende de un literal.

enum ZonaForja
{
    ZONA_CAMPAMENTO = 0,
    ZONA_ANILLO_FORJA,
    ZONA_PUENTE_BASALTO,
    ZONA_GALERIA_MINERA,
    ZONA_FUMAROLAS,
    ZONA_MINA_CRISTAL,
    CANTIDAD_ZONAS_FORJA
};

static const char* const NOMBRES_ZONAS_FORJA[CANTIDAD_ZONAS_FORJA] =
{
    "CAMPAMENTO MINERO",
    "ANILLO DE LA FORJA",
    "PUENTE DE BASALTO",
    "GALERIA MINERA",
    "LAS FUMAROLAS",
    "MINA DE CRISTAL"
};

static const int CASILLAS_ANILLO_FORJA = 18;
static const int CASILLAS_PUENTE_FORJA = 4;
static const int CASILLAS_CAMPAMENTO_FORJA = 10;
static const int MAX_TROFEOS_FORJA = 16;

// Casillas clave (las llena ConstruirRutaForja).
struct NodosForja
{
    int inicio = 0;

    int anillo[CASILLAS_ANILLO_FORJA] = {};
    int campamento[CASILLAS_CAMPAMENTO_FORJA] = {};
    int puente[CASILLAS_PUENTE_FORJA] = {};

    // Bifurcacion del puente levadizo (su conexion 1 es el tablon) y
    // casilla donde el puente vuelve al anillo.
    int puenteBifurcacion = 0;
    int puenteUnion = 0;

    // Primeras y ultimas casillas de la galeria (bocas del tunel).
    int galeriaPrimera = 0;
    int galeriaUltima = 0;

    // Casillas del lazo exterior de las fumarolas que cruza el rio de
    // lava (el puentecito va entre ellas).
    int rioFumarolasA = 0;
    int rioFumarolasB = 0;
};

static NodosForja nodosForja;

static int casillasTrofeoForja[MAX_TROFEOS_FORJA];
static int cantidadTrofeosForja = 0;

static const char* descripcionRutaForja[MAX_CASILLAS_TABLERO][2];

// Llama de la mitad este (la oeste es partida.casillaPeligro). Se reinicia
// en ConstruirForjaVolcan, no en ConstruirRutaForja: el tablero de
// referencia del dibujo tambien construye la ruta.
static int llamaEsteForja = -1;


struct DatoNodoForja
{
    char tipo;
    float x;
    float z;
};


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


static int NodoForja(
    Tablero& tablero,
    ZonaForja zona,
    const DatoNodoForja& dato
)
{
    int indice =
        tablero.AgregarCasilla(
            Vector3{ dato.x, ALTURA_CASILLA_FORJA, dato.z },
            TipoDesdeLetraForja(dato.tipo)
        );

    if (indice >= 0)
    {
        tablero.casillas[indice].zona = (int)zona;
    }

    return indice;
}


// Crea una tira de casillas de la misma zona y guarda sus indices.
static void TramoForja(
    Tablero& tablero,
    ZonaForja zona,
    const DatoNodoForja* datos,
    int cantidad,
    int* indices
)
{
    for (int i = 0; i < cantidad; i++)
    {
        indices[i] = NodoForja(tablero, zona, datos[i]);
    }
}


// Conecta en cadena: indices[0] -> indices[1] -> ... -> indices[n - 1].
static void EncadenarForja(
    Tablero& tablero,
    const int* indices,
    int cantidad
)
{
    for (int i = 0; i + 1 < cantidad; i++)
    {
        tablero.ConectarCasillas(indices[i], indices[i + 1]);
    }
}


static void DefinirOpcionesForja(
    int casilla,
    const char* opcion0,
    const char* opcion1
)
{
    if (casilla < 0 || casilla >= MAX_CASILLAS_TABLERO)
    {
        return;
    }

    descripcionRutaForja[casilla][0] = opcion0;
    descripcionRutaForja[casilla][1] = opcion1;
}


static void ConstruirRutaForja(Tablero& t)
{
    t.cantidadCasillas = 0;
    t.recorridoValido = false;

    for (int i = 0; i < MAX_CASILLAS_TABLERO; i++)
    {
        t.casillas[i] = Casilla{};
        descripcionRutaForja[i][0] = nullptr;
        descripcionRutaForja[i][1] = nullptr;
    }

    cantidadTrofeosForja = 0;

    NodosForja& n = nodosForja;

    // Letras: + verde, - roja, n neutra, E especial, R regalo, C caos,
    // I intercambio, D duelo, T tienda, V evento de la Forja.

    // --- CAMPAMENTO: el inicio va primero (casilla 0) ---
    n.inicio = NodoForja(t, ZONA_CAMPAMENTO, DatoNodoForja{ 'n', -18.30f, 7.95f });

    // --- ANILLO: r0 abajo (cabecera sur del puente) y hacia el oeste ---
    static const DatoNodoForja ANILLO[CASILLAS_ANILLO_FORJA] =
    {
        { '+',   0.00f,   6.20f },   // r0: bifurcacion del puente
        { '+',  -3.39f,   5.82f },
        { '-',  -6.57f,   4.64f },   // r2: salida al campamento
        { '+',  -9.27f,   2.60f },
        { 'E', -10.96f,  -0.32f },
        { '-', -10.96f,  -3.68f },
        { '+',  -9.27f,  -6.60f },   // r6: vuelta del campamento
        { 'V',  -6.56f,  -8.65f },
        { 'n',  -3.38f,  -9.82f },
        { 'E',   0.00f, -10.20f },   // r9: cabecera norte, salida a las fumarolas
        { '+',   3.39f,  -9.82f },
        { 'D',   6.57f,  -8.64f },
        { '+',   9.27f,  -6.60f },   // r12: vuelta de las fumarolas
        { '+',  10.96f,  -3.68f },   // r13: boca norte de la galeria, salida a la mina
        { '-',  10.96f,  -0.32f },
        { 'V',   9.27f,   2.60f },
        { '-',   6.56f,   4.65f },
        { 'I',   3.38f,   5.82f }    // r17: boca sur, vuelta de la mina
    };

    int* r = n.anillo;

    for (int k = 0; k < CASILLAS_ANILLO_FORJA; k++)
    {
        ZonaForja zona = ZONA_ANILLO_FORJA;

        if (k == 0 || k == 9)
        {
            zona = ZONA_PUENTE_BASALTO;
        }
        else if (k >= 13)
        {
            zona = ZONA_GALERIA_MINERA;
        }

        r[k] = NodoForja(t, zona, ANILLO[k]);
    }

    // --- CAMPAMENTO: de r2 por la orilla oeste hasta r6 (c3 = inicio) ---
    static const DatoNodoForja CAMPAMENTO[CASILLAS_CAMPAMENTO_FORJA] =
    {
        { '+',  -8.82f,   7.40f },
        { 'R', -11.75f,   9.05f },
        { '-', -15.19f,   9.69f },
        { 'n', -18.30f,   7.95f },   // inicio (ya creado)
        { 'T', -20.46f,   5.46f },   // puesto del herrero
        { '+', -21.47f,   2.05f },
        { 'V', -20.59f,  -1.31f },
        { '+', -18.91f,  -4.30f },
        { 'n', -16.11f,  -6.49f },
        { 'I', -12.81f,  -6.96f }
    };

    int* c = n.campamento;

    for (int k = 0; k < CASILLAS_CAMPAMENTO_FORJA; k++)
    {
        c[k] =
            k == 3
            ? n.inicio
            : NodoForja(t, ZONA_CAMPAMENTO, CAMPAMENTO[k]);
    }

    // --- PUENTE DE BASALTO: de r0 a r9 sobre el lago ---
    static const DatoNodoForja PUENTE[CASILLAS_PUENTE_FORJA] =
    {
        { '-', 0.0f,  2.40f },
        { 'V', 0.0f, -0.90f },
        { '-', 0.0f, -4.15f },
        { '+', 0.0f, -7.20f }
    };

    TramoForja(t, ZONA_PUENTE_BASALTO, PUENTE, CASILLAS_PUENTE_FORJA, n.puente);

    // --- LAS FUMAROLAS: de r9 al pie del volcan y vuelta a r12 ---
    static const DatoNodoForja FUMAROLAS_SUBIDA[2] =
    {
        { '-', -0.30f, -13.80f },
        { '+', -0.60f, -17.40f }     // fA: al pie del volcan / entre fumarolas
    };

    static const DatoNodoForja FUMAROLAS_EXTERIOR[5] =
    {
        { 'V', -1.58f, -21.43f },
        { '-',  1.07f, -24.62f },
        { 'C',  5.09f, -25.53f },
        { 'R',  9.00f, -24.34f },
        { '+', 11.34f, -21.00f }
    };

    static const DatoNodoForja FUMAROLAS_INTERIOR[2] =
    {
        { '-', 3.00f, -17.27f },
        { '-', 6.60f, -17.13f }
    };

    static const DatoNodoForja FUMAROLAS_BAJADA[3] =
    {
        { 'E', 10.20f, -17.00f },    // fB: union de los dos caminos
        { '-', 12.23f, -13.67f },
        { '+', 10.90f, -10.14f }
    };

    int fs[2];
    int fo[5];
    int fi[2];
    int fb[3];

    TramoForja(t, ZONA_FUMAROLAS, FUMAROLAS_SUBIDA, 2, fs);
    TramoForja(t, ZONA_FUMAROLAS, FUMAROLAS_EXTERIOR, 5, fo);
    TramoForja(t, ZONA_FUMAROLAS, FUMAROLAS_INTERIOR, 2, fi);
    TramoForja(t, ZONA_FUMAROLAS, FUMAROLAS_BAJADA, 3, fb);

    // --- MINA DE CRISTAL: de r13 al este y vuelta a r17 ---
    static const DatoNodoForja MINA_ENTRADA[2] =
    {
        { '+', 14.48f, -4.54f },
        { 'T', 18.00f, -5.40f }      // mC: vetas / caverna (tienda de los mineros)
    };

    static const DatoNodoForja MINA_EXTERIOR[4] =
    {
        { '+', 21.73f, -5.83f },
        { 'V', 23.44f, -2.08f },
        { 'D', 23.60f,  2.12f },
        { '+', 21.55f,  5.78f }
    };

    static const DatoNodoForja MINA_CAVERNA[2] =
    {
        { '-', 17.93f, -1.73f },
        { '+', 17.87f,  1.93f }
    };

    static const DatoNodoForja MINA_SALIDA[5] =
    {
        { 'E', 17.80f, 5.60f },      // mD: union de los dos caminos
        { 'R', 15.28f, 8.21f },
        { 'n', 11.90f, 9.56f },
        { '-',  8.30f, 9.99f },
        { '+',  4.79f, 9.17f }
    };

    int me[2];
    int mx[4];
    int mk[2];
    int ms[5];

    TramoForja(t, ZONA_MINA_CRISTAL, MINA_ENTRADA, 2, me);
    TramoForja(t, ZONA_MINA_CRISTAL, MINA_EXTERIOR, 4, mx);
    TramoForja(t, ZONA_MINA_CRISTAL, MINA_CAVERNA, 2, mk);
    TramoForja(t, ZONA_MINA_CRISTAL, MINA_SALIDA, 5, ms);

    // --- Conexiones (opcion 0 primero) ---

    // Anillo: r0 -> r1 -> ... -> r17 -> r0, con las salidas a los lobulos
    // antes del paso por el anillo donde el lobulo es la ruta larga.
    for (int k = 0; k < CASILLAS_ANILLO_FORJA; k++)
    {
        int siguiente = r[(k + 1) % CASILLAS_ANILLO_FORJA];

        if (k == 2)
        {
            t.ConectarCasillas(r[2], c[0]);
        }
        else if (k == 9)
        {
            t.ConectarCasillas(r[9], fs[0]);
        }
        else if (k == 13)
        {
            t.ConectarCasillas(r[13], me[0]);
        }

        t.ConectarCasillas(r[k], siguiente);

        // Puente: la conexion 1 de r0 es el tablon levadizo.
        if (k == 0)
        {
            t.ConectarCasillas(r[0], n.puente[0]);
        }
    }

    // Campamento.
    EncadenarForja(t, c, CASILLAS_CAMPAMENTO_FORJA);
    t.ConectarCasillas(c[CASILLAS_CAMPAMENTO_FORJA - 1], r[6]);

    // Puente.
    EncadenarForja(t, n.puente, CASILLAS_PUENTE_FORJA);
    t.ConectarCasillas(n.puente[CASILLAS_PUENTE_FORJA - 1], r[9]);

    // Fumarolas: al pie del volcan (largo) o entre las fumarolas (corto).
    t.ConectarCasillas(fs[0], fs[1]);
    t.ConectarCasillas(fs[1], fo[0]);
    t.ConectarCasillas(fs[1], fi[0]);
    EncadenarForja(t, fo, 5);
    t.ConectarCasillas(fo[4], fb[0]);
    EncadenarForja(t, fi, 2);
    t.ConectarCasillas(fi[1], fb[0]);
    EncadenarForja(t, fb, 3);
    t.ConectarCasillas(fb[2], r[12]);

    // Mina: por las vetas (largo) o por la caverna de cristal (corto).
    t.ConectarCasillas(me[0], me[1]);
    t.ConectarCasillas(me[1], mx[0]);
    t.ConectarCasillas(me[1], mk[0]);
    EncadenarForja(t, mx, 4);
    t.ConectarCasillas(mx[3], ms[0]);
    EncadenarForja(t, mk, 2);
    t.ConectarCasillas(mk[1], ms[0]);
    EncadenarForja(t, ms, 5);
    t.ConectarCasillas(ms[4], r[17]);

    // --- Casillas clave para el gimmick y el dibujo ---
    n.puenteBifurcacion = r[0];
    n.puenteUnion = r[9];
    n.galeriaPrimera = r[13];
    n.galeriaUltima = r[17];
    n.rioFumarolasA = fo[2];
    n.rioFumarolasB = fo[3];

    // --- Textos de las bifurcaciones ---
    DefinirOpcionesForja(r[0], "SEGUIR EL ANILLO (LARGO)", "PUENTE DE BASALTO (ATAJO)");
    DefinirOpcionesForja(r[2], "CAMPAMENTO MINERO (LARGO)", "SEGUIR EL ANILLO (CORTO)");
    DefinirOpcionesForja(r[9], "LAS FUMAROLAS (ARRIESGADO)", "SEGUIR EL ANILLO (CORTO)");
    DefinirOpcionesForja(r[13], "MINA DE CRISTAL (LARGO)", "GALERIA MINERA (ATAJO)");
    DefinirOpcionesForja(fs[1], "AL PIE DEL VOLCÁN (LARGO)", "ENTRE FUMAROLAS (CORTO)");
    DefinirOpcionesForja(me[1], "VETAS DE CRISTAL (LARGO)", "CAVERNA DE CRISTAL (CORTO)");

    // --- Candidatas del trofeo: 2-3 por zona, nunca sobre el puente ---
    const int candidatas[] =
    {
        c[2], c[7],
        r[3], r[7], r[11],
        r[0], r[9],
        r[14], r[16],
        fo[1], fo[3], fb[1],
        mx[0], mx[3], ms[2]
    };

    for (int indice : candidatas)
    {
        if (cantidadTrofeosForja < MAX_TROFEOS_FORJA)
        {
            casillasTrofeoForja[cantidadTrofeosForja++] = indice;
        }
    }

    // Se valida con todo abierto.
    t.recorridoValido = t.ValidarRecorrido();
}


static void ConstruirForjaVolcan(
    Tablero& tablero
)
{
    ConstruirRutaForja(tablero);

    // Estado propio del gimmick (la llama oeste vive en la partida).
    llamaEsteForja = -1;
}


// Tablero completo (todo abierto) solo para el dibujo.
static const Tablero& TableroReferenciaForja()
{
    static Tablero referencia;
    static bool lista = false;

    if (!lista)
    {
        ConstruirRutaForja(referencia);
        lista = true;
    }

    return referencia;
}


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


static bool EsLlamaForja(
    const PartidaTablero& partida,
    int indice
)
{
    return indice >= 0 &&
        (indice == partida.casillaPeligro || indice == llamaEsteForja);
}


static bool EsCasillaBasicaForja(
    const Casilla* casilla
)
{
    return casilla != nullptr &&
        (
            casilla->tipo == CASILLA_NEUTRA ||
            casilla->tipo == CASILLA_POSITIVA ||
            casilla->tipo == CASILLA_NEGATIVA
        );
}


//==================================================
// GEOGRAFIA DEL DIORAMA
//==================================================
//
// Todo en coordenadas de mundo (las mismas de las casillas). La lava es un
// plano global a NIVEL_LAVA: aparece donde el terreno se hunde (lago, canal
// del puente, rios, poza de las fumarolas y el mar alrededor de la isla).

static const float NIVEL_LAVA = -0.35f;
static const float FONDO_CAUCE = -1.0f;

// Volcan (landmark principal): cono con crestas y crater.
static const float VOLCAN_X = 5.0f;
static const float VOLCAN_Z = -37.0f;
static const float VOLCAN_RADIO = 14.0f;
static const float VOLCAN_ALTURA = 18.0f;
static const float VOLCAN_CRATER = 2.8f;

// Lago de lava dentro del anillo y canal recto bajo el puente.
static const float LAGO_X = 0.0f;
static const float LAGO_Z = -2.0f;
static const float LAGO_RX = 8.0f;
static const float LAGO_RZ = 5.0f;
static const float CANAL_SEMIANCHO = 1.4f;
static const float CANAL_Z_NORTE = -7.6f;
static const float CANAL_Z_SUR = 3.4f;

// Poza de las fumarolas (entre el lazo exterior y el interior).
static const float POZA_X = 5.0f;
static const float POZA_Z = -21.1f;
static const float POZA_RADIO = 1.25f;

struct PuntoRioForja
{
    float x;
    float z;
};

struct RioForja
{
    const PuntoRioForja* puntos;
    int cantidad;
    float semiancho;
};

// Rios de lava: dos bajan del pie del volcan al mar (sin cruzar caminos) y
// uno corto alimenta la poza de las fumarolas, pasando bajo el lazo
// exterior entre la casilla del CAOS y la del regalo (puentecito).
static const PuntoRioForja PUNTOS_RIO_OESTE[] =
{
    { -6.0f, -30.0f }, { -13.0f, -28.0f }, { -21.0f, -25.5f }, { -29.0f, -24.0f }, { -44.0f, -24.0f }
};

static const PuntoRioForja PUNTOS_RIO_ESTE[] =
{
    { 14.0f, -31.0f }, { 20.0f, -27.0f }, { 27.0f, -24.0f }, { 34.0f, -22.5f }, { 48.0f, -22.0f }
};

static const PuntoRioForja PUNTOS_RIO_FUMAROLAS[] =
{
    { 7.8f, -28.6f }, { 7.3f, -26.6f }, { 7.05f, -24.9f }, { 5.8f, -22.6f }, { POZA_X, POZA_Z }
};

static const RioForja RIOS_FORJA[] =
{
    { PUNTOS_RIO_OESTE, 5, 1.15f },
    { PUNTOS_RIO_ESTE, 5, 1.15f },
    { PUNTOS_RIO_FUMAROLAS, 5, 0.55f }
};

static const int CANTIDAD_RIOS_FORJA =
    (int)(sizeof(RIOS_FORJA) / sizeof(RIOS_FORJA[0]));


// Distancia de (x, z) al segmento ab y parametro del punto mas cercano.
static float DistanciaSegmento(
    float x, float z,
    float ax, float az,
    float bx, float bz,
    float* parametro = nullptr
)
{
    float dx = bx - ax;
    float dz = bz - az;
    float largo2 = dx * dx + dz * dz;

    float k =
        largo2 > 0.0001f
        ? ((x - ax) * dx + (z - az) * dz) / largo2
        : 0.0f;

    k = Limitar01(k);

    if (parametro != nullptr)
    {
        *parametro = k;
    }

    float cx = ax + dx * k - x;
    float cz = az + dz * k - z;

    return std::sqrt(cx * cx + cz * cz);
}


// Distancia al sendero mas cercano (segmentos entre casillas conectadas).
static float DistanciaSenderoForja(
    float x,
    float z
)
{
    const Tablero& t = TableroReferenciaForja();
    float mejor = 1000.0f;

    for (int i = 0; i < t.cantidadCasillas; i++)
    {
        const Casilla& a = t.casillas[i];

        for (int c = 0; c < a.cantidadConexiones; c++)
        {
            const Casilla* b = t.ObtenerCasilla(a.conexiones[c].destino);

            if (b == nullptr)
            {
                continue;
            }

            // Descarte rapido por caja.
            float minX = (a.posicion.x < b->posicion.x ? a.posicion.x : b->posicion.x) - mejor;
            float maxX = (a.posicion.x > b->posicion.x ? a.posicion.x : b->posicion.x) + mejor;
            float minZ = (a.posicion.z < b->posicion.z ? a.posicion.z : b->posicion.z) - mejor;
            float maxZ = (a.posicion.z > b->posicion.z ? a.posicion.z : b->posicion.z) + mejor;

            if (x < minX || x > maxX || z < minZ || z > maxZ)
            {
                continue;
            }

            float d =
                DistanciaSegmento(
                    x, z,
                    a.posicion.x, a.posicion.z,
                    b->posicion.x, b->posicion.z
                );

            if (d < mejor)
            {
                mejor = d;
            }
        }
    }

    return mejor;
}


// Distancia al cauce de un rio (0 en el eje).
static float DistanciaRioForja(
    const RioForja& rio,
    float x,
    float z
)
{
    float mejor = 1000.0f;

    for (int i = 0; i + 1 < rio.cantidad; i++)
    {
        float d =
            DistanciaSegmento(
                x, z,
                rio.puntos[i].x, rio.puntos[i].z,
                rio.puntos[i + 1].x, rio.puntos[i + 1].z
            );

        if (d < mejor)
        {
            mejor = d;
        }
    }

    return mejor;
}


// Cuanto le falta a (x, z) para salir de un cauce de lava (<= 0 = dentro).
// Cubre lago, canal, rios y poza.
static float BordeLavaForja(
    float x,
    float z
)
{
    // Lago (aproximacion en metros de la elipse).
    float ex = (x - LAGO_X) / LAGO_RX;
    float ez = (z - LAGO_Z) / LAGO_RZ;
    float lago = (std::sqrt(ex * ex + ez * ez) - 1.0f) * LAGO_RZ;

    // Canal del puente (caja).
    float canal = std::fabs(x - LAGO_X) - CANAL_SEMIANCHO;
    float canalZ = z - CANAL_Z_SUR;

    if (CANAL_Z_NORTE - z > canalZ)
    {
        canalZ = CANAL_Z_NORTE - z;
    }

    if (canalZ > canal)
    {
        canal = canalZ;
    }

    float borde = lago < canal ? lago : canal;

    for (int i = 0; i < CANTIDAD_RIOS_FORJA; i++)
    {
        float rio =
            DistanciaRioForja(RIOS_FORJA[i], x, z) - RIOS_FORJA[i].semiancho;

        if (rio < borde)
        {
            borde = rio;
        }
    }

    float dx = x - POZA_X;
    float dz = z - POZA_Z;
    float poza = std::sqrt(dx * dx + dz * dz) - POZA_RADIO;

    return poza < borde ? poza : borde;
}


static float Meseta(
    float x, float z,
    float cx, float cz,
    float radioPlano, float radioBase,
    float altura
)
{
    float dx = x - cx;
    float dz = z - cz;
    float r = std::sqrt(dx * dx + dz * dz);

    float t = 1.0f - Limitar01((r - radioPlano) / (radioBase - radioPlano));

    // Dos escalones: terrazas de basalto con cantil.
    return altura * 0.5f *
        (Suave01((t - 0.10f) / 0.10f) + Suave01((t - 0.55f) / 0.10f));
}


// Altura del cono del volcan (0 fuera de su base).
static float AlturaVolcan(
    float x,
    float z
)
{
    float dx = x - VOLCAN_X;
    float dz = z - VOLCAN_Z;
    float r = std::sqrt(dx * dx + dz * dz);

    if (r >= VOLCAN_RADIO)
    {
        return 0.0f;
    }

    float borde =
        VOLCAN_ALTURA *
        std::pow(1.0f - VOLCAN_CRATER / VOLCAN_RADIO, 1.5f);

    if (r < VOLCAN_CRATER)
    {
        float k = r / VOLCAN_CRATER;
        return borde - 2.2f * (1.0f - k * k);
    }

    float h =
        VOLCAN_ALTURA *
        std::pow(1.0f - r / VOLCAN_RADIO, 1.5f);

    // Crestas radiales en las laderas.
    float ang = std::atan2(dz, dx);

    h *= 1.0f +
        0.07f * std::sin(ang * 7.0f + 1.3f) * Limitar01((r - VOLCAN_CRATER) / 3.0f) +
        0.04f * std::sin(ang * 13.0f + 0.4f);

    return h;
}


// Forma de la isla: 0 = interior, 1 = ya en el mar de lava. Es una
// superelipse (exponente 4, casi un rectangulo de esquinas redondas): deja
// >= 10 de terreno alrededor de todas las casillas, tambien en las esquinas
// del campamento y de la mina. El volcan queda en la costa del fondo.
static const float ISLA_X = 1.0f;
static const float ISLA_Z = -11.0f;
static const float ISLA_RX = 41.0f;
static const float ISLA_RZ = 39.0f;

static float DistanciaCostaForja(
    float x,
    float z,
    float* atras = nullptr
)
{
    float ex = (x - ISLA_X) / ISLA_RX;
    float ez = (z - ISLA_Z) / ISLA_RZ;

    float ang = std::atan2(ez, ex);

    float radio =
        1.0f +
        0.05f * std::sin(3.0f * ang + 0.7f) +
        0.035f * std::sin(7.0f * ang + 2.1f) +
        0.02f * std::sin(13.0f * ang);

    if (atras != nullptr)
    {
        *atras = Suave01(-ez * 1.4f + 0.3f);
    }

    float ex2 = ex * ex;
    float ez2 = ez * ez;

    return std::sqrt(std::sqrt(ex2 * ex2 + ez2 * ez2)) / radio;
}


static float AlturaTerrenoForja(
    float x,
    float z
)
{
    float atras = 0.0f;
    float d = DistanciaCostaForja(x, z, &atras);

    // Reborde de roca (mas alto al fondo) y acantilado hacia el mar.
    float reborde =
        (0.35f + 2.4f * atras) *
        Suave01((d - 0.72f) / 0.14f) *
        (1.0f - Suave01((d - 0.91f) / 0.04f));

    float acantilado = -2.4f * Suave01((d - 0.90f) / 0.05f);

    float h = reborde + acantilado;

    // Ondulacion suave del basalto.
    h += 0.12f * std::sin(x * 0.45f + z * 0.2f) * std::sin(z * 0.37f - x * 0.15f);

    // Mesetas (dos al fondo, una al oeste y otra al este).
    h += Meseta(x, z, -22.0f, -18.5f, 3.4f, 8.0f, 2.4f);
    h += Meseta(x, z, 26.5f, -18.5f, 3.4f, 8.0f, 2.8f);
    h += Meseta(x, z, -30.5f, -5.0f, 2.4f, 6.0f, 1.6f);
    h += Meseta(x, z, 32.5f, 3.0f, 2.2f, 5.8f, 1.8f);

    // Volcan.
    float volcan = AlturaVolcan(x, z);

    if (volcan > h)
    {
        h = volcan + (h > 0.0f ? h * 0.3f : 0.0f);
    }

    // Los senderos quedan planos (suelo del camino a y = 0).
    float dp = DistanciaSenderoForja(x, z);
    h = h * Suave01((dp - 2.0f) / 1.8f);

    // Cauces de lava (mandan sobre el sendero: ahi van puentes).
    float borde = BordeLavaForja(x, z);

    if (borde < 0.9f)
    {
        float t = Suave01(borde / 0.9f);
        h = FONDO_CAUCE * (1.0f - t) + h * t;
    }

    if (h < -2.4f)
    {
        h = -2.4f;
    }

    return h;
}


//==================================================
// LUZ HORNEADA Y PIEZAS EMISIVAS
//==================================================
//
// Las mallas no tienen luces: cada triangulo se oscurece segun su normal
// (como el terreno de Arboleda). Lo incandescente (lava, brasas, cristales)
// se marca con alfa 254 y no se oscurece.

static const unsigned char ALFA_EMISIVO = 254;


static Color Emisivo(Color c)
{
    c.a = ALFA_EMISIVO;
    return c;
}


static float FactorLuz(float nx, float ny, float nz)
{
    float luz = -0.45f * nx + 0.80f * ny + 0.40f * nz;

    if (luz < 0.0f)
    {
        luz = 0.0f;
    }

    return 0.66f + 0.46f * luz;
}


// Aplica la luz a los vertices emitidos desde "desde" (multiplo de 3).
static void IluminarMalla(
    ConstructorMalla& m,
    int desde
)
{
    int total = m.CantidadVertices();

    for (int v = desde; v + 2 < total; v += 3)
    {
        const float* p = &m.posiciones[v * 3];

        float ux = p[3] - p[0], uy = p[4] - p[1], uz = p[5] - p[2];
        float wx = p[6] - p[0], wy = p[7] - p[1], wz = p[8] - p[2];

        float nx = uy * wz - uz * wy;
        float ny = uz * wx - ux * wz;
        float nz = ux * wy - uy * wx;

        float largo = std::sqrt(nx * nx + ny * ny + nz * nz);

        float factor = 1.0f;

        if (largo > 0.000001f)
        {
            factor = FactorLuz(nx / largo, ny / largo, nz / largo);
        }

        for (int k = 0; k < 3; k++)
        {
            unsigned char* c = &m.colores[(v + k) * 4];

            if (c[3] == ALFA_EMISIVO)
            {
                c[3] = 255;
                continue;
            }

            for (int canal = 0; canal < 3; canal++)
            {
                float valor = c[canal] * factor;
                c[canal] = (unsigned char)(valor > 255.0f ? 255.0f : valor);
            }
        }
    }
}


//==================================================
// TERRENO (malla horneada una vez)
//==================================================

static const float TERRENO_X0 = -46.0f;
static const float TERRENO_Z0 = -62.0f;
static const float TERRENO_PASO = 0.8f;
static const int TERRENO_NX = 119;
static const int TERRENO_NZ = 113;

static float alturasTerreno[TERRENO_NX][TERRENO_NZ];
static bool alturasListas = false;

static MallaGpu mallaTerreno;


static void CalcularAlturasTerreno()
{
    for (int ix = 0; ix < TERRENO_NX; ix++)
    {
        for (int iz = 0; iz < TERRENO_NZ; iz++)
        {
            alturasTerreno[ix][iz] =
                AlturaTerrenoForja(
                    TERRENO_X0 + ix * TERRENO_PASO,
                    TERRENO_Z0 + iz * TERRENO_PASO
                );
        }
    }

    alturasListas = true;
}


// Altura del terreno dibujado (interpolacion sobre la rejilla): mucho mas
// barata que AlturaTerrenoForja para colocar props y particulas.
static float AlturaSuelo(float x, float z)
{
    if (!alturasListas)
    {
        CalcularAlturasTerreno();
    }

    float fx = (x - TERRENO_X0) / TERRENO_PASO;
    float fz = (z - TERRENO_Z0) / TERRENO_PASO;

    int ix = (int)std::floor(fx);
    int iz = (int)std::floor(fz);

    if (ix < 0 || iz < 0 || ix >= TERRENO_NX - 1 || iz >= TERRENO_NZ - 1)
    {
        return -2.4f;
    }

    float tx = fx - ix;
    float tz = fz - iz;

    float a = alturasTerreno[ix][iz] + (alturasTerreno[ix + 1][iz] - alturasTerreno[ix][iz]) * tx;
    float b = alturasTerreno[ix][iz + 1] + (alturasTerreno[ix + 1][iz + 1] - alturasTerreno[ix][iz + 1]) * tx;

    return a + (b - a) * tz;
}


// Grietas incandescentes: vetas finas de un patron de ondas cruzadas.
static float Grieta(float x, float z)
{
    float a = std::sin(x * 0.55f + std::sin(z * 0.31f) * 2.2f);
    float b = std::sin(z * 0.62f - x * 0.18f + std::sin(x * 0.27f) * 1.8f);

    float va = 1.0f - Limitar01(std::fabs(a) / 0.05f);
    float vb = 1.0f - Limitar01(std::fabs(b) / 0.04f);

    // Solo en algunas regiones (no es un enrejado continuo).
    float region = 0.5f + 0.5f * std::sin(x * 0.13f + 1.7f) * std::sin(z * 0.11f - 0.6f);

    float v = va > vb ? va : vb;

    return v * Suave01((region - 0.45f) / 0.2f);
}


// Color de un triangulo del terreno. "emisivo" = no recibe la luz horneada.
static Color ColorTerrenoForja(
    float cx, float cz,
    float h, float ny,
    int ix, int iz,
    bool& emisivo
)
{
    emisivo = false;

    float n = Hash2(ix, iz);

    const Color basalto = { 62, 50, 54, 255 };
    const Color basaltoClaro = { 84, 70, 70, 255 };
    const Color ceniza = { 112, 100, 96, 255 };
    const Color brasa = { 255, 112, 34, 255 };
    const Color brasaOscura = { 196, 58, 24, 255 };

    // Fondo de cauces y orillas: roca al rojo.
    if (h < NIVEL_LAVA + 0.12f)
    {
        emisivo = true;
        return Mezcla(brasaOscura, brasa, Suave01((NIVEL_LAVA + 0.12f - h) / 0.5f));
    }

    Color c = Mezcla(basalto, basaltoClaro, n * 0.6f + 0.4f * (0.5f + 0.5f * std::sin(cx * 0.3f + cz * 0.21f)));

    // Ceniza en lo alto (mesetas, reborde y laderas del volcan).
    if (h > 0.9f)
    {
        c = Mezcla(c, ceniza, Suave01((h - 0.9f) / 2.0f) * 0.7f);
    }

    // Tintes por zona: tierra del campamento, azufre de las fumarolas y
    // piedra azulada de la mina.
    float dCamp = std::sqrt((cx + 17.0f) * (cx + 17.0f) + (cz - 2.0f) * (cz - 2.0f));
    c = Mezcla(c, Color{ 104, 82, 64, 255 }, (1.0f - Suave01((dCamp - 4.0f) / 8.0f)) * 0.55f);

    float dFum = std::sqrt((cx - 5.0f) * (cx - 5.0f) + (cz + 20.0f) * (cz + 20.0f));
    float azufre = (1.0f - Suave01((dFum - 3.0f) / 8.0f));
    c = Mezcla(c, Color{ 132, 118, 70, 255 }, azufre * (0.25f + 0.35f * n));

    float dMina = std::sqrt((cx - 20.5f) * (cx - 20.5f) + (cz - 0.0f) * (cz - 0.0f));
    c = Mezcla(c, Color{ 70, 74, 98, 255 }, (1.0f - Suave01((dMina - 4.0f) / 7.0f)) * 0.5f);

    // Camino pisado alrededor del sendero.
    float dp = DistanciaSenderoForja(cx, cz);

    if (dp < 3.6f)
    {
        c = Mezcla(c, Color{ 92, 80, 76, 255 }, 0.45f * (1.0f - Suave01((dp - 1.8f) / 1.8f)));
    }

    // Borde de los cauces: escoria rojiza que se enfria.
    float borde = BordeLavaForja(cx, cz);

    if (borde < 1.6f)
    {
        c = Mezcla(c, Color{ 120, 52, 34, 255 }, 0.6f * (1.0f - Suave01(borde / 1.6f)));
    }

    // Volcan: laderas oscuras, coladas incandescentes y crater al rojo.
    float vx = cx - VOLCAN_X;
    float vz = cz - VOLCAN_Z;
    float rv = std::sqrt(vx * vx + vz * vz);

    if (rv < VOLCAN_RADIO)
    {
        // Falda de basalto oscuro y cono de ceniza clara (se recorta contra
        // el cielo oscuro y la cordillera).
        float alto = 1.0f - rv / VOLCAN_RADIO;
        c = Mezcla(c, Color{ 56, 44, 48, 255 }, Suave01(alto / 0.3f) * 0.5f);
        c = Mezcla(c, Color{ 136, 120, 112, 255 }, Suave01((alto - 0.4f) / 0.35f) * 0.75f);

        if (rv < VOLCAN_CRATER)
        {
            emisivo = true;
            return Mezcla(brasa, Color{ 255, 200, 80, 255 }, 1.0f - rv / VOLCAN_CRATER);
        }

        if (rv < VOLCAN_CRATER + 0.9f)
        {
            c = Mezcla(c, Color{ 140, 50, 32, 255 }, 0.7f);
        }

        // Coladas: bajan hacia la cabecera de cada rio (y una al fondo).
        const float coladas[4][2] =
        {
            { PUNTOS_RIO_OESTE[0].x, PUNTOS_RIO_OESTE[0].z },
            { PUNTOS_RIO_ESTE[0].x, PUNTOS_RIO_ESTE[0].z },
            { PUNTOS_RIO_FUMAROLAS[0].x, PUNTOS_RIO_FUMAROLAS[0].z },
            { VOLCAN_X - 4.0f, VOLCAN_Z - 14.0f }
        };

        float ang = std::atan2(vz, vx);

        for (const auto& colada : coladas)
        {
            float objetivo = std::atan2(colada[1] - VOLCAN_Z, colada[0] - VOLCAN_X);
            float diferencia = ang - objetivo + std::sin(rv * 0.9f) * 0.05f;

            while (diferencia > 3.14159265f) diferencia -= 6.2831853f;
            while (diferencia < -3.14159265f) diferencia += 6.2831853f;

            float ancho = 1.0f / (rv + 0.5f);

            if (std::fabs(diferencia) < ancho)
            {
                emisivo = true;
                return Mezcla(brasa, brasaOscura, rv / VOLCAN_RADIO);
            }

            if (std::fabs(diferencia) < ancho * 1.8f)
            {
                c = Mezcla(c, Color{ 110, 44, 30, 255 }, 0.8f);
            }
        }
    }

    // Pendientes fuertes: roca desnuda.
    if (ny < 0.88f)
    {
        Color roca = Mezcla(Color{ 76, 64, 66, 255 }, Color{ 46, 38, 42, 255 }, (0.86f - ny) / 0.3f);
        c = Mezcla(c, roca, Suave01((0.88f - ny) / 0.18f));
    }

    // Vetas de escoria rojiza lejos del camino y en terreno llano (las
    // grietas incandescentes son piezas aparte: ver GRIETAS).
    if (dp > 3.0f && ny > 0.92f && h > -0.2f && h < 1.6f)
    {
        float g = Grieta(cx, cz);

        if (g > 0.3f)
        {
            c = Mezcla(c, Color{ 104, 42, 32, 255 }, (g - 0.3f) * 1.2f);
        }
    }

    return c;
}


static void ConstruirMallaTerreno()
{
    if (!alturasListas)
    {
        CalcularAlturasTerreno();
    }

    ConstructorMalla m;
    m.Limpiar();

    for (int ix = 0; ix < TERRENO_NX - 1; ix++)
    {
        for (int iz = 0; iz < TERRENO_NZ - 1; iz++)
        {
            float x0 = TERRENO_X0 + ix * TERRENO_PASO;
            float z0 = TERRENO_Z0 + iz * TERRENO_PASO;
            float x1 = x0 + TERRENO_PASO;
            float z1 = z0 + TERRENO_PASO;

            Vector3 p00 = { x0, alturasTerreno[ix][iz], z0 };
            Vector3 p01 = { x0, alturasTerreno[ix][iz + 1], z1 };
            Vector3 p10 = { x1, alturasTerreno[ix + 1][iz], z0 };
            Vector3 p11 = { x1, alturasTerreno[ix + 1][iz + 1], z1 };

            // Todo hundido bajo la lava: no se ve.
            if (p00.y < -2.3f && p01.y < -2.3f && p10.y < -2.3f && p11.y < -2.3f)
            {
                continue;
            }

            Vector3 tri[2][3] =
            {
                { p00, p01, p10 },
                { p10, p01, p11 }
            };

            for (int k = 0; k < 2; k++)
            {
                const Vector3* v = tri[k];

                Vector3 u = { v[1].x - v[0].x, v[1].y - v[0].y, v[1].z - v[0].z };
                Vector3 w = { v[2].x - v[0].x, v[2].y - v[0].y, v[2].z - v[0].z };

                Vector3 normal =
                {
                    u.y * w.z - u.z * w.y,
                    u.z * w.x - u.x * w.z,
                    u.x * w.y - u.y * w.x
                };

                float largo = std::sqrt(normal.x * normal.x + normal.y * normal.y + normal.z * normal.z);

                if (largo > 0.0001f)
                {
                    normal.x /= largo;
                    normal.y /= largo;
                    normal.z /= largo;
                }

                if (normal.y < 0.0f)
                {
                    normal.x = -normal.x;
                    normal.y = -normal.y;
                    normal.z = -normal.z;
                }

                float cx = (v[0].x + v[1].x + v[2].x) / 3.0f;
                float cz = (v[0].z + v[1].z + v[2].z) / 3.0f;
                float h = (v[0].y + v[1].y + v[2].y) / 3.0f;

                bool emisivo = false;
                Color color = ColorTerrenoForja(cx, cz, h, normal.y, ix + k, iz + k, emisivo);

                if (!emisivo)
                {
                    color = Oscurecer(color, FactorLuz(normal.x, normal.y, normal.z));
                }

                m.Triangulo(v[0], v[1], v[2], color);
            }
        }
    }

    ActualizarMallaGpu(mallaTerreno, m);
}


static void DibujarTerrenoForja()
{
    if (!mallaTerreno.cargada)
    {
        ConstruirMallaTerreno();
    }

    rlDisableBackfaceCulling();
    DibujarMallaGpu(mallaTerreno);
    rlEnableBackfaceCulling();
}


//==================================================
// HORIZONTE: CORDILLERA VOLCANICA (malla horneada)
//==================================================
//
// Un anillo de montes oscuros alrededor de la isla, mas alla del mar de
// lava, con vetas incandescentes y alguna chimenea humeante. Se ve desde
// la camara de partida y desde la vista previa que orbita.

static MallaGpu mallaCordillera;


static void ConstruirMallaCordillera()
{
    ConstructorMalla m;
    m.Limpiar();

    const int MONTES = 46;

    for (int i = 0; i < MONTES; i++)
    {
        float ang = 6.2831853f * (i + 0.5f * Hash2(i, 3)) / MONTES;
        float radio = 78.0f + 26.0f * Hash2(i, 7);
        float x = 1.0f + std::cos(ang) * radio * 1.05f;
        float z = -17.0f + std::sin(ang) * radio;
        float alto = 9.0f + 16.0f * Hash2(i, 11);
        float base = 10.0f + 9.0f * Hash2(i, 13);

        // Mas bajos por delante: no tapan el mar de lava del primer plano.
        if (std::sin(ang) > 0.2f)
        {
            alto *= 0.55f;
        }

        Color roca = Mezcla(Color{ 44, 32, 40, 255 }, Color{ 66, 48, 56, 255 }, Hash2(i, 17));

        m.Cilindro(
            Vector3{ x, -2.5f, z }, Vector3{ x, alto, z },
            base, base * 0.08f, 7, roca
        );

        // Hombro secundario.
        float a2 = ang + 0.05f;
        float x2 = 1.0f + std::cos(a2) * (radio + 6.0f) * 1.05f;
        float z2 = -17.0f + std::sin(a2) * (radio + 6.0f);

        m.Cilindro(
            Vector3{ x2, -2.5f, z2 }, Vector3{ x2, alto * 0.62f, z2 },
            base * 0.8f, base * 0.1f, 6, Oscurecer(roca, 0.85f)
        );

        // Vetas de lava por la ladera que mira a la isla.
        if (Hash2(i, 19) > 0.45f)
        {
            float hacia = ang + 3.14159265f;
            float vx = std::cos(hacia);
            float vz = std::sin(hacia);

            m.Cilindro(
                Vector3{ x, alto * 0.85f, z },
                Vector3{ x + vx * base * 0.95f, -0.3f, z + vz * base * 0.95f },
                0.35f, 0.9f, 4,
                Emisivo(Color{ 255, 104, 30, 255 })
            );        }
    }

    // Luz horneada (las vetas emisivas quedan intactas).
    IluminarMalla(m, 0);

    ActualizarMallaGpu(mallaCordillera, m);
}


//==================================================
// SENDEROS (malla horneada por calidad)
//==================================================
//
// Como en Arboleda: cinta por conexion con borde, losas de dos carriles y
// huella; disco en cada nodo y plaza empedrada en bifurcaciones y uniones.
// Material por zona: losas de basalto con juntas de brasa en el anillo,
// tierra apisonada en el campamento, ceniza azufrada en las fumarolas,
// piedra azulada en la mina y grava con durmientes y rieles en la galeria.
// No se dibuja donde el terreno cae a la lava (puente, canal, rio).

static const float SENDERO_SEMIANCHO = 1.5f;
static const float SENDERO_BORDE = 0.30f;
static const float SENDERO_PLAZA = 1.85f;

static const float LIFT_BORDE = 0.030f;
static const float LIFT_BORDE_NODO = 0.050f;
static const float LIFT_TIERRA = 0.070f;
static const float LIFT_TIERRA_NODO = 0.090f;
static const float LIFT_DETALLE = 0.108f;
static const float LIFT_RIEL = 0.122f;

struct MaterialSendero
{
    Color tierra;
    Color borde;
    float contraste;
    float largoLosa;
};

static MaterialSendero MaterialSenderoZona(int zona)
{
    switch (zona)
    {
        case ZONA_CAMPAMENTO:
            return { Color{ 128, 102, 80, 255 }, Color{ 82, 64, 52, 255 }, 0.07f, 1.4f };
        case ZONA_ANILLO_FORJA:
            return { Color{ 96, 84, 86, 255 }, Color{ 50, 40, 44, 255 }, 0.14f, 0.9f };
        case ZONA_PUENTE_BASALTO:
            return { Color{ 108, 94, 96, 255 }, Color{ 58, 48, 52, 255 }, 0.12f, 0.8f };
        case ZONA_GALERIA_MINERA:
            return { Color{ 92, 84, 78, 255 }, Color{ 60, 52, 48, 255 }, 0.05f, 1.0f };
        case ZONA_FUMAROLAS:
            return { Color{ 134, 120, 82, 255 }, Color{ 86, 74, 54, 255 }, 0.09f, 1.1f };
        case ZONA_MINA_CRISTAL:
            return { Color{ 100, 104, 122, 255 }, Color{ 58, 60, 78, 255 }, 0.12f, 0.95f };
        default:
            break;
    }

    return { Color{ 100, 88, 86, 255 }, Color{ 56, 46, 50, 255 }, 0.09f, 1.2f };
}

static const Color COLOR_BORDE_SUELO = Color{ 70, 58, 60, 255 };

static MallaGpu mallaSenderos;
static int nivelSenderosConstruido = -1;


struct SeccionSendero
{
    float x, y, z;
    float nx, nz;
};


static bool ConexionInversa(const Tablero& t, int a, int b)
{
    const Casilla* cb = t.ObtenerCasilla(b);

    for (int k = 0; cb != nullptr && k < cb->cantidadConexiones; k++)
    {
        if (cb->conexiones[k].destino == a)
        {
            return true;
        }
    }

    return false;
}


static void CuadroSendero(
    ConstructorMalla& m,
    const SeccionSendero& s0, const SeccionSendero& s1,
    float lat0, float lat1,
    float lift,
    Color color
)
{
    Vector3 a = { s0.x + s0.nx * lat0, s0.y + lift, s0.z + s0.nz * lat0 };
    Vector3 b = { s0.x + s0.nx * lat1, s0.y + lift, s0.z + s0.nz * lat1 };
    Vector3 c = { s1.x + s1.nx * lat0, s1.y + lift, s1.z + s1.nz * lat0 };
    Vector3 d = { s1.x + s1.nx * lat1, s1.y + lift, s1.z + s1.nz * lat1 };

    m.TrianguloDobleCara(a, b, c, color);
    m.TrianguloDobleCara(b, d, c, color);
}


static void DiscoSendero(
    ConstructorMalla& m,
    float x, float y, float z,
    float radio, float lift,
    Color color
)
{
    const int LADOS_DISCO = 18;

    for (int i = 0; i < LADOS_DISCO; i++)
    {
        float a0 = 6.2831853f * i / LADOS_DISCO;
        float a1 = 6.2831853f * (i + 1) / LADOS_DISCO;

        m.TrianguloDobleCara(
            Vector3{ x, y + lift, z },
            Vector3{ x + std::cos(a0) * radio, y + lift, z + std::sin(a0) * radio },
            Vector3{ x + std::cos(a1) * radio, y + lift, z + std::sin(a1) * radio },
            color
        );
    }
}


static void AnilloSendero(
    ConstructorMalla& m,
    float x, float y, float z,
    float radioInterior, float radioExterior, float lift,
    Color a, Color b, int losas
)
{
    for (int i = 0; i < losas; i++)
    {
        float a0 = 6.2831853f * i / losas;
        float a1 = 6.2831853f * (i + 0.88f) / losas;
        Color c = (i % 2 == 0) ? a : b;

        Vector3 p00 = { x + std::cos(a0) * radioInterior, y + lift, z + std::sin(a0) * radioInterior };
        Vector3 p01 = { x + std::cos(a0) * radioExterior, y + lift, z + std::sin(a0) * radioExterior };
        Vector3 p10 = { x + std::cos(a1) * radioInterior, y + lift, z + std::sin(a1) * radioInterior };
        Vector3 p11 = { x + std::cos(a1) * radioExterior, y + lift, z + std::sin(a1) * radioExterior };

        m.TrianguloDobleCara(p00, p01, p10, c);
        m.TrianguloDobleCara(p01, p11, p10, c);
    }
}


static void ConstruirMallaSenderos(NivelCalidadGrafica nivel)
{
    const Tablero& t = TableroReferenciaForja();
    const bool detalle = nivel != CALIDAD_BAJA;
    const float paso = 0.42f;
    const float limiteBorde = SENDERO_SEMIANCHO + SENDERO_BORDE;

    static ConstructorMalla m;
    m.Limpiar();

    int grado[MAX_CASILLAS_TABLERO] = {};

    for (int i = 0; i < t.cantidadCasillas; i++)
    {
        const Casilla& a = t.casillas[i];

        for (int k = 0; k < a.cantidadConexiones; k++)
        {
            int j = a.conexiones[k].destino;

            if (j < 0 || j >= t.cantidadCasillas)
            {
                continue;
            }

            grado[i]++;

            if (!ConexionInversa(t, i, j))
            {
                grado[j]++;
            }
        }
    }

    // --- Cintas entre casillas conectadas ---
    for (int i = 0; i < t.cantidadCasillas; i++)
    {
        const Casilla& a = t.casillas[i];

        for (int k = 0; k < a.cantidadConexiones; k++)
        {
            int j = a.conexiones[k].destino;
            const Casilla* pb = t.ObtenerCasilla(j);

            if (pb == nullptr || (j < i && ConexionInversa(t, i, j)))
            {
                continue;
            }

            const Casilla& b = *pb;

            float dx = b.posicion.x - a.posicion.x;
            float dz = b.posicion.z - a.posicion.z;
            float largo = std::sqrt(dx * dx + dz * dz);

            if (largo < 0.01f)
            {
                continue;
            }

            int n = (int)std::ceil(largo / paso);
            float px = -dz / largo;
            float pz = dx / largo;

            // Ligera curvatura (recta en puente y galeria).
            bool recto =
                a.zona == ZONA_GALERIA_MINERA ||
                a.zona == ZONA_PUENTE_BASALTO ||
                b.zona == ZONA_PUENTE_BASALTO;

            float amp =
                recto
                ? 0.0f
                : (Hash2(i * 31 + 7, j * 17 + 3) - 0.5f) * 0.45f;

            static SeccionSendero sec[256];
            static bool suelo[256];

            if (n > 250)
            {
                n = 250;
            }

            for (int s = 0; s <= n; s++)
            {
                float u = (float)s / n;
                float curva = std::sin(3.14159265f * u) * amp;

                sec[s].x = a.posicion.x + dx * u + px * curva;
                sec[s].z = a.posicion.z + dz * u + pz * curva;
                sec[s].y = (a.posicion.y + (b.posicion.y - a.posicion.y) * u) - 0.25f;
            }

            for (int s = 0; s <= n; s++)
            {
                int s0 = s > 0 ? s - 1 : s;
                int s1 = s < n ? s + 1 : s;
                float tx = sec[s1].x - sec[s0].x;
                float tz = sec[s1].z - sec[s0].z;
                float tl = std::sqrt(tx * tx + tz * tz);

                if (tl < 0.0001f)
                {
                    tx = dx;
                    tz = dz;
                    tl = largo;
                }

                sec[s].nx = -tz / tl;
                sec[s].nz = tx / tl;

                // El suelo debe estar al nivel del camino (si no, hay puente).
                bool firme = true;

                for (int lado = -1; lado <= 1; lado++)
                {
                    float h =
                        AlturaSuelo(
                            sec[s].x + sec[s].nx * limiteBorde * lado,
                            sec[s].z + sec[s].nz * limiteBorde * lado
                        );

                    if (h < sec[s].y - 0.10f)
                    {
                        firme = false;
                    }
                }

                suelo[s] = firme;
            }

            MaterialSendero ma = MaterialSenderoZona(a.zona);
            MaterialSendero mb = MaterialSenderoZona(b.zona);

            bool galeria =
                a.zona == ZONA_GALERIA_MINERA &&
                b.zona == ZONA_GALERIA_MINERA;

            bool brasas =
                a.zona == ZONA_ANILLO_FORJA ||
                b.zona == ZONA_ANILLO_FORJA;

            int bloqueAnterior = -1;

            for (int s = 0; s < n; s++)
            {
                if (!suelo[s] || !suelo[s + 1])
                {
                    continue;
                }

                float u = ((float)s + 0.5f) / n;
                float tamLosa = ma.largoLosa + (mb.largoLosa - ma.largoLosa) * u;
                int bloque = (int)std::floor(u * largo / tamLosa);
                float contraste = ma.contraste + (mb.contraste - ma.contraste) * u;

                Color tierra = Mezcla(ma.tierra, mb.tierra, u);
                Color borde = Mezcla(ma.borde, mb.borde, u);
                Color bordeExt = Mezcla(borde, COLOR_BORDE_SUELO, 0.45f);

                for (int lado = -1; lado <= 1; lado += 2)
                {
                    CuadroSendero(
                        m, sec[s], sec[s + 1],
                        lado * SENDERO_SEMIANCHO * 0.9f, lado * limiteBorde,
                        LIFT_BORDE, bordeExt
                    );
                }

                for (int lado = 0; lado < 2; lado++)
                {
                    float v = Hash2(i * 131 + bloque * 7 + 1, j * 17 + lado);
                    Color c = Oscurecer(tierra, 1.0f + (v - 0.5f) * 2.0f * contraste);

                    CuadroSendero(
                        m, sec[s], sec[s + 1],
                        lado == 0 ? -SENDERO_SEMIANCHO : 0.0f,
                        lado == 0 ? 0.0f : SENDERO_SEMIANCHO,
                        LIFT_TIERRA, c
                    );
                }

                if (galeria)
                {
                    // Durmiente de madera en cada seccion y dos rieles.
                    if (detalle && s % 2 == 0)
                    {
                        SeccionSendero d0 = sec[s];
                        SeccionSendero d1 = sec[s + 1];

                        d1.x = d0.x + (sec[s + 1].x - d0.x) * 0.45f;
                        d1.z = d0.z + (sec[s + 1].z - d0.z) * 0.45f;

                        CuadroSendero(
                            m, d0, d1, -0.80f, 0.80f, LIFT_DETALLE,
                            Oscurecer(Color{ 116, 78, 48, 255 }, 0.9f + 0.2f * Hash2(i + s, j))
                        );
                    }

                    for (int lado = -1; lado <= 1; lado += 2)
                    {
                        CuadroSendero(
                            m, sec[s], sec[s + 1],
                            lado * 0.52f - 0.05f, lado * 0.52f + 0.05f,
                            LIFT_RIEL, Color{ 160, 160, 170, 255 }
                        );
                    }
                }
                else if (brasas && detalle && bloque != bloqueAnterior && bloqueAnterior >= 0)
                {
                    // Junta de brasa entre losas de basalto.
                    SeccionSendero j1 = sec[s];
                    j1.x += (sec[s + 1].x - sec[s].x) * 0.18f;
                    j1.z += (sec[s + 1].z - sec[s].z) * 0.18f;

                    CuadroSendero(
                        m, sec[s], j1, -SENDERO_SEMIANCHO * 0.92f, SENDERO_SEMIANCHO * 0.92f,
                        LIFT_DETALLE, Color{ 232, 98, 36, 255 }
                    );
                }
                else if (detalle && !brasas)
                {
                    CuadroSendero(
                        m, sec[s], sec[s + 1], -0.50f, 0.50f, LIFT_DETALLE,
                        Mezcla(tierra, Color{ 186, 172, 150, 255 }, 0.22f)
                    );
                }

                bloqueAnterior = bloque;
            }

            // Guijarros de escoria en el borde (solo calidad alta).
            if (nivel == CALIDAD_ALTA)
            {
                for (int s = 1; s < n; s += 2)
                {
                    if (!suelo[s])
                    {
                        continue;
                    }

                    for (int lado = -1; lado <= 1; lado += 2)
                    {
                        float v = Hash2(i * 97 + s, j * 13 + lado + 5);

                        if (v > 0.55f)
                        {
                            continue;
                        }

                        float lat = lado * (SENDERO_SEMIANCHO + 0.05f + v * 0.5f);
                        float r = 0.07f + Hash2(s * 3 + i, j + lado) * 0.09f;
                        float tono = 0.80f + Hash2(s, i * 5 + j) * 0.35f;

                        m.Esfera(
                            Vector3{
                                sec[s].x + sec[s].nx * lat,
                                sec[s].y + 0.04f + r * 0.2f,
                                sec[s].z + sec[s].nz * lat
                            },
                            r, 3, 5,
                            Oscurecer(Color{ 92, 80, 82, 255 }, tono)
                        );
                    }
                }
            }
        }
    }

    // --- Nodos: disco de union y plaza empedrada en bifurcaciones ---
    for (int i = 0; i < t.cantidadCasillas; i++)
    {
        const Casilla& c = t.casillas[i];

        // Las casillas del puente estan sobre la lava: su apoyo es la calzada.
        if (c.zona == ZONA_PUENTE_BASALTO && AlturaSuelo(c.posicion.x, c.posicion.z) < -0.2f)
        {
            continue;
        }

        MaterialSendero ms = MaterialSenderoZona(c.zona);
        float y = c.posicion.y - 0.25f;

        bool plaza = grado[i] >= 3;
        float semi = plaza ? SENDERO_PLAZA : SENDERO_SEMIANCHO;

        DiscoSendero(
            m, c.posicion.x, y, c.posicion.z,
            semi + SENDERO_BORDE, LIFT_BORDE_NODO,
            Mezcla(ms.borde, COLOR_BORDE_SUELO, 0.45f)
        );

        DiscoSendero(
            m, c.posicion.x, y, c.posicion.z,
            semi, LIFT_TIERRA_NODO, ms.tierra
        );

        if (plaza)
        {
            Color claro = Mezcla(ms.tierra, Color{ 150, 138, 132, 255 }, 0.5f);
            Color oscuro = Mezcla(ms.tierra, Color{ 52, 44, 48, 255 }, 0.5f);

            AnilloSendero(
                m, c.posicion.x, y, c.posicion.z,
                SENDERO_SEMIANCHO + 0.05f, SENDERO_PLAZA - 0.05f,
                LIFT_DETALLE, claro, oscuro, 16
            );
        }
    }

    ActualizarMallaGpu(mallaSenderos, m);
    nivelSenderosConstruido = (int)nivel;
}


static void DibujarSenderosForja()
{
    NivelCalidadGrafica nivel = CalidadDecoracion();

    if (!mallaSenderos.cargada || nivelSenderosConstruido != (int)nivel)
    {
        ConstruirMallaSenderos(nivel);
    }

    DibujarMallaGpu(mallaSenderos);
}


//==================================================
// LAVA (modo inmediato: plano global animado)
//==================================================

static void CuadroHorizontal(
    float x, float y, float z,
    float mitadX, float mitadZ,
    Color color
)
{
    Vector3 a = { x - mitadX, y, z - mitadZ };
    Vector3 b = { x - mitadX, y, z + mitadZ };
    Vector3 c = { x + mitadX, y, z + mitadZ };
    Vector3 d = { x + mitadX, y, z - mitadZ };

    DrawTriangle3D(a, b, c, color);
    DrawTriangle3D(a, c, d, color);
}


static void DibujarMarDeLava(float tiempo)
{
    float pulso = Parpadeo(tiempo, 1.3f);

    rlDisableBackfaceCulling();

    // Mar de lava: llega al horizonte en todas direcciones.
    CuadroHorizontal(
        1.0f, NIVEL_LAVA, -17.0f, 200.0f, 200.0f,
        Color{ 224, (unsigned char)(70 + 18 * pulso), 18, 255 }
    );

    rlEnableBackfaceCulling();
}


//==================================================
// LUGARES DE LOS LANDMARKS
//==================================================
//
// Un landmark (o mas) por zona, todos a >= 2.8 del sendero y fuera de la
// franja frontal de las casillas. Alrededor de cada uno se reserva un claro
// donde no se planta decoracion. El landmark principal es el volcan
// (terreno, 17 de alto) al fondo de las fumarolas.

// ANILLO: dos islotes en el lago, el crisol (oeste) y el gran yunque (este).
static const float CRISOL_X = -4.6f;
static const float CRISOL_Z = -2.0f;
static const float YUNQUE_X = 4.6f;
static const float YUNQUE_Z = -2.0f;

// CAMPAMENTO: castillete minero, fogata, carpas y la herreria (junto a la
// tienda del herrero).
static const float CASTILLETE_X = -15.2f;
static const float CASTILLETE_Z = 3.6f;
static const float FOGATA_X = -15.6f;
static const float FOGATA_Z = 0.5f;
static const float HERRERIA_X = -26.6f;
static const float HERRERIA_Z = 6.0f;

static const float CARPAS_FORJA[3][2] =
{
    { -15.6f, -2.2f },
    { -12.3f,  5.2f },
    { -25.5f,  1.5f }
};

// GALERIA: cresta con la boca de la mina al este de los rieles.
static const float BOCA_MINA_X = 14.6f;
static const float BOCA_MINA_Z = 0.6f;

// FUMAROLAS: gran fumarola al oeste del lobulo.
static const float GRAN_FUMAROLA_X = -7.0f;
static const float GRAN_FUMAROLA_Z = -17.5f;

// MINA: gran cristal en el margen este y un grupo dentro del lazo.
static const float GRAN_CRISTAL_X = 28.5f;
static const float GRAN_CRISTAL_Z = -1.0f;
static const float CRISTAL_LAZO_X = 20.7f;
static const float CRISTAL_LAZO_Z = 0.0f;

struct ZonaReservadaForja
{
    float x;
    float z;
    float radio;
};

static const ZonaReservadaForja ZONAS_RESERVADAS_FORJA[] =
{
    { CRISOL_X, CRISOL_Z, 2.0f },
    { YUNQUE_X, YUNQUE_Z, 2.0f },
    { CASTILLETE_X, CASTILLETE_Z, 2.4f },
    { FOGATA_X, FOGATA_Z, 1.6f },
    { HERRERIA_X, HERRERIA_Z, 3.6f },
    { CARPAS_FORJA[0][0], CARPAS_FORJA[0][1], 1.5f },
    { CARPAS_FORJA[1][0], CARPAS_FORJA[1][1], 1.5f },
    { CARPAS_FORJA[2][0], CARPAS_FORJA[2][1], 1.5f },
    { BOCA_MINA_X, BOCA_MINA_Z, 2.6f },
    { GRAN_FUMAROLA_X, GRAN_FUMAROLA_Z, 3.2f },
    { GRAN_CRISTAL_X, GRAN_CRISTAL_Z, 3.6f },
    { CRISTAL_LAZO_X, CRISTAL_LAZO_Z, 1.6f }
};


static bool DespejadoForja(float x, float z, float radio)
{
    for (const ZonaReservadaForja& r : ZONAS_RESERVADAS_FORJA)
    {
        float dx = x - r.x;
        float dz = z - r.z;
        float limite = r.radio + radio;

        if (dx * dx + dz * dz < limite * limite)
        {
            return false;
        }
    }

    return true;
}


// Algo alto delante de una casilla (hacia la camara) la taparia.
static bool OcultaCasillaForja(float x, float z)
{
    const Tablero& t = TableroReferenciaForja();

    for (int i = 0; i < t.cantidadCasillas; i++)
    {
        const Vector3& p = t.casillas[i].posicion;

        if (
            z > p.z + 0.4f && z < p.z + 3.4f &&
            std::fabs(x - p.x) < 2.0f
        )
        {
            return true;
        }
    }

    return false;
}


// Altura del borde superior del crater (para humo y bombas de lava).
static float AlturaCraterForja()
{
    return AlturaVolcan(VOLCAN_X + VOLCAN_CRATER + 0.05f, VOLCAN_Z);
}


//==================================================
// PIEZAS DE ESCENOGRAFIA (se emiten a mallas)
//==================================================
//
// Cada pieza es una funcion que emite a un ConstructorMalla; la misma
// funcion se reutiliza cientos de veces en la malla de decoracion.

static const Color MADERA_FORJA = Color{ 112, 80, 54, 255 };
static const Color MADERA_OSCURA_FORJA = Color{ 78, 56, 40, 255 };
static const Color HIERRO_FORJA = Color{ 70, 66, 72, 255 };
static const Color BASALTO_FORJA = Color{ 64, 54, 60, 255 };
static const Color BRASA_FORJA = Color{ 255, 110, 34, 255 };


static void PiezaRoca(
    ConstructorMalla& m,
    float x, float y, float z,
    float radio, float giro,
    Color color
)
{
    TransformacionMalla t;
    t.tx = x;
    t.ty = y;
    t.tz = z;
    t.giroY = giro;
    t.sx = 1.15f;
    t.sy = 0.72f;
    t.sz = 0.9f;

    m.EstablecerTransformacion(t);

    m.Esfera(Vector3{ 0.0f, radio * 0.45f, 0.0f }, radio, 5, 6, color);

    m.Esfera(
        Vector3{ radio * 0.25f, radio * 0.95f, -radio * 0.1f },
        radio * 0.55f, 4, 5,
        Mezcla(color, Color{ 150, 136, 136, 255 }, 0.25f)
    );

    m.QuitarTransformacion();
}


// Monton de escoria; algunas conservan un nucleo al rojo.
static void PiezaEscoria(
    ConstructorMalla& m,
    float x, float y, float z,
    float escala, float variacion
)
{
    Color oscura = Oscurecer(Color{ 60, 42, 42, 255 }, 0.85f + 0.3f * variacion);

    for (int k = 0; k < 3; k++)
    {
        float ang = variacion * 6.283f + k * 2.1f;
        float r = (0.14f + 0.06f * k) * escala;

        m.Esfera(
            Vector3{
                x + std::cos(ang) * 0.2f * escala,
                y + r * 0.45f,
                z + std::sin(ang) * 0.2f * escala
            },
            r, 3, 5,
            Mezcla(oscura, Color{ 96, 50, 40, 255 }, k * 0.25f)
        );
    }

    if (variacion > 0.62f)
    {
        m.Esfera(
            Vector3{ x, y + 0.12f * escala, z },
            0.09f * escala, 2, 4,
            Emisivo(BRASA_FORJA)
        );
    }
}


// Arbol calcinado: tronco torcido, ramas secas y alguna brasa.
static void PiezaArbolQuemado(
    ConstructorMalla& m,
    float x, float y, float z,
    float escala, float variacion, float giro
)
{
    Color carbon = Oscurecer(Color{ 54, 44, 42, 255 }, 0.85f + 0.3f * variacion);
    float alto = (1.7f + 0.9f * variacion) * escala;
    float inclinacion = (variacion - 0.5f) * 0.3f * escala;

    m.Cilindro(
        Vector3{ x, y - 0.05f, z },
        Vector3{ x + inclinacion, y + alto, z },
        0.17f * escala, 0.05f * escala, 5, carbon
    );

    for (int k = 0; k < 3; k++)
    {
        float a = giro * DEG2RAD + k * 2.2f;
        float h = alto * (0.42f + 0.17f * k);
        float largo = (0.5f + 0.25f * Hash2(k, (int)(variacion * 97.0f))) * escala;
        float base = inclinacion * h / alto;

        m.Cilindro(
            Vector3{ x + base, y + h, z },
            Vector3{
                x + base + std::cos(a) * largo,
                y + h + largo * 0.65f,
                z + std::sin(a) * largo
            },
            0.06f * escala, 0.015f * escala, 4, carbon
        );
    }

    if (variacion > 0.55f)
    {
        m.Esfera(
            Vector3{ x + inclinacion * 0.3f, y + alto * 0.3f, z + 0.12f * escala },
            0.06f * escala, 2, 4,
            Emisivo(BRASA_FORJA)
        );
    }
}


// Matorral seco: ramitas en abanico.
static void PiezaArbustoSeco(
    ConstructorMalla& m,
    float x, float y, float z,
    float escala, float variacion
)
{
    Color rama = Oscurecer(Color{ 120, 98, 74, 255 }, 0.8f + 0.35f * variacion);

    m.Esfera(Vector3{ x, y + 0.06f * escala, z }, 0.14f * escala, 2, 4, Oscurecer(rama, 0.7f));

    for (int k = 0; k < 5; k++)
    {
        float a = variacion * 5.0f + k * 1.257f;
        float largo = (0.45f + 0.2f * Hash2(k, (int)(variacion * 53.0f))) * escala;

        m.Cilindro(
            Vector3{ x, y, z },
            Vector3{ x + std::cos(a) * largo * 0.6f, y + largo, z + std::sin(a) * largo * 0.6f },
            0.035f * escala, 0.008f, 3, rama
        );
    }
}


// Agujas de obsidiana (vidrio volcanico negro violaceo).
static void PiezaObsidiana(
    ConstructorMalla& m,
    float x, float y, float z,
    float escala, float variacion
)
{
    const Color negro = Color{ 36, 28, 46, 255 };
    const Color brillo = Color{ 84, 70, 112, 255 };

    for (int k = 0; k < 3; k++)
    {
        float a = variacion * 6.0f + k * 2.3f;
        float alto = (0.55f + 0.35f * k * (0.4f + variacion)) * escala;
        float abre = 0.18f * escala;

        m.Cilindro(
            Vector3{ x + std::cos(a) * abre * 0.5f, y - 0.05f, z + std::sin(a) * abre * 0.5f },
            Vector3{ x + std::cos(a) * abre * 1.6f, y + alto, z + std::sin(a) * abre * 1.6f },
            0.16f * escala, 0.0f, 4,
            k == 1 ? brillo : negro
        );
    }
}


// Un cristal: prisma hexagonal con punta (la punta brilla).
static void PiezaCristal(
    ConstructorMalla& m,
    Vector3 base,
    float inclinacionX, float inclinacionZ,
    float alto, float radio,
    Color color
)
{
    Vector3 medio =
    {
        base.x + inclinacionX * alto * 0.72f,
        base.y + alto * 0.72f,
        base.z + inclinacionZ * alto * 0.72f
    };

    Vector3 punta =
    {
        base.x + inclinacionX * alto,
        base.y + alto,
        base.z + inclinacionZ * alto
    };

    m.Cilindro(base, medio, radio, radio * 0.92f, 6, color);
    m.Cilindro(medio, punta, radio * 0.92f, 0.0f, 6, Emisivo(Mezcla(color, WHITE, 0.4f)));
}


static Color TonoCristal(float variacion)
{
    if (variacion < 0.45f)
    {
        return Color{ 90, 210, 250, 255 };
    }

    if (variacion < 0.8f)
    {
        return Color{ 176, 116, 250, 255 };
    }

    return Color{ 250, 120, 200, 255 };
}


// Grupo de cristales sobre una roca azulada.
static void PiezaGrupoCristales(
    ConstructorMalla& m,
    float x, float y, float z,
    float escala, float variacion
)
{
    Color tono = TonoCristal(variacion);

    m.Esfera(Vector3{ x, y + 0.05f, z }, 0.32f * escala, 3, 5, Color{ 62, 64, 86, 255 });

    for (int k = 0; k < 4; k++)
    {
        float a = variacion * 6.0f + k * 1.6f;
        float abre = k == 0 ? 0.0f : 0.35f;

        PiezaCristal(
            m,
            Vector3{ x + std::cos(a) * 0.12f * escala, y, z + std::sin(a) * 0.12f * escala },
            std::cos(a) * abre, std::sin(a) * abre,
            (k == 0 ? 1.05f : 0.55f + 0.15f * k) * escala,
            (k == 0 ? 0.14f : 0.09f) * escala,
            Oscurecer(tono, 0.85f + 0.1f * k)
        );
    }
}


// Costra y agujas de azufre.
static void PiezaAzufre(
    ConstructorMalla& m,
    float x, float y, float z,
    float escala, float variacion
)
{
    m.Cilindro(
        Vector3{ x, y - 0.02f, z }, Vector3{ x, y + 0.05f, z },
        0.48f * escala, 0.40f * escala, 7,
        Mezcla(Color{ 140, 124, 72, 255 }, Color{ 176, 158, 84, 255 }, variacion)
    );

    for (int k = 0; k < 3; k++)
    {
        float a = variacion * 5.0f + k * 2.1f;

        m.Cilindro(
            Vector3{ x + std::cos(a) * 0.16f * escala, y + 0.03f, z + std::sin(a) * 0.16f * escala },
            Vector3{ x + std::cos(a) * 0.26f * escala, y + (0.32f + 0.1f * k) * escala, z + std::sin(a) * 0.26f * escala },
            0.07f * escala, 0.0f, 4,
            Color{ 222, 198, 78, 255 }
        );
    }
}


// Roca volcanica con grietas al rojo.
static void PiezaBrasa(
    ConstructorMalla& m,
    float x, float y, float z,
    float escala, float variacion, float giro
)
{
    PiezaRoca(m, x, y, z, 0.42f * escala, giro, Color{ 52, 40, 42, 255 });

    for (int k = 0; k < 2; k++)
    {
        float a = giro * DEG2RAD + k * 2.6f;

        m.Esfera(
            Vector3{ x + std::cos(a) * 0.32f * escala, y + (0.22f + 0.1f * variacion) * escala, z + std::sin(a) * 0.32f * escala },
            0.09f * escala, 2, 4,
            Emisivo(Mezcla(BRASA_FORJA, Color{ 255, 200, 90, 255 }, variacion))
        );
    }
}


// Fumarola pequena: cono de azufre con boca al rojo (el vapor es animado).
static void PiezaFumarola(
    ConstructorMalla& m,
    float x, float y, float z,
    float escala
)
{
    float alto = 0.75f * escala;

    m.Cilindro(
        Vector3{ x, y - 0.1f, z }, Vector3{ x, y + alto, z },
        0.95f * escala, 0.32f * escala, 8, Color{ 116, 100, 80, 255 }
    );

    m.Cilindro(
        Vector3{ x, y + alto - 0.1f * escala, z }, Vector3{ x, y + alto + 0.05f, z },
        0.40f * escala, 0.34f * escala, 8, Color{ 222, 198, 78, 255 }
    );

    m.Cilindro(
        Vector3{ x, y + alto + 0.05f, z }, Vector3{ x, y + alto + 0.07f, z },
        0.22f * escala, 0.22f * escala, 8, Emisivo(Color{ 255, 140, 50, 255 })
    );
}


// Grieta incandescente en el suelo: zigzag de cinco tramos.
static void PiezaGrieta(
    ConstructorMalla& m,
    float x, float z,
    float escala, float variacion, float giro
)
{
    float a = giro * DEG2RAD;
    float ux = std::cos(a);
    float uz = std::sin(a);

    float px = x - ux * 1.2f * escala;
    float pz = z - uz * 1.2f * escala;

    for (int k = 0; k < 5; k++)
    {
        float desvio = (Hash2(k, (int)(variacion * 211.0f)) - 0.5f) * 0.7f;
        float qx = px + (ux - uz * desvio) * 0.5f * escala;
        float qz = pz + (uz + ux * desvio) * 0.5f * escala;

        float dx = qx - px;
        float dz = qz - pz;
        float largo = std::sqrt(dx * dx + dz * dz) + 0.0001f;

        // Mas ancha en el centro.
        float ancho = (0.05f + 0.07f * (1.0f - std::fabs(k - 2.0f) / 2.5f)) * escala;
        float nx = -dz / largo * ancho;
        float nz = dx / largo * ancho;

        float y0 = AlturaSuelo(px, pz) + 0.04f;
        float y1 = AlturaSuelo(qx, qz) + 0.04f;

        Color brasa = Emisivo(Mezcla(Color{ 214, 64, 26, 255 }, Color{ 255, 170, 60, 255 }, 1.0f - std::fabs(k - 2.0f) / 2.0f));

        m.TrianguloDobleCara(
            Vector3{ px - nx, y0, pz - nz }, Vector3{ px + nx, y0, pz + nz },
            Vector3{ qx + nx, y1, qz + nz }, brasa
        );

        m.TrianguloDobleCara(
            Vector3{ px - nx, y0, pz - nz }, Vector3{ qx + nx, y1, qz + nz },
            Vector3{ qx - nx, y1, qz - nz }, brasa
        );

        px = qx;
        pz = qz;
    }
}


// Caja girada en Y (para tablones, vigas y techos).
static void CajaGirada(
    ConstructorMalla& m,
    float x, float y, float z,
    float giro,
    Vector3 tamano,
    Color color
)
{
    TransformacionMalla t;
    t.tx = x;
    t.ty = y;
    t.tz = z;
    t.giroY = giro;

    m.EstablecerTransformacion(t);
    m.Caja(Vector3{ 0.0f, 0.0f, 0.0f }, tamano, color);
    m.QuitarTransformacion();
}


static void PiezaBarril(
    ConstructorMalla& m,
    float x, float y, float z
)
{
    m.Cilindro(Vector3{ x, y, z }, Vector3{ x, y + 0.75f, z }, 0.3f, 0.3f, 8, MADERA_FORJA);
    m.Cilindro(Vector3{ x, y + 0.14f, z }, Vector3{ x, y + 0.2f, z }, 0.315f, 0.315f, 8, HIERRO_FORJA);
    m.Cilindro(Vector3{ x, y + 0.56f, z }, Vector3{ x, y + 0.62f, z }, 0.315f, 0.315f, 8, HIERRO_FORJA);
}


static void PiezaCajon(
    ConstructorMalla& m,
    float x, float y, float z,
    float giro, float lado
)
{
    CajaGirada(m, x, y + lado * 0.5f, z, giro, Vector3{ lado, lado, lado }, Color{ 150, 112, 72, 255 });
    CajaGirada(m, x, y + lado * 0.5f, z, giro, Vector3{ lado * 1.02f, lado * 0.16f, lado * 1.02f }, MADERA_OSCURA_FORJA);
}


// Vagoneta minera (sobre sus rieles) cargada de mineral o cristal.
static void PiezaVagoneta(
    ConstructorMalla& m,
    float x, float y, float z,
    float giro,
    Color carga
)
{
    CajaGirada(m, x, y + 0.55f, z, giro, Vector3{ 1.2f, 0.55f, 0.8f }, Color{ 96, 88, 92, 255 });
    CajaGirada(m, x, y + 0.84f, z, giro, Vector3{ 1.26f, 0.06f, 0.86f }, HIERRO_FORJA);

    float a = giro * DEG2RAD;
    float ux = std::cos(a);
    float uz = -std::sin(a);

    for (int k = -1; k <= 1; k += 2)
    {
        for (int lado = -1; lado <= 1; lado += 2)
        {
            float px = x + ux * 0.4f * k + uz * 0.42f * lado;
            float pz = z + uz * 0.4f * k - ux * 0.42f * lado;

            m.Esfera(Vector3{ px, y + 0.2f, pz }, 0.17f, 3, 6, Color{ 44, 40, 46, 255 });
        }
    }

    m.Esfera(Vector3{ x, y + 0.86f, z }, 0.42f, 3, 6, carga);
}


//==================================================
// DECORACION DISPERSA (colocada una vez, horneada por calidad)
//==================================================
//
// ~1100 piezas en ALTA, solo cerca del sendero (diorama). Cada tipo tiene
// su regla: distancia al sendero y a la lava, separacion y, si es alto,
// fuera de la franja frontal de las casillas. Algunas reglas prefieren una
// region (cristales en la mina, azufre y obsidiana en las fumarolas).

enum TipoPropForja
{
    PROP_ROCA = 0,
    PROP_ESCORIA,
    PROP_ARBOL_QUEMADO,
    PROP_ARBUSTO_SECO,
    PROP_OBSIDIANA,
    PROP_CRISTAL,
    PROP_AZUFRE,
    PROP_BRASA,
    PROP_FUMAROLA,
    PROP_GRIETA
};

struct PropForja
{
    TipoPropForja tipo;
    float x;
    float y;
    float z;
    float escala;
    float variacion;
    float giro;
};

struct ReglaPropForja
{
    TipoPropForja tipo;
    int cantidad;
    float senderoMin;
    float senderoMax;
    float lavaMin;
    float separacion;
    bool alto;
    float escalaMin;
    float escalaMax;

    // Region preferida (radio 0 = toda la isla).
    float regionX;
    float regionZ;
    float regionRadio;
};

static const ReglaPropForja REGLAS_PROPS_FORJA[] =
{
    { PROP_FUMAROLA,      16, 2.8f, 14.0f, 1.0f, 3.2f, false, 0.8f, 1.3f,  3.0f, -22.0f, 13.0f },
    { PROP_ARBOL_QUEMADO, 110, 2.6f, 15.0f, 1.0f, 1.9f, true,  0.8f, 1.3f,  0.0f,   0.0f,  0.0f },
    { PROP_CRISTAL,       70, 2.0f, 12.0f, 0.5f, 0.75f, false, 0.6f, 1.3f, 20.0f,   0.0f, 11.0f },
    { PROP_OBSIDIANA,     90, 2.2f, 16.0f, 0.4f, 0.8f, false, 0.6f, 1.2f,  4.0f, -24.0f, 18.0f },
    { PROP_AZUFRE,        50, 2.0f, 12.0f, 0.3f, 0.6f, false, 0.7f, 1.2f,  5.0f, -20.0f, 11.0f },
    { PROP_ROCA,         240, 2.1f, 18.0f, 0.3f, 0.9f, false, 0.3f, 0.95f, 0.0f,   0.0f,  0.0f },
    { PROP_BRASA,         70, 2.0f, 12.0f, 0.2f, 0.8f, false, 0.6f, 1.1f,  0.0f,   0.0f,  0.0f },
    { PROP_ESCORIA,      150, 2.0f, 12.0f, 0.2f, 0.6f, false, 0.7f, 1.3f,  0.0f,   0.0f,  0.0f },
    { PROP_ARBUSTO_SECO, 170, 2.0f, 11.0f, 0.6f, 0.5f, false, 0.7f, 1.2f,  0.0f,   0.0f,  0.0f },
    { PROP_GRIETA,        40, 3.0f, 14.0f, 1.5f, 2.5f, false, 0.8f, 1.4f,  0.0f,   0.0f,  0.0f }
};

static const int MAX_PROPS_FORJA = 1600;

static PropForja propsForja[MAX_PROPS_FORJA];
static int cantidadPropsForja = 0;
static bool propsColocados = false;

static unsigned int semillaPropsForja = 24680u;


static float AleatorioForja()
{
    semillaPropsForja = semillaPropsForja * 1664525u + 1013904223u;
    return (float)((semillaPropsForja >> 8) & 0xFFFF) / 65535.0f;
}


static bool SeparadoDeSuTipo(
    TipoPropForja tipo,
    float x, float z,
    float separacion
)
{
    for (int i = 0; i < cantidadPropsForja; i++)
    {
        const PropForja& p = propsForja[i];

        if (p.tipo != tipo)
        {
            continue;
        }

        float dx = p.x - x;
        float dz = p.z - z;

        if (dx * dx + dz * dz < separacion * separacion)
        {
            return false;
        }
    }

    return true;
}


// Suelo casi plano alrededor (las grietas son cintas pegadas al suelo).
static bool TerrenoLlanoForja(float x, float z, float radio)
{
    float h = AlturaSuelo(x, z);

    return
        std::fabs(AlturaSuelo(x + radio, z) - h) < 0.1f &&
        std::fabs(AlturaSuelo(x - radio, z) - h) < 0.1f &&
        std::fabs(AlturaSuelo(x, z + radio) - h) < 0.1f &&
        std::fabs(AlturaSuelo(x, z - radio) - h) < 0.1f;
}


static void ColocarPropsForja()
{
    semillaPropsForja = 24680u;
    cantidadPropsForja = 0;

    // Rectangulo de la isla util (la costa recorta el resto).
    const float X0 = -36.0f;
    const float ANCHO = 74.0f;
    const float Z0 = -46.0f;
    const float LARGO = 68.0f;

    for (const ReglaPropForja& regla : REGLAS_PROPS_FORJA)
    {
        for (int intento = 0, n = 0; intento < 30000 && n < regla.cantidad; intento++)
        {
            float x;
            float z;

            if (regla.regionRadio > 0.0f)
            {
                float ang = AleatorioForja() * 6.2831853f;
                float rad = std::sqrt(AleatorioForja()) * regla.regionRadio;
                x = regla.regionX + std::cos(ang) * rad;
                z = regla.regionZ + std::sin(ang) * rad;
            }
            else
            {
                x = X0 + AleatorioForja() * ANCHO;
                z = Z0 + AleatorioForja() * LARGO;
            }

            float h = AlturaSuelo(x, z);
            bool arbolOMata =
                regla.tipo == PROP_ARBOL_QUEMADO ||
                regla.tipo == PROP_ARBUSTO_SECO;

            // Sin nada en la lava, al borde del acantilado ni (las matas)
            // en las laderas altas del volcan y las mesetas.
            if (
                h < -0.15f ||
                DistanciaCostaForja(x, z) > 0.86f ||
                (arbolOMata && h > 1.4f) ||
                (regla.tipo == PROP_GRIETA && (h > 0.5f || !TerrenoLlanoForja(x, z, 1.6f))) ||
                BordeLavaForja(x, z) < regla.lavaMin ||
                !DespejadoForja(x, z, regla.alto ? 1.0f : 0.4f) ||
                (regla.alto && OcultaCasillaForja(x, z))
            )
            {
                continue;
            }

            float sendero = DistanciaSenderoForja(x, z);

            if (sendero < regla.senderoMin || sendero > regla.senderoMax)
            {
                continue;
            }

            if (!SeparadoDeSuTipo(regla.tipo, x, z, regla.separacion))
            {
                continue;
            }

            if (cantidadPropsForja >= MAX_PROPS_FORJA)
            {
                break;
            }

            PropForja& p = propsForja[cantidadPropsForja++];

            p.tipo = regla.tipo;
            p.x = x;
            p.y = h - 0.03f;
            p.z = z;
            p.escala = regla.escalaMin + AleatorioForja() * (regla.escalaMax - regla.escalaMin);
            p.variacion = AleatorioForja();
            p.giro = AleatorioForja() * 360.0f;

            n++;
        }
    }

    propsColocados = true;
}


// Calidad de decoracion: ALTA conserva todo; MEDIA recorta parte de lo
// menudo; BAJA deja solo lo cercano al sendero y poco de lo menudo.
static bool ConservarPropForja(
    int indice,
    const PropForja& p,
    NivelCalidadGrafica nivel
)
{
    if (nivel == CALIDAD_ALTA)
    {
        return true;
    }

    bool menudo =
        p.tipo == PROP_ESCORIA ||
        p.tipo == PROP_ARBUSTO_SECO ||
        p.tipo == PROP_AZUFRE ||
        p.tipo == PROP_GRIETA;

    float azar = Hash2(indice, 77);

    if (nivel == CALIDAD_MEDIA)
    {
        return !menudo || azar < FactorCalidadGrafica(nivel);
    }

    if (DistanciaSenderoForja(p.x, p.z) > 11.0f)
    {
        return false;
    }

    return !menudo || azar < FactorCalidadGrafica(nivel) * 0.7f;
}


static MallaGpu mallaProps;
static int nivelPropsConstruido = -1;


static void ConstruirMallaProps(NivelCalidadGrafica nivel)
{
    if (!propsColocados)
    {
        ColocarPropsForja();
    }

    static ConstructorMalla m;
    m.Limpiar();
    m.reduccionDetalle = nivel == CALIDAD_ALTA ? 0 : 1;

    for (int i = 0; i < cantidadPropsForja; i++)
    {
        const PropForja& p = propsForja[i];

        if (!ConservarPropForja(i, p, nivel))
        {
            continue;
        }

        switch (p.tipo)
        {
            case PROP_ROCA:
                PiezaRoca(
                    m, p.x, p.y, p.z, p.escala, p.giro,
                    Oscurecer(Mezcla(Color{ 78, 66, 68, 255 }, Color{ 104, 92, 90, 255 }, p.variacion), 0.9f)
                );
                break;

            case PROP_ESCORIA:
                PiezaEscoria(m, p.x, p.y, p.z, p.escala, p.variacion);
                break;

            case PROP_ARBOL_QUEMADO:
                PiezaArbolQuemado(m, p.x, p.y, p.z, p.escala, p.variacion, p.giro);
                break;

            case PROP_ARBUSTO_SECO:
                PiezaArbustoSeco(m, p.x, p.y, p.z, p.escala, p.variacion);
                break;

            case PROP_OBSIDIANA:
                PiezaObsidiana(m, p.x, p.y, p.z, p.escala, p.variacion);
                break;

            case PROP_CRISTAL:
                PiezaGrupoCristales(m, p.x, p.y, p.z, p.escala, p.variacion);
                break;

            case PROP_AZUFRE:
                PiezaAzufre(m, p.x, p.y, p.z, p.escala, p.variacion);
                break;

            case PROP_BRASA:
                PiezaBrasa(m, p.x, p.y, p.z, p.escala, p.variacion, p.giro);
                break;

            case PROP_FUMAROLA:
                PiezaFumarola(m, p.x, p.y, p.z, p.escala);
                break;

            case PROP_GRIETA:
                PiezaGrieta(m, p.x, p.z, p.escala, p.variacion, p.giro);
                break;
        }
    }

    IluminarMalla(m, 0);

    ActualizarMallaGpu(mallaProps, m);
    nivelPropsConstruido = (int)nivel;
}


static void DibujarPropsForja()
{
    NivelCalidadGrafica nivel = CalidadDecoracion();

    if (!mallaProps.cargada || nivelPropsConstruido != (int)nivel)
    {
        ConstruirMallaProps(nivel);
    }

    DibujarMallaGpu(mallaProps);
}


//==================================================
// LANDMARKS (una malla horneada, construida una vez)
//==================================================
//
// Calzadas sobre la lava, islotes del crisol y del yunque, castillete y
// carpas del campamento, herreria, boca de la mina con sus farolas, gran
// fumarola, cristales gigantes y carteles de las bifurcaciones. Lo que se
// mueve (humo, vapor, fuego, chorro y brillos) va en ANIMACIONES.

static MallaGpu mallaLandmarks;

// Farolas de la galeria (la luz parpadea en modo inmediato).
static const float FAROLAS_FORJA[3][2] =
{
    { 13.5f, -0.8f },
    { 13.6f,  4.4f },
    { 11.0f,  6.3f }
};

static const float ALTO_FAROLA = 2.0f;


// Calzada (o puentecito) donde el sendero cruza la lava: losas sobre
// columnas de basalto. El tablon levadizo del puente es aparte.
static void ConstruirCalzadasForja(ConstructorMalla& m)
{
    const Tablero& t = TableroReferenciaForja();

    for (int i = 0; i < t.cantidadCasillas; i++)
    {
        const Casilla& a = t.casillas[i];

        for (int k = 0; k < a.cantidadConexiones; k++)
        {
            int j = a.conexiones[k].destino;
            const Casilla* pb = t.ObtenerCasilla(j);

            if (
                pb == nullptr ||
                (j < i && ConexionInversa(t, i, j)) ||
                (i == nodosForja.puenteBifurcacion && j == nodosForja.puente[0])
            )
            {
                continue;
            }

            const Casilla& b = *pb;

            float dx = b.posicion.x - a.posicion.x;
            float dz = b.posicion.z - a.posicion.z;
            float largo = std::sqrt(dx * dx + dz * dz);

            // Solo si el eje del camino cae a la lava en algun punto.
            float minimo = 10.0f;

            for (int s = 1; s < 10; s++)
            {
                float u = s / 10.0f;
                float h = AlturaSuelo(a.posicion.x + dx * u, a.posicion.z + dz * u);

                if (h < minimo)
                {
                    minimo = h;
                }
            }

            if (minimo > -0.15f || largo < 0.01f)
            {
                continue;
            }

            bool basalto =
                a.zona == ZONA_PUENTE_BASALTO ||
                b.zona == ZONA_PUENTE_BASALTO;

            float giro = std::atan2(-dz, dx) * RAD2DEG;
            float px = -dz / largo;
            float pz = dx / largo;

            int losas = (int)std::ceil(largo / 0.75f);

            for (int s = 0; s < losas; s++)
            {
                float u = (s + 0.5f) / losas;

                Color c =
                    basalto
                    ? Oscurecer(BASALTO_FORJA, s % 2 == 0 ? 1.15f : 1.3f)
                    : Oscurecer(MADERA_FORJA, 0.9f + 0.2f * Hash2(i + s, j));

                CajaGirada(
                    m,
                    a.posicion.x + dx * u, 0.0f, a.posicion.z + dz * u,
                    giro,
                    Vector3{ largo / losas * 0.94f, 0.24f, basalto ? 2.6f : 2.4f },
                    c
                );
            }

            // Apoyos y baranda baja.
            int apoyos = (int)(largo / 1.5f) + 1;

            for (int s = 0; s <= apoyos; s++)
            {
                float u = (float)s / apoyos;
                float cx = a.posicion.x + dx * u;
                float cz = a.posicion.z + dz * u;

                for (int lado = -1; lado <= 1; lado += 2)
                {
                    float lx = cx + px * 1.1f * lado;
                    float lz = cz + pz * 1.1f * lado;

                    if (basalto)
                    {
                        // Columna bajo la calzada y mojon bajo en el borde.
                        m.Cilindro(
                            Vector3{ lx, -1.4f, lz }, Vector3{ lx, -0.1f, lz },
                            0.4f, 0.4f, 6,
                            Oscurecer(BASALTO_FORJA, 0.9f + 0.25f * Hash2(s, i * 3 + lado))
                        );

                        float bx = cx + px * 1.42f * lado;
                        float bz = cz + pz * 1.42f * lado;

                        m.Cilindro(
                            Vector3{ bx, 0.0f, bz }, Vector3{ bx, 0.2f + 0.1f * Hash2(s * 7 + i, lado + j), bz },
                            0.14f, 0.14f, 6,
                            Oscurecer(BASALTO_FORJA, 1.35f)
                        );
                    }
                    else
                    {
                        m.Cilindro(
                            Vector3{ lx, -1.2f, lz }, Vector3{ lx, 0.65f, lz },
                            0.09f, 0.09f, 5, MADERA_OSCURA_FORJA
                        );
                    }
                }
            }

            if (!basalto)
            {
                for (int lado = -1; lado <= 1; lado += 2)
                {
                    m.Cilindro(
                        Vector3{ a.posicion.x + px * 1.1f * lado, 0.6f, a.posicion.z + pz * 1.1f * lado },
                        Vector3{ b.posicion.x + px * 1.1f * lado, 0.6f, b.posicion.z + pz * 1.1f * lado },
                        0.05f, 0.05f, 4, MADERA_FORJA
                    );
                }
            }
        }
    }

    // Plataforma hexagonal bajo las casillas que quedan sobre la lava.
    for (int i = 0; i < t.cantidadCasillas; i++)
    {
        const Vector3& p = t.casillas[i].posicion;

        if (AlturaSuelo(p.x, p.z) < -0.2f)
        {
            m.Cilindro(
                Vector3{ p.x, -1.4f, p.z }, Vector3{ p.x, 0.1f, p.z },
                1.5f, 1.45f, 6, Oscurecer(BASALTO_FORJA, 1.1f)
            );
        }
    }
}


// Islote de columnas de basalto que asoma de la lava.
static void IsloteBasalto(
    ConstructorMalla& m,
    float x, float z,
    float radio, float alto
)
{
    m.Cilindro(
        Vector3{ x, -1.3f, z }, Vector3{ x, alto, z },
        radio, radio * 0.92f, 6, BASALTO_FORJA
    );

    // Tapa mas clara (se lee mejor sobre la lava) y tres columnas sueltas.
    m.Cilindro(
        Vector3{ x, alto, z }, Vector3{ x, alto + 0.04f, z },
        radio * 0.92f, radio * 0.92f, 6, Oscurecer(BASALTO_FORJA, 1.5f)
    );

    for (int k = 0; k < 3; k++)
    {
        float a = (k * 120.0f + 50.0f) * DEG2RAD;
        float cx = x + std::cos(a) * radio * 1.05f;
        float cz = z + std::sin(a) * radio * 1.05f;

        m.Cilindro(
            Vector3{ cx, -1.3f, cz },
            Vector3{ cx, alto - 0.3f + 0.25f * Hash2(k, (int)(x * 10.0f)), cz },
            radio * 0.24f, radio * 0.24f, 6,
            Oscurecer(BASALTO_FORJA, 1.1f + 0.3f * Hash2(k, 5))
        );
    }
}


// Crisol: cuenco de hierro sobre un tripode, con portico y cadenas. Vierte
// lava hacia el oeste (el chorro animado sale de su pico).
static void ConstruirCrisol(ConstructorMalla& m)
{
    const float x = CRISOL_X;
    const float z = CRISOL_Z;

    IsloteBasalto(m, x, z, 1.7f, 0.25f);

    for (int k = 0; k < 3; k++)
    {
        float a = (k * 120.0f + 90.0f) * DEG2RAD;

        m.Cilindro(
            Vector3{ x + std::cos(a) * 1.0f, 0.3f, z + std::sin(a) * 1.0f },
            Vector3{ x + std::cos(a) * 0.55f, 1.45f, z + std::sin(a) * 0.55f },
            0.1f, 0.08f, 5, HIERRO_FORJA
        );
    }

    m.Cilindro(Vector3{ x, 1.3f, z }, Vector3{ x, 2.3f, z }, 0.6f, 0.95f, 10, Color{ 140, 118, 104, 255 });
    m.Cilindro(Vector3{ x, 2.24f, z }, Vector3{ x, 2.38f, z }, 1.0f, 1.0f, 10, HIERRO_FORJA);
    m.Cilindro(Vector3{ x, 2.3f, z }, Vector3{ x, 2.34f, z }, 0.9f, 0.9f, 10, Emisivo(Color{ 255, 150, 50, 255 }));

    // Pico vertedor.
    m.Cilindro(Vector3{ x - 0.85f, 2.22f, z }, Vector3{ x - 1.35f, 2.12f, z }, 0.22f, 0.14f, 6, HIERRO_FORJA);

    // Portico con cadenas.
    for (int lado = -1; lado <= 1; lado += 2)
    {
        m.Cilindro(
            Vector3{ x + 0.2f, 0.3f, z + 1.35f * lado },
            Vector3{ x + 0.2f, 3.6f, z + 1.35f * lado },
            0.12f, 0.1f, 6, MADERA_OSCURA_FORJA
        );

        m.Cilindro(
            Vector3{ x + 0.2f, 3.5f, z + 0.35f * lado },
            Vector3{ x + 0.1f, 2.38f, z + 0.85f * lado },
            0.03f, 0.03f, 4, HIERRO_FORJA
        );
    }

    m.Cilindro(
        Vector3{ x + 0.2f, 3.6f, z - 1.5f }, Vector3{ x + 0.2f, 3.6f, z + 1.5f },
        0.13f, 0.13f, 6, MADERA_FORJA
    );
}


// Gran yunque con el lingote al rojo y un martillo gigante apoyado.
static void ConstruirYunque(ConstructorMalla& m)
{
    const float x = YUNQUE_X;
    const float z = YUNQUE_Z;
    const Color hierroClaro = Color{ 92, 88, 98, 255 };

    IsloteBasalto(m, x, z, 1.7f, 0.25f);

    m.Caja(Vector3{ x, 0.55f, z }, Vector3{ 1.5f, 0.5f, 1.1f }, HIERRO_FORJA);
    m.Caja(Vector3{ x, 1.0f, z }, Vector3{ 0.8f, 0.5f, 0.6f }, HIERRO_FORJA);
    m.Caja(Vector3{ x, 1.5f, z }, Vector3{ 2.0f, 0.55f, 0.95f }, hierroClaro);
    m.Caja(Vector3{ x + 1.15f, 1.6f, z }, Vector3{ 0.35f, 0.35f, 0.8f }, hierroClaro);
    m.Cilindro(Vector3{ x - 1.0f, 1.55f, z }, Vector3{ x - 1.9f, 1.62f, z }, 0.27f, 0.0f, 8, hierroClaro);

    m.Caja(Vector3{ x + 0.1f, 1.84f, z }, Vector3{ 0.9f, 0.13f, 0.3f }, Emisivo(Color{ 255, 176, 70, 255 }));

    // Martillo.
    m.Cilindro(
        Vector3{ x + 1.25f, 0.3f, z + 0.95f }, Vector3{ x + 0.7f, 3.0f, z + 0.5f },
        0.09f, 0.09f, 6, MADERA_FORJA
    );

    CajaGirada(m, x + 0.66f, 3.2f, z + 0.47f, 40.0f, Vector3{ 0.55f, 0.55f, 1.15f }, HIERRO_FORJA);
}


// Grupos de columnas de basalto en las orillas del lago.
static void ConstruirColumnasLago(ConstructorMalla& m)
{
    const float grupos[2][2] =
    {
        { -7.0f, -2.0f }, { 7.0f, -2.0f }
    };

    for (int g = 0; g < 2; g++)
    {
        for (int k = 0; k < 5; k++)
        {
            float a = k * 1.26f + g;
            float r = k == 0 ? 0.0f : 0.55f;
            float x = grupos[g][0] + std::cos(a) * r;
            float z = grupos[g][1] + std::sin(a) * r;
            float alto = -0.15f + 0.95f * Hash2(g * 5 + k, 31) * (k == 0 ? 1.2f : 0.8f);

            m.Cilindro(
                Vector3{ x, -1.3f, z }, Vector3{ x, alto, z },
                0.32f, 0.32f, 6,
                Oscurecer(BASALTO_FORJA, 0.85f + 0.35f * Hash2(k, g))
            );
        }
    }
}


// Castillete minero (torre de extraccion) con su polea.
static void ConstruirCastillete(ConstructorMalla& m)
{
    const float x = CASTILLETE_X;
    const float z = CASTILLETE_Z;
    const float ALTO = 5.4f;
    const float BASE = 1.1f;
    const float CIMA = 0.45f;

    Vector3 abajo[4];
    Vector3 arriba[4];

    for (int k = 0; k < 4; k++)
    {
        float sx = (k == 0 || k == 3) ? -1.0f : 1.0f;
        float sz = (k < 2) ? -1.0f : 1.0f;

        abajo[k] = Vector3{ x + sx * BASE, 0.0f, z + sz * BASE };
        arriba[k] = Vector3{ x + sx * CIMA, ALTO, z + sz * CIMA };

        m.Cilindro(abajo[k], arriba[k], 0.13f, 0.1f, 5, MADERA_FORJA);
    }

    // Travesanos y cruces.
    for (int nivel = 1; nivel <= 3; nivel++)
    {
        float u = nivel / 4.0f;

        for (int k = 0; k < 4; k++)
        {
            const Vector3& a0 = abajo[k];
            const Vector3& a1 = arriba[k];
            const Vector3& b0 = abajo[(k + 1) % 4];
            const Vector3& b1 = arriba[(k + 1) % 4];

            Vector3 pa = { a0.x + (a1.x - a0.x) * u, a0.y + (a1.y - a0.y) * u, a0.z + (a1.z - a0.z) * u };
            Vector3 pb = { b0.x + (b1.x - b0.x) * u, b0.y + (b1.y - b0.y) * u, b0.z + (b1.z - b0.z) * u };

            m.Cilindro(pa, pb, 0.06f, 0.06f, 4, MADERA_OSCURA_FORJA);

            float v = (nivel - 1) / 4.0f;
            Vector3 qa = { a0.x + (a1.x - a0.x) * v, a0.y + (a1.y - a0.y) * v, a0.z + (a1.z - a0.z) * v };

            m.Cilindro(qa, pb, 0.045f, 0.045f, 4, MADERA_OSCURA_FORJA);
        }
    }

    m.Caja(Vector3{ x, ALTO, z }, Vector3{ 1.3f, 0.12f, 1.3f }, MADERA_OSCURA_FORJA);

    // Polea (rueda vertical) y cable al pozo.
    const float RUEDA = 0.7f;
    float yr = ALTO + 0.8f;

    for (int k = 0; k < 14; k++)
    {
        float a0 = 6.2831853f * k / 14;
        float a1 = 6.2831853f * (k + 1) / 14;

        m.Cilindro(
            Vector3{ x + std::cos(a0) * RUEDA, yr + std::sin(a0) * RUEDA, z },
            Vector3{ x + std::cos(a1) * RUEDA, yr + std::sin(a1) * RUEDA, z },
            0.06f, 0.06f, 4, HIERRO_FORJA
        );
    }

    for (int k = 0; k < 3; k++)
    {
        float a = 3.14159265f * k / 3;

        m.Cilindro(
            Vector3{ x + std::cos(a) * RUEDA, yr + std::sin(a) * RUEDA, z },
            Vector3{ x - std::cos(a) * RUEDA, yr - std::sin(a) * RUEDA, z },
            0.035f, 0.035f, 4, HIERRO_FORJA
        );
    }

    m.Cilindro(Vector3{ x, ALTO, z }, Vector3{ x, yr, z }, 0.08f, 0.08f, 4, MADERA_OSCURA_FORJA);
    m.Cilindro(Vector3{ x + RUEDA, yr, z }, Vector3{ x + 0.3f, 0.6f, z }, 0.025f, 0.025f, 3, Color{ 40, 38, 42, 255 });

    // Brocal del pozo.
    m.Caja(Vector3{ x, 0.3f, z }, Vector3{ 1.6f, 0.6f, 1.6f }, MADERA_OSCURA_FORJA);
    m.Caja(Vector3{ x, 0.61f, z }, Vector3{ 1.1f, 0.02f, 1.1f }, Color{ 20, 16, 18, 255 });

    // Farol en lo alto.
    m.Esfera(Vector3{ x, yr + RUEDA + 0.25f, z }, 0.16f, 3, 6, Emisivo(Color{ 255, 206, 120, 255 }));
}


// Carpas, fogata, barriles y cajones del campamento.
static void ConstruirCampamento(ConstructorMalla& m)
{
    const Color lonas[3] =
    {
        Color{ 196, 170, 128, 255 },
        Color{ 176, 116, 82, 255 },
        Color{ 150, 140, 112, 255 }
    };

    for (int k = 0; k < 3; k++)
    {
        float x = CARPAS_FORJA[k][0];
        float z = CARPAS_FORJA[k][1];
        float y = AlturaSuelo(x, z);

        m.Cilindro(Vector3{ x, y, z }, Vector3{ x, y + 1.45f, z }, 1.2f, 0.0f, 4, lonas[k]);
        m.Cilindro(Vector3{ x, y + 0.02f, z }, Vector3{ x, y + 0.6f, z }, 1.24f, 0.75f, 4, Oscurecer(lonas[k], 0.8f));
        m.Cilindro(Vector3{ x, y + 1.3f, z }, Vector3{ x, y + 1.85f, z }, 0.04f, 0.03f, 3, MADERA_OSCURA_FORJA);
        m.Caja(Vector3{ x + 0.18f, y + 1.76f, z }, Vector3{ 0.32f, 0.16f, 0.02f }, Color{ 214, 70, 46, 255 });
    }

    // Fogata: anillo de piedras, lenos y bancos (el fuego es animado).
    float fy = AlturaSuelo(FOGATA_X, FOGATA_Z);

    for (int k = 0; k < 8; k++)
    {
        float a = k * 0.785f;

        m.Esfera(
            Vector3{ FOGATA_X + std::cos(a) * 0.6f, fy + 0.06f, FOGATA_Z + std::sin(a) * 0.6f },
            0.17f, 3, 5, Color{ 96, 88, 86, 255 }
        );
    }

    for (int k = 0; k < 3; k++)
    {
        float a = k * 2.1f;

        m.Cilindro(
            Vector3{ FOGATA_X + std::cos(a) * 0.5f, fy + 0.05f, FOGATA_Z + std::sin(a) * 0.5f },
            Vector3{ FOGATA_X - std::cos(a) * 0.1f, fy + 0.3f, FOGATA_Z - std::sin(a) * 0.1f },
            0.07f, 0.06f, 5, MADERA_OSCURA_FORJA
        );
    }

    m.Esfera(Vector3{ FOGATA_X, fy + 0.08f, FOGATA_Z }, 0.24f, 3, 6, Emisivo(BRASA_FORJA));

    for (int lado = -1; lado <= 1; lado += 2)
    {
        m.Cilindro(
            Vector3{ FOGATA_X + 1.25f * lado, fy + 0.2f, FOGATA_Z - 0.6f },
            Vector3{ FOGATA_X + 1.25f * lado, fy + 0.2f, FOGATA_Z + 0.6f },
            0.2f, 0.2f, 6, MADERA_FORJA
        );
    }

    // Pertrechos junto a las carpas y al castillete.
    PiezaBarril(m, CARPAS_FORJA[0][0] + 1.1f, AlturaSuelo(CARPAS_FORJA[0][0] + 1.1f, CARPAS_FORJA[0][1] - 0.6f), CARPAS_FORJA[0][1] - 0.6f);
    PiezaCajon(m, CARPAS_FORJA[1][0] - 1.15f, 0.0f, CARPAS_FORJA[1][1] - 0.5f, 20.0f, 0.55f);
    PiezaCajon(m, CARPAS_FORJA[2][0] + 0.2f, 0.0f, CARPAS_FORJA[2][1] - 1.25f, 65.0f, 0.5f);
    PiezaBarril(m, CASTILLETE_X + 1.55f, 0.0f, CASTILLETE_Z - 1.2f);
    PiezaBarril(m, CASTILLETE_X + 1.25f, 0.0f, CASTILLETE_Z - 1.75f);
    PiezaCajon(m, CASTILLETE_X - 1.6f, 0.0f, CASTILLETE_Z + 1.3f, 10.0f, 0.6f);
}


// Herreria (el puesto del herrero esta junto a la tienda del campamento):
// casa de piedra, fragua encendida, chimenea y alero con su yunque.
static void ConstruirHerreria(ConstructorMalla& m)
{
    const float x = HERRERIA_X;
    const float z = HERRERIA_Z;
    const float y = AlturaSuelo(x, z);
    const Color piedra = Color{ 104, 92, 88, 255 };
    const Color teja = Color{ 128, 64, 46, 255 };

    m.Caja(Vector3{ x, y + 1.1f, z }, Vector3{ 3.2f, 2.2f, 2.6f }, piedra);

    // Tejado a dos aguas (cumbrera en X).
    Vector3 c0 = { x - 1.85f, y + 3.1f, z };
    Vector3 c1 = { x + 1.85f, y + 3.1f, z };

    for (int lado = -1; lado <= 1; lado += 2)
    {
        Vector3 e0 = { x - 1.85f, y + 2.05f, z + 1.6f * lado };
        Vector3 e1 = { x + 1.85f, y + 2.05f, z + 1.6f * lado };

        m.TrianguloDobleCara(c0, c1, e1, teja);
        m.TrianguloDobleCara(c0, e1, e0, teja);

        // Hastiales.
        m.TrianguloDobleCara(
            Vector3{ x + 1.6f * lado, y + 2.2f, z - 1.3f },
            Vector3{ x + 1.6f * lado, y + 2.2f, z + 1.3f },
            Vector3{ x + 1.6f * lado, y + 3.05f, z },
            piedra
        );
    }

    // Chimenea humeante (el humo es animado).
    m.Caja(Vector3{ x - 0.9f, y + 2.6f, z - 0.5f }, Vector3{ 0.7f, 2.4f, 0.7f }, Oscurecer(piedra, 0.8f));
    m.Caja(Vector3{ x - 0.9f, y + 3.81f, z - 0.5f }, Vector3{ 0.5f, 0.04f, 0.5f }, Emisivo(BRASA_FORJA));

    // Boca de la fragua y puerta hacia el camino.
    m.Caja(Vector3{ x + 1.61f, y + 0.7f, z - 0.6f }, Vector3{ 0.04f, 0.8f, 0.9f }, Emisivo(Color{ 255, 120, 40, 255 }));
    m.Caja(Vector3{ x + 1.61f, y + 0.85f, z + 0.65f }, Vector3{ 0.05f, 1.5f, 0.8f }, MADERA_OSCURA_FORJA);

    // Alero sobre el yunque.
    for (int lado = -1; lado <= 1; lado += 2)
    {
        m.Cilindro(
            Vector3{ x + 2.7f, y, z + 1.0f * lado }, Vector3{ x + 2.7f, y + 1.85f, z + 1.0f * lado },
            0.08f, 0.08f, 5, MADERA_FORJA
        );
    }

    m.TrianguloDobleCara(
        Vector3{ x + 1.6f, y + 2.15f, z - 1.25f }, Vector3{ x + 2.9f, y + 1.85f, z - 1.25f },
        Vector3{ x + 2.9f, y + 1.85f, z + 1.25f }, teja
    );

    m.TrianguloDobleCara(
        Vector3{ x + 1.6f, y + 2.15f, z - 1.25f }, Vector3{ x + 2.9f, y + 1.85f, z + 1.25f },
        Vector3{ x + 1.6f, y + 2.15f, z + 1.25f }, teja
    );

    m.Caja(Vector3{ x + 2.2f, y + 0.3f, z + 0.3f }, Vector3{ 0.5f, 0.6f, 0.4f }, MADERA_OSCURA_FORJA);
    m.Caja(Vector3{ x + 2.2f, y + 0.7f, z + 0.3f }, Vector3{ 0.8f, 0.22f, 0.32f }, HIERRO_FORJA);

    PiezaBarril(m, x + 0.9f, y, z + 2.0f);
    PiezaBarril(m, x + 0.2f, y, z + 2.15f);
    PiezaCajon(m, x - 0.8f, y, z + 2.05f, 15.0f, 0.6f);
    PiezaCajon(m, x - 2.3f, y, z - 0.6f, 40.0f, 0.7f);
}


// Boca de la mina en la cresta al este de la galeria, farolas y vagonetas.
static void ConstruirBocaMina(ConstructorMalla& m)
{
    const float x = BOCA_MINA_X;
    const float z = BOCA_MINA_Z;

    // La cresta cabe justo entre la galeria y la caverna de la mina.
    PiezaRoca(m, x, -0.1f, z, 1.1f, 20.0f, Color{ 84, 74, 76, 255 });
    PiezaRoca(m, x + 0.1f, -0.1f, z - 1.3f, 0.85f, 70.0f, Color{ 74, 66, 70, 255 });
    PiezaRoca(m, x + 0.1f, -0.1f, z + 1.3f, 0.8f, 130.0f, Color{ 78, 70, 74, 255 });

    // Portal de madera mirando a los rieles.
    for (int lado = -1; lado <= 1; lado += 2)
    {
        m.Caja(Vector3{ x - 1.1f, 0.6f, z + 0.55f * lado }, Vector3{ 0.2f, 1.2f, 0.2f }, MADERA_FORJA);
    }

    m.Caja(Vector3{ x - 1.1f, 1.25f, z }, Vector3{ 0.26f, 0.2f, 1.5f }, MADERA_OSCURA_FORJA);
    m.Caja(Vector3{ x - 1.02f, 0.58f, z }, Vector3{ 0.1f, 1.1f, 0.9f }, Color{ 22, 18, 20, 255 });

    // Farolas a lo largo de la galeria (la luz es animada).
    for (const auto& f : FAROLAS_FORJA)
    {
        float y = AlturaSuelo(f[0], f[1]);

        m.Cilindro(Vector3{ f[0], y, f[1] }, Vector3{ f[0], y + ALTO_FAROLA, f[1] }, 0.06f, 0.05f, 5, MADERA_OSCURA_FORJA);
        m.Caja(Vector3{ f[0], y + ALTO_FAROLA + 0.15f, f[1] }, Vector3{ 0.28f, 0.34f, 0.28f }, HIERRO_FORJA);
        m.Caja(Vector3{ f[0], y + ALTO_FAROLA + 0.15f, f[1] }, Vector3{ 0.3f, 0.2f, 0.18f }, Emisivo(Color{ 255, 214, 130, 255 }));
    }

    // Vagonetas de la mina de cristal, cargadas.
    PiezaVagoneta(m, 15.4f, AlturaSuelo(15.4f, -8.0f), -8.0f, 25.0f, Color{ 120, 200, 240, 255 });
    PiezaVagoneta(m, 25.6f, AlturaSuelo(25.6f, 5.6f), 5.6f, 70.0f, Color{ 180, 130, 240, 255 });
}


// Gran fumarola: chimenea de azufre al oeste del lobulo.
static void ConstruirGranFumarola(ConstructorMalla& m)
{
    const float x = GRAN_FUMAROLA_X;
    const float z = GRAN_FUMAROLA_Z;
    const float y = AlturaSuelo(x, z);

    // Chimenea esbelta con anillos de azufre (no debe leerse como el volcan).
    m.Cilindro(Vector3{ x, y - 0.1f, z }, Vector3{ x, y + 0.9f, z }, 2.0f, 1.2f, 9, Color{ 118, 100, 80, 255 });
    m.Cilindro(Vector3{ x, y + 0.9f, z }, Vector3{ x, y + 3.9f, z }, 1.05f, 0.6f, 9, Color{ 150, 132, 92, 255 });

    for (int k = 0; k < 3; k++)
    {
        float h = y + 1.4f + 0.9f * k;
        float r = 1.05f - 0.15f * (k + 0.55f);

        m.Cilindro(Vector3{ x, h, z }, Vector3{ x, h + 0.18f, z }, r + 0.08f, r + 0.05f, 9, Color{ 214, 192, 80, 255 });
    }

    m.Cilindro(Vector3{ x, y + 3.85f, z }, Vector3{ x, y + 4.1f, z }, 0.7f, 0.64f, 9, Color{ 228, 206, 82, 255 });
    m.Cilindro(Vector3{ x, y + 4.1f, z }, Vector3{ x, y + 4.12f, z }, 0.42f, 0.42f, 9, Emisivo(Color{ 255, 150, 60, 255 }));

    for (int k = 0; k < 6; k++)
    {
        float a = k * 1.05f + 0.4f;

        PiezaAzufre(m, x + std::cos(a) * 2.4f, y, z + std::sin(a) * 2.4f, 1.1f, Hash2(k, 9));
    }

    PiezaFumarola(m, x + 2.0f, y, z + 1.4f, 0.9f);
}


// Cristal gigante (margen este) y grupo dentro del lazo de la mina.
static void ConstruirCristales(ConstructorMalla& m)
{
    struct GrupoGigante
    {
        float x;
        float z;
        float alto;
        float radio;
        int piezas;
    };

    const GrupoGigante grupos[2] =
    {
        { GRAN_CRISTAL_X, GRAN_CRISTAL_Z, 9.0f, 0.9f, 7 },
        { CRISTAL_LAZO_X, CRISTAL_LAZO_Z, 3.4f, 0.42f, 4 }
    };

    for (const GrupoGigante& g : grupos)
    {
        float y = AlturaSuelo(g.x, g.z);

        PiezaRoca(m, g.x, y - 0.2f, g.z, g.radio * 1.7f, 30.0f, Color{ 58, 60, 84, 255 });

        PiezaCristal(
            m, Vector3{ g.x, y, g.z }, 0.05f, -0.04f,
            g.alto, g.radio, Color{ 90, 210, 250, 255 }
        );

        for (int k = 1; k < g.piezas; k++)
        {
            float a = k * 6.2831853f / (g.piezas - 1) + 0.3f;
            float abre = 0.28f;

            PiezaCristal(
                m,
                Vector3{ g.x + std::cos(a) * g.radio * 0.9f, y, g.z + std::sin(a) * g.radio * 0.9f },
                std::cos(a) * abre, std::sin(a) * abre,
                g.alto * (0.35f + 0.3f * Hash2(k, 4)),
                g.radio * (0.45f + 0.2f * Hash2(k, 6)),
                TonoCristal(Hash2(k, 8))
            );
        }
    }
}


// Un cartel por bifurcacion: una tabla por camino, apuntando a su primera
// casilla (la clara es la opcion 0, la rojiza el atajo).
static void ConstruirCartelesForja(ConstructorMalla& m)
{
    const Tablero& t = TableroReferenciaForja();

    const float lugares[6][2] =
    {
        { -1.8f, 1.4f }, { 1.8f, 1.4f },
        { -2.3f, -0.6f }, { 2.3f, -0.6f },
        { -1.7f, -1.9f }, { 1.7f, -1.9f }
    };

    for (int i = 0; i < t.cantidadCasillas; i++)
    {
        const Casilla& c = t.casillas[i];

        if (c.cantidadConexiones < 2)
        {
            continue;
        }

        // El primer lugar libre (fuera del sendero y de la lava).
        float px = c.posicion.x + lugares[0][0];
        float pz = c.posicion.z + lugares[0][1];

        for (const auto& l : lugares)
        {
            float x = c.posicion.x + l[0];
            float z = c.posicion.z + l[1];

            if (
                DistanciaSenderoForja(x, z) >= 2.0f &&
                BordeLavaForja(x, z) > 0.6f &&
                DespejadoForja(x, z, 0.2f)
            )
            {
                px = x;
                pz = z;
                break;
            }
        }

        float y = AlturaSuelo(px, pz);

        m.Cilindro(Vector3{ px, y, pz }, Vector3{ px, y + 1.9f, pz }, 0.08f, 0.07f, 5, MADERA_OSCURA_FORJA);

        for (int o = 0; o < 2; o++)
        {
            const Casilla* destino = t.ObtenerCasilla(c.conexiones[o].destino);

            if (destino == nullptr)
            {
                continue;
            }

            float dx = destino->posicion.x - c.posicion.x;
            float dz = destino->posicion.z - c.posicion.z;
            float giro = std::atan2(-dz, dx);
            float ux = std::cos(giro);
            float uz = -std::sin(giro);

            Color tabla =
                o == 0
                ? Color{ 196, 150, 96, 255 }
                : Color{ 214, 110, 86, 255 };

            float ty = y + 1.6f - 0.4f * o;

            CajaGirada(m, px + ux * 0.55f, ty, pz + uz * 0.55f, giro * RAD2DEG, Vector3{ 1.1f, 0.22f, 0.07f }, tabla);
            CajaGirada(m, px + ux * 1.15f, ty, pz + uz * 1.15f, giro * RAD2DEG, Vector3{ 0.2f, 0.12f, 0.07f }, tabla);
        }
    }
}


static void ConstruirMallaLandmarks()
{
    static ConstructorMalla m;
    m.Limpiar();

    ConstruirCalzadasForja(m);
    ConstruirColumnasLago(m);
    ConstruirCrisol(m);
    ConstruirYunque(m);
    ConstruirCastillete(m);
    ConstruirCampamento(m);
    ConstruirHerreria(m);
    ConstruirBocaMina(m);
    ConstruirGranFumarola(m);
    ConstruirCristales(m);
    ConstruirCartelesForja(m);

    IluminarMalla(m, 0);

    ActualizarMallaGpu(mallaLandmarks, m);
}


static void DibujarLandmarksForja()
{
    if (!mallaLandmarks.cargada)
    {
        ConstruirMallaLandmarks();
    }

    DibujarMallaGpu(mallaLandmarks);
}


//==================================================
// ANIMACIONES (modo inmediato: solo lo que se mueve o es translucido)
//==================================================
//
// Corrientes y burbujas de lava, chorro del crisol, fuego de la fogata,
// brasas, bombas de la gran erupcion, humo y vapor, y brillos de farolas y
// cristales. En ALTA son ~230 primitivas por frame; los efectos y las
// particulas se recortan con la calidad grafica.

// Punto a una distancia "s" a lo largo de un rio (y su direccion).
static void PuntoRioForja(
    const RioForja& rio,
    float s,
    float& x, float& z,
    float& tx, float& tz
)
{
    for (int i = 0; i + 1 < rio.cantidad; i++)
    {
        float dx = rio.puntos[i + 1].x - rio.puntos[i].x;
        float dz = rio.puntos[i + 1].z - rio.puntos[i].z;
        float largo = std::sqrt(dx * dx + dz * dz);

        if (s <= largo || i + 2 == rio.cantidad)
        {
            float u = largo > 0.0f ? s / largo : 0.0f;

            x = rio.puntos[i].x + dx * u;
            z = rio.puntos[i].z + dz * u;
            tx = largo > 0.0f ? dx / largo : 1.0f;
            tz = largo > 0.0f ? dz / largo : 0.0f;
            return;
        }

        s -= largo;
    }

    x = rio.puntos[0].x;
    z = rio.puntos[0].z;
    tx = 1.0f;
    tz = 0.0f;
}


static float LargoRioForja(const RioForja& rio)
{
    float total = 0.0f;

    for (int i = 0; i + 1 < rio.cantidad; i++)
    {
        float dx = rio.puntos[i + 1].x - rio.puntos[i].x;
        float dz = rio.puntos[i + 1].z - rio.puntos[i].z;

        total += std::sqrt(dx * dx + dz * dz);
    }

    return total;
}


// Veta clara que avanza sobre la lava (cuadrilatero orientado).
static void VetaLava(
    float x, float z,
    float tx, float tz,
    float largo, float ancho,
    Color color
)
{
    float y = NIVEL_LAVA + 0.03f;
    float nx = -tz * ancho;
    float nz = tx * ancho;
    float lx = tx * largo * 0.5f;
    float lz = tz * largo * 0.5f;

    Vector3 a = { x - lx - nx, y, z - lz - nz };
    Vector3 b = { x - lx + nx, y, z - lz + nz };
    Vector3 c = { x + lx + nx, y, z + lz + nz };
    Vector3 d = { x + lx - nx, y, z + lz - nz };

    DrawTriangle3D(a, b, c, color);
    DrawTriangle3D(a, c, d, color);
}


// Corrientes de los rios (bajan hacia el mar o la poza) y remolino lento
// alrededor de los islotes del lago.
static void DibujarCorrientesLava(float tiempo)
{
    NivelCalidadGrafica efectos = CalidadEfectos();

    if (efectos == CALIDAD_BAJA)
    {
        return;
    }

    int porRio = efectos == CALIDAD_ALTA ? 9 : 5;

    rlDisableBackfaceCulling();

    for (int r = 0; r < CANTIDAD_RIOS_FORJA; r++)
    {
        const RioForja& rio = RIOS_FORJA[r];
        float largo = LargoRioForja(rio);

        for (int k = 0; k < porRio; k++)
        {
            float s = std::fmod(tiempo * 1.1f + largo * (k + 0.37f * Hash2(k, r)) / porRio, largo);
            float x, z, tx, tz;

            PuntoRioForja(rio, s, x, z, tx, tz);

            float lateral = (Hash2(k, r + 7) - 0.5f) * rio.semiancho;
            x += -tz * lateral;
            z += tx * lateral;

            VetaLava(x, z, tx, tz, 1.1f, 0.07f + 0.05f * rio.semiancho, Color{ 255, 196, 84, 255 });
        }
    }

    int remolino = efectos == CALIDAD_ALTA ? 14 : 8;

    for (int k = 0; k < remolino; k++)
    {
        float a = tiempo * 0.12f + k * 6.2831853f / remolino;
        float escala = 0.55f + 0.3f * Hash2(k, 3);
        float x = LAGO_X + std::cos(a) * LAGO_RX * escala;
        float z = LAGO_Z + std::sin(a) * LAGO_RZ * escala;

        // El canal del puente queda libre de vetas (alli van las columnas).
        if (std::fabs(x) < CANAL_SEMIANCHO + 0.6f)
        {
            continue;
        }

        VetaLava(x, z, -std::sin(a), std::cos(a), 1.3f, 0.08f, Color{ 255, 176, 66, 255 });
    }

    rlEnableBackfaceCulling();
}


// Burbujas que se hinchan y revientan en el lago, la poza y los rios.
static void DibujarBurbujasLava(float tiempo)
{
    if (CalidadEfectos() == CALIDAD_BAJA)
    {
        return;
    }

    const float lugares[10][2] =
    {
        { -6.2f, -0.4f }, { -2.9f, -4.2f }, { 3.0f, -4.6f }, { 6.4f, -3.6f },
        { -3.2f, 0.9f }, { 2.6f, 0.4f },
        { POZA_X, POZA_Z }, { POZA_X - 0.5f, POZA_Z + 0.4f },
        { -13.0f, -28.0f }, { 20.0f, -27.0f }
    };

    for (int k = 0; k < 10; k++)
    {
        float fase = std::fmod(tiempo * 0.55f + Hash2(k, 11), 1.0f);
        float radio = 0.32f * Suave01(fase / 0.85f);

        if (fase > 0.85f)
        {
            continue;
        }

        DrawSphereEx(
            Vector3{ lugares[k][0], NIVEL_LAVA, lugares[k][1] },
            radio, 3, 6,
            Color{ 255, (unsigned char)(120 + 80 * fase), 40, 255 }
        );
    }
}


// Gotas que caen por el chorro del crisol y salpicadura abajo.
static void DibujarChorroCrisol(float tiempo)
{
    Vector3 pico = { CRISOL_X - 1.35f, 2.08f, CRISOL_Z };
    Vector3 fondo = { CRISOL_X - 1.55f, NIVEL_LAVA, CRISOL_Z };

    for (int k = 0; k < 4; k++)
    {
        float fase = std::fmod(tiempo * 1.4f + k * 0.25f, 1.0f);
        float caida = fase * fase;

        DrawSphereEx(
            Vector3{
                pico.x + (fondo.x - pico.x) * caida,
                pico.y + (fondo.y - pico.y) * caida,
                pico.z
            },
            0.12f, 3, 5,
            Color{ 255, 210, 90, 255 }
        );
    }

    float pulso = Parpadeo(tiempo, 6.0f);

    DrawCylinderEx(
        Vector3{ fondo.x, NIVEL_LAVA + 0.01f, fondo.z },
        Vector3{ fondo.x, NIVEL_LAVA + 0.05f, fondo.z },
        0.35f + 0.2f * pulso, 0.25f + 0.2f * pulso, 8,
        Color{ 255, 200, 80, 255 }
    );
}


// Fuego de la fogata del campamento.
static void DibujarFogata(float tiempo)
{
    float y = AlturaSuelo(FOGATA_X, FOGATA_Z) + 0.1f;
    float a = Parpadeo(tiempo, 9.0f);
    float b = Parpadeo(tiempo + 0.7f, 13.0f);

    DrawCylinderEx(
        Vector3{ FOGATA_X, y, FOGATA_Z },
        Vector3{ FOGATA_X + 0.05f * b, y + 0.75f + 0.25f * a, FOGATA_Z },
        0.32f, 0.0f, 7,
        Color{ 255, (unsigned char)(110 + 50 * a), 30, 255 }
    );

    DrawCylinderEx(
        Vector3{ FOGATA_X, y, FOGATA_Z },
        Vector3{ FOGATA_X - 0.04f * a, y + 0.45f + 0.15f * b, FOGATA_Z },
        0.18f, 0.0f, 6,
        Color{ 255, 226, 120, 255 }
    );
}


// Brasas que suben del lago, la fogata, la herreria y el crater. En la
// gran erupcion (ultima ronda) son mas y el volcan escupe bombas.
static void DibujarBrasasForja(float tiempo, bool granErupcion)
{
    float factor = FactorCalidadGrafica(CalidadParticulas());
    int cantidad = (int)((granErupcion ? 110.0f : 70.0f) * factor);
    float yCrater = AlturaCraterForja();

    for (int i = 0; i < cantidad; i++)
    {
        float fase = std::fmod(tiempo * (0.18f + 0.12f * Hash2(i, 2)) + Hash2(i, 1), 1.0f);
        int fuente = i % 7;

        float x;
        float z;
        float y0;
        float subida;

        if (fuente < 4)
        {
            float a = Hash2(i, 3) * 6.2831853f;
            float r = std::sqrt(Hash2(i, 4)) * 0.85f;
            x = LAGO_X + std::cos(a) * LAGO_RX * r;
            z = LAGO_Z + std::sin(a) * LAGO_RZ * r;
            y0 = NIVEL_LAVA;
            subida = 3.5f;
        }
        else if (fuente == 4)
        {
            x = FOGATA_X + (Hash2(i, 3) - 0.5f) * 0.4f;
            z = FOGATA_Z + (Hash2(i, 4) - 0.5f) * 0.4f;
            y0 = 0.4f;
            subida = 2.2f;
        }
        else if (fuente == 5)
        {
            x = HERRERIA_X - 0.9f + (Hash2(i, 3) - 0.5f) * 0.4f;
            z = HERRERIA_Z - 0.5f + (Hash2(i, 4) - 0.5f) * 0.4f;
            y0 = 3.9f;
            subida = 2.5f;
        }
        else
        {
            float a = Hash2(i, 3) * 6.2831853f;
            x = VOLCAN_X + std::cos(a) * VOLCAN_CRATER * 0.7f;
            z = VOLCAN_Z + std::sin(a) * VOLCAN_CRATER * 0.7f;
            y0 = yCrater;
            subida = granErupcion ? 9.0f : 5.0f;
        }

        x += std::sin(tiempo * 1.7f + i) * 0.35f * fase;

        float lado = 0.07f + 0.05f * (1.0f - fase);

        DrawCubeV(
            Vector3{ x, y0 + subida * fase, z },
            Vector3{ lado, lado, lado },
            Color{ 255, (unsigned char)(200 - 110 * fase), 50, 255 }
        );
    }

    if (!granErupcion)
    {
        return;
    }

    // Bombas de lava en parabola desde el crater hacia las laderas.
    for (int k = 0; k < 6; k++)
    {
        float fase = std::fmod(tiempo * 0.4f + k / 6.0f, 1.0f);
        float a = Hash2(k, 21) * 6.2831853f;
        float alcance = 6.0f + 6.0f * Hash2(k, 22);

        float x = VOLCAN_X + std::cos(a) * alcance * fase;
        float z = VOLCAN_Z + std::sin(a) * alcance * fase;
        float y = yCrater + 9.0f * fase * (1.0f - fase) * 4.0f - 6.0f * fase * fase;

        DrawSphereEx(Vector3{ x, y, z }, 0.38f, 3, 6, Color{ 255, 140, 40, 255 });
    }
}


// Bocanada de humo o vapor que sube, crece y se desvanece.
static void Bocanada(
    Vector3 origen,
    float fase,
    float subida,
    float deriva,
    float radio0, float radio1,
    Color color
)
{
    color.a = (unsigned char)(color.a * (1.0f - fase) * Suave01(fase / 0.15f));

    if (color.a < 4)
    {
        return;
    }

    DrawSphereEx(
        Vector3{ origen.x + deriva * fase, origen.y + subida * fase, origen.z - 0.4f * deriva * fase },
        radio0 + (radio1 - radio0) * fase,
        4, 6,
        color
    );
}


// Humo del crater y de la chimenea de la herreria; vapor de las
// fumarolas. Es translucido: no escribe profundidad.
static void DibujarHumoForja(float tiempo, bool granErupcion)
{
    float factor = FactorCalidadGrafica(CalidadParticulas());

    rlDrawRenderBatchActive();
    rlDisableDepthMask();

    // Penacho del volcan (mas oscuro y alto en la gran erupcion).
    int penacho = (int)(16.0f * factor) + 2;
    Vector3 crater = { VOLCAN_X, AlturaCraterForja() + 0.4f, VOLCAN_Z };

    for (int k = 0; k < penacho; k++)
    {
        float fase = std::fmod(tiempo * 0.07f + (float)k / penacho, 1.0f);

        Bocanada(
            crater, fase,
            granErupcion ? 14.0f : 10.0f, 7.0f,
            1.2f, 4.2f,
            granErupcion ? Color{ 60, 44, 44, 190 } : Color{ 92, 82, 86, 150 }
        );
    }

    // Resplandor del crater.
    float pulso = Parpadeo(tiempo, 1.6f);

    DrawCylinderEx(
        Vector3{ VOLCAN_X, crater.y - 0.3f, VOLCAN_Z },
        Vector3{ VOLCAN_X, crater.y + 0.6f, VOLCAN_Z },
        VOLCAN_CRATER * 0.9f, VOLCAN_CRATER * 1.2f, 12,
        Color{ 255, 120, 40, (unsigned char)(50 + 50 * pulso) }
    );

    // Chimenea de la herreria.
    Vector3 chimenea = { HERRERIA_X - 0.9f, AlturaSuelo(HERRERIA_X, HERRERIA_Z) + 3.9f, HERRERIA_Z - 0.5f };
    int humoChimenea = (int)(6.0f * factor) + 1;

    for (int k = 0; k < humoChimenea; k++)
    {
        float fase = std::fmod(tiempo * 0.22f + (float)k / humoChimenea, 1.0f);
        Bocanada(chimenea, fase, 3.2f, 1.2f, 0.25f, 0.8f, Color{ 80, 74, 76, 150 });
    }

    // Vapor de la gran fumarola y de las fumarolas pequenas.
    const Color vapor = { 236, 230, 204, 120 };
    Vector3 granBoca = { GRAN_FUMAROLA_X, AlturaSuelo(GRAN_FUMAROLA_X, GRAN_FUMAROLA_Z) + 4.15f, GRAN_FUMAROLA_Z };
    int vaporGrande = (int)(9.0f * factor) + 1;

    for (int k = 0; k < vaporGrande; k++)
    {
        float fase = std::fmod(tiempo * 0.3f + (float)k / vaporGrande, 1.0f);
        Bocanada(granBoca, fase, 4.0f, 1.5f, 0.4f, 1.4f, vapor);
    }

    int bocas = 0;
    int maxBocas = (int)(10.0f * factor) + 1;

    for (int i = 0; i < cantidadPropsForja && bocas < maxBocas; i++)
    {
        const PropForja& p = propsForja[i];

        if (p.tipo != PROP_FUMAROLA)
        {
            continue;
        }

        Vector3 boca = { p.x, p.y + 0.8f * p.escala, p.z };

        for (int k = 0; k < 3; k++)
        {
            float fase = std::fmod(tiempo * 0.35f + k / 3.0f + Hash2(i, 5), 1.0f);
            Bocanada(boca, fase, 2.0f, 0.6f, 0.18f, 0.6f, vapor);
        }

        bocas++;
    }

    // Halo de las farolas y de los cristales gigantes.
    for (const auto& f : FAROLAS_FORJA)
    {
        float brillo = 0.75f + 0.25f * Parpadeo(tiempo + f[0], 11.0f) * Parpadeo(tiempo * 0.7f + f[1], 7.0f);

        DrawSphereEx(
            Vector3{ f[0], AlturaSuelo(f[0], f[1]) + ALTO_FAROLA + 0.15f, f[1] },
            0.45f, 4, 6,
            Color{ 255, 210, 120, (unsigned char)(70 * brillo) }
        );
    }

    float latido = Parpadeo(tiempo, 1.3f);

    DrawSphereEx(
        Vector3{ GRAN_CRISTAL_X, AlturaSuelo(GRAN_CRISTAL_X, GRAN_CRISTAL_Z) + 7.6f, GRAN_CRISTAL_Z },
        1.6f + 0.3f * latido, 5, 8,
        Color{ 120, 220, 255, (unsigned char)(30 + 30 * latido) }
    );

    DrawSphereEx(
        Vector3{ CRISTAL_LAZO_X, AlturaSuelo(CRISTAL_LAZO_X, CRISTAL_LAZO_Z) + 2.8f, CRISTAL_LAZO_Z },
        0.8f + 0.15f * latido, 4, 6,
        Color{ 120, 220, 255, (unsigned char)(30 + 30 * latido) }
    );

    rlDrawRenderBatchActive();
    rlEnableDepthMask();
}


//==================================================
// RECURSOS
//==================================================

static void DescargarRecursosForjaVolcan()
{
    DescargarMalla(mallaTerreno);
    DescargarMalla(mallaCordillera);
    DescargarMalla(mallaSenderos);
    nivelSenderosConstruido = -1;
    DescargarMalla(mallaProps);
    nivelPropsConstruido = -1;
    DescargarMalla(mallaLandmarks);
}


//==================================================
// DECORACION
//==================================================

static void DibujarDecoracionForjaVolcan(
    const PartidaTablero& partida
)
{
    float tiempo = partida.tiempoTablero;

    // La vista previa no tiene rondas: ahi no hay gran erupcion.
    bool granErupcion =
        partida.cantidadRondas > 0 &&
        EsGranErupcion(partida);

    // MODELO FUTURO: la isla volcanica completa como GLB.
    if (!mallaCordillera.cargada)
    {
        ConstruirMallaCordillera();
    }

    // Estatico (mallas horneadas).
    DibujarMallaGpu(mallaCordillera);
    DibujarTerrenoForja();
    DibujarMarDeLava(tiempo);
    DibujarSenderosForja();
    DibujarPropsForja();
    DibujarLandmarksForja();

    // Animado.
    DibujarCorrientesLava(tiempo);
    DibujarBurbujasLava(tiempo);
    DibujarChorroCrisol(tiempo);
    DibujarFogata(tiempo);
    DibujarBrasasForja(tiempo, granErupcion);

    // Translucido al final.
    DibujarHumoForja(tiempo, granErupcion);
}


// Calor del volcan: degradado rojizo con brasas y ceniza que cruzan la
// pantalla. En la gran erupcion el cielo late en rojo.
static void DibujarCapaPantallaForjaVolcan(
    const PartidaTablero& partida
)
{
    int ancho = GetScreenWidth();
    int alto = GetScreenHeight();
    float t = partida.tiempoTablero;
    float escala = alto / 720.0f;

    bool granErupcion =
        partida.cantidadRondas > 0 &&
        EsGranErupcion(partida);

    float latido = granErupcion ? Parpadeo(t, 2.2f) : 0.0f;

    DrawRectangleGradientV(
        0, 0, ancho, alto / 2,
        Color{ 60, 20, 24, (unsigned char)(70 + 50 * latido) },
        Color{ 60, 20, 24, 0 }
    );

    DrawRectangleGradientV(
        0, alto / 2, ancho, alto - alto / 2,
        Color{ 255, 110, 40, 0 },
        Color{ 255, 110, 40, (unsigned char)(38 + 40 * latido) }
    );

    float factor = FactorCalidadGrafica(CalidadParticulas());
    int brasas = (int)std::ceil((granErupcion ? 60.0f : 34.0f) * factor);

    for (int i = 0; i < brasas; i++)
    {
        float vy = 30.0f + 50.0f * Hash2(i, 5);
        float vx = 8.0f + 14.0f * Hash2(i, 3);

        float x = std::fmod(Hash2(i, 1) * ancho + t * vx, (float)ancho + 20.0f) - 10.0f;
        float y = alto + 10.0f - std::fmod(Hash2(i, 2) * alto + t * vy, (float)alto + 20.0f);

        x += 14.0f * std::sin(t * 1.4f + i);

        // Se apagan al subir.
        float vida = Limitar01(y / alto);
        float lado = (2.0f + 2.5f * Hash2(i, 4)) * escala;

        DrawRectangle(
            (int)x, (int)y, (int)lado, (int)lado,
            ConAlfa(Color{ 255, (unsigned char)(120 + 90 * Hash2(i, 6)), 40, 255 }, 0.85f * vida)
        );
    }
}

//==================================================
// GIMMICK: PUENTE LEVADIZO Y LLAMAS
//==================================================

// Tablon del puente articulado en el borde de la bifurcacion, mirando a la
// primera casilla del puente. gimmickAnim 0 = bajado, 1 = levantado.
static void DibujarPuenteLevadizo(
    const PartidaTablero& partida
)
{
    const Casilla* base =
        partida.tablero.ObtenerCasilla(nodosForja.puenteBifurcacion);

    const Casilla* primera =
        partida.tablero.ObtenerCasilla(nodosForja.puente[0]);

    if (base == nullptr || primera == nullptr)
    {
        return;
    }

    float dx = primera->posicion.x - base->posicion.x;
    float dz = primera->posicion.z - base->posicion.z;
    float distancia = std::sqrt(dx * dx + dz * dz);

    if (distancia < 0.01f)
    {
        return;
    }

    // Del borde de una losa al borde de la otra (con un poco de apoyo).
    const float BISAGRA = 0.80f;
    float largo = distancia - BISAGRA - 0.70f;

    float giro = std::atan2(-dx, -dz) * RAD2DEG;
    float angulo = partida.gimmickAnim * 78.0f;
    float radianes = angulo * DEG2RAD;

    const float Y_BISAGRA = 0.47f;

    rlPushMatrix();
    rlTranslatef(
        base->posicion.x + dx / distancia * BISAGRA,
        Y_BISAGRA,
        base->posicion.z + dz / distancia * BISAGRA
    );
    rlRotatef(giro, 0.0f, 1.0f, 0.0f);

    // En este espacio el tablon avanza hacia -z.
    rlPushMatrix();
    rlRotatef(angulo, 1.0f, 0.0f, 0.0f);

    DrawCubeV(
        Vector3{ 0.0f, 0.06f, -largo * 0.5f },
        Vector3{ 2.0f, 0.14f, largo },
        Color{ 128, 84, 52, 255 }
    );

    // Flejes de hierro.
    for (int k = 0; k < 3; k++)
    {
        DrawCubeV(
            Vector3{ 0.0f, 0.14f, -largo * (0.2f + 0.3f * k) },
            Vector3{ 2.04f, 0.03f, 0.12f },
            Color{ 60, 56, 62, 255 }
        );
    }

    DrawCubeWiresV(
        Vector3{ 0.0f, 0.06f, -largo * 0.5f },
        Vector3{ 2.0f, 0.14f, largo },
        Color{ 50, 30, 20, 255 }
    );

    rlPopMatrix();

    // Torres del mecanismo con luz de estado y cadenas a la punta.
    Vector3 punta =
    {
        0.0f,
        largo * std::sin(radianes) + 0.1f,
        -largo * std::cos(radianes)
    };

    bool cerrado = partida.gimmickAnim > 0.5f;

    Color luz =
        cerrado
        ? Color{ 235, 60, 50, 255 }
        : Color{ 80, 230, 110, 255 };

    for (int lado = -1; lado <= 1; lado += 2)
    {
        Vector3 torre = { lado * 1.45f, 0.75f, 0.15f };

        DrawCube(torre, 0.5f, 2.0f, 0.5f, Color{ 86, 78, 84, 255 });
        DrawCube(Vector3{ torre.x, 1.85f, torre.z }, 0.66f, 0.22f, 0.66f, Color{ 60, 54, 60, 255 });

        Vector3 cima = { torre.x, 2.15f, torre.z };

        DrawSphere(cima, 0.20f, luz);

        Vector3 extremo = { lado * 0.95f, punta.y, punta.z };

        DrawLine3D(Vector3{ torre.x, 1.8f, torre.z }, extremo, Color{ 40, 40, 46, 255 });
        DrawLine3D(Vector3{ torre.x * 0.98f, 1.75f, torre.z }, extremo, Color{ 70, 70, 76, 255 });
    }

    rlPopMatrix();
}


static void DibujarLlamaForja(
    const PartidaTablero& partida,
    int indice,
    float desfase
)
{
    const Casilla* casilla =
        partida.tablero.ObtenerCasilla(indice);

    if (casilla == nullptr)
    {
        return;
    }

    float pulso = Parpadeo(partida.tiempoTablero + desfase, 7.0f);

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

    DrawCylinder(
        base,
        0.02f,
        0.30f * escala,
        (1.0f + 0.4f * pulso) * escala,
        8,
        Color{ 255, 226, 120, 255 }
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
        DibujarLlamaForja(partida, partida.casillaPeligro, 0.0f);
    }

    if (llamaEsteForja >= 0)
    {
        DibujarLlamaForja(partida, llamaEsteForja, 1.7f);
    }
}


//==================================================
// GIMMICK
//==================================================

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


// Casilla basica de una mitad del mapa para una llama (-1 si no hay).
// Nunca el inicio, el trofeo, la llama anterior de esa mitad ni la otra.
static int ElegirLlamaForja(
    const PartidaTablero& partida,
    bool mitadOeste,
    int anterior,
    int otraLlama
)
{
    int candidatas[MAX_CASILLAS_TABLERO];
    int cantidad = 0;

    for (int i = 0; i < partida.tablero.cantidadCasillas; i++)
    {
        const Casilla* casilla = partida.tablero.ObtenerCasilla(i);

        if (
            !EsCasillaBasicaForja(casilla) ||
            (casilla->posicion.x < CENTRO_CASILLAS_X) != mitadOeste ||
            i == nodosForja.inicio ||
            i == partida.casillaTrofeo ||
            i == anterior ||
            i == otraLlama
        )
        {
            continue;
        }

        candidatas[cantidad++] = i;
    }

    if (cantidad <= 0)
    {
        return -1;
    }

    return candidatas[GetRandomValue(0, cantidad - 1)];
}


static void AlIniciarRondaForjaVolcan(
    PartidaTablero& partida
)
{
    bool levantado =
        partida.rondaActual % 2 == 0;

    partida.gimmickActivo = levantado;

    // Con el puente levantado la bifurcacion solo conserva el anillo.
    // La conexion 1 sigue guardada y se reabre al bajar el puente.
    int bifurcacion = nodosForja.puenteBifurcacion;

    if (bifurcacion >= 0 && bifurcacion < partida.tablero.cantidadCasillas)
    {
        partida.tablero.casillas[bifurcacion].cantidadConexiones =
            levantado
            ? 1
            : 2;
    }

    // Dos llamas: una en cada mitad del mapa.
    partida.casillaPeligro =
        ElegirLlamaForja(partida, true, partida.casillaPeligro, -1);

    llamaEsteForja =
        ElegirLlamaForja(partida, false, llamaEsteForja, partida.casillaPeligro);

    bool gran = EsGranErupcion(partida);

    std::snprintf(
        partida.estadoGimmick,
        sizeof(partida.estadoGimmick),
        "%s | %s -%d MONEDAS",
        levantado
            ? "PUENTE LEVANTADO (ATAJO CERRADO)"
            : "PUENTE BAJADO (ATAJO ABIERTO)",
        gran
            ? "GRAN ERUPCIÓN: 2 LLAMAS"
            : "2 LLAMAS DEL VOLCÁN:",
        PerdidaErupcion(partida)
    );

    char mensaje[96];

    if (gran)
    {
        std::snprintf(
            mensaje,
            sizeof(mensaje),
            "¡GRAN ERUPCIÓN! 2 LLAMAS QUITAN %d. %s",
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


// Destino del geiser: una casilla basica cualquiera que no sea la actual,
// la del trofeo ni una llama.
static int ElegirDestinoGeiserForja(
    const PartidaTablero& partida,
    int actual
)
{
    int candidatas[MAX_CASILLAS_TABLERO];
    int cantidad = 0;

    for (int i = 0; i < partida.tablero.cantidadCasillas; i++)
    {
        if (
            !EsCasillaBasicaForja(partida.tablero.ObtenerCasilla(i)) ||
            i == actual ||
            i == partida.casillaTrofeo ||
            EsLlamaForja(partida, i)
        )
        {
            continue;
        }

        candidatas[cantidad++] = i;
    }

    if (cantidad <= 0)
    {
        return actual;
    }

    return candidatas[GetRandomValue(0, cantidad - 1)];
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
        // Geiser: dispara al jugador a una casilla basica de cualquier zona.
        int destino =
            ElegirDestinoGeiserForja(partida, jugador.casillaActual);

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

    if (EsLlamaForja(partida, jugador.casillaActual))
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

    // Las dos llamas del volcan mandan sobre el tipo de la casilla.
    if (EsLlamaForja(partida, indice))
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


// Con monedas para el trofeo, el bot toma el camino mas corto hasta el
// (BFS sobre las conexiones abiertas); sin ellas elige la rama con mejores
// casillas por delante. Siempre deja algo de azar (y ante un empate elige
// al azar).
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

    if (
        casilla < 0 ||
        casilla >= MAX_CASILLAS_TABLERO ||
        opcion < 0 ||
        opcion > 1
    )
    {
        return nullptr;
    }

    return descripcionRutaForja[casilla][opcion];
}


//==================================================
// ESTILO DE LAS LOSAS
//==================================================

// Losas de basalto oscuro con brasas en el borde (en lugar de musgo).
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
            "Dos llamas del volcán quitan 5 monedas (10 en la última ronda).";

        definicion.dificultad = "MEDIA";

        definicion.colorTema = Color{ 235, 100, 35, 255 };
        definicion.colorFondo = Color{ 40, 24, 36, 255 };

        // Camara diorama: objetivo en el centro de la caja de casillas
        // (45 x 35.5 de mundo), desfase (0, 17, 15.5). El mapa es grande:
        // la camara sigue al jugador y da una vista general cada ronda.
        definicion.camaraObjetivo = { CENTRO_CASILLAS_X, 0.0f, CENTRO_CASILLAS_Z };
        definicion.camaraPosicion = { CENTRO_CASILLAS_X, 17.0f, CENTRO_CASILLAS_Z + 15.5f };
        definicion.camaraFovy = 48.0f;
        definicion.camaraSeguimiento = 1.0f;
        definicion.camaraZoomGeneral = 2.5f;
        definicion.camaraEscalaVistaPrevia = 2.5f;

        // Landmark: el puente levadizo. El sistema comun lo enfoca cuando
        // gimmickActivo (puente levantado) y hay aviso de ronda, asi que la
        // camara muestra justo el mecanismo que acaba de cerrar el atajo.
        definicion.camaraLandmark = { 0.0f, 1.0f, 4.3f };
        definicion.tieneLandmark = true;

        definicion.estiloCasilla = ObtenerEstiloCasillaBasalto();

        definicion.nombresZonas = NOMBRES_ZONAS_FORJA;
        definicion.cantidadZonas = CANTIDAD_ZONAS_FORJA;

        // Candidatas del trofeo: las llena la construccion de la ruta.
        TableroReferenciaForja();

        definicion.casillasTrofeo = casillasTrofeoForja;
        definicion.cantidadCasillasTrofeo = cantidadTrofeosForja;

        definicion.costoTrofeo = 20;
        definicion.cantidadRondas = 5;

        definicion.Construir = ConstruirForjaVolcan;
        definicion.DibujarDecoracion = DibujarDecoracionForjaVolcan;
        definicion.DescargarRecursos = DescargarRecursosForjaVolcan;
        definicion.DibujarCapaPantalla = DibujarCapaPantallaForjaVolcan;
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
