#include "UI/SeleccionPersonajes3D.h"

#include "Core/RecursosJuego.h"
#include "Minigames/ModeloJugadorCompartido.h"
#include "Systems/CalidadGrafica.h"

#include "raymath.h"
#include "rlgl.h"

#include <cmath>
// Esta escena no es un minijuego, pero los headers de minijuegos redefinen
// funciones de raylib con macros que arrastran estado global:
//  - DrawSphere/DrawCube/DrawCylinder/DrawModelEx estampan sombras de
//    suelo (manchas gigantes en banderines, confeti y personajes).
//  - BeginMode3D dibuja el tema del ultimo minijuego (lava, nieve...) y
//    aplica el temblor de camara pendiente.
// Aqui se usan siempre las funciones originales de raylib.
#undef DrawSphere
#undef DrawCube
#undef DrawCylinder
#undef DrawModelEx
#undef BeginMode3D


//==================================================
// CONSTANTES DE LA ESCENA
//==================================================

static const float SEPARACION_PEDESTALES = 3.2f;
static const float ALTURA_PEDESTAL = 0.55f;
static const float RADIO_PEDESTAL = 1.15f;

// Playa: arena a esta altura alrededor de la tarima, dunas detras y mar
// por debajo de la arena.
static const float ALTURA_ARENA = -0.70f;
static const float NIVEL_MAR = -1.15f;

// Escala del modelo: la de partida, para que el roster sea reconocible.
static const float ESCALA_BASE_PERSONAJE =
    ESCALA_MODELO_JUGADOR_3D;

// Altura medida del modelo con ESCALA_BASE_PERSONAJE (en unidades).
static const float ALTURA_MODELO_ESCALA_BASE = 2.27f;

// Realce del personaje apuntado (leve).
static const float REALCE_FOCO = 0.10f;

static const Color PALETA_FIESTA[6] =
{
    Color{ 255, 90, 90, 255 },
    Color{ 255, 190, 60, 255 },
    Color{ 120, 220, 110, 255 },
    Color{ 90, 190, 255, 255 },
    Color{ 190, 120, 255, 255 },
    Color{ 255, 130, 200, 255 }
};


//==================================================
// UTILIDADES
//==================================================

static float Hash01(int semilla)
{
    unsigned int x = (unsigned int)semilla * 2654435761u;
    x ^= x >> 15;
    x *= 2246822519u;
    x ^= x >> 13;

    return (float)(x & 0xFFFFu) / 65535.0f;
}


static Color MezclarColor(Color a, Color b, float t)
{
    return Color{
        (unsigned char)(a.r + (b.r - a.r) * t),
        (unsigned char)(a.g + (b.g - a.g) * t),
        (unsigned char)(a.b + (b.b - a.b) * t),
        255
    };
}


static Color Oscurecer(Color c, float factor)
{
    return Color{
        (unsigned char)(c.r * factor),
        (unsigned char)(c.g * factor),
        (unsigned char)(c.b * factor),
        c.a
    };
}


static float AlturaModelo(float escala)
{
    return ALTURA_MODELO_ESCALA_BASE * escala / ESCALA_BASE_PERSONAJE;
}


//==================================================
// CAMARA
//==================================================

Camera3D ObtenerCamaraEscenaSeleccion(
    float tiempo
)
{
    Camera3D camara = {};

    // Vaiven minimo: da vida sin mover la composicion.
    camara.position = Vector3{
        std::sin(tiempo * 0.25f) * 0.18f,
        3.3f + std::sin(tiempo * 0.18f) * 0.05f,
        11.0f
    };

    camara.target = Vector3{ 0.0f, 1.55f, 0.0f };
    camara.up = Vector3{ 0.0f, 1.0f, 0.0f };
    camara.fovy = 38.0f;
    camara.projection = CAMERA_PERSPECTIVE;

    return camara;
}


Vector3 ObtenerPosicionPedestalSeleccion(
    int indice
)
{
    float centrado =
        (float)indice - (ESCENA_SELECCION_PEDESTALES - 1) * 0.5f;

    return Vector3{ centrado * SEPARACION_PEDESTALES, 0.0f, 0.0f };
}


float ObtenerAlturaCabezaSeleccion(
    const PersonajeEscena3D& personaje
)
{
    float escala =
        ESCALA_BASE_PERSONAJE * (1.0f + REALCE_FOCO * personaje.foco);

    return
        ALTURA_PEDESTAL +
        AlturaModelo(escala) +
        (personaje.foco * 0.06f);
}


//==================================================
// CIELO (2D, detras de todo)
//==================================================

static void DibujarCielo(float tiempo, Camera3D camara)
{
    int ancho = GetScreenWidth();
    int alto = GetScreenHeight();

    DrawRectangleGradientV(
        0,
        0,
        ancho,
        alto,
        Color{ 52, 36, 112, 255 },
        Color{ 255, 150, 104, 255 }
    );

    // Sol poniente apoyado en el horizonte del mar: se calcula la altura
    // del horizonte en pantalla y el mar 3D (que se dibuja despues) tapa
    // su parte inferior. Halo con degradado suave, sin borde duro.
    float horizonteY =
        GetWorldToScreen(Vector3{ 0.0f, NIVEL_MAR, -250.0f }, camara).y;

    int solX = ancho * 64 / 100;
    int radio = alto / 11;
    int solY = (int)horizonteY - radio / 3;

    DrawCircleGradient(
        Vector2{ (float)solX, (float)solY },
        (float)radio * 2.6f,
        Fade(Color{ 255, 205, 150, 255 }, 0.35f),
        Fade(Color{ 255, 170, 120, 255 }, 0.0f)
    );

    DrawCircle(solX, solY, (float)radio, Color{ 255, 224, 172, 255 });

    (void)tiempo;
}


//==================================================
// BANDERINES
//==================================================

static void DibujarGuirnalda(
    Vector3 inicio,
    Vector3 fin,
    float caida,
    int cantidad,
    float tiempo,
    int semilla
)
{
    Vector3 anterior = inicio;

    for (int i = 1; i <= cantidad; i++)
    {
        float t = (float)i / cantidad;

        Vector3 punto = Vector3Lerp(inicio, fin, t);
        punto.y -= caida * 4.0f * t * (1.0f - t);

        DrawLine3D(anterior, punto, Color{ 60, 40, 50, 255 });

        // Banderin colgante (se dibuja por ambos lados).
        float ondulacion = std::sin(tiempo * 1.6f + i * 0.9f) * 0.05f;
        Color color = PALETA_FIESTA[(i + semilla) % 6];

        Vector3 a = Vector3{ punto.x - 0.14f, punto.y, punto.z };
        Vector3 b = Vector3{ punto.x + 0.14f, punto.y, punto.z };
        Vector3 c = Vector3{ punto.x + ondulacion, punto.y - 0.34f, punto.z };

        DrawTriangle3D(a, c, b, color);
        DrawTriangle3D(a, b, c, Oscurecer(color, 0.8f));

        anterior = punto;
    }
}


static void DibujarPostesYGuirnalda(float tiempo)
{
    for (int lado = -1; lado <= 1; lado += 2)
    {
        float x = lado * 7.4f;

        // Poste clavado en la arena (base enterrada bajo el suelo).
        DrawCylinder(Vector3{ x, ALTURA_ARENA - 0.25f, -2.2f }, 0.14f, 0.18f, 5.9f, 8, Color{ 130, 84, 60, 255 });
        DrawSphere(Vector3{ x, 5.1f, -2.2f }, 0.22f, Color{ 255, 220, 120, 255 });
    }

    DibujarGuirnalda(
        Vector3{ -7.4f, 4.9f, -2.2f },
        Vector3{ 7.4f, 4.9f, -2.2f },
        0.8f,
        18,
        tiempo,
        0
    );
}


//==================================================
// TARIMA Y PEDESTALES
//==================================================

// Textura de granos de arena generada por codigo (sin archivos nuevos).
// Es casi blanca: se multiplica por el color de cada vertice del terreno.
static Texture2D texturaArena = {};
static Texture2D texturaMadera = {};

static void CargarTexturaArena()
{
    if (texturaArena.id != 0)
    {
        return;
    }

    const int lado = 128;
    Image imagen = GenImageColor(lado, lado, Color{ 246, 246, 246, 255 });

    for (int y = 0; y < lado; y++)
    {
        for (int x = 0; x < lado; x++)
        {
            float azar = Hash01(y * lado + x + 17);
            unsigned char valor = 246;

            if (azar < 0.16f)
            {
                valor = (unsigned char)(196 + Hash01(x * 31 + y * 57) * 30.0f);
            }
            else if (azar > 0.95f)
            {
                valor = 255;
            }

            ImageDrawPixel(&imagen, x, y, Color{ valor, valor, valor, 255 });
        }
    }

    texturaArena = LoadTextureFromImage(imagen);
    UnloadImage(imagen);

    GenTextureMipmaps(&texturaArena);
    SetTextureFilter(texturaArena, TEXTURE_FILTER_TRILINEAR);
    SetTextureWrap(texturaArena, TEXTURE_WRAP_REPEAT);
}


void DescargarEscenaSeleccion()
{
    if (texturaArena.id != 0)
    {
        UnloadTexture(texturaArena);
        texturaArena = {};
    }

    if (texturaMadera.id != 0)
    {
        UnloadTexture(texturaMadera);
        texturaMadera = {};
    }
}


static float PasoSuave(float t)
{
    t = Clamp(t, 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}


// Altura de la arena: plana alrededor de la tarima, una cresta de dunas
// detras y orilla que se hunde bajo el mar hacia el fondo y los costados.
static float AlturaTerreno(float x, float z)
{
    float subida = PasoSuave((-z - 3.0f) / 6.0f);
    float bajada = PasoSuave((-z - 9.0f) / 6.0f);
    float orilla = PasoSuave((std::fabs(x) - 13.0f) / 8.0f);

    float amplitud =
        1.1f *
        (0.8f + 0.2f * std::sin(0.35f * x + 1.0f) + 0.1f * std::sin(0.8f * x));

    float altura =
        ALTURA_ARENA +
        amplitud * subida -
        (amplitud + 1.0f) * bajada;

    // Rizado suave del viento, solo donde hay duna.
    altura +=
        0.03f *
        std::sin(z * 2.6f + 1.7f * std::sin(x * 0.45f)) *
        subida * (1.0f - bajada);

    return altura + (ALTURA_ARENA - 1.0f - altura) * orilla;
}


static void VerticeArena(float x, float z)
{
    float y = AlturaTerreno(x, z);

    // Sombreado por pendiente (luz del sol poniente) y arena mojada cerca
    // del agua.
    float dx = AlturaTerreno(x + 0.3f, z) - AlturaTerreno(x - 0.3f, z);
    float dz = AlturaTerreno(x, z + 0.3f) - AlturaTerreno(x, z - 0.3f);
    Vector3 normal = Vector3Normalize(Vector3{ -dx, 0.6f, -dz });
    Vector3 luz = Vector3Normalize(Vector3{ 0.45f, 0.8f, -0.4f });
    float brillo = 0.80f + 0.30f * Clamp(Vector3DotProduct(normal, luz), 0.0f, 1.0f);

    Color seca = Color{ 238, 214, 158, 255 };
    Color mojada = Color{ 184, 156, 112, 255 };
    Color color = MezclarColor(mojada, seca, PasoSuave((y - NIVEL_MAR) / 0.45f));

    rlColor4ub(
        (unsigned char)Clamp(color.r * brillo, 0.0f, 255.0f),
        (unsigned char)Clamp(color.g * brillo, 0.0f, 255.0f),
        (unsigned char)Clamp(color.b * brillo, 0.0f, 255.0f),
        255
    );
    rlTexCoord2f(x * 0.35f, z * 0.35f);
    rlVertex3f(x, y, z);
}


static void DibujarArena()
{
    CargarTexturaArena();

    const float paso = 0.75f;
    const float xMin = -24.0f;
    const float zMax = 10.0f;
    const int columnas = 64;
    const int filas = 36;

    for (int f = 0; f < filas; f++)
    {
        float z0 = zMax - f * paso;
        float z1 = z0 - paso;

        for (int c = 0; c < columnas; c++)
        {
            float x0 = xMin + c * paso;
            float x1 = x0 + paso;

            rlCheckRenderBatchLimit(4);
            rlSetTexture(texturaArena.id);
            rlBegin(RL_QUADS);
            rlNormal3f(0.0f, 1.0f, 0.0f);
            VerticeArena(x0, z0);
            VerticeArena(x1, z0);
            VerticeArena(x1, z1);
            VerticeArena(x0, z1);
            rlEnd();
            rlSetTexture(0);
        }
    }
}


static void FranjaMar(float zCerca, float zLejos, Color cerca, Color lejos)
{
    const float ancho = 300.0f;

    rlCheckRenderBatchLimit(4);
    rlBegin(RL_QUADS);
    rlNormal3f(0.0f, 1.0f, 0.0f);
    rlColor4ub(cerca.r, cerca.g, cerca.b, 255);
    rlVertex3f(-ancho, NIVEL_MAR, zCerca);
    rlVertex3f(ancho, NIVEL_MAR, zCerca);
    rlColor4ub(lejos.r, lejos.g, lejos.b, 255);
    rlVertex3f(ancho, NIVEL_MAR, zLejos);
    rlVertex3f(-ancho, NIVEL_MAR, zLejos);
    rlEnd();
}


static void DibujarMar(float tiempo)
{
    // Turquesa junto a la orilla que se vuelve azul profundo en el horizonte.
    Color orilla = Color{ 74, 176, 214, 255 };
    Color medio = Color{ 44, 128, 204, 255 };
    Color fondo = Color{ 34, 92, 178, 255 };

    FranjaMar(30.0f, -30.0f, orilla, medio);
    FranjaMar(-30.0f, -260.0f, medio, fondo);

    // Destellos que derivan sobre el agua; crecen con la distancia para
    // seguir viendose en el horizonte.
    for (int i = 0; i < 70; i++)
    {
        float z = -22.0f - Hash01(i * 5 + 1) * 110.0f;
        float deriva = std::fmod(Hash01(i * 9 + 2) * 200.0f + tiempo * 0.5f, 200.0f);
        float x = deriva - 100.0f;
        float largo = 0.9f + (-z) * 0.07f;
        float brillo = 0.35f + 0.25f * std::sin(tiempo * 1.7f + i);

        DrawCube(
            Vector3{ x, NIVEL_MAR + 0.01f, z },
            largo,
            0.01f,
            largo * 0.06f,
            Fade(Color{ 255, 236, 200, 255 }, brillo)
        );
    }
}


// Textura de tablones generada por codigo: 4 tablones horizontales con
// veta, juntas, nudos y clavos. Es clara: se tine con el color del vertice.

static void CargarTexturaMadera()
{
    if (texturaMadera.id != 0)
    {
        return;
    }

    const int lado = 128;
    const int altoTablon = 32;
    Image imagen = GenImageColor(lado, lado, WHITE);

    for (int y = 0; y < lado; y++)
    {
        int tablon = y / altoTablon;
        int fila = y % altoTablon;
        float tonoTablon = 0.88f + 0.12f * Hash01(tablon * 13 + 5);

        // Junta vertical propia de cada tablon.
        int junta = 20 + (int)(Hash01(tablon * 29 + 3) * 88.0f);

        for (int x = 0; x < lado; x++)
        {
            // Veta: ondas largas en x que se deforman con la fila.
            float veta =
                0.5f +
                0.5f * std::sin(x * 0.11f + fila * 0.55f + tablon * 2.0f +
                    2.2f * std::sin(fila * 0.21f + tablon));

            float tono = tonoTablon * (0.84f + 0.16f * veta);

            // Granos finos.
            tono *= 0.95f + 0.05f * Hash01(y * lado + x + 91);

            // Borde oscuro entre tablones.
            if (fila < 2 || fila >= altoTablon - 1)
            {
                tono *= 0.55f;
            }

            // Junta vertical.
            if (x == junta || x == junta + 1)
            {
                tono *= 0.55f;
            }

            // Clavos junto a la junta.
            int dx = x - (junta + 6);
            int dy = fila - altoTablon / 2;
            if (dx * dx + dy * dy <= 2)
            {
                tono = 0.35f;
            }

            // Nudo.
            int nx = x - (30 + tablon * 23) % lado;
            int ny = fila - 16;
            if (nx * nx / 9 + ny * ny / 4 <= 1)
            {
                tono *= 0.72f;
            }

            unsigned char v = (unsigned char)Clamp(tono * 255.0f, 0.0f, 255.0f);
            ImageDrawPixel(&imagen, x, y, Color{ v, v, v, 255 });
        }
    }

    texturaMadera = LoadTextureFromImage(imagen);
    UnloadImage(imagen);

    GenTextureMipmaps(&texturaMadera);
    SetTextureFilter(texturaMadera, TEXTURE_FILTER_TRILINEAR);
    SetTextureWrap(texturaMadera, TEXTURE_WRAP_REPEAT);
}


// Cara rectangular texturada. Los cuatro vertices ya vienen en orden
// antihorario visto desde afuera (si no, el culling la descarta).
static void CaraMadera(
    Vector3 a, Vector3 b, Vector3 c, Vector3 d,
    float uMax, float vMax,
    Color color
)
{
    rlCheckRenderBatchLimit(4);
    rlSetTexture(texturaMadera.id);
    rlBegin(RL_QUADS);
    rlColor4ub(color.r, color.g, color.b, 255);
    rlTexCoord2f(0.0f, vMax);
    rlVertex3f(a.x, a.y, a.z);
    rlTexCoord2f(uMax, vMax);
    rlVertex3f(b.x, b.y, b.z);
    rlTexCoord2f(uMax, 0.0f);
    rlVertex3f(c.x, c.y, c.z);
    rlTexCoord2f(0.0f, 0.0f);
    rlVertex3f(d.x, d.y, d.z);
    rlEnd();
    rlSetTexture(0);
}


static void DibujarTarimaMadera()
{
    CargarTexturaMadera();

    const float x0 = -6.9f;
    const float x1 = 6.9f;
    const float zFrente = 1.4f;
    const float zFondo = -1.6f;
    const float yBase = ALTURA_ARENA - 0.05f;
    const float yTapa = 0.18f;

    float alto = yTapa - yBase;

    // Frente (mira a la camara).
    CaraMadera(
        Vector3{ x0, yBase, zFrente }, Vector3{ x1, yBase, zFrente },
        Vector3{ x1, yTapa, zFrente }, Vector3{ x0, yTapa, zFrente },
        (x1 - x0) / 3.2f, alto / 1.2f,
        Color{ 196, 128, 92, 255 }
    );

    // Tapa (tablones a lo largo, un poco mas clara).
    CaraMadera(
        Vector3{ x0, yTapa, zFrente }, Vector3{ x1, yTapa, zFrente },
        Vector3{ x1, yTapa, zFondo }, Vector3{ x0, yTapa, zFondo },
        (x1 - x0) / 3.2f, (zFrente - zFondo) / 1.2f,
        Color{ 226, 160, 118, 255 }
    );

    // Laterales.
    CaraMadera(
        Vector3{ x0, yBase, zFondo }, Vector3{ x0, yBase, zFrente },
        Vector3{ x0, yTapa, zFrente }, Vector3{ x0, yTapa, zFondo },
        (zFrente - zFondo) / 3.2f, alto / 1.2f,
        Color{ 150, 96, 70, 255 }
    );

    CaraMadera(
        Vector3{ x1, yBase, zFrente }, Vector3{ x1, yBase, zFondo },
        Vector3{ x1, yTapa, zFondo }, Vector3{ x1, yTapa, zFrente },
        (zFrente - zFondo) / 3.2f, alto / 1.2f,
        Color{ 150, 96, 70, 255 }
    );
}


static void DibujarSuelo(float tiempo)
{
    DibujarMar(tiempo);
    DibujarArena();

    // Tarima de madera elevada sobre la arena: se ve todo su frente.
    DibujarTarimaMadera();
}


static void DibujarPedestal(
    Vector3 posicion,
    const PersonajeEscena3D& personaje
)
{
    Color cuerpo = Oscurecer(personaje.color, 0.60f);
    Color tapa = MezclarColor(personaje.color, WHITE, 0.45f);

    DrawCylinder(
        Vector3{ posicion.x, 0.15f, posicion.z },
        RADIO_PEDESTAL + 0.12f,
        RADIO_PEDESTAL + 0.12f,
        0.25f,
        28,
        Oscurecer(cuerpo, 0.8f)
    );

    DrawCylinder(
        Vector3{ posicion.x, 0.40f, posicion.z },
        RADIO_PEDESTAL,
        RADIO_PEDESTAL - 0.05f,
        ALTURA_PEDESTAL - 0.40f,
        28,
        cuerpo
    );

    float yTapa = ALTURA_PEDESTAL;

    DrawCylinder(
        Vector3{ posicion.x, yTapa - 0.02f, posicion.z },
        RADIO_PEDESTAL,
        RADIO_PEDESTAL,
        0.06f,
        28,
        tapa
    );

    // Aro de color de cada jugador que apunta aqui (el mas externo primero).
    // Un solo aro grueso por jugador: es el unico realce del pedestal.
    for (int k = 0; k < personaje.cantidadAros; k++)
    {
        float radioExterior = RADIO_PEDESTAL - 0.03f - k * 0.20f;
        float yAro = yTapa + 0.06f + k * 0.012f;

        DrawCylinder(
            Vector3{ posicion.x, yAro, posicion.z },
            radioExterior,
            radioExterior,
            0.012f,
            32,
            personaje.aros[k]
        );

        DrawCylinder(
            Vector3{ posicion.x, yAro + 0.006f, posicion.z },
            radioExterior - 0.14f,
            radioExterior - 0.14f,
            0.012f,
            32,
            tapa
        );
    }

    // Placa frontal gris cuando un bot ocupa el personaje.
    if (personaje.bot)
    {
        DrawCube(
            Vector3{ posicion.x, 0.34f, posicion.z + RADIO_PEDESTAL * 0.98f },
            0.9f,
            0.2f,
            0.05f,
            Color{ 150, 150, 160, 255 }
        );
    }
}


//==================================================
// PERSONAJE
//==================================================

static void DibujarPersonaje(
    int indice,
    Vector3 pedestal,
    const PersonajeEscena3D& personaje,
    float tiempo
)
{
    float escala =
        ESCALA_BASE_PERSONAJE * (1.0f + REALCE_FOCO * personaje.foco);

    // Salto parabolico de confirmacion.
    float salto = 0.0f;

    if (personaje.progresoSalto > 0.0f && personaje.progresoSalto < 1.0f)
    {
        float p = personaje.progresoSalto;
        salto = 4.0f * p * (1.0f - p) * 0.55f;
    }

    // Respiracion idle.
    float respiracion =
        std::sin(tiempo * 2.2f + indice * 1.3f) *
        (0.012f + 0.012f * personaje.foco);

    Vector3 pies = {
        pedestal.x,
        ALTURA_PEDESTAL + 0.05f + salto + respiracion + personaje.foco * 0.04f,
        pedestal.z
    };

    // Mira a camara (angulo 0 mira hacia +Z). El giro solo ocurre al confirmar.
    float angulo =
        std::sin(tiempo * 1.1f + indice) * 6.0f * personaje.foco +
        personaje.giroExtra;

    // Sombra de contacto sobre el pedestal.
    DrawCylinder(
        Vector3{ pedestal.x, ALTURA_PEDESTAL + 0.025f, pedestal.z },
        0.55f,
        0.55f,
        0.01f,
        16,
        Fade(BLACK, 0.28f)
    );

    // Remera con el color propio del personaje; el no apuntado solo se
    // atenua un poco para que el apuntado destaque sin ocultar al resto.
    Color remera = Oscurecer(personaje.color, 0.88f + 0.12f * personaje.foco);

    if (personaje.listo)
    {
        remera = MezclarColor(remera, Color{ 255, 236, 160, 255 }, 0.25f);
    }

    // Cada pedestal tiene su propio reloj para que no respiren a la par;
    // al confirmar salta con los brazos arriba.
    PosePersonajeBloque pose;
    pose.tiempo = tiempo + indice * 0.9f;
    pose.enAire = salto > 0.02f;

    DibujarModeloJugadorEnPosicion(indice, pies, angulo, remera, escala, pose);
}


//==================================================
// CONFETI
//==================================================

static void DibujarConfeti(
    float tiempo,
    float intensidad
)
{
    // Solo durante TODOS LISTOS; escala con la calidad grafica.
    if (intensidad <= 0.01f)
    {
        return;
    }

    int cantidad =
        (int)(110.0f * intensidad * FactorCalidadGrafica(CalidadParticulas()));

    for (int i = 0; i < cantidad; i++)
    {
        float velocidad = 0.35f + Hash01(i * 3 + 1) * 0.35f;
        float fase = std::fmod(Hash01(i * 7 + 2) + tiempo * velocidad * 0.18f, 1.0f);

        Vector3 pos = {
            (Hash01(i * 11 + 3) - 0.5f) * 15.0f + std::sin(tiempo * 1.3f + i) * 0.3f,
            6.0f - fase * 6.0f,
            (Hash01(i * 13 + 4) - 0.5f) * 3.0f - 1.5f
        };

        float tam = 0.05f + Hash01(i * 17 + 5) * 0.04f;
        Color color = PALETA_FIESTA[i % 6];

        DrawCube(pos, tam * 1.6f, tam * 0.35f, tam, color);
    }
}


//==================================================
// ESCENA COMPLETA
//==================================================

void DibujarEscenaSeleccion(
    float tiempo,
    const PersonajeEscena3D personajes[],
    int cantidad,
    float intensidadConfeti
)
{
    // Las pantallas de menu no llaman ClearBackground y main.cpp tampoco:
    // sin limpiar, el depth buffer conserva la escena del frame anterior y
    // la nueva (casi a la misma profundidad) falla el test de profundidad,
    // dejando solo franjas sueltas de personajes y pedestales.
    ClearBackground(Color{ 52, 36, 112, 255 });

    Camera3D camara =
        ObtenerCamaraEscenaSeleccion(tiempo);

    DibujarCielo(tiempo, camara);

    // Con el plano cercano por defecto (0.01) la resolucion de profundidad a
    // ~35 m es de ~1 cm: los discos casi coplanares de la plaza y los aros
    // parpadean (z-fighting). Se acotan los planos y se restauran al final.
    double cercaPrevio = rlGetCullDistanceNear();
    double lejosPrevio = rlGetCullDistanceFar();
    rlSetClipPlanes(0.5, 400.0);

    BeginMode3D(camara);

    DibujarSuelo(tiempo);
    DibujarPostesYGuirnalda(tiempo);

    for (int i = 0; i < cantidad; i++)
    {
        Vector3 pedestal = ObtenerPosicionPedestalSeleccion(i);

        DibujarPedestal(pedestal, personajes[i]);
        DibujarPersonaje(i, pedestal, personajes[i], tiempo);
    }

    DibujarConfeti(tiempo, intensidadConfeti);

    EndMode3D();

    rlSetClipPlanes(cercaPrevio, lejosPrevio);
}
