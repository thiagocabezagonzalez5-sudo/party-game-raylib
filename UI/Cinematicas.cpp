#include "UI/Cinematicas.h"

#include "Config.h"
#include "Entities/PersonajesBloque.h"
#include "UI/EstiloUIGlobal.h"

#include "raymath.h"
#include "rlgl.h"

#include <cmath>
#include <cstdio>
#include <cstring>


//==================================================
// UTILIDADES COMUNES
//==================================================

// Tiempo minimo antes de aceptar "omitir": la pulsacion que llevo a esta
// pantalla no debe saltarla en el mismo frame.
static const float MINIMO_PARA_OMITIR = 0.4f;


static void AvanzarCinematica(
    EstadoCinematica& c,
    float deltaTime
)
{
    if (c.terminada)
    {
        return;
    }

    c.tiempo += deltaTime;

    EntradaUI entrada = LeerEntradaUI(c.lector);

    if (c.tiempo >= MINIMO_PARA_OMITIR && (entrada.aceptar || entrada.volver))
    {
        c.terminada = true;
        c.omitida = true;
    }

    if (c.tiempo >= c.duracion)
    {
        c.terminada = true;
    }
}


static void ReiniciarCinematica(
    EstadoCinematica& c,
    float duracion
)
{
    c.tiempo = 0.0f;
    c.duracion = duracion;
    c.terminada = false;
    c.omitida = false;
    c.lector = LectorEntradaUI{};
}


static float Tramo(float t, float inicio, float duracion)
{
    if (duracion <= 0.0f) return t >= inicio ? 1.0f : 0.0f;
    float u = (t - inicio) / duracion;
    return u < 0.0f ? 0.0f : (u > 1.0f ? 1.0f : u);
}


static float Suave(float u)
{
    return u * u * (3.0f - 2.0f * u);
}


// Sale con un pequeno rebote al final (entrada de titulos).
static float SalidaElastica(float u)
{
    if (u <= 0.0f) return 0.0f;
    if (u >= 1.0f) return 1.0f;
    return 1.0f + 1.6f * std::pow(1.0f - u, 3.0f) * std::sin(u * 9.0f) - std::pow(1.0f - u, 2.0f);
}


static float Hash01(int a, int b)
{
    unsigned int h = ((unsigned int)a * 73856093u) ^ ((unsigned int)b * 19349663u);
    h ^= h >> 13;
    h *= 0x5bd1e995u;
    h ^= h >> 15;
    return (float)(h & 0xFFFFu) / 65535.0f;
}


static bool ReducirMovimiento()
{
    return ObtenerEstiloUI().reducirMovimiento;
}


// Franjas negras de cine (0 = fuera, 1 = puestas).
static void DibujarFranjasCine(float k)
{
    if (k <= 0.0f) return;

    int alto = GetScreenHeight();
    int franja = (int)(alto * 0.095f * Suave(k));

    DrawRectangle(0, 0, GetScreenWidth(), franja, BLACK);
    DrawRectangle(0, alto - franja, GetScreenWidth(), franja, BLACK);
}


static void DibujarAyudaOmitir(float alfa)
{
    if (alfa <= 0.0f) return;

    DibujarTextoMundoContornoUI(
        "ENTER / A: OMITIR",
        GetScreenWidth() - Px(120.0f),
        GetScreenHeight() - Px(40.0f),
        UI_TEXTO_PEQUENO,
        UI_TEXTO_MUNDO,
        0.75f * alfa
    );
}


static void DibujarFundido(float alfa, Color color = BLACK)
{
    if (alfa <= 0.0f) return;
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(color, alfa > 1.0f ? 1.0f : alfa));
}


static const Color PALETA_CONFETI[6] =
{
    { 255, 90, 90, 255 }, { 255, 200, 60, 255 }, { 90, 200, 120, 255 },
    { 80, 160, 255, 255 }, { 200, 110, 240, 255 }, { 255, 255, 255, 255 }
};


// Confeti 3D cayendo dentro de una caja centrada en "centro".
static void DibujarConfeti3D(
    Vector3 centro,
    float ancho,
    float alto,
    float t,
    int cantidad
)
{
    for (int k = 0; k < cantidad; k++)
    {
        float hx = Hash01(k, 1), hz = Hash01(k, 2), hv = Hash01(k, 3), hy = Hash01(k, 4);
        float velocidad = 1.6f + 1.4f * hv;
        float y = centro.y + alto - std::fmod(t * velocidad + hy * alto, alto);
        float vaiven = std::sin(t * (2.0f + hv * 2.0f) + (float)k) * 0.25f;

        Vector3 p = {
            centro.x + (hx - 0.5f) * ancho + vaiven,
            y,
            centro.z + (hz - 0.5f) * ancho * 0.6f
        };

        float lado = 0.07f + 0.05f * hx;
        DrawCube(p, lado, lado * 0.25f, lado, PALETA_CONFETI[k % 6]);
    }
}


// Haz de reflector (aditivo, sin escribir profundidad).
static void DibujarHaz(Vector3 foco, Vector3 suelo, float radio, Color color)
{
    rlDrawRenderBatchActive();
    rlDisableDepthMask();
    BeginBlendMode(BLEND_ADDITIVE);
    DrawCylinderEx(suelo, foco, radio, 0.08f, 18, color);
    EndBlendMode();
    rlDrawRenderBatchActive();
    rlEnableDepthMask();
}


static void DibujarCieloNoche(Color arriba, Color abajo)
{
    // Limpia color y PROFUNDIDAD: sin esto la escena 3D se compararia con
    // el depth buffer del frame anterior.
    ClearBackground(arriba);
    DrawRectangleGradientV(0, 0, GetScreenWidth(), GetScreenHeight(), arriba, abajo);

    for (int k = 0; k < 70; k++)
    {
        float x = Hash01(k, 11) * GetScreenWidth();
        float y = Hash01(k, 12) * GetScreenHeight() * 0.55f;
        float brillo = 0.5f + 0.5f * std::sin((float)GetTime() * (1.0f + Hash01(k, 13) * 2.0f) + (float)k);
        DrawRectangle((int)x, (int)y, 2, 2, Fade(WHITE, 0.25f + 0.55f * brillo));
    }
}


// Tablado redondo del escenario.
static void DibujarTablado(float radio)
{
    DrawCylinder({ 0.0f, -0.35f, 0.0f }, radio + 0.25f, radio + 0.25f, 0.30f, 40, Color{ 70, 40, 30, 255 });
    DrawCylinder({ 0.0f, -0.08f, 0.0f }, radio, radio, 0.08f, 40, Color{ 156, 102, 64, 255 });

    for (int k = 0; k < 24; k++)
    {
        float a = (float)k / 24.0f * 2.0f * PI;
        Vector3 p = { std::cos(a) * (radio + 0.12f), -0.02f, std::sin(a) * (radio + 0.12f) };
        bool encendida = ((int)(GetTime() * 6.0f) + k) % 3 == 0;
        DrawSphere(p, 0.07f, encendida ? Color{ 255, 230, 140, 255 } : Color{ 150, 110, 70, 255 });
    }
}


static void EtiquetaSobre(
    const Camera3D& camara,
    Vector3 punto,
    const char* texto,
    Color color,
    float alfa,
    int token = UI_TEXTO
)
{
    if (alfa <= 0.0f) return;
    Vector2 p = GetWorldToScreen(punto, camara);
    DibujarTextoMundoContornoUI(texto, p.x, p.y, token, color, alfa);
}


// Token de texto que entra en anchoMaximo pixeles (nunca menor a 10).
static int TokenAjustado(const char* texto, int token, float anchoMaximo)
{
    float pixeles = (float)TamanoTextoAjustadoUI(texto, token, anchoMaximo);
    int ajustado = (int)(pixeles / EscalaUI());
    return ajustado < 10 ? 10 : (ajustado > token ? token : ajustado);
}


static Color ColorTexto(Color c)
{
    // Aclara colores muy oscuros para que lean sobre el fondo.
    return Color{
        (unsigned char)(c.r + (255 - c.r) * 0.25f),
        (unsigned char)(c.g + (255 - c.g) * 0.25f),
        (unsigned char)(c.b + (255 - c.b) * 0.25f),
        255
    };
}


//==================================================
// APERTURA
//==================================================

void CinematicaApertura::Inicializar()
{
    ReiniciarCinematica(estado, DURACION);
}


void CinematicaApertura::Actualizar(float deltaTime)
{
    AvanzarCinematica(estado, deltaTime);
}


void CinematicaApertura::Dibujar() const
{
    const float t = estado.tiempo;
    const bool reducir = ReducirMovimiento();

    DibujarCieloNoche(Color{ 18, 16, 48, 255 }, Color{ 92, 54, 110, 255 });

    // Camara: de cerca y baja a abierta y alta mientras festejan.
    float abrir = reducir ? 1.0f : Suave(Tramo(t, 4.3f, 1.8f));
    float vaiven = reducir ? 0.0f : std::sin(t * 0.6f) * 0.8f * (1.0f - abrir * 0.5f);

    Camera3D camara{};
    camara.position = { vaiven, 1.5f + 2.6f * abrir, 6.0f + 5.5f * abrir };
    camara.target = { 0.0f, 1.0f + 0.2f * abrir, 0.0f };
    camara.up = { 0.0f, 1.0f, 0.0f };
    camara.fovy = 42.0f;
    camara.projection = CAMERA_PERSPECTIVE;

    const float LLEGADA0 = 0.7f;
    const float ENTRE_LLEGADAS = 0.75f;
    const float CAIDA = 0.45f;
    const float xs[4] = { -2.4f, -0.8f, 0.8f, 2.4f };
    const Color remeras[4] = { { 230, 70, 70, 255 }, { 70, 130, 235, 255 }, { 70, 195, 100, 255 }, { 240, 200, 60, 255 } };

    BeginMode3D(camara);

    DibujarTablado(4.2f);

    // Reflectores que barren el escenario.
    for (int k = 0; k < 3; k++)
    {
        float barrido = reducir ? 0.0f : std::sin(t * 0.9f + (float)k * 2.1f) * 2.6f;
        Color luz = k == 1 ? Color{ 255, 220, 150, 26 } : Color{ 160, 190, 255, 20 };
        DibujarHaz({ -6.0f + 6.0f * k, 9.0f, -3.0f }, { barrido + (k - 1) * 1.6f, 0.0f, 0.4f }, 1.1f, luz);
    }

    for (int i = 0; i < CANTIDAD_PERSONAJES_BLOQUE; i++)
    {
        float llegada = LLEGADA0 + ENTRE_LLEGADAS * i;
        float u = Tramo(t, llegada - CAIDA, CAIDA);

        if (u <= 0.0f) continue;

        PosePersonajeBloque pose;
        pose.tiempo = t + i * 0.7f;

        float y = 7.0f * (1.0f - u * u);
        pose.enAire = u < 1.0f;

        // Aterrizaje: se aplasta y recupera.
        float despues = t - llegada;

        if (despues >= 0.0f && despues < 0.3f)
        {
            pose.escalaVertical = 1.0f - 0.32f * std::sin(despues / 0.3f * PI);
        }

        // Festejo final: dos saltos.
        if (t > 4.6f && !reducir)
        {
            float s = std::sin((t - 4.6f) * 6.0f + i * 0.8f);
            if (s > 0.0f) { y += s * 0.55f; pose.enAire = true; }
        }

        DibujarPersonajeBloque(i, { xs[i], y, 0.0f }, 0.0f, 0.32f, remeras[i], pose);

        // Polvo del aterrizaje.
        if (despues >= 0.0f && despues < 0.45f)
        {
            float r = 0.3f + 1.8f * despues;
            DrawCircle3D({ xs[i], 0.02f, 0.0f }, r, { 1.0f, 0.0f, 0.0f }, 90.0f, Fade(Color{ 240, 225, 200, 255 }, 1.0f - despues / 0.45f));
        }
    }

    if (t > 5.0f)
    {
        DibujarConfeti3D({ 0.0f, 0.0f, 0.0f }, 9.0f, 7.0f, t - 5.0f, 110);
    }

    EndMode3D();

    // Nombres de cada personaje mientras se presentan.
    for (int i = 0; i < CANTIDAD_PERSONAJES_BLOQUE; i++)
    {
        float llegada = LLEGADA0 + ENTRE_LLEGADAS * i;
        float alfa = Tramo(t, llegada, 0.2f) * (1.0f - Tramo(t, 4.4f, 0.4f));

        EtiquetaSobre(camara, { xs[i], 2.45f, 0.0f }, ObtenerNombrePersonajeBloque(i), ColorTexto(ObtenerColorPersonajeBloque(i)), alfa, UI_TEXTO_GRANDE);
        EtiquetaSobre(camara, { xs[i], 2.15f, 0.0f }, ObtenerLemaPersonajeBloque(i), UI_TEXTO_MUNDO, alfa * 0.9f, UI_TEXTO_PEQUENO);
    }

    // Titulo: cae con rebote y tiembla al asentarse.
    float uTitulo = Tramo(t, 5.0f, 0.55f);

    if (uTitulo > 0.0f)
    {
        float escala = reducir ? 1.0f : 2.4f - 1.4f * SalidaElastica(uTitulo);
        float temblor = (reducir || uTitulo >= 1.0f) ? 0.0f : std::sin(t * 70.0f) * Px(4.0f) * (1.0f - uTitulo);
        int token = (int)(96 * escala);

        DibujarTextoMundoContornoUI(
            "JUEGO DE PARTY",
            GetScreenWidth() * 0.5f + temblor,
            GetScreenHeight() * 0.22f,
            token,
            Color{ 255, 214, 90, 255 },
            Tramo(t, 5.0f, 0.15f)
        );

        DibujarTextoMundoContornoUI(
            "¡QUE EMPIECE LA FIESTA!",
            GetScreenWidth() * 0.5f,
            GetScreenHeight() * 0.22f + Px(82.0f),
            UI_TEXTO_GRANDE,
            UI_TEXTO_MUNDO,
            Tramo(t, 5.5f, 0.3f)
        );
    }

    DibujarFranjasCine(1.0f - Tramo(t, 4.8f, 0.6f));
    DibujarAyudaOmitir(Tramo(t, MINIMO_PARA_OMITIR, 0.3f));

    // Entrada desde negro y salida a negro hacia el menu.
    DibujarFundido(1.0f - Tramo(t, 0.0f, 0.5f));
    DibujarFundido(Tramo(t, DURACION - 0.5f, 0.5f));
}


//==================================================
// PRESENTACION DE MINIJUEGO
//==================================================

void CinematicaMinijuego::Inicializar(
    IdMinijuego id,
    const Participante participantes[],
    bool esDuelo
)
{
    ReiniciarCinematica(estado, DURACION);
    minijuego = id;
    duelo = esDuelo;
    cantidad = 0;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        figuras[i] = FiguraCinematica{};

        if (!participantes[i].activo) continue;

        FiguraCinematica& f = figuras[cantidad++];
        f.activo = true;
        f.personaje = participantes[i].personajeSeleccionado >= 0
            ? NormalizarPersonajeBloque(participantes[i].personajeSeleccionado)
            : NormalizarPersonajeBloque(participantes[i].numeroJugador - 1);
        f.numeroJugador = participantes[i].numeroJugador;
        f.esBot = participantes[i].esBot;
        f.color = participantes[i].color;
    }
}


void CinematicaMinijuego::Actualizar(float deltaTime)
{
    AvanzarCinematica(estado, deltaTime);
}


void CinematicaMinijuego::Dibujar() const
{
    const float t = estado.tiempo;
    const bool reducir = ReducirMovimiento();
    const DatosMinijuegoCatalogo& datos = ObtenerDatosMinijuego(minijuego);
    const int ancho = GetScreenWidth();
    const int alto = GetScreenHeight();

    // Fondo: color del minijuego con franjas diagonales en movimiento.
    Color base = datos.color;
    Color oscuro = { (unsigned char)(base.r * 0.30f), (unsigned char)(base.g * 0.30f), (unsigned char)(base.b * 0.30f), 255 };
    Color medio = { (unsigned char)(base.r * 0.45f), (unsigned char)(base.g * 0.45f), (unsigned char)(base.b * 0.45f), 255 };
    ClearBackground(oscuro);

    float desplazamiento = reducir ? 0.0f : std::fmod(t * Px(160.0f), Px(120.0f));

    for (int k = -2; k < ancho / (int)Px(120.0f) + 4; k++)
    {
        DrawRectanglePro(
            Rectangle{ k * Px(120.0f) + desplazamiento, -Px(100.0f), Px(54.0f), alto + Px(400.0f) },
            { 0.0f, 0.0f },
            20.0f,
            medio
        );
    }

    Camera3D camara{};
    // Mira por encima de los personajes: quedan en la mitad baja y el
    // titulo arriba no se superpone ni cuando saltan.
    camara.position = { 0.0f, 2.1f, reducir ? 7.6f : 7.6f - 0.5f * Suave(Tramo(t, 0.0f, 2.4f)) };
    camara.target = { 0.0f, 1.75f, 0.0f };
    camara.up = { 0.0f, 1.0f, 0.0f };
    camara.fovy = 40.0f;
    camara.projection = CAMERA_PERSPECTIVE;

    const float separacion = cantidad <= 2 ? 2.4f : 1.7f;
    Vector3 posiciones[MAX_PARTICIPANTES]{};

    BeginMode3D(camara);

    DrawCylinder({ 0.0f, -0.12f, 0.0f }, 4.0f, 4.0f, 0.12f, 40, Fade(BLACK, 0.35f));

    for (int i = 0; i < cantidad; i++)
    {
        const FiguraCinematica& f = figuras[i];
        float xFinal = ((float)i - 0.5f * (float)(cantidad - 1)) * separacion;

        // Entran deslizandose desde los costados.
        float u = reducir ? 1.0f : Suave(Tramo(t, 0.05f + 0.08f * i, 0.45f));
        float lado = xFinal < 0.0f ? -1.0f : 1.0f;
        float x = xFinal + lado * 6.0f * (1.0f - u);

        PosePersonajeBloque pose;
        pose.tiempo = t + i;
        pose.caminar = u < 1.0f ? 1.0f : 0.0f;
        pose.faseCaminar = t * 12.0f;

        // Golpe de presentacion y salto al grito de "a jugar".
        float golpe = Tramo(t, 1.0f + 0.07f * i, 0.42f);
        if (golpe > 0.0f && golpe < 1.0f) pose.golpe = golpe;

        float y = 0.0f;
        float salto = Tramo(t, 1.95f, 0.5f);
        if (salto > 0.0f && salto < 1.0f && !reducir)
        {
            y = 4.0f * salto * (1.0f - salto) * 0.45f;
            pose.enAire = true;
        }

        // En un duelo se miran; si no, miran a camara.
        float angulo = duelo && cantidad == 2 ? (i == 0 ? 55.0f : -55.0f) : 0.0f;

        posiciones[i] = { x, y, 0.0f };
        DibujarPersonajeBloque(f.personaje, posiciones[i], angulo, 0.27f, f.color, pose);
    }

    EndMode3D();

    // Rotulos de jugador bajo cada personaje.
    for (int i = 0; i < cantidad; i++)
    {
        char rotulo[24];
        std::snprintf(rotulo, sizeof(rotulo), figuras[i].esBot ? "J%d BOT" : "J%d", figuras[i].numeroJugador);
        EtiquetaSobre(camara, { posiciones[i].x, -0.30f, 0.6f }, rotulo, ColorTexto(figuras[i].color), Tramo(t, 0.5f, 0.2f));
    }

    // Titulo del minijuego con golpe de entrada.
    const char* etiqueta = duelo ? "¡DUELO!" : "MINIJUEGO";
    DibujarTextoMundoContornoUI(etiqueta, ancho * 0.5f, alto * 0.13f, UI_TEXTO_GRANDE, duelo ? Color{ 255, 120, 90, 255 } : UI_TEXTO_MUNDO, Tramo(t, 0.15f, 0.2f));

    float uNombre = Tramo(t, 0.4f, 0.35f);

    if (uNombre > 0.0f)
    {
        float escala = reducir ? 1.0f : 2.0f - SalidaElastica(uNombre);
        int token = TokenAjustado(datos.nombre, 64, ancho * 0.9f);
        DibujarTextoMundoContornoUI(datos.nombre, ancho * 0.5f, alto * 0.13f + Px(46.0f), (int)(token * escala), ColorTexto(base), Tramo(t, 0.4f, 0.1f));
    }

    DibujarTextoMundoContornoUI(
        datos.descripcion,
        ancho * 0.5f,
        alto * 0.13f + Px(118.0f),
        TokenAjustado(datos.descripcion, UI_TEXTO, ancho * 0.84f),
        UI_TEXTO_MUNDO,
        Tramo(t, 0.85f, 0.3f)
    );

    // Formato 2 vs 2 cuando corresponde.
    if (datos.requiereParDeJugadores && cantidad == 4)
    {
        DibujarTextoMundoContornoUI("2 VS 2", ancho * 0.5f, alto * 0.13f + Px(150.0f), UI_TEXTO, Color{ 255, 214, 90, 255 }, Tramo(t, 1.0f, 0.3f));
    }

    float uJugar = Tramo(t, 1.9f, 0.3f);

    if (uJugar > 0.0f)
    {
        float escala = reducir ? 1.0f : 1.8f - 0.8f * SalidaElastica(uJugar);
        DibujarTextoMundoContornoUI("¡A JUGAR!", ancho * 0.5f, alto * 0.80f, (int)(72 * escala), Color{ 255, 230, 120, 255 }, uJugar);
    }

    DibujarFranjasCine(Tramo(t, 0.0f, 0.25f));
    DibujarAyudaOmitir(Tramo(t, MINIMO_PARA_OMITIR, 0.3f));

    DibujarFundido(1.0f - Tramo(t, 0.0f, 0.2f));
    // Termina en negro: el juego entra al minijuego con su fundido general.
    DibujarFundido(Tramo(t, DURACION - 0.3f, 0.3f));
}


//==================================================
// CAMPEON
//==================================================

void CinematicaCampeon::Inicializar(
    const int ranking[],
    const int puestos[],
    int cantidadJugadores,
    const Participante participantes[]
)
{
    ReiniciarCinematica(estado, DURACION);
    cantidad = 0;

    for (int k = 0; k < MAX_PARTICIPANTES; k++)
    {
        figuras[k] = FiguraCinematica{};
        puesto[k] = 0;
    }

    for (int k = 0; k < cantidadJugadores && k < MAX_PARTICIPANTES; k++)
    {
        int indice = ranking[k];
        if (indice < 0 || indice >= MAX_PARTICIPANTES) continue;

        const Participante& p = participantes[indice];
        FiguraCinematica& f = figuras[cantidad];
        f.activo = true;
        f.personaje = p.personajeSeleccionado >= 0
            ? NormalizarPersonajeBloque(p.personajeSeleccionado)
            : NormalizarPersonajeBloque(p.numeroJugador - 1);
        f.numeroJugador = p.numeroJugador;
        f.esBot = p.esBot;
        f.color = p.color;
        puesto[cantidad] = puestos[k] > 0 ? puestos[k] : k + 1;
        cantidad++;
    }

    empateEnPrimero = cantidad > 1 && puesto[1] == 1;
}


void CinematicaCampeon::Actualizar(float deltaTime)
{
    AvanzarCinematica(estado, deltaTime);
}


void CinematicaCampeon::Dibujar() const
{
    const float t = estado.tiempo;
    const bool reducir = ReducirMovimiento();

    DibujarCieloNoche(Color{ 20, 24, 60, 255 }, Color{ 120, 70, 90, 255 });

    // Camara: arranca cerca del ganador y se abre orbitando el podio.
    float abrir = reducir ? 1.0f : Suave(Tramo(t, 0.2f, 2.4f));
    float orbita = reducir ? 0.0f : std::sin(t * 0.45f) * 0.35f * abrir;

    Camera3D camara{};
    float distancia = 3.2f + 6.4f * abrir;
    camara.position = { std::sin(orbita) * distancia, 2.7f + 1.0f * abrir, std::cos(orbita) * distancia };
    // Mira alto: la escena queda en la mitad baja y el campeon saltando no
    // se mete detras del titulo.
    camara.target = { 0.0f, 2.4f - 0.1f * abrir, 0.0f };
    camara.up = { 0.0f, 1.0f, 0.0f };
    camara.fovy = 40.0f;
    camara.projection = CAMERA_PERSPECTIVE;

    // Lugar en el podio segun la posicion en la tabla.
    const Vector3 lugares[4] = { { 0.0f, 0.0f, 0.0f }, { -1.7f, 0.0f, 0.2f }, { 1.7f, 0.0f, 0.2f }, { 3.2f, 0.0f, 1.0f } };
    const float alturas[4] = { 1.2f, 0.8f, 0.5f, 0.0f };
    Vector3 cabezas[MAX_PARTICIPANTES]{};

    BeginMode3D(camara);

    DibujarTablado(5.0f);

    for (int k = 0; k < 3 && k < cantidad; k++)
    {
        float h = alturas[puesto[k] - 1 < 3 ? puesto[k] - 1 : 3];
        if (h <= 0.0f) continue;

        Color madera = k == 0 ? Color{ 214, 170, 70, 255 } : Color{ 150, 100, 64, 255 };
        DrawCube({ lugares[k].x, h * 0.5f, lugares[k].z }, 1.4f, h, 1.2f, madera);
        DrawCube({ lugares[k].x, h + 0.02f, lugares[k].z }, 1.46f, 0.05f, 1.26f, Color{ 245, 230, 190, 255 });
        DrawCube({ lugares[k].x, h * 0.55f, lugares[k].z + 0.605f }, 0.5f, 0.38f, 0.02f, Color{ 255, 240, 200, 255 });
    }

    for (int k = 0; k < cantidad; k++)
    {
        const FiguraCinematica& f = figuras[k];
        int p = puesto[k] - 1;
        float h = alturas[p < 3 ? p : 3];

        PosePersonajeBloque pose;
        pose.tiempo = t + k * 0.6f;

        float y = h;

        if (puesto[k] == 1 && t > 0.8f && !reducir)
        {
            // El campeon salta con los brazos arriba.
            float s = std::sin((t - 0.8f) * 5.5f + k);
            if (s > 0.0f) y += s * 0.6f;
            pose.enAire = true;
        }
        else if (puesto[k] > 1 && puesto[k] == puesto[cantidad - 1] && cantidad >= 3)
        {
            // El ultimo queda mareado.
            pose.aturdido = true;
        }
        else if (!reducir)
        {
            // Los demas festejan en el lugar.
            pose.caminar = 0.35f;
            pose.faseCaminar = t * 9.0f + k;
        }

        DibujarPersonajeBloque(f.personaje, { lugares[k].x, y, lugares[k].z }, 0.0f, 0.30f, f.color, pose);
        cabezas[k] = { lugares[k].x, h + 1.95f, lugares[k].z };
    }

    if (t > 0.6f)
    {
        DibujarConfeti3D({ 0.0f, 0.0f, 0.0f }, 9.0f, 8.0f, t - 0.6f, 140);
    }

    EndMode3D();

    // Medallas sobre cada uno (los primeros ya los nombra el titulo).
    for (int k = 0; k < cantidad; k++)
    {
        if (puesto[k] == 1) continue;

        char texto[24];
        std::snprintf(texto, sizeof(texto), "%dº J%d", puesto[k], figuras[k].numeroJugador);
        EtiquetaSobre(camara, cabezas[k], texto, ColorTexto(figuras[k].color), Tramo(t, 2.0f, 0.3f), UI_TEXTO);
    }

    float uTitulo = Tramo(t, 1.0f, 0.5f);

    if (uTitulo > 0.0f && cantidad > 0)
    {
        float escala = reducir ? 1.0f : 2.2f - 1.2f * SalidaElastica(uTitulo);
        DibujarTextoMundoContornoUI(
            empateEnPrimero ? "¡EMPATE EN LA CIMA!" : "¡CAMPEÓN!",
            GetScreenWidth() * 0.5f,
            GetScreenHeight() * 0.13f,
            (int)(84 * escala),
            Color{ 255, 214, 90, 255 },
            Tramo(t, 1.0f, 0.12f)
        );

        char subtitulo[64];
        std::snprintf(subtitulo, sizeof(subtitulo), "J%d · %s", figuras[0].numeroJugador, ObtenerNombrePersonajeBloque(figuras[0].personaje));
        DibujarTextoMundoContornoUI(subtitulo, GetScreenWidth() * 0.5f, GetScreenHeight() * 0.13f + Px(78.0f), UI_TEXTO_GRANDE, ColorTexto(figuras[0].color), Tramo(t, 1.5f, 0.3f));
    }

    DibujarFranjasCine(1.0f);
    DibujarAyudaOmitir(Tramo(t, MINIMO_PARA_OMITIR, 0.3f));

    DibujarFundido(1.0f - Tramo(t, 0.0f, 0.4f));
    DibujarFundido(Tramo(t, DURACION - 0.4f, 0.4f));
}


//==================================================
// PRESENTACION DEL TABLERO
//==================================================
//
// Un solo codigo para los cinco tableros. Cada linea del guion es un plano:
// la camara viaja (TRANSICION) mientras el guia salta a su nuevo lugar, y
// luego se acerca despacio mientras el guia habla. El guia siempre se para
// sobre una casilla (el camino es lo unico seguro de cada mapa: nunca
// queda dentro de un edificio ni en el agua).

static const float ESCALA_GUIA_TABLERO = 0.24f;     // fichas: 0.18
static const float ALTURA_PIES_CASILLA = 0.47f;     // igual que las fichas
static const float LETRAS_POR_SEGUNDO = 42.0f;


static Vector3 Mezcla3(Vector3 a, Vector3 b, float t)
{
    return { a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t };
}


// Direccion horizontal desde el objetivo hacia la camara del tablero.
static Vector3 DireccionVistaTablero(const DefinicionTablero& def)
{
    Vector3 d = { def.camaraPosicion.x - def.camaraObjetivo.x, 0.0f, def.camaraPosicion.z - def.camaraObjetivo.z };
    float largo = std::sqrt(d.x * d.x + d.z * d.z);

    if (largo < 0.001f)
    {
        return { 0.0f, 0.0f, 1.0f };
    }

    return { d.x / largo, 0.0f, d.z / largo };
}


// Posicion de camara mirando "objetivo" desde la direccion de vista girada
// "giro" grados, con "elevacion" grados sobre el horizonte.
static Vector3 PosicionOrbita(
    Vector3 objetivo,
    Vector3 vista,
    float giro,
    float elevacion,
    float distancia
)
{
    float g = giro * DEG2RAD;
    float e = elevacion * DEG2RAD;
    float c = std::cos(g);
    float s = std::sin(g);
    Vector3 dir = { vista.x * c + vista.z * s, 0.0f, vista.z * c - vista.x * s };

    return {
        objetivo.x + dir.x * distancia * std::cos(e),
        objetivo.y + distancia * std::sin(e),
        objetivo.z + dir.z * distancia * std::cos(e)
    };
}


static Vector3 PiesSobreCasilla(const Casilla& casilla)
{
    return { casilla.posicion.x, casilla.posicion.y + ALTURA_PIES_CASILLA, casilla.posicion.z };
}


// Casilla del tipo pedido mas cercana al centro del tablero (encuadre
// comodo, lejos de los bordes del mapa). -1 si no hay.
static int BuscarCasillaTipo(const PartidaTablero& partida, TipoCasilla tipo)
{
    const Vector3 centro = partida.definicion->camaraObjetivo;
    int mejor = -1;
    float mejorDistancia = 1.0e9f;

    for (int i = 1; i < partida.tablero.cantidadCasillas; i++)
    {
        const Casilla& c = partida.tablero.casillas[i];

        if (c.tipo != tipo || i == partida.casillaTrofeo) continue;

        float dx = c.posicion.x - centro.x;
        float dz = c.posicion.z - centro.z;
        float d = dx * dx + dz * dz;

        if (d < mejorDistancia)
        {
            mejorDistancia = d;
            mejor = i;
        }
    }

    return mejor;
}


// Casilla vecina (conectada en cualquier sentido) donde se para el guia:
// la que queda mas a la izquierda en pantalla, para que el guia senale
// hacia la derecha y no tape la casilla.
static int BuscarCasillaVecina(const PartidaTablero& partida, int casilla, Vector3 derecha)
{
    const Casilla* objetivo = partida.tablero.ObtenerCasilla(casilla);

    if (objetivo == nullptr) return -1;

    int mejor = -1;
    float mejorPuntaje = -1.0e9f;

    for (int i = 0; i < partida.tablero.cantidadCasillas; i++)
    {
        const Casilla& c = partida.tablero.casillas[i];

        if (i == casilla) continue;

        bool vecina = false;

        for (int k = 0; k < c.cantidadConexiones; k++)
        {
            vecina = vecina || c.conexiones[k].destino == casilla;
        }

        for (int k = 0; k < objetivo->cantidadConexiones; k++)
        {
            vecina = vecina || objetivo->conexiones[k].destino == i;
        }

        if (!vecina) continue;

        float dx = c.posicion.x - objetivo->posicion.x;
        float dz = c.posicion.z - objetivo->posicion.z;
        float puntaje = -(dx * derecha.x + dz * derecha.z);

        // Evita la casilla del trofeo y la salida (ya hay algo encima).
        if (i == partida.casillaTrofeo || i == 0) puntaje -= 100.0f;

        if (puntaje > mejorPuntaje)
        {
            mejorPuntaje = puntaje;
            mejor = i;
        }
    }

    return mejor;
}


static float AnguloHacia(Vector3 desde, Vector3 hacia)
{
    return std::atan2(hacia.x - desde.x, hacia.z - desde.z) * RAD2DEG;
}


// Duracion de un plano segun el largo de su frase (lectura comoda).
static float DuracionPlanoTablero(const char* texto)
{
    float largo = texto != nullptr ? (float)std::strlen(texto) : 0.0f;
    float d = 0.8f + largo * 0.03f;
    return d < 2.2f ? 2.2f : (d > 3.1f ? 3.1f : d);
}


// Segundos desde que empieza a escribirse la frase del plano.
static float InicioFrasePlano(int indice)
{
    return indice > 0 ? CinematicaTablero::TRANSICION * 0.5f : 0.3f;
}


void CinematicaTablero::Inicializar(PartidaTablero& partidaTablero, int cantidadRondas)
{
    partida = &partidaTablero;
    rondas = cantidadRondas;
    guion = &ObtenerGuionCinematicaTablero(partidaTablero.idTablero);
    cantidad = 0;

    const DefinicionTablero& def = *partidaTablero.definicion;
    const Vector3 vista = DireccionVistaTablero(def);
    const Vector3 derecha = { vista.z, 0.0f, -vista.x };
    const Vector3 desfase = Vector3Subtract(def.camaraPosicion, def.camaraObjetivo);
    const float largoDesfase = Vector3Length(desfase) > 0.001f ? Vector3Length(desfase) : 1.0f;
    const float elevacionTablero = std::asin(desfase.y / largoDesfase) * RAD2DEG;
    const float zoomGeneral = def.camaraZoomGeneral > 1.0f ? def.camaraZoomGeneral : 1.6f;

    // Final: la vista general con la que empieza la primera ronda.
    const Vector3 posicionGeneral = Vector3Add(def.camaraObjetivo, Vector3Scale(desfase, zoomGeneral));

    // El guia empieza junto a la salida (en la salida estan las fichas).
    int casillaGuia = BuscarCasillaVecina(partidaTablero, 0, derecha);
    const Casilla* inicioGuia = partidaTablero.tablero.ObtenerCasilla(casillaGuia >= 0 ? casillaGuia : 0);
    Vector3 guia = inicioGuia != nullptr ? PiesSobreCasilla(*inicioGuia) : Vector3{};
    float anguloGuia = AnguloHacia(guia, Vector3Add(guia, vista));

    float inicio = 0.0f;

    for (int i = 0; i < MAX_LINEAS_GUION_TABLERO && guion->lineas[i].texto != nullptr; i++)
    {
        const LineaGuionTablero& linea = guion->lineas[i];
        PlanoCinematicaTablero& p = planos[cantidad];
        p = PlanoCinematicaTablero{};
        p.inicio = inicio;
        p.duracion = DuracionPlanoTablero(linea.texto);

        // Cada plano gira hacia un lado distinto: no hay dos encuadres
        // iguales seguidos.
        const float lado = (cantidad % 2 == 0) ? 1.0f : -1.0f;

        int casillaFoco = -1;

        if (linea.plano == PLANO_CASILLA)
        {
            casillaFoco = BuscarCasillaTipo(partidaTablero, linea.casilla);
        }
        else if (linea.plano == PLANO_TROFEO)
        {
            casillaFoco = partidaTablero.casillaTrofeo;
        }

        const Casilla* foco = partidaTablero.tablero.ObtenerCasilla(casillaFoco);
        bool resuelto = false;

        if ((linea.plano == PLANO_CASILLA || linea.plano == PLANO_TROFEO) && foco != nullptr)
        {
            int vecina = BuscarCasillaVecina(partidaTablero, casillaFoco, derecha);
            const Casilla* parada = partidaTablero.tablero.ObtenerCasilla(vecina);

            if (parada != nullptr)
            {
                guia = PiesSobreCasilla(*parada);
            }

            Vector3 centroCasilla = PiesSobreCasilla(*foco);
            Vector3 haciaCasilla = Vector3Normalize(Vector3Subtract(centroCasilla, guia));

            // La camara mira de costado a la linea guia-casilla (del lado
            // de la vista de juego): guia y casilla quedan lado a lado y
            // ninguno tapa al otro.
            Vector3 vistaPlano = { haciaCasilla.z, 0.0f, -haciaCasilla.x };

            if (vistaPlano.x * vista.x + vistaPlano.z * vista.z < 0.0f)
            {
                vistaPlano = Vector3Scale(vistaPlano, -1.0f);
            }

            vistaPlano = Vector3Normalize(Vector3Add(Vector3Scale(vistaPlano, 0.75f), Vector3Scale(vista, 0.25f)));

            // Mira a la casilla, girado hacia la camara para verle la cara.
            Vector3 mirada = Vector3Add(Vector3Scale(haciaCasilla, 0.65f), Vector3Scale(vistaPlano, 0.55f));
            anguloGuia = AnguloHacia(guia, Vector3Add(guia, mirada));

            Vector3 medio = Mezcla3(guia, centroCasilla, 0.5f);
            medio.y += 0.45f;

            float separacion = Vector3Distance(guia, centroCasilla);
            float distancia = 4.6f + separacion * 0.8f;

            p.objetivoInicio = medio;
            p.objetivoFin = medio;
            p.posicionInicio = PosicionOrbita(medio, vistaPlano, 7.0f * lado, 30.0f, distancia + 0.6f);
            p.posicionFin = PosicionOrbita(medio, vistaPlano, 2.0f * lado, 26.0f, distancia);
            p.casilla = casillaFoco;
            p.levantaCasilla = linea.plano == PLANO_CASILLA;
            resuelto = true;
        }
        else if (linea.plano == PLANO_LANDMARK && def.tieneLandmark)
        {
            p.objetivoInicio = def.camaraLandmark;
            p.objetivoFin = def.camaraLandmark;
            p.posicionInicio = PosicionOrbita(def.camaraLandmark, vista, -12.0f * lado, 36.0f, 12.5f);
            p.posicionFin = PosicionOrbita(def.camaraLandmark, vista, 4.0f * lado, 32.0f, 11.0f);
            resuelto = true;
        }
        else if (linea.plano == PLANO_GUIA || linea.plano == PLANO_DESPEDIDA)
        {
            if (linea.plano == PLANO_DESPEDIDA && inicioGuia != nullptr)
            {
                guia = PiesSobreCasilla(*inicioGuia);
            }

            anguloGuia = AnguloHacia(guia, Vector3Add(guia, vista));

            Vector3 cara = { guia.x, guia.y + 0.75f, guia.z };

            p.objetivoInicio = cara;
            p.posicionInicio = PosicionOrbita(cara, vista, 10.0f * lado, 16.0f, 4.2f);

            if (linea.plano == PLANO_DESPEDIDA)
            {
                // Se abre hasta la vista general con la que empieza la ronda.
                p.objetivoFin = def.camaraObjetivo;
                p.posicionFin = posicionGeneral;
            }
            else
            {
                p.objetivoFin = cara;
                p.posicionFin = PosicionOrbita(cara, vista, 3.0f * lado, 12.0f, 3.5f);
            }

            resuelto = true;
        }

        if (!resuelto)
        {
            // Panorama (o una casilla/landmark que este tablero no tiene).
            float distancia = largoDesfase * zoomGeneral * 0.92f;

            p.objetivoInicio = def.camaraObjetivo;
            p.objetivoFin = def.camaraObjetivo;
            p.posicionInicio = PosicionOrbita(def.camaraObjetivo, vista, -16.0f, elevacionTablero + 6.0f, distancia * 1.08f);
            p.posicionFin = PosicionOrbita(def.camaraObjetivo, vista, 2.0f, elevacionTablero, distancia);
        }

        p.guia = guia;
        p.anguloGuia = anguloGuia;

        inicio += p.duracion;
        cantidad++;
    }

    ReiniciarCinematica(estado, inicio + FUNDIDO_FINAL);
}


void CinematicaTablero::Actualizar(float deltaTime)
{
    AvanzarCinematica(estado, deltaTime);

    // Solo el reloj visual del tablero (agua, luces, gimmick animado).
    if (partida != nullptr && !estado.terminada)
    {
        partida->tiempoTablero += deltaTime;
    }
}


int CinematicaTablero::PlanoActual() const
{
    int actual = 0;

    for (int i = 0; i < cantidad; i++)
    {
        if (estado.tiempo >= planos[i].inicio)
        {
            actual = i;
        }
    }

    return actual;
}


// Globo de dialogo al pie con el nombre del guia y la frase escrita letra
// por letra. La cola apunta a la cabeza del guia si esta en pantalla.
static void DibujarGloboDialogo(
    const char* nombre,
    Color colorGuia,
    const char* texto,
    float tiempoLinea,
    bool conCola,
    Vector2 cabeza,
    float alfa
)
{
    if (alfa <= 0.0f || texto == nullptr) return;

    const Rectangle globo = RectRef(250.0f, 532.0f, 780.0f, 84.0f);
    const float radio = 0.35f;
    const Color papel = Color{ 255, 248, 232, 255 };

    // Entrada: el globo crece desde abajo.
    float u = Suave(Tramo(tiempoLinea, 0.0f, 0.18f));
    float escala = 0.9f + 0.1f * u;
    Rectangle r = {
        globo.x + globo.width * (1.0f - escala) * 0.5f,
        globo.y + globo.height * (1.0f - escala),
        globo.width * escala,
        globo.height * escala
    };

    float a = alfa * (0.4f + 0.6f * u);

    if (conCola && cabeza.y < r.y - Px(30.0f))
    {
        float minimo = r.x + Px(60.0f);
        float maximo = r.x + r.width - Px(60.0f);
        float base = cabeza.x < minimo ? minimo : (cabeza.x > maximo ? maximo : cabeza.x);

        // Piquito corto que apunta hacia la cabeza (nunca la atraviesa).
        Vector2 hacia = { cabeza.x - base, cabeza.y - r.y };
        float largo = std::sqrt(hacia.x * hacia.x + hacia.y * hacia.y);
        float corto = largo > 1.0f ? Px(34.0f) / largo : 0.0f;
        Vector2 punta = { base + hacia.x * corto, r.y + hacia.y * corto };

        // DrawTriangle necesita los vertices en sentido antihorario.
        DrawTriangle({ base + Px(18.0f), r.y + Px(4.0f) }, punta, { base - Px(18.0f), r.y + Px(4.0f) }, Fade(UI_TINTA, a));
        DrawTriangle({ base + Px(12.0f), r.y + Px(6.0f) }, { punta.x, punta.y + Px(7.0f) }, { base - Px(12.0f), r.y + Px(6.0f) }, Fade(papel, a));
    }

    DrawRectangleRounded({ r.x + Px(4.0f), r.y + Px(6.0f), r.width, r.height }, radio, 8, Fade(BLACK, 0.30f * a));
    DrawRectangleRounded({ r.x - Px(3.0f), r.y - Px(3.0f), r.width + Px(6.0f), r.height + Px(6.0f) }, radio, 8, Fade(UI_TINTA, a));
    DrawRectangleRounded(r, radio, 8, Fade(papel, a));

    // Letra por letra (un caracter UTF-8 nunca se corta a la mitad).
    char visible[160];
    int total = (int)std::strlen(texto);
    int mostrar = tiempoLinea > 0.0f ? (int)(tiempoLinea * LETRAS_POR_SEGUNDO) : 0;

    if (mostrar > total) mostrar = total;
    while (mostrar < total && ((unsigned char)texto[mostrar] & 0xC0) == 0x80) mostrar++;
    if (mostrar > (int)sizeof(visible) - 1) mostrar = (int)sizeof(visible) - 1;

    std::memcpy(visible, texto, (size_t)mostrar);
    visible[mostrar] = 0;

    // Se mide con la frase completa para centrar sin saltos de linea.
    const float anchoTexto = r.width - Px(56.0f);
    const float altoTexto = r.height - Px(22.0f);
    MedidaParrafoUI medida = MedirParrafoUI(texto, UI_TEXTO_GRANDE, anchoTexto, altoTexto);
    float y = r.y + (r.height - medida.alto) * 0.5f + Px(4.0f);

    DibujarParrafoUI(visible, r.x + Px(28.0f), y, medida.tamano > 0 ? UI_TEXTO_GRANDE : UI_TEXTO, anchoTexto, Fade(UI_TINTA, a), UI_ALINEAR_IZQUIERDA, altoTexto);

    // Placa con el nombre del guia, inclinada sobre el borde del globo.
    OpcionesEtiquetaUI etiqueta;
    etiqueta.borde = colorGuia;
    etiqueta.alfa = a;
    etiqueta.tokenTexto = UI_TEXTO;

    DibujarEtiquetaUI(nombre, { r.x + Px(90.0f), r.y - Px(4.0f) }, -3.0f, etiqueta);
}


void CinematicaTablero::Dibujar() const
{
    if (partida == nullptr || partida->definicion == nullptr || guion == nullptr || cantidad <= 0)
    {
        ClearBackground(BLACK);
        return;
    }

    const PartidaTablero& p = *partida;
    const DefinicionTablero& def = *p.definicion;
    const float t = estado.tiempo;
    const bool reducir = ReducirMovimiento();
    const int indice = PlanoActual();
    const PlanoCinematicaTablero& plano = planos[indice];
    const PlanoCinematicaTablero& anterior = planos[indice > 0 ? indice - 1 : 0];
    const LineaGuionTablero& linea = guion->lineas[indice];
    const float local = t - plano.inicio;
    const float total = estado.duracion - FUNDIDO_FINAL;

    // Camara: viaje desde el final del plano anterior y luego acercamiento.
    float viaje = indice > 0 ? Suave(Tramo(local, 0.0f, TRANSICION)) : 1.0f;
    float acercar = Suave(Tramo(local, indice > 0 ? TRANSICION * 0.6f : 0.0f, plano.duracion));

    if (linea.plano == PLANO_DESPEDIDA)
    {
        // La apertura empieza tras el festejo y termina en la vista de juego.
        acercar = Suave(Tramo(local, TRANSICION + 0.6f, plano.duracion - 0.4f));
    }

    if (reducir)
    {
        viaje = 1.0f;
        acercar = linea.plano == PLANO_DESPEDIDA ? acercar : 0.0f;
    }

    Vector3 objetivo = Mezcla3(plano.objetivoInicio, plano.objetivoFin, acercar);
    Vector3 posicion = Mezcla3(plano.posicionInicio, plano.posicionFin, acercar);

    if (viaje < 1.0f)
    {
        objetivo = Mezcla3(anterior.objetivoFin, objetivo, viaje);
        posicion = Mezcla3(anterior.posicionFin, posicion, viaje);
    }

    Camera3D camara{};
    camara.position = posicion;
    camara.target = objetivo;
    camara.up = { 0.0f, 1.0f, 0.0f };
    camara.fovy = def.camaraFovy;
    camara.projection = CAMERA_PERSPECTIVE;

    // Guia: salta del lugar anterior al nuevo durante el viaje.
    Vector3 pies = plano.guia;
    float angulo = plano.anguloGuia;
    bool enAire = false;

    if (viaje < 1.0f && Vector3Distance(anterior.guia, plano.guia) > 0.05f)
    {
        float distancia = Vector3Distance(anterior.guia, plano.guia);
        float altura = 0.6f + distancia * 0.12f;

        if (altura > 3.0f) altura = 3.0f;

        pies = Mezcla3(anterior.guia, plano.guia, viaje);
        pies.y += std::sin(viaje * PI) * altura;
        angulo = AnguloHacia(anterior.guia, plano.guia);
        enAire = true;
    }

    const float tiempoLinea = local - InicioFrasePlano(indice);
    const int largoTexto = linea.texto != nullptr ? (int)std::strlen(linea.texto) : 0;
    const bool hablando = tiempoLinea > 0.0f && tiempoLinea * LETRAS_POR_SEGUNDO < (float)largoTexto;

    PosePersonajeBloque pose;
    pose.tiempo = t;
    pose.enAire = enAire;
    pose.hablando = hablando && !enAire;
    pose.gesto = linea.gesto;

    if (linea.gesto == GESTO_NINGUNO && linea.plano == PLANO_CASILLA)
    {
        pose.gesto = GESTO_SENALAR;
    }

    pose.pesoGesto = enAire ? 0.0f : Suave(Tramo(local, indice > 0 ? TRANSICION : 0.2f, 0.25f));

    if (linea.gesto == GESTO_MANOS_CABEZA && !enAire)
    {
        pose.susto = pose.pesoGesto;
        pose.giroCabeza = 0.35f * std::sin(t * 5.0f) * pose.pesoGesto;
    }

    // Festejo final: saltitos en el lugar.
    if (linea.plano == PLANO_DESPEDIDA && !enAire && !reducir && local > TRANSICION)
    {
        float s = std::sin((local - TRANSICION) * 7.0f);

        if (s > 0.0f)
        {
            pies.y += s * 0.35f;
            pose.enAire = true;
        }
    }

    // Aterrizaje: se aplasta un instante.
    if (indice > 0 && !reducir)
    {
        float despues = local - TRANSICION;

        if (despues >= 0.0f && despues < 0.22f)
        {
            pose.escalaVertical = 1.0f - 0.25f * std::sin(despues / 0.22f * PI);
        }
    }

    //------------------------------
    // ESCENA 3D
    //------------------------------

    ClearBackground(def.colorFondo);

    BeginMode3D(camara);

    DibujarEscenaTablero3D(p);

    // Casilla presentada: asoma de su lugar con un aro de su color.
    const Casilla* casilla = p.tablero.ObtenerCasilla(plano.casilla);

    if (casilla != nullptr && viaje >= 1.0f)
    {
        float u = local - (indice > 0 ? TRANSICION : 0.0f);
        Color color = ObtenerColorCasilla(casilla->tipo);

        if (plano.levantaCasilla)
        {
            float salto = 0.08f;

            if (!reducir)
            {
                salto += 0.16f * std::sin(Tramo(u, 0.0f, 0.45f) * PI) + 0.03f * std::sin(t * 4.0f);
            }

            Casilla copia = *casilla;
            copia.posicion.y += salto;
            DibujarCasillaTematica(copia, def.estiloCasilla, p.tiempoTablero);
        }

        float aro = Tramo(u, 0.0f, 0.5f);
        float pulso = reducir ? 0.5f : 0.5f + 0.5f * std::sin(t * 6.0f);
        Vector3 centro = { casilla->posicion.x, casilla->posicion.y + 0.27f, casilla->posicion.z };

        rlDrawRenderBatchActive();
        rlDisableDepthMask();
        BeginBlendMode(BLEND_ADDITIVE);
        DrawCylinderEx(centro, { centro.x, centro.y + 0.9f + 0.2f * pulso, centro.z }, 1.05f * aro, 0.75f * aro, 20, Fade(color, 0.20f));
        DrawCircle3D({ centro.x, centro.y + 0.02f, centro.z }, 1.0f + 0.25f * pulso, { 1.0f, 0.0f, 0.0f }, 90.0f, Fade(color, 0.9f * aro));
        EndBlendMode();
        rlDrawRenderBatchActive();
        rlEnableDepthMask();
    }

    // Sombra del guia (mas chica cuanto mas alto salta) y el guia.
    {
        Vector3 suelo = viaje < 1.0f ? Mezcla3(anterior.guia, plano.guia, viaje) : plano.guia;
        float alturaSalto = pies.y - suelo.y;

        if (alturaSalto < 0.0f) alturaSalto = 0.0f;

        float r = 0.32f / (1.0f + alturaSalto);

        DrawCylinderEx({ suelo.x, suelo.y + 0.005f, suelo.z }, { suelo.x, suelo.y + 0.01f, suelo.z }, r, r, 12, Color{ 20, 20, 30, 110 });
    }

    DibujarPersonajeBloque(guion->guia, pies, angulo, ESCALA_GUIA_TABLERO, ObtenerColorPersonajeBloque(guion->guia), pose);

    EndMode3D();

    if (def.DibujarCapaPantalla != nullptr)
    {
        def.DibujarCapaPantalla(p);
    }

    //------------------------------
    // CAPA 2D
    //------------------------------

    DibujarFranjasCine(Tramo(t, 0.0f, 0.5f) * (1.0f - Tramo(t, total - 0.6f, 0.6f)));

    // Nombre del tablero sobre el mundo durante el panorama.
    if (linea.plano == PLANO_PANORAMA)
    {
        float aparece = reducir ? 1.0f : Tramo(local, 0.2f, 0.4f);
        float alfa = aparece * (1.0f - Tramo(local, plano.duracion - 0.35f, 0.35f));
        float bajada = reducir ? 0.0f : (1.0f - Suave(aparece)) * Px(-20.0f);
        float y = (float)GetScreenHeight() * 0.15f + bajada;

        DibujarTextoMundoContornoUI(def.nombre, GetScreenWidth() * 0.5f, y, UI_TEXTO_TITULO, UI_TEXTO_MUNDO, alfa);

        OpcionesEtiquetaUI etiqueta;
        etiqueta.borde = def.colorTema;
        etiqueta.alfa = alfa;

        DibujarEtiquetaUI(TextFormat("%d RONDAS", rondas), { GetScreenWidth() * 0.5f, y + Px(84.0f) }, 2.0f, etiqueta);
    }

    Vector2 cabeza = GetWorldToScreen({ pies.x, pies.y + 1.55f, pies.z }, camara);
    bool guiaEnPantalla =
        linea.plano != PLANO_PANORAMA && linea.plano != PLANO_LANDMARK &&
        cabeza.x > 0.0f && cabeza.x < (float)GetScreenWidth() && cabeza.y > 0.0f;

    float salida = indice == cantidad - 1
        ? Tramo(t, total - 0.5f, 0.4f)
        : Tramo(local, plano.duracion - 0.12f, 0.12f);

    DibujarGloboDialogo(
        ObtenerNombrePersonajeBloque(guion->guia),
        ObtenerColorPersonajeBloque(guion->guia),
        linea.texto,
        tiempoLinea,
        guiaEnPantalla,
        cabeza,
        Tramo(tiempoLinea, 0.0f, 0.12f) * (1.0f - salida)
    );

    DibujarAyudaOmitir(Tramo(t, MINIMO_PARA_OMITIR, 0.3f) * (1.0f - Tramo(t, total - 0.6f, 0.4f)));

    DibujarFundido(1.0f - Tramo(t, 0.0f, 0.45f));
    DibujarFundido(Tramo(t, total, FUNDIDO_FINAL));
}
