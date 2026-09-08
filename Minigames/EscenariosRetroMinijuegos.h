#pragma once

#include "Minigames/EfectosVisualesMinijuegos.h"

#include <cmath>


// EfectosVisualesMinijuegos ya centraliza BeginMode3D para aplicar el
// temblor general. Aqui lo extendemos con fondos 2D de pantalla completa.
// La razon es simple: los fondos enormes hechos como geometria 3D podian
// entrar en el frustum, tapar la camara o deformarse segun la perspectiva.
// Un fondo 2D siempre ocupa exactamente el viewport; la geometria 3D queda
// reservada para los elementos que realmente necesitan profundidad.
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


inline void DibujarCuadrilateroRetro(
    Vector2 a,
    Vector2 b,
    Vector2 c,
    Vector2 d,
    Color color
)
{
    DrawTriangle(a, b, c, color);
    DrawTriangle(a, c, d, color);
}


inline void DibujarTexturaFondoCompletoRetro(
    const TexturaOpcionalMinijuego& slot
)
{
    if (!slot.cargada || !IsTextureValid(slot.textura))
    {
        return;
    }

    Rectangle origen =
    {
        0.0f,
        0.0f,
        (float)slot.textura.width,
        (float)slot.textura.height
    };

    Rectangle destino =
    {
        0.0f,
        0.0f,
        (float)GetScreenWidth(),
        (float)GetScreenHeight()
    };

    DrawTexturePro(
        slot.textura,
        origen,
        destino,
        { 0.0f, 0.0f },
        0.0f,
        WHITE
    );
}


//==================================================
// COLOR SEGURO - FONDO 2D DE CUEVA VOLCANICA
//==================================================

inline void DibujarVolcanPantallaRetro(
    float centroX,
    float baseY,
    float ancho,
    float altura,
    Color roca,
    bool activo
)
{
    Vector2 izquierda =
    {
        centroX - ancho * 0.50f,
        baseY
    };

    Vector2 cima =
    {
        centroX,
        baseY - altura
    };

    Vector2 derecha =
    {
        centroX + ancho * 0.50f,
        baseY
    };

    DrawTriangle(
        izquierda,
        cima,
        derecha,
        roca
    );

    float anchoCrater = ancho * 0.13f;
    float yCrater = cima.y + altura * 0.08f;

    DrawRectangle(
        (int)(centroX - anchoCrater * 0.50f),
        (int)yCrater,
        (int)anchoCrater,
        4,
        Color{ 37, 30, 32, 255 }
    );

    if (activo)
    {
        DrawRectangle(
            (int)(centroX - anchoCrater * 0.34f),
            (int)yCrater + 1,
            (int)(anchoCrater * 0.68f),
            3,
            Color{ 255, 105, 23, 255 }
        );
    }
}


inline void DibujarFondoColorSeguroRetro()
{
    const int ancho = GetScreenWidth();
    const int alto = GetScreenHeight();

    DrawRectangle(
        0,
        0,
        ancho,
        alto,
        Color{ 5, 6, 10, 255 }
    );

    TexturasTematicasMinijuegos& texturas =
        ObtenerTexturasTematicasMinijuegos();

    if (texturas.lavaFondo.cargada)
    {
        DibujarTexturaFondoCompletoRetro(
            texturas.lavaFondo
        );
    }

    EstadoEfectosVisualesMinijuegos& estado =
        ObtenerEstadoEfectosVisualesMinijuegos();

    for (int i = 0; i < 42; i++)
    {
        int x =
            (i * 157 + 43) %
            (ancho > 1 ? ancho : 1);

        int zonaEstrellas =
            (int)((float)alto * 0.50f);

        int y =
            18 +
            ((i * 89 + 31) %
             (zonaEstrellas > 20 ? zonaEstrellas - 20 : 1));

        float pulso =
            0.70f +
            0.30f *
            std::sin(
                estado.tiempoGlobal * 1.8f +
                (float)i
            );

        int radio =
            i % 7 == 0 ? 2 : 1;

        DrawCircle(
            x,
            y,
            (float)radio,
            Fade(RAYWHITE, 0.55f + pulso * 0.35f)
        );
    }

    const float baseVolcanes =
        (float)alto * 0.60f;

    const float posiciones[7] =
    {
        -0.08f,
        0.10f,
        0.28f,
        0.49f,
        0.69f,
        0.88f,
        1.08f
    };

    for (int i = 0; i < 7; i++)
    {
        float centroX =
            posiciones[i] * (float)ancho;

        float anchoVolcan =
            (float)ancho *
            (0.24f + 0.025f * (float)(i % 3));

        float alturaVolcan =
            (float)alto *
            (0.24f + 0.025f * (float)(i % 4));

        Color roca =
            i % 2 == 0
                ? Color{ 70, 69, 72, 255 }
                : Color{ 87, 80, 80, 255 };

        DibujarVolcanPantallaRetro(
            centroX,
            baseVolcanes,
            anchoVolcan,
            alturaVolcan,
            roca,
            i == 1 || i == 3 || i == 5
        );
    }

    int inicioLava =
        (int)((float)alto * 0.54f);

    DrawRectangleGradientV(
        0,
        inicioLava,
        ancho,
        alto - inicioLava,
        Color{ 242, 76, 19, 255 },
        Color{ 125, 19, 18, 255 }
    );

    const int segmentosOrilla = 12;
    float anchoSegmento =
        (float)ancho / (float)segmentosOrilla;

    for (int i = 0; i < segmentosOrilla; i++)
    {
        float x0 =
            (float)i * anchoSegmento - 2.0f;

        float x1 =
            (float)(i + 1) * anchoSegmento + 2.0f;

        float variacion =
            (float)alto *
            (0.015f + 0.010f * (float)(i % 3));

        float yCentro =
            (float)inicioLava +
            std::sin((float)i * 1.4f) * variacion;

        DrawTriangle(
            { x0, (float)inicioLava - variacion },
            { (x0 + x1) * 0.5f, yCentro + variacion },
            { x1, (float)inicioLava - variacion * 0.4f },
            Color{ 255, 119, 22, 255 }
        );
    }

    for (int i = 0; i < 14; i++)
    {
        int x =
            (i * 137 + 53) %
            (ancho > 1 ? ancho : 1);

        int altoLava =
            alto - inicioLava;

        int y =
            inicioLava + 20 +
            ((i * 73) %
             (altoLava > 40 ? altoLava - 40 : 1));

        int largo =
            18 + (i % 4) * 13;

        DrawRectangle(
            x,
            y,
            largo,
            2,
            Fade(Color{ 255, 213, 68, 255 }, 0.55f)
        );
    }

    for (int i = 0; i < 28; i++)
    {
        float velocidad =
            9.0f + (float)(i % 5) * 2.0f;

        float x =
            RepetirPositivoEscenarioRetro(
                (float)(i * 97) +
                estado.tiempoGlobal * velocidad,
                (float)ancho + 20.0f
            ) - 10.0f;

        float y =
            45.0f +
            RepetirPositivoEscenarioRetro(
                (float)(i * 61),
                (float)alto * 0.50f
            );

        DrawCircleV(
            { x, y },
            1.0f + (float)(i % 3) * 0.55f,
            Fade(Color{ 93, 86, 83, 255 }, 0.70f)
        );
    }

    const Color rocaExterior =
        Color{ 22, 22, 24, 255 };

    const Color rocaInterior =
        Color{ 33, 32, 35, 255 };

    float w = (float)ancho;
    float h = (float)alto;

    DibujarCuadrilateroRetro(
        { 0.0f, 0.0f },
        { w * 0.17f, 0.0f },
        { w * 0.095f, h * 0.50f },
        { 0.0f, h * 0.66f },
        rocaExterior
    );

    DibujarCuadrilateroRetro(
        { w * 0.83f, 0.0f },
        { w, 0.0f },
        { w, h * 0.66f },
        { w * 0.905f, h * 0.50f },
        rocaExterior
    );

    DrawRectangle(
        0,
        0,
        ancho,
        (int)(h * 0.055f),
        rocaExterior
    );

    DrawTriangle(
        { w * 0.14f, 0.0f },
        { w * 0.20f, h * 0.18f },
        { w * 0.25f, 0.0f },
        rocaInterior
    );

    DrawTriangle(
        { w * 0.34f, 0.0f },
        { w * 0.38f, h * 0.12f },
        { w * 0.42f, 0.0f },
        rocaInterior
    );

    DrawTriangle(
        { w * 0.57f, 0.0f },
        { w * 0.61f, h * 0.15f },
        { w * 0.66f, 0.0f },
        rocaInterior
    );

    DrawTriangle(
        { w * 0.75f, 0.0f },
        { w * 0.81f, h * 0.19f },
        { w * 0.86f, 0.0f },
        rocaInterior
    );
}


//==================================================
// PELOTAS - FONDO 2D DE CORDILLERA + NIEVE
//==================================================

inline void DibujarMontanaPantallaRetro(
    float centroX,
    float baseY,
    float ancho,
    float altura,
    Color roca,
    Color nieve
)
{
    Vector2 izquierda =
    {
        centroX - ancho * 0.50f,
        baseY
    };

    Vector2 cima =
    {
        centroX,
        baseY - altura
    };

    Vector2 derecha =
    {
        centroX + ancho * 0.50f,
        baseY
    };

    DrawTriangle(
        izquierda,
        cima,
        derecha,
        roca
    );

    float yNieve =
        cima.y + altura * 0.30f;

    DrawTriangle(
        { centroX - ancho * 0.15f, yNieve },
        cima,
        { centroX + ancho * 0.15f, yNieve },
        nieve
    );
}


inline void DibujarFondoPelotasRetro()
{
    const int ancho = GetScreenWidth();
    const int alto = GetScreenHeight();

    DrawRectangleGradientV(
        0,
        0,
        ancho,
        alto,
        Color{ 50, 169, 230, 255 },
        Color{ 193, 229, 246, 255 }
    );

    TexturasTematicasMinijuegos& texturas =
        ObtenerTexturasTematicasMinijuegos();

    if (texturas.nieveFondo.cargada)
    {
        DibujarTexturaFondoCompletoRetro(
            texturas.nieveFondo
        );
    }

    float baseLejana =
        (float)alto * 0.72f;

    const float posicionesLejanas[7] =
    {
        -0.12f,
        0.08f,
        0.27f,
        0.47f,
        0.68f,
        0.88f,
        1.10f
    };

    for (int i = 0; i < 7; i++)
    {
        DibujarMontanaPantallaRetro(
            posicionesLejanas[i] * (float)ancho,
            baseLejana,
            (float)ancho *
                (0.29f + 0.02f * (float)(i % 3)),
            (float)alto *
                (0.43f + 0.025f * (float)(i % 4)),
            i % 2 == 0
                ? Color{ 170, 183, 195, 255 }
                : Color{ 153, 169, 182, 255 },
            Color{ 246, 250, 252, 255 }
        );
    }

    float baseDelantera =
        (float)alto * 0.79f;

    const float posicionesDelanteras[5] =
    {
        -0.03f,
        0.24f,
        0.50f,
        0.76f,
        1.03f
    };

    for (int i = 0; i < 5; i++)
    {
        DibujarMontanaPantallaRetro(
            posicionesDelanteras[i] * (float)ancho,
            baseDelantera,
            (float)ancho * 0.36f,
            (float)alto *
                (0.40f + 0.035f * (float)(i % 3)),
            i % 2 == 0
                ? Color{ 119, 139, 157, 255 }
                : Color{ 132, 150, 166, 255 },
            RAYWHITE
        );
    }
}


inline void DibujarCoposNieveFondoRetro()
{
    EstadoEfectosVisualesMinijuegos& estado =
        ObtenerEstadoEfectosVisualesMinijuegos();

    const int ancho = GetScreenWidth();
    const int alto = GetScreenHeight();

    for (int i = 0; i < 48; i++)
    {
        float velocidad =
            25.0f + (float)(i % 5) * 7.0f;

        float y =
            RepetirPositivoEscenarioRetro(
                (float)(i * 47) +
                estado.tiempoGlobal * velocidad,
                (float)alto + 20.0f
            ) - 10.0f;

        float xBase =
            (float)((i * 131 + 37) %
                (ancho > 1 ? ancho : 1));

        float deriva =
            std::sin(
                estado.tiempoGlobal * 0.75f +
                (float)i * 0.85f
            ) *
            (7.0f + (float)(i % 4) * 3.0f);

        float radio =
            1.1f + (float)(i % 3) * 0.55f;

        DrawCircleV(
            { xBase + deriva, y },
            radio,
            Fade(RAYWHITE, 0.82f)
        );
    }
}


inline void DibujarCoposNieveFrenteRetro()
{
    EstadoEfectosVisualesMinijuegos& estado =
        ObtenerEstadoEfectosVisualesMinijuegos();

    const int ancho = GetScreenWidth();
    const int alto = GetScreenHeight();

    for (int i = 0; i < 18; i++)
    {
        float velocidad =
            42.0f + (float)(i % 4) * 8.0f;

        float y =
            RepetirPositivoEscenarioRetro(
                (float)(i * 83) +
                estado.tiempoGlobal * velocidad,
                (float)alto + 28.0f
            ) - 14.0f;

        float x =
            (float)((i * 193 + 71) %
                (ancho > 1 ? ancho : 1)) +
            std::sin(
                estado.tiempoGlobal * 1.1f +
                (float)i
            ) * 11.0f;

        DrawCircleV(
            { x, y },
            1.8f + (float)(i % 3) * 0.7f,
            Fade(RAYWHITE, 0.90f)
        );
    }
}


//==================================================
// DIBUJO 2D/3D CENTRALIZADO
//==================================================

inline void DibujarFondoEscenarioRetro2D()
{
    switch (
        ObtenerEstadoEfectosVisualesMinijuegos().tema
    )
    {
        case TEMA_VISUAL_LAVA:
            DibujarFondoColorSeguroRetro();
            break;

        case TEMA_VISUAL_NIEVE:
            DibujarFondoPelotasRetro();
            DibujarCoposNieveFondoRetro();
            break;

        case TEMA_VISUAL_CUEVA:
        case TEMA_VISUAL_MAGNETICO:
        case TEMA_VISUAL_NINGUNO:
            break;
    }
}


inline void DibujarDecoracionEscenarioRetro3D()
{
    switch (
        ObtenerEstadoEfectosVisualesMinijuegos().tema
    )
    {
        case TEMA_VISUAL_LAVA:
            break;

        case TEMA_VISUAL_NIEVE:
            DrawCylinderEx(
                { 0.0f, -24.0f, 0.0f },
                { 0.0f, -1.85f, 0.0f },
                11.8f,
                6.45f,
                18,
                Color{ 88, 108, 128, 255 }
            );

            DrawCylinderEx(
                { 0.0f, -5.0f, 0.0f },
                { 0.0f, -1.55f, 0.0f },
                7.7f,
                6.38f,
                18,
                Color{ 206, 224, 235, 255 }
            );
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
