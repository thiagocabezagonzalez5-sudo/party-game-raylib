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
// El piso de una torre de relojeria. El tablero es la ESFERA de un reloj
// gigante: un anillo de 24 casillas (2 por hora) que se recorre en sentido
// horario. La casilla 0 (inicio) esta en "las 6", hacia la camara.
//
// Dos AGUJAS cruzan el centro como atajos y comparten la casilla central
// (EL EJE), que es otra bifurcacion:
//
//   - AGUJA DE LAS HORAS:   las 12 (casilla 12) -> 24, 25 -> EJE (26)
//                           -> 27, 28 -> las 6 (casilla 0).
//   - AGUJA DE LOS MINUTOS: las 3 (casilla 18) -> 29, 30 -> EJE (26)
//                           -> 31, 32 -> las 9 (casilla 6).
//
// En las 12 y en las 3 la opcion 0 sigue la esfera (larga) y la 1 toma la
// aguja (atajo). En EL EJE la opcion 0 baja hacia las 6 y la 1 dobla hacia
// las 9.
//
// GIMMICK: LA ESFERA GIRA. Las casillas de color del anillo (azules, verdes
// y rojas) forman un patron que, al empezar cada ronda desde la 2, avanza
// N posiciones (1..3) en sentido horario. Se giran los TIPOS en el mismo
// partida.tablero. Las casillas especiales, la casilla 0 y las agujas
// estan "atornilladas" y no se mueven, asi que los conteos por tipo nunca
// cambian (la regla obligatoria se cumple siempre).
//
// EVENTO (LA CAMPANADA), al azar entre dos:
//   a) CUERDA AL RELOJ: la esfera gira 1 paso ya mismo y +3 monedas.
//   b) MARCHA ATRAS: el jugador retrocede 3 casillas por la esfera.
//
// Animacion: partida.gimmickAnim guarda los pasos de giro que FALTAN
// animar (<= 0). Cada giro le resta N y ActualizarGimmick lo lleva a 0.
// El giro ya aplicado se deduce comparando los tipos actuales con el
// patron de diseno, asi que no hace falta estado estatico (la vista previa
// del menu usa otra PartidaTablero).

static const int CASILLAS_ESFERA = 24;

static const int BIFURCACION_HORAS = 12;
static const int BIFURCACION_MINUTOS = 18;
static const int LLEGADA_HORAS = 0;
static const int LLEGADA_MINUTOS = 6;

static const int HORAS_PRIMERA = 24;     // 24, 25 antes del eje
static const int CASILLA_EJE = 26;
static const int HORAS_SALIDA = 27;      // 27, 28 despues del eje
static const int MINUTOS_PRIMERA = 29;   // 29, 30 antes del eje
static const int MINUTOS_SALIDA = 31;    // 31, 32 despues del eje
static const int ULTIMA_CASILLA_RELOJ = 32;

static const int MIN_PASOS_GIRO = 1;
static const int MAX_PASOS_GIRO = 3;
static const int PREMIO_CUERDA = 3;
static const int PASOS_MARCHA_ATRAS = 3;

static const float RADIO_ESFERA = 8.2f;       // centro de las casillas
static const float RADIO_CARA = 9.7f;         // cara de marfil
static const float RADIO_BISEL = 10.0f;       // bisel fijo de laton
static const float RADIO_ARO = 10.5f;         // aro giratorio
static const float RADIO_DIENTES_ARO = 10.85f;
static const float ALTURA_CASILLA_RELOJ = 0.25f;
static const float SEPARACION_AGUJA = 2.733f;  // RADIO_ESFERA / 3

static const float VELOCIDAD_GIRO = 1.6f;     // pasos animados por segundo

static const float PI_RELOJ = 3.14159265f;
static const float GRADOS = 57.29578f;

// Patron de diseno del anillo (indice = casilla 0..23, empieza en las 6).
// N azul, + verde, - roja, R regalo, T tienda, V evento, I intercambio,
// D duelo. Solo N, + y - (salvo la casilla 0) giran.
static const char* const PATRON_ESFERA = "N+RNT-N+NV+IN-R+TN+D-VN+";

// Agujas: 24..28 (horas, con el eje en 26) y 29..32 (minutos).
static const char* const PATRON_AGUJAS = "-NC-+-+-N";

static const int CASILLAS_TROFEO_RELOJ[] =
{
    3,
    6,
    10,
    13,
    15,
    20,
    22,
    25,
    30
};


enum ZonaReloj
{
    ZONA_ESFERA = 0,
    ZONA_AGUJA_HORAS,
    ZONA_AGUJA_MINUTOS,
    ZONA_EJE,
    CANTIDAD_ZONAS_RELOJ
};

static const char* const NOMBRES_ZONAS_RELOJ[CANTIDAD_ZONAS_RELOJ] =
{
    "ESFERA",
    "AGUJA DE LAS HORAS",
    "AGUJA DE LOS MINUTOS",
    "EL EJE"
};


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


// Angulo de la esfera (0 = las 12, creciente en sentido horario).
static float AnguloCasillaEsfera(
    int indice
)
{
    return PI_RELOJ + indice * (2.0f * PI_RELOJ / CASILLAS_ESFERA);
}


// Punto de la esfera: las 12 hacia -z, las 3 hacia +x, las 6 hacia +z.
static Vector3 PuntoEsfera(
    float angulo,
    float radio,
    float y
)
{
    return { radio * std::sin(angulo), y, -radio * std::cos(angulo) };
}


static Vector3 PosicionAnillo(
    int indice
)
{
    return PuntoEsfera(
        AnguloCasillaEsfera(indice),
        RADIO_ESFERA,
        ALTURA_CASILLA_RELOJ
    );
}


static float Parpadeo(
    float tiempo,
    float velocidad
)
{
    return 0.5f + 0.5f * std::sin(tiempo * velocidad);
}


static float Suave01(float t)
{
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;

    return t * t * (3.0f - 2.0f * t);
}


//==================================================
// GIRO DE LA ESFERA
//==================================================

// La casilla del anillo gira con el patron (casilla de color, no la 0).
static bool CasillaGiratoria(
    int indice
)
{
    if (indice <= 0 || indice >= CASILLAS_ESFERA)
    {
        return false;
    }

    char letra = PATRON_ESFERA[indice];

    return letra == 'N' || letra == '+' || letra == '-';
}


// Indices de las casillas giratorias en orden horario. Devuelve cuantas.
static int ListarCasillasGiratorias(
    int lista[CASILLAS_ESFERA]
)
{
    int cantidad = 0;

    for (int i = 1; i < CASILLAS_ESFERA; i++)
    {
        if (CasillaGiratoria(i))
        {
            lista[cantidad] = i;
            cantidad++;
        }
    }

    return cantidad;
}


// Desplaza los tipos de las casillas giratorias `pasos` posiciones en
// sentido horario: el tipo de la posicion j pasa a la j + pasos.
static void GirarEsfera(
    Tablero& tablero,
    int pasos
)
{
    int lista[CASILLAS_ESFERA];
    int cantidad = ListarCasillasGiratorias(lista);

    if (cantidad <= 0 || tablero.cantidadCasillas < CASILLAS_ESFERA)
    {
        return;
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
// que la posicion j tiene el tipo de diseno de la posicion j - k. El
// patron no es periodico, asi que k es unico.
static int GiroActualEsfera(
    const Tablero& tablero,
    int& cantidad
)
{
    int lista[CASILLAS_ESFERA];
    cantidad = ListarCasillasGiratorias(lista);

    if (tablero.cantidadCasillas < CASILLAS_ESFERA)
    {
        return 0;
    }

    for (int k = 0; k < cantidad; k++)
    {
        bool coincide = true;

        for (int j = 0; j < cantidad && coincide; j++)
        {
            int origen = lista[(j - k + cantidad) % cantidad];

            coincide =
                tablero.casillas[lista[j]].tipo ==
                TipoDesdeLetra(PATRON_ESFERA[origen]);
        }

        if (coincide)
        {
            return k;
        }
    }

    return 0;
}


// Giro que se ve en el aro: el aplicado menos lo que falta animar, con un
// "tic" de relojeria (cada paso acelera y frena).
static float GiroVisualEsfera(
    const PartidaTablero& partida,
    int& cantidad
)
{
    float giro = (float)GiroActualEsfera(partida.tablero, cantidad);
    float pendiente = partida.gimmickAnim;

    if (pendiente < 0.0f)
    {
        float base = std::floor(pendiente);
        pendiente = base + Suave01(pendiente - base);
    }
    else
    {
        pendiente = 0.0f;
    }

    return giro + pendiente;
}


// Gira la esfera y deja los pasos pendientes de animar.
static void AplicarGiroEsfera(
    PartidaTablero& partida,
    int pasos
)
{
    GirarEsfera(partida.tablero, pasos);

    if (partida.gimmickAnim > 0.0f)
    {
        partida.gimmickAnim = 0.0f;
    }

    partida.gimmickAnim -= (float)pasos;
    partida.gimmickActivo = true;
}


//==================================================
// CONSTRUCCION
//==================================================

static int NodoReloj(
    Tablero& tablero,
    ZonaReloj zona,
    char letra,
    Vector3 posicion
)
{
    int indice = tablero.AgregarCasilla(posicion, TipoDesdeLetra(letra));

    if (indice >= 0)
    {
        tablero.casillas[indice].zona = (int)zona;
    }

    return indice;
}


static void ConstruirTorreReloj(
    Tablero& tablero
)
{
    tablero.cantidadCasillas = 0;
    tablero.recorridoValido = false;

    for (int i = 0; i < MAX_CASILLAS_TABLERO; i++)
    {
        tablero.casillas[i] = Casilla{};
    }

    // Esfera: 0..23 en sentido horario desde las 6.
    for (int i = 0; i < CASILLAS_ESFERA; i++)
    {
        NodoReloj(tablero, ZONA_ESFERA, PATRON_ESFERA[i], PosicionAnillo(i));
    }

    const float y = ALTURA_CASILLA_RELOJ;
    const float s = SEPARACION_AGUJA;

    // Aguja de las horas (columna x = 0, de las 12 hacia las 6).
    NodoReloj(tablero, ZONA_AGUJA_HORAS, PATRON_AGUJAS[0], { 0.0f, y, -2.0f * s }); // 24
    NodoReloj(tablero, ZONA_AGUJA_HORAS, PATRON_AGUJAS[1], { 0.0f, y, -s });        // 25
    NodoReloj(tablero, ZONA_EJE, PATRON_AGUJAS[2], { 0.0f, y, 0.0f });              // 26
    NodoReloj(tablero, ZONA_AGUJA_HORAS, PATRON_AGUJAS[3], { 0.0f, y, s });         // 27
    NodoReloj(tablero, ZONA_AGUJA_HORAS, PATRON_AGUJAS[4], { 0.0f, y, 2.0f * s });  // 28

    // Aguja de los minutos (fila z = 0, de las 3 hacia las 9).
    NodoReloj(tablero, ZONA_AGUJA_MINUTOS, PATRON_AGUJAS[5], { 2.0f * s, y, 0.0f });  // 29
    NodoReloj(tablero, ZONA_AGUJA_MINUTOS, PATRON_AGUJAS[6], { s, y, 0.0f });         // 30
    NodoReloj(tablero, ZONA_AGUJA_MINUTOS, PATRON_AGUJAS[7], { -s, y, 0.0f });        // 31
    NodoReloj(tablero, ZONA_AGUJA_MINUTOS, PATRON_AGUJAS[8], { -2.0f * s, y, 0.0f }); // 32

    // La esfera es un circuito cerrado (conexion 0 = seguir la esfera).
    for (int i = 0; i < CASILLAS_ESFERA; i++)
    {
        tablero.ConectarCasillas(i, (i + 1) % CASILLAS_ESFERA);
    }

    // Aguja de las horas: las 12 -> 24 -> 25 -> EJE.
    tablero.ConectarCasillas(BIFURCACION_HORAS, HORAS_PRIMERA);
    tablero.ConectarCasillas(HORAS_PRIMERA, HORAS_PRIMERA + 1);
    tablero.ConectarCasillas(HORAS_PRIMERA + 1, CASILLA_EJE);

    // Aguja de los minutos: las 3 -> 29 -> 30 -> EJE.
    tablero.ConectarCasillas(BIFURCACION_MINUTOS, MINUTOS_PRIMERA);
    tablero.ConectarCasillas(MINUTOS_PRIMERA, MINUTOS_PRIMERA + 1);
    tablero.ConectarCasillas(MINUTOS_PRIMERA + 1, CASILLA_EJE);

    // EL EJE: conexion 0 baja hacia las 6, conexion 1 dobla hacia las 9.
    tablero.ConectarCasillas(CASILLA_EJE, HORAS_SALIDA);
    tablero.ConectarCasillas(HORAS_SALIDA, HORAS_SALIDA + 1);
    tablero.ConectarCasillas(HORAS_SALIDA + 1, LLEGADA_HORAS);

    tablero.ConectarCasillas(CASILLA_EJE, MINUTOS_SALIDA);
    tablero.ConectarCasillas(MINUTOS_SALIDA, MINUTOS_SALIDA + 1);
    tablero.ConectarCasillas(MINUTOS_SALIDA + 1, LLEGADA_MINUTOS);

    tablero.recorridoValido =
        tablero.ValidarRecorrido();
}


//==================================================
// COLORES
//==================================================

static const Color BRONCE_OSCURO = { 86, 56, 32, 255 };
static const Color BRONCE = { 146, 98, 50, 255 };
static const Color LATON = { 202, 158, 80, 255 };
static const Color ORO_RELOJ = { 240, 200, 96, 255 };
static const Color MARFIL = { 228, 216, 186, 255 };
static const Color MARFIL_PISTA = { 210, 196, 164, 255 };
static const Color TINTA = { 56, 44, 40, 255 };
static const Color ACERO = { 148, 150, 162, 255 };
static const Color MADERA = { 86, 58, 38, 255 };
static const Color MADERA_CLARA = { 104, 72, 46, 255 };
static const Color MADERA_OSCURA = { 44, 30, 22, 255 };
static const Color PIEDRA = { 58, 58, 76, 255 };
static const Color PIEDRA_JUNTA = { 40, 40, 54, 255 };
static const Color VIDRIO_LUNA = { 112, 142, 198, 255 };


//==================================================
// MALLAS HORNEADAS
//==================================================
//
// La geometria que no se mueve (piso, cara del reloj, marcas, placas del
// camino, agujas, tornillos y la arquitectura de la torre) se hornea en
// una malla. El aro giratorio y el engranaje (una sola malla reutilizada
// con distinta escala) tambien, y se dibujan con rlRotatef. Se rehacen si
// cambia la calidad de decoracion y se liberan en DescargarRecursos.

static MallaGpu mallaFijaReloj;
static MallaGpu mallaAroReloj;
static MallaGpu mallaEngranajeReloj;
static int nivelMallasReloj = -1;


// Corona plana horizontal entre dos radios (dos caras, sin tapa central).
static void AnilloPlanoMalla(
    ConstructorMalla& malla,
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

        Vector3 i0 = PuntoEsfera(a0, radioInterior, y);
        Vector3 i1 = PuntoEsfera(a1, radioInterior, y);
        Vector3 e0 = PuntoEsfera(a0, radioExterior, y);
        Vector3 e1 = PuntoEsfera(a1, radioExterior, y);

        malla.TrianguloDobleCara(i0, e0, e1, color);
        malla.TrianguloDobleCara(i0, e1, i1, color);
    }
}


// Pared vertical circular sin tapas (borde de un aro).
static void ParedCircularMalla(
    ConstructorMalla& malla,
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

        Vector3 b0 = PuntoEsfera(a0, radio, y0);
        Vector3 b1 = PuntoEsfera(a1, radio, y0);
        Vector3 t0 = PuntoEsfera(a0, radio, y1);
        Vector3 t1 = PuntoEsfera(a1, radio, y1);

        malla.TrianguloDobleCara(b0, b1, t1, color);
        malla.TrianguloDobleCara(b0, t1, t0, color);
    }
}


// Caja orientada en la direccion radial del angulo de la esfera: `local`
// se mide con las 12 hacia -z (x = lateral, z = -radio).
static void CajaRadialMalla(
    ConstructorMalla& malla,
    float angulo,
    Vector3 local,
    Vector3 tamano,
    Color color
)
{
    TransformacionMalla t;
    t.giroY = -angulo * GRADOS;

    malla.EstablecerTransformacion(t);
    malla.Caja(local, tamano, color);
    malla.QuitarTransformacion();
}


// Punta de aguja en forma de rombo plano.
static void RomboAgujaMalla(
    ConstructorMalla& malla,
    Vector3 centro,
    Vector3 direccion,
    float largoPunta,
    float largoCola,
    float semiancho,
    Color color
)
{
    Vector3 lateral = { -direccion.z, 0.0f, direccion.x };

    Vector3 punta =
    {
        centro.x + direccion.x * largoPunta,
        centro.y,
        centro.z + direccion.z * largoPunta
    };

    Vector3 cola =
    {
        centro.x - direccion.x * largoCola,
        centro.y,
        centro.z - direccion.z * largoCola
    };

    Vector3 izquierda =
    {
        centro.x + lateral.x * semiancho,
        centro.y,
        centro.z + lateral.z * semiancho
    };

    Vector3 derecha =
    {
        centro.x - lateral.x * semiancho,
        centro.y,
        centro.z - lateral.z * semiancho
    };

    malla.TrianguloDobleCara(punta, izquierda, cola, color);
    malla.TrianguloDobleCara(punta, cola, derecha, color);
}


static void ConstruirPisoYTorre(
    ConstructorMalla& m,
    NivelCalidadGrafica nivel
)
{
    // Piso de tablones de la torre (tapa a y = -0.30).
    m.Caja({ 0.0f, -0.62f, 0.5f }, { 36.4f, 0.3f, 27.4f }, MADERA_OSCURA);

    for (int i = 0; i < 24; i++)
    {
        m.Caja(
            { -17.25f + 1.5f * i, -0.50f, 0.5f },
            { 1.44f, 0.40f, 27.0f },
            i % 2 == 0 ? MADERA : MADERA_CLARA
        );
    }

    // Muro de piedra del fondo.
    m.Caja({ 0.0f, 5.0f, -13.6f }, { 36.0f, 11.0f, 1.2f }, PIEDRA);

    if (nivel != CALIDAD_BAJA)
    {
        // Juntas horizontales de los sillares.
        for (int fila = 0; fila < 10; fila++)
        {
            m.Caja(
                { 0.0f, 0.2f + fila * 1.05f, -12.98f },
                { 36.0f, 0.07f, 0.05f },
                PIEDRA_JUNTA
            );

            // Juntas verticales alternadas (solo en ALTA).
            for (int c = 0; nivel == CALIDAD_ALTA && c < 18; c++)
            {
                float x = -17.0f + 2.0f * c + (fila % 2 == 0 ? 0.0f : 1.0f);

                m.Caja(
                    { x, 0.72f + fila * 1.05f, -12.98f },
                    { 0.07f, 1.0f, 0.05f },
                    PIEDRA_JUNTA
                );
            }
        }

        // Columnas laterales de la torre.
        const float columnasZ[] = { -11.0f, -2.0f, 7.0f };

        for (float z : columnasZ)
        {
            for (int lado = -1; lado <= 1; lado += 2)
            {
                float x = 16.0f * lado;

                m.Caja({ x, -0.05f, z }, { 1.9f, 0.5f, 1.9f }, PIEDRA_JUNTA);
                m.Cilindro({ x, -0.3f, z }, { x, 12.0f, z }, 0.75f, 0.70f, 12, PIEDRA);
            }
        }
    }

    // Caja del pendulo (centro del muro).
    m.Caja({ 0.0f, -0.05f, -12.4f }, { 6.4f, 0.5f, 1.2f }, MADERA_OSCURA);
    m.Caja({ 0.0f, 4.4f, -12.9f }, { 5.4f, 8.0f, 0.1f }, MADERA);
    m.Caja({ -2.9f, 4.3f, -12.4f }, { 0.35f, 9.2f, 0.35f }, MADERA_OSCURA);
    m.Caja({ 2.9f, 4.3f, -12.4f }, { 0.35f, 9.2f, 0.35f }, MADERA_OSCURA);
    m.Caja({ 0.0f, 8.7f, -12.4f }, { 6.2f, 0.4f, 0.5f }, MADERA_OSCURA);

    // Armazon de la campana (izquierda).
    m.Caja({ -12.0f, 3.3f, -11.4f }, { 0.35f, 7.2f, 0.35f }, MADERA_OSCURA);
    m.Caja({ -8.2f, 3.3f, -11.4f }, { 0.35f, 7.2f, 0.35f }, MADERA_OSCURA);
    m.Caja({ -10.1f, 6.9f, -11.4f }, { 4.3f, 0.4f, 0.5f }, MADERA_OSCURA);

    // Ventana con luna (derecha).
    m.Caja({ 10.5f, 3.9f, -12.92f }, { 2.8f, 4.0f, 0.12f }, MADERA_OSCURA);
    m.Caja({ 10.5f, 3.9f, -12.85f }, { 2.3f, 3.5f, 0.06f }, VIDRIO_LUNA);
    m.Cilindro(
        { 11.0f, 4.7f, -12.83f },
        { 11.0f, 4.7f, -12.81f },
        0.42f, 0.42f, 16,
        Color{ 238, 236, 214, 255 }
    );
    m.Caja({ 10.5f, 3.9f, -12.78f }, { 0.12f, 3.5f, 0.08f }, MADERA_OSCURA);
    m.Caja({ 10.5f, 4.2f, -12.78f }, { 2.3f, 0.12f, 0.08f }, MADERA_OSCURA);
}


static void ConstruirCaraReloj(
    ConstructorMalla& m,
    const Tablero& tablero,
    NivelCalidadGrafica nivel
)
{
    int lados = nivel == CALIDAD_BAJA ? 40 : 72;

    // Zocalo, bisel fijo y cara de marfil (tapa a y = 0).
    m.Cilindro({ 0.0f, -0.33f, 0.0f }, { 0.0f, -0.14f, 0.0f }, 11.0f, 10.8f, lados, BRONCE_OSCURO);
    AnilloPlanoMalla(m, RADIO_CARA, RADIO_BISEL, -0.01f, lados, LATON);
    ParedCircularMalla(m, RADIO_BISEL, -0.14f, -0.01f, lados, BRONCE);
    m.Cilindro({ 0.0f, -0.12f, 0.0f }, { 0.0f, 0.0f, 0.0f }, RADIO_CARA, RADIO_CARA, lados, MARFIL);

    // Pista de las casillas enmarcada por dos circulos de tinta.
    AnilloPlanoMalla(m, 7.05f, 9.35f, 0.003f, lados, MARFIL_PISTA);
    AnilloPlanoMalla(m, 9.35f, 9.45f, 0.006f, lados, TINTA);
    AnilloPlanoMalla(m, 6.95f, 7.05f, 0.006f, lados, TINTA);
    AnilloPlanoMalla(m, 3.95f, 4.05f, 0.006f, lados, TINTA);

    // Minutero exterior (60 marcas), solo en ALTA.
    for (int i = 0; nivel == CALIDAD_ALTA && i < 60; i++)
    {
        bool larga = i % 5 == 0;

        CajaRadialMalla(
            m,
            2.0f * PI_RELOJ * i / 60.0f,
            { 0.0f, 0.01f, -9.58f },
            { larga ? 0.09f : 0.05f, 0.02f, larga ? 0.22f : 0.12f },
            TINTA
        );
    }

    // Marcas de las horas (las cardinales, dobles a los lados de la aguja).
    for (int h = 0; h < 12; h++)
    {
        float angulo = 2.0f * PI_RELOJ * h / 12.0f;

        if (h % 3 == 0)
        {
            for (int lado = -1; lado <= 1; lado += 2)
            {
                CajaRadialMalla(
                    m, angulo,
                    { 0.95f * lado, 0.01f, -6.25f },
                    { 0.20f, 0.03f, 1.1f },
                    TINTA
                );
            }
        }
        else
        {
            CajaRadialMalla(m, angulo, { 0.0f, 0.01f, -6.3f }, { 0.22f, 0.03f, 0.9f }, TINTA);
            CajaRadialMalla(m, angulo, { 0.0f, 0.012f, -4.0f }, { 0.26f, 0.03f, 0.26f }, BRONCE);
        }
    }

    // Placas metalicas del camino de la esfera.
    for (int i = 0; i < CASILLAS_ESFERA; i++)
    {
        const Casilla* a = tablero.ObtenerCasilla(i);
        const Casilla* b = tablero.ObtenerCasilla((i + 1) % CASILLAS_ESFERA);

        if (a == nullptr || b == nullptr)
        {
            continue;
        }

        AgregarCintaSueloMalla(m, a->posicion, b->posicion, 0.95f, 0.012f, BRONCE_OSCURO);
        AgregarCintaSueloMalla(m, a->posicion, b->posicion, 0.60f, 0.020f, LATON);
    }

    // AGUJA DE LAS HORAS (vertical, punta hacia las 12): es el camino
    // fisico de su atajo.
    const float largoAguja = 7.4f;
    const float s = SEPARACION_AGUJA;

    m.Caja({ 0.0f, 0.025f, 0.0f }, { 1.1f, 0.05f, 2.0f * largoAguja }, BRONCE_OSCURO);
    m.Caja({ 0.0f, 0.056f, 0.0f }, { 0.5f, 0.012f, 2.0f * largoAguja - 0.2f }, LATON);
    RomboAgujaMalla(m, { 0.0f, 0.052f, -2.0f * s }, { 0.0f, 0.0f, -1.0f }, 1.9f, 1.6f, 1.25f, BRONCE_OSCURO);
    RomboAgujaMalla(m, { 0.0f, 0.064f, -2.0f * s }, { 0.0f, 0.0f, -1.0f }, 1.4f, 1.1f, 0.8f, LATON);
    m.Cilindro({ 0.0f, 0.0f, 2.0f * s }, { 0.0f, 0.055f, 2.0f * s }, 1.15f, 1.15f, 20, BRONCE_OSCURO);

    // AGUJA DE LOS MINUTOS (horizontal, punta hacia las 3), por encima.
    m.Caja({ 0.0f, 0.06f, 0.0f }, { 2.0f * largoAguja, 0.05f, 0.9f }, BRONCE);
    m.Caja({ 0.0f, 0.091f, 0.0f }, { 2.0f * largoAguja - 0.2f, 0.012f, 0.42f }, ORO_RELOJ);
    RomboAgujaMalla(m, { 2.0f * s, 0.088f, 0.0f }, { 1.0f, 0.0f, 0.0f }, 1.9f, 1.6f, 1.15f, BRONCE);
    RomboAgujaMalla(m, { 2.0f * s, 0.10f, 0.0f }, { 1.0f, 0.0f, 0.0f }, 1.4f, 1.1f, 0.7f, ORO_RELOJ);
    m.Cilindro({ -2.0f * s, 0.0f, 0.0f }, { -2.0f * s, 0.09f, 0.0f }, 1.0f, 1.0f, 20, BRONCE);

    // Cubo central del eje.
    m.Cilindro({ 0.0f, 0.0f, 0.0f }, { 0.0f, 0.11f, 0.0f }, 1.75f, 1.75f, 24, BRONCE_OSCURO);
    m.Cilindro({ 0.0f, 0.0f, 0.0f }, { 0.0f, 0.16f, 0.0f }, 1.5f, 1.5f, 24, LATON);

    // Tornillos: las casillas fijas del anillo estan "atornilladas".
    for (int i = 0; i < CASILLAS_ESFERA; i++)
    {
        if (CasillaGiratoria(i))
        {
            continue;
        }

        Vector3 centro = PosicionAnillo(i);

        for (int k = 0; k < 4; k++)
        {
            float a = AnguloCasillaEsfera(i) + PI_RELOJ * 0.25f + k * PI_RELOJ * 0.5f;
            float x = centro.x + std::sin(a) * 1.0f;
            float z = centro.z - std::cos(a) * 1.0f;

            m.Cilindro({ x, 0.0f, z }, { x, 0.08f, z }, 0.12f, 0.10f, 6, ACERO);
        }
    }
}


// Aro dentado alrededor de la esfera (gira con el patron). La marca dorada
// queda en las 6 cuando el giro acumulado es 0.
static void ConstruirAroReloj(
    ConstructorMalla& m,
    NivelCalidadGrafica nivel
)
{
    int lados = nivel == CALIDAD_BAJA ? 40 : 72;

    AnilloPlanoMalla(m, RADIO_BISEL, RADIO_ARO, 0.03f, lados, BRONCE);
    ParedCircularMalla(m, RADIO_ARO, -0.14f, 0.03f, lados, BRONCE_OSCURO);
    ParedCircularMalla(m, RADIO_BISEL, -0.01f, 0.03f, lados, BRONCE_OSCURO);

    const int dientes = 48;

    for (int i = 0; i < dientes; i++)
    {
        float angulo = 2.0f * PI_RELOJ * i / dientes;

        CajaRadialMalla(
            m, angulo,
            { 0.0f, -0.04f, -(RADIO_ARO + (RADIO_DIENTES_ARO - RADIO_ARO) * 0.5f) },
            { 0.42f, 0.18f, RADIO_DIENTES_ARO - RADIO_ARO + 0.04f },
            BRONCE
        );

        // Remaches cada 4 dientes: hacen visible el giro.
        if (i % 4 == 0)
        {
            Vector3 p = PuntoEsfera(angulo, (RADIO_BISEL + RADIO_ARO) * 0.5f, 0.03f);

            m.Cilindro(p, { p.x, p.y + 0.05f, p.z }, 0.09f, 0.07f, 6, LATON);
        }
    }

    // Marca dorada (en las 6 = hacia la camara).
    CajaRadialMalla(
        m, PI_RELOJ,
        { 0.0f, 0.07f, -(RADIO_BISEL + RADIO_ARO) * 0.5f },
        { 0.55f, 0.10f, 0.40f },
        ORO_RELOJ
    );

    Vector3 joya = PuntoEsfera(PI_RELOJ, (RADIO_BISEL + RADIO_ARO) * 0.5f, 0.16f);
    m.Esfera(joya, 0.16f, 6, 8, Color{ 230, 70, 60, 255 });
}


// Engranaje unitario (radio 1, eje Y, grosor 0.3).
static void ConstruirEngranajeReloj(
    ConstructorMalla& m,
    NivelCalidadGrafica nivel
)
{
    int lados = nivel == CALIDAD_BAJA ? 14 : 24;

    m.Cilindro({ 0.0f, -0.15f, 0.0f }, { 0.0f, 0.15f, 0.0f }, 0.86f, 0.86f, lados, BRONCE);
    m.Cilindro({ 0.0f, 0.15f, 0.0f }, { 0.0f, 0.17f, 0.0f }, 0.66f, 0.66f, lados, LATON);
    m.Cilindro({ 0.0f, -0.17f, 0.0f }, { 0.0f, -0.15f, 0.0f }, 0.66f, 0.66f, lados, LATON);
    m.Cilindro({ 0.0f, -0.26f, 0.0f }, { 0.0f, 0.26f, 0.0f }, 0.2f, 0.2f, 10, BRONCE_OSCURO);

    // Agujeros oscuros en la cara (se nota el giro).
    for (int k = 0; k < 4; k++)
    {
        float a = k * PI_RELOJ * 0.5f;
        float x = std::sin(a) * 0.43f;
        float z = std::cos(a) * 0.43f;

        m.Cilindro({ x, 0.165f, z }, { x, 0.18f, z }, 0.13f, 0.13f, 8, MADERA_OSCURA);
        m.Cilindro({ x, -0.18f, z }, { x, -0.165f, z }, 0.13f, 0.13f, 8, MADERA_OSCURA);
    }

    const int dientes = 16;

    for (int i = 0; i < dientes; i++)
    {
        CajaRadialMalla(
            m,
            2.0f * PI_RELOJ * i / dientes,
            { 0.0f, 0.0f, -0.95f },
            { 0.20f, 0.30f, 0.24f },
            BRONCE
        );
    }
}


static void PrepararMallasReloj(
    const Tablero& tablero
)
{
    NivelCalidadGrafica nivel = CalidadDecoracion();

    if (nivelMallasReloj == (int)nivel && mallaFijaReloj.cargada)
    {
        return;
    }

    ConstructorMalla constructor;

    ConstruirPisoYTorre(constructor, nivel);
    ConstruirCaraReloj(constructor, tablero, nivel);
    ActualizarMallaGpu(mallaFijaReloj, constructor);

    constructor.Limpiar();
    ConstruirAroReloj(constructor, nivel);
    ActualizarMallaGpu(mallaAroReloj, constructor);

    constructor.Limpiar();
    ConstruirEngranajeReloj(constructor, nivel);
    ActualizarMallaGpu(mallaEngranajeReloj, constructor);

    nivelMallasReloj = (int)nivel;
}


static void DescargarRecursosReloj()
{
    DescargarMalla(mallaFijaReloj);
    DescargarMalla(mallaAroReloj);
    DescargarMalla(mallaEngranajeReloj);
    nivelMallasReloj = -1;
}


//==================================================
// DECORACION
//==================================================

struct EngranajeReloj
{
    Vector3 posicion;
    float radio;
    bool vertical;          // true = en el muro (eje Z); false = en el piso
    float velocidad;        // grados por segundo (el signo es el sentido)
    float acople;           // vueltas por cada grado del aro (motor del aro)
    NivelCalidadGrafica nivelMinimo;
};

// Los pares que engranan giran en sentidos opuestos y a velocidad inversa
// a su radio. Los dos pinones junto al aro solo se mueven con el giro.
static const EngranajeReloj ENGRANAJES_RELOJ[] =
{
    { { 8.27f, -0.12f, 8.27f }, 0.95f, false, 0.0f, RADIO_DIENTES_ARO / 0.95f, CALIDAD_BAJA },
    { { -8.27f, -0.12f, 8.27f }, 0.95f, false, 0.0f, RADIO_DIENTES_ARO / 0.95f, CALIDAD_BAJA },
    { { -5.8f, 2.9f, -12.75f }, 2.0f, true, 14.0f, 0.0f, CALIDAD_BAJA },
    { { 6.0f, 2.6f, -12.75f }, 2.2f, true, -10.0f, 0.0f, CALIDAD_BAJA },
    { { -4.4f, 5.6f, -12.75f }, 1.1f, true, -25.5f, 0.0f, CALIDAD_MEDIA },
    { { 7.3f, 5.8f, -12.75f }, 1.2f, true, 18.3f, 0.0f, CALIDAD_MEDIA },
    { { -13.4f, -0.12f, 5.2f }, 2.4f, false, 9.0f, 0.0f, CALIDAD_MEDIA },
    { { 13.2f, -0.12f, 4.4f }, 2.1f, false, -8.0f, 0.0f, CALIDAD_MEDIA },
    { { -12.9f, -0.12f, 0.6f }, 2.0f, false, -10.8f, 0.0f, CALIDAD_ALTA },
    { { 13.6f, -0.12f, -0.4f }, 2.6f, false, 6.5f, 0.0f, CALIDAD_ALTA }
};


static void DibujarEngranajesReloj(
    float tiempo,
    float anguloAro
)
{
    NivelCalidadGrafica nivel = CalidadDecoracion();

    for (const EngranajeReloj& e : ENGRANAJES_RELOJ)
    {
        if ((int)nivel < (int)e.nivelMinimo)
        {
            continue;
        }

        float angulo = tiempo * e.velocidad + anguloAro * e.acople;

        rlPushMatrix();
        rlTranslatef(e.posicion.x, e.posicion.y, e.posicion.z);

        if (e.vertical)
        {
            // El eje Y del engranaje pasa a mirar a la camara (+Z).
            rlRotatef(90.0f, 1.0f, 0.0f, 0.0f);
        }

        rlRotatef(angulo, 0.0f, 1.0f, 0.0f);
        rlScalef(e.radio, 1.0f, e.radio);

        DibujarMallaGpu(mallaEngranajeReloj);

        rlPopMatrix();
    }
}


static void DibujarPenduloReloj(
    float tiempo
)
{
    // MODELO FUTURO: pendulo tallado como modelo 3D.
    float angulo = 16.0f * std::sin(tiempo * 2.0f * PI_RELOJ / 2.6f);

    rlPushMatrix();
    rlTranslatef(0.0f, 8.4f, -12.1f);
    rlRotatef(angulo, 0.0f, 0.0f, 1.0f);

    DrawCube({ 0.0f, -2.7f, 0.0f }, 0.14f, 5.4f, 0.10f, LATON);

    DrawCylinderEx(
        { 0.0f, -5.4f, -0.10f },
        { 0.0f, -5.4f, 0.10f },
        0.85f, 0.85f, 18,
        ORO_RELOJ
    );

    DrawCylinderEx(
        { 0.0f, -5.4f, 0.10f },
        { 0.0f, -5.4f, 0.13f },
        0.55f, 0.55f, 18,
        BRONCE
    );

    rlPopMatrix();
}


// La campana se mece sola, mas fuerte mientras la esfera gira y a todo
// vuelo cuando alguien cae en LA CAMPANADA.
static void DibujarCampanaReloj(
    const PartidaTablero& partida
)
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

    float angulo = amplitud * std::sin(partida.tiempoTablero * 3.2f);

    rlPushMatrix();
    rlTranslatef(-10.1f, 6.7f, -11.4f);
    rlRotatef(angulo, 0.0f, 0.0f, 1.0f);

    DrawCube({ 0.0f, -0.25f, 0.0f }, 0.12f, 0.5f, 0.12f, ACERO);
    DrawSphere({ 0.0f, -0.6f, 0.0f }, 0.42f, BRONCE);
    DrawCylinder({ 0.0f, -1.85f, 0.0f }, 0.42f, 1.0f, 1.25f, 16, BRONCE);
    DrawCylinder({ 0.0f, -1.95f, 0.0f }, 1.05f, 1.08f, 0.14f, 16, LATON);
    DrawSphere({ 0.0f, -1.9f, 0.0f }, 0.18f, BRONCE_OSCURO);

    rlPopMatrix();
}


static void DibujarDecoracionTorreReloj(
    const PartidaTablero& partida
)
{
    // MODELO FUTURO: la sala de la torre y la esfera como modelo 3D.
    PrepararMallasReloj(partida.tablero);

    DibujarMallaGpu(mallaFijaReloj);

    int cantidad = 0;
    float giro = GiroVisualEsfera(partida, cantidad);
    float anguloAro =
        cantidad > 0
        ? giro * 360.0f / (float)cantidad
        : 0.0f;

    DibujarEngranajesReloj(partida.tiempoTablero, anguloAro);
    DibujarPenduloReloj(partida.tiempoTablero);
    DibujarCampanaReloj(partida);
}


//==================================================
// GIMMICK (dibujo)
//==================================================

// Haz de luna desde la ventana y polvo flotando (translucidos).
static void DibujarLuzLunaReloj(
    float tiempo
)
{
    if (CalidadEfectos() == CALIDAD_BAJA)
    {
        return;
    }

    const Color haz = { 170, 190, 240, 26 };
    const Color mancha = { 170, 190, 240, 40 };

    Vector3 vArribaIzq = { 9.35f, 5.65f, -12.8f };
    Vector3 vArribaDer = { 11.65f, 5.65f, -12.8f };
    Vector3 vAbajoIzq = { 9.35f, 2.15f, -12.8f };
    Vector3 vAbajoDer = { 11.65f, 2.15f, -12.8f };

    Vector3 pCercaIzq = { 8.4f, -0.28f, -6.6f };
    Vector3 pCercaDer = { 11.0f, -0.28f, -6.6f };
    Vector3 pLejosIzq = { 8.6f, -0.28f, -8.6f };
    Vector3 pLejosDer = { 11.2f, -0.28f, -8.6f };

    rlDrawRenderBatchActive();
    rlDisableDepthMask();
    rlDisableBackfaceCulling();

    DrawTriangle3D(vArribaIzq, pCercaIzq, pCercaDer, haz);
    DrawTriangle3D(vArribaIzq, pCercaDer, vArribaDer, haz);
    DrawTriangle3D(vAbajoIzq, pLejosIzq, pLejosDer, haz);
    DrawTriangle3D(vAbajoIzq, pLejosDer, vAbajoDer, haz);
    DrawTriangle3D(pLejosIzq, pCercaIzq, pCercaDer, mancha);
    DrawTriangle3D(pLejosIzq, pCercaDer, pLejosDer, mancha);

    int motas = (int)std::ceil(14.0f * FactorCalidadGrafica(CalidadParticulas()));

    for (int i = 0; i < motas; i++)
    {
        float u = std::fmod(i * 0.381f + tiempo * 0.04f, 1.0f);
        float lado = std::fmod(i * 0.618f, 1.0f);

        Vector3 p =
        {
            9.4f + lado * 2.0f + 0.2f * std::sin(tiempo * 0.7f + i),
            4.0f * (1.0f - u) + 0.3f * std::sin(tiempo * 0.5f + i * 2.1f),
            -12.6f + u * 5.0f
        };

        DrawCube(p, 0.05f, 0.05f, 0.05f, Color{ 236, 236, 255, 170 });
    }

    rlDrawRenderBatchActive();
    rlEnableBackfaceCulling();
    rlEnableDepthMask();
}


// Halos dorados en las casillas que se mueven mientras la esfera gira.
static void DibujarHalosGiroReloj(
    const PartidaTablero& partida
)
{
    if (partida.gimmickAnim > -0.001f)
    {
        return;
    }

    int lista[CASILLAS_ESFERA];
    int cantidad = ListarCasillasGiratorias(lista);

    float pulso = Parpadeo(partida.tiempoTablero, 9.0f);

    Color halo =
    {
        255,
        (unsigned char)(196 + (int)(pulso * 40.0f)),
        96,
        (unsigned char)(110 + (int)(pulso * 90.0f))
    };

    rlDrawRenderBatchActive();
    rlDisableDepthMask();

    for (int j = 0; j < cantidad; j++)
    {
        const Casilla* casilla = partida.tablero.ObtenerCasilla(lista[j]);

        if (casilla == nullptr)
        {
            continue;
        }

        DrawCylinder(
            { casilla->posicion.x, 0.025f, casilla->posicion.z },
            1.06f, 1.06f, 0.02f, 20,
            halo
        );
    }

    rlDrawRenderBatchActive();
    rlEnableDepthMask();
}


static void DibujarGimmickTorreReloj(
    const PartidaTablero& partida
)
{
    PrepararMallasReloj(partida.tablero);

    // Aro giratorio: sentido horario visto desde arriba (angulo negativo).
    int cantidad = 0;
    float giro = GiroVisualEsfera(partida, cantidad);
    float anguloAro =
        cantidad > 0
        ? giro * 360.0f / (float)cantidad
        : 0.0f;

    rlPushMatrix();
    rlRotatef(-anguloAro, 0.0f, 1.0f, 0.0f);
    DibujarMallaGpu(mallaAroReloj);
    rlPopMatrix();

    DibujarHalosGiroReloj(partida);
    DibujarLuzLunaReloj(partida.tiempoTablero);
}


//==================================================
// GIMMICK (logica)
//==================================================

static void ActualizarGimmickTorreReloj(
    PartidaTablero& partida,
    float deltaTime
)
{
    // gimmickAnim = pasos de giro que faltan animar (<= 0).
    if (partida.gimmickAnim < 0.0f)
    {
        partida.gimmickAnim += VELOCIDAD_GIRO * deltaTime;
    }

    if (partida.gimmickAnim >= 0.0f)
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

        std::snprintf(
            partida.estadoGimmick,
            sizeof(partida.estadoGimmick),
            "LA ESFERA GIRA AL EMPEZAR CADA RONDA (DESDE LA 2)"
        );

        EstablecerMensajeGimmick(
            partida,
            "LAS CASILLAS DE COLOR GIRAN CADA RONDA"
        );

        return;
    }

    int pasos = GetRandomValue(MIN_PASOS_GIRO, MAX_PASOS_GIRO);

    AplicarGiroEsfera(partida, pasos);

    std::snprintf(
        partida.estadoGimmick,
        sizeof(partida.estadoGimmick),
        "LA ESFERA GIRA AL EMPEZAR CADA RONDA (ÚLTIMO GIRO: %d)",
        pasos
    );

    char mensaje[96];

    std::snprintf(
        mensaje,
        sizeof(mensaje),
        "¡LA ESFERA GIRA %d %s!",
        pasos,
        pasos == 1 ? "PASO" : "PASOS"
    );

    EstablecerMensajeGimmick(partida, mensaje);
}


//==================================================
// EVENTO PROPIO: LA CAMPANADA
//==================================================

// Casilla a la que vuelve MARCHA ATRAS: 3 casillas antes por la esfera o,
// desde una aguja, la casilla de la esfera de donde sale esa aguja. Nunca
// deja al jugador sobre el trofeo.
static int CasillaMarchaAtras(
    const PartidaTablero& partida,
    int casilla
)
{
    int destino = BIFURCACION_HORAS;

    if (casilla >= 0 && casilla < CASILLAS_ESFERA)
    {
        destino =
            (casilla - PASOS_MARCHA_ATRAS + CASILLAS_ESFERA) % CASILLAS_ESFERA;
    }
    else if (casilla >= MINUTOS_PRIMERA && casilla <= ULTIMA_CASILLA_RELOJ)
    {
        destino = BIFURCACION_MINUTOS;
    }

    if (destino == partida.casillaTrofeo)
    {
        destino = (destino - 1 + CASILLAS_ESFERA) % CASILLAS_ESFERA;
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
        AplicarGiroEsfera(partida, 1);

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
        // b) MARCHA ATRAS: retrocede por la esfera.
        TeletransportarJugadorTablero(
            partida,
            participante,
            CasillaMarchaAtras(partida, jugador.casillaActual)
        );

        std::snprintf(
            partida.textoEvento,
            sizeof(partida.textoEvento),
            "CAMPANADA: ¡MARCHA ATRÁS! J%d RETROCEDE POR LA ESFERA",
            participante + 1
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
// de casillas mas ricas segun como quedo la esfera. Siempre deja algo de azar.
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

    if (casilla == BIFURCACION_HORAS)
    {
        return opcion == 0
            ? "SEGUIR LA ESFERA"
            : "POR LA AGUJA DE LAS HORAS (ATAJO)";
    }

    if (casilla == BIFURCACION_MINUTOS)
    {
        return opcion == 0
            ? "SEGUIR LA ESFERA"
            : "POR LA AGUJA DE LOS MINUTOS (ATAJO)";
    }

    if (casilla == CASILLA_EJE)
    {
        return opcion == 0
            ? "BAJAR HACIA LAS 6"
            : "DOBLAR HACIA LAS 9";
    }

    return nullptr;
}


//==================================================
// DEFINICION
//==================================================

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
            "La esfera gira: cada ronda las casillas de color avanzan 1 a 3 pasos.";

        definicion.dificultad = "DIFICIL";

        definicion.colorTema = Color{ 208, 160, 72, 255 };
        definicion.colorFondo = Color{ 22, 28, 52, 255 };

        definicion.camaraPosicion = { 0.0f, 19.0f, 17.5f };
        definicion.camaraObjetivo = { 0.0f, 0.0f, 0.4f };
        definicion.camaraFovy = 48.0f;

        // Landmark: el centro de la esfera (enfoque cuando gira).
        definicion.camaraLandmark = { 0.0f, 0.5f, 0.0f };
        definicion.tieneLandmark = true;

        // Losas de bronce con remaches de laton.
        EstiloCasilla estilo;
        estilo.piedraLado = Color{ 118, 82, 48, 255 };
        estilo.piedraTapa = Color{ 170, 130, 76, 255 };
        estilo.borde = Color{ 64, 44, 30, 255 };
        estilo.detalle = Color{ 214, 178, 96, 255 };
        estilo.sendero = LATON;
        estilo.senderoBorde = BRONCE_OSCURO;
        estilo.anchoSendero = 0.95f;
        definicion.estiloCasilla = estilo;

        definicion.nombresZonas = NOMBRES_ZONAS_RELOJ;
        definicion.cantidadZonas = CANTIDAD_ZONAS_RELOJ;

        definicion.casillasTrofeo = CASILLAS_TROFEO_RELOJ;
        definicion.cantidadCasillasTrofeo =
            (int)(sizeof(CASILLAS_TROFEO_RELOJ) /
                  sizeof(CASILLAS_TROFEO_RELOJ[0]));

        definicion.costoTrofeo = 20;
        definicion.cantidadRondas = 5;

        definicion.Construir = ConstruirTorreReloj;
        definicion.DibujarDecoracion = DibujarDecoracionTorreReloj;
        definicion.AlIniciarRonda = AlIniciarRondaTorreReloj;
        definicion.ResolverCasilla = ResolverCasillaTorreReloj;
        definicion.ActualizarGimmick = ActualizarGimmickTorreReloj;
        definicion.DibujarGimmick = DibujarGimmickTorreReloj;
        definicion.DescargarRecursos = DescargarRecursosReloj;
        definicion.DecidirRutaBot = DecidirRutaBotReloj;
        definicion.DescribirOpcionRuta = DescribirRutaTorreReloj;

        preparada = true;
    }

    return definicion;
}
