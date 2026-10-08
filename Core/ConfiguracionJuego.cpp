#include "Core/ConfiguracionJuego.h"

#include <fstream>
#include <string>
#include <cstdlib>


static int LeerEntero(
    const std::string& texto,
    int valorDefault
)
{
    try
    {
        return std::stoi(texto);
    }
    catch (...)
    {
        return valorDefault;
    }
}


static float LeerFloat(
    const std::string& texto,
    float valorDefault
)
{
    try
    {
        return std::stof(texto);
    }
    catch (...)
    {
        return valorDefault;
    }
}


int LimitarZonaMuertaStick(
    int porcentaje
)
{
    if (porcentaje < ZONA_MUERTA_STICK_MINIMA)
    {
        porcentaje = ZONA_MUERTA_STICK_MINIMA;
    }

    if (porcentaje > ZONA_MUERTA_STICK_MAXIMA)
    {
        porcentaje = ZONA_MUERTA_STICK_MAXIMA;
    }

    int pasos =
        (porcentaje - ZONA_MUERTA_STICK_MINIMA + ZONA_MUERTA_STICK_PASO / 2) /
        ZONA_MUERTA_STICK_PASO;

    return ZONA_MUERTA_STICK_MINIMA + pasos * ZONA_MUERTA_STICK_PASO;
}


static float LimitarVolumen(
    float valor
)
{
    // La comparacion negada tambien atrapa NaN.
    if (!(valor >= 0.0f))
        return 0.0f;

    if (valor > 1.0f)
        return 1.0f;

    return valor;
}


static int LimitarNivel(
    int valor
)
{
    if (valor < (int)CALIDAD_BAJA)
        return (int)CALIDAD_BAJA;

    if (valor > (int)CALIDAD_ALTA)
        return (int)CALIDAD_ALTA;

    return valor;
}


OpcionesCalidadGrafica ObtenerOpcionesCalidadDesdeConfiguracion(
    const ConfiguracionJuego& config
)
{
    OpcionesCalidadGrafica opciones;

    opciones.particulas = (NivelCalidadGrafica)LimitarNivel(config.calidadParticulas);
    opciones.decoracion = (NivelCalidadGrafica)LimitarNivel(config.calidadDecoracion);
    opciones.efectos = (NivelCalidadGrafica)LimitarNivel(config.calidadEfectos);
    opciones.sombras = (NivelCalidadGrafica)LimitarNivel(config.calidadSombras);

    return opciones;
}


void ActualizarPresetGrafico(
    ConfiguracionJuego& config
)
{
    config.presetGrafico = PRESET_GRAFICO_PERSONALIZADO;

    for (int nivel = (int)CALIDAD_BAJA; nivel <= (int)CALIDAD_ALTA; nivel++)
    {
        if (
            config.calidadParticulas == nivel &&
            config.calidadDecoracion == nivel &&
            config.calidadEfectos == nivel &&
            config.calidadSombras == nivel
        )
        {
            config.presetGrafico = nivel;
        }
    }
}


void NormalizarConfiguracion(
    ConfiguracionJuego& config,
    int cantidadResoluciones,
    int cantidadOpcionesFPS,
    int indiceNativo
)
{
    if (
        config.indiceResolucion < 0 ||
        config.indiceResolucion >= cantidadResoluciones
    )
    {
        config.indiceResolucion = indiceNativo;
    }

    if (
        config.indiceFPS < 0 ||
        config.indiceFPS >= cantidadOpcionesFPS
    )
    {
        config.indiceFPS = cantidadOpcionesFPS > 1 ? 1 : 0;
    }

    if (
        config.modoVentana < MODO_VENTANA ||
        config.modoVentana > MODO_SIN_BORDES
    )
    {
        config.modoVentana = MODO_PANTALLA_COMPLETA;
    }

    if (
        config.modoTeclado < TECLADO_COMPLETO ||
        config.modoTeclado > TECLADO_DIVIDIDO
    )
    {
        config.modoTeclado = TECLADO_DIVIDIDO;
    }

    config.calidadParticulas = LimitarNivel(config.calidadParticulas);
    config.calidadDecoracion = LimitarNivel(config.calidadDecoracion);
    config.calidadEfectos = LimitarNivel(config.calidadEfectos);
    config.calidadSombras = LimitarNivel(config.calidadSombras);

    // El preset siempre refleja las cuatro calidades individuales.
    ActualizarPresetGrafico(config);

    config.volumenMusica = LimitarVolumen(config.volumenMusica);
    config.volumenSonidos = LimitarVolumen(config.volumenSonidos);

    config.zonaMuertaStick = LimitarZonaMuertaStick(config.zonaMuertaStick);
}


bool CargarConfiguracion(
    const char* ruta,
    ConfiguracionJuego& config
)
{
    std::ifstream archivo(ruta);

    if (!archivo.is_open())
    {
        return false;
    }

    std::string linea;

    while (std::getline(archivo, linea))
    {
        size_t igual = linea.find('=');

        if (igual == std::string::npos)
        {
            continue;
        }

        std::string clave = linea.substr(0, igual);
        std::string valor = linea.substr(igual + 1);

        // Tolera archivos con finales de linea de Windows.
        while (!valor.empty() && (valor.back() == '\r' || valor.back() == ' '))
            valor.pop_back();

        if (clave == "modoVentana")
        {
            int modo = LeerEntero(valor, (int)MODO_PANTALLA_COMPLETA);

            if (modo < (int)MODO_VENTANA)
                modo = (int)MODO_VENTANA;

            if (modo > (int)MODO_SIN_BORDES)
                modo = (int)MODO_SIN_BORDES;

            config.modoVentana = (ModoVentana)modo;
        }
        else if (clave == "indiceResolucion")
        {
            config.indiceResolucion = LeerEntero(valor, 0);
        }
        else if (clave == "indiceFPS")
        {
            config.indiceFPS = LeerEntero(valor, 1);
        }
        else if (clave == "mostrarFPS")
        {
            config.mostrarFPS = (LeerEntero(valor, 1) != 0);
        }
        else if (clave == "reducirMovimiento")
        {
            config.reducirMovimiento = (LeerEntero(valor, 0) != 0);
        }
        else if (clave == "vsync")
        {
            config.vsync = (LeerEntero(valor, 0) != 0);
        }
        else if (clave == "calidadParticulas")
        {
            config.calidadParticulas = LimitarNivel(LeerEntero(valor, CALIDAD_ALTA));
        }
        else if (clave == "calidadDecoracion")
        {
            config.calidadDecoracion = LimitarNivel(LeerEntero(valor, CALIDAD_ALTA));
        }
        else if (clave == "calidadEfectos")
        {
            config.calidadEfectos = LimitarNivel(LeerEntero(valor, CALIDAD_ALTA));
        }
        else if (clave == "calidadSombras")
        {
            config.calidadSombras = LimitarNivel(LeerEntero(valor, CALIDAD_ALTA));
        }
        else if (clave == "volumenMusica")
        {
            config.volumenMusica = LimitarVolumen(LeerFloat(valor, 0.35f));
        }
        else if (clave == "volumenSonidos")
        {
            config.volumenSonidos = LimitarVolumen(LeerFloat(valor, 0.60f));
        }
        else if (clave == "modoTeclado")
        {
            int modo = LeerEntero(valor, (int)TECLADO_DIVIDIDO);

            if (modo < (int)TECLADO_COMPLETO)
                modo = (int)TECLADO_COMPLETO;

            if (modo > (int)TECLADO_DIVIDIDO)
                modo = (int)TECLADO_DIVIDIDO;

            config.modoTeclado = (ModoTeclado)modo;
        }
        else if (clave == "zonaMuertaStick")
        {
            config.zonaMuertaStick = LimitarZonaMuertaStick(
                LeerEntero(valor, ZONA_MUERTA_STICK_DEFECTO)
            );
        }
    }

    archivo.close();

    ActualizarPresetGrafico(config);

    return true;
}


bool GuardarConfiguracion(
    const char* ruta,
    const ConfiguracionJuego& config
)
{
    std::ofstream archivo(ruta);

    if (!archivo.is_open())
    {
        return false;
    }

    archivo << "modoVentana=" << (int)config.modoVentana << "\n";
    archivo << "indiceResolucion=" << config.indiceResolucion << "\n";
    archivo << "indiceFPS=" << config.indiceFPS << "\n";
    archivo << "mostrarFPS=" << (config.mostrarFPS ? 1 : 0) << "\n";
    archivo << "vsync=" << (config.vsync ? 1 : 0) << "\n";
    archivo << "reducirMovimiento=" << (config.reducirMovimiento ? 1 : 0) << "\n";
    archivo << "presetGrafico=" << config.presetGrafico << "\n";
    archivo << "calidadParticulas=" << config.calidadParticulas << "\n";
    archivo << "calidadDecoracion=" << config.calidadDecoracion << "\n";
    archivo << "calidadEfectos=" << config.calidadEfectos << "\n";
    archivo << "calidadSombras=" << config.calidadSombras << "\n";
    archivo << "volumenMusica=" << config.volumenMusica << "\n";
    archivo << "volumenSonidos=" << config.volumenSonidos << "\n";
    archivo << "modoTeclado=" << (int)config.modoTeclado << "\n";
    archivo << "zonaMuertaStick=" << config.zonaMuertaStick << "\n";

    archivo.close();

    return true;
}
