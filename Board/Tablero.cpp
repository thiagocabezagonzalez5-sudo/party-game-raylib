#include "Board/Tablero.h"


//==================================================
// AGREGAR CASILLA
//==================================================

int Tablero::AgregarCasilla(
    Vector3 posicion,
    TipoCasilla tipo
)
{
    if (
        cantidadCasillas < 0 ||
        cantidadCasillas >= MAX_CASILLAS_TABLERO
    )
    {
        return -1;
    }

    int indice =
        cantidadCasillas;

    Casilla& casilla =
        casillas[indice];

    casilla =
        Casilla{};

    casilla.indice =
        indice;

    casilla.posicion =
        posicion;

    casilla.tipo =
        tipo;

    cantidadCasillas++;

    return indice;
}


//==================================================
// CONECTAR CASILLAS
//==================================================

bool Tablero::ConectarCasillas(
    int origen,
    int destino
)
{
    if (
        origen < 0 ||
        origen >= cantidadCasillas ||
        destino < 0 ||
        destino >= cantidadCasillas
    )
    {
        return false;
    }

    Casilla& casillaOrigen =
        casillas[origen];

    if (
        casillaOrigen.cantidadConexiones >=
        MAX_CONEXIONES_CASILLA
    )
    {
        return false;
    }

    for (
        int i = 0;
        i < casillaOrigen.cantidadConexiones;
        i++
    )
    {
        if (
            casillaOrigen.conexiones[i].destino ==
            destino
        )
        {
            return false;
        }
    }

    casillaOrigen
        .conexiones[
            casillaOrigen.cantidadConexiones
        ]
        .destino =
        destino;

    casillaOrigen.cantidadConexiones++;

    return true;
}


//==================================================
// INICIALIZAR PROTOTIPO
//==================================================

void Tablero::InicializarPrototipo()
{
    cantidadCasillas =
        0;

    recorridoValido =
        false;

    for (
        int i = 0;
        i < MAX_CASILLAS_TABLERO;
        i++
    )
    {
        casillas[i] =
            Casilla{};
    }

    // Circuito exterior.
    AgregarCasilla({ -8.0f, 0.25f,  5.0f }, CASILLA_ESPECIAL); // 0
    AgregarCasilla({ -5.0f, 0.25f,  5.0f }, CASILLA_NEUTRA);   // 1
    AgregarCasilla({ -2.0f, 0.25f,  5.0f }, CASILLA_POSITIVA); // 2
    AgregarCasilla({  1.0f, 0.25f,  5.0f }, CASILLA_NEUTRA);   // 3
    AgregarCasilla({  4.0f, 0.25f,  5.0f }, CASILLA_NEGATIVA); // 4
    AgregarCasilla({  7.0f, 0.25f,  5.0f }, CASILLA_POSITIVA); // 5
    AgregarCasilla({  8.0f, 0.25f,  2.0f }, CASILLA_NEUTRA);   // 6
    AgregarCasilla({  8.0f, 0.25f, -1.0f }, CASILLA_NEGATIVA); // 7
    AgregarCasilla({  8.0f, 0.25f, -4.0f }, CASILLA_POSITIVA); // 8
    AgregarCasilla({  5.0f, 0.25f, -5.0f }, CASILLA_NEUTRA);   // 9
    AgregarCasilla({  2.0f, 0.25f, -5.0f }, CASILLA_NEGATIVA); // 10
    AgregarCasilla({ -1.0f, 0.25f, -5.0f }, CASILLA_POSITIVA); // 11
    AgregarCasilla({ -4.0f, 0.25f, -5.0f }, CASILLA_NEUTRA);   // 12
    AgregarCasilla({ -7.0f, 0.25f, -5.0f }, CASILLA_NEGATIVA); // 13
    AgregarCasilla({ -8.0f, 0.25f, -2.0f }, CASILLA_POSITIVA); // 14
    AgregarCasilla({ -8.0f, 0.25f,  2.0f }, CASILLA_NEUTRA);   // 15

    // Ruta interior de la bifurcacion.
    AgregarCasilla({  3.0f, 0.25f,  2.5f }, CASILLA_NEGATIVA); // 16
    AgregarCasilla({  4.5f, 0.25f,  0.0f }, CASILLA_POSITIVA); // 17
    AgregarCasilla({  6.0f, 0.25f, -2.5f }, CASILLA_NEUTRA);   // 18

    for (
        int i = 0;
        i < 15;
        i++
    )
    {
        ConectarCasillas(
            i,
            i + 1
        );
    }

    ConectarCasillas(
        15,
        0
    );

    // La casilla 3 permite elegir entre el circuito
    // exterior y el atajo interior.
    ConectarCasillas(
        3,
        16
    );

    ConectarCasillas(
        16,
        17
    );

    ConectarCasillas(
        17,
        18
    );

    ConectarCasillas(
        18,
        8
    );

    recorridoValido =
        ValidarRecorrido();
}


//==================================================
// OBTENER CASILLA
//==================================================

const Casilla* Tablero::ObtenerCasilla(
    int indice
) const
{
    if (
        indice < 0 ||
        indice >= cantidadCasillas
    )
    {
        return nullptr;
    }

    return &casillas[indice];
}


//==================================================
// VALIDAR RECORRIDO
//==================================================

bool Tablero::ValidarRecorrido()
{
    if (
        cantidadCasillas <= 0 ||
        cantidadCasillas > MAX_CASILLAS_TABLERO
    )
    {
        recorridoValido =
            false;

        return false;
    }

    bool visitadas[
        MAX_CASILLAS_TABLERO
    ] = {};

    int pendientes[
        MAX_CASILLAS_TABLERO
    ] = {};

    int inicioCola =
        0;

    int finCola =
        0;

    pendientes[finCola] =
        0;

    finCola++;

    visitadas[0] =
        true;

    while (inicioCola < finCola)
    {
        int indiceActual =
            pendientes[inicioCola];

        inicioCola++;

        const Casilla& casilla =
            casillas[indiceActual];

        if (
            casilla.cantidadConexiones <= 0 ||
            casilla.cantidadConexiones > MAX_CONEXIONES_CASILLA
        )
        {
            recorridoValido =
                false;

            return false;
        }

        for (
            int i = 0;
            i < casilla.cantidadConexiones;
            i++
        )
        {
            int destino =
                casilla.conexiones[i].destino;

            if (
                destino < 0 ||
                destino >= cantidadCasillas
            )
            {
                recorridoValido =
                    false;

                return false;
            }

            if (!visitadas[destino])
            {
                visitadas[destino] =
                    true;

                pendientes[finCola] =
                    destino;

                finCola++;
            }
        }
    }

    for (
        int i = 0;
        i < cantidadCasillas;
        i++
    )
    {
        if (!visitadas[i])
        {
            recorridoValido =
                false;

            return false;
        }
    }

    recorridoValido =
        true;

    return true;
}


//==================================================
// DIBUJAR
//==================================================

void Tablero::Dibujar() const
{
    DrawPlane(
        Vector3{
            0.0f,
            -0.05f,
            0.0f
        },
        Vector2{
            22.0f,
            16.0f
        },
        Color{
            48,
            72,
            78,
            255
        }
    );

    for (
        int i = 0;
        i < cantidadCasillas;
        i++
    )
    {
        const Casilla& casilla =
            casillas[i];

        for (
            int conexion = 0;
            conexion < casilla.cantidadConexiones;
            conexion++
        )
        {
            const Casilla* destino =
                ObtenerCasilla(
                    casilla.conexiones[conexion].destino
                );

            if (destino == nullptr)
            {
                continue;
            }

            Vector3 origenLinea =
                casilla.posicion;

            Vector3 destinoLinea =
                destino->posicion;

            origenLinea.y =
                0.12f;

            destinoLinea.y =
                0.12f;

            DrawCylinderEx(
                origenLinea,
                destinoLinea,
                0.10f,
                0.10f,
                8,
                Color{
                    45,
                    45,
                    55,
                    255
                }
            );
        }
    }

    for (
        int i = 0;
        i < cantidadCasillas;
        i++
    )
    {
        const Casilla& casilla =
            casillas[i];

        Color color =
            ObtenerColorCasilla(
                casilla.tipo
            );

        DrawCube(
            casilla.posicion,
            1.55f,
            0.45f,
            1.55f,
            color
        );

        DrawCubeWires(
            casilla.posicion,
            1.55f,
            0.45f,
            1.55f,
            Color{
                25,
                30,
                38,
                255
            }
        );
    }
}
