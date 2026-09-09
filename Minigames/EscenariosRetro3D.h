#pragma once

#include "Minigames/EfectosVisualesMinijuegos.h"

#include <cmath>


//==================================================
// ESCENARIOS RETRO 3D EXTENDIDOS
//==================================================
//
// Los fondos importantes viven ahora en el mismo espacio 3D que el
// minijuego. Se extienden mucho mas alla del encuadre para que no aparezcan
// cortes laterales al cambiar resolucion o temblar la camara.
//
// En las montanas nevadas, la nieve NO es otro objeto colocado encima:
// son caras superiores de la misma montana pintadas de blanco.
//==================================================

#ifdef BeginMode3D
#undef BeginMode3D
#endif


inline float RepetirEscenario3D(
    float valor,
    float longitud
)
{
    float resultado = std::fmod(valor, longitud);

    if (resultado < 0.0f)
    {
        resultado += longitud;
    }

    return resultado;
}


inline void DibujarCaraDobleEscenario3D(
    Vector3 a,
    Vector3 b,
    Vector3 c,
    Color color
)
{
    DrawTriangle3D(a, b, c, color);
    DrawTriangle3D(c, b, a, color);
}


inline void DibujarCuadrilateroEscenario3D(
    Vector3 a,
    Vector3 b,
    Vector3 c,
    Vector3 d,
    Color color
)
{
    DibujarCaraDobleEscenario3D(a, b, c, color);
    DibujarCaraDobleEscenario3D(a, c, d, color);
}


// Montana tipo prisma triangular. Esto da volumen real sin convertir el
// fondo en conos gigantes. La franja blanca usa las MISMAS caras del prisma.
inline void DibujarMontanaPrismaEscenario3D(
    Vector3 centroBase,
    float ancho,
    float altura,
    float profundidad,
    Color rocaFrontal,
    Color rocaLateral,
    Color colorCima,
    float proporcionCima
)
{
    if (proporcionCima < 0.0f)
    {
        proporcionCima = 0.0f;
    }

    if (proporcionCima > 0.55f)
    {
        proporcionCima = 0.55f;
    }

    const float mitadAncho = ancho * 0.5f;
    const float mitadProfundidad = profundidad * 0.5f;

    const float yBase = centroBase.y;
    const float yPico = yBase + altura;

    const float zFrente = centroBase.z + mitadProfundidad;
    const float zAtras = centroBase.z - mitadProfundidad;

    Vector3 frenteIzq =
    {
        centroBase.x - mitadAncho,
        yBase,
        zFrente
    };

    Vector3 frenteDer =
    {
        centroBase.x + mitadAncho,
        yBase,
        zFrente
    };

    Vector3 frentePico =
    {
        centroBase.x,
        yPico,
        zFrente
    };

    Vector3 atrasIzq =
    {
        centroBase.x - mitadAncho,
        yBase,
        zAtras
    };

    Vector3 atrasDer =
    {
        centroBase.x + mitadAncho,
        yBase,
        zAtras
    };

    Vector3 atrasPico =
    {
        centroBase.x,
        yPico,
        zAtras
    };

    if (proporcionCima > 0.001f)
    {
        const float yCorte =
            yPico - altura * proporcionCima;

        const float mitadCorte =
            mitadAncho * proporcionCima;

        Vector3 frenteCorteIzq =
        {
            centroBase.x - mitadCorte,
            yCorte,
            zFrente
        };

        Vector3 frenteCorteDer =
        {
            centroBase.x + mitadCorte,
            yCorte,
            zFrente
        };

        Vector3 atrasCorteIzq =
        {
            centroBase.x - mitadCorte,
            yCorte,
            zAtras
        };

        Vector3 atrasCorteDer =
        {
            centroBase.x + mitadCorte,
            yCorte,
            zAtras
        };

        DibujarCuadrilateroEscenario3D(
            frenteIzq,
            frenteDer,
            frenteCorteDer,
            frenteCorteIzq,
            rocaFrontal
        );

        DibujarCuadrilateroEscenario3D(
            atrasDer,
            atrasIzq,
            atrasCorteIzq,
            atrasCorteDer,
            rocaFrontal
        );

        DibujarCaraDobleEscenario3D(
            frenteCorteIzq,
            frenteCorteDer,
            frentePico,
            colorCima
        );

        DibujarCaraDobleEscenario3D(
            atrasCorteDer,
            atrasCorteIzq,
            atrasPico,
            colorCima
        );

        DibujarCuadrilateroEscenario3D(
            frenteCorteIzq,
            frentePico,
            atrasPico,
            atrasCorteIzq,
            colorCima
        );

        DibujarCuadrilateroEscenario3D(
            frentePico,
            frenteCorteDer,
            atrasCorteDer,
            atrasPico,
            colorCima
        );

        DibujarCuadrilateroEscenario3D(
            frenteIzq,
            frenteCorteIzq,
            atrasCorteIzq,
            atrasIzq,
            rocaLateral
        );

        DibujarCuadrilateroEscenario3D(
            frenteCorteDer,
            frenteDer,
            atrasDer,
            atrasCorteDer,
            rocaLateral
        );
    }
    else
    {
        DibujarCaraDobleEscenario3D(
            frenteIzq,
            frenteDer,
            frentePico,
            rocaFrontal
        );

        DibujarCaraDobleEscenario3D(
            atrasDer,
            atrasIzq,
            atrasPico,
            rocaFrontal
        );

        DibujarCuadrilateroEscenario3D(
            frenteIzq,
            frentePico,
            atrasPico,
            atrasIzq,
            rocaLateral
        );

        DibujarCuadrilateroEscenario3D(
            frentePico,
            frenteDer,
            atrasDer,
            atrasPico,
            rocaLateral
        );
    }

    DibujarCuadrilateroEscenario3D(
        frenteIzq,
        atrasIzq,
        atrasDer,
        frenteDer,
        rocaLateral
    );
}


inline void DibujarCieloPlanoEscenario3D(
    Color color,
    float z,
    float ancho,
    float alto
)
{
    DrawCube(
        { 0.0f, alto * 0.32f, z },
        ancho,
        alto,
        1.2f,
        color
    );
}


inline void DibujarEstrellasLavaEscenario3D()
{
    EstadoEfectosVisualesMinijuegos& estado =
        ObtenerEstadoEfectosVisualesMinijuegos();

    const float z = -67.8f;

    for (int i = 0; i < 84; i++)
    {
        float x =
            -78.0f +
            RepetirEscenario3D(
                (float)(i * 29),
                156.0f
            );

        float y =
            -1.5f +
            RepetirEscenario3D(
                (float)(i * 17),
                25.0f
            );

        float pulso =
            0.72f +
            0.28f * std::sin(
                estado.tiempoGlobal * 1.8f +
                (float)i * 0.71f
            );

        float radio =
            0.055f + 0.035f * (float)(i % 3);

        DrawSphere(
            { x, y, z },
            radio,
            Fade(RAYWHITE, 0.52f + pulso * 0.36f)
        );
    }
}


inline void DibujarVolcanesLavaEscenario3D()
{
    // Fila lejana.
    for (int i = 0; i < 13; i++)
    {
        float x = -72.0f + (float)i * 12.0f;
        float altura = 8.2f + (float)(i % 4) * 1.35f;
        float ancho = 13.5f + (float)(i % 3) * 1.6f;

        DibujarMontanaPrismaEscenario3D(
            { x, -2.55f, -49.0f - (float)(i % 2) * 1.8f },
            ancho,
            altura,
            7.5f,
            Color{ 63, 54, 56, 255 },
            Color{ 45, 39, 42, 255 },
            Color{ 63, 54, 56, 255 },
            0.0f
        );
    }

    // Fila cercana, menos alta para que nunca tape el cielo completo.
    for (int i = 0; i < 11; i++)
    {
        float x = -60.0f + (float)i * 12.0f;
        float altura = 6.6f + (float)(i % 3) * 1.15f;
        float ancho = 12.0f + (float)(i % 2) * 1.8f;

        DibujarMontanaPrismaEscenario3D(
            { x, -2.55f, -34.0f - (float)(i % 2) * 1.6f },
            ancho,
            altura,
            6.0f,
            Color{ 82, 67, 65, 255 },
            Color{ 57, 48, 49, 255 },
            Color{ 82, 67, 65, 255 },
            0.0f
        );

        if (i % 3 == 1)
        {
            DrawSphere(
                { x, -2.55f + altura * 0.92f, -30.9f - (float)(i % 2) * 1.6f },
                0.16f,
                Color{ 255, 126, 28, 255 }
            );
        }
    }
}


inline void DibujarLavaEscenario3D()
{
    DrawPlane(
        { 0.0f, -2.62f, -4.0f },
        { 180.0f, 170.0f },
        Color{ 225, 61, 17, 255 }
    );

    DrawPlane(
        { 0.0f, -2.60f, -4.0f },
        { 180.0f, 170.0f },
        Fade(Color{ 255, 116, 21, 255 }, 0.55f)
    );

    EstadoEfectosVisualesMinijuegos& estado =
        ObtenerEstadoEfectosVisualesMinijuegos();

    for (int i = 0; i < 26; i++)
    {
        float x =
            -55.0f +
            RepetirEscenario3D(
                (float)(i * 7) + estado.tiempoGlobal * 2.2f,
                110.0f
            );

        float z =
            -22.0f +
            RepetirEscenario3D(
                (float)(i * 13),
                58.0f
            );

        DrawCube(
            { x, -2.54f, z },
            0.55f + (float)(i % 3) * 0.24f,
            0.025f,
            0.055f,
            Fade(Color{ 255, 220, 75, 255 }, 0.70f)
        );
    }
}


inline void DibujarCenizaLavaEscenario3D()
{
    EstadoEfectosVisualesMinijuegos& estado =
        ObtenerEstadoEfectosVisualesMinijuegos();

    for (int i = 0; i < 38; i++)
    {
        float x =
            -32.0f +
            RepetirEscenario3D(
                (float)(i * 11) + estado.tiempoGlobal * 1.8f,
                64.0f
            );

        float y =
            0.2f +
            RepetirEscenario3D(
                (float)(i * 7) + estado.tiempoGlobal * 0.45f,
                13.0f
            );

        float z =
            -6.0f -
            RepetirEscenario3D(
                (float)(i * 5),
                34.0f
            );

        DrawSphere(
            { x, y, z },
            0.035f + 0.025f * (float)(i % 3),
            Fade(Color{ 90, 84, 82, 255 }, 0.62f)
        );
    }
}


inline void DibujarTemaLavaRetro3D()
{
    DibujarCieloPlanoEscenario3D(
        Color{ 5, 6, 10, 255 },
        -69.0f,
        180.0f,
        58.0f
    );

    DibujarEstrellasLavaEscenario3D();
    DibujarVolcanesLavaEscenario3D();
    DibujarLavaEscenario3D();
    DibujarCenizaLavaEscenario3D();
}


inline void DibujarCordilleraNieveEscenario3D()
{
    // Segunda linea, muy lejana. Se extiende mucho fuera del encuadre.
    for (int i = 0; i < 15; i++)
    {
        float x = -84.0f + (float)i * 12.0f;
        float altura = 9.0f + (float)(i % 5) * 1.15f;
        float ancho = 14.0f + (float)(i % 3) * 1.4f;

        DibujarMontanaPrismaEscenario3D(
            { x, -3.4f, -51.0f - (float)(i % 2) * 1.7f },
            ancho,
            altura,
            7.0f,
            Color{ 129, 146, 160, 255 },
            Color{ 102, 120, 137, 255 },
            Color{ 239, 246, 250, 255 },
            0.24f
        );
    }

    // Primera linea, a distancia segura de la camara.
    for (int i = 0; i < 13; i++)
    {
        float x = -72.0f + (float)i * 12.0f;
        float altura = 7.6f + (float)(i % 4) * 1.0f;
        float ancho = 12.5f + (float)(i % 3) * 1.2f;

        DibujarMontanaPrismaEscenario3D(
            { x, -3.3f, -35.0f - (float)(i % 2) * 1.5f },
            ancho,
            altura,
            6.0f,
            Color{ 155, 171, 184, 255 },
            Color{ 121, 140, 156, 255 },
            RAYWHITE,
            0.27f
        );
    }
}


inline void DibujarCieloNieveEscenario3D()
{
    DibujarCieloPlanoEscenario3D(
        Color{ 54, 169, 226, 255 },
        -70.0f,
        190.0f,
        62.0f
    );
}


inline void DibujarCoposNieveEscenario3D()
{
    EstadoEfectosVisualesMinijuegos& estado =
        ObtenerEstadoEfectosVisualesMinijuegos();

    for (int i = 0; i < 72; i++)
    {
        float x =
            -40.0f +
            RepetirEscenario3D(
                (float)(i * 13),
                80.0f
            );

        float y =
            -1.0f +
            RepetirEscenario3D(
                (float)(i * 19) +
                estado.tiempoGlobal * (0.8f + (float)(i % 4) * 0.16f),
                18.0f
            );

        float z =
            -6.0f -
            RepetirEscenario3D(
                (float)(i * 7),
                48.0f
            );

        float deriva =
            std::sin(
                estado.tiempoGlobal * 0.9f +
                (float)i * 0.7f
            ) * 0.25f;

        DrawSphere(
            { x + deriva, y, z },
            0.045f + 0.025f * (float)(i % 3),
            Fade(RAYWHITE, 0.86f)
        );
    }
}


inline void DibujarCumbreAltaEscenario3D()
{
    // Cuerpo de la montana del minijuego. Muy profundo para que no se vea
    // el final cuando un jugador cae.
    DrawCylinderEx(
        { 0.0f, -34.0f, 0.0f },
        { 0.0f, -1.85f, 0.0f },
        14.5f,
        6.45f,
        24,
        Color{ 97, 119, 139, 255 }
    );

    DrawCylinderEx(
        { 0.0f, -7.0f, 0.0f },
        { 0.0f, -1.70f, 0.0f },
        8.2f,
        6.35f,
        24,
        Color{ 191, 213, 227, 255 }
    );
}


inline void DibujarTemaNieveRetro3D()
{
    DibujarCieloNieveEscenario3D();
    DibujarCordilleraNieveEscenario3D();
    DibujarCoposNieveEscenario3D();
    DibujarCumbreAltaEscenario3D();
}


inline void DibujarDecoracionEscenarioRetro3D()
{
    switch (
        ObtenerEstadoEfectosVisualesMinijuegos().tema
    )
    {
        case TEMA_VISUAL_LAVA:
            DibujarTemaLavaRetro3D();
            break;

        case TEMA_VISUAL_NIEVE:
            DibujarTemaNieveRetro3D();
            break;

        case TEMA_VISUAL_CUEVA:
            DibujarTemaCueva();
            break;

        case TEMA_VISUAL_MAGNETICO:
            DibujarTemaMagnetico();
            break;

        case TEMA_VISUAL_NINGUNO:
            break;
    }
}


inline void BeginMode3DConEscenarioRetro3D(
    Camera3D camara
)
{
    Camera3D camaraFinal =
        AplicarTemblorGeneralACamara(camara);

    BeginMode3D(camaraFinal);
    DibujarDecoracionEscenarioRetro3D();
}


#define BeginMode3D(...) BeginMode3DConEscenarioRetro3D(__VA_ARGS__)
