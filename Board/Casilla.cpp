#include "Board/Casilla.h"


//==================================================
// COLOR
//==================================================

Color ObtenerColorCasilla(
    TipoCasilla tipo
)
{
    switch (tipo)
    {
        case CASILLA_POSITIVA:
            return Color{
                70,
                185,
                105,
                255
            };

        case CASILLA_NEGATIVA:
            return Color{
                220,
                75,
                75,
                255
            };

        case CASILLA_ESPECIAL:
            return Color{
                245,
                175,
                45,
                255
            };

        case CASILLA_NEUTRA:
            break;
    }

    return Color{
        80,
        145,
        220,
        255
    };
}


//==================================================
// NOMBRE
//==================================================

const char* ObtenerNombreTipoCasilla(
    TipoCasilla tipo
)
{
    switch (tipo)
    {
        case CASILLA_POSITIVA:
            return "POSITIVA";

        case CASILLA_NEGATIVA:
            return "NEGATIVA";

        case CASILLA_ESPECIAL:
            return "ESPECIAL";

        case CASILLA_NEUTRA:
            break;
    }

    return "NEUTRA";
}
