#include "Board/Tablero.h"

#include "Systems/CalidadGrafica.h"

#include <cmath>
#include <cstring>


//==================================================
// AGREGAR CASILLA
//==================================================

int Tablero::AgregarCasilla(
    Vector3 posicion,
    TipoCasilla tipo
)
{
    if (
        cantidadCasillas < 0 ||
        cantidadCasillas >= MAX_CASILLAS_TABLERO
    )
    {
        return -1;
    }

    int indice =
        cantidadCasillas;

    Casilla& casilla =
        casillas[indice];

    casilla =
        Casilla{};

    casilla.indice =
        indice;

    casilla.posicion =
        posicion;

    casilla.tipo =
        tipo;

    cantidadCasillas++;

    return indice;
}


//==================================================
// CONECTAR CASILLAS
//==================================================

bool Tablero::ConectarCasillas(
    int origen,
    int destino
)
{
    if (
        origen < 0 ||
        origen >= cantidadCasillas ||
        destino < 0 ||
        destino >= cantidadCasillas
    )
    {
        return false;
    }

    Casilla& casillaOrigen =
        casillas[origen];

    if (
        casillaOrigen.cantidadConexiones >=
        MAX_CONEXIONES_CASILLA
    )
    {
        return false;
    }

    for (
        int i = 0;
        i < casillaOrigen.cantidadConexiones;
        i++
    )
    {
        if (
            casillaOrigen.conexiones[i].destino ==
            destino
        )
        {
            return false;
        }
    }

    casillaOrigen
        .conexiones[
            casillaOrigen.cantidadConexiones
        ]
        .destino =
        destino;

    casillaOrigen.cantidadConexiones++;

    return true;
}


//==================================================
// INICIALIZAR PROTOTIPO
//==================================================

void Tablero::InicializarPrototipo()
{
    cantidadCasillas =
        0;

    recorridoValido =
        false;

    for (
        int i = 0;
        i < MAX_CASILLAS_TABLERO;
        i++
    )
    {
        casillas[i] =
            Casilla{};
    }

    // Circuito exterior.
    AgregarCasilla({ -8.0f, 0.25f,  5.0f }, CASILLA_ESPECIAL); // 0
    AgregarCasilla({ -5.0f, 0.25f,  5.0f }, CASILLA_NEUTRA);   // 1
    AgregarCasilla({ -2.0f, 0.25f,  5.0f }, CASILLA_POSITIVA); // 2
    AgregarCasilla({  1.0f, 0.25f,  5.0f }, CASILLA_NEUTRA);   // 3
    AgregarCasilla({  4.0f, 0.25f,  5.0f }, CASILLA_NEGATIVA); // 4
    AgregarCasilla({  7.0f, 0.25f,  5.0f }, CASILLA_POSITIVA); // 5
    AgregarCasilla({  8.0f, 0.25f,  2.0f }, CASILLA_NEUTRA);   // 6
    AgregarCasilla({  8.0f, 0.25f, -1.0f }, CASILLA_NEGATIVA); // 7
    AgregarCasilla({  8.0f, 0.25f, -4.0f }, CASILLA_POSITIVA); // 8
    AgregarCasilla({  5.0f, 0.25f, -5.0f }, CASILLA_NEUTRA);   // 9
    AgregarCasilla({  2.0f, 0.25f, -5.0f }, CASILLA_NEGATIVA); // 10
    AgregarCasilla({ -1.0f, 0.25f, -5.0f }, CASILLA_POSITIVA); // 11
    AgregarCasilla({ -4.0f, 0.25f, -5.0f }, CASILLA_NEUTRA);   // 12
    AgregarCasilla({ -7.0f, 0.25f, -5.0f }, CASILLA_NEGATIVA); // 13
    AgregarCasilla({ -8.0f, 0.25f, -2.0f }, CASILLA_POSITIVA); // 14
    AgregarCasilla({ -8.0f, 0.25f,  2.0f }, CASILLA_NEUTRA);   // 15

    // Ruta interior de la bifurcacion.
    AgregarCasilla({  3.0f, 0.25f,  2.5f }, CASILLA_NEGATIVA); // 16
    AgregarCasilla({  4.5f, 0.25f,  0.0f }, CASILLA_POSITIVA); // 17
    AgregarCasilla({  6.0f, 0.25f, -2.5f }, CASILLA_NEUTRA);   // 18

    for (
        int i = 0;
        i < 15;
        i++
    )
    {
        ConectarCasillas(
            i,
            i + 1
        );
    }

    ConectarCasillas(
        15,
        0
    );

    // La casilla 3 permite elegir entre el circuito
    // exterior y el atajo interior.
    ConectarCasillas(
        3,
        16
    );

    ConectarCasillas(
        16,
        17
    );

    ConectarCasillas(
        17,
        18
    );

    ConectarCasillas(
        18,
        8
    );

    recorridoValido =
        ValidarRecorrido();
}


//==================================================
// OBTENER CASILLA
//==================================================

const Casilla* Tablero::ObtenerCasilla(
    int indice
) const
{
    if (
        indice < 0 ||
        indice >= cantidadCasillas
    )
    {
        return nullptr;
    }

    return &casillas[indice];
}


//==================================================
// VALIDAR RECORRIDO
//==================================================

bool Tablero::ValidarRecorrido()
{
    if (
        cantidadCasillas <= 0 ||
        cantidadCasillas > MAX_CASILLAS_TABLERO
    )
    {
        recorridoValido =
            false;

        return false;
    }

    bool visitadas[
        MAX_CASILLAS_TABLERO
    ] = {};

    int pendientes[
        MAX_CASILLAS_TABLERO
    ] = {};

    int inicioCola =
        0;

    int finCola =
        0;

    pendientes[finCola] =
        0;

    finCola++;

    visitadas[0] =
        true;

    while (inicioCola < finCola)
    {
        int indiceActual =
            pendientes[inicioCola];

        inicioCola++;

        const Casilla& casilla =
            casillas[indiceActual];

        if (
            casilla.cantidadConexiones <= 0 ||
            casilla.cantidadConexiones > MAX_CONEXIONES_CASILLA
        )
        {
            recorridoValido =
                false;

            return false;
        }

        for (
            int i = 0;
            i < casilla.cantidadConexiones;
            i++
        )
        {
            int destino =
                casilla.conexiones[i].destino;

            if (
                destino < 0 ||
                destino >= cantidadCasillas
            )
            {
                recorridoValido =
                    false;

                return false;
            }

            if (!visitadas[destino])
            {
                visitadas[destino] =
                    true;

                pendientes[finCola] =
                    destino;

                finCola++;
            }
        }
    }

    for (
        int i = 0;
        i < cantidadCasillas;
        i++
    )
    {
        if (!visitadas[i])
        {
            recorridoValido =
                false;

            return false;
        }
    }

    recorridoValido =
        true;

    return true;
}


//==================================================
// DIBUJAR
//==================================================

void Tablero::Dibujar() const
{
    DrawPlane(
        Vector3{
            0.0f,
            -0.05f,
            0.0f
        },
        Vector2{
            22.0f,
            16.0f
        },
        Color{
            48,
            72,
            78,
            255
        }
    );

    DibujarRuta();
}


//==================================================
// DIBUJAR RUTA
//==================================================

void Tablero::DibujarRuta() const
{
    DibujarRuta(
        ObtenerEstiloCasillaPiedra(),
        0.0f
    );
}


//==================================================
// CACHE DE MALLAS DE LA RUTA
//==================================================
//
// Senderos y losas no cambian entre frames: se construyen una vez y se
// dibujan desde la GPU. Solo el pulso de la incrustacion reescribe
// colores. Hay unas pocas entradas para que la vista previa del menu y la
// partida no se pisen. La huella cubre casillas, conexiones, estilo y
// calidad, asi que cualquier cambio reconstruye la entrada.

struct CacheRutaTablero
{
    bool ocupada = false;
    unsigned int huella = 0;
    unsigned int ultimoUso = 0;

    ConstructorMalla fija;
    ConstructorMalla incrustacion;
    MallaGpu mallaFija;
    MallaGpu mallaIncrustacion;

    int inicioIncrustacion[MAX_CASILLAS_TABLERO];
    float tiempoColores = -1.0e9f;
};

static const int ENTRADAS_CACHE_RUTA = 3;

// En el modo inmediato anterior las cintas se dibujaban con el culling
// "desactivado" solo entre llamadas (raylib lo aplica al vaciar el lote),
// asi que quedaban de espaldas a la camara y no se veian nunca. Se mantiene
// ese aspecto: poner a true muestra los senderos de tierra de verdad (las
// mallas son de dos caras).
static const bool SENDEROS_VISIBLES = false;

static CacheRutaTablero cachesRuta[ENTRADAS_CACHE_RUTA];
static unsigned int contadorUsoCacheRuta = 0;


static void MezclarHuella(unsigned int& h, unsigned int valor)
{
    h ^= valor + 0x9e3779b9u + (h << 6) + (h >> 2);
}


static void MezclarHuella(unsigned int& h, float valor)
{
    unsigned int bits;
    std::memcpy(&bits, &valor, sizeof(bits));
    MezclarHuella(h, bits);
}


static void MezclarHuella(unsigned int& h, Color c)
{
    MezclarHuella(h, (unsigned int)(c.r | (c.g << 8) | (c.b << 16) | (c.a << 24)));
}


static unsigned int CalcularHuellaRuta(
    const Tablero& tablero,
    const EstiloCasilla& estilo,
    bool conSombra,
    bool conMusgo
)
{
    unsigned int h = 2166136261u;

    MezclarHuella(h, (unsigned int)tablero.cantidadCasillas);
    MezclarHuella(h, (unsigned int)(conSombra ? 1 : 0) + (conMusgo ? 2u : 0u));

    MezclarHuella(h, estilo.piedraLado);
    MezclarHuella(h, estilo.piedraTapa);
    MezclarHuella(h, estilo.borde);
    MezclarHuella(h, estilo.detalle);
    MezclarHuella(h, estilo.sendero);
    MezclarHuella(h, estilo.senderoBorde);
    MezclarHuella(h, (unsigned int)(SENDEROS_VISIBLES ? 1 : 0));
    MezclarHuella(h, estilo.radio);
    MezclarHuella(h, estilo.altura);
    MezclarHuella(h, estilo.anchoSendero);

    for (int i = 0; i < tablero.cantidadCasillas; i++)
    {
        const Casilla& c = tablero.casillas[i];

        MezclarHuella(h, c.posicion.x);
        MezclarHuella(h, c.posicion.y);
        MezclarHuella(h, c.posicion.z);
        MezclarHuella(h, (unsigned int)c.tipo);
        MezclarHuella(h, (unsigned int)c.cantidadConexiones);

        for (int k = 0; k < c.cantidadConexiones; k++)
        {
            MezclarHuella(h, (unsigned int)c.conexiones[k].destino);
        }
    }

    return h;
}


static void ConstruirCacheRuta(
    CacheRutaTablero& cache,
    const Tablero& tablero,
    const EstiloCasilla& estilo,
    bool conSombra,
    bool conMusgo
)
{
    cache.fija.Limpiar();
    cache.incrustacion.Limpiar();

    // Senderos: cinta de tierra con borde mas oscuro debajo.
    for (int i = 0; SENDEROS_VISIBLES && i < tablero.cantidadCasillas; i++)
    {
        const Casilla& casilla = tablero.casillas[i];

        for (int k = 0; k < casilla.cantidadConexiones; k++)
        {
            const Casilla* destino =
                tablero.ObtenerCasilla(casilla.conexiones[k].destino);

            if (destino == nullptr)
            {
                continue;
            }

            AgregarCintaSueloMalla(
                cache.fija,
                casilla.posicion, destino->posicion,
                estilo.anchoSendero + 0.22f, 0.035f, estilo.senderoBorde
            );

            AgregarCintaSueloMalla(
                cache.fija,
                casilla.posicion, destino->posicion,
                estilo.anchoSendero, 0.050f, estilo.sendero
            );
        }
    }

    for (int i = 0; i < tablero.cantidadCasillas; i++)
    {
        cache.inicioIncrustacion[i] = cache.incrustacion.CantidadVertices();

        AgregarCasillaTematicaMalla(
            cache.fija, cache.incrustacion,
            tablero.casillas[i], estilo,
            conSombra, conMusgo
        );
    }

    ActualizarMallaGpu(cache.mallaFija, cache.fija);
    ActualizarMallaGpu(cache.mallaIncrustacion, cache.incrustacion);
    cache.tiempoColores = -1.0e9f;
}


static void ReescribirColoresIncrustacion(
    CacheRutaTablero& cache,
    const Tablero& tablero,
    const EstiloCasilla& estilo,
    float tiempo
)
{
    // Cada cilindro de la incrustacion tiene 14 lados * 12 vertices.
    const int VERTICES_CILINDRO = 14 * 12;

    for (int i = 0; i < tablero.cantidadCasillas; i++)
    {
        Color claro;
        Color normal;

        ObtenerColoresIncrustacionCasilla(
            tablero.casillas[i], estilo, tiempo, claro, normal
        );

        unsigned char* destino =
            cache.incrustacion.colores.data() + cache.inicioIncrustacion[i] * 4;

        for (int v = 0; v < VERTICES_CILINDRO * 2; v++)
        {
            Color c = v < VERTICES_CILINDRO ? claro : normal;

            destino[v * 4 + 0] = c.r;
            destino[v * 4 + 1] = c.g;
            destino[v * 4 + 2] = c.b;
            destino[v * 4 + 3] = 255;
        }
    }

    ActualizarColoresMallaGpu(cache.mallaIncrustacion, cache.incrustacion);
    cache.tiempoColores = tiempo;
}


void DescargarMallasRutaTablero()
{
    for (CacheRutaTablero& cache : cachesRuta)
    {
        DescargarMalla(cache.mallaFija);
        DescargarMalla(cache.mallaIncrustacion);
        cache.fija = ConstructorMalla{};
        cache.incrustacion = ConstructorMalla{};
        cache.ocupada = false;
    }
}


void Tablero::DibujarRuta(
    const EstiloCasilla& estilo,
    float tiempo
) const
{
    // Calidad: BAJA sin sombra de contacto ni musgo (detalle menudo).
    bool conSombra = CalidadSombras() != CALIDAD_BAJA;
    bool conMusgo = CalidadDecoracion() != CALIDAD_BAJA;

    unsigned int huella = CalcularHuellaRuta(*this, estilo, conSombra, conMusgo);

    CacheRutaTablero* cache = nullptr;
    CacheRutaTablero* masAntigua = &cachesRuta[0];

    for (CacheRutaTablero& c : cachesRuta)
    {
        if (c.ocupada && c.huella == huella)
        {
            cache = &c;
            break;
        }

        if (!c.ocupada || c.ultimoUso < masAntigua->ultimoUso)
        {
            masAntigua = &c;
        }
    }

    if (cache == nullptr)
    {
        cache = masAntigua;
        ConstruirCacheRuta(*cache, *this, estilo, conSombra, conMusgo);
        cache->huella = huella;
        cache->ocupada = true;
    }

    cache->ultimoUso = ++contadorUsoCacheRuta;

    if (tiempo != cache->tiempoColores)
    {
        ReescribirColoresIncrustacion(*cache, *this, estilo, tiempo);
    }

    DibujarMallaGpu(cache->mallaFija);
    DibujarMallaGpu(cache->mallaIncrustacion);
}
