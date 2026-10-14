#include "UI/SeleccionMinijuegos.h"

#include "Systems/Input.h"
#include "UI/EstiloUIGlobal.h"
#include "UI/MiniaturasMinijuegos.h"
#include "rlgl.h"

#include <cmath>
#include <cstdio>
#include <cstring>


//==================================================
// LAYOUT (unidades de referencia 1280x720)
//==================================================

static const int COLUMNAS_CATALOGO = 8;

// Filas visibles a la vez. Con mas minijuegos la grilla se desplaza
// en lugar de achicar las postales hasta volverlas ilegibles.
static const int FILAS_VISIBLES_CATALOGO = 3;

// Tablon del puerto: grilla de postales arriba y ficha del minijuego
// con foco abajo.
static const Rectangle PANEL_TABLON = { 40.0f, 92.0f, 1200.0f, 568.0f };
static const Rectangle AREA_GRILLA = { 72.0f, 150.0f, 1116.0f, 338.0f };
static const float SEPARACION_POSTALES = 12.0f;
static const float X_CUERDA_DESPLAZAMIENTO = 1209.0f;
static const Rectangle FICHA_TABLON = { 72.0f, 500.0f, 1136.0f, 144.0f };

// Cartel de la previa.
static const Rectangle PANEL_PREVIA = { 70.0f, 84.0f, 1140.0f, 584.0f };
static const Rectangle MARCO_ILUSTRACION_PREVIA = { 108.0f, 112.0f, 440.0f, 252.0f };
static const float X_INFO_PREVIA = 580.0f;
static const float ANCHO_INFO_PREVIA = 592.0f;
static const Rectangle TARJETA_CONTROLES = { 580.0f, 282.0f, 592.0f, 148.0f };
static const float X_TRIPULACION = 108.0f;
static const float Y_TRIPULACION = 444.0f;
static const float ANCHO_TRIPULACION = 1064.0f;
static const float Y_PUESTOS = 474.0f;
static const float ALTO_PUESTO = 158.0f;
static const float SEPARACION_PUESTOS = 16.0f;


//==================================================
// TIEMPOS
//==================================================

// La previa ignora la entrada un instante: la misma pulsacion que la
// abrio no puede marcar "listo".
static const float BLOQUEO_ENTRADA_PREVIA = 0.25f;

// Con todos listos se ve la ultima bandera subir antes de empezar.
static const float ESPERA_TODOS_LISTOS = 0.9f;

// Los bots izan su bandera solos, uno tras otro.
static const float RETRASO_IZADO_BOT = 0.35f;
static const float ESCALONADO_IZADO_BOT = 0.14f;

static const float DURACION_ENTRADA_FILAS = 0.18f;

static const Color COLOR_BOT = { 145, 145, 152, 255 };


//==================================================
// UTILIDADES
//==================================================

static float Limitar01(float valor)
{
    if (valor < 0.0f) return 0.0f;
    if (valor > 1.0f) return 1.0f;
    return valor;
}


// Curva de salida cubica (arranca rapido y se asienta).
static float Suavizar(float t)
{
    float resto = 1.0f - Limitar01(t);
    return 1.0f - resto * resto * resto;
}


static Vector2 PuntoRef(float x, float y)
{
    Rectangle rect = RectRef(x, y, 0.0f, 0.0f);
    return Vector2{ rect.x, rect.y };
}


static Rectangle Encoger(Rectangle rect, float margen)
{
    return Rectangle{
        rect.x + margen,
        rect.y + margen,
        rect.width - margen * 2.0f,
        rect.height - margen * 2.0f
    };
}


//==================================================
// GRILLA
//==================================================

static int ObtenerFilasTotalesCatalogo()
{
    return
        (CANTIDAD_MINIJUEGOS + COLUMNAS_CATALOGO - 1) /
        COLUMNAS_CATALOGO;
}


static int ObtenerFilaInicialMaxima()
{
    int maximo = ObtenerFilasTotalesCatalogo() - FILAS_VISIBLES_CATALOGO;
    return maximo > 0 ? maximo : 0;
}


static bool EsCeldaVisible(
    int indice,
    int filaInicial
)
{
    int fila = indice / COLUMNAS_CATALOGO;

    return
        fila >= filaInicial &&
        fila < filaInicial + FILAS_VISIBLES_CATALOGO;
}


// Postal en unidades de referencia.
static Rectangle ObtenerCeldaRef(
    int indice,
    int filaInicial
)
{
    float ancho =
        (AREA_GRILLA.width - SEPARACION_POSTALES * (COLUMNAS_CATALOGO - 1)) /
        (float)COLUMNAS_CATALOGO;

    float alto =
        (AREA_GRILLA.height - SEPARACION_POSTALES * (FILAS_VISIBLES_CATALOGO - 1)) /
        (float)FILAS_VISIBLES_CATALOGO;

    int columna = indice % COLUMNAS_CATALOGO;
    int fila = indice / COLUMNAS_CATALOGO - filaInicial;

    return Rectangle
    {
        AREA_GRILLA.x + columna * (ancho + SEPARACION_POSTALES),
        AREA_GRILLA.y + fila * (alto + SEPARACION_POSTALES),
        ancho,
        alto
    };
}


// Postal en pixeles (para el mouse).
static Rectangle ObtenerCeldaCatalogo(
    int indice,
    int filaInicial
)
{
    Rectangle ref = ObtenerCeldaRef(indice, filaInicial);
    return RectRef(ref.x, ref.y, ref.width, ref.height);
}


static int MoverIndice(
    int actual,
    int deltaX,
    int deltaY
)
{
    const int columnas = COLUMNAS_CATALOGO;
    const int filas = ObtenerFilasTotalesCatalogo();

    int columna = actual % columnas;
    int fila = actual / columnas;

    columna += deltaX;
    fila += deltaY;

    if (columna < 0) columna = columnas - 1;
    if (columna >= columnas) columna = 0;
    if (fila < 0) fila = filas - 1;
    if (fila >= filas) fila = 0;

    int candidato = fila * columnas + columna;

    if (candidato >= CANTIDAD_MINIJUEGOS)
    {
        candidato = CANTIDAD_MINIJUEGOS - 1;
    }

    return candidato;
}


// Puesto de la tripulacion en la previa (unidades de referencia).
static Rectangle ObtenerPuestoRef(
    int indice
)
{
    float ancho =
        (ANCHO_TRIPULACION - SEPARACION_PUESTOS * (MAX_PARTICIPANTES - 1)) /
        (float)MAX_PARTICIPANTES;

    return Rectangle
    {
        X_TRIPULACION + indice * (ancho + SEPARACION_PUESTOS),
        Y_PUESTOS,
        ancho,
        ALTO_PUESTO
    };
}


//==================================================
// TECLAS (fieles a LeerInputSeleccionParticipante)
//==================================================
//
// Teclado completo: ESPACIO o ENTER confirman, RETROCESO cancela.
// Teclado WASD: ESPACIO / RETROCESO. Teclado flechas: ENTER / SHIFT DER.
// Mando: A / B. ESC siempre vuelve (lo lee esta pantalla).

static bool EsHumanoConectado(
    const Participante& participante
)
{
    return
        participante.activo &&
        !participante.esBot &&
        participante.conectado;
}


static int ObtenerMascaraControles(
    const Participante participantes[],
    int cantidad
)
{
    int mascara = 0;

    for (int i = 0; i < cantidad; i++)
    {
        if (EsHumanoConectado(participantes[i]))
        {
            mascara |= 1 << participantes[i].control;
        }
    }

    return mascara;
}


static void ArmarTeclasConfirmar(
    int mascara,
    char* destino,
    size_t tamano
)
{
    const bool completo = (mascara & (1 << CONTROL_TECLADO_COMPLETO)) != 0;
    const bool espacio = completo || (mascara & (1 << CONTROL_TECLADO_WASD)) != 0;
    const bool flechas = (mascara & (1 << CONTROL_TECLADO_FLECHAS)) != 0;
    const bool enter = completo || flechas;
    const bool mando = (mascara & (1 << CONTROL_GAMEPAD)) != 0;

    if (mascara == 0)
    {
        std::snprintf(destino, tamano, "ESPACIO / ENTER");
        return;
    }

    std::snprintf(
        destino,
        tamano,
        "%s%s%s",
        espacio ? "ESPACIO" : "",
        enter ? (espacio ? " / ENTER" : "ENTER") : "",
        mando ? ((espacio || enter) ? " / A" : "A") : ""
    );
}


static void ArmarTeclasCancelar(
    int mascara,
    bool conEscape,
    char* destino,
    size_t tamano
)
{
    const bool retroceso =
        (mascara & (1 << CONTROL_TECLADO_COMPLETO)) != 0 ||
        (mascara & (1 << CONTROL_TECLADO_WASD)) != 0;
    const bool flechas = (mascara & (1 << CONTROL_TECLADO_FLECHAS)) != 0;
    const bool mando = (mascara & (1 << CONTROL_GAMEPAD)) != 0;

    char texto[64] = "";

    std::snprintf(
        texto,
        sizeof(texto),
        "%s%s%s%s",
        conEscape ? "ESC" : "",
        retroceso ? " / RETROCESO" : "",
        flechas ? " / SHIFT DER" : "",
        mando ? " / B" : ""
    );

    // Sin ESC delante sobra el primer separador.
    const char* inicio = texto;

    if (!conEscape && std::strncmp(inicio, " / ", 3) == 0)
    {
        inicio += 3;
    }

    std::snprintf(destino, tamano, "%s", inicio);
}


static const char* ObtenerTeclasMover(
    const Participante& participante
)
{
    switch (participante.control)
    {
        case CONTROL_TECLADO_WASD:
            return "WASD";

        case CONTROL_TECLADO_FLECHAS:
            return "FLECHAS";

        case CONTROL_GAMEPAD:
            return "CRUCETA / STICK";

        case CONTROL_TECLADO_COMPLETO:
        case CONTROL_NINGUNO:
            break;
    }

    return "FLECHAS / WASD";
}


// Tecla con la que ESE jugador marca "listo" y la que lo desmarca.
static const char* ObtenerTeclaListo(
    const Participante& participante
)
{
    switch (participante.control)
    {
        case CONTROL_TECLADO_COMPLETO:
            return "ESPACIO O ENTER";

        case CONTROL_TECLADO_FLECHAS:
            return "ENTER";

        case CONTROL_GAMEPAD:
            return "A";

        case CONTROL_TECLADO_WASD:
        case CONTROL_NINGUNO:
            break;
    }

    return "ESPACIO";
}


static const char* ObtenerTeclaBajar(
    const Participante& participante
)
{
    switch (participante.control)
    {
        case CONTROL_TECLADO_FLECHAS:
            return "SHIFT DER";

        case CONTROL_GAMEPAD:
            return "B";

        case CONTROL_TECLADO_COMPLETO:
        case CONTROL_TECLADO_WASD:
        case CONTROL_NINGUNO:
            break;
    }

    return "RETROCESO";
}


// Barra de ayuda: si no cabe con separadores anchos, los acorta.
// parte3 puede ser nullptr (solo dos partes).
static void DibujarAyudaSeleccion(
    const EstiloUI& estilo,
    const char* parte1,
    const char* parte2,
    const char* parte3
)
{
    const char* separadores[2] = { "   |   ", "  |  " };
    char texto[256] = "";

    for (int i = 0; i < 2; i++)
    {
        std::snprintf(
            texto,
            sizeof(texto),
            "%s%s%s%s%s",
            parte1,
            separadores[i],
            parte2,
            parte3 != nullptr ? separadores[i] : "",
            parte3 != nullptr ? parte3 : ""
        );

        if (MeasureText(texto, TamanoTexto(UI_TEXTO_PEQUENO)) <= (int)Px(976.0f))
        {
            break;
        }
    }

    DibujarAyudaUI(estilo, texto);
}


//==================================================
// DIBUJO: PIEZAS LOCALES
//==================================================

// Velo calido: deja ver la isla del HUB y oscurece hacia abajo.
static void DibujarVeloPlaya(
    float opacidad
)
{
    const Color sombra = { 24, 14, 30, 255 };

    DrawRectangleGradientV(
        0,
        0,
        GetScreenWidth(),
        GetScreenHeight(),
        Fade(sombra, 0.40f * opacidad),
        Fade(sombra, 0.72f * opacidad)
    );
}


// Sello: texto en tinta dentro de un recuadro de color, con el icono
// del formato delante si se pide. Devuelve el ancho en pixeles.
static float DibujarSello(
    const char* texto,
    float x,
    float y,
    Color borde,
    int formato = -1,
    float alfa = 1.0f
)
{
    const int tamano = TamanoTexto(UI_TEXTO_PEQUENO);
    const float alto = (float)tamano + Px(10.0f);
    const float anchoIcono = formato >= 0 ? alto * 1.9f : 0.0f;

    Rectangle rect =
    {
        x,
        y,
        (float)MeasureText(texto, tamano) + Px(16.0f) + anchoIcono,
        alto
    };

    DrawRectangleRec(rect, Fade(borde, 0.22f * alfa));
    DrawRectangleLinesEx(rect, Px(2.0f), Fade(borde, alfa));

    if (formato >= 0)
    {
        DibujarIconoFormatoMinijuego(
            (FormatoMinijuego)formato,
            Vector2{ x + Px(8.0f) + anchoIcono * 0.5f - Px(3.0f), y + alto * 0.5f },
            alto * 0.55f,
            alfa
        );
    }

    DrawText(
        texto,
        (int)(x + Px(8.0f) + anchoIcono),
        (int)(y + Px(5.0f)),
        tamano,
        Fade(UI_TINTA, alfa)
    );

    return rect.width;
}


// Parte el texto en lineas de anchoMaximo pixeles. Devuelve cuantas
// salen; si `dibujar`, las dibuja desde (x, y).
static int PartirParrafo(
    const char* texto,
    float x,
    float y,
    int tamano,
    float anchoMaximo,
    float interlineado,
    Color color,
    bool dibujar
)
{
    const int largo = (int)std::strlen(texto);
    int inicio = 0;
    int lineas = 0;
    char linea[256];

    while (inicio < largo)
    {
        while (inicio < largo && texto[inicio] == ' ')
        {
            inicio++;
        }

        if (inicio >= largo)
        {
            break;
        }

        // Agrega palabras mientras la linea quepa (al menos una).
        int mejor = -1;
        int fin = inicio;

        while (true)
        {
            int siguiente = fin;

            while (siguiente < largo && texto[siguiente] != ' ')
            {
                siguiente++;
            }

            int cantidad = siguiente - inicio;
            if (cantidad > 255) cantidad = 255;

            std::memcpy(linea, texto + inicio, (size_t)cantidad);
            linea[cantidad] = '\0';

            if (mejor >= 0 && MeasureText(linea, tamano) > anchoMaximo)
            {
                break;
            }

            mejor = siguiente;

            if (siguiente >= largo)
            {
                break;
            }

            fin = siguiente + 1;
        }

        int cantidad = mejor - inicio;
        if (cantidad > 255) cantidad = 255;

        std::memcpy(linea, texto + inicio, (size_t)cantidad);
        linea[cantidad] = '\0';

        if (dibujar)
        {
            DrawText(linea, (int)x, (int)(y + lineas * interlineado), tamano, color);
        }

        lineas++;
        inicio = mejor;
    }

    return lineas;
}


// Parrafo en tinta: reduce la fuente hasta que entre en maximoLineas.
static void DibujarParrafo(
    const char* texto,
    float x,
    float y,
    int tokenTexto,
    float anchoMaximo,
    int maximoLineas,
    Color color
)
{
    int tamano = TamanoTexto(tokenTexto);

    while (
        tamano > 10 &&
        PartirParrafo(texto, x, y, tamano, anchoMaximo, 0.0f, color, false) > maximoLineas
    )
    {
        tamano--;
    }

    float interlineado = (float)tamano + std::fmax(4.0f, tamano * 0.35f);

    PartirParrafo(texto, x, y, tamano, anchoMaximo, interlineado, color, true);
}


// Tarjeta fina: marco de madera a media escala y placa clara.
static Rectangle DibujarTarjetaMadera(
    const EstiloUI& estilo,
    Rectangle rect,
    float sombra
)
{
    DrawRectangleRec(
        Rectangle{ rect.x + Px(4.0f), rect.y + Px(5.0f), rect.width, rect.height },
        Fade(BLACK, sombra)
    );

    DibujarNueveCortes(estilo.madera, 12, rect, EscalaUI() * 0.6f, WHITE);

    Rectangle placa = Encoger(rect, Px(7.0f));
    DibujarNueveCortes(estilo.placa, 8, placa, EscalaUI(), WHITE);

    return placa;
}


// Ilustracion enmarcada en madera (como un cuadro colgado).
static void DibujarCuadro(
    const EstiloUI& estilo,
    IdMinijuego id,
    Rectangle rect,
    float grosorMarco
)
{
    DrawRectangleRec(
        Rectangle{ rect.x + Px(4.0f), rect.y + Px(5.0f), rect.width, rect.height },
        Fade(BLACK, 0.25f)
    );

    DibujarNueveCortes(estilo.madera, 12, rect, EscalaUI() * grosorMarco / 12.0f, WHITE);

    Rectangle foto = Encoger(rect, Px(grosorMarco));

    DibujarIlustracionMinijuego(id, foto, true);
    DrawRectangleLinesEx(foto, Px(1.5f), UI_MADERA_OSCURA);
}


// Nombre de una postal: una linea si entra comoda; si no, dos lineas
// partidas en el espacio mas cercano al centro.
static void DibujarNombrePostal(
    const char* nombre,
    float centroX,
    float y,
    float ancho,
    float alto
)
{
    const int tamanoUnaLinea = TamanoTexto(15);

    if (MeasureText(nombre, tamanoUnaLinea) <= ancho)
    {
        DrawText(
            nombre,
            (int)(centroX - MeasureText(nombre, tamanoUnaLinea) * 0.5f),
            (int)(y + (alto - tamanoUnaLinea) * 0.5f),
            tamanoUnaLinea,
            UI_TINTA
        );

        return;
    }

    const int largo = (int)std::strlen(nombre);
    int corte = -1;

    for (int i = 0; i < largo; i++)
    {
        if (
            nombre[i] == ' ' &&
            (corte < 0 || std::abs(i - largo / 2) < std::abs(corte - largo / 2))
        )
        {
            corte = i;
        }
    }

    if (corte < 0 || corte >= 63 || largo - corte - 1 >= 63)
    {
        DibujarTextoAjustadoUI(
            nombre,
            centroX,
            y + (alto - tamanoUnaLinea) * 0.5f,
            15,
            ancho,
            UI_TINTA,
            true
        );

        return;
    }

    char lineas[2][64] = {};
    std::memcpy(lineas[0], nombre, (size_t)corte);
    std::memcpy(lineas[1], nombre + corte + 1, (size_t)(largo - corte - 1));

    // Las dos lineas comparten tamano para que se lean parejas.
    int tamano = TamanoTextoAjustadoUI(lineas[0], 13, ancho);
    int tamanoB = TamanoTextoAjustadoUI(lineas[1], 13, ancho);
    if (tamanoB < tamano) tamano = tamanoB;

    float separacion = std::fmax(1.0f, Px(2.0f));
    float yInicio = y + (alto - (tamano * 2.0f + separacion)) * 0.5f;

    for (int i = 0; i < 2; i++)
    {
        DrawText(
            lineas[i],
            (int)(centroX - MeasureText(lineas[i], tamano) * 0.5f),
            (int)(yInicio + i * (tamano + separacion)),
            tamano,
            UI_TINTA
        );
    }
}


// Chinche del color del formato que sujeta la postal al tablon.
static void DibujarChinche(
    Vector2 centro,
    Color color
)
{
    float radio = Px(6.0f);

    DrawCircleV(Vector2{ centro.x + Px(1.5f), centro.y + Px(2.0f) }, radio, Fade(BLACK, 0.30f));
    DrawCircleV(centro, radio, color);
    DrawCircleLinesV(centro, radio, UI_TINTA);
    DrawCircleV(Vector2{ centro.x - radio * 0.35f, centro.y - radio * 0.35f }, radio * 0.32f, Fade(WHITE, 0.70f));
}


// Postal del tablon. `ref` en unidades de referencia (ya desplazada).
static void DibujarPostal(
    const EstiloUI& estilo,
    int indice,
    Rectangle ref,
    float elevacion,
    bool foco
)
{
    const IdMinijuego id = ObtenerIdMinijuegoPorIndice(indice);
    const DatosMinijuegoCatalogo& datos = ObtenerDatosMinijuego(id);

    // Con foco la postal se despega del tablon: sube y crece un poco.
    float crecer = 0.07f * elevacion;

    Rectangle grande =
    {
        ref.x - ref.width * crecer * 0.5f,
        ref.y - ref.height * crecer * 0.5f - 6.0f * elevacion,
        ref.width * (1.0f + crecer),
        ref.height * (1.0f + crecer)
    };

    Rectangle rect = RectRef(grande.x, grande.y, grande.width, grande.height);

    DrawRectangleRec(
        Rectangle
        {
            rect.x + Px(3.0f + 4.0f * elevacion),
            rect.y + Px(4.0f + 6.0f * elevacion),
            rect.width,
            rect.height
        },
        Fade(BLACK, 0.22f + 0.12f * elevacion)
    );

    DibujarNueveCortes(estilo.madera, 12, rect, EscalaUI() * 0.4f, WHITE);

    Rectangle placa = Encoger(rect, Px(4.0f));
    DibujarNueveCortes(estilo.placa, 8, placa, EscalaUI(), WHITE);

    float margen = Px(5.0f);
    float altoNombre = Px(30.0f);

    Rectangle foto =
    {
        placa.x + margen,
        placa.y + margen,
        placa.width - margen * 2.0f,
        placa.height - margen - altoNombre
    };

    DibujarIlustracionMinijuego(id, foto, true);
    DrawRectangleLinesEx(foto, std::fmax(1.0f, Px(1.5f)), UI_MADERA_OSCURA);

    DibujarNombrePostal(
        datos.nombre,
        placa.x + placa.width * 0.5f,
        foto.y + foto.height,
        placa.width - Px(8.0f),
        altoNombre
    );

    DibujarChinche(
        Vector2{ rect.x + rect.width * 0.5f, rect.y + Px(4.0f) },
        ObtenerColorFormatoMinijuego(ObtenerFormatoMinijuego(id))
    );

    if (foco)
    {
        DibujarMarcoFoco(rect);
    }
}


// Leyenda de formatos (chinche + fichas + nombre), alineada a la derecha.
static void DibujarLeyendaFormatos(
    float xFin,
    float y
)
{
    const int tamano = TamanoTexto(UI_TEXTO_PEQUENO);
    const float altoIcono = Px(13.0f);
    const float anchoIcono = Px(50.0f);
    const float separacion = Px(24.0f);

    float x = xFin;

    for (int f = CANTIDAD_FORMATOS_MINIJUEGO - 1; f >= 0; f--)
    {
        FormatoMinijuego formato = (FormatoMinijuego)f;
        const char* nombre = ObtenerNombreFormatoMinijuego(formato);

        x -= (float)MeasureText(nombre, tamano);

        DrawText(nombre, (int)x, (int)y, tamano, UI_TINTA_SUAVE);

        x -= Px(8.0f) + anchoIcono;

        DibujarIconoFormatoMinijuego(
            formato,
            Vector2{ x + anchoIcono * 0.5f, y + tamano * 0.5f },
            altoIcono
        );

        x -= Px(8.0f);

        DibujarChinche(
            Vector2{ x - Px(4.0f), y + tamano * 0.5f },
            ObtenerColorFormatoMinijuego(formato)
        );

        x -= Px(10.0f) + separacion;
    }
}


// Bandera de un puesto: mastil, driza y bandera que sube con `izado`.
static void DibujarMastil(
    const EstiloUI& estilo,
    float x,
    float arriba,
    float abajo,
    float izado,
    Color color,
    bool flamear
)
{
    // Base y palo.
    DrawRectangleRec(
        Rectangle{ x - Px(12.0f), abajo - Px(6.0f), Px(24.0f), Px(6.0f) },
        UI_MADERA_OSCURA
    );

    DrawLineEx(
        Vector2{ x, arriba },
        Vector2{ x, abajo },
        Px(4.0f),
        UI_MADERA_OSCURA
    );

    DrawCircleV(Vector2{ x, arriba }, Px(4.5f), UI_FOCO);
    DrawCircleLinesV(Vector2{ x, arriba }, Px(4.5f), UI_TINTA);

    float altoBandera = Px(26.0f);
    float yBaja = abajo - Px(10.0f) - altoBandera;
    float yAlta = arriba + Px(6.0f);
    float yBandera = yBaja + (yAlta - yBaja) * izado;

    // Driza: la cuerda que sube la bandera.
    DrawLineEx(
        Vector2{ x + Px(3.0f), arriba },
        Vector2{ x + Px(3.0f), abajo - Px(6.0f) },
        std::fmax(1.0f, Px(1.5f)),
        UI_CUERDA
    );

    float onda = flamear ? std::sin(estilo.tiempo * 5.0f) * Px(3.0f) : 0.0f;

    Vector2 a = { x + Px(3.0f), yBandera };
    Vector2 b = { x + Px(3.0f), yBandera + altoBandera };
    Vector2 punta = { x + Px(42.0f), yBandera + altoBandera * 0.5f + onda };

    DrawTriangle(a, b, punta, color);
    DrawTriangleLines(a, b, punta, UI_TINTA);
}


//==================================================
// ACTUALIZAR
//==================================================

void SeleccionMinijuegos::Inicializar()
{
    indiceSeleccionado = 0;
    indiceAnterior = -1;
    confirmado = false;
    volver = false;
    progresoPanel = 0.0f;
    filaInicial = 0;

    // La direccion que venga mantenida de la pantalla anterior no mueve
    // el foco al entrar.
    bloqueoHorizontal = true;
    bloqueoVertical = true;

    vista = VISTA_TABLON_MINIJUEGOS;
    panelTablon.Iniciar(PANEL_TABLON, 0.0f);

    for (int i = 0; i < CANTIDAD_MINIJUEGOS; i++)
    {
        elevacion[i] = ResorteUI{};
    }

    aparicionFilas = 1.0f;
    direccionDesplazamiento = 0;
    tiempo = 0.0f;

    IniciarPreviaMinijuego(previa, ObtenerIdMinijuegoPorIndice(0), true);

    cacheValida = false;
}


void IniciarPreviaMinijuego(
    PreviaMinijuego& previa,
    IdMinijuego id,
    bool permiteVolver
)
{
    previa.id = id;
    previa.permiteVolver = permiteVolver;
    previa.panel.Iniciar(PANEL_PREVIA, 0.0f);
    previa.tiempo = 0.0f;
    previa.tiempoTodosListos = 0.0f;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        previa.listo[i] = false;
        previa.izado[i] = ResorteUI{};
    }
}


static void AbrirPrevia(
    SeleccionMinijuegos& seleccion
)
{
    seleccion.vista = VISTA_PREVIA_MINIJUEGO;

    // Al volver al tablon (desde la previa o el minijuego), un empuje
    // que siga mantenido no mueve el foco hasta soltarlo.
    seleccion.bloqueoHorizontal = true;
    seleccion.bloqueoVertical = true;

    IniciarPreviaMinijuego(
        seleccion.previa,
        ObtenerIdMinijuegoPorIndice(seleccion.indiceSeleccionado),
        true
    );
}


// Tablon: J1 mueve el foco por la grilla y abre la previa.
static void ActualizarTablon(
    SeleccionMinijuegos& seleccion,
    const Participante& jugadorUno
)
{
    InputSeleccionParticipante entrada =
        LeerInputSeleccionParticipante(jugadorUno);

    int nuevoIndice = seleccion.indiceSeleccionado;
    int filaPrevia = seleccion.filaInicial;
    bool abrir = false;

    // El teclado dispara una vez por pulsacion; el mando usa bloqueos
    // para que un empuje sea un solo paso (igual que SeleccionPersonajes).
    const bool esGamepad = jugadorUno.control == CONTROL_GAMEPAD;
    const bool puedeHorizontal = !esGamepad || !seleccion.bloqueoHorizontal;
    const bool puedeVertical = !esGamepad || !seleccion.bloqueoVertical;

    if (entrada.izquierda && puedeHorizontal)
        nuevoIndice = MoverIndice(nuevoIndice, -1, 0);

    if (entrada.derecha && puedeHorizontal)
        nuevoIndice = MoverIndice(nuevoIndice, 1, 0);

    if (entrada.arriba && puedeVertical)
        nuevoIndice = MoverIndice(nuevoIndice, 0, -1);

    if (entrada.abajo && puedeVertical)
        nuevoIndice = MoverIndice(nuevoIndice, 0, 1);

    // El bloqueo queda puesto mientras haya direccion y se suelta al
    // volver al centro. Un mando desconectado no da entrada, asi que
    // el bloqueo se suelta solo y no queda pegado.
    seleccion.bloqueoHorizontal = entrada.izquierda || entrada.derecha;
    seleccion.bloqueoVertical = entrada.arriba || entrada.abajo;

    // La rueda desplaza la grilla sin cambiar la seleccion.
    int& filaInicial = seleccion.filaInicial;
    float rueda = GetMouseWheelMove();
    if (rueda > 0.0f) filaInicial--;
    if (rueda < 0.0f) filaInicial++;
    if (filaInicial < 0) filaInicial = 0;
    if (filaInicial > ObtenerFilaInicialMaxima()) filaInicial = ObtenerFilaInicialMaxima();

    Vector2 mouse = GetMousePosition();
    Vector2 deltaMouse = GetMouseDelta();

    // El mouse solo elige si se movio o hizo clic; asi una flecha que
    // desplaza la grilla no queda pisada por un cursor quieto.
    bool mouseActivo =
        deltaMouse.x != 0.0f ||
        deltaMouse.y != 0.0f ||
        IsMouseButtonPressed(MOUSE_BUTTON_LEFT);

    for (int i = 0; i < CANTIDAD_MINIJUEGOS && mouseActivo; i++)
    {
        if (!EsCeldaVisible(i, filaInicial))
        {
            continue;
        }

        Rectangle celda = ObtenerCeldaCatalogo(i, filaInicial);

        if (CheckCollisionPointRec(mouse, celda))
        {
            nuevoIndice = i;

            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            {
                abrir = true;
            }
        }
    }

    // Al cambiar la seleccion, mantener visible su fila.
    if (nuevoIndice != seleccion.indiceSeleccionado)
    {
        int filaSeleccion = nuevoIndice / COLUMNAS_CATALOGO;
        if (filaSeleccion < filaInicial) filaInicial = filaSeleccion;
        if (filaSeleccion >= filaInicial + FILAS_VISIBLES_CATALOGO)
        {
            filaInicial = filaSeleccion - FILAS_VISIBLES_CATALOGO + 1;
        }
    }

    if (filaInicial != filaPrevia)
    {
        seleccion.direccionDesplazamiento = filaInicial > filaPrevia ? 1 : -1;
        seleccion.aparicionFilas = 0.0f;
    }

    if (nuevoIndice != seleccion.indiceSeleccionado)
    {
        seleccion.indiceAnterior = seleccion.indiceSeleccionado;
        seleccion.indiceSeleccionado = nuevoIndice;
        seleccion.progresoPanel = 0.0f;
    }

    if (entrada.confirmar)
    {
        abrir = true;
    }

    if (entrada.cancelar || IsKeyPressed(KEY_ESCAPE))
    {
        seleccion.volver = true;
        return;
    }

    if (abrir)
    {
        AbrirPrevia(seleccion);
    }
}


// Previa: cada humano iza su bandera; con todas arriba empieza el juego.
ResultadoPreviaMinijuego ActualizarPreviaMinijuego(
    PreviaMinijuego& previa,
    float deltaTime,
    const Participante participantes[],
    int cantidad
)
{
    const EstiloUI& estilo = ObtenerEstiloUI();

    previa.tiempo += deltaTime;

    const bool puedeLeer = previa.tiempo >= BLOQUEO_ENTRADA_PREVIA;
    bool volver = previa.permiteVolver && puedeLeer && IsKeyPressed(KEY_ESCAPE);

    for (int i = 0; i < cantidad && i < MAX_PARTICIPANTES; i++)
    {
        const Participante& participante = participantes[i];

        if (!participante.activo)
        {
            previa.listo[i] = false;
            continue;
        }

        // Bots listos solos; un mando desconectado no deja la previa
        // esperando para siempre.
        if (participante.esBot || !participante.conectado)
        {
            previa.listo[i] = true;
            continue;
        }

        if (!puedeLeer)
        {
            continue;
        }

        InputSeleccionParticipante entrada =
            LeerInputSeleccionParticipante(participante);

        if (entrada.confirmar)
        {
            previa.listo[i] = true;
        }
        else if (entrada.cancelar)
        {
            // Primero baja la bandera; J1 sin bandera vuelve al tablon
            // (si hay tablon al que volver).
            if (previa.listo[i])
            {
                previa.listo[i] = false;
            }
            else if (i == 0 && previa.permiteVolver)
            {
                volver = true;
            }
        }
    }

    // Clic sobre el puesto de J1: sube o baja su bandera.
    if (
        puedeLeer &&
        cantidad > 0 &&
        EsHumanoConectado(participantes[0]) &&
        IsMouseButtonPressed(MOUSE_BUTTON_LEFT)
    )
    {
        Rectangle puesto = ObtenerPuestoRef(0);

        if (CheckCollisionPointRec(
                GetMousePosition(),
                RectRef(puesto.x, puesto.y, puesto.width, puesto.height)
            ))
        {
            previa.listo[0] = !previa.listo[0];
        }
    }

    if (volver)
    {
        return PREVIA_VOLVER;
    }

    ActualizarPanelUI(previa.panel, estilo, deltaTime);

    bool todosListos = cantidad > 0;
    int bot = 0;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        bool activo = i < cantidad && participantes[i].activo;
        bool arriba = activo && previa.listo[i];

        if (activo && !previa.listo[i])
        {
            todosListos = false;
        }

        // Los bots izan en orden, despues de que el cartel aparece.
        if (arriba && participantes[i].esBot)
        {
            arriba =
                previa.tiempo >=
                RETRASO_IZADO_BOT + bot * ESCALONADO_IZADO_BOT;

            bot++;
        }

        float objetivo = arriba ? 1.0f : 0.0f;

        if (estilo.reducirMovimiento)
        {
            previa.izado[i].valor = objetivo;
            previa.izado[i].velocidad = 0.0f;
        }
        else
        {
            previa.izado[i].Actualizar(objetivo, deltaTime, 90.0f, 13.0f);
        }
    }

    previa.tiempoTodosListos =
        todosListos ? previa.tiempoTodosListos + deltaTime : 0.0f;

    if (previa.tiempoTodosListos >= ESPERA_TODOS_LISTOS)
    {
        previa.tiempoTodosListos = 0.0f;
        return PREVIA_CONFIRMADA;
    }

    return PREVIA_ESPERANDO;
}


void SeleccionMinijuegos::Actualizar(
    float deltaTime,
    const Participante& jugadorUno
)
{
    Actualizar(deltaTime, &jugadorUno, 1);
}


void SeleccionMinijuegos::Actualizar(
    float deltaTime,
    const Participante participantes[],
    int cantidad
)
{
    confirmado = false;
    volver = false;

    const EstiloUI& estilo = ObtenerEstiloUI();

    tiempo += deltaTime;

    ActualizarPanelUI(panelTablon, estilo, deltaTime);

    if (cantidad > 0)
    {
        if (vista == VISTA_TABLON_MINIJUEGOS)
        {
            ActualizarTablon(*this, participantes[0]);
        }
        else
        {
            ResultadoPreviaMinijuego resultado =
                ActualizarPreviaMinijuego(previa, deltaTime, participantes, cantidad);

            // Al volver del minijuego se ve otra vez el tablon.
            if (resultado != PREVIA_ESPERANDO)
            {
                vista = VISTA_TABLON_MINIJUEGOS;
            }

            if (resultado == PREVIA_CONFIRMADA)
            {
                confirmado = true;
            }
        }
    }

    // Juego cambia de pantalla en este mismo frame: la textura de la
    // grilla no se vuelve a usar hasta regresar (y ahi se regenera).
    if (confirmado || volver)
    {
        Descargar();
    }

    progresoPanel += deltaTime * 6.0f;
    if (progresoPanel > 1.0f) progresoPanel = 1.0f;

    aparicionFilas += deltaTime / DURACION_ENTRADA_FILAS;
    if (aparicionFilas > 1.0f || estilo.reducirMovimiento) aparicionFilas = 1.0f;

    for (int i = 0; i < CANTIDAD_MINIJUEGOS; i++)
    {
        float objetivo = i == indiceSeleccionado ? 1.0f : 0.0f;

        if (estilo.reducirMovimiento)
        {
            elevacion[i].valor = objetivo;
            elevacion[i].velocidad = 0.0f;
        }
        else
        {
            elevacion[i].Actualizar(objetivo, deltaTime, 260.0f, 22.0f);
        }
    }
}


//==================================================
// DIBUJAR
//==================================================

static void DibujarFichaTablon(
    const SeleccionMinijuegos& seleccion,
    const EstiloUI& estilo,
    float dy
)
{
    const IdMinijuego id = ObtenerIdMinijuegoPorIndice(seleccion.indiceSeleccionado);
    const DatosMinijuegoCatalogo& datos = ObtenerDatosMinijuego(id);
    const FormatoMinijuego formato = ObtenerFormatoMinijuego(id);

    Rectangle rect = RectRef(
        FICHA_TABLON.x,
        FICHA_TABLON.y + dy,
        FICHA_TABLON.width,
        FICHA_TABLON.height
    );

    Rectangle placa = DibujarTarjetaMadera(estilo, rect, 0.22f);

    Rectangle cuadro =
    {
        placa.x + Px(8.0f),
        placa.y + Px(8.0f),
        Px(210.0f),
        placa.height - Px(16.0f)
    };

    DibujarCuadro(estilo, id, cuadro, 7.0f);

    // Al cambiar de minijuego el texto entra deslizandose desde la derecha.
    float t = Suavizar(seleccion.progresoPanel);
    float desliza = estilo.reducirMovimiento ? 0.0f : (1.0f - t) * Px(18.0f);

    float x = cuadro.x + cuadro.width + Px(20.0f) + desliza;
    float ancho = placa.x + placa.width - Px(16.0f) - x;

    DibujarTextoAjustadoUI(
        datos.nombre,
        x,
        placa.y + Px(10.0f),
        UI_TEXTO_GRANDE,
        ancho,
        UI_TINTA,
        false
    );

    float ySellos = placa.y + Px(48.0f);
    float anchoSello = DibujarSello(
        ObtenerNombreFormatoMinijuego(formato),
        x,
        ySellos,
        ObtenerColorFormatoMinijuego(formato),
        formato
    );

    DibujarSello(
        ObtenerTextoJugadoresMinijuego(id),
        x + anchoSello + Px(10.0f),
        ySellos,
        UI_MADERA_MEDIA
    );

    DibujarParrafo(
        datos.descripcion,
        x,
        placa.y + Px(84.0f),
        UI_TEXTO,
        ancho,
        2,
        UI_TINTA
    );
}


static bool DibujaFija(
    CapaDibujoMinijuegos capa
)
{
    return capa != CAPA_VIVA;
}


static bool DibujaViva(
    CapaDibujoMinijuegos capa
)
{
    return capa != CAPA_FIJA;
}


// Lo vivo del tablon es solo la cabecera (posicion y quien elige); no
// se superpone con nada fijo, asi que dibujarla al final no cambia la
// imagen.
static void DibujarTablon(
    const SeleccionMinijuegos& seleccion,
    const EstiloUI& estilo,
    const Participante& jugadorUno,
    CapaDibujoMinijuegos capa
)
{
    if (DibujaFija(capa))
    {
        DibujarPanelUI(estilo, seleccion.panelTablon);
    }

    if (!estilo.cargado || seleccion.panelTablon.colgado.retraso > 0.0f)
    {
        return;
    }

    // Todo el contenido acompana la caida y el rebote del tablon.
    const float dy = seleccion.panelTablon.colgado.caida.valor;

    //------------------------------
    // CABECERA: posicion, quien elige y leyenda de formatos
    //------------------------------

    if (DibujaViva(capa))
    {
        Vector2 cabecera = PuntoRef(AREA_GRILLA.x, 114.0f + dy);

        const char* posicion = TextFormat(
            "MINIJUEGO %d DE %d",
            seleccion.indiceSeleccionado + 1,
            CANTIDAD_MINIJUEGOS
        );

        DibujarTextoTinta(posicion, cabecera.x, cabecera.y, UI_TEXTO, UI_TINTA);

        float xElige =
            cabecera.x + (float)MeasureText(posicion, TamanoTexto(UI_TEXTO)) + Px(18.0f);

        const char* elige = TextFormat(
            "ELIGE J1 (%s)",
            ObtenerNombreControlParticipante(jugadorUno)
        );

        DibujarTextoTinta(elige, xElige, cabecera.y + Px(3.0f), UI_TEXTO_PEQUENO, UI_TINTA_SUAVE);
    }

    if (!DibujaFija(capa))
    {
        return;
    }

    Vector2 finLeyenda = PuntoRef(AREA_GRILLA.x + AREA_GRILLA.width, 117.0f + dy);
    DibujarLeyendaFormatos(finLeyenda.x, finLeyenda.y);

    //------------------------------
    // GRILLA DE POSTALES
    //------------------------------

    // Al desplazar, las postales llegan desde el lado por el que se avanzo.
    const float entrada = estilo.reducirMovimiento
        ? 0.0f
        : (1.0f - Suavizar(seleccion.aparicionFilas)) * 14.0f *
            (float)seleccion.direccionDesplazamiento;

    for (int pasada = 0; pasada < 2; pasada++)
    {
        for (int i = 0; i < CANTIDAD_MINIJUEGOS; i++)
        {
            if (!EsCeldaVisible(i, seleccion.filaInicial))
            {
                continue;
            }

            // La postal con foco se dibuja la ultima, por encima.
            bool foco = i == seleccion.indiceSeleccionado;

            if ((pasada == 0) == foco)
            {
                continue;
            }

            Rectangle ref = ObtenerCeldaRef(i, seleccion.filaInicial);
            ref.y += dy + entrada;

            DibujarPostal(
                estilo,
                i,
                ref,
                Limitar01(seleccion.elevacion[i].valor),
                foco && seleccion.vista == VISTA_TABLON_MINIJUEGOS
            );
        }
    }

    //------------------------------
    // CUERDA DE DESPLAZAMIENTO
    //------------------------------

    if (ObtenerFilaInicialMaxima() > 0)
    {
        float arriba = AREA_GRILLA.y;
        float abajo = AREA_GRILLA.y + AREA_GRILLA.height;

        DibujarCuerdaUI(
            estilo,
            PuntoRef(X_CUERDA_DESPLAZAMIENTO, arriba + dy),
            PuntoRef(X_CUERDA_DESPLAZAMIENTO, abajo + dy)
        );

        float proporcion =
            (float)FILAS_VISIBLES_CATALOGO /
            (float)ObtenerFilasTotalesCatalogo();
        float avance =
            (float)seleccion.filaInicial / (float)ObtenerFilaInicialMaxima();
        float altoTaco = (abajo - arriba) * proporcion;
        float yTaco = arriba + (abajo - arriba - altoTaco) * avance;

        Rectangle taco = RectRef(
            X_CUERDA_DESPLAZAMIENTO - 7.0f,
            yTaco + dy,
            14.0f,
            altoTaco
        );

        DrawRectangleRec(
            Rectangle{ taco.x + Px(2.0f), taco.y + Px(3.0f), taco.width, taco.height },
            Fade(BLACK, 0.25f)
        );

        DibujarNueveCortes(estilo.madera, 12, taco, EscalaUI() * 0.4f, WHITE);
    }

    DibujarFichaTablon(seleccion, estilo, dy);
}


static void DibujarControlesPrevia(
    const EstiloUI& estilo,
    const Participante participantes[],
    int cantidad,
    float dy
)
{
    Rectangle rect = RectRef(
        TARJETA_CONTROLES.x,
        TARJETA_CONTROLES.y + dy,
        TARJETA_CONTROLES.width,
        TARJETA_CONTROLES.height
    );

    Rectangle placa = DibujarTarjetaMadera(estilo, rect, 0.20f);

    const float relleno = Px(14.0f);
    const float xTeclado = placa.x + Px(150.0f);
    const float xMando = placa.x + Px(410.0f);
    const float anchoTeclado = xMando - xTeclado - Px(12.0f);
    const float anchoMando = placa.x + placa.width - relleno - xMando;

    DibujarTextoTinta("CONTROLES BÁSICOS", placa.x + relleno, placa.y + Px(9.0f), UI_TEXTO, UI_TINTA);

    DrawRectangleRec(
        Rectangle{ placa.x + relleno, placa.y + Px(34.0f), placa.width - relleno * 2.0f, Px(2.0f) },
        Fade(UI_MADERA_OSCURA, 0.45f)
    );

    // Encabezados de columna bajo el titulo (asi no lo pisan).
    DibujarTextoTinta("TECLADO", xTeclado, placa.y + Px(42.0f), UI_TEXTO_PEQUENO, UI_TINTA_SUAVE);
    DibujarTextoTinta("MANDO", xMando, placa.y + Px(42.0f), UI_TEXTO_PEQUENO, UI_TINTA_SUAVE);

    // Teclas de InputMinijuegoParticipante segun los teclados en juego:
    // WASD + ESPACIO + E y flechas + ENTER + SHIFT DER; el teclado
    // completo mueve con ambos pero salta con ESPACIO y golpea con E.
    const int mascara = ObtenerMascaraControles(participantes, cantidad);
    const bool completo = (mascara & (1 << CONTROL_TECLADO_COMPLETO)) != 0;
    const bool wasd = (mascara & (1 << CONTROL_TECLADO_WASD)) != 0;
    const bool flechas = (mascara & (1 << CONTROL_TECLADO_FLECHAS)) != 0;

    const char* teclado[3] = { "WASD O FLECHAS", "ESPACIO", "E" };

    if (!completo && wasd && flechas)
    {
        teclado[0] = "WASD  |  FLECHAS";
        teclado[1] = "ESPACIO  |  ENTER";
        teclado[2] = "E  |  SHIFT DER";
    }
    else if (!completo && flechas)
    {
        teclado[0] = "FLECHAS";
        teclado[1] = "ENTER";
        teclado[2] = "SHIFT DER";
    }
    else if (!completo && wasd)
    {
        teclado[0] = "WASD";
    }

    const char* acciones[3] = { "MOVER", "SALTAR", "ACCIÓN" };
    const char* mando[3] = { "STICK O CRUCETA", "A", "B" };

    for (int fila = 0; fila < 3; fila++)
    {
        float y = placa.y + Px(64.0f + fila * 21.0f);

        DibujarTextoTinta(acciones[fila], placa.x + relleno, y, UI_TEXTO_PEQUENO, UI_TINTA_SUAVE);
        DibujarTextoAjustadoUI(teclado[fila], xTeclado, y, UI_TEXTO_PEQUENO, anchoTeclado, UI_TINTA, false);
        DibujarTextoAjustadoUI(mando[fila], xMando, y, UI_TEXTO_PEQUENO, anchoMando, UI_TINTA, false);
    }
}


// Lo fijo de un puesto es la tarjeta; banderas, sellos y textos van
// vivos (el tinte de "listo" pinta la placa por encima de la tarjeta).
static void DibujarPuesto(
    const PreviaMinijuego& previa,
    const EstiloUI& estilo,
    const Participante& participante,
    int indice,
    float dy,
    bool todosListos,
    CapaDibujoMinijuegos capa
)
{
    Rectangle ref = ObtenerPuestoRef(indice);
    Rectangle rect = RectRef(ref.x, ref.y + dy, ref.width, ref.height);

    const bool humano = !participante.esBot;
    const bool desconectado = humano && !participante.conectado;
    const bool listo = previa.listo[indice];

    // Misma placa que devuelve DibujarTarjetaMadera.
    Rectangle placa = Encoger(rect, Px(7.0f));

    if (DibujaFija(capa))
    {
        DibujarTarjetaMadera(estilo, rect, 0.20f);
    }

    if (!DibujaViva(capa))
    {
        return;
    }

    if (listo)
    {
        DrawRectangleRec(placa, Fade(UI_POSITIVO, todosListos ? 0.16f : 0.08f));
    }

    DibujarMastil(
        estilo,
        placa.x + Px(28.0f),
        placa.y + Px(14.0f),
        placa.y + placa.height - Px(10.0f),
        Limitar01(previa.izado[indice].valor),
        humano ? participante.color : COLOR_BOT,
        listo && !estilo.reducirMovimiento
    );

    const float x = placa.x + Px(80.0f);
    const float ancho = placa.x + placa.width - Px(10.0f) - x;

    DibujarTextoTinta(TextFormat("J%d", indice + 1), x, placa.y + Px(10.0f), UI_TEXTO_GRANDE, UI_TINTA);

    DibujarTextoAjustadoUI(
        ObtenerNombreControlParticipante(participante),
        x,
        placa.y + Px(46.0f),
        UI_TEXTO_PEQUENO,
        ancho,
        UI_TINTA_SUAVE,
        false
    );

    const char* sello = listo ? "LISTO" : "ESPERANDO";
    Color colorSello = listo ? UI_POSITIVO : UI_FOCO;
    const char* pista = nullptr;

    if (desconectado)
    {
        sello = "SIN CONEXIÓN";
        colorSello = UI_PELIGRO;
        pista = "NO SE LO ESPERA";
    }
    else if (!humano)
    {
        pista = "BOT: SIEMPRE LISTO";
    }
    else if (listo)
    {
        pista = TextFormat("%s: BAJAR", ObtenerTeclaBajar(participante));
    }
    else
    {
        pista = TextFormat("PULSÁ %s", ObtenerTeclaListo(participante));
    }

    DibujarSello(sello, x, placa.y + Px(72.0f), colorSello);

    DibujarTextoAjustadoUI(
        pista,
        x,
        placa.y + Px(112.0f),
        UI_TEXTO_PEQUENO,
        ancho,
        UI_TINTA,
        false
    );
}


// 0..1: entrada del velo y del titulo de la previa.
static float AparicionPrevia(
    const PreviaMinijuego& previa,
    const EstiloUI& estilo
)
{
    return estilo.reducirMovimiento
        ? 1.0f
        : Limitar01(previa.tiempo / 0.25f);
}


// Lo vivo de la previa es el estado de la tripulacion y lo que cambia
// en cada puesto; no se superpone con nada fijo dibujado despues.
static void DibujarCartelPrevia(
    const PreviaMinijuego& previa,
    const EstiloUI& estilo,
    const Participante participantes[],
    int cantidad,
    CapaDibujoMinijuegos capa
)
{
    const float aparicion = AparicionPrevia(previa, estilo);

    if (DibujaFija(capa))
    {
        // Velo extra: el tablon queda atras, apagado.
        DrawRectangle(
            0,
            0,
            GetScreenWidth(),
            GetScreenHeight(),
            Fade(Color{ 16, 10, 22, 255 }, 0.55f * aparicion)
        );

        Vector2 titulo = PuntoRef(640.0f, 20.0f);
        DibujarTextoMundo("PRÓXIMO MINIJUEGO", titulo.x, titulo.y, UI_TEXTO_TITULO, Fade(UI_TEXTO_MUNDO, aparicion));

        DibujarPanelUI(estilo, previa.panel);
    }

    if (!estilo.cargado || previa.panel.colgado.retraso > 0.0f)
    {
        return;
    }

    const float dy = previa.panel.colgado.caida.valor;

    const IdMinijuego id = previa.id;
    const DatosMinijuegoCatalogo& datos = ObtenerDatosMinijuego(id);
    const FormatoMinijuego formato = ObtenerFormatoMinijuego(id);

    if (DibujaFija(capa))
    {
        //------------------------------
        // ILUSTRACION Y FORMATO
        //------------------------------

        DibujarCuadro(
            estilo,
            id,
            RectRef(
                MARCO_ILUSTRACION_PREVIA.x,
                MARCO_ILUSTRACION_PREVIA.y + dy,
                MARCO_ILUSTRACION_PREVIA.width,
                MARCO_ILUSTRACION_PREVIA.height
            ),
            12.0f
        );

        Vector2 sellos = PuntoRef(
            MARCO_ILUSTRACION_PREVIA.x,
            MARCO_ILUSTRACION_PREVIA.y + MARCO_ILUSTRACION_PREVIA.height + 16.0f + dy
        );

        float anchoSello = DibujarSello(
            ObtenerNombreFormatoMinijuego(formato),
            sellos.x,
            sellos.y,
            ObtenerColorFormatoMinijuego(formato),
            formato
        );

        DibujarSello(
            ObtenerTextoJugadoresMinijuego(id),
            sellos.x + anchoSello + Px(10.0f),
            sellos.y,
            UI_MADERA_MEDIA
        );

        //------------------------------
        // NOMBRE, DESCRIPCION Y CONTROLES
        //------------------------------

        Vector2 info = PuntoRef(X_INFO_PREVIA, 110.0f + dy);
        const float anchoInfo = Px(ANCHO_INFO_PREVIA);

        DibujarTextoAjustadoUI(datos.nombre, info.x, info.y, UI_TEXTO_GRANDE, anchoInfo, UI_TINTA, false);

        Rectangle cinta = { info.x, info.y + Px(38.0f), anchoInfo, Px(6.0f) };
        DrawRectangleRec(cinta, datos.color);
        DrawRectangleLinesEx(cinta, 1.0f, UI_TINTA);

        DibujarParrafo(datos.descripcion, info.x, info.y + Px(58.0f), UI_TEXTO, anchoInfo, 4, UI_TINTA);

        DibujarControlesPrevia(estilo, participantes, cantidad, dy);
    }

    //------------------------------
    // TRIPULACION: una bandera por jugador
    //------------------------------

    int esperando = 0;
    bool todosListos = cantidad > 0;

    for (int i = 0; i < cantidad && i < MAX_PARTICIPANTES; i++)
    {
        if (participantes[i].activo && !previa.listo[i])
        {
            todosListos = false;
            esperando++;
        }
    }

    Vector2 cabecera = PuntoRef(X_TRIPULACION, Y_TRIPULACION + dy);

    if (DibujaFija(capa))
    {
        DibujarTextoTinta("TRIPULACIÓN", cabecera.x, cabecera.y, UI_TEXTO, UI_TINTA);

        DibujarTextoTinta(
            "IZÁ TU BANDERA PARA EMPEZAR",
            cabecera.x + (float)MeasureText("TRIPULACIÓN", TamanoTexto(UI_TEXTO)) + Px(16.0f),
            cabecera.y + Px(3.0f),
            UI_TEXTO_PEQUENO,
            UI_TINTA_SUAVE
        );
    }

    if (DibujaViva(capa))
    {
        const char* estado = todosListos
            ? "TODOS LISTOS: A JUGAR"
            : (esperando == 1 ? "FALTA 1 JUGADOR" : TextFormat("FALTAN %d JUGADORES", esperando));

        const int tamanoPequeno = TamanoTexto(UI_TEXTO_PEQUENO);
        float anchoEstado = (float)MeasureText(estado, tamanoPequeno) + Px(16.0f);
        Vector2 finCabecera = PuntoRef(X_TRIPULACION + ANCHO_TRIPULACION, Y_TRIPULACION - 3.0f + dy);

        DibujarSello(
            estado,
            finCabecera.x - anchoEstado,
            finCabecera.y,
            todosListos ? UI_POSITIVO : UI_FOCO
        );
    }

    // Con todo dibujado junto, cada puesto completo uno tras otro (como
    // siempre); separado, las tarjetas van en la cache y el resto vivo.
    for (int i = 0; i < cantidad && i < MAX_PARTICIPANTES; i++)
    {
        if (participantes[i].activo)
        {
            DibujarPuesto(previa, estilo, participantes[i], i, dy, todosListos, capa);
        }
    }
}


static void DibujarAyudaPrevia(
    const PreviaMinijuego& previa,
    const EstiloUI& estilo,
    const Participante participantes[],
    int cantidad
)
{
    const int mascara = ObtenerMascaraControles(participantes, cantidad);

    char confirmar[64] = "";
    char cancelar[64] = "";

    ArmarTeclasConfirmar(mascara, confirmar, sizeof(confirmar));
    ArmarTeclasCancelar(mascara, false, cancelar, sizeof(cancelar));

    const char* teclasCancelar = cancelar[0] != '\0' ? cancelar : "RETROCESO";

    // En la ruleta no hay tablon al que volver: cancelar solo baja la
    // propia bandera.
    if (previa.permiteVolver)
    {
        DibujarAyudaSeleccion(
            estilo,
            TextFormat("%s: ESTOY LISTO", confirmar),
            TextFormat("%s: CANCELAR", teclasCancelar),
            "ESC: VOLVER AL TABLÓN"
        );
    }
    else
    {
        DibujarAyudaSeleccion(
            estilo,
            TextFormat("%s: ESTOY LISTO", confirmar),
            TextFormat("%s: BAJAR BANDERA", teclasCancelar),
            nullptr
        );
    }
}


void DibujarPreviaMinijuego(
    const PreviaMinijuego& previa,
    const Participante participantes[],
    int cantidad,
    CapaDibujoMinijuegos capa
)
{
    const EstiloUI& estilo = ObtenerEstiloUI();

    DibujarCartelPrevia(previa, estilo, participantes, cantidad, capa);

    if (DibujaViva(capa))
    {
        DibujarAyudaPrevia(previa, estilo, participantes, cantidad);
    }
}


bool PreviaMinijuegoQuieta(
    const PreviaMinijuego& previa
)
{
    const EstiloUI& estilo = ObtenerEstiloUI();

    return
        estilo.cargado &&
        !previa.panel.colgado.EnMovimiento() &&
        AparicionPrevia(previa, estilo) >= 1.0f;
}


//==================================================
// CACHE DE PANTALLA (compartida con la ruleta)
//==================================================

bool DibujarEnCacheMinijuegos(
    RenderTexture2D& cache,
    void (*dibujar)(const void* datos),
    const void* datos
)
{
    const int ancho = GetScreenWidth();
    const int alto = GetScreenHeight();

    if (ancho <= 0 || alto <= 0)
    {
        return false;
    }

    // Ventana de otro tamano: textura nueva.
    if (
        cache.id != 0 &&
        (cache.texture.width != ancho || cache.texture.height != alto)
    )
    {
        UnloadRenderTexture(cache);
        cache = RenderTexture2D{};
    }

    if (cache.id == 0)
    {
        cache = LoadRenderTexture(ancho, alto);

        if (cache.id == 0)
        {
            return false;
        }
    }

    BeginTextureMode(cache);
    ClearBackground(BLANK);

    // La textura resuelve al reves que la pantalla los bordes que caen
    // justo en el centro de un pixel (lineas de 1,5 px, por ejemplo).
    // Dibujando con la y hacia arriba coinciden pixel a pixel; a cambio
    // se invierten el sentido de los triangulos (se descartan los de
    // adelante en vez de los de atras) y el recorte de las fotos.
    rlDrawRenderBatchActive();
    rlMatrixMode(RL_PROJECTION);
    rlLoadIdentity();
    rlOrtho(0.0, (double)ancho, 0.0, (double)alto, 0.0, 1.0);
    rlMatrixMode(RL_MODELVIEW);
    rlLoadIdentity();
    rlSetCullFace(RL_CULL_FACE_FRONT);
    EstablecerProyeccionInvertidaMiniaturas(true);

    // Color premultiplicado y alfa acumulado: compuesta con
    // BLEND_ALPHA_PREMULTIPLY, la textura da el mismo resultado que
    // dibujar la capa directo sobre la pantalla.
    rlSetBlendFactorsSeparate(
        RL_SRC_ALPHA,
        RL_ONE_MINUS_SRC_ALPHA,
        RL_ONE,
        RL_ONE_MINUS_SRC_ALPHA,
        RL_FUNC_ADD,
        RL_FUNC_ADD
    );
    BeginBlendMode(BLEND_CUSTOM_SEPARATE);

    dibujar(datos);

    rlDrawRenderBatchActive();
    rlSetCullFace(RL_CULL_FACE_BACK);
    EstablecerProyeccionInvertidaMiniaturas(false);

    EndBlendMode();
    EndTextureMode();

    return true;
}


void ComponerCacheMinijuegos(
    const RenderTexture2D& cache
)
{
    // La textura guarda color premultiplicado y ya esta al derecho
    // (ver DibujarEnCacheMinijuegos): se compone con esa mezcla.
    BeginBlendMode(BLEND_ALPHA_PREMULTIPLY);

    DrawTextureRec(
        cache.texture,
        Rectangle
        {
            0.0f,
            0.0f,
            (float)cache.texture.width,
            (float)cache.texture.height
        },
        Vector2{ 0.0f, 0.0f },
        WHITE
    );

    EndBlendMode();
}


unsigned int FirmaParticipantesMinijuegos(
    const Participante participantes[],
    int cantidad
)
{
    unsigned int firma = 0;

    for (int i = 0; i < cantidad && i < MAX_PARTICIPANTES; i++)
    {
        const Participante& p = participantes[i];

        firma |=
            (unsigned int)(
                (p.activo ? 1 : 0) |
                (p.esBot ? 2 : 0) |
                (p.conectado ? 4 : 0) |
                (((int)p.control & 7) << 3)
            ) << (i * 6);
    }

    return firma;
}


//==================================================
// CACHE DE LO FIJO
//==================================================

// Postal quieta: llego a su altura (1 con foco, 0 sin foco) y no se mueve.
static bool PostalQuieta(
    const SeleccionMinijuegos& seleccion,
    int indice
)
{
    float objetivo = indice == seleccion.indiceSeleccionado ? 1.0f : 0.0f;

    return
        std::fabs(seleccion.elevacion[indice].valor - objetivo) < 0.002f &&
        std::fabs(seleccion.elevacion[indice].velocidad) < 0.02f;
}


// La cache solo se usa con todo lo fijo quieto: tablon asentado, filas
// en su lugar, ficha y postales sin animar y, en la previa, su cartel
// asentado y su velo completo.
static bool PantallaQuieta(
    const SeleccionMinijuegos& seleccion,
    const EstiloUI& estilo
)
{
    if (
        !estilo.cargado ||
        seleccion.panelTablon.colgado.EnMovimiento() ||
        seleccion.aparicionFilas < 1.0f ||
        seleccion.progresoPanel < 1.0f
    )
    {
        return false;
    }

    for (int i = 0; i < CANTIDAD_MINIJUEGOS; i++)
    {
        if (EsCeldaVisible(i, seleccion.filaInicial) && !PostalQuieta(seleccion, i))
        {
            return false;
        }
    }

    if (seleccion.vista == VISTA_PREVIA_MINIJUEGO)
    {
        return PreviaMinijuegoQuieta(seleccion.previa);
    }

    return true;
}


// Centesimas de unidad: la cache sigue valiendo mientras el tablon o el
// cartel no se corran mas de eso (0,015 px a 1080p).
static int CentesimasRef(
    float valor
)
{
    return (int)std::lround(valor * 100.0f);
}


// Todo lo que cambia la imagen fija. Los participantes entran por los
// textos de controles y por el "ELIGE J1" de la cabecera bajo la previa.
static ClaveCacheSeleccion ArmarClaveCache(
    const SeleccionMinijuegos& seleccion,
    const Participante participantes[],
    int cantidad
)
{
    ClaveCacheSeleccion clave;

    clave.ancho = GetScreenWidth();
    clave.alto = GetScreenHeight();
    clave.vista = (int)seleccion.vista;
    clave.indice = seleccion.indiceSeleccionado;
    clave.fila = seleccion.filaInicial;
    clave.dyTablon = CentesimasRef(seleccion.panelTablon.colgado.caida.valor);
    clave.cantidad = cantidad;
    clave.participantes = FirmaParticipantesMinijuegos(participantes, cantidad);

    if (seleccion.vista == VISTA_PREVIA_MINIJUEGO)
    {
        clave.idPrevia = (int)seleccion.previa.id;
        clave.dyPrevia = CentesimasRef(seleccion.previa.panel.colgado.caida.valor);
    }

    return clave;
}


static bool ClavesIguales(
    const ClaveCacheSeleccion& a,
    const ClaveCacheSeleccion& b
)
{
    return
        a.ancho == b.ancho &&
        a.alto == b.alto &&
        a.vista == b.vista &&
        a.indice == b.indice &&
        a.fila == b.fila &&
        a.dyTablon == b.dyTablon &&
        a.dyPrevia == b.dyPrevia &&
        a.idPrevia == b.idPrevia &&
        a.cantidad == b.cantidad &&
        a.participantes == b.participantes;
}


// Una pasada completa de la pantalla (sin la barra de ayuda).
static void DibujarCapaSeleccion(
    const SeleccionMinijuegos& seleccion,
    const EstiloUI& estilo,
    const Participante participantes[],
    int cantidad,
    CapaDibujoMinijuegos capa
)
{
    Participante sinJugador{};
    const Participante& jugadorUno = cantidad > 0 ? participantes[0] : sinJugador;

    if (DibujaFija(capa))
    {
        DibujarVeloPlaya(1.0f);

        if (seleccion.vista == VISTA_TABLON_MINIJUEGOS)
        {
            Vector2 titulo = PuntoRef(640.0f, 22.0f);
            DibujarTextoMundo("MINIJUEGOS", titulo.x, titulo.y, UI_TEXTO_TITULO, UI_TEXTO_MUNDO);
        }
    }

    if (seleccion.vista == VISTA_TABLON_MINIJUEGOS)
    {
        DibujarTablon(seleccion, estilo, jugadorUno, capa);
        return;
    }

    // Bajo la previa el tablon entero queda fijo (cabecera incluida).
    if (DibujaFija(capa))
    {
        DibujarTablon(seleccion, estilo, jugadorUno, CAPA_TODO);
    }

    DibujarCartelPrevia(seleccion.previa, estilo, participantes, cantidad, capa);
}


// Datos de una pasada fija para DibujarEnCacheMinijuegos.
struct PasadaFijaSeleccion
{
    const SeleccionMinijuegos* seleccion;
    const EstiloUI* estilo;
    const Participante* participantes;
    int cantidad;
};


static void DibujarPasadaFijaSeleccion(
    const void* datos
)
{
    const PasadaFijaSeleccion& pasada = *(const PasadaFijaSeleccion*)datos;

    DibujarCapaSeleccion(
        *pasada.seleccion,
        *pasada.estilo,
        pasada.participantes,
        pasada.cantidad,
        CAPA_FIJA
    );
}


// Deja en cacheFija la capa fija de la pantalla. Solo la vuelve a
// dibujar si cambio la clave. Devuelve false si no hay textura (se
// dibuja todo en vivo).
static bool PrepararCacheFija(
    const SeleccionMinijuegos& seleccion,
    const EstiloUI& estilo,
    const Participante participantes[],
    int cantidad
)
{
    const ClaveCacheSeleccion clave = ArmarClaveCache(seleccion, participantes, cantidad);

    if (
        seleccion.cacheValida &&
        seleccion.cacheFija.id != 0 &&
        ClavesIguales(seleccion.cacheClave, clave)
    )
    {
        return true;
    }

    PasadaFijaSeleccion pasada = { &seleccion, &estilo, participantes, cantidad };

    seleccion.cacheValida =
        DibujarEnCacheMinijuegos(seleccion.cacheFija, DibujarPasadaFijaSeleccion, &pasada);
    seleccion.cacheClave = clave;

    return seleccion.cacheValida;
}


void SeleccionMinijuegos::Descargar()
{
    if (cacheFija.id != 0)
    {
        UnloadRenderTexture(cacheFija);
    }

    cacheFija = RenderTexture2D{};
    cacheValida = false;
}


void SeleccionMinijuegos::Dibujar(
    const Participante& jugadorUno
) const
{
    Dibujar(&jugadorUno, 1);
}


void SeleccionMinijuegos::Dibujar(
    const Participante participantes[],
    int cantidad
) const
{
    const EstiloUI& estilo = ObtenerEstiloUI();

    // En reposo: la capa fija sale de la cache (una sola textura en vez
    // de velo, tablon, 24 postales y cartel) y encima va lo vivo.
    if (
        PantallaQuieta(*this, estilo) &&
        PrepararCacheFija(*this, estilo, participantes, cantidad)
    )
    {
        ComponerCacheMinijuegos(cacheFija);

        DibujarCapaSeleccion(*this, estilo, participantes, cantidad, CAPA_VIVA);
    }
    else
    {
        DibujarCapaSeleccion(*this, estilo, participantes, cantidad, CAPA_TODO);
    }

    if (vista == VISTA_PREVIA_MINIJUEGO)
    {
        DibujarAyudaPrevia(previa, estilo, participantes, cantidad);
        return;
    }

    Participante sinJugador{};
    const Participante& jugadorUno = cantidad > 0 ? participantes[0] : sinJugador;

    char confirmar[64] = "";
    char cancelar[64] = "";

    const int mascara = cantidad > 0 ? 1 << jugadorUno.control : 0;

    ArmarTeclasConfirmar(mascara, confirmar, sizeof(confirmar));
    ArmarTeclasCancelar(mascara, true, cancelar, sizeof(cancelar));

    DibujarAyudaSeleccion(
        estilo,
        TextFormat("%s: ELEGIR", ObtenerTeclasMover(jugadorUno)),
        TextFormat("%s: VER MINIJUEGO", confirmar),
        TextFormat("%s: ATRÁS", cancelar)
    );
}
