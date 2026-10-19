#include "Minigames/MinijuegoCablesCruzados.h"

#include "Minigames/AudioMinijuegos.h"
#include "Minigames/HudMinijuego.h"
#include "Minigames/SombrasRetro.h"
#include "Systems/CalidadGrafica.h"
#include "Systems/Input.h"

#include "raylib.h"
#include "rlgl.h"

#include <cmath>
#include <cstdio>


//==================================================
// CABLES CRUZADOS
//==================================================
//
// Duelo de puzzle de circuitos: dos paneles con el mismo tablero de piezas
// de cable desordenadas; gana la ronda quien primero conecta la pila con la
// bombilla. Mejor de 3. Las reglas completas estan en el .h.
//==================================================


static const float DURACION_PREPARACION_CABLES = 3.0f;
static const float DURACION_INTRO_RONDA_CABLES = 1.1f;
static const float DURACION_FIN_RONDA_CABLES = 1.9f;
static const float DURACION_FIN_PARTIDA_CABLES = 1.5f;

static const float DURACION_RONDA_CABLES[MAX_RONDAS_CABLES] = { 20.0f, 24.0f, 28.0f };
static const int COLUMNAS_RONDA_CABLES[MAX_RONDAS_CABLES] = { 4, 5, 5 };
static const int FILAS_RONDA_CABLES[MAX_RONDAS_CABLES] = { 4, 4, 5 };
static const int LARGO_MIN_RONDA_CABLES[MAX_RONDAS_CABLES] = { 6, 7, 9 };
static const int LARGO_MAX_RONDA_CABLES[MAX_RONDAS_CABLES] = { 8, 10, 12 };
static const int CLICS_MIN_RONDA_CABLES[MAX_RONDAS_CABLES] = { 5, 6, 8 };

static const float CENTRO_PANEL_X_CABLES = 3.9f;

static const int DX_CABLES[4] = { 0, 1, 0, -1 };
static const int DZ_CABLES[4] = { -1, 0, 1, 0 };


//==================================================
// LOGICA PURA DEL TABLERO
//==================================================


struct RngCables
{
    unsigned int s = 1;
};


static RngCables CrearRngCables(unsigned int semilla)
{
    RngCables rng;
    rng.s = semilla * 2654435761u ^ 0x9E3779B9u;

    if (rng.s == 0)
    {
        rng.s = 1;
    }

    for (int i = 0; i < 6; i++)
    {
        rng.s ^= rng.s << 13;
        rng.s ^= rng.s >> 17;
        rng.s ^= rng.s << 5;
    }

    return rng;
}


static unsigned int SiguienteRngCables(RngCables& rng)
{
    rng.s ^= rng.s << 13;
    rng.s ^= rng.s >> 17;
    rng.s ^= rng.s << 5;
    return rng.s;
}


static int RangoRngCables(RngCables& rng, int minimo, int maximo)
{
    return minimo + (int)(SiguienteRngCables(rng) % (unsigned int)(maximo - minimo + 1));
}


static int OpuestoCables(int direccion)
{
    return (direccion + 2) & 3;
}


static unsigned char BitCables(int direccion)
{
    return (unsigned char)(1 << direccion);
}


static int ModuloCables(int valor, int divisor)
{
    int resto = valor % divisor;
    return resto < 0 ? resto + divisor : resto;
}


unsigned char MascaraGiradaCables(unsigned char base, int giro)
{
    unsigned int mascara = base & 15u;
    int vueltas = ModuloCables(giro, 4);

    for (int i = 0; i < vueltas; i++)
    {
        mascara = ((mascara << 1) | (mascara >> 3)) & 15u;
    }

    return (unsigned char)mascara;
}


static unsigned char MascaraActualCables(const TableroCables& tablero, int celda)
{
    if (tablero.fija[celda])
    {
        return tablero.base[celda];
    }

    return MascaraGiradaCables(tablero.base[celda], tablero.giro[celda]);
}


// Menor cantidad de giros (en cualquier sentido) para que la pieza tenga los
// lados pedidos; -1 si ningun giro lo logra.
static int MejorGiroCables(
    unsigned char base,
    int giroActual,
    unsigned char necesarios,
    unsigned char& giroObjetivo
)
{
    int mejor = -1;

    for (int g = 0; g < 4; g++)
    {
        if ((MascaraGiradaCables(base, g) & necesarios) != necesarios)
        {
            continue;
        }

        int horario = ModuloCables(g - giroActual, 4);
        int clics = horario <= 2 ? horario : 4 - horario;

        if (mejor < 0 || clics < mejor)
        {
            mejor = clics;
            giroObjetivo = (unsigned char)g;
        }
    }

    return mejor;
}


struct BusquedaCaminoCables
{
    RngCables* rng = nullptr;
    int columnas = 4;
    int filas = 4;
    int origen = 0;
    int bombilla = 0;
    int objetivo = 0;
    int largoMin = 0;
    int largoMax = 0;
    int camino[MAX_CELDAS_CABLES]{};
    int largo = 0;
    bool visitado[MAX_CELDAS_CABLES]{};
    int nodos = 0;
};


static bool ExplorarCaminoCables(BusquedaCaminoCables& b, int celda)
{
    if (++b.nodos > 8000)
    {
        return false;
    }

    if (celda == b.objetivo)
    {
        return b.largo >= b.largoMin;
    }

    if (b.largo >= b.largoMax)
    {
        return false;
    }

    int orden[4] = { 0, 1, 2, 3 };

    for (int i = 3; i > 0; i--)
    {
        int j = RangoRngCables(*b.rng, 0, i);
        int temporal = orden[i];
        orden[i] = orden[j];
        orden[j] = temporal;
    }

    int columna = celda % b.columnas;
    int fila = celda / b.columnas;
    int columnaObjetivo = b.objetivo % b.columnas;
    int filaObjetivo = b.objetivo / b.columnas;

    for (int k = 0; k < 4; k++)
    {
        int d = orden[k];
        int nc = columna + DX_CABLES[d];
        int nf = fila + DZ_CABLES[d];

        if (nc < 0 || nc >= b.columnas || nf < 0 || nf >= b.filas)
        {
            continue;
        }

        int vecina = nf * b.columnas + nc;

        if (b.visitado[vecina] || vecina == b.origen || vecina == b.bombilla)
        {
            continue;
        }

        int restante = std::abs(nc - columnaObjetivo) + std::abs(nf - filaObjetivo);

        if (b.largo + 1 + restante > b.largoMax)
        {
            continue;
        }

        b.visitado[vecina] = true;
        b.camino[b.largo++] = vecina;

        if (ExplorarCaminoCables(b, vecina))
        {
            return true;
        }

        b.largo--;
        b.visitado[vecina] = false;
    }

    return false;
}


static int DireccionEntreCables(int columnas, int a, int b)
{
    int dx = b % columnas - a % columnas;
    int dz = b / columnas - a / columnas;

    for (int d = 0; d < 4; d++)
    {
        if (DX_CABLES[d] == dx && DZ_CABLES[d] == dz)
        {
            return d;
        }
    }

    return 1;
}


struct SolverCables
{
    const TableroCables* tablero = nullptr;
    SolucionCables* mejores = nullptr;
    int maximo = 0;
    int encontradas = 0;
    int celdas[MAX_CELDAS_CABLES]{};
    unsigned char giros[MAX_CELDAS_CABLES]{};
    int largo = 0;
    bool visitado[MAX_CELDAS_CABLES]{};
};


static void RegistrarSolucionCables(SolverCables& s, int costo)
{
    // Inserta ordenado por giros; descarta si no entra entre las mejores.
    int posicion = s.encontradas;

    while (posicion > 0 && s.mejores[posicion - 1].clics > costo)
    {
        posicion--;
    }

    if (posicion >= s.maximo)
    {
        return;
    }

    int ultimo = s.encontradas < s.maximo ? s.encontradas : s.maximo - 1;

    for (int i = ultimo; i > posicion; i--)
    {
        s.mejores[i] = s.mejores[i - 1];
    }

    SolucionCables& destino = s.mejores[posicion];
    destino.cantidad = s.largo;
    destino.clics = costo;

    for (int i = 0; i < s.largo; i++)
    {
        destino.celdas[i] = s.celdas[i];
        destino.giro[i] = s.giros[i];
    }

    if (s.encontradas < s.maximo)
    {
        s.encontradas++;
    }
}


static void RecorrerSolucionCables(SolverCables& s, int celda, int direccionMovida, int costo)
{
    const TableroCables& t = *s.tablero;

    if (s.encontradas >= s.maximo && costo >= s.mejores[s.maximo - 1].clics)
    {
        return;
    }

    if (celda == t.bombilla)
    {
        if (t.base[celda] & BitCables(OpuestoCables(direccionMovida)))
        {
            RegistrarSolucionCables(s, costo);
        }

        return;
    }

    int entrada = OpuestoCables(direccionMovida);
    int columna = celda % t.columnas;
    int fila = celda / t.columnas;

    for (int d = 0; d < 4; d++)
    {
        if (d == entrada)
        {
            continue;
        }

        int nc = columna + DX_CABLES[d];
        int nf = fila + DZ_CABLES[d];

        if (nc < 0 || nc >= t.columnas || nf < 0 || nf >= t.filas)
        {
            continue;
        }

        int vecina = nf * t.columnas + nc;

        if (s.visitado[vecina] || vecina == t.origen)
        {
            continue;
        }

        unsigned char objetivo = 0;
        int clics = MejorGiroCables(
            t.base[celda],
            t.giro[celda],
            (unsigned char)(BitCables(entrada) | BitCables(d)),
            objetivo
        );

        if (clics < 0)
        {
            continue;
        }

        s.visitado[vecina] = true;
        s.celdas[s.largo] = celda;
        s.giros[s.largo] = objetivo;
        s.largo++;

        RecorrerSolucionCables(s, vecina, d, costo + clics);

        s.largo--;
        s.visitado[vecina] = false;
    }
}


int ResolverTableroCables(
    const TableroCables& tablero,
    SolucionCables soluciones[],
    int maximo
)
{
    if (maximo <= 0 || tablero.columnas < 3)
    {
        return 0;
    }

    SolverCables solver;
    solver.tablero = &tablero;
    solver.mejores = soluciones;
    solver.maximo = maximo;

    // La pila solo sale por el este.
    int primera = tablero.origen + 1;
    solver.visitado[tablero.origen] = true;
    solver.visitado[primera] = true;
    RecorrerSolucionCables(solver, primera, 1, 0);

    return solver.encontradas;
}


void AplicarSolucionCables(TableroCables& tablero, const SolucionCables& solucion)
{
    for (int i = 0; i < solucion.cantidad; i++)
    {
        tablero.giro[solucion.celdas[i]] = solucion.giro[i];
    }
}


int CalcularEnergiaCables(
    const TableroCables& tablero,
    int distancia[],
    int* profundidad
)
{
    int celdas = tablero.columnas * tablero.filas;
    int cola[MAX_CELDAS_CABLES];
    int cabeza = 0;
    int cola_fin = 0;
    int maxima = 0;

    for (int i = 0; i < MAX_CELDAS_CABLES; i++)
    {
        distancia[i] = -1;
    }

    distancia[tablero.origen] = 0;
    cola[cola_fin++] = tablero.origen;

    while (cabeza < cola_fin)
    {
        int celda = cola[cabeza++];
        unsigned char mascara = MascaraActualCables(tablero, celda);
        int columna = celda % tablero.columnas;
        int fila = celda / tablero.columnas;

        for (int d = 0; d < 4; d++)
        {
            if (!(mascara & BitCables(d)))
            {
                continue;
            }

            int nc = columna + DX_CABLES[d];
            int nf = fila + DZ_CABLES[d];

            if (nc < 0 || nc >= tablero.columnas || nf < 0 || nf >= tablero.filas)
            {
                continue;
            }

            int vecina = nf * tablero.columnas + nc;

            if (distancia[vecina] >= 0)
            {
                continue;
            }

            if (!(MascaraActualCables(tablero, vecina) & BitCables(OpuestoCables(d))))
            {
                continue;
            }

            distancia[vecina] = distancia[celda] + 1;

            if (distancia[vecina] > maxima)
            {
                maxima = distancia[vecina];
            }

            cola[cola_fin++] = vecina;
        }
    }

    if (profundidad != nullptr)
    {
        *profundidad = maxima;
    }

    (void)celdas;
    return cola_fin;
}


bool BombillaEncendidaCables(const TableroCables& tablero)
{
    int distancia[MAX_CELDAS_CABLES];
    CalcularEnergiaCables(tablero, distancia, nullptr);
    return distancia[tablero.bombilla] >= 0;
}


static unsigned char PiezaDecoradoCables(RngCables& rng)
{
    static const unsigned char TIPOS[4] = { 5, 3, 7, 15 };   // recta, esquina, T, cruz
    int tirada = RangoRngCables(rng, 0, 99);

    if (tirada < 8)
    {
        return 0;
    }

    if (tirada < 36)
    {
        return TIPOS[0];
    }

    if (tirada < 80)
    {
        return TIPOS[1];
    }

    if (tirada < 96)
    {
        return TIPOS[2];
    }

    return TIPOS[3];
}


void GenerarTableroCables(
    unsigned int semilla,
    int columnas,
    int filas,
    int largoMin,
    int largoMax,
    int clicsMin,
    TableroCables& tablero
)
{
    columnas = columnas < 4 ? 4 : (columnas > MAX_COLUMNAS_CABLES ? MAX_COLUMNAS_CABLES : columnas);
    filas = filas < 3 ? 3 : (filas > MAX_FILAS_CABLES ? MAX_FILAS_CABLES : filas);

    RngCables rng = CrearRngCables(semilla);

    for (int intento = 0; intento < 600; intento++)
    {
        bool relajado = intento >= 400;
        int minimo = relajado ? 2 : largoMin;
        int maximo = relajado ? columnas * filas - 2 : largoMax;

        tablero = TableroCables{};
        tablero.columnas = columnas;
        tablero.filas = filas;

        int filaOrigen = RangoRngCables(rng, 0, filas - 1);
        int filaBombilla = RangoRngCables(rng, 0, filas - 1);
        tablero.origen = filaOrigen * columnas;
        tablero.bombilla = filaBombilla * columnas + columnas - 1;

        BusquedaCaminoCables busqueda;
        busqueda.rng = &rng;
        busqueda.columnas = columnas;
        busqueda.filas = filas;
        busqueda.origen = tablero.origen;
        busqueda.bombilla = tablero.bombilla;
        busqueda.objetivo = tablero.bombilla - 1;
        busqueda.largoMin = minimo;
        busqueda.largoMax = maximo;
        busqueda.camino[0] = tablero.origen + 1;
        busqueda.largo = 1;
        busqueda.visitado[tablero.origen + 1] = true;

        if (!ExplorarCaminoCables(busqueda, tablero.origen + 1))
        {
            continue;
        }

        bool delCamino[MAX_CELDAS_CABLES]{};

        for (int i = 0; i < busqueda.largo; i++)
        {
            int celda = busqueda.camino[i];
            int entrada = i == 0
                ? 3
                : OpuestoCables(DireccionEntreCables(columnas, busqueda.camino[i - 1], celda));
            int salida = i == busqueda.largo - 1
                ? 1
                : DireccionEntreCables(columnas, celda, busqueda.camino[i + 1]);

            unsigned char mascara = (unsigned char)(BitCables(entrada) | BitCables(salida));
            int tirada = RangoRngCables(rng, 0, 99);

            if (tirada < 24)
            {
                // Una T: el tercer lado queda como rama suelta (senuelo).
                int libres[4];
                int cantidad = 0;

                for (int d = 0; d < 4; d++)
                {
                    if (!(mascara & BitCables(d)))
                    {
                        libres[cantidad++] = d;
                    }
                }

                if (cantidad > 0)
                {
                    mascara |= BitCables(libres[RangoRngCables(rng, 0, cantidad - 1)]);
                }
            }
            else if (tirada < 29)
            {
                mascara = 15;
            }

            tablero.base[celda] = mascara;
            tablero.camino[i] = celda;
            delCamino[celda] = true;
        }

        tablero.largoCamino = busqueda.largo;
        tablero.base[tablero.origen] = CABLE_ESTE;
        tablero.base[tablero.bombilla] = CABLE_OESTE;
        tablero.fija[tablero.origen] = true;
        tablero.fija[tablero.bombilla] = true;
        delCamino[tablero.origen] = true;
        delCamino[tablero.bombilla] = true;

        for (int celda = 0; celda < columnas * filas; celda++)
        {
            if (!delCamino[celda])
            {
                tablero.base[celda] = PiezaDecoradoCables(rng);
            }

            if (!tablero.fija[celda])
            {
                tablero.giro[celda] = (unsigned char)RangoRngCables(rng, 0, 3);
            }
        }

        if (BombillaEncendidaCables(tablero))
        {
            continue;
        }

        SolucionCables mejor[1];

        if (ResolverTableroCables(tablero, mejor, 1) == 0)
        {
            continue;
        }

        if (!relajado && mejor[0].clics < clicsMin)
        {
            continue;
        }

        return;
    }
}


//==================================================
// AYUDAS DE PARTIDA
//==================================================


static bool EsControlBotCables(const Participante& participante)
{
    return participante.esBot || !participante.conectado;
}


static float Azar01Cables()
{
    return (float)GetRandomValue(0, 10000) / 10000.0f;
}


static float AzarRangoCables(float minimo, float maximo)
{
    return minimo + (maximo - minimo) * Azar01Cables();
}


static float CentroPanelXCables(int lado)
{
    return lado == 0 ? -CENTRO_PANEL_X_CABLES : CENTRO_PANEL_X_CABLES;
}


static Vector3 CentroCeldaCables(int lado, const TableroCables& tablero, int celda)
{
    int columna = celda % tablero.columnas;
    int fila = celda / tablero.columnas;

    return Vector3{
        CentroPanelXCables(lado) + ((float)columna - (float)(tablero.columnas - 1) * 0.5f),
        0.07f,
        ((float)fila - (float)(tablero.filas - 1) * 0.5f)
    };
}


static unsigned int MezclarCables(unsigned int a, unsigned int b)
{
    unsigned int h = a * 747796405u + b * 2891336453u + 0x9E3779B9u;
    h ^= h >> 15;
    h *= 2246822519u;
    h ^= h >> 13;
    return h;
}


static void AgregarParticulaCables(
    MinijuegoCablesCruzados& m,
    Vector3 posicion,
    Vector3 velocidad,
    float vida,
    float tamano,
    Color color
)
{
    for (int i = 0; i < MAX_PARTICULAS_CABLES; i++)
    {
        ParticulaCables& p = m.particulas[i];

        if (p.activa)
        {
            continue;
        }

        p.activa = true;
        p.posicion = posicion;
        p.velocidad = velocidad;
        p.vida = vida;
        p.vidaMaxima = vida;
        p.tamano = tamano;
        p.color = color;
        return;
    }
}


static void EmitirChispasCables(
    MinijuegoCablesCruzados& m,
    Vector3 centro,
    int cantidad,
    float velocidad,
    Color color
)
{
    int total = (int)std::ceil(
        (float)cantidad * FactorCalidadGrafica(ObtenerOpcionesCalidadGrafica().particulas)
    );

    for (int i = 0; i < total; i++)
    {
        float angulo = AzarRangoCables(0.0f, 6.2832f);
        float fuerza = AzarRangoCables(0.35f, 1.0f) * velocidad;

        AgregarParticulaCables(
            m,
            centro,
            Vector3{
                std::cos(angulo) * fuerza,
                AzarRangoCables(1.2f, 3.2f) * velocidad * 0.55f,
                std::sin(angulo) * fuerza
            },
            AzarRangoCables(0.35f, 0.75f),
            AzarRangoCables(0.045f, 0.095f),
            color
        );
    }
}


static void ActualizarParticulasCables(MinijuegoCablesCruzados& m, float dt)
{
    for (int i = 0; i < MAX_PARTICULAS_CABLES; i++)
    {
        ParticulaCables& p = m.particulas[i];

        if (!p.activa)
        {
            continue;
        }

        p.vida -= dt;
        p.velocidad.y -= 7.5f * dt;
        p.posicion.x += p.velocidad.x * dt;
        p.posicion.y += p.velocidad.y * dt;
        p.posicion.z += p.velocidad.z * dt;

        if (p.vida <= 0.0f || p.posicion.y < 0.05f)
        {
            p.activa = false;
        }
    }
}


static int GiroLogicoCables(const LadoCables& lado, int celda)
{
    return (int)lado.tablero.giro[celda];
}


// Recalcula la corriente del lado; avisa de piezas nuevas con corriente.
static void ReevaluarLadoCables(
    MinijuegoCablesCruzados& m,
    int indiceLado,
    bool conSonido
)
{
    LadoCables& lado = m.lados[indiceLado];
    int anterior[MAX_CELDAS_CABLES];

    for (int i = 0; i < MAX_CELDAS_CABLES; i++)
    {
        anterior[i] = lado.distancia[i];
    }

    int profundidad = 0;
    lado.energizadas = CalcularEnergiaCables(lado.tablero, lado.distancia, &profundidad);

    int nuevas = 0;
    int celdas = lado.tablero.columnas * lado.tablero.filas;

    for (int i = 0; i < celdas; i++)
    {
        if (anterior[i] < 0 && lado.distancia[i] >= 0)
        {
            nuevas++;
            EmitirChispasCables(
                m,
                CentroCeldaCables(indiceLado, lado.tablero, i),
                3,
                1.5f,
                Color{ 255, 226, 110, 255 }
            );
        }
    }

    if (profundidad != lado.profundidad)
    {
        lado.profundidad = profundidad;
        lado.tiempoUltimoAvance = m.tiempoRonda;
    }

    lado.encendida = lado.distancia[lado.tablero.bombilla] >= 0;

    if (conSonido && nuevas > 0)
    {
        ReproducirSonidoMinijuego(m.audio, SONIDO_RECOGER_OBJETO);
    }
}


static bool MoverCursorCables(LadoCables& lado, int direccion)
{
    const TableroCables& t = lado.tablero;
    int columna = lado.cursor % t.columnas + DX_CABLES[direccion];
    int fila = lado.cursor / t.columnas + DZ_CABLES[direccion];

    if (columna < 0 || columna >= t.columnas || fila < 0 || fila >= t.filas)
    {
        return false;
    }

    lado.cursor = fila * t.columnas + columna;
    return true;
}


// Gira la pieza bajo el cursor (+1 horario, -1 antihorario).
static void GirarPiezaCables(
    MinijuegoCablesCruzados& m,
    int indiceLado,
    int sentido,
    bool conSonido
)
{
    LadoCables& lado = m.lados[indiceLado];
    int celda = lado.cursor;
    unsigned char base = lado.tablero.base[celda];

    if (lado.tablero.fija[celda] || base == 0 || base == 15)
    {
        lado.sacudida = 0.22f;
        return;
    }

    lado.giro[celda] += sentido;
    lado.tablero.giro[celda] = (unsigned char)ModuloCables(lado.giro[celda], 4);
    lado.destello[celda] = 0.3f;
    lado.clics++;

    if (conSonido)
    {
        ReproducirSonidoMinijuego(m.audio, SONIDO_BOTON);
    }

    Vector3 centro = CentroCeldaCables(indiceLado, lado.tablero, celda);
    EmitirChispasCables(m, centro, 2, 0.9f, Color{ 190, 196, 210, 255 });

    ReevaluarLadoCables(m, indiceLado, conSonido);
}


//==================================================
// PARTIDA: RONDAS Y RESULTADO
//==================================================


static void PrepararRondaCables(MinijuegoCablesCruzados& m)
{
    int r = m.ronda < MAX_RONDAS_CABLES ? m.ronda : MAX_RONDAS_CABLES - 1;

    TableroCables tablero;
    GenerarTableroCables(
        MezclarCables(m.semilla, (unsigned int)(r + 1)),
        COLUMNAS_RONDA_CABLES[r],
        FILAS_RONDA_CABLES[r],
        LARGO_MIN_RONDA_CABLES[r],
        LARGO_MAX_RONDA_CABLES[r],
        CLICS_MIN_RONDA_CABLES[r],
        tablero
    );

    m.duracionRonda = DURACION_RONDA_CABLES[r];
    m.tiempoRestante = m.duracionRonda;
    m.tiempoRonda = 0.0f;
    m.tiempoAparicion = 0.0f;
    m.ladoGanadorRonda = -1;
    m.rondaPorTiempo = false;
    m.monedaDesempate = (int)((MezclarCables(m.semilla, 77u + (unsigned int)r) >> 9) & 1u);

    for (int s = 0; s < 2; s++)
    {
        LadoCables& lado = m.lados[s];
        lado.tablero = tablero;
        lado.encendida = false;
        lado.cursor = tablero.origen + 1;
        lado.dirPrevia = -1;
        lado.repeticion = 0.0f;
        lado.onda = -1.0f;
        lado.brilloBombilla = 0.0f;
        lado.sacudida = 0.0f;
        lado.clics = 0;
        lado.profundidad = 0;
        lado.tiempoUltimoAvance = 0.0f;

        for (int i = 0; i < MAX_CELDAS_CABLES; i++)
        {
            lado.giro[i] = tablero.giro[i];
            lado.giroVisual[i] = (float)tablero.giro[i];
            lado.giroVelocidad[i] = 0.0f;
            lado.destello[i] = 0.0f;
        }

        lado.energizadas = CalcularEnergiaCables(lado.tablero, lado.distancia, &lado.profundidad);

        BotCables& bot = lado.bot;
        bot.espera = 0.0f;
        bot.planificado = false;
        bot.rondaPlan = -1;
        bot.paso = 0;
        bot.celdaReaccion = -1;
        bot.reaccionado = false;
    }
}


static int GanadorPorTiempoCables(const MinijuegoCablesCruzados& m)
{
    const LadoCables& a = m.lados[0];
    const LadoCables& b = m.lados[1];

    if (a.profundidad != b.profundidad)
    {
        return a.profundidad > b.profundidad ? 0 : 1;
    }

    if (a.energizadas != b.energizadas)
    {
        return a.energizadas > b.energizadas ? 0 : 1;
    }

    if (std::fabs(a.tiempoUltimoAvance - b.tiempoUltimoAvance) > 0.0001f)
    {
        return a.tiempoUltimoAvance < b.tiempoUltimoAvance ? 0 : 1;
    }

    return m.monedaDesempate;
}


static void FinalizarRondaCables(MinijuegoCablesCruzados& m, int ganador, bool porTiempo)
{
    m.ladoGanadorRonda = ganador;
    m.rondaPorTiempo = porTiempo;
    m.lados[ganador].rondasGanadas++;
    m.fase = FASE_CABLES_FIN_RONDA;

    bool partidaDecidida = m.lados[ganador].rondasGanadas >= RONDAS_PARA_GANAR_CABLES;
    m.tiempoFase = partidaDecidida ? DURACION_FIN_PARTIDA_CABLES : DURACION_FIN_RONDA_CABLES;

    if (!porTiempo)
    {
        LadoCables& lado = m.lados[ganador];
        lado.onda = 0.0f;
        ReproducirSonidoMinijuego(m.audio, SONIDO_ACIERTO);

        Vector3 bombilla = CentroCeldaCables(ganador, lado.tablero, lado.tablero.bombilla);
        bombilla.y = 0.45f;
        EmitirChispasCables(m, bombilla, 26, 3.4f, Color{ 255, 236, 140, 255 });
        EmitirChispasCables(m, bombilla, 14, 2.6f, Color{ 255, 255, 255, 255 });
    }
    else
    {
        ReproducirSonidoMinijuego(m.audio, SONIDO_ALERTA_TIEMPO);
    }
}


static void FinalizarPartidaCables(MinijuegoCablesCruzados& m)
{
    if (m.resultado.estado == RESULTADO_MINIJUEGO_FINALIZADO)
    {
        return;
    }

    int ganador = m.lados[0].rondasGanadas > m.lados[1].rondasGanadas ? 0 : 1;
    m.ganadorPartida = ganador;

    for (int s = 0; s < 2; s++)
    {
        int indice = m.lados[s].participante;

        if (indice < 0)
        {
            continue;
        }

        ResultadoParticipante& r = m.resultado.participantes[indice];
        r.posicionFinal = s == ganador ? 1 : 2;
        r.puntuacionMinijuego = m.lados[s].rondasGanadas * 100 + m.lados[s].energizadas;
    }

    m.resultado.estado = RESULTADO_MINIJUEGO_FINALIZADO;
    m.resultado.desenlace = DESENLACE_CON_GANADOR;
    m.fase = FASE_CABLES_TERMINADO;
    ReproducirSonidoMinijuego(m.audio, SONIDO_RESULTADO);
}


//==================================================
// BOT
//==================================================


static void PlanificarBotCables(const MinijuegoCablesCruzados& m, LadoCables& lado)
{
    BotCables& bot = lado.bot;
    SolucionCables soluciones[MAX_SOLUCIONES_CABLES];
    int cantidad = ResolverTableroCables(lado.tablero, soluciones, MAX_SOLUCIONES_CABLES);

    bot.planificado = true;
    bot.rondaPlan = m.ronda;
    bot.paso = 0;
    bot.celdaReaccion = -1;
    bot.reaccionado = false;

    if (cantidad <= 0)
    {
        bot.plan = SolucionCables{};
        return;
    }

    // Una persona no siempre ve el camino mas corto.
    int elegida = 0;

    if (cantidad > 1 && Azar01Cables() < 0.08f + 0.30f * bot.torpeza)
    {
        elegida = (cantidad > 2 && Azar01Cables() < 0.35f) ? 2 : 1;
    }

    bot.plan = soluciones[elegida];
}


static void ActualizarBotCables(
    MinijuegoCablesCruzados& m,
    int indiceLado,
    float dt
)
{
    LadoCables& lado = m.lados[indiceLado];
    BotCables& bot = lado.bot;

    if (lado.encendida)
    {
        return;
    }

    bot.espera -= dt;

    if (bot.espera > 0.0f)
    {
        return;
    }

    if (!bot.planificado || bot.rondaPlan != m.ronda)
    {
        bool primeraVez = bot.rondaPlan != m.ronda;
        PlanificarBotCables(m, lado);
        // Primero "mira" el panel; si retoma a mitad de ronda es mas rapido.
        bot.espera = primeraVez && m.tiempoRonda < 1.0f
            ? bot.planificacion * AzarRangoCables(0.8f, 1.25f)
            : 0.3f;
        return;
    }

    while (
        bot.paso < bot.plan.cantidad &&
        GiroLogicoCables(lado, bot.plan.celdas[bot.paso]) == (int)bot.plan.giro[bot.paso]
    )
    {
        bot.paso++;
    }

    if (bot.paso >= bot.plan.cantidad)
    {
        // El plan ya esta cumplido pero falta luz (no deberia pasar): replantea.
        bot.planificado = false;
        bot.rondaPlan = m.ronda;
        bot.espera = 0.1f;
        return;
    }

    int celda = bot.plan.celdas[bot.paso];

    if (lado.cursor != celda)
    {
        int dx = celda % lado.tablero.columnas - lado.cursor % lado.tablero.columnas;
        int dz = celda / lado.tablero.columnas - lado.cursor / lado.tablero.columnas;
        bool horizontal;

        if (dx == 0)
        {
            horizontal = false;
        }
        else if (dz == 0)
        {
            horizontal = true;
        }
        else
        {
            horizontal = Azar01Cables() < (float)std::abs(dx) / (float)(std::abs(dx) + std::abs(dz));
        }

        int direccion = horizontal ? (dx > 0 ? 1 : 3) : (dz > 0 ? 2 : 0);
        MoverCursorCables(lado, direccion);
        bot.espera = bot.ritmo * AzarRangoCables(0.55f, 0.95f);
        return;
    }

    if (bot.celdaReaccion != celda)
    {
        bot.celdaReaccion = celda;
        bot.reaccionado = false;
    }

    if (!bot.reaccionado)
    {
        bot.reaccionado = true;
        bot.espera = AzarRangoCables(0.08f, 0.2f) * (0.7f + bot.torpeza);

        if (Azar01Cables() < bot.probDuda)
        {
            bot.espera += AzarRangoCables(0.3f, 0.8f);
        }

        return;
    }

    int falta = ModuloCables((int)bot.plan.giro[bot.paso] - GiroLogicoCables(lado, celda), 4);
    bool horario = falta <= 2;

    if (Azar01Cables() < bot.probError)
    {
        horario = !horario;
    }

    GirarPiezaCables(m, indiceLado, horario ? 1 : -1, false);
    bot.espera = bot.ritmo * AzarRangoCables(0.85f, 1.35f);
}


//==================================================
// ENTRADA HUMANA
//==================================================


static void ActualizarHumanoCables(
    MinijuegoCablesCruzados& m,
    int indiceLado,
    float dt,
    const Participante& participante
)
{
    LadoCables& lado = m.lados[indiceLado];
    InputMinijuegoParticipante entrada = LeerInputMinijuegoParticipante(participante);

    bool presionadas[4] = { entrada.adelante, entrada.derecha, entrada.atras, entrada.izquierda };
    int direccion = -1;

    if (lado.dirPrevia >= 0 && presionadas[lado.dirPrevia])
    {
        direccion = lado.dirPrevia;
    }
    else
    {
        for (int d = 0; d < 4; d++)
        {
            if (presionadas[d])
            {
                direccion = d;
                break;
            }
        }
    }

    if (direccion < 0)
    {
        lado.dirPrevia = -1;
        lado.repeticion = 0.0f;
    }
    else
    {
        if (direccion != lado.dirPrevia)
        {
            MoverCursorCables(lado, direccion);
            lado.repeticion = 0.26f;
        }
        else
        {
            lado.repeticion -= dt;

            if (lado.repeticion <= 0.0f)
            {
                MoverCursorCables(lado, direccion);
                lado.repeticion = 0.11f;
            }
        }

        lado.dirPrevia = direccion;
    }

    if (entrada.golpear)
    {
        GirarPiezaCables(m, indiceLado, 1, true);
    }

    if (entrada.saltar)
    {
        GirarPiezaCables(m, indiceLado, -1, true);
    }
}


//==================================================
// ESTADO
//==================================================


void MinijuegoCablesCruzados::Inicializar()
{
    resultado = {};
    resultado.formato = FORMATO_MINIJUEGO_INDIVIDUAL;

    for (int s = 0; s < 2; s++)
    {
        lados[s] = LadoCables{};
    }

    for (int i = 0; i < MAX_PARTICULAS_CABLES; i++)
    {
        particulas[i] = ParticulaCables{};
    }

    semilla = (unsigned int)GetRandomValue(1, 2000000000);
    ronda = 0;
    ladoGanadorRonda = -1;
    rondaPorTiempo = false;
    ganadorPartida = -1;
    partidaValida = false;

    fase = FASE_CABLES_PREPARACION;
    tiempoPreparacion = DURACION_PREPARACION_CABLES;
    tiempoFase = 0.0f;
    tiempoAnimacion = 0.0f;

    camara.position = { 0.0f, 10.1f, 8.4f };
    camara.target = { 0.0f, 0.0f, -0.15f };
    camara.up = { 0.0f, 1.0f, 0.0f };
    camara.fovy = 42.0f;
    camara.projection = CAMERA_PERSPECTIVE;

    PrepararRondaCables(*this);
}


void MinijuegoCablesCruzados::Reiniciar(
    JugadorPrueba jugadores[],
    Participante participantes[],
    int cantidadMaxima
)
{
    (void)jugadores;

    Inicializar();
    InicializarResultadoMinijuego(resultado, participantes, FORMATO_MINIJUEGO_INDIVIDUAL);

    int indices[2] = { -1, -1 };
    int encontrados = 0;
    int limite = cantidadMaxima < MAX_PARTICIPANTES ? cantidadMaxima : MAX_PARTICIPANTES;

    for (int i = 0; i < limite; i++)
    {
        if (!participantes[i].activo)
        {
            continue;
        }

        if (encontrados < 2)
        {
            indices[encontrados++] = i;
        }
        else
        {
            // Es un duelo: solo juegan los dos primeros.
            resultado.participantes[i].participo = false;
            resultado.cantidadParticipantes--;
        }
    }

    if (encontrados < 2)
    {
        resultado.estado = RESULTADO_MINIJUEGO_CANCELADO;
        fase = FASE_CABLES_TERMINADO;
        return;
    }

    for (int s = 0; s < 2; s++)
    {
        BotCables& bot = lados[s].bot;
        float torpeza = AzarRangoCables(0.0f, 1.0f);

        lados[s].participante = indices[s];
        bot.torpeza = torpeza;
        bot.ritmo = 0.36f + 0.56f * torpeza;
        bot.planificacion = 1.6f + 3.4f * torpeza;
        bot.probError = 0.04f + 0.10f * torpeza;
        bot.probDuda = 0.06f + 0.16f * torpeza;
    }

    partidaValida = true;
}


void MinijuegoCablesCruzados::Actualizar(
    float deltaTime,
    JugadorPrueba jugadores[],
    int cantidadMaxima,
    Participante participantes[]
)
{
    (void)jugadores;
    (void)cantidadMaxima;

    tiempoAnimacion += deltaTime;
    tiempoAparicion += deltaTime;

    ActualizarParticulasCables(*this, deltaTime);

    // Animacion de giro (resorte con un poco de rebote) y brillos.
    for (int s = 0; s < 2; s++)
    {
        LadoCables& lado = lados[s];
        int celdas = lado.tablero.columnas * lado.tablero.filas;

        for (int i = 0; i < celdas; i++)
        {
            float objetivo = (float)lado.giro[i];
            lado.giroVelocidad[i] += (objetivo - lado.giroVisual[i]) * 520.0f * deltaTime;
            lado.giroVelocidad[i] *= std::exp(-17.0f * deltaTime);
            lado.giroVisual[i] += lado.giroVelocidad[i] * deltaTime;

            if (lado.destello[i] > 0.0f)
            {
                lado.destello[i] -= deltaTime;
            }
        }

        if (lado.sacudida > 0.0f)
        {
            lado.sacudida -= deltaTime;
        }

        float objetivoBrillo = lado.encendida ? 1.0f : 0.0f;
        lado.brilloBombilla += (objetivoBrillo - lado.brilloBombilla) * std::fmin(1.0f, deltaTime * 9.0f);

        if (lado.onda >= 0.0f && lado.onda < (float)lado.profundidad + 6.0f)
        {
            lado.onda += deltaTime * 15.0f;
        }
    }

    if (!partidaValida || resultado.estado == RESULTADO_MINIJUEGO_CANCELADO)
    {
        return;
    }

    switch (fase)
    {
    case FASE_CABLES_PREPARACION:
    {
        float antes = tiempoPreparacion;
        tiempoPreparacion -= deltaTime;
        ActualizarAudioCuentaRegresiva(audio, antes, tiempoPreparacion);

        if (tiempoPreparacion <= 0.0f)
        {
            tiempoPreparacion = 0.0f;
            fase = FASE_CABLES_JUGANDO;
            tiempoAparicion = 0.0f;
        }

        break;
    }

    case FASE_CABLES_INTRO_RONDA:
        tiempoFase -= deltaTime;

        if (tiempoFase <= 0.0f)
        {
            fase = FASE_CABLES_JUGANDO;
        }

        break;

    case FASE_CABLES_JUGANDO:
    {
        float antes = tiempoRestante;
        tiempoRonda += deltaTime;
        tiempoRestante -= deltaTime;
        ActualizarAudioAlertaTiempo(audio, antes, tiempoRestante);

        // El orden de los lados se sortea por ronda: sin ventaja de posicion.
        int primero = monedaDesempate;

        for (int k = 0; k < 2; k++)
        {
            int s = k == 0 ? primero : 1 - primero;
            int indice = lados[s].participante;

            if (EsControlBotCables(participantes[indice]))
            {
                ActualizarBotCables(*this, s, deltaTime);
            }
            else
            {
                ActualizarHumanoCables(*this, s, deltaTime, participantes[indice]);
            }

            if (lados[s].encendida)
            {
                FinalizarRondaCables(*this, s, false);
                return;
            }
        }

        if (tiempoRestante <= 0.0f)
        {
            tiempoRestante = 0.0f;
            FinalizarRondaCables(*this, GanadorPorTiempoCables(*this), true);
        }

        break;
    }

    case FASE_CABLES_FIN_RONDA:
        tiempoFase -= deltaTime;

        // Chispas sobre la bombilla ganadora mientras dura el festejo.
        if (!rondaPorTiempo && ladoGanadorRonda >= 0 && GetRandomValue(0, 100) < 22)
        {
            LadoCables& g = lados[ladoGanadorRonda];
            Vector3 b = CentroCeldaCables(ladoGanadorRonda, g.tablero, g.tablero.bombilla);
            b.y = 0.5f;
            EmitirChispasCables(*this, b, 2, 2.2f, Color{ 255, 232, 120, 255 });
        }

        if (tiempoFase <= 0.0f)
        {
            if (lados[ladoGanadorRonda].rondasGanadas >= RONDAS_PARA_GANAR_CABLES)
            {
                FinalizarPartidaCables(*this);
            }
            else
            {
                ronda++;
                PrepararRondaCables(*this);
                fase = FASE_CABLES_INTRO_RONDA;
                tiempoFase = DURACION_INTRO_RONDA_CABLES;
            }
        }

        break;

    case FASE_CABLES_TERMINADO:
        break;
    }
}


const ResultadoMinijuego& MinijuegoCablesCruzados::ObtenerResultado() const
{
    return resultado;
}



//==================================================
// VISUAL
//==================================================
//
// Banco de taller en 3D voxel: dos paneles con marco del color de cada
// jugador, piezas de cable que giran sobre una placa, una pila a la izquierda
// y una bombilla a la derecha. Los cubos se dibujan con caras sombreadas
// (arriba claro, laterales mas oscuros) para dar volumen sin luces reales.
//==================================================


static Color SombrearCables(Color c, float f)
{
    return Color{
        (unsigned char)std::fmin(255.0f, (float)c.r * f),
        (unsigned char)std::fmin(255.0f, (float)c.g * f),
        (unsigned char)std::fmin(255.0f, (float)c.b * f),
        c.a
    };
}


static Color MezclarColorCables(Color a, Color b, float t)
{
    t = t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t);

    return Color{
        (unsigned char)((float)a.r + ((float)b.r - (float)a.r) * t),
        (unsigned char)((float)a.g + ((float)b.g - (float)a.g) * t),
        (unsigned char)((float)a.b + ((float)b.b - (float)a.b) * t),
        255
    };
}


// Cubo con caras sombreadas; respeta la matriz actual (para piezas que giran).
static void CuboCables(Vector3 c, Vector3 t, Color color)
{
    float x0 = c.x - t.x * 0.5f, x1 = c.x + t.x * 0.5f;
    float y0 = c.y - t.y * 0.5f, y1 = c.y + t.y * 0.5f;
    float z0 = c.z - t.z * 0.5f, z1 = c.z + t.z * 0.5f;

    Color arriba = SombrearCables(color, 1.0f);
    Color frente = SombrearCables(color, 0.78f);
    Color derecha = SombrearCables(color, 0.62f);
    Color izquierda = SombrearCables(color, 0.70f);

    rlBegin(RL_TRIANGLES);

    // Arriba (+Y)
    rlColor4ub(arriba.r, arriba.g, arriba.b, arriba.a);
    rlVertex3f(x0, y1, z0); rlVertex3f(x0, y1, z1); rlVertex3f(x1, y1, z1);
    rlVertex3f(x1, y1, z0); rlVertex3f(x0, y1, z0); rlVertex3f(x1, y1, z1);

    // Frente (+Z)
    rlColor4ub(frente.r, frente.g, frente.b, frente.a);
    rlVertex3f(x0, y0, z1); rlVertex3f(x1, y0, z1); rlVertex3f(x0, y1, z1);
    rlVertex3f(x1, y1, z1); rlVertex3f(x0, y1, z1); rlVertex3f(x1, y0, z1);

    // Derecha (+X)
    rlColor4ub(derecha.r, derecha.g, derecha.b, derecha.a);
    rlVertex3f(x1, y0, z0); rlVertex3f(x1, y1, z0); rlVertex3f(x1, y1, z1);
    rlVertex3f(x1, y0, z1); rlVertex3f(x1, y0, z0); rlVertex3f(x1, y1, z1);

    // Izquierda (-X)
    rlColor4ub(izquierda.r, izquierda.g, izquierda.b, izquierda.a);
    rlVertex3f(x0, y0, z0); rlVertex3f(x0, y1, z1); rlVertex3f(x0, y1, z0);
    rlVertex3f(x0, y0, z1); rlVertex3f(x0, y1, z1); rlVertex3f(x0, y0, z0);

    rlEnd();
}


static Color ColorCableApagado() { return Color{ 176, 128, 84, 255 }; }
static Color ColorCableEncendido() { return Color{ 255, 226, 96, 255 }; }


static float FraccionCables(float v)
{
    return v - std::floor(v);
}


// Las piezas aparecen en ola desde la esquina con un pequeno rebote.
static float EscalaAparicionCables(float tiempo, int columna, int fila)
{
    float t = (tiempo - 0.045f * (float)(columna + fila)) / 0.32f;

    if (t <= 0.0f)
    {
        return 0.0f;
    }

    if (t >= 1.0f)
    {
        return 1.0f;
    }

    float u = t - 1.0f;
    return 1.0f + 2.70158f * u * u * u + 1.70158f * u * u;
}


// Bombilla voxel (MODELO FUTURO: reemplazar por un GLB de bombilla).
static void DibujarBombillaCables(Vector3 p, float brillo, float tiempo)
{
    Color casquillo = Color{ 150, 154, 168, 255 };
    CuboCables({ p.x, p.y + 0.06f, p.z }, { 0.40f, 0.12f, 0.40f }, Color{ 70, 62, 58, 255 });
    CuboCables({ p.x, p.y + 0.16f, p.z }, { 0.30f, 0.08f, 0.30f }, casquillo);
    CuboCables({ p.x, p.y + 0.23f, p.z }, { 0.26f, 0.06f, 0.26f }, SombrearCables(casquillo, 0.8f));

    Color apagado = Color{ 188, 200, 214, 255 };
    Color encendido = Color{ 255, 240, 150, 255 };
    Color vidrio = MezclarColorCables(apagado, encendido, brillo);
    float pulso = 1.0f + 0.04f * std::sin(tiempo * 14.0f) * brillo;

    CuboCables({ p.x, p.y + 0.46f, p.z }, { 0.40f * pulso, 0.30f, 0.40f * pulso }, vidrio);
    CuboCables({ p.x, p.y + 0.46f, p.z }, { 0.30f, 0.42f, 0.30f }, vidrio);
    CuboCables({ p.x, p.y + 0.64f, p.z }, { 0.28f, 0.10f, 0.28f }, SombrearCables(vidrio, 1.04f));

    // Filamento.
    Color filamento = MezclarColorCables(Color{ 120, 110, 100, 255 }, Color{ 255, 255, 235, 255 }, brillo);
    CuboCables({ p.x, p.y + 0.42f, p.z }, { 0.05f, 0.22f, 0.05f }, filamento);
    CuboCables({ p.x, p.y + 0.52f, p.z }, { 0.16f, 0.04f, 0.04f }, filamento);
}


// Pila voxel (MODELO FUTURO: reemplazar por un GLB de bateria).
static void DibujarPilaCables(Vector3 p, float tiempo)
{
    CuboCables({ p.x, p.y + 0.22f, p.z }, { 0.56f, 0.44f, 0.56f }, Color{ 214, 76, 62, 255 });
    CuboCables({ p.x, p.y + 0.38f, p.z }, { 0.58f, 0.10f, 0.58f }, Color{ 60, 66, 80, 255 });
    CuboCables({ p.x, p.y + 0.50f, p.z }, { 0.20f, 0.10f, 0.20f }, Color{ 200, 204, 214, 255 });

    // Rayo en el frente.
    Color rayo = MezclarColorCables(Color{ 255, 214, 70, 255 }, Color{ 255, 255, 200, 255 },
        0.5f + 0.5f * std::sin(tiempo * 7.0f));
    CuboCables({ p.x + 0.04f, p.y + 0.28f, p.z + 0.285f }, { 0.10f, 0.12f, 0.02f }, rayo);
    CuboCables({ p.x - 0.03f, p.y + 0.18f, p.z + 0.285f }, { 0.10f, 0.12f, 0.02f }, rayo);
    CuboCables({ p.x + 0.01f, p.y + 0.09f, p.z + 0.285f }, { 0.06f, 0.08f, 0.02f }, rayo);
}


static void DibujarCursorCables(Vector3 c, Color color, float tiempo)
{
    float pulso = 0.5f + 0.5f * std::sin(tiempo * 7.0f);
    Color c1 = MezclarColorCables(color, WHITE, 0.25f + 0.35f * pulso);
    float r = 0.5f + 0.015f * pulso;
    float y = c.y + 0.16f;

    CuboCables({ c.x, y, c.z - r }, { 1.04f, 0.07f, 0.07f }, c1);
    CuboCables({ c.x, y, c.z + r }, { 1.04f, 0.07f, 0.07f }, c1);
    CuboCables({ c.x - r, y, c.z }, { 0.07f, 0.07f, 0.94f }, c1);
    CuboCables({ c.x + r, y, c.z }, { 0.07f, 0.07f, 0.94f }, c1);

    // Esquinas altas.
    for (int i = 0; i < 4; i++)
    {
        float sx = (i & 1) ? 1.0f : -1.0f;
        float sz = (i & 2) ? 1.0f : -1.0f;
        CuboCables({ c.x + sx * r, y + 0.05f, c.z + sz * r }, { 0.12f, 0.16f, 0.12f }, c1);
    }
}


static void DibujarPiezaCables(
    const MinijuegoCablesCruzados& m,
    int indiceLado,
    int celda
)
{
    const LadoCables& lado = m.lados[indiceLado];
    const TableroCables& t = lado.tablero;
    int columna = celda % t.columnas;
    int fila = celda / t.columnas;
    Vector3 centro = CentroCeldaCables(indiceLado, t, celda);

    float escala = EscalaAparicionCables(m.tiempoAparicion, columna, fila);

    if (escala <= 0.001f)
    {
        return;
    }

    bool par = ((columna + fila) & 1) == 0;
    Color placa = par ? Color{ 58, 76, 88, 255 } : Color{ 50, 66, 78, 255 };

    rlPushMatrix();
    rlTranslatef(centro.x, 0.0f, centro.z);
    rlScalef(escala, escala, escala);

    // Placa fija.
    CuboCables({ 0.0f, 0.05f, 0.0f }, { 0.94f, 0.04f, 0.94f }, placa);

    bool esOrigen = celda == t.origen;
    bool esBombilla = celda == t.bombilla;
    unsigned char base = t.base[celda];
    int dist = lado.distancia[celda];
    bool energizada = dist >= 0;
    float flash = lado.destello[celda] > 0.0f ? lado.destello[celda] / 0.3f : 0.0f;
    float ondaExtra = 0.0f;

    if (energizada && lado.onda >= 0.0f)
    {
        float d = (float)dist - lado.onda;
        ondaExtra = std::exp(-d * d * 0.8f);
    }

    float ola = 0.5f + 0.5f * std::sin(m.tiempoAnimacion * 6.0f - (float)dist * 1.1f);
    Color cable = energizada
        ? MezclarColorCables(ColorCableEncendido(), Color{ 255, 255, 235, 255 }, 0.15f + 0.35f * ola + ondaExtra)
        : ColorCableApagado();

    if (flash > 0.0f && !energizada)
    {
        cable = MezclarColorCables(cable, Color{ 255, 240, 220, 255 }, flash * 0.6f);
    }

    if (esOrigen)
    {
        rlPushMatrix();
        rlTranslatef(0.0f, 0.07f, 0.0f);
        DibujarPilaCables({ 0.0f, 0.0f, 0.0f }, m.tiempoAnimacion);
        // Cable de salida hacia el este.
        CuboCables({ 0.38f, 0.05f, 0.0f }, { 0.28f, 0.09f, 0.16f }, SombrearCables(cable, 0.9f));
        rlPopMatrix();
    }
    else if (esBombilla)
    {
        rlPushMatrix();
        rlTranslatef(0.0f, 0.07f, 0.0f);
        CuboCables({ -0.38f, 0.05f, 0.0f }, { 0.28f, 0.09f, 0.16f }, cable);
        DibujarBombillaCables({ 0.0f, 0.0f, 0.0f }, lado.brilloBombilla, m.tiempoAnimacion);
        rlPopMatrix();
    }
    else if (base == 0)
    {
        // Hueco: tornillo suelto.
        CuboCables({ 0.0f, 0.085f, 0.0f }, { 0.16f, 0.03f, 0.16f }, Color{ 36, 42, 50, 255 });
        CuboCables({ 0.0f, 0.10f, 0.0f }, { 0.10f, 0.02f, 0.03f }, Color{ 140, 146, 158, 255 });
    }
    else
    {
        float angulo = -lado.giroVisual[celda] * 90.0f;
        int giroLogico = GiroLogicoCables(lado, celda);

        rlPushMatrix();
        rlTranslatef(0.0f, 0.07f, 0.0f);
        rlRotatef(angulo, 0.0f, 1.0f, 0.0f);

        // Disco giratorio.
        Color disco = energizada ? Color{ 74, 62, 36, 255 } : Color{ 66, 70, 82, 255 };
        (DrawCylinder)({ 0.0f, 0.0f, 0.0f }, 0.43f, 0.43f, 0.035f, 8, disco);

        for (int k = 0; k < 4; k++)
        {
            if (!(base & BitCables(k)))
            {
                continue;
            }

            float dx = (float)DX_CABLES[k];
            float dz = (float)DZ_CABLES[k];
            float ax = dx != 0.0f ? 0.26f : 0.22f;
            float az = dz != 0.0f ? 0.26f : 0.22f;

            CuboCables({ dx * 0.25f, 0.065f, dz * 0.25f }, { ax + 0.02f, 0.07f, az + 0.02f },
                SombrearCables(cable, 0.72f));
            CuboCables({ dx * 0.25f, 0.105f, dz * 0.25f },
                { dx != 0.0f ? 0.50f : 0.10f, 0.02f, dz != 0.0f ? 0.50f : 0.10f }, cable);

            // Perlas de corriente que viajan por el cable.
            if (energizada)
            {
                int mundo = (k + giroLogico) & 3;
                int vc = columna + DX_CABLES[mundo];
                int vf = fila + DZ_CABLES[mundo];

                if (vc >= 0 && vc < t.columnas && vf >= 0 && vf < t.filas)
                {
                    int vecina = vf * t.columnas + vc;
                    int dv = lado.distancia[vecina];
                    float tau = m.tiempoAnimacion * 1.7f;
                    float u = FraccionCables(tau - (float)dist + 0.5f) - 0.5f;
                    float s = -999.0f;

                    if (dv == dist - 1 && u < 0.0f)
                    {
                        s = -u;
                    }
                    else if (dv == dist + 1 && u >= 0.0f)
                    {
                        s = u;
                    }

                    if (s >= 0.0f)
                    {
                        CuboCables({ dx * s, 0.15f, dz * s }, { 0.11f, 0.08f, 0.11f }, WHITE);
                    }
                }
            }
        }

        CuboCables({ 0.0f, 0.075f, 0.0f }, { 0.32f, 0.09f, 0.32f }, SombrearCables(cable, 0.85f));
        CuboCables({ 0.0f, 0.12f, 0.0f }, { 0.18f, 0.02f, 0.18f }, cable);

        rlPopMatrix();
    }

    rlPopMatrix();
}


static void DibujarPanelCables(
    const MinijuegoCablesCruzados& m,
    int indiceLado,
    Color colorJugador
)
{
    const LadoCables& lado = m.lados[indiceLado];
    const TableroCables& t = lado.tablero;
    float cx = CentroPanelXCables(indiceLado);
    float ancho = (float)t.columnas;
    float alto = (float)t.filas;
    float iw = ancho * 0.5f + 0.16f;
    float ih = alto * 0.5f + 0.16f;

    // Placa base (circuito impreso).
    CuboCables({ cx, -0.02f, 0.0f }, { iw * 2.0f, 0.10f, ih * 2.0f }, Color{ 26, 52, 58, 255 });

    // Marco del color del jugador; el riel delantero es mas ancho (focos de ronda).
    Color marco = colorJugador;
    float grosor = 0.26f;
    float frente = 0.54f;
    CuboCables({ cx, 0.05f, -(ih + grosor * 0.5f) }, { iw * 2.0f + grosor * 2.0f, 0.22f, grosor }, marco);
    CuboCables({ cx, 0.05f, ih + frente * 0.5f }, { iw * 2.0f + grosor * 2.0f, 0.22f, frente }, marco);
    CuboCables({ cx - iw - grosor * 0.5f, 0.05f, 0.0f }, { grosor, 0.22f, ih * 2.0f }, marco);
    CuboCables({ cx + iw + grosor * 0.5f, 0.05f, 0.0f }, { grosor, 0.22f, ih * 2.0f }, marco);

    // Tornillos.
    for (int i = 0; i < 4; i++)
    {
        float sx = (i & 1) ? 1.0f : -1.0f;
        float sz = (i & 2) ? 1.0f : -1.0f;
        float zc = sz < 0.0f ? -(ih + grosor * 0.5f) : ih + 0.5f * frente;
        CuboCables({ cx + sx * (iw + grosor * 0.5f), 0.18f, zc }, { 0.10f, 0.05f, 0.10f },
            SombrearCables(marco, 1.5f));
    }

    // Focos de ronda sobre el riel delantero.
    for (int i = 0; i < RONDAS_PARA_GANAR_CABLES; i++)
    {
        float x = cx + (i == 0 ? -0.34f : 0.34f);
        float z = ih + frente * 0.5f;
        bool ganado = i < lado.rondasGanadas;
        CuboCables({ x, 0.19f, z }, { 0.34f, 0.07f, 0.34f }, Color{ 40, 44, 54, 255 });
        Color foco = ganado ? Color{ 255, 236, 120, 255 } : Color{ 96, 104, 120, 255 };
        CuboCables({ x, 0.29f, z }, { 0.22f, 0.14f, 0.22f }, foco);
    }

    int celdas = t.columnas * t.filas;

    for (int i = 0; i < celdas; i++)
    {
        DibujarPiezaCables(m, indiceLado, i);
    }
}


static void DibujarEscenarioCables(float tiempo)
{
    const OpcionesCalidadGrafica& calidad = ObtenerOpcionesCalidadGrafica();

    // Tapete de laboratorio cuadriculado sobre la mesa.
    CuboCables({ 0.0f, -0.20f, 0.8f }, { 30.0f, 0.4f, 16.0f }, Color{ 46, 60, 76, 255 });

    for (int i = -15; i <= 15; i++)
    {
        CuboCables({ (float)i, 0.004f, 0.8f }, { 0.04f, 0.008f, 16.0f }, Color{ 58, 76, 94, 255 });
    }

    for (int i = -7; i <= 7; i++)
    {
        CuboCables({ 0.0f, 0.006f, (float)i + 0.8f }, { 30.0f, 0.008f, 0.04f }, Color{ 58, 76, 94, 255 });
    }

    // Pared del fondo con tablero perforado.
    CuboCables({ 0.0f, 4.0f, -7.4f }, { 30.0f, 9.0f, 0.4f }, Color{ 40, 74, 84, 255 });

    if (calidad.decoracion == CALIDAD_BAJA)
    {
        return;
    }

    int paso = calidad.decoracion == CALIDAD_ALTA ? 1 : 2;

    for (int fila = 0; fila < 7; fila += paso)
    {
        for (int col = -11; col <= 11; col += paso)
        {
            CuboCables({ (float)col * 0.8f, 1.2f + (float)fila * 0.9f, -7.16f }, { 0.12f, 0.12f, 0.06f },
                Color{ 22, 44, 52, 255 });
        }
    }

    // Herramientas colgadas (MODELO FUTURO: modelos de herramientas).
    // Llave inglesa.
    CuboCables({ -8.0f, 3.2f, -7.05f }, { 0.28f, 2.2f, 0.18f }, Color{ 190, 194, 204, 255 });
    CuboCables({ -8.0f, 4.4f, -7.05f }, { 0.80f, 0.42f, 0.18f }, Color{ 190, 194, 204, 255 });
    // Martillo.
    CuboCables({ -5.2f, 3.0f, -7.05f }, { 0.26f, 2.0f, 0.18f }, Color{ 176, 120, 66, 255 });
    CuboCables({ -5.2f, 4.2f, -7.05f }, { 1.10f, 0.45f, 0.20f }, Color{ 120, 126, 140, 255 });
    // Destornillador.
    CuboCables({ 5.6f, 3.0f, -7.05f }, { 0.14f, 1.4f, 0.14f }, Color{ 200, 204, 214, 255 });
    CuboCables({ 5.6f, 2.0f, -7.05f }, { 0.34f, 0.9f, 0.22f }, Color{ 230, 90, 70, 255 });
    // Alicate.
    CuboCables({ 8.2f, 3.4f, -7.05f }, { 0.52f, 1.5f, 0.16f }, Color{ 236, 190, 70, 255 });

    // Estante con frascos de colores.
    CuboCables({ 0.0f, 6.3f, -6.9f }, { 9.0f, 0.18f, 0.7f }, Color{ 150, 104, 64, 255 });

    static const Color FRASCOS[4] = {
        { 230, 96, 82, 255 }, { 96, 190, 230, 255 }, { 240, 200, 84, 255 }, { 120, 214, 132, 255 }
    };

    for (int i = 0; i < 9; i++)
    {
        CuboCables({ -4.0f + (float)i * 1.0f, 6.72f, -6.9f }, { 0.5f, 0.6f, 0.4f }, FRASCOS[i & 3]);
    }

    // Tuercas y destornillador sobre la mesa.
    // (MODELO FUTURO: modelos de tuercas, destornillador y carrete.)
    CuboCables({ -8.6f, 0.12f, 3.7f }, { 0.4f, 0.24f, 0.4f }, Color{ 170, 174, 186, 255 });
    CuboCables({ -8.0f, 0.12f, 3.9f }, { 0.34f, 0.24f, 0.34f }, Color{ 150, 154, 168, 255 });
    CuboCables({ 8.2f, 0.08f, 3.8f }, { 1.6f, 0.16f, 0.16f }, Color{ 200, 204, 214, 255 });
    CuboCables({ 7.1f, 0.12f, 3.8f }, { 0.5f, 0.24f, 0.30f }, Color{ 76, 150, 220, 255 });

    // Multimetro.
    CuboCables({ -8.6f, 0.14f, -1.6f }, { 1.1f, 0.28f, 1.6f }, Color{ 240, 200, 84, 255 });
    CuboCables({ -8.6f, 0.30f, -1.9f }, { 0.8f, 0.04f, 0.5f }, Color{ 120, 200, 170, 255 });
    CuboCables({ -8.6f, 0.30f, -1.2f }, { 0.3f, 0.06f, 0.3f }, Color{ 60, 66, 80, 255 });

    // Carrete de cable.
    (DrawCylinder)({ 8.7f, 0.0f, -1.8f }, 0.8f, 0.8f, 0.14f, 10, Color{ 96, 70, 48, 255 });
    (DrawCylinder)({ 8.7f, 0.14f, -1.8f }, 0.55f, 0.55f, 0.5f, 10, Color{ 176, 128, 84, 255 });
    (DrawCylinder)({ 8.7f, 0.64f, -1.8f }, 0.8f, 0.8f, 0.14f, 10, Color{ 96, 70, 48, 255 });
    (void)tiempo;
}


static void TextoContornoCables(const char* texto, int xCentro, int y, int tamano, Color color)
{
    int ancho = MeasureText(texto, tamano);
    int x = xCentro - ancho / 2;
    Color contorno = Color{ 30, 20, 12, 255 };

    DrawText(texto, x + 3, y + 3, tamano, Fade(BLACK, 0.55f));
    DrawText(texto, x - 2, y, tamano, contorno);
    DrawText(texto, x + 2, y, tamano, contorno);
    DrawText(texto, x, y - 2, tamano, contorno);
    DrawText(texto, x, y + 2, tamano, contorno);
    DrawText(texto, x, y, tamano, color);
}


void MinijuegoCablesCruzados::Dibujar(
    const JugadorPrueba jugadores[],
    int cantidadMaxima,
    const Participante participantes[],
    bool mostrarDebug
) const
{
    (void)jugadores;
    (void)cantidadMaxima;
    (void)mostrarDebug;

    ClearBackground(Color{ 24, 28, 34, 255 });

    int ancho = GetScreenWidth();
    int alto = GetScreenHeight();
    bool hayDuelo = partidaValida && lados[0].participante >= 0 && lados[1].participante >= 0;

    if (hayDuelo)
    {
        BeginMode3D(camara);

        DibujarEscenarioCables(tiempoAnimacion);

        for (int s = 0; s < 2; s++)
        {
            DibujarPanelCables(*this, s, participantes[lados[s].participante].color);
        }

        // Cursores (no durante el festejo de fin de ronda).
        if (fase != FASE_CABLES_FIN_RONDA && fase != FASE_CABLES_TERMINADO)
        {
            for (int s = 0; s < 2; s++)
            {
                const LadoCables& lado = lados[s];
                Vector3 c = CentroCeldaCables(s, lado.tablero, lado.cursor);

                if (lado.sacudida > 0.0f)
                {
                    c.x += std::sin(tiempoAnimacion * 90.0f) * 0.05f;
                }

                DibujarCursorCables(c, participantes[lado.participante].color, tiempoAnimacion);
            }
        }

        // Chispas.
        for (int i = 0; i < MAX_PARTICULAS_CABLES; i++)
        {
            const ParticulaCables& p = particulas[i];

            if (!p.activa)
            {
                continue;
            }

            float f = p.vida / p.vidaMaxima;
            CuboCables(p.posicion, { p.tamano, p.tamano, p.tamano }, MezclarColorCables(WHITE, p.color, 1.0f - f * 0.6f));
        }

        // Resplandor de las bombillas encendidas (aditivo, al final).
        if (ObtenerOpcionesCalidadGrafica().efectos != CALIDAD_BAJA)
        {
            BeginBlendMode(BLEND_ADDITIVE);

            // Charcos de luz calida de las lamparas sobre cada panel.
            for (int s = 0; s < 2; s++)
            {
                float cx = CentroPanelXCables(s);
                for (int r = 0; r < 4; r++)
                {
                    float radio = 5.4f - (float)r * 0.8f;
                    (DrawCylinder)({ cx, 0.012f + 0.002f * (float)r, 0.1f }, radio, radio, 0.002f, 24,
                        Fade(Color{ 255, 190, 110, 255 }, 0.018f));
                }
            }

            for (int s = 0; s < 2; s++)
            {
                const LadoCables& lado = lados[s];

                if (lado.brilloBombilla < 0.02f)
                {
                    continue;
                }

                Vector3 b = CentroCeldaCables(s, lado.tablero, lado.tablero.bombilla);
                float pulso = 1.0f + 0.06f * std::sin(tiempoAnimacion * 12.0f);
                (DrawSphere)({ b.x, b.y + 0.52f, b.z }, 0.62f * pulso, Fade(Color{ 255, 214, 110, 255 }, 0.32f * lado.brilloBombilla));
                (DrawSphere)({ b.x, b.y + 0.52f, b.z }, 0.95f * pulso, Fade(Color{ 255, 190, 80, 255 }, 0.12f * lado.brilloBombilla));
            }

            EndBlendMode();
        }

        EndMode3D();
    }

    // Anuncios sobre el mundo.
    if (hayDuelo && fase != FASE_CABLES_PREPARACION && fase != FASE_CABLES_TERMINADO)
    {
        int margen = MargenBarraRondaMinijuegos();
        TextoContornoCables(
            TextFormat("RONDA %d DE %d", ronda + 1, MAX_RONDAS_CABLES),
            ancho / 2,
            alto - 46 - margen,
            24,
            Color{ 255, 236, 170, 255 }
        );
    }

    if (hayDuelo && fase == FASE_CABLES_INTRO_RONDA)
    {
        float t = 1.0f - tiempoFase / DURACION_INTRO_RONDA_CABLES;
        float escala = t < 0.25f ? 0.6f + t * 1.6f : 1.0f;
        TextoContornoCables(
            TextFormat("RONDA %d", ronda + 1),
            ancho / 2,
            alto / 2 - (int)(40.0f * escala),
            (int)(80.0f * escala),
            Color{ 255, 236, 170, 255 }
        );
    }

    if (hayDuelo && fase == FASE_CABLES_FIN_RONDA && ladoGanadorRonda >= 0)
    {
        int numero = participantes[lados[ladoGanadorRonda].participante].numeroJugador;
        const char* texto = rondaPorTiempo
            ? TextFormat("¡TIEMPO! J%d LLEGÓ MÁS LEJOS", numero)
            : TextFormat("¡J%d ENCENDIÓ LA BOMBILLA!", numero);
        TextoContornoCables(texto, ancho / 2, (int)((float)alto * 0.2f), 34,
            MezclarColorCables(participantes[lados[ladoGanadorRonda].participante].color, WHITE, 0.5f));
    }

    // HUD comun.
    DatosHudMinijuego hud;
    PrepararDatosHudMinijuego(
        hud,
        "CABLES CRUZADOS",
        "DUELO: CONECTÁ LA PILA CON LA BOMBILLA GIRANDO LOS CABLES. GANA QUIEN LLEGUE A 2 RONDAS.",
        resultado
    );

    hud.fase = fase == FASE_CABLES_PREPARACION
        ? HUD_MINIJUEGO_CUENTA
        : (fase == FASE_CABLES_TERMINADO ? HUD_MINIJUEGO_FIN : HUD_MINIJUEGO_JUGANDO);
    hud.cuentaRestante = tiempoPreparacion;
    hud.reloj = HUD_RELOJ_CUENTA_ATRAS;
    hud.tiempoRestante = tiempoRestante;
    hud.tiempoTotal = duracionRonda;
    hud.segundosAlerta = 5.0f;
    hud.finPorTiempo = false;

    for (int s = 0; s < 2 && hayDuelo; s++)
    {
        const LadoCables& lado = lados[s];
        JugadorHudMinijuego& jugador = hud.jugadores[lado.participante];
        const Participante& participante = participantes[lado.participante];

        jugador.visible = true;
        jugador.rol = fase == FASE_CABLES_TERMINADO ? nullptr : "RONDAS";
        jugador.valorNumerico = lado.rondasGanadas;
        std::snprintf(jugador.valor, sizeof(jugador.valor), "%d/%d", lado.rondasGanadas, RONDAS_PARA_GANAR_CABLES);
        std::snprintf(jugador.detalleFinal, sizeof(jugador.detalleFinal), "%d RONDAS", lado.rondasGanadas);

        float avance = (float)lado.profundidad / (float)(lado.tablero.largoCamino + 1);
        jugador.progreso = lado.encendida ? 1.0f : (avance > 0.98f ? 0.98f : avance);

        if (!participante.esBot && participante.conectado)
        {
            std::snprintf(
                jugador.controles,
                sizeof(jugador.controles),
                "MOVÉ: %s\nGIRÁ: %s / %s",
                TextoMoverHud(participante),
                TextoBotonGolpeHud(participante),
                ObtenerTextoBotonPrincipal(participante)
            );
        }
    }

    DibujarHudMinijuego(hud, participantes);
}
