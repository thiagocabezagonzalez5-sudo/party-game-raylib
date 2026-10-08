#include "Board/Casilla.h"

#include "rlgl.h"

#include <cmath>


//==================================================
// COLOR
//==================================================

Color ObtenerColorCasilla(
    TipoCasilla tipo
)
{
    switch (tipo)
    {
        case CASILLA_POSITIVA:
            return Color{
                70,
                185,
                105,
                255
            };

        case CASILLA_NEGATIVA:
            return Color{
                220,
                75,
                75,
                255
            };

        case CASILLA_REGALO: return Color{ 120, 210, 90, 255 };
        case CASILLA_CAOS: return Color{ 130, 60, 170, 255 };
        case CASILLA_INTERCAMBIO: return Color{ 60, 200, 210, 255 };
        case CASILLA_DUELO: return Color{ 240, 120, 40, 255 };
        case CASILLA_TIENDA: return Color{ 230, 205, 120, 255 };
        case CASILLA_EVENTO: return Color{ 255, 120, 190, 255 };
        case CANTIDAD_TIPOS_CASILLA: break;

        case CASILLA_ESPECIAL:
            return Color{
                245,
                175,
                45,
                255
            };

        case CASILLA_NEUTRA:
            break;
    }

    return Color{
        80,
        145,
        220,
        255
    };
}


//==================================================
// NOMBRE
//==================================================

const char* ObtenerNombreTipoCasilla(
    TipoCasilla tipo
)
{
    switch (tipo)
    {
        case CASILLA_POSITIVA:
            return "POSITIVA";

        case CASILLA_NEGATIVA:
            return "NEGATIVA";

        case CASILLA_REGALO: return "REGALO";
        case CASILLA_CAOS: return "CAOS";
        case CASILLA_INTERCAMBIO: return "INTERCAMBIO";
        case CASILLA_DUELO: return "DUELO";
        case CASILLA_TIENDA: return "TIENDA";
        case CASILLA_EVENTO: return "EVENTO";
        case CANTIDAD_TIPOS_CASILLA: break;

        case CASILLA_ESPECIAL:
            return "ESPECIAL";

        case CASILLA_NEUTRA:
            break;
    }

    return "NEUTRA";
}


//==================================================
// ESTILO POR DEFECTO
//==================================================

EstiloCasilla ObtenerEstiloCasillaPiedra()
{
    return EstiloCasilla{};
}


//==================================================
// UTILIDADES DE COLOR
//==================================================

static Color MezclarColorCasilla(
    Color a,
    Color b,
    float t
)
{
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;

    return Color
    {
        (unsigned char)(a.r + (b.r - a.r) * t),
        (unsigned char)(a.g + (b.g - a.g) * t),
        (unsigned char)(a.b + (b.b - a.b) * t),
        255
    };
}


//==================================================
// CINTA DE SUELO
//==================================================

void DibujarCintaSuelo(
    Vector3 origen,
    Vector3 destino,
    float ancho,
    float altura,
    Color color
)
{
    float dx = destino.x - origen.x;
    float dz = destino.z - origen.z;
    float largo = std::sqrt(dx * dx + dz * dz);

    if (largo < 0.001f)
    {
        return;
    }

    // Normal horizontal de la direccion del tramo.
    float nx = -dz / largo * ancho * 0.5f;
    float nz = dx / largo * ancho * 0.5f;

    Vector3 a = { origen.x + nx, altura, origen.z + nz };
    Vector3 b = { origen.x - nx, altura, origen.z - nz };
    Vector3 c = { destino.x - nx, altura, destino.z - nz };
    Vector3 d = { destino.x + nx, altura, destino.z + nz };

    // Se dibuja con las dos caras para no depender del sentido.
    rlDisableBackfaceCulling();
    DrawTriangle3D(a, b, c, color);
    DrawTriangle3D(a, c, d, color);
    rlEnableBackfaceCulling();
}


//==================================================
// CASILLA TEMATICA
//==================================================

void DibujarCasillaTematica(
    const Casilla& casilla,
    const EstiloCasilla& estilo,
    float tiempo
)
{
    const int LADOS = 14;
    const Vector3 p = casilla.posicion;
    const float suelo = p.y - 0.25f;

    Color funcional =
        ObtenerColorCasilla(casilla.tipo);

    float pulso =
        1.0f +
        estilo.pulso * std::sin(tiempo * 2.6f + casilla.indice * 0.9f);

    Color brillo =
    {
        (unsigned char)std::fmin(255.0f, funcional.r * pulso),
        (unsigned char)std::fmin(255.0f, funcional.g * pulso),
        (unsigned char)std::fmin(255.0f, funcional.b * pulso),
        255
    };

    Color brilloClaro =
        MezclarColorCasilla(brillo, WHITE, 0.45f);

    float r = estilo.radio;
    float alto = estilo.altura;

    // Sombra de contacto sobre el suelo.
    DrawCylinderEx(
        Vector3{ p.x + 0.06f, suelo + 0.03f, p.z + 0.08f },
        Vector3{ p.x + 0.06f, suelo + 0.045f, p.z + 0.08f },
        r + 0.16f,
        r + 0.16f,
        LADOS,
        Color{ 20, 24, 20, 90 }
    );

    // Cuerpo de la losa: aro de borde + piedra.
    DrawCylinderEx(
        Vector3{ p.x, suelo + 0.0f, p.z },
        Vector3{ p.x, suelo + alto * 0.62f, p.z },
        r + 0.06f,
        r,
        LADOS,
        estilo.borde
    );

    DrawCylinderEx(
        Vector3{ p.x, suelo + alto * 0.30f, p.z },
        Vector3{ p.x, suelo + alto * 0.70f, p.z },
        r + 0.02f,
        r - 0.02f,
        LADOS,
        estilo.piedraLado
    );

    // Tapa de piedra mas clara.
    DrawCylinderEx(
        Vector3{ p.x, suelo + alto * 0.70f, p.z },
        Vector3{ p.x, suelo + alto * 0.94f, p.z },
        r - 0.04f,
        r - 0.07f,
        LADOS,
        estilo.piedraTapa
    );

    // Musgo en el borde de tres puntos de cada losa (varia por indice).
    for (int i = 0; i < 3; i++)
    {
        float ang = casilla.indice * 1.7f + i * 2.3f;

        DrawSphereEx(
            Vector3
            {
                p.x + std::cos(ang) * (r - 0.02f),
                suelo + alto * 0.74f,
                p.z + std::sin(ang) * (r - 0.02f)
            },
            0.13f,
            4,
            5,
            estilo.detalle
        );
    }

    // Incrustacion: anillo luminoso + disco funcional.
    float techo = suelo + alto * 0.94f;

    DrawCylinderEx(
        Vector3{ p.x, techo, p.z },
        Vector3{ p.x, techo + 0.025f, p.z },
        r * 0.80f,
        r * 0.80f,
        LADOS,
        brilloClaro
    );

    DrawCylinderEx(
        Vector3{ p.x, techo + 0.025f, p.z },
        Vector3{ p.x, techo + 0.05f, p.z },
        r * 0.70f,
        r * 0.70f,
        LADOS,
        brillo
    );

    // Simbolo: legible sin depender del color.
    Color simbolo = Color{ 255, 255, 255, 235 };
    float ys = techo + 0.07f;

    if (casilla.tipo == CASILLA_POSITIVA)
    {
        DrawCubeV(Vector3{ p.x, ys, p.z }, Vector3{ 0.46f, 0.04f, 0.12f }, simbolo);
        DrawCubeV(Vector3{ p.x, ys, p.z }, Vector3{ 0.12f, 0.04f, 0.46f }, simbolo);
    }
    else if (casilla.tipo == CASILLA_NEGATIVA)
    {
        DrawCubeV(Vector3{ p.x, ys, p.z }, Vector3{ 0.46f, 0.04f, 0.12f }, simbolo);
    }
    else if (casilla.tipo == CASILLA_ESPECIAL)
    {
        // Estrella: dos cuadrados girados 45 grados.
        DrawCubeV(Vector3{ p.x, ys, p.z }, Vector3{ 0.30f, 0.04f, 0.30f }, simbolo);

        rlPushMatrix();
        rlTranslatef(p.x, ys, p.z);
        rlRotatef(45.0f, 0.0f, 1.0f, 0.0f);
        DrawCubeV(Vector3{ 0.0f, 0.0f, 0.0f }, Vector3{ 0.30f, 0.04f, 0.30f }, simbolo);
        rlPopMatrix();
    }
}


//==================================================
// VERSION EN MALLA (estatica, para la GPU)
//==================================================
//
// Misma geometria que DibujarCintaSuelo y DibujarCasillaTematica, pero
// emitida a un ConstructorMalla. La parte que pulsa (anillo y disco de la
// incrustacion) va a una malla aparte cuyos colores se actualizan por frame.

void AgregarCintaSueloMalla(
    ConstructorMalla& malla,
    Vector3 origen,
    Vector3 destino,
    float ancho,
    float altura,
    Color color
)
{
    float dx = destino.x - origen.x;
    float dz = destino.z - origen.z;
    float largo = std::sqrt(dx * dx + dz * dz);

    if (largo < 0.001f)
    {
        return;
    }

    float nx = -dz / largo * ancho * 0.5f;
    float nz = dx / largo * ancho * 0.5f;

    Vector3 a = { origen.x + nx, altura, origen.z + nz };
    Vector3 b = { origen.x - nx, altura, origen.z - nz };
    Vector3 c = { destino.x - nx, altura, destino.z - nz };
    Vector3 d = { destino.x + nx, altura, destino.z + nz };

    malla.TrianguloDobleCara(a, b, c, color);
    malla.TrianguloDobleCara(a, c, d, color);
}


void ObtenerColoresIncrustacionCasilla(
    const Casilla& casilla,
    const EstiloCasilla& estilo,
    float tiempo,
    Color& claro,
    Color& normal
)
{
    Color funcional =
        ObtenerColorCasilla(casilla.tipo);

    float pulso =
        1.0f +
        estilo.pulso * std::sin(tiempo * 2.6f + casilla.indice * 0.9f);

    normal =
    {
        (unsigned char)std::fmin(255.0f, funcional.r * pulso),
        (unsigned char)std::fmin(255.0f, funcional.g * pulso),
        (unsigned char)std::fmin(255.0f, funcional.b * pulso),
        255
    };

    claro = MezclarColorCasilla(normal, WHITE, 0.45f);
}


void AgregarCasillaTematicaMalla(
    ConstructorMalla& fija,
    ConstructorMalla& incrustacion,
    const Casilla& casilla,
    const EstiloCasilla& estilo,
    bool conSombra,
    bool conMusgo
)
{
    const int LADOS = 14;
    const Vector3 p = casilla.posicion;
    const float suelo = p.y - 0.25f;

    float r = estilo.radio;
    float alto = estilo.altura;

    // Sombra de contacto sobre el suelo.
    if (conSombra)
    {
        fija.Cilindro(
            Vector3{ p.x + 0.06f, suelo + 0.03f, p.z + 0.08f },
            Vector3{ p.x + 0.06f, suelo + 0.045f, p.z + 0.08f },
            r + 0.16f,
            r + 0.16f,
            LADOS,
            Color{ 20, 24, 20, 90 }
        );
    }

    fija.Cilindro(
        Vector3{ p.x, suelo + 0.0f, p.z },
        Vector3{ p.x, suelo + alto * 0.62f, p.z },
        r + 0.06f,
        r,
        LADOS,
        estilo.borde
    );

    fija.Cilindro(
        Vector3{ p.x, suelo + alto * 0.30f, p.z },
        Vector3{ p.x, suelo + alto * 0.70f, p.z },
        r + 0.02f,
        r - 0.02f,
        LADOS,
        estilo.piedraLado
    );

    fija.Cilindro(
        Vector3{ p.x, suelo + alto * 0.70f, p.z },
        Vector3{ p.x, suelo + alto * 0.94f, p.z },
        r - 0.04f,
        r - 0.07f,
        LADOS,
        estilo.piedraTapa
    );

    if (conMusgo)
    {
        for (int i = 0; i < 3; i++)
        {
            float ang = casilla.indice * 1.7f + i * 2.3f;

            fija.Esfera(
                Vector3
                {
                    p.x + std::cos(ang) * (r - 0.02f),
                    suelo + alto * 0.74f,
                    p.z + std::sin(ang) * (r - 0.02f)
                },
                0.13f,
                4,
                5,
                estilo.detalle
            );
        }
    }

    // Incrustacion (los colores reales los pone cada frame el pulso).
    float techo = suelo + alto * 0.94f;

    incrustacion.Cilindro(
        Vector3{ p.x, techo, p.z },
        Vector3{ p.x, techo + 0.025f, p.z },
        r * 0.80f,
        r * 0.80f,
        LADOS,
        WHITE
    );

    incrustacion.Cilindro(
        Vector3{ p.x, techo + 0.025f, p.z },
        Vector3{ p.x, techo + 0.05f, p.z },
        r * 0.70f,
        r * 0.70f,
        LADOS,
        WHITE
    );

    // Simbolo: legible sin depender del color.
    Color simbolo = Color{ 255, 255, 255, 235 };
    float ys = techo + 0.07f;

    if (casilla.tipo == CASILLA_POSITIVA)
    {
        incrustacion.Caja(Vector3{ p.x, ys, p.z }, Vector3{ 0.46f, 0.04f, 0.12f }, simbolo);
        incrustacion.Caja(Vector3{ p.x, ys, p.z }, Vector3{ 0.12f, 0.04f, 0.46f }, simbolo);
    }
    else if (casilla.tipo == CASILLA_NEGATIVA)
    {
        incrustacion.Caja(Vector3{ p.x, ys, p.z }, Vector3{ 0.46f, 0.04f, 0.12f }, simbolo);
    }
    else if (casilla.tipo == CASILLA_ESPECIAL)
    {
        incrustacion.Caja(Vector3{ p.x, ys, p.z }, Vector3{ 0.30f, 0.04f, 0.30f }, simbolo);

        TransformacionMalla giro;
        giro.tx = p.x;
        giro.ty = ys;
        giro.tz = p.z;
        giro.giroY = 45.0f;

        incrustacion.EstablecerTransformacion(giro);
        incrustacion.Caja(Vector3{ 0.0f, 0.0f, 0.0f }, Vector3{ 0.30f, 0.04f, 0.30f }, simbolo);
        incrustacion.QuitarTransformacion();
    }
}
