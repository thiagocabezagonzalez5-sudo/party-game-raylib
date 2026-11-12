#include "Minigames/HudMinijuego.h"

#include "Minigames/EfectosVisualesMinijuegos.h"
#include "Systems/Input.h"
#include "UI/EstiloUI.h"
#include "UI/EstiloUIGlobal.h"
#include "rlgl.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>


//==================================================
// MEDIDAS (unidades de referencia a 1280x720)
//==================================================

static const float ANCHO_ETIQUETA = 164.0f;
static const float ALTO_ETIQUETA = 62.0f;
static const float SEPARACION_ETIQUETAS = 10.0f;
static const float ANCHO_RELOJ = 128.0f;
static const float ALTO_RELOJ = 76.0f;
static const float SEPARACION_RELOJ = 14.0f;

static const float Y_SOGA = 14.0f;              // altura de la soga en los extremos
static const float COMBA_SOGA = 6.0f;           // cuanto baja en el centro
static const float SOGA_SOBRANTE = 30.0f;       // soga a cada lado de la fila

// El cartel de presentacion va plantado abajo y al centro: angosto para
// dejar libres las esquinas de abajo (donde arrancan los escaladores de
// la Cantera) y bajo la fila de personajes de Nucleos e Isla.
static const float ANCHO_PRESENTACION = 372.0f;
static const float MARGEN_INFERIOR_PRESENTACION = 60.0f;  // sobre la barra de ayuda
static const float ANCHO_TABLON_FINAL = 540.0f;
static const float Y_CARTELES = 150.0f;         // tablon final

static const int TOKEN_ANUNCIO = 128;           // «3», «2», «1», «¡YA!»
static const int TOKEN_ANUNCIO_FIN = 96;        // «¡TIEMPO!», «¡FIN!»
static const int TOKEN_AYUDA = 14;

// Tiempos (segundos).
static const float T_YA = 0.9f;
static const float T_ANUNCIO_FIN = 1.4f;
static const float T_TABLON_FINAL = 1.2f;
static const float T_SALTO_PUNTAJE = 0.28f;
static const float T_DESTELLO_PUNTAJE = 0.6f;
static const float T_RETIRO_PRESENTACION = 0.35f; // cuenta restante al empezar a bajar

// Colores locales (sobre placa clara; contraste >= 4.5:1 con la placa).
static const Color COLOR_SELLO = { 150, 36, 30, 255 };          // «FUERA»
static const Color COLOR_TINTA_ALERTA = { 160, 30, 24, 255 };    // reloj en rojo
static const Color COLOR_PLACA_ALERTA = { 255, 190, 170, 255 };
static const Color COLOR_CONTORNO_ANUNCIO = { 46, 28, 18, 255 };
static const Color COLOR_GRIS_FUERA = { 128, 128, 132, 255 };


//==================================================
// ESTADO DE ANIMACION
//==================================================
//
// Solo se dibuja un minijuego a la vez, asi que el HUD guarda aqui su
// estado de animacion (el Dibujar de los minijuegos es const y el HUD no
// debe tocar su logica). Se reinicia al cambiar de minijuego o al volver
// a la cuenta regresiva (tecla R).

struct EstadoHudMinijuego
{
    bool iniciado = false;
    const char* titulo = nullptr;

    float reloj = 0.0f;                 // EstiloUI::tiempo del ultimo dibujo
    FaseHudMinijuego fase = HUD_MINIJUEGO_CUENTA;
    float tiempoFase = 0.0f;
    float cuentaPrevia = 0.0f;

    int numeroCuenta = -1;
    float tiempoNumero = 0.0f;

    int valorPrevio[MAX_PARTICIPANTES] = {};
    float tiempoCambio[MAX_PARTICIPANTES] = {};
    bool eliminadoPrevio[MAX_PARTICIPANTES] = {};
    float tiempoEliminado[MAX_PARTICIPANTES] = {};

    int segundoPrevio = -1;
    ColgadoUI pendulo;                  // sacudida del reloj en la alerta

    ResorteUI caidaPresentacion;
    ResorteUI caidaTablon;
};


static EstadoHudMinijuego estadoHud;

// Ver ReiniciarMarcaHudMinijuego / HudMinijuegoDibujado.
static bool hudDibujadoEsteFrame = false;


static float Limitar01(float v)
{
    return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
}


static void ReiniciarEstadoHud(const DatosHudMinijuego& datos, bool reducir)
{
    float reloj = estadoHud.reloj;

    estadoHud = EstadoHudMinijuego{};
    estadoHud.iniciado = true;
    estadoHud.titulo = datos.titulo;
    estadoHud.reloj = reloj;
    estadoHud.fase = datos.fase;
    estadoHud.cuentaPrevia = datos.cuentaRestante;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        estadoHud.valorPrevio[i] = datos.jugadores[i].valorNumerico;
        estadoHud.tiempoCambio[i] = 99.0f;
        estadoHud.eliminadoPrevio[i] = datos.jugadores[i].eliminado;
        estadoHud.tiempoEliminado[i] = 99.0f;
    }

    estadoHud.pendulo.Iniciar(0.0f, 0.0f, 0.0f, 0.35f);
    estadoHud.pendulo.aterrizo = true;

    estadoHud.caidaPresentacion.valor = reducir ? 0.0f : -120.0f;
    estadoHud.caidaTablon.valor = reducir ? 0.0f : -260.0f;
}


static float ActualizarEstadoHud(const DatosHudMinijuego& datos, const EstiloUI& estilo)
{
    // Reloj del EstiloUI compartido (Juego lo avanza cada frame).
    float dt = estilo.tiempo - estadoHud.reloj;
    estadoHud.reloj = estilo.tiempo;

    if (!(dt > 0.0f && dt < 0.25f))
    {
        dt = GetFrameTime();
        if (dt > 0.1f) dt = 0.1f;
    }

    bool reinicio =
        !estadoHud.iniciado ||
        datos.titulo != estadoHud.titulo ||
        (datos.fase == HUD_MINIJUEGO_CUENTA && estadoHud.fase != HUD_MINIJUEGO_CUENTA) ||
        (datos.fase == HUD_MINIJUEGO_CUENTA && datos.cuentaRestante > estadoHud.cuentaPrevia + 0.5f);

    if (reinicio)
    {
        ReiniciarEstadoHud(datos, estilo.reducirMovimiento);
    }

    estadoHud.cuentaPrevia = datos.cuentaRestante;

    if (datos.fase != estadoHud.fase)
    {
        estadoHud.fase = datos.fase;
        estadoHud.tiempoFase = 0.0f;
    }
    else
    {
        estadoHud.tiempoFase += dt;
    }

    int numero = (int)std::ceil(datos.cuentaRestante);
    if (numero < 1) numero = 1;

    if (numero != estadoHud.numeroCuenta)
    {
        estadoHud.numeroCuenta = numero;
        estadoHud.tiempoNumero = 0.0f;
    }
    else
    {
        estadoHud.tiempoNumero += dt;
    }

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        const JugadorHudMinijuego& jugador = datos.jugadores[i];

        if (jugador.valorNumerico != estadoHud.valorPrevio[i])
        {
            estadoHud.valorPrevio[i] = jugador.valorNumerico;
            estadoHud.tiempoCambio[i] = 0.0f;
        }
        else
        {
            estadoHud.tiempoCambio[i] += dt;
        }

        if (jugador.eliminado && !estadoHud.eliminadoPrevio[i])
        {
            estadoHud.tiempoEliminado[i] = 0.0f;
        }
        else
        {
            estadoHud.tiempoEliminado[i] += dt;
        }

        estadoHud.eliminadoPrevio[i] = jugador.eliminado;
    }

    // Reloj en alerta: una sacudida corta por cada segundo que pasa.
    bool sacudida = false;
    int segundo = (int)std::ceil(datos.tiempoRestante);

    if (
        datos.fase == HUD_MINIJUEGO_JUGANDO &&
        datos.reloj == HUD_RELOJ_CUENTA_ATRAS &&
        datos.tiempoRestante > 0.0f &&
        datos.tiempoRestante <= datos.segundosAlerta &&
        segundo != estadoHud.segundoPrevio
    )
    {
        sacudida = true;
    }

    estadoHud.segundoPrevio = segundo;
    estadoHud.pendulo.Actualizar(estilo, dt, sacudida);

    // Caidas con rebote (sin rebote ni caida con reducirMovimiento).
    if (estilo.reducirMovimiento)
    {
        estadoHud.caidaPresentacion = ResorteUI{};
        estadoHud.caidaTablon = ResorteUI{};
    }
    else
    {
        estadoHud.caidaPresentacion.Actualizar(0.0f, dt, 90.0f, 9.0f);

        if (datos.fase == HUD_MINIJUEGO_FIN && estadoHud.tiempoFase >= T_TABLON_FINAL)
        {
            estadoHud.caidaTablon.Actualizar(0.0f, dt, 90.0f, 9.0f);
        }
    }

    return dt;
}


//==================================================
// LIENZO: unidades de referencia ancladas arriba y al centro
//==================================================

static float OrigenX()
{
    return ((float)GetScreenWidth() - UI_REF_ANCHO * EscalaUI()) * 0.5f;
}


static float X(float x)
{
    return OrigenX() + x * EscalaUI();
}


static float Y(float y)
{
    return y * EscalaUI();
}


// Altura de la soga (unidades ref) en x, con una comba suave.
static float AlturaSoga(float x, float inicio, float fin)
{
    float centro = (inicio + fin) * 0.5f;
    float medio = (fin - inicio) * 0.5f;

    if (medio <= 0.0f)
    {
        return Y_SOGA;
    }

    float u = (x - centro) / medio;

    return Y_SOGA + COMBA_SOGA * (1.0f - u * u);
}


//==================================================
// PIEZAS DE DIBUJO
//==================================================

// Anuncio grande sobre el mundo: contorno oscuro y sombra dura gruesa
// (para leerse sobre cielo claro, nieve o lava) y la cara con
// DibujarTextoMundo. cy es el centro vertical en pixeles.
static void DibujarAnuncio(const char* texto, float cy, float tokenEscalado, Color color, float alfa)
{
    if (alfa <= 0.01f)
    {
        return;
    }

    int token = (int)(tokenEscalado + 0.5f);
    int tamano = TamanoTexto(token);
    int ancho = MeasureText(texto, tamano);
    float cx = (float)GetScreenWidth() * 0.5f;
    float x = cx - ancho * 0.5f;
    float y = cy - tamano * 0.5f;
    float grosor = (float)tamano / 22.0f;
    if (grosor < 2.0f) grosor = 2.0f;

    DrawText(texto, (int)(x + grosor * 2.2f), (int)(y + grosor * 2.6f), tamano, Fade(BLACK, 0.55f * alfa));

    for (int dx = -1; dx <= 1; dx++)
    {
        for (int dy = -1; dy <= 1; dy++)
        {
            if (dx == 0 && dy == 0) continue;
            DrawText(texto, (int)(x + dx * grosor), (int)(y + dy * grosor), tamano, Fade(COLOR_CONTORNO_ANUNCIO, alfa));
        }
    }

    DibujarTextoMundo(texto, cx, y, token, Fade(color, alfa));
}


// Parte el texto en lineas de hasta anchoMax, palabra por palabra. Lo
// que no entra en la ultima linea queda en ella (DibujarTextoAjustadoUI
// la achica). Devuelve la cantidad de lineas (1 a 3).
static int PartirPorAncho(const char* texto, int token, float anchoMax, char lineas[3][128])
{
    int tamano = TamanoTexto(token);
    int cantidad = 0;
    const char* resto = texto;

    while (*resto == ' ') resto++;

    while (*resto != '\0' && cantidad < 3)
    {
        char* linea = lineas[cantidad];
        std::snprintf(linea, 128, "%s", resto);
        int largo = (int)std::strlen(linea);
        int corte = largo;

        if (cantidad < 2 && (float)MeasureText(linea, tamano) > anchoMax)
        {
            // Ultimo espacio con el que la linea entra.
            for (int i = largo - 1; i > 0; i--)
            {
                if (linea[i] != ' ') continue;
                linea[i] = '\0';
                bool entra = (float)MeasureText(linea, tamano) <= anchoMax;
                linea[i] = ' ';
                if (entra) { corte = i; break; }
            }
        }

        linea[corte] = '\0';
        resto += corte;
        while (*resto == ' ') resto++;
        cantidad++;
    }

    return cantidad;
}


// Como PartirPorAncho, pero con lineas parejas: busca el ancho mas chico
// que da la misma cantidad de lineas (sin una palabra sola al final).
static int PartirEnTresLineas(const char* texto, int token, float anchoMax, char lineas[3][128])
{
    int cantidad = PartirPorAncho(texto, token, anchoMax, lineas);
    int tamano = TamanoTexto(token);
    float ancho = anchoMax;

    for (float prueba = anchoMax - Px(8.0f); cantidad > 1 && prueba > anchoMax * 0.5f; prueba -= Px(8.0f))
    {
        char intento[3][128] = {};
        int lineasIntento = PartirPorAncho(texto, token, prueba, intento);

        if (lineasIntento != cantidad || (float)MeasureText(intento[cantidad - 1], tamano) > prueba)
        {
            break;
        }

        ancho = prueba;
    }

    return ancho < anchoMax ? PartirPorAncho(texto, token, ancho, lineas) : cantidad;
}


// Rebote al aparecer: 1 + extra al principio, baja a 1 en `duracion`.
static float EscalaRebote(float t, float duracion, float extra, bool reducir)
{
    if (reducir)
    {
        return 1.0f;
    }

    float k = Limitar01(t / duracion);
    float resto = 1.0f - k;

    return 1.0f + extra * resto * resto * resto;
}


//==================================================
// ETIQUETA DE JUGADOR
//==================================================

static void TextoSegundaLinea(
    const JugadorHudMinijuego& jugador,
    const Participante& participante,
    char* destino,
    int capacidad
)
{
    if (jugador.eliminado)
    {
        std::snprintf(destino, capacidad, "%s", jugador.rol != nullptr ? jugador.rol : "");
    }
    else if (jugador.estado[0] != '\0')
    {
        std::snprintf(destino, capacidad, "%s", jugador.estado);
    }
    else if (jugador.rol != nullptr)
    {
        std::snprintf(destino, capacidad, "%s", jugador.rol);
    }
    else if (!participante.esBot)
    {
        // Nombres cortos y del mismo largo aproximado para que todas las
        // etiquetas usen la misma letra («TECLADO FLECHAS» no entraba).
        switch (participante.control)
        {
            case CONTROL_TECLADO_WASD:
                std::snprintf(destino, capacidad, "WASD");
                break;

            case CONTROL_TECLADO_FLECHAS:
                std::snprintf(destino, capacidad, "FLECHAS");
                break;

            case CONTROL_GAMEPAD:
                std::snprintf(destino, capacidad, "MANDO %d", participante.indiceGamepad + 1);
                break;

            default:
                std::snprintf(destino, capacidad, "%s", ObtenerNombreControlParticipante(participante));
                break;
        }
    }
    else
    {
        destino[0] = '\0';
    }
}


static void DibujarEtiquetaJugador(
    const EstiloUI& estilo,
    const DatosHudMinijuego& datos,
    int indice,
    const Participante& participante,
    float xRef,
    float yRef,
    float ySogaPx,
    bool ganador
)
{
    const JugadorHudMinijuego& jugador = datos.jugadores[indice];
    bool reducir = estilo.reducirMovimiento;

    Rectangle rect = { X(xRef), Y(yRef), Px(ANCHO_ETIQUETA), Px(ALTO_ETIQUETA) };

    float pinzaIzq = rect.x + Px(22.0f);
    float pinzaDer = rect.x + rect.width - Px(22.0f);

    // Salto corto cuando cambia el valor (puntaje).
    float tCambio = estadoHud.tiempoCambio[indice];
    if (!reducir && tCambio < T_SALTO_PUNTAJE)
    {
        rect.y -= Px(7.0f) * std::sin(PI * tCambio / T_SALTO_PUNTAJE);
    }

    // Eliminado: pierde la pinza derecha y queda torcido, colgando de la
    // izquierda (con un balanceo amortiguado al soltarse).
    float angulo = 0.0f;
    if (jugador.eliminado)
    {
        float t = estadoHud.tiempoEliminado[indice];
        angulo = reducir
            ? 12.0f
            : 12.0f * (1.0f - std::exp(-3.5f * t) * std::cos(9.0f * t));
    }

    rlPushMatrix();
    rlTranslatef(pinzaIzq, ySogaPx, 0.0f);
    rlRotatef(angulo, 0.0f, 0.0f, 1.0f);
    rlTranslatef(-pinzaIzq, -ySogaPx, 0.0f);

    Rectangle placa = DibujarPlacaConMarcoUI(estilo, rect, 6.0f);

    // Linea 1: J1 [BOT] ............ valor
    float xTexto = rect.x + Px(40.0f);
    float yLinea1 = rect.y + Px(10.0f);
    char nombre[8] = "";
    std::snprintf(nombre, sizeof(nombre), "J%d", participante.numeroJugador);
    int tamanoNombre = TamanoTexto(UI_TEXTO);
    int tamanoBot = TamanoTexto(UI_TEXTO_PEQUENO);

    float finNombre = xTexto + MeasureText(nombre, tamanoNombre) + Px(6.0f);
    float xBot = finNombre;

    if (participante.esBot)
    {
        finNombre += MeasureText("BOT", tamanoBot) + Px(6.0f);
    }

    float derecha = rect.x + rect.width - Px(12.0f);

    // Destello detras del valor al cambiar (solo del ancho del valor, para
    // no tapar «BOT»).
    if (tCambio < T_DESTELLO_PUNTAJE && !jugador.eliminado && jugador.valor[0] != '\0')
    {
        float k = 1.0f - tCambio / T_DESTELLO_PUNTAJE;
        int tamanoValor = TamanoTextoAjustadoUI(jugador.valor, UI_TEXTO, derecha - finNombre);
        float anchoValor = (float)MeasureText(jugador.valor, tamanoValor) + Px(8.0f);
        float xDestello = derecha - anchoValor + Px(4.0f);
        if (xDestello < finNombre - Px(3.0f)) xDestello = finNombre - Px(3.0f);

        DrawRectangleRec(
            Rectangle{ xDestello, placa.y + Px(2.0f), derecha + Px(4.0f) - xDestello, Px(26.0f) },
            Fade(UI_FOCO, 0.75f * k)
        );
    }

    Color colorJugador = jugador.eliminado ? COLOR_GRIS_FUERA : participante.color;
    DibujarBanderinColganteUI(Vector2{ rect.x + Px(12.0f), rect.y + Px(11.0f) }, colorJugador, 20.0f, 36.0f);

    DrawText(nombre, (int)xTexto, (int)yLinea1, tamanoNombre, UI_TINTA);

    if (participante.esBot)
    {
        DrawText("BOT", (int)xBot, (int)(yLinea1 + Px(3.0f)), tamanoBot, UI_TINTA_SUAVE);
    }

    if (!jugador.eliminado && jugador.valor[0] != '\0')
    {
        DibujarTextoDerechaUI(jugador.valor, derecha, yLinea1, UI_TEXTO, derecha - finNombre, UI_TINTA);
    }

    // Linea 2: estado, rol o control.
    char linea2[32] = "";
    TextoSegundaLinea(jugador, participante, linea2, (int)sizeof(linea2));

    if (linea2[0] != '\0')
    {
        DibujarTextoAjustadoUI(
            linea2,
            xTexto,
            rect.y + Px(33.0f),
            UI_TEXTO_PEQUENO,
            derecha - xTexto,
            jugador.estado[0] != '\0' && !jugador.eliminado ? COLOR_SELLO : UI_TINTA_SUAVE,
            false
        );
    }

    // Barra de progreso.
    if (jugador.progreso >= 0.0f && !jugador.eliminado)
    {
        Rectangle barra = { xTexto, rect.y + Px(51.0f), derecha - xTexto, Px(4.0f) };
        DrawRectangleRec(barra, Fade(UI_MADERA_OSCURA, 0.30f));
        barra.width *= Limitar01(jugador.progreso);
        DrawRectangleRec(barra, participante.color);
    }

    if (jugador.eliminado)
    {
        DrawRectangleRec(placa, Fade(Color{ 120, 120, 124, 255 }, 0.30f));

        // Sello «FUERA», un poco torcido.
        int tamanoSello = TamanoTexto(UI_TEXTO_PEQUENO);
        int anchoSello = MeasureText("FUERA", tamanoSello);
        float cx = rect.x + rect.width * 0.66f;
        float cy = rect.y + rect.height * 0.66f;

        rlPushMatrix();
        rlTranslatef(cx, cy, 0.0f);
        rlRotatef(-9.0f, 0.0f, 0.0f, 1.0f);

        Rectangle marcoSello = {
            -anchoSello * 0.5f - Px(7.0f), -tamanoSello * 0.5f - Px(4.0f),
            anchoSello + Px(14.0f), tamanoSello + Px(8.0f)
        };
        DrawRectangleRec(marcoSello, Fade(UI_PLACA_CLARA, 0.85f));
        DrawRectangleLinesEx(marcoSello, Px(2.0f) < 1.0f ? 1.0f : Px(2.0f), COLOR_SELLO);
        DrawText("FUERA", (int)(-anchoSello * 0.5f), (int)(-tamanoSello * 0.5f), tamanoSello, COLOR_SELLO);

        rlPopMatrix();
    }

    if (ganador)
    {
        DibujarMarcoFoco(rect);
    }

    rlPopMatrix();

    // Pinzas sobre la soga (la derecha falta si esta eliminado).
    DibujarPinzaUI(Vector2{ pinzaIzq, ySogaPx });

    if (!jugador.eliminado)
    {
        DibujarPinzaUI(Vector2{ pinzaDer, ySogaPx });
    }

    // Ayuda de controles colgando debajo (solo humanos, mientras se juega).
    if (
        jugador.controles[0] != '\0' &&
        !participante.esBot &&
        !jugador.eliminado &&
        datos.fase != HUD_MINIJUEGO_FIN
    )
    {
        char lineas[2][64] = {};
        int cantidad = 1;
        const char* salto = std::strchr(jugador.controles, '\n');

        if (salto != nullptr)
        {
            int largo = (int)(salto - jugador.controles);
            if (largo > 63) largo = 63;
            std::memcpy(lineas[0], jugador.controles, largo);
            lineas[0][largo] = '\0';
            std::snprintf(lineas[1], sizeof(lineas[1]), "%s", salto + 1);
            cantidad = 2;
        }
        else
        {
            std::snprintf(lineas[0], sizeof(lineas[0]), "%s", jugador.controles);
        }

        float altoLinea = 16.0f;
        Rectangle tira = {
            rect.x,
            rect.y + rect.height + Px(10.0f),
            rect.width,
            Px(cantidad * altoLinea + 8.0f)
        };

        DibujarCuerdaUI(estilo, Vector2{ rect.x + Px(24.0f), rect.y + rect.height - Px(3.0f) }, Vector2{ rect.x + Px(24.0f), tira.y + Px(3.0f) });
        DibujarCuerdaUI(estilo, Vector2{ rect.x + rect.width - Px(24.0f), rect.y + rect.height - Px(3.0f) }, Vector2{ rect.x + rect.width - Px(24.0f), tira.y + Px(3.0f) });

        DrawRectangleRec(
            Rectangle{ tira.x + Px(2.0f), tira.y + Px(3.0f), tira.width, tira.height },
            Fade(BLACK, 0.22f)
        );
        DibujarNueveCortes(estilo.placa, 8, tira, EscalaUI(), WHITE);

        for (int l = 0; l < cantidad; l++)
        {
            DibujarTextoAjustadoUI(
                lineas[l],
                tira.x + tira.width * 0.5f,
                tira.y + Px(5.0f + l * altoLinea),
                TOKEN_AYUDA,
                tira.width - Px(12.0f),
                UI_TINTA,
                true
            );
        }
    }
}


//==================================================
// CARTEL DEL RELOJ
//==================================================

static void DibujarMecha(float x0, float x1, float y, float fraccion, bool encendida, bool reducir, float tiempo)
{
    const EstiloUI& estilo = ObtenerEstiloUI();
    float punto = x0 + (x1 - x0) * Limitar01(fraccion);

    // Ceniza de lo que ya se quemo.
    for (float x = punto + Px(5.0f); x < x1; x += Px(7.0f))
    {
        DrawCircleV(Vector2{ x, y }, Px(1.6f), Fade(UI_TINTA, 0.35f));
    }

    if (punto > x0 + 1.0f)
    {
        DibujarCuerdaUI(estilo, Vector2{ x0, y }, Vector2{ punto, y });
    }

    if (!encendida)
    {
        return;
    }

    float chispa = reducir ? 0.0f : 0.9f * std::sin(tiempo * 38.0f) + 0.5f * std::sin(tiempo * 23.0f);

    DrawCircleV(Vector2{ punto, y }, Px(5.5f + chispa), Fade(UI_FOCO, 0.85f));
    DrawCircleV(Vector2{ punto, y }, Px(3.4f), Color{ 255, 130, 40, 255 });
    DrawCircleV(Vector2{ punto, y }, Px(1.6f), Color{ 255, 250, 225, 255 });
}


static void DibujarCartelReloj(
    const EstiloUI& estilo,
    const DatosHudMinijuego& datos,
    float xRef,
    float yRef,
    float ySogaPx
)
{
    Rectangle rect = { X(xRef), Y(yRef), Px(ANCHO_RELOJ), Px(ALTO_RELOJ) };
    Vector2 pivote = { rect.x + rect.width * 0.5f, ySogaPx };

    bool medidor = datos.reloj == HUD_RELOJ_MEDIDOR;
    bool alerta =
        !medidor &&
        datos.fase == HUD_MINIJUEGO_JUGANDO &&
        datos.tiempoRestante <= datos.segundosAlerta;

    rlPushMatrix();
    rlTranslatef(pivote.x, pivote.y, 0.0f);
    rlRotatef(estadoHud.pendulo.angulo, 0.0f, 0.0f, 1.0f);
    rlTranslatef(-pivote.x, -pivote.y, 0.0f);

    DrawRectangleRec(
        Rectangle{ rect.x + Px(4.0f), rect.y + Px(5.0f), rect.width, rect.height },
        Fade(BLACK, 0.28f)
    );
    DibujarNueveCortes(estilo.cartel, 10, rect, EscalaUI(), WHITE);

    Rectangle cara = {
        rect.x + Px(9.0f), rect.y + Px(9.0f),
        rect.width - Px(18.0f), rect.height - Px(18.0f)
    };

    if (alerta)
    {
        DrawRectangleRec(cara, Fade(COLOR_PLACA_ALERTA, 0.85f));
    }

    float cx = rect.x + rect.width * 0.5f;
    const char* etiqueta = medidor ? datos.etiquetaMedidor : "TIEMPO";

    DibujarTextoAjustadoUI(
        etiqueta, cx, rect.y + Px(12.0f), UI_TEXTO_PEQUENO - 2,
        cara.width - Px(6.0f), alerta ? COLOR_TINTA_ALERTA : UI_TINTA_SUAVE, true
    );

    char numero[16] = "";

    if (medidor)
    {
        std::snprintf(numero, sizeof(numero), "%s", datos.valorMedidor);
    }
    else
    {
        float segundos = datos.fase == HUD_MINIJUEGO_CUENTA ? datos.tiempoTotal : datos.tiempoRestante;
        int entero = (int)std::ceil(segundos > 0.0f ? segundos : 0.0f);
        std::snprintf(numero, sizeof(numero), "%d", entero);
    }

    DibujarTextoAjustadoUI(
        numero, cx, rect.y + Px(27.0f), UI_TEXTO_GRANDE,
        cara.width - Px(6.0f), alerta ? COLOR_TINTA_ALERTA : UI_TINTA, true
    );

    float x0 = rect.x + Px(18.0f);
    float x1 = rect.x + rect.width - Px(18.0f);
    float yMecha = rect.y + rect.height - Px(16.0f);

    if (medidor)
    {
        Rectangle riel = { x0, yMecha - Px(3.0f), x1 - x0, Px(6.0f) };
        DrawRectangleRec(riel, Fade(UI_MADERA_OSCURA, 0.30f));
        riel.width *= Limitar01(datos.progresoMedidor);
        DrawRectangleRec(riel, Color{ 226, 104, 46, 255 });
    }
    else
    {
        float fraccion = datos.fase == HUD_MINIJUEGO_CUENTA
            ? 1.0f
            : (datos.tiempoTotal > 0.0f ? datos.tiempoRestante / datos.tiempoTotal : 0.0f);

        DibujarMecha(
            x0, x1, yMecha, fraccion,
            datos.fase == HUD_MINIJUEGO_JUGANDO && datos.tiempoRestante > 0.0f,
            estilo.reducirMovimiento, estilo.tiempo
        );
    }

    rlPopMatrix();

    DibujarPinzaUI(Vector2{ rect.x + Px(26.0f), ySogaPx });
    DibujarPinzaUI(Vector2{ rect.x + rect.width - Px(26.0f), ySogaPx });
}


//==================================================
// CARTELES QUE CUELGAN DEL RELOJ
//==================================================

// Dos sogas desde la soga principal hasta el cartel (pixeles). Se dibujan
// antes que la fila, asi que pasan por detras de etiquetas y reloj.
static void DibujarSogasDesdeSoga(const EstiloUI& estilo, float cx, float ySoga, float yCartel)
{
    float separacion = Px(40.0f);

    if (yCartel <= ySoga)
    {
        return;
    }

    DibujarCuerdaUI(estilo, Vector2{ cx - separacion, ySoga }, Vector2{ cx - separacion, yCartel + Px(4.0f) });
    DibujarCuerdaUI(estilo, Vector2{ cx + separacion, ySoga }, Vector2{ cx + separacion, yCartel + Px(4.0f) });
}


// Cartel de presentacion: titulo y objetivo durante toda la cuenta
// regresiva. Va plantado sobre dos postes abajo y al centro, para que la
// cuenta deje ver a todos los personajes; sube al aparecer y baja justo
// antes del «¡YA!» (con reducirMovimiento, aparece y se va sin moverse).
static void DibujarPresentacion(
    const EstiloUI& estilo,
    const DatosHudMinijuego& datos,
    float cxRef
)
{
    if (datos.fase != HUD_MINIJUEGO_CUENTA || datos.cuentaRestante <= 0.0f)
    {
        return;
    }

    float anchoUtil = Px(ANCHO_PRESENTACION - 7.0f * 2.0f - 24.0f);
    char lineas[3][128] = {};
    int cantidad = PartirEnTresLineas(datos.objetivo, UI_TEXTO_PEQUENO, anchoUtil, lineas);
    float alto = 7.0f + 10.0f + 36.0f + cantidad * 18.0f + 8.0f + 7.0f;

    // Desplazamiento hacia abajo (unidades ref): entrada desde abajo y
    // salida al final de la cuenta.
    float bajada = -estadoHud.caidaPresentacion.valor;

    if (!estilo.reducirMovimiento && datos.cuentaRestante < T_RETIRO_PRESENTACION)
    {
        float k = Limitar01((T_RETIRO_PRESENTACION - datos.cuentaRestante) / T_RETIRO_PRESENTACION);
        bajada += (alto + MARGEN_INFERIOR_PRESENTACION) * k * k;
    }

    float yFondo = (float)GetScreenHeight() - Px(MARGEN_INFERIOR_PRESENTACION);

    Rectangle rect = {
        X(cxRef - ANCHO_PRESENTACION * 0.5f),
        yFondo - Px(alto) + Px(bajada),
        Px(ANCHO_PRESENTACION),
        Px(alto)
    };

    // Postes hasta el borde de abajo (pasan detras de la barra de ayuda).
    for (int lado = -1; lado <= 1; lado += 2)
    {
        float cxPoste = rect.x + rect.width * 0.5f + lado * Px(ANCHO_PRESENTACION * 0.5f - 46.0f);
        Rectangle poste = { cxPoste - Px(5.0f), rect.y + Px(12.0f), Px(10.0f), (float)GetScreenHeight() - rect.y };

        DrawRectangleRec(Rectangle{ poste.x + Px(3.0f), poste.y, poste.width, poste.height }, Fade(BLACK, 0.25f));
        DrawRectangleRec(poste, UI_MADERA_OSCURA);
        DrawRectangleRec(Rectangle{ poste.x + Px(2.0f), poste.y, Px(2.0f), poste.height }, UI_MADERA_MEDIA);
    }

    Rectangle placa = DibujarPlacaConMarcoUI(estilo, rect, 7.0f);
    float cx = rect.x + rect.width * 0.5f;

    DibujarTextoAjustadoUI(
        datos.titulo, cx, placa.y + Px(10.0f), UI_TEXTO_GRANDE,
        anchoUtil, UI_TINTA, true
    );

    for (int l = 0; l < cantidad; l++)
    {
        DibujarTextoAjustadoUI(
            lineas[l], cx, placa.y + Px(46.0f + l * 18.0f), UI_TEXTO_PEQUENO,
            anchoUtil, UI_TINTA_SUAVE, true
        );
    }
}


static void TituloFinal(
    const DatosHudMinijuego& datos,
    const Participante participantes[],
    char* destino,
    int capacidad
)
{
    if (datos.tituloFinal != nullptr)
    {
        std::snprintf(destino, capacidad, "%s", datos.tituloFinal);
        return;
    }

    const ResultadoMinijuego& resultado = *datos.resultado;

    if (resultado.desenlace == DESENLACE_EMPATE)
    {
        std::snprintf(destino, capacidad, "¡EMPATE!");
        return;
    }

    int ganadores[MAX_PARTICIPANTES] = {};
    int cantidad = ObtenerIndicesGanadores(resultado, ganadores, MAX_PARTICIPANTES);

    if (cantidad <= 0)
    {
        std::snprintf(destino, capacidad, "¡FIN!");
        return;
    }

    if (cantidad == 1)
    {
        std::snprintf(destino, capacidad, "¡GANA J%d!", participantes[ganadores[0]].numeroJugador);
        return;
    }

    // «¡GANAN J1, J3 Y J4!»
    int usado = std::snprintf(destino, capacidad, "¡GANAN ");

    for (int k = 0; k < cantidad && usado < capacidad; k++)
    {
        const char* separador = k == 0 ? "" : (k == cantidad - 1 ? " Y " : ", ");
        usado += std::snprintf(destino + usado, capacidad - usado, "%sJ%d", separador, participantes[ganadores[k]].numeroJugador);
    }

    if (usado < capacidad)
    {
        std::snprintf(destino + usado, capacidad - usado, "!");
    }
}


// cxRef centra el tablon; cxSogasRef es el reloj, de donde cuelga: asi
// sus sogas nunca cruzan una etiqueta aunque el reloj quede corrido.
static void DibujarTablonFinal(
    const EstiloUI& estilo,
    const DatosHudMinijuego& datos,
    const Participante participantes[],
    float cxRef,
    float cxSogasRef,
    float ySogaRef
)
{
    if (
        datos.fase != HUD_MINIJUEGO_FIN ||
        estadoHud.tiempoFase < T_TABLON_FINAL ||
        datos.resultado == nullptr ||
        datos.resultado->estado != RESULTADO_MINIJUEGO_FINALIZADO
    )
    {
        return;
    }

    const ResultadoMinijuego& resultado = *datos.resultado;

    // Orden por puesto (0 = sin puesto, al final).
    int orden[MAX_PARTICIPANTES] = {};
    int filas = 0;

    for (int puesto = 1; puesto <= MAX_PARTICIPANTES + 1; puesto++)
    {
        for (int i = 0; i < MAX_PARTICIPANTES; i++)
        {
            if (!datos.jugadores[i].visible) continue;

            int p = resultado.participantes[i].posicionFinal;
            bool coincide = puesto <= MAX_PARTICIPANTES ? p == puesto : (p <= 0 || p > MAX_PARTICIPANTES);
            if (coincide) orden[filas++] = i;
        }
    }

    // «R: REINICIAR» no va en el tablon: ZonaPruebas lo muestra en la
    // tira de ayuda de abajo (HudMinijuegoDibujado), una sola vez.
    bool conNota = datos.notaFinal != nullptr && datos.notaFinal[0] != '\0';
    const float altoFila = 32.0f;
    float alto = 14.0f + 52.0f + filas * altoFila + (conNota ? 22.0f : 0.0f) + 8.0f + 14.0f;

    Rectangle rect = {
        X(cxRef - ANCHO_TABLON_FINAL * 0.5f),
        Y(Y_CARTELES + estadoHud.caidaTablon.valor),
        Px(ANCHO_TABLON_FINAL),
        Px(alto)
    };

    DibujarSogasDesdeSoga(estilo, X(cxSogasRef), Y(ySogaRef), rect.y);

    DrawRectangleRec(
        Rectangle{ rect.x + Px(6.0f), rect.y + Px(8.0f), rect.width, rect.height },
        Fade(BLACK, 0.30f)
    );
    DibujarNueveCortes(estilo.madera, 12, rect, EscalaUI(), WHITE);

    float margen = Px(14.0f);
    Rectangle placa = { rect.x + margen, rect.y + margen, rect.width - margen * 2.0f, rect.height - margen * 2.0f };
    DibujarNueveCortes(estilo.placa, 8, placa, EscalaUI(), WHITE);

    char titulo[48] = "";
    TituloFinal(datos, participantes, titulo, (int)sizeof(titulo));

    float cx = rect.x + rect.width * 0.5f;
    DibujarTextoAjustadoUI(titulo, cx, placa.y + Px(12.0f), UI_TEXTO_GRANDE, placa.width - Px(24.0f), UI_TINTA, true);

    DrawRectangleRec(
        Rectangle{ placa.x + Px(16.0f), placa.y + Px(48.0f), placa.width - Px(32.0f), Px(2.0f) },
        Fade(UI_MADERA_OSCURA, 0.45f)
    );

    static const Color medallas[4] = {
        { 255, 200, 70, 255 },      // oro (UI_FOCO)
        { 206, 208, 216, 255 },     // plata
        { 220, 156, 104, 255 },     // bronce
        { 196, 170, 130, 255 }      // madera clara
    };

    for (int f = 0; f < filas; f++)
    {
        int i = orden[f];
        const Participante& participante = participantes[i];
        int puesto = resultado.participantes[i].posicionFinal;
        bool gana = ParticipanteEsGanador(resultado, i);

        float y = placa.y + Px(56.0f + f * altoFila);
        float centroY = y + Px(altoFila * 0.5f);

        if (gana)
        {
            DrawRectangleRec(
                Rectangle{ placa.x + Px(8.0f), y + Px(2.0f), placa.width - Px(16.0f), Px(altoFila - 4.0f) },
                Fade(UI_FOCO, 0.30f)
            );
        }

        // Medalla con el puesto.
        float xMedalla = placa.x + Px(30.0f);
        Color medalla = medallas[puesto >= 1 && puesto <= 3 ? puesto - 1 : 3];
        DrawCircleV(Vector2{ xMedalla, centroY }, Px(12.5f), UI_TINTA);
        DrawCircleV(Vector2{ xMedalla, centroY }, Px(11.0f), medalla);

        const char* textoPuesto = puesto > 0 ? TextFormat("%d", puesto) : "-";
        int tamanoPuesto = TamanoTexto(UI_TEXTO_PEQUENO);
        DrawText(textoPuesto, (int)(xMedalla - MeasureText(textoPuesto, tamanoPuesto) * 0.5f), (int)(centroY - tamanoPuesto * 0.5f), tamanoPuesto, UI_TINTA);

        DibujarBanderinColganteUI(Vector2{ placa.x + Px(54.0f), centroY - Px(11.0f) }, participante.color, 13.0f, 22.0f);

        int tamanoNombre = TamanoTexto(UI_TEXTO);
        const char* nombre = TextFormat("J%d", participante.numeroJugador);
        float xNombre = placa.x + Px(78.0f);
        DrawText(nombre, (int)xNombre, (int)(centroY - tamanoNombre * 0.5f), tamanoNombre, UI_TINTA);

        float finNombre = xNombre + MeasureText(nombre, tamanoNombre) + Px(8.0f);
        const char* segunda = participante.esBot ? "BOT" : "";
        if (datos.jugadores[i].rol != nullptr)
        {
            segunda = participante.esBot
                ? TextFormat("%s  BOT", datos.jugadores[i].rol)
                : datos.jugadores[i].rol;
        }

        const char* desempate = datos.jugadores[i].detalleDesempate;
        bool conDesempate = desempate[0] != '\0';

        if (segunda[0] != '\0')
        {
            DibujarTextoAjustadoUI(
                segunda, finNombre, centroY - TamanoTexto(UI_TEXTO_PEQUENO) * 0.5f + Px(1.0f),
                UI_TEXTO_PEQUENO, placa.width * (conDesempate ? 0.26f : 0.40f), UI_TINTA_SUAVE, false
            );
        }

        float derechaDetalle = placa.x + placa.width - Px(18.0f);
        const char* detalle = datos.jugadores[i].detalleFinal;
        float anchoDetalle = placa.width * (conDesempate ? 0.20f : 0.32f);

        DibujarTextoDerechaUI(detalle, derechaDetalle, centroY - tamanoNombre * 0.5f, UI_TEXTO, anchoDetalle, UI_TINTA);

        // Criterio de desempate en letra chica, a la izquierda del puntaje.
        if (conDesempate)
        {
            int tamanoDetalle = TamanoTextoAjustadoUI(detalle, UI_TEXTO, anchoDetalle);
            float derechaDesempate = derechaDetalle - MeasureText(detalle, tamanoDetalle) - Px(16.0f);

            DibujarTextoDerechaUI(
                desempate,
                derechaDesempate,
                centroY - TamanoTexto(UI_TEXTO_PEQUENO) * 0.5f + Px(1.0f),
                UI_TEXTO_PEQUENO,
                placa.width * 0.24f,
                UI_TINTA_SUAVE
            );
        }
    }

    if (conNota)
    {
        float yNota = placa.y + Px(56.0f + filas * altoFila + 6.0f);

        DibujarTextoAjustadoUI(
            datos.notaFinal, cx, yNota, UI_TEXTO_PEQUENO,
            placa.width - Px(24.0f), UI_TINTA_SUAVE, true
        );
    }
}


//==================================================
// ANUNCIOS SOBRE EL MUNDO
//==================================================

static void DibujarAnuncios(const EstiloUI& estilo, const DatosHudMinijuego& datos)
{
    bool reducir = estilo.reducirMovimiento;
    float cy = (float)GetScreenHeight() * 0.5f + Px(20.0f);
    float t = estadoHud.tiempoFase;

    if (datos.fase == HUD_MINIJUEGO_CUENTA)
    {
        if (datos.ocultarNumeroCuenta)
        {
            return;
        }

        const char* texto = TextFormat("%d", estadoHud.numeroCuenta);
        float escala = EscalaRebote(estadoHud.tiempoNumero, 0.22f, 0.5f, reducir);

        DibujarAnuncio(texto, cy, TOKEN_ANUNCIO * escala, UI_TEXTO_MUNDO, 1.0f);
        return;
    }

    if (datos.fase == HUD_MINIJUEGO_JUGANDO && t < T_YA)
    {
        float escala = EscalaRebote(t, 0.25f, 0.35f, reducir);
        float alfa = t > T_YA - 0.3f ? (T_YA - t) / 0.3f : 1.0f;

        DibujarAnuncio("¡YA!", cy, TOKEN_ANUNCIO * escala, UI_FOCO, alfa);
        return;
    }

    if (datos.fase == HUD_MINIJUEGO_FIN && t < T_ANUNCIO_FIN)
    {
        float escala = EscalaRebote(t, 0.25f, 0.35f, reducir);
        float alfa = t > T_ANUNCIO_FIN - 0.3f ? (T_ANUNCIO_FIN - t) / 0.3f : 1.0f;

        DibujarAnuncio(datos.finPorTiempo ? "¡TIEMPO!" : "¡FIN!", cy, TOKEN_ANUNCIO_FIN * escala, UI_FOCO, alfa);
    }
}


//==================================================
// API
//==================================================

void PrepararDatosHudMinijuego(
    DatosHudMinijuego& datos,
    const char* titulo,
    const char* objetivo,
    const ResultadoMinijuego& resultado
)
{
    datos.titulo = titulo;
    datos.objetivo = objetivo;
    datos.resultado = &resultado;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        datos.jugadores[i].visible = resultado.participantes[i].participo;
    }
}


void DibujarHudMinijuego(
    const DatosHudMinijuego& datos,
    const Participante participantes[]
)
{
    const EstiloUI& estilo = ObtenerEstiloUI();

    if (!estilo.cargado)
    {
        return;
    }

    hudDibujadoEsteFrame = true;

    ActualizarEstadoHud(datos, estilo);

    // Reparto: equipo 0 a la izquierda del reloj y equipo 1 a la derecha;
    // sin equipos, la primera mitad a la izquierda.
    int izquierda[MAX_PARTICIPANTES] = {};
    int derecha[MAX_PARTICIPANTES] = {};
    int cantidadIzq = 0;
    int cantidadDer = 0;
    int visibles = 0;
    bool porEquipos = false;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!datos.jugadores[i].visible) continue;
        visibles++;
        if (datos.jugadores[i].equipo >= 0) porEquipos = true;
    }

    int mitad = (visibles + 1) / 2;
    int vistos = 0;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (!datos.jugadores[i].visible) continue;

        bool aLaIzquierda = porEquipos ? datos.jugadores[i].equipo == 0 : vistos < mitad;
        if (aLaIzquierda) izquierda[cantidadIzq++] = i;
        else derecha[cantidadDer++] = i;
        vistos++;
    }

    float anchoIzq = cantidadIzq > 0 ? cantidadIzq * ANCHO_ETIQUETA + (cantidadIzq - 1) * SEPARACION_ETIQUETAS : 0.0f;
    float anchoDer = cantidadDer > 0 ? cantidadDer * ANCHO_ETIQUETA + (cantidadDer - 1) * SEPARACION_ETIQUETAS : 0.0f;
    float total = anchoIzq + SEPARACION_RELOJ + ANCHO_RELOJ + SEPARACION_RELOJ + anchoDer;
    float inicio = UI_REF_ANCHO * 0.5f - total * 0.5f;
    float xReloj = inicio + anchoIzq + SEPARACION_RELOJ;
    float cxReloj = xReloj + ANCHO_RELOJ * 0.5f;

    float sogaInicio = inicio - SOGA_SOBRANTE;
    float sogaFin = inicio + total + SOGA_SOBRANTE;

    float yRelojRef = AlturaSoga(cxReloj, sogaInicio, sogaFin) + 8.0f;

    // 1) Carteles grandes, centrados en pantalla aunque el reloj quede
    //    corrido (formatos por equipos). El tablon final cuelga del tramo
    //    de soga del reloj, por detras de la fila.
    float cxCarteles = UI_REF_ANCHO * 0.5f;
    float margenSogas = ANCHO_TABLON_FINAL * 0.5f - 70.0f;
    float cxSogas = cxReloj;
    if (cxSogas < cxCarteles - margenSogas) cxSogas = cxCarteles - margenSogas;
    if (cxSogas > cxCarteles + margenSogas) cxSogas = cxCarteles + margenSogas;
    float ySogaCarteles = AlturaSoga(cxSogas, sogaInicio, sogaFin);

    DibujarPresentacion(estilo, datos, cxCarteles);
    DibujarTablonFinal(estilo, datos, participantes, cxCarteles, cxSogas, ySogaCarteles);

    // 2) Soga: dos tramos verticales desde el borde y la comba.
    DibujarCuerdaUI(estilo, Vector2{ X(sogaInicio), 0.0f }, Vector2{ X(sogaInicio), Y(Y_SOGA) });
    DibujarCuerdaUI(estilo, Vector2{ X(sogaFin), 0.0f }, Vector2{ X(sogaFin), Y(Y_SOGA) });

    const int tramos = 12;
    for (int k = 0; k < tramos; k++)
    {
        float xa = sogaInicio + (sogaFin - sogaInicio) * k / tramos;
        float xb = sogaInicio + (sogaFin - sogaInicio) * (k + 1) / tramos;
        DibujarCuerdaUI(
            estilo,
            Vector2{ X(xa), Y(AlturaSoga(xa, sogaInicio, sogaFin)) },
            Vector2{ X(xb), Y(AlturaSoga(xb, sogaInicio, sogaFin)) }
        );
    }

    DrawCircleV(Vector2{ X(sogaInicio), Y(Y_SOGA) }, Px(4.5f), UI_MADERA_OSCURA);
    DrawCircleV(Vector2{ X(sogaFin), Y(Y_SOGA) }, Px(4.5f), UI_MADERA_OSCURA);

    // 3) Etiquetas y reloj.
    bool hayGanadores =
        datos.fase == HUD_MINIJUEGO_FIN &&
        datos.resultado != nullptr &&
        datos.resultado->estado == RESULTADO_MINIJUEGO_FINALIZADO;

    for (int lado = 0; lado < 2; lado++)
    {
        const int* lista = lado == 0 ? izquierda : derecha;
        int cantidad = lado == 0 ? cantidadIzq : cantidadDer;
        float x = lado == 0 ? inicio : xReloj + ANCHO_RELOJ + SEPARACION_RELOJ;

        for (int k = 0; k < cantidad; k++)
        {
            int i = lista[k];
            float centro = x + ANCHO_ETIQUETA * 0.5f;
            float ySoga = AlturaSoga(centro, sogaInicio, sogaFin);
            bool ganador = hayGanadores && ParticipanteEsGanador(*datos.resultado, i);

            DibujarEtiquetaJugador(estilo, datos, i, participantes[i], x, ySoga + 8.0f, Y(ySoga), ganador);

            x += ANCHO_ETIQUETA + SEPARACION_ETIQUETAS;
        }
    }

    DibujarCartelReloj(estilo, datos, xReloj, yRelojRef, Y(AlturaSoga(cxReloj, sogaInicio, sogaFin)));

    // 4) Anuncios grandes encima de todo.
    DibujarAnuncios(estilo, datos);
}


void ReiniciarMarcaHudMinijuego()
{
    hudDibujadoEsteFrame = false;
}


bool HudMinijuegoDibujado()
{
    return hudDibujadoEsteFrame;
}


const char* TextoBotonGolpeHud(const Participante& participante)
{
    if (participante.esBot)
    {
        return "";
    }

    switch (participante.control)
    {
        case CONTROL_GAMEPAD:
            return "X";

        case CONTROL_TECLADO_FLECHAS:
            return "SHIFT DER";

        case CONTROL_TECLADO_COMPLETO:
        case CONTROL_TECLADO_WASD:
            return "E";

        case CONTROL_NINGUNO:
            break;
    }

    return "";
}


const char* TextoMoverHud(const Participante& participante)
{
    if (participante.esBot)
    {
        return "";
    }

    switch (participante.control)
    {
        case CONTROL_GAMEPAD:
            return "STICK";

        case CONTROL_TECLADO_FLECHAS:
            return "FLECHAS";

        case CONTROL_TECLADO_WASD:
            return "WASD";

        case CONTROL_TECLADO_COMPLETO:
            return "WASD/FLECHAS";

        case CONTROL_NINGUNO:
            break;
    }

    return "";
}
