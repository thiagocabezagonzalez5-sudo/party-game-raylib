#include "Board/CatalogoTableros.h"

#include "Gameplay/PartidaTablero.h"

#include "Board/MallaTablero.h"
#include "Systems/CalidadGrafica.h"

#include "rlgl.h"

#include <cmath>
#include <cstdio>


//==================================================
// ISLA ARBOLEDA (tablero 1)
//==================================================
//
// Mapa de ~54 casillas en forma de trebol alrededor de las Ruinas, con
// seis zonas y un landmark por zona (ver RUTA DEL TABLERO). El trofeo
// brota en un claro distinto cada vez que alguien lo cosecha.
//
// Gimmick: FLORACION. En las rondas pares los arboles florecen y las
// casillas verdes pagan +5 en lugar de +3.
//
// PRESENTACION (diorama de isla boscosa):
//   - Terreno: malla de triangulos con alturas (costa irregular con
//     playa y acantilados, terrazas, rio con laguna), generada una vez.
//   - Agua translucida con oleaje y espuma en la orilla.
//   - Vegetacion y rocas colocadas una sola vez con un generador
//     determinista que respeta senderos, rio y claros.
//   - Landmarks: Gran Roble (meseta del fondo), cascada, puentes, molino,
//     circulo de piedras con cristal, arco, muelle, cabana, atalaya y
//     setas gigantes. Todo se dibuja a ESCALA_ISLA del espacio de diseno.
//   - La FLORACION cambia colores del terreno, copas, flores, petalos
//     y la luz de la pantalla.
//
// La logica (casillas, rutas, eventos) vive en Tablero y en los hooks de
// abajo; todo lo de aqui es dibujo. MODELO FUTURO: el terreno completo y
// cada arbol/roca seran modelos GLB.

static const int MONEDAS_VERDE_FLORACION = 5;
static const float VELOCIDAD_ANIMACION_FLORACION = 1.4f;

static const float NIVEL_AGUA = -0.30f;

// La isla se disena a esta escala (terreno, landmarks y casillas comparten
// el "espacio de diseno"); al jugar todo se reduce por ESCALA_ISLA para que
// las casillas queden mas juntas sin cambiar su tamano.
static const float ESCALA_ISLA = 0.75f;

// Posicion del Gran Roble (centro de la meseta del fondo).
static const float ROBLE_X = -4.0f;
static const float ROBLE_Z = -25.0f;


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


static float Hash2(int a, int b)
{
    unsigned int h =
        (unsigned int)(a * 374761393 + b * 668265263);

    h = (h ^ (h >> 13)) * 1274126177u;

    return (float)((h ^ (h >> 16)) & 0xFFFF) / 65535.0f;
}


//==================================================
// RUTA DEL TABLERO (datos de la logica)
//==================================================
//
// Mapa mediano y denso (~53 casillas) con seis zonas, cada una con su
// landmark y sus eventos cerca. Tiene forma de TREBOL alrededor de las
// Ruinas (el centro de la isla): tres lazos que salen del circuito de las
// ruinas y vuelven a el, de modo que casi cualquier casilla queda a pocos
// turnos del centro.
//
//   - RUINAS Y JARDIN (centro): circuito de 7 casillas con tres salidas.
//   - COSTA (sur): puerta de salida, muelle con comerciante y la playa;
//     vuelve a las ruinas por el sendero del puente.
//   - BOSQUE BAJO (oeste): pradera con la cabana.
//   - RIO Y PUENTES: tres puentes, molino, laguna y cascada.
//   - CLARO DEL GRAN ROBLE (norte): lazo por el borde norte con rampa a la
//     meseta del Gran Roble y circuito propio.
//   - ALTIPLANO Y SETAS (este): lazo por el arco, la atalaya y el bosque
//     de pinos (casillas arriesgadas).
//
// En cada bifurcacion la opcion 0 es seguir en el lazo/circuito y la 1 el
// atajo. Las posiciones estan en "espacio de diseno" (el mismo del terreno y
// de los landmarks); ConstruirArboleda las reduce por ESCALA_ISLA al crear
// el tablero real y la decoracion se dibuja con esa misma escala.

enum ZonaArboleda
{
    ZONA_COSTA = 0,
    ZONA_BOSQUE_BAJO,
    ZONA_RIO,
    ZONA_RUINAS,
    ZONA_CLARO_ROBLE,
    ZONA_ALTIPLANO,
    CANTIDAD_ZONAS_ARBOLEDA
};

static const char* const NOMBRES_ZONAS_ARBOLEDA[CANTIDAD_ZONAS_ARBOLEDA] =
{
    "COSTA",
    "BOSQUE BAJO",
    "RIO Y PUENTES",
    "RUINAS Y JARDIN",
    "CLARO DEL GRAN ROBLE",
    "ALTIPLANO Y SETAS"
};

static const int MAX_PUENTES_ARBOLEDA = 8;
static const int MAX_TROFEOS_ARBOLEDA = 16;

// Datos que acompanan a las casillas (puentes, trofeo, textos de ruta).
static int puentesArboleda[MAX_PUENTES_ARBOLEDA][2];
static int cantidadPuentesArboleda = 0;

static int casillasTrofeoArboleda[MAX_TROFEOS_ARBOLEDA];
static int cantidadTrofeosArboleda = 0;

static const char* descripcionRutaArboleda[MAX_CASILLAS_TABLERO][2];

// Casillas clave que el dibujo necesita conocer.
static int casillaInicioArboleda = 0;


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


static int NodoArboleda(
    Tablero& t,
    ZonaArboleda zona,
    char tipo,
    float x, float y, float z
)
{
    int indice = t.AgregarCasilla(Vector3{ x, y, z }, TipoDesdeLetra(tipo));

    if (indice >= 0)
    {
        t.casillas[indice].zona = (int)zona;
    }

    return indice;
}


static void DefinirOpcionesArboleda(
    int casilla,
    const char* opcion0,
    const char* opcion1
)
{
    descripcionRutaArboleda[casilla][0] = opcion0;
    descripcionRutaArboleda[casilla][1] = opcion1;
}


static void ConstruirRutaArboleda(Tablero& t)
{
    t.cantidadCasillas = 0;
    t.recorridoValido = false;

    for (int i = 0; i < MAX_CASILLAS_TABLERO; i++)
    {
        t.casillas[i] = Casilla{};
        descripcionRutaArboleda[i][0] = nullptr;
        descripcionRutaArboleda[i][1] = nullptr;
    }

    cantidadPuentesArboleda = 0;
    cantidadTrofeosArboleda = 0;

    // Letras: + verde, - roja, n neutra, E especial, R regalo, C caos,
    // I intercambio, D duelo, T tienda, V evento.

    // --- COSTA: puerta (salida), muelle y playa del sur ---
    int s0 = NodoArboleda(t, ZONA_COSTA, 'E', -21.4f, 0.25f, 12.6f);
    int c1 = NodoArboleda(t, ZONA_COSTA, '+', -17.0f, 0.25f, 14.9f);
    int c2 = NodoArboleda(t, ZONA_COSTA, 'T', -13.0f, 0.25f, 15.6f);
    int c3 = NodoArboleda(t, ZONA_RIO, '-', -8.7f, 0.25f, 16.0f);
    int c4 = NodoArboleda(t, ZONA_RIO, '+', -4.3f, 0.25f, 16.2f);
    int c5 = NodoArboleda(t, ZONA_COSTA, 'R', 0.2f, 0.25f, 15.9f);
    int c6 = NodoArboleda(t, ZONA_COSTA, '-', 4.8f, 0.25f, 14.6f);
    int f1 = NodoArboleda(t, ZONA_COSTA, '+', 9.4f, 0.25f, 13.1f);
    int t1 = NodoArboleda(t, ZONA_COSTA, '+', 2.0f, 0.25f, 11.4f);

    // --- BOSQUE BAJO: junta del oeste y costa oeste con la cabana ---
    int w1 = NodoArboleda(t, ZONA_BOSQUE_BAJO, '+', -23.0f, 0.25f, 7.8f);
    int w2 = NodoArboleda(t, ZONA_BOSQUE_BAJO, '-', -25.2f, 0.25f, 2.6f);
    int w3 = NodoArboleda(t, ZONA_BOSQUE_BAJO, 'R', -24.2f, 0.25f, -2.2f);
    int w4 = NodoArboleda(t, ZONA_BOSQUE_BAJO, '+', -21.4f, 0.25f, -6.6f);
    int w5 = NodoArboleda(t, ZONA_BOSQUE_BAJO, 'I', -18.0f, 0.25f, -10.2f);

    // --- RIO: orilla norte y puente del norte ---
    int n1 = NodoArboleda(t, ZONA_RIO, '-', -13.9f, 0.25f, -13.2f);
    int n2 = NodoArboleda(t, ZONA_RIO, '+', -8.9f, 0.25f, -14.8f);
    int n3 = NodoArboleda(t, ZONA_RIO, 'V', -4.6f, 0.25f, -14.6f);

    // --- GRAN ROBLE: borde norte, circuito de la meseta y rampa ---
    int n4 = NodoArboleda(t, ZONA_CLARO_ROBLE, '+', -0.6f, 0.25f, -15.0f);
    int n5 = NodoArboleda(t, ZONA_CLARO_ROBLE, '-', 4.6f, 0.25f, -15.0f);

    const int NUM_ROBLE = 7;
    int ol[NUM_ROBLE];
    const char tiposRoble[NUM_ROBLE] = { 'n', '+', 'V', 'R', '+', 'C', '-' };

    for (int k = 0; k < NUM_ROBLE; k++)
    {
        float ang = (75.0f + 360.0f / NUM_ROBLE * k) * 0.0174533f;

        ol[k] =
            NodoArboleda(
                t, ZONA_CLARO_ROBLE, tiposRoble[k],
                ROBLE_X + std::cos(ang) * 5.0f,
                2.85f,
                ROBLE_Z + std::sin(ang) * 5.0f
            );
    }

    int rampa = NodoArboleda(t, ZONA_CLARO_ROBLE, '+', 3.2f, 1.0f, -19.0f);

    // --- RUINAS Y JARDIN: circuito central de 7 casillas ---
    const float jx = 7.5f;
    const float jz = 3.5f;
    int jar[7];
    const char tiposJardin[7] = { 'E', '-', '+', 'I', 'R', '+', 'D' };

    for (int k = 0; k < 7; k++)
    {
        float ang = (180.0f + 360.0f / 7.0f * k) * 0.0174533f;

        jar[k] =
            NodoArboleda(
                t, ZONA_RUINAS, tiposJardin[k],
                jx + std::cos(ang) * 5.5f,
                0.25f,
                jz + std::sin(ang) * 5.5f
            );
    }

    // Sendero del puente (oeste -> ruinas).
    int p1 = NodoArboleda(t, ZONA_BOSQUE_BAJO, '+', -19.0f, 0.25f, 6.6f);
    int p2 = NodoArboleda(t, ZONA_RIO, 'V', -14.8f, 0.25f, 5.8f);
    int bra = NodoArboleda(t, ZONA_RIO, '-', -10.5f, 0.25f, 5.4f);
    int brb = NodoArboleda(t, ZONA_RIO, '+', -6.3f, 0.25f, 5.4f);
    int p4 = NodoArboleda(t, ZONA_RIO, 'I', -2.2f, 0.25f, 4.7f);

    // Salida norte de las ruinas (hacia el borde del Gran Roble).
    int q1 = NodoArboleda(t, ZONA_RUINAS, '-', 4.4f, 0.25f, -5.4f);
    int q2 = NodoArboleda(t, ZONA_RUINAS, '+', 4.6f, 0.25f, -10.0f);

    // Lazo del este: arco, altiplano y bosque de pinos.
    int k1 = NodoArboleda(t, ZONA_RUINAS, 'T', 17.2f, 0.25f, 7.2f);
    int k2 = NodoArboleda(t, ZONA_RUINAS, '+', 21.2f, 0.60f, 8.4f);
    int se1 = NodoArboleda(t, ZONA_ALTIPLANO, '-', 25.6f, 1.20f, 9.6f);
    int e3 = NodoArboleda(t, ZONA_ALTIPLANO, '+', 28.2f, 2.00f, 5.6f);
    int e2 = NodoArboleda(t, ZONA_ALTIPLANO, 'E', 29.4f, 2.20f, 0.8f);
    int e1 = NodoArboleda(t, ZONA_ALTIPLANO, 'V', 28.8f, 1.30f, -4.0f);
    int j1 = NodoArboleda(t, ZONA_ALTIPLANO, 'D', 26.6f, 0.70f, -8.6f);
    int a3 = NodoArboleda(t, ZONA_ALTIPLANO, '-', 22.2f, 0.45f, -10.6f);
    int a2 = NodoArboleda(t, ZONA_ALTIPLANO, 'C', 18.2f, 0.30f, -12.4f);
    int a1 = NodoArboleda(t, ZONA_ALTIPLANO, '-', 14.0f, 0.25f, -12.8f);
    int r1 = NodoArboleda(t, ZONA_ALTIPLANO, '+', 12.4f, 0.25f, -8.8f);
    int u1 = NodoArboleda(t, ZONA_RUINAS, '-', 9.0f, 0.25f, -12.2f);
    int r2 = NodoArboleda(t, ZONA_ALTIPLANO, 'R', 12.8f, 0.25f, -4.4f);

    // --- Conexiones (opcion 0 primero) ---

    // Circuito de las ruinas. jar: 0=O (entrada) 1=NO 2=N 3=NE 4=E 5=SE 6=S
    t.ConectarCasillas(jar[0], jar[1]);

    // NO: seguir en el jardin o salir al norte.
    t.ConectarCasillas(jar[1], jar[2]);
    t.ConectarCasillas(jar[1], q1);

    t.ConectarCasillas(jar[2], jar[3]);
    t.ConectarCasillas(jar[3], jar[4]);

    // SE: seguir en el jardin o salir por el arco.
    t.ConectarCasillas(jar[4], jar[5]);
    t.ConectarCasillas(jar[4], k1);

    // S: seguir en el jardin o bajar a la costa.
    t.ConectarCasillas(jar[5], jar[6]);
    t.ConectarCasillas(jar[5], f1);

    t.ConectarCasillas(jar[6], jar[0]);

    // Lazo del norte: ruinas -> borde norte -> costa oeste -> junta.
    t.ConectarCasillas(q1, q2);
    t.ConectarCasillas(q2, n5);

    // Borde norte: seguir al oeste o volver a las ruinas por el este.
    t.ConectarCasillas(n5, n4);
    t.ConectarCasillas(n5, u1);
    t.ConectarCasillas(u1, r1);

    // Subir al Gran Roble.
    t.ConectarCasillas(n4, n3);
    t.ConectarCasillas(n4, ol[0]);

    t.ConectarCasillas(n3, n2);
    t.ConectarCasillas(n2, n1);
    t.ConectarCasillas(n1, w5);
    t.ConectarCasillas(w5, w4);
    t.ConectarCasillas(w4, w3);
    t.ConectarCasillas(w3, w2);
    t.ConectarCasillas(w2, w1);

    // Circuito del Gran Roble: seguir en el claro o bajar por la rampa.
    for (int k = 0; k < NUM_ROBLE - 1; k++)
    {
        t.ConectarCasillas(ol[k], ol[k + 1]);
    }

    t.ConectarCasillas(ol[NUM_ROBLE - 1], ol[0]);
    t.ConectarCasillas(ol[NUM_ROBLE - 1], rampa);
    t.ConectarCasillas(rampa, n5);

    // Lazo del sur: ruinas -> playa -> puerta -> junta.
    t.ConectarCasillas(f1, c6);

    // Playa: seguir hacia el muelle o volver a las ruinas.
    t.ConectarCasillas(c6, c5);
    t.ConectarCasillas(c5, c4);
    t.ConectarCasillas(c5, t1);
    t.ConectarCasillas(t1, jar[6]);

    t.ConectarCasillas(c4, c3);
    t.ConectarCasillas(c3, c2);
    t.ConectarCasillas(c2, c1);
    t.ConectarCasillas(c1, s0);
    t.ConectarCasillas(s0, w1);

    // Junta del oeste -> sendero del puente -> ruinas.
    t.ConectarCasillas(w1, p1);
    t.ConectarCasillas(p1, p2);
    t.ConectarCasillas(p2, bra);
    t.ConectarCasillas(bra, brb);
    t.ConectarCasillas(brb, p4);
    t.ConectarCasillas(p4, jar[0]);

    // Lazo del este: arco -> altiplano -> pinos -> ruinas.
    t.ConectarCasillas(k1, k2);
    t.ConectarCasillas(k2, se1);
    t.ConectarCasillas(se1, e3);
    t.ConectarCasillas(e3, e2);
    t.ConectarCasillas(e2, e1);
    t.ConectarCasillas(e1, j1);
    t.ConectarCasillas(j1, a3);
    t.ConectarCasillas(a3, a2);
    t.ConectarCasillas(a2, a1);
    t.ConectarCasillas(a1, r1);
    t.ConectarCasillas(r1, r2);
    t.ConectarCasillas(r2, jar[2]);

    // --- Puentes (rio) ---
    puentesArboleda[cantidadPuentesArboleda][0] = n2;
    puentesArboleda[cantidadPuentesArboleda++][1] = n3;
    puentesArboleda[cantidadPuentesArboleda][0] = bra;
    puentesArboleda[cantidadPuentesArboleda++][1] = brb;
    puentesArboleda[cantidadPuentesArboleda][0] = c4;
    puentesArboleda[cantidadPuentesArboleda++][1] = c3;

    // --- Textos de las bifurcaciones ---
    DefinirOpcionesArboleda(jar[1], "SEGUIR EL JARDIN", "SALIR AL NORTE");
    DefinirOpcionesArboleda(jar[4], "SEGUIR EL JARDIN", "POR EL ARCO");
    DefinirOpcionesArboleda(jar[5], "SEGUIR EL JARDIN", "BAJAR A LA COSTA");
    DefinirOpcionesArboleda(n5, "SEGUIR EL BORDE", "VOLVER A LAS RUINAS");
    DefinirOpcionesArboleda(n4, "SEGUIR EL BORDE", "SUBIR AL GRAN ROBLE");
    DefinirOpcionesArboleda(ol[NUM_ROBLE - 1], "SEGUIR EN EL CLARO", "BAJAR AL CAMINO");
    DefinirOpcionesArboleda(c5, "SEGUIR POR LA PLAYA", "VOLVER A LAS RUINAS");

    // --- Candidatas del trofeo: centrales y repartidas por las zonas ---
    const int candidatas[] =
    {
        c4, c5, f1, w3, w5, n3, n5, ol[2], jar[2], jar[4], jar[6], se1, e1, r1
    };

    for (int indice : candidatas)
    {
        casillasTrofeoArboleda[cantidadTrofeosArboleda++] = indice;
    }

    casillaInicioArboleda = s0;

    t.ValidarRecorrido();
}


static const Tablero& TableroReferencia()
{
    static Tablero referencia;
    static bool lista = false;

    if (!lista)
    {
        ConstruirRutaArboleda(referencia);
        lista = true;
    }

    return referencia;
}


// Distancia al sendero mas cercano (segmentos entre casillas conectadas).
// Si se pide, devuelve tambien la altura del suelo del camino ahi.
static float DistanciaSendero(
    float x,
    float z,
    float* alturaCamino = nullptr
)
{
    const Tablero& t = TableroReferencia();
    float mejor = 1000.0f;
    float alturaMejor = 0.0f;

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

            float dx = b->posicion.x - a.posicion.x;
            float dz = b->posicion.z - a.posicion.z;
            float largo2 = dx * dx + dz * dz;

            float k =
                largo2 > 0.0001f
                ? ((x - a.posicion.x) * dx + (z - a.posicion.z) * dz) / largo2
                : 0.0f;

            k = Limitar01(k);

            float cx = a.posicion.x + dx * k - x;
            float cz = a.posicion.z + dz * k - z;
            float d2 = cx * cx + cz * cz;

            if (d2 < mejor * mejor)
            {
                mejor = std::sqrt(d2);
                alturaMejor =
                    a.posicion.y + (b->posicion.y - a.posicion.y) * k - 0.25f;
            }
        }
    }

    if (alturaCamino != nullptr)
    {
        *alturaCamino = alturaMejor;
    }

    return mejor;
}


// Eje del rio: nace al pie de la meseta del Gran Roble (cascada) y baja
// al mar pasando entre las casillas de los tres puentes.
struct PuntoRio
{
    float z;
    float x;
};

static const PuntoRio PUNTOS_RIO[] =
{
    { -17.4f, -6.7f },
    { -14.7f, -6.7f },
    { -10.0f, -7.5f },
    {  -5.0f, -9.0f },
    {  -1.5f, -10.5f },
    {   2.0f, -9.7f },
    {   5.4f, -8.55f },
    {  10.0f, -7.6f },
    {  16.1f, -6.5f },
    {  22.0f, -6.3f },
    {  32.0f, -6.3f }
};

static const int CANTIDAD_PUNTOS_RIO =
    (int)(sizeof(PUNTOS_RIO) / sizeof(PUNTOS_RIO[0]));


static float RioX(float z)
{
    if (z <= PUNTOS_RIO[0].z)
    {
        return PUNTOS_RIO[0].x;
    }

    for (int i = 1; i < CANTIDAD_PUNTOS_RIO; i++)
    {
        if (z <= PUNTOS_RIO[i].z)
        {
            float t =
                (z - PUNTOS_RIO[i - 1].z) /
                (PUNTOS_RIO[i].z - PUNTOS_RIO[i - 1].z);

            return PUNTOS_RIO[i - 1].x +
                (PUNTOS_RIO[i].x - PUNTOS_RIO[i - 1].x) * t;
        }
    }

    return PUNTOS_RIO[CANTIDAD_PUNTOS_RIO - 1].x;
}


// Semiancho del cauce: estrecho en los puentes y ancho en la laguna.
static float RioSemiancho(float z)
{
    float dz = z + 1.5f;
    float laguna = std::exp(-(dz * dz) / (2.0f * 1.8f * 1.8f));

    return 0.72f + 2.0f * laguna;
}


static float DistanciaRio(float x, float z)
{
    float dx = x - RioX(z);

    if (z < PUNTOS_RIO[0].z)
    {
        float dz = z - PUNTOS_RIO[0].z;
        return std::sqrt(dx * dx + dz * dz);
    }

    return std::fabs(dx);
}


//==================================================
// ALTURA DEL TERRENO
//==================================================

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

    // Dos escalones suaves: terrazas.
    return altura * 0.5f *
        (Suave01((t - 0.12f) / 0.14f) + Suave01((t - 0.55f) / 0.14f));
}


static float AlturaTerreno(float x, float z)
{
    float ex = (x - 1.5f) / 42.0f;
    float ez = (z + 9.0f) / 34.0f;

    float ang = std::atan2(ez, ex);

    float radio =
        1.0f +
        0.06f * std::sin(3.0f * ang + 1.0f) +
        0.04f * std::sin(5.0f * ang + 2.3f) +
        0.02f * std::sin(9.0f * ang);

    float d = std::sqrt(ex * ex + ez * ez) / radio;

    // Norte y este: acantilados. Sur y oeste: playas.
    float acantilado =
        Suave01((std::cos(ang + 0.9f) + 0.15f));

    float inicioCosta = 0.80f + 0.13f * acantilado;

    float elevacion =
        acantilado * 1.9f *
        Suave01((d - 0.62f) / 0.22f) *
        (1.0f - Suave01((d - 0.88f) / 0.10f));

    float costa =
        -0.20f * Suave01((d - inicioCosta) / 0.14f) -
        1.05f * Suave01(
            (d - (inicioCosta + 0.14f)) / (0.10f - 0.04f * acantilado)
        );

    float h = elevacion + costa;

    h += Meseta(x, z, ROBLE_X, ROBLE_Z, 6.8f, 10.5f, 2.6f);
    h += Meseta(x, z, 32.0f, -21.0f, 3.0f, 9.0f, 2.2f);
    h += Meseta(x, z, -31.0f, -15.0f, 3.0f, 8.0f, 1.6f);
    h += Meseta(x, z, -31.0f, 13.0f, 2.0f, 6.0f, 1.0f);

    if (h > -0.05f)
    {
        h += 0.07f * std::sin(x * 1.3f + z * 0.7f) *
            std::sin(z * 1.1f - x * 0.4f);
    }

    // Los senderos quedan planos.
    float suelo = 0.0f;
    float dp = DistanciaSendero(x, z, &suelo);
    h = suelo + (h - suelo) * Suave01((dp - 2.0f) / 1.8f);

    // Cauce del rio.
    float rd = DistanciaRio(x, z);
    float hw = RioSemiancho(z);

    if (rd < hw + 0.7f)
    {
        float t = Suave01((rd - hw) / 0.7f);
        h = -0.9f * (1.0f - t) + h * t;
    }

    if (h < -1.25f)
    {
        h = -1.25f;
    }

    return h;
}


//==================================================
// MALLA DEL TERRENO (se construye una vez)
//==================================================

static const float TERRENO_X0 = -44.0f;
static const float TERRENO_Z0 = -48.0f;
static const float TERRENO_PASO = 0.8f;
static const int TERRENO_NX = 113;
static const int TERRENO_NZ = 96;

struct TrianguloTerreno
{
    Vector3 v[3];
    Color normal;
    Color floracion;
};

struct CeldaEspuma
{
    float x;
    float z;
};

static const int MAX_TRIANGULOS_TERRENO =
    (TERRENO_NX - 1) * (TERRENO_NZ - 1) * 2;

static const int MAX_CELDAS_ESPUMA = 4000;

static TrianguloTerreno triangulosTerreno[MAX_TRIANGULOS_TERRENO];
static int cantidadTriangulosTerreno = 0;

static CeldaEspuma celdasEspuma[MAX_CELDAS_ESPUMA];
static int cantidadCeldasEspuma = 0;

static bool terrenoConstruido = false;


static Color ColorBaseTerreno(
    float cx, float cz,
    float h, float ny,
    int ix, int iz,
    bool floracion
)
{
    float n = Hash2(ix, iz);
    Color c;

    const Color mar = Color{ 34, 84, 108, 255 };
    const Color arenaHumeda = Color{ 196, 184, 138, 255 };
    const Color arena = Color{ 228, 212, 158, 255 };

    if (h < -0.75f)
    {
        c = mar;
    }
    else if (h < NIVEL_AGUA)
    {
        c = Mezcla(mar, arenaHumeda, Suave01((h + 0.75f) / 0.45f));
    }
    else if (h < -0.06f)
    {
        c = Mezcla(arenaHumeda, arena, Suave01((h - NIVEL_AGUA) / 0.24f));
    }
    else
    {
        Color pasto =
            Mezcla(
                Color{ 84, 152, 70, 255 },
                Color{ 112, 180, 82, 255 },
                n * 0.7f + 0.3f * (0.5f + 0.5f * std::sin(cx * 0.4f + cz * 0.3f))
            );

        c = Mezcla(arena, pasto, Suave01((h + 0.06f) / 0.06f));

        if (h > 1.3f)
        {
            c = Mezcla(c, Color{ 120, 150, 84, 255 }, Suave01((h - 1.3f) / 1.4f));
        }

        // Claros: centro (circulo de piedras) y pradera del oeste.
        float claro1 = 1.0f - Limitar01(
            std::sqrt((cx - 7.5f) * (cx - 7.5f) + (cz - 3.5f) * (cz - 3.5f)) / 7.5f
        );

        float claro2 = 1.0f - Limitar01(
            std::sqrt((cx + 29.0f) * (cx + 29.0f) + (cz + 3.0f) * (cz + 3.0f)) / 5.5f
        );

        float claro3 = 1.0f - Limitar01(
            std::sqrt((cx - ROBLE_X) * (cx - ROBLE_X) + (cz - ROBLE_Z) * (cz - ROBLE_Z)) / 8.0f
        );

        float claro = claro1 > claro2 ? claro1 : claro2;
        if (claro3 > claro) claro = claro3;

        c = Mezcla(c, Color{ 128, 196, 94, 255 }, Suave01(claro) * 0.65f);

        // Bosque profundo: verde mas oscuro.
        float profundo = 1.0f - Limitar01(
            std::sqrt((cx - 20.0f) * (cx - 20.0f) + (cz + 20.0f) * (cz + 20.0f)) / 14.0f
        );

        c = Mezcla(c, Color{ 46, 104, 62, 255 }, Suave01(profundo) * 0.55f);

        float dp = DistanciaSendero(cx, cz);

        if (dp < 3.6f)
        {
            // Pasto pisado alrededor del camino.
            c = Mezcla(c, Color{ 150, 170, 88, 255 }, 0.42f * (1.0f - Suave01((dp - 1.8f) / 1.8f)));
        }

        float rd = DistanciaRio(cx, cz);
        float hw = RioSemiancho(cz);

        if (rd < hw + 1.0f && h > NIVEL_AGUA)
        {
            c = Mezcla(c, Color{ 98, 120, 72, 255 }, 0.5f * (1.0f - (rd - hw) / 1.0f));
        }

        if (floracion && h > 0.0f)
        {
            c = Mezcla(c, Color{ 214, 200, 132, 255 }, 0.16f);

            if (n > 0.82f)
            {
                c = Mezcla(c, Color{ 238, 150, 184, 255 }, 0.78f);
            }
            else if (n > 0.72f)
            {
                c = Mezcla(c, Color{ 250, 236, 222, 255 }, 0.62f);
            }
        }
    }

    // Pendientes fuertes: roca.
    if (ny < 0.90f && h > NIVEL_AGUA)
    {
        Color roca =
            Mezcla(
                Color{ 150, 144, 136, 255 },
                Color{ 104, 100, 98, 255 },
                (0.86f - ny) / 0.3f
            );

        c = Mezcla(c, roca, Suave01((0.90f - ny) / 0.18f));
    }

    return c;
}


// Alturas de la rejilla del terreno (las rellena ConstruirTerreno).
static float alturasTerreno[TERRENO_NX][TERRENO_NZ];


// Altura del terreno dibujado (interpolacion sobre la rejilla). Mucho mas
// barata que AlturaTerreno y suficiente para particulas que caen al suelo.
static float AlturaTerrenoMalla(float x, float z)
{
    float fx = (x - TERRENO_X0) / TERRENO_PASO;
    float fz = (z - TERRENO_Z0) / TERRENO_PASO;

    int ix = (int)std::floor(fx);
    int iz = (int)std::floor(fz);

    if (ix < 0 || iz < 0 || ix >= TERRENO_NX - 1 || iz >= TERRENO_NZ - 1)
    {
        return AlturaTerreno(x, z);
    }

    float tx = fx - ix;
    float tz = fz - iz;

    float a = alturasTerreno[ix][iz] + (alturasTerreno[ix + 1][iz] - alturasTerreno[ix][iz]) * tx;
    float b = alturasTerreno[ix][iz + 1] + (alturasTerreno[ix + 1][iz + 1] - alturasTerreno[ix][iz + 1]) * tx;

    return a + (b - a) * tz;
}



static void ConstruirTerreno()
{


    for (int ix = 0; ix < TERRENO_NX; ix++)
    {
        for (int iz = 0; iz < TERRENO_NZ; iz++)
        {
            alturasTerreno[ix][iz] =
                AlturaTerreno(
                    TERRENO_X0 + ix * TERRENO_PASO,
                    TERRENO_Z0 + iz * TERRENO_PASO
                );
        }
    }

    cantidadTriangulosTerreno = 0;
    cantidadCeldasEspuma = 0;

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

            float minimo = p00.y;
            float maximo = p00.y;
            const Vector3* esquinas[3] = { &p01, &p10, &p11 };

            for (const Vector3* e : esquinas)
            {
                if (e->y < minimo) minimo = e->y;
                if (e->y > maximo) maximo = e->y;
            }

            if (
                minimo <= NIVEL_AGUA &&
                maximo >= NIVEL_AGUA &&
                cantidadCeldasEspuma < MAX_CELDAS_ESPUMA
            )
            {
                celdasEspuma[cantidadCeldasEspuma++] =
                    { (x0 + x1) * 0.5f, (z0 + z1) * 0.5f };
            }

            Vector3 tri[2][3] =
            {
                { p00, p01, p10 },
                { p10, p01, p11 }
            };

            for (int k = 0; k < 2; k++)
            {
                TrianguloTerreno& t = triangulosTerreno[cantidadTriangulosTerreno++];

                for (int i = 0; i < 3; i++)
                {
                    t.v[i] = tri[k][i];
                }

                // Normal del triangulo (apuntando hacia arriba).
                Vector3 u = { t.v[1].x - t.v[0].x, t.v[1].y - t.v[0].y, t.v[1].z - t.v[0].z };
                Vector3 w = { t.v[2].x - t.v[0].x, t.v[2].y - t.v[0].y, t.v[2].z - t.v[0].z };

                Vector3 n =
                {
                    u.y * w.z - u.z * w.y,
                    u.z * w.x - u.x * w.z,
                    u.x * w.y - u.y * w.x
                };

                float largo = std::sqrt(n.x * n.x + n.y * n.y + n.z * n.z);

                if (largo > 0.0001f)
                {
                    n.x /= largo; n.y /= largo; n.z /= largo;
                }

                if (n.y < 0.0f)
                {
                    n.x = -n.x; n.y = -n.y; n.z = -n.z;
                }

                float cx = (t.v[0].x + t.v[1].x + t.v[2].x) / 3.0f;
                float cz = (t.v[0].z + t.v[1].z + t.v[2].z) / 3.0f;
                float h = (t.v[0].y + t.v[1].y + t.v[2].y) / 3.0f;

                // Luz fija desde arriba-izquierda-frente, horneada.
                float luz = -0.45f * n.x + 0.80f * n.y + 0.40f * n.z;
                if (luz < 0.0f) luz = 0.0f;
                float sombra = 0.66f + 0.46f * luz;

                int idx = ix + k;
                int idz = iz + k;

                t.normal = Oscurecer(
                    ColorBaseTerreno(cx, cz, h, n.y, idx, idz, false), sombra);

                t.floracion = Oscurecer(
                    ColorBaseTerreno(cx, cz, h, n.y, idx, idz, true), sombra * 1.04f);
            }
        }
    }

    terrenoConstruido = true;
}


// El terreno es estatico: su geometria se sube una vez a la GPU y solo se
// reescriben los colores cuando cambia la floracion (cuantizada).
static ConstructorMalla constructorTerreno;
static MallaGpu mallaTerreno;
static int pasoFloracionTerreno = -1;

static const int PASOS_FLORACION = 32;


static void LlenarColoresTerreno(float floracion)
{
    constructorTerreno.colores.clear();

    for (int i = 0; i < cantidadTriangulosTerreno; i++)
    {
        const TrianguloTerreno& t = triangulosTerreno[i];
        Color c = Mezcla(t.normal, t.floracion, floracion);

        for (int v = 0; v < 3; v++)
        {
            constructorTerreno.colores.push_back(c.r);
            constructorTerreno.colores.push_back(c.g);
            constructorTerreno.colores.push_back(c.b);
            constructorTerreno.colores.push_back(255);
        }
    }
}


static void DibujarTerreno(float floracion)
{
    if (!terrenoConstruido)
    {
        ConstruirTerreno();
    }

    int paso = (int)(floracion * PASOS_FLORACION + 0.5f);

    if (!mallaTerreno.cargada)
    {
        constructorTerreno.Limpiar();

        for (int i = 0; i < cantidadTriangulosTerreno; i++)
        {
            const TrianguloTerreno& t = triangulosTerreno[i];

            for (int v = 0; v < 3; v++)
            {
                constructorTerreno.posiciones.push_back(t.v[v].x);
                constructorTerreno.posiciones.push_back(t.v[v].y);
                constructorTerreno.posiciones.push_back(t.v[v].z);
            }
        }

        LlenarColoresTerreno((float)paso / PASOS_FLORACION);
        ActualizarMallaGpu(mallaTerreno, constructorTerreno);
        pasoFloracionTerreno = paso;
    }
    else if (paso != pasoFloracionTerreno)
    {
        LlenarColoresTerreno((float)paso / PASOS_FLORACION);
        ActualizarColoresMallaGpu(mallaTerreno, constructorTerreno);
        pasoFloracionTerreno = paso;
    }

    rlDisableBackfaceCulling();
    DibujarMallaGpu(mallaTerreno);
    rlEnableBackfaceCulling();
}


//==================================================
// AGUA
//==================================================

// Cuadrilatero horizontal (dos caras, sin depender del sentido).
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


static void DibujarAgua(float tiempo, float floracion)
{
    // Fondo marino profundo y superficie translucida.
    DrawPlane(
        Vector3{ 0.0f, -1.30f, 0.0f },
        Vector2{ 400.0f, 400.0f },
        Color{ 26, 70, 98, 255 }
    );

    Color superficie =
        Mezcla(Color{ 56, 168, 204, 255 }, Color{ 96, 176, 204, 255 }, floracion * 0.5f);

    superficie.a = 150;

    rlDisableBackfaceCulling();

    CuadroHorizontal(0.0f, NIVEL_AGUA, 0.0f, 200.0f, 200.0f, superficie);

    // Franjas de oleaje que avanzan hacia la costa.
    NivelCalidadGrafica efectos = CalidadEfectos();
    int pasoOleaje = efectos == CALIDAD_ALTA ? 3 : 4;

    for (int gx = -24; efectos != CALIDAD_BAJA && gx <= 24; gx += pasoOleaje)
    {
        for (int gz = -24; gz <= 24; gz += pasoOleaje)
        {
            float fase = tiempo * 0.9f + gx * 0.55f + gz * 0.8f;
            float onda = 0.5f + 0.5f * std::sin(fase);

            if (onda < 0.35f)
            {
                continue;
            }

            float desplazamiento = 1.2f * std::sin(tiempo * 0.5f + gz * 0.7f);

            CuadroHorizontal(
                gx + desplazamiento,
                NIVEL_AGUA + 0.012f,
                gz + 0.6f * std::sin(gx * 0.9f),
                0.95f,
                0.14f,
                Color{ 255, 255, 255, (unsigned char)(60.0f * onda) }
            );
        }
    }

    // Espuma en la orilla (costa y rio).
    // Espuma: en calidad media/baja solo una de cada 2/3 celdas.
    int pasoEspuma = efectos == CALIDAD_ALTA ? 1 : (efectos == CALIDAD_MEDIA ? 2 : 3);

    for (int i = 0; i < cantidadCeldasEspuma; i += pasoEspuma)
    {
        const CeldaEspuma& e = celdasEspuma[i];
        float onda = 0.5f + 0.5f * std::sin(tiempo * 1.7f + e.x * 0.8f + e.z * 0.6f);

        CuadroHorizontal(
            e.x,
            NIVEL_AGUA + 0.02f,
            e.z,
            0.27f,
            0.27f,
            Color{ 255, 255, 255, (unsigned char)(70.0f + 90.0f * onda) }
        );
    }

    rlEnableBackfaceCulling();
}


//==================================================
// PIEZAS BASICAS DE ESCENOGRAFIA
//==================================================

static const Color CORTEZA = Color{ 98, 68, 46, 255 };
static const Color CORTEZA_OSCURA = Color{ 72, 50, 36, 255 };


static void DibujarRoca(
    float x, float y, float z,
    float radio, float giro,
    Color color
)
{
    rlPushMatrix();
    rlTranslatef(x, y, z);
    rlRotatef(giro, 0.0f, 1.0f, 0.0f);
    rlScalef(1.15f, 0.72f, 0.9f);

    DrawSphereEx(Vector3{ 0.0f, radio * 0.45f, 0.0f }, radio, 5, 6, color);

    DrawSphereEx(
        Vector3{ radio * 0.25f, radio * 0.95f, -radio * 0.1f },
        radio * 0.55f, 4, 5,
        Mezcla(color, WHITE, 0.18f)
    );

    rlPopMatrix();
}


static void DibujarPino(
    ConstructorMalla& m,
    float x, float y, float z,
    float escala, float variacion
)
{
    m.Cilindro(
        Vector3{ x, y, z },
        Vector3{ x, y + 0.9f * escala, z },
        0.15f * escala, 0.19f * escala, 6, CORTEZA
    );

    const Color colores[3] =
    {
        Color{ 30, 92, 60, 255 },
        Color{ 40, 112, 68, 255 },
        Color{ 56, 136, 78, 255 }
    };

    for (int k = 0; k < 3; k++)
    {
        float base = y + (0.55f + k * 0.78f) * escala;
        float radio = (1.05f - 0.24f * k) * escala;

        m.Cilindro(
            Vector3{ x, base, z },
            Vector3{ x, base + (1.25f - 0.15f * k) * escala, z },
            radio, 0.0f, 7,
            Oscurecer(colores[k], 0.9f + 0.2f * variacion)
        );
    }
}


static void DibujarRoble(
    ConstructorMalla& m,
    ConstructorMalla& copas,
    float x, float y, float z,
    float escala, float variacion,
    bool floral, float floracion
)
{
    m.Cilindro(
        Vector3{ x, y, z },
        Vector3{ x, y + 1.3f * escala, z },
        0.20f * escala, 0.28f * escala, 7, CORTEZA
    );

    // El vaiven de la copa lo aplica el desplazamiento de la malla de su fase.
    Color oscuro = Oscurecer(Color{ 40, 104, 56, 255 }, 0.9f + 0.2f * variacion);
    Color medio = Oscurecer(Color{ 64, 138, 68, 255 }, 0.9f + 0.2f * variacion);
    Color claro = Color{ 94, 170, 82, 255 };

    if (floral)
    {
        oscuro = Mezcla(oscuro, Color{ 190, 96, 140, 255 }, floracion);
        medio = Mezcla(medio, Color{ 232, 134, 172, 255 }, floracion);
        claro = Mezcla(claro, Color{ 255, 190, 212, 255 }, floracion);
    }

    float crece = 1.0f + (floral ? 0.18f * floracion : 0.0f);

    copas.Esfera(Vector3{ x, y + 1.55f * escala, z }, 0.95f * escala * crece, 6, 7, oscuro);
    copas.Esfera(Vector3{ x + 0.55f * escala, y + 1.4f * escala, z + 0.2f * escala }, 0.68f * escala * crece, 5, 6, medio);
    copas.Esfera(Vector3{ x - 0.5f * escala, y + 1.45f * escala, z - 0.2f * escala }, 0.64f * escala * crece, 5, 6, medio);
    copas.Esfera(Vector3{ x - 0.15f * escala, y + 2.05f * escala, z + 0.1f * escala }, 0.62f * escala * crece, 5, 6, claro);
}


static void DibujarArbusto(
    ConstructorMalla& m,
    ConstructorMalla& flores,
    float x, float y, float z,
    float escala, float variacion,
    float floracion
)
{
    Color verde = Oscurecer(Color{ 62, 130, 66, 255 }, 0.9f + 0.25f * variacion);

    m.Esfera(Vector3{ x, y + 0.26f * escala, z }, 0.38f * escala, 5, 6, verde);
    m.Esfera(
        Vector3{ x + 0.25f * escala, y + 0.2f * escala, z + 0.1f * escala },
        0.28f * escala, 4, 5,
        Mezcla(verde, WHITE, 0.12f)
    );

    // Algunos arbustos echan flores en la floracion.
    if (variacion > 0.55f && floracion > 0.05f)
    {
        flores.Esfera(
            Vector3{ x - 0.12f * escala, y + 0.55f * escala, z + 0.1f * escala },
            0.10f * escala * floracion + 0.02f, 3, 4,
            Color{ 255, 170, 200, 255 }
        );

        flores.Esfera(
            Vector3{ x + 0.2f * escala, y + 0.5f * escala, z - 0.1f * escala },
            0.09f * escala * floracion + 0.02f, 3, 4,
            Color{ 255, 232, 240, 255 }
        );
    }
}


static void DibujarFlor(
    ConstructorMalla& m,
    ConstructorMalla& cabezas,
    float x, float y, float z,
    float variacion,
    float floracion
)
{
    const Color petalos[5] =
    {
        Color{ 255, 140, 180, 255 },
        Color{ 250, 244, 236, 255 },
        Color{ 255, 214, 70, 255 },
        Color{ 176, 120, 235, 255 },
        Color{ 240, 90, 90, 255 }
    };

    int tipo = (int)(variacion * 4.99f);

    // En floracion las flores se abren mas y se ven mas grandes.
    float apertura = 0.55f + 0.7f * floracion;

    m.Cilindro(
        Vector3{ x, y, z },
        Vector3{ x, y + 0.26f, z },
        0.015f, 0.015f, 4,
        Color{ 70, 140, 66, 255 }
    );

    cabezas.Esfera(
        Vector3{ x, y + 0.28f, z },
        0.07f * apertura, 3, 4,
        petalos[tipo]
    );

    cabezas.Esfera(
        Vector3{ x, y + 0.31f * apertura + 0.0f, z },
        0.03f, 3, 3,
        Color{ 255, 220, 90, 255 }
    );
}


static void DibujarSeta(
    ConstructorMalla& m,
    float x, float y, float z,
    float escala, float variacion
)
{
    m.Cilindro(
        Vector3{ x, y, z },
        Vector3{ x, y + 0.22f * escala, z },
        0.05f * escala, 0.07f * escala, 5,
        Color{ 238, 228, 205, 255 }
    );

    Color gorro =
        variacion > 0.5f
        ? Color{ 214, 64, 58, 255 }
        : Color{ 230, 160, 70, 255 };

    m.Cilindro(
        Vector3{ x, y + 0.2f * escala, z },
        Vector3{ x, y + 0.40f * escala, z },
        0.22f * escala, 0.0f, 7,
        gorro
    );

    m.Esfera(
        Vector3{ x + 0.06f * escala, y + 0.33f * escala, z + 0.08f * escala },
        0.035f * escala, 3, 3, Color{ 255, 250, 240, 255 }
    );
}


//==================================================
// VEGETACION (colocada una sola vez)
//==================================================

enum TipoFlora
{
    FLORA_PINO = 0,
    FLORA_ROBLE,
    FLORA_ROBLE_FLORAL,
    FLORA_ARBUSTO,
    FLORA_FLOR,
    FLORA_SETA,
    FLORA_ROCA
};

struct ElementoFlora
{
    TipoFlora tipo;
    float x;
    float y;
    float z;
    float escala;
    float variacion;
    float giro;
};

static const int MAX_FLORA = 2600;

static ElementoFlora flora[MAX_FLORA];
static int cantidadFlora = 0;
static bool floraConstruida = false;

static unsigned int semillaFlora = 12345u;


static float AleatorioFlora()
{
    semillaFlora = semillaFlora * 1664525u + 1013904223u;
    return (float)((semillaFlora >> 8) & 0xFFFF) / 65535.0f;
}


// Lugares reservados para landmarks y claros (no se planta ahi).
struct ZonaReservada
{
    float x;
    float z;
    float radio;
};

static const ZonaReservada ZONAS_RESERVADAS[] =
{
    { 7.5f, 3.5f, 4.4f },       // ruinas del jardin
    { 17.0f, 9.5f, 3.2f },      // arco ruinoso
    { -14.0f, 21.0f, 4.2f },    // muelle
    { -20.0f, 13.5f, 3.4f },    // puerta de la costa
    { -29.5f, -3.0f, 3.4f },    // cabana
    { -4.8f, -7.0f, 3.6f },     // molino
    { 33.0f, -3.0f, 3.6f },     // atalaya
    { 9.5f, -23.0f, 3.0f },     // setas gigantes
    { 13.5f, -27.5f, 3.0f },
    { 9.0f, -30.5f, 3.0f }
};


static bool Despejado(float x, float z, float radioArbol)
{
    for (const ZonaReservada& r : ZONAS_RESERVADAS)
    {
        float dx = x - r.x;
        float dz = z - r.z;
        float lim = r.radio + radioArbol;

        if (dx * dx + dz * dz < lim * lim)
        {
            return false;
        }
    }

    // Gran Roble y su meseta alta (solo arboles pequenos fuera del claro).
    float dx = x - ROBLE_X;
    float dz = z - ROBLE_Z;
    float lim = 7.5f + radioArbol;

    return dx * dx + dz * dz > lim * lim;
}


// Un arbol delante de una casilla (hacia la camara) la taparia.
static bool OcultaSendero(float x, float z)
{
    const Tablero& t = TableroReferencia();

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


static bool LibreDeArboles(float x, float z, float distancia)
{
    for (int i = 0; i < cantidadFlora; i++)
    {
        const ElementoFlora& e = flora[i];

        if (
            e.tipo != FLORA_PINO &&
            e.tipo != FLORA_ROBLE &&
            e.tipo != FLORA_ROBLE_FLORAL
        )
        {
            continue;
        }

        float dx = e.x - x;
        float dz = e.z - z;

        if (dx * dx + dz * dz < distancia * distancia)
        {
            return false;
        }
    }

    return true;
}


static void AgregarFlora(
    TipoFlora tipo,
    float x, float z,
    float escala
)
{
    if (cantidadFlora >= MAX_FLORA)
    {
        return;
    }

    ElementoFlora& e = flora[cantidadFlora++];

    e.tipo = tipo;
    e.x = x;
    e.y = AlturaTerreno(x, z) - 0.04f;
    e.z = z;
    e.escala = escala;
    e.variacion = AleatorioFlora();
    e.giro = AleatorioFlora() * 360.0f;
}


// La isla es un diorama: lo que queda lejos de cualquier sendero no se
// decora (o casi), asi la vegetacion se concentra donde se juega.
static bool FueraDelAreaJugable(float x, float z, float margen)
{
    return DistanciaSendero(x, z) > margen;
}


static float DistanciaPunto(float ax, float az, float bx, float bz)
{
    float dx = ax - bx;
    float dz = az - bz;

    return std::sqrt(dx * dx + dz * dz);
}


static void ConstruirFlora()
{
    semillaFlora = 12345u;
    cantidadFlora = 0;

    const float X0 = -38.0f;
    const float ANCHO = 80.0f;
    const float Z0 = -42.0f;
    const float LARGO = 66.0f;

    // Pinos: tierras altas, acantilados y bosque profundo (denso).
    for (int intento = 0, n = 0; intento < 30000 && n < 190; intento++)
    {
        float x = X0 + AleatorioFlora() * ANCHO;
        float z = Z0 + AleatorioFlora() * LARGO;

        bool profundo = DistanciaPunto(x, z, 20.0f, -20.0f) < 14.0f;
        float h = AlturaTerreno(x, z);

        bool alto = h > 0.9f || (x > 14.0f && h > 0.05f) || profundo;

        if (
            !alto ||
            h < 0.0f ||
            FueraDelAreaJugable(x, z, 15.0f) ||
            DistanciaSendero(x, z) < 2.4f ||
            DistanciaRio(x, z) < RioSemiancho(z) + 1.0f ||
            !Despejado(x, z, 0.8f) ||
            OcultaSendero(x, z) ||
            !LibreDeArboles(x, z, profundo ? 1.5f : 1.9f)
        )
        {
            continue;
        }

        AgregarFlora(FLORA_PINO, x, z, 0.85f + AleatorioFlora() * 0.6f);
        n++;
    }

    // Robles (algunos florales) por las tierras medias y el bosque bajo.
    for (int intento = 0, n = 0; intento < 30000 && n < 120; intento++)
    {
        float x = X0 + AleatorioFlora() * ANCHO;
        float z = Z0 + AleatorioFlora() * LARGO;
        float h = AlturaTerreno(x, z);

        if (
            h < -0.01f || h > 1.2f ||
            FueraDelAreaJugable(x, z, 15.0f) ||
            DistanciaSendero(x, z) < 2.8f ||
            DistanciaRio(x, z) < RioSemiancho(z) + 1.2f ||
            !Despejado(x, z, 1.3f) ||
            OcultaSendero(x, z) ||
            !LibreDeArboles(x, z, 2.4f)
        )
        {
            continue;
        }

        bool floral = AleatorioFlora() < 0.6f;

        AgregarFlora(
            floral ? FLORA_ROBLE_FLORAL : FLORA_ROBLE,
            x, z,
            0.95f + AleatorioFlora() * 0.5f
        );

        n++;
    }

    // Arbustos.
    for (int intento = 0, n = 0; intento < 20000 && n < 200; intento++)
    {
        float x = X0 + AleatorioFlora() * ANCHO;
        float z = Z0 + AleatorioFlora() * LARGO;

        if (
            AlturaTerreno(x, z) < -0.01f ||
            FueraDelAreaJugable(x, z, 11.0f) ||
            DistanciaSendero(x, z) < 2.1f ||
            DistanciaRio(x, z) < RioSemiancho(z) + 0.8f ||
            !Despejado(x, z, 0.3f)
        )
        {
            continue;
        }

        AgregarFlora(FLORA_ARBUSTO, x, z, 0.8f + AleatorioFlora() * 0.7f);
        n++;
    }

    // Flores en grupos: jardin de las ruinas, pradera del oeste y resto.
    for (int intento = 0, n = 0; intento < 30000 && n < 160; intento++)
    {
        float cx;
        float cz;

        if (n < 60)
        {
            float ang = AleatorioFlora() * 6.283f;
            float rad = 5.0f + AleatorioFlora() * 7.0f;
            cx = 7.5f + std::cos(ang) * rad;
            cz = 3.5f + std::sin(ang) * rad;
        }
        else if (n < 100)
        {
            cx = -29.0f + (AleatorioFlora() - 0.5f) * 10.0f;
            cz = -3.0f + (AleatorioFlora() - 0.5f) * 20.0f;
        }
        else
        {
            cx = X0 + AleatorioFlora() * ANCHO;
            cz = Z0 + AleatorioFlora() * LARGO;
        }

        if (
            AlturaTerreno(cx, cz) < -0.01f ||
            FueraDelAreaJugable(cx, cz, 11.0f) ||
            DistanciaSendero(cx, cz) < 2.0f ||
            DistanciaRio(cx, cz) < RioSemiancho(cz) + 0.8f ||
            !Despejado(cx, cz, 0.1f)
        )
        {
            continue;
        }

        for (int k = 0; k < 3; k++)
        {
            AgregarFlora(
                FLORA_FLOR,
                cx + (AleatorioFlora() - 0.5f) * 0.9f,
                cz + (AleatorioFlora() - 0.5f) * 0.9f,
                1.0f
            );
        }

        n++;
    }

    // Setas al pie de los arboles.
    int arbolesPrevios = cantidadFlora;

    for (int i = 0, n = 0; i < arbolesPrevios && n < 70; i++)
    {
        if (
            flora[i].tipo != FLORA_ROBLE &&
            flora[i].tipo != FLORA_ROBLE_FLORAL &&
            flora[i].tipo != FLORA_PINO
        )
        {
            continue;
        }

        float ang = AleatorioFlora() * 6.283f;
        float x = flora[i].x + std::cos(ang) * 0.95f;
        float z = flora[i].z + std::sin(ang) * 0.95f;

        if (
            DistanciaSendero(x, z) < 2.0f ||
            DistanciaRio(x, z) < RioSemiancho(z) + 0.6f
        )
        {
            continue;
        }

        AgregarFlora(FLORA_SETA, x, z, 0.9f + AleatorioFlora() * 0.6f);
        n++;
    }

    // Rocas: costa, colinas y orillas.
    for (int intento = 0, n = 0; intento < 20000 && n < 100; intento++)
    {
        float x = X0 - 2.0f + AleatorioFlora() * (ANCHO + 4.0f);
        float z = Z0 + AleatorioFlora() * (LARGO + 2.0f);
        float h = AlturaTerreno(x, z);

        if (
            h < -0.2f ||
            FueraDelAreaJugable(x, z, 18.0f) ||
            DistanciaSendero(x, z) < 2.1f ||
            !Despejado(x, z, 0.2f)
        )
        {
            continue;
        }

        // Prefiere cerca del agua o en pendiente.
        float hv = AlturaTerreno(x + 0.6f, z + 0.4f);
        bool interesante = h < 0.25f || std::fabs(hv - h) > 0.18f;

        if (!interesante && AleatorioFlora() < 0.85f)
        {
            continue;
        }

        AgregarFlora(FLORA_ROCA, x, z, 0.35f + AleatorioFlora() * 0.6f);
        n++;
    }

    floraConstruida = true;
}


//==================================================
// VEGETACION: MALLAS EN GPU
//==================================================
//
// Toda la flora se convierte en pocas mallas (en vez de ~2000 primitivas
// inmediatas por frame):
//   - fija: pinos, troncos, arbustos, tallos, setas y rocas;
//   - copas de roble, repartidas por fase del vaiven (la malla de cada
//     fase se desplaza como un todo);
//   - lo que depende de la floracion (copas florales, flores de arbustos
//     y cabezas de flor), que se reconstruye al cambiar la floracion.
// La calidad de decoracion recorta cantidad y detalle (ver
// ConservarFlora y reduccionDetalle).

static const int FASES_VAIVEN = 8;
static const float AMPLITUD_VAIVEN = 0.04f;

struct MallasFlora
{
    MallaGpu fija;
    MallaGpu copas[FASES_VAIVEN];
    MallaGpu copasFlorales[FASES_VAIVEN];
    MallaGpu floresDinamicas;
};

static MallasFlora mallasFlora;
static int nivelFloraConstruido = -1;
static int pasoFloracionFlora = -1;


static void AgregarRocaMalla(
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
        Mezcla(color, WHITE, 0.18f)
    );

    m.QuitarTransformacion();
}


// Calidad de decoracion: que elementos se plantan. ALTA conserva todos.
// MEDIA recorta parte de lo menudo y casi todas las setas; BAJA deja los
// arboles y rocas cercanos al sendero y poco de lo menudo.
static bool ConservarFlora(int indice, const ElementoFlora& e, NivelCalidadGrafica nivel)
{
    if (nivel == CALIDAD_ALTA)
    {
        return true;
    }

    bool menudo =
        e.tipo == FLORA_FLOR ||
        e.tipo == FLORA_SETA ||
        e.tipo == FLORA_ARBUSTO;

    float azar = Hash2(indice, 77);

    if (nivel == CALIDAD_MEDIA)
    {
        if (e.tipo == FLORA_SETA)
        {
            return azar < 0.35f;
        }

        return !menudo || azar < FactorCalidadGrafica(nivel);
    }

    // BAJA: nada fuera del area jugable y solo una fraccion de lo menudo.
    if (DistanciaSendero(e.x, e.z) > 11.0f)
    {
        return false;
    }

    if (e.tipo == FLORA_SETA)
    {
        return false;
    }

    return !menudo || azar < FactorCalidadGrafica(nivel) * 0.7f;
}


static int FaseVaiven(const ElementoFlora& e)
{
    float fase = std::fmod(e.x * 1.3f + e.z, 6.2831853f);

    if (fase < 0.0f)
    {
        fase += 6.2831853f;
    }

    return (int)(fase / 6.2831853f * FASES_VAIVEN + 0.5f) % FASES_VAIVEN;
}


static ConstructorMalla constructorFija;
static ConstructorMalla constructorCopas[FASES_VAIVEN];
static ConstructorMalla constructorCopasFlorales[FASES_VAIVEN];
static ConstructorMalla constructorFloresDinamicas;


// soloDinamico: solo reconstruye lo que depende de la floracion.
static void ConstruirMallasFlora(
    NivelCalidadGrafica nivel,
    float floracion,
    bool soloDinamico
)
{
    int reduccion = nivel == CALIDAD_ALTA ? 0 : 1;

    if (!soloDinamico)
    {
        constructorFija.Limpiar();
        constructorFija.reduccionDetalle = reduccion;
    }

    constructorFloresDinamicas.Limpiar();
    constructorFloresDinamicas.reduccionDetalle = reduccion;

    for (int k = 0; k < FASES_VAIVEN; k++)
    {
        if (!soloDinamico)
        {
            constructorCopas[k].Limpiar();
            constructorCopas[k].reduccionDetalle = reduccion;
        }

        constructorCopasFlorales[k].Limpiar();
        constructorCopasFlorales[k].reduccionDetalle = reduccion;
    }

    // Constructores descartables para lo fijo cuando solo se rehace lo dinamico.
    ConstructorMalla descartado;
    descartado.reduccionDetalle = reduccion;

    for (int i = 0; i < cantidadFlora; i++)
    {
        const ElementoFlora& e = flora[i];

        if (!ConservarFlora(i, e, nivel))
        {
            continue;
        }

        descartado.Limpiar();
        ConstructorMalla& fija = soloDinamico ? descartado : constructorFija;

        switch (e.tipo)
        {
            case FLORA_PINO:
                if (!soloDinamico)
                {
                    DibujarPino(fija, e.x, e.y, e.z, e.escala, e.variacion);
                }
                break;

            case FLORA_ROBLE:
                if (!soloDinamico)
                {
                    DibujarRoble(
                        fija, constructorCopas[FaseVaiven(e)],
                        e.x, e.y, e.z, e.escala, e.variacion, false, floracion
                    );
                }
                break;

            case FLORA_ROBLE_FLORAL:
                DibujarRoble(
                    fija, constructorCopasFlorales[FaseVaiven(e)],
                    e.x, e.y, e.z, e.escala, e.variacion, true, floracion
                );
                break;

            case FLORA_ARBUSTO:
                DibujarArbusto(
                    fija, constructorFloresDinamicas,
                    e.x, e.y, e.z, e.escala, e.variacion, floracion
                );
                break;

            case FLORA_FLOR:
                DibujarFlor(
                    fija, constructorFloresDinamicas,
                    e.x, e.y, e.z, e.variacion, floracion
                );
                break;

            case FLORA_SETA:
                if (!soloDinamico)
                {
                    DibujarSeta(fija, e.x, e.y, e.z, e.escala, e.variacion);
                }
                break;

            case FLORA_ROCA:
                if (!soloDinamico)
                {
                    AgregarRocaMalla(
                        fija,
                        e.x, e.y, e.z,
                        e.escala, e.giro,
                        Oscurecer(Color{ 128, 124, 120, 255 }, 0.85f + 0.3f * e.variacion)
                    );
                }
                break;
        }
    }

    if (!soloDinamico)
    {
        ActualizarMallaGpu(mallasFlora.fija, constructorFija);
    }

    ActualizarMallaGpu(mallasFlora.floresDinamicas, constructorFloresDinamicas);

    for (int k = 0; k < FASES_VAIVEN; k++)
    {
        if (!soloDinamico)
        {
            ActualizarMallaGpu(mallasFlora.copas[k], constructorCopas[k]);
        }

        ActualizarMallaGpu(mallasFlora.copasFlorales[k], constructorCopasFlorales[k]);
    }
}


static void DescargarMallasFlora()
{
    DescargarMalla(mallasFlora.fija);
    DescargarMalla(mallasFlora.floresDinamicas);

    for (int k = 0; k < FASES_VAIVEN; k++)
    {
        DescargarMalla(mallasFlora.copas[k]);
        DescargarMalla(mallasFlora.copasFlorales[k]);
    }

    nivelFloraConstruido = -1;
    pasoFloracionFlora = -1;
}


static void DibujarFlora(
    float tiempo,
    float floracion
)
{
    if (!floraConstruida)
    {
        ConstruirFlora();
    }

    NivelCalidadGrafica nivel = CalidadDecoracion();
    int paso = (int)(floracion * PASOS_FLORACION + 0.5f);

    if ((int)nivel != nivelFloraConstruido)
    {
        // Cambio de calidad (o primera vez): todo se reconstruye.
        ConstruirMallasFlora(nivel, (float)paso / PASOS_FLORACION, false);
        nivelFloraConstruido = (int)nivel;
        pasoFloracionFlora = paso;
    }
    else if (paso != pasoFloracionFlora)
    {
        ConstruirMallasFlora(nivel, (float)paso / PASOS_FLORACION, true);
        pasoFloracionFlora = paso;
    }

    DibujarMallaGpu(mallasFlora.fija);
    DibujarMallaGpu(mallasFlora.floresDinamicas);

    for (int k = 0; k < FASES_VAIVEN; k++)
    {
        float vaiven =
            AMPLITUD_VAIVEN *
            std::sin(tiempo * 1.1f + k * 6.2831853f / FASES_VAIVEN);

        DibujarMallaGpu(mallasFlora.copas[k], vaiven, 0.0f, 0.0f);
        DibujarMallaGpu(mallasFlora.copasFlorales[k], vaiven, 0.0f, 0.0f);
    }
}


//==================================================
// GRAN ROBLE (landmark principal)
//==================================================
//
// Raices, tronco, ramas y copas son estaticos: se suben una vez a la GPU
// (las copas se recolorean al cambiar la floracion, cuantizada) y solo los
// detalles que se mueven (faroles, runas, luciernagas) se dibujan en modo
// inmediato.

struct LumbreRoble
{
    float dx, dy, dz, r;
};

static const LumbreRoble LUMBRES_ROBLE[] =
{
    {  0.0f, 8.4f,  0.0f, 3.4f },
    { -2.8f, 7.6f,  0.8f, 2.6f },
    {  2.9f, 7.5f, -0.6f, 2.7f },
    {  0.6f, 7.4f,  2.9f, 2.4f },
    { -0.8f, 7.6f, -2.8f, 2.5f },
    { -1.2f, 10.0f, 0.6f, 2.2f },
    {  1.8f, 9.6f,  0.4f, 2.0f },
    {  3.9f, 8.4f,  1.2f, 1.7f },
    { -3.9f, 8.5f, -0.8f, 1.8f }
};

static ConstructorMalla constructorRobleFijo;
static ConstructorMalla constructorRobleCopas;
static MallaGpu mallaRobleFija;
static MallaGpu mallaRobleCopas;
static int pasoFloracionRoble = -1;
static float yBaseRoble = 0.0f;


static void ConstruirCopasRoble(float floracion)
{
    ConstructorMalla& m = constructorRobleCopas;
    float y0 = yBaseRoble;

    m.Limpiar();

    Color oscuro = Mezcla(Color{ 36, 96, 52, 255 }, Color{ 176, 86, 128, 255 }, floracion);
    Color medio = Mezcla(Color{ 60, 132, 66, 255 }, Color{ 228, 128, 168, 255 }, floracion);
    Color claro = Mezcla(Color{ 92, 168, 80, 255 }, Color{ 252, 186, 210, 255 }, floracion);

    float crece = 1.0f + 0.10f * floracion;

    for (const LumbreRoble& l : LUMBRES_ROBLE)
    {
        Vector3 c = { ROBLE_X + l.dx * crece, y0 + l.dy + 1.0f, ROBLE_Z + l.dz * crece };

        m.Esfera(c, l.r, 7, 8, oscuro);
        m.Esfera(Vector3{ c.x - 0.1f, c.y + l.r * 0.12f, c.z + 0.1f }, l.r * 0.88f, 7, 8, medio);
        m.Esfera(Vector3{ c.x - l.r * 0.2f, c.y + l.r * 0.34f, c.z + l.r * 0.18f }, l.r * 0.58f, 6, 7, claro);
    }
}


static void ConstruirTroncoRoble()
{
    ConstructorMalla& m = constructorRobleFijo;
    float y0 = yBaseRoble;

    m.Limpiar();

    // Raices que se hunden en la meseta.
    for (int i = 0; i < 7; i++)
    {
        float ang = i * 0.897f + 0.3f;

        m.Cilindro(
            Vector3{ ROBLE_X + std::cos(ang) * 0.8f, y0 + 1.1f, ROBLE_Z + std::sin(ang) * 0.8f },
            Vector3{ ROBLE_X + std::cos(ang) * 3.2f, y0 + 0.1f, ROBLE_Z + std::sin(ang) * 3.2f },
            0.55f, 0.16f, 6,
            CORTEZA_OSCURA
        );
    }

    // Tronco enorme en dos tramos.
    m.Cilindro(
        Vector3{ ROBLE_X, y0, ROBLE_Z },
        Vector3{ ROBLE_X, y0 + 4.6f, ROBLE_Z },
        1.7f, 1.05f, 10, CORTEZA
    );

    m.Cilindro(
        Vector3{ ROBLE_X, y0 + 4.5f, ROBLE_Z },
        Vector3{ ROBLE_X + 0.2f, y0 + 7.0f, ROBLE_Z },
        1.05f, 0.7f, 9, CORTEZA
    );

    // Ramas gruesas hacia las copas.
    for (int i = 0; i < 5; i++)
    {
        float ang = i * 1.257f + 0.5f;

        m.Cilindro(
            Vector3{ ROBLE_X, y0 + 5.2f, ROBLE_Z },
            Vector3{ ROBLE_X + std::cos(ang) * 3.0f, y0 + 7.0f, ROBLE_Z + std::sin(ang) * 3.0f },
            0.48f, 0.2f, 6, CORTEZA
        );
    }

    // Puerta con luz calida en el tronco (mira a la camara).
    m.Caja(
        Vector3{ ROBLE_X, y0 + 0.95f, ROBLE_Z + 1.62f },
        Vector3{ 0.95f, 1.5f, 0.16f },
        Color{ 36, 24, 20, 255 }
    );

    m.Caja(
        Vector3{ ROBLE_X, y0 + 0.9f, ROBLE_Z + 1.72f },
        Vector3{ 0.62f, 1.1f, 0.05f },
        Color{ 255, 206, 120, 255 }
    );
}


static void DibujarGranRoble(float tiempo, float floracion)
{
    if (!mallaRobleFija.cargada)
    {
        yBaseRoble = AlturaTerreno(ROBLE_X, ROBLE_Z) - 0.2f;

        ConstruirTroncoRoble();
        ActualizarMallaGpu(mallaRobleFija, constructorRobleFijo);
        pasoFloracionRoble = -1;
    }

    int paso = (int)(floracion * PASOS_FLORACION + 0.5f);

    if (!mallaRobleCopas.cargada || paso != pasoFloracionRoble)
    {
        ConstruirCopasRoble((float)paso / PASOS_FLORACION);
        ActualizarMallaGpu(mallaRobleCopas, constructorRobleCopas);
        pasoFloracionRoble = paso;
    }

    float y0 = yBaseRoble;

    // El Gran Roble es mas grande que el resto: escala desde su base.
    rlPushMatrix();
    rlTranslatef(ROBLE_X, y0, ROBLE_Z);
    rlScalef(1.25f, 1.25f, 1.25f);
    rlTranslatef(-ROBLE_X, -y0, -ROBLE_Z);

    DibujarMallaGpu(mallaRobleFija);

    // El vaiven de la copa lo aplica el desplazamiento de su malla.
    DibujarMallaGpu(mallaRobleCopas, 0.05f * std::sin(tiempo * 0.9f), 0.0f, 0.0f);

    // Faroles colgando de la copa.
    for (int i = 0; i < 6; i++)
    {
        float ang = i * 1.047f + 0.4f;
        float brillo = 0.75f + 0.25f * std::sin(tiempo * 2.0f + i);

        Vector3 farol =
        {
            ROBLE_X + std::cos(ang) * 3.3f,
            y0 + 5.6f,
            ROBLE_Z + std::sin(ang) * 3.3f
        };

        DrawCylinderEx(
            Vector3{ farol.x, farol.y + 1.0f, farol.z },
            farol, 0.015f, 0.015f, 3, CORTEZA_OSCURA
        );

        DrawSphereEx(
            farol, 0.2f, 4, 5,
            Color{ 255, (unsigned char)(200 * brillo), 110, 255 }
        );
    }

    // Anillo de runas luminosas alrededor de las raices.
    for (int i = 0; i < 8; i++)
    {
        float ang = i * 0.785f + tiempo * 0.1f;
        float brillo = 0.5f + 0.5f * std::sin(tiempo * 2.4f + i * 0.8f);

        Vector3 p =
        {
            ROBLE_X + std::cos(ang) * 4.1f,
            y0 + 0.12f,
            ROBLE_Z + std::sin(ang) * 4.1f
        };

        DrawCubeV(p, Vector3{ 0.3f, 0.12f, 0.3f }, Color{ 150, 230, 255, 255 });
        DrawSphereEx(
            Vector3{ p.x, p.y + 0.22f, p.z },
            0.1f + 0.06f * brillo, 3, 4,
            Color{ 190, 245, 255, 255 }
        );
    }

    // Luciernagas.
    for (int i = 0; i < 12; i++)
    {
        float fase = tiempo * 0.5f + i * 0.53f;

        DrawSphereEx(
            Vector3
            {
                ROBLE_X + std::cos(fase * 1.3f + i) * (3.0f + 0.1f * i),
                y0 + 2.0f + 2.5f * (0.5f + 0.5f * std::sin(fase + i)),
                ROBLE_Z + std::sin(fase * 1.1f + i * 2.0f) * (2.4f + 0.1f * i)
            },
            0.07f, 3, 3,
            Color{ 255, 240, 140, 255 }
        );
    }

    rlPopMatrix();
}


static void DescargarRoble()
{
    DescargarMalla(mallaRobleFija);
    DescargarMalla(mallaRobleCopas);
    pasoFloracionRoble = -1;
}


//==================================================
// PUENTES, CASCADA, CIRCULO DE PIEDRAS, MUELLE, CARTEL
//==================================================

static void DibujarPuente(const Casilla& a, const Casilla& b)
{
    float dx = b.posicion.x - a.posicion.x;
    float dz = b.posicion.z - a.posicion.z;
    float largo = std::sqrt(dx * dx + dz * dz);

    float giro = std::atan2(-dz, dx) * 57.29578f;

    rlPushMatrix();
    rlTranslatef(
        (a.posicion.x + b.posicion.x) * 0.5f,
        0.0f,
        (a.posicion.z + b.posicion.z) * 0.5f
    );
    rlRotatef(giro, 0.0f, 1.0f, 0.0f);

    float mitad = largo * 0.5f - 0.55f;
    float ancho = 2.1f;
    float yCubierta = 0.13f;

    // Tablones.
    int tablones = 9;
    float paso = (2.0f * (mitad + 0.5f)) / tablones;

    for (int i = 0; i < tablones; i++)
    {
        float x = -mitad - 0.5f + paso * (i + 0.5f);

        DrawCubeV(
            Vector3{ x, yCubierta, 0.0f },
            Vector3{ paso * 0.88f, 0.09f, ancho },
            i % 2 == 0 ? Color{ 156, 108, 66, 255 } : Color{ 134, 92, 56, 255 }
        );
    }

    // Vigas laterales.
    DrawCubeV(Vector3{ 0.0f, 0.04f, ancho * 0.5f - 0.05f }, Vector3{ 2.0f * (mitad + 0.5f), 0.12f, 0.12f }, CORTEZA_OSCURA);
    DrawCubeV(Vector3{ 0.0f, 0.04f, -ancho * 0.5f + 0.05f }, Vector3{ 2.0f * (mitad + 0.5f), 0.12f, 0.12f }, CORTEZA_OSCURA);

    // Postes, barandilla y faroles.
    for (int lado = -1; lado <= 1; lado += 2)
    {
        float z = lado * (ancho * 0.5f - 0.04f);

        for (int extremo = -1; extremo <= 1; extremo += 2)
        {
            float x = extremo * mitad;

            DrawCylinderEx(
                Vector3{ x, -0.9f, z },
                Vector3{ x, 0.85f, z },
                0.07f, 0.07f, 5, CORTEZA
            );

            DrawSphereEx(
                Vector3{ x, 0.92f, z }, 0.09f, 4, 5,
                Color{ 255, 210, 120, 255 }
            );
        }

        DrawCubeV(
            Vector3{ 0.0f, 0.62f, z },
            Vector3{ 2.0f * mitad, 0.06f, 0.06f },
            Color{ 170, 122, 78, 255 }
        );
    }

    rlPopMatrix();
}


static void CuadroLibre(
    Vector3 a, Vector3 b, Vector3 c, Vector3 d,
    Color color
)
{
    DrawTriangle3D(a, b, c, color);
    DrawTriangle3D(a, c, d, color);
}


static void DibujarCascada(float tiempo)
{
    float x = RioX(PUNTOS_RIO[0].z);
    float zTop = PUNTOS_RIO[0].z - 1.6f;
    float yTop = AlturaTerreno(x, zTop) + 0.1f;

    rlDisableBackfaceCulling();

    float ancho = 0.55f;

    CuadroLibre(
        Vector3{ x - ancho, yTop, zTop },
        Vector3{ x + ancho, yTop, zTop },
        Vector3{ x + ancho, NIVEL_AGUA + 0.02f, zTop + 1.15f },
        Vector3{ x - ancho, NIVEL_AGUA + 0.02f, zTop + 1.15f },
        Color{ 170, 224, 245, 175 }
    );

    // Hilos de espuma que bajan.
    int hilos = (int)std::ceil(6.0f * FactorCalidadGrafica(CalidadParticulas()));

    for (int i = 0; i < hilos; i++)
    {
        float fase = std::fmod(tiempo * 0.9f + i * 0.17f, 1.0f);
        float px = x - ancho + (i + 0.5f) * (2.0f * ancho / 6.0f);
        float y = yTop + (NIVEL_AGUA - yTop) * fase;
        float z = zTop + 1.15f * fase + 0.02f;

        CuadroLibre(
            Vector3{ px - 0.07f, y + 0.5f, z - 0.1f },
            Vector3{ px + 0.07f, y + 0.5f, z - 0.1f },
            Vector3{ px + 0.07f, y, z },
            Vector3{ px - 0.07f, y, z },
            Color{ 255, 255, 255, 190 }
        );
    }

    rlEnableBackfaceCulling();

    // Remolino en la base.
    float pulso = 0.5f + 0.5f * std::sin(tiempo * 4.0f);

    DrawCylinderEx(
        Vector3{ x, NIVEL_AGUA + 0.03f, zTop + 1.3f },
        Vector3{ x, NIVEL_AGUA + 0.05f, zTop + 1.3f },
        0.8f + 0.2f * pulso, 0.8f + 0.2f * pulso, 12,
        Color{ 255, 255, 255, 90 }
    );
}


// Ruinas del Jardin: dais con columnas rotas, cristal y flores (centro).
// La piedra es estatica y vive en una malla; el cristal, su halo y las
// enredaderas (que cambian con la floracion) se dibujan al momento.
static ConstructorMalla constructorRuinas;
static MallaGpu mallaRuinas;


static void ConstruirMallaRuinas(float cx, float cz, float y)
{
    ConstructorMalla& m = constructorRuinas;

    m.Limpiar();

    m.Cilindro(
        Vector3{ cx, y, cz }, Vector3{ cx, y + 0.22f, cz },
        2.9f, 2.7f, 16, Color{ 136, 132, 128, 255 }
    );

    m.Cilindro(
        Vector3{ cx, y + 0.22f, cz }, Vector3{ cx, y + 0.30f, cz },
        2.3f, 2.3f, 16, Color{ 168, 164, 156, 255 }
    );

    // Columnas: algunas rotas, con escombros.
    for (int i = 0; i < 6; i++)
    {
        float ang = i * 1.0472f + 0.3f;
        float px = cx + std::cos(ang) * 2.25f;
        float pz = cz + std::sin(ang) * 2.25f;
        bool rota = (i % 2) == 1;
        float altura = rota ? 1.1f + 0.5f * Hash2(i, 3) : 2.7f;

        m.Cilindro(
            Vector3{ px, y + 0.3f, pz },
            Vector3{ px, y + 0.3f + altura, pz },
            0.34f, 0.30f, 8, Color{ 176, 172, 164, 255 }
        );

        if (!rota)
        {
            TransformacionMalla t;
            t.tx = px;
            t.ty = y + 0.3f + altura + 0.1f;
            t.tz = pz;
            t.giroY = ang * 57.3f;

            m.EstablecerTransformacion(t);
            m.Caja(Vector3{ 0.0f, 0.0f, 0.0f }, Vector3{ 0.95f, 0.2f, 0.95f }, Color{ 150, 146, 140, 255 });
            m.QuitarTransformacion();
        }
        else
        {
            AgregarRocaMalla(
                m, px + 0.5f, y + 0.3f, pz - 0.3f,
                0.25f, i * 40.0f, Color{ 150, 146, 140, 255 }
            );
        }
    }
}


static void DibujarRuinasJardin(float tiempo, float floracion)
{
    const float cx = 7.5f;
    const float cz = 3.5f;
    float y = AlturaTerreno(cx, cz);

    if (!mallaRuinas.cargada)
    {
        ConstruirMallaRuinas(cx, cz, y);
        ActualizarMallaGpu(mallaRuinas, constructorRuinas);
    }

    DibujarMallaGpu(mallaRuinas);

    // Enredaderas (y flores rosas al florecer).
    for (int i = 0; i < 6; i++)
    {
        float ang = i * 1.0472f + 0.3f;
        float px = cx + std::cos(ang) * 2.25f;
        float pz = cz + std::sin(ang) * 2.25f;
        bool rota = (i % 2) == 1;
        float altura = rota ? 1.1f + 0.5f * Hash2(i, 3) : 2.7f;

        DrawSphereEx(
            Vector3{ px, y + 0.3f + altura * 0.55f, pz },
            0.22f, 4, 5,
            Mezcla(Color{ 76, 134, 66, 255 }, Color{ 238, 150, 190, 255 }, floracion * 0.7f)
        );
    }

    // Cristal grande flotante con halo.
    float flotar = 0.2f * std::sin(tiempo * 1.8f);

    rlPushMatrix();
    rlTranslatef(cx, y + 1.9f + flotar, cz);
    rlRotatef(tiempo * 55.0f, 0.0f, 1.0f, 0.0f);

    DrawCylinderEx(Vector3{ 0.0f, 0.0f, 0.0f }, Vector3{ 0.0f, 0.95f, 0.0f }, 0.42f, 0.0f, 6, Color{ 150, 232, 255, 255 });
    DrawCylinderEx(Vector3{ 0.0f, 0.0f, 0.0f }, Vector3{ 0.0f, -0.95f, 0.0f }, 0.42f, 0.0f, 6, Color{ 84, 184, 238, 255 });

    rlPopMatrix();

    float pulso = 0.5f + 0.5f * std::sin(tiempo * 2.6f);

    DrawCylinderEx(
        Vector3{ cx, y + 0.31f, cz }, Vector3{ cx, y + 0.33f, cz },
        1.3f + 0.2f * pulso, 1.3f + 0.2f * pulso, 16,
        Color{ 140, 230, 255, 90 }
    );
}


// Arco ruinoso al este del jardin.
static void DibujarArcoRuinoso(float floracion)
{
    const float x = 17.0f;
    const float z = 9.5f;
    float y = AlturaTerreno(x, z);

    rlPushMatrix();
    rlTranslatef(x, y, z);
    rlRotatef(20.0f, 0.0f, 1.0f, 0.0f);

    DrawCubeV(Vector3{ -1.7f, 1.7f, 0.0f }, Vector3{ 0.8f, 3.4f, 0.8f }, Color{ 150, 146, 140, 255 });
    DrawCubeV(Vector3{ 1.7f, 1.1f, 0.0f }, Vector3{ 0.8f, 2.2f, 0.8f }, Color{ 140, 136, 130, 255 });
    DrawCubeV(Vector3{ -0.5f, 3.5f, 0.0f }, Vector3{ 3.0f, 0.55f, 0.9f }, Color{ 162, 158, 150, 255 });

    DrawSphereEx(Vector3{ -1.7f, 3.0f, 0.4f }, 0.3f, 4, 5,
        Mezcla(Color{ 76, 134, 66, 255 }, Color{ 238, 150, 190, 255 }, floracion * 0.7f));

    rlPopMatrix();

    DibujarRoca(x + 2.8f, y, z + 0.8f, 0.35f, 40.0f, Color{ 140, 136, 130, 255 });
}


// Muelle de la costa con una barquita.
static void DibujarMuelle(float tiempo)
{
    const float x = -14.0f;

    for (int i = 0; i < 14; i++)
    {
        float z = 18.6f + i * 0.38f;

        DrawCubeV(
            Vector3{ x, 0.10f, z },
            Vector3{ 1.5f, 0.08f, 0.34f },
            i % 2 == 0 ? Color{ 160, 112, 70, 255 } : Color{ 138, 96, 58, 255 }
        );
    }

    for (int i = 0; i < 5; i++)
    {
        float z = 18.9f + i * 1.25f;

        for (int lado = -1; lado <= 1; lado += 2)
        {
            DrawCylinderEx(
                Vector3{ x + lado * 0.78f, -1.1f, z },
                Vector3{ x + lado * 0.78f, 0.45f, z },
                0.07f, 0.07f, 5, CORTEZA
            );
        }
    }

    DrawSphereEx(Vector3{ x, 0.62f, 23.6f }, 0.12f, 4, 5, Color{ 255, 210, 120, 255 });

    // Barquita atada.
    float vaiven = 0.04f * std::sin(tiempo * 1.5f);

    rlPushMatrix();
    rlTranslatef(-11.6f, NIVEL_AGUA + 0.02f + vaiven, 23.4f);
    rlRotatef(12.0f + 4.0f * std::sin(tiempo * 0.8f), 0.0f, 1.0f, 0.0f);

    DrawCubeV(Vector3{ 0.0f, 0.12f, 0.0f }, Vector3{ 0.9f, 0.28f, 1.9f }, Color{ 150, 92, 58, 255 });
    DrawCubeV(Vector3{ 0.0f, 0.28f, 0.0f }, Vector3{ 0.7f, 0.04f, 1.6f }, Color{ 188, 132, 86, 255 });
    DrawCylinderEx(Vector3{ 0.0f, 0.3f, 0.1f }, Vector3{ 0.0f, 1.7f, 0.1f }, 0.04f, 0.04f, 4, CORTEZA_OSCURA);
    DrawCubeV(Vector3{ 0.0f, 1.1f, 0.12f }, Vector3{ 0.02f, 0.9f, 0.65f }, Color{ 245, 240, 226, 255 });

    rlPopMatrix();
}


// Puerta de bienvenida sobre la casilla inicial.
static void DibujarPuertaCosta(float tiempo)
{
    const Tablero& t = TableroReferencia();
    const Casilla* inicio = t.ObtenerCasilla(casillaInicioArboleda);

    if (inicio == nullptr)
    {
        return;
    }

    rlPushMatrix();
    rlTranslatef(inicio->posicion.x, 0.0f, inicio->posicion.z);
    rlRotatef(39.3f, 0.0f, 1.0f, 0.0f);

    for (int lado = -1; lado <= 1; lado += 2)
    {
        DrawCylinderEx(
            Vector3{ lado * 2.3f, -0.1f, 0.0f },
            Vector3{ lado * 2.3f, 2.7f, 0.0f },
            0.2f, 0.26f, 6, CORTEZA
        );

        DrawSphereEx(
            Vector3{ lado * 2.3f, 2.85f, 0.0f }, 0.22f, 4, 5,
            Color{ 255, 206, 110, 255 }
        );
    }

    DrawCubeV(Vector3{ 0.0f, 2.6f, 0.0f }, Vector3{ 5.2f, 0.3f, 0.4f }, CORTEZA_OSCURA);

    // Banderines que ondean.
    for (int i = 0; i < 7; i++)
    {
        float x = -2.0f + i * 0.66f;
        float onda = 0.12f * std::sin(tiempo * 2.4f + i);

        Color colores[3] =
        {
            Color{ 236, 96, 96, 255 },
            Color{ 246, 206, 80, 255 },
            Color{ 90, 190, 120, 255 }
        };

        DrawCubeV(Vector3{ x, 2.3f + onda, 0.0f }, Vector3{ 0.4f, 0.4f, 0.04f }, colores[i % 3]);
    }

    rlPopMatrix();
}


// Cabana del bosque bajo.
static void DibujarCabana()
{
    const float x = -29.5f;
    const float z = -3.0f;
    float y = AlturaTerreno(x, z);

    rlPushMatrix();
    rlTranslatef(x, y, z);
    rlRotatef(-70.0f, 0.0f, 1.0f, 0.0f);

    DrawCubeV(Vector3{ 0.0f, 1.0f, 0.0f }, Vector3{ 3.2f, 2.0f, 2.6f }, Color{ 176, 128, 84, 255 });

    // Techo a cuatro aguas.
    rlPushMatrix();
    rlTranslatef(0.0f, 2.0f, 0.0f);
    rlRotatef(45.0f, 0.0f, 1.0f, 0.0f);
    DrawCylinderEx(Vector3{ 0.0f, 0.0f, 0.0f }, Vector3{ 0.0f, 1.5f, 0.0f }, 2.6f, 0.0f, 4, Color{ 156, 70, 60, 255 });
    rlPopMatrix();

    DrawCubeV(Vector3{ 0.0f, 0.7f, 1.32f }, Vector3{ 0.8f, 1.4f, 0.06f }, Color{ 96, 64, 44, 255 });
    DrawCubeV(Vector3{ 1.0f, 1.2f, 1.32f }, Vector3{ 0.6f, 0.6f, 0.06f }, Color{ 255, 214, 130, 255 });
    DrawCubeV(Vector3{ -1.0f, 3.0f, -0.6f }, Vector3{ 0.5f, 1.2f, 0.5f }, Color{ 130, 124, 120, 255 });

    rlPopMatrix();
}


// Molino de agua junto al rio (norte).
static void DibujarMolino(float tiempo)
{
    const float hx = -4.6f;
    const float hz = -7.0f;
    float y = AlturaTerreno(hx, hz);

    DrawCubeV(Vector3{ hx, y + 1.1f, hz }, Vector3{ 2.6f, 2.2f, 2.4f }, Color{ 190, 184, 170, 255 });

    rlPushMatrix();
    rlTranslatef(hx, y + 2.2f, hz);
    rlRotatef(45.0f, 0.0f, 1.0f, 0.0f);
    DrawCylinderEx(Vector3{ 0.0f, 0.0f, 0.0f }, Vector3{ 0.0f, 1.5f, 0.0f }, 2.1f, 0.0f, 4, Color{ 176, 80, 62, 255 });
    rlPopMatrix();

    DrawCubeV(Vector3{ hx + 0.2f, y + 0.7f, hz + 1.22f }, Vector3{ 0.7f, 1.4f, 0.06f }, Color{ 96, 64, 44, 255 });

    // Eje y rueda sobre el rio.
    float rx = RioX(hz);

    DrawCylinderEx(
        Vector3{ rx, 0.6f, hz }, Vector3{ hx - 1.3f, 0.6f, hz },
        0.12f, 0.12f, 5, CORTEZA_OSCURA
    );

    rlPushMatrix();
    rlTranslatef(rx, 0.6f, hz);
    rlRotatef(tiempo * 40.0f, 0.0f, 0.0f, 1.0f);

    for (int k = 0; k < 4; k++)
    {
        rlPushMatrix();
        rlRotatef(k * 45.0f, 0.0f, 0.0f, 1.0f);
        DrawCubeV(Vector3{ 0.0f, 0.0f, 0.0f }, Vector3{ 2.4f, 0.12f, 0.5f }, CORTEZA);
        rlPopMatrix();
    }

    for (int k = 0; k < 8; k++)
    {
        float ang = k * 0.7854f;
        DrawCubeV(
            Vector3{ std::cos(ang) * 1.2f, std::sin(ang) * 1.2f, 0.0f },
            Vector3{ 0.3f, 0.3f, 0.7f },
            Color{ 160, 112, 70, 255 }
        );
    }

    rlPopMatrix();
}


// Atalaya de madera en el altiplano.
static void DibujarAtalaya(float tiempo)
{
    const float x = 33.0f;
    const float z = -3.0f;
    float y = AlturaTerreno(x, z);

    for (int sx = -1; sx <= 1; sx += 2)
    {
        for (int sz = -1; sz <= 1; sz += 2)
        {
            DrawCylinderEx(
                Vector3{ x + sx * 1.0f, y - 0.2f, z + sz * 1.0f },
                Vector3{ x + sx * 0.8f, y + 4.6f, z + sz * 0.8f },
                0.16f, 0.12f, 6, CORTEZA
            );
        }
    }

    DrawCubeV(Vector3{ x, y + 4.6f, z }, Vector3{ 2.4f, 0.2f, 2.4f }, Color{ 156, 108, 68, 255 });

    // Barandilla y techo.
    for (int lado = -1; lado <= 1; lado += 2)
    {
        DrawCubeV(Vector3{ x + lado * 1.15f, y + 5.1f, z }, Vector3{ 0.08f, 0.1f, 2.3f }, CORTEZA_OSCURA);
        DrawCubeV(Vector3{ x, y + 5.1f, z + lado * 1.15f }, Vector3{ 2.3f, 0.1f, 0.08f }, CORTEZA_OSCURA);
    }

    rlPushMatrix();
    rlTranslatef(x, y + 6.7f, z);
    rlRotatef(45.0f, 0.0f, 1.0f, 0.0f);
    DrawCylinderEx(Vector3{ 0.0f, 0.0f, 0.0f }, Vector3{ 0.0f, 1.4f, 0.0f }, 2.1f, 0.0f, 4, Color{ 176, 80, 62, 255 });
    rlPopMatrix();


    // Farol y banderin.
    float brillo = 0.75f + 0.25f * std::sin(tiempo * 2.0f);

    DrawSphereEx(Vector3{ x, y + 5.55f, z }, 0.22f, 4, 5, Color{ 255, (unsigned char)(210 * brillo), 120, 255 });

    DrawCylinderEx(
        Vector3{ x, y + 8.0f, z }, Vector3{ x, y + 9.6f, z },
        0.05f, 0.05f, 4, CORTEZA_OSCURA
    );

    DrawCubeV(
        Vector3{ x + 0.5f + 0.1f * std::sin(tiempo * 3.0f), y + 9.2f, z },
        Vector3{ 1.0f, 0.5f, 0.04f },
        Color{ 236, 96, 96, 255 }
    );
}


// Setas gigantes luminosas del bosque profundo (la seta es estatica; solo
// el halo de esporas pulsa).
struct SetaGigante
{
    float x;
    float z;
    float radio;
};

static const SetaGigante SETAS_GIGANTES[] =
{
    { 9.5f, -23.0f, 1.4f },
    { 13.5f, -27.5f, 1.9f },
    { 9.0f, -30.5f, 1.3f }
};

static ConstructorMalla constructorSetas;
static MallaGpu mallaSetas;
static float alturaSetas[3];


static void ConstruirMallaSetas()
{
    ConstructorMalla& m = constructorSetas;

    m.Limpiar();

    for (int i = 0; i < 3; i++)
    {
        float x = SETAS_GIGANTES[i].x;
        float z = SETAS_GIGANTES[i].z;
        float r = SETAS_GIGANTES[i].radio;
        float y = AlturaTerreno(x, z);
        alturaSetas[i] = y;

        m.Cilindro(
            Vector3{ x, y, z }, Vector3{ x, y + r * 1.5f, z },
            r * 0.28f, r * 0.22f, 8, Color{ 236, 226, 204, 255 }
        );

        // Sombrero aplastado.
        TransformacionMalla t;
        t.tx = x;
        t.ty = y + r * 1.4f;
        t.tz = z;
        t.sy = 0.55f;

        m.EstablecerTransformacion(t);
        m.Esfera(
            Vector3{ 0.0f, 0.0f, 0.0f }, r, 6, 8,
            i == 1 ? Color{ 150, 90, 200, 255 } : Color{ 214, 70, 74, 255 }
        );
        m.QuitarTransformacion();

        for (int k = 0; k < 5; k++)
        {
            float ang = k * 1.2566f + i;

            m.Esfera(
                Vector3
                {
                    x + std::cos(ang) * r * 0.55f,
                    y + r * 1.4f + r * 0.38f,
                    z + std::sin(ang) * r * 0.55f
                },
                r * 0.12f, 3, 4, Color{ 255, 246, 236, 255 }
            );
        }
    }
}


static void DibujarSetasGigantes(float tiempo)
{
    if (!mallaSetas.cargada)
    {
        ConstruirMallaSetas();
        ActualizarMallaGpu(mallaSetas, constructorSetas);
    }

    DibujarMallaGpu(mallaSetas);

    // Halo de esporas.
    for (int i = 0; i < 3; i++)
    {
        float x = SETAS_GIGANTES[i].x;
        float z = SETAS_GIGANTES[i].z;
        float r = SETAS_GIGANTES[i].radio;
        float y = alturaSetas[i];
        float pulso = 0.5f + 0.5f * std::sin(tiempo * 2.0f + i);

        DrawCylinderEx(
            Vector3{ x, y + 0.02f, z }, Vector3{ x, y + 0.04f, z },
            r * (1.1f + 0.2f * pulso), r * (1.1f + 0.2f * pulso), 12,
            Color{ 240, 150, 220, 60 }
        );
    }
}


// Carteles en cada bifurcacion: una tabla por camino (la segunda, mas roja,
// indica el atajo).
static void DibujarCartelesBifurcacion()
{
    const Tablero& t = TableroReferencia();

    for (int i = 0; i < t.cantidadCasillas; i++)
    {
        const Casilla& c = t.casillas[i];

        if (c.cantidadConexiones < 2)
        {
            continue;
        }

        float px = c.posicion.x - 1.8f;
        float pz = c.posicion.z + 1.4f;
        float y = AlturaTerreno(px, pz);

        DrawCylinderEx(
            Vector3{ px, y, pz }, Vector3{ px, y + 1.9f, pz },
            0.07f, 0.09f, 5, CORTEZA
        );

        for (int o = 0; o < 2; o++)
        {
            const Casilla* destino =
                t.ObtenerCasilla(c.conexiones[o].destino);

            if (destino == nullptr)
            {
                continue;
            }

            float dx = destino->posicion.x - c.posicion.x;
            float dz = destino->posicion.z - c.posicion.z;
            float giro = std::atan2(-dz, dx) * 57.29578f;

            rlPushMatrix();
            rlTranslatef(px, y + 1.6f - 0.4f * o, pz);
            rlRotatef(giro, 0.0f, 1.0f, 0.0f);

            DrawCubeV(
                Vector3{ 0.55f, 0.0f, 0.0f },
                Vector3{ 1.1f, 0.22f, 0.07f },
                o == 0 ? Color{ 196, 150, 96, 255 } : Color{ 214, 110, 86, 255 }
            );

            DrawCubeV(
                Vector3{ 1.15f, 0.0f, 0.0f },
                Vector3{ 0.2f, 0.12f, 0.07f },
                o == 0 ? Color{ 196, 150, 96, 255 } : Color{ 214, 110, 86, 255 }
            );

            rlPopMatrix();
        }
    }
}


//==================================================
// FLORACION: PETALOS Y NENUFARES
//==================================================

static void DibujarPetalosYLaguna(float tiempo, float floracion)
{
    if (!floraConstruida)
    {
        ConstruirFlora();
    }

    // Nenufares de la laguna (flores rosas al florecer).
    for (int i = 0; i < 8; i++)
    {
        float z = -3.3f + i * 0.55f;
        float x = RioX(z) + std::sin(i * 2.1f) * 1.6f;

        DrawCylinderEx(
            Vector3{ x, NIVEL_AGUA + 0.03f, z },
            Vector3{ x, NIVEL_AGUA + 0.06f, z },
            0.32f, 0.32f, 8, Color{ 70, 150, 84, 255 }
        );

        if (floracion > 0.05f)
        {
            DrawSphereEx(
                Vector3{ x, NIVEL_AGUA + 0.12f, z },
                0.14f * floracion, 3, 4,
                Color{ 255, 176, 206, 255 }
            );
        }
    }

    if (floracion < 0.05f)
    {
        return;
    }

    // Petalos que caen de las copas florales y del Gran Roble.
    int cantidad = (int)(110.0f * floracion * FactorCalidadGrafica(CalidadParticulas()));

    int florales = 0;
    int indices[64];

    for (int i = 0; i < cantidadFlora && florales < 64; i++)
    {
        if (flora[i].tipo == FLORA_ROBLE_FLORAL)
        {
            indices[florales++] = i;
        }
    }

    float yRoble = AlturaTerrenoMalla(ROBLE_X, ROBLE_Z);

    for (int i = 0; i < cantidad; i++)
    {
        float fase = std::fmod(tiempo * 0.32f + i * 0.137f, 1.0f);

        float x;
        float z;
        float yInicio;

        if (i % 3 == 0 || florales == 0)
        {
            // Del Gran Roble: caen desde muy alto y se dispersan.
            float ang = i * 2.399f;
            x = ROBLE_X + std::cos(ang) * (2.0f + 2.5f * fase);
            z = ROBLE_Z + std::sin(ang) * (2.0f + 2.5f * fase) + 1.5f * fase;
            yInicio = yRoble + 12.0f;
        }
        else
        {
            const ElementoFlora& arbol = flora[indices[i % florales]];
            float ang = i * 1.7f;
            x = arbol.x + std::cos(ang) * (0.6f + 0.8f * fase);
            z = arbol.z + std::sin(ang) * (0.6f + 0.8f * fase);
            yInicio = arbol.y + 2.6f * arbol.escala;
        }

        float ySuelo = AlturaTerrenoMalla(x, z);
        float y = yInicio + (ySuelo + 0.1f - yInicio) * fase;

        x += std::sin(tiempo * 1.5f + i) * 0.35f * fase;

        if (y < ySuelo + 0.05f)
        {
            continue;
        }

        DrawCubeV(
            Vector3{ x, y, z },
            Vector3{ 0.14f, 0.02f, 0.1f },
            Color{ 255, 190, 214, 255 }
        );
    }
}


//==================================================
// SENDEROS FISICOS
//==================================================
//
// El camino se hornea a partir del grafo de casillas: una cinta por cada
// conexion (con borde, tierra y huella central), discos en los nodos y una
// plaza empedrada donde el camino se bifurca o se junta. Sigue la altura de
// las casillas (el terreno se aplana bajo el) y no se dibuja donde el
// terreno cae al rio: ahi estan los puentes. Es una sola malla en GPU.

static const float SENDERO_SEMIANCHO = 1.5f;
static const float SENDERO_BORDE = 0.30f;
static const float SENDERO_PLAZA = 1.85f;

// Alturas sobre el suelo del camino (cada capa un escalon por encima de
// la anterior para evitar el z-fighting).
static const float LIFT_BORDE = 0.030f;
static const float LIFT_BORDE_NODO = 0.050f;
static const float LIFT_TIERRA = 0.070f;
static const float LIFT_TIERRA_NODO = 0.090f;
static const float LIFT_DETALLE = 0.108f;

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
        case ZONA_COSTA:
            return { Color{ 218, 196, 144, 255 }, Color{ 176, 158, 108, 255 }, 0.05f, 1.5f };
        case ZONA_BOSQUE_BAJO:
            return { Color{ 150, 118, 78, 255 }, Color{ 98, 96, 58, 255 }, 0.07f, 1.4f };
        case ZONA_RIO:
            return { Color{ 160, 132, 92, 255 }, Color{ 108, 106, 64, 255 }, 0.07f, 1.4f };
        case ZONA_RUINAS:
            return { Color{ 184, 178, 164, 255 }, Color{ 112, 110, 100, 255 }, 0.12f, 0.95f };
        case ZONA_CLARO_ROBLE:
            return { Color{ 162, 120, 78, 255 }, Color{ 104, 96, 54, 255 }, 0.07f, 1.4f };
        default:
            break;
    }

    return { Color{ 172, 144, 106, 255 }, Color{ 124, 112, 82, 255 }, 0.09f, 1.2f };
}

static const Color COLOR_BORDE_PASTO = Color{ 98, 146, 72, 255 };

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


// Cuadrilatero entre dos secciones (laterales lat0..lat1) a una altura extra.
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


// Anillo (empedrado de la plaza) con losas alternas.
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
    if (!terrenoConstruido)
    {
        ConstruirTerreno();
    }

    const Tablero& t = TableroReferencia();
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

            // Ligera curvatura para que no parezca trazado con regla.
            float amp = (Hash2(i * 31 + 7, j * 17 + 3) - 0.5f) * 0.50f;

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
                sec[s].y =
                    (a.posicion.y + (b.posicion.y - a.posicion.y) * u) - 0.25f;
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

                // El suelo debe estar al nivel del camino (si cae al rio, hay puente).
                bool firme = true;

                for (int lado = -1; lado <= 1; lado++)
                {
                    float h =
                        AlturaTerrenoMalla(
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
                Color bordeExt = Mezcla(borde, COLOR_BORDE_PASTO, 0.45f);

                // Borde: tierra oscura con pasto pisado hacia afuera.
                for (int lado = -1; lado <= 1; lado += 2)
                {
                    CuadroSendero(
                        m, sec[s], sec[s + 1],
                        lado * SENDERO_SEMIANCHO * 0.9f, lado * limiteBorde,
                        LIFT_BORDE, bordeExt
                    );
                }

                // Tierra / losas: dos carriles con tono propio por bloque.
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

                // Huella central mas clara (suelo muy pisado).
                if (detalle && a.zona != ZONA_RUINAS)
                {
                    CuadroSendero(
                        m, sec[s], sec[s + 1], -0.50f, 0.50f, LIFT_DETALLE,
                        Mezcla(tierra, Color{ 236, 220, 178, 255 }, 0.30f)
                    );
                }
            }

            // Guijarros en el borde (solo calidad alta).
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
                            Oscurecer(Color{ 150, 144, 134, 255 }, tono)
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
        MaterialSendero ms = MaterialSenderoZona(c.zona);
        float y = c.posicion.y - 0.25f;

        bool plaza = grado[i] >= 3;
        float semi = plaza ? SENDERO_PLAZA : SENDERO_SEMIANCHO;

        DiscoSendero(
            m, c.posicion.x, y, c.posicion.z,
            semi + SENDERO_BORDE, LIFT_BORDE_NODO,
            Mezcla(ms.borde, COLOR_BORDE_PASTO, 0.45f)
        );

        DiscoSendero(
            m, c.posicion.x, y, c.posicion.z,
            semi, LIFT_TIERRA_NODO, ms.tierra
        );

        if (plaza)
        {
            Color claro = Mezcla(ms.tierra, Color{ 214, 208, 194, 255 }, 0.55f);
            Color oscuro = Mezcla(ms.tierra, Color{ 140, 134, 124, 255 }, 0.55f);

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


static void DibujarSenderos()
{
    NivelCalidadGrafica nivel = CalidadDecoracion();

    if (!mallaSenderos.cargada || nivelSenderosConstruido != (int)nivel)
    {
        ConstruirMallaSenderos(nivel);
    }

    DibujarMallaGpu(mallaSenderos);
}


//==================================================
// RECURSOS
//==================================================

static void DescargarRecursosArboleda()
{
    DescargarMalla(mallaTerreno);
    DescargarMalla(mallaSenderos);
    nivelSenderosConstruido = -1;
    pasoFloracionTerreno = -1;
    DescargarMallasFlora();
    DescargarRoble();
    DescargarMalla(mallaRuinas);
    DescargarMalla(mallaSetas);
}


//==================================================
// CONSTRUCCION
//==================================================

static void ConstruirArboleda(
    Tablero& tablero
)
{
    // Casillas, conexiones y validacion: ver ConstruirRutaArboleda.
    ConstruirRutaArboleda(tablero);

    // Del espacio de diseno al mundo de juego.
    for (int i = 0; i < tablero.cantidadCasillas; i++)
    {
        Vector3& p = tablero.casillas[i].posicion;

        p.x *= ESCALA_ISLA;
        p.y *= ESCALA_ISLA;
        p.z *= ESCALA_ISLA;
    }
}


//==================================================
// DECORACION
//==================================================

static void DibujarDecoracionArboleda(
    const PartidaTablero& partida
)
{
    float floracion = partida.gimmickAnim;
    float tiempo = partida.tiempoTablero;

    // Todo el escenario esta en espacio de diseno: se reduce al mundo de juego.
    rlPushMatrix();
    rlScalef(ESCALA_ISLA, ESCALA_ISLA, ESCALA_ISLA);

    // MODELO FUTURO: la isla completa (terreno, vegetacion, roca) como GLB.
    DibujarTerreno(floracion);
    DibujarSenderos();
    DibujarFlora(tiempo, floracion);
    DibujarGranRoble(tiempo, floracion);
    DibujarRuinasJardin(tiempo, floracion);
    DibujarArcoRuinoso(floracion);
    DibujarMuelle(tiempo);
    DibujarPuertaCosta(tiempo);
    DibujarCabana();
    DibujarMolino(tiempo);
    DibujarAtalaya(tiempo);
    DibujarSetasGigantes(tiempo);
    DibujarCartelesBifurcacion();

    // Puentes donde el sendero cruza el rio.
    const Tablero& referencia = TableroReferencia();

    for (int i = 0; i < cantidadPuentesArboleda; i++)
    {
        const Casilla* a = referencia.ObtenerCasilla(puentesArboleda[i][0]);
        const Casilla* b = referencia.ObtenerCasilla(puentesArboleda[i][1]);

        if (a != nullptr && b != nullptr)
        {
            DibujarPuente(*a, *b);
        }
    }

    // Translucidos al final: agua, cascada y petalos.
    DibujarAgua(tiempo, floracion);
    DibujarCascada(tiempo);
    DibujarPetalosYLaguna(tiempo, floracion);

    rlPopMatrix();
}


// Luz calida y petalos que cruzan la pantalla durante la floracion.
static void DibujarCapaPantallaArboleda(
    const PartidaTablero& partida
)
{
    float f = partida.gimmickAnim;

    if (f < 0.01f)
    {
        return;
    }

    int ancho = GetScreenWidth();
    int alto = GetScreenHeight();

    DrawRectangleGradientV(
        0, 0, ancho, alto,
        Color{ 255, 196, 140, (unsigned char)(70 * f) },
        Color{ 255, 150, 170, (unsigned char)(40 * f) }
    );

    float escala = alto / 720.0f;
    float t = partida.tiempoTablero;

    int petalosPantalla = (int)std::ceil(46.0f * FactorCalidadGrafica(CalidadParticulas()));

    for (int i = 0; i < petalosPantalla; i++)
    {
        float vx = 22.0f + 18.0f * Hash2(i, 3);
        float vy = 38.0f + 40.0f * Hash2(i, 5);

        float x = std::fmod(Hash2(i, 1) * ancho + t * vx, (float)ancho + 40.0f) - 20.0f;
        float y = std::fmod(Hash2(i, 2) * alto + t * vy, (float)alto + 40.0f) - 20.0f;

        x += 18.0f * std::sin(t * 1.3f + i);

        float rx = (4.0f + 5.0f * Hash2(i, 4)) * escala;
        float ry = rx * 0.55f;

        DrawEllipse(
            (int)x, (int)y, rx, ry,
            Color{ 255, 192, 216, (unsigned char)(200 * f) }
        );
    }
}


//==================================================
// GIMMICK
//==================================================

static void ActualizarGimmickArboleda(
    PartidaTablero& partida,
    float deltaTime
)
{
    float objetivo =
        partida.gimmickActivo
        ? 1.0f
        : 0.0f;

    float paso =
        VELOCIDAD_ANIMACION_FLORACION * deltaTime;

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


static void AlIniciarRondaArboleda(
    PartidaTablero& partida
)
{
    partida.gimmickActivo =
        partida.rondaActual % 2 == 0;

    std::snprintf(
        partida.estadoGimmick,
        sizeof(partida.estadoGimmick),
        "%s",
        partida.gimmickActivo
            ? "FLORACION: CASILLAS VERDES +5 ESTA RONDA"
            : "BOSQUE EN REPOSO: FLORECE EN RONDAS PARES"
    );

    EstablecerMensajeGimmick(
        partida,
        partida.gimmickActivo
            ? "FLORACION! LAS CASILLAS VERDES PAGAN +5"
            : "EL BOSQUE DESCANSA: VERDES +3"
    );
}


static bool ResolverCasillaArboleda(
    PartidaTablero& partida,
    int participante
)
{
    EstadoJugadorPartidaTablero& jugador =
        partida.jugadores[participante];

    const Casilla* casilla =
        partida.tablero.ObtenerCasilla(jugador.casillaActual);

    if (
        casilla == nullptr ||
        casilla->tipo != CASILLA_POSITIVA ||
        !partida.gimmickActivo
    )
    {
        return false;
    }

    jugador.monedas += MONEDAS_VERDE_FLORACION;
    partida.variacionMonedasEvento = MONEDAS_VERDE_FLORACION;

    std::snprintf(
        partida.textoEvento,
        sizeof(partida.textoEvento),
        "FLORACION: CASILLA VERDE +%d MONEDAS",
        MONEDAS_VERDE_FLORACION
    );

    ReproducirSonidoTablero(partida, SONIDO_CASILLA_POSITIVA);

    return true;
}


// Valor aproximado de pisar una casilla (para que los bots elijan ruta).
static int ValorCasillaBotArboleda(
    const PartidaTablero& partida,
    const Casilla& casilla
)
{
    switch (casilla.tipo)
    {
        case CASILLA_POSITIVA:
            return partida.gimmickActivo ? MONEDAS_VERDE_FLORACION : 3;

        case CASILLA_REGALO: return 4;
        case CASILLA_NEGATIVA: return -3;
        case CASILLA_CAOS: return -3;
        case CASILLA_TIENDA: return 1;
        case CASILLA_EVENTO: return 1;
        case CASILLA_ESPECIAL: return 1;
        default: break;
    }

    return 0;
}


// Valor medio de los proximos pasos desde una casilla (todas las ramas).
static float ValorRutaBotArboleda(
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
            ValorRutaBotArboleda(
                partida, actual->conexiones[c].destino, profundidad - 1
            );
    }

    if (actual->cantidadConexiones > 0)
    {
        resto /= (float)actual->cantidadConexiones;
    }

    return (float)ValorCasillaBotArboleda(partida, *actual) + resto;
}


// Pasos minimos desde una casilla hasta otra (-1 si no hay camino).
static int PasosHastaArboleda(
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


// Con monedas para el trofeo, el bot va hacia el; sin ellas busca la ruta
// de casillas mas ricas. Siempre deja algo de azar.
static int DecidirRutaBotArboleda(
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
                PasosHastaArboleda(
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
            ValorRutaBotArboleda(
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


static const char* DescribirRutaArboleda(
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

    return descripcionRutaArboleda[casilla][opcion];
}


//==================================================
// DEFINICION
//==================================================

const DefinicionTablero& ObtenerDefinicionTableroArboleda()
{
    static DefinicionTablero definicion;
    static bool preparada = false;

    if (!preparada)
    {
        definicion.id = TABLERO_ISLA_ARBOLEDA;
        definicion.nombre = "ISLA ARBOLEDA";

        definicion.historia =
            "El Gran Roble regala un trofeo dorado a quien cuide su isla. "
            "Tras cada cosecha, el trofeo brota en otro claro del bosque.";

        definicion.objetivo =
            "Reune mas trofeos (20 monedas cada uno) en 5 rondas.";

        definicion.gimmick =
            "Floracion: en rondas pares las casillas verdes pagan +5.";

        definicion.dificultad = "FACIL";

        definicion.colorTema = Color{ 48, 145, 74, 255 };
        definicion.colorFondo = Color{ 86, 165, 210, 255 };

        // Camara diorama inclinada (la partida la mueve hacia el foco).
        // El mapa es grande: la camara sigue al jugador, da una vista
        // general al empezar la ronda y encuadra las bifurcaciones.
        definicion.camaraObjetivo = { 1.0f, 0.8f, -6.0f };
        definicion.camaraPosicion = { 1.0f, 17.8f, 9.0f };
        definicion.camaraFovy = 50.0f;
        definicion.camaraSeguimiento = 1.0f;
        definicion.camaraZoomGeneral = 2.4f;
        definicion.camaraEscalaVistaPrevia = 2.4f;

        // Landmark: el Gran Roble (enfoque al empezar la floracion).
        definicion.camaraLandmark =
            { ROBLE_X * ESCALA_ISLA, 3.0f, (ROBLE_Z + 2.0f) * ESCALA_ISLA };
        definicion.tieneLandmark = true;

        definicion.estiloCasilla = ObtenerEstiloCasillaPiedra();

        definicion.nombresZonas = NOMBRES_ZONAS_ARBOLEDA;
        definicion.cantidadZonas = CANTIDAD_ZONAS_ARBOLEDA;

        // Candidatas del trofeo: casillas de zonas distintas.
        TableroReferencia();

        definicion.casillasTrofeo = casillasTrofeoArboleda;
        definicion.cantidadCasillasTrofeo = cantidadTrofeosArboleda;

        definicion.costoTrofeo = 20;
        definicion.cantidadRondas = 5;

        definicion.Construir = ConstruirArboleda;
        definicion.DibujarDecoracion = DibujarDecoracionArboleda;
        definicion.DescargarRecursos = DescargarRecursosArboleda;
        definicion.DibujarCapaPantalla = DibujarCapaPantallaArboleda;
        definicion.AlIniciarRonda = AlIniciarRondaArboleda;
        definicion.ResolverCasilla = ResolverCasillaArboleda;
        definicion.ActualizarGimmick = ActualizarGimmickArboleda;
        definicion.DescribirOpcionRuta = DescribirRutaArboleda;
        definicion.DecidirRutaBot = DecidirRutaBotArboleda;

        preparada = true;
    }

    return definicion;
}
