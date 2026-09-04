#pragma once

#include "Tipos.h"

#include "raylib.h"

#include "Core/ConfiguracionJuego.h"
#include "Core/Participante.h"
#include "Core/WindowUtils.h"

#include "Systems/Audio.h"

#include "UI/MenuPrincipal.h"
#include "UI/MenuConfiguracion.h"
#include "UI/MenuModoJuego.h"
#include "UI/PantallaLogo.h"
#include "UI/SeleccionMinijuegos.h"
#include "UI/SeleccionPersonajes.h"

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

    ZonaPruebas zonaPruebas;

    AudioJuego audio;
    ConfiguracionJuego config;

    Participante participantes[MAX_PARTICIPANTES];

    // En el juego final siempre habra cuatro puestos activos.
    // Antes de confirmar personajes puede ser 0.
    int cantidadParticipantes = 0;

    const char* rutaConfiguracion = "config.ini";

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

    void Inicializar();
    void InicializarResoluciones();

    void Actualizar(
        float deltaTime
    );

    void Dibujar();

    bool DebeCerrar();
    void Descargar();
};
