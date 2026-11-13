#include "Entities/PersonajesBloque.h"

#include "raymath.h"
#include "rlgl.h"

#include <cmath>


//==================================================
// CAJAS SOMBREADAS
//==================================================
//
// Cada caja se transforma en CPU con la matriz de su parte del cuerpo y
// cada cara recibe un tono segun su normal frente a una luz fija (sol
// arriba-adelante-derecha). Asi el personaje tiene volumen sin shaders y
// funciona en cualquier escena. La matriz rlgl del llamador se sigue
// aplicando encima (por eso los minijuegos pueden hacer rlPushMatrix).

static const Vector3 DIRECCION_LUZ_PERSONAJE =
    { 0.4417f, 0.7852f, 0.3926f };


static Color TonoCara(
    Color color,
    Vector3 normal,
    float alpha,
    bool emisiva
)
{
    float tono = 1.0f;

    if (!emisiva)
    {
        float luz =
            normal.x * DIRECCION_LUZ_PERSONAJE.x +
            normal.y * DIRECCION_LUZ_PERSONAJE.y +
            normal.z * DIRECCION_LUZ_PERSONAJE.z;

        tono = 0.62f + 0.38f * (luz > 0.0f ? luz : 0.0f);

        if (normal.y < 0.0f)
        {
            tono -= 0.12f * -normal.y;
        }
    }

    auto canal = [tono](unsigned char c)
    {
        float v = c * tono;
        return (unsigned char)(v > 255.0f ? 255.0f : v);
    };

    float a = color.a * alpha;

    return Color{
        canal(color.r),
        canal(color.g),
        canal(color.b),
        (unsigned char)(a > 255.0f ? 255.0f : (a < 0.0f ? 0.0f : a))
    };
}


// Caja centrada en "centro" (coordenadas locales de la parte) con su
// tamano completo en "tam".
static void Caja(
    const Matrix& m,
    Vector3 centro,
    Vector3 tam,
    Color color,
    float alpha,
    bool emisiva = false
)
{
    if (alpha <= 0.01f)
    {
        return;
    }

    const float hx = tam.x * 0.5f;
    const float hy = tam.y * 0.5f;
    const float hz = tam.z * 0.5f;

    // Esquinas: bit0 = x, bit1 = y, bit2 = z (0 = negativo, 1 = positivo).
    Vector3 v[8];

    for (int i = 0; i < 8; i++)
    {
        Vector3 local = {
            centro.x + ((i & 1) ? hx : -hx),
            centro.y + ((i & 2) ? hy : -hy),
            centro.z + ((i & 4) ? hz : -hz)
        };

        v[i] = Vector3Transform(local, m);
    }

    // Caras en sentido antihorario visto desde afuera (frente = CCW).
    static const int CARAS[6][4] =
    {
        { 5, 1, 3, 7 },   // +X
        { 0, 4, 6, 2 },   // -X
        { 6, 7, 3, 2 },   // +Y
        { 0, 1, 5, 4 },   // -Y
        { 4, 5, 7, 6 },   // +Z
        { 1, 0, 2, 3 }    // -Z
    };

    static const Vector3 NORMALES[6] =
    {
        { 1, 0, 0 }, { -1, 0, 0 },
        { 0, 1, 0 }, { 0, -1, 0 },
        { 0, 0, 1 }, { 0, 0, -1 }
    };

    Vector3 origen = Vector3Transform(Vector3Zero(), m);

    rlCheckRenderBatchLimit(36);
    rlBegin(RL_TRIANGLES);

    for (int c = 0; c < 6; c++)
    {
        Vector3 normal =
            Vector3Normalize(
                Vector3Subtract(
                    Vector3Transform(NORMALES[c], m),
                    origen
                )
            );

        Color tono = TonoCara(color, normal, alpha, emisiva);
        rlColor4ub(tono.r, tono.g, tono.b, tono.a);

        const int* q = CARAS[c];
        const int orden[6] = { q[0], q[1], q[2], q[0], q[2], q[3] };

        for (int k = 0; k < 6; k++)
        {
            rlVertex3f(v[orden[k]].x, v[orden[k]].y, v[orden[k]].z);
        }
    }

    rlEnd();
}


// Caja con aristas escalonadas (tres cajas cruzadas): da el canto
// redondeado de una pieza de juguete sin geometria curva.
static void CajaBiselada(
    const Matrix& m,
    Vector3 centro,
    Vector3 tam,
    float bisel,
    Color color,
    float alpha
)
{
    float b2 = bisel * 2.0f;

    Caja(m, centro, { tam.x, tam.y - b2, tam.z - b2 }, color, alpha);
    Caja(m, centro, { tam.x - b2, tam.y, tam.z - b2 }, color, alpha);
    Caja(m, centro, { tam.x - b2, tam.y - b2, tam.z }, color, alpha);
}


static Matrix Pivote(
    Vector3 punto,
    Matrix rotacion
)
{
    return MatrixMultiply(
        MatrixMultiply(
            MatrixTranslate(-punto.x, -punto.y, -punto.z),
            rotacion
        ),
        MatrixTranslate(punto.x, punto.y, punto.z)
    );
}


// Aplica primero "local" y despues "padre".
static Matrix Hijo(
    const Matrix& local,
    const Matrix& padre
)
{
    return MatrixMultiply(local, padre);
}


static Color Mezclar(
    Color a,
    Color b,
    float t
)
{
    return Color{
        (unsigned char)(a.r + (b.r - a.r) * t),
        (unsigned char)(a.g + (b.g - a.g) * t),
        (unsigned char)(a.b + (b.b - a.b) * t),
        a.a
    };
}


static float Suavizar(
    float t
)
{
    if (t <= 0.0f) return 0.0f;
    if (t >= 1.0f) return 1.0f;
    return t * t * (3.0f - 2.0f * t);
}


static float SalidaRapida(
    float t
)
{
    if (t <= 0.0f) return 0.0f;
    if (t >= 1.0f) return 1.0f;
    float u = 1.0f - t;
    return 1.0f - u * u * u;
}


//==================================================
// DATOS DE CADA PERSONAJE
//==================================================

struct FormaPersonajeBloque
{
    const char* nombre;
    const char* lema;
    Color distintivo;

    // Proporciones (unidades de juego a escala normal).
    float altoPierna;
    float anchoPierna;
    float separacionPiernas;

    float anchoTorso;
    float altoTorso;
    float fondoTorso;

    float anchoBrazo;
    float largoBrazo;

    float anchoCabeza;
    float altoCabeza;
    float fondoCabeza;

    // Distancia maxima de la espalda (casco, mochila, pelo...) al eje:
    // al caer de espaldas el cuerpo apoya justo sobre el suelo.
    float fondoEspalda;

    Color piel;
    Color pantalon;
    Color zapato;
    Color mano;
};


// El vendedor de la tienda usa el mismo cuerpo con una forma propia, en el
// indice siguiente a los cuatro jugables (no es elegible en la seleccion).
static const int PERSONAJE_VENDEDOR = CANTIDAD_PERSONAJES_BLOQUE;

static const FormaPersonajeBloque FORMAS[CANTIDAD_PERSONAJES_BLOQUE + 1] =
{
    // BRICO: albanil robusto, torso ancho y piernas cortas.
    {
        "BRICO", "Fuerte y con casco", Color{ 255, 196, 40, 255 },
        0.46f, 0.22f, 0.125f,
        0.62f, 0.42f, 0.32f,
        0.17f, 0.40f,
        0.44f, 0.40f, 0.40f,
        0.25f,
        Color{ 232, 178, 128, 255 },
        Color{ 58, 92, 150, 255 },
        Color{ 92, 60, 36, 255 },
        Color{ 214, 158, 64, 255 }
    },

    // PIXI: exploradora alta y delgada, coleta y mochila.
    {
        "PIXI", "Rapida y curiosa", Color{ 236, 104, 52, 255 },
        0.54f, 0.17f, 0.10f,
        0.48f, 0.40f, 0.26f,
        0.14f, 0.40f,
        0.40f, 0.40f, 0.38f,
        0.27f,
        Color{ 250, 214, 182, 255 },
        Color{ 196, 164, 112, 255 },
        Color{ 120, 76, 44, 255 },
        Color{ 250, 214, 182, 255 }
    },

    // ROBOTO: robot de chapa, cabeza ancha con antena.
    {
        "ROBOTO", "Pura chapa y voltaje", Color{ 90, 214, 240, 255 },
        0.44f, 0.22f, 0.13f,
        0.58f, 0.44f, 0.34f,
        0.18f, 0.40f,
        0.46f, 0.38f, 0.42f,
        0.24f,
        Color{ 176, 186, 202, 255 },
        Color{ 110, 118, 134, 255 },
        Color{ 70, 76, 90, 255 },
        Color{ 150, 158, 174, 255 }
    },

    // MOMO: criatura bajita y cabezona con orejas y cola.
    {
        "MOMO", "Tierno pero tramposo", Color{ 186, 120, 230, 255 },
        0.32f, 0.21f, 0.12f,
        0.50f, 0.36f, 0.34f,
        0.16f, 0.32f,
        0.56f, 0.50f, 0.50f,
        0.27f,
        Color{ 214, 150, 92, 255 },
        Color{ 214, 150, 92, 255 },
        Color{ 166, 108, 62, 255 },
        Color{ 214, 150, 92, 255 }
    },

    // DON CAMBALACHE: vendedor panzon con delantal y gorro a rayas.
    {
        "DON CAMBALACHE", "Todo tiene precio", Color{ 70, 160, 90, 255 },
        0.38f, 0.22f, 0.13f,
        0.66f, 0.44f, 0.40f,
        0.17f, 0.38f,
        0.46f, 0.42f, 0.42f,
        0.26f,
        Color{ 240, 190, 150, 255 },
        Color{ 92, 62, 42, 255 },
        Color{ 60, 40, 30, 255 },
        Color{ 240, 190, 150, 255 }
    }
};


static const Color COLOR_OJO = { 28, 28, 38, 255 };
static const Color COLOR_BRILLO_OJO = { 255, 255, 255, 255 };


const char* ObtenerNombrePersonajeBloque(
    int personaje
)
{
    return FORMAS[NormalizarPersonajeBloque(personaje)].nombre;
}


const char* ObtenerLemaPersonajeBloque(
    int personaje
)
{
    return FORMAS[NormalizarPersonajeBloque(personaje)].lema;
}


Color ObtenerColorPersonajeBloque(
    int personaje
)
{
    return FORMAS[NormalizarPersonajeBloque(personaje)].distintivo;
}


int NormalizarPersonajeBloque(
    int personaje
)
{
    if (personaje < 0)
    {
        return 0;
    }

    return personaje % CANTIDAD_PERSONAJES_BLOQUE;
}


//==================================================
// CARA
//==================================================

// Ojos: abiertos, parpadeando o en cruz (aturdido). "frente" es la Z de la
// cara delantera de la cabeza.
static void DibujarOjos(
    const Matrix& cabeza,
    float frente,
    float altura,
    float separacion,
    Vector3 tam,
    float apertura,
    bool aturdido,
    float alpha
)
{
    for (int lado = -1; lado <= 1; lado += 2)
    {
        float x = separacion * lado;
        Vector3 centro = { x, altura, frente + tam.z * 0.5f - 0.004f };

        if (aturdido)
        {
            // Ojos en espiral simplificada: una cruz inclinada.
            for (int k = -1; k <= 1; k += 2)
            {
                Matrix giro = Hijo(
                    Pivote(centro, MatrixRotateZ(0.785f * k)),
                    cabeza
                );

                Caja(giro, centro, { tam.x * 1.1f, tam.x * 0.28f, tam.z }, COLOR_OJO, alpha);
            }

            continue;
        }

        // apertura > 1 = susto: el ojo tambien se ensancha.
        float ancho = apertura > 1.0f ? tam.x * (1.0f + (apertura - 1.0f) * 0.6f) : tam.x;

        Caja(
            cabeza,
            centro,
            { ancho, tam.y * apertura, tam.z },
            COLOR_OJO,
            alpha
        );

        if (apertura > 0.5f)
        {
            Caja(
                cabeza,
                {
                    x - tam.x * 0.22f,
                    altura + tam.y * 0.22f,
                    centro.z + tam.z * 0.5f
                },
                { tam.x * 0.34f, tam.x * 0.34f, 0.012f },
                COLOR_BRILLO_OJO,
                alpha,
                true
            );
        }
    }
}


static float AperturaOjos(
    float tiempo,
    int personaje
)
{
    float periodo = 3.6f + personaje * 0.45f;
    float t = std::fmod(tiempo + personaje * 1.7f, periodo);

    if (t < 0.0f) t += periodo;

    return t < 0.11f ? 0.15f : 1.0f;
}


// Apertura de ojos segun la pose: con susto quedan muy abiertos y no
// parpadean.
static float AperturaOjosPose(
    const PosePersonajeBloque& pose,
    int personaje
)
{
    if (pose.susto > 0.05f)
    {
        return 1.0f + 0.5f * (pose.susto > 1.0f ? 1.0f : pose.susto);
    }

    return AperturaOjos(pose.tiempo, personaje);
}


// Apertura de la boca 0..1: en O con susto, abre y cierra al hablar.
static float AperturaBoca(
    const PosePersonajeBloque& pose
)
{
    if (pose.susto > 0.4f)
    {
        return 1.0f;
    }

    if (pose.hablando)
    {
        float s = std::sin(pose.tiempo * 15.0f) * 0.5f + 0.5f;
        return 0.15f + 0.6f * s * s;
    }

    return 0.0f;
}


//==================================================
// CABEZAS
//==================================================

static void CabezaBrico(
    const FormaPersonajeBloque& f,
    const Matrix& m,
    const PosePersonajeBloque& pose,
    float alpha
)
{
    const float A = f.altoCabeza;
    const float frente = f.fondoCabeza * 0.5f;

    CajaBiselada(m, { 0, A * 0.5f, 0 }, { f.anchoCabeza, A, f.fondoCabeza }, 0.03f, f.piel, alpha);

    // Orejas.
    for (int lado = -1; lado <= 1; lado += 2)
    {
        Caja(m, { lado * (f.anchoCabeza * 0.5f + 0.02f), A * 0.48f, 0.0f }, { 0.04f, 0.09f, 0.08f }, Mezclar(f.piel, BLACK, 0.08f), alpha);
    }

    // Casco de obra con visera, raya naranja y cresta.
    Color casco = { 255, 200, 44, 255 };
    Caja(m, { 0, A + 0.045f, 0 }, { f.anchoCabeza + 0.04f, 0.13f, f.fondoCabeza + 0.04f }, casco, alpha);
    Caja(m, { 0, A + 0.125f, 0 }, { 0.10f, 0.05f, f.fondoCabeza + 0.06f }, Mezclar(casco, BLACK, 0.12f), alpha);
    Caja(m, { 0, A - 0.005f, frente + 0.06f }, { f.anchoCabeza + 0.10f, 0.035f, 0.16f }, casco, alpha);
    Caja(m, { 0, A + 0.01f, 0 }, { f.anchoCabeza + 0.05f, 0.035f, f.fondoCabeza + 0.05f }, Color{ 240, 120, 30, 255 }, alpha);

    // Cejas gruesas (se fruncen al golpear).
    float ceno = (pose.golpe >= 0.0f ? 0.02f : 0.0f) - 0.03f * pose.susto;
    Color pelo = { 92, 58, 34, 255 };

    for (int lado = -1; lado <= 1; lado += 2)
    {
        Caja(m, { lado * 0.10f, A * 0.73f - ceno, frente + 0.01f }, { 0.13f, 0.04f, 0.02f }, pelo, alpha);
    }

    DibujarOjos(m, frente, A * 0.58f, 0.10f, { 0.07f, 0.085f, 0.02f }, AperturaOjosPose(pose, PERSONAJE_BRICO), pose.aturdido, alpha);

    // Nariz, bigote y sonrisa.
    Caja(m, { 0, A * 0.43f, frente + 0.025f }, { 0.07f, 0.07f, 0.05f }, Mezclar(f.piel, Color{ 200, 90, 70, 255 }, 0.25f), alpha);
    Caja(m, { 0, A * 0.31f, frente + 0.015f }, { 0.24f, 0.055f, 0.03f }, pelo, alpha);
    float boca = AperturaBoca(pose);
    Caja(m, { 0, A * 0.19f - 0.02f * boca, frente + 0.008f }, { 0.12f - 0.04f * boca, 0.028f + 0.06f * boca, 0.02f }, Color{ 120, 40, 40, 255 }, alpha);
}


static void CabezaPixi(
    const FormaPersonajeBloque& f,
    const Matrix& m,
    const PosePersonajeBloque& pose,
    float alpha
)
{
    const float A = f.altoCabeza;
    const float frente = f.fondoCabeza * 0.5f;
    const Color pelo = { 226, 96, 46, 255 };

    CajaBiselada(m, { 0, A * 0.5f, 0 }, { f.anchoCabeza, A, f.fondoCabeza }, 0.03f, f.piel, alpha);

    // Pelo: copete, nuca, mechones laterales y flequillo.
    Caja(m, { 0, A + 0.02f, -0.005f }, { f.anchoCabeza + 0.04f, 0.10f, f.fondoCabeza + 0.03f }, pelo, alpha);
    Caja(m, { 0, A * 0.60f, -frente - 0.015f }, { f.anchoCabeza + 0.04f, A * 0.80f, 0.05f }, pelo, alpha);

    for (int lado = -1; lado <= 1; lado += 2)
    {
        Caja(m, { lado * (f.anchoCabeza * 0.5f + 0.012f), A * 0.62f, -0.05f }, { 0.045f, A * 0.62f, f.fondoCabeza * 0.62f }, pelo, alpha);
    }

    Caja(m, { -0.06f, A * 0.88f, frente + 0.012f }, { f.anchoCabeza * 0.62f, 0.07f, 0.03f }, pelo, alpha);

    // Antiparras en la frente.
    Caja(m, { 0, A + 0.005f, -0.005f }, { f.anchoCabeza + 0.06f, 0.05f, f.fondoCabeza + 0.05f }, Color{ 70, 48, 34, 255 }, alpha);

    for (int lado = -1; lado <= 1; lado += 2)
    {
        float zLente = (f.fondoCabeza + 0.05f) * 0.5f;
        Caja(m, { lado * 0.09f, A + 0.01f, zLente + 0.012f }, { 0.12f, 0.10f, 0.025f }, Color{ 60, 64, 72, 255 }, alpha);
        Caja(m, { lado * 0.09f, A + 0.01f, zLente + 0.026f }, { 0.085f, 0.068f, 0.012f }, Color{ 120, 220, 240, 255 }, alpha, true);
    }

    DibujarOjos(m, frente, A * 0.56f, 0.09f, { 0.075f, 0.105f, 0.02f }, AperturaOjosPose(pose, PERSONAJE_PIXI), pose.aturdido, alpha);

    // Pecas y sonrisa abierta.
    Color peca = { 196, 112, 72, 255 };

    for (int lado = -1; lado <= 1; lado += 2)
    {
        for (int k = 0; k < 3; k++)
        {
            Caja(
                m,
                { lado * (0.10f + 0.03f * (k - 1)), A * (0.40f + 0.025f * (k % 2)), frente + 0.006f },
                { 0.02f, 0.02f, 0.012f },
                peca,
                alpha
            );
        }
    }

    float boca = AperturaBoca(pose);
    Caja(m, { 0, A * 0.24f - 0.015f * boca, frente + 0.008f }, { 0.09f - 0.02f * boca, 0.045f + 0.045f * boca, 0.02f }, Color{ 150, 50, 64, 255 }, alpha);

    // Coleta alta que se balancea.
    float vaiven =
        std::sin(pose.tiempo * 3.2f) * 0.10f +
        std::sin(pose.faseCaminar * 2.0f) * 0.22f * pose.caminar;

    Vector3 atadura = { 0, A + 0.06f, -frente + 0.02f };
    Matrix coleta = Hijo(Pivote(atadura, MatrixMultiply(MatrixRotateX(-0.55f - 0.25f * pose.tambaleo), MatrixRotateZ(vaiven))), m);

    Caja(m, atadura, { 0.08f, 0.08f, 0.08f }, Color{ 60, 170, 120, 255 }, alpha);
    Caja(coleta, { 0, atadura.y - 0.04f, atadura.z - 0.12f }, { 0.11f, 0.10f, 0.20f }, pelo, alpha);
    Caja(coleta, { 0, atadura.y - 0.08f, atadura.z - 0.25f }, { 0.08f, 0.08f, 0.10f }, Mezclar(pelo, WHITE, 0.10f), alpha);
}


static void CabezaRoboto(
    const FormaPersonajeBloque& f,
    const Matrix& m,
    const PosePersonajeBloque& pose,
    float alpha
)
{
    const float A = f.altoCabeza;
    const float frente = f.fondoCabeza * 0.5f;

    CajaBiselada(m, { 0, A * 0.5f, 0 }, { f.anchoCabeza, A, f.fondoCabeza }, 0.035f, f.piel, alpha);
    Caja(m, { 0, A + 0.012f, 0 }, { f.anchoCabeza * 0.5f, 0.03f, f.fondoCabeza * 0.5f }, Mezclar(f.piel, WHITE, 0.25f), alpha);

    // Tornillos laterales con luz.
    for (int lado = -1; lado <= 1; lado += 2)
    {
        Caja(m, { lado * (f.anchoCabeza * 0.5f + 0.025f), A * 0.52f, 0.0f }, { 0.05f, 0.13f, 0.13f }, Color{ 96, 104, 120, 255 }, alpha);
        Caja(m, { lado * (f.anchoCabeza * 0.5f + 0.052f), A * 0.52f, 0.0f }, { 0.012f, 0.05f, 0.05f }, Color{ 255, 90, 70, 255 }, alpha, true);
    }

    // Visor oscuro con ojos LED.
    Caja(m, { 0, A * 0.58f, frente + 0.012f }, { f.anchoCabeza * 0.86f, A * 0.36f, 0.03f }, Color{ 34, 40, 56, 255 }, alpha);

    float apertura = AperturaOjosPose(pose, PERSONAJE_ROBOTO);
    Color led = pose.aturdido ? Color{ 255, 120, 60, 255 } : Color{ 96, 240, 255, 255 };

    for (int lado = -1; lado <= 1; lado += 2)
    {
        float alto = pose.aturdido ? 0.02f : 0.06f * apertura;
        float ancho = apertura > 1.0f ? 0.11f * (1.0f + (apertura - 1.0f) * 0.4f) : 0.11f;
        Caja(m, { lado * 0.10f, A * 0.58f, frente + 0.032f }, { ancho, alto, 0.014f }, led, alpha, true);
    }

    // Boca de rejilla; al hablar o asustarse se enciende una barra LED.
    for (int k = 0; k < 3; k++)
    {
        Caja(m, { 0, A * 0.17f + k * 0.035f, frente + 0.008f }, { 0.17f, 0.018f, 0.02f }, Color{ 50, 56, 70, 255 }, alpha);
    }

    float boca = AperturaBoca(pose);

    if (boca > 0.0f)
    {
        Caja(m, { 0, A * 0.205f, frente + 0.02f }, { 0.12f, 0.012f + 0.05f * boca, 0.008f }, led, alpha, true);
    }

    // Antena con luz que late; se mece con el movimiento.
    float meneo =
        std::sin(pose.tiempo * 5.0f) * 0.06f +
        std::sin(pose.faseCaminar * 2.0f) * 0.18f * pose.caminar;

    Vector3 baseAntena = { 0.08f, A + 0.02f, 0.0f };
    Matrix antena = Hijo(Pivote(baseAntena, MatrixRotateZ(meneo - 0.12f * pose.tambaleo)), m);
    float latido = 0.65f + 0.35f * std::sin(pose.tiempo * 4.0f);

    Caja(antena, { baseAntena.x, baseAntena.y + 0.09f, 0.0f }, { 0.03f, 0.18f, 0.03f }, Color{ 120, 128, 144, 255 }, alpha);
    Caja(
        antena,
        { baseAntena.x, baseAntena.y + 0.21f, 0.0f },
        { 0.075f, 0.075f, 0.075f },
        Color{ 255, (unsigned char)(70 + 90 * latido), 70, 255 },
        alpha,
        true
    );
}


static void CabezaMomo(
    const FormaPersonajeBloque& f,
    const Matrix& m,
    const PosePersonajeBloque& pose,
    float alpha
)
{
    const float A = f.altoCabeza;
    const float frente = f.fondoCabeza * 0.5f;
    const Color crema = { 250, 228, 196, 255 };
    const Color rosa = { 246, 150, 170, 255 };

    CajaBiselada(m, { 0, A * 0.5f, 0 }, { f.anchoCabeza, A, f.fondoCabeza }, 0.04f, f.piel, alpha);

    // Orejas cubicas que se mueven un poco.
    for (int lado = -1; lado <= 1; lado += 2)
    {
        Vector3 base = { lado * 0.18f, A, -0.04f };
        float aleteo = std::sin(pose.tiempo * 2.6f + lado) * 0.08f + 0.25f * pose.tambaleo;
        Matrix oreja = Hijo(Pivote(base, MatrixRotateZ(-lado * (0.12f + aleteo))), m);

        Caja(oreja, { base.x, A + 0.07f, base.z }, { 0.14f, 0.15f, 0.08f }, f.piel, alpha);
        Caja(oreja, { base.x, A + 0.065f, base.z + 0.044f }, { 0.075f, 0.09f, 0.012f }, rosa, alpha);
    }

    // Mancha clara de la frente y hocico.
    Caja(m, { 0, A * 0.30f, frente + 0.035f }, { 0.24f, 0.14f, 0.07f }, crema, alpha);
    Caja(m, { 0, A * 0.37f, frente + 0.078f }, { 0.08f, 0.05f, 0.025f }, COLOR_OJO, alpha);
    float boca = AperturaBoca(pose);
    Caja(m, { 0, A * 0.25f - 0.015f * boca, frente + 0.074f }, { 0.07f - 0.02f * boca, 0.02f + 0.05f * boca, 0.02f }, Color{ 120, 60, 60, 255 }, alpha);

    DibujarOjos(m, frente, A * 0.60f, 0.135f, { 0.095f, 0.12f, 0.02f }, AperturaOjosPose(pose, PERSONAJE_MOMO), pose.aturdido, alpha);

    // Cachetes rosados.
    for (int lado = -1; lado <= 1; lado += 2)
    {
        Caja(m, { lado * 0.20f, A * 0.40f, frente + 0.006f }, { 0.08f, 0.04f, 0.012f }, rosa, alpha);
    }
}


static void CabezaVendedor(
    const FormaPersonajeBloque& f,
    const Matrix& m,
    const PosePersonajeBloque& pose,
    float alpha
)
{
    const float A = f.altoCabeza;
    const float frente = f.fondoCabeza * 0.5f;
    const Color pelo = { 70, 44, 30, 255 };
    const Color rojo = { 214, 58, 52, 255 };
    const Color blanco = { 250, 244, 232, 255 };

    CajaBiselada(m, { 0, A * 0.5f, 0 }, { f.anchoCabeza, A, f.fondoCabeza }, 0.035f, f.piel, alpha);

    // Orejas y patillas.
    for (int lado = -1; lado <= 1; lado += 2)
    {
        Caja(m, { lado * (f.anchoCabeza * 0.5f + 0.02f), A * 0.50f, 0.0f }, { 0.04f, 0.09f, 0.08f }, Mezclar(f.piel, BLACK, 0.08f), alpha);
        Caja(m, { lado * (f.anchoCabeza * 0.5f - 0.01f), A * 0.66f, frente - 0.10f }, { 0.04f, 0.16f, 0.12f }, pelo, alpha);
    }

    // Gorro de vendedor a rayas rojas y blancas con cinta verde y moneda.
    const float anchoGorro = f.anchoCabeza + 0.04f;
    const int franjas = 5;

    for (int k = 0; k < franjas; k++)
    {
        float x = -anchoGorro * 0.5f + anchoGorro * ((float)k + 0.5f) / (float)franjas;
        Caja(m, { x, A + 0.09f, 0 }, { anchoGorro / (float)franjas, 0.16f, f.fondoCabeza + 0.04f }, k % 2 == 0 ? rojo : blanco, alpha);
    }

    Caja(m, { 0, A + 0.03f, 0 }, { anchoGorro + 0.02f, 0.05f, f.fondoCabeza + 0.06f }, f.distintivo, alpha);
    Caja(m, { 0, A + 0.04f, frente + 0.045f }, { 0.07f, 0.07f, 0.02f }, Color{ 255, 206, 60, 255 }, alpha, true);
    Caja(m, { 0, A + 0.002f, frente + 0.07f }, { anchoGorro, 0.025f, 0.10f }, rojo, alpha);

    // Cejas pobladas (suben con el susto).
    for (int lado = -1; lado <= 1; lado += 2)
    {
        Caja(m, { lado * 0.10f, A * 0.74f + 0.03f * pose.susto, frente + 0.01f }, { 0.12f, 0.035f, 0.02f }, pelo, alpha);
    }

    DibujarOjos(m, frente, A * 0.60f, 0.10f, { 0.07f, 0.08f, 0.02f }, AperturaOjosPose(pose, PERSONAJE_VENDEDOR), pose.aturdido, alpha);

    // Nariz grande, cachetes, bigote de manubrio y boca.
    Caja(m, { 0, A * 0.44f, frente + 0.035f }, { 0.09f, 0.08f, 0.07f }, Mezclar(f.piel, Color{ 220, 100, 80, 255 }, 0.30f), alpha);

    for (int lado = -1; lado <= 1; lado += 2)
    {
        Caja(m, { lado * 0.15f, A * 0.40f, frente + 0.006f }, { 0.07f, 0.04f, 0.012f }, Color{ 246, 150, 140, 255 }, alpha);
        Caja(m, { lado * 0.08f, A * 0.31f, frente + 0.02f }, { 0.14f, 0.055f, 0.04f }, pelo, alpha);
        Caja(m, { lado * 0.165f, A * 0.35f, frente + 0.02f }, { 0.05f, 0.05f, 0.04f }, pelo, alpha);
    }

    float boca = AperturaBoca(pose);
    Caja(m, { 0, A * 0.19f - 0.015f * boca, frente + 0.008f }, { 0.11f - 0.03f * boca, 0.026f + 0.05f * boca, 0.02f }, Color{ 120, 40, 40, 255 }, alpha);
}


//==================================================
// TORSOS Y EXTREMIDADES
//==================================================

static void DetallesTorso(
    int personaje,
    const FormaPersonajeBloque& f,
    const Matrix& m,
    float yBase,
    Color remera,
    const PosePersonajeBloque& pose,
    float alpha
)
{
    const float T = f.altoTorso;
    const float frente = f.fondoTorso * 0.5f;

    if (personaje == PERSONAJE_BRICO)
    {
        // Cinturon de herramientas con hebilla y bolsillo en el pecho.
        Caja(m, { 0, yBase + 0.04f, 0 }, { f.anchoTorso + 0.02f, 0.07f, f.fondoTorso + 0.02f }, Color{ 120, 78, 42, 255 }, alpha);
        Caja(m, { 0, yBase + 0.04f, frente + 0.015f }, { 0.09f, 0.06f, 0.02f }, Color{ 250, 206, 70, 255 }, alpha);
        Caja(m, { 0.15f, yBase + 0.025f, frente + 0.03f }, { 0.08f, 0.10f, 0.05f }, Color{ 96, 62, 34, 255 }, alpha);
        Caja(m, { -0.14f, yBase + T * 0.70f, frente + 0.006f }, { 0.11f, 0.09f, 0.012f }, Mezclar(remera, BLACK, 0.22f), alpha);
    }
    else if (personaje == PERSONAJE_PIXI)
    {
        // Mochila con tiras.
        Color mochila = { 104, 128, 70, 255 };
        Caja(m, { 0, yBase + T * 0.52f, -frente - 0.065f }, { 0.34f, 0.32f, 0.13f }, mochila, alpha);
        Caja(m, { 0, yBase + T * 0.68f, -frente - 0.135f }, { 0.26f, 0.10f, 0.02f }, Mezclar(mochila, BLACK, 0.2f), alpha);

        for (int lado = -1; lado <= 1; lado += 2)
        {
            Caja(m, { lado * 0.12f, yBase + T * 0.55f, frente + 0.006f }, { 0.05f, T * 0.9f, 0.012f }, Mezclar(mochila, BLACK, 0.25f), alpha);
        }
    }
    else if (personaje == PERSONAJE_ROBOTO)
    {
        // Panel del pecho con pantalla y botones.
        Caja(m, { 0, yBase + T * 0.58f, frente + 0.01f }, { 0.26f, 0.19f, 0.022f }, Color{ 40, 46, 60, 255 }, alpha);

        float latido = 0.5f + 0.5f * std::sin(pose.tiempo * 3.0f);
        Caja(m, { -0.05f, yBase + T * 0.60f, frente + 0.024f }, { 0.08f, 0.08f, 0.01f }, Color{ 80, (unsigned char)(170 + 80 * latido), 140, 255 }, alpha, true);
        Caja(m, { 0.06f, yBase + T * 0.65f, frente + 0.026f }, { 0.04f, 0.04f, 0.02f }, Color{ 250, 80, 70, 255 }, alpha, true);
        Caja(m, { 0.06f, yBase + T * 0.52f, frente + 0.026f }, { 0.04f, 0.04f, 0.02f }, Color{ 250, 210, 60, 255 }, alpha, true);

        // Cintura metalica.
        Caja(m, { 0, yBase + 0.03f, 0 }, { f.anchoTorso * 0.86f, 0.07f, f.fondoTorso * 0.86f }, Color{ 96, 104, 120, 255 }, alpha);
    }
    else if (personaje == PERSONAJE_VENDEDOR)
    {
        // Delantal verde con tiras, bolsillo y moneda bordada.
        Color delantal = f.distintivo;
        Color oscuro = Mezclar(delantal, BLACK, 0.25f);

        Caja(m, { 0, yBase + T * 0.42f, frente + 0.012f }, { f.anchoTorso * 0.78f, T * 0.84f, 0.02f }, delantal, alpha);
        Caja(m, { 0, yBase - 0.10f, frente + 0.03f }, { f.anchoTorso * 0.86f, 0.22f, 0.02f }, delantal, alpha);
        Caja(m, { 0, yBase + 0.02f, frente + 0.026f }, { f.anchoTorso * 0.80f, 0.05f, 0.012f }, oscuro, alpha);

        for (int lado = -1; lado <= 1; lado += 2)
        {
            Caja(m, { lado * 0.17f, yBase + T * 0.92f, frente + 0.006f }, { 0.05f, T * 0.20f, 0.014f }, oscuro, alpha);
        }

        Caja(m, { 0, yBase + T * 0.38f, frente + 0.026f }, { 0.20f, 0.12f, 0.012f }, oscuro, alpha);
        Caja(m, { 0, yBase + T * 0.40f, frente + 0.034f }, { 0.07f, 0.07f, 0.01f }, Color{ 255, 206, 60, 255 }, alpha, true);
    }
    else
    {
        // Buzo con bolsillo canguro y capucha.
        Caja(m, { 0, yBase + T * 0.30f, frente + 0.008f }, { 0.28f, 0.11f, 0.018f }, Mezclar(remera, BLACK, 0.18f), alpha);
        Caja(m, { 0, yBase + T + 0.02f, -frente + 0.02f }, { 0.38f, 0.10f, 0.12f }, Mezclar(remera, BLACK, 0.10f), alpha);
    }
}


static void DibujarPierna(
    int personaje,
    const FormaPersonajeBloque& f,
    const Matrix& m,
    int lado,
    float alpha
)
{
    const float x = lado * f.separacionPiernas;
    const float alto = f.altoPierna;
    const float ancho = f.anchoPierna;
    const float fondo = ancho + 0.03f;

    if (personaje == PERSONAJE_PIXI)
    {
        // Short, pierna y botas altas.
        Caja(m, { x, alto * 0.80f, 0 }, { ancho + 0.02f, alto * 0.40f, fondo + 0.02f }, f.pantalon, alpha);
        Caja(m, { x, alto * 0.44f, 0 }, { ancho - 0.02f, alto * 0.36f, fondo - 0.03f }, f.piel, alpha);
        Caja(m, { x, alto * 0.14f, 0.02f }, { ancho + 0.03f, alto * 0.28f, fondo + 0.06f }, f.zapato, alpha);
        Caja(m, { x, alto * 0.27f, 0.02f }, { ancho + 0.04f, 0.03f, fondo + 0.07f }, Mezclar(f.zapato, WHITE, 0.2f), alpha);
        return;
    }

    float altoZapato = personaje == PERSONAJE_MOMO ? 0.08f : 0.10f;

    Caja(m, { x, (alto + altoZapato) * 0.5f, 0 }, { ancho, alto - altoZapato, fondo }, f.pantalon, alpha);

    if (personaje == PERSONAJE_ROBOTO)
    {
        Caja(m, { x, alto * 0.52f, 0 }, { ancho + 0.03f, 0.07f, fondo + 0.03f }, Color{ 70, 76, 90, 255 }, alpha);
    }

    float largoPie = personaje == PERSONAJE_ROBOTO ? fondo + 0.12f : fondo + 0.07f;

    Caja(m, { x, altoZapato * 0.5f, 0.03f }, { ancho + 0.03f, altoZapato, largoPie }, f.zapato, alpha);

    if (personaje == PERSONAJE_MOMO)
    {
        // Dedos claros de la pata.
        Caja(m, { x, altoZapato * 0.5f, 0.03f + largoPie * 0.5f }, { ancho, altoZapato * 0.6f, 0.02f }, Color{ 250, 228, 196, 255 }, alpha);
    }
}


static void DibujarBrazo(
    int personaje,
    const FormaPersonajeBloque& f,
    const Matrix& m,
    Vector3 hombro,
    Color remera,
    float alpha
)
{
    const float L = f.largoBrazo;
    const float a = f.anchoBrazo;

    // Manga (color del jugador) y antebrazo.
    Caja(m, { hombro.x, hombro.y - L * 0.22f, 0 }, { a + 0.015f, L * 0.48f, a + 0.015f }, remera, alpha);

    Color antebrazo = personaje == PERSONAJE_MOMO ? f.piel : (personaje == PERSONAJE_ROBOTO ? f.mano : f.piel);
    Caja(m, { hombro.x, hombro.y - L * 0.68f, 0 }, { a, L * 0.46f, a }, antebrazo, alpha);

    // Mano.
    Vector3 mano = { hombro.x, hombro.y - L - 0.045f, 0.0f };

    if (personaje == PERSONAJE_ROBOTO)
    {
        Caja(m, mano, { 0.15f, 0.10f, 0.15f }, Color{ 96, 104, 120, 255 }, alpha);

        for (int k = -1; k <= 1; k += 2)
        {
            Caja(m, { mano.x, mano.y - 0.07f, k * 0.045f }, { 0.05f, 0.06f, 0.035f }, Color{ 70, 76, 90, 255 }, alpha);
        }

        return;
    }

    Caja(m, mano, { 0.145f, 0.14f, 0.15f }, f.mano, alpha);

    if (personaje == PERSONAJE_BRICO)
    {
        // Guante de trabajo con punio.
        Caja(m, { mano.x, mano.y + 0.075f, 0 }, { 0.165f, 0.04f, 0.165f }, Mezclar(f.mano, BLACK, 0.2f), alpha);
    }
    else if (personaje == PERSONAJE_MOMO)
    {
        Caja(m, { mano.x, mano.y - 0.02f, 0.077f }, { 0.07f, 0.06f, 0.01f }, Color{ 246, 150, 170, 255 }, alpha);
    }
}


//==================================================
// ESTRELLITAS DE ATURDIMIENTO
//==================================================

static void DibujarEstrellitas(
    Vector3 centro,
    float factor,
    float tiempo,
    float alpha
)
{
    for (int k = 0; k < 3; k++)
    {
        float angulo = tiempo * 4.2f + k * 2.0944f;
        Vector3 p = {
            centro.x + std::cos(angulo) * 0.30f * factor,
            centro.y + std::sin(tiempo * 6.0f + k) * 0.04f * factor,
            centro.z + std::sin(angulo) * 0.30f * factor
        };

        Matrix giro = MatrixMultiply(
            MatrixMultiply(MatrixScale(factor, factor, factor), MatrixRotateY(-angulo)),
            MatrixTranslate(p.x, p.y, p.z)
        );

        Color estrella = k == 1 ? Color{ 255, 250, 200, 255 } : Color{ 255, 214, 60, 255 };

        Caja(giro, { 0, 0, 0 }, { 0.12f, 0.035f, 0.035f }, estrella, alpha, true);
        Caja(giro, { 0, 0, 0 }, { 0.035f, 0.12f, 0.035f }, estrella, alpha, true);
    }
}


//==================================================
// POSE Y DIBUJO
//==================================================

// Angulos objetivo de un brazo para un gesto. lado -1 = brazo derecho.
// pitch < 0 lleva el brazo al frente/arriba; roll > 0 lo abre hacia afuera.
// Devuelve false si el gesto no mueve ese brazo.
static bool ObjetivoBrazoGesto(
    GestoPersonajeBloque gesto,
    int lado,
    float tiempo,
    float& pitch,
    float& roll
)
{
    switch (gesto)
    {
        case GESTO_SALUDAR:
            if (lado > 0) return false;
            pitch = -0.25f;
            roll = 2.55f + 0.35f * std::sin(tiempo * 11.0f);
            return true;

        case GESTO_SENALAR:
            if (lado > 0) return false;
            pitch = -1.50f;
            roll = 0.22f;
            return true;

        case GESTO_PRESENTAR:
            pitch = -0.75f;
            roll = 0.95f + 0.05f * std::sin(tiempo * 3.0f + lado);
            return true;

        case GESTO_FESTEJAR:
            pitch = -0.30f;
            roll = 2.65f + 0.20f * std::sin(tiempo * 9.0f + lado * 1.4f);
            return true;

        case GESTO_MANOS_CABEZA:
            pitch = -2.80f;
            roll = 0.32f + 0.05f * std::sin(tiempo * 14.0f + lado);
            return true;

        case GESTO_SORPRESA:
            pitch = -0.95f;
            roll = 1.25f;
            return true;

        case GESTO_NINGUNO:
            break;
    }

    return false;
}

// Angulos de los brazos y del torso para el golpe segun su progreso:
// anticipacion (brazo atras), impacto (brazo al frente), sostener y volver.
struct PoseGolpe
{
    float brazo = 0.0f;
    float otroBrazo = 0.0f;
    float giroTorso = 0.0f;
    float inclinacion = 0.0f;
    float avance = 0.0f;
    float peso = 0.0f;
};


static PoseGolpe CalcularPoseGolpe(
    float p
)
{
    PoseGolpe g;

    if (p < 0.0f || p > 1.0f)
    {
        return g;
    }

    const float ANTICIPACION = 0.18f;
    const float IMPACTO = 0.40f;
    const float SOSTENER = 0.58f;

    g.peso = 1.0f;

    if (p < ANTICIPACION)
    {
        float k = Suavizar(p / ANTICIPACION);
        g.brazo = 0.95f * k;
        g.otroBrazo = -0.35f * k;
        g.giroTorso = -0.32f * k;
        g.inclinacion = -0.08f * k;
        return g;
    }

    float k = SalidaRapida((p - ANTICIPACION) / (IMPACTO - ANTICIPACION));

    g.brazo = 0.95f + (-1.62f - 0.95f) * k;
    g.otroBrazo = -0.35f + (0.55f + 0.35f) * k;
    g.giroTorso = -0.32f + (0.40f + 0.32f) * k;
    g.inclinacion = -0.08f + (0.14f + 0.08f) * k;
    g.avance = 0.10f * k;

    if (p > SOSTENER)
    {
        g.peso = 1.0f - Suavizar((p - SOSTENER) / (1.0f - SOSTENER));
    }

    return g;
}


// Cuerpo comun de los personajes de bloques. "personaje" ya es un indice
// valido de FORMAS (0..3 jugables o PERSONAJE_VENDEDOR).
static void DibujarCuerpoBloque(
    int personaje,
    Vector3 pies,
    float anguloY,
    float escala,
    Color colorJugador,
    const PosePersonajeBloque& pose
)
{
    const FormaPersonajeBloque& f = FORMAS[personaje];
    const float alpha = pose.alpha;

    if (escala <= 0.0f || alpha <= 0.01f)
    {
        return;
    }

    // Remera algo aclarada: los colores oscuros de jugador siguen leyendo.
    Color remera = Mezclar(colorJugador, WHITE, 0.10f);
    remera.a = 255;

    const float factor = escala / 0.25f;

    //------------------------------
    // RAIZ (posicion, giro, escala y caida)
    //------------------------------

    float caida = Suavizar(pose.caida);
    float caminar = pose.caminar * (1.0f - caida);
    float fase = pose.faseCaminar;

    PoseGolpe golpe = CalcularPoseGolpe(caida > 0.01f ? -1.0f : pose.golpe);

    float rebote = 0.035f * caminar * (0.5f - 0.5f * std::cos(fase * 2.0f));

    Matrix raiz = MatrixMultiply(
        MatrixMultiply(
            MatrixScale(factor, factor * pose.escalaVertical, factor),
            MatrixRotateY(anguloY * DEG2RAD)
        ),
        MatrixTranslate(pies.x, pies.y, pies.z)
    );

    // Caer de espaldas: giro alrededor del talon trasero.
    Matrix local = MatrixTranslate(0.0f, rebote, golpe.avance * golpe.peso);

    if (caida > 0.001f)
    {
        float apoyo = -f.fondoEspalda - 0.01f;
        local = MatrixMultiply(local, Pivote({ 0.0f, 0.0f, apoyo }, MatrixRotateX(-PI * 0.5f * caida)));
    }

    Matrix cuerpo = Hijo(local, raiz);

    //------------------------------
    // PIERNAS
    //------------------------------

    const float yCadera = f.altoPierna;

    for (int lado = -1; lado <= 1; lado += 2)
    {
        float angulo = lado * 0.62f * caminar * std::sin(fase);

        if (pose.enAire && caida < 0.01f)
        {
            angulo = lado > 0 ? -0.55f : 0.30f;
        }

        angulo -= 0.30f * caida;

        Matrix pierna = Hijo(
            Pivote({ lado * f.separacionPiernas, yCadera, 0.0f }, MatrixRotateX(angulo)),
            cuerpo
        );

        DibujarPierna(personaje, f, pierna, lado, alpha);
    }

    //------------------------------
    // TRONCO (torso, brazos y cabeza)
    //------------------------------

    float respiracion = 0.006f * std::sin(pose.tiempo * 2.2f + personaje);
    float inclinacion =
        golpe.inclinacion * golpe.peso -
        0.38f * pose.tambaleo +
        0.05f * caminar;
    float giroTorso =
        golpe.giroTorso * golpe.peso +
        0.06f * caminar * std::sin(fase);

    Matrix tronco = Hijo(
        MatrixMultiply(
            Pivote({ 0.0f, yCadera, 0.0f }, MatrixMultiply(MatrixRotateX(inclinacion), MatrixRotateY(giroTorso))),
            MatrixTranslate(0.0f, respiracion, 0.0f)
        ),
        cuerpo
    );

    const float yTorso = yCadera + 0.02f;

    // Cadera/cinturon con el color del pantalon.
    Caja(tronco, { 0, yCadera, 0 }, { f.separacionPiernas * 2.0f + f.anchoPierna, 0.08f, f.anchoPierna + 0.05f }, f.pantalon, alpha);

    CajaBiselada(
        tronco,
        { 0, yTorso + f.altoTorso * 0.5f, 0 },
        { f.anchoTorso, f.altoTorso, f.fondoTorso },
        0.03f,
        remera,
        alpha
    );

    DetallesTorso(personaje, f, tronco, yTorso, remera, pose, alpha);

    if (personaje == PERSONAJE_MOMO)
    {
        // Cola cuadrada que se menea.
        Vector3 baseCola = { 0.0f, yCadera + 0.06f, -f.fondoTorso * 0.5f };
        float meneo = std::sin(pose.tiempo * 6.5f) * (0.25f + 0.35f * caminar);
        Matrix cola = Hijo(Pivote(baseCola, MatrixMultiply(MatrixRotateX(0.65f), MatrixRotateY(meneo))), tronco);

        Caja(cola, { 0, baseCola.y, baseCola.z - 0.09f }, { 0.10f, 0.10f, 0.20f }, f.piel, alpha);
        Caja(cola, { 0, baseCola.y, baseCola.z - 0.23f }, { 0.12f, 0.12f, 0.10f }, Color{ 250, 228, 196, 255 }, alpha);
    }

    // Brazos: lado -1 es el brazo derecho del personaje (el que golpea).
    const float yHombro = yTorso + f.altoTorso - 0.07f;

    for (int lado = -1; lado <= 1; lado += 2)
    {
        Vector3 hombro = {
            lado * (f.anchoTorso * 0.5f + f.anchoBrazo * 0.5f - 0.01f),
            yHombro,
            0.0f
        };

        float pitch = -lado * 0.50f * caminar * std::sin(fase);
        float roll = 0.08f + 0.02f * std::sin(pose.tiempo * 2.2f + lado);

        if (pose.enAire && caida < 0.01f)
        {
            pitch = -0.35f;
            roll = 0.95f;
        }

        if (golpe.peso > 0.0f)
        {
            float objetivo = lado < 0 ? golpe.brazo : golpe.otroBrazo;
            pitch += (objetivo - pitch) * golpe.peso;
        }

        // Tambaleo: brazos abiertos para no perder el equilibrio.
        roll += 0.70f * pose.tambaleo;

        // Gesto (saludar, senalar...) mezclado con la pose base.
        if (pose.pesoGesto > 0.0f && caida < 0.01f)
        {
            float pitchGesto = 0.0f;
            float rollGesto = 0.0f;

            if (ObjetivoBrazoGesto(pose.gesto, lado, pose.tiempo, pitchGesto, rollGesto))
            {
                float w = pose.pesoGesto > 1.0f ? 1.0f : pose.pesoGesto;
                pitch += (pitchGesto - pitch) * w;
                roll += (rollGesto - roll) * w;
            }
        }

        // Caida: brazos por encima de la cabeza y abiertos.
        pitch += (-2.55f - pitch) * caida;
        roll += 0.45f * caida;

        Matrix brazo = Hijo(
            Pivote(hombro, MatrixMultiply(MatrixRotateZ(lado * roll), MatrixRotateX(pitch))),
            tronco
        );

        DibujarBrazo(personaje, f, brazo, hombro, remera, alpha);
    }

    // Cabeza: cabecea al caminar y se bambolea aturdida.
    float yCuello = yTorso + f.altoTorso;
    float bamboleo = pose.aturdido ? 0.16f * std::sin(pose.tiempo * 7.0f) : 0.0f;
    float cabeceo = 0.04f * caminar * std::sin(fase * 2.0f) - 0.20f * pose.tambaleo;

    Caja(tronco, { 0, yCuello + 0.012f, 0 }, { 0.16f, 0.04f, 0.16f }, Mezclar(f.piel, BLACK, 0.12f), alpha);

    Matrix cabeza = Hijo(
        MatrixMultiply(
            MatrixMultiply(MatrixMultiply(MatrixRotateY(pose.giroCabeza), MatrixRotateZ(bamboleo)), MatrixRotateX(cabeceo)),
            MatrixTranslate(0.0f, yCuello + 0.025f, 0.0f)
        ),
        tronco
    );

    switch (personaje)
    {
        case PERSONAJE_BRICO: CabezaBrico(f, cabeza, pose, alpha); break;
        case PERSONAJE_PIXI: CabezaPixi(f, cabeza, pose, alpha); break;
        case PERSONAJE_ROBOTO: CabezaRoboto(f, cabeza, pose, alpha); break;
        case PERSONAJE_VENDEDOR: CabezaVendedor(f, cabeza, pose, alpha); break;
        default: CabezaMomo(f, cabeza, pose, alpha); break;
    }

    if (pose.aturdido)
    {
        // Se dibujan en el espacio del llamador (horizontales aunque el
        // personaje este acostado).
        Vector3 coronilla = Vector3Transform({ 0.0f, f.altoCabeza + 0.16f, 0.0f }, cabeza);

        if (caida > 0.5f)
        {
            coronilla.y += 0.25f * factor * caida;
        }

        DibujarEstrellitas(coronilla, factor, pose.tiempo, alpha);
    }
}


void DibujarPersonajeBloque(
    int personaje,
    Vector3 pies,
    float anguloY,
    float escala,
    Color colorJugador,
    const PosePersonajeBloque& pose
)
{
    DibujarCuerpoBloque(NormalizarPersonajeBloque(personaje), pies, anguloY, escala, colorJugador, pose);
}


void DibujarVendedorBloque(
    Vector3 pies,
    float anguloY,
    float escala,
    const PosePersonajeBloque& pose
)
{
    // Camisa crema bajo el delantal.
    DibujarCuerpoBloque(PERSONAJE_VENDEDOR, pies, anguloY, escala, Color{ 244, 232, 210, 255 }, pose);
}


const char* ObtenerNombreVendedorBloque()
{
    return FORMAS[PERSONAJE_VENDEDOR].nombre;
}
