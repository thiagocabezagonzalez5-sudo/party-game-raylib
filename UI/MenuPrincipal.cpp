#include "UI/MenuPrincipal.h"
#include "Systems/Input.h"

#include <math.h>


//==================================================
// CONSTANTES
//==================================================

static const float DURACION_FADE_BLANCO =
    0.90f;

static const float DURACION_FADE_NEGRO =
    0.40f;

static const float RETRASO_UI =
    0.20f;

static const float DURACION_FADE_UI =
    0.40f;

// Tiempo de bloqueo de input al entrar.
static const float DURACION_ENTRADA =
    0.45f;

// Segundos sin input antes de volver al recorrido ambiental.
static const float TIEMPO_VOLVER_AMBIENTAL =
    6.0f;

// Duracion del acercamiento antes de levantar el flag.
static const float DURACION_CONFIRMACION =
    0.5f;

static const float OPACIDAD_FONDO_OSCURO =
    0.45f;

static const char* NOMBRES_OPCION[OPCION_MENU_CANTIDAD] =
{
    "PARTIDA",
    "MINIJUEGOS",
    "CONFIGURACION",
    "SALIR"
};

static const char* DESCRIPCIONES_OPCION[OPCION_MENU_CANTIDAD] =
{
    "Tablero por turnos: dados, casillas y minijuegos",
    "Juega retos sueltos sin tablero",
    "Controles, video y audio",
    "Cerrar el juego"
};

static const Color COLOR_ACENTO =
    { 255, 200, 70, 255 };


//==================================================
// UTILIDADES DE TEXTO
//==================================================

static void DibujarTextoCentrado(
    const char* texto,
    int centroX,
    int y,
    int tamano,
    Color color,
    float opacidad
)
{
    int ancho =
        MeasureText(texto, tamano);

    int x =
        centroX - ancho / 2;

    DrawText(
        texto,
        x + 2,
        y + 2,
        tamano,
        Fade(BLACK, 0.7f * opacidad)
    );

    DrawText(
        texto,
        x,
        y,
        tamano,
        Fade(color, opacidad)
    );
}


//==================================================
// ICONOS (primitivas 2D)
//==================================================

static void DibujarIconoOpcion(
    int opcion,
    float cx,
    float cy,
    float r,
    Color color
)
{
    switch (opcion)
    {
        case OPCION_MENU_PARTIDA:
        {
            // Dirigible.
            DrawEllipse((int)cx, (int)(cy - r * 0.2f), r * 0.95f, r * 0.55f, color);
            DrawRectangle((int)(cx - r * 0.3f), (int)(cy + r * 0.45f), (int)(r * 0.6f), (int)(r * 0.25f), color);
            DrawTriangle(
                { cx - r * 0.85f, cy - r * 0.2f },
                { cx - r * 1.15f, cy - r * 0.55f },
                { cx - r * 1.15f, cy + r * 0.15f },
                color
            );
            break;
        }

        case OPCION_MENU_MINIJUEGOS:
        {
            // Barco con vela.
            DrawTriangle(
                { cx - r * 0.9f, cy + r * 0.2f },
                { cx + r * 0.9f, cy + r * 0.2f },
                { cx + r * 0.5f, cy + r * 0.7f },
                color
            );
            DrawTriangle(
                { cx - r * 0.5f, cy + r * 0.7f },
                { cx - r * 0.9f, cy + r * 0.2f },
                { cx + r * 0.5f, cy + r * 0.7f },
                color
            );
            DrawRectangle((int)(cx - 2), (int)(cy - r * 0.8f), 4, (int)(r * 1.0f), color);
            DrawTriangle(
                { cx + 3, cy - r * 0.8f },
                { cx + 3, cy + r * 0.05f },
                { cx + r * 0.7f, cy + r * 0.05f },
                color
            );
            break;
        }

        case OPCION_MENU_CONFIGURACION:
        {
            // Engranaje.
            for (int k = 0; k < 8; k++)
            {
                float a =
                    k * (PI / 4.0f);

                DrawCircle(
                    (int)(cx + cosf(a) * r * 0.85f),
                    (int)(cy + sinf(a) * r * 0.85f),
                    r * 0.2f,
                    color
                );
            }

            DrawCircle((int)cx, (int)cy, r * 0.75f, color);
            DrawCircle((int)cx, (int)cy, r * 0.32f, { 40, 30, 60, 255 });
            break;
        }

        case OPCION_MENU_SALIR:
        {
            // Puerta con arco.
            DrawRectangle((int)(cx - r * 0.6f), (int)cy - 2, (int)(r * 1.2f), (int)(r * 0.9f) + 2, color);
            DrawCircle((int)cx, (int)cy, r * 0.6f, color);
            DrawRectangle((int)(cx - r * 0.3f), (int)(cy - r * 0.1f), (int)(r * 0.6f), (int)(r * 0.9f), { 40, 30, 60, 255 });
            DrawCircle((int)cx, (int)(cy - r * 0.1f), r * 0.3f, { 40, 30, 60, 255 });
            break;
        }

        default:
        {
            break;
        }
    }
}


//==================================================
// RECTANGULO DEL PANEL INFERIOR
//==================================================

static Rectangle ObtenerRectPanel()
{
    float ancho =
        (float)GetScreenWidth();

    float alto =
        (float)GetScreenHeight();

    float anchoPanel =
        ancho * 0.8f;

    if (anchoPanel > 720.0f)
    {
        anchoPanel =
            720.0f;
    }

    return Rectangle
    {
        ancho / 2.0f - anchoPanel / 2.0f,
        alto - 150.0f,
        anchoPanel,
        92.0f
    };
}


//==================================================
// REINICIAR ESTADO DE INTERACCION
//==================================================

static void ReiniciarAcciones(
    MenuPrincipal& menu
)
{
    menu.empezarTablero =
        false;

    menu.empezarMinijuegos =
        false;

    menu.abrirConfiguracion =
        false;

    menu.salir =
        false;

    menu.confirmando =
        false;

    menu.tiempoConfirmacion =
        0.0f;

    menu.tiempoSinInput =
        999.0f;

    for (int i = 0; i < 4; i++)
    {
        menu.ejeHorizontalPrevio[i] =
            0;

        menu.ejeVerticalPrevio[i] =
            0;
    }
}


//==================================================
// APLICAR ACCION
//==================================================

static void AplicarAccion(
    MenuPrincipal& menu
)
{
    switch (menu.opcionSeleccionada)
    {
        case OPCION_MENU_PARTIDA:
        {
            menu.empezarTablero =
                true;
            break;
        }

        case OPCION_MENU_MINIJUEGOS:
        {
            menu.empezarMinijuegos =
                true;
            break;
        }

        case OPCION_MENU_CONFIGURACION:
        {
            menu.abrirConfiguracion =
                true;
            break;
        }

        case OPCION_MENU_SALIR:
        {
            menu.salir =
                true;
            break;
        }

        default:
        {
            break;
        }
    }
}


//==================================================
// LEER NAVEGACION
//==================================================
// Teclado (flechas, A/D, Enter/Espacio) y gamepads 0-3 (cruceta,
// stick izquierdo, boton A/cruz). Devuelve el paso de seleccion
// (-1, 0, 1) y si se pidio confirmar.

static void LeerNavegacion(
    MenuPrincipal& menu,
    int& paso,
    bool& confirmar
)
{
    paso =
        0;

    confirmar =
        false;

    if (
        IsKeyPressed(KEY_RIGHT) ||
        IsKeyPressed(KEY_D) ||
        IsKeyPressed(KEY_DOWN)
    )
    {
        paso++;
    }

    if (
        IsKeyPressed(KEY_LEFT) ||
        IsKeyPressed(KEY_A) ||
        IsKeyPressed(KEY_UP)
    )
    {
        paso--;
    }

    if (
        IsKeyPressed(KEY_ENTER) ||
        IsKeyPressed(KEY_SPACE) ||
        IsKeyPressed(KEY_KP_ENTER)
    )
    {
        confirmar =
            true;
    }

    for (int g = 0; g < 4; g++)
    {
        if (!IsGamepadAvailable(g))
        {
            menu.ejeHorizontalPrevio[g] =
                0;

            menu.ejeVerticalPrevio[g] =
                0;

            continue;
        }

        // Stick ya filtrado por la zona muerta configurada.
        Vector2 stick = LeerStickIzquierdo(g);

        int horizontal =
            stick.x > 0.4f ? 1 : (stick.x < -0.4f ? -1 : 0);

        int vertical =
            stick.y > 0.4f ? 1 : (stick.y < -0.4f ? -1 : 0);

        if (
            IsGamepadButtonPressed(g, GAMEPAD_BUTTON_LEFT_FACE_RIGHT) ||
            IsGamepadButtonPressed(g, GAMEPAD_BUTTON_LEFT_FACE_DOWN) ||
            (horizontal == 1 && menu.ejeHorizontalPrevio[g] != 1) ||
            (vertical == 1 && menu.ejeVerticalPrevio[g] != 1)
        )
        {
            paso++;
        }

        if (
            IsGamepadButtonPressed(g, GAMEPAD_BUTTON_LEFT_FACE_LEFT) ||
            IsGamepadButtonPressed(g, GAMEPAD_BUTTON_LEFT_FACE_UP) ||
            (horizontal == -1 && menu.ejeHorizontalPrevio[g] != -1) ||
            (vertical == -1 && menu.ejeVerticalPrevio[g] != -1)
        )
        {
            paso--;
        }

        if (IsGamepadButtonPressed(g, GAMEPAD_BUTTON_RIGHT_FACE_DOWN))
        {
            confirmar =
                true;
        }

        menu.ejeHorizontalPrevio[g] =
            horizontal;

        menu.ejeVerticalPrevio[g] =
            vertical;
    }

    if (paso > 1)
    {
        paso =
            1;
    }

    if (paso < -1)
    {
        paso =
            -1;
    }
}


//==================================================
// OPCION BAJO EL MOUSE
//==================================================

static int ObtenerOpcionBajoMouse(
    const MenuPrincipal& menu
)
{
    Vector2 mouse =
        GetMousePosition();

    Rectangle panel =
        ObtenerRectPanel();

    if (mouse.y > panel.y)
    {
        return -1;
    }

    float ancho =
        (float)GetScreenWidth();

    int mejor =
        -1;

    float mejorDistancia =
        ancho * 0.12f;

    for (int i = 0; i < OPCION_MENU_CANTIDAD; i++)
    {
        if (!menu.hub.opcionVisible[i])
        {
            continue;
        }

        float distancia =
            fabsf(mouse.x - menu.hub.posicionPantallaOpcion[i].x);

        if (distancia < mejorDistancia)
        {
            mejorDistancia =
                distancia;

            mejor =
                i;
        }
    }

    return mejor;
}


//==================================================
// INICIALIZAR
//==================================================

void MenuPrincipal::Inicializar()
{
    opcionSeleccionada =
        0;

    ReiniciarAcciones(*this);

    if (!recursosCargados)
    {
        hub.Inicializar();

        recursosCargados =
            true;
    }
}


//==================================================
// PREPARAR ENTRADA
//==================================================

void MenuPrincipal::PrepararEntrada(
    bool desdeLogo
)
{
    opcionSeleccionada =
        0;

    ReiniciarAcciones(*this);

    tiempoEntrada =
        0.0f;

    entradaActiva =
        true;

    fadeBlancoActivo =
        desdeLogo;
}


//==================================================
// ACTUALIZAR
//==================================================

void MenuPrincipal::Actualizar(
    float deltaTime
)
{
    //------------------------------
    // TRANSICION
    //------------------------------

    if (entradaActiva)
    {
        tiempoEntrada +=
            deltaTime;

        if (tiempoEntrada >= DURACION_ENTRADA)
        {
            entradaActiva =
                false;
        }
    }

    //------------------------------
    // INPUT
    //------------------------------

    if (confirmando)
    {
        tiempoConfirmacion +=
            deltaTime;

        if (tiempoConfirmacion >= DURACION_CONFIRMACION)
        {
            confirmando =
                false;

            tiempoConfirmacion =
                0.0f;

            AplicarAccion(*this);
        }
    }
    else if (!entradaActiva)
    {
        tiempoSinInput +=
            deltaTime;

        int paso;
        bool confirmar;

        LeerNavegacion(
            *this,
            paso,
            confirmar
        );

        if (paso != 0)
        {
            opcionSeleccionada =
                (
                    opcionSeleccionada +
                    paso +
                    OPCION_MENU_CANTIDAD
                ) % OPCION_MENU_CANTIDAD;

            tiempoSinInput =
                0.0f;
        }

        // Mouse opcional: pasar sobre un objeto lo selecciona.
        Vector2 movimientoMouse =
            GetMouseDelta();

        bool mouseMovido =
            movimientoMouse.x != 0.0f ||
            movimientoMouse.y != 0.0f;

        int bajoMouse =
            ObtenerOpcionBajoMouse(*this);

        if (mouseMovido && bajoMouse >= 0)
        {
            if (bajoMouse != opcionSeleccionada)
            {
                opcionSeleccionada =
                    bajoMouse;
            }

            tiempoSinInput =
                0.0f;
        }

        if (
            bajoMouse >= 0 &&
            IsMouseButtonPressed(MOUSE_BUTTON_LEFT)
        )
        {
            opcionSeleccionada =
                bajoMouse;

            confirmar =
                true;
        }

        if (confirmar)
        {
            confirmando =
                true;

            tiempoConfirmacion =
                0.0f;

            tiempoSinInput =
                0.0f;
        }
    }

    //------------------------------
    // CAMARA
    //------------------------------

    bool enfocar =
        confirmando ||
        tiempoSinInput < TIEMPO_VOLVER_AMBIENTAL;

    hub.Actualizar(
        deltaTime,
        opcionSeleccionada,
        enfocar,
        confirmando ? 1.0f : 0.0f
    );
}


//==================================================
// DIBUJAR
//==================================================

void MenuPrincipal::Dibujar()
{
    int ancho =
        GetScreenWidth();

    int alto =
        GetScreenHeight();

    hub.Dibujar(opcionSeleccionada);

    //------------------------------
    // PROGRESO DE LA UI
    //------------------------------

    float progresoUI =
        (tiempoEntrada - RETRASO_UI) / DURACION_FADE_UI;

    if (progresoUI < 0.0f)
    {
        progresoUI =
            0.0f;
    }

    if (progresoUI > 1.0f)
    {
        progresoUI =
            1.0f;
    }

    if (!entradaActiva)
    {
        progresoUI =
            1.0f;
    }

    // Al confirmar, la interfaz se desvanece mientras la camara se acerca.
    float desvanecerConfirmacion =
        confirmando
        ? 1.0f - tiempoConfirmacion / DURACION_CONFIRMACION * 0.6f
        : 1.0f;

    float opacidad =
        progresoUI * desvanecerConfirmacion;

    float tiempo =
        hub.tiempo;

    // Vineta suave para dar contraste a los textos.
    DrawRectangleGradientV(
        0,
        0,
        ancho,
        140,
        Fade(BLACK, 0.35f * opacidad),
        Fade(BLACK, 0.0f)
    );

    DrawRectangleGradientV(
        0,
        alto - 200,
        ancho,
        200,
        Fade(BLACK, 0.0f),
        Fade(BLACK, 0.5f * opacidad)
    );

    //------------------------------
    // TITULO
    //------------------------------

    DibujarTextoCentrado(
        "PARTY GAME",
        ancho / 2,
        22,
        54,
        COLOR_ACENTO,
        opacidad
    );

    //------------------------------
    // CARTELES SOBRE LOS OBJETOS
    //------------------------------

    for (int i = 0; i < OPCION_MENU_CANTIDAD; i++)
    {
        if (!hub.opcionVisible[i])
        {
            continue;
        }

        Vector2 posicion =
            hub.posicionPantallaOpcion[i];

        bool seleccionada =
            i == opcionSeleccionada;

        int tamano =
            seleccionada ? 30 : 20;

        float rebote =
            seleccionada ? 5.0f * sinf(tiempo * 4.0f) : 0.0f;

        int anchoTexto =
            MeasureText(NOMBRES_OPCION[i], tamano);

        float cartelAncho =
            (float)anchoTexto + 24.0f;

        float cartelAlto =
            (float)tamano + 14.0f;

        Rectangle cartel =
        {
            posicion.x - cartelAncho / 2.0f,
            posicion.y - cartelAlto + rebote,
            cartelAncho,
            cartelAlto
        };

        DrawRectangleRounded(
            cartel,
            0.35f,
            8,
            seleccionada
            ? Fade({ 60, 36, 84, 255 }, 0.92f * opacidad)
            : Fade({ 30, 24, 54, 255 }, 0.55f * opacidad)
        );

        if (seleccionada)
        {
            DrawRectangleLinesEx(
                cartel,
                2.0f,
                Fade(COLOR_ACENTO, opacidad)
            );

            DrawTriangle(
                { posicion.x - 9.0f, cartel.y + cartelAlto },
                { posicion.x, cartel.y + cartelAlto + 11.0f },
                { posicion.x + 9.0f, cartel.y + cartelAlto },
                Fade(COLOR_ACENTO, opacidad)
            );
        }

        DrawText(
            NOMBRES_OPCION[i],
            (int)(cartel.x + 12.0f),
            (int)(cartel.y + 7.0f),
            tamano,
            seleccionada
            ? Fade(COLOR_ACENTO, opacidad)
            : Fade(RAYWHITE, 0.85f * opacidad)
        );
    }

    //------------------------------
    // PANEL INFERIOR
    //------------------------------

    Rectangle panel =
        ObtenerRectPanel();

    DrawRectangleRounded(
        panel,
        0.2f,
        8,
        Fade({ 34, 24, 58, 255 }, 0.88f * opacidad)
    );

    DrawRectangleLinesEx(
        panel,
        2.0f,
        Fade(COLOR_ACENTO, 0.8f * opacidad)
    );

    float centroIconoX =
        panel.x + 56.0f;

    float centroIconoY =
        panel.y + panel.height / 2.0f;

    DrawCircle(
        (int)centroIconoX,
        (int)centroIconoY,
        36.0f,
        Fade({ 70, 48, 104, 255 }, opacidad)
    );

    DibujarIconoOpcion(
        opcionSeleccionada,
        centroIconoX,
        centroIconoY,
        24.0f,
        Fade(COLOR_ACENTO, opacidad)
    );

    DrawText(
        NOMBRES_OPCION[opcionSeleccionada],
        (int)(panel.x + 112.0f),
        (int)(panel.y + 14.0f),
        32,
        Fade(RAYWHITE, opacidad)
    );

    DrawText(
        DESCRIPCIONES_OPCION[opcionSeleccionada],
        (int)(panel.x + 112.0f),
        (int)(panel.y + 54.0f),
        20,
        Fade({ 220, 210, 235, 255 }, opacidad)
    );

    // Indicador de posicion (4 puntos).
    for (int i = 0; i < OPCION_MENU_CANTIDAD; i++)
    {
        DrawCircle(
            (int)(panel.x + panel.width - 24.0f - (OPCION_MENU_CANTIDAD - 1 - i) * 18.0f),
            (int)(panel.y + 20.0f),
            i == opcionSeleccionada ? 6.0f : 4.0f,
            i == opcionSeleccionada
            ? Fade(COLOR_ACENTO, opacidad)
            : Fade(GRAY, 0.7f * opacidad)
        );
    }

    //------------------------------
    // AYUDA DE CONTROLES
    //------------------------------

    DibujarTextoCentrado(
        "<- -> ELEGIR     ENTER / A: CONFIRMAR     MOUSE: APUNTAR Y CLICK",
        ancho / 2,
        alto - 40,
        18,
        { 235, 228, 240, 255 },
        opacidad
    );

    //------------------------------
    // FADE DE ENTRADA (por encima de todo)
    //------------------------------

    if (entradaActiva)
    {
        float duracion =
            fadeBlancoActivo
            ? DURACION_FADE_BLANCO
            : DURACION_FADE_NEGRO;

        float progreso =
            tiempoEntrada / duracion;

        if (progreso > 1.0f)
        {
            progreso =
                1.0f;
        }

        // Curva suave para que el fade no tenga arranque brusco.
        float alfa =
            1.0f - progreso * progreso * (3.0f - 2.0f * progreso);

        if (alfa > 0.0f)
        {
            DrawRectangle(
                0,
                0,
                ancho,
                alto,
                Fade(
                    fadeBlancoActivo ? WHITE : BLACK,
                    alfa
                )
            );
        }
    }
}


//==================================================
// FONDO AMBIENTAL
//==================================================

void MenuPrincipal::ActualizarFondo(
    float deltaTime
)
{
    if (!recursosCargados)
    {
        Inicializar();
    }

    hub.Actualizar(
        deltaTime,
        -1,
        false,
        0.0f
    );
}


void MenuPrincipal::DibujarFondo()
{
    if (!recursosCargados)
    {
        Inicializar();
    }

    hub.Dibujar(-1);

    DrawRectangle(
        0,
        0,
        GetScreenWidth(),
        GetScreenHeight(),
        Fade(BLACK, OPACIDAD_FONDO_OSCURO)
    );
}


//==================================================
// DESCARGAR
//==================================================

void MenuPrincipal::Descargar()
{
    hub.Descargar();

    recursosCargados =
        false;
}
