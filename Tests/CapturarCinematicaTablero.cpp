// Abre una ventana real y reproduce la presentacion (CinematicaTablero) de
// cada tablero tal como en el juego, guardando una captura PNG por plano
// (a mitad del plano, con el globo de dialogo ya escrito) y midiendo los
// FPS de toda la presentacion.
//
// Salida: build/capturas/cinematicas/<tablero>_plano<NN>.png
// Uso: CapturarCinematicaTablero.exe [--tablero N] [--resolucion ANCHO ALTO]
// Ejecutar desde la raiz del repo (rutas de Assets relativas).
#include "Board/CatalogoTableros.h"
#include "Gameplay/PartidaTablero.h"
#include "Systems/CalidadGrafica.h"
#include "UI/Cinematicas.h"
#include "UI/EstiloUIGlobal.h"

#include "raylib.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>


static const char* CARPETA = "build/capturas/cinematicas";
static const Color COLORES[MAX_PARTICIPANTES] = { RED, BLUE, GREEN, GOLD };

static Participante participantes[MAX_PARTICIPANTES];
static PartidaTablero partida;
static CinematicaTablero presentacion;


int main(int argc, char** argv)
{
    int filtro = -1;
    int ancho = 1280;
    int alto = 720;

    for (int i = 1; i < argc; i++)
    {
        if (std::strcmp(argv[i], "--tablero") == 0 && i + 1 < argc) filtro = std::atoi(argv[++i]);
        else if (std::strcmp(argv[i], "--resolucion") == 0 && i + 2 < argc)
        {
            ancho = std::atoi(argv[++i]);
            alto = std::atoi(argv[++i]);
        }
    }

    SetTraceLogLevel(LOG_WARNING);
    SetConfigFlags(FLAG_WINDOW_UNDECORATED);
    InitWindow(ancho, alto, "Capturas de cinematicas de tablero");
    SetExitKey(KEY_NULL);
    SetTargetFPS(0);
    EstablecerOpcionesCalidadGrafica(CrearPresetCalidadGrafica(CALIDAD_ALTA));

    if (!DirectoryExists("build/capturas")) MakeDirectory("build/capturas");
    if (!DirectoryExists(CARPETA)) MakeDirectory(CARPETA);

    ObtenerEstiloUI();

    for (int t = 0; t < ObtenerCantidadTableros(); t++)
    {
        if (filtro >= 0 && filtro != t) continue;

        for (int i = 0; i < MAX_PARTICIPANTES; i++)
        {
            participantes[i] = Participante{};
            participantes[i].numeroJugador = i + 1;
        }

        CompletarParticipantesConBots(participantes, MAX_PARTICIPANTES);

        for (int i = 0; i < MAX_PARTICIPANTES; i++)
        {
            participantes[i].color = COLORES[i];
        }

        SetRandomSeed(1234u + (unsigned int)t);
        partida = PartidaTablero{};
        partida.Inicializar(participantes, MAX_PARTICIPANTES, nullptr, (IdTablero)t);
        presentacion = CinematicaTablero{};
        presentacion.Inicializar(partida, 5);

        // Tiempo fijo por frame (como a 60 FPS) para que las tomas caigan
        // siempre en el mismo instante; el FPS se mide con el reloj real.
        const float dt = 1.0f / 60.0f;
        int siguiente = 0;
        int frames = 0;
        double inicio = GetTime();
        float peor = 0.0f;

        while (!presentacion.estado.terminada && !WindowShouldClose())
        {
            double antes = GetTime();

            presentacion.Actualizar(dt);

            BeginDrawing();
            presentacion.Dibujar();
            EndDrawing();

            float duracionFrame = (float)(GetTime() - antes);
            if (frames > 10 && duracionFrame > peor) peor = duracionFrame;
            frames++;

            if (siguiente < presentacion.cantidad)
            {
                const PlanoCinematicaTablero& plano = presentacion.planos[siguiente];

                if (presentacion.estado.tiempo >= plano.inicio + plano.duracion * 0.72f)
                {
                    char ruta[256];
                    std::snprintf(ruta, sizeof(ruta), "%s/tablero%d_plano%02d.png", CARPETA, t, siguiente);
                    TakeScreenshot(ruta);
                    siguiente++;
                }
            }
        }

        double total = GetTime() - inicio;
        std::printf("tablero %d: %d planos, %.1f s de cinematica, %.0f FPS promedio, peor frame %.1f ms\n",
            t, presentacion.cantidad, presentacion.estado.duracion,
            frames / (total > 0.0 ? total : 1.0), peor * 1000.0f);
    }

    partida = PartidaTablero{};
    DescargarVistaPreviaTableros();
    DescargarEstiloUIGlobal();
    CloseWindow();
    return 0;
}
