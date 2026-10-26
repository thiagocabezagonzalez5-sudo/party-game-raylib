#include "Board/CatalogoTableros.h"

#include "Gameplay/PartidaTablero.h"
#include "Systems/CalidadGrafica.h"

#include "rlgl.h"

#include <cmath>
#include <cstdio>


//==================================================
// TORRE DEL RELOJ (tablero 4)
//==================================================
//
// La sala de maquinas de una torre de relojeria. El tablero es la ESFERA
// de un reloj gigante apoyada en un escenario de madera, con dos lobulos a
// los costados:
//
//   - ESFERA: anillo exterior de 24 casillas (2 por hora), sentido horario.
//     La casilla 0 (inicio) esta en "las 6", hacia la camara.
//   - LA CORONA: anillo interior de 12 casillas que se recorre al reves
//     (antihorario). Sus cuatro cruces (12, 3, 6 y 9) pertenecen a las
//     agujas.
//   - AGUJA DE LAS HORAS: atajo de las 12 a las 6 por el centro. Cruza la
//     corona y pasa por EL EJE.
//   - AGUJA DE LOS MINUTOS: atajo de las 3 a las 9 por el centro.
//   - TALLER DEL RELOJERO: lobulo oeste (de E4 a E8) con una tienda.
//   - CAMPANARIO: lobulo este (de E16 a E20) junto a la campana gigante.
//
// En cada bifurcacion la opcion 0 es la ruta larga y la 1 el atajo; en la
// corona la opcion 0 sigue la corona y la 1 baja al eje o sale a la esfera.
//
// GIMMICK: LA ESFERA Y LA CORONA GIRAN. Al empezar cada ronda desde la 2,
// las casillas de color (azules, verdes y rojas) de la esfera avanzan 1..3
// pasos en sentido horario y las de la corona 1..2 en sentido antihorario.
// Se giran los TIPOS en el mismo partida.tablero. Las especiales, la
// casilla 0 y los cruces de las agujas estan "atornillados", asi que los
// conteos por tipo nunca cambian (la regla obligatoria se cumple siempre).
//
// EVENTO (LA CAMPANADA), al azar entre dos:
//   a) CUERDA AL RELOJ: la esfera gira 1 paso ya mismo y +3 monedas.
//   b) MARCHA ATRAS: el jugador retrocede 3 casillas.
//
// El giro ya aplicado de cada anillo se deduce comparando los tipos
// actuales con su patron de diseno (aperiodico), asi que la logica no
// guarda estado estatico. Solo la animacion pendiente de la corona vive en
// un static que se reinicia al construir (la de la esfera es gimmickAnim).

static const int CASILLAS_ESFERA = 24;
static const int CASILLAS_CORONA = 12;
static const int CASILLAS_LOBULO = 7;
static const int MAX_TROFEOS_RELOJ = 16;

static const int MIN_PASOS_GIRO = 1;
static const int MAX_PASOS_GIRO = 3;
static const int MIN_PASOS_CORONA = 1;
static const int MAX_PASOS_CORONA = 2;
static const int PREMIO_CUERDA = 3;
static const int PASOS_MARCHA_ATRAS = 3;

static const float RADIO_ESFERA = 13.0f;       // centro de las casillas
static const float RADIO_CORONA = 6.6f;
static const float RADIO_AGUJA_EXTERIOR = 9.8f;
static const float RADIO_AGUJA_INTERIOR = 3.3f;
static const float LOBULO_CENTRO_X = 14.0f;    // taller en -x, campanario en +x
static const float LOBULO_RADIO = 7.05f;
static const float ALTURA_CASILLA_RELOJ = 0.25f;

static const float VELOCIDAD_GIRO = 1.6f;      // pasos animados por segundo

static const float PI_RELOJ = 3.14159265f;
static const float GRADOS = 57.29578f;

// Patrones de diseno (letra por casilla): N azul, + verde, - roja,
// R regalo, T tienda, V evento, I intercambio, D duelo, C caos, E especial.
// En la esfera giran N, + y - (salvo la casilla 0); en la corona giran las
// mismas letras salvo los cuatro cruces (indices multiplos de 3).
static const char* const PATRON_ESFERA = "N+R-+-N+VN+I-+TN+-N-D+R-";
static const char* const PATRON_CORONA = "++VI-+VRNV+-";
static const char* const PATRON_TALLER = "+T+VE-R";
static const char* const PATRON_CAMPANARIO = "+-D+VE+";


enum ZonaReloj
{
    ZONA_ESFERA = 0,
    ZONA_CORONA,
    ZONA_AGUJA_HORAS,
    ZONA_AGUJA_MINUTOS,
    ZONA_TALLER,
    ZONA_CAMPANARIO,
    CANTIDAD_ZONAS_RELOJ
};

static const char* const NOMBRES_ZONAS_RELOJ[CANTIDAD_ZONAS_RELOJ] =
{
    "ESFERA",
    "LA CORONA",
    "AGUJA DE LAS HORAS",
    "AGUJA DE LOS MINUTOS",
    "TALLER DEL RELOJERO",
    "CAMPANARIO"
};


// Casillas clave (las llena ConstruirRutaReloj; nada usa indices literales).
struct NodosReloj
{
    int esfera[CASILLAS_ESFERA] = {};
    int corona[CASILLAS_CORONA] = {};
    int taller[CASILLAS_LOBULO] = {};
    int campanario[CASILLAS_LOBULO] = {};

    // Agujas: [0] exterior de entrada, [1] interior de entrada,
    // [2] interior de salida, [3] exterior de salida.
    int horas[4] = {};
    int minutos[4] = {};
    int eje = 0;
};

static NodosReloj nodosReloj;

static int casillasTrofeoReloj[MAX_TROFEOS_RELOJ];
static int cantidadTrofeosReloj = 0;

static const char* descripcionRutaReloj[MAX_CASILLAS_TABLERO][2];

// Pasos de la corona que faltan animar (<= 0). Solo es visual: se reinicia
// en ConstruirTorreReloj (no en la ruta, que tambien construye la
// referencia del dibujo).
static float pendienteCoronaReloj = 0.0f;


//==================================================
// UTILIDADES
//==================================================

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


static bool LetraDeColor(char letra)
{
    return letra == 'N' || letra == '+' || letra == '-';
}


// Punto de la esfera: angulo 0 = las 12 (hacia -z), creciente en sentido
// horario visto desde arriba (las 3 hacia +x, las 6 hacia +z).
static Vector3 PuntoEsfera(
    float angulo,
    float radio,
    float y
)
{
    return { radio * std::sin(angulo), y, -radio * std::cos(angulo) };
}


// Angulo de la casilla i de la esfera (0 = las 6).
static float AnguloCasillaEsfera(int indice)
{
    return PI_RELOJ + indice * (2.0f * PI_RELOJ / CASILLAS_ESFERA);
}


// Angulo de la casilla j de la corona (0 = las 12, antihorario).
static float AnguloCasillaCorona(int indice)
{
    return -indice * (2.0f * PI_RELOJ / CASILLAS_CORONA);
}


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


static float Parpadeo(
    float tiempo,
    float velocidad
)
{
    return 0.5f + 0.5f * std::sin(tiempo * velocidad);
}


static float Hash2(int a, int b)
{
    unsigned int h =
        (unsigned int)(a * 374761393 + b * 668265263);

    h = (h ^ (h >> 13)) * 1274126177u;

    return (float)((h ^ (h >> 16)) & 0xFFFF) / 65535.0f;
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


//==================================================
// CONSTRUCCION DE LA RUTA
//==================================================

static int NodoReloj(
    Tablero& tablero,
    ZonaReloj zona,
    char letra,
    float x,
    float z
)
{
    int indice =
        tablero.AgregarCasilla(
            Vector3{ x, ALTURA_CASILLA_RELOJ, z },
            TipoDesdeLetra(letra)
        );

    if (indice >= 0)
    {
        tablero.casillas[indice].zona = (int)zona;
    }

    return indice;
}


static void DefinirOpcionesReloj(
    int casilla,
    const char* opcion0,
    const char* opcion1
)
{
    if (casilla < 0 || casilla >= MAX_CASILLAS_TABLERO)
    {
        return;
    }

    descripcionRutaReloj[casilla][0] = opcion0;
    descripcionRutaReloj[casilla][1] = opcion1;
}


// Posicion de la casilla k (0..6) del lobulo oeste (taller). El campanario
// es su reflejo puntual, asi los dos lobulos se recorren en sentido horario.
static Vector3 PosicionLobuloOeste(int k)
{
    Vector3 e4 = PuntoEsfera(AnguloCasillaEsfera(4), RADIO_ESFERA, 0.0f);

    float inicio = std::atan2(e4.z, e4.x + LOBULO_CENTRO_X);
    float arco = 2.0f * PI_RELOJ - 2.0f * inicio;
    float angulo = inicio + (k + 1) * arco / (CASILLAS_LOBULO + 1);

    return
    {
        -LOBULO_CENTRO_X + LOBULO_RADIO * std::cos(angulo),
        ALTURA_CASILLA_RELOJ,
        LOBULO_RADIO * std::sin(angulo)
    };
}


static void ConstruirRutaReloj(
    Tablero& t
)
{
    t.cantidadCasillas = 0;
    t.recorridoValido = false;

    for (int i = 0; i < MAX_CASILLAS_TABLERO; i++)
    {
        t.casillas[i] = Casilla{};
        descripcionRutaReloj[i][0] = nullptr;
        descripcionRutaReloj[i][1] = nullptr;
    }

    cantidadTrofeosReloj = 0;

    NodosReloj& n = nodosReloj;

    // --- ESFERA: la casilla 0 (inicio) va primero, en las 6 ---
    for (int i = 0; i < CASILLAS_ESFERA; i++)
    {
        Vector3 p = PuntoEsfera(AnguloCasillaEsfera(i), RADIO_ESFERA, 0.0f);
        n.esfera[i] = NodoReloj(t, ZONA_ESFERA, PATRON_ESFERA[i], p.x, p.z);
    }

    // --- CORONA: de las 12 hacia las 9 (antihorario) ---
    for (int j = 0; j < CASILLAS_CORONA; j++)
    {
        ZonaReloj zona = ZONA_CORONA;

        if (j == 0 || j == 6)
        {
            zona = ZONA_AGUJA_HORAS;
        }
        else if (j == 3 || j == 9)
        {
            zona = ZONA_AGUJA_MINUTOS;
        }

        Vector3 p = PuntoEsfera(AnguloCasillaCorona(j), RADIO_CORONA, 0.0f);
        n.corona[j] = NodoReloj(t, zona, PATRON_CORONA[j], p.x, p.z);
    }

    // --- AGUJA DE LAS HORAS (x = 0, de las 12 a las 6) con EL EJE ---
    const float re = RADIO_AGUJA_EXTERIOR;
    const float ri = RADIO_AGUJA_INTERIOR;

    n.horas[0] = NodoReloj(t, ZONA_AGUJA_HORAS, '-', 0.0f, -re);
    n.horas[1] = NodoReloj(t, ZONA_AGUJA_HORAS, '-', 0.0f, -ri);
    n.eje = NodoReloj(t, ZONA_AGUJA_HORAS, 'C', 0.0f, 0.0f);
    n.horas[2] = NodoReloj(t, ZONA_AGUJA_HORAS, '+', 0.0f, ri);
    n.horas[3] = NodoReloj(t, ZONA_AGUJA_HORAS, '+', 0.0f, re);

    // --- AGUJA DE LOS MINUTOS (z = 0, de las 3 a las 9) ---
    n.minutos[0] = NodoReloj(t, ZONA_AGUJA_MINUTOS, '+', re, 0.0f);
    n.minutos[1] = NodoReloj(t, ZONA_AGUJA_MINUTOS, '-', ri, 0.0f);
    n.minutos[2] = NodoReloj(t, ZONA_AGUJA_MINUTOS, '+', -ri, 0.0f);
    n.minutos[3] = NodoReloj(t, ZONA_AGUJA_MINUTOS, '-', -re, 0.0f);

    // --- LOBULOS: taller (oeste) y campanario (este, reflejo puntual) ---
    for (int k = 0; k < CASILLAS_LOBULO; k++)
    {
        Vector3 p = PosicionLobuloOeste(k);
        n.taller[k] = NodoReloj(t, ZONA_TALLER, PATRON_TALLER[k], p.x, p.z);
    }

    for (int k = 0; k < CASILLAS_LOBULO; k++)
    {
        Vector3 p = PosicionLobuloOeste(k);
        n.campanario[k] = NodoReloj(t, ZONA_CAMPANARIO, PATRON_CAMPANARIO[k], -p.x, -p.z);
    }

    const int* e = n.esfera;
    const int* c = n.corona;

    // --- Conexiones (opcion 0 primero) ---

    // Esfera: los lobulos son la ruta larga (se conectan antes); las
    // agujas son el atajo (se conectan despues).
    for (int i = 0; i < CASILLAS_ESFERA; i++)
    {
        if (i == 4)
        {
            t.ConectarCasillas(e[4], n.taller[0]);
        }
        else if (i == 16)
        {
            t.ConectarCasillas(e[16], n.campanario[0]);
        }

        t.ConectarCasillas(e[i], e[(i + 1) % CASILLAS_ESFERA]);

        if (i == 12)
        {
            t.ConectarCasillas(e[12], n.horas[0]);
        }
        else if (i == 18)
        {
            t.ConectarCasillas(e[18], n.minutos[0]);
        }
    }

    for (int k = 0; k + 1 < CASILLAS_LOBULO; k++)
    {
        t.ConectarCasillas(n.taller[k], n.taller[k + 1]);
        t.ConectarCasillas(n.campanario[k], n.campanario[k + 1]);
    }

    t.ConectarCasillas(n.taller[CASILLAS_LOBULO - 1], e[8]);
    t.ConectarCasillas(n.campanario[CASILLAS_LOBULO - 1], e[20]);

    // Entradas de las agujas a la corona.
    t.ConectarCasillas(n.horas[0], c[0]);
    t.ConectarCasillas(n.minutos[0], c[9]);

    // Corona: seguir la corona (0) o tomar la aguja (1).
    for (int j = 0; j < CASILLAS_CORONA; j++)
    {
        t.ConectarCasillas(c[j], c[(j + 1) % CASILLAS_CORONA]);

        if (j == 0)
        {
            t.ConectarCasillas(c[0], n.horas[1]);
        }
        else if (j == 9)
        {
            t.ConectarCasillas(c[9], n.minutos[1]);
        }
        else if (j == 6)
        {
            t.ConectarCasillas(c[6], n.horas[3]);
        }
        else if (j == 3)
        {
            t.ConectarCasillas(c[3], n.minutos[3]);
        }
    }

    // EL EJE: opcion 0 baja hacia las 6, opcion 1 dobla hacia las 9.
    t.ConectarCasillas(n.horas[1], n.eje);
    t.ConectarCasillas(n.minutos[1], n.eje);
    t.ConectarCasillas(n.eje, n.horas[2]);
    t.ConectarCasillas(n.eje, n.minutos[2]);
    t.ConectarCasillas(n.horas[2], c[6]);
    t.ConectarCasillas(n.minutos[2], c[3]);

    // Salidas de la corona a la esfera.
    t.ConectarCasillas(n.horas[3], e[0]);
    t.ConectarCasillas(n.minutos[3], e[6]);

    // --- Textos de las bifurcaciones ---
    DefinirOpcionesReloj(e[4], "TALLER DEL RELOJERO (LARGO)", "SEGUIR LA ESFERA (CORTO)");
    DefinirOpcionesReloj(e[16], "EL CAMPANARIO (LARGO)", "SEGUIR LA ESFERA (CORTO)");
    DefinirOpcionesReloj(e[12], "SEGUIR LA ESFERA (LARGO)", "AGUJA DE LAS HORAS (ATAJO)");
    DefinirOpcionesReloj(e[18], "SEGUIR LA ESFERA (LARGO)", "AGUJA DE LOS MINUTOS (ATAJO)");
    DefinirOpcionesReloj(c[0], "RECORRER LA CORONA", "BAJAR AL EJE");
    DefinirOpcionesReloj(c[9], "RECORRER LA CORONA", "BAJAR AL EJE");
    DefinirOpcionesReloj(c[6], "RECORRER LA CORONA", "SALIR A LAS 6");
    DefinirOpcionesReloj(c[3], "RECORRER LA CORONA", "SALIR A LAS 9");
    DefinirOpcionesReloj(n.eje, "BAJAR HACIA LAS 6", "DOBLAR HACIA LAS 9");

    // --- Candidatas del trofeo: 2-3 por zona ---
    const int candidatas[] =
    {
        e[3], e[10], e[21],
        c[4], c[10],
        n.horas[0], n.horas[3],
        n.minutos[0], n.minutos[3],
        n.taller[0], n.taller[3], n.taller[5],
        n.campanario[1], n.campanario[3], n.campanario[5]
    };

    for (int indice : candidatas)
    {
        if (cantidadTrofeosReloj < MAX_TROFEOS_RELOJ)
        {
            casillasTrofeoReloj[cantidadTrofeosReloj++] = indice;
        }
    }

    t.recorridoValido = t.ValidarRecorrido();
}


static void ConstruirTorreReloj(
    Tablero& tablero
)
{
    ConstruirRutaReloj(tablero);

    pendienteCoronaReloj = 0.0f;
}


// Tablero completo solo para el dibujo (las posiciones no cambian).
static const Tablero& TableroReferenciaReloj()
{
    static Tablero referencia;
    static bool lista = false;

    if (!lista)
    {
        ConstruirRutaReloj(referencia);
        lista = true;
    }

    return referencia;
}


//==================================================
// GIRO DE LA ESFERA Y DE LA CORONA
//==================================================

enum AnilloReloj
{
    ANILLO_ESFERA = 0,
    ANILLO_CORONA
};


// Casillas giratorias de un anillo en el orden de su giro (horario en la
// esfera, antihorario en la corona) y su letra de diseno. Devuelve cuantas.
static int ListarGiratoriasReloj(
    AnilloReloj anillo,
    int lista[CASILLAS_ESFERA],
    char letras[CASILLAS_ESFERA]
)
{
    int cantidad = 0;

    if (anillo == ANILLO_ESFERA)
    {
        for (int i = 1; i < CASILLAS_ESFERA; i++)
        {
            if (LetraDeColor(PATRON_ESFERA[i]))
            {
                lista[cantidad] = nodosReloj.esfera[i];
                letras[cantidad] = PATRON_ESFERA[i];
                cantidad++;
            }
        }
    }
    else
    {
        for (int j = 0; j < CASILLAS_CORONA; j++)
        {
            if (j % 3 != 0 && LetraDeColor(PATRON_CORONA[j]))
            {
                lista[cantidad] = nodosReloj.corona[j];
                letras[cantidad] = PATRON_CORONA[j];
                cantidad++;
            }
        }
    }

    return cantidad;
}


// La casilla gira con el patron de su anillo.
static bool CasillaGiratoriaReloj(
    int casilla
)
{
    int lista[CASILLAS_ESFERA];
    char letras[CASILLAS_ESFERA];

    for (int a = ANILLO_ESFERA; a <= ANILLO_CORONA; a++)
    {
        int cantidad = ListarGiratoriasReloj((AnilloReloj)a, lista, letras);

        for (int j = 0; j < cantidad; j++)
        {
            if (lista[j] == casilla)
            {
                return true;
            }
        }
    }

    return false;
}


// Desplaza los tipos de las casillas giratorias `pasos` posiciones en el
// sentido del anillo: el tipo de la posicion j pasa a la j + pasos.
static void GirarAnilloReloj(
    Tablero& tablero,
    AnilloReloj anillo,
    int pasos
)
{
    int lista[CASILLAS_ESFERA];
    char letras[CASILLAS_ESFERA];
    int cantidad = ListarGiratoriasReloj(anillo, lista, letras);

    if (cantidad <= 0)
    {
        return;
    }

    for (int j = 0; j < cantidad; j++)
    {
        if (lista[j] < 0 || lista[j] >= tablero.cantidadCasillas)
        {
            return;
        }
    }

    TipoCasilla tipos[CASILLAS_ESFERA];

    for (int j = 0; j < cantidad; j++)
    {
        tipos[j] = tablero.casillas[lista[j]].tipo;
    }

    pasos = ((pasos % cantidad) + cantidad) % cantidad;

    for (int j = 0; j < cantidad; j++)
    {
        tablero.casillas[lista[(j + pasos) % cantidad]].tipo = tipos[j];
    }
}


// Giro acumulado (0..cantidad-1) deducido de los tipos actuales: el k tal
// que la posicion j tiene el tipo de diseno de la posicion j - k. Los
// patrones no son periodicos, asi que k es unico.
static int GiroActualAnilloReloj(
    const Tablero& tablero,
    AnilloReloj anillo,
    int& cantidad
)
{
    int lista[CASILLAS_ESFERA];
    char letras[CASILLAS_ESFERA];
    cantidad = ListarGiratoriasReloj(anillo, lista, letras);

    for (int j = 0; j < cantidad; j++)
    {
        if (lista[j] < 0 || lista[j] >= tablero.cantidadCasillas)
        {
            return 0;
        }
    }

    for (int k = 0; k < cantidad; k++)
    {
        bool coincide = true;

        for (int j = 0; j < cantidad && coincide; j++)
        {
            char origen = letras[(j - k + cantidad) % cantidad];

            coincide =
                tablero.casillas[lista[j]].tipo == TipoDesdeLetra(origen);
        }

        if (coincide)
        {
            return k;
        }
    }

    return 0;
}


// Pasos pendientes con un "tic" de relojeria (cada paso acelera y frena).
static float TicPendiente(float pendiente)
{
    if (pendiente >= 0.0f)
    {
        return 0.0f;
    }

    float base = std::floor(pendiente);
    return base + Suave01(pendiente - base);
}


// Angulo visual (grados, sentido del giro) de un anillo: el aplicado menos
// lo que falta animar.
static float AnguloVisualAnilloReloj(
    const PartidaTablero& partida,
    AnilloReloj anillo
)
{
    int cantidad = 0;
    float giro = (float)GiroActualAnilloReloj(partida.tablero, anillo, cantidad);

    if (cantidad <= 0)
    {
        return 0.0f;
    }

    float pendiente =
        anillo == ANILLO_ESFERA
        ? partida.gimmickAnim
        : pendienteCoronaReloj;

    return (giro + TicPendiente(pendiente)) * 360.0f / (float)cantidad;
}


// Gira los anillos y deja los pasos pendientes de animar.
static void AplicarGiroReloj(
    PartidaTablero& partida,
    int pasosEsfera,
    int pasosCorona
)
{
    if (pasosEsfera > 0)
    {
        GirarAnilloReloj(partida.tablero, ANILLO_ESFERA, pasosEsfera);

        if (partida.gimmickAnim > 0.0f)
        {
            partida.gimmickAnim = 0.0f;
        }

        partida.gimmickAnim -= (float)pasosEsfera;
    }

    if (pasosCorona > 0)
    {
        GirarAnilloReloj(partida.tablero, ANILLO_CORONA, pasosCorona);
        pendienteCoronaReloj -= (float)pasosCorona;
    }

    partida.gimmickActivo = true;
}


//==================================================
// COLORES
//==================================================

static const Color BRONCE_OSCURO = { 86, 56, 32, 255 };
static const Color BRONCE = { 146, 98, 50, 255 };
static const Color LATON = { 202, 158, 80, 255 };
static const Color ORO_RELOJ = { 240, 200, 96, 255 };
static const Color MARFIL = { 228, 216, 186, 255 };
static const Color MARFIL_PISTA = { 204, 188, 152, 255 };
static const Color TINTA = { 56, 44, 40, 255 };
static const Color ACERO = { 148, 150, 162, 255 };
static const Color ACERO_AZUL = { 58, 76, 128, 255 };
static const Color COBRE = { 170, 96, 62, 255 };
static const Color MADERA = { 96, 64, 40, 255 };
static const Color MADERA_CLARA = { 122, 84, 52, 255 };
static const Color MADERA_OSCURA = { 48, 32, 24, 255 };
static const Color MADERA_ROJIZA = { 108, 54, 38, 255 };
static const Color PIEDRA = { 74, 72, 88, 255 };
static const Color PIEDRA_CLARA = { 104, 100, 112, 255 };
static const Color PIEDRA_JUNTA = { 40, 40, 54, 255 };
static const Color LUZ_VENTANA = { 255, 210, 132, 255 };
static const Color LUZ_LUNA = { 240, 236, 214, 255 };


//==================================================
// LUZ HORNEADA Y PIEZAS EMISIVAS
//==================================================
//
// Las mallas no tienen luces: cada triangulo se oscurece segun su normal
// (como el terreno de Arboleda). Lo que brilla (ventanas de la ciudad,
// luna, velas, lamparas) se marca con alfa 254 y no se oscurece.

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
// GEOMETRIA DE LA SALA
//==================================================
//
// Alturas (capas separadas >= 0.02 para que no titilen de lejos):
//   suelo de la sala -0.30, escenario -0.06, plataformas de los lobulos y
//   cara del reloj 0, marcas de la cara y placa de la corona 0.02, detalles
//   de la corona 0.04, manecillas 0.06, sendero desde 0.09.

static const float SUELO_SALA_Y = -0.30f;
static const float ESCENARIO_Y = -0.06f;

static const float RADIO_CARA = 15.4f;          // cara de marfil
static const float RADIO_BISEL = 15.8f;         // bisel fijo de laton
static const float RADIO_ARO = 16.4f;           // aro giratorio
static const float RADIO_DIENTES_ARO = 16.8f;
static const float RADIO_PINON = 0.95f;
static const float RADIO_PINONES = 17.65f;      // centro de los pinones del aro
static const float RADIO_ESCENARIO = 18.9f;
static const float RADIO_PLATAFORMA = 9.6f;     // plataforma de cada lobulo

static const float CORONA_INTERIOR = 3.9f;      // placa giratoria de la corona
static const float CORONA_EXTERIOR = 9.3f;

// Muros (cara interior) y largo de la sala.
static const float MURO_X = 33.0f;
static const float MURO_FONDO_Z = -21.5f;
static const float MURO_FRENTE_Z = 36.0f;
static const float PISO_FRENTE_Z = 64.0f;
static const float GROSOR_MURO = 1.2f;
static const float ALTO_MURO = 17.5f;

// Ventanales (arco de medio punto sobre un rectangulo).
static const float VENTANA_B0 = 3.0f;
static const float VENTANA_B1 = 10.0f;

// Landmarks.
static const float PENDULO_X = 0.0f;
static const float PENDULO_Z = -20.3f;
static const float CAMPANARIO_X = 22.0f;
static const float CAMPANARIO_Z = -14.0f;
static const float MESA_LOBULO_X = 17.9f;       // taller en -x, campanas en +x
static const float ENGRANAJES_X = 28.0f;
static const float ENGRANAJES_Z = 1.0f;


// Pared vista desde dentro: o = pie de la pared sobre el suelo, u = eje a
// lo largo, n = normal hacia la sala. Las coordenadas locales son (u, b, d):
// u a lo largo, b altura sobre el suelo, d hacia la sala.
struct MarcoPared
{
    Vector3 o;
    Vector3 u;
    Vector3 n;
};

enum MuroReloj
{
    MURO_FONDO = 0,
    MURO_OESTE,
    MURO_ESTE,
    CANTIDAD_MUROS_RELOJ
};


static MarcoPared MarcoMuro(MuroReloj muro)
{
    switch (muro)
    {
        case MURO_FONDO:
            return { { 0.0f, SUELO_SALA_Y, MURO_FONDO_Z }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } };
        case MURO_OESTE:
            return { { -MURO_X, SUELO_SALA_Y, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f, 0.0f } };
        case MURO_ESTE:
        case CANTIDAD_MUROS_RELOJ:
            break;
    }

    return { { MURO_X, SUELO_SALA_Y, 0.0f }, { 0.0f, 0.0f, 1.0f }, { -1.0f, 0.0f, 0.0f } };
}


static Vector3 PuntoPared(
    const MarcoPared& p,
    float u,
    float b,
    float d
)
{
    return
    {
        p.o.x + p.u.x * u + p.n.x * d,
        p.o.y + b,
        p.o.z + p.u.z * u + p.n.z * d
    };
}


// Caja alineada a la pared: ancho a lo largo, alto y fondo hacia la sala.
static void CajaPared(
    ConstructorMalla& m,
    const MarcoPared& p,
    float u,
    float b,
    float d,
    float ancho,
    float alto,
    float fondo,
    Color color
)
{
    Vector3 centro = PuntoPared(p, u, b, d);

    Vector3 tamano =
        std::fabs(p.u.x) > 0.5f
        ? Vector3{ ancho, alto, fondo }
        : Vector3{ fondo, alto, ancho };

    m.Caja(centro, tamano, color);
}


// Cuadrilatero de dos caras.
static void CuadroDobleCara(
    ConstructorMalla& m,
    Vector3 a,
    Vector3 b,
    Vector3 c,
    Vector3 d,
    Color color
)
{
    m.TrianguloDobleCara(a, b, c, color);
    m.TrianguloDobleCara(a, c, d, color);
}


// Rectangulo plano sobre la pared (a la distancia d de la cara).
static void PlacaPared(
    ConstructorMalla& m,
    const MarcoPared& p,
    float u0,
    float b0,
    float u1,
    float b1,
    float d,
    Color color
)
{
    CuadroDobleCara(
        m,
        PuntoPared(p, u0, b0, d),
        PuntoPared(p, u1, b0, d),
        PuntoPared(p, u1, b1, d),
        PuntoPared(p, u0, b1, d),
        color
    );
}


// Corona circular sobre la pared (relojes, aros de ventanas).
static void AnilloPared(
    ConstructorMalla& m,
    const MarcoPared& p,
    float u,
    float b,
    float d,
    float radioInterior,
    float radioExterior,
    int lados,
    Color color
)
{
    for (int i = 0; i < lados; i++)
    {
        float a0 = 2.0f * PI_RELOJ * i / lados;
        float a1 = 2.0f * PI_RELOJ * (i + 1) / lados;

        Vector3 i0 = PuntoPared(p, u + std::cos(a0) * radioInterior, b + std::sin(a0) * radioInterior, d);
        Vector3 i1 = PuntoPared(p, u + std::cos(a1) * radioInterior, b + std::sin(a1) * radioInterior, d);
        Vector3 e0 = PuntoPared(p, u + std::cos(a0) * radioExterior, b + std::sin(a0) * radioExterior, d);
        Vector3 e1 = PuntoPared(p, u + std::cos(a1) * radioExterior, b + std::sin(a1) * radioExterior, d);

        if (radioInterior <= 0.0f)
        {
            m.TrianguloDobleCara(i0, e0, e1, color);
            continue;
        }

        CuadroDobleCara(m, i0, e0, e1, i1, color);
    }
}


// Corona plana horizontal entre dos radios alrededor de (cx, cz).
static void AnilloPlanoCentro(
    ConstructorMalla& m,
    float cx,
    float cz,
    float radioInterior,
    float radioExterior,
    float y,
    int lados,
    Color color
)
{
    for (int i = 0; i < lados; i++)
    {
        float a0 = 2.0f * PI_RELOJ * i / lados;
        float a1 = 2.0f * PI_RELOJ * (i + 1) / lados;

        Vector3 i0 = { cx + std::sin(a0) * radioInterior, y, cz - std::cos(a0) * radioInterior };
        Vector3 i1 = { cx + std::sin(a1) * radioInterior, y, cz - std::cos(a1) * radioInterior };
        Vector3 e0 = { cx + std::sin(a0) * radioExterior, y, cz - std::cos(a0) * radioExterior };
        Vector3 e1 = { cx + std::sin(a1) * radioExterior, y, cz - std::cos(a1) * radioExterior };

        if (radioInterior <= 0.0f)
        {
            m.TrianguloDobleCara(i0, e0, e1, color);
            continue;
        }

        CuadroDobleCara(m, i0, e0, e1, i1, color);
    }
}


static void AnilloPlanoMalla(
    ConstructorMalla& m,
    float radioInterior,
    float radioExterior,
    float y,
    int lados,
    Color color
)
{
    AnilloPlanoCentro(m, 0.0f, 0.0f, radioInterior, radioExterior, y, lados, color);
}


// Pared vertical circular sin tapas (borde de un aro).
static void ParedCircularMalla(
    ConstructorMalla& m,
    float radio,
    float y0,
    float y1,
    int lados,
    Color color
)
{
    for (int i = 0; i < lados; i++)
    {
        float a0 = 2.0f * PI_RELOJ * i / lados;
        float a1 = 2.0f * PI_RELOJ * (i + 1) / lados;

        CuadroDobleCara(
            m,
            PuntoEsfera(a0, radio, y0),
            PuntoEsfera(a1, radio, y0),
            PuntoEsfera(a1, radio, y1),
            PuntoEsfera(a0, radio, y1),
            color
        );
    }
}


// Caja orientada en la direccion radial del angulo de la esfera: `local`
// se mide con las 12 hacia -z (x = lateral, z = -radio).
static void CajaRadialMalla(
    ConstructorMalla& m,
    float angulo,
    Vector3 local,
    Vector3 tamano,
    Color color
)
{
    TransformacionMalla t;
    t.giroY = -angulo * GRADOS;

    m.EstablecerTransformacion(t);
    m.Caja(local, tamano, color);
    m.QuitarTransformacion();
}


// Caja girada en Y (grados) alrededor de su centro.
static void CajaGirada(
    ConstructorMalla& m,
    float x,
    float y,
    float z,
    float giroGrados,
    Vector3 tamano,
    Color color
)
{
    TransformacionMalla t;
    t.tx = x;
    t.ty = y;
    t.tz = z;
    t.giroY = giroGrados;

    m.EstablecerTransformacion(t);
    m.Caja(Vector3{ 0.0f, 0.0f, 0.0f }, tamano, color);
    m.QuitarTransformacion();
}


// Trazo plano horizontal entre dos puntos (x, z).
static void TrazoPlano(
    ConstructorMalla& m,
    float x0,
    float z0,
    float x1,
    float z1,
    float ancho,
    float y,
    Color color
)
{
    float dx = x1 - x0;
    float dz = z1 - z0;
    float largo = std::sqrt(dx * dx + dz * dz);

    if (largo < 0.0001f)
    {
        return;
    }

    float nx = -dz / largo * ancho * 0.5f;
    float nz = dx / largo * ancho * 0.5f;

    CuadroDobleCara(
        m,
        Vector3{ x0 + nx, y, z0 + nz },
        Vector3{ x1 + nx, y, z1 + nz },
        Vector3{ x1 - nx, y, z1 - nz },
        Vector3{ x0 - nx, y, z0 - nz },
        color
    );
}


// Viga de seccion cuadrada entre dos puntos cualesquiera.
static void Viga(
    ConstructorMalla& m,
    Vector3 a,
    Vector3 b,
    float radio,
    Color color
)
{
    m.Cilindro(a, b, radio, radio, 4, color);
}


//==================================================
// DISTANCIAS Y SUELO
//==================================================

static float DistanciaSegmento(
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


// Distancia al sendero mas cercano (segmentos entre casillas conectadas).
static float DistanciaSenderoReloj(
    float x,
    float z
)
{
    const Tablero& t = TableroReferenciaReloj();
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


// Algo alto delante de una casilla (hacia la camara) la taparia.
static bool OcultaCasillaReloj(float x, float z)
{
    const Tablero& t = TableroReferenciaReloj();

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


static bool SobrePlataformaLobulo(float x, float z)
{
    float dx = std::fabs(x) - LOBULO_CENTRO_X;

    return
        dx * dx + z * z < RADIO_PLATAFORMA * RADIO_PLATAFORMA &&
        x * x + z * z > RADIO_CARA * RADIO_CARA;
}


// Altura del suelo: cara y lobulos a 0, escenario y suelo de la sala debajo.
static float AlturaSueloReloj(float x, float z)
{
    float r2 = x * x + z * z;

    if (r2 < RADIO_CARA * RADIO_CARA || SobrePlataformaLobulo(x, z))
    {
        return 0.0f;
    }

    if (r2 < RADIO_ESCENARIO * RADIO_ESCENARIO)
    {
        return ESCENARIO_Y;
    }

    return SUELO_SALA_Y;
}


//==================================================
// ZONAS RESERVADAS (landmarks, muebles y carteles)
//==================================================

struct ZonaReservadaReloj
{
    float x;
    float z;
    float radio;
};

static const ZonaReservadaReloj ZONAS_RESERVADAS_RELOJ[] =
{
    { PENDULO_X, PENDULO_Z, 4.4f },
    { CAMPANARIO_X, CAMPANARIO_Z, 4.4f },
    { -MESA_LOBULO_X, 0.0f, 1.7f },
    { MESA_LOBULO_X, 0.0f, 1.7f },
    { ENGRANAJES_X, ENGRANAJES_Z, 4.6f },
    { ENGRANAJES_X - 1.5f, -6.4f, 2.6f },
    { -30.6f, -2.0f, 2.6f },          // banco largo del taller
    { -30.6f, 3.0f, 2.6f },
    { -26.8f, 11.8f, 2.2f },          // mesa del relojero
    { -27.6f, -9.4f, 2.0f },          // torno
    { -22.2f, 11.0f, 1.7f },          // vitrina de la tienda
    { 0.0f, 27.0f, 3.0f },            // mesa de lectura del frente
    { -16.0f, -17.5f, 2.0f },         // caja fuerte de piezas
    { 13.5f, -18.4f, 1.6f },          // carro de herramientas
    { -24.0f, -16.3f, 4.0f }          // engranajes del piso (rincon oeste)
};


static bool DespejadoReloj(float x, float z, float radio)
{
    for (const ZonaReservadaReloj& r : ZONAS_RESERVADAS_RELOJ)
    {
        float dx = x - r.x;
        float dz = z - r.z;
        float limite = r.radio + radio;

        if (dx * dx + dz * dz < limite * limite)
        {
            return false;
        }
    }

    // Pinones del aro.
    for (int k = 0; k < 4; k++)
    {
        float angulo = (k < 2 ? PI_RELOJ : 0.0f) + (k % 2 == 0 ? -1.0f : 1.0f) * PI_RELOJ / 6.0f;
        Vector3 p = PuntoEsfera(angulo, RADIO_PINONES, 0.0f);
        float dx = x - p.x;
        float dz = z - p.z;
        float limite = RADIO_PINON + 0.4f + radio;

        if (dx * dx + dz * dz < limite * limite)
        {
            return false;
        }
    }

    return true;
}


//==================================================
// PIEZAS DE PARED
//==================================================

// Arco (parte de una corona) sobre la pared, de a0 a a1 (0 = hacia +u).
static void ArcoPared(
    ConstructorMalla& m,
    const MarcoPared& p,
    float u,
    float b,
    float d,
    float radioInterior,
    float radioExterior,
    float a0,
    float a1,
    int lados,
    Color color
)
{
    for (int i = 0; i < lados; i++)
    {
        float t0 = a0 + (a1 - a0) * i / lados;
        float t1 = a0 + (a1 - a0) * (i + 1) / lados;

        CuadroDobleCara(
            m,
            PuntoPared(p, u + std::cos(t0) * radioInterior, b + std::sin(t0) * radioInterior, d),
            PuntoPared(p, u + std::cos(t0) * radioExterior, b + std::sin(t0) * radioExterior, d),
            PuntoPared(p, u + std::cos(t1) * radioExterior, b + std::sin(t1) * radioExterior, d),
            PuntoPared(p, u + std::cos(t1) * radioInterior, b + std::sin(t1) * radioInterior, d),
            color
        );
    }
}


static void VerticeColor(
    ConstructorMalla& m,
    Vector3 v,
    Color c
)
{
    m.Vertice(v.x, v.y, v.z, c);
}


// Rectangulo de pared con degradado vertical (dos caras).
static void PlacaDegradada(
    ConstructorMalla& m,
    const MarcoPared& p,
    float u0,
    float b0,
    float u1,
    float b1,
    float d,
    Color abajo,
    Color arriba
)
{
    Vector3 a = PuntoPared(p, u0, b0, d);
    Vector3 b = PuntoPared(p, u1, b0, d);
    Vector3 c = PuntoPared(p, u1, b1, d);
    Vector3 e = PuntoPared(p, u0, b1, d);

    VerticeColor(m, a, abajo);
    VerticeColor(m, b, abajo);
    VerticeColor(m, c, arriba);
    VerticeColor(m, a, abajo);
    VerticeColor(m, c, arriba);
    VerticeColor(m, e, arriba);

    VerticeColor(m, a, abajo);
    VerticeColor(m, c, arriba);
    VerticeColor(m, b, abajo);
    VerticeColor(m, a, abajo);
    VerticeColor(m, e, arriba);
    VerticeColor(m, c, arriba);
}


// Cilindro con el eje perpendicular a la pared (relojes, engranajes).
static void DiscoPared(
    ConstructorMalla& m,
    const MarcoPared& p,
    float u,
    float b,
    float d0,
    float d1,
    float radio,
    int lados,
    Color color
)
{
    m.Cilindro(PuntoPared(p, u, b, d0), PuntoPared(p, u, b, d1), radio, radio, lados, color);
}


//==================================================
// FONDO PINTADO: LA CIUDAD DE NOCHE (malla horneada)
//==================================================
//
// Detras de los ventanales: cielo nocturno con estrellas y luna llena, y
// la ciudad en dos capas con ventanas encendidas. Son tres telones grandes
// (fondo y costados) que tambien cubren lo poco que asoma por encima de los
// muros en la vista general. No reciben luz horneada.

static const float TELON_Z = -46.0f;
static const float TELON_X = 56.0f;
static const float TELON_FRENTE_Z = 76.0f;
static const float TELON_Y0 = -44.0f;
static const float TELON_Y1 = 66.0f;

static const float LUNA_X = 14.0f;
static const float LUNA_Y = 3.6f;
static const float LUNA_RADIO = 3.2f;

static MallaGpu mallaTelonReloj;
static int nivelTelonReloj = -1;


// 0 = fondo, 1 = oeste, 2 = este. b es la altura absoluta.
static MarcoPared MarcoTelon(int lado)
{
    if (lado == 1)
    {
        return { { -TELON_X, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f, 0.0f } };
    }

    if (lado == 2)
    {
        return { { TELON_X, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { -1.0f, 0.0f, 0.0f } };
    }

    return { { 0.0f, 0.0f, TELON_Z }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } };
}


static void CieloTelon(
    ConstructorMalla& m,
    const MarcoPared& p,
    float u0,
    float u1
)
{
    const float alturas[] = { TELON_Y0, -18.0f, -4.0f, 6.0f, 20.0f, 38.0f, TELON_Y1 };

    const Color colores[] =
    {
        { 30, 24, 40, 255 },
        { 58, 42, 64, 255 },
        { 66, 62, 108, 255 },
        { 44, 50, 98, 255 },
        { 26, 32, 74, 255 },
        { 14, 18, 50, 255 },
        { 8, 10, 30, 255 }
    };

    for (int k = 0; k + 1 < 7; k++)
    {
        PlacaDegradada(m, p, u0, alturas[k], u1, alturas[k + 1], 0.0f, colores[k], colores[k + 1]);
    }
}


static void EstrellasTelon(
    ConstructorMalla& m,
    const MarcoPared& p,
    float u0,
    float u1,
    int lado,
    int cantidad
)
{
    for (int i = 0; i < cantidad; i++)
    {
        float u = u0 + (u1 - u0) * Hash2(i, 300 + lado);
        float h = Hash2(i, 400 + lado);
        float b = 2.0f + 60.0f * h * h;
        float s = 0.10f + 0.22f * Hash2(i, 500 + lado);

        float du = u - LUNA_X;
        float db = b - LUNA_Y;

        if (lado == 0 && du * du + db * db < 49.0f)
        {
            continue;
        }

        Color c = Mezcla(Color{ 170, 180, 230, 255 }, Color{ 255, 248, 222, 255 }, Hash2(i, 600 + lado));

        CuadroDobleCara(
            m,
            PuntoPared(p, u - s, b, 0.06f),
            PuntoPared(p, u, b - s, 0.06f),
            PuntoPared(p, u + s, b, 0.06f),
            PuntoPared(p, u, b + s, 0.06f),
            c
        );
    }
}


static void LunaTelon(
    ConstructorMalla& m,
    const MarcoPared& p
)
{
    AnilloPared(m, p, LUNA_X, LUNA_Y, 0.04f, LUNA_RADIO + 1.0f, LUNA_RADIO * 2.1f, 40, Color{ 62, 66, 116, 255 });
    AnilloPared(m, p, LUNA_X, LUNA_Y, 0.05f, LUNA_RADIO, LUNA_RADIO + 1.0f, 40, Color{ 96, 100, 148, 255 });
    AnilloPared(m, p, LUNA_X, LUNA_Y, 0.08f, 0.0f, LUNA_RADIO, 40, LUZ_LUNA);

    const float crateres[4][3] =
    {
        { -0.9f, 0.8f, 0.8f },
        { 1.1f, -0.4f, 0.6f },
        { -0.2f, -1.4f, 0.5f },
        { 1.4f, 1.3f, 0.35f }
    };

    for (const auto& c : crateres)
    {
        AnilloPared(m, p, LUNA_X + c[0], LUNA_Y + c[1], 0.10f, 0.0f, c[2], 14, Color{ 212, 206, 184, 255 });
    }
}


// Siluetas de la ciudad. La capa lejana es mas alta y azulada; la cercana,
// oscura y llena de ventanas encendidas.
static void EdificiosTelon(
    ConstructorMalla& m,
    const MarcoPared& p,
    float u0,
    float u1,
    int lado,
    int capa,
    NivelCalidadGrafica nivel
)
{
    const bool lejos = capa == 0;
    const float d = lejos ? 0.14f : 0.28f;
    const float topeMin = lejos ? -8.0f : -15.0f;
    const float topeMax = lejos ? 5.0f : -2.5f;
    const Color cuerpo = lejos ? Color{ 40, 42, 76, 255 } : Color{ 22, 21, 36, 255 };
    const float probabilidad = lejos ? 0.10f : 0.30f;

    int filas = nivel == CALIDAD_ALTA ? 12 : (nivel == CALIDAD_MEDIA ? 8 : 4);

    if (lejos && nivel == CALIDAD_BAJA)
    {
        filas = 0;
    }

    float u = u0;

    for (int k = 0; u < u1; k++)
    {
        int semilla = 700 + lado * 10 + capa;
        float ancho = (lejos ? 3.0f : 2.2f) + 4.0f * Hash2(k, semilla);
        float tope = topeMin + (topeMax - topeMin) * Hash2(k, semilla + 100);
        float ua = u + 0.1f;
        float ub = u + ancho;

        if (ub > u1)
        {
            ub = u1;
        }

        float um = (ua + ub) * 0.5f;
        float w = ub - ua;
        Color c = Oscurecer(cuerpo, 0.85f + 0.3f * Hash2(k, semilla + 200));

        PlacaPared(m, p, ua, TELON_Y0, ub, tope, d, c);

        int techo = (int)(Hash2(k, semilla + 300) * 6.0f);

        switch (techo)
        {
            case 1:
                // Dos aguas.
                m.TrianguloDobleCara(
                    PuntoPared(p, ua, tope, d),
                    PuntoPared(p, ub, tope, d),
                    PuntoPared(p, um, tope + w * 0.35f, d),
                    c
                );
                break;

            case 2:
                // Cupula con linterna.
                ArcoPared(m, p, um, tope, d, 0.0f, w * 0.4f, 0.0f, PI_RELOJ, 8, c);
                PlacaPared(m, p, um - 0.12f, tope + w * 0.38f, um + 0.12f, tope + w * 0.38f + 1.0f, d, c);
                break;

            case 3:
                // Campanario lejano con aguja.
                PlacaPared(m, p, um - w * 0.2f, tope, um + w * 0.2f, tope + 2.6f, d, c);
                m.TrianguloDobleCara(
                    PuntoPared(p, um - w * 0.24f, tope + 2.6f, d),
                    PuntoPared(p, um + w * 0.24f, tope + 2.6f, d),
                    PuntoPared(p, um, tope + 6.0f, d),
                    c
                );
                break;

            case 4:
                // Chimeneas.
                PlacaPared(m, p, ua + w * 0.2f, tope, ua + w * 0.2f + 0.4f, tope + 1.1f, d, c);
                PlacaPared(m, p, ub - w * 0.3f, tope, ub - w * 0.3f + 0.35f, tope + 0.8f, d, c);
                break;

            default:
                // Azotea con parapeto.
                PlacaPared(m, p, ua - 0.08f, tope, ub + 0.08f, tope + 0.25f, d, c);
                break;
        }

        // Ventanas encendidas (las de abajo no se ven por los ventanales).
        for (int f = 0; f < filas; f++)
        {
            float b = tope - 1.0f - f * 1.15f;
            int columna = 0;

            for (float uu = ua + 0.4f; uu < ub - 0.4f; uu += 0.8f, columna++)
            {
                if (Hash2(k * 37 + columna, f + capa * 50 + lado * 400) > probabilidad)
                {
                    continue;
                }

                Color luz = Mezcla(LUZ_VENTANA, Color{ 255, 238, 186, 255 }, Hash2(f + k, columna));

                if (lejos)
                {
                    luz = Oscurecer(luz, 0.62f);
                }

                PlacaPared(m, p, uu, b, uu + 0.34f, b + 0.46f, d + 0.03f, luz);
            }
        }

        u += ancho;
    }
}


static void ConstruirMallaTelonReloj(NivelCalidadGrafica nivel)
{
    static ConstructorMalla m;
    m.Limpiar();

    int estrellas = nivel == CALIDAD_ALTA ? 160 : (nivel == CALIDAD_MEDIA ? 100 : 50);

    for (int lado = 0; lado < 3; lado++)
    {
        MarcoPared p = MarcoTelon(lado);
        float u0 = lado == 0 ? -TELON_X : TELON_Z;
        float u1 = lado == 0 ? TELON_X : TELON_FRENTE_Z;

        CieloTelon(m, p, u0, u1);
        EstrellasTelon(m, p, u0, u1, lado, estrellas);

        if (lado == 0)
        {
            LunaTelon(m, p);
        }

        EdificiosTelon(m, p, u0, u1, lado, 0, nivel);
        EdificiosTelon(m, p, u0, u1, lado, 1, nivel);
    }

    ActualizarMallaGpu(mallaTelonReloj, m);
    nivelTelonReloj = (int)nivel;
}


//==================================================
// SALA DE RELOJERIA (malla horneada por calidad)
//==================================================
//
// Piso de tablones, muros de sillares con zocalo de madera, ventanales de
// medio punto, pilastras, cornisa y vigas (el techo se insinua con los
// canecillos y el arranque de las cerchas), estanterias llenas, el tablero
// de herramientas del taller, relojes de pared y apliques con velas.

static const float VENTANA_RADIO = 2.5f;        // medio ancho del ventanal
static const float ALTO_HILADA = 1.1f;
static const float LARGO_SILLAR = 2.2f;

static const float VENTANAS_FONDO[] = { -23.75f, -10.75f, 10.75f, 23.75f };
static const float PILASTRAS_FONDO[] = { -30.5f, -17.0f, -4.5f, 4.5f, 17.0f, 30.5f };
static const float VENTANAS_LADO[] = { -9.0f, 15.0f };
static const float PILASTRAS_LADO[] = { -15.0f, -3.0f, 9.0f, 21.0f, 33.0f, 45.0f, 57.0f };

static const int CANTIDAD_VENTANAS_FONDO = 4;
static const int CANTIDAD_PILASTRAS_FONDO = 6;
static const int CANTIDAD_VENTANAS_LADO = 2;
static const int CANTIDAD_PILASTRAS_LADO = 7;

// Relojes de pared (las agujas y los pendulos se animan aparte).
struct RelojParedReloj
{
    MuroReloj muro;
    float u;
    float b;
    float radio;
    bool pendulo;
};

static const RelojParedReloj RELOJES_PARED[] =
{
    { MURO_FONDO, -17.0f, 10.6f, 1.25f, false },
    { MURO_FONDO, 17.0f, 10.6f, 1.25f, false },
    { MURO_OESTE, -18.6f, 12.4f, 1.15f, false },
    { MURO_OESTE, 3.0f, 11.8f, 1.5f, true },
    { MURO_OESTE, 27.0f, 12.8f, 1.3f, false },
    { MURO_ESTE, -18.6f, 12.4f, 1.15f, false },
    { MURO_ESTE, 27.0f, 12.6f, 1.4f, true }
};

// Estanterias contra los muros laterales.
struct EstanteReloj
{
    MuroReloj muro;
    float u;
    float ancho;
    float alto;
};

static const EstanteReloj ESTANTES_RELOJ[] =
{
    { MURO_OESTE, -18.6f, 5.0f, 10.0f },
    { MURO_OESTE, 27.0f, 9.4f, 10.4f },
    { MURO_OESTE, 39.0f, 9.4f, 10.4f },
    { MURO_ESTE, -18.6f, 5.0f, 10.0f },
    { MURO_ESTE, 27.0f, 9.4f, 10.4f },
    { MURO_ESTE, 39.0f, 9.4f, 10.4f }
};

// Apliques con vela en las pilastras visibles (b = altura de la llama).
static const float ALTURA_APLIQUE = 6.6f;

static MallaGpu mallaSalaReloj;
static int nivelSalaReloj = -1;


static void LimitesMuro(
    MuroReloj muro,
    float& uMin,
    float& uMax
)
{
    if (muro == MURO_FONDO)
    {
        uMin = -(MURO_X + GROSOR_MURO);
        uMax = MURO_X + GROSOR_MURO;
        return;
    }

    uMin = MURO_FONDO_Z;
    uMax = PISO_FRENTE_Z;
}


static int VentanasMuro(
    MuroReloj muro,
    const float*& ventanas
)
{
    if (muro == MURO_FONDO)
    {
        ventanas = VENTANAS_FONDO;
        return CANTIDAD_VENTANAS_FONDO;
    }

    ventanas = VENTANAS_LADO;
    return CANTIDAD_VENTANAS_LADO;
}


static int PilastrasMuro(
    MuroReloj muro,
    const float*& pilastras
)
{
    if (muro == MURO_FONDO)
    {
        pilastras = PILASTRAS_FONDO;
        return CANTIDAD_PILASTRAS_FONDO;
    }

    pilastras = PILASTRAS_LADO;
    return CANTIDAD_PILASTRAS_LADO;
}


// La pilastra tiene aplique si se ve desde la camara de partida.
static bool PilastraConAplique(
    MuroReloj muro,
    float u
)
{
    return muro == MURO_FONDO || u < 30.0f;
}


static void PisoSalaReloj(
    ConstructorMalla& m,
    NivelCalidadGrafica nivel
)
{
    const float x0 = -MURO_X;
    const float x1 = MURO_X;
    const float z0 = MURO_FONDO_Z;
    const float z1 = PISO_FRENTE_Z;

    // Fondo oscuro: se ve por las juntas de los tablones.
    CuadroDobleCara(
        m,
        { x0, SUELO_SALA_Y - 0.02f, z0 },
        { x1, SUELO_SALA_Y - 0.02f, z0 },
        { x1, SUELO_SALA_Y - 0.02f, z1 },
        { x0, SUELO_SALA_Y - 0.02f, z1 },
        MADERA_OSCURA
    );

    const float ancho = 1.1f;
    const float junta = nivel == CALIDAD_BAJA ? 0.0f : 0.05f;
    int columnas = (int)((x1 - x0) / ancho);

    for (int c = 0; c < columnas; c++)
    {
        float xa = x0 + c * ancho + junta;
        float xb = x0 + (c + 1) * ancho - junta;
        float z = z0 + (nivel == CALIDAD_BAJA ? 0.0f : -3.0f * Hash2(c, 11));

        for (int t = 0; z < z1; t++)
        {
            float largo =
                nivel == CALIDAD_BAJA
                ? (z1 - z0)
                : 3.2f + 1.6f * Hash2(c * 31 + t, 12);

            float za = z < z0 ? z0 : z;
            float zb = z + largo - junta;

            if (zb > z1)
            {
                zb = z1;
            }

            z += largo;

            // Tablon tapado por el escenario o la plataforma de un lobulo.
            if (
                AlturaSueloReloj(xa, za) > SUELO_SALA_Y &&
                AlturaSueloReloj(xb, za) > SUELO_SALA_Y &&
                AlturaSueloReloj(xa, zb) > SUELO_SALA_Y &&
                AlturaSueloReloj(xb, zb) > SUELO_SALA_Y
            )
            {
                continue;
            }

            float tono = 0.82f + 0.24f * Hash2(c * 7 + t, 13);
            Color color = Oscurecer(Mezcla(MADERA, MADERA_CLARA, Hash2(c, t + 14)), tono);

            CuadroDobleCara(
                m,
                { xa, SUELO_SALA_Y, za },
                { xb, SUELO_SALA_Y, za },
                { xb, SUELO_SALA_Y, zb },
                { xa, SUELO_SALA_Y, zb },
                color
            );
        }
    }
}


// Sillares de un rectangulo de muro: hiladas de 1.1 con juntas oscuras
// (el fondo oscuro del muro asoma entre bloques).
static void SillaresPared(
    ConstructorMalla& m,
    const MarcoPared& p,
    int muro,
    float u0,
    float u1,
    float b0,
    float b1,
    NivelCalidadGrafica nivel
)
{
    if (u1 <= u0 || b1 <= b0)
    {
        return;
    }

    const bool juntas = nivel != CALIDAD_BAJA;
    const float junta = 0.06f;

    int f0 = (int)std::floor(b0 / ALTO_HILADA);
    int f1 = (int)std::ceil(b1 / ALTO_HILADA);

    for (int f = f0; f < f1; f++)
    {
        float fb0 = f * ALTO_HILADA;
        float fb1 = (f + 1) * ALTO_HILADA - (juntas ? junta : 0.0f);

        if (fb0 < b0) fb0 = b0;
        if (fb1 > b1) fb1 = b1;

        if (fb1 <= fb0)
        {
            continue;
        }

        Color tono = Oscurecer(PIEDRA, 0.9f + 0.16f * Hash2(f, muro * 7 + 1));

        if (!juntas)
        {
            PlacaPared(m, p, u0, fb0, u1, fb1, 0.0f, tono);
            continue;
        }

        float desfase = (f % 2 != 0) ? LARGO_SILLAR * 0.5f : 0.0f;
        int s0 = (int)std::floor((u0 - desfase) / LARGO_SILLAR);
        int s1 = (int)std::ceil((u1 - desfase) / LARGO_SILLAR);

        for (int s = s0; s < s1; s++)
        {
            float su0 = desfase + s * LARGO_SILLAR;
            float su1 = desfase + (s + 1) * LARGO_SILLAR - junta;

            if (su0 < u0) su0 = u0;
            if (su1 > u1) su1 = u1;

            if (su1 <= su0)
            {
                continue;
            }

            Color c =
                nivel == CALIDAD_ALTA
                ? Oscurecer(tono, 0.93f + 0.12f * Hash2(s + 1000 * muro, f + 3))
                : tono;

            PlacaPared(m, p, su0, fb0, su1, fb1, 0.0f, c);
        }
    }
}


// Cara interior del muro con los huecos de los ventanales, sus jambas,
// alfeizar, arco moldurado, clave y la carpinteria de hierro.
static void MuroConVentanas(
    ConstructorMalla& m,
    MuroReloj muro,
    NivelCalidadGrafica nivel
)
{
    MarcoPared p = MarcoMuro(muro);

    float uMin = 0.0f;
    float uMax = 0.0f;
    LimitesMuro(muro, uMin, uMax);

    const float* ventanas = nullptr;
    int cantidad = VentanasMuro(muro, ventanas);

    const float R = VENTANA_RADIO;
    const float H = ALTO_MURO;
    const int lados = nivel == CALIDAD_BAJA ? 8 : 14;

    const Color jamba = Oscurecer(PIEDRA_CLARA, 0.78f);
    const Color moldura = PIEDRA_CLARA;
    const Color hierro = Color{ 38, 36, 44, 255 };

    float inicio = uMin;

    for (int v = 0; v < cantidad; v++)
    {
        float wc = ventanas[v];

        // Fondo oscuro (juntas) y sillares alrededor del hueco.
        if (nivel != CALIDAD_BAJA)
        {
            PlacaPared(m, p, inicio, 0.0f, wc - R, H, -0.03f, PIEDRA_JUNTA);
            PlacaPared(m, p, wc - R, 0.0f, wc + R, VENTANA_B0, -0.03f, PIEDRA_JUNTA);
            PlacaPared(m, p, wc - R, VENTANA_B1 + R, wc + R, H, -0.03f, PIEDRA_JUNTA);
        }

        SillaresPared(m, p, (int)muro, inicio, wc - R, 0.0f, H, nivel);
        SillaresPared(m, p, (int)muro, wc - R, wc + R, 0.0f, VENTANA_B0, nivel);
        SillaresPared(m, p, (int)muro, wc - R, wc + R, VENTANA_B1 + R, H, nivel);

        // Timpanos entre el arco y la hilada de encima.
        for (int i = 0; i < lados; i++)
        {
            float a0 = PI_RELOJ * i / lados;
            float a1 = PI_RELOJ * (i + 1) / lados;

            CuadroDobleCara(
                m,
                PuntoPared(p, wc + R * std::cos(a0), VENTANA_B1 + R * std::sin(a0), 0.0f),
                PuntoPared(p, wc + R * std::cos(a1), VENTANA_B1 + R * std::sin(a1), 0.0f),
                PuntoPared(p, wc + R * std::cos(a1), VENTANA_B1 + R, 0.0f),
                PuntoPared(p, wc + R * std::cos(a0), VENTANA_B1 + R, 0.0f),
                PIEDRA
            );

            // Intrados del arco (el grosor del muro).
            CuadroDobleCara(
                m,
                PuntoPared(p, wc + R * std::cos(a0), VENTANA_B1 + R * std::sin(a0), 0.0f),
                PuntoPared(p, wc + R * std::cos(a1), VENTANA_B1 + R * std::sin(a1), 0.0f),
                PuntoPared(p, wc + R * std::cos(a1), VENTANA_B1 + R * std::sin(a1), -GROSOR_MURO),
                PuntoPared(p, wc + R * std::cos(a0), VENTANA_B1 + R * std::sin(a0), -GROSOR_MURO),
                jamba
            );
        }

        // Jambas y alfeizar del hueco.
        for (int lado = -1; lado <= 1; lado += 2)
        {
            float u = wc + lado * R;

            CuadroDobleCara(
                m,
                PuntoPared(p, u, VENTANA_B0, 0.0f),
                PuntoPared(p, u, VENTANA_B1, 0.0f),
                PuntoPared(p, u, VENTANA_B1, -GROSOR_MURO),
                PuntoPared(p, u, VENTANA_B0, -GROSOR_MURO),
                jamba
            );
        }

        CuadroDobleCara(
            m,
            PuntoPared(p, wc - R, VENTANA_B0, 0.0f),
            PuntoPared(p, wc + R, VENTANA_B0, 0.0f),
            PuntoPared(p, wc + R, VENTANA_B0, -GROSOR_MURO),
            PuntoPared(p, wc - R, VENTANA_B0, -GROSOR_MURO),
            jamba
        );

        // Moldura del arco con dovelas, jambas molduradas y clave.
        int dovelas = nivel == CALIDAD_BAJA ? 5 : 9;

        for (int i = 0; i < dovelas; i++)
        {
            float a0 = PI_RELOJ * i / dovelas;
            float a1 = PI_RELOJ * (i + 1) / dovelas - 0.02f;

            ArcoPared(
                m, p, wc, VENTANA_B1, 0.05f, R, R + 0.55f, a0, a1, 2,
                i % 2 == 0 ? moldura : Oscurecer(moldura, 0.9f)
            );
        }

        PlacaPared(m, p, wc - R - 0.55f, VENTANA_B0, wc - R, VENTANA_B1, 0.05f, moldura);
        PlacaPared(m, p, wc + R, VENTANA_B0, wc + R + 0.55f, VENTANA_B1, 0.05f, moldura);
        CajaPared(m, p, wc, VENTANA_B1 + R + 0.3f, 0.12f, 0.7f, 0.9f, 0.24f, Oscurecer(moldura, 1.06f));
        CajaPared(m, p, wc, VENTANA_B0 - 0.12f, 0.22f, 2.0f * R + 0.8f, 0.24f, 0.5f, moldura);

        // Carpinteria de hierro: marco, parteluz, travesanos y roseton.
        const float dv = -0.55f;

        for (int lado = -1; lado <= 1; lado += 2)
        {
            CajaPared(m, p, wc + lado * (R - 0.09f), (VENTANA_B0 + VENTANA_B1) * 0.5f, dv, 0.18f, VENTANA_B1 - VENTANA_B0, 0.14f, hierro);
        }

        ArcoPared(m, p, wc, VENTANA_B1, dv, R - 0.18f, R, 0.0f, PI_RELOJ, lados, hierro);
        CajaPared(m, p, wc, (VENTANA_B0 + VENTANA_B1 + R) * 0.5f, dv, 0.12f, VENTANA_B1 + R - VENTANA_B0, 0.12f, hierro);
        CajaPared(m, p, wc, VENTANA_B1, dv, 2.0f * R, 0.12f, 0.12f, hierro);
        CajaPared(m, p, wc, (VENTANA_B0 + VENTANA_B1) * 0.5f, dv, 2.0f * R, 0.1f, 0.1f, hierro);

        if (nivel != CALIDAD_BAJA)
        {
            ArcoPared(m, p, wc, VENTANA_B1 + R * 0.5f, dv, R * 0.28f, R * 0.36f, 0.0f, 2.0f * PI_RELOJ, lados, hierro);

            for (int lado = -1; lado <= 1; lado += 2)
            {
                CajaPared(m, p, wc + lado * R * 0.5f, (VENTANA_B0 + VENTANA_B1) * 0.5f, dv, 0.06f, VENTANA_B1 - VENTANA_B0, 0.06f, hierro);
            }

            for (float b = VENTANA_B0 + 0.9f; b < VENTANA_B1 - 0.2f; b += 0.9f)
            {
                CajaPared(m, p, wc, b, dv, 2.0f * R, 0.05f, 0.06f, hierro);
            }
        }

        inicio = wc + R;
    }

    if (nivel != CALIDAD_BAJA)
    {
        PlacaPared(m, p, inicio, 0.0f, uMax, H, -0.03f, PIEDRA_JUNTA);
    }

    SillaresPared(m, p, (int)muro, inicio, uMax, 0.0f, H, nivel);

    // Coronacion del muro (se ve por encima desde la vista general).
    CuadroDobleCara(
        m,
        PuntoPared(p, uMin, H, 0.0f),
        PuntoPared(p, uMax, H, 0.0f),
        PuntoPared(p, uMax, H, -GROSOR_MURO),
        PuntoPared(p, uMin, H, -GROSOR_MURO),
        PIEDRA_JUNTA
    );
}


// Zocalo de madera, pilastras con apliques, cornisa, viga y canecillos.
static void ArquitecturaMuro(
    ConstructorMalla& m,
    MuroReloj muro,
    NivelCalidadGrafica nivel
)
{
    MarcoPared p = MarcoMuro(muro);

    float uMin = 0.0f;
    float uMax = 0.0f;
    LimitesMuro(muro, uMin, uMax);

    float largo = uMax - uMin;
    float centro = (uMin + uMax) * 0.5f;
    const float H = ALTO_MURO;

    // Zocalo de madera con cuarterones.
    PlacaPared(m, p, uMin, 0.0f, uMax, 2.7f, 0.06f, MADERA_OSCURA);
    CajaPared(m, p, centro, 2.78f, 0.12f, largo, 0.18f, 0.24f, MADERA_CLARA);
    CajaPared(m, p, centro, 0.12f, 0.1f, largo, 0.24f, 0.2f, MADERA_OSCURA);

    if (nivel != CALIDAD_BAJA)
    {
        for (float u = uMin + 1.0f; u < uMax - 0.8f; u += 1.8f)
        {
            CajaPared(m, p, u, 1.45f, 0.1f, 1.5f, 2.0f, 0.08f, MADERA);
        }
    }

    // Pilastras con basa, capitel y aplique.
    const float* pilastras = nullptr;
    int cantidad = PilastrasMuro(muro, pilastras);

    for (int i = 0; i < cantidad; i++)
    {
        float u = pilastras[i];

        CajaPared(m, p, u, H * 0.5f, 0.3f, 1.3f, H, 0.6f, PIEDRA_CLARA);
        CajaPared(m, p, u, 0.6f, 0.45f, 1.8f, 1.2f, 0.9f, Oscurecer(PIEDRA_CLARA, 0.85f));
        CajaPared(m, p, u, H - 1.5f, 0.45f, 1.8f, 0.5f, 0.9f, Oscurecer(PIEDRA_CLARA, 1.08f));

        if (nivel != CALIDAD_BAJA)
        {
            CajaPared(m, p, u, 1.3f, 0.62f, 1.6f, 0.16f, 0.4f, PIEDRA_CLARA);
            CajaPared(m, p, u, H - 1.85f, 0.62f, 1.5f, 0.16f, 0.5f, PIEDRA_CLARA);
        }

        if (!PilastraConAplique(muro, u))
        {
            continue;
        }

        // Aplique: brazo, platillo, vela y llama (la llama no se oscurece).
        const float b = ALTURA_APLIQUE;

        CajaPared(m, p, u, b - 1.0f, 0.75f, 0.12f, 0.12f, 0.6f, BRONCE_OSCURO);
        CajaPared(m, p, u, b - 1.3f, 0.66f, 0.3f, 0.5f, 0.12f, BRONCE_OSCURO);
        m.Cilindro(PuntoPared(p, u, b - 0.95f, 1.05f), PuntoPared(p, u, b - 0.85f, 1.05f), 0.32f, 0.36f, 10, LATON);
        m.Cilindro(PuntoPared(p, u, b - 0.85f, 1.05f), PuntoPared(p, u, b - 0.25f, 1.05f), 0.1f, 0.1f, 8, MARFIL);
        m.Esfera(PuntoPared(p, u, b - 0.08f, 1.05f), 0.11f, 4, 6, Emisivo(Color{ 255, 214, 120, 255 }));
    }

    // Cornisa, viga de madera y canecillos (arranque del techo).
    CajaPared(m, p, centro, H - 0.45f, 0.45f, largo, 0.5f, 0.9f, PIEDRA_CLARA);
    CajaPared(m, p, centro, H - 0.1f, 0.7f, largo, 0.2f, 1.4f, Oscurecer(PIEDRA_CLARA, 0.9f));
    CajaPared(m, p, centro, H + 0.35f, 0.6f, largo, 0.7f, 1.8f, MADERA_OSCURA);

    float pasoCanecillo = nivel == CALIDAD_BAJA ? 5.4f : 2.7f;

    for (float u = uMin + 1.0f; u < uMax; u += pasoCanecillo)
    {
        CajaPared(m, p, u, H + 0.15f, 2.4f, 0.45f, 0.5f, 1.6f, MADERA);
    }

    // Cerchas que se pierden hacia arriba (solo se ven desde lejos).
    if (nivel != CALIDAD_BAJA)
    {
        for (int i = 0; i < cantidad; i++)
        {
            float u = pilastras[i];

            Viga(m, PuntoPared(p, u, H + 0.6f, 0.9f), PuntoPared(p, u, H + 4.6f, 7.5f), 0.28f, MADERA_OSCURA);
            Viga(m, PuntoPared(p, u, H - 2.2f, 0.65f), PuntoPared(p, u, H + 0.1f, 2.6f), 0.18f, MADERA_OSCURA);
        }
    }
}


// Reloj de pared (la caja y la esfera; las agujas se animan aparte).
static void RelojPared(
    ConstructorMalla& m,
    const RelojParedReloj& r,
    NivelCalidadGrafica nivel
)
{
    MarcoPared p = MarcoMuro(r.muro);
    int lados = nivel == CALIDAD_BAJA ? 14 : 24;

    if (r.pendulo)
    {
        // Regulador: caja alta con ventanilla para el pendulo.
        CajaPared(m, p, r.u, r.b - 2.4f, 0.3f, r.radio * 1.7f, 4.6f, 0.5f, MADERA_ROJIZA);
        CajaPared(m, p, r.u, r.b - 3.0f, 0.56f, r.radio * 1.2f, 2.8f, 0.04f, MADERA_OSCURA);
        CajaPared(m, p, r.u, r.b - 4.85f, 0.4f, r.radio * 1.9f, 0.3f, 0.6f, MADERA_OSCURA);

        m.Esfera(PuntoPared(p, r.u, r.b - 5.15f, 0.4f), 0.16f, 4, 6, LATON);
    }

    DiscoPared(m, p, r.u, r.b, 0.0f, 0.42f, r.radio + 0.22f, lados, MADERA_OSCURA);
    DiscoPared(m, p, r.u, r.b, 0.42f, 0.5f, r.radio + 0.1f, lados, LATON);
    DiscoPared(m, p, r.u, r.b, 0.5f, 0.54f, r.radio, lados, MARFIL);

    for (int h = 0; h < 12; h++)
    {
        float a = 2.0f * PI_RELOJ * h / 12.0f;
        float largo = h % 3 == 0 ? 0.26f : 0.14f;
        float ru = std::sin(a);
        float rb = std::cos(a);

        Vector3 a0 = PuntoPared(p, r.u + ru * (r.radio - 0.08f), r.b + rb * (r.radio - 0.08f), 0.56f);
        Vector3 a1 = PuntoPared(p, r.u + ru * (r.radio - 0.08f - largo), r.b + rb * (r.radio - 0.08f - largo), 0.56f);

        m.Cilindro(a0, a1, 0.035f, 0.035f, 4, TINTA);
    }

    DiscoPared(m, p, r.u, r.b, 0.54f, 0.6f, 0.08f, 8, BRONCE_OSCURO);

    // Remate de bronce arriba.
    m.Esfera(PuntoPared(p, r.u, r.b + r.radio + 0.3f, 0.3f), 0.18f, 4, 6, LATON);
}


// Objetos de una tabla de la estanteria: libros, frascos, relojitos,
// engranajes y cajitas. Devuelve cuantos puso.
static int ObjetosTabla(
    ConstructorMalla& m,
    const MarcoPared& p,
    float u0,
    float u1,
    float b,
    int semilla,
    NivelCalidadGrafica nivel
)
{
    const Color lomos[] =
    {
        { 128, 40, 36, 255 }, { 46, 74, 112, 255 }, { 54, 96, 60, 255 },
        { 150, 112, 52, 255 }, { 90, 50, 86, 255 }, { 132, 104, 80, 255 }
    };

    int puestos = 0;
    float u = u0 + 0.1f;

    for (int k = 0; u < u1 - 0.2f; k++)
    {
        float h = Hash2(semilla, k);
        int tipo = (int)(h * 7.0f);
        bool menudo = tipo >= 3;

        if (nivel == CALIDAD_BAJA && menudo)
        {
            u += 0.5f;
            continue;
        }

        if (nivel == CALIDAD_MEDIA && menudo && Hash2(k, semilla + 1) > 0.6f)
        {
            u += 0.5f;
            continue;
        }

        switch (tipo)
        {
            case 0:
            case 1:
            case 2:
            {
                // Fila de libros.
                int libros = 3 + (int)(Hash2(k, semilla + 2) * 6.0f);

                for (int l = 0; l < libros && u < u1 - 0.2f; l++)
                {
                    float grosor = 0.13f + 0.12f * Hash2(l + k * 9, semilla + 3);
                    float alto = 0.75f + 0.45f * Hash2(l + k * 9, semilla + 4);
                    Color c = Oscurecer(lomos[(l + k + semilla) % 6], 0.8f + 0.3f * Hash2(l, k));

                    CajaPared(m, p, u + grosor * 0.5f, b + alto * 0.5f, 0.55f, grosor - 0.015f, alto, 0.62f, c);
                    u += grosor;
                    puestos++;
                }

                u += 0.15f;
                break;
            }

            case 3:
                // Frasco de vidrio con tapa.
                m.Cilindro(PuntoPared(p, u + 0.2f, b, 0.55f), PuntoPared(p, u + 0.2f, b + 0.55f, 0.55f), 0.18f, 0.18f, 8, Color{ 92, 128, 120, 255 });
                m.Cilindro(PuntoPared(p, u + 0.2f, b + 0.55f, 0.55f), PuntoPared(p, u + 0.2f, b + 0.65f, 0.55f), 0.14f, 0.14f, 8, LATON);
                u += 0.5f;
                puestos++;
                break;

            case 4:
                // Relojito de mesa.
                CajaPared(m, p, u + 0.3f, b + 0.35f, 0.5f, 0.55f, 0.7f, 0.35f, MADERA_ROJIZA);
                DiscoPared(m, p, u + 0.3f, b + 0.42f, 0.67f, 0.7f, 0.2f, 10, MARFIL);
                u += 0.7f;
                puestos++;
                break;

            case 5:
                // Engranaje parado.
                DiscoPared(m, p, u + 0.36f, b + 0.36f, 0.45f, 0.55f, 0.34f, 12, LATON);
                DiscoPared(m, p, u + 0.36f, b + 0.36f, 0.55f, 0.58f, 0.1f, 6, BRONCE_OSCURO);
                u += 0.8f;
                puestos++;
                break;

            default:
                // Cajita de piezas.
                CajaPared(m, p, u + 0.35f, b + 0.2f, 0.55f, 0.65f, 0.4f, 0.55f, Oscurecer(MADERA_CLARA, 0.9f + 0.2f * h));
                u += 0.8f;
                puestos++;
                break;
        }
    }

    return puestos;
}


static int EstanteriaPared(
    ConstructorMalla& m,
    const EstanteReloj& e,
    int semilla,
    NivelCalidadGrafica nivel
)
{
    MarcoPared p = MarcoMuro(e.muro);

    float u0 = e.u - e.ancho * 0.5f;
    float u1 = e.u + e.ancho * 0.5f;
    const int tablas = 5;
    const float fondo = 1.1f;

    PlacaPared(m, p, u0, 0.0f, u1, e.alto, 0.08f, Oscurecer(MADERA_OSCURA, 0.8f));

    for (int lado = -1; lado <= 1; lado += 2)
    {
        CajaPared(m, p, e.u + lado * (e.ancho * 0.5f + 0.1f), e.alto * 0.5f, fondo * 0.5f, 0.2f, e.alto, fondo, MADERA);
    }

    CajaPared(m, p, e.u, e.alto + 0.15f, fondo * 0.5f + 0.05f, e.ancho + 0.7f, 0.3f, fondo + 0.15f, MADERA_OSCURA);

    int puestos = 0;

    for (int t = 0; t < tablas; t++)
    {
        float b = 0.35f + t * (e.alto - 0.4f) / tablas;

        CajaPared(m, p, e.u, b - 0.06f, fondo * 0.5f, e.ancho, 0.12f, fondo, MADERA_CLARA);

        // En calidad baja quedan cargadas solo tres tablas de cinco.
        if (nivel == CALIDAD_BAJA && t % 2 == 1)
        {
            continue;
        }

        puestos += ObjetosTabla(m, p, u0, u1, b, semilla * 13 + t, nivel);
    }

    return puestos;
}


// Tablero de herramientas sobre el banco largo del taller (muro oeste).
static int TableroHerramientas(
    ConstructorMalla& m,
    NivelCalidadGrafica nivel
)
{
    MarcoPared p = MarcoMuro(MURO_OESTE);

    const float u0 = -2.2f;
    const float u1 = 8.2f;
    const float b0 = 3.4f;
    const float b1 = 7.8f;

    PlacaPared(m, p, u0, b0, u1, b1, 0.1f, Color{ 150, 116, 78, 255 });
    CajaPared(m, p, (u0 + u1) * 0.5f, b1 + 0.1f, 0.14f, u1 - u0 + 0.3f, 0.2f, 0.12f, MADERA_OSCURA);
    CajaPared(m, p, (u0 + u1) * 0.5f, b0 - 0.1f, 0.14f, u1 - u0 + 0.3f, 0.2f, 0.12f, MADERA_OSCURA);

    int puestos = 0;
    int k = 0;

    for (float u = u0 + 0.5f; u < u1 - 0.4f; u += 0.62f, k++)
    {
        if (nivel == CALIDAD_BAJA && k % 2 == 1)
        {
            continue;
        }

        int fila = k % 2;
        float b = fila == 0 ? 6.2f : 4.4f;
        int tipo = (int)(Hash2(k, 91) * 4.0f);

        switch (tipo)
        {
            case 0:
                // Llave fija.
                CajaPared(m, p, u, b, 0.16f, 0.1f, 1.1f, 0.05f, ACERO);
                AnilloPared(m, p, u, b + 0.6f, 0.17f, 0.08f, 0.2f, 8, ACERO);
                break;

            case 1:
                // Martillo.
                CajaPared(m, p, u, b - 0.1f, 0.16f, 0.1f, 1.1f, 0.06f, MADERA_CLARA);
                CajaPared(m, p, u, b + 0.48f, 0.18f, 0.5f, 0.18f, 0.12f, ACERO);
                break;

            case 2:
                // Destornillador.
                CajaPared(m, p, u, b + 0.25f, 0.16f, 0.14f, 0.5f, 0.1f, Color{ 160, 52, 40, 255 });
                CajaPared(m, p, u, b - 0.3f, 0.16f, 0.04f, 0.6f, 0.04f, ACERO);
                break;

            default:
                // Compas o pinzas.
                Viga(m, PuntoPared(p, u - 0.2f, b - 0.5f, 0.16f), PuntoPared(p, u, b + 0.45f, 0.16f), 0.03f, LATON);
                Viga(m, PuntoPared(p, u + 0.2f, b - 0.5f, 0.16f), PuntoPared(p, u, b + 0.45f, 0.16f), 0.03f, LATON);
                break;
        }

        puestos++;
    }

    return puestos;
}


static void ConstruirMallaSalaReloj(NivelCalidadGrafica nivel)
{
    static ConstructorMalla m;
    m.Limpiar();
    m.reduccionDetalle = nivel == CALIDAD_ALTA ? 0 : 1;

    PisoSalaReloj(m, nivel);

    for (int muro = MURO_FONDO; muro < CANTIDAD_MUROS_RELOJ; muro++)
    {
        MuroConVentanas(m, (MuroReloj)muro, nivel);
        ArquitecturaMuro(m, (MuroReloj)muro, nivel);
    }

    for (const RelojParedReloj& r : RELOJES_PARED)
    {
        RelojPared(m, r, nivel);
    }

    int semilla = 1;

    for (const EstanteReloj& e : ESTANTES_RELOJ)
    {
        EstanteriaPared(m, e, semilla++, nivel);
    }

    TableroHerramientas(m, nivel);

    IluminarMalla(m, 0);

    ActualizarMallaGpu(mallaSalaReloj, m);
    nivelSalaReloj = (int)nivel;
}


//==================================================
// ESCENARIO, PLATAFORMAS Y CARA DEL RELOJ (malla horneada)
//==================================================
//
// Lo fijo de la esfera: escenario de madera con su surco, plataformas de
// los lobulos (parquet en el taller, losas en el campanario), la cara de
// marfil con minutero, numeros romanos, sol central y rubies, el bisel y
// las manecillas de acero pavonado bajo los senderos de las agujas.

static const float RADIO_PLATAFORMA_INTERIOR = RADIO_DIENTES_ARO + 0.15f;
static const float MANECILLA_Y = 0.06f;

static MallaGpu mallaEsferaReloj;
static int nivelEsferaReloj = -1;


// Radio (desde el centro del lobulo) donde la plataforma deja paso al aro.
static float RadioInteriorPlataforma(
    float cx,
    float angulo
)
{
    float c = std::cos(angulo);
    float r2 = RADIO_PLATAFORMA_INTERIOR * RADIO_PLATAFORMA_INTERIOR;
    float disc = cx * cx * c * c - cx * cx + r2;

    if (disc < 0.0f)
    {
        return 0.0f;
    }

    float r = -cx * c + std::sqrt(disc);
    return r < 0.0f ? 0.0f : r;
}


// Plataforma en media luna de un lobulo (lado -1 = taller, +1 = campanario).
static void PlataformaLobulo(
    ConstructorMalla& m,
    int lado,
    NivelCalidadGrafica nivel
)
{
    const float cx = lado * LOBULO_CENTRO_X;
    const float R = RADIO_PLATAFORMA;
    const int pasos = nivel == CALIDAD_BAJA ? 56 : 112;
    const int anillos = nivel == CALIDAD_BAJA ? 2 : 5;

    const Color claro = lado < 0 ? Color{ 150, 106, 64, 255 } : Color{ 128, 124, 134, 255 };
    const Color oscuro = lado < 0 ? Color{ 118, 80, 48, 255 } : Color{ 96, 92, 104, 255 };

    auto punto = [&](float t, float r, float y)
    {
        return Vector3{ cx + r * std::cos(t), y, r * std::sin(t) };
    };

    for (int i = 0; i < pasos; i++)
    {
        float t0 = 2.0f * PI_RELOJ * i / pasos;
        float t1 = 2.0f * PI_RELOJ * (i + 1) / pasos;

        float ra = RadioInteriorPlataforma(cx, t0);
        float rb = RadioInteriorPlataforma(cx, t1);

        if (ra >= R && rb >= R)
        {
            continue;
        }

        if (ra > R) ra = R;
        if (rb > R) rb = R;

        for (int k = 0; k < anillos; k++)
        {
            float fa = (float)k / anillos;
            float fb = (float)(k + 1) / anillos;

            float a0 = ra + (R - ra) * fa;
            float a1 = ra + (R - ra) * fb;
            float b0 = rb + (R - rb) * fa;
            float b1 = rb + (R - rb) * fb;

            bool par = ((i / 4) + k) % 2 == 0;
            Color c = Oscurecer(par ? claro : oscuro, 0.92f + 0.12f * Hash2(i / 4, k + lado * 10));

            CuadroDobleCara(m, punto(t0, a0, 0.0f), punto(t1, b0, 0.0f), punto(t1, b1, 0.0f), punto(t0, a1, 0.0f), c);
        }

        // Canto exterior con ribete de laton y canto interior junto al aro.
        CuadroDobleCara(m, punto(t0, R, SUELO_SALA_Y), punto(t1, R, SUELO_SALA_Y), punto(t1, R, 0.0f), punto(t0, R, 0.0f), MADERA_OSCURA);
        CuadroDobleCara(m, punto(t0, R - 0.3f, 0.02f), punto(t1, R - 0.3f, 0.02f), punto(t1, R, 0.02f), punto(t0, R, 0.02f), LATON);
        CuadroDobleCara(m, punto(t0, ra, ESCENARIO_Y), punto(t1, rb, ESCENARIO_Y), punto(t1, rb, 0.0f), punto(t0, ra, 0.0f), BRONCE_OSCURO);
    }
}


// Numero romano plano sobre la cara (I, V y X con barras arriba y abajo).
// La parte de arriba de cada letra mira hacia afuera.
static void NumeroRomano(
    ConstructorMalla& m,
    float angulo,
    float radio,
    const char* texto,
    Color color
)
{
    const float alto = 0.92f;
    const float trazo = 0.11f;
    const float y = 0.025f;

    float ancho = 0.0f;

    for (const char* c = texto; *c != '\0'; c++)
    {
        ancho += (*c == 'I') ? 0.26f : 0.66f;
    }

    Vector3 centro = PuntoEsfera(angulo, radio, 0.0f);
    float arribaX = std::sin(angulo);
    float arribaZ = -std::cos(angulo);
    float derechaX = std::cos(angulo);
    float derechaZ = std::sin(angulo);

    auto local = [&](float gx, float gy, float& x, float& z)
    {
        x = centro.x + derechaX * gx + arribaX * gy;
        z = centro.z + derechaZ * gx + arribaZ * gy;
    };

    auto trazar = [&](float ax, float ay, float bx, float by, float grosor)
    {
        float x0, z0, x1, z1;
        local(ax, ay, x0, z0);
        local(bx, by, x1, z1);
        TrazoPlano(m, x0, z0, x1, z1, grosor, y, color);
    };

    float gx = -ancho * 0.5f;
    const float h = alto * 0.5f;

    for (const char* c = texto; *c != '\0'; c++)
    {
        if (*c == 'I')
        {
            trazar(gx + 0.13f, -h, gx + 0.13f, h, trazo);
            gx += 0.26f;
        }
        else if (*c == 'V')
        {
            trazar(gx + 0.06f, h, gx + 0.33f, -h, trazo);
            trazar(gx + 0.60f, h, gx + 0.33f, -h, trazo * 0.7f);
            gx += 0.66f;
        }
        else
        {
            trazar(gx + 0.06f, h, gx + 0.60f, -h, trazo);
            trazar(gx + 0.06f, -h, gx + 0.60f, h, trazo * 0.7f);
            gx += 0.66f;
        }
    }

    // Barras de arriba y de abajo (como en los relojes de torre).
    trazar(-ancho * 0.5f - 0.05f, h + 0.04f, ancho * 0.5f + 0.05f, h + 0.04f, 0.06f);
    trazar(-ancho * 0.5f - 0.05f, -h - 0.04f, ancho * 0.5f + 0.05f, -h - 0.04f, 0.06f);
}


// Rombo plano (puntas de manecilla y marcas de hora).
static void RomboPlano(
    ConstructorMalla& m,
    float cx,
    float cz,
    float dx,
    float dz,
    float largoPunta,
    float largoCola,
    float semiancho,
    float y,
    Color color
)
{
    Vector3 punta = { cx + dx * largoPunta, y, cz + dz * largoPunta };
    Vector3 cola = { cx - dx * largoCola, y, cz - dz * largoCola };
    Vector3 izquierda = { cx - dz * semiancho, y, cz + dx * semiancho };
    Vector3 derecha = { cx + dz * semiancho, y, cz - dx * semiancho };

    m.TrianguloDobleCara(punta, izquierda, cola, color);
    m.TrianguloDobleCara(punta, cola, derecha, color);
}


static void CaraReloj(
    ConstructorMalla& m,
    NivelCalidadGrafica nivel
)
{
    const int lados = nivel == CALIDAD_BAJA ? 56 : 112;

    // Escenario: canto de madera con faja de laton y tapa en anillos (sin
    // capas superpuestas): surco del aro, marqueteria y borde.
    ParedCircularMalla(m, RADIO_ESCENARIO, SUELO_SALA_Y, ESCENARIO_Y, lados, MADERA_ROJIZA);
    ParedCircularMalla(m, RADIO_ESCENARIO + 0.02f, -0.22f, -0.15f, lados, LATON);
    AnilloPlanoMalla(m, RADIO_BISEL, RADIO_PLATAFORMA_INTERIOR, ESCENARIO_Y, lados, BRONCE_OSCURO);
    AnilloPlanoMalla(m, RADIO_ESCENARIO - 0.3f, RADIO_ESCENARIO, ESCENARIO_Y, lados, LATON);

    const int segmentos = nivel == CALIDAD_BAJA ? 24 : 64;

    for (int i = 0; i < segmentos; i++)
    {
        float a0 = 2.0f * PI_RELOJ * i / segmentos;
        float a1 = 2.0f * PI_RELOJ * (i + 1) / segmentos;
        Color c = i % 2 == 0 ? MADERA : MADERA_ROJIZA;

        CuadroDobleCara(
            m,
            PuntoEsfera(a0, RADIO_PLATAFORMA_INTERIOR, ESCENARIO_Y),
            PuntoEsfera(a1, RADIO_PLATAFORMA_INTERIOR, ESCENARIO_Y),
            PuntoEsfera(a1, RADIO_ESCENARIO - 0.3f, ESCENARIO_Y),
            PuntoEsfera(a0, RADIO_ESCENARIO - 0.3f, ESCENARIO_Y),
            c
        );
    }

    // Cara de marfil y bisel fijo.
    m.Cilindro({ 0.0f, ESCENARIO_Y - 0.1f, 0.0f }, { 0.0f, 0.0f, 0.0f }, RADIO_CARA, RADIO_CARA, lados, MARFIL);
    AnilloPlanoMalla(m, RADIO_CARA, RADIO_BISEL, 0.01f, lados, LATON);
    ParedCircularMalla(m, RADIO_BISEL, ESCENARIO_Y, 0.01f, lados, BRONCE);
    ParedCircularMalla(m, RADIO_CARA, 0.0f, 0.01f, lados, BRONCE);

    // Minutero: dos filetes y 60 marcas; las horas, rombos de laton.
    AnilloPlanoMalla(m, 14.92f, 14.98f, 0.02f, lados, TINTA);
    AnilloPlanoMalla(m, 15.28f, 15.34f, 0.02f, lados, TINTA);

    for (int i = 0; i < 60; i++)
    {
        float angulo = 2.0f * PI_RELOJ * i / 60.0f;

        if (i % 5 == 0)
        {
            Vector3 p = PuntoEsfera(angulo, 15.13f, 0.0f);
            RomboPlano(m, p.x, p.z, std::sin(angulo), -std::cos(angulo), 0.3f, 0.3f, 0.17f, 0.03f, ORO_RELOJ);
            continue;
        }

        if (nivel == CALIDAD_BAJA)
        {
            continue;
        }

        CajaRadialMalla(m, angulo, { 0.0f, 0.02f, -15.13f }, { 0.05f, 0.01f, 0.28f }, TINTA);
    }

    // Numeros romanos (las 12, 3, 6 y 9 son las agujas) y puntos de las
    // medias horas.
    const char* const numeros[12] =
    {
        nullptr, "I", "II", nullptr, "IIII", "V",
        nullptr, "VII", "VIII", nullptr, "X", "XI"
    };

    for (int h = 0; h < 12; h++)
    {
        float angulo = 2.0f * PI_RELOJ * h / 12.0f;

        if (numeros[h] != nullptr)
        {
            NumeroRomano(m, angulo, 10.62f, numeros[h], TINTA);
        }

        float media = angulo + PI_RELOJ / 12.0f;
        Vector3 p = PuntoEsfera(media, 10.62f, 0.0f);
        RomboPlano(m, p.x, p.z, std::sin(media), -std::cos(media), 0.16f, 0.16f, 0.1f, 0.025f, BRONCE);
    }

    AnilloPlanoMalla(m, 9.98f, 10.04f, 0.02f, lados, TINTA);

    // Sol guilloche alrededor del eje y rubies engastados.
    if (nivel != CALIDAD_BAJA)
    {
        for (int i = 0; i < 48; i++)
        {
            float angulo = 2.0f * PI_RELOJ * (i + 0.5f) / 48.0f;
            Vector3 a = PuntoEsfera(angulo, 2.3f, 0.0f);
            Vector3 b = PuntoEsfera(angulo, i % 2 == 0 ? 3.8f : 3.3f, 0.0f);

            TrazoPlano(m, a.x, a.z, b.x, b.z, 0.05f, 0.02f, MARFIL_PISTA);
        }
    }

    for (int k = 0; k < 4; k++)
    {
        Vector3 p = PuntoEsfera(PI_RELOJ * 0.25f + k * PI_RELOJ * 0.5f, 3.2f, 0.0f);

        m.Cilindro(p, { p.x, 0.035f, p.z }, 0.3f, 0.3f, 10, LATON);
        m.Cilindro({ p.x, 0.035f, p.z }, { p.x, 0.07f, p.z }, 0.2f, 0.14f, 8, Color{ 190, 30, 48, 255 });
    }

    // Manecillas de acero pavonado bajo los senderos de las agujas: la de
    // las horas apunta a las 12 y la de los minutos a las 3.
    const Color pavonado = Color{ 44, 58, 104, 255 };
    const Color pavonadoClaro = Color{ 70, 92, 150, 255 };
    const float re = RADIO_AGUJA_EXTERIOR;

    TrazoPlano(m, 0.0f, 1.2f, 0.0f, -re, 4.0f, MANECILLA_Y, pavonado);
    RomboPlano(m, 0.0f, -re, 0.0f, -1.0f, 3.0f, 2.4f, 2.6f, MANECILLA_Y + 0.008f, pavonadoClaro);

    TrazoPlano(m, -1.2f, 0.0f, re, 0.0f, 3.8f, MANECILLA_Y + 0.004f, pavonado);
    RomboPlano(m, re, 0.0f, 1.0f, 0.0f, 3.2f, 2.0f, 2.3f, MANECILLA_Y + 0.012f, pavonadoClaro);

    // Cubo del eje.
    AnilloPlanoCentro(m, 0.0f, 0.0f, 2.1f, 2.65f, MANECILLA_Y + 0.018f, 32, BRONCE_OSCURO);
    AnilloPlanoCentro(m, 0.0f, 0.0f, 2.65f, 2.8f, MANECILLA_Y + 0.018f, 32, LATON);

    // Plataformas de los lobulos.
    PlataformaLobulo(m, -1, nivel);
    PlataformaLobulo(m, 1, nivel);
}


static void ConstruirMallaEsferaReloj(NivelCalidadGrafica nivel)
{
    static ConstructorMalla m;
    m.Limpiar();

    CaraReloj(m, nivel);

    IluminarMalla(m, 0);

    ActualizarMallaGpu(mallaEsferaReloj, m);
    nivelEsferaReloj = (int)nivel;
}


//==================================================
// SENDEROS (malla horneada por calidad)
//==================================================
//
// Como en Arboleda: cinta por conexion con borde, dos carriles y detalle;
// disco en cada nodo y plaza en las bifurcaciones y uniones. Material por
// zona: placas de laton remachadas en la esfera, esmalte con filetes de oro
// en la corona, acero pavonado con linea dorada en las agujas, parquet en el
// taller y losas en el campanario. Las casillas fijas (las que no giran)
// llevan cuatro tornillos.

static const float SENDERO_Y = 0.06f;
static const float SENDERO_SEMIANCHO = 1.5f;
static const float SENDERO_BORDE = 0.30f;
static const float SENDERO_PLAZA = 1.85f;

static const float LIFT_BORDE = 0.030f;
static const float LIFT_BORDE_NODO = 0.050f;
static const float LIFT_TIERRA = 0.070f;
static const float LIFT_TIERRA_NODO = 0.090f;
static const float LIFT_DETALLE = 0.108f;

struct MaterialSenderoReloj
{
    Color base;
    Color borde;
    Color detalle;
    float contraste;
    float largoLosa;
};


static MaterialSenderoReloj MaterialSenderoZonaReloj(int zona)
{
    switch (zona)
    {
        case ZONA_CORONA:
            return { Color{ 214, 196, 160, 255 }, Color{ 126, 88, 46, 255 }, ORO_RELOJ, 0.04f, 1.0f };
        case ZONA_AGUJA_HORAS:
        case ZONA_AGUJA_MINUTOS:
            return { Color{ 72, 92, 148, 255 }, Color{ 30, 36, 64, 255 }, ORO_RELOJ, 0.05f, 1.6f };
        case ZONA_TALLER:
            return { Color{ 152, 106, 64, 255 }, MADERA_OSCURA, Color{ 70, 46, 30, 255 }, 0.12f, 0.55f };
        case ZONA_CAMPANARIO:
            return { Color{ 134, 128, 138, 255 }, Color{ 64, 62, 74, 255 }, PIEDRA_JUNTA, 0.12f, 1.0f };
        default:
            break;
    }

    return { Color{ 184, 140, 76, 255 }, BRONCE_OSCURO, BRONCE_OSCURO, 0.06f, 1.1f };
}


static MallaGpu mallaSenderosReloj;
static int nivelSenderosReloj = -1;


struct SeccionSendero
{
    float x, z;
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
    const SeccionSendero& s0,
    const SeccionSendero& s1,
    float lat0,
    float lat1,
    float lift,
    Color color
)
{
    float y = SENDERO_Y + lift;

    Vector3 a = { s0.x + s0.nx * lat0, y, s0.z + s0.nz * lat0 };
    Vector3 b = { s0.x + s0.nx * lat1, y, s0.z + s0.nz * lat1 };
    Vector3 c = { s1.x + s1.nx * lat0, y, s1.z + s1.nz * lat0 };
    Vector3 d = { s1.x + s1.nx * lat1, y, s1.z + s1.nz * lat1 };

    m.TrianguloDobleCara(a, b, c, color);
    m.TrianguloDobleCara(b, d, c, color);
}


static void DiscoSendero(
    ConstructorMalla& m,
    float x,
    float z,
    float radio,
    float lift,
    Color color
)
{
    const int LADOS_DISCO = 20;
    float y = SENDERO_Y + lift;

    for (int i = 0; i < LADOS_DISCO; i++)
    {
        float a0 = 2.0f * PI_RELOJ * i / LADOS_DISCO;
        float a1 = 2.0f * PI_RELOJ * (i + 1) / LADOS_DISCO;

        m.TrianguloDobleCara(
            Vector3{ x, y, z },
            Vector3{ x + std::cos(a0) * radio, y, z + std::sin(a0) * radio },
            Vector3{ x + std::cos(a1) * radio, y, z + std::sin(a1) * radio },
            color
        );
    }
}


static void AnilloSendero(
    ConstructorMalla& m,
    float x,
    float z,
    float radioInterior,
    float radioExterior,
    float lift,
    Color a,
    Color b,
    int losas
)
{
    float y = SENDERO_Y + lift;

    for (int i = 0; i < losas; i++)
    {
        float a0 = 2.0f * PI_RELOJ * i / losas;
        float a1 = 2.0f * PI_RELOJ * (i + 0.88f) / losas;
        Color c = (i % 2 == 0) ? a : b;

        Vector3 p00 = { x + std::cos(a0) * radioInterior, y, z + std::sin(a0) * radioInterior };
        Vector3 p01 = { x + std::cos(a0) * radioExterior, y, z + std::sin(a0) * radioExterior };
        Vector3 p10 = { x + std::cos(a1) * radioInterior, y, z + std::sin(a1) * radioInterior };
        Vector3 p11 = { x + std::cos(a1) * radioExterior, y, z + std::sin(a1) * radioExterior };

        m.TrianguloDobleCara(p00, p01, p10, c);
        m.TrianguloDobleCara(p01, p11, p10, c);
    }
}


static void Tornillo(
    ConstructorMalla& m,
    float x,
    float z,
    float giro
)
{
    float y = SENDERO_Y + LIFT_DETALLE;

    m.Cilindro({ x, y, z }, { x, y + 0.06f, z }, 0.14f, 0.12f, 6, ACERO);

    float dx = std::cos(giro) * 0.1f;
    float dz = std::sin(giro) * 0.1f;

    TrazoPlano(m, x - dx, z - dz, x + dx, z + dz, 0.035f, y + 0.065f, PIEDRA_JUNTA);
}


static void ConstruirMallaSenderosReloj(NivelCalidadGrafica nivel)
{
    const Tablero& t = TableroReferenciaReloj();
    const bool detalle = nivel != CALIDAD_BAJA;
    const float paso = 0.42f;
    const float limiteBorde = SENDERO_SEMIANCHO + SENDERO_BORDE;

    static ConstructorMalla m;
    m.Limpiar();
    m.reduccionDetalle = nivel == CALIDAD_ALTA ? 0 : 1;

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

            if (n > 120)
            {
                n = 120;
            }

            static SeccionSendero sec[128];

            for (int s = 0; s <= n; s++)
            {
                float u = (float)s / n;

                sec[s].x = a.posicion.x + dx * u;
                sec[s].z = a.posicion.z + dz * u;
                sec[s].nx = -dz / largo;
                sec[s].nz = dx / largo;
            }

            MaterialSenderoReloj ma = MaterialSenderoZonaReloj(a.zona);
            MaterialSenderoReloj mb = MaterialSenderoZonaReloj(b.zona);

            int bloqueAnterior = -1;

            for (int s = 0; s < n; s++)
            {
                float u = ((float)s + 0.5f) / n;
                int zona = u < 0.5f ? a.zona : b.zona;
                float tamLosa = ma.largoLosa + (mb.largoLosa - ma.largoLosa) * u;
                int bloque = (int)std::floor(u * largo / tamLosa);
                float contraste = ma.contraste + (mb.contraste - ma.contraste) * u;

                Color base = Mezcla(ma.base, mb.base, u);
                Color borde = Mezcla(ma.borde, mb.borde, u);
                Color detalleColor = u < 0.5f ? ma.detalle : mb.detalle;

                for (int lado = -1; lado <= 1; lado += 2)
                {
                    CuadroSendero(
                        m, sec[s], sec[s + 1],
                        lado * SENDERO_SEMIANCHO * 0.9f, lado * limiteBorde,
                        LIFT_BORDE, borde
                    );
                }

                for (int lado = 0; lado < 2; lado++)
                {
                    float v = Hash2(i * 131 + bloque * 7 + 1, j * 17 + lado);
                    Color c = Oscurecer(base, 1.0f + (v - 0.5f) * 2.0f * contraste);

                    CuadroSendero(
                        m, sec[s], sec[s + 1],
                        lado == 0 ? -SENDERO_SEMIANCHO : 0.0f,
                        lado == 0 ? 0.0f : SENDERO_SEMIANCHO,
                        LIFT_TIERRA, c
                    );
                }

                if (!detalle)
                {
                    bloqueAnterior = bloque;
                    continue;
                }

                bool junta = bloque != bloqueAnterior && bloqueAnterior >= 0;

                if (junta)
                {
                    // Junta entre placas, tablas o losas.
                    SeccionSendero j1 = sec[s];
                    j1.x += (sec[s + 1].x - sec[s].x) * 0.16f;
                    j1.z += (sec[s + 1].z - sec[s].z) * 0.16f;

                    CuadroSendero(
                        m, sec[s], j1,
                        -SENDERO_SEMIANCHO * 0.95f, SENDERO_SEMIANCHO * 0.95f,
                        LIFT_DETALLE, Oscurecer(borde, 0.9f)
                    );

                    // Remaches en las placas de la esfera.
                    if (zona == ZONA_ESFERA && nivel == CALIDAD_ALTA)
                    {
                        for (int lado = -1; lado <= 1; lado += 2)
                        {
                            float x = sec[s].x + sec[s].nx * 1.2f * lado;
                            float z = sec[s].z + sec[s].nz * 1.2f * lado;
                            float y = SENDERO_Y + LIFT_DETALLE;

                            m.Cilindro({ x, y, z }, { x, y + 0.04f, z }, 0.08f, 0.06f, 6, LATON);
                        }
                    }
                }
                else if (zona == ZONA_AGUJA_HORAS || zona == ZONA_AGUJA_MINUTOS)
                {
                    CuadroSendero(m, sec[s], sec[s + 1], -0.11f, 0.11f, LIFT_DETALLE, detalleColor);
                }
                else if (zona == ZONA_CORONA)
                {
                    for (int lado = -1; lado <= 1; lado += 2)
                    {
                        CuadroSendero(m, sec[s], sec[s + 1], lado * 1.22f - 0.05f, lado * 1.22f + 0.05f, LIFT_DETALLE, detalleColor);
                    }
                }
                else if (zona == ZONA_ESFERA)
                {
                    CuadroSendero(m, sec[s], sec[s + 1], -0.05f, 0.05f, LIFT_DETALLE, Oscurecer(base, 0.82f));
                }

                bloqueAnterior = bloque;
            }
        }
    }

    // --- Nodos: disco de union y plaza en bifurcaciones y uniones ---
    for (int i = 0; i < t.cantidadCasillas; i++)
    {
        const Casilla& c = t.casillas[i];
        MaterialSenderoReloj ms = MaterialSenderoZonaReloj(c.zona);

        bool plaza = grado[i] >= 3;
        float semi = plaza ? SENDERO_PLAZA : SENDERO_SEMIANCHO;

        DiscoSendero(m, c.posicion.x, c.posicion.z, semi + SENDERO_BORDE, LIFT_BORDE_NODO, ms.borde);
        DiscoSendero(m, c.posicion.x, c.posicion.z, semi, LIFT_TIERRA_NODO, ms.base);

        if (plaza)
        {
            Color claro = Oscurecer(ms.base, 1.1f);
            Color oscuro = Oscurecer(ms.base, 0.78f);

            AnilloSendero(
                m, c.posicion.x, c.posicion.z,
                SENDERO_SEMIANCHO - 0.25f, SENDERO_PLAZA - 0.05f,
                LIFT_DETALLE, claro, oscuro, 16
            );

            if (detalle)
            {
                AnilloSendero(
                    m, c.posicion.x, c.posicion.z,
                    SENDERO_PLAZA - 0.05f, SENDERO_PLAZA + 0.06f,
                    LIFT_DETALLE, ms.detalle, ms.detalle, 24
                );
            }
        }
    }

    // --- Tornillos de las casillas fijas de la esfera y la corona ---
    const NodosReloj& n = nodosReloj;
    int fijas[CASILLAS_ESFERA + CASILLAS_CORONA];
    int cantidadFijas = 0;

    for (int i = 0; i < CASILLAS_ESFERA; i++)
    {
        if (!CasillaGiratoriaReloj(n.esfera[i]))
        {
            fijas[cantidadFijas++] = n.esfera[i];
        }
    }

    for (int j = 0; j < CASILLAS_CORONA; j++)
    {
        if (!CasillaGiratoriaReloj(n.corona[j]))
        {
            fijas[cantidadFijas++] = n.corona[j];
        }
    }

    for (int f = 0; f < cantidadFijas; f++)
    {
        const Casilla* c = t.ObtenerCasilla(fijas[f]);

        if (c == nullptr)
        {
            continue;
        }

        for (int k = 0; k < 4; k++)
        {
            float a = PI_RELOJ * 0.25f + k * PI_RELOJ * 0.5f;

            Tornillo(
                m,
                c->posicion.x + std::cos(a) * 1.16f,
                c->posicion.z + std::sin(a) * 1.16f,
                a + 0.6f * f
            );
        }
    }

    IluminarMalla(m, 0);

    ActualizarMallaGpu(mallaSenderosReloj, m);
    nivelSenderosReloj = (int)nivel;
}


//==================================================
// PIEZAS QUE SE MUEVEN (mallas horneadas que se dibujan transformadas)
//==================================================
//
// El aro dentado y la placa de la corona giran con el giro deducido del
// tablero. El engranaje es una sola malla unitaria reutilizada (pinones del
// aro, maquina de la sala, muro y piso). El pendulo del reloj de pie y la
// campana gigante se mecen alrededor de su eje.

static MallaGpu mallaAroReloj;
static MallaGpu mallaCoronaReloj;
static MallaGpu mallaEngranajeReloj;
static MallaGpu mallaPenduloReloj;
static MallaGpu mallaCampanaReloj;
static int nivelPiezasMovilesReloj = -1;

// Reloj de pie (landmark del fondo) y campana gigante (campanario).
static const float RELOJ_PIE_ANCHO = 5.0f;
static const float RELOJ_PIE_FONDO = 2.4f;
static const float RELOJ_PIE_FRENTE_Z = MURO_FONDO_Z + RELOJ_PIE_FONDO;
static const float PENDULO_PIVOTE_Y = 10.2f;
static const float RELOJ_PIE_ESFERA_Y = 12.0f;
static const float CAMPANA_PIVOTE_Y = 12.4f;
static const float CAMPANA_ALTO = 9.4f;      // del eje al labio (con el yugo, mas de 10)
static const float CAMPANA_RADIO = 3.4f;


// Aro dentado alrededor de la esfera: 60 dientes, 17 muescas (una por paso
// del giro: hay 17 casillas giratorias) y la marca dorada, que queda en las
// 6 cuando el giro acumulado es 0. Todo queda por debajo de los senderos.
static void ConstruirAroReloj(
    ConstructorMalla& m,
    NivelCalidadGrafica nivel
)
{
    const int lados = nivel == CALIDAD_BAJA ? 56 : 112;

    AnilloPlanoMalla(m, RADIO_BISEL, RADIO_ARO, 0.03f, lados, BRONCE);
    ParedCircularMalla(m, RADIO_ARO, ESCENARIO_Y, 0.03f, lados, BRONCE_OSCURO);
    ParedCircularMalla(m, RADIO_BISEL, 0.01f, 0.03f, lados, BRONCE_OSCURO);

    const int dientes = 60;

    for (int i = 0; i < dientes; i++)
    {
        float angulo = 2.0f * PI_RELOJ * (i + 0.5f) / dientes;

        CajaRadialMalla(
            m, angulo,
            { 0.0f, -0.01f, -(RADIO_ARO + (RADIO_DIENTES_ARO - RADIO_ARO) * 0.5f) },
            { 0.44f, 0.08f, RADIO_DIENTES_ARO - RADIO_ARO + 0.04f },
            BRONCE
        );
    }

    const int muescas = 17;

    for (int i = 0; i < muescas; i++)
    {
        float angulo = PI_RELOJ + 2.0f * PI_RELOJ * i / muescas;
        Vector3 p = PuntoEsfera(angulo, (RADIO_BISEL + RADIO_ARO) * 0.5f, 0.03f);

        m.Cilindro(p, { p.x, 0.065f, p.z }, 0.11f, 0.08f, 6, LATON);
    }

    CajaRadialMalla(
        m, PI_RELOJ,
        { 0.0f, 0.045f, -(RADIO_BISEL + RADIO_ARO) * 0.5f },
        { 0.7f, 0.03f, 0.46f },
        ORO_RELOJ
    );

    Vector3 joya = PuntoEsfera(PI_RELOJ, (RADIO_BISEL + RADIO_ARO) * 0.5f, 0.06f);
    m.Cilindro(joya, { joya.x, 0.075f, joya.z }, 0.17f, 0.12f, 8, Color{ 210, 40, 52, 255 });
}


// Placa giratoria de la corona: puntas con perlas hacia afuera, dientes
// hacia adentro, filetes grabados y seis rubies (uno por paso).
static void ConstruirCoronaReloj(
    ConstructorMalla& m,
    NivelCalidadGrafica nivel
)
{
    const int lados = nivel == CALIDAD_BAJA ? 48 : 96;
    const Color placa = Color{ 196, 150, 74, 255 };

    AnilloPlanoMalla(m, CORONA_INTERIOR, CORONA_EXTERIOR, 0.02f, lados, placa);
    ParedCircularMalla(m, CORONA_EXTERIOR, 0.0f, 0.02f, lados, BRONCE);
    ParedCircularMalla(m, CORONA_INTERIOR, 0.0f, 0.02f, lados, BRONCE);

    AnilloPlanoMalla(m, 8.55f, 8.65f, 0.04f, lados, BRONCE_OSCURO);
    AnilloPlanoMalla(m, 4.55f, 4.65f, 0.04f, lados, BRONCE_OSCURO);

    // Puntas de la corona.
    for (int i = 0; i < 12; i++)
    {
        float a = 2.0f * PI_RELOJ * (i + 0.5f) / 12.0f;
        float da = PI_RELOJ / 24.0f;

        m.TrianguloDobleCara(
            PuntoEsfera(a - da, CORONA_EXTERIOR - 0.02f, 0.02f),
            PuntoEsfera(a + da, CORONA_EXTERIOR - 0.02f, 0.02f),
            PuntoEsfera(a, CORONA_EXTERIOR + 0.75f, 0.02f),
            ORO_RELOJ
        );

        Vector3 perla = PuntoEsfera(a, CORONA_EXTERIOR + 0.68f, 0.04f);
        m.Cilindro({ perla.x, 0.02f, perla.z }, perla, 0.12f, 0.12f, 6, MARFIL);
    }

    // Dientes interiores.
    for (int i = 0; i < 36; i++)
    {
        float a = 2.0f * PI_RELOJ * i / 36.0f;

        CajaRadialMalla(m, a, { 0.0f, 0.01f, -(CORONA_INTERIOR - 0.16f) }, { 0.26f, 0.02f, 0.34f }, BRONCE);
    }

    // Grabado radial.
    if (nivel != CALIDAD_BAJA)
    {
        for (int i = 0; i < 48; i++)
        {
            float a = 2.0f * PI_RELOJ * (i + 0.5f) / 48.0f;
            Vector3 p0 = PuntoEsfera(a, 8.7f, 0.0f);
            Vector3 p1 = PuntoEsfera(a, 9.2f, 0.0f);
            Vector3 q0 = PuntoEsfera(a, 3.95f, 0.0f);
            Vector3 q1 = PuntoEsfera(a, 4.5f, 0.0f);

            TrazoPlano(m, p0.x, p0.z, p1.x, p1.z, 0.05f, 0.04f, BRONCE_OSCURO);
            TrazoPlano(m, q0.x, q0.z, q1.x, q1.z, 0.05f, 0.04f, BRONCE_OSCURO);
        }
    }

    // Seis rubies (60 grados por paso) en la banda exterior y la interior.
    for (int i = 0; i < 6; i++)
    {
        float a = 2.0f * PI_RELOJ * (i + 0.25f) / 6.0f;

        for (int banda = 0; banda < 2; banda++)
        {
            Vector3 p = PuntoEsfera(a, banda == 0 ? 8.92f : 4.25f, 0.02f);

            m.Cilindro(p, { p.x, 0.045f, p.z }, 0.24f, 0.24f, 10, LATON);
            m.Cilindro({ p.x, 0.045f, p.z }, { p.x, 0.075f, p.z }, 0.16f, 0.1f, 8, Color{ 196, 30, 50, 255 });
        }
    }
}


// Engranaje unitario: radio 1, eje Y, grosor 0.3, con rayos calados.
static void ConstruirEngranajeUnitario(
    ConstructorMalla& m,
    NivelCalidadGrafica nivel
)
{
    const int lados = nivel == CALIDAD_BAJA ? 16 : 28;

    m.Cilindro({ 0.0f, -0.15f, 0.0f }, { 0.0f, 0.15f, 0.0f }, 0.86f, 0.86f, lados, BRONCE);
    m.Cilindro({ 0.0f, 0.15f, 0.0f }, { 0.0f, 0.165f, 0.0f }, 0.78f, 0.78f, lados, LATON);
    m.Cilindro({ 0.0f, -0.165f, 0.0f }, { 0.0f, -0.15f, 0.0f }, 0.78f, 0.78f, lados, LATON);
    m.Cilindro({ 0.0f, -0.3f, 0.0f }, { 0.0f, 0.3f, 0.0f }, 0.2f, 0.2f, 10, BRONCE_OSCURO);
    m.Cilindro({ 0.0f, 0.165f, 0.0f }, { 0.0f, 0.18f, 0.0f }, 0.3f, 0.3f, 10, LATON);
    m.Cilindro({ 0.0f, -0.18f, 0.0f }, { 0.0f, -0.165f, 0.0f }, 0.3f, 0.3f, 10, LATON);

    // Calados entre los rayos (oscuros, en las dos caras).
    for (int k = 0; k < 5; k++)
    {
        float a = k * 2.0f * PI_RELOJ / 5.0f;
        float x = std::sin(a) * 0.5f;
        float z = std::cos(a) * 0.5f;

        m.Cilindro({ x, 0.166f, z }, { x, 0.176f, z }, 0.2f, 0.2f, 8, MADERA_OSCURA);
        m.Cilindro({ x, -0.176f, z }, { x, -0.166f, z }, 0.2f, 0.2f, 8, MADERA_OSCURA);
    }

    const int dientes = 20;

    for (int i = 0; i < dientes; i++)
    {
        CajaRadialMalla(
            m,
            2.0f * PI_RELOJ * i / dientes,
            { 0.0f, 0.0f, -0.94f },
            { 0.17f, 0.3f, 0.2f },
            BRONCE
        );
    }
}


// Pendulo del reloj de pie: pivote en el origen, cuelga hacia -y.
static void ConstruirPenduloReloj(ConstructorMalla& m)
{
    m.Caja({ 0.0f, -3.0f, 0.0f }, { 0.12f, 6.0f, 0.06f }, LATON);
    m.Caja({ 0.0f, -0.15f, 0.0f }, { 0.5f, 0.3f, 0.12f }, BRONCE_OSCURO);
    m.Cilindro({ 0.0f, -6.2f, -0.08f }, { 0.0f, -6.2f, 0.08f }, 0.8f, 0.8f, 20, ORO_RELOJ);
    m.Cilindro({ 0.0f, -6.2f, 0.08f }, { 0.0f, -6.2f, 0.12f }, 0.55f, 0.55f, 20, LATON);
    m.Cilindro({ 0.0f, -6.2f, 0.12f }, { 0.0f, -6.2f, 0.15f }, 0.18f, 0.18f, 10, BRONCE_OSCURO);
}


// Campana gigante: el eje de vaiven pasa por el origen (a lo largo de z) y
// la campana cuelga hacia -y: yugo, asas, hombro, cintura, medio pie,
// labio y badajo.
static void ConstruirCampanaReloj(
    ConstructorMalla& m,
    NivelCalidadGrafica nivel
)
{
    const int lados = nivel == CALIDAD_BAJA ? 18 : 32;
    const float H = CAMPANA_ALTO;
    const float e = H / 8.4f;              // el perfil esta dibujado para 8.4
    const float R = CAMPANA_RADIO;
    const Color bronceCampana = Color{ 150, 108, 58, 255 };
    const Color bronceClaro = Color{ 186, 142, 80, 255 };

    // Yugo de madera con herrajes y eje.
    m.Caja({ 0.0f, 0.0f, 0.0f }, { 1.4f, 1.0f, 6.6f }, MADERA_OSCURA);
    m.Cilindro({ 0.0f, 0.0f, -4.2f }, { 0.0f, 0.0f, 4.2f }, 0.22f, 0.22f, 10, ACERO);

    for (int lado = -1; lado <= 1; lado += 2)
    {
        m.Caja({ 0.0f, 0.0f, lado * 2.6f }, { 1.5f, 1.1f, 0.2f }, ACERO);
    }

    // Asas (corona de la campana).
    for (int k = -1; k <= 1; k++)
    {
        m.Cilindro({ 0.0f, -0.4f, k * 0.6f }, { 0.0f, -1.0f, k * 0.6f }, 0.18f, 0.2f, 8, bronceCampana);
    }

    // Perfil: (altura bajo el pivote, radio).
    const float perfil[][2] =
    {
        { 0.9f, 0.0f },
        { 1.0f, 1.55f },
        { 1.5f, 1.85f },
        { 3.0f, 1.95f },
        { 4.6f, 2.2f },
        { 6.0f, 2.75f },
        { 7.0f, 3.2f },
        { 7.6f, R },
        { 8.0f, R + 0.1f },
        { 8.6f, R - 0.05f }
    };

    const int puntos = (int)(sizeof(perfil) / sizeof(perfil[0]));

    for (int k = 0; k + 1 < puntos; k++)
    {
        Color c = (k == 6 || k == 8) ? bronceClaro : bronceCampana;

        m.Cilindro(
            { 0.0f, -perfil[k][0] * e - 0.4f, 0.0f },
            { 0.0f, -perfil[k + 1][0] * e - 0.4f, 0.0f },
            perfil[k][1], perfil[k + 1][1],
            lados, c
        );
    }

    // Interior oscuro, filetes en relieve y badajo.
    float fondoY = -(8.6f * e + 0.4f);

    m.Cilindro({ 0.0f, fondoY + 0.02f, 0.0f }, { 0.0f, fondoY + 0.01f, 0.0f }, R - 0.12f, R - 0.12f, lados, Color{ 40, 28, 18, 255 });
    m.Cilindro({ 0.0f, -2.6f * e, 0.0f }, { 0.0f, -2.75f * e, 0.0f }, 2.02f, 2.04f, lados, bronceClaro);
    m.Cilindro({ 0.0f, -6.2f * e, 0.0f }, { 0.0f, -6.38f * e, 0.0f }, 2.92f, 2.98f, lados, bronceClaro);
    m.Cilindro({ 0.0f, -1.6f, 0.0f }, { 0.0f, fondoY + 0.3f, 0.0f }, 0.16f, 0.16f, 8, ACERO);
    m.Esfera({ 0.0f, fondoY + 0.1f, 0.0f }, 0.48f, 6, 8, ACERO);
}


static void PrepararPiezasMovilesReloj()
{
    NivelCalidadGrafica nivel = CalidadDecoracion();

    if (nivelPiezasMovilesReloj == (int)nivel && mallaAroReloj.cargada)
    {
        return;
    }

    static ConstructorMalla m;
    m.reduccionDetalle = nivel == CALIDAD_ALTA ? 0 : 1;

    m.Limpiar();
    ConstruirAroReloj(m, nivel);
    IluminarMalla(m, 0);
    ActualizarMallaGpu(mallaAroReloj, m);

    m.Limpiar();
    ConstruirCoronaReloj(m, nivel);
    IluminarMalla(m, 0);
    ActualizarMallaGpu(mallaCoronaReloj, m);

    m.Limpiar();
    ConstruirEngranajeUnitario(m, nivel);
    IluminarMalla(m, 0);
    ActualizarMallaGpu(mallaEngranajeReloj, m);

    m.Limpiar();
    ConstruirPenduloReloj(m);
    IluminarMalla(m, 0);
    ActualizarMallaGpu(mallaPenduloReloj, m);

    m.Limpiar();
    ConstruirCampanaReloj(m, nivel);
    IluminarMalla(m, 0);
    ActualizarMallaGpu(mallaCampanaReloj, m);

    nivelPiezasMovilesReloj = (int)nivel;
}


// Engranajes animados. eje: 0 = Y (acostado), 1 = Z (de frente),
// 2 = X (contra un muro lateral). Los que engranan giran al reves y a una
// velocidad inversa a su radio. Los pinones del aro solo se mueven con el
// giro de la esfera (acople = vueltas por cada grado del aro).
struct EngranajeReloj
{
    Vector3 centro;
    float radio;
    int eje;
    float velocidad;
    float acople;
    NivelCalidadGrafica nivelMinimo;
};

static const float VELOCIDAD_MAQUINA = 9.0f;
static const float ACOPLE_PINON = RADIO_DIENTES_ARO / RADIO_PINON;

static const EngranajeReloj ENGRANAJES_RELOJ[] =
{
    // Pinones acoplados al aro (las 11, la 1, las 5 y las 7).
    { { -8.83f, ESCENARIO_Y + 0.16f, -15.29f }, RADIO_PINON, 0, 0.0f, ACOPLE_PINON, CALIDAD_BAJA },
    { { 8.83f, ESCENARIO_Y + 0.16f, -15.29f }, RADIO_PINON, 0, 0.0f, ACOPLE_PINON, CALIDAD_BAJA },
    { { 8.83f, ESCENARIO_Y + 0.16f, 15.29f }, RADIO_PINON, 0, 0.0f, ACOPLE_PINON, CALIDAD_MEDIA },
    { { -8.83f, ESCENARIO_Y + 0.16f, 15.29f }, RADIO_PINON, 0, 0.0f, ACOPLE_PINON, CALIDAD_MEDIA },

    // Maquina de la sala de engranajes (este), de frente a la camara.
    { { ENGRANAJES_X, 4.0f, ENGRANAJES_Z }, 3.0f, 1, VELOCIDAD_MAQUINA, 0.0f, CALIDAD_BAJA },
    { { ENGRANAJES_X - 2.43f, 7.46f, ENGRANAJES_Z + 0.35f }, 1.6f, 1, -VELOCIDAD_MAQUINA * 3.0f / 1.6f, 0.0f, CALIDAD_BAJA },
    { { ENGRANAJES_X + 3.46f, 2.74f, ENGRANAJES_Z + 0.35f }, 1.0f, 1, -VELOCIDAD_MAQUINA * 3.0f, 0.0f, CALIDAD_MEDIA },

    // Engranajes del muro este, sobre la maquina.
    { { MURO_X - 0.45f, 11.6f, 4.0f }, 2.4f, 2, -5.0f, 0.0f, CALIDAD_MEDIA },
    { { MURO_X - 0.75f, 13.1f, 7.6f }, 1.35f, 2, 5.0f * 2.4f / 1.35f, 0.0f, CALIDAD_ALTA },

    // Engranajes acostados del rincon oeste.
    { { -25.4f, 0.55f, -16.6f }, 2.6f, 0, 7.0f, 0.0f, CALIDAD_BAJA },
    { { -21.5f, 0.55f, -15.6f }, 1.45f, 0, -7.0f * 2.6f / 1.45f, 0.0f, CALIDAD_MEDIA }
};


//==================================================
// LANDMARKS (malla horneada por calidad)
//==================================================
//
// Uno o mas por zona: el reloj de pie del fondo (esfera), el campanario con
// la campana gigante, el taller (mesa del relojero, banco largo con el
// tablero de herramientas, torno, vitrina, caja fuerte y mesita), la
// maquina de la sala de engranajes con su tambor de pesas, los soportes de
// los engranajes del piso, el atril del frente y un cartel por bifurcacion.

static MallaGpu mallaLandmarksReloj;
static int nivelLandmarksReloj = -1;


static void TransformarMalla(
    ConstructorMalla& m,
    float x,
    float y,
    float z,
    float giroGrados
)
{
    TransformacionMalla t;
    t.tx = x;
    t.ty = y;
    t.tz = z;
    t.giroY = giroGrados;

    m.EstablecerTransformacion(t);
}


// Patas de mueble en las cuatro esquinas (coordenadas locales).
static void PatasMueble(
    ConstructorMalla& m,
    float ancho,
    float fondo,
    float alto,
    float grosor,
    Color color
)
{
    for (int i = -1; i <= 1; i += 2)
    {
        for (int j = -1; j <= 1; j += 2)
        {
            m.Caja(
                { i * (ancho * 0.5f - grosor * 0.5f), alto * 0.5f, j * (fondo * 0.5f - grosor * 0.5f) },
                { grosor, alto, grosor },
                color
            );
        }
    }
}


// Reloj de pie gigante contra el muro del fondo (esfera, 16.6 de alto).
static void ConstruirRelojPie(
    ConstructorMalla& m,
    NivelCalidadGrafica nivel
)
{
    MarcoPared p = MarcoMuro(MURO_FONDO);

    const float F = RELOJ_PIE_FONDO;
    const float dm = F * 0.5f;
    const int lados = nivel == CALIDAD_BAJA ? 16 : 28;

    // Pedestal.
    CajaPared(m, p, 0.0f, 0.15f, dm, RELOJ_PIE_ANCHO + 0.4f, 0.3f, F + 0.2f, MADERA_OSCURA);
    CajaPared(m, p, 0.0f, 1.2f, dm, RELOJ_PIE_ANCHO, 1.9f, F, MADERA_OSCURA);
    CajaPared(m, p, 0.0f, 2.25f, dm, RELOJ_PIE_ANCHO + 0.3f, 0.2f, F + 0.2f, MADERA_ROJIZA);
    CajaPared(m, p, 0.0f, 1.2f, F + 0.01f, 3.4f, 1.3f, 0.04f, MADERA_ROJIZA);
    DiscoPared(m, p, 0.0f, 1.2f, F + 0.03f, F + 0.09f, 0.42f, 12, LATON);

    // Tronco: costados, fondo oscuro, travesanos y la puerta de vidrio.
    for (int lado = -1; lado <= 1; lado += 2)
    {
        CajaPared(m, p, lado * 1.85f, 6.2f, dm, 0.5f, 7.7f, 2.0f, MADERA_ROJIZA);
        CajaPared(m, p, lado * 1.62f, 6.2f, F - 0.12f, 0.16f, 7.0f, 0.12f, LATON);

        // Pesas colgando.
        m.Cilindro(PuntoPared(p, lado * 0.95f, 6.8f, 0.85f), PuntoPared(p, lado * 0.95f, 8.4f, 0.85f), 0.24f, 0.24f, 10, LATON);
        Viga(m, PuntoPared(p, lado * 0.95f, 8.4f, 0.85f), PuntoPared(p, lado * 0.95f, 9.9f, 0.85f), 0.03f, ACERO);
    }

    CajaPared(m, p, 0.0f, 6.2f, 0.12f, 3.2f, 7.7f, 0.2f, Color{ 40, 26, 20, 255 });
    CajaPared(m, p, 0.0f, 2.6f, dm, 4.2f, 0.5f, F - 0.2f, MADERA_ROJIZA);
    CajaPared(m, p, 0.0f, 9.75f, dm, 4.2f, 0.5f, F - 0.2f, MADERA_ROJIZA);
    CajaPared(m, p, 0.0f, 3.0f, F - 0.12f, 3.3f, 0.14f, 0.12f, LATON);
    CajaPared(m, p, 0.0f, 9.4f, F - 0.12f, 3.3f, 0.14f, 0.12f, LATON);

    // Capitel con columnitas, esfera, arco de la luna y frontis partido.
    CajaPared(m, p, 0.0f, 12.1f, dm, 4.8f, 4.2f, F, MADERA_ROJIZA);
    CajaPared(m, p, 0.0f, 10.1f, dm + 0.1f, 5.1f, 0.3f, F + 0.2f, MADERA_OSCURA);
    CajaPared(m, p, 0.0f, 14.35f, dm + 0.1f, 5.3f, 0.4f, F + 0.3f, MADERA_OSCURA);

    for (int lado = -1; lado <= 1; lado += 2)
    {
        m.Cilindro(PuntoPared(p, lado * 2.15f, 10.25f, F), PuntoPared(p, lado * 2.15f, 14.1f, F), 0.17f, 0.17f, 10, MADERA_OSCURA);
        m.Cilindro(PuntoPared(p, lado * 2.15f, 13.9f, F), PuntoPared(p, lado * 2.15f, 14.15f, F), 0.24f, 0.24f, 10, LATON);
        m.Cilindro(PuntoPared(p, lado * 2.15f, 10.25f, F), PuntoPared(p, lado * 2.15f, 10.5f, F), 0.24f, 0.24f, 10, LATON);
    }

    DiscoPared(m, p, 0.0f, RELOJ_PIE_ESFERA_Y, F - 0.02f, F + 0.06f, 1.98f, lados, LATON);
    DiscoPared(m, p, 0.0f, RELOJ_PIE_ESFERA_Y, F + 0.06f, F + 0.1f, 1.72f, lados, MARFIL);

    for (int h = 0; h < 12; h++)
    {
        float a = 2.0f * PI_RELOJ * h / 12.0f;
        float largo = h % 3 == 0 ? 0.36f : 0.2f;
        float ru = std::sin(a);
        float rb = std::cos(a);

        m.Cilindro(
            PuntoPared(p, ru * 1.55f, RELOJ_PIE_ESFERA_Y + rb * 1.55f, F + 0.11f),
            PuntoPared(p, ru * (1.55f - largo), RELOJ_PIE_ESFERA_Y + rb * (1.55f - largo), F + 0.11f),
            0.05f, 0.05f, 4, TINTA
        );
    }

    ArcoPared(m, p, 0.0f, 13.75f, F + 0.02f, 0.0f, 1.0f, 0.0f, PI_RELOJ, 12, ACERO_AZUL);
    AnilloPared(m, p, 0.35f, 14.15f, F + 0.04f, 0.0f, 0.24f, 10, ORO_RELOJ);

    for (int lado = -1; lado <= 1; lado += 2)
    {
        Viga(m, PuntoPared(p, lado * 2.6f, 14.6f, dm + 0.2f), PuntoPared(p, lado * 0.7f, 15.7f, dm + 0.2f), 0.28f, MADERA_OSCURA);
        m.Cilindro(PuntoPared(p, lado * 2.4f, 14.55f, dm), PuntoPared(p, lado * 2.4f, 14.95f, dm), 0.12f, 0.12f, 8, LATON);
        m.Esfera(PuntoPared(p, lado * 2.4f, 15.15f, dm), 0.24f, 6, 8, ORO_RELOJ);
    }

    CajaPared(m, p, 0.0f, 15.2f, dm, 0.9f, 1.0f, 0.9f, MADERA_ROJIZA);
    m.Cilindro(PuntoPared(p, 0.0f, 15.7f, dm), PuntoPared(p, 0.0f, 16.2f, dm), 0.16f, 0.16f, 8, LATON);
    m.Esfera(PuntoPared(p, 0.0f, 16.45f, dm), 0.32f, 6, 8, ORO_RELOJ);
}


// Campanario: zocalo de piedra, dos caballetes de madera, cabezal,
// tejado a cuatro aguas con veleta, cuerda y escalera. La campana gigante
// (malla aparte) cuelga del eje.
static void ConstruirCampanario(
    ConstructorMalla& m,
    NivelCalidadGrafica nivel
)
{
    const float cx = CAMPANARIO_X;
    const float cz = CAMPANARIO_Z;
    const float base = SUELO_SALA_Y + 0.8f;
    const Color tejas = Color{ 112, 52, 40, 255 };

    m.Caja({ cx, SUELO_SALA_Y + 0.4f, cz }, { 9.0f, 0.8f, 9.0f }, PIEDRA);
    m.Caja({ cx, base + 0.05f, cz }, { 9.3f, 0.1f, 9.3f }, PIEDRA_CLARA);

    if (nivel != CALIDAD_BAJA)
    {
        for (int i = 0; i < 4; i++)
        {
            m.Caja({ cx - 3.4f + i * 2.25f, SUELO_SALA_Y + 0.4f, cz + 4.51f }, { 0.05f, 0.8f, 0.02f }, PIEDRA_JUNTA);
        }
    }

    // Caballetes (planos z = cz +- 4).
    for (int lado = -1; lado <= 1; lado += 2)
    {
        float z = cz + lado * 4.0f;

        Viga(m, { cx - 3.8f, base, z }, { cx - 0.5f, 13.4f, z }, 0.32f, MADERA_OSCURA);
        Viga(m, { cx + 3.8f, base, z }, { cx + 0.5f, 13.4f, z }, 0.32f, MADERA_OSCURA);
        m.Caja({ cx, 6.0f, z }, { 5.4f, 0.45f, 0.5f }, MADERA);
        m.Caja({ cx, 12.9f, z }, { 2.4f, 0.9f, 0.9f }, MADERA_OSCURA);
        m.Caja({ cx, CAMPANA_PIVOTE_Y, z }, { 0.7f, 0.7f, 1.0f }, ACERO);
        Viga(m, { cx - 2.6f, 6.0f, z }, { cx - 1.4f, 10.2f, z }, 0.16f, MADERA);
        Viga(m, { cx + 2.6f, 6.0f, z }, { cx + 1.4f, 10.2f, z }, 0.16f, MADERA);
    }

    // Vigas que unen los caballetes arriba.
    for (int lado = -1; lado <= 1; lado += 2)
    {
        m.Caja({ cx + lado * 1.1f, 13.6f, cz }, { 0.45f, 0.45f, 9.4f }, MADERA_OSCURA);
    }

    // Tejado a cuatro aguas sobre canecillos.
    const float alero = 5.1f;
    const float yAlero = 14.1f;
    Vector3 cumbre = { cx, 18.4f, cz };
    Vector3 esquinas[4] =
    {
        { cx - alero, yAlero, cz - alero },
        { cx + alero, yAlero, cz - alero },
        { cx + alero, yAlero, cz + alero },
        { cx - alero, yAlero, cz + alero }
    };

    for (int i = 0; i < 4; i++)
    {
        Vector3 a = esquinas[i];
        Vector3 b = esquinas[(i + 1) % 4];

        m.TrianguloDobleCara(a, b, cumbre, i % 2 == 0 ? tejas : Oscurecer(tejas, 0.86f));
        Viga(m, a, cumbre, 0.14f, MADERA_OSCURA);

        if (nivel != CALIDAD_BAJA)
        {
            // Hiladas de tejas.
            for (int f = 1; f < 5; f++)
            {
                float t = f / 5.0f;
                Vector3 p0 = { a.x + (cumbre.x - a.x) * t, a.y + (cumbre.y - a.y) * t + 0.03f, a.z + (cumbre.z - a.z) * t };
                Vector3 p1 = { b.x + (cumbre.x - b.x) * t, b.y + (cumbre.y - b.y) * t + 0.03f, b.z + (cumbre.z - b.z) * t };

                Viga(m, p0, p1, 0.07f, Oscurecer(tejas, 0.72f));
            }
        }
    }

    m.Caja({ cx, yAlero - 0.2f, cz }, { 2.0f * alero, 0.3f, 2.0f * alero }, MADERA_OSCURA);

    // Linterna y veleta dorada.
    m.Caja({ cx, 18.7f, cz }, { 0.9f, 0.9f, 0.9f }, MADERA_ROJIZA);
    m.Cilindro({ cx, 19.1f, cz }, { cx, 20.6f, cz }, 0.06f, 0.06f, 6, ORO_RELOJ);
    m.Esfera({ cx, 19.4f, cz }, 0.18f, 4, 6, ORO_RELOJ);
    m.TrianguloDobleCara({ cx, 20.1f, cz }, { cx + 1.1f, 20.25f, cz }, { cx + 0.8f, 20.55f, cz }, ORO_RELOJ);
    m.TrianguloDobleCara({ cx, 20.1f, cz }, { cx - 0.6f, 20.3f, cz }, { cx - 0.6f, 20.0f, cz }, ORO_RELOJ);

    // Rueda de la cuerda, cuerda y escalera.
    m.Cilindro({ cx, CAMPANA_PIVOTE_Y, cz + 3.3f }, { cx, CAMPANA_PIVOTE_Y, cz + 3.5f }, 1.3f, 1.3f, 14, MADERA);
    Viga(m, { cx + 1.25f, CAMPANA_PIVOTE_Y, cz + 3.4f }, { cx + 2.2f, base, cz + 3.6f }, 0.05f, Color{ 190, 164, 120, 255 });

    for (int lado = -1; lado <= 1; lado += 2)
    {
        Viga(m, { cx - 3.0f + lado * 0.45f, base, cz - 4.4f }, { cx - 1.2f + lado * 0.45f, 11.0f, cz - 4.4f }, 0.08f, MADERA_CLARA);
    }

    for (int r = 1; r < 12; r++)
    {
        float t = r / 12.0f;
        float x = cx - 3.0f + 1.8f * t;
        float y = base + (11.0f - base) * t;

        m.Caja({ x, y, cz - 4.4f }, { 1.0f, 0.08f, 0.08f }, MADERA_CLARA);
    }

    // Carillon de campanitas en el centro del lobulo.
    const float mx = MESA_LOBULO_X;

    for (int lado = -1; lado <= 1; lado += 2)
    {
        m.Caja({ mx, 1.2f, lado * 0.85f }, { 0.16f, 2.4f, 0.16f }, MADERA_OSCURA);
    }

    m.Caja({ mx, 2.4f, 0.0f }, { 0.22f, 0.22f, 1.9f }, MADERA_OSCURA);
    m.Caja({ mx, 0.05f, 0.0f }, { 1.1f, 0.1f, 2.0f }, MADERA);

    for (int k = -1; k <= 1; k++)
    {
        float r = 0.34f - 0.06f * k;
        float y = 2.25f - 0.15f * (k + 1);

        m.Cilindro({ mx, y, k * 0.5f }, { mx, y - 0.65f + 0.08f * k, k * 0.5f }, r * 0.45f, r, 12, Color{ 176, 132, 70, 255 });
    }
}


// Mesa del relojero: tablero, cajonera, morsa, lampara con lupa, un
// movimiento desarmado y relojes de bolsillo abiertos. Banqueta al lado.
static void ConstruirMesaRelojero(ConstructorMalla& m)
{
    TransformarMalla(m, -26.8f, SUELO_SALA_Y, 11.8f, 90.0f);

    PatasMueble(m, 3.6f, 1.6f, 1.45f, 0.16f, MADERA_OSCURA);
    m.Caja({ 0.0f, 1.5f, 0.0f }, { 3.8f, 0.12f, 1.8f }, MADERA_CLARA);
    m.Caja({ 1.15f, 0.95f, 0.0f }, { 1.2f, 0.9f, 1.5f }, MADERA);

    for (int c = 0; c < 3; c++)
    {
        m.Caja({ 1.15f, 0.68f + c * 0.28f, 0.76f }, { 1.0f, 0.22f, 0.04f }, MADERA_CLARA);
        m.Caja({ 1.15f, 0.68f + c * 0.28f, 0.8f }, { 0.2f, 0.05f, 0.05f }, LATON);
    }

    // Morsa.
    m.Caja({ -1.5f, 1.68f, 0.5f }, { 0.4f, 0.25f, 0.5f }, ACERO);
    m.Cilindro({ -1.5f, 1.72f, 0.75f }, { -1.5f, 1.72f, 1.05f }, 0.05f, 0.05f, 6, ACERO);

    // Lampara articulada con lupa (el foco brilla).
    m.Cilindro({ 1.4f, 1.56f, -0.6f }, { 1.4f, 1.62f, -0.6f }, 0.22f, 0.22f, 10, BRONCE_OSCURO);
    Viga(m, { 1.4f, 1.6f, -0.6f }, { 1.0f, 2.6f, -0.5f }, 0.04f, BRONCE_OSCURO);
    Viga(m, { 1.0f, 2.6f, -0.5f }, { 0.3f, 2.3f, -0.1f }, 0.04f, BRONCE_OSCURO);
    m.Cilindro({ 0.3f, 2.3f, -0.1f }, { 0.25f, 2.05f, 0.0f }, 0.08f, 0.32f, 10, Color{ 46, 70, 54, 255 });
    m.Esfera({ 0.25f, 2.02f, 0.0f }, 0.12f, 4, 6, Emisivo(Color{ 255, 226, 150, 255 }));
    m.Cilindro({ -0.3f, 1.95f, 0.2f }, { -0.3f, 1.98f, 0.2f }, 0.28f, 0.28f, 12, Color{ 150, 180, 196, 255 });

    // Movimiento desarmado y relojes de bolsillo.
    m.Cilindro({ -0.4f, 1.56f, 0.2f }, { -0.4f, 1.66f, 0.2f }, 0.42f, 0.42f, 14, LATON);
    m.Cilindro({ -0.25f, 1.66f, 0.1f }, { -0.25f, 1.72f, 0.1f }, 0.2f, 0.2f, 10, BRONCE);
    m.Cilindro({ -0.6f, 1.66f, 0.35f }, { -0.6f, 1.71f, 0.35f }, 0.14f, 0.14f, 8, ORO_RELOJ);

    for (int k = 0; k < 3; k++)
    {
        float x = 0.6f + 0.42f * k;

        m.Cilindro({ x, 1.56f, 0.5f }, { x, 1.62f, 0.5f }, 0.16f, 0.16f, 10, ORO_RELOJ);
        m.Cilindro({ x, 1.62f, 0.5f }, { x, 1.63f, 0.5f }, 0.12f, 0.12f, 10, MARFIL);
    }

    // Banqueta.
    m.Cilindro({ 0.0f, 0.0f, 1.6f }, { 0.0f, 0.95f, 1.6f }, 0.08f, 0.08f, 6, MADERA_OSCURA);
    m.Cilindro({ 0.0f, 0.95f, 1.6f }, { 0.0f, 1.05f, 1.6f }, 0.42f, 0.42f, 12, MADERA_ROJIZA);
    m.Cilindro({ 0.0f, 0.05f, 1.6f }, { 0.0f, 0.12f, 1.6f }, 0.4f, 0.4f, 8, MADERA_OSCURA);

    m.QuitarTransformacion();
}


// Banco largo bajo el tablero de herramientas, con taladro de columna,
// relojes a medio arreglar y un candil.
static void ConstruirBancoLargo(ConstructorMalla& m)
{
    TransformarMalla(m, -31.2f, SUELO_SALA_Y, 1.0f, 90.0f);

    PatasMueble(m, 9.2f, 2.2f, 1.3f, 0.2f, MADERA_OSCURA);
    m.Caja({ 0.0f, 1.36f, 0.0f }, { 9.4f, 0.14f, 2.3f }, MADERA_CLARA);
    m.Caja({ 0.0f, 0.35f, 0.0f }, { 9.0f, 0.08f, 2.0f }, MADERA);

    // Cajones bajo el tablero.
    for (int c = -3; c <= 3; c++)
    {
        m.Caja({ c * 1.25f, 1.12f, 1.0f }, { 1.1f, 0.3f, 0.08f }, MADERA);
        m.Caja({ c * 1.25f, 1.12f, 1.05f }, { 0.22f, 0.05f, 0.05f }, LATON);
    }

    // Taladro de columna.
    m.Caja({ -3.8f, 1.5f, -0.2f }, { 0.8f, 0.14f, 0.8f }, ACERO);
    m.Cilindro({ -3.8f, 1.5f, -0.5f }, { -3.8f, 3.2f, -0.5f }, 0.1f, 0.1f, 8, ACERO);
    m.Caja({ -3.8f, 3.0f, -0.25f }, { 0.4f, 0.5f, 0.7f }, Color{ 60, 86, 70, 255 });
    m.Cilindro({ -3.8f, 2.75f, 0.05f }, { -3.8f, 2.35f, 0.05f }, 0.04f, 0.02f, 6, ACERO);
    m.Cilindro({ -3.55f, 3.0f, 0.0f }, { -3.1f, 3.0f, 0.0f }, 0.03f, 0.03f, 6, ACERO);

    // Relojes en arreglo: cajas con la esfera hacia la sala.
    for (int k = 0; k < 4; k++)
    {
        float x = -1.8f + k * 1.4f;
        float alto = 0.7f + 0.25f * (k % 2);

        m.Caja({ x, 1.43f + alto * 0.5f, -0.2f }, { 0.8f, alto, 0.5f }, k % 2 == 0 ? MADERA_ROJIZA : MADERA);
        m.Cilindro({ x, 1.43f + alto * 0.6f, 0.05f }, { x, 1.43f + alto * 0.6f, 0.09f }, 0.27f, 0.27f, 12, MARFIL);
    }

    // Candil.
    m.Cilindro({ 3.9f, 1.43f, 0.4f }, { 3.9f, 1.55f, 0.4f }, 0.25f, 0.2f, 10, LATON);
    m.Cilindro({ 3.9f, 1.55f, 0.4f }, { 3.9f, 1.95f, 0.4f }, 0.14f, 0.16f, 10, Emisivo(Color{ 255, 206, 120, 255 }));
    m.Cilindro({ 3.9f, 1.95f, 0.4f }, { 3.9f, 2.05f, 0.4f }, 0.16f, 0.1f, 10, LATON);

    m.QuitarTransformacion();
}


// Torno de relojero sobre su banco, con volante y pedal.
static void ConstruirTorno(ConstructorMalla& m)
{
    TransformarMalla(m, -27.6f, SUELO_SALA_Y, -9.4f, 0.0f);

    PatasMueble(m, 3.0f, 1.1f, 1.2f, 0.18f, Color{ 60, 58, 66, 255 });
    m.Caja({ 0.0f, 1.25f, 0.0f }, { 3.2f, 0.12f, 1.2f }, MADERA_OSCURA);
    m.Caja({ 0.0f, 1.42f, 0.0f }, { 2.6f, 0.22f, 0.36f }, ACERO);
    m.Caja({ -1.0f, 1.75f, 0.0f }, { 0.6f, 0.5f, 0.5f }, ACERO_AZUL);
    m.Cilindro({ -0.7f, 1.8f, 0.0f }, { -0.45f, 1.8f, 0.0f }, 0.18f, 0.18f, 10, ACERO);
    m.Caja({ 0.9f, 1.68f, 0.0f }, { 0.4f, 0.32f, 0.36f }, ACERO_AZUL);
    m.Cilindro({ 0.7f, 1.75f, 0.0f }, { 0.35f, 1.75f, 0.0f }, 0.05f, 0.02f, 6, ACERO);
    m.Caja({ 0.1f, 1.62f, 0.25f }, { 0.12f, 0.2f, 0.2f }, ACERO);

    // Volante y pedal.
    m.Cilindro({ -1.75f, 0.75f, 0.0f }, { -1.6f, 0.75f, 0.0f }, 0.65f, 0.65f, 16, Color{ 60, 58, 66, 255 });
    m.Cilindro({ -1.8f, 0.75f, 0.0f }, { -1.55f, 0.75f, 0.0f }, 0.12f, 0.12f, 8, ACERO);
    Viga(m, { -1.68f, 1.4f, 0.0f }, { -1.68f, 0.75f, 0.0f }, 0.03f, Color{ 120, 90, 60, 255 });
    m.Caja({ -0.6f, 0.12f, 0.45f }, { 1.4f, 0.06f, 0.4f }, Color{ 60, 58, 66, 255 });

    m.QuitarTransformacion();
}


// Vitrina de relojes de bolsillo junto a la tienda del taller.
static void ConstruirVitrina(ConstructorMalla& m)
{
    TransformarMalla(m, -22.2f, SUELO_SALA_Y, 11.0f, 0.0f);

    m.Caja({ 0.0f, 0.5f, 0.0f }, { 2.4f, 1.0f, 1.0f }, MADERA_ROJIZA);
    m.Caja({ 0.0f, 1.03f, 0.0f }, { 2.5f, 0.06f, 1.1f }, LATON);
    m.Caja({ 0.0f, 1.08f, 0.0f }, { 2.3f, 0.04f, 0.9f }, Color{ 110, 30, 40, 255 });

    // Relojes sobre terciopelo.
    for (int k = 0; k < 5; k++)
    {
        float x = -0.9f + k * 0.45f;
        float z = (k % 2 == 0) ? -0.15f : 0.2f;

        m.Cilindro({ x, 1.1f, z }, { x, 1.15f, z }, 0.15f, 0.15f, 10, ORO_RELOJ);
        m.Cilindro({ x, 1.15f, z }, { x, 1.16f, z }, 0.11f, 0.11f, 10, MARFIL);
    }

    // Marco del vidrio (aristas de laton).
    for (int i = -1; i <= 1; i += 2)
    {
        for (int j = -1; j <= 1; j += 2)
        {
            m.Caja({ i * 1.2f, 1.45f, j * 0.5f }, { 0.05f, 0.75f, 0.05f }, LATON);
        }

        m.Caja({ 0.0f, 1.82f, i * 0.5f }, { 2.45f, 0.05f, 0.05f }, LATON);
        m.Caja({ i * 1.2f, 1.82f, 0.0f }, { 0.05f, 0.05f, 1.05f }, LATON);
    }

    // Cartelito de precio.
    m.Caja({ 0.0f, 0.6f, 0.52f }, { 0.8f, 0.3f, 0.03f }, MARFIL);

    m.QuitarTransformacion();
}


// Caja fuerte de piezas, carro de herramientas, mesita del taller y atril.
static void ConstruirMueblesSueltos(ConstructorMalla& m)
{
    // Caja fuerte.
    TransformarMalla(m, -16.0f, SUELO_SALA_Y, -17.5f, 15.0f);

    m.Caja({ 0.0f, 1.0f, 0.0f }, { 1.7f, 2.0f, 1.5f }, Color{ 50, 74, 62, 255 });
    m.Caja({ 0.0f, 1.0f, 0.77f }, { 1.4f, 1.7f, 0.04f }, Color{ 60, 88, 72, 255 });
    m.Cilindro({ 0.2f, 1.3f, 0.78f }, { 0.2f, 1.3f, 0.86f }, 0.25f, 0.25f, 14, LATON);
    m.Cilindro({ 0.2f, 1.3f, 0.86f }, { 0.2f, 1.3f, 0.9f }, 0.1f, 0.1f, 8, BRONCE_OSCURO);
    m.Caja({ -0.35f, 0.8f, 0.82f }, { 0.08f, 0.5f, 0.08f }, LATON);
    m.Caja({ 0.0f, 2.03f, 0.0f }, { 1.8f, 0.08f, 1.6f }, LATON);

    m.QuitarTransformacion();

    // Carro de herramientas con ruedas.
    TransformarMalla(m, 13.5f, SUELO_SALA_Y, -18.4f, -10.0f);

    m.Caja({ 0.0f, 0.75f, 0.0f }, { 1.8f, 1.0f, 0.9f }, Color{ 140, 44, 38, 255 });

    for (int c = 0; c < 3; c++)
    {
        m.Caja({ 0.0f, 0.45f + c * 0.3f, 0.46f }, { 1.6f, 0.24f, 0.03f }, Color{ 160, 56, 46, 255 });
        m.Caja({ 0.0f, 0.45f + c * 0.3f, 0.49f }, { 0.4f, 0.05f, 0.04f }, ACERO);
    }

    for (int i = -1; i <= 1; i += 2)
    {
        for (int j = -1; j <= 1; j += 2)
        {
            m.Cilindro({ i * 0.75f, 0.12f, j * 0.38f - 0.05f }, { i * 0.75f, 0.12f, j * 0.38f + 0.05f }, 0.12f, 0.12f, 8, PIEDRA_JUNTA);
        }
    }

    m.Caja({ -0.4f, 1.32f, 0.0f }, { 0.8f, 0.14f, 0.12f }, ACERO);
    m.Caja({ 0.4f, 1.3f, 0.1f }, { 0.5f, 0.1f, 0.4f }, LATON);
    Viga(m, { 0.95f, 1.2f, -0.35f }, { 1.2f, 1.6f, -0.35f }, 0.04f, ACERO);
    Viga(m, { 0.95f, 1.2f, 0.35f }, { 1.2f, 1.6f, 0.35f }, 0.04f, ACERO);

    m.QuitarTransformacion();

    // Mesita redonda del centro del taller con un reloj abierto.
    TransformarMalla(m, -MESA_LOBULO_X, 0.0f, 0.0f, 0.0f);

    m.Cilindro({ 0.0f, 0.0f, 0.0f }, { 0.0f, 0.08f, 0.0f }, 0.55f, 0.55f, 12, MADERA_OSCURA);
    m.Cilindro({ 0.0f, 0.08f, 0.0f }, { 0.0f, 0.95f, 0.0f }, 0.1f, 0.1f, 8, MADERA_OSCURA);
    m.Cilindro({ 0.0f, 0.95f, 0.0f }, { 0.0f, 1.05f, 0.0f }, 0.95f, 0.95f, 18, MADERA_ROJIZA);
    m.Cilindro({ 0.0f, 1.05f, 0.0f }, { 0.0f, 1.2f, 0.0f }, 0.5f, 0.5f, 16, ORO_RELOJ);
    m.Cilindro({ 0.0f, 1.2f, 0.0f }, { 0.0f, 1.24f, 0.0f }, 0.4f, 0.4f, 16, LATON);
    m.Cilindro({ 0.15f, 1.24f, 0.05f }, { 0.15f, 1.29f, 0.05f }, 0.18f, 0.18f, 10, BRONCE);
    m.Cilindro({ -0.18f, 1.24f, -0.1f }, { -0.18f, 1.28f, -0.1f }, 0.12f, 0.12f, 8, ORO_RELOJ);
    Viga(m, { 0.0f, 1.2f, -0.5f }, { 0.0f, 1.75f, -0.75f }, 0.04f, ORO_RELOJ);
    m.Cilindro({ 0.0f, 1.75f, -0.75f }, { 0.0f, 1.95f, -0.85f }, 0.48f, 0.48f, 16, ORO_RELOJ);

    m.QuitarTransformacion();

    // Atril con el libro de planos y una vela.
    TransformarMalla(m, 0.0f, SUELO_SALA_Y, 27.0f, 0.0f);

    m.Cilindro({ 0.0f, 0.0f, 0.0f }, { 0.0f, 0.1f, 0.0f }, 0.7f, 0.7f, 12, MADERA_OSCURA);
    m.Cilindro({ 0.0f, 0.1f, 0.0f }, { 0.0f, 1.5f, 0.0f }, 0.12f, 0.12f, 8, MADERA_OSCURA);
    m.Caja({ 0.0f, 1.6f, 0.0f }, { 1.6f, 0.1f, 1.1f }, MADERA_ROJIZA);
    m.Caja({ -0.38f, 1.7f, 0.0f }, { 0.7f, 0.06f, 0.95f }, MARFIL);
    m.Caja({ 0.38f, 1.7f, 0.0f }, { 0.7f, 0.06f, 0.95f }, MARFIL);
    m.Caja({ 0.0f, 1.69f, 0.0f }, { 0.06f, 0.06f, 0.95f }, MADERA_ROJIZA);
    m.Cilindro({ 0.7f, 1.65f, 0.4f }, { 0.7f, 2.0f, 0.4f }, 0.07f, 0.07f, 6, MARFIL);
    m.Esfera({ 0.7f, 2.07f, 0.4f }, 0.07f, 4, 6, Emisivo(Color{ 255, 214, 120, 255 }));

    m.QuitarTransformacion();
}


// Maquina de la sala de engranajes (los engranajes son mallas aparte),
// tambor de pesas, placa del muro este y soportes del rincon oeste.
static void ConstruirMaquinaEngranajes(
    ConstructorMalla& m,
    NivelCalidadGrafica nivel
)
{
    const float x = ENGRANAJES_X;
    const float z = ENGRANAJES_Z;
    const Color hierro = Color{ 54, 54, 64, 255 };

    m.Caja({ x, SUELO_SALA_Y + 0.4f, z - 0.2f }, { 8.6f, 0.8f, 2.4f }, PIEDRA);
    m.Caja({ x, SUELO_SALA_Y + 0.85f, z - 0.2f }, { 8.8f, 0.1f, 2.6f }, PIEDRA_CLARA);

    for (int lado = -1; lado <= 1; lado += 2)
    {
        m.Caja({ x + lado * 3.9f, 5.2f, z - 0.75f }, { 0.4f, 9.6f, 0.4f }, hierro);
        m.Caja({ x + lado * 3.9f, 5.2f, z - 0.75f }, { 0.12f, 9.6f, 0.6f }, hierro);
    }

    m.Caja({ x, 9.9f, z - 0.75f }, { 8.2f, 0.4f, 0.5f }, hierro);
    m.Caja({ x, 4.0f, z - 0.75f }, { 8.0f, 0.25f, 0.25f }, hierro);
    Viga(m, { x - 3.9f, 1.0f, z - 0.75f }, { x + 3.9f, 9.7f, z - 0.75f }, 0.1f, hierro);

    // Ejes de los engranajes hasta el bastidor.
    const float ejes[3][3] =
    {
        { x, 4.0f, z },
        { x - 2.43f, 7.46f, z + 0.35f },
        { x + 3.46f, 2.74f, z + 0.35f }
    };

    for (const auto& e : ejes)
    {
        m.Cilindro({ e[0], e[1], z - 0.75f }, { e[0], e[1], e[2] + 0.5f }, 0.18f, 0.18f, 8, ACERO);
        m.Cilindro({ e[0], e[1], z - 0.9f }, { e[0], e[1], z - 0.55f }, 0.35f, 0.35f, 10, LATON);
    }

    // Tambor de pesas con su cable y la pesa.
    const float tx = ENGRANAJES_X - 1.5f;
    const float tz = -6.4f;

    for (int lado = -1; lado <= 1; lado += 2)
    {
        m.Caja({ tx + lado * 1.6f, 1.2f, tz }, { 0.35f, 3.0f, 1.2f }, hierro);
    }

    m.Cilindro({ tx - 1.4f, 2.3f, tz }, { tx + 1.4f, 2.3f, tz }, 0.75f, 0.75f, 16, MADERA);
    m.Cilindro({ tx - 1.8f, 2.3f, tz }, { tx + 1.8f, 2.3f, tz }, 0.14f, 0.14f, 8, ACERO);

    if (nivel != CALIDAD_BAJA)
    {
        for (int k = 0; k < 6; k++)
        {
            float xx = tx - 1.2f + k * 0.48f;
            m.Cilindro({ xx, 2.3f, tz }, { xx + 0.06f, 2.3f, tz }, 0.78f, 0.78f, 16, Color{ 170, 150, 110, 255 });
        }
    }

    m.Cilindro({ tx + 1.0f, 0.4f, tz - 0.9f }, { tx + 1.0f, 1.4f, tz - 0.9f }, 0.45f, 0.45f, 12, PIEDRA_JUNTA);
    Viga(m, { tx + 1.0f, 1.4f, tz - 0.9f }, { tx + 1.0f, 2.3f, tz - 0.7f }, 0.03f, ACERO);

    // Placa remachada del muro este para sus engranajes.
    MarcoPared p = MarcoMuro(MURO_ESTE);

    PlacaPared(m, p, 0.8f, 8.6f, 9.6f, 15.6f, 0.12f, hierro);

    if (nivel != CALIDAD_BAJA)
    {
        for (float u = 1.1f; u < 9.5f; u += 0.8f)
        {
            DiscoPared(m, p, u, 8.85f, 0.12f, 0.2f, 0.07f, 6, ACERO);
            DiscoPared(m, p, u, 15.35f, 0.12f, 0.2f, 0.07f, 6, ACERO);
        }
    }

    DiscoPared(m, p, 4.0f, 11.6f, 0.12f, 0.9f, 0.18f, 8, ACERO);
    DiscoPared(m, p, 7.6f, 13.1f, 0.12f, 0.9f, 0.18f, 8, ACERO);

    // Soportes de los engranajes acostados del rincon oeste.
    const float soportes[2][2] = { { -25.4f, -16.6f }, { -21.5f, -15.6f } };

    for (const auto& s : soportes)
    {
        m.Cilindro({ s[0], SUELO_SALA_Y, s[1] }, { s[0], SUELO_SALA_Y + 0.12f, s[1] }, 0.9f, 0.9f, 12, PIEDRA);
        m.Cilindro({ s[0], SUELO_SALA_Y + 0.12f, s[1] }, { s[0], 0.3f, s[1] }, 0.28f, 0.22f, 10, hierro);
    }
}


// Un cartel por bifurcacion: una tabla por camino apuntando a su primera
// casilla (la clara es la opcion 0, la rojiza el atajo).
static void ConstruirCartelesReloj(ConstructorMalla& m)
{
    const Tablero& t = TableroReferenciaReloj();

    // Lugares candidatos alrededor de la casilla (primero los de Arboleda).
    const float lugares[][2] =
    {
        { -1.8f, 1.4f }, { 1.8f, 1.4f },
        { -2.3f, -0.6f }, { 2.3f, -0.6f },
        { -1.7f, -1.9f }, { 1.7f, -1.9f },
        { -2.6f, 0.6f }, { 2.6f, 0.6f },
        { 0.0f, -2.5f }, { -2.2f, -2.2f }, { 2.2f, -2.2f }
    };

    for (int i = 0; i < t.cantidadCasillas; i++)
    {
        const Casilla& c = t.casillas[i];

        if (c.cantidadConexiones < 2)
        {
            continue;
        }

        // El lugar mas despejado de los que no tapan otra casilla.
        float px = c.posicion.x + lugares[0][0];
        float pz = c.posicion.z + lugares[0][1];
        float mejor = -1.0f;

        for (const auto& l : lugares)
        {
            float x = c.posicion.x + l[0];
            float z = c.posicion.z + l[1];
            float r = std::sqrt(x * x + z * z);

            // Fuera del aro giratorio y de los pinones.
            if (r > RADIO_CARA - 0.2f && r < RADIO_PLATAFORMA_INTERIOR + 0.2f)
            {
                continue;
            }

            if (!DespejadoReloj(x, z, 0.3f))
            {
                continue;
            }

            float distancia = DistanciaSenderoReloj(x, z);

            if (distancia > mejor + 0.15f)
            {
                mejor = distancia;
                px = x;
                pz = z;
            }

            if (distancia >= 2.05f)
            {
                break;
            }
        }

        float y = AlturaSueloReloj(px, pz);

        m.Cilindro({ px, y, pz }, { px, y + 0.12f, pz }, 0.22f, 0.22f, 8, LATON);
        m.Cilindro({ px, y, pz }, { px, y + 1.9f, pz }, 0.07f, 0.06f, 6, MADERA_OSCURA);
        m.Esfera({ px, y + 1.95f, pz }, 0.09f, 4, 6, LATON);

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

            CajaGirada(m, px + ux * 0.55f, ty, pz + uz * 0.55f, giro * GRADOS, Vector3{ 1.1f, 0.22f, 0.07f }, tabla);
            CajaGirada(m, px + ux * 1.15f, ty, pz + uz * 1.15f, giro * GRADOS, Vector3{ 0.2f, 0.12f, 0.07f }, tabla);
        }
    }
}


static void ConstruirMallaLandmarksReloj(NivelCalidadGrafica nivel)
{
    static ConstructorMalla m;
    m.Limpiar();
    m.reduccionDetalle = nivel == CALIDAD_ALTA ? 0 : 1;

    ConstruirRelojPie(m, nivel);
    ConstruirCampanario(m, nivel);
    ConstruirMesaRelojero(m);
    ConstruirBancoLargo(m);
    ConstruirTorno(m);
    ConstruirVitrina(m);
    ConstruirMueblesSueltos(m);
    ConstruirMaquinaEngranajes(m, nivel);
    ConstruirCartelesReloj(m);

    IluminarMalla(m, 0);

    ActualizarMallaGpu(mallaLandmarksReloj, m);
    nivelLandmarksReloj = (int)nivel;
}


//==================================================
// PIEZAS DE ESCENOGRAFIA (se emiten a la malla de decoracion)
//==================================================
//
// Cada pieza emite en coordenadas de mundo (o con una transformacion local
// que se quita al terminar); la misma funcion se emite muchas veces.

static Color TonoMetal(float variacion)
{
    if (variacion < 0.4f)
    {
        return Mezcla(LATON, ORO_RELOJ, variacion * 2.0f);
    }

    if (variacion < 0.75f)
    {
        return Mezcla(BRONCE, COBRE, (variacion - 0.4f) * 2.5f);
    }

    return Mezcla(ACERO, Color{ 120, 122, 134, 255 }, (variacion - 0.75f) * 4.0f);
}


// Engranaje suelto acostado en el piso (dientes planos sobre la tapa).
static void PiezaEngranajeSuelto(
    ConstructorMalla& m,
    float x, float y, float z,
    float escala,
    float variacion,
    float giro
)
{
    float r = 0.34f * escala;
    float alto = 0.07f * escala;
    Color c = TonoMetal(variacion);
    int dientes = 8 + (int)(variacion * 6.0f);

    m.Cilindro({ x, y, z }, { x, y + alto, z }, r, r, 12, c);
    m.Cilindro({ x, y + alto, z }, { x, y + alto + 0.01f, z }, r * 0.3f, r * 0.3f, 8, Oscurecer(c, 0.55f));

    for (int i = 0; i < dientes; i++)
    {
        float a = giro * DEG2RAD + 2.0f * PI_RELOJ * i / dientes;
        float ca = std::cos(a);
        float sa = std::sin(a);

        TrazoPlano(m, x + ca * r * 0.92f, z + sa * r * 0.92f, x + ca * (r + 0.09f * escala), z + sa * (r + 0.09f * escala), 0.08f * escala, y + alto * 0.6f, c);
    }
}


// Punado de tuercas, tornillos y arandelas.
static void PiezaTuercas(
    ConstructorMalla& m,
    float x, float y, float z,
    float escala,
    float variacion
)
{
    int cantidad = 3 + (int)(variacion * 4.0f);

    for (int k = 0; k < cantidad; k++)
    {
        float a = k * 2.3f + variacion * 6.0f;
        float d = 0.08f + 0.06f * k * escala;
        float px = x + std::cos(a) * d;
        float pz = z + std::sin(a) * d;
        Color c = TonoMetal(std::fmod(variacion + k * 0.37f, 1.0f));

        if (k % 3 == 2)
        {
            // Tornillo acostado.
            m.Cilindro({ px, y + 0.03f, pz }, { px + std::cos(a + 1.0f) * 0.24f * escala, y + 0.03f, pz + std::sin(a + 1.0f) * 0.24f * escala }, 0.025f, 0.025f, 4, c);
            m.Cilindro({ px, y, pz }, { px, y + 0.06f, pz }, 0.05f, 0.05f, 6, c);
        }
        else
        {
            m.Cilindro({ px, y, pz }, { px, y + 0.05f * escala, pz }, 0.07f * escala, 0.07f * escala, 6, c);
        }
    }
}


// Resorte en espiral (anillos alternados).
static void PiezaResorte(
    ConstructorMalla& m,
    float x, float y, float z,
    float escala,
    float variacion
)
{
    Color c = variacion < 0.5f ? ACERO : LATON;
    float r = 0.11f * escala;

    for (int k = 0; k < 6; k++)
    {
        float dx = (k % 2 == 0 ? 0.015f : -0.015f) * escala;
        float y0 = y + k * 0.05f * escala;

        m.Cilindro({ x + dx, y0, z }, { x - dx, y0 + 0.035f * escala, z }, r, r, 8, c);
    }
}


// Virutas de laton o cobre: tiras en zigzag pegadas al piso.
static void PiezaViruta(
    ConstructorMalla& m,
    float x, float y, float z,
    float escala,
    float variacion,
    float giro
)
{
    Color c = variacion < 0.5f ? LATON : COBRE;

    for (int k = 0; k < 4; k++)
    {
        float a = giro * DEG2RAD + k * 1.6f;
        float x0 = x + std::cos(a) * 0.1f * k * escala;
        float z0 = z + std::sin(a) * 0.1f * k * escala;
        float x1 = x0 + std::cos(a + 0.9f) * 0.22f * escala;
        float z1 = z0 + std::sin(a + 0.9f) * 0.22f * escala;
        float x2 = x1 + std::cos(a - 0.6f) * 0.2f * escala;
        float z2 = z1 + std::sin(a - 0.6f) * 0.2f * escala;

        TrazoPlano(m, x0, z0, x1, z1, 0.05f, y + 0.02f + 0.01f * k, c);
        TrazoPlano(m, x1, z1, x2, z2, 0.05f, y + 0.025f + 0.01f * k, Oscurecer(c, 0.85f));
    }
}


// Herramienta en el piso: llave, martillo o destornillador.
static void PiezaHerramienta(
    ConstructorMalla& m,
    float x, float y, float z,
    float escala,
    float variacion,
    float giro
)
{
    float a = giro * DEG2RAD;
    float ux = std::cos(a);
    float uz = std::sin(a);
    float l = 0.45f * escala;

    if (variacion < 0.35f)
    {
        TrazoPlano(m, x - ux * l, z - uz * l, x + ux * l, z + uz * l, 0.09f * escala, y + 0.02f, ACERO);
        m.Cilindro({ x + ux * l, y, z + uz * l }, { x + ux * l, y + 0.04f, z + uz * l }, 0.13f * escala, 0.13f * escala, 8, ACERO);
        m.Cilindro({ x - ux * l, y, z - uz * l }, { x - ux * l, y + 0.04f, z - uz * l }, 0.1f * escala, 0.1f * escala, 8, ACERO);
    }
    else if (variacion < 0.7f)
    {
        m.Cilindro({ x - ux * l, y + 0.05f, z - uz * l }, { x + ux * l * 0.7f, y + 0.05f, z + uz * l * 0.7f }, 0.045f * escala, 0.04f * escala, 6, MADERA_CLARA);
        m.Cilindro({ x + ux * l * 0.7f - uz * 0.16f * escala, y + 0.07f, z + uz * l * 0.7f + ux * 0.16f * escala }, { x + ux * l * 0.7f + uz * 0.16f * escala, y + 0.07f, z + uz * l * 0.7f - ux * 0.16f * escala }, 0.07f * escala, 0.07f * escala, 6, ACERO);
    }
    else
    {
        m.Cilindro({ x - ux * l, y + 0.05f, z - uz * l }, { x, y + 0.05f, z }, 0.06f * escala, 0.06f * escala, 6, Color{ 160, 52, 40, 255 });
        m.Cilindro({ x, y + 0.05f, z }, { x + ux * l, y + 0.05f, z + uz * l }, 0.02f * escala, 0.015f * escala, 4, ACERO);
    }
}


// Cajon de madera con listones (a veces dos apilados).
static void PiezaCajon(
    ConstructorMalla& m,
    float x, float y, float z,
    float escala,
    float variacion,
    float giro
)
{
    float s = 0.9f * escala;
    Color madera = Oscurecer(Mezcla(MADERA_CLARA, Color{ 150, 112, 70, 255 }, variacion), 0.95f);

    TransformarMalla(m, x, y, z, giro);

    m.Caja({ 0.0f, s * 0.5f, 0.0f }, { s, s, s }, madera);

    for (int k = -1; k <= 1; k += 2)
    {
        m.Caja({ 0.0f, s * 0.5f, k * (s * 0.5f + 0.01f) }, { s * 0.95f, 0.1f, 0.03f }, Oscurecer(madera, 0.75f));
        m.Caja({ k * (s * 0.5f + 0.01f), s * 0.5f, 0.0f }, { 0.03f, 0.1f, s * 0.95f }, Oscurecer(madera, 0.75f));
    }

    m.Caja({ 0.0f, s + 0.02f, 0.0f }, { s * 0.95f, 0.04f, 0.12f }, Oscurecer(madera, 0.75f));

    if (variacion > 0.65f)
    {
        float s2 = s * 0.7f;
        m.Caja({ 0.05f, s + s2 * 0.5f, -0.05f }, { s2, s2, s2 }, Oscurecer(madera, 1.08f));
    }

    m.QuitarTransformacion();
}


// Caja de herramientas metalica con asa.
static void PiezaCajaHerramientas(
    ConstructorMalla& m,
    float x, float y, float z,
    float escala,
    float variacion,
    float giro
)
{
    Color c = variacion < 0.5f ? Color{ 150, 46, 38, 255 } : Color{ 54, 92, 72, 255 };

    TransformarMalla(m, x, y, z, giro);

    m.Caja({ 0.0f, 0.2f * escala, 0.0f }, { 0.9f * escala, 0.4f * escala, 0.42f * escala }, c);
    m.Caja({ 0.0f, 0.41f * escala, 0.0f }, { 0.92f * escala, 0.04f * escala, 0.44f * escala }, Oscurecer(c, 0.8f));
    m.Caja({ 0.0f, 0.55f * escala, 0.0f }, { 0.5f * escala, 0.05f * escala, 0.06f * escala }, ACERO);
    m.Caja({ -0.24f * escala, 0.49f * escala, 0.0f }, { 0.04f * escala, 0.14f * escala, 0.06f * escala }, ACERO);
    m.Caja({ 0.24f * escala, 0.49f * escala, 0.0f }, { 0.04f * escala, 0.14f * escala, 0.06f * escala }, ACERO);

    m.QuitarTransformacion();
}


// Reloj de mesa parado (capilla o carrillon), esfera hacia +z local.
static void PiezaRelojMesa(
    ConstructorMalla& m,
    float x, float y, float z,
    float escala,
    float variacion,
    float giro
)
{
    Color caja = variacion < 0.5f ? MADERA_ROJIZA : MADERA;

    TransformarMalla(m, x, y, z, giro);

    m.Caja({ 0.0f, 0.04f, 0.0f }, { 0.7f * escala, 0.08f, 0.4f * escala }, MADERA_OSCURA);
    m.Caja({ 0.0f, 0.42f * escala, 0.0f }, { 0.56f * escala, 0.7f * escala, 0.32f * escala }, caja);

    if (variacion > 0.5f)
    {
        m.Cilindro({ 0.0f, 0.77f * escala, -0.15f * escala }, { 0.0f, 0.77f * escala, 0.15f * escala }, 0.28f * escala, 0.28f * escala, 10, caja);
    }

    m.Cilindro({ 0.0f, 0.5f * escala, 0.16f * escala }, { 0.0f, 0.5f * escala, 0.19f * escala }, 0.21f * escala, 0.21f * escala, 12, LATON);
    m.Cilindro({ 0.0f, 0.5f * escala, 0.19f * escala }, { 0.0f, 0.5f * escala, 0.2f * escala }, 0.17f * escala, 0.17f * escala, 12, MARFIL);
    m.Caja({ 0.0f, 0.56f * escala, 0.205f * escala }, { 0.02f, 0.12f * escala, 0.01f }, TINTA);
    m.Caja({ 0.04f * escala, 0.5f * escala, 0.205f * escala }, { 0.09f * escala, 0.02f, 0.01f }, TINTA);

    m.QuitarTransformacion();
}


// Pila de libros acostados.
static void PiezaLibros(
    ConstructorMalla& m,
    float x, float y, float z,
    float escala,
    float variacion,
    float giro
)
{
    const Color tapas[] =
    {
        { 128, 40, 36, 255 }, { 46, 74, 112, 255 }, { 54, 96, 60, 255 },
        { 150, 112, 52, 255 }, { 90, 50, 86, 255 }
    };

    int libros = 2 + (int)(variacion * 4.0f);
    float yy = y;

    for (int k = 0; k < libros; k++)
    {
        float grosor = (0.1f + 0.06f * Hash2(k, (int)(variacion * 100.0f))) * escala;
        Color tapa = tapas[(k + (int)(variacion * 10.0f)) % 5];

        CajaGirada(m, x, yy + grosor * 0.5f, z, giro + k * 13.0f - 20.0f, { 0.62f * escala, grosor, 0.45f * escala }, tapa);
        CajaGirada(m, x, yy + grosor * 0.5f, z, giro + k * 13.0f - 20.0f, { 0.58f * escala, grosor * 0.7f, 0.47f * escala }, MARFIL);

        yy += grosor;
    }
}


// Saco de arpillera con engranajes derramados.
static void PiezaSaco(
    ConstructorMalla& m,
    float x, float y, float z,
    float escala,
    float variacion
)
{
    TransformacionMalla t;
    t.tx = x;
    t.ty = y;
    t.tz = z;
    t.sx = 1.0f;
    t.sy = 1.25f;
    t.sz = 0.85f;

    m.EstablecerTransformacion(t);
    m.Esfera({ 0.0f, 0.36f * escala, 0.0f }, 0.4f * escala, 5, 8, Color{ 156, 132, 92, 255 });
    m.Cilindro({ 0.0f, 0.72f * escala, 0.0f }, { 0.0f, 0.86f * escala, 0.0f }, 0.1f * escala, 0.16f * escala, 6, Color{ 132, 108, 72, 255 });
    m.QuitarTransformacion();

    PiezaEngranajeSuelto(m, x + 0.5f * escala, y, z + 0.2f, 0.45f, variacion, variacion * 90.0f);
    PiezaEngranajeSuelto(m, x + 0.3f * escala, y, z + 0.55f, 0.35f, 1.0f - variacion, 30.0f);
}


// Lampara de pie con globo encendido (el halo se anima aparte).
static void PiezaLampara(
    ConstructorMalla& m,
    float x, float y, float z,
    float escala
)
{
    float h = 2.3f * escala;

    m.Cilindro({ x, y, z }, { x, y + 0.08f, z }, 0.38f, 0.36f, 10, BRONCE_OSCURO);
    m.Cilindro({ x, y + 0.08f, z }, { x, y + h, z }, 0.05f, 0.05f, 6, BRONCE_OSCURO);
    m.Cilindro({ x, y + h - 0.05f, z }, { x, y + h + 0.05f, z }, 0.16f, 0.16f, 8, LATON);
    m.Esfera({ x, y + h + 0.25f, z }, 0.24f, 5, 8, Emisivo(Color{ 255, 220, 150, 255 }));
    m.Cilindro({ x, y + h + 0.42f, z }, { x, y + h + 0.55f, z }, 0.18f, 0.08f, 8, LATON);
}


// Reloj de pie chico (esfera hacia +z local).
static void PiezaRelojPieChico(
    ConstructorMalla& m,
    float x, float y, float z,
    float escala,
    float variacion,
    float giro
)
{
    Color caja = variacion < 0.5f ? MADERA_ROJIZA : MADERA;

    TransformarMalla(m, x, y, z, giro);

    m.Caja({ 0.0f, 0.3f * escala, 0.0f }, { 0.9f * escala, 0.6f * escala, 0.6f * escala }, MADERA_OSCURA);
    m.Caja({ 0.0f, 1.3f * escala, 0.0f }, { 0.66f * escala, 1.4f * escala, 0.46f * escala }, caja);
    m.Caja({ 0.0f, 1.3f * escala, 0.24f * escala }, { 0.36f * escala, 1.0f * escala, 0.02f }, Color{ 40, 28, 22, 255 });
    m.Cilindro({ 0.0f, 1.0f * escala, 0.0f }, { 0.0f, 0.55f * escala, 0.0f }, 0.02f, 0.02f, 4, LATON);
    m.Cilindro({ 0.0f, 0.55f * escala, 0.2f * escala }, { 0.0f, 0.55f * escala, 0.23f * escala }, 0.12f * escala, 0.12f * escala, 8, ORO_RELOJ);
    m.Caja({ 0.0f, 2.35f * escala, 0.0f }, { 0.8f * escala, 0.7f * escala, 0.55f * escala }, caja);
    m.Cilindro({ 0.0f, 2.35f * escala, 0.27f * escala }, { 0.0f, 2.35f * escala, 0.3f * escala }, 0.27f * escala, 0.27f * escala, 12, LATON);
    m.Cilindro({ 0.0f, 2.35f * escala, 0.3f * escala }, { 0.0f, 2.35f * escala, 0.31f * escala }, 0.22f * escala, 0.22f * escala, 12, MARFIL);
    m.Caja({ 0.0f, 2.78f * escala, 0.0f }, { 0.9f * escala, 0.16f * escala, 0.62f * escala }, MADERA_OSCURA);
    m.Esfera({ 0.0f, 2.95f * escala, 0.0f }, 0.1f * escala, 4, 6, LATON);

    m.QuitarTransformacion();
}


// Engranaje grande parado, apoyado en una cuna de madera.
static void PiezaEngranajeParado(
    ConstructorMalla& m,
    float x, float y, float z,
    float escala,
    float variacion,
    float giro
)
{
    float r = 0.75f * escala;
    Color c = TonoMetal(variacion);

    TransformarMalla(m, x, y, z, giro);

    m.Caja({ 0.0f, 0.1f, 0.0f }, { r * 1.4f, 0.2f, 0.5f }, MADERA_OSCURA);
    m.Cilindro({ 0.0f, r + 0.05f, -0.09f }, { 0.0f, r + 0.05f, 0.09f }, r, r, 16, c);
    m.Cilindro({ 0.0f, r + 0.05f, -0.12f }, { 0.0f, r + 0.05f, 0.12f }, r * 0.22f, r * 0.22f, 8, Oscurecer(c, 0.6f));

    for (int i = 0; i < 14; i++)
    {
        float a = 2.0f * PI_RELOJ * i / 14.0f;
        float px = std::sin(a) * (r + 0.07f);
        float py = r + 0.05f + std::cos(a) * (r + 0.07f);

        if (py < 0.15f)
        {
            continue;
        }

        m.Caja({ px, py, 0.0f }, { 0.12f * escala, 0.12f * escala, 0.16f }, c);
    }

    m.QuitarTransformacion();
}


// Alfombra con guarda y medallon (capas separadas 0.02).
static void PiezaAlfombra(
    ConstructorMalla& m,
    float x, float y, float z,
    float escala,
    float variacion,
    float giro
)
{
    const Color fondos[] =
    {
        { 118, 34, 40, 255 }, { 40, 56, 96, 255 }, { 96, 70, 40, 255 }
    };

    Color fondo = fondos[(int)(variacion * 2.99f)];
    Color guarda = Mezcla(fondo, ORO_RELOJ, 0.55f);
    float a = 2.4f * escala;
    float b = 1.6f * escala;

    TransformarMalla(m, x, y, z, giro);

    auto rect = [&](float ha, float hb, float yy, Color c)
    {
        CuadroDobleCara(m, { -ha, yy, -hb }, { ha, yy, -hb }, { ha, yy, hb }, { -ha, yy, hb }, c);
    };

    rect(a, b, 0.03f, fondo);
    rect(a - 0.2f, b - 0.2f, 0.05f, guarda);
    rect(a - 0.35f, b - 0.35f, 0.07f, fondo);

    // Medallon y flecos.
    m.TrianguloDobleCara({ -a * 0.5f, 0.09f, 0.0f }, { 0.0f, 0.09f, -b * 0.5f }, { a * 0.5f, 0.09f, 0.0f }, guarda);
    m.TrianguloDobleCara({ -a * 0.5f, 0.09f, 0.0f }, { a * 0.5f, 0.09f, 0.0f }, { 0.0f, 0.09f, b * 0.5f }, guarda);

    for (float f = -b + 0.1f; f < b; f += 0.2f)
    {
        TrazoPlano(m, -a, f, -a - 0.18f, f, 0.05f, 0.04f, MARFIL);
        TrazoPlano(m, a, f, a + 0.18f, f, 0.05f, 0.04f, MARFIL);
    }

    m.QuitarTransformacion();
}


// Banqueta de tres patas.
static void PiezaTaburete(
    ConstructorMalla& m,
    float x, float y, float z,
    float escala,
    float giro
)
{
    float h = 0.75f * escala;

    for (int k = 0; k < 3; k++)
    {
        float a = giro * DEG2RAD + k * 2.0944f;

        m.Cilindro({ x + std::cos(a) * 0.3f * escala, y, z + std::sin(a) * 0.3f * escala }, { x, y + h, z }, 0.04f, 0.04f, 4, MADERA_OSCURA);
    }

    m.Cilindro({ x, y + h, z }, { x, y + h + 0.08f, z }, 0.3f * escala, 0.3f * escala, 10, MADERA_ROJIZA);
}


// Aceitera de laton o frasco de vidrio.
static void PiezaBotella(
    ConstructorMalla& m,
    float x, float y, float z,
    float escala,
    float variacion,
    float giro
)
{
    if (variacion < 0.5f)
    {
        float a = giro * DEG2RAD;

        m.Cilindro({ x, y, z }, { x, y + 0.14f * escala, z }, 0.14f * escala, 0.1f * escala, 8, LATON);
        m.Cilindro({ x, y + 0.14f * escala, z }, { x + std::cos(a) * 0.25f * escala, y + 0.32f * escala, z + std::sin(a) * 0.25f * escala }, 0.03f * escala, 0.012f * escala, 4, LATON);
    }
    else
    {
        Color vidrio = variacion < 0.75f ? Color{ 70, 112, 82, 255 } : Color{ 120, 74, 46, 255 };

        m.Cilindro({ x, y, z }, { x, y + 0.3f * escala, z }, 0.09f * escala, 0.09f * escala, 8, vidrio);
        m.Cilindro({ x, y + 0.3f * escala, z }, { x, y + 0.42f * escala, z }, 0.09f * escala, 0.035f * escala, 8, vidrio);
        m.Cilindro({ x, y + 0.42f * escala, z }, { x, y + 0.47f * escala, z }, 0.04f * escala, 0.04f * escala, 6, MADERA_CLARA);
    }
}


// Rollo de cuerda (junto al campanario).
static void PiezaRolloCuerda(
    ConstructorMalla& m,
    float x, float y, float z,
    float escala
)
{
    const Color cuerda = Color{ 176, 148, 104, 255 };

    for (int k = 0; k < 3; k++)
    {
        float r = (0.42f - 0.08f * k) * escala;
        float yy = y + k * 0.07f * escala;

        m.Cilindro({ x, yy, z }, { x, yy + 0.07f * escala, z }, r, r, 12, Oscurecer(cuerda, 1.0f - 0.08f * k));
    }

    m.Cilindro({ x, y + 0.2f * escala, z }, { x, y + 0.22f * escala, z }, 0.14f * escala, 0.14f * escala, 8, Oscurecer(cuerda, 0.6f));
}


//==================================================
// DECORACION DISPERSA (colocada una vez, horneada por calidad)
//==================================================
//
// ~1000 piezas en ALTA, cerca del camino (a 2-13 del sendero) y en las zonas
// que se ven: costados de la sala, franja del fondo, frente, anillo del
// escenario y plataformas. Cada tipo tiene su regla de distancia al
// sendero, separacion y region preferida; lo alto queda fuera de la franja
// frontal de las casillas. Nada va sobre la cara ni el aro.

enum TipoPropReloj
{
    PROP_ENGRANAJE = 0,
    PROP_TUERCAS,
    PROP_RESORTE,
    PROP_VIRUTA,
    PROP_HERRAMIENTA,
    PROP_CAJON,
    PROP_CAJA_HERRAMIENTAS,
    PROP_RELOJ_MESA,
    PROP_LIBROS,
    PROP_SACO,
    PROP_LAMPARA,
    PROP_RELOJ_PIE,
    PROP_ENGRANAJE_PARADO,
    PROP_ALFOMBRA,
    PROP_TABURETE,
    PROP_BOTELLA,
    PROP_CUERDA
};

struct PropReloj
{
    TipoPropReloj tipo;
    float x;
    float y;
    float z;
    float escala;
    float variacion;
    float giro;
};

struct ReglaPropReloj
{
    TipoPropReloj tipo;
    int cantidad;
    float senderoMin;
    float senderoMax;
    float radio;
    float separacion;
    bool alto;
    float escalaMin;
    float escalaMax;

    // Region preferida (radio 0 = toda la sala).
    float regionX;
    float regionZ;
    float regionRadio;
};

static const ReglaPropReloj REGLAS_PROPS_RELOJ[] =
{
    { PROP_ALFOMBRA,           6, 4.4f, 14.0f, 2.6f, 7.0f, false, 0.9f, 1.15f,   0.0f,   0.0f,  0.0f },
    { PROP_RELOJ_PIE,         16, 3.0f, 13.0f, 0.6f, 3.2f, true,  0.9f, 1.15f,   0.0f,   0.0f,  0.0f },
    { PROP_LAMPARA,           18, 2.8f, 12.0f, 0.4f, 4.5f, true,  0.9f, 1.1f,    0.0f,   0.0f,  0.0f },
    { PROP_ENGRANAJE_PARADO,  40, 2.6f, 12.0f, 0.8f, 1.8f, true,  0.8f, 1.5f,    0.0f,   0.0f,  0.0f },
    { PROP_CAJON,             40, 2.6f, 13.0f, 0.7f, 1.5f, true,  0.8f, 1.2f,    0.0f,   0.0f,  0.0f },
    { PROP_CAJA_HERRAMIENTAS, 30, 2.3f, 12.0f, 0.5f, 1.3f, false, 0.8f, 1.2f,  -24.0f,   0.0f, 14.0f },
    { PROP_SACO,              30, 2.4f, 12.0f, 0.6f, 1.4f, false, 0.8f, 1.2f,    0.0f,   0.0f,  0.0f },
    { PROP_CUERDA,            16, 2.2f, 12.0f, 0.5f, 1.4f, false, 0.8f, 1.2f,   22.0f,  -6.0f, 12.0f },
    { PROP_TABURETE,          20, 2.4f, 12.0f, 0.4f, 1.6f, false, 0.9f, 1.1f,    0.0f,   0.0f,  0.0f },
    { PROP_RELOJ_MESA,        36, 2.2f, 12.0f, 0.4f, 0.9f, false, 0.8f, 1.3f,    0.0f,   0.0f,  0.0f },
    { PROP_LIBROS,            30, 2.2f, 12.0f, 0.4f, 0.9f, false, 0.8f, 1.2f,    0.0f,   0.0f,  0.0f },
    { PROP_ENGRANAJE,         50, 2.3f, 12.0f, 0.4f, 0.65f, false, 0.6f, 1.6f,   0.0f,   0.0f,  0.0f },
    { PROP_HERRAMIENTA,       35, 2.3f, 11.0f, 0.4f, 0.6f, false, 0.8f, 1.2f,  -22.0f,   0.0f, 16.0f },
    { PROP_BOTELLA,           25, 2.3f, 11.0f, 0.2f, 0.5f, false, 0.8f, 1.3f,    0.0f,   0.0f,  0.0f },
    { PROP_RESORTE,           15, 2.3f, 11.0f, 0.2f, 0.55f, false, 0.8f, 1.3f,   0.0f,   0.0f,  0.0f },
    { PROP_VIRUTA,            25, 2.3f, 10.0f, 0.3f, 0.6f, false, 0.8f, 1.3f,    0.0f,   0.0f,  0.0f },
    { PROP_TUERCAS,           40, 2.3f, 11.0f, 0.25f, 0.35f, false, 0.8f, 1.3f,  0.0f,   0.0f,  0.0f }
};

static const int MAX_PROPS_RELOJ = 1400;

static PropReloj propsReloj[MAX_PROPS_RELOJ];
static int cantidadPropsReloj = 0;
static bool propsColocadosReloj = false;

static unsigned int semillaPropsReloj = 13579u;

static MallaGpu mallaPropsReloj;
static int nivelPropsReloj = -1;


static float AleatorioReloj()
{
    semillaPropsReloj = semillaPropsReloj * 1664525u + 1013904223u;
    return (float)((semillaPropsReloj >> 8) & 0xFFFF) / 65535.0f;
}


static bool SeparadoDeSuTipoReloj(
    TipoPropReloj tipo,
    float x,
    float z,
    float separacion
)
{
    for (int i = 0; i < cantidadPropsReloj; i++)
    {
        const PropReloj& p = propsReloj[i];

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


// Cualquier pieza (no solo las de su tipo) a menos de `distancia`.
static bool LibreDeOtrasReloj(
    float x,
    float z,
    float radio
)
{
    for (int i = 0; i < cantidadPropsReloj; i++)
    {
        const PropReloj& p = propsReloj[i];

        if (p.tipo == PROP_ALFOMBRA)
        {
            continue;
        }

        float dx = p.x - x;
        float dz = p.z - z;
        float limite = radio + 0.35f;

        if (dx * dx + dz * dz < limite * limite)
        {
            return false;
        }
    }

    return true;
}


// La pieza cabe: dentro de la sala, fuera de la cara y del aro, sobre un
// suelo parejo y lejos de los muebles grandes.
static bool LugarValidoPropReloj(
    float x,
    float z,
    float radio
)
{
    if (
        std::fabs(x) > MURO_X - 1.4f - radio ||
        z < MURO_FONDO_Z + 1.0f + radio ||
        z > 26.0f
    )
    {
        return false;
    }

    float r = std::sqrt(x * x + z * z);

    if (r < RADIO_PLATAFORMA_INTERIOR + 0.25f + radio)
    {
        return false;
    }

    float h = AlturaSueloReloj(x, z);

    if (
        AlturaSueloReloj(x + radio, z) != h ||
        AlturaSueloReloj(x - radio, z) != h ||
        AlturaSueloReloj(x, z + radio) != h ||
        AlturaSueloReloj(x, z - radio) != h
    )
    {
        return false;
    }

    // Zocalo del campanario y bastidor de la maquina.
    if (
        std::fabs(x - CAMPANARIO_X) < 4.9f + radio &&
        std::fabs(z - CAMPANARIO_Z) < 4.9f + radio
    )
    {
        return false;
    }

    if (
        std::fabs(x - ENGRANAJES_X) < 4.6f + radio &&
        std::fabs(z - (ENGRANAJES_Z - 0.2f)) < 1.6f + radio
    )
    {
        return false;
    }

    return DespejadoReloj(x, z, radio);
}


static void ColocarPropsReloj()
{
    semillaPropsReloj = 13579u;
    cantidadPropsReloj = 0;

    for (const ReglaPropReloj& regla : REGLAS_PROPS_RELOJ)
    {
        for (int intento = 0, n = 0; intento < 40000 && n < regla.cantidad; intento++)
        {
            float x;
            float z;

            if (regla.regionRadio > 0.0f)
            {
                float ang = AleatorioReloj() * 2.0f * PI_RELOJ;
                float rad = std::sqrt(AleatorioReloj()) * regla.regionRadio;
                x = regla.regionX + std::cos(ang) * rad;
                z = regla.regionZ + std::sin(ang) * rad;
            }
            else
            {
                x = -MURO_X + AleatorioReloj() * 2.0f * MURO_X;
                z = MURO_FONDO_Z + AleatorioReloj() * (26.0f - MURO_FONDO_Z);
            }

            if (!LugarValidoPropReloj(x, z, regla.radio))
            {
                continue;
            }

            if (regla.alto && OcultaCasillaReloj(x, z))
            {
                continue;
            }

            float sendero = DistanciaSenderoReloj(x, z);

            if (sendero < regla.senderoMin || sendero > regla.senderoMax)
            {
                continue;
            }

            if (!SeparadoDeSuTipoReloj(regla.tipo, x, z, regla.separacion))
            {
                continue;
            }

            if (regla.tipo != PROP_ALFOMBRA && !LibreDeOtrasReloj(x, z, regla.radio))
            {
                continue;
            }

            if (cantidadPropsReloj >= MAX_PROPS_RELOJ)
            {
                break;
            }

            PropReloj& p = propsReloj[cantidadPropsReloj++];

            p.tipo = regla.tipo;
            p.x = x;
            p.y = AlturaSueloReloj(x, z);
            p.z = z;
            p.escala = regla.escalaMin + AleatorioReloj() * (regla.escalaMax - regla.escalaMin);
            p.variacion = AleatorioReloj();
            p.giro = AleatorioReloj() * 360.0f;

            // Lo que tiene esfera mira mas o menos a la camara.
            if (regla.tipo == PROP_RELOJ_PIE || regla.tipo == PROP_RELOJ_MESA)
            {
                p.giro = (AleatorioReloj() - 0.5f) * 50.0f;
            }

            n++;
        }
    }

    propsColocadosReloj = true;
}


// Calidad de decoracion: ALTA conserva todo; MEDIA recorta parte de lo
// menudo; BAJA deja solo lo cercano al sendero y poco de lo menudo.
static bool ConservarPropReloj(
    int indice,
    const PropReloj& p,
    NivelCalidadGrafica nivel
)
{
    if (nivel == CALIDAD_ALTA)
    {
        return true;
    }

    bool menudo =
        p.tipo == PROP_TUERCAS ||
        p.tipo == PROP_RESORTE ||
        p.tipo == PROP_VIRUTA ||
        p.tipo == PROP_HERRAMIENTA ||
        p.tipo == PROP_BOTELLA ||
        p.tipo == PROP_ENGRANAJE;

    float azar = Hash2(indice, 77);

    if (nivel == CALIDAD_MEDIA)
    {
        return !menudo || azar < FactorCalidadGrafica(nivel);
    }

    if (DistanciaSenderoReloj(p.x, p.z) > 9.0f)
    {
        return false;
    }

    return !menudo || azar < FactorCalidadGrafica(nivel) * 0.8f;
}


static void ConstruirMallaPropsReloj(NivelCalidadGrafica nivel)
{
    if (!propsColocadosReloj)
    {
        ColocarPropsReloj();
    }

    static ConstructorMalla m;
    m.Limpiar();
    m.reduccionDetalle = nivel == CALIDAD_ALTA ? 0 : 1;

    for (int i = 0; i < cantidadPropsReloj; i++)
    {
        const PropReloj& p = propsReloj[i];

        if (!ConservarPropReloj(i, p, nivel))
        {
            continue;
        }

        switch (p.tipo)
        {
            case PROP_ENGRANAJE:
                PiezaEngranajeSuelto(m, p.x, p.y, p.z, p.escala, p.variacion, p.giro);
                break;

            case PROP_TUERCAS:
                PiezaTuercas(m, p.x, p.y, p.z, p.escala, p.variacion);
                break;

            case PROP_RESORTE:
                PiezaResorte(m, p.x, p.y, p.z, p.escala, p.variacion);
                break;

            case PROP_VIRUTA:
                PiezaViruta(m, p.x, p.y, p.z, p.escala, p.variacion, p.giro);
                break;

            case PROP_HERRAMIENTA:
                PiezaHerramienta(m, p.x, p.y, p.z, p.escala, p.variacion, p.giro);
                break;

            case PROP_CAJON:
                PiezaCajon(m, p.x, p.y, p.z, p.escala, p.variacion, p.giro);
                break;

            case PROP_CAJA_HERRAMIENTAS:
                PiezaCajaHerramientas(m, p.x, p.y, p.z, p.escala, p.variacion, p.giro);
                break;

            case PROP_RELOJ_MESA:
                PiezaRelojMesa(m, p.x, p.y, p.z, p.escala, p.variacion, p.giro);
                break;

            case PROP_LIBROS:
                PiezaLibros(m, p.x, p.y, p.z, p.escala, p.variacion, p.giro);
                break;

            case PROP_SACO:
                PiezaSaco(m, p.x, p.y, p.z, p.escala, p.variacion);
                break;

            case PROP_LAMPARA:
                PiezaLampara(m, p.x, p.y, p.z, p.escala);
                break;

            case PROP_RELOJ_PIE:
                PiezaRelojPieChico(m, p.x, p.y, p.z, p.escala, p.variacion, p.giro);
                break;

            case PROP_ENGRANAJE_PARADO:
                PiezaEngranajeParado(m, p.x, p.y, p.z, p.escala, p.variacion, p.giro);
                break;

            case PROP_ALFOMBRA:
                PiezaAlfombra(m, p.x, p.y, p.z, p.escala, p.variacion, p.giro);
                break;

            case PROP_TABURETE:
                PiezaTaburete(m, p.x, p.y, p.z, p.escala, p.giro);
                break;

            case PROP_BOTELLA:
                PiezaBotella(m, p.x, p.y, p.z, p.escala, p.variacion, p.giro);
                break;

            case PROP_CUERDA:
                PiezaRolloCuerda(m, p.x, p.y, p.z, p.escala);
                break;
        }
    }

    IluminarMalla(m, 0);

    ActualizarMallaGpu(mallaPropsReloj, m);
    nivelPropsReloj = (int)nivel;
}


//==================================================
// ANIMACIONES (modo inmediato y mallas transformadas)
//==================================================
//
// Aro y corona giran con el giro deducido del tablero; engranajes,
// pendulos, agujas de los relojes y la campana se mueven con el tiempo.
// Lo translucido (halos, haces de luna, polvo y resplandores) va al final.

// Engranajes: rotacion propia mas el acople con el aro de la esfera.
static void DibujarEngranajesReloj(
    float tiempo,
    float anguloEsfera
)
{
    NivelCalidadGrafica nivel = CalidadDecoracion();

    for (const EngranajeReloj& e : ENGRANAJES_RELOJ)
    {
        if ((int)nivel < (int)e.nivelMinimo)
        {
            continue;
        }

        float angulo = tiempo * e.velocidad + anguloEsfera * e.acople;
        float grosor = 0.6f + 0.4f * e.radio;

        rlPushMatrix();
        rlTranslatef(e.centro.x, e.centro.y, e.centro.z);

        if (e.eje == 1)
        {
            rlRotatef(90.0f, 1.0f, 0.0f, 0.0f);
        }
        else if (e.eje == 2)
        {
            rlRotatef(90.0f, 0.0f, 0.0f, 1.0f);
        }

        rlRotatef(angulo, 0.0f, 1.0f, 0.0f);
        rlScalef(e.radio, grosor, e.radio);

        DibujarMallaGpu(mallaEngranajeReloj);

        rlPopMatrix();
    }
}


static void DibujarPenduloRelojPie(float tiempo)
{
    float angulo = 6.0f * std::sin(tiempo * 2.0f * PI_RELOJ / 2.4f);

    rlPushMatrix();
    rlTranslatef(0.0f, SUELO_SALA_Y + PENDULO_PIVOTE_Y, MURO_FONDO_Z + 1.5f);
    rlRotatef(angulo, 0.0f, 0.0f, 1.0f);
    DibujarMallaGpu(mallaPenduloReloj);
    rlPopMatrix();
}


// La campana se mece sola, mas fuerte mientras giran los anillos y a todo
// vuelo cuando alguien cae en LA CAMPANADA.
static float AnguloCampanaReloj(const PartidaTablero& partida)
{
    float amplitud = 3.0f;

    if (partida.gimmickActivo)
    {
        amplitud = 10.0f;
    }

    int participante = ObtenerParticipanteTurnoTableroFinal(partida);

    if (
        participante >= 0 &&
        partida.fase == FASE_PARTIDA_TABLERO_EVENTO_CASILLA
    )
    {
        const Casilla* casilla =
            partida.tablero.ObtenerCasilla(
                partida.jugadores[participante].casillaActual
            );

        if (casilla != nullptr && casilla->tipo == CASILLA_EVENTO)
        {
            amplitud = 24.0f;
        }
    }

    return amplitud * std::sin(partida.tiempoTablero * 1.9f);
}


static void DibujarCampanaGigante(const PartidaTablero& partida)
{
    rlPushMatrix();
    rlTranslatef(CAMPANARIO_X, CAMPANA_PIVOTE_Y, CAMPANARIO_Z);
    rlRotatef(AnguloCampanaReloj(partida), 0.0f, 0.0f, 1.0f);
    DibujarMallaGpu(mallaCampanaReloj);
    rlPopMatrix();
}


// Marco local de una pared: +z local = normal hacia la sala.
static void EntrarMarcoPared(
    MuroReloj muro,
    Vector3 centro
)
{
    rlPushMatrix();
    rlTranslatef(centro.x, centro.y, centro.z);

    if (muro == MURO_OESTE)
    {
        rlRotatef(90.0f, 0.0f, 1.0f, 0.0f);
    }
    else if (muro == MURO_ESTE)
    {
        rlRotatef(-90.0f, 0.0f, 1.0f, 0.0f);
    }
}


// Aguja que gira en sentido horario visto de frente (grados desde las 12).
static void AgujaReloj(
    float grados,
    float largo,
    float ancho,
    Color color
)
{
    rlPushMatrix();
    rlRotatef(-grados, 0.0f, 0.0f, 1.0f);
    DrawCube({ 0.0f, largo * 0.4f, 0.0f }, ancho, largo, 0.03f, color);
    rlPopMatrix();
}


// Agujas de los relojes de pared y del reloj de pie; pendulos de los
// reguladores. El minutero da la vuelta en un minuto.
static void DibujarRelojesAnimados(float tiempo)
{
    int k = 0;

    for (const RelojParedReloj& r : RELOJES_PARED)
    {
        MarcoPared p = MarcoMuro(r.muro);
        float fase = 360.0f * Hash2(k++, 5);
        float minutos = fase + tiempo * 6.0f;

        EntrarMarcoPared(r.muro, PuntoPared(p, r.u, r.b, 0.6f));
        AgujaReloj(minutos / 12.0f + fase, r.radio * 0.55f, 0.07f, TINTA);
        AgujaReloj(minutos, r.radio * 0.85f, 0.045f, TINTA);
        rlPopMatrix();

        if (r.pendulo)
        {
            float vaiven = 8.0f * std::sin(tiempo * 2.0f * PI_RELOJ / 1.6f + fase);

            EntrarMarcoPared(r.muro, PuntoPared(p, r.u, r.b - 1.75f, 0.62f));
            rlRotatef(vaiven, 0.0f, 0.0f, 1.0f);
            DrawCube({ 0.0f, -1.0f, 0.0f }, 0.05f, 2.0f, 0.03f, LATON);
            DrawCube({ 0.0f, -2.1f, 0.0f }, 0.4f, 0.4f, 0.06f, ORO_RELOJ);
            rlPopMatrix();
        }
    }

    MarcoPared fondo = MarcoMuro(MURO_FONDO);
    float minutos = tiempo * 6.0f + 40.0f;

    EntrarMarcoPared(MURO_FONDO, PuntoPared(fondo, 0.0f, RELOJ_PIE_ESFERA_Y, RELOJ_PIE_FONDO + 0.14f));
    AgujaReloj(minutos / 12.0f + 300.0f, 1.0f, 0.12f, TINTA);
    AgujaReloj(minutos, 1.45f, 0.08f, TINTA);
    DrawCylinderEx({ 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.04f }, 0.1f, 0.1f, 8, LATON);
    rlPopMatrix();
}


// Halos dorados en las casillas que se mueven mientras giran los anillos.
static void DibujarHalosGiroReloj(const PartidaTablero& partida)
{
    float pulso = Parpadeo(partida.tiempoTablero, 9.0f);

    Color halo =
    {
        255,
        (unsigned char)(196 + (int)(pulso * 40.0f)),
        96,
        (unsigned char)(90 + (int)(pulso * 90.0f))
    };

    for (int a = ANILLO_ESFERA; a <= ANILLO_CORONA; a++)
    {
        float pendiente =
            a == ANILLO_ESFERA
            ? partida.gimmickAnim
            : pendienteCoronaReloj;

        if (pendiente > -0.001f)
        {
            continue;
        }

        int lista[CASILLAS_ESFERA];
        char letras[CASILLAS_ESFERA];
        int cantidad = ListarGiratoriasReloj((AnilloReloj)a, lista, letras);

        for (int j = 0; j < cantidad; j++)
        {
            const Casilla* casilla = partida.tablero.ObtenerCasilla(lista[j]);

            if (casilla == nullptr)
            {
                continue;
            }

            DrawCylinder(
                { casilla->posicion.x, SENDERO_Y + LIFT_DETALLE + 0.03f, casilla->posicion.z },
                1.12f, 1.12f, 0.02f, 20,
                halo
            );
        }
    }
}


// Haces de luna que entran por los ventanales del fondo, con polvo que
// flota adentro.
static void DibujarHacesLunaReloj(float tiempo)
{
    if (CalidadEfectos() == CALIDAD_BAJA)
    {
        return;
    }

    // Direccion de la luz (desde la luna, detras del muro del fondo).
    const float lx = 0.10f;
    const float ly = -0.85f;
    const float lz = 0.52f;

    const Color haz = { 176, 196, 255, 20 };
    const Color mancha = { 186, 204, 255, 30 };
    const Color mota = { 236, 238, 255, 150 };

    MarcoPared p = MarcoMuro(MURO_FONDO);

    int motas = (int)std::ceil(16.0f * FactorCalidadGrafica(CalidadParticulas()));

    for (int v = 0; v < CANTIDAD_VENTANAS_FONDO; v++)
    {
        float wc = VENTANAS_FONDO[v];
        float r = VENTANA_RADIO - 0.1f;

        Vector3 arribaI = PuntoPared(p, wc - r, VENTANA_B1 + 1.2f, -0.5f);
        Vector3 arribaD = PuntoPared(p, wc + r, VENTANA_B1 + 1.2f, -0.5f);
        Vector3 abajoI = PuntoPared(p, wc - r, VENTANA_B0, -0.5f);
        Vector3 abajoD = PuntoPared(p, wc + r, VENTANA_B0, -0.5f);

        auto proyectar = [&](Vector3 a)
        {
            float t = (SUELO_SALA_Y + 0.02f - a.y) / ly;
            return Vector3{ a.x + lx * t, SUELO_SALA_Y + 0.02f, a.z + lz * t };
        };

        Vector3 lejosI = proyectar(abajoI);
        Vector3 lejosD = proyectar(abajoD);
        Vector3 cercaI = proyectar(arribaI);
        Vector3 cercaD = proyectar(arribaD);

        DrawTriangle3D(arribaI, cercaI, cercaD, haz);
        DrawTriangle3D(arribaI, cercaD, arribaD, haz);
        DrawTriangle3D(abajoI, lejosI, lejosD, haz);
        DrawTriangle3D(abajoI, lejosD, abajoD, haz);
        DrawTriangle3D(lejosI, cercaI, cercaD, mancha);
        DrawTriangle3D(lejosI, cercaD, lejosD, mancha);

        for (int i = 0; i < motas; i++)
        {
            float u = std::fmod(i * 0.381f + v * 0.23f + tiempo * 0.03f, 1.0f);
            float lado = std::fmod(i * 0.618f + v * 0.11f, 1.0f);
            float alto = std::fmod(i * 0.271f + v * 0.37f, 1.0f);

            Vector3 origen = PuntoPared(p, wc - r + 2.0f * r * lado, VENTANA_B0 + (VENTANA_B1 + 1.2f - VENTANA_B0) * alto, -0.5f);
            float t = (origen.y - SUELO_SALA_Y) / -ly * u;

            Vector3 pos =
            {
                origen.x + lx * t + 0.2f * std::sin(tiempo * 0.7f + i),
                origen.y + ly * t + 0.25f * std::sin(tiempo * 0.5f + i * 2.1f),
                origen.z + lz * t
            };

            DrawCube(pos, 0.05f, 0.05f, 0.05f, ConAlfa(mota, 0.6f * std::sin(PI_RELOJ * u)));
        }
    }
}


static void Resplandor(
    Vector3 centro,
    float radio,
    float alfa,
    Color color
)
{
    DrawSphereEx(centro, radio, 5, 8, ConAlfa(color, alfa));
}


// Llamas de los apliques y luz de lamparas y velas (parpadean).
static void DibujarResplandoresReloj(float tiempo)
{
    if (CalidadEfectos() == CALIDAD_BAJA)
    {
        return;
    }

    const Color calido = { 255, 196, 110, 255 };

    for (int muro = MURO_FONDO; muro < CANTIDAD_MUROS_RELOJ; muro++)
    {
        MarcoPared p = MarcoMuro((MuroReloj)muro);
        const float* pilastras = nullptr;
        int cantidad = PilastrasMuro((MuroReloj)muro, pilastras);

        for (int i = 0; i < cantidad; i++)
        {
            if (!PilastraConAplique((MuroReloj)muro, pilastras[i]))
            {
                continue;
            }

            float vela = Parpadeo(tiempo + i * 1.7f + muro, 11.0f) * Parpadeo(tiempo * 0.7f + i, 5.0f);
            Vector3 llama = PuntoPared(p, pilastras[i], ALTURA_APLIQUE - 0.05f, 1.05f);

            Resplandor(llama, 0.42f + 0.06f * vela, 0.16f + 0.08f * vela, calido);
            Resplandor(llama, 0.16f, 0.5f + 0.3f * vela, Color{ 255, 236, 170, 255 });
        }
    }

    for (int i = 0; i < cantidadPropsReloj; i++)
    {
        const PropReloj& p = propsReloj[i];

        if (p.tipo != PROP_LAMPARA)
        {
            continue;
        }

        float vela = Parpadeo(tiempo + i, 7.0f);
        Vector3 globo = { p.x, p.y + 2.3f * p.escala + 0.25f, p.z };

        Resplandor(globo, 0.42f + 0.04f * vela, 0.08f + 0.04f * vela, calido);
    }

    Resplandor({ -26.8f, 1.72f, 11.55f }, 0.45f, 0.15f, calido);
    Resplandor({ -30.8f, 1.45f, 4.9f }, 0.5f, 0.16f + 0.05f * Parpadeo(tiempo, 9.0f), calido);
    Resplandor({ 0.7f, 1.77f, 27.4f }, 0.35f, 0.18f, calido);
}


//==================================================
// RECURSOS
//==================================================

static void PrepararMallasReloj()
{
    NivelCalidadGrafica nivel = CalidadDecoracion();

    if (!mallaTelonReloj.cargada || nivelTelonReloj != (int)nivel)
    {
        ConstruirMallaTelonReloj(nivel);
    }

    if (!mallaSalaReloj.cargada || nivelSalaReloj != (int)nivel)
    {
        ConstruirMallaSalaReloj(nivel);
    }

    if (!mallaEsferaReloj.cargada || nivelEsferaReloj != (int)nivel)
    {
        ConstruirMallaEsferaReloj(nivel);
    }

    if (!mallaSenderosReloj.cargada || nivelSenderosReloj != (int)nivel)
    {
        ConstruirMallaSenderosReloj(nivel);
    }

    if (!mallaPropsReloj.cargada || nivelPropsReloj != (int)nivel)
    {
        ConstruirMallaPropsReloj(nivel);
    }

    if (!mallaLandmarksReloj.cargada || nivelLandmarksReloj != (int)nivel)
    {
        ConstruirMallaLandmarksReloj(nivel);
    }

    PrepararPiezasMovilesReloj();
}


static void DescargarRecursosReloj()
{
    DescargarMalla(mallaTelonReloj);
    DescargarMalla(mallaSalaReloj);
    DescargarMalla(mallaEsferaReloj);
    DescargarMalla(mallaSenderosReloj);
    DescargarMalla(mallaPropsReloj);
    DescargarMalla(mallaLandmarksReloj);
    DescargarMalla(mallaAroReloj);
    DescargarMalla(mallaCoronaReloj);
    DescargarMalla(mallaEngranajeReloj);
    DescargarMalla(mallaPenduloReloj);
    DescargarMalla(mallaCampanaReloj);

    nivelTelonReloj = -1;
    nivelSalaReloj = -1;
    nivelEsferaReloj = -1;
    nivelSenderosReloj = -1;
    nivelPropsReloj = -1;
    nivelLandmarksReloj = -1;
    nivelPiezasMovilesReloj = -1;
}


//==================================================
// DECORACION
//==================================================

static void DibujarDecoracionTorreReloj(
    const PartidaTablero& partida
)
{
    // MODELO FUTURO: la sala de la torre y la esfera como modelo 3D.
    PrepararMallasReloj();

    float tiempo = partida.tiempoTablero;
    float anguloEsfera = AnguloVisualAnilloReloj(partida, ANILLO_ESFERA);
    float anguloCorona = AnguloVisualAnilloReloj(partida, ANILLO_CORONA);

    // Estatico (mallas horneadas).
    DibujarMallaGpu(mallaTelonReloj);
    DibujarMallaGpu(mallaSalaReloj);
    DibujarMallaGpu(mallaEsferaReloj);
    DibujarMallaGpu(mallaPropsReloj);
    DibujarMallaGpu(mallaLandmarksReloj);

    // Aro (horario) y corona (antihorario) giran bajo los senderos.
    rlPushMatrix();
    rlRotatef(-anguloEsfera, 0.0f, 1.0f, 0.0f);
    DibujarMallaGpu(mallaAroReloj);
    rlPopMatrix();

    rlPushMatrix();
    rlRotatef(anguloCorona, 0.0f, 1.0f, 0.0f);
    DibujarMallaGpu(mallaCoronaReloj);
    rlPopMatrix();

    DibujarMallaGpu(mallaSenderosReloj);

    // Animado.
    DibujarEngranajesReloj(tiempo, anguloEsfera);
    DibujarPenduloRelojPie(tiempo);
    DibujarCampanaGigante(partida);
    DibujarRelojesAnimados(tiempo);
}


// Luz calida de velas abajo, noche azul arriba y motas doradas que flotan.
static void DibujarCapaPantallaTorreReloj(
    const PartidaTablero& partida
)
{
    int ancho = GetScreenWidth();
    int alto = GetScreenHeight();
    float t = partida.tiempoTablero;
    float escala = alto / 720.0f;

    DrawRectangleGradientV(
        0, 0, ancho, alto * 2 / 5,
        Color{ 16, 20, 52, 80 },
        Color{ 16, 20, 52, 0 }
    );

    DrawRectangleGradientV(
        0, alto / 2, ancho, alto - alto / 2,
        Color{ 255, 168, 80, 0 },
        Color{ 255, 168, 80, 42 }
    );

    DrawRectangleGradientH(0, 0, ancho / 6, alto, Color{ 10, 8, 20, 70 }, Color{ 10, 8, 20, 0 });
    DrawRectangleGradientH(ancho - ancho / 6, 0, ancho / 6, alto, Color{ 10, 8, 20, 0 }, Color{ 10, 8, 20, 70 });

    int motas = (int)std::ceil(36.0f * FactorCalidadGrafica(CalidadParticulas()));

    for (int i = 0; i < motas; i++)
    {
        float vx = 6.0f + 10.0f * Hash2(i, 3);
        float vy = 4.0f + 8.0f * Hash2(i, 5);

        float x = std::fmod(Hash2(i, 1) * ancho + t * vx, (float)ancho + 20.0f) - 10.0f;
        float y = std::fmod(Hash2(i, 2) * alto + t * vy, (float)alto + 20.0f) - 10.0f;

        x += 10.0f * std::sin(t * 0.6f + i);

        float brillo = 0.25f + 0.35f * Parpadeo(t + i * 0.7f, 1.3f);
        float lado = (1.5f + 2.0f * Hash2(i, 4)) * escala;

        DrawRectangle(
            (int)x, (int)y, (int)lado, (int)lado,
            ConAlfa(Color{ 255, 220, 150, 255 }, brillo)
        );
    }
}


//==================================================
// GIMMICK (dibujo)
//==================================================

static void DibujarGimmickTorreReloj(
    const PartidaTablero& partida
)
{
    float tiempo = partida.tiempoTablero;

    rlDrawRenderBatchActive();
    rlDisableDepthMask();
    rlDisableBackfaceCulling();

    DibujarHalosGiroReloj(partida);
    DibujarHacesLunaReloj(tiempo);
    DibujarResplandoresReloj(tiempo);

    rlDrawRenderBatchActive();
    rlEnableBackfaceCulling();
    rlEnableDepthMask();
}


//==================================================
// GIMMICK (logica)
//==================================================

static void ActualizarGimmickTorreReloj(
    PartidaTablero& partida,
    float deltaTime
)
{
    // gimmickAnim = pasos de la esfera que faltan animar (<= 0).
    if (partida.gimmickAnim < 0.0f)
    {
        partida.gimmickAnim += VELOCIDAD_GIRO * deltaTime;

        if (partida.gimmickAnim > 0.0f)
        {
            partida.gimmickAnim = 0.0f;
        }
    }

    if (pendienteCoronaReloj < 0.0f)
    {
        pendienteCoronaReloj += VELOCIDAD_GIRO * deltaTime;

        if (pendienteCoronaReloj > 0.0f)
        {
            pendienteCoronaReloj = 0.0f;
        }
    }

    if (partida.gimmickAnim >= 0.0f && pendienteCoronaReloj >= 0.0f)
    {
        partida.gimmickAnim = 0.0f;
        partida.gimmickActivo = false;
    }
}


static void AlIniciarRondaTorreReloj(
    PartidaTablero& partida
)
{
    if (partida.rondaActual < 2)
    {
        partida.gimmickActivo = false;
        partida.gimmickAnim = 0.0f;
        pendienteCoronaReloj = 0.0f;

        std::snprintf(
            partida.estadoGimmick,
            sizeof(partida.estadoGimmick),
            "LA ESFERA Y LA CORONA GIRAN CADA RONDA (DESDE LA 2)"
        );

        EstablecerMensajeGimmick(
            partida,
            "LAS CASILLAS DE COLOR GIRAN CADA RONDA"
        );

        return;
    }

    int pasosEsfera = GetRandomValue(MIN_PASOS_GIRO, MAX_PASOS_GIRO);
    int pasosCorona = GetRandomValue(MIN_PASOS_CORONA, MAX_PASOS_CORONA);

    AplicarGiroReloj(partida, pasosEsfera, pasosCorona);

    std::snprintf(
        partida.estadoGimmick,
        sizeof(partida.estadoGimmick),
        "LA ESFERA Y LA CORONA GIRAN CADA RONDA (ÚLTIMO GIRO: %d Y %d)",
        pasosEsfera,
        pasosCorona
    );

    char mensaje[96];

    std::snprintf(
        mensaje,
        sizeof(mensaje),
        "¡LA ESFERA GIRA %d Y LA CORONA %d!",
        pasosEsfera,
        pasosCorona
    );

    EstablecerMensajeGimmick(partida, mensaje);
}


//==================================================
// EVENTO PROPIO: LA CAMPANADA
//==================================================

// Casilla anterior por el grafo: prefiere un predecesor de la misma zona
// (la esfera vuelve por la esfera, el lobulo por el lobulo).
static int PredecesorReloj(
    const Tablero& tablero,
    int casilla
)
{
    const Casilla* actual = tablero.ObtenerCasilla(casilla);

    if (actual == nullptr)
    {
        return -1;
    }

    int cualquiera = -1;

    for (int i = 0; i < tablero.cantidadCasillas; i++)
    {
        const Casilla& otra = tablero.casillas[i];

        for (int c = 0; c < otra.cantidadConexiones; c++)
        {
            if (otra.conexiones[c].destino != casilla)
            {
                continue;
            }

            if (otra.zona == actual->zona)
            {
                return i;
            }

            if (cualquiera < 0)
            {
                cualquiera = i;
            }
        }
    }

    return cualquiera;
}


// Casilla a la que vuelve MARCHA ATRAS: 3 pasos hacia atras. Nunca deja al
// jugador sobre el trofeo (retrocede uno mas).
static int CasillaMarchaAtras(
    const PartidaTablero& partida,
    int casilla
)
{
    int destino = casilla;

    for (int paso = 0; paso < PASOS_MARCHA_ATRAS; paso++)
    {
        int anterior = PredecesorReloj(partida.tablero, destino);

        if (anterior < 0)
        {
            break;
        }

        destino = anterior;
    }

    for (int extra = 0; extra < 2 && destino == partida.casillaTrofeo; extra++)
    {
        int anterior = PredecesorReloj(partida.tablero, destino);

        if (anterior < 0)
        {
            break;
        }

        destino = anterior;
    }

    return destino;
}


static bool ResolverCasillaTorreReloj(
    PartidaTablero& partida,
    int participante
)
{
    EstadoJugadorPartidaTablero& jugador =
        partida.jugadores[participante];

    const Casilla* casilla =
        partida.tablero.ObtenerCasilla(jugador.casillaActual);

    if (casilla == nullptr || casilla->tipo != CASILLA_EVENTO)
    {
        return false;
    }

    ReproducirSonidoTablero(partida, SONIDO_EVENTO_TABLERO);

    if (GetRandomValue(0, 1) == 0)
    {
        // a) CUERDA AL RELOJ: la esfera da un paso ya mismo.
        AplicarGiroReloj(partida, 1, 0);

        jugador.monedas += PREMIO_CUERDA;
        partida.ultimoEventoEspecial = EVENTO_TABLERO_BONIFICACION;
        partida.variacionMonedasEvento = PREMIO_CUERDA;

        std::snprintf(
            partida.textoEvento,
            sizeof(partida.textoEvento),
            "CAMPANADA: ¡CUERDA AL RELOJ! LA ESFERA GIRA 1 PASO Y +%d MONEDAS",
            PREMIO_CUERDA
        );
    }
    else
    {
        // b) MARCHA ATRAS: retrocede por donde vino.
        TeletransportarJugadorTablero(
            partida,
            participante,
            CasillaMarchaAtras(partida, jugador.casillaActual)
        );

        std::snprintf(
            partida.textoEvento,
            sizeof(partida.textoEvento),
            "CAMPANADA: ¡MARCHA ATRÁS! J%d RETROCEDE %d CASILLAS",
            participante + 1,
            PASOS_MARCHA_ATRAS
        );
    }

    return true;
}


//==================================================
// BOTS Y TEXTOS DE RUTA
//==================================================

// Valor aproximado de pisar una casilla con su tipo ACTUAL (tras el giro).
static int ValorCasillaBotReloj(
    const Casilla& casilla
)
{
    // Sin "default": un tipo nuevo de casilla obliga a puntuarlo aqui.
    switch (casilla.tipo)
    {
        case CASILLA_POSITIVA: return MONEDAS_CASILLA_POSITIVA;
        case CASILLA_NEGATIVA: return -MONEDAS_CASILLA_NEGATIVA;
        case CASILLA_ESPECIAL: return 1;
        case CASILLA_REGALO: return MONEDAS_REGALO;
        case CASILLA_CAOS: return -4;           // ruleta de castigos
        case CASILLA_INTERCAMBIO: return -1;    // azar: algo arriesgada
        case CASILLA_EVENTO: return 1;          // +3 monedas o marcha atras
        case CASILLA_DUELO: return 0;           // apuesta voluntaria
        case CASILLA_TIENDA: return 1;          // objetos: algo util
        case CASILLA_NEUTRA: return 0;
        case CANTIDAD_TIPOS_CASILLA: return 0;
    }

    return 0;
}


// Valor medio de los proximos pasos desde una casilla (todas las ramas).
static float ValorRutaBotReloj(
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
            ValorRutaBotReloj(
                partida, actual->conexiones[c].destino, profundidad - 1
            );
    }

    if (actual->cantidadConexiones > 0)
    {
        resto /= (float)actual->cantidadConexiones;
    }

    return (float)ValorCasillaBotReloj(*actual) + resto;
}


// Pasos minimos desde una casilla hasta otra (-1 si no hay camino).
static int PasosHastaReloj(
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
// de casillas mas ricas segun como quedaron los anillos. Siempre deja algo
// de azar.
static int DecidirRutaBotReloj(
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
                PasosHastaReloj(
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
            ValorRutaBotReloj(
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


static const char* DescribirRutaTorreReloj(
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

    return descripcionRutaReloj[casilla][opcion];
}


//==================================================
// DEFINICION
//==================================================

static const Color LATON_TEMA = { 202, 158, 80, 255 };
static const Color BRONCE_TEMA = { 86, 56, 32, 255 };

const DefinicionTablero& ObtenerDefinicionTableroTorreReloj()
{
    static DefinicionTablero definicion;
    static bool preparada = false;

    if (!preparada)
    {
        definicion.id = TABLERO_TORRE_RELOJ;
        definicion.nombre = "TORRE DEL RELOJ";

        definicion.historia =
            "En lo alto de la torre late el reloj más antiguo de la ciudad. "
            "El relojero premia con un trofeo de bronce a quien le pague la cuerda.";

        definicion.objetivo =
            "Reuní más trofeos (20 monedas cada uno) en 5 rondas.";

        definicion.gimmick =
            "La esfera gira: cada ronda sus casillas de color avanzan 1 a 3 pasos "
            "y las de la corona retroceden 1 a 2.";

        definicion.dificultad = "DIFICIL";

        definicion.colorTema = Color{ 208, 160, 72, 255 };
        definicion.colorFondo = Color{ 22, 28, 52, 255 };

        // Camara diorama: objetivo en el centro de la caja de casillas
        // (42 x 26 de mundo), desfase (0, 17.5, 16). El mapa es ancho: la
        // camara sigue al jugador y da una vista general cada ronda
        // (zoom = diagonal 49.5 / |desfase| 23.7).
        definicion.camaraObjetivo = { 0.0f, 0.0f, 0.0f };
        definicion.camaraPosicion = { 0.0f, 17.5f, 16.0f };
        definicion.camaraFovy = 48.0f;
        definicion.camaraSeguimiento = 1.0f;
        definicion.camaraZoomGeneral = 2.1f;
        definicion.camaraEscalaVistaPrevia = 2.1f;

        // Landmark: el centro de la esfera (enfoque cuando giran los anillos).
        definicion.camaraLandmark = { 0.0f, 0.5f, 0.0f };
        definicion.tieneLandmark = true;

        // Losas de bronce con remaches de laton.
        EstiloCasilla estilo;
        estilo.piedraLado = Color{ 118, 82, 48, 255 };
        estilo.piedraTapa = Color{ 170, 130, 76, 255 };
        estilo.borde = Color{ 64, 44, 30, 255 };
        estilo.detalle = Color{ 214, 178, 96, 255 };
        estilo.sendero = LATON_TEMA;
        estilo.senderoBorde = BRONCE_TEMA;
        estilo.anchoSendero = 0.95f;
        definicion.estiloCasilla = estilo;

        definicion.nombresZonas = NOMBRES_ZONAS_RELOJ;
        definicion.cantidadZonas = CANTIDAD_ZONAS_RELOJ;

        // Candidatas del trofeo: las llena la construccion de la ruta.
        TableroReferenciaReloj();

        definicion.casillasTrofeo = casillasTrofeoReloj;
        definicion.cantidadCasillasTrofeo = cantidadTrofeosReloj;

        definicion.costoTrofeo = 20;
        definicion.cantidadRondas = 5;

        definicion.Construir = ConstruirTorreReloj;
        definicion.DibujarDecoracion = DibujarDecoracionTorreReloj;
        definicion.AlIniciarRonda = AlIniciarRondaTorreReloj;
        definicion.ResolverCasilla = ResolverCasillaTorreReloj;
        definicion.ActualizarGimmick = ActualizarGimmickTorreReloj;
        definicion.DibujarGimmick = DibujarGimmickTorreReloj;
        definicion.DescargarRecursos = DescargarRecursosReloj;
        definicion.DibujarCapaPantalla = DibujarCapaPantallaTorreReloj;
        definicion.DecidirRutaBot = DecidirRutaBotReloj;
        definicion.DescribirOpcionRuta = DescribirRutaTorreReloj;

        preparada = true;
    }

    return definicion;
}
