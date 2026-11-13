// Abre una ventana real, juega una partida de tablero con 4 bots en cada
// tablero pedido y guarda capturas PNG tal como se ven en partida: escena 3D,
// capa de pantalla del tablero, HUD y contador de FPS (el mismo texto que
// dibuja Core/Juego.cpp con config.mostrarFPS).
//
// Tomas por tablero:
//   general      vista general del comienzo de ronda (camaraZoomGeneral)
//   recorrido    camara de partida siguiendo al jugador activo, tras unos
//                turnos simulados (fichas repartidas)
//   gimmick      landmark del gimmick con el gimmick activo
//   fondo        mirando por encima del borde trasero del escenario, mas baja
//   bifurcacion  encuadre de las dos rutas de una casilla con 2 conexiones
//
// Cada toma se deja asentar ~1 s, se mide el FPS promedio y el peor frame de
// ~2 s (sin contar el frame de la captura) y se guarda
// build/capturas/tableros/<tablero>_<toma>_<ancho>x<alto>.png.
//
// Uso: CapturarTableros.exe [--tablero N|nombre] [--resolucion ANCHO ALTO]
//                           [--calidad baja|media|alta] [--fps N] (0 = sin limite)
//                           [--salida CARPETA]
// Normalmente se lanza con Tests/CapturarTableros.ps1.
// Ejecutar desde la raiz del repo (rutas de Assets relativas).
#include "Board/CatalogoTableros.h"
#include "Core/ResultadoMinijuego.h"
#include "Gameplay/PartidaTablero.h"
#include "Minigames/ModeloJugadorCompartido.h"
#include "UI/EstiloUIGlobal.h"

#include "raylib.h"
#include "rlgl.h"

#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>


//==================================================
// PARAMETROS
//==================================================

static const char* CARPETA_CAPTURAS = "build/capturas/tableros";

static const float DT_SIMULACION = 1.0f / 30.0f;
static const float SEGUNDOS_ASENTAR = 1.2f;
static const int FRAMES_MINIMOS_ASENTAR = 40;
static const float SEGUNDOS_MEDICION = 2.0f;

// Simulacion rapida (sin dibujar) antes de la toma de recorrido.
static const float SEGUNDOS_SIMULADOS_RECORRIDO = 40.0f;

// Limite de simulacion rapida buscando una ronda con el gimmick activo.
static const float SEGUNDOS_SIMULADOS_MAXIMO = 1800.0f;

// Una imagen con menos de este porcentaje de pixeles no oscuros se da
// por negra (la toma fallo).
static const float PORCENTAJE_MINIMO_CLAROS = 2.0f;


// Los colores de la seleccion de personajes (UI/SeleccionPersonajes.cpp):
// las fichas se distinguen como en una partida con humanos.
static const Color COLORES_JUGADORES[MAX_PARTICIPANTES] = { RED, BLUE, GREEN, GOLD };


// Los bots no leen entrada; si alguno no lo fuera, no pulsa nada (el
// teclado real no debe colarse en la captura).
static InputSeleccionParticipante EntradaVacia(const Participante&)
{
    return InputSeleccionParticipante{};
}


//==================================================
// CAMARA
//==================================================

// Misma formula que el final de ActualizarCamaraTablero
// (Gameplay/PartidaTablero.cpp): foco, desfase de la definicion escalado
// por el zoom y giro leve segun x. `elevacion` escala la altura del
// desfase (1 = la de partida).
static Camera3D CamaraTablero(
    const DefinicionTablero& def,
    Vector3 foco,
    float zoom,
    float elevacion = 1.0f
)
{
    Vector3 desfase =
    {
        def.camaraPosicion.x - def.camaraObjetivo.x,
        (def.camaraPosicion.y - def.camaraObjetivo.y) * elevacion,
        def.camaraPosicion.z - def.camaraObjetivo.z
    };

    float giro = foco.x * 0.003f;
    float c = std::cos(giro);
    float s = std::sin(giro);

    Camera3D camara{};
    camara.target = foco;
    camara.position =
    {
        foco.x + (desfase.x * c + desfase.z * s) * zoom,
        foco.y + desfase.y * zoom,
        foco.z + (desfase.z * c - desfase.x * s) * zoom
    };
    camara.up = { 0.0f, 1.0f, 0.0f };
    camara.fovy = def.camaraFovy;
    camara.projection = CAMERA_PERSPECTIVE;

    return camara;
}


struct CajaCasillas
{
    float minX = 1e9f, maxX = -1e9f;
    float minZ = 1e9f, maxZ = -1e9f;

    void Incluir(float x, float z)
    {
        if (x < minX) minX = x;
        if (x > maxX) maxX = x;
        if (z < minZ) minZ = z;
        if (z > maxZ) maxZ = z;
    }
};


static CajaCasillas CajaTablero(const Tablero& tablero)
{
    CajaCasillas caja;

    for (int i = 0; i < tablero.cantidadCasillas; i++)
    {
        caja.Incluir(tablero.casillas[i].posicion.x, tablero.casillas[i].posicion.z);
    }

    return caja;
}


// Casilla `pasos` mas adelante siguiendo siempre la primera conexion.
static const Casilla* AvanzarCadena(const Tablero& tablero, int casilla, int pasos)
{
    const Casilla* actual = tablero.ObtenerCasilla(casilla);

    for (int i = 0; i < pasos && actual != nullptr && actual->cantidadConexiones > 0; i++)
    {
        actual = tablero.ObtenerCasilla(actual->conexiones[0].destino);
    }

    return actual;
}


//==================================================
// MINIJUEGO FALSO
//==================================================

// Igual que Tests/SimularTableros.cpp: en un duelo solo juegan retador y
// rival; el resultado (ganador al azar) se aplica y se continua.
static void ResolverMinijuegoFalso(PartidaTablero& partida, const Participante participantes[])
{
    bool duelo = partida.EsDueloPendiente();
    Participante jugando[MAX_PARTICIPANTES];

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        jugando[i] = participantes[i];

        if (duelo)
        {
            jugando[i].activo =
                participantes[i].activo &&
                (i == partida.duelo.retador || i == partida.duelo.rival);
        }
    }

    ResultadoMinijuego resultado;
    InicializarResultadoMinijuego(resultado, jugando, FORMATO_MINIJUEGO_INDIVIDUAL);
    resultado.estado = RESULTADO_MINIJUEGO_FINALIZADO;

    int activos[MAX_PARTICIPANTES];
    int cantidad = ObtenerIndicesParticipantesActivos(jugando, activos, MAX_PARTICIPANTES);

    if (cantidad > 0)
    {
        resultado.desenlace = DESENLACE_CON_GANADOR;
        int ganador = activos[GetRandomValue(0, cantidad - 1)];

        for (int i = 0; i < cantidad; i++)
        {
            resultado.participantes[activos[i]].posicionFinal = activos[i] == ganador ? 1 : 2;
        }
    }

    if (ValidarResultadoMinijuego(resultado, jugando))
    {
        partida.AplicarResultadoMinijuego(resultado);
    }

    partida.ContinuarTrasMinijuego();
}


// Un paso de partida como en ESTADO_PARTIDA de Core/Juego.cpp; la ronda de
// minijuego se resuelve al instante.
static void ActualizarPartida(PartidaTablero& partida, const Participante participantes[], float dt)
{
    if (partida.fase == FASE_PARTIDA_TABLERO_TERMINADA)
    {
        return;
    }

    partida.Actualizar(dt);

    if (partida.SolicitaSalida())
    {
        partida.salidaSolicitada = false;
    }

    if (partida.SolicitaMinijuego())
    {
        ResolverMinijuegoFalso(partida, participantes);
    }
}


//==================================================
// PARTIDA
//==================================================

static PartidaTablero partida;
static Participante participantes[MAX_PARTICIPANTES];


// Partida nueva con 4 bots, como PrepararPartidaTablero de Core/Juego.cpp
// (sin audio: los sonidos no aparecen en la captura).
static void PrepararPartida(IdTablero id, unsigned int semilla)
{
    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        participantes[i] = Participante{};
        participantes[i].numeroJugador = i + 1;
    }

    CompletarParticipantesConBots(participantes, MAX_PARTICIPANTES);

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        participantes[i].color = COLORES_JUGADORES[i];
    }

    SetRandomSeed(semilla);

    partida = PartidaTablero{};
    partida.lectorEntrada = EntradaVacia;
    partida.Inicializar(participantes, MAX_PARTICIPANTES, nullptr, id);
    partida.cantidadRondas = partida.definicion->cantidadRondas;
}


// Avanza la partida sin dibujar hasta que `listo` se cumple (o se agota el
// tiempo). Devuelve true si se cumplio.
template <typename Condicion>
static bool SimularHasta(float segundosMaximos, Condicion listo)
{
    for (float t = 0.0f; t < segundosMaximos; t += DT_SIMULACION)
    {
        if (listo())
        {
            return true;
        }

        if (partida.fase == FASE_PARTIDA_TABLERO_TERMINADA)
        {
            return false;
        }

        ActualizarPartida(partida, participantes, DT_SIMULACION);
    }

    return listo();
}


static bool FichasRepartidas()
{
    int primera = -1;
    int distintas = 0;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!partida.jugadores[i].participa)
        {
            continue;
        }

        if (primera < 0)
        {
            primera = partida.jugadores[i].casillaActual;
        }
        else if (partida.jugadores[i].casillaActual != primera)
        {
            distintas++;
        }
    }

    return distintas >= 2;
}


//==================================================
// CAPTURA
//==================================================

// "Isla Arboleda" -> "isla_arboleda"; las vocales acentuadas UTF-8 se
// reducen a su vocal y el resto de simbolos se descarta.
static void NombreArchivoTablero(const char* nombre, char* destino, int capacidad)
{
    int n = 0;
    bool separar = false;

    for (const unsigned char* p = (const unsigned char*)nombre; *p != '\0' && n < capacidad - 2; p++)
    {
        char c = 0;

        if (std::isalnum(*p))
        {
            c = (char)std::tolower(*p);
        }
        else if (*p == 0xC3 && p[1] != '\0')
        {
            p++;
            switch (*p | 0x20)
            {
                case 0xA1: case 0xA0: case 0xA4: c = 'a'; break;
                case 0xA9: case 0xA8: case 0xAB: c = 'e'; break;
                case 0xAD: case 0xAC: case 0xAF: c = 'i'; break;
                case 0xB3: case 0xB2: case 0xB6: c = 'o'; break;
                case 0xBA: case 0xB9: case 0xBC: c = 'u'; break;
                case 0xB1: c = 'n'; break;
                default: break;
            }
        }
        else
        {
            separar = n > 0;
        }

        if (c != 0)
        {
            if (separar)
            {
                destino[n++] = '_';
                separar = false;
            }

            destino[n++] = c;
        }
    }

    destino[n] = '\0';
}


// Porcentaje de pixeles con algun canal por encima de 24 (detecta una
// captura negra o vacia).
static float PorcentajeClaros(const Image& imagen)
{
    Color* pixeles = LoadImageColors(imagen);
    int total = imagen.width * imagen.height;
    int claros = 0;

    for (int i = 0; i < total; i++)
    {
        if (pixeles[i].r > 24 || pixeles[i].g > 24 || pixeles[i].b > 24)
        {
            claros++;
        }
    }

    UnloadImageColors(pixeles);

    return total > 0 ? 100.0f * (float)claros / (float)total : 0.0f;
}


// Debe llamarse con el frame ya dibujado, antes de EndDrawing.
static bool GuardarCaptura(const char* ruta)
{
    rlDrawRenderBatchActive();

    Image imagen = LoadImageFromScreen();
    float claros = PorcentajeClaros(imagen);
    bool guardada = ExportImage(imagen, ruta);
    int ancho = imagen.width;
    int alto = imagen.height;
    UnloadImage(imagen);

    if (!guardada)
    {
        std::printf("FALLA: no se pudo guardar %s\n", ruta);
        return false;
    }

    if (claros < PORCENTAJE_MINIMO_CLAROS)
    {
        std::printf("FALLA: %s sale casi negra (%.1f%% de pixeles claros)\n", ruta, claros);
        return false;
    }

    if (ancho != GetScreenWidth() || alto != GetScreenHeight())
    {
        std::printf("AVISO: %s mide %dx%d (ventana %dx%d)\n", ruta, ancho, alto, GetScreenWidth(), GetScreenHeight());
    }

    return true;
}


// Frame como ESTADO_PARTIDA de Juego::Dibujar, con config.mostrarFPS.
static void DibujarFrame()
{
    partida.Dibujar();

    const char* texto = TextFormat("FPS: %d", GetFPS());

    DrawText(
        texto,
        GetScreenWidth() - MeasureText(texto, 20) - 20,
        20,
        20,
        DARKGREEN
    );
}


//==================================================
// TOMAS
//==================================================

struct Captura
{
    const char* carpeta = CARPETA_CAPTURAS;
    char tablero[64] = {};
    int fallas = 0;
};


// Juega en tiempo real con la camara de partida (camara == nullptr) o con
// una camara fija escrita sobre partida.camara despues de cada Actualizar.
// Asienta, mide el FPS promedio y el peor frame, y guarda la captura en un
// frame extra que no entra en la medicion (codificar el PNG lo sesgaria).
static bool EjecutarToma(
    Captura& captura,
    const char* toma,
    const Camera3D* camara,
    float segundosAsentar = SEGUNDOS_ASENTAR
)
{
    char ruta[512];
    std::snprintf(ruta, sizeof(ruta), "%s/%s_%s_%dx%d.png",
        captura.carpeta, captura.tablero, toma, GetScreenWidth(), GetScreenHeight());

    int frame = 0;
    int framesMedidos = 0;
    double inicio = GetTime();
    double inicioMedicion = -1.0;
    double inicioFrame = inicio;
    double peorFrame = 0.0;

    while (!WindowShouldClose())
    {
        // Cada frame se mide de inicio a inicio: incluye actualizar, dibujar
        // y presentar, igual que el bucle del juego.
        double ahora = GetTime();

        if (inicioMedicion >= 0.0 && frame > 0)
        {
            double duracion = ahora - inicioFrame;

            if (duracion > peorFrame)
            {
                peorFrame = duracion;
            }

            framesMedidos++;
        }

        inicioFrame = ahora;

        if (
            inicioMedicion < 0.0 &&
            ahora - inicio >= segundosAsentar &&
            frame >= FRAMES_MINIMOS_ASENTAR
        )
        {
            inicioMedicion = ahora;
            framesMedidos = 0;
            peorFrame = 0.0;
        }

        // Pasados los segundos de medicion, este frame es el de la captura y
        // la medicion termina en su inicio.
        bool ultimo = inicioMedicion >= 0.0 && ahora - inicioMedicion >= SEGUNDOS_MEDICION;
        double segundosMedidos = ahora - inicioMedicion;

        float dt = GetFrameTime();

        if (dt > 0.1f || frame == 0)
        {
            dt = frame == 0 ? 1.0f / 60.0f : 0.1f;
        }

        ActualizarEstiloUI(ObtenerEstiloUI(), dt);
        ActualizarPartida(partida, participantes, dt);

        if (camara != nullptr)
        {
            partida.camara = *camara;
        }

        BeginDrawing();
        DibujarFrame();

        bool guardada = true;

        if (ultimo)
        {
            guardada = GuardarCaptura(ruta);
        }

        EndDrawing();

        frame++;

        if (ultimo)
        {
            float fps = segundosMedidos > 0.0 ? (float)(framesMedidos / segundosMedidos) : 0.0f;

            std::printf("%s %s %.1f %.1f (%d frames en %.2f s)\n",
                captura.tablero, toma, fps, peorFrame * 1000.0, framesMedidos, segundosMedidos);
            std::printf("    -> %s\n", ruta);
            std::fflush(stdout);

            if (!guardada)
            {
                captura.fallas++;
            }

            return guardada;
        }
    }

    std::printf("FALLA: ventana cerrada durante %s %s\n", captura.tablero, toma);
    captura.fallas++;
    return false;
}


static float ZoomVistaGeneral(const DefinicionTablero& def)
{
    return def.camaraZoomGeneral > 1.05f ? def.camaraZoomGeneral : 1.0f;
}


// Encuadre de ActualizarCamaraTablero en FASE_PARTIDA_TABLERO_ELIGIENDO_RUTA:
// la casilla y unos pasos de cada rama. Devuelve false si no hay bifurcacion.
static bool CamaraBifurcacion(const DefinicionTablero& def, Camera3D& camara, int& casillaElegida)
{
    const Tablero& tablero = partida.tablero;
    casillaElegida = -1;

    // La bifurcacion mas cercana al centro del tablero.
    float mejor = 1e9f;

    for (int i = 0; i < tablero.cantidadCasillas; i++)
    {
        if (tablero.casillas[i].cantidadConexiones < 2)
        {
            continue;
        }

        float dx = tablero.casillas[i].posicion.x - def.camaraObjetivo.x;
        float dz = tablero.casillas[i].posicion.z - def.camaraObjetivo.z;
        float d = dx * dx + dz * dz;

        if (d < mejor)
        {
            mejor = d;
            casillaElegida = i;
        }
    }

    if (casillaElegida < 0)
    {
        return false;
    }

    const Casilla& casilla = tablero.casillas[casillaElegida];
    CajaCasillas caja;
    caja.Incluir(casilla.posicion.x, casilla.posicion.z + 1.6f);

    for (int o = 0; o < casilla.cantidadConexiones; o++)
    {
        int destino = casilla.conexiones[o].destino;
        const Casilla* cerca = AvanzarCadena(tablero, destino, 0);
        const Casilla* lejos = AvanzarCadena(tablero, destino, 6);

        if (cerca != nullptr) caja.Incluir(cerca->posicion.x, cerca->posicion.z);
        if (lejos != nullptr) caja.Incluir(lejos->posicion.x, lejos->posicion.z);
    }

    Vector3 foco =
    {
        (caja.minX + caja.maxX) * 0.5f,
        0.4f,
        (caja.minZ + caja.maxZ) * 0.5f + 1.2f
    };

    float mitad = (caja.maxX - caja.minX) * 0.5f;
    float mitadZ = (caja.maxZ - caja.minZ) * 0.75f;

    if (mitadZ > mitad) mitad = mitadZ;

    float zoom = mitad / 8.0f;

    if (zoom < 1.2f) zoom = 1.2f;
    if (zoom > 2.0f) zoom = 2.0f;

    camara = CamaraTablero(def, foco, zoom);
    return true;
}


// Mas alla de la casilla mas lejana en la direccion de la mirada, con la
// camara mas baja para que se vea el horizonte del escenario.
static Camera3D CamaraFondo(const DefinicionTablero& def)
{
    CajaCasillas caja = CajaTablero(partida.tablero);

    float mx = def.camaraObjetivo.x - def.camaraPosicion.x;
    float mz = def.camaraObjetivo.z - def.camaraPosicion.z;
    float largo = std::sqrt(mx * mx + mz * mz);

    if (largo < 0.001f)
    {
        mx = 0.0f;
        mz = -1.0f;
    }
    else
    {
        mx /= largo;
        mz /= largo;
    }

    float cx = (caja.minX + caja.maxX) * 0.5f;
    float cz = (caja.minZ + caja.maxZ) * 0.5f;
    float alcance = 0.0f;

    for (int i = 0; i < partida.tablero.cantidadCasillas; i++)
    {
        const Vector3& p = partida.tablero.casillas[i].posicion;
        float proyeccion = (p.x - cx) * mx + (p.z - cz) * mz;

        if (proyeccion > alcance) alcance = proyeccion;
    }

    Vector3 foco = { cx + mx * (alcance + 3.0f), 0.0f, cz + mz * (alcance + 3.0f) };

    return CamaraTablero(def, foco, 1.7f, 0.5f);
}


// Landmark del gimmick como lo enfoca la partida (zoom 1.08); sin landmark,
// la casilla de peligro o el centro del tablero.
static Camera3D CamaraGimmick(const DefinicionTablero& def)
{
    if (def.tieneLandmark)
    {
        return CamaraTablero(def, def.camaraLandmark, 1.08f);
    }

    const Casilla* peligro = partida.tablero.ObtenerCasilla(partida.casillaPeligro);

    if (peligro != nullptr)
    {
        Vector3 foco = { peligro->posicion.x, 0.4f, peligro->posicion.z + 1.6f };
        return CamaraTablero(def, foco, 0.9f);
    }

    return CamaraTablero(def, def.camaraObjetivo, 1.0f);
}


//==================================================
// UN TABLERO
//==================================================

static void CapturarTablero(IdTablero id, Captura& captura)
{
    const DefinicionTablero& def = ObtenerDefinicionTablero(id);
    unsigned int semilla = 7000u + 31u * (unsigned int)id;

    NombreArchivoTablero(def.nombre, captura.tablero, (int)sizeof(captura.tablero));

    if (captura.tablero[0] == '\0')
    {
        std::snprintf(captura.tablero, sizeof(captura.tablero), "tablero_%d", (int)id);
    }

    std::printf("Tablero %d: %s\n", (int)id, def.nombre);

    PrepararPartida(id, semilla);

    if (!partida.inicializado || !partida.tablero.recorridoValido)
    {
        std::printf("FALLA: %s no inicializa (recorrido invalido)\n", captura.tablero);
        captura.fallas++;
        return;
    }

    // 1. Vista general del comienzo de la ronda 1.
    Camera3D general = CamaraTablero(def, def.camaraObjetivo, ZoomVistaGeneral(def));
    EjecutarToma(captura, "general", &general);

    // 2. Recorrido: unos turnos simulados y la camara de la propia partida
    //    al empezar un turno (fuera de la vista general de ronda).
    PrepararPartida(id, semilla);
    SimularHasta(SEGUNDOS_SIMULADOS_RECORRIDO, [] { return false; });
    SimularHasta(SEGUNDOS_SIMULADOS_MAXIMO, []
    {
        return
            partida.fase == FASE_PARTIDA_TABLERO_ESPERANDO_DADO &&
            partida.camaraTiempoVista <= 0.0f &&
            partida.camaraTiempoTrofeo <= 0.0f &&
            FichasRepartidas();
    });
    EjecutarToma(captura, "recorrido", nullptr);

    // 3. Bifurcacion (si el tablero tiene).
    Camera3D bifurcacion{};
    int casillaBifurcacion = -1;

    if (CamaraBifurcacion(def, bifurcacion, casillaBifurcacion))
    {
        EjecutarToma(captura, "bifurcacion", &bifurcacion);
    }
    else
    {
        std::printf("    (sin bifurcaciones: se omite la toma)\n");
    }

    // 4. Gimmick: se juega hasta una ronda con el gimmick activo.
    PrepararPartida(id, semilla);

    bool activo = SimularHasta(SEGUNDOS_SIMULADOS_MAXIMO, []
    {
        return
            partida.gimmickActivo &&
            partida.fase != FASE_PARTIDA_TABLERO_FIN_RONDA &&
            partida.fase != FASE_PARTIDA_TABLERO_TERMINADA;
    });

    if (!activo)
    {
        std::printf("    (el gimmick no se activo en la simulacion: se fuerza gimmickActivo)\n");
        PrepararPartida(id, semilla);
        partida.gimmickActivo = true;
    }
    else
    {
        std::printf("    (gimmick activo en la ronda %d)\n", partida.rondaActual);
    }

    Camera3D gimmick = CamaraGimmick(def);
    EjecutarToma(captura, "gimmick", &gimmick, SEGUNDOS_ASENTAR + 1.5f);

    // 5. Fondo del escenario.
    Camera3D fondo = CamaraFondo(def);
    EjecutarToma(captura, "fondo", &fondo);
}


//==================================================
// PRINCIPAL
//==================================================

static bool CoincideTablero(const char* filtro, int indice, const char* nombre)
{
    if (filtro == nullptr || filtro[0] == '\0')
    {
        return true;
    }

    char* fin = nullptr;
    long numero = std::strtol(filtro, &fin, 10);

    if (fin != filtro && *fin == '\0')
    {
        return numero == indice;
    }

    // Subcadena sin distinguir mayusculas.
    for (const char* p = nombre; *p != '\0'; p++)
    {
        int k = 0;

        while (
            filtro[k] != '\0' && p[k] != '\0' &&
            std::tolower((unsigned char)p[k]) == std::tolower((unsigned char)filtro[k])
        )
        {
            k++;
        }

        if (filtro[k] == '\0')
        {
            return true;
        }
    }

    return false;
}


static void Uso(const char* programa)
{
    std::fprintf(stderr,
        "Uso: %s [--tablero N|nombre] [--resolucion ANCHO ALTO] [--calidad baja|media|alta]\n"
        "          [--fps N] [--salida CARPETA]\n", programa);
}


int main(int argc, char** argv)
{
    const char* filtro = nullptr;
    int ancho = 1280;
    int alto = 720;
    int fpsObjetivo = 0;
    NivelCalidadGrafica calidad = CALIDAD_ALTA;
    const char* carpeta = CARPETA_CAPTURAS;

    for (int i = 1; i < argc; i++)
    {
        if (std::strcmp(argv[i], "--tablero") == 0 && i + 1 < argc)
        {
            filtro = argv[++i];
        }
        else if (std::strcmp(argv[i], "--resolucion") == 0 && i + 2 < argc)
        {
            ancho = std::atoi(argv[++i]);
            alto = std::atoi(argv[++i]);
        }
        else if (std::strcmp(argv[i], "--calidad") == 0 && i + 1 < argc)
        {
            const char* nivel = argv[++i];

            if (std::strcmp(nivel, "baja") == 0) calidad = CALIDAD_BAJA;
            else if (std::strcmp(nivel, "media") == 0) calidad = CALIDAD_MEDIA;
            else if (std::strcmp(nivel, "alta") == 0) calidad = CALIDAD_ALTA;
            else
            {
                Uso(argv[0]);
                return 2;
            }
        }
        else if (std::strcmp(argv[i], "--fps") == 0 && i + 1 < argc)
        {
            fpsObjetivo = std::atoi(argv[++i]);
        }
        else if (std::strcmp(argv[i], "--salida") == 0 && i + 1 < argc)
        {
            carpeta = argv[++i];
        }
        else
        {
            Uso(argv[0]);
            return 2;
        }
    }

    if (ancho < 320 || alto < 240)
    {
        Uso(argv[0]);
        return 2;
    }

    SetTraceLogLevel(LOG_WARNING);

    // Sin borde: 1920x1080 cabe entera en un monitor de 1080p.
    SetConfigFlags(FLAG_WINDOW_UNDECORATED);
    InitWindow(ancho, alto, "Capturas de tableros");

    if (!IsWindowReady())
    {
        std::printf("FALLA: no se pudo abrir la ventana\n");
        return 1;
    }

    int monitor = GetCurrentMonitor();
    Vector2 origen = GetMonitorPosition(monitor);
    int sobraX = GetMonitorWidth(monitor) - ancho;
    int sobraY = GetMonitorHeight(monitor) - alto;
    SetWindowPosition(
        (int)origen.x + (sobraX > 0 ? sobraX / 2 : 0),
        (int)origen.y + (sobraY > 0 ? sobraY / 2 : 0)
    );

    SetExitKey(KEY_NULL);
    SetTargetFPS(fpsObjetivo);
    EstablecerOpcionesCalidadGrafica(CrearPresetCalidadGrafica(calidad));

    if (GetScreenWidth() != ancho || GetScreenHeight() != alto)
    {
        std::printf("AVISO: la ventana quedo en %dx%d (pedida %dx%d)\n",
            GetScreenWidth(), GetScreenHeight(), ancho, alto);
    }

    if (!DirectoryExists(carpeta))
    {
        MakeDirectory(carpeta);
    }

    // Carga perezosa de las texturas del HUD, igual que el primer frame del juego.
    ObtenerEstiloUI();

    std::printf("Capturas %dx%d, calidad %d, FPS objetivo %d (0 = sin limite)\n",
        GetScreenWidth(), GetScreenHeight(), (int)calidad, fpsObjetivo);
    std::printf("tablero toma fps_promedio peor_frame_ms\n");

    int tableros = 0;
    int fallas = 0;

    for (int t = 0; t < ObtenerCantidadTableros(); t++)
    {
        const DefinicionTablero& def = ObtenerDefinicionTablero((IdTablero)t);
        const char* nombre = def.nombre != nullptr ? def.nombre : "";

        if (!CoincideTablero(filtro, t, nombre))
        {
            continue;
        }

        tableros++;

        Captura captura;
        captura.carpeta = carpeta;
        CapturarTablero((IdTablero)t, captura);
        fallas += captura.fallas;

        if (WindowShouldClose())
        {
            break;
        }
    }

    partida = PartidaTablero{};
    DescargarVistaPreviaTableros();
    DescargarEstiloUIGlobal();
    CloseWindow();

    if (tableros == 0)
    {
        std::printf("FALLA: ningun tablero coincide con '%s'\n", filtro != nullptr ? filtro : "");
        return 3;
    }

    if (fallas > 0)
    {
        std::printf("FALLA: %d toma(s) con problemas\n", fallas);
        return 1;
    }

    std::printf("OK: %d tablero(s) capturados\n", tableros);
    return 0;
}
