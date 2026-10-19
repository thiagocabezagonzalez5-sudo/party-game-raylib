#pragma once

#include "Systems/Input.h"

#include "raylib.h"


struct PartidaTablero;


//==================================================
// TIENDA DEL TABLERO
//==================================================
//
// La casilla TIENDA abre un evento interactivo (EVENTO_INTERACTIVO_TIENDA,
// ver EventosCasillas.h): un comerciante ofrece 3 objetos AL AZAR distintos
// del catalogo en cada visita. El jugador puede comprar varios hasta salir.
// No se compra sin monedas suficientes ni con el inventario lleno.
//
// Humano: izquierda/derecha elige (3 objetos y "SALIR"), confirmar compra o
// sale, cancelar sale. Bot: espera su retardo, compra a lo sumo 2 objetos
// con una regla simple y sale.

inline constexpr int OBJETOS_POR_OFERTA_TIENDA = 3;
inline constexpr int COMPRAS_MAXIMAS_BOT = 2;


struct EstadoTienda
{
    int oferta[OBJETOS_POR_OFERTA_TIENDA] = { -1, -1, -1 };
    int cantidadOferta = 0;

    // 0..cantidadOferta-1 = objeto, cantidadOferta = salir.
    int seleccion = 0;

    int compras = 0;

    // Bot: segundo (del paso) en que hace su proxima accion.
    float proximaAccionBot = 1.0f;

    // Humano: segundos sin tocar nada (a los 20 s se va).
    float tiempoSinAccion = 0.0f;

    // Frase del comerciante y su estado (0 neutro, 1 ok, 2 error).
    char mensaje[96] = {};
    int tipoMensaje = 0;
    float tiempoMensaje = 0.0f;

    // Instante de la ultima compra (para el rebote del objeto).
    float tiempoCompra = 99.0f;
    int ultimoComprado = -1;

    bool terminado = false;
};


// Sortea la oferta y la frase de bienvenida.
void IniciarTienda(PartidaTablero& partida, int jugador, EstadoTienda& tienda);

// Por que no se puede comprar ahora (nullptr = se puede).
const char* MotivoNoCompra(const PartidaTablero& partida, int jugador, int id);

// Compra el objeto: descuenta, lo guarda y suena. false = no se pudo.
bool ComprarObjetoTienda(
    PartidaTablero& partida,
    int jugador,
    int id,
    EstadoTienda& tienda
);

// Indice de la oferta que compraria el bot, o -1 (sale).
int DecidirCompraBot(
    const PartidaTablero& partida,
    int jugador,
    const EstadoTienda& tienda
);

void ActualizarTienda(
    PartidaTablero& partida,
    int jugador,
    EstadoTienda& tienda,
    float deltaTime,
    bool esAutomatico,
    bool puedeDecidir,
    const InputSeleccionParticipante& entrada
);

// Velo, comerciante, ofertas y mochila.
void DibujarTienda2D(
    const PartidaTablero& partida,
    const EstadoTienda& tienda,
    int jugador,
    float tiempoTotal
);

void TextoTienda(
    const PartidaTablero& partida,
    const EstadoTienda& tienda,
    int jugador,
    char* destino,
    int capacidad
);
