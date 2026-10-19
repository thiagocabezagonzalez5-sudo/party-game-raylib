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
// Un mercado sobre canales al atardecer. Un anillo de 28 casillas de
// MUELLE rodea el canal; en el centro, la isla de la PLAZA DEL MERCADO
// (circuito de 8 casillas) con la torre de la campana. Tres bifurcaciones:
//
//   - Casilla 2 (EMBARCADERO): seguir el muelle o cruzar el PUENTE SUR
//     hasta la plaza (atajo).
//   - Casilla 9 (MUELLE DE LAS ESPECIAS): seguir el muelle o subir a la
//     GONDOLA que cruza el canal hasta la plaza (atajo corto).
//   - Plaza norte: seguir en la plaza o salir por el PUENTE NORTE al
//     muelle de las flores.
//
// Ninguna bifurcacion bloquea: el muelle es un circuito y la plaza siempre
// desemboca en el muelle norte.
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
// PRESENTACION (solo primitivas): agua animada, malecon de piedra con
// casas de colores, muelles de tablones con pilotes bajo el camino,
// puentes con barandas, boyas de la gondola, barcas que se mecen y un
// puesto con toldo junto a cada casilla azul (gris libre, color del
// dueno si esta ocupado). Lo estatico se hornea en una MallaGpu.
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

// Tapa de los tablones del muelle (la losa empieza en y = 0).
static const float ALTURA_MUELLE = 0.10f;
static const float ANCHO_MUELLE = 1.20f;

static const float RADIO_PLAZA = 3.0f;
static const float RADIO_ISLA_PLAZA = 4.6f;

// Anillo de muelles y borde interior del malecon.
static const float MUELLE_X = 10.4f;
static const float MUELLE_Z = 7.6f;
static const float MALECON_X = 11.4f;
static const float MALECON_Z = 8.6f;

// Distancia del puesto al centro de su casilla (fuera de la losa).
static const float DISTANCIA_PUESTO = 1.45f;

// La campana suena unos segundos al empezar la ronda y en su evento.
static const float SEGUNDOS_CAMPANA = 3.2f;
static const float VELOCIDAD_ANIMACION_CAMPANA = 2.5f;

static const int CASILLAS_MUELLE = 28;


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
    ZONA_CANAL_GONDOLAS,
    ZONA_PUENTES_CANAL,
    CANTIDAD_ZONAS_MERCADO
};

static const char* const NOMBRES_ZONAS_MERCADO[CANTIDAD_ZONAS_MERCADO] =
{
    "EMBARCADERO",
    "MUELLE DE LAS ESPECIAS",
    "MUELLE DE LAS FLORES",
    "MUELLE DE LOS PESCADORES",
    "PLAZA DEL MERCADO",
    "CANAL DE LAS GÓNDOLAS",
    "PUENTES DEL CANAL"
};

static const int MAX_TROFEOS_MERCADO = 16;

// Datos que acompanan a las casillas (trofeo, textos de ruta, puestos).
static int casillasTrofeoMercado[MAX_TROFEOS_MERCADO];
static int cantidadTrofeosMercado = 0;

static const char* descripcionRutaMercado[MAX_CASILLAS_TABLERO][2];

// Puesto de cada casilla azul: posicion (al costado de la losa) y giro en Y
// para que el mostrador mire a la casilla.
static Vector3 posicionPuestoMercado[MAX_CASILLAS_TABLERO];
static float giroPuestoMercado[MAX_CASILLAS_TABLERO];

// Casillas clave que el dibujo necesita conocer.
static int casillaPuenteSurMercado = -1;
static int casillaPuenteNorteMercado = -1;
static int casillaGondolaAMercado = -1;
static int casillaGondolaBMercado = -1;
static int casillaMuelleGondolaMercado = -1;
static int casillaPlazaGondolaMercado = -1;

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
    if (i <= 6)
    {
        return { -5.2f + 2.6f * i, MUELLE_Z };
    }

    if (i <= 11)
    {
        return { MUELLE_X, 5.0f - 2.5f * (i - 7) };
    }

    if (i <= 20)
    {
        return { MUELLE_X - 2.6f * (i - 12), -MUELLE_Z };
    }

    if (i <= 25)
    {
        return { -MUELLE_X, -5.0f + 2.5f * (i - 21) };
    }

    return { -MUELLE_X + 2.6f * (i - 26), MUELLE_Z };
}


static ZonaMercado ZonaMuelleMercado(int i)
{
    if (i <= 3 || i >= 26)
    {
        return ZONA_EMBARCADERO;
    }

    if (i <= 11)
    {
        return ZONA_MUELLE_ESPECIAS;
    }

    if (i <= 19)
    {
        return ZONA_MUELLE_FLORES;
    }

    return ZONA_MUELLE_PESCADORES;
}


// Elige a que lado de cada casilla azul va su puesto: el lugar con mas
// espacio libre respecto de muelles, casillas, otros puestos y la torre de
// la campana. Evita el lado de la camara (+Z) para no tapar fichas y
// prefiere, a igualdad, el lado de afuera del tablero.
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

        float mejorPuntaje = -1.0e9f;
        float mejorX = casilla.posicion.x;
        float mejorZ = casilla.posicion.z - DISTANCIA_PUESTO;

        for (int d = 0; d < 8; d++)
        {
            float angulo = d * 45.0f * DEG2RAD;
            float dirX = std::cos(angulo);
            float dirZ = std::sin(angulo);

            float px = casilla.posicion.x + dirX * DISTANCIA_PUESTO;
            float pz = casilla.posicion.z + dirZ * DISTANCIA_PUESTO;

            float puntaje = 1.0e9f;

            // Muelles y senderos (todas las conexiones).
            for (int a = 0; a < t.cantidadCasillas; a++)
            {
                for (int c = 0; c < t.casillas[a].cantidadConexiones; c++)
                {
                    int b = t.casillas[a].conexiones[c].destino;

                    if (b < 0 || b >= t.cantidadCasillas)
                    {
                        continue;
                    }

                    float distancia =
                        DistanciaSegmentoMercado(
                            px, pz,
                            t.casillas[a].posicion,
                            t.casillas[b].posicion
                        ) - 0.75f;

                    if (distancia < puntaje) puntaje = distancia;
                }
            }

            // Otras losas.
            for (int a = 0; a < t.cantidadCasillas; a++)
            {
                if (a == i)
                {
                    continue;
                }

                float dx = t.casillas[a].posicion.x - px;
                float dz = t.casillas[a].posicion.z - pz;
                float distancia = std::sqrt(dx * dx + dz * dz) - 1.25f;

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
                float distancia = std::sqrt(dx * dx + dz * dz) - 1.3f;

                if (distancia < puntaje) puntaje = distancia;
            }

            // Torre de la campana (centro de la plaza).
            float distanciaTorre = std::sqrt(px * px + pz * pz) - 2.0f;

            if (distanciaTorre < puntaje) puntaje = distanciaTorre;

            // Hacia la camara taparia las fichas.
            if (dirZ > 0.5f)
            {
                puntaje -= 1.0f;
            }

            // A igualdad, hacia afuera del tablero.
            if (dirX * casilla.posicion.x + dirZ * casilla.posicion.z > 0.0f)
            {
                puntaje += 0.05f;
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

    // Letras: + verde, - roja, n azul (puesto), E especial, R regalo,
    // C caos, I intercambio, D duelo, T tienda, V evento (campana).

    // --- Anillo de muelles: 0..27 (0 = embarcadero, salida) ---
    const char* tiposMuelle = "+nRn-n+nTn-n+nIn-n+nVn-nIn+n";

    for (int i = 0; i < CASILLAS_MUELLE; i++)
    {
        Vector2 p = PosicionMuelleMercado(i);
        NodoMercado(t, ZonaMuelleMercado(i), tiposMuelle[i], p.x, p.y);
    }

    // --- Plaza del mercado: circuito de 8 casillas (28..35) ---
    // 0 = sur (llega el puente sur), 2 = este, 3 = noreste (llega la
    // gondola), 4 = norte (sale el puente norte).
    const char tiposPlaza[8] = { 'T', 'n', '+', 'V', 'R', 'n', '+', '-' };
    int plaza[8];

    for (int k = 0; k < 8; k++)
    {
        float angulo = k * 45.0f * DEG2RAD;

        plaza[k] =
            NodoMercado(
                t, ZONA_PLAZA_MERCADO, tiposPlaza[k],
                std::sin(angulo) * RADIO_PLAZA,
                std::cos(angulo) * RADIO_PLAZA
            );
    }

    // --- Puentes sobre el canal (36, 37) ---
    int puenteSur = NodoMercado(t, ZONA_PUENTES_CANAL, 'D', 0.0f, 5.3f);
    int puenteNorte = NodoMercado(t, ZONA_PUENTES_CANAL, '+', 0.0f, -5.3f);

    // --- Gondola: dos casillas sobre el canal del este (38, 39) ---
    const int bifurcacionMuelle = 2;
    const int bifurcacionGondola = 9;
    const int llegadaPuenteNorte = 16;

    Vector3 muelleGondola = t.casillas[bifurcacionGondola].posicion;
    Vector3 plazaGondola = t.casillas[plaza[3]].posicion;

    int gondolaA =
        NodoMercado(
            t, ZONA_CANAL_GONDOLAS, 'C',
            muelleGondola.x + (plazaGondola.x - muelleGondola.x) / 3.0f,
            muelleGondola.z + (plazaGondola.z - muelleGondola.z) / 3.0f
        );

    int gondolaB =
        NodoMercado(
            t, ZONA_CANAL_GONDOLAS, '-',
            muelleGondola.x + (plazaGondola.x - muelleGondola.x) * 2.0f / 3.0f,
            muelleGondola.z + (plazaGondola.z - muelleGondola.z) * 2.0f / 3.0f
        );

    // --- Conexiones (la opcion 0 se conecta primero) ---

    // El anillo de muelles es un circuito cerrado.
    for (int i = 0; i < CASILLAS_MUELLE; i++)
    {
        t.ConectarCasillas(i, (i + 1) % CASILLAS_MUELLE);
    }

    // Puente sur: embarcadero -> plaza.
    t.ConectarCasillas(bifurcacionMuelle, puenteSur);
    t.ConectarCasillas(puenteSur, plaza[0]);

    // Circuito de la plaza; en el norte se puede salir por el puente.
    for (int k = 0; k < 8; k++)
    {
        t.ConectarCasillas(plaza[k], plaza[(k + 1) % 8]);
    }

    t.ConectarCasillas(plaza[4], puenteNorte);
    t.ConectarCasillas(puenteNorte, llegadaPuenteNorte);

    // Gondola: muelle de las especias -> plaza (noreste).
    t.ConectarCasillas(bifurcacionGondola, gondolaA);
    t.ConectarCasillas(gondolaA, gondolaB);
    t.ConectarCasillas(gondolaB, plaza[3]);

    // --- Textos de las bifurcaciones ---
    DefinirOpcionesMercado(bifurcacionMuelle, "SEGUIR EL MUELLE", "PUENTE A LA PLAZA");
    DefinirOpcionesMercado(bifurcacionGondola, "SEGUIR EL MUELLE", "GÓNDOLA A LA PLAZA");
    DefinirOpcionesMercado(plaza[4], "SEGUIR EN LA PLAZA", "PUENTE AL MUELLE NORTE");

    // --- Candidatas del trofeo: repartidas por todas las zonas ---
    const int candidatas[] =
    {
        3, 6, 11, 14, 18, 21, 25, plaza[2], plaza[5], plaza[7]
    };

    for (int indice : candidatas)
    {
        if (cantidadTrofeosMercado < MAX_TROFEOS_MERCADO)
        {
            casillasTrofeoMercado[cantidadTrofeosMercado++] = indice;
        }
    }

    casillaPuenteSurMercado = puenteSur;
    casillaPuenteNorteMercado = puenteNorte;
    casillaGondolaAMercado = gondolaA;
    casillaGondolaBMercado = gondolaB;
    casillaMuelleGondolaMercado = bifurcacionGondola;
    casillaPlazaGondolaMercado = plaza[3];

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
// ESCENARIO ESTATICO (malla horneada)
//==================================================

static const Color MADERA_MERCADO = Color{ 150, 104, 66, 255 };
static const Color MADERA_CLARA_MERCADO = Color{ 184, 138, 90, 255 };
static const Color MADERA_OSCURA_MERCADO = Color{ 96, 64, 42, 255 };
static const Color PIEDRA_MALECON = Color{ 182, 164, 142, 255 };
static const Color PIEDRA_OSCURA_MERCADO = Color{ 122, 108, 96, 255 };
static const Color BALDOSA_PLAZA = Color{ 212, 190, 156, 255 };
static const Color AGUA_MERCADO = Color{ 46, 146, 164, 255 };

static ConstructorMalla constructorMercado;
static MallaGpu mallaMercado;
static int nivelMallaMercado = -1;


static bool EsCasillaGondolaMercado(int indice)
{
    return
        indice == casillaGondolaAMercado ||
        indice == casillaGondolaBMercado;
}


static bool EsCasillaPuenteMercado(int indice)
{
    return
        indice == casillaPuenteSurMercado ||
        indice == casillaPuenteNorteMercado;
}


static void AgregarPiloteMercado(
    ConstructorMalla& m,
    float x,
    float z
)
{
    m.Cilindro(
        Vector3{ x, NIVEL_AGUA_MERCADO - 0.45f, z },
        Vector3{ x, ALTURA_MUELLE - 0.06f, z },
        0.09f, 0.08f, 6,
        MADERA_OSCURA_MERCADO
    );
}


// Tramo de muelle de tablones entre dos casillas (orientado con giroY).
static void AgregarTramoMuelleMercado(
    ConstructorMalla& m,
    Vector3 a,
    Vector3 b,
    bool barandas,
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

    TransformacionMalla tr;
    tr.tx = (a.x + b.x) * 0.5f;
    tr.tz = (a.z + b.z) * 0.5f;
    tr.giroY = std::atan2(dx, dz) * RAD2DEG;

    m.EstablecerTransformacion(tr);

    // Tablones cruzados (o una sola tabla en calidad baja).
    if (detalle)
    {
        int tablones = (int)(largo / 0.30f);

        if (tablones < 1) tablones = 1;

        float paso = largo / tablones;

        for (int k = 0; k < tablones; k++)
        {
            float z = -largo * 0.5f + (k + 0.5f) * paso;

            m.Caja(
                Vector3{ 0.0f, ALTURA_MUELLE - 0.05f, z },
                Vector3{ ANCHO_MUELLE, 0.10f, paso * 0.86f },
                (k % 2 == 0) ? MADERA_MERCADO : MADERA_CLARA_MERCADO
            );
        }
    }
    else
    {
        m.Caja(
            Vector3{ 0.0f, ALTURA_MUELLE - 0.05f, 0.0f },
            Vector3{ ANCHO_MUELLE, 0.10f, largo },
            MADERA_MERCADO
        );
    }

    // Vigas laterales bajo los tablones.
    for (int lado = -1; lado <= 1; lado += 2)
    {
        m.Caja(
            Vector3{ lado * ANCHO_MUELLE * 0.5f, ALTURA_MUELLE - 0.13f, 0.0f },
            Vector3{ 0.10f, 0.12f, largo },
            MADERA_OSCURA_MERCADO
        );
    }

    // Pilotes en el centro del tramo.
    AgregarPiloteMercado(m, -ANCHO_MUELLE * 0.5f, 0.0f);
    AgregarPiloteMercado(m, ANCHO_MUELLE * 0.5f, 0.0f);

    // Puentes: barandas con postes (fuera de la zona de las losas).
    if (barandas)
    {
        float tramo = largo - 1.9f;

        if (tramo > 0.2f)
        {
            for (int lado = -1; lado <= 1; lado += 2)
            {
                float x = lado * (ANCHO_MUELLE * 0.5f - 0.04f);

                m.Caja(
                    Vector3{ x, ALTURA_MUELLE + 0.52f, 0.0f },
                    Vector3{ 0.07f, 0.07f, tramo },
                    MADERA_CLARA_MERCADO
                );

                int postes = 1 + (int)(tramo / 0.7f);

                for (int k = 0; k <= postes; k++)
                {
                    float z = -tramo * 0.5f + tramo * k / postes;

                    m.Caja(
                        Vector3{ x, ALTURA_MUELLE + 0.26f, z },
                        Vector3{ 0.08f, 0.52f, 0.08f },
                        MADERA_OSCURA_MERCADO
                    );
                }
            }
        }
    }

    m.QuitarTransformacion();
}


// Sendero de baldosas sobre la isla de la plaza.
static void AgregarSenderoPlazaMercado(
    ConstructorMalla& m,
    Vector3 a,
    Vector3 b
)
{
    float dx = b.x - a.x;
    float dz = b.z - a.z;
    float largo = std::sqrt(dx * dx + dz * dz);

    TransformacionMalla tr;
    tr.tx = (a.x + b.x) * 0.5f;
    tr.tz = (a.z + b.z) * 0.5f;
    tr.giroY = std::atan2(dx, dz) * RAD2DEG;

    m.EstablecerTransformacion(tr);

    m.Caja(
        Vector3{ 0.0f, 0.05f, 0.0f },
        Vector3{ 1.25f, 0.04f, largo },
        PIEDRA_OSCURA_MERCADO
    );

    m.Caja(
        Vector3{ 0.0f, 0.06f, 0.0f },
        Vector3{ 1.05f, 0.04f, largo },
        BALDOSA_PLAZA
    );

    m.QuitarTransformacion();
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
        float ox = nx * 0.85f * lado;
        float oz = nz * 0.85f * lado;

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
                Vector3
                {
                    a.x + dx * t + ox,
                    NIVEL_AGUA_MERCADO + 0.06f,
                    a.z + dz * t + oz
                },
                0.11f, 6, 8,
                (k % 2 == 0)
                    ? Color{ 226, 64, 52, 255 }
                    : Color{ 244, 240, 230, 255 }
            );
        }
    }
}


// Plataforma de madera bajo una casilla de muelle o de puente.
static void AgregarPlataformaCasillaMercado(
    ConstructorMalla& m,
    const Casilla& casilla,
    bool barandas
)
{
    float x = casilla.posicion.x;
    float z = casilla.posicion.z;

    m.Caja(
        Vector3{ x, ALTURA_MUELLE - 0.06f, z },
        Vector3{ 2.0f, 0.12f, 2.0f },
        MADERA_MERCADO
    );

    m.Caja(
        Vector3{ x, ALTURA_MUELLE - 0.15f, z },
        Vector3{ 2.1f, 0.10f, 2.1f },
        MADERA_OSCURA_MERCADO
    );

    for (int sx = -1; sx <= 1; sx += 2)
    {
        for (int sz = -1; sz <= 1; sz += 2)
        {
            AgregarPiloteMercado(m, x + sx * 0.92f, z + sz * 0.92f);
        }
    }

    // Puentes: barandas a los costados (los puentes van de norte a sur).
    if (barandas)
    {
        for (int lado = -1; lado <= 1; lado += 2)
        {
            float bx = x + lado * 0.96f;

            m.Caja(
                Vector3{ bx, ALTURA_MUELLE + 0.52f, z },
                Vector3{ 0.07f, 0.07f, 2.0f },
                MADERA_CLARA_MERCADO
            );

            for (int k = 0; k < 4; k++)
            {
                m.Caja(
                    Vector3{ bx, ALTURA_MUELLE + 0.26f, z - 0.96f + k * 0.64f },
                    Vector3{ 0.08f, 0.52f, 0.08f },
                    MADERA_OSCURA_MERCADO
                );
            }
        }
    }
}


// Parte fija de un puesto: balsa o tarima, postes, mostrador y mercaderia.
// El toldo (color del dueno) se dibuja aparte en el gimmick.
static void AgregarPuestoEstaticoMercado(
    ConstructorMalla& m,
    int indice,
    bool detalle
)
{
    Vector3 p = posicionPuestoMercado[indice];

    float radioCentro = std::sqrt(p.x * p.x + p.z * p.z);

    bool enAgua =
        std::fabs(p.x) < MALECON_X &&
        std::fabs(p.z) < MALECON_Z &&
        radioCentro > RADIO_ISLA_PLAZA;

    TransformacionMalla tr;
    tr.tx = p.x;
    tr.tz = p.z;
    tr.giroY = giroPuestoMercado[indice];

    m.EstablecerTransformacion(tr);

    // Los puestos sobre el canal flotan sobre troncos.
    if (enAgua)
    {
        for (int k = -1; k <= 1; k++)
        {
            m.Cilindro(
                Vector3{ -0.62f, NIVEL_AGUA_MERCADO + 0.02f, k * 0.32f },
                Vector3{ 0.62f, NIVEL_AGUA_MERCADO + 0.02f, k * 0.32f },
                0.15f, 0.15f, 6,
                MADERA_OSCURA_MERCADO
            );
        }
    }

    float base = enAgua ? NIVEL_AGUA_MERCADO + 0.17f : 0.02f;

    m.Caja(
        Vector3{ 0.0f, base + 0.05f, 0.0f },
        Vector3{ 1.20f, 0.10f, 0.96f },
        MADERA_CLARA_MERCADO
    );

    // Postes: los de atras mas altos (el toldo cae hacia el frente).
    for (int sx = -1; sx <= 1; sx += 2)
    {
        for (int sz = -1; sz <= 1; sz += 2)
        {
            float alto = (sz < 0 ? 1.36f : 1.19f) - base;

            m.Caja(
                Vector3{ sx * 0.52f, base + alto * 0.5f, sz * 0.38f },
                Vector3{ 0.07f, alto, 0.07f },
                MADERA_OSCURA_MERCADO
            );
        }
    }

    // Mostrador al frente.
    m.Caja(
        Vector3{ 0.0f, base + 0.34f, 0.30f },
        Vector3{ 0.96f, 0.48f, 0.26f },
        MADERA_MERCADO
    );

    m.Caja(
        Vector3{ 0.0f, base + 0.60f, 0.30f },
        Vector3{ 1.02f, 0.05f, 0.32f },
        MADERA_CLARA_MERCADO
    );

    if (detalle)
    {
        // Mercaderia: frutas, especias o flores segun el puesto.
        const Color colores[] =
        {
            Color{ 236, 96, 48, 255 },
            Color{ 246, 196, 54, 255 },
            Color{ 120, 186, 70, 255 },
            Color{ 196, 64, 120, 255 },
            Color{ 236, 140, 40, 255 }
        };

        for (int k = 0; k < 3; k++)
        {
            int c = (int)(Hash2Mercado(indice, k) * 4.99f);

            m.Esfera(
                Vector3{ -0.30f + k * 0.30f, base + 0.71f, 0.30f },
                0.10f, 6, 8,
                colores[c]
            );
        }

        // Cajon y canasto al fondo.
        m.Caja(
            Vector3{ -0.25f, base + 0.22f, -0.22f },
            Vector3{ 0.36f, 0.34f, 0.30f },
            MADERA_OSCURA_MERCADO
        );

        m.Cilindro(
            Vector3{ 0.28f, base + 0.05f, -0.20f },
            Vector3{ 0.28f, base + 0.32f, -0.20f },
            0.15f, 0.18f, 8,
            Color{ 196, 160, 96, 255 }
        );
    }

    m.QuitarTransformacion();
}


// Casa de colores del malecon (el frente, +Z local, mira al canal).
static void AgregarCasaMercado(
    ConstructorMalla& m,
    float cx,
    float cz,
    float giro,
    float ancho,
    float fondo,
    float alto,
    Color pared,
    Color techo,
    bool detalle
)
{
    TransformacionMalla tr;
    tr.tx = cx;
    tr.ty = 0.02f;
    tr.tz = cz;
    tr.giroY = giro;

    m.EstablecerTransformacion(tr);

    m.Caja(
        Vector3{ 0.0f, alto * 0.5f, 0.0f },
        Vector3{ ancho, alto, fondo },
        pared
    );

    m.Caja(
        Vector3{ 0.0f, 0.15f, 0.0f },
        Vector3{ ancho + 0.06f, 0.30f, fondo + 0.06f },
        OscurecerMercado(pared, 0.7f)
    );

    // Techo a dos aguas (cumbrera paralela a X).
    float y0 = alto;
    float y1 = alto + 0.85f;
    float x0 = -ancho * 0.5f - 0.12f;
    float x1 = ancho * 0.5f + 0.12f;
    float z0 = fondo * 0.5f + 0.18f;

    Color techoOscuro = OscurecerMercado(techo, 0.78f);

    m.TrianguloDobleCara({ x0, y0, z0 }, { x1, y0, z0 }, { x1, y1, 0.0f }, techo);
    m.TrianguloDobleCara({ x0, y0, z0 }, { x1, y1, 0.0f }, { x0, y1, 0.0f }, techo);
    m.TrianguloDobleCara({ x1, y0, -z0 }, { x0, y0, -z0 }, { x0, y1, 0.0f }, techoOscuro);
    m.TrianguloDobleCara({ x1, y0, -z0 }, { x0, y1, 0.0f }, { x1, y1, 0.0f }, techoOscuro);

    // Hastiales.
    for (int lado = -1; lado <= 1; lado += 2)
    {
        float x = lado * ancho * 0.5f;

        m.TrianguloDobleCara(
            { x, y0, fondo * 0.5f },
            { x, y0, -fondo * 0.5f },
            { x, y1 - 0.06f, 0.0f },
            pared
        );
    }

    if (detalle)
    {
        const Color ventana = Color{ 255, 214, 140, 255 };
        const Color marco = Color{ 246, 240, 226, 255 };

        // Puerta y dos ventanas por piso.
        m.Caja(
            Vector3{ 0.0f, 0.55f, fondo * 0.5f + 0.02f },
            Vector3{ 0.50f, 0.90f, 0.04f },
            MADERA_OSCURA_MERCADO
        );

        for (float y = 1.4f; y < alto - 0.3f; y += 1.1f)
        {
            for (int lado = -1; lado <= 1; lado += 2)
            {
                float x = lado * ancho * 0.26f;

                m.Caja(
                    Vector3{ x, y, fondo * 0.5f + 0.02f },
                    Vector3{ 0.50f, 0.58f, 0.04f },
                    marco
                );

                m.Caja(
                    Vector3{ x, y, fondo * 0.5f + 0.04f },
                    Vector3{ 0.38f, 0.46f, 0.04f },
                    ventana
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
    float z
)
{
    m.Cilindro(
        Vector3{ x, y, z },
        Vector3{ x, y + 1.30f, z },
        0.05f, 0.04f, 6,
        Color{ 52, 48, 54, 255 }
    );

    m.Caja(
        Vector3{ x, y + 1.42f, z },
        Vector3{ 0.22f, 0.24f, 0.22f },
        Color{ 255, 206, 120, 255 }
    );

    m.Caja(
        Vector3{ x, y + 1.57f, z },
        Vector3{ 0.28f, 0.06f, 0.28f },
        Color{ 52, 48, 54, 255 }
    );
}


// Torre de la campana en el centro de la plaza (landmark).
static void AgregarTorreCampanaMercado(
    ConstructorMalla& m
)
{
    const Color piedra = Color{ 224, 206, 176, 255 };
    const Color tejado = Color{ 196, 82, 58, 255 };

    m.Caja(Vector3{ 0.0f, 0.17f, 0.0f }, Vector3{ 1.8f, 0.30f, 1.8f }, PIEDRA_OSCURA_MERCADO);

    for (int sx = -1; sx <= 1; sx += 2)
    {
        for (int sz = -1; sz <= 1; sz += 2)
        {
            m.Caja(
                Vector3{ sx * 0.62f, 1.52f, sz * 0.62f },
                Vector3{ 0.20f, 2.40f, 0.20f },
                piedra
            );
        }
    }

    m.Caja(Vector3{ 0.0f, 2.82f, 0.0f }, Vector3{ 1.6f, 0.20f, 1.6f }, piedra);

    // Tejado piramidal y remate dorado.
    m.Cilindro(
        Vector3{ 0.0f, 2.92f, 0.0f },
        Vector3{ 0.0f, 3.90f, 0.0f },
        1.12f, 0.0f, 4,
        tejado
    );

    m.Esfera(Vector3{ 0.0f, 3.98f, 0.0f }, 0.12f, 6, 8, Color{ 246, 200, 70, 255 });
}


static void ConstruirMallaMercado(
    NivelCalidadGrafica nivel
)
{
    ConstructorMalla& m = constructorMercado;
    const Tablero& t = TableroReferenciaMercado();

    m.Limpiar();

    m.reduccionDetalle =
        nivel == CALIDAD_ALTA
        ? 0
        : (nivel == CALIDAD_MEDIA ? 1 : 2);

    bool detalle = nivel != CALIDAD_BAJA;

    // --- Malecon de piedra alrededor del canal ---
    const float bordeX = 17.0f;
    const float bordeZ = 15.0f;

    m.Caja(
        Vector3{ 0.0f, -0.20f, -(MALECON_Z + bordeZ) * 0.5f },
        Vector3{ bordeX * 2.0f, 0.44f, bordeZ - MALECON_Z },
        PIEDRA_MALECON
    );

    m.Caja(
        Vector3{ 0.0f, -0.20f, (MALECON_Z + 13.0f) * 0.5f },
        Vector3{ bordeX * 2.0f, 0.44f, 13.0f - MALECON_Z },
        PIEDRA_MALECON
    );

    for (int lado = -1; lado <= 1; lado += 2)
    {
        m.Caja(
            Vector3{ lado * (MALECON_X + bordeX) * 0.5f, -0.20f, 0.0f },
            Vector3{ bordeX - MALECON_X, 0.44f, MALECON_Z * 2.0f },
            PIEDRA_MALECON
        );
    }

    // Cordon del borde del canal.
    m.Caja(Vector3{ 0.0f, 0.04f, -MALECON_Z - 0.15f }, Vector3{ MALECON_X * 2.0f + 0.6f, 0.08f, 0.30f }, PIEDRA_OSCURA_MERCADO);
    m.Caja(Vector3{ 0.0f, 0.04f, MALECON_Z + 0.15f }, Vector3{ MALECON_X * 2.0f + 0.6f, 0.08f, 0.30f }, PIEDRA_OSCURA_MERCADO);
    m.Caja(Vector3{ -MALECON_X - 0.15f, 0.04f, 0.0f }, Vector3{ 0.30f, 0.08f, MALECON_Z * 2.0f }, PIEDRA_OSCURA_MERCADO);
    m.Caja(Vector3{ MALECON_X + 0.15f, 0.04f, 0.0f }, Vector3{ 0.30f, 0.08f, MALECON_Z * 2.0f }, PIEDRA_OSCURA_MERCADO);

    // --- Isla de la plaza ---
    m.Cilindro(
        Vector3{ 0.0f, NIVEL_AGUA_MERCADO - 0.4f, 0.0f },
        Vector3{ 0.0f, 0.02f, 0.0f },
        RADIO_ISLA_PLAZA + 0.15f, RADIO_ISLA_PLAZA, 28,
        PIEDRA_OSCURA_MERCADO
    );

    m.Cilindro(
        Vector3{ 0.0f, 0.02f, 0.0f },
        Vector3{ 0.0f, 0.04f, 0.0f },
        RADIO_ISLA_PLAZA - 0.25f, RADIO_ISLA_PLAZA - 0.25f, 28,
        Color{ 200, 178, 146, 255 }
    );

    AgregarTorreCampanaMercado(m);

    // --- Camino fisico entre casillas conectadas ---
    for (int a = 0; a < t.cantidadCasillas; a++)
    {
        const Casilla& origen = t.casillas[a];

        for (int c = 0; c < origen.cantidadConexiones; c++)
        {
            int b = origen.conexiones[c].destino;

            if (b < 0 || b >= t.cantidadCasillas)
            {
                continue;
            }

            const Casilla& destino = t.casillas[b];

            if (EsCasillaGondolaMercado(a) || EsCasillaGondolaMercado(b))
            {
                AgregarBoyasGondolaMercado(m, origen.posicion, destino.posicion, detalle);
            }
            else if (
                origen.zona == ZONA_PLAZA_MERCADO &&
                destino.zona == ZONA_PLAZA_MERCADO
            )
            {
                AgregarSenderoPlazaMercado(m, origen.posicion, destino.posicion);
            }
            else
            {
                AgregarTramoMuelleMercado(
                    m,
                    origen.posicion,
                    destino.posicion,
                    EsCasillaPuenteMercado(a) || EsCasillaPuenteMercado(b),
                    detalle
                );
            }
        }
    }

    // Plataformas bajo las losas del muelle y de los puentes.
    for (int i = 0; i < t.cantidadCasillas; i++)
    {
        const Casilla& casilla = t.casillas[i];

        if (
            casilla.zona == ZONA_PLAZA_MERCADO ||
            EsCasillaGondolaMercado(i)
        )
        {
            continue;
        }

        AgregarPlataformaCasillaMercado(m, casilla, EsCasillaPuenteMercado(i));
    }

    // --- Puestos (parte fija) ---
    for (int i = 0; i < t.cantidadCasillas; i++)
    {
        if (t.casillas[i].tipo == CASILLA_NEUTRA)
        {
            AgregarPuestoEstaticoMercado(m, i, detalle);
        }
    }

    // --- Faroles: esquinas de las barandas de los puentes y orilla de la
    // plaza (solo donde hay lugar) ---
    const int puentes[2] = { casillaPuenteSurMercado, casillaPuenteNorteMercado };

    for (int indice : puentes)
    {
        const Casilla* puente = t.ObtenerCasilla(indice);

        if (puente == nullptr)
        {
            continue;
        }

        for (int sx = -1; sx <= 1; sx += 2)
        {
            for (int sz = -1; sz <= 1; sz += 2)
            {
                AgregarFarolMercado(
                    m,
                    puente->posicion.x + sx * 0.96f,
                    ALTURA_MUELLE,
                    puente->posicion.z + sz * 0.96f
                );
            }
        }
    }

    if (detalle)
    {
        for (int k = 0; k < 8; k++)
        {
            float angulo = (22.5f + k * 45.0f) * DEG2RAD;
            float x = std::sin(angulo) * (RADIO_ISLA_PLAZA - 0.45f);
            float z = std::cos(angulo) * (RADIO_ISLA_PLAZA - 0.45f);

            float libre = 1.0e9f;

            for (int a = 0; a < t.cantidadCasillas; a++)
            {
                for (int c = 0; c < t.casillas[a].cantidadConexiones; c++)
                {
                    int b = t.casillas[a].conexiones[c].destino;

                    float d =
                        DistanciaSegmentoMercado(
                            x, z, t.casillas[a].posicion, t.casillas[b].posicion
                        );

                    if (d < libre) libre = d;
                }

                if (t.casillas[a].tipo == CASILLA_NEUTRA)
                {
                    float px = posicionPuestoMercado[a].x - x;
                    float pz = posicionPuestoMercado[a].z - z;
                    float d = std::sqrt(px * px + pz * pz) - 0.5f;

                    if (d < libre) libre = d;
                }
            }

            if (libre > 1.05f)
            {
                AgregarFarolMercado(m, x, 0.04f, z);
            }
        }
    }

    // --- Casas de colores sobre el malecon ---
    const Color paredes[] =
    {
        Color{ 236, 178, 120, 255 },
        Color{ 222, 120, 104, 255 },
        Color{ 246, 214, 140, 255 },
        Color{ 132, 190, 186, 255 },
        Color{ 236, 156, 170, 255 },
        Color{ 168, 196, 120, 255 }
    };

    const Color techos[] =
    {
        Color{ 176, 74, 52, 255 },
        Color{ 120, 70, 60, 255 },
        Color{ 64, 104, 120, 255 }
    };

    int paso = detalle ? 1 : 2;

    // Fila norte (frente al canal, hacia +Z).
    for (int k = 0; k < 13; k += paso)
    {
        float x = -15.6f + 2.6f * k;
        float alto = 2.0f + 1.4f * Hash2Mercado(k, 7);

        AgregarCasaMercado(
            m, x, -11.9f, 0.0f, 2.4f, 2.6f, alto,
            paredes[k % 6], techos[k % 3], detalle
        );
    }

    // Columnas este y oeste (frente hacia el canal).
    for (int lado = -1; lado <= 1; lado += 2)
    {
        for (int k = 0; k < 6; k += paso)
        {
            float z = -6.5f + 2.6f * k;
            float alto = 1.9f + 1.2f * Hash2Mercado(k + lado * 11, 3);

            AgregarCasaMercado(
                m, lado * 14.0f, z, lado < 0 ? 90.0f : -90.0f,
                2.4f, 2.6f, alto,
                paredes[(k + 2 + lado) % 6], techos[(k + 1) % 3], detalle
            );
        }
    }

    // Malecon sur (lado de la camara): bitas, cajones y barriles bajos.
    if (detalle)
    {
        for (int k = 0; k < 9; k++)
        {
            float x = -10.4f + 2.6f * k;

            m.Cilindro(
                Vector3{ x + 1.3f, 0.02f, 9.15f },
                Vector3{ x + 1.3f, 0.32f, 9.15f },
                0.13f, 0.11f, 8,
                Color{ 58, 54, 58, 255 }
            );

            if (k % 3 == 1)
            {
                m.Caja(
                    Vector3{ x, 0.24f, 10.2f },
                    Vector3{ 0.5f, 0.44f, 0.5f },
                    MADERA_MERCADO
                );

                m.Cilindro(
                    Vector3{ x + 0.6f, 0.02f, 10.1f },
                    Vector3{ x + 0.6f, 0.52f, 10.1f },
                    0.20f, 0.20f, 8,
                    MADERA_OSCURA_MERCADO
                );
            }
        }
    }

    ActualizarMallaGpu(mallaMercado, m);
    nivelMallaMercado = (int)nivel;
}


//==================================================
// DECORACION ANIMADA (agua y barcas)
//==================================================

static void DibujarAguaMercado(float tiempo)
{
    // Superficie del canal.
    DrawCube(
        Vector3{ 0.0f, NIVEL_AGUA_MERCADO - 0.05f, 0.0f },
        MALECON_X * 2.0f,
        0.10f,
        MALECON_Z * 2.0f,
        AGUA_MERCADO
    );

    // Reflejos que se deslizan (cielo de atardecer y espuma clara).
    int reflejos = (int)std::ceil(16.0f * FactorCalidadGrafica(CalidadEfectos()));
    float anchoCanal = MALECON_X * 2.0f - 1.6f;

    for (int i = 0; i < reflejos; i++)
    {
        float velocidad = 0.25f + 0.25f * Hash2Mercado(i, 1);
        float z = -MALECON_Z + 0.6f + Hash2Mercado(i, 2) * (MALECON_Z * 2.0f - 1.2f);
        float x =
            std::fmod(Hash2Mercado(i, 3) * anchoCanal + tiempo * velocidad, anchoCanal) -
            anchoCanal * 0.5f;

        float brillo = 0.5f + 0.5f * std::sin(tiempo * 1.7f + i * 1.3f);

        Color color =
            (i % 3 == 0)
            ? Color{ 246, (unsigned char)(170 + (int)(brillo * 40.0f)), 130, 255 }
            : Color{ 130, (unsigned char)(200 + (int)(brillo * 30.0f)), 206, 255 };

        DrawCube(
            Vector3{ x, NIVEL_AGUA_MERCADO + 0.005f, z },
            0.9f + 0.6f * Hash2Mercado(i, 4),
            0.01f,
            0.07f,
            color
        );
    }
}


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
    DrawCube(Vector3{ 0.0f, 0.14f, largo * 0.5f + 0.10f }, ancho * 0.5f, 0.34f, 0.26f, casco);
    DrawCube(Vector3{ 0.0f, 0.14f, -largo * 0.5f - 0.10f }, ancho * 0.5f, 0.34f, 0.26f, casco);

    if (carga)
    {
        DrawCube(Vector3{ 0.08f, 0.28f, -0.30f }, 0.30f, 0.24f, 0.30f, MADERA_OSCURA_MERCADO);
        DrawSphere(Vector3{ -0.12f, 0.26f, 0.22f }, 0.13f, Color{ 236, 120, 48, 255 });
        DrawSphere(Vector3{ 0.12f, 0.24f, 0.36f }, 0.11f, Color{ 246, 200, 60, 255 });
    }

    rlPopMatrix();
}


static void DibujarBarcasMercado(
    const PartidaTablero& partida,
    float tiempo
)
{
    const Tablero& t = partida.tablero;
    const float y = NIVEL_AGUA_MERCADO + 0.10f;

    // Gondolas bajo las dos casillas del canal (la losa va encima).
    const Casilla* muelle = t.ObtenerCasilla(casillaMuelleGondolaMercado);
    const Casilla* plaza = t.ObtenerCasilla(casillaPlazaGondolaMercado);

    if (muelle != nullptr && plaza != nullptr)
    {
        float giro =
            std::atan2(
                plaza->posicion.x - muelle->posicion.x,
                plaza->posicion.z - muelle->posicion.z
            ) * RAD2DEG;

        const int gondolas[2] = { casillaGondolaAMercado, casillaGondolaBMercado };

        for (int k = 0; k < 2; k++)
        {
            const Casilla* casilla = t.ObtenerCasilla(gondolas[k]);

            if (casilla == nullptr)
            {
                continue;
            }

            Vector3 centro = casilla->posicion;
            centro.y = NIVEL_AGUA_MERCADO + 0.12f;

            DibujarBarcaMercado(
                centro, giro, 1.30f, 2.50f,
                Color{ 40, 36, 44, 255 },
                k * 1.7f, tiempo, false
            );
        }
    }

    if (CalidadDecoracion() == CALIDAD_BAJA)
    {
        return;
    }

    // Barcas amarradas con mercaderia.
    DibujarBarcaMercado(Vector3{ 6.8f, y, 3.6f }, 0.0f, 0.80f, 1.80f, Color{ 52, 120, 160, 255 }, 0.3f, tiempo, true);
    DibujarBarcaMercado(Vector3{ -5.8f, y, 5.9f }, 70.0f, 0.80f, 1.70f, Color{ 176, 64, 52, 255 }, 1.9f, tiempo, true);
    DibujarBarcaMercado(Vector3{ 6.0f, y, -4.8f }, 30.0f, 0.80f, 1.80f, Color{ 64, 132, 82, 255 }, 3.1f, tiempo, true);

    // Una barca que recorre el canal del oeste.
    float recorrido = std::sin(tiempo * 0.22f) * 3.6f;

    DibujarBarcaMercado(
        Vector3{ -7.3f, y, recorrido },
        0.0f, 0.80f, 1.90f,
        Color{ 214, 160, 60, 255 },
        0.9f, tiempo, true
    );
}


static void DibujarDecoracionMercado(
    const PartidaTablero& partida
)
{
    float tiempo = partida.tiempoTablero;

    DibujarAguaMercado(tiempo);

    NivelCalidadGrafica nivel = CalidadDecoracion();

    if (!mallaMercado.cargada || nivelMallaMercado != (int)nivel)
    {
        ConstruirMallaMercado(nivel);
    }

    DibujarMallaGpu(mallaMercado);

    DibujarBarcasMercado(partida, tiempo);
}


//==================================================
// GIMMICK (toldos de los puestos y campana)
//==================================================

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


// Toldo a rayas del puesto: gris si esta libre, del color del dueno si no.
static void DibujarToldoPuestoMercado(
    Vector3 posicion,
    float giro,
    bool libre,
    Color dueno,
    float tiempo,
    int semilla
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

    rlPushMatrix();
    rlTranslatef(posicion.x, 0.0f, posicion.z);
    rlRotatef(giro, 0.0f, 1.0f, 0.0f);

    // Lona inclinada: alta atras, baja adelante (+Z local = la casilla).
    rlPushMatrix();
    rlTranslatef(0.0f, 1.30f, 0.0f);
    rlRotatef(14.0f, 1.0f, 0.0f, 0.0f);

    for (int k = 0; k < 4; k++)
    {
        DrawCube(
            Vector3{ -0.45f + 0.30f * k, 0.0f, 0.0f },
            0.30f, 0.05f, 1.02f,
            (k % 2 == 0) ? franja : claro
        );
    }

    rlPopMatrix();

    // Faldon del frente.
    for (int k = 0; k < 4; k++)
    {
        DrawCube(
            Vector3{ -0.45f + 0.30f * k, 1.10f, 0.50f },
            0.30f, 0.16f, 0.03f,
            (k % 2 == 0) ? claro : franja
        );
    }

    // Banderin del dueno.
    if (!libre)
    {
        float flameo = std::sin(tiempo * 4.0f + semilla) * 0.04f;

        DrawCube(Vector3{ 0.52f, 1.62f, -0.38f }, 0.04f, 0.50f, 0.04f, MADERA_OSCURA_MERCADO);
        DrawCube(Vector3{ 0.66f, 1.76f, -0.38f + flameo }, 0.26f, 0.17f, 0.02f, dueno);
    }

    rlPopMatrix();
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
    rlTranslatef(0.0f, 2.66f, 0.0f);
    rlRotatef(vaiven, 1.0f, 0.0f, 0.0f);

    DrawCube(Vector3{ 0.0f, 0.04f, 0.0f }, 0.24f, 0.10f, 0.24f, MADERA_OSCURA_MERCADO);
    DrawCylinder(Vector3{ 0.0f, -0.62f, 0.0f }, 0.14f, 0.40f, 0.60f, 14, bronce);
    DrawCylinder(Vector3{ 0.0f, -0.67f, 0.0f }, 0.42f, 0.42f, 0.05f, 14, bronceOscuro);
    DrawSphere(Vector3{ 0.0f, -0.72f, 0.0f }, 0.09f, MADERA_OSCURA_MERCADO);

    rlPopMatrix();

    // Ondas de sonido mientras suena.
    if (fuerza > 0.05f && CalidadEfectos() != CALIDAD_BAJA)
    {
        for (int k = 0; k < 2; k++)
        {
            float fase = std::fmod(tiempo * 0.9f + k * 0.5f, 1.0f);

            DrawCircle3D(
                Vector3{ 0.0f, 2.3f, 0.0f },
                0.8f + fase * 2.6f,
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

    for (int i = 0; i < t.cantidadCasillas; i++)
    {
        if (t.casillas[i].tipo != CASILLA_NEUTRA)
        {
            continue;
        }

        int dueno = duenoPuesto[i];

        DibujarToldoPuestoMercado(
            posicionPuestoMercado[i],
            giroPuestoMercado[i],
            dueno < 0,
            ColorJugadorMercado(partida, dueno),
            partida.tiempoTablero,
            i
        );
    }

    DibujarCampanaMercado(partida);
}


// Luz calida de atardecer sobre la parte alta de la pantalla.
static void DibujarCapaPantallaMercado(
    const PartidaTablero& partida
)
{
    (void)partida;

    int ancho = GetScreenWidth();
    int alto = GetScreenHeight();

    DrawRectangleGradientV(
        0, 0, ancho, (int)(alto * 0.45f),
        Color{ 255, 170, 110, 46 },
        Color{ 255, 170, 110, 0 }
    );
}


//==================================================
// RECURSOS
//==================================================

static void DescargarRecursosMercado()
{
    DescargarMalla(mallaMercado);
    nivelMallaMercado = -1;
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

        // Mapa mediano (muelles de 21 x 15): sigue a la ficha a medias y
        // da una vista general al empezar la ronda.
        definicion.camaraPosicion = { 0.0f, 17.0f, 17.5f };
        definicion.camaraObjetivo = { 0.0f, 0.0f, 0.5f };
        definicion.camaraFovy = 48.0f;
        definicion.camaraSeguimiento = 0.6f;
        definicion.camaraZoomGeneral = 1.3f;
        definicion.camaraEscalaVistaPrevia = 1.3f;

        // Landmark: la torre de la campana en la plaza.
        definicion.camaraLandmark = { 0.0f, 1.6f, 0.0f };
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
