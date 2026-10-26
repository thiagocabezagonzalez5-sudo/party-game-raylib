#include "Board/CatalogoTableros.h"

#include "Gameplay/PartidaTablero.h"

#include "Board/MallaTablero.h"
#include "Systems/CalidadGrafica.h"

#include "rlgl.h"

#include <cmath>
#include <cstdio>


//==================================================
// FERIA NOCTURNA (tablero 5)
//==================================================
//
// Una feria de noche de 57 casillas en seis zonas (ver RUTA DEL TABLERO).
// El corazon sigue siendo el OCHO: dos lazos que se cruzan en el centro,
// junto a LA GRAN RUEDA de la fortuna y su frasco de monedas. En cada
// bifurcacion la opcion 0 es la ruta larga y la 1 el atajo:
//
//   - ENTRADA (suroeste): el portal, la casilla de inicio y el lado
//     interior del lazo oeste.
//   - CALLE DE LAS CARPAS (lazo oeste): carpas de circo y un CALLEJON que
//     corta el lazo por el medio (atajo).
//   - LA NORIA (lazo este): la noria grande al fondo, puestos de comida y
//     un PASILLO entre los puestos (atajo).
//   - LA GRAN RUEDA (centro): la X del ocho. En el cruce se elige lazo.
//   - EL CARRUSEL (sur): lobulo que une el pie del lazo este con el del
//     oeste rodeando el carrusel; el atajo va directo al cruce.
//   - LA MONTAÑA RUSA (noroeste): lobulo largo al pie de la montaña rusa;
//     el atajo sigue por la calle de las carpas.
//
// Mecanica 1, EL POZO: lo que se pierde en las casillas rojas no
// desaparece, cae al frasco de la Gran Rueda (el Escudo lo bloquea).
// Mecanica 2, REMATE: desde la ronda 2 el precio del trofeo se sortea
// entre 15, 20 y 25 monedas (en la ultima ronda no baja de 20).
// Evento propio (casilla EVENTO, una o mas por zona): girar la Gran Rueda
// y llevarse todo el pozo (30 %), la mitad (45 %) o trabarla y pagar al
// pozo (25 %).
//
// PRESENTACION (diorama nocturno): lomas de pasto horneadas con caminos de
// tierra, telon de colinas, ciudad iluminada, luna y estrellas; carpas,
// puestos, noria, carrusel y montaña rusa horneados en mallas, guirnaldas
// de bombillas con luces que corren, y en modo inmediato solo lo animado
// (ruedas, vagon, caballitos, fuegos artificiales, globos y destellos).

static const int COSTO_TROFEO_FERIA = 20;
static const int POZO_INICIAL = 5;
static const int PAGO_RUEDA_TRABADA = 3;

// La rueda: 30 % todo el pozo, 45 % medio pozo, 25 % trabada.
static const int PROBABILIDAD_PREMIO_MAYOR = 30;
static const int PROBABILIDAD_MEDIO_POZO = 45;

static const int PRECIOS_REMATE[] = { 15, 20, 25 };
static const int PRECIO_MINIMO_ULTIMA_RONDA = 20;

static const float ALTURA_CASILLA_FERIA = 0.25f;

// Centro de la caja de casillas (objetivo de la camara).
static const float CENTRO_CASILLAS_X = -1.7f;
static const float CENTRO_CASILLAS_Z = -0.1f;

// Escenografia del gimmick.
static const float RUEDA_X = 0.0f;
static const float RUEDA_Z = -5.6f;
static const float RUEDA_ALTO_EJE = 3.2f;
static const float RUEDA_RADIO = 2.3f;
static const int GAJOS_RUEDA = 12;

static const float FRASCO_X = 0.0f;
static const float FRASCO_Z = 6.3f;
static const float FRASCO_RADIO = 0.95f;
static const float FRASCO_BASE = 0.45f;
static const float FRASCO_ALTO = 2.15f;


//==================================================
// ESTADO DEL GIMMICK
//==================================================
//
// Vive en este archivo y se reinicia en Construir (Reiniciar llama a
// Construir antes de AlIniciarRonda de la ronda 1).

static int monedasPozo = POZO_INICIAL;

// Solo visual: altura suavizada de las monedas, giro de la Gran Rueda y
// moneda que cae al frasco (1 = arriba, 0 = ya cayo).
static float pozoVisualFeria = (float)POZO_INICIAL;
static float anguloRuedaFeria = 0.0f;
static float caidaPozoFeria = 0.0f;


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


static Color MezclaFeria(Color a, Color b, float t)
{
    t = Limitar01(t);

    return Color
    {
        (unsigned char)(a.r + (b.r - a.r) * t),
        (unsigned char)(a.g + (b.g - a.g) * t),
        (unsigned char)(a.b + (b.b - a.b) * t),
        (unsigned char)(a.a + (b.a - a.a) * t)
    };
}


static Color OscurecerFeria(Color c, float factor)
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


static Color ConAlfaFeria(Color c, float alfa)
{
    c.a = (unsigned char)(255.0f * Limitar01(alfa));
    return c;
}


static float HashFeria(int a, int b)
{
    unsigned int h =
        (unsigned int)a * 374761393u +
        (unsigned int)b * 668265263u;

    h = (h ^ (h >> 13)) * 1274126177u;
    h ^= h >> 16;

    return (float)(h & 0xFFFFu) / 65535.0f;
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
// Las posiciones salen de un diseno validado (distancias 3.1-4.1 entre
// casillas conectadas, sin caminos que se toquen y nunca mas de 5 casillas
// seguidas sin especial). Los indices que el dibujo necesita se guardan en
// nodosFeria al construir: nada depende de un literal.

enum ZonaFeria
{
    ZONA_ENTRADA = 0,
    ZONA_CARPAS,
    ZONA_NORIA,
    ZONA_GRAN_RUEDA,
    ZONA_CARRUSEL,
    ZONA_MONTANA_RUSA,
    CANTIDAD_ZONAS_FERIA
};

static const char* const NOMBRES_ZONAS_FERIA[CANTIDAD_ZONAS_FERIA] =
{
    "ENTRADA",
    "CALLE DE LAS CARPAS",
    "LA NORIA",
    "LA GRAN RUEDA",
    "EL CARRUSEL",
    "LA MONTAÑA RUSA"
};

static const int CASILLAS_LAZO_FERIA = 11;
static const int CASILLAS_RUSA_FERIA = 9;
static const int CASILLAS_CARRUSEL_FERIA = 7;
static const int CASILLAS_ATAJO_FERIA = 3;
static const int MAX_TROFEOS_FERIA = 16;

// Casillas clave (las llena ConstruirRutaFeria).
struct NodosFeria
{
    int inicio = 0;

    // Lazos: esquina interior de arriba (TI), de abajo (BI) y casillas
    // por fuera (w1..w11 / e1..e11).
    int oesteArriba = 0;
    int oesteAbajo = 0;
    int esteArriba = 0;
    int esteAbajo = 0;

    int oeste[CASILLAS_LAZO_FERIA] = {};
    int este[CASILLAS_LAZO_FERIA] = {};

    // Lado interior del lazo oeste (sube de oesteAbajo a oesteArriba).
    int interior[2] = {};

    // La X del ocho.
    int cruce = 0;
    int cruceNoroeste = 0;
    int cruceNoreste = 0;
    int cruceSuroeste[2] = {};
    int cruceSureste[2] = {};

    // Lobulos y atajos.
    int rusa[CASILLAS_RUSA_FERIA] = {};
    int carrusel[CASILLAS_CARRUSEL_FERIA] = {};
    int callejon[CASILLAS_ATAJO_FERIA] = {};
    int pasillo[CASILLAS_ATAJO_FERIA] = {};
};

static NodosFeria nodosFeria;

static int casillasTrofeoFeria[MAX_TROFEOS_FERIA];
static int cantidadTrofeosFeria = 0;

static const char* descripcionRutaFeria[MAX_CASILLAS_TABLERO][2];


struct DatoNodoFeria
{
    char tipo;
    float x;
    float z;
};


// Tipo de casilla a partir de su letra en la tabla de diseno.
static TipoCasilla TipoDesdeLetra(char letra)
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


static int NodoFeria(
    Tablero& t,
    ZonaFeria zona,
    const DatoNodoFeria& dato
)
{
    int indice =
        t.AgregarCasilla(
            Vector3{ dato.x, ALTURA_CASILLA_FERIA, dato.z },
            TipoDesdeLetra(dato.tipo)
        );

    if (indice >= 0)
    {
        t.casillas[indice].zona = (int)zona;
    }

    return indice;
}


// Crea una tira de casillas de la misma zona y guarda sus indices.
static void TramoFeria(
    Tablero& t,
    ZonaFeria zona,
    const DatoNodoFeria* datos,
    int cantidad,
    int* indices
)
{
    for (int i = 0; i < cantidad; i++)
    {
        indices[i] = NodoFeria(t, zona, datos[i]);
    }
}


// Conecta en cadena: indices[0] -> indices[1] -> ... -> indices[n - 1].
static void EncadenarFeria(
    Tablero& t,
    const int* indices,
    int cantidad
)
{
    for (int i = 0; i + 1 < cantidad; i++)
    {
        t.ConectarCasillas(indices[i], indices[i + 1]);
    }
}


static void DefinirOpcionesFeria(
    int casilla,
    const char* opcion0,
    const char* opcion1
)
{
    if (casilla < 0 || casilla >= MAX_CASILLAS_TABLERO)
    {
        return;
    }

    descripcionRutaFeria[casilla][0] = opcion0;
    descripcionRutaFeria[casilla][1] = opcion1;
}


static void ConstruirRutaFeria(Tablero& t)
{
    t.cantidadCasillas = 0;
    t.recorridoValido = false;

    for (int i = 0; i < MAX_CASILLAS_TABLERO; i++)
    {
        t.casillas[i] = Casilla{};
        descripcionRutaFeria[i][0] = nullptr;
        descripcionRutaFeria[i][1] = nullptr;
    }

    cantidadTrofeosFeria = 0;

    NodosFeria& n = nodosFeria;

    // Letras: + verde, - roja, n neutra, E especial, R regalo, C caos,
    // I intercambio, D duelo, T tienda, V evento (la Gran Rueda).

    // --- Lazo oeste por fuera: w1..w11 (w8..w11 son la ENTRADA) ---
    static const DatoNodoFeria OESTE[CASILLAS_LAZO_FERIA] =
    {
        { '+',  -8.45f,  -6.69f },   // w1: salida a la montaña rusa
        { '-', -11.47f,  -8.36f },
        { 'n', -14.94f,  -8.56f },   // w3: salida al callejon
        { '-', -18.16f,  -7.37f },
        { '+', -20.59f,  -4.89f },   // w5: vuelta de la montaña rusa
        { 'V', -21.57f,  -1.58f },
        { 'T', -21.16f,   1.87f },   // puesto del mago (tienda)
        { '+', -19.44f,   4.86f },
        { 'n', -16.83f,   7.16f },   // w9: INICIO, vuelta del callejon
        { 'E', -13.69f,   8.59f },
        { '+', -10.66f,   7.03f }
    };

    const int INDICE_INICIO = 8;

    // El inicio se agrega primero: es la casilla 0.
    n.inicio = NodoFeria(t, ZONA_ENTRADA, OESTE[INDICE_INICIO]);

    for (int k = 0; k < CASILLAS_LAZO_FERIA; k++)
    {
        if (k == INDICE_INICIO)
        {
            n.oeste[k] = n.inicio;
            continue;
        }

        n.oeste[k] =
            NodoFeria(t, k >= 7 ? ZONA_ENTRADA : ZONA_CARPAS, OESTE[k]);
    }

    n.oesteArriba = NodoFeria(t, ZONA_CARPAS, DatoNodoFeria{ 'E', -6.40f, -3.90f });
    n.oesteAbajo = NodoFeria(t, ZONA_ENTRADA, DatoNodoFeria{ 'n', -7.65f, 5.25f });

    static const DatoNodoFeria INTERIOR[2] =
    {
        { '-', -8.63f,  2.05f },
        { 'V', -8.45f, -1.26f }
    };

    TramoFeria(t, ZONA_ENTRADA, INTERIOR, 2, n.interior);

    static const DatoNodoFeria CALLEJON[CASILLAS_ATAJO_FERIA] =
    {
        { '-', -14.62f, -4.49f },
        { 'I', -14.44f, -0.44f },
        { '-', -14.77f,  3.63f }
    };

    TramoFeria(t, ZONA_CARPAS, CALLEJON, CASILLAS_ATAJO_FERIA, n.callejon);

    // --- Lazo este por fuera: e1..e11 (e8..e11 son del CARRUSEL) ---
    static const DatoNodoFeria ESTE[CASILLAS_LAZO_FERIA] =
    {
        { '+',  8.45f, -6.69f },
        { '-', 11.47f, -8.36f },
        { 'V', 14.94f, -8.56f },     // e3: salida al pasillo
        { '+', 18.16f, -7.37f },
        { 'n', 20.59f, -4.89f },
        { 'D', 21.57f, -1.58f },
        { '+', 21.16f,  1.87f },
        { 'n', 19.44f,  4.86f },
        { '+', 16.83f,  7.16f },     // e9: vuelta del pasillo
        { 'I', 13.69f,  8.59f },
        { '-', 10.66f,  7.03f }
    };

    for (int k = 0; k < CASILLAS_LAZO_FERIA; k++)
    {
        n.este[k] =
            NodoFeria(t, k >= 7 ? ZONA_CARRUSEL : ZONA_NORIA, ESTE[k]);
    }

    n.esteArriba = NodoFeria(t, ZONA_NORIA, DatoNodoFeria{ 'n', 6.40f, -3.90f });
    n.esteAbajo = NodoFeria(t, ZONA_CARRUSEL, DatoNodoFeria{ 'n', 7.65f, 5.25f });

    static const DatoNodoFeria PASILLO[CASILLAS_ATAJO_FERIA] =
    {
        { '-', 14.62f, -4.49f },
        { '-', 14.44f, -0.44f },
        { '+', 14.77f,  3.63f }
    };

    TramoFeria(t, ZONA_NORIA, PASILLO, CASILLAS_ATAJO_FERIA, n.pasillo);

    // --- La Gran Rueda: la X del ocho ---
    n.cruce = NodoFeria(t, ZONA_GRAN_RUEDA, DatoNodoFeria{ 'I', 0.0f, 0.0f });
    n.cruceNoroeste = NodoFeria(t, ZONA_GRAN_RUEDA, DatoNodoFeria{ 'V', -3.20f, -1.95f });
    n.cruceNoreste = NodoFeria(t, ZONA_GRAN_RUEDA, DatoNodoFeria{ 'n', 3.20f, -1.95f });

    // [0] junto al cruce, [1] junto al lazo.
    n.cruceSuroeste[0] = NodoFeria(t, ZONA_GRAN_RUEDA, DatoNodoFeria{ '+', -2.55f, 1.75f });
    n.cruceSuroeste[1] = NodoFeria(t, ZONA_GRAN_RUEDA, DatoNodoFeria{ '-', -5.10f, 3.50f });
    n.cruceSureste[0] = NodoFeria(t, ZONA_GRAN_RUEDA, DatoNodoFeria{ '+', 2.55f, 1.75f });
    n.cruceSureste[1] = NodoFeria(t, ZONA_GRAN_RUEDA, DatoNodoFeria{ '-', 5.10f, 3.50f });

    // --- LA MONTAÑA RUSA: de w1 por el noroeste hasta w5 ---
    static const DatoNodoFeria RUSA[CASILLAS_RUSA_FERIA] =
    {
        { '+',  -8.66f, -10.03f },
        { '-', -10.67f, -12.68f },
        { 'D', -13.41f, -14.61f },
        { '+', -16.61f, -15.54f },
        { 'V', -19.94f, -15.16f },
        { 'C', -22.85f, -13.55f },
        { '+', -24.66f, -10.75f },
        { 'R', -25.01f,  -7.43f },
        { '+', -23.89f,  -4.42f }
    };

    TramoFeria(t, ZONA_MONTANA_RUSA, RUSA, CASILLAS_RUSA_FERIA, n.rusa);

    // --- EL CARRUSEL: de eBI por el sur hasta wBI ---
    static const DatoNodoFeria CARRUSEL[CASILLAS_CARRUSEL_FERIA] =
    {
        { '+',  7.06f,  8.75f },
        { 'T',  6.04f, 12.12f },     // puesto de golosinas (tienda)
        { '+',  3.42f, 14.44f },
        { 'V',  0.02f, 15.27f },
        { 'C', -3.40f, 14.45f },
        { '+', -6.03f, 12.14f },
        { 'R', -7.06f,  8.77f }
    };

    TramoFeria(t, ZONA_CARRUSEL, CARRUSEL, CASILLAS_CARRUSEL_FERIA, n.carrusel);

    // --- Conexiones (opcion 0 primero) ---
    const int* w = n.oeste;
    const int* e = n.este;

    // Lazo oeste: wTI -> w1 -> ... -> w11 -> wBI, con la montaña rusa
    // (larga) antes de w2 y el callejon (atajo) despues de w4.
    t.ConectarCasillas(n.oesteArriba, w[0]);
    t.ConectarCasillas(w[0], n.rusa[0]);
    t.ConectarCasillas(w[0], w[1]);
    t.ConectarCasillas(w[1], w[2]);
    t.ConectarCasillas(w[2], w[3]);
    t.ConectarCasillas(w[2], n.callejon[0]);

    for (int k = 3; k + 1 < CASILLAS_LAZO_FERIA; k++)
    {
        t.ConectarCasillas(w[k], w[k + 1]);
    }

    t.ConectarCasillas(w[CASILLAS_LAZO_FERIA - 1], n.oesteAbajo);

    EncadenarFeria(t, n.rusa, CASILLAS_RUSA_FERIA);
    t.ConectarCasillas(n.rusa[CASILLAS_RUSA_FERIA - 1], w[4]);

    EncadenarFeria(t, n.callejon, CASILLAS_ATAJO_FERIA);
    t.ConectarCasillas(n.callejon[CASILLAS_ATAJO_FERIA - 1], w[8]);

    // Pie del lazo oeste: al cruce (largo) o subir por el lado interior.
    t.ConectarCasillas(n.oesteAbajo, n.cruceSuroeste[1]);
    t.ConectarCasillas(n.oesteAbajo, n.interior[0]);
    EncadenarFeria(t, n.interior, 2);
    t.ConectarCasillas(n.interior[1], n.oesteArriba);

    // Lazo este: eTI -> e1 -> ... -> e11 -> eBI, con el pasillo (atajo)
    // despues de e4.
    t.ConectarCasillas(n.esteArriba, e[0]);

    for (int k = 0; k + 1 < CASILLAS_LAZO_FERIA; k++)
    {
        t.ConectarCasillas(e[k], e[k + 1]);

        if (k == 2)
        {
            t.ConectarCasillas(e[2], n.pasillo[0]);
        }
    }

    t.ConectarCasillas(e[CASILLAS_LAZO_FERIA - 1], n.esteAbajo);

    EncadenarFeria(t, n.pasillo, CASILLAS_ATAJO_FERIA);
    t.ConectarCasillas(n.pasillo[CASILLAS_ATAJO_FERIA - 1], e[8]);

    // Pie del lazo este: rodear el carrusel (largo) o ir al cruce.
    t.ConectarCasillas(n.esteAbajo, n.carrusel[0]);
    t.ConectarCasillas(n.esteAbajo, n.cruceSureste[1]);
    EncadenarFeria(t, n.carrusel, CASILLAS_CARRUSEL_FERIA);
    t.ConectarCasillas(n.carrusel[CASILLAS_CARRUSEL_FERIA - 1], n.oesteAbajo);

    // La X: los brazos de abajo llegan al cruce y los de arriba salen
    // hacia el lazo que se elija.
    t.ConectarCasillas(n.cruceSuroeste[1], n.cruceSuroeste[0]);
    t.ConectarCasillas(n.cruceSuroeste[0], n.cruce);
    t.ConectarCasillas(n.cruceSureste[1], n.cruceSureste[0]);
    t.ConectarCasillas(n.cruceSureste[0], n.cruce);
    t.ConectarCasillas(n.cruce, n.cruceNoreste);
    t.ConectarCasillas(n.cruce, n.cruceNoroeste);
    t.ConectarCasillas(n.cruceNoreste, n.esteArriba);
    t.ConectarCasillas(n.cruceNoroeste, n.oesteArriba);

    // --- Textos de las bifurcaciones ---
    DefinirOpcionesFeria(w[0], "A LA MONTAÑA RUSA (LARGO)", "SEGUIR LAS CARPAS (CORTO)");
    DefinirOpcionesFeria(w[2], "SEGUIR LA CALLE (LARGO)", "CALLEJÓN DE LAS CARPAS (ATAJO)");
    DefinirOpcionesFeria(n.oesteAbajo, "AL CRUCE DE LA GRAN RUEDA", "SEGUIR EN LAS CARPAS");
    DefinirOpcionesFeria(e[2], "RODEAR LA NORIA (LARGO)", "PASILLO DE LOS PUESTOS (ATAJO)");
    DefinirOpcionesFeria(n.esteAbajo, "AL CARRUSEL (LARGO)", "AL CRUCE DE LA GRAN RUEDA");
    DefinirOpcionesFeria(n.cruce, "HACIA LA NORIA", "HACIA LAS CARPAS");

    // --- Candidatas del trofeo: 2-3 por zona, ninguna en los atajos ---
    const int candidatas[] =
    {
        w[7], w[10],
        w[0], w[4],
        e[0], e[3], e[6],
        n.cruce, n.cruceNoreste,
        n.carrusel[2], n.carrusel[5], e[8],
        n.rusa[1], n.rusa[3], n.rusa[6]
    };

    for (int indice : candidatas)
    {
        if (cantidadTrofeosFeria < MAX_TROFEOS_FERIA)
        {
            casillasTrofeoFeria[cantidadTrofeosFeria++] = indice;
        }
    }

    t.recorridoValido =
        t.ValidarRecorrido();
}


static const Tablero& TableroReferenciaFeria()
{
    static Tablero referencia;
    static bool lista = false;

    if (!lista)
    {
        ConstruirRutaFeria(referencia);
        lista = true;
    }

    return referencia;
}


//==================================================
// COLORES Y BOMBILLAS
//==================================================

static const Color TIERRA_FERIA = Color{ 112, 86, 70, 255 };
static const Color TIERRA_FERIA_BORDE = Color{ 66, 50, 46, 255 };
static const Color MADERA_FERIA = Color{ 104, 70, 52, 255 };
static const Color MADERA_FERIA_OSCURA = Color{ 70, 46, 38, 255 };
static const Color DORADO_FERIA = Color{ 236, 184, 66, 255 };
static const Color MAGENTA_FERIA = Color{ 206, 62, 176, 255 };
static const Color VIOLETA_FERIA = Color{ 92, 54, 150, 255 };
static const Color CREMA_FERIA = Color{ 236, 222, 196, 255 };
static const Color ROJO_FERIA = Color{ 212, 70, 64, 255 };
static const Color CELESTE_FERIA = Color{ 76, 176, 214, 255 };

static const Color COLORES_BOMBILLA[] =
{
    Color{ 255, 214, 90, 255 },
    Color{ 255, 92, 196, 255 },
    Color{ 96, 222, 255, 255 },
    Color{ 255, 150, 60, 255 }
};

static const int CANTIDAD_COLORES_BOMBILLA =
    (int)(sizeof(COLORES_BOMBILLA) / sizeof(COLORES_BOMBILLA[0]));


// Bombilla de feria con parpadeo en cadena (indice = posicion en la tira).
static void DibujarBombillaFeria(
    Vector3 posicion,
    int indice,
    float tiempo,
    float radio
)
{
    Color encendida =
        COLORES_BOMBILLA[indice % CANTIDAD_COLORES_BOMBILLA];

    Color apagada = MezclaFeria(encendida, Color{ 30, 20, 40, 255 }, 0.7f);

    float brillo = Parpadeo(tiempo * 3.2f - indice * 0.85f, 1.0f);

    DrawSphereEx(
        posicion,
        radio,
        3,
        6,
        brillo > 0.35f
            ? MezclaFeria(apagada, encendida, 0.55f + brillo * 0.45f)
            : apagada
    );
}


//==================================================
// GEOMETRIA DEL MAPA (sendero, landmarks y zonas reservadas)
//==================================================

static float DistanciaSegmentoFeria(
    float x, float z,
    float ax, float az,
    float bx, float bz
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

    float cx = ax + dx * k - x;
    float cz = az + dz * k - z;

    return std::sqrt(cx * cx + cz * cz);
}


// Camino de entrada (decorativo): baja del inicio al borde sur bajo el
// portal. Cuenta como sendero para el terreno y la decoracion.
static const float ENTRADA_SUR_Z = 26.0f;


// Distancia al sendero mas cercano (segmentos entre casillas conectadas
// y camino de entrada).
static float DistanciaSenderoFeria(
    float x,
    float z
)
{
    const Tablero& t = TableroReferenciaFeria();
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
                DistanciaSegmentoFeria(
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

    const Vector3& inicio = t.casillas[nodosFeria.inicio].posicion;

    float entrada =
        DistanciaSegmentoFeria(
            x, z,
            inicio.x, inicio.z,
            inicio.x, ENTRADA_SUR_Z
        );

    return entrada < mejor ? entrada : mejor;
}


// Lugares de los landmarks: todos a >= 2.5 del sendero y fuera de la franja
// frontal de las casillas (ver herramienta de layout en las notas).
static const float CARPA_MAYOR_X = 0.0f;
static const float CARPA_MAYOR_Z = -13.0f;

static const float NORIA_X = 14.4f;
static const float NORIA_Z = -13.8f;
static const float NORIA_ALTO_EJE = 6.0f;
static const float NORIA_RADIO = 4.6f;
static const int CABINAS_NORIA = 12;

static const float CARRUSEL_X = 0.0f;
static const float CARRUSEL_Z = 10.4f;
static const float CARRUSEL_RADIO = 2.45f;

static const float PORTAL_X = -16.83f;
// El arco queda a mas de 5 delante del inicio: no tapa la casilla.
static const float PORTAL_Z = 12.4f;

static const float TIRO_X = 25.8f;
static const float TIRO_Z = -1.4f;

static const float TAQUILLA_RUSA_X = -16.4f;
static const float TAQUILLA_RUSA_Z = -11.0f;

// Carpas del lazo oeste (x, z, radio, alto): entre las franjas frontales
// de la calle y del callejon.
static const float CARPAS_FERIA[5][4] =
{
    { -17.6f, -2.7f, 0.90f, 2.4f },
    { -18.1f,  0.6f, 1.15f, 2.9f },   // carpa del mago, junto a la tienda
    { -11.5f, -3.7f, 0.85f, 2.2f },
    { -11.6f,  0.1f, 0.80f, 2.1f },
    { -11.8f,  3.9f, 0.75f, 1.9f }
};

// Puestos de comida (x, z, giro en grados: 0 = mostrador hacia +z).
static const float PUESTOS_FERIA[9][3] =
{
    {  11.4f, -2.4f,  90.0f },        // dentro del lazo de la noria
    {  11.3f,  1.9f,  90.0f },
    {  17.8f, -2.4f, -90.0f },
    {  17.5f,  1.2f, -90.0f },
    {  12.6f, 13.0f,   0.0f },        // patio de comidas (sureste)
    {  17.0f, 12.8f,   0.0f },
    {  20.6f, 10.8f,   0.0f },
    {  -5.2f, -12.2f,  0.0f },        // al pie de la carpa mayor
    {   5.2f, -12.2f,  0.0f }
};

static const int CANTIDAD_CARPAS_FERIA = 5;
static const int CANTIDAD_PUESTOS_FERIA = 9;

struct ZonaReservadaFeria
{
    float x;
    float z;
    float radio;
};

static const ZonaReservadaFeria ZONAS_RESERVADAS_FERIA[] =
{
    { RUEDA_X, RUEDA_Z - 0.3f, 3.0f },
    { CARPA_MAYOR_X, CARPA_MAYOR_Z, 4.3f },
    { FRASCO_X, FRASCO_Z, 1.5f },
    { CARRUSEL_X, CARRUSEL_Z, 3.0f },
    { NORIA_X, NORIA_Z, 3.2f },
    { NORIA_X - 3.4f, NORIA_Z, 1.8f },
    { NORIA_X + 3.4f, NORIA_Z, 1.8f },
    { PORTAL_X, PORTAL_Z, 2.6f },
    { TIRO_X, TIRO_Z, 2.4f },
    { TAQUILLA_RUSA_X, TAQUILLA_RUSA_Z, 1.3f },
    { -17.6f, -2.7f, 1.3f },
    { -18.1f,  0.6f, 1.6f },
    { -18.1f,  2.1f, 0.5f },          // bola de cristal del mago
    { -11.5f, -3.7f, 1.2f },
    { -11.6f,  0.1f, 1.2f },
    { -11.8f,  3.9f, 1.1f },
    {  11.4f, -2.4f, 1.5f },
    {  11.3f,  1.9f, 1.5f },
    {  17.8f, -2.4f, 1.5f },
    {  17.5f,  1.2f, 1.5f },
    {  12.6f, 13.0f, 1.6f },
    {  17.0f, 12.8f, 1.6f },
    {  20.6f, 10.8f, 1.6f },
    {  -5.2f, -12.2f, 1.6f },
    {   5.2f, -12.2f, 1.6f },
    {   4.3f,  8.6f, 1.0f },          // carrito de globos
    { -21.4f,  9.8f, 1.2f },          // taquilla de la entrada
    {   7.8f, -17.6f, 2.1f },         // casas rodantes
    {   3.8f, -20.2f, 2.1f },
    {  -9.0f, -18.6f, 3.3f }          // estacion de la montaña rusa
};


static bool DespejadoFeria(float x, float z, float radio)
{
    for (const ZonaReservadaFeria& r : ZONAS_RESERVADAS_FERIA)
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


// Algo alto delante de una casilla (hacia la camara) la taparia. "radio"
// agranda la huella de la pieza.
static bool OcultaCasillaFeria(float x, float z, float radio)
{
    const Tablero& t = TableroReferenciaFeria();

    for (int i = 0; i < t.cantidadCasillas; i++)
    {
        const Vector3& p = t.casillas[i].posicion;

        if (
            z + radio > p.z + 0.4f && z - radio < p.z + 3.4f &&
            std::fabs(x - p.x) < 2.0f + radio
        )
        {
            return true;
        }
    }

    return false;
}


//==================================================
// LUZ HORNEADA Y PIEZAS EMISIVAS
//==================================================
//
// Las mallas no tienen luces: cada triangulo se oscurece segun su normal
// (luz de luna, como el terreno de Arboleda). Lo que brilla (bombillas,
// ventanas, cielo) se marca con alfa 254 y no se oscurece.

static const unsigned char ALFA_EMISIVO_FERIA = 254;


static Color EmisivoFeria(Color c)
{
    c.a = ALFA_EMISIVO_FERIA;
    return c;
}


static float FactorLuzFeria(float nx, float ny, float nz)
{
    float luz = -0.45f * nx + 0.80f * ny + 0.40f * nz;

    if (luz < 0.0f)
    {
        luz = 0.0f;
    }

    return 0.66f + 0.46f * luz;
}


// Aplica la luz a los vertices emitidos desde "desde" (multiplo de 3).
static void IluminarMallaFeria(
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
            // Las piezas de dos caras: se toma la cara que mira arriba.
            if (ny < 0.0f)
            {
                nx = -nx;
                ny = -ny;
                nz = -nz;
            }

            factor = FactorLuzFeria(nx / largo, ny / largo, nz / largo);
        }

        for (int k = 0; k < 3; k++)
        {
            unsigned char* c = &m.colores[(v + k) * 4];

            if (c[3] == ALFA_EMISIVO_FERIA)
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
// CHARCOS DE LUZ (tinte calido horneado en el suelo)
//==================================================
//
// Faroles, puestos y atracciones tinen el pasto y la tierra que tienen
// cerca. Es solo color horneado: no hay luces dinamicas.

struct LuzSueloFeria
{
    float x;
    float z;
    float radio;
    Color color;
    float intensidad;
};

static const int MAX_LUCES_SUELO_FERIA = 160;

static LuzSueloFeria lucesSueloFeria[MAX_LUCES_SUELO_FERIA];
static int cantidadLucesSueloFeria = 0;


static void AgregarLuzSueloFeria(
    float x, float z,
    float radio,
    Color color,
    float intensidad
)
{
    if (cantidadLucesSueloFeria >= MAX_LUCES_SUELO_FERIA)
    {
        return;
    }

    lucesSueloFeria[cantidadLucesSueloFeria++] =
        LuzSueloFeria{ x, z, radio, color, intensidad };
}


// Suma el resplandor de las luces cercanas a un color ya iluminado.
static Color TenirConLucesFeria(Color c, float x, float z)
{
    float r = c.r;
    float g = c.g;
    float b = c.b;

    for (int i = 0; i < cantidadLucesSueloFeria; i++)
    {
        const LuzSueloFeria& l = lucesSueloFeria[i];

        float dx = x - l.x;
        float dz = z - l.z;
        float d2 = dx * dx + dz * dz;

        if (d2 >= l.radio * l.radio)
        {
            continue;
        }

        float k = 1.0f - std::sqrt(d2) / l.radio;
        k = k * k * l.intensidad;

        r += l.color.r * k;
        g += l.color.g * k;
        b += l.color.b * k;
    }

    return Color
    {
        (unsigned char)(r > 255.0f ? 255.0f : r),
        (unsigned char)(g > 255.0f ? 255.0f : g),
        (unsigned char)(b > 255.0f ? 255.0f : b),
        c.a
    };
}


//==================================================
// TERRENO (lomas de pasto horneadas una vez)
//==================================================
//
// Lomas suaves de pasto nocturno, planas junto al sendero y bajo las
// atracciones, que suben hacia un cerco de colinas en el borde. Al fondo,
// el telon del horizonte (ver HORIZONTE).

static const float TELON_RADIO = 47.0f;

static const float TERRENO_X0 = -50.0f;
static const float TERRENO_Z0 = -48.5f;
static const float TERRENO_PASO = 0.8f;
static const int TERRENO_NX = 122;
static const int TERRENO_NZ = 122;

static float alturasTerrenoFeria[TERRENO_NX][TERRENO_NZ];
static bool alturasFeriaListas = false;

static MallaGpu mallaTerrenoFeria;


static float AlturaTerrenoFeria(
    float x,
    float z
)
{
    // Lomas suaves.
    float h =
        0.42f +
        0.50f * std::sin(x * 0.19f + 1.3f) * std::sin(z * 0.16f - 0.4f) +
        0.32f * std::sin(x * 0.083f - z * 0.121f + 2.0f) +
        0.16f * std::sin((x + z) * 0.29f + 0.7f);

    // Cerco de colinas hacia el borde.
    float dx = x - CENTRO_CASILLAS_X;
    float dz = z - CENTRO_CASILLAS_Z;
    float r = std::sqrt(dx * dx + dz * dz);
    float ang = std::atan2(dz, dx);

    h += 3.6f * Suave01((r - 29.0f) / 15.0f) *
        (0.75f + 0.25f * std::sin(ang * 4.0f + 0.5f) + 0.18f * std::sin(ang * 9.0f + 2.0f));

    // Plano junto al sendero (el camino queda a y = 0).
    float dp = DistanciaSenderoFeria(x, z);
    h *= Suave01((dp - 2.0f) / 1.8f);

    // Plano bajo las atracciones y los puestos.
    for (const ZonaReservadaFeria& zona : ZONAS_RESERVADAS_FERIA)
    {
        float ex = x - zona.x;
        float ez = z - zona.z;
        float d = std::sqrt(ex * ex + ez * ez);

        h *= Suave01((d - zona.radio) / 2.2f);
    }

    return h;
}


static void CalcularAlturasTerrenoFeria()
{
    for (int ix = 0; ix < TERRENO_NX; ix++)
    {
        for (int iz = 0; iz < TERRENO_NZ; iz++)
        {
            alturasTerrenoFeria[ix][iz] =
                AlturaTerrenoFeria(
                    TERRENO_X0 + ix * TERRENO_PASO,
                    TERRENO_Z0 + iz * TERRENO_PASO
                );
        }
    }

    alturasFeriaListas = true;
}


// Altura del terreno dibujado (interpolacion sobre la rejilla): mucho mas
// barata que AlturaTerrenoFeria para colocar props y particulas.
static float AlturaSueloFeria(float x, float z)
{
    if (!alturasFeriaListas)
    {
        CalcularAlturasTerrenoFeria();
    }

    float fx = (x - TERRENO_X0) / TERRENO_PASO;
    float fz = (z - TERRENO_Z0) / TERRENO_PASO;

    int ix = (int)std::floor(fx);
    int iz = (int)std::floor(fz);

    if (ix < 0 || iz < 0 || ix >= TERRENO_NX - 1 || iz >= TERRENO_NZ - 1)
    {
        return 0.0f;
    }

    float tx = fx - ix;
    float tz = fz - iz;

    float a = alturasTerrenoFeria[ix][iz] + (alturasTerrenoFeria[ix + 1][iz] - alturasTerrenoFeria[ix][iz]) * tx;
    float b = alturasTerrenoFeria[ix][iz + 1] + (alturasTerrenoFeria[ix + 1][iz + 1] - alturasTerrenoFeria[ix][iz + 1]) * tx;

    return a + (b - a) * tz;
}


// Plazas de tierra pisada bajo las atracciones (x, z, radio).
static const float PLAZAS_TIERRA_FERIA[][3] =
{
    { RUEDA_X, RUEDA_Z - 0.3f, 3.4f },
    { CARPA_MAYOR_X, CARPA_MAYOR_Z, 5.2f },
    { FRASCO_X, FRASCO_Z, 2.0f },
    { CARRUSEL_X, CARRUSEL_Z, 3.6f },
    { NORIA_X, NORIA_Z, 4.6f },
    { PORTAL_X, PORTAL_Z, 2.8f },
    { TIRO_X, TIRO_Z, 2.8f },
    { 16.6f, 12.4f, 5.4f },
    { -14.7f, 0.0f, 5.2f }
};


// Color de un triangulo del terreno (antes de la luz).
static Color ColorTerrenoFeria(
    float cx, float cz,
    float h, float ny,
    int ix, int iz
)
{
    float n = HashFeria(ix, iz);

    const Color pasto = { 30, 52, 46, 255 };
    const Color pastoClaro = { 40, 66, 52, 255 };
    const Color pastoOscuro = { 22, 40, 40, 255 };
    const Color loma = { 26, 40, 54, 255 };
    const Color tierra = { 82, 66, 56, 255 };

    float manchas = 0.5f + 0.5f * std::sin(cx * 0.23f + cz * 0.17f) * std::sin(cz * 0.31f - cx * 0.11f);

    Color c = MezclaFeria(pastoOscuro, pastoClaro, n * 0.45f + 0.55f * manchas);
    c = MezclaFeria(c, pasto, 0.35f);

    // Colinas del borde: pasto mas azulado por la luna.
    if (h > 1.2f)
    {
        c = MezclaFeria(c, loma, Suave01((h - 1.2f) / 2.6f) * 0.8f);
    }

    // Pasto pisado alrededor del sendero.
    float dp = DistanciaSenderoFeria(cx, cz);

    if (dp < 3.6f)
    {
        c = MezclaFeria(c, Color{ 64, 60, 50, 255 }, 0.5f * (1.0f - Suave01((dp - 1.7f) / 1.9f)));
    }

    // Tierra bajo las atracciones.
    for (const auto& plaza : PLAZAS_TIERRA_FERIA)
    {
        float dx = cx - plaza[0];
        float dz = cz - plaza[1];
        float d = std::sqrt(dx * dx + dz * dz);

        if (d < plaza[2])
        {
            float k = 1.0f - Suave01((d - plaza[2] * 0.6f) / (plaza[2] * 0.4f));
            c = MezclaFeria(c, OscurecerFeria(tierra, 0.9f + 0.2f * n), k * 0.85f);
        }
    }

    // Pendientes: pasto mas oscuro.
    if (ny < 0.9f)
    {
        c = MezclaFeria(c, Color{ 20, 32, 40, 255 }, Suave01((0.9f - ny) / 0.2f) * 0.6f);
    }

    return c;
}


static void ConstruirMallaTerrenoFeria()
{
    if (!alturasFeriaListas)
    {
        CalcularAlturasTerrenoFeria();
    }

    static ConstructorMalla m;
    m.Limpiar();

    for (int ix = 0; ix < TERRENO_NX - 1; ix++)
    {
        for (int iz = 0; iz < TERRENO_NZ - 1; iz++)
        {
            float x0 = TERRENO_X0 + ix * TERRENO_PASO;
            float z0 = TERRENO_Z0 + iz * TERRENO_PASO;
            float x1 = x0 + TERRENO_PASO;
            float z1 = z0 + TERRENO_PASO;

            // Fuera del telon no se ve.
            float ex = (x0 + x1) * 0.5f - CENTRO_CASILLAS_X;
            float ez = (z0 + z1) * 0.5f - CENTRO_CASILLAS_Z;

            if (ex * ex + ez * ez > (TELON_RADIO + 1.5f) * (TELON_RADIO + 1.5f))
            {
                continue;
            }

            Vector3 p00 = { x0, alturasTerrenoFeria[ix][iz], z0 };
            Vector3 p01 = { x0, alturasTerrenoFeria[ix][iz + 1], z1 };
            Vector3 p10 = { x1, alturasTerrenoFeria[ix + 1][iz], z0 };
            Vector3 p11 = { x1, alturasTerrenoFeria[ix + 1][iz + 1], z1 };

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

                Color color = ColorTerrenoFeria(cx, cz, h, normal.y, ix + k, iz + k);
                color = OscurecerFeria(color, FactorLuzFeria(normal.x, normal.y, normal.z));
                color = TenirConLucesFeria(color, cx, cz);

                m.Triangulo(v[0], v[1], v[2], color);
            }
        }
    }

    ActualizarMallaGpu(mallaTerrenoFeria, m);
}


static void DibujarTerrenoFeria()
{
    if (!mallaTerrenoFeria.cargada)
    {
        ConstruirMallaTerrenoFeria();
    }

    rlDisableBackfaceCulling();
    DibujarMallaGpu(mallaTerrenoFeria);
    rlEnableBackfaceCulling();
}


//==================================================
// HORIZONTE (telon horneado una vez)
//==================================================
//
// Un anillo alrededor de la feria: cielo nocturno en degrade con estrellas,
// la luna saliendo detras de dos filas de colinas y una ciudad con ventanas
// encendidas al norte. Todo es emisivo (no recibe la luz horneada). Cubre
// el borde del terreno desde la camara de partida y desde la vista previa.

static MallaGpu mallaTelonFeria;

// Luna: angulo sobre el anillo (norte = -PI/2) y altura.
static const float LUNA_ANGULO = -2.05f;
static const float LUNA_ALTO = 7.5f;
static const float LUNA_RADIO = 3.2f;


static Vector3 PuntoTelonFeria(float angulo, float radio, float y)
{
    return Vector3
    {
        CENTRO_CASILLAS_X + std::cos(angulo) * radio,
        y,
        CENTRO_CASILLAS_Z + std::sin(angulo) * radio
    };
}


static Color CieloFeria(float y)
{
    const Color horizonte = { 92, 58, 112, 255 };
    const Color medio = { 40, 30, 84, 255 };
    const Color alto = { 12, 10, 36, 255 };

    if (y < 7.0f)
    {
        return MezclaFeria(horizonte, medio, Suave01(y / 7.0f));
    }

    return MezclaFeria(medio, alto, Suave01((y - 7.0f) / 16.0f));
}


// Cuadro vertical sobre el anillo entre dos angulos (dos caras).
static void CuadroTelonFeria(
    ConstructorMalla& m,
    float a0, float a1,
    float radio,
    float y0, float y1,
    Color abajo, Color arriba
)
{
    Vector3 p00 = PuntoTelonFeria(a0, radio, y0);
    Vector3 p10 = PuntoTelonFeria(a1, radio, y0);
    Vector3 p01 = PuntoTelonFeria(a0, radio, y1);
    Vector3 p11 = PuntoTelonFeria(a1, radio, y1);

    // Degrade vertical: un color por fila de vertices.
    m.Vertice(p00.x, p00.y, p00.z, abajo);
    m.Vertice(p10.x, p10.y, p10.z, abajo);
    m.Vertice(p11.x, p11.y, p11.z, arriba);

    m.Vertice(p00.x, p00.y, p00.z, abajo);
    m.Vertice(p11.x, p11.y, p11.z, arriba);
    m.Vertice(p01.x, p01.y, p01.z, arriba);

    m.Vertice(p00.x, p00.y, p00.z, abajo);
    m.Vertice(p11.x, p11.y, p11.z, arriba);
    m.Vertice(p10.x, p10.y, p10.z, abajo);

    m.Vertice(p00.x, p00.y, p00.z, abajo);
    m.Vertice(p01.x, p01.y, p01.z, arriba);
    m.Vertice(p11.x, p11.y, p11.z, arriba);
}


// Disco vertical mirando al centro (luna, halo).
static void DiscoTelonFeria(
    ConstructorMalla& m,
    float angulo, float radioAnillo,
    float y, float radio,
    int lados,
    Color color
)
{
    Vector3 centro = PuntoTelonFeria(angulo, radioAnillo, y);

    // Ejes del plano: tangente horizontal y vertical.
    float tx = -std::sin(angulo);
    float tz = std::cos(angulo);

    for (int i = 0; i < lados; i++)
    {
        float b0 = 6.2831853f * i / lados;
        float b1 = 6.2831853f * (i + 1) / lados;

        Vector3 p0 = { centro.x + tx * std::cos(b0) * radio, centro.y + std::sin(b0) * radio, centro.z + tz * std::cos(b0) * radio };
        Vector3 p1 = { centro.x + tx * std::cos(b1) * radio, centro.y + std::sin(b1) * radio, centro.z + tz * std::cos(b1) * radio };

        m.TrianguloDobleCara(centro, p0, p1, color);
    }
}


// Perfil de las dos filas de colinas pintadas.
static float ColinaLejanaFeria(float a)
{
    return 3.0f + 1.6f * std::sin(a * 3.0f + 1.0f) + 1.1f * std::sin(a * 7.0f + 2.0f) + 0.5f * std::sin(a * 17.0f);
}


static float ColinaCercanaFeria(float a)
{
    return 1.6f + 1.2f * std::sin(a * 5.0f + 0.3f) + 0.7f * std::sin(a * 11.0f + 1.0f) + 0.3f * std::sin(a * 23.0f + 0.4f);
}


static void ConstruirMallaTelonFeria()
{
    static ConstructorMalla m;
    m.Limpiar();

    const int SEGMENTOS = 120;
    const float BANDAS[] = { -3.0f, 1.5f, 4.0f, 7.0f, 11.0f, 16.0f, 23.0f, 32.0f };
    const int CANTIDAD_BANDAS = (int)(sizeof(BANDAS) / sizeof(BANDAS[0]));

    // --- Cielo ---
    for (int i = 0; i < SEGMENTOS; i++)
    {
        float a0 = 6.2831853f * i / SEGMENTOS;
        float a1 = 6.2831853f * (i + 1) / SEGMENTOS;

        for (int b = 0; b + 1 < CANTIDAD_BANDAS; b++)
        {
            CuadroTelonFeria(
                m, a0, a1, TELON_RADIO,
                BANDAS[b], BANDAS[b + 1],
                CieloFeria(BANDAS[b]), CieloFeria(BANDAS[b + 1])
            );
        }
    }

    // --- Estrellas (rombos chicos, mas densas arriba) ---
    for (int i = 0; i < 420; i++)
    {
        float a = 6.2831853f * HashFeria(i, 401);
        float y = 3.5f + 26.0f * std::sqrt(HashFeria(i, 402));
        float lado = 0.10f + 0.16f * HashFeria(i, 403);

        // Ni sobre la luna ni detras de las colinas.
        float da = a - (LUNA_ANGULO + 6.2831853f);

        while (da > 3.14159265f) da -= 6.2831853f;
        while (da < -3.14159265f) da += 6.2831853f;

        float ladoLuna = da * TELON_RADIO;
        float altoLuna = y - LUNA_ALTO;

        if (ladoLuna * ladoLuna + altoLuna * altoLuna < 6.0f * 6.0f)
        {
            continue;
        }

        if (y < ColinaLejanaFeria(a) + 0.6f)
        {
            continue;
        }

        Vector3 c = PuntoTelonFeria(a, TELON_RADIO - 0.15f, y);
        float tx = -std::sin(a);
        float tz = std::cos(a);

        Color brillo =
            HashFeria(i, 404) < 0.2f
            ? Color{ 255, 230, 180, 255 }
            : Color{ 220, 230, 255, 255 };

        brillo = MezclaFeria(CieloFeria(y), brillo, 0.55f + 0.45f * HashFeria(i, 405));

        Vector3 izq = { c.x - tx * lado, c.y, c.z - tz * lado };
        Vector3 der = { c.x + tx * lado, c.y, c.z + tz * lado };
        Vector3 arriba = { c.x, c.y + lado, c.z };
        Vector3 abajo = { c.x, c.y - lado, c.z };

        m.TrianguloDobleCara(izq, arriba, der, brillo);
        m.TrianguloDobleCara(izq, der, abajo, brillo);
    }

    // --- Luna con halo ---
    const Color cieloLuna = CieloFeria(LUNA_ALTO);

    DiscoTelonFeria(m, LUNA_ANGULO, TELON_RADIO - 0.25f, LUNA_ALTO, LUNA_RADIO * 2.1f, 36, MezclaFeria(cieloLuna, Color{ 150, 140, 170, 255 }, 0.25f));
    DiscoTelonFeria(m, LUNA_ANGULO, TELON_RADIO - 0.30f, LUNA_ALTO, LUNA_RADIO * 1.5f, 36, MezclaFeria(cieloLuna, Color{ 190, 180, 200, 255 }, 0.4f));
    DiscoTelonFeria(m, LUNA_ANGULO, TELON_RADIO - 0.35f, LUNA_ALTO, LUNA_RADIO, 36, Color{ 246, 240, 214, 255 });

    // Mares de la luna.
    const float mares[3][3] = { { -0.9f, 0.6f, 0.9f }, { 0.8f, -0.4f, 0.7f }, { 0.2f, 1.2f, 0.5f } };

    for (const auto& mar : mares)
    {
        float a = LUNA_ANGULO + mar[0] / TELON_RADIO;

        DiscoTelonFeria(
            m, a, TELON_RADIO - 0.4f, LUNA_ALTO + mar[1], mar[2], 16,
            Color{ 222, 214, 192, 255 }
        );
    }

    // --- Colinas pintadas (lejanas y cercanas) ---
    for (int i = 0; i < SEGMENTOS * 2; i++)
    {
        float a0 = 6.2831853f * i / (SEGMENTOS * 2);
        float a1 = 6.2831853f * (i + 1) / (SEGMENTOS * 2);

        float h0 = ColinaLejanaFeria(a0);
        float h1 = ColinaLejanaFeria(a1);

        Vector3 b0 = PuntoTelonFeria(a0, TELON_RADIO - 0.5f, -3.0f);
        Vector3 b1 = PuntoTelonFeria(a1, TELON_RADIO - 0.5f, -3.0f);
        Vector3 c0 = PuntoTelonFeria(a0, TELON_RADIO - 0.5f, h0);
        Vector3 c1 = PuntoTelonFeria(a1, TELON_RADIO - 0.5f, h1);

        Color lejana = Color{ 40, 36, 72, 255 };

        m.TrianguloDobleCara(b0, b1, c1, lejana);
        m.TrianguloDobleCara(b0, c1, c0, lejana);

        h0 = ColinaCercanaFeria(a0 + 0.7f);
        h1 = ColinaCercanaFeria(a1 + 0.7f);

        b0 = PuntoTelonFeria(a0, TELON_RADIO - 1.0f, -3.0f);
        b1 = PuntoTelonFeria(a1, TELON_RADIO - 1.0f, -3.0f);
        c0 = PuntoTelonFeria(a0, TELON_RADIO - 1.0f, h0);
        c1 = PuntoTelonFeria(a1, TELON_RADIO - 1.0f, h1);

        Color cercana = Color{ 22, 26, 44, 255 };

        m.TrianguloDobleCara(b0, b1, c1, cercana);
        m.TrianguloDobleCara(b0, c1, c0, cercana);
    }

    // --- Ciudad al norte: edificios con ventanas encendidas ---
    float a = -2.55f;
    int edificio = 0;

    while (a < -0.75f)
    {
        float ancho = 1.4f + 2.2f * HashFeria(edificio, 501);
        float alto = 3.2f + 6.5f * HashFeria(edificio, 502) * HashFeria(edificio, 503) + 1.5f * HashFeria(edificio, 504);

        // Mas altos en el centro de la ciudad.
        float centroCiudad = 1.0f - std::fabs(a + 1.65f) / 0.9f;
        alto *= 0.6f + 0.7f * Limitar01(centroCiudad);

        float da = ancho / TELON_RADIO;
        float radio = TELON_RADIO - 1.4f - 0.3f * HashFeria(edificio, 505);
        Color muro = MezclaFeria(Color{ 14, 14, 30, 255 }, Color{ 26, 22, 44, 255 }, HashFeria(edificio, 506));

        CuadroTelonFeria(m, a, a + da, radio, -3.0f, alto, muro, muro);

        // Ventanas.
        int columnas = (int)(ancho / 0.5f);
        int filas = (int)((alto - 1.2f) / 0.6f);

        for (int f = 0; f < filas; f++)
        {
            for (int c = 0; c < columnas; c++)
            {
                float azar = HashFeria(edificio * 97 + f, c * 13 + 7);

                if (azar > 0.42f)
                {
                    continue;
                }

                float u = (c + 0.5f) / columnas;
                float av = a + da * u;
                float y = 1.4f + f * 0.6f;

                Color luz =
                    azar < 0.12f
                    ? Color{ 170, 210, 255, 255 }
                    : (azar < 0.3f ? Color{ 255, 206, 120, 255 } : Color{ 250, 170, 90, 255 });

                CuadroTelonFeria(m, av - 0.11f / TELON_RADIO, av + 0.11f / TELON_RADIO, radio - 0.05f, y, y + 0.28f, luz, luz);
            }
        }

        a += da + (0.15f + 0.5f * HashFeria(edificio, 507)) / TELON_RADIO;
        edificio++;
    }

    // Antena con luz roja (la luz que titila es inmediata).
    Vector3 pieAntena = PuntoTelonFeria(-1.55f, TELON_RADIO - 1.8f, 0.0f);

    m.Cilindro(pieAntena, Vector3{ pieAntena.x, 14.0f, pieAntena.z }, 0.12f, 0.05f, 4, Color{ 30, 26, 48, 255 });

    ActualizarMallaGpu(mallaTelonFeria, m);
}


static Vector3 PuntaAntenaFeria()
{
    Vector3 p = PuntoTelonFeria(-1.55f, TELON_RADIO - 1.8f, 0.0f);
    p.y = 14.1f;
    return p;
}


static void DibujarTelonFeria()
{
    if (!mallaTelonFeria.cargada)
    {
        ConstruirMallaTelonFeria();
    }

    DibujarMallaGpu(mallaTelonFeria);
}


//==================================================
// SENDEROS (malla horneada por calidad)
//==================================================
//
// Como en Arboleda: cinta por conexion con borde, dos carriles y huella;
// disco en cada nodo y plaza en bifurcaciones y uniones. Material por zona:
// ladrillo en la entrada, aserrin en las carpas, tierra en la noria,
// baldosas en la Gran Rueda, adoquines en el carrusel y tablones en la
// montaña rusa. Confeti pisado en calidad alta.

static const float SENDERO_SEMIANCHO = 1.5f;
static const float SENDERO_BORDE = 0.30f;
static const float SENDERO_PLAZA = 1.85f;

static const float LIFT_BORDE = 0.030f;
static const float LIFT_BORDE_NODO = 0.050f;
static const float LIFT_TIERRA = 0.070f;
static const float LIFT_TIERRA_NODO = 0.090f;
static const float LIFT_DETALLE = 0.108f;

struct MaterialSenderoFeria
{
    Color tierra;
    Color borde;
    float contraste;
    float largoLosa;
};


static MaterialSenderoFeria MaterialSenderoZonaFeria(int zona)
{
    switch (zona)
    {
        case ZONA_ENTRADA:
            return { Color{ 132, 92, 78, 255 }, Color{ 76, 52, 50, 255 }, 0.12f, 0.7f };
        case ZONA_CARPAS:
            return { Color{ 142, 114, 84, 255 }, Color{ 82, 64, 52, 255 }, 0.06f, 1.3f };
        case ZONA_NORIA:
            return { Color{ 118, 92, 72, 255 }, Color{ 68, 52, 46, 255 }, 0.07f, 1.2f };
        case ZONA_GRAN_RUEDA:
            return { Color{ 136, 106, 124, 255 }, Color{ 72, 52, 74, 255 }, 0.20f, 0.75f };
        case ZONA_CARRUSEL:
            return { Color{ 124, 108, 100, 255 }, Color{ 66, 56, 60, 255 }, 0.12f, 0.8f };
        case ZONA_MONTANA_RUSA:
            return { Color{ 124, 88, 62, 255 }, Color{ 64, 46, 38, 255 }, 0.16f, 0.5f };
        default:
            break;
    }

    return { TIERRA_FERIA, TIERRA_FERIA_BORDE, 0.08f, 1.2f };
}

static const Color COLOR_BORDE_SUELO_FERIA = Color{ 46, 52, 48, 255 };

static MallaGpu mallaSenderosFeria;
static int nivelSenderosFeria = -1;


struct SeccionSenderoFeria
{
    float x, y, z;
    float nx, nz;
};


static bool ConexionInversaFeria(const Tablero& t, int a, int b)
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


static void CuadroSenderoFeria(
    ConstructorMalla& m,
    const SeccionSenderoFeria& s0, const SeccionSenderoFeria& s1,
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


static void DiscoSenderoFeria(
    ConstructorMalla& m,
    float x, float y, float z,
    float radio, float lift,
    int lados,
    Color color
)
{
    for (int i = 0; i < lados; i++)
    {
        float a0 = 6.2831853f * i / lados;
        float a1 = 6.2831853f * (i + 1) / lados;

        m.TrianguloDobleCara(
            Vector3{ x, y + lift, z },
            Vector3{ x + std::cos(a0) * radio, y + lift, z + std::sin(a0) * radio },
            Vector3{ x + std::cos(a1) * radio, y + lift, z + std::sin(a1) * radio },
            color
        );
    }
}


static void AnilloSenderoFeria(
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


// Una cinta de sendero de "a" a "b" (zonas para el material; "semilla"
// varia las losas y la curva).
static void TramoSenderoFeria(
    ConstructorMalla& m,
    Vector3 a, int zonaA,
    Vector3 b, int zonaB,
    int semillaA, int semillaB,
    bool curvo,
    NivelCalidadGrafica nivel
)
{
    const bool detalle = nivel != CALIDAD_BAJA;
    const float paso = 0.42f;
    const float limiteBorde = SENDERO_SEMIANCHO + SENDERO_BORDE;

    float dx = b.x - a.x;
    float dz = b.z - a.z;
    float largo = std::sqrt(dx * dx + dz * dz);

    if (largo < 0.01f)
    {
        return;
    }

    int n = (int)std::ceil(largo / paso);

    if (n > 250)
    {
        n = 250;
    }

    float px = -dz / largo;
    float pz = dx / largo;

    float amp =
        curvo
        ? (HashFeria(semillaA * 31 + 7, semillaB * 17 + 3) - 0.5f) * 0.45f
        : 0.0f;

    static SeccionSenderoFeria sec[256];

    for (int s = 0; s <= n; s++)
    {
        float u = (float)s / n;
        float curva = std::sin(3.14159265f * u) * amp;

        sec[s].x = a.x + dx * u + px * curva;
        sec[s].z = a.z + dz * u + pz * curva;
        sec[s].y = 0.0f;
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
    }

    MaterialSenderoFeria ma = MaterialSenderoZonaFeria(zonaA);
    MaterialSenderoFeria mb = MaterialSenderoZonaFeria(zonaB);

    bool tablones =
        zonaA == ZONA_MONTANA_RUSA ||
        zonaB == ZONA_MONTANA_RUSA;

    bool baldosas =
        zonaA == ZONA_GRAN_RUEDA &&
        zonaB == ZONA_GRAN_RUEDA;

    for (int s = 0; s < n; s++)
    {
        float u = ((float)s + 0.5f) / n;
        float tamLosa = ma.largoLosa + (mb.largoLosa - ma.largoLosa) * u;
        int bloque = (int)std::floor(u * largo / tamLosa);
        float contraste = ma.contraste + (mb.contraste - ma.contraste) * u;

        Color tierra = MezclaFeria(ma.tierra, mb.tierra, u);
        Color borde = MezclaFeria(ma.borde, mb.borde, u);
        Color bordeExt = MezclaFeria(borde, COLOR_BORDE_SUELO_FERIA, 0.45f);

        for (int lado = -1; lado <= 1; lado += 2)
        {
            CuadroSenderoFeria(
                m, sec[s], sec[s + 1],
                lado * SENDERO_SEMIANCHO * 0.9f, lado * limiteBorde,
                LIFT_BORDE, bordeExt
            );
        }

        for (int lado = 0; lado < 2; lado++)
        {
            float v = HashFeria(semillaA * 131 + bloque * 7 + 1, semillaB * 17 + (tablones ? 0 : lado));

            // Baldosas: damero de dos tonos.
            if (baldosas)
            {
                v = ((bloque + lado) % 2 == 0) ? 0.95f : 0.1f;
            }

            Color c = OscurecerFeria(tierra, 1.0f + (v - 0.5f) * 2.0f * contraste);

            CuadroSenderoFeria(
                m, sec[s], sec[s + 1],
                lado == 0 ? -SENDERO_SEMIANCHO : 0.0f,
                lado == 0 ? 0.0f : SENDERO_SEMIANCHO,
                LIFT_TIERRA, c
            );
        }

        if (!detalle)
        {
            continue;
        }

        if (tablones)
        {
            // Junta oscura entre tablones.
            if (s % 2 == 0)
            {
                SeccionSenderoFeria j1 = sec[s];
                j1.x += (sec[s + 1].x - sec[s].x) * 0.16f;
                j1.z += (sec[s + 1].z - sec[s].z) * 0.16f;

                CuadroSenderoFeria(
                    m, sec[s], j1, -SENDERO_SEMIANCHO * 0.96f, SENDERO_SEMIANCHO * 0.96f,
                    LIFT_DETALLE, OscurecerFeria(tierra, 0.62f)
                );
            }
        }
        else if (!baldosas)
        {
            // Huella central mas clara.
            CuadroSenderoFeria(
                m, sec[s], sec[s + 1], -0.50f, 0.50f, LIFT_DETALLE,
                MezclaFeria(tierra, Color{ 196, 172, 140, 255 }, 0.16f)
            );
        }
    }

    // Confeti pisado y piedritas en el borde (solo calidad alta).
    if (nivel == CALIDAD_ALTA)
    {
        for (int s = 1; s < n; s++)
        {
            float v = HashFeria(semillaA * 97 + s, semillaB * 13 + 5);

            if (v < 0.45f)
            {
                float lat = (HashFeria(s * 3 + semillaA, semillaB) - 0.5f) * 2.6f;
                float lado = 0.07f;
                float cx = sec[s].x + sec[s].nx * lat;
                float cz = sec[s].z + sec[s].nz * lat;

                Color confeti =
                    MezclaFeria(
                        COLORES_BOMBILLA[(s + semillaA) % CANTIDAD_COLORES_BOMBILLA],
                        ma.tierra, 0.35f
                    );

                m.TrianguloDobleCara(
                    Vector3{ cx - lado, LIFT_DETALLE + 0.004f, cz },
                    Vector3{ cx, LIFT_DETALLE + 0.004f, cz - lado },
                    Vector3{ cx + lado, LIFT_DETALLE + 0.004f, cz + lado * 0.6f },
                    confeti
                );
            }

            if (s % 2 == 1 && v > 0.7f)
            {
                for (int lado = -1; lado <= 1; lado += 2)
                {
                    float w = HashFeria(semillaA * 89 + s, semillaB * 11 + lado + 5);

                    if (w > 0.5f)
                    {
                        continue;
                    }

                    float lat = lado * (SENDERO_SEMIANCHO + 0.05f + w * 0.4f);
                    float r = 0.06f + w * 0.12f;

                    m.Esfera(
                        Vector3{ sec[s].x + sec[s].nx * lat, r * 0.2f, sec[s].z + sec[s].nz * lat },
                        r, 3, 5,
                        OscurecerFeria(Color{ 88, 84, 86, 255 }, 0.8f + w * 0.6f)
                    );
                }
            }
        }
    }
}


static void ConstruirMallaSenderosFeria(NivelCalidadGrafica nivel)
{
    const Tablero& t = TableroReferenciaFeria();

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

            if (!ConexionInversaFeria(t, i, j))
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

            if (pb == nullptr || (j < i && ConexionInversaFeria(t, i, j)))
            {
                continue;
            }

            bool curvo =
                a.zona != ZONA_GRAN_RUEDA ||
                pb->zona != ZONA_GRAN_RUEDA;

            TramoSenderoFeria(
                m,
                a.posicion, a.zona,
                pb->posicion, pb->zona,
                i, j, curvo, nivel
            );
        }
    }

    // --- Camino de entrada (del inicio al borde sur, bajo el portal) ---
    const Vector3& inicio = t.casillas[nodosFeria.inicio].posicion;

    TramoSenderoFeria(
        m,
        inicio, ZONA_ENTRADA,
        Vector3{ inicio.x, 0.0f, ENTRADA_SUR_Z }, ZONA_ENTRADA,
        301, 302, false, nivel
    );

    // --- Nodos: disco de union y plaza en bifurcaciones ---
    for (int i = 0; i < t.cantidadCasillas; i++)
    {
        const Casilla& c = t.casillas[i];

        MaterialSenderoFeria ms = MaterialSenderoZonaFeria(c.zona);
        float y = c.posicion.y - ALTURA_CASILLA_FERIA;

        bool plaza = grado[i] >= 3;
        float semi = plaza ? SENDERO_PLAZA : SENDERO_SEMIANCHO;

        DiscoSenderoFeria(
            m, c.posicion.x, y, c.posicion.z,
            semi + SENDERO_BORDE, LIFT_BORDE_NODO, 20,
            MezclaFeria(ms.borde, COLOR_BORDE_SUELO_FERIA, 0.45f)
        );

        DiscoSenderoFeria(
            m, c.posicion.x, y, c.posicion.z,
            semi, LIFT_TIERRA_NODO, 20, ms.tierra
        );

        if (plaza)
        {
            Color claro = MezclaFeria(ms.tierra, Color{ 196, 170, 150, 255 }, 0.4f);
            Color oscuro = MezclaFeria(ms.tierra, Color{ 52, 40, 52, 255 }, 0.45f);

            AnilloSenderoFeria(
                m, c.posicion.x, y, c.posicion.z,
                SENDERO_SEMIANCHO + 0.05f, SENDERO_PLAZA - 0.05f,
                LIFT_DETALLE, claro, oscuro, 16
            );
        }
    }

    // Luz de luna y charcos de luz.
    int total = m.CantidadVertices();

    IluminarMallaFeria(m, 0);

    for (int v = 0; v + 2 < total; v += 3)
    {
        float cx = (m.posiciones[v * 3] + m.posiciones[v * 3 + 3] + m.posiciones[v * 3 + 6]) / 3.0f;
        float cz = (m.posiciones[v * 3 + 2] + m.posiciones[v * 3 + 5] + m.posiciones[v * 3 + 8]) / 3.0f;

        Color base =
        {
            m.colores[v * 4], m.colores[v * 4 + 1], m.colores[v * 4 + 2], 255
        };

        Color tenido = TenirConLucesFeria(base, cx, cz);

        for (int k = 0; k < 3; k++)
        {
            m.colores[(v + k) * 4] = tenido.r;
            m.colores[(v + k) * 4 + 1] = tenido.g;
            m.colores[(v + k) * 4 + 2] = tenido.b;
        }
    }

    ActualizarMallaGpu(mallaSenderosFeria, m);
    nivelSenderosFeria = (int)nivel;
}


static void DibujarSenderosFeria()
{
    NivelCalidadGrafica nivel = CalidadDecoracion();

    if (!mallaSenderosFeria.cargada || nivelSenderosFeria != (int)nivel)
    {
        ConstruirMallaSenderosFeria(nivel);
    }

    DibujarMallaGpu(mallaSenderosFeria);
}


//==================================================
// PIEZAS DE ESCENOGRAFIA (se emiten a mallas)
//==================================================
//
// Cada pieza es una funcion que emite a un ConstructorMalla; la misma
// funcion se reutiliza cientos de veces en la malla de decoracion. Las
// bombillas y ventanas son emisivas (no las oscurece la luz horneada).

static const Color CORTEZA_FERIA = Color{ 58, 44, 40, 255 };
static const Color ACERO_FERIA = Color{ 196, 196, 210, 255 };
static const Color LUZ_CALIDA_FERIA = Color{ 255, 220, 150, 255 };


static void CajaGiradaFeria(
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


// Punto local de una pieza con frente: (lx, lz) girado "giro" grados.
static Vector3 LocalFeria(
    float x, float y, float z,
    float giro,
    float lx, float ly, float lz
)
{
    float a = giro * DEG2RAD;
    float c = std::cos(a);
    float s = std::sin(a);

    return Vector3{ x + lx * c + lz * s, y + ly, z - lx * s + lz * c };
}


static void BombillaHorneadaFeria(
    ConstructorMalla& m,
    Vector3 p,
    float radio,
    Color color
)
{
    m.Esfera(p, radio, 2, 4, EmisivoFeria(color));
}


// Arbol de copa redonda; con "luces", una tira de bombillas alrededor.
static void PiezaArbolFeria(
    ConstructorMalla& m,
    float x, float y, float z,
    float escala, float variacion,
    bool luces
)
{
    Color copa = MezclaFeria(Color{ 24, 50, 48, 255 }, Color{ 38, 70, 58, 255 }, variacion);

    m.Cilindro(
        Vector3{ x, y - 0.05f, z }, Vector3{ x, y + 1.0f * escala, z },
        0.15f * escala, 0.10f * escala, 5, CORTEZA_FERIA
    );

    m.Esfera(Vector3{ x, y + 1.45f * escala, z }, 0.85f * escala, 5, 7, copa);
    m.Esfera(Vector3{ x + 0.35f * escala, y + 1.95f * escala, z - 0.1f * escala }, 0.58f * escala, 4, 6, MezclaFeria(copa, Color{ 60, 96, 80, 255 }, 0.25f));
    m.Esfera(Vector3{ x - 0.32f * escala, y + 1.88f * escala, z + 0.16f * escala }, 0.52f * escala, 4, 6, OscurecerFeria(copa, 0.92f));

    if (!luces)
    {
        return;
    }

    // Espiral de bombillas sobre la copa.
    for (int k = 0; k < 11; k++)
    {
        float a = variacion * 6.0f + k * 1.9f;
        float h = 1.05f + 0.10f * k;
        float r = 0.88f - 0.035f * k;

        Color luz =
            variacion < 0.5f
            ? LUZ_CALIDA_FERIA
            : COLORES_BOMBILLA[k % CANTIDAD_COLORES_BOMBILLA];

        BombillaHorneadaFeria(
            m,
            Vector3{ x + std::cos(a) * r * escala, y + h * escala, z + std::sin(a) * r * escala },
            0.065f * escala,
            luz
        );
    }
}


static void PiezaPinoFeria(
    ConstructorMalla& m,
    float x, float y, float z,
    float escala, float variacion
)
{
    Color verde = MezclaFeria(Color{ 18, 40, 42, 255 }, Color{ 28, 58, 52, 255 }, variacion);

    m.Cilindro(
        Vector3{ x, y - 0.05f, z }, Vector3{ x, y + 0.6f * escala, z },
        0.12f * escala, 0.09f * escala, 5, CORTEZA_FERIA
    );

    for (int k = 0; k < 3; k++)
    {
        float base = (0.45f + 0.62f * k) * escala;
        float radio = (0.95f - 0.24f * k) * escala;

        m.Cilindro(
            Vector3{ x, y + base, z }, Vector3{ x, y + base + 1.15f * escala, z },
            radio, 0.0f, 7,
            OscurecerFeria(verde, 0.9f + 0.08f * k)
        );
    }
}


static void PiezaArbustoFeria(
    ConstructorMalla& m,
    float x, float y, float z,
    float escala, float variacion
)
{
    Color hoja = MezclaFeria(Color{ 24, 48, 44, 255 }, Color{ 40, 72, 56, 255 }, variacion);

    for (int k = 0; k < 3; k++)
    {
        float a = variacion * 6.0f + k * 2.1f;
        float r = (0.30f + 0.08f * k) * escala;

        m.Esfera(
            Vector3{ x + std::cos(a) * 0.22f * escala, y + r * 0.7f, z + std::sin(a) * 0.22f * escala },
            r, 3, 6,
            OscurecerFeria(hoja, 0.9f + 0.1f * k)
        );
    }

    // Algunos con flores palidas.
    if (variacion > 0.7f)
    {
        for (int k = 0; k < 4; k++)
        {
            float a = k * 1.6f + variacion * 3.0f;

            m.Esfera(
                Vector3{ x + std::cos(a) * 0.3f * escala, y + 0.55f * escala, z + std::sin(a) * 0.3f * escala },
                0.06f * escala, 2, 4,
                k % 2 == 0 ? Color{ 230, 200, 230, 255 } : Color{ 240, 230, 170, 255 }
            );
        }
    }
}


// Mata de pasto alto: hojas finas en abanico.
static void PiezaPastoFeria(
    ConstructorMalla& m,
    float x, float y, float z,
    float escala, float variacion
)
{
    Color hoja = MezclaFeria(Color{ 40, 72, 56, 255 }, Color{ 60, 92, 64, 255 }, variacion);

    for (int k = 0; k < 5; k++)
    {
        float a = variacion * 5.0f + k * 1.257f;
        float largo = (0.32f + 0.16f * HashFeria(k, (int)(variacion * 71.0f))) * escala;

        m.Cilindro(
            Vector3{ x, y - 0.02f, z },
            Vector3{ x + std::cos(a) * largo * 0.4f, y + largo, z + std::sin(a) * largo * 0.4f },
            0.045f * escala, 0.0f, 3, hoja
        );
    }
}


static void PiezaFloresFeria(
    ConstructorMalla& m,
    float x, float y, float z,
    float escala, float variacion
)
{
    static const Color PETALOS[] =
    {
        { 236, 120, 180, 255 },
        { 246, 226, 140, 255 },
        { 170, 150, 240, 255 },
        { 240, 240, 236, 255 }
    };

    Color petalo = PETALOS[(int)(variacion * 3.99f)];

    for (int k = 0; k < 5; k++)
    {
        float a = variacion * 4.0f + k * 1.3f;
        float r = 0.18f * escala * (0.4f + 0.6f * HashFeria(k, (int)(variacion * 37.0f)));
        float alto = (0.22f + 0.1f * (k % 2)) * escala;

        Vector3 pie = { x + std::cos(a) * r, y, z + std::sin(a) * r };
        Vector3 flor = { pie.x, y + alto, pie.z };

        m.Cilindro(pie, flor, 0.018f, 0.018f, 3, Color{ 46, 84, 54, 255 });
        m.Esfera(flor, 0.065f * escala, 2, 4, petalo);
    }
}


static void PiezaRocaFeria(
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

    m.Esfera(Vector3{ 0.0f, radio * 0.45f, 0.0f }, radio, 4, 6, color);

    m.Esfera(
        Vector3{ radio * 0.25f, radio * 0.95f, -radio * 0.1f },
        radio * 0.55f, 3, 5,
        MezclaFeria(color, Color{ 140, 140, 160, 255 }, 0.2f)
    );

    m.QuitarTransformacion();
}


// Fardo de heno acostado.
static void PiezaFardoFeria(
    ConstructorMalla& m,
    float x, float y, float z,
    float giro
)
{
    Vector3 a = LocalFeria(x, y + 0.36f, z, giro, -0.45f, 0.0f, 0.0f);
    Vector3 b = LocalFeria(x, y + 0.36f, z, giro, 0.45f, 0.0f, 0.0f);

    m.Cilindro(a, b, 0.38f, 0.38f, 9, Color{ 170, 140, 82, 255 });

    for (int k = -1; k <= 1; k += 2)
    {
        Vector3 c0 = LocalFeria(x, y + 0.36f, z, giro, 0.22f * k - 0.03f, 0.0f, 0.0f);
        Vector3 c1 = LocalFeria(x, y + 0.36f, z, giro, 0.22f * k + 0.03f, 0.0f, 0.0f);

        m.Cilindro(c0, c1, 0.39f, 0.39f, 9, Color{ 120, 92, 56, 255 });
    }
}


static void PiezaBarrilFeria(
    ConstructorMalla& m,
    float x, float y, float z,
    Color color
)
{
    m.Cilindro(Vector3{ x, y, z }, Vector3{ x, y + 0.75f, z }, 0.3f, 0.3f, 8, color);
    m.Cilindro(Vector3{ x, y + 0.14f, z }, Vector3{ x, y + 0.2f, z }, 0.315f, 0.315f, 8, Color{ 70, 66, 72, 255 });
    m.Cilindro(Vector3{ x, y + 0.56f, z }, Vector3{ x, y + 0.62f, z }, 0.315f, 0.315f, 8, Color{ 70, 66, 72, 255 });
}


static void PiezaCajonFeria(
    ConstructorMalla& m,
    float x, float y, float z,
    float giro, float lado
)
{
    CajaGiradaFeria(m, x, y + lado * 0.5f, z, giro, Vector3{ lado, lado, lado }, Color{ 150, 112, 72, 255 });
    CajaGiradaFeria(m, x, y + lado * 0.5f, z, giro, Vector3{ lado * 1.02f, lado * 0.16f, lado * 1.02f }, MADERA_FERIA_OSCURA);
}


// Banco de plaza mirando hacia "giro" (frente = +z local).
static void PiezaBancoFeria(
    ConstructorMalla& m,
    float x, float y, float z,
    float giro
)
{
    Vector3 asiento = LocalFeria(x, y + 0.42f, z, giro, 0.0f, 0.0f, 0.0f);
    Vector3 respaldo = LocalFeria(x, y + 0.72f, z, giro, 0.0f, 0.0f, -0.2f);

    CajaGiradaFeria(m, asiento.x, asiento.y, asiento.z, giro, Vector3{ 1.3f, 0.07f, 0.42f }, MADERA_FERIA);
    CajaGiradaFeria(m, respaldo.x, respaldo.y, respaldo.z, giro, Vector3{ 1.3f, 0.32f, 0.06f }, MADERA_FERIA);

    for (int k = -1; k <= 1; k += 2)
    {
        Vector3 pata = LocalFeria(x, y + 0.2f, z, giro, 0.55f * k, 0.0f, 0.0f);

        CajaGiradaFeria(m, pata.x, pata.y, pata.z, giro, Vector3{ 0.07f, 0.42f, 0.42f }, Color{ 40, 36, 44, 255 });
    }
}


static void PiezaTachoFeria(
    ConstructorMalla& m,
    float x, float y, float z
)
{
    m.Cilindro(Vector3{ x, y, z }, Vector3{ x, y + 0.7f, z }, 0.24f, 0.27f, 8, Color{ 46, 112, 80, 255 });
    m.Cilindro(Vector3{ x, y + 0.7f, z }, Vector3{ x, y + 0.78f, z }, 0.29f, 0.25f, 8, Color{ 34, 84, 62, 255 });
}


// Farol de feria: poste, farol emisivo y sombrerete. El halo es inmediato.
static const float ALTO_FAROL_FERIA = 2.3f;

static void PiezaFarolFeria(
    ConstructorMalla& m,
    float x, float y, float z
)
{
    m.Cilindro(Vector3{ x, y, z }, Vector3{ x, y + 0.15f, z }, 0.16f, 0.13f, 6, Color{ 34, 30, 40, 255 });
    m.Cilindro(Vector3{ x, y, z }, Vector3{ x, y + ALTO_FAROL_FERIA, z }, 0.05f, 0.045f, 5, Color{ 34, 30, 40, 255 });
    m.Caja(Vector3{ x, y + ALTO_FAROL_FERIA + 0.1f, z }, Vector3{ 0.24f, 0.3f, 0.24f }, EmisivoFeria(Color{ 255, 214, 140, 255 }));
    m.Cilindro(Vector3{ x, y + ALTO_FAROL_FERIA + 0.25f, z }, Vector3{ x, y + ALTO_FAROL_FERIA + 0.45f, z }, 0.22f, 0.0f, 6, Color{ 40, 34, 46, 255 });
}


// Racimo de globos atados a una estaca.
static void PiezaGlobosFeria(
    ConstructorMalla& m,
    float x, float y, float z,
    float variacion
)
{
    static const Color GLOBOS[] =
    {
        { 236, 70, 90, 255 },
        { 250, 210, 70, 255 },
        { 90, 200, 240, 255 },
        { 200, 110, 240, 255 },
        { 110, 220, 140, 255 }
    };

    Vector3 nudo = { x, y + 1.0f, z };

    m.Cilindro(Vector3{ x, y, z }, nudo, 0.04f, 0.035f, 4, MADERA_FERIA_OSCURA);

    for (int k = 0; k < 5; k++)
    {
        float a = variacion * 6.0f + k * 1.26f;
        float r = 0.32f + 0.12f * (k % 2);
        Vector3 globo = { x + std::cos(a) * r, y + 2.0f + 0.25f * HashFeria(k, (int)(variacion * 91.0f)), z + std::sin(a) * r };

        m.Cilindro(nudo, Vector3{ globo.x, globo.y - 0.25f, globo.z }, 0.01f, 0.01f, 3, Color{ 220, 220, 220, 255 });

        TransformacionMalla t;
        t.tx = globo.x;
        t.ty = globo.y;
        t.tz = globo.z;
        t.sy = 1.2f;

        m.EstablecerTransformacion(t);
        m.Esfera(Vector3{ 0.0f, 0.0f, 0.0f }, 0.24f, 4, 7, GLOBOS[(k + (int)(variacion * 5.0f)) % 5]);
        m.QuitarTransformacion();
    }
}


// Tramo de valla de madera entre dos puntos.
static void PiezaVallaFeria(
    ConstructorMalla& m,
    Vector3 a,
    Vector3 b,
    Color color
)
{
    float dx = b.x - a.x;
    float dz = b.z - a.z;
    float largo = std::sqrt(dx * dx + dz * dz);
    int postes = 1 + (int)(largo / 1.1f);

    for (int k = 0; k <= postes; k++)
    {
        float u = (float)k / postes;
        float x = a.x + dx * u;
        float z = a.z + dz * u;
        float y = a.y + (b.y - a.y) * u;

        m.Cilindro(Vector3{ x, y - 0.05f, z }, Vector3{ x, y + 0.7f, z }, 0.05f, 0.05f, 4, color);
    }

    for (int k = 0; k < 2; k++)
    {
        float h = 0.32f + 0.3f * k;

        m.Cilindro(
            Vector3{ a.x, a.y + h, a.z }, Vector3{ b.x, b.y + h, b.z },
            0.035f, 0.035f, 4,
            OscurecerFeria(color, 1.1f)
        );
    }
}


// Carpa de circo: pared y techo conico a rayas, entrada oscura hacia la
// camara, festones con bombillas y banderin en la punta.
static void PiezaCarpaFeria(
    ConstructorMalla& m,
    float x, float y, float z,
    float radio,
    float alto,
    Color rayaA,
    Color rayaB,
    bool bombillas
)
{
    const int GAJOS = 16;
    float altoPared = alto * 0.42f;
    float radioTecho = radio * 1.14f;

    Vector3 punta = { x, y + alto, z };

    for (int i = 0; i < GAJOS; i++)
    {
        float a0 = 2.0f * PI * i / GAJOS;
        float a1 = 2.0f * PI * (i + 1) / GAJOS;
        float medio = (a0 + a1) * 0.5f;

        Color raya = i % 2 == 0 ? rayaA : rayaB;

        Vector3 p00 = { x + std::cos(a0) * radio, y, z + std::sin(a0) * radio };
        Vector3 p10 = { x + std::cos(a1) * radio, y, z + std::sin(a1) * radio };
        Vector3 p01 = { p00.x, y + altoPared, p00.z };
        Vector3 p11 = { p10.x, y + altoPared, p10.z };

        m.TrianguloDobleCara(p00, p10, p11, raya);
        m.TrianguloDobleCara(p00, p11, p01, raya);

        Vector3 t0 = { x + std::cos(a0) * radioTecho, y + altoPared - 0.08f, z + std::sin(a0) * radioTecho };
        Vector3 t1 = { x + std::cos(a1) * radioTecho, y + altoPared - 0.08f, z + std::sin(a1) * radioTecho };

        m.TrianguloDobleCara(punta, t0, t1, raya);

        // Festones del alero.
        Vector3 colgante = { x + std::cos(medio) * radioTecho, y + altoPared - 0.30f, z + std::sin(medio) * radioTecho };

        m.TrianguloDobleCara(t0, t1, colgante, i % 2 == 0 ? rayaB : rayaA);

        if (bombillas)
        {
            BombillaHorneadaFeria(
                m, Vector3{ t0.x, t0.y - 0.02f, t0.z }, 0.055f,
                COLORES_BOMBILLA[i % CANTIDAD_COLORES_BOMBILLA]
            );
        }
    }

    // Entrada (mira a la camara, +z) con luz calida adentro.
    m.Caja(
        Vector3{ x, y + altoPared * 0.42f, z + radio * 0.97f },
        Vector3{ radio * 0.62f, altoPared * 0.84f, 0.05f },
        EmisivoFeria(Color{ 120, 70, 50, 255 })
    );

    // Mastil y banderin.
    m.Cilindro(
        Vector3{ x, y + alto - 0.05f, z },
        Vector3{ x, y + alto + 0.65f, z },
        0.035f, 0.035f, 5,
        MADERA_FERIA_OSCURA
    );

    m.Caja(
        Vector3{ x + 0.2f, y + alto + 0.52f, z },
        Vector3{ 0.36f, 0.20f, 0.02f },
        rayaA
    );
}


// Puesto de feria: casilla con toldo a rayas, mostrador iluminado y un
// adorno en el techo segun el tipo (0 pochoclo, 1 panchos, 2 helados,
// 3 algodon de azucar, 4 manzanas, 5 juego de aros con peluches).
static void PiezaPuestoFeria(
    ConstructorMalla& m,
    float x, float z,
    float giro,
    int tipo
)
{
    static const Color TOLDOS[][2] =
    {
        { { 214, 58, 64, 255 }, { 240, 232, 214, 255 } },
        { { 236, 170, 50, 255 }, { 200, 70, 50, 255 } },
        { { 110, 196, 230, 255 }, { 250, 240, 250, 255 } },
        { { 240, 130, 196, 255 }, { 250, 236, 246, 255 } },
        { { 90, 178, 110, 255 }, { 236, 226, 190, 255 } },
        { { 120, 80, 200, 255 }, { 246, 210, 80, 255 } }
    };

    float y = AlturaSueloFeria(x, z);
    const Color* toldo = TOLDOS[tipo % 6];
    Color madera = MezclaFeria(MADERA_FERIA, toldo[0], 0.25f);

    // Cuerpo y mostrador (frente = +z local).
    Vector3 cuerpo = LocalFeria(x, y + 0.75f, z, giro, 0.0f, 0.0f, -0.15f);
    CajaGiradaFeria(m, cuerpo.x, cuerpo.y, cuerpo.z, giro, Vector3{ 2.0f, 1.5f, 1.1f }, madera);

    Vector3 mostrador = LocalFeria(x, y + 0.5f, z, giro, 0.0f, 0.0f, 0.5f);
    CajaGiradaFeria(m, mostrador.x, mostrador.y, mostrador.z, giro, Vector3{ 2.1f, 1.0f, 0.3f }, toldo[1]);

    Vector3 tabla = LocalFeria(x, y + 1.02f, z, giro, 0.0f, 0.0f, 0.55f);
    CajaGiradaFeria(m, tabla.x, tabla.y, tabla.z, giro, Vector3{ 2.2f, 0.06f, 0.45f }, MADERA_FERIA_OSCURA);

    // Ventana iluminada.
    Vector3 ventana = LocalFeria(x, y + 1.35f, z, giro, 0.0f, 0.0f, 0.42f);
    CajaGiradaFeria(m, ventana.x, ventana.y, ventana.z, giro, Vector3{ 1.7f, 0.55f, 0.03f }, EmisivoFeria(Color{ 255, 206, 130, 255 }));

    // Postes y toldo a rayas inclinado.
    for (int k = -1; k <= 1; k += 2)
    {
        Vector3 poste = LocalFeria(x, y, z, giro, 0.98f * k, 0.0f, 0.62f);
        m.Cilindro(poste, Vector3{ poste.x, poste.y + 2.05f, poste.z }, 0.04f, 0.04f, 4, MADERA_FERIA_OSCURA);
    }

    const int RAYAS = 6;

    for (int r = 0; r < RAYAS; r++)
    {
        float x0 = -1.1f + 2.2f * r / RAYAS;
        float x1 = -1.1f + 2.2f * (r + 1) / RAYAS;

        Vector3 a0 = LocalFeria(x, y + 2.35f, z, giro, x0, 0.0f, -0.7f);
        Vector3 a1 = LocalFeria(x, y + 2.35f, z, giro, x1, 0.0f, -0.7f);
        Vector3 b0 = LocalFeria(x, y + 1.95f, z, giro, x0, 0.0f, 0.85f);
        Vector3 b1 = LocalFeria(x, y + 1.95f, z, giro, x1, 0.0f, 0.85f);

        Color c = toldo[r % 2];

        m.TrianguloDobleCara(a0, a1, b1, c);
        m.TrianguloDobleCara(a0, b1, b0, c);

        // Faldon del toldo y una bombilla por raya.
        Vector3 f = LocalFeria(x, y + 1.82f, z, giro, (x0 + x1) * 0.5f, 0.0f, 0.86f);
        m.TrianguloDobleCara(b0, b1, f, toldo[(r + 1) % 2]);

        BombillaHorneadaFeria(m, Vector3{ f.x, f.y - 0.05f, f.z }, 0.05f, COLORES_BOMBILLA[r % CANTIDAD_COLORES_BOMBILLA]);
    }

    // Cartel del techo con bombillas.
    Vector3 cartel = LocalFeria(x, y + 2.65f, z, giro, 0.0f, 0.0f, -0.2f);
    CajaGiradaFeria(m, cartel.x, cartel.y, cartel.z, giro, Vector3{ 1.4f, 0.5f, 0.08f }, toldo[0]);

    for (int k = 0; k < 6; k++)
    {
        Vector3 b = LocalFeria(x, y + 2.92f, z, giro, -0.6f + 0.24f * k, 0.0f, -0.15f);
        BombillaHorneadaFeria(m, b, 0.045f, LUZ_CALIDA_FERIA);
    }

    // Adorno del techo segun el tipo.
    Vector3 techo = LocalFeria(x, y + 3.0f, z, giro, 0.0f, 0.0f, -0.2f);

    switch (tipo % 6)
    {
        case 0:
            // Caja de pochoclo gigante.
            m.Cilindro(techo, Vector3{ techo.x, techo.y + 0.55f, techo.z }, 0.22f, 0.3f, 8, Color{ 230, 60, 60, 255 });
            m.Esfera(Vector3{ techo.x, techo.y + 0.6f, techo.z }, 0.3f, 3, 6, Color{ 250, 240, 200, 255 });
            break;

        case 1:
        {
            // Pancho gigante.
            Vector3 a = LocalFeria(x, y + 3.15f, z, giro, -0.45f, 0.0f, -0.2f);
            Vector3 b = LocalFeria(x, y + 3.15f, z, giro, 0.45f, 0.0f, -0.2f);
            m.Cilindro(a, b, 0.2f, 0.2f, 8, Color{ 226, 170, 100, 255 });
            Vector3 c = LocalFeria(x, y + 3.25f, z, giro, -0.55f, 0.0f, -0.2f);
            Vector3 d = LocalFeria(x, y + 3.25f, z, giro, 0.55f, 0.0f, -0.2f);
            m.Cilindro(c, d, 0.11f, 0.11f, 6, Color{ 190, 80, 60, 255 });
            break;
        }

        case 2:
            // Cucurucho de helado.
            m.Cilindro(techo, Vector3{ techo.x, techo.y + 0.55f, techo.z }, 0.02f, 0.2f, 8, Color{ 214, 160, 90, 255 });
            m.Esfera(Vector3{ techo.x, techo.y + 0.68f, techo.z }, 0.24f, 4, 7, Color{ 246, 190, 210, 255 });
            break;

        case 3:
            // Nube de algodon de azucar.
            m.Cilindro(techo, Vector3{ techo.x, techo.y + 0.4f, techo.z }, 0.03f, 0.03f, 4, CREMA_FERIA);
            m.Esfera(Vector3{ techo.x, techo.y + 0.62f, techo.z }, 0.34f, 4, 7, Color{ 255, 170, 214, 255 });
            m.Esfera(Vector3{ techo.x + 0.2f, techo.y + 0.7f, techo.z }, 0.22f, 4, 6, Color{ 190, 220, 255, 255 });
            break;

        case 4:
            // Manzana acaramelada.
            m.Esfera(Vector3{ techo.x, techo.y + 0.3f, techo.z }, 0.3f, 4, 7, Color{ 200, 30, 40, 255 });
            m.Cilindro(Vector3{ techo.x, techo.y + 0.5f, techo.z }, Vector3{ techo.x, techo.y + 0.85f, techo.z }, 0.03f, 0.03f, 4, MADERA_FERIA);
            break;

        default:
            // Peluches colgados del toldo.
            for (int k = 0; k < 4; k++)
            {
                Vector3 p = LocalFeria(x, y + 1.62f, z, giro, -0.75f + 0.5f * k, 0.0f, 0.78f);
                m.Esfera(p, 0.14f, 3, 6, COLORES_BOMBILLA[(k + 1) % CANTIDAD_COLORES_BOMBILLA]);
                m.Esfera(Vector3{ p.x, p.y + 0.16f, p.z }, 0.09f, 3, 5, COLORES_BOMBILLA[(k + 1) % CANTIDAD_COLORES_BOMBILLA]);
            }
            break;
    }
}


// Casa rodante de los feriantes, con ventanas encendidas.
static void PiezaRemolqueFeria(
    ConstructorMalla& m,
    float x, float z,
    float giro,
    Color color
)
{
    float y = AlturaSueloFeria(x, z);

    Vector3 cuerpo = LocalFeria(x, y + 1.0f, z, giro, 0.0f, 0.0f, 0.0f);
    CajaGiradaFeria(m, cuerpo.x, cuerpo.y, cuerpo.z, giro, Vector3{ 3.2f, 1.3f, 1.5f }, color);

    Vector3 techo = LocalFeria(x, y + 1.72f, z, giro, 0.0f, 0.0f, 0.0f);
    CajaGiradaFeria(m, techo.x, techo.y, techo.z, giro, Vector3{ 3.0f, 0.16f, 1.4f }, CREMA_FERIA);

    Vector3 franja = LocalFeria(x, y + 0.75f, z, giro, 0.0f, 0.0f, 0.0f);
    CajaGiradaFeria(m, franja.x, franja.y, franja.z, giro, Vector3{ 3.22f, 0.14f, 1.52f }, CREMA_FERIA);

    for (int k = -1; k <= 1; k += 2)
    {
        Vector3 ventana = LocalFeria(x, y + 1.15f, z, giro, 0.8f * k, 0.0f, 0.76f);
        CajaGiradaFeria(m, ventana.x, ventana.y, ventana.z, giro, Vector3{ 0.55f, 0.38f, 0.03f }, EmisivoFeria(Color{ 255, 196, 120, 255 }));

        for (int lado = -1; lado <= 1; lado += 2)
        {
            Vector3 rueda0 = LocalFeria(x, y + 0.3f, z, giro, 0.7f * k, 0.0f, 0.68f * lado);
            Vector3 rueda1 = LocalFeria(x, y + 0.3f, z, giro, 0.7f * k, 0.0f, 0.84f * lado);
            m.Cilindro(rueda0, rueda1, 0.3f, 0.3f, 8, Color{ 30, 28, 34, 255 });
        }
    }

    Vector3 lanza0 = LocalFeria(x, y + 0.4f, z, giro, 1.6f, 0.0f, 0.0f);
    Vector3 lanza1 = LocalFeria(x, y + 0.25f, z, giro, 2.4f, 0.0f, 0.0f);
    m.Cilindro(lanza0, lanza1, 0.05f, 0.05f, 4, Color{ 40, 38, 44, 255 });
}


//==================================================
// MONTAÑA RUSA (pista muestreada una vez)
//==================================================
//
// Pista cerrada detras del lobulo noroeste (z <= -19, nada queda delante de
// una casilla): estacion, subida con cadena hasta 11 de alto, caida, rizo,
// joroba y curva de vuelta. Se muestrea por largo de arco con un marco
// (tangente, lateral, arriba) transportado, para los rieles horneados y
// para el tren animado.

static const float PUNTOS_RUSA[][3] =
{
    {  -6.6f,  1.3f, -19.5f },   // estacion
    {  -9.0f,  1.3f, -19.5f },
    { -11.4f,  1.4f, -19.5f },   // subida
    { -14.0f,  3.0f, -19.5f },
    { -17.0f,  5.6f, -19.5f },
    { -20.0f,  8.4f, -19.5f },
    { -22.6f, 10.6f, -19.6f },
    { -24.6f, 11.0f, -20.2f },   // cima
    { -26.6f,  8.6f, -21.2f },   // caida
    { -27.9f,  4.4f, -22.4f },
    { -27.4f,  1.6f, -23.7f },
    { -25.5f,  1.4f, -24.1f },
    { -23.0f,  1.4f, -24.3f },   // rizo
    { -20.88f, 2.28f, -24.45f },
    { -20.0f,  4.4f, -24.6f },
    { -20.88f, 6.52f, -24.75f },
    { -23.0f,  7.4f, -24.9f },
    { -25.12f, 6.52f, -25.05f },
    { -26.0f,  4.4f, -25.2f },
    { -25.12f, 2.28f, -25.35f },
    { -23.0f,  1.4f, -25.5f },
    { -20.5f,  1.6f, -25.7f },
    { -17.0f,  5.2f, -26.0f },   // joroba
    { -14.0f,  6.0f, -26.1f },
    { -11.0f,  2.6f, -26.0f },
    {  -8.4f,  2.2f, -25.6f },
    {  -6.4f,  3.4f, -24.4f },   // curva de vuelta
    {  -5.6f,  2.6f, -22.4f },
    {  -5.9f,  1.6f, -20.5f }
};

static const int CANTIDAD_PUNTOS_RUSA =
    (int)(sizeof(PUNTOS_RUSA) / sizeof(PUNTOS_RUSA[0]));

// Tramo de la subida (cadena: velocidad constante).
static const int RUSA_SUBIDA_DESDE = 2;
static const int RUSA_SUBIDA_HASTA = 7;

static const float PASO_RUSA = 0.35f;
static const float ALTO_CIMA_RUSA = 11.2f;
static const float PAUSA_ESTACION_RUSA = 2.5f;
static const float ANCHO_RIELES_RUSA = 0.38f;

struct MuestraRusaFeria
{
    Vector3 p;
    Vector3 tangente;
    Vector3 lateral;
    Vector3 arriba;
    float tiempo;     // segundos desde la salida de la estacion
};

static const int MAX_MUESTRAS_RUSA = 420;

static MuestraRusaFeria muestrasRusa[MAX_MUESTRAS_RUSA];
static int cantidadMuestrasRusa = 0;
static float vueltaRusa = 1.0f;


static Vector3 SumarFeria(Vector3 a, Vector3 b, float k)
{
    return Vector3{ a.x + b.x * k, a.y + b.y * k, a.z + b.z * k };
}


static Vector3 NormalizarFeria(Vector3 v)
{
    float l = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);

    if (l < 0.000001f)
    {
        return Vector3{ 0.0f, 1.0f, 0.0f };
    }

    return Vector3{ v.x / l, v.y / l, v.z / l };
}


static Vector3 CruzFeria(Vector3 a, Vector3 b)
{
    return Vector3
    {
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
}


static float PuntoFeria(Vector3 a, Vector3 b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}


static Vector3 CatmullRomFeria(int i, float t)
{
    const int n = CANTIDAD_PUNTOS_RUSA;
    const float* p0 = PUNTOS_RUSA[(i - 1 + n) % n];
    const float* p1 = PUNTOS_RUSA[i % n];
    const float* p2 = PUNTOS_RUSA[(i + 1) % n];
    const float* p3 = PUNTOS_RUSA[(i + 2) % n];

    float t2 = t * t;
    float t3 = t2 * t;
    float r[3];

    for (int k = 0; k < 3; k++)
    {
        r[k] = 0.5f * (
            2.0f * p1[k] +
            (-p0[k] + p2[k]) * t +
            (2.0f * p0[k] - 5.0f * p1[k] + 4.0f * p2[k] - p3[k]) * t2 +
            (-p0[k] + 3.0f * p1[k] - 3.0f * p2[k] + p3[k]) * t3
        );
    }

    return Vector3{ r[0], r[1], r[2] };
}


static void PrepararPistaRusaFeria()
{
    if (cantidadMuestrasRusa > 0)
    {
        return;
    }

    // Curva densa con su largo acumulado y el tramo de cada punto.
    const int SUB = 40;
    static Vector3 densa[CANTIDAD_PUNTOS_RUSA * SUB + 1];
    static float acumulado[CANTIDAD_PUNTOS_RUSA * SUB + 1];
    static int tramo[CANTIDAD_PUNTOS_RUSA * SUB + 1];
    int total = 0;

    for (int i = 0; i < CANTIDAD_PUNTOS_RUSA; i++)
    {
        for (int s = 0; s < SUB; s++)
        {
            densa[total] = CatmullRomFeria(i, (float)s / SUB);
            tramo[total] = i;
            total++;
        }
    }

    densa[total] = densa[0];
    tramo[total] = 0;
    acumulado[0] = 0.0f;

    for (int i = 1; i <= total; i++)
    {
        float dx = densa[i].x - densa[i - 1].x;
        float dy = densa[i].y - densa[i - 1].y;
        float dz = densa[i].z - densa[i - 1].z;

        acumulado[i] = acumulado[i - 1] + std::sqrt(dx * dx + dy * dy + dz * dz);
    }

    float largo = acumulado[total];
    int muestras = (int)(largo / PASO_RUSA);

    if (muestras > MAX_MUESTRAS_RUSA)
    {
        muestras = MAX_MUESTRAS_RUSA;
    }

    static bool subida[MAX_MUESTRAS_RUSA];
    int j = 0;

    for (int i = 0; i < muestras; i++)
    {
        float objetivo = largo * i / muestras;

        while (j + 1 < total && acumulado[j + 1] < objetivo)
        {
            j++;
        }

        float k = (objetivo - acumulado[j]) / (acumulado[j + 1] - acumulado[j] + 0.000001f);

        muestrasRusa[i].p = SumarFeria(densa[j], Vector3{ densa[j + 1].x - densa[j].x, densa[j + 1].y - densa[j].y, densa[j + 1].z - densa[j].z }, k);
        subida[i] = tramo[j] >= RUSA_SUBIDA_DESDE && tramo[j] < RUSA_SUBIDA_HASTA;
    }

    cantidadMuestrasRusa = muestras;

    // Marco transportado: lateral horizontal donde se puede (sin perder
    // el sentido en el rizo, que queda cabeza abajo).
    Vector3 lateral = { 0.0f, 0.0f, 1.0f };
    const Vector3 vertical = { 0.0f, 1.0f, 0.0f };

    for (int i = 0; i < muestras; i++)
    {
        const Vector3& a = muestrasRusa[(i - 1 + muestras) % muestras].p;
        const Vector3& b = muestrasRusa[(i + 1) % muestras].p;

        Vector3 t = NormalizarFeria(Vector3{ b.x - a.x, b.y - a.y, b.z - a.z });

        if (i == 0)
        {
            lateral = NormalizarFeria(CruzFeria(t, vertical));
        }

        lateral = NormalizarFeria(SumarFeria(lateral, t, -PuntoFeria(lateral, t)));

        Vector3 horizontal = CruzFeria(t, vertical);
        float largoH = std::sqrt(PuntoFeria(horizontal, horizontal));

        if (largoH > 0.3f)
        {
            horizontal = NormalizarFeria(horizontal);

            if (PuntoFeria(horizontal, lateral) > 0.5f)
            {
                lateral = NormalizarFeria(SumarFeria(lateral, horizontal, 0.25f));
            }
        }

        muestrasRusa[i].tangente = t;
        muestrasRusa[i].lateral = lateral;
        muestrasRusa[i].arriba = CruzFeria(lateral, t);
    }

    // Horario: cadena lenta en la subida; despues, la velocidad crece con
    // la caida desde la cima.
    float tiempo = 0.0f;

    for (int i = 0; i < muestras; i++)
    {
        muestrasRusa[i].tiempo = tiempo;

        float y = muestrasRusa[i].p.y;
        float caida = ALTO_CIMA_RUSA - y;

        float velocidad =
            subida[i]
            ? 1.9f
            : 2.4f + 2.3f * std::sqrt(caida > 0.0f ? caida : 0.0f);

        tiempo += (largo / muestras) / velocidad;
    }

    vueltaRusa = tiempo;
}


// Posicion de un punto del tren a "atras" muestras detras de la cabeza.
static int MuestraTrenRusaFeria(float tiempo, float& fraccion)
{
    float ciclo = vueltaRusa + PAUSA_ESTACION_RUSA;
    float t = std::fmod(tiempo, ciclo) - PAUSA_ESTACION_RUSA;

    if (t < 0.0f)
    {
        fraccion = 0.0f;
        return 0;
    }

    int bajo = 0;
    int alto = cantidadMuestrasRusa - 1;

    while (bajo < alto)
    {
        int medio = (bajo + alto + 1) / 2;

        if (muestrasRusa[medio].tiempo <= t)
        {
            bajo = medio;
        }
        else
        {
            alto = medio - 1;
        }
    }

    float t0 = muestrasRusa[bajo].tiempo;
    float t1 = bajo + 1 < cantidadMuestrasRusa ? muestrasRusa[bajo + 1].tiempo : vueltaRusa;

    fraccion = Limitar01((t - t0) / (t1 - t0 + 0.0001f));
    return bajo;
}


// Distancia horizontal a la pista (para no plantar nada debajo).
static float DistanciaPistaRusaFeria(float x, float z)
{
    float mejor = 1000.0f;

    for (int i = 0; i < cantidadMuestrasRusa; i += 2)
    {
        float dx = x - muestrasRusa[i].p.x;
        float dz = z - muestrasRusa[i].p.z;
        float d = dx * dx + dz * dz;

        if (d < mejor)
        {
            mejor = d;
        }
    }

    return std::sqrt(mejor);
}


static void ConstruirMontanaRusaFeria(ConstructorMalla& m)
{
    PrepararPistaRusaFeria();

    const Color riel = Color{ 214, 54, 70, 255 };
    const Color espina = Color{ 150, 36, 56, 255 };
    const Color durmiente = Color{ 110, 110, 124, 255 };
    const int n = cantidadMuestrasRusa;

    for (int i = 0; i < n; i++)
    {
        const MuestraRusaFeria& a = muestrasRusa[i];
        const MuestraRusaFeria& b = muestrasRusa[(i + 1) % n];

        // Dos rieles y la viga de abajo.
        for (int lado = -1; lado <= 1; lado += 2)
        {
            m.Cilindro(
                SumarFeria(a.p, a.lateral, ANCHO_RIELES_RUSA * lado),
                SumarFeria(b.p, b.lateral, ANCHO_RIELES_RUSA * lado),
                0.06f, 0.06f, 4, riel
            );
        }

        m.Cilindro(
            SumarFeria(a.p, a.arriba, -0.28f),
            SumarFeria(b.p, b.arriba, -0.28f),
            0.11f, 0.11f, 5, espina
        );

        // Durmientes y luces de borde.
        if (i % 2 == 0)
        {
            m.Cilindro(
                SumarFeria(a.p, a.lateral, -ANCHO_RIELES_RUSA),
                SumarFeria(a.p, a.lateral, ANCHO_RIELES_RUSA),
                0.04f, 0.04f, 3, durmiente
            );

            m.Cilindro(
                SumarFeria(a.p, a.arriba, -0.28f),
                a.p,
                0.035f, 0.035f, 3, durmiente
            );
        }

        if (i % 4 == 0)
        {
            BombillaHorneadaFeria(
                m,
                SumarFeria(SumarFeria(a.p, a.lateral, ANCHO_RIELES_RUSA + 0.08f), a.arriba, -0.05f),
                0.055f,
                COLORES_BOMBILLA[(i / 4) % CANTIDAD_COLORES_BOMBILLA]
            );
        }

        // Columnas donde la pista va derecha (en el rizo no).
        if (i % 6 == 3 && a.arriba.y > 0.6f && a.p.y > 0.9f)
        {
            Vector3 arriba = SumarFeria(a.p, a.arriba, -0.32f);
            float suelo = AlturaSueloFeria(a.p.x, a.p.z) - 0.05f;
            float alto = arriba.y - suelo;

            m.Cilindro(Vector3{ arriba.x, suelo, arriba.z }, arriba, 0.13f, 0.09f, 6, ACERO_FERIA);

            if (alto > 3.2f)
            {
                // Patas abiertas y travesaño.
                Vector3 lat = NormalizarFeria(Vector3{ a.lateral.x, 0.0f, a.lateral.z });

                for (int lado = -1; lado <= 1; lado += 2)
                {
                    Vector3 pie = { arriba.x + lat.x * 1.1f * lado, suelo, arriba.z + lat.z * 1.1f * lado };
                    m.Cilindro(pie, Vector3{ arriba.x, arriba.y - 0.3f, arriba.z }, 0.08f, 0.06f, 5, ACERO_FERIA);
                }

                float yb = suelo + alto * 0.45f;
                float abre = 1.1f * 0.55f;

                m.Cilindro(
                    Vector3{ arriba.x - lat.x * abre, yb, arriba.z - lat.z * abre },
                    Vector3{ arriba.x + lat.x * abre, yb, arriba.z + lat.z * abre },
                    0.05f, 0.05f, 4, ACERO_FERIA
                );
            }
        }
    }

    // Torres del rizo: a los lados, por fuera de la pista.
    const float lados[2] = { -20.0f, -26.0f };

    for (float x : lados)
    {
        for (int k = -1; k <= 1; k += 2)
        {
            float z = -24.9f + 1.0f * k;
            float suelo = AlturaSueloFeria(x, z) - 0.05f;

            m.Cilindro(Vector3{ x, suelo, z }, Vector3{ x, 4.4f, z }, 0.14f, 0.1f, 6, ACERO_FERIA);
            m.Cilindro(Vector3{ x, 4.4f, z }, Vector3{ x, 4.4f, -24.9f + 0.4f * k }, 0.07f, 0.07f, 4, ACERO_FERIA);
        }

        m.Cilindro(Vector3{ x, 2.2f, -25.9f }, Vector3{ x, 2.2f, -23.9f }, 0.06f, 0.06f, 4, ACERO_FERIA);
    }

    // Estacion: anden, techo a rayas, cartel con bombillas y escaleras.
    const float ex = -9.0f;
    const float ez = -18.4f;
    float ey = AlturaSueloFeria(ex, ez);

    m.Caja(Vector3{ ex, ey + 0.5f, ez }, Vector3{ 5.6f, 1.0f, 1.5f }, MADERA_FERIA);
    m.Caja(Vector3{ ex, ey + 1.02f, ez }, Vector3{ 5.7f, 0.06f, 1.6f }, MADERA_FERIA_OSCURA);

    for (int k = 0; k < 3; k++)
    {
        m.Caja(
            Vector3{ ex + 1.6f, ey + 0.17f + 0.33f * k, ez + 1.25f - 0.28f * k },
            Vector3{ 1.2f, 0.33f, 0.3f },
            MADERA_FERIA_OSCURA
        );
    }

    const float postesX[2] = { ex - 2.6f, ex + 2.6f };
    const float postesZ[2] = { ez + 0.6f, ez - 1.9f };

    for (float px : postesX)
    {
        for (float pz : postesZ)
        {
            m.Cilindro(Vector3{ px, ey, pz }, Vector3{ px, ey + 3.3f, pz }, 0.08f, 0.08f, 5, ACERO_FERIA);
        }
    }

    for (int r = 0; r < 8; r++)
    {
        float x0 = ex - 3.0f + 6.0f * r / 8.0f;

        m.Caja(
            Vector3{ x0 + 0.375f, ey + 3.38f, ez - 0.65f },
            Vector3{ 0.75f, 0.12f, 3.2f },
            r % 2 == 0 ? Color{ 214, 54, 70, 255 } : CREMA_FERIA
        );
    }

    m.Caja(Vector3{ ex, ey + 3.95f, ez + 0.75f }, Vector3{ 3.6f, 0.8f, 0.12f }, MAGENTA_FERIA);
    m.Caja(Vector3{ ex, ey + 3.95f, ez + 0.79f }, Vector3{ 3.2f, 0.5f, 0.1f }, VIOLETA_FERIA);

    for (int k = 0; k < 14; k++)
    {
        float u = (float)k / 13.0f;

        BombillaHorneadaFeria(
            m, Vector3{ ex - 1.75f + 3.5f * u, ey + 4.38f, ez + 0.82f }, 0.06f,
            COLORES_BOMBILLA[k % CANTIDAD_COLORES_BOMBILLA]
        );

        BombillaHorneadaFeria(
            m, Vector3{ ex - 1.75f + 3.5f * u, ey + 3.52f, ez + 0.82f }, 0.06f,
            COLORES_BOMBILLA[(k + 2) % CANTIDAD_COLORES_BOMBILLA]
        );
    }

    // Valla de la fila.
    PiezaVallaFeria(m, Vector3{ ex - 2.8f, ey, ez + 1.1f }, Vector3{ ex + 0.9f, ey, ez + 1.1f }, Color{ 150, 110, 90, 255 });
}


//==================================================
// LANDMARKS (una malla horneada, construida una vez)
//==================================================

static MallaGpu mallaLandmarksFeria;


// Soporte de la Gran Rueda: tarima, patas en A, mastil del puntero y arco
// de bombillas detras.
static void ConstruirBaseGranRuedaFeria(ConstructorMalla& m)
{
    m.Caja(Vector3{ RUEDA_X, 0.16f, RUEDA_Z - 0.35f }, Vector3{ 5.0f, 0.32f, 1.8f }, MADERA_FERIA);
    m.Caja(Vector3{ RUEDA_X, 0.34f, RUEDA_Z + 0.53f }, Vector3{ 5.0f, 0.06f, 0.10f }, DORADO_FERIA);

    Vector3 eje = { RUEDA_X, RUEDA_ALTO_EJE, RUEDA_Z - 0.32f };

    m.Cilindro(Vector3{ RUEDA_X - 1.8f, 0.30f, RUEDA_Z - 0.32f }, eje, 0.14f, 0.10f, 8, VIOLETA_FERIA);
    m.Cilindro(Vector3{ RUEDA_X + 1.8f, 0.30f, RUEDA_Z - 0.32f }, eje, 0.14f, 0.10f, 8, VIOLETA_FERIA);
    m.Cilindro(Vector3{ RUEDA_X, 0.30f, RUEDA_Z - 1.05f }, eje, 0.10f, 0.08f, 8, VIOLETA_FERIA);

    // Mastil del puntero (de su punta salen guirnaldas).
    float altoMastil = RUEDA_ALTO_EJE + RUEDA_RADIO + 0.85f;

    m.Cilindro(
        Vector3{ RUEDA_X, 0.30f, RUEDA_Z - 0.65f },
        Vector3{ RUEDA_X, altoMastil, RUEDA_Z - 0.65f },
        0.09f, 0.07f, 8,
        MADERA_FERIA_OSCURA
    );

    m.Caja(Vector3{ RUEDA_X, altoMastil - 0.2f, RUEDA_Z - 0.25f }, Vector3{ 0.16f, 0.16f, 0.8f }, MADERA_FERIA_OSCURA);

    // Puntero rojo que apunta al gajo ganador.
    m.Cilindro(
        Vector3{ RUEDA_X, altoMastil - 0.13f, RUEDA_Z + 0.22f },
        Vector3{ RUEDA_X, RUEDA_ALTO_EJE + RUEDA_RADIO - 0.12f, RUEDA_Z + 0.22f },
        0.24f, 0.02f, 8,
        Color{ 226, 48, 58, 255 }
    );

    m.Esfera(Vector3{ RUEDA_X, altoMastil + 0.07f, RUEDA_Z - 0.65f }, 0.15f, 6, 8, DORADO_FERIA);

    // Arco de bombillas detras de la rueda.
    const int TRAMOS = 18;
    const float radioArco = RUEDA_RADIO + 1.0f;
    const float zArco = RUEDA_Z - 1.0f;

    for (int k = 0; k < TRAMOS; k++)
    {
        float a0 = PI * k / TRAMOS;
        float a1 = PI * (k + 1) / TRAMOS;

        Vector3 p0 = { RUEDA_X + std::cos(a0) * radioArco, RUEDA_ALTO_EJE + std::sin(a0) * radioArco, zArco };
        Vector3 p1 = { RUEDA_X + std::cos(a1) * radioArco, RUEDA_ALTO_EJE + std::sin(a1) * radioArco, zArco };

        m.Cilindro(p0, p1, 0.09f, 0.09f, 5, MAGENTA_FERIA);
        BombillaHorneadaFeria(m, Vector3{ p0.x, p0.y, p0.z + 0.1f }, 0.075f, COLORES_BOMBILLA[k % CANTIDAD_COLORES_BOMBILLA]);
    }

    for (int lado = -1; lado <= 1; lado += 2)
    {
        float x = RUEDA_X + radioArco * lado;

        m.Cilindro(Vector3{ x, 0.0f, zArco }, Vector3{ x, RUEDA_ALTO_EJE, zArco }, 0.1f, 0.09f, 6, MAGENTA_FERIA);

        for (int k = 0; k < 5; k++)
        {
            BombillaHorneadaFeria(
                m, Vector3{ x, 0.5f + 0.6f * k, zArco + 0.1f }, 0.07f,
                COLORES_BOMBILLA[(k + lado + 4) % CANTIDAD_COLORES_BOMBILLA]
            );
        }
    }
}


// Pedestal del frasco y aro de la boca (el vidrio y las monedas son del gimmick).
static void ConstruirBaseFrascoFeria(ConstructorMalla& m)
{
    m.Cilindro(Vector3{ FRASCO_X, 0.0f, FRASCO_Z }, Vector3{ FRASCO_X, 0.34f, FRASCO_Z }, 1.28f, 1.16f, 20, MADERA_FERIA);
    m.Cilindro(Vector3{ FRASCO_X, 0.34f, FRASCO_Z }, Vector3{ FRASCO_X, FRASCO_BASE, FRASCO_Z }, FRASCO_RADIO + 0.07f, FRASCO_RADIO + 0.05f, 20, DORADO_FERIA);

    const int SEGMENTOS = 18;
    float y = FRASCO_BASE + FRASCO_ALTO;

    for (int i = 0; i < SEGMENTOS; i++)
    {
        float a0 = 2.0f * PI * i / SEGMENTOS;
        float a1 = 2.0f * PI * (i + 1) / SEGMENTOS;

        m.Cilindro(
            Vector3{ FRASCO_X + std::cos(a0) * FRASCO_RADIO, y, FRASCO_Z + std::sin(a0) * FRASCO_RADIO },
            Vector3{ FRASCO_X + std::cos(a1) * FRASCO_RADIO, y, FRASCO_Z + std::sin(a1) * FRASCO_RADIO },
            0.07f, 0.07f, 5,
            DORADO_FERIA
        );

        if (i % 2 == 0)
        {
            BombillaHorneadaFeria(
                m,
                Vector3{ FRASCO_X + std::cos(a0) * 1.22f, 0.36f, FRASCO_Z + std::sin(a0) * 1.22f },
                0.05f, LUZ_CALIDA_FERIA
            );
        }
    }
}


// Carpa mayor del circo, detras de la Gran Rueda.
static void ConstruirCarpaMayorFeria(ConstructorMalla& m)
{
    const float x = CARPA_MAYOR_X;
    const float z = CARPA_MAYOR_Z;
    const float radio = 4.0f;
    const float alto = 7.4f;

    PiezaCarpaFeria(m, x, 0.0f, z, radio, alto, Color{ 206, 48, 60, 255 }, Color{ 238, 226, 200, 255 }, true);

    // Tunel de la entrada con cartel y bombillas.
    m.Caja(Vector3{ x, 1.0f, z + radio + 0.4f }, Vector3{ 2.2f, 2.0f, 1.2f }, Color{ 206, 48, 60, 255 });
    m.Caja(Vector3{ x, 0.85f, z + radio + 1.02f }, Vector3{ 1.4f, 1.7f, 0.05f }, EmisivoFeria(Color{ 150, 80, 50, 255 }));
    m.Caja(Vector3{ x, 2.45f, z + radio + 1.0f }, Vector3{ 2.8f, 0.8f, 0.14f }, DORADO_FERIA);
    m.Caja(Vector3{ x, 2.45f, z + radio + 1.06f }, Vector3{ 2.5f, 0.5f, 0.1f }, VIOLETA_FERIA);

    for (int k = 0; k < 12; k++)
    {
        BombillaHorneadaFeria(
            m, Vector3{ x - 1.35f + 2.7f * k / 11.0f, 2.9f, z + radio + 1.08f }, 0.06f,
            COLORES_BOMBILLA[k % CANTIDAD_COLORES_BOMBILLA]
        );
    }

    // Tiras de bombillas de la punta a las estacas y vientos.
    const int TIRAS = 10;

    for (int t = 0; t < TIRAS; t++)
    {
        float a = 2.0f * PI * (t + 0.5f) / TIRAS;
        Vector3 punta = { x, alto + 0.5f, z };
        Vector3 estaca = { x + std::cos(a) * (radio + 2.2f), 0.0f, z + std::sin(a) * (radio + 2.2f) };
        Vector3 alero = { x + std::cos(a) * radio * 1.14f, alto * 0.42f, z + std::sin(a) * radio * 1.14f };

        m.Cilindro(alero, estaca, 0.015f, 0.015f, 3, Color{ 200, 190, 170, 255 });
        m.Cilindro(estaca, Vector3{ estaca.x, 0.3f, estaca.z }, 0.05f, 0.04f, 4, MADERA_FERIA_OSCURA);

        for (int k = 1; k < 9; k++)
        {
            float u = k / 9.0f;
            Vector3 p = SumarFeria(punta, Vector3{ alero.x - punta.x, alero.y - punta.y, alero.z - punta.z }, u);

            BombillaHorneadaFeria(m, Vector3{ p.x, p.y + 0.05f, p.z }, 0.06f, COLORES_BOMBILLA[(k + t) % CANTIDAD_COLORES_BOMBILLA]);
        }
    }

    // Banderines de colores en un mastil doble.
    for (int lado = -1; lado <= 1; lado += 2)
    {
        float bx = x + 1.4f * lado;

        m.Cilindro(Vector3{ bx, alto * 0.75f, z }, Vector3{ bx, alto + 0.9f, z }, 0.05f, 0.04f, 5, MADERA_FERIA_OSCURA);
        m.Caja(Vector3{ bx + 0.25f * lado, alto + 0.7f, z }, Vector3{ 0.5f, 0.28f, 0.02f }, lado < 0 ? DORADO_FERIA : CELESTE_FERIA);
    }
}


// Patas en A, eje, anden y boleteria de la noria (la rueda gira aparte).
static void ConstruirBaseNoriaFeria(ConstructorMalla& m)
{
    float suelo = AlturaSueloFeria(NORIA_X, NORIA_Z);

    for (int lado = -1; lado <= 1; lado += 2)
    {
        float zEje = NORIA_Z + 0.95f * lado;
        Vector3 eje = { NORIA_X, NORIA_ALTO_EJE, zEje };

        for (int pata = -1; pata <= 1; pata += 2)
        {
            Vector3 pie = { NORIA_X + 3.1f * pata, suelo, NORIA_Z + 1.5f * lado };

            m.Cilindro(pie, eje, 0.16f, 0.11f, 7, Color{ 236, 236, 246, 255 });

            for (int k = 1; k < 9; k++)
            {
                Vector3 p = SumarFeria(pie, Vector3{ eje.x - pie.x, eje.y - pie.y, eje.z - pie.z }, k / 9.0f);
                BombillaHorneadaFeria(m, Vector3{ p.x, p.y, p.z + 0.14f * lado }, 0.06f, COLORES_BOMBILLA[(k + pata + 2) % CANTIDAD_COLORES_BOMBILLA]);
            }
        }

        // Travesaño.
        m.Cilindro(
            Vector3{ NORIA_X - 1.55f, NORIA_ALTO_EJE * 0.5f, NORIA_Z + 1.22f * lado },
            Vector3{ NORIA_X + 1.55f, NORIA_ALTO_EJE * 0.5f, NORIA_Z + 1.22f * lado },
            0.07f, 0.07f, 5, Color{ 236, 236, 246, 255 }
        );
    }

    m.Cilindro(
        Vector3{ NORIA_X, NORIA_ALTO_EJE, NORIA_Z - 1.1f },
        Vector3{ NORIA_X, NORIA_ALTO_EJE, NORIA_Z + 1.1f },
        0.2f, 0.2f, 10, DORADO_FERIA
    );

    // Anden con escalera hacia la camara.
    m.Caja(Vector3{ NORIA_X, suelo + 0.2f, NORIA_Z }, Vector3{ 2.6f, 0.4f, 2.2f }, MADERA_FERIA);
    m.Caja(Vector3{ NORIA_X, suelo + 0.1f, NORIA_Z + 1.35f }, Vector3{ 1.4f, 0.2f, 0.5f }, MADERA_FERIA_OSCURA);

    // Boleteria al pie.
    float bx = NORIA_X + 2.2f;
    float bz = NORIA_Z + 2.0f;

    m.Caja(Vector3{ bx, suelo + 0.6f, bz }, Vector3{ 0.9f, 1.2f, 0.7f }, MAGENTA_FERIA);
    m.Caja(Vector3{ bx, suelo + 0.85f, bz + 0.36f }, Vector3{ 0.6f, 0.4f, 0.03f }, EmisivoFeria(Color{ 255, 206, 130, 255 }));
    m.Caja(Vector3{ bx, suelo + 1.28f, bz }, Vector3{ 1.1f, 0.12f, 0.9f }, CREMA_FERIA);

    // Valla alrededor.
    Color valla = Color{ 150, 110, 90, 255 };
    float vx0 = NORIA_X - 3.4f;
    float vx1 = NORIA_X + 3.4f;
    float vz0 = NORIA_Z - 2.2f;
    float vz1 = NORIA_Z + 2.4f;

    PiezaVallaFeria(m, Vector3{ vx0, suelo, vz1 }, Vector3{ NORIA_X - 1.0f, suelo, vz1 }, valla);
    PiezaVallaFeria(m, Vector3{ NORIA_X + 1.0f, suelo, vz1 }, Vector3{ bx - 0.6f, suelo, vz1 }, valla);
    PiezaVallaFeria(m, Vector3{ vx0, suelo, vz0 }, Vector3{ vx0, suelo, vz1 }, valla);
    PiezaVallaFeria(m, Vector3{ vx1, suelo, vz0 }, Vector3{ vx1, suelo, vz1 }, valla);
}


// Base fija del carrusel: tarima con escalon y valla redonda (la parte que
// gira y los caballitos son mallas aparte).
static void ConstruirBaseCarruselFeria(ConstructorMalla& m)
{
    m.Cilindro(Vector3{ CARRUSEL_X, 0.0f, CARRUSEL_Z }, Vector3{ CARRUSEL_X, 0.16f, CARRUSEL_Z }, CARRUSEL_RADIO + 0.45f, CARRUSEL_RADIO + 0.4f, 24, MADERA_FERIA_OSCURA);
    m.Cilindro(Vector3{ CARRUSEL_X, 0.16f, CARRUSEL_Z }, Vector3{ CARRUSEL_X, 0.32f, CARRUSEL_Z }, CARRUSEL_RADIO + 0.2f, CARRUSEL_RADIO + 0.18f, 24, VIOLETA_FERIA);

    for (int k = 0; k < 24; k++)
    {
        float a = 2.0f * PI * k / 24.0f;

        BombillaHorneadaFeria(
            m,
            Vector3{ CARRUSEL_X + std::cos(a) * (CARRUSEL_RADIO + 0.21f), 0.24f, CARRUSEL_Z + std::sin(a) * (CARRUSEL_RADIO + 0.21f) },
            0.045f, LUZ_CALIDA_FERIA
        );
    }
}


// Portal de la entrada: postes a rayas, arco con cartel y bombillas.
static void ConstruirPortalFeria(ConstructorMalla& m)
{
    const float SEPARACION = 2.15f;
    const int TRAMOS = 8;
    const float ALTO = 3.4f;

    for (int lado = -1; lado <= 1; lado += 2)
    {
        float x = PORTAL_X + SEPARACION * lado;

        for (int i = 0; i < TRAMOS; i++)
        {
            m.Cilindro(
                Vector3{ x, ALTO * i / TRAMOS, PORTAL_Z },
                Vector3{ x, ALTO * (i + 1) / TRAMOS, PORTAL_Z },
                0.19f, 0.19f, 10,
                i % 2 == 0 ? Color{ 224, 52, 70, 255 } : CREMA_FERIA
            );
        }

        m.Esfera(Vector3{ x, ALTO + 0.15f, PORTAL_Z }, 0.24f, 6, 8, DORADO_FERIA);
    }

    // Arco: tablero curvo de un poste al otro.
    const int SEGMENTOS = 12;

    for (int k = 0; k < SEGMENTOS; k++)
    {
        float u0 = (float)k / SEGMENTOS;
        float u1 = (float)(k + 1) / SEGMENTOS;

        float x0 = PORTAL_X - SEPARACION + 2.0f * SEPARACION * u0;
        float x1 = PORTAL_X - SEPARACION + 2.0f * SEPARACION * u1;
        float y0 = ALTO - 0.2f + 1.1f * std::sin(PI * u0);
        float y1 = ALTO - 0.2f + 1.1f * std::sin(PI * u1);

        Vector3 a0 = { x0, y0 - 0.35f, PORTAL_Z };
        Vector3 a1 = { x1, y1 - 0.35f, PORTAL_Z };
        Vector3 b0 = { x0, y0 + 0.35f, PORTAL_Z };
        Vector3 b1 = { x1, y1 + 0.35f, PORTAL_Z };

        m.TrianguloDobleCara(a0, a1, b1, MAGENTA_FERIA);
        m.TrianguloDobleCara(a0, b1, b0, MAGENTA_FERIA);

        Vector3 c0 = { x0, y0 - 0.2f, PORTAL_Z + 0.03f };
        Vector3 c1 = { x1, y1 - 0.2f, PORTAL_Z + 0.03f };
        Vector3 d0 = { x0, y0 + 0.2f, PORTAL_Z + 0.03f };
        Vector3 d1 = { x1, y1 + 0.2f, PORTAL_Z + 0.03f };

        m.TrianguloDobleCara(c0, c1, d1, VIOLETA_FERIA);
        m.TrianguloDobleCara(c0, d1, d0, VIOLETA_FERIA);

        BombillaHorneadaFeria(m, Vector3{ x0, y0 + 0.42f, PORTAL_Z + 0.06f }, 0.07f, COLORES_BOMBILLA[k % CANTIDAD_COLORES_BOMBILLA]);
        BombillaHorneadaFeria(m, Vector3{ x0, y0 - 0.42f, PORTAL_Z + 0.06f }, 0.06f, LUZ_CALIDA_FERIA);
    }

    // Estrella dorada en lo alto.
    m.Esfera(Vector3{ PORTAL_X, ALTO + 1.45f, PORTAL_Z }, 0.3f, 4, 8, EmisivoFeria(Color{ 255, 220, 110, 255 }));

    // Taquilla de la entrada.
    const float tx = -21.4f;
    const float tz = 9.8f;
    float ty = AlturaSueloFeria(tx, tz);

    CajaGiradaFeria(m, tx, ty + 0.8f, tz, 90.0f, Vector3{ 1.4f, 1.6f, 1.2f }, Color{ 224, 52, 70, 255 });
    CajaGiradaFeria(m, tx + 0.62f, ty + 1.1f, tz, 90.0f, Vector3{ 0.8f, 0.5f, 0.03f }, EmisivoFeria(Color{ 255, 206, 130, 255 }));
    m.Cilindro(Vector3{ tx, ty + 1.6f, tz }, Vector3{ tx, ty + 2.4f, tz }, 1.05f, 0.0f, 8, CREMA_FERIA);
    m.Esfera(Vector3{ tx, ty + 2.45f, tz }, 0.12f, 4, 6, DORADO_FERIA);
}


// Tiro al blanco al este de la noria, mirando al sendero (-x).
static void ConstruirTiroAlBlancoFeria(ConstructorMalla& m)
{
    const float giro = -90.0f;
    const float x = TIRO_X;
    const float z = TIRO_Z;
    float y = AlturaSueloFeria(x, z);

    Vector3 fondo = LocalFeria(x, y + 1.2f, z, giro, 0.0f, 0.0f, -0.6f);
    CajaGiradaFeria(m, fondo.x, fondo.y, fondo.z, giro, Vector3{ 3.2f, 2.4f, 0.15f }, MADERA_FERIA_OSCURA);

    Vector3 mostrador = LocalFeria(x, y + 0.45f, z, giro, 0.0f, 0.0f, 0.5f);
    CajaGiradaFeria(m, mostrador.x, mostrador.y, mostrador.z, giro, Vector3{ 3.2f, 0.9f, 0.5f }, ROJO_FERIA);

    Vector3 tabla = LocalFeria(x, y + 0.93f, z, giro, 0.0f, 0.0f, 0.5f);
    CajaGiradaFeria(m, tabla.x, tabla.y, tabla.z, giro, Vector3{ 3.3f, 0.08f, 0.62f }, CREMA_FERIA);

    for (int lado = -1; lado <= 1; lado += 2)
    {
        Vector3 pared = LocalFeria(x, y + 1.2f, z, giro, 1.6f * lado, 0.0f, 0.0f);
        CajaGiradaFeria(m, pared.x, pared.y, pared.z, giro, Vector3{ 0.12f, 2.4f, 1.3f }, ROJO_FERIA);
    }

    // Techo a rayas y cartel.
    for (int r = 0; r < 8; r++)
    {
        Vector3 raya = LocalFeria(x, y + 2.5f, z, giro, -1.6f + 0.4f * r + 0.2f, 0.0f, 0.0f);
        CajaGiradaFeria(m, raya.x, raya.y, raya.z, giro, Vector3{ 0.4f, 0.12f, 1.5f }, r % 2 == 0 ? ROJO_FERIA : CREMA_FERIA);
    }

    Vector3 cartel = LocalFeria(x, y + 3.0f, z, giro, 0.0f, 0.0f, 0.6f);
    CajaGiradaFeria(m, cartel.x, cartel.y, cartel.z, giro, Vector3{ 2.4f, 0.7f, 0.1f }, DORADO_FERIA);

    for (int k = 0; k < 10; k++)
    {
        Vector3 b = LocalFeria(x, y + 3.38f, z, giro, -1.1f + 2.2f * k / 9.0f, 0.0f, 0.62f);
        BombillaHorneadaFeria(m, b, 0.055f, COLORES_BOMBILLA[k % CANTIDAD_COLORES_BOMBILLA]);
    }

    // Dianas en el fondo (dos filas) y patitos en una repisa.
    for (int fila = 0; fila < 2; fila++)
    {
        for (int k = 0; k < 4; k++)
        {
            Vector3 c = LocalFeria(x, y + 1.25f + 0.75f * fila, z, giro, -1.05f + 0.7f * k, 0.0f, -0.5f);
            Vector3 d = LocalFeria(x, y + 1.25f + 0.75f * fila, z, giro, -1.05f + 0.7f * k, 0.0f, -0.46f);
            Vector3 e = LocalFeria(x, y + 1.25f + 0.75f * fila, z, giro, -1.05f + 0.7f * k, 0.0f, -0.43f);
            Vector3 f = LocalFeria(x, y + 1.25f + 0.75f * fila, z, giro, -1.05f + 0.7f * k, 0.0f, -0.40f);

            m.Cilindro(c, d, 0.26f, 0.26f, 12, Color{ 220, 50, 60, 255 });
            m.Cilindro(d, e, 0.17f, 0.17f, 12, CREMA_FERIA);
            m.Cilindro(e, f, 0.08f, 0.08f, 8, Color{ 220, 50, 60, 255 });
        }
    }

    for (int k = 0; k < 6; k++)
    {
        Vector3 p = LocalFeria(x, y + 1.05f, z, giro, -1.25f + 0.5f * k, 0.0f, -0.25f);
        m.Esfera(p, 0.1f, 3, 6, Color{ 250, 214, 60, 255 });
        m.Esfera(Vector3{ p.x, p.y + 0.12f, p.z }, 0.065f, 3, 5, Color{ 250, 214, 60, 255 });
    }
}


static void ConstruirCarpasLazoFeria(ConstructorMalla& m)
{
    static const Color RAYAS[CANTIDAD_CARPAS_FERIA][2] =
    {
        { { 60, 150, 190, 255 }, { 236, 222, 196, 255 } },
        { { 206, 62, 176, 255 }, { 236, 222, 196, 255 } },   // el mago
        { { 236, 184, 66, 255 }, { 92, 54, 150, 255 } },
        { { 70, 170, 110, 255 }, { 236, 222, 196, 255 } },
        { { 212, 70, 64, 255 }, { 236, 222, 196, 255 } }
    };

    for (int i = 0; i < CANTIDAD_CARPAS_FERIA; i++)
    {
        const float* c = CARPAS_FERIA[i];

        PiezaCarpaFeria(
            m, c[0], AlturaSueloFeria(c[0], c[1]), c[1],
            c[2], c[3], RAYAS[i][0], RAYAS[i][1], true
        );
    }

    // Bola de cristal del mago (frente a su carpa).
    float y = AlturaSueloFeria(-18.1f, 2.1f);

    m.Cilindro(Vector3{ -18.1f, y, 2.1f }, Vector3{ -18.1f, y + 0.7f, 2.1f }, 0.12f, 0.18f, 6, MADERA_FERIA_OSCURA);
    m.Esfera(Vector3{ -18.1f, y + 0.92f, 2.1f }, 0.24f, 5, 8, EmisivoFeria(Color{ 170, 140, 255, 255 }));

    // Taquilla de la montaña rusa, dentro de su lobulo.
    float tx = TAQUILLA_RUSA_X;
    float tz = TAQUILLA_RUSA_Z;
    float ty = AlturaSueloFeria(tx, tz);

    m.Caja(Vector3{ tx, ty + 0.75f, tz }, Vector3{ 1.2f, 1.5f, 1.0f }, Color{ 214, 54, 70, 255 });
    m.Caja(Vector3{ tx, ty + 1.05f, tz + 0.51f }, Vector3{ 0.7f, 0.45f, 0.03f }, EmisivoFeria(Color{ 255, 206, 130, 255 }));
    m.Caja(Vector3{ tx, ty + 1.58f, tz }, Vector3{ 1.4f, 0.14f, 1.2f }, CREMA_FERIA);
    m.Caja(Vector3{ tx, ty + 1.95f, tz }, Vector3{ 1.1f, 0.5f, 0.08f }, DORADO_FERIA);

    for (int k = 0; k < 5; k++)
    {
        BombillaHorneadaFeria(m, Vector3{ tx - 0.45f + 0.225f * k, ty + 2.25f, tz + 0.05f }, 0.05f, COLORES_BOMBILLA[k % CANTIDAD_COLORES_BOMBILLA]);
    }
}


static void ConstruirPuestosFeria(ConstructorMalla& m)
{
    for (int i = 0; i < CANTIDAD_PUESTOS_FERIA; i++)
    {
        const float* p = PUESTOS_FERIA[i];
        PiezaPuestoFeria(m, p[0], p[1], p[2], i);
    }

    // Casas rodantes de los feriantes, al fondo.
    PiezaRemolqueFeria(m, 7.8f, -17.6f, 15.0f, Color{ 120, 170, 190, 255 });
    PiezaRemolqueFeria(m, 3.8f, -20.2f, -10.0f, Color{ 220, 170, 110, 255 });

    // Carrito de globos junto al carrusel.
    const float gx = 4.3f;
    const float gz = 8.6f;

    m.Caja(Vector3{ gx, 0.55f, gz }, Vector3{ 1.0f, 0.5f, 0.6f }, CELESTE_FERIA);
    m.Cilindro(Vector3{ gx - 0.35f, 0.25f, gz + 0.32f }, Vector3{ gx - 0.35f, 0.25f, gz + 0.38f }, 0.25f, 0.25f, 8, Color{ 40, 36, 44, 255 });
    m.Cilindro(Vector3{ gx + 0.35f, 0.25f, gz + 0.32f }, Vector3{ gx + 0.35f, 0.25f, gz + 0.38f }, 0.25f, 0.25f, 8, Color{ 40, 36, 44, 255 });
    PiezaGlobosFeria(m, gx, 0.8f, gz, 0.3f);

    // Mesas del patio de comidas con sombrillas.
    const float mesas[4][2] = { { 13.6f, 16.2f }, { 17.4f, 15.8f }, { 21.2f, 14.2f }, { 9.6f, 16.0f } };

    for (int k = 0; k < 4; k++)
    {
        float x = mesas[k][0];
        float z = mesas[k][1];
        float y = AlturaSueloFeria(x, z);

        m.Cilindro(Vector3{ x, y + 0.7f, z }, Vector3{ x, y + 0.76f, z }, 0.55f, 0.55f, 10, CREMA_FERIA);
        m.Cilindro(Vector3{ x, y, z }, Vector3{ x, y + 2.2f, z }, 0.04f, 0.04f, 4, Color{ 60, 56, 64, 255 });

        for (int g = 0; g < 8; g++)
        {
            float a0 = 2.0f * PI * g / 8.0f;
            float a1 = 2.0f * PI * (g + 1) / 8.0f;

            m.TrianguloDobleCara(
                Vector3{ x, y + 2.3f, z },
                Vector3{ x + std::cos(a0) * 1.0f, y + 1.85f, z + std::sin(a0) * 1.0f },
                Vector3{ x + std::cos(a1) * 1.0f, y + 1.85f, z + std::sin(a1) * 1.0f },
                g % 2 == 0 ? COLORES_BOMBILLA[k % CANTIDAD_COLORES_BOMBILLA] : CREMA_FERIA
            );
        }

        for (int s = 0; s < 3; s++)
        {
            float a = s * 2.1f + k;
            PiezaBarrilFeria(m, x + std::cos(a) * 0.95f, y, z + std::sin(a) * 0.95f, Color{ 120, 84, 60, 255 });
        }
    }
}


// Un cartel por bifurcacion: tabla de madera = opcion 0 (largo), rojiza =
// atajo, apuntando hacia cada destino.
static void ConstruirCartelesFeria(ConstructorMalla& m)
{
    const Tablero& t = TableroReferenciaFeria();

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

        // El primer lugar libre (fuera del sendero y de los landmarks); si
        // ninguno sirve (cruces apretados), el mas alejado del sendero en
        // un anillo alrededor de la casilla.
        float px = c.posicion.x + lugares[0][0];
        float pz = c.posicion.z + lugares[0][1];
        bool encontrado = false;

        for (const auto& l : lugares)
        {
            float x = c.posicion.x + l[0];
            float z = c.posicion.z + l[1];

            if (DistanciaSenderoFeria(x, z) >= 2.0f && DespejadoFeria(x, z, 0.2f))
            {
                px = x;
                pz = z;
                encontrado = true;
                break;
            }
        }

        float mejor = -1.0f;

        for (int r = 0; r < 3 && !encontrado; r++)
        {
            for (int k = 0; k < 16; k++)
            {
                float a = 2.0f * PI * k / 16.0f;
                float x = c.posicion.x + std::cos(a) * (2.4f + 0.45f * r);
                float z = c.posicion.z + std::sin(a) * (2.4f + 0.45f * r);
                float d = DistanciaSenderoFeria(x, z);

                if (DespejadoFeria(x, z, 0.2f) && !OcultaCasillaFeria(x, z, 0.0f) && d > mejor)
                {
                    mejor = d;
                    px = x;
                    pz = z;
                }
            }

            encontrado = mejor >= 1.9f;
        }

        float y = AlturaSueloFeria(px, pz);

        m.Cilindro(Vector3{ px, y, pz }, Vector3{ px, y + 1.9f, pz }, 0.08f, 0.07f, 5, MADERA_FERIA_OSCURA);
        BombillaHorneadaFeria(m, Vector3{ px, y + 1.98f, pz }, 0.08f, LUZ_CALIDA_FERIA);

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

            CajaGiradaFeria(m, px + ux * 0.55f, ty, pz + uz * 0.55f, giro * RAD2DEG, Vector3{ 1.1f, 0.22f, 0.07f }, tabla);
            CajaGiradaFeria(m, px + ux * 1.15f, ty, pz + uz * 1.15f, giro * RAD2DEG, Vector3{ 0.2f, 0.12f, 0.07f }, tabla);
        }
    }
}


//==================================================
// GUIRNALDAS Y FAROLES (colocados una vez a lo largo del sendero)
//==================================================
//
// Un poste por tramo de sendero, del lado de atras (o del que haya lugar),
// fuera de la franja frontal de las casillas; las guirnaldas unen los
// postes de tramos consecutivos, cruzan el camino de entrada y bajan desde
// el mastil de la Gran Rueda. Postes, cables y bombillas apagadas van en
// una malla; las encendidas, en cuatro mallas de fase que se alternan
// (luces que corren) con una sola llamada por frame.

static const int MAX_ANCLAS_FERIA = 160;
static const int MAX_GUIRNALDAS_FERIA = 200;
static const int FASES_GUIRNALDA = 4;
static const float ALTO_POSTE_FERIA = 3.0f;

struct AnclaFeria
{
    Vector3 punta;
    bool poste;
};

static AnclaFeria anclasFeria[MAX_ANCLAS_FERIA];
static int cantidadAnclasFeria = 0;

// Ancla de cada tramo (indice de conexion origen * 2 + opcion), -1 = sin poste.
static int anclaTramoFeria[MAX_CASILLAS_TABLERO][2];

static int guirnaldasFeria[MAX_GUIRNALDAS_FERIA][2];
static int cantidadGuirnaldasFeria = 0;

static const int MAX_FAROLES_FERIA = 60;

static Vector3 farolesFeria[MAX_FAROLES_FERIA];
static int cantidadFarolesFeria = 0;

static bool escenaFeriaPreparada = false;


static bool LibreDeAnclasFeria(float x, float z, float separacion)
{
    for (int i = 0; i < cantidadAnclasFeria; i++)
    {
        float dx = anclasFeria[i].punta.x - x;
        float dz = anclasFeria[i].punta.z - z;

        if (dx * dx + dz * dz < separacion * separacion)
        {
            return false;
        }
    }

    for (int i = 0; i < cantidadFarolesFeria; i++)
    {
        float dx = farolesFeria[i].x - x;
        float dz = farolesFeria[i].z - z;

        if (dx * dx + dz * dz < separacion * separacion)
        {
            return false;
        }
    }

    return true;
}


static bool LugarDePosteFeria(float x, float z)
{
    return
        DistanciaSenderoFeria(x, z) >= 2.15f &&
        DespejadoFeria(x, z, 0.3f) &&
        DistanciaPistaRusaFeria(x, z) > 1.5f &&
        !OcultaCasillaFeria(x, z, 0.1f) &&
        LibreDeAnclasFeria(x, z, 1.8f);
}


static int AgregarAnclaFeria(Vector3 punta, bool poste)
{
    if (cantidadAnclasFeria >= MAX_ANCLAS_FERIA)
    {
        return -1;
    }

    anclasFeria[cantidadAnclasFeria] = AnclaFeria{ punta, poste };
    return cantidadAnclasFeria++;
}


static void AgregarGuirnaldaFeria(int a, int b)
{
    if (a < 0 || b < 0 || a == b || cantidadGuirnaldasFeria >= MAX_GUIRNALDAS_FERIA)
    {
        return;
    }

    float dx = anclasFeria[a].punta.x - anclasFeria[b].punta.x;
    float dz = anclasFeria[a].punta.z - anclasFeria[b].punta.z;
    float d = std::sqrt(dx * dx + dz * dz);

    if (d < 1.5f || d > 9.0f)
    {
        return;
    }

    guirnaldasFeria[cantidadGuirnaldasFeria][0] = a;
    guirnaldasFeria[cantidadGuirnaldasFeria][1] = b;
    cantidadGuirnaldasFeria++;
}


// Coloca postes, guirnaldas y faroles, y los charcos de luz de todo.
static void PrepararEscenaFeria()
{
    if (escenaFeriaPreparada)
    {
        return;
    }

    escenaFeriaPreparada = true;

    const Tablero& t = TableroReferenciaFeria();

    PrepararPistaRusaFeria();

    cantidadAnclasFeria = 0;
    cantidadGuirnaldasFeria = 0;
    cantidadFarolesFeria = 0;
    cantidadLucesSueloFeria = 0;

    // --- Faroles: junto a las casillas, del lado de atras ---
    for (int i = 0; i < t.cantidadCasillas && cantidadFarolesFeria < MAX_FAROLES_FERIA; i++)
    {
        const Casilla& c = t.casillas[i];

        if (c.cantidadConexiones == 0)
        {
            continue;
        }

        const Casilla* siguiente = t.ObtenerCasilla(c.conexiones[0].destino);

        if (siguiente == nullptr)
        {
            continue;
        }

        float dx = siguiente->posicion.x - c.posicion.x;
        float dz = siguiente->posicion.z - c.posicion.z;
        float largo = std::sqrt(dx * dx + dz * dz) + 0.0001f;
        float nx = -dz / largo;
        float nz = dx / largo;

        // Lado de atras primero; si no hay lugar, un poco mas lejos o
        // corrido a lo largo del sendero.
        float ladoAtras = nz < 0.0f ? 1.0f : -1.0f;
        const float distancias[3] = { 2.5f, 2.9f, 3.3f };
        const float corrimientos[3] = { 0.0f, 0.8f, -0.8f };
        bool puesto = false;

        for (int intento = 0; intento < 2 && !puesto; intento++)
        {
            float lado = intento == 0 ? ladoAtras : -ladoAtras;

            for (int d = 0; d < 3 && !puesto; d++)
            {
                for (int u = 0; u < 3 && !puesto; u++)
                {
                    float x = c.posicion.x + nx * distancias[d] * lado + dx / largo * corrimientos[u];
                    float z = c.posicion.z + nz * distancias[d] * lado + dz / largo * corrimientos[u];

                    bool lejosDeFaroles = true;

                    for (int f = 0; f < cantidadFarolesFeria; f++)
                    {
                        float ex = farolesFeria[f].x - x;
                        float ez = farolesFeria[f].z - z;

                        if (ex * ex + ez * ez < 6.5f * 6.5f)
                        {
                            lejosDeFaroles = false;
                        }
                    }

                    if (!lejosDeFaroles)
                    {
                        // Ya hay un farol cerca de esta casilla.
                        puesto = true;
                    }
                    else if (LugarDePosteFeria(x, z))
                    {
                        farolesFeria[cantidadFarolesFeria++] = Vector3{ x, AlturaSueloFeria(x, z), z };
                        puesto = true;
                    }
                }
            }
        }
    }

    // --- Postes de guirnalda: uno por tramo ---
    for (int i = 0; i < t.cantidadCasillas; i++)
    {
        anclaTramoFeria[i][0] = -1;
        anclaTramoFeria[i][1] = -1;

        const Casilla& a = t.casillas[i];

        for (int k = 0; k < a.cantidadConexiones; k++)
        {
            const Casilla* b = t.ObtenerCasilla(a.conexiones[k].destino);

            if (b == nullptr)
            {
                continue;
            }

            float mx = (a.posicion.x + b->posicion.x) * 0.5f;
            float mz = (a.posicion.z + b->posicion.z) * 0.5f;
            float dx = b->posicion.x - a.posicion.x;
            float dz = b->posicion.z - a.posicion.z;
            float largo = std::sqrt(dx * dx + dz * dz) + 0.0001f;
            float nx = -dz / largo;
            float nz = dx / largo;

            // Primero el lado mas despejado (a igualdad, el de atras).
            float da = DistanciaSenderoFeria(mx + nx * 4.0f, mz + nz * 4.0f);
            float db = DistanciaSenderoFeria(mx - nx * 4.0f, mz - nz * 4.0f);
            float ladoPreferido = (da > db + 0.5f || (std::fabs(da - db) <= 0.5f && nz < 0.0f)) ? 1.0f : -1.0f;
            const float distancias[3] = { 2.45f, 2.85f, 3.3f };
            const float avances[3] = { 0.0f, -0.18f, 0.18f };
            bool puesto = false;

            for (int intento = 0; intento < 2 && !puesto; intento++)
            {
                float lado = intento == 0 ? ladoPreferido : -ladoPreferido;

                for (int d = 0; d < 3 && !puesto; d++)
                {
                    for (int u = 0; u < 3 && !puesto; u++)
                    {
                        float x = mx + nx * distancias[d] * lado + dx * avances[u];
                        float z = mz + nz * distancias[d] * lado + dz * avances[u];

                        if (LugarDePosteFeria(x, z))
                        {
                            float y = AlturaSueloFeria(x, z);
                            anclaTramoFeria[i][k] = AgregarAnclaFeria(Vector3{ x, y + ALTO_POSTE_FERIA, z }, true);
                            puesto = true;
                        }
                    }
                }
            }
        }
    }

    // Guirnaldas entre tramos consecutivos (a -> b -> c).
    for (int i = 0; i < t.cantidadCasillas; i++)
    {
        const Casilla& a = t.casillas[i];

        for (int k = 0; k < a.cantidadConexiones; k++)
        {
            int b = a.conexiones[k].destino;

            if (b < 0 || b >= t.cantidadCasillas)
            {
                continue;
            }

            for (int k2 = 0; k2 < t.casillas[b].cantidadConexiones; k2++)
            {
                AgregarGuirnaldaFeria(anclaTramoFeria[i][k], anclaTramoFeria[b][k2]);
            }
        }
    }

    // Mastil de la Gran Rueda: guirnaldas a los postes cercanos.
    int mastil =
        AgregarAnclaFeria(
            Vector3{ RUEDA_X, RUEDA_ALTO_EJE + RUEDA_RADIO + 0.85f, RUEDA_Z - 0.65f },
            false
        );

    for (int i = 0; i + 1 < cantidadAnclasFeria; i++)
    {
        float dx = anclasFeria[i].punta.x - RUEDA_X;
        float dz = anclasFeria[i].punta.z - RUEDA_Z;

        if (dx * dx + dz * dz < 8.5f * 8.5f)
        {
            AgregarGuirnaldaFeria(mastil, i);
        }
    }

    // Camino de entrada: postes a los dos lados y guirnaldas en zigzag
    // que cruzan el camino, desde el portal hacia el sur.
    const Vector3& inicio = t.casillas[nodosFeria.inicio].posicion;
    int anterior[2] =
    {
        AgregarAnclaFeria(Vector3{ PORTAL_X - 2.15f, 3.55f, PORTAL_Z }, false),
        AgregarAnclaFeria(Vector3{ PORTAL_X + 2.15f, 3.55f, PORTAL_Z }, false)
    };

    for (float z = PORTAL_Z + 3.4f; z < ENTRADA_SUR_Z - 1.0f; z += 3.4f)
    {
        int par[2];

        for (int lado = 0; lado < 2; lado++)
        {
            float x = inicio.x + (lado == 0 ? -2.5f : 2.5f);
            par[lado] = AgregarAnclaFeria(Vector3{ x, AlturaSueloFeria(x, z) + ALTO_POSTE_FERIA, z }, true);

            AgregarGuirnaldaFeria(anterior[lado], par[lado]);
        }

        AgregarGuirnaldaFeria(anterior[0], par[1]);
        anterior[0] = par[0];
        anterior[1] = par[1];
    }

    // --- Charcos de luz ---
    for (int i = 0; i < cantidadFarolesFeria; i++)
    {
        AgregarLuzSueloFeria(farolesFeria[i].x, farolesFeria[i].z, 3.8f, Color{ 255, 190, 110, 255 }, 0.30f);
    }

    for (int i = 0; i < cantidadAnclasFeria; i++)
    {
        if (anclasFeria[i].poste)
        {
            AgregarLuzSueloFeria(anclasFeria[i].punta.x, anclasFeria[i].punta.z, 2.4f, COLORES_BOMBILLA[i % CANTIDAD_COLORES_BOMBILLA], 0.10f);
        }
    }

    AgregarLuzSueloFeria(RUEDA_X, RUEDA_Z + 1.0f, 5.5f, Color{ 255, 200, 120, 255 }, 0.32f);
    AgregarLuzSueloFeria(FRASCO_X, FRASCO_Z, 3.2f, Color{ 255, 210, 90, 255 }, 0.30f);
    AgregarLuzSueloFeria(CARPA_MAYOR_X, CARPA_MAYOR_Z + 5.0f, 5.0f, Color{ 255, 170, 110, 255 }, 0.32f);
    AgregarLuzSueloFeria(CARRUSEL_X, CARRUSEL_Z, 5.6f, Color{ 255, 170, 220, 255 }, 0.30f);
    AgregarLuzSueloFeria(NORIA_X, NORIA_Z + 1.0f, 6.0f, Color{ 190, 130, 255, 255 }, 0.25f);
    AgregarLuzSueloFeria(PORTAL_X, PORTAL_Z, 4.4f, Color{ 255, 180, 120, 255 }, 0.34f);
    AgregarLuzSueloFeria(TIRO_X - 1.2f, TIRO_Z, 4.0f, Color{ 255, 160, 120, 255 }, 0.30f);
    AgregarLuzSueloFeria(-9.0f, -17.4f, 4.6f, Color{ 255, 120, 150, 255 }, 0.26f);
    AgregarLuzSueloFeria(-18.1f, 2.1f, 3.0f, Color{ 170, 140, 255, 255 }, 0.30f);
    AgregarLuzSueloFeria(TAQUILLA_RUSA_X, TAQUILLA_RUSA_Z + 1.0f, 2.6f, Color{ 255, 190, 120, 255 }, 0.26f);

    for (int i = 0; i < CANTIDAD_PUESTOS_FERIA; i++)
    {
        const float* p = PUESTOS_FERIA[i];
        Vector3 frente = LocalFeria(p[0], 0.0f, p[1], p[2], 0.0f, 0.0f, 1.2f);

        AgregarLuzSueloFeria(frente.x, frente.z, 3.4f, Color{ 255, 196, 120, 255 }, 0.32f);
    }

    for (int i = 0; i < CANTIDAD_CARPAS_FERIA; i++)
    {
        const float* c = CARPAS_FERIA[i];
        AgregarLuzSueloFeria(c[0], c[1] + c[2] + 0.4f, 2.4f, Color{ 255, 180, 110, 255 }, 0.24f);
    }
}


static MallaGpu mallaGuirnaldasFeria;
static MallaGpu mallasFaseGuirnaldaFeria[FASES_GUIRNALDA];


// Punto de una guirnalda con la comba de una catenaria simple.
static Vector3 PuntoGuirnaldaFeria(int g, float u)
{
    Vector3 a = anclasFeria[guirnaldasFeria[g][0]].punta;
    Vector3 b = anclasFeria[guirnaldasFeria[g][1]].punta;

    float dx = b.x - a.x;
    float dz = b.z - a.z;
    float largo = std::sqrt(dx * dx + dz * dz);
    float comba = largo * 0.10f;

    if (comba < 0.25f) comba = 0.25f;
    if (comba > 0.75f) comba = 0.75f;

    return Vector3
    {
        a.x + dx * u,
        a.y + (b.y - a.y) * u - comba * 4.0f * u * (1.0f - u),
        a.z + dz * u
    };
}


static void ConstruirMallasGuirnaldasFeria()
{
    PrepararEscenaFeria();

    static ConstructorMalla base;
    static ConstructorMalla fases[FASES_GUIRNALDA];

    base.Limpiar();

    for (int f = 0; f < FASES_GUIRNALDA; f++)
    {
        fases[f].Limpiar();
    }

    // Postes con remate dorado.
    for (int i = 0; i < cantidadAnclasFeria; i++)
    {
        if (!anclasFeria[i].poste)
        {
            continue;
        }

        Vector3 p = anclasFeria[i].punta;
        float suelo = p.y - ALTO_POSTE_FERIA;

        base.Cilindro(Vector3{ p.x, suelo, p.z }, Vector3{ p.x, p.y + 0.1f, p.z }, 0.08f, 0.06f, 6, MADERA_FERIA_OSCURA);
        base.Esfera(Vector3{ p.x, p.y + 0.15f, p.z }, 0.1f, 4, 6, DORADO_FERIA);
    }

    // Faroles.
    for (int i = 0; i < cantidadFarolesFeria; i++)
    {
        PiezaFarolFeria(base, farolesFeria[i].x, farolesFeria[i].y, farolesFeria[i].z);
    }

    int indiceBombilla = 0;
    const int TRAMOS_CABLE = 8;

    for (int g = 0; g < cantidadGuirnaldasFeria; g++)
    {
        Vector3 anterior = PuntoGuirnaldaFeria(g, 0.0f);

        for (int k = 1; k <= TRAMOS_CABLE; k++)
        {
            Vector3 actual = PuntoGuirnaldaFeria(g, (float)k / TRAMOS_CABLE);
            base.Cilindro(anterior, actual, 0.014f, 0.014f, 3, Color{ 26, 20, 30, 255 });
            anterior = actual;
        }

        Vector3 a = anclasFeria[guirnaldasFeria[g][0]].punta;
        Vector3 b = anclasFeria[guirnaldasFeria[g][1]].punta;
        float largo = std::sqrt((b.x - a.x) * (b.x - a.x) + (b.z - a.z) * (b.z - a.z));
        int bombillas = (int)(largo / 0.55f);

        if (bombillas < 4)
        {
            bombillas = 4;
        }

        for (int k = 0; k < bombillas; k++)
        {
            Vector3 p = PuntoGuirnaldaFeria(g, (k + 0.5f) / bombillas);
            p.y -= 0.08f;

            Color color = COLORES_BOMBILLA[indiceBombilla % CANTIDAD_COLORES_BOMBILLA];

            base.Esfera(p, 0.075f, 2, 4, EmisivoFeria(MezclaFeria(color, Color{ 30, 20, 40, 255 }, 0.62f)));
            fases[indiceBombilla % FASES_GUIRNALDA].Esfera(p, 0.092f, 2, 5, color);

            indiceBombilla++;
        }
    }

    IluminarMallaFeria(base, 0);

    ActualizarMallaGpu(mallaGuirnaldasFeria, base);

    for (int f = 0; f < FASES_GUIRNALDA; f++)
    {
        ActualizarMallaGpu(mallasFaseGuirnaldaFeria[f], fases[f]);
    }
}


static void ConstruirMallaLandmarksFeria()
{
    PrepararEscenaFeria();

    static ConstructorMalla m;
    m.Limpiar();

    ConstruirBaseGranRuedaFeria(m);
    ConstruirBaseFrascoFeria(m);
    ConstruirCarpaMayorFeria(m);
    ConstruirBaseNoriaFeria(m);
    ConstruirBaseCarruselFeria(m);
    ConstruirMontanaRusaFeria(m);
    ConstruirPortalFeria(m);
    ConstruirTiroAlBlancoFeria(m);
    ConstruirCarpasLazoFeria(m);
    ConstruirPuestosFeria(m);
    ConstruirCartelesFeria(m);

    IluminarMallaFeria(m, 0);

    ActualizarMallaGpu(mallaLandmarksFeria, m);
}


static void DibujarLandmarksFeria(float tiempo)
{
    if (!mallaLandmarksFeria.cargada)
    {
        ConstruirMallaLandmarksFeria();
    }

    if (!mallaGuirnaldasFeria.cargada)
    {
        ConstruirMallasGuirnaldasFeria();
    }

    DibujarMallaGpu(mallaLandmarksFeria);
    DibujarMallaGpu(mallaGuirnaldasFeria);

    // Luces que corren: dos fases encendidas a la vez.
    int fase = (int)(tiempo * 5.0f) % FASES_GUIRNALDA;

    DibujarMallaGpu(mallasFaseGuirnaldaFeria[fase]);
    DibujarMallaGpu(mallasFaseGuirnaldaFeria[(fase + 1) % FASES_GUIRNALDA]);
}


//==================================================
// DECORACION DISPERSA (colocada una vez, horneada por calidad)
//==================================================
//
// ~1000 piezas en ALTA, cerca del sendero (diorama). Cada tipo tiene su
// regla: distancia al sendero, separacion y, si es alto, fuera de la franja
// frontal de las casillas. Nada bajo la montaña rusa ni en los claros de
// los landmarks. Los pinos suben a las colinas del borde.

enum TipoPropFeria
{
    PROP_FERIA_PINO = 0,
    PROP_FERIA_ARBOL,
    PROP_FERIA_ARBOL_LUCES,
    PROP_FERIA_ARBUSTO,
    PROP_FERIA_PASTO,
    PROP_FERIA_FLORES,
    PROP_FERIA_ROCA,
    PROP_FERIA_FARDO,
    PROP_FERIA_BARRIL,
    PROP_FERIA_CAJON,
    PROP_FERIA_GLOBOS,
    PROP_FERIA_BANCO,
    PROP_FERIA_TACHO
};

struct PropFeria
{
    TipoPropFeria tipo;
    float x;
    float y;
    float z;
    float escala;
    float variacion;
    float giro;
};

struct ReglaPropFeria
{
    TipoPropFeria tipo;
    int cantidad;
    float senderoMin;
    float senderoMax;
    float separacion;
    bool alto;
    bool llano;        // solo en suelo bajo (no en las colinas)
    float escalaMin;
    float escalaMax;
};

static const ReglaPropFeria REGLAS_PROPS_FERIA[] =
{
    { PROP_FERIA_ARBOL_LUCES,  44, 2.8f,  9.0f, 3.2f, true,  true,  0.85f, 1.2f },
    { PROP_FERIA_PINO,        110, 3.0f, 22.0f, 2.0f, true,  false, 0.9f,  1.5f },
    { PROP_FERIA_ARBOL,       120, 2.8f, 17.0f, 1.9f, true,  false, 0.8f,  1.3f },
    { PROP_FERIA_GLOBOS,       16, 2.4f,  6.0f, 5.0f, true,  true,  1.0f,  1.0f },
    { PROP_FERIA_BANCO,        18, 2.2f,  3.0f, 4.0f, false, true,  1.0f,  1.0f },
    { PROP_FERIA_TACHO,        16, 2.2f,  3.4f, 5.0f, false, true,  1.0f,  1.0f },
    { PROP_FERIA_FARDO,        28, 2.4f,  9.0f, 2.0f, false, true,  1.0f,  1.0f },
    { PROP_FERIA_BARRIL,       30, 2.2f,  8.0f, 1.6f, false, true,  1.0f,  1.0f },
    { PROP_FERIA_CAJON,        30, 2.2f,  8.0f, 1.6f, false, true,  0.4f,  0.65f },
    { PROP_FERIA_ROCA,         60, 2.1f, 18.0f, 1.3f, false, false, 0.3f,  0.7f },
    { PROP_FERIA_ARBUSTO,     210, 2.1f, 14.0f, 0.9f, false, false, 0.7f,  1.25f },
    { PROP_FERIA_FLORES,      130, 2.0f, 10.0f, 0.7f, false, true,  0.8f,  1.2f },
    { PROP_FERIA_PASTO,       280, 2.0f, 14.0f, 0.55f, false, false, 0.7f, 1.3f }
};

static const int MAX_PROPS_FERIA = 1300;

static PropFeria propsFeria[MAX_PROPS_FERIA];
static int cantidadPropsFeria = 0;
static bool propsFeriaColocados = false;

static unsigned int semillaPropsFeria = 13579u;


static float AleatorioFeria()
{
    semillaPropsFeria = semillaPropsFeria * 1664525u + 1013904223u;
    return (float)((semillaPropsFeria >> 8) & 0xFFFF) / 65535.0f;
}


static bool SeparadoDeSuTipoFeria(
    TipoPropFeria tipo,
    float x, float z,
    float separacion
)
{
    for (int i = 0; i < cantidadPropsFeria; i++)
    {
        const PropFeria& p = propsFeria[i];

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


static void ColocarPropsFeria()
{
    PrepararEscenaFeria();

    semillaPropsFeria = 13579u;
    cantidadPropsFeria = 0;

    const float RADIO_UTIL = TELON_RADIO - 3.5f;

    for (const ReglaPropFeria& regla : REGLAS_PROPS_FERIA)
    {
        for (int intento = 0, n = 0; intento < 40000 && n < regla.cantidad; intento++)
        {
            float x = CENTRO_CASILLAS_X - RADIO_UTIL + AleatorioFeria() * RADIO_UTIL * 2.0f;
            float z = CENTRO_CASILLAS_Z - RADIO_UTIL + AleatorioFeria() * RADIO_UTIL * 2.0f;

            float ex = x - CENTRO_CASILLAS_X;
            float ez = z - CENTRO_CASILLAS_Z;

            if (ex * ex + ez * ez > RADIO_UTIL * RADIO_UTIL)
            {
                continue;
            }

            float h = AlturaSueloFeria(x, z);

            if (
                (regla.llano && h > 0.9f) ||
                !DespejadoFeria(x, z, regla.alto ? 1.0f : 0.4f) ||
                DistanciaPistaRusaFeria(x, z) < (regla.alto ? 2.2f : 1.2f) ||
                (regla.alto && OcultaCasillaFeria(x, z, 0.4f)) ||
                !LibreDeAnclasFeria(x, z, regla.alto ? 1.4f : 0.6f)
            )
            {
                continue;
            }

            float sendero = DistanciaSenderoFeria(x, z);

            if (sendero < regla.senderoMin || sendero > regla.senderoMax)
            {
                continue;
            }

            if (!SeparadoDeSuTipoFeria(regla.tipo, x, z, regla.separacion))
            {
                continue;
            }

            if (cantidadPropsFeria >= MAX_PROPS_FERIA)
            {
                break;
            }

            PropFeria& p = propsFeria[cantidadPropsFeria++];

            p.tipo = regla.tipo;
            p.x = x;
            p.y = h - 0.03f;
            p.z = z;
            p.escala = regla.escalaMin + AleatorioFeria() * (regla.escalaMax - regla.escalaMin);
            p.variacion = AleatorioFeria();
            p.giro = AleatorioFeria() * 360.0f;

            // Los bancos miran al sendero.
            if (regla.tipo == PROP_FERIA_BANCO)
            {
                float gx = DistanciaSenderoFeria(x + 0.3f, z) - DistanciaSenderoFeria(x - 0.3f, z);
                float gz = DistanciaSenderoFeria(x, z + 0.3f) - DistanciaSenderoFeria(x, z - 0.3f);

                p.giro = std::atan2(-gx, -gz) * RAD2DEG;
            }

            n++;
        }

    }

    propsFeriaColocados = true;
}


// Calidad de decoracion: ALTA conserva todo; MEDIA recorta parte de lo
// menudo; BAJA deja solo lo cercano al sendero y poco de lo menudo.
static bool ConservarPropFeria(
    int indice,
    const PropFeria& p,
    NivelCalidadGrafica nivel
)
{
    if (nivel == CALIDAD_ALTA)
    {
        return true;
    }

    bool menudo =
        p.tipo == PROP_FERIA_PASTO ||
        p.tipo == PROP_FERIA_FLORES ||
        p.tipo == PROP_FERIA_ARBUSTO ||
        p.tipo == PROP_FERIA_ROCA ||
        p.tipo == PROP_FERIA_CAJON ||
        p.tipo == PROP_FERIA_BARRIL;

    float azar = HashFeria(indice, 77);

    if (nivel == CALIDAD_MEDIA)
    {
        return !menudo || azar < FactorCalidadGrafica(nivel);
    }

    if (DistanciaSenderoFeria(p.x, p.z) > 11.0f)
    {
        return false;
    }

    return !menudo || azar < FactorCalidadGrafica(nivel) * 0.8f;
}


static MallaGpu mallaPropsFeria;
static int nivelPropsFeria = -1;


static void ConstruirMallaPropsFeria(NivelCalidadGrafica nivel)
{
    if (!propsFeriaColocados)
    {
        ColocarPropsFeria();
    }

    static ConstructorMalla m;
    m.Limpiar();
    m.reduccionDetalle = nivel == CALIDAD_ALTA ? 0 : 1;

    for (int i = 0; i < cantidadPropsFeria; i++)
    {
        const PropFeria& p = propsFeria[i];

        if (!ConservarPropFeria(i, p, nivel))
        {
            continue;
        }

        switch (p.tipo)
        {
            case PROP_FERIA_PINO:
                PiezaPinoFeria(m, p.x, p.y, p.z, p.escala, p.variacion);
                break;

            case PROP_FERIA_ARBOL:
                PiezaArbolFeria(m, p.x, p.y, p.z, p.escala, p.variacion, false);
                break;

            case PROP_FERIA_ARBOL_LUCES:
                PiezaArbolFeria(m, p.x, p.y, p.z, p.escala, p.variacion, true);
                break;

            case PROP_FERIA_ARBUSTO:
                PiezaArbustoFeria(m, p.x, p.y, p.z, p.escala, p.variacion);
                break;

            case PROP_FERIA_PASTO:
                PiezaPastoFeria(m, p.x, p.y, p.z, p.escala, p.variacion);
                break;

            case PROP_FERIA_FLORES:
                PiezaFloresFeria(m, p.x, p.y, p.z, p.escala, p.variacion);
                break;

            case PROP_FERIA_ROCA:
                PiezaRocaFeria(
                    m, p.x, p.y, p.z, p.escala, p.giro,
                    MezclaFeria(Color{ 70, 72, 84, 255 }, Color{ 96, 96, 108, 255 }, p.variacion)
                );
                break;

            case PROP_FERIA_FARDO:
                PiezaFardoFeria(m, p.x, p.y, p.z, p.giro);
                break;

            case PROP_FERIA_BARRIL:
                PiezaBarrilFeria(
                    m, p.x, p.y, p.z,
                    p.variacion < 0.5f ? Color{ 120, 84, 60, 255 } : Color{ 170, 60, 60, 255 }
                );
                break;

            case PROP_FERIA_CAJON:
                PiezaCajonFeria(m, p.x, p.y, p.z, p.giro, p.escala);
                break;

            case PROP_FERIA_GLOBOS:
                PiezaGlobosFeria(m, p.x, p.y, p.z, p.variacion);
                break;

            case PROP_FERIA_BANCO:
                PiezaBancoFeria(m, p.x, p.y, p.z, p.giro);
                break;

            case PROP_FERIA_TACHO:
                PiezaTachoFeria(m, p.x, p.y, p.z);
                break;
        }
    }

    // Luz de luna y charcos de luz de faroles y puestos.
    int total = m.CantidadVertices();

    IluminarMallaFeria(m, 0);

    for (int v = 0; v + 2 < total; v += 3)
    {
        float cx = m.posiciones[v * 3];
        float cz = m.posiciones[v * 3 + 2];

        Color base = { m.colores[v * 4], m.colores[v * 4 + 1], m.colores[v * 4 + 2], 255 };
        Color tenido = TenirConLucesFeria(base, cx, cz);

        for (int k = 0; k < 3; k++)
        {
            m.colores[(v + k) * 4] = tenido.r;
            m.colores[(v + k) * 4 + 1] = tenido.g;
            m.colores[(v + k) * 4 + 2] = tenido.b;
        }
    }

    ActualizarMallaGpu(mallaPropsFeria, m);
    nivelPropsFeria = (int)nivel;
}


static void DibujarPropsFeria()
{
    NivelCalidadGrafica nivel = CalidadDecoracion();

    if (!mallaPropsFeria.cargada || nivelPropsFeria != (int)nivel)
    {
        ConstruirMallaPropsFeria(nivel);
    }

    DibujarMallaGpu(mallaPropsFeria);
}


//==================================================
// ATRACCIONES QUE GIRAN (mallas horneadas bajo una transformacion)
//==================================================
//
// La rueda de la noria y la parte giratoria del carrusel se hornean una vez
// en coordenadas locales y se dibujan girando con rlRotatef: una llamada
// por pieza en lugar de cientos de primitivas inmediatas.

static MallaGpu mallaRuedaNoriaFeria;
static MallaGpu mallaRotorCarruselFeria;
static MallaGpu mallasCaballosFeria[2];

static const float RADIO_CABALLOS = 1.75f;
static const int CABALLOS_CARRUSEL = 8;


// Rueda de la noria con el eje en el origen (gira alrededor de z).
static void ConstruirRuedaNoriaFeria()
{
    static ConstructorMalla m;
    m.Limpiar();

    const Color blanco = Color{ 236, 236, 246, 255 };
    const int SEGMENTOS = 36;
    const float caras[2] = { -0.42f, 0.42f };

    for (float dz : caras)
    {
        for (int i = 0; i < SEGMENTOS; i++)
        {
            float a0 = 2.0f * PI * i / SEGMENTOS;
            float a1 = 2.0f * PI * (i + 1) / SEGMENTOS;

            m.Cilindro(
                Vector3{ std::cos(a0) * NORIA_RADIO, std::sin(a0) * NORIA_RADIO, dz },
                Vector3{ std::cos(a1) * NORIA_RADIO, std::sin(a1) * NORIA_RADIO, dz },
                0.07f, 0.07f, 4, blanco
            );

            m.Cilindro(
                Vector3{ std::cos(a0) * NORIA_RADIO * 0.55f, std::sin(a0) * NORIA_RADIO * 0.55f, dz },
                Vector3{ std::cos(a1) * NORIA_RADIO * 0.55f, std::sin(a1) * NORIA_RADIO * 0.55f, dz },
                0.04f, 0.04f, 3, MAGENTA_FERIA
            );
        }

        for (int k = 0; k < CABINAS_NORIA; k++)
        {
            float a = 2.0f * PI * k / CABINAS_NORIA;

            m.Cilindro(
                Vector3{ std::cos(a) * 0.3f, std::sin(a) * 0.3f, dz },
                Vector3{ std::cos(a) * NORIA_RADIO, std::sin(a) * NORIA_RADIO, dz },
                0.045f, 0.04f, 4, blanco
            );
        }
    }

    // Travesaños de las cabinas y cubo central.
    for (int k = 0; k < CABINAS_NORIA; k++)
    {
        float a = 2.0f * PI * k / CABINAS_NORIA;

        m.Cilindro(
            Vector3{ std::cos(a) * NORIA_RADIO, std::sin(a) * NORIA_RADIO, -0.45f },
            Vector3{ std::cos(a) * NORIA_RADIO, std::sin(a) * NORIA_RADIO, 0.45f },
            0.045f, 0.045f, 4, DORADO_FERIA
        );
    }

    m.Cilindro(Vector3{ 0.0f, 0.0f, -0.55f }, Vector3{ 0.0f, 0.0f, 0.55f }, 0.36f, 0.36f, 12, DORADO_FERIA);

    // Bombillas del aro delantero y de los rayos.
    for (int i = 0; i < SEGMENTOS; i++)
    {
        float a = 2.0f * PI * (i + 0.5f) / SEGMENTOS;

        BombillaHorneadaFeria(
            m, Vector3{ std::cos(a) * NORIA_RADIO, std::sin(a) * NORIA_RADIO, 0.52f }, 0.075f,
            COLORES_BOMBILLA[i % CANTIDAD_COLORES_BOMBILLA]
        );
    }

    for (int k = 0; k < CABINAS_NORIA; k++)
    {
        float a = 2.0f * PI * k / CABINAS_NORIA;

        for (int b = 1; b <= 4; b++)
        {
            float r = NORIA_RADIO * (0.15f + 0.2f * b);

            BombillaHorneadaFeria(
                m, Vector3{ std::cos(a) * r, std::sin(a) * r, 0.5f }, 0.055f,
                k % 2 == 0 ? LUZ_CALIDA_FERIA : Color{ 120, 220, 255, 255 }
            );
        }
    }

    IluminarMallaFeria(m, 0);
    ActualizarMallaGpu(mallaRuedaNoriaFeria, m);
}


// Un caballito con el centro del cuerpo en (x, y, z) mirando "giro".
static void PiezaCaballitoFeria(
    ConstructorMalla& m,
    float x, float y, float z,
    float giro,
    int variante
)
{
    static const Color PELAJES[4] =
    {
        { 236, 230, 220, 255 },
        { 150, 92, 62, 255 },
        { 60, 52, 62, 255 },
        { 186, 186, 196, 255 }
    };

    static const Color MONTURAS[4] =
    {
        { 214, 54, 70, 255 },
        { 236, 184, 66, 255 },
        { 76, 176, 214, 255 },
        { 206, 62, 176, 255 }
    };

    Color pelaje = PELAJES[variante % 4];
    Color crin = variante % 4 == 2 ? Color{ 236, 226, 200, 255 } : Color{ 70, 50, 44, 255 };

    TransformacionMalla t;
    t.tx = x;
    t.ty = y;
    t.tz = z;
    t.giroY = giro;
    t.sx = 1.75f;
    t.sz = 0.8f;

    m.EstablecerTransformacion(t);
    m.Esfera(Vector3{ 0.0f, 0.0f, 0.0f }, 0.2f, 4, 7, pelaje);
    m.QuitarTransformacion();

    // Cuello, cabeza y crin.
    m.Cilindro(LocalFeria(x, y, z, giro, 0.22f, 0.06f, 0.0f), LocalFeria(x, y, z, giro, 0.4f, 0.36f, 0.0f), 0.09f, 0.07f, 5, pelaje);

    t.tx = LocalFeria(x, y, z, giro, 0.5f, 0.38f, 0.0f).x;
    t.ty = y + 0.38f;
    t.tz = LocalFeria(x, y, z, giro, 0.5f, 0.38f, 0.0f).z;
    t.sx = 1.6f;
    t.sz = 0.75f;

    m.EstablecerTransformacion(t);
    m.Esfera(Vector3{ 0.0f, 0.0f, 0.0f }, 0.09f, 3, 6, pelaje);
    m.QuitarTransformacion();

    m.Cilindro(LocalFeria(x, y, z, giro, 0.2f, 0.16f, 0.0f), LocalFeria(x, y, z, giro, 0.4f, 0.46f, 0.0f), 0.04f, 0.03f, 4, crin);

    // Patas al galope y cola.
    for (int lado = -1; lado <= 1; lado += 2)
    {
        m.Cilindro(LocalFeria(x, y, z, giro, 0.2f, -0.08f, 0.08f * lado), LocalFeria(x, y, z, giro, 0.38f, -0.42f, 0.08f * lado), 0.035f, 0.03f, 4, pelaje);
        m.Cilindro(LocalFeria(x, y, z, giro, -0.2f, -0.08f, 0.08f * lado), LocalFeria(x, y, z, giro, -0.4f, -0.38f, 0.08f * lado), 0.035f, 0.03f, 4, pelaje);
    }

    m.Cilindro(LocalFeria(x, y, z, giro, -0.32f, 0.04f, 0.0f), LocalFeria(x, y, z, giro, -0.48f, -0.22f, 0.0f), 0.05f, 0.02f, 4, crin);

    // Montura.
    Vector3 montura = LocalFeria(x, y + 0.17f, z, giro, -0.02f, 0.0f, 0.0f);
    CajaGiradaFeria(m, montura.x, montura.y, montura.z, giro, Vector3{ 0.26f, 0.07f, 0.3f }, MONTURAS[variante % 4]);
}


// Parte giratoria del carrusel con el centro en el origen (a nivel del suelo).
static void ConstruirRotorCarruselFeria()
{
    static ConstructorMalla m;
    m.Limpiar();

    const int GAJOS = 16;
    const float radioPiso = CARRUSEL_RADIO - 0.1f;
    const float radioTecho = CARRUSEL_RADIO + 0.3f;
    const float altoTecho = 3.3f;
    const float punta = 4.6f;

    m.Cilindro(Vector3{ 0.0f, 0.32f, 0.0f }, Vector3{ 0.0f, 0.42f, 0.0f }, radioPiso, radioPiso, GAJOS, DORADO_FERIA);

    for (int i = 0; i < GAJOS; i++)
    {
        float a0 = 2.0f * PI * i / GAJOS;
        float a1 = 2.0f * PI * (i + 1) / GAJOS;
        float medio = (a0 + a1) * 0.5f;

        Color piso = i % 2 == 0 ? Color{ 230, 214, 186, 255 } : Color{ 160, 54, 70, 255 };

        m.TrianguloDobleCara(
            Vector3{ 0.0f, 0.43f, 0.0f },
            Vector3{ std::cos(a0) * radioPiso, 0.43f, std::sin(a0) * radioPiso },
            Vector3{ std::cos(a1) * radioPiso, 0.43f, std::sin(a1) * radioPiso },
            piso
        );

        // Techo conico a rayas.
        Color gajo = i % 2 == 0 ? MAGENTA_FERIA : CREMA_FERIA;

        m.TrianguloDobleCara(
            Vector3{ 0.0f, punta, 0.0f },
            Vector3{ std::cos(a0) * radioTecho, altoTecho, std::sin(a0) * radioTecho },
            Vector3{ std::cos(a1) * radioTecho, altoTecho, std::sin(a1) * radioTecho },
            gajo
        );

        m.TrianguloDobleCara(
            Vector3{ 0.0f, altoTecho - 0.02f, 0.0f },
            Vector3{ std::cos(a0) * radioTecho, altoTecho - 0.02f, std::sin(a0) * radioTecho },
            Vector3{ std::cos(a1) * radioTecho, altoTecho - 0.02f, std::sin(a1) * radioTecho },
            VIOLETA_FERIA
        );

        // Faldon con festones.
        Vector3 f0 = { std::cos(a0) * radioTecho, altoTecho, std::sin(a0) * radioTecho };
        Vector3 f1 = { std::cos(a1) * radioTecho, altoTecho, std::sin(a1) * radioTecho };
        Vector3 colgante = { std::cos(medio) * radioTecho, altoTecho - 0.42f, std::sin(medio) * radioTecho };

        m.TrianguloDobleCara(f0, f1, colgante, i % 2 == 0 ? DORADO_FERIA : MAGENTA_FERIA);

        BombillaHorneadaFeria(m, Vector3{ f0.x, f0.y + 0.05f, f0.z }, 0.07f, COLORES_BOMBILLA[i % CANTIDAD_COLORES_BOMBILLA]);
        BombillaHorneadaFeria(m, Vector3{ colgante.x, colgante.y - 0.03f, colgante.z }, 0.055f, LUZ_CALIDA_FERIA);
    }

    // Columna central con espejos iluminados.
    m.Cilindro(Vector3{ 0.0f, 0.42f, 0.0f }, Vector3{ 0.0f, altoTecho, 0.0f }, 0.45f, 0.45f, 12, DORADO_FERIA);

    for (int k = 0; k < 8; k++)
    {
        float a = 2.0f * PI * (k + 0.5f) / 8.0f;

        CajaGiradaFeria(
            m, std::cos(a) * 0.47f, 1.9f, std::sin(a) * 0.47f, -a * RAD2DEG,
            Vector3{ 0.03f, 1.8f, 0.24f },
            EmisivoFeria(k % 2 == 0 ? Color{ 255, 226, 170, 255 } : Color{ 200, 170, 255, 255 })
        );
    }

    // Barras de los caballitos.
    for (int k = 0; k < CABALLOS_CARRUSEL; k++)
    {
        float a = 2.0f * PI * k / CABALLOS_CARRUSEL;
        float x = std::cos(a) * RADIO_CABALLOS;
        float z = std::sin(a) * RADIO_CABALLOS;

        m.Cilindro(Vector3{ x, 0.42f, z }, Vector3{ x, altoTecho, z }, 0.035f, 0.035f, 5, DORADO_FERIA);
    }

    // Remate con banderin.
    m.Esfera(Vector3{ 0.0f, punta + 0.08f, 0.0f }, 0.16f, 4, 6, DORADO_FERIA);
    m.Cilindro(Vector3{ 0.0f, punta, 0.0f }, Vector3{ 0.0f, punta + 0.7f, 0.0f }, 0.025f, 0.025f, 4, MADERA_FERIA_OSCURA);
    m.Caja(Vector3{ 0.18f, punta + 0.6f, 0.0f }, Vector3{ 0.32f, 0.18f, 0.02f }, CELESTE_FERIA);

    IluminarMallaFeria(m, 0);
    ActualizarMallaGpu(mallaRotorCarruselFeria, m);

    // Caballitos en dos mallas (pares e impares suben y bajan a contratiempo).
    for (int grupo = 0; grupo < 2; grupo++)
    {
        static ConstructorMalla c;
        c.Limpiar();

        for (int k = grupo; k < CABALLOS_CARRUSEL; k += 2)
        {
            float a = 2.0f * PI * k / CABALLOS_CARRUSEL;
            float x = std::cos(a) * RADIO_CABALLOS;
            float z = std::sin(a) * RADIO_CABALLOS;

            // Mira hacia donde avanza al girar.
            float giro = std::atan2(std::cos(a), std::sin(a)) * RAD2DEG;

            PiezaCaballitoFeria(c, x, 1.35f, z, giro, k);
        }

        IluminarMallaFeria(c, 0);
        ActualizarMallaGpu(mallasCaballosFeria[grupo], c);
    }
}


static void DibujarNoriaFeria(float tiempo)
{
    if (!mallaRuedaNoriaFeria.cargada)
    {
        ConstruirRuedaNoriaFeria();
    }

    float giro = tiempo * 0.14f;

    rlPushMatrix();
    rlTranslatef(NORIA_X, NORIA_ALTO_EJE, NORIA_Z);
    rlRotatef(giro * RAD2DEG, 0.0f, 0.0f, 1.0f);
    DibujarMallaGpu(mallaRuedaNoriaFeria);
    rlPopMatrix();

    // Cabinas: cuelgan siempre derechas.
    for (int i = 0; i < CABINAS_NORIA; i++)
    {
        float a = giro + 2.0f * PI * i / CABINAS_NORIA;

        Vector3 enganche =
        {
            NORIA_X + std::cos(a) * NORIA_RADIO,
            NORIA_ALTO_EJE + std::sin(a) * NORIA_RADIO,
            NORIA_Z
        };

        Vector3 cabina = { enganche.x, enganche.y - 0.5f, enganche.z };
        Color color = COLORES_BOMBILLA[i % CANTIDAD_COLORES_BOMBILLA];

        DrawLine3D(enganche, Vector3{ cabina.x, cabina.y + 0.2f, cabina.z }, Color{ 40, 30, 50, 255 });
        DrawCubeV(cabina, Vector3{ 0.56f, 0.42f, 0.6f }, MezclaFeria(color, Color{ 40, 20, 50, 255 }, 0.35f));
        DrawCubeV(Vector3{ cabina.x, cabina.y + 0.25f, cabina.z }, Vector3{ 0.66f, 0.08f, 0.7f }, MezclaFeria(color, WHITE, 0.2f));
    }
}


static void DibujarCarruselFeria(float tiempo)
{
    if (!mallaRotorCarruselFeria.cargada)
    {
        ConstruirRotorCarruselFeria();
    }

    float sube = std::sin(tiempo * 2.4f) * 0.2f;

    rlPushMatrix();
    rlTranslatef(CARRUSEL_X, 0.0f, CARRUSEL_Z);
    rlRotatef(tiempo * 32.0f, 0.0f, 1.0f, 0.0f);
    DibujarMallaGpu(mallaRotorCarruselFeria);
    DibujarMallaGpu(mallasCaballosFeria[0], 0.0f, sube, 0.0f);
    DibujarMallaGpu(mallasCaballosFeria[1], 0.0f, -sube, 0.0f);
    rlPopMatrix();
}


//==================================================
// ANIMACIONES (modo inmediato: solo lo que se mueve o es translucido)
//==================================================

// Tren de la montaña rusa: tres carros sobre la pista muestreada.
static void DibujarTrenRusaFeria(float tiempo)
{
    if (cantidadMuestrasRusa < 4)
    {
        return;
    }

    static const Color CARROS[3] =
    {
        { 250, 204, 60, 255 },
        { 230, 70, 80, 255 },
        { 80, 170, 230, 255 }
    };

    float fraccion = 0.0f;
    int cabeza = MuestraTrenRusaFeria(tiempo, fraccion);
    const int n = cantidadMuestrasRusa;
    const float SEPARACION = 1.2f / PASO_RUSA;

    for (int c = 0; c < 3; c++)
    {
        float s = cabeza + fraccion - c * SEPARACION;

        while (s < 0.0f)
        {
            s += n;
        }

        int i0 = (int)s % n;
        int i1 = (i0 + 1) % n;
        float k = s - std::floor(s);

        const MuestraRusaFeria& a = muestrasRusa[i0];
        const MuestraRusaFeria& b = muestrasRusa[i1];

        Vector3 p = SumarFeria(a.p, Vector3{ b.p.x - a.p.x, b.p.y - a.p.y, b.p.z - a.p.z }, k);
        Vector3 t = NormalizarFeria(SumarFeria(a.tangente, Vector3{ b.tangente.x - a.tangente.x, b.tangente.y - a.tangente.y, b.tangente.z - a.tangente.z }, k));
        Vector3 l = NormalizarFeria(SumarFeria(a.lateral, Vector3{ b.lateral.x - a.lateral.x, b.lateral.y - a.lateral.y, b.lateral.z - a.lateral.z }, k));
        Vector3 u = CruzFeria(l, t);

        // Columnas: lateral, arriba, adelante y posicion.
        float matriz[16] =
        {
            l.x, l.y, l.z, 0.0f,
            u.x, u.y, u.z, 0.0f,
            t.x, t.y, t.z, 0.0f,
            p.x, p.y, p.z, 1.0f
        };

        rlPushMatrix();
        rlMultMatrixf(matriz);

        DrawCubeV(Vector3{ 0.0f, 0.1f, 0.0f }, Vector3{ 0.74f, 0.16f, 1.0f }, Color{ 40, 36, 46, 255 });
        DrawCubeV(Vector3{ 0.0f, 0.36f, 0.0f }, Vector3{ 0.8f, 0.36f, 1.04f }, CARROS[c]);
        DrawCubeV(Vector3{ 0.0f, 0.62f, -0.34f }, Vector3{ 0.7f, 0.3f, 0.12f }, Color{ 50, 40, 60, 255 });

        if (c == 0)
        {
            DrawCubeV(Vector3{ 0.0f, 0.3f, 0.58f }, Vector3{ 0.7f, 0.28f, 0.16f }, Color{ 250, 230, 140, 255 });
        }

        DrawSphereEx(Vector3{ -0.18f, 0.74f, 0.08f }, 0.12f, 3, 5, Color{ 230, 180, 140, 255 });
        DrawSphereEx(Vector3{ 0.18f, 0.74f, 0.08f }, 0.12f, 3, 5, Color{ 160, 110, 80, 255 });

        rlPopMatrix();
    }
}


// Fuegos artificiales al fondo de la feria: cohete, estallido y chispas
// que caen. La cantidad depende de la calidad de particulas.
static void DibujarFuegosFeria(float tiempo)
{
    float factor = FactorCalidadGrafica(CalidadParticulas());
    int fuegos = (int)std::ceil(4.0f * factor);
    int chispas = (int)std::ceil(22.0f * factor);

    if (chispas < 8)
    {
        chispas = 8;
    }

    static const Color COLORES_FUEGO[5] =
    {
        { 255, 120, 200, 255 },
        { 120, 220, 255, 255 },
        { 255, 220, 110, 255 },
        { 170, 255, 140, 255 },
        { 255, 140, 90, 255 }
    };

    for (int f = 0; f < fuegos; f++)
    {
        float periodo = 3.6f + 0.9f * f;
        float desfase = f * 1.37f;
        float ciclo = (tiempo + desfase) / periodo;
        int vuelta = (int)std::floor(ciclo);
        float u = ciclo - vuelta;

        // Lugar al fondo y a los costados del mapa (cambia cada vuelta).
        float ang = -2.9f + 2.7f * HashFeria(vuelta * 7 + f, 11);
        float dist = 30.0f + 6.0f * HashFeria(vuelta * 7 + f, 12);
        float x = CENTRO_CASILLAS_X + std::cos(ang) * dist;
        float z = CENTRO_CASILLAS_Z + std::sin(ang) * dist * 0.85f;
        float alto = 10.0f + 4.0f * HashFeria(vuelta * 7 + f, 13);
        Color color = COLORES_FUEGO[(vuelta + f) % 5];

        if (u < 0.22f)
        {
            // Cohete subiendo con estela.
            float k = u / 0.22f;
            float y = 1.0f + (alto - 1.0f) * (1.0f - (1.0f - k) * (1.0f - k));

            DrawSphereEx(Vector3{ x, y, z }, 0.09f, 2, 4, Color{ 255, 230, 180, 255 });
            DrawLine3D(Vector3{ x, y - 1.2f, z }, Vector3{ x, y, z }, ConAlfaFeria(Color{ 255, 200, 140, 255 }, 0.6f));
            continue;
        }

        if (u > 0.9f)
        {
            continue;
        }

        float k = (u - 0.22f) / 0.68f;
        float radio = 3.2f * (1.0f - (1.0f - k) * (1.0f - k) * (1.0f - k));
        float caida = 2.2f * k * k;
        float vida = 1.0f - k;

        for (int c = 0; c < chispas; c++)
        {
            // Direcciones repartidas en la esfera.
            float yv = 1.0f - 2.0f * (c + 0.5f) / chispas;
            float rv = std::sqrt(1.0f - yv * yv);
            float av = c * 2.39996f + f;

            Vector3 p =
            {
                x + std::cos(av) * rv * radio,
                alto + yv * radio - caida,
                z + std::sin(av) * rv * radio
            };

            DrawSphereEx(p, 0.11f * (0.4f + 0.6f * vida), 2, 4, ConAlfaFeria(MezclaFeria(WHITE, color, 0.3f + 0.7f * k), vida));
        }
    }
}


// Globos que se escapan y suben hasta perderse.
static void DibujarGlobosSueltosFeria(float tiempo)
{
    int globos = (int)std::ceil(6.0f * FactorCalidadGrafica(CalidadParticulas()));

    static const Color COLORES[4] =
    {
        { 236, 70, 90, 255 },
        { 250, 210, 70, 255 },
        { 90, 200, 240, 255 },
        { 200, 110, 240, 255 }
    };

    for (int g = 0; g < globos; g++)
    {
        float periodo = 11.0f + 2.0f * g;
        float ciclo = (tiempo + g * 3.1f) / periodo;
        int vuelta = (int)std::floor(ciclo);
        float u = ciclo - vuelta;

        // Salen del carrito, de los racimos o de las carpas.
        float x0 = -20.0f + 36.0f * HashFeria(vuelta * 5 + g, 21);
        float z0 = -10.0f + 22.0f * HashFeria(vuelta * 5 + g, 22);

        Vector3 p =
        {
            x0 + std::sin(tiempo * 0.7f + g) * 0.6f + u * 3.0f,
            1.5f + u * 16.0f,
            z0 - u * 4.0f
        };

        Color color = COLORES[(vuelta + g) % 4];

        DrawSphereEx(p, 0.26f, 3, 6, color);
        DrawLine3D(Vector3{ p.x, p.y - 0.26f, p.z }, Vector3{ p.x + 0.1f, p.y - 1.0f, p.z }, Color{ 220, 220, 220, 255 });
    }
}


// Luciernagas sobre el pasto, lejos de las luces.
static void DibujarLuciernagasFeria(float tiempo)
{
    int cantidad =
        (int)std::ceil(24.0f * FactorCalidadGrafica(CalidadParticulas()));

    for (int i = 0; i < cantidad; i++)
    {
        float ang = 6.2831853f * HashFeria(i, 31);
        float dist = 8.0f + 26.0f * HashFeria(i, 32);

        float x = CENTRO_CASILLAS_X + std::cos(ang) * dist + std::sin(tiempo * 0.4f + i) * 0.8f;
        float z = CENTRO_CASILLAS_Z + std::sin(ang) * dist * 0.8f + std::cos(tiempo * 0.3f + i * 1.7f) * 0.8f;
        float y = AlturaSueloFeria(x, z) + 0.6f + 1.2f * HashFeria(i, 33) + std::sin(tiempo * 1.1f + i * 2.3f) * 0.25f;

        float brillo = Parpadeo(tiempo + i * 1.3f, 2.4f);

        if (brillo < 0.3f)
        {
            continue;
        }

        DrawSphereEx(
            Vector3{ x, y, z },
            0.05f,
            3,
            5,
            MezclaFeria(Color{ 80, 110, 60, 255 }, Color{ 220, 255, 140, 255 }, brillo)
        );
    }
}


// Halos de faroles, reflectores que barren el cielo y la luz de la antena.
static void DibujarResplandoresFeria(float tiempo)
{
    if (CalidadEfectos() == CALIDAD_BAJA)
    {
        return;
    }

    rlDrawRenderBatchActive();
    rlDisableDepthMask();

    for (int i = 0; i < cantidadFarolesFeria; i++)
    {
        const Vector3& f = farolesFeria[i];
        float pulso = 0.85f + 0.15f * std::sin(tiempo * 3.0f + i * 1.7f);

        DrawSphereEx(
            Vector3{ f.x, f.y + ALTO_FAROL_FERIA + 0.1f, f.z },
            0.42f * pulso, 4, 8,
            Color{ 255, 200, 120, 46 }
        );
    }

    // Halo de la estrella del portal y del frasco.
    DrawSphereEx(Vector3{ PORTAL_X, 4.85f, PORTAL_Z }, 0.7f, 4, 8, Color{ 255, 220, 120, 40 });

    // Dos reflectores detras de la carpa mayor.
    for (int lado = -1; lado <= 1; lado += 2)
    {
        float a = 0.45f * std::sin(tiempo * 0.35f + lado * 1.4f) + 0.2f * lado;
        Vector3 base = { CARPA_MAYOR_X + 5.4f * lado, 0.4f, CARPA_MAYOR_Z - 3.2f };
        Vector3 fin = { base.x + std::sin(a) * 9.0f, base.y + 17.0f, base.z - 5.0f };

        DrawCylinderEx(base, fin, 0.2f, 2.2f, 10, Color{ 200, 210, 255, 20 });
    }

    rlDrawRenderBatchActive();
    rlEnableDepthMask();

    // Luz roja de la antena de la ciudad.
    if (std::fmod(tiempo, 1.6f) < 0.6f)
    {
        DrawSphereEx(PuntaAntenaFeria(), 0.35f, 3, 6, Color{ 255, 60, 60, 255 });
    }
}


//==================================================
// DECORACION
//==================================================

static void DibujarDecoracionFeria(
    const PartidaTablero& partida
)
{
    float tiempo = partida.tiempoTablero;

    // Postes, faroles y charcos de luz antes de hornear el suelo.
    PrepararEscenaFeria();

    // MODELO FUTURO: la feria completa como GLB.
    DibujarTelonFeria();
    DibujarTerrenoFeria();
    DibujarSenderosFeria();
    DibujarPropsFeria();
    DibujarLandmarksFeria(tiempo);

    // Animado.
    DibujarNoriaFeria(tiempo);
    DibujarCarruselFeria(tiempo);
    DibujarTrenRusaFeria(tiempo);
    DibujarLuciernagasFeria(tiempo);
    DibujarGlobosSueltosFeria(tiempo);

    // Translucido al final.
    DibujarFuegosFeria(tiempo);
    DibujarResplandoresFeria(tiempo);
}


// Velo nocturno: cielo oscuro arriba, bordes en penumbra, el resplandor
// calido de la feria abajo y confeti que cae despacio.
static void DibujarCapaPantallaFeria(
    const PartidaTablero& partida
)
{
    int ancho = GetScreenWidth();
    int alto = GetScreenHeight();
    int borde = ancho / 7;
    float t = partida.tiempoTablero;
    float escala = alto / 720.0f;

    DrawRectangleGradientV(
        0, 0, ancho, alto / 2,
        Color{ 12, 6, 40, 96 },
        Color{ 12, 6, 40, 0 }
    );

    DrawRectangleGradientV(
        0, alto - alto / 3, ancho, alto / 3,
        Color{ 255, 120, 170, 0 },
        Color{ 255, 120, 170, (unsigned char)(26 + 8 * Parpadeo(t, 0.9f)) }
    );

    DrawRectangleGradientH(0, 0, borde, alto, Color{ 8, 4, 22, 92 }, Color{ 8, 4, 22, 0 });
    DrawRectangleGradientH(ancho - borde, 0, borde, alto, Color{ 8, 4, 22, 0 }, Color{ 8, 4, 22, 92 });

    // Confeti.
    int papelitos = (int)std::ceil(30.0f * FactorCalidadGrafica(CalidadParticulas()));

    for (int i = 0; i < papelitos; i++)
    {
        float vy = 22.0f + 26.0f * HashFeria(i, 5);
        float x = HashFeria(i, 1) * ancho + 30.0f * std::sin(t * 0.8f + i * 1.3f);
        float y = std::fmod(HashFeria(i, 2) * alto + t * vy, (float)alto + 20.0f) - 10.0f;

        float giro = t * (2.0f + 3.0f * HashFeria(i, 6)) + i;
        float anchoPapel = (2.0f + 3.0f * std::fabs(std::sin(giro))) * escala;
        float altoPapel = 3.5f * escala;

        Color color = COLORES_BOMBILLA[i % CANTIDAD_COLORES_BOMBILLA];

        DrawRectangle(
            (int)x, (int)y, (int)anchoPapel + 1, (int)altoPapel,
            ConAlfaFeria(color, 0.55f)
        );
    }
}


//==================================================
// GIMMICK: LA GRAN RUEDA Y EL POZO
//==================================================

// Patron de gajos (aproxima las probabilidades): G = premio mayor,
// M = medio pozo, T = rueda trabada.
static const char PATRON_GAJOS_RUEDA[GAJOS_RUEDA + 1] = "GMMTGMGMTGMT";


static Color ColorGajoRueda(int gajo)
{
    switch (PATRON_GAJOS_RUEDA[gajo % GAJOS_RUEDA])
    {
        case 'G': return Color{ 246, 196, 60, 255 };
        case 'M': return Color{ 76, 196, 232, 255 };
        default: break;
    }

    return Color{ 222, 52, 66, 255 };
}


static void DibujarGranRuedaFeria(float tiempo)
{
    // MODELO FUTURO: la Gran Rueda como modelo 3D.
    Vector3 eje = { RUEDA_X, RUEDA_ALTO_EJE, RUEDA_Z };

    // Aro dorado (de fondo) y disco oscuro.
    DrawCylinderEx(
        Vector3{ eje.x, eje.y, eje.z - 0.24f },
        Vector3{ eje.x, eje.y, eje.z },
        RUEDA_RADIO + 0.16f, RUEDA_RADIO + 0.16f, 28,
        DORADO_FERIA
    );

    float giro = anguloRuedaFeria * DEG2RAD;
    float zCara = eje.z + 0.03f;
    Vector3 centro = { eje.x, eje.y, zCara };

    int luzActiva = (int)(tiempo * 6.0f) % 3;

    for (int i = 0; i < GAJOS_RUEDA; i++)
    {
        float a0 = giro + 2.0f * PI * i / GAJOS_RUEDA;
        float a1 = giro + 2.0f * PI * (i + 1) / GAJOS_RUEDA;

        Vector3 p0 = { eje.x + std::cos(a0) * RUEDA_RADIO, eje.y + std::sin(a0) * RUEDA_RADIO, zCara };
        Vector3 p1 = { eje.x + std::cos(a1) * RUEDA_RADIO, eje.y + std::sin(a1) * RUEDA_RADIO, zCara };

        Color color = ColorGajoRueda(i);

        // Dos caras: no depende del sentido del triangulo.
        DrawTriangle3D(centro, p0, p1, color);
        DrawTriangle3D(centro, p1, p0, color);

        // Separador y clavija del borde.
        DrawCylinderEx(
            Vector3{ centro.x, centro.y, zCara + 0.02f },
            Vector3{ p0.x, p0.y, zCara + 0.02f },
            0.025f, 0.025f, 3,
            Color{ 60, 30, 70, 255 }
        );

        Vector3 clavija =
        {
            eje.x + std::cos(a0) * (RUEDA_RADIO + 0.04f),
            eje.y + std::sin(a0) * (RUEDA_RADIO + 0.04f),
            zCara + 0.06f
        };

        DrawSphereEx(
            clavija,
            0.075f,
            3, 6,
            i % 3 == luzActiva ? Color{ 255, 246, 200, 255 } : Color{ 150, 110, 60, 255 }
        );
    }

    // Cubo central con estrella.
    DrawCylinderEx(
        Vector3{ eje.x, eje.y, zCara },
        Vector3{ eje.x, eje.y, zCara + 0.18f },
        0.40f, 0.32f, 14,
        VIOLETA_FERIA
    );

    DrawSphereEx(Vector3{ eje.x, eje.y, zCara + 0.2f }, 0.18f, 4, 8, DORADO_FERIA);
}


// Marquesina de bombillas en el frente de la tarima.
static void DibujarMarquesinaFeria(float tiempo)
{
    if (CalidadEfectos() == CALIDAD_BAJA)
    {
        return;
    }

    const int BOMBILLAS = 11;

    for (int i = 0; i < BOMBILLAS; i++)
    {
        DibujarBombillaFeria(
            Vector3{ RUEDA_X - 2.3f + 4.6f * i / (BOMBILLAS - 1), 0.42f, RUEDA_Z + 0.52f },
            i,
            tiempo * 1.6f,
            0.07f
        );
    }
}


static void DibujarFrascoPozoFeria(float tiempo)
{
    // Pila de monedas: crece con el pozo (suavizado) hasta llenar el frasco.
    const float GROSOR_CAPA = 0.09f;
    float alturaMaxima = FRASCO_ALTO - 0.15f;
    float altura = 0.08f + pozoVisualFeria * 0.055f;

    if (altura > alturaMaxima)
    {
        altura = alturaMaxima;
    }

    int capas = (int)(altura / GROSOR_CAPA);

    for (int i = 0; i < capas; i++)
    {
        DrawCylinder(
            Vector3{ FRASCO_X, FRASCO_BASE + i * GROSOR_CAPA, FRASCO_Z },
            FRASCO_RADIO - 0.10f,
            FRASCO_RADIO - 0.10f,
            GROSOR_CAPA - 0.012f,
            16,
            i % 2 == 0 ? Color{ 238, 186, 52, 255 } : Color{ 210, 154, 40, 255 }
        );
    }

    // Monedas sueltas encima de la pila.
    float cima = FRASCO_BASE + capas * GROSOR_CAPA;

    for (int i = 0; i < 4; i++)
    {
        float a = i * 1.7f;
        Vector3 p = { FRASCO_X + std::cos(a) * 0.38f, cima + 0.03f, FRASCO_Z + std::sin(a) * 0.38f };

        DrawCylinderEx(
            p,
            Vector3{ p.x + 0.03f, p.y + 0.06f, p.z },
            0.22f, 0.22f, 10,
            Color{ 252, 210, 80, 255 }
        );
    }

    // Moneda que cae al pozo despues de una casilla roja.
    if (caidaPozoFeria > 0.0f)
    {
        float giro = tiempo * 9.0f;
        Vector3 p = { FRASCO_X, cima + 0.2f + caidaPozoFeria * 3.2f, FRASCO_Z };
        Vector3 grosor = { std::cos(giro) * 0.035f, 0.0f, std::sin(giro) * 0.035f };

        DrawCylinderEx(
            Vector3{ p.x - grosor.x, p.y, p.z - grosor.z },
            Vector3{ p.x + grosor.x, p.y, p.z + grosor.z },
            0.24f, 0.24f, 12,
            Color{ 255, 220, 90, 255 }
        );
    }

    // Vidrio translucido al final, sin escribir profundidad.
    rlDrawRenderBatchActive();
    rlDisableDepthMask();

    DrawCylinder(
        Vector3{ FRASCO_X, FRASCO_BASE, FRASCO_Z },
        FRASCO_RADIO,
        FRASCO_RADIO,
        FRASCO_ALTO,
        24,
        Color{ 170, 214, 255, 60 }
    );

    DrawCubeV(
        Vector3{ FRASCO_X - FRASCO_RADIO * 0.62f, FRASCO_BASE + FRASCO_ALTO * 0.5f, FRASCO_Z + FRASCO_RADIO * 0.72f },
        Vector3{ 0.10f, FRASCO_ALTO * 0.8f, 0.02f },
        Color{ 255, 255, 255, 70 }
    );

    rlDrawRenderBatchActive();
    rlEnableDepthMask();
}


static void DibujarGimmickFeria(
    const PartidaTablero& partida
)
{
    float t = partida.tiempoTablero;

    DibujarGranRuedaFeria(t);
    DibujarMarquesinaFeria(t);
    DibujarFrascoPozoFeria(t);
}


static void ActualizarGimmickFeria(
    PartidaTablero& partida,
    float deltaTime
)
{
    // gimmickAnim = impulso de giro de la Gran Rueda (1 al girarla).
    if (partida.gimmickAnim > 0.0f)
    {
        partida.gimmickAnim -= deltaTime * 0.45f;

        if (partida.gimmickAnim < 0.0f)
        {
            partida.gimmickAnim = 0.0f;
        }
    }

    float impulso = partida.gimmickAnim;
    float velocidad = 16.0f + 640.0f * impulso * impulso;

    anguloRuedaFeria =
        std::fmod(anguloRuedaFeria + velocidad * deltaTime, 360.0f);

    float suavizado = 1.0f - std::exp(-3.0f * deltaTime);

    pozoVisualFeria +=
        ((float)monedasPozo - pozoVisualFeria) * suavizado;

    if (caidaPozoFeria > 0.0f)
    {
        caidaPozoFeria -= deltaTime * 1.3f;

        if (caidaPozoFeria < 0.0f)
        {
            caidaPozoFeria = 0.0f;
        }
    }
}


static void ActualizarEstadoFeria(
    PartidaTablero& partida
)
{
    std::snprintf(
        partida.estadoGimmick,
        sizeof(partida.estadoGimmick),
        "POZO: %d MONEDAS | TROFEO: %d",
        monedasPozo,
        partida.costoTrofeo
    );
}


static void DepositarEnPozoFeria(
    PartidaTablero& partida,
    int monedas
)
{
    if (monedas <= 0)
    {
        return;
    }

    monedasPozo += monedas;
    caidaPozoFeria = 1.0f;

    ActualizarEstadoFeria(partida);
}


// Cobra hasta `monto` monedas (sin dejar negativo) y las echa al pozo.
// El Escudo anula la perdida: `bloqueada` = true. Devuelve lo cobrado.
static int CobrarAlPozoFeria(
    PartidaTablero& partida,
    int participante,
    int monto,
    bool& bloqueada
)
{
    EstadoJugadorPartidaTablero& jugador =
        partida.jugadores[participante];

    int perdida =
        jugador.monedas < monto
        ? jugador.monedas
        : monto;

    bloqueada = false;

    if (perdida > 0 && EscudoAbsorbePerdida(partida, participante))
    {
        bloqueada = true;
        return 0;
    }

    jugador.monedas -= perdida;
    partida.variacionMonedasEvento = -perdida;

    DepositarEnPozoFeria(partida, perdida);

    return perdida;
}


// Remate del trofeo: desde la ronda 2 el precio se sortea cada ronda.
static void AlIniciarRondaFeria(
    PartidaTablero& partida
)
{
    if (partida.rondaActual <= 1)
    {
        partida.costoTrofeo = COSTO_TROFEO_FERIA;
        partida.gimmickActivo = false;

        EstablecerMensajeGimmick(
            partida,
            "¡BIENVENIDOS! LAS CASILLAS ROJAS LLENAN EL POZO"
        );
    }
    else
    {
        int cantidadPrecios =
            (int)(sizeof(PRECIOS_REMATE) / sizeof(PRECIOS_REMATE[0]));

        int precio =
            PRECIOS_REMATE[GetRandomValue(0, cantidadPrecios - 1)];

        if (
            partida.rondaActual >= partida.cantidadRondas &&
            precio < PRECIO_MINIMO_ULTIMA_RONDA
        )
        {
            precio = PRECIO_MINIMO_ULTIMA_RONDA;
        }

        partida.costoTrofeo = precio;

        // Activo = la camara enfoca la Gran Rueda mientras dura el aviso.
        partida.gimmickActivo = true;

        char mensaje[96];

        std::snprintf(
            mensaje,
            sizeof(mensaje),
            "¡REMATE! EL TROFEO CUESTA %d MONEDAS ESTA RONDA",
            precio
        );

        EstablecerMensajeGimmick(partida, mensaje);
    }

    ActualizarEstadoFeria(partida);
}


//==================================================
// EVENTOS DE CASILLA
//==================================================

// Casilla roja: la perdida cae al pozo (el Escudo la bloquea igual que la
// regla comun).
static void ResolverCasillaRojaFeria(
    PartidaTablero& partida,
    int participante
)
{
    bool bloqueada = false;

    int perdida =
        CobrarAlPozoFeria(
            partida,
            participante,
            MONEDAS_CASILLA_NEGATIVA,
            bloqueada
        );

    if (bloqueada)
    {
        std::snprintf(
            partida.textoEvento,
            sizeof(partida.textoEvento),
            "¡EL ESCUDO DE J%d BLOQUEA LA PÉRDIDA!",
            participante + 1
        );
    }
    else if (perdida > 0)
    {
        std::snprintf(
            partida.textoEvento,
            sizeof(partida.textoEvento),
            "-%d MONEDAS: CAEN AL POZO (%d)",
            perdida,
            monedasPozo
        );
    }
    else
    {
        std::snprintf(
            partida.textoEvento,
            sizeof(partida.textoEvento),
            "NO TENÉS MONEDAS: EL POZO SIGUE EN %d",
            monedasPozo
        );
    }

    ReproducirSonidoTablero(partida, SONIDO_CASILLA_NEGATIVA);
}


// Evento propio: girar LA GRAN RUEDA (resolucion inmediata).
static void GirarGranRuedaFeria(
    PartidaTablero& partida,
    int participante
)
{
    EstadoJugadorPartidaTablero& jugador =
        partida.jugadores[participante];

    ReproducirSonidoTablero(partida, SONIDO_EVENTO_TABLERO);

    // Impulso visual de giro.
    partida.gimmickAnim = 1.0f;

    int tirada = GetRandomValue(0, 99);

    if (tirada < PROBABILIDAD_PREMIO_MAYOR + PROBABILIDAD_MEDIO_POZO)
    {
        if (monedasPozo <= 0)
        {
            std::snprintf(
                partida.textoEvento,
                sizeof(partida.textoEvento),
                "LA RUEDA PAGA... ¡PERO EL POZO NO TIENE MONEDAS!"
            );

            return;
        }

        int premio = 0;

        if (tirada < PROBABILIDAD_PREMIO_MAYOR)
        {
            premio = monedasPozo;
            monedasPozo = POZO_INICIAL;

            std::snprintf(
                partida.textoEvento,
                sizeof(partida.textoEvento),
                "¡PREMIO MAYOR! TE LLEVÁS EL POZO: +%d MONEDAS",
                premio
            );
        }
        else
        {
            premio = monedasPozo / 2;

            if (premio < 1)
            {
                premio = 1;
            }

            monedasPozo -= premio;

            std::snprintf(
                partida.textoEvento,
                sizeof(partida.textoEvento),
                "MEDIO POZO: +%d MONEDAS",
                premio
            );
        }

        jugador.monedas += premio;
        partida.ultimoEventoEspecial = EVENTO_TABLERO_BONIFICACION;
        partida.variacionMonedasEvento = premio;

        ActualizarEstadoFeria(partida);
        ReproducirSonidoTablero(partida, SONIDO_MONEDA);

        return;
    }

    // 25 %: la rueda se traba y el jugador paga al pozo.
    bool bloqueada = false;

    int pago =
        CobrarAlPozoFeria(
            partida,
            participante,
            PAGO_RUEDA_TRABADA,
            bloqueada
        );

    partida.ultimoEventoEspecial = EVENTO_TABLERO_MULTA;

    if (bloqueada)
    {
        std::snprintf(
            partida.textoEvento,
            sizeof(partida.textoEvento),
            "¡LA RUEDA SE TRABA! EL ESCUDO DE J%d BLOQUEA LA PÉRDIDA",
            participante + 1
        );
    }
    else if (pago > 0)
    {
        std::snprintf(
            partida.textoEvento,
            sizeof(partida.textoEvento),
            "¡LA RUEDA SE TRABA! PAGÁS %d AL POZO",
            pago
        );
    }
    else
    {
        std::snprintf(
            partida.textoEvento,
            sizeof(partida.textoEvento),
            "¡LA RUEDA SE TRABA! NO TENÉS MONEDAS PARA PAGAR"
        );
    }
}


static bool ResolverCasillaFeria(
    PartidaTablero& partida,
    int participante
)
{
    const Casilla* casilla =
        partida.tablero.ObtenerCasilla(
            partida.jugadores[participante].casillaActual
        );

    if (casilla == nullptr)
    {
        return false;
    }

    if (casilla->tipo == CASILLA_EVENTO)
    {
        GirarGranRuedaFeria(partida, participante);
        return true;
    }

    if (casilla->tipo == CASILLA_NEGATIVA)
    {
        ResolverCasillaRojaFeria(partida, participante);
        return true;
    }

    // REGALO, CAOS, INTERCAMBIO, DUELO, TIENDA y las basicas: regla comun.
    return false;
}


// Leyenda del HUD: las rojas alimentan el pozo y el EVENTO es la Gran Rueda.
static bool TextoLeyendaCasillaFeria(
    const PartidaTablero& partida,
    TipoCasilla tipo,
    char* texto,
    int capacidad
)
{
    (void)partida;

    if (tipo == CASILLA_NEGATIVA)
    {
        std::snprintf(texto, capacidad, "-%d AL POZO", MONEDAS_CASILLA_NEGATIVA);
        return true;
    }

    if (tipo == CASILLA_EVENTO)
    {
        std::snprintf(texto, capacidad, "GRAN RUEDA");
        return true;
    }

    return false;
}


//==================================================
// BOTS Y TEXTOS DE RUTA
//==================================================

// Valor aproximado de pisar una casilla (para que los bots elijan ruta).
static int ValorCasillaBotFeria(
    const PartidaTablero& partida,
    const Casilla& casilla
)
{
    (void)partida;

    // Sin "default": un tipo nuevo de casilla obliga a puntuarlo aqui.
    switch (casilla.tipo)
    {
        case CASILLA_POSITIVA: return MONEDAS_CASILLA_POSITIVA;
        case CASILLA_NEGATIVA: return -MONEDAS_CASILLA_NEGATIVA;
        case CASILLA_ESPECIAL: return 1;
        case CASILLA_REGALO: return MONEDAS_REGALO;
        case CASILLA_CAOS: return -4;           // ruleta de castigos
        case CASILLA_INTERCAMBIO: return -1;    // azar: algo arriesgada

        // Gran Rueda: esperanza 0.30 * pozo + 0.45 * pozo / 2 - 0.25 * 3,
        // vale mas cuanto mas lleno esta el frasco.
        case CASILLA_EVENTO: return (monedasPozo * 21 - PAGO_RUEDA_TRABADA * 10) / 40;

        case CASILLA_DUELO: return 0;           // apuesta voluntaria: ni premio ni castigo
        case CASILLA_TIENDA: return 1;          // compra objetos: algo util, no un premio
        case CASILLA_NEUTRA: return 0;
        case CANTIDAD_TIPOS_CASILLA: return 0;
    }

    return 0;
}


// Valor medio de los proximos pasos desde una casilla (todas las ramas).
static float ValorRutaBotFeria(
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
            ValorRutaBotFeria(
                partida, actual->conexiones[c].destino, profundidad - 1
            );
    }

    if (actual->cantidadConexiones > 0)
    {
        resto /= (float)actual->cantidadConexiones;
    }

    return (float)ValorCasillaBotFeria(partida, *actual) + resto;
}


// Pasos minimos desde una casilla hasta otra (-1 si no hay camino).
static int PasosHastaFeria(
    const Tablero& tablero,
    int desde,
    int hasta
)
{
    int distancia[MAX_CASILLAS_TABLERO];
    int cola[MAX_CASILLAS_TABLERO];
    int inicio = 0;
    int fin = 0;

    for (int i = 0; i < MAX_CASILLAS_TABLERO; i++)
    {
        distancia[i] = -1;
    }

    if (desde < 0 || desde >= tablero.cantidadCasillas)
    {
        return -1;
    }

    distancia[desde] = 0;
    cola[fin++] = desde;

    while (inicio < fin)
    {
        int actual = cola[inicio++];

        if (actual == hasta)
        {
            return distancia[actual];
        }

        for (int c = 0; c < tablero.casillas[actual].cantidadConexiones; c++)
        {
            int destino = tablero.casillas[actual].conexiones[c].destino;

            if (destino >= 0 && distancia[destino] < 0)
            {
                distancia[destino] = distancia[actual] + 1;
                cola[fin++] = destino;
            }
        }
    }

    return -1;
}


// Con monedas para el trofeo (al precio del remate), el bot va hacia el;
// sin ellas busca la ruta de casillas mas ricas (6 pasos: los lobulos son
// largos). Siempre deja algo de azar.
static int DecidirRutaBotFeria(
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
                PasosHastaFeria(
                    partida.tablero,
                    actual->conexiones[c].destino,
                    partida.casillaTrofeo
                );

            if (pasos >= 0 && pasos < mejorPasos)
            {
                mejorPasos = pasos;
                mejor = c;
            }
            else if (pasos == mejorPasos)
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
            ValorRutaBotFeria(
                partida, actual->conexiones[c].destino, 6
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


static const char* DescribirRutaFeria(
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

    return descripcionRutaFeria[casilla][opcion];
}


//==================================================
// RECURSOS Y CONSTRUCCION
//==================================================

static void DescargarRecursosFeria()
{
    DescargarMalla(mallaTerrenoFeria);
    DescargarMalla(mallaTelonFeria);
    DescargarMalla(mallaSenderosFeria);
    nivelSenderosFeria = -1;
    DescargarMalla(mallaLandmarksFeria);
    DescargarMalla(mallaPropsFeria);
    nivelPropsFeria = -1;
    DescargarMalla(mallaGuirnaldasFeria);
    DescargarMalla(mallaRuedaNoriaFeria);
    DescargarMalla(mallaRotorCarruselFeria);
    DescargarMalla(mallasCaballosFeria[0]);
    DescargarMalla(mallasCaballosFeria[1]);

    for (int f = 0; f < FASES_GUIRNALDA; f++)
    {
        DescargarMalla(mallasFaseGuirnaldaFeria[f]);
    }
}


static void ConstruirFeria(
    Tablero& tablero
)
{
    // Casillas, conexiones y validacion: ver ConstruirRutaFeria.
    ConstruirRutaFeria(tablero);

    // Partida nueva: el pozo vuelve a su semilla.
    monedasPozo = POZO_INICIAL;
    pozoVisualFeria = (float)POZO_INICIAL;
    caidaPozoFeria = 0.0f;
    anguloRuedaFeria = 0.0f;
}


//==================================================
// DEFINICION
//==================================================

const DefinicionTablero& ObtenerDefinicionTableroFeriaNocturna()
{
    static DefinicionTablero definicion;
    static bool preparada = false;

    if (!preparada)
    {
        definicion.id = TABLERO_FERIA_NOCTURNA;
        definicion.nombre = "FERIA NOCTURNA";

        definicion.historia =
            "Al caer la noche abre la feria. Las monedas que se pierden caen "
            "al frasco de la Gran Rueda y el trofeo se remata cada ronda.";

        definicion.objetivo =
            "Reuní más trofeos en 5 rondas: el trofeo cuesta 15, 20 o 25 monedas según el remate.";

        definicion.gimmick =
            "El pozo: lo que perdés en las rojas cae al frasco. Girá la Gran Rueda para llevártelo.";

        definicion.dificultad = "MEDIA";

        definicion.colorTema = Color{ 214, 70, 200, 255 };
        definicion.colorFondo = Color{ 16, 12, 34, 255 };

        // Camara diorama: objetivo en el centro de la caja de casillas
        // (46.6 x 30.8 de mundo, diagonal 55.8), desfase (0, 17, 15.5).
        // El mapa es grande: la camara sigue al jugador y abre una vista
        // general al empezar cada ronda.
        definicion.camaraObjetivo = { CENTRO_CASILLAS_X, 0.0f, CENTRO_CASILLAS_Z };
        definicion.camaraPosicion = { CENTRO_CASILLAS_X, 17.0f, CENTRO_CASILLAS_Z + 15.5f };
        definicion.camaraFovy = 48.0f;
        definicion.camaraSeguimiento = 1.0f;
        definicion.camaraZoomGeneral = 2.4f;
        definicion.camaraEscalaVistaPrevia = 2.4f;

        // Landmark: la Gran Rueda (enfoque al anunciar el remate).
        definicion.camaraLandmark = { RUEDA_X, 2.4f, RUEDA_Z + 1.2f };
        definicion.tieneLandmark = true;

        // Losas de madera pintada con confeti dorado.
        EstiloCasilla estilo;
        estilo.piedraLado = Color{ 84, 52, 92, 255 };
        estilo.piedraTapa = Color{ 142, 106, 146, 255 };
        estilo.borde = Color{ 40, 26, 48, 255 };
        estilo.detalle = Color{ 236, 190, 80, 255 };
        estilo.sendero = TIERRA_FERIA;
        estilo.senderoBorde = TIERRA_FERIA_BORDE;
        estilo.pulso = 0.18f;
        definicion.estiloCasilla = estilo;

        definicion.nombresZonas = NOMBRES_ZONAS_FERIA;
        definicion.cantidadZonas = CANTIDAD_ZONAS_FERIA;

        // Candidatas del trofeo (las llena ConstruirRutaFeria).
        TableroReferenciaFeria();

        definicion.casillasTrofeo = casillasTrofeoFeria;
        definicion.cantidadCasillasTrofeo = cantidadTrofeosFeria;

        definicion.costoTrofeo = COSTO_TROFEO_FERIA;
        definicion.cantidadRondas = 5;

        definicion.Construir = ConstruirFeria;
        definicion.DibujarDecoracion = DibujarDecoracionFeria;
        definicion.DescargarRecursos = DescargarRecursosFeria;
        definicion.DibujarCapaPantalla = DibujarCapaPantallaFeria;
        definicion.AlIniciarRonda = AlIniciarRondaFeria;
        definicion.ResolverCasilla = ResolverCasillaFeria;
        definicion.TextoLeyendaCasilla = TextoLeyendaCasillaFeria;
        definicion.ActualizarGimmick = ActualizarGimmickFeria;
        definicion.DibujarGimmick = DibujarGimmickFeria;
        definicion.DescribirOpcionRuta = DescribirRutaFeria;
        definicion.DecidirRutaBot = DecidirRutaBotFeria;

        preparada = true;
    }

    return definicion;
}
