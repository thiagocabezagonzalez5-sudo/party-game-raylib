#pragma once

#include "Core/Participante.h"
#include "Core/ResultadoMinijuego.h"
#include "raylib.h"


//==================================================
// HUD COMUN DE MINIJUEGOS: "EL TENDEDERO"
//==================================================
//
// Estilo «Carteles de playa» (docs/ui_style.md). Una soga cruza el borde
// superior de la pantalla y de ella cuelgan, con pinzas de madera:
//  - una etiqueta por jugador (banderin de su color, J1..J4, BOT o su
//    control, valor y barra opcional). Al eliminarse, la etiqueta pierde
//    una pinza, queda torcida, gris y con el sello «FUERA»;
//  - en el medio, el cartel del reloj: los segundos y una mecha de soga
//    que se consume (o un medidor, si el minijuego no tiene tiempo).
// Durante la cuenta regresiva, el cartel de presentacion va plantado
// abajo y al centro (no tapa a los personajes) y baja antes del «¡YA!».
// Al terminar, el tablon de resultados cuelga del tramo de soga del
// reloj, asi sus sogas no cruzan ninguna etiqueta. Solo
// los anuncios grandes («3, 2, 1, ¡YA!», «¡TIEMPO!», «¡FIN!») van sobre
// el mundo, en texto claro con contorno y sombra dura.
//
// El HUD SOLO LEE: cada frame el minijuego llena DatosHudMinijuego con su
// estado y llama a DibujarHudMinijuego al final de su Dibujar. Las
// animaciones (saltos de puntaje, caida de carteles, pendulo del reloj)
// guardan su estado dentro de HudMinijuego.cpp y usan el reloj del
// EstiloUI compartido; respetan reducirMovimiento.
//==================================================


enum FaseHudMinijuego
{
    HUD_MINIJUEGO_CUENTA = 0,       // cuenta regresiva inicial
    HUD_MINIJUEGO_JUGANDO,
    HUD_MINIJUEGO_FIN
};


enum RelojHudMinijuego
{
    HUD_RELOJ_CUENTA_ATRAS = 0,     // segundos restantes + mecha
    HUD_RELOJ_MEDIDOR               // etiqueta + valor + barra (sin tiempo)
};


// Lo que muestra la etiqueta de un jugador. Los textos se copian aqui
// (no guardar punteros de TextFormat: sus buffers rotan).
struct JugadorHudMinijuego
{
    bool visible = false;           // participa del minijuego

    char valor[16] = "";            // texto a la derecha: "7/15", "64 %"
    int valorNumerico = 0;          // si cambia, la etiqueta salta
    float progreso = -1.0f;         // 0..1 dibuja una barra; < 0 no

    char estado[20] = "";           // reemplaza la segunda linea ("ATURDIDO")
    bool eliminado = false;         // etiqueta descolgada con «FUERA»

    int equipo = -1;                // 0 / 1 en formatos por equipos
    const char* rol = nullptr;      // segunda linea ("OPERADOR")

    // Solo humanos: ayuda bajo la etiqueta. '\n' parte en dos lineas.
    char controles[64] = "";

    char detalleFinal[24] = "";     // columna del tablon final ("12 PTS")
    char detalleDesempate[24] = ""; // criterio de desempate ("2 GOLPES")
};


struct DatosHudMinijuego
{
    const char* titulo = "";
    const char* objetivo = "";      // una linea en el cartel de presentacion

    FaseHudMinijuego fase = HUD_MINIJUEGO_CUENTA;
    float cuentaRestante = 0.0f;    // segundos de preparacion que faltan

    RelojHudMinijuego reloj = HUD_RELOJ_CUENTA_ATRAS;
    float tiempoRestante = 0.0f;
    float tiempoTotal = 1.0f;
    float segundosAlerta = 10.0f;   // por debajo, el cartel se pone rojo

    // HUD_RELOJ_MEDIDOR
    const char* etiquetaMedidor = "";
    char valorMedidor[16] = "";
    float progresoMedidor = 0.0f;

    JugadorHudMinijuego jugadores[MAX_PARTICIPANTES];

    bool finPorTiempo = false;      // «¡TIEMPO!» en lugar de «¡FIN!»
    const char* tituloFinal = nullptr;  // reemplaza «GANA J2» / «EMPATE»
    const char* notaFinal = nullptr;    // linea al pie del tablon (desempate)

    // Ganadores, posiciones y desenlace del tablon final.
    const ResultadoMinijuego* resultado = nullptr;
};


// Deja los datos listos: titulo, objetivo, resultado y jugadores
// visibles segun resultado.participantes[i].participo.
void PrepararDatosHudMinijuego(
    DatosHudMinijuego& datos,
    const char* titulo,
    const char* objetivo,
    const ResultadoMinijuego& resultado
);


// Dibuja todo el HUD (2D). Llamar despues de EndMode3D.
void DibujarHudMinijuego(
    const DatosHudMinijuego& datos,
    const Participante participantes[]
);


// Marca "este frame se dibujo el HUD comun". ZonaPruebas la reinicia
// antes de dibujar el minijuego activo; despues ZonaPruebas y Juego la
// consultan para mostrar sus controles (R, ESC, vuelta al tablero) en la
// tira de ayuda del estilo nuevo en lugar de las capas oscuras viejas,
// que siguen igual para los minijuegos con HUD propio.
void ReiniciarMarcaHudMinijuego();

bool HudMinijuegoDibujado();


// Textos de teclas fieles a Systems/Input.cpp para armar la ayuda.
// Golpe / accion: E, SHIFT DER o X. Movimiento: WASD/FLECHAS, WASD,
// FLECHAS o STICK. Vacio para bots.
const char* TextoBotonGolpeHud(const Participante& participante);

const char* TextoMoverHud(const Participante& participante);
