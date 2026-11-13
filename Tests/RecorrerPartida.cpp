// Partida de tablero COMPLETA con el flujo real del juego y 4 bots:
// orden de turno -> tablero -> ruleta -> minijuego real -> resultado ->
// tablero ... -> resultados de la partida. Registra cada cambio de estado,
// los minijuegos jugados y guarda una captura por transicion.
//
// Uso: RecorrerPartida.exe [--tablero N] [--rondas N] [--salida carpeta]
// Codigo de salida: 0 si la partida llega a los resultados finales.
#include "Core/Juego.h"

#include "rlgl.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

static Juego juego;
static const float DT = 1.0f / 60.0f;

static const char* NombreEstado(EstadoJuego e)
{
    switch (e)
    {
        case ESTADO_LOGO: return "LOGO";
        case ESTADO_MENU: return "MENU";
        case ESTADO_ORDEN_TURNO: return "ORDEN_TURNO";
        case ESTADO_INTRO_TABLERO: return "INTRO_TABLERO";
        case ESTADO_PARTIDA: return "PARTIDA (tablero)";
        case ESTADO_RULETA_MINIJUEGO: return "RULETA_MINIJUEGO";
        case ESTADO_MINIJUEGO: return "MINIJUEGO";
        case ESTADO_RESULTADO: return "RESULTADO";
        case ESTADO_RESULTADOS_PARTIDA: return "RESULTADOS_PARTIDA";
        case ESTADO_ZONA_PRUEBAS: return "ZONA_PRUEBAS";
        case ESTADO_INTRO_MINIJUEGO: return "INTRO_MINIJUEGO";
        case ESTADO_CINEMATICA_CAMPEON: return "CINEMATICA_CAMPEON";
        case ESTADO_CINEMATICA_APERTURA: return "CINEMATICA_APERTURA";
        default: return "OTRO";
    }
}

static void Frame(bool dibujar, const char* captura = nullptr)
{
    juego.Actualizar(DT);

    if (!dibujar && captura == nullptr)
    {
        return;
    }

    BeginDrawing();
    juego.Dibujar();

    if (captura != nullptr)
    {
        rlDrawRenderBatchActive();
        Image imagen = LoadImageFromScreen();
        ExportImage(imagen, captura);
        UnloadImage(imagen);
    }

    EndDrawing();
}

int main(int argc, char** argv)
{
    int tablero = 0;
    int rondas = 2;
    const char* salida = "build/capturas/partida";

    for (int a = 1; a < argc; a++)
    {
        if (!std::strcmp(argv[a], "--tablero") && a + 1 < argc) tablero = std::atoi(argv[++a]);
        else if (!std::strcmp(argv[a], "--rondas") && a + 1 < argc) rondas = std::atoi(argv[++a]);
        else if (!std::strcmp(argv[a], "--salida") && a + 1 < argc) salida = argv[++a];
    }

    juego.anchoVentanaForzada = 1280;
    juego.altoVentanaForzada = 720;
    juego.Inicializar();
    SetTargetFPS(0);
    SetExitKey(KEY_NULL);
    SetTraceLogLevel(LOG_WARNING);

    for (int f = 0; f < 60; f++) Frame(true);

    // Cuatro bots con personajes distintos (como tras la seleccion).
    const Color colores[4] = { { 230, 60, 60, 255 }, { 60, 120, 230, 255 }, { 60, 190, 90, 255 }, { 240, 200, 50, 255 } };

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        Participante& p = juego.participantes[i];
        p = Participante{};
        p.activo = i < 4;
        p.conectado = i < 4;
        p.esBot = true;
        p.numeroJugador = i + 1;
        p.personajeSeleccionado = i % 4;
        p.color = colores[i % 4];
    }

    juego.cantidadParticipantes = MAX_PARTICIPANTES;
    juego.partidaTablero.Inicializar(juego.participantes, MAX_PARTICIPANTES, &juego.audio, (IdTablero)tablero);
    juego.partidaTablero.cantidadRondas = rondas;
    juego.rondaEsDuelo = false;
    juego.zonaPruebas.modoCatalogo = false;
    juego.zonaPruebas.modoTablero = false;
    juego.ordenTurno.Inicializar(juego.partidaTablero);
    juego.estado = ESTADO_ORDEN_TURNO;

    EstadoJuego anterior = (EstadoJuego)-1;
    int transiciones = 0;
    int minijuegos = 0;
    float segundos = 0.0f;
    float enEstado = 0.0f;
    const float TOPE = 60.0f * 30.0f;
    bool llegoAlFinal = false;

    while (segundos < TOPE)
    {
        bool cambio = juego.estado != anterior;

        if (cambio)
        {
            char ruta[512];
            std::snprintf(ruta, sizeof(ruta), "%s/t%d_%02d_%s.png", salida, tablero, transiciones, NombreEstado(juego.estado));
            for (char* c = ruta; *c; c++) if (*c == ' ' || *c == '(' || *c == ')') *c = '_';

            if (juego.estado == ESTADO_MINIJUEGO)
            {
                minijuegos++;
                IdMinijuego id = juego.zonaPruebas.gestorMinijuegos.ObtenerIdActivo();
                std::printf("%7.1f s  %-20s  minijuego %d: %s%s\n", segundos, NombreEstado(juego.estado),
                    (int)id, ObtenerDatosMinijuego(id).nombre, juego.rondaEsDuelo ? " (duelo)" : "");
            }
            else
            {
                std::printf("%7.1f s  %-20s  ronda %d/%d\n", segundos, NombreEstado(juego.estado),
                    juego.partidaTablero.rondaActual, juego.partidaTablero.cantidadRondas);
            }

            std::fflush(stdout);
            anterior = juego.estado;
            transiciones++;
            enEstado = 0.0f;

            // La captura se toma un poco despues de entrar al estado.
            for (int f = 0; f < 90; f++) { Frame(f % 3 == 0); segundos += DT; enEstado += DT; }
            Frame(true, ruta);
            continue;
        }

        if (juego.estado == ESTADO_RESULTADOS_PARTIDA && enEstado > 3.0f)
        {
            llegoAlFinal = true;
            break;
        }

        // Un estado que no avanza en 4 minutos es un bloqueo.
        if (enEstado > 240.0f)
        {
            std::printf("BLOQUEO: %s no avanza en 240 s\n", NombreEstado(juego.estado));
            break;
        }

        Frame(((int)(segundos * 60.0f)) % 4 == 0);
        segundos += DT;
        enEstado += DT;
    }

    std::printf("Partida %s: %d transiciones, %d minijuegos, %.0f s simulados\n",
        llegoAlFinal ? "COMPLETA" : "INCOMPLETA", transiciones, minijuegos, segundos);

    juego.Descargar();
    CloseWindow();
    return llegoAlFinal ? 0 : 1;
}
