// Genera las miniaturas del menu de minijuegos con capturas reales de
// gameplay (ventana real, Zona de Pruebas en modo catalogo, igual que el
// menu). Cada minijuego se juega con bots (4 si los admite, si no 2) y,
// en un instante ALEATORIO de la fase de juego (nunca la cuenta regresiva
// ni la pantalla final), se toma la escena justo antes de que se dibuje el
// HUD comun (ver EstablecerFuncionAntesHudMinijuego): la captura sale sin
// tendedero, reloj ni carteles. Se reduce a ANCHO x ALTO, sin canal alfa
// y con colores 5-6-5 (PNG de ~40-70 KB), y se guarda en Assets/UI/Miniaturas/<minijuego>.png (la ruta que lee el
// menu, ObtenerRutaMiniaturaMinijuego).
//
// Si la ronda termina antes del instante elegido, o la imagen sale casi
// negra o plana (fundido, pantalla vacia), se vuelve a jugar con otro
// instante, hasta INTENTOS veces.
//
// Uso: CapturarMiniaturas.exe [--minijuego N] [--semilla N] [--salida carpeta]
// Normalmente se lanza con Tests/CapturarMiniaturas.ps1, desde la raiz del
// repo (rutas de Assets relativas).
// Codigo de salida: cantidad de minijuegos sin miniatura valida.
#include "Core/Juego.h"
#include "Minigames/HudMinijuego.h"
#include "Systems/CalidadGrafica.h"
#include "UI/MiniaturasMinijuegos.h"

#include "rlgl.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <random>

static Juego juego;

static const float DT = 1.0f / 60.0f;

// Tamano final de cada miniatura (16:9, como la ventana de captura).
static const int ANCHO_MINIATURA = 320;
static const int ALTO_MINIATURA = 180;

static const int INTENTOS = 4;

// Sin ver el HUD o sin llegar a jugar en este tiempo, el intento falla.
static const float TOPE_ESPERA_SEGUNDOS = 40.0f;

// Imagen aceptable: brillo medio y contraste minimos (0..255).
static const float BRILLO_MINIMO = 28.0f;
static const float CONTRASTE_MINIMO = 14.0f;


//==================================================
// CAPTURA ANTES DEL HUD
//==================================================

// Lo que ve y hace la funcion que se engancha antes del HUD.
struct EstadoCaptura
{
    bool hudVisto = false;
    FaseHudMinijuego fase = HUD_MINIJUEGO_CUENTA;
    RelojHudMinijuego reloj = HUD_RELOJ_CUENTA_ATRAS;
    float tiempoTotal = 0.0f;

    bool pedida = false;            // capturar en el proximo frame de juego
    bool hecha = false;
    float brillo = 0.0f;
    float contraste = 0.0f;
    Image imagen = {};              // ya reducida; la libera quien la usa
};

static EstadoCaptura captura;


// Brillo medio y desvio del brillo de la imagen reducida.
static void MedirImagen(
    const Image& imagen,
    float& brillo,
    float& contraste
)
{
    Color* colores = LoadImageColors(imagen);
    const int cantidad = imagen.width * imagen.height;

    double suma = 0.0;
    double sumaCuadrados = 0.0;

    for (int i = 0; i < cantidad; i++)
    {
        double luz = 0.299 * colores[i].r + 0.587 * colores[i].g + 0.114 * colores[i].b;
        suma += luz;
        sumaCuadrados += luz * luz;
    }

    UnloadImageColors(colores);

    double media = cantidad > 0 ? suma / cantidad : 0.0;
    double varianza = cantidad > 0 ? sumaCuadrados / cantidad - media * media : 0.0;

    brillo = (float)media;
    contraste = (float)std::sqrt(varianza > 0.0 ? varianza : 0.0);
}


static void AntesDelHud(
    const DatosHudMinijuego& datos
)
{
    captura.hudVisto = true;
    captura.fase = datos.fase;
    captura.reloj = datos.reloj;
    captura.tiempoTotal = datos.tiempoTotal;

    if (!captura.pedida || datos.fase != HUD_MINIJUEGO_JUGANDO)
    {
        return;
    }

    // Lo pendiente del lote tiene que llegar a la pantalla antes de leerla.
    rlDrawRenderBatchActive();

    Image imagen = LoadImageFromScreen();
    ImageFormat(&imagen, PIXELFORMAT_UNCOMPRESSED_R8G8B8);
    ImageResize(&imagen, ANCHO_MINIATURA, ALTO_MINIATURA);

    // Colores a 5-6-5 bits (sin tramado): a este tamano no se nota y el PNG
    // queda en ~60 % del peso (menos colores distintos comprimen mejor).
    ImageFormat(&imagen, PIXELFORMAT_UNCOMPRESSED_R5G6B5);
    ImageFormat(&imagen, PIXELFORMAT_UNCOMPRESSED_R8G8B8);

    if (captura.imagen.data != nullptr)
    {
        UnloadImage(captura.imagen);
    }

    captura.imagen = imagen;
    MedirImagen(captura.imagen, captura.brillo, captura.contraste);
    captura.pedida = false;
    captura.hecha = true;
}


//==================================================
// FLUJO
//==================================================

static void Frame()
{
    juego.Actualizar(DT);

    BeginDrawing();
    juego.Dibujar();
    EndDrawing();
}


static void PrepararParticipantes(
    int cantidad
)
{
    const Color colores[4] = { { 230, 60, 60, 255 }, { 60, 120, 230, 255 }, { 60, 190, 90, 255 }, { 240, 200, 50, 255 } };

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        Participante& p = juego.participantes[i];
        p = Participante{};
        p.activo = i < cantidad;
        p.conectado = i < cantidad;
        p.esBot = true;
        p.numeroJugador = i + 1;
        p.personajeSeleccionado = i % 4;
        p.color = colores[i % 4];
    }
}


// Segundos de juego (sin contar la cuenta regresiva) hasta la captura.
// Con reloj se elige dentro del primer 20-65 % de la ronda, donde suele
// haber todos en pie y accion; sin reloj, entre 3 y 9 segundos.
static float ElegirInstante(
    std::mt19937& azar,
    int intento
)
{
    float minimo = 3.0f;
    float maximo = 9.0f;

    if (captura.reloj == HUD_RELOJ_CUENTA_ATRAS && captura.tiempoTotal > 6.0f)
    {
        minimo = captura.tiempoTotal * 0.20f;
        maximo = captura.tiempoTotal * 0.65f;
        if (minimo < 2.5f) minimo = 2.5f;
        if (maximo > 25.0f) maximo = 25.0f;
        if (maximo < minimo) maximo = minimo;
    }

    // Si la ronda ya termino antes otras veces, cada intento apunta antes.
    for (int i = 0; i < intento; i++)
    {
        minimo *= 0.5f;
        maximo *= 0.5f;
    }

    if (minimo < 1.0f) minimo = 1.0f;
    if (maximo < minimo) maximo = minimo;

    std::uniform_real_distribution<float> reparto(minimo, maximo);
    return reparto(azar);
}


// Un intento: juega el minijuego hasta el instante elegido y captura.
// Devuelve true si quedo una imagen en captura.imagen.
static bool Intentar(
    IdMinijuego id,
    int cantidad,
    std::mt19937& azar,
    int intento,
    float& segundosJuego
)
{
    PrepararParticipantes(cantidad);
    juego.zonaPruebas.Inicializar(juego.participantes, MAX_PARTICIPANTES, &juego.audio);
    juego.zonaPruebas.modoCatalogo = true;
    juego.zonaPruebas.CambiarMinijuego(id);
    juego.estado = ESTADO_ZONA_PRUEBAS;

    captura.hudVisto = false;
    captura.fase = HUD_MINIJUEGO_CUENTA;
    captura.pedida = false;
    captura.hecha = false;

    float espera = 0.0f;
    float instante = -1.0f;
    segundosJuego = 0.0f;

    while (espera < TOPE_ESPERA_SEGUNDOS + 30.0f)
    {
        Frame();

        if (captura.hecha)
        {
            return true;
        }

        const ResultadoMinijuego* resultado = juego.zonaPruebas.ObtenerResultadoMinijuego();

        if (
            resultado == nullptr ||
            resultado->estado != RESULTADO_MINIJUEGO_EN_CURSO ||
            (captura.hudVisto && captura.fase == HUD_MINIJUEGO_FIN)
        )
        {
            // Termino antes del instante elegido.
            return false;
        }

        if (captura.hudVisto && captura.fase == HUD_MINIJUEGO_JUGANDO)
        {
            // El primer frame de juego ya trae el tiempo total de la ronda.
            if (instante < 0.0f)
            {
                instante = ElegirInstante(azar, intento);
            }

            segundosJuego += DT;

            if (segundosJuego >= instante)
            {
                captura.pedida = true;
            }
        }
        else
        {
            espera += DT;

            if (espera > TOPE_ESPERA_SEGUNDOS)
            {
                return false;
            }
        }
    }

    return false;
}


int main(int argc, char** argv)
{
    int soloMinijuego = -1;
    unsigned int semilla = (unsigned int)std::time(nullptr);
    const char* salida = "Assets/UI/Miniaturas";

    for (int a = 1; a < argc; a++)
    {
        if (!std::strcmp(argv[a], "--minijuego") && a + 1 < argc) soloMinijuego = std::atoi(argv[++a]);
        else if (!std::strcmp(argv[a], "--semilla") && a + 1 < argc) semilla = (unsigned int)std::strtoul(argv[++a], nullptr, 10);
        else if (!std::strcmp(argv[a], "--salida") && a + 1 < argc) salida = argv[++a];
    }

    juego.anchoVentanaForzada = 1280;
    juego.altoVentanaForzada = 720;
    juego.Inicializar();
    SetTargetFPS(0);
    ClearWindowState(FLAG_VSYNC_HINT);  // sin esperar al monitor: va mas rapido
    SetExitKey(KEY_NULL);
    SetTraceLogLevel(LOG_WARNING);
    SetMasterVolume(0.0f);
    EstablecerOpcionesCalidadGrafica(CrearPresetCalidadGrafica(CALIDAD_ALTA));
    SetRandomSeed(semilla);

    if (!DirectoryExists(salida))
    {
        MakeDirectory(salida);
    }

    std::mt19937 azar(semilla);
    EstablecerFuncionAntesHudMinijuego(AntesDelHud);

    for (int f = 0; f < 60; f++) Frame();

    std::printf("Semilla %u, miniaturas %dx%d en %s\n", semilla, ANCHO_MINIATURA, ALTO_MINIATURA, salida);
    std::printf("id  minijuego                      jug int  seg_juego brillo contraste   KB  archivo\n");

    int fallidos = 0;
    long bytesTotales = 0;

    for (int indice = 0; indice < CANTIDAD_MINIJUEGOS; indice++)
    {
        IdMinijuego id = ObtenerIdMinijuegoPorIndice(indice);

        if (soloMinijuego >= 0 && (int)id != soloMinijuego) continue;

        const DatosMinijuegoCatalogo& datos = ObtenerDatosMinijuego(id);
        const int cantidad = MinijuegoAdmiteCantidadJugadores(id, 4) ? 4 : 2;

        // Mismo nombre que lee el menu, pero dentro de `salida`.
        char ruta[512];
        std::snprintf(ruta, sizeof(ruta), "%s/%s", salida, GetFileName(ObtenerRutaMiniaturaMinijuego(id)));

        bool guardada = false;
        int intento = 0;
        float segundosJuego = 0.0f;

        for (; intento < INTENTOS && !guardada; intento++)
        {
            if (!Intentar(id, cantidad, azar, intento, segundosJuego))
            {
                continue;
            }

            if (captura.brillo < BRILLO_MINIMO || captura.contraste < CONTRASTE_MINIMO)
            {
                continue;
            }

            guardada = ExportImage(captura.imagen, ruta);
        }

        int bytes = guardada ? GetFileLength(ruta) : 0;
        bytesTotales += bytes;

        if (!guardada) fallidos++;

        std::printf(
            "%2d  %-30s %d  %d   %8.1f %6.1f %9.1f %5.1f  %s%s\n",
            (int)id,
            datos.nombre,
            cantidad,
            intento,
            segundosJuego,
            captura.brillo,
            captura.contraste,
            bytes / 1024.0f,
            ruta,
            guardada ? "" : "  <-- SIN MINIATURA"
        );
        std::fflush(stdout);

        if (captura.imagen.data != nullptr)
        {
            UnloadImage(captura.imagen);
            captura.imagen = Image{};
        }
    }

    std::printf("Total: %.1f KB. Minijuegos sin miniatura: %d\n", bytesTotales / 1024.0f, fallidos);

    EstablecerFuncionAntesHudMinijuego(nullptr);
    juego.Descargar();
    CloseWindow();
    return fallidos;
}
