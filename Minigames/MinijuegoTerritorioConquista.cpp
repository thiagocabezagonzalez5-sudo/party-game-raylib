#include "Minigames/MinijuegoTerritorioConquista.h"

#include "Minigames/AudioMinijuegos.h"
#include "Minigames/MecanicasJugador.h"
#include "Minigames/UtilidadesMinijuegos.h"
#include "Systems/Input.h"

#include "raylib.h"

#include <cmath>
#include <cstdlib>
#include <ctime>


//==================================================
// CONSTANTES DE GAMEPLAY
//==================================================

static const float DURACION_PREPARACION_TERRITORIO = 3.0f;
static const float DURACION_PARTIDA_TERRITORIO = 38.0f;

// Geometria de la arena (el suelo fisico queda en Y = SUELO_TERRITORIO).
static const float TAMANO_BALDOSA_TERRITORIO = 1.3f;
static const float MITAD_ARENA_TERRITORIO =
    COLUMNAS_TERRITORIO * TAMANO_BALDOSA_TERRITORIO * 0.5f;
static const float SUELO_TERRITORIO = -0.05f;
static const float GROSOR_MURO_TERRITORIO = 0.40f;
static const float ALTO_MURO_TERRITORIO = 2.0f;

// Captura al caminar: segundos de permanencia sobre la baldosa.
static const float TIEMPO_CAPTURA_LIBRE = 0.20f;
static const float TIEMPO_CAPTURA_RIVAL = 0.34f;
static const float TIEMPO_OLVIDO_CAPTURA = 0.50f;

// Ground pound: area reclamada, empuje y aturdimiento de rivales.
static const float RADIO_RECLAMO_POUND = 2.2f;
static const float RADIO_EMPUJE_POUND = 2.5f;
static const float FUERZA_EMPUJE_POUND = 7.0f;
static const float DURACION_ATURDIDO_POUND = 1.0f;
static const float RECARGA_POUND_TERRITORIO = 1.0f;

// Regla secundaria: un sector (cuadrante) se sella periodicamente.
static const float TIEMPO_PRIMER_SELLADO = 7.0f;
static const float DURACION_AVISO_SELLADO = 1.6f;
static const float DURACION_SELLADO = 4.5f;
static const float PAUSA_ENTRE_SELLADOS = 5.0f;
static const float TIEMPO_MINIMO_NUEVO_SELLADO = 8.0f;

// Baldosas del obstaculo central (2x2).
static const int INICIO_OBSTACULO_TERRITORIO = 4;
static const int FIN_OBSTACULO_TERRITORIO = 5;


//==================================================
// UTILIDADES
//==================================================

static float LimitarTerritorio(float valor, float minimo, float maximo)
{
    if (valor < minimo) return minimo;
    if (valor > maximo) return maximo;
    return valor;
}


static Color MezclarColorTerritorio(Color a, Color b, float t)
{
    t = LimitarTerritorio(t, 0.0f, 1.0f);

    return Color{
        (unsigned char)(a.r + (b.r - a.r) * t),
        (unsigned char)(a.g + (b.g - a.g) * t),
        (unsigned char)(a.b + (b.b - a.b) * t),
        255
    };
}


// Valor pseudoaleatorio estable en [0, 1) para decorar sin parpadeos.
static float PseudoAleatorioTerritorio(int semilla)
{
    float valor = std::sin((float)semilla * 12.9898f) * 43758.5453f;
    return valor - std::floor(valor);
}


static Vector3 CentroBaldosaTerritorio(int columna, int fila)
{
    return Vector3{
        -MITAD_ARENA_TERRITORIO + (columna + 0.5f) * TAMANO_BALDOSA_TERRITORIO,
        0.0f,
        -MITAD_ARENA_TERRITORIO + (fila + 0.5f) * TAMANO_BALDOSA_TERRITORIO
    };
}


static bool BaldosaDePosicionTerritorio(
    Vector3 posicion,
    int& columna,
    int& fila
)
{
    columna = (int)std::floor(
        (posicion.x + MITAD_ARENA_TERRITORIO) / TAMANO_BALDOSA_TERRITORIO
    );
    fila = (int)std::floor(
        (posicion.z + MITAD_ARENA_TERRITORIO) / TAMANO_BALDOSA_TERRITORIO
    );

    return
        columna >= 0 && columna < COLUMNAS_TERRITORIO &&
        fila >= 0 && fila < FILAS_TERRITORIO;
}


static int IndiceBaldosaTerritorio(int columna, int fila)
{
    return fila * COLUMNAS_TERRITORIO + columna;
}


// Sectores: 0 = arriba izq, 1 = arriba der, 2 = abajo izq, 3 = abajo der.
static int SectorDeBaldosaTerritorio(int columna, int fila)
{
    return (fila >= FILAS_TERRITORIO / 2 ? 2 : 0) +
        (columna >= COLUMNAS_TERRITORIO / 2 ? 1 : 0);
}


static bool BaldosaSelladaTerritorio(
    const MinijuegoTerritorioConquista& minijuego,
    int columna,
    int fila
)
{
    return
        minijuego.estadoSector == SECTOR_TERRITORIO_SELLADO &&
        minijuego.sectorActual == SectorDeBaldosaTerritorio(columna, fila);
}


static bool BaldosaEnSectorActivoTerritorio(
    const MinijuegoTerritorioConquista& minijuego,
    int columna,
    int fila
)
{
    return
        minijuego.estadoSector != SECTOR_TERRITORIO_LIBRE &&
        minijuego.sectorActual == SectorDeBaldosaTerritorio(columna, fila);
}


static bool BaldosaReclamableTerritorio(
    const MinijuegoTerritorioConquista& minijuego,
    int columna,
    int fila
)
{
    if (
        columna < 0 || columna >= COLUMNAS_TERRITORIO ||
        fila < 0 || fila >= FILAS_TERRITORIO
    )
    {
        return false;
    }

    return
        !minijuego.baldosas[IndiceBaldosaTerritorio(columna, fila)].obstaculo &&
        !BaldosaSelladaTerritorio(minijuego, columna, fila);
}


static void ReclamarBaldosaTerritorio(
    MinijuegoTerritorioConquista& minijuego,
    int columna,
    int fila,
    int jugador
)
{
    BaldosaTerritorio& baldosa =
        minijuego.baldosas[IndiceBaldosaTerritorio(columna, fila)];

    baldosa.dueno = jugador;
    baldosa.candidato = -1;
    baldosa.progreso = 0.0f;
    baldosa.destello = 1.0f;
}


static int ContarBaldosasAreaTerritorio(
    const MinijuegoTerritorioConquista& minijuego,
    int jugador,
    Vector3 centro
)
{
    int cantidad = 0;

    for (int fila = 0; fila < FILAS_TERRITORIO; fila++)
    {
        for (int columna = 0; columna < COLUMNAS_TERRITORIO; columna++)
        {
            if (!BaldosaReclamableTerritorio(minijuego, columna, fila))
            {
                continue;
            }

            Vector3 c = CentroBaldosaTerritorio(columna, fila);
            float dx = c.x - centro.x;
            float dz = c.z - centro.z;

            if (
                dx * dx + dz * dz <= RADIO_RECLAMO_POUND * RADIO_RECLAMO_POUND &&
                minijuego.baldosas[IndiceBaldosaTerritorio(columna, fila)].dueno
                    != jugador
            )
            {
                cantidad++;
            }
        }
    }

    return cantidad;
}


static void ReclamarAreaPoundTerritorio(
    MinijuegoTerritorioConquista& minijuego,
    int jugador,
    Vector3 centro
)
{
    for (int fila = 0; fila < FILAS_TERRITORIO; fila++)
    {
        for (int columna = 0; columna < COLUMNAS_TERRITORIO; columna++)
        {
            if (!BaldosaReclamableTerritorio(minijuego, columna, fila))
            {
                continue;
            }

            Vector3 c = CentroBaldosaTerritorio(columna, fila);
            float dx = c.x - centro.x;
            float dz = c.z - centro.z;

            if (dx * dx + dz * dz <= RADIO_RECLAMO_POUND * RADIO_RECLAMO_POUND)
            {
                ReclamarBaldosaTerritorio(minijuego, columna, fila, jugador);
            }
        }
    }
}


static void RecontarBaldosasTerritorio(MinijuegoTerritorioConquista& minijuego)
{
    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        minijuego.estadosJugadores[i].baldosas = 0;
    }

    for (int i = 0; i < MAX_BALDOSAS_TERRITORIO; i++)
    {
        int dueno = minijuego.baldosas[i].dueno;

        if (dueno >= 0 && dueno < MAX_PARTICIPANTES)
        {
            minijuego.estadosJugadores[dueno].baldosas++;
        }
    }
}


static float PorcentajeTerritorio(
    const MinijuegoTerritorioConquista& minijuego,
    int jugador
)
{
    if (minijuego.totalBaldosasJugables <= 0)
    {
        return 0.0f;
    }

    return 100.0f * (float)minijuego.estadosJugadores[jugador].baldosas /
        (float)minijuego.totalBaldosasJugables;
}


static void CrearOndaTerritorio(
    MinijuegoTerritorioConquista& minijuego,
    Vector3 posicion,
    Color color
)
{
    for (int i = 0; i < MAX_ONDAS_TERRITORIO; i++)
    {
        if (minijuego.ondas[i].activa)
        {
            continue;
        }

        minijuego.ondas[i].activa = true;
        minijuego.ondas[i].posicion = posicion;
        minijuego.ondas[i].tiempo = 0.0f;
        minijuego.ondas[i].color = color;
        return;
    }
}


//==================================================
// COLORES DE JUGADOR
//==================================================

static bool ColoresCercanosTerritorio(Color a, Color b)
{
    int diferencia =
        std::abs((int)a.r - (int)b.r) +
        std::abs((int)a.g - (int)b.g) +
        std::abs((int)a.b - (int)b.b);

    return diferencia < 130;
}


// Los bots llegan en gris desde el tablero. Para poder distinguir el
// territorio de cada uno se les asigna un color propio que no choque con los
// de los humanos ni con otros bots.
static void AsignarColoresTerritorio(
    MinijuegoTerritorioConquista& minijuego,
    const Participante participantes[]
)
{
    const Color respaldo[] =
    {
        Color{ 232, 62, 62, 255 },
        Color{ 62, 124, 238, 255 },
        Color{ 66, 202, 96, 255 },
        Color{ 246, 206, 52, 255 },
        Color{ 232, 92, 204, 255 },
        Color{ 250, 144, 44, 255 }
    };
    const int cantidadRespaldo = (int)(sizeof(respaldo) / sizeof(respaldo[0]));

    bool asignado[MAX_PARTICIPANTES]{};

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        minijuego.coloresJugadores[i] = participantes[i].color;

        if (participantes[i].activo && !participantes[i].esBot)
        {
            asignado[i] = true;
        }
    }

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!participantes[i].activo || asignado[i])
        {
            continue;
        }

        for (int c = 0; c < cantidadRespaldo; c++)
        {
            bool libre = true;

            for (int j = 0; j < MAX_PARTICIPANTES; j++)
            {
                if (
                    j != i &&
                    participantes[j].activo &&
                    (asignado[j]) &&
                    ColoresCercanosTerritorio(
                        respaldo[c],
                        minijuego.coloresJugadores[j]
                    )
                )
                {
                    libre = false;
                    break;
                }
            }

            if (libre)
            {
                minijuego.coloresJugadores[i] = respaldo[c];
                break;
            }
        }

        asignado[i] = true;
    }
}


//==================================================
// IA DE BOTS
//==================================================

static bool BaldosaUtilParaBotTerritorio(
    const MinijuegoTerritorioConquista& minijuego,
    int columna,
    int fila,
    int indiceBot
)
{
    if (
        !BaldosaReclamableTerritorio(minijuego, columna, fila) ||
        BaldosaEnSectorActivoTerritorio(minijuego, columna, fila)
    )
    {
        return false;
    }

    return
        minijuego.baldosas[IndiceBaldosaTerritorio(columna, fila)].dueno
            != indiceBot;
}


static void ElegirObjetivoBotTerritorio(
    MinijuegoTerritorioConquista& minijuego,
    int indice,
    const JugadorPrueba& jugador
)
{
    EstadoJugadorTerritorio& estado = minijuego.estadosJugadores[indice];

    float mejorPuntaje = -100000.0f;
    int mejorColumna = -1;
    int mejorFila = -1;

    for (int fila = 0; fila < FILAS_TERRITORIO; fila++)
    {
        for (int columna = 0; columna < COLUMNAS_TERRITORIO; columna++)
        {
            if (!BaldosaUtilParaBotTerritorio(minijuego, columna, fila, indice))
            {
                continue;
            }

            Vector3 centro = CentroBaldosaTerritorio(columna, fila);
            float dx = centro.x - jugador.posicion.x;
            float dz = centro.z - jugador.posicion.z;
            float distancia =
                std::sqrt(dx * dx + dz * dz) / TAMANO_BALDOSA_TERRITORIO;

            int vecinos = 0;

            for (int vf = -1; vf <= 1; vf++)
            {
                for (int vc = -1; vc <= 1; vc++)
                {
                    if (
                        (vf != 0 || vc != 0) &&
                        BaldosaUtilParaBotTerritorio(
                            minijuego,
                            columna + vc,
                            fila + vf,
                            indice
                        )
                    )
                    {
                        vecinos++;
                    }
                }
            }

            bool rival =
                minijuego.baldosas[IndiceBaldosaTerritorio(columna, fila)].dueno
                    >= 0;

            float puntaje =
                (rival ? 1.25f : 1.0f) +
                0.30f * (float)vecinos -
                0.22f * distancia +
                (float)GetRandomValue(0, 60) / 100.0f;

            if (puntaje > mejorPuntaje)
            {
                mejorPuntaje = puntaje;
                mejorColumna = columna;
                mejorFila = fila;
            }
        }
    }

    estado.objetivoColumna = mejorColumna;
    estado.objetivoFila = mejorFila;
    estado.tiempoReevaluar = (float)GetRandomValue(70, 120) / 100.0f;
}


static InputMinijuegoParticipante CrearEntradaBotTerritorio(
    MinijuegoTerritorioConquista& minijuego,
    int indice,
    const JugadorPrueba& jugador,
    float deltaTime
)
{
    InputMinijuegoParticipante entrada{};
    EstadoJugadorTerritorio& estado = minijuego.estadosJugadores[indice];

    estado.tiempoReevaluar -= deltaTime;
    estado.recargaBot -= deltaTime;
    estado.tiempoSalto -= deltaTime;

    bool objetivoValido =
        estado.objetivoColumna >= 0 &&
        BaldosaUtilParaBotTerritorio(
            minijuego,
            estado.objetivoColumna,
            estado.objetivoFila,
            indice
        );

    if (!objetivoValido || estado.tiempoReevaluar <= 0.0f)
    {
        ElegirObjetivoBotTerritorio(minijuego, indice, jugador);
    }

    if (estado.objetivoColumna >= 0)
    {
        Vector3 destino = CentroBaldosaTerritorio(
            estado.objetivoColumna,
            estado.objetivoFila
        );
        float dx = destino.x - jugador.posicion.x;
        float dz = destino.z - jugador.posicion.z;

        if (dx < -0.2f) entrada.izquierda = true;
        else if (dx > 0.2f) entrada.derecha = true;

        if (dz < -0.2f) entrada.adelante = true;
        else if (dz > 0.2f) entrada.atras = true;
    }

    // Secuencia de ground pound: saltar, esperar un instante en el aire y
    // volver a pulsar salto.
    if (estado.faseSalto == 1)
    {
        if (
            estado.tiempoSalto <= 0.0f &&
            !jugador.enSuelo &&
            !jugador.preparandoGolpeSuelo &&
            !jugador.golpeSueloActivo
        )
        {
            entrada.saltar = true;
            estado.faseSalto = 2;
        }
    }

    if (
        estado.faseSalto != 0 &&
        jugador.enSuelo &&
        estado.tiempoSalto < -0.25f
    )
    {
        estado.faseSalto = 0;
    }

    bool puedeSaltar =
        estado.faseSalto == 0 &&
        jugador.enSuelo &&
        !jugador.aplastado &&
        !jugador.golpeSueloActivo &&
        !jugador.preparandoGolpeSuelo &&
        estado.recargaBot <= 0.0f &&
        estado.recargaPound <= 0.0f;

    int umbral = 5 + (indice % 3);

    if (
        puedeSaltar &&
        ContarBaldosasAreaTerritorio(minijuego, indice, jugador.posicion)
            >= umbral
    )
    {
        entrada.saltar = true;
        estado.faseSalto = 1;
        estado.tiempoSalto = 0.24f + (float)GetRandomValue(0, 12) / 100.0f;
        estado.recargaBot = 1.3f + 0.4f * (float)(indice % 3);
    }

    return entrada;
}


//==================================================
// REGLA DEL SECTOR SELLADO
//==================================================

static void ActualizarSectorTerritorio(
    MinijuegoTerritorioConquista& minijuego,
    float deltaTime
)
{
    minijuego.tiempoSector -= deltaTime;

    if (minijuego.estadoSector == SECTOR_TERRITORIO_LIBRE)
    {
        if (
            minijuego.tiempoSector <= 0.0f &&
            minijuego.tiempoRestante > TIEMPO_MINIMO_NUEVO_SELLADO
        )
        {
            int sector = GetRandomValue(0, 3);

            if (sector == minijuego.ultimoSector)
            {
                sector = (sector + 1 + GetRandomValue(0, 2)) % 4;
            }

            minijuego.sectorActual = sector;
            minijuego.estadoSector = SECTOR_TERRITORIO_AVISO;
            minijuego.tiempoSector = DURACION_AVISO_SELLADO;
        }
    }
    else if (minijuego.estadoSector == SECTOR_TERRITORIO_AVISO)
    {
        if (minijuego.tiempoSector <= 0.0f)
        {
            minijuego.estadoSector = SECTOR_TERRITORIO_SELLADO;
            minijuego.tiempoSector = DURACION_SELLADO;
        }
    }
    else if (minijuego.tiempoSector <= 0.0f)
    {
        minijuego.ultimoSector = minijuego.sectorActual;
        minijuego.sectorActual = -1;
        minijuego.estadoSector = SECTOR_TERRITORIO_LIBRE;
        minijuego.tiempoSector = PAUSA_ENTRE_SELLADOS;
    }
}


//==================================================
// FIN DE PARTIDA
//==================================================

static void FinalizarTerritorio(MinijuegoTerritorioConquista& minijuego)
{
    if (minijuego.resultado.estado == RESULTADO_MINIJUEGO_FINALIZADO)
    {
        return;
    }

    RecontarBaldosasTerritorio(minijuego);

    int cantidadPrimeros = 0;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        ResultadoParticipante& resultadoJugador =
            minijuego.resultado.participantes[i];

        if (!resultadoJugador.participo)
        {
            continue;
        }

        int posicion = 1;

        for (int j = 0; j < MAX_PARTICIPANTES; j++)
        {
            if (
                minijuego.resultado.participantes[j].participo &&
                minijuego.estadosJugadores[j].baldosas >
                    minijuego.estadosJugadores[i].baldosas
            )
            {
                posicion++;
            }
        }

        resultadoJugador.posicionFinal = posicion;
        resultadoJugador.numeroEquipo = -1;
        resultadoJugador.puntuacionMinijuego =
            minijuego.estadosJugadores[i].baldosas;
        resultadoJugador.puntosObtenidos = 0;

        if (posicion == 1)
        {
            cantidadPrimeros++;
        }
    }

    minijuego.resultado.estado = RESULTADO_MINIJUEGO_FINALIZADO;
    ReproducirSonidoMinijuego(minijuego.audio, SONIDO_RESULTADO);
    minijuego.resultado.desenlace =
        cantidadPrimeros == 1 ? DESENLACE_CON_GANADOR : DESENLACE_EMPATE;
    minijuego.fase = FASE_TERRITORIO_TERMINADO;
}


//==================================================
// CONFIGURACION DE LA ARENA (GAMEPLAY)
//==================================================

static void ConfigurarArenaTerritorio(MinijuegoTerritorioConquista& minijuego)
{
    minijuego.cantidadBloques = 0;

    const float ancho = COLUMNAS_TERRITORIO * TAMANO_BALDOSA_TERRITORIO;
    const float centroMuroY = SUELO_TERRITORIO + ALTO_MURO_TERRITORIO * 0.5f;
    const float posicionMuro = MITAD_ARENA_TERRITORIO + GROSOR_MURO_TERRITORIO * 0.5f;
    const float largoMuro = ancho + GROSOR_MURO_TERRITORIO * 2.0f;
    const Color gris = Color{ 80, 80, 80, 255 };

    // 0: suelo.
    AgregarBloquePrueba(minijuego.bloques, minijuego.cantidadBloques,
        MAX_BLOQUES_TERRITORIO,
        { 0.0f, SUELO_TERRITORIO - 0.5f, 0.0f },
        { ancho, 1.0f, ancho }, gris);
    // 1 y 2: muros izquierdo y derecho.
    AgregarBloquePrueba(minijuego.bloques, minijuego.cantidadBloques,
        MAX_BLOQUES_TERRITORIO,
        { -posicionMuro, centroMuroY, 0.0f },
        { GROSOR_MURO_TERRITORIO, ALTO_MURO_TERRITORIO, largoMuro }, gris);
    AgregarBloquePrueba(minijuego.bloques, minijuego.cantidadBloques,
        MAX_BLOQUES_TERRITORIO,
        { posicionMuro, centroMuroY, 0.0f },
        { GROSOR_MURO_TERRITORIO, ALTO_MURO_TERRITORIO, largoMuro }, gris);
    // 3 y 4: muros trasero y delantero.
    AgregarBloquePrueba(minijuego.bloques, minijuego.cantidadBloques,
        MAX_BLOQUES_TERRITORIO,
        { 0.0f, centroMuroY, -posicionMuro },
        { ancho, ALTO_MURO_TERRITORIO, GROSOR_MURO_TERRITORIO }, gris);
    AgregarBloquePrueba(minijuego.bloques, minijuego.cantidadBloques,
        MAX_BLOQUES_TERRITORIO,
        { 0.0f, centroMuroY, posicionMuro },
        { ancho, ALTO_MURO_TERRITORIO, GROSOR_MURO_TERRITORIO }, gris);
    // 5: obstaculo central (cubre las 2x2 baldosas del medio).
    const float ladoObstaculo = TAMANO_BALDOSA_TERRITORIO * 2.0f;
    AgregarBloquePrueba(minijuego.bloques, minijuego.cantidadBloques,
        MAX_BLOQUES_TERRITORIO,
        { 0.0f, centroMuroY, 0.0f },
        { ladoObstaculo, ALTO_MURO_TERRITORIO, ladoObstaculo }, gris);
}


//==================================================
// INICIALIZACION Y REINICIO
//==================================================

void MinijuegoTerritorioConquista::Inicializar()
{
    resultado = {};
    resultado.formato = FORMATO_MINIJUEGO_INDIVIDUAL;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        estadosJugadores[i] = {};
        coloresJugadores[i] = LIGHTGRAY;
    }

    totalBaldosasJugables = 0;

    for (int fila = 0; fila < FILAS_TERRITORIO; fila++)
    {
        for (int columna = 0; columna < COLUMNAS_TERRITORIO; columna++)
        {
            BaldosaTerritorio& baldosa =
                baldosas[IndiceBaldosaTerritorio(columna, fila)];
            baldosa = {};
            baldosa.obstaculo =
                columna >= INICIO_OBSTACULO_TERRITORIO &&
                columna <= FIN_OBSTACULO_TERRITORIO &&
                fila >= INICIO_OBSTACULO_TERRITORIO &&
                fila <= FIN_OBSTACULO_TERRITORIO;

            if (!baldosa.obstaculo)
            {
                totalBaldosasJugables++;
            }
        }
    }

    for (int i = 0; i < MAX_PARTICULAS_TERRITORIO; i++)
    {
        particulas[i] = {};
    }

    for (int i = 0; i < MAX_ONDAS_TERRITORIO; i++)
    {
        ondas[i] = {};
    }

    ConfigurarArenaTerritorio(*this);

    // Tema pseudoaleatorio segun el reloj del sistema.
    tema = (TemaTerritorio)(
        (int)(std::time(nullptr) % (std::time_t)CANTIDAD_TEMAS_TERRITORIO)
    );

    camara.position = { 0.0f, 18.5f, 10.5f };
    camara.target = { 0.0f, 0.0f, 0.4f };
    camara.up = { 0.0f, 1.0f, 0.0f };
    camara.fovy = 46.0f;
    camara.projection = CAMERA_PERSPECTIVE;

    fase = FASE_TERRITORIO_PREPARACION;
    estadoSector = SECTOR_TERRITORIO_LIBRE;
    sectorActual = -1;
    ultimoSector = -1;
    tiempoSector = TIEMPO_PRIMER_SELLADO;
    tiempoPreparacion = DURACION_PREPARACION_TERRITORIO;
    tiempoRestante = DURACION_PARTIDA_TERRITORIO;
    tiempoAnimacion = 0.0f;
}


void MinijuegoTerritorioConquista::Reiniciar(
    JugadorPrueba jugadores[],
    Participante participantes[],
    int cantidadMaxima
)
{
    Inicializar();
    InicializarResultadoMinijuego(
        resultado,
        participantes,
        FORMATO_MINIJUEGO_INDIVIDUAL
    );

    AsignarColoresTerritorio(*this, participantes);

    int indices[MAX_PARTICIPANTES]{};
    int cantidad = ObtenerIndicesParticipantesActivos(
        participantes,
        indices,
        MAX_PARTICIPANTES
    );

    if (cantidad < 2)
    {
        resultado.estado = RESULTADO_MINIJUEGO_CANCELADO;
        fase = FASE_TERRITORIO_TERMINADO;
        return;
    }

    // Con 2 jugadores quedan en esquinas opuestas.
    const float esquina = MITAD_ARENA_TERRITORIO - TAMANO_BALDOSA_TERRITORIO * 0.5f;
    const Vector3 esquinas[MAX_PARTICIPANTES] =
    {
        { -esquina, 0.0f, esquina },
        { esquina, 0.0f, -esquina },
        { esquina, 0.0f, esquina },
        { -esquina, 0.0f, -esquina }
    };

    int limite = cantidadMaxima < MAX_PARTICIPANTES
        ? cantidadMaxima
        : MAX_PARTICIPANTES;

    for (int k = 0; k < cantidad; k++)
    {
        int i = indices[k];

        if (i >= limite)
        {
            continue;
        }

        Vector3 spawn = esquinas[k % MAX_PARTICIPANTES];
        spawn.y = SUELO_TERRITORIO + 0.72f;

        ConfigurarJugadorMinijuegoEstandar(jugadores[i], spawn);

        float dx = -spawn.x;
        float dz = -spawn.z;
        float longitud = std::sqrt(dx * dx + dz * dz);
        jugadores[i].direccionMirada = { dx / longitud, 0.0f, dz / longitud };
    }
}


//==================================================
// ACTUALIZACION
//==================================================

void MinijuegoTerritorioConquista::Actualizar(
    float deltaTime,
    JugadorPrueba jugadores[],
    int cantidadMaxima,
    Participante participantes[]
)
{
    tiempoAnimacion += deltaTime;

    ActualizarParticulasTierra(particulas, MAX_PARTICULAS_TERRITORIO, deltaTime);

    for (int i = 0; i < MAX_ONDAS_TERRITORIO; i++)
    {
        if (!ondas[i].activa)
        {
            continue;
        }

        ondas[i].tiempo += deltaTime;

        if (ondas[i].tiempo > 0.45f)
        {
            ondas[i].activa = false;
        }
    }

    for (int i = 0; i < MAX_BALDOSAS_TERRITORIO; i++)
    {
        if (baldosas[i].destello > 0.0f)
        {
            baldosas[i].destello -= deltaTime * 3.0f;

            if (baldosas[i].destello < 0.0f)
            {
                baldosas[i].destello = 0.0f;
            }
        }
    }

    if (
        fase == FASE_TERRITORIO_TERMINADO ||
        resultado.estado == RESULTADO_MINIJUEGO_CANCELADO
    )
    {
        return;
    }

    if (fase == FASE_TERRITORIO_PREPARACION)
    {
        float preparacionAntes = tiempoPreparacion;
        tiempoPreparacion -= deltaTime;
        ActualizarAudioCuentaRegresiva(audio, preparacionAntes, tiempoPreparacion);

        if (tiempoPreparacion <= 0.0f)
        {
            tiempoPreparacion = 0.0f;
            fase = FASE_TERRITORIO_JUGANDO;
        }

        return;
    }

    int limite = cantidadMaxima < MAX_PARTICIPANTES
        ? cantidadMaxima
        : MAX_PARTICIPANTES;

    float restanteAntes = tiempoRestante;
    tiempoRestante -= deltaTime;
    ActualizarAudioAlertaTiempo(audio, restanteAntes, tiempoRestante);

    if (tiempoRestante < 0.0f)
    {
        tiempoRestante = 0.0f;
    }

    ActualizarSectorTerritorio(*this, deltaTime);

    // Un humano desconectado se trata como bot; para las funciones
    // compartidas se le considera presente igualmente.
    Participante efectivos[MAX_PARTICIPANTES];

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        efectivos[i] = participantes[i];

        if (efectivos[i].activo)
        {
            efectivos[i].conectado = true;
        }
    }

    // 1. Movimiento y fisica estandar de cada jugador.
    for (int i = 0; i < limite; i++)
    {
        if (!resultado.participantes[i].participo)
        {
            continue;
        }

        JugadorPrueba& jugador = jugadores[i];
        EstadoJugadorTerritorio& estado = estadosJugadores[i];

        estado.recargaPound -= deltaTime;

        if (estado.recargaPound < 0.0f)
        {
            estado.recargaPound = 0.0f;
        }

        InputMinijuegoParticipante entrada{};

        if (participantes[i].esBot || !participantes[i].conectado)
        {
            entrada = CrearEntradaBotTerritorio(*this, i, jugador, deltaTime);
        }
        else
        {
            entrada = LeerInputMinijuegoParticipante(participantes[i]);
        }

        // Solo existe el salto / ground pound: no hay golpe horizontal.
        entrada.golpear = false;

        // Recarga: mientras dura, el salto en el aire no inicia otro pound.
        if (estado.recargaPound > 0.0f && !jugador.enSuelo)
        {
            entrada.saltar = false;
        }

        ActualizarJugadorPruebaNormal(
            jugador,
            entrada,
            bloques,
            cantidadBloques,
            particulas,
            MAX_PARTICULAS_TERRITORIO,
            true,
            true,
            deltaTime
        );

        // Red de seguridad: nunca se permite salir de la arena.
        const float limiteXZ = MITAD_ARENA_TERRITORIO - 0.4f;
        jugador.posicion.x = LimitarTerritorio(jugador.posicion.x, -limiteXZ, limiteXZ);
        jugador.posicion.z = LimitarTerritorio(jugador.posicion.z, -limiteXZ, limiteXZ);

        if (jugador.cayendo || jugador.posicion.y < -3.0f)
        {
            ReiniciarJugadorPrueba(jugador);
        }
    }

    // 2. Impactos de ground pound: reclamo de area y empuje. Debe ir antes de
    //    ResolverGolpesSuelo, que consume la bandera impactoGolpeSuelo.
    Vector3 empujes[MAX_PARTICIPANTES]{};

    for (int i = 0; i < limite; i++)
    {
        if (
            !resultado.participantes[i].participo ||
            !jugadores[i].impactoGolpeSuelo
        )
        {
            continue;
        }

        Vector3 centro = jugadores[i].posicion;

        ReclamarAreaPoundTerritorio(*this, i, centro);
        CrearOndaTerritorio(
            *this,
            { centro.x, 0.05f, centro.z },
            coloresJugadores[i]
        );
        ActivarTemblorCamaraGeneral(0.14f, 0.22f);
        estadosJugadores[i].recargaPound = RECARGA_POUND_TERRITORIO;

        for (int j = 0; j < limite; j++)
        {
            if (
                j == i ||
                !resultado.participantes[j].participo ||
                jugadores[j].cayendo ||
                jugadores[j].tiempoInmunidad > 0.0f
            )
            {
                continue;
            }

            float dx = jugadores[j].posicion.x - centro.x;
            float dz = jugadores[j].posicion.z - centro.z;
            float distancia = std::sqrt(dx * dx + dz * dz);

            if (distancia > RADIO_EMPUJE_POUND)
            {
                continue;
            }

            if (distancia < 0.05f)
            {
                dx = 1.0f;
                dz = 0.0f;
                distancia = 1.0f;
            }

            float intensidad = 1.0f - 0.55f * (distancia / RADIO_EMPUJE_POUND);
            empujes[j].x += dx / distancia * FUERZA_EMPUJE_POUND * intensidad;
            empujes[j].z += dz / distancia * FUERZA_EMPUJE_POUND * intensidad;
        }
    }

    ResolverGolpesSuelo(
        jugadores,
        efectivos,
        limite,
        DURACION_ATURDIDO_POUND
    );

    for (int j = 0; j < limite; j++)
    {
        jugadores[j].empuje.x += empujes[j].x;
        jugadores[j].empuje.z += empujes[j].z;
    }

    ResolverColisionesJugadoresSinEmpuje(jugadores, efectivos, limite);

    // 3. Captura suave al caminar.
    for (int i = 0; i < MAX_BALDOSAS_TERRITORIO; i++)
    {
        baldosas[i].tocadaEsteFrame = false;
    }

    for (int i = 0; i < limite; i++)
    {
        const JugadorPrueba& jugador = jugadores[i];

        if (
            !resultado.participantes[i].participo ||
            jugador.cayendo ||
            !jugador.enSuelo ||
            jugador.aplastado
        )
        {
            continue;
        }

        int columna = 0;
        int fila = 0;

        if (
            !BaldosaDePosicionTerritorio(jugador.posicion, columna, fila) ||
            !BaldosaReclamableTerritorio(*this, columna, fila)
        )
        {
            continue;
        }

        BaldosaTerritorio& baldosa =
            baldosas[IndiceBaldosaTerritorio(columna, fila)];

        if (baldosa.dueno == i)
        {
            continue;
        }

        float necesario =
            baldosa.dueno < 0 ? TIEMPO_CAPTURA_LIBRE : TIEMPO_CAPTURA_RIVAL;

        baldosa.tocadaEsteFrame = true;

        if (baldosa.candidato != i)
        {
            if (baldosa.candidato < 0 || baldosa.progreso <= 0.0f)
            {
                baldosa.candidato = i;
                baldosa.progreso = 0.0f;
            }
            else
            {
                // Otro jugador pisa una baldosa que un rival estaba marcando.
                baldosa.progreso -= deltaTime / necesario * 1.5f;

                if (baldosa.progreso <= 0.0f)
                {
                    baldosa.candidato = i;
                    baldosa.progreso = 0.0f;
                }
            }
        }

        if (baldosa.candidato == i)
        {
            baldosa.progreso += deltaTime / necesario;

            if (baldosa.progreso >= 1.0f)
            {
                ReclamarBaldosaTerritorio(*this, columna, fila, i);
            }
        }
    }

    for (int i = 0; i < MAX_BALDOSAS_TERRITORIO; i++)
    {
        BaldosaTerritorio& baldosa = baldosas[i];

        if (baldosa.candidato >= 0 && !baldosa.tocadaEsteFrame)
        {
            baldosa.progreso -= deltaTime / TIEMPO_OLVIDO_CAPTURA;

            if (baldosa.progreso <= 0.0f)
            {
                baldosa.progreso = 0.0f;
                baldosa.candidato = -1;
            }
        }
    }

    RecontarBaldosasTerritorio(*this);

    if (tiempoRestante <= 0.0f)
    {
        FinalizarTerritorio(*this);
    }
}


//==================================================
// VISUAL: PALETAS POR TEMA
//==================================================

struct PaletaTerritorio
{
    Color fondo;
    Color exterior;
    Color baldosaA;
    Color baldosaB;
    Color rejilla;
    Color muro;
    Color muroAcento;
    Color sellado;
    Color acento;
    const char* nombreTema;
    const char* nombreSector;
};


static PaletaTerritorio ObtenerPaletaTerritorio(TemaTerritorio tema)
{
    switch (tema)
    {
    case TEMA_TERRITORIO_CYBER:
        return {
            Color{ 8, 10, 28, 255 }, Color{ 10, 14, 36, 255 },
            Color{ 150, 158, 176, 255 }, Color{ 130, 138, 158, 255 },
            Color{ 14, 20, 44, 255 }, Color{ 24, 30, 58, 255 },
            Color{ 0, 235, 255, 255 }, Color{ 40, 14, 60, 255 },
            Color{ 255, 60, 220, 255 },
            "CIUDAD CYBER", "CORTAFUEGOS"
        };
    case TEMA_TERRITORIO_DULCE:
        return {
            Color{ 255, 214, 232, 255 }, Color{ 255, 226, 238, 255 },
            Color{ 236, 224, 214, 255 }, Color{ 222, 208, 196, 255 },
            Color{ 150, 100, 90, 255 }, Color{ 255, 140, 180, 255 },
            Color{ 255, 255, 255, 255 }, Color{ 150, 90, 60, 255 },
            Color{ 255, 90, 120, 255 },
            "MUNDO DULCE", "CARAMELO FUNDIDO"
        };
    case TEMA_TERRITORIO_JARDIN:
        return {
            Color{ 150, 214, 255, 255 }, Color{ 96, 160, 70, 255 },
            Color{ 214, 200, 170, 255 }, Color{ 198, 182, 150, 255 },
            Color{ 90, 120, 60, 255 }, Color{ 46, 130, 58, 255 },
            Color{ 255, 190, 60, 255 }, Color{ 70, 50, 30, 255 },
            Color{ 255, 120, 160, 255 },
            "JARDIN GIGANTE", "FUMIGACION"
        };
    case TEMA_TERRITORIO_TOXICO:
        return {
            Color{ 26, 40, 22, 255 }, Color{ 54, 96, 30, 255 },
            Color{ 178, 178, 170, 255 }, Color{ 158, 158, 150, 255 },
            Color{ 36, 44, 30, 255 }, Color{ 236, 200, 30, 255 },
            Color{ 36, 36, 36, 255 }, Color{ 60, 90, 20, 255 },
            Color{ 150, 255, 60, 255 },
            "ZONA TOXICA", "FUGA TOXICA"
        };
    case TEMA_TERRITORIO_VOLCAN:
    default:
        return {
            Color{ 70, 26, 24, 255 }, Color{ 230, 90, 20, 255 },
            Color{ 176, 160, 150, 255 }, Color{ 158, 142, 132, 255 },
            Color{ 38, 28, 28, 255 }, Color{ 60, 46, 44, 255 },
            Color{ 255, 120, 20, 255 }, Color{ 40, 20, 20, 255 },
            Color{ 255, 190, 40, 255 },
            "VOLCAN", "COLADA DE LAVA"
        };
    }
}


//==================================================
// VISUAL: ESCENARIO
//==================================================
//
// Todo lo siguiente es solo presentacion: no toca colisiones ni reglas. La
// geometria fisica sigue siendo la de "bloques".
//==================================================

// Obstaculo central tematico. La colision real es el cubo del medio.
// MODELO FUTURO: reemplazar por un .glb del obstaculo (cono volcanico,
// pilar de neon, cupcake, maceta con flor gigante o pila de barriles).
static void DibujarObstaculoCentralVisual(
    TemaTerritorio tema,
    const PaletaTerritorio& paleta,
    float t
)
{
    const float y = SUELO_TERRITORIO;

    switch (tema)
    {
    case TEMA_TERRITORIO_CYBER:
        DrawCube({ 0.0f, y + 1.2f, 0.0f }, 1.7f, 2.4f, 1.7f, Color{ 20, 26, 52, 255 });
        DrawCubeWires({ 0.0f, y + 1.2f, 0.0f }, 1.72f, 2.42f, 1.72f, paleta.muroAcento);
        for (int k = 0; k < 3; k++)
        {
            float altura = y + 0.5f + 0.7f * (float)k;
            DrawCylinder({ 0.0f, altura, 0.0f }, 1.45f, 1.45f, 0.06f, 20,
                Fade(k % 2 == 0 ? paleta.muroAcento : paleta.acento, 0.8f));
        }
        DrawSphere({ 0.0f, y + 2.8f + std::sin(t * 3.0f) * 0.12f, 0.0f }, 0.3f, paleta.acento);
        break;

    case TEMA_TERRITORIO_DULCE:
        DrawCylinder({ 0.0f, y, 0.0f }, 1.2f, 0.9f, 1.2f, 20, Color{ 190, 130, 80, 255 });
        DrawSphere({ 0.0f, y + 1.55f, 0.0f }, 1.15f, Color{ 255, 160, 200, 255 });
        DrawSphere({ 0.0f, y + 2.25f, 0.0f }, 0.8f, Color{ 255, 255, 255, 255 });
        DrawSphere({ 0.0f, y + 2.85f, 0.0f }, 0.27f, Color{ 220, 30, 50, 255 });
        break;

    case TEMA_TERRITORIO_JARDIN:
        DrawCylinder({ 0.0f, y, 0.0f }, 1.25f, 0.95f, 1.2f, 20, Color{ 190, 96, 60, 255 });
        DrawCylinder({ 0.0f, y + 1.2f, 0.0f }, 0.14f, 0.14f, 1.4f, 8, Color{ 50, 150, 60, 255 });
        for (int k = 0; k < 8; k++)
        {
            float angulo = (float)k * 0.785398f + t * 0.3f;
            DrawSphere(
                { std::cos(angulo) * 0.55f, y + 2.7f, std::sin(angulo) * 0.55f },
                0.3f,
                k % 2 == 0 ? Color{ 255, 120, 170, 255 } : Color{ 255, 170, 210, 255 }
            );
        }
        DrawSphere({ 0.0f, y + 2.75f, 0.0f }, 0.32f, Color{ 255, 205, 40, 255 });
        break;

    case TEMA_TERRITORIO_TOXICO:
        for (int k = 0; k < 4; k++)
        {
            float bx = (k % 2 == 0 ? -0.62f : 0.62f);
            float bz = (k < 2 ? -0.62f : 0.62f);
            DrawCylinder({ bx, y, bz }, 0.52f, 0.52f, 1.5f, 14, Color{ 90, 140, 40, 255 });
            DrawCylinder({ bx, y + 0.5f, bz }, 0.54f, 0.54f, 0.22f, 14, Color{ 30, 30, 30, 255 });
        }
        DrawCylinder({ 0.0f, y + 1.5f, 0.0f }, 0.55f, 0.55f, 1.0f, 14, Color{ 236, 200, 30, 255 });
        DrawSphere({ 0.0f, y + 2.7f + std::sin(t * 2.0f) * 0.1f, 0.0f }, 0.28f, Fade(paleta.acento, 0.8f));
        break;

    case TEMA_TERRITORIO_VOLCAN:
    default:
        DrawCylinder({ 0.0f, y, 0.0f }, 0.75f, 1.7f, 2.4f, 16, Color{ 74, 56, 52, 255 });
        DrawCylinder({ 0.0f, y + 2.35f, 0.0f }, 0.62f, 0.62f, 0.12f, 16, Color{ 255, 120, 20, 255 });
        for (int k = 0; k < 3; k++)
        {
            float fase = std::fmod(t * 0.9f + (float)k * 0.33f, 1.0f);
            DrawSphere(
                { std::sin((float)k * 2.1f) * 0.25f, y + 2.6f + fase * 1.4f, std::cos((float)k * 2.1f) * 0.25f },
                0.22f * (1.0f - fase * 0.5f),
                Fade(Color{ 255, 150, 40, 255 }, 1.0f - fase)
            );
        }
        break;
    }
}


// Muros del borde, dibujados por tramos para poder decorarlos por tema.
// MODELO FUTURO: reemplazar por un .glb modular de muro/valla por tema.
static void DibujarMurosVisual(
    const MinijuegoTerritorioConquista& minijuego,
    const PaletaTerritorio& paleta,
    float t
)
{
    const int tramos = 11;

    for (int lado = 1; lado <= 4; lado++)
    {
        const BloquePrueba& muro = minijuego.bloques[lado];
        bool largoEnZ = muro.tamano.z > muro.tamano.x;
        float largo = largoEnZ ? muro.tamano.z : muro.tamano.x;
        float tramo = largo / (float)tramos;

        for (int k = 0; k < tramos; k++)
        {
            float desplazamiento = -largo * 0.5f + (k + 0.5f) * tramo;
            Vector3 centro = muro.posicion;

            if (largoEnZ) centro.z += desplazamiento;
            else centro.x += desplazamiento;

            Vector3 tam = muro.tamano;

            if (largoEnZ) tam.z = tramo - 0.02f;
            else tam.x = tramo - 0.02f;

            Color color = paleta.muro;
            bool alterna = (k + lado) % 2 == 0;

            if (
                minijuego.tema == TEMA_TERRITORIO_DULCE ||
                minijuego.tema == TEMA_TERRITORIO_TOXICO
            )
            {
                color = alterna ? paleta.muro : paleta.muroAcento;
            }
            else if (minijuego.tema == TEMA_TERRITORIO_JARDIN)
            {
                color = alterna ? paleta.muro : Color{ 38, 112, 50, 255 };
            }
            else if (minijuego.tema == TEMA_TERRITORIO_VOLCAN)
            {
                color = alterna ? paleta.muro : Color{ 74, 58, 54, 255 };
            }

            DrawCubeV(centro, tam, color);

            Vector3 cima = { centro.x, muro.posicion.y + muro.tamano.y * 0.5f, centro.z };

            switch (minijuego.tema)
            {
            case TEMA_TERRITORIO_CYBER:
                DrawCubeV({ cima.x, cima.y + 0.03f, cima.z },
                    { largoEnZ ? 0.14f : tam.x * 0.9f, 0.08f, largoEnZ ? tam.z * 0.9f : 0.14f },
                    Fade(paleta.muroAcento, 0.75f + 0.25f * std::sin(t * 4.0f + (float)k)));
                break;
            case TEMA_TERRITORIO_DULCE:
                if (alterna)
                    DrawSphere({ cima.x, cima.y + 0.12f, cima.z }, 0.28f, Color{ 255, 255, 255, 255 });
                break;
            case TEMA_TERRITORIO_JARDIN:
                DrawSphere({ cima.x, cima.y + 0.1f, cima.z }, 0.2f,
                    k % 3 == 0 ? Color{ 255, 110, 160, 255 }
                    : k % 3 == 1 ? Color{ 255, 220, 80, 255 }
                                 : Color{ 255, 255, 255, 255 });
                break;
            case TEMA_TERRITORIO_VOLCAN:
                DrawSphere({ cima.x, cima.y + 0.05f, cima.z }, 0.22f + 0.1f * PseudoAleatorioTerritorio(k + lado * 17), Color{ 50, 38, 36, 255 });
                break;
            case TEMA_TERRITORIO_TOXICO:
                if (k % 4 == 0)
                    DrawSphere({ cima.x, cima.y + 0.15f, cima.z }, 0.16f,
                        Fade(paleta.acento, 0.6f + 0.4f * std::sin(t * 5.0f + (float)k)));
                break;
            default:
                break;
            }
        }
    }
}


// Decoracion fuera de la arena y fondo lejano.
// MODELO FUTURO: reemplazar por escenarios .glb (volcanes lejanos, torres
// de neon, colinas de caramelo, setas gigantes, tuberias y barriles).
static void DibujarFondoVisual(
    TemaTerritorio tema,
    const PaletaTerritorio& paleta,
    float t
)
{
    const float yExterior = -0.9f;

    DrawPlane({ 0.0f, yExterior, 0.0f }, { 90.0f, 90.0f }, paleta.exterior);

    switch (tema)
    {
    case TEMA_TERRITORIO_CYBER:
        for (int k = -20; k <= 20; k += 2)
        {
            DrawLine3D({ (float)k, yExterior + 0.02f, -30.0f }, { (float)k, yExterior + 0.02f, 30.0f }, Fade(paleta.muroAcento, 0.35f));
            DrawLine3D({ -30.0f, yExterior + 0.02f, (float)k }, { 30.0f, yExterior + 0.02f, (float)k }, Fade(paleta.muroAcento, 0.35f));
        }
        for (int k = 0; k < 14; k++)
        {
            float angulo = (float)k * 0.62f;
            float radio = 17.0f + 5.0f * PseudoAleatorioTerritorio(k);
            float alto = 4.0f + 9.0f * PseudoAleatorioTerritorio(k + 40);
            Vector3 pos = { std::cos(angulo) * radio, yExterior + alto * 0.5f, -std::fabs(std::sin(angulo)) * radio - 6.0f };
            DrawCube(pos, 2.2f, alto, 2.2f, Color{ 18, 22, 48, 255 });
            DrawCubeWires(pos, 2.2f, alto, 2.2f, k % 2 == 0 ? paleta.muroAcento : paleta.acento);
        }
        for (int k = 0; k < 8; k++)
        {
            Vector3 pos = { -11.0f + 3.2f * (float)k, 2.5f + std::sin(t * 1.5f + (float)k) * 0.6f, 9.5f + (float)(k % 3) };
            DrawCubeWires(pos, 0.7f, 0.7f, 0.7f, Fade(paleta.acento, 0.8f));
        }
        break;

    case TEMA_TERRITORIO_DULCE:
        for (int k = 0; k < 9; k++)
        {
            float x = -18.0f + 4.4f * (float)k;
            float z = -14.0f - 6.0f * PseudoAleatorioTerritorio(k);
            DrawSphere({ x, yExterior + 1.0f, z }, 4.2f + 2.0f * PseudoAleatorioTerritorio(k + 9), k % 2 == 0 ? Color{ 255, 190, 220, 255 } : Color{ 255, 232, 190, 255 });
        }
        for (int k = 0; k < 10; k++)
        {
            float lado = k % 2 == 0 ? -1.0f : 1.0f;
            float x = lado * (9.5f + 2.5f * PseudoAleatorioTerritorio(k + 3));
            float z = -9.0f + 2.2f * (float)(k / 2) * 1.6f;
            DrawCylinder({ x, yExterior, z }, 0.1f, 0.1f, 2.6f, 8, Color{ 255, 255, 255, 255 });
            DrawSphere({ x, yExterior + 2.8f, z }, 0.9f, k % 3 == 0 ? Color{ 255, 90, 140, 255 } : k % 3 == 1 ? Color{ 120, 200, 255, 255 } : Color{ 255, 220, 90, 255 });
        }
        break;

    case TEMA_TERRITORIO_JARDIN:
        DrawSphere({ 16.0f, 16.0f, -26.0f }, 3.0f, Color{ 255, 232, 90, 255 });
        for (int k = 0; k < 8; k++)
        {
            float lado = k % 2 == 0 ? -1.0f : 1.0f;
            float x = lado * (9.5f + 3.0f * PseudoAleatorioTerritorio(k + 5));
            float z = -12.0f + 3.4f * (float)(k / 2);
            float alto = 2.2f + 1.6f * PseudoAleatorioTerritorio(k + 21);
            DrawCylinder({ x, yExterior, z }, 0.35f, 0.45f, alto, 10, Color{ 240, 232, 214, 255 });
            DrawSphere({ x, yExterior + alto, z }, 1.5f, k % 2 == 0 ? Color{ 220, 50, 50, 255 } : Color{ 230, 140, 40, 255 });
            DrawSphere({ x + 0.5f, yExterior + alto + 0.9f, z + 0.5f }, 0.22f, WHITE);
        }
        for (int k = 0; k < 12; k++)
        {
            float x = -16.0f + 2.9f * (float)k;
            float z = 11.0f + 2.0f * PseudoAleatorioTerritorio(k + 60);
            DrawCylinder({ x, yExterior, z }, 0.06f, 0.06f, 1.4f, 6, Color{ 40, 130, 50, 255 });
            DrawSphere({ x, yExterior + 1.5f, z }, 0.32f, k % 2 == 0 ? Color{ 255, 120, 170, 255 } : Color{ 255, 220, 80, 255 });
        }
        break;

    case TEMA_TERRITORIO_TOXICO:
        for (int k = 0; k < 10; k++)
        {
            float x = -17.0f + 3.8f * (float)k;
            float z = -13.0f - 5.0f * PseudoAleatorioTerritorio(k + 2);
            DrawCylinder({ x, yExterior, z }, 0.9f, 0.9f, 3.0f + 5.0f * PseudoAleatorioTerritorio(k + 30), 10, Color{ 70, 76, 66, 255 });
        }
        for (int k = 0; k < 7; k++)
        {
            float x = -16.0f + 5.2f * (float)k;
            DrawCylinderEx({ x, yExterior + 1.2f, 12.5f }, { x + 3.0f, yExterior + 1.2f, 12.5f }, 0.4f, 0.4f, 8, Color{ 98, 100, 92, 255 });
        }
        for (int k = 0; k < 12; k++)
        {
            float fase = std::fmod(t * 0.35f + PseudoAleatorioTerritorio(k + 80), 1.0f);
            DrawSphere(
                { -14.0f + 2.6f * (float)k, yExterior + 0.5f + fase * 5.0f, 10.0f + 4.0f * PseudoAleatorioTerritorio(k + 55) },
                0.7f + fase,
                Fade(paleta.acento, 0.35f * (1.0f - fase))
            );
        }
        break;

    case TEMA_TERRITORIO_VOLCAN:
    default:
        for (int k = 0; k < 3; k++)
        {
            float x = -17.0f + 17.0f * (float)k;
            float z = -22.0f - 3.0f * (float)(k % 2);
            DrawCylinder({ x, yExterior, z }, 2.0f, 9.0f, 12.0f, 18, Color{ 62, 40, 38, 255 });
            DrawCylinder({ x, yExterior + 11.9f, z }, 2.0f, 2.0f, 0.2f, 18, Color{ 255, 110, 20, 255 });
            for (int h = 0; h < 4; h++)
            {
                float fase = std::fmod(t * 0.25f + (float)h * 0.25f + (float)k * 0.1f, 1.0f);
                DrawSphere({ x + std::sin((float)h * 2.0f) * 1.2f, yExterior + 12.5f + fase * 7.0f, z }, 1.2f + fase * 1.8f, Fade(Color{ 90, 80, 78, 255 }, 0.55f * (1.0f - fase)));
            }
        }
        for (int k = 0; k < 14; k++)
        {
            float angulo = (float)k * 0.9f;
            float radio = 9.5f + 4.5f * PseudoAleatorioTerritorio(k + 14);
            DrawSphere({ std::cos(angulo) * radio, yExterior + 0.2f, std::sin(angulo) * radio * 0.9f }, 0.5f + 0.5f * PseudoAleatorioTerritorio(k + 77), Color{ 52, 40, 38, 255 });
        }
        break;
    }
}


static Color ColorBaldosaVisualTerritorio(
    const MinijuegoTerritorioConquista& minijuego,
    const PaletaTerritorio& paleta,
    int columna,
    int fila
)
{
    const BaldosaTerritorio& baldosa =
        minijuego.baldosas[IndiceBaldosaTerritorio(columna, fila)];
    bool alterna = (columna + fila) % 2 == 0;

    Color color = alterna ? paleta.baldosaA : paleta.baldosaB;

    if (baldosa.dueno >= 0)
    {
        Color propio = minijuego.coloresJugadores[baldosa.dueno];
        color = MezclarColorTerritorio(
            propio,
            alterna ? WHITE : BLACK,
            alterna ? 0.06f : 0.10f
        );
    }

    if (BaldosaSelladaTerritorio(minijuego, columna, fila))
    {
        color = MezclarColorTerritorio(color, paleta.sellado, 0.72f);
    }
    else if (BaldosaEnSectorActivoTerritorio(minijuego, columna, fila))
    {
        bool parpadeo = std::fmod(minijuego.tiempoAnimacion, 0.3f) < 0.15f;

        if (parpadeo)
        {
            color = MezclarColorTerritorio(color, paleta.sellado, 0.72f);
        }
    }

    return color;
}


// Suelo cuadriculado: es la parte que comunica el estado del juego.
static void DibujarBaldosasVisual(
    const MinijuegoTerritorioConquista& minijuego,
    const PaletaTerritorio& paleta
)
{
    const float lado = TAMANO_BALDOSA_TERRITORIO - 0.08f;

    // Base oscura visible en las juntas entre baldosas.
    DrawCube(
        { 0.0f, SUELO_TERRITORIO - 0.5f, 0.0f },
        MITAD_ARENA_TERRITORIO * 2.0f,
        1.0f,
        MITAD_ARENA_TERRITORIO * 2.0f,
        paleta.rejilla
    );

    for (int fila = 0; fila < FILAS_TERRITORIO; fila++)
    {
        for (int columna = 0; columna < COLUMNAS_TERRITORIO; columna++)
        {
            const BaldosaTerritorio& baldosa =
                minijuego.baldosas[IndiceBaldosaTerritorio(columna, fila)];

            if (baldosa.obstaculo)
            {
                continue;
            }

            Vector3 centro = CentroBaldosaTerritorio(columna, fila);
            bool sellada = BaldosaSelladaTerritorio(minijuego, columna, fila);
            float elevacion = baldosa.destello * 0.12f - (sellada ? 0.03f : 0.0f);

            DrawCube(
                { centro.x, -0.05f + elevacion, centro.z },
                lado,
                0.10f,
                lado,
                ColorBaldosaVisualTerritorio(minijuego, paleta, columna, fila)
            );

            // Progreso de captura al caminar: cuadro interior que crece.
            if (baldosa.candidato >= 0 && baldosa.progreso > 0.0f)
            {
                float tamano = lado * LimitarTerritorio(baldosa.progreso, 0.0f, 1.0f);
                DrawCube(
                    { centro.x, 0.015f, centro.z },
                    tamano,
                    0.05f,
                    tamano,
                    minijuego.coloresJugadores[baldosa.candidato]
                );
            }

            if (sellada)
            {
                float m = lado * 0.5f - 0.05f;
                DrawLine3D({ centro.x - m, 0.03f, centro.z - m }, { centro.x + m, 0.03f, centro.z + m }, paleta.muroAcento);
                DrawLine3D({ centro.x - m, 0.03f, centro.z + m }, { centro.x + m, 0.03f, centro.z - m }, paleta.muroAcento);
            }
        }
    }
}


// Escenario completo. Para cambiar la representacion (modelos .glb) basta con
// editar esta funcion y sus ayudantes: el gameplay no depende de ella.
// MODELO FUTURO: arena completa (suelo con baldosas, muros, obstaculo central
// y fondo) como un unico escenario .glb por tema; las baldosas podrian seguir
// dibujandose por codigo para reflejar el dominio de cada jugador.
static void DibujarEscenarioVisualTerritorio(
    const MinijuegoTerritorioConquista& minijuego
)
{
    PaletaTerritorio paleta = ObtenerPaletaTerritorio(minijuego.tema);
    float t = minijuego.tiempoAnimacion;

    // Lava y toxico "respiran" suavemente en el exterior.
    if (minijuego.tema == TEMA_TERRITORIO_VOLCAN)
    {
        paleta.exterior = MezclarColorTerritorio(
            paleta.exterior,
            Color{ 255, 150, 30, 255 },
            0.5f + 0.5f * std::sin(t * 1.6f)
        );
    }

    DibujarFondoVisual(minijuego.tema, paleta, t);
    DibujarBaldosasVisual(minijuego, paleta);
    DibujarMurosVisual(minijuego, paleta, t);
    DibujarObstaculoCentralVisual(minijuego.tema, paleta, t);
}


//==================================================
// DIBUJO
//==================================================

static const char* NombreJugadorTerritorio(
    const Participante& participante,
    int indice
)
{
    return TextFormat(
        "J%d%s",
        participante.numeroJugador > 0 ? participante.numeroJugador : indice + 1,
        participante.esBot ? " BOT" : ""
    );
}


void MinijuegoTerritorioConquista::Dibujar(
    const JugadorPrueba jugadores[],
    int cantidadMaxima,
    const Participante participantes[],
    bool mostrarDebug
) const
{
    int limite = cantidadMaxima < MAX_PARTICIPANTES
        ? cantidadMaxima
        : MAX_PARTICIPANTES;

    PaletaTerritorio paleta = ObtenerPaletaTerritorio(tema);

    ClearBackground(paleta.fondo);
    BeginMode3D(camara);

    DibujarEscenarioVisualTerritorio(*this);

    for (int i = 0; i < MAX_ONDAS_TERRITORIO; i++)
    {
        if (!ondas[i].activa)
        {
            continue;
        }

        float progreso = ondas[i].tiempo / 0.45f;
        DrawCircle3D(
            ondas[i].posicion,
            0.4f + progreso * RADIO_RECLAMO_POUND,
            { 1.0f, 0.0f, 0.0f },
            90.0f,
            Fade(ondas[i].color, 1.0f - progreso)
        );
    }

    for (int i = 0; i < limite; i++)
    {
        if (!resultado.participantes[i].participo)
        {
            continue;
        }

        const JugadorPrueba& jugador = jugadores[i];

        // Aro de color bajo los pies para reconocer a cada jugador.
        DrawCircle3D(
            { jugador.posicion.x, 0.03f, jugador.posicion.z },
            0.62f,
            { 1.0f, 0.0f, 0.0f },
            90.0f,
            coloresJugadores[i]
        );
        DrawCircle3D(
            { jugador.posicion.x, 0.03f, jugador.posicion.z },
            0.52f,
            { 1.0f, 0.0f, 0.0f },
            90.0f,
            WHITE
        );

        // Zona que reclamara el ground pound en curso.
        if (jugador.preparandoGolpeSuelo || jugador.golpeSueloActivo)
        {
            DrawCircle3D(
                { jugador.posicion.x, 0.04f, jugador.posicion.z },
                RADIO_RECLAMO_POUND,
                { 1.0f, 0.0f, 0.0f },
                90.0f,
                Fade(coloresJugadores[i], 0.9f)
            );
        }

        Participante visual = participantes[i];
        visual.conectado = true;
        visual.color = coloresJugadores[i];
        DibujarJugadorCuboPrueba(jugador, visual);

        if (mostrarDebug)
        {
            DrawBoundingBox(CrearHitboxJugadorPrueba(jugador), LIME);
        }
    }

    DibujarParticulasTierra(particulas, MAX_PARTICULAS_TERRITORIO);

    if (mostrarDebug)
    {
        for (int i = 0; i < cantidadBloques; i++)
        {
            DrawBoundingBox(CrearHitboxBloquePrueba(bloques[i]), YELLOW);
        }
    }

    EndMode3D();

    // Etiquetas flotantes sobre los jugadores. Si dos etiquetas caen en el
    // mismo lugar de la pantalla, la posterior se apila hacia arriba.
    int etiquetaX[MAX_PARTICIPANTES]{};
    int etiquetaY[MAX_PARTICIPANTES]{};
    int etiquetaAncho[MAX_PARTICIPANTES]{};
    int etiquetasColocadas = 0;

    for (int i = 0; i < limite; i++)
    {
        if (!resultado.participantes[i].participo)
        {
            continue;
        }

        Vector2 pantalla = GetWorldToScreen(
            { jugadores[i].posicion.x, jugadores[i].posicion.y + 1.35f, jugadores[i].posicion.z },
            camara
        );
        const char* nombre = NombreJugadorTerritorio(participantes[i], i);
        int ancho = MeasureText(nombre, 18);
        int x = (int)pantalla.x;
        int y = (int)pantalla.y;

        for (int intento = 0; intento < MAX_PARTICIPANTES; intento++)
        {
            bool solapa = false;

            for (int j = 0; j < etiquetasColocadas; j++)
            {
                if (
                    std::abs(x - etiquetaX[j]) < (ancho + etiquetaAncho[j]) / 2 + 8 &&
                    std::abs(y - etiquetaY[j]) < 24
                )
                {
                    solapa = true;
                    y = etiquetaY[j] - 25;
                    break;
                }
            }

            if (!solapa) break;
        }

        etiquetaX[etiquetasColocadas] = x;
        etiquetaY[etiquetasColocadas] = y;
        etiquetaAncho[etiquetasColocadas] = ancho;
        etiquetasColocadas++;

        DrawRectangle(x - ancho / 2 - 4, y - 2, ancho + 8, 22, Fade(BLACK, 0.55f));
        DrawText(nombre, x - ancho / 2, y, 18, coloresJugadores[i]);
    }

    int anchoPantalla = GetScreenWidth();
    int altoPantalla = GetScreenHeight();

    // Cabecera.
    const char* textoAyuda =
        "CAMINA PARA MARCAR BALDOSAS. SALTA Y PULSA SALTO EN EL AIRE: GROUND POUND";
    int anchoCabecera = MeasureText(textoAyuda, 16) + 28;
    if (anchoCabecera < 640) anchoCabecera = 640;

    DrawRectangle(14, 12, anchoCabecera, 92, Fade(BLACK, 0.74f));
    DrawText("TERRITORIO EN CONQUISTA", 28, 20, 28, GOLD);
    DrawText(TextFormat("ESCENARIO: %s", paleta.nombreTema), 28, 52, 18, paleta.acento);
    DrawText(textoAyuda, 28, 76, 16, RAYWHITE);

    if (fase == FASE_TERRITORIO_JUGANDO)
    {
        const char* textoTiempo = TextFormat("TIEMPO %.1f", tiempoRestante);
        DrawRectangle(anchoPantalla - 214, 12, 200, 44, Fade(BLACK, 0.74f));
        DrawText(textoTiempo, anchoPantalla - 200, 22, 26, tiempoRestante <= 6.0f ? RED : GOLD);

        if (estadoSector == SECTOR_TERRITORIO_AVISO)
        {
            const char* aviso = TextFormat("ALERTA: %s EN %.1f", paleta.nombreSector, tiempoSector);
            int a = MeasureText(aviso, 26);
            DrawRectangle(anchoPantalla / 2 - a / 2 - 14, 112, a + 28, 40, Fade(BLACK, 0.8f));
            DrawText(aviso, anchoPantalla / 2 - a / 2, 120, 26, paleta.acento);
        }
        else if (estadoSector == SECTOR_TERRITORIO_SELLADO)
        {
            const char* aviso = TextFormat("SECTOR SELLADO %.1f", tiempoSector);
            int a = MeasureText(aviso, 22);
            DrawRectangle(anchoPantalla / 2 - a / 2 - 14, 112, a + 28, 36, Fade(BLACK, 0.7f));
            DrawText(aviso, anchoPantalla / 2 - a / 2, 119, 22, RAYWHITE);
        }
    }

    // Marcador inferior: porcentaje de territorio por jugador.
    int activos = 0;

    for (int i = 0; i < limite; i++)
    {
        if (resultado.participantes[i].participo) activos++;
    }

    if (activos > 0)
    {
        int separacion = 10;
        int anchoTarjeta = (anchoPantalla - 28 - separacion * (activos - 1)) / activos;

        if (anchoTarjeta > 300) anchoTarjeta = 300;

        int xInicial = (anchoPantalla - (anchoTarjeta * activos + separacion * (activos - 1))) / 2;
        int k = 0;

        for (int i = 0; i < limite; i++)
        {
            if (!resultado.participantes[i].participo)
            {
                continue;
            }

            int x = xInicial + k * (anchoTarjeta + separacion);
            int y = altoPantalla - 92;
            float porcentaje = PorcentajeTerritorio(*this, i);

            DrawRectangle(x, y, anchoTarjeta, 80, Fade(BLACK, 0.78f));
            DrawRectangle(x, y, 8, 80, coloresJugadores[i]);
            DrawText(NombreJugadorTerritorio(participantes[i], i), x + 18, y + 8, 20, coloresJugadores[i]);
            DrawText(TextFormat("%.0f%%", porcentaje), x + anchoTarjeta - 66, y + 8, 22, RAYWHITE);
            DrawRectangle(x + 18, y + 36, anchoTarjeta - 32, 10, Fade(WHITE, 0.2f));
            DrawRectangle(
                x + 18,
                y + 36,
                (int)((float)(anchoTarjeta - 32) * LimitarTerritorio(porcentaje / 100.0f, 0.0f, 1.0f)),
                10,
                coloresJugadores[i]
            );

            if (participantes[i].esBot)
            {
                DrawText("POUND: AUTO", x + 18, y + 54, 16, LIGHTGRAY);
            }
            else if (estadosJugadores[i].recargaPound > 0.0f)
            {
                DrawText("POUND: RECARGANDO", x + 18, y + 54, 16, ORANGE);
            }
            else
            {
                DrawText(
                    TextFormat("POUND LISTO  (SALTO: %s)", ObtenerTextoBotonPrincipal(participantes[i])),
                    x + 18,
                    y + 54,
                    16,
                    LIME
                );
            }

            k++;
        }
    }

    if (fase == FASE_TERRITORIO_PREPARACION)
    {
        int numero = (int)std::ceil(tiempoPreparacion);

        if (numero < 1) numero = 1;

        const char* texto = TextFormat("%d", numero);
        DrawText(
            texto,
            anchoPantalla / 2 - MeasureText(texto, 96) / 2,
            altoPantalla / 2 - 60,
            96,
            GOLD
        );
    }
    else if (
        fase == FASE_TERRITORIO_TERMINADO &&
        resultado.estado == RESULTADO_MINIJUEGO_FINALIZADO
    )
    {
        int panelAncho = 560;
        int panelAlto = 130 + 30 * activos;
        int px = anchoPantalla / 2 - panelAncho / 2;
        int py = altoPantalla / 2 - panelAlto / 2 - 20;

        DrawRectangle(px, py, panelAncho, panelAlto, Fade(BLACK, 0.9f));

        int ganadores[MAX_PARTICIPANTES]{};
        int cantidadGanadores = ObtenerIndicesGanadores(resultado, ganadores, MAX_PARTICIPANTES);
        const char* titulo = "EMPATE EN EL TERRITORIO";

        if (resultado.desenlace == DESENLACE_CON_GANADOR && cantidadGanadores == 1)
        {
            titulo = TextFormat("GANA %s", NombreJugadorTerritorio(participantes[ganadores[0]], ganadores[0]));
        }

        DrawText(titulo, anchoPantalla / 2 - MeasureText(titulo, 32) / 2, py + 14, 32, GOLD);

        int fila = 0;

        for (int posicion = 1; posicion <= MAX_PARTICIPANTES; posicion++)
        {
            for (int i = 0; i < limite; i++)
            {
                if (
                    !resultado.participantes[i].participo ||
                    resultado.participantes[i].posicionFinal != posicion
                )
                {
                    continue;
                }

                DrawText(
                    TextFormat(
                        "%d.  %s   %d baldosas  (%.0f%%)",
                        posicion,
                        NombreJugadorTerritorio(participantes[i], i),
                        estadosJugadores[i].baldosas,
                        PorcentajeTerritorio(*this, i)
                    ),
                    px + 40,
                    py + 62 + fila * 30,
                    22,
                    coloresJugadores[i]
                );
                fila++;
            }
        }

        DrawText(
            TextoReinicioMinijuego(),
            anchoPantalla / 2 - MeasureText(TextoReinicioMinijuego(), 18) / 2,
            py + panelAlto - 30,
            18,
            RAYWHITE
        );
    }
}


const ResultadoMinijuego& MinijuegoTerritorioConquista::ObtenerResultado() const
{
    return resultado;
}
