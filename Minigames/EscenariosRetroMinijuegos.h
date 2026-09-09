#pragma once

#include "Minigames/EfectosVisualesMinijuegos.h"

#include <cmath>


//==================================================
// ESCENARIOS RETRO COMPARTIDOS
//==================================================
//
// Los fondos de Color Seguro y Pelotas se dibujan en 2D antes de entrar
// en BeginMode3D. De esta forma llenan siempre el viewport completo y no
// pueden meterse delante de la camara como pasaba con geometria gigante.
// La parte jugable sigue siendo 3D.
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


inline void DibujarCuadrilateroRetro(
    Vector2 a,
    Vector2 b,
    Vector2 c,
    Vector2 d,
    Color color
)
{
    DrawTriangle(a, c, b, color);
    DrawTriangle(a, d, c, color);
}


inline void DibujarTexturaFondoCompletoRetro(
    const TexturaOpcionalMinijuego& slot,
    float alpha = 1.0f
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
        Fade(WHITE, alpha)
    );
}


//==================================================
// COLOR SEGURO - CUEVA, VOLCANES Y LAVA
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

    Vector2 derecha =
    {
        centroX + ancho * 0.50f,
        baseY
    };

    Vector2 cima =
    {
        centroX,
        baseY - altura
    };

    // El orden izquierda -> derecha -> cima evita que el triangulo sea
    // descartado por el winding de DrawTriangle en coordenadas de pantalla.
    DrawTriangle(
        izquierda,
        derecha,
        cima,
        roca
    );

    float anchoCrater = ancho * 0.14f;
    float yCrater = cima.y + altura * 0.085f;

    DrawRectangle(
        (int)(centroX - anchoCrater * 0.50f),
        (int)yCrater,
        (int)anchoCrater,
        5,
        Color{ 32, 27, 29, 255 }
    );

    if (activo)
    {
        DrawRectangle(
            (int)(centroX - anchoCrater * 0.33f),
            (int)yCrater + 1,
            (int)(anchoCrater * 0.66f),
            3,
            Color{ 255, 111, 24, 255 }
        );

        DrawCircleV(
            { centroX, yCrater - 5.0f },
            2.0f,
            Fade(Color{ 255, 195, 54, 255 }, 0.85f)
        );
    }
}


inline void DibujarEstrellasColorSeguroRetro(
    int ancho,
    int alto
)
{
    EstadoEfectosVisualesMinijuegos& estado =
        ObtenerEstadoEfectosVisualesMinijuegos();

    int zonaEstrellas =
        (int)((float)alto * 0.52f);

    for (int i = 0; i < 46; i++)
    {
        int x =
            (i * 157 + 43) %
            (ancho > 1 ? ancho : 1);

        int y =
            18 +
            ((i * 89 + 31) %
             (zonaEstrellas > 20 ? zonaEstrellas - 20 : 1));

        float pulso =
            0.68f +
            0.32f *
            std::sin(
                estado.tiempoGlobal * 1.8f +
                (float)i * 0.71f
            );

        float radio =
            i % 8 == 0 ? 2.0f : 1.0f;

        DrawCircleV(
            { (float)x, (float)y },
            radio,
            Fade(RAYWHITE, 0.52f + pulso * 0.38f)
        );
    }
}


inline void DibujarCenizaColorSeguroRetro(
    int ancho,
    int alto
)
{
    EstadoEfectosVisualesMinijuegos& estado =
        ObtenerEstadoEfectosVisualesMinijuegos();

    for (int i = 0; i < 28; i++)
    {
        float velocidad =
            8.0f + (float)(i % 5) * 1.8f;

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
                (float)alto * 0.48f
            );

        DrawCircleV(
            { x, y },
            1.0f + (float)(i % 3) * 0.45f,
            Fade(Color{ 104, 96, 92, 255 }, 0.58f)
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

    // Las texturas siguen siendo opcionales. Se dibujan suavemente para
    // no destruir la lectura de los elementos low-poly si son muy cargadas.
    if (texturas.lavaFondo.cargada)
    {
        DibujarTexturaFondoCompletoRetro(
            texturas.lavaFondo,
            0.72f
        );
    }

    DibujarEstrellasColorSeguroRetro(ancho, alto);

    // Cadena de volcanes. Los dos extremos salen de pantalla a proposito
    // para que el fondo no tenga cortes laterales.
    const float baseVolcanes =
        (float)alto * 0.595f;

    const float posiciones[8] =
    {
        -0.10f,
        0.06f,
        0.22f,
        0.39f,
        0.57f,
        0.74f,
        0.91f,
        1.09f
    };

    for (int i = 0; i < 8; i++)
    {
        float centroX =
            posiciones[i] * (float)ancho;

        float anchoVolcan =
            (float)ancho *
            (0.22f + 0.022f * (float)(i % 3));

        float alturaVolcan =
            (float)alto *
            (0.26f + 0.035f * (float)(i % 4));

        Color roca =
            i % 2 == 0
                ? Color{ 74, 72, 76, 255 }
                : Color{ 92, 82, 80, 255 };

        DibujarVolcanPantallaRetro(
            centroX,
            baseVolcanes,
            anchoVolcan,
            alturaVolcan,
            roca,
            i == 1 || i == 3 || i == 6
        );
    }

    // Lava continua y lisa. Se elimina por completo la vieja orilla hecha
    // con triangulos, que era la responsable de los picos naranjas raros.
    int inicioLava =
        (int)((float)alto * 0.565f);

    DrawRectangleGradientV(
        0,
        inicioLava,
        ancho,
        alto - inicioLava,
        Color{ 248, 91, 18, 255 },
        Color{ 122, 18, 16, 255 }
    );

    DrawRectangle(
        0,
        inicioLava,
        ancho,
        7,
        Color{ 255, 138, 27, 255 }
    );

    EstadoEfectosVisualesMinijuegos& estado =
        ObtenerEstadoEfectosVisualesMinijuegos();

    for (int i = 0; i < 12; i++)
    {
        float x =
            RepetirPositivoEscenarioRetro(
                (float)(i * 149) +
                estado.tiempoGlobal * (10.0f + (float)(i % 4) * 2.0f),
                (float)ancho + 90.0f
            ) - 45.0f;

        float y =
            (float)inicioLava +
            36.0f +
            (float)((i * 71) %
                ((alto - inicioLava) > 90 ?
                    (alto - inicioLava - 70) : 20));

        float largo =
            25.0f + (float)(i % 4) * 16.0f;

        DrawLineEx(
            { x, y },
            { x + largo, y },
            2.0f,
            Fade(Color{ 255, 216, 64, 255 }, 0.52f)
        );
    }

    // Marco de cueva. Es intencionalmente ancho, pero deja el centro libre
    // para TV, volcanes y plataformas.
    const Color rocaExterior =
        Color{ 22, 22, 24, 255 };

    const Color rocaInterior =
        Color{ 34, 33, 36, 255 };

    float w = (float)ancho;
    float h = (float)alto;

    DibujarCuadrilateroRetro(
        { 0.0f, 0.0f },
        { w * 0.16f, 0.0f },
        { w * 0.085f, h * 0.47f },
        { 0.0f, h * 0.62f },
        rocaExterior
    );

    DibujarCuadrilateroRetro(
        { w * 0.84f, 0.0f },
        { w, 0.0f },
        { w, h * 0.62f },
        { w * 0.915f, h * 0.47f },
        rocaExterior
    );

    DrawRectangle(
        0,
        0,
        ancho,
        (int)(h * 0.050f),
        rocaExterior
    );

    DrawTriangle(
        { w * 0.13f, 0.0f },
        { w * 0.24f, 0.0f },
        { w * 0.19f, h * 0.16f },
        rocaInterior
    );

    DrawTriangle(
        { w * 0.33f, 0.0f },
        { w * 0.42f, 0.0f },
        { w * 0.375f, h * 0.12f },
        rocaInterior
    );

    DrawTriangle(
        { w * 0.57f, 0.0f },
        { w * 0.66f, 0.0f },
        { w * 0.615f, h * 0.14f },
        rocaInterior
    );

    DrawTriangle(
        { w * 0.75f, 0.0f },
        { w * 0.86f, 0.0f },
        { w * 0.805f, h * 0.17f },
        rocaInterior
    );

    DibujarCenizaColorSeguroRetro(ancho, alto);
}


//==================================================
// PELOTAS - CORDILLERA NEVADA
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

    Vector2 derecha =
    {
        centroX + ancho * 0.50f,
        baseY
    };

    Vector2 cima =
    {
        centroX,
        baseY - altura
    };

    DrawTriangle(
        izquierda,
        derecha,
        cima,
        roca
    );

    float yNieve =
        cima.y + altura * 0.31f;

    DrawTriangle(
        { centroX - ancho * 0.16f, yNieve },
        { centroX + ancho * 0.16f, yNieve },
        cima,
        nieve
    );
}


inline void DibujarCoposNieveRetro(
    int ancho,
    int alto
)
{
    EstadoEfectosVisualesMinijuegos& estado =
        ObtenerEstadoEfectosVisualesMinijuegos();

    for (int i = 0; i < 54; i++)
    {
        float velocidad =
            22.0f + (float)(i % 5) * 6.0f;

        float y =
            RepetirPositivoEscenarioRetro(
                (float)(i * 47) +
                estado.tiempoGlobal * velocidad,
                (float)alto + 24.0f
            ) - 12.0f;

        float xBase =
            (float)((i * 131 + 37) %
                (ancho > 1 ? ancho : 1));

        float deriva =
            std::sin(
                estado.tiempoGlobal * 0.72f +
                (float)i * 0.83f
            ) *
            (6.0f + (float)(i % 4) * 2.5f);

        float radio =
            1.0f + (float)(i % 3) * 0.45f;

        DrawCircleV(
            { xBase + deriva, y },
            radio,
            Fade(RAYWHITE, 0.84f)
        );
    }
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
        Color{ 48, 169, 229, 255 },
        Color{ 189, 225, 243, 255 }
    );

    TexturasTematicasMinijuegos& texturas =
        ObtenerTexturasTematicasMinijuegos();

    if (texturas.nieveFondo.cargada)
    {
        DibujarTexturaFondoCompletoRetro(
            texturas.nieveFondo,
            0.45f
        );
    }

    // Primera capa: montanas lejanas claras y altas.
    const float baseLejana =
        (float)alto * 0.67f;

    const float posicionesLejanas[8] =
    {
        -0.11f,
        0.05f,
        0.20f,
        0.36f,
        0.53f,
        0.70f,
        0.87f,
        1.08f
    };

    for (int i = 0; i < 8; i++)
    {
        DibujarMontanaPantallaRetro(
            posicionesLejanas[i] * (float)ancho,
            baseLejana,
            (float)ancho *
                (0.23f + 0.018f * (float)(i % 3)),
            (float)alto *
                (0.40f + 0.035f * (float)(i % 4)),
            i % 2 == 0
                ? Color{ 177, 188, 198, 255 }
                : Color{ 158, 173, 186, 255 },
            Color{ 246, 250, 252, 255 }
        );
    }

    // Segunda capa: montanas mas oscuras y cercanas. Se dejan huecos
    // suficientes para seguir viendo cielo y copos entre los picos.
    const float baseCercana =
        (float)alto * 0.76f;

    const float posicionesCercanas[6] =
    {
        -0.07f,
        0.14f,
        0.36f,
        0.59f,
        0.81f,
        1.05f
    };

    for (int i = 0; i < 6; i++)
    {
        DibujarMontanaPantallaRetro(
            posicionesCercanas[i] * (float)ancho,
            baseCercana,
            (float)ancho * 0.29f,
            (float)alto *
                (0.37f + 0.03f * (float)(i % 3)),
            i % 2 == 0
                ? Color{ 121, 141, 159, 255 }
                : Color{ 135, 153, 169, 255 },
            RAYWHITE
        );
    }

    DibujarCoposNieveRetro(ancho, alto);
}


//==================================================
// SUAVIZADO VISUAL DEL MONTICULO DE PELOTAS
//==================================================
//
// MinijuegoPelotas usa colores alternados por segmento para su costado.
// Eso producia el efecto de "cebra" que se veia en la captura. Como el
// tema nieve es global solo durante ese minijuego, remapeamos esas cuatro
// tonalidades a dos colores uniformes y conservamos la forma low-poly.
//==================================================

inline bool EsColorRetro(
    Color color,
    unsigned char r,
    unsigned char g,
    unsigned char b
)
{
    return
        color.r == r &&
        color.g == g &&
        color.b == b;
}


inline Color AjustarColorTriangulo3DPorTemaRetro(
    Color color
)
{
    if (
        ObtenerEstadoEfectosVisualesMinijuegos().tema !=
        TEMA_VISUAL_NIEVE
    )
    {
        return color;
    }

    if (EsColorRetro(color, 207, 225, 235))
    {
        return Color{ 246, 250, 252, color.a };
    }

    if (
        EsColorRetro(color, 153, 190, 208) ||
        EsColorRetro(color, 93, 132, 153)
    )
    {
        return Color{ 180, 202, 215, color.a };
    }

    return color;
}


inline void DibujarTriangulo3DConTemaRetro(
    Vector3 a,
    Vector3 b,
    Vector3 c,
    Color color
)
{
    DrawTriangle3D(
        a,
        b,
        c,
        AjustarColorTriangulo3DPorTemaRetro(color)
    );
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
            // Prolongacion de la montana jugable hacia abajo. Un solo tono
            // por tramo evita texturas/rayas visuales extrañas.
            DrawCylinderEx(
                { 0.0f, -27.0f, 0.0f },
                { 0.0f, -2.00f, 0.0f },
                12.2f,
                6.45f,
                24,
                Color{ 96, 116, 135, 255 }
            );

            DrawCylinderEx(
                { 0.0f, -6.4f, 0.0f },
                { 0.0f, -1.62f, 0.0f },
                8.0f,
                6.38f,
                24,
                Color{ 211, 226, 235, 255 }
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
#define DrawTriangle3D(...) DibujarTriangulo3DConTemaRetro(__VA_ARGS__)
