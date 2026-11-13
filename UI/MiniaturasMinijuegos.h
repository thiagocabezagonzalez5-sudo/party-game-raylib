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
enum FormatoMiniaturaMinijuego
{
    FORMATO_TODOS_CONTRA_TODOS = 0,
    FORMATO_UNO_CONTRA_TRES,
    FORMATO_DOS_CONTRA_DOS,
    FORMATO_UNO_CONTRA_UNO,

    CANTIDAD_FORMATOS_MINIJUEGO
};

FormatoMiniaturaMinijuego ObtenerFormatoMinijuego(
    IdMinijuego id
);

// "TODOS CONTRA TODOS", "1 VS 3", "2 VS 2" o "1 VS 1" (duelo).
const char* ObtenerNombreFormatoMinijuego(
    FormatoMiniaturaMinijuego formato
);

// Color propio de cada formato (chinches y sellos).
Color ObtenerColorFormatoMinijuego(
    FormatoMiniaturaMinijuego formato
);

// "2 A 4 JUGADORES", "2 O 4 JUGADORES" si solo admite parejas o
// "SOLO 2 JUGADORES" en los duelos.
const char* ObtenerTextoJugadoresMinijuego(
    IdMinijuego id
);

// Fichas de jugador que explican el formato (4 sueltas, 1 | 3, 2 | 2, 1 | 1),
// centradas en `centro`; `alto` en pixeles.
void DibujarIconoFormatoMinijuego(
    FormatoMiniaturaMinijuego formato,
    Vector2 centro,
    float alto,
    float alfa = 1.0f
);


//==================================================
// ILUSTRACION
//==================================================

// Ilustracion del minijuego ocupando todo `rect` (pixeles), sin franjas
// reservadas: quien la usa dibuja marco y nombre alrededor.
// Es la captura de gameplay de ObtenerRutaMiniaturaMinijuego, recortada
// al centro para llenar `rect` (una sola textura chica por minijuego, que
// se carga la primera vez que se dibuja). Si falta el archivo se dibuja
// el dibujo de respaldo con primitivas.
// recortar = false cuando se dibuja dentro de una matriz rotada (el
// recorte de raylib es siempre un rectangulo de pantalla; la captura no
// lo necesita, solo el dibujo de respaldo).
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

// "Assets/UI/Miniaturas/<minijuego>.png": captura de gameplay que genera
// Tests/CapturarMiniaturas.ps1.
const char* ObtenerRutaMiniaturaMinijuego(
    IdMinijuego id
);

// Libera las capturas cargadas por DibujarIlustracionMinijuego (la
// proxima vez que se dibujen se vuelven a cargar). La llaman
// SeleccionMinijuegos::Descargar y RuletaMinijuegos::Descargar al dejar
// sus pantallas y al cerrar el juego. Seguro llamar varias veces.
void DescargarMiniaturasMinijuegos();
