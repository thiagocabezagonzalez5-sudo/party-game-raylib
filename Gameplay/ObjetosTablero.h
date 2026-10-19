#pragma once

#include "Systems/Input.h"

#include "raylib.h"


struct PartidaTablero;


//==================================================
// OBJETOS DEL TABLERO
//==================================================
//
// Catalogo data-driven de objetos (nombres y arte propios, dibujados con
// primitivas 2D), inventario de 3 espacios por jugador y las reglas de su
// uso. Todo son funciones libres sobre PartidaTablero; los datos de cada
// jugador viven en EstadoJugadorPartidaTablero (inventario y efectos
// armados).
//
//  - BOTAS:      +3 casillas en la proxima tirada (se usan antes de tirar).
//  - DADO DOBLE: la proxima tirada son dos dados de 6 que se suman.
//  - ESCUDO:     bloquea la proxima perdida de monedas o trofeos (casilla
//                negativa, azar, CAOS, objeto malo, intercambio, iman).
//                Se consume solo; no se "usa".
//  - IMAN:       roba 5 monedas a un rival (el humano elige, el bot al
//                mas rico).
//
// Para agregar un objeto: sumar su id y su fila en CATALOGO (cpp), dibujar
// su icono y resolver su efecto en UsarObjeto().

inline constexpr int MAX_OBJETOS_INVENTARIO = 3;
inline constexpr int OBJETO_VACIO = -1;

inline constexpr int BOTAS_CASILLAS_EXTRA = 3;
inline constexpr int DADO_DOBLE_CARAS = 6;
inline constexpr int IMAN_MONEDAS_ROBADAS = 5;

enum IdObjetoTablero
{
    OBJETO_BOTAS = 0,
    OBJETO_DADO_DOBLE,
    OBJETO_ESCUDO,
    OBJETO_IMAN,
    CANTIDAD_OBJETOS_TABLERO
};


struct DefinicionObjetoTablero
{
    const char* nombre;
    const char* descripcion;
    int precio;                 // monedas en la tienda (5 a 10)
    bool barato;                // puede salir de un REGALO
    bool usoManual;             // se usa antes de tirar (el escudo es automatico)
    Color color;
};

const DefinicionObjetoTablero& ObtenerDefinicionObjeto(int id);


//--------------------------------------------------
// INVENTARIO (arreglo de MAX_OBJETOS_INVENTARIO ids o OBJETO_VACIO)
//--------------------------------------------------

int ContarObjetosInventario(const int inventario[]);
bool InventarioLleno(const int inventario[]);
bool TieneObjetoInventario(const int inventario[], int id);

// Pone el objeto en el primer espacio libre. false = inventario lleno.
bool AgregarObjetoInventario(int inventario[], int id);

// Vacia el espacio y devuelve el id que habia (o OBJETO_VACIO).
int QuitarObjetoInventario(int inventario[], int espacio);


//--------------------------------------------------
// REGLAS
//--------------------------------------------------

// Se puede usar el objeto del espacio ahora mismo (iman: hay a quien
// robar; botas/dado doble: no estan ya armados).
bool ObjetoUsableAhora(const PartidaTablero& partida, int jugador, int espacio);

// Rivales validos para el iman (con monedas). Devuelve cuantos.
int ListarRivalesImanes(const PartidaTablero& partida, int jugador, int rivales[]);

// Usa el objeto del espacio: lo consume y aplica su efecto. `rival` solo
// importa para el iman. Deja el resultado en palabras en `resumen`.
bool UsarObjeto(
    PartidaTablero& partida,
    int jugador,
    int espacio,
    int rival,
    char* resumen,
    int capacidad
);

// Si el jugador tiene escudo lo consume, avisa en pantalla y devuelve true:
// quien llama debe cancelar la perdida. Llamar SOLO cuando la perdida
// realmente ocurriria (monedas o trofeos > 0).
bool EscudoAbsorbePerdida(PartidaTablero& partida, int jugador);

// Tirada del jugador con los efectos armados (botas, dado doble). Consume
// los efectos y deja el detalle ("4 + 6 + BOTAS 3") en `detalle`.
int TirarDadoDeJugador(
    PartidaTablero& partida,
    int jugador,
    char* detalle,
    int capacidad
);

// Cartel temporal de objetos sobre el mundo ("¡EL ESCUDO DE J2 ...!").
void AvisarObjeto(PartidaTablero& partida, const char* texto);


//--------------------------------------------------
// BOTS
//--------------------------------------------------

// Que objeto usa el bot antes de tirar: espacio del inventario o -1.
// `rival` queda con el objetivo del iman.
int DecidirObjetoBot(
    const PartidaTablero& partida,
    int jugador,
    int& rival
);

// Pasos minimos desde una casilla hasta otra (-1 si no hay camino).
int DistanciaEntreCasillas(
    const PartidaTablero& partida,
    int desde,
    int hasta
);


//--------------------------------------------------
// MENU DE USO (antes de tirar el dado)
//--------------------------------------------------

struct MenuObjetosTablero
{
    bool activo = false;
    bool terminado = false;

    // 0 = elegir objeto, 1 = elegir rival (iman).
    int paso = 0;
    int seleccion = 0;
    int espacioIman = -1;

    int rivales[MAX_PARTICIPANTES] = {};
    int cantidadRivales = 0;
    int rivalSeleccion = 0;

    float tiempo = 0.0f;
    float tiempoMensaje = 0.0f;
    char mensaje[96] = {};
};

// Abre el menu para quien tiene el turno.
void IniciarMenuObjetos(PartidaTablero& partida, int jugador);

void ActualizarMenuObjetos(
    PartidaTablero& partida,
    float deltaTime,
    bool esAutomatico,
    bool puedeDecidir,
    const InputSeleccionParticipante& entrada
);

// El jugador con turno tiene al menos un objeto que se pueda usar ahora.
bool HayObjetoUsableAhora(const PartidaTablero& partida, int jugador);


//--------------------------------------------------
// DIBUJO
//--------------------------------------------------

// Icono del objeto en un disco de `radio` pixeles centrado en `centro`.
void DibujarIconoObjeto(int id, Vector2 centro, float radio, float alfa);

// Un espacio del inventario: placa con borde y el icono (o vacio).
void DibujarEspacioObjeto(Rectangle rect, int id, bool resaltar, float alfa);

// Menu de uso (velo, espacios, rival del iman).
void DibujarMenuObjetos2D(const PartidaTablero& partida);

// Inventario de todos los jugadores, abajo a la izquierda.
void DibujarInventariosHud(const PartidaTablero& partida);

// Cartel de aviso de objeto (escudo, iman, uso del bot...).
void DibujarAvisoObjeto(const PartidaTablero& partida);

void TextoMenuObjetos(
    const PartidaTablero& partida,
    char* destino,
    int capacidad
);
