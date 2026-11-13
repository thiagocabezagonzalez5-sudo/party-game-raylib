// Recorrido automatico de TODOS los minijuegos con el flujo real del juego
// (ventana real, Zona de Pruebas en modo catalogo, igual que el menu).
// Cada minijuego se juega con 2, 3 y 4 participantes bot hasta que termina
// y se informa: estado final, desenlace, duracion, costo de actualizar y
// dibujar por frame. Ademas guarda una captura de la pantalla final.
//
// Uso: RecorrerMinijuegos.exe [--minijuego N] [--jugadores N] [--salida carpeta]
// Codigo de salida: cantidad de corridas con problema (0 = todo bien).
#include "Core/Juego.h"

#include "rlgl.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

static Juego juego;

static const float DT = 1.0f / 60.0f;
static const float TOPE_SEGUNDOS = 240.0f;

static const char* TextoEstado(EstadoResultadoMinijuego e)
{
    switch (e)
    {
        case RESULTADO_MINIJUEGO_EN_CURSO: return "EN CURSO";
        case RESULTADO_MINIJUEGO_FINALIZADO: return "FINALIZADO";
        default: return "CANCELADO";
    }
}

static const char* TextoDesenlace(DesenlaceMinijuego d)
{
    switch (d)
    {
        case DESENLACE_CON_GANADOR: return "ganador";
        case DESENLACE_EMPATE: return "empate";
        case DESENLACE_VICTORIA_COOPERATIVA: return "victoria coop";
        case DESENLACE_DERROTA_COOPERATIVA: return "derrota coop";
        case DESENLACE_COMPLETADO: return "completado";
        default: return "sin definir";
    }
}

// Un frame del juego real; devuelve los milisegundos de dibujo.
static double Frame(bool dibujar, const char* captura = nullptr)
{
    juego.Actualizar(DT);

    if (!dibujar && captura == nullptr)
    {
        return 0.0;
    }

    double inicio = GetTime();
    BeginDrawing();
    juego.Dibujar();

    if (captura != nullptr)
    {
        rlDrawRenderBatchActive();
        Image imagen = LoadImageFromScreen();
        ExportImage(imagen, captura);
        UnloadImage(imagen);
    }

    // Solo el trabajo de dibujo: EndDrawing ademas espera el limite de FPS.
    rlDrawRenderBatchActive();
    double milisegundos = (GetTime() - inicio) * 1000.0;
    EndDrawing();
    return milisegundos;
}

static void PrepararParticipantes(int cantidad)
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

int main(int argc, char** argv)
{
    int soloMinijuego = -1;
    int soloJugadores = 0;
    const char* salida = "build/capturas/minijuegos";

    for (int a = 1; a < argc; a++)
    {
        if (!std::strcmp(argv[a], "--minijuego") && a + 1 < argc) soloMinijuego = std::atoi(argv[++a]);
        else if (!std::strcmp(argv[a], "--jugadores") && a + 1 < argc) soloJugadores = std::atoi(argv[++a]);
        else if (!std::strcmp(argv[a], "--salida") && a + 1 < argc) salida = argv[++a];
    }

    juego.anchoVentanaForzada = 1280;
    juego.altoVentanaForzada = 720;
    juego.Inicializar();
    SetTargetFPS(0);
    SetExitKey(KEY_NULL);
    SetTraceLogLevel(LOG_WARNING);

    for (int f = 0; f < 60; f++) Frame(true);

    int problemas = 0;

    std::printf("id  minijuego                      jug  estado      desenlace      seg    upd_ms dib_ms\n");

    for (int indice = 0; indice < CANTIDAD_MINIJUEGOS; indice++)
    {
        IdMinijuego id = ObtenerIdMinijuegoPorIndice(indice);

        if (soloMinijuego >= 0 && (int)id != soloMinijuego) continue;

        const DatosMinijuegoCatalogo& datos = ObtenerDatosMinijuego(id);

        for (int cantidad = 2; cantidad <= 4; cantidad++)
        {
            if (soloJugadores > 0 && cantidad != soloJugadores) continue;

            bool admite = MinijuegoAdmiteCantidadJugadores(id, cantidad);

            PrepararParticipantes(cantidad);
            juego.zonaPruebas.Inicializar(juego.participantes, MAX_PARTICIPANTES, &juego.audio);
            juego.zonaPruebas.modoCatalogo = true;
            juego.zonaPruebas.CambiarMinijuego(id);
            juego.estado = ESTADO_ZONA_PRUEBAS;

            float segundos = 0.0f;
            double sumaDibujo = 0.0;
            double peorActualizar = 0.0;
            int framesDibujados = 0;
            const ResultadoMinijuego* resultado = juego.zonaPruebas.ObtenerResultadoMinijuego();

            while (segundos < TOPE_SEGUNDOS)
            {
                double inicio = GetTime();
                bool dibujar = (framesDibujados < 400) || ((int)(segundos * 60.0f) % 6 == 0);
                double dibujo = Frame(dibujar);
                double total = (GetTime() - inicio) * 1000.0 - dibujo;

                if (total > peorActualizar) peorActualizar = total;
                if (dibujar) { sumaDibujo += dibujo; framesDibujados++; }

                segundos += DT;
                resultado = juego.zonaPruebas.ObtenerResultadoMinijuego();

                if (resultado == nullptr || resultado->estado != RESULTADO_MINIJUEGO_EN_CURSO)
                {
                    break;
                }

                // Cantidad no admitida: el minijuego queda esperando
                // jugadores (pantalla "SE NECESITAN..." con ESC para salir).
                if (!admite && segundos > 10.0f)
                {
                    break;
                }
            }

            // Dos segundos de pantalla final y captura.
            for (int f = 0; f < 120; f++) Frame(f % 4 == 0);

            char ruta[512];
            std::snprintf(ruta, sizeof(ruta), "%s/mj%02d_%dj.png", salida, (int)id, cantidad);
            Frame(true, ruta);

            EstadoResultadoMinijuego estado = resultado ? resultado->estado : RESULTADO_MINIJUEGO_CANCELADO;
            DesenlaceMinijuego desenlace = resultado ? resultado->desenlace : DESENLACE_SIN_DEFINIR;

            bool problema =
                resultado == nullptr ||
                (admite && estado != RESULTADO_MINIJUEGO_FINALIZADO) ||
                (estado == RESULTADO_MINIJUEGO_FINALIZADO && desenlace == DESENLACE_SIN_DEFINIR);

            if (problema) problemas++;

            std::printf(
                "%2d  %-30s %d%s  %-11s %-14s %6.1f %6.1f %6.2f%s\n",
                (int)id,
                datos.nombre,
                cantidad,
                admite ? " " : "*",
                TextoEstado(estado),
                TextoDesenlace(desenlace),
                segundos,
                peorActualizar,
                framesDibujados > 0 ? sumaDibujo / framesDibujados : 0.0,
                problema ? "  <-- REVISAR" : ""
            );
            std::fflush(stdout);
        }
    }

    std::printf("(* = cantidad de jugadores no admitida: se espera CANCELADO o la pantalla de espera EN CURSO)\n");
    std::printf("Corridas con problema: %d\n", problemas);

    juego.Descargar();
    CloseWindow();
    return problemas;
}
