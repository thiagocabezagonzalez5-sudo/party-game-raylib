#pragma once

#include "raylib.h"
#include "Minigames/EfectosVisualesMinijuegos.h"

#include <cmath>


//==================================================
// SOMBRAS RETRO DE BAJO COSTO
//==================================================
//
// En vez de shadow maps usamos manchas planas de pocos triangulos.
// Es mucho mas barato para un party game con muchas primitivas y,
// ademas, encaja mejor con el look retro/low-poly del proyecto.
//==================================================

inline float LimitarSombraRetro(
    float valor,
    float minimo,
    float maximo
)
{
    if (valor < minimo) return minimo;
    if (valor > maximo) return maximo;
    return valor;
}


inline void DibujarSombraRetroCircular(
    Vector3 posicionObjeto,
    float radioX,
    float radioZ,
    float alturaSuelo = 0.018f,
    float alphaBase = 0.27f
)
{
    if (radioX <= 0.025f || radioZ <= 0.025f)
    {
        return;
    }

    float altura =
        posicionObjeto.y - alturaSuelo;

    if (altura < 0.0f)
    {
        altura = 0.0f;
    }

    float factorAltura =
        LimitarSombraRetro(
            altura / 7.0f,
            0.0f,
            1.0f
        );

    float desplazamiento =
        LimitarSombraRetro(
            altura * 0.085f,
            0.0f,
            0.62f
        );

    float expansion =
        1.0f + factorAltura * 0.32f;

    float alpha =
        alphaBase *
        (1.0f - factorAltura * 0.58f);

    Vector3 centro =
    {
        posicionObjeto.x + desplazamiento * 0.72f,
        alturaSuelo,
        posicionObjeto.z + desplazamiento * 0.45f
    };

    const int LADOS = 8;
    Color color = Fade(BLACK, alpha);

    for (int i = 0; i < LADOS; i++)
    {
        float a0 =
            2.0f * PI *
            (float)i /
            (float)LADOS;

        float a1 =
            2.0f * PI *
            (float)(i + 1) /
            (float)LADOS;

        Vector3 p0 =
        {
            centro.x +
                std::cos(a0) * radioX * expansion,
            alturaSuelo,
            centro.z +
                std::sin(a0) * radioZ * expansion
        };

        Vector3 p1 =
        {
            centro.x +
                std::cos(a1) * radioX * expansion,
            alturaSuelo,
            centro.z +
                std::sin(a1) * radioZ * expansion
        };

        DrawTriangle3D(
            centro,
            p1,
            p0,
            color
        );
    }
}


inline void DibujarSombraRetroRectangular(
    Vector3 posicionObjeto,
    float ancho,
    float largo,
    float rotacionRad = 0.0f,
    float alturaSuelo = 0.018f,
    float alphaBase = 0.24f
)
{
    if (ancho <= 0.04f || largo <= 0.04f)
    {
        return;
    }

    float altura =
        posicionObjeto.y - alturaSuelo;

    if (altura < 0.0f)
    {
        altura = 0.0f;
    }

    float factorAltura =
        LimitarSombraRetro(
            altura / 7.0f,
            0.0f,
            1.0f
        );

    float desplazamiento =
        LimitarSombraRetro(
            altura * 0.085f,
            0.0f,
            0.62f
        );

    float expansion =
        1.0f + factorAltura * 0.22f;

    float alpha =
        alphaBase *
        (1.0f - factorAltura * 0.58f);

    float hx =
        ancho * 0.5f * expansion;

    float hz =
        largo * 0.5f * expansion;

    float c = std::cos(rotacionRad);
    float s = std::sin(rotacionRad);

    Vector3 centro =
    {
        posicionObjeto.x + desplazamiento * 0.72f,
        alturaSuelo,
        posicionObjeto.z + desplazamiento * 0.45f
    };

    Vector3 esquinas[4] =
    {
        { -hx, 0.0f, -hz },
        { hx, 0.0f, -hz },
        { hx, 0.0f, hz },
        { -hx, 0.0f, hz }
    };

    for (int i = 0; i < 4; i++)
    {
        float x = esquinas[i].x;
        float z = esquinas[i].z;

        esquinas[i].x =
            centro.x + x * c - z * s;

        esquinas[i].y =
            alturaSuelo;

        esquinas[i].z =
            centro.z + x * s + z * c;
    }

    Color color =
        Fade(BLACK, alpha);

    DrawTriangle3D(
        esquinas[0],
        esquinas[2],
        esquinas[1],
        color
    );

    DrawTriangle3D(
        esquinas[0],
        esquinas[3],
        esquinas[2],
        color
    );
}


inline bool EsSuperficieBaseRetro(
    float ancho,
    float alto,
    float largo
)
{
    return
        alto <= 0.82f &&
        ancho * largo >= 18.0f;
}


inline bool EsCuboJugadorEstandarRetro(
    float ancho,
    float alto,
    float largo
)
{
    bool plantaJugador =
        std::fabs(ancho - 0.80f) <= 0.09f &&
        std::fabs(largo - 0.80f) <= 0.09f;

    bool alturaNormal =
        alto >= 1.18f &&
        alto <= 1.52f;

    bool alturaAplastada =
        alto >= 0.20f &&
        alto <= 0.42f;

    return
        plantaJugador &&
        (alturaNormal || alturaAplastada);
}


inline bool EsAcumulacionNieveViejaRetro(
    Vector3 posicion,
    float radio,
    Color color
)
{
    if (
        ObtenerEstadoEfectosVisualesMinijuegos().tema !=
        TEMA_VISUAL_NIEVE
    )
    {
        return false;
    }

    bool casiBlanco =
        color.r >= 238 &&
        color.g >= 238 &&
        color.b >= 238;

    bool alturaBorde =
        posicion.y >= -0.55f &&
        posicion.y <= -0.08f;

    bool radioBorde =
        radio >= 0.35f &&
        radio <= 0.58f;

    // El minijuego antiguo dibujaba doce esferas blancas alrededor de la
    // cima. Con el nuevo escenario parecian bolas gigantes flotando.
    // Las filtramos aqui sin tocar la fisica ni las pelotas-jugador.
    return
        casiBlanco &&
        alturaBorde &&
        radioBorde;
}


inline void DibujarCuboConSombraRetro(
    Vector3 posicion,
    float ancho,
    float alto,
    float largo,
    Color color
)
{
    bool yaTieneSombraDeJugador =
        EsCuboJugadorEstandarRetro(
            ancho,
            alto,
            largo
        );

    bool objetoVisible =
        ancho >= 0.20f &&
        largo >= 0.20f &&
        alto >= 0.08f;

    if (
        !yaTieneSombraDeJugador &&
        !EsSuperficieBaseRetro(ancho, alto, largo) &&
        objetoVisible
    )
    {
        DibujarSombraRetroRectangular(
            posicion,
            ancho * 0.92f,
            largo * 0.92f
        );
    }

    DrawCube(
        posicion,
        ancho,
        alto,
        largo,
        color
    );
}


inline void DibujarEsferaConSombraRetro(
    Vector3 posicion,
    float radio,
    Color color
)
{
    if (
        EsAcumulacionNieveViejaRetro(
            posicion,
            radio,
            color
        )
    )
    {
        return;
    }

    // Las pelotas-jugador ya proyectan su propia sombra desde
    // UtilidadesMinijuegos. Evitamos dibujarla dos veces.
    bool pelotaJugador =
        radio >= 0.60f &&
        radio <= 0.70f;

    if (!pelotaJugador && radio >= 0.10f)
    {
        DibujarSombraRetroCircular(
            posicion,
            radio * 0.92f,
            radio * 0.72f
        );
    }

    DrawSphere(
        posicion,
        radio,
        color
    );
}


inline void DibujarCilindroConSombraRetro(
    Vector3 posicion,
    float radioSuperior,
    float radioInferior,
    float alto,
    int lados,
    Color color
)
{
    float radio =
        radioSuperior > radioInferior
            ? radioSuperior
            : radioInferior;

    bool esBaseGrande =
        alto <= 0.82f &&
        radio >= 2.35f;

    if (
        !esBaseGrande &&
        radio >= 0.10f &&
        alto >= 0.07f
    )
    {
        DibujarSombraRetroCircular(
            posicion,
            radio * 0.92f,
            radio * 0.72f
        );
    }

    DrawCylinder(
        posicion,
        radioSuperior,
        radioInferior,
        alto,
        lados,
        color
    );
}


inline void DibujarModeloExConSombraRetro(
    Model modelo,
    Vector3 posicion,
    Vector3 ejeRotacion,
    float anguloRotacion,
    Vector3 escala,
    Color tinte
)
{
    float radioX =
        std::fabs(escala.x) * 0.52f;

    float radioZ =
        std::fabs(escala.z) * 0.44f;

    if (radioX < 0.24f)
    {
        radioX = 0.24f;
    }

    if (radioZ < 0.20f)
    {
        radioZ = 0.20f;
    }

    DibujarSombraRetroCircular(
        posicion,
        radioX,
        radioZ
    );

    DrawModelEx(
        modelo,
        posicion,
        ejeRotacion,
        anguloRotacion,
        escala,
        tinte
    );
}


#ifdef SOMBRAS_RETRO_AUTOMATICAS
// Variadicas para aceptar llamadas con Vector3{ x, y, z } sin que las
// comas del inicializador confundan al preprocesador.
#define DrawCube(...) DibujarCuboConSombraRetro(__VA_ARGS__)
#define DrawSphere(...) DibujarEsferaConSombraRetro(__VA_ARGS__)
#define DrawCylinder(...) DibujarCilindroConSombraRetro(__VA_ARGS__)
#define DrawModelEx(...) DibujarModeloExConSombraRetro(__VA_ARGS__)
#endif
