#pragma once

#include "Minigames/EfectosVisualesMinijuegos.h"

#include <cmath>


// EfectosVisualesMinijuegos define un BeginMode3D centralizado. Lo
// reemplazamos aqui por una version que conserva el temblor general y usa
// escenarios mas grandes para Color Seguro y Pelotas. Cueva y Magnetico
// siguen reutilizando sus decoraciones anteriores.
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


//==================================================
// COLOR SEGURO - CUEVA VOLCANICA
//==================================================

inline void DibujarVolcanFondoRetro(
    float x,
    float z,
    float radioBase,
    float altura,
    int variante
)
{
    Vector3 base =
    {
        x,
        -3.2f,
        z
    };

    Vector3 cima =
    {
        x,
        -3.2f + altura,
        z
    };

    Color roca =
        variante % 2 == 0
            ? Color{ 68, 62, 65, 255 }
            : Color{ 83, 72, 69, 255 };

    DrawCylinderEx(
        base,
        cima,
        radioBase,
        radioBase * 0.20f,
        12,
        roca
    );

    DrawCylinderEx(
        { x, cima.y - 0.45f, z },
        { x, cima.y - 0.05f, z },
        radioBase * 0.29f,
        radioBase * 0.22f,
        12,
        Color{ 38, 34, 37, 255 }
    );

    DrawCylinderEx(
        { x, cima.y - 0.16f, z },
        { x, cima.y + 0.05f, z },
        radioBase * 0.18f,
        radioBase * 0.13f,
        12,
        Color{ 246, 91, 25, 255 }
    );

    DrawSphere(
        { x, cima.y + 0.12f, z },
        radioBase * 0.09f,
        Color{ 255, 185, 48, 255 }
    );
}


inline void DibujarCuevaBordesRetro()
{
    const Color rocaExterior =
        Color{ 18, 18, 21, 255 };

    const Color rocaInterior =
        Color{ 30, 29, 32, 255 };

    // Todo se hace deliberadamente mas grande que el encuadre. La camara
    // puede cambiar de resolucion sin revelar huecos en las esquinas.
    DrawCube(
        { 0.0f, 20.0f, -10.0f },
        90.0f,
        20.0f,
        48.0f,
        rocaExterior
    );

    DrawCube(
        { -27.0f, 6.0f, -7.0f },
        26.0f,
        34.0f,
        52.0f,
        rocaExterior
    );

    DrawCube(
        { 27.0f, 6.0f, -7.0f },
        26.0f,
        34.0f,
        52.0f,
        rocaExterior
    );

    DrawCube(
        { -18.0f, 11.7f, -8.0f },
        13.0f,
        5.2f,
        30.0f,
        rocaInterior
    );

    DrawCube(
        { 18.0f, 11.7f, -8.0f },
        13.0f,
        5.2f,
        30.0f,
        rocaInterior
    );

    const float posicionesX[8] =
    {
        -20.0f,
        -16.5f,
        -13.0f,
        -10.3f,
        10.3f,
        13.0f,
        16.5f,
        20.0f
    };

    for (int i = 0; i < 8; i++)
    {
        float largo =
            2.2f +
            (float)(i % 3) * 0.75f;

        DrawCylinderEx(
            { posicionesX[i], 10.9f, -6.5f },
            { posicionesX[i], 10.9f - largo, -6.5f },
            0.65f,
            0.05f,
            8,
            Color{ 42, 40, 43, 255 }
        );
    }
}


inline void DibujarCenizaRetro()
{
    EstadoEfectosVisualesMinijuegos& estado =
        ObtenerEstadoEfectosVisualesMinijuegos();

    for (int i = 0; i < 34; i++)
    {
        float baseX =
            -15.0f +
            RepetirPositivoEscenarioRetro(
                (float)i * 7.17f,
                30.0f
            );

        float baseZ =
            -14.0f +
            RepetirPositivoEscenarioRetro(
                (float)i * 5.63f,
                28.0f
            );

        float progreso =
            RepetirPositivoEscenarioRetro(
                estado.tiempoGlobal *
                    (0.34f + 0.025f * (float)(i % 5)) +
                (float)i * 0.73f,
                8.5f
            );

        float y =
            1.1f + progreso;

        float desplazamiento =
            std::sin(
                estado.tiempoGlobal * 0.62f +
                (float)i * 1.3f
            ) * 0.55f;

        DrawSphere(
            {
                baseX + desplazamiento,
                y,
                baseZ
            },
            0.045f + 0.018f * (float)(i % 4),
            Fade(Color{ 95, 88, 84, 255 }, 0.72f)
        );
    }
}


inline void DibujarEscenarioLavaPantallaCompleta()
{
    EstadoEfectosVisualesMinijuegos& estado =
        ObtenerEstadoEfectosVisualesMinijuegos();

    // Fondo negro gigante. Es un respaldo 3D colocado muy atras: incluso
    // en resoluciones anchas cubre el viewport completo.
    DrawCube(
        { 0.0f, 8.0f, -54.0f },
        150.0f,
        74.0f,
        1.0f,
        Color{ 6, 7, 11, 255 }
    );

    // Estrellas del cielo abierto que se ve entre las rocas de la cueva.
    for (int i = 0; i < 32; i++)
    {
        float x =
            -43.0f +
            RepetirPositivoEscenarioRetro(
                (float)i * 17.3f,
                86.0f
            );

        float y =
            3.5f +
            RepetirPositivoEscenarioRetro(
                (float)i * 6.4f,
                19.0f
            );

        float radio =
            0.07f + 0.025f * (float)(i % 3);

        DrawSphere(
            { x, y, -52.9f },
            radio,
            i % 5 == 0
                ? Color{ 255, 221, 166, 255 }
                : RAYWHITE
        );
    }

    // Cordillera volcanica de fondo. Los laterales sobresalen mucho del
    // encuadre para que nunca termine la escenografia en los bordes.
    const float volcanesX[7] =
    {
        -34.0f,
        -24.0f,
        -14.0f,
        -3.5f,
        7.5f,
        19.0f,
        32.0f
    };

    for (int i = 0; i < 7; i++)
    {
        float radio =
            7.2f + 0.85f * (float)(i % 3);

        float altura =
            10.8f + 1.8f * (float)(i % 4);

        DibujarVolcanFondoRetro(
            volcanesX[i],
            -33.0f - 1.4f * (float)(i % 2),
            radio,
            altura,
            i
        );
    }

    // Piscina de lava enorme bajo todas las plataformas.
    DrawCube(
        { 0.0f, -3.65f, 3.0f },
        100.0f,
        0.80f,
        105.0f,
        Color{ 178, 34, 24, 255 }
    );

    DrawCube(
        { 0.0f, -3.20f, 3.0f },
        98.0f,
        0.10f,
        102.0f,
        Color{ 255, 106, 22, 255 }
    );

    for (int i = 0; i < 14; i++)
    {
        float x =
            -30.0f + (float)i * 4.7f;

        float z =
            -9.0f +
            RepetirPositivoEscenarioRetro(
                (float)i * 8.1f,
                34.0f
            );

        float pulso =
            0.10f +
            0.035f *
            (1.0f + std::sin(
                estado.tiempoGlobal * 2.0f +
                (float)i
            ));

        DrawSphere(
            { x, -3.07f, z },
            pulso,
            Color{ 255, 210, 60, 255 }
        );
    }

    DibujarCuevaBordesRetro();
    DibujarCenizaRetro();
}


//==================================================
// PELOTAS - CUMBRE NEVADA
//==================================================

inline void DibujarMontanaNieveFondoRetro(
    float x,
    float z,
    float radioBase,
    float altura,
    int variante
)
{
    Vector3 base =
    {
        x,
        -12.5f,
        z
    };

    Vector3 cima =
    {
        x,
        -12.5f + altura,
        z
    };

    Color roca =
        variante % 2 == 0
            ? Color{ 137, 148, 159, 255 }
            : Color{ 115, 129, 143, 255 };

    DrawCylinderEx(
        base,
        cima,
        radioBase,
        0.30f,
        11,
        roca
    );

    float inicioNieve =
        cima.y - altura * 0.29f;

    DrawCylinderEx(
        { x, inicioNieve, z },
        { x, cima.y + 0.05f, z },
        radioBase * 0.30f,
        0.16f,
        11,
        variante % 3 == 0
            ? Color{ 239, 247, 252, 255 }
            : RAYWHITE
    );
}


inline void DibujarCoposNieveRetro()
{
    EstadoEfectosVisualesMinijuegos& estado =
        ObtenerEstadoEfectosVisualesMinijuegos();

    for (int i = 0; i < 72; i++)
    {
        float xBase =
            -16.5f +
            RepetirPositivoEscenarioRetro(
                (float)i * 7.37f,
                33.0f
            );

        float z =
            10.0f -
            RepetirPositivoEscenarioRetro(
                (float)i * 5.91f,
                34.0f
            );

        float velocidad =
            1.05f + 0.12f * (float)(i % 5);

        float caida =
            RepetirPositivoEscenarioRetro(
                estado.tiempoGlobal * velocidad +
                (float)i * 0.61f,
                13.5f
            );

        float y =
            12.5f - caida;

        float deriva =
            std::sin(
                estado.tiempoGlobal * 0.85f +
                (float)i * 0.92f
            ) *
            (0.22f + 0.06f * (float)(i % 4));

        float radio =
            0.035f + 0.016f * (float)(i % 4);

        DrawSphere(
            { xBase + deriva, y, z },
            radio,
            Fade(RAYWHITE, 0.94f)
        );
    }
}


inline void DibujarEscenarioNievePantallaCompleta()
{
    // Respaldo celeste gigante para que nunca se vea el color de limpieza
    // fuera de la escenografia, incluso con relacion de aspecto ancha.
    DrawCube(
        { 0.0f, 8.0f, -58.0f },
        160.0f,
        84.0f,
        1.0f,
        Color{ 80, 183, 232, 255 }
    );

    // Montanas de fondo solapadas. Se extienden bastante mas alla de los
    // laterales visibles, siguiendo el boceto del usuario.
    const float montanasX[8] =
    {
        -38.0f,
        -28.0f,
        -18.0f,
        -8.0f,
        3.0f,
        14.0f,
        26.0f,
        39.0f
    };

    for (int i = 0; i < 8; i++)
    {
        float radio =
            10.0f + 1.2f * (float)(i % 3);

        float altura =
            25.0f + 3.0f * (float)(i % 4);

        DibujarMontanaNieveFondoRetro(
            montanasX[i],
            -34.0f - 1.4f * (float)(i % 2),
            radio,
            altura,
            i
        );
    }

    // La montana jugable se prolonga muchisimo hacia abajo. Desde la
    // camara parece una cumbre alta y no una plataforma flotante.
    DrawCylinderEx(
        { 0.0f, -48.0f, 0.0f },
        { 0.0f, -1.10f, 0.0f },
        25.0f,
        6.7f,
        18,
        Color{ 91, 112, 133, 255 }
    );

    DrawCylinderEx(
        { 0.0f, -8.0f, 0.0f },
        { 0.0f, -0.72f, 0.0f },
        10.5f,
        6.9f,
        18,
        Color{ 220, 234, 243, 255 }
    );

    // Acumulaciones grandes de nieve alrededor de la cima.
    for (int i = 0; i < 16; i++)
    {
        float angulo =
            (2.0f * PI * (float)i) / 16.0f;

        float radio =
            7.0f + 0.30f * (float)(i % 3);

        DrawSphere(
            {
                std::cos(angulo) * radio,
                -0.42f,
                std::sin(angulo) * radio
            },
            0.42f + 0.08f * (float)(i % 3),
            Fade(RAYWHITE, 0.97f)
        );
    }

    DibujarCoposNieveRetro();
}


inline void DibujarDecoracionEscenarioRetro()
{
    switch (
        ObtenerEstadoEfectosVisualesMinijuegos().tema
    )
    {
        case TEMA_VISUAL_LAVA:
            DibujarEscenarioLavaPantallaCompleta();
            break;

        case TEMA_VISUAL_NIEVE:
            DibujarEscenarioNievePantallaCompleta();
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
    Camera3D camaraFinal =
        AplicarTemblorGeneralACamara(camara);

    BeginMode3D(camaraFinal);
    DibujarDecoracionEscenarioRetro();
}


#define BeginMode3D(...) BeginMode3DConEscenarioRetro(__VA_ARGS__)
