#include "Minigames/MinijuegoColorSeguro.h"
#include "Minigames/AudioMinijuegos.h"
#include "Minigames/BotsMinijuegos1v3.h"
#include "Minigames/CombateMinijuegos.h"
#include "Minigames/HudMinijuego.h"
#include "Systems/CalidadGrafica.h"
#include "UI/EstiloUI.h"

#include "rlgl.h"

#include <cmath>
#include <cstdio>


static const float DURACION_PREPARACION_COLOR = 3.0f;
static const float RADIO_HEXAGONO_COLOR = 2.35f;

// Color Seguro ya utilizaba un aplastamiento mas corto que el perfil
// estandar. Se conserva de forma explicita al centralizar el impacto.
static const float DURACION_APLASTADO_GROUND_POUND_COLOR = 1.15f;

// Escenario volcanico (solo visual).
static const float NIVEL_LAVA_COLOR = -2.20f;
static const float DURACION_EMERGER_COLOR = 0.45f;
static const float CENTRO_CRATER_Z_COLOR = -3.0f;
static const float RADIO_CRATER_X_COLOR = 17.0f;
static const float RADIO_CRATER_Z_COLOR = 15.0f;
static const Vector3 CENTRO_TV_COLOR = { 0.0f, 2.9f, -9.6f };
static const float ANCHO_TV_COLOR = 6.0f;
static const float ALTO_TV_COLOR = 3.4f;
static const float FONDO_TV_COLOR = 0.7f;
static const Vector3 OJO_NIEBLA_COLOR = { 0.0f, 12.0f, 15.0f };


//==================================================
// COLORES Y SIMBOLOS DE LAS PLATAFORMAS
//==================================================
//
// Tonos bien separados entre si (el rojo es carmin oscuro y el rosa es
// claro; el violeta es purpura y el celeste es cian). Ademas cada color
// lleva un simbolo propio, el mismo en la plataforma y en la TV.

static Color ObtenerColorPlataforma(int indice)
{
    switch (indice)
    {
        case 0: return Color{ 238, 236, 228, 255 };   // blanco
        case 1: return Color{ 250, 206, 40, 255 };    // amarillo
        case 2: return Color{ 200, 30, 42, 255 };     // rojo
        case 3: return Color{ 56, 178, 76, 255 };     // verde
        case 4: return Color{ 255, 146, 212, 255 };   // rosa
        case 5: return Color{ 134, 66, 214, 255 };    // violeta
        case 6: return Color{ 84, 210, 244, 255 };    // celeste
    }

    return Color{ 238, 236, 228, 255 };
}


static const char* NombreColorPlataforma(int indice)
{
    switch (indice)
    {
        case 0: return "BLANCO";
        case 1: return "AMARILLO";
        case 2: return "ROJO";
        case 3: return "VERDE";
        case 4: return "ROSA";
        case 5: return "VIOLETA";
        case 6: return "CELESTE";
    }

    return "?";
}


//==================================================
// AYUDAS VISUALES
//==================================================

static float Limitar01ColorSeguro(float v)
{
    return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
}


static float SuavizarColorSeguro(float u)
{
    u = Limitar01ColorSeguro(u);
    return u * u * (3.0f - 2.0f * u);
}


static float EaseOutBackColorSeguro(float u)
{
    u = Limitar01ColorSeguro(u);
    const float c1 = 1.70158f;
    const float c3 = c1 + 1.0f;
    float k = u - 1.0f;
    return 1.0f + c3 * k * k * k + c1 * k * k;
}


static Color TinteColorSeguro(Color c, float f)
{
    float r = (float)c.r * f, g = (float)c.g * f, b = (float)c.b * f;
    return Color
    {
        (unsigned char)(r > 255.0f ? 255.0f : r),
        (unsigned char)(g > 255.0f ? 255.0f : g),
        (unsigned char)(b > 255.0f ? 255.0f : b),
        c.a
    };
}


static Color MezclarColorSeguro(Color a, Color b, float t)
{
    t = Limitar01ColorSeguro(t);
    return Color
    {
        (unsigned char)((float)a.r + ((float)b.r - (float)a.r) * t),
        (unsigned char)((float)a.g + ((float)b.g - (float)a.g) * t),
        (unsigned char)((float)a.b + ((float)b.b - (float)a.b) * t),
        (unsigned char)((float)a.a + ((float)b.a - (float)a.a) * t)
    };
}


static Color AlfaColorSeguro(Color c, float alfa)
{
    c.a = (unsigned char)(255.0f * Limitar01ColorSeguro(alfa));
    return c;
}


static float Hash01ColorSeguro(int a, int b)
{
    unsigned int h = (unsigned int)a * 374761393u + (unsigned int)b * 668265263u + 1013904223u;
    h = (h ^ (h >> 13)) * 1274126177u;
    h ^= h >> 16;
    return (float)(h & 0xFFFFu) / 65535.0f;
}


// Niebla de profundidad: lo lejano se funde con el humo rojizo del crater.
static Color NieblaColorSeguro(Color c, Vector3 p)
{
    float dx = p.x - OJO_NIEBLA_COLOR.x;
    float dy = p.y - OJO_NIEBLA_COLOR.y;
    float dz = p.z - OJO_NIEBLA_COLOR.z;
    float d = std::sqrt(dx * dx + dy * dy + dz * dz);
    float f = Limitar01ColorSeguro((d - 19.0f) / 26.0f) * 0.62f;
    return MezclarColorSeguro(c, Color{ 74, 30, 26, c.a }, f);
}


static void VerticeColorSeguro(Vector3 p, Color c)
{
    rlColor4ub(c.r, c.g, c.b, c.a);
    rlVertex3f(p.x, p.y, p.z);
}


// Luz falsa de las caras laterales segun hacia donde mira su normal.
static float LuzCaraColorSeguro(float anguloNormalGrados)
{
    float a = anguloNormalGrados * DEG2RAD;
    float d = std::cos(a) * -0.41f + std::sin(a) * 0.91f;
    return 0.50f + 0.44f * (d > 0.0f ? d : 0.0f);
}


// Prisma hexagonal con caras sombreadas (columnas de basalto, plataformas).
// El color de abajo permite el reflejo caliente de la lava.
static void PrismaHexColorSeguro(
    Vector3 base,
    float radioAbajo,
    float radioArriba,
    float yAbajo,
    float yArriba,
    float giro,
    Color colorArriba,
    Color colorAbajo,
    bool tapa,
    Color colorTapa
)
{
    rlCheckRenderBatchLimit(6 * 9);
    rlBegin(RL_TRIANGLES);

    Vector3 centroTapa = { base.x, base.y + yArriba, base.z };

    for (int k = 0; k < 6; k++)
    {
        float a0 = (giro + 30.0f + 60.0f * (float)k) * DEG2RAD;
        float a1 = (giro + 90.0f + 60.0f * (float)k) * DEG2RAD;
        float luz = LuzCaraColorSeguro(giro + 60.0f + 60.0f * (float)k);

        Vector3 A0 = { base.x + std::cos(a0) * radioArriba, base.y + yArriba, base.z + std::sin(a0) * radioArriba };
        Vector3 A1 = { base.x + std::cos(a1) * radioArriba, base.y + yArriba, base.z + std::sin(a1) * radioArriba };
        Vector3 B0 = { base.x + std::cos(a0) * radioAbajo, base.y + yAbajo, base.z + std::sin(a0) * radioAbajo };
        Vector3 B1 = { base.x + std::cos(a1) * radioAbajo, base.y + yAbajo, base.z + std::sin(a1) * radioAbajo };

        Color ca = TinteColorSeguro(colorArriba, luz);
        Color cb = TinteColorSeguro(colorAbajo, luz);

        VerticeColorSeguro(A0, ca); VerticeColorSeguro(A1, ca); VerticeColorSeguro(B1, cb);
        VerticeColorSeguro(A0, ca); VerticeColorSeguro(B1, cb); VerticeColorSeguro(B0, cb);

        if (tapa)
        {
            VerticeColorSeguro(centroTapa, colorTapa);
            VerticeColorSeguro(A1, colorTapa);
            VerticeColorSeguro(A0, colorTapa);
        }
    }

    rlEnd();
}


// Caja con caras sombreadas (estructura de la TV, rocas).
static void CajaFacetadaColorSeguro(Vector3 c, Vector3 t, Color color)
{
    float x0 = c.x - t.x * 0.5f, x1 = c.x + t.x * 0.5f;
    float y0 = c.y - t.y * 0.5f, y1 = c.y + t.y * 0.5f;
    float z0 = c.z - t.z * 0.5f, z1 = c.z + t.z * 0.5f;

    Color tapa = TinteColorSeguro(color, 1.25f);
    Color frente = color;
    Color atras = TinteColorSeguro(color, 0.58f);
    Color der = TinteColorSeguro(color, 0.66f);
    Color izq = TinteColorSeguro(color, 0.80f);
    Color abajo = TinteColorSeguro(color, 0.45f);

    DrawTriangle3D({ x0, y1, z0 }, { x0, y1, z1 }, { x1, y1, z1 }, tapa);
    DrawTriangle3D({ x0, y1, z0 }, { x1, y1, z1 }, { x1, y1, z0 }, tapa);
    DrawTriangle3D({ x0, y0, z1 }, { x1, y0, z1 }, { x1, y1, z1 }, frente);
    DrawTriangle3D({ x0, y0, z1 }, { x1, y1, z1 }, { x0, y1, z1 }, frente);
    DrawTriangle3D({ x0, y0, z0 }, { x0, y1, z0 }, { x1, y1, z0 }, atras);
    DrawTriangle3D({ x0, y0, z0 }, { x1, y1, z0 }, { x1, y0, z0 }, atras);
    DrawTriangle3D({ x1, y0, z0 }, { x1, y1, z0 }, { x1, y1, z1 }, der);
    DrawTriangle3D({ x1, y0, z0 }, { x1, y1, z1 }, { x1, y0, z1 }, der);
    DrawTriangle3D({ x0, y0, z0 }, { x0, y0, z1 }, { x0, y1, z1 }, izq);
    DrawTriangle3D({ x0, y0, z0 }, { x0, y1, z1 }, { x0, y1, z0 }, izq);
    DrawTriangle3D({ x0, y0, z0 }, { x1, y0, z0 }, { x1, y0, z1 }, abajo);
    DrawTriangle3D({ x0, y0, z0 }, { x1, y0, z1 }, { x0, y0, z1 }, abajo);
}


static void RocaGiradaColorSeguro(Vector3 c, Vector3 t, float angulo, Color color)
{
    rlPushMatrix();
    rlTranslatef(c.x, c.y, c.z);
    rlRotatef(angulo, 0.0f, 1.0f, 0.0f);
    rlRotatef(angulo * 0.21f, 1.0f, 0.0f, 0.0f);
    CajaFacetadaColorSeguro({ 0.0f, 0.0f, 0.0f }, t, color);
    rlPopMatrix();
}


// Disco de luz suave (centro opaco, borde transparente) en el plano u-v.
// Solo con mezcla aditiva y sin escribir profundidad.
static void ResplandorColorSeguro(Vector3 centro, Vector3 u, Vector3 v, float radio, Color color)
{
    const int LADOS = 16;
    rlCheckRenderBatchLimit(LADOS * 3);
    rlBegin(RL_TRIANGLES);

    for (int k = 0; k < LADOS; k++)
    {
        float a0 = 6.2832f * (float)k / (float)LADOS;
        float a1 = 6.2832f * (float)(k + 1) / (float)LADOS;
        float c0 = std::cos(a0) * radio, s0 = std::sin(a0) * radio;
        float c1 = std::cos(a1) * radio, s1 = std::sin(a1) * radio;

        rlColor4ub(color.r, color.g, color.b, color.a);
        rlVertex3f(centro.x, centro.y, centro.z);
        rlColor4ub(color.r, color.g, color.b, 0);
        rlVertex3f(centro.x + u.x * c0 + v.x * s0, centro.y + u.y * c0 + v.y * s0, centro.z + u.z * c0 + v.z * s0);
        rlColor4ub(color.r, color.g, color.b, 0);
        rlVertex3f(centro.x + u.x * c1 + v.x * s1, centro.y + u.y * c1 + v.y * s1, centro.z + u.z * c1 + v.z * s1);
    }

    rlEnd();
}


static void ResplandorSueloColorSeguro(float x, float y, float z, float radio, Color color)
{
    ResplandorColorSeguro({ x, y, z }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, radio, color);
}


// Anillo horizontal relleno (ondas de lava, eliminaciones).
static void AnilloColorSeguro(Vector3 centro, float radioInterior, float radioExterior, Color color)
{
    const int LADOS = 28;
    rlCheckRenderBatchLimit(LADOS * 6);
    rlBegin(RL_TRIANGLES);

    for (int k = 0; k < LADOS; k++)
    {
        float a0 = 6.2832f * (float)k / (float)LADOS;
        float a1 = 6.2832f * (float)(k + 1) / (float)LADOS;
        Vector3 i0 = { centro.x + std::cos(a0) * radioInterior, centro.y, centro.z + std::sin(a0) * radioInterior };
        Vector3 i1 = { centro.x + std::cos(a1) * radioInterior, centro.y, centro.z + std::sin(a1) * radioInterior };
        Vector3 e0 = { centro.x + std::cos(a0) * radioExterior, centro.y, centro.z + std::sin(a0) * radioExterior };
        Vector3 e1 = { centro.x + std::cos(a1) * radioExterior, centro.y, centro.z + std::sin(a1) * radioExterior };

        VerticeColorSeguro(i0, color); VerticeColorSeguro(e0, color); VerticeColorSeguro(e1, color);
        VerticeColorSeguro(i0, color); VerticeColorSeguro(e1, color); VerticeColorSeguro(i1, color);
    }

    rlEnd();
}


static Vector3 PuntoSimboloColorSeguro(Vector3 c, Vector3 u, Vector3 v, float s, float a, float b)
{
    return { c.x + (u.x * a + v.x * b) * s, c.y + (u.y * a + v.y * b) * s, c.z + (u.z * a + v.z * b) * s };
}


static void RectSimboloColorSeguro(Vector3 c, Vector3 u, Vector3 v, float s, float x0, float y0, float x1, float y1, Color color)
{
    Vector3 p00 = PuntoSimboloColorSeguro(c, u, v, s, x0, y0);
    Vector3 p10 = PuntoSimboloColorSeguro(c, u, v, s, x1, y0);
    Vector3 p11 = PuntoSimboloColorSeguro(c, u, v, s, x1, y1);
    Vector3 p01 = PuntoSimboloColorSeguro(c, u, v, s, x0, y1);
    DrawTriangle3D(p00, p10, p11, color);
    DrawTriangle3D(p00, p11, p01, color);
}


// Simbolo de accesibilidad de cada color (mismo en plataforma y en TV):
// circulo, triangulo, cuadrado, cruz, estrella, rombo y tres franjas.
static void DibujarSimboloColorSeguro(int indice, Vector3 c, Vector3 u, Vector3 v, float s, Color color)
{
    switch (indice)
    {
        case 0:
        {
            const int LADOS = 22;
            for (int k = 0; k < LADOS; k++)
            {
                float a0 = 6.2832f * (float)k / (float)LADOS;
                float a1 = 6.2832f * (float)(k + 1) / (float)LADOS;
                DrawTriangle3D(
                    c,
                    PuntoSimboloColorSeguro(c, u, v, s, std::cos(a0) * 0.92f, std::sin(a0) * 0.92f),
                    PuntoSimboloColorSeguro(c, u, v, s, std::cos(a1) * 0.92f, std::sin(a1) * 0.92f),
                    color
                );
            }
            break;
        }

        case 1:
            DrawTriangle3D(
                PuntoSimboloColorSeguro(c, u, v, s, 0.0f, 1.02f),
                PuntoSimboloColorSeguro(c, u, v, s, -1.0f, -0.74f),
                PuntoSimboloColorSeguro(c, u, v, s, 1.0f, -0.74f),
                color
            );
            break;

        case 2:
            RectSimboloColorSeguro(c, u, v, s, -0.78f, -0.78f, 0.78f, 0.78f, color);
            break;

        case 3:
            RectSimboloColorSeguro(c, u, v, s, -0.30f, -0.98f, 0.30f, 0.98f, color);
            RectSimboloColorSeguro(c, u, v, s, -0.98f, -0.30f, 0.98f, 0.30f, color);
            break;

        case 4:
        {
            for (int k = 0; k < 5; k++)
            {
                float aExt = (90.0f + 72.0f * (float)k) * DEG2RAD;
                float aIn0 = aExt - 36.0f * DEG2RAD;
                float aIn1 = aExt + 36.0f * DEG2RAD;
                Vector3 punta = PuntoSimboloColorSeguro(c, u, v, s, std::cos(aExt) * 1.06f, std::sin(aExt) * 1.06f);
                Vector3 in0 = PuntoSimboloColorSeguro(c, u, v, s, std::cos(aIn0) * 0.44f, std::sin(aIn0) * 0.44f);
                Vector3 in1 = PuntoSimboloColorSeguro(c, u, v, s, std::cos(aIn1) * 0.44f, std::sin(aIn1) * 0.44f);
                DrawTriangle3D(c, in0, punta, color);
                DrawTriangle3D(c, punta, in1, color);
            }
            break;
        }

        case 5:
        {
            Vector3 arriba = PuntoSimboloColorSeguro(c, u, v, s, 0.0f, 1.08f);
            Vector3 derecha = PuntoSimboloColorSeguro(c, u, v, s, 0.74f, 0.0f);
            Vector3 abajo = PuntoSimboloColorSeguro(c, u, v, s, 0.0f, -1.08f);
            Vector3 izquierda = PuntoSimboloColorSeguro(c, u, v, s, -0.74f, 0.0f);
            DrawTriangle3D(arriba, derecha, abajo, color);
            DrawTriangle3D(arriba, abajo, izquierda, color);
            break;
        }

        case 6:
            RectSimboloColorSeguro(c, u, v, s, -0.95f, 0.50f, 0.95f, 0.86f, color);
            RectSimboloColorSeguro(c, u, v, s, -0.95f, -0.18f, 0.95f, 0.18f, color);
            RectSimboloColorSeguro(c, u, v, s, -0.95f, -0.86f, 0.95f, -0.50f, color);
            break;
    }
}


//==================================================
// ESCENARIO: CRATER, LAVA, BASALTO
//==================================================

// Punto de la pared del crater (elipse) para un angulo y una capa.
static Vector3 PuntoCraterColorSeguro(float theta, int capa, int semilla)
{
    static const float ALTURAS[6] = { -2.9f, 0.4f, 3.0f, 5.4f, 7.9f, 10.8f };
    static const float ESCALAS[6] = { 0.97f, 1.03f, 1.09f, 1.16f, 1.21f, 1.30f };

    float jitterR = (Hash01ColorSeguro(semilla, capa) - 0.5f) * 0.05f;
    float jitterY = capa == 0 ? 0.0f : (Hash01ColorSeguro(semilla, capa + 11) - 0.5f) * (capa == 5 ? 1.8f : 0.8f);
    float s = ESCALAS[capa] + jitterR;

    return
    {
        std::cos(theta) * RADIO_CRATER_X_COLOR * s,
        ALTURAS[capa] + jitterY,
        CENTRO_CRATER_Z_COLOR + std::sin(theta) * RADIO_CRATER_Z_COLOR * s
    };
}


static Color ColorParedCraterColorSeguro(int banda, float theta, int semilla, Vector3 p)
{
    static const Color ESTRATOS[5] =
    {
        { 62, 40, 36, 255 },
        { 92, 52, 38, 255 },
        { 50, 38, 38, 255 },
        { 98, 62, 44, 255 },
        { 68, 48, 42, 255 }
    };

    // Las caras que miran a la camara (pared del fondo) reciben mas luz.
    float frente = -std::sin(theta);
    float luz = 0.60f + 0.36f * (frente > 0.0f ? frente : 0.0f);
    luz *= 0.86f + 0.24f * Hash01ColorSeguro(semilla, banda + 40);

    Color c = TinteColorSeguro(ESTRATOS[banda], luz);

    // Reflejo caliente de la lava en las capas bajas.
    float calor = Limitar01ColorSeguro(1.0f - (p.y - NIVEL_LAVA_COLOR) / 7.0f);
    c = MezclarColorSeguro(c, Color{ 196, 72, 30, 255 }, calor * calor * 0.62f);

    return NieblaColorSeguro(c, p);
}


static int SegmentosCraterColorSeguro()
{
    NivelCalidadGrafica calidad = CalidadDecoracion();
    return calidad == CALIDAD_ALTA ? 44 : (calidad == CALIDAD_MEDIA ? 32 : 22);
}


static const float THETA_INICIO_CRATER = -207.0f * DEG2RAD;
static const float THETA_FIN_CRATER = 27.0f * DEG2RAD;


// Pared del crater en capas (estratos) con borde irregular.
static void DibujarParedesCraterColorSeguro()
{
    const int SEG = SegmentosCraterColorSeguro();

    for (int banda = 0; banda < 5; banda++)
    {
        rlCheckRenderBatchLimit(SEG * 6);
        rlBegin(RL_TRIANGLES);

        for (int j = 0; j < SEG; j++)
        {
            float t0 = THETA_INICIO_CRATER + (THETA_FIN_CRATER - THETA_INICIO_CRATER) * (float)j / (float)SEG;
            float t1 = THETA_INICIO_CRATER + (THETA_FIN_CRATER - THETA_INICIO_CRATER) * (float)(j + 1) / (float)SEG;

            Vector3 a0 = PuntoCraterColorSeguro(t0, banda, j);
            Vector3 a1 = PuntoCraterColorSeguro(t1, banda, j + 1);
            Vector3 b0 = PuntoCraterColorSeguro(t0, banda + 1, j);
            Vector3 b1 = PuntoCraterColorSeguro(t1, banda + 1, j + 1);

            Color ca0 = ColorParedCraterColorSeguro(banda, t0, j, a0);
            Color ca1 = ColorParedCraterColorSeguro(banda, t1, j, a1);
            Color cb0 = TinteColorSeguro(ColorParedCraterColorSeguro(banda, t0, j, b0), 1.12f);
            Color cb1 = TinteColorSeguro(ColorParedCraterColorSeguro(banda, t1, j, b1), 1.12f);

            VerticeColorSeguro(a0, ca0); VerticeColorSeguro(a1, ca1); VerticeColorSeguro(b1, cb1);
            VerticeColorSeguro(a0, ca0); VerticeColorSeguro(b1, cb1); VerticeColorSeguro(b0, cb0);
        }

        rlEnd();
    }

    // Lineas de estrato y grietas: rompen la regularidad de las bandas.
    for (int j = 0; j < SEG; j++)
    {
        float t0 = THETA_INICIO_CRATER + (THETA_FIN_CRATER - THETA_INICIO_CRATER) * (float)j / (float)SEG;
        float t1 = THETA_INICIO_CRATER + (THETA_FIN_CRATER - THETA_INICIO_CRATER) * (float)(j + 1) / (float)SEG;

        for (int capa = 1; capa < 5; capa++)
        {
            Vector3 p0 = PuntoCraterColorSeguro(t0, capa, j);
            Vector3 p1 = PuntoCraterColorSeguro(t1, capa, j + 1);
            DrawLine3D(p0, p1, NieblaColorSeguro(Color{ 30, 22, 22, 255 }, p0));
        }

        if (Hash01ColorSeguro(j, 77) > 0.62f)
        {
            Vector3 p0 = PuntoCraterColorSeguro(t0, 2, j);
            Vector3 p1 = PuntoCraterColorSeguro(t0, 3, j);
            Vector3 medio = { (p0.x + p1.x) * 0.5f + 0.3f, (p0.y + p1.y) * 0.5f, (p0.z + p1.z) * 0.5f };
            DrawLine3D(p0, medio, Color{ 34, 24, 24, 255 });
            DrawLine3D(medio, p1, Color{ 34, 24, 24, 255 });
        }
    }

    // Repisas: rocas sobre las terrazas.
    if (CalidadDecoracion() == CALIDAD_BAJA) return;

    int paso = CalidadDecoracion() == CALIDAD_ALTA ? 2 : 3;

    for (int j = 1; j < SEG - 1; j += paso)
    {
        float th = THETA_INICIO_CRATER + (THETA_FIN_CRATER - THETA_INICIO_CRATER) * ((float)j + 0.5f) / (float)SEG;
        int capa = 1 + (int)(Hash01ColorSeguro(j, 5) * 3.0f);
        Vector3 p = PuntoCraterColorSeguro(th, capa, j);
        p.x *= 0.985f;
        p.z = CENTRO_CRATER_Z_COLOR + (p.z - CENTRO_CRATER_Z_COLOR) * 0.985f;
        float tam = 0.7f + Hash01ColorSeguro(j, 9) * 1.1f;
        Color roca = NieblaColorSeguro(TinteColorSeguro(Color{ 74, 56, 50, 255 }, 0.85f + 0.3f * Hash01ColorSeguro(j, 3)), p);
        RocaGiradaColorSeguro({ p.x, p.y + tam * 0.3f, p.z }, { tam * 1.4f, tam * 0.8f, tam }, Hash01ColorSeguro(j, 8) * 90.0f, roca);
    }
}


// Cascadas de lava que bajan por la pared del crater.
static int CantidadCascadasColorSeguro()
{
    return CalidadDecoracion() == CALIDAD_BAJA ? 2 : 4;
}


static float ThetaCascadaColorSeguro(int indice)
{
    static const float ANGULOS[4] = { -128.0f, -52.0f, -168.0f, -14.0f };
    return ANGULOS[indice] * DEG2RAD;
}


static Vector3 PuntoCascadaColorSeguro(float theta, float y)
{
    // Altura -> escala de la pared (interpolada entre capas) y algo adentro.
    static const float ALTURAS[6] = { -2.9f, 0.4f, 3.0f, 5.4f, 7.9f, 10.8f };
    static const float ESCALAS[6] = { 0.97f, 1.03f, 1.09f, 1.16f, 1.21f, 1.30f };

    float s = ESCALAS[0];
    for (int k = 0; k < 5; k++)
    {
        if (y >= ALTURAS[k] && y <= ALTURAS[k + 1])
        {
            float u = (y - ALTURAS[k]) / (ALTURAS[k + 1] - ALTURAS[k]);
            s = ESCALAS[k] + (ESCALAS[k + 1] - ESCALAS[k]) * u;
        }
    }

    s -= 0.035f;

    return
    {
        std::cos(theta) * RADIO_CRATER_X_COLOR * s,
        y,
        CENTRO_CRATER_Z_COLOR + std::sin(theta) * RADIO_CRATER_Z_COLOR * s
    };
}


static void DibujarCascadasLavaColorSeguro(float t)
{
    const int FILAS = 14;
    const float Y_ARRIBA = 6.6f;
    const float Y_ABAJO = NIVEL_LAVA_COLOR - 0.1f;

    for (int c = 0; c < CantidadCascadasColorSeguro(); c++)
    {
        float th = ThetaCascadaColorSeguro(c);
        float ancho = 0.055f + 0.02f * (float)(c % 2);

        rlCheckRenderBatchLimit(FILAS * 6);
        rlBegin(RL_TRIANGLES);

        for (int f = 0; f < FILAS; f++)
        {
            float u0 = (float)f / (float)FILAS;
            float u1 = (float)(f + 1) / (float)FILAS;
            float y0 = Y_ARRIBA + (Y_ABAJO - Y_ARRIBA) * u0;
            float y1 = Y_ARRIBA + (Y_ABAJO - Y_ARRIBA) * u1;

            // Se ensancha al bajar.
            float w0 = ancho * (0.55f + 0.6f * u0);
            float w1 = ancho * (0.55f + 0.6f * u1);

            Vector3 i0 = PuntoCascadaColorSeguro(th - w0, y0);
            Vector3 d0 = PuntoCascadaColorSeguro(th + w0, y0);
            Vector3 i1 = PuntoCascadaColorSeguro(th - w1, y1);
            Vector3 d1 = PuntoCascadaColorSeguro(th + w1, y1);

            // Bandas brillantes que bajan con el flujo.
            float b0 = 0.5f + 0.5f * std::sin(u0 * 22.0f - t * 7.0f + (float)c);
            float b1 = 0.5f + 0.5f * std::sin(u1 * 22.0f - t * 7.0f + (float)c);
            Color c0 = NieblaColorSeguro(MezclarColorSeguro(Color{ 214, 62, 16, 255 }, Color{ 255, 196, 70, 255 }, b0), i0);
            Color c1 = NieblaColorSeguro(MezclarColorSeguro(Color{ 214, 62, 16, 255 }, Color{ 255, 196, 70, 255 }, b1), i1);

            VerticeColorSeguro(i0, c0); VerticeColorSeguro(d0, c0); VerticeColorSeguro(d1, c1);
            VerticeColorSeguro(i0, c0); VerticeColorSeguro(d1, c1); VerticeColorSeguro(i1, c1);
        }

        rlEnd();

        // Boca de la cascada: roca oscura con borde encendido.
        Vector3 boca = PuntoCascadaColorSeguro(th, Y_ARRIBA + 0.15f);
        RocaGiradaColorSeguro(boca, { 1.5f, 0.5f, 0.9f }, th * RAD2DEG, NieblaColorSeguro(Color{ 54, 36, 32, 255 }, boca));
    }
}


static Color ColorLavaColorSeguro(float x, float z, float t)
{
    float n =
        0.5f * std::sin(x * 0.42f + t * 0.9f + std::sin(z * 0.31f - t * 0.4f) * 1.6f) +
        0.5f * std::sin(z * 0.57f + x * 0.21f - t * 0.7f);
    float h = 0.5f + 0.5f * n;

    Color c = h < 0.5f
        ? MezclarColorSeguro(Color{ 124, 24, 10, 255 }, Color{ 220, 68, 18, 255 }, h * 2.0f)
        : MezclarColorSeguro(Color{ 220, 68, 18, 255 }, Color{ 255, 178, 56, 255 }, (h - 0.5f) * 2.0f);

    // Costra fria contra las paredes.
    float ex = x / RADIO_CRATER_X_COLOR;
    float ez = (z - CENTRO_CRATER_Z_COLOR) / RADIO_CRATER_Z_COLOR;
    float e = ex * ex + ez * ez;
    if (e > 0.60f)
    {
        c = MezclarColorSeguro(c, Color{ 60, 24, 16, 255 }, Limitar01ColorSeguro((e - 0.60f) / 0.42f) * 0.82f);
    }

    return NieblaColorSeguro(c, { x, NIVEL_LAVA_COLOR, z });
}


// Lago de lava: malla con color por vertice que fluye con el tiempo.
static void DibujarLagoLavaColorSeguro(float t)
{
    const bool baja = CalidadDecoracion() == CALIDAD_BAJA;
    const int NX = baja ? 14 : 26;
    const int NZ = baja ? 11 : 20;
    const float X0 = -22.0f, X1 = 22.0f;
    const float Z0 = -21.0f, Z1 = 13.0f;
    const float y = NIVEL_LAVA_COLOR;

    for (int iz = 0; iz < NZ; iz++)
    {
        float z0 = Z0 + (Z1 - Z0) * (float)iz / (float)NZ;
        float z1 = Z0 + (Z1 - Z0) * (float)(iz + 1) / (float)NZ;

        rlCheckRenderBatchLimit(NX * 6);
        rlBegin(RL_TRIANGLES);

        for (int ix = 0; ix < NX; ix++)
        {
            float x0 = X0 + (X1 - X0) * (float)ix / (float)NX;
            float x1 = X0 + (X1 - X0) * (float)(ix + 1) / (float)NX;

            Color c00 = ColorLavaColorSeguro(x0, z0, t);
            Color c10 = ColorLavaColorSeguro(x1, z0, t);
            Color c01 = ColorLavaColorSeguro(x0, z1, t);
            Color c11 = ColorLavaColorSeguro(x1, z1, t);

            VerticeColorSeguro({ x0, y, z0 }, c00); VerticeColorSeguro({ x0, y, z1 }, c01); VerticeColorSeguro({ x1, y, z1 }, c11);
            VerticeColorSeguro({ x0, y, z0 }, c00); VerticeColorSeguro({ x1, y, z1 }, c11); VerticeColorSeguro({ x1, y, z0 }, c10);
        }

        rlEnd();
    }
}


// Costras de basalto que derivan en circulo sobre la lava, con bordes
// encendidos.
static void DibujarCostrasLavaColorSeguro(float t)
{
    const int N = CalidadDecoracion() == CALIDAD_BAJA ? 8 : 18;

    for (int k = 0; k < N; k++)
    {
        float r = 0.55f + 0.38f * Hash01ColorSeguro(k, 1);
        float a = Hash01ColorSeguro(k, 2) * 6.2832f + t * (0.010f + 0.012f * Hash01ColorSeguro(k, 3));
        float x = std::cos(a) * RADIO_CRATER_X_COLOR * r;
        float z = CENTRO_CRATER_Z_COLOR + std::sin(a) * RADIO_CRATER_Z_COLOR * r;
        if (z > 11.0f) continue;

        float tam = 0.7f + 0.8f * Hash01ColorSeguro(k, 4);
        float giro = Hash01ColorSeguro(k, 5) * 6.2832f + t * 0.05f;
        Vector3 centro = { x, NIVEL_LAVA_COLOR + 0.05f, z };
        Vector3 bordes[7]{};

        for (int v = 0; v < 6; v++)
        {
            float av = giro + 1.0472f * (float)v;
            float rv = tam * (0.75f + 0.4f * Hash01ColorSeguro(k * 7 + v, 6));
            bordes[v] = { x + std::cos(av) * rv, centro.y, z + std::sin(av) * rv * 0.8f };
        }
        bordes[6] = bordes[0];

        Color costra = NieblaColorSeguro(Color{ 56, 30, 24, 255 }, centro);
        Color brillo = NieblaColorSeguro(Color{ 255, 150, 50, 255 }, centro);

        for (int v = 0; v < 6; v++)
        {
            DrawTriangle3D(centro, bordes[v + 1], bordes[v], costra);
            DrawLine3D(
                { bordes[v].x, centro.y + 0.01f, bordes[v].z },
                { bordes[v + 1].x, centro.y + 0.01f, bordes[v + 1].z },
                brillo
            );
        }
    }
}


// Burbujas: crecen, revientan y dejan una onda. Posiciones deterministas.
static void DibujarBurbujasLavaColorSeguro(float t)
{
    const int N = CalidadDecoracion() == CALIDAD_BAJA ? 5 : 14;

    for (int k = 0; k < N; k++)
    {
        float periodo = 1.6f + 1.5f * Hash01ColorSeguro(k, 11);
        float tiempo = t + Hash01ColorSeguro(k, 12) * 7.0f;
        int ciclo = (int)std::floor(tiempo / periodo);
        float fase = (tiempo - (float)ciclo * periodo) / periodo;

        float r = 0.3f + 0.62f * std::sqrt(Hash01ColorSeguro(k * 13 + ciclo, 14));
        float a = Hash01ColorSeguro(k * 17 + ciclo, 15) * 6.2832f;
        float x = std::cos(a) * RADIO_CRATER_X_COLOR * r;
        float z = CENTRO_CRATER_Z_COLOR + std::sin(a) * RADIO_CRATER_Z_COLOR * r;
        if (z > 11.0f) continue;

        Vector3 p = { x, NIVEL_LAVA_COLOR, z };

        if (fase < 0.8f)
        {
            float radio = 0.08f + 0.34f * SuavizarColorSeguro(fase / 0.8f);
            DrawSphereEx(p, radio, 5, 8, NieblaColorSeguro(Color{ 255, 150, 52, 255 }, p));
            DrawSphereEx({ p.x - radio * 0.3f, p.y + radio * 0.55f, p.z + radio * 0.2f }, radio * 0.3f, 3, 5, Color{ 255, 226, 140, 255 });
        }
        else
        {
            float u = (fase - 0.8f) / 0.2f;
            DrawCircle3D({ x, NIVEL_LAVA_COLOR + 0.03f, z }, 0.3f + 0.7f * u, { 1.0f, 0.0f, 0.0f }, 90.0f, Fade(Color{ 255, 210, 110, 255 }, 1.0f - u));
        }
    }
}


static Vector3 CentroCumuloBasaltoColorSeguro(int indice)
{
    static const Vector3 CUMULOS[8] =
    {
        { -12.6f, 0.0f, -1.2f }, { -10.2f, 0.0f, -8.4f }, { 12.8f, 0.0f, -2.6f }, { 10.4f, 0.0f, -8.8f },
        { -6.6f, 0.0f, -13.6f }, { 7.4f, 0.0f, -13.4f }, { -13.6f, 0.0f, 5.0f }, { 13.8f, 0.0f, 5.4f }
    };
    return CUMULOS[indice];
}


// Columnas de basalto: prismas hexagonales en cumulos sobre la lava.
static void DibujarColumnasBasaltoColorSeguro()
{
    NivelCalidadGrafica calidad = CalidadDecoracion();
    int porCumulo = calidad == CALIDAD_ALTA ? 7 : (calidad == CALIDAD_MEDIA ? 5 : 3);

    for (int c = 0; c < 8; c++)
    {
        Vector3 centro = CentroCumuloBasaltoColorSeguro(c);
        float lejania = Limitar01ColorSeguro((-centro.z + 2.0f) / 14.0f);

        for (int k = 0; k < porCumulo; k++)
        {
            float a = Hash01ColorSeguro(c, k) * 6.2832f;
            float d = k == 0 ? 0.0f : 0.7f + 1.0f * Hash01ColorSeguro(c, k + 20);
            float radio = 0.45f + 0.38f * Hash01ColorSeguro(c, k + 40);
            float alto = 0.8f + (2.0f + 3.4f * lejania) * Hash01ColorSeguro(c, k + 60);
            if (k == 0) alto += 1.4f;

            Vector3 base = { centro.x + std::cos(a) * d, NIVEL_LAVA_COLOR, centro.z + std::sin(a) * d };
            Color lado = NieblaColorSeguro(TinteColorSeguro(Color{ 66, 58, 62, 255 }, 0.85f + 0.3f * Hash01ColorSeguro(c, k + 80)), base);
            Color pie = NieblaColorSeguro(Color{ 150, 52, 26, 255 }, base);
            Color tapa = NieblaColorSeguro(Color{ 104, 94, 96, 255 }, base);

            PrismaHexColorSeguro(base, radio * 1.08f, radio, -0.3f, alto, Hash01ColorSeguro(c, k + 90) * 60.0f, lado, pie, true, tapa);
        }
    }
}


// Rocas grandes en las esquinas inferiores: enmarcan la escena.
static void DibujarPrimerPlanoColorSeguro()
{
    const Color roca = { 70, 52, 46, 255 };
    RocaGiradaColorSeguro({ -12.4f, NIVEL_LAVA_COLOR + 0.7f, 8.4f }, { 4.4f, 2.6f, 3.2f }, 24.0f, roca);
    RocaGiradaColorSeguro({ -10.2f, NIVEL_LAVA_COLOR + 0.3f, 10.2f }, { 2.4f, 1.4f, 2.0f }, -18.0f, TinteColorSeguro(roca, 0.85f));
    RocaGiradaColorSeguro({ 12.6f, NIVEL_LAVA_COLOR + 0.8f, 8.0f }, { 4.0f, 2.9f, 3.4f }, -30.0f, roca);
    RocaGiradaColorSeguro({ 10.4f, NIVEL_LAVA_COLOR + 0.2f, 10.4f }, { 2.2f, 1.2f, 1.8f }, 40.0f, TinteColorSeguro(roca, 0.9f));
}


//==================================================
// TELEVISOR (ANUNCIO DEL COLOR)
//==================================================

static float PulsoAnuncioColorSeguro(const MinijuegoColorSeguro& m)
{
    return std::exp(-m.tiempoAnuncio * 2.6f);
}


static void DibujarTelevisorColorSeguro(const MinijuegoColorSeguro& m, float t)
{
    const Vector3 c = CENTRO_TV_COLOR;
    const int indice = m.indicePlataformaSegura;
    const Color colorPantalla = ObtenerColorPlataforma(indice);
    const Color hierro = { 74, 66, 70, 255 };

    // Pilares de basalto que salen de la lava.
    for (int lado = -1; lado <= 1; lado += 2)
    {
        Vector3 base = { 3.9f * (float)lado, NIVEL_LAVA_COLOR, c.z };
        PrismaHexColorSeguro(base, 0.78f, 0.62f, -0.4f, 8.9f, 10.0f, Color{ 70, 62, 66, 255 }, Color{ 150, 56, 28, 255 }, true, Color{ 104, 96, 98, 255 });

        // Abrazaderas de hierro.
        CajaFacetadaColorSeguro({ base.x, 2.2f, c.z }, { 1.5f, 0.28f, 1.5f }, hierro);
        CajaFacetadaColorSeguro({ base.x, 5.6f, c.z }, { 1.5f, 0.28f, 1.5f }, hierro);

        // Tirantes diagonales hacia el travesano.
        DrawCylinderEx({ base.x, 4.9f, c.z + 0.2f }, { base.x * 0.55f, 6.4f, c.z + 0.2f }, 0.09f, 0.09f, 6, TinteColorSeguro(hierro, 0.8f));
        // Cadena que sostiene la pantalla.
        DrawCylinderEx({ 1.9f * (float)lado, 6.5f, c.z }, { 1.9f * (float)lado, c.y + ALTO_TV_COLOR * 0.5f, c.z }, 0.06f, 0.06f, 5, Color{ 40, 36, 38, 255 });
    }

    // Travesano superior.
    CajaFacetadaColorSeguro({ 0.0f, 6.65f, c.z }, { 9.4f, 0.55f, 0.8f }, hierro);
    for (int k = -4; k <= 4; k++)
    {
        DrawCubeV({ (float)k * 1.0f, 6.65f, c.z + 0.42f }, { 0.12f, 0.12f, 0.05f }, Color{ 40, 36, 38, 255 });
    }

    // Carcasa de la TV.
    CajaFacetadaColorSeguro(c, { ANCHO_TV_COLOR, ALTO_TV_COLOR, FONDO_TV_COLOR }, Color{ 46, 42, 50, 255 });
    DrawCubeWiresV(c, { ANCHO_TV_COLOR, ALTO_TV_COLOR, FONDO_TV_COLOR }, Fade(BLACK, 0.4f));

    float zFrente = c.z + FONDO_TV_COLOR * 0.5f;
    float anchoPantalla = ANCHO_TV_COLOR - 0.8f;
    float altoPantalla = ALTO_TV_COLOR - 0.9f;

    // Marco de hierro.
    CajaFacetadaColorSeguro({ c.x, c.y + altoPantalla * 0.5f + 0.18f, zFrente + 0.06f }, { anchoPantalla + 0.5f, 0.22f, 0.14f }, hierro);
    CajaFacetadaColorSeguro({ c.x, c.y - altoPantalla * 0.5f - 0.18f, zFrente + 0.06f }, { anchoPantalla + 0.5f, 0.22f, 0.14f }, hierro);
    CajaFacetadaColorSeguro({ c.x - anchoPantalla * 0.5f - 0.14f, c.y, zFrente + 0.06f }, { 0.22f, altoPantalla + 0.5f, 0.14f }, hierro);
    CajaFacetadaColorSeguro({ c.x + anchoPantalla * 0.5f + 0.14f, c.y, zFrente + 0.06f }, { 0.22f, altoPantalla + 0.5f, 0.14f }, hierro);

    // Franja de peligro bajo la pantalla.
    for (int k = 0; k < 12; k++)
    {
        float x = c.x - ANCHO_TV_COLOR * 0.5f + 0.25f + (float)k * 0.5f;
        DrawCubeV({ x, c.y - ALTO_TV_COLOR * 0.5f - 0.12f, c.z + 0.2f }, { 0.5f, 0.24f, 0.3f }, k % 2 == 0 ? Color{ 214, 170, 70, 255 } : Color{ 34, 30, 32, 255 });
    }

    // Pantalla: centro mas brillante, bordes algo mas oscuros.
    float pulso = PulsoAnuncioColorSeguro(m);
    Color centro = TinteColorSeguro(colorPantalla, 1.05f + 0.25f * pulso);
    Color borde = TinteColorSeguro(colorPantalla, 0.78f);
    float x0 = c.x - anchoPantalla * 0.5f, x1 = c.x + anchoPantalla * 0.5f;
    float y0 = c.y - altoPantalla * 0.5f, y1 = c.y + altoPantalla * 0.5f;
    float zp = zFrente + 0.02f;

    rlCheckRenderBatchLimit(12);
    rlBegin(RL_TRIANGLES);
    Vector3 pc = { c.x, c.y, zp };
    Vector3 esquinas[5] = { { x0, y0, zp }, { x1, y0, zp }, { x1, y1, zp }, { x0, y1, zp }, { x0, y0, zp } };
    for (int k = 0; k < 4; k++)
    {
        VerticeColorSeguro(pc, centro);
        VerticeColorSeguro(esquinas[k], borde);
        VerticeColorSeguro(esquinas[k + 1], borde);
    }
    rlEnd();

    // Simbolo del color (en tinta oscura) y lineas de barrido.
    Color tinta = TinteColorSeguro(colorPantalla, 0.30f);
    DibujarSimboloColorSeguro(indice, { c.x, c.y, zp + 0.02f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, 0.85f, tinta);

    for (int k = 0; k < 9; k++)
    {
        float u = std::fmod((float)k / 9.0f + t * 0.12f, 1.0f);
        float y = y0 + 0.05f + (altoPantalla - 0.1f) * u;
        DrawLine3D({ x0 + 0.04f, y, zp + 0.03f }, { x1 - 0.04f, y, zp + 0.03f }, Fade(BLACK, 0.16f));
    }

    // Luces de aviso en las esquinas: titilan con el anuncio.
    for (int lado = -1; lado <= 1; lado += 2)
    {
        Vector3 lampara = { (ANCHO_TV_COLOR * 0.5f - 0.35f) * (float)lado, c.y + ALTO_TV_COLOR * 0.5f + 0.18f, c.z + 0.1f };
        DrawCubeV({ lampara.x, lampara.y - 0.12f, lampara.z }, { 0.42f, 0.14f, 0.42f }, hierro);
        float encendida = 0.55f + 0.45f * (pulso > 0.08f ? (std::sin(m.tiempoAnuncio * 22.0f) > 0.0f ? 1.0f : 0.3f) : 0.6f);
        DrawSphereEx(lampara, 0.2f, 6, 8, TinteColorSeguro(colorPantalla, encendida));
    }
}


//==================================================
// PLATAFORMAS HEXAGONALES
//==================================================

static float BajadaVisualPlataformaColorSeguro(const MinijuegoColorSeguro& m, int i)
{
    if (m.emergerPlataforma[i] <= 0.0f) return 0.0f;
    float u = 1.0f - m.emergerPlataforma[i] / DURACION_EMERGER_COLOR;
    return (1.0f - EaseOutBackColorSeguro(u)) * 2.8f;
}


// Plataforma: tapa del color con bisel, banda, cuerpo de roca y una
// columna de basalto que se hunde en la lava. Todas se dibujan igual
// hasta que caen (ninguna se marca antes del anuncio).
static void DibujarPlataformaColorSeguro(const MinijuegoColorSeguro& m, int i)
{
    const BloquePrueba& p = m.plataformas[i];
    float yTop = p.posicion.y + p.tamano.y * 0.5f - BajadaVisualPlataformaColorSeguro(m, i);

    if (yTop < NIVEL_LAVA_COLOR - 3.6f) return;

    float caida = p.cayendo ? -p.posicion.y : 0.0f;
    float inclinacion = caida > 0.0f ? (caida * 6.0f > 14.0f ? 14.0f : caida * 6.0f) : 0.0f;
    float ejeAngulo = Hash01ColorSeguro(i, 3) * 6.2832f;

    // Al hundirse la tapa se pone al rojo.
    float calor = Limitar01ColorSeguro((NIVEL_LAVA_COLOR + 1.2f - yTop) / 1.4f);
    Color color = MezclarColorSeguro(p.color, Color{ 255, 122, 40, 255 }, calor * 0.85f);

    rlPushMatrix();
    rlTranslatef(p.posicion.x, yTop, p.posicion.z);
    if (inclinacion > 0.0f)
    {
        rlRotatef(inclinacion, std::cos(ejeAngulo), 0.0f, std::sin(ejeAngulo));
    }

    const Vector3 o = { 0.0f, 0.0f, 0.0f };
    const float R = RADIO_HEXAGONO_COLOR;
    const float rTapa = R - 0.22f;
    Color roca = TinteColorSeguro(Color{ 70, 62, 64, 255 }, 0.9f + 0.2f * Hash01ColorSeguro(i, 7));

    // Columna de basalto bajo la plataforma (llega a la lava).
    PrismaHexColorSeguro(o, R * 0.62f, R * 0.74f, -3.3f, -0.6f, 0.0f, TinteColorSeguro(roca, 0.8f), Color{ 160, 58, 26, 255 }, false, roca);
    // Cuerpo de roca.
    PrismaHexColorSeguro(o, R * 0.88f, R, -0.62f, -0.18f, 0.0f, roca, TinteColorSeguro(roca, 0.75f), false, roca);
    // Banda del color.
    PrismaHexColorSeguro(o, R, R, -0.18f, -0.07f, 0.0f, TinteColorSeguro(color, 0.62f), TinteColorSeguro(color, 0.5f), false, color);
    // Bisel hacia la tapa.
    PrismaHexColorSeguro(o, R, rTapa, -0.07f, 0.0f, 0.0f, TinteColorSeguro(color, 0.92f), TinteColorSeguro(color, 0.74f), false, color);

    // Tapa: centro algo mas claro.
    rlCheckRenderBatchLimit(18);
    rlBegin(RL_TRIANGLES);
    Color centro = TinteColorSeguro(color, 1.06f);
    Color bordeTapa = TinteColorSeguro(color, 0.93f);
    for (int k = 0; k < 6; k++)
    {
        float a0 = (30.0f + 60.0f * (float)k) * DEG2RAD;
        float a1 = (90.0f + 60.0f * (float)k) * DEG2RAD;
        VerticeColorSeguro(o, centro);
        VerticeColorSeguro({ std::cos(a1) * rTapa, 0.0f, std::sin(a1) * rTapa }, bordeTapa);
        VerticeColorSeguro({ std::cos(a0) * rTapa, 0.0f, std::sin(a0) * rTapa }, bordeTapa);
    }
    rlEnd();

    // Contorno interior, aristas y simbolo.
    Color linea = TinteColorSeguro(color, 0.70f);
    for (int k = 0; k < 6; k++)
    {
        float a0 = (30.0f + 60.0f * (float)k) * DEG2RAD;
        float a1 = (90.0f + 60.0f * (float)k) * DEG2RAD;
        DrawLine3D({ std::cos(a0) * 1.72f, 0.02f, std::sin(a0) * 1.72f }, { std::cos(a1) * 1.72f, 0.02f, std::sin(a1) * 1.72f }, linea);
        DrawLine3D({ std::cos(a0) * R, -0.07f, std::sin(a0) * R }, { std::cos(a1) * R, -0.07f, std::sin(a1) * R }, Fade(BLACK, 0.55f));
        DrawLine3D({ std::cos(a0) * R, -0.07f, std::sin(a0) * R }, { std::cos(a0) * R * 0.88f, -0.62f, std::sin(a0) * R * 0.88f }, Fade(BLACK, 0.45f));
    }

    DibujarSimboloColorSeguro(
        i,
        { 0.0f, 0.025f, 0.0f },
        { 1.0f, 0.0f, 0.0f },
        { 0.0f, 0.0f, -1.0f },
        0.72f,
        TinteColorSeguro(color, 0.40f)
    );

    rlPopMatrix();
}


// Anillo de color bajo los pies y flecha sobre la cabeza: el modelo del
// jugador es oscuro y sobre la lava no se distingue quien es quien.
// MODELO FUTURO: la flecha puede pasar a ser un icono de jugador del GLB.
static void DibujarIndicadorJugadorColorSeguro(
    const JugadorPrueba& jugador,
    Color color,
    float tiempo
)
{
    float pies = jugador.posicion.y - jugador.tamano.y * 0.5f;
    if (jugador.cayendo || pies < -0.35f) return;

    Vector3 centro = { jugador.posicion.x, pies + 0.04f, jugador.posicion.z };
    for (int k = 0; k < 3; k++)
    {
        DrawCircle3D(
            centro,
            0.52f + k * 0.045f,
            { 1.0f, 0.0f, 0.0f },
            90.0f,
            color
        );
    }

    float rebote = std::sin(tiempo * 5.0f + jugador.posicion.x) * 0.06f;
    float cabeza = pies + 2.15f + rebote;
    DrawCylinderEx(
        { jugador.posicion.x, cabeza, jugador.posicion.z },
        { jugador.posicion.x, cabeza + 0.32f, jugador.posicion.z },
        0.0f,
        0.20f,
        10,
        color
    );
}


//==================================================
// PARTICULAS PROPIAS
//==================================================

static float AzarColorSeguro(MinijuegoColorSeguro& m, float minimo, float maximo)
{
    m.semillaEfectos = m.semillaEfectos * 1664525u + 1013904223u;
    float u = (float)((m.semillaEfectos >> 8) & 0xFFFFu) / 65535.0f;
    return minimo + (maximo - minimo) * u;
}


static int CantidadParticulasColorSeguro(int cantidad)
{
    long n = std::lround((float)cantidad * FactorCalidadGrafica(CalidadParticulas()));
    return n < 1 ? 1 : (int)n;
}


static ParticulaColorSeguro* NuevaParticulaColorSeguro(MinijuegoColorSeguro& m)
{
    for (int i = 0; i < MAX_PARTICULAS_COLOR_SEGURO; i++)
    {
        if (!m.efectos[i].activa)
        {
            m.efectos[i] = ParticulaColorSeguro{};
            m.efectos[i].activa = true;
            return &m.efectos[i];
        }
    }

    return nullptr;
}


static void EmitirParticulaColorSeguro(
    MinijuegoColorSeguro& m,
    TipoParticulaColorSeguro tipo,
    Vector3 posicion,
    Vector3 velocidad,
    float vida,
    float tamano,
    Color color
)
{
    ParticulaColorSeguro* p = NuevaParticulaColorSeguro(m);
    if (p == nullptr) return;

    p->tipo = tipo;
    p->posicion = posicion;
    p->velocidad = velocidad;
    p->vida = vida;
    p->vidaMaxima = vida;
    p->tamano = tamano;
    p->color = color;
}


// Salpicadura de lava: gotas, brasas y humo alrededor de un punto.
static void EmitirSalpicaduraColorSeguro(MinijuegoColorSeguro& m, Vector3 centro, float radio, int cantidad)
{
    int n = CantidadParticulasColorSeguro(cantidad);

    for (int k = 0; k < n; k++)
    {
        float a = AzarColorSeguro(m, 0.0f, 6.2832f);
        float r = radio * AzarColorSeguro(m, 0.5f, 1.05f);
        float fuerza = AzarColorSeguro(m, 1.0f, 3.2f);
        Vector3 pos = { centro.x + std::cos(a) * r, NIVEL_LAVA_COLOR + 0.05f, centro.z + std::sin(a) * r };
        Color color = MezclarColorSeguro(Color{ 255, 110, 30, 255 }, Color{ 255, 214, 90, 255 }, AzarColorSeguro(m, 0.0f, 1.0f));

        EmitirParticulaColorSeguro(
            m, PARTICULA_COLOR_GOTA, pos,
            { std::cos(a) * fuerza, AzarColorSeguro(m, 4.5f, 8.5f), std::sin(a) * fuerza },
            AzarColorSeguro(m, 0.8f, 1.3f), AzarColorSeguro(m, 0.10f, 0.22f), color
        );
    }

    for (int k = 0; k < CantidadParticulasColorSeguro(cantidad / 3); k++)
    {
        float a = AzarColorSeguro(m, 0.0f, 6.2832f);
        Vector3 pos = { centro.x + std::cos(a) * radio * 0.6f, NIVEL_LAVA_COLOR + 0.2f, centro.z + std::sin(a) * radio * 0.6f };
        EmitirParticulaColorSeguro(
            m, PARTICULA_COLOR_BRASA, pos,
            { AzarColorSeguro(m, -0.8f, 0.8f), AzarColorSeguro(m, 2.0f, 4.0f), AzarColorSeguro(m, -0.8f, 0.8f) },
            AzarColorSeguro(m, 0.9f, 1.8f), AzarColorSeguro(m, 0.05f, 0.09f), Color{ 255, 190, 80, 255 }
        );
    }

    for (int k = 0; k < CantidadParticulasColorSeguro(4); k++)
    {
        Vector3 pos = { centro.x + AzarColorSeguro(m, -radio, radio) * 0.5f, NIVEL_LAVA_COLOR + 0.3f, centro.z + AzarColorSeguro(m, -radio, radio) * 0.5f };
        EmitirParticulaColorSeguro(
            m, PARTICULA_COLOR_HUMO, pos,
            { AzarColorSeguro(m, -0.3f, 0.3f), AzarColorSeguro(m, 0.9f, 1.5f), AzarColorSeguro(m, -0.3f, 0.3f) },
            AzarColorSeguro(m, 1.6f, 2.4f), AzarColorSeguro(m, 0.5f, 0.8f), Color{ 70, 58, 56, 255 }
        );
    }
}


// Escombros que se desprenden del borde cuando la plataforma empieza a caer.
static void EmitirEscombrosColorSeguro(MinijuegoColorSeguro& m, Vector3 centro)
{
    int n = CantidadParticulasColorSeguro(9);

    for (int k = 0; k < n; k++)
    {
        float a = AzarColorSeguro(m, 0.0f, 6.2832f);
        float r = RADIO_HEXAGONO_COLOR * AzarColorSeguro(m, 0.8f, 1.0f);
        Vector3 pos = { centro.x + std::cos(a) * r, centro.y - 0.2f, centro.z + std::sin(a) * r };
        EmitirParticulaColorSeguro(
            m, PARTICULA_COLOR_ROCA, pos,
            { std::cos(a) * AzarColorSeguro(m, 0.5f, 1.6f), AzarColorSeguro(m, 0.5f, 2.2f), std::sin(a) * AzarColorSeguro(m, 0.5f, 1.6f) },
            AzarColorSeguro(m, 0.8f, 1.4f), AzarColorSeguro(m, 0.12f, 0.24f), Color{ 76, 66, 66, 255 }
        );
    }

    for (int k = 0; k < CantidadParticulasColorSeguro(3); k++)
    {
        float a = AzarColorSeguro(m, 0.0f, 6.2832f);
        Vector3 pos = { centro.x + std::cos(a) * 2.0f, centro.y, centro.z + std::sin(a) * 2.0f };
        EmitirParticulaColorSeguro(
            m, PARTICULA_COLOR_HUMO, pos,
            { std::cos(a) * 0.4f, AzarColorSeguro(m, 0.3f, 0.7f), std::sin(a) * 0.4f },
            AzarColorSeguro(m, 1.0f, 1.6f), AzarColorSeguro(m, 0.35f, 0.55f), Color{ 120, 104, 98, 255 }
        );
    }
}


static Vector3 PuntoAzarLagoColorSeguro(MinijuegoColorSeguro& m)
{
    float a = AzarColorSeguro(m, 0.0f, 6.2832f);
    float r = std::sqrt(AzarColorSeguro(m, 0.0f, 1.0f)) * 0.92f;
    float z = CENTRO_CRATER_Z_COLOR + std::sin(a) * RADIO_CRATER_Z_COLOR * r;
    if (z > 10.0f) z = 10.0f - (z - 10.0f);
    return { std::cos(a) * RADIO_CRATER_X_COLOR * r, NIVEL_LAVA_COLOR + 0.1f, z };
}


// Ambiente: brasas que suben del lago y humo al pie de las cascadas.
static void ActualizarAmbienteColorSeguro(MinijuegoColorSeguro& m, float deltaTime)
{
    float factor = FactorCalidadGrafica(CalidadParticulas());

    m.acumuladorBrasas += deltaTime * 16.0f * factor;
    while (m.acumuladorBrasas >= 1.0f)
    {
        m.acumuladorBrasas -= 1.0f;
        Vector3 pos = PuntoAzarLagoColorSeguro(m);
        EmitirParticulaColorSeguro(
            m, PARTICULA_COLOR_BRASA, pos,
            { AzarColorSeguro(m, -0.3f, 0.3f), AzarColorSeguro(m, 0.8f, 1.9f), AzarColorSeguro(m, -0.3f, 0.3f) },
            AzarColorSeguro(m, 2.0f, 4.0f), AzarColorSeguro(m, 0.04f, 0.08f),
            MezclarColorSeguro(Color{ 255, 120, 40, 255 }, Color{ 255, 210, 110, 255 }, AzarColorSeguro(m, 0.0f, 1.0f))
        );
    }

    m.acumuladorHumo += deltaTime * 3.2f * factor;
    while (m.acumuladorHumo >= 1.0f)
    {
        m.acumuladorHumo -= 1.0f;
        int cascada = (int)AzarColorSeguro(m, 0.0f, (float)CantidadCascadasColorSeguro() - 0.01f);
        Vector3 pos = PuntoCascadaColorSeguro(ThetaCascadaColorSeguro(cascada), NIVEL_LAVA_COLOR + 0.3f);
        pos.x *= 0.96f;
        pos.z = CENTRO_CRATER_Z_COLOR + (pos.z - CENTRO_CRATER_Z_COLOR) * 0.96f;
        if (AzarColorSeguro(m, 0.0f, 1.0f) < 0.4f) pos = PuntoAzarLagoColorSeguro(m);

        EmitirParticulaColorSeguro(
            m, PARTICULA_COLOR_HUMO, pos,
            { AzarColorSeguro(m, -0.2f, 0.3f), AzarColorSeguro(m, 0.7f, 1.2f), AzarColorSeguro(m, -0.2f, 0.2f) },
            AzarColorSeguro(m, 3.0f, 4.6f), AzarColorSeguro(m, 0.6f, 1.0f), Color{ 64, 50, 50, 255 }
        );
    }
}


static void ActualizarParticulasColorSeguro(MinijuegoColorSeguro& m, float deltaTime)
{
    for (int i = 0; i < MAX_PARTICULAS_COLOR_SEGURO; i++)
    {
        ParticulaColorSeguro& p = m.efectos[i];
        if (!p.activa) continue;

        p.vida -= deltaTime;
        if (p.vida <= 0.0f)
        {
            p.activa = false;
            continue;
        }

        switch (p.tipo)
        {
            case PARTICULA_COLOR_GOTA:
            case PARTICULA_COLOR_ROCA:
                p.velocidad.y -= 16.0f * deltaTime;
                break;

            case PARTICULA_COLOR_BRASA:
                p.velocidad.x += std::sin(m.tiempoAnimacion * 3.0f + (float)i) * 0.6f * deltaTime;
                break;

            case PARTICULA_COLOR_HUMO:
                p.tamano += deltaTime * 0.45f;
                p.velocidad.x *= 1.0f - deltaTime * 0.3f;
                break;
        }

        p.posicion.x += p.velocidad.x * deltaTime;
        p.posicion.y += p.velocidad.y * deltaTime;
        p.posicion.z += p.velocidad.z * deltaTime;

        if (
            (p.tipo == PARTICULA_COLOR_GOTA || p.tipo == PARTICULA_COLOR_ROCA) &&
            p.posicion.y < NIVEL_LAVA_COLOR
        )
        {
            p.activa = false;
        }
    }
}


static void DibujarParticulasSolidasColorSeguro(const MinijuegoColorSeguro& m)
{
    for (int i = 0; i < MAX_PARTICULAS_COLOR_SEGURO; i++)
    {
        const ParticulaColorSeguro& p = m.efectos[i];
        if (!p.activa) continue;
        if (p.tipo != PARTICULA_COLOR_GOTA && p.tipo != PARTICULA_COLOR_ROCA) continue;

        DrawCubeV(p.posicion, { p.tamano, p.tamano, p.tamano }, p.color);
    }
}


static void DibujarHumoColorSeguro(const MinijuegoColorSeguro& m)
{
    for (int i = 0; i < MAX_PARTICULAS_COLOR_SEGURO; i++)
    {
        const ParticulaColorSeguro& p = m.efectos[i];
        if (!p.activa || p.tipo != PARTICULA_COLOR_HUMO) continue;

        float u = 1.0f - p.vida / p.vidaMaxima;
        float alfa = (u < 0.15f ? u / 0.15f : 1.0f - (u - 0.15f) / 0.85f) * 0.34f;
        DrawSphereEx(p.posicion, p.tamano, 4, 6, AlfaColorSeguro(p.color, alfa));
    }
}


static void DibujarBrasasColorSeguro(const MinijuegoColorSeguro& m)
{
    for (int i = 0; i < MAX_PARTICULAS_COLOR_SEGURO; i++)
    {
        const ParticulaColorSeguro& p = m.efectos[i];
        if (!p.activa || p.tipo != PARTICULA_COLOR_BRASA) continue;

        float alfa = Limitar01ColorSeguro(p.vida / p.vidaMaxima * 1.6f);
        DrawCubeV(p.posicion, { p.tamano, p.tamano, p.tamano }, AlfaColorSeguro(p.color, alfa));
    }
}


//==================================================
// RESULTADO Y REGLAS
//==================================================

static void InicializarResultadoColorSeguro(
    MinijuegoColorSeguro& minijuego,
    const Participante participantes[]
)
{
    InicializarResultadoMinijuego(
        minijuego.resultado,
        participantes,
        FORMATO_MINIJUEGO_INDIVIDUAL
    );
}


static int ContarJugadoresVivosColorSeguro(
    const MinijuegoColorSeguro& minijuego
)
{
    int cantidad = 0;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (
            minijuego.resultado.participantes[i].participo &&
            !minijuego.estadosJugadores[i].eliminado
        )
        {
            cantidad++;
        }
    }

    return cantidad;
}


static int RegistrarEliminacionesColorSeguro(
    MinijuegoColorSeguro& minijuego,
    JugadorPrueba jugadores[],
    int vivosAntes
)
{
    int eliminados = 0;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (
            minijuego.resultado.participantes[i].participo &&
            !minijuego.estadosJugadores[i].eliminado &&
            jugadores[i].cayendo
        )
        {
            eliminados++;
        }
    }

    if (eliminados == 0)
    {
        return 0;
    }

    int posicion = vivosAntes - eliminados + 1;

    int tiempoMs =
        (int)std::lround(
            minijuego.tiempoJugado * 1000.0f
        );

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (
            minijuego.resultado.participantes[i].participo &&
            !minijuego.estadosJugadores[i].eliminado &&
            jugadores[i].cayendo
        )
        {
            minijuego.estadosJugadores[i].eliminado = true;
            minijuego.estadosJugadores[i].posicionFinal = posicion;
            minijuego.estadosJugadores[i].tiempoSobrevividoMs = tiempoMs;

            // Efecto visual: salpicadura y anillo donde cayo.
            minijuego.efectoEliminacion[i] = 0.001f;
            minijuego.posicionEliminacion[i] = { jugadores[i].posicion.x, NIVEL_LAVA_COLOR + 0.06f, jugadores[i].posicion.z };
            EmitirSalpicaduraColorSeguro(minijuego, minijuego.posicionEliminacion[i], 0.6f, 14);

            jugadores[i].velocidad = {};
            jugadores[i].empuje = {};
            jugadores[i].tiempoRalentizado = 0.0f;
        }
    }

    ReproducirSonidoMinijuego(minijuego.audio, SONIDO_ELIMINADO);
    ActivarTemblorCamaraGeneral(0.12f, 0.22f);

    return eliminados;
}


static void RegistrarRondaSobrevividaColorSeguro(
    MinijuegoColorSeguro& minijuego
)
{
    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (
            minijuego.resultado.participantes[i].participo &&
            !minijuego.estadosJugadores[i].eliminado
        )
        {
            minijuego.estadosJugadores[i].rondasSobrevividas++;
        }
    }
}


static void FinalizarResultadoColorSeguro(
    MinijuegoColorSeguro& minijuego
)
{
    if (
        minijuego.resultado.estado !=
        RESULTADO_MINIJUEGO_EN_CURSO
    )
    {
        return;
    }

    int vivos = ContarJugadoresVivosColorSeguro(minijuego);

    minijuego.resultado.estado = RESULTADO_MINIJUEGO_FINALIZADO;
    minijuego.resultado.desenlace =
        vivos == 1
        ? DESENLACE_CON_GANADOR
        : DESENLACE_EMPATE;

    int tiempoFinalMs =
        (int)std::lround(
            minijuego.tiempoJugado * 1000.0f
        );

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        ResultadoParticipante& resultadoJugador =
            minijuego.resultado.participantes[i];

        if (!resultadoJugador.participo)
        {
            continue;
        }

        EstadoJugadorColorSeguro& estadoJugador =
            minijuego.estadosJugadores[i];

        if (!estadoJugador.eliminado)
        {
            estadoJugador.posicionFinal = 1;
            estadoJugador.tiempoSobrevividoMs = tiempoFinalMs;
        }

        resultadoJugador.posicionFinal = estadoJugador.posicionFinal;
        resultadoJugador.numeroEquipo = -1;
        resultadoJugador.puntuacionMinijuego =
            estadoJugador.tiempoSobrevividoMs;
        resultadoJugador.puntosObtenidos = 0;
    }

    minijuego.estado = COLOR_SEGURO_TERMINADO;
    ReproducirSonidoMinijuego(minijuego.audio, SONIDO_RESULTADO);
}


static void ActualizarDificultadColorSeguro(
    MinijuegoColorSeguro& minijuego
)
{
    float reduccion =
        (float)(minijuego.numeroRonda - 1) * 0.18f;

    minijuego.duracionElegirPlataforma = 5.0f - reduccion;
    minijuego.duracionCaidaPlataformas = 2.0f - reduccion * 0.22f;

    if (minijuego.duracionElegirPlataforma < 2.0f)
    {
        minijuego.duracionElegirPlataforma = 2.0f;
    }

    if (minijuego.duracionCaidaPlataformas < 1.10f)
    {
        minijuego.duracionCaidaPlataformas = 1.10f;
    }
}


static void ElegirNuevaPlataformaSegura(
    MinijuegoColorSeguro& minijuego
)
{
    int anterior = minijuego.indicePlataformaSegura;

    if (minijuego.cantidadPlataformas <= 1)
    {
        minijuego.indicePlataformaSegura = 0;
    }
    else
    {
        do
        {
            minijuego.indicePlataformaSegura =
                GetRandomValue(0, minijuego.cantidadPlataformas - 1);
        }
        while (minijuego.indicePlataformaSegura == anterior);
    }

    minijuego.fase = FASE_ELEGIR_PLATAFORMA;
    minijuego.tiempoFase = minijuego.duracionElegirPlataforma;

    // La TV late con el color nuevo (mismo instante que antes).
    minijuego.tiempoAnuncio = 0.0f;
}


static void TirarPlataformasIncorrectas(
    MinijuegoColorSeguro& minijuego
)
{
    for (int i = 0; i < minijuego.cantidadPlataformas; i++)
    {
        BloquePrueba& plataforma = minijuego.plataformas[i];

        if (i == minijuego.indicePlataformaSegura)
        {
            plataforma.activaColision = true;
            plataforma.cayendo = false;
        }
        else
        {
            plataforma.activaColision = true;
            plataforma.cayendo = true;
            plataforma.velocidadCaida = 0.0f;

            EmitirEscombrosColorSeguro(minijuego, plataforma.posicion);
        }
    }

    minijuego.fase = FASE_CAIDA_PLATAFORMAS;
    minijuego.tiempoFase = minijuego.duracionCaidaPlataformas;

    ReproducirSonidoMinijuego(minijuego.audio, SONIDO_PLATAFORMA);
    ActivarTemblorCamaraGeneral(0.10f, 0.40f);
}


// Caida de las plataformas; al tocar la lava salpican (solo visual).
static void ActualizarCaidaPlataformas(
    MinijuegoColorSeguro& minijuego,
    float deltaTime
)
{
    bool salpico = false;

    for (int i = 0; i < minijuego.cantidadPlataformas; i++)
    {
        BloquePrueba& plataforma = minijuego.plataformas[i];

        if (!plataforma.cayendo)
        {
            continue;
        }

        float topeAntes = plataforma.posicion.y + plataforma.tamano.y * 0.5f;

        plataforma.velocidadCaida += 12.0f * deltaTime;
        plataforma.posicion.y -= plataforma.velocidadCaida * deltaTime;

        float topeDespues = plataforma.posicion.y + plataforma.tamano.y * 0.5f;

        if (topeAntes > NIVEL_LAVA_COLOR && topeDespues <= NIVEL_LAVA_COLOR)
        {
            minijuego.salpicaduraPlataforma[i] = 0.001f;
            EmitirSalpicaduraColorSeguro(
                minijuego,
                { plataforma.posicion.x, NIVEL_LAVA_COLOR, plataforma.posicion.z },
                RADIO_HEXAGONO_COLOR,
                10
            );
            salpico = true;
        }
    }

    if (salpico)
    {
        ReproducirSonidoMinijuego(minijuego.audio, SONIDO_IMPACTO);
        ActivarTemblorCamaraGeneral(0.14f, 0.30f);
    }
}


// IA simple: al empezar cada ronda el bot decide (tras un breve retraso de
// reaccion) correr hacia el color indicado por la TV. Con una probabilidad
// que crece por ronda se equivoca de plataforma, para que haya ganador.
static InputMinijuegoParticipante CrearEntradaBotColorSeguro(
    MinijuegoColorSeguro& minijuego,
    int indice,
    const JugadorPrueba& jugador,
    float deltaTime
)
{
    EstadoJugadorColorSeguro& bot = minijuego.estadosJugadores[indice];

    if (bot.rondaBot != minijuego.numeroRonda)
    {
        bot.rondaBot = minijuego.numeroRonda;
        bot.retrasoBot = (float)GetRandomValue(120, 650) / 1000.0f;

        float probabilidadError =
            0.06f + 0.025f * (float)(minijuego.numeroRonda - 1);

        if (probabilidadError > 0.40f)
        {
            probabilidadError = 0.40f;
        }

        int destino = minijuego.indicePlataformaSegura;

        if (
            minijuego.cantidadPlataformas > 1 &&
            (float)GetRandomValue(0, 999) / 1000.0f < probabilidadError
        )
        {
            do
            {
                destino = GetRandomValue(0, minijuego.cantidadPlataformas - 1);
            }
            while (destino == minijuego.indicePlataformaSegura);
        }

        bot.objetivoBot = minijuego.plataformas[destino].posicion;
        bot.objetivoBot.x += (float)GetRandomValue(-800, 800) / 1000.0f;
        bot.objetivoBot.z += (float)GetRandomValue(-700, 700) / 1000.0f;
    }

    if (bot.retrasoBot > 0.0f)
    {
        bot.retrasoBot -= deltaTime;
        return InputMinijuegoParticipante{};
    }

    return CrearEntradaBotHaciaObjetivo1v3(
        jugador.posicion,
        bot.objetivoBot,
        0.25f
    );
}


static void ReiniciarEfectosColorSeguro(MinijuegoColorSeguro& m)
{
    m.tiempoAnimacion = 0.0f;
    m.tiempoAnuncio = 0.0f;
    m.acumuladorBrasas = 0.0f;
    m.acumuladorHumo = 0.0f;

    for (int i = 0; i < CANTIDAD_PLATAFORMAS_COLOR; i++)
    {
        m.salpicaduraPlataforma[i] = 0.0f;
        m.emergerPlataforma[i] = 0.0f;
    }

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        m.efectoEliminacion[i] = 0.0f;
    }

    for (int i = 0; i < MAX_PARTICULAS_COLOR_SEGURO; i++)
    {
        m.efectos[i].activa = false;
    }
}


// Avanza todo lo que es solo visual (tambien en la cuenta y en el final).
static void ActualizarEfectosColorSeguro(MinijuegoColorSeguro& m, float deltaTime)
{
    m.tiempoAnimacion += deltaTime;
    m.tiempoAnuncio += deltaTime;

    for (int i = 0; i < CANTIDAD_PLATAFORMAS_COLOR; i++)
    {
        if (m.salpicaduraPlataforma[i] > 0.0f)
        {
            m.salpicaduraPlataforma[i] += deltaTime;
            if (m.salpicaduraPlataforma[i] > 1.4f) m.salpicaduraPlataforma[i] = 0.0f;
        }

        if (m.emergerPlataforma[i] > 0.0f)
        {
            m.emergerPlataforma[i] -= deltaTime;
            if (m.emergerPlataforma[i] < 0.0f) m.emergerPlataforma[i] = 0.0f;
        }
    }

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (m.efectoEliminacion[i] > 0.0f)
        {
            m.efectoEliminacion[i] += deltaTime;
            if (m.efectoEliminacion[i] > 1.3f) m.efectoEliminacion[i] = 0.0f;
        }
    }

    ActualizarAmbienteColorSeguro(m, deltaTime);
    ActualizarParticulasColorSeguro(m, deltaTime);
}


//==================================================
// CICLO DEL MINIJUEGO
//==================================================

void MinijuegoColorSeguro::Inicializar()
{
    resultado = {};
    resultado.formato = FORMATO_MINIJUEGO_INDIVIDUAL;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        estadosJugadores[i] = {};
    }

    cantidadPlataformas = 0;

    const float distanciaHorizontal = 4.07032f;
    const float medioHorizontal = distanciaHorizontal / 2.0f;
    const float distanciaZ = RADIO_HEXAGONO_COLOR * 1.5f;

    Vector3 posiciones[CANTIDAD_PLATAFORMAS_COLOR] =
    {
        { 0.0f, 0.0f, 0.0f },
        { -medioHorizontal, 0.0f, -distanciaZ },
        { medioHorizontal, 0.0f, -distanciaZ },
        { distanciaHorizontal, 0.0f, 0.0f },
        { medioHorizontal, 0.0f, distanciaZ },
        { -medioHorizontal, 0.0f, distanciaZ },
        { -distanciaHorizontal, 0.0f, 0.0f }
    };

    for (int i = 0; i < CANTIDAD_PLATAFORMAS_COLOR; i++)
    {
        AgregarBloquePrueba(
            plataformas,
            cantidadPlataformas,
            CANTIDAD_PLATAFORMAS_COLOR,
            posiciones[i],
            Vector3{ 4.10f, 0.60f, 3.65f },
            ObtenerColorPlataforma(i)
        );
    }

    estado = COLOR_SEGURO_PREPARACION;
    fase = FASE_ELEGIR_PLATAFORMA;
    indicePlataformaSegura = -1;
    numeroRonda = 1;

    tiempoPreparacion = DURACION_PREPARACION_COLOR;

    // Se conserva el miembro por compatibilidad con el struct, pero ya no
    // funciona como cuenta regresiva ni puede finalizar la partida.
    tiempoRestante = 0.0f;
    tiempoJugado = 0.0f;

    duracionElegirPlataforma = 5.0f;
    duracionCaidaPlataformas = 2.0f;
    ActualizarDificultadColorSeguro(*this);
    tiempoFase = duracionElegirPlataforma;

    // Camara fija: deja libre el tendedero (arriba) y el pie de pantalla.
    posicionCamaraBase = { 0.0f, 12.0f, 15.0f };
    objetivoCamaraBase = { 0.0f, 0.2f, -2.2f };

    camara.position = posicionCamaraBase;
    camara.target = objetivoCamaraBase;
    camara.up = { 0.0f, 1.0f, 0.0f };
    camara.fovy = 55.0f;
    camara.projection = CAMERA_PERSPECTIVE;

    ReiniciarEfectosColorSeguro(*this);
    ElegirNuevaPlataformaSegura(*this);
}


void MinijuegoColorSeguro::ConfigurarJugadores(
    JugadorPrueba jugadores[],
    int cantidadMaxima
) const
{
    Vector3 spawns[MAX_JUGADORES_PRUEBA] =
    {
        { -0.65f, 1.0f, 0.55f },
        { 0.65f, 1.0f, 0.55f },
        { -0.65f, 1.0f, -0.55f },
        { 0.65f, 1.0f, -0.55f }
    };

    int limite =
        cantidadMaxima < MAX_JUGADORES_PRUEBA
        ? cantidadMaxima
        : MAX_JUGADORES_PRUEBA;

    for (int i = 0; i < limite; i++)
    {
        ConfigurarJugadorMinijuegoEstandar(
            jugadores[i],
            spawns[i]
        );
    }
}


void MinijuegoColorSeguro::Reiniciar(
    JugadorPrueba jugadores[],
    int cantidadMaxima
)
{
    resultado = {};
    resultado.formato = FORMATO_MINIJUEGO_INDIVIDUAL;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        estadosJugadores[i] = {};
    }

    ReiniciarBloquesPrueba(plataformas, cantidadPlataformas);

    numeroRonda = 1;
    estado = COLOR_SEGURO_PREPARACION;
    fase = FASE_ELEGIR_PLATAFORMA;
    indicePlataformaSegura = -1;
    tiempoPreparacion = DURACION_PREPARACION_COLOR;
    tiempoRestante = 0.0f;
    tiempoJugado = 0.0f;

    duracionElegirPlataforma = 5.0f;
    duracionCaidaPlataformas = 2.0f;
    ActualizarDificultadColorSeguro(*this);
    ReiniciarEfectosColorSeguro(*this);
    ElegirNuevaPlataformaSegura(*this);

    camara.position = posicionCamaraBase;
    camara.target = objetivoCamaraBase;

    for (int i = 0; i < cantidadMaxima; i++)
    {
        ReiniciarJugadorPrueba(jugadores[i]);
    }
}


void MinijuegoColorSeguro::Actualizar(
    float deltaTime,
    JugadorPrueba jugadores[],
    int cantidadMaxima,
    Participante participantes[],
    ParticulaTierra particulas[],
    int cantidadParticulas
)
{
    if (resultado.cantidadParticipantes == 0)
    {
        InicializarResultadoColorSeguro(*this, participantes);
    }

    ActualizarEfectosColorSeguro(*this, deltaTime);

    if (estado == COLOR_SEGURO_TERMINADO)
    {
        // Las plataformas que ya caian terminan de hundirse.
        if (fase == FASE_CAIDA_PLATAFORMAS)
        {
            ActualizarCaidaPlataformas(*this, deltaTime);
        }

        return;
    }

    if (estado == COLOR_SEGURO_PREPARACION)
    {
        for (int i = 0; i < MAX_PARTICIPANTES; i++)
        {
            jugadores[i].velocidad = {};
            jugadores[i].empuje = {};
        }

        float antes = tiempoPreparacion;
        tiempoPreparacion -= deltaTime;
        ActualizarAudioCuentaRegresiva(audio, antes, tiempoPreparacion);

        if (tiempoPreparacion <= 0.0f)
        {
            tiempoPreparacion = 0.0f;
            estado = COLOR_SEGURO_JUGANDO;
            tiempoAnuncio = 0.0f;
        }

        return;
    }

    tiempoJugado += deltaTime;

    float faseAntes = tiempoFase;
    tiempoFase -= deltaTime;

    if (fase == FASE_ELEGIR_PLATAFORMA)
    {
        // Tic de alerta en los ultimos 3 segundos para elegir.
        ActualizarAudioAlertaTiempo(audio, faseAntes, tiempoFase, 3.0f);
    }

    if (
        fase == FASE_ELEGIR_PLATAFORMA &&
        tiempoFase <= 0.0f
    )
    {
        TirarPlataformasIncorrectas(*this);
    }

    if (fase == FASE_CAIDA_PLATAFORMAS)
    {
        ActualizarCaidaPlataformas(*this, deltaTime);
    }

    int vivosAntes = ContarJugadoresVivosColorSeguro(*this);

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        JugadorPrueba& jugador = jugadores[i];

        if (
            !resultado.participantes[i].participo ||
            estadosJugadores[i].eliminado
        )
        {
            jugador.cayendo = true;
            jugador.velocidad = {};
            jugador.empuje = {};
            jugador.tiempoRalentizado = 0.0f;
            continue;
        }

        InputMinijuegoParticipante entrada{};

        if (participantes[i].esBot || !participantes[i].conectado)
        {
            entrada = CrearEntradaBotColorSeguro(
                *this,
                i,
                jugador,
                deltaTime
            );
        }
        else if (participantes[i].activo)
        {
            entrada = LeerInputMinijuegoParticipante(participantes[i]);
        }

        ActualizarJugadorPruebaNormal(
            jugador,
            entrada,
            plataformas,
            cantidadPlataformas,
            particulas,
            cantidadParticulas,
            true,
            false,
            deltaTime
        );
    }

    // El temblor del ground pound lo aplica el sistema comun.
    ResolverGolpesSuelo(
        jugadores,
        participantes,
        cantidadMaxima,
        DURACION_APLASTADO_GROUND_POUND_COLOR
    );

    ResolverGolpesJugadoresConEfectos(
        jugadores,
        participantes,
        cantidadMaxima,
        particulas,
        cantidadParticulas
    );

    ResolverColisionesJugadoresSinEmpuje(
        jugadores,
        participantes,
        cantidadMaxima
    );

    int eliminados = RegistrarEliminacionesColorSeguro(
        *this,
        jugadores,
        vivosAntes
    );

    int vivosDespues = vivosAntes - eliminados;

    if (vivosDespues <= 1)
    {
        FinalizarResultadoColorSeguro(*this);
        return;
    }

    if (
        fase == FASE_CAIDA_PLATAFORMAS &&
        tiempoFase <= 0.0f
    )
    {
        RegistrarRondaSobrevividaColorSeguro(*this);

        // Las plataformas caidas vuelven a emerger de la lava.
        for (int i = 0; i < cantidadPlataformas; i++)
        {
            if (plataformas[i].cayendo)
            {
                emergerPlataforma[i] = DURACION_EMERGER_COLOR;
                EmitirSalpicaduraColorSeguro(
                    *this,
                    { plataformas[i].posicion.x, NIVEL_LAVA_COLOR, plataformas[i].posicion.z },
                    RADIO_HEXAGONO_COLOR * 0.8f,
                    5
                );
            }
        }

        ReiniciarBloquesPrueba(plataformas, cantidadPlataformas);
        numeroRonda++;

        ActualizarDificultadColorSeguro(*this);
        ElegirNuevaPlataformaSegura(*this);
        ReproducirSonidoMinijuego(audio, SONIDO_ACIERTO);
    }
}


//==================================================
// DIBUJO
//==================================================

static void DibujarOndasColorSeguro(const MinijuegoColorSeguro& m)
{
    for (int i = 0; i < m.cantidadPlataformas; i++)
    {
        float s = m.salpicaduraPlataforma[i];
        if (s <= 0.0f) continue;

        float u = s / 1.4f;
        Vector3 c = { m.plataformas[i].posicion.x, NIVEL_LAVA_COLOR + 0.07f, m.plataformas[i].posicion.z };
        float r = RADIO_HEXAGONO_COLOR * (0.9f + 1.3f * SuavizarColorSeguro(u));
        AnilloColorSeguro(c, r, r + 0.35f * (1.0f - u) + 0.08f, Color{ 255, 196, 90, (unsigned char)(200.0f * (1.0f - u)) });
        AnilloColorSeguro(c, r * 0.6f, r * 0.6f + 0.2f, Color{ 255, 140, 50, (unsigned char)(150.0f * (1.0f - u)) });
    }

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        float e = m.efectoEliminacion[i];
        if (e <= 0.0f) continue;

        float u = e / 1.3f;
        float r = 0.3f + 2.2f * SuavizarColorSeguro(u);
        AnilloColorSeguro(m.posicionEliminacion[i], r, r + 0.22f, Color{ 236, 52, 40, (unsigned char)(230.0f * (1.0f - u)) });
        AnilloColorSeguro(m.posicionEliminacion[i], r * 0.5f, r * 0.5f + 0.14f, Color{ 255, 170, 70, (unsigned char)(180.0f * (1.0f - u)) });
    }
}


static void DibujarResplandoresColorSeguro(const MinijuegoColorSeguro& m, float t)
{
    // Calor general del lago, latiendo despacio.
    ResplandorSueloColorSeguro(0.0f, NIVEL_LAVA_COLOR + 0.1f, -1.0f, 15.0f, Color{ 255, 120, 40, (unsigned char)(46.0f + 12.0f * std::sin(t * 1.3f)) });

    for (int c = 0; c < CantidadCascadasColorSeguro(); c++)
    {
        Vector3 pie = PuntoCascadaColorSeguro(ThetaCascadaColorSeguro(c), NIVEL_LAVA_COLOR + 0.1f);
        float pulso = 0.8f + 0.2f * std::sin(t * 4.0f + (float)c * 1.7f);
        ResplandorSueloColorSeguro(pie.x * 0.97f, pie.y, pie.z * 0.97f, 4.2f * pulso, Color{ 255, 150, 50, 80 });

        Vector3 pared = PuntoCascadaColorSeguro(ThetaCascadaColorSeguro(c), 2.4f);
        float th = ThetaCascadaColorSeguro(c);
        ResplandorColorSeguro(pared, { -std::sin(th), 0.0f, std::cos(th) }, { 0.0f, 1.0f, 0.0f }, 3.6f * pulso, Color{ 255, 120, 40, 46 });
    }

    // Pulso de luz del color anunciado en la TV (solo alrededor de la TV).
    Color colorTV = ObtenerColorPlataforma(m.indicePlataformaSegura);
    float pulso = PulsoAnuncioColorSeguro(m);
    float base = 0.30f + 0.06f * std::sin(t * 3.0f);
    float fuerza = base + 0.70f * pulso;
    Vector3 frente = { CENTRO_TV_COLOR.x, CENTRO_TV_COLOR.y, CENTRO_TV_COLOR.z + FONDO_TV_COLOR * 0.5f + 0.1f };

    ResplandorColorSeguro(frente, { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, 4.6f + 1.6f * pulso, AlfaColorSeguro(colorTV, 0.38f * fuerza));
    ResplandorSueloColorSeguro(0.0f, NIVEL_LAVA_COLOR + 0.12f, CENTRO_TV_COLOR.z + 1.4f, 5.5f + 2.0f * pulso, AlfaColorSeguro(colorTV, 0.42f * fuerza));

    for (int lado = -1; lado <= 1; lado += 2)
    {
        Vector3 lampara = { (ANCHO_TV_COLOR * 0.5f - 0.35f) * (float)lado, CENTRO_TV_COLOR.y + ALTO_TV_COLOR * 0.5f + 0.18f, CENTRO_TV_COLOR.z + 0.4f };
        ResplandorColorSeguro(lampara, { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, 0.9f + 0.8f * pulso, AlfaColorSeguro(colorTV, 0.5f * fuerza));
    }

    // Salpicaduras de las plataformas: fogonazo breve.
    for (int i = 0; i < m.cantidadPlataformas; i++)
    {
        float s = m.salpicaduraPlataforma[i];
        if (s <= 0.0f) continue;
        float u = s / 1.4f;
        ResplandorSueloColorSeguro(m.plataformas[i].posicion.x, NIVEL_LAVA_COLOR + 0.14f, m.plataformas[i].posicion.z, 4.0f, Color{ 255, 170, 70, (unsigned char)(150.0f * (1.0f - u)) });
    }

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        float e = m.efectoEliminacion[i];
        if (e <= 0.0f) continue;
        float u = e / 1.3f;
        Vector3 p = m.posicionEliminacion[i];
        ResplandorSueloColorSeguro(p.x, p.y + 0.08f, p.z, 2.6f, Color{ 255, 90, 50, (unsigned char)(170.0f * (1.0f - u)) });
    }
}


void MinijuegoColorSeguro::Dibujar(
    const JugadorPrueba jugadores[],
    int cantidadMaxima,
    const Participante participantes[],
    const ParticulaTierra particulas[],
    int cantidadParticulas,
    bool mostrarDebug
) const
{
    (void)cantidadMaxima;
    const float t = tiempoAnimacion;

    // Escenario propio: sin el tema de lava global.
    ClearBackground(Color{ 40, 18, 16, 255 });

    // Intro de la cuenta: la camara baja hasta su lugar fijo.
    Camera3D vista = camara;
    if (estado == COLOR_SEGURO_PREPARACION)
    {
        float u = 1.0f - tiempoPreparacion / DURACION_PREPARACION_COLOR;
        float resto = (1.0f - SuavizarColorSeguro(u * 1.15f)) * ObtenerFactorMovimientoCamaraMinijuegos();
        vista.position.y += 3.0f * resto;
        vista.position.z += 3.5f * resto;
    }

    BeginMode3D(vista);

    // Geometria hecha a mano: sin descarte de caras traseras.
    rlDrawRenderBatchActive();
    rlDisableBackfaceCulling();

    DibujarParedesCraterColorSeguro();
    DibujarCascadasLavaColorSeguro(t);
    DibujarLagoLavaColorSeguro(t);
    DibujarCostrasLavaColorSeguro(t);
    DibujarBurbujasLavaColorSeguro(t);
    DibujarColumnasBasaltoColorSeguro();
    DibujarTelevisorColorSeguro(*this, t);

    for (int i = 0; i < cantidadPlataformas; i++)
    {
        DibujarPlataformaColorSeguro(*this, i);

        if (mostrarDebug)
        {
            DrawBoundingBox(
                CrearHitboxBloquePrueba(plataformas[i]),
                YELLOW
            );
        }
    }

    DibujarPrimerPlanoColorSeguro();

    rlDrawRenderBatchActive();
    rlEnableBackfaceCulling();

    DibujarParticulasTierra(particulas, cantidadParticulas);
    DibujarParticulasSolidasColorSeguro(*this);

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (estadosJugadores[i].eliminado)
        {
            continue;
        }

        DibujarJugadorCuboPrueba(jugadores[i], participantes[i]);

        if (participantes[i].activo)
        {
            DibujarIndicadorJugadorColorSeguro(
                jugadores[i],
                participantes[i].color,
                t
            );
        }

        if (
            mostrarDebug &&
            participantes[i].activo &&
            participantes[i].conectado &&
            !jugadores[i].cayendo
        )
        {
            DrawBoundingBox(
                CrearHitboxJugadorPrueba(jugadores[i]),
                LIME
            );
        }
    }

    // Translucidos sin escribir profundidad: humo y ondas.
    rlDrawRenderBatchActive();
    rlDisableDepthMask();
    rlDisableBackfaceCulling();
    DibujarHumoColorSeguro(*this);
    DibujarOndasColorSeguro(*this);
    rlDrawRenderBatchActive();

    // Luz falsa aditiva: calor del lago, cascadas, TV y brasas.
    if (FactorCalidadGrafica(CalidadEfectos()) >= 0.5f)
    {
        BeginBlendMode(BLEND_ADDITIVE);
        DibujarResplandoresColorSeguro(*this, t);
        DibujarBrasasColorSeguro(*this);
        rlDrawRenderBatchActive();
        EndBlendMode();
    }
    else
    {
        DibujarBrasasColorSeguro(*this);
        rlDrawRenderBatchActive();
    }

    rlEnableBackfaceCulling();
    rlEnableDepthMask();

    EndMode3D();

    // Anuncio del color bajo la TV (mismo instante que la pantalla).
    if (estado != COLOR_SEGURO_TERMINADO && indicePlataformaSegura >= 0)
    {
        Vector2 pos = GetWorldToScreen(
            { CENTRO_TV_COLOR.x, CENTRO_TV_COLOR.y - ALTO_TV_COLOR * 0.5f - 0.45f, CENTRO_TV_COLOR.z + 0.5f },
            vista
        );

        char texto[32];
        std::snprintf(texto, sizeof(texto), "¡%s!", NombreColorPlataforma(indicePlataformaSegura));

        bool recien = estado == COLOR_SEGURO_JUGANDO && tiempoAnuncio < 0.45f;
        DibujarTextoMundoContornoUI(
            texto,
            pos.x,
            pos.y,
            recien ? UI_TEXTO_TITULO : UI_TEXTO_GRANDE,
            ObtenerColorPlataforma(indicePlataformaSegura)
        );
    }

    // HUD comun (Minigames/HudMinijuego): solo lee el estado.
    DatosHudMinijuego hud;
    PrepararDatosHudMinijuego(
        hud,
        "COLOR SEGURO",
        "CORRÉ AL COLOR DE LA TV: LAS DEMÁS SE HUNDEN. SIN LIMITE, CADA RONDA ES MÁS RÁPIDA.",
        resultado
    );

    hud.fase = estado == COLOR_SEGURO_PREPARACION
        ? HUD_MINIJUEGO_CUENTA
        : (estado == COLOR_SEGURO_JUGANDO ? HUD_MINIJUEGO_JUGANDO : HUD_MINIJUEGO_FIN);
    hud.cuentaRestante = tiempoPreparacion;

    // Sin limite de tiempo: el cartel central muestra la ronda y el
    // tiempo que queda para llegar al color (vacio mientras caen).
    hud.reloj = HUD_RELOJ_MEDIDOR;
    hud.etiquetaMedidor = "RONDA";
    std::snprintf(hud.valorMedidor, sizeof(hud.valorMedidor), "%d", numeroRonda);
    hud.progresoMedidor =
        fase == FASE_ELEGIR_PLATAFORMA && duracionElegirPlataforma > 0.0f
        ? Limitar01ColorSeguro(tiempoFase / duracionElegirPlataforma)
        : 0.0f;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        JugadorHudMinijuego& jugador = hud.jugadores[i];
        if (!jugador.visible) continue;

        const EstadoJugadorColorSeguro& estadoJugador = estadosJugadores[i];
        jugador.eliminado = estadoJugador.eliminado;

        int rondas = estadoJugador.rondasSobrevividas;
        std::snprintf(jugador.valor, sizeof(jugador.valor), "%d", rondas);
        jugador.valorNumerico = rondas;

        if (!participantes[i].esBot && !participantes[i].conectado && !estadoJugador.eliminado)
        {
            std::snprintf(jugador.estado, sizeof(jugador.estado), "SIN CONTROL");
        }

        if (!participantes[i].esBot)
        {
            std::snprintf(
                jugador.controles,
                sizeof(jugador.controles),
                "MOVÉ: %s  GOLPE: %s\nAIRE+%s: APLASTAR",
                TextoMoverHud(participantes[i]),
                TextoBotonGolpeHud(participantes[i]),
                ObtenerTextoBotonPrincipal(participantes[i])
            );
        }

        // Tiempo sobrevivido con coma decimal: "12,3 S".
        std::snprintf(
            jugador.detalleFinal,
            sizeof(jugador.detalleFinal),
            "%d,%d S",
            resultado.participantes[i].puntuacionMinijuego / 1000,
            (resultado.participantes[i].puntuacionMinijuego % 1000) / 100
        );
    }

    // Tablon final: quien sigue en pie termina con el mismo tiempo que el
    // ultimo en caer, y los que caen juntos comparten puesto.
    if (hud.fase == HUD_MINIJUEGO_FIN && resultado.estado == RESULTADO_MINIJUEGO_FINALIZADO)
    {
        for (int i = 0; i < MAX_PARTICIPANTES; i++)
        {
            if (!hud.jugadores[i].visible) continue;

            const ResultadoParticipante& yo = resultado.participantes[i];

            for (int j = 0; j < MAX_PARTICIPANTES; j++)
            {
                if (j == i || !hud.jugadores[j].visible) continue;

                const ResultadoParticipante& otro = resultado.participantes[j];
                if (otro.puntuacionMinijuego / 100 != yo.puntuacionMinijuego / 100) continue;

                if (otro.posicionFinal == yo.posicionFinal)
                {
                    if (hud.notaFinal == nullptr)
                    {
                        hud.notaFinal = "CAYERON A LA VEZ: COMPARTEN EL PUESTO";
                    }
                    continue;
                }

                if (!estadosJugadores[i].eliminado || !estadosJugadores[j].eliminado)
                {
                    std::snprintf(
                        hud.jugadores[i].detalleDesempate,
                        sizeof(hud.jugadores[i].detalleDesempate),
                        "%s",
                        estadosJugadores[i].eliminado ? "CAYÓ" : "EN PIE"
                    );
                    hud.notaFinal = "MISMO TIEMPO: QUEDA ARRIBA QUIEN SIGUE EN PIE";
                }
                else
                {
                    // Dos caidas en la misma decima: las centesimas dan el orden.
                    std::snprintf(
                        hud.jugadores[i].detalleFinal,
                        sizeof(hud.jugadores[i].detalleFinal),
                        "%d,%02d S",
                        yo.puntuacionMinijuego / 1000,
                        (yo.puntuacionMinijuego % 1000) / 10
                    );
                }
            }
        }
    }

    DibujarHudMinijuego(hud, participantes);
}


const ResultadoMinijuego&
MinijuegoColorSeguro::ObtenerResultado() const
{
    return resultado;
}
