#pragma once

#include "Core/RecursosJuego.h"


//==================================================
// CONFIGURACION GLOBAL
//==================================================
//
// Las constantes compartidas viven en un unico lugar. Se usan inline
// constexpr para que este header pueda incluirse desde cualquier unidad de
// compilacion sin crear definiciones globales duplicadas.
//==================================================

inline constexpr int ANCHO_INICIAL = 1280;
inline constexpr int ALTO_INICIAL = 720;

inline constexpr const char* TITULO_JUEGO =
    "Juego de Party";

inline constexpr int CANTIDAD_MAXIMA_JUGADORES = 4;

inline constexpr float GRAVEDAD = 20.0f;
inline constexpr float DELTA_TIME_MAXIMO = 0.1f;


//==================================================
// ALIASES DE COMPATIBILIDAD
//==================================================
//
// Conservamos los nombres antiguos de texturas para no romper codigo que
// todavia los use. La ruta real, sin embargo, existe una sola vez en
// Core/RecursosJuego.h.
//==================================================

inline constexpr const char* TEXTURA_PISO =
    RUTA_TEXTURA_PISO;

inline constexpr const char* TEXTURA_BLOQUE_1 =
    RUTA_TEXTURA_ORO;

inline constexpr const char* TEXTURA_BLOQUE_2 =
    RUTA_TEXTURA_ACERO;

inline constexpr const char* TEXTURA_JUGADOR =
    RUTA_TEXTURA_JUGADOR;

inline constexpr const char* TEXTURA_MODELO_JUGADOR_3D =
    "";
