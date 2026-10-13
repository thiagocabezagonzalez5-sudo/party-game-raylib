#pragma once

#include "UI/EstiloUI.h"


//==================================================
// ESTILO UI GLOBAL
//==================================================
//
// Instancia unica de EstiloUI que comparten todas las pantallas del
// juego, para no generar las texturas de madera y cuerda una vez por
// pantalla.
//
// - Carga perezosa: el primer ObtenerEstiloUI() con la ventana abierta
//   genera las texturas. Antes de InitWindow devuelve el estilo sin
//   cargar (no se puede crear texturas sin contexto OpenGL).
// - Juego actualiza el tiempo y copia config.reducirMovimiento cada frame.
// - DescargarEstiloUIGlobal() se llama al cerrar, antes de CloseWindow.
//==================================================

EstiloUI& ObtenerEstiloUI();

// Descarga simetrica de la carga perezosa. Seguro llamar varias veces.
void DescargarEstiloUIGlobal();
