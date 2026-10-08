#include "UI/Hub3D.h"

#include "raymath.h"
#include "rlgl.h"

#include <math.h>


//==================================================
// MODELO FUTURO
//==================================================
// Todo lo de este archivo es greybox con primitivas de raylib.
// Reemplazables por modelos GLB (cargados una sola vez y descargados
// en Hub3D::Descargar):
//   - Dirigible completo (DibujarDirigible) y torre de amarre.
//   - Barco y embarcadero (DibujarBarco, DibujarEmbarcadero).
//   - Torre de engranajes (DibujarTorreEngranajes, DibujarEngranaje).
//   - Arco de salida con portal (DibujarArcoSalida).
//   - Arboles, faroles, fuente y banderines (seccion DECORACION).
//   - Isla base, islas lejanas y nubes.
// La logica de seleccion y la camara no cambian al sustituirlos.


//==================================================
// CONSTANTES DE ESCENA
//==================================================

// Punto que la camara mira al enfocar cada opcion.
static const Vector3 ANCLAS_OPCION[HUB_CANTIDAD_OPCIONES] =
{
    { -15.0f, 4.6f, -1.0f },   // dirigible
    {  -7.0f, 1.8f, 11.5f },   // barco
    {   5.0f, 4.2f, -1.0f },   // torre de engranajes
    {  15.0f, 3.2f, -1.0f }    // arco de salida
};

// Base en el suelo (para halo y haz de luz).
static const Vector3 BASES_OPCION[HUB_CANTIDAD_OPCIONES] =
{
    { -15.0f, 0.0f, -1.0f },
    {  -7.0f, -0.4f, 11.5f },
    {   5.0f, 0.0f, -1.0f },
    {  15.0f, 0.0f, -1.0f }
};

static const float RADIOS_HALO[HUB_CANTIDAD_OPCIONES] =
{
    4.4f, 3.0f, 3.6f, 4.2f
};

// Alturas a las que flota el cartel de cada opcion.
static const float ALTURAS_CARTEL[HUB_CANTIDAD_OPCIONES] =
{
    8.0f, 5.2f, 10.8f, 9.6f
};

static const float DURACION_RECORRIDO =
    28.0f;

static const float CAMARA_X_MIN =
    -17.0f;

static const float CAMARA_X_MAX =
    17.0f;

static const Color COLOR_HALO =
    { 255, 214, 110, 255 };


//==================================================
// UTILIDADES
//==================================================

static float Suavizar(
    float x
)
{
    if (x < 0.0f)
    {
        x = 0.0f;
    }

    if (x > 1.0f)
    {
        x = 1.0f;
    }

    return x * x * (3.0f - 2.0f * x);
}


static float MoverHacia(
    float valor,
    float objetivo,
    float paso
)
{
    if (valor < objetivo)
    {
        valor += paso;

        if (valor > objetivo)
        {
            valor = objetivo;
        }
    }
    else
    {
        valor -= paso;

        if (valor < objetivo)
        {
            valor = objetivo;
        }
    }

    return valor;
}


static float Hash01(
    int n
)
{
    unsigned int x =
        (unsigned int)n * 747796405u + 2891336453u;

    x =
        ((x >> ((x >> 28u) + 4u)) ^ x) * 277803737u;

    x =
        (x >> 22u) ^ x;

    return (float)(x & 0xFFFFFFu) / 16777216.0f;
}


static Color Mezclar(
    Color a,
    Color b,
    float t
)
{
    return Color
    {
        (unsigned char)(a.r + (b.r - a.r) * t),
        (unsigned char)(a.g + (b.g - a.g) * t),
        (unsigned char)(a.b + (b.b - a.b) * t),
        (unsigned char)(a.a + (b.a - a.a) * t)
    };
}


// Entra en la transformacion de un objeto: posicion, escala y giro Y.
static void EntrarObjeto(
    float x,
    float y,
    float z,
    float escala
)
{
    rlPushMatrix();
    rlTranslatef(x, y, z);
    rlScalef(escala, escala, escala);
}


static void SalirObjeto()
{
    rlPopMatrix();
}


//==================================================
// DECORACION
//==================================================

static void DibujarAgua(
    float t
)
{
    DrawPlane(
        { 0.0f, -0.45f, 0.0f },
        { 700.0f, 700.0f },
        { 62, 96, 150, 255 }
    );

    // Destellos que se deslizan sobre el agua.
    for (int i = 0; i < 90; i++)
    {
        float velocidad =
            0.25f + Hash01(i * 7 + 3) * 0.5f;

        float x =
            fmodf(
                Hash01(i * 3 + 1) * 160.0f + t * velocidad,
                160.0f
            ) - 80.0f;

        float z =
            -60.0f + Hash01(i * 5 + 2) * 110.0f;

        float largo =
            1.2f + Hash01(i) * 2.6f;

        float brillo =
            0.15f + 0.20f * (0.5f + 0.5f * sinf(t * 1.3f + i));

        DrawCube(
            { x, -0.40f, z },
            largo,
            0.02f,
            0.10f,
            Fade({ 255, 205, 160, 255 }, brillo)
        );
    }
}


static void DibujarIslasLejanas()
{
    const float datos[][4] =
    {
        { -90.0f, -120.0f, 9.0f, 16.0f },
        { -30.0f, -150.0f, 7.0f, 22.0f },
        {  55.0f, -135.0f, 10.0f, 18.0f },
        { 115.0f, -105.0f, 8.0f, 14.0f },
        { -140.0f, -70.0f, 6.0f, 10.0f }
    };

    for (int i = 0; i < 5; i++)
    {
        DrawCylinderEx(
            { datos[i][0], -0.5f, datos[i][1] },
            { datos[i][0], datos[i][3], datos[i][1] },
            datos[i][2] * 2.2f,
            0.6f,
            10,
            { 96, 82, 140, 255 }
        );
    }
}


static void DibujarNubes(
    float t
)
{
    for (int i = 0; i < 9; i++)
    {
        float x =
            fmodf(
                Hash01(i * 11) * 220.0f + t * (0.5f + Hash01(i) * 0.6f),
                220.0f
            ) - 110.0f;

        float y =
            13.0f + Hash01(i * 13) * 12.0f;

        float z =
            -40.0f - Hash01(i * 17) * 70.0f;

        float escala =
            0.8f + Hash01(i * 19) * 0.8f;

        Color nube =
            { 255, 196, 190, 255 };

        DrawSphere({ x, y, z }, 3.2f * escala, nube);
        DrawSphere({ x + 3.0f * escala, y - 0.5f, z }, 2.5f * escala, nube);
        DrawSphere({ x - 3.0f * escala, y - 0.7f, z + 0.5f }, 2.3f * escala, nube);
        DrawSphere({ x + 0.6f, y + 1.3f * escala, z }, 2.2f * escala, nube);
    }
}


static void DibujarIsla()
{
    Color pasto =
        { 96, 158, 84, 255 };

    Color piedra =
        { 112, 100, 108, 255 };

    Color piedraOscura =
        { 84, 74, 90, 255 };

    // Cuerpo central y extremos redondeados.
    DrawCube({ 0.0f, -0.25f, -1.0f }, 48.0f, 0.5f, 16.0f, pasto);
    DrawCube({ 0.0f, -1.9f, -1.0f }, 48.0f, 2.8f, 16.0f, piedra);

    for (int lado = -1; lado <= 1; lado += 2)
    {
        float x =
            24.0f * lado;

        DrawCylinderEx({ x, -0.5f, -1.0f }, { x, 0.0f, -1.0f }, 8.0f, 8.0f, 24, pasto);
        DrawCylinderEx({ x, -3.3f, -1.0f }, { x, -0.5f, -1.0f }, 8.0f, 8.0f, 24, piedra);
    }

    // Franja de piedra oscura bajo el borde del agua.
    DrawCube({ 0.0f, -0.6f, 7.02f }, 48.0f, 0.25f, 0.06f, piedraOscura);

    // Camino adoquinado frontal.
    DrawCube({ 0.0f, 0.03f, 3.2f }, 46.0f, 0.06f, 2.2f, { 196, 170, 140, 255 });

    for (int i = -15; i <= 15; i++)
    {
        DrawCube(
            { i * 1.5f, 0.07f, 3.2f },
            0.06f,
            0.02f,
            2.2f,
            { 150, 126, 108, 255 }
        );
    }

    // Plazoletas bajo cada objeto.
    for (int i = 0; i < HUB_CANTIDAD_OPCIONES; i++)
    {
        if (i == 1)
        {
            continue;
        }

        Vector3 base =
            BASES_OPCION[i];

        DrawCylinderEx(
            { base.x, 0.0f, base.z },
            { base.x, 0.12f, base.z },
            RADIOS_HALO[i] * 0.88f,
            RADIOS_HALO[i] * 0.88f,
            28,
            { 176, 158, 150, 255 }
        );
    }
}


static void DibujarArbol(
    float x,
    float z,
    float escala,
    int variante,
    float t
)
{
    Color follaje =
        variante % 2 == 0
        ? Color{ 58, 128, 82, 255 }
        : Color{ 84, 146, 70, 255 };

    float balanceo =
        0.05f * sinf(t * 0.8f + x);

    EntrarObjeto(x, 0.0f, z, escala);

    DrawCylinderEx({ 0, 0, 0 }, { 0, 1.8f, 0 }, 0.28f, 0.2f, 8, { 110, 76, 56, 255 });

    rlRotatef(balanceo * 57.0f, 0.0f, 0.0f, 1.0f);

    DrawCylinderEx({ 0, 1.4f, 0 }, { 0, 3.4f, 0 }, 1.5f, 0.2f, 10, follaje);
    DrawCylinderEx({ 0, 2.8f, 0 }, { 0, 4.8f, 0 }, 1.1f, 0.0f, 10, Mezclar(follaje, WHITE, 0.15f));

    SalirObjeto();
}


static void DibujarFarol(
    float x,
    float z,
    float t
)
{
    DrawCylinderEx({ x, 0.0f, z }, { x, 3.6f, z }, 0.12f, 0.09f, 8, { 54, 48, 60, 255 });

    float parpadeo =
        0.85f + 0.15f * sinf(t * 3.0f + x);

    DrawSphere({ x, 3.8f, z }, 0.28f, { 255, 232, 150, 255 });
    DrawSphere({ x, 3.8f, z }, 0.55f * parpadeo, Fade({ 255, 220, 120, 255 }, 0.22f));
}


static void DibujarBanderines(
    float x0,
    float x1,
    float z,
    float t
)
{
    const Color colores[] =
    {
        { 240, 90, 90, 255 },
        { 250, 200, 80, 255 },
        { 90, 190, 230, 255 },
        { 140, 220, 120, 255 }
    };

    int segmentos =
        (int)((x1 - x0));

    for (int i = 0; i <= segmentos; i++)
    {
        float u =
            (float)i / (float)segmentos;

        float x =
            x0 + (x1 - x0) * u;

        float y =
            3.6f - 0.9f * sinf(u * PI);

        float oscilacion =
            0.04f * sinf(t * 2.0f + i);

        DrawCube(
            { x, y - 0.25f, z + oscilacion },
            0.34f,
            0.46f,
            0.04f,
            colores[i % 4]
        );

        if (i < segmentos)
        {
            float uSig =
                (float)(i + 1) / (float)segmentos;

            DrawLine3D(
                { x, y, z },
                {
                    x0 + (x1 - x0) * uSig,
                    3.6f - 0.9f * sinf(uSig * PI),
                    z
                },
                { 60, 50, 60, 255 }
            );
        }
    }
}


static void DibujarFuente(
    float t
)
{
    EntrarObjeto(-1.0f, 0.0f, -1.5f, 1.0f);

    DrawCylinderEx({ 0, 0, 0 }, { 0, 0.55f, 0 }, 1.7f, 1.6f, 20, { 170, 160, 170, 255 });
    DrawCylinderEx({ 0, 0.5f, 0 }, { 0, 0.58f, 0 }, 1.45f, 1.45f, 20, { 90, 190, 220, 255 });
    DrawCylinderEx({ 0, 0.5f, 0 }, { 0, 1.7f, 0 }, 0.22f, 0.18f, 10, { 170, 160, 170, 255 });
    DrawSphere({ 0, 1.8f, 0 }, 0.3f, { 120, 210, 235, 255 });

    for (int i = 0; i < 8; i++)
    {
        float fase =
            fmodf(t * 0.9f + i * 0.125f, 1.0f);

        float angulo =
            i * (PI / 4.0f);

        float radio =
            1.1f * fase;

        float altura =
            1.8f + 1.1f * 4.0f * fase * (1.0f - fase) - 0.6f * fase;

        DrawSphere(
            { cosf(angulo) * radio, altura, sinf(angulo) * radio },
            0.08f,
            { 190, 235, 250, 255 }
        );
    }

    SalirObjeto();
}


static void DibujarDecoracion(
    float t
)
{
    // Arboles del fondo y laterales.
    const float arboles[][4] =
    {
        { -26.0f, -6.0f, 1.3f, 0.0f },
        { -21.0f, -7.0f, 1.0f, 1.0f },
        { -10.5f, -6.5f, 1.2f, 0.0f },
        {  -3.5f, -7.0f, 0.9f, 1.0f },
        {   1.5f, -6.0f, 1.1f, 0.0f },
        {  10.0f, -6.8f, 1.3f, 1.0f },
        {  20.5f, -7.0f, 1.0f, 0.0f },
        {  26.5f, -5.0f, 1.3f, 1.0f },
        {  -4.0f,  3.0f, 0.7f, 0.0f },
        {  10.0f,  2.5f, 0.7f, 1.0f }
    };

    for (int i = 0; i < 10; i++)
    {
        DibujarArbol(
            arboles[i][0],
            arboles[i][1],
            arboles[i][2],
            (int)arboles[i][3],
            t
        );
    }

    // Faroles del frente unidos por banderines.
    const float faroles[] =
    {
        -21.0f, -11.5f, -1.5f, 9.0f, 20.0f
    };

    for (int i = 0; i < 5; i++)
    {
        DibujarFarol(faroles[i], 5.8f, t);

        if (i < 4)
        {
            DibujarBanderines(
                faroles[i],
                faroles[i + 1],
                5.8f,
                t
            );
        }
    }

    DibujarFuente(t);
}


//==================================================
// OPCION 0: DIRIGIBLE
//==================================================

static void DibujarTorreAmarre()
{
    DrawCylinderEx({ -9.3f, 0.0f, -2.5f }, { -9.3f, 2.6f, -2.5f }, 0.7f, 0.5f, 12, { 150, 130, 120, 255 });
    DrawCylinderEx({ -9.3f, 2.6f, -2.5f }, { -9.3f, 2.9f, -2.5f }, 0.9f, 0.9f, 12, { 80, 70, 80, 255 });
    DrawSphere({ -9.3f, 3.1f, -2.5f }, 0.25f, { 240, 200, 90, 255 });
}


static void DibujarDirigible(
    float t,
    float escala
)
{
    Color globo =
        { 226, 88, 70, 255 };

    Color crema =
        { 250, 226, 180, 255 };

    float altura =
        5.0f + 0.25f * sinf(t * 0.9f);

    EntrarObjeto(-15.0f, altura, -1.0f, escala);

    rlRotatef(2.5f * sinf(t * 0.7f), 0.0f, 0.0f, 1.0f);

    // Globo elipsoidal.
    rlPushMatrix();
    rlScalef(4.4f, 1.7f, 1.7f);
    DrawSphereEx({ 0, 0, 0 }, 1.0f, 18, 24, globo);
    rlPopMatrix();

    // Franjas crema.
    const float posicionesFranja[] =
    {
        -2.4f, 0.0f, 2.4f
    };

    for (int i = 0; i < 3; i++)
    {
        float x =
            posicionesFranja[i];

        float r =
            1.7f * sqrtf(1.0f - (x / 4.4f) * (x / 4.4f)) + 0.03f;

        DrawCylinderEx({ x - 0.3f, 0, 0 }, { x + 0.3f, 0, 0 }, r, r, 20, crema);
    }

    // Gondola.
    DrawCube({ 0.0f, -2.25f, 0.0f }, 2.4f, 0.75f, 1.0f, { 130, 90, 62, 255 });
    DrawCube({ 0.0f, -1.8f, 0.0f }, 2.6f, 0.12f, 1.2f, { 90, 62, 46, 255 });

    for (int i = -1; i <= 1; i++)
    {
        DrawCube({ i * 0.7f, -2.2f, 0.52f }, 0.34f, 0.3f, 0.04f, { 255, 236, 150, 255 });
    }

    // Cuerdas globo-gondola.
    for (int sx = -1; sx <= 1; sx += 2)
    {
        for (int sz = -1; sz <= 1; sz += 2)
        {
            DrawLine3D(
                { sx * 1.0f, -1.5f, sz * 0.4f },
                { sx * 1.1f, -1.9f, sz * 0.45f },
                { 70, 56, 50, 255 }
            );
        }
    }

    // Aletas de cola.
    DrawCube({ -4.0f, 1.0f, 0.0f }, 1.0f, 1.5f, 0.1f, globo);
    DrawCube({ -4.0f, -1.0f, 0.0f }, 1.0f, 1.5f, 0.1f, globo);
    DrawCube({ -4.0f, 0.0f, 1.0f }, 1.0f, 0.1f, 1.5f, crema);
    DrawCube({ -4.0f, 0.0f, -1.0f }, 1.0f, 0.1f, 1.5f, crema);

    // Helice trasera.
    rlPushMatrix();
    rlTranslatef(-4.9f, -0.2f, 0.0f);
    rlRotatef(fmodf(t * 540.0f, 360.0f), 1.0f, 0.0f, 0.0f);
    DrawCube({ 0, 0, 0 }, 0.08f, 2.0f, 0.22f, { 60, 54, 66, 255 });
    DrawCube({ 0, 0, 0 }, 0.08f, 0.22f, 2.0f, { 60, 54, 66, 255 });
    rlPopMatrix();
    DrawSphere({ -4.8f, -0.2f, 0.0f }, 0.2f, { 250, 200, 90, 255 });

    SalirObjeto();

    // Cuerdas de amarre del morro a la torre.
    float desplazamiento =
        0.25f * sinf(t * 0.9f);

    DrawLine3D(
        { -10.7f, 5.0f + desplazamiento, -1.0f },
        { -9.3f, 3.0f, -2.5f },
        { 230, 210, 170, 255 }
    );

    DrawLine3D(
        { -10.8f, 4.7f + desplazamiento, -0.7f },
        { -9.4f, 3.0f, -2.4f },
        { 230, 210, 170, 255 }
    );
}


//==================================================
// OPCION 1: BARCO
//==================================================

static void DibujarEmbarcadero()
{
    Color madera =
        { 150, 108, 74, 255 };

    DrawCube({ -4.2f, -0.12f, 10.0f }, 2.2f, 0.22f, 7.0f, madera);

    for (int i = 0; i < 14; i++)
    {
        DrawCube(
            { -4.2f, -0.0f, 6.7f + i * 0.5f },
            2.2f,
            0.03f,
            0.04f,
            { 96, 66, 48, 255 }
        );
    }

    for (int i = 0; i < 4; i++)
    {
        float z =
            7.2f + i * 2.0f;

        DrawCylinderEx({ -5.2f, -1.2f, z }, { -5.2f, 0.7f, z }, 0.12f, 0.12f, 8, { 96, 66, 48, 255 });
        DrawCylinderEx({ -3.2f, -1.2f, z }, { -3.2f, 0.7f, z }, 0.12f, 0.12f, 8, { 96, 66, 48, 255 });
    }
}


static void DibujarBarco(
    float t,
    float escala
)
{
    Color casco =
        { 138, 84, 58, 255 };

    float cabeceo =
        -0.30f + 0.08f * sinf(t * 1.4f);

    EntrarObjeto(-7.0f, cabeceo, 11.5f, escala);

    rlRotatef(3.5f * sinf(t * 1.1f), 1.0f, 0.0f, 0.0f);
    rlRotatef(2.0f * sinf(t * 0.8f), 0.0f, 0.0f, 1.0f);

    // Casco con proa en punta.
    DrawCube({ 0.0f, 0.15f, 0.0f }, 3.4f, 0.7f, 1.5f, casco);

    rlPushMatrix();
    rlTranslatef(1.7f, 0.15f, 0.0f);
    rlRotatef(45.0f, 0.0f, 1.0f, 0.0f);
    DrawCube({ 0, 0, 0 }, 1.06f, 0.7f, 1.06f, casco);
    rlPopMatrix();

    DrawCube({ 0.0f, 0.5f, 0.0f }, 3.5f, 0.1f, 1.6f, { 230, 80, 70, 255 });
    DrawCube({ 0.0f, 0.57f, 0.0f }, 3.2f, 0.06f, 1.3f, { 200, 164, 120, 255 });

    // Cabina.
    DrawCube({ -1.0f, 1.1f, 0.0f }, 1.1f, 1.0f, 1.0f, { 244, 236, 220, 255 });
    DrawCube({ -1.0f, 1.7f, 0.0f }, 1.3f, 0.16f, 1.2f, { 90, 130, 190, 255 });
    DrawCube({ -0.44f, 1.15f, 0.0f }, 0.04f, 0.34f, 0.5f, { 255, 236, 150, 255 });

    // Mastil y vela.
    DrawCylinderEx({ 0.5f, 0.6f, 0.0f }, { 0.5f, 4.2f, 0.0f }, 0.1f, 0.07f, 8, { 100, 70, 52, 255 });

    Vector3 a = { 0.5f, 4.0f, 0.0f };
    Vector3 b = { 0.5f, 1.2f, 0.0f };
    Vector3 c = { 2.4f, 1.3f, 0.0f };

    DrawTriangle3D(a, b, c, { 250, 240, 220, 255 });
    DrawTriangle3D(a, c, b, { 250, 240, 220, 255 });

    // Banderin.
    Vector3 p1 = { 0.5f, 4.2f, 0.0f };
    Vector3 p2 = { 0.5f, 3.8f, 0.0f };
    Vector3 p3 = { -0.4f, 4.0f + 0.1f * sinf(t * 5.0f), 0.0f };

    DrawTriangle3D(p1, p2, p3, { 240, 90, 90, 255 });
    DrawTriangle3D(p1, p3, p2, { 240, 90, 90, 255 });

    SalirObjeto();

    // Espuma alrededor del casco.
    for (int i = 0; i < 2; i++)
    {
        float fase =
            fmodf(t * 0.4f + i * 0.5f, 1.0f);

        DrawCircle3D(
            { -7.0f, -0.40f, 11.5f },
            2.0f + fase * 1.6f,
            { 1.0f, 0.0f, 0.0f },
            90.0f,
            Fade(WHITE, 0.35f * (1.0f - fase))
        );
    }
}


//==================================================
// OPCION 2: TORRE DE ENGRANAJES
//==================================================

static void DibujarEngranaje(
    float x,
    float y,
    float z,
    float radio,
    float angulo,
    Color color
)
{
    int dientes =
        (int)(radio * 6.0f);

    rlPushMatrix();
    rlTranslatef(x, y, z);
    rlRotatef(angulo, 0.0f, 0.0f, 1.0f);

    DrawCylinderEx({ 0, 0, -0.2f }, { 0, 0, 0.2f }, radio, radio, 20, color);
    DrawCylinderEx({ 0, 0, 0.1f }, { 0, 0, 0.23f }, radio * 0.45f, radio * 0.45f, 16, Mezclar(color, BLACK, 0.45f));
    DrawCylinderEx({ 0, 0, 0.0f }, { 0, 0, 0.3f }, radio * 0.12f, radio * 0.12f, 8, { 240, 210, 120, 255 });

    for (int k = 0; k < dientes; k++)
    {
        rlPushMatrix();
        rlRotatef(k * 360.0f / dientes, 0.0f, 0.0f, 1.0f);
        DrawCube({ radio + 0.12f, 0.0f, 0.0f }, 0.34f, 0.28f, 0.4f, color);
        rlPopMatrix();
    }

    rlPopMatrix();
}


static void DibujarTorreEngranajes(
    float t,
    float escala,
    float velocidad
)
{
    Color piedra =
        { 140, 126, 134, 255 };

    Color cobre =
        { 192, 120, 74, 255 };

    EntrarObjeto(5.0f, 0.0f, -1.0f, escala);

    DrawCylinderEx({ 0, 0, 0 }, { 0, 1.0f, 0 }, 2.5f, 2.3f, 16, { 110, 98, 108, 255 });
    DrawCylinderEx({ 0, 1.0f, 0 }, { 0, 6.8f, 0 }, 1.9f, 1.4f, 16, piedra);
    DrawCylinderEx({ 0, 6.8f, 0 }, { 0, 7.2f, 0 }, 1.8f, 1.8f, 16, { 96, 84, 94, 255 });
    DrawCylinderEx({ 0, 7.2f, 0 }, { 0, 9.4f, 0 }, 1.9f, 0.0f, 16, cobre);
    DrawSphere({ 0, 9.6f, 0 }, 0.22f, { 250, 214, 100, 255 });

    // Ventana luminosa.
    DrawCube({ 0.0f, 2.2f, 1.75f }, 0.7f, 1.1f, 0.12f, { 255, 220, 130, 255 });

    float base =
        t * 22.0f * velocidad;

    // Engranajes engranados: giran en sentidos opuestos.
    DibujarEngranaje(0.0f, 4.1f, 1.7f, 1.5f, base, cobre);
    DibujarEngranaje(2.2f, 5.9f, 1.55f, 0.9f, -base * 1.65f, { 214, 170, 90, 255 });
    DibujarEngranaje(-2.0f, 5.5f, 1.55f, 1.1f, -base * 1.35f + 8.0f, { 170, 110, 84, 255 });
    DibujarEngranaje(-1.6f, 2.6f, 1.55f, 0.7f, base * 2.1f, { 214, 170, 90, 255 });

    SalirObjeto();
}


//==================================================
// OPCION 3: ARCO DE SALIDA
//==================================================

static void DibujarArcoSalida(
    float t,
    float escala
)
{
    Color piedra =
        { 150, 138, 146, 255 };

    EntrarObjeto(15.0f, 0.0f, -1.0f, escala);

    DrawCube({ 0, 0.15f, 1.4f }, 7.0f, 0.3f, 1.2f, { 120, 108, 118, 255 });
    DrawCube({ 0, 0.4f, 1.0f }, 6.4f, 0.3f, 1.2f, { 130, 118, 126, 255 });

    DrawCube({ -2.2f, 3.2f, 0.0f }, 1.4f, 6.0f, 1.6f, piedra);
    DrawCube({ 2.2f, 3.2f, 0.0f }, 1.4f, 6.0f, 1.6f, piedra);
    DrawCube({ 0.0f, 6.5f, 0.0f }, 6.6f, 1.3f, 1.8f, { 164, 152, 160, 255 });
    DrawCube({ 0.0f, 7.4f, 0.0f }, 1.2f, 0.7f, 1.4f, { 214, 170, 90, 255 });

    // Portal con brillo pulsante.
    float pulso =
        0.5f + 0.5f * sinf(t * 2.0f);

    DrawCube({ 0, 2.9f, 0.0f }, 3.0f, 5.8f, 0.2f, { 28, 22, 54, 255 });
    DrawCube({ 0, 2.9f, 0.13f }, 2.8f, 5.6f, 0.04f, Fade({ 150, 100, 240, 255 }, 0.45f + 0.25f * pulso));
    DrawCube({ 0, 2.9f, 0.18f }, 1.7f, 4.2f, 0.04f, Fade({ 210, 170, 255, 255 }, 0.25f + 0.2f * pulso));

    // Farolillos colgantes.
    for (int lado = -1; lado <= 1; lado += 2)
    {
        float oscilacion =
            0.1f * sinf(t * 1.6f + lado);

        DrawLine3D(
            { lado * 2.2f, 5.8f, 0.9f },
            { lado * 2.2f + oscilacion, 5.0f, 0.9f },
            { 60, 50, 60, 255 }
        );

        DrawSphere({ lado * 2.2f + oscilacion, 4.8f, 0.9f }, 0.28f, { 255, 190, 110, 255 });
    }

    SalirObjeto();
}


//==================================================
// RESALTADO DE LA OPCION ELEGIDA
//==================================================

static void DibujarResaltado(
    int opcion,
    float t
)
{
    Vector3 base =
        BASES_OPCION[opcion];

    float radio =
        RADIOS_HALO[opcion];

    float pulso =
        0.5f + 0.5f * sinf(t * 3.2f);

    float y =
        base.y + 0.16f;

    // Anillos de suelo.
    for (int i = 0; i < 3; i++)
    {
        DrawCircle3D(
            { base.x, y, base.z },
            radio + i * 0.07f,
            { 1.0f, 0.0f, 0.0f },
            90.0f,
            Fade(COLOR_HALO, 0.65f + 0.3f * pulso)
        );
    }

    // Onda que se expande.
    float fase =
        fmodf(t * 0.7f, 1.0f);

    DrawCircle3D(
        { base.x, y + 0.02f, base.z },
        radio * (0.6f + 0.7f * fase),
        { 1.0f, 0.0f, 0.0f },
        90.0f,
        Fade(WHITE, 0.6f * (1.0f - fase))
    );

    // Haz de luz translucido.
    DrawCylinderEx(
        { base.x, base.y, base.z },
        { base.x, base.y + 15.0f, base.z },
        radio * 0.85f,
        radio * 0.55f,
        24,
        Fade(COLOR_HALO, 0.07f + 0.04f * pulso)
    );
}


//==================================================
// ESCENA COMPLETA
//==================================================

static void DibujarEscena(
    float t,
    int resaltada
)
{
    DibujarAgua(t);
    DibujarIslasLejanas();
    DibujarNubes(t);
    DibujarIsla();
    DibujarDecoracion(t);

    float pulso =
        0.04f * sinf(t * 4.0f) + 0.03f;

    float e0 =
        resaltada == 0 ? 1.0f + pulso : 1.0f;

    float e1 =
        resaltada == 1 ? 1.0f + pulso : 1.0f;

    float e2 =
        resaltada == 2 ? 1.0f + pulso : 1.0f;

    float e3 =
        resaltada == 3 ? 1.0f + pulso : 1.0f;

    DibujarTorreAmarre();
    DibujarDirigible(t, e0);

    DibujarEmbarcadero();
    DibujarBarco(t, e1);

    DibujarTorreEngranajes(
        t,
        e2,
        resaltada == 2 ? 2.6f : 1.0f
    );

    DibujarArcoSalida(t, e3);

    // Lo translucido va al final.
    if (resaltada >= 0 && resaltada < HUB_CANTIDAD_OPCIONES)
    {
        DibujarResaltado(resaltada, t);
    }
}


//==================================================
// CAMARA
//==================================================

static void ObtenerCamaraAmbiental(
    float tiempoAmbiental,
    Vector3& posicion,
    Vector3& objetivo
)
{
    // Ping-pong con easing coseno: velocidad cero en los extremos.
    float fase =
        fmodf(tiempoAmbiental / DURACION_RECORRIDO, 2.0f);

    float s =
        fase < 1.0f
        ? fase
        : 2.0f - fase;

    float u =
        0.5f - 0.5f * cosf(PI * s);

    posicion =
    {
        CAMARA_X_MIN + (CAMARA_X_MAX - CAMARA_X_MIN) * u,
        7.0f + 0.5f * sinf(tiempoAmbiental * 0.23f),
        26.0f
    };

    objetivo =
    {
        -14.0f + 28.0f * u,
        4.0f,
        0.0f
    };
}


static void ObtenerCamaraEnfocada(
    Vector3 ancla,
    Vector3& posicion,
    Vector3& objetivo
)
{
    posicion =
    {
        ancla.x - ancla.x * 0.12f,
        ancla.y + 2.8f,
        ancla.z + 17.0f
    };

    objetivo =
    {
        ancla.x,
        ancla.y - 1.2f,
        ancla.z
    };
}


//==================================================
// INICIALIZAR
//==================================================

void Hub3D::Inicializar()
{
    tiempo =
        0.0f;

    tiempoAmbiental =
        0.0f;

    enfoque =
        0.0f;

    zoomConfirmacion =
        0.0f;

    anclaFoco =
        ANCLAS_OPCION[0];

    camaraInicializada =
        false;

    for (int i = 0; i < HUB_CANTIDAD_OPCIONES; i++)
    {
        posicionPantallaOpcion[i] =
            { 0.0f, 0.0f };

        opcionVisible[i] =
            false;
    }
}


//==================================================
// ACTUALIZAR
//==================================================

void Hub3D::Actualizar(
    float deltaTime,
    int opcionEnfocada,
    bool enfocar,
    float zoomObjetivo
)
{
    if (deltaTime > 0.1f)
    {
        deltaTime =
            0.1f;
    }

    tiempo +=
        deltaTime;

    // Enfoque lento al elegir; regreso al recorrido todavia mas lento.
    enfoque =
        MoverHacia(
            enfoque,
            enfocar ? 1.0f : 0.0f,
            deltaTime * (enfocar ? 0.5f : 0.2f)
        );

    zoomConfirmacion =
        MoverHacia(
            zoomConfirmacion,
            zoomObjetivo,
            deltaTime * 2.0f
        );

    // El recorrido se detiene mientras la camara esta enfocada,
    // asi al volver continua desde el mismo punto.
    tiempoAmbiental +=
        deltaTime * (1.0f - Suavizar(enfoque));

    if (
        opcionEnfocada >= 0 &&
        opcionEnfocada < HUB_CANTIDAD_OPCIONES
    )
    {
        Vector3 objetivoAncla =
            ANCLAS_OPCION[opcionEnfocada];

        float k =
            1.0f - expf(-3.0f * deltaTime);

        anclaFoco =
            Vector3Lerp(anclaFoco, objetivoAncla, k);
    }

    Vector3 posicionAmbiental;
    Vector3 objetivoAmbiental;

    ObtenerCamaraAmbiental(
        tiempoAmbiental,
        posicionAmbiental,
        objetivoAmbiental
    );

    Vector3 posicionFoco;
    Vector3 objetivoFoco;

    ObtenerCamaraEnfocada(
        anclaFoco,
        posicionFoco,
        objetivoFoco
    );

    float mezcla =
        Suavizar(enfoque);

    Vector3 posicionDeseada =
        Vector3Lerp(posicionAmbiental, posicionFoco, mezcla);

    Vector3 objetivoDeseado =
        Vector3Lerp(objetivoAmbiental, objetivoFoco, mezcla);

    // Acercamiento de confirmacion: la camara avanza hacia el objetivo.
    posicionDeseada =
        Vector3Lerp(
            posicionDeseada,
            objetivoDeseado,
            0.38f * Suavizar(zoomConfirmacion)
        );

    if (!camaraInicializada)
    {
        camaraPosicion =
            posicionDeseada;

        camaraObjetivo =
            objetivoDeseado;

        camaraInicializada =
            true;

        return;
    }

    // Suavizado exponencial extra: nunca hay saltos aunque cambie la opcion.
    float suavizado =
        1.0f - expf(-4.0f * deltaTime);

    camaraPosicion =
        Vector3Lerp(camaraPosicion, posicionDeseada, suavizado);

    camaraObjetivo =
        Vector3Lerp(camaraObjetivo, objetivoDeseado, suavizado);
}


//==================================================
// DIBUJAR
//==================================================

void Hub3D::Dibujar(
    int opcionResaltada
)
{
    Camera3D camara = {};

    camara.position =
        camaraPosicion;

    camara.target =
        camaraObjetivo;

    camara.up =
        { 0.0f, 1.0f, 0.0f };

    camara.fovy =
        45.0f;

    camara.projection =
        CAMERA_PERSPECTIVE;

    int ancho =
        GetScreenWidth();

    int alto =
        GetScreenHeight();

    //------------------------------
    // LIMPIAR COLOR Y PROFUNDIDAD
    //------------------------------
    // Las pantallas de menu no llaman ClearBackground: sin esto el depth
    // buffer conserva los valores del cuadro anterior y la escena sale
    // recortada y con poligonos gigantes. ClearBackground limpia color y
    // profundidad, asi el HUB no depende del estado de pantallas previas.

    ClearBackground({ 255, 178, 118, 255 });

    //------------------------------
    // CIELO (2D, detras de todo)
    //------------------------------

    Vector3 adelante =
        Vector3Subtract(camaraObjetivo, camaraPosicion);

    adelante.y =
        0.0f;

    adelante =
        Vector3Normalize(adelante);

    Vector2 horizontePantalla =
        GetWorldToScreen(
            Vector3Add(
                { camaraPosicion.x, -0.45f, camaraPosicion.z },
                Vector3Scale(adelante, 600.0f)
            ),
            camara
        );

    int horizonte =
        (int)horizontePantalla.y;

    if (horizonte < 0)
    {
        horizonte =
            0;
    }

    if (horizonte > alto)
    {
        horizonte =
            alto;
    }

    Color cieloAlto =
        { 44, 40, 108, 255 };

    Color cieloMedio =
        { 214, 108, 124, 255 };

    Color cieloBajo =
        { 255, 178, 118, 255 };

    int mitad =
        horizonte / 2;

    DrawRectangleGradientV(0, 0, ancho, mitad + 1, cieloAlto, cieloMedio);
    DrawRectangleGradientV(0, mitad, ancho, horizonte - mitad + 1, cieloMedio, cieloBajo);

    if (horizonte < alto)
    {
        DrawRectangle(0, horizonte, ancho, alto - horizonte, cieloBajo);
    }

    int solX =
        (int)(ancho * 0.72f - camaraPosicion.x * ancho * 0.004f);

    int solY =
        horizonte - (int)(alto * 0.10f);

    DrawCircleGradient({ (float)solX, (float)solY }, alto * 0.28f, Fade({ 255, 224, 150, 255 }, 0.55f), Fade({ 255, 190, 130, 255 }, 0.0f));
    DrawCircle(solX, solY, alto * 0.055f, { 255, 244, 205, 255 });

    //------------------------------
    // ESCENA 3D
    //------------------------------

    BeginMode3D(camara);

    DibujarEscena(tiempo, opcionResaltada);

    EndMode3D();

    //------------------------------
    // POSICIONES DE CARTELES
    //------------------------------

    Vector3 mirada =
        Vector3Subtract(camaraObjetivo, camaraPosicion);

    for (int i = 0; i < HUB_CANTIDAD_OPCIONES; i++)
    {
        Vector3 punto =
            {
                BASES_OPCION[i].x,
                BASES_OPCION[i].y + ALTURAS_CARTEL[i],
                BASES_OPCION[i].z
            };

        Vector3 haciaPunto =
            Vector3Subtract(punto, camaraPosicion);

        opcionVisible[i] =
            Vector3DotProduct(haciaPunto, mirada) > 0.0f;

        posicionPantallaOpcion[i] =
            GetWorldToScreen(punto, camara);
    }
}


//==================================================
// DESCARGAR
//==================================================

void Hub3D::Descargar()
{
    // El HUB solo usa primitivas: no hay recursos que liberar.
    // Con modelos GLB, descargarlos aqui.
    camaraInicializada =
        false;
}
