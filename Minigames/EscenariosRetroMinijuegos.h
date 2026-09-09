#pragma once

#include "Minigames/EfectosVisualesMinijuegos.h"

#include <cmath>


//==================================================
// ESCENARIOS RETRO COMPARTIDOS
//==================================================
//
// Color Seguro y Pelotas usan escenarios 3D muy extendidos para que no
// aparezcan cortes laterales ni fondos vacios al cambiar resolucion.
// Las cimas nevadas no son objetos separados: son caras blancas dentro
// de la misma montana low-poly.
//==================================================

#ifdef BeginMode3D
#undef BeginMode3D
#endif


inline float RepetirPositivoEscenarioRetro(
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


inline Color OscurecerColorRetro(
    Color color,
    float factor
)
{
    if (factor < 0.0f)
    {
        factor = 0.0f;
    }

    if (factor > 1.0f)
    {
        factor = 1.0f;
    }

    return Color
    {
        (unsigned char)((float)color.r * factor),
        (unsigned char)((float)color.g * factor),
        (unsigned char)((float)color.b * factor),
        color.a
    };
}


//==================================================
// CIELOS 3D MUY EXTENDIDOS
//==================================================

inline void DibujarCielo3DExtendidoRetro(
    Color color,
    float zFondo
)
{
    // Plano posterior enorme. El ancho y alto superan por mucho lo visible
    // para que el cielo siga existiendo incluso en resoluciones ultrawide.
    DrawCube(
        { 0.0f, 15.0f, zFondo },
        220.0f,
        100.0f,
        1.0f,
        color
    );

    // Laterales adicionales para evitar que una perspectiva muy abierta
    // revele vacio en los bordes del plano principal.
    DrawCube(
        { -108.0f, 8.0f, zFondo + 20.0f },
        1.0f,
        90.0f,
        80.0f,
        color
    );

    DrawCube(
        { 108.0f, 8.0f, zFondo + 20.0f },
        1.0f,
        90.0f,
        80.0f,
        color
    );
}


inline void DibujarEstrellas3DExtendidasRetro()
{
    EstadoEfectosVisualesMinijuegos& estado =
        ObtenerEstadoEfectosVisualesMinijuegos();

    for (int i = 0; i < 120; i++)
    {
        float x =
            -78.0f +
            RepetirPositivoEscenarioRetro(
                (float)i * 13.73f,
                156.0f
            );

        float y =
            1.5f +
            RepetirPositivoEscenarioRetro(
                (float)i * 6.37f,
                35.0f
            );

        float z =
            -75.0f -
            (float)(i % 5) * 1.8f;

        float pulso =
            0.78f +
            std::sin(
                estado.tiempoGlobal * 1.7f +
                (float)i * 0.63f
            ) * 0.18f;

        float radio =
            (i % 11 == 0 ? 0.11f : 0.055f) * pulso;

        DrawSphere(
            { x, y, z },
            radio,
            i % 9 == 0
                ? Color{ 255, 223, 176, 255 }
                : RAYWHITE
        );
    }
}


//==================================================
// MONTANAS LOW-POLY
//==================================================

inline void DibujarMontanaNevada3DRetro(
    Vector3 centroBase,
    float radioBase,
    float altura,
    int lados,
    Color roca,
    float inicioNieve = 0.72f
)
{
    if (lados < 6)
    {
        lados = 6;
    }

    Vector3 cima =
    {
        centroBase.x,
        centroBase.y + altura,
        centroBase.z
    };

    float alturaNieve =
        centroBase.y +
        altura * inicioNieve;

    float radioNieve =
        radioBase *
        (1.0f - inicioNieve) * 1.05f;

    for (int i = 0; i < lados; i++)
    {
        float a0 =
            2.0f * PI *
            (float)i /
            (float)lados;

        float a1 =
            2.0f * PI *
            (float)(i + 1) /
            (float)lados;

        float irregularidad0 =
            1.0f +
            0.055f * std::sin((float)i * 1.7f);

        float irregularidad1 =
            1.0f +
            0.055f * std::sin((float)(i + 1) * 1.7f);

        Vector3 base0 =
        {
            centroBase.x +
                std::cos(a0) * radioBase * irregularidad0,
            centroBase.y,
            centroBase.z +
                std::sin(a0) * radioBase * irregularidad0
        };

        Vector3 base1 =
        {
            centroBase.x +
                std::cos(a1) * radioBase * irregularidad1,
            centroBase.y,
            centroBase.z +
                std::sin(a1) * radioBase * irregularidad1
        };

        Vector3 nieve0 =
        {
            centroBase.x +
                std::cos(a0) * radioNieve * irregularidad0,
            alturaNieve,
            centroBase.z +
                std::sin(a0) * radioNieve * irregularidad0
        };

        Vector3 nieve1 =
        {
            centroBase.x +
                std::cos(a1) * radioNieve * irregularidad1,
            alturaNieve,
            centroBase.z +
                std::sin(a1) * radioNieve * irregularidad1
        };

        float sombraCara =
            0.76f +
            0.16f *
            (0.5f + 0.5f * std::sin(a0 + 0.8f));

        Color rocaCara =
            OscurecerColorRetro(
                roca,
                sombraCara
            );

        // Parte rocosa de la misma superficie.
        DrawTriangle3D(
            base0,
            base1,
            nieve1,
            rocaCara
        );

        DrawTriangle3D(
            base0,
            nieve1,
            nieve0,
            rocaCara
        );

        // Punta blanca: no es otro modelo ni un cono encima. Son las caras
        // superiores del mismo volumen pintadas de blanco, como una textura.
        Color nieveCara =
            i % 3 == 0
                ? Color{ 230, 239, 245, 255 }
                : RAYWHITE;

        DrawTriangle3D(
            nieve0,
            nieve1,
            cima,
            nieveCara
        );
    }
}


inline void DibujarVolcan3DRetro(
    Vector3 centroBase,
    float radioBase,
    float altura,
    int lados,
    Color roca,
    bool activo
)
{
    if (lados < 6)
    {
        lados = 6;
    }

    float radioCrater =
        radioBase * 0.20f;

    float yCrater =
        centroBase.y + altura;

    for (int i = 0; i < lados; i++)
    {
        float a0 =
            2.0f * PI * (float)i / (float)lados;

        float a1 =
            2.0f * PI * (float)(i + 1) / (float)lados;

        Vector3 base0 =
        {
            centroBase.x + std::cos(a0) * radioBase,
            centroBase.y,
            centroBase.z + std::sin(a0) * radioBase
        };

        Vector3 base1 =
        {
            centroBase.x + std::cos(a1) * radioBase,
            centroBase.y,
            centroBase.z + std::sin(a1) * radioBase
        };

        Vector3 crater0 =
        {
            centroBase.x + std::cos(a0) * radioCrater,
            yCrater,
            centroBase.z + std::sin(a0) * radioCrater
        };

        Vector3 crater1 =
        {
            centroBase.x + std::cos(a1) * radioCrater,
            yCrater,
            centroBase.z + std::sin(a1) * radioCrater
        };

        float factor =
            0.74f +
            0.18f *
            (0.5f + 0.5f * std::sin(a0 + 0.4f));

        Color cara =
            OscurecerColorRetro(
                roca,
                factor
            );

        DrawTriangle3D(
            base0,
            base1,
            crater1,
            cara
        );

        DrawTriangle3D(
            base0,
            crater1,
            crater0,
            cara
        );

        Vector3 centroCrater =
        {
            centroBase.x,
            yCrater - 0.06f,
            centroBase.z
        };

        DrawTriangle3D(
            crater0,
            crater1,
            centroCrater,
            activo
                ? Color{ 255, 103, 20, 255 }
                : Color{ 37, 31, 33, 255 }
        );
    }
}


//==================================================
// COLOR SEGURO - CUEVA VOLCANICA 3D
//==================================================

inline void DibujarLavaColorSeguro3DRetro()
{
    DrawCube(
        { 0.0f, -3.45f, 4.0f },
        150.0f,
        0.55f,
        150.0f,
        Color{ 224, 49, 20, 255 }
    );

    DrawCube(
        { 0.0f, -3.13f, 4.0f },
        148.0f,
        0.10f,
        148.0f,
        Color{ 255, 102, 20, 255 }
    );

    EstadoEfectosVisualesMinijuegos& estado =
        ObtenerEstadoEfectosVisualesMinijuegos();

    for (int i = 0; i < 24; i++)
    {
        float x =
            -45.0f +
            RepetirPositivoEscenarioRetro(
                (float)i * 7.6f +
                estado.tiempoGlobal * 1.8f,
                90.0f
            );

        float z =
            -20.0f +
            RepetirPositivoEscenarioRetro(
                (float)i * 11.3f,
                70.0f
            );

        DrawSphere(
            { x, -3.02f, z },
            0.07f + 0.025f * (float)(i % 3),
            Color{ 255, 208, 55, 255 }
        );
    }
}


inline void DibujarCordilleraVolcanica3DRetro()
{
    for (int fila = 0; fila < 2; fila++)
    {
        float z =
            fila == 0
                ? -27.0f
                : -44.0f;

        float baseY =
            fila == 0
                ? -3.35f
                : -4.6f;

        for (int i = -6; i <= 6; i++)
        {
            float x =
                (float)i * 9.5f +
                (fila == 0 ? 0.0f : 4.5f);

            float radio =
                (fila == 0 ? 7.2f : 9.0f) +
                0.55f * (float)((i + 12) % 3);

            float altura =
                (fila == 0 ? 10.0f : 13.5f) +
                1.1f * (float)((i + 12) % 4);

            DibujarVolcan3DRetro(
                { x, baseY, z },
                radio,
                altura,
                10,
                fila == 0
                    ? Color{ 80, 70, 70, 255 }
                    : Color{ 60, 57, 62, 255 },
                ((i + fila) % 4) == 0
            );
        }
    }
}


inline void DibujarCuevaColorSeguro3DRetro()
{
    const Color roca =
        Color{ 24, 23, 26, 255 };

    DrawCube(
        { -24.0f, 8.0f, -8.0f },
        15.0f,
        34.0f,
        80.0f,
        roca
    );

    DrawCube(
        { 24.0f, 8.0f, -8.0f },
        15.0f,
        34.0f,
        80.0f,
        roca
    );

    DrawCube(
        { 0.0f, 24.0f, -10.0f },
        70.0f,
        13.0f,
        80.0f,
        roca
    );

    const float posicionesX[8] =
    {
        -18.0f,
        -13.5f,
        -9.0f,
        -4.5f,
        4.5f,
        9.0f,
        13.5f,
        18.0f
    };

    for (int i = 0; i < 8; i++)
    {
        float largo =
            2.0f +
            0.7f * (float)(i % 3);

        DrawCylinderEx(
            { posicionesX[i], 18.5f, -8.0f },
            { posicionesX[i], 18.5f - largo, -8.0f },
            0.50f,
            0.04f,
            8,
            Color{ 42, 40, 44, 255 }
        );
    }
}


inline void DibujarCenizaColorSeguro3DRetro()
{
    EstadoEfectosVisualesMinijuegos& estado =
        ObtenerEstadoEfectosVisualesMinijuegos();

    for (int i = 0; i < 42; i++)
    {
        float x =
            -18.0f +
            RepetirPositivoEscenarioRetro(
                (float)i * 5.7f +
                estado.tiempoGlobal * 0.9f,
                36.0f
            );

        float y =
            0.5f +
            RepetirPositivoEscenarioRetro(
                (float)i * 1.9f +
                estado.tiempoGlobal * 0.34f,
                9.0f
            );

        float z =
            -16.0f +
            RepetirPositivoEscenarioRetro(
                (float)i * 4.1f,
                30.0f
            );

        DrawSphere(
            { x, y, z },
            0.035f + 0.018f * (float)(i % 3),
            Fade(Color{ 110, 101, 96, 255 }, 0.72f)
        );
    }
}


inline void DibujarEscenarioColorSeguro3DRetro()
{
    DibujarCielo3DExtendidoRetro(
        Color{ 5, 6, 10, 255 },
        -82.0f
    );

    DibujarEstrellas3DExtendidasRetro();
    DibujarCordilleraVolcanica3DRetro();
    DibujarLavaColorSeguro3DRetro();
    DibujarCuevaColorSeguro3DRetro();
    DibujarCenizaColorSeguro3DRetro();
}


//==================================================
// PELOTAS - CORDILLERA NEVADA 3D
//==================================================

inline void DibujarCordilleraNevada3DRetro()
{
    // Dos filas y muchas montanas a ambos lados. La extension lateral es
    // deliberadamente exagerada para que nunca se vea donde termina.
    for (int fila = 0; fila < 2; fila++)
    {
        float z =
            fila == 0
                ? -27.0f
                : -46.0f;

        float baseY =
            fila == 0
                ? -8.0f
                : -10.0f;

        for (int i = -7; i <= 7; i++)
        {
            float x =
                (float)i * 9.0f +
                (fila == 0 ? 0.0f : 4.3f);

            float radio =
                (fila == 0 ? 7.4f : 9.6f) +
                0.65f * (float)((i + 16) % 3);

            float altura =
                (fila == 0 ? 17.0f : 22.0f) +
                1.7f * (float)((i + 16) % 4);

            Color roca =
                fila == 0
                    ? Color{ 132, 146, 159, 255 }
                    : Color{ 101, 118, 135, 255 };

            DibujarMontanaNevada3DRetro(
                { x, baseY, z },
                radio,
                altura,
                10,
                roca,
                0.70f + 0.025f * (float)((i + 16) % 3)
            );
        }
    }
}


inline void DibujarCoposNieve3DRetro()
{
    EstadoEfectosVisualesMinijuegos& estado =
        ObtenerEstadoEfectosVisualesMinijuegos();

    for (int i = 0; i < 80; i++)
    {
        float xBase =
            -22.0f +
            RepetirPositivoEscenarioRetro(
                (float)i * 7.2f,
                44.0f
            );

        float z =
            11.0f -
            RepetirPositivoEscenarioRetro(
                (float)i * 5.3f,
                38.0f
            );

        float velocidad =
            0.9f +
            0.10f * (float)(i % 5);

        float y =
            14.0f -
            RepetirPositivoEscenarioRetro(
                estado.tiempoGlobal * velocidad +
                (float)i * 0.67f,
                16.0f
            );

        float deriva =
            std::sin(
                estado.tiempoGlobal * 0.82f +
                (float)i * 0.91f
            ) * 0.30f;

        DrawSphere(
            { xBase + deriva, y, z },
            0.035f + 0.018f * (float)(i % 4),
            Fade(RAYWHITE, 0.92f)
        );
    }
}


inline void DibujarEscenarioPelotas3DRetro()
{
    DibujarCielo3DExtendidoRetro(
        Color{ 72, 178, 231, 255 },
        -84.0f
    );

    DibujarCordilleraNevada3DRetro();

    // Prolongacion de la montana jugable hacia abajo para que parezca una
    // cumbre enorme y no una plataforma flotante.
    DrawCylinderEx(
        { 0.0f, -44.0f, 0.0f },
        { 0.0f, -1.82f, 0.0f },
        24.0f,
        6.55f,
        18,
        Color{ 88, 108, 128, 255 }
    );

    DrawCylinderEx(
        { 0.0f, -7.5f, 0.0f },
        { 0.0f, -1.62f, 0.0f },
        10.5f,
        6.40f,
        18,
        Color{ 197, 216, 229, 255 }
    );

    DibujarCoposNieve3DRetro();
}


//==================================================
// COMPATIBILIDAD / DIBUJO CENTRALIZADO
//==================================================

inline void DibujarCoposNieveFrenteRetro()
{
    // Se mantiene por compatibilidad con llamadas antiguas. La nieve ahora
    // se dibuja en 3D dentro del escenario para conservar profundidad.
}


inline void DibujarFondoEscenarioRetro2D()
{
    // Color Seguro y Pelotas ya no usan montanas ni cielo 2D.
}


inline void DibujarDecoracionEscenarioRetro3D()
{
    switch (
        ObtenerEstadoEfectosVisualesMinijuegos().tema
    )
    {
        case TEMA_VISUAL_LAVA:
            DibujarEscenarioColorSeguro3DRetro();
            break;

        case TEMA_VISUAL_NIEVE:
            DibujarEscenarioPelotas3DRetro();
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


inline void BeginMode3DConEscenarioRetro(
    Camera3D camara
)
{
    DibujarFondoEscenarioRetro2D();

    Camera3D camaraFinal =
        AplicarTemblorGeneralACamara(camara);

    BeginMode3D(camaraFinal);
    DibujarDecoracionEscenarioRetro3D();
}


#define BeginMode3D(...) BeginMode3DConEscenarioRetro(__VA_ARGS__)
