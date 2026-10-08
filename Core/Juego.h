#pragma once

#include "Tipos.h"
#include "Config.h"

#include "raylib.h"

#include "Core/ConfiguracionJuego.h"
#include "Core/Participante.h"
#include "Core/WindowUtils.h"

#include "Systems/Audio.h"

#include "UI/MenuPrincipal.h"
#include "UI/MenuConfiguracion.h"
#include "UI/MenuModoJuego.h"
#include "UI/PantallaLogo.h"
#include "UI/RuletaMinijuegos.h"
#include "UI/FlujoPartida.h"
#include "UI/SeleccionMinijuegos.h"
#include "UI/SeleccionPersonajes.h"

#include "Gameplay/PartidaTablero.h"
#include "Gameplay/ZonaPruebas.h"


struct Juego
{
    static const int MAX_RESOLUCIONES = 10;
    static const int CANTIDAD_OPCIONES_FPS = 5;

    EstadoJuego estado = ESTADO_LOGO;
    bool cerrarJuego = false;

    MenuPrincipal menuPrincipal;
    MenuConfiguracion menuConfiguracion;
    MenuModoJuego menuModoJuego;
    PantallaLogo pantallaLogo;
    SeleccionPersonajes seleccionPersonajes;
    SeleccionMinijuegos seleccionMinijuegos;
    RuletaMinijuegos ruletaMinijuegos;

    // Pantallas de la partida de tablero (ver UI/FlujoPartida.h).
    SeleccionTablero seleccionTablero;
    ConfiguracionPartida configuracionPartida;
    IntroTablero introTablero;
    OrdenTurno ordenTurno;
    ResultadosPartida resultadosPartida;

    ZonaPruebas zonaPruebas;
    PartidaTablero partidaTablero;

    AudioJuego audio;
    ConfiguracionJuego config;

    Participante participantes[MAX_PARTICIPANTES];

    // En el juego final siempre habra cuatro puestos activos.
    // Antes de confirmar personajes puede ser 0.
    int cantidadParticipantes = 0;

    Resolucion resoluciones[MAX_RESOLUCIONES];
    int cantidadResoluciones = 0;

    int opcionesFPS[CANTIDAD_OPCIONES_FPS] =
    {
        30,
        60,
        120,
        144,
        240
    };

    bool menuPreparado = false;
    bool cargaMenuSolicitada = false;

    // Se usa solo cuando el modo Tablero lanza el minijuego
    // que cierra cada ronda.
    float tiempoResultadoMinijuegoTablero = 0.0f;

    // Red de seguridad: una ronda de tablero que nunca termina vuelve sola.
    float tiempoRondaMinijuegoTablero = 0.0f;

    // Tiempo que llevan todos listos en la seleccion de personajes: hace
    // falta confirmar de nuevo para seguir (no se avanza solo).
    float tiempoTodosListos = 0.0f;

    // Fundido corto desde negro al cambiar de pantalla (1 = negro).
    float fundidoEntrada = 0.0f;

    // Confirmacion de salida de la partida (ESC en el tablero).
    bool confirmandoSalida = false;
    int opcionSalida = 0;
    float tiempoSalida = 0.0f;
    LectorEntradaFlujo lectorSalida;

    void Inicializar();
    void InicializarResoluciones();

    void Actualizar(
        float deltaTime
    );

    void Dibujar();

    bool DebeCerrar();
    void Descargar();
};
