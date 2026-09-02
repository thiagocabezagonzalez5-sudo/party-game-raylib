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
        else if (clave == "volumenMusica")
        {
            config.volumenMusica = LeerFloat(valor, 0.35f);
        }
        else if (clave == "volumenSonidos")
        {
            config.volumenSonidos = LeerFloat(valor, 0.60f);
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
    }

    archivo.close();

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
    archivo << "volumenMusica=" << config.volumenMusica << "\n";
    archivo << "volumenSonidos=" << config.volumenSonidos << "\n";
    archivo << "modoTeclado=" << (int)config.modoTeclado << "\n";

    archivo.close();

    return true;
}
