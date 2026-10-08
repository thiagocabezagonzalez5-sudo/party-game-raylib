#pragma once

#include "raylib.h"

#include "Board/MallaTablero.h"

#define SOMBRAS_RETRO_AUTOMATICAS
#include "Minigames/SombrasRetro.h"


//==================================================
// TIPOS DE CASILLA
//==================================================

enum TipoCasilla
{
    CASILLA_NEUTRA = 0,
    CASILLA_POSITIVA,
    CASILLA_NEGATIVA,
    CASILLA_ESPECIAL,

    // Casillas de evento con personalidad (comportamiento en PartidaTablero):
    CASILLA_REGALO,       // el escenario o un NPC entrega algo aleatorio
    CASILLA_CAOS,         // evento global negativo con presentacion propia
    CASILLA_INTERCAMBIO,  // ruleta de que intercambiar y con quien
    CASILLA_DUELO,        // desafio a otro jugador con apuesta limitada
    CASILLA_TIENDA,       // NPC comerciante
    CASILLA_EVENTO,       // evento propio del tablero (gimmick local)

    CANTIDAD_TIPOS_CASILLA
};


inline constexpr int MAX_CONEXIONES_CASILLA =
    2;


//==================================================
// CONEXION
//==================================================

struct ConexionCasilla
{
    int destino = -1;
};


//==================================================
// CASILLA
//==================================================

struct Casilla
{
    int indice = -1;

    Vector3 posicion{};

    TipoCasilla tipo =
        CASILLA_NEUTRA;

    ConexionCasilla conexiones[
        MAX_CONEXIONES_CASILLA
    ];

    // Zona del mapa (indice en DefinicionTablero::nombresZonas), -1 = ninguna.
    int zona = -1;

    int cantidadConexiones =
        0;
};


Color ObtenerColorCasilla(
    TipoCasilla tipo
);


//==================================================
// ESTILO VISUAL DE CASILLA
//==================================================
//
// Las casillas son losas de piedra (o madera, hielo, basalto...) con el
// color funcional incrustado en el centro. Cada tablero entrega su
// EstiloCasilla: solo cambian los colores y las proporciones, el dibujo
// es comun. MODELO FUTURO: una losa GLB por material.

struct EstiloCasilla
{
    // Cuerpo de la losa (lado) y tapa superior.
    Color piedraLado = Color{ 112, 108, 102, 255 };
    Color piedraTapa = Color{ 158, 152, 140, 255 };

    // Borde oscuro y detalle (musgo, hollin, escarcha) del contorno.
    Color borde = Color{ 62, 60, 58, 255 };
    Color detalle = Color{ 84, 138, 70, 255 };

    // Sendero que une las casillas.
    Color sendero = Color{ 196, 170, 122, 255 };
    Color senderoBorde = Color{ 150, 126, 88, 255 };

    float radio = 0.84f;
    float altura = 0.44f;
    float anchoSendero = 1.05f;

    // Intensidad del pulso luminoso de la incrustacion (0 = fija).
    float pulso = 0.10f;
};


// Estilo por defecto: piedra clara con musgo (Isla Arboleda y prototipo).
EstiloCasilla ObtenerEstiloCasillaPiedra();


// Dibuja una casilla como losa tematica con el color funcional
// incrustado y un simbolo (+, -, estrella) legible sin color.
void DibujarCasillaTematica(
    const Casilla& casilla,
    const EstiloCasilla& estilo,
    float tiempo
);


// Cinta plana sobre el suelo entre dos puntos (senderos).
void DibujarCintaSuelo(
    Vector3 origen,
    Vector3 destino,
    float ancho,
    float altura,
    Color color
);


const char* ObtenerNombreTipoCasilla(
    TipoCasilla tipo
);


//==================================================
// VERSION EN MALLA (usada por Tablero::DibujarRuta)
//==================================================

// Igual que DibujarCintaSuelo pero emitida a una malla (dos caras).
void AgregarCintaSueloMalla(
    ConstructorMalla& malla,
    Vector3 origen,
    Vector3 destino,
    float ancho,
    float altura,
    Color color
);


// Colores del anillo claro y del disco de la incrustacion en este instante
// (pulso incluido).
void ObtenerColoresIncrustacionCasilla(
    const Casilla& casilla,
    const EstiloCasilla& estilo,
    float tiempo,
    Color& claro,
    Color& normal
);


// Emite la losa a "fija" y el anillo + disco de la incrustacion a
// "incrustacion" (2 cilindros de LADOS lados por casilla, cuyos colores se
// reescriben por frame, seguidos del simbolo). Mismo aspecto que DibujarCasillaTematica.
void AgregarCasillaTematicaMalla(
    ConstructorMalla& fija,
    ConstructorMalla& incrustacion,
    const Casilla& casilla,
    const EstiloCasilla& estilo,
    bool conSombra,
    bool conMusgo
);
