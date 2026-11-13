// Prueba de las cinematicas (sin pantalla): duracion, omision con Enter
// solo despues del minimo, datos de la presentacion de minijuego y del
// campeon (incluido el empate en el primer puesto).
// Se enlaza con -Wl,--wrap=IsKeyPressed para simular la tecla.
#include "UI/Cinematicas.h"

#include <cmath>
#include <cstdarg>
#include <cstdio>

static int errores = 0;
static int teclaSimulada = KEY_NULL;

extern "C" bool __wrap_IsKeyPressed(int tecla)
{
    return tecla == teclaSimulada;
}

static void Comprobar(bool condicion, const char* formato, ...)
{
    if (!condicion)
    {
        char texto[256];
        va_list argumentos;
        va_start(argumentos, formato);
        std::vsnprintf(texto, sizeof(texto), formato, argumentos);
        va_end(argumentos);
        std::fprintf(stderr, "FALLO: %s\n", texto);
        errores++;
    }
}

static const float DT = 1.0f / 60.0f;

template <typename T>
static int CorrerHastaTerminar(T& cinematica, float tope)
{
    int frames = 0;
    while (!cinematica.estado.terminada && frames * DT < tope)
    {
        cinematica.Actualizar(DT);
        frames++;
    }
    return frames;
}

int main()
{
    SetTraceLogLevel(LOG_WARNING);

    // Apertura: sin entrada dura lo previsto.
    CinematicaApertura apertura;
    apertura.Inicializar();
    int frames = CorrerHastaTerminar(apertura, 20.0f);
    Comprobar(apertura.estado.terminada && !apertura.estado.omitida, "La apertura termina sola");
    Comprobar(std::fabs(frames * DT - CinematicaApertura::DURACION) < 0.05f, "La apertura dura %.2f s", frames * DT);

    // Enter apretado desde el primer frame (la pulsacion que la abrio): no
    // la salta hasta pasado el minimo; despues si.
    apertura.Inicializar();
    teclaSimulada = KEY_ENTER;
    apertura.Actualizar(DT);
    Comprobar(!apertura.estado.terminada, "No se omite en el primer frame");
    frames = 1 + CorrerHastaTerminar(apertura, 20.0f);
    teclaSimulada = KEY_NULL;
    Comprobar(apertura.estado.omitida, "Enter omite la apertura");
    Comprobar(frames * DT >= 0.39f && frames * DT < 0.5f, "Se omite justo tras el minimo (%.2f s)", frames * DT);

    // Escape tambien omite.
    apertura.Inicializar();
    for (int f = 0; f < 40; f++) apertura.Actualizar(DT);
    teclaSimulada = KEY_ESCAPE;
    apertura.Actualizar(DT);
    teclaSimulada = KEY_NULL;
    Comprobar(apertura.estado.omitida, "Esc omite la apertura");

    // Presentacion de minijuego: solo los participantes activos.
    Participante participantes[MAX_PARTICIPANTES]{};
    const Color colores[4] = { RED, BLUE, GREEN, YELLOW };
    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        participantes[i].activo = i != 1;
        participantes[i].numeroJugador = i + 1;
        participantes[i].personajeSeleccionado = i == 3 ? -1 : (i + 2) % 4;
        participantes[i].color = colores[i];
    }

    CinematicaMinijuego minijuego;
    minijuego.Inicializar(MINIJUEGO_TRONCO, participantes, false);
    Comprobar(minijuego.cantidad == 3, "La presentacion muestra solo a los activos (%d)", minijuego.cantidad);
    Comprobar(minijuego.figuras[0].numeroJugador == 1 && minijuego.figuras[1].numeroJugador == 3, "Orden de jugadores conservado");
    Comprobar(minijuego.figuras[0].personaje == 2, "Personaje elegido en la seleccion");
    Comprobar(minijuego.figuras[2].personaje == 3, "Sin eleccion: personaje segun el puesto");
    frames = CorrerHastaTerminar(minijuego, 10.0f);
    Comprobar(std::fabs(frames * DT - CinematicaMinijuego::DURACION) < 0.05f, "La presentacion dura %.2f s", frames * DT);

    // Campeon: ranking y empate en el primer puesto.
    int ranking[4] = { 2, 0, 3, 1 };
    int puestos[4] = { 1, 2, 3, 4 };
    for (int i = 0; i < MAX_PARTICIPANTES; i++) participantes[i].activo = true;

    CinematicaCampeon campeon;
    campeon.Inicializar(ranking, puestos, 4, participantes);
    Comprobar(campeon.cantidad == 4 && campeon.figuras[0].numeroJugador == 3, "El campeon es el primero del ranking");
    Comprobar(!campeon.empateEnPrimero, "Sin empate en el primer puesto");

    int puestosEmpate[4] = { 1, 1, 3, 4 };
    campeon.Inicializar(ranking, puestosEmpate, 4, participantes);
    Comprobar(campeon.empateEnPrimero, "Empate en el primer puesto detectado");

    frames = CorrerHastaTerminar(campeon, 10.0f);
    Comprobar(std::fabs(frames * DT - CinematicaCampeon::DURACION) < 0.05f, "El festejo dura %.2f s", frames * DT);

    // Presentacion de cada tablero: mismo codigo, guion propio.
    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        participantes[i] = Participante{};
        participantes[i].numeroJugador = i + 1;
    }

    CompletarParticipantesConBots(participantes, MAX_PARTICIPANTES);

    const TipoCasilla obligatorias[6] =
    {
        CASILLA_REGALO, CASILLA_CAOS, CASILLA_INTERCAMBIO,
        CASILLA_DUELO, CASILLA_TIENDA, CASILLA_EVENTO
    };

    for (int t = 0; t < ObtenerCantidadTableros(); t++)
    {
        static PartidaTablero partida;
        partida = PartidaTablero{};
        partida.Inicializar(participantes, MAX_PARTICIPANTES, nullptr, (IdTablero)t);

        const char* nombre = partida.definicion->nombre;
        const FasePartidaTablero faseAntes = partida.fase;
        const int monedasAntes = partida.jugadores[partida.ordenParticipantes[0]].monedas;
        const int trofeoAntes = partida.casillaTrofeo;

        static CinematicaTablero presentacion;
        presentacion = CinematicaTablero{};
        presentacion.Inicializar(partida, 5);

        const float duracion = presentacion.estado.duracion;
        Comprobar(duracion >= 15.0f && duracion <= 30.0f, "%s: la presentacion dura %.1f s (15-30)", nombre, duracion);
        Comprobar(presentacion.cantidad >= 6, "%s: %d planos", nombre, presentacion.cantidad);

        // Cada tipo especial se presenta, y sobre una casilla REAL de ese tipo.
        bool presentado[CANTIDAD_TIPOS_CASILLA] = {};

        for (int k = 0; k < presentacion.cantidad; k++)
        {
            const LineaGuionTablero& linea = presentacion.guion->lineas[k];
            const PlanoCinematicaTablero& plano = presentacion.planos[k];

            if (linea.plano == PLANO_CASILLA)
            {
                const Casilla* casilla = partida.tablero.ObtenerCasilla(plano.casilla);
                Comprobar(casilla != nullptr && casilla->tipo == linea.casilla,
                    "%s: el plano %d muestra una casilla %s real", nombre, k, ObtenerNombreTipoCasilla(linea.casilla));

                if (casilla != nullptr && casilla->tipo == linea.casilla)
                {
                    presentado[linea.casilla] = true;
                }
            }

            if (linea.plano == PLANO_TROFEO)
            {
                Comprobar(plano.casilla == partida.casillaTrofeo, "%s: el plano del trofeo muestra la casilla del trofeo", nombre);
            }

            // El guia siempre queda parado sobre una casilla del recorrido.
            bool sobreCasilla = false;

            for (int c = 0; c < partida.tablero.cantidadCasillas; c++)
            {
                Vector3 pos = partida.tablero.casillas[c].posicion;
                float dx = pos.x - plano.guia.x;
                float dz = pos.z - plano.guia.z;
                sobreCasilla = sobreCasilla || (dx * dx + dz * dz < 0.0001f);
            }

            Comprobar(sobreCasilla, "%s: el guia del plano %d esta sobre una casilla", nombre, k);
        }

        for (TipoCasilla tipo : obligatorias)
        {
            Comprobar(presentado[tipo], "%s: falta presentar la casilla %s", nombre, ObtenerNombreTipoCasilla(tipo));
        }

        Comprobar(presentacion.guion->lineas[0].plano == PLANO_PANORAMA, "%s: empieza con la vista general", nombre);
        Comprobar(presentacion.guion->lineas[presentacion.cantidad - 1].plano == PLANO_DESPEDIDA, "%s: termina con la despedida", nombre);

        // Sin entrada termina sola; no toca la logica de la partida.
        frames = CorrerHastaTerminar(presentacion, 40.0f);
        Comprobar(presentacion.estado.terminada && !presentacion.estado.omitida, "%s: la presentacion termina sola", nombre);
        Comprobar(std::fabs(frames * DT - duracion) < 0.05f, "%s: dura lo previsto (%.2f s)", nombre, frames * DT);
        Comprobar(partida.fase == faseAntes && partida.casillaTrofeo == trofeoAntes &&
            partida.jugadores[partida.ordenParticipantes[0]].monedas == monedasAntes,
            "%s: la presentacion no cambia la partida", nombre);
        Comprobar(partida.tiempoTablero > duracion - 0.1f, "%s: el reloj visual del tablero avanza", nombre);

        // Se puede omitir (Enter o Esc) pasado el minimo.
        presentacion.Inicializar(partida, 5);
        teclaSimulada = KEY_ENTER;
        presentacion.Actualizar(DT);
        Comprobar(!presentacion.estado.terminada, "%s: no se omite en el primer frame", nombre);
        frames = 1 + CorrerHastaTerminar(presentacion, 40.0f);
        teclaSimulada = KEY_NULL;
        Comprobar(presentacion.estado.omitida && frames * DT < 0.5f, "%s: Enter la omite (%.2f s)", nombre, frames * DT);

        std::printf("%-18s guia %-7s %2d planos %.1f s\n", nombre,
            ObtenerNombrePersonajeBloque(presentacion.guion->guia), presentacion.cantidad, duracion);
    }

    std::printf("Verificacion Cinematicas: %d errores\n", errores);
    return errores == 0 ? 0 : 1;
}
