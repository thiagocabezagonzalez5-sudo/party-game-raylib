#pragma once

#include "Gameplay/TiendaTablero.h"
#include "Systems/Input.h"
#include "UI/RuletaOpciones.h"

#include "raylib.h"


struct PartidaTablero;


//==================================================
// EVENTOS INTERACTIVOS DE CASILLA
//==================================================
//
// Maquina de estados de los eventos de casilla que necesitan varios pasos
// con presentacion propia: CAOS (ruleta), INTERCAMBIO (tres dados) y los
// dos eventos de ISLA ARBOLEDA (objetos del arbol y abejas).
//
// PartidaTablero solo hace tres cosas con este modulo:
//   1. IniciarEventoInteractivo() al resolver la casilla (el resultado se
//      decide AHI, antes de la animacion: lo que se ve nunca cambia lo
//      sorteado);
//   2. en la fase FASE_PARTIDA_TABLERO_EVENTO_INTERACTIVO llama a
//      ActualizarEventoInteractivo() cada frame hasta que `terminado`;
//   3. dibuja con DibujarEventoInteractivo2D / DibujarEventosCasillas3D y
//      pide los textos de fase y ayuda.
// Los efectos (monedas, trofeos, movimiento) se aplican al ENTRAR al paso
// de resultado, para que el HUD anime el cambio mientras se muestra.
//
// Cada paso funciona para humanos (confirmar) y bots (retardo corto) y
// tiene limite de tiempo: la partida nunca queda trabada.

enum TipoEventoInteractivo
{
    EVENTO_INTERACTIVO_NINGUNO = 0,
    EVENTO_INTERACTIVO_CAOS,
    EVENTO_INTERACTIVO_INTERCAMBIO,
    EVENTO_INTERACTIVO_OBJETOS_ARBOL,
    EVENTO_INTERACTIVO_ABEJAS,
    EVENTO_INTERACTIVO_TIENDA
};


// Que espera el evento del jugador que cayo (para el texto de ayuda).
enum PedidoEventoInteractivo
{
    PEDIDO_EVENTO_NADA = 0,
    PEDIDO_EVENTO_GIRAR,
    PEDIDO_EVENTO_DADOS,
    PEDIDO_EVENTO_ELEGIR_OBJETO,
    PEDIDO_EVENTO_TIENDA,
    PEDIDO_EVENTO_CONTINUAR
};


//--------------------------------------------------
// CAOS
//--------------------------------------------------

inline constexpr int CAOS_MONEDAS_PERDIDAS = 20;
inline constexpr int CAOS_MONEDAS_SUSTITUTA = 10;
inline constexpr int CAOS_MONEDAS_GLOBAL = 20;

enum OpcionCaos
{
    CAOS_PIERDE_TROFEO = 0,
    CAOS_PIERDE_MONEDAS,        // -20
    CAOS_PIERDE_TODAS,          // todas las monedas
    CAOS_PARA_TODOS,            // -20 a TODOS los jugadores
    CAOS_PIERDE_POCAS,          // -10: sustituye a la de trofeo sin trofeos
    CANTIDAD_OPCIONES_CAOS
};


//--------------------------------------------------
// INTERCAMBIO
//--------------------------------------------------

inline constexpr int INTERCAMBIO_MONEDAS_MIN = 5;
inline constexpr int INTERCAMBIO_MONEDAS_MAX = 15;

enum AccionIntercambio
{
    INTERCAMBIO_DA = 0,         // la persona 1 le da a la 2
    INTERCAMBIO_RECIBE,         // la persona 1 recibe de la 2
    INTERCAMBIO_CAMBIA          // se permutan los totales
};

enum RecursoIntercambio
{
    RECURSO_INTERCAMBIO_MONEDAS = 0,
    RECURSO_INTERCAMBIO_TROFEOS
};


//--------------------------------------------------
// OBJETOS DEL ARBOL
//--------------------------------------------------

inline constexpr int OBJETO_BUENO_CASILLAS = 3;
inline constexpr int OBJETO_MALO_MONEDAS = 5;


struct EventoInteractivo
{
    TipoEventoInteractivo tipo = EVENTO_INTERACTIVO_NINGUNO;

    bool activo = false;
    bool terminado = false;

    // Quien cayo en la casilla (decide los pasos interactivos).
    int jugador = -1;

    int paso = 0;
    float tiempoPaso = 0.0f;
    float tiempoTotal = 0.0f;

    // Retardo con el que reacciona un bot en el paso de decision.
    float reaccionBot = 0.9f;

    // Texto con el origen del evento (p. ej. "EL GRAN ROBLE").
    const char* fuente = "";

    // Resultado en palabras (se llena al aplicar el efecto).
    char resumen[112] = {};

    // Casillas que PartidaTablero debe avanzar al cerrar el evento.
    int pasosExtra = 0;

    // CAOS
    RuletaOpciones ruleta;
    OpcionCaos opcionSector[MAX_SECTORES_RULETA_OPCIONES] = {};
    OpcionCaos opcionCaos = CAOS_PIERDE_MONEDAS;

    // INTERCAMBIO
    int personaA = -1;
    int personaB = -1;
    AccionIntercambio accion = INTERCAMBIO_DA;
    RecursoIntercambio recurso = RECURSO_INTERCAMBIO_MONEDAS;
    int cantidad = 0;
    int movido = 0;
    bool sinEfecto = false;

    // OBJETOS DEL ARBOL: 0 = izquierda, 1 = derecha.
    int ladoBueno = 0;
    int ladoResaltado = 0;
    int ladoElegido = -1;

    // TIENDA
    EstadoTienda tienda;

    // ABEJAS
    Vector3 origenVuelo{};
    Vector3 destinoVuelo{};
};


//--------------------------------------------------
// FLUJO
//--------------------------------------------------

// Deja listo el evento y decide su resultado. `fuente` es el nombre del
// origen para los textos (nullptr = generico).
void IniciarEventoInteractivo(
    PartidaTablero& partida,
    int jugador,
    TipoEventoInteractivo tipo,
    const char* fuente = nullptr
);

// esAutomatico: el que cayo es bot o un humano desconectado que ya paso a
// manos de la IA. puedeDecidir: humano conectado.
void ActualizarEventoInteractivo(
    PartidaTablero& partida,
    float deltaTime,
    bool esAutomatico,
    bool puedeDecidir,
    const InputSeleccionParticipante& entrada
);

bool EventoInteractivoActivo(const PartidaTablero& partida);

// true mientras el paso actual espera una decision del jugador.
PedidoEventoInteractivo PedidoDelEventoInteractivo(const PartidaTablero& partida);

// Segunda linea del cartel del turno.
void TextoEventoInteractivo(
    const PartidaTablero& partida,
    char* destino,
    int capacidad
);


//--------------------------------------------------
// REGLAS (separadas para poder probarlas sin pantalla)
//--------------------------------------------------

// Elige una opcion con efecto real para quien cayo (sin trofeos no sale
// la de trofeo; sin monedas no salen las de monedas).
OpcionCaos ElegirOpcionCaos(
    const PartidaTablero& partida,
    int jugador
);

// Aplica la opcion (topes en 0) y deja el texto en `resumen`.
void AplicarOpcionCaos(
    PartidaTablero& partida,
    int jugador,
    OpcionCaos opcion,
    char* resumen,
    int capacidad
);

// Sortea persona 1, accion, recurso, cantidad y persona 2.
void SortearIntercambio(
    const PartidaTablero& partida,
    EventoInteractivo& evento
);

// Aplica el intercambio sorteado (nadie queda en negativo) y deja el texto.
void AplicarIntercambio(
    PartidaTablero& partida,
    EventoInteractivo& evento
);


//--------------------------------------------------
// DIBUJO
//--------------------------------------------------

// Capa 2D del evento (velo, ruleta, dados, objetos). Va bajo el tendedero
// del HUD y el cartel del turno.
void DibujarEventoInteractivo2D(const PartidaTablero& partida);

// Capa 3D (abejas y cofre del REGALO). Dentro de BeginMode3D.
void DibujarEventosCasillas3D(const PartidaTablero& partida);
