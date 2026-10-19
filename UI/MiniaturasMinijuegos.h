#pragma once

#include "Core/CatalogoMinijuegos.h"
#include "raylib.h"


// Datos de presentacion de los minijuegos que comparten
// SeleccionMinijuegos y RuletaMinijuegos.


//==================================================
// FORMATO
//==================================================

// Quien compite contra quien. El catalogo no guarda este dato: se
// deduce de requiereParDeJugadores y de la descripcion de cada juego.
enum FormatoMinijuego
{
    FORMATO_TODOS_CONTRA_TODOS = 0,
    FORMATO_UNO_CONTRA_TRES,
    FORMATO_DOS_CONTRA_DOS,
    FORMATO_UNO_CONTRA_UNO,

    CANTIDAD_FORMATOS_MINIJUEGO
};

FormatoMinijuego ObtenerFormatoMinijuego(
    IdMinijuego id
);

// "TODOS CONTRA TODOS", "1 VS 3", "2 VS 2" o "1 VS 1" (duelo).
const char* ObtenerNombreFormatoMinijuego(
    FormatoMinijuego formato
);

// Color propio de cada formato (chinches y sellos).
Color ObtenerColorFormatoMinijuego(
    FormatoMinijuego formato
);

// "2 A 4 JUGADORES", "2 O 4 JUGADORES" si solo admite parejas o
// "SOLO 2 JUGADORES" en los duelos.
const char* ObtenerTextoJugadoresMinijuego(
    IdMinijuego id
);

// Fichas de jugador que explican el formato (4 sueltas, 1 | 3, 2 | 2, 1 | 1),
// centradas en `centro`; `alto` en pixeles.
void DibujarIconoFormatoMinijuego(
    FormatoMinijuego formato,
    Vector2 centro,
    float alto,
    float alfa = 1.0f
);


//==================================================
// ILUSTRACION
//==================================================

// Ilustracion del minijuego ocupando todo `rect` (pixeles), sin franjas
// reservadas: quien la usa dibuja marco y nombre alrededor. Hoy es un
// dibujo con primitivas; para pasar a imagenes reales basta con dibujar
// la textura dentro de esta funcion, sin tocar las pantallas.
// recortar = false cuando se dibuja dentro de una matriz rotada (el
// recorte de raylib es siempre un rectangulo de pantalla).
void DibujarIlustracionMinijuego(
    IdMinijuego id,
    Rectangle rect,
    bool recortar = true
);

// Para dibujar en una RenderTexture con la proyeccion invertida en
// vertical (cache del tablon de SeleccionMinijuegos): ahi el recorte de
// BeginScissorMode queda espejado, asi que se calcula al derecho.
// Volver a false al terminar.
void EstablecerProyeccionInvertidaMiniaturas(
    bool invertida
);
