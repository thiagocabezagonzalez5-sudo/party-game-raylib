#pragma once

#include "UI/EstiloUI.h"


//==================================================
// PANTALLA DE DEMO DEL ESTILO UI
//==================================================
//
// Muestra todos los componentes de UI/EstiloUI en una pantalla de
// configuracion de ejemplo (3 pestanas). No toca el flujo del juego:
// se ejecuta con Tests/DemoEstiloUI.ps1.
//
// Controles: arriba/abajo cambia de elemento, izquierda/derecha ajusta,
// aceptar confirma, Q/E (LB/RB) cambia de pestana, volver sale.
//==================================================

inline constexpr int DEMO_PESTANAS = 3;
inline constexpr int DEMO_FILAS = 3;


struct DemoEstiloUI
{
    EstiloUI estilo;

    PanelUI panel;
    ColgadoUI colgadoTitulo;
    BotonUI pestanas[DEMO_PESTANAS];
    int pestanaActiva = 0;

    // Filas reutilizadas por pestana.
    SelectorUI selectores[2];
    PalancaUI palancas[3];
    InterruptorUI interruptores[4];

    int opcionSelector[2] = {};

    BotonUI botonVolver;
    BotonUI botonAplicar;

    // Foco: 0..DEMO_FILAS-1 = filas, DEMO_FILAS = fila de botones.
    EnfoqueUI enfoque;
    int botonConFoco = 0;

    // Las filas del panel solo aparecen cuando las pestanas dejaron de
    // balancearse: asi el cartel entrante nunca tapa la primera fila.
    bool filasListas = false;

    bool salir = false;
    float tiempo = 0.0f;

    void Inicializar();
    void Actualizar(float deltaTime, const EntradaUI& entrada);
    void Dibujar() const;
    void Descargar();
};
