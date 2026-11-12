#pragma once

#include "raylib.h"

#include <cmath>


//==================================================
// ESTADO VISUAL COMPARTIDO DE LOS MINIJUEGOS
//==================================================
//
// Temblor de camara, barra de ronda / ayuda y la opcion de reducir
// movimiento. Cada minijuego dibuja su propio escenario completo: aca no
// hay decorado comun.
//==================================================

struct EstadoEfectosVisualesMinijuegos
{
    float tiempoGlobal = 0.0f;
    float tiempoTemblor = 0.0f;
    float duracionTemblor = 0.0f;
    float intensidadTemblor = 0.0f;

    // Accesibilidad: "reducir movimiento" anula el temblor de camara.
    bool reducirMovimiento = false;

    // Ronda oficial lanzada desde el tablero (ZonaPruebas.modoTablero):
    // ahi R no reinicia, asi que los minijuegos no deben ofrecerlo.
    bool rondaOficialTablero = false;

    // Modo libre del catalogo: ZonaPruebas dibuja su ayuda en una barra
    // fina al pie, del mismo alto que la barra de ronda.
    bool barraAyudaCatalogo = false;
};


inline EstadoEfectosVisualesMinijuegos&
ObtenerEstadoEfectosVisualesMinijuegos()
{
    static EstadoEfectosVisualesMinijuegos estado;
    return estado;
}


//==================================================
// TEMBLOR DE CAMARA GENERAL
//==================================================

inline void ActualizarEfectosVisualesMinijuegos(
    float deltaTime
)
{
    EstadoEfectosVisualesMinijuegos& estado =
        ObtenerEstadoEfectosVisualesMinijuegos();

    estado.tiempoGlobal += deltaTime;

    if (estado.tiempoTemblor > 0.0f)
    {
        estado.tiempoTemblor -= deltaTime;

        if (estado.tiempoTemblor < 0.0f)
        {
            estado.tiempoTemblor = 0.0f;
        }
    }
}


// ZonaPruebas lo actualiza cada frame segun su modo.
inline void EstablecerRondaOficialTableroMinijuegos(
    bool oficial
)
{
    ObtenerEstadoEfectosVisualesMinijuegos().rondaOficialTablero = oficial;
}


// ZonaPruebas lo actualiza cada frame: true en modo libre del catalogo.
inline void EstablecerBarraAyudaCatalogoMinijuegos(
    bool activa
)
{
    ObtenerEstadoEfectosVisualesMinijuegos().barraAyudaCatalogo = activa;
}


// Texto del aviso de reinicio en la pantalla final de cada minijuego.
// En una ronda oficial de tablero se devuelve vacio: R esta bloqueada y
// Juego ya muestra como se vuelve al tablero.
inline const char* TextoReinicioMinijuego()
{
    return ObtenerEstadoEfectosVisualesMinijuegos().rondaOficialTablero
        ? ""
        : "R PARA REINICIAR";
}


// Alto de la barra de ronda que Juego dibuja al pie de la pantalla en una
// ronda oficial de tablero (solo con los minijuegos de HUD viejo); ZonaPruebas
// usa el mismo alto para la barra de ayuda del modo libre del catalogo.
static const int ALTO_BARRA_RONDA_MINIJUEGOS = 28;


// Espacio que el HUD inferior de un minijuego de HUD viejo debe dejar libre
// al pie de la pantalla: la barra de ronda en una ronda oficial o la barra de
// ayuda en el modo libre del catalogo, 0 si no hay ninguna.
inline int MargenBarraRondaMinijuegos()
{
    const EstadoEfectosVisualesMinijuegos& estado =
        ObtenerEstadoEfectosVisualesMinijuegos();

    return estado.rondaOficialTablero || estado.barraAyudaCatalogo
        ? ALTO_BARRA_RONDA_MINIJUEGOS
        : 0;
}


// Opcion de accesibilidad (la aplica Juego desde ConfiguracionJuego).
inline void EstablecerReducirMovimientoMinijuegos(
    bool reducir
)
{
    ObtenerEstadoEfectosVisualesMinijuegos().reducirMovimiento = reducir;
}


// Los minijuegos con temblor propio pueden multiplicar su intensidad por
// este factor para respetar la misma opcion.
inline float ObtenerFactorMovimientoCamaraMinijuegos()
{
    return ObtenerEstadoEfectosVisualesMinijuegos().reducirMovimiento ? 0.0f : 1.0f;
}


inline void ActivarTemblorCamaraGeneral(
    float intensidad = 0.16f,
    float duracion = 0.26f
)
{
    EstadoEfectosVisualesMinijuegos& estado =
        ObtenerEstadoEfectosVisualesMinijuegos();

    if (estado.reducirMovimiento)
    {
        return;
    }

    if (
        intensidad > estado.intensidadTemblor ||
        estado.tiempoTemblor <= 0.0f
    )
    {
        estado.intensidadTemblor = intensidad;
    }

    if (duracion > estado.tiempoTemblor)
    {
        estado.tiempoTemblor = duracion;
        estado.duracionTemblor = duracion;
    }
}


inline Camera3D AplicarTemblorGeneralACamara(
    Camera3D camara
)
{
    EstadoEfectosVisualesMinijuegos& estado =
        ObtenerEstadoEfectosVisualesMinijuegos();

    if (estado.tiempoTemblor <= 0.0f)
    {
        return camara;
    }

    float porcentaje =
        estado.duracionTemblor > 0.0f
            ? estado.tiempoTemblor / estado.duracionTemblor
            : 0.0f;

    if (porcentaje < 0.0f) porcentaje = 0.0f;
    if (porcentaje > 1.0f) porcentaje = 1.0f;

    float amplitud =
        estado.intensidadTemblor * porcentaje;

    float dx =
        std::sin(estado.tiempoGlobal * 47.0f) * amplitud;

    float dy =
        std::cos(estado.tiempoGlobal * 61.0f) *
        amplitud * 0.72f;

    float dz =
        std::sin(estado.tiempoGlobal * 37.0f + 0.8f) *
        amplitud * 0.46f;

    camara.position.x += dx;
    camara.position.y += dy;
    camara.position.z += dz;

    camara.target.x += dx * 0.34f;
    camara.target.y += dy * 0.28f;
    camara.target.z += dz * 0.34f;

    return camara;
}


// Deja el estado visual global como si no hubiera minijuego activo:
// llamarlo al salir de un minijuego hacia pantallas que no lo son
// (tablero, menus) para que un temblor pendiente no quede congelado.
inline void ReiniciarEstadoVisualMinijuegos()
{
    EstadoEfectosVisualesMinijuegos& estado =
        ObtenerEstadoEfectosVisualesMinijuegos();

    estado.tiempoTemblor = 0.0f;
    estado.duracionTemblor = 0.0f;
    estado.intensidadTemblor = 0.0f;
    estado.rondaOficialTablero = false;
    estado.barraAyudaCatalogo = false;
}
