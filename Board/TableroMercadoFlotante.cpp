#include "Board/CatalogoTableros.h"

#include "Gameplay/PartidaTablero.h"

#include "Board/MallaTablero.h"
#include "Systems/CalidadGrafica.h"

#include "rlgl.h"

#include <cmath>
#include <cstdio>


//==================================================
// MERCADO FLOTANTE (tablero 3)
//==================================================
//
// Un puerto mercante sobre canales al atardecer (60 casillas, 7 zonas).
// Un anillo de 26 casillas de MUELLE rodea el canal; en el centro, la isla
// de la PLAZA DEL MERCADO (circuito de 8 casillas) con la torre de la
// campana. Dos lazos largos salen del anillo: la ISLA DEL FARO (noreste) y
// el ASTILLERO (suroeste). Seis bifurcaciones (opcion 0 = lo largo):
//
//   - Embarcadero: seguir el muelle o cruzar el PUENTE SUR a la plaza.
//   - Muelle de las especias: seguir el muelle o la GONDOLA ESTE a la plaza.
//   - Muelle de las especias (norte): rodear la ISLA DEL FARO (12 pasos)
//     o seguir el muelle (3).
//   - Plaza norte: seguir en la plaza o el PUENTE NORTE al muelle norte.
//   - Muelle de los pescadores: ir al ASTILLERO (12 pasos) o seguir el
//     muelle (4); enseguida, seguir el muelle o la GONDOLA OESTE a la plaza.
//
// Ninguna bifurcacion bloquea: el muelle es un circuito, los lazos vuelven
// a el y la plaza siempre desemboca en el muelle norte. Los indices de las
// casillas clave se guardan al construir en nodoMercado.
//
// MECANICA PRINCIPAL: PUESTOS. Cada casilla azul (CASILLA_NEUTRA) es un
// puesto de mercado:
//   - libre: quien cae lo abre si tiene COSTO_ABRIR_PUESTO monedas;
//   - de un rival: se le paga ALQUILER_PUESTO (el Escudo lo evita);
//   - propio: el dueno cobra GANANCIA_PUESTO_PROPIO.
// El dueno de cada puesto vive en duenoPuesto[] (se reinicia en Construir).
//
// EVENTO propio (LA CAMPANA DEL MERCADO), al azar entre dos:
//   - DIA DE FERIA: cada dueno cobra 1 moneda por puesto (maximo 5).
//   - INSPECCION: quien mas puestos tiene pierde uno al azar.
//
// PRESENTACION (solo primitivas): mar al atardecer, ciudad de manzanas de
// piedra con casas escalonadas, muelles de tablones con pilotes, puentes
// con barandas, gondolas, la torre de la campana, el faro, el astillero y
// un puesto con toldo junto a cada casilla azul (gris libre, color del
// dueno si esta ocupado). Lo estatico se hornea en seis MallaGpu que se
// reconstruyen solo al cambiar la calidad; los toldos solo recolorean.
// MODELO FUTURO: muelles, casas y puestos como modelos GLB.

static const int COSTO_ABRIR_PUESTO = 3;
static const int ALQUILER_PUESTO = 3;
static const int GANANCIA_PUESTO_PROPIO = 2;

// Dia de feria: tope de lo que cobra cada dueno y propina si nadie
// tiene puestos todavia.
static const int MAXIMO_COBRO_FERIA = 5;
static const int PROPINA_FERIA = 2;

static const float ALTURA_CASILLA_MERCADO = 0.25f;
static const float NIVEL_AGUA_MERCADO = -0.24f;

// Distancia del puesto al centro de su casilla (fuera del sendero de 2.7;
// en los cruces el sendero se abre en una plaza).
static const float DISTANCIA_PUESTO = 2.2f;
static const float DISTANCIA_PUESTO_CRUCE = 2.5f;

// La campana suena unos segundos al empezar la ronda y en su evento.
static const float SEGUNDOS_CAMPANA = 3.2f;
static const float VELOCIDAD_ANIMACION_CAMPANA = 2.5f;

// Anillo de muelles: rectangulo de casillas a paso 3.4 (ninguna en x = 0,
// donde la torre tapa lo que queda detras).
static const int CASILLAS_MUELLE = 26;
static const int CASILLAS_PLAZA = 8;
static const int CASILLAS_LAZO = 11;

static const float ANILLO_X = 11.9f;
static const float ANILLO_Z_NORTE = -12.2f;
static const float ANILLO_Z_SUR = 8.2f;
static const float PASO_ANILLO = 3.4f;

// Plaza: circuito de radio 4.5 alrededor de la torre de la campana.
static const float PLAZA_CX = 0.0f;
static const float PLAZA_CZ = -2.0f;
static const float RADIO_CIRCUITO_PLAZA = 4.5f;


//==================================================
// UTILIDADES MATEMATICAS Y DE COLOR
//==================================================

static float Hash2Mercado(int a, int b)
{
    unsigned int h =
        (unsigned int)(a * 374761393 + b * 668265263);

    h = (h ^ (h >> 13)) * 1274126177u;

    return (float)((h ^ (h >> 16)) & 0xFFFF) / 65535.0f;
}


static Color OscurecerMercado(Color c, float factor)
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


// Distancia en el plano XZ de un punto a un segmento.
static float DistanciaSegmentoMercado(
    float px, float pz,
    Vector3 a, Vector3 b
)
{
    float dx = b.x - a.x;
    float dz = b.z - a.z;
    float largo2 = dx * dx + dz * dz;
    float t = 0.0f;

    if (largo2 > 0.0001f)
    {
        t = ((px - a.x) * dx + (pz - a.z) * dz) / largo2;

        if (t < 0.0f) t = 0.0f;
        if (t > 1.0f) t = 1.0f;
    }

    float cx = a.x + dx * t - px;
    float cz = a.z + dz * t - pz;

    return std::sqrt(cx * cx + cz * cz);
}


//==================================================
// RUTA DEL TABLERO (datos de la logica)
//==================================================

enum ZonaMercado
{
    ZONA_EMBARCADERO = 0,
    ZONA_MUELLE_ESPECIAS,
    ZONA_MUELLE_FLORES,
    ZONA_MUELLE_PESCADORES,
    ZONA_PLAZA_MERCADO,
    ZONA_ISLA_FARO,
    ZONA_ASTILLERO,
    CANTIDAD_ZONAS_MERCADO
};

static const char* const NOMBRES_ZONAS_MERCADO[CANTIDAD_ZONAS_MERCADO] =
{
    "EMBARCADERO",
    "MUELLE DE LAS ESPECIAS",
    "MUELLE DE LAS FLORES",
    "MUELLE DE LOS PESCADORES",
    "PLAZA DEL MERCADO",
    "ISLA DEL FARO",
    "ASTILLERO"
};


// Indices de las casillas clave, guardados al construir la ruta (nada del
// juego ni del dibujo depende de un indice literal).
struct NodosMercado
{
    int anillo[CASILLAS_MUELLE];
    int plaza[CASILLAS_PLAZA];
    int faro[CASILLAS_LAZO];
    int astillero[CASILLAS_LAZO];

    int puenteSur;
    int puenteNorte;
    int gondolaEste;
    int gondolaOeste;

    // Bifurcaciones (en el anillo y en la plaza).
    int bifurcacionPuenteSur;
    int bifurcacionGondolaEste;
    int bifurcacionFaro;
    int bifurcacionPuenteNorte;
    int bifurcacionAstillero;
    int bifurcacionGondolaOeste;

    // Llegadas de las gondolas a la plaza.
    int plazaGondolaEste;
    int plazaGondolaOeste;
};

static NodosMercado nodoMercado;

static const int MAX_TROFEOS_MERCADO = 16;

// Datos que acompanan a las casillas (trofeo, textos de ruta, puestos).
static int casillasTrofeoMercado[MAX_TROFEOS_MERCADO];
static int cantidadTrofeosMercado = 0;

static const char* descripcionRutaMercado[MAX_CASILLAS_TABLERO][2];

// Puesto de cada casilla azul: posicion (al costado de la losa) y giro en Y
// para que el mostrador mire a la casilla.
static Vector3 posicionPuestoMercado[MAX_CASILLAS_TABLERO];
static float giroPuestoMercado[MAX_CASILLAS_TABLERO];

// --- Estado de la partida (se reinicia en ConstruirMercado) ---

// Participante dueno del puesto de cada casilla (-1 = libre).
static int duenoPuesto[MAX_CASILLAS_TABLERO];

// Segundos que le quedan sonando a la campana.
static float tiempoCampana = 0.0f;


// Tipo de casilla a partir de su letra en la tabla de diseno.
static TipoCasilla TipoDesdeLetraMercado(char letra)
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


static int NodoMercado(
    Tablero& t,
    ZonaMercado zona,
    char tipo,
    float x, float z
)
{
    int indice =
        t.AgregarCasilla(
            Vector3{ x, ALTURA_CASILLA_MERCADO, z },
            TipoDesdeLetraMercado(tipo)
        );

    if (indice >= 0)
    {
        t.casillas[indice].zona = (int)zona;
    }

    return indice;
}


static void DefinirOpcionesMercado(
    int casilla,
    const char* opcion0,
    const char* opcion1
)
{
    if (casilla < 0 || casilla >= MAX_CASILLAS_TABLERO)
    {
        return;
    }

    descripcionRutaMercado[casilla][0] = opcion0;
    descripcionRutaMercado[casilla][1] = opcion1;
}


// Posicion de la casilla i del anillo de muelles (circuito que empieza
// en el embarcadero, recorre el sur hacia el este, sube por el este,
// cruza el norte hacia el oeste y baja por el oeste).
static Vector2 PosicionMuelleMercado(int i)
{
    if (i <= 4)
    {
        return { -PASO_ANILLO * 0.5f + PASO_ANILLO * i, ANILLO_Z_SUR };
    }

    if (i <= 10)
    {
        return { ANILLO_X, ANILLO_Z_SUR - PASO_ANILLO * (i - 4) };
    }

    if (i <= 17)
    {
        return { ANILLO_X - PASO_ANILLO * (i - 10), ANILLO_Z_NORTE };
    }

    if (i <= 23)
    {
        return { -ANILLO_X, ANILLO_Z_NORTE + PASO_ANILLO * (i - 17) };
    }

    return { -ANILLO_X + PASO_ANILLO * (i - 23), ANILLO_Z_SUR };
}


static ZonaMercado ZonaMuelleMercado(int i)
{
    if (i <= 4 || i >= 24)
    {
        return ZONA_EMBARCADERO;
    }

    if (i <= 10)
    {
        return ZONA_MUELLE_ESPECIAS;
    }

    if (i <= 17)
    {
        return ZONA_MUELLE_FLORES;
    }

    return ZONA_MUELLE_PESCADORES;
}


// Lazos (disenados con una curva suave y paso de 3.2-3.4): la Isla del
// Faro va del muelle este al muelle norte por el noreste y el Astillero
// del muelle oeste al embarcadero por el suroeste.
static const Vector2 POSICIONES_FARO_MERCADO[CASILLAS_LAZO] =
{
    { 15.00f, -4.55f }, { 18.04f, -5.44f }, { 20.49f, -7.52f },
    { 21.79f, -10.45f }, { 21.82f, -13.66f }, { 20.49f, -16.57f },
    { 18.02f, -18.62f }, { 14.94f, -19.53f }, { 11.74f, -19.41f },
    { 8.85f, -18.01f }, { 7.38f, -15.20f }
};

static const Vector2 POSICIONES_ASTILLERO_MERCADO[CASILLAS_LAZO] =
{
    { -14.94f, -3.36f }, { -18.27f, -3.67f }, { -21.44f, -2.72f },
    { -23.55f, -0.15f }, { -24.22f, 3.09f }, { -23.62f, 6.35f },
    { -21.86f, 9.20f }, { -19.33f, 11.34f }, { -16.24f, 12.66f },
    { -12.93f, 12.84f }, { -10.18f, 11.12f }
};


//==================================================
// GEOGRAFIA (tierra firme, canales y landmarks)
//==================================================
//
// El puerto es una ciudad de manzanas de piedra (suelo a y = 0) cortada
// por el canal del anillo, con la isla redonda de la plaza en el centro,
// la Isla del Faro al noreste (terreno natural) y el mar alrededor. Las
// casillas del anillo van sobre muelles de tablones junto al malecon.

// Malecon: borde de piedra del canal, por fuera del anillo de muelles.
static const float CANAL_X = 13.4f;
static const float CANAL_Z_NORTE = -13.7f;
static const float CANAL_Z_SUR = 9.7f;

// Isla de la plaza (radio del muro de piedra).
static const float RADIO_ISLA_MERCADO = 6.2f;

// Landmarks.
static const float FARO_X = 25.2f;
static const float FARO_Z = -19.8f;
static const float DIQUE_X = -17.6f;
static const float DIQUE_Z = 4.4f;
static const float DIQUE_MEDIO_ANCHO = 2.1f;
static const float DIQUE_MEDIO_LARGO = 3.2f;
static const float CASA_FARERO_X = 17.2f;
static const float CASA_FARERO_Z = -11.0f;
static const float AHUMADERO_X = -25.4f;
static const float AHUMADERO_Z = -11.8f;
static const float ALTURA_AHUMADERO = 7.2f;
static const float GRUA_ALTA_X = -26.6f;
static const float GRUA_ALTA_Z[2] = { -4.0f, 9.6f };

struct RectanguloMercado
{
    float x0, x1, z0, z1;
};

// Manzanas de tierra firme (la isla del faro y la plaza van aparte).
static const RectanguloMercado MANZANAS_MERCADO[] =
{
    { -31.0f, 5.0f, -38.0f, CANAL_Z_NORTE },        // barrio norte
    { -31.0f, -CANAL_X, CANAL_Z_NORTE, -5.4f },     // barrio noroeste
    { -27.6f, -CANAL_X, -5.4f, 15.8f },             // astillero
    { -CANAL_X, -6.6f, CANAL_Z_SUR, 15.8f },        // gradas del astillero
    { -6.6f, 31.0f, CANAL_Z_SUR, 12.4f },           // malecon sur
    { CANAL_X, 31.0f, -1.6f, CANAL_Z_SUR }          // barrio este
};

static const int CANTIDAD_MANZANAS_MERCADO =
    (int)(sizeof(MANZANAS_MERCADO) / sizeof(MANZANAS_MERCADO[0]));

struct ZonaReservadaMercado
{
    float x, z, radio;
};

// Alrededor de los landmarks no va decoracion ni puestos.
static const ZonaReservadaMercado ZONAS_RESERVADAS_MERCADO[] =
{
    { PLAZA_CX, PLAZA_CZ, 2.2f },             // torre de la campana
    { FARO_X, FARO_Z, 3.2f },                 // faro
    { CASA_FARERO_X, CASA_FARERO_Z, 2.0f },   // casa del farero
    { DIQUE_X, DIQUE_Z - 1.6f, 3.4f },        // dique seco (mitad norte)
    { DIQUE_X, DIQUE_Z + 1.6f, 3.4f },        // dique seco (mitad sur)
    { GRUA_ALTA_X, GRUA_ALTA_Z[0], 1.5f },    // gruas altas
    { GRUA_ALTA_X, GRUA_ALTA_Z[1], 1.5f }
};


static bool DentroRectanguloMercado(
    const RectanguloMercado& r,
    float x, float z,
    float margen
)
{
    return
        x > r.x0 - margen && x < r.x1 + margen &&
        z > r.z0 - margen && z < r.z1 + margen;
}


// Distancia con signo al borde de la Isla del Faro (< 0 = tierra). La isla
// sigue el lazo del faro, con un lomo en el medio y la punta del faro, y la
// separa del muelle un canal angosto.
static float DistanciaIslaFaroMercado(float x, float z)
{
    Vector3 anterior =
        Vector3{ ANILLO_X, 0.0f, ANILLO_Z_SUR - PASO_ANILLO * 4.0f };

    float distancia = 1.0e9f;

    for (int k = 0; k <= CASILLAS_LAZO; k++)
    {
        Vector3 siguiente =
            k < CASILLAS_LAZO
            ? Vector3{ POSICIONES_FARO_MERCADO[k].x, 0.0f, POSICIONES_FARO_MERCADO[k].y }
            : Vector3{ ANILLO_X - PASO_ANILLO, 0.0f, ANILLO_Z_NORTE };

        float d = DistanciaSegmentoMercado(x, z, anterior, siguiente);

        if (d < distancia) distancia = d;

        anterior = siguiente;
    }

    // Franja de orilla de 3.4 a cada lado del sendero.
    distancia -= 3.4f;

    // El interior del lazo (entre el sendero y el muelle) tambien es isla.
    Vector2 poligono[CASILLAS_LAZO + 4];
    int vertices = 0;

    poligono[vertices++] = Vector2{ ANILLO_X, ANILLO_Z_SUR - PASO_ANILLO * 4.0f };

    for (int k = 0; k < CASILLAS_LAZO; k++)
    {
        poligono[vertices++] = POSICIONES_FARO_MERCADO[k];
    }

    poligono[vertices++] = Vector2{ ANILLO_X - PASO_ANILLO, ANILLO_Z_NORTE };
    poligono[vertices++] = Vector2{ ANILLO_X, ANILLO_Z_NORTE };
    poligono[vertices++] = Vector2{ ANILLO_X, ANILLO_Z_NORTE + PASO_ANILLO };

    bool dentro = false;

    for (int i = 0, j = vertices - 1; i < vertices; j = i++)
    {
        if (
            (poligono[i].y > z) != (poligono[j].y > z) &&
            x < (poligono[j].x - poligono[i].x) * (z - poligono[i].y) /
                (poligono[j].y - poligono[i].y) + poligono[i].x
        )
        {
            dentro = !dentro;
        }
    }

    if (dentro)
    {
        distancia = -3.4f;
    }

    float punta = std::sqrt((x - FARO_X) * (x - FARO_X) + (z - FARO_Z) * (z - FARO_Z)) - 3.6f;

    if (punta < distancia) distancia = punta;

    // Canal entre la isla y el muelle (lado este y norte del anillo).
    float canal = 1.0e9f;

    canal = std::fmin(
        canal,
        DistanciaSegmentoMercado(
            x, z,
            Vector3{ ANILLO_X, 0.0f, 1.4f },
            Vector3{ ANILLO_X, 0.0f, ANILLO_Z_NORTE }
        )
    );

    canal = std::fmin(
        canal,
        DistanciaSegmentoMercado(
            x, z,
            Vector3{ ANILLO_X, 0.0f, ANILLO_Z_NORTE },
            Vector3{ 4.0f, 0.0f, ANILLO_Z_NORTE }
        )
    );

    float fueraCanal = 2.5f - canal;

    return distancia > fueraCanal ? distancia : fueraCanal;
}


static bool EnManzanaMercado(float x, float z, float margen)
{
    for (int i = 0; i < CANTIDAD_MANZANAS_MERCADO; i++)
    {
        if (DentroRectanguloMercado(MANZANAS_MERCADO[i], x, z, margen))
        {
            return true;
        }
    }

    return false;
}


static bool EnIslaPlazaMercado(float x, float z, float margen)
{
    float dx = x - PLAZA_CX;
    float dz = z - PLAZA_CZ;

    return dx * dx + dz * dz < (RADIO_ISLA_MERCADO + margen) * (RADIO_ISLA_MERCADO + margen);
}


// Tierra firme (manzana, plaza o isla del faro); lo demas es agua.
static bool EsTierraMercado(float x, float z)
{
    return
        EnManzanaMercado(x, z, 0.0f) ||
        EnIslaPlazaMercado(x, z, 0.0f) ||
        DistanciaIslaFaroMercado(x, z) < 0.0f;
}


static float DistanciaReservadaMercado(float x, float z)
{
    float mejor = 1.0e9f;

    for (const ZonaReservadaMercado& r : ZONAS_RESERVADAS_MERCADO)
    {
        float d = std::sqrt((x - r.x) * (x - r.x) + (z - r.z) * (z - r.z)) - r.radio;

        if (d < mejor) mejor = d;
    }

    return mejor;
}


// Franja frontal de una casilla (hacia la camara): ahi no va nada alto.
static bool EnFranjaFrontalMercado(
    const Tablero& t,
    float x, float z,
    float holgura
)
{
    for (int a = 0; a < t.cantidadCasillas; a++)
    {
        float dx = x - t.casillas[a].posicion.x;
        float dz = z - t.casillas[a].posicion.z;

        if (
            std::fabs(dx) < 2.0f + holgura &&
            dz > 0.4f - holgura &&
            dz < 3.4f + holgura
        )
        {
            return true;
        }
    }

    return false;
}


// Grado (entradas + salidas) de cada casilla: >= 3 = cruce con plaza.
static int GradoCasillaMercado(const Tablero& t, int indice)
{
    int grado = t.casillas[indice].cantidadConexiones;

    for (int a = 0; a < t.cantidadCasillas; a++)
    {
        for (int c = 0; c < t.casillas[a].cantidadConexiones; c++)
        {
            if (t.casillas[a].conexiones[c].destino == indice)
            {
                grado++;
            }
        }
    }

    return grado;
}


// Distancia al eje del sendero mas cercano (todas las conexiones).
static float DistanciaSenderoMercado(const Tablero& t, float x, float z)
{
    float mejor = 1.0e9f;

    for (int a = 0; a < t.cantidadCasillas; a++)
    {
        for (int c = 0; c < t.casillas[a].cantidadConexiones; c++)
        {
            int b = t.casillas[a].conexiones[c].destino;

            if (b < 0 || b >= t.cantidadCasillas)
            {
                continue;
            }

            float d =
                DistanciaSegmentoMercado(
                    x, z,
                    t.casillas[a].posicion,
                    t.casillas[b].posicion
                );

            if (d < mejor) mejor = d;
        }
    }

    return mejor;
}


// Elige a que lado de cada casilla azul va su puesto: el lugar con mas
// espacio libre respecto de senderos, casillas, otros puestos y landmarks.
// Castiga fuerte la franja frontal de cualquier casilla (taparia fichas)
// y, a igualdad, prefiere la tierra firme del malecon.
static void CalcularPuestosMercado(const Tablero& t)
{
    for (int i = 0; i < t.cantidadCasillas; i++)
    {
        const Casilla& casilla = t.casillas[i];

        posicionPuestoMercado[i] = casilla.posicion;
        giroPuestoMercado[i] = 0.0f;

        if (casilla.tipo != CASILLA_NEUTRA)
        {
            continue;
        }

        // En los cruces la plaza del sendero es mas ancha.
        float distanciaPuesto =
            GradoCasillaMercado(t, i) >= 3
            ? DISTANCIA_PUESTO_CRUCE
            : DISTANCIA_PUESTO;

        float mejorPuntaje = -1.0e9f;
        float mejorX = casilla.posicion.x;
        float mejorZ = casilla.posicion.z - distanciaPuesto;

        for (int d = 0; d < 16; d++)
        {
            float angulo = d * 22.5f * DEG2RAD;
            float dirX = std::cos(angulo);
            float dirZ = std::sin(angulo);

            float px = casilla.posicion.x + dirX * distanciaPuesto;
            float pz = casilla.posicion.z + dirZ * distanciaPuesto;

            float puntaje = DistanciaSenderoMercado(t, px, pz) - 1.9f;

            // Otras losas.
            for (int a = 0; a < t.cantidadCasillas; a++)
            {
                if (a == i)
                {
                    continue;
                }

                float dx = t.casillas[a].posicion.x - px;
                float dz = t.casillas[a].posicion.z - pz;
                float distancia = std::sqrt(dx * dx + dz * dz) - 2.2f;

                if (distancia < puntaje) puntaje = distancia;
            }

            // Puestos ya ubicados.
            for (int a = 0; a < i; a++)
            {
                if (t.casillas[a].tipo != CASILLA_NEUTRA)
                {
                    continue;
                }

                float dx = posicionPuestoMercado[a].x - px;
                float dz = posicionPuestoMercado[a].z - pz;
                float distancia = std::sqrt(dx * dx + dz * dz) - 1.6f;

                if (distancia < puntaje) puntaje = distancia;
            }

            // Landmarks (torre, faro, dique...).
            float reservada = DistanciaReservadaMercado(px, pz) - 0.7f;

            if (reservada < puntaje) puntaje = reservada;

            // Delante de una casilla taparia las fichas.
            if (EnFranjaFrontalMercado(t, px, pz, 0.3f))
            {
                puntaje -= 3.0f;
            }

            // A igualdad, sobre tierra firme y mirando al sendero.
            if (EsTierraMercado(px, pz))
            {
                puntaje += 0.08f;
            }

            if (puntaje > mejorPuntaje)
            {
                mejorPuntaje = puntaje;
                mejorX = px;
                mejorZ = pz;
            }
        }

        posicionPuestoMercado[i] = Vector3{ mejorX, 0.0f, mejorZ };

        // El frente del puesto (+Z local) mira a la casilla.
        float frenteX = casilla.posicion.x - mejorX;
        float frenteZ = casilla.posicion.z - mejorZ;

        giroPuestoMercado[i] = std::atan2(frenteX, frenteZ) * RAD2DEG;
    }
}


static void ConstruirRutaMercado(Tablero& t)
{
    t.cantidadCasillas = 0;
    t.recorridoValido = false;

    for (int i = 0; i < MAX_CASILLAS_TABLERO; i++)
    {
        t.casillas[i] = Casilla{};
        descripcionRutaMercado[i][0] = nullptr;
        descripcionRutaMercado[i][1] = nullptr;
    }

    cantidadTrofeosMercado = 0;

    NodosMercado& nodo = nodoMercado;

    // Letras: + verde, - roja, n azul (puesto), E especial, R regalo,
    // C caos, I intercambio, D duelo, T tienda, V evento (campana).
    // Puestos 20 (33 %), + 12, - 8 y 20 especiales con personalidad;
    // nunca mas de 4 seguidas sin un especial.

    // --- Anillo de muelles (anillo[0] = embarcadero, salida) ---
    const char* tiposMuelle = "+nVnRnnTV+-+nInV+n-V+n-nDn";

    for (int i = 0; i < CASILLAS_MUELLE; i++)
    {
        Vector2 p = PosicionMuelleMercado(i);

        nodo.anillo[i] =
            NodoMercado(t, ZonaMuelleMercado(i), tiposMuelle[i], p.x, p.y);
    }

    // --- Plaza del mercado: circuito de 8 casillas alrededor de la torre ---
    // plaza[k] a 22.5 + 45k grados desde el sur, hacia el este: 0 = sur-
    // sureste (llega el puente sur), 1 = este (gondola este), 3 = norte
    // (sale el puente norte), 6 = oeste (gondola oeste). Ninguna queda
    // justo detras de la torre.
    const char tiposPlaza[CASILLAS_PLAZA] = { 'E', 'n', 'T', 'n', 'V', 'n', 'R', '+' };

    for (int k = 0; k < CASILLAS_PLAZA; k++)
    {
        float angulo = (22.5f + 45.0f * k) * DEG2RAD;

        nodo.plaza[k] =
            NodoMercado(
                t, ZONA_PLAZA_MERCADO, tiposPlaza[k],
                PLAZA_CX + std::sin(angulo) * RADIO_CIRCUITO_PLAZA,
                PLAZA_CZ + std::cos(angulo) * RADIO_CIRCUITO_PLAZA
            );
    }

    // --- Atajos sobre el canal: dos puentes y dos gondolas ---
    nodo.puenteSur = NodoMercado(t, ZONA_EMBARCADERO, '-', 1.70f, 5.20f);
    nodo.puenteNorte = NodoMercado(t, ZONA_MUELLE_FLORES, '+', 1.70f, -9.20f);
    nodo.gondolaEste = NodoMercado(t, ZONA_MUELLE_ESPECIAS, '-', 8.03f, 0.56f);
    nodo.gondolaOeste = NodoMercado(t, ZONA_MUELLE_PESCADORES, '-', -8.03f, 0.56f);

    // --- Lazos largos ---
    const char* tiposFaro = "+nE+nVCnD+-";
    const char* tiposAstillero = "n+In+VnCn-R";

    for (int k = 0; k < CASILLAS_LAZO; k++)
    {
        nodo.faro[k] =
            NodoMercado(
                t, ZONA_ISLA_FARO, tiposFaro[k],
                POSICIONES_FARO_MERCADO[k].x, POSICIONES_FARO_MERCADO[k].y
            );
    }

    for (int k = 0; k < CASILLAS_LAZO; k++)
    {
        nodo.astillero[k] =
            NodoMercado(
                t, ZONA_ASTILLERO, tiposAstillero[k],
                POSICIONES_ASTILLERO_MERCADO[k].x, POSICIONES_ASTILLERO_MERCADO[k].y
            );
    }

    // Casillas del anillo donde nace cada desvio.
    nodo.bifurcacionPuenteSur = nodo.anillo[1];
    nodo.bifurcacionGondolaEste = nodo.anillo[6];
    nodo.bifurcacionFaro = nodo.anillo[8];
    nodo.bifurcacionPuenteNorte = nodo.plaza[3];
    nodo.bifurcacionAstillero = nodo.anillo[20];
    nodo.bifurcacionGondolaOeste = nodo.anillo[21];
    nodo.plazaGondolaEste = nodo.plaza[1];
    nodo.plazaGondolaOeste = nodo.plaza[6];

    // --- Conexiones (la opcion 0, lo largo, se conecta primero) ---

    // Anillo de muelles; los lazos salen antes de seguir el muelle.
    for (int i = 0; i < CASILLAS_MUELLE; i++)
    {
        int actual = nodo.anillo[i];
        int siguiente = nodo.anillo[(i + 1) % CASILLAS_MUELLE];

        if (actual == nodo.bifurcacionFaro)
        {
            t.ConectarCasillas(actual, nodo.faro[0]);
        }
        else if (actual == nodo.bifurcacionAstillero)
        {
            t.ConectarCasillas(actual, nodo.astillero[0]);
        }

        t.ConectarCasillas(actual, siguiente);

        if (actual == nodo.bifurcacionPuenteSur)
        {
            t.ConectarCasillas(actual, nodo.puenteSur);
        }
        else if (actual == nodo.bifurcacionGondolaEste)
        {
            t.ConectarCasillas(actual, nodo.gondolaEste);
        }
        else if (actual == nodo.bifurcacionGondolaOeste)
        {
            t.ConectarCasillas(actual, nodo.gondolaOeste);
        }
    }

    // Atajos a la plaza.
    t.ConectarCasillas(nodo.puenteSur, nodo.plaza[0]);
    t.ConectarCasillas(nodo.gondolaEste, nodo.plazaGondolaEste);
    t.ConectarCasillas(nodo.gondolaOeste, nodo.plazaGondolaOeste);

    // Circuito de la plaza; en el norte se puede salir por el puente.
    for (int k = 0; k < CASILLAS_PLAZA; k++)
    {
        t.ConectarCasillas(nodo.plaza[k], nodo.plaza[(k + 1) % CASILLAS_PLAZA]);

        if (nodo.plaza[k] == nodo.bifurcacionPuenteNorte)
        {
            t.ConectarCasillas(nodo.plaza[k], nodo.puenteNorte);
        }
    }

    t.ConectarCasillas(nodo.puenteNorte, nodo.anillo[13]);

    // Lazos: vuelven al muelle norte y al embarcadero.
    for (int k = 0; k + 1 < CASILLAS_LAZO; k++)
    {
        t.ConectarCasillas(nodo.faro[k], nodo.faro[k + 1]);
        t.ConectarCasillas(nodo.astillero[k], nodo.astillero[k + 1]);
    }

    t.ConectarCasillas(nodo.faro[CASILLAS_LAZO - 1], nodo.anillo[11]);
    t.ConectarCasillas(nodo.astillero[CASILLAS_LAZO - 1], nodo.anillo[24]);

    // --- Textos de las bifurcaciones ---
    DefinirOpcionesMercado(nodo.bifurcacionPuenteSur, "SEGUIR EL MUELLE", "PUENTE A LA PLAZA");
    DefinirOpcionesMercado(nodo.bifurcacionGondolaEste, "SEGUIR EL MUELLE", "GÓNDOLA A LA PLAZA");
    DefinirOpcionesMercado(nodo.bifurcacionFaro, "RODEAR LA ISLA DEL FARO", "SEGUIR EL MUELLE");
    DefinirOpcionesMercado(nodo.bifurcacionPuenteNorte, "SEGUIR EN LA PLAZA", "PUENTE AL MUELLE NORTE");
    DefinirOpcionesMercado(nodo.bifurcacionAstillero, "IR AL ASTILLERO", "SEGUIR EL MUELLE");
    DefinirOpcionesMercado(nodo.bifurcacionGondolaOeste, "SEGUIR EL MUELLE", "GÓNDOLA A LA PLAZA");

    // --- Candidatas del trofeo: 2-3 por zona, ninguna en los atajos ---
    const int candidatas[] =
    {
        nodo.anillo[3], nodo.anillo[25],                        // embarcadero
        nodo.anillo[5], nodo.anillo[8],                         // especias
        nodo.anillo[12], nodo.anillo[16],                       // flores
        nodo.anillo[18], nodo.anillo[20],                       // pescadores
        nodo.plaza[1], nodo.plaza[5],                           // plaza
        nodo.faro[3], nodo.faro[7],                             // faro
        nodo.astillero[1], nodo.astillero[4], nodo.astillero[8] // astillero
    };

    for (int indice : candidatas)
    {
        if (cantidadTrofeosMercado < MAX_TROFEOS_MERCADO)
        {
            casillasTrofeoMercado[cantidadTrofeosMercado++] = indice;
        }
    }

    CalcularPuestosMercado(t);

    t.recorridoValido = t.ValidarRecorrido();
}


// Copia del tablero solo para el dibujo (no toca el estado de partida).
static const Tablero& TableroReferenciaMercado()
{
    static Tablero referencia;
    static bool lista = false;

    if (!lista)
    {
        ConstruirRutaMercado(referencia);
        lista = true;
    }

    return referencia;
}


static void ConstruirMercado(
    Tablero& tablero
)
{
    ConstruirRutaMercado(tablero);

    // Partida nueva: todos los puestos libres y la campana en silencio.
    for (int i = 0; i < MAX_CASILLAS_TABLERO; i++)
    {
        duenoPuesto[i] = -1;
    }

    tiempoCampana = 0.0f;
}


//==================================================
// PUESTOS (mecanica principal)
//==================================================

static int ContarPuestosMercado(int jugador)
{
    int cantidad = 0;

    for (int i = 0; i < MAX_CASILLAS_TABLERO; i++)
    {
        if (duenoPuesto[i] == jugador)
        {
            cantidad++;
        }
    }

    return cantidad;
}


// Resumen para el HUD: "PUESTOS  J1:2  J2:0 ..." (solo quien participa).
static void ActualizarEstadoPuestosMercado(
    PartidaTablero& partida
)
{
    char* texto = partida.estadoGimmick;
    const int capacidad = (int)sizeof(partida.estadoGimmick);

    int largo = std::snprintf(texto, capacidad, "PUESTOS");
    bool alguno = false;

    for (int i = 0; i < MAX_PARTICIPANTES && largo > 0 && largo < capacidad; i++)
    {
        if (!partida.jugadores[i].participa)
        {
            continue;
        }

        alguno = true;

        largo +=
            std::snprintf(
                texto + largo,
                capacidad - largo,
                "  J%d:%d",
                i + 1,
                ContarPuestosMercado(i)
            );
    }

    // Vista previa del menu (sin jugadores): la regla en una linea.
    if (!alguno)
    {
        std::snprintf(
            texto,
            capacidad,
            "PUESTOS AZULES: ABRILOS POR %d Y COBRÁ ALQUILER",
            COSTO_ABRIR_PUESTO
        );
    }
}


// Casilla azul: abrir el puesto libre, cobrar el propio o pagar alquiler.
static void ResolverPuestoMercado(
    PartidaTablero& partida,
    int participante,
    int casilla
)
{
    EstadoJugadorPartidaTablero& jugador =
        partida.jugadores[participante];

    int dueno = duenoPuesto[casilla];

    if (dueno < 0)
    {
        if (jugador.monedas >= COSTO_ABRIR_PUESTO)
        {
            jugador.monedas -= COSTO_ABRIR_PUESTO;
            duenoPuesto[casilla] = participante;
            partida.variacionMonedasEvento = -COSTO_ABRIR_PUESTO;

            std::snprintf(
                partida.textoEvento,
                sizeof(partida.textoEvento),
                "¡ABRISTE UN PUESTO! PAGASTE %d MONEDAS",
                COSTO_ABRIR_PUESTO
            );

            ReproducirSonidoTablero(partida, SONIDO_COMPRA);
        }
        else
        {
            std::snprintf(
                partida.textoEvento,
                sizeof(partida.textoEvento),
                "NO TE ALCANZA PARA ABRIR EL PUESTO (CUESTA %d)",
                COSTO_ABRIR_PUESTO
            );

            ReproducirSonidoTablero(partida, SONIDO_UI_CANCELAR);
        }
    }
    else if (dueno == participante)
    {
        jugador.monedas += GANANCIA_PUESTO_PROPIO;
        partida.variacionMonedasEvento = GANANCIA_PUESTO_PROPIO;

        std::snprintf(
            partida.textoEvento,
            sizeof(partida.textoEvento),
            "¡TU PUESTO VENDIÓ! COBRÁS %d MONEDAS",
            GANANCIA_PUESTO_PROPIO
        );

        ReproducirSonidoTablero(partida, SONIDO_MONEDA);
    }
    else
    {
        int pago =
            jugador.monedas < ALQUILER_PUESTO
            ? jugador.monedas
            : ALQUILER_PUESTO;

        if (pago <= 0)
        {
            std::snprintf(
                partida.textoEvento,
                sizeof(partida.textoEvento),
                "PUESTO DE J%d: NO TENÉS MONEDAS PARA EL ALQUILER",
                dueno + 1
            );
        }
        else if (EscudoAbsorbePerdida(partida, participante))
        {
            std::snprintf(
                partida.textoEvento,
                sizeof(partida.textoEvento),
                "¡EL ESCUDO DE J%d EVITA EL ALQUILER!",
                participante + 1
            );
        }
        else
        {
            jugador.monedas -= pago;
            partida.jugadores[dueno].monedas += pago;
            partida.variacionMonedasEvento = -pago;

            std::snprintf(
                partida.textoEvento,
                sizeof(partida.textoEvento),
                "ALQUILER: LE PAGÁS %d MONEDAS A J%d",
                pago,
                dueno + 1
            );

            ReproducirSonidoTablero(partida, SONIDO_CASILLA_NEGATIVA);
        }
    }

    ActualizarEstadoPuestosMercado(partida);
}


//==================================================
// EVENTO: LA CAMPANA DEL MERCADO
//==================================================

static void HacerSonarCampanaMercado(
    PartidaTablero& partida
)
{
    // gimmickActivo + mensaje: la camara mira un momento la torre.
    tiempoCampana = SEGUNDOS_CAMPANA;
    partida.gimmickActivo = true;
}


static void ResolverFeriaMercado(
    PartidaTablero& partida,
    int participante
)
{
    char resumen[64] = {};
    int largo = 0;
    int cobradoJugador = 0;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!partida.jugadores[i].participa)
        {
            continue;
        }

        int cobro = ContarPuestosMercado(i);

        if (cobro > MAXIMO_COBRO_FERIA)
        {
            cobro = MAXIMO_COBRO_FERIA;
        }

        if (cobro <= 0)
        {
            continue;
        }

        partida.jugadores[i].monedas += cobro;

        if (i == participante)
        {
            cobradoJugador = cobro;
        }

        if (largo >= 0 && largo < (int)sizeof(resumen))
        {
            largo +=
                std::snprintf(
                    resumen + largo,
                    sizeof(resumen) - largo,
                    " J%d +%d",
                    i + 1,
                    cobro
                );
        }
    }

    if (largo == 0)
    {
        // Nadie tiene puestos: la feria deja una propina a quien cayo.
        partida.jugadores[participante].monedas += PROPINA_FERIA;
        cobradoJugador = PROPINA_FERIA;

        std::snprintf(
            partida.textoEvento,
            sizeof(partida.textoEvento),
            "¡DIA DE FERIA! SIN PUESTOS ABIERTOS: +%d PARA VOS",
            PROPINA_FERIA
        );
    }
    else
    {
        std::snprintf(
            partida.textoEvento,
            sizeof(partida.textoEvento),
            "¡DIA DE FERIA! COBRAN:%s",
            resumen
        );
    }

    partida.variacionMonedasEvento = cobradoJugador;

    ReproducirSonidoTablero(partida, SONIDO_MONEDA);
}


// Quien mas puestos tiene (empate: uno de ellos al azar) pierde uno.
// Devuelve false si nadie tiene puestos.
static bool ResolverInspeccionMercado(
    PartidaTablero& partida,
    int participante
)
{
    int maximo = 0;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (partida.jugadores[i].participa)
        {
            int cantidad = ContarPuestosMercado(i);

            if (cantidad > maximo) maximo = cantidad;
        }
    }

    if (maximo <= 0)
    {
        return false;
    }

    int candidatos[MAX_PARTICIPANTES]{};
    int cantidadCandidatos = 0;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (
            partida.jugadores[i].participa &&
            ContarPuestosMercado(i) == maximo
        )
        {
            candidatos[cantidadCandidatos++] = i;
        }
    }

    int inspeccionado =
        candidatos[GetRandomValue(0, cantidadCandidatos - 1)];

    int puestos[MAX_CASILLAS_TABLERO]{};
    int cantidadPuestos = 0;

    for (int i = 0; i < MAX_CASILLAS_TABLERO; i++)
    {
        if (duenoPuesto[i] == inspeccionado)
        {
            puestos[cantidadPuestos++] = i;
        }
    }

    duenoPuesto[puestos[GetRandomValue(0, cantidadPuestos - 1)]] = -1;

    if (inspeccionado == participante)
    {
        std::snprintf(
            partida.textoEvento,
            sizeof(partida.textoEvento),
            "¡INSPECCIÓN! PERDÉS UN PUESTO, QUE QUEDA LIBRE"
        );
    }
    else
    {
        std::snprintf(
            partida.textoEvento,
            sizeof(partida.textoEvento),
            "¡INSPECCIÓN! J%d PIERDE UN PUESTO, QUE QUEDA LIBRE",
            inspeccionado + 1
        );
    }

    ReproducirSonidoTablero(partida, SONIDO_CASILLA_NEGATIVA);

    return true;
}


static void ResolverCampanaMercado(
    PartidaTablero& partida,
    int participante
)
{
    ReproducirSonidoTablero(partida, SONIDO_EVENTO_TABLERO);

    HacerSonarCampanaMercado(partida);
    EstablecerMensajeGimmick(partida, "¡SUENA LA CAMPANA DEL MERCADO!");

    // Inspeccion sin puestos en el mercado: pasa a dia de feria.
    if (
        GetRandomValue(0, 1) == 1 &&
        ResolverInspeccionMercado(partida, participante)
    )
    {
        ActualizarEstadoPuestosMercado(partida);
        return;
    }

    ResolverFeriaMercado(partida, participante);
    ActualizarEstadoPuestosMercado(partida);
}


static bool ResolverCasillaMercado(
    PartidaTablero& partida,
    int participante
)
{
    int indice = partida.jugadores[participante].casillaActual;

    const Casilla* casilla =
        partida.tablero.ObtenerCasilla(indice);

    if (casilla == nullptr)
    {
        return false;
    }

    if (casilla->tipo == CASILLA_EVENTO)
    {
        ResolverCampanaMercado(partida, participante);
        return true;
    }

    if (casilla->tipo == CASILLA_NEUTRA)
    {
        ResolverPuestoMercado(partida, participante, indice);
        return true;
    }

    // El resto de las casillas usa la regla comun.
    return false;
}


//==================================================
// RONDAS Y ANIMACION
//==================================================

static void AlIniciarRondaMercado(
    PartidaTablero& partida
)
{
    // La campana abre cada ronda del mercado.
    HacerSonarCampanaMercado(partida);
    ActualizarEstadoPuestosMercado(partida);

    if (partida.rondaActual <= 1)
    {
        char mensaje[96];

        std::snprintf(
            mensaje,
            sizeof(mensaje),
            "PUESTOS AZULES: ABRILOS POR %d Y COBRÁ ALQUILER",
            COSTO_ABRIR_PUESTO
        );

        EstablecerMensajeGimmick(partida, mensaje);
        return;
    }

    // Rondas siguientes: quien domina el mercado (si es uno solo).
    int lider = -1;
    int maximo = 0;
    bool empate = false;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!partida.jugadores[i].participa)
        {
            continue;
        }

        int cantidad = ContarPuestosMercado(i);

        if (cantidad > maximo)
        {
            maximo = cantidad;
            lider = i;
            empate = false;
        }
        else if (cantidad == maximo && cantidad > 0)
        {
            empate = true;
        }
    }

    if (lider >= 0 && !empate)
    {
        char mensaje[96];

        std::snprintf(
            mensaje,
            sizeof(mensaje),
            "J%d LIDERA EL MERCADO CON %d PUESTOS",
            lider + 1,
            maximo
        );

        EstablecerMensajeGimmick(partida, mensaje);
    }
    else
    {
        EstablecerMensajeGimmick(partida, "¡LA CAMPANA ABRE OTRA RONDA DEL MERCADO!");
    }
}


static void ActualizarGimmickMercado(
    PartidaTablero& partida,
    float deltaTime
)
{
    if (tiempoCampana > 0.0f)
    {
        tiempoCampana -= deltaTime;

        if (tiempoCampana <= 0.0f)
        {
            tiempoCampana = 0.0f;
            partida.gimmickActivo = false;
        }
    }
    else
    {
        partida.gimmickActivo = false;
    }

    // gimmickAnim = fuerza del vaiven de la campana.
    float objetivo =
        partida.gimmickActivo
        ? 1.0f
        : 0.0f;

    float paso =
        VELOCIDAD_ANIMACION_CAMPANA * deltaTime;

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


// Leyenda del HUD: las azules son puestos y el evento es la campana.
static bool TextoLeyendaCasillaMercado(
    const PartidaTablero& partida,
    TipoCasilla tipo,
    char* texto,
    int capacidad
)
{
    (void)partida;

    if (tipo == CASILLA_NEUTRA)
    {
        std::snprintf(texto, capacidad, "PUESTO");
        return true;
    }

    if (tipo == CASILLA_EVENTO)
    {
        std::snprintf(texto, capacidad, "CAMPANA");
        return true;
    }

    return false;
}


//==================================================
// BOTS Y TEXTOS DE RUTA
//==================================================

// Valor aproximado de pisar una casilla para este participante.
static int ValorCasillaBotMercado(
    const PartidaTablero& partida,
    int participante,
    int indice
)
{
    const Casilla* casilla = partida.tablero.ObtenerCasilla(indice);

    if (casilla == nullptr)
    {
        return 0;
    }

    // Sin "default": un tipo nuevo de casilla obliga a puntuarlo aqui.
    switch (casilla->tipo)
    {
        case CASILLA_NEUTRA:
        {
            int dueno = duenoPuesto[indice];

            if (dueno < 0)
            {
                // Abrir un puesto es una inversion si se puede pagar.
                return partida.jugadores[participante].monedas >= COSTO_ABRIR_PUESTO
                    ? 1
                    : 0;
            }

            return dueno == participante
                ? GANANCIA_PUESTO_PROPIO
                : -ALQUILER_PUESTO;
        }

        case CASILLA_POSITIVA: return MONEDAS_CASILLA_POSITIVA;
        case CASILLA_NEGATIVA: return -MONEDAS_CASILLA_NEGATIVA;
        case CASILLA_ESPECIAL: return 1;
        case CASILLA_REGALO: return MONEDAS_REGALO;
        case CASILLA_CAOS: return -4;           // ruleta de castigos
        case CASILLA_INTERCAMBIO: return -1;    // azar: algo arriesgada
        case CASILLA_EVENTO: return 1;          // feria o inspeccion
        case CASILLA_DUELO: return 0;           // apuesta voluntaria
        case CASILLA_TIENDA: return 1;          // compra objetos: algo util
        case CANTIDAD_TIPOS_CASILLA: return 0;
    }

    return 0;
}


// Valor medio de los proximos pasos desde una casilla (todas las ramas).
static float ValorRutaBotMercado(
    const PartidaTablero& partida,
    int participante,
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
            ValorRutaBotMercado(
                partida,
                participante,
                actual->conexiones[c].destino,
                profundidad - 1
            );
    }

    if (actual->cantidadConexiones > 0)
    {
        resto /= (float)actual->cantidadConexiones;
    }

    return (float)ValorCasillaBotMercado(partida, participante, casilla) + resto;
}


// Pasos minimos desde una casilla hasta otra (-1 si no hay camino).
static int PasosHastaMercado(
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
// mas rentable (sus puestos, sin alquileres de rivales). Algo de azar.
static int DecidirRutaBotMercado(
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
                PasosHastaMercado(
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
            ValorRutaBotMercado(
                partida,
                participante,
                actual->conexiones[c].destino,
                5
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


static const char* DescribirRutaMercado(
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

    return descripcionRutaMercado[casilla][opcion];
}


//==================================================
// ESCENARIO: COLORES, LUZ Y PIEZAS COMUNES
//==================================================
//
// Todo lo estatico se hornea en pocas MallaGpu que se reconstruyen solo al
// cambiar la calidad de decoracion:
//   - mar: plano grande con degrade hacia el horizonte del atardecer;
//   - tierra: manzanas de piedra con malecon, isla de la plaza, Isla del
//     Faro (terreno con relieve) y el dique seco del astillero;
//   - senderos: muelles de tablones de 2.7 con pilotes, plazas en los
//     cruces, baldosas en la plaza y barandas en los puentes;
//   - escenario: casas, landmarks y props (ver mas abajo).
// La luz va horneada por triangulo, igual que en Arboleda.

static const Color MADERA_MERCADO = Color{ 150, 104, 66, 255 };
static const Color MADERA_CLARA_MERCADO = Color{ 184, 138, 90, 255 };
static const Color MADERA_OSCURA_MERCADO = Color{ 96, 64, 42, 255 };
static const Color PIEDRA_MALECON = Color{ 190, 170, 146, 255 };
static const Color PIEDRA_CLARA_MERCADO = Color{ 222, 206, 180, 255 };
static const Color PIEDRA_OSCURA_MERCADO = Color{ 122, 108, 96, 255 };
static const Color BALDOSA_PLAZA = Color{ 214, 192, 158, 255 };
static const Color HIERRO_MERCADO = Color{ 52, 48, 54, 255 };
static const Color LUZ_FAROL_MERCADO = Color{ 255, 214, 130, 255 };

// Alturas del sendero sobre el suelo (cada capa un escalon por encima de
// la anterior para evitar el z-fighting), como en Arboleda.
static const float SENDERO_SEMIANCHO_MERCADO = 1.35f;
static const float SENDERO_BORDE_MERCADO = 0.30f;
static const float SENDERO_PLAZA_MERCADO = 1.85f;
static const float LIFT_BORDE_MERCADO = 0.030f;
static const float LIFT_BORDE_NODO_MERCADO = 0.050f;
static const float LIFT_TABLA_MERCADO = 0.070f;
static const float LIFT_TABLA_NODO_MERCADO = 0.090f;
static const float LIFT_DETALLE_MERCADO = 0.108f;

static MallaGpu mallaMarMercado;
static MallaGpu mallaTierraMercado;
static MallaGpu mallaSenderosMercado;
static int nivelMallasMercado = -1;

// Un solo constructor reutilizado para todas las mallas (se copia a la GPU).
static ConstructorMalla constructorMercado;


static float Limitar01Mercado(float t)
{
    return t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t);
}


static float Suave01Mercado(float t)
{
    t = Limitar01Mercado(t);
    return t * t * (3.0f - 2.0f * t);
}


static Color MezclaMercado(Color a, Color b, float t)
{
    t = Limitar01Mercado(t);

    return Color
    {
        (unsigned char)(a.r + (b.r - a.r) * t),
        (unsigned char)(a.g + (b.g - a.g) * t),
        (unsigned char)(a.b + (b.b - a.b) * t),
        (unsigned char)(a.a + (b.a - a.a) * t)
    };
}


// Ventanas encendidas, faroles y vidrios: no los oscurece la luz horneada.
static const unsigned char ALFA_EMISIVO_MERCADO = 254;

static Color EmisivoMercado(Color c)
{
    c.a = ALFA_EMISIVO_MERCADO;
    return c;
}


static float FactorLuzMercado(float nx, float ny, float nz)
{
    float luz = -0.45f * nx + 0.80f * ny + 0.40f * nz;

    if (luz < 0.0f)
    {
        luz = 0.0f;
    }

    return 0.66f + 0.46f * luz;
}


// Aplica la luz fija a los triangulos emitidos desde "desde" (multiplo de 3).
static void IluminarMallaMercado(
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
            factor = FactorLuzMercado(nx / largo, ny / largo, nz / largo);
        }

        for (int k = 0; k < 3; k++)
        {
            unsigned char* c = &m.colores[(v + k) * 4];

            if (c[3] == ALFA_EMISIVO_MERCADO)
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


// Cuadro horizontal que mira hacia arriba (x0 < x1, z0 < z1).
static void CuadroSueloMercado(
    ConstructorMalla& m,
    float x0, float z0,
    float x1, float z1,
    float y,
    Color color
)
{
    m.Triangulo(Vector3{ x0, y, z0 }, Vector3{ x0, y, z1 }, Vector3{ x1, y, z1 }, color);
    m.Triangulo(Vector3{ x0, y, z0 }, Vector3{ x1, y, z1 }, Vector3{ x1, y, z0 }, color);
}


// Pared vertical de dos caras entre (ax, az) y (bx, bz).
static void ParedMercado(
    ConstructorMalla& m,
    float ax, float az,
    float bx, float bz,
    float y0, float y1,
    Color color
)
{
    Vector3 a = { ax, y0, az };
    Vector3 b = { bx, y0, bz };
    Vector3 c = { bx, y1, bz };
    Vector3 d = { ax, y1, az };

    m.TrianguloDobleCara(a, b, c, color);
    m.TrianguloDobleCara(a, c, d, color);
}


//==================================================
// MAR
//==================================================

// Centro de la caja de casillas (camara y degrade del mar).
static const float CENTRO_MAPA_X = -1.2f;
static const float CENTRO_MAPA_Z = -3.35f;


// Turquesa cerca del puerto, azul profundo lejos y el reflejo rosado del
// sol poniente hacia el norte (detras del mapa, donde mira la camara).
static Color ColorMarMercado(float x, float z)
{
    const Color cerca = Color{ 50, 148, 166, 255 };
    const Color lejos = Color{ 38, 82, 126, 255 };
    const Color ocaso = Color{ 232, 146, 122, 255 };

    float dx = x - CENTRO_MAPA_X;
    float dz = z - CENTRO_MAPA_Z;
    float r = std::sqrt(dx * dx + dz * dz);

    Color c = MezclaMercado(cerca, lejos, Suave01Mercado((r - 28.0f) / 70.0f));

    float brillo =
        Suave01Mercado((-z - 36.0f) / 90.0f) *
        (1.0f - Suave01Mercado(std::fabs(x) / 130.0f));

    return MezclaMercado(c, ocaso, brillo * 0.8f);
}


static void ConstruirMallaMarMercado(NivelCalidadGrafica nivel)
{
    ConstructorMalla& m = constructorMercado;
    m.Limpiar();

    const float extension = 150.0f;
    const int celdas = nivel == CALIDAD_BAJA ? 30 : 50;
    const float paso = extension * 2.0f / celdas;
    const float y = NIVEL_AGUA_MERCADO;

    for (int ix = 0; ix < celdas; ix++)
    {
        for (int iz = 0; iz < celdas; iz++)
        {
            float x0 = -extension + ix * paso;
            float z0 = -extension + iz * paso;
            float x1 = x0 + paso;
            float z1 = z0 + paso;

            Color c00 = ColorMarMercado(x0, z0);
            Color c01 = ColorMarMercado(x0, z1);
            Color c11 = ColorMarMercado(x1, z1);
            Color c10 = ColorMarMercado(x1, z0);

            m.Vertice(x0, y, z0, c00);
            m.Vertice(x0, y, z1, c01);
            m.Vertice(x1, y, z1, c11);

            m.Vertice(x0, y, z0, c00);
            m.Vertice(x1, y, z1, c11);
            m.Vertice(x1, y, z0, c10);
        }
    }

    ActualizarMallaGpu(mallaMarMercado, m);
}


//==================================================
// TIERRA: MANZANAS, MALECON, PLAZA E ISLA DEL FARO
//==================================================

// Piso de cada manzana (adoquines) y del astillero (tierra y tablones).
static Color ColorSueloManzanaMercado(int manzana, int ix, int iz)
{
    float v = Hash2Mercado(ix * 7 + manzana * 131, iz * 13 + 5);

    Color base = Color{ 186, 168, 144, 255 };

    if (manzana == 2 || manzana == 3)
    {
        // Astillero: tierra apisonada con manchas de tablones.
        base =
            Hash2Mercado(ix / 3 + 17, iz / 3 + 3) > 0.62f
            ? Color{ 150, 112, 76, 255 }
            : Color{ 168, 146, 112, 255 };
    }
    else if (manzana == 4)
    {
        base = Color{ 198, 180, 152, 255 };
    }

    return OscurecerMercado(base, 0.92f + 0.14f * v);
}


static bool EnDiqueMercado(float x, float z, float margen)
{
    return
        std::fabs(x - DIQUE_X) < DIQUE_MEDIO_ANCHO + margen &&
        std::fabs(z - DIQUE_Z) < DIQUE_MEDIO_LARGO + margen;
}


// Muro del malecon y cordon de piedra en los bordes de manzana que dan al agua.
static void AgregarMaleconManzanaMercado(
    ConstructorMalla& m,
    const RectanguloMercado& r,
    bool detalle
)
{
    const float tramo = detalle ? 1.0f : 2.0f;
    const float fondo = -1.4f;

    for (int lado = 0; lado < 4; lado++)
    {
        float ax, az, bx, bz, nx, nz;

        switch (lado)
        {
            case 0: ax = r.x0; az = r.z1; bx = r.x1; bz = r.z1; nx = 0.0f; nz = 1.0f; break;
            case 1: ax = r.x0; az = r.z0; bx = r.x1; bz = r.z0; nx = 0.0f; nz = -1.0f; break;
            case 2: ax = r.x0; az = r.z0; bx = r.x0; bz = r.z1; nx = -1.0f; nz = 0.0f; break;
            default: ax = r.x1; az = r.z0; bx = r.x1; bz = r.z1; nx = 1.0f; nz = 0.0f; break;
        }

        float largo = std::sqrt((bx - ax) * (bx - ax) + (bz - az) * (bz - az));
        int tramos = (int)std::ceil(largo / tramo);

        for (int k = 0; k < tramos; k++)
        {
            float t0 = (float)k / tramos;
            float t1 = (float)(k + 1) / tramos;

            float px0 = ax + (bx - ax) * t0;
            float pz0 = az + (bz - az) * t0;
            float px1 = ax + (bx - ax) * t1;
            float pz1 = az + (bz - az) * t1;
            float mx = (px0 + px1) * 0.5f;
            float mz = (pz0 + pz1) * 0.5f;

            if (EsTierraMercado(mx + nx * 0.4f, mz + nz * 0.4f))
            {
                continue;
            }

            float v = Hash2Mercado((int)(mx * 3.0f) + 900, (int)(mz * 3.0f) + lado);

            ParedMercado(
                m, px0, pz0, px1, pz1, fondo, 0.0f,
                OscurecerMercado(PIEDRA_MALECON, 0.84f + 0.16f * v)
            );

            // Linea de algas y humedad sobre el agua.
            ParedMercado(
                m,
                px0 + nx * 0.01f, pz0 + nz * 0.01f,
                px1 + nx * 0.01f, pz1 + nz * 0.01f,
                NIVEL_AGUA_MERCADO - 0.05f, NIVEL_AGUA_MERCADO + 0.10f,
                Color{ 88, 104, 78, 255 }
            );

            // Cordon de piedra clara.
            m.Caja(
                Vector3{ mx - nx * 0.18f, 0.05f, mz - nz * 0.18f },
                Vector3
                {
                    nx != 0.0f ? 0.46f : std::fabs(px1 - px0) + 0.02f,
                    0.10f,
                    nz != 0.0f ? 0.46f : std::fabs(pz1 - pz0) + 0.02f
                },
                OscurecerMercado(PIEDRA_CLARA_MERCADO, 0.94f + 0.08f * v)
            );
        }
    }
}


static void AgregarManzanasMercado(ConstructorMalla& m, NivelCalidadGrafica nivel)
{
    bool detalle = nivel != CALIDAD_BAJA;
    float celda = nivel == CALIDAD_ALTA ? 1.0f : (nivel == CALIDAD_MEDIA ? 1.5f : 3.0f);

    for (int i = 0; i < CANTIDAD_MANZANAS_MERCADO; i++)
    {
        const RectanguloMercado& r = MANZANAS_MERCADO[i];

        int nx = (int)std::ceil((r.x1 - r.x0) / celda);
        int nz = (int)std::ceil((r.z1 - r.z0) / celda);

        for (int ix = 0; ix < nx; ix++)
        {
            for (int iz = 0; iz < nz; iz++)
            {
                float x0 = r.x0 + ix * celda;
                float z0 = r.z0 + iz * celda;
                float x1 = std::fmin(x0 + celda, r.x1);
                float z1 = std::fmin(z0 + celda, r.z1);

                // Hueco del dique seco.
                if (EnDiqueMercado((x0 + x1) * 0.5f, (z0 + z1) * 0.5f, -0.01f))
                {
                    continue;
                }

                CuadroSueloMercado(
                    m, x0, z0, x1, z1, 0.0f,
                    ColorSueloManzanaMercado(i, ix, iz)
                );
            }
        }

        AgregarMaleconManzanaMercado(m, r, detalle);
    }
}


// Dique seco: pozo de piedra con un charco al fondo (el casco va encima).
static void AgregarDiqueSecoMercado(ConstructorMalla& m)
{
    const float fondo = -1.8f;
    float x0 = DIQUE_X - DIQUE_MEDIO_ANCHO;
    float x1 = DIQUE_X + DIQUE_MEDIO_ANCHO;
    float z0 = DIQUE_Z - DIQUE_MEDIO_LARGO;
    float z1 = DIQUE_Z + DIQUE_MEDIO_LARGO;

    // Escalones de piedra en las paredes.
    for (int e = 0; e < 3; e++)
    {
        float y0 = fondo + e * 0.6f;
        float y1 = y0 + 0.6f;
        float entra = (2 - e) * 0.25f;
        Color c = OscurecerMercado(PIEDRA_MALECON, 0.72f + 0.08f * e);

        ParedMercado(m, x0 + entra, z0 + entra, x1 - entra, z0 + entra, y0, y1, c);
        ParedMercado(m, x0 + entra, z1 - entra, x1 - entra, z1 - entra, y0, y1, c);
        ParedMercado(m, x0 + entra, z0 + entra, x0 + entra, z1 - entra, y0, y1, c);
        ParedMercado(m, x1 - entra, z0 + entra, x1 - entra, z1 - entra, y0, y1, c);

        if (e > 0)
        {
            float ex = entra + 0.25f;
            Color huella = OscurecerMercado(PIEDRA_MALECON, 0.86f);

            CuadroSueloMercado(m, x0 + entra, z0 + entra, x1 - entra, z0 + ex, y0, huella);
            CuadroSueloMercado(m, x0 + entra, z1 - ex, x1 - entra, z1 - entra, y0, huella);
            CuadroSueloMercado(m, x0 + entra, z0 + ex, x0 + ex, z1 - ex, y0, huella);
            CuadroSueloMercado(m, x1 - ex, z0 + ex, x1 - entra, z1 - ex, y0, huella);
        }
    }

    CuadroSueloMercado(m, x0 + 0.5f, z0 + 0.5f, x1 - 0.5f, z1 - 0.5f, fondo, Color{ 92, 88, 80, 255 });
    CuadroSueloMercado(m, x0 + 0.9f, z0 + 1.2f, x1 - 0.9f, z1 - 1.6f, fondo + 0.04f, Color{ 52, 104, 112, 255 });

    // Cordon alrededor del borde.
    m.Caja(Vector3{ DIQUE_X, 0.05f, z0 - 0.18f }, Vector3{ x1 - x0 + 0.72f, 0.10f, 0.36f }, PIEDRA_CLARA_MERCADO);
    m.Caja(Vector3{ DIQUE_X, 0.05f, z1 + 0.18f }, Vector3{ x1 - x0 + 0.72f, 0.10f, 0.36f }, PIEDRA_CLARA_MERCADO);
    m.Caja(Vector3{ x0 - 0.18f, 0.05f, DIQUE_Z }, Vector3{ 0.36f, 0.10f, z1 - z0 }, PIEDRA_CLARA_MERCADO);
    m.Caja(Vector3{ x1 + 0.18f, 0.05f, DIQUE_Z }, Vector3{ 0.36f, 0.10f, z1 - z0 }, PIEDRA_CLARA_MERCADO);
}


// Isla de la plaza: muro redondo y empedrado en anillos.
static void AgregarIslaPlazaMercado(ConstructorMalla& m, NivelCalidadGrafica nivel)
{
    const int lados = nivel == CALIDAD_BAJA ? 24 : 40;
    const float r = RADIO_ISLA_MERCADO;

    m.Cilindro(
        Vector3{ PLAZA_CX, -1.4f, PLAZA_CZ },
        Vector3{ PLAZA_CX, 0.0f, PLAZA_CZ },
        r + 0.12f, r, lados,
        PIEDRA_MALECON
    );

    // Algas en la linea del agua.
    m.Cilindro(
        Vector3{ PLAZA_CX, NIVEL_AGUA_MERCADO - 0.05f, PLAZA_CZ },
        Vector3{ PLAZA_CX, NIVEL_AGUA_MERCADO + 0.10f, PLAZA_CZ },
        r + 0.14f, r + 0.13f, lados,
        Color{ 88, 104, 78, 255 }
    );

    int anillos = nivel == CALIDAD_BAJA ? 3 : 7;

    for (int a = 0; a < anillos; a++)
    {
        float r0 = (r - 0.3f) * a / anillos;
        float r1 = (r - 0.3f) * (a + 1) / anillos;
        int segmentos = 8 + a * (nivel == CALIDAD_BAJA ? 6 : 5);

        for (int s = 0; s < segmentos; s++)
        {
            float a0 = 6.2831853f * s / segmentos;
            float a1 = 6.2831853f * (s + 1) / segmentos;

            Color c =
                ((s + a) % 2 == 0)
                ? BALDOSA_PLAZA
                : OscurecerMercado(BALDOSA_PLAZA, 0.90f);

            Vector3 p00 = { PLAZA_CX + std::sin(a0) * r0, 0.01f, PLAZA_CZ + std::cos(a0) * r0 };
            Vector3 p01 = { PLAZA_CX + std::sin(a0) * r1, 0.01f, PLAZA_CZ + std::cos(a0) * r1 };
            Vector3 p10 = { PLAZA_CX + std::sin(a1) * r0, 0.01f, PLAZA_CZ + std::cos(a1) * r0 };
            Vector3 p11 = { PLAZA_CX + std::sin(a1) * r1, 0.01f, PLAZA_CZ + std::cos(a1) * r1 };

            m.TrianguloDobleCara(p00, p01, p11, c);
            m.TrianguloDobleCara(p00, p11, p10, c);
        }
    }

    // Cordon del borde.
    for (int s = 0; s < lados; s++)
    {
        float a0 = 6.2831853f * s / lados;
        float a1 = 6.2831853f * (s + 1) / lados;

        Vector3 p00 = { PLAZA_CX + std::sin(a0) * (r - 0.32f), 0.05f, PLAZA_CZ + std::cos(a0) * (r - 0.32f) };
        Vector3 p01 = { PLAZA_CX + std::sin(a0) * (r + 0.06f), 0.05f, PLAZA_CZ + std::cos(a0) * (r + 0.06f) };
        Vector3 p10 = { PLAZA_CX + std::sin(a1) * (r - 0.32f), 0.05f, PLAZA_CZ + std::cos(a1) * (r - 0.32f) };
        Vector3 p11 = { PLAZA_CX + std::sin(a1) * (r + 0.06f), 0.05f, PLAZA_CZ + std::cos(a1) * (r + 0.06f) };

        m.TrianguloDobleCara(p00, p01, p11, PIEDRA_CLARA_MERCADO);
        m.TrianguloDobleCara(p00, p11, p10, PIEDRA_CLARA_MERCADO);
    }
}


// Distancia al sendero del lazo del faro (con sus dos enlaces al muelle).
static float DistanciaSenderoFaroMercado(float x, float z)
{
    Vector3 anterior =
        Vector3{ ANILLO_X, 0.0f, ANILLO_Z_SUR - PASO_ANILLO * 4.0f };

    float distancia = 1.0e9f;

    for (int k = 0; k <= CASILLAS_LAZO; k++)
    {
        Vector3 siguiente =
            k < CASILLAS_LAZO
            ? Vector3{ POSICIONES_FARO_MERCADO[k].x, 0.0f, POSICIONES_FARO_MERCADO[k].y }
            : Vector3{ ANILLO_X - PASO_ANILLO, 0.0f, ANILLO_Z_NORTE };

        float d = DistanciaSegmentoMercado(x, z, anterior, siguiente);

        if (d < distancia) distancia = d;

        anterior = siguiente;
    }

    return distancia;
}


// Relieve de la Isla del Faro: plano junto al sendero, un lomo de pasto en
// el medio, playa en la orilla y el penon del faro en la punta.
static float AlturaIslaFaroMercado(float x, float z)
{
    float d = DistanciaIslaFaroMercado(x, z);

    if (d > 0.0f)
    {
        return NIVEL_AGUA_MERCADO - 0.30f - std::fmin(d, 3.0f) * 0.4f;
    }

    float adentro = -d;
    float bajo = NIVEL_AGUA_MERCADO - 0.30f;
    float orilla = bajo + (0.0f - bajo) * Suave01Mercado(adentro / 1.3f);

    float libre = Suave01Mercado((DistanciaSenderoFaroMercado(x, z) - 2.0f) / 2.0f);
    float lomo = 1.5f * Suave01Mercado((adentro - 1.0f) / 3.0f) * libre;
    float ondas = 0.22f * std::sin(x * 0.9f + 0.4f) * std::sin(z * 0.75f + 1.3f) * libre;

    float df = std::sqrt((x - FARO_X) * (x - FARO_X) + (z - FARO_Z) * (z - FARO_Z));
    float penon = 0.9f * (1.0f - Suave01Mercado((df - 1.6f) / 2.4f)) * Suave01Mercado(adentro / 1.0f);

    return orilla + lomo + ondas * Suave01Mercado(adentro / 2.0f) + penon;
}


static Color ColorIslaFaroMercado(float x, float z, float h, float pendiente)
{
    float v = Hash2Mercado((int)(x * 2.0f) + 400, (int)(z * 2.0f) + 77);

    Color arena = Color{ 214, 188, 138, 255 };
    Color pasto = Color{ 124, 148, 76, 255 };
    Color roca = Color{ 146, 136, 126, 255 };

    Color c;

    if (h < 0.03f)
    {
        c = MezclaMercado(Color{ 168, 150, 116, 255 }, arena, Suave01Mercado((h - NIVEL_AGUA_MERCADO) / 0.25f));
    }
    else
    {
        c = MezclaMercado(arena, pasto, Suave01Mercado(h / 0.25f));
    }

    if (pendiente > 0.55f || h > 1.25f)
    {
        c = MezclaMercado(c, roca, 0.75f);
    }

    return OscurecerMercado(c, 0.93f + 0.12f * v);
}


static void AgregarIslaFaroMercado(ConstructorMalla& m, NivelCalidadGrafica nivel)
{
    const float x0 = 4.0f;
    const float x1 = 30.0f;
    const float z0 = -25.5f;
    const float z1 = -1.0f;
    const float paso = nivel == CALIDAD_ALTA ? 0.5f : (nivel == CALIDAD_MEDIA ? 0.75f : 1.0f);

    int nx = (int)((x1 - x0) / paso);
    int nz = (int)((z1 - z0) / paso);

    static float alturas[80][80];

    if (nx > 79) nx = 79;
    if (nz > 79) nz = 79;

    for (int ix = 0; ix <= nx; ix++)
    {
        for (int iz = 0; iz <= nz; iz++)
        {
            alturas[ix][iz] = AlturaIslaFaroMercado(x0 + ix * paso, z0 + iz * paso);
        }
    }

    const float sumergido = NIVEL_AGUA_MERCADO - 0.06f;

    for (int ix = 0; ix < nx; ix++)
    {
        for (int iz = 0; iz < nz; iz++)
        {
            float ax = x0 + ix * paso;
            float az = z0 + iz * paso;
            float bx = ax + paso;
            float bz = az + paso;

            float h00 = alturas[ix][iz];
            float h01 = alturas[ix][iz + 1];
            float h11 = alturas[ix + 1][iz + 1];
            float h10 = alturas[ix + 1][iz];

            if (h00 < sumergido && h01 < sumergido && h11 < sumergido && h10 < sumergido)
            {
                continue;
            }

            float hMedia = (h00 + h01 + h11 + h10) * 0.25f;
            float pendiente =
                (std::fabs(h00 - h11) + std::fabs(h01 - h10)) / (paso * 1.4142f);

            Color c =
                ColorIslaFaroMercado(ax + paso * 0.5f, az + paso * 0.5f, hMedia, pendiente);

            m.Triangulo(Vector3{ ax, h00, az }, Vector3{ ax, h01, bz }, Vector3{ bx, h11, bz }, c);
            m.Triangulo(Vector3{ ax, h00, az }, Vector3{ bx, h11, bz }, Vector3{ bx, h10, az }, OscurecerMercado(c, 0.98f));
        }
    }
}


static void ConstruirMallaTierraMercado(NivelCalidadGrafica nivel)
{
    ConstructorMalla& m = constructorMercado;
    m.Limpiar();
    m.reduccionDetalle = nivel == CALIDAD_ALTA ? 0 : 1;

    AgregarManzanasMercado(m, nivel);
    AgregarDiqueSecoMercado(m);
    AgregarIslaPlazaMercado(m, nivel);
    AgregarIslaFaroMercado(m, nivel);

    IluminarMallaMercado(m, 0);
    ActualizarMallaGpu(mallaTierraMercado, m);
}


//==================================================
// SENDEROS: MUELLES, PUENTES Y PLAZAS
//==================================================
//
// Se hornea a partir del grafo de casillas: una cinta de 2.7 por cada
// conexion (tablones cruzados en los muelles, baldosas en la plaza),
// discos en los nodos y una plaza mas ancha donde el camino se bifurca o se
// junta. Donde debajo hay agua lleva pilotes y un faldon de madera; los
// puentes llevan baranda y las gondolas, una cuerda con boyas.

struct MaterialSenderoMercado
{
    Color tablaA;
    Color tablaB;
    Color borde;
    bool piedra;
};


static MaterialSenderoMercado MaterialSenderoZonaMercado(int zona)
{
    switch (zona)
    {
        case ZONA_PLAZA_MERCADO:
            return { BALDOSA_PLAZA, Color{ 196, 174, 142, 255 }, Color{ 150, 132, 110, 255 }, true };
        case ZONA_ISLA_FARO:
            return { Color{ 158, 144, 124, 255 }, Color{ 176, 162, 140, 255 }, Color{ 96, 86, 74, 255 }, false };
        case ZONA_ASTILLERO:
            return { Color{ 128, 94, 64, 255 }, Color{ 146, 108, 74, 255 }, Color{ 74, 54, 40, 255 }, false };
        default:
            break;
    }

    return { Color{ 162, 114, 72, 255 }, Color{ 182, 132, 84, 255 }, MADERA_OSCURA_MERCADO, false };
}


struct SeccionSenderoMercado
{
    float x, z;
    float nx, nz;
};


static bool ConexionInversaMercado(const Tablero& t, int a, int b)
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


static bool EsGondolaMercado(int indice)
{
    return indice == nodoMercado.gondolaEste || indice == nodoMercado.gondolaOeste;
}


static bool EsPuenteMercado(int indice)
{
    return indice == nodoMercado.puenteSur || indice == nodoMercado.puenteNorte;
}


static void CuadroSeccionMercado(
    ConstructorMalla& m,
    const SeccionSenderoMercado& s0,
    const SeccionSenderoMercado& s1,
    float lat0, float lat1,
    float y,
    Color color
)
{
    Vector3 a = { s0.x + s0.nx * lat0, y, s0.z + s0.nz * lat0 };
    Vector3 b = { s0.x + s0.nx * lat1, y, s0.z + s0.nz * lat1 };
    Vector3 c = { s1.x + s1.nx * lat0, y, s1.z + s1.nz * lat0 };
    Vector3 d = { s1.x + s1.nx * lat1, y, s1.z + s1.nz * lat1 };

    m.TrianguloDobleCara(a, b, c, color);
    m.TrianguloDobleCara(b, d, c, color);
}


static void DiscoMercado(
    ConstructorMalla& m,
    float x, float y, float z,
    float radio,
    Color color
)
{
    const int LADOS = 20;

    for (int i = 0; i < LADOS; i++)
    {
        float a0 = 6.2831853f * i / LADOS;
        float a1 = 6.2831853f * (i + 1) / LADOS;

        m.TrianguloDobleCara(
            Vector3{ x, y, z },
            Vector3{ x + std::cos(a0) * radio, y, z + std::sin(a0) * radio },
            Vector3{ x + std::cos(a1) * radio, y, z + std::sin(a1) * radio },
            color
        );
    }
}


static void AnilloMercado(
    ConstructorMalla& m,
    float x, float y, float z,
    float radioInterior, float radioExterior,
    Color a, Color b, int piezas
)
{
    for (int i = 0; i < piezas; i++)
    {
        float a0 = 6.2831853f * i / piezas;
        float a1 = 6.2831853f * (i + 0.9f) / piezas;
        Color c = (i % 2 == 0) ? a : b;

        Vector3 p00 = { x + std::cos(a0) * radioInterior, y, z + std::sin(a0) * radioInterior };
        Vector3 p01 = { x + std::cos(a0) * radioExterior, y, z + std::sin(a0) * radioExterior };
        Vector3 p10 = { x + std::cos(a1) * radioInterior, y, z + std::sin(a1) * radioInterior };
        Vector3 p11 = { x + std::cos(a1) * radioExterior, y, z + std::sin(a1) * radioExterior };

        m.TrianguloDobleCara(p00, p01, p10, c);
        m.TrianguloDobleCara(p01, p11, p10, c);
    }
}


static void PiloteMercado(ConstructorMalla& m, float x, float z, float alto)
{
    m.Cilindro(
        Vector3{ x, NIVEL_AGUA_MERCADO - 0.5f, z },
        Vector3{ x, alto, z },
        0.10f, 0.09f, 6,
        MADERA_OSCURA_MERCADO
    );
}


// Cuerda con boyas rojas y blancas que marca el paso de la gondola.
static void AgregarBoyasGondolaMercado(
    ConstructorMalla& m,
    Vector3 a,
    Vector3 b,
    bool detalle
)
{
    float dx = b.x - a.x;
    float dz = b.z - a.z;
    float largo = std::sqrt(dx * dx + dz * dz);

    if (largo < 0.01f)
    {
        return;
    }

    float nx = -dz / largo;
    float nz = dx / largo;

    for (int lado = -1; lado <= 1; lado += 2)
    {
        float ox = nx * 1.05f * lado;
        float oz = nz * 1.05f * lado;

        m.Cilindro(
            Vector3{ a.x + ox, NIVEL_AGUA_MERCADO + 0.06f, a.z + oz },
            Vector3{ b.x + ox, NIVEL_AGUA_MERCADO + 0.06f, b.z + oz },
            0.025f, 0.025f, 4,
            Color{ 226, 210, 170, 255 }
        );

        if (!detalle)
        {
            continue;
        }

        int boyas = (int)(largo / 0.55f);

        for (int k = 1; k < boyas; k++)
        {
            float t = (float)k / boyas;

            m.Esfera(
                Vector3{ a.x + dx * t + ox, NIVEL_AGUA_MERCADO + 0.06f, a.z + dz * t + oz },
                0.11f, 5, 6,
                (k % 2 == 0)
                    ? Color{ 226, 64, 52, 255 }
                    : Color{ 244, 240, 230, 255 }
            );
        }
    }
}


static void AgregarCintaSenderoMercado(
    ConstructorMalla& m,
    const Tablero& t,
    int i,
    int j,
    NivelCalidadGrafica nivel
)
{
    const Casilla& a = t.casillas[i];
    const Casilla& b = t.casillas[j];

    float dx = b.posicion.x - a.posicion.x;
    float dz = b.posicion.z - a.posicion.z;
    float largo = std::sqrt(dx * dx + dz * dz);

    if (largo < 0.01f)
    {
        return;
    }

    if (EsGondolaMercado(i) || EsGondolaMercado(j))
    {
        AgregarBoyasGondolaMercado(m, a.posicion, b.posicion, nivel != CALIDAD_BAJA);
        return;
    }

    bool plaza = a.zona == ZONA_PLAZA_MERCADO && b.zona == ZONA_PLAZA_MERCADO;
    bool puente = EsPuenteMercado(i) || EsPuenteMercado(j);

    MaterialSenderoMercado ma =
        MaterialSenderoZonaMercado(plaza ? ZONA_PLAZA_MERCADO : (a.zona == ZONA_PLAZA_MERCADO ? b.zona : a.zona));

    const float pasoTabla = 0.40f;
    int n = (int)std::ceil(largo / pasoTabla);

    if (n > 120) n = 120;

    float px = -dz / largo;
    float pz = dx / largo;

    SeccionSenderoMercado sec[121];
    bool agua[121][2];

    for (int s = 0; s <= n; s++)
    {
        float u = (float)s / n;

        sec[s].x = a.posicion.x + dx * u;
        sec[s].z = a.posicion.z + dz * u;
        sec[s].nx = px;
        sec[s].nz = pz;

        for (int lado = 0; lado < 2; lado++)
        {
            float l = (lado == 0 ? -1.0f : 1.0f) * (SENDERO_SEMIANCHO_MERCADO + 0.2f);
            agua[s][lado] = !EsTierraMercado(sec[s].x + px * l, sec[s].z + pz * l);
        }
    }

    const float limite = SENDERO_SEMIANCHO_MERCADO + SENDERO_BORDE_MERCADO;

    for (int s = 0; s < n; s++)
    {
        // Borde: vigas oscuras (madera) o cordon (piedra).
        for (int lado = -1; lado <= 1; lado += 2)
        {
            CuadroSeccionMercado(
                m, sec[s], sec[s + 1],
                lado * SENDERO_SEMIANCHO_MERCADO * 0.92f, lado * limite,
                LIFT_BORDE_MERCADO, ma.borde
            );
        }

        if (ma.piedra)
        {
            // Baldosas en dos carriles con tono propio por bloque.
            int bloque = (int)(s * pasoTabla / 0.9f);

            for (int lado = 0; lado < 2; lado++)
            {
                float v = Hash2Mercado(i * 131 + bloque * 7 + 1, j * 17 + lado);
                Color c = OscurecerMercado(v > 0.5f ? ma.tablaA : ma.tablaB, 0.95f + 0.1f * v);

                CuadroSeccionMercado(
                    m, sec[s], sec[s + 1],
                    lado == 0 ? -SENDERO_SEMIANCHO_MERCADO : 0.02f,
                    lado == 0 ? -0.02f : SENDERO_SEMIANCHO_MERCADO,
                    LIFT_TABLA_MERCADO, c
                );
            }
        }
        else
        {
            // Tablones cruzados de tres tonos.
            float v = Hash2Mercado(i * 97 + s, j * 31 + 7);
            Color c = MezclaMercado(ma.tablaA, ma.tablaB, v);

            if (v < 0.15f)
            {
                c = OscurecerMercado(c, 0.86f);
            }

            CuadroSeccionMercado(
                m, sec[s], sec[s + 1],
                -SENDERO_SEMIANCHO_MERCADO, SENDERO_SEMIANCHO_MERCADO,
                LIFT_TABLA_MERCADO, c
            );
        }

        // Faldon de madera sobre el agua.
        for (int lado = 0; lado < 2 && !ma.piedra; lado++)
        {
            if (!agua[s][lado] || !agua[s + 1][lado])
            {
                continue;
            }

            float l = (lado == 0 ? -1.0f : 1.0f) * limite;

            ParedMercado(
                m,
                sec[s].x + px * l, sec[s].z + pz * l,
                sec[s + 1].x + px * l, sec[s + 1].z + pz * l,
                NIVEL_AGUA_MERCADO + 0.02f, LIFT_BORDE_MERCADO,
                MADERA_OSCURA_MERCADO
            );
        }
    }

    // Pilotes cada ~1.6 donde hay agua debajo.
    for (int s = 2; s < n - 1; s += 4)
    {
        for (int lado = 0; lado < 2; lado++)
        {
            if (!agua[s][lado])
            {
                continue;
            }

            float l = (lado == 0 ? -1.0f : 1.0f) * (limite - 0.08f);
            PiloteMercado(m, sec[s].x + px * l, sec[s].z + pz * l, LIFT_TABLA_MERCADO + 0.12f);
        }
    }

    // Barandas de los puentes (sin entrar en las losas).
    if (puente)
    {
        float tramo = largo - 2.2f;

        for (int lado = -1; lado <= 1 && tramo > 0.3f; lado += 2)
        {
            float l = lado * (limite - 0.10f);
            float cx = (a.posicion.x + b.posicion.x) * 0.5f + px * l;
            float cz = (a.posicion.z + b.posicion.z) * 0.5f + pz * l;
            float ux = dx / largo;
            float uz = dz / largo;
            float y = LIFT_TABLA_MERCADO;

            m.Cilindro(
                Vector3{ cx - ux * tramo * 0.5f, y + 0.62f, cz - uz * tramo * 0.5f },
                Vector3{ cx + ux * tramo * 0.5f, y + 0.62f, cz + uz * tramo * 0.5f },
                0.05f, 0.05f, 5,
                MADERA_CLARA_MERCADO
            );

            int postes = 1 + (int)(tramo / 0.75f);

            for (int k = 0; k <= postes; k++)
            {
                float f = -0.5f + (float)k / postes;

                m.Caja(
                    Vector3{ cx + ux * tramo * f, y + 0.31f, cz + uz * tramo * f },
                    Vector3{ 0.09f, 0.62f, 0.09f },
                    MADERA_OSCURA_MERCADO
                );
            }
        }
    }
}


static void ConstruirMallaSenderosMercado(NivelCalidadGrafica nivel)
{
    ConstructorMalla& m = constructorMercado;
    m.Limpiar();
    m.reduccionDetalle = nivel == CALIDAD_ALTA ? 0 : 1;

    const Tablero& t = TableroReferenciaMercado();

    // --- Cintas entre casillas conectadas (una por par) ---
    for (int i = 0; i < t.cantidadCasillas; i++)
    {
        const Casilla& a = t.casillas[i];

        for (int k = 0; k < a.cantidadConexiones; k++)
        {
            int j = a.conexiones[k].destino;

            if (j < 0 || j >= t.cantidadCasillas || (j < i && ConexionInversaMercado(t, i, j)))
            {
                continue;
            }

            AgregarCintaSenderoMercado(m, t, i, j, nivel);
        }
    }

    // --- Nodos: disco de union y plaza mas ancha en los cruces ---
    for (int i = 0; i < t.cantidadCasillas; i++)
    {
        const Casilla& c = t.casillas[i];

        if (EsGondolaMercado(i))
        {
            continue;
        }

        MaterialSenderoMercado ms = MaterialSenderoZonaMercado(c.zona);
        bool cruce = GradoCasillaMercado(t, i) >= 3;
        float semi = cruce ? SENDERO_PLAZA_MERCADO : SENDERO_SEMIANCHO_MERCADO + 0.1f;

        DiscoMercado(m, c.posicion.x, LIFT_BORDE_NODO_MERCADO, c.posicion.z, semi + SENDERO_BORDE_MERCADO, ms.borde);
        DiscoMercado(m, c.posicion.x, LIFT_TABLA_NODO_MERCADO, c.posicion.z, semi, ms.tablaA);

        AnilloMercado(
            m, c.posicion.x, LIFT_DETALLE_MERCADO, c.posicion.z,
            semi - 0.42f, semi - 0.04f,
            ms.tablaB, OscurecerMercado(ms.tablaA, 0.9f),
            cruce ? 18 : 14
        );

        // Pilotes bajo los nodos que caen sobre el agua.
        for (int p = 0; p < 4; p++)
        {
            float ang = (45.0f + 90.0f * p) * DEG2RAD;
            float px = c.posicion.x + std::cos(ang) * (semi + 0.05f);
            float pz = c.posicion.z + std::sin(ang) * (semi + 0.05f);

            if (!EsTierraMercado(px, pz))
            {
                PiloteMercado(m, px, pz, LIFT_TABLA_NODO_MERCADO + 0.10f);
            }
        }
    }

    IluminarMallaMercado(m, 0);
    ActualizarMallaGpu(mallaSenderosMercado, m);
}


//==================================================
// AGUA ANIMADA (oleaje y reflejos del atardecer)
//==================================================

static const int MAX_CELDAS_OLEAJE_MERCADO = 400;

static Vector2 celdasOleajeMercado[MAX_CELDAS_OLEAJE_MERCADO];
static int cantidadCeldasOleajeMercado = 0;
static bool oleajeListoMercado = false;


// Celdas de agua libre (ni tierra ni muelle) donde se dibujan las olas.
static void PrepararOleajeMercado()
{
    const Tablero& t = TableroReferenciaMercado();

    cantidadCeldasOleajeMercado = 0;

    for (int gx = -44; gx <= 40; gx += 4)
    {
        for (int gz = -40; gz <= 24; gz += 4)
        {
            float x = gx + 1.3f * std::sin(gz * 0.7f);
            float z = gz + 0.9f * std::sin(gx * 0.9f);

            if (
                EsTierraMercado(x, z) ||
                EsTierraMercado(x + 1.0f, z) ||
                EsTierraMercado(x - 1.0f, z) ||
                DistanciaSenderoMercado(t, x, z) < 2.2f
            )
            {
                continue;
            }

            if (cantidadCeldasOleajeMercado < MAX_CELDAS_OLEAJE_MERCADO)
            {
                celdasOleajeMercado[cantidadCeldasOleajeMercado++] = Vector2{ x, z };
            }
        }
    }

    oleajeListoMercado = true;
}


static void CuadroAguaMercado(
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


static void DibujarOleajeMercado(float tiempo)
{
    NivelCalidadGrafica efectos = CalidadEfectos();

    if (efectos == CALIDAD_BAJA)
    {
        return;
    }

    if (!oleajeListoMercado)
    {
        PrepararOleajeMercado();
    }

    int paso = efectos == CALIDAD_ALTA ? 1 : 2;

    rlDisableBackfaceCulling();

    for (int i = 0; i < cantidadCeldasOleajeMercado; i += paso)
    {
        Vector2 c = celdasOleajeMercado[i];

        float onda = 0.5f + 0.5f * std::sin(tiempo * 0.9f + c.x * 0.55f + c.y * 0.8f);

        if (onda < 0.45f)
        {
            continue;
        }

        float deriva = 0.9f * std::sin(tiempo * 0.5f + c.y * 0.7f);

        // Lejos hacia el norte, el sol pinta las crestas de rosa.
        bool ocaso = c.y < -18.0f;

        CuadroAguaMercado(
            c.x + deriva,
            NIVEL_AGUA_MERCADO + 0.012f,
            c.y,
            0.95f,
            0.11f,
            ocaso
                ? Color{ 255, 206, 170, (unsigned char)(90.0f * onda) }
                : Color{ 236, 250, 255, (unsigned char)(60.0f * onda) }
        );
    }

    rlEnableBackfaceCulling();
}


//==================================================
// ESCENARIO: PIEZAS (casas, faroles, barcas y props)
//==================================================
//
// Cada pieza es una funcion que emite su geometria al constructor (con la
// transformacion de la pieza) y se reutiliza muchas veces en la misma malla.

static MallaGpu mallaEdificiosMercado;
static MallaGpu mallaPropsMercado;

// Huellas ocupadas por casas, landmarks y props grandes (las usa la
// colocacion de la decoracion para no superponer nada).
static const int MAX_OCUPADOS_MERCADO = 400;
static RectanguloMercado ocupadosMercado[MAX_OCUPADOS_MERCADO];
static int cantidadOcupadosMercado = 0;

// Faroles y ventanas del escenario (el dibujo animado les pone halo).
static const int MAX_FAROLES_MERCADO = 64;
static Vector3 farolesMercado[MAX_FAROLES_MERCADO];
static int cantidadFarolesMercado = 0;


static void OcuparMercado(float cx, float cz, float medioX, float medioZ)
{
    if (cantidadOcupadosMercado < MAX_OCUPADOS_MERCADO)
    {
        ocupadosMercado[cantidadOcupadosMercado++] =
            RectanguloMercado{ cx - medioX, cx + medioX, cz - medioZ, cz + medioZ };
    }
}


static bool LibreDeOcupadosMercado(float x, float z, float margen)
{
    for (int i = 0; i < cantidadOcupadosMercado; i++)
    {
        if (DentroRectanguloMercado(ocupadosMercado[i], x, z, margen))
        {
            return false;
        }
    }

    return true;
}


// Pieza giradas: frente de la pieza = +Z local.
static void EstablecerPiezaMercado(
    ConstructorMalla& m,
    float x, float y, float z,
    float giro,
    float escala = 1.0f
)
{
    TransformacionMalla tr;
    tr.tx = x;
    tr.ty = y;
    tr.tz = z;
    tr.giroY = giro;
    tr.sx = escala;
    tr.sy = escala;
    tr.sz = escala;

    m.EstablecerTransformacion(tr);
}


// Cuadro de fachada que mira a +Z local (una sola cara).
static void FachadaZMercado(
    ConstructorMalla& m,
    float x0, float y0, float x1, float y1,
    float z,
    Color color
)
{
    m.Triangulo(Vector3{ x0, y0, z }, Vector3{ x1, y0, z }, Vector3{ x1, y1, z }, color);
    m.Triangulo(Vector3{ x0, y0, z }, Vector3{ x1, y1, z }, Vector3{ x0, y1, z }, color);
}


// Cuadro de fachada que mira a +X (lado = 1) o -X (lado = -1) local.
static void FachadaXMercado(
    ConstructorMalla& m,
    float z0, float y0, float z1, float y1,
    float x,
    int lado,
    Color color
)
{
    if (lado > 0)
    {
        m.Triangulo(Vector3{ x, y0, z1 }, Vector3{ x, y0, z0 }, Vector3{ x, y1, z0 }, color);
        m.Triangulo(Vector3{ x, y0, z1 }, Vector3{ x, y1, z0 }, Vector3{ x, y1, z1 }, color);
    }
    else
    {
        m.Triangulo(Vector3{ x, y0, z0 }, Vector3{ x, y0, z1 }, Vector3{ x, y1, z1 }, color);
        m.Triangulo(Vector3{ x, y0, z0 }, Vector3{ x, y1, z1 }, Vector3{ x, y1, z0 }, color);
    }
}


// Techo a dos aguas con cumbrera paralela a X (local), con hastiales.
static void TechoDosAguasMercado(
    ConstructorMalla& m,
    float ancho, float fondo,
    float y0, float alto,
    Color techo, Color pared
)
{
    float y1 = y0 + alto;
    float x0 = -ancho * 0.5f - 0.14f;
    float x1 = ancho * 0.5f + 0.14f;
    float z0 = fondo * 0.5f + 0.20f;

    Color techoOscuro = OscurecerMercado(techo, 0.86f);

    m.TrianguloDobleCara({ x0, y0, z0 }, { x1, y0, z0 }, { x1, y1, 0.0f }, techo);
    m.TrianguloDobleCara({ x0, y0, z0 }, { x1, y1, 0.0f }, { x0, y1, 0.0f }, techo);
    m.TrianguloDobleCara({ x1, y0, -z0 }, { x0, y0, -z0 }, { x0, y1, 0.0f }, techoOscuro);
    m.TrianguloDobleCara({ x1, y0, -z0 }, { x0, y1, 0.0f }, { x1, y1, 0.0f }, techoOscuro);

    for (int lado = -1; lado <= 1; lado += 2)
    {
        float x = lado * ancho * 0.5f;

        m.TrianguloDobleCara(
            { x, y0, fondo * 0.5f },
            { x, y0, -fondo * 0.5f },
            { x, y1 - 0.05f, 0.0f },
            pared
        );
    }
}


static const Color PAREDES_MERCADO[] =
{
    Color{ 236, 178, 120, 255 },
    Color{ 222, 120, 104, 255 },
    Color{ 246, 214, 140, 255 },
    Color{ 132, 190, 186, 255 },
    Color{ 236, 156, 170, 255 },
    Color{ 168, 196, 120, 255 },
    Color{ 240, 226, 200, 255 },
    Color{ 206, 150, 196, 255 }
};

static const Color TECHOS_MERCADO[] =
{
    Color{ 176, 74, 52, 255 },
    Color{ 150, 84, 60, 255 },
    Color{ 196, 104, 64, 255 },
    Color{ 70, 104, 120, 255 }
};

static const Color POSTIGOS_MERCADO[] =
{
    Color{ 64, 120, 96, 255 },
    Color{ 60, 96, 150, 255 },
    Color{ 150, 70, 54, 255 },
    Color{ 220, 196, 120, 255 }
};


// Casa de colores (frente = +Z local). Variante (semilla): techo a dos
// aguas o terraza, balcon, toldo en la planta baja, chimenea y ventanas
// encendidas. detalle = ventanas y balcones (calidad media y alta).
static void AgregarCasaMercado(
    ConstructorMalla& m,
    float cx, float cz,
    float giro,
    float ancho, float fondo,
    int pisos,
    int semilla,
    bool detalle
)
{
    const float altoPiso = 1.05f;
    float alto = 0.35f + pisos * altoPiso;

    Color pared = PAREDES_MERCADO[semilla % 8];
    Color techo = TECHOS_MERCADO[(semilla / 3) % 4];
    Color postigo = POSTIGOS_MERCADO[(semilla / 5) % 4];

    EstablecerPiezaMercado(m, cx, 0.0f, cz, giro);

    m.Caja(Vector3{ 0.0f, alto * 0.5f, 0.0f }, Vector3{ ancho, alto, fondo }, pared);

    // Zocalo y cornisa.
    m.Caja(
        Vector3{ 0.0f, 0.22f, 0.0f },
        Vector3{ ancho + 0.06f, 0.44f, fondo + 0.06f },
        OscurecerMercado(pared, 0.72f)
    );

    m.Caja(
        Vector3{ 0.0f, alto - 0.06f, 0.0f },
        Vector3{ ancho + 0.14f, 0.12f, fondo + 0.14f },
        PIEDRA_CLARA_MERCADO
    );

    bool terraza = Hash2Mercado(semilla, 3) < 0.22f;

    if (terraza)
    {
        // Azotea con pretil y una pergola o tanque.
        m.Caja(Vector3{ 0.0f, alto + 0.16f, fondo * 0.5f }, Vector3{ ancho + 0.1f, 0.32f, 0.10f }, pared);
        m.Caja(Vector3{ 0.0f, alto + 0.16f, -fondo * 0.5f }, Vector3{ ancho + 0.1f, 0.32f, 0.10f }, pared);
        m.Caja(Vector3{ ancho * 0.5f, alto + 0.16f, 0.0f }, Vector3{ 0.10f, 0.32f, fondo }, pared);
        m.Caja(Vector3{ -ancho * 0.5f, alto + 0.16f, 0.0f }, Vector3{ 0.10f, 0.32f, fondo }, pared);

        m.Cilindro(
            Vector3{ ancho * 0.22f, alto, -fondo * 0.18f },
            Vector3{ ancho * 0.22f, alto + 0.7f, -fondo * 0.18f },
            0.32f, 0.32f, 8,
            Color{ 150, 120, 96, 255 }
        );
    }
    else
    {
        TechoDosAguasMercado(m, ancho, fondo, alto, 0.62f + 0.12f * ancho, techo, pared);
    }

    // Chimenea.
    if (Hash2Mercado(semilla, 9) < 0.6f)
    {
        float chx = (Hash2Mercado(semilla, 10) - 0.5f) * ancho * 0.6f;

        m.Caja(
            Vector3{ chx, alto + 0.75f, -fondo * 0.22f },
            Vector3{ 0.32f, 1.1f, 0.32f },
            Color{ 160, 96, 76, 255 }
        );
    }

    if (detalle)
    {
        float frente = fondo * 0.5f + 0.01f;
        int columnas = ancho > 2.7f ? 3 : 2;

        // Puerta y toldo de la planta baja.
        FachadaZMercado(m, -0.28f, 0.0f, 0.28f, 0.92f, frente, MADERA_OSCURA_MERCADO);
        FachadaZMercado(m, -0.36f, 0.92f, 0.36f, 1.0f, frente, PIEDRA_CLARA_MERCADO);

        if (Hash2Mercado(semilla, 4) < 0.45f)
        {
            Color lona = POSTIGOS_MERCADO[(semilla + 1) % 4];

            m.TrianguloDobleCara(
                { -ancho * 0.42f, 1.18f, frente },
                { ancho * 0.42f, 1.18f, frente },
                { ancho * 0.42f, 0.98f, frente + 0.55f },
                lona
            );

            m.TrianguloDobleCara(
                { -ancho * 0.42f, 1.18f, frente },
                { ancho * 0.42f, 0.98f, frente + 0.55f },
                { -ancho * 0.42f, 0.98f, frente + 0.55f },
                lona
            );
        }

        for (int p = 1; p < pisos; p++)
        {
            float y = 0.35f + p * altoPiso + 0.38f;

            for (int c = 0; c < columnas; c++)
            {
                float x = (c - (columnas - 1) * 0.5f) * (ancho / columnas);

                bool encendida = Hash2Mercado(semilla * 7 + p, c + 11) < 0.35f;

                Color vidrio =
                    encendida
                    ? EmisivoMercado(Color{ 255, 206, 128, 255 })
                    : Color{ 70, 82, 104, 255 };

                FachadaZMercado(m, x - 0.22f, y - 0.30f, x + 0.22f, y + 0.30f, frente, PIEDRA_CLARA_MERCADO);
                FachadaZMercado(m, x - 0.16f, y - 0.24f, x + 0.16f, y + 0.24f, frente + 0.01f, vidrio);

                // Postigos.
                FachadaZMercado(m, x - 0.36f, y - 0.26f, x - 0.24f, y + 0.26f, frente + 0.01f, postigo);
                FachadaZMercado(m, x + 0.24f, y - 0.26f, x + 0.36f, y + 0.26f, frente + 0.01f, postigo);
            }

            // Ventanas del costado derecho (la camara ve el lado este).
            float costado = ancho * 0.5f + 0.01f;

            FachadaXMercado(m, -0.20f, y - 0.24f, 0.20f, y + 0.24f, costado, 1, Color{ 70, 82, 104, 255 });
            FachadaXMercado(m, -0.20f, y - 0.24f, 0.20f, y + 0.24f, -costado, -1, Color{ 70, 82, 104, 255 });
        }

        // Balcon con macetas en el primer piso.
        if (pisos >= 2 && Hash2Mercado(semilla, 6) < 0.5f)
        {
            float y = 0.35f + altoPiso;

            m.Caja(Vector3{ 0.0f, y, frente + 0.25f }, Vector3{ ancho * 0.55f, 0.08f, 0.5f }, PIEDRA_CLARA_MERCADO);
            m.Caja(Vector3{ 0.0f, y + 0.30f, frente + 0.48f }, Vector3{ ancho * 0.55f, 0.05f, 0.05f }, HIERRO_MERCADO);

            for (int k = -1; k <= 1; k += 2)
            {
                m.Caja(
                    Vector3{ k * ancho * 0.18f, y + 0.12f, frente + 0.36f },
                    Vector3{ 0.22f, 0.16f, 0.18f },
                    Color{ 176, 96, 64, 255 }
                );

                m.Esfera(
                    Vector3{ k * ancho * 0.18f, y + 0.28f, frente + 0.36f },
                    0.16f, 4, 5,
                    Hash2Mercado(semilla, k + 20) < 0.5f
                        ? Color{ 226, 72, 110, 255 }
                        : Color{ 98, 150, 70, 255 }
                );
            }
        }
    }

    m.QuitarTransformacion();
}


static void AgregarFarolMercado(
    ConstructorMalla& m,
    float x,
    float y,
    float z,
    float alto
)
{
    m.Cilindro(
        Vector3{ x, y, z },
        Vector3{ x, y + 0.18f, z },
        0.12f, 0.09f, 6,
        HIERRO_MERCADO
    );

    m.Cilindro(
        Vector3{ x, y, z },
        Vector3{ x, y + alto, z },
        0.05f, 0.04f, 6,
        HIERRO_MERCADO
    );

    m.Caja(
        Vector3{ x, y + alto + 0.13f, z },
        Vector3{ 0.24f, 0.26f, 0.24f },
        EmisivoMercado(LUZ_FAROL_MERCADO)
    );

    m.Cilindro(
        Vector3{ x, y + alto + 0.26f, z },
        Vector3{ x, y + alto + 0.44f, z },
        0.20f, 0.02f, 4,
        HIERRO_MERCADO
    );

    if (cantidadFarolesMercado < MAX_FAROLES_MERCADO)
    {
        farolesMercado[cantidadFarolesMercado++] = Vector3{ x, y + alto + 0.13f, z };
    }
}


static void AgregarBarrilMercado(ConstructorMalla& m, float x, float y, float z, float v)
{
    Color madera = OscurecerMercado(MADERA_MERCADO, 0.85f + 0.3f * v);

    m.Cilindro(Vector3{ x, y, z }, Vector3{ x, y + 0.56f, z }, 0.22f, 0.22f, 8, madera);
    m.Cilindro(Vector3{ x, y + 0.12f, z }, Vector3{ x, y + 0.16f, z }, 0.235f, 0.235f, 8, HIERRO_MERCADO);
    m.Cilindro(Vector3{ x, y + 0.42f, z }, Vector3{ x, y + 0.46f, z }, 0.235f, 0.235f, 8, HIERRO_MERCADO);
}


static void AgregarCajonMercado(ConstructorMalla& m, float x, float y, float z, float giro, float v)
{
    EstablecerPiezaMercado(m, x, y, z, giro);

    float lado = 0.42f + 0.16f * v;
    Color madera = OscurecerMercado(MADERA_CLARA_MERCADO, 0.82f + 0.25f * v);

    m.Caja(Vector3{ 0.0f, lado * 0.5f, 0.0f }, Vector3{ lado, lado, lado }, madera);
    m.Caja(Vector3{ 0.0f, lado * 0.5f, 0.0f }, Vector3{ lado + 0.02f, 0.06f, lado + 0.02f }, MADERA_OSCURA_MERCADO);

    // A veces otro cajon encima, o fruta a la vista.
    if (v > 0.6f)
    {
        m.Caja(Vector3{ 0.04f, lado + lado * 0.4f, -0.03f }, Vector3{ lado * 0.8f, lado * 0.8f, lado * 0.8f }, madera);
    }
    else if (v < 0.3f)
    {
        m.Caja(
            Vector3{ 0.0f, lado + 0.02f, 0.0f },
            Vector3{ lado * 0.8f, 0.06f, lado * 0.8f },
            v < 0.15f ? Color{ 236, 120, 48, 255 } : Color{ 120, 176, 64, 255 }
        );
    }

    m.QuitarTransformacion();
}


static void AgregarSacosMercado(ConstructorMalla& m, float x, float y, float z, float giro, float v)
{
    EstablecerPiezaMercado(m, x, y, z, giro);

    const Color tela = Color{ 206, 180, 132, 255 };
    const Color especias[3] =
    {
        Color{ 214, 120, 40, 255 },
        Color{ 196, 64, 40, 255 },
        Color{ 236, 196, 64, 255 }
    };

    for (int k = 0; k < 3; k++)
    {
        float sx = (k - 1) * 0.36f;

        m.Cilindro(Vector3{ sx, 0.0f, 0.0f }, Vector3{ sx, 0.36f, 0.0f }, 0.18f, 0.15f, 6, tela);
        m.Cilindro(
            Vector3{ sx, 0.36f, 0.0f }, Vector3{ sx, 0.40f, 0.0f },
            0.14f, 0.10f, 6,
            especias[(k + (int)(v * 3.0f)) % 3]
        );
    }

    m.QuitarTransformacion();
}


static void AgregarMacetaMercado(ConstructorMalla& m, float x, float y, float z, float v, bool arbolito)
{
    m.Cilindro(Vector3{ x, y, z }, Vector3{ x, y + 0.34f, z }, 0.20f, 0.26f, 8, Color{ 186, 98, 66, 255 });

    if (arbolito)
    {
        // Limonero en maceta.
        m.Cilindro(Vector3{ x, y + 0.3f, z }, Vector3{ x, y + 1.0f, z }, 0.05f, 0.04f, 5, MADERA_OSCURA_MERCADO);
        m.Esfera(Vector3{ x, y + 1.22f, z }, 0.42f, 5, 7, Color{ 76, 132, 64, 255 });
        m.Esfera(Vector3{ x + 0.2f, y + 1.18f, z + 0.24f }, 0.07f, 3, 4, Color{ 246, 214, 60, 255 });
        m.Esfera(Vector3{ x - 0.22f, y + 1.32f, z + 0.18f }, 0.07f, 3, 4, Color{ 246, 214, 60, 255 });
    }
    else
    {
        const Color flores[4] =
        {
            Color{ 236, 72, 110, 255 },
            Color{ 246, 196, 60, 255 },
            Color{ 196, 110, 220, 255 },
            Color{ 246, 246, 236, 255 }
        };

        m.Esfera(Vector3{ x, y + 0.42f, z }, 0.24f, 4, 6, Color{ 88, 140, 66, 255 });
        m.Esfera(Vector3{ x + 0.08f, y + 0.56f, z + 0.06f }, 0.12f, 3, 5, flores[(int)(v * 3.99f)]);
    }
}


static void AgregarBitaMercado(ConstructorMalla& m, float x, float y, float z)
{
    m.Cilindro(Vector3{ x, y, z }, Vector3{ x, y + 0.36f, z }, 0.14f, 0.12f, 8, Color{ 92, 88, 90, 255 });
    m.Cilindro(Vector3{ x, y + 0.36f, z }, Vector3{ x, y + 0.42f, z }, 0.17f, 0.17f, 8, Color{ 92, 88, 90, 255 });
}


static void AgregarRolloCuerdaMercado(ConstructorMalla& m, float x, float y, float z)
{
    m.Cilindro(Vector3{ x, y, z }, Vector3{ x, y + 0.14f, z }, 0.32f, 0.30f, 10, Color{ 196, 170, 120, 255 });
    m.Cilindro(Vector3{ x, y + 0.14f, z }, Vector3{ x, y + 0.16f, z }, 0.14f, 0.14f, 8, Color{ 120, 100, 72, 255 });
}


static void AgregarBancoMercado(ConstructorMalla& m, float x, float y, float z, float giro)
{
    EstablecerPiezaMercado(m, x, y, z, giro);

    m.Caja(Vector3{ 0.0f, 0.38f, 0.0f }, Vector3{ 1.2f, 0.07f, 0.38f }, MADERA_CLARA_MERCADO);
    m.Caja(Vector3{ 0.0f, 0.66f, -0.18f }, Vector3{ 1.2f, 0.26f, 0.05f }, MADERA_CLARA_MERCADO);
    m.Caja(Vector3{ -0.5f, 0.19f, 0.0f }, Vector3{ 0.07f, 0.38f, 0.34f }, HIERRO_MERCADO);
    m.Caja(Vector3{ 0.5f, 0.19f, 0.0f }, Vector3{ 0.07f, 0.38f, 0.34f }, HIERRO_MERCADO);

    m.QuitarTransformacion();
}


static void AgregarPilaMaderaMercado(ConstructorMalla& m, float x, float y, float z, float giro, float v)
{
    EstablecerPiezaMercado(m, x, y, z, giro);

    int capas = 2 + (int)(v * 2.0f);

    for (int c = 0; c < capas; c++)
    {
        for (int k = 0; k < 4 - c % 2; k++)
        {
            float px = (k - 1.5f + (c % 2) * 0.5f) * 0.26f;

            m.Caja(
                Vector3{ px, 0.12f + c * 0.22f, 0.0f },
                Vector3{ 0.22f, 0.2f, 1.9f },
                OscurecerMercado(MADERA_CLARA_MERCADO, 0.88f + 0.14f * Hash2Mercado(c, k))
            );
        }
    }

    m.QuitarTransformacion();
}


static void AgregarAnclaMercado(ConstructorMalla& m, float x, float y, float z, float giro)
{
    EstablecerPiezaMercado(m, x, y, z, giro);

    m.Caja(Vector3{ 0.0f, 0.06f, 0.0f }, Vector3{ 0.12f, 0.12f, 1.3f }, HIERRO_MERCADO);
    m.Caja(Vector3{ 0.0f, 0.06f, 0.6f }, Vector3{ 0.9f, 0.12f, 0.12f }, HIERRO_MERCADO);
    m.Caja(Vector3{ 0.0f, 0.06f, -0.6f }, Vector3{ 0.5f, 0.12f, 0.1f }, HIERRO_MERCADO);

    m.QuitarTransformacion();
}


static void AgregarRocaMercado(ConstructorMalla& m, float x, float y, float z, float radio, float v)
{
    TransformacionMalla tr;
    tr.tx = x;
    tr.ty = y;
    tr.tz = z;
    tr.giroY = v * 360.0f;
    tr.sx = 1.15f;
    tr.sy = 0.72f;
    tr.sz = 0.9f;

    m.EstablecerTransformacion(tr);

    Color piedra = OscurecerMercado(Color{ 140, 132, 124, 255 }, 0.82f + 0.3f * v);

    m.Esfera(Vector3{ 0.0f, radio * 0.4f, 0.0f }, radio, 4, 6, piedra);
    m.Esfera(
        Vector3{ radio * 0.3f, radio * 0.9f, -radio * 0.1f },
        radio * 0.55f, 3, 5,
        MezclaMercado(piedra, WHITE, 0.15f)
    );

    m.QuitarTransformacion();
}


static void AgregarPinoMercado(ConstructorMalla& m, float x, float y, float z, float escala, float v)
{
    Color hoja = MezclaMercado(Color{ 52, 98, 66, 255 }, Color{ 84, 120, 62, 255 }, v);

    m.Cilindro(Vector3{ x, y, z }, Vector3{ x, y + 0.7f * escala, z }, 0.12f * escala, 0.09f * escala, 5, MADERA_OSCURA_MERCADO);

    for (int k = 0; k < 3; k++)
    {
        float base = y + (0.55f + k * 0.62f) * escala;
        float radio = (0.95f - k * 0.24f) * escala;

        m.Cilindro(
            Vector3{ x, base, z },
            Vector3{ x, base + 0.95f * escala, z },
            radio, 0.0f, 7,
            OscurecerMercado(hoja, 0.9f + 0.08f * k)
        );
    }
}


static void AgregarCipresMercado(ConstructorMalla& m, float x, float y, float z, float escala)
{
    m.Cilindro(Vector3{ x, y, z }, Vector3{ x, y + 0.4f * escala, z }, 0.08f * escala, 0.07f * escala, 5, MADERA_OSCURA_MERCADO);
    m.Cilindro(
        Vector3{ x, y + 0.3f * escala, z },
        Vector3{ x, y + 2.8f * escala, z },
        0.42f * escala, 0.05f * escala, 7,
        Color{ 50, 88, 58, 255 }
    );
}


static void AgregarArbustoMercado(ConstructorMalla& m, float x, float y, float z, float v)
{
    Color hoja = MezclaMercado(Color{ 92, 132, 62, 255 }, Color{ 120, 146, 70, 255 }, v);

    m.Esfera(Vector3{ x, y + 0.22f, z }, 0.36f, 4, 6, hoja);
    m.Esfera(Vector3{ x + 0.24f, y + 0.18f, z + 0.1f }, 0.24f, 3, 5, OscurecerMercado(hoja, 0.9f));

    if (v > 0.55f)
    {
        m.Esfera(Vector3{ x - 0.1f, y + 0.5f, z + 0.12f }, 0.08f, 3, 4, Color{ 246, 236, 220, 255 });
    }
}


static void AgregarFloresMercado(ConstructorMalla& m, float x, float y, float z, float v)
{
    const Color colores[4] =
    {
        Color{ 236, 72, 110, 255 },
        Color{ 246, 196, 60, 255 },
        Color{ 196, 110, 220, 255 },
        Color{ 246, 246, 236, 255 }
    };

    Color c = colores[(int)(v * 3.99f)];

    for (int k = 0; k < 3; k++)
    {
        float a = (v * 6.0f + k * 2.1f);
        float fx = x + std::cos(a) * 0.22f;
        float fz = z + std::sin(a) * 0.22f;

        m.Cilindro(Vector3{ fx, y, fz }, Vector3{ fx, y + 0.24f, fz }, 0.02f, 0.02f, 3, Color{ 80, 130, 60, 255 });
        m.Esfera(Vector3{ fx, y + 0.27f, fz }, 0.07f, 3, 4, c);
    }
}


// Barca amarrada (frente = +Z local): casco, bancos y a veces carga.
static void AgregarBarcaAmarradaMercado(
    ConstructorMalla& m,
    float x, float z,
    float giro,
    float largo,
    Color casco,
    bool carga
)
{
    EstablecerPiezaMercado(m, x, NIVEL_AGUA_MERCADO + 0.06f, z, giro);

    float ancho = largo * 0.42f;
    Color interior = Color{ 176, 132, 86, 255 };

    m.Caja(Vector3{ 0.0f, 0.04f, 0.0f }, Vector3{ ancho, 0.30f, largo - 0.4f }, casco);
    m.Caja(Vector3{ 0.0f, 0.04f, largo * 0.5f - 0.12f }, Vector3{ ancho * 0.6f, 0.30f, 0.36f }, casco);
    m.Caja(Vector3{ 0.0f, 0.04f, -largo * 0.5f + 0.12f }, Vector3{ ancho * 0.6f, 0.30f, 0.36f }, casco);
    m.Caja(Vector3{ 0.0f, 0.20f, 0.0f }, Vector3{ ancho - 0.14f, 0.04f, largo - 0.6f }, interior);
    m.Caja(Vector3{ 0.0f, 0.22f, largo * 0.5f - 0.02f }, Vector3{ ancho * 0.5f, 0.18f, 0.16f }, casco);

    // Bancos.
    m.Caja(Vector3{ 0.0f, 0.26f, largo * 0.15f }, Vector3{ ancho - 0.1f, 0.05f, 0.18f }, MADERA_CLARA_MERCADO);
    m.Caja(Vector3{ 0.0f, 0.26f, -largo * 0.2f }, Vector3{ ancho - 0.1f, 0.05f, 0.18f }, MADERA_CLARA_MERCADO);

    if (carga)
    {
        m.Caja(Vector3{ 0.08f, 0.38f, -0.1f }, Vector3{ 0.30f, 0.24f, 0.30f }, MADERA_OSCURA_MERCADO);
        m.Esfera(Vector3{ -0.12f, 0.34f, 0.28f }, 0.13f, 4, 5, Color{ 236, 120, 48, 255 });
        m.Esfera(Vector3{ 0.14f, 0.32f, 0.40f }, 0.11f, 4, 5, Color{ 246, 200, 60, 255 });
    }

    m.QuitarTransformacion();
}


//==================================================
// LANDMARKS
//==================================================
//
//   - PLAZA: torre de la campana (13.4 de alto, landmark principal).
//   - ISLA DEL FARO: faro rayado (~13.7) en la punta y casa del farero.
//   - ASTILLERO: dique seco con un casco a medio armar, andamios, grua
//     portico y dos gruas altas sobre el malecon oeste.
//   - EMBARCADERO: la aduana con su reloj. ESPECIAS: el almacen con la
//     polea. FLORES: la basilica de la cupula. PESCADORES: la lonja y la
//     chimenea del ahumadero.

static const float ALTURA_CAMPANA_MERCADO = 9.95f;


static void AgregarTorreCampanaMercado(ConstructorMalla& m, bool detalle)
{
    const Color piedra = Color{ 226, 206, 174, 255 };
    const Color piedraOscura = Color{ 186, 162, 128, 255 };
    const Color tejado = Color{ 196, 82, 58, 255 };
    const Color oro = Color{ 246, 200, 70, 255 };

    EstablecerPiezaMercado(m, PLAZA_CX, 0.0f, PLAZA_CZ, 0.0f);

    // Gradas y zocalo.
    m.Caja(Vector3{ 0.0f, 0.12f, 0.0f }, Vector3{ 2.7f, 0.24f, 2.7f }, PIEDRA_OSCURA_MERCADO);
    m.Caja(Vector3{ 0.0f, 0.36f, 0.0f }, Vector3{ 2.2f, 0.24f, 2.2f }, piedraOscura);

    // Fuste.
    m.Caja(Vector3{ 0.0f, 4.25f, 0.0f }, Vector3{ 1.5f, 7.5f, 1.5f }, piedra);

    // Pilastras de las esquinas y fajas.
    for (int sx = -1; sx <= 1; sx += 2)
    {
        for (int sz = -1; sz <= 1; sz += 2)
        {
            m.Caja(
                Vector3{ sx * 0.72f, 4.25f, sz * 0.72f },
                Vector3{ 0.22f, 7.5f, 0.22f },
                piedraOscura
            );
        }
    }

    const float fajas[3] = { 2.9f, 5.4f, 7.9f };

    for (float y : fajas)
    {
        m.Caja(Vector3{ 0.0f, y, 0.0f }, Vector3{ 1.68f, 0.16f, 1.68f }, PIEDRA_CLARA_MERCADO);
    }

    if (detalle)
    {
        // Ventanucos y relojes en las cuatro caras.
        for (int cara = 0; cara < 4; cara++)
        {
            EstablecerPiezaMercado(m, PLAZA_CX, 0.0f, PLAZA_CZ, cara * 90.0f);

            FachadaZMercado(m, -0.12f, 1.6f, 0.12f, 2.3f, 0.76f, Color{ 60, 50, 52, 255 });
            FachadaZMercado(m, -0.12f, 4.0f, 0.12f, 4.8f, 0.76f, Color{ 60, 50, 52, 255 });

            m.Cilindro(
                Vector3{ 0.0f, 6.6f, 0.75f }, Vector3{ 0.0f, 6.6f, 0.80f },
                0.52f, 0.52f, 14,
                Color{ 120, 84, 52, 255 }
            );

            m.Cilindro(
                Vector3{ 0.0f, 6.6f, 0.79f }, Vector3{ 0.0f, 6.6f, 0.83f },
                0.44f, 0.44f, 14,
                EmisivoMercado(Color{ 250, 240, 214, 255 })
            );

            m.Caja(Vector3{ 0.0f, 6.78f, 0.85f }, Vector3{ 0.05f, 0.36f, 0.02f }, HIERRO_MERCADO);
            m.Caja(Vector3{ 0.12f, 6.6f, 0.85f }, Vector3{ 0.26f, 0.05f, 0.02f }, HIERRO_MERCADO);
        }

        EstablecerPiezaMercado(m, PLAZA_CX, 0.0f, PLAZA_CZ, 0.0f);
    }

    // Campanario: losa, cuatro pilares, baranda y techo.
    m.Caja(Vector3{ 0.0f, 8.1f, 0.0f }, Vector3{ 1.9f, 0.24f, 1.9f }, piedraOscura);

    for (int sx = -1; sx <= 1; sx += 2)
    {
        for (int sz = -1; sz <= 1; sz += 2)
        {
            m.Caja(
                Vector3{ sx * 0.68f, 9.25f, sz * 0.68f },
                Vector3{ 0.3f, 2.1f, 0.3f },
                piedra
            );
        }
    }

    for (int lado = 0; lado < 4; lado++)
    {
        float s = (lado < 2) ? 1.0f : -1.0f;
        bool enX = lado % 2 == 0;

        m.Caja(
            Vector3{ enX ? s * 0.86f : 0.0f, 8.55f, enX ? 0.0f : s * 0.86f },
            Vector3{ enX ? 0.08f : 1.7f, 0.08f, enX ? 1.7f : 0.08f },
            PIEDRA_CLARA_MERCADO
        );
    }

    m.Caja(Vector3{ 0.0f, 10.38f, 0.0f }, Vector3{ 1.95f, 0.26f, 1.95f }, piedraOscura);

    // Viga de la campana (la campana se dibuja animada).
    m.Caja(Vector3{ 0.0f, ALTURA_CAMPANA_MERCADO + 0.12f, 0.0f }, Vector3{ 1.36f, 0.12f, 0.14f }, MADERA_OSCURA_MERCADO);

    m.QuitarTransformacion();

    // Techo piramidal (alineado con el fuste) y remate dorado.
    TransformacionMalla tr;
    tr.tx = PLAZA_CX;
    tr.tz = PLAZA_CZ;
    tr.giroY = 45.0f;
    m.EstablecerTransformacion(tr);

    m.Cilindro(
        Vector3{ 0.0f, 10.5f, 0.0f },
        Vector3{ 0.0f, 12.7f, 0.0f },
        1.42f, 0.0f, 4,
        tejado
    );

    m.QuitarTransformacion();

    m.Esfera(Vector3{ PLAZA_CX, 12.82f, PLAZA_CZ }, 0.16f, 5, 6, oro);
    m.Cilindro(
        Vector3{ PLAZA_CX, 12.9f, PLAZA_CZ },
        Vector3{ PLAZA_CX, 13.45f, PLAZA_CZ },
        0.03f, 0.03f, 4,
        HIERRO_MERCADO
    );
    m.Caja(Vector3{ PLAZA_CX + 0.12f, 13.32f, PLAZA_CZ }, Vector3{ 0.3f, 0.12f, 0.02f }, HIERRO_MERCADO);
}


static float AlturaBaseFaroMercado()
{
    return AlturaIslaFaroMercado(FARO_X, FARO_Z);
}


static void AgregarFaroMercado(ConstructorMalla& m, bool detalle)
{
    const Color blanco = Color{ 244, 238, 226, 255 };
    const Color rojo = Color{ 206, 64, 56, 255 };

    float y0 = AlturaBaseFaroMercado();
    float x = FARO_X;
    float z = FARO_Z;

    // Rocas al pie.
    for (int k = 0; k < 7; k++)
    {
        float a = k * 0.9f + 0.3f;
        float r = 1.9f + 0.4f * Hash2Mercado(k, 41);

        AgregarRocaMercado(
            m,
            x + std::cos(a) * r, y0 - 0.25f, z + std::sin(a) * r,
            0.55f + 0.3f * Hash2Mercado(k, 42), Hash2Mercado(k, 43)
        );
    }

    m.Cilindro(Vector3{ x, y0 - 0.4f, z }, Vector3{ x, y0 + 0.6f, z }, 1.6f, 1.45f, 12, PIEDRA_OSCURA_MERCADO);

    // Torre en franjas rojas y blancas.
    const int franjas = 6;
    const float alto = 9.4f;

    for (int k = 0; k < franjas; k++)
    {
        float t0 = (float)k / franjas;
        float t1 = (float)(k + 1) / franjas;

        m.Cilindro(
            Vector3{ x, y0 + 0.6f + alto * t0, z },
            Vector3{ x, y0 + 0.6f + alto * t1, z },
            1.1f - 0.35f * t0, 1.1f - 0.35f * t1, 12,
            k % 2 == 0 ? blanco : rojo
        );
    }

    float yg = y0 + 0.6f + alto;

    // Galeria con baranda.
    m.Cilindro(Vector3{ x, yg, z }, Vector3{ x, yg + 0.16f, z }, 1.2f, 1.2f, 12, HIERRO_MERCADO);

    if (detalle)
    {
        for (int k = 0; k < 12; k++)
        {
            float a = k * 6.2831853f / 12.0f;

            m.Caja(
                Vector3{ x + std::cos(a) * 1.12f, yg + 0.42f, z + std::sin(a) * 1.12f },
                Vector3{ 0.05f, 0.5f, 0.05f },
                HIERRO_MERCADO
            );
        }

        m.Cilindro(Vector3{ x, yg + 0.66f, z }, Vector3{ x, yg + 0.7f, z }, 1.14f, 1.14f, 12, HIERRO_MERCADO);
    }

    // Linterna (se ilumina en el dibujo animado) y cupula.
    m.Cilindro(Vector3{ x, yg + 0.16f, z }, Vector3{ x, yg + 1.36f, z }, 0.62f, 0.62f, 10, EmisivoMercado(Color{ 255, 236, 170, 255 }));

    for (int k = 0; k < 6; k++)
    {
        float a = k * 6.2831853f / 6.0f;

        m.Caja(
            Vector3{ x + std::cos(a) * 0.62f, yg + 0.76f, z + std::sin(a) * 0.62f },
            Vector3{ 0.06f, 1.2f, 0.06f },
            HIERRO_MERCADO
        );
    }

    m.Cilindro(Vector3{ x, yg + 1.36f, z }, Vector3{ x, yg + 2.1f, z }, 0.74f, 0.05f, 10, rojo);
    m.Esfera(Vector3{ x, yg + 2.2f, z }, 0.12f, 4, 6, HIERRO_MERCADO);

    // Puerta.
    EstablecerPiezaMercado(m, x, y0, z, 210.0f);
    FachadaZMercado(m, -0.28f, 0.6f, 0.28f, 1.5f, 1.12f, MADERA_OSCURA_MERCADO);
    m.QuitarTransformacion();
}


static void AgregarCasaFareroMercado(ConstructorMalla& m, bool detalle)
{
    float y = AlturaIslaFaroMercado(CASA_FARERO_X, CASA_FARERO_Z);

    EstablecerPiezaMercado(m, CASA_FARERO_X, y - 0.1f, CASA_FARERO_Z, 0.0f);

    const Color cal = Color{ 244, 240, 230, 255 };
    const Color azul = Color{ 70, 110, 160, 255 };

    m.Caja(Vector3{ 0.0f, 0.95f, 0.0f }, Vector3{ 2.4f, 1.9f, 1.9f }, cal);
    TechoDosAguasMercado(m, 2.4f, 1.9f, 1.9f, 0.9f, azul, cal);
    m.Caja(Vector3{ 0.7f, 2.6f, -0.3f }, Vector3{ 0.3f, 0.8f, 0.3f }, cal);

    if (detalle)
    {
        FachadaZMercado(m, -0.25f, 0.0f, 0.25f, 0.95f, 0.96f, azul);
        FachadaZMercado(m, 0.55f, 0.9f, 0.95f, 1.35f, 0.96f, EmisivoMercado(Color{ 255, 206, 128, 255 }));
        FachadaZMercado(m, -0.95f, 0.9f, -0.55f, 1.35f, 0.96f, Color{ 70, 82, 104, 255 });
    }

    // Redes de pesca colgadas a secar.
    for (int k = 0; k < 2; k++)
    {
        float px = -1.9f + k * 3.8f;

        m.Cilindro(Vector3{ px, 0.0f, 1.7f }, Vector3{ px, 1.5f, 1.7f }, 0.05f, 0.05f, 5, MADERA_OSCURA_MERCADO);
    }

    m.Cilindro(Vector3{ -1.9f, 1.45f, 1.7f }, Vector3{ 1.9f, 1.45f, 1.7f }, 0.03f, 0.03f, 4, MADERA_OSCURA_MERCADO);
    m.TrianguloDobleCara({ -1.8f, 1.45f, 1.7f }, { 1.8f, 1.45f, 1.7f }, { 1.4f, 0.7f, 1.75f }, Color{ 96, 116, 110, 255 });
    m.TrianguloDobleCara({ -1.8f, 1.45f, 1.7f }, { 1.4f, 0.7f, 1.75f }, { -1.4f, 0.8f, 1.75f }, Color{ 96, 116, 110, 255 });

    m.QuitarTransformacion();
}


// Casco de barco a medio armar dentro del dique seco (eslora a lo largo de Z):
// tablazon en la proa y la mitad del casco, cuadernas al aire en la popa.
static void AgregarCascoAstilleroMercado(ConstructorMalla& m, bool detalle)
{
    const float fondo = -1.55f;
    const float cubierta = 0.95f;
    const float eslora = 5.6f;
    const float manga = 1.55f;
    const int secciones = detalle ? 14 : 8;
    const int alturas = 4;

    const Color tablaA = Color{ 156, 104, 64, 255 };
    const Color tablaB = Color{ 132, 88, 56, 255 };

    // Picaderos.
    for (int k = -2; k <= 2; k++)
    {
        m.Caja(
            Vector3{ DIQUE_X, fondo + 0.15f, DIQUE_Z + k * 1.0f },
            Vector3{ 0.9f, 0.3f, 0.3f },
            MADERA_OSCURA_MERCADO
        );
    }

    // Quilla.
    m.Caja(
        Vector3{ DIQUE_X, fondo + 0.42f, DIQUE_Z },
        Vector3{ 0.18f, 0.24f, eslora + 0.4f },
        MADERA_OSCURA_MERCADO
    );

    auto puntoCasco = [&](float u, float v, float lado) -> Vector3
    {
        // u: 0 popa .. 1 proa; v: 0 quilla .. 1 borda.
        float zl = -eslora * 0.5f + eslora * u;
        float forma = std::sqrt(Limitar01Mercado(1.0f - std::pow(std::fabs(u - 0.45f) / 0.58f, 2.4f)));
        float ancho = manga * forma * std::sqrt(v);
        float y = fondo + 0.5f + (cubierta - fondo - 0.5f) * v + 0.25f * u * u * v;

        return Vector3{ DIQUE_X + lado * ancho, y, DIQUE_Z - zl };
    };

    for (int s = 0; s < secciones; s++)
    {
        float u0 = (float)s / secciones;
        float u1 = (float)(s + 1) / secciones;
        bool forrado = u0 >= 0.32f;

        for (int h = 0; h < alturas; h++)
        {
            float v0 = (float)h / alturas;
            float v1 = (float)(h + 1) / alturas;

            for (int lado = -1; lado <= 1; lado += 2)
            {
                Vector3 a = puntoCasco(u0, v0, (float)lado);
                Vector3 b = puntoCasco(u1, v0, (float)lado);
                Vector3 c = puntoCasco(u1, v1, (float)lado);
                Vector3 d = puntoCasco(u0, v1, (float)lado);

                if (forrado)
                {
                    Color c0 = (h % 2 == 0) ? tablaA : tablaB;

                    m.TrianguloDobleCara(a, b, c, c0);
                    m.TrianguloDobleCara(a, c, d, c0);
                }
                else if (s % 2 == 0)
                {
                    // Cuaderna: listón a lo largo del contorno.
                    m.Cilindro(a, d, 0.07f, 0.07f, 4, Color{ 176, 128, 80, 255 });
                }
            }
        }
    }

    // Regala (borde superior) y cubierta parcial.
    for (int s = 0; s < secciones; s++)
    {
        float u0 = (float)s / secciones;
        float u1 = (float)(s + 1) / secciones;

        for (int lado = -1; lado <= 1; lado += 2)
        {
            m.Cilindro(
                puntoCasco(u0, 1.0f, (float)lado),
                puntoCasco(u1, 1.0f, (float)lado),
                0.06f, 0.06f, 4,
                MADERA_OSCURA_MERCADO
            );
        }

        if (u0 >= 0.55f)
        {
            Vector3 a = puntoCasco(u0, 0.98f, -1.0f);
            Vector3 b = puntoCasco(u1, 0.98f, -1.0f);
            Vector3 c = puntoCasco(u1, 0.98f, 1.0f);
            Vector3 d = puntoCasco(u0, 0.98f, 1.0f);

            m.TrianguloDobleCara(a, b, c, MADERA_CLARA_MERCADO);
            m.TrianguloDobleCara(a, c, d, MADERA_CLARA_MERCADO);
        }
    }

    // Roda de proa.
    m.Cilindro(
        Vector3{ DIQUE_X, fondo + 0.5f, DIQUE_Z - eslora * 0.5f },
        Vector3{ DIQUE_X, cubierta + 0.55f, DIQUE_Z - eslora * 0.5f - 0.45f },
        0.09f, 0.07f, 4,
        MADERA_OSCURA_MERCADO
    );

    // Andamios a los dos costados.
    if (detalle)
    {
        for (int lado = -1; lado <= 1; lado += 2)
        {
            float x = DIQUE_X + lado * (manga + 0.42f);

            for (int k = 0; k < 6; k++)
            {
                float z = DIQUE_Z - 2.6f + k * 1.04f;

                m.Cilindro(Vector3{ x, fondo, z }, Vector3{ x, cubierta + 0.9f, z }, 0.04f, 0.04f, 4, Color{ 196, 170, 120, 255 });
            }

            for (int nivel = 0; nivel < 2; nivel++)
            {
                float y = -0.5f + nivel * 1.05f;

                m.Caja(
                    Vector3{ x - lado * 0.12f, y, DIQUE_Z },
                    Vector3{ 0.36f, 0.05f, 5.3f },
                    MADERA_CLARA_MERCADO
                );

                m.Cilindro(
                    Vector3{ x + lado * 0.04f, y + 0.55f, DIQUE_Z - 2.6f },
                    Vector3{ x + lado * 0.04f, y + 0.55f, DIQUE_Z + 2.6f },
                    0.025f, 0.025f, 4,
                    Color{ 196, 170, 120, 255 }
                );
            }
        }
    }
}


// Grua portico sobre el dique: dos patas en A y una viga con carro y gancho.
static void AgregarGruaPorticoMercado(ConstructorMalla& m)
{
    const Color amarillo = Color{ 226, 168, 52, 255 };
    const float alto = 5.4f;
    const float zp = DIQUE_Z - 1.1f;
    const float separacion = DIQUE_MEDIO_ANCHO + 0.75f;

    for (int lado = -1; lado <= 1; lado += 2)
    {
        float x = DIQUE_X + lado * separacion;

        m.Caja(Vector3{ x, 0.12f, zp }, Vector3{ 0.5f, 0.24f, 2.4f }, HIERRO_MERCADO);

        m.Cilindro(Vector3{ x, 0.2f, zp - 1.0f }, Vector3{ x, alto, zp }, 0.11f, 0.1f, 5, amarillo);
        m.Cilindro(Vector3{ x, 0.2f, zp + 1.0f }, Vector3{ x, alto, zp }, 0.11f, 0.1f, 5, amarillo);
        m.Cilindro(Vector3{ x, 1.6f, zp - 0.7f }, Vector3{ x, 1.6f, zp + 0.7f }, 0.06f, 0.06f, 4, amarillo);
    }

    m.Caja(
        Vector3{ DIQUE_X, alto + 0.12f, zp },
        Vector3{ separacion * 2.0f + 0.5f, 0.34f, 0.4f },
        amarillo
    );

    m.Caja(Vector3{ DIQUE_X + 0.4f, alto - 0.14f, zp }, Vector3{ 0.6f, 0.3f, 0.6f }, HIERRO_MERCADO);
    m.Cilindro(Vector3{ DIQUE_X + 0.4f, alto - 0.3f, zp }, Vector3{ DIQUE_X + 0.4f, 1.7f, zp }, 0.02f, 0.02f, 3, HIERRO_MERCADO);
    m.Caja(Vector3{ DIQUE_X + 0.4f, 1.6f, zp }, Vector3{ 0.2f, 0.25f, 0.1f }, HIERRO_MERCADO);

    // Viga de madera colgando del gancho.
    m.Caja(Vector3{ DIQUE_X + 0.4f, 1.35f, zp }, Vector3{ 0.2f, 0.2f, 2.0f }, MADERA_CLARA_MERCADO);
}


// Grua alta de puerto (torre de celosia, cabina y pluma hacia el mar).
static void AgregarGruaAltaMercado(ConstructorMalla& m, float x, float z, float giro)
{
    const Color rojo = Color{ 190, 70, 50, 255 };
    const float alto = 7.6f;

    EstablecerPiezaMercado(m, x, 0.0f, z, giro);

    m.Caja(Vector3{ 0.0f, 0.2f, 0.0f }, Vector3{ 1.8f, 0.4f, 1.8f }, PIEDRA_OSCURA_MERCADO);

    for (int sx = -1; sx <= 1; sx += 2)
    {
        for (int sz = -1; sz <= 1; sz += 2)
        {
            m.Cilindro(
                Vector3{ sx * 0.7f, 0.4f, sz * 0.7f },
                Vector3{ sx * 0.38f, alto, sz * 0.38f },
                0.07f, 0.06f, 4,
                rojo
            );
        }
    }

    // Arriostramiento en X en el frente y los costados.
    for (int tramo = 0; tramo < 3; tramo++)
    {
        float y0 = 0.4f + tramo * (alto - 0.4f) / 3.0f;
        float y1 = y0 + (alto - 0.4f) / 3.0f;
        float a0 = 0.7f - 0.32f * tramo / 3.0f;
        float a1 = 0.7f - 0.32f * (tramo + 1) / 3.0f;

        m.Cilindro(Vector3{ -a0, y0, a0 }, Vector3{ a1, y1, a1 }, 0.035f, 0.035f, 3, rojo);
        m.Cilindro(Vector3{ a0, y0, a0 }, Vector3{ -a1, y1, a1 }, 0.035f, 0.035f, 3, rojo);
        m.Cilindro(Vector3{ a0, y0, -a0 }, Vector3{ a1, y1, a1 }, 0.035f, 0.035f, 3, rojo);
        m.Cilindro(Vector3{ a0, y0, a0 }, Vector3{ a1, y1, -a1 }, 0.035f, 0.035f, 3, rojo);
    }

    // Cabina.
    m.Caja(Vector3{ 0.0f, alto + 0.5f, 0.0f }, Vector3{ 1.2f, 1.0f, 1.3f }, Color{ 226, 214, 190, 255 });
    m.Caja(Vector3{ 0.0f, alto + 1.05f, 0.0f }, Vector3{ 1.35f, 0.12f, 1.45f }, rojo);
    FachadaZMercado(m, -0.4f, alto + 0.5f, 0.4f, alto + 0.85f, 0.66f, EmisivoMercado(Color{ 255, 214, 140, 255 }));

    // Pluma inclinada hacia +Z local y contrapeso atras.
    m.Cilindro(Vector3{ -0.25f, alto + 0.4f, 0.5f }, Vector3{ -0.1f, alto + 3.4f, 5.4f }, 0.07f, 0.05f, 4, rojo);
    m.Cilindro(Vector3{ 0.25f, alto + 0.4f, 0.5f }, Vector3{ 0.1f, alto + 3.4f, 5.4f }, 0.07f, 0.05f, 4, rojo);

    for (int k = 1; k < 5; k++)
    {
        float t = k / 5.0f;
        float y = alto + 0.4f + 3.0f * t;
        float zz = 0.5f + 4.9f * t;
        float a = 0.25f - 0.15f * t;

        m.Cilindro(Vector3{ -a, y, zz }, Vector3{ a, y, zz }, 0.03f, 0.03f, 3, rojo);
    }

    m.Caja(Vector3{ 0.0f, alto + 0.7f, -1.0f }, Vector3{ 0.9f, 0.7f, 0.6f }, PIEDRA_OSCURA_MERCADO);

    // Cable y gancho.
    m.Cilindro(Vector3{ 0.0f, alto + 3.35f, 5.35f }, Vector3{ 0.0f, alto - 1.2f, 5.35f }, 0.02f, 0.02f, 3, HIERRO_MERCADO);
    m.Caja(Vector3{ 0.0f, alto - 1.3f, 5.35f }, Vector3{ 0.2f, 0.25f, 0.12f }, HIERRO_MERCADO);

    m.QuitarTransformacion();
}


// La aduana del embarcadero (frente al sur, al mar) con su torre del reloj.
static void AgregarAduanaMercado(ConstructorMalla& m, bool detalle)
{
    const float x = 20.6f;
    const float z = 6.2f;
    const Color pared = Color{ 238, 214, 150, 255 };
    const Color tejado = Color{ 168, 74, 54, 255 };

    EstablecerPiezaMercado(m, x, 0.0f, z, 0.0f);

    m.Caja(Vector3{ 0.0f, 1.9f, 0.0f }, Vector3{ 5.6f, 3.8f, 3.4f }, pared);
    m.Caja(Vector3{ 0.0f, 0.25f, 0.0f }, Vector3{ 5.7f, 0.5f, 3.5f }, OscurecerMercado(pared, 0.75f));
    m.Caja(Vector3{ 0.0f, 3.86f, 0.0f }, Vector3{ 5.9f, 0.16f, 3.7f }, PIEDRA_CLARA_MERCADO);
    TechoDosAguasMercado(m, 5.6f, 3.4f, 3.9f, 1.0f, tejado, pared);

    if (detalle)
    {
        // Arcadas de la planta baja y ventanas altas.
        for (int k = 0; k < 4; k++)
        {
            float ax = -2.1f + k * 1.4f;

            FachadaZMercado(m, ax - 0.42f, 0.0f, ax + 0.42f, 1.5f, 1.71f, Color{ 120, 90, 60, 255 });
            FachadaZMercado(m, ax - 0.36f, 2.2f, ax + 0.36f, 3.2f, 1.71f, PIEDRA_CLARA_MERCADO);
            FachadaZMercado(
                m, ax - 0.28f, 2.3f, ax + 0.28f, 3.1f, 1.72f,
                k % 2 == 0 ? EmisivoMercado(Color{ 255, 206, 128, 255 }) : Color{ 70, 82, 104, 255 }
            );
        }
    }

    // Torre del reloj sobre la cumbrera.
    m.Caja(Vector3{ 0.0f, 5.6f, 0.0f }, Vector3{ 1.5f, 2.2f, 1.5f }, pared);
    m.Caja(Vector3{ 0.0f, 6.75f, 0.0f }, Vector3{ 1.7f, 0.14f, 1.7f }, PIEDRA_CLARA_MERCADO);

    m.Cilindro(Vector3{ 0.0f, 5.8f, 0.76f }, Vector3{ 0.0f, 5.8f, 0.8f }, 0.5f, 0.5f, 14, EmisivoMercado(Color{ 250, 240, 214, 255 }));
    m.Caja(Vector3{ 0.0f, 5.95f, 0.82f }, Vector3{ 0.05f, 0.32f, 0.02f }, HIERRO_MERCADO);
    m.Caja(Vector3{ -0.1f, 5.8f, 0.82f }, Vector3{ 0.22f, 0.05f, 0.02f }, HIERRO_MERCADO);

    m.QuitarTransformacion();

    TransformacionMalla tr;
    tr.tx = x;
    tr.tz = z;
    tr.giroY = 45.0f;
    m.EstablecerTransformacion(tr);
    m.Cilindro(Vector3{ 0.0f, 6.82f, 0.0f }, Vector3{ 0.0f, 8.2f, 0.0f }, 1.2f, 0.0f, 4, tejado);
    m.QuitarTransformacion();

    // Mastil con la bandera del puerto.
    m.Cilindro(Vector3{ x + 2.4f, 3.9f, z + 1.2f }, Vector3{ x + 2.4f, 6.4f, z + 1.2f }, 0.04f, 0.04f, 4, HIERRO_MERCADO);

    OcuparMercado(x, z, 3.0f, 1.9f);
}


// El almacen de especias (frente al oeste, al canal) con su polea.
static void AgregarAlmacenEspeciasMercado(ConstructorMalla& m, bool detalle)
{
    const float x = 20.4f;
    const float z = 1.3f;
    const Color pared = Color{ 214, 150, 86, 255 };
    const Color tejado = Color{ 120, 74, 54, 255 };

    EstablecerPiezaMercado(m, x, 0.0f, z, -90.0f);

    m.Caja(Vector3{ 0.0f, 2.3f, 0.0f }, Vector3{ 3.6f, 4.6f, 5.0f }, pared);
    m.Caja(Vector3{ 0.0f, 0.2f, 0.0f }, Vector3{ 3.7f, 0.4f, 5.1f }, OscurecerMercado(pared, 0.7f));

    // Techo a dos aguas con la cumbrera hacia el canal.
    TransformacionMalla tr;
    tr.tx = x;
    tr.tz = z;
    tr.giroY = 0.0f;
    m.EstablecerTransformacion(tr);
    TechoDosAguasMercado(m, 5.0f, 3.6f, 4.6f, 1.3f, tejado, pared);

    EstablecerPiezaMercado(m, x, 0.0f, z, -90.0f);

    if (detalle)
    {
        float frente = 2.51f;

        FachadaZMercado(m, -0.8f, 0.0f, 0.8f, 2.0f, frente, MADERA_OSCURA_MERCADO);
        FachadaZMercado(m, -0.55f, 2.8f, 0.55f, 3.9f, frente, MADERA_MERCADO);
        FachadaZMercado(m, -1.35f, 2.9f, -0.95f, 3.5f, frente, Color{ 70, 82, 104, 255 });
        FachadaZMercado(m, 0.95f, 2.9f, 1.35f, 3.5f, frente, EmisivoMercado(Color{ 255, 206, 128, 255 }));
    }

    // Viga de la polea con un fardo colgando.
    m.Caja(Vector3{ 0.0f, 4.4f, 3.0f }, Vector3{ 0.2f, 0.2f, 1.2f }, MADERA_OSCURA_MERCADO);
    m.Cilindro(Vector3{ 0.0f, 4.3f, 3.5f }, Vector3{ 0.0f, 2.6f, 3.5f }, 0.02f, 0.02f, 3, Color{ 196, 170, 120, 255 });
    m.Cilindro(Vector3{ 0.0f, 2.1f, 3.5f }, Vector3{ 0.0f, 2.6f, 3.5f }, 0.3f, 0.22f, 6, Color{ 206, 180, 132, 255 });

    m.QuitarTransformacion();

    OcuparMercado(x, z, 2.6f, 1.9f);
}


// La basilica de la cupula (detras del muelle de las flores).
static void AgregarBasilicaMercado(ConstructorMalla& m, bool detalle)
{
    const float x = -4.6f;
    const float z = -23.4f;
    const Color piedra = Color{ 236, 222, 198, 255 };
    const Color cupula = Color{ 196, 110, 72, 255 };

    EstablecerPiezaMercado(m, x, 0.0f, z, 0.0f);

    // Nave y transepto.
    m.Caja(Vector3{ 0.0f, 2.5f, 0.0f }, Vector3{ 4.0f, 5.0f, 7.0f }, piedra);
    m.Caja(Vector3{ 0.0f, 2.2f, -0.6f }, Vector3{ 7.0f, 4.4f, 3.0f }, piedra);
    TechoDosAguasMercado(m, 7.0f, 3.0f, 4.4f, 1.0f, Color{ 176, 92, 64, 255 }, piedra);

    // Fachada con rosetón y dos campanarios bajos.
    m.Caja(Vector3{ 0.0f, 3.3f, 3.45f }, Vector3{ 4.6f, 6.6f, 0.4f }, PIEDRA_CLARA_MERCADO);

    if (detalle)
    {
        m.Cilindro(Vector3{ 0.0f, 4.6f, 3.66f }, Vector3{ 0.0f, 4.6f, 3.7f }, 0.8f, 0.8f, 14, Color{ 120, 84, 60, 255 });
        m.Cilindro(Vector3{ 0.0f, 4.6f, 3.69f }, Vector3{ 0.0f, 4.6f, 3.73f }, 0.62f, 0.62f, 14, EmisivoMercado(Color{ 250, 170, 120, 255 }));
        FachadaZMercado(m, -0.6f, 0.0f, 0.6f, 2.2f, 3.66f, MADERA_OSCURA_MERCADO);
    }

    for (int lado = -1; lado <= 1; lado += 2)
    {
        m.Caja(Vector3{ lado * 2.6f, 3.8f, 3.2f }, Vector3{ 1.0f, 7.6f, 1.0f }, piedra);
        m.Cilindro(Vector3{ lado * 2.6f, 7.6f, 3.2f }, Vector3{ lado * 2.6f, 9.0f, 3.2f }, 0.62f, 0.0f, 8, cupula);
    }

    // Tambor y cupula sobre el crucero.
    m.Cilindro(Vector3{ 0.0f, 4.4f, -0.6f }, Vector3{ 0.0f, 6.0f, -0.6f }, 2.0f, 2.0f, 16, piedra);
    m.Esfera(Vector3{ 0.0f, 6.0f, -0.6f }, 2.0f, 8, 16, cupula);
    m.Cilindro(Vector3{ 0.0f, 7.9f, -0.6f }, Vector3{ 0.0f, 8.8f, -0.6f }, 0.42f, 0.42f, 8, piedra);
    m.Cilindro(Vector3{ 0.0f, 8.8f, -0.6f }, Vector3{ 0.0f, 9.6f, -0.6f }, 0.5f, 0.0f, 8, cupula);
    m.Esfera(Vector3{ 0.0f, 9.7f, -0.6f }, 0.12f, 4, 6, Color{ 246, 200, 70, 255 });

    m.QuitarTransformacion();

    OcuparMercado(x, z, 3.7f, 3.8f);
}


// La lonja de los pescadores (frente al este, al canal) y el ahumadero.
static void AgregarLonjaMercado(ConstructorMalla& m, bool detalle)
{
    const float x = -21.2f;
    const float z = -9.4f;
    const Color pared = Color{ 132, 176, 186, 255 };

    EstablecerPiezaMercado(m, x, 0.0f, z, 90.0f);

    // Nave abierta: pilares, techo alto a dos aguas y mostradores de pescado.
    for (int k = 0; k < 4; k++)
    {
        float px = -2.4f + k * 1.6f;

        m.Caja(Vector3{ px, 1.3f, 1.8f }, Vector3{ 0.3f, 2.6f, 0.3f }, pared);
        m.Caja(Vector3{ px, 1.3f, -1.8f }, Vector3{ 0.3f, 2.6f, 0.3f }, pared);
    }

    m.Caja(Vector3{ 0.0f, 1.3f, -1.95f }, Vector3{ 5.2f, 2.6f, 0.15f }, OscurecerMercado(pared, 0.9f));
    m.Caja(Vector3{ 0.0f, 2.68f, 0.0f }, Vector3{ 5.4f, 0.16f, 4.0f }, PIEDRA_CLARA_MERCADO);
    TechoDosAguasMercado(m, 5.4f, 4.0f, 2.76f, 1.6f, Color{ 70, 104, 120, 255 }, pared);

    if (detalle)
    {
        for (int k = 0; k < 3; k++)
        {
            float px = -1.6f + k * 1.6f;

            m.Caja(Vector3{ px, 0.45f, 0.0f }, Vector3{ 1.2f, 0.9f, 0.7f }, Color{ 226, 226, 220, 255 });
            m.Caja(Vector3{ px, 0.93f, 0.0f }, Vector3{ 1.1f, 0.06f, 0.6f }, Color{ 150, 196, 210, 255 });

            for (int p = 0; p < 3; p++)
            {
                m.Esfera(
                    Vector3{ px - 0.3f + p * 0.3f, 1.0f, 0.05f },
                    0.09f, 3, 5,
                    p % 2 == 0 ? Color{ 176, 190, 200, 255 } : Color{ 226, 120, 96, 255 }
                );
            }
        }
    }

    m.QuitarTransformacion();

    // Chimenea del ahumadero (el humo se dibuja animado).
    m.Caja(Vector3{ AHUMADERO_X, 0.9f, AHUMADERO_Z }, Vector3{ 2.2f, 1.8f, 2.0f }, Color{ 150, 96, 74, 255 });
    m.Cilindro(
        Vector3{ AHUMADERO_X, 1.8f, AHUMADERO_Z },
        Vector3{ AHUMADERO_X, ALTURA_AHUMADERO, AHUMADERO_Z },
        0.48f, 0.36f, 8,
        Color{ 168, 92, 70, 255 }
    );
    m.Cilindro(
        Vector3{ AHUMADERO_X, ALTURA_AHUMADERO - 0.3f, AHUMADERO_Z },
        Vector3{ AHUMADERO_X, ALTURA_AHUMADERO, AHUMADERO_Z },
        0.46f, 0.46f, 8,
        HIERRO_MERCADO
    );

    // Secaderos de pescado.
    for (int k = 0; k < 2; k++)
    {
        float sx = -27.6f;
        float sz = -9.6f + k * 2.6f;

        m.Cilindro(Vector3{ sx - 0.9f, 0.0f, sz }, Vector3{ sx - 0.9f, 1.4f, sz }, 0.05f, 0.05f, 4, MADERA_OSCURA_MERCADO);
        m.Cilindro(Vector3{ sx + 0.9f, 0.0f, sz }, Vector3{ sx + 0.9f, 1.4f, sz }, 0.05f, 0.05f, 4, MADERA_OSCURA_MERCADO);
        m.Cilindro(Vector3{ sx - 0.9f, 1.35f, sz }, Vector3{ sx + 0.9f, 1.35f, sz }, 0.03f, 0.03f, 4, MADERA_OSCURA_MERCADO);

        for (int p = 0; p < 5; p++)
        {
            m.Caja(
                Vector3{ sx - 0.7f + p * 0.35f, 1.12f, sz },
                Vector3{ 0.1f, 0.4f, 0.04f },
                Color{ 196, 200, 196, 255 }
            );
        }
    }

    OcuparMercado(x, z, 2.3f, 3.0f);
    OcuparMercado(AHUMADERO_X, AHUMADERO_Z, 1.3f, 1.2f);
    OcuparMercado(-27.6f, -8.3f, 1.2f, 1.9f);
}


// Parte fija de un puesto: balsa o tarima, postes, mostrador y mercaderia.
// El toldo (color del dueno) va en su propia malla.
static void AgregarPuestoEstaticoMercado(
    ConstructorMalla& m,
    int indice,
    bool detalle
)
{
    Vector3 p = posicionPuestoMercado[indice];

    bool enAgua = !EsTierraMercado(p.x, p.z);

    EstablecerPiezaMercado(m, p.x, 0.0f, p.z, giroPuestoMercado[indice]);

    // Los puestos sobre el canal flotan sobre troncos.
    if (enAgua)
    {
        for (int k = -1; k <= 1; k++)
        {
            m.Cilindro(
                Vector3{ -0.66f, NIVEL_AGUA_MERCADO + 0.02f, k * 0.34f },
                Vector3{ 0.66f, NIVEL_AGUA_MERCADO + 0.02f, k * 0.34f },
                0.16f, 0.16f, 6,
                MADERA_OSCURA_MERCADO
            );
        }
    }

    float base = enAgua ? NIVEL_AGUA_MERCADO + 0.18f : 0.02f;

    m.Caja(Vector3{ 0.0f, base + 0.05f, 0.0f }, Vector3{ 1.3f, 0.10f, 1.04f }, MADERA_CLARA_MERCADO);

    // Postes: los de atras mas altos (el toldo cae hacia el frente).
    for (int sx = -1; sx <= 1; sx += 2)
    {
        for (int sz = -1; sz <= 1; sz += 2)
        {
            float alto = (sz < 0 ? 1.44f : 1.20f) - base;

            m.Caja(
                Vector3{ sx * 0.56f, base + alto * 0.5f, sz * 0.42f },
                Vector3{ 0.07f, alto, 0.07f },
                MADERA_OSCURA_MERCADO
            );
        }
    }

    // Mostrador al frente.
    m.Caja(Vector3{ 0.0f, base + 0.34f, 0.32f }, Vector3{ 1.04f, 0.48f, 0.28f }, MADERA_MERCADO);
    m.Caja(Vector3{ 0.0f, base + 0.60f, 0.32f }, Vector3{ 1.10f, 0.05f, 0.34f }, MADERA_CLARA_MERCADO);

    if (detalle)
    {
        // Mercaderia segun la zona: especias, flores, pescado o fruta.
        const Color colores[] =
        {
            Color{ 236, 96, 48, 255 },
            Color{ 246, 196, 54, 255 },
            Color{ 120, 186, 70, 255 },
            Color{ 196, 64, 120, 255 },
            Color{ 236, 140, 40, 255 },
            Color{ 176, 190, 200, 255 }
        };

        for (int k = 0; k < 3; k++)
        {
            int c = (int)(Hash2Mercado(indice, k) * 5.99f);

            m.Esfera(
                Vector3{ -0.32f + k * 0.32f, base + 0.71f, 0.32f },
                0.11f, 4, 6,
                colores[c]
            );
        }

        // Cajon y canasto al fondo.
        m.Caja(
            Vector3{ -0.27f, base + 0.22f, -0.24f },
            Vector3{ 0.38f, 0.34f, 0.30f },
            MADERA_OSCURA_MERCADO
        );

        m.Cilindro(
            Vector3{ 0.30f, base + 0.05f, -0.22f },
            Vector3{ 0.30f, base + 0.32f, -0.22f },
            0.15f, 0.18f, 8,
            Color{ 196, 160, 96, 255 }
        );
    }

    m.QuitarTransformacion();
}


//==================================================
// ESCENARIO: CASAS Y LANDMARKS (malla de edificios)
//==================================================
//
// Las casas se plantan en hileras escalonadas (mas pisos cuanto mas lejos
// del canal) en el barrio norte, una columna al oeste frente a la lonja y
// otras al este. Nada alto queda por delante de una casilla: el norte esta
// detras de los muelles y los costados quedan a mas de 2 de las franjas.

static int cantidadCasasMercado = 0;

// Frente de cada casa plantada (los props ponen macetas y cajones en la vereda).
struct FrenteCasaMercado
{
    float x, z;
    float dirX, dirZ;
    float ancho;
};

static const int MAX_CASAS_MERCADO = 160;
static FrenteCasaMercado frentesCasasMercado[MAX_CASAS_MERCADO];


// Huella libre para una casa: toda en tierra, lejos del sendero, fuera de
// landmarks, del dique y de lo ya ocupado.
static bool HuellaCasaLibreMercado(
    const Tablero& t,
    float cx, float cz,
    float medioX, float medioZ
)
{
    const float esquinas[5][2] =
    {
        { -1.0f, -1.0f }, { 1.0f, -1.0f }, { 1.0f, 1.0f }, { -1.0f, 1.0f }, { 0.0f, 0.0f }
    };

    for (const auto& e : esquinas)
    {
        float x = cx + e[0] * medioX;
        float z = cz + e[1] * medioZ;

        if (
            !EnManzanaMercado(x, z, -0.05f) ||
            EnDiqueMercado(x, z, 0.6f) ||
            DistanciaSenderoMercado(t, x, z) < 2.5f ||
            DistanciaReservadaMercado(x, z) < 0.3f ||
            EnFranjaFrontalMercado(t, x, z, 0.1f)
        )
        {
            return false;
        }
    }

    for (int i = 0; i < cantidadOcupadosMercado; i++)
    {
        const RectanguloMercado& r = ocupadosMercado[i];

        if (
            cx + medioX > r.x0 - 0.1f && cx - medioX < r.x1 + 0.1f &&
            cz + medioZ > r.z0 - 0.1f && cz - medioZ < r.z1 + 0.1f
        )
        {
            return false;
        }
    }

    return true;
}


// Planta una casa si cabe. giro 0 = frente al sur, 90 = este, -90 = oeste.
static void PlantarCasaMercado(
    ConstructorMalla& m,
    const Tablero& t,
    float cx, float cz,
    float giro,
    float ancho, float fondo,
    int pisos,
    bool detalle
)
{
    bool girada = std::fabs(giro) > 45.0f && std::fabs(giro) < 135.0f;
    float medioX = (girada ? fondo : ancho) * 0.5f;
    float medioZ = (girada ? ancho : fondo) * 0.5f;

    if (!HuellaCasaLibreMercado(t, cx, cz, medioX, medioZ))
    {
        return;
    }

    int semilla = (int)(cx * 13.0f) * 7 + (int)(cz * 7.0f) * 3 + 1000;

    if (semilla < 0)
    {
        semilla = -semilla;
    }

    AgregarCasaMercado(m, cx, cz, giro, ancho, fondo, pisos, semilla, detalle);
    OcuparMercado(cx, cz, medioX, medioZ);

    if (cantidadCasasMercado < MAX_CASAS_MERCADO)
    {
        float dirX = std::sin(giro * DEG2RAD);
        float dirZ = std::cos(giro * DEG2RAD);

        frentesCasasMercado[cantidadCasasMercado] =
            FrenteCasaMercado{ cx + dirX * fondo * 0.5f, cz + dirZ * fondo * 0.5f, dirX, dirZ, ancho };
    }

    cantidadCasasMercado++;
}


// Hilera de casas a lo largo de X, de x0 a x1, con anchos al azar.
static void HileraCasasMercado(
    ConstructorMalla& m,
    const Tablero& t,
    float x0, float x1,
    float z,
    float giro,
    int pisosMinimos,
    int fila,
    bool detalle
)
{
    float x = x0;
    int k = 0;

    while (x < x1)
    {
        float ancho = 2.2f + 1.0f * Hash2Mercado(fila * 31 + k, 5);
        float hueco = 0.15f + 0.35f * Hash2Mercado(fila * 31 + k, 6);
        float cx = x + ancho * 0.5f;

        if (cx + ancho * 0.5f > x1)
        {
            break;
        }

        int pisos = pisosMinimos + (Hash2Mercado(fila * 31 + k, 7) < 0.5f ? 0 : 1);
        float dz = (Hash2Mercado(fila * 31 + k, 8) - 0.5f) * 0.3f;

        PlantarCasaMercado(m, t, cx, z + dz, giro, ancho, 2.6f, pisos, detalle);

        x += ancho + hueco;
        k++;
    }
}


// Columna de casas a lo largo de Z (frente al este o al oeste).
static void ColumnaCasasMercado(
    ConstructorMalla& m,
    const Tablero& t,
    float x,
    float z0, float z1,
    float giro,
    int pisosMinimos,
    int fila,
    bool detalle
)
{
    float z = z0;
    int k = 0;

    while (z < z1)
    {
        float ancho = 2.3f + 0.8f * Hash2Mercado(fila * 17 + k, 15);
        float hueco = 0.2f + 0.3f * Hash2Mercado(fila * 17 + k, 16);
        float cz = z + ancho * 0.5f;

        if (cz + ancho * 0.5f > z1)
        {
            break;
        }

        int pisos = pisosMinimos + (Hash2Mercado(fila * 17 + k, 17) < 0.5f ? 0 : 1);

        PlantarCasaMercado(m, t, x, cz, giro, ancho, 2.4f, pisos, detalle);

        z += ancho + hueco;
        k++;
    }
}


static void ConstruirMallaEdificiosMercado(NivelCalidadGrafica nivel)
{
    ConstructorMalla& m = constructorMercado;
    m.Limpiar();
    m.reduccionDetalle = nivel == CALIDAD_ALTA ? 0 : 1;

    const Tablero& t = TableroReferenciaMercado();
    bool detalle = nivel != CALIDAD_BAJA;

    cantidadOcupadosMercado = 0;
    cantidadCasasMercado = 0;
    cantidadFarolesMercado = 0;

    // Los puestos ya tienen su lugar: nada se planta encima.
    for (int i = 0; i < t.cantidadCasillas; i++)
    {
        if (t.casillas[i].tipo == CASILLA_NEUTRA)
        {
            OcuparMercado(posicionPuestoMercado[i].x, posicionPuestoMercado[i].z, 0.85f, 0.85f);
        }
    }

    // --- Landmarks ---
    AgregarTorreCampanaMercado(m, detalle);
    AgregarFaroMercado(m, detalle);
    AgregarCasaFareroMercado(m, detalle);
    AgregarCascoAstilleroMercado(m, detalle);
    AgregarGruaPorticoMercado(m);

    for (float z : GRUA_ALTA_Z)
    {
        AgregarGruaAltaMercado(m, GRUA_ALTA_X, z, -90.0f);
    }

    AgregarAduanaMercado(m, detalle);
    AgregarAlmacenEspeciasMercado(m, detalle);
    AgregarBasilicaMercado(m, detalle);
    AgregarLonjaMercado(m, detalle);

    // --- Barrio norte: hileras escalonadas hacia el horizonte ---
    // En BAJA solo las tres primeras hileras (las de atras casi no se ven).
    const float filasNorte[6] = { -16.7f, -20.1f, -23.5f, -26.9f, -30.3f, -33.7f };
    int filas = nivel == CALIDAD_BAJA ? 3 : 6;

    for (int f = 0; f < filas; f++)
    {
        float desfase = (f % 2) * 1.1f;

        HileraCasasMercado(m, t, -30.6f + desfase, 4.6f, filasNorte[f], 0.0f, 2 + (f + 1) / 2, f, detalle);
    }

    // --- Barrio noroeste: columna frente al canal ---
    ColumnaCasasMercado(m, t, -16.4f, -13.4f, -5.6f, 90.0f, 2, 10, detalle);
    ColumnaCasasMercado(m, t, -29.6f, -13.4f, -5.6f, 90.0f, 3, 11, detalle);

    // --- Barrio este: columnas frente al canal y hacia el mar ---
    ColumnaCasasMercado(m, t, 16.0f, 1.2f, 9.4f, -90.0f, 2, 12, detalle);
    ColumnaCasasMercado(m, t, 25.6f, -1.4f, 9.4f, -90.0f, 3, 13, detalle);
    ColumnaCasasMercado(m, t, 28.6f, -1.4f, 9.4f, -90.0f, 3, 14, detalle);

    // --- Astillero: taller de la carpinteria de ribera ---
    PlantarCasaMercado(m, t, -25.0f, 14.2f, 0.0f, 3.4f, 2.4f, 1, detalle);

    IluminarMallaMercado(m, 0);
    ActualizarMallaGpu(mallaEdificiosMercado, m);
}


//==================================================
// ESCENARIO: PROPS (malla de decoracion menuda)
//==================================================
//
// Decoracion repartida en una grilla con desvio al azar, segun la zona:
//   - malecones: barriles, cajones, sacos de especias, macetas, bancos;
//   - astillero: pilas de madera, barriles, rollos de cuerda y anclas;
//   - Isla del Faro: rocas, pinos, arbustos y flores.
// Mas las bitas del malecon, los faroles junto a los muelles, la plaza, las
// barcas amarradas, los carteles de las bifurcaciones y la parte fija de
// los puestos. Calidad MEDIA ~60 % y BAJA ~20 % de lo menudo.

static int cantidadPropsMercado = 0;


// Lo menudo se recorta en MEDIA y BAJA (como la flora de Arboleda).
static bool ConservarMenudoMercado(int a, int b, NivelCalidadGrafica nivel)
{
    if (nivel == CALIDAD_ALTA)
    {
        return true;
    }

    float azar = Hash2Mercado(a * 3 + 101, b * 7 + 13);

    return azar < (nivel == CALIDAD_MEDIA ? 0.6f : 0.2f);
}


static float DistanciaPuestoMasCercanoMercado(const Tablero& t, float x, float z)
{
    float mejor = 1.0e9f;

    for (int i = 0; i < t.cantidadCasillas; i++)
    {
        if (t.casillas[i].tipo != CASILLA_NEUTRA)
        {
            continue;
        }

        float dx = posicionPuestoMercado[i].x - x;
        float dz = posicionPuestoMercado[i].z - z;
        float d = std::sqrt(dx * dx + dz * dz);

        if (d < mejor) mejor = d;
    }

    return mejor;
}


static void AgregarPropsDispersosMercado(
    ConstructorMalla& m,
    const Tablero& t,
    NivelCalidadGrafica nivel
)
{
    const float celda = 0.95f;

    for (int ix = 0; ix < 67; ix++)
    {
        for (int iz = 0; iz < 57; iz++)
        {
            float v = Hash2Mercado(ix * 5 + 3, iz * 11 + 7);
            float x = -31.0f + (ix + 0.15f + 0.7f * Hash2Mercado(ix, iz + 300)) * celda;
            float z = -38.0f + (iz + 0.15f + 0.7f * Hash2Mercado(ix + 300, iz)) * celda;

            if (!EsTierraMercado(x, z) || EnIslaPlazaMercado(x, z, 0.5f))
            {
                continue;
            }

            bool faro = DistanciaIslaFaroMercado(x, z) < 0.0f && !EnManzanaMercado(x, z, 0.0f);
            bool astillero =
                DentroRectanguloMercado(MANZANAS_MERCADO[2], x, z, 0.0f) ||
                DentroRectanguloMercado(MANZANAS_MERCADO[3], x, z, 0.0f);
            bool maleconSur = DentroRectanguloMercado(MANZANAS_MERCADO[4], x, z, 0.0f);

            float densidad = faro ? 0.95f : (astillero ? 0.7f : (maleconSur ? 0.22f : 0.56f));

            // Detras de la tercera hilera del barrio norte casi no se ve el suelo.
            if (z < -22.0f)
            {
                densidad *= 0.35f;
            }

            if (Hash2Mercado(ix + 77, iz + 91) > densidad)
            {
                continue;
            }

            if (!ConservarMenudoMercado(ix, iz, nivel))
            {
                continue;
            }

            float sendero = DistanciaSenderoMercado(t, x, z);

            if (
                sendero < 2.0f ||
                EnDiqueMercado(x, z, 0.7f) ||
                DistanciaReservadaMercado(x, z) < 0.2f ||
                !LibreDeOcupadosMercado(x, z, 0.4f) ||
                DistanciaPuestoMasCercanoMercado(t, x, z) < 1.25f
            )
            {
                continue;
            }

            // Lo que pasa de ~0.8 de alto no va en la franja frontal.
            bool permiteAlto =
                sendero >= 2.6f &&
                !maleconSur &&
                !EnFranjaFrontalMercado(t, x, z, 0.3f);

            float giro = v * 360.0f;
            float w = Hash2Mercado(ix + 13, iz + 29);

            if (faro)
            {
                float y = AlturaIslaFaroMercado(x, z);

                if (y < -0.12f)
                {
                    // Orilla: rocas medio hundidas.
                    AgregarRocaMercado(m, x, y - 0.1f, z, 0.35f + 0.35f * w, v);
                }
                else if (v < 0.24f && permiteAlto)
                {
                    AgregarPinoMercado(m, x, y, z, 0.8f + 0.5f * w, v);
                }
                else if (v < 0.48f)
                {
                    AgregarArbustoMercado(m, x, y, z, w);
                }
                else if (v < 0.70f)
                {
                    AgregarFloresMercado(m, x, y, z, w);
                }
                else
                {
                    AgregarRocaMercado(m, x, y - 0.05f, z, 0.22f + 0.3f * w, v);
                }

                // Matas de flores silvestres entre lo demas.
                if (y > -0.05f && w > 0.35f && DistanciaSenderoMercado(t, x + 0.45f, z - 0.35f) > 1.9f)
                {
                    AgregarFloresMercado(m, x + 0.45f, AlturaIslaFaroMercado(x + 0.45f, z - 0.35f), z - 0.35f, v);
                    cantidadPropsMercado++;
                }
            }
            else if (astillero)
            {
                if (v < 0.22f && permiteAlto)
                {
                    AgregarPilaMaderaMercado(m, x, 0.0f, z, giro, w);
                    OcuparMercado(x, z, 0.8f, 0.8f);
                }
                else if (v < 0.45f)
                {
                    AgregarBarrilMercado(m, x, 0.0f, z, w);
                }
                else if (v < 0.65f)
                {
                    AgregarCajonMercado(m, x, 0.0f, z, giro, w);
                }
                else if (v < 0.85f)
                {
                    AgregarRolloCuerdaMercado(m, x, 0.0f, z);
                }
                else
                {
                    AgregarAnclaMercado(m, x, 0.0f, z, giro);
                }
            }
            else
            {
                if (v < 0.22f)
                {
                    AgregarBarrilMercado(m, x, 0.0f, z, w);
                }
                else if (v < 0.44f)
                {
                    AgregarCajonMercado(m, x, 0.0f, z, giro, w);
                }
                else if (v < 0.58f)
                {
                    AgregarSacosMercado(m, x, 0.0f, z, giro, w);
                }
                else if (v < 0.80f || !permiteAlto)
                {
                    AgregarMacetaMercado(m, x, 0.0f, z, w, false);
                }
                else if (v < 0.90f)
                {
                    AgregarMacetaMercado(m, x, 0.0f, z, w, true);
                }
                else if (z < -14.0f)
                {
                    AgregarCipresMercado(m, x, 0.0f, z, 0.9f + 0.4f * w);
                }
                else if (w < 0.5f)
                {
                    AgregarFarolMercado(m, x, 0.0f, z, 1.5f);
                }
                else
                {
                    AgregarBancoMercado(m, x, 0.0f, z, giro);
                }
            }

            cantidadPropsMercado++;
        }
    }
}


// Bitas de amarre a lo largo de los bordes de manzana que dan al agua.
static void AgregarBitasMercado(ConstructorMalla& m, const Tablero& t, NivelCalidadGrafica nivel)
{
    const float paso = nivel == CALIDAD_BAJA ? 5.6f : 2.8f;

    for (int i = 0; i < CANTIDAD_MANZANAS_MERCADO; i++)
    {
        const RectanguloMercado& r = MANZANAS_MERCADO[i];

        for (int lado = 0; lado < 4; lado++)
        {
            float ax, az, bx, bz, nx, nz;

            switch (lado)
            {
                case 0: ax = r.x0; az = r.z1; bx = r.x1; bz = r.z1; nx = 0.0f; nz = 1.0f; break;
                case 1: ax = r.x0; az = r.z0; bx = r.x1; bz = r.z0; nx = 0.0f; nz = -1.0f; break;
                case 2: ax = r.x0; az = r.z0; bx = r.x0; bz = r.z1; nx = -1.0f; nz = 0.0f; break;
                default: ax = r.x1; az = r.z0; bx = r.x1; bz = r.z1; nx = 1.0f; nz = 0.0f; break;
            }

            float largo = std::fabs(bx - ax) + std::fabs(bz - az);
            int cantidad = (int)(largo / paso);

            for (int k = 1; k < cantidad; k++)
            {
                float u = (float)k / cantidad;
                float x = ax + (bx - ax) * u - nx * 0.45f;
                float z = az + (bz - az) * u - nz * 0.45f;

                if (
                    EsTierraMercado(x + nx * 0.9f, z + nz * 0.9f) ||
                    DistanciaSenderoMercado(t, x, z) < 1.9f ||
                    DistanciaPuestoMasCercanoMercado(t, x, z) < 1.0f ||
                    !LibreDeOcupadosMercado(x, z, 0.2f) ||
                    EnDiqueMercado(x, z, 0.4f)
                )
                {
                    continue;
                }

                AgregarBitaMercado(m, x, 0.0f, z);
                cantidadPropsMercado++;
            }
        }
    }
}


// Vereda de cada casa: macetas, cajones o un banco junto a la puerta.
static void AgregarVeredasCasasMercado(
    ConstructorMalla& m,
    const Tablero& t,
    NivelCalidadGrafica nivel
)
{
    int casas = cantidadCasasMercado < MAX_CASAS_MERCADO ? cantidadCasasMercado : MAX_CASAS_MERCADO;

    for (int i = 0; i < casas; i++)
    {
        const FrenteCasaMercado& f = frentesCasasMercado[i];

        // Hacia el costado de la fachada (perpendicular a su frente).
        float lx = f.dirZ;
        float lz = -f.dirX;

        for (int k = 0; k < 2; k++)
        {
            if (!ConservarMenudoMercado(i, k + 40, nivel))
            {
                continue;
            }

            float lado = (k == 0 ? -1.0f : 1.0f) * f.ancho * (0.28f + 0.1f * Hash2Mercado(i, k + 3));
            float x = f.x + f.dirX * 0.42f + lx * lado;
            float z = f.z + f.dirZ * 0.42f + lz * lado;

            if (
                !EsTierraMercado(x, z) ||
                DistanciaSenderoMercado(t, x, z) < 1.9f ||
                DistanciaPuestoMasCercanoMercado(t, x, z) < 1.1f ||
                DistanciaReservadaMercado(x, z) < 0.1f
            )
            {
                continue;
            }

            float v = Hash2Mercado(i * 3 + k, 57);

            if (v < 0.5f)
            {
                AgregarMacetaMercado(m, x, 0.0f, z, Hash2Mercado(i, k + 9), false);
            }
            else if (v < 0.7f)
            {
                AgregarCajonMercado(m, x, 0.0f, z, v * 360.0f, Hash2Mercado(i, k + 19));
            }
            else if (v < 0.85f)
            {
                AgregarBarrilMercado(m, x, 0.0f, z, Hash2Mercado(i, k + 29));
            }
            else
            {
                AgregarMacetaMercado(m, x, 0.0f, z, Hash2Mercado(i, k + 9), !EnFranjaFrontalMercado(t, x, z, 0.3f));
            }

            cantidadPropsMercado++;
        }
    }
}


// Rocas en la orilla de la Isla del Faro y escollera al pie de los
// malecones que dan al mar abierto.
static void AgregarRocasCostaMercado(
    ConstructorMalla& m,
    const Tablero& t,
    NivelCalidadGrafica nivel
)
{
    const float paso = 0.8f;

    for (int ix = 0; ix < 34; ix++)
    {
        for (int iz = 0; iz < 32; iz++)
        {
            float x = 4.0f + (ix + Hash2Mercado(ix + 500, iz)) * paso;
            float z = -25.5f + (iz + Hash2Mercado(ix, iz + 500)) * paso;
            float d = DistanciaIslaFaroMercado(x, z);

            if (d < -0.5f || d > 0.6f || Hash2Mercado(ix + 31, iz + 47) > 0.5f)
            {
                continue;
            }

            if (!ConservarMenudoMercado(ix + 200, iz, nivel) || DistanciaSenderoMercado(t, x, z) < 1.9f)
            {
                continue;
            }

            float y = std::fmax(AlturaIslaFaroMercado(x, z), NIVEL_AGUA_MERCADO - 0.25f);

            AgregarRocaMercado(m, x, y - 0.12f, z, 0.3f + 0.35f * Hash2Mercado(ix, iz + 7), Hash2Mercado(ix + 3, iz));
            cantidadPropsMercado++;
        }
    }

    // Escollera: solo donde el malecon da al mar (lejos del canal).
    for (int i = 0; i < CANTIDAD_MANZANAS_MERCADO; i++)
    {
        const RectanguloMercado& r = MANZANAS_MERCADO[i];

        for (int lado = 0; lado < 4; lado++)
        {
            float ax, az, bx, bz, nx, nz;

            switch (lado)
            {
                case 0: ax = r.x0; az = r.z1; bx = r.x1; bz = r.z1; nx = 0.0f; nz = 1.0f; break;
                case 1: ax = r.x0; az = r.z0; bx = r.x1; bz = r.z0; nx = 0.0f; nz = -1.0f; break;
                case 2: ax = r.x0; az = r.z0; bx = r.x0; bz = r.z1; nx = -1.0f; nz = 0.0f; break;
                default: ax = r.x1; az = r.z0; bx = r.x1; bz = r.z1; nx = 1.0f; nz = 0.0f; break;
            }

            float largo = std::fabs(bx - ax) + std::fabs(bz - az);
            int cantidad = (int)(largo / 1.1f);

            for (int k = 0; k < cantidad; k++)
            {
                float u = (k + 0.5f) / cantidad;
                float x = ax + (bx - ax) * u + nx * 0.5f;
                float z = az + (bz - az) * u + nz * 0.5f;

                if (
                    nz > 0.0f ||
                    Hash2Mercado(i * 50 + k, lado + 61) > 0.55f ||
                    !ConservarMenudoMercado(i * 50 + k, lado + 300, nivel) ||
                    EsTierraMercado(x, z) ||
                    DistanciaSenderoMercado(t, x, z) < 5.0f ||
                    (std::fabs(x) < CANAL_X + 1.0f && z > CANAL_Z_NORTE - 1.0f && z < CANAL_Z_SUR + 1.0f)
                )
                {
                    continue;
                }

                AgregarRocaMercado(
                    m, x, NIVEL_AGUA_MERCADO - 0.2f, z,
                    0.35f + 0.3f * Hash2Mercado(k, i + lado),
                    Hash2Mercado(k + lado, i)
                );

                cantidadPropsMercado++;
            }
        }
    }
}


// Boyas y nasas flotando en el mar, lejos de los senderos.
static void AgregarBoyasMarMercado(ConstructorMalla& m, const Tablero& t, NivelCalidadGrafica nivel)
{
    const int cantidad = nivel == CALIDAD_BAJA ? 8 : 30;
    int puestas = 0;

    for (int i = 0; i < cantidad * 3 && puestas < cantidad; i++)
    {
        float x = -38.0f + 76.0f * Hash2Mercado(i, 801);
        float z = -32.0f + 52.0f * Hash2Mercado(i, 802);

        if (
            EsTierraMercado(x, z) ||
            EsTierraMercado(x + 1.2f, z) ||
            EsTierraMercado(x - 1.2f, z) ||
            DistanciaSenderoMercado(t, x, z) < 2.6f
        )
        {
            continue;
        }

        if (Hash2Mercado(i, 803) < 0.6f)
        {
            Color c = Hash2Mercado(i, 804) < 0.5f ? Color{ 226, 64, 52, 255 } : Color{ 246, 196, 60, 255 };

            m.Cilindro(
                Vector3{ x, NIVEL_AGUA_MERCADO - 0.1f, z },
                Vector3{ x, NIVEL_AGUA_MERCADO + 0.3f, z },
                0.18f, 0.12f, 6,
                c
            );
            m.Cilindro(
                Vector3{ x, NIVEL_AGUA_MERCADO + 0.3f, z },
                Vector3{ x, NIVEL_AGUA_MERCADO + 0.62f, z },
                0.03f, 0.03f, 3,
                HIERRO_MERCADO
            );
        }
        else
        {
            // Nasa de pescador (jaula de madera) con su corcho.
            m.Caja(Vector3{ x, NIVEL_AGUA_MERCADO + 0.05f, z }, Vector3{ 0.5f, 0.24f, 0.36f }, MADERA_CLARA_MERCADO);
            m.Esfera(Vector3{ x + 0.5f, NIVEL_AGUA_MERCADO + 0.05f, z + 0.2f }, 0.1f, 3, 5, Color{ 236, 120, 48, 255 });
        }

        cantidadPropsMercado++;
        puestas++;
    }
}


// Faroles junto a los muelles (del lado de tierra, nunca delante de una
// casilla) y alrededor de la torre.
static void AgregarFarolesMercado(ConstructorMalla& m, const Tablero& t)
{
    for (int k = 0; k < CASILLAS_MUELLE; k += 2)
    {
        const Casilla& c = t.casillas[nodoMercado.anillo[k]];

        // Hacia afuera del canal.
        float nx = 0.0f;
        float nz = 0.0f;

        if (c.posicion.z <= ANILLO_Z_NORTE + 0.1f) nz = -1.0f;
        else if (c.posicion.x >= ANILLO_X - 0.1f) nx = 1.0f;
        else if (c.posicion.x <= -ANILLO_X + 0.1f) nx = -1.0f;
        else continue;

        for (int intento = 0; intento < 3; intento++)
        {
            float lateral = (intento - 1) * 1.7f;
            float x = c.posicion.x + nx * 2.5f + nz * lateral;
            float z = c.posicion.z + nz * 2.5f + nx * lateral;

            if (
                !EsTierraMercado(x, z) ||
                DistanciaSenderoMercado(t, x, z) < 2.3f ||
                DistanciaPuestoMasCercanoMercado(t, x, z) < 1.3f ||
                EnFranjaFrontalMercado(t, x, z, 0.2f) ||
                !LibreDeOcupadosMercado(x, z, 0.3f)
            )
            {
                continue;
            }

            AgregarFarolMercado(m, x, 0.0f, z, 1.5f);
            cantidadPropsMercado++;
            break;
        }
    }

    // Plaza: dos faroles a los lados de la torre.
    AgregarFarolMercado(m, PLAZA_CX - 2.3f, 0.02f, PLAZA_CZ, 1.7f);
    AgregarFarolMercado(m, PLAZA_CX + 2.3f, 0.02f, PLAZA_CZ, 1.7f);
    cantidadPropsMercado += 2;
}


// Plaza: bancos, macetas y la fuente baja del frente de la torre.
static void AgregarPlazaMercado(ConstructorMalla& m, bool detalle)
{
    AgregarBancoMercado(m, PLAZA_CX - 1.55f, 0.02f, PLAZA_CZ + 1.2f, 127.7f);
    AgregarBancoMercado(m, PLAZA_CX + 1.55f, 0.02f, PLAZA_CZ + 1.2f, -127.7f);

    AgregarMacetaMercado(m, PLAZA_CX - 1.45f, 0.02f, PLAZA_CZ - 1.45f, 0.1f, false);
    AgregarMacetaMercado(m, PLAZA_CX + 1.45f, 0.02f, PLAZA_CZ - 1.45f, 0.6f, false);
    AgregarMacetaMercado(m, PLAZA_CX - 2.2f, 0.02f, PLAZA_CZ + 0.6f, 0.35f, false);
    AgregarMacetaMercado(m, PLAZA_CX + 2.2f, 0.02f, PLAZA_CZ + 0.6f, 0.85f, false);

    // Fuente baja delante de la torre (no tapa nada).
    m.Cilindro(
        Vector3{ PLAZA_CX, 0.02f, PLAZA_CZ + 1.9f },
        Vector3{ PLAZA_CX, 0.36f, PLAZA_CZ + 1.9f },
        0.78f, 0.78f, 16,
        PIEDRA_CLARA_MERCADO
    );

    m.Cilindro(
        Vector3{ PLAZA_CX, 0.30f, PLAZA_CZ + 1.9f },
        Vector3{ PLAZA_CX, 0.37f, PLAZA_CZ + 1.9f },
        0.66f, 0.66f, 16,
        EmisivoMercado(Color{ 96, 186, 206, 255 })
    );

    if (detalle)
    {
        m.Cilindro(
            Vector3{ PLAZA_CX, 0.3f, PLAZA_CZ + 1.9f },
            Vector3{ PLAZA_CX, 0.75f, PLAZA_CZ + 1.9f },
            0.12f, 0.08f, 8,
            PIEDRA_CLARA_MERCADO
        );

        m.Cilindro(
            Vector3{ PLAZA_CX, 0.75f, PLAZA_CZ + 1.9f },
            Vector3{ PLAZA_CX, 0.82f, PLAZA_CZ + 1.9f },
            0.3f, 0.3f, 10,
            PIEDRA_CLARA_MERCADO
        );
    }

    // Mastiles de los banderines (al este y al oeste de la isla).
    for (int lado = -1; lado <= 1; lado += 2)
    {
        float x = PLAZA_CX + lado * (RADIO_ISLA_MERCADO - 0.25f);

        m.Cilindro(Vector3{ x, 0.0f, PLAZA_CZ }, Vector3{ x, 3.6f, PLAZA_CZ }, 0.06f, 0.05f, 5, MADERA_OSCURA_MERCADO);
        m.Esfera(Vector3{ x, 3.66f, PLAZA_CZ }, 0.09f, 3, 5, Color{ 246, 200, 70, 255 });
    }

    cantidadPropsMercado += 9;
}


// Barcas amarradas en el canal y frente a los malecones (las que chocan con
// un sendero o la costa no se ponen).
static void AgregarBarcasAmarradasMercado(ConstructorMalla& m, const Tablero& t)
{
    struct BarcaFija
    {
        float x, z, giro, largo;
        Color casco;
        bool carga;
    };

    const BarcaFija barcas[] =
    {
        { 6.6f, 5.0f, 75.0f, 1.8f, Color{ 52, 120, 160, 255 }, true },
        { -6.4f, 5.1f, 105.0f, 1.7f, Color{ 176, 64, 52, 255 }, true },
        { 7.4f, -9.0f, 95.0f, 1.8f, Color{ 64, 132, 82, 255 }, false },
        { -6.6f, -9.1f, 80.0f, 1.8f, Color{ 214, 160, 60, 255 }, true },
        { 9.6f, -4.6f, 10.0f, 1.7f, Color{ 120, 70, 150, 255 }, false },
        { -9.5f, -4.4f, 170.0f, 1.8f, Color{ 52, 120, 160, 255 }, true },
        { -2.5f, 14.4f, 90.0f, 2.2f, Color{ 196, 70, 54, 255 }, false },
        { 4.8f, 14.0f, 80.0f, 2.0f, Color{ 52, 110, 150, 255 }, true },
        { 11.6f, 14.3f, 100.0f, 2.2f, Color{ 230, 214, 180, 255 }, false },
        { 19.2f, 13.9f, 85.0f, 2.0f, Color{ 64, 132, 82, 255 }, true },
        { 26.4f, 14.2f, 95.0f, 2.3f, Color{ 176, 64, 52, 255 }, false },
        { 33.0f, 4.0f, 0.0f, 2.2f, Color{ 214, 160, 60, 255 }, false },
        { 33.2f, -0.4f, 10.0f, 2.0f, Color{ 52, 120, 160, 255 }, true },
        { -30.0f, 2.0f, 0.0f, 2.2f, Color{ 120, 70, 150, 255 }, false },
        { -30.2f, 6.6f, 170.0f, 2.0f, Color{ 196, 70, 54, 255 }, true },
        { 6.8f, -22.6f, 120.0f, 2.0f, Color{ 230, 214, 180, 255 }, true },
        { 10.4f, -24.2f, 75.0f, 2.2f, Color{ 52, 110, 150, 255 }, false }
    };

    for (const BarcaFija& b : barcas)
    {
        float ux = std::sin(b.giro * DEG2RAD) * b.largo * 0.55f;
        float uz = std::cos(b.giro * DEG2RAD) * b.largo * 0.55f;

        bool choca = false;

        for (int k = -1; k <= 1; k++)
        {
            float x = b.x + ux * k;
            float z = b.z + uz * k;

            if (EsTierraMercado(x, z) || DistanciaSenderoMercado(t, x, z) < 1.9f)
            {
                choca = true;
            }
        }

        if (choca)
        {
            continue;
        }

        AgregarBarcaAmarradaMercado(m, b.x, b.z, b.giro, b.largo, b.casco, b.carga);
        cantidadPropsMercado++;
    }
}


// Un cartel por bifurcacion: poste con dos tablas que apuntan a cada camino
// (madera = opcion 0, rojiza = atajo). Se busca un lugar junto a la casilla
// fuera del sendero y de las franjas frontales; sobre el agua va en un pilote.
static void AgregarCartelesMercado(ConstructorMalla& m, const Tablero& t)
{
    const float desvios[6][2] =
    {
        { -1.8f, 1.4f }, { 1.8f, 1.4f }, { -2.0f, -1.3f },
        { 2.0f, -1.3f }, { -2.3f, 0.0f }, { 2.3f, 0.0f }
    };

    for (int i = 0; i < t.cantidadCasillas; i++)
    {
        const Casilla& c = t.casillas[i];

        if (c.cantidadConexiones < 2)
        {
            continue;
        }

        float mejorPuntaje = -1.0e9f;
        float px = c.posicion.x + desvios[0][0];
        float pz = c.posicion.z + desvios[0][1];

        for (int k = 0; k < 6; k++)
        {
            float x = c.posicion.x + desvios[k][0];
            float z = c.posicion.z + desvios[k][1];

            float puntaje = DistanciaSenderoMercado(t, x, z);

            if (puntaje > 1.6f) puntaje = 1.6f + (6 - k) * 0.01f;

            if (DistanciaPuestoMasCercanoMercado(t, x, z) < 0.9f) puntaje -= 2.0f;

            // Franja frontal de otra casilla (la propia la tolera, como Arboleda).
            for (int a = 0; a < t.cantidadCasillas; a++)
            {
                float dx = x - t.casillas[a].posicion.x;
                float dz = z - t.casillas[a].posicion.z;

                if (a != i && std::fabs(dx) < 2.0f && dz > 0.4f && dz < 3.4f)
                {
                    puntaje -= 1.0f;
                }
            }

            if (puntaje > mejorPuntaje)
            {
                mejorPuntaje = puntaje;
                px = x;
                pz = z;
            }
        }

        float y = 0.0f;

        if (DistanciaIslaFaroMercado(px, pz) < 0.0f && !EnManzanaMercado(px, pz, 0.0f))
        {
            y = AlturaIslaFaroMercado(px, pz);
        }

        if (!EsTierraMercado(px, pz))
        {
            y = NIVEL_AGUA_MERCADO - 0.4f;
            PiloteMercado(m, px, pz, 0.1f);
        }

        m.Cilindro(
            Vector3{ px, y, pz }, Vector3{ px, 1.9f, pz },
            0.07f, 0.08f, 5,
            MADERA_OSCURA_MERCADO
        );

        for (int o = 0; o < 2; o++)
        {
            const Casilla* destino = t.ObtenerCasilla(c.conexiones[o].destino);

            if (destino == nullptr)
            {
                continue;
            }

            float dx = destino->posicion.x - c.posicion.x;
            float dz = destino->posicion.z - c.posicion.z;
            Color tabla = o == 0 ? Color{ 196, 150, 96, 255 } : Color{ 214, 110, 86, 255 };

            // La tabla apunta al destino: +X local hacia el camino.
            EstablecerPiezaMercado(m, px, 1.6f - 0.4f * o, pz, std::atan2(-dz, dx) * RAD2DEG);

            m.Caja(Vector3{ 0.55f, 0.0f, 0.0f }, Vector3{ 1.1f, 0.22f, 0.07f }, tabla);
            m.Caja(Vector3{ 1.15f, 0.0f, 0.0f }, Vector3{ 0.2f, 0.12f, 0.07f }, tabla);

            m.QuitarTransformacion();
        }

        cantidadPropsMercado++;
    }
}


static void ConstruirMallaPropsMercado(NivelCalidadGrafica nivel)
{
    ConstructorMalla& m = constructorMercado;
    m.Limpiar();
    m.reduccionDetalle = nivel == CALIDAD_ALTA ? 0 : 1;

    const Tablero& t = TableroReferenciaMercado();
    bool detalle = nivel != CALIDAD_BAJA;

    cantidadPropsMercado = 0;

    // Puestos (parte fija; el toldo va aparte).
    for (int i = 0; i < t.cantidadCasillas; i++)
    {
        if (t.casillas[i].tipo == CASILLA_NEUTRA)
        {
            AgregarPuestoEstaticoMercado(m, i, detalle);
        }
    }

    AgregarPlazaMercado(m, detalle);
    AgregarFarolesMercado(m, t);
    AgregarCartelesMercado(m, t);
    AgregarBarcasAmarradasMercado(m, t);
    AgregarBitasMercado(m, t, nivel);
    AgregarVeredasCasasMercado(m, t, nivel);
    AgregarRocasCostaMercado(m, t, nivel);
    AgregarBoyasMarMercado(m, t, nivel);
    AgregarPropsDispersosMercado(m, t, nivel);

    IluminarMallaMercado(m, 0);
    ActualizarMallaGpu(mallaPropsMercado, m);
}


//==================================================
// ANIMACIONES (modo inmediato: solo lo que se mueve o es translucido)
//==================================================
//
// Presupuesto en ALTA: oleaje ~150 cuadros, destellos 24, gondolas y
// barcas ~40 cubos, gaviotas 20, haz del faro, halos de faroles, humo,
// banderines ~40 y la campana: menos de 600 primitivas por frame.

static const Color CASCO_GONDOLA_MERCADO = Color{ 36, 32, 40, 255 };


static void DibujarDestellosMercado(float tiempo)
{
    if (!oleajeListoMercado)
    {
        PrepararOleajeMercado();
    }

    int cantidad = (int)(24.0f * FactorCalidadGrafica(CalidadEfectos()));

    if (cantidadCeldasOleajeMercado == 0)
    {
        return;
    }

    rlDisableBackfaceCulling();

    for (int i = 0; i < cantidad; i++)
    {
        // Cada destello vive un ciclo corto en una celda al azar.
        float ciclo = tiempo * 0.7f + Hash2Mercado(i, 61) * 10.0f;
        int vuelta = (int)ciclo;
        float fase = ciclo - vuelta;
        float brillo = std::sin(fase * 3.14159265f);

        int celda = (int)(Hash2Mercado(i * 17 + vuelta, 63) * (cantidadCeldasOleajeMercado - 1));
        Vector2 c = celdasOleajeMercado[celda];

        float x = c.x + (Hash2Mercado(i, vuelta + 5) - 0.5f) * 3.0f;
        float z = c.y + (Hash2Mercado(vuelta, i + 9) - 0.5f) * 3.0f;
        float lado = 0.10f + 0.12f * brillo;

        CuadroAguaMercado(
            x, NIVEL_AGUA_MERCADO + 0.02f, z,
            lado * 1.6f, lado * 0.5f,
            Color{ 255, 236, 196, (unsigned char)(200.0f * brillo) }
        );
    }

    rlEnableBackfaceCulling();
}


// Barca con balanceo (frente = +Z local tras el giro).
static void DibujarBarcaMercado(
    Vector3 centro,
    float giro,
    float ancho,
    float largo,
    Color casco,
    float fase,
    float tiempo,
    bool carga
)
{
    float balanceo = std::sin(tiempo * 1.6f + fase) * 0.03f;

    rlPushMatrix();
    rlTranslatef(centro.x, centro.y + balanceo, centro.z);
    rlRotatef(giro, 0.0f, 1.0f, 0.0f);
    rlRotatef(std::sin(tiempo * 1.1f + fase) * 2.5f, 0.0f, 0.0f, 1.0f);

    Color interior = Color{ 176, 132, 86, 255 };

    DrawCube(Vector3{ 0.0f, 0.0f, 0.0f }, ancho, 0.30f, largo, casco);
    DrawCube(Vector3{ 0.0f, 0.10f, 0.0f }, ancho - 0.16f, 0.14f, largo - 0.30f, interior);

    // Proa y popa levantadas.
    DrawCube(Vector3{ 0.0f, 0.16f, largo * 0.5f + 0.10f }, ancho * 0.5f, 0.38f, 0.26f, casco);
    DrawCube(Vector3{ 0.0f, 0.16f, -largo * 0.5f - 0.10f }, ancho * 0.5f, 0.38f, 0.26f, casco);

    if (carga)
    {
        DrawCube(Vector3{ 0.08f, 0.28f, -0.30f }, 0.30f, 0.24f, 0.30f, MADERA_OSCURA_MERCADO);
        DrawCube(Vector3{ -0.12f, 0.26f, 0.22f }, 0.24f, 0.2f, 0.24f, Color{ 236, 120, 48, 255 });
    }

    rlPopMatrix();
}


// Gondolas bajo las casillas del canal (la losa va encima) con su
// gondolero que rema en la popa.
static void DibujarGondolasMercado(const PartidaTablero& partida, float tiempo)
{
    const Tablero& t = partida.tablero;

    const int gondolas[2][2] =
    {
        { nodoMercado.bifurcacionGondolaEste, nodoMercado.gondolaEste },
        { nodoMercado.bifurcacionGondolaOeste, nodoMercado.gondolaOeste }
    };

    for (int k = 0; k < 2; k++)
    {
        const Casilla* muelle = t.ObtenerCasilla(gondolas[k][0]);
        const Casilla* casilla = t.ObtenerCasilla(gondolas[k][1]);

        if (muelle == nullptr || casilla == nullptr)
        {
            continue;
        }

        float giro =
            std::atan2(
                casilla->posicion.x - muelle->posicion.x,
                casilla->posicion.z - muelle->posicion.z
            ) * RAD2DEG;

        Vector3 centro = casilla->posicion;
        centro.y = NIVEL_AGUA_MERCADO + 0.14f;

        DibujarBarcaMercado(centro, giro, 1.45f, 3.1f, CASCO_GONDOLA_MERCADO, k * 1.7f, tiempo, false);

        // Ferro de proa y gondolero con la pertiga.
        float remo = std::sin(tiempo * 1.3f + k) * 18.0f;

        rlPushMatrix();
        rlTranslatef(centro.x, centro.y, centro.z);
        rlRotatef(giro, 0.0f, 1.0f, 0.0f);

        DrawCube(Vector3{ 0.0f, 0.42f, 1.62f }, 0.08f, 0.4f, 0.12f, Color{ 220, 214, 200, 255 });

        DrawCube(Vector3{ 0.0f, 0.62f, -1.35f }, 0.24f, 0.62f, 0.2f, Color{ 238, 238, 232, 255 });
        DrawCube(Vector3{ 0.0f, 0.68f, -1.35f }, 0.26f, 0.1f, 0.22f, Color{ 52, 64, 120, 255 });
        DrawCube(Vector3{ 0.0f, 1.03f, -1.35f }, 0.18f, 0.18f, 0.18f, Color{ 236, 190, 150, 255 });
        DrawCylinder(Vector3{ 0.0f, 1.12f, -1.35f }, 0.2f, 0.2f, 0.05f, 8, Color{ 236, 214, 120, 255 });

        rlPushMatrix();
        rlTranslatef(0.18f, 0.9f, -1.35f);
        rlRotatef(remo, 1.0f, 0.0f, 0.0f);
        DrawCube(Vector3{ 0.0f, -0.4f, -0.2f }, 0.05f, 1.6f, 0.05f, MADERA_OSCURA_MERCADO);
        rlPopMatrix();

        rlPopMatrix();
    }
}


// Barcas y un velero que navegan lejos de los senderos.
static void DibujarBarcasNavegandoMercado(float tiempo)
{
    if (CalidadDecoracion() == CALIDAD_BAJA)
    {
        return;
    }

    const float y = NIVEL_AGUA_MERCADO + 0.10f;

    // Barca de remos que cruza frente al malecon sur.
    float a = std::fmod(tiempo * 0.9f, 64.0f);
    DibujarBarcaMercado(Vector3{ -30.0f + a, y, 17.6f }, 90.0f, 0.8f, 1.9f, Color{ 214, 160, 60, 255 }, 0.9f, tiempo, true);

    // Bote que va y viene por el mar del faro.
    float b = std::sin(tiempo * 0.12f);
    DibujarBarcaMercado(
        Vector3{ 8.0f + 3.0f * b, y, -24.5f + 1.2f * b },
        b > 0.0f ? 70.0f : 250.0f, 0.8f, 1.8f, Color{ 64, 132, 82, 255 }, 2.1f, tiempo, false
    );

    // Velero lejano detras del puerto (hacia el sol).
    float c = std::fmod(tiempo * 0.6f, 120.0f) - 60.0f;
    float balanceo = std::sin(tiempo * 1.2f) * 3.0f;

    rlPushMatrix();
    rlTranslatef(c, y, -46.0f);
    rlRotatef(90.0f, 0.0f, 1.0f, 0.0f);
    rlRotatef(balanceo, 0.0f, 0.0f, 1.0f);

    DrawCube(Vector3{ 0.0f, 0.1f, 0.0f }, 1.1f, 0.5f, 3.6f, Color{ 120, 70, 54, 255 });
    DrawCube(Vector3{ 0.0f, 2.2f, 0.0f }, 0.08f, 4.0f, 0.08f, MADERA_OSCURA_MERCADO);
    DrawTriangle3D(Vector3{ 0.05f, 4.0f, 0.0f }, Vector3{ 0.05f, 0.6f, 0.0f }, Vector3{ 0.05f, 0.6f, -1.6f }, Color{ 246, 236, 216, 255 });
    DrawTriangle3D(Vector3{ -0.05f, 4.0f, 0.0f }, Vector3{ -0.05f, 0.6f, -1.6f }, Vector3{ -0.05f, 0.6f, 0.0f }, Color{ 236, 224, 204, 255 });

    rlPopMatrix();
}


static void DibujarGaviotasMercado(float tiempo)
{
    int cantidad = (int)std::ceil(10.0f * FactorCalidadGrafica(CalidadParticulas()));

    rlDisableBackfaceCulling();

    for (int i = 0; i < cantidad; i++)
    {
        float radio = 6.0f + 9.0f * Hash2Mercado(i, 71);
        float velocidad = (0.18f + 0.12f * Hash2Mercado(i, 72)) * (i % 2 == 0 ? 1.0f : -1.0f);
        float angulo = tiempo * velocidad + i * 1.9f;

        float cx = -4.0f + 18.0f * (Hash2Mercado(i, 73) - 0.5f);
        float cz = -12.0f + 14.0f * (Hash2Mercado(i, 74) - 0.5f);

        float x = cx + std::cos(angulo) * radio;
        float z = cz + std::sin(angulo) * radio * 0.6f;
        float y = 7.5f + 2.0f * Hash2Mercado(i, 75) + 0.4f * std::sin(tiempo * 0.7f + i);

        // Direccion de vuelo (tangente) y aleteo.
        float dx = -std::sin(angulo) * (velocidad > 0.0f ? 1.0f : -1.0f);
        float dz = std::cos(angulo) * 0.6f * (velocidad > 0.0f ? 1.0f : -1.0f);
        float largo = std::sqrt(dx * dx + dz * dz);

        dx /= largo;
        dz /= largo;

        float nx = -dz;
        float nz = dx;
        float aleteo = 0.28f * std::sin(tiempo * 7.0f + i * 2.3f);

        Vector3 cuerpo = { x, y, z };
        Vector3 cola = { x - dx * 0.3f, y, z - dz * 0.3f };
        Vector3 izquierda = { x + nx * 0.62f - dx * 0.1f, y + aleteo, z + nz * 0.62f - dz * 0.1f };
        Vector3 derecha = { x - nx * 0.62f - dx * 0.1f, y + aleteo, z - nz * 0.62f - dz * 0.1f };

        Color blanco = Color{ 248, 246, 240, 255 };

        DrawTriangle3D(cuerpo, izquierda, cola, blanco);
        DrawTriangle3D(cuerpo, cola, derecha, Color{ 226, 224, 220, 255 });
    }

    rlEnableBackfaceCulling();
}


// Haz giratorio del faro y resplandor de la linterna.
static void DibujarHazFaroMercado(float tiempo)
{
    float y = AlturaBaseFaroMercado() + 0.6f + 9.4f + 0.8f;
    float pulso = 0.5f + 0.5f * std::sin(tiempo * 3.0f);

    DrawSphereEx(
        Vector3{ FARO_X, y, FARO_Z },
        0.85f + 0.08f * pulso, 5, 8,
        Color{ 255, 236, 170, (unsigned char)(60 + 30 * pulso) }
    );

    if (CalidadEfectos() == CALIDAD_BAJA)
    {
        return;
    }

    rlDisableBackfaceCulling();

    for (int k = 0; k < 2; k++)
    {
        float a = tiempo * 0.9f + k * 3.14159265f;
        float dx = std::cos(a);
        float dz = std::sin(a);
        float nx = -dz;
        float nz = dx;
        const float largo = 11.0f;

        Vector3 origen = { FARO_X, y, FARO_Z };
        Vector3 p1 = { FARO_X + dx * largo + nx * 1.6f, y - 0.9f, FARO_Z + dz * largo + nz * 1.6f };
        Vector3 p2 = { FARO_X + dx * largo - nx * 1.6f, y - 0.9f, FARO_Z + dz * largo - nz * 1.6f };

        DrawTriangle3D(origen, p1, p2, Color{ 255, 240, 190, 46 });
    }

    rlEnableBackfaceCulling();
}


// Halos de los faroles: un cubo translucido que late.
static void DibujarHalosFarolesMercado(float tiempo)
{
    if (CalidadEfectos() == CALIDAD_BAJA)
    {
        return;
    }

    for (int i = 0; i < cantidadFarolesMercado; i++)
    {
        float pulso = 0.5f + 0.5f * std::sin(tiempo * 2.3f + i * 1.7f);

        DrawCube(
            farolesMercado[i],
            0.42f + 0.05f * pulso, 0.44f + 0.05f * pulso, 0.42f + 0.05f * pulso,
            Color{ 255, 214, 130, (unsigned char)(50 + 40 * pulso) }
        );
    }
}


// Humo del ahumadero de la lonja.
static void DibujarHumoMercado(float tiempo)
{
    int bocanadas = (int)std::ceil(8.0f * FactorCalidadGrafica(CalidadParticulas()));

    for (int i = 0; i < bocanadas; i++)
    {
        float fase = std::fmod(tiempo * 0.22f + i / (float)bocanadas, 1.0f);
        float y = ALTURA_AHUMADERO + 0.2f + fase * 3.4f;
        float deriva = fase * fase * 2.2f;

        DrawSphereEx(
            Vector3{ AHUMADERO_X + deriva, y, AHUMADERO_Z - deriva * 0.4f + 0.2f * std::sin(tiempo + i) },
            0.32f + fase * 0.7f, 4, 6,
            Color{ 214, 206, 200, (unsigned char)(150.0f * (1.0f - fase)) }
        );
    }
}


// Banderines de colores de la torre a los mastiles de la isla.
static void DibujarBanderinesMercado(float tiempo)
{
    const Color colores[5] =
    {
        Color{ 226, 72, 64, 255 },
        Color{ 246, 196, 60, 255 },
        Color{ 64, 156, 200, 255 },
        Color{ 96, 176, 96, 255 },
        Color{ 236, 120, 170, 255 }
    };

    int banderines = CalidadDecoracion() == CALIDAD_BAJA ? 5 : 10;

    rlDisableBackfaceCulling();

    for (int lado = -1; lado <= 1; lado += 2)
    {
        Vector3 a = { PLAZA_CX + lado * 0.95f, 8.05f, PLAZA_CZ };
        Vector3 b = { PLAZA_CX + lado * (RADIO_ISLA_MERCADO - 0.25f), 3.55f, PLAZA_CZ };

        DrawLine3D(a, b, Color{ 80, 64, 52, 255 });

        for (int k = 1; k <= banderines; k++)
        {
            float u = (float)k / (banderines + 1);
            float comba = 0.55f * std::sin(u * 3.14159265f);

            Vector3 p =
            {
                a.x + (b.x - a.x) * u,
                a.y + (b.y - a.y) * u - comba,
                a.z
            };

            float flameo = 0.12f * std::sin(tiempo * 5.0f + k * 1.3f + lado);

            DrawTriangle3D(
                Vector3{ p.x - 0.14f, p.y, p.z },
                Vector3{ p.x + 0.14f, p.y, p.z },
                Vector3{ p.x, p.y - 0.36f, p.z + flameo },
                colores[(k + (lado > 0 ? 2 : 0)) % 5]
            );
        }
    }

    // Bandera de la aduana.
    float flameo = std::sin(tiempo * 4.0f);

    DrawTriangle3D(
        Vector3{ 23.0f, 6.35f, 7.4f },
        Vector3{ 23.0f, 5.75f, 7.4f },
        Vector3{ 23.0f + 1.0f, 6.05f + 0.06f * flameo, 7.4f + 0.18f * flameo },
        Color{ 38, 176, 178, 255 }
    );

    rlEnableBackfaceCulling();
}


//==================================================
// DECORACION (entrada del dibujo)
//==================================================

// Reconstruye las mallas estaticas solo al cambiar la calidad.
static void AsegurarMallasMercado()
{
    NivelCalidadGrafica nivel = CalidadDecoracion();

    if (nivelMallasMercado == (int)nivel && mallaMarMercado.cargada)
    {
        return;
    }

    ConstruirMallaMarMercado(nivel);
    ConstruirMallaTierraMercado(nivel);
    ConstruirMallaSenderosMercado(nivel);

    // Los edificios ocupan su lugar antes de repartir los props.
    ConstruirMallaEdificiosMercado(nivel);
    ConstruirMallaPropsMercado(nivel);

    nivelMallasMercado = (int)nivel;
}


static void DibujarDecoracionMercado(
    const PartidaTablero& partida
)
{
    float tiempo = partida.tiempoTablero;

    AsegurarMallasMercado();

    DibujarMallaGpu(mallaMarMercado);
    DibujarMallaGpu(mallaTierraMercado);
    DibujarMallaGpu(mallaSenderosMercado);
    DibujarMallaGpu(mallaEdificiosMercado);
    DibujarMallaGpu(mallaPropsMercado);

    DibujarGondolasMercado(partida, tiempo);
    DibujarBarcasNavegandoMercado(tiempo);

    // Translucidos al final.
    DibujarOleajeMercado(tiempo);
    DibujarDestellosMercado(tiempo);
    DibujarBanderinesMercado(tiempo);
    DibujarGaviotasMercado(tiempo);
    DibujarHumoMercado(tiempo);
    DibujarHalosFarolesMercado(tiempo);
    DibujarHazFaroMercado(tiempo);
}


//==================================================
// GIMMICK (toldos de los puestos y campana)
//==================================================
//
// Los toldos van en su propia malla: la geometria no cambia y, cuando un
// puesto cambia de dueno, solo se reescriben los colores en la GPU.

static MallaGpu mallaToldosMercado;
static int firmaToldosMercado[MAX_CASILLAS_TABLERO];
static int nivelToldosMercado = -1;


// Color de la ficha del participante (si no hay participantes, como en la
// vista previa del menu, uno parecido de reserva).
static Color ColorJugadorMercado(
    const PartidaTablero& partida,
    int jugador
)
{
    if (
        partida.participantes != nullptr &&
        jugador >= 0 &&
        jugador < MAX_PARTICIPANTES
    )
    {
        return partida.participantes[jugador].color;
    }

    const Color reserva[4] =
    {
        Color{ 230, 60, 60, 255 },
        Color{ 60, 120, 230, 255 },
        Color{ 60, 190, 90, 255 },
        Color{ 240, 200, 50, 255 }
    };

    return reserva[(jugador < 0 ? 0 : jugador) % 4];
}


// Toldo a rayas: gris si el puesto esta libre, del color del dueno si no.
// Lona inclinada (alta atras, baja adelante: +Z local = la casilla) y faldon.
static void AgregarToldoMercado(
    ConstructorMalla& m,
    Vector3 posicion,
    float giro,
    bool libre,
    Color dueno
)
{
    Color claro =
        libre
        ? Color{ 222, 220, 214, 255 }
        : Color{ 248, 242, 228, 255 };

    Color franja =
        libre
        ? Color{ 146, 148, 156, 255 }
        : dueno;

    EstablecerPiezaMercado(m, posicion.x, 0.0f, posicion.z, giro);

    for (int k = 0; k < 4; k++)
    {
        float x0 = -0.66f + 0.33f * k;
        float x1 = x0 + 0.33f;
        Color lona = (k % 2 == 0) ? franja : claro;
        Color faldon = (k % 2 == 0) ? claro : franja;

        m.TrianguloDobleCara({ x0, 1.48f, -0.56f }, { x1, 1.48f, -0.56f }, { x1, 1.22f, 0.56f }, lona);
        m.TrianguloDobleCara({ x0, 1.48f, -0.56f }, { x1, 1.22f, 0.56f }, { x0, 1.22f, 0.56f }, lona);

        m.TrianguloDobleCara({ x0, 1.22f, 0.57f }, { x1, 1.22f, 0.57f }, { x1, 1.04f, 0.57f }, faldon);
        m.TrianguloDobleCara({ x0, 1.22f, 0.57f }, { x1, 1.04f, 0.57f }, { x0, 1.04f, 0.57f }, faldon);
    }

    m.QuitarTransformacion();
}


static void ActualizarToldosMercado(const PartidaTablero& partida)
{
    const Tablero& t = partida.tablero;

    bool cambio =
        !mallaToldosMercado.cargada ||
        nivelToldosMercado != (int)CalidadDecoracion();

    for (int i = 0; i < t.cantidadCasillas; i++)
    {
        int firma = -1;

        if (t.casillas[i].tipo == CASILLA_NEUTRA && duenoPuesto[i] >= 0)
        {
            Color c = ColorJugadorMercado(partida, duenoPuesto[i]);
            firma = duenoPuesto[i] * 16777216 + c.r * 65536 + c.g * 256 + c.b;
        }

        if (firma != firmaToldosMercado[i])
        {
            firmaToldosMercado[i] = firma;
            cambio = true;
        }
    }

    if (!cambio)
    {
        return;
    }

    ConstructorMalla& m = constructorMercado;
    m.Limpiar();

    for (int i = 0; i < t.cantidadCasillas; i++)
    {
        if (t.casillas[i].tipo != CASILLA_NEUTRA)
        {
            continue;
        }

        AgregarToldoMercado(
            m,
            posicionPuestoMercado[i],
            giroPuestoMercado[i],
            duenoPuesto[i] < 0,
            ColorJugadorMercado(partida, duenoPuesto[i])
        );
    }

    IluminarMallaMercado(m, 0);

    // Misma geometria: si ya estaba cargada solo cambian los colores.
    if (mallaToldosMercado.cargada && mallaToldosMercado.malla.vertexCount == m.CantidadVertices())
    {
        ActualizarColoresMallaGpu(mallaToldosMercado, m);
    }
    else
    {
        ActualizarMallaGpu(mallaToldosMercado, m);
    }

    nivelToldosMercado = (int)CalidadDecoracion();
}


static void DibujarCampanaMercado(
    const PartidaTablero& partida
)
{
    float tiempo = partida.tiempoTablero;
    float fuerza = partida.gimmickAnim;
    float vaiven = std::sin(tiempo * 7.0f) * 28.0f * fuerza;

    const Color bronce = Color{ 214, 160, 64, 255 };
    const Color bronceOscuro = Color{ 150, 104, 40, 255 };

    rlPushMatrix();
    rlTranslatef(PLAZA_CX, ALTURA_CAMPANA_MERCADO, PLAZA_CZ);
    rlRotatef(vaiven, 1.0f, 0.0f, 0.0f);

    DrawCube(Vector3{ 0.0f, 0.04f, 0.0f }, 0.26f, 0.10f, 0.26f, MADERA_OSCURA_MERCADO);
    DrawCylinder(Vector3{ 0.0f, -0.70f, 0.0f }, 0.16f, 0.48f, 0.68f, 14, bronce);
    DrawCylinder(Vector3{ 0.0f, -0.75f, 0.0f }, 0.50f, 0.50f, 0.06f, 14, bronceOscuro);
    DrawSphere(Vector3{ 0.0f, -0.80f, 0.0f }, 0.10f, MADERA_OSCURA_MERCADO);

    rlPopMatrix();

    // Ondas de sonido mientras suena.
    if (fuerza > 0.05f && CalidadEfectos() != CALIDAD_BAJA)
    {
        for (int k = 0; k < 3; k++)
        {
            float fase = std::fmod(tiempo * 0.9f + k / 3.0f, 1.0f);

            DrawCircle3D(
                Vector3{ PLAZA_CX, ALTURA_CAMPANA_MERCADO - 0.4f, PLAZA_CZ },
                1.0f + fase * 4.0f,
                Vector3{ 1.0f, 0.0f, 0.0f },
                90.0f,
                Color{ 255, 226, 150, (unsigned char)(220.0f * (1.0f - fase) * fuerza) }
            );
        }
    }
}


static void DibujarGimmickMercado(
    const PartidaTablero& partida
)
{
    const Tablero& t = partida.tablero;

    ActualizarToldosMercado(partida);
    DibujarMallaGpu(mallaToldosMercado);

    // Banderin del dueno en cada puesto ocupado.
    for (int i = 0; i < t.cantidadCasillas; i++)
    {
        if (t.casillas[i].tipo != CASILLA_NEUTRA || duenoPuesto[i] < 0)
        {
            continue;
        }

        float flameo = std::sin(partida.tiempoTablero * 4.0f + i) * 0.05f;

        rlPushMatrix();
        rlTranslatef(posicionPuestoMercado[i].x, 0.0f, posicionPuestoMercado[i].z);
        rlRotatef(giroPuestoMercado[i], 0.0f, 1.0f, 0.0f);

        DrawCube(Vector3{ 0.56f, 1.72f, -0.42f }, 0.04f, 0.56f, 0.04f, MADERA_OSCURA_MERCADO);
        DrawCube(
            Vector3{ 0.72f, 1.88f, -0.42f + flameo }, 0.30f, 0.19f, 0.02f,
            ColorJugadorMercado(partida, duenoPuesto[i])
        );

        rlPopMatrix();
    }

    DibujarCampanaMercado(partida);
}


// Luz calida de atardecer arriba, un velo turquesa abajo y motas de luz que
// flotan en el aire.
static void DibujarCapaPantallaMercado(
    const PartidaTablero& partida
)
{
    int ancho = GetScreenWidth();
    int alto = GetScreenHeight();

    DrawRectangleGradientV(
        0, 0, ancho, (int)(alto * 0.45f),
        Color{ 255, 170, 110, 50 },
        Color{ 255, 170, 110, 0 }
    );

    DrawRectangleGradientV(
        0, (int)(alto * 0.7f), ancho, alto - (int)(alto * 0.7f),
        Color{ 30, 120, 140, 0 },
        Color{ 30, 120, 140, 34 }
    );

    float escala = alto / 720.0f;
    float t = partida.tiempoTablero;
    int motas = (int)std::ceil(26.0f * FactorCalidadGrafica(CalidadParticulas()));

    for (int i = 0; i < motas; i++)
    {
        float vx = 8.0f + 10.0f * Hash2Mercado(i, 81);
        float vy = -(6.0f + 10.0f * Hash2Mercado(i, 82));

        float x = std::fmod(Hash2Mercado(i, 83) * ancho + t * vx, (float)ancho + 40.0f) - 20.0f;
        float y = std::fmod(Hash2Mercado(i, 84) * alto + t * vy + alto * 4.0f, (float)alto + 40.0f) - 20.0f;

        x += 14.0f * std::sin(t * 0.8f + i);

        float brillo = 0.5f + 0.5f * std::sin(t * 1.6f + i * 2.1f);
        float radio = (1.6f + 2.2f * Hash2Mercado(i, 85)) * escala;

        DrawCircleV(
            Vector2{ x, y },
            radio,
            Color{ 255, 226, 170, (unsigned char)(40 + 90 * brillo) }
        );
    }
}


//==================================================
// RECURSOS
//==================================================

static void DescargarRecursosMercado()
{
    DescargarMalla(mallaMarMercado);
    DescargarMalla(mallaTierraMercado);
    DescargarMalla(mallaSenderosMercado);
    DescargarMalla(mallaEdificiosMercado);
    DescargarMalla(mallaPropsMercado);
    DescargarMalla(mallaToldosMercado);
    nivelToldosMercado = -1;
    nivelMallasMercado = -1;
}


//==================================================
// DEFINICION
//==================================================

const DefinicionTablero& ObtenerDefinicionTableroMercadoFlotante()
{
    static DefinicionTablero definicion;
    static bool preparada = false;

    if (!preparada)
    {
        definicion.id = TABLERO_MERCADO_FLOTANTE;
        definicion.nombre = "MERCADO FLOTANTE";

        definicion.historia =
            "Al atardecer, las barcas llenan los canales del mercado. "
            "Abrí puestos, cobrá alquileres y comprá los trofeos del puerto.";

        definicion.objetivo =
            "Reuní más trofeos (20 monedas cada uno) en 5 rondas.";

        definicion.gimmick =
            "Puestos: las casillas azules se abren por 3 monedas y los rivales te pagan 3 de alquiler.";

        definicion.dificultad = "MEDIA";

        definicion.colorTema = Color{ 38, 176, 178, 255 };
        definicion.colorFondo = Color{ 236, 150, 118, 255 };

        // Mapa grande (caja de casillas de 46 x 32.4, diagonal 56.3): la
        // camara sigue a la ficha, da una vista general al empezar la ronda
        // (zoom = diagonal / |desfase|) y apunta al centro de la caja.
        definicion.camaraObjetivo = { CENTRO_MAPA_X, 0.5f, CENTRO_MAPA_Z };
        definicion.camaraPosicion = { CENTRO_MAPA_X, 17.5f, CENTRO_MAPA_Z + 16.5f };
        definicion.camaraFovy = 48.0f;
        definicion.camaraSeguimiento = 1.0f;
        definicion.camaraZoomGeneral = 2.35f;
        definicion.camaraEscalaVistaPrevia = 2.35f;

        // Landmark: la torre de la campana en el centro de la plaza.
        definicion.camaraLandmark = { PLAZA_CX, 5.0f, PLAZA_CZ };
        definicion.tieneLandmark = true;

        // Losas de madera con algas.
        EstiloCasilla estilo = ObtenerEstiloCasillaPiedra();
        estilo.piedraLado = Color{ 128, 88, 58, 255 };
        estilo.piedraTapa = Color{ 196, 160, 112, 255 };
        estilo.borde = Color{ 82, 56, 40, 255 };
        estilo.detalle = Color{ 70, 150, 140, 255 };
        definicion.estiloCasilla = estilo;

        definicion.nombresZonas = NOMBRES_ZONAS_MERCADO;
        definicion.cantidadZonas = CANTIDAD_ZONAS_MERCADO;

        // Candidatas del trofeo (se calculan al construir la ruta).
        TableroReferenciaMercado();

        definicion.casillasTrofeo = casillasTrofeoMercado;
        definicion.cantidadCasillasTrofeo = cantidadTrofeosMercado;

        definicion.costoTrofeo = 20;
        definicion.cantidadRondas = 5;

        definicion.Construir = ConstruirMercado;
        definicion.DibujarDecoracion = DibujarDecoracionMercado;
        definicion.DescargarRecursos = DescargarRecursosMercado;
        definicion.DibujarCapaPantalla = DibujarCapaPantallaMercado;
        definicion.AlIniciarRonda = AlIniciarRondaMercado;
        definicion.ResolverCasilla = ResolverCasillaMercado;
        definicion.TextoLeyendaCasilla = TextoLeyendaCasillaMercado;
        definicion.ActualizarGimmick = ActualizarGimmickMercado;
        definicion.DibujarGimmick = DibujarGimmickMercado;
        definicion.DecidirRutaBot = DecidirRutaBotMercado;
        definicion.DescribirOpcionRuta = DescribirRutaMercado;

        preparada = true;
    }

    return definicion;
}
