#include "Minigames/MinijuegoPelotas.h"

#include "Minigames/AudioMinijuegos.h"
#include "Minigames/BotsMinijuegos1v3.h"
#include "Minigames/EfectosVisualesMinijuegos.h"
#include "Minigames/HudMinijuego.h"
#include "Minigames/UtilidadesMinijuegos.h"
#include "Systems/CalidadGrafica.h"
#include "UI/EstiloUI.h"
#include "rlgl.h"

#include <cmath>
#include <cstdio>


static const float DURACION_PREPARACION_PELOTAS = 3.0f;
static const float DURACION_PARTIDA_PELOTAS = 60.0f;
static const float GRACIA_INICIAL_PELOTAS = 1.2f;

static const float VELOCIDAD_MAXIMA_PELOTAS = 9.0f;
static const float ACELERACION_MAXIMA_PELOTAS = 6.0f;
static const float MULTIPLICADOR_EMPUJE_CHOQUE_PELOTAS = 1.40f;
static const float VELOCIDAD_MAXIMA_LANZAMIENTO_PELOTAS = 13.5f;

// La colision superior usa exactamente el mismo borde irregular que se
// dibuja. La colision lateral conserva ese radio hacia abajo: la montana
// visual se abre hacia la base, pero su nucleo solido no debe empujar a los
// jugadores hasta el contorno exterior decorativo.
static const float RADIO_ARENA_PELOTAS = 6.35f;
static const int SEGMENTOS_ARENA_PELOTAS = 48;
static const float ALTURA_BASE_MONTANA_PELOTAS = -2.10f;
static const float ALTURA_DEBUG_COLISION_MONTANA_PELOTAS = -1.70f;

// Solo visual.
static const float DURACION_AVISO_PELOTAS = 1.6f;
static const float VIDA_HUELLA_PELOTAS = 2.4f;
static const float MARGEN_AVISO_BORDE_PELOTAS = 0.8f;


static float MagnitudHorizontalPelotas(float x, float z)
{
    return std::sqrt(x * x + z * z);
}


static float FactorIrregularidadBordePelotas(float angulo)
{
    return
        1.0f +
        0.018f * std::sin(angulo * 5.0f) +
        0.012f * std::cos(angulo * 9.0f);
}


static float RadioVisualBordePelotas(float x, float z)
{
    float angulo = std::atan2(z, x);
    return RADIO_ARENA_PELOTAS * FactorIrregularidadBordePelotas(angulo);
}


static float RadioSoportePelotas(
    float x,
    float z
)
{
    return RadioVisualBordePelotas(x, z);
}


static void LimitarMovimientoPelota(
    JugadorPrueba& jugador,
    float velocidadAnteriorX,
    float velocidadAnteriorZ,
    float deltaTime
)
{
    float anterior = MagnitudHorizontalPelotas(
        velocidadAnteriorX,
        velocidadAnteriorZ
    );

    float actual = MagnitudHorizontalPelotas(
        jugador.velocidad.x,
        jugador.velocidad.z
    );

    if (actual <= 0.001f)
    {
        return;
    }

    float limiteAceleracion =
        anterior + ACELERACION_MAXIMA_PELOTAS * deltaTime;

    float limite =
        limiteAceleracion < VELOCIDAD_MAXIMA_PELOTAS
        ? limiteAceleracion
        : VELOCIDAD_MAXIMA_PELOTAS;

    if (actual > limite)
    {
        float factor = limite / actual;
        jugador.velocidad.x *= factor;
        jugador.velocidad.z *= factor;
    }
}


static void AplicarEmpujePendientePelotas(
    JugadorPrueba& jugador,
    Vector3 empujePendiente
)
{
    jugador.velocidad.x += empujePendiente.x;
    jugador.velocidad.z += empujePendiente.z;

    float velocidad = MagnitudHorizontalPelotas(
        jugador.velocidad.x,
        jugador.velocidad.z
    );

    if (velocidad > VELOCIDAD_MAXIMA_LANZAMIENTO_PELOTAS)
    {
        float factor = VELOCIDAD_MAXIMA_LANZAMIENTO_PELOTAS / velocidad;
        jugador.velocidad.x *= factor;
        jugador.velocidad.z *= factor;
    }
}


static bool JugadorSobreArenaCircularPelotas(
    const JugadorPrueba& jugador,
    const Camera3D& camara
)
{
    // La linea debug esta debajo del suelo. Proyectamos la posicion del
    // jugador desde la camara hasta ese mismo plano antes de comprobar el
    // radio; asi el borde visible y el punto donde desaparece el suelo
    // coinciden exactamente en pantalla.
    float divisor = -camara.position.y;
    float factorProyeccion = 1.0f;

    if (std::fabs(divisor) > 0.001f)
    {
        factorProyeccion =
            (ALTURA_DEBUG_COLISION_MONTANA_PELOTAS - camara.position.y) /
            divisor;
    }

    float xPlanoColision =
        camara.position.x +
        (jugador.posicion.x - camara.position.x) * factorProyeccion;

    float zPlanoColision =
        camara.position.z +
        (jugador.posicion.z - camara.position.z) * factorProyeccion;

    float distancia = MagnitudHorizontalPelotas(
        xPlanoColision,
        zPlanoColision
    );

    float radioSoporte = RadioSoportePelotas(
        xPlanoColision,
        zPlanoColision
    );

    return distancia <= radioSoporte;
}


static void ResolverColisionMontanaPelotas(
    JugadorPrueba& jugador,
    Vector3 posicionAnterior
)
{
    float radioPelota = jugador.tamano.x / 2.0f;
    float distancia = MagnitudHorizontalPelotas(
        jugador.posicion.x,
        jugador.posicion.z
    );

    bool tocaAlturaMontana =
        jugador.posicion.y - radioPelota < 0.0f &&
        jugador.posicion.y + radioPelota > ALTURA_BASE_MONTANA_PELOTAS;

    if (tocaAlturaMontana)
    {
        float radioColision = RadioVisualBordePelotas(
            jugador.posicion.x,
            jugador.posicion.z
        );

        float distanciaMinima =
            radioColision + radioPelota * 0.82f;

        float zonaCercanaAlBorde =
            radioColision - radioPelota * 0.45f;

        if (
            distancia > zonaCercanaAlBorde &&
            distancia < distanciaMinima
        )
        {
            float normalX = 1.0f;
            float normalZ = 0.0f;

            if (distancia > 0.001f)
            {
                normalX = jugador.posicion.x / distancia;
                normalZ = jugador.posicion.z / distancia;
            }
            else
            {
                float anterior = MagnitudHorizontalPelotas(
                    posicionAnterior.x,
                    posicionAnterior.z
                );

                if (anterior > 0.001f)
                {
                    normalX = posicionAnterior.x / anterior;
                    normalZ = posicionAnterior.z / anterior;
                }
            }

            jugador.posicion.x = normalX * distanciaMinima;
            jugador.posicion.z = normalZ * distanciaMinima;

            float velocidadHaciaCentro =
                jugador.velocidad.x * normalX +
                jugador.velocidad.z * normalZ;

            if (velocidadHaciaCentro < 0.0f)
            {
                jugador.velocidad.x -= normalX * velocidadHaciaCentro;
                jugador.velocidad.z -= normalZ * velocidadHaciaCentro;
            }
        }
    }

    float radioBase = RadioVisualBordePelotas(
        jugador.posicion.x,
        jugador.posicion.z
    );

    float parteSuperiorAnterior = posicionAnterior.y + radioPelota;
    float parteSuperiorActual = jugador.posicion.y + radioPelota;

    if (
        posicionAnterior.y < ALTURA_BASE_MONTANA_PELOTAS &&
        jugador.velocidad.y > 0.0f &&
        parteSuperiorAnterior <= ALTURA_BASE_MONTANA_PELOTAS &&
        parteSuperiorActual >= ALTURA_BASE_MONTANA_PELOTAS &&
        distancia <= radioBase + radioPelota
    )
    {
        jugador.posicion.y =
            ALTURA_BASE_MONTANA_PELOTAS - radioPelota - 0.001f;
        jugador.velocidad.y = 0.0f;
    }
}


static void PotenciarEmpujesPelotas(
    JugadorPrueba jugadores[],
    const Participante participantes[],
    int cantidadMaxima
)
{
    for (int i = 0; i < cantidadMaxima; i++)
    {
        if (
            !participantes[i].activo ||
            !participantes[i].conectado ||
            jugadores[i].cayendo
        )
        {
            continue;
        }

        jugadores[i].empuje.x *= MULTIPLICADOR_EMPUJE_CHOQUE_PELOTAS;
        jugadores[i].empuje.z *= MULTIPLICADOR_EMPUJE_CHOQUE_PELOTAS;
    }
}


static float IrregularidadBordePelotas(int indice)
{
    float angulo =
        (2.0f * PI * (float)indice) /
        (float)SEGMENTOS_ARENA_PELOTAS;

    return FactorIrregularidadBordePelotas(angulo);
}


static Vector3 PuntoCircularPelotas(
    int indice,
    float radio,
    float altura
)
{
    float angulo =
        (2.0f * PI * (float)indice) /
        (float)SEGMENTOS_ARENA_PELOTAS;

    float irregularidad = IrregularidadBordePelotas(indice);

    return
    {
        std::cos(angulo) * radio * irregularidad,
        altura,
        std::sin(angulo) * radio * irregularidad
    };
}


static Vector3 PuntoContornoColisionMontanaPelotas(int indice)
{
    return PuntoCircularPelotas(
        indice,
        RADIO_ARENA_PELOTAS,
        ALTURA_DEBUG_COLISION_MONTANA_PELOTAS
    );
}


static void DibujarContornoColisionMontanaPelotas(
    const Camera3D& camara
)
{
    for (int i = 0; i < SEGMENTOS_ARENA_PELOTAS; i++)
    {
        int siguiente = (i + 1) % SEGMENTOS_ARENA_PELOTAS;

        Vector2 inicio = GetWorldToScreen(
            PuntoContornoColisionMontanaPelotas(i),
            camara
        );

        Vector2 fin = GetWorldToScreen(
            PuntoContornoColisionMontanaPelotas(siguiente),
            camara
        );

        DrawLineEx(inicio, fin, 3.0f, RED);
    }
}


//==================================================
// GEOMETRIA VISIBLE DE LA CIMA
//==================================================
//
// El soporte real se decide proyectando al jugador desde la camara hasta
// el plano del contorno debug (y = -1,70). A la altura del suelo (y = 0)
// esa zona es el mismo contorno irregular, achicado y corrido hacia la
// camara. La cima se dibuja exactamente ahi: el borde de nieve que se ve
// es el borde por el que se cae, de cualquier lado de la arena.

struct CimaPelotas
{
    float centroX = 0.0f;
    float centroZ = 0.0f;
    float escala = 1.0f;
};


static CimaPelotas CalcularCimaPelotas(const Camera3D& camara)
{
    CimaPelotas cima;
    float bajada = camara.position.y - ALTURA_DEBUG_COLISION_MONTANA_PELOTAS;

    if (std::fabs(camara.position.y) < 0.001f || std::fabs(bajada) < 0.001f)
    {
        return cima;
    }

    cima.escala = camara.position.y / bajada;
    cima.centroX = camara.position.x * (1.0f - cima.escala);
    cima.centroZ = camara.position.z * (1.0f - cima.escala);
    return cima;
}


static float AnguloSegmentoPelotas(int indice)
{
    return (2.0f * PI * (float)indice) / (float)SEGMENTOS_ARENA_PELOTAS;
}


static Vector3 PuntoCimaPelotas(
    const CimaPelotas& cima,
    float angulo,
    float factor,
    float altura
)
{
    float radio =
        RADIO_ARENA_PELOTAS *
        FactorIrregularidadBordePelotas(angulo) *
        cima.escala *
        factor;

    return
    {
        cima.centroX + std::cos(angulo) * radio,
        altura,
        cima.centroZ + std::sin(angulo) * radio
    };
}


// Distancia (positiva adentro) entre el jugador y el borde por el que cae.
static float MargenBordePelotas(const CimaPelotas& cima, float x, float z)
{
    float dx = x - cima.centroX;
    float dz = z - cima.centroZ;
    float angulo = std::atan2(dz, dx);
    float radio =
        RADIO_ARENA_PELOTAS *
        FactorIrregularidadBordePelotas(angulo) *
        cima.escala;

    return radio - MagnitudHorizontalPelotas(dx, dz);
}


static Vector3 PuntoBordeCercanoPelotas(const CimaPelotas& cima, Vector3 posicion)
{
    float angulo = std::atan2(posicion.z - cima.centroZ, posicion.x - cima.centroX);
    return PuntoCimaPelotas(cima, angulo, 1.0f, 0.04f);
}


//==================================================
// UTILIDADES VISUALES
//==================================================

static float AzarPelotas(float minimo, float maximo)
{
    return minimo + (maximo - minimo) * (float)GetRandomValue(0, 10000) / 10000.0f;
}


static float Limitar01Pelotas(float valor)
{
    if (valor < 0.0f) return 0.0f;
    if (valor > 1.0f) return 1.0f;
    return valor;
}


static float Hash01Pelotas(int a, int b)
{
    unsigned int h = ((unsigned int)a * 73856093u) ^ ((unsigned int)b * 19349663u);
    h ^= h >> 13;
    h *= 0x5bd1e995u;
    h ^= h >> 15;
    return (float)(h & 0xFFFFu) / 65535.0f;
}


static Color TintePelotas(Color c, float f)
{
    float r = c.r * f;
    float g = c.g * f;
    float b = c.b * f;
    if (r > 255.0f) r = 255.0f;
    if (g > 255.0f) g = 255.0f;
    if (b > 255.0f) b = 255.0f;
    return Color{ (unsigned char)r, (unsigned char)g, (unsigned char)b, c.a };
}


static Color MezclarPelotas(Color a, Color b, float t)
{
    t = Limitar01Pelotas(t);
    return Color
    {
        (unsigned char)(a.r + (b.r - a.r) * t),
        (unsigned char)(a.g + (b.g - a.g) * t),
        (unsigned char)(a.b + (b.b - a.b) * t),
        (unsigned char)(a.a + (b.a - a.a) * t)
    };
}


static const Color COLOR_NIEBLA_PELOTAS = { 203, 221, 236, 255 };
static const Color COLOR_CIELO_ALTO_PELOTAS = { 74, 120, 182, 255 };
static const Color COLOR_NIEVE_PELOTAS = { 241, 246, 251, 255 };
static const Color COLOR_NIEVE_SOMBRA_PELOTAS = { 212, 226, 240, 255 };
static const Color COLOR_HIELO_PELOTAS = { 160, 204, 234, 255 };
static const Color COLOR_HIELO_OSCURO_PELOTAS = { 112, 160, 202, 255 };
static const Color COLOR_ROCA_PELOTAS = { 84, 94, 112, 255 };

// Luz fria que llega desde la izquierda y de frente (angulo en XZ).
static const float ANGULO_LUZ_PELOTAS = 2.21f;


// Niebla de profundidad: lo lejano (z negativo) y lo profundo se funden
// con el aire frio del valle.
static float FactorNieblaPelotas(Vector3 p)
{
    float porLejania = Limitar01Pelotas((-p.z - 10.0f) / 95.0f);
    float porProfundidad = Limitar01Pelotas((-p.y - 7.0f) / 18.0f) * 0.55f;
    float total = porLejania + porProfundidad;
    return total > 0.86f ? 0.86f : total;
}


static Color NieblaPelotas(Color c, Vector3 p)
{
    Color niebla = { COLOR_NIEBLA_PELOTAS.r, COLOR_NIEBLA_PELOTAS.g, COLOR_NIEBLA_PELOTAS.b, c.a };
    return MezclarPelotas(c, niebla, FactorNieblaPelotas(p));
}


static float LuzCaraPelotas(float angulo)
{
    return 0.72f + 0.28f * std::cos(angulo - ANGULO_LUZ_PELOTAS);
}


static void CulloPelotas(bool activo)
{
    rlDrawRenderBatchActive();
    if (activo) rlEnableBackfaceCulling();
    else rlDisableBackfaceCulling();
}


static void VerticePelotas(Vector3 p, Color c)
{
    rlColor4ub(c.r, c.g, c.b, c.a);
    rlVertex3f(p.x, p.y, p.z);
}


// Cuadrilatero con un color por vertice (degrade suave sin texturas).
static void CuadPelotas(
    Vector3 a0, Vector3 a1, Vector3 b1, Vector3 b0,
    Color ca0, Color ca1, Color cb1, Color cb0
)
{
    rlBegin(RL_TRIANGLES);
    VerticePelotas(a0, ca0);
    VerticePelotas(a1, ca1);
    VerticePelotas(b1, cb1);
    VerticePelotas(a0, ca0);
    VerticePelotas(b1, cb1);
    VerticePelotas(b0, cb0);
    rlEnd();
}


// Caja con cada cara a un brillo distinto: volumen sin luces reales.
static void CajaFacetadaPelotas(Vector3 centro, Vector3 tamano, Color color, float brilloTapa)
{
    float x0 = centro.x - tamano.x * 0.5f;
    float x1 = centro.x + tamano.x * 0.5f;
    float y0 = centro.y - tamano.y * 0.5f;
    float y1 = centro.y + tamano.y * 0.5f;
    float z0 = centro.z - tamano.z * 0.5f;
    float z1 = centro.z + tamano.z * 0.5f;

    Color tapa = TintePelotas(color, brilloTapa);
    Color frente = TintePelotas(color, 1.0f);
    Color atras = TintePelotas(color, 0.58f);
    Color derecha = TintePelotas(color, 0.78f);
    Color izquierda = TintePelotas(color, 0.66f);

    DrawTriangle3D({ x0, y1, z0 }, { x0, y1, z1 }, { x1, y1, z1 }, tapa);
    DrawTriangle3D({ x0, y1, z0 }, { x1, y1, z1 }, { x1, y1, z0 }, tapa);
    DrawTriangle3D({ x0, y0, z1 }, { x1, y0, z1 }, { x1, y1, z1 }, frente);
    DrawTriangle3D({ x0, y0, z1 }, { x1, y1, z1 }, { x0, y1, z1 }, frente);
    DrawTriangle3D({ x0, y0, z0 }, { x0, y1, z0 }, { x1, y1, z0 }, atras);
    DrawTriangle3D({ x0, y0, z0 }, { x1, y1, z0 }, { x1, y0, z0 }, atras);
    DrawTriangle3D({ x1, y0, z0 }, { x1, y1, z0 }, { x1, y1, z1 }, derecha);
    DrawTriangle3D({ x1, y0, z0 }, { x1, y1, z1 }, { x1, y0, z1 }, derecha);
    DrawTriangle3D({ x0, y0, z0 }, { x0, y0, z1 }, { x0, y1, z1 }, izquierda);
    DrawTriangle3D({ x0, y0, z0 }, { x0, y1, z1 }, { x0, y1, z0 }, izquierda);
}


static void RocaGiradaPelotas(Vector3 centro, Vector3 tamano, float angulo, Color color)
{
    rlPushMatrix();
    rlTranslatef(centro.x, centro.y, centro.z);
    rlRotatef(angulo, 0.0f, 1.0f, 0.0f);
    CajaFacetadaPelotas({ 0.0f, 0.0f, 0.0f }, tamano, color, 1.25f);
    rlPopMatrix();
}


// Disco de luz (centro opaco, borde transparente). Solo con mezcla aditiva.
static void ResplandorPelotas(Vector3 centro, Vector3 u, Vector3 v, float radio, Color color)
{
    const int LADOS = 14;
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


static void ResplandorSueloPelotas(Vector3 centro, float radio, Color color)
{
    ResplandorPelotas(centro, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, radio, color);
}


// Halo vertical que mira a la camara (fija): ventanas, destellos.
static void ResplandorFrontalPelotas(Vector3 centro, float radio, Color color)
{
    ResplandorPelotas(centro, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.84f, -0.54f }, radio, color);
}


//==================================================
// EFECTOS: PARTICULAS, ONDAS, HUELLAS Y AVISOS
//==================================================

static int CantidadEscaladaPelotas(int cantidad)
{
    float factor = FactorCalidadGrafica(CalidadParticulas());
    int escalada = (int)std::lround((float)cantidad * factor);
    return escalada < 1 ? 1 : escalada;
}


static ParticulaPelotas* ReservarParticulaPelotas(MinijuegoPelotas& m)
{
    for (int i = 0; i < MAX_PARTICULAS_PELOTAS; i++)
    {
        if (!m.particulasNieve[i].activa) return &m.particulasNieve[i];
    }

    return nullptr;
}


static void EmitirNievePelotas(
    MinijuegoPelotas& m,
    Vector3 posicion,
    int cantidad,
    float fuerza,
    float tamano,
    Vector3 sesgo
)
{
    int total = CantidadEscaladaPelotas(cantidad);

    for (int i = 0; i < total; i++)
    {
        ParticulaPelotas* p = ReservarParticulaPelotas(m);
        if (p == nullptr) return;

        float angulo = AzarPelotas(0.0f, 6.2832f);
        float velocidad = AzarPelotas(0.25f, 1.0f) * fuerza;

        *p = {};
        p->activa = true;
        p->tipo = PARTICULA_PELOTAS_NIEVE;
        p->posicion = posicion;
        p->velocidad =
        {
            std::cos(angulo) * velocidad + sesgo.x,
            AzarPelotas(0.3f, 1.0f) * fuerza * 0.55f + sesgo.y,
            std::sin(angulo) * velocidad + sesgo.z
        };
        p->vidaMaxima = AzarPelotas(0.5f, 1.0f);
        p->vida = p->vidaMaxima;
        p->tamano = tamano * AzarPelotas(0.7f, 1.35f);
        unsigned char tono = (unsigned char)GetRandomValue(226, 252);
        p->color = { (unsigned char)(tono - 10), (unsigned char)(tono - 3), tono, 220 };
    }
}


static void EmitirChispasHieloPelotas(
    MinijuegoPelotas& m,
    Vector3 posicion,
    Vector3 normal,
    int cantidad,
    float fuerza
)
{
    int total = CantidadEscaladaPelotas(cantidad);

    for (int i = 0; i < total; i++)
    {
        ParticulaPelotas* p = ReservarParticulaPelotas(m);
        if (p == nullptr) return;

        // Salen en abanico, de costado al choque.
        float lado = GetRandomValue(0, 1) == 0 ? -1.0f : 1.0f;
        float abertura = AzarPelotas(-0.6f, 0.6f);
        float px = -normal.z * lado + normal.x * abertura;
        float pz = normal.x * lado + normal.z * abertura;
        float velocidad = AzarPelotas(0.5f, 1.0f) * fuerza;

        *p = {};
        p->activa = true;
        p->tipo = PARTICULA_PELOTAS_CHISPA;
        p->posicion = posicion;
        p->velocidad = { px * velocidad, AzarPelotas(1.5f, 4.2f), pz * velocidad };
        p->vidaMaxima = AzarPelotas(0.35f, 0.7f);
        p->vida = p->vidaMaxima;
        p->tamano = AzarPelotas(0.05f, 0.11f);
        p->color = GetRandomValue(0, 2) == 0
            ? Color{ 255, 255, 255, 255 }
            : Color{ 170, 226, 255, 255 };
    }
}


static void EmitirTrozosCornisaPelotas(
    MinijuegoPelotas& m,
    Vector3 posicion,
    Vector3 haciaFuera,
    int cantidad
)
{
    int total = CantidadEscaladaPelotas(cantidad);

    for (int i = 0; i < total; i++)
    {
        ParticulaPelotas* p = ReservarParticulaPelotas(m);
        if (p == nullptr) return;

        float velocidad = AzarPelotas(0.8f, 2.6f);

        *p = {};
        p->activa = true;
        p->tipo = PARTICULA_PELOTAS_TROZO;
        p->posicion =
        {
            posicion.x + AzarPelotas(-0.35f, 0.35f),
            posicion.y - AzarPelotas(0.0f, 0.25f),
            posicion.z + AzarPelotas(-0.35f, 0.35f)
        };
        p->velocidad =
        {
            haciaFuera.x * velocidad + AzarPelotas(-0.6f, 0.6f),
            AzarPelotas(0.4f, 2.2f),
            haciaFuera.z * velocidad + AzarPelotas(-0.6f, 0.6f)
        };
        p->vidaMaxima = AzarPelotas(0.9f, 1.4f);
        p->vida = p->vidaMaxima;
        p->tamano = AzarPelotas(0.12f, 0.26f);
        p->color = GetRandomValue(0, 1) == 0
            ? Color{ 214, 236, 250, 255 }
            : Color{ 160, 202, 232, 255 };
    }
}


static void EmitirHumoPelotas(MinijuegoPelotas& m, Vector3 posicion)
{
    ParticulaPelotas* p = ReservarParticulaPelotas(m);
    if (p == nullptr) return;

    *p = {};
    p->activa = true;
    p->tipo = PARTICULA_PELOTAS_HUMO;
    p->posicion = posicion;
    p->velocidad = { AzarPelotas(0.1f, 0.3f), AzarPelotas(0.6f, 0.9f), AzarPelotas(-0.1f, 0.1f) };
    p->vidaMaxima = AzarPelotas(2.2f, 3.0f);
    p->vida = p->vidaMaxima;
    p->tamano = AzarPelotas(0.22f, 0.32f);
    unsigned char tono = (unsigned char)GetRandomValue(186, 214);
    p->color = { tono, tono, (unsigned char)(tono + 10), 170 };
}


static void AgregarOndaPelotas(
    MinijuegoPelotas& m,
    Vector3 centro,
    float radioMaximo,
    float duracion,
    Color color
)
{
    int elegido = 0;
    float menorVida = 1000.0f;

    for (int i = 0; i < MAX_ONDAS_PELOTAS; i++)
    {
        if (!m.ondas[i].activa) { elegido = i; break; }
        if (m.ondas[i].vida < menorVida) { menorVida = m.ondas[i].vida; elegido = i; }
    }

    OndaPelotas& onda = m.ondas[elegido];
    onda.activa = true;
    onda.centro = centro;
    onda.vidaMaxima = duracion;
    onda.vida = duracion;
    onda.radioMaximo = radioMaximo;
    onda.color = color;
}


static void AgregarHuellaPelotas(MinijuegoPelotas& m, Vector3 posicion, float angulo, float ancho)
{
    HuellaPelotas& huella = m.huellas[m.siguienteHuella];
    m.siguienteHuella = (m.siguienteHuella + 1) % MAX_HUELLAS_PELOTAS;
    huella.activa = true;
    huella.posicion = posicion;
    huella.angulo = angulo;
    huella.vida = VIDA_HUELLA_PELOTAS;
    huella.ancho = ancho;
}


static void AgregarAvisoPelotas(MinijuegoPelotas& m, Vector3 posicion, int jugador)
{
    int elegido = 0;
    float menorVida = 1000.0f;

    for (int i = 0; i < MAX_AVISOS_PELOTAS; i++)
    {
        if (!m.avisos[i].activo) { elegido = i; break; }
        if (m.avisos[i].vida < menorVida) { menorVida = m.avisos[i].vida; elegido = i; }
    }

    AvisoPelotas& aviso = m.avisos[elegido];
    aviso.activo = true;
    aviso.posicion = posicion;
    aviso.vida = DURACION_AVISO_PELOTAS;
    aviso.jugador = jugador;
}


static void ReiniciarCopoPelotas(CopoNievePelotas& copo, bool arriba)
{
    copo.posicion =
    {
        AzarPelotas(-19.0f, 19.0f),
        arriba ? AzarPelotas(9.0f, 12.0f) : AzarPelotas(-6.0f, 12.0f),
        AzarPelotas(-16.0f, 11.0f)
    };
    copo.fase = AzarPelotas(0.0f, 6.2832f);
    copo.caida = AzarPelotas(0.9f, 1.8f);
    copo.tamano = AzarPelotas(0.045f, 0.09f);
}


static void ReiniciarEfectosPelotas(MinijuegoPelotas& m)
{
    for (int i = 0; i < MAX_PARTICULAS_PELOTAS; i++) m.particulasNieve[i] = {};
    for (int i = 0; i < MAX_ONDAS_PELOTAS; i++) m.ondas[i] = {};
    for (int i = 0; i < MAX_HUELLAS_PELOTAS; i++) m.huellas[i] = {};
    for (int i = 0; i < MAX_AVISOS_PELOTAS; i++) m.avisos[i] = {};
    for (int i = 0; i < MAX_COPOS_PELOTAS; i++) ReiniciarCopoPelotas(m.copos[i], false);

    m.siguienteHuella = 0;
    m.esperaSonidoChoque = 0.0f;
    m.esperaHumo = 0.0f;
    m.esperaFestejo = 0.0f;
}


static void ActualizarEfectosPelotas(MinijuegoPelotas& m, float deltaTime)
{
    m.tiempoAnimacion += deltaTime;
    if (m.esperaSonidoChoque > 0.0f) m.esperaSonidoChoque -= deltaTime;

    for (int i = 0; i < MAX_PARTICULAS_PELOTAS; i++)
    {
        ParticulaPelotas& p = m.particulasNieve[i];
        if (!p.activa) continue;

        p.vida -= deltaTime;
        if (p.vida <= 0.0f)
        {
            p.activa = false;
            continue;
        }

        switch (p.tipo)
        {
            case PARTICULA_PELOTAS_NIEVE:
            {
                float freno = 1.0f - 2.6f * deltaTime;
                if (freno < 0.0f) freno = 0.0f;
                p.velocidad.x *= freno;
                p.velocidad.z *= freno;
                p.velocidad.y = p.velocidad.y * freno - 0.8f * deltaTime;
                break;
            }

            case PARTICULA_PELOTAS_CHISPA:
                p.velocidad.y -= 12.0f * deltaTime;
                break;

            case PARTICULA_PELOTAS_TROZO:
                p.velocidad.y -= 14.0f * deltaTime;
                break;

            case PARTICULA_PELOTAS_HUMO:
                p.velocidad.x += 0.25f * deltaTime;
                break;
        }

        p.posicion.x += p.velocidad.x * deltaTime;
        p.posicion.y += p.velocidad.y * deltaTime;
        p.posicion.z += p.velocidad.z * deltaTime;

        if (p.tipo == PARTICULA_PELOTAS_CHISPA && p.posicion.y < 0.04f && p.posicion.y > -0.4f)
        {
            p.posicion.y = 0.04f;
            p.velocidad.y *= -0.3f;
            p.velocidad.x *= 0.6f;
            p.velocidad.z *= 0.6f;
        }
    }

    for (int i = 0; i < MAX_ONDAS_PELOTAS; i++)
    {
        if (!m.ondas[i].activa) continue;
        m.ondas[i].vida -= deltaTime;
        if (m.ondas[i].vida <= 0.0f) m.ondas[i].activa = false;
    }

    for (int i = 0; i < MAX_HUELLAS_PELOTAS; i++)
    {
        if (!m.huellas[i].activa) continue;
        m.huellas[i].vida -= deltaTime;
        if (m.huellas[i].vida <= 0.0f) m.huellas[i].activa = false;
    }

    for (int i = 0; i < MAX_AVISOS_PELOTAS; i++)
    {
        if (!m.avisos[i].activo) continue;
        m.avisos[i].vida -= deltaTime;
        if (m.avisos[i].vida <= 0.0f) m.avisos[i].activo = false;
    }

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        float& destello = m.estadosJugadores[i].destelloChoque;
        destello -= deltaTime * 4.0f;
        if (destello < 0.0f) destello = 0.0f;
    }

    // Nevada de ambiente: cae en diagonal con el viento y se recicla.
    float t = m.tiempoAnimacion;
    for (int i = 0; i < MAX_COPOS_PELOTAS; i++)
    {
        CopoNievePelotas& copo = m.copos[i];
        copo.posicion.y -= copo.caida * deltaTime;
        copo.posicion.x += (0.55f + 0.45f * std::sin(t * 0.7f + copo.fase)) * deltaTime;
        copo.posicion.z += 0.25f * std::cos(t * 0.9f + copo.fase) * deltaTime;

        if (copo.posicion.y < -7.0f || copo.posicion.x > 20.0f)
        {
            ReiniciarCopoPelotas(copo, true);
            if (copo.posicion.x > 14.0f) copo.posicion.x -= 34.0f;
        }
    }

    // Humo de la chimenea del refugio.
    if (CalidadDecoracion() != CALIDAD_BAJA)
    {
        m.esperaHumo -= deltaTime;
        if (m.esperaHumo <= 0.0f)
        {
            m.esperaHumo = 0.22f / (0.4f + 0.6f * FactorCalidadGrafica(CalidadParticulas()));
            EmitirHumoPelotas(m, { -17.55f, -7.25f, -9.75f });
        }
    }
}


// Choques: ResolverColisionesPelotas (compartido) arma el cooldown y el
// empuje; aca solo se leen para el feedback (no cambia la fisica).
static void DetectarChoquesPelotas(
    MinijuegoPelotas& m,
    const JugadorPrueba jugadores[],
    const Participante participantes[],
    int limite,
    const float cooldownAntes[],
    const Vector3 empujeAntes[]
)
{
    bool nuevo[MAX_PARTICIPANTES]{};
    float impulso[MAX_PARTICIPANTES]{};

    for (int i = 0; i < limite; i++)
    {
        nuevo[i] =
            participantes[i].activo &&
            !jugadores[i].cayendo &&
            cooldownAntes[i] <= 0.0f &&
            jugadores[i].cooldownChoque > 0.1f;

        impulso[i] = MagnitudHorizontalPelotas(
            jugadores[i].empuje.x - empujeAntes[i].x,
            jugadores[i].empuje.z - empujeAntes[i].z
        );
    }

    for (int i = 0; i < limite; i++)
    {
        for (int j = i + 1; j < limite; j++)
        {
            if (!nuevo[i] || !nuevo[j]) continue;

            const JugadorPrueba& a = jugadores[i];
            const JugadorPrueba& b = jugadores[j];
            float dx = b.posicion.x - a.posicion.x;
            float dz = b.posicion.z - a.posicion.z;
            float distancia = MagnitudHorizontalPelotas(dx, dz);
            float contacto = (a.tamano.x + b.tamano.x) * 0.5f;
            if (distancia > contacto * 1.25f) continue;

            float nx = distancia > 0.001f ? dx / distancia : 1.0f;
            float nz = distancia > 0.001f ? dz / distancia : 0.0f;
            float fuerza = (impulso[i] + impulso[j]) * 0.5f;
            Vector3 punto = { a.posicion.x + dx * 0.5f, 0.6f, a.posicion.z + dz * 0.5f };

            int chispas = 6 + (int)(fuerza * 1.6f);
            if (chispas > 26) chispas = 26;
            EmitirChispasHieloPelotas(m, punto, { nx, 0.0f, nz }, chispas, 2.2f + fuerza * 0.35f);
            EmitirNievePelotas(m, { punto.x, 0.15f, punto.z }, 5 + (int)fuerza, 1.6f + fuerza * 0.2f, 0.14f, { 0.0f, 0.0f, 0.0f });

            float radioOnda = 0.8f + fuerza * 0.12f;
            if (radioOnda > 2.2f) radioOnda = 2.2f;
            AgregarOndaPelotas(m, { punto.x, 0.06f, punto.z }, radioOnda, 0.45f, Color{ 214, 240, 255, 255 });

            float destello = 0.5f + fuerza * 0.08f;
            if (destello > 1.0f) destello = 1.0f;
            m.estadosJugadores[i].destelloChoque = destello;
            m.estadosJugadores[j].destelloChoque = destello;

            // Temblor proporcional (respeta «reducir movimiento»).
            if (fuerza > 1.0f)
            {
                float intensidad = 0.035f + fuerza * 0.016f;
                float extra = fuerza * 0.01f;
                ActivarTemblorCamaraGeneral(
                    intensidad > 0.24f ? 0.24f : intensidad,
                    0.16f + (extra > 0.12f ? 0.12f : extra)
                );
            }

            if (m.esperaSonidoChoque <= 0.0f)
            {
                ReproducirSonidoMinijuego(m.audio, fuerza > 4.0f ? SONIDO_IMPACTO : SONIDO_GOLPE);
                m.esperaSonidoChoque = 0.09f;
            }
        }
    }
}


// Huellas y polvo al rodar; polvo, trozos de cornisa y anillo al caer.
static void ActualizarEstelasYCaidasPelotas(
    MinijuegoPelotas& m,
    const JugadorPrueba jugadores[],
    const Participante participantes[],
    int limite,
    float deltaTime
)
{
    CimaPelotas cima = CalcularCimaPelotas(m.camara);

    for (int i = 0; i < limite; i++)
    {
        EstadoJugadorPelotas& e = m.estadosJugadores[i];
        const JugadorPrueba& j = jugadores[i];

        if (
            !participantes[i].activo ||
            !participantes[i].conectado ||
            !m.resultado.participantes[i].participo ||
            e.eliminado ||
            j.cayendo
        )
        {
            continue;
        }

        bool soportado = JugadorSobreArenaCircularPelotas(j, m.camara);

        if (!e.fueraDeCima && !soportado && j.posicion.y < j.tamano.y * 0.5f - 0.15f)
        {
            e.fueraDeCima = true;
            e.puntoCaida = PuntoBordeCercanoPelotas(cima, j.posicion);

            float fx = e.puntoCaida.x - cima.centroX;
            float fz = e.puntoCaida.z - cima.centroZ;
            float largo = MagnitudHorizontalPelotas(fx, fz);
            if (largo > 0.001f) { fx /= largo; fz /= largo; }

            EmitirNievePelotas(m, e.puntoCaida, 18, 2.2f, 0.2f, { fx * 1.2f, 0.4f, fz * 1.2f });
            EmitirTrozosCornisaPelotas(m, { e.puntoCaida.x, -0.2f, e.puntoCaida.z }, { fx, 0.0f, fz }, 8);
            AgregarOndaPelotas(m, { e.puntoCaida.x, 0.06f, e.puntoCaida.z }, 1.4f, 0.6f, participantes[i].color);
            ActivarTemblorCamaraGeneral(0.06f, 0.18f);
            continue;
        }

        if (e.fueraDeCima && soportado && j.enSuelo) e.fueraDeCima = false;

        if (!j.enSuelo || e.fueraDeCima)
        {
            e.tiempoEstela = 0.0f;
            continue;
        }

        float velocidad = MagnitudHorizontalPelotas(j.velocidad.x, j.velocidad.z);
        if (velocidad < 1.2f) continue;

        e.tiempoEstela += deltaTime;
        float intervalo = 0.42f / velocidad;
        if (intervalo < 0.045f) intervalo = 0.045f;
        if (intervalo > 0.16f) intervalo = 0.16f;

        while (e.tiempoEstela >= intervalo)
        {
            e.tiempoEstela -= intervalo;
            float angulo = std::atan2(j.velocidad.z, j.velocidad.x);
            AgregarHuellaPelotas(m, { j.posicion.x, 0.03f, j.posicion.z }, angulo, 0.42f);

            if (velocidad > 3.0f)
            {
                float ux = j.velocidad.x / velocidad;
                float uz = j.velocidad.z / velocidad;
                EmitirNievePelotas(
                    m,
                    { j.posicion.x - ux * 0.45f, 0.12f, j.posicion.z - uz * 0.45f },
                    2,
                    0.7f,
                    0.11f,
                    { -j.velocidad.x * 0.08f, 0.5f, -j.velocidad.z * 0.08f }
                );
            }
        }
    }
}


static void RegistrarEliminacionPelotas(
    MinijuegoPelotas& m,
    const JugadorPrueba& jugador,
    int indice
)
{
    EstadoJugadorPelotas& e = m.estadosJugadores[indice];

    if (!e.fueraDeCima)
    {
        e.puntoCaida = PuntoBordeCercanoPelotas(CalcularCimaPelotas(m.camara), jugador.posicion);
    }

    Vector3 borde = e.puntoCaida;
    AgregarAvisoPelotas(m, borde, indice);
    AgregarOndaPelotas(m, { borde.x, 0.07f, borde.z }, 2.0f, 0.85f, Color{ 236, 84, 64, 255 });

    // Penacho de nieve que sube desde la bruma donde se perdio.
    EmitirNievePelotas(m, { jugador.posicion.x, -7.6f, jugador.posicion.z }, 22, 2.4f, 0.32f, { 0.0f, 3.2f, 0.0f });
    ActivarTemblorCamaraGeneral(0.10f, 0.26f);
}


static void ActualizarFestejoPelotas(
    MinijuegoPelotas& m,
    const JugadorPrueba jugadores[],
    const Participante participantes[],
    float deltaTime
)
{
    m.esperaFestejo -= deltaTime;
    if (m.esperaFestejo > 0.0f) return;
    m.esperaFestejo = 0.16f;

    bool conOnda = std::fmod(m.tiempoAnimacion, 0.8f) < 0.17f;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (
            !m.resultado.participantes[i].participo ||
            m.estadosJugadores[i].eliminado ||
            m.resultado.participantes[i].posicionFinal != 1 ||
            jugadores[i].cayendo
        )
        {
            continue;
        }

        const JugadorPrueba& j = jugadores[i];
        EmitirChispasHieloPelotas(m, { j.posicion.x, j.posicion.y + 1.2f, j.posicion.z }, { 1.0f, 0.0f, 0.0f }, 3, 2.0f);

        if (conOnda)
        {
            AgregarOndaPelotas(m, { j.posicion.x, 0.06f, j.posicion.z }, 1.3f, 0.7f, participantes[i].color);
        }
    }
}


//==================================================
// VISUAL: CIELO Y FONDO LEJANO
//==================================================

static void DibujarCieloPelotas(float t)
{
    int ancho = GetScreenWidth();
    int alto = GetScreenHeight();

    DrawRectangleGradientV(0, 0, ancho, (int)(alto * 0.42f), COLOR_CIELO_ALTO_PELOTAS, COLOR_NIEBLA_PELOTAS);

    // Sol de invierno bajo, con halo palido que late muy lento.
    float pulso = 1.0f + 0.03f * std::sin(t * 0.8f);
    int solX = (int)(ancho * 0.82f);
    int solY = (int)(alto * 0.07f);
    Color sol = { 255, 246, 222, 255 };
    DrawCircleGradient({ (float)solX, (float)solY }, alto * 0.30f * pulso, Fade(sol, 0.45f), Fade(sol, 0.0f));
    DrawCircleGradient({ (float)solX, (float)solY }, alto * 0.06f, Fade(Color{ 255, 252, 240, 255 }, 0.95f), Fade(sol, 0.0f));
}


// Pico de tres caras (lado iluminado, lado en sombra y casquete de nieve).
static void DibujarPicoPelotas(
    Vector3 base,
    float cima,
    float ancho,
    float profundidad,
    float desplazamiento,
    float nieve,
    int semilla
)
{
    Vector3 p = { base.x + desplazamiento, cima, base.z };
    Vector3 l = { base.x - ancho, base.y, base.z };
    Vector3 r = { base.x + ancho, base.y, base.z };
    Vector3 f = { base.x + desplazamiento * 0.4f, base.y, base.z + profundidad };

    float tono = 0.9f + 0.2f * Hash01Pelotas(semilla, 11);
    Color rocaLuz = NieblaPelotas(TintePelotas(Color{ 92, 108, 134, 255 }, tono), p);
    Color rocaSombra = NieblaPelotas(TintePelotas(Color{ 60, 72, 96, 255 }, tono), p);
    Color nieveLuz = NieblaPelotas(Color{ 238, 245, 252, 255 }, p);
    Color nieveSombra = NieblaPelotas(Color{ 170, 194, 222, 255 }, p);

    // Linea de nieve irregular: baja mas por la arista frontal.
    float sL = nieve * (0.9f + 0.2f * Hash01Pelotas(semilla, 3));
    float sF = nieve * (1.15f + 0.25f * Hash01Pelotas(semilla, 5));
    float sR = nieve * (0.85f + 0.2f * Hash01Pelotas(semilla, 7));
    if (sF > 0.92f) sF = 0.92f;

    Vector3 nl = { p.x + (l.x - p.x) * sL, p.y + (l.y - p.y) * sL, p.z + (l.z - p.z) * sL };
    Vector3 nf = { p.x + (f.x - p.x) * sF, p.y + (f.y - p.y) * sF, p.z + (f.z - p.z) * sF };
    Vector3 nr = { p.x + (r.x - p.x) * sR, p.y + (r.y - p.y) * sR, p.z + (r.z - p.z) * sR };

    // Punta del chorreado de nieve de cada cara (siempre sobre su plano,
    // entre la linea de nieve y la base).
    Vector3 medioL = { (nl.x + nf.x) * 0.5f, (nl.y + nf.y) * 0.5f, (nl.z + nf.z) * 0.5f };
    Vector3 puntaL = { medioL.x + (medioL.x - p.x) * 0.35f, medioL.y + (medioL.y - p.y) * 0.35f, medioL.z + (medioL.z - p.z) * 0.35f };
    Vector3 medioR = { (nf.x + nr.x) * 0.5f, (nf.y + nr.y) * 0.5f, (nf.z + nr.z) * 0.5f };
    Vector3 puntaR = { medioR.x + (medioR.x - p.x) * 0.28f, medioR.y + (medioR.y - p.y) * 0.28f, medioR.z + (medioR.z - p.z) * 0.28f };

    // Cada cara se reparte en regiones que NO se superponen: la roca cubre
    // solo lo que queda bajo la nieve (pentagono base-linea de nieve con la
    // muesca del chorreado) y la nieve lo de arriba. Antes la nieve se
    // dibujaba encima de la cara de roca completa en el mismo plano y, con
    // otra triangulacion, la profundidad interpolada peleaba (rayado a la
    // distancia). Mismo sentido de giro que la cara original.
    DrawTriangle3D(l, f, puntaL, rocaLuz);
    DrawTriangle3D(f, nf, puntaL, rocaLuz);
    DrawTriangle3D(puntaL, nl, l, rocaLuz);
    DrawTriangle3D(nl, nf, p, nieveLuz);
    DrawTriangle3D(nl, puntaL, nf, nieveLuz);

    DrawTriangle3D(f, r, puntaR, rocaSombra);
    DrawTriangle3D(r, nr, puntaR, rocaSombra);
    DrawTriangle3D(puntaR, nf, f, rocaSombra);
    DrawTriangle3D(nf, nr, p, nieveSombra);
    DrawTriangle3D(nf, puntaR, nr, nieveSombra);
}


static void DibujarCordilleraPelotas(
    float z,
    float x0,
    float x1,
    int picos,
    float alturaMinima,
    float alturaMaxima,
    int semilla
)
{
    float paso = (x1 - x0) / (float)picos;

    for (int k = 0; k < picos; k++)
    {
        float h1 = Hash01Pelotas(semilla, k);
        float h2 = Hash01Pelotas(semilla + 17, k);
        float h3 = Hash01Pelotas(semilla + 31, k);

        float centroX = x0 + ((float)k + 0.5f) * paso + (h1 - 0.5f) * paso * 0.6f;
        float cima = alturaMinima + (alturaMaxima - alturaMinima) * h2;
        float ancho = paso * (0.85f + 0.6f * h3);

        DibujarPicoPelotas(
            { centroX, -32.0f, z + (h3 - 0.5f) * 6.0f },
            cima,
            ancho,
            ancho * 0.7f,
            (h1 - 0.5f) * ancho * 0.5f,
            0.34f + 0.16f * h3,
            semilla * 100 + k
        );
    }
}


// Mar de nubes del valle: tapa la base de todo lo lejano.
static void DibujarMarNubesPelotas(float t)
{
    const float Y = -17.0f;
    Color cerca = { 226, 236, 245, 255 };
    Color lejos = COLOR_NIEBLA_PELOTAS;

    CuadPelotas(
        { -320.0f, Y, 40.0f }, { 320.0f, Y, 40.0f },
        { 320.0f, Y, -280.0f }, { -320.0f, Y, -280.0f },
        cerca, cerca, lejos, lejos
    );

    if (CalidadDecoracion() == CALIDAD_BAJA) return;

    // Bancos de nube que derivan despacio con el viento.
    int paso = CalidadDecoracion() == CALIDAD_ALTA ? 1 : 2;
    for (int i = 0; i < 22; i += paso)
    {
        float h1 = Hash01Pelotas(i, 41);
        float h2 = Hash01Pelotas(i, 43);
        float h3 = Hash01Pelotas(i, 47);

        float x = -70.0f + std::fmod(h1 * 140.0f + t * (0.5f + h3 * 0.6f), 140.0f);
        float z = -8.0f - h2 * 70.0f;
        Vector3 centro = { x, Y + 0.2f, z };
        Color color = NieblaPelotas(Color{ 246, 250, 254, 255 }, centro);

        rlPushMatrix();
        rlTranslatef(centro.x, centro.y, centro.z);
        rlScalef(5.0f + h3 * 6.0f, 1.0f + h1 * 0.8f, 3.0f + h2 * 3.0f);
        DrawSphereEx({ 0.0f, 0.0f, 0.0f }, 1.0f, 6, 10, color);
        rlPopMatrix();
    }
}


//==================================================
// VISUAL: LA CIMA (ARENA, CORNISA Y PAREDES)
//==================================================

static Color ColorNieveCimaPelotas(int anillo, int segmento, float factor)
{
    int s = segmento % SEGMENTOS_ARENA_PELOTAS;
    float h = anillo == 0 ? 0.4f : Hash01Pelotas(anillo * 31 + 7, s);
    Color c = MezclarPelotas(COLOR_NIEVE_PELOTAS, COLOR_NIEVE_SOMBRA_PELOTAS, 0.12f + 0.5f * h);

    float angulo = AnguloSegmentoPelotas(s);
    c = TintePelotas(c, 1.0f + 0.03f * std::cos(angulo - ANGULO_LUZ_PELOTAS) * factor);

    // Hacia el borde la nieve se compacta y azulea, anunciando el hielo.
    if (factor > 0.66f)
    {
        c = MezclarPelotas(c, Color{ 198, 218, 238, 255 }, (factor - 0.66f) * 1.6f);
    }

    return c;
}


static void DibujarSuperficieCimaPelotas(const CimaPelotas& cima)
{
    const int ANILLOS = 7;
    const float factores[ANILLOS] = { 0.0f, 0.2f, 0.4f, 0.58f, 0.73f, 0.84f, 0.905f };

    for (int i = 0; i < SEGMENTOS_ARENA_PELOTAS; i++)
    {
        float a0 = AnguloSegmentoPelotas(i);
        float a1 = AnguloSegmentoPelotas(i + 1);

        for (int r = 0; r < ANILLOS - 1; r++)
        {
            float f0 = factores[r];
            float f1 = factores[r + 1];
            Vector3 p00 = PuntoCimaPelotas(cima, a0, f0, 0.0f);
            Vector3 p01 = PuntoCimaPelotas(cima, a1, f0, 0.0f);
            Vector3 p10 = PuntoCimaPelotas(cima, a0, f1, 0.0f);
            Vector3 p11 = PuntoCimaPelotas(cima, a1, f1, 0.0f);

            CuadPelotas(
                p00, p01, p11, p10,
                ColorNieveCimaPelotas(r, i, f0),
                ColorNieveCimaPelotas(r, i + 1, f0),
                ColorNieveCimaPelotas(r + 1, i + 1, f1),
                ColorNieveCimaPelotas(r + 1, i, f1)
            );
        }

        // Franja de hielo del borde: la zona donde ya conviene frenar.
        Vector3 h00 = PuntoCimaPelotas(cima, a0, 0.905f, 0.0f);
        Vector3 h01 = PuntoCimaPelotas(cima, a1, 0.905f, 0.0f);
        Vector3 h10 = PuntoCimaPelotas(cima, a0, 1.0f, 0.0f);
        Vector3 h11 = PuntoCimaPelotas(cima, a1, 1.0f, 0.0f);
        float v0 = 0.94f + 0.12f * Hash01Pelotas(i % SEGMENTOS_ARENA_PELOTAS, 91);
        float v1 = 0.94f + 0.12f * Hash01Pelotas((i + 1) % SEGMENTOS_ARENA_PELOTAS, 91);
        Color interior0 = TintePelotas(COLOR_HIELO_PELOTAS, v0 * 1.06f);
        Color interior1 = TintePelotas(COLOR_HIELO_PELOTAS, v1 * 1.06f);
        Color exterior0 = TintePelotas(COLOR_HIELO_OSCURO_PELOTAS, v0);
        Color exterior1 = TintePelotas(COLOR_HIELO_OSCURO_PELOTAS, v1);
        CuadPelotas(h00, h01, h11, h10, interior0, interior1, exterior1, exterior0);

        // Linea luminosa justo en el borde de caida.
        DrawLine3D(
            PuntoCimaPelotas(cima, a0, 1.0f, 0.03f),
            PuntoCimaPelotas(cima, a1, 1.0f, 0.03f),
            Color{ 240, 251, 255, 255 }
        );
        DrawLine3D(
            PuntoCimaPelotas(cima, a0, 0.993f, 0.03f),
            PuntoCimaPelotas(cima, a1, 0.993f, 0.03f),
            Color{ 214, 240, 255, 255 }
        );
        DrawLine3D(
            PuntoCimaPelotas(cima, a0, 0.905f, 0.02f),
            PuntoCimaPelotas(cima, a1, 0.905f, 0.02f),
            Color{ 150, 190, 222, 200 }
        );
    }
}


// Ondulaciones del viento en la nieve (sastrugi): detalle fino y plano.
static void DibujarOndulacionesNievePelotas(const CimaPelotas& cima)
{
    if (CalidadDecoracion() == CALIDAD_BAJA) return;

    Color color = { 196, 214, 233, 255 };
    for (int k = 0; k < 14; k++)
    {
        float angulo = Hash01Pelotas(k, 5) * 6.2832f;
        float factor = 0.18f + 0.55f * Hash01Pelotas(k, 9);
        Vector3 inicio = PuntoCimaPelotas(cima, angulo, factor, 0.018f);
        float largo = 0.9f + 0.9f * Hash01Pelotas(k, 13);

        Vector3 anterior = inicio;
        for (int s = 1; s <= 5; s++)
        {
            float u = (float)s / 5.0f;
            Vector3 punto =
            {
                inicio.x + largo * u,
                0.018f,
                inicio.z + largo * 0.3f * u + std::sin(u * 3.1416f) * 0.12f
            };
            DrawLine3D(anterior, punto, color);
            anterior = punto;
        }
    }
}


// Medallon de hielo en el centro: desempata por tiempo (mas cerca gana).
static void DibujarCentroCimaPelotas()
{
    const Vector3 centro = { 0.0f, 0.015f, 0.0f };
    const int LADOS = 24;
    Color interior = { 196, 222, 242, 255 };
    Color exterior = { 168, 202, 230, 255 };

    rlBegin(RL_TRIANGLES);
    for (int k = 0; k < LADOS; k++)
    {
        float a0 = 6.2832f * (float)k / (float)LADOS;
        float a1 = 6.2832f * (float)(k + 1) / (float)LADOS;
        VerticePelotas(centro, interior);
        VerticePelotas({ std::cos(a1) * 1.05f, centro.y, std::sin(a1) * 1.05f }, exterior);
        VerticePelotas({ std::cos(a0) * 1.05f, centro.y, std::sin(a0) * 1.05f }, exterior);
    }
    rlEnd();

    Color linea = { 112, 158, 200, 255 };
    DrawCircle3D({ 0.0f, 0.03f, 0.0f }, 1.05f, { 1.0f, 0.0f, 0.0f }, 90.0f, linea);
    DrawCircle3D({ 0.0f, 0.03f, 0.0f }, 1.10f, { 1.0f, 0.0f, 0.0f }, 90.0f, linea);
    DrawCircle3D({ 0.0f, 0.03f, 0.0f }, 0.45f, { 1.0f, 0.0f, 0.0f }, 90.0f, Fade(linea, 0.8f));

    // Rosa de los vientos grabada en el hielo.
    for (int k = 0; k < 4; k++)
    {
        float a = 1.5708f * (float)k;
        Vector3 punta = { std::cos(a) * 0.9f, 0.03f, std::sin(a) * 0.9f };
        Vector3 izquierda = { std::cos(a + 0.5f) * 0.22f, 0.03f, std::sin(a + 0.5f) * 0.22f };
        Vector3 derecha = { std::cos(a - 0.5f) * 0.22f, 0.03f, std::sin(a - 0.5f) * 0.22f };
        DrawLine3D(izquierda, punta, linea);
        DrawLine3D(derecha, punta, linea);
    }
}


struct PerfilParedPelotas
{
    float factor;
    float altura;
    Color color;
    bool roca;
};


// Cornisa de hielo con volumen y pared de roca que baja hasta las nubes.
// Todo queda por dentro de la vertical del borde: quien cae, cae al vacio.
static void DibujarParedesCimaPelotas(const CimaPelotas& cima)
{
    const int PUNTOS = 11;
    const PerfilParedPelotas perfil[PUNTOS] =
    {
        { 1.000f,   0.00f, { 238, 249, 255, 255 }, false },
        { 1.030f,  -0.10f, { 206, 234, 252, 255 }, false },
        { 1.042f,  -0.32f, { 150, 198, 232, 255 }, false },
        { 1.004f,  -0.56f, { 104, 142, 180, 255 }, false },
        { 0.976f,  -0.66f, {  86, 104, 128, 255 }, true  },
        { 0.935f,  -2.30f, {  74,  84, 102, 255 }, true  },
        { 0.952f,  -2.46f, { 222, 234, 246, 255 }, false },
        { 0.925f,  -5.00f, {  66,  74,  92, 255 }, true  },
        { 0.985f,  -8.60f, {  80,  90, 110, 255 }, true  },
        { 1.360f, -12.60f, { 208, 222, 236, 255 }, false },
        { 2.350f, -21.50f, { 214, 226, 238, 255 }, false }
    };

    for (int i = 0; i < SEGMENTOS_ARENA_PELOTAS; i++)
    {
        float a0 = AnguloSegmentoPelotas(i);
        float a1 = AnguloSegmentoPelotas(i + 1);
        float luz0 = LuzCaraPelotas(a0);
        float luz1 = LuzCaraPelotas(a1);

        for (int k = 0; k < PUNTOS - 1; k++)
        {
            const PerfilParedPelotas& arriba = perfil[k];
            const PerfilParedPelotas& abajo = perfil[k + 1];

            Vector3 p00 = PuntoCimaPelotas(cima, a0, arriba.factor, arriba.altura);
            Vector3 p01 = PuntoCimaPelotas(cima, a1, arriba.factor, arriba.altura);
            Vector3 p10 = PuntoCimaPelotas(cima, a0, abajo.factor, abajo.altura);
            Vector3 p11 = PuntoCimaPelotas(cima, a1, abajo.factor, abajo.altura);

            // La roca alterna tonos por cara: se ve facetada y no lisa.
            float variacion = arriba.roca
                ? 0.82f + 0.34f * Hash01Pelotas(i, k + 3)
                : 1.0f;

            Color c00 = NieblaPelotas(TintePelotas(arriba.color, variacion * luz0), p00);
            Color c01 = NieblaPelotas(TintePelotas(arriba.color, variacion * luz1), p01);
            Color c10 = NieblaPelotas(TintePelotas(abajo.color, variacion * luz0), p10);
            Color c11 = NieblaPelotas(TintePelotas(abajo.color, variacion * luz1), p11);

            CuadPelotas(p00, p01, p11, p10, c00, c01, c11, c10);
        }
    }
}


// Rocas que asoman bajo la cornisa y carambanos colgando del labio.
static void DibujarDetallesParedPelotas(const CimaPelotas& cima)
{
    if (CalidadDecoracion() == CALIDAD_BAJA) return;

    int paso = CalidadDecoracion() == CALIDAD_ALTA ? 1 : 2;

    for (int i = 0; i < SEGMENTOS_ARENA_PELOTAS; i += paso)
    {
        float angulo = AnguloSegmentoPelotas(i) + 0.06f;
        float h1 = Hash01Pelotas(i, 61);
        float h2 = Hash01Pelotas(i, 67);

        if (h1 > 0.45f)
        {
            Vector3 raiz = PuntoCimaPelotas(cima, angulo, 1.012f, -0.50f);
            float largo = 0.22f + 0.5f * h2;
            DrawCylinderEx(raiz, { raiz.x, raiz.y - largo, raiz.z }, 0.07f, 0.0f, 5, Color{ 204, 234, 252, 240 });

            Vector3 raiz2 = PuntoCimaPelotas(cima, angulo + 0.045f, 1.008f, -0.52f);
            DrawCylinderEx(raiz2, { raiz2.x, raiz2.y - largo * 0.55f, raiz2.z }, 0.05f, 0.0f, 5, Color{ 186, 224, 248, 235 });
        }

        if (h2 > 0.55f)
        {
            Vector3 centro = PuntoCimaPelotas(cima, angulo + 0.03f, 0.955f, -1.15f - h1 * 0.9f);
            Vector3 tamano = { 0.7f + 0.6f * h1, 0.55f + 0.4f * h2, 0.6f + 0.5f * h2 };
            Color roca = NieblaPelotas(TintePelotas(COLOR_ROCA_PELOTAS, 0.85f + 0.3f * h1), centro);
            float giro = angulo * 57.3f + h1 * 40.0f;
            RocaGiradaPelotas(centro, tamano, giro, roca);
            // Nieve acumulada encima de la roca: gira con ella (sin girar
            // quedaba cruzada sobre la roca y sus esquinas la atravesaban).
            RocaGiradaPelotas(
                { centro.x, centro.y + tamano.y * 0.5f + 0.05f, centro.z },
                { tamano.x * 0.8f, 0.1f, tamano.z * 0.8f },
                giro,
                NieblaPelotas(COLOR_NIEVE_PELOTAS, centro)
            );
        }
    }
}


//==================================================
// VISUAL: LADERAS, PINOS Y UTILERIA
//==================================================
//
// Toda la utileria esta muy por debajo y lejos del borde (estribaciones a
// mas de 9 m y 10 m mas abajo): nadie la confunde con la arena y ningun
// jugador que cae la atraviesa antes de perderse en la bruma.

static void DibujarPinoPelotas(Vector3 base, float altura, int semilla)
{
    float h = Hash01Pelotas(semilla, 77);
    Color tronco = NieblaPelotas(Color{ 86, 62, 48, 255 }, base);
    Color hoja = NieblaPelotas(TintePelotas(Color{ 38, 78, 72, 255 }, 0.85f + 0.3f * h), base);
    Color nieve = NieblaPelotas(Color{ 236, 244, 251, 255 }, base);

    DrawCylinderEx(base, { base.x, base.y + altura * 0.25f, base.z }, altura * 0.06f, altura * 0.045f, 5, tronco);

    for (int k = 0; k < 3; k++)
    {
        float y0 = base.y + altura * (0.16f + (float)k * 0.24f);
        float radio = altura * (0.34f - (float)k * 0.08f);
        float alto = altura * 0.42f;

        DrawCylinderEx({ base.x, y0, base.z }, { base.x, y0 + alto, base.z }, radio, 0.0f, 7, hoja);
        DrawCylinderEx(
            { base.x, y0 + alto * 0.40f, base.z },
            { base.x, y0 + alto + 0.02f, base.z },
            radio * 0.64f,
            0.0f,
            7,
            nieve
        );
    }
}


// Estribacion: bloques de roca escalonados con nieve encima.
static void DibujarEstribacionPelotas(float lado)
{
    const Vector3 bloques[4] =
    {
        { 17.6f, -11.3f,  -9.2f },
        { 22.8f, -12.4f, -12.0f },
        { 27.6f, -13.6f, -15.5f },
        { 14.6f, -12.6f,  -4.8f }
    };
    const Vector3 tamanos[4] =
    {
        { 7.0f, 2.6f, 7.0f },
        { 7.5f, 3.4f, 8.5f },
        { 8.0f, 4.2f, 9.5f },
        { 5.0f, 2.8f, 5.0f }
    };

    for (int k = 0; k < 4; k++)
    {
        Vector3 c = { bloques[k].x * lado, bloques[k].y, bloques[k].z };
        Vector3 tam = tamanos[k];
        float giro = (Hash01Pelotas(k, lado > 0.0f ? 3 : 4) - 0.5f) * 24.0f;
        Color roca = NieblaPelotas(TintePelotas(COLOR_ROCA_PELOTAS, 0.9f + 0.15f * Hash01Pelotas(k, 8)), c);

        RocaGiradaPelotas(c, tam, giro, roca);
        RocaGiradaPelotas(
            { c.x, c.y + tam.y * 0.5f + 0.16f, c.z },
            { tam.x + 0.3f, 0.34f, tam.z + 0.3f },
            giro,
            // Nieve algo mas apagada que la de la cima: la arena es lo
            // mas claro de la pantalla.
            MezclarPelotas(Color{ 212, 224, 238, 255 }, COLOR_NIEBLA_PELOTAS, 0.25f)
        );
    }
}


static void DibujarPinosPelotas()
{
    if (CalidadDecoracion() == CALIDAD_BAJA) return;

    int paso = CalidadDecoracion() == CALIDAD_ALTA ? 1 : 2;

    // Pinos sobre las estribaciones.
    const Vector3 pinos[14] =
    {
        { -21.0f, -10.8f, -6.6f }, { -23.6f, -10.8f, -8.0f }, { -24.8f, -10.8f, -14.4f },
        { -26.4f, -11.5f, -11.2f }, { -29.4f, -11.5f, -13.4f }, { -14.4f, -11.2f, -2.6f },
        { -20.2f, -10.8f, -12.8f },
        {  21.2f, -10.8f,  -6.2f }, {  24.6f, -10.8f,  -9.0f }, {  23.0f, -10.8f, -15.0f },
        {  27.0f, -11.5f, -12.0f }, {  30.0f, -11.5f, -14.8f }, {  15.0f, -11.2f,  -2.4f },
        {  19.0f, -10.8f, -13.6f }
    };

    for (int k = 0; k < 14; k += paso)
    {
        float altura = 2.2f + 1.4f * Hash01Pelotas(k, 21);
        DibujarPinoPelotas(pinos[k], altura, k);
    }

    // Bosque en la loma del fondo, detras de la cima.
    for (int k = 0; k < 26; k += paso)
    {
        float h1 = Hash01Pelotas(k, 33);
        float h2 = Hash01Pelotas(k, 35);
        Vector3 base = { -34.0f + (float)k * 2.7f + h1 * 1.2f, -14.6f + h2 * 1.4f, -31.0f - h2 * 5.0f };
        DibujarPinoPelotas(base, 2.6f + 1.6f * h1, 100 + k);
    }
}


// Loma baja del fondo (sostiene el bosque lejano).
static void DibujarLomaFondoPelotas()
{
    for (int k = 0; k < 9; k++)
    {
        float h = Hash01Pelotas(k, 51);
        Vector3 c = { -36.0f + (float)k * 9.0f, -16.6f + h * 1.2f, -33.0f - h * 4.0f };
        RocaGiradaPelotas(c, { 11.0f, 3.2f, 9.0f }, (h - 0.5f) * 30.0f, NieblaPelotas(Color{ 220, 232, 243, 255 }, c));
    }
}


static void DibujarBanderinesPelotas(Vector3 a, Vector3 b, float t, int semilla)
{
    const Color colores[5] =
    {
        { 206, 86, 74, 255 },
        { 232, 190, 84, 255 },
        { 84, 148, 204, 255 },
        { 96, 172, 116, 255 },
        { 236, 236, 236, 255 }
    };
    const int TRAMOS = 10;

    Vector3 anterior = a;
    for (int k = 1; k <= TRAMOS; k++)
    {
        float u = (float)k / (float)TRAMOS;
        float comba = 4.0f * u * (1.0f - u) * 0.55f;
        Vector3 punto = { a.x + (b.x - a.x) * u, a.y + (b.y - a.y) * u - comba, a.z + (b.z - a.z) * u };
        DrawLine3D(anterior, punto, NieblaPelotas(Color{ 70, 60, 54, 255 }, punto));

        // Banderin triangular que flamea con el viento.
        if (k < TRAMOS)
        {
            float vaiven = std::sin(t * 4.2f + (float)k * 0.9f + (float)semilla) * 0.12f;
            Vector3 izquierda = { punto.x - 0.17f, punto.y, punto.z };
            Vector3 derecha = { punto.x + 0.17f, punto.y, punto.z };
            Vector3 punta = { punto.x + vaiven, punto.y - 0.42f, punto.z + 0.08f + vaiven * 0.5f };
            DrawTriangle3D(izquierda, derecha, punta, NieblaPelotas(colores[(k + semilla) % 5], punto));
        }

        anterior = punto;
    }
}


static void DibujarRefugioPelotas(float t)
{
    // Piso del refugio sobre la estribacion izquierda.
    const Vector3 piso = { -18.4f, -9.95f, -9.6f };
    Color madera = NieblaPelotas(Color{ 128, 84, 58, 255 }, piso);
    Color maderaOscura = NieblaPelotas(Color{ 84, 54, 40, 255 }, piso);
    Color nieve = NieblaPelotas(COLOR_NIEVE_PELOTAS, piso);
    Color nieveSombra = NieblaPelotas(Color{ 196, 212, 230, 255 }, piso);

    // Paredes de troncos.
    CajaFacetadaPelotas({ piso.x, piso.y + 0.75f, piso.z }, { 2.8f, 1.5f, 2.1f }, madera, 1.05f);
    for (int k = 0; k < 4; k++)
    {
        float y = piso.y + 0.22f + (float)k * 0.36f;
        DrawLine3D({ piso.x - 1.4f, y, piso.z + 1.06f }, { piso.x + 1.4f, y, piso.z + 1.06f }, maderaOscura);
        DrawLine3D({ piso.x + 1.41f, y, piso.z - 1.05f }, { piso.x + 1.41f, y, piso.z + 1.05f }, maderaOscura);
    }

    // Ventanas con luz calida y puerta.
    Color ventana = { 255, 204, 120, 255 };
    CajaFacetadaPelotas({ piso.x - 0.7f, piso.y + 0.85f, piso.z + 1.06f }, { 0.5f, 0.42f, 0.04f }, ventana, 1.0f);
    CajaFacetadaPelotas({ piso.x + 0.75f, piso.y + 0.85f, piso.z + 1.06f }, { 0.5f, 0.42f, 0.04f }, ventana, 1.0f);
    CajaFacetadaPelotas({ piso.x, piso.y + 0.55f, piso.z + 1.07f }, { 0.48f, 1.05f, 0.04f }, maderaOscura, 1.0f);

    // Techo a dos aguas cubierto de nieve.
    float alero = 1.42f;
    float cumbrera = piso.y + 2.25f;
    float baseTecho = piso.y + 1.42f;
    Vector3 ci = { piso.x - 1.75f, cumbrera, piso.z };
    Vector3 cd = { piso.x + 1.75f, cumbrera, piso.z };
    Vector3 fi = { piso.x - 1.75f, baseTecho, piso.z + alero };
    Vector3 fd = { piso.x + 1.75f, baseTecho, piso.z + alero };
    Vector3 ti = { piso.x - 1.75f, baseTecho, piso.z - alero };
    Vector3 td = { piso.x + 1.75f, baseTecho, piso.z - alero };
    CuadPelotas(ci, cd, fd, fi, nieve, nieve, nieveSombra, nieveSombra);
    CuadPelotas(ci, cd, td, ti, nieveSombra, nieveSombra, nieveSombra, nieveSombra);
    DrawTriangle3D({ piso.x + 1.4f, baseTecho, piso.z + 1.05f }, { piso.x + 1.4f, cumbrera - 0.1f, piso.z }, { piso.x + 1.4f, baseTecho, piso.z - 1.05f }, TintePelotas(madera, 0.8f));
    DrawTriangle3D({ piso.x - 1.4f, baseTecho, piso.z + 1.05f }, { piso.x - 1.4f, cumbrera - 0.1f, piso.z }, { piso.x - 1.4f, baseTecho, piso.z - 1.05f }, TintePelotas(madera, 0.66f));
    DrawLine3D(fi, fd, Color{ 170, 190, 214, 255 });

    // Chimenea de piedra.
    Vector3 chimenea = { piso.x + 0.85f, cumbrera + 0.05f, piso.z - 0.15f };
    CajaFacetadaPelotas(chimenea, { 0.4f, 1.0f, 0.4f }, NieblaPelotas(Color{ 118, 120, 128, 255 }, piso), 1.2f);
    CajaFacetadaPelotas({ chimenea.x, chimenea.y + 0.52f, chimenea.z }, { 0.46f, 0.08f, 0.46f }, nieve, 1.0f);

    // Lena apilada con nieve encima.
    CajaFacetadaPelotas({ piso.x - 1.85f, piso.y + 0.22f, piso.z + 0.6f }, { 0.7f, 0.44f, 0.9f }, maderaOscura, 1.2f);
    CajaFacetadaPelotas({ piso.x - 1.85f, piso.y + 0.48f, piso.z + 0.6f }, { 0.76f, 0.1f, 0.96f }, nieve, 1.0f);

    // Poste y banderines.
    Vector3 poste = { piso.x + 3.6f, piso.y, piso.z + 2.6f };
    DrawCylinderEx(poste, { poste.x, poste.y + 2.4f, poste.z }, 0.06f, 0.05f, 5, maderaOscura);
    DibujarBanderinesPelotas({ cd.x, cumbrera, cd.z }, { poste.x, poste.y + 2.35f, poste.z }, t, 0);
}


static Vector3 PuntoCablePelotas(Vector3 a, Vector3 b, float u, float comba)
{
    return
    {
        a.x + (b.x - a.x) * u,
        a.y + (b.y - a.y) * u - 4.0f * u * (1.0f - u) * comba,
        a.z + (b.z - a.z) * u
    };
}


static void DibujarTelefericoPelotas(float t)
{
    // Estacion sobre la estribacion derecha.
    const Vector3 piso = { 18.4f, -9.95f, -10.2f };
    Color hormigon = NieblaPelotas(Color{ 150, 156, 168, 255 }, piso);
    Color vidrio = NieblaPelotas(Color{ 60, 98, 128, 255 }, piso);
    Color nieve = NieblaPelotas(COLOR_NIEVE_PELOTAS, piso);
    Color acero = NieblaPelotas(Color{ 70, 76, 88, 255 }, piso);

    CajaFacetadaPelotas({ piso.x, piso.y + 0.6f, piso.z }, { 3.0f, 1.2f, 2.4f }, hormigon, 1.1f);
    CajaFacetadaPelotas({ piso.x - 0.2f, piso.y + 1.55f, piso.z }, { 2.4f, 0.7f, 2.0f }, vidrio, 1.2f);
    CajaFacetadaPelotas({ piso.x - 0.2f, piso.y + 2.0f, piso.z }, { 3.2f, 0.2f, 2.8f }, hormigon, 1.1f);
    CajaFacetadaPelotas({ piso.x - 0.2f, piso.y + 2.16f, piso.z }, { 3.1f, 0.14f, 2.7f }, nieve, 1.0f);

    // Rueda del cable.
    Vector3 rueda = { piso.x - 1.9f, piso.y + 1.7f, piso.z - 0.2f };
    DrawCylinderEx({ rueda.x, rueda.y - 0.05f, rueda.z }, { rueda.x, rueda.y + 0.05f, rueda.z }, 0.7f, 0.7f, 12, acero);
    DrawCylinderEx({ rueda.x, rueda.y - 0.9f, rueda.z }, { rueda.x, rueda.y, rueda.z }, 0.08f, 0.08f, 5, acero);

    // Torres y cables que se pierden en la niebla del valle.
    const Vector3 torre1 = { 6.0f, -8.6f, -40.0f };
    const Vector3 torre2 = { -16.0f, -9.0f, -78.0f };
    const Vector3 torres[2] = { torre1, torre2 };
    for (int k = 0; k < 2; k++)
    {
        Vector3 tope = torres[k];
        Color colorTorre = NieblaPelotas(Color{ 84, 90, 104, 255 }, tope);
        CajaFacetadaPelotas({ tope.x, tope.y - 5.0f, tope.z }, { 0.45f, 10.0f, 0.45f }, colorTorre, 1.1f);
        CajaFacetadaPelotas(tope, { 2.2f, 0.25f, 0.3f }, colorTorre, 1.1f);
    }

    const int TRAMOS = 14;
    for (int linea = 0; linea < 2; linea++)
    {
        float separacion = linea == 0 ? -0.55f : 0.55f;
        Vector3 a = { rueda.x + separacion, rueda.y, rueda.z };
        Vector3 b = { torre1.x + separacion, torre1.y, torre1.z };
        Vector3 c = { torre2.x + separacion, torre2.y, torre2.z };

        Vector3 anterior = a;
        for (int k = 1; k <= TRAMOS; k++)
        {
            Vector3 p = PuntoCablePelotas(a, b, (float)k / (float)TRAMOS, 1.2f);
            DrawLine3D(anterior, p, NieblaPelotas(Color{ 46, 50, 60, 255 }, p));
            anterior = p;
        }
        for (int k = 1; k <= TRAMOS; k++)
        {
            Vector3 p = PuntoCablePelotas(b, c, (float)k / (float)TRAMOS, 1.6f);
            DrawLine3D(anterior, p, NieblaPelotas(Color{ 46, 50, 60, 255 }, p));
            anterior = p;
        }
    }

    // Dos cabinas que van y vienen por el primer tramo.
    for (int k = 0; k < 2; k++)
    {
        float ciclo = std::fmod(t * 0.035f + (float)k * 0.5f, 1.0f);
        float u = ciclo < 0.5f ? ciclo * 2.0f : 2.0f - ciclo * 2.0f;
        u = 0.08f + u * 0.84f;
        float separacion = k == 0 ? -0.55f : 0.55f;
        Vector3 a = { rueda.x + separacion, rueda.y, rueda.z };
        Vector3 b = { torre1.x + separacion, torre1.y, torre1.z };
        Vector3 enganche = PuntoCablePelotas(a, b, u, 1.2f);
        Vector3 cuerpo = { enganche.x, enganche.y - 1.0f, enganche.z };
        float balanceo = std::sin(t * 1.3f + (float)k) * 0.04f;

        DrawLine3D(enganche, { cuerpo.x + balanceo, cuerpo.y + 0.4f, cuerpo.z }, NieblaPelotas(acero, cuerpo));
        CajaFacetadaPelotas({ cuerpo.x + balanceo, cuerpo.y, cuerpo.z }, { 0.9f, 0.75f, 0.75f }, NieblaPelotas(Color{ 206, 108, 64, 255 }, cuerpo), 1.2f);
        CajaFacetadaPelotas({ cuerpo.x + balanceo, cuerpo.y + 0.08f, cuerpo.z + 0.38f }, { 0.74f, 0.26f, 0.02f }, NieblaPelotas(Color{ 190, 224, 244, 255 }, cuerpo), 1.0f);
        CajaFacetadaPelotas({ cuerpo.x + balanceo, cuerpo.y + 0.42f, cuerpo.z }, { 0.94f, 0.08f, 0.8f }, NieblaPelotas(COLOR_NIEVE_PELOTAS, cuerpo), 1.0f);
    }

    // Banderines entre la estacion y un mastil.
    Vector3 mastil = { piso.x + 2.4f, piso.y, piso.z + 2.8f };
    DrawCylinderEx(mastil, { mastil.x, mastil.y + 2.6f, mastil.z }, 0.06f, 0.05f, 5, acero);
    DibujarBanderinesPelotas({ piso.x + 1.2f, piso.y + 2.1f, piso.z + 1.3f }, { mastil.x, mastil.y + 2.55f, mastil.z }, t, 2);
}


// Bruma baja alrededor de la montana: alli se pierden los que caen.
static void DibujarBrumaPelotas(const CimaPelotas& cima, float t)
{
    if (CalidadDecoracion() == CALIDAD_BAJA) return;

    int paso = CalidadDecoracion() == CALIDAD_ALTA ? 1 : 2;
    rlDrawRenderBatchActive();
    rlDisableDepthMask();

    for (int k = 0; k < 18; k += paso)
    {
        float h1 = Hash01Pelotas(k, 71);
        float h2 = Hash01Pelotas(k, 73);
        float angulo = (float)k / 18.0f * 6.2832f + t * 0.015f + h1 * 0.3f;
        Vector3 c = PuntoCimaPelotas(cima, angulo, 1.15f + h2 * 0.5f, -8.6f + h1 * 0.8f);
        Color color = NieblaPelotas(Color{ 244, 248, 252, 120 }, c);

        rlPushMatrix();
        rlTranslatef(c.x, c.y, c.z);
        rlRotatef(angulo * 57.3f, 0.0f, 1.0f, 0.0f);
        rlScalef(2.6f + h1 * 1.6f, 0.55f + h2 * 0.35f, 1.6f + h2);
        DrawSphereEx({ 0.0f, 0.0f, 0.0f }, 1.0f, 6, 10, color);
        rlPopMatrix();
    }

    rlDrawRenderBatchActive();
    rlEnableDepthMask();
}


//==================================================
// VISUAL: JUGADORES Y EFECTOS EN EL MUNDO
//==================================================

// Disco y anillos de color bajo los pies y flecha sobre la cabeza: el
// modelo del jugador es oscuro y sobre la nieve no se distingue quien es.
static void DibujarIndicadorJugadorPelotas(
    const JugadorPrueba& jugador,
    Color color,
    float t
)
{
    float pies = jugador.posicion.y - jugador.tamano.y * 0.5f;
    if (jugador.cayendo || pies < -0.35f) return;

    Vector3 centro = { jugador.posicion.x, 0.045f, jugador.posicion.z };
    const int LADOS = 18;
    Color relleno = Fade(color, 0.30f);
    rlBegin(RL_TRIANGLES);
    for (int k = 0; k < LADOS; k++)
    {
        float a0 = 6.2832f * (float)k / (float)LADOS;
        float a1 = 6.2832f * (float)(k + 1) / (float)LADOS;
        VerticePelotas(centro, relleno);
        VerticePelotas({ centro.x + std::cos(a1) * 0.52f, centro.y, centro.z + std::sin(a1) * 0.52f }, relleno);
        VerticePelotas({ centro.x + std::cos(a0) * 0.52f, centro.y, centro.z + std::sin(a0) * 0.52f }, relleno);
    }
    rlEnd();

    for (int k = 0; k < 3; k++)
    {
        DrawCircle3D(
            { jugador.posicion.x, 0.075f, jugador.posicion.z },
            0.52f + k * 0.045f,
            { 1.0f, 0.0f, 0.0f },
            90.0f,
            color
        );
    }

    float rebote = std::sin(t * 5.0f + jugador.posicion.x) * 0.06f;
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


// Las huellas son calcomanias translucidas que se pisan unas a otras en el
// mismo plano (y = 0.03): se prueban contra la profundidad pero no la
// escriben, asi se mezclan entre si y con las lineas grabadas del centro
// en vez de recortarse a manchas segun la precision del depth buffer.
static void DibujarHuellasPelotas(const MinijuegoPelotas& m)
{
    rlDrawRenderBatchActive();
    rlDisableDepthMask();

    for (int i = 0; i < MAX_HUELLAS_PELOTAS; i++)
    {
        const HuellaPelotas& h = m.huellas[i];
        if (!h.activa) continue;

        float vida = h.vida / VIDA_HUELLA_PELOTAS;
        Color color = { 176, 198, 224, (unsigned char)(150.0f * vida) };
        float c = std::cos(h.angulo);
        float s = std::sin(h.angulo);
        float largo = 0.28f;
        float ancho = h.ancho * 0.5f;
        Vector3 p = h.posicion;

        Vector3 a = { p.x - c * largo - s * ancho, p.y, p.z - s * largo + c * ancho };
        Vector3 b = { p.x + c * largo - s * ancho, p.y, p.z + s * largo + c * ancho };
        Vector3 d = { p.x - c * largo + s * ancho, p.y, p.z - s * largo - c * ancho };
        Vector3 e = { p.x + c * largo + s * ancho, p.y, p.z + s * largo - c * ancho };
        CuadPelotas(a, b, e, d, color, color, color, color);
    }

    rlDrawRenderBatchActive();
    rlEnableDepthMask();
}


static void DibujarParticulasPelotas(const MinijuegoPelotas& m)
{
    for (int i = 0; i < MAX_PARTICULAS_PELOTAS; i++)
    {
        const ParticulaPelotas& p = m.particulasNieve[i];
        if (!p.activa) continue;

        float vida = p.vida / p.vidaMaxima;
        float tamano = p.tamano;
        float alfa = 1.0f;

        switch (p.tipo)
        {
            case PARTICULA_PELOTAS_NIEVE:
                tamano *= 1.7f - 0.7f * vida;
                alfa = vida * 0.9f;
                break;
            case PARTICULA_PELOTAS_CHISPA:
                alfa = vida > 0.3f ? 1.0f : vida / 0.3f;
                break;
            case PARTICULA_PELOTAS_TROZO:
                alfa = vida > 0.25f ? 1.0f : vida / 0.25f;
                break;
            case PARTICULA_PELOTAS_HUMO:
                tamano *= 2.6f - 1.6f * vida;
                alfa = vida * 0.7f;
                break;
        }

        Color color = p.color;
        color.a = (unsigned char)((float)p.color.a * Limitar01Pelotas(alfa));
        DrawCubeV(p.posicion, { tamano, tamano, tamano }, color);
    }
}


static void DibujarCoposPelotas(const MinijuegoPelotas& m)
{
    int cantidad = (int)std::lround((float)MAX_COPOS_PELOTAS * FactorCalidadGrafica(CalidadParticulas()));
    Color color = { 250, 252, 255, 215 };

    for (int i = 0; i < cantidad && i < MAX_COPOS_PELOTAS; i++)
    {
        const CopoNievePelotas& copo = m.copos[i];
        DrawCubeV(copo.posicion, { copo.tamano, copo.tamano, copo.tamano }, color);
    }
}


static void DibujarOndasPelotas(const MinijuegoPelotas& m)
{
    for (int i = 0; i < MAX_ONDAS_PELOTAS; i++)
    {
        const OndaPelotas& onda = m.ondas[i];
        if (!onda.activa) continue;

        float u = 1.0f - onda.vida / onda.vidaMaxima;
        float radio = 0.2f + onda.radioMaximo * (1.0f - (1.0f - u) * (1.0f - u));
        Color color = Fade(onda.color, (1.0f - u) * 0.95f);

        for (int k = 0; k < 3; k++)
        {
            DrawCircle3D(onda.centro, radio + (float)k * 0.045f, { 1.0f, 0.0f, 0.0f }, 90.0f, color);
        }
    }
}


// Luces aditivas: brillos frios sobre el hielo, ventanas calidas y halos
// de choque. Van al final de la escena y nunca escriben profundidad.
static void DibujarLucesPelotas(
    const MinijuegoPelotas& m,
    const CimaPelotas& cima,
    const JugadorPrueba jugadores[],
    const Participante participantes[]
)
{
    if (FactorCalidadGrafica(CalidadEfectos()) < 0.5f) return;

    float t = m.tiempoAnimacion;
    rlDrawRenderBatchActive();
    rlDisableDepthMask();
    BeginBlendMode(BLEND_ADDITIVE);

    // Reflejos que recorren la franja de hielo del borde.
    for (int g = 0; g < 3; g++)
    {
        float angulo = t * 0.32f + (float)g * 2.094f;
        float pulso = 0.75f + 0.25f * std::sin(t * 2.3f + (float)g);
        Vector3 brillo = PuntoCimaPelotas(cima, angulo, 0.952f, 0.04f);
        ResplandorSueloPelotas(brillo, 1.0f * pulso, Color{ 150, 210, 255, 70 });
        ResplandorSueloPelotas(brillo, 0.35f * pulso, Color{ 230, 248, 255, 90 });
    }

    // Destellos sueltos de la nieve (centelleo fino, no marca nada).
    for (int k = 0; k < 26; k++)
    {
        float h1 = Hash01Pelotas(k, 201);
        float h2 = Hash01Pelotas(k, 203);
        float brillo = std::sin(t * (1.6f + h2 * 1.4f) + h1 * 40.0f);
        if (brillo < 0.82f) continue;
        float intensidad = (brillo - 0.82f) / 0.18f;
        Vector3 p = PuntoCimaPelotas(cima, h1 * 6.2832f, 0.15f + 0.8f * h2, 0.05f);
        ResplandorSueloPelotas(p, 0.16f, Color{ 255, 255, 255, (unsigned char)(200.0f * intensidad) });
    }

    // Ventanas del refugio y luz de la estacion.
    if (CalidadDecoracion() != CALIDAD_BAJA)
    {
        float titileo = 0.85f + 0.15f * std::sin(t * 7.0f) * std::sin(t * 2.3f);
        ResplandorFrontalPelotas({ -19.1f, -9.1f, -8.5f }, 0.9f * titileo, Color{ 255, 170, 80, 90 });
        ResplandorFrontalPelotas({ -17.65f, -9.1f, -8.5f }, 0.9f * titileo, Color{ 255, 170, 80, 90 });
        ResplandorSueloPelotas({ -18.4f, -9.9f, -7.9f }, 1.6f * titileo, Color{ 255, 160, 70, 40 });
        float faro = 0.6f + 0.4f * std::sin(t * 3.0f);
        ResplandorFrontalPelotas({ 18.2f, -8.4f, -9.1f }, 0.8f * faro, Color{ 255, 220, 150, 80 });
    }

    // Halo de cada choque y brillo de color bajo los jugadores en pie.
    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!participantes[i].activo || m.estadosJugadores[i].eliminado) continue;
        const JugadorPrueba& j = jugadores[i];
        if (j.cayendo || j.posicion.y < 0.2f) continue;

        float destello = m.estadosJugadores[i].destelloChoque;
        if (destello > 0.0f)
        {
            ResplandorFrontalPelotas({ j.posicion.x, j.posicion.y + 0.2f, j.posicion.z }, 1.4f * destello, Color{ 200, 236, 255, (unsigned char)(150.0f * destello) });
        }

        Color propio = participantes[i].color;
        ResplandorSueloPelotas({ j.posicion.x, 0.05f, j.posicion.z }, 0.95f, Color{ propio.r, propio.g, propio.b, 45 });
    }

    EndBlendMode();
    rlDrawRenderBatchActive();
    rlEnableDepthMask();
}


// «¡J2 AFUERA!» sube desde el borde por donde cayo.
static void DibujarAvisosPelotas(
    const MinijuegoPelotas& m,
    const Camera3D& vista,
    const Participante participantes[]
)
{
    for (int i = 0; i < MAX_AVISOS_PELOTAS; i++)
    {
        const AvisoPelotas& aviso = m.avisos[i];
        if (!aviso.activo) continue;

        float u = 1.0f - aviso.vida / DURACION_AVISO_PELOTAS;
        Vector3 p = { aviso.posicion.x, aviso.posicion.y + 0.8f + u * 1.4f, aviso.posicion.z };
        Vector2 s = GetWorldToScreen(p, vista);

        float ancho = (float)GetScreenWidth();
        if (s.y < Px(150.0f)) s.y = Px(150.0f);
        if (s.y > Px(630.0f)) s.y = Px(630.0f);
        if (s.x < Px(150.0f)) s.x = Px(150.0f);
        if (s.x > ancho - Px(150.0f)) s.x = ancho - Px(150.0f);

        float alfa = aviso.vida < 0.45f ? aviso.vida / 0.45f : 1.0f;
        if (u < 0.08f) alfa *= u / 0.08f;

        char texto[32];
        std::snprintf(texto, sizeof(texto), "¡J%d AFUERA!", participantes[aviso.jugador].numeroJugador);
        DibujarTextoMundoContornoUI(texto, s.x, s.y, UI_TEXTO_GRANDE, participantes[aviso.jugador].color, alfa);
    }
}


//==================================================
// LOGICA
//==================================================

static void InicializarResultadoPelotas(
    MinijuegoPelotas& minijuego,
    const Participante participantes[]
)
{
    InicializarResultadoMinijuego(
        minijuego.resultado,
        participantes,
        FORMATO_MINIJUEGO_INDIVIDUAL
    );
}


static int ContarJugadoresVivosPelotas(
    const MinijuegoPelotas& minijuego,
    const Participante participantes[]
)
{
    int cantidad = 0;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (
            participantes[i].activo &&
            minijuego.resultado.participantes[i].participo &&
            !minijuego.estadosJugadores[i].eliminado
        )
        {
            cantidad++;
        }
    }

    return cantidad;
}


// IA de bots: evita el borde, y si esta a salvo embiste a un rival desde
// el lado del centro para empujarlo hacia afuera. Un ruido lento le quita
// perfeccion y evita que se quede atascado.
static InputMinijuegoParticipante CrearEntradaBotPelotas(
    MinijuegoPelotas& minijuego,
    const JugadorPrueba jugadores[],
    const Participante participantes[],
    int indice,
    float deltaTime
)
{
    const JugadorPrueba& yo = jugadores[indice];
    EstadoBotPelotas& bot = minijuego.bots[indice];
    float t = minijuego.tiempoJugado;
    float distanciaCentro = MagnitudHorizontalPelotas(yo.posicion.x, yo.posicion.z);

    // Peligro: cerca del borde se vuelve al centro (con anticipacion por inercia).
    float proyectadaX = yo.posicion.x + yo.velocidad.x * 0.35f;
    float proyectadaZ = yo.posicion.z + yo.velocidad.z * 0.35f;
    float distanciaProyectada = MagnitudHorizontalPelotas(proyectadaX, proyectadaZ);

    bool peligro = distanciaCentro > 4.3f || distanciaProyectada > 4.7f;

    if (!peligro)
    {
        bot.esperaBorde = 0.0f;
    }
    else
    {
        if (bot.esperaBorde <= 0.0f)
            bot.retardoBorde = (float)GetRandomValue(200, 400) / 1000.0f;
        bot.esperaBorde += deltaTime;
    }

    if (peligro && bot.esperaBorde >= bot.retardoBorde)
    {
        return CrearEntradaBotHaciaObjetivo1v3(
            yo.posicion,
            { 0.0f, yo.posicion.y, 0.0f },
            0.25f
        );
    }

    int objetivo = -1;
    float mejor = 1000000.0f;

    for (int j = 0; j < MAX_PARTICIPANTES; j++)
    {
        if (
            j == indice ||
            !participantes[j].activo ||
            !minijuego.resultado.participantes[j].participo ||
            minijuego.estadosJugadores[j].eliminado ||
            jugadores[j].cayendo
        )
        {
            continue;
        }

        float dx = jugadores[j].posicion.x - yo.posicion.x;
        float dz = jugadores[j].posicion.z - yo.posicion.z;
        float distancia = dx * dx + dz * dz;

        if (distancia < mejor)
        {
            mejor = distancia;
            objetivo = j;
        }
    }

    if (objetivo < 0)
    {
        return CrearEntradaBotHaciaObjetivo1v3(yo.posicion, { 0.0f, yo.posicion.y, 0.0f }, 0.5f);
    }

    Vector3 rival = jugadores[objetivo].posicion;
    float distanciaRival = MagnitudHorizontalPelotas(rival.x, rival.z);
    float haciaFueraX = distanciaRival > 0.001f ? rival.x / distanciaRival : 1.0f;
    float haciaFueraZ = distanciaRival > 0.001f ? rival.z / distanciaRival : 0.0f;

    // Estoy del lado del centro respecto al rival?
    float lado =
        (yo.posicion.x - rival.x) * haciaFueraX +
        (yo.posicion.z - rival.z) * haciaFueraZ;

    Vector3 meta = rival;

    if (lado > -0.4f)
    {
        // Rodear: ponerse entre el centro y el rival antes de embestir.
        meta.x = rival.x - haciaFueraX * 2.4f;
        meta.z = rival.z - haciaFueraZ * 2.4f;
    }

    // Error lento de punteria y pausas cortas de duda.
    meta.x += std::sin(t * 1.3f + bot.faseX) * bot.amplitudError;
    meta.z += std::cos(t * 1.1f + bot.faseZ) * bot.amplitudError;

    if (std::sin(t * 0.7f + bot.faseDuda) > 0.93f)
    {
        meta = { 0.0f, yo.posicion.y, 0.0f };
    }

    return CrearEntradaBotHaciaObjetivo1v3(yo.posicion, meta, 0.3f);
}


static void FinalizarResultadoPelotas(
    MinijuegoPelotas& minijuego,
    const JugadorPrueba jugadores[],
    const Participante participantes[]
)
{
    if (minijuego.resultado.estado != RESULTADO_MINIJUEGO_EN_CURSO)
    {
        return;
    }

    int vivos = ContarJugadoresVivosPelotas(minijuego, participantes);
    minijuego.terminoPorTiempo = vivos > 1;

    // Solo para el tablon: distancia al centro de quien sigue en pie.
    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        minijuego.estadosJugadores[i].distanciaCentroFinal =
            MagnitudHorizontalPelotas(jugadores[i].posicion.x, jugadores[i].posicion.z);
    }

    // Desempate por tiempo agotado: entre los que siguen en pie gana quien
    // esta mas cerca del centro de la montana.
    if (vivos > 1)
    {
        float distancias[MAX_PARTICIPANTES]{};

        for (int i = 0; i < MAX_PARTICIPANTES; i++)
        {
            distancias[i] = MagnitudHorizontalPelotas(
                jugadores[i].posicion.x,
                jugadores[i].posicion.z
            );
        }

        for (int i = 0; i < MAX_PARTICIPANTES; i++)
        {
            if (
                !participantes[i].activo ||
                !minijuego.resultado.participantes[i].participo ||
                minijuego.estadosJugadores[i].eliminado
            )
            {
                continue;
            }

            int posicion = 1;

            for (int j = 0; j < MAX_PARTICIPANTES; j++)
            {
                if (
                    j != i &&
                    participantes[j].activo &&
                    minijuego.resultado.participantes[j].participo &&
                    !minijuego.estadosJugadores[j].eliminado &&
                    distancias[j] < distancias[i] - 0.05f
                )
                {
                    posicion++;
                }
            }

            minijuego.estadosJugadores[i].posicionFinal = posicion;
            minijuego.estadosJugadores[i].tiempoSobrevividoMs =
                (int)std::lround(minijuego.tiempoJugado * 1000.0f);
        }
    }

    int primeros = 0;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (
            minijuego.resultado.participantes[i].participo &&
            (minijuego.estadosJugadores[i].eliminado
                ? minijuego.estadosJugadores[i].posicionFinal == 1
                : (vivos > 1
                    ? minijuego.estadosJugadores[i].posicionFinal == 1
                    : true))
        )
        {
            primeros++;
        }
    }

    minijuego.resultado.estado = RESULTADO_MINIJUEGO_FINALIZADO;
    minijuego.resultado.desenlace =
        primeros == 1 ? DESENLACE_CON_GANADOR : DESENLACE_EMPATE;
    ReproducirSonidoMinijuego(minijuego.audio, SONIDO_RESULTADO);

    int tiempoFinalMs =
        (int)std::lround(minijuego.tiempoJugado * 1000.0f);

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        ResultadoParticipante& resultadoJugador =
            minijuego.resultado.participantes[i];

        if (!resultadoJugador.participo)
        {
            continue;
        }

        EstadoJugadorPelotas& estadoJugador = minijuego.estadosJugadores[i];

        if (!estadoJugador.eliminado)
        {
            if (vivos <= 1)
            {
                estadoJugador.posicionFinal = 1;
            }

            estadoJugador.tiempoSobrevividoMs = tiempoFinalMs;
        }

        resultadoJugador.posicionFinal = estadoJugador.posicionFinal;
        resultadoJugador.numeroEquipo = -1;
        resultadoJugador.puntuacionMinijuego = estadoJugador.tiempoSobrevividoMs;
        resultadoJugador.puntosObtenidos = 0;
    }

    minijuego.fase = FASE_PELOTAS_TERMINADO;
}


static void SortearBotsPelotas(MinijuegoPelotas& minijuego)
{
    for (int i = 0; i < MAX_JUGADORES_PRUEBA; i++)
    {
        EstadoBotPelotas& bot = minijuego.bots[i];
        bot = {};
        bot.faseX = (float)GetRandomValue(0, 6283) / 1000.0f;
        bot.faseZ = (float)GetRandomValue(0, 6283) / 1000.0f;
        bot.faseDuda = (float)GetRandomValue(0, 6283) / 1000.0f;
        bot.amplitudError = (float)GetRandomValue(600, 1400) / 1000.0f;
    }
}


void MinijuegoPelotas::Inicializar()
{
    resultado = {};
    resultado.formato = FORMATO_MINIJUEGO_INDIVIDUAL;
    fase = FASE_PELOTAS_PREPARACION;
    tiempoPreparacion = DURACION_PREPARACION_PELOTAS;
    tiempoRestante = DURACION_PARTIDA_PELOTAS;
    tiempoJugado = 0.0f;
    terminoPorTiempo = false;
    tiempoAnimacion = 0.0f;
    SortearBotsPelotas(*this);

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        estadosJugadores[i] = {};
    }

    ReiniciarEfectosPelotas(*this);

    cantidadBloques = 0;

    AgregarBloquePrueba(
        bloques,
        cantidadBloques,
        1,
        { 0.0f, -0.5f, 2.25f },
        { 16.0f, 1.0f, 16.0f },
        RAYWHITE
    );

    camara.position = { 0.0f, 9.0f, 14.2f };
    camara.target = { 0.0f, -0.15f, 0.0f };
    camara.up = { 0.0f, 1.0f, 0.0f };
    camara.fovy = 52.0f;
    camara.projection = CAMERA_PERSPECTIVE;
}


void MinijuegoPelotas::ConfigurarJugadores(
    JugadorPrueba jugadores[],
    int cantidadMaxima
) const
{
    Vector3 spawns[MAX_JUGADORES_PRUEBA] =
    {
        { -2.2f, 0.65f,  2.2f },
        {  2.2f, 0.65f,  2.2f },
        { -2.2f, 0.65f, -2.2f },
        {  2.2f, 0.65f, -2.2f }
    };

    int limite =
        cantidadMaxima < MAX_JUGADORES_PRUEBA
        ? cantidadMaxima
        : MAX_JUGADORES_PRUEBA;

    for (int i = 0; i < limite; i++)
    {
        jugadores[i].posicionSpawn = spawns[i];
        jugadores[i].tamano = { 1.30f, 1.30f, 1.30f };
        jugadores[i].velocidadMovimiento = VELOCIDAD_MAXIMA_PELOTAS;
        jugadores[i].fuerzaSalto = 0.0f;
        jugadores[i].gravedad = 18.0f;
        jugadores[i].duracionRespawn = 1.2f;
        ReiniciarJugadorPrueba(jugadores[i]);
    }
}


void MinijuegoPelotas::Reiniciar(
    JugadorPrueba jugadores[],
    int cantidadMaxima
)
{
    bool participantesAnteriores[MAX_PARTICIPANTES]{};

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        participantesAnteriores[i] = resultado.participantes[i].participo;
    }

    resultado = {};
    resultado.formato = FORMATO_MINIJUEGO_INDIVIDUAL;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        resultado.participantes[i].participo = participantesAnteriores[i];
        if (participantesAnteriores[i]) resultado.cantidadParticipantes++;
        estadosJugadores[i] = {};
    }

    fase = FASE_PELOTAS_PREPARACION;
    tiempoPreparacion = DURACION_PREPARACION_PELOTAS;
    tiempoRestante = DURACION_PARTIDA_PELOTAS;
    tiempoJugado = 0.0f;
    terminoPorTiempo = false;
    tiempoAnimacion = 0.0f;
    SortearBotsPelotas(*this);
    ReiniciarEfectosPelotas(*this);

    for (int i = 0; i < cantidadMaxima; i++)
    {
        ReiniciarJugadorPrueba(jugadores[i]);
    }
}


void MinijuegoPelotas::Actualizar(
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
        InicializarResultadoPelotas(*this, participantes);
    }

    ActualizarEfectosPelotas(*this, deltaTime);

    if (fase == FASE_PELOTAS_TERMINADO)
    {
        ActualizarFestejoPelotas(*this, jugadores, participantes, deltaTime);
        return;
    }

    if (fase == FASE_PELOTAS_PREPARACION)
    {
        for (int i = 0; i < MAX_PARTICIPANTES; i++)
        {
            jugadores[i].velocidad = {};
            jugadores[i].empuje = {};
        }

        float preparacionAntes = tiempoPreparacion;
        tiempoPreparacion -= deltaTime;
        ActualizarAudioCuentaRegresiva(audio, preparacionAntes, tiempoPreparacion);

        if (tiempoPreparacion <= 0.0f)
        {
            tiempoPreparacion = 0.0f;
            fase = FASE_PELOTAS_JUGANDO;
        }

        return;
    }

    tiempoJugado += deltaTime;

    int vivosAntes = ContarJugadoresVivosPelotas(*this, participantes);

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        JugadorPrueba& jugador = jugadores[i];

        if (estadosJugadores[i].eliminado)
        {
            jugador.cayendo = true;
            jugador.velocidad = {};
            jugador.empuje = {};
            continue;
        }

        if (!participantes[i].activo || !participantes[i].conectado)
        {
            continue;
        }

        // Gracia inicial: los bots esperan para que un humano se oriente.
        InputMinijuegoParticipante entrada{};

        if (participantes[i].esBot)
        {
            if (tiempoJugado >= GRACIA_INICIAL_PELOTAS + bots[i].faseDuda * 0.08f)
            {
                entrada = CrearEntradaBotPelotas(*this, jugadores, participantes, i, deltaTime);
            }
        }
        else
        {
            entrada = LeerInputMinijuegoParticipante(participantes[i]);
        }

        float velocidadAnteriorX = jugador.velocidad.x;
        float velocidadAnteriorZ = jugador.velocidad.z;
        Vector3 posicionAnterior = jugador.posicion;
        Vector3 empujePendiente = jugador.empuje;
        jugador.empuje = {};

        BloquePrueba sueloJugador = bloques[0];
        sueloJugador.activaColision =
            JugadorSobreArenaCircularPelotas(jugador, camara);

        ActualizarJugadorPrueba(
            jugador,
            entrada,
            &sueloJugador,
            1,
            particulas,
            cantidadParticulas,
            false,
            true,
            false,
            deltaTime
        );

        bool sobreArenaDespuesDeMover =
            JugadorSobreArenaCircularPelotas(jugador, camara);

        if (!sobreArenaDespuesDeMover)
        {
            if (sueloJugador.activaColision && jugador.enSuelo)
            {
                jugador.velocidad.y -= jugador.gravedad * deltaTime;
                jugador.posicion.y += jugador.velocidad.y * deltaTime;
            }

            jugador.enSuelo = false;
        }

        LimitarMovimientoPelota(
            jugador,
            velocidadAnteriorX,
            velocidadAnteriorZ,
            deltaTime
        );

        AplicarEmpujePendientePelotas(jugador, empujePendiente);
        ResolverColisionMontanaPelotas(jugador, posicionAnterior);
    }

    // Se guarda el estado previo al choque solo para el feedback visual.
    int limite = cantidadMaxima < MAX_PARTICIPANTES ? cantidadMaxima : MAX_PARTICIPANTES;
    float cooldownAntes[MAX_PARTICIPANTES]{};
    Vector3 empujeAntes[MAX_PARTICIPANTES]{};

    for (int i = 0; i < limite; i++)
    {
        cooldownAntes[i] = jugadores[i].cooldownChoque;
        empujeAntes[i] = jugadores[i].empuje;
    }

    ResolverColisionesPelotas(jugadores, participantes, cantidadMaxima);
    DetectarChoquesPelotas(*this, jugadores, participantes, limite, cooldownAntes, empujeAntes);

    if (tiempoJugado >= GRACIA_INICIAL_PELOTAS)
    {
        PotenciarEmpujesPelotas(jugadores, participantes, cantidadMaxima);
    }

    ActualizarEstelasYCaidasPelotas(*this, jugadores, participantes, limite, deltaTime);

    int eliminadosEsteFrame = 0;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (
            resultado.participantes[i].participo &&
            !estadosJugadores[i].eliminado &&
            jugadores[i].cayendo
        )
        {
            eliminadosEsteFrame++;
        }
    }

    int posicionEliminados = vivosAntes - eliminadosEsteFrame + 1;
    int tiempoSobrevividoMs =
        (int)std::lround(tiempoJugado * 1000.0f);

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (
            resultado.participantes[i].participo &&
            !estadosJugadores[i].eliminado &&
            jugadores[i].cayendo
        )
        {
            RegistrarEliminacionPelotas(*this, jugadores[i], i);
            estadosJugadores[i].eliminado = true;
            estadosJugadores[i].posicionFinal = posicionEliminados;
            estadosJugadores[i].tiempoSobrevividoMs = tiempoSobrevividoMs;
            jugadores[i].velocidad = {};
            jugadores[i].empuje = {};
        }
    }

    int vivosDespues = vivosAntes - eliminadosEsteFrame;

    float restanteAntes = tiempoRestante;
    tiempoRestante -= deltaTime;
    ActualizarAudioAlertaTiempo(audio, restanteAntes, tiempoRestante);

    // La caida ya suena desde la utilidad compartida; aca se marca la baja.
    if (eliminadosEsteFrame > 0)
    {
        ReproducirSonidoMinijuego(audio, SONIDO_ELIMINADO);
    }

    if (vivosDespues <= 1 || tiempoRestante <= 0.0f)
    {
        if (tiempoRestante < 0.0f) tiempoRestante = 0.0f;
        FinalizarResultadoPelotas(*this, jugadores, participantes);
    }
}


void MinijuegoPelotas::Dibujar(
    const JugadorPrueba jugadores[],
    int cantidadMaxima,
    const Participante participantes[],
    bool mostrarDebug
) const
{
    (void)cantidadMaxima;

    // Escenario alpino propio: sin el tema de nieve global.

    float t = tiempoAnimacion;
    CimaPelotas cima = CalcularCimaPelotas(camara);

    // Intro durante la cuenta: solo cambia la vista; la logica usa la
    // camara fija (de ella depende el borde de caida).
    Camera3D vista = camara;
    if (fase == FASE_PELOTAS_PREPARACION)
    {
        float u = Limitar01Pelotas(tiempoPreparacion / DURACION_PREPARACION_PELOTAS);
        float suave = u * u * (3.0f - 2.0f * u) * ObtenerFactorMovimientoCamaraMinijuegos();
        vista.position.y += 2.6f * suave;
        vista.position.z += 3.2f * suave;
    }

    ClearBackground(COLOR_NIEBLA_PELOTAS);
    DibujarCieloPelotas(t);

    BeginMode3D(vista);
    CulloPelotas(false);

    // Fondo: cordilleras en capas con niebla, macizos y mar de nubes.
    DibujarCordilleraPelotas(-100.0f, -170.0f, 170.0f, 9, 4.0f, 16.0f, 3);
    DibujarCordilleraPelotas(-74.0f, -130.0f, 130.0f, 8, -4.0f, 8.0f, 5);
    DibujarCordilleraPelotas(-52.0f, -90.0f, 90.0f, 7, -12.0f, -1.0f, 7);
    DibujarPicoPelotas({ -36.0f, -32.0f, -24.0f }, 6.0f, 16.0f, 12.0f, 3.0f, 0.36f, 901);
    DibujarPicoPelotas({ 38.0f, -32.0f, -20.0f }, 3.0f, 15.0f, 11.0f, -3.0f, 0.38f, 902);
    DibujarMarNubesPelotas(t);

    // Plano medio: loma con bosque, estribaciones, refugio y teleferico.
    if (CalidadDecoracion() != CALIDAD_BAJA) DibujarLomaFondoPelotas();
    DibujarEstribacionPelotas(-1.0f);
    DibujarEstribacionPelotas(1.0f);
    DibujarPinosPelotas();
    if (CalidadDecoracion() != CALIDAD_BAJA)
    {
        DibujarRefugioPelotas(t);
        DibujarTelefericoPelotas(t);
    }

    // La cima: paredes, cornisa, nieve, hielo del borde y centro.
    DibujarParedesCimaPelotas(cima);
    DibujarDetallesParedPelotas(cima);
    DibujarSuperficieCimaPelotas(cima);
    DibujarOndulacionesNievePelotas(cima);
    DibujarCentroCimaPelotas();
    DibujarHuellasPelotas(*this);
    DibujarOndasPelotas(*this);

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (estadosJugadores[i].eliminado)
        {
            continue;
        }

        if (participantes[i].activo)
        {
            DibujarIndicadorJugadorPelotas(jugadores[i], participantes[i].color, t);
        }

        DibujarJugadorPelotaPrueba(jugadores[i], participantes[i]);

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

    DibujarParticulasPelotas(*this);
    DibujarBrumaPelotas(cima, t);
    DibujarCoposPelotas(*this);
    DibujarLucesPelotas(*this, cima, jugadores, participantes);

    CulloPelotas(true);
    EndMode3D();

    if (mostrarDebug)
    {
        DibujarContornoColisionMontanaPelotas(camara);
    }

    DibujarAvisosPelotas(*this, vista, participantes);

    // HUD comun (Minigames/HudMinijuego): solo lee el estado.
    DatosHudMinijuego hud;
    PrepararDatosHudMinijuego(
        hud,
        "PELOTAS: EMPUJONES",
        "SACÁ A LOS RIVALES DE LA CIMA. SI SE ACABA EL TIEMPO, GANA EL MÁS CERCANO AL CENTRO.",
        resultado
    );

    hud.fase = fase == FASE_PELOTAS_PREPARACION
        ? HUD_MINIJUEGO_CUENTA
        : (fase == FASE_PELOTAS_JUGANDO ? HUD_MINIJUEGO_JUGANDO : HUD_MINIJUEGO_FIN);
    hud.cuentaRestante = tiempoPreparacion;
    hud.reloj = HUD_RELOJ_CUENTA_ATRAS;
    hud.tiempoRestante = tiempoRestante > 0.0f ? tiempoRestante : 0.0f;
    hud.tiempoTotal = DURACION_PARTIDA_PELOTAS;
    hud.segundosAlerta = 10.0f;
    hud.finPorTiempo = terminoPorTiempo;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        JugadorHudMinijuego& jugador = hud.jugadores[i];
        if (!jugador.visible) continue;

        const EstadoJugadorPelotas& estado = estadosJugadores[i];
        jugador.eliminado = estado.eliminado;

        if (!estado.eliminado)
        {
            // Barra de velocidad: mas velocidad, mas empuje.
            float velocidad = MagnitudHorizontalPelotas(jugadores[i].velocidad.x, jugadores[i].velocidad.z);
            jugador.progreso = fase == FASE_PELOTAS_JUGANDO
                ? Limitar01Pelotas(velocidad / VELOCIDAD_MAXIMA_PELOTAS)
                : 0.0f;

            if (fase == FASE_PELOTAS_JUGANDO)
            {
                if (estado.fueraDeCima)
                {
                    std::snprintf(jugador.estado, sizeof(jugador.estado), "CAYENDO");
                }
                else if (
                    jugadores[i].enSuelo &&
                    MargenBordePelotas(cima, jugadores[i].posicion.x, jugadores[i].posicion.z) < MARGEN_AVISO_BORDE_PELOTAS
                )
                {
                    std::snprintf(jugador.estado, sizeof(jugador.estado), "¡AL BORDE!");
                }
            }
        }

        if (!participantes[i].esBot)
        {
            std::snprintf(
                jugador.controles,
                sizeof(jugador.controles),
                "MOVER: %s\nCHOCÁ FUERTE PARA EMPUJAR",
                TextoMoverHud(participantes[i])
            );
        }

        int ms = resultado.participantes[i].puntuacionMinijuego;
        if (estado.eliminado)
        {
            std::snprintf(jugador.detalleFinal, sizeof(jugador.detalleFinal), "CAYÓ A LOS %d,%d S", ms / 1000, (ms % 1000) / 100);
        }
        else
        {
            std::snprintf(jugador.detalleFinal, sizeof(jugador.detalleFinal), "EN PIE");
        }
    }

    // Tiempo agotado con varios arriba: se explica el desempate.
    if (
        hud.fase == HUD_MINIJUEGO_FIN &&
        resultado.estado == RESULTADO_MINIJUEGO_FINALIZADO &&
        terminoPorTiempo
    )
    {
        for (int i = 0; i < MAX_PARTICIPANTES; i++)
        {
            if (!hud.jugadores[i].visible || estadosJugadores[i].eliminado) continue;

            int decimas = (int)std::lround(estadosJugadores[i].distanciaCentroFinal * 10.0f);
            std::snprintf(
                hud.jugadores[i].detalleDesempate,
                sizeof(hud.jugadores[i].detalleDesempate),
                "A %d,%d M DEL CENTRO",
                decimas / 10,
                decimas % 10
            );
        }

        hud.notaFinal = "TIEMPO: ENTRE LOS QUE SIGUEN ARRIBA, GANA EL MÁS CERCANO AL CENTRO";
    }

    DibujarHudMinijuego(hud, participantes);
}


const ResultadoMinijuego&
MinijuegoPelotas::ObtenerResultado() const
{
    return resultado;
}
