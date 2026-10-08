#pragma once

#include "Board/Tablero.h"

#include "raylib.h"


struct PartidaTablero;


//==================================================
// CATALOGO DE TABLEROS
//==================================================
//
// El SISTEMA COMUN (turnos, dado, movimiento, rondas, monedas, trofeos,
// minijuegos y resultados) vive en PartidaTablero. Cada tablero aporta
// solo sus DATOS y REGLAS mediante una DefinicionTablero: casillas,
// identidad, decoracion 3D y hooks opcionales del gimmick.
//
// Convenciones:
//   - La casilla 0 es siempre la casilla inicial.
//   - En una bifurcacion la opcion 0 es la ruta larga/segura y la
//     opcion 1 el atajo.
//   - Los hooks son opcionales (nullptr = comportamiento comun).

enum IdTablero
{
    TABLERO_ISLA_ARBOLEDA = 0,
    TABLERO_FORJA_VOLCAN,
    CANTIDAD_TABLEROS
};


struct DefinicionTablero
{
    IdTablero id = TABLERO_ISLA_ARBOLEDA;

    // Identidad (para la pantalla de seleccion y el HUD).
    const char* nombre = "";
    const char* historia = "";
    const char* objetivo = "";
    const char* gimmick = "";
    const char* dificultad = "";

    Color colorTema = WHITE;
    Color colorFondo = BLACK;

    // Camara inicial de la partida (la vista previa la orbita).
    Vector3 camaraPosicion{};
    Vector3 camaraObjetivo{};
    float camaraFovy = 48.0f;

    // Cuanto sigue la camara al jugador activo (0 = fija en el centro,
    // 1 = centrada en la ficha) y landmark que enfoca en eventos del gimmick.
    float camaraSeguimiento = 0.5f;
    Vector3 camaraLandmark{};
    bool tieneLandmark = false;

    // Multiplica el desfase de camara en la vista previa del menu
    // (los tableros grandes necesitan mostrarse completos).
    float camaraEscalaVistaPrevia = 1.0f;

    // Zoom de la vista general al empezar cada ronda (<= 1 = sin vista general).
    float camaraZoomGeneral = 1.0f;

    // Zonas reconocibles del mapa (Casilla::zona indexa esta lista).
    const char* const* nombresZonas = nullptr;
    int cantidadZonas = 0;

    // Estilo de las losas y senderos (el tablero 2 puede reutilizarlo
    // con otros colores/material).
    EstiloCasilla estiloCasilla{};

    // Trofeo: casillas candidatas y coste.
    const int* casillasTrofeo = nullptr;
    int cantidadCasillasTrofeo = 0;
    int costoTrofeo = 20;
    int cantidadRondas = 5;

    // Casillas, conexiones y validacion. Debe dejar el tablero
    // con todas las conexiones abiertas y recorridoValido calculado.
    void (*Construir)(Tablero& tablero) = nullptr;

    // MODELO FUTURO: la decoracion sera un modelo 3D por tablero.
    // Suelo y escenografia (sin gimmick animado).
    void (*DibujarDecoracion)(const PartidaTablero& partida) = nullptr;

    // --- Hooks opcionales del gimmick ---

    // Al empezar cada ronda (incluida la primera).
    void (*AlIniciarRonda)(PartidaTablero& partida) = nullptr;

    // Al caer en una casilla, antes de la regla comun. Devuelve true
    // si el hook resolvio todo (monedas, sonido y partida.textoEvento).
    bool (*ResolverCasilla)(
        PartidaTablero& partida,
        int participante
    ) = nullptr;

    // Cada frame (animacion de gimmickAnim hacia gimmickActivo, etc.).
    void (*ActualizarGimmick)(
        PartidaTablero& partida,
        float deltaTime
    ) = nullptr;

    // MODELO FUTURO: elementos animados del gimmick como modelos.
    // Dibujo 3D del gimmick (dentro de BeginMode3D).
    void (*DibujarGimmick)(const PartidaTablero& partida) = nullptr;

    // Libera mallas y recursos de GPU del tablero (al cerrar el juego).
    void (*DescargarRecursos)() = nullptr;

    // Capa 2D sobre la escena 3D y bajo el HUD (luz calida, petalos...).
    void (*DibujarCapaPantalla)(const PartidaTablero& partida) = nullptr;

    // Decision de un bot en una bifurcacion: indice de conexion,
    // o -1 para elegir al azar.
    int (*DecidirRutaBot)(
        const PartidaTablero& partida,
        int participante
    ) = nullptr;

    // Texto corto de una opcion de bifurcacion para el HUD.
    const char* (*DescribirOpcionRuta)(
        const PartidaTablero& partida,
        int casilla,
        int opcion
    ) = nullptr;
};


// Definiciones concretas (una por archivo Board/TableroXxx.cpp).
const DefinicionTablero& ObtenerDefinicionTableroArboleda();
const DefinicionTablero& ObtenerDefinicionTableroForjaVolcan();


int ObtenerCantidadTableros();

const DefinicionTablero& ObtenerDefinicionTablero(
    IdTablero id
);


// Atajos para la pantalla de seleccion.
const char* ObtenerNombreTablero(IdTablero id);
const char* ObtenerHistoriaTablero(IdTablero id);
const char* ObtenerObjetivoTablero(IdTablero id);
const char* ObtenerGimmickTablero(IdTablero id);
const char* ObtenerDificultadTablero(IdTablero id);
Color ObtenerColorTemaTablero(IdTablero id);


// Dibuja una vista 3D del tablero girando dentro de areaPantalla.
// No usa estado de partida ni carga modelos; usa una RenderTexture
// interna creada de forma perezosa que se libera con
// DescargarVistaPreviaTableros() (llamar al cerrar el juego).
// Debe llamarse entre BeginDrawing y EndDrawing, fuera de BeginMode3D.
void DibujarVistaPreviaTablero(
    IdTablero id,
    Rectangle areaPantalla,
    float tiempo
);

void DescargarVistaPreviaTableros();
