// Ventana real: fuerza una visita a la TIENDA con el flujo real de
// PartidaTablero (la casilla se resuelve en Actualizar como en partida) y
// guarda capturas del vendedor: saludo, cambio de producto, sin monedas y
// compra. Comprueba que las compras y las reacciones se registran y que la
// tienda se cierra (la partida sigue).
//
// Salida: build/capturas/tienda/*.png
// Ejecutar desde la raiz del repo (rutas de Assets relativas).
#include "Board/CatalogoTableros.h"
#include "Gameplay/PartidaTablero.h"
#include "Systems/CalidadGrafica.h"
#include "UI/EstiloUIGlobal.h"

#include "raylib.h"

#include <cstdio>
#include <cstring>


static const char* CARPETA = "build/capturas/tienda";
static const Color COLORES[MAX_PARTICIPANTES] = { RED, BLUE, GREEN, GOLD };
static const float DT = 1.0f / 60.0f;

static Participante participantes[MAX_PARTICIPANTES];
static PartidaTablero partida;
static InputSeleccionParticipante entradaGuion;
static int fallas = 0;


static InputSeleccionParticipante LeerGuion(const Participante&)
{
    return entradaGuion;
}


static void Fallar(const char* texto)
{
    std::printf("FALLA: %s\n", texto);
    fallas++;
}


// Partida con el jugador 1 humano y con el turno; los demas, bots.
static void Preparar(IdTablero id, int personajeHumano)
{
    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        participantes[i] = Participante{};
        participantes[i].numeroJugador = i + 1;
    }

    participantes[0].activo = true;
    participantes[0].conectado = true;
    participantes[0].personajeSeleccionado = personajeHumano;

    CompletarParticipantesConBots(participantes, MAX_PARTICIPANTES);

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        participantes[i].color = COLORES[i];
    }

    SetRandomSeed(77u);
    partida = PartidaTablero{};
    partida.lectorEntrada = LeerGuion;
    partida.Inicializar(participantes, MAX_PARTICIPANTES, nullptr, id);

    for (int k = 0; k < partida.cantidadJugadores; k++)
    {
        if (partida.ordenParticipantes[k] == 0)
        {
            partida.indiceOrdenTurno = k;
        }
    }
}


// Lleva al jugador 1 a una casilla del tipo pedido y deja que la partida
// resuelva la casilla en el proximo Actualizar.
static bool CaerEn(TipoCasilla tipo)
{
    for (int i = 0; i < partida.tablero.cantidadCasillas; i++)
    {
        if (partida.tablero.casillas[i].tipo != tipo) continue;

        Vector3 p = partida.tablero.casillas[i].posicion;
        partida.jugadores[0].casillaActual = i;
        partida.jugadores[0].posicionVisual = { p.x, p.y + 0.78f, p.z };
        partida.camaraFoco = { p.x, 0.4f, p.z + 1.6f };
        partida.fase = FASE_PARTIDA_TABLERO_EVENTO_CASILLA;
        partida.tiempoFase = 0.0f;
        return true;
    }

    return false;
}


static void Paso(float& t)
{
    partida.Actualizar(DT);
    entradaGuion = InputSeleccionParticipante{};

    BeginDrawing();
    partida.Dibujar();
    EndDrawing();

    t += DT;
}


static void Capturar(const char* nombre)
{
    char ruta[256];
    std::snprintf(ruta, sizeof(ruta), "%s/%s.png", CARPETA, nombre);
    TakeScreenshot(ruta);
}


static void ProbarTienda(IdTablero id)
{
    Preparar(id, 1);

    // La camara se asienta sobre la ficha antes de la visita.
    if (!CaerEn(CASILLA_TIENDA)) { Fallar("sin casilla de tienda"); return; }
    partida.fase = FASE_PARTIDA_TABLERO_FIN_RONDA;      // quieta
    partida.minijuegoSolicitado = true;
    float t = 0.0f;
    while (t < 2.6f) Paso(t);

    CaerEn(CASILLA_TIENDA);
    t = 0.0f;
    bool capturas[4] = {};
    int compras = 0;

    while (t < 7.0f && !WindowShouldClose())
    {
        if (partida.fase != FASE_PARTIDA_TABLERO_EVENTO_INTERACTIVO && t > 0.1f) break;

        EstadoTienda& tienda = partida.evento.tienda;

        if (t >= 2.4f && t < 2.4f + DT) entradaGuion.derecha = true;

        if (t >= 3.2f && t < 3.2f + DT)
        {
            partida.jugadores[0].monedas = 0;
            entradaGuion.confirmar = true;
        }

        if (t >= 4.6f && t < 4.6f + DT)
        {
            partida.jugadores[0].monedas = 40;
            compras = tienda.compras;
            entradaGuion.confirmar = true;
        }

        if (t >= 6.2f && t < 6.2f + DT) entradaGuion.cancelar = true;

        Paso(t);

        char nombre[64];
        const char* tomas[4] = { "saludo", "presenta", "sin_monedas", "compra" };
        const float instantes[4] = { 0.9f, 2.75f, 3.5f, 4.95f };

        for (int k = 0; k < 4; k++)
        {
            if (!capturas[k] && t >= instantes[k])
            {
                std::snprintf(nombre, sizeof(nombre), "tienda%d_%s", (int)id, tomas[k]);
                Capturar(nombre);
                capturas[k] = true;
            }
        }

        if (t >= 3.3f && t < 3.3f + DT && tienda.reaccion != REACCION_VENDEDOR_SIN_MONEDAS)
        {
            Fallar("el vendedor no reacciona a la falta de monedas");
        }

        if (t >= 4.7f && t < 4.7f + DT && (tienda.compras != compras + 1 || tienda.reaccion != REACCION_VENDEDOR_COMPRA))
        {
            Fallar("la compra no se registro o el vendedor no la festejo");
        }
    }

    if (partida.fase == FASE_PARTIDA_TABLERO_EVENTO_INTERACTIVO)
    {
        Fallar("la tienda no se cerro al cancelar");
    }

    std::printf("tienda tablero %d: compras ok, sale en fase %d\n", (int)id, (int)partida.fase);
}


int main()
{
    SetTraceLogLevel(LOG_WARNING);
    SetConfigFlags(FLAG_WINDOW_UNDECORATED);
    InitWindow(1280, 720, "Capturas de la tienda");
    SetExitKey(KEY_NULL);
    SetTargetFPS(0);
    EstablecerOpcionesCalidadGrafica(CrearPresetCalidadGrafica(CALIDAD_ALTA));

    if (!DirectoryExists("build/capturas")) MakeDirectory("build/capturas");
    if (!DirectoryExists(CARPETA)) MakeDirectory(CARPETA);

    ObtenerEstiloUI();

    ProbarTienda(TABLERO_ISLA_ARBOLEDA);
    ProbarTienda(TABLERO_FERIA_NOCTURNA);

    partida = PartidaTablero{};
    DescargarVistaPreviaTableros();
    DescargarEstiloUIGlobal();
    CloseWindow();

    std::printf(fallas == 0 ? "OK\n" : "FALLAS: %d\n", fallas);
    return fallas == 0 ? 0 : 1;
}
