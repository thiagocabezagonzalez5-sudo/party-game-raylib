#pragma once

#include "Minigames/EfectosVisualesMinijuegos.h"

#include <cmath>


//==================================================
// ESCENARIOS RETRO 3D EXTENDIDOS
//==================================================
// Los fondos viven en 3D y se extienden mucho mas alla del encuadre.
// Las montanas son pocas pero grandes para formar una silueta continua.
// En nieve, la punta blanca pertenece a la misma geometria de la montana.
//==================================================

#ifdef BeginMode3D
#undef BeginMode3D
#endif


inline float RepetirEscenario3D(float valor, float longitud)
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


// Prisma triangular con volumen real. Si proporcionCima > 0, la parte
// superior de las MISMAS caras se pinta con colorCima; no agrega otro objeto.
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
    if (proporcionCima < 0.0f) proporcionCima = 0.0f;
    if (proporcionCima > 0.55f) proporcionCima = 0.55f;

    const float mitadAncho = ancho * 0.5f;
    const float mitadProfundidad = profundidad * 0.5f;
    const float yBase = centroBase.y;
    const float yPico = yBase + altura;
    const float zFrente = centroBase.z + mitadProfundidad;
    const float zAtras = centroBase.z - mitadProfundidad;

    Vector3 frenteIzq = { centroBase.x - mitadAncho, yBase, zFrente };
    Vector3 frenteDer = { centroBase.x + mitadAncho, yBase, zFrente };
    Vector3 frentePico = { centroBase.x, yPico, zFrente };

    Vector3 atrasIzq = { centroBase.x - mitadAncho, yBase, zAtras };
    Vector3 atrasDer = { centroBase.x + mitadAncho, yBase, zAtras };
    Vector3 atrasPico = { centroBase.x, yPico, zAtras };

    if (proporcionCima > 0.001f)
    {
        const float yCorte = yPico - altura * proporcionCima;
        const float mitadCorte = mitadAncho * proporcionCima;

        Vector3 frenteCorteIzq =
            { centroBase.x - mitadCorte, yCorte, zFrente };
        Vector3 frenteCorteDer =
            { centroBase.x + mitadCorte, yCorte, zFrente };
        Vector3 atrasCorteIzq =
            { centroBase.x - mitadCorte, yCorte, zAtras };
        Vector3 atrasCorteDer =
            { centroBase.x + mitadCorte, yCorte, zAtras };

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
        { 0.0f, alto * 0.30f, z },
        ancho,
        alto,
        1.5f,
        color
    );
}


//==================================================
// COLOR SEGURO - LAVA / VOLCANES
//==================================================

inline void DibujarEstrellasLavaEscenario3D()
{
    EstadoEfectosVisualesMinijuegos& estado =
        ObtenerEstadoEfectosVisualesMinijuegos();

    const float z = -73.6f;

    for (int i = 0; i < 96; i++)
    {
        float x =
            -96.0f + RepetirEscenario3D((float)(i * 31), 192.0f);

        float y =
            -2.0f + RepetirEscenario3D((float)(i * 19), 34.0f);

        float pulso =
            0.72f +
            0.28f * std::sin(
                estado.tiempoGlobal * 1.8f + (float)i * 0.71f
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
    // Pocos volcanes, pero enormes. Los extremos estan muy fuera de camara
    // y se superponen, por lo que la cadena no deja huecos laterales.
    const float posiciones[7] =
    {
        -76.0f,
        -51.0f,
        -26.0f,
        0.0f,
        27.0f,
        53.0f,
        78.0f
    };

    for (int i = 0; i < 7; i++)
    {
        float altura =
            15.5f + (float)(i % 3) * 2.1f;

        float ancho =
            31.0f + (float)(i % 2) * 4.5f;

        DibujarMontanaPrismaEscenario3D(
            {
                posiciones[i],
                -2.60f,
                -43.0f - (float)(i % 2) * 4.0f
            },
            ancho,
            altura,
            12.0f,
            i % 2 == 0
                ? Color{ 68, 56, 58, 255 }
                : Color{ 78, 62, 62, 255 },
            Color{ 45, 39, 42, 255 },
            Color{ 68, 56, 58, 255 },
            0.0f
        );

        if (i == 1 || i == 3 || i == 5)
        {
            DrawSphere(
                {
                    posiciones[i],
                    -2.60f + altura * 0.94f,
                    -36.8f - (float)(i % 2) * 4.0f
                },
                0.17f,
                Color{ 255, 126, 28, 255 }
            );
        }
    }
}


inline void DibujarLavaEscenario3D()
{
    DrawPlane(
        { 0.0f, -2.62f, -4.0f },
        { 220.0f, 190.0f },
        Color{ 225, 61, 17, 255 }
    );

    DrawPlane(
        { 0.0f, -2.60f, -4.0f },
        { 220.0f, 190.0f },
        Fade(Color{ 255, 116, 21, 255 }, 0.55f)
    );

    EstadoEfectosVisualesMinijuegos& estado =
        ObtenerEstadoEfectosVisualesMinijuegos();

    for (int i = 0; i < 26; i++)
    {
        float x =
            -60.0f +
            RepetirEscenario3D(
                (float)(i * 7) + estado.tiempoGlobal * 2.2f,
                120.0f
            );

        float z =
            -28.0f + RepetirEscenario3D((float)(i * 13), 68.0f);

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
            -36.0f +
            RepetirEscenario3D(
                (float)(i * 11) + estado.tiempoGlobal * 1.8f,
                72.0f
            );

        float y =
            0.2f +
            RepetirEscenario3D(
                (float)(i * 7) + estado.tiempoGlobal * 0.45f,
                15.0f
            );

        float z =
            -6.0f - RepetirEscenario3D((float)(i * 5), 42.0f);

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
        -75.0f,
        230.0f,
        76.0f
    );

    DibujarEstrellasLavaEscenario3D();
    DibujarVolcanesLavaEscenario3D();
    DibujarLavaEscenario3D();
    DibujarCenizaLavaEscenario3D();
}


//==================================================
// PELOTAS - CORDILLERA / NIEBLA
//==================================================

inline void DibujarCordilleraNieveEscenario3D()
{
    // Una sola cadena principal: menos montanas y mucho mas grandes.
    // Los extremos quedan deliberadamente fuera del campo de vision.
    const float posiciones[7] =
    {
        -82.0f,
        -55.0f,
        -28.0f,
        0.0f,
        29.0f,
        57.0f,
        84.0f
    };

    for (int i = 0; i < 7; i++)
    {
        float altura =
            16.0f + (float)(i % 3) * 2.6f;

        float ancho =
            33.0f + (float)(i % 2) * 4.0f;

        DibujarMontanaPrismaEscenario3D(
            {
                posiciones[i],
                -3.45f,
                -43.0f - (float)(i % 2) * 3.0f
            },
            ancho,
            altura,
            12.0f,
            i % 2 == 0
                ? Color{ 148, 165, 179, 255 }
                : Color{ 136, 154, 169, 255 },
            Color{ 108, 127, 145, 255 },
            RAYWHITE,
            0.24f
        );
    }
}


inline void DibujarCieloNieveEscenario3D()
{
    DibujarCieloPlanoEscenario3D(
        Color{ 54, 169, 226, 255 },
        -76.0f,
        240.0f,
        80.0f
    );
}


inline void DibujarCoposNieveEscenario3D()
{
    EstadoEfectosVisualesMinijuegos& estado =
        ObtenerEstadoEfectosVisualesMinijuegos();

    for (int i = 0; i < 72; i++)
    {
        float x =
            -48.0f + RepetirEscenario3D((float)(i * 13), 96.0f);

        float y =
            -1.0f +
            RepetirEscenario3D(
                (float)(i * 19) +
                estado.tiempoGlobal *
                (0.8f + (float)(i % 4) * 0.16f),
                20.0f
            );

        float z =
            -5.0f - RepetirEscenario3D((float)(i * 7), 58.0f);

        float deriva =
            std::sin(
                estado.tiempoGlobal * 0.9f + (float)i * 0.7f
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
    // Solo se modela lo que queda por encima de la niebla. Debajo de la
    // capa opaca no hace falta desarrollar detalle visual.
    DrawCylinderEx(
        { 0.0f, -8.5f, 0.0f },
        { 0.0f, -1.85f, 0.0f },
        9.5f,
        6.45f,
        24,
        Color{ 105, 126, 145, 255 }
    );

    DrawCylinderEx(
        { 0.0f, -5.8f, 0.0f },
        { 0.0f, -1.70f, 0.0f },
        7.8f,
        6.35f,
        24,
        Color{ 196, 217, 230, 255 }
    );
}


inline void DibujarNieblaCaidaPelotasEscenario3D()
{
    // La primera capa es mas transparente para que no parezca una pared.
    // Las siguientes se vuelven practicamente opacas: cuando el jugador
    // cae por debajo de la cumbre desaparece gradualmente dentro de la niebla.
    DrawPlane(
        { 0.0f, -3.15f, 1.0f },
        { 115.0f, 115.0f },
        Fade(Color{ 225, 239, 247, 255 }, 0.42f)
    );

    DrawPlane(
        { 0.0f, -3.75f, 1.0f },
        { 120.0f, 120.0f },
        Fade(Color{ 218, 235, 245, 255 }, 0.78f)
    );

    DrawPlane(
        { 0.0f, -4.45f, 1.0f },
        { 125.0f, 125.0f },
        Color{ 211, 231, 242, 250 }
    );

    // Volumen inferior muy grande: garantiza que nunca se vea el final de
    // la montana ni objetos cayendo por debajo de la zona que nos interesa.
    DrawCube(
        { 0.0f, -16.0f, 0.0f },
        130.0f,
        22.0f,
        130.0f,
        Color{ 211, 231, 242, 255 }
    );
}


inline void DibujarTemaNieveRetro3D()
{
    DibujarCieloNieveEscenario3D();
    DibujarCordilleraNieveEscenario3D();
    DibujarCoposNieveEscenario3D();
    DibujarCumbreAltaEscenario3D();
    DibujarNieblaCaidaPelotasEscenario3D();
}


//==================================================
// ENTRADA COMUN A LOS TEMAS
//==================================================

inline void DibujarDecoracionEscenarioRetro3D()
{
    switch (ObtenerEstadoEfectosVisualesMinijuegos().tema)
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


inline void BeginMode3DConEscenarioRetro3D(Camera3D camara)
{
    Camera3D camaraFinal =
        AplicarTemblorGeneralACamara(camara);

    BeginMode3D(camaraFinal);
    DibujarDecoracionEscenarioRetro3D();
}


#define BeginMode3D(...) BeginMode3DConEscenarioRetro3D(__VA_ARGS__)
