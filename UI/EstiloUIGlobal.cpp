#include "UI/EstiloUIGlobal.h"


//==================================================
// INSTANCIA COMPARTIDA
//==================================================

static EstiloUI estiloGlobal;


EstiloUI& ObtenerEstiloUI()
{
    // Las texturas necesitan el contexto OpenGL de la ventana.
    if (!estiloGlobal.cargado && IsWindowReady())
    {
        CargarEstiloUI(estiloGlobal);
    }

    return estiloGlobal;
}


void DescargarEstiloUIGlobal()
{
    DescargarEstiloUI(estiloGlobal);
}
