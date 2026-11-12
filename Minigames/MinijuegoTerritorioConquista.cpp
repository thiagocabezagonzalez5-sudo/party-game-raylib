#include "Minigames/MinijuegoTerritorioConquista.h"

#include "Minigames/AudioMinijuegos.h"
#include "Minigames/EfectosVisualesMinijuegos.h"
#include "Minigames/HudMinijuego.h"
#include "Minigames/MecanicasJugador.h"
#include "Minigames/ModeloJugadorCompartido.h"
#include "Minigames/UtilidadesMinijuegos.h"
#include "Systems/CalidadGrafica.h"
#include "Systems/Input.h"
#include "UI/EstiloUI.h"

#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>


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

// Estado solo visual (definidos con el dibujo, al final del archivo).
static void ReiniciarVisualTerritorio(MinijuegoTerritorioConquista& m);
static void ActualizarVisualTerritorio(
    MinijuegoTerritorioConquista& m,
    float deltaTime,
    const JugadorPrueba jugadores[],
    int cantidadMaxima,
    const Participante participantes[]
);


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

    // Pasteleria: la torta-arena vista en diagonal, con la ventana de la
    // cocina asomando arriba. Solo visual (el movimiento va por ejes del mundo).
    camara.position = { 0.0f, 15.2f, 17.4f };
    camara.target = { 0.0f, -0.8f, -0.6f };
    camara.up = { 0.0f, 1.0f, 0.0f };
    camara.fovy = 40.0f;
    camara.projection = CAMERA_PERSPECTIVE;

    fase = FASE_TERRITORIO_PREPARACION;
    estadoSector = SECTOR_TERRITORIO_LIBRE;
    sectorActual = -1;
    ultimoSector = -1;
    tiempoSector = TIEMPO_PRIMER_SELLADO;
    tiempoPreparacion = DURACION_PREPARACION_TERRITORIO;
    tiempoRestante = DURACION_PARTIDA_TERRITORIO;
    tiempoAnimacion = 0.0f;

    ReiniciarVisualTerritorio(*this);
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

    ActualizarVisualTerritorio(*this, deltaTime, jugadores, cantidadMaxima, participantes);
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
// VISUAL: PASTELERIA GIGANTE
//==================================================
//
// La arena es la cara de arriba de una torta cuadrada rodeada de barritas
// de chocolate (el muro real) con un mono lila, sobre un mantel a cuadros.
// Cada baldosa es un cuadro de glaseado: crema sin dueno, del color del
// jugador cuando la reclama. El sector sellado se cubre de azucar glas que
// cae de un colador gigante. El obstaculo central es una torta de capas con
// ganache que gotea y una vela. Alrededor: macarons, cupcakes, donas,
// palote, manga, bol con batidor, frasco de chispitas volcado; atras, torta
// de tres pisos, batidora, campana de vidrio, frascos, estantes, horno
// encendido y la ventana de la cocina con el jardin al atardecer.
//==================================================

static const float Y_MESA_TER = -3.8f;
static const float BORDE_TER = MITAD_ARENA_TERRITORIO + GROSOR_MURO_TERRITORIO;
static const float TOPE_MURO_TER = SUELO_TERRITORIO + ALTO_MURO_TERRITORIO;
static const float MITAD_OBSTACULO_TER = TAMANO_BALDOSA_TERRITORIO;
static const float Z_PARED_TER = -12.6f;
static const float X_VENTANA_TER = 11.0f;
static const float Y_ALFEIZAR_TER = Y_MESA_TER + 0.6f;
static const float Y_DINTEL_TER = 26.0f;
static const float SALIDA_COLADOR_TER = 1.1f;   // segundos que tamiza antes de irse

// Ojo de referencia para la niebla (la camara definitiva).
static const Vector3 OJO_TER = { 0.0f, 15.2f, 17.4f };

static const Color COLOR_PENUMBRA_TER = { 128, 88, 76, 255 };
static const Color COLOR_CALIMA_TER = { 255, 214, 176, 255 };
static const Color GLASEADO_A_TER = { 250, 238, 214, 255 };
static const Color GLASEADO_B_TER = { 240, 222, 192, 255 };
static const Color BIZCOCHO_TER = { 212, 156, 94, 255 };
static const Color MIGA_TER = { 176, 118, 70, 255 };
static const Color CHOCOLATE_TER = { 96, 56, 36, 255 };
static const Color CHOCOLATE_CLARO_TER = { 136, 84, 52, 255 };
static const Color GANACHE_TER = { 72, 40, 28, 255 };
static const Color CREMA_TER = { 255, 246, 232, 255 };
static const Color CINTA_TER = { 178, 146, 214, 255 };
static const Color MANTEL_TER = { 206, 128, 134, 255 };
static const Color MANTEL_MEDIO_TER = { 230, 186, 180, 255 };
static const Color MANTEL_CLARO_TER = { 252, 240, 226, 255 };
static const Color MADERA_TER = { 168, 116, 76, 255 };
static const Color AZUCAR_TER = { 255, 253, 248, 255 };
static const Color METAL_TER = { 196, 200, 208, 255 };


//==================================================
// VISUAL: UTILIDADES
//==================================================

static float Limitar01Ter(float v)
{
    return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
}


static unsigned char CanalTer(float v)
{
    if (v < 0.0f) return 0;
    if (v > 255.0f) return 255;
    return (unsigned char)v;
}


static Color TonoTer(Color c, float f)
{
    return Color{ CanalTer((float)c.r * f), CanalTer((float)c.g * f), CanalTer((float)c.b * f), c.a };
}


static Color MezclaTer(Color a, Color b, float t)
{
    t = Limitar01Ter(t);
    return Color{
        CanalTer((float)a.r + ((float)b.r - (float)a.r) * t),
        CanalTer((float)a.g + ((float)b.g - (float)a.g) * t),
        CanalTer((float)a.b + ((float)b.b - (float)a.b) * t),
        CanalTer((float)a.a + ((float)b.a - (float)a.a) * t)
    };
}


static Color AlfaTer(Color c, float alfa)
{
    c.a = CanalTer(alfa);
    return c;
}


// Hash determinista en [0, 1) para decorar sin parpadeos.
static float Hash01Ter(int a, int b)
{
    unsigned int h = (unsigned int)a * 374761393u + (unsigned int)b * 668265263u + 2246822519u;
    h = (h ^ (h >> 13)) * 1274126177u;
    h ^= h >> 16;
    return (float)(h & 0xFFFFFFu) / 16777216.0f;
}


// Azar propio del dibujo: nunca consume el GetRandomValue de la logica.
static float AzarTer(MinijuegoTerritorioConquista& m, float minimo, float maximo)
{
    m.semillaVisual = m.semillaVisual * 1664525u + 1013904223u;
    float u = (float)((m.semillaVisual >> 8) & 0xFFFFFFu) / 16777216.0f;
    return minimo + (maximo - minimo) * u;
}


static float DistanciaOjoTer(Vector3 p)
{
    float dx = p.x - OJO_TER.x;
    float dy = p.y - OJO_TER.y;
    float dz = p.z - OJO_TER.z;
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}


// Cocina en penumbra calida: lo lejano de la mesa se apaga un poco.
static Color TinteTer(Color c, Vector3 p)
{
    float n = (DistanciaOjoTer(p) - 20.0f) / 40.0f;
    Color niebla = COLOR_PENUMBRA_TER;
    niebla.a = c.a;
    return MezclaTer(c, niebla, n < 0.0f ? 0.0f : (n > 0.30f ? 0.30f : n));
}


// Afuera: el jardin y las sierras se aclaran hacia la calima.
static Color TinteLejosTer(Color c, Vector3 p)
{
    float n = (DistanciaOjoTer(p) - 30.0f) / 40.0f;
    Color calima = COLOR_CALIMA_TER;
    calima.a = c.a;
    return MezclaTer(c, calima, n < 0.0f ? 0.0f : (n > 0.78f ? 0.78f : n));
}


// Luz falsa por normal: sol de la ventana arriba-atras-izquierda y relleno
// calido de las lamparas (arriba 1.0, frente ~0.78, izq ~0.70, der ~0.62).
static float LuzNormalTer(Vector3 n)
{
    float d = n.x * -0.25f + n.y * 0.86f + n.z * 0.45f;
    float f = 0.66f + 0.34f * d;
    return f < 0.48f ? 0.48f : f;
}


static float LuzLateralTer(float angulo)
{
    return LuzNormalTer({ std::cos(angulo), 0.0f, std::sin(angulo) });
}


static void VerticeTer(Vector3 p, Color c)
{
    rlColor4ub(c.r, c.g, c.b, c.a);
    rlVertex3f(p.x, p.y, p.z);
}


static void QuadTer(Vector3 a, Vector3 b, Vector3 c, Vector3 d, Color color)
{
    VerticeTer(a, color);
    VerticeTer(b, color);
    VerticeTer(c, color);
    VerticeTer(d, color);
}


static void QuadNieblaTer(Vector3 a, Vector3 b, Vector3 c, Vector3 d, Color color)
{
    VerticeTer(a, TinteTer(color, a));
    VerticeTer(b, TinteTer(color, b));
    VerticeTer(c, TinteTer(color, c));
    VerticeTer(d, TinteTer(color, d));
}


// Rectangulo horizontal (centro, medios lados) a la altura c.y.
static void LosaTer(Vector3 c, float mx, float mz, Color color)
{
    rlBegin(RL_QUADS);
    QuadTer(
        { c.x - mx, c.y, c.z - mz },
        { c.x - mx, c.y, c.z + mz },
        { c.x + mx, c.y, c.z + mz },
        { c.x + mx, c.y, c.z - mz },
        color
    );
    rlEnd();
}


// Caja sobre tres ejes (medios lados ya escalados), caras sombreadas.
static void CajaEjesTer(Vector3 c, Vector3 ex, Vector3 ey, Vector3 ez, Color color)
{
    Vector3 v[8];

    for (int k = 0; k < 8; k++)
    {
        float sx = (k & 1) ? 1.0f : -1.0f;
        float sy = (k & 2) ? 1.0f : -1.0f;
        float sz = (k & 4) ? 1.0f : -1.0f;
        v[k] =
        {
            c.x + ex.x * sx + ey.x * sy + ez.x * sz,
            c.y + ex.y * sx + ey.y * sy + ez.y * sz,
            c.z + ex.z * sx + ey.z * sy + ez.z * sz
        };
    }

    rlBegin(RL_QUADS);
    QuadTer(v[6], v[7], v[3], v[2], TonoTer(color, 1.0f));     // arriba
    QuadTer(v[4], v[5], v[7], v[6], TonoTer(color, 0.80f));    // frente
    QuadTer(v[1], v[0], v[2], v[3], TonoTer(color, 0.56f));    // fondo
    QuadTer(v[5], v[1], v[3], v[7], TonoTer(color, 0.64f));    // derecha
    QuadTer(v[0], v[4], v[6], v[2], TonoTer(color, 0.72f));    // izquierda
    QuadTer(v[0], v[1], v[5], v[4], TonoTer(color, 0.46f));    // abajo
    rlEnd();
}


static void CuboTer(Vector3 c, Vector3 t, Color color)
{
    CajaEjesTer(c, { t.x * 0.5f, 0.0f, 0.0f }, { 0.0f, t.y * 0.5f, 0.0f }, { 0.0f, 0.0f, t.z * 0.5f }, color);
}


static void CuboNieblaTer(Vector3 c, Vector3 t, Color color)
{
    CuboTer(c, t, TinteTer(color, c));
}


static void CuboGiradoTer(Vector3 c, Vector3 t, float giroGrados, Color color)
{
    float a = giroGrados * DEG2RAD;
    float co = std::cos(a);
    float si = std::sin(a);

    CajaEjesTer(
        c,
        { co * t.x * 0.5f, 0.0f, -si * t.x * 0.5f },
        { 0.0f, t.y * 0.5f, 0.0f },
        { si * t.z * 0.5f, 0.0f, co * t.z * 0.5f },
        color
    );
}


// Viga de seccion cuadrada entre dos puntos (mangos, alambres, patas).
static void VigaTer(Vector3 a, Vector3 b, float grosor, Color color)
{
    Vector3 eje = Vector3Subtract(b, a);
    float largo = Vector3Length(eje);

    if (largo < 0.0001f)
    {
        return;
    }

    Vector3 dir = Vector3Scale(eje, 1.0f / largo);
    Vector3 c = Vector3Scale(Vector3Add(a, b), 0.5f);
    float g = grosor * 0.5f;
    Vector3 ayuda = std::fabs(dir.y) > 0.7f ? Vector3{ 0.0f, 0.0f, 1.0f } : Vector3{ 0.0f, 1.0f, 0.0f };
    Vector3 ex = Vector3Normalize(Vector3CrossProduct(ayuda, dir));
    Vector3 ey = Vector3CrossProduct(dir, ex);

    if (std::fabs(dir.y) > 0.7f)
    {
        CajaEjesTer(c, Vector3Scale(ex, g), Vector3Scale(dir, largo * 0.5f * (dir.y < 0.0f ? -1.0f : 1.0f)), Vector3Scale(ey, g), color);
    }
    else
    {
        if (ey.y < 0.0f) ey = Vector3Negate(ey);
        CajaEjesTer(c, Vector3Scale(dir, largo * 0.5f), Vector3Scale(ey, g), Vector3Scale(ex, g), color);
    }
}


// Tronco de cono vertical (r0 abajo, r1 arriba) con tapa arriba.
static void PrismaTer(Vector3 base, float r0, float r1, float alto, int lados, Color color, bool tapa = true)
{
    float yArriba = base.y + alto;
    float paso = 6.2832f / (float)lados;
    Color tapaArriba = TonoTer(color, 1.0f);

    rlBegin(RL_QUADS);

    for (int k = 0; k < lados; k++)
    {
        float a0 = paso * (float)k;
        float a1 = a0 + paso;
        Color cara = TonoTer(color, LuzLateralTer(a0 + paso * 0.5f));
        float c0 = std::cos(a0), s0 = std::sin(a0);
        float c1 = std::cos(a1), s1 = std::sin(a1);

        QuadTer(
            { base.x + c0 * r0, base.y, base.z + s0 * r0 },
            { base.x + c0 * r1, yArriba, base.z + s0 * r1 },
            { base.x + c1 * r1, yArriba, base.z + s1 * r1 },
            { base.x + c1 * r0, base.y, base.z + s1 * r0 },
            cara
        );

        if (tapa && r1 > 0.001f)
        {
            Vector3 centro = { base.x, yArriba, base.z };
            QuadTer(centro, { base.x + c1 * r1, yArriba, base.z + s1 * r1 }, { base.x + c0 * r1, yArriba, base.z + s0 * r1 }, centro, tapaArriba);
        }
    }

    rlEnd();
}


// Capacillo plisado: las caras alternan luz para marcar los pliegues.
static void PlisadoTer(Vector3 base, float r0, float r1, float alto, int lados, Color color)
{
    float paso = 6.2832f / (float)lados;

    rlBegin(RL_QUADS);

    for (int k = 0; k < lados; k++)
    {
        float a0 = paso * (float)k;
        float a1 = a0 + paso;
        float pliegue = (k % 2 == 0) ? 1.0f : 0.84f;
        Color cara = TonoTer(color, LuzLateralTer(a0 + paso * 0.5f) * pliegue);
        float c0 = std::cos(a0), s0 = std::sin(a0);
        float c1 = std::cos(a1), s1 = std::sin(a1);

        QuadTer(
            { base.x + c0 * r0, base.y, base.z + s0 * r0 },
            { base.x + c0 * r1, base.y + alto, base.z + s0 * r1 },
            { base.x + c1 * r1, base.y + alto, base.z + s1 * r1 },
            { base.x + c1 * r0, base.y, base.z + s1 * r0 },
            cara
        );
    }

    rlEnd();
}


// Corona plana (r0..r1) a la altura c.y.
static void AnilloTer(Vector3 c, float r0, float r1, Color color, int lados = 28)
{
    rlBegin(RL_QUADS);

    for (int k = 0; k < lados; k++)
    {
        float a0 = 6.2832f * (float)k / (float)lados;
        float a1 = 6.2832f * (float)(k + 1) / (float)lados;

        QuadTer(
            { c.x + std::cos(a0) * r1, c.y, c.z + std::sin(a0) * r1 },
            { c.x + std::cos(a0) * r0, c.y, c.z + std::sin(a0) * r0 },
            { c.x + std::cos(a1) * r0, c.y, c.z + std::sin(a1) * r0 },
            { c.x + std::cos(a1) * r1, c.y, c.z + std::sin(a1) * r1 },
            color
        );
    }

    rlEnd();
}


// Media cupula elipsoidal sobre c (macarons, crema, bizcochos).
static void DomoTer(Vector3 c, float rx, float ry, float rz, Color color, int lados = 14, int anillos = 4)
{
    rlBegin(RL_QUADS);

    for (int a = 0; a < anillos; a++)
    {
        float f0 = 1.5708f * (float)a / (float)anillos;
        float f1 = 1.5708f * (float)(a + 1) / (float)anillos;

        for (int k = 0; k < lados; k++)
        {
            float t0 = 6.2832f * (float)k / (float)lados;
            float t1 = 6.2832f * (float)(k + 1) / (float)lados;
            float fm = (f0 + f1) * 0.5f;
            float tm = (t0 + t1) * 0.5f;
            Color cara = TonoTer(color, LuzNormalTer({ std::cos(fm) * std::cos(tm), std::sin(fm), std::cos(fm) * std::sin(tm) }));

            QuadTer(
                { c.x + rx * std::cos(f0) * std::cos(t0), c.y + ry * std::sin(f0), c.z + rz * std::cos(f0) * std::sin(t0) },
                { c.x + rx * std::cos(f1) * std::cos(t0), c.y + ry * std::sin(f1), c.z + rz * std::cos(f1) * std::sin(t0) },
                { c.x + rx * std::cos(f1) * std::cos(t1), c.y + ry * std::sin(f1), c.z + rz * std::cos(f1) * std::sin(t1) },
                { c.x + rx * std::cos(f0) * std::cos(t1), c.y + ry * std::sin(f0), c.z + rz * std::cos(f0) * std::sin(t1) },
                cara
            );
        }
    }

    rlEnd();
}


// Rosca acostada (donas, crema en espiral). La mitad de arriba puede llevar
// glaseado con borde ondulado.
static void ToroTer(Vector3 c, float R, float r, Color masa, Color glaseado, bool conGlaseado, int semilla, int lados = 18, int anillos = 8)
{
    rlBegin(RL_QUADS);

    for (int k = 0; k < lados; k++)
    {
        float t0 = 6.2832f * (float)k / (float)lados;
        float t1 = 6.2832f * (float)(k + 1) / (float)lados;
        float tm = (t0 + t1) * 0.5f;
        float limite = -0.25f + 0.35f * std::sin(tm * 5.0f + (float)semilla);

        for (int a = 0; a < anillos; a++)
        {
            float f0 = 6.2832f * (float)a / (float)anillos;
            float f1 = 6.2832f * (float)(a + 1) / (float)anillos;
            float fm = (f0 + f1) * 0.5f;
            Vector3 n = { std::cos(fm) * std::cos(tm), std::sin(fm), std::cos(fm) * std::sin(tm) };
            Color base = (conGlaseado && std::sin(fm) > limite) ? glaseado : masa;
            Color cara = TonoTer(base, LuzNormalTer(n));

            QuadTer(
                { c.x + (R + r * std::cos(f0)) * std::cos(t0), c.y + r * std::sin(f0), c.z + (R + r * std::cos(f0)) * std::sin(t0) },
                { c.x + (R + r * std::cos(f0)) * std::cos(t1), c.y + r * std::sin(f0), c.z + (R + r * std::cos(f0)) * std::sin(t1) },
                { c.x + (R + r * std::cos(f1)) * std::cos(t1), c.y + r * std::sin(f1), c.z + (R + r * std::cos(f1)) * std::sin(t1) },
                { c.x + (R + r * std::cos(f1)) * std::cos(t0), c.y + r * std::sin(f1), c.z + (R + r * std::cos(f1)) * std::sin(t0) },
                cara
            );
        }
    }

    rlEnd();
}


// Disco con borde que se apaga (u y v definen el plano).
static void ResplandorTer(Vector3 centro, Vector3 u, Vector3 v, float radio, Color color, float estirado = 1.0f)
{
    const int LADOS = 18;
    rlBegin(RL_TRIANGLES);

    for (int k = 0; k < LADOS; k++)
    {
        float a0 = 6.2832f * (float)k / (float)LADOS;
        float a1 = 6.2832f * (float)(k + 1) / (float)LADOS;
        float c0 = std::cos(a0) * radio * estirado, s0 = std::sin(a0) * radio;
        float c1 = std::cos(a1) * radio * estirado, s1 = std::sin(a1) * radio;

        rlColor4ub(color.r, color.g, color.b, color.a);
        rlVertex3f(centro.x, centro.y, centro.z);
        rlColor4ub(color.r, color.g, color.b, 0);
        rlVertex3f(centro.x + u.x * c0 + v.x * s0, centro.y + u.y * c0 + v.y * s0, centro.z + u.z * c0 + v.z * s0);
        rlColor4ub(color.r, color.g, color.b, 0);
        rlVertex3f(centro.x + u.x * c1 + v.x * s1, centro.y + u.y * c1 + v.y * s1, centro.z + u.z * c1 + v.z * s1);
    }

    rlEnd();
}


static void ResplandorSueloTer(Vector3 c, float radio, Color color, float estirado = 1.0f)
{
    ResplandorTer(c, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, radio, color, estirado);
}


// Halo que mira a la camara.
static void ResplandorFrenteTer(Vector3 c, float radio, Color color)
{
    ResplandorTer(c, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.68f, 0.73f }, radio, color);
}


static int CantidadParticulasTer(int base)
{
    int n = (int)((float)base * FactorCalidadGrafica(CalidadParticulas()) + 0.5f);
    return n < 1 ? 1 : n;
}


//==================================================
// VISUAL: ESTADO Y EFECTOS (una vez por frame desde Actualizar)
//==================================================

static void EmitirParticulaTer(
    MinijuegoTerritorioConquista& m,
    Vector3 posicion,
    Vector3 velocidad,
    Color color,
    float vida,
    float tamano,
    float gravedad,
    bool brillo
)
{
    for (int i = 0; i < MAX_PARTICULAS_VISUALES_TERRITORIO; i++)
    {
        ParticulaVisualTerritorio& p = m.particulasVisuales[i];

        if (p.activa)
        {
            continue;
        }

        p.activa = true;
        p.brillo = brillo;
        p.posicion = posicion;
        p.velocidad = velocidad;
        p.color = color;
        p.vida = vida;
        p.vidaMaxima = vida;
        p.tamano = tamano;
        p.gravedad = gravedad;
        p.giro = AzarTer(m, 0.0f, 180.0f);
        return;
    }
}


// Colores de las chispitas de azucar (nunca los de los jugadores puros).
static Color ColorChispitaTer(int k)
{
    const Color chispas[5] =
    {
        Color{ 255, 250, 240, 255 },
        Color{ 255, 182, 204, 255 },
        Color{ 176, 220, 255, 255 },
        Color{ 255, 226, 150, 255 },
        Color{ 196, 236, 186, 255 }
    };

    return chispas[(k % 5 + 5) % 5];
}


static void ReiniciarVisualTerritorio(MinijuegoTerritorioConquista& m)
{
    m.semillaVisual = 2246822519u;

    for (int i = 0; i < MAX_BALDOSAS_TERRITORIO; i++)
    {
        m.duenoVisto[i] = -1;
    }

    for (int i = 0; i < MAX_ONDAS_TERRITORIO; i++)
    {
        m.ondaVista[i] = false;
    }

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        m.aturdidoVisto[i] = false;
        m.saltoMarcador[i] = 0.0f;
    }

    for (int i = 0; i < MAX_PARTICULAS_VISUALES_TERRITORIO; i++)
    {
        m.particulasVisuales[i] = {};
    }

    m.esperaSonidoBaldosa = 0.0f;
    m.sectorVisto = SECTOR_TERRITORIO_LIBRE;
    m.sectorAnunciado = -1;
    m.tiempoAnuncio = -1.0f;
    m.tiempoFin = 0.0f;
    m.finVisto = false;
}


// Centro y medio lado del cuadrante de un sector (0..3).
static Vector3 CentroSectorTer(int sector)
{
    float mitad = MITAD_ARENA_TERRITORIO * 0.5f;
    return Vector3{
        (sector % 2 == 0) ? -mitad : mitad,
        0.0f,
        (sector < 2) ? -mitad : mitad
    };
}


static void ActualizarVisualTerritorio(
    MinijuegoTerritorioConquista& m,
    float deltaTime,
    const JugadorPrueba jugadores[],
    int cantidadMaxima,
    const Participante participantes[]
)
{
    int limite = cantidadMaxima < MAX_PARTICIPANTES ? cantidadMaxima : MAX_PARTICIPANTES;

    // Particulas: caen, se posan sobre la torta o la mesa y se apagan.
    for (int i = 0; i < MAX_PARTICULAS_VISUALES_TERRITORIO; i++)
    {
        ParticulaVisualTerritorio& p = m.particulasVisuales[i];

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

        p.velocidad.y -= p.gravedad * deltaTime;
        p.posicion = Vector3Add(p.posicion, Vector3Scale(p.velocidad, deltaTime));
        p.giro += deltaTime * (p.brillo ? 0.0f : 240.0f) * (p.velocidad.y < -0.2f ? 1.0f : 0.0f);

        bool sobreTorta = std::fabs(p.posicion.x) < BORDE_TER && std::fabs(p.posicion.z) < BORDE_TER;
        float piso = sobreTorta ? 0.03f : Y_MESA_TER + 0.03f;

        if (p.posicion.y < piso && p.gravedad > 0.0f)
        {
            p.posicion.y = piso;
            p.velocidad = { 0.0f, 0.0f, 0.0f };

            if (p.vida > 0.5f)
            {
                p.vida = 0.5f;
            }
        }
    }

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        m.saltoMarcador[i] -= deltaTime * 3.0f;
        if (m.saltoMarcador[i] < 0.0f) m.saltoMarcador[i] = 0.0f;
    }

    if (m.esperaSonidoBaldosa > 0.0f)
    {
        m.esperaSonidoBaldosa -= deltaTime;
    }

    // Baldosas que cambiaron de dueno: chispitas del color del jugador.
    int cambios = 0;

    for (int i = 0; i < MAX_BALDOSAS_TERRITORIO; i++)
    {
        if (m.baldosas[i].dueno != m.duenoVisto[i])
        {
            cambios++;
        }
    }

    for (int i = 0; i < MAX_BALDOSAS_TERRITORIO; i++)
    {
        int dueno = m.baldosas[i].dueno;

        if (dueno == m.duenoVisto[i])
        {
            continue;
        }

        bool robo = m.duenoVisto[i] >= 0;
        m.duenoVisto[i] = dueno;

        if (dueno < 0 || dueno >= MAX_PARTICIPANTES)
        {
            continue;
        }

        Vector3 c = CentroBaldosaTerritorio(i % COLUMNAS_TERRITORIO, i / COLUMNAS_TERRITORIO);
        Color color = m.coloresJugadores[dueno];
        int chispas = CantidadParticulasTer(cambios > 6 ? 1 : (robo ? 6 : 4));

        for (int k = 0; k < chispas; k++)
        {
            float angulo = AzarTer(m, 0.0f, 6.2832f);
            float rapidez = AzarTer(m, 0.8f, 2.2f);
            Color chispa = (k % 2 == 0) ? MezclaTer(color, WHITE, 0.25f) : ColorChispitaTer((int)AzarTer(m, 0.0f, 5.0f));
            EmitirParticulaTer(
                m,
                { c.x + AzarTer(m, -0.4f, 0.4f), 0.12f, c.z + AzarTer(m, -0.4f, 0.4f) },
                { std::cos(angulo) * rapidez, AzarTer(m, 2.4f, 4.0f), std::sin(angulo) * rapidez },
                chispa,
                AzarTer(m, 0.9f, 1.4f),
                AzarTer(m, 0.05f, 0.075f),
                12.0f,
                false
            );
        }

        EmitirParticulaTer(m, { c.x, 0.2f, c.z }, { 0.0f, 0.6f, 0.0f }, color, 0.35f, 0.5f, 0.0f, true);
        m.saltoMarcador[dueno] = 1.0f;

        if (
            cambios <= 6 &&
            m.esperaSonidoBaldosa <= 0.0f &&
            dueno < limite &&
            !participantes[dueno].esBot &&
            participantes[dueno].conectado
        )
        {
            ReproducirSonidoMinijuego(m.audio, robo ? SONIDO_ACIERTO : SONIDO_RECOGER_OBJETO);
            m.esperaSonidoBaldosa = 0.14f;
        }
    }

    // Ground pound: anillo de chispitas y migas que saltan.
    for (int i = 0; i < MAX_ONDAS_TERRITORIO; i++)
    {
        const OndaTerritorio& onda = m.ondas[i];

        if (onda.activa && !m.ondaVista[i])
        {
            int cantidad = CantidadParticulasTer(26);

            for (int k = 0; k < cantidad; k++)
            {
                float angulo = 6.2832f * (float)k / (float)cantidad + AzarTer(m, -0.1f, 0.1f);
                float rapidez = AzarTer(m, 2.6f, 4.6f);
                bool miga = k % 3 == 0;
                EmitirParticulaTer(
                    m,
                    { onda.posicion.x + std::cos(angulo) * 0.4f, 0.15f, onda.posicion.z + std::sin(angulo) * 0.4f },
                    { std::cos(angulo) * rapidez, AzarTer(m, 2.8f, 5.0f), std::sin(angulo) * rapidez },
                    miga ? MIGA_TER : (k % 3 == 1 ? MezclaTer(onda.color, WHITE, 0.2f) : ColorChispitaTer(k)),
                    AzarTer(m, 0.9f, 1.5f),
                    miga ? AzarTer(m, 0.08f, 0.12f) : AzarTer(m, 0.05f, 0.08f),
                    13.0f,
                    false
                );
            }

            ReproducirSonidoMinijuego(m.audio, SONIDO_GROUND_POUND);
        }

        m.ondaVista[i] = onda.activa;
    }

    // Aturdidos por el pound de un rival: estrellitas grises y golpe.
    for (int i = 0; i < limite; i++)
    {
        if (!m.resultado.participantes[i].participo)
        {
            m.aturdidoVisto[i] = false;
            continue;
        }

        bool aturdido = jugadores[i].aplastado;

        if (aturdido && !m.aturdidoVisto[i])
        {
            Vector3 p = jugadores[i].posicion;

            for (int k = 0; k < CantidadParticulasTer(8); k++)
            {
                float angulo = AzarTer(m, 0.0f, 6.2832f);
                EmitirParticulaTer(
                    m,
                    { p.x, p.y + 0.5f, p.z },
                    { std::cos(angulo) * 1.6f, AzarTer(m, 1.5f, 3.0f), std::sin(angulo) * 1.6f },
                    Color{ 214, 206, 196, 255 },
                    0.8f,
                    0.08f,
                    9.0f,
                    false
                );
            }

            ReproducirSonidoMinijuego(m.audio, SONIDO_GOLPE);
        }

        m.aturdidoVisto[i] = aturdido;
    }

    // Sector: aviso (baja el colador) y sellado (cae azucar glas).
    if (m.estadoSector != m.sectorVisto)
    {
        if (m.estadoSector == SECTOR_TERRITORIO_AVISO)
        {
            m.sectorAnunciado = m.sectorActual;
            ReproducirSonidoMinijuego(m.audio, SONIDO_PLATAFORMA);
        }
        else if (m.estadoSector == SECTOR_TERRITORIO_SELLADO)
        {
            ReproducirSonidoMinijuego(m.audio, SONIDO_IMPACTO);
        }

        m.sectorVisto = m.estadoSector;
        m.tiempoAnuncio = 0.0f;
    }
    else if (m.tiempoAnuncio >= 0.0f)
    {
        m.tiempoAnuncio += deltaTime;
    }

    if (m.estadoSector == SECTOR_TERRITORIO_SELLADO && m.sectorActual >= 0)
    {
        Vector3 c = CentroSectorTer(m.sectorActual);
        float mitad = MITAD_ARENA_TERRITORIO * 0.5f;
        float tasa = (m.tiempoAnuncio < SALIDA_COLADOR_TER ? 80.0f : 22.0f) * FactorCalidadGrafica(CalidadParticulas());
        int cantidad = (int)(tasa * deltaTime + AzarTer(m, 0.0f, 1.0f));

        for (int k = 0; k < cantidad; k++)
        {
            EmitirParticulaTer(
                m,
                { c.x + AzarTer(m, -mitad, mitad) * 0.8f, AzarTer(m, 1.4f, 3.8f), c.z + AzarTer(m, -mitad, mitad) * 0.8f },
                { AzarTer(m, -0.3f, 0.3f), AzarTer(m, -1.6f, -0.8f), AzarTer(m, -0.3f, 0.3f) },
                AZUCAR_TER,
                3.0f,
                AzarTer(m, 0.035f, 0.06f),
                1.2f,
                false
            );
        }
    }

    // Fin: confeti sobre el ganador (o los empatados).
    if (m.fase == FASE_TERRITORIO_TERMINADO)
    {
        m.tiempoFin += deltaTime;

        if (!m.finVisto && m.resultado.estado == RESULTADO_MINIJUEGO_FINALIZADO)
        {
            m.finVisto = true;

            for (int i = 0; i < limite; i++)
            {
                if (!m.resultado.participantes[i].participo || m.resultado.participantes[i].posicionFinal != 1)
                {
                    continue;
                }

                Vector3 p = jugadores[i].posicion;
                int cantidad = CantidadParticulasTer(46);

                for (int k = 0; k < cantidad; k++)
                {
                    float angulo = AzarTer(m, 0.0f, 6.2832f);
                    float rapidez = AzarTer(m, 0.6f, 2.6f);
                    EmitirParticulaTer(
                        m,
                        { p.x, p.y + 2.2f, p.z },
                        { std::cos(angulo) * rapidez, AzarTer(m, 3.5f, 6.5f), std::sin(angulo) * rapidez },
                        k % 3 == 0 ? m.coloresJugadores[i] : ColorChispitaTer(k),
                        AzarTer(m, 2.0f, 3.2f),
                        AzarTer(m, 0.08f, 0.12f),
                        5.0f,
                        false
                    );
                }
            }
        }
    }
}


//==================================================
// VISUAL: LO QUE SE VE POR LA VENTANA (jardin al atardecer)
//==================================================
//
// La camara mira hacia abajo: por la ventana se ve una franja del afuera.
// Cada plano se ubica por la pendiente del rayo desde el ojo (s = caida por
// unidad de distancia), asi el cielo queda arriba, las sierras al medio y
// el cerco abajo sin importar a que profundidad este cada uno.

static float YPendienteTer(float s, float z)
{
    return OJO_TER.y - s * (OJO_TER.z - z);
}


static void SiluetaTer(float x0, float x1, float yBase, float yTope0, float yTope1, float z, Color color)
{
    Vector3 a = { x0, yBase, z };
    Vector3 b = { x1, yBase, z };
    Vector3 c = { x1, yTope1, z };
    Vector3 d = { x0, yTope0, z };

    rlBegin(RL_QUADS);
    VerticeTer(a, TinteLejosTer(TonoTer(color, 0.86f), a));
    VerticeTer(b, TinteLejosTer(TonoTer(color, 0.86f), b));
    VerticeTer(c, TinteLejosTer(color, c));
    VerticeTer(d, TinteLejosTer(color, d));
    rlEnd();
}


static void NubeTer(Vector3 c, float ancho, Color color)
{
    for (int k = 0; k < 5; k++)
    {
        float dx = (-0.5f + 0.25f * (float)k) * ancho;
        float r = ancho * (0.16f + 0.10f * (k == 2 ? 1.0f : (k == 1 || k == 3 ? 0.6f : 0.0f)));
        ResplandorTer({ c.x + dx, c.y + r * 0.2f, c.z }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, r, color, 1.5f);
    }
}


static void DibujarExteriorTer(float t, bool detalle)
{
    const float zCielo = -54.0f;
    float yArriba = YPendienteTer(0.30f, zCielo);
    float yHorizonte = YPendienteTer(0.485f, zCielo);
    float yAbajo = YPendienteTer(0.70f, zCielo);
    const float xCielo = 60.0f;

    // Cielo: celeste lavanda arriba, durazno sobre las sierras.
    rlBegin(RL_QUADS);
    VerticeTer({ -xCielo, yArriba, zCielo }, Color{ 168, 186, 232, 255 });
    VerticeTer({ xCielo, yArriba, zCielo }, Color{ 168, 186, 232, 255 });
    VerticeTer({ xCielo, yHorizonte, zCielo }, Color{ 255, 206, 166, 255 });
    VerticeTer({ -xCielo, yHorizonte, zCielo }, Color{ 255, 206, 166, 255 });
    VerticeTer({ -xCielo, yHorizonte, zCielo }, Color{ 255, 206, 166, 255 });
    VerticeTer({ xCielo, yHorizonte, zCielo }, Color{ 255, 206, 166, 255 });
    VerticeTer({ xCielo, yAbajo, zCielo }, Color{ 250, 190, 150, 255 });
    VerticeTer({ -xCielo, yAbajo, zCielo }, Color{ 250, 190, 150, 255 });
    rlEnd();

    // Sol bajo y nubes alargadas que se desplazan.
    Vector3 sol = { -9.0f, YPendienteTer(0.452f, zCielo + 1.0f), zCielo + 1.0f };
    ResplandorTer(sol, { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, 2.2f, Color{ 255, 246, 214, 255 });
    ResplandorTer({ sol.x, sol.y, sol.z + 0.05f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, 1.5f, Color{ 255, 252, 236, 255 });

    if (detalle)
    {
        for (int k = 0; k < 6; k++)
        {
            float ancho = 7.0f + 5.0f * Hash01Ter(k, 3);
            float x = std::fmod(-40.0f + 16.0f * (float)k + t * (0.5f + 0.3f * Hash01Ter(k, 4)), 96.0f) - 44.0f;
            float s = 0.39f + 0.06f * Hash01Ter(k, 5);
            NubeTer({ x, YPendienteTer(s, zCielo + 2.0f), zCielo + 2.0f }, ancho, Color{ 255, 236, 226, 150 });
        }
    }

    // Sierras lejanas (lila desvaido) y lomas con un pueblito.
    const float zSierra = -48.0f;
    float baseSierra = YPendienteTer(0.70f, zSierra);

    for (int k = 0; k < 20; k++)
    {
        float x0 = -60.0f + 6.0f * (float)k;
        float s0 = 0.468f + 0.018f * Hash01Ter(k, 11);
        float s1 = 0.468f + 0.018f * Hash01Ter(k + 1, 11);
        SiluetaTer(x0, x0 + 6.0f, baseSierra, YPendienteTer(s0, zSierra), YPendienteTer(s1, zSierra), zSierra, Color{ 170, 150, 176, 255 });
    }

    const float zLoma = -40.0f;
    float baseLoma = YPendienteTer(0.70f, zLoma);

    for (int k = 0; k < 24; k++)
    {
        float x0 = -54.0f + 4.5f * (float)k;
        float s0 = 0.490f + 0.010f * std::sin((float)k * 0.9f) + 0.006f * Hash01Ter(k, 12);
        float s1 = 0.490f + 0.010f * std::sin((float)(k + 1) * 0.9f) + 0.006f * Hash01Ter(k + 1, 12);
        SiluetaTer(x0, x0 + 4.5f, baseLoma, YPendienteTer(s0, zLoma), YPendienteTer(s1, zLoma), zLoma, Color{ 150, 162, 104, 255 });
    }

    if (detalle)
    {
        for (int k = 0; k < 7; k++)
        {
            float x = -22.0f + 7.0f * (float)k + 2.0f * Hash01Ter(k, 13);
            float y = YPendienteTer(0.492f + 0.008f * Hash01Ter(k, 14), zLoma + 0.5f);
            float ancho = 1.4f + 0.8f * Hash01Ter(k, 15);
            Vector3 c = { x, y + 0.5f, zLoma + 0.5f };
            CuboTer(c, { ancho, 1.0f, 1.0f }, TinteLejosTer(Color{ 246, 232, 214, 255 }, c));
            Vector3 techo = { x, y + 1.15f, zLoma + 0.5f };
            CajaEjesTer(techo, { ancho * 0.6f, 0.0f, 0.0f }, { 0.0f, 0.18f, 0.0f }, { 0.0f, 0.0f, 0.6f }, TinteLejosTer(Color{ 196, 108, 82, 255 }, techo));
        }
    }

    // Arboles del jardin, cerco vivo y cerquito blanco.
    const float zArboles = -31.0f;

    for (int k = 0; k < 9; k++)
    {
        float x = -28.0f + 7.0f * (float)k + 2.5f * Hash01Ter(k, 16);
        float yTope = YPendienteTer(0.505f + 0.02f * Hash01Ter(k, 17), zArboles);
        float yBase = YPendienteTer(0.60f, zArboles);
        float radio = 1.8f + 0.8f * Hash01Ter(k, 18);
        Vector3 copa = { x, yTope - radio * 0.9f, zArboles };
        Color verde = TinteLejosTer(Color{ 92, 138, 78, 255 }, copa);
        VigaTer({ x, yBase, zArboles }, { x, copa.y, zArboles }, 0.4f, TinteLejosTer(Color{ 110, 80, 60, 255 }, copa));
        DomoTer(copa, radio, radio * 0.95f, radio * 0.8f, verde, 10, 3);
        DomoTer(copa, radio, -radio * 0.7f, radio * 0.8f, TonoTer(verde, 0.8f), 10, 2);
    }

    const float zSeto = -24.0f;
    float ySeto = YPendienteTer(0.538f, zSeto);
    Vector3 seto = { 0.0f, (ySeto + YPendienteTer(0.68f, zSeto)) * 0.5f, zSeto };
    CuboTer(seto, { 70.0f, ySeto - YPendienteTer(0.68f, zSeto), 1.6f }, TinteLejosTer(Color{ 74, 116, 66, 255 }, seto));

    for (int k = 0; k < 24 && detalle; k++)
    {
        Vector3 c = { -34.0f + 3.0f * (float)k, ySeto - 0.1f, zSeto + 0.3f };
        DomoTer(c, 1.6f, 0.6f + 0.4f * Hash01Ter(k, 19), 0.9f, TinteLejosTer(Color{ 82, 128, 72, 255 }, c), 8, 2);
    }

    const float zCerco = -19.5f;
    float yCerco = YPendienteTer(0.556f, zCerco);
    float yCercoBase = YPendienteTer(0.66f, zCerco);
    Color blanco = TinteLejosTer(Color{ 244, 240, 232, 255 }, { 0.0f, yCerco, zCerco });

    for (int k = 0; k < 40; k++)
    {
        float x = -26.0f + 1.3f * (float)k;
        CuboTer({ x, (yCerco + yCercoBase) * 0.5f, zCerco }, { 0.5f, yCerco - yCercoBase, 0.18f }, blanco);
    }

    CuboTer({ 0.0f, yCerco - 0.8f, zCerco + 0.15f }, { 54.0f, 0.3f, 0.12f }, TonoTer(blanco, 0.92f));
}


//==================================================
// VISUAL: PARED DE LA COCINA, VENTANA, ESTANTES Y HORNO
//==================================================

static void FrascoTer(Vector3 base, float radio, float alto, Color contenido, Color tapa)
{
    PrismaTer(base, radio * 0.94f, radio * 0.94f, alto * 0.72f, 12, TinteTer(contenido, base));
    PrismaTer({ base.x, base.y + alto * 0.72f, base.z }, radio, radio, alto * 0.2f, 12, TinteTer(Color{ 214, 228, 232, 255 }, base));
    PrismaTer({ base.x, base.y + alto * 0.92f, base.z }, radio * 1.04f, radio * 1.04f, alto * 0.12f, 12, TinteTer(tapa, base));
}


static void DibujarParedTer(float t, bool detalle)
{
    const float z = Z_PARED_TER;
    const float yTope = 14.0f;
    const float xMax = 48.0f;

    // Azulejos tipo subte crema con junta, desfasados por fila.
    Color junta = Color{ 188, 170, 150, 255 };
    rlBegin(RL_QUADS);
    QuadNieblaTer({ -xMax, Y_MESA_TER, z }, { -X_VENTANA_TER - 0.9f, Y_MESA_TER, z }, { -X_VENTANA_TER - 0.9f, yTope, z }, { -xMax, yTope, z }, junta);
    QuadNieblaTer({ X_VENTANA_TER + 0.9f, Y_MESA_TER, z }, { xMax, Y_MESA_TER, z }, { xMax, yTope, z }, { X_VENTANA_TER + 0.9f, yTope, z }, junta);
    QuadNieblaTer({ -X_VENTANA_TER - 0.9f, Y_MESA_TER, z }, { X_VENTANA_TER + 0.9f, Y_MESA_TER, z }, { X_VENTANA_TER + 0.9f, Y_ALFEIZAR_TER - 0.4f, z }, { -X_VENTANA_TER - 0.9f, Y_ALFEIZAR_TER - 0.4f, z }, junta);
    rlEnd();

    const float ancho = 2.2f;
    const float alto = 1.1f;
    int filas = (int)((yTope - Y_MESA_TER) / alto) + 1;
    int paso = detalle ? 1 : 2;

    rlBegin(RL_QUADS);

    for (int f = 0; f < filas; f++)
    {
        float y0 = Y_MESA_TER + (float)f * alto + 0.05f;
        float y1 = y0 + alto - 0.1f;
        float desfase = (f % 2 == 0) ? 0.0f : ancho * 0.5f;

        for (int c = 0; c * ancho < xMax * 2.0f + ancho; c += paso)
        {
            float x0 = -xMax - desfase + (float)c * ancho + 0.05f;
            float x1 = x0 + ancho * (float)paso - 0.1f;

            if (x1 > -X_VENTANA_TER - 1.0f && x0 < X_VENTANA_TER + 1.0f && y1 > Y_ALFEIZAR_TER - 0.5f)
            {
                continue;
            }

            float variacion = 0.94f + 0.08f * Hash01Ter(f, c);
            Color azulejo = TonoTer(Color{ 244, 236, 218, 255 }, variacion);
            QuadNieblaTer({ x0, y0, z + 0.02f }, { x1, y0, z + 0.02f }, { x1, y1, z + 0.02f }, { x0, y1, z + 0.02f }, azulejo);
            // Brillo esmaltado en el borde de arriba.
            QuadNieblaTer({ x0, y1 - 0.14f, z + 0.03f }, { x1, y1 - 0.14f, z + 0.03f }, { x1, y1, z + 0.03f }, { x0, y1, z + 0.03f }, TonoTer(azulejo, 1.05f));
        }
    }

    rlEnd();

    // Marco de la ventana: jambas, alfeizar, parteluz y travesano.
    Color marco = Color{ 250, 246, 238, 255 };
    CuboNieblaTer({ -X_VENTANA_TER - 0.45f, 6.0f, z + 0.3f }, { 0.9f, 20.0f, 0.6f }, marco);
    CuboNieblaTer({ X_VENTANA_TER + 0.45f, 6.0f, z + 0.3f }, { 0.9f, 20.0f, 0.6f }, marco);
    CuboNieblaTer({ 0.0f, Y_ALFEIZAR_TER - 0.2f, z + 0.7f }, { X_VENTANA_TER * 2.0f + 2.6f, 0.4f, 1.4f }, marco);
    CuboNieblaTer({ 0.0f, 6.0f, z - 0.1f }, { 0.36f, 20.0f, 0.3f }, marco);
    CuboNieblaTer({ 0.0f, 4.4f, z - 0.1f }, { X_VENTANA_TER * 2.0f, 0.36f, 0.3f }, marco);

    // Macetas con hierbas sobre el alfeizar.
    for (int k = 0; k < 3; k++)
    {
        float x = -7.5f + 7.5f * (float)k + (k == 1 ? 0.0f : 0.6f);
        Vector3 base = { x, Y_ALFEIZAR_TER, z + 0.75f };
        PrismaTer(base, 0.55f, 0.72f, 1.0f, 10, TinteTer(Color{ 200, 112, 78, 255 }, base));

        for (int h = 0; h < 6; h++)
        {
            float a = 6.2832f * (float)h / 6.0f + (float)k;
            float vaiven = std::sin(t * 1.3f + (float)h + (float)k * 2.0f) * 0.08f;
            Vector3 hoja = { x + std::cos(a) * 0.35f + vaiven, Y_ALFEIZAR_TER + 1.3f + 0.25f * Hash01Ter(k, h), z + 0.75f + std::sin(a) * 0.3f };
            DomoTer(hoja, 0.34f, 0.24f, 0.22f, TinteTer(Color{ 96, 160, 84, 255 }, hoja), 8, 2);
        }
    }

    if (!detalle)
    {
        return;
    }

    // Cortinas a cuadritos que ondean con la brisa.
    for (int lado = -1; lado <= 1; lado += 2)
    {
        const int COLUMNAS = 10;
        const int FILAS = 8;
        float xInterior = (float)lado * (X_VENTANA_TER - 1.2f);
        float xExterior = (float)lado * (X_VENTANA_TER + 2.6f);
        float yAbajo = Y_ALFEIZAR_TER + 1.2f;
        float yArriba = 12.0f;

        rlBegin(RL_QUADS);

        for (int c = 0; c < COLUMNAS; c++)
        {
            for (int f = 0; f < FILAS; f++)
            {
                float u0 = (float)c / (float)COLUMNAS;
                float u1 = (float)(c + 1) / (float)COLUMNAS;
                float v0 = (float)f / (float)FILAS;
                float v1 = (float)(f + 1) / (float)FILAS;
                auto punto = [&](float u, float v)
                {
                    float onda = std::sin(u * 18.0f + t * 1.6f + (float)lado) * 0.22f * (1.0f - v * 0.6f);
                    float x = xExterior + (xInterior - xExterior) * u * (0.86f + 0.14f * (1.0f - v));
                    return Vector3{ x + onda * 0.4f, yAbajo + (yArriba - yAbajo) * v, z + 1.0f + onda + 0.3f * std::sin(u * 9.0f) };
                };
                bool a = (c % 2) == 0;
                bool b = (f % 2) == 0;
                Color tela = (a && b) ? MANTEL_TER : ((a || b) ? MANTEL_MEDIO_TER : MANTEL_CLARO_TER);
                tela = TonoTer(tela, 0.82f + 0.18f * std::fabs(std::cos(u0 * 18.0f + t * 1.6f)));
                QuadNieblaTer(punto(u0, v0), punto(u1, v0), punto(u1, v1), punto(u0, v1), tela);
            }
        }

        rlEnd();

        // Abrazadera de la cortina.
        CuboNieblaTer({ (float)lado * (X_VENTANA_TER + 1.0f), yAbajo + 2.4f, z + 1.3f }, { 2.4f, 0.3f, 0.4f }, CINTA_TER);
    }

    // Estantes con frascos de galletitas, caramelos y fideos.
    for (int lado = -1; lado <= 1; lado += 2)
    {
        float x0 = (float)lado * 15.0f;
        float x1 = (float)lado * 36.0f;
        float y = 3.6f;
        Vector3 tabla = { (x0 + x1) * 0.5f, y, z + 0.9f };
        CuboNieblaTer(tabla, { std::fabs(x1 - x0), 0.3f, 1.8f }, MADERA_TER);

        for (int k = 0; k < 3; k++)
        {
            float x = x0 + (x1 - x0) * (0.12f + 0.38f * (float)k);
            CuboNieblaTer({ x, y - 0.6f, z + 0.5f }, { 0.25f, 1.0f, 1.0f }, TonoTer(MADERA_TER, 0.85f));
        }

        for (int k = 0; k < 7; k++)
        {
            float x = x0 + (x1 - x0) * (0.06f + 0.135f * (float)k);
            float alto = 1.5f + 0.9f * Hash01Ter(k, lado + 7);
            const Color contenidos[4] =
            {
                Color{ 214, 170, 112, 255 },
                Color{ 236, 176, 196, 255 },
                Color{ 246, 226, 168, 255 },
                Color{ 186, 214, 196, 255 }
            };
            FrascoTer({ x, y + 0.15f, z + 0.9f }, 0.55f, alto, contenidos[(k + (lado > 0 ? 1 : 0)) % 4], Color{ 196, 150, 104, 255 });
        }
    }

    // Horno de pared a la derecha: puerta con vidrio encendido.
    Vector3 horno = { 29.0f, 0.0f, z + 1.4f };
    CuboNieblaTer(horno, { 12.0f, 7.6f, 2.8f }, Color{ 226, 218, 204, 255 });
    CuboNieblaTer({ horno.x, horno.y - 0.6f, horno.z + 1.42f }, { 9.6f, 5.0f, 0.1f }, Color{ 60, 44, 40, 255 });
    float pulso = 0.9f + 0.1f * std::sin(t * 2.3f);
    CuboTer({ horno.x, horno.y - 0.6f, horno.z + 1.48f }, { 8.4f, 3.8f, 0.04f }, TonoTer(Color{ 255, 150, 70, 255 }, pulso));
    CuboNieblaTer({ horno.x, horno.y + 2.4f, horno.z + 1.6f }, { 8.0f, 0.3f, 0.3f }, METAL_TER);

    for (int k = 0; k < 4; k++)
    {
        CuboNieblaTer({ horno.x - 3.6f + 2.4f * (float)k, horno.y + 3.3f, horno.z + 1.45f }, { 0.7f, 0.7f, 0.2f }, Color{ 80, 72, 70, 255 });
    }
}


//==================================================
// VISUAL: MESA CON MANTEL A CUADROS, PUNTILLA Y BANDEJA
//==================================================

static void DibujarMantelTer(int paso)
{
    const float lado = 2.4f;
    const float xMax = 48.0f;
    const float zMin = Z_PARED_TER;
    const float zMax = 26.0f;
    int columnas = (int)(2.0f * xMax / lado);
    int filas = (int)((zMax - zMin) / lado);
    float y = Y_MESA_TER;

    rlBegin(RL_QUADS);

    for (int f = 0; f < filas; f++)
    {
        for (int c = 0; c < columnas; c += paso)
        {
            float x0 = -xMax + (float)c * lado;
            float x1 = x0 + lado * (float)paso;
            float z0 = zMin + (float)f * lado;
            float z1 = z0 + lado;
            bool a = (c % 2) == 0;
            bool b = (f % 2) == 0;
            Color tela = (a && b) ? MANTEL_TER : ((a || b) ? MANTEL_MEDIO_TER : MANTEL_CLARO_TER);
            tela = TonoTer(tela, 0.95f + 0.06f * Hash01Ter(c, f + 300));
            QuadNieblaTer({ x0, y, z0 }, { x0, y, z1 }, { x1, y, z1 }, { x1, y, z0 }, tela);
        }
    }

    rlEnd();

    // Arrugas suaves del mantel: franjas apenas mas oscuras.
    for (int k = 0; k < 6; k++)
    {
        float x = (k < 3 ? -1.0f : 1.0f) * (14.0f + 8.0f * Hash01Ter(k, 301));
        float z0 = -8.0f + 14.0f * Hash01Ter(k, 302);
        rlBegin(RL_QUADS);
        QuadNieblaTer({ x - 0.3f, y + 0.01f, z0 }, { x - 0.1f, y + 0.01f, z0 + 7.0f }, { x + 0.25f, y + 0.01f, z0 + 7.0f }, { x + 0.05f, y + 0.01f, z0 }, Color{ 150, 96, 96, 70 });
        rlEnd();
    }
}


// Puntilla de papel bajo la bandeja: festones y calado.
static void DibujarPuntillaTer()
{
    const int LADOS = 72;
    const float radio = 10.8f;
    float y = Y_MESA_TER + 0.015f;
    Color papel = Color{ 252, 248, 242, 255 };

    rlBegin(RL_TRIANGLES);

    for (int k = 0; k < LADOS; k++)
    {
        float a0 = 6.2832f * (float)k / (float)LADOS;
        float a1 = 6.2832f * (float)(k + 1) / (float)LADOS;
        float r0 = radio * (1.0f + 0.035f * std::fabs(std::cos(a0 * 12.0f)));
        float r1 = radio * (1.0f + 0.035f * std::fabs(std::cos(a1 * 12.0f)));
        VerticeTer({ 0.0f, y, 0.0f }, TinteTer(papel, { 0.0f, y, 0.0f }));
        VerticeTer({ std::cos(a1) * r1, y, std::sin(a1) * r1 }, TinteTer(papel, { std::cos(a1) * r1, y, std::sin(a1) * r1 }));
        VerticeTer({ std::cos(a0) * r0, y, std::sin(a0) * r0 }, TinteTer(papel, { std::cos(a0) * r0, y, std::sin(a0) * r0 }));
    }

    rlEnd();

    // Calado: agujeritos que dejan ver el mantel.
    for (int k = 0; k < 36; k++)
    {
        float a = 6.2832f * (float)k / 36.0f;
        Vector3 c = { std::cos(a) * (radio - 0.9f), y + 0.004f, std::sin(a) * (radio - 0.9f) };
        AnilloTer(c, 0.0f, 0.32f, TinteTer(Color{ 226, 196, 190, 255 }, c), 8);
    }

    AnilloTer({ 0.0f, y + 0.003f, 0.0f }, radio - 1.6f, radio - 1.45f, Color{ 232, 218, 206, 255 }, 64);
}


//==================================================
// VISUAL: LA TORTA-ARENA (barritas, mono, glaseado por baldosa)
//==================================================

// Barritas de chocolate alrededor: su cara interior coincide con el muro
// real. Se dibujan bajas (nadie salta tan alto) para no tapar la fila del
// frente con la camara en diagonal.
static void DibujarBarritasTer(bool detalle)
{
    float yBase = Y_MESA_TER + 0.17f;
    const int POR_LADO = 22;

    for (int lado = 0; lado < 4; lado++)
    {
        bool frenteFondo = lado < 2;
        float signo = (lado % 2 == 0) ? 1.0f : -1.0f;
        float largo = frenteFondo ? BORDE_TER * 2.0f : MITAD_ARENA_TERRITORIO * 2.0f;
        float ancho = largo / (float)POR_LADO;

        for (int k = 0; k < POR_LADO; k++)
        {
            float a = -largo * 0.5f + ancho * ((float)k + 0.5f);
            // Las del frente, mas bajas todavia: dejan entera la ultima fila.
            float tope = lado == 0
                ? 0.28f + 0.08f * Hash01Ter(k, lado + 40)
                : 0.62f + 0.14f * Hash01Ter(k, lado + 40);
            float alto = tope - yBase;
            float centroGrosor = signo * (MITAD_ARENA_TERRITORIO + GROSOR_MURO_TERRITORIO * 0.5f);
            Vector3 c = frenteFondo
                ? Vector3{ a, yBase + alto * 0.5f, centroGrosor }
                : Vector3{ centroGrosor, yBase + alto * 0.5f, a };
            Vector3 t = frenteFondo
                ? Vector3{ ancho - 0.05f, alto, GROSOR_MURO_TERRITORIO }
                : Vector3{ GROSOR_MURO_TERRITORIO, alto, ancho - 0.05f };
            Color choco = TonoTer(CHOCOLATE_TER, 0.94f + 0.1f * Hash01Ter(k, lado + 41));
            CuboNieblaTer(c, t, choco);

            // Tapa redondeada mas clara y la ranura del medio de cada barrita.
            Vector3 tapa = { c.x, tope + 0.04f, c.z };
            Vector3 tt = frenteFondo ? Vector3{ ancho - 0.16f, 0.08f, GROSOR_MURO_TERRITORIO - 0.1f } : Vector3{ GROSOR_MURO_TERRITORIO - 0.1f, 0.08f, ancho - 0.16f };
            CuboNieblaTer(tapa, tt, CHOCOLATE_CLARO_TER);

            if (detalle && lado == 0)
            {
                Vector3 ranura = { c.x, c.y, c.z + GROSOR_MURO_TERRITORIO * 0.5f + 0.005f };
                CuboTer(ranura, { 0.05f, alto - 0.3f, 0.01f }, TinteTer(TonoTer(CHOCOLATE_TER, 0.62f), ranura));
            }
        }
    }

    // Mono lila: banda alrededor del costado y el lazo al frente.
    float yCinta = -2.0f;
    float altoCinta = 1.0f;
    float b = BORDE_TER + 0.03f;
    CuboNieblaTer({ 0.0f, yCinta, b }, { b * 2.0f, altoCinta, 0.05f }, CINTA_TER);
    CuboNieblaTer({ 0.0f, yCinta, -b }, { b * 2.0f, altoCinta, 0.05f }, CINTA_TER);
    CuboNieblaTer({ b, yCinta, 0.0f }, { 0.05f, altoCinta, b * 2.0f }, CINTA_TER);
    CuboNieblaTer({ -b, yCinta, 0.0f }, { 0.05f, altoCinta, b * 2.0f }, CINTA_TER);
    CuboNieblaTer({ 0.0f, yCinta + altoCinta * 0.5f - 0.08f, b + 0.03f }, { b * 2.0f, 0.12f, 0.02f }, TonoTer(CINTA_TER, 1.12f));

    Vector3 nudo = { 0.0f, yCinta, b + 0.25f };
    CuboNieblaTer(nudo, { 0.7f, 0.9f, 0.4f }, TonoTer(CINTA_TER, 1.05f));

    for (int s = -1; s <= 1; s += 2)
    {
        Vector3 lazo = { (float)s * 1.1f, yCinta + 0.35f, b + 0.35f };
        CajaEjesTer(lazo, { (float)s * 0.9f, 0.32f, 0.0f }, { -(float)s * 0.18f, 0.5f, 0.0f }, { 0.0f, 0.0f, 0.18f }, CINTA_TER);
        Vector3 cola = { (float)s * 0.6f, yCinta - 0.9f, b + 0.3f };
        CajaEjesTer(cola, { 0.22f, 0.0f, 0.0f }, { (float)s * 0.3f, 0.7f, 0.0f }, { 0.0f, 0.0f, 0.05f }, TonoTer(CINTA_TER, 0.92f));
    }
}


static Color ColorBaldosaTer(const MinijuegoTerritorioConquista& m, int columna, int fila, float& hundir, float& azucar)
{
    const BaldosaTerritorio& baldosa = m.baldosas[IndiceBaldosaTerritorio(columna, fila)];
    bool alterna = (columna + fila) % 2 == 0;
    Color color = alterna ? GLASEADO_A_TER : GLASEADO_B_TER;
    color = TonoTer(color, 0.96f + 0.06f * Hash01Ter(columna, fila + 500));

    if (baldosa.dueno >= 0 && baldosa.dueno < MAX_PARTICIPANTES)
    {
        color = TonoTer(m.coloresJugadores[baldosa.dueno], alterna ? 1.04f : 0.92f);
    }

    hundir = 0.0f;
    azucar = 0.0f;

    if (BaldosaSelladaTerritorio(m, columna, fila))
    {
        hundir = 0.03f;
        azucar = 0.5f;
    }
    else if (BaldosaEnSectorActivoTerritorio(m, columna, fila))
    {
        azucar = std::fmod(m.tiempoAnimacion, 0.3f) < 0.15f ? 0.3f : 0.0f;
    }

    return MezclaTer(color, AZUCAR_TER, azucar);
}


// Cuadros de glaseado: es la parte que comunica el estado del juego.
static void DibujarBaldosasTer(const MinijuegoTerritorioConquista& m)
{
    const float lado = TAMANO_BALDOSA_TERRITORIO - 0.1f;

    // Cuerpo de la torta (se asoma entre las barritas) y bizcochuelo a la
    // vista en las juntas de los cuadros.
    float yBase = Y_MESA_TER + 0.17f;
    CuboTer({ 0.0f, (yBase - 0.5f) * 0.5f, 0.0f }, { MITAD_ARENA_TERRITORIO * 2.0f, -0.5f - yBase, MITAD_ARENA_TERRITORIO * 2.0f }, BIZCOCHO_TER);
    CuboTer({ 0.0f, -0.28f, 0.0f }, { MITAD_ARENA_TERRITORIO * 2.0f, 0.44f, MITAD_ARENA_TERRITORIO * 2.0f }, MIGA_TER);

    for (int fila = 0; fila < FILAS_TERRITORIO; fila++)
    {
        for (int columna = 0; columna < COLUMNAS_TERRITORIO; columna++)
        {
            int indice = IndiceBaldosaTerritorio(columna, fila);
            const BaldosaTerritorio& baldosa = m.baldosas[indice];

            if (baldosa.obstaculo)
            {
                continue;
            }

            Vector3 c = CentroBaldosaTerritorio(columna, fila);
            float hundir = 0.0f;
            float azucar = 0.0f;
            Color color = ColorBaldosaTer(m, columna, fila, hundir, azucar);
            bool propia = baldosa.dueno >= 0;
            float alto = propia ? 0.13f : 0.1f;
            float tope = -0.06f + alto + baldosa.destello * 0.14f - hundir;
            float escala = 1.0f + baldosa.destello * 0.06f;

            CuboTer({ c.x, tope - alto * 0.5f, c.z }, { lado * escala, alto, lado * escala }, color);

            // Bisel: el centro del cuadro un poco mas claro, como glaseado liso.
            LosaTer({ c.x, tope + 0.002f, c.z }, lado * 0.36f * escala, lado * 0.36f * escala, MezclaTer(color, WHITE, propia ? 0.12f : 0.18f));

            if (propia)
            {
                // Chispitas de azucar sobre el glaseado propio.
                for (int k = 0; k < 3; k++)
                {
                    float px = (Hash01Ter(indice, k * 3) - 0.5f) * lado * 0.7f;
                    float pz = (Hash01Ter(indice, k * 3 + 1) - 0.5f) * lado * 0.7f;
                    float giro = Hash01Ter(indice, k * 3 + 2) * 180.0f;
                    CuboGiradoTer({ c.x + px, tope + 0.02f, c.z + pz }, { 0.16f, 0.04f, 0.05f }, giro, MezclaTer(ColorChispitaTer(indice + k), color, 0.15f));
                }
            }

            if (azucar >= 0.5f)
            {
                // Montoncitos de azucar glas sobre el sector sellado.
                for (int k = 0; k < 2; k++)
                {
                    float px = (Hash01Ter(indice, k + 20) - 0.5f) * lado * 0.6f;
                    float pz = (Hash01Ter(indice, k + 22) - 0.5f) * lado * 0.6f;
                    DomoTer({ c.x + px, tope, c.z + pz }, 0.2f, 0.08f, 0.2f, AZUCAR_TER, 8, 2);
                }
            }

            // Captura al caminar: el glaseado del candidato se extiende.
            if (baldosa.candidato >= 0 && baldosa.candidato < MAX_PARTICIPANTES && baldosa.progreso > 0.0f)
            {
                float p = lado * Limitar01Ter(baldosa.progreso) * 0.5f;
                Color nuevo = m.coloresJugadores[baldosa.candidato];
                LosaTer({ c.x, tope + 0.012f, c.z }, p, p, nuevo);
                LosaTer({ c.x, tope + 0.014f, c.z }, p * 0.6f, p * 0.6f, MezclaTer(nuevo, WHITE, 0.25f));
            }
        }
    }
}


// Torta de capas con ganache que gotea: ocupa exacto las 2x2 baldosas del
// obstaculo y llega a la altura del bloque real.
static void DibujarTortaCentralTer(float t, bool detalle)
{
    const float h = MITAD_OBSTACULO_TER;
    float y = SUELO_TERRITORIO;
    const float capas[5] = { 0.52f, 0.14f, 0.52f, 0.14f, 0.48f };

    for (int k = 0; k < 5; k++)
    {
        bool crema = (k % 2) == 1;
        float lado = crema ? h * 2.0f + 0.04f : h * 2.0f;
        CuboTer({ 0.0f, y + capas[k] * 0.5f, 0.0f }, { lado, capas[k], lado }, crema ? CREMA_TER : TonoTer(BIZCOCHO_TER, 1.0f - 0.03f * (float)k));
        y += capas[k];
    }

    // Ganache arriba (hasta el tope del bloque) y gotas por los costados.
    float alto = TOPE_MURO_TER - y;
    CuboTer({ 0.0f, y + alto * 0.5f, 0.0f }, { h * 2.0f + 0.06f, alto, h * 2.0f + 0.06f }, GANACHE_TER);
    LosaTer({ -0.3f, TOPE_MURO_TER + 0.003f, -0.3f }, h * 0.6f, h * 0.35f, Color{ 120, 78, 58, 255 });

    for (int cara = 0; cara < 4; cara++)
    {
        for (int k = 0; k < 7; k++)
        {
            float a = -h + 0.2f + (2.0f * h - 0.4f) * (float)k / 6.0f;
            float largo = 0.25f + 0.55f * Hash01Ter(k, cara + 60);

            // Una gota por cara crece despacio, se suelta y vuelve a empezar.
            float ciclo = 0.0f;
            if (k == 2 + cara % 3)
            {
                ciclo = std::fmod(t * 0.28f + (float)cara * 0.31f, 1.0f);
                largo = 0.3f + 0.9f * ciclo;
            }

            float yCentro = y - largo * 0.5f + 0.02f;
            float salida = h + 0.035f;
            Vector3 c;
            Vector3 tam;

            switch (cara)
            {
                case 0: c = { a, yCentro, salida }; tam = { 0.2f, largo, 0.06f }; break;
                case 1: c = { a, yCentro, -salida }; tam = { 0.2f, largo, 0.06f }; break;
                case 2: c = { salida, yCentro, a }; tam = { 0.06f, largo, 0.2f }; break;
                default: c = { -salida, yCentro, a }; tam = { 0.06f, largo, 0.2f }; break;
            }

            CuboTer(c, tam, GANACHE_TER);
            DrawSphereEx({ c.x, y - largo + 0.03f, c.z }, 0.11f + 0.04f * ciclo, 6, 6, TonoTer(GANACHE_TER, 0.9f));

            if (ciclo > 0.85f)
            {
                // La gota cae al cuadro de abajo.
                float caida = (ciclo - 0.85f) / 0.15f;
                float yGota = y - largo - caida * caida * (y - largo - SUELO_TERRITORIO);
                Vector3 fuera = { c.x * 1.06f, yGota, c.z * 1.06f };
                DrawSphereEx(fuera, 0.1f, 6, 6, GANACHE_TER);
            }
        }
    }

    // Copos de crema sobre el borde, macarons y la vela.
    float yTope = TOPE_MURO_TER;
    for (int k = 0; k < 12; k++)
    {
        float u = (float)k / 12.0f * 4.0f;
        int cara = (int)u;
        float f = u - (float)cara;
        float a = -h + 0.25f + (2.0f * h - 0.5f) * f;
        float b = h - 0.25f;
        Vector3 p = cara == 0 ? Vector3{ a, yTope, b } : (cara == 1 ? Vector3{ b, yTope, -a } : (cara == 2 ? Vector3{ -a, yTope, -b } : Vector3{ -b, yTope, a }));
        DomoTer(p, 0.24f, 0.26f, 0.24f, CREMA_TER, 8, 3);
        PrismaTer({ p.x, yTope + 0.22f, p.z }, 0.1f, 0.0f, 0.16f, 6, CREMA_TER);
    }

    if (detalle)
    {
        const Color macarons[3] = { Color{ 236, 182, 202, 255 }, Color{ 200, 186, 236, 255 }, Color{ 250, 226, 170, 255 } };

        for (int k = 0; k < 3; k++)
        {
            float a = 2.1f * (float)k + 0.6f;
            Vector3 base = { std::cos(a) * 0.62f, yTope, std::sin(a) * 0.62f };
            PrismaTer(base, 0.3f, 0.32f, 0.08f, 10, TonoTer(macarons[k], 0.92f));
            PrismaTer({ base.x, base.y + 0.08f, base.z }, 0.27f, 0.27f, 0.07f, 10, CREMA_TER);
            PrismaTer({ base.x, base.y + 0.15f, base.z }, 0.32f, 0.3f, 0.06f, 10, TonoTer(macarons[k], 0.92f));
            DomoTer({ base.x, base.y + 0.21f, base.z }, 0.3f, 0.12f, 0.3f, macarons[k], 10, 2);
        }
    }

    // Vela a rayas (la llama va en la pasada de luces).
    for (int k = 0; k < 6; k++)
    {
        PrismaTer({ 0.0f, yTope + 0.18f * (float)k, 0.0f }, 0.13f, 0.13f, 0.18f, 10, k % 2 == 0 ? CREMA_TER : CINTA_TER);
    }

    VigaTer({ 0.0f, yTope + 1.08f, 0.0f }, { 0.0f, yTope + 1.2f, 0.0f }, 0.03f, Color{ 40, 32, 30, 255 });
}


//==================================================
// VISUAL: UTILERIA DE LA PASTELERIA
//==================================================

// MODELO FUTURO: macaron (dos tapas con pie y relleno).
static void MacaronTer(Vector3 base, float r, Color color)
{
    color = TinteTer(color, base);
    Color relleno = TinteTer(CREMA_TER, base);
    PrismaTer(base, r * 0.9f, r, r * 0.22f, 14, TonoTer(color, 0.95f));
    PrismaTer({ base.x, base.y + r * 0.22f, base.z }, r * 1.02f, r * 1.02f, r * 0.1f, 14, MezclaTer(color, WHITE, 0.2f));
    PrismaTer({ base.x, base.y + r * 0.32f, base.z }, r * 0.88f, r * 0.88f, r * 0.24f, 14, relleno);
    PrismaTer({ base.x, base.y + r * 0.56f, base.z }, r * 1.02f, r * 1.02f, r * 0.1f, 14, MezclaTer(color, WHITE, 0.2f));
    DomoTer({ base.x, base.y + r * 0.66f, base.z }, r, r * 0.42f, r, color, 14, 3);
}


static void TorreMacaronesTer(Vector3 base, int cantidad, float r, int semilla)
{
    const Color colores[5] =
    {
        Color{ 236, 180, 200, 255 },
        Color{ 198, 184, 236, 255 },
        Color{ 250, 226, 168, 255 },
        Color{ 196, 226, 200, 255 },
        Color{ 246, 200, 168, 255 }
    };

    for (int k = 0; k < cantidad; k++)
    {
        float dx = (Hash01Ter(semilla, k) - 0.5f) * r * 0.25f;
        float dz = (Hash01Ter(semilla, k + 9) - 0.5f) * r * 0.25f;
        MacaronTer({ base.x + dx, base.y + r * 1.08f * (float)k, base.z + dz }, r, colores[(semilla + k) % 5]);
    }
}


// MODELO FUTURO: cupcake (capacillo plisado, crema en espiral y cereza).
static void CupcakeTer(Vector3 base, float s, Color crema, Color capacillo, int semilla)
{
    crema = TinteTer(crema, base);
    PlisadoTer(base, 0.72f * s, 0.98f * s, 1.05f * s, 18, TinteTer(capacillo, base));
    DomoTer({ base.x, base.y + 1.05f * s, base.z }, 1.0f * s, 0.35f * s, 1.0f * s, TinteTer(BIZCOCHO_TER, base), 14, 2);

    float y = base.y + 1.2f * s;
    const float radios[3] = { 0.78f, 0.58f, 0.36f };

    for (int k = 0; k < 3; k++)
    {
        float R = radios[k] * s;
        ToroTer({ base.x, y, base.z }, R, 0.26f * s, crema, crema, false, semilla + k, 16, 6);
        DomoTer({ base.x, y, base.z }, R, 0.2f * s, R, crema, 12, 2);
        y += 0.32f * s;
    }

    PrismaTer({ base.x, y - 0.1f * s, base.z }, 0.22f * s, 0.0f, 0.4f * s, 8, crema);
    DrawSphereEx({ base.x + 0.08f * s, y + 0.38f * s, base.z }, 0.2f * s, 8, 8, TinteTer(Color{ 168, 32, 52, 255 }, base));
    VigaTer({ base.x + 0.08f * s, y + 0.52f * s, base.z }, { base.x + 0.2f * s, y + 0.85f * s, base.z - 0.05f * s }, 0.04f * s, TinteTer(Color{ 96, 120, 60, 255 }, base));

    for (int k = 0; k < 8; k++)
    {
        float a = Hash01Ter(semilla, k + 30) * 6.2832f;
        float r = (0.25f + 0.45f * Hash01Ter(semilla, k + 40)) * s;
        float yy = base.y + (1.3f + 0.6f * Hash01Ter(semilla, k + 50)) * s;
        CuboGiradoTer({ base.x + std::cos(a) * r, yy + 0.2f * s, base.z + std::sin(a) * r }, { 0.16f * s, 0.05f * s, 0.05f * s }, a * 57.0f, ColorChispitaTer(k + semilla));
    }
}


// MODELO FUTURO: dona glaseada con chispitas.
static void DonaTer(Vector3 base, float R, Color glaseado, int semilla)
{
    Vector3 c = { base.x, base.y + R * 0.42f, base.z };
    ToroTer(c, R, R * 0.42f, TinteTer(Color{ 214, 160, 100, 255 }, base), TinteTer(glaseado, base), true, semilla);

    for (int k = 0; k < 10; k++)
    {
        float a = 6.2832f * (float)k / 10.0f + Hash01Ter(semilla, k);
        float r = R * (0.85f + 0.3f * Hash01Ter(semilla, k + 20));
        CuboGiradoTer({ c.x + std::cos(a) * r, c.y + R * 0.4f, c.z + std::sin(a) * r }, { R * 0.22f, R * 0.06f, R * 0.06f }, Hash01Ter(semilla, k + 40) * 180.0f, ColorChispitaTer(k + semilla));
    }
}


static void PaloteTer(Vector3 c, float giro)
{
    float a = giro * DEG2RAD;
    Vector3 eje = { std::cos(a), 0.0f, std::sin(a) };
    Color madera = TinteTer(Color{ 214, 170, 118, 255 }, c);
    DrawCylinderEx(Vector3Add(c, Vector3Scale(eje, -2.6f)), Vector3Add(c, Vector3Scale(eje, 2.6f)), 0.62f, 0.62f, 14, madera);
    DrawCylinderEx(Vector3Add(c, Vector3Scale(eje, 2.6f)), Vector3Add(c, Vector3Scale(eje, 3.9f)), 0.22f, 0.26f, 10, TonoTer(madera, 0.85f));
    DrawCylinderEx(Vector3Add(c, Vector3Scale(eje, -3.9f)), Vector3Add(c, Vector3Scale(eje, -2.6f)), 0.26f, 0.22f, 10, TonoTer(madera, 0.85f));
}


static void BatidorTer(Vector3 c, float giro)
{
    float a = giro * DEG2RAD;
    Vector3 eje = { std::cos(a), 0.0f, std::sin(a) };
    Vector3 lado = { -eje.z, 0.0f, eje.x };
    Color metal = TinteTer(METAL_TER, c);
    Vector3 mango0 = Vector3Add(c, Vector3Scale(eje, -3.2f));
    Vector3 mango1 = Vector3Add(c, Vector3Scale(eje, -0.8f));
    DrawCylinderEx(mango0, mango1, 0.26f, 0.2f, 10, TinteTer(Color{ 196, 160, 214, 255 }, c));

    for (int k = 0; k < 5; k++)
    {
        float abre = -0.9f + 0.45f * (float)k;
        Vector3 previo = mango1;

        for (int s = 1; s <= 6; s++)
        {
            float u = (float)s / 6.0f;
            float ancho = std::sin(u * 3.1416f * 0.9f) * abre;
            Vector3 p = Vector3Add(Vector3Add(mango1, Vector3Scale(eje, u * 3.0f)), Vector3Scale(lado, ancho));
            p.y = c.y + 0.25f + std::sin(u * 3.1416f) * 0.5f * (1.0f - std::fabs(abre));
            VigaTer(previo, p, 0.06f, metal);
            previo = p;
        }
    }
}


static void MangaTer(Vector3 punta, Vector3 cola)
{
    Color tela = TinteTer(Color{ 244, 240, 236, 255 }, punta);
    DrawCylinderEx(punta, cola, 0.12f, 1.0f, 14, tela);
    DrawCylinderEx(cola, Vector3Add(cola, Vector3Scale(Vector3Normalize(Vector3Subtract(cola, punta)), 0.6f)), 1.0f, 0.4f, 14, TonoTer(tela, 0.9f));

    // Crema que sale por la boquilla: un rulo sobre el mantel.
    Color crema = TinteTer(Color{ 246, 206, 220, 255 }, punta);
    for (int k = 0; k < 6; k++)
    {
        float a = (float)k * 1.1f;
        DomoTer({ punta.x - 0.4f - 0.28f * (float)k, Y_MESA_TER, punta.z + 0.2f * std::sin(a) }, 0.32f, 0.28f, 0.32f, crema, 8, 2);
    }
}


static void BolTer(Vector3 base, float r, Color esmalte, float t)
{
    esmalte = TinteTer(esmalte, base);
    PrismaTer(base, r * 0.5f, r, r * 0.9f, 20, esmalte, false);
    AnilloTer({ base.x, base.y + r * 0.9f, base.z }, r * 0.92f, r * 1.04f, TonoTer(esmalte, 1.08f), 24);
    // Masa dentro con el rulo que deja la cuchara.
    Color masa = TinteTer(Color{ 246, 222, 176, 255 }, base);
    AnilloTer({ base.x, base.y + r * 0.66f, base.z }, 0.0f, r * 0.88f, masa, 20);
    AnilloTer({ base.x, base.y + r * 0.665f, base.z }, r * 0.3f, r * 0.42f, TonoTer(masa, 0.9f), 20);
    float a = t * 0.0f + 0.7f;
    VigaTer({ base.x + std::cos(a) * r * 0.2f, base.y + r * 0.6f, base.z + std::sin(a) * r * 0.2f }, { base.x + std::cos(a) * r * 1.4f, base.y + r * 1.9f, base.z + std::sin(a) * r * 0.9f }, 0.18f, TinteTer(MADERA_TER, base));
}


static void GalletasTer(Vector3 base, int cantidad, int semilla)
{
    for (int k = 0; k < cantidad; k++)
    {
        Vector3 c = { base.x + (Hash01Ter(semilla, k) - 0.5f) * 0.3f, base.y + 0.32f * (float)k, base.z + (Hash01Ter(semilla, k + 5) - 0.5f) * 0.3f };
        PrismaTer(c, 1.15f, 1.2f, 0.3f, 14, TinteTer(Color{ 222, 172, 110, 255 }, c));

        for (int g = 0; g < 5; g++)
        {
            float a = Hash01Ter(semilla + k, g) * 6.2832f;
            float r = 0.3f + 0.6f * Hash01Ter(semilla + k, g + 9);
            CuboTer({ c.x + std::cos(a) * r, c.y + 0.31f, c.z + std::sin(a) * r }, { 0.16f, 0.06f, 0.16f }, TinteTer(CHOCOLATE_TER, c));
        }
    }
}


// Frasco de chispitas volcado: las chispitas desparramadas por el mantel.
static void FrascoVolcadoTer(Vector3 c)
{
    Color vidrio = TinteTer(Color{ 214, 230, 236, 255 }, c);
    DrawCylinderEx({ c.x - 1.2f, c.y + 0.75f, c.z - 0.4f }, { c.x + 0.8f, c.y + 0.75f, c.z + 0.3f }, 0.75f, 0.75f, 14, vidrio);
    DrawCylinderEx({ c.x - 1.4f, c.y + 0.75f, c.z - 0.47f }, { c.x - 1.15f, c.y + 0.75f, c.z - 0.38f }, 0.8f, 0.8f, 14, TinteTer(Color{ 236, 150, 170, 255 }, c));

    for (int k = 0; k < 46; k++)
    {
        float a = Hash01Ter(k, 70) * 1.6f - 0.4f;
        float r = 0.9f + 3.2f * Hash01Ter(k, 71);
        CuboGiradoTer({ c.x + 0.8f + std::cos(a) * r, Y_MESA_TER + 0.03f, c.z + 0.3f + std::sin(a) * r }, { 0.2f, 0.06f, 0.07f }, Hash01Ter(k, 72) * 180.0f, ColorChispitaTer(k));
    }
}


static void TazaTer(Vector3 base)
{
    Color loza = TinteTer(Color{ 246, 242, 236, 255 }, base);
    AnilloTer({ base.x, base.y + 0.02f, base.z }, 0.0f, 1.5f, TonoTer(loza, 0.92f), 20);
    PrismaTer({ base.x, base.y + 0.04f, base.z }, 0.6f, 0.95f, 1.1f, 18, loza, false);
    AnilloTer({ base.x, base.y + 0.86f, base.z }, 0.0f, 0.86f, TinteTer(Color{ 150, 96, 60, 255 }, base), 18);
    DrawCylinderEx({ base.x + 0.95f, base.y + 0.85f, base.z }, { base.x + 1.35f, base.y + 0.5f, base.z }, 0.12f, 0.12f, 6, loza);
    DrawCylinderEx({ base.x + 1.35f, base.y + 0.5f, base.z }, { base.x + 0.85f, base.y + 0.25f, base.z }, 0.12f, 0.12f, 6, loza);
    AnilloTer({ base.x, base.y + 1.15f, base.z }, 0.85f, 0.95f, CINTA_TER, 18);
}


// MODELO FUTURO: torta de tres pisos sobre fuente con pie.
static void TortaPisosTer(Vector3 base, float t)
{
    Color loza = TinteTer(Color{ 240, 236, 230, 255 }, base);
    PrismaTer(base, 1.4f, 0.5f, 1.2f, 18, loza);
    PrismaTer({ base.x, base.y + 1.2f, base.z }, 3.8f, 3.8f, 0.18f, 24, loza);

    float y = base.y + 1.38f;
    const float radios[3] = { 3.3f, 2.4f, 1.55f };
    const float altos[3] = { 2.0f, 1.8f, 1.6f };
    Color crema = TinteTer(Color{ 252, 240, 232, 255 }, base);
    Color rosa = TinteTer(Color{ 238, 176, 196, 255 }, base);

    for (int k = 0; k < 3; k++)
    {
        PrismaTer({ base.x, y, base.z }, radios[k], radios[k], altos[k], 22, crema);

        // Cordon de perlas al pie y goteo rosa arriba.
        for (int p = 0; p < 18; p++)
        {
            float a = 6.2832f * (float)p / 18.0f;
            DrawSphereEx({ base.x + std::cos(a) * radios[k], y + 0.12f, base.z + std::sin(a) * radios[k] }, 0.16f, 5, 5, TonoTer(crema, 0.96f));

            if (std::sin(a) > -0.2f)
            {
                float largo = 0.3f + 0.4f * Hash01Ter(k, p);
                Vector3 g = { base.x + std::cos(a) * (radios[k] + 0.03f), y + altos[k] - largo * 0.5f, base.z + std::sin(a) * (radios[k] + 0.03f) };
                CuboGiradoTer(g, { 0.32f, largo, 0.06f }, -a * 57.2958f + 90.0f, rosa);
            }
        }

        AnilloTer({ base.x, y + altos[k] + 0.005f, base.z }, 0.0f, radios[k] + 0.04f, rosa, 22);
        y += altos[k];
    }

    // Flores de azucar arriba que se mecen apenas.
    for (int k = 0; k < 3; k++)
    {
        float a = 2.1f * (float)k + std::sin(t * 0.8f) * 0.05f;
        Vector3 f = { base.x + std::cos(a) * 0.6f, y + 0.25f, base.z + std::sin(a) * 0.6f };
        DomoTer(f, 0.45f, 0.35f, 0.45f, TinteTer(Color{ 250, 208, 222, 255 }, base), 10, 2);
        DrawSphereEx({ f.x, f.y + 0.32f, f.z }, 0.14f, 6, 6, TinteTer(Color{ 250, 226, 150, 255 }, base));
    }
}


// MODELO FUTURO: batidora planetaria con el batidor girando.
static void BatidoraTer(Vector3 base, float t)
{
    Color cuerpo = TinteTer(Color{ 140, 196, 184, 255 }, base);
    Color metal = TinteTer(METAL_TER, base);
    CuboTer({ base.x, base.y + 0.35f, base.z }, { 4.2f, 0.7f, 6.2f }, cuerpo);
    CuboTer({ base.x, base.y + 3.4f, base.z - 2.3f }, { 2.4f, 5.6f, 1.8f }, cuerpo);
    CuboTer({ base.x, base.y + 6.8f, base.z - 0.4f }, { 2.6f, 1.9f, 5.6f }, cuerpo);
    DrawCylinderEx({ base.x, base.y + 6.8f, base.z + 2.4f }, { base.x, base.y + 6.8f, base.z + 2.6f }, 0.9f, 0.9f, 14, metal);
    // Bol de acero.
    PrismaTer({ base.x, base.y + 0.7f, base.z + 0.6f }, 1.1f, 2.0f, 2.4f, 20, metal, false);
    AnilloTer({ base.x, base.y + 2.5f, base.z + 0.6f }, 0.0f, 1.85f, TinteTer(Color{ 250, 236, 214, 255 }, base), 20);
    // Batidor que gira dentro del bol.
    float giro = t * 6.0f;
    Vector3 eje = { base.x, base.y + 5.8f, base.z + 0.6f };
    VigaTer(eje, { eje.x, base.y + 3.2f, eje.z }, 0.18f, metal);

    for (int k = 0; k < 3; k++)
    {
        float a = giro + 2.094f * (float)k;
        Vector3 p0 = { eje.x + std::cos(a) * 0.15f, base.y + 3.4f, eje.z + std::sin(a) * 0.15f };
        Vector3 p1 = { eje.x + std::cos(a) * 0.8f, base.y + 2.4f, eje.z + std::sin(a) * 0.8f };
        VigaTer(p0, p1, 0.12f, metal);
    }
}


static void PlatosTer(Vector3 base, int cantidad)
{
    Color loza = TinteTer(Color{ 244, 240, 234, 255 }, base);

    for (int k = 0; k < cantidad; k++)
    {
        PrismaTer({ base.x, base.y + 0.22f * (float)k, base.z }, 1.6f, 2.2f, 0.2f, 20, k % 2 == 0 ? loza : TonoTer(loza, 0.96f));
        AnilloTer({ base.x, base.y + 0.22f * (float)k + 0.205f, base.z }, 2.0f, 2.12f, TinteTer(CINTA_TER, base), 20);
    }
}


static void BolsaHarinaTer(Vector3 base)
{
    Color papel = TinteTer(Color{ 226, 200, 160, 255 }, base);
    CuboGiradoTer({ base.x, base.y + 2.2f, base.z }, { 3.0f, 4.4f, 2.0f }, 12.0f, papel);
    CuboGiradoTer({ base.x, base.y + 4.6f, base.z }, { 2.8f, 0.4f, 1.0f }, 12.0f, TonoTer(papel, 0.9f));
    CuboGiradoTer({ base.x + 0.05f, base.y + 2.4f, base.z + 1.0f }, { 2.0f, 1.4f, 0.04f }, 12.0f, TinteTer(Color{ 246, 240, 228, 255 }, base));
    // Harina derramada al pie.
    AnilloTer({ base.x + 1.0f, Y_MESA_TER + 0.02f, base.z + 2.0f }, 0.0f, 1.6f, TinteTer(AZUCAR_TER, base), 14);
}


static void HuevosTer(Vector3 base)
{
    Color carton = TinteTer(Color{ 200, 186, 160, 255 }, base);
    CuboTer({ base.x, base.y + 0.4f, base.z }, { 4.6f, 0.8f, 2.4f }, carton);

    for (int k = 0; k < 6; k++)
    {
        Vector3 h = { base.x - 1.8f + 0.72f * (float)(k % 6), base.y + 0.8f, base.z + ((k % 2) == 0 ? -0.5f : 0.5f) };
        DomoTer(h, 0.36f, 0.62f, 0.36f, TinteTer(Color{ 246, 228, 204, 255 }, base), 10, 3);
    }
}


static void DibujarUtileriaTer(float t, int paso)
{
    float y = Y_MESA_TER;

    // Media distancia: sobre la mesada, contra la pared.
    TortaPisosTer({ -15.6f, y, -8.4f }, t);
    BatidoraTer({ 16.0f, y, -8.6f }, t);
    BolsaHarinaTer({ -23.0f, y, -9.0f });
    PlatosTer({ 23.5f, y, -4.0f }, 6);

    FrascoTer({ -8.6f, y, -10.8f }, 1.0f, 3.2f, Color{ 214, 170, 112, 255 }, Color{ 196, 150, 104, 255 });
    FrascoTer({ 8.8f, y, -10.6f }, 0.9f, 2.6f, Color{ 236, 176, 196, 255 }, Color{ 196, 150, 104, 255 });

    // Anillo cercano: alrededor de la torta-arena.
    TorreMacaronesTer({ -9.6f, y, -2.6f }, 4, 0.95f, 1);
    TorreMacaronesTer({ -10.6f, y, 3.6f }, 3, 0.9f, 3);
    CupcakeTer({ -12.2f, y, -5.8f }, 1.25f, Color{ 246, 204, 220, 255 }, Color{ 196, 184, 232, 255 }, 5);
    CupcakeTer({ -13.2f, y, 0.6f }, 1.05f, Color{ 252, 244, 232, 255 }, Color{ 238, 186, 200, 255 }, 9);
    GalletasTer({ -14.0f, y, 6.2f }, 4, 2);
    PaloteTer({ -16.6f, y + 0.62f, -2.0f }, 80.0f);

    DonaTer({ 10.4f, y, -3.4f }, 1.2f, Color{ 244, 172, 196, 255 }, 4);
    DonaTer({ 10.5f, y + 1.05f, -3.3f }, 1.15f, Color{ 120, 74, 50, 255 }, 7);
    DonaTer({ 12.8f, y, 4.2f }, 1.1f, Color{ 250, 246, 238, 255 }, 11);
    MangaTer({ 10.6f, y + 0.5f, 0.4f }, { 14.0f, y + 1.4f, 1.6f });
    BolTer({ 15.6f, y, -2.8f }, 2.3f, Color{ 172, 210, 222, 255 }, t);
    CupcakeTer({ 9.8f, y, -7.4f }, 1.1f, Color{ 214, 196, 240, 255 }, Color{ 250, 232, 196, 255 }, 13);

    if (paso == 1)
    {
        BatidorTer({ 14.6f, y + 0.1f, 7.2f }, 200.0f);
        FrascoVolcadoTer({ 9.2f, y, 7.6f });
        TazaTer({ -18.6f, y, 2.8f });
        HuevosTer({ 20.4f, y, 2.2f });
        CupcakeTer({ -9.6f, y, 8.6f }, 0.95f, Color{ 250, 230, 176, 255 }, Color{ 186, 214, 228, 255 }, 17);
    }

    // Al frente, bajitos para no tapar la arena.
    MacaronTer({ -4.6f, y, 10.6f }, 0.9f, Color{ 198, 184, 236, 255 });
    MacaronTer({ 3.6f, y, 11.2f }, 0.85f, Color{ 236, 180, 200, 255 });
    MacaronTer({ 5.2f, y + 0.0f, 10.0f }, 0.8f, Color{ 250, 226, 168, 255 });
    DrawSphereEx({ -2.0f, y + 0.4f, 11.6f }, 0.42f, 10, 10, TinteTer(Color{ 168, 32, 52, 255 }, { -2.0f, y, 11.6f }));
}


//==================================================
// VISUAL: COLADOR DEL SECTOR SELLADO
//==================================================

// Altura del colador sobre su sector: baja durante el aviso, tamiza un
// momento al sellarse y se va para no tapar el resto de la torta.
// Devuelve < 0 si no se ve.
static float AlturaColadorTer(const MinijuegoTerritorioConquista& m, int& sector)
{
    sector = m.sectorAnunciado;

    if (sector < 0 || m.tiempoAnuncio < 0.0f)
    {
        return -1.0f;
    }

    if (m.estadoSector == SECTOR_TERRITORIO_AVISO)
    {
        float u = Limitar01Ter(m.tiempoAnuncio / DURACION_AVISO_SELLADO);
        float suave = u * u * (3.0f - 2.0f * u);
        return 9.5f - 4.7f * suave;
    }

    if (m.estadoSector == SECTOR_TERRITORIO_SELLADO && m.tiempoAnuncio < SALIDA_COLADOR_TER + 1.0f)
    {
        float u = Limitar01Ter((m.tiempoAnuncio - SALIDA_COLADOR_TER) / 1.0f);
        return 4.8f + 9.0f * u * u;
    }

    return -1.0f;
}


static void DibujarColadorTer(const MinijuegoTerritorioConquista& m)
{
    int sector = -1;
    float altura = AlturaColadorTer(m, sector);

    if (altura < 0.0f)
    {
        return;
    }

    Vector3 c = CentroSectorTer(sector);
    float sacudida = m.estadoSector == SECTOR_TERRITORIO_SELLADO ? std::sin(m.tiempoAnimacion * 26.0f) * 0.12f : 0.0f;
    c.x += sacudida;
    c.y = altura;
    // Adelantado hacia la camara: asi, visto en diagonal, queda justo encima
    // de su sector en lugar de tapar el cuadrante de atras.
    c.z += altura * 0.85f;
    const float radio = 2.2f;
    Color metal = METAL_TER;

    // Aro, canasta de malla y mango hacia afuera de la torta.
    PrismaTer({ c.x, c.y, c.z }, radio, radio, 0.35f, 28, metal, false);
    PrismaTer({ c.x, c.y - 0.9f, c.z }, radio * 0.55f, radio * 0.98f, 0.9f, 24, Color{ 150, 154, 162, 255 }, false);
    AnilloTer({ c.x, c.y - 0.9f, c.z }, 0.0f, radio * 0.55f, AZUCAR_TER, 20);
    AnilloTer({ c.x, c.y - 0.2f, c.z }, 0.0f, radio * 0.95f, AZUCAR_TER, 24);

    float lado = (sector % 2 == 0) ? -1.0f : 1.0f;
    VigaTer({ c.x + lado * radio, c.y + 0.2f, c.z }, { c.x + lado * (radio + 3.4f), c.y + 0.9f, c.z }, 0.3f, metal);
    VigaTer({ c.x - lado * radio, c.y + 0.2f, c.z }, { c.x - lado * (radio + 0.6f), c.y + 0.2f, c.z }, 0.2f, metal);
}


//==================================================
// VISUAL: JUGADORES SOBRE LA TORTA
//==================================================

static void DibujarJugadoresTer(
    const MinijuegoTerritorioConquista& m,
    const JugadorPrueba jugadores[],
    int limite,
    const Participante participantes[],
    bool mostrarDebug
)
{
    for (int i = 0; i < limite; i++)
    {
        if (!m.resultado.participantes[i].participo)
        {
            continue;
        }

        const JugadorPrueba& jugador = jugadores[i];
        Color color = m.coloresJugadores[i];
        Vector3 pie = { jugador.posicion.x, 0.075f, jugador.posicion.z };

        // Aro de su color en el piso (blanco por dentro para separarlo del
        // glaseado propio) y el area que reclamara el pound en curso.
        AnilloTer(pie, 0.5f, 0.6f, color, 22);
        AnilloTer({ pie.x, pie.y - 0.003f, pie.z }, 0.6f, 0.66f, WHITE, 22);

        if (jugador.preparandoGolpeSuelo || jugador.golpeSueloActivo)
        {
            float pulso = 0.5f + 0.5f * std::sin(m.tiempoAnimacion * 18.0f);
            const int TRAMOS = 24;

            for (int k = 0; k < TRAMOS; k += 2)
            {
                float a0 = 6.2832f * (float)k / (float)TRAMOS;
                float a1 = 6.2832f * (float)(k + 1) / (float)TRAMOS;
                rlBegin(RL_QUADS);
                float r0 = RADIO_RECLAMO_POUND - 0.1f;
                float r1 = RADIO_RECLAMO_POUND + 0.05f;
                Color tramo = MezclaTer(color, WHITE, 0.2f * pulso);
                QuadTer(
                    { pie.x + std::cos(a0) * r1, 0.09f, pie.z + std::sin(a0) * r1 },
                    { pie.x + std::cos(a0) * r0, 0.09f, pie.z + std::sin(a0) * r0 },
                    { pie.x + std::cos(a1) * r0, 0.09f, pie.z + std::sin(a1) * r0 },
                    { pie.x + std::cos(a1) * r1, 0.09f, pie.z + std::sin(a1) * r1 },
                    tramo
                );
                rlEnd();
            }
        }

        Participante visual = participantes[i];
        visual.conectado = true;
        visual.color = color;
        DibujarJugadorModeloCompartido(jugador, visual);

        if (mostrarDebug)
        {
            DrawBoundingBox(CrearHitboxJugadorPrueba(jugador), LIME);
        }
    }
}


static void DibujarParticulasVisualesTer(const MinijuegoTerritorioConquista& m)
{
    for (int i = 0; i < MAX_PARTICULAS_VISUALES_TERRITORIO; i++)
    {
        const ParticulaVisualTerritorio& p = m.particulasVisuales[i];

        if (!p.activa || p.brillo)
        {
            continue;
        }

        float s = p.tamano * Limitar01Ter(p.vida / 0.3f + 0.3f);
        CuboGiradoTer(p.posicion, { s * 2.4f, s * 0.7f, s * 0.8f }, p.giro, p.color);
    }
}


//==================================================
// VISUAL: TRANSLUCIDOS Y LUCES FALSAS
//==================================================

static void DibujarTranslucidosTer(const MinijuegoTerritorioConquista& m, bool decoracion)
{
    rlDrawRenderBatchActive();
    rlDisableDepthMask();

    // Sombra del colador sobre su sector.
    int sector = -1;
    float altura = AlturaColadorTer(m, sector);

    if (altura > 0.0f)
    {
        Vector3 c = CentroSectorTer(sector);
        float cerca = Limitar01Ter(1.0f - (altura - 4.8f) / 8.0f);
        ResplandorSueloTer({ c.x, 0.1f, c.z }, 2.4f + 1.4f * (1.0f - cerca), Color{ 70, 40, 30, (unsigned char)(40.0f + 70.0f * cerca) });
    }

    // Borde del sector en aviso o sellado: cinta punteada blanca y chocolate.
    if (m.estadoSector != SECTOR_TERRITORIO_LIBRE && m.sectorActual >= 0)
    {
        Vector3 c = CentroSectorTer(m.sectorActual);
        float mitad = MITAD_ARENA_TERRITORIO * 0.5f - 0.05f;
        bool aviso = m.estadoSector == SECTOR_TERRITORIO_AVISO;
        float pulso = aviso ? 0.6f + 0.4f * std::sin(m.tiempoAnimacion * 14.0f) : 0.85f;
        const int TRAMOS = 10;

        for (int lado = 0; lado < 4; lado++)
        {
            for (int k = 0; k < TRAMOS; k++)
            {
                float u0 = -mitad + 2.0f * mitad * (float)k / (float)TRAMOS;
                float u1 = u0 + 2.0f * mitad / (float)TRAMOS;
                Color tramo = (k + lado) % 2 == 0 ? Color{ 255, 255, 255, (unsigned char)(230.0f * pulso) } : Color{ 92, 54, 36, (unsigned char)(230.0f * pulso) };
                float g = 0.09f;
                Vector3 a, b, cc, d;

                if (lado == 0 || lado == 1)
                {
                    float z = c.z + (lado == 0 ? -mitad : mitad);
                    a = { c.x + u0, 0.11f, z - g }; b = { c.x + u0, 0.11f, z + g };
                    cc = { c.x + u1, 0.11f, z + g }; d = { c.x + u1, 0.11f, z - g };
                }
                else
                {
                    float x = c.x + (lado == 2 ? -mitad : mitad);
                    a = { x - g, 0.11f, c.z + u0 }; b = { x + g, 0.11f, c.z + u0 };
                    cc = { x + g, 0.11f, c.z + u1 }; d = { x - g, 0.11f, c.z + u1 };
                }

                rlBegin(RL_QUADS);
                QuadTer(a, b, cc, d, tramo);
                rlEnd();
            }
        }
    }

    // Vidrio de la ventana: reflejos diagonales.
    if (decoracion)
    {
        for (int k = 0; k < 2; k++)
        {
            float x0 = (k == 0 ? -8.5f : 3.0f);
            rlBegin(RL_QUADS);
            QuadTer(
                { x0, Y_ALFEIZAR_TER + 0.3f, Z_PARED_TER - 0.05f },
                { x0 + 2.0f, Y_ALFEIZAR_TER + 0.3f, Z_PARED_TER - 0.05f },
                { x0 + 6.0f, 10.0f, Z_PARED_TER - 0.05f },
                { x0 + 4.6f, 10.0f, Z_PARED_TER - 0.05f },
                Color{ 255, 255, 255, 26 }
            );
            rlEnd();
        }

        // Campana de vidrio sobre una porcion, entre la torta de pisos y la pared.
        Vector3 campana = { -6.8f, Y_MESA_TER, -10.0f };
        PrismaTer(campana, 1.6f, 1.6f, 0.2f, 20, Color{ 240, 236, 230, 255 });
        CuboTer({ campana.x, campana.y + 0.75f, campana.z }, { 1.6f, 1.1f, 1.1f }, TinteTer(Color{ 120, 74, 50, 255 }, campana));
        DomoTer({ campana.x, campana.y + 0.2f, campana.z }, 1.5f, 2.4f, 1.5f, Color{ 220, 236, 244, 70 }, 16, 4);
        DrawSphereEx({ campana.x, campana.y + 2.75f, campana.z }, 0.25f, 8, 8, Color{ 220, 236, 244, 140 });
    }

    rlDrawRenderBatchActive();
    rlEnableDepthMask();
}


static Vector3 PosicionLamparaTer(int k, float t)
{
    float lado = k == 0 ? -1.0f : 1.0f;
    float vaiven = std::sin(t * 0.9f + (float)k * 1.7f) * 0.25f;
    return Vector3{ lado * 12.6f + vaiven, 6.2f, -1.6f };
}


static void DibujarLamparasTer(float t)
{
    for (int k = 0; k < 2; k++)
    {
        Vector3 p = PosicionLamparaTer(k, t);
        VigaTer({ p.x, 30.0f, p.z }, { p.x, p.y + 1.4f, p.z }, 0.08f, Color{ 60, 50, 46, 255 });
        Color pantalla = TinteTer(Color{ 238, 220, 186, 255 }, p);
        PrismaTer({ p.x, p.y, p.z }, 1.7f, 0.5f, 1.4f, 18, pantalla, false);
        PrismaTer({ p.x, p.y + 1.4f, p.z }, 0.5f, 0.3f, 0.2f, 12, MADERA_TER);
        DrawSphereEx({ p.x, p.y + 0.1f, p.z }, 0.45f, 8, 8, Color{ 255, 244, 214, 255 });
    }
}


static void DibujarLucesTer(const MinijuegoTerritorioConquista& m, bool decoracion)
{
    if (CalidadEfectos() == CALIDAD_BAJA)
    {
        return;
    }

    float t = m.tiempoAnimacion;

    rlDrawRenderBatchActive();
    rlDisableDepthMask();
    BeginBlendMode(BLEND_ADDITIVE);

    // Sol del atardecer detras de la ventana y haces que bajan a la mesa.
    float vaiven = 0.85f + 0.15f * std::sin(t * 0.4f);
    ResplandorTer({ -8.0f, 0.0f, Z_PARED_TER - 0.4f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, 9.0f, Color{ 255, 170, 100, 40 }, 1.4f);

    for (int k = 0; k < 4; k++)
    {
        float x0 = -X_VENTANA_TER + 0.6f + 5.4f * (float)k;
        float x1 = x0 + 4.2f;
        float zv = Z_PARED_TER - 0.2f;
        float caidaX = 3.0f;
        unsigned char alfa = (unsigned char)(20.0f * vaiven);

        rlBegin(RL_QUADS);
        rlColor4ub(255, 176, 104, alfa); rlVertex3f(x0, 8.0f, zv);
        rlColor4ub(255, 176, 104, alfa); rlVertex3f(x1, 8.0f, zv);
        rlColor4ub(255, 160, 96, 0); rlVertex3f(x1 + caidaX, Y_MESA_TER, zv + 9.0f);
        rlColor4ub(255, 160, 96, 0); rlVertex3f(x0 + caidaX, Y_MESA_TER, zv + 9.0f);
        rlEnd();

        // Charco del pano sobre el mantel, al pie de la ventana.
        Color charco = Color{ 255, 170, 100, (unsigned char)(30.0f * vaiven) };
        Color lejos = Color{ 255, 170, 100, 0 };
        rlBegin(RL_QUADS);
        VerticeTer({ x0 + 0.6f, Y_MESA_TER + 0.03f, Z_PARED_TER + 0.6f }, charco);
        VerticeTer({ x0 + caidaX * 0.7f, Y_MESA_TER + 0.03f, Z_PARED_TER + 5.0f }, lejos);
        VerticeTer({ x1 + caidaX * 0.7f, Y_MESA_TER + 0.03f, Z_PARED_TER + 5.0f }, lejos);
        VerticeTer({ x1 + 0.6f, Y_MESA_TER + 0.03f, Z_PARED_TER + 0.6f }, charco);
        rlEnd();
    }

    // Lamparas colgantes: halo, cono y charco sobre la mesa.
    if (decoracion)
    {
        for (int k = 0; k < 2; k++)
        {
            Vector3 p = PosicionLamparaTer(k, t);
            Vector3 foco = { p.x, p.y - 0.1f, p.z };
            float pulso = 0.95f + 0.05f * std::sin(t * 5.0f + (float)k * 2.0f);
            ResplandorFrenteTer(foco, 2.2f * pulso, Color{ 255, 200, 130, 70 });
            ResplandorFrenteTer(foco, 0.9f * pulso, Color{ 255, 238, 200, 130 });
            DrawCylinderEx({ foco.x, foco.y, foco.z }, { foco.x, Y_MESA_TER + 0.05f, foco.z }, 1.5f, 4.6f, 18, Color{ 255, 206, 140, 10 });

            for (int r = 0; r < 3; r++)
            {
                ResplandorSueloTer({ foco.x, Y_MESA_TER + 0.04f + 0.003f * (float)r, foco.z }, (2.2f + 1.6f * (float)r) * pulso, Color{ 255, 196, 120, 30 });
            }
        }

        // Horno encendido: brillo en la puerta y charco en la mesada.
        float pulsoHorno = 0.9f + 0.1f * std::sin(t * 2.3f);
        ResplandorTer({ 29.0f, -0.6f, Z_PARED_TER + 3.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, 6.0f, Color{ 255, 140, 60, (unsigned char)(60.0f * pulsoHorno) }, 1.4f);
        ResplandorSueloTer({ 28.0f, Y_MESA_TER + 0.04f, Z_PARED_TER + 5.0f }, 6.0f * pulsoHorno, Color{ 255, 140, 60, 40 }, 1.3f);
    }

    // Llama de la vela de la torta central y su charco sobre el glaseado.
    {
        float titileo = 0.85f + 0.15f * std::sin(t * 13.0f) * std::sin(t * 7.3f + 1.0f);
        Vector3 llama = { std::sin(t * 3.0f) * 0.03f, TOPE_MURO_TER + 1.42f, 0.0f };
        ResplandorFrenteTer(llama, 1.6f * titileo, Color{ 255, 170, 70, 80 });
        ResplandorFrenteTer(llama, 0.55f * titileo, Color{ 255, 220, 140, 200 });
        ResplandorTer(llama, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.68f, 0.73f }, 0.22f * titileo, Color{ 255, 250, 220, 255 }, 0.55f);

        for (int r = 0; r < 3; r++)
        {
            ResplandorSueloTer({ 0.0f, TOPE_MURO_TER + 0.01f + 0.002f * (float)r, 0.0f }, (1.0f + 0.5f * (float)r) * titileo, Color{ 255, 180, 90, 34 });
            ResplandorSueloTer({ 0.0f, 0.06f + 0.002f * (float)r, 0.0f }, (2.4f + 1.0f * (float)r) * titileo, Color{ 255, 180, 90, 14 });
        }
    }

    // Destello del glaseado recien reclamado.
    for (int fila = 0; fila < FILAS_TERRITORIO; fila++)
    {
        for (int columna = 0; columna < COLUMNAS_TERRITORIO; columna++)
        {
            const BaldosaTerritorio& b = m.baldosas[IndiceBaldosaTerritorio(columna, fila)];

            if (b.destello <= 0.0f || b.dueno < 0 || b.dueno >= MAX_PARTICIPANTES)
            {
                continue;
            }

            Vector3 c = CentroBaldosaTerritorio(columna, fila);
            Color color = m.coloresJugadores[b.dueno];
            color.a = (unsigned char)(120.0f * b.destello);
            LosaTer({ c.x, 0.16f, c.z }, 0.62f, 0.62f, color);
        }
    }

    // Ondas del ground pound: corona que se abre.
    for (int i = 0; i < MAX_ONDAS_TERRITORIO; i++)
    {
        const OndaTerritorio& onda = m.ondas[i];

        if (!onda.activa)
        {
            continue;
        }

        float u = Limitar01Ter(onda.tiempo / 0.45f);
        float r = 0.4f + u * RADIO_RECLAMO_POUND;
        AnilloTer({ onda.posicion.x, 0.14f, onda.posicion.z }, r - 0.35f, r, AlfaTer(onda.color, 200.0f * (1.0f - u)), 32);
        ResplandorSueloTer({ onda.posicion.x, 0.13f, onda.posicion.z }, r, AlfaTer(MezclaTer(onda.color, WHITE, 0.4f), 90.0f * (1.0f - u)));
    }

    for (int i = 0; i < MAX_PARTICULAS_VISUALES_TERRITORIO; i++)
    {
        const ParticulaVisualTerritorio& p = m.particulasVisuales[i];

        if (!p.activa || !p.brillo)
        {
            continue;
        }

        float vida = p.vida / p.vidaMaxima;
        ResplandorSueloTer({ p.posicion.x, 0.17f, p.posicion.z }, p.tamano * (2.0f - vida), AlfaTer(p.color, 140.0f * vida));
    }

    // Polvo de harina que flota en los haces de sol.
    if (decoracion)
    {
        int motas = (int)(40.0f * FactorCalidadGrafica(CalidadParticulas()));

        for (int k = 0; k < motas; k++)
        {
            float ciclo = 10.0f + 8.0f * Hash01Ter(k, 220);
            float u = std::fmod(t / ciclo + Hash01Ter(k, 221), 1.0f);
            float x = -X_VENTANA_TER + 2.0f * X_VENTANA_TER * Hash01Ter(k, 222) + u * 3.0f;
            Vector3 p =
            {
                x + std::sin(t * 0.4f + (float)k) * 0.4f,
                7.0f - u * 9.0f + std::sin(t * 0.7f + (float)k * 1.3f) * 0.3f,
                Z_PARED_TER + 0.5f + u * 9.0f
            };
            float alfa = std::sin(u * 3.1416f);
            ResplandorFrenteTer(p, 0.09f, Color{ 255, 226, 180, (unsigned char)(170.0f * alfa) });
        }
    }

    EndBlendMode();
    rlEnableDepthMask();
}


//==================================================
// DIBUJO
//==================================================

void MinijuegoTerritorioConquista::Dibujar(
    const JugadorPrueba jugadores[],
    int cantidadMaxima,
    const Participante participantes[],
    bool mostrarDebug
) const
{

    int limite = cantidadMaxima < MAX_PARTICIPANTES ? cantidadMaxima : MAX_PARTICIPANTES;
    const float t = tiempoAnimacion;
    bool decoracion = CalidadDecoracion() != CALIDAD_BAJA;
    int paso = CalidadDecoracion() == CALIDAD_ALTA ? 1 : 2;
    int ancho = GetScreenWidth();
    int alto = GetScreenHeight();

    // Entrada: durante la cuenta la camara baja desde mas arriba y atras.
    Camera3D vista = camara;

    if (fase == FASE_TERRITORIO_PREPARACION)
    {
        float u = Limitar01Ter(tiempoPreparacion / DURACION_PREPARACION_TERRITORIO);
        float suave = u * u * (3.0f - 2.0f * u) * ObtenerFactorMovimientoCamaraMinijuegos();
        vista.position.y += 4.0f * suave;
        vista.position.z += 4.5f * suave;
    }

    // Fondo por si algo queda sin cubrir: penumbra calida de la cocina.
    DrawRectangleGradientV(0, 0, ancho, alto, Color{ 236, 198, 168, 255 }, COLOR_PENUMBRA_TER);

    BeginMode3D(vista);
    rlDisableBackfaceCulling();

    DibujarExteriorTer(t, decoracion);
    DibujarParedTer(t, decoracion);
    DibujarMantelTer(decoracion ? 1 : 2);

    if (decoracion)
    {
        DibujarPuntillaTer();
        DibujarUtileriaTer(t, paso);
        DibujarLamparasTer(t);
    }

    // Bandeja plateada bajo la torta.
    CuboNieblaTer({ 0.0f, Y_MESA_TER + 0.085f, 0.0f }, { BORDE_TER * 2.0f + 2.2f, 0.17f, BORDE_TER * 2.0f + 2.2f }, Color{ 214, 212, 216, 255 });

    DibujarBarritasTer(decoracion);
    DibujarBaldosasTer(*this);
    DibujarTortaCentralTer(t, decoracion);
    DibujarColadorTer(*this);

    DibujarJugadoresTer(*this, jugadores, limite, participantes, mostrarDebug);
    DibujarParticulasTierra(particulas, MAX_PARTICULAS_TERRITORIO);
    DibujarParticulasVisualesTer(*this);

    if (mostrarDebug)
    {
        for (int i = 0; i < cantidadBloques; i++)
        {
            DrawBoundingBox(CrearHitboxBloquePrueba(bloques[i]), YELLOW);
        }
    }

    DibujarTranslucidosTer(*this, decoracion);
    DibujarLucesTer(*this, decoracion);

    rlEnableBackfaceCulling();
    EndMode3D();

    // Rotulos J1..J4 sobre cada jugador, con el color de su glaseado.
    for (int i = 0; i < limite; i++)
    {
        if (!resultado.participantes[i].participo || !participantes[i].activo)
        {
            continue;
        }

        Vector3 cabeza = jugadores[i].posicion;
        cabeza.y += jugadores[i].tamano.y * 0.5f + 0.75f;
        Vector2 p = GetWorldToScreen(cabeza, vista);

        char rotulo[8];
        std::snprintf(rotulo, sizeof(rotulo), "J%d", participantes[i].numeroJugador > 0 ? participantes[i].numeroJugador : i + 1);

        OpcionesEtiquetaUI op;
        op.borde = coloresJugadores[i];
        op.pico = true;
        DibujarEtiquetaUI(rotulo, p, 0.0f, op);
    }

    // Anuncio del sector: sobre el cuadrante que se va a sellar.
    if (fase == FASE_TERRITORIO_JUGANDO && estadoSector != SECTOR_TERRITORIO_LIBRE && sectorActual >= 0)
    {
        Vector3 c = CentroSectorTer(sectorActual);
        Vector2 s = GetWorldToScreen({ c.x, 3.2f, c.z }, vista);
        float limiteAbajo = (float)(alto - MargenBarraRondaMinijuegos()) - Px(70.0f);
        if (s.y < Px(150.0f)) s.y = Px(150.0f);
        if (s.y > limiteAbajo) s.y = limiteAbajo;

        if (estadoSector == SECTOR_TERRITORIO_AVISO)
        {
            float u = Limitar01Ter(tiempoAnuncio / 0.25f);
            float escalaY = Px(10.0f) * (1.0f - u);
            DibujarTextoMundoContornoUI("¡AZÚCAR GLAS!", s.x, s.y - escalaY, UI_TEXTO_GRANDE, Color{ 255, 250, 240, 255 }, u);
            char texto[32];
            float quedan = tiempoSector > 0.0f ? tiempoSector : 0.0f;
            std::snprintf(texto, sizeof(texto), "ESTA ZONA SE SELLA EN %d,%d", (int)quedan, (int)(quedan * 10.0f) % 10);
            DibujarTextoMundoContornoUI(texto, s.x, s.y + Px(34.0f), UI_TEXTO, UI_TEXTO_MUNDO, u);
        }
        else
        {
            char texto[24];
            std::snprintf(texto, sizeof(texto), "SELLADA %d", (int)std::ceil(tiempoSector > 0.0f ? tiempoSector : 0.0f));
            DibujarTextoMundoContornoUI(texto, s.x, s.y, UI_TEXTO, Color{ 255, 250, 240, 255 }, 0.9f);
        }
    }

    // HUD comun. Los bots llevan el color propio de su glaseado.
    Participante visuales[MAX_PARTICIPANTES];

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        visuales[i] = participantes[i];

        if (resultado.participantes[i].participo)
        {
            visuales[i].color = coloresJugadores[i];
        }
    }

    DatosHudMinijuego hud;
    PrepararDatosHudMinijuego(
        hud,
        "TERRITORIO",
        "CAMINÁ PARA GLASEAR CUADROS CON TU COLOR. GANA QUIEN TENGA MÁS AL FINAL.",
        resultado
    );

    hud.fase = fase == FASE_TERRITORIO_PREPARACION
        ? HUD_MINIJUEGO_CUENTA
        : (fase == FASE_TERRITORIO_TERMINADO ? HUD_MINIJUEGO_FIN : HUD_MINIJUEGO_JUGANDO);
    hud.cuentaRestante = tiempoPreparacion;
    hud.reloj = HUD_RELOJ_CUENTA_ATRAS;
    hud.tiempoRestante = tiempoRestante;
    hud.tiempoTotal = DURACION_PARTIDA_TERRITORIO;
    hud.finPorTiempo = true;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        JugadorHudMinijuego& j = hud.jugadores[i];
        if (!j.visible) continue;

        float porcentaje = PorcentajeTerritorio(*this, i);
        std::snprintf(j.valor, sizeof(j.valor), "%.0f %%", porcentaje);
        j.valorNumerico = estadosJugadores[i].baldosas;
        j.progreso = Limitar01Ter(porcentaje / 100.0f);
        std::snprintf(j.detalleFinal, sizeof(j.detalleFinal), estadosJugadores[i].baldosas == 1 ? "%d CUADRO" : "%d CUADROS", estadosJugadores[i].baldosas);

        if (fase == FASE_TERRITORIO_JUGANDO && i < limite && jugadores[i].aplastado)
        {
            std::snprintf(j.estado, sizeof(j.estado), "¡ATURDIDO!");
        }
        else if (fase == FASE_TERRITORIO_JUGANDO && estadosJugadores[i].recargaPound > 0.0f && !participantes[i].esBot)
        {
            std::snprintf(j.estado, sizeof(j.estado), "RECARGANDO");
        }

        if (!participantes[i].esBot)
        {
            std::snprintf(
                j.controles,
                sizeof(j.controles),
                "MOVÉ: %s\nPOUND: %s EN EL AIRE",
                TextoMoverHud(participantes[i]),
                ObtenerTextoBotonPrincipal(participantes[i])
            );
        }
    }

    DibujarHudMinijuego(hud, visuales);
}


const ResultadoMinijuego& MinijuegoTerritorioConquista::ObtenerResultado() const
{
    return resultado;
}
