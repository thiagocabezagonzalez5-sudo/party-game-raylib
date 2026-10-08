#pragma once


//==================================================
// CALIDAD GRAFICA
//==================================================
//
// Opciones graficas globales. Las elige el jugador en Configuracion
// (se guardan en config.ini a traves de ConfiguracionJuego) y Juego las
// aplica al iniciar y al cambiarlas. Tableros y minijuegos las CONSULTAN
// para decidir cuanta decoracion, particulas y efectos dibujar.
//
// ALTO debe conservar el aspecto visual de referencia: los niveles
// inferiores solo recortan trabajo que no cambia la jugabilidad.
//==================================================

enum NivelCalidadGrafica
{
    CALIDAD_BAJA = 0,
    CALIDAD_MEDIA,
    CALIDAD_ALTA,
    CANTIDAD_NIVELES_CALIDAD
};


struct OpcionesCalidadGrafica
{
    // Particulas ambientales (petalos, brasas, humo, chispas, confeti).
    NivelCalidadGrafica particulas = CALIDAD_ALTA;

    // Props decorativos y elementos lejanos fuera del area jugable.
    NivelCalidadGrafica decoracion = CALIDAD_ALTA;

    // Efectos secundarios (agua animada, destellos, haces de luz).
    NivelCalidadGrafica efectos = CALIDAD_ALTA;

    // Sombras simuladas (manchas de SombrasRetro).
    NivelCalidadGrafica sombras = CALIDAD_ALTA;
};


inline OpcionesCalidadGrafica& ObtenerOpcionesCalidadGrafica()
{
    static OpcionesCalidadGrafica opciones;
    return opciones;
}


inline void EstablecerOpcionesCalidadGrafica(
    const OpcionesCalidadGrafica& opciones
)
{
    ObtenerOpcionesCalidadGrafica() = opciones;
}


// Preset completo (BAJO / MEDIO / ALTO) aplicado a todas las categorias.
inline OpcionesCalidadGrafica CrearPresetCalidadGrafica(
    NivelCalidadGrafica nivel
)
{
    OpcionesCalidadGrafica opciones;
    opciones.particulas = nivel;
    opciones.decoracion = nivel;
    opciones.efectos = nivel;
    opciones.sombras = nivel;
    return opciones;
}


// Factor 0..1 para escalar cantidades (p.ej. numero de particulas):
// BAJA = 0.25, MEDIA = 0.6, ALTA = 1.
inline float FactorCalidadGrafica(
    NivelCalidadGrafica nivel
)
{
    switch (nivel)
    {
        case CALIDAD_BAJA: return 0.25f;
        case CALIDAD_MEDIA: return 0.6f;
        case CALIDAD_ALTA: return 1.0f;
        case CANTIDAD_NIVELES_CALIDAD: break;
    }

    return 1.0f;
}


// Atajos de lectura para quien dibuja.
inline NivelCalidadGrafica CalidadParticulas()
{
    return ObtenerOpcionesCalidadGrafica().particulas;
}

inline NivelCalidadGrafica CalidadDecoracion()
{
    return ObtenerOpcionesCalidadGrafica().decoracion;
}

inline NivelCalidadGrafica CalidadEfectos()
{
    return ObtenerOpcionesCalidadGrafica().efectos;
}

inline NivelCalidadGrafica CalidadSombras()
{
    return ObtenerOpcionesCalidadGrafica().sombras;
}
