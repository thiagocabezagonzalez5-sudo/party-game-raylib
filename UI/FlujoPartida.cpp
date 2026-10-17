#include "UI/FlujoPartida.h"

#include "Systems/Input.h"
#include "UI/EstiloUIGlobal.h"

#include "rlgl.h"

#include <cmath>
#include <cstdio>
#include <cstring>


//==================================================
// UTILIDADES DE DIBUJO
//==================================================

static float Limitar01(
    float valor
)
{
    if (valor < 0.0f)
    {
        return 0.0f;
    }

    if (valor > 1.0f)
    {
        return 1.0f;
    }

    return valor;
}


static float Suavizar(
    float u
)
{
    u = Limitar01(u);

    return u * u * (3.0f - 2.0f * u);
}


// Aparicion progresiva: 0 antes de inicio, 1 pasada la duracion.
static float Aparicion(
    float tiempo,
    float inicio,
    float duracion = 0.45f
)
{
    return Limitar01((tiempo - inicio) / duracion);
}


// Texto con salto de linea por palabras. Devuelve la Y siguiente.
static int DibujarParrafo(
    const char* texto,
    int x,
    int y,
    int ancho,
    int tamano,
    Color color
)
{
    const int largo = (int)std::strlen(texto);
    int inicio = 0;

    while (inicio < largo)
    {
        int mejor = inicio;

        for (int i = inicio; i <= largo; i++)
        {
            if (i != largo && texto[i] != ' ')
            {
                continue;
            }

            char tramo[256];
            int cantidad = i - inicio;

            if (cantidad > 255)
            {
                cantidad = 255;
            }

            std::memcpy(tramo, texto + inicio, (size_t)cantidad);
            tramo[cantidad] = '\0';

            if (
                mejor == inicio ||
                MeasureText(tramo, tamano) <= ancho
            )
            {
                mejor = i;
            }
            else
            {
                break;
            }
        }

        char linea[256];
        int cantidadLinea = mejor - inicio;

        if (cantidadLinea > 255)
        {
            cantidadLinea = 255;
        }

        std::memcpy(linea, texto + inicio, (size_t)cantidadLinea);
        linea[cantidadLinea] = '\0';

        DrawText(linea, x, y, tamano, color);

        y += tamano + 6;
        inicio = mejor + 1;
    }

    return y;
}


static const char* ObtenerEtiquetaControl(
    const Participante& participante
)
{
    return ObtenerNombreControlParticipante(participante);
}


//==================================================
// ENTRADA COMUN
//==================================================

void LectorEntradaFlujo::Reiniciar()
{
    for (int i = 0; i < 4; i++)
    {
        nivelPrevio[i] = true;
    }
}


EntradaFlujo LectorEntradaFlujo::Leer(
    const Participante participantes[],
    int cantidad
)
{
    EntradaFlujo entrada{};
    bool nivel[4] = {};
    bool hayHumano = false;

    for (int i = 0; i < cantidad; i++)
    {
        const Participante& participante = participantes[i];

        if (
            !participante.activo ||
            participante.esBot ||
            !participante.conectado
        )
        {
            continue;
        }

        hayHumano = true;

        InputSeleccionParticipante lectura =
            LeerInputSeleccionParticipante(participante);

        entrada.confirmar = entrada.confirmar || lectura.confirmar;
        entrada.cancelar = entrada.cancelar || lectura.cancelar;

        if (participante.control == CONTROL_GAMEPAD)
        {
            // Nivel (mantenido): se convierte en flanco mas abajo.
            nivel[0] = nivel[0] || lectura.izquierda;
            nivel[1] = nivel[1] || lectura.derecha;
            nivel[2] = nivel[2] || lectura.arriba;
            nivel[3] = nivel[3] || lectura.abajo;
        }
        else
        {
            entrada.izquierda = entrada.izquierda || lectura.izquierda;
            entrada.derecha = entrada.derecha || lectura.derecha;
            entrada.arriba = entrada.arriba || lectura.arriba;
            entrada.abajo = entrada.abajo || lectura.abajo;
        }
    }

    // Sin humanos conectados (por ejemplo, se desconectaron todos los
    // mandos) el teclado sigue pudiendo avanzar o salir.
    if (!hayHumano)
    {
        entrada.izquierda = IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A);
        entrada.derecha = IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D);
        entrada.arriba = IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W);
        entrada.abajo = IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S);
        entrada.confirmar = IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE);
    }

    entrada.cancelar = entrada.cancelar || IsKeyPressed(KEY_ESCAPE);

    bool* destinos[4] =
    {
        &entrada.izquierda,
        &entrada.derecha,
        &entrada.arriba,
        &entrada.abajo
    };

    for (int i = 0; i < 4; i++)
    {
        if (nivel[i] && !nivelPrevio[i])
        {
            *destinos[i] = true;
        }

        nivelPrevio[i] = nivel[i];
    }

    return entrada;
}


//==================================================
// ESTILO "CARTELES DE PLAYA" (tablero y configuracion)
//==================================================
//
// SeleccionTablero y ConfiguracionPartida se presentan como la
// preparacion de un viaje en el muelle: una carta de rumbo con los
// carteles de destino y una bitacora de zarpe con la tripulacion y la
// ruta. Usan la instancia compartida de EstiloUI; las posiciones van en
// unidades de referencia (1280x720) y se convierten con RectRef / Px.

// Color que recibe un bot al completarse la partida
// (CompletarParticipantesConBots).
static const Color COLOR_BOT_TRIPULACION = { 145, 145, 152, 255 };

// Contorno oscuro de los dibujos hechos con primitivas.
static const Color CONTORNO_DIBUJO = { 58, 36, 24, 255 };


// Punto en unidades de referencia -> pixeles.
static Vector2 PuntoRef(
    float x,
    float y
)
{
    Rectangle rect = RectRef(x, y, 0.0f, 0.0f);

    return Vector2{ rect.x, rect.y };
}


// Velo calido: deja ver la isla del HUB y oscurece hacia abajo.
static void DibujarVeloPlaya(
    float aparicion
)
{
    const Color sombra = { 24, 14, 30, 255 };

    DrawRectangleGradientV(
        0,
        0,
        GetScreenWidth(),
        GetScreenHeight(),
        Fade(sombra, 0.40f * aparicion),
        Fade(sombra, 0.72f * aparicion)
    );
}


// Titulo pintado sobre el mundo (unica excepcion al texto sobre placa).
static void DibujarTituloPlaya(
    const char* texto,
    float aparicion
)
{
    Vector2 posicion = PuntoRef(640.0f, 22.0f);

    DibujarTextoMundo(
        texto,
        posicion.x,
        posicion.y,
        UI_TEXTO_TITULO,
        Fade(UI_TEXTO_MUNDO, aparicion)
    );
}


// Rotulo pequeno de seccion sobre la placa clara.
static void DibujarRotulo(
    const char* texto,
    float x,
    float y,
    float alfa = 1.0f
)
{
    DibujarTextoTinta(texto, x, y, UI_TEXTO_PEQUENO, Fade(UI_TINTA_SUAVE, alfa));
}


// Cinta del color del tablero (no lleva texto encima).
static void DibujarCintaTema(
    float x,
    float y,
    float ancho,
    Color tema
)
{
    Rectangle cinta = { x, y, ancho, Px(6.0f) };

    DrawRectangleRec(cinta, tema);
    DrawRectangleLinesEx(cinta, 1.0f, CONTORNO_DIBUJO);
}


// Sello de goma: texto en tinta dentro de un recuadro de color.
static float MedirSello(
    const char* texto
)
{
    return (float)MeasureText(texto, TamanoTexto(UI_TEXTO_PEQUENO)) + Px(16.0f);
}


static void DibujarSello(
    const char* texto,
    float x,
    float y,
    Color borde,
    float alfa = 1.0f
)
{
    const int tamano = TamanoTexto(UI_TEXTO_PEQUENO);

    Rectangle rect =
    {
        x,
        y,
        MedirSello(texto),
        (float)tamano + Px(10.0f)
    };

    DrawRectangleRec(rect, Fade(borde, 0.20f * alfa));
    DrawRectangleLinesEx(rect, Px(2.0f), Fade(borde, alfa));

    DrawText(
        texto,
        (int)(x + Px(8.0f)),
        (int)(y + Px(5.0f)),
        tamano,
        Fade(UI_TINTA, alfa)
    );
}


// Flecha triangular. direccion: -1 izquierda, +1 derecha.
static void DibujarFlechaPlaya(
    Vector2 centro,
    float medio,
    int direccion,
    Color color
)
{
    Vector2 punta = { centro.x + direccion * medio, centro.y };
    Vector2 a = { centro.x - direccion * medio, centro.y - medio * 1.2f };
    Vector2 b = { centro.x - direccion * medio, centro.y + medio * 1.2f };

    // DrawTriangle exige sentido antihorario.
    if (direccion > 0)
    {
        DrawTriangle(a, b, punta, color);
    }
    else
    {
        DrawTriangle(b, a, punta, color);
    }
}


// Marco de madera (9-slice del estilo) alrededor de una vista 3D.
static void DibujarMarcoMadera(
    const EstiloUI& estilo,
    Rectangle rect
)
{
    DrawRectangleRec(
        Rectangle{ rect.x + Px(4.0f), rect.y + Px(5.0f), rect.width, rect.height },
        Fade(BLACK, 0.25f)
    );

    DibujarNueveCortes(estilo.madera, 12, rect, EscalaUI(), WHITE);
}


// Banderin de un tripulante: mastil de madera y bandera de su color.
// (x, y) es la punta del mastil, en pixeles.
static void DibujarBanderin(
    float x,
    float y,
    Color color,
    float alfa
)
{
    DrawLineEx(
        Vector2{ x, y },
        Vector2{ x, y + Px(56.0f) },
        Px(4.0f),
        Fade(UI_MADERA_OSCURA, alfa)
    );

    DrawCircleV(Vector2{ x, y }, Px(4.0f), Fade(UI_MADERA_OSCURA, alfa));

    Vector2 a = { x + Px(2.0f), y + Px(4.0f) };
    Vector2 b = { x + Px(2.0f), y + Px(32.0f) };
    Vector2 punta = { x + Px(42.0f), y + Px(18.0f) };

    DrawTriangle(a, b, punta, Fade(color, alfa));
    DrawTriangleLines(a, b, punta, Fade(CONTORNO_DIBUJO, alfa));
}


// Mascara (1 << ControlParticipante) de los humanos que lee
// LectorEntradaFlujo: activos, no bots y conectados.
static int ObtenerControlesAyuda(
    const Participante participantes[],
    int cantidad
)
{
    int mascara = 0;

    for (int i = 0; i < cantidad; i++)
    {
        const Participante& participante = participantes[i];

        if (participante.activo && !participante.esBot && participante.conectado)
        {
            mascara |= 1 << participante.control;
        }
    }

    return mascara;
}


// Barra de ayuda con las teclas que acepta HOY la entrada del flujo
// (LeerInputSeleccionParticipante): teclado completo confirma con ESPACIO
// o ENTER y WASD solo con ESPACIO, y ambos vuelven con RETROCESO; flechas
// usan ENTER y SHIFT DER; el mando A y B. ESC siempre vuelve. Sin humanos
// conectados el teclado acepta ENTER o ESPACIO.
//
// accionMover y accionVolver pueden ser nullptr: esa parte no se muestra
// (pantallas sin eleccion o que no vuelven atras).
static void DibujarAyudaAccionesFlujo(
    const EstiloUI& estilo,
    int controles,
    const char* accionMover,
    const char* accionConfirmar,
    const char* accionVolver
)
{
    const bool completo = (controles & (1 << CONTROL_TECLADO_COMPLETO)) != 0;
    const bool espacio =
        completo ||
        (controles & (1 << CONTROL_TECLADO_WASD));
    const bool flechas = (controles & (1 << CONTROL_TECLADO_FLECHAS)) != 0;
    const bool enter = completo || flechas;
    const bool mando = (controles & (1 << CONTROL_GAMEPAD)) != 0;

    char confirmar[64] = "";
    char volver[64] = "ESC";

    if (controles == 0)
    {
        std::snprintf(confirmar, sizeof(confirmar), "ENTER / ESPACIO");
    }
    else
    {
        std::snprintf(
            confirmar,
            sizeof(confirmar),
            "%s%s%s",
            espacio ? "ESPACIO" : "",
            enter ? (espacio ? " / ENTER" : "ENTER") : "",
            mando ? ((espacio || enter) ? " / A" : "A") : ""
        );

        std::snprintf(
            volver,
            sizeof(volver),
            "ESC%s%s%s",
            espacio ? " / RETROCESO" : "",
            flechas ? " / SHIFT DER" : "",
            mando ? " / B" : ""
        );
    }

    // Con muchos controles a la vez se acortan los separadores para que
    // el texto quepa en la barra.
    const char* separadores[2] = { "   |   ", "  |  " };
    char texto[256] = "";

    for (int i = 0; i < 2; i++)
    {
        int usado = 0;

        if (accionMover != nullptr)
        {
            usado += std::snprintf(
                texto + usado,
                sizeof(texto) - usado,
                "IZQ / DER: %s%s",
                accionMover,
                separadores[i]
            );
        }

        usado += std::snprintf(
            texto + usado,
            sizeof(texto) - usado,
            "%s: %s",
            confirmar,
            accionConfirmar
        );

        if (accionVolver != nullptr && usado < (int)sizeof(texto))
        {
            std::snprintf(
                texto + usado,
                sizeof(texto) - usado,
                "%s%s: %s",
                separadores[i],
                volver,
                accionVolver
            );
        }

        if (MeasureText(texto, TamanoTexto(UI_TEXTO_PEQUENO)) <= (int)Px(976.0f))
        {
            break;
        }
    }

    DibujarAyudaUI(estilo, texto);
}


// Ayuda de las pantallas de eleccion (tablero y configuracion).
static void DibujarAyudaFlujo(
    const EstiloUI& estilo,
    int controles,
    const char* accionMover,
    const char* accionConfirmar
)
{
    DibujarAyudaAccionesFlujo(estilo, controles, accionMover, accionConfirmar, "ATRÁS");
}


//==================================================
// SELECCION DE TABLERO
//==================================================

// Carta de rumbo (unidades de referencia).
static const Rectangle PANEL_SELECCION_TABLERO = { 40.0f, 92.0f, 1200.0f, 568.0f };
static const Rectangle MARCO_VISTA_TABLERO = { 70.0f, 120.0f, 696.0f, 404.0f };
static const Rectangle RIEL_CARTELES_TABLERO = { 70.0f, 546.0f, 696.0f, 12.0f };
static const float X_INFO_TABLERO = 796.0f;
static const float ANCHO_INFO_TABLERO = 410.0f;


static int ObtenerCantidadCartelesTablero()
{
    const int total = ObtenerCantidadTableros();

    return total < CANTIDAD_TABLEROS ? total : CANTIDAD_TABLEROS;
}


// Carteles de destino colgados del riel, centrados bajo la vista.
static Rectangle RectCartelTablero(
    int indice,
    int total
)
{
    const float separacion = 20.0f;
    float ancho = (RIEL_CARTELES_TABLERO.width - separacion * (total - 1)) / total;

    if (ancho > 300.0f)
    {
        ancho = 300.0f;
    }

    const float anchoTotal = ancho * total + separacion * (total - 1);
    const float x0 =
        RIEL_CARTELES_TABLERO.x + (RIEL_CARTELES_TABLERO.width - anchoTotal) * 0.5f;

    // El ancla de la cuerda (UI_LARGO_CUERDA arriba) cae sobre el riel.
    const float y =
        RIEL_CARTELES_TABLERO.y + RIEL_CARTELES_TABLERO.height * 0.5f + UI_LARGO_CUERDA;

    return Rectangle{ x0 + indice * (ancho + separacion), y, ancho, 50.0f };
}


static void ActualizarPresentacionTablero(
    SeleccionTablero& seleccion,
    float deltaTime
)
{
    const EstiloUI& estilo = ObtenerEstiloUI();

    ActualizarPanelUI(seleccion.panel, estilo, deltaTime);

    const int total = ObtenerCantidadCartelesTablero();

    for (int i = 0; i < total; i++)
    {
        const bool elegido = i == seleccion.indice;

        seleccion.carteles[i].activo = elegido;
        ActualizarBotonUI(seleccion.carteles[i], estilo, deltaTime, elegido);
    }

    seleccion.destelloCambio = Limitar01(seleccion.destelloCambio - deltaTime / 0.35f);
}


void SeleccionTablero::Inicializar()
{
    tiempo = 0.0f;
    confirmado = false;
    volver = false;

    if (
        indice < 0 ||
        indice >= ObtenerCantidadTableros()
    )
    {
        indice = 0;
    }

    // Presentacion: la carta cae y despues los carteles de destino.
    panel.Iniciar(PANEL_SELECCION_TABLERO, 0.0f);
    destelloCambio = 0.0f;

    const int total = ObtenerCantidadCartelesTablero();

    for (int i = 0; i < total; i++)
    {
        carteles[i].Iniciar(
            RectCartelTablero(i, total),
            ObtenerNombreTablero((IdTablero)i),
            (i % 2 == 0) ? -1.5f : 1.5f,
            0.45f + i * UI_T_ESCALONADO
        );
    }

    // Un stick mantenido al entrar no cuenta como movimiento.
    lector.Reiniciar();
}


IdTablero SeleccionTablero::ObtenerIdElegido() const
{
    return (IdTablero)indice;
}


void SeleccionTablero::Actualizar(
    float deltaTime,
    const Participante participantes[],
    int cantidad,
    AudioJuego& audio
)
{
    tiempo += deltaTime;

    ActualizarPresentacionTablero(*this, deltaTime);

    controlesAyuda = ObtenerControlesAyuda(participantes, cantidad);

    EntradaFlujo entrada = lector.Leer(participantes, cantidad);

    if (tiempo < BLOQUEO_ENTRADA_FLUJO)
    {
        return;
    }

    if (entrada.cancelar)
    {
        volver = true;
        audio.ReproducirSonido(SONIDO_UI_CANCELAR);
        return;
    }

    if (entrada.confirmar)
    {
        confirmado = true;

        if (indice >= 0 && indice < ObtenerCantidadCartelesTablero())
        {
            carteles[indice].colgado.Pulsar();
        }

        audio.ReproducirSonido(SONIDO_UI_CONFIRMAR);
        return;
    }

    int cambio =
        ((entrada.derecha || entrada.abajo) ? 1 : 0) -
        ((entrada.izquierda || entrada.arriba) ? 1 : 0);

    int total = ObtenerCantidadTableros();

    if (cambio != 0 && total > 1)
    {
        indice = (indice + cambio + total) % total;
        destelloCambio = 1.0f;
        audio.ReproducirSonido(SONIDO_UI_MOVER);
    }
}


void SeleccionTablero::Dibujar() const
{
    const EstiloUI& estilo = ObtenerEstiloUI();

    const IdTablero id = ObtenerIdElegido();
    const Color tema = ObtenerColorTemaTablero(id);
    const float aparicion = Aparicion(tiempo, 0.0f, 0.30f);
    const int total = ObtenerCantidadCartelesTablero();

    DibujarVeloPlaya(aparicion);
    DibujarTituloPlaya("ELEGI EL DESTINO", aparicion);

    DibujarPanelUI(estilo, panel);

    if (!estilo.cargado || panel.colgado.retraso > 0.0f)
    {
        return;
    }

    // Todo el contenido acompana la caida y el rebote de la carta.
    const float dy = panel.colgado.caida.valor;

    //------------------------------
    // VISTA DEL DESTINO (catalejo)
    //------------------------------

    Rectangle marco = RectRef(
        MARCO_VISTA_TABLERO.x,
        MARCO_VISTA_TABLERO.y + dy,
        MARCO_VISTA_TABLERO.width,
        MARCO_VISTA_TABLERO.height
    );

    DibujarMarcoMadera(estilo, marco);

    Rectangle vista = RectRef(
        MARCO_VISTA_TABLERO.x + 12.0f,
        MARCO_VISTA_TABLERO.y + 12.0f + dy,
        MARCO_VISTA_TABLERO.width - 24.0f,
        MARCO_VISTA_TABLERO.height - 24.0f
    );

    DrawRectangleRec(vista, BLACK);
    DibujarVistaPreviaTablero(id, vista, tiempo);

    // Destello del color del destino al cambiar de tablero.
    if (destelloCambio > 0.0f)
    {
        DrawRectangleRec(vista, Fade(tema, 0.35f * destelloCambio));
    }

    if (total > 1)
    {
        const float centroY = vista.y + vista.height * 0.5f;
        const float radio = Px(20.0f);

        for (int lado = -1; lado <= 1; lado += 2)
        {
            Vector2 centro =
            {
                lado < 0 ? vista.x + Px(30.0f) : vista.x + vista.width - Px(30.0f),
                centroY
            };

            DrawCircleV(centro, radio, UI_PLACA_CLARA);
            DrawCircleLinesV(centro, radio, CONTORNO_DIBUJO);
            DibujarFlechaPlaya(
                Vector2{ centro.x + lado * Px(2.0f), centro.y },
                Px(8.0f),
                lado,
                UI_TINTA
            );
        }
    }

    //------------------------------
    // CARTELES DE DESTINO
    //------------------------------

    DibujarNueveCortes(
        estilo.madera,
        12,
        RectRef(
            RIEL_CARTELES_TABLERO.x,
            RIEL_CARTELES_TABLERO.y + dy,
            RIEL_CARTELES_TABLERO.width,
            RIEL_CARTELES_TABLERO.height
        ),
        EscalaUI(),
        WHITE
    );

    for (int i = 0; i < total; i++)
    {
        BotonUI cartel = carteles[i];
        cartel.ref.y += dy;

        DibujarBotonUI(estilo, cartel, i == indice);
    }

    //------------------------------
    // BITACORA DEL DESTINO
    //------------------------------

    Vector2 origen = PuntoRef(X_INFO_TABLERO, 122.0f + dy);
    const float x = origen.x;
    const int anchoTexto = (int)Px(ANCHO_INFO_TABLERO);
    const int tamanoTexto = TamanoTexto(UI_TEXTO);
    float y = origen.y;

    DibujarRotulo(TextFormat("DESTINO %d DE %d", indice + 1, ObtenerCantidadTableros()), x, y);
    y += Px(22.0f);

    DibujarTextoTinta(ObtenerNombreTablero(id), x, y, UI_TEXTO_GRANDE, UI_TINTA);
    y += TamanoTexto(UI_TEXTO_GRANDE) + Px(8.0f);

    DibujarCintaTema(x, y, Px(ANCHO_INFO_TABLERO), tema);
    y += Px(20.0f);

    y = (float)DibujarParrafo(
        ObtenerHistoriaTablero(id),
        (int)x,
        (int)y,
        anchoTexto,
        tamanoTexto,
        UI_TINTA
    );

    y += Px(12.0f);
    DibujarRotulo("OBJETIVO", x, y);
    y += Px(22.0f);

    y = (float)DibujarParrafo(
        ObtenerObjetivoTablero(id),
        (int)x,
        (int)y,
        anchoTexto,
        tamanoTexto,
        UI_TINTA
    );

    y += Px(12.0f);
    DibujarRotulo("GIMMICK", x, y);
    y += Px(22.0f);

    y = (float)DibujarParrafo(
        ObtenerGimmickTablero(id),
        (int)x,
        (int)y,
        anchoTexto,
        tamanoTexto,
        UI_TINTA
    );

    y += Px(12.0f);
    DibujarRotulo("DIFICULTAD", x, y);
    y += Px(22.0f);

    DibujarSello(ObtenerDificultadTablero(id), x, y, tema);

    if (controlesAyuda >= 0)
    {
        DibujarAyudaFlujo(estilo, controlesAyuda, "CAMBIAR DESTINO", "ELEGIR");
    }
}


//==================================================
// CONFIGURACION DE PARTIDA
//==================================================

// Bitacora de zarpe (unidades de referencia).
static const Rectangle PANEL_CONFIGURACION = { 40.0f, 92.0f, 1200.0f, 500.0f };
static const float X_TRIPULACION = 76.0f;
static const float X_FIN_TRIPULACION = 610.0f;
static const float X_DIVISOR = 638.0f;
static const float X_VIAJE = 664.0f;
static const float ANCHO_VIAJE = 550.0f;

// Ruta del muelle a la isla; las boyas marcan las rondas.
static const float X_MUELLE = 676.0f;
static const float X_INICIO_RUTA = 720.0f;
static const float X_FIN_RUTA = 1144.0f;
static const float X_ISLA = 1186.0f;
static const float Y_AGUA_RUTA = 450.0f;

// El barco sale del muelle cuando la bitacora ya cayo.
static const float RETRASO_BARCO = 0.45f;


static const char* NOMBRES_DURACION[CANTIDAD_OPCIONES_RONDAS] =
{
    "CORTA",
    "MEDIA",
    "LARGA"
};


static float XBoyaRondas(
    int opcion
)
{
    const float maximo =
        (float)OPCIONES_RONDAS_PARTIDA[CANTIDAD_OPCIONES_RONDAS - 1];

    return
        X_INICIO_RUTA +
        (X_FIN_RUTA - X_INICIO_RUTA) * OPCIONES_RONDAS_PARTIDA[opcion] / maximo;
}


static void ActualizarPresentacionConfiguracion(
    ConfiguracionPartida& configuracion,
    float deltaTime
)
{
    const EstiloUI& estilo = ObtenerEstiloUI();

    ActualizarPanelUI(configuracion.panel, estilo, deltaTime);
    ActualizarSelectorUI(configuracion.selectorRondas, estilo, deltaTime);
    ActualizarBotonUI(configuracion.cartelZarpar, estilo, deltaTime, false);

    // El barco amarra justo antes de la boya elegida.
    const float objetivo = XBoyaRondas(configuracion.opcionRondas) - 40.0f;

    if (estilo.reducirMovimiento)
    {
        configuracion.barco = ResorteUI{};
        configuracion.barco.valor = objetivo;
        return;
    }

    if (configuracion.tiempo >= RETRASO_BARCO)
    {
        // Amortiguacion critica: llega a la boya sin pasarse.
        configuracion.barco.Actualizar(objetivo, deltaTime, 30.0f, 11.0f);
    }
}


// Barco de vela pequeno. base = centro de la linea de flotacion (px).
static void DibujarBarco(
    Vector2 base,
    float angulo,
    Color bandera
)
{
    const float u = EscalaUI();

    rlPushMatrix();
    rlTranslatef(base.x, base.y, 0.0f);
    rlRotatef(angulo, 0.0f, 0.0f, 1.0f);

    // Casco (trapecio).
    Vector2 arribaIzq = { -22.0f * u, -10.0f * u };
    Vector2 arribaDer = { 22.0f * u, -10.0f * u };
    Vector2 abajoIzq = { -15.0f * u, 0.0f };
    Vector2 abajoDer = { 15.0f * u, 0.0f };

    DrawTriangle(arribaIzq, abajoIzq, abajoDer, UI_MADERA_MEDIA);
    DrawTriangle(arribaIzq, abajoDer, arribaDer, UI_MADERA_MEDIA);
    DrawLineEx(arribaIzq, arribaDer, 2.0f * u, CONTORNO_DIBUJO);
    DrawLineEx(arribaIzq, abajoIzq, 2.0f * u, CONTORNO_DIBUJO);
    DrawLineEx(abajoIzq, abajoDer, 2.0f * u, CONTORNO_DIBUJO);
    DrawLineEx(abajoDer, arribaDer, 2.0f * u, CONTORNO_DIBUJO);

    // Mastil, vela y bandera del destino.
    DrawLineEx(
        Vector2{ 0.0f, -10.0f * u },
        Vector2{ 0.0f, -46.0f * u },
        3.0f * u,
        UI_MADERA_OSCURA
    );

    Vector2 velaA = { 3.0f * u, -42.0f * u };
    Vector2 velaB = { 3.0f * u, -14.0f * u };
    Vector2 velaC = { 24.0f * u, -14.0f * u };

    DrawTriangle(velaA, velaB, velaC, UI_PLACA_CLARA);
    DrawTriangleLines(velaA, velaB, velaC, CONTORNO_DIBUJO);

    Vector2 banderaA = { -2.0f * u, -46.0f * u };
    Vector2 banderaB = { -2.0f * u, -37.0f * u };
    Vector2 banderaC = { -15.0f * u, -41.5f * u };

    DrawTriangle(banderaB, banderaA, banderaC, bandera);

    rlPopMatrix();
}


// Boya roja y blanca. centro en pixeles.
static void DibujarBoya(
    Vector2 centro,
    float radio,
    bool elegida
)
{
    if (elegida)
    {
        DrawCircleV(centro, radio + Px(5.0f), Fade(UI_FOCO, 0.55f));
    }

    DrawCircleV(centro, radio, RAYWHITE);
    DrawCircleSector(centro, radio, 180.0f, 360.0f, 16, UI_PELIGRO);
    DrawCircleLinesV(centro, radio, CONTORNO_DIBUJO);
}


// Isla de destino con la bandera del tablero.
static void DibujarIslaDestino(
    Vector2 base,
    Color tema
)
{
    const float u = EscalaUI();

    DrawCircleSector(base, 26.0f * u, 180.0f, 360.0f, 24, Color{ 196, 154, 92, 255 });
    DrawCircleSectorLines(base, 26.0f * u, 180.0f, 360.0f, 24, CONTORNO_DIBUJO);
    DrawCircleSector(
        Vector2{ base.x + 4.0f * u, base.y },
        15.0f * u,
        180.0f,
        360.0f,
        16,
        Color{ 88, 160, 92, 255 }
    );
    DrawLineEx(
        Vector2{ base.x - 30.0f * u, base.y },
        Vector2{ base.x + 30.0f * u, base.y },
        2.0f * u,
        CONTORNO_DIBUJO
    );

    // Mastil con la bandera del destino.
    Vector2 punta = { base.x - 8.0f * u, base.y - 50.0f * u };

    DrawLineEx(
        Vector2{ punta.x, base.y - 14.0f * u },
        punta,
        2.5f * u,
        UI_MADERA_OSCURA
    );

    Vector2 a = { punta.x + 1.0f * u, punta.y };
    Vector2 b = { punta.x + 1.0f * u, punta.y + 12.0f * u };
    Vector2 c = { punta.x + 20.0f * u, punta.y + 6.0f * u };

    DrawTriangle(a, b, c, tema);
    DrawTriangleLines(a, b, c, CONTORNO_DIBUJO);
}


void ConfiguracionPartida::Inicializar(
    IdTablero idTablero
)
{
    tablero = idTablero;
    tiempo = 0.0f;
    confirmado = false;
    volver = false;

    if (
        opcionRondas < 0 ||
        opcionRondas >= CANTIDAD_OPCIONES_RONDAS
    )
    {
        opcionRondas = 0;
    }

    // Presentacion: la bitacora cae, despues el selector y el cartel de
    // zarpe; el barco espera en el muelle.
    panel.Iniciar(PANEL_CONFIGURACION, 0.0f);

    selectorRondas.Iniciar(
        Rectangle{ X_VIAJE, 510.0f, ANCHO_VIAJE, UI_ALTO_FILA },
        "RONDAS",
        0.45f
    );

    // Cuelga de DETRAS de la bitacora: el ancla queda oculta tras el marco
    // inferior (activo lo baja otras 6 unidades) y el cartel asoma por
    // debajo. Asi deja ~18 unidades de aire sobre la barra de ayuda (674).
    const float yCartel =
        PANEL_CONFIGURACION.y + PANEL_CONFIGURACION.height - 20.0f + UI_LARGO_CUERDA;

    cartelZarpar.Iniciar(
        Rectangle{ X_VIAJE + ANCHO_VIAJE * 0.5f - 110.0f, yCartel, 220.0f, 44.0f },
        "ZARPAR",
        1.5f,
        0.55f
    );
    cartelZarpar.activo = true;

    // Balanceo de llegada mas corto: la esquina baja no se acerca a la ayuda.
    cartelZarpar.colgado.intensidad = 0.5f;

    barco = ResorteUI{};
    barco.valor = X_MUELLE + 40.0f;    // amarrado al muelle

    lector.Reiniciar();
}


int ConfiguracionPartida::ObtenerRondas() const
{
    return OPCIONES_RONDAS_PARTIDA[opcionRondas];
}


void ConfiguracionPartida::Actualizar(
    float deltaTime,
    const Participante participantes[],
    int cantidad,
    AudioJuego& audio
)
{
    tiempo += deltaTime;

    ActualizarPresentacionConfiguracion(*this, deltaTime);

    EntradaFlujo entrada = lector.Leer(participantes, cantidad);

    if (tiempo < BLOQUEO_ENTRADA_FLUJO)
    {
        return;
    }

    if (entrada.cancelar)
    {
        volver = true;
        audio.ReproducirSonido(SONIDO_UI_CANCELAR);
        return;
    }

    if (entrada.confirmar)
    {
        confirmado = true;
        cartelZarpar.colgado.Pulsar();
        audio.ReproducirSonido(SONIDO_UI_CONFIRMAR);
        return;
    }

    int cambio =
        ((entrada.derecha || entrada.abajo) ? 1 : 0) -
        ((entrada.izquierda || entrada.arriba) ? 1 : 0);

    if (cambio != 0)
    {
        opcionRondas =
            (opcionRondas + cambio + CANTIDAD_OPCIONES_RONDAS) %
            CANTIDAD_OPCIONES_RONDAS;

        GolpearSelectorUI(selectorRondas, cambio);
        audio.ReproducirSonido(SONIDO_UI_MOVER);
    }
}


void ConfiguracionPartida::Dibujar(
    const Participante participantes[],
    int cantidad
) const
{
    const EstiloUI& estilo = ObtenerEstiloUI();

    const Color tema = ObtenerColorTemaTablero(tablero);
    const float aparicion = Aparicion(tiempo, 0.0f, 0.30f);

    DibujarVeloPlaya(aparicion);
    DibujarTituloPlaya("PREPARAR EL VIAJE", aparicion);

    // Cartel de zarpe ANTES que la bitacora: durante la entrada sale de
    // detras de ella sin taparla. Solo acompana a la bitacora hacia arriba;
    // el rebote hacia abajo lo cubre la propia bitacora y no lo acerca a
    // la barra de ayuda.
    BotonUI zarpar = cartelZarpar;
    zarpar.ref.y += std::fmin(panel.colgado.caida.valor, 0.0f);

    DibujarBotonUI(estilo, zarpar, false);

    DibujarPanelUI(estilo, panel);

    if (!estilo.cargado || panel.colgado.retraso > 0.0f)
    {
        return;
    }

    // Todo el contenido acompana la caida y el rebote de la bitacora.
    const float dy = panel.colgado.caida.valor;

    //------------------------------
    // TRIPULACION (solo informacion: se elige con los personajes)
    //------------------------------

    int humanos = 0;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (i < cantidad && participantes[i].activo && !participantes[i].esBot)
        {
            humanos++;
        }
    }

    const int bots = MAX_PARTICIPANTES - humanos;

    Vector2 cabecera = PuntoRef(X_TRIPULACION, 120.0f + dy);

    DibujarTextoTinta("TRIPULACIÓN", cabecera.x, cabecera.y, UI_TEXTO_GRANDE, UI_TINTA);

    const char* resumen = TextFormat(
        "%d %s  /  %d %s",
        humanos,
        humanos == 1 ? "HUMANO" : "HUMANOS",
        bots,
        bots == 1 ? "BOT" : "BOTS"
    );

    const int tamanoPequeno = TamanoTexto(UI_TEXTO_PEQUENO);
    Vector2 finCabecera = PuntoRef(X_FIN_TRIPULACION, 128.0f + dy);

    DibujarRotulo(
        resumen,
        finCabecera.x - (float)MeasureText(resumen, tamanoPequeno),
        finCabecera.y
    );

    const float altoFila = 88.0f;
    const float yPrimeraFila = 166.0f;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        const bool humano =
            i < cantidad && participantes[i].activo && !participantes[i].esBot;

        const bool desconectado = humano && !participantes[i].conectado;

        // Las filas se pegan una a una en la bitacora (solo opacidad).
        const float alfa = estilo.reducirMovimiento
            ? 1.0f
            : Aparicion(tiempo, 0.35f + i * UI_T_ESCALONADO, 0.25f);

        const float yFila = yPrimeraFila + i * altoFila + dy;

        Vector2 mastil = PuntoRef(X_TRIPULACION + 22.0f, yFila + 14.0f);

        DibujarBanderin(
            mastil.x,
            mastil.y,
            humano ? participantes[i].color : COLOR_BOT_TRIPULACION,
            alfa
        );

        Vector2 nombre = PuntoRef(X_TRIPULACION + 84.0f, yFila + 12.0f);

        DibujarTextoTinta(
            TextFormat("J%d", i + 1),
            nombre.x,
            nombre.y,
            UI_TEXTO_GRANDE,
            Fade(UI_TINTA, alfa)
        );

        // Rol en tinta y detalle (control o puesto libre) en tinta suave.
        Vector2 rol = PuntoRef(X_TRIPULACION + 84.0f, yFila + 50.0f);
        const char* textoRol = humano ? "HUMANO" : "BOT";
        const char* detalle =
            desconectado ? "SIN CONEXIÓN: PASARÁ A BOT"
            : (humano ? ObtenerEtiquetaControl(participantes[i]) : "PUESTO LIBRE");

        DibujarTextoTinta(textoRol, rol.x, rol.y, UI_TEXTO, Fade(UI_TINTA, alfa));

        DibujarTextoTinta(
            detalle,
            rol.x + (float)MeasureText(textoRol, TamanoTexto(UI_TEXTO)) + Px(14.0f),
            rol.y + Px(3.0f),
            UI_TEXTO_PEQUENO,
            Fade(UI_TINTA_SUAVE, alfa)
        );

        // Sello de estado a la derecha.
        const char* sello =
            desconectado ? "SIN CONEXIÓN" : (humano ? "A BORDO" : "BOT");

        const Color colorSello =
            desconectado ? UI_PELIGRO : (humano ? UI_POSITIVO : UI_MADERA_MEDIA);

        Vector2 finFila = PuntoRef(X_FIN_TRIPULACION, yFila + 24.0f);

        DibujarSello(sello, finFila.x - MedirSello(sello), finFila.y, colorSello, alfa);

        // Separador entre tripulantes.
        if (i < MAX_PARTICIPANTES - 1)
        {
            Vector2 a = PuntoRef(X_TRIPULACION, yFila + altoFila - 6.0f);
            Vector2 b = PuntoRef(X_FIN_TRIPULACION, yFila + altoFila - 6.0f);

            DrawLineEx(a, b, Px(1.5f), Fade(UI_TINTA_SUAVE, 0.45f * alfa));
        }
    }

    Vector2 nota = PuntoRef(X_TRIPULACION, 524.0f + dy);

    DibujarRotulo("LA TRIPULACIÓN SE ARMA AL ELEGIR PERSONAJES", nota.x, nota.y);

    // Divisor: una cuerda vertical entre tripulacion y viaje.
    DibujarCuerdaUI(
        estilo,
        PuntoRef(X_DIVISOR, 116.0f + dy),
        PuntoRef(X_DIVISOR, 570.0f + dy)
    );

    //------------------------------
    // DESTINO
    //------------------------------

    Vector2 destino = PuntoRef(X_VIAJE, 120.0f + dy);

    DibujarRotulo("DESTINO", destino.x, destino.y);
    DibujarTextoTinta(
        ObtenerNombreTablero(tablero),
        destino.x,
        destino.y + Px(20.0f),
        UI_TEXTO_GRANDE,
        UI_TINTA
    );

    DibujarCintaTema(
        destino.x,
        destino.y + Px(20.0f) + TamanoTexto(UI_TEXTO_GRANDE) + Px(6.0f),
        Px(ANCHO_VIAJE),
        tema
    );

    Rectangle marco = RectRef(X_VIAJE, 190.0f + dy, ANCHO_VIAJE, 162.0f);

    DibujarMarcoMadera(estilo, marco);

    Rectangle vista = RectRef(X_VIAJE + 10.0f, 200.0f + dy, ANCHO_VIAJE - 20.0f, 142.0f);

    DrawRectangleRec(vista, BLACK);
    DibujarVistaPreviaTablero(tablero, vista, tiempo);

    //------------------------------
    // RUTA: muelle -> boyas (rondas) -> isla
    //------------------------------

    Vector2 rotuloRuta = PuntoRef(X_VIAJE, 364.0f + dy);

    DibujarRotulo("DURACIÓN DEL VIAJE", rotuloRuta.x, rotuloRuta.y);

    const float yAgua = Y_AGUA_RUTA + dy;

    // Muelle: un tablon del estilo.
    DibujarNueveCortes(
        estilo.madera,
        12,
        RectRef(X_MUELLE - 16.0f, yAgua - 6.0f, 44.0f, 20.0f),
        EscalaUI(),
        WHITE
    );

    // La cuerda de la ruta une el muelle con la isla.
    DibujarCuerdaUI(
        estilo,
        PuntoRef(X_MUELLE + 28.0f, yAgua + 4.0f),
        PuntoRef(X_ISLA - 26.0f, yAgua + 4.0f)
    );

    const int tamanoNumero = TamanoTexto(UI_TEXTO);

    for (int i = 0; i < CANTIDAD_OPCIONES_RONDAS; i++)
    {
        const bool elegida = i == opcionRondas;
        Vector2 centro = PuntoRef(XBoyaRondas(i), yAgua + 4.0f);

        DibujarBoya(centro, Px(elegida ? 10.0f : 8.0f), elegida);

        const char* numero = TextFormat("%d", OPCIONES_RONDAS_PARTIDA[i]);

        DrawText(
            numero,
            (int)(centro.x - MeasureText(numero, tamanoNumero) * 0.5f),
            (int)(centro.y + Px(16.0f)),
            tamanoNumero,
            elegida ? UI_TINTA : UI_TINTA_SUAVE
        );
    }

    DibujarIslaDestino(PuntoRef(X_ISLA, yAgua + 4.0f), tema);

    // El barco se inclina un poco mientras navega y se endereza al llegar.
    float inclinacion = -barco.velocidad * 0.05f;

    if (inclinacion > 10.0f)
    {
        inclinacion = 10.0f;
    }
    else if (inclinacion < -10.0f)
    {
        inclinacion = -10.0f;
    }

    DibujarBarco(PuntoRef(barco.valor, yAgua - 6.0f), inclinacion, tema);

    //------------------------------
    // SELECTOR DE RONDAS (el cartel de zarpe se dibuja antes del panel)
    //------------------------------

    SelectorUI selector = selectorRondas;
    selector.ref.y += dy;

    DibujarSelectorUI(
        estilo,
        selector,
        TextFormat("%d - %s", ObtenerRondas(), NOMBRES_DURACION[opcionRondas]),
        true
    );

    DibujarAyudaFlujo(
        estilo,
        ObtenerControlesAyuda(participantes, cantidad),
        "RONDAS",
        "ZARPAR"
    );
}


//==================================================
// INTRO DEL TABLERO
//==================================================
//
// "Tierra a la vista": el mapa ocupa toda la pantalla, el nombre va
// pintado sobre el mundo (con contorno, legible sobre cualquier mapa) y
// la bitacora del viaje se apoya al pie. El barco de la ruta marca el
// tiempo que falta para que la presentacion termine sola.

// Bitacora al pie del mapa (unidades de referencia). Termina por encima
// de la barra de ayuda (674).
static const Rectangle TARJETA_INTRO = { 120.0f, 420.0f, 1040.0f, 236.0f };
static const float X_HISTORIA_INTRO = 146.0f;
static const float ANCHO_HISTORIA_INTRO = 536.0f;
static const float X_DIVISOR_INTRO = 710.0f;
static const float X_DATOS_INTRO = 736.0f;
static const float ANCHO_DATOS_INTRO = 398.0f;

// Ruta del barco: del muelle a la isla, en el borde inferior de la bitacora.
static const float X_MUELLE_INTRO = 162.0f;
static const float X_ISLA_INTRO = 1108.0f;
static const float Y_AGUA_INTRO = 632.0f;


void IntroTablero::Inicializar(
    IdTablero idTablero,
    int cantidadRondas
)
{
    tablero = idTablero;
    rondas = cantidadRondas;
    tiempo = 0.0f;
    terminada = false;
    controlesAyuda = -1;

    // La bitacora asoma despues de que aparece el mapa.
    bitacora.Iniciar(TARJETA_INTRO, nullptr, 0.15f);

    lector.Reiniciar();
}


void IntroTablero::Actualizar(
    float deltaTime,
    const Participante participantes[],
    int cantidad
)
{
    tiempo += deltaTime;

    ActualizarTarjetaUI(bitacora, ObtenerEstiloUI(), deltaTime);

    controlesAyuda = ObtenerControlesAyuda(participantes, cantidad);

    EntradaFlujo entrada = lector.Leer(participantes, cantidad);

    if (tiempo >= DURACION)
    {
        terminada = true;
        return;
    }

    // Saltar solo despues del bloqueo, para que una pulsacion
    // heredada no se salte la presentacion. Cancelar (ESC / B / ...) no
    // hace nada: la partida ya esta preparada, no hay pantalla anterior a
    // la que volver y la confirmacion de salida vive en el tablero.
    if (
        tiempo >= BLOQUEO_ENTRADA_FLUJO + 0.2f &&
        entrada.confirmar
    )
    {
        terminada = true;
    }
}


void IntroTablero::Dibujar() const
{
    const EstiloUI& estilo = ObtenerEstiloUI();

    const int ancho = GetScreenWidth();
    const int alto = GetScreenHeight();
    const Color tema = ObtenerColorTemaTablero(tablero);
    const bool reducir = estilo.reducirMovimiento;

    // Vista del mapa a pantalla completa.
    DibujarVistaPreviaTablero(
        tablero,
        Rectangle{ 0.0f, 0.0f, (float)ancho, (float)alto },
        tiempo
    );

    // Sombra suave solo arriba, detras del nombre (no a pantalla completa).
    const float aparicion = Aparicion(tiempo, 0.0f, 0.30f);
    const int altoSombra = (int)PuntoRef(0.0f, 210.0f).y;

    DrawRectangleGradientV(
        0,
        0,
        ancho,
        altoSombra,
        Fade(UI_VELO, 0.50f * aparicion),
        Fade(UI_VELO, 0.0f)
    );

    // Con reducirMovimiento todo aparece de una vez y sin desplazarse.
    const float aNombre = reducir ? 1.0f : Aparicion(tiempo, 0.3f);
    const float aHistoria = reducir ? 1.0f : Aparicion(tiempo, 1.0f);
    const float aObjetivo = reducir ? 1.0f : Aparicion(tiempo, 2.2f);
    const float aGimmick = reducir ? 1.0f : Aparicion(tiempo, 3.2f);

    //------------------------------
    // NOMBRE DEL DESTINO (sobre el mundo)
    //------------------------------

    const float bajada = reducir ? 0.0f : -(1.0f - Suavizar(aNombre)) * 24.0f;

    if (aNombre > 0.0f)
    {
        const char* nombre = ObtenerNombreTablero(tablero);
        const int tamanoNombre = TamanoTexto(UI_TEXTO_TITULO);

        OpcionesEtiquetaUI etiqueta;
        etiqueta.borde = tema;
        etiqueta.alfa = aNombre;

        DibujarEtiquetaUI("PRÓXIMO DESTINO", PuntoRef(640.0f, 34.0f + bajada), -2.0f, etiqueta);

        Vector2 posicion = PuntoRef(640.0f, 54.0f + bajada);

        DibujarTextoMundoContornoUI(
            nombre,
            posicion.x,
            posicion.y,
            UI_TEXTO_TITULO,
            UI_TEXTO_MUNDO,
            aNombre
        );

        // Cinta del color del tablero bajo el nombre.
        const float anchoCinta = (float)MeasureText(nombre, tamanoNombre);

        Rectangle cinta =
        {
            posicion.x - anchoCinta * 0.5f,
            posicion.y + tamanoNombre + Px(10.0f),
            anchoCinta,
            Px(7.0f)
        };

        DrawRectangleRec(cinta, Fade(tema, aNombre));
        DrawRectangleLinesEx(cinta, Px(1.5f), Fade(CONTORNO_DIBUJO, aNombre));

        DibujarEtiquetaUI(
            TextFormat("%d RONDAS", rondas),
            PuntoRef(640.0f, 148.0f + bajada),
            2.0f,
            etiqueta
        );
    }

    //------------------------------
    // BITACORA DEL VIAJE
    //------------------------------

    DibujarTarjetaUI(estilo, bitacora);

    if (estilo.cargado && bitacora.colgado.retraso <= 0.0f)
    {
        const float dy = bitacora.colgado.caida.valor;

        // Historia a la izquierda.
        Vector2 historia = PuntoRef(X_HISTORIA_INTRO, 438.0f + dy);

        DibujarRotulo("HISTORIA", historia.x, historia.y, aHistoria);

        DibujarParrafoUI(
            ObtenerHistoriaTablero(tablero),
            historia.x,
            historia.y + Px(22.0f),
            UI_TEXTO,
            Px(ANCHO_HISTORIA_INTRO),
            Fade(UI_TINTA, aHistoria),
            UI_ALINEAR_IZQUIERDA,
            Px(110.0f)
        );

        // Divisor: una cuerda vertical entre historia y reglas.
        DibujarCuerdaUI(
            estilo,
            PuntoRef(X_DIVISOR_INTRO, 436.0f + dy),
            PuntoRef(X_DIVISOR_INTRO, 572.0f + dy)
        );

        // Objetivo y gimmick a la derecha.
        Vector2 objetivo = PuntoRef(X_DATOS_INTRO, 436.0f + dy);

        DibujarRotulo("OBJETIVO", objetivo.x, objetivo.y, aObjetivo);

        DibujarParrafoUI(
            ObtenerObjetivoTablero(tablero),
            objetivo.x,
            objetivo.y + Px(20.0f),
            UI_TEXTO,
            Px(ANCHO_DATOS_INTRO),
            Fade(UI_TINTA, aObjetivo),
            UI_ALINEAR_IZQUIERDA,
            Px(46.0f)
        );

        Vector2 gimmick = PuntoRef(X_DATOS_INTRO, 510.0f + dy);

        DibujarRotulo("GIMMICK", gimmick.x, gimmick.y, aGimmick);

        DibujarParrafoUI(
            ObtenerGimmickTablero(tablero),
            gimmick.x,
            gimmick.y + Px(20.0f),
            UI_TEXTO,
            Px(ANCHO_DATOS_INTRO),
            Fade(UI_TINTA, aGimmick),
            UI_ALINEAR_IZQUIERDA,
            Px(42.0f)
        );

        //------------------------------
        // RUTA: el barco avanza con el tiempo de la presentacion
        //------------------------------

        const float yAgua = Y_AGUA_INTRO + dy;

        DibujarNueveCortes(
            estilo.madera,
            12,
            RectRef(X_MUELLE_INTRO - 16.0f, yAgua - 4.0f, 44.0f, 14.0f),
            EscalaUI(),
            WHITE
        );

        DibujarCuerdaUI(
            estilo,
            PuntoRef(X_MUELLE_INTRO + 28.0f, yAgua + 4.0f),
            PuntoRef(X_ISLA_INTRO - 26.0f, yAgua + 4.0f)
        );

        DibujarIslaDestino(PuntoRef(X_ISLA_INTRO, yAgua + 4.0f), tema);

        const float progreso = Limitar01(tiempo / DURACION);
        const float xBarco =
            (X_MUELLE_INTRO + 40.0f) +
            ((X_ISLA_INTRO - 48.0f) - (X_MUELLE_INTRO + 40.0f)) * progreso;

        // Cabeceo leve mientras navega (quieto con reducirMovimiento).
        const float cabeceo = reducir ? 0.0f : std::sin(tiempo * 3.0f) * 3.0f;

        DibujarBarco(PuntoRef(xBarco, yAgua - 6.0f), cabeceo, tema);
    }

    if (controlesAyuda >= 0)
    {
        DibujarAyudaAccionesFlujo(estilo, controlesAyuda, nullptr, "SALTAR", nullptr);
    }
}


//==================================================
// ORDEN DE TURNO
//==================================================
//
// "Sorteo en el muelle": un tablon con una ficha por jugador. Cada uno
// tira su dado; al revelar, las fichas se reordenan por turno, reciben
// el sello de su puesto y la primera queda con el marco de foco.

static const float INICIO_TIRADA_ORDEN = 0.7f;

static const Rectangle PANEL_ORDEN = { 140.0f, 92.0f, 1000.0f, 490.0f };
static const float ANCHO_FICHA_ORDEN = 200.0f;
static const float ALTO_FICHA_ORDEN = 270.0f;
static const float SEPARACION_FICHA_ORDEN = 24.0f;
static const float Y_FICHA_ORDEN = 162.0f;

// Cara del dado: un blanco calido, mas claro que la placa.
static const Color COLOR_CARA_DADO = { 255, 250, 238, 255 };

// Medallas por puesto (las mismas del tablon final de los minijuegos).
static const Color COLORES_MEDALLA[MAX_PARTICIPANTES] =
{
    { 255, 200, 70, 255 },      // oro (UI_FOCO)
    { 206, 208, 216, 255 },     // plata
    { 220, 156, 104, 255 },     // bronce
    { 196, 170, 130, 255 }      // madera clara
};


static Color ObtenerColorMedalla(
    int puesto
)
{
    if (puesto < 1 || puesto > MAX_PARTICIPANTES)
    {
        return COLORES_MEDALLA[MAX_PARTICIPANTES - 1];
    }

    return COLORES_MEDALLA[puesto - 1];
}


static float ObtenerTiempoDetencionDado(
    int posicionTirador
)
{
    return 1.5f + 0.55f * (float)posicionTirador;
}


static float ObtenerTiempoRevelarOrden(
    int cantidad
)
{
    return ObtenerTiempoDetencionDado(cantidad - 1) + 0.8f;
}


// X (unidades de referencia) de la ficha en la posicion dada (puede ser
// fraccionaria mientras las fichas se reordenan).
static float XFichaOrden(
    float posicion,
    int cantidad
)
{
    const float total =
        cantidad * ANCHO_FICHA_ORDEN + (cantidad - 1) * SEPARACION_FICHA_ORDEN;

    return 640.0f - total * 0.5f + posicion * (ANCHO_FICHA_ORDEN + SEPARACION_FICHA_ORDEN);
}


// Dado de madera clara. centro en pixeles; lado en unidades de referencia.
// valor 0 = todavia sin tirar ("?").
static void DibujarDadoOrden(
    Vector2 centro,
    float ladoRef,
    float angulo,
    float escala,
    int valor
)
{
    const float lado = Px(ladoRef) * escala;
    const float medio = lado * 0.5f;

    rlPushMatrix();
    rlTranslatef(centro.x, centro.y, 0.0f);
    rlRotatef(angulo, 0.0f, 0.0f, 1.0f);

    DrawRectangleRec(
        Rectangle{ -medio + Px(4.0f), -medio + Px(5.0f), lado, lado },
        Fade(BLACK, 0.25f)
    );

    Rectangle cara = { -medio, -medio, lado, lado };

    DrawRectangleRec(cara, COLOR_CARA_DADO);
    DrawRectangleLinesEx(cara, Px(3.0f) * escala, CONTORNO_DIBUJO);

    const char* texto = valor > 0 ? TextFormat("%d", valor) : "?";
    const int tamano = (int)(TamanoTexto(48) * escala);

    DrawText(
        texto,
        -MeasureText(texto, tamano) / 2,
        -tamano / 2,
        tamano,
        UI_TINTA
    );

    rlPopMatrix();
}


void OrdenTurno::Inicializar(
    const PartidaTablero& partida
)
{
    cantidad = partida.cantidadJugadores;
    tablero = partida.idTablero;
    tiempo = 0.0f;
    revelado = false;
    terminado = false;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        orden[i] = i < cantidad ? partida.ordenParticipantes[i] : -1;
        tiradores[i] = orden[i];
        dado[i] = 0;
        dadoSonado[i] = false;
    }

    // Los tiradores se muestran por numero de jugador.
    for (int i = 0; i < cantidad - 1; i++)
    {
        for (int j = i + 1; j < cantidad; j++)
        {
            if (tiradores[j] < tiradores[i])
            {
                int temporal = tiradores[i];
                tiradores[i] = tiradores[j];
                tiradores[j] = temporal;
            }
        }
    }

    // Valores distintos 1..10, el mas alto para quien abre la partida.
    int reserva[10];

    for (int i = 0; i < 10; i++)
    {
        reserva[i] = i + 1;
    }

    for (int i = 9; i > 0; i--)
    {
        int j = GetRandomValue(0, i);
        int temporal = reserva[i];
        reserva[i] = reserva[j];
        reserva[j] = temporal;
    }

    int valores[MAX_PARTICIPANTES] = {};

    for (int i = 0; i < cantidad; i++)
    {
        valores[i] = reserva[i];
    }

    for (int i = 0; i < cantidad - 1; i++)
    {
        for (int j = i + 1; j < cantidad; j++)
        {
            if (valores[j] > valores[i])
            {
                int temporal = valores[i];
                valores[i] = valores[j];
                valores[j] = temporal;
            }
        }
    }

    for (int k = 0; k < cantidad; k++)
    {
        dado[orden[k]] = valores[k];
    }

    // Presentacion: cae el tablon y despues las fichas, una a una.
    panel.Iniciar(PANEL_ORDEN, 0.0f);

    for (int j = 0; j < MAX_PARTICIPANTES; j++)
    {
        fichas[j].Iniciar(
            Rectangle
            {
                XFichaOrden((float)j, cantidad),
                Y_FICHA_ORDEN,
                ANCHO_FICHA_ORDEN,
                ALTO_FICHA_ORDEN
            },
            nullptr,
            0.30f + j * UI_T_ESCALONADO
        );
    }

    lector.Reiniciar();
}


void OrdenTurno::Actualizar(
    float deltaTime,
    const Participante participantes[],
    int cantidadParticipantes,
    AudioJuego& audio
)
{
    tiempo += deltaTime;

    const EstiloUI& estilo = ObtenerEstiloUI();

    ActualizarPanelUI(panel, estilo, deltaTime);

    for (int j = 0; j < cantidad; j++)
    {
        ActualizarTarjetaUI(fichas[j], estilo, deltaTime);
    }

    EntradaFlujo entrada =
        lector.Leer(participantes, cantidadParticipantes);

    const float tiempoRevelar = ObtenerTiempoRevelarOrden(cantidad);

    for (int j = 0; j < cantidad; j++)
    {
        const int participante = tiradores[j];

        if (
            !dadoSonado[participante] &&
            tiempo >= ObtenerTiempoDetencionDado(j)
        )
        {
            dadoSonado[participante] = true;
            audio.ReproducirSonido(SONIDO_DADO);
        }
    }

    if (!revelado && tiempo >= tiempoRevelar)
    {
        revelado = true;
        audio.ReproducirSonido(SONIDO_UI_CONFIRMAR);
    }

    if (revelado && tiempo >= tiempoRevelar + 2.2f)
    {
        terminado = true;
        return;
    }

    if (
        tiempo < BLOQUEO_ENTRADA_FLUJO ||
        (!entrada.confirmar && !entrada.cancelar)
    )
    {
        return;
    }

    if (!revelado)
    {
        // Primera pulsacion: muestra el resultado ya.
        for (int j = 0; j < cantidad; j++)
        {
            dadoSonado[tiradores[j]] = true;
        }

        tiempo = tiempoRevelar;
        revelado = true;
        audio.ReproducirSonido(SONIDO_UI_CONFIRMAR);
    }
    else if (tiempo >= tiempoRevelar + 0.6f)
    {
        terminado = true;
    }
}


// Contenido de una ficha del sorteo. (x, y): esquina de la ficha en
// unidades de referencia, ya con la caida de entrada.
static void DibujarFichaOrden(
    const OrdenTurno& sorteo,
    int posicionTirador,
    int posicionFinal,
    const Participante& datos,
    float x,
    float y,
    float reorden,
    bool reducir
)
{
    const int participante = sorteo.tiradores[posicionTirador];
    const float centroX = x + ANCHO_FICHA_ORDEN * 0.5f;

    // Cabecera: banderin del jugador, numero y control.
    Vector2 mastil = PuntoRef(x + 28.0f, y + 20.0f);

    DibujarBanderin(mastil.x, mastil.y, datos.color, 1.0f);

    Vector2 nombre = PuntoRef(x + 82.0f, y + 20.0f);

    DibujarTextoTinta(
        TextFormat("J%d", participante + 1),
        nombre.x,
        nombre.y,
        UI_TEXTO_GRANDE,
        UI_TINTA
    );

    DibujarTextoAjustadoUI(
        ObtenerEtiquetaControl(datos),
        nombre.x,
        nombre.y + Px(36.0f),
        UI_TEXTO_PEQUENO,
        Px(ANCHO_FICHA_ORDEN - 96.0f),
        UI_TINTA_SUAVE,
        false
    );

    DrawLineEx(
        PuntoRef(x + 16.0f, y + 90.0f),
        PuntoRef(x + ANCHO_FICHA_ORDEN - 16.0f, y + 90.0f),
        Px(1.5f),
        Fade(UI_TINTA_SUAVE, 0.45f)
    );

    // Dado: gira mientras rueda, salta al detenerse.
    const float detencion = ObtenerTiempoDetencionDado(posicionTirador);
    const bool detenido = sorteo.tiempo >= detencion;
    const bool rodando = !detenido && sorteo.tiempo >= INICIO_TIRADA_ORDEN;

    int valor = 0;
    float angulo = 0.0f;
    float escala = 1.0f;

    if (detenido)
    {
        valor = sorteo.dado[participante];

        const float desde = sorteo.tiempo - detencion;

        if (!reducir && desde < 0.25f)
        {
            escala = 1.0f + 0.2f * (1.0f - desde / 0.25f);
        }
    }
    else if (rodando)
    {
        valor = ((int)(sorteo.tiempo * 22.0f) + participante * 3) % 10 + 1;

        if (!reducir)
        {
            angulo = std::sin(sorteo.tiempo * 31.0f + posicionTirador * 1.9f) * 8.0f;
        }
    }

    DibujarDadoOrden(PuntoRef(centroX, y + 152.0f), 92.0f, angulo, escala, valor);

    // Pie de la ficha: lo que saco y, al revelar, su turno.
    Vector2 pie = PuntoRef(centroX, y + 214.0f);

    if (reorden > 0.7f)
    {
        DibujarTextoAjustadoUI(
            TextFormat("TURNO %d", posicionFinal + 1),
            pie.x,
            pie.y,
            UI_TEXTO_GRANDE,
            Px(ANCHO_FICHA_ORDEN - 24.0f),
            UI_TINTA,
            true
        );
    }
    else if (detenido)
    {
        DibujarTextoAjustadoUI(
            TextFormat("SACÓ %d", valor),
            pie.x,
            pie.y + Px(6.0f),
            UI_TEXTO,
            Px(ANCHO_FICHA_ORDEN - 24.0f),
            UI_TINTA_SUAVE,
            true
        );
    }
}


void OrdenTurno::Dibujar(
    const Participante participantes[]
) const
{
    const EstiloUI& estilo = ObtenerEstiloUI();

    const bool reducir = estilo.reducirMovimiento;
    const float aparicion = Aparicion(tiempo, 0.0f, 0.30f);

    DibujarVistaPreviaTablero(
        tablero,
        Rectangle{ 0.0f, 0.0f, (float)GetScreenWidth(), (float)GetScreenHeight() },
        tiempo
    );

    DibujarVeloPlaya(aparicion);
    DibujarTituloPlaya("ORDEN DE TURNO", aparicion);

    DibujarPanelUI(estilo, panel);

    if (!estilo.cargado || panel.colgado.retraso > 0.0f)
    {
        return;
    }

    // Todo el contenido acompana la caida y el rebote del tablon.
    const float dy = panel.colgado.caida.valor;
    const float tiempoRevelar = ObtenerTiempoRevelarOrden(cantidad);

    Vector2 subtitulo = PuntoRef(640.0f, 116.0f + dy);

    DibujarTextoAjustadoUI(
        revelado ? "EL DADO MÁS ALTO JUEGA PRIMERO" : "TIRANDO LOS DADOS...",
        subtitulo.x,
        subtitulo.y,
        UI_TEXTO,
        Px(PANEL_ORDEN.width - 80.0f),
        UI_TINTA,
        true
    );

    DrawLineEx(
        PuntoRef(PANEL_ORDEN.x + 40.0f, 146.0f + dy),
        PuntoRef(PANEL_ORDEN.x + PANEL_ORDEN.width - 40.0f, 146.0f + dy),
        Px(1.5f),
        Fade(UI_TINTA_SUAVE, 0.45f)
    );

    // Reordenamiento de las fichas (0 = por numero, 1 = por turno).
    float reorden = 0.0f;

    if (revelado)
    {
        reorden = reducir ? 1.0f : Suavizar((tiempo - tiempoRevelar) / 0.6f);
    }

    for (int j = 0; j < cantidad; j++)
    {
        const int participante = tiradores[j];

        int posicionFinal = j;

        for (int k = 0; k < cantidad; k++)
        {
            if (orden[k] == participante)
            {
                posicionFinal = k;
            }
        }

        if (fichas[j].colgado.retraso > 0.0f)
        {
            continue;
        }

        TarjetaUI ficha = fichas[j];
        ficha.ref.x = XFichaOrden(
            (float)j + ((float)posicionFinal - (float)j) * reorden,
            cantidad
        );
        ficha.ref.y += dy;

        DibujarTarjetaUI(estilo, ficha);

        const float x = ficha.ref.x;
        const float y = ficha.ref.y + ficha.colgado.caida.valor;

        DibujarFichaOrden(
            *this,
            j,
            posicionFinal,
            participantes[participante],
            x,
            y,
            reorden,
            reducir
        );

        // Sello del puesto: cae como un sello de goma al revelar.
        const float sello = Limitar01((reorden - 0.7f) / 0.3f);

        if (sello > 0.0f)
        {
            const bool primero = posicionFinal == 0;

            if (primero)
            {
                DibujarMarcoFoco(RectRef(x, y, ANCHO_FICHA_ORDEN, ALTO_FICHA_ORDEN));
            }

            DibujarSelloUI(
                TextFormat("%dº", posicionFinal + 1),
                PuntoRef(x + ANCHO_FICHA_ORDEN - 14.0f, y + 14.0f),
                30.0f * (reducir ? 1.0f : 1.0f + 0.5f * (1.0f - sello)),
                primero ? UI_SELLO_ESTRELLA : UI_SELLO_CIRCULO,
                ObtenerColorMedalla(posicionFinal + 1),
                primero ? -10.0f : 8.0f,
                reducir ? 1.0f : sello
            );
        }
    }

    // Quien abre la partida y el resto del orden.
    if (revelado && cantidad > 0)
    {
        const float alfa =
            reducir ? 1.0f : Aparicion(tiempo, tiempoRevelar + 0.4f, 0.3f);

        Vector2 abre = PuntoRef(640.0f, 450.0f + dy);

        DibujarTextoAjustadoUI(
            TextFormat("¡J%d ABRE LA PARTIDA!", orden[0] + 1),
            abre.x,
            abre.y,
            UI_TEXTO_GRANDE,
            Px(PANEL_ORDEN.width - 80.0f),
            Fade(UI_TINTA, alfa),
            true
        );

        if (cantidad > 1)
        {
            char despues[64] = "DESPUÉS:";
            int usado = (int)std::strlen(despues);

            for (int k = 1; k < cantidad && usado < (int)sizeof(despues); k++)
            {
                const char* separador =
                    k == 1 ? " " : (k == cantidad - 1 ? " Y " : ", ");

                usado += std::snprintf(
                    despues + usado,
                    sizeof(despues) - usado,
                    "%sJ%d",
                    separador,
                    orden[k] + 1
                );
            }

            DibujarTextoAjustadoUI(
                despues,
                abre.x,
                abre.y + Px(42.0f),
                UI_TEXTO,
                Px(PANEL_ORDEN.width - 80.0f),
                Fade(UI_TINTA_SUAVE, alfa),
                true
            );
        }
    }

    DibujarAyudaAccionesFlujo(
        estilo,
        ObtenerControlesAyuda(participantes, MAX_PARTICIPANTES),
        nullptr,
        revelado ? "COMENZAR LA PARTIDA" : "VER EL RESULTADO",
        nullptr
    );
}


//==================================================
// RESULTADOS DE LA PARTIDA
//==================================================
//
// "Entrega de trofeos": la misma carta que la eleccion de destino. A la
// izquierda, el podio de madera donde se izan los banderines de los
// jugadores y, colgadas del riel, las opciones; a la derecha, la
// clasificacion con trofeos y monedas.

static const Rectangle PANEL_RESULTADOS = { 40.0f, 92.0f, 1200.0f, 568.0f };
static const float X_CENTRO_PODIO = 418.0f;
static const float Y_BASE_PODIO = 500.0f;
static const float ANCHO_COLUMNA_PODIO = 136.0f;
static const float SEPARACION_COLUMNA_PODIO = 22.0f;
static const float ALTO_MASTIL_PODIO = 76.0f;

// Clasificacion (mismas columnas que la bitacora del destino).
static const float ALTO_FILA_CLASIFICACION = 46.0f;
static const float X_TROFEOS_CLASIFICACION = 1110.0f;     // borde derecho
static const float X_MONEDAS_CLASIFICACION = 1206.0f;     // borde derecho

static const char* TEXTOS_OPCIONES_RESULTADOS[ResultadosPartida::CANTIDAD_OPCIONES] =
{
    "REVANCHA",
    "NUEVA PARTIDA",
    "VOLVER AL HUB"
};

static const char* DESCRIPCIONES_OPCIONES_RESULTADOS[ResultadosPartida::CANTIDAD_OPCIONES] =
{
    "JUGÁS OTRA VEZ EN EL MISMO TABLERO, CON LOS MISMOS JUGADORES Y RONDAS.",
    "VOLVÉS A ELEGIR PERSONAJES, TABLERO Y RONDAS.",
    "VOLVÉS AL MENÚ PRINCIPAL."
};


static float ObtenerTiempoRevelarPuesto(
    int slot,
    int cantidad
)
{
    float tiempo = 0.9f + (float)(cantidad - 1 - slot) * 1.0f;

    if (slot == 0)
    {
        tiempo += 0.7f;
    }

    return tiempo;
}


static float ObtenerTiempoMenuResultados(
    int cantidad
)
{
    return ObtenerTiempoRevelarPuesto(0, cantidad) + 1.4f;
}


void ResultadosPartida::Inicializar(
    const PartidaTablero& partida
)
{
    cantidad = partida.cantidadJugadores;
    tablero = partida.idTablero;
    rondas = partida.cantidadRondas;
    tiempo = 0.0f;
    sonidosReproducidos = 0;
    opcion = 0;
    accion = RESULTADOS_NINGUNA;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        ranking[i] = i < cantidad ? partida.ordenParticipantes[i] : -1;
        puesto[i] = 0;
        trofeos[i] = partida.jugadores[i].trofeos;
        monedas[i] = partida.jugadores[i].monedas;
    }

    // Trofeos primero, monedas como desempate; el orden de turno
    // mantiene estable la lista.
    for (int i = 1; i < cantidad; i++)
    {
        int actual = ranking[i];
        int j = i - 1;

        while (
            j >= 0 &&
            (trofeos[ranking[j]] < trofeos[actual] ||
             (trofeos[ranking[j]] == trofeos[actual] &&
              monedas[ranking[j]] < monedas[actual]))
        )
        {
            ranking[j + 1] = ranking[j];
            j--;
        }

        ranking[j + 1] = actual;
    }

    for (int k = 0; k < cantidad; k++)
    {
        const bool empate =
            k > 0 &&
            trofeos[ranking[k]] == trofeos[ranking[k - 1]] &&
            monedas[ranking[k]] == monedas[ranking[k - 1]];

        puesto[k] = empate ? puesto[k - 1] : k + 1;
    }

    // Presentacion: la carta cae ya; los carteles, al aparecer el menu.
    panel.Iniciar(PANEL_RESULTADOS, 0.0f);
    menuPresentado = false;

    lector.Reiniciar();
}


void ResultadosPartida::Actualizar(
    float deltaTime,
    const Participante participantes[],
    int cantidadParticipantes,
    AudioJuego& audio
)
{
    tiempo += deltaTime;

    const EstiloUI& estilo = ObtenerEstiloUI();
    const float tiempoMenu = ObtenerTiempoMenuResultados(cantidad);

    ActualizarPanelUI(panel, estilo, deltaTime);

    if (!menuPresentado && tiempo >= tiempoMenu)
    {
        menuPresentado = true;

        for (int i = 0; i < CANTIDAD_OPCIONES; i++)
        {
            carteles[i].Iniciar(
                RectCartelTablero(i, CANTIDAD_OPCIONES),
                TEXTOS_OPCIONES_RESULTADOS[i],
                (i % 2 == 0) ? -1.5f : 1.5f,
                i * UI_T_ESCALONADO
            );
        }
    }

    if (menuPresentado)
    {
        for (int i = 0; i < CANTIDAD_OPCIONES; i++)
        {
            carteles[i].activo = i == opcion;
            ActualizarBotonUI(carteles[i], estilo, deltaTime, i == opcion);
        }
    }

    EntradaFlujo entrada =
        lector.Leer(participantes, cantidadParticipantes);

    while (
        sonidosReproducidos < cantidad &&
        tiempo >= ObtenerTiempoRevelarPuesto(
            cantidad - 1 - sonidosReproducidos,
            cantidad
        )
    )
    {
        const bool ganador = sonidosReproducidos == cantidad - 1;

        audio.ReproducirSonido(
            ganador ? SONIDO_RESULTADO : SONIDO_MONEDA
        );

        sonidosReproducidos++;
    }

    if (tiempo < BLOQUEO_ENTRADA_FLUJO)
    {
        return;
    }

    // Durante la revelacion, confirmar o cancelar la salta.
    if (tiempo < tiempoMenu)
    {
        if (entrada.confirmar || entrada.cancelar)
        {
            tiempo = tiempoMenu;
            sonidosReproducidos = cantidad;
        }

        return;
    }

    // El menu espera un instante tras aparecer: la pulsacion que salto
    // la animacion no puede elegir una opcion.
    if (tiempo < tiempoMenu + BLOQUEO_ENTRADA_FLUJO)
    {
        return;
    }

    if (entrada.cancelar)
    {
        accion = RESULTADOS_VOLVER_HUB;
        audio.ReproducirSonido(SONIDO_UI_CANCELAR);
        return;
    }

    if (entrada.confirmar)
    {
        accion =
            opcion == 0 ? RESULTADOS_REVANCHA :
            opcion == 1 ? RESULTADOS_NUEVA_PARTIDA :
                          RESULTADOS_VOLVER_HUB;

        carteles[opcion].colgado.Pulsar();
        audio.ReproducirSonido(SONIDO_UI_CONFIRMAR);
        return;
    }

    int cambio =
        ((entrada.derecha || entrada.abajo) ? 1 : 0) -
        ((entrada.izquierda || entrada.arriba) ? 1 : 0);

    if (cambio != 0)
    {
        opcion = (opcion + cambio + CANTIDAD_OPCIONES) % CANTIDAD_OPCIONES;
        audio.ReproducirSonido(SONIDO_UI_MOVER);
    }
}


// Corona dorada con contorno. centroX / yBase en pixeles.
static void DibujarCorona(
    float centroX,
    float yBase,
    float escala
)
{
    const float ancho = 40.0f * escala;
    const float alto = 24.0f * escala;
    const float izquierda = centroX - ancho / 2.0f;
    const float yBanda = yBase - alto * 0.35f;

    Vector2 puntas[3];

    for (int i = 0; i < 3; i++)
    {
        puntas[i] = Vector2{ izquierda + ancho * (0.17f + 0.33f * (float)i), yBase - alto };
    }

    // Contorno: la misma silueta un poco mas grande, detras.
    const float borde = 2.0f * escala;

    for (int capa = 0; capa < 2; capa++)
    {
        const float g = capa == 0 ? borde : 0.0f;
        const Color color = capa == 0 ? CONTORNO_DIBUJO : UI_FOCO;

        DrawRectangleRec(
            Rectangle{ izquierda - g, yBanda - g, ancho + g * 2.0f, yBase - yBanda + g * 2.0f },
            color
        );

        for (int i = 0; i < 3; i++)
        {
            DrawTriangle(
                Vector2{ puntas[i].x - ancho * 0.17f - g, yBanda + g * 0.5f },
                Vector2{ puntas[i].x + ancho * 0.17f + g, yBanda + g * 0.5f },
                Vector2{ puntas[i].x, puntas[i].y - g * 1.5f },
                color
            );
        }
    }

    // Joyas.
    for (int i = 0; i < 3; i++)
    {
        DrawCircleV(
            Vector2{ puntas[i].x, yBanda + (yBase - yBanda) * 0.5f },
            2.2f * escala,
            UI_PELIGRO
        );
    }
}


// Banderin chico para las filas de la clasificacion. (x, y) = punta del
// mastil en pixeles.
static void DibujarBanderinFila(
    float x,
    float y,
    Color color,
    float alfa
)
{
    DrawLineEx(
        Vector2{ x, y },
        Vector2{ x, y + Px(30.0f) },
        Px(3.0f),
        Fade(UI_MADERA_OSCURA, alfa)
    );

    Vector2 a = { x + Px(1.5f), y + Px(2.0f) };
    Vector2 b = { x + Px(1.5f), y + Px(18.0f) };
    Vector2 punta = { x + Px(24.0f), y + Px(10.0f) };

    DrawTriangle(a, b, punta, Fade(color, alfa));
    DrawTriangleLines(a, b, punta, Fade(CONTORNO_DIBUJO, alfa));
}


// Texto en tinta alineado a la derecha (borde en pixeles).
static void DibujarTextoDerechaFlujo(
    const char* texto,
    float derecha,
    float y,
    int tokenTexto,
    Color color
)
{
    const int tamano = TamanoTexto(tokenTexto);

    DrawText(texto, (int)(derecha - MeasureText(texto, tamano)), (int)y, tamano, color);
}


// Confeti del ganador: se dibuja encima de todo y se apaga solo a los
// pocos segundos para no distraer del menu.
static void DibujarConfetiResultados(
    float desde
)
{
    const float alfa = 1.0f - Limitar01((desde - 6.0f) / 1.5f);

    if (alfa <= 0.0f)
    {
        return;
    }

    const float ancho = (float)GetScreenWidth();
    const float alto = (float)GetScreenHeight();
    const float u = EscalaUI();

    const Color paleta[5] =
    {
        UI_FOCO,
        UI_POSITIVO,
        UI_PELIGRO,
        { 120, 200, 236, 255 },
        UI_PLACA_CLARA
    };

    for (int i = 0; i < 80; i++)
    {
        const float rx = std::fmod((float)i * 0.6180339f, 1.0f);
        const float velocidad = (90.0f + std::fmod((float)i * 37.0f, 120.0f)) * u;
        const float y = std::fmod(desde * velocidad + (float)i * 53.0f * u, alto + 40.0f * u) - 20.0f * u;
        const float x = rx * ancho + std::sin(desde * 2.0f + (float)i) * 22.0f * u;

        DrawRectanglePro(
            Rectangle{ x, y, 9.0f * u, 5.0f * u },
            Vector2{ 4.5f * u, 2.5f * u },
            desde * 180.0f + (float)i * 40.0f,
            Fade(paleta[i % 5], 0.9f * alfa)
        );
    }
}


void ResultadosPartida::Dibujar(
    const Participante participantes[]
) const
{
    const EstiloUI& estilo = ObtenerEstiloUI();

    const Color tema = ObtenerColorTemaTablero(tablero);
    const bool reducir = estilo.reducirMovimiento;
    const float aparicion = Aparicion(tiempo, 0.0f, 0.30f);

    DibujarVistaPreviaTablero(
        tablero,
        Rectangle{ 0.0f, 0.0f, (float)GetScreenWidth(), (float)GetScreenHeight() },
        tiempo * 0.5f
    );

    DibujarVeloPlaya(aparicion);
    DibujarTituloPlaya("FIN DE LA PARTIDA", aparicion);

    DibujarPanelUI(estilo, panel);

    if (!estilo.cargado || panel.colgado.retraso > 0.0f)
    {
        return;
    }

    // Todo el contenido acompana la caida y el rebote de la carta.
    const float dy = panel.colgado.caida.valor;
    const float tiempoGanador = ObtenerTiempoRevelarPuesto(0, cantidad);
    const float tiempoMenu = ObtenerTiempoMenuResultados(cantidad);
    const bool hayGanador = tiempo >= tiempoGanador;

    //------------------------------
    // ANUNCIO DEL GANADOR
    //------------------------------

    Vector2 anuncio = PuntoRef(X_CENTRO_PODIO, 122.0f + dy);

    if (hayGanador)
    {
        int empatados = 0;

        for (int k = 0; k < cantidad; k++)
        {
            if (puesto[k] == 1)
            {
                empatados++;
            }
        }

        const char* texto =
            empatados > 1
            ? "¡EMPATE EN EL PRIMER PUESTO!"
            : TextFormat("¡GANA J%d!", ranking[0] + 1);

        // Aparece un poco mas grande y se asienta.
        float crecer = 1.0f;

        if (!reducir)
        {
            crecer = 1.0f + 0.35f * (1.0f - Suavizar((tiempo - tiempoGanador) / 0.3f));
        }

        int tamano = (int)(TamanoTexto(UI_TEXTO_GRANDE) * crecer);
        const float anchoMaximo = Px(660.0f);

        while (tamano > 10 && MeasureText(texto, tamano) > anchoMaximo)
        {
            tamano--;
        }

        DrawText(
            texto,
            (int)(anuncio.x - MeasureText(texto, tamano) * 0.5f),
            (int)(anuncio.y + (TamanoTexto(UI_TEXTO_GRANDE) - tamano) * 0.5f),
            tamano,
            UI_TINTA
        );
    }
    else
    {
        DibujarTextoAjustadoUI(
            "CONTANDO TROFEOS...",
            anuncio.x,
            anuncio.y + Px(6.0f),
            UI_TEXTO,
            Px(660.0f),
            UI_TINTA_SUAVE,
            true
        );
    }

    //------------------------------
    // PODIO: 2.o, 1.o, 3.o, 4.o de izquierda a derecha
    //------------------------------

    const int ordenPantalla[MAX_PARTICIPANTES] = { 1, 0, 2, 3 };

    int columnas[MAX_PARTICIPANTES] = {};
    int cantidadColumnas = 0;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (ordenPantalla[i] < cantidad)
        {
            columnas[cantidadColumnas] = ordenPantalla[i];
            cantidadColumnas++;
        }
    }

    const float totalPodio =
        cantidadColumnas * ANCHO_COLUMNA_PODIO +
        (cantidadColumnas - 1) * SEPARACION_COLUMNA_PODIO;

    const float x0 = X_CENTRO_PODIO - totalPodio * 0.5f;
    const float yBase = Y_BASE_PODIO + dy;

    // Tablon de la base, un poco mas ancho que las columnas.
    DibujarNueveCortes(
        estilo.madera,
        12,
        RectRef(x0 - 24.0f, yBase, totalPodio + 48.0f, 18.0f),
        EscalaUI(),
        WHITE
    );

    for (int pos = 0; pos < cantidadColumnas; pos++)
    {
        const int slot = columnas[pos];
        const int participante = ranking[slot];
        const Participante& datos = participantes[participante];

        const float inicio = ObtenerTiempoRevelarPuesto(slot, cantidad);

        float crecimiento = Suavizar((tiempo - inicio) / 0.5f);

        if (reducir)
        {
            crecimiento = tiempo >= inicio ? 1.0f : 0.0f;
        }

        if (crecimiento <= 0.0f)
        {
            continue;
        }

        // Altura segun el puesto (los empatados quedan a la par).
        const float alturaFinal = 70.0f + (float)(cantidad - puesto[slot]) * 40.0f;
        const float altura = alturaFinal * crecimiento;
        const float x = x0 + pos * (ANCHO_COLUMNA_PODIO + SEPARACION_COLUMNA_PODIO);
        const float arriba = yBase - altura;
        const float centroX = x + ANCHO_COLUMNA_PODIO * 0.5f;
        const bool campeon = puesto[slot] == 1;

        Rectangle columna = RectRef(x, arriba, ANCHO_COLUMNA_PODIO, altura + 4.0f);

        DrawRectangleRec(
            Rectangle{ columna.x + Px(5.0f), columna.y + Px(6.0f), columna.width, columna.height },
            Fade(BLACK, 0.25f)
        );

        // Mientras crece, los bordes del 9-slice se achican para no
        // superar la mitad del alto de la columna.
        DibujarNueveCortes(
            estilo.madera,
            12,
            columna,
            std::fmin(EscalaUI(), columna.height / 24.0f),
            WHITE
        );

        if (crecimiento < 0.9f)
        {
            continue;
        }

        // Franja del color del jugador y sello del puesto.
        Rectangle franja = RectRef(x + 12.0f, arriba + 12.0f, ANCHO_COLUMNA_PODIO - 24.0f, 8.0f);

        DrawRectangleRec(franja, datos.color);
        DrawRectangleLinesEx(franja, Px(1.5f), CONTORNO_DIBUJO);

        DibujarSelloUI(
            TextFormat("%dº", puesto[slot]),
            PuntoRef(centroX, arriba + 46.0f),
            26.0f,
            campeon ? UI_SELLO_ESTRELLA : UI_SELLO_CIRCULO,
            ObtenerColorMedalla(puesto[slot]),
            campeon ? -8.0f : 0.0f
        );

        // El banderin del campeon se iza y baja un poco (salvo
        // reducirMovimiento): el mastil siempre apoya en la columna.
        float izado = 0.0f;

        if (campeon && hayGanador && !reducir)
        {
            izado = std::fabs(std::sin(tiempo * 4.0f)) * 10.0f;
        }

        Rectangle tela =
        {
            centroX - 26.0f,
            arriba - ALTO_MASTIL_PODIO - izado,
            62.0f,
            28.0f
        };

        DibujarBanderinUI(
            estilo,
            TextFormat("J%d", participante + 1),
            tela,
            ALTO_MASTIL_PODIO + izado,
            datos.color,
            true,
            pos * 1.3f
        );

        if (campeon && hayGanador)
        {
            // Sobre la perilla del mastil (borde izquierdo de la tela).
            Vector2 perilla = PuntoRef(tela.x - 4.0f, tela.y - 12.0f);

            DibujarCorona(perilla.x, perilla.y - Px(8.0f), EscalaUI());
        }
    }

    //------------------------------
    // OPCIONES (carteles colgados del riel)
    //------------------------------

    DibujarNueveCortes(
        estilo.madera,
        12,
        RectRef(
            RIEL_CARTELES_TABLERO.x,
            RIEL_CARTELES_TABLERO.y + dy,
            RIEL_CARTELES_TABLERO.width,
            RIEL_CARTELES_TABLERO.height
        ),
        EscalaUI(),
        WHITE
    );

    if (menuPresentado)
    {
        for (int i = 0; i < CANTIDAD_OPCIONES; i++)
        {
            BotonUI cartel = carteles[i];
            cartel.ref.y += dy;

            DibujarBotonUI(estilo, cartel, i == opcion);
        }
    }

    //------------------------------
    // CLASIFICACION
    //------------------------------

    Vector2 origen = PuntoRef(X_INFO_TABLERO, 122.0f + dy);
    const float xInfo = origen.x;
    float y = origen.y;

    DibujarRotulo(TextFormat("PARTIDA DE %d RONDAS", rondas), xInfo, y);
    y += Px(22.0f);

    DibujarTextoAjustadoUI(
        ObtenerNombreTablero(tablero),
        xInfo,
        y,
        UI_TEXTO_GRANDE,
        Px(ANCHO_INFO_TABLERO),
        UI_TINTA,
        false
    );
    y += TamanoTexto(UI_TEXTO_GRANDE) + Px(8.0f);

    DibujarCintaTema(xInfo, y, Px(ANCHO_INFO_TABLERO), tema);
    y += Px(22.0f);

    const float xTrofeos = PuntoRef(X_TROFEOS_CLASIFICACION, 0.0f).x;
    const float xMonedas = PuntoRef(X_MONEDAS_CLASIFICACION, 0.0f).x;
    const int tamanoPequeno = TamanoTexto(UI_TEXTO_PEQUENO);
    const int tamanoTexto = TamanoTexto(UI_TEXTO);

    DibujarRotulo("CLASIFICACIÓN", xInfo, y);
    DibujarTextoDerechaFlujo("TROFEOS", xTrofeos, y, UI_TEXTO_PEQUENO, UI_TINTA_SUAVE);
    DibujarTextoDerechaFlujo("MONEDAS", xMonedas, y, UI_TEXTO_PEQUENO, UI_TINTA_SUAVE);
    y += Px(24.0f);

    for (int k = 0; k < cantidad; k++)
    {
        const float inicio = ObtenerTiempoRevelarPuesto(k, cantidad);
        const float alfa =
            reducir
            ? (tiempo >= inicio ? 1.0f : 0.0f)
            : Aparicion(tiempo, inicio, 0.3f);

        const float yFila = y + Px(ALTO_FILA_CLASIFICACION) * k;
        const float centroY = yFila + Px(ALTO_FILA_CLASIFICACION) * 0.5f;

        if (alfa <= 0.0f)
        {
            continue;
        }

        const int participante = ranking[k];
        const Participante& datos = participantes[participante];

        if (puesto[k] == 1 && hayGanador)
        {
            DrawRectangleRec(
                Rectangle
                {
                    xInfo - Px(8.0f),
                    yFila + Px(2.0f),
                    Px(ANCHO_INFO_TABLERO) + Px(14.0f),
                    Px(ALTO_FILA_CLASIFICACION - 4.0f)
                },
                Fade(UI_FOCO, 0.30f * alfa)
            );
        }

        // Medalla con el puesto.
        Vector2 medalla = { xInfo + Px(14.0f), centroY };

        DrawCircleV(medalla, Px(14.0f), Fade(UI_TINTA, alfa));
        DrawCircleV(medalla, Px(12.5f), Fade(ObtenerColorMedalla(puesto[k]), alfa));

        const char* textoPuesto = TextFormat("%d", puesto[k]);

        DrawText(
            textoPuesto,
            (int)(medalla.x - MeasureText(textoPuesto, tamanoPequeno) * 0.5f),
            (int)(medalla.y - tamanoPequeno * 0.5f),
            tamanoPequeno,
            Fade(UI_TINTA, alfa)
        );

        DibujarBanderinFila(xInfo + Px(40.0f), centroY - Px(15.0f), datos.color, alfa);

        const char* nombre = TextFormat("J%d", participante + 1);
        const float xNombre = xInfo + Px(72.0f);

        DrawText(
            nombre,
            (int)xNombre,
            (int)(centroY - tamanoTexto * 0.5f),
            tamanoTexto,
            Fade(UI_TINTA, alfa)
        );

        const float xControl = xNombre + MeasureText(nombre, tamanoTexto) + Px(10.0f);

        DibujarTextoAjustadoUI(
            ObtenerEtiquetaControl(datos),
            xControl,
            centroY - tamanoPequeno * 0.5f + Px(1.0f),
            UI_TEXTO_PEQUENO,
            xTrofeos - Px(78.0f) - xControl,
            Fade(UI_TINTA_SUAVE, alfa),
            false
        );

        // Los numeros cuentan hasta su valor al aparecer la fila.
        const float conteo = reducir ? 1.0f : Limitar01((tiempo - inicio) / 0.8f);

        DibujarTextoDerechaFlujo(
            TextFormat("%d", (int)((float)trofeos[participante] * conteo + 0.5f)),
            xTrofeos,
            centroY - tamanoTexto * 0.5f,
            UI_TEXTO,
            Fade(UI_TINTA, alfa)
        );

        DibujarTextoDerechaFlujo(
            TextFormat("%d", (int)((float)monedas[participante] * conteo + 0.5f)),
            xMonedas,
            centroY - tamanoTexto * 0.5f,
            UI_TEXTO,
            Fade(UI_TINTA, alfa)
        );
    }

    // Regla de desempate y la opcion elegida, al pie de la columna.
    Vector2 regla = PuntoRef(X_INFO_TABLERO, 420.0f + dy);

    DrawLineEx(
        Vector2{ regla.x, regla.y },
        Vector2{ regla.x + Px(ANCHO_INFO_TABLERO), regla.y },
        Px(1.5f),
        Fade(UI_TINTA_SUAVE, 0.45f)
    );

    DibujarParrafoUI(
        "GANA QUIEN TENGA MÁS TROFEOS. CON LOS MISMOS TROFEOS, DESEMPATAN LAS MONEDAS.",
        regla.x,
        regla.y + Px(12.0f),
        UI_TEXTO_PEQUENO,
        Px(ANCHO_INFO_TABLERO),
        UI_TINTA_SUAVE
    );

    if (menuPresentado)
    {
        const float alfa = reducir ? 1.0f : Aparicion(tiempo, tiempoMenu, 0.3f);
        Vector2 detalle = PuntoRef(X_INFO_TABLERO, 500.0f + dy);

        DibujarRotulo(TEXTOS_OPCIONES_RESULTADOS[opcion], detalle.x, detalle.y, alfa);

        DibujarParrafoUI(
            DESCRIPCIONES_OPCIONES_RESULTADOS[opcion],
            detalle.x,
            detalle.y + Px(22.0f),
            UI_TEXTO,
            Px(ANCHO_INFO_TABLERO),
            Fade(UI_TINTA, alfa),
            UI_ALINEAR_IZQUIERDA,
            Px(110.0f)
        );
    }

    if (hayGanador && !reducir)
    {
        DibujarConfetiResultados(tiempo - tiempoGanador);
    }

    const int controles = ObtenerControlesAyuda(participantes, MAX_PARTICIPANTES);

    if (menuPresentado)
    {
        DibujarAyudaAccionesFlujo(estilo, controles, "ELEGIR", "CONFIRMAR", "VOLVER AL HUB");
    }
    else
    {
        DibujarAyudaAccionesFlujo(estilo, controles, nullptr, "SALTAR", nullptr);
    }
}
