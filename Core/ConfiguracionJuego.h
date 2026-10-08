#pragma once

#include "Core/WindowUtils.h"
#include "Systems/CalidadGrafica.h"


enum ModoTeclado
{
    TECLADO_COMPLETO = 0,
    TECLADO_DIVIDIDO
};


// Zona muerta del stick (porcentaje de inclinacion ignorado). 20 es el
// valor que ya usaba el movimiento analogico antes de ser configurable.
inline constexpr int ZONA_MUERTA_STICK_MINIMA = 5;
inline constexpr int ZONA_MUERTA_STICK_MAXIMA = 35;
inline constexpr int ZONA_MUERTA_STICK_PASO = 5;
inline constexpr int ZONA_MUERTA_STICK_DEFECTO = 20;

// Limita al rango permitido y redondea al paso.
int LimitarZonaMuertaStick(
    int porcentaje
);


// Preset grafico elegido. PRESET_GRAFICO_PERSONALIZADO = las cuatro
// calidades individuales no coinciden con BAJO / MEDIO / ALTO.
enum PresetGrafico
{
    PRESET_GRAFICO_BAJO = 0,
    PRESET_GRAFICO_MEDIO,
    PRESET_GRAFICO_ALTO,
    PRESET_GRAFICO_PERSONALIZADO,
    CANTIDAD_PRESETS_GRAFICOS
};


struct ConfiguracionJuego
{
    ModoVentana modoVentana = MODO_PANTALLA_COMPLETA;

    int indiceResolucion = 0;
    int indiceFPS = 1;

    bool mostrarFPS = true;

    // Sincronizacion vertical (se aplica con AplicarVSync de WindowUtils).
    bool vsync = false;

    // Accesibilidad: anula el temblor de camara en minijuegos.
    bool reducirMovimiento = false;

    // Calidad grafica (valores de NivelCalidadGrafica: 0 baja .. 2 alta).
    int presetGrafico = PRESET_GRAFICO_ALTO;
    int calidadParticulas = CALIDAD_ALTA;
    int calidadDecoracion = CALIDAD_ALTA;
    int calidadEfectos = CALIDAD_ALTA;
    int calidadSombras = CALIDAD_ALTA;

    float volumenMusica = 0.35f;
    float volumenSonidos = 0.60f;

    ModoTeclado modoTeclado = TECLADO_DIVIDIDO;

    // Zona muerta radial del stick izquierdo de todos los mandos, en
    // porcentaje (ZONA_MUERTA_STICK_MINIMA..MAXIMA, de 5 en 5). Juego la
    // aplica con EstablecerZonaMuertaStick (Systems/Input).
    int zonaMuertaStick = ZONA_MUERTA_STICK_DEFECTO;
};


// Opciones graficas de la configuracion, listas para
// EstablecerOpcionesCalidadGrafica (Juego las aplica al iniciar).
OpcionesCalidadGrafica ObtenerOpcionesCalidadDesdeConfiguracion(
    const ConfiguracionJuego& config
);


// Recalcula presetGrafico a partir de las cuatro calidades individuales.
void ActualizarPresetGrafico(
    ConfiguracionJuego& config
);


bool CargarConfiguracion(
    const char* ruta,
    ConfiguracionJuego& config
);


bool GuardarConfiguracion(
    const char* ruta,
    const ConfiguracionJuego& config
);


// Deja todos los campos dentro de rangos validos. Los indices de resolucion
// y FPS se ajustan a las listas reales; si el indice de resolucion es
// invalido se usa indiceNativo. Tambien corrige NaN en los volumenes.
void NormalizarConfiguracion(
    ConfiguracionJuego& config,
    int cantidadResoluciones,
    int cantidadOpcionesFPS,
    int indiceNativo
);
