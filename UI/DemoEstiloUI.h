#pragma once

#include "UI/EstiloUI.h"


//==================================================
// PANTALLA DE DEMO DEL ESTILO UI
//==================================================
//
// Muestra todos los componentes de UI/EstiloUI en una pantalla de
// configuracion de ejemplo (6 pestanas, una tarjeta y dos avisos
// modales). La pestana EXTRAS muestra la segunda tanda: texto con
// contorno, etiquetas, parrafo, sellos, velo, banderines y marco con
// recorte. La pestana HUD muestra la tercera: pinzas, banderin colgante,
// placa con marco, texto alineado, iconos de moneda y trofeo, y el alfa
// de la tarjeta y el banderin. No toca el flujo del juego: se ejecuta con
// Tests/DemoEstiloUI.ps1.
//
// Controles: arriba/abajo cambia de elemento, izquierda/derecha ajusta,
// aceptar confirma, Q/E (LB/RB) cambia de pestana, volver pregunta si
// salir. En EXTRAS: izquierda/derecha cambia la alineacion del parrafo y
// aceptar muestra u oculta el velo. En HUD: izquierda/derecha cambia el
// alfa (25 % a 100 %).
//==================================================

inline constexpr int DEMO_PESTANAS = 6;
inline constexpr int DEMO_FILAS = 3;


struct DemoEstiloUI
{
    EstiloUI estilo;

    PanelUI panel;
    ColgadoUI colgadoTitulo;
    BotonUI pestanas[DEMO_PESTANAS];
    int pestanaActiva = 0;

    // Filas reutilizadas por pestana.
    SelectorUI selectores[3];
    PalancaUI palancas[4];
    InterruptorUI interruptores[4];
    TarjetaUI tarjeta;

    int opcionSelector[3] = {};

    // Pestana EXTRAS (segunda tanda de componentes).
    TarjetaUI tarjetaParrafo;
    int alineacionParrafo = 0;      // AlineacionUI
    VeloUI velo;

    // Pestana HUD (tercera tanda): tarjeta que se funde y alfa elegido
    // en cuartos (1..4 = 25 %..100 %).
    TarjetaUI tarjetaAlfa;
    int cuartosAlfa = 4;

    BotonUI botonVolver;
    BotonUI botonAplicar;

    // Avisos: salir sin aplicar (VOLVER / Esc) y confirmar con cuenta
    // atras (APLICAR).
    ModalUI modalSalir;
    ModalUI modalAplicar;
    float tiempoConfirmacion = 0.0f;

    // Foco: 0..filas con foco - 1 = filas, el siguiente = fila de botones.
    EnfoqueUI enfoque;
    int botonConFoco = 0;

    // Las filas del panel solo aparecen cuando las pestanas dejaron de
    // balancearse: asi el cartel entrante nunca tapa la primera fila.
    bool filasListas = false;

    bool salir = false;
    float tiempo = 0.0f;

    void Inicializar(bool reducirMovimiento = false);
    void Actualizar(float deltaTime, const EntradaUI& entrada);
    void Dibujar() const;
    void Descargar();
};
