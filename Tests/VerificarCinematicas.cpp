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

    std::printf("Verificacion Cinematicas: %d errores\n", errores);
    return errores == 0 ? 0 : 1;
}
