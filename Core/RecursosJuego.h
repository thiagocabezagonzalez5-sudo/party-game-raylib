#pragma once


//==================================================
// RUTAS COMPARTIDAS DE RECURSOS
//==================================================
//
// Este archivo es la unica fuente de verdad para las rutas de Assets.
// Si un recurso cambia de carpeta o de nombre, se corrige aqui y no en
// cada minijuego/pantalla que lo utiliza.
//==================================================

inline constexpr const char* DIRECTORIO_RECURSOS =
    "Assets";

inline constexpr const char* RUTA_CONFIGURACION_JUEGO =
    "config.ini";


//==================================================
// MODELO GENERAL DEL JUGADOR
//==================================================

inline constexpr const char* RUTA_MODELO_JUGADOR_3D =
    "Assets/Modelos/Jugador_Raylib_Normalizado.glb";

inline constexpr float ESCALA_MODELO_JUGADOR_3D =
    0.25f;

inline constexpr float ROTACION_X_MODELO_JUGADOR_3D =
    90.0f;


//==================================================
// TEXTURAS GENERALES
//==================================================

inline constexpr const char* RUTA_TEXTURA_PISO =
    "Assets/Texturas/Piso.png";

inline constexpr const char* RUTA_TEXTURA_ORO =
    "Assets/Texturas/Oro.png";

inline constexpr const char* RUTA_TEXTURA_ACERO =
    "Assets/Texturas/Acero.png";

inline constexpr const char* RUTA_TEXTURA_JUGADOR =
    "Assets/Texturas/Jugador.png";


//==================================================
// UI
//==================================================

inline constexpr const char* RUTA_LOGO_CREADOR =
    "Assets/UI/LogoCreador.png";

inline constexpr const char* RUTA_FONDO_MENU =
    "Assets/UI/MenuFondo.gif";

inline constexpr const char* RUTA_FONDO_MENU_ALTERNATIVO =
    "Assets/UI/MenuFondoCristina.gif";


//==================================================
// AUDIO
//==================================================

inline constexpr const char* RUTA_MUSICA_MENU =
    "Assets/Audio/Musica/musicaMenu.mp3";

inline constexpr const char* RUTA_SFX_UI_MOVER_WAV =
    "Assets/Audio/SFX/ui_mover.wav";
inline constexpr const char* RUTA_SFX_UI_MOVER_MP3 =
    "Assets/Audio/SFX/ui_mover.mp3";

inline constexpr const char* RUTA_SFX_UI_CONFIRMAR_WAV =
    "Assets/Audio/SFX/ui_confirmar.wav";
inline constexpr const char* RUTA_SFX_UI_CONFIRMAR_MP3 =
    "Assets/Audio/SFX/ui_confirmar.mp3";

inline constexpr const char* RUTA_SFX_CUENTA_REGRESIVA_WAV =
    "Assets/Audio/SFX/cuenta_regresiva.wav";
inline constexpr const char* RUTA_SFX_CUENTA_REGRESIVA_MP3 =
    "Assets/Audio/SFX/cuenta_regresiva.mp3";

inline constexpr const char* RUTA_SFX_INICIO_MINIJUEGO_WAV =
    "Assets/Audio/SFX/inicio_minijuego.wav";
inline constexpr const char* RUTA_SFX_INICIO_MINIJUEGO_MP3 =
    "Assets/Audio/SFX/inicio_minijuego.mp3";

inline constexpr const char* RUTA_SFX_RECOGER_NUCLEO_WAV =
    "Assets/Audio/SFX/recoger_nucleo.wav";
inline constexpr const char* RUTA_SFX_RECOGER_NUCLEO_MP3 =
    "Assets/Audio/SFX/recoger_nucleo.mp3";

inline constexpr const char* RUTA_SFX_RECOGER_NUCLEO_ESPECIAL_WAV =
    "Assets/Audio/SFX/recoger_nucleo_especial.wav";
inline constexpr const char* RUTA_SFX_RECOGER_NUCLEO_ESPECIAL_MP3 =
    "Assets/Audio/SFX/recoger_nucleo_especial.mp3";

inline constexpr const char* RUTA_SFX_ALERTA_TIEMPO_WAV =
    "Assets/Audio/SFX/alerta_tiempo.wav";
inline constexpr const char* RUTA_SFX_ALERTA_TIEMPO_MP3 =
    "Assets/Audio/SFX/alerta_tiempo.mp3";

inline constexpr const char* RUTA_SFX_RESULTADO_WAV =
    "Assets/Audio/SFX/resultado.wav";
inline constexpr const char* RUTA_SFX_RESULTADO_MP3 =
    "Assets/Audio/SFX/resultado.mp3";


//==================================================
// TEXTURAS OPCIONALES DE MINIJUEGOS
//==================================================

inline constexpr const char* RUTA_TEXTURA_LAVA_SUELO =
    "Assets/Texturas/Minijuegos/ColorSeguro/lava_suelo.png";
inline constexpr const char* RUTA_TEXTURA_LAVA_FONDO =
    "Assets/Texturas/Minijuegos/ColorSeguro/fondo_volcanes.png";

inline constexpr const char* RUTA_TEXTURA_NIEVE_SUELO =
    "Assets/Texturas/Minijuegos/Pelotas/nieve_suelo.png";
inline constexpr const char* RUTA_TEXTURA_NIEVE_FONDO =
    "Assets/Texturas/Minijuegos/Pelotas/fondo_montanas.png";

inline constexpr const char* RUTA_TEXTURA_CUEVA_ROCA =
    "Assets/Texturas/Minijuegos/RefugioTaladros/roca_cueva.png";
inline constexpr const char* RUTA_TEXTURA_CUEVA_TALADRO =
    "Assets/Texturas/Minijuegos/RefugioTaladros/metal_taladro.png";

inline constexpr const char* RUTA_TEXTURA_MAGNETICA_METAL =
    "Assets/Texturas/Minijuegos/TormentaMagnetica/metal_arena.png";
inline constexpr const char* RUTA_TEXTURA_MAGNETICA_ENERGIA =
    "Assets/Texturas/Minijuegos/TormentaMagnetica/energia.png";


//==================================================
// SELECCION DE PERSONAJES
//==================================================

inline constexpr const char* RUTA_ICONO_PERSONAJE_1 =
    "Assets/Personajes/TungTung/Icono.png";
inline constexpr const char* RUTA_RETRATO_PERSONAJE_1 =
    "Assets/Personajes/TungTung/Retrato.png";

inline constexpr const char* RUTA_ICONO_PERSONAJE_2 =
    "Assets/Personajes/Personaje2/Icono.png";
inline constexpr const char* RUTA_RETRATO_PERSONAJE_2 =
    "Assets/Personajes/Personaje2/Retrato.png";

inline constexpr const char* RUTA_ICONO_PERSONAJE_3 =
    "Assets/Personajes/Personaje3/Icono.png";
inline constexpr const char* RUTA_RETRATO_PERSONAJE_3 =
    "Assets/Personajes/Personaje3/Retrato.png";

inline constexpr const char* RUTA_ICONO_PERSONAJE_4 =
    "Assets/Personajes/Personaje4/Icono.png";
inline constexpr const char* RUTA_RETRATO_PERSONAJE_4 =
    "Assets/Personajes/Personaje4/Retrato.png";
