#include "Minigames/MinijuegoRefugioPinchos.h"

#include "Minigames/AudioMinijuegos.h"
#include "Minigames/EfectosVisualesMinijuegos.h"
#include "Minigames/MecanicasJugador.h"
#include "Minigames/UtilidadesMinijuegos.h"
#include "Systems/CalidadGrafica.h"
#include "Systems/Input.h"

#include "rlgl.h"

#include <cmath>

// SombrasRetro.h estampa manchas de sombra en cada DrawCube/DrawSphere/
// DrawCylinder. Este escenario es una cueva con iluminacion propia (antorchas,
// cristales, luces de aviso): usamos las primitivas puras de raylib y
// dibujamos a mano las sombras que importan (jugadores y coberturas).
#undef DrawCube
#undef DrawSphere
#undef DrawCylinder


//==================================================
// REFUGIO TALADROS (1 vs 3)
//==================================================
//
// La arena es una rejilla de 8 x 7 carriles; cada carril tiene un taladro en
// cada pared. El operador (1) elige por que lado atacan y la maquina compone
// el patron (salva, ola, alterno, rafaga, tenaza, cruce, vaiven). El equipo
// (2-3) debe ponerse a salvo antes del impacto:
//   - detras de una COBERTURA de roca: el taladro se detiene al chocar con
//     ella y todo lo que hay detras, en su carril, queda a salvo.
//   - no existe ningun hueco entre taladros: los carriles se tocan, asi que
//     fuera de la sombra de una cobertura no hay sitio seguro.
// Cada franja roja del suelo ES exactamente la zona letal de un taladro: va
// desde la boca hasta la primera cobertura que se cruza en su carril.
//==================================================

static const float DURACION_PREPARACION_PINCHOS = 3.0f;
static const float DURACION_PARTIDA_PINCHOS = 44.0f;
static const float MUERTE_SUBITA_DESDE_PINCHOS = 24.0f;
static const float AVISO_INICIAL_PINCHOS = 1.15f;
static const float AVISO_FINAL_PINCHOS = 0.65f;
static const float EXTENDER_INICIAL_PINCHOS = 0.34f;
static const float EXTENDER_FINAL_PINCHOS = 0.24f;
static const float TIEMPO_MANTENER_PINCHOS = 0.16f;
static const float TIEMPO_RETRAER_PINCHOS = 0.38f;
static const float COOLDOWN_INICIAL_PINCHOS = 0.90f;
static const float COOLDOWN_FINAL_PINCHOS = 0.30f;
static const float ESPERA_AUTOMATICA_PINCHOS = 3.0f;

// Geometria de la arena. Caras interiores de los muros y zona caminable.
static const float CARRIL_PINCHOS = 1.25f;
static const float PARED_X_PINCHOS = (float)COLUMNAS_PINCHOS * CARRIL_PINCHOS * 0.5f;
static const float PARED_Z_PINCHOS = (float)FILAS_PINCHOS * CARRIL_PINCHOS * 0.5f;
static const float LIMITE_X_JUGADORES_PINCHOS = PARED_X_PINCHOS - 0.42f;
static const float LIMITE_Z_JUGADORES_PINCHOS = PARED_Z_PINCHOS - 0.42f;
static const float CELDA_PINCHOS = 10.0f / (float)CELDAS_X_PINCHOS;

static const float RADIO_TALADRO_PINCHOS = 0.50f;
static const float ALTURA_TALADRO_PINCHOS = 0.62f;
static const float ALTURA_COBERTURA_PINCHOS = 1.30f;
static const float ESCONDITE_RETRAIDO_PINCHOS = -0.30f;

// Criterios de una disposicion de coberturas valida (ver
// EvaluarDisposicionPinchos): refugio holgado desde cada lado y cerca de
// cualquier punto de la arena.
static const int MIN_CELDAS_SEGURAS_PINCHOS = 40;
static const float DISTANCIA_COMODA_PINCHOS = 3.0f;
static const int MIN_CELDAS_COMODAS_PINCHOS = 150;
static const float MAX_DISTANCIA_REFUGIO_PINCHOS = 6.4f;
static const float MAX_FRACCION_SEGURA_PINCHOS = 0.62f;

static const Color COLOR_CIAN_REFUGIO = { 70, 225, 230, 255 };
static const Color COLOR_PELIGRO_PINCHOS = { 255, 70, 40, 255 };

struct CristalAmbientePinchos
{
    float x;
    float y;
    float z;
    float alto;
    Color color;
};

static const CristalAmbientePinchos CRISTALES_AMBIENTE_PINCHOS[4] =
{
    { -6.8f, 2.80f, -4.6f, 1.3f, { 70, 230, 130, 255 } },
    { -8.2f, 0.00f, 1.8f, 1.5f, { 70, 230, 130, 255 } },
    { 6.8f, 2.80f, -4.6f, 1.2f, { 255, 170, 60, 255 } },
    { 8.3f, 0.00f, -1.4f, 1.4f, { 255, 170, 60, 255 } }
};

static const char* NOMBRES_PATRON_PINCHOS[PATRON_TOTAL] =
{
    "SALVA", "OLA", "ALTERNO", "RAFAGA", "TENAZA", "CRUCE", "VAIVEN"
};


//==================================================
// UTILIDADES
//==================================================

static float AzarPinchos(float minimo, float maximo)
{
    return minimo + (maximo - minimo) * (float)GetRandomValue(0, 1000) / 1000.0f;
}


static float LimitarPinchos(float valor, float minimo, float maximo)
{
    if (valor < minimo) return minimo;
    if (valor > maximo) return maximo;
    return valor;
}


static float SuavizarPinchos(float u)
{
    u = LimitarPinchos(u, 0.0f, 1.0f);
    return u * u * (3.0f - 2.0f * u);
}


static float Hash01Pinchos(int a, int b)
{
    unsigned int h = (unsigned int)(a * 374761393 + b * 668265263);
    h = (h ^ (h >> 13)) * 1274126177u;
    h ^= h >> 16;
    return (float)(h & 0xFFFF) / 65535.0f;
}


static float DistanciaCuadradaXZ(Vector3 a, Vector3 b)
{
    float dx = a.x - b.x;
    float dz = a.z - b.z;
    return dx * dx + dz * dz;
}


static bool EsEjeZPinchos(DireccionPinchos direccion)
{
    return direccion == PINCHOS_DESDE_ARRIBA || direccion == PINCHOS_DESDE_ABAJO;
}


static DireccionPinchos LadoOpuestoPinchos(DireccionPinchos d)
{
    return (DireccionPinchos)((int)d ^ 1);
}


static const char* NombreDireccionPinchos(DireccionPinchos direccion)
{
    switch (direccion)
    {
        case PINCHOS_DESDE_ARRIBA: return "ARRIBA";
        case PINCHOS_DESDE_ABAJO: return "ABAJO";
        case PINCHOS_DESDE_IZQUIERDA: return "IZQUIERDA";
        case PINCHOS_DESDE_DERECHA: return "DERECHA";
    }

    return "?";
}


static bool JugadorEnPartida(const MinijuegoRefugioPinchos& m, int i)
{
    return
        i != m.indiceSolo &&
        m.resultado.participantes[i].participo &&
        !m.estadosJugadores[i].eliminado;
}


static int ContarRivalesVivosPinchos(const MinijuegoRefugioPinchos& minijuego)
{
    int vivos = 0;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (JugadorEnPartida(minijuego, i)) vivos++;
    }

    return vivos;
}


// Un participante lo controla la IA si es bot o si es un humano desconectado.
static bool ControlaIAPinchos(const Participante& p)
{
    return p.esBot || !p.conectado;
}


static bool LeerDireccionHumanaPinchos(
    const Participante& participante,
    DireccionPinchos& direccion
)
{
    InputSeleccionParticipante entrada =
        LeerInputSeleccionParticipante(participante);

    if (entrada.arriba) { direccion = PINCHOS_DESDE_ARRIBA; return true; }
    if (entrada.abajo) { direccion = PINCHOS_DESDE_ABAJO; return true; }
    if (entrada.izquierda) { direccion = PINCHOS_DESDE_IZQUIERDA; return true; }
    if (entrada.derecha) { direccion = PINCHOS_DESDE_DERECHA; return true; }

    return false;
}


//==================================================
// LOGICA: CARRILES Y GEOMETRIA DE LOS TALADROS
//==================================================
// MODELO FUTURO: nada de esta seccion depende de la decoracion; los
// taladros, muros y torres pueden pasar a GLB sin tocar estas funciones.

static int CantidadCarrilesPinchos(DireccionPinchos direccion)
{
    return EsEjeZPinchos(direccion) ? COLUMNAS_PINCHOS : FILAS_PINCHOS;
}


static float CentroCarrilPinchos(DireccionPinchos direccion, int carril)
{
    float inicio = EsEjeZPinchos(direccion) ? -PARED_X_PINCHOS : -PARED_Z_PINCHOS;
    return inicio + ((float)carril + 0.5f) * CARRIL_PINCHOS;
}


static unsigned int MascaraCompletaPinchos(DireccionPinchos direccion)
{
    return (1u << CantidadCarrilesPinchos(direccion)) - 1u;
}


static void ObtenerOrigenTaladroPinchos(
    DireccionPinchos direccion,
    int carril,
    Vector3& origen,
    Vector3& avance
)
{
    float c = CentroCarrilPinchos(direccion, carril);

    switch (direccion)
    {
        case PINCHOS_DESDE_ARRIBA:
            origen = { c, ALTURA_TALADRO_PINCHOS, -PARED_Z_PINCHOS };
            avance = { 0.0f, 0.0f, 1.0f };
            break;
        case PINCHOS_DESDE_ABAJO:
            origen = { c, ALTURA_TALADRO_PINCHOS, PARED_Z_PINCHOS };
            avance = { 0.0f, 0.0f, -1.0f };
            break;
        case PINCHOS_DESDE_IZQUIERDA:
            origen = { -PARED_X_PINCHOS, ALTURA_TALADRO_PINCHOS, c };
            avance = { 1.0f, 0.0f, 0.0f };
            break;
        case PINCHOS_DESDE_DERECHA:
            origen = { PARED_X_PINCHOS, ALTURA_TALADRO_PINCHOS, c };
            avance = { -1.0f, 0.0f, 0.0f };
            break;
    }
}


// Carril y distancia (desde la cara del muro del lado) de un punto del suelo.
static bool UbicarEnCarrilPinchos(
    DireccionPinchos direccion,
    float x,
    float z,
    int& carril,
    float& longitudinal
)
{
    float lateral = EsEjeZPinchos(direccion) ? x + PARED_X_PINCHOS : z + PARED_Z_PINCHOS;
    int cantidad = CantidadCarrilesPinchos(direccion);

    carril = (int)std::floor(lateral / CARRIL_PINCHOS);
    if (carril < 0) carril = 0;
    if (carril >= cantidad) carril = cantidad - 1;

    switch (direccion)
    {
        case PINCHOS_DESDE_ARRIBA: longitudinal = z + PARED_Z_PINCHOS; break;
        case PINCHOS_DESDE_ABAJO: longitudinal = PARED_Z_PINCHOS - z; break;
        case PINCHOS_DESDE_IZQUIERDA: longitudinal = x + PARED_X_PINCHOS; break;
        case PINCHOS_DESDE_DERECHA: longitudinal = PARED_X_PINCHOS - x; break;
    }

    return longitudinal >= 0.0f;
}


// Recalcula hasta donde llega cada carril: pared opuesta o cara de la
// primera cobertura cuyo ancho solapa el carril.
static void CalcularAlcancesPinchos(MinijuegoRefugioPinchos& m)
{
    for (int d = 0; d < 4; d++)
    {
        DireccionPinchos direccion = (DireccionPinchos)d;
        bool ejeZ = EsEjeZPinchos(direccion);
        float total = ejeZ ? PARED_Z_PINCHOS * 2.0f : PARED_X_PINCHOS * 2.0f;

        for (int k = 0; k < CantidadCarrilesPinchos(direccion); k++)
        {
            float centro = CentroCarrilPinchos(direccion, k);
            float a0 = centro - CARRIL_PINCHOS * 0.5f;
            float a1 = centro + CARRIL_PINCHOS * 0.5f;
            float alcance = total;
            bool choca = false;

            for (int i = 1; i < m.cantidadBloques; i++)
            {
                const BloquePrueba& b = m.bloques[i];
                float minX = b.posicion.x - b.tamano.x * 0.5f;
                float maxX = b.posicion.x + b.tamano.x * 0.5f;
                float minZ = b.posicion.z - b.tamano.z * 0.5f;
                float maxZ = b.posicion.z + b.tamano.z * 0.5f;
                float l0 = ejeZ ? minX : minZ;
                float l1 = ejeZ ? maxX : maxZ;

                float solape = (a1 < l1 ? a1 : l1) - (a0 > l0 ? a0 : l0);
                if (solape <= 0.02f) continue;

                float distancia = 0.0f;
                switch (direccion)
                {
                    case PINCHOS_DESDE_ARRIBA: distancia = minZ + PARED_Z_PINCHOS; break;
                    case PINCHOS_DESDE_ABAJO: distancia = PARED_Z_PINCHOS - maxZ; break;
                    case PINCHOS_DESDE_IZQUIERDA: distancia = minX + PARED_X_PINCHOS; break;
                    case PINCHOS_DESDE_DERECHA: distancia = PARED_X_PINCHOS - maxX; break;
                }

                if (distancia > 0.0f && distancia < alcance)
                {
                    alcance = distancia;
                    choca = true;
                }
            }

            m.alcance[d][k] = alcance;
            m.chocaCobertura[d][k] = choca;
        }
    }
}


// Punto dentro de la franja letal de un carril (franja completa, sin
// tener en cuenta el instante del ataque).
static bool PuntoEnCarrilLetalPinchos(
    const MinijuegoRefugioPinchos& m,
    DireccionPinchos lado,
    unsigned int mascara,
    float x,
    float z
)
{
    int carril = 0;
    float longitudinal = 0.0f;

    if (!UbicarEnCarrilPinchos(lado, x, z, carril, longitudinal)) return false;
    if ((mascara & (1u << carril)) == 0) return false;

    return longitudinal <= m.alcance[(int)lado][carril];
}


// Ventana temporal de un carril de una salva.
struct EstadoCarrilPinchos
{
    bool activo = false;       // algo que dibujar (aviso o taladro fuera)
    bool aviso = false;        // marcado en el suelo, taladro todavia asomando
    bool letal = false;        // taladro extendiendose o sostenido
    float progresoAviso = 0.0f;
    float fraccion = 0.0f;     // 0 retraido, 1 extension maxima
    float desvanece = 0.0f;    // 0..1 durante la retirada
};


static EstadoCarrilPinchos EstadoCarrilSalvaPinchos(
    const MinijuegoRefugioPinchos& m,
    const SalvaPinchos& s,
    int carril
)
{
    EstadoCarrilPinchos e{};

    if ((s.mascara & (1u << carril)) == 0) return e;

    float t = m.tiempoPatron;
    if (t < s.inicioAviso) return e;

    int cantidad = CantidadCarrilesPinchos(s.lado);
    int orden = s.invertido ? cantidad - 1 - carril : carril;
    float disparo = s.disparo + (float)orden * s.paso;
    float tl = t - disparo;

    if (tl < 0.0f)
    {
        float espera = s.disparo - s.inicioAviso;
        if (espera < 0.01f) espera = 0.01f;

        e.activo = true;
        e.aviso = true;
        e.progresoAviso = LimitarPinchos((t - s.inicioAviso) / espera, 0.0f, 1.0f);
        return e;
    }

    float ext = m.tiempoExtension;

    if (tl < ext)
    {
        e.activo = true;
        e.letal = true;
        e.fraccion = tl / ext;
        return e;
    }

    if (tl < ext + TIEMPO_MANTENER_PINCHOS)
    {
        e.activo = true;
        e.letal = true;
        e.fraccion = 1.0f;
        return e;
    }

    float r = (tl - ext - TIEMPO_MANTENER_PINCHOS) / TIEMPO_RETRAER_PINCHOS;

    if (r < 1.0f)
    {
        e.activo = true;
        e.fraccion = 1.0f - SuavizarPinchos(r);
        e.desvanece = r;
    }

    return e;
}


// Posicion de la punta medida desde la cara del muro (negativo = dentro).
static float PuntaTaladroPinchos(
    const MinijuegoRefugioPinchos& m,
    DireccionPinchos lado,
    int carril,
    float fraccion
)
{
    return ESCONDITE_RETRAIDO_PINCHOS +
        (m.alcance[(int)lado][carril] - ESCONDITE_RETRAIDO_PINCHOS) * fraccion;
}


// Largo con el que se dibuja el taladro de un carril (el mayor de todas las
// salvas que lo usan en este instante).
static float LargoTaladroPinchos(
    const MinijuegoRefugioPinchos& m,
    DireccionPinchos lado,
    int carril
)
{
    float mejor = ESCONDITE_RETRAIDO_PINCHOS;
    if (!m.patronActivo) return mejor;

    for (int i = 0; i < m.patron.cantidad; i++)
    {
        const SalvaPinchos& s = m.patron.salvas[i];
        if (s.lado != lado) continue;

        EstadoCarrilPinchos e = EstadoCarrilSalvaPinchos(m, s, carril);
        float largo = ESCONDITE_RETRAIDO_PINCHOS;

        if (e.aviso)
        {
            float asoma = 0.10f + 0.34f * SuavizarPinchos(e.progresoAviso * 2.2f);
            float vibracion = std::sin(m.tiempoPulso * 58.0f + (float)carril) * 0.035f * e.progresoAviso;
            largo = ESCONDITE_RETRAIDO_PINCHOS + asoma + vibracion;
        }
        else if (e.activo)
        {
            largo = PuntaTaladroPinchos(m, lado, carril, e.fraccion);
        }

        if (largo > mejor) mejor = largo;
    }

    return mejor;
}


// El carril esta marcado o en ataque en este instante (para luces y bocas).
static bool CarrilActivoPinchos(
    const MinijuegoRefugioPinchos& m,
    DireccionPinchos lado,
    int carril
)
{
    if (!m.patronActivo) return false;

    for (int i = 0; i < m.patron.cantidad; i++)
    {
        const SalvaPinchos& s = m.patron.salvas[i];
        if (s.lado != lado) continue;
        if (EstadoCarrilSalvaPinchos(m, s, carril).activo) return true;
    }

    return false;
}


static bool LadoActivoPinchos(const MinijuegoRefugioPinchos& m, DireccionPinchos lado)
{
    if (!m.patronActivo) return false;

    for (int k = 0; k < CantidadCarrilesPinchos(lado); k++)
    {
        if (CarrilActivoPinchos(m, lado, k)) return true;
    }

    return false;
}


// Impacto: el centro del jugador esta en la franja de un taladro letal y la
// punta ya lo ha alcanzado.
static bool JugadorGolpeadoPinchos(
    const MinijuegoRefugioPinchos& m,
    float x,
    float z
)
{
    if (!m.patronActivo) return false;

    for (int i = 0; i < m.patron.cantidad; i++)
    {
        const SalvaPinchos& s = m.patron.salvas[i];
        int carril = 0;
        float longitudinal = 0.0f;

        if (!UbicarEnCarrilPinchos(s.lado, x, z, carril, longitudinal)) continue;

        EstadoCarrilPinchos e = EstadoCarrilSalvaPinchos(m, s, carril);
        if (!e.letal) continue;

        if (longitudinal <= PuntaTaladroPinchos(m, s.lado, carril, e.fraccion)) return true;
    }

    return false;
}


// Punto letal para alguna de las salvas desde `desde` en adelante.
static bool PuntoLetalDesdePinchos(
    const MinijuegoRefugioPinchos& m,
    const PatronActivoPinchos& patron,
    int desde,
    float x,
    float z
)
{
    for (int i = desde; i < patron.cantidad; i++)
    {
        if (PuntoEnCarrilLetalPinchos(m, patron.salvas[i].lado, patron.salvas[i].mascara, x, z))
            return true;
    }

    return false;
}


// Seguro con margen: el cuerpo entero del jugador cabe a salvo.
static bool PuntoSeguroDesdePinchos(
    const MinijuegoRefugioPinchos& m,
    const PatronActivoPinchos& patron,
    int desde,
    float x,
    float z,
    float margen
)
{
    if (PuntoLetalDesdePinchos(m, patron, desde, x, z)) return false;
    if (margen <= 0.0f) return true;

    return
        !PuntoLetalDesdePinchos(m, patron, desde, x - margen, z) &&
        !PuntoLetalDesdePinchos(m, patron, desde, x + margen, z) &&
        !PuntoLetalDesdePinchos(m, patron, desde, x, z - margen) &&
        !PuntoLetalDesdePinchos(m, patron, desde, x, z + margen);
}


//==================================================
// LOGICA: REJILLA DE NAVEGACION PARA BOTS
//==================================================

static float CentroCeldaXPinchos(int i)
{
    return ((float)i - (float)(CELDAS_X_PINCHOS - 1) * 0.5f) * CELDA_PINCHOS;
}


static float CentroCeldaZPinchos(int j)
{
    return ((float)j - (float)(CELDAS_Z_PINCHOS - 1) * 0.5f) * CELDA_PINCHOS;
}


static Vector3 CentroCeldaPinchos(int celda)
{
    return
    {
        CentroCeldaXPinchos(celda % CELDAS_X_PINCHOS),
        0.0f,
        CentroCeldaZPinchos(celda / CELDAS_X_PINCHOS)
    };
}


static int CeldaDePosicionPinchos(float x, float z)
{
    int i = (int)std::lround(x / CELDA_PINCHOS + (float)(CELDAS_X_PINCHOS - 1) * 0.5f);
    int j = (int)std::lround(z / CELDA_PINCHOS + (float)(CELDAS_Z_PINCHOS - 1) * 0.5f);

    if (i < 0) i = 0;
    if (i >= CELDAS_X_PINCHOS) i = CELDAS_X_PINCHOS - 1;
    if (j < 0) j = 0;
    if (j >= CELDAS_Z_PINCHOS) j = CELDAS_Z_PINCHOS - 1;

    return j * CELDAS_X_PINCHOS + i;
}


// Celda libre mas cercana a una posicion (la rejilla bloquea un margen
// alrededor de las coberturas, y un bot puede estar dentro de ese margen).
static int CeldaLibreCercanaPinchos(
    const MinijuegoRefugioPinchos& m,
    Vector3 posicion
)
{
    int base = CeldaDePosicionPinchos(posicion.x, posicion.z);
    if (!m.bloqueada[base]) return base;

    int mejor = base;
    float mejorDistancia = 1.0e9f;
    int bi = base % CELDAS_X_PINCHOS;
    int bj = base / CELDAS_X_PINCHOS;

    for (int dj = -4; dj <= 4; dj++)
    {
        for (int di = -4; di <= 4; di++)
        {
            int i = bi + di;
            int j = bj + dj;
            if (i < 0 || i >= CELDAS_X_PINCHOS || j < 0 || j >= CELDAS_Z_PINCHOS) continue;

            int celda = j * CELDAS_X_PINCHOS + i;
            if (m.bloqueada[celda]) continue;

            float d = DistanciaCuadradaXZ(CentroCeldaPinchos(celda), posicion);
            if (d < mejorDistancia)
            {
                mejorDistancia = d;
                mejor = celda;
            }
        }
    }

    return mejor;
}


// Distancia (por la rejilla, rodeando coberturas) desde cada celda hasta la
// fuente mas cercana. Etiquetado iterativo con cola circular.
static void CalcularCampoPinchos(
    const bool bloqueada[],
    const bool fuente[],
    float campo[]
)
{
    static int cola[CELDAS_TOTALES_PINCHOS + 1];
    static bool enCola[CELDAS_TOTALES_PINCHOS];

    int inicio = 0;
    int fin = 0;

    for (int c = 0; c < CELDAS_TOTALES_PINCHOS; c++)
    {
        enCola[c] = false;
        campo[c] = 1.0e9f;

        if (fuente[c] && !bloqueada[c])
        {
            campo[c] = 0.0f;
            cola[fin] = c;
            fin = (fin + 1) % (CELDAS_TOTALES_PINCHOS + 1);
            enCola[c] = true;
        }
    }

    while (inicio != fin)
    {
        int actual = cola[inicio];
        inicio = (inicio + 1) % (CELDAS_TOTALES_PINCHOS + 1);
        enCola[actual] = false;

        int ci = actual % CELDAS_X_PINCHOS;
        int cj = actual / CELDAS_X_PINCHOS;

        for (int dj = -1; dj <= 1; dj++)
        {
            for (int di = -1; di <= 1; di++)
            {
                if (di == 0 && dj == 0) continue;

                int i = ci + di;
                int j = cj + dj;
                if (i < 0 || i >= CELDAS_X_PINCHOS || j < 0 || j >= CELDAS_Z_PINCHOS) continue;

                int vecino = j * CELDAS_X_PINCHOS + i;
                if (bloqueada[vecino]) continue;

                float costo = CELDA_PINCHOS;

                if (di != 0 && dj != 0)
                {
                    if (bloqueada[cj * CELDAS_X_PINCHOS + i] || bloqueada[j * CELDAS_X_PINCHOS + ci])
                        continue;
                    costo *= 1.4142f;
                }

                float nuevo = campo[actual] + costo;

                if (nuevo < campo[vecino])
                {
                    campo[vecino] = nuevo;

                    if (!enCola[vecino])
                    {
                        cola[fin] = vecino;
                        fin = (fin + 1) % (CELDAS_TOTALES_PINCHOS + 1);
                        enCola[vecino] = true;
                    }
                }
            }
        }
    }
}


// Avanza `pasos` celdas siguiendo el gradiente del campo. Devuelve false si
// la celda ya es la meta (campo ~ 0) o no tiene salida.
static bool SeguirCampoPinchos(
    const float campo[],
    const bool bloqueada[],
    int celda,
    int pasos,
    Vector3& destino
)
{
    destino = CentroCeldaPinchos(celda);

    if (campo[celda] >= 1.0e8f) return false;
    if (campo[celda] < 0.001f) return false;

    int actual = celda;

    for (int paso = 0; paso < pasos; paso++)
    {
        int ci = actual % CELDAS_X_PINCHOS;
        int cj = actual / CELDAS_X_PINCHOS;
        int mejor = actual;
        float mejorValor = campo[actual];

        for (int dj = -1; dj <= 1; dj++)
        {
            for (int di = -1; di <= 1; di++)
            {
                if (di == 0 && dj == 0) continue;

                int i = ci + di;
                int j = cj + dj;
                if (i < 0 || i >= CELDAS_X_PINCHOS || j < 0 || j >= CELDAS_Z_PINCHOS) continue;

                int vecino = j * CELDAS_X_PINCHOS + i;
                if (bloqueada[vecino]) continue;

                if (di != 0 && dj != 0)
                {
                    if (bloqueada[cj * CELDAS_X_PINCHOS + i] || bloqueada[j * CELDAS_X_PINCHOS + ci])
                        continue;
                }

                if (campo[vecino] < mejorValor)
                {
                    mejorValor = campo[vecino];
                    mejor = vecino;
                }
            }
        }

        if (mejor == actual) break;
        actual = mejor;
    }

    destino = CentroCeldaPinchos(actual);
    return actual != celda;
}


// Celdas bloqueadas (coberturas con margen + borde de la arena) y celdas a
// salvo de una salva completa de cada lado.
static void PrecalcularRejillaPinchos(MinijuegoRefugioPinchos& m)
{
    for (int c = 0; c < CELDAS_TOTALES_PINCHOS; c++)
    {
        Vector3 centro = CentroCeldaPinchos(c);
        bool bloquea =
            std::fabs(centro.x) > LIMITE_X_JUGADORES_PINCHOS ||
            std::fabs(centro.z) > LIMITE_Z_JUGADORES_PINCHOS;

        for (int i = 1; i < m.cantidadBloques && !bloquea; i++)
        {
            const BloquePrueba& b = m.bloques[i];
            float margen = 0.46f;

            if (
                std::fabs(centro.x - b.posicion.x) < b.tamano.x * 0.5f + margen &&
                std::fabs(centro.z - b.posicion.z) < b.tamano.z * 0.5f + margen
            )
            {
                bloquea = true;
            }
        }

        m.bloqueada[c] = bloquea;
    }

    for (int d = 0; d < 4; d++)
    {
        DireccionPinchos lado = (DireccionPinchos)d;
        unsigned int mascara = MascaraCompletaPinchos(lado);

        for (int c = 0; c < CELDAS_TOTALES_PINCHOS; c++)
        {
            Vector3 centro = CentroCeldaPinchos(c);
            bool segura = !m.bloqueada[c];

            if (segura)
            {
                const float margen = 0.25f;
                segura =
                    !PuntoEnCarrilLetalPinchos(m, lado, mascara, centro.x, centro.z) &&
                    !PuntoEnCarrilLetalPinchos(m, lado, mascara, centro.x - margen, centro.z) &&
                    !PuntoEnCarrilLetalPinchos(m, lado, mascara, centro.x + margen, centro.z) &&
                    !PuntoEnCarrilLetalPinchos(m, lado, mascara, centro.x, centro.z - margen) &&
                    !PuntoEnCarrilLetalPinchos(m, lado, mascara, centro.x, centro.z + margen);
            }

            m.coberturaLado[d][c] = segura;
        }

        // Distancia (por la rejilla) de cada celda al refugio mas cercano de este lado.
        static bool fuente[CELDAS_TOTALES_PINCHOS];
        for (int c = 0; c < CELDAS_TOTALES_PINCHOS; c++) fuente[c] = !m.bloqueada[c] && m.coberturaLado[d][c];
        CalcularCampoPinchos(m.bloqueada, fuente, m.distRefugio[d]);
    }
}


//==================================================
// LOGICA: DISPOSICION DE LAS COBERTURAS
//==================================================
// Las coberturas ocupan celdas enteras de la rejilla de carriles, a una
// celda del borde y con al menos dos celdas libres entre ellas (o en X o en
// Z): se puede correr entre ellas y rodearlas por varios lados, y nunca
// forman un pasillo estrecho ni un laberinto.

struct CoberturaCeldaPinchos
{
    int c;
    int r;
    int w;
    int h;
};


static int SeparacionCeldasPinchos(const CoberturaCeldaPinchos& a, const CoberturaCeldaPinchos& b)
{
    int gx1 = b.c - (a.c + a.w);
    int gx2 = a.c - (b.c + b.w);
    int gz1 = b.r - (a.r + a.h);
    int gz2 = a.r - (b.r + b.h);
    int gx = gx1 > gx2 ? gx1 : gx2;
    int gz = gz1 > gz2 ? gz1 : gz2;
    return gx > gz ? gx : gz;
}


static int SortearCoberturasPinchos(CoberturaCeldaPinchos salida[], int objetivo)
{
    const int formas[5][2] = { { 2, 1 }, { 1, 2 }, { 1, 1 }, { 2, 1 }, { 1, 2 } };
    int cantidad = 0;

    for (int intento = 0; intento < 500 && cantidad < objetivo; intento++)
    {
        int f = GetRandomValue(0, 4);
        CoberturaCeldaPinchos k{};
        k.w = formas[f][0];
        k.h = formas[f][1];
        k.c = GetRandomValue(1, COLUMNAS_PINCHOS - 1 - k.w);
        k.r = GetRandomValue(1, FILAS_PINCHOS - 1 - k.h);

        bool valida = true;
        for (int i = 0; i < cantidad; i++)
        {
            if (SeparacionCeldasPinchos(k, salida[i]) < 2) { valida = false; break; }
        }

        if (valida) salida[cantidad++] = k;
    }

    return cantidad;
}


static void ConstruirCoberturasPinchos(
    MinijuegoRefugioPinchos& m,
    const CoberturaCeldaPinchos cobertura[],
    int cantidad
)
{
    m.cantidadBloques = 1;

    for (int i = 0; i < cantidad; i++)
    {
        float ancho = (float)cobertura[i].w * CARRIL_PINCHOS;
        float fondo = (float)cobertura[i].h * CARRIL_PINCHOS;
        float cx = -PARED_X_PINCHOS + (float)cobertura[i].c * CARRIL_PINCHOS + ancho * 0.5f;
        float cz = -PARED_Z_PINCHOS + (float)cobertura[i].r * CARRIL_PINCHOS + fondo * 0.5f;

        AgregarBloquePrueba(
            m.bloques,
            m.cantidadBloques,
            MAX_BLOQUES_PINCHOS,
            { cx, ALTURA_COBERTURA_PINCHOS * 0.5f, cz },
            { ancho, ALTURA_COBERTURA_PINCHOS, fondo },
            Color{ 92, 84, 78, 255 }
        );
    }

    CalcularAlcancesPinchos(m);
    PrecalcularRejillaPinchos(m);
}


// Una disposicion es valida si desde cada lado hay refugio holgado (varios
// jugadores caben), todo el suelo caminable esta conectado y hay suficiente
// espacio "comodo" (a poca distancia de un refugio sea cual sea el lado).
// Ademas deja marcadas las celdas comodas: ahi aparecen los jugadores.
static bool EvaluarDisposicionPinchos(MinijuegoRefugioPinchos& m)
{
    static bool fuente[CELDAS_TOTALES_PINCHOS];
    static float campo[CELDAS_TOTALES_PINCHOS];
    static float peor[CELDAS_TOTALES_PINCHOS];

    int libres = 0;
    int primera = -1;

    for (int c = 0; c < CELDAS_TOTALES_PINCHOS; c++)
    {
        m.celdaComoda[c] = false;
        peor[c] = 0.0f;
        if (m.bloqueada[c]) continue;
        libres++;
        if (primera < 0) primera = c;
    }

    if (primera < 0) return false;

    for (int c = 0; c < CELDAS_TOTALES_PINCHOS; c++) fuente[c] = c == primera;
    CalcularCampoPinchos(m.bloqueada, fuente, campo);

    for (int c = 0; c < CELDAS_TOTALES_PINCHOS; c++)
    {
        if (!m.bloqueada[c] && campo[c] >= 1.0e8f) return false;
    }

    for (int d = 0; d < 4; d++)
    {
        int seguras = 0;

        for (int c = 0; c < CELDAS_TOTALES_PINCHOS; c++)
        {
            fuente[c] = !m.bloqueada[c] && m.coberturaLado[d][c];
            if (fuente[c]) seguras++;
        }

        if (seguras < MIN_CELDAS_SEGURAS_PINCHOS) return false;
        if ((float)seguras > MAX_FRACCION_SEGURA_PINCHOS * (float)libres) return false;

        CalcularCampoPinchos(m.bloqueada, fuente, campo);

        for (int c = 0; c < CELDAS_TOTALES_PINCHOS; c++)
        {
            if (m.bloqueada[c]) continue;
            if (campo[c] > MAX_DISTANCIA_REFUGIO_PINCHOS) return false;
            if (campo[c] > peor[c]) peor[c] = campo[c];
        }
    }

    int comodas = 0;

    for (int c = 0; c < CELDAS_TOTALES_PINCHOS; c++)
    {
        if (m.bloqueada[c] || peor[c] > DISTANCIA_COMODA_PINCHOS) continue;
        m.celdaComoda[c] = true;
        comodas++;
    }

    return comodas >= MIN_CELDAS_COMODAS_PINCHOS;
}


static void GenerarDisposicionPinchos(MinijuegoRefugioPinchos& m)
{
    CoberturaCeldaPinchos cobertura[MAX_BLOQUES_PINCHOS]{};

    for (int intento = 0; intento < 80; intento++)
    {
        int cantidad = SortearCoberturasPinchos(cobertura, 5);
        if (cantidad < 4) continue;

        ConstruirCoberturasPinchos(m, cobertura, cantidad);
        if (EvaluarDisposicionPinchos(m)) return;
    }

    // Respaldo fijo (validado en las pruebas): cuatro coberturas repartidas.
    const CoberturaCeldaPinchos respaldo[4] =
    {
        { 1, 1, 2, 1 },
        { 5, 2, 1, 2 },
        { 2, 4, 2, 1 },
        { 6, 5, 1, 1 }
    };

    ConstruirCoberturasPinchos(m, respaldo, 4);
    EvaluarDisposicionPinchos(m);
}
//==================================================
// LOGICA: PATRONES DE ATAQUE
//==================================================

// 0 al empezar la ronda, 1 al final: gobierna la muerte subita.
static float IntensidadPinchos(const MinijuegoRefugioPinchos& m)
{
    float transcurrido = DURACION_PARTIDA_PINCHOS - m.tiempoRestante;
    return LimitarPinchos(transcurrido / (DURACION_PARTIDA_PINCHOS - 4.0f), 0.0f, 1.0f);
}


static bool MuerteSubitaPinchos(const MinijuegoRefugioPinchos& m)
{
    return DURACION_PARTIDA_PINCHOS - m.tiempoRestante >= MUERTE_SUBITA_DESDE_PINCHOS;
}


static void AgregarSalvaPinchos(
    PatronActivoPinchos& patron,
    DireccionPinchos lado,
    float inicioAviso,
    float disparo,
    float paso,
    bool invertido,
    unsigned int mascara
)
{
    if (patron.cantidad >= MAX_SALVAS_PINCHOS) return;

    SalvaPinchos& s = patron.salvas[patron.cantidad++];
    s.lado = lado;
    s.inicioAviso = inicioAviso;
    s.disparo = disparo;
    s.paso = paso;
    s.invertido = invertido;
    s.mascara = mascara;
}


static DireccionPinchos LadoPerpendicularPinchos(DireccionPinchos lado)
{
    if (EsEjeZPinchos(lado))
        return GetRandomValue(0, 1) == 0 ? PINCHOS_DESDE_IZQUIERDA : PINCHOS_DESDE_DERECHA;

    return GetRandomValue(0, 1) == 0 ? PINCHOS_DESDE_ARRIBA : PINCHOS_DESDE_ABAJO;
}


static float DuracionVentanaCarrilPinchos(const MinijuegoRefugioPinchos& m)
{
    return m.tiempoExtension + TIEMPO_MANTENER_PINCHOS + TIEMPO_RETRAER_PINCHOS;
}


static PatronActivoPinchos ConstruirPatronPinchos(
    const MinijuegoRefugioPinchos& m,
    PatronPinchos tipo,
    DireccionPinchos lado,
    DireccionPinchos ladoSecundario
)
{
    PatronActivoPinchos p{};
    p.tipo = tipo;

    const float aviso = m.avisoActual;
    const float intensidad = IntensidadPinchos(m);
    const unsigned int completa = MascaraCompletaPinchos(lado);

    switch (tipo)
    {
        case PATRON_SALVA:
            AgregarSalvaPinchos(p, lado, 0.0f, aviso, 0.0f, false, completa);
            break;

        case PATRON_OLA:
            AgregarSalvaPinchos(
                p, lado, 0.0f, aviso, 0.085f - 0.030f * intensidad,
                GetRandomValue(0, 1) == 1, completa
            );
            break;

        case PATRON_ALTERNO:
        {
            unsigned int pares = 0;
            unsigned int impares = 0;

            for (int k = 0; k < CantidadCarrilesPinchos(lado); k++)
            {
                if (k % 2 == 0) pares |= 1u << k;
                else impares |= 1u << k;
            }

            bool imparesPrimero = GetRandomValue(0, 1) == 1;
            float retardo = 0.62f - 0.14f * intensidad;
            AgregarSalvaPinchos(p, lado, 0.0f, aviso, 0.0f, false, imparesPrimero ? impares : pares);
            AgregarSalvaPinchos(p, lado, 0.0f, aviso + retardo, 0.0f, false, imparesPrimero ? pares : impares);
            break;
        }

        case PATRON_RAFAGA:
        {
            float separacion = 1.00f - 0.12f * intensidad;

            for (int k = 0; k < 3; k++)
            {
                float disparo = aviso + (float)k * separacion;
                AgregarSalvaPinchos(p, lado, k == 0 ? 0.0f : disparo - 0.50f, disparo, 0.0f, false, completa);
            }
            break;
        }

        case PATRON_TENAZA:
            AgregarSalvaPinchos(p, lado, 0.0f, aviso, 0.0f, false, completa);
            AgregarSalvaPinchos(p, LadoOpuestoPinchos(lado), 0.0f, aviso, 0.0f, false, MascaraCompletaPinchos(LadoOpuestoPinchos(lado)));
            break;

        case PATRON_CRUCE:
            AgregarSalvaPinchos(p, lado, 0.0f, aviso, 0.0f, false, completa);
            AgregarSalvaPinchos(p, ladoSecundario, 0.0f, aviso, 0.0f, false, MascaraCompletaPinchos(ladoSecundario));
            break;

        case PATRON_VAIVEN:
        {
            float separacion = 1.55f - 0.30f * intensidad;
            AgregarSalvaPinchos(p, lado, 0.0f, aviso, 0.0f, false, completa);
            AgregarSalvaPinchos(
                p, ladoSecundario, aviso + 0.45f, aviso + separacion, 0.0f, false,
                MascaraCompletaPinchos(ladoSecundario)
            );
            break;
        }

        case PATRON_TOTAL:
            break;
    }

    float fin = 0.0f;

    for (int i = 0; i < p.cantidad; i++)
    {
        const SalvaPinchos& s = p.salvas[i];
        float termino = s.disparo + (float)(CantidadCarrilesPinchos(s.lado) - 1) * s.paso +
            DuracionVentanaCarrilPinchos(m);
        if (termino > fin) fin = termino;
    }

    p.duracion = fin + 0.10f;
    return p;
}


// Instante en que termina la parte letal de una salva.
static float FinLetalSalvaPinchos(const MinijuegoRefugioPinchos& m, const SalvaPinchos& s)
{
    return
        s.disparo + (float)(CantidadCarrilesPinchos(s.lado) - 1) * s.paso +
        m.tiempoExtension + TIEMPO_MANTENER_PINCHOS + 0.05f;
}


// Primera salva cuya parte letal todavia no ha terminado (cantidad si ya no
// queda ninguna).
static int EtapaActualPinchos(const MinijuegoRefugioPinchos& m, const PatronActivoPinchos& patron)
{
    for (int i = 0; i < patron.cantidad; i++)
    {
        if (m.tiempoPatron <= FinLetalSalvaPinchos(m, patron.salvas[i])) return i;
    }

    return patron.cantidad;
}


// Un patron es jugable si de verdad ofrece refugio alcanzable:
//  - tenaza / cruce: hay celdas a salvo de ambas salvas a la vez;
//  - vaiven: hay un refugio de la primera salva desde el que se llega
//    corriendo al refugio de la segunda dentro del tiempo del aviso.
// El resto (un solo lado) esta garantizado por la disposicion de coberturas.
static bool PatronValidoPinchos(const MinijuegoRefugioPinchos& m, const PatronActivoPinchos& patron)
{
    if (patron.tipo != PATRON_TENAZA && patron.tipo != PATRON_CRUCE && patron.tipo != PATRON_VAIVEN)
        return true;

    static bool fuente[CELDAS_TOTALES_PINCHOS];
    static float campo[CELDAS_TOTALES_PINCHOS];

    int seguras = 0;

    for (int c = 0; c < CELDAS_TOTALES_PINCHOS; c++)
    {
        Vector3 centro = CentroCeldaPinchos(c);
        fuente[c] = !m.bloqueada[c] && PuntoSeguroDesdePinchos(m, patron, 0, centro.x, centro.z, 0.25f);
        if (fuente[c]) seguras++;
    }

    if (seguras >= 24) return true;
    if (patron.tipo != PATRON_VAIVEN) return false;

    // Vaiven: distancia entre el refugio de la primera salva y el de la segunda.
    static bool fuenteSegunda[CELDAS_TOTALES_PINCHOS];

    for (int c = 0; c < CELDAS_TOTALES_PINCHOS; c++)
    {
        Vector3 centro = CentroCeldaPinchos(c);
        fuenteSegunda[c] = !m.bloqueada[c] && PuntoSeguroDesdePinchos(m, patron, 1, centro.x, centro.z, 0.25f);
    }

    CalcularCampoPinchos(m.bloqueada, fuenteSegunda, campo);

    float mejor = 1.0e9f;

    for (int c = 0; c < CELDAS_TOTALES_PINCHOS; c++)
    {
        if (m.bloqueada[c]) continue;

        Vector3 centro = CentroCeldaPinchos(c);
        if (!PuntoSeguroDesdePinchos(m, patron, 0, centro.x, centro.z, 0.25f)) continue;
        // Las celdas que son refugio de la primera salva pero no de la
        // segunda: hay que correr desde ellas.
        if (campo[c] < mejor) mejor = campo[c];
    }

    const float tiempoCambio = patron.salvas[1].disparo - FinLetalSalvaPinchos(m, patron.salvas[0]);
    float alcanzable = (tiempoCambio - 0.30f) * VELOCIDAD_JUGADOR_ESTANDAR;
    if (alcanzable > 3.4f) alcanzable = 3.4f;

    return mejor <= alcanzable;
}


// Elige un patron acorde a la intensidad: la variedad aparece poco a poco.
static PatronActivoPinchos ElegirPatronPinchos(
    const MinijuegoRefugioPinchos& m,
    DireccionPinchos lado
)
{
    float intensidad = IntensidadPinchos(m);

    for (int intento = 0; intento < 12; intento++)
    {
        float pesos[PATRON_TOTAL] = { 3.0f, 2.2f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };

        if (intensidad >= 0.12f) { pesos[PATRON_ALTERNO] = 2.0f; }
        if (intensidad >= 0.18f) { pesos[PATRON_RAFAGA] = 1.6f; }
        if (intensidad >= 0.30f) { pesos[PATRON_TENAZA] = 1.4f; pesos[PATRON_CRUCE] = 1.6f; }
        if (intensidad >= 0.42f) { pesos[PATRON_VAIVEN] = 2.2f; }

        if (m.ultimoPatron != PATRON_TOTAL) pesos[m.ultimoPatron] *= 0.25f;

        float total = 0.0f;
        for (int i = 0; i < PATRON_TOTAL; i++) total += pesos[i];

        float sorteo = AzarPinchos(0.0f, total);
        PatronPinchos tipo = PATRON_SALVA;

        for (int i = 0; i < PATRON_TOTAL; i++)
        {
            if (sorteo < pesos[i]) { tipo = (PatronPinchos)i; break; }
            sorteo -= pesos[i];
        }

        DireccionPinchos secundario = LadoOpuestoPinchos(lado);

        if (tipo == PATRON_CRUCE) secundario = LadoPerpendicularPinchos(lado);
        else if (tipo == PATRON_VAIVEN && GetRandomValue(0, 2) == 0) secundario = LadoPerpendicularPinchos(lado);

        PatronActivoPinchos patron = ConstruirPatronPinchos(m, tipo, lado, secundario);
        if (PatronValidoPinchos(m, patron)) return patron;
    }

    return ConstruirPatronPinchos(m, PATRON_SALVA, lado, LadoOpuestoPinchos(lado));
}


//==================================================
// PARTICULAS LOCALES (polvo, chispas, escombros)
//==================================================

static ParticulaRefugioPinchos* ReservarParticulaPinchos(MinijuegoRefugioPinchos& m)
{
    for (int i = 0; i < MAX_PARTICULAS_PINCHOS; i++)
    {
        if (!m.particulasLocales[i].activa) return &m.particulasLocales[i];
    }

    return nullptr;
}


static int CantidadParticulasPinchos(int cantidad)
{
    float factor = FactorCalidadGrafica(CalidadParticulas());
    int escalada = (int)std::lround((float)cantidad * factor);
    return escalada < 1 ? 1 : escalada;
}


static void EmitirPolvoPinchos(
    MinijuegoRefugioPinchos& m,
    Vector3 posicion,
    int cantidad,
    float fuerza,
    float tamano
)
{
    int total = CantidadParticulasPinchos(cantidad);

    for (int i = 0; i < total; i++)
    {
        ParticulaRefugioPinchos* p = ReservarParticulaPinchos(m);
        if (p == nullptr) return;

        float angulo = AzarPinchos(0.0f, 6.2832f);
        float velocidad = AzarPinchos(0.2f, 1.0f) * fuerza;

        *p = {};
        p->activa = true;
        p->chispa = false;
        p->posicion = posicion;
        p->velocidad =
        {
            std::cos(angulo) * velocidad,
            AzarPinchos(0.2f, 0.9f) * fuerza * 0.6f,
            std::sin(angulo) * velocidad
        };
        p->vidaMaxima = AzarPinchos(0.55f, 1.05f);
        p->vida = p->vidaMaxima;
        p->tamano = tamano * AzarPinchos(0.7f, 1.3f);
        unsigned char tono = (unsigned char)GetRandomValue(128, 168);
        p->color = { tono, (unsigned char)(tono - 12), (unsigned char)(tono - 30), 200 };
    }
}


static void EmitirChispasPinchos(
    MinijuegoRefugioPinchos& m,
    Vector3 posicion,
    Vector3 direccionBase,
    int cantidad,
    float fuerza
)
{
    int total = CantidadParticulasPinchos(cantidad);

    for (int i = 0; i < total; i++)
    {
        ParticulaRefugioPinchos* p = ReservarParticulaPinchos(m);
        if (p == nullptr) return;

        *p = {};
        p->activa = true;
        p->chispa = true;
        p->posicion = posicion;
        p->velocidad =
        {
            direccionBase.x * fuerza * AzarPinchos(0.3f, 1.0f) + AzarPinchos(-1.6f, 1.6f),
            AzarPinchos(1.0f, 3.6f),
            direccionBase.z * fuerza * AzarPinchos(0.3f, 1.0f) + AzarPinchos(-1.6f, 1.6f)
        };
        p->vidaMaxima = AzarPinchos(0.22f, 0.50f);
        p->vida = p->vidaMaxima;
        p->tamano = AzarPinchos(0.04f, 0.075f);
        p->color = { 255, (unsigned char)GetRandomValue(170, 235), (unsigned char)GetRandomValue(50, 110), 255 };
    }
}


static void EmitirEscombroPinchos(
    MinijuegoRefugioPinchos& m,
    Vector3 posicion,
    int cantidad,
    float fuerza,
    Color color
)
{
    int total = CantidadParticulasPinchos(cantidad);

    for (int i = 0; i < total; i++)
    {
        ParticulaRefugioPinchos* p = ReservarParticulaPinchos(m);
        if (p == nullptr) return;

        float angulo = AzarPinchos(0.0f, 6.2832f);
        float velocidad = AzarPinchos(0.3f, 1.0f) * fuerza;

        *p = {};
        p->activa = true;
        p->chispa = true;
        p->posicion = posicion;
        p->velocidad = { std::cos(angulo) * velocidad, AzarPinchos(0.5f, 2.6f) * fuerza * 0.5f, std::sin(angulo) * velocidad };
        p->vidaMaxima = AzarPinchos(0.7f, 1.2f);
        p->vida = p->vidaMaxima;
        p->tamano = AzarPinchos(0.06f, 0.13f);
        p->color = color;
    }
}


// Brasas que suben de las antorchas y motas de polvo flotando en la cueva.
static void ActualizarAmbientePinchos(MinijuegoRefugioPinchos& m, float deltaTime)
{
    if (FactorCalidadGrafica(CalidadParticulas()) < 0.5f) return;

    m.temporizadorAmbiente -= deltaTime;
    if (m.temporizadorAmbiente > 0.0f) return;

    m.temporizadorAmbiente = AzarPinchos(0.12f, 0.28f);

    ParticulaRefugioPinchos* brasa = ReservarParticulaPinchos(m);
    if (brasa != nullptr)
    {
        const float xAntorchas[3] = { -3.2f, 0.0f, 3.2f };
        *brasa = {};
        brasa->activa = true;
        brasa->posicion = { xAntorchas[GetRandomValue(0, 2)] + AzarPinchos(-0.05f, 0.05f), 1.62f, -PARED_Z_PINCHOS + 0.12f };
        brasa->velocidad = { AzarPinchos(-0.25f, 0.25f), AzarPinchos(0.5f, 1.0f), AzarPinchos(0.05f, 0.30f) };
        brasa->vidaMaxima = AzarPinchos(1.0f, 1.8f);
        brasa->vida = brasa->vidaMaxima;
        brasa->tamano = 0.05f;
        brasa->color = { 255, 170, 70, 230 };
    }

    ParticulaRefugioPinchos* mota = ReservarParticulaPinchos(m);
    if (mota != nullptr)
    {
        *mota = {};
        mota->activa = true;
        mota->posicion = { AzarPinchos(-4.8f, 4.8f), AzarPinchos(0.4f, 2.0f), AzarPinchos(-4.0f, 4.0f) };
        mota->velocidad = { AzarPinchos(-0.15f, 0.15f), AzarPinchos(0.05f, 0.25f), AzarPinchos(-0.15f, 0.15f) };
        mota->vidaMaxima = AzarPinchos(2.0f, 3.5f);
        mota->vida = mota->vidaMaxima;
        mota->tamano = 0.04f;
        mota->color = { 190, 180, 160, 120 };
    }
}


static void ActualizarParticulasPinchos(MinijuegoRefugioPinchos& m, float deltaTime)
{
    for (int i = 0; i < MAX_PARTICULAS_PINCHOS; i++)
    {
        ParticulaRefugioPinchos& p = m.particulasLocales[i];
        if (!p.activa) continue;

        p.vida -= deltaTime;
        if (p.vida <= 0.0f)
        {
            p.activa = false;
            continue;
        }

        if (p.chispa)
        {
            p.velocidad.y -= 11.0f * deltaTime;
        }
        else
        {
            float freno = 1.0f - 2.0f * deltaTime;
            if (freno < 0.0f) freno = 0.0f;
            p.velocidad.x *= freno;
            p.velocidad.z *= freno;
            p.velocidad.y += 0.6f * deltaTime;
        }

        p.posicion.x += p.velocidad.x * deltaTime;
        p.posicion.y += p.velocidad.y * deltaTime;
        p.posicion.z += p.velocidad.z * deltaTime;

        if (p.chispa && p.posicion.y < 0.04f)
        {
            p.posicion.y = 0.04f;
            p.velocidad.y *= -0.30f;
            p.velocidad.x *= 0.6f;
            p.velocidad.z *= 0.6f;
        }
    }
}


//==================================================
// LOGICA: LIMITES Y BOTS
//==================================================

static void LimitarJugadorSalaPinchos(JugadorPrueba& jugador)
{
    if (jugador.posicion.x < -LIMITE_X_JUGADORES_PINCHOS)
    {
        jugador.posicion.x = -LIMITE_X_JUGADORES_PINCHOS;
        if (jugador.velocidad.x < 0.0f) jugador.velocidad.x = 0.0f;
        if (jugador.empuje.x < 0.0f) jugador.empuje.x = 0.0f;
    }

    if (jugador.posicion.x > LIMITE_X_JUGADORES_PINCHOS)
    {
        jugador.posicion.x = LIMITE_X_JUGADORES_PINCHOS;
        if (jugador.velocidad.x > 0.0f) jugador.velocidad.x = 0.0f;
        if (jugador.empuje.x > 0.0f) jugador.empuje.x = 0.0f;
    }

    if (jugador.posicion.z < -LIMITE_Z_JUGADORES_PINCHOS)
    {
        jugador.posicion.z = -LIMITE_Z_JUGADORES_PINCHOS;
        if (jugador.velocidad.z < 0.0f) jugador.velocidad.z = 0.0f;
        if (jugador.empuje.z < 0.0f) jugador.empuje.z = 0.0f;
    }

    if (jugador.posicion.z > LIMITE_Z_JUGADORES_PINCHOS)
    {
        jugador.posicion.z = LIMITE_Z_JUGADORES_PINCHOS;
        if (jugador.velocidad.z > 0.0f) jugador.velocidad.z = 0.0f;
        if (jugador.empuje.z > 0.0f) jugador.empuje.z = 0.0f;
    }

    if (jugador.posicion.y < 0.72f)
    {
        jugador.posicion.y = 0.72f;
        jugador.velocidad.y = 0.0f;
        jugador.enSuelo = true;
        jugador.cayendo = false;
    }
}


// Penalizacion por acercarse al destino de otro bot del equipo.
static float ApinamientoPinchos(
    const MinijuegoRefugioPinchos& m,
    int indice,
    Vector3 centro,
    int cantidadMaxima
)
{
    float penalizacion = 0.0f;

    for (int j = 0; j < cantidadMaxima; j++)
    {
        if (j == indice || !JugadorEnPartida(m, j)) continue;

        const CerebroBotRefugioPinchos& otro = m.cerebros[j];
        if (otro.celdaDestino < 0) continue;

        float dist = std::sqrt(DistanciaCuadradaXZ(centro, CentroCeldaPinchos(otro.celdaDestino)));
        if (dist < 1.3f) penalizacion += (1.3f - dist) * 1.6f;
    }

    return penalizacion;
}


// Entre ataques el bot se coloca en un punto que le proteja del mayor numero
// de lados posible, cerca de donde esta y sin amontonarse con sus companeros.
static void ElegirDestinoEsperaPinchos(
    MinijuegoRefugioPinchos& m,
    int indice,
    const JugadorPrueba jugadores[],
    int cantidadMaxima
)
{
    CerebroBotRefugioPinchos& cerebro = m.cerebros[indice];
    float mejorPuntaje = 1.0e9f;
    int mejorCelda = -1;

    for (int celda = 0; celda < CELDAS_TOTALES_PINCHOS; celda++)
    {
        if (m.bloqueada[celda]) continue;

        float peor = 0.0f;
        for (int d = 0; d < 4; d++)
        {
            if (m.distRefugio[d][celda] > peor) peor = m.distRefugio[d][celda];
        }

        if (peor >= 1.0e8f) continue;

        Vector3 centro = CentroCeldaPinchos(celda);
        float viaje = std::sqrt(DistanciaCuadradaXZ(centro, jugadores[indice].posicion));
        float puntaje =
            peor + viaje * 0.12f +
            ApinamientoPinchos(m, indice, centro, cantidadMaxima) +
            AzarPinchos(0.0f, 0.8f);

        if (puntaje < mejorPuntaje)
        {
            mejorPuntaje = puntaje;
            mejorCelda = celda;
        }
    }

    cerebro.proximaReubicacion = AzarPinchos(1.8f, 3.6f);

    if (mejorCelda < 0) return;

    static bool fuente[CELDAS_TOTALES_PINCHOS];
    for (int c = 0; c < CELDAS_TOTALES_PINCHOS; c++) fuente[c] = c == mejorCelda;

    CalcularCampoPinchos(m.bloqueada, fuente, cerebro.campoDestino);
    cerebro.tieneDestino = true;
    cerebro.destinoPatron = false;
    cerebro.celdaDestino = mejorCelda;
}


// Reaccion al ataque: busca el refugio alcanzable mas barato. Si queda un
// refugio valido para todas las salvas pendientes lo prefiere; si no,
// elige uno para la salva inminente que quede cerca del siguiente.
static void ElegirDestinoPatronPinchos(
    MinijuegoRefugioPinchos& m,
    int indice,
    const JugadorPrueba jugadores[],
    int cantidadMaxima,
    int etapa
)
{
    CerebroBotRefugioPinchos& cerebro = m.cerebros[indice];
    const PatronActivoPinchos& referencia = cerebro.confundido ? cerebro.percibido : m.patron;

    static bool fuenteBot[CELDAS_TOTALES_PINCHOS];
    static float campoBot[CELDAS_TOTALES_PINCHOS];
    static bool candidata[CELDAS_TOTALES_PINCHOS];
    static bool fuenteSiguiente[CELDAS_TOTALES_PINCHOS];
    static float campoSiguiente[CELDAS_TOTALES_PINCHOS];

    int celdaBot = CeldaLibreCercanaPinchos(m, jugadores[indice].posicion);
    for (int c = 0; c < CELDAS_TOTALES_PINCHOS; c++) fuenteBot[c] = c == celdaBot;
    CalcularCampoPinchos(m.bloqueada, fuenteBot, campoBot);

    float margen = 0.25f;
    bool usaSiguiente = false;
    int total = 0;

    for (int c = 0; c < CELDAS_TOTALES_PINCHOS; c++)
    {
        Vector3 centro = CentroCeldaPinchos(c);
        candidata[c] =
            !m.bloqueada[c] && campoBot[c] < 1.0e8f &&
            PuntoSeguroDesdePinchos(m, referencia, etapa, centro.x, centro.z, margen);
        if (candidata[c]) total++;
    }

    if (total < 6)
    {
        // Solo la salva inminente; ademas se valora estar cerca del refugio de la siguiente.
        PatronActivoPinchos inminente{};
        inminente.cantidad = 1;
        inminente.salvas[0] = referencia.salvas[etapa];

        total = 0;
        for (int c = 0; c < CELDAS_TOTALES_PINCHOS; c++)
        {
            Vector3 centro = CentroCeldaPinchos(c);
            candidata[c] =
                !m.bloqueada[c] && campoBot[c] < 1.0e8f &&
                PuntoSeguroDesdePinchos(m, inminente, 0, centro.x, centro.z, margen);
            if (candidata[c]) total++;
        }

        if (etapa + 1 < referencia.cantidad)
        {
            for (int c = 0; c < CELDAS_TOTALES_PINCHOS; c++)
            {
                Vector3 centro = CentroCeldaPinchos(c);
                fuenteSiguiente[c] =
                    !m.bloqueada[c] &&
                    PuntoSeguroDesdePinchos(m, referencia, etapa + 1, centro.x, centro.z, margen);
            }

            CalcularCampoPinchos(m.bloqueada, fuenteSiguiente, campoSiguiente);
            usaSiguiente = true;
        }
    }

    if (total == 0) margen = 0.0f;

    float mejorPuntaje = 1.0e9f;
    int mejorCelda = -1;

    for (int c = 0; c < CELDAS_TOTALES_PINCHOS; c++)
    {
        if (total > 0 && !candidata[c]) continue;
        if (m.bloqueada[c] || campoBot[c] >= 1.0e8f) continue;

        Vector3 centro = CentroCeldaPinchos(c);

        if (total == 0 && !PuntoSeguroDesdePinchos(m, referencia, etapa, centro.x, centro.z, 0.0f)) continue;

        float puntaje =
            campoBot[c] +
            ApinamientoPinchos(m, indice, centro, cantidadMaxima) +
            AzarPinchos(0.0f, 0.9f);

        if (usaSiguiente && campoSiguiente[c] < 1.0e8f) puntaje += 0.7f * campoSiguiente[c];

        if (puntaje < mejorPuntaje)
        {
            mejorPuntaje = puntaje;
            mejorCelda = c;
        }
    }

    cerebro.destinoPatron = true;
    if (mejorCelda < 0) return;

    static bool fuente[CELDAS_TOTALES_PINCHOS];
    for (int c = 0; c < CELDAS_TOTALES_PINCHOS; c++) fuente[c] = c == mejorCelda;

    CalcularCampoPinchos(m.bloqueada, fuente, cerebro.campoDestino);
    cerebro.tieneDestino = true;
    cerebro.celdaDestino = mejorCelda;
}


// El bot "ve" la senal con un retardo humano; a veces duda un instante
// mas o lee mal el lado (se corrige al ver asomar los taladros).
static void IniciarEtapaBotPinchos(MinijuegoRefugioPinchos& m, int indice, int etapa)
{
    CerebroBotRefugioPinchos& cerebro = m.cerebros[indice];
    cerebro.etapa = etapa;
    cerebro.destinoPatron = false;

    if (etapa == 0)
    {
        cerebro.tiempoReaccion = AzarPinchos(0.28f, 0.60f);
        if (GetRandomValue(0, 99) < 12) cerebro.tiempoReaccion += AzarPinchos(0.20f, 0.40f);

        cerebro.confundido = GetRandomValue(0, 99) < 8;

        if (cerebro.confundido)
        {
            int giro = GetRandomValue(1, 3);
            cerebro.percibido = m.patron;

            for (int i = 0; i < cerebro.percibido.cantidad; i++)
            {
                SalvaPinchos& s = cerebro.percibido.salvas[i];
                DireccionPinchos nuevo = (DireccionPinchos)(((int)s.lado + giro) % 4);
                s.lado = nuevo;
                s.mascara = MascaraCompletaPinchos(nuevo);
            }
        }
    }
    else
    {
        cerebro.tiempoReaccion = AzarPinchos(0.10f, 0.26f);
    }
}


static InputMinijuegoParticipante CrearEntradaBotEquipoPinchos(
    MinijuegoRefugioPinchos& m,
    int indice,
    const JugadorPrueba jugadores[],
    int cantidadMaxima,
    float deltaTime
)
{
    CerebroBotRefugioPinchos& cerebro = m.cerebros[indice];
    const JugadorPrueba& jugador = jugadores[indice];

    int etapa = m.patronActivo ? EtapaActualPinchos(m, m.patron) : -1;
    bool amenaza = m.patronActivo && etapa < m.patron.cantidad;

    if (amenaza)
    {
        if (cerebro.etapa != etapa) IniciarEtapaBotPinchos(m, indice, etapa);

        // El bot confundido se da cuenta cuando los taladros asoman.
        const SalvaPinchos& salva = m.patron.salvas[etapa];
        float corrige = salva.inicioAviso + 0.55f * (salva.disparo - salva.inicioAviso);

        if (cerebro.confundido && m.tiempoPatron >= corrige)
        {
            cerebro.confundido = false;
            cerebro.destinoPatron = false;
            cerebro.tiempoReaccion = AzarPinchos(0.10f, 0.22f);
        }

        if (cerebro.tiempoReaccion > 0.0f) cerebro.tiempoReaccion -= deltaTime;
        else if (!cerebro.destinoPatron) ElegirDestinoPatronPinchos(m, indice, jugadores, cantidadMaxima, etapa);
    }
    else
    {
        cerebro.etapa = -1;

        if (cerebro.destinoPatron)
        {
            cerebro.destinoPatron = false;
            cerebro.tieneDestino = false;
        }

        cerebro.proximaReubicacion -= deltaTime;

        if (!cerebro.tieneDestino || cerebro.proximaReubicacion <= 0.0f)
            ElegirDestinoEsperaPinchos(m, indice, jugadores, cantidadMaxima);
    }

    InputMinijuegoParticipante entrada{};
    if (!cerebro.tieneDestino) return entrada;

    int celda = CeldaLibreCercanaPinchos(m, jugador.posicion);
    Vector3 destino{};

    if (SeguirCampoPinchos(cerebro.campoDestino, m.bloqueada, celda, 3, destino))
        entrada = CrearEntradaBotHaciaObjetivo1v3(jugador.posicion, destino, 0.10f);

    // Antiatasco: si pide moverse y no avanza, replantea el destino.
    bool pideMover = entrada.izquierda || entrada.derecha || entrada.adelante || entrada.atras;
    float desplazamiento =
        std::fabs(jugador.posicion.x - cerebro.ultimoX) + std::fabs(jugador.posicion.z - cerebro.ultimoZ);

    if (!pideMover || desplazamiento > 0.03f)
    {
        cerebro.ultimoX = jugador.posicion.x;
        cerebro.ultimoZ = jugador.posicion.z;
        cerebro.tiempoAtasco = 0.0f;
    }
    else
    {
        cerebro.tiempoAtasco += deltaTime;

        if (cerebro.tiempoAtasco > 0.5f)
        {
            cerebro.tiempoAtasco = 0.0f;
            if (cerebro.destinoPatron) cerebro.destinoPatron = false;
            else cerebro.tieneDestino = false;
        }
    }

    return entrada;
}


// El operador bot elige el lado que deja a mas jugadores sin refugio, con
// ruido y algo de capricho para no ser predecible.
static DireccionPinchos ElegirLadoBotSoloPinchos(
    const MinijuegoRefugioPinchos& m,
    const JugadorPrueba jugadores[],
    int cantidadMaxima
)
{
    if (GetRandomValue(0, 99) < 10) return (DireccionPinchos)GetRandomValue(0, 3);

    DireccionPinchos mejor = PINCHOS_DESDE_ARRIBA;
    float mejorPuntaje = -1.0e9f;

    for (int d = 0; d < 4; d++)
    {
        float puntaje = 0.0f;

        for (int i = 0; i < cantidadMaxima; i++)
        {
            if (!JugadorEnPartida(m, i)) continue;

            bool expuesto = PuntoEnCarrilLetalPinchos(
                m, (DireccionPinchos)d, MascaraCompletaPinchos((DireccionPinchos)d),
                jugadores[i].posicion.x, jugadores[i].posicion.z
            );

            if (expuesto)
            {
                float dist = m.distRefugio[d][CeldaLibreCercanaPinchos(m, jugadores[i].posicion)];
                if (dist > 6.0f) dist = 6.0f;
                puntaje += 1.0f + 0.3f * dist;
            }
        }

        puntaje += AzarPinchos(0.0f, 1.0f);
        if ((DireccionPinchos)d == m.ultimaDireccion) puntaje -= 0.5f;

        if (puntaje > mejorPuntaje)
        {
            mejorPuntaje = puntaje;
            mejor = (DireccionPinchos)d;
        }
    }

    return mejor;
}


//==================================================
// LOGICA: FLUJO DE LA RONDA
//==================================================

static void SacudirCamaraPinchos(float intensidad, float duracion)
{
    ActivarTemblorCamaraGeneral(intensidad, duracion);
}


static void EmitirPolvoBocasPinchos(MinijuegoRefugioPinchos& m, const SalvaPinchos& salva)
{
    for (int c = 0; c < CantidadCarrilesPinchos(salva.lado); c++)
    {
        if ((salva.mascara & (1u << c)) == 0) continue;

        Vector3 origen{};
        Vector3 avance{};
        ObtenerOrigenTaladroPinchos(salva.lado, c, origen, avance);
        EmitirPolvoPinchos(m, { origen.x + avance.x * 0.4f, 0.15f, origen.z + avance.z * 0.4f }, 3, 1.0f, 0.18f);
    }
}


static void IniciarPatronPinchos(
    MinijuegoRefugioPinchos& m,
    DireccionPinchos lado
)
{
    float intensidad = IntensidadPinchos(m);
    m.avisoActual = AVISO_INICIAL_PINCHOS + (AVISO_FINAL_PINCHOS - AVISO_INICIAL_PINCHOS) * intensidad;
    m.tiempoExtension = EXTENDER_INICIAL_PINCHOS + (EXTENDER_FINAL_PINCHOS - EXTENDER_INICIAL_PINCHOS) * intensidad;

    m.patron = ElegirPatronPinchos(m, lado);
    m.patronActivo = true;
    m.tiempoPatron = 0.0f;
    m.ultimoPatron = m.patron.tipo;
    m.ultimaDireccion = lado;
    m.fase = FASE_PINCHOS_AVISO;
    m.temporizadorEmision = 0.0f;

    for (int s = 0; s < MAX_SALVAS_PINCHOS; s++)
    {
        m.salvaSonada[s] = false;
        m.avisoSonado[s] = false;
        m.ticSonado[s] = false;

        for (int k = 0; k < MAX_CARRILES_PINCHOS; k++)
        {
            m.salvaLanzada[s][k] = false;
            m.salvaImpactada[s][k] = false;
        }
    }

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        m.cerebros[i].etapa = -1;
        m.cerebros[i].confundido = false;
    }

    m.avisoSonado[0] = true;
    EmitirPolvoBocasPinchos(m, m.patron.salvas[0]);

    ReproducirSonidoMinijuego(m.audio, SONIDO_ERROR);
    SacudirCamaraPinchos(0.03f, 0.2f);
}


static void ActualizarEfectosPatronPinchos(MinijuegoRefugioPinchos& m, float deltaTime)
{
    m.temporizadorEmision -= deltaTime;
    bool emitir = m.temporizadorEmision <= 0.0f;
    if (emitir) m.temporizadorEmision = 0.05f;

    for (int si = 0; si < m.patron.cantidad; si++)
    {
        const SalvaPinchos& s = m.patron.salvas[si];
        int cantidad = CantidadCarrilesPinchos(s.lado);

        // Aviso de una salva posterior.
        if (!m.avisoSonado[si] && m.tiempoPatron >= s.inicioAviso)
        {
            m.avisoSonado[si] = true;
            EmitirPolvoBocasPinchos(m, s);
            ReproducirSonidoMinijuego(m.audio, SONIDO_ALERTA_TIEMPO);
            SacudirCamaraPinchos(0.03f, 0.18f);
        }

        // Ultimo tic justo antes de que salgan los taladros.
        if (!m.ticSonado[si] && m.tiempoPatron >= s.disparo - 0.35f && m.tiempoPatron < s.disparo)
        {
            m.ticSonado[si] = true;
            if (si == 0) ReproducirSonidoMinijuego(m.audio, SONIDO_ALERTA_TIEMPO);
        }

        // Temblor del muro: polvo y piedras caen de la boca de los tuneles.
        if (emitir && m.tiempoPatron >= s.inicioAviso && m.tiempoPatron < s.disparo)
        {
            int c = GetRandomValue(0, cantidad - 1);

            if (s.mascara & (1u << c))
            {
                Vector3 origen{};
                Vector3 avance{};
                ObtenerOrigenTaladroPinchos(s.lado, c, origen, avance);
                EmitirPolvoPinchos(m, { origen.x + avance.x * 0.3f, 1.2f, origen.z + avance.z * 0.3f }, 2, 0.8f, 0.16f);

                if (GetRandomValue(0, 3) == 0)
                    EmitirEscombroPinchos(m, { origen.x + avance.x * 0.3f, 1.9f, origen.z + avance.z * 0.3f }, 1, 0.4f, Color{ 100, 90, 82, 255 });
            }
        }

        for (int c = 0; c < cantidad; c++)
        {
            EstadoCarrilPinchos e = EstadoCarrilSalvaPinchos(m, s, c);

            Vector3 origen{};
            Vector3 avance{};
            ObtenerOrigenTaladroPinchos(s.lado, c, origen, avance);

            if (e.letal && !m.salvaLanzada[si][c])
            {
                m.salvaLanzada[si][c] = true;
                Vector3 boca = { origen.x + avance.x * 0.5f, 0.55f, origen.z + avance.z * 0.5f };
                EmitirPolvoPinchos(m, boca, 4, 2.2f, 0.26f);
                EmitirChispasPinchos(m, boca, avance, 4, 4.0f);

                if (!m.salvaSonada[si])
                {
                    m.salvaSonada[si] = true;
                    ReproducirSonidoMinijuego(m.audio, SONIDO_GROUND_POUND);
                    SacudirCamaraPinchos(0.10f, 0.28f);
                }
            }

            float largo = PuntaTaladroPinchos(m, s.lado, c, e.fraccion);
            Vector3 punta = { origen.x + avance.x * largo, 0.10f, origen.z + avance.z * largo };

            if (emitir && e.letal && e.fraccion > 0.05f && e.fraccion < 0.999f)
            {
                // Rastro de polvo y chispas que levanta la broca al avanzar.
                EmitirPolvoPinchos(m, punta, 1, 1.4f, 0.20f);
                EmitirChispasPinchos(m, { punta.x, 0.45f, punta.z }, avance, 1, 2.0f);
            }

            if (m.chocaCobertura[(int)s.lado][c] && !m.salvaImpactada[si][c] && e.letal && e.fraccion >= 0.999f)
            {
                m.salvaImpactada[si][c] = true;
                Vector3 choque = { punta.x, 0.60f, punta.z };
                EmitirChispasPinchos(m, choque, { -avance.x, 0.0f, -avance.z }, 10, 4.5f);
                EmitirEscombroPinchos(m, choque, 4, 3.0f, Color{ 112, 100, 90, 255 });
                EmitirPolvoPinchos(m, choque, 4, 2.0f, 0.30f);

                bool primero = true;
                for (int k = 0; k < c; k++)
                {
                    if (m.salvaImpactada[si][k]) primero = false;
                }

                if (primero)
                {
                    ReproducirSonidoMinijuego(m.audio, SONIDO_IMPACTO);
                    SacudirCamaraPinchos(0.14f, 0.24f);
                }
            }
        }
    }
}


static void ResolverImpactosTaladros(
    MinijuegoRefugioPinchos& m,
    JugadorPrueba jugadores[],
    int cantidadMaxima,
    const Participante participantes[]
)
{
    for (int i = 0; i < cantidadMaxima; i++)
    {
        if (!JugadorEnPartida(m, i)) continue;
        if (!JugadorGolpeadoPinchos(m, jugadores[i].posicion.x, jugadores[i].posicion.z)) continue;

        m.estadosJugadores[i].eliminado = true;
        m.estadosJugadores[i].tiempoDesdeEliminacion = 0.0f;
        jugadores[i].velocidad = {};
        jugadores[i].empuje = {};
        jugadores[i].aplastado = true;

        Vector3 centro = { jugadores[i].posicion.x, 0.45f, jugadores[i].posicion.z };
        EmitirEscombroPinchos(m, centro, 12, 3.2f, participantes[i].color);
        EmitirPolvoPinchos(m, centro, 8, 2.4f, 0.30f);
        EmitirChispasPinchos(m, centro, { 0.0f, 0.0f, 0.0f }, 8, 2.0f);

        ReproducirSonidoMinijuego(m.audio, SONIDO_ELIMINADO);
        SacudirCamaraPinchos(0.16f, 0.30f);
    }
}


static void FinalizarRefugioPinchos(MinijuegoRefugioPinchos& minijuego, bool ganaSolo)
{
    if (minijuego.resultado.estado == RESULTADO_MINIJUEGO_FINALIZADO) return;

    minijuego.resultado.estado = RESULTADO_MINIJUEGO_FINALIZADO;
    minijuego.resultado.desenlace = DESENLACE_CON_GANADOR;
    minijuego.resultado.cantidadEquipos = 2;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        ResultadoParticipante& resultadoJugador = minijuego.resultado.participantes[i];
        if (!resultadoJugador.participo) continue;

        bool esSolo = i == minijuego.indiceSolo;
        bool ganador = esSolo == ganaSolo;
        resultadoJugador.numeroEquipo = esSolo ? 0 : 1;
        resultadoJugador.posicionFinal = ganador ? 1 : 2;
        int rivales = 0;
        for (int k = 0; k < MAX_PARTICIPANTES; k++)
        {
            if (k != minijuego.indiceSolo && minijuego.resultado.participantes[k].participo) rivales++;
        }

        resultadoJugador.puntuacionMinijuego = esSolo
            ? rivales - ContarRivalesVivosPinchos(minijuego)
            : (!minijuego.estadosJugadores[i].eliminado ? 1 : 0);
        resultadoJugador.puntosObtenidos = 0;
    }

    minijuego.patronActivo = false;
    minijuego.fase = FASE_PINCHOS_TERMINADO;
    ReproducirSonidoMinijuego(minijuego.audio, SONIDO_RESULTADO);
}


void MinijuegoRefugioPinchos::Inicializar()
{
    resultado = {};
    resultado.formato = FORMATO_MINIJUEGO_EQUIPOS;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        estadosJugadores[i] = {};
        cerebros[i] = {};
    }

    for (int i = 0; i < MAX_PARTICULAS_PINCHOS; i++) particulasLocales[i] = {};

    cantidadBloques = 0;

    AgregarBloquePrueba(
        bloques,
        cantidadBloques,
        MAX_BLOQUES_PINCHOS,
        { 0.0f, -0.30f, 0.0f },
        { PARED_X_PINCHOS * 2.0f + 0.10f, 0.60f, PARED_Z_PINCHOS * 2.0f + 0.10f },
        Color{ 80, 82, 88, 255 }
    );

    GenerarDisposicionPinchos(*this);

    indiceSolo = -1;
    ultimaDireccion = PINCHOS_DESDE_ARRIBA;
    ultimoPatron = PATRON_TOTAL;
    patron = {};
    patronActivo = false;
    tiempoPatron = 0.0f;
    avisoActual = AVISO_INICIAL_PINCHOS;
    tiempoExtension = EXTENDER_INICIAL_PINCHOS;
    fase = FASE_PINCHOS_PREPARACION;
    tiempoPreparacion = DURACION_PREPARACION_PINCHOS;
    tiempoRestante = DURACION_PARTIDA_PINCHOS;
    cooldownAtaque = COOLDOWN_INICIAL_PINCHOS;
    tiempoEleccion = 0.0f;
    decisionBotSolo = 0.0f;
    tiempoPulso = 0.0f;
    ataquesRealizados = 0;
    tiempoAgotado = false;
    temporizadorEmision = 0.0f;
    temporizadorAmbiente = 0.0f;

    for (int s = 0; s < MAX_SALVAS_PINCHOS; s++)
    {
        salvaSonada[s] = false;
        avisoSonado[s] = false;
        ticSonado[s] = false;

        for (int k = 0; k < MAX_CARRILES_PINCHOS; k++)
        {
            salvaLanzada[s][k] = false;
            salvaImpactada[s][k] = false;
        }
    }

    camara.position = { 0.0f, 14.6f, 8.6f };
    camara.target = { 0.0f, 0.4f, -0.5f };
    camara.up = { 0.0f, 1.0f, 0.0f };
    camara.fovy = 46.0f;
    camara.projection = CAMERA_PERSPECTIVE;
}


void MinijuegoRefugioPinchos::Reiniciar(
    JugadorPrueba jugadores[],
    Participante participantes[],
    int cantidadMaxima
)
{
    Inicializar();

    InicializarResultadoMinijuego(resultado, participantes, FORMATO_MINIJUEGO_EQUIPOS);

    int indices[MAX_PARTICIPANTES]{};
    int cantidad = ObtenerIndicesParticipantesActivos(participantes, indices, MAX_PARTICIPANTES);

    if (cantidad < 2)
    {
        resultado.estado = RESULTADO_MINIJUEGO_CANCELADO;
        fase = FASE_PINCHOS_TERMINADO;
        return;
    }

    indiceSolo = indices[GetRandomValue(0, cantidad - 1)];
    resultado.cantidadEquipos = 2;
    decisionBotSolo = AzarPinchos(0.9f, 1.4f);

    Vector3 spawnsUsados[MAX_PARTICIPANTES]{};
    int cantidadSpawns = 0;

    for (int i = 0; i < cantidadMaxima; i++)
    {
        if (!resultado.participantes[i].participo) continue;

        bool esSolo = i == indiceSolo;
        resultado.participantes[i].numeroEquipo = esSolo ? 0 : 1;

        if (esSolo)
        {
            // El operador no pisa la arena: maneja los taladros desde su cabina.
            ConfigurarJugadorMinijuegoEstandar(jugadores[i], { 0.0f, 3.72f, -PARED_Z_PINCHOS - 1.0f });
            jugadores[i].direccionMirada = { 0.0f, 0.0f, 1.0f };
            jugadores[i].enSuelo = true;
            jugadores[i].cayendo = false;
            continue;
        }

        // Aparicion aleatoria en una celda libre, lejos de las coberturas y
        // de los demas para que nadie nazca encima de otro.
        Vector3 mejor = { -3.0f, 0.72f, 2.0f };
        for (int intento = 0; intento < 120; intento++)
        {
            int celda = GetRandomValue(0, CELDAS_TOTALES_PINCHOS - 1);
            if (bloqueada[celda] || (!celdaComoda[celda] && intento < 100)) continue;

            Vector3 centro = CentroCeldaPinchos(celda);
            bool libre = true;
            for (int k = 0; k < cantidadSpawns; k++)
            {
                if (DistanciaCuadradaXZ(centro, spawnsUsados[k]) < 2.2f * 2.2f) libre = false;
            }

            if (!libre) continue;

            mejor = { centro.x, 0.72f, centro.z };
            break;
        }

        spawnsUsados[cantidadSpawns++] = mejor;
        ConfigurarJugadorMinijuegoEstandar(jugadores[i], mejor);
        jugadores[i].direccionMirada = { 0.0f, 0.0f, -1.0f };
        cerebros[i].ultimoX = mejor.x;
        cerebros[i].ultimoZ = mejor.z;
    }
}


void MinijuegoRefugioPinchos::Actualizar(
    float deltaTime,
    JugadorPrueba jugadores[],
    int cantidadMaxima,
    Participante participantes[],
    ParticulaTierra particulas[],
    int cantidadParticulas
)
{
    if (resultado.estado == RESULTADO_MINIJUEGO_CANCELADO) return;

    ActualizarParticulasPinchos(*this, deltaTime);
    if (fase == FASE_PINCHOS_TERMINADO) return;

    tiempoPulso += deltaTime;
    ActualizarAmbientePinchos(*this, deltaTime);

    if (fase == FASE_PINCHOS_PREPARACION)
    {
        float preparacionAntes = tiempoPreparacion;
        tiempoPreparacion -= deltaTime;
        ActualizarAudioCuentaRegresiva(audio, preparacionAntes, tiempoPreparacion);

        for (int i = 0; i < cantidadMaxima; i++)
        {
            if (i == indiceSolo || !resultado.participantes[i].participo) continue;

            InputMinijuegoParticipante quieto{};
            ActualizarJugadorPruebaNormal(
                jugadores[i], quieto, bloques, cantidadBloques,
                particulas, cantidadParticulas, false, false, deltaTime
            );
            LimitarJugadorSalaPinchos(jugadores[i]);
        }

        if (tiempoPreparacion <= 0.0f)
        {
            tiempoPreparacion = 0.0f;
            fase = FASE_PINCHOS_ESPERANDO;
            tiempoEleccion = 0.0f;
        }

        return;
    }

    float restanteAntes = tiempoRestante;
    tiempoRestante -= deltaTime;
    if (tiempoRestante < 0.0f) tiempoRestante = 0.0f;
    ActualizarAudioAlertaTiempo(audio, restanteAntes, tiempoRestante);
    if (tiempoRestante <= 0.0f) tiempoAgotado = true;

    if (cooldownAtaque > 0.0f)
    {
        cooldownAtaque -= deltaTime;
        if (cooldownAtaque < 0.0f) cooldownAtaque = 0.0f;
    }

    for (int i = 0; i < cantidadMaxima; i++)
    {
        if (!resultado.participantes[i].participo || i == indiceSolo) continue;

        if (estadosJugadores[i].eliminado)
        {
            estadosJugadores[i].tiempoDesdeEliminacion += deltaTime;
            continue;
        }

        InputMinijuegoParticipante entrada{};

        if (ControlaIAPinchos(participantes[i]))
            entrada = CrearEntradaBotEquipoPinchos(*this, i, jugadores, cantidadMaxima, deltaTime);
        else
            entrada = LeerInputMinijuegoParticipante(participantes[i]);

        entrada.saltar = false;
        entrada.golpear = false;

        ActualizarJugadorPruebaNormal(
            jugadores[i], entrada, bloques, cantidadBloques,
            particulas, cantidadParticulas, false, false, deltaTime
        );

        LimitarJugadorSalaPinchos(jugadores[i]);
    }

    if (fase == FASE_PINCHOS_ESPERANDO)
    {
        if (tiempoAgotado)
        {
            FinalizarRefugioPinchos(*this, ContarRivalesVivosPinchos(*this) <= 0);
            return;
        }

        if (cooldownAtaque <= 0.0f)
        {
            tiempoEleccion += deltaTime;

            DireccionPinchos eleccion = ultimaDireccion;
            bool iniciar = false;

            if (ControlaIAPinchos(participantes[indiceSolo]))
            {
                decisionBotSolo -= deltaTime;

                if (decisionBotSolo <= 0.0f)
                {
                    eleccion = ElegirLadoBotSoloPinchos(*this, jugadores, cantidadMaxima);
                    iniciar = true;
                }
            }
            else
            {
                iniciar = LeerDireccionHumanaPinchos(participantes[indiceSolo], eleccion);

                // Si el operador humano duda demasiado, la maquina elige sola
                // para que la ronda nunca se quede detenida.
                if (!iniciar && tiempoEleccion >= ESPERA_AUTOMATICA_PINCHOS)
                {
                    eleccion = ElegirLadoBotSoloPinchos(*this, jugadores, cantidadMaxima);
                    iniciar = true;
                }
            }

            if (iniciar)
            {
                IniciarPatronPinchos(*this, eleccion);
                ReproducirSonidoMinijuego(audio, SONIDO_BOTON);
            }
        }

        return;
    }

    // Aviso / ataque: el patron avanza sobre su propia linea de tiempo.
    tiempoPatron += deltaTime;

    float primerDisparo = patron.salvas[0].disparo;
    for (int i = 1; i < patron.cantidad; i++)
    {
        if (patron.salvas[i].disparo < primerDisparo) primerDisparo = patron.salvas[i].disparo;
    }

    fase = tiempoPatron < primerDisparo ? FASE_PINCHOS_AVISO : FASE_PINCHOS_ATAQUE;

    ActualizarEfectosPatronPinchos(*this, deltaTime);
    ResolverImpactosTaladros(*this, jugadores, cantidadMaxima, participantes);

    if (ContarRivalesVivosPinchos(*this) <= 0)
    {
        FinalizarRefugioPinchos(*this, true);
        return;
    }

    if (tiempoPatron >= patron.duracion)
    {
        patronActivo = false;
        fase = FASE_PINCHOS_ESPERANDO;
        ataquesRealizados++;

        // La muerte subita acorta la pausa entre patrones.
        float intensidad = IntensidadPinchos(*this);
        cooldownAtaque =
            COOLDOWN_INICIAL_PINCHOS + (COOLDOWN_FINAL_PINCHOS - COOLDOWN_INICIAL_PINCHOS) * intensidad;
        tiempoEleccion = 0.0f;
        decisionBotSolo = AzarPinchos(0.10f, 0.45f) * (1.0f - 0.6f * intensidad);
    }
}


//==================================================
// VISUAL: HERRAMIENTAS DE DIBUJO
//==================================================
// MODELO FUTURO: todo lo que sigue es decoracion con primitivas y puede
// sustituirse por GLB sin tocar la logica de arriba: muros de roca y
// portales, bancos de taladros laterales, maquinas de los portales, taladros
// (carcasa + broca), cabina del operador, coberturas de roca con cristales,
// columnas, estalactitas, antorchas, vagoneta/cajas del primer plano.

static Color TinteColorPinchos(Color c, float f)
{
    float r = c.r * f;
    float g = c.g * f;
    float b = c.b * f;
    if (r > 255.0f) r = 255.0f;
    if (g > 255.0f) g = 255.0f;
    if (b > 255.0f) b = 255.0f;
    return Color{ (unsigned char)r, (unsigned char)g, (unsigned char)b, c.a };
}


static Color MezclaColorPinchos(Color a, Color b, float t)
{
    t = LimitarPinchos(t, 0.0f, 1.0f);
    return Color
    {
        (unsigned char)(a.r + (b.r - a.r) * t),
        (unsigned char)(a.g + (b.g - a.g) * t),
        (unsigned char)(a.b + (b.b - a.b) * t),
        (unsigned char)(a.a + (b.a - a.a) * t)
    };
}


static Color ColorRocaPinchos(int semilla, float brillo)
{
    float v = 0.82f + 0.36f * Hash01Pinchos(semilla, 3);
    Color base = { 70, 60, 56, 255 };
    if (Hash01Pinchos(semilla, 9) > 0.7f) base = { 70, 70, 74, 255 };
    return TinteColorPinchos(base, v * brillo);
}


static void CulloPinchos(bool activo)
{
    rlDrawRenderBatchActive();
    if (activo) rlEnableBackfaceCulling();
    else rlDisableBackfaceCulling();
}


// Caja con cada cara a un brillo distinto (luz desde arriba y de frente):
// es lo que da volumen a un bloque sin usar iluminacion real.
static void CajaFacetadaPinchos(Vector3 centro, Vector3 tamano, Color color, float brilloTapa)
{
    float x0 = centro.x - tamano.x * 0.5f;
    float x1 = centro.x + tamano.x * 0.5f;
    float y0 = centro.y - tamano.y * 0.5f;
    float y1 = centro.y + tamano.y * 0.5f;
    float z0 = centro.z - tamano.z * 0.5f;
    float z1 = centro.z + tamano.z * 0.5f;

    Color tapa = TinteColorPinchos(color, brilloTapa);
    Color frente = TinteColorPinchos(color, 1.0f);
    Color atras = TinteColorPinchos(color, 0.58f);
    Color derecha = TinteColorPinchos(color, 0.80f);
    Color izquierda = TinteColorPinchos(color, 0.64f);

    DrawTriangle3D({ x0, y1, z0 }, { x0, y1, z1 }, { x1, y1, z1 }, tapa);
    DrawTriangle3D({ x0, y1, z0 }, { x1, y1, z1 }, { x1, y1, z0 }, tapa);
    DrawTriangle3D({ x0, y0, z1 }, { x1, y0, z1 }, { x1, y1, z1 }, frente);
    DrawTriangle3D({ x0, y0, z1 }, { x1, y1, z1 }, { x0, y1, z1 }, frente);
    DrawTriangle3D({ x0, y0, z0 }, { x0, y1, z0 }, { x1, y1, z0 }, atras);
    DrawTriangle3D({ x0, y0, z0 }, { x1, y1, z0 }, { x1, y0, z0 }, atras);
    DrawTriangle3D({ x1, y0, z0 }, { x1, y1, z0 }, { x1, y1, z1 }, derecha);
    DrawTriangle3D({ x1, y0, z0 }, { x1, y1, z1 }, { x1, y0, z1 }, derecha);
    DrawTriangle3D({ x0, y0, z0 }, { x0, y0, z1 }, { x0, y1, z1 }, izquierda);
    DrawTriangle3D({ x0, y0, z0 }, { x0, y1, z1 }, { x0, y1, z0 }, izquierda);
}


// Bloque de roca facetado con borde oscuro.
static void CajaRocaPinchos(Vector3 centro, Vector3 tamano, Color color)
{
    CajaFacetadaPinchos(centro, tamano, color, 1.30f);
    DrawCubeWiresV(centro, tamano, Fade(BLACK, 0.28f));
}


// Roca irregular: bloque girado y achatado, mas organico que un bloque recto.
static void RocaGiradaPinchos(Vector3 centro, Vector3 tamano, float angulo, Color color)
{
    rlPushMatrix();
    rlTranslatef(centro.x, centro.y, centro.z);
    rlRotatef(angulo, 0.0f, 1.0f, 0.0f);
    CajaFacetadaPinchos({ 0.0f, 0.0f, 0.0f }, tamano, color, 1.30f);
    rlPopMatrix();
}


// Rectangulo horizontal con la normal hacia arriba.
static void RectSueloPinchos(float x0, float z0, float x1, float z1, float y, Color color)
{
    if (x1 < x0) { float t = x0; x0 = x1; x1 = t; }
    if (z1 < z0) { float t = z0; z0 = z1; z1 = t; }

    DrawTriangle3D({ x0, y, z0 }, { x0, y, z1 }, { x1, y, z1 }, color);
    DrawTriangle3D({ x0, y, z0 }, { x1, y, z1 }, { x1, y, z0 }, color);
}


// Disco de luz suave (centro opaco, borde transparente). Solo con mezcla
// aditiva y sin escribir profundidad (ver DibujarResplandoresPinchos).
static void ResplandorPinchos(Vector3 centro, Vector3 u, Vector3 v, float radio, Color color)
{
    const int LADOS = 14;
    rlBegin(RL_TRIANGLES);

    for (int k = 0; k < LADOS; k++)
    {
        float a0 = 6.2832f * (float)k / (float)LADOS;
        float a1 = 6.2832f * (float)(k + 1) / (float)LADOS;
        float c0 = std::cos(a0) * radio, s0 = std::sin(a0) * radio;
        float c1 = std::cos(a1) * radio, s1 = std::sin(a1) * radio;

        rlColor4ub(color.r, color.g, color.b, color.a);
        rlVertex3f(centro.x, centro.y, centro.z);
        rlColor4ub(color.r, color.g, color.b, 0);
        rlVertex3f(centro.x + u.x * c0 + v.x * s0, centro.y + u.y * c0 + v.y * s0, centro.z + u.z * c0 + v.z * s0);
        rlColor4ub(color.r, color.g, color.b, 0);
        rlVertex3f(centro.x + u.x * c1 + v.x * s1, centro.y + u.y * c1 + v.y * s1, centro.z + u.z * c1 + v.z * s1);
    }

    rlEnd();
}


static void ResplandorSueloPinchos(float x, float z, float radio, Color color)
{
    ResplandorPinchos({ x, 0.045f, z }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, radio, color);
}


static void ResplandorMuroPinchos(Vector3 centro, bool paredEnZ, float radio, Color color)
{
    if (paredEnZ)
        ResplandorPinchos(centro, { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, radio, color);
    else
        ResplandorPinchos(centro, { 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f, 0.0f }, radio, color);
}


static bool ParpadeoRojoPinchos(const MinijuegoRefugioPinchos& m)
{
    return std::sin(m.tiempoPulso * 20.0f) > 0.0f;
}


//==================================================
// VISUAL: SUELO
//==================================================

static void DibujarSueloPinchos()
{
    const Color juntas = { 30, 26, 26, 255 };

    DrawCubeV({ 0.0f, -0.46f, 0.0f }, { PARED_X_PINCHOS * 2.0f + 1.2f, 0.92f, PARED_Z_PINCHOS * 2.0f + 1.2f }, juntas);

    const int NX = COLUMNAS_PINCHOS * 2;
    const int NZ = FILAS_PINCHOS * 2;
    const float sx = PARED_X_PINCHOS * 2.0f / (float)NX;
    const float sz = PARED_Z_PINCHOS * 2.0f / (float)NZ;

    for (int iz = 0; iz < NZ; iz++)
    {
        for (int ix = 0; ix < NX; ix++)
        {
            float x = -PARED_X_PINCHOS + ((float)ix + 0.5f) * sx;
            float z = -PARED_Z_PINCHOS + ((float)iz + 0.5f) * sz;
            float altura = 0.004f + Hash01Pinchos(ix, iz) * 0.026f;
            float nx = x / PARED_X_PINCHOS;
            float nz = z / PARED_Z_PINCHOS;
            float oscurecimiento = 1.0f - 0.30f * (nx * nx * 0.6f + nz * nz * 0.8f);
            float variacion = 0.82f + 0.30f * Hash01Pinchos(ix + 31, iz + 7);
            Color color = TinteColorPinchos(Color{ 112, 96, 80, 255 }, variacion * oscurecimiento);

            DrawCubeV({ x, altura - 0.07f, z }, { sx - 0.06f, 0.14f, sz - 0.06f }, color);
        }
    }

    // Grietas: rompen la regularidad de las losas.
    const Color grieta = { 38, 32, 30, 255 };
    DrawLine3D({ -4.6f, 0.04f, -1.4f }, { -3.7f, 0.04f, -1.0f }, grieta);
    DrawLine3D({ -3.7f, 0.04f, -1.0f }, { -3.2f, 0.04f, -0.3f }, grieta);
    DrawLine3D({ 1.9f, 0.04f, 3.3f }, { 2.6f, 0.04f, 2.8f }, grieta);
    DrawLine3D({ 2.6f, 0.04f, 2.8f }, { 3.0f, 0.04f, 2.0f }, grieta);
    DrawLine3D({ 0.5f, 0.04f, -3.2f }, { 1.1f, 0.04f, -2.5f }, grieta);
    DrawLine3D({ -2.2f, 0.04f, 2.2f }, { -1.5f, 0.04f, 2.9f }, grieta);
}


// Sombras de contacto en la base de los muros: rompen la planitud del suelo.
static void DibujarBordesSueloPinchos()
{
    const Color sombra = Fade(BLACK, 0.34f);
    const float ancho = 0.45f;

    RectSueloPinchos(-PARED_X_PINCHOS, -PARED_Z_PINCHOS, PARED_X_PINCHOS, -PARED_Z_PINCHOS + ancho, 0.038f, sombra);
    RectSueloPinchos(-PARED_X_PINCHOS, PARED_Z_PINCHOS - ancho, PARED_X_PINCHOS, PARED_Z_PINCHOS, 0.038f, sombra);
    RectSueloPinchos(-PARED_X_PINCHOS, -PARED_Z_PINCHOS, -PARED_X_PINCHOS + ancho, PARED_Z_PINCHOS, 0.038f, sombra);
    RectSueloPinchos(PARED_X_PINCHOS - ancho, -PARED_Z_PINCHOS, PARED_X_PINCHOS, PARED_Z_PINCHOS, 0.038f, sombra);
}


// Placas amarillas y negras delante de cada boca: marcan DONDE nace el
// peligro aunque todavia no haya aviso. Quedan dentro de la franja letal.
static void DibujarPlacasBocasPinchos()
{
    for (int d = 0; d < 4; d++)
    {
        DireccionPinchos lado = (DireccionPinchos)d;

        for (int c = 0; c < CantidadCarrilesPinchos(lado); c++)
        {
            Vector3 origen{};
            Vector3 avance{};
            ObtenerOrigenTaladroPinchos(lado, c, origen, avance);

            const float mitad = CARRIL_PINCHOS * 0.5f - 0.05f;

            for (int b = 0; b < 3; b++)
            {
                float t = 0.10f + (float)b * 0.15f;
                Color color = b % 2 == 0 ? Color{ 214, 170, 40, 255 } : Color{ 34, 30, 28, 255 };
                float ancho = 0.10f;

                if (EsEjeZPinchos(lado))
                {
                    float z = origen.z + avance.z * t;
                    RectSueloPinchos(origen.x - mitad, z - ancho * 0.5f, origen.x + mitad, z + ancho * 0.5f, 0.040f, color);
                }
                else
                {
                    float x = origen.x + avance.x * t;
                    RectSueloPinchos(x - ancho * 0.5f, origen.z - mitad, x + ancho * 0.5f, origen.z + mitad, 0.040f, color);
                }
            }
        }
    }
}


//==================================================
// VISUAL: LAMPARAS Y TORCHAS
//==================================================

static void DibujarLamparaPinchos(const MinijuegoRefugioPinchos& m, Vector3 pos, bool activa)
{
    Color bombilla = { 120, 86, 40, 255 };

    if (activa)
        bombilla = ParpadeoRojoPinchos(m) || m.fase == FASE_PINCHOS_ATAQUE
            ? Color{ 255, 56, 36, 255 }
            : Color{ 130, 18, 14, 255 };

    DrawCubeV(pos, { 0.30f, 0.30f, 0.30f }, Color{ 30, 30, 34, 255 });
    DrawCubeV({ pos.x, pos.y + 0.19f, pos.z }, { 0.38f, 0.07f, 0.38f }, Color{ 54, 56, 62, 255 });
    DrawSphereEx(pos, 0.21f, 7, 8, bombilla);
}


static void DibujarAntorchaPinchos(Vector3 base, int semilla)
{
    float t = (float)GetTime();
    float parpadeo = 0.85f + 0.15f * std::sin(t * 11.0f + (float)semilla * 1.7f) + 0.06f * std::sin(t * 23.0f + (float)semilla);

    DrawCylinderEx(base, { base.x, base.y + 0.55f, base.z }, 0.05f, 0.065f, 6, Color{ 96, 68, 42, 255 });
    DrawCylinderEx(
        { base.x, base.y + 0.50f, base.z }, { base.x, base.y + 0.66f, base.z },
        0.10f, 0.15f, 8, Color{ 52, 50, 54, 255 }
    );
    DrawCylinderEx(
        { base.x, base.y + 0.60f, base.z }, { base.x, base.y + 0.60f + 0.42f * parpadeo, base.z },
        0.13f, 0.0f, 7, Color{ 255, 130, 36, 255 }
    );
    DrawCylinderEx(
        { base.x, base.y + 0.62f, base.z }, { base.x, base.y + 0.62f + 0.26f * parpadeo, base.z },
        0.075f, 0.0f, 6, Color{ 255, 220, 110, 255 }
    );
}


//==================================================
// VISUAL: MUROS, PORTALES Y MAQUINARIA
//==================================================

static void DibujarBocaTaladroPinchos(
    const MinijuegoRefugioPinchos& m,
    DireccionPinchos direccion,
    int carril,
    bool conCarcasa
)
{
    Vector3 origen{};
    Vector3 avance{};
    ObtenerOrigenTaladroPinchos(direccion, carril, origen, avance);

    auto punto = [&](float s) -> Vector3
    {
        return { origen.x + avance.x * s, origen.y, origen.z + avance.z * s };
    };

    bool activa = CarrilActivoPinchos(m, direccion, carril);
    Color aro = activa ? Color{ 214, 170, 40, 255 } : Color{ 108, 112, 122, 255 };

    if (conCarcasa)
    {
        Vector3 centro = punto(-0.68f);
        bool ejeZ = EsEjeZPinchos(direccion);
        const float ancho = CARRIL_PINCHOS - 0.10f;
        Vector3 tam = ejeZ ? Vector3{ ancho, 1.20f, 0.74f } : Vector3{ 0.74f, 1.20f, ancho };
        CajaFacetadaPinchos(centro, tam, Color{ 66, 72, 84, 255 }, 1.25f);
        DrawCubeWiresV(centro, tam, Fade(BLACK, 0.5f));
        // Franja de peligro sobre la carcasa (se ve desde la camara).
        Vector3 franja = { centro.x, centro.y + tam.y * 0.5f + 0.01f, centro.z };
        Vector3 tamFranja = ejeZ ? Vector3{ ancho * 0.75f, 0.02f, 0.16f } : Vector3{ 0.16f, 0.02f, ancho * 0.75f };
        DrawCubeV(franja, tamFranja, Color{ 214, 170, 40, 255 });
    }

    DrawCylinderEx(punto(-0.32f), punto(0.04f), 0.58f, 0.54f, 16, Color{ 74, 80, 92, 255 });
    DrawCylinderEx(punto(0.03f), punto(0.07f), 0.60f, 0.60f, 16, aro);
    DrawCylinderEx(punto(0.065f), punto(0.075f), 0.44f, 0.44f, 14, Color{ 8, 8, 10, 255 });
}


static void DibujarMuroPortalesPinchos(const MinijuegoRefugioPinchos& m, bool lejano)
{
    const float s = lejano ? -1.0f : 1.0f;
    const float alto = lejano ? 2.75f : 0.85f;
    const float zc = s * (PARED_Z_PINCHOS + 0.55f);
    const DireccionPinchos dir = lejano ? PINCHOS_DESDE_ARRIBA : PINCHOS_DESDE_ABAJO;
    const float extra = 2.4f;

    // Macizos de roca a los lados de la hilera de taladros.
    for (int lado = -1; lado <= 1; lado += 2)
    {
        float cx = (float)lado * (PARED_X_PINCHOS + extra * 0.5f);
        float variacion = 0.10f * Hash01Pinchos(lado + 5, lejano ? 1 : 2);
        CajaRocaPinchos(
            { cx, (alto + variacion) * 0.5f, zc },
            { extra, alto + variacion, 1.10f },
            ColorRocaPinchos(lado + (lejano ? 0 : 20), 1.0f)
        );
    }

    for (int c = 0; c < COLUMNAS_PINCHOS; c++)
    {
        float cx = CentroCarrilPinchos(dir, c);

        if (lejano)
        {
            // Fondo oscuro del tunel.
            DrawCubeV({ cx, 0.85f, s * (PARED_Z_PINCHOS + 1.05f) }, { CARRIL_PINCHOS - 0.10f, 1.7f, 0.1f }, Color{ 10, 9, 11, 255 });

            // Dintel de roca y viga de madera.
            CajaRocaPinchos(
                { cx, (1.75f + alto) * 0.5f, zc },
                { CARRIL_PINCHOS, alto - 1.75f, 1.10f },
                ColorRocaPinchos(c + 70, 0.9f)
            );
            DrawCubeV({ cx, 1.70f, s * (PARED_Z_PINCHOS + 0.04f) }, { CARRIL_PINCHOS + 0.04f, 0.16f, 0.24f }, Color{ 102, 72, 44, 255 });
            DibujarLamparaPinchos(m, { cx, 2.05f, s * (PARED_Z_PINCHOS - 0.04f) }, CarrilActivoPinchos(m, dir, c));
        }
        else
        {
            DibujarLamparaPinchos(m, { cx, 1.50f, s * (PARED_Z_PINCHOS + 0.66f) }, CarrilActivoPinchos(m, dir, c));
        }

        DibujarBocaTaladroPinchos(m, dir, c, true);
    }

    // Postes de madera entre taladros (solo en el muro alto).
    if (lejano)
    {
        for (int k = 0; k <= COLUMNAS_PINCHOS; k++)
        {
            float x = -PARED_X_PINCHOS + (float)k * CARRIL_PINCHOS;
            DrawCubeV({ x, 0.85f, s * (PARED_Z_PINCHOS + 0.06f) }, { 0.12f, 1.70f, 0.22f }, Color{ 90, 64, 40, 255 });
        }

        // Estratos: lineas de roca sedimentaria.
        for (int lado = -1; lado <= 1; lado += 2)
        {
            DrawCubeV(
                { (float)lado * (PARED_X_PINCHOS + extra * 0.5f), alto * 0.38f, zc - s * 0.01f },
                { extra + 0.02f, 0.05f, 1.12f },
                ColorRocaPinchos(lado + 50, 0.55f)
            );
        }
    }
}


static void DibujarBancoLateralPinchos(const MinijuegoRefugioPinchos& m, bool izquierda)
{
    const float s = izquierda ? -1.0f : 1.0f;
    DireccionPinchos dir = izquierda ? PINCHOS_DESDE_IZQUIERDA : PINCHOS_DESDE_DERECHA;
    const float xPlaca = s * (PARED_X_PINCHOS + 0.28f);
    const float largo = PARED_Z_PINCHOS * 2.0f + 0.30f;

    // Masa de roca irregular detras y encima del banco.
    CajaRocaPinchos({ s * (PARED_X_PINCHOS + 1.25f), 1.15f, 0.0f }, { 1.5f, 2.3f, largo + 0.9f }, ColorRocaPinchos(izquierda ? 5 : 6, 0.95f));
    for (int k = 0; k < 7; k++)
    {
        int semilla = k + (izquierda ? 100 : 200);
        float z = -4.0f + (float)k * 1.3f + 0.2f * Hash01Pinchos(semilla, 1);
        float alto = 0.7f + 1.3f * Hash01Pinchos(semilla, 2);
        RocaGiradaPinchos(
            { s * (PARED_X_PINCHOS + 0.80f + 0.4f * Hash01Pinchos(semilla, 3)), 2.3f + alto * 0.5f, z },
            { 1.1f + 0.5f * Hash01Pinchos(semilla, 4), alto, 1.3f },
            40.0f * Hash01Pinchos(semilla, 5),
            ColorRocaPinchos(semilla, 1.0f)
        );
    }

    // Placa de acero con los taladros del banco.
    DrawCubeV({ xPlaca, 0.95f, 0.0f }, { 0.56f, 1.90f, largo }, Color{ 56, 62, 72, 255 });
    DrawCubeWiresV({ xPlaca, 0.95f, 0.0f }, { 0.56f, 1.90f, largo }, Fade(BLACK, 0.5f));

    float xFrente = s * (PARED_X_PINCHOS + 0.005f);

    // Banda de peligro amarilla y negra.
    for (int k = 0; k < 18; k++)
    {
        float z = -PARED_Z_PINCHOS + 0.25f + (float)k * 0.5f;
        if (z > PARED_Z_PINCHOS) break;
        Color color = k % 2 == 0 ? Color{ 214, 170, 40, 255 } : Color{ 30, 28, 28, 255 };
        DrawCubeV({ xFrente, 1.62f, z }, { 0.04f, 0.18f, 0.5f }, color);
    }

    // Separadores de panel (uno por limite de carril).
    for (int k = 0; k <= FILAS_PINCHOS; k++)
    {
        float z = -PARED_Z_PINCHOS + (float)k * CARRIL_PINCHOS;
        DrawCubeV({ xFrente, 0.95f, z }, { 0.07f, 1.90f, 0.07f }, Color{ 26, 28, 32, 255 });
    }

    for (int c = 0; c < FILAS_PINCHOS; c++)
    {
        float z = CentroCarrilPinchos(dir, c);
        DibujarBocaTaladroPinchos(m, dir, c, false);
        DibujarLamparaPinchos(m, { s * (PARED_X_PINCHOS - 0.12f), 1.30f, z }, CarrilActivoPinchos(m, dir, c));
    }

    // Maquinaria sobre el banco: depositos y pistones.
    for (int k = 0; k < 3; k++)
    {
        float z = ((float)k - 1.0f) * 2.8f;
        DrawCylinderEx({ xPlaca, 1.90f, z }, { xPlaca, 2.45f, z }, 0.30f, 0.30f, 10, Color{ 70, 76, 86, 255 });
        DrawCylinderEx({ xPlaca, 2.40f, z }, { xPlaca, 2.55f, z }, 0.34f, 0.34f, 10, Color{ 214, 170, 40, 255 });
    }
}


static void DibujarColumnasPinchos()
{
    for (int lado = -1; lado <= 1; lado += 2)
    {
        float x = (float)lado * (PARED_X_PINCHOS + 0.10f);
        float z = -PARED_Z_PINCHOS - 0.05f;
        DrawCylinderEx({ x, 0.0f, z }, { x, 3.20f, z }, 0.60f, 0.46f, 8, ColorRocaPinchos(lado + 40, 1.0f));
        DrawCylinderEx({ x, 0.0f, z }, { x, 0.35f, z }, 0.70f, 0.62f, 8, ColorRocaPinchos(lado + 44, 0.8f));
        DrawCylinderEx({ x, 1.35f, z }, { x, 1.45f, z }, 0.63f, 0.63f, 8, Color{ 48, 40, 36, 255 });
    }
}


//==================================================
// VISUAL: ENTORNO DE LA CUEVA
//==================================================

static void DibujarCuevaExteriorPinchos()
{
    float factor = FactorCalidadGrafica(CalidadDecoracion());

    // Suelo oscuro de la caverna y masa de roca de fondo.
    DrawCubeV({ 0.0f, -1.05f, 0.0f }, { 70.0f, 1.0f, 50.0f }, Color{ 22, 19, 20, 255 });

    int cantidad = (int)(16.0f * factor);
    if (cantidad < 6) cantidad = 6;

    for (int lado = -1; lado <= 1; lado += 2)
    {
        for (int k = 0; k < cantidad; k++)
        {
            int semilla = k * 7 + (lado + 1) * 13;
            float x = (float)lado * (9.0f + 5.5f * Hash01Pinchos(semilla, 1));
            float z = -8.0f + 15.0f * Hash01Pinchos(semilla, 2);
            float alto = 1.2f + 2.6f * Hash01Pinchos(semilla, 3);
            float ancho = 1.5f + 2.6f * Hash01Pinchos(semilla, 4);
            float lejania = 1.0f - 0.35f * (std::fabs(x) - 8.0f) / 6.0f;

            RocaGiradaPinchos(
                { x, alto * 0.5f - 0.5f, z },
                { ancho, alto, ancho * (0.7f + 0.6f * Hash01Pinchos(semilla, 5)) },
                360.0f * Hash01Pinchos(semilla, 6),
                ColorRocaPinchos(semilla, 0.78f * lejania)
            );
        }
    }

    for (int k = 0; k < cantidad + 4; k++)
    {
        int semilla = k * 11 + 400;
        float x = -13.0f + 26.0f * Hash01Pinchos(semilla, 1);
        float z = -11.5f + 3.5f * Hash01Pinchos(semilla, 2);
        float alto = 2.0f + 3.4f * Hash01Pinchos(semilla, 3);
        float ancho = 1.8f + 2.8f * Hash01Pinchos(semilla, 4);

        RocaGiradaPinchos(
            { x, alto * 0.5f - 0.5f, z },
            { ancho, alto, ancho * 0.9f },
            360.0f * Hash01Pinchos(semilla, 6),
            ColorRocaPinchos(semilla, 0.62f)
        );
    }

    // Estalactitas colgando de la cornisa del fondo y de los flancos.
    int estalactitas = (int)(12.0f * factor);
    for (int k = 0; k < estalactitas; k++)
    {
        float x = -8.5f + 17.0f * ((float)k / 11.0f);
        float z = -8.2f + 1.0f * Hash01Pinchos(k, 55);
        if (std::fabs(x) < 2.0f) continue;
        float largo = 0.8f + 1.3f * Hash01Pinchos(k, 56);
        DrawCylinderEx({ x, 5.4f, z }, { x, 5.4f - largo, z }, 0.34f, 0.0f, 6, ColorRocaPinchos(k + 80, 0.9f));
    }

    // Estalagmitas en los flancos.
    int estalagmitas = (int)(10.0f * factor);
    for (int k = 0; k < estalagmitas; k++)
    {
        float lado = k % 2 == 0 ? -1.0f : 1.0f;
        float x = lado * (7.6f + 3.5f * Hash01Pinchos(k, 71));
        float z = -4.0f + 8.5f * Hash01Pinchos(k, 72);
        float alto = 0.9f + 1.4f * Hash01Pinchos(k, 73);
        DrawCylinderEx({ x, -0.5f, z }, { x, -0.5f + alto, z }, 0.42f, 0.0f, 6, ColorRocaPinchos(k + 90, 0.85f));
    }

    // Cristales de colores: iluminan las rocas del entorno.
    for (int k = 0; k < 4; k++)
    {
        const CristalAmbientePinchos& cristal = CRISTALES_AMBIENTE_PINCHOS[k];

        for (int p = 0; p < 3; p++)
        {
            float dx = ((float)p - 1.0f) * 0.30f;
            float alto = cristal.alto * (p == 1 ? 1.0f : 0.62f);
            Color cuerpo = p == 1 ? TinteColorPinchos(cristal.color, 1.15f) : cristal.color;
            DrawCylinderEx({ cristal.x + dx, cristal.y, cristal.z + 0.1f * (float)p }, { cristal.x + dx * 1.4f, cristal.y + alto, cristal.z + 0.1f * (float)p }, 0.22f, 0.0f, 5, cuerpo);
        }
    }
}


static void DibujarPrimerPlanoPinchos()
{
    float factor = FactorCalidadGrafica(CalidadDecoracion());
    const float zr = PARED_Z_PINCHOS + 2.7f;

    // Rieles y vagoneta minera delante del muro frontal.
    for (int k = -11; k <= 11; k++)
    {
        DrawCubeV({ (float)k * 0.9f, 0.02f, zr }, { 0.22f, 0.07f, 1.35f }, Color{ 90, 66, 42, 255 });
    }
    DrawCubeV({ 0.0f, 0.09f, zr - 0.42f }, { 21.0f, 0.07f, 0.09f }, Color{ 120, 124, 132, 255 });
    DrawCubeV({ 0.0f, 0.09f, zr + 0.42f }, { 21.0f, 0.07f, 0.09f }, Color{ 120, 124, 132, 255 });

    // Vagoneta.
    DrawCubeV({ -6.2f, 0.62f, zr }, { 1.9f, 0.9f, 1.1f }, Color{ 70, 62, 58, 255 });
    DrawCubeWiresV({ -6.2f, 0.62f, zr }, { 1.9f, 0.9f, 1.1f }, Fade(BLACK, 0.5f));
    DrawCubeV({ -6.2f, 1.12f, zr }, { 1.6f, 0.2f, 0.85f }, Color{ 112, 98, 84, 255 });
    DrawCylinderEx({ -6.9f, 0.22f, zr - 0.58f }, { -6.9f, 0.22f, zr + 0.58f }, 0.26f, 0.26f, 8, Color{ 34, 34, 38, 255 });
    DrawCylinderEx({ -5.5f, 0.22f, zr - 0.58f }, { -5.5f, 0.22f, zr + 0.58f }, 0.26f, 0.26f, 8, Color{ 34, 34, 38, 255 });

    if (factor < 0.5f) return;

    // Cajas y barriles.
    DrawCubeV({ 6.0f, 0.40f, zr - 0.8f }, { 0.8f, 0.8f, 0.8f }, Color{ 112, 80, 48, 255 });
    DrawCubeWiresV({ 6.0f, 0.40f, zr - 0.8f }, { 0.8f, 0.8f, 0.8f }, Fade(BLACK, 0.5f));
    DrawCubeV({ 6.9f, 0.40f, zr - 0.6f }, { 0.8f, 0.8f, 0.8f }, Color{ 100, 72, 44, 255 });
    DrawCubeWiresV({ 6.9f, 0.40f, zr - 0.6f }, { 0.8f, 0.8f, 0.8f }, Fade(BLACK, 0.5f));
    DrawCubeV({ 6.4f, 1.15f, zr - 0.7f }, { 0.7f, 0.7f, 0.7f }, Color{ 118, 86, 52, 255 });
    DrawCylinderEx({ 4.9f, 0.0f, zr - 0.5f }, { 4.9f, 0.85f, zr - 0.5f }, 0.36f, 0.36f, 10, Color{ 76, 56, 40, 255 });
    DrawCylinderEx({ 4.9f, 0.55f, zr - 0.5f }, { 4.9f, 0.62f, zr - 0.5f }, 0.39f, 0.39f, 10, Color{ 40, 38, 40, 255 });

    // Picos apoyados.
    DrawCylinderEx({ -3.4f, 0.0f, zr - 0.8f }, { -3.0f, 1.3f, zr - 0.7f }, 0.04f, 0.04f, 6, Color{ 96, 68, 42, 255 });
    DrawCubeV({ -2.95f, 1.32f, zr - 0.7f }, { 0.5f, 0.08f, 0.08f }, Color{ 130, 134, 142, 255 });
}


//==================================================
// VISUAL: COBERTURAS
//==================================================
// El dibujo no sobresale del bloque de colision (lo que ves es lo que
// bloquea al taladro y al jugador).

static void DibujarCoberturaPinchos(const BloquePrueba& b, int indice)
{
    const float x = b.posicion.x;
    const float z = b.posicion.z;
    const float alto = b.tamano.y;
    const float hx = b.tamano.x * 0.5f;
    const float hz = b.tamano.z * 0.5f;
    float t = (float)GetTime();

    // Sombra de contacto y aro de luz cian pegado a la base.
    RectSueloPinchos(x - hx - 0.14f, z - hz - 0.14f, x + hx + 0.22f, z + hz + 0.22f, 0.036f, Fade(BLACK, 0.30f));

    Color cian = Fade(COLOR_CIAN_REFUGIO, 0.50f + 0.2f * std::sin(t * 3.0f + (float)indice));
    const float g = 0.07f;
    RectSueloPinchos(x - hx - g, z - hz - g, x + hx + g, z - hz, 0.050f, cian);
    RectSueloPinchos(x - hx - g, z + hz, x + hx + g, z + hz + g, 0.050f, cian);
    RectSueloPinchos(x - hx - g, z - hz, x - hx, z + hz, 0.050f, cian);
    RectSueloPinchos(x + hx, z - hz, x + hx + g, z + hz, 0.050f, cian);

    // Cuerpo de roca facetado con estratos.
    Color roca = Color{ 98, 90, 88, 255 };
    CajaFacetadaPinchos({ x, alto * 0.5f, z }, b.tamano, roca, 1.22f);
    DrawCubeWiresV({ x, alto * 0.5f, z }, b.tamano, Fade(BLACK, 0.55f));
    DrawCubeV({ x, 0.40f, z }, { b.tamano.x + 0.03f, 0.06f, b.tamano.z + 0.03f }, Color{ 62, 54, 52, 255 });
    DrawCubeV({ x, 0.82f, z }, { b.tamano.x + 0.03f, 0.06f, b.tamano.z + 0.03f }, Color{ 66, 58, 56, 255 });
    DrawCubeV({ x, alto + 0.01f, z }, { b.tamano.x * 0.94f, 0.04f, b.tamano.z * 0.94f }, Color{ 140, 130, 120, 255 });

    // Cristales azules sobre la cumbre: identifican la cobertura.
    const float cx[4] = { -0.22f, 0.22f, 0.02f, -0.30f };
    const float cz[4] = { -0.10f, 0.14f, -0.24f, 0.22f };
    const float ch[4] = { 0.62f, 0.46f, 0.80f, 0.34f };
    float extX = hx - 0.28f;
    float extZ = hz - 0.28f;
    int copias = (b.tamano.x > CARRIL_PINCHOS * 1.5f || b.tamano.z > CARRIL_PINCHOS * 1.5f) ? 2 : 1;

    for (int c = 0; c < copias; c++)
    {
        float sx = 0.0f;
        float sz = 0.0f;

        if (copias == 2)
        {
            float desplazamiento = c == 0 ? -0.5f : 0.5f;
            if (b.tamano.x >= b.tamano.z) sx = desplazamiento * (hx - 0.35f) * 1.4f;
            else sz = desplazamiento * (hz - 0.35f) * 1.4f;
        }

        for (int k = 0; k < 4; k++)
        {
            float px = LimitarPinchos(cx[k] + sx, -extX, extX);
            float pz = LimitarPinchos(cz[k] + sz, -extZ, extZ);
            Vector3 base = { x + px, alto + 0.04f, z + pz };
            Color cuerpo = k % 2 == 0 ? Color{ 56, 206, 224, 255 } : Color{ 90, 232, 240, 255 };
            DrawCylinderEx(base, { base.x + px * 0.2f, base.y + ch[k], base.z + pz * 0.2f }, 0.14f, 0.0f, 5, cuerpo);
            DrawCylinderEx(
                { base.x, base.y - 0.02f, base.z }, { base.x, base.y + ch[k] * 0.35f, base.z },
                0.09f, 0.12f, 5, Color{ 180, 250, 255, 255 }
            );
        }
    }
}


//==================================================
// VISUAL: TELEGRAFIA EN EL SUELO
//==================================================
// Cada franja es la zona letal real de un carril: el MISMO rectangulo que
// usa el calculo de impactos (alcance desde la boca hasta la primera
// cobertura). Tras una cobertura se tine de cian la zona que queda a salvo.

static void FranjaSueloPinchos(
    DireccionPinchos lado,
    int carril,
    float desde,
    float hasta,
    float margenLateral,
    float y,
    Color color
)
{
    Vector3 origen{};
    Vector3 avance{};
    ObtenerOrigenTaladroPinchos(lado, carril, origen, avance);

    float a = CentroCarrilPinchos(lado, carril) - CARRIL_PINCHOS * 0.5f + margenLateral;
    float b = CentroCarrilPinchos(lado, carril) + CARRIL_PINCHOS * 0.5f - margenLateral;

    if (EsEjeZPinchos(lado))
        RectSueloPinchos(a, origen.z + avance.z * desde, b, origen.z + avance.z * hasta, y, color);
    else
        RectSueloPinchos(origen.x + avance.x * desde, a, origen.x + avance.x * hasta, b, y, color);
}


static void DibujarFranjasPeligroPinchos(const MinijuegoRefugioPinchos& m)
{
    if (!m.patronActivo) return;

    float pulso = 0.5f + 0.5f * std::sin(m.tiempoPulso * 16.0f);

    // Zona a salvo detras de cada cobertura (debajo de las franjas rojas).
    for (int si = 0; si < m.patron.cantidad; si++)
    {
        const SalvaPinchos& s = m.patron.salvas[si];
        float total = EsEjeZPinchos(s.lado) ? PARED_Z_PINCHOS * 2.0f : PARED_X_PINCHOS * 2.0f;

        for (int c = 0; c < CantidadCarrilesPinchos(s.lado); c++)
        {
            EstadoCarrilPinchos e = EstadoCarrilSalvaPinchos(m, s, c);
            if (!e.activo || !m.chocaCobertura[(int)s.lado][c]) continue;

            float fuerza = e.aviso ? 0.12f + 0.12f * e.progresoAviso : 0.24f * (1.0f - e.desvanece);
            FranjaSueloPinchos(
                s.lado, c, m.alcance[(int)s.lado][c], total, 0.05f, 0.046f,
                Fade(COLOR_CIAN_REFUGIO, fuerza + 0.05f * pulso)
            );
        }
    }

    for (int si = 0; si < m.patron.cantidad; si++)
    {
        const SalvaPinchos& s = m.patron.salvas[si];

        for (int c = 0; c < CantidadCarrilesPinchos(s.lado); c++)
        {
            EstadoCarrilPinchos e = EstadoCarrilSalvaPinchos(m, s, c);
            if (!e.activo) continue;

            float largo = m.alcance[(int)s.lado][c];
            float restante = s.disparo + (float)(s.invertido ? CantidadCarrilesPinchos(s.lado) - 1 - c : c) * s.paso - m.tiempoPatron;
            bool destello = e.aviso && restante < 0.30f;

            float alfa = 0.0f;
            if (e.aviso) alfa = 0.40f + 0.28f * e.progresoAviso + 0.12f * pulso;
            else if (e.letal) alfa = 0.72f;
            else alfa = 0.72f * (1.0f - e.desvanece);
            if (destello) alfa = 0.80f;

            Color relleno = destello ? Color{ 255, 150, 110, 255 } : COLOR_PELIGRO_PINCHOS;

            FranjaSueloPinchos(s.lado, c, 0.0f, largo, 0.0f, 0.049f, Fade(BLACK, 0.50f));
            FranjaSueloPinchos(s.lado, c, 0.0f, largo, 0.0f, 0.052f, Fade(relleno, alfa));

            // Chevrones que avanzan en la direccion del taladro.
            float paso = 0.95f;
            float desplazamiento = std::fmod(m.tiempoPulso * (e.aviso ? 3.2f : 7.0f), paso);
            Color chevron = Fade(Color{ 255, 205, 70, 255 }, (0.55f + 0.3f * pulso) * (e.aviso ? 1.0f : 1.0f - e.desvanece));

            for (float p = 0.2f + desplazamiento; p < largo - 0.1f; p += paso)
                FranjaSueloPinchos(s.lado, c, p - 0.10f, p + 0.10f, 0.12f, 0.058f, chevron);

            // Bordes brillantes (limite exacto del carril) y marca de fin de recorrido.
            Color borde = Fade(Color{ 255, 120, 60, 255 }, 0.9f * (e.aviso ? 1.0f : 1.0f - e.desvanece));

            Vector3 origen{};
            Vector3 avance{};
            ObtenerOrigenTaladroPinchos(s.lado, c, origen, avance);
            float a = CentroCarrilPinchos(s.lado, c) - CARRIL_PINCHOS * 0.5f;
            float b = CentroCarrilPinchos(s.lado, c) + CARRIL_PINCHOS * 0.5f;
            const float g = 0.06f;

            if (EsEjeZPinchos(s.lado))
            {
                float z0 = origen.z;
                float z1 = origen.z + avance.z * largo;
                RectSueloPinchos(a, z0, a + g, z1, 0.060f, borde);
                RectSueloPinchos(b - g, z0, b, z1, 0.060f, borde);
                RectSueloPinchos(a, z1 - 0.06f * avance.z, b, z1 + 0.06f * avance.z, 0.062f, Color{ 255, 220, 90, 255 });
            }
            else
            {
                float x0 = origen.x;
                float x1 = origen.x + avance.x * largo;
                RectSueloPinchos(x0, a, x1, a + g, 0.060f, borde);
                RectSueloPinchos(x0, b - g, x1, b, 0.060f, borde);
                RectSueloPinchos(x1 - 0.06f * avance.x, a, x1 + 0.06f * avance.x, b, 0.062f, Color{ 255, 220, 90, 255 });
            }
        }
    }
}


//==================================================
// VISUAL: TALADROS
//==================================================

static void DibujarTaladroPinchos(
    const MinijuegoRefugioPinchos& m,
    DireccionPinchos direccion,
    int carril
)
{
    float largo = LargoTaladroPinchos(m, direccion, carril);
    if (largo <= ESCONDITE_RETRAIDO_PINCHOS + 0.02f) return;

    Vector3 origen{};
    Vector3 avance{};
    ObtenerOrigenTaladroPinchos(direccion, carril, origen, avance);

    const Vector3 lateral = { avance.z, 0.0f, -avance.x };
    const bool atacando = m.fase == FASE_PINCHOS_ATAQUE;
    const float R = RADIO_TALADRO_PINCHOS;
    const float tiempo = (float)GetTime();

    auto punto = [&](float s) -> Vector3
    {
        return { origen.x + avance.x * s, origen.y, origen.z + avance.z * s };
    };

    float largoBroca = 1.30f;
    if (largoBroca > largo + 1.0f) largoBroca = largo + 1.0f;
    float base = largo - largoBroca;

    // Carcasa.
    DrawCylinderEx(punto(-1.2f), punto(base), R, R, 14, Color{ 78, 84, 96, 255 });

    // Anillos de refuerzo amarillos y oscuros, fijos al taladro.
    int k = 0;
    for (float s = base - 0.40f; s > -0.9f; s -= 0.80f, k++)
    {
        Color anillo = k % 2 == 0 ? Color{ 222, 176, 40, 255 } : Color{ 40, 42, 48, 255 };
        DrawCylinderEx(punto(s), punto(s + 0.18f), R + 0.04f, R + 0.04f, 14, anillo);
    }

    // Collar y cabezal.
    DrawCylinderEx(punto(base - 0.10f), punto(base + 0.14f), R + 0.09f, R + 0.09f, 14, Color{ 44, 47, 54, 255 });

    Color metal = Color{ 150, 164, 184, 255 };
    if (atacando) metal = MezclaColorPinchos(metal, Color{ 255, 120, 50, 255 }, 0.16f);

    DrawCylinderEx(punto(base + 0.12f), punto(largo), R * 1.10f, 0.04f, 12, metal);

    // Estrias helicoidales de la broca (giran segun el tiempo).
    float giro = tiempo * (atacando ? 22.0f : 9.0f);
    const int PASOS = 8;

    for (int p = 0; p < PASOS; p++)
    {
        float t = ((float)p + 0.5f) / (float)PASOS;
        float s = base + 0.12f + (largo - base - 0.12f) * t;
        float radio = R * 1.10f * (1.0f - t) + 0.05f;

        for (int hebra = 0; hebra < 2; hebra++)
        {
            float angulo = giro + t * 7.0f + (float)hebra * 3.1416f;
            Vector3 centro = punto(s);
            centro.x += lateral.x * std::cos(angulo) * radio;
            centro.z += lateral.z * std::cos(angulo) * radio;
            centro.y += std::sin(angulo) * radio;
            DrawSphereEx(centro, 0.085f * (1.0f - t * 0.55f), 4, 5, Color{ 70, 76, 88, 255 });
        }
    }

    // Sombra bajo el taladro (se proyecta sobre el suelo).
    Vector3 sombraA = punto(0.0f);
    Vector3 sombraB = punto(largo);
    DrawCubeV(
        { (sombraA.x + sombraB.x) * 0.5f, 0.045f, (sombraA.z + sombraB.z) * 0.5f },
        {
            std::fabs(avance.x) * (largo + 0.1f) + std::fabs(lateral.x) * R * 2.1f,
            0.01f,
            std::fabs(avance.z) * (largo + 0.1f) + std::fabs(lateral.z) * R * 2.1f
        },
        Fade(BLACK, 0.35f)
    );
}


//==================================================
// VISUAL: CABINA DEL OPERADOR
//==================================================

static void DibujarCabinaOperadorPinchos(const MinijuegoRefugioPinchos& m, Color colorOperador)
{
    const float yPlataforma = 2.75f;
    const float zc = -PARED_Z_PINCHOS - 1.0f;

    // Plataforma sobre el muro del fondo.
    CajaRocaPinchos({ 0.0f, yPlataforma + 0.10f, zc }, { 3.8f, 0.30f, 2.5f }, Color{ 54, 58, 66, 255 });
    DrawCubeV({ 0.0f, yPlataforma + 0.27f, zc + 1.17f }, { 3.8f, 0.10f, 0.08f }, Color{ 214, 170, 40, 255 });

    // Consola con pantalla.
    DrawCubeV({ 0.0f, 3.45f, zc + 0.40f }, { 2.7f, 0.80f, 0.85f }, Color{ 40, 44, 52, 255 });
    DrawCubeWiresV({ 0.0f, 3.45f, zc + 0.40f }, { 2.7f, 0.80f, 0.85f }, Fade(BLACK, 0.5f));
    DrawCubeV({ 0.0f, 3.98f, zc + 0.13f }, { 2.0f, 0.55f, 0.08f }, Color{ 20, 30, 34, 255 });
    DrawCubeV({ 0.0f, 3.98f, zc + 0.18f }, { 1.8f, 0.40f, 0.02f }, Fade(COLOR_CIAN_REFUGIO, 0.85f));

    // Cruz de direccion: se enciende la que el operador ha elegido.
    struct FlechaOperador { float x; float z; DireccionPinchos dir; };
    const FlechaOperador flechas[4] =
    {
        { 0.0f, -0.24f, PINCHOS_DESDE_ARRIBA },
        { 0.0f, 0.24f, PINCHOS_DESDE_ABAJO },
        { -0.40f, 0.0f, PINCHOS_DESDE_IZQUIERDA },
        { 0.40f, 0.0f, PINCHOS_DESDE_DERECHA }
    };

    for (int k = 0; k < 4; k++)
    {
        bool activa = LadoActivoPinchos(m, flechas[k].dir);
        Color color = activa ? Color{ 255, 120, 40, 255 } : Color{ 74, 78, 88, 255 };
        DrawCubeV({ flechas[k].x, 3.88f, zc + 0.40f + flechas[k].z }, { 0.30f, 0.10f, 0.30f }, color);
    }

    // Banderola del color del operador: se reconoce de un vistazo.
    DrawCubeV({ -1.55f, 4.15f, zc - 0.35f }, { 0.10f, 2.2f, 0.10f }, Color{ 50, 52, 58, 255 });
    DrawCubeV({ 1.55f, 4.15f, zc - 0.35f }, { 0.10f, 2.2f, 0.10f }, Color{ 50, 52, 58, 255 });
    DrawCubeV({ 0.0f, 5.30f, zc - 0.35f }, { 3.2f, 0.18f, 0.18f }, colorOperador);
}


//==================================================
// VISUAL: PARTICULAS Y RESPLANDORES
//==================================================

static void DibujarParticulasPinchos(const MinijuegoRefugioPinchos& m)
{
    for (int i = 0; i < MAX_PARTICULAS_PINCHOS; i++)
    {
        const ParticulaRefugioPinchos& p = m.particulasLocales[i];
        if (!p.activa) continue;

        float vida = p.vida / p.vidaMaxima;
        float tamano = p.chispa ? p.tamano : p.tamano * (1.6f - 0.6f * vida);
        Color color = p.color;
        color.a = (unsigned char)((float)p.color.a * (p.chispa ? (vida > 0.3f ? 1.0f : vida / 0.3f) : vida * 0.8f));

        DrawCubeV(p.posicion, { tamano, tamano, tamano }, color);
    }
}


static void DibujarResplandoresPinchos(const MinijuegoRefugioPinchos& m)
{
    if (FactorCalidadGrafica(CalidadEfectos()) < 0.5f) return;

    float t = (float)GetTime();

    rlDrawRenderBatchActive();
    rlDisableDepthMask();
    BeginBlendMode(BLEND_ADDITIVE);

    // Antorchas del muro del fondo: luz calida en la pared y en el suelo.
    const float xAntorchas[3] = { -3.2f, 0.0f, 3.2f };
    for (int k = 0; k < 3; k++)
    {
        float parpadeo = 0.80f + 0.20f * std::sin(t * 11.0f + (float)k * 1.7f);
        ResplandorMuroPinchos({ xAntorchas[k], 1.9f, -PARED_Z_PINCHOS + 0.12f }, true, 1.4f * parpadeo, Color{ 255, 140, 50, 120 });
        ResplandorSueloPinchos(xAntorchas[k], -PARED_Z_PINCHOS + 1.0f, 2.0f * parpadeo, Color{ 255, 130, 45, 60 });
    }

    // Cristales de las coberturas: luz fria sobre la cumbre.
    for (int i = 1; i < m.cantidadBloques; i++)
    {
        const BloquePrueba& b = m.bloques[i];
        float pulso = 0.85f + 0.15f * std::sin(t * 3.0f + (float)i);
        ResplandorPinchos({ b.posicion.x, b.tamano.y + 0.45f, b.posicion.z }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, 0.9f * pulso, Color{ 90, 230, 245, 70 });
    }

    // Cristales del entorno: verdes a la izquierda, ambar a la derecha.
    for (int k = 0; k < 4; k++)
    {
        const CristalAmbientePinchos& cristal = CRISTALES_AMBIENTE_PINCHOS[k];
        float pulso = 0.8f + 0.2f * std::sin(t * 2.2f + (float)k * 1.9f);
        ResplandorPinchos({ cristal.x, cristal.y + 0.05f, cristal.z }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, 3.0f * pulso, Color{ cristal.color.r, cristal.color.g, cristal.color.b, 85 });
        ResplandorPinchos({ cristal.x, cristal.y + cristal.alto * 0.6f, cristal.z }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, 1.3f * pulso, Color{ cristal.color.r, cristal.color.g, cristal.color.b, 90 });
    }

    // Luces de aviso en cada boca y calor de las brocas.
    if (m.patronActivo)
    {
        for (int si = 0; si < m.patron.cantidad; si++)
        {
            const SalvaPinchos& s = m.patron.salvas[si];

            for (int c = 0; c < CantidadCarrilesPinchos(s.lado); c++)
            {
                EstadoCarrilPinchos e = EstadoCarrilSalvaPinchos(m, s, c);
                if (!e.activo) continue;

                Vector3 origen{};
                Vector3 avance{};
                ObtenerOrigenTaladroPinchos(s.lado, c, origen, avance);

                float intensidad = e.aviso ? (ParpadeoRojoPinchos(m) ? 1.0f : 0.45f) : 1.0f;
                Vector3 boca = { origen.x + avance.x * 0.9f, 0.0f, origen.z + avance.z * 0.9f };
                ResplandorSueloPinchos(boca.x, boca.z, 1.3f, Color{ 255, 60, 30, (unsigned char)(110.0f * intensidad) });

                if (e.letal)
                {
                    float largo = PuntaTaladroPinchos(m, s.lado, c, e.fraccion);
                    ResplandorSueloPinchos(origen.x + avance.x * largo, origen.z + avance.z * largo, 1.0f, Color{ 255, 150, 60, 110 });
                }
            }
        }
    }

    EndBlendMode();
    rlEnableDepthMask();
}


//==================================================
// VISUAL: ESCENA 3D + HUD
//==================================================

static void DibujarTextoSombraPinchos(const char* texto, int x, int y, int tamano, Color color)
{
    DrawText(texto, x + 2, y + 2, tamano, Fade(BLACK, 0.75f));
    DrawText(texto, x, y, tamano, color);
}


static void DibujarTextoCentradoPinchos(const char* texto, int centroX, int y, int tamano, Color color)
{
    DibujarTextoSombraPinchos(texto, centroX - MeasureText(texto, tamano) / 2, y, tamano, color);
}


// Flecha triangular que apunta hacia el lado indicado (de donde vienen los taladros).
static void DibujarFlechaLadoPinchos(DireccionPinchos d, int ancho, int alto, float e, Color color)
{
    float tam = 34.0f * e;
    float cx = (float)ancho * 0.5f;
    float cy = (float)alto * 0.5f;

    switch (d)
    {
        case PINCHOS_DESDE_IZQUIERDA:
            DrawTriangle({ 18.0f * e, cy - tam }, { 18.0f * e, cy + tam }, { 18.0f * e + tam * 1.3f, cy }, color);
            break;
        case PINCHOS_DESDE_DERECHA:
            DrawTriangle({ (float)ancho - 18.0f * e, cy - tam }, { (float)ancho - 18.0f * e - tam * 1.3f, cy }, { (float)ancho - 18.0f * e, cy + tam }, color);
            break;
        case PINCHOS_DESDE_ARRIBA:
            DrawTriangle({ cx - tam, 84.0f * e }, { cx + tam, 84.0f * e }, { cx, 84.0f * e + tam * 1.3f }, color);
            break;
        case PINCHOS_DESDE_ABAJO:
            DrawTriangle({ cx - tam, (float)alto - 18.0f * e }, { cx, (float)alto - 18.0f * e - tam * 1.3f }, { cx + tam, (float)alto - 18.0f * e }, color);
            break;
    }
}


// Texto corto del ataque en curso: patron y lado(s).
static const char* TextoAtaquePinchos(const MinijuegoRefugioPinchos& m)
{
    const PatronActivoPinchos& p = m.patron;
    const char* nombre = NOMBRES_PATRON_PINCHOS[p.tipo];

    if (p.tipo == PATRON_TENAZA)
        return TextFormat("%s: %s Y %s", nombre, NombreDireccionPinchos(p.salvas[0].lado), NombreDireccionPinchos(p.salvas[1].lado));
    if (p.tipo == PATRON_CRUCE)
        return TextFormat("%s: %s Y %s", nombre, NombreDireccionPinchos(p.salvas[0].lado), NombreDireccionPinchos(p.salvas[1].lado));
    if (p.tipo == PATRON_VAIVEN)
        return TextFormat("%s: %s Y LUEGO %s", nombre, NombreDireccionPinchos(p.salvas[0].lado), NombreDireccionPinchos(p.salvas[1].lado));

    return TextFormat("%s: %s", nombre, NombreDireccionPinchos(p.salvas[0].lado));
}


void MinijuegoRefugioPinchos::Dibujar(
    const JugadorPrueba jugadores[],
    int cantidadMaxima,
    const Participante participantes[],
    bool mostrarDebug
) const
{
    SeleccionarTemaVisualMinijuego(TEMA_VISUAL_NINGUNO);
    ClearBackground(Color{ 14, 12, 14, 255 });

    BeginMode3D(camara);
    CulloPinchos(false);

    DibujarCuevaExteriorPinchos();
    DibujarPrimerPlanoPinchos();
    DibujarSueloPinchos();
    DibujarBordesSueloPinchos();
    DibujarPlacasBocasPinchos();

    DibujarMuroPortalesPinchos(*this, true);
    DibujarMuroPortalesPinchos(*this, false);
    DibujarBancoLateralPinchos(*this, true);
    DibujarBancoLateralPinchos(*this, false);
    DibujarColumnasPinchos();

    // Torchas del muro del fondo.
    DibujarAntorchaPinchos({ -3.2f, 0.95f, -PARED_Z_PINCHOS + 0.10f }, 1);
    DibujarAntorchaPinchos({ 0.0f, 0.95f, -PARED_Z_PINCHOS + 0.10f }, 2);
    DibujarAntorchaPinchos({ 3.2f, 0.95f, -PARED_Z_PINCHOS + 0.10f }, 3);

    Color colorOperador = indiceSolo >= 0 ? participantes[indiceSolo].color : RED;
    DibujarCabinaOperadorPinchos(*this, colorOperador);

    for (int i = 1; i < cantidadBloques; i++)
    {
        DibujarCoberturaPinchos(bloques[i], i);
        if (mostrarDebug) DrawBoundingBox(CrearHitboxBloquePrueba(bloques[i]), YELLOW);
    }

    DibujarFranjasPeligroPinchos(*this);

    for (int d = 0; d < 4; d++)
    {
        for (int c = 0; c < CantidadCarrilesPinchos((DireccionPinchos)d); c++)
            DibujarTaladroPinchos(*this, (DireccionPinchos)d, c);
    }

    // Operador en su cabina.
    if (indiceSolo >= 0)
    {
        JugadorPrueba operador = jugadores[indiceSolo];
        Participante figura = participantes[indiceSolo];
        operador.posicion = { 0.0f, 3.72f, -PARED_Z_PINCHOS - 1.0f };
        operador.direccionMirada = { 0.0f, 0.0f, 1.0f };
        operador.enSuelo = true;
        operador.cayendo = false;
        operador.aplastado = false;
        figura.conectado = true;
        DibujarJugadorCuboPrueba(operador, figura);
    }

    for (int i = 0; i < MAX_PARTICIPANTES && i < cantidadMaxima; i++)
    {
        if (i == indiceSolo || !resultado.participantes[i].participo) continue;

        if (estadosJugadores[i].eliminado)
        {
            // Aplastado: se queda un momento en el suelo y luego se desvanece.
            if (estadosJugadores[i].tiempoDesdeEliminacion < 1.6f)
            {
                JugadorPrueba caido = jugadores[i];
                caido.aplastado = true;
                DibujarJugadorCuboPrueba(caido, participantes[i]);
            }
            continue;
        }

        // Anillo del color del jugador y marca flotante sobre la cabeza.
        // MODELO FUTURO: la flecha puede pasar a ser un icono del GLB.
        float pies = jugadores[i].posicion.y - jugadores[i].tamano.y * 0.5f;
        for (int k = 0; k < 3; k++)
        {
            DrawCircle3D(
                { jugadores[i].posicion.x, pies + 0.08f, jugadores[i].posicion.z },
                0.52f + k * 0.045f, { 1.0f, 0.0f, 0.0f }, 90.0f, participantes[i].color
            );
        }

        float cabeza = pies + 2.15f + std::sin((float)GetTime() * 5.0f + (float)i) * 0.06f;
        DrawCylinderEx(
            { jugadores[i].posicion.x, cabeza, jugadores[i].posicion.z },
            { jugadores[i].posicion.x, cabeza + 0.32f, jugadores[i].posicion.z },
            0.0f, 0.20f, 10, participantes[i].color
        );

        DibujarJugadorCuboPrueba(jugadores[i], participantes[i]);
        if (mostrarDebug) DrawBoundingBox(CrearHitboxJugadorPrueba(jugadores[i]), LIME);
    }

    DibujarParticulasPinchos(*this);
    DibujarResplandoresPinchos(*this);

    if (mostrarDebug)
    {
        DrawCubeWires(
            { 0.0f, 0.80f, 0.0f },
            LIMITE_X_JUGADORES_PINCHOS * 2.0f, 1.60f, LIMITE_Z_JUGADORES_PINCHOS * 2.0f, PURPLE
        );
    }

    CulloPinchos(true);
    EndMode3D();

    //----------------------------------------------
    // HUD: solo lo necesario (tiempo, vivos, ataque actual)
    //----------------------------------------------
    const int ancho = GetScreenWidth();
    const int alto = GetScreenHeight();
    const float e = (float)alto / 720.0f;

    DrawRectangleGradientV(0, 0, ancho, (int)(56 * e), Fade(BLACK, 0.62f), Fade(BLACK, 0.0f));

    if (indiceSolo >= 0)
    {
        const char* rol = TextFormat(
            "OPERADOR J%d%s",
            participantes[indiceSolo].numeroJugador,
            ControlaIAPinchos(participantes[indiceSolo]) ? " (BOT)" : ""
        );
        DibujarTextoSombraPinchos(rol, (int)(22 * e), (int)(12 * e), (int)(20 * e), colorOperador);
    }

    if (fase != FASE_PINCHOS_PREPARACION && fase != FASE_PINCHOS_TERMINADO)
    {
        const char* reloj = TextFormat("%.1f", tiempoRestante);
        int tamanoReloj = (int)(36 * e);
        DibujarTextoSombraPinchos(
            reloj, ancho - MeasureText(reloj, tamanoReloj) - (int)(26 * e), (int)(8 * e), tamanoReloj,
            tiempoRestante <= 5.0f ? Color{ 255, 90, 80, 255 } : GOLD
        );

        if (MuerteSubitaPinchos(*this))
        {
            const char* subita = "MUERTE SUBITA";
            int tamano = (int)(16 * e);
            DibujarTextoSombraPinchos(
                subita, ancho - MeasureText(subita, tamano) - (int)(26 * e), (int)(46 * e), tamano,
                ParpadeoRojoPinchos(*this) ? Color{ 255, 90, 70, 255 } : Color{ 255, 170, 90, 255 }
            );
        }
    }

    // Nombre sobre cada jugador vivo.
    for (int i = 0; i < MAX_PARTICIPANTES && i < cantidadMaxima; i++)
    {
        if (i == indiceSolo || !resultado.participantes[i].participo || estadosJugadores[i].eliminado) continue;

        Vector2 pantalla = GetWorldToScreen(
            { jugadores[i].posicion.x, jugadores[i].posicion.y + 1.75f, jugadores[i].posicion.z }, camara
        );
        const char* etiqueta = TextFormat("J%d", participantes[i].numeroJugador);
        int tamano = (int)(18 * e);
        DibujarTextoCentradoPinchos(etiqueta, (int)pantalla.x, (int)pantalla.y - tamano, tamano, participantes[i].color);
    }

    // Estado del equipo (abajo a la izquierda).
    {
        int y = alto - (int)(32 * e);
        int x = (int)(22 * e);

        for (int i = 0; i < MAX_PARTICIPANTES; i++)
        {
            if (i == indiceSolo || !resultado.participantes[i].participo) continue;

            bool vivo = !estadosJugadores[i].eliminado;
            DrawRectangle(x, y, (int)(22 * e), (int)(22 * e), vivo ? participantes[i].color : Fade(participantes[i].color, 0.25f));
            DrawRectangleLines(x, y, (int)(22 * e), (int)(22 * e), BLACK);
            DrawText(vivo ? "OK" : "X", x + (int)(30 * e), y + (int)(2 * e), (int)(18 * e), vivo ? RAYWHITE : Color{ 255, 110, 90, 255 });
            x += (int)(82 * e);
        }
    }

    // Ataque en curso: nombre del patron y, mientras avisa, la cuenta atras.
    if (patronActivo && (fase == FASE_PINCHOS_AVISO || fase == FASE_PINCHOS_ATAQUE))
    {
        bool avisando = fase == FASE_PINCHOS_AVISO;
        bool parpadeo = ParpadeoRojoPinchos(*this);
        Color color = avisando && parpadeo ? Color{ 255, 90, 60, 255 } : Color{ 255, 190, 70, 255 };

        const char* texto = TextoAtaquePinchos(*this);
        int tamano = (int)(26 * e);
        DibujarTextoCentradoPinchos(texto, ancho / 2, (int)(10 * e), tamano, color);

        if (avisando)
        {
            float primero = patron.salvas[0].disparo;
            float progreso = LimitarPinchos(tiempoPatron / primero, 0.0f, 1.0f);
            int anchoBarra = (int)(300 * e);
            int xBarra = ancho / 2 - anchoBarra / 2;
            int yBarra = (int)(44 * e);
            DrawRectangle(xBarra, yBarra, anchoBarra, (int)(8 * e), Fade(BLACK, 0.6f));
            DrawRectangle(xBarra, yBarra, (int)((float)anchoBarra * progreso), (int)(8 * e), color);
        }

        for (int d = 0; d < 4; d++)
        {
            if (!LadoActivoPinchos(*this, (DireccionPinchos)d)) continue;
            DibujarFlechaLadoPinchos((DireccionPinchos)d, ancho, alto, e, Fade(color, avisando ? (parpadeo ? 0.90f : 0.50f) : 0.90f));
        }
    }
    else if (fase == FASE_PINCHOS_ESPERANDO && indiceSolo >= 0 && !ControlaIAPinchos(participantes[indiceSolo]) && cooldownAtaque <= 0.0f)
    {
        const char* linea = TextFormat("ELIGE EL LADO DEL ATAQUE  %.0f", std::ceil(ESPERA_AUTOMATICA_PINCHOS - tiempoEleccion));
        DibujarTextoCentradoPinchos(linea, ancho / 2, (int)(12 * e), (int)(22 * e), RAYWHITE);
    }

    // Instrucciones breves solo durante la cuenta atras.
    if (fase == FASE_PINCHOS_PREPARACION)
    {
        int numero = (int)std::ceil(tiempoPreparacion);
        if (numero < 1) numero = 1;
        const char* texto = TextFormat("%d", numero);
        int tamano = (int)(100 * e);
        DibujarTextoCentradoPinchos(texto, ancho / 2, (int)((float)alto * 0.38f), tamano, GOLD);

        DibujarTextoCentradoPinchos("LA FRANJA ROJA ES MORTAL: ESCONDETE DETRAS DE LAS ROCAS", ancho / 2, alto - (int)(40 * e), (int)(18 * e), RAYWHITE);
    }

    if (fase == FASE_PINCHOS_TERMINADO && resultado.estado == RESULTADO_MINIJUEGO_FINALIZADO)
    {
        bool ganaSolo = ContarRivalesVivosPinchos(*this) <= 0;
        const char* titulo = ganaSolo ? "GANA EL OPERADOR" : "GANA EL EQUIPO";
        const char* detalle = ganaSolo
            ? "TODO EL EQUIPO QUEDO APLASTADO"
            : TextFormat("SOBREVIVIERON %d DEL EQUIPO", ContarRivalesVivosPinchos(*this));

        int anchoPanel = (int)(560 * e);
        int altoPanel = (int)(130 * e);
        int yPanel = alto - altoPanel - (int)(60 * e);
        DrawRectangle(ancho / 2 - anchoPanel / 2, yPanel, anchoPanel, altoPanel, Fade(BLACK, 0.82f));
        DrawRectangleLines(ancho / 2 - anchoPanel / 2, yPanel, anchoPanel, altoPanel, GOLD);
        DibujarTextoCentradoPinchos(titulo, ancho / 2, yPanel + (int)(16 * e), (int)(36 * e), GOLD);
        DibujarTextoCentradoPinchos(detalle, ancho / 2, yPanel + (int)(62 * e), (int)(20 * e), RAYWHITE);
        DibujarTextoCentradoPinchos(TextoReinicioMinijuego(), ancho / 2, yPanel + altoPanel - (int)(34 * e), (int)(18 * e), LIGHTGRAY);
    }
}


const ResultadoMinijuego& MinijuegoRefugioPinchos::ObtenerResultado() const
{
    return resultado;
}


void MinijuegoRefugioPinchos::ConfigurarTaladrosVisuales() const
{
    // Este minijuego dibuja su propio escenario (Dibujar selecciona
    // TEMA_VISUAL_NINGUNO); se publica el estado por compatibilidad con el
    // tema global de cueva.
    DireccionPinchos lado = patronActivo ? patron.salvas[0].lado : PINCHOS_DESDE_ARRIBA;
    float progreso = 0.0f;

    if (patronActivo)
    {
        for (int c = 0; c < CantidadCarrilesPinchos(lado); c++)
        {
            float f = EstadoCarrilSalvaPinchos(*this, patron.salvas[0], c).fraccion;
            if (f > progreso) progreso = f;
        }
    }

    ConfigurarTaladrosVisualesMinijuego(
        (int)lado,
        progreso,
        fase == FASE_PINCHOS_AVISO
    );
}
