#include "Board/CatalogoTableros.h"

#include "Gameplay/PartidaTablero.h"
#include "Systems/CalidadGrafica.h"

#include "rlgl.h"

#include <cmath>
#include <cstdio>


//==================================================
// FERIA NOCTURNA (tablero 5)
//==================================================
//
// Una feria de noche con forma de OCHO: dos lazos que se cruzan en el
// centro, junto a LA GRAN RUEDA de la fortuna y su frasco de monedas.
//
//   - Lazo oeste "CALLE DE LAS CARPAS" (sentido antihorario visto desde
//     arriba): entrada (casilla 0) abajo, tiro al blanco al oeste y un
//     CALLEJON entre las carpas que corta el lazo por el medio (atajo).
//   - Lazo este "LA NORIA" (sentido horario), con la noria en el centro.
//   - CRUCE: desde la esquina interior de abajo de cada lazo se puede
//     seguir en el lazo (opcion 0, sube por el lado interior) o ir al
//     cruce (opcion 1). En el cruce se elige lazo: los dos tramos forman
//     la X del ocho.
//
// Mecanica 1, EL POZO: lo que se pierde en las casillas rojas no
// desaparece, cae al frasco de la Gran Rueda.
// Mecanica 2, REMATE: desde la ronda 2 el precio del trofeo se sortea
// entre 15, 20 y 25 monedas (en la ultima ronda no baja de 20).
// Evento propio (casilla EVENTO): girar la Gran Rueda y llevarse todo el
// pozo, la mitad, o trabarla y pagar al pozo.

static const int COSTO_TROFEO_FERIA = 20;
static const int POZO_INICIAL = 5;
static const int PAGO_RUEDA_TRABADA = 3;

static const int PROBABILIDAD_PREMIO_MAYOR = 40;
static const int PROBABILIDAD_MEDIO_POZO = 40;

static const int PRECIOS_REMATE[] = { 15, 20, 25 };
static const int PRECIO_MINIMO_ULTIMA_RONDA = 20;

static const float ALTURA_CASILLA_FERIA = 0.25f;

// Lazos del ocho (elipses): centro en +-LAZO_CENTRO_X.
static const float LAZO_CENTRO_X = 9.0f;
static const float LAZO_RADIO_X = 6.0f;
static const float LAZO_RADIO_Z = 5.4f;
static const int CASILLAS_ARCO = 13;
static const float PASO_ARCO = 22.5f;

// Escenografia principal.
static const float RUEDA_X = 0.0f;
static const float RUEDA_Z = -4.45f;
static const float RUEDA_ALTO_EJE = 3.0f;
static const float RUEDA_RADIO = 2.05f;
static const int GAJOS_RUEDA = 12;

static const float FRASCO_X = 0.0f;
static const float FRASCO_Z = 3.4f;
static const float FRASCO_RADIO = 0.95f;
static const float FRASCO_BASE = 0.45f;
static const float FRASCO_ALTO = 2.15f;

static const float NORIA_X = LAZO_CENTRO_X;
static const float NORIA_Z = -0.4f;
static const float NORIA_ALTO_EJE = 2.9f;
static const float NORIA_RADIO = 2.25f;
static const int CABINAS_NORIA = 8;

static const float PORTAL_X = -LAZO_CENTRO_X;
static const float PORTAL_Z = 8.3f;

static const float RADIO_POSTES_X = LAZO_RADIO_X + 1.6f;
static const float RADIO_POSTES_Z = LAZO_RADIO_Z + 1.6f;
static const float ALTO_POSTE = 3.0f;

// Suelo.
static const float SUELO_ANCHO = 40.0f;
static const float SUELO_FONDO = 19.0f;
static const float SUELO_CENTRO_Z = 0.5f;


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
// CONSTRUCCION
//==================================================

enum ZonaFeria
{
    ZONA_ENTRADA = 0,
    ZONA_CARPAS,
    ZONA_NORIA,
    ZONA_GRAN_RUEDA,
    CANTIDAD_ZONAS_FERIA
};

static const char* const NOMBRES_ZONAS_FERIA[CANTIDAD_ZONAS_FERIA] =
{
    "ENTRADA",
    "CALLE DE LAS CARPAS",
    "LA NORIA",
    "LA GRAN RUEDA"
};

static const int MAX_TROFEOS_FERIA = 16;

static int casillasTrofeoFeria[MAX_TROFEOS_FERIA];
static int cantidadTrofeosFeria = 0;

static const char* descripcionRutaFeria[MAX_CASILLAS_TABLERO][2];


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
    char tipo,
    float x, float z
)
{
    int indice =
        t.AgregarCasilla(
            Vector3{ x, ALTURA_CASILLA_FERIA, z },
            TipoDesdeLetra(tipo)
        );

    if (indice >= 0)
    {
        t.casillas[indice].zona = (int)zona;
    }

    return indice;
}


// Punto de un lazo (elipse centrada en centroX) en el angulo dado.
static void PuntoLazoFeria(
    float centroX,
    float grados,
    float& x,
    float& z
)
{
    float a = grados * DEG2RAD;

    x = centroX + std::cos(a) * LAZO_RADIO_X;
    z = std::sin(a) * LAZO_RADIO_Z;
}


static void DefinirOpcionesFeria(
    int casilla,
    const char* opcion0,
    const char* opcion1
)
{
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

    // Letras: + verde, - roja, n neutra, R regalo, C caos, I intercambio,
    // D duelo, T tienda, V evento (la Gran Rueda).

    // --- Lazo oeste: 13 casillas de la esquina interior de arriba (k0)
    // a la de abajo (k12), por fuera. k2 = arriba (callejon), k10 = entrada.
    const char* tiposOeste = "+nn-+Dn-+In+n";
    const int ARRIBA_OESTE = 2;
    const int ENTRADA_OESTE = 10;

    int oeste[CASILLAS_ARCO];

    // La entrada se agrega primero: es la casilla 0.
    {
        float x = 0.0f;
        float z = 0.0f;
        PuntoLazoFeria(-LAZO_CENTRO_X, -45.0f - PASO_ARCO * ENTRADA_OESTE, x, z);
        oeste[ENTRADA_OESTE] = NodoFeria(t, ZONA_ENTRADA, tiposOeste[ENTRADA_OESTE], x, z);
    }

    for (int k = 0; k < CASILLAS_ARCO; k++)
    {
        if (k == ENTRADA_OESTE)
        {
            continue;
        }

        float x = 0.0f;
        float z = 0.0f;
        PuntoLazoFeria(-LAZO_CENTRO_X, -45.0f - PASO_ARCO * k, x, z);

        ZonaFeria zona =
            (k == ENTRADA_OESTE - 1 || k == ENTRADA_OESTE + 1)
            ? ZONA_ENTRADA
            : ZONA_CARPAS;

        oeste[k] = NodoFeria(t, zona, tiposOeste[k], x, z);
    }

    // Lado interior del lazo oeste (sube de k12 a k0).
    float xInteriorOeste = -LAZO_CENTRO_X + std::cos(45.0f * DEG2RAD) * LAZO_RADIO_X;
    float zEsquina = std::sin(45.0f * DEG2RAD) * LAZO_RADIO_Z;

    int interiorOeste1 = NodoFeria(t, ZONA_CARPAS, '-', xInteriorOeste, zEsquina / 3.0f);
    int interiorOeste2 = NodoFeria(t, ZONA_CARPAS, 'R', xInteriorOeste, -zEsquina / 3.0f);

    // Callejon de las carpas: de arriba (k2) a la entrada (k10), por el medio.
    int callejon1 = NodoFeria(t, ZONA_CARPAS, '-', -LAZO_CENTRO_X, -LAZO_RADIO_Z * 0.5f);
    int callejon2 = NodoFeria(t, ZONA_CARPAS, 'T', -LAZO_CENTRO_X, 0.0f);
    int callejon3 = NodoFeria(t, ZONA_CARPAS, 'n', -LAZO_CENTRO_X, LAZO_RADIO_Z * 0.5f);

    // --- La Gran Rueda: la X del ocho y el cruce en el centro ---
    float xMedio = xInteriorOeste * 0.5f;
    float zMedio = zEsquina * 0.5f;

    int cruceSuroeste = NodoFeria(t, ZONA_GRAN_RUEDA, '-', xMedio, zMedio);
    int cruceSureste = NodoFeria(t, ZONA_GRAN_RUEDA, '+', -xMedio, zMedio);
    int cruce = NodoFeria(t, ZONA_GRAN_RUEDA, 'n', 0.0f, 0.0f);
    int cruceNoreste = NodoFeria(t, ZONA_GRAN_RUEDA, 'V', -xMedio, -zMedio);
    int cruceNoroeste = NodoFeria(t, ZONA_GRAN_RUEDA, 'V', xMedio, -zMedio);

    // --- Lazo este: 13 casillas de la esquina interior de arriba (k0) a
    // la de abajo (k12), por fuera (sentido horario).
    const char* tiposEste = "n+Cn-+Rn-+n+n";

    int este[CASILLAS_ARCO];

    for (int k = 0; k < CASILLAS_ARCO; k++)
    {
        float x = 0.0f;
        float z = 0.0f;
        PuntoLazoFeria(LAZO_CENTRO_X, -135.0f + PASO_ARCO * k, x, z);

        este[k] = NodoFeria(t, ZONA_NORIA, tiposEste[k], x, z);
    }

    int interiorEste1 = NodoFeria(t, ZONA_NORIA, '-', -xInteriorOeste, zEsquina / 3.0f);
    int interiorEste2 = NodoFeria(t, ZONA_NORIA, 'n', -xInteriorOeste, -zEsquina / 3.0f);

    // --- Conexiones (opcion 0 primero) ---

    // Lazos por fuera.
    for (int k = 0; k + 1 < CASILLAS_ARCO; k++)
    {
        t.ConectarCasillas(oeste[k], oeste[k + 1]);
        t.ConectarCasillas(este[k], este[k + 1]);
    }

    // Arriba del lazo oeste: seguir la calle o meterse en el callejon.
    t.ConectarCasillas(oeste[ARRIBA_OESTE], callejon1);
    t.ConectarCasillas(callejon1, callejon2);
    t.ConectarCasillas(callejon2, callejon3);
    t.ConectarCasillas(callejon3, oeste[ENTRADA_OESTE]);

    // Esquina de abajo del lazo oeste: seguir en el lazo o ir al cruce.
    t.ConectarCasillas(oeste[CASILLAS_ARCO - 1], interiorOeste1);
    t.ConectarCasillas(oeste[CASILLAS_ARCO - 1], cruceSuroeste);
    t.ConectarCasillas(interiorOeste1, interiorOeste2);
    t.ConectarCasillas(interiorOeste2, oeste[0]);

    // Esquina de abajo del lazo este: igual.
    t.ConectarCasillas(este[CASILLAS_ARCO - 1], interiorEste1);
    t.ConectarCasillas(este[CASILLAS_ARCO - 1], cruceSureste);
    t.ConectarCasillas(interiorEste1, interiorEste2);
    t.ConectarCasillas(interiorEste2, este[0]);

    // Cruce: los dos tramos de la X llegan al centro y salen hacia
    // el lazo que se elija.
    t.ConectarCasillas(cruceSuroeste, cruce);
    t.ConectarCasillas(cruceSureste, cruce);
    t.ConectarCasillas(cruce, cruceNoreste);
    t.ConectarCasillas(cruce, cruceNoroeste);
    t.ConectarCasillas(cruceNoreste, este[0]);
    t.ConectarCasillas(cruceNoroeste, oeste[0]);

    // --- Textos de las bifurcaciones ---
    DefinirOpcionesFeria(oeste[ARRIBA_OESTE], "SEGUIR LA CALLE (LARGO)", "CALLEJÓN DE LAS CARPAS (ATAJO)");
    DefinirOpcionesFeria(oeste[CASILLAS_ARCO - 1], "SEGUIR EN LAS CARPAS", "AL CRUCE DE LA GRAN RUEDA");
    DefinirOpcionesFeria(este[CASILLAS_ARCO - 1], "SEGUIR EN LA NORIA", "AL CRUCE DE LA GRAN RUEDA");
    DefinirOpcionesFeria(cruce, "HACIA LA NORIA", "HACIA LAS CARPAS");

    // --- Candidatas del trofeo: repartidas por las zonas ---
    const int candidatas[] =
    {
        oeste[1], oeste[4], oeste[6], oeste[8],
        cruce,
        este[3], este[5], este[7], este[10], interiorEste2
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
// UTILIDADES
//==================================================

static float Parpadeo(
    float tiempo,
    float velocidad
)
{
    return 0.5f + 0.5f * std::sin(tiempo * velocidad);
}


static Color MezclaFeria(Color a, Color b, float t)
{
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;

    return Color
    {
        (unsigned char)(a.r + (b.r - a.r) * t),
        (unsigned char)(a.g + (b.g - a.g) * t),
        (unsigned char)(a.b + (b.b - a.b) * t),
        (unsigned char)(a.a + (b.a - a.a) * t)
    };
}


// Sombreado falso: las caras que miran a la luz (frente izquierdo) se
// aclaran y las de atras se oscurecen.
static Color SombrearFeria(Color c, float angulo)
{
    float f = 0.78f + 0.22f * std::cos(angulo - 2.2f);

    return Color
    {
        (unsigned char)(c.r * f),
        (unsigned char)(c.g * f),
        (unsigned char)(c.b * f),
        c.a
    };
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
// ESCENARIO HORNEADO (suelo, senderos, carpas, estructuras)
//==================================================
//
// Todo lo que no se mueve se emite una vez a una malla en GPU y se dibuja
// con una sola llamada. Se reconstruye si cambia la calidad de decoracion.

static ConstructorMalla constructorEscenarioFeria;
static MallaGpu mallaEscenarioFeria;
static int nivelEscenarioFeria = -1;

static const Color PASTO_NOCHE = Color{ 30, 50, 46, 255 };
static const Color PASTO_NOCHE_OSCURO = Color{ 22, 36, 36, 255 };
static const Color TIERRA_FERIA = Color{ 112, 86, 70, 255 };
static const Color TIERRA_FERIA_BORDE = Color{ 66, 50, 46, 255 };
static const Color MADERA_FERIA = Color{ 104, 70, 52, 255 };
static const Color MADERA_FERIA_OSCURA = Color{ 70, 46, 38, 255 };
static const Color DORADO_FERIA = Color{ 236, 184, 66, 255 };
static const Color MAGENTA_FERIA = Color{ 206, 62, 176, 255 };
static const Color VIOLETA_FERIA = Color{ 92, 54, 150, 255 };
static const Color CREMA_FERIA = Color{ 236, 222, 196, 255 };


static void DiscoSueloFeria(
    ConstructorMalla& m,
    float x, float z,
    float radio, float y,
    int lados,
    Color color
)
{
    for (int i = 0; i < lados; i++)
    {
        float a0 = 2.0f * PI * i / lados;
        float a1 = 2.0f * PI * (i + 1) / lados;

        m.TrianguloDobleCara(
            Vector3{ x, y, z },
            Vector3{ x + std::cos(a0) * radio, y, z + std::sin(a0) * radio },
            Vector3{ x + std::cos(a1) * radio, y, z + std::sin(a1) * radio },
            color
        );
    }
}


static void AgregarSueloFeria(
    ConstructorMalla& m,
    bool detalle
)
{
    // Pasto de noche con un zocalo mas oscuro.
    m.Caja(
        Vector3{ 0.0f, -0.30f, SUELO_CENTRO_Z },
        Vector3{ SUELO_ANCHO, 0.60f, SUELO_FONDO },
        PASTO_NOCHE
    );

    m.Caja(
        Vector3{ 0.0f, -0.62f, SUELO_CENTRO_Z },
        Vector3{ SUELO_ANCHO + 0.8f, 0.30f, SUELO_FONDO + 0.8f },
        PASTO_NOCHE_OSCURO
    );

    // Plazas de tierra: cruce, Gran Rueda, frasco y entrada.
    DiscoSueloFeria(m, 0.0f, 0.0f, 1.9f, 0.012f, 24, TIERRA_FERIA_BORDE);
    DiscoSueloFeria(m, RUEDA_X, RUEDA_Z - 0.2f, 2.6f, 0.010f, 24, TIERRA_FERIA_BORDE);
    DiscoSueloFeria(m, FRASCO_X, FRASCO_Z, 1.7f, 0.010f, 20, TIERRA_FERIA_BORDE);
    DiscoSueloFeria(m, PORTAL_X, LAZO_RADIO_Z + 1.2f, 2.2f, 0.010f, 20, TIERRA_FERIA_BORDE);

    if (!detalle)
    {
        return;
    }

    // Manchas de pasto y confeti pisado.
    for (int i = 0; i < 14; i++)
    {
        float x = -18.0f + 36.0f * HashFeria(i, 1);
        float z = -7.5f + 16.0f * HashFeria(i, 2);

        DiscoSueloFeria(
            m, x, z,
            0.8f + 1.4f * HashFeria(i, 3),
            0.004f,
            10,
            HashFeria(i, 4) < 0.5f ? PASTO_NOCHE_OSCURO : Color{ 36, 58, 50, 255 }
        );
    }

    for (int i = 0; i < 90; i++)
    {
        float x = -18.5f + 37.0f * HashFeria(i, 11);
        float z = -7.8f + 17.0f * HashFeria(i, 12);

        m.Caja(
            Vector3{ x, 0.008f, z },
            Vector3{ 0.13f, 0.012f, 0.13f },
            MezclaFeria(
                COLORES_BOMBILLA[i % CANTIDAD_COLORES_BOMBILLA],
                PASTO_NOCHE,
                0.45f
            )
        );
    }
}


// Senderos de tierra entre las casillas conectadas (borde y centro) con un
// disco en cada casilla para redondear las uniones.
static void AgregarSenderosFeria(
    ConstructorMalla& m,
    const Tablero& t
)
{
    for (int i = 0; i < t.cantidadCasillas; i++)
    {
        const Casilla& origen = t.casillas[i];

        for (int c = 0; c < origen.cantidadConexiones; c++)
        {
            const Casilla* destino =
                t.ObtenerCasilla(origen.conexiones[c].destino);

            if (destino == nullptr)
            {
                continue;
            }

            AgregarCintaSueloMalla(m, origen.posicion, destino->posicion, 1.35f, 0.022f, TIERRA_FERIA_BORDE);
            AgregarCintaSueloMalla(m, origen.posicion, destino->posicion, 0.98f, 0.036f, TIERRA_FERIA);
        }
    }

    for (int i = 0; i < t.cantidadCasillas; i++)
    {
        Vector3 p = t.casillas[i].posicion;

        DiscoSueloFeria(m, p.x, p.z, 0.68f, 0.023f, 16, TIERRA_FERIA_BORDE);
        DiscoSueloFeria(m, p.x, p.z, 0.50f, 0.037f, 16, TIERRA_FERIA);
    }
}


// Carpa de circo: pared y techo conico a rayas, entrada oscura hacia la
// camara y banderin en la punta.
static void AgregarCarpaFeria(
    ConstructorMalla& m,
    float x, float z,
    float radio,
    float alto,
    Color rayaA,
    Color rayaB
)
{
    const int GAJOS = 14;
    float altoPared = alto * 0.42f;
    float radioTecho = radio * 1.14f;

    Vector3 punta = { x, alto, z };

    for (int i = 0; i < GAJOS; i++)
    {
        float a0 = 2.0f * PI * i / GAJOS;
        float a1 = 2.0f * PI * (i + 1) / GAJOS;
        float medio = (a0 + a1) * 0.5f;

        Color raya = SombrearFeria(i % 2 == 0 ? rayaA : rayaB, medio);

        Vector3 p00 = { x + std::cos(a0) * radio, 0.0f, z + std::sin(a0) * radio };
        Vector3 p10 = { x + std::cos(a1) * radio, 0.0f, z + std::sin(a1) * radio };
        Vector3 p01 = { p00.x, altoPared, p00.z };
        Vector3 p11 = { p10.x, altoPared, p10.z };

        m.TrianguloDobleCara(p00, p10, p11, raya);
        m.TrianguloDobleCara(p00, p11, p01, raya);

        Vector3 t0 = { x + std::cos(a0) * radioTecho, altoPared - 0.08f, z + std::sin(a0) * radioTecho };
        Vector3 t1 = { x + std::cos(a1) * radioTecho, altoPared - 0.08f, z + std::sin(a1) * radioTecho };

        m.TrianguloDobleCara(punta, t0, t1, raya);

        // Festones del alero.
        Vector3 colgante = { x + std::cos(medio) * radioTecho, altoPared - 0.30f, z + std::sin(medio) * radioTecho };

        m.TrianguloDobleCara(t0, t1, colgante, SombrearFeria(i % 2 == 0 ? rayaB : rayaA, medio));
    }

    // Entrada (mira a la camara, +z).
    m.Caja(
        Vector3{ x, altoPared * 0.42f, z + radio * 0.97f },
        Vector3{ radio * 0.62f, altoPared * 0.84f, 0.05f },
        Color{ 34, 18, 40, 255 }
    );

    // Mastil y banderin.
    m.Cilindro(
        Vector3{ x, alto - 0.05f, z },
        Vector3{ x, alto + 0.65f, z },
        0.035f, 0.035f, 5,
        MADERA_FERIA_OSCURA
    );

    m.Caja(
        Vector3{ x + 0.2f, alto + 0.52f, z },
        Vector3{ 0.36f, 0.20f, 0.02f },
        rayaA
    );
}


static void AgregarCarpasFeria(ConstructorMalla& m)
{
    const float cx = -LAZO_CENTRO_X;

    // Carpa grande del comerciante, junto a la TIENDA del callejon.
    AgregarCarpaFeria(m, cx - 3.0f, 0.0f, 1.45f, 3.1f, MAGENTA_FERIA, CREMA_FERIA);

    // Carpas chicas a los dos lados del callejon.
    AgregarCarpaFeria(m, cx + 2.1f, 0.0f, 1.0f, 2.3f, VIOLETA_FERIA, DORADO_FERIA);
    AgregarCarpaFeria(m, cx - 2.9f, -2.95f, 0.78f, 1.9f, Color{ 60, 150, 190, 255 }, CREMA_FERIA);
    AgregarCarpaFeria(m, cx - 2.9f, 2.95f, 0.78f, 1.9f, Color{ 212, 70, 64, 255 }, CREMA_FERIA);
    AgregarCarpaFeria(m, cx + 2.1f, -2.95f, 0.68f, 1.7f, DORADO_FERIA, MAGENTA_FERIA);
    AgregarCarpaFeria(m, cx + 2.1f, 2.95f, 0.68f, 1.7f, Color{ 70, 170, 110, 255 }, CREMA_FERIA);
}


// Soporte de la Gran Rueda: tarima, patas en A y mastil del puntero.
static void AgregarBaseGranRuedaFeria(ConstructorMalla& m)
{
    m.Caja(
        Vector3{ RUEDA_X, 0.16f, RUEDA_Z - 0.35f },
        Vector3{ 4.6f, 0.32f, 1.7f },
        MADERA_FERIA
    );

    m.Caja(
        Vector3{ RUEDA_X, 0.34f, RUEDA_Z + 0.48f },
        Vector3{ 4.6f, 0.06f, 0.10f },
        DORADO_FERIA
    );

    Vector3 eje = { RUEDA_X, RUEDA_ALTO_EJE, RUEDA_Z - 0.32f };

    m.Cilindro(Vector3{ RUEDA_X - 1.6f, 0.30f, RUEDA_Z - 0.32f }, eje, 0.13f, 0.10f, 8, VIOLETA_FERIA);
    m.Cilindro(Vector3{ RUEDA_X + 1.6f, 0.30f, RUEDA_Z - 0.32f }, eje, 0.13f, 0.10f, 8, VIOLETA_FERIA);
    m.Cilindro(Vector3{ RUEDA_X, 0.30f, RUEDA_Z - 1.0f }, eje, 0.10f, 0.08f, 8, VIOLETA_FERIA);

    // Mastil del puntero (las guirnaldas del norte cuelgan de su punta).
    m.Cilindro(
        Vector3{ RUEDA_X, 0.30f, RUEDA_Z - 0.6f },
        Vector3{ RUEDA_X, 5.75f, RUEDA_Z - 0.6f },
        0.08f, 0.07f, 8,
        MADERA_FERIA_OSCURA
    );

    m.Caja(
        Vector3{ RUEDA_X, 5.55f, RUEDA_Z - 0.2f },
        Vector3{ 0.16f, 0.16f, 0.8f },
        MADERA_FERIA_OSCURA
    );

    // Puntero rojo que apunta al gajo ganador.
    m.Cilindro(
        Vector3{ RUEDA_X, 5.62f, RUEDA_Z + 0.22f },
        Vector3{ RUEDA_X, RUEDA_ALTO_EJE + RUEDA_RADIO - 0.12f, RUEDA_Z + 0.22f },
        0.24f, 0.02f, 8,
        Color{ 226, 48, 58, 255 }
    );

    m.Esfera(Vector3{ RUEDA_X, 5.82f, RUEDA_Z - 0.6f }, 0.14f, 6, 8, DORADO_FERIA);
}


// Pedestal del frasco y aro de la boca (el vidrio y las monedas son del gimmick).
static void AgregarBaseFrascoFeria(ConstructorMalla& m)
{
    m.Cilindro(
        Vector3{ FRASCO_X, 0.0f, FRASCO_Z },
        Vector3{ FRASCO_X, 0.34f, FRASCO_Z },
        1.28f, 1.16f, 20,
        MADERA_FERIA
    );

    m.Cilindro(
        Vector3{ FRASCO_X, 0.34f, FRASCO_Z },
        Vector3{ FRASCO_X, FRASCO_BASE, FRASCO_Z },
        FRASCO_RADIO + 0.07f, FRASCO_RADIO + 0.05f, 20,
        DORADO_FERIA
    );

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
    }
}


// Patas y eje de la noria (la rueda gira en modo inmediato).
static void AgregarBaseNoriaFeria(ConstructorMalla& m)
{
    m.Caja(
        Vector3{ NORIA_X, 0.10f, NORIA_Z },
        Vector3{ 3.8f, 0.20f, 2.0f },
        MADERA_FERIA
    );

    const float lados[] = { -0.55f, 0.55f };

    for (float dz : lados)
    {
        Vector3 eje = { NORIA_X, NORIA_ALTO_EJE, NORIA_Z + dz };

        m.Cilindro(Vector3{ NORIA_X - 1.7f, 0.2f, NORIA_Z + dz * 1.4f }, eje, 0.10f, 0.08f, 6, VIOLETA_FERIA);
        m.Cilindro(Vector3{ NORIA_X + 1.7f, 0.2f, NORIA_Z + dz * 1.4f }, eje, 0.10f, 0.08f, 6, VIOLETA_FERIA);
    }

    m.Cilindro(
        Vector3{ NORIA_X, NORIA_ALTO_EJE, NORIA_Z - 0.62f },
        Vector3{ NORIA_X, NORIA_ALTO_EJE, NORIA_Z + 0.62f },
        0.13f, 0.13f, 8,
        DORADO_FERIA
    );

    // Boleteria al pie.
    m.Caja(Vector3{ NORIA_X + 1.2f, 0.55f, NORIA_Z + 1.05f }, Vector3{ 0.8f, 0.7f, 0.5f }, MAGENTA_FERIA);
    m.Caja(Vector3{ NORIA_X + 1.2f, 0.95f, NORIA_Z + 1.05f }, Vector3{ 0.95f, 0.10f, 0.62f }, CREMA_FERIA);
}


// Portal de la entrada: postes a rayas (baston de caramelo) y cartel.
static void AgregarPortalFeria(ConstructorMalla& m)
{
    const float postes[] = { PORTAL_X - 1.75f, PORTAL_X + 1.75f };
    const int TRAMOS = 6;
    const float ALTO = 2.4f;

    for (float x : postes)
    {
        for (int i = 0; i < TRAMOS; i++)
        {
            m.Cilindro(
                Vector3{ x, ALTO * i / TRAMOS, PORTAL_Z },
                Vector3{ x, ALTO * (i + 1) / TRAMOS, PORTAL_Z },
                0.17f, 0.17f, 10,
                i % 2 == 0 ? Color{ 224, 52, 70, 255 } : CREMA_FERIA
            );
        }

        m.Esfera(Vector3{ x, ALTO + 0.12f, PORTAL_Z }, 0.2f, 6, 8, DORADO_FERIA);
    }

    m.Caja(Vector3{ PORTAL_X, 2.5f, PORTAL_Z }, Vector3{ 3.9f, 0.55f, 0.22f }, MAGENTA_FERIA);
    m.Caja(Vector3{ PORTAL_X, 2.5f, PORTAL_Z + 0.02f }, Vector3{ 3.4f, 0.32f, 0.24f }, VIOLETA_FERIA);
    m.Caja(Vector3{ PORTAL_X, 2.5f, PORTAL_Z + 0.04f }, Vector3{ 2.2f, 0.10f, 0.24f }, DORADO_FERIA);
}


//--------------------------------------------------
// Guirnaldas (postes horneados, bombillas animadas)
//--------------------------------------------------

static const int MAX_GUIRNALDAS_FERIA = 16;
static const int ANGULOS_POSTES = 7;
static const float ANGULOS_POSTE_FERIA[ANGULOS_POSTES] = { 45.0f, 90.0f, 135.0f, 180.0f, 225.0f, 270.0f, 315.0f };

static Vector3 guirnaldasFeria[MAX_GUIRNALDAS_FERIA][2];
static int cantidadGuirnaldasFeria = 0;


// Punta del poste de un lazo (lado 0 = oeste, 1 = este espejado). En el
// oeste, el poste de abajo es el portal de la entrada.
static Vector3 PuntaPosteFeria(int lado, float grados)
{
    if (lado == 0 && grados == 90.0f)
    {
        return Vector3{ PORTAL_X, 2.75f, PORTAL_Z };
    }

    float a = grados * DEG2RAD;
    float x = -LAZO_CENTRO_X + std::cos(a) * RADIO_POSTES_X;

    return Vector3
    {
        lado == 0 ? x : -x,
        ALTO_POSTE,
        std::sin(a) * RADIO_POSTES_Z
    };
}


static void PrepararGuirnaldasFeria()
{
    if (cantidadGuirnaldasFeria > 0)
    {
        return;
    }

    for (int lado = 0; lado < 2; lado++)
    {
        for (int i = 0; i + 1 < ANGULOS_POSTES; i++)
        {
            guirnaldasFeria[cantidadGuirnaldasFeria][0] = PuntaPosteFeria(lado, ANGULOS_POSTE_FERIA[i]);
            guirnaldasFeria[cantidadGuirnaldasFeria][1] = PuntaPosteFeria(lado, ANGULOS_POSTE_FERIA[i + 1]);
            cantidadGuirnaldasFeria++;
        }
    }

    // Sur: de lazo a lazo por delante del frasco.
    guirnaldasFeria[cantidadGuirnaldasFeria][0] = PuntaPosteFeria(0, 45.0f);
    guirnaldasFeria[cantidadGuirnaldasFeria][1] = PuntaPosteFeria(1, 45.0f);
    cantidadGuirnaldasFeria++;

    // Norte: de cada lazo a la punta del mastil de la Gran Rueda.
    Vector3 mastil = { RUEDA_X, 5.75f, RUEDA_Z - 0.6f };

    guirnaldasFeria[cantidadGuirnaldasFeria][0] = PuntaPosteFeria(0, 315.0f);
    guirnaldasFeria[cantidadGuirnaldasFeria][1] = mastil;
    cantidadGuirnaldasFeria++;

    guirnaldasFeria[cantidadGuirnaldasFeria][0] = mastil;
    guirnaldasFeria[cantidadGuirnaldasFeria][1] = PuntaPosteFeria(1, 315.0f);
    cantidadGuirnaldasFeria++;
}


static void AgregarPostesFeria(ConstructorMalla& m)
{
    for (int lado = 0; lado < 2; lado++)
    {
        for (int i = 0; i < ANGULOS_POSTES; i++)
        {
            if (lado == 0 && ANGULOS_POSTE_FERIA[i] == 90.0f)
            {
                continue;
            }

            Vector3 punta = PuntaPosteFeria(lado, ANGULOS_POSTE_FERIA[i]);

            m.Cilindro(
                Vector3{ punta.x, 0.0f, punta.z },
                Vector3{ punta.x, punta.y + 0.1f, punta.z },
                0.08f, 0.06f, 6,
                MADERA_FERIA_OSCURA
            );

            m.Esfera(Vector3{ punta.x, punta.y + 0.15f, punta.z }, 0.09f, 4, 6, DORADO_FERIA);
        }
    }
}


// Puestos del borde: tiro al blanco (junto al DUELO) y algodon de azucar.
static void AgregarPuestosFeria(ConstructorMalla& m)
{
    // Tiro al blanco, mirando al tablero (+x).
    const float tx = -17.6f;
    const float tz = -2.9f;

    m.Caja(Vector3{ tx - 0.55f, 1.1f, tz }, Vector3{ 0.15f, 2.2f, 2.4f }, MADERA_FERIA_OSCURA);
    m.Caja(Vector3{ tx + 0.25f, 0.45f, tz }, Vector3{ 0.5f, 0.9f, 2.4f }, Color{ 212, 70, 64, 255 });
    m.Caja(Vector3{ tx + 0.25f, 0.92f, tz }, Vector3{ 0.62f, 0.08f, 2.5f }, CREMA_FERIA);
    m.Caja(Vector3{ tx, 2.3f, tz }, Vector3{ 1.4f, 0.12f, 2.7f }, Color{ 212, 70, 64, 255 });
    m.Caja(Vector3{ tx, 2.36f, tz }, Vector3{ 1.2f, 0.06f, 2.5f }, CREMA_FERIA);

    for (int i = 0; i < 3; i++)
    {
        Vector3 centro = { tx - 0.46f, 1.45f, tz - 0.75f + 0.75f * i };

        m.Cilindro(centro, Vector3{ centro.x + 0.04f, centro.y, centro.z }, 0.34f, 0.34f, 14, Color{ 220, 50, 60, 255 });
        m.Cilindro(Vector3{ centro.x + 0.04f, centro.y, centro.z }, Vector3{ centro.x + 0.07f, centro.y, centro.z }, 0.23f, 0.23f, 14, CREMA_FERIA);
        m.Cilindro(Vector3{ centro.x + 0.07f, centro.y, centro.z }, Vector3{ centro.x + 0.10f, centro.y, centro.z }, 0.11f, 0.11f, 10, Color{ 220, 50, 60, 255 });
    }

    // Algodon de azucar (junto al REGALO del este).
    const float ax = 17.5f;
    const float az = 2.7f;

    m.Caja(Vector3{ ax, 0.5f, az }, Vector3{ 1.2f, 1.0f, 0.9f }, Color{ 96, 196, 220, 255 });
    m.Caja(Vector3{ ax, 1.04f, az }, Vector3{ 1.3f, 0.08f, 1.0f }, CREMA_FERIA);
    m.Cilindro(Vector3{ ax - 0.45f, 1.0f, az - 0.3f }, Vector3{ ax - 0.45f, 2.2f, az - 0.3f }, 0.04f, 0.04f, 5, MADERA_FERIA_OSCURA);
    m.Cilindro(Vector3{ ax + 0.45f, 1.0f, az - 0.3f }, Vector3{ ax + 0.45f, 2.2f, az - 0.3f }, 0.04f, 0.04f, 5, MADERA_FERIA_OSCURA);
    m.Cilindro(Vector3{ ax, 2.15f, az - 0.3f }, Vector3{ ax, 2.6f, az - 0.3f }, 0.9f, 0.05f, 10, Color{ 255, 150, 200, 255 });
    m.Esfera(Vector3{ ax - 0.2f, 1.35f, az }, 0.24f, 6, 8, Color{ 255, 170, 214, 255 });
    m.Esfera(Vector3{ ax + 0.25f, 1.3f, az + 0.1f }, 0.2f, 6, 8, Color{ 190, 220, 255, 255 });
}


// Arboleda oscura al fondo y a los costados.
static void AgregarFondoFeria(
    ConstructorMalla& m,
    NivelCalidadGrafica nivel
)
{
    float paso = nivel == CALIDAD_ALTA ? 2.3f : 3.4f;
    float fondo = SUELO_CENTRO_Z - SUELO_FONDO * 0.5f;

    int i = 0;

    for (float x = -SUELO_ANCHO * 0.5f + 0.8f; x < SUELO_ANCHO * 0.5f; x += paso)
    {
        float alto = 2.6f + 1.6f * HashFeria(i, 21);
        float z = fondo + 0.7f + 0.5f * HashFeria(i, 22);

        m.Cilindro(Vector3{ x, 0.0f, z }, Vector3{ x, 0.6f, z }, 0.14f, 0.12f, 6, Color{ 40, 30, 30, 255 });
        m.Cilindro(Vector3{ x, 0.5f, z }, Vector3{ x, 0.5f + alto, z }, 0.95f, 0.0f, 8, Color{ 18, 34, 34, 255 });

        // Arbustos entre los arboles.
        m.Esfera(
            Vector3{ x + paso * 0.5f, 0.3f, fondo + 1.5f + 0.3f * HashFeria(i, 23) },
            0.55f + 0.2f * HashFeria(i, 24),
            5, 8,
            Color{ 24, 44, 40, 255 }
        );

        i++;
    }

    const float costados[] = { -SUELO_ANCHO * 0.5f + 0.7f, SUELO_ANCHO * 0.5f - 0.7f };

    for (float x : costados)
    {
        for (float z = -5.5f; z < 8.0f; z += paso * 1.5f)
        {
            float alto = 2.2f + 1.2f * HashFeria(i, 25);

            m.Cilindro(Vector3{ x, 0.4f, z }, Vector3{ x, 0.4f + alto, z }, 0.8f, 0.0f, 8, Color{ 18, 34, 34, 255 });
            i++;
        }
    }
}


static void ConstruirMallaEscenarioFeria(NivelCalidadGrafica nivel)
{
    ConstructorMalla& m = constructorEscenarioFeria;

    m.Limpiar();
    m.reduccionDetalle =
        nivel == CALIDAD_ALTA
        ? 0
        : (nivel == CALIDAD_MEDIA ? 2 : 4);

    const bool detalle = nivel != CALIDAD_BAJA;

    AgregarSueloFeria(m, detalle);
    AgregarSenderosFeria(m, TableroReferenciaFeria());
    AgregarCarpasFeria(m);
    AgregarBaseGranRuedaFeria(m);
    AgregarBaseFrascoFeria(m);
    AgregarBaseNoriaFeria(m);
    AgregarPortalFeria(m);
    AgregarPostesFeria(m);

    if (detalle)
    {
        AgregarPuestosFeria(m);
        AgregarFondoFeria(m, nivel);
    }

    ActualizarMallaGpu(mallaEscenarioFeria, m);
    nivelEscenarioFeria = (int)nivel;
}


//==================================================
// DECORACION
//==================================================

static void DibujarNoriaFeria(float tiempo)
{
    // MODELO FUTURO: noria completa como modelo 3D animado.
    const int SEGMENTOS = 16;
    float giro = tiempo * 0.22f;

    Vector3 eje = { NORIA_X, NORIA_ALTO_EJE, NORIA_Z };

    const Color aro = Color{ 214, 86, 196, 255 };
    const Color rayo = Color{ 150, 120, 200, 255 };

    const float caras[] = { -0.30f, 0.30f };

    for (float dz : caras)
    {
        for (int i = 0; i < SEGMENTOS; i++)
        {
            float a0 = giro + 2.0f * PI * i / SEGMENTOS;
            float a1 = giro + 2.0f * PI * (i + 1) / SEGMENTOS;

            Vector3 p0 = { eje.x + std::cos(a0) * NORIA_RADIO, eje.y + std::sin(a0) * NORIA_RADIO, eje.z + dz };
            Vector3 p1 = { eje.x + std::cos(a1) * NORIA_RADIO, eje.y + std::sin(a1) * NORIA_RADIO, eje.z + dz };

            DrawCylinderEx(p0, p1, 0.055f, 0.055f, 4, aro);

            if (i % 2 == 0)
            {
                DrawCylinderEx(Vector3{ eje.x, eje.y, eje.z + dz }, p0, 0.035f, 0.035f, 4, rayo);
            }
        }
    }

    // Cabinas: cuelgan siempre derechas.
    for (int i = 0; i < CABINAS_NORIA; i++)
    {
        float a = giro + 2.0f * PI * i / CABINAS_NORIA;

        Vector3 enganche = { eje.x + std::cos(a) * NORIA_RADIO, eje.y + std::sin(a) * NORIA_RADIO, eje.z };

        DrawCylinderEx(
            Vector3{ enganche.x, enganche.y, enganche.z - 0.32f },
            Vector3{ enganche.x, enganche.y, enganche.z + 0.32f },
            0.04f, 0.04f, 4,
            rayo
        );

        Vector3 cabina = { enganche.x, enganche.y - 0.40f, enganche.z };
        Color color = COLORES_BOMBILLA[i % CANTIDAD_COLORES_BOMBILLA];

        DrawLine3D(enganche, Vector3{ cabina.x, cabina.y + 0.2f, cabina.z }, Color{ 40, 30, 50, 255 });
        DrawCubeV(cabina, Vector3{ 0.46f, 0.36f, 0.42f }, MezclaFeria(color, Color{ 40, 20, 50, 255 }, 0.25f));
        DrawCubeV(Vector3{ cabina.x, cabina.y + 0.24f, cabina.z }, Vector3{ 0.56f, 0.08f, 0.50f }, MezclaFeria(color, WHITE, 0.25f));
    }

    // Bombillas del aro delantero.
    int bombillas =
        (int)std::ceil(16.0f * FactorCalidadGrafica(CalidadEfectos()));

    if (CalidadEfectos() == CALIDAD_BAJA)
    {
        bombillas = 0;
    }

    for (int i = 0; i < bombillas; i++)
    {
        float a = giro + 2.0f * PI * (i + 0.5f) / bombillas;

        DibujarBombillaFeria(
            Vector3{ eje.x + std::cos(a) * NORIA_RADIO, eje.y + std::sin(a) * NORIA_RADIO, eje.z + 0.36f },
            i,
            tiempo,
            0.075f
        );
    }
}


static void DibujarGuirnaldasFeria(float tiempo)
{
    PrepararGuirnaldasFeria();

    // Calidad: cantidad de bombillas por tira.
    int porTira =
        (int)std::ceil(10.0f * FactorCalidadGrafica(CalidadEfectos()));

    if (porTira < 3)
    {
        porTira = 3;
    }

    const int TRAMOS_CABLE = 8;
    const Color cable = Color{ 26, 20, 30, 255 };

    int indiceBombilla = 0;

    for (int g = 0; g < cantidadGuirnaldasFeria; g++)
    {
        Vector3 a = guirnaldasFeria[g][0];
        Vector3 b = guirnaldasFeria[g][1];

        float dx = b.x - a.x;
        float dz = b.z - a.z;
        float largo = std::sqrt(dx * dx + dz * dz);
        float comba = largo * 0.10f;

        if (comba < 0.25f) comba = 0.25f;
        if (comba > 0.75f) comba = 0.75f;

        // Punto de la tira con la comba de una catenaria simple.
        auto punto = [&](float t)
        {
            return Vector3
            {
                a.x + (b.x - a.x) * t,
                a.y + (b.y - a.y) * t - comba * 4.0f * t * (1.0f - t),
                a.z + (b.z - a.z) * t
            };
        };

        Vector3 anterior = a;

        for (int k = 1; k <= TRAMOS_CABLE; k++)
        {
            Vector3 actual = punto((float)k / TRAMOS_CABLE);
            DrawLine3D(anterior, actual, cable);
            anterior = actual;
        }

        for (int k = 0; k < porTira; k++)
        {
            Vector3 p = punto((k + 0.5f) / porTira);
            p.y -= 0.08f;

            DibujarBombillaFeria(p, indiceBombilla, tiempo, 0.085f);
            indiceBombilla++;
        }
    }
}


// Luciernagas y chispas flotando sobre el pasto.
static void DibujarLuciernagasFeria(float tiempo)
{
    int cantidad =
        (int)std::ceil(14.0f * FactorCalidadGrafica(CalidadParticulas()));

    for (int i = 0; i < cantidad; i++)
    {
        float x = -17.0f + 34.0f * HashFeria(i, 31) + std::sin(tiempo * 0.4f + i) * 0.8f;
        float z = -7.0f + 14.0f * HashFeria(i, 32) + std::cos(tiempo * 0.3f + i * 1.7f) * 0.8f;
        float y = 0.6f + 1.2f * HashFeria(i, 33) + std::sin(tiempo * 1.1f + i * 2.3f) * 0.25f;

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


static void DibujarDecoracionFeria(
    const PartidaTablero& partida
)
{
    // MODELO FUTURO: la feria completa (suelo, carpas, puestos) como GLB.
    NivelCalidadGrafica nivel = CalidadDecoracion();

    if (!mallaEscenarioFeria.cargada || nivelEscenarioFeria != (int)nivel)
    {
        ConstruirMallaEscenarioFeria(nivel);
    }

    DibujarMallaGpu(mallaEscenarioFeria);

    float t = partida.tiempoTablero;

    DibujarNoriaFeria(t);
    DibujarGuirnaldasFeria(t);
    DibujarLuciernagasFeria(t);
}


// Velo nocturno: cielo oscuro arriba y bordes en penumbra.
static void DibujarCapaPantallaFeria(
    const PartidaTablero& partida
)
{
    (void)partida;

    int ancho = GetScreenWidth();
    int alto = GetScreenHeight();
    int borde = ancho / 8;

    DrawRectangleGradientV(
        0, 0, ancho, alto / 2,
        Color{ 10, 6, 34, 90 },
        Color{ 10, 6, 34, 0 }
    );

    DrawRectangleGradientH(0, 0, borde, alto, Color{ 8, 4, 22, 80 }, Color{ 8, 4, 22, 0 });
    DrawRectangleGradientH(ancho - borde, 0, borde, alto, Color{ 8, 4, 22, 0 }, Color{ 8, 4, 22, 80 });
}


//==================================================
// GIMMICK: LA GRAN RUEDA Y EL POZO
//==================================================

// Patron de gajos: G = premio mayor (40%), M = medio pozo (40%),
// T = rueda trabada (20%).
static const char PATRON_GAJOS_RUEDA[GAJOS_RUEDA + 1] = "GMGMTGMGMTGM";


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
            Vector3{ RUEDA_X - 2.1f + 4.2f * i / (BOMBILLAS - 1), 0.42f, RUEDA_Z + 0.52f },
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

    // 20%: la rueda se traba y el jugador paga al pozo.
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

        // Gran Rueda: esperanza 0.4 * pozo + 0.4 * pozo / 2 - 0.2 * 3,
        // vale mas cuanto mas lleno esta el frasco.
        case CASILLA_EVENTO: return (monedasPozo * 3 - PAGO_RUEDA_TRABADA) / 5;

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
// sin ellas busca la ruta de casillas mas ricas. Siempre deja algo de azar.
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
    DescargarMalla(mallaEscenarioFeria);
    nivelEscenarioFeria = -1;
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

        // Camara diorama: el ocho entra casi completo; sigue al jugador
        // a medias y abre una vista general al empezar la ronda.
        definicion.camaraObjetivo = { 0.0f, 0.5f, 0.8f };
        definicion.camaraPosicion = { 0.0f, 15.5f, 15.0f };
        definicion.camaraFovy = 48.0f;
        definicion.camaraSeguimiento = 0.7f;
        definicion.camaraZoomGeneral = 1.3f;
        definicion.camaraEscalaVistaPrevia = 1.35f;

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
