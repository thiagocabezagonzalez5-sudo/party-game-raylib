#include "Minigames/MinijuegoGemasDeslizantes.h"

#include "Minigames/AudioMinijuegos.h"
#include "Minigames/HudMinijuego.h"
#include "Systems/CalidadGrafica.h"
#include "Systems/Input.h"

#include "raylib.h"

#include <cmath>
#include <cstdio>
#include <cstring>


//==================================================
// CONSTANTES
//==================================================

static const float DURACION_PREPARACION_GEMAS = 3.0f;
static const float DURACION_REPARTO_GEMAS = 1.7f;
static const float DURACION_FIN_RONDA_GEMAS = 2.6f;
static const float DURACION_FIN_PARTIDA_GEMAS = 2.4f;
static const float DURACION_RONDA_GEMAS = 28.0f;

static const float TAMANO_CELDA_GEMAS = 1.15f;
static const float CENTRO_X_GEMAS = 4.7f;
static const float CENTRO_Z_GEMAS = 0.80f;
static const float ALTURA_SUELO_BANDEJA = 0.10f;
static const float ALTURA_GEMAS = ALTURA_SUELO_BANDEJA + 0.03f;

static const float RETRASO_REPETICION_GEMAS = 0.20f;
static const float INTERVALO_REPETICION_GEMAS = 0.09f;
static const float RECARGA_MOVIMIENTO_GEMAS = 0.07f;

static const int RONDAS_PARA_GANAR_GEMAS = 2;
static const int RANGO_MIN_GEMAS[MAX_RONDAS_GEMAS] = { 9, 10, 11 };
static const int RANGO_MAX_GEMAS[MAX_RONDAS_GEMAS] = { 11, 12, 13 };

static const int CANTIDAD_PATRONES_GEMAS = 4;

// Colores: 0 hueco, 1 rubi, 2 zafiro, 3 esmeralda, 4 topacio, 5 amatista.
static const uint8_t PATRONES_GEMAS[CANTIDAD_PATRONES_GEMAS][CELDAS_GEMAS] =
{
    { 4, 0, 4,   1, 3, 1,   2, 5, 2 },   // CORONA
    { 5, 1, 5,   1, 4, 1,   3, 0, 3 },   // FLOR
    { 4, 2, 4,   1, 0, 1,   3, 5, 3 },   // TESORO
    { 5, 0, 5,   2, 1, 2,   3, 4, 3 }    // MARIPOSA
};

static const char* NOMBRES_PATRONES_GEMAS[CANTIDAD_PATRONES_GEMAS] =
{
    "CORONA", "FLOR", "TESORO", "MARIPOSA"
};

// Direccion en que SE MUEVE la gema: 0 arriba, 1 abajo, 2 izquierda, 3 derecha.
static const int DX_GEMAS[4] = { 0, 0, -1, 1 };
static const int DZ_GEMAS[4] = { -1, 1, 0, 0 };

// Figura grabada por color (5x5, bit 4 = columna izquierda).
static const uint8_t GLIFOS_GEMAS[6][5] =
{
    { 0, 0, 0, 0, 0 },
    { 0x04, 0x0E, 0x1F, 0x0E, 0x04 },   // rubi: rombo
    { 0x0E, 0x11, 0x11, 0x11, 0x0E },   // zafiro: aro
    { 0x04, 0x0E, 0x0E, 0x1F, 0x1F },   // esmeralda: triangulo
    { 0x1B, 0x0E, 0x04, 0x0E, 0x1B },   // topacio: equis
    { 0x1F, 0x1F, 0x1F, 0x1F, 0x1F }    // amatista: cuadrado
};


//==================================================
// ESTADOS Y DISTANCIAS (BFS DESDE EL DIBUJO OBJETIVO)
//==================================================
//
// Un estado son los 9 colores de la bandeja (3 bits cada uno). Se hace un
// BFS desde el dibujo resuelto y se guarda la distancia de cada estado
// alcanzable en una tabla hash estatica (sin asignaciones). La tabla sirve a
// la eleccion del tablero mezclado y al resolvedor de los bots, y se rearma
// al repartir cada ronda. Hay una sola partida activa a la vez.
//==================================================

static const int BITS_TABLA_GEMAS = 17;
static const int TAMANO_TABLA_GEMAS = 1 << BITS_TABLA_GEMAS;
static const int CAPACIDAD_COLA_GEMAS = 1 << 16;
static const uint32_t CLAVE_VACIA_GEMAS = 0xFFFFFFFFu;

static uint32_t g_claves[TAMANO_TABLA_GEMAS];
static uint8_t g_distancias[TAMANO_TABLA_GEMAS];
static uint32_t g_cola[CAPACIDAD_COLA_GEMAS];
static int g_estadosTabla = 0;
static int g_patronTabla = -1;


static uint32_t CodificarEstadoGemas(const uint8_t colores[CELDAS_GEMAS])
{
    uint32_t clave = 0;

    for (int i = 0; i < CELDAS_GEMAS; i++)
    {
        clave |= (uint32_t)colores[i] << (3 * i);
    }

    return clave;
}


static void DecodificarEstadoGemas(uint32_t clave, uint8_t colores[CELDAS_GEMAS])
{
    for (int i = 0; i < CELDAS_GEMAS; i++)
    {
        colores[i] = (uint8_t)((clave >> (3 * i)) & 7u);
    }
}


static int RanuraTablaGemas(uint32_t clave)
{
    uint32_t indice = (clave * 2654435761u) >> (32 - BITS_TABLA_GEMAS);

    while (g_claves[indice] != CLAVE_VACIA_GEMAS && g_claves[indice] != clave)
    {
        indice = (indice + 1) & (TAMANO_TABLA_GEMAS - 1);
    }

    return (int)indice;
}


// Mueve una gema hacia "dir" dentro de un estado de colores. Devuelve false
// si no hay gema que empujar en ese sentido.
static bool MoverColoresGemas(uint8_t colores[CELDAS_GEMAS], int dir)
{
    int hueco = 0;

    while (hueco < CELDAS_GEMAS && colores[hueco] != 0)
    {
        hueco++;
    }

    int fila = hueco / 3 - DZ_GEMAS[dir];
    int columna = hueco % 3 - DX_GEMAS[dir];

    if (fila < 0 || fila > 2 || columna < 0 || columna > 2)
    {
        return false;
    }

    colores[hueco] = colores[fila * 3 + columna];
    colores[fila * 3 + columna] = 0;
    return true;
}


static void ConstruirDistanciasGemas(int patron)
{
    if (g_patronTabla == patron)
    {
        return;
    }

    std::memset(g_claves, 0xFF, sizeof(g_claves));
    g_estadosTabla = 0;
    g_patronTabla = patron;

    uint32_t inicio = CodificarEstadoGemas(PATRONES_GEMAS[patron]);
    int ranura = RanuraTablaGemas(inicio);
    g_claves[ranura] = inicio;
    g_distancias[ranura] = 0;
    g_estadosTabla = 1;

    int cabeza = 0;
    int cola = 0;
    g_cola[cola++] = inicio;

    while (cabeza < cola)
    {
        uint32_t clave = g_cola[cabeza++];
        uint8_t distancia = g_distancias[RanuraTablaGemas(clave)];

        for (int dir = 0; dir < 4; dir++)
        {
            uint8_t colores[CELDAS_GEMAS];
            DecodificarEstadoGemas(clave, colores);

            if (!MoverColoresGemas(colores, dir))
            {
                continue;
            }

            uint32_t vecino = CodificarEstadoGemas(colores);
            int ranuraVecino = RanuraTablaGemas(vecino);

            if (g_claves[ranuraVecino] != CLAVE_VACIA_GEMAS)
            {
                continue;
            }

            if (cola >= CAPACIDAD_COLA_GEMAS)
            {
                continue;
            }

            g_claves[ranuraVecino] = vecino;
            g_distancias[ranuraVecino] = (uint8_t)(distancia + 1);
            g_estadosTabla++;
            g_cola[cola++] = vecino;
        }
    }
}


// -1 si el estado no esta en la tabla.
static int DistanciaEstadoGemas(const uint8_t colores[CELDAS_GEMAS])
{
    uint32_t clave = CodificarEstadoGemas(colores);
    int ranura = RanuraTablaGemas(clave);

    return g_claves[ranura] == clave ? (int)g_distancias[ranura] : -1;
}


//==================================================
// ALEATORIO PROPIO
//==================================================

static uint32_t SiguienteAleatorioGemas(MinijuegoGemasDeslizantes& minijuego)
{
    uint32_t x = minijuego.semilla;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    minijuego.semilla = x != 0 ? x : 0x9E3779B9u;
    return minijuego.semilla;
}


static float Aleatorio01Gemas(MinijuegoGemasDeslizantes& minijuego)
{
    return (float)(SiguienteAleatorioGemas(minijuego) >> 8) / 16777216.0f;
}


static float AleatorioRangoGemas(MinijuegoGemasDeslizantes& minijuego, float minimo, float maximo)
{
    return minimo + (maximo - minimo) * Aleatorio01Gemas(minijuego);
}


//==================================================
// UTILIDADES
//==================================================

static bool EsControlBotGemas(const Participante& participante)
{
    return participante.esBot || !participante.conectado;
}


static float Limitar01Gemas(float valor)
{
    if (valor < 0.0f) return 0.0f;
    if (valor > 1.0f) return 1.0f;
    return valor;
}


static void PosicionCeldaGemas(int celda, float& x, float& z)
{
    x = (float)(celda % 3 - 1) * TAMANO_CELDA_GEMAS;
    z = (float)(celda / 3 - 1) * TAMANO_CELDA_GEMAS;
}


static int BuscarHuecoGemas(const TableroGemas& tablero)
{
    for (int i = 0; i < CELDAS_GEMAS; i++)
    {
        if (tablero.celda[i] == GEMA_HUECO)
        {
            return i;
        }
    }

    return 0;
}


static void ColoresTableroGemas(const TableroGemas& tablero, uint8_t colores[CELDAS_GEMAS])
{
    for (int i = 0; i < CELDAS_GEMAS; i++)
    {
        colores[i] = tablero.celda[i] == GEMA_HUECO ? 0 : tablero.colorGema[tablero.celda[i]];
    }
}


static float CentroXTableroGemas(int ranura)
{
    return ranura == 0 ? -CENTRO_X_GEMAS : CENTRO_X_GEMAS;
}


static Color ColorGemaGemas(int color)
{
    switch (color)
    {
        case 1: return Color{ 228, 46, 66, 255 };
        case 2: return Color{ 50, 112, 242, 255 };
        case 3: return Color{ 36, 198, 104, 255 };
        case 4: return Color{ 252, 204, 40, 255 };
        case 5: return Color{ 178, 98, 238, 255 };
        default: break;
    }

    return Color{ 40, 40, 50, 255 };
}


static Color MezclarGemas(Color a, Color b, float t)
{
    t = Limitar01Gemas(t);

    return Color{
        (unsigned char)(a.r + (b.r - a.r) * t),
        (unsigned char)(a.g + (b.g - a.g) * t),
        (unsigned char)(a.b + (b.b - a.b) * t),
        (unsigned char)(a.a + (b.a - a.a) * t)
    };
}


static Color OscurecerGemas(Color c, float factor)
{
    return Color{
        (unsigned char)(c.r * factor),
        (unsigned char)(c.g * factor),
        (unsigned char)(c.b * factor),
        c.a
    };
}


static float EaseOutBackGemas(float t)
{
    t = Limitar01Gemas(t);
    const float c1 = 1.70158f;
    const float c3 = c1 + 1.0f;
    float u = t - 1.0f;
    return 1.0f + c3 * u * u * u + c1 * u * u;
}


//==================================================
// PARTICULAS
//==================================================

static void EmitirParticulaGemas(
    MinijuegoGemasDeslizantes& minijuego,
    Vector3 posicion,
    Vector3 velocidad,
    Color color,
    float vida,
    float tamano,
    int tipo
)
{
    ParticulaGemas& p = minijuego.particulas[minijuego.siguienteParticula];
    minijuego.siguienteParticula = (minijuego.siguienteParticula + 1) % MAX_PARTICULAS_GEMAS;

    p.activa = true;
    p.posicion = posicion;
    p.velocidad = velocidad;
    p.color = color;
    p.vida = vida;
    p.vidaMaxima = vida;
    p.tamano = tamano;
    p.tipo = tipo;
}


static int CantidadParticulasGemas(int base)
{
    int cantidad = (int)(base * FactorCalidadGrafica(CalidadParticulas()));
    return cantidad < 1 ? 1 : cantidad;
}


static void EmitirConfetiGemas(MinijuegoGemasDeslizantes& minijuego, Vector3 centro, int cantidad)
{
    int total = CantidadParticulasGemas(cantidad);

    for (int i = 0; i < total; i++)
    {
        float angulo = Aleatorio01Gemas(minijuego) * 6.2831853f;
        float fuerza = AleatorioRangoGemas(minijuego, 1.2f, 3.6f);
        Vector3 velocidad = {
            std::cos(angulo) * fuerza,
            AleatorioRangoGemas(minijuego, 4.5f, 8.0f),
            std::sin(angulo) * fuerza
        };
        Vector3 posicion = {
            centro.x + AleatorioRangoGemas(minijuego, -1.8f, 1.8f),
            centro.y,
            centro.z + AleatorioRangoGemas(minijuego, -1.8f, 1.8f)
        };
        int color = 1 + (int)(SiguienteAleatorioGemas(minijuego) % 5u);

        EmitirParticulaGemas(
            minijuego,
            posicion,
            velocidad,
            ColorGemaGemas(color),
            AleatorioRangoGemas(minijuego, 1.4f, 2.4f),
            AleatorioRangoGemas(minijuego, 0.10f, 0.17f),
            0
        );
    }
}


static void EmitirDestellosGemas(
    MinijuegoGemasDeslizantes& minijuego,
    Vector3 centro,
    float radio,
    int cantidad,
    Color color
)
{
    int total = CantidadParticulasGemas(cantidad);

    for (int i = 0; i < total; i++)
    {
        float angulo = Aleatorio01Gemas(minijuego) * 6.2831853f;
        float distancia = Aleatorio01Gemas(minijuego) * radio;
        Vector3 posicion = {
            centro.x + std::cos(angulo) * distancia,
            centro.y + AleatorioRangoGemas(minijuego, 0.0f, 0.5f),
            centro.z + std::sin(angulo) * distancia
        };
        Vector3 velocidad = {
            AleatorioRangoGemas(minijuego, -0.4f, 0.4f),
            AleatorioRangoGemas(minijuego, 0.8f, 1.9f),
            AleatorioRangoGemas(minijuego, -0.4f, 0.4f)
        };

        EmitirParticulaGemas(
            minijuego,
            posicion,
            velocidad,
            color,
            AleatorioRangoGemas(minijuego, 0.5f, 0.9f),
            AleatorioRangoGemas(minijuego, 0.12f, 0.2f),
            1
        );
    }
}


static void EmitirPolvoGemas(MinijuegoGemasDeslizantes& minijuego, Vector3 posicion, Color color)
{
    int total = CantidadParticulasGemas(3);

    for (int i = 0; i < total; i++)
    {
        Vector3 velocidad = {
            AleatorioRangoGemas(minijuego, -0.9f, 0.9f),
            AleatorioRangoGemas(minijuego, 0.7f, 1.6f),
            AleatorioRangoGemas(minijuego, -0.9f, 0.9f)
        };

        EmitirParticulaGemas(
            minijuego,
            posicion,
            velocidad,
            MezclarGemas(color, WHITE, 0.5f),
            AleatorioRangoGemas(minijuego, 0.25f, 0.45f),
            0.07f,
            2
        );
    }
}


static void ActualizarParticulasGemas(MinijuegoGemasDeslizantes& minijuego, float deltaTime)
{
    for (int i = 0; i < MAX_PARTICULAS_GEMAS; i++)
    {
        ParticulaGemas& p = minijuego.particulas[i];

        if (!p.activa)
        {
            continue;
        }

        p.vida -= deltaTime;

        if (p.vida <= 0.0f)
        {
            p.activa = false;
            continue;
        }

        if (p.tipo == 0)
        {
            p.velocidad.y -= 9.5f * deltaTime;
            p.velocidad.x *= 1.0f - 0.9f * deltaTime;
            p.velocidad.z *= 1.0f - 0.9f * deltaTime;
        }
        else
        {
            p.velocidad.y -= 0.8f * deltaTime;
        }

        p.posicion.x += p.velocidad.x * deltaTime;
        p.posicion.y += p.velocidad.y * deltaTime;
        p.posicion.z += p.velocidad.z * deltaTime;

        if (p.posicion.y < 0.14f)
        {
            p.posicion.y = 0.14f;
            p.velocidad = { 0.0f, 0.0f, 0.0f };
        }
    }
}


//==================================================
// TABLEROS Y REGLAS
//==================================================

static void RecalcularProgresoGemas(TableroGemas& tablero, int patron)
{
    int correctas = 0;

    for (int i = 0; i < CELDAS_GEMAS; i++)
    {
        int gema = tablero.celda[i];

        if (gema != GEMA_HUECO && tablero.colorGema[gema] == PATRONES_GEMAS[patron][i])
        {
            correctas++;
        }
    }

    uint8_t colores[CELDAS_GEMAS];
    ColoresTableroGemas(tablero, colores);
    int distancia = DistanciaEstadoGemas(colores);

    tablero.correctas = correctas;
    tablero.distancia = distancia < 0 ? 99 : distancia;

    if (correctas > tablero.mejorCorrectas)
    {
        tablero.mejorCorrectas = correctas;
        tablero.tiempoMejor = tablero.tiempoRonda;
    }
}


static Vector3 PosicionMundoGemaGemas(const TableroGemas& tablero, int gema, int ranura)
{
    return {
        CentroXTableroGemas(ranura) + tablero.px[gema],
        ALTURA_GEMAS,
        CENTRO_Z_GEMAS + tablero.pz[gema]
    };
}


// Desliza una gema hacia "dir" si hay una que empujar. Devuelve si se movio.
static bool IntentarMoverGemas(
    MinijuegoGemasDeslizantes& minijuego,
    int ranura,
    int dir,
    bool sonar
)
{
    TableroGemas& tablero = minijuego.tableros[ranura];
    int hueco = BuscarHuecoGemas(tablero);
    int fila = hueco / 3 - DZ_GEMAS[dir];
    int columna = hueco % 3 - DX_GEMAS[dir];

    if (fila < 0 || fila > 2 || columna < 0 || columna > 2)
    {
        return false;
    }

    int origen = fila * 3 + columna;
    int gema = tablero.celda[origen];
    int correctasAntes = tablero.correctas;

    tablero.celda[hueco] = (uint8_t)gema;
    tablero.celda[origen] = GEMA_HUECO;
    tablero.movimientos++;
    tablero.ultimaDireccion = dir;

    RecalcularProgresoGemas(tablero, minijuego.patronActual);

    Color color = ColorGemaGemas(tablero.colorGema[gema]);
    EmitirPolvoGemas(minijuego, PosicionMundoGemaGemas(tablero, gema, ranura), color);

    if (sonar)
    {
        ReproducirSonidoMinijuego(
            minijuego.audio,
            tablero.correctas > correctasAntes ? SONIDO_ACIERTO : SONIDO_UI_MOVER
        );
    }

    // Destello corto de la gema que acaba de caer en su sitio.
    if (tablero.correctas > correctasAntes)
    {
        tablero.destello[gema] = 0.7f;
    }

    return true;
}


// Elige el tablero mezclado: un estado al azar a distancia [minimo, maximo].
static void ElegirTableroMezcladoGemas(
    MinijuegoGemasDeslizantes& minijuego,
    int minimo,
    int maximo,
    uint8_t colores[CELDAS_GEMAS]
)
{
    uint32_t elegido = CLAVE_VACIA_GEMAS;
    int mejorDistancia = -1;
    uint32_t masLejano = CLAVE_VACIA_GEMAS;

    for (int ampliacion = 0; ampliacion < 6 && elegido == CLAVE_VACIA_GEMAS; ampliacion++)
    {
        int candidatos = 0;
        int bajo = minimo - ampliacion;
        int alto = maximo + ampliacion;

        for (int i = 0; i < TAMANO_TABLA_GEMAS; i++)
        {
            if (g_claves[i] == CLAVE_VACIA_GEMAS)
            {
                continue;
            }

            int distancia = g_distancias[i];

            if (distancia > mejorDistancia)
            {
                mejorDistancia = distancia;
                masLejano = g_claves[i];
            }

            if (distancia < bajo || distancia > alto)
            {
                continue;
            }

            // Muestreo por reservorio: uniforme sin lista auxiliar.
            candidatos++;

            if (SiguienteAleatorioGemas(minijuego) % (uint32_t)candidatos == 0u)
            {
                elegido = g_claves[i];
            }
        }
    }

    if (elegido == CLAVE_VACIA_GEMAS)
    {
        elegido = masLejano;
    }

    DecodificarEstadoGemas(elegido, colores);
}


static void RepartirRondaGemas(MinijuegoGemasDeslizantes& minijuego)
{
    minijuego.patronActual = (minijuego.patronBase + minijuego.ronda) % CANTIDAD_PATRONES_GEMAS;
    ConstruirDistanciasGemas(minijuego.patronActual);

    uint8_t colores[CELDAS_GEMAS];
    int ronda = minijuego.ronda < MAX_RONDAS_GEMAS ? minijuego.ronda : MAX_RONDAS_GEMAS - 1;
    ElegirTableroMezcladoGemas(
        minijuego,
        RANGO_MIN_GEMAS[ronda],
        RANGO_MAX_GEMAS[ronda],
        colores
    );

    for (int ranura = 0; ranura < 2; ranura++)
    {
        TableroGemas& tablero = minijuego.tableros[ranura];
        int gema = 0;

        for (int celda = 0; celda < CELDAS_GEMAS; celda++)
        {
            if (colores[celda] == 0)
            {
                tablero.celda[celda] = GEMA_HUECO;
                continue;
            }

            tablero.celda[celda] = (uint8_t)gema;
            tablero.colorGema[gema] = colores[celda];
            PosicionCeldaGemas(celda, tablero.px[gema], tablero.pz[gema]);
            tablero.vx[gema] = 0.0f;
            tablero.vz[gema] = 0.0f;
            tablero.alza[gema] = 0.0f;
            tablero.destello[gema] = 0.0f;
            gema++;
        }

        tablero.movimientos = 0;
        tablero.mejorCorrectas = 0;
        tablero.tiempoMejor = 0.0f;
        tablero.tiempoRonda = 0.0f;
        tablero.completo = false;
        tablero.tiempoCompleto = 0.0f;
        tablero.tiempoBrillo = 0.0f;
        tablero.cantidadCola = 0;
        tablero.dirMantenida = -1;
        tablero.tiempoMantenida = 0.0f;
        tablero.recarga = 0.0f;
        tablero.avisoBloqueo = 0.0f;
        tablero.ultimaDireccion = -1;
        std::memset(tablero.dirPrevia, 0, sizeof(tablero.dirPrevia));

        RecalcularProgresoGemas(tablero, minijuego.patronActual);

        // Habilidad del bot: misma distribucion para ambos lados.
        tablero.habilidad = AleatorioRangoGemas(minijuego, 0.78f, 1.28f);
        tablero.tiempoDecision =
            AleatorioRangoGemas(minijuego, 1.0f, 2.0f) / tablero.habilidad;
    }

    minijuego.tiempoReparto = 0.0f;
    minijuego.tiempoRestante = DURACION_RONDA_GEMAS;
    minijuego.ganadorRonda = -1;
    minijuego.rondaPorTiempo = false;
}


//==================================================
// RESULTADO DE RONDA Y DE PARTIDA
//==================================================

// Gana el que esta mas cerca: gemas bien puestas, distancia, quien llego
// antes a su mejor marca, menos movimientos y, al final, alterna por ronda.
static int GanadorPorProgresoGemas(const MinijuegoGemasDeslizantes& minijuego)
{
    const TableroGemas& a = minijuego.tableros[0];
    const TableroGemas& b = minijuego.tableros[1];

    if (a.correctas != b.correctas) return a.correctas > b.correctas ? 0 : 1;
    if (a.distancia != b.distancia) return a.distancia < b.distancia ? 0 : 1;
    if (a.tiempoMejor != b.tiempoMejor) return a.tiempoMejor < b.tiempoMejor ? 0 : 1;
    if (a.movimientos != b.movimientos) return a.movimientos < b.movimientos ? 0 : 1;

    return minijuego.ronda % 2;
}


static void CerrarRondaGemas(MinijuegoGemasDeslizantes& minijuego, int ganador, bool porTiempo)
{
    minijuego.ganadorRonda = ganador;
    minijuego.rondaPorTiempo = porTiempo;
    minijuego.tableros[ganador].victorias++;
    minijuego.rondasJugadas++;
    minijuego.fase = FASE_GEMAS_FIN_RONDA;
    minijuego.tiempoFase = 0.0f;

    TableroGemas& tablero = minijuego.tableros[ganador];
    Vector3 centro = { CentroXTableroGemas(ganador), 1.2f, CENTRO_Z_GEMAS };

    if (!tablero.completo)
    {
        tablero.tiempoBrillo = 0.01f;
        EmitirConfetiGemas(minijuego, centro, 24);
    }

    ReproducirSonidoMinijuego(minijuego.audio, SONIDO_RESULTADO);
}


static void FinalizarPartidaGemas(MinijuegoGemasDeslizantes& minijuego)
{
    if (minijuego.resultado.estado == RESULTADO_MINIJUEGO_FINALIZADO)
    {
        return;
    }

    int ganador = minijuego.tableros[0].victorias > minijuego.tableros[1].victorias ? 0 : 1;

    for (int ranura = 0; ranura < 2; ranura++)
    {
        int indice = minijuego.tableros[ranura].participante;

        if (indice < 0)
        {
            continue;
        }

        ResultadoParticipante& resultado = minijuego.resultado.participantes[indice];
        resultado.posicionFinal = ranura == ganador ? 1 : 2;
        resultado.puntuacionMinijuego = minijuego.tableros[ranura].victorias;
    }

    minijuego.resultado.estado = RESULTADO_MINIJUEGO_FINALIZADO;
    minijuego.resultado.desenlace = DESENLACE_CON_GANADOR;
    minijuego.partidaValida = true;
    minijuego.fase = FASE_GEMAS_TERMINADO;
    minijuego.tiempoFase = 0.0f;
}


//==================================================
// ENTRADA HUMANA
//==================================================

static void EncolarDireccionGemas(TableroGemas& tablero, int dir)
{
    if (tablero.cantidadCola < COLA_ENTRADA_GEMAS)
    {
        tablero.cola[tablero.cantidadCola++] = dir;
    }
}


static void ActualizarEntradaHumanaGemas(
    MinijuegoGemasDeslizantes& minijuego,
    int ranura,
    float deltaTime,
    const Participante& participante
)
{
    TableroGemas& tablero = minijuego.tableros[ranura];
    InputMinijuegoParticipante entrada = LeerInputMinijuegoParticipante(participante);
    bool actuales[4] = { entrada.adelante, entrada.atras, entrada.izquierda, entrada.derecha };

    int nueva = -1;

    for (int d = 0; d < 4; d++)
    {
        if (actuales[d] && !tablero.dirPrevia[d] && nueva < 0)
        {
            nueva = d;
        }
    }

    if (nueva >= 0)
    {
        tablero.dirMantenida = nueva;
        tablero.tiempoMantenida = 0.0f;
        tablero.proximaRepeticion = 0.0f;
        EncolarDireccionGemas(tablero, nueva);
    }
    else if (tablero.dirMantenida >= 0 && !actuales[tablero.dirMantenida])
    {
        int otra = -1;

        for (int d = 0; d < 4; d++)
        {
            if (actuales[d])
            {
                otra = d;
            }
        }

        tablero.dirMantenida = otra;
        tablero.tiempoMantenida = 0.0f;
        tablero.proximaRepeticion = 0.0f;
    }
    else if (tablero.dirMantenida >= 0)
    {
        tablero.tiempoMantenida += deltaTime;

        if (tablero.tiempoMantenida >= RETRASO_REPETICION_GEMAS)
        {
            tablero.proximaRepeticion -= deltaTime;

            if (tablero.proximaRepeticion <= 0.0f)
            {
                if (tablero.cantidadCola == 0)
                {
                    EncolarDireccionGemas(tablero, tablero.dirMantenida);
                }

                tablero.proximaRepeticion = INTERVALO_REPETICION_GEMAS;
            }
        }
    }

    for (int d = 0; d < 4; d++)
    {
        tablero.dirPrevia[d] = actuales[d];
    }

    tablero.recarga -= deltaTime;

    while (tablero.cantidadCola > 0 && tablero.recarga <= 0.0f)
    {
        int dir = tablero.cola[0];
        tablero.cola[0] = tablero.cola[1];
        tablero.cantidadCola--;

        if (IntentarMoverGemas(minijuego, ranura, dir, true))
        {
            tablero.recarga = RECARGA_MOVIMIENTO_GEMAS;
        }
        else
        {
            tablero.avisoBloqueo = 0.18f;
            tablero.recarga = 0.04f;
        }
    }
}


//==================================================
// BOTS
//==================================================
//
// Siguen el gradiente de distancia del BFS (solucion optima), pero tardan
// como una persona: lectura inicial, tiempo de pensar por movimiento que
// depende de su habilidad, dudas largas y algun movimiento de mas.
//==================================================

static void ActualizarBotGemas(MinijuegoGemasDeslizantes& minijuego, int ranura, float deltaTime)
{
    TableroGemas& tablero = minijuego.tableros[ranura];

    tablero.tiempoDecision -= deltaTime;

    if (tablero.tiempoDecision > 0.0f || tablero.distancia == 0)
    {
        return;
    }

    uint8_t actuales[CELDAS_GEMAS];
    ColoresTableroGemas(tablero, actuales);

    int distanciaActual = tablero.distancia;
    int mejores[4];
    int cantidadMejores = 0;
    int peores[4];
    int cantidadPeores = 0;
    int legales[4];
    int cantidadLegales = 0;

    for (int dir = 0; dir < 4; dir++)
    {
        uint8_t copia[CELDAS_GEMAS];
        std::memcpy(copia, actuales, sizeof(copia));

        if (!MoverColoresGemas(copia, dir))
        {
            continue;
        }

        legales[cantidadLegales++] = dir;
        int distancia = DistanciaEstadoGemas(copia);

        if (distancia >= 0 && distancia < distanciaActual)
        {
            mejores[cantidadMejores++] = dir;
        }
        else if (distancia >= distanciaActual)
        {
            peores[cantidadPeores++] = dir;
        }
    }

    if (cantidadLegales == 0)
    {
        tablero.tiempoDecision = 0.3f;
        return;
    }

    bool errar = cantidadPeores > 0 && Aleatorio01Gemas(minijuego) < 0.10f;
    int dir;

    if (errar)
    {
        dir = peores[SiguienteAleatorioGemas(minijuego) % (uint32_t)cantidadPeores];
    }
    else if (cantidadMejores > 0)
    {
        dir = mejores[SiguienteAleatorioGemas(minijuego) % (uint32_t)cantidadMejores];
    }
    else
    {
        dir = legales[SiguienteAleatorioGemas(minijuego) % (uint32_t)cantidadLegales];
    }

    IntentarMoverGemas(minijuego, ranura, dir, false);

    float espera = AleatorioRangoGemas(minijuego, 0.30f, 0.85f);

    if (Aleatorio01Gemas(minijuego) < 0.08f)
    {
        espera += AleatorioRangoGemas(minijuego, 0.5f, 1.1f);
    }

    tablero.tiempoDecision = espera / tablero.habilidad;
}


//==================================================
// ANIMACION DE LAS BANDEJAS
//==================================================

static void ActualizarVisualesTableroGemas(
    MinijuegoGemasDeslizantes& minijuego,
    int ranura,
    float deltaTime
)
{
    TableroGemas& tablero = minijuego.tableros[ranura];
    const float rigidez = 760.0f;
    const float amortiguacion = 35.0f;
    float paso = deltaTime < (1.0f / 30.0f) ? deltaTime : (1.0f / 30.0f);

    for (int celda = 0; celda < CELDAS_GEMAS; celda++)
    {
        int gema = tablero.celda[celda];

        if (gema == GEMA_HUECO)
        {
            continue;
        }

        float objetivoX = 0.0f;
        float objetivoZ = 0.0f;
        PosicionCeldaGemas(celda, objetivoX, objetivoZ);

        tablero.vx[gema] += (rigidez * (objetivoX - tablero.px[gema]) - amortiguacion * tablero.vx[gema]) * paso;
        tablero.vz[gema] += (rigidez * (objetivoZ - tablero.pz[gema]) - amortiguacion * tablero.vz[gema]) * paso;
        tablero.px[gema] += tablero.vx[gema] * paso;
        tablero.pz[gema] += tablero.vz[gema] * paso;

        float velocidad = std::sqrt(
            tablero.vx[gema] * tablero.vx[gema] + tablero.vz[gema] * tablero.vz[gema]
        );
        float alzaObjetivo = velocidad * 0.035f;

        if (alzaObjetivo > 0.2f)
        {
            alzaObjetivo = 0.2f;
        }

        tablero.alza[gema] += (alzaObjetivo - tablero.alza[gema]) * Limitar01Gemas(paso * 18.0f);

        if (tablero.destello[gema] > 0.0f && !tablero.completo)
        {
            tablero.destello[gema] -= paso * 3.2f;

            if (tablero.destello[gema] < 0.0f)
            {
                tablero.destello[gema] = 0.0f;
            }
        }
    }

    if (tablero.avisoBloqueo > 0.0f)
    {
        tablero.avisoBloqueo -= deltaTime;
    }

    if (!tablero.completo && tablero.tiempoBrillo <= 0.0f)
    {
        return;
    }

    // Celebracion: ola de destellos que recorre el dibujo y chispas sobre la bandeja.
    float anterior = tablero.tiempoCompleto;
    tablero.tiempoCompleto += deltaTime;
    tablero.tiempoBrillo += deltaTime;
    float centroX = CentroXTableroGemas(ranura);

    if (tablero.completo)
    {
        for (int celda = 0; celda < CELDAS_GEMAS; celda++)
        {
            int gema = tablero.celda[celda];

            if (gema == GEMA_HUECO)
            {
                continue;
            }

            float fase = tablero.tiempoCompleto * 9.0f - (float)celda;
            float destello = fase > 0.0f ? Limitar01Gemas(1.0f - fase * 0.42f) : 0.0f;

            if (tablero.tiempoCompleto > 1.4f)
            {
                destello = 0.22f * (0.5f + 0.5f * std::sin(tablero.tiempoCompleto * 5.0f + celda));
            }

            tablero.destello[gema] = destello;

            // Un destello de estrella por gema cuando pasa la ola.
            float faseAntes = anterior * 9.0f - (float)celda;

            if (faseAntes <= 0.0f && fase > 0.0f)
            {
                float x = 0.0f;
                float z = 0.0f;
                PosicionCeldaGemas(celda, x, z);
                EmitirDestellosGemas(
                    minijuego,
                    { centroX + x, 0.9f, CENTRO_Z_GEMAS + z },
                    0.3f,
                    3,
                    ColorGemaGemas(tablero.colorGema[gema])
                );
            }
        }
    }

    // Chispas que caen sobre la bandeja.
    if (std::fmod(tablero.tiempoBrillo, 0.07f) < deltaTime && tablero.tiempoBrillo < 2.6f)
    {
        EmitirDestellosGemas(
            minijuego,
            { centroX, 1.0f, CENTRO_Z_GEMAS },
            2.0f,
            2,
            Color{ 255, 236, 150, 255 }
        );
    }
}


//==================================================
// FLUJO
//==================================================

void MinijuegoGemasDeslizantes::Inicializar()
{
    resultado = {};
    resultado.formato = FORMATO_MINIJUEGO_INDIVIDUAL;

    for (int i = 0; i < 2; i++)
    {
        tableros[i] = {};
        tableros[i].participante = -1;
    }

    for (int i = 0; i < MAX_PARTICULAS_GEMAS; i++)
    {
        particulas[i] = {};
    }

    siguienteParticula = 0;

    uint32_t base = (uint32_t)GetRandomValue(1, 0x3FFFFFFF);
    semilla = (base * 2654435761u) ^ (uint32_t)GetRandomValue(1, 0x3FFFFFFF);

    if (semilla == 0)
    {
        semilla = 0x1234567u;
    }

    patronBase = (int)(SiguienteAleatorioGemas(*this) % (uint32_t)CANTIDAD_PATRONES_GEMAS);
    patronActual = patronBase;
    ronda = 0;
    rondasJugadas = 0;
    ganadorRonda = -1;
    rondaPorTiempo = false;
    partidaValida = false;

    camara.position = { 0.0f, 8.9f, 8.2f };
    camara.target = { 0.0f, 0.0f, 0.55f };
    camara.up = { 0.0f, 1.0f, 0.0f };
    camara.fovy = 50.0f;
    camara.projection = CAMERA_PERSPECTIVE;

    fase = FASE_GEMAS_PREPARACION;
    tiempoPreparacion = DURACION_PREPARACION_GEMAS;
    tiempoRestante = DURACION_RONDA_GEMAS;
    tiempoFase = 0.0f;
    tiempoReparto = 0.0f;
    tiempoAnimacion = 0.0f;
    brilloReferencia = 0.0f;
}


void MinijuegoGemasDeslizantes::Reiniciar(
    JugadorPrueba jugadores[],
    Participante participantes[],
    int cantidadMaxima
)
{
    (void)jugadores;

    Inicializar();
    InicializarResultadoMinijuego(
        resultado,
        participantes,
        FORMATO_MINIJUEGO_INDIVIDUAL
    );

    int limite = cantidadMaxima < MAX_PARTICIPANTES ? cantidadMaxima : MAX_PARTICIPANTES;
    int ranura = 0;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!resultado.participantes[i].participo)
        {
            continue;
        }

        if (i < limite && ranura < 2)
        {
            tableros[ranura].participante = i;
            ranura++;
        }
        else
        {
            // Duelo: solo juegan los dos primeros participantes activos.
            resultado.participantes[i].participo = false;
        }
    }

    if (ranura < 2)
    {
        resultado.estado = RESULTADO_MINIJUEGO_CANCELADO;
        fase = FASE_GEMAS_TERMINADO;
        return;
    }

    resultado.cantidadParticipantes = 2;
    RepartirRondaGemas(*this);
}


void MinijuegoGemasDeslizantes::Actualizar(
    float deltaTime,
    JugadorPrueba jugadores[],
    int cantidadMaxima,
    Participante participantes[]
)
{
    (void)jugadores;
    (void)cantidadMaxima;

    if (deltaTime > 0.1f)
    {
        deltaTime = 0.1f;
    }

    tiempoAnimacion += deltaTime;
    tiempoFase += deltaTime;

    if (resultado.estado == RESULTADO_MINIJUEGO_CANCELADO)
    {
        return;
    }

    ActualizarParticulasGemas(*this, deltaTime);

    if (brilloReferencia > 0.0f)
    {
        brilloReferencia -= deltaTime;
    }

    if (fase == FASE_GEMAS_TERMINADO)
    {
        return;
    }

    for (int ranura = 0; ranura < 2; ranura++)
    {
        ActualizarVisualesTableroGemas(*this, ranura, deltaTime);
    }

    switch (fase)
    {
    case FASE_GEMAS_PREPARACION:
    {
        tiempoReparto += deltaTime;
        float antes = tiempoPreparacion;
        tiempoPreparacion -= deltaTime;
        ActualizarAudioCuentaRegresiva(audio, antes, tiempoPreparacion);

        if (tiempoPreparacion <= 0.0f)
        {
            tiempoPreparacion = 0.0f;
            fase = FASE_GEMAS_JUGANDO;
            tiempoFase = 0.0f;
        }

        break;
    }

    case FASE_GEMAS_REPARTO:
        tiempoReparto += deltaTime;

        if (tiempoFase >= DURACION_REPARTO_GEMAS)
        {
            fase = FASE_GEMAS_JUGANDO;
            tiempoFase = 0.0f;
            ReproducirSonidoMinijuego(audio, SONIDO_INICIO_MINIJUEGO);
        }

        break;

    case FASE_GEMAS_JUGANDO:
    {
        tiempoReparto += deltaTime;
        float antes = tiempoRestante;
        tiempoRestante -= deltaTime;
        ActualizarAudioAlertaTiempo(audio, antes, tiempoRestante);

        for (int ranura = 0; ranura < 2; ranura++)
        {
            TableroGemas& tablero = tableros[ranura];
            tablero.tiempoRonda += deltaTime;

            if (tablero.completo)
            {
                continue;
            }

            const Participante& participante = participantes[tablero.participante];

            if (EsControlBotGemas(participante))
            {
                ActualizarBotGemas(*this, ranura, deltaTime);
            }
            else
            {
                ActualizarEntradaHumanaGemas(*this, ranura, deltaTime, participante);
            }
        }

        bool completoA = tableros[0].distancia == 0;
        bool completoB = tableros[1].distancia == 0;

        if (completoA || completoB)
        {
            int ganador = completoA ? 0 : 1;

            if (completoA && completoB)
            {
                // Terminaron en el mismo cuadro: menos movimientos; si no, alterna.
                if (tableros[0].movimientos != tableros[1].movimientos)
                {
                    ganador = tableros[0].movimientos < tableros[1].movimientos ? 0 : 1;
                }
                else
                {
                    ganador = ronda % 2;
                }
            }

            for (int ranura = 0; ranura < 2; ranura++)
            {
                if (tableros[ranura].distancia == 0)
                {
                    tableros[ranura].completo = true;
                    tableros[ranura].tiempoCompleto = 0.0f;
                    tableros[ranura].tiempoBrillo = 0.01f;
                }
            }

            Vector3 centro = { CentroXTableroGemas(ganador), 1.4f, CENTRO_Z_GEMAS };
            EmitirConfetiGemas(*this, centro, 70);
            brilloReferencia = 2.6f;
            ReproducirSonidoMinijuego(audio, SONIDO_RECOGER_NUCLEO_ESPECIAL);
            CerrarRondaGemas(*this, ganador, false);
            break;
        }

        if (tiempoRestante <= 0.0f)
        {
            tiempoRestante = 0.0f;
            CerrarRondaGemas(*this, GanadorPorProgresoGemas(*this), true);
        }

        break;
    }

    case FASE_GEMAS_FIN_RONDA:
    {
        bool partidaDecidida =
            tableros[0].victorias >= RONDAS_PARA_GANAR_GEMAS ||
            tableros[1].victorias >= RONDAS_PARA_GANAR_GEMAS ||
            rondasJugadas >= MAX_RONDAS_GEMAS;
        float duracion = partidaDecidida ? DURACION_FIN_PARTIDA_GEMAS : DURACION_FIN_RONDA_GEMAS;

        if (tiempoFase < duracion)
        {
            break;
        }

        if (partidaDecidida)
        {
            FinalizarPartidaGemas(*this);
            break;
        }

        ronda++;
        RepartirRondaGemas(*this);
        fase = FASE_GEMAS_REPARTO;
        tiempoFase = 0.0f;
        ReproducirSonidoMinijuego(audio, SONIDO_CUENTA_REGRESIVA);
        break;
    }

    case FASE_GEMAS_TERMINADO:
        break;
    }
}


//==================================================
// VISUAL
//==================================================
//
// Escena compartida: una mesa de joyero de terciopelo con las dos bandejas
// (cada una con el marco del color de su jugador) y, entre ellas, el
// pedestal con el dibujo objetivo. Los cubos se dibujan con (DrawCube) para
// no activar las sombras automaticas de props: las gemas se apoyan sobre una
// bandeja a 0.1 de altura y llevan su propia sombra de contacto al deslizar.
// MODELO FUTURO: bandejas, gemas facetadas, lupas y frascos por modelos GLB.
//==================================================

static void Cubo(float x, float y, float z, float ancho, float alto, float largo, Color color)
{
    (DrawCube)({ x, y, z }, ancho, alto, largo, color);
}


static void TextoContornoGemas(const char* texto, int x, int y, int tamano, Color color)
{
    DrawText(texto, x + 2, y + 2, tamano, Color{ 0, 0, 0, 170 });
    DrawText(texto, x - 1, y, tamano, Color{ 10, 10, 20, 255 });
    DrawText(texto, x + 1, y, tamano, Color{ 10, 10, 20, 255 });
    DrawText(texto, x, y - 1, tamano, Color{ 10, 10, 20, 255 });
    DrawText(texto, x, y + 1, tamano, Color{ 10, 10, 20, 255 });
    DrawText(texto, x, y, tamano, color);
}


static void TextoCentradoGemas(const char* texto, int centroX, int y, int tamano, Color color)
{
    TextoContornoGemas(texto, centroX - MeasureText(texto, tamano) / 2, y, tamano, color);
}


// Gema voxel: base oscura, banda de color, hombros claros y faceta superior
// con la figura grabada del color.
static void DibujarGemaVoxelGemas(
    float x,
    float y,
    float z,
    int color,
    float escala,
    float destello,
    float tamano
)
{
    float s = escala * tamano;

    if (s <= 0.01f)
    {
        return;
    }

    Color base = MezclarGemas(ColorGemaGemas(color), WHITE, destello * 0.85f);
    Color oscuro = OscurecerGemas(base, 0.58f);
    Color claro = MezclarGemas(base, WHITE, 0.24f);
    Color faceta = MezclarGemas(base, WHITE, 0.42f);

    Cubo(x, y + 0.13f * s, z, 1.06f * s, 0.26f * s, 1.06f * s, oscuro);
    Cubo(x, y + 0.36f * s, z, 0.98f * s, 0.20f * s, 0.98f * s, base);
    Cubo(x, y + 0.53f * s, z, 0.80f * s, 0.14f * s, 0.80f * s, claro);
    Cubo(x, y + 0.625f * s, z, 0.64f * s, 0.05f * s, 0.64f * s, faceta);

    if (tamano > 0.8f)
    {
        (DrawCubeWires)({ x, y + 0.36f * s, z }, 0.98f * s, 0.20f * s, 0.98f * s, OscurecerGemas(base, 0.38f));
    }

    // Figura grabada: clara sobre colores oscuros, oscura sobre el topacio.
    Color sinDestello = ColorGemaGemas(color);
    int luminosidad = (sinDestello.r * 30 + sinDestello.g * 59 + sinDestello.b * 11) / 100;
    Color tinta = luminosidad > 165 ? Color{ 52, 36, 18, 255 } : Color{ 255, 255, 255, 255 };
    float punto = 0.115f * s;
    float altura = y + 0.665f * s;

    for (int fila = 0; fila < 5; fila++)
    {
        uint8_t bits = GLIFOS_GEMAS[color][fila];
        int columna = 0;

        while (columna < 5)
        {
            if (((bits >> (4 - columna)) & 1) == 0)
            {
                columna++;
                continue;
            }

            int largo = 0;

            while (columna + largo < 5 && ((bits >> (4 - (columna + largo))) & 1) != 0)
            {
                largo++;
            }

            float cx = x + ((float)columna + (float)largo * 0.5f - 2.5f) * punto;
            Cubo(cx, altura, z + ((float)fila - 2.0f) * punto, (float)largo * punto, 0.04f * s, punto, tinta);
            columna += largo;
        }
    }
}


static void DibujarEscenarioGemas(float tiempo)
{
    // Piso del taller (madera oscura).
    (DrawPlane)({ 0.0f, -1.0f, 0.0f }, { 70.0f, 50.0f }, Color{ 34, 22, 20, 255 });

    // Mesa: terciopelo azul petroleo con ribete dorado.
    Cubo(0.0f, -0.25f, 0.45f, 21.0f, 0.5f, 10.6f, Color{ 22, 52, 66, 255 });
    Cubo(0.0f, -0.40f, 5.9f, 21.4f, 0.8f, 0.34f, Color{ 92, 58, 36, 255 });
    Cubo(0.0f, -0.40f, -4.9f, 21.4f, 0.8f, 0.34f, Color{ 70, 44, 30, 255 });

    Color oro = Color{ 214, 170, 70, 255 };
    Cubo(0.0f, 0.012f, -4.55f, 20.2f, 0.024f, 0.09f, oro);
    Cubo(0.0f, 0.012f, 5.45f, 20.2f, 0.024f, 0.09f, oro);
    Cubo(-10.1f, 0.012f, 0.45f, 0.09f, 0.024f, 10.0f, oro);
    Cubo(10.1f, 0.012f, 0.45f, 0.09f, 0.024f, 10.0f, oro);

    // Pared del fondo con estantes y frascos de gemas.
    Cubo(0.0f, 3.6f, -5.55f, 30.0f, 9.0f, 0.5f, Color{ 56, 36, 30, 255 });
    Cubo(0.0f, 0.55f, -5.27f, 30.0f, 1.3f, 0.08f, Color{ 74, 48, 38, 255 });
    Cubo(0.0f, 1.25f, -5.2f, 30.0f, 0.10f, 0.22f, Color{ 120, 84, 52, 255 });

    if (CalidadDecoracion() != CALIDAD_BAJA)
    {
        const int colores[5] = { 1, 2, 3, 4, 5 };

        for (int estante = 0; estante < 2; estante++)
        {
            float y = estante == 0 ? 1.35f : 3.05f;
            Cubo(0.0f, y - 0.1f, -4.95f, 24.0f, 0.16f, 0.9f, Color{ 112, 76, 46, 255 });

            for (int i = 0; i < 17; i++)
            {
                float x = -8.0f + (float)i * 1.0f;
                int color = colores[(i * 2 + estante * 3) % 5];
                Color vidrio = MezclarGemas(ColorGemaGemas(color), Color{ 20, 20, 30, 255 }, 0.35f);
                Cubo(x, y + 0.3f, -4.95f, 0.5f, 0.6f, 0.5f, vidrio);
                Cubo(x, y + 0.66f, -4.95f, 0.36f, 0.12f, 0.36f, Color{ 190, 150, 70, 255 });
                Cubo(x, y + 0.35f, -4.68f, 0.14f, 0.14f, 0.05f, MezclarGemas(ColorGemaGemas(color), WHITE, 0.55f));
            }
        }
    }

    // Lamparas de mesa en los rincones traseros: luz calida.
    for (int lado = -1; lado <= 1; lado += 2)
    {
        float x = 9.0f * (float)lado;
        Cubo(x, 0.12f, -3.9f, 0.9f, 0.24f, 0.9f, Color{ 96, 62, 36, 255 });
        Cubo(x, 0.9f, -3.9f, 0.14f, 1.5f, 0.14f, oro);
        float parpadeo = 0.92f + 0.08f * std::sin(tiempo * 7.0f + (float)lado);
        (DrawSphere)({ x, 1.85f, -3.9f }, 0.42f, Color{ 255, 226, 150, 255 });
        (DrawSphere)({ x, 1.85f, -3.9f }, 0.75f * parpadeo, Color{ 255, 200, 110, 40 });
    }

    // Lupa y pinzas del joyero en el borde delantero.
    (DrawCylinder)({ -7.9f, 0.05f, 4.55f }, 0.42f, 0.42f, 0.10f, 14, Color{ 196, 152, 66, 255 });
    (DrawCylinder)({ -7.9f, 0.10f, 4.55f }, 0.34f, 0.34f, 0.04f, 14, Color{ 170, 214, 232, 255 });
    Cubo(-7.2f, 0.07f, 4.9f, 1.2f, 0.10f, 0.14f, Color{ 150, 90, 50, 255 });
    Cubo(7.5f, 0.05f, 4.6f, 1.5f, 0.05f, 0.08f, Color{ 200, 205, 214, 255 });
    Cubo(7.5f, 0.05f, 4.74f, 1.5f, 0.05f, 0.08f, Color{ 170, 176, 188, 255 });

    // Gemas sueltas sobre el terciopelo.
    if (CalidadDecoracion() == CALIDAD_ALTA)
    {
        const float sueltas[8][3] =
        {
            { -9.2f, 3.2f, 1 }, { -8.6f, 4.0f, 3 }, { 8.7f, 3.6f, 2 }, { 9.3f, 4.2f, 4 },
            { -2.1f, 4.9f, 5 }, { 2.4f, 4.7f, 1 }, { 9.0f, -2.0f, 3 }, { -9.1f, -2.6f, 2 }
        };

        for (int i = 0; i < 8; i++)
        {
            DibujarGemaVoxelGemas(sueltas[i][0], 0.02f, sueltas[i][1], (int)sueltas[i][2], 1.0f, 0.0f, 0.34f);
        }
    }

    // Charcos de luz calida sobre la mesa (translucidos, siempre al final de la mesa).
    for (int lado = -1; lado <= 1; lado += 2)
    {
        (DrawCylinder)({ 4.7f * (float)lado, 0.022f, 0.8f }, 4.0f, 4.0f, 0.004f, 28, Color{ 255, 210, 140, 16 });
        (DrawCylinder)({ 4.7f * (float)lado, 0.026f, 0.8f }, 3.1f, 3.1f, 0.004f, 28, Color{ 255, 214, 150, 14 });
    }
}


static void DibujarBandejaGemas(
    const MinijuegoGemasDeslizantes& minijuego,
    int ranura,
    Color colorMarco
)
{
    const TableroGemas& tablero = minijuego.tableros[ranura];
    float cx = CentroXTableroGemas(ranura);
    float cz = CENTRO_Z_GEMAS;
    float tiempo = minijuego.tiempoAnimacion;

    Color oro = Color{ 222, 180, 78, 255 };
    Color marcoClaro = MezclarGemas(colorMarco, WHITE, 0.18f);
    Color marcoOscuro = OscurecerGemas(colorMarco, 0.55f);

    // Tapete claro bajo la bandeja y cuerpo del marco.
    Cubo(cx, 0.012f, cz, 5.6f, 0.024f, 5.6f, Color{ 30, 72, 88, 255 });
    Cubo(cx, 0.12f, cz, 4.6f, 0.24f, 4.6f, marcoOscuro);

    // Marco: cuatro barras con el color del jugador y ribete dorado.
    const float mitad = 2.22f;
    Cubo(cx, 0.21f, cz - mitad + 0.21f, 4.44f, 0.42f, 0.42f, colorMarco);
    Cubo(cx, 0.21f, cz + mitad - 0.21f, 4.44f, 0.42f, 0.42f, marcoClaro);
    Cubo(cx - mitad + 0.21f, 0.21f, cz, 0.42f, 0.42f, 3.6f, colorMarco);
    Cubo(cx + mitad - 0.21f, 0.21f, cz, 0.42f, 0.42f, 3.6f, colorMarco);
    Cubo(cx, 0.432f, cz - 1.8f, 3.6f, 0.024f, 0.06f, oro);
    Cubo(cx, 0.432f, cz + 1.8f, 3.6f, 0.024f, 0.06f, oro);
    Cubo(cx - 1.8f, 0.432f, cz, 0.06f, 0.024f, 3.6f, oro);
    Cubo(cx + 1.8f, 0.432f, cz, 0.06f, 0.024f, 3.6f, oro);

    for (int a = -1; a <= 1; a += 2)
    {
        for (int b = -1; b <= 1; b += 2)
        {
            Cubo(cx + (float)a * mitad, 0.3f, cz + (float)b * mitad, 0.3f, 0.6f, 0.3f, oro);
        }
    }

    // Piso aterciopelado de la bandeja y casilleros.
    Cubo(cx, 0.05f, cz, 3.6f, 0.10f, 3.6f, Color{ 24, 22, 40, 255 });

    int hueco = BuscarHuecoGemas(tablero);

    for (int celda = 0; celda < CELDAS_GEMAS; celda++)
    {
        float x = 0.0f;
        float z = 0.0f;
        PosicionCeldaGemas(celda, x, z);
        Cubo(cx + x, 0.11f, cz + z, 1.04f, 0.02f, 1.04f, Color{ 36, 32, 58, 255 });
    }

    // Marca del hueco: contorno pulsante del color del jugador.
    {
        float x = 0.0f;
        float z = 0.0f;
        PosicionCeldaGemas(hueco, x, z);
        float pulso = 0.55f + 0.45f * std::sin(tiempo * 5.0f);
        Color contorno = MezclarGemas(marcoClaro, WHITE, 0.25f * pulso);
        contorno.a = (unsigned char)(150 + 90 * pulso);

        if (tablero.avisoBloqueo > 0.0f)
        {
            contorno = Color{ 255, 70, 70, 255 };
        }

        float lado = 1.08f;
        float grosor = 0.07f;
        Cubo(cx + x, 0.125f, cz + z - lado * 0.5f, lado, 0.02f, grosor, contorno);
        Cubo(cx + x, 0.125f, cz + z + lado * 0.5f, lado, 0.02f, grosor, contorno);
        Cubo(cx + x - lado * 0.5f, 0.125f, cz + z, grosor, 0.02f, lado, contorno);
        Cubo(cx + x + lado * 0.5f, 0.125f, cz + z, grosor, 0.02f, lado, contorno);
    }

    // Placas de acierto bajo las gemas bien puestas.
    for (int celda = 0; celda < CELDAS_GEMAS; celda++)
    {
        int gema = tablero.celda[celda];

        if (gema == GEMA_HUECO || tablero.colorGema[gema] != PATRONES_GEMAS[minijuego.patronActual][celda])
        {
            continue;
        }

        float x = 0.0f;
        float z = 0.0f;
        PosicionCeldaGemas(celda, x, z);
        float brillo = 0.5f + 0.5f * std::sin(tiempo * 3.0f + (float)celda);
        Cubo(cx + x, 0.1275f, cz + z, 1.06f, 0.011f, 1.06f, Color{ 255, 232, 140, (unsigned char)(56 + 22 * brillo) });
    }

    // Gemas (y su sombra de contacto al deslizar).
    for (int gema = 0; gema < GEMAS_POR_TABLERO; gema++)
    {
        float aparicion = 0.1f + 0.9f * Limitar01Gemas(EaseOutBackGemas((minijuego.tiempoReparto - 0.09f * (float)gema) / 0.4f) * 0.97f + 0.03f);
        float x = cx + tablero.px[gema];
        float z = cz + tablero.pz[gema];
        float caida = (1.0f - Limitar01Gemas((minijuego.tiempoReparto - 0.09f * (float)gema) / 0.4f)) * 0.8f;
        float y = ALTURA_GEMAS + tablero.alza[gema] + caida;

        if (tablero.alza[gema] > 0.01f || caida > 0.0f)
        {
            DibujarSombraRetroRectangular({ x, y, z }, 0.9f, 0.9f, 0.0f, 0.14f, 0.34f);
        }

        DibujarGemaVoxelGemas(x, y, z, tablero.colorGema[gema], aparicion, tablero.destello[gema], 0.885f);
    }

    // Perdedor: la bandeja se apaga; ganador con destello extra.
    if (minijuego.fase == FASE_GEMAS_FIN_RONDA && minijuego.ganadorRonda >= 0 && minijuego.ganadorRonda != ranura)
    {
        Cubo(cx, 0.72f, cz, 3.6f, 0.02f, 3.6f, Color{ 8, 8, 20, 110 });
    }
}


static void DibujarReferenciaGemas(const MinijuegoGemasDeslizantes& minijuego)
{
    float cz = CENTRO_Z_GEMAS;
    float tiempo = minijuego.tiempoAnimacion;
    Color oro = Color{ 226, 184, 82, 255 };
    float brillo = Limitar01Gemas(minijuego.brilloReferencia);
    const float alturaPlato = 0.74f;

    // Halo dorado y pedestal.
    float latido = 0.5f + 0.5f * std::sin(tiempo * 2.4f);
    (DrawCylinder)({ 0.0f, 0.024f, cz }, 2.0f, 2.0f, 0.004f, 24, Color{ 255, 214, 120, (unsigned char)(18 + 20 * latido + 60 * brillo) });
    Cubo(0.0f, 0.22f, cz, 3.2f, 0.44f, 3.2f, Color{ 98, 64, 40, 255 });
    Cubo(0.0f, 0.47f, cz, 2.9f, 0.10f, 2.9f, oro);
    Cubo(0.0f, 0.60f, cz, 2.68f, 0.16f, 2.68f, Color{ 30, 26, 48, 255 });

    // Marco dorado de la ventana.
    const float lado = 2.68f;
    Cubo(0.0f, 0.70f, cz - lado * 0.5f, lado, 0.08f, 0.08f, oro);
    Cubo(0.0f, 0.70f, cz + lado * 0.5f, lado, 0.08f, 0.08f, oro);
    Cubo(-lado * 0.5f, 0.70f, cz, 0.08f, 0.08f, lado, oro);
    Cubo(lado * 0.5f, 0.70f, cz, 0.08f, 0.08f, lado, oro);

    for (int celda = 0; celda < CELDAS_GEMAS; celda++)
    {
        float x = (float)(celda % 3 - 1) * 0.86f;
        float z = (float)(celda / 3 - 1) * 0.86f;
        int color = PATRONES_GEMAS[minijuego.patronActual][celda];

        if (color == 0)
        {
            Cubo(x, alturaPlato - 0.02f, cz + z, 0.72f, 0.02f, 0.72f, Color{ 44, 38, 70, 255 });
            continue;
        }

        float rebote = brillo > 0.0f ? 0.08f * std::sin(tiempo * 12.0f + (float)celda) * brillo : 0.0f;
        float aparicion = EaseOutBackGemas((minijuego.tiempoReparto - 0.05f * (float)celda) / 0.4f);
        DibujarGemaVoxelGemas(x, alturaPlato + rebote, cz + z, color, aparicion, brillo * 0.35f, 0.74f);
    }
}


static void DibujarParticulasGemas(const MinijuegoGemasDeslizantes& minijuego)
{
    for (int i = 0; i < MAX_PARTICULAS_GEMAS; i++)
    {
        const ParticulaGemas& p = minijuego.particulas[i];

        if (!p.activa)
        {
            continue;
        }

        float vida = Limitar01Gemas(p.vida / p.vidaMaxima);
        Color color = p.color;

        if (p.tipo == 1)
        {
            // Destello: cruz de cuatro puntas que se encoge.
            float tam = p.tamano * (0.4f + vida);
            color.a = (unsigned char)(255 * vida);
            Cubo(p.posicion.x, p.posicion.y, p.posicion.z, tam * 2.6f, tam * 0.5f, tam * 0.5f, color);
            Cubo(p.posicion.x, p.posicion.y, p.posicion.z, tam * 0.5f, tam * 2.6f, tam * 0.5f, color);
            Cubo(p.posicion.x, p.posicion.y, p.posicion.z, tam * 0.5f, tam * 0.5f, tam * 2.6f, color);
        }
        else
        {
            color.a = (unsigned char)(255 * (vida > 0.3f ? 1.0f : vida / 0.3f));
            float giro = (float)i * 0.7f + minijuego.tiempoAnimacion * 5.0f;
            float tam = p.tamano;
            Cubo(
                p.posicion.x, p.posicion.y, p.posicion.z,
                tam * (1.0f + 0.5f * std::sin(giro)),
                tam * 0.5f,
                tam * (1.0f + 0.5f * std::cos(giro)),
                color
            );
        }
    }
}


static void RombosGemas(int centroX, int centroY, int radio, Color relleno, Color borde)
{
    Vector2 puntos[4] = {
        { (float)centroX, (float)(centroY - radio) },
        { (float)(centroX - radio), (float)centroY },
        { (float)centroX, (float)(centroY + radio) },
        { (float)(centroX + radio), (float)centroY }
    };

    DrawTriangle(puntos[0], puntos[1], puntos[2], relleno);
    DrawTriangle(puntos[0], puntos[2], puntos[3], relleno);
    DrawLineEx(puntos[0], puntos[1], 2.0f, borde);
    DrawLineEx(puntos[1], puntos[2], 2.0f, borde);
    DrawLineEx(puntos[2], puntos[3], 2.0f, borde);
    DrawLineEx(puntos[3], puntos[0], 2.0f, borde);
}


void MinijuegoGemasDeslizantes::Dibujar(
    const JugadorPrueba jugadores[],
    int cantidadMaxima,
    const Participante participantes[],
    bool mostrarDebug
) const
{
    (void)jugadores;
    (void)cantidadMaxima;
    (void)mostrarDebug;

    ClearBackground(Color{ 14, 10, 18, 255 });

    if (resultado.estado == RESULTADO_MINIJUEGO_CANCELADO)
    {
        return;
    }

    BeginMode3D(camara);

    DibujarEscenarioGemas(tiempoAnimacion);

    for (int ranura = 0; ranura < 2; ranura++)
    {
        Color marco = participantes[tableros[ranura].participante].color;
        DibujarBandejaGemas(*this, ranura, marco);
    }

    DibujarReferenciaGemas(*this);
    DibujarParticulasGemas(*this);

    EndMode3D();

    int ancho = GetScreenWidth();
    int alto = GetScreenHeight();
    float escalaUI = (float)alto / 720.0f;

    // Placa del dibujo sobre el pedestal.
    {
        Vector2 placa = GetWorldToScreen({ 0.0f, 1.0f, CENTRO_Z_GEMAS - 1.85f }, camara);
        int tamano = (int)(24.0f * escalaUI);
        const char* nombre = NOMBRES_PATRONES_GEMAS[patronActual];
        int anchoTexto = MeasureText(nombre, tamano);
        int cx = (int)placa.x;
        int y = (int)placa.y - tamano - 4;

        DrawRectangle(cx - anchoTexto / 2 - 12, y - 4, anchoTexto + 24, tamano + 10, Color{ 24, 16, 30, 215 });
        DrawRectangleLines(cx - anchoTexto / 2 - 12, y - 4, anchoTexto + 24, tamano + 10, Color{ 226, 184, 82, 255 });
        DrawText(nombre, cx - anchoTexto / 2, y + 1, tamano, Color{ 255, 232, 170, 255 });
    }

    // Rondas ganadas bajo cada bandeja.
    for (int ranura = 0; ranura < 2; ranura++)
    {
        Vector2 punto = GetWorldToScreen(
            { CentroXTableroGemas(ranura), 0.2f, CENTRO_Z_GEMAS + 2.85f },
            camara
        );
        int radio = (int)(11.0f * escalaUI);
        Color jugador = participantes[tableros[ranura].participante].color;

        for (int i = 0; i < RONDAS_PARA_GANAR_GEMAS; i++)
        {
            int x = (int)punto.x + (int)((float)(i * 2 - 1) * 20.0f * escalaUI);
            bool ganada = tableros[ranura].victorias > i;
            RombosGemas(
                x,
                (int)punto.y,
                radio,
                ganada ? jugador : Color{ 24, 22, 38, 220 },
                ganada ? WHITE : Color{ 150, 150, 170, 255 }
            );
        }
    }

    // Aviso grande de la ronda (sobre el pedestal, sin tapar las bandejas).
    {
        Vector2 aviso = GetWorldToScreen({ 0.0f, 1.0f, CENTRO_Z_GEMAS - 3.3f }, camara);
        int centroX = (int)aviso.x;
        int y = (int)aviso.y;

        if (fase == FASE_GEMAS_REPARTO || (fase == FASE_GEMAS_PREPARACION && tiempoPreparacion > 0.0f))
        {
            const char* texto = TextFormat("RONDA %d", ronda + 1);
            TextoCentradoGemas(texto, centroX, y - (int)(18.0f * escalaUI), (int)(40.0f * escalaUI), Color{ 255, 226, 120, 255 });
        }
        else if (fase == FASE_GEMAS_FIN_RONDA && ganadorRonda >= 0)
        {
            int indice = tableros[ganadorRonda].participante;
            const char* texto = TextFormat("¡RONDA PARA J%d!", participantes[indice].numeroJugador);
            TextoCentradoGemas(
                texto,
                centroX,
                y - (int)(18.0f * escalaUI),
                (int)(34.0f * escalaUI),
                participantes[indice].color
            );

            if (rondaPorTiempo)
            {
                TextoCentradoGemas(
                    "SE ACABÓ EL TIEMPO",
                    centroX,
                    y + (int)(18.0f * escalaUI),
                    (int)(20.0f * escalaUI),
                    RAYWHITE
                );
            }
        }
    }

    // HUD comun.
    DatosHudMinijuego hud;
    PrepararDatosHudMinijuego(
        hud,
        "GEMAS DESLIZANTES",
        "DESLIZÁ LAS GEMAS Y ARMÁ EL DIBUJO DEL CENTRO. GANA QUIEN SE LLEVE 2 RONDAS.",
        resultado
    );

    hud.fase = fase == FASE_GEMAS_PREPARACION
        ? HUD_MINIJUEGO_CUENTA
        : (fase == FASE_GEMAS_TERMINADO ? HUD_MINIJUEGO_FIN : HUD_MINIJUEGO_JUGANDO);
    hud.cuentaRestante = tiempoPreparacion;
    hud.reloj = HUD_RELOJ_CUENTA_ATRAS;
    hud.tiempoRestante = tiempoRestante > 0.0f ? tiempoRestante : 0.0f;
    hud.tiempoTotal = DURACION_RONDA_GEMAS;
    hud.segundosAlerta = 8.0f;
    hud.finPorTiempo = false;

    for (int ranura = 0; ranura < 2; ranura++)
    {
        const TableroGemas& tablero = tableros[ranura];
        int indice = tablero.participante;

        if (indice < 0)
        {
            continue;
        }

        JugadorHudMinijuego& jugador = hud.jugadores[indice];

        if (!jugador.visible)
        {
            continue;
        }

        std::snprintf(jugador.valor, sizeof(jugador.valor), "%d/%d", tablero.victorias, RONDAS_PARA_GANAR_GEMAS);
        jugador.valorNumerico = tablero.victorias;
        jugador.progreso = (float)tablero.correctas / (float)GEMAS_POR_TABLERO;
        std::snprintf(
            jugador.detalleFinal,
            sizeof(jugador.detalleFinal),
            tablero.victorias == 1 ? "%d RONDA" : "%d RONDAS",
            tablero.victorias
        );

        if (!participantes[indice].esBot)
        {
            std::snprintf(
                jugador.controles,
                sizeof(jugador.controles),
                "DESLIZAR: %s",
                TextoMoverHud(participantes[indice])
            );
        }
    }

    (void)ancho;
    DibujarHudMinijuego(hud, participantes);
}


const ResultadoMinijuego&
MinijuegoGemasDeslizantes::ObtenerResultado() const
{
    return resultado;
}
