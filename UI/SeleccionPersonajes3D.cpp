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
        GetWorldToScreen(Vector3{ 0.0f, -1.45f, -80.0f }, camara).y;

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

        DrawCylinder(Vector3{ x, 0.0f, -2.2f }, 0.14f, 0.18f, 5.0f, 8, Color{ 90, 62, 58, 255 });
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

static void DibujarSuelo()
{
    // Mar al fondo.
    DrawCylinder(Vector3{ 0.0f, -1.5f, 0.0f }, 90.0f, 90.0f, 0.1f, 40, Color{ 60, 82, 150, 255 });

    // Plaza de piedra (dos anillos para dar sensacion de baldosas).
    DrawCylinder(Vector3{ 0.0f, -1.4f, 0.0f }, 26.0f, 26.0f, 1.4f, 48, Color{ 150, 122, 128, 255 });
    DrawCylinder(Vector3{ 0.0f, -0.06f, 0.0f }, 24.0f, 24.0f, 0.06f, 48, Color{ 206, 178, 160, 255 });
    DrawCylinder(Vector3{ 0.0f, -0.05f, 0.0f }, 16.5f, 16.5f, 0.06f, 48, Color{ 190, 160, 146, 255 });
    DrawCylinder(Vector3{ 0.0f, -0.04f, 0.0f }, 15.8f, 15.8f, 0.06f, 48, Color{ 214, 188, 168, 255 });

    // Tarima de madera baja y poco profunda: sostiene los pedestales sin
    // formar un frente alto que tape la plaza (las etiquetas 2D de los
    // personajes quedan sobre la plaza, no sobre un zocalo de colores).
    DrawCube(Vector3{ 0.0f, 0.06f, -0.1f }, 13.8f, 0.12f, 3.0f, Color{ 150, 92, 76, 255 });
    DrawCube(Vector3{ 0.0f, 0.135f, -0.1f }, 13.5f, 0.03f, 2.7f, Color{ 178, 118, 92, 255 });
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
        float yAro = yTapa + 0.045f + k * 0.004f;

        DrawCylinder(
            Vector3{ posicion.x, yAro, posicion.z },
            radioExterior,
            radioExterior,
            0.012f,
            32,
            personaje.aros[k]
        );

        DrawCylinder(
            Vector3{ posicion.x, yAro + 0.003f, posicion.z },
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

    // Tinte propio de cada personaje, siempre claro; el no apuntado solo
    // se atenua un poco para que el apuntado destaque sin ocultar al resto.
    Color base = MezclarColor(personaje.color, WHITE, 0.62f);
    Color tinte = Oscurecer(base, 0.90f + 0.10f * personaje.foco);

    if (personaje.listo)
    {
        tinte = MezclarColor(tinte, Color{ 255, 236, 160, 255 }, 0.30f);
    }

    InicializarModeloJugadorCompartido();

    const Model* modelo = ObtenerModeloJugadorCompartidoRender();

    if (modelo == nullptr)
    {
        DibujarModeloJugadorEnPosicion(pies, angulo, tinte, escala);
        return;
    }

    // El modelo y su pose son compartidos: se fija un fotograma distinto
    // por personaje justo antes de cada dibujo para que no se muevan igual.
    int fotogramas = ObtenerCantidadFotogramasIdleModeloJugadorCompartido();

    if (fotogramas > 0)
    {
        AplicarFotogramaIdleModeloJugadorCompartido(
            (int)(tiempo * 30.0f) + indice * (fotogramas / 4 + 3)
        );
    }

    DrawModelEx(
        *modelo,
        pies,
        Vector3{ 0.0f, 1.0f, 0.0f },
        angulo,
        Vector3{ escala, escala, escala },
        tinte
    );
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

    BeginMode3D(camara);

    DibujarSuelo();
    DibujarPostesYGuirnalda(tiempo);

    for (int i = 0; i < cantidad; i++)
    {
        Vector3 pedestal = ObtenerPosicionPedestalSeleccion(i);

        DibujarPedestal(pedestal, personajes[i]);
        DibujarPersonaje(i, pedestal, personajes[i], tiempo);
    }

    DibujarConfeti(tiempo, intensidadConfeti);

    EndMode3D();
}
