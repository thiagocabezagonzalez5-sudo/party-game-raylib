#include "UI/RuletaMinijuegos.h"

#include "UI/EstiloUIGlobal.h"
#include "UI/MiniaturasMinijuegos.h"
#include "rlgl.h"

#include <cmath>


//==================================================
// CONSTANTES
//==================================================

// Duracion aproximada total: 0.4 + (0.6 a 2.0) + 1.9 + 1.1 = 4 a 5.4 s.
static const float DURACION_INICIO_RULETA = 0.40f;
static const float VELOCIDAD_GIRO_RULETA = 16.0f;
static const float GIRO_MINIMO_RULETA = 0.60f;
static const float DURACION_FRENADO_RULETA = 1.90f;
static const float PAUSA_SELECCIONADO_RULETA = 1.10f;

// Confirmar se acepta despues del primer tramo y solo acelera el reloj.
static const float TIEMPO_MINIMO_CONFIRMAR_RULETA = 0.80f;
static const float ESCALA_TIEMPO_ACELERADA_RULETA = 2.6f;
static const float TIEMPO_MINIMO_SALTAR_PAUSA_RULETA = 0.30f;

static const float SEPARACION_TICKS_RULETA = 0.045f;


//==================================================
// UTILIDADES
//==================================================

static int ModuloPositivo(
    int valor,
    int divisor
)
{
    if (divisor <= 0)
    {
        return 0;
    }

    int resultado = valor % divisor;

    return resultado < 0
        ? resultado + divisor
        : resultado;
}


static IdMinijuego ObtenerOpcionEnPosicion(
    const RuletaMinijuegos& ruleta,
    int posicion
)
{
    if (ruleta.cantidadOpciones <= 0)
    {
        return ruleta.minijuegoElegido;
    }

    return ruleta.opciones[
        ModuloPositivo(posicion, ruleta.cantidadOpciones)
    ];
}


// Frenado con curva cubica: arranca a la velocidad de giro
// (derivada inicial 3) y llega a velocidad cero en u = 1.
static float CurvaFrenadoRuleta(float u)
{
    float resto = 1.0f - u;
    return 1.0f - resto * resto * resto;
}


static float DistanciaFrenadoRuleta()
{
    return VELOCIDAD_GIRO_RULETA * DURACION_FRENADO_RULETA / 3.0f;
}


static void ReproducirSonidoRuleta(
    RuletaMinijuegos& ruleta,
    TipoSonidoJuego tipo
)
{
    if (ruleta.audio != nullptr)
    {
        ruleta.audio->ReproducirSonido(tipo);
    }
}


static void CambiarEstadoRuleta(
    RuletaMinijuegos& ruleta,
    EstadoRuletaMinijuegos nuevoEstado,
    float tiempoInicial
)
{
    ruleta.estado = nuevoEstado;
    ruleta.tiempoEstado = tiempoInicial;
}


//==================================================
// INICIAR
//==================================================

void RuletaMinijuegos::Iniciar(
    IdMinijuego elegido,
    AudioJuego* audioJuego
)
{
    audio = audioJuego;
    Descargar();
    minijuegoElegido = elegido;

    cantidadOpciones = 0;
    indiceElegido = -1;

    int disponibles = ObtenerCantidadMinijuegosDisponiblesTablero();

    for (
        int i = 0;
        i < disponibles && cantidadOpciones < CANTIDAD_MINIJUEGOS;
        i++
    )
    {
        IdMinijuego id = ObtenerMinijuegoDisponibleTablero(i);

        if (id == elegido)
        {
            indiceElegido = cantidadOpciones;
        }

        opciones[cantidadOpciones] = id;
        cantidadOpciones++;
    }

    // El elegido siempre debe poder mostrarse. Si no estaba entre los
    // disponibles se agrega para que lo visto coincida con lo que se juega.
    if (indiceElegido < 0)
    {
        TraceLog(
            LOG_WARNING,
            "Ruleta: el minijuego elegido no figura como disponible para tablero."
        );

        if (cantidadOpciones >= CANTIDAD_MINIJUEGOS)
        {
            cantidadOpciones = CANTIDAD_MINIJUEGOS - 1;
        }

        indiceElegido = cantidadOpciones;
        opciones[cantidadOpciones] = elegido;
        cantidadOpciones++;
    }

    // Arranque visual aleatorio: no influye en el resultado.
    posicionInicial =
        (float)GetRandomValue(0, cantidadOpciones - 1);

    float finInicio =
        posicionInicial +
        VELOCIDAD_GIRO_RULETA * DURACION_INICIO_RULETA * 0.5f;

    float minimoInicioFrenado =
        finInicio +
        VELOCIDAD_GIRO_RULETA * GIRO_MINIMO_RULETA;

    float distanciaFrenado = DistanciaFrenadoRuleta();

    // Primera posicion entera, alcanzable tras el giro minimo,
    // en la que queda centrado exactamente el minijuego elegido.
    int destino =
        (int)std::ceil(minimoInicioFrenado + distanciaFrenado);

    destino +=
        ModuloPositivo(
            indiceElegido - destino,
            cantidadOpciones
        );

    posicionFinal = (float)destino;
    posicionInicioFrenado = posicionFinal - distanciaFrenado;

    posicion = posicionInicial;
    tiempoTotal = 0.0f;
    frenadoAcelerado = false;
    ultimaCasillaCentrada = (int)std::floor(posicion + 0.5f);
    tiempoDesdeUltimoTick = 0.0f;

    CambiarEstadoRuleta(*this, RULETA_INICIANDO, 0.0f);
}


//==================================================
// ACTUALIZAR
//==================================================

void RuletaMinijuegos::Actualizar(
    float deltaTime,
    bool confirmar,
    const Participante participantes[],
    int cantidad
)
{
    if (
        estado == RULETA_INACTIVA ||
        estado == RULETA_TERMINADA
    )
    {
        return;
    }

    // Un pico de lag solo salta animacion; el destino no cambia.
    if (deltaTime > 0.10f)
    {
        deltaTime = 0.10f;
    }

    tiempoTotal += deltaTime;
    tiempoDesdeUltimoTick += deltaTime;

    bool puedeAcelerar =
        (
            estado == RULETA_GIRANDO ||
            estado == RULETA_FRENANDO
        ) &&
        tiempoTotal >= TIEMPO_MINIMO_CONFIRMAR_RULETA;

    if (confirmar && puedeAcelerar)
    {
        frenadoAcelerado = true;
    }

    float deltaAnimacion =
        frenadoAcelerado
        ? deltaTime * ESCALA_TIEMPO_ACELERADA_RULETA
        : deltaTime;

    switch (estado)
    {
        case RULETA_INICIANDO:
        {
            tiempoEstado += deltaAnimacion;

            if (tiempoEstado < DURACION_INICIO_RULETA)
            {
                // Aceleracion constante desde cero hasta la velocidad de giro.
                posicion =
                    posicionInicial +
                    VELOCIDAD_GIRO_RULETA *
                    tiempoEstado * tiempoEstado /
                    (2.0f * DURACION_INICIO_RULETA);

                break;
            }

            float exceso = tiempoEstado - DURACION_INICIO_RULETA;

            posicion =
                posicionInicial +
                VELOCIDAD_GIRO_RULETA * DURACION_INICIO_RULETA * 0.5f +
                VELOCIDAD_GIRO_RULETA * exceso;

            CambiarEstadoRuleta(*this, RULETA_GIRANDO, exceso);
            break;
        }

        case RULETA_GIRANDO:
        {
            tiempoEstado += deltaAnimacion;
            posicion += VELOCIDAD_GIRO_RULETA * deltaAnimacion;

            if (posicion >= posicionInicioFrenado)
            {
                // El tramo recorrido de mas se convierte en tiempo
                // de frenado para no perder continuidad.
                float exceso =
                    (posicion - posicionInicioFrenado) /
                    VELOCIDAD_GIRO_RULETA;

                CambiarEstadoRuleta(*this, RULETA_FRENANDO, exceso);

                posicion =
                    posicionInicioFrenado +
                    (posicionFinal - posicionInicioFrenado) *
                    CurvaFrenadoRuleta(
                        tiempoEstado / DURACION_FRENADO_RULETA
                    );
            }

            break;
        }

        case RULETA_FRENANDO:
        {
            tiempoEstado += deltaAnimacion;

            float progreso = tiempoEstado / DURACION_FRENADO_RULETA;

            if (progreso >= 1.0f)
            {
                posicion = posicionFinal;
                CambiarEstadoRuleta(*this, RULETA_SELECCIONADO, 0.0f);
                ReproducirSonidoRuleta(*this, SONIDO_RESULTADO);
                break;
            }

            posicion =
                posicionInicioFrenado +
                (posicionFinal - posicionInicioFrenado) *
                CurvaFrenadoRuleta(progreso);

            break;
        }

        case RULETA_SELECCIONADO:
        {
            tiempoEstado += deltaTime;

            bool saltarPausa =
                confirmar &&
                tiempoEstado >= TIEMPO_MINIMO_SALTAR_PAUSA_RULETA;

            if (
                saltarPausa ||
                tiempoEstado >= PAUSA_SELECCIONADO_RULETA
            )
            {
                // Sin "volver": no hay otro minijuego que elegir.
                IniciarPreviaMinijuego(previa, minijuegoElegido, false);
                CambiarEstadoRuleta(*this, RULETA_PREVIA, 0.0f);
            }

            break;
        }

        case RULETA_PREVIA:
        {
            tiempoEstado += deltaTime;

            ResultadoPreviaMinijuego resultado =
                ActualizarPreviaMinijuego(previa, deltaTime, participantes, cantidad);

            // Recien con todos listos Termino() deja arrancar el minijuego.
            if (resultado == PREVIA_CONFIRMADA)
            {
                ReproducirSonidoRuleta(*this, SONIDO_UI_CONFIRMAR);
                CambiarEstadoRuleta(*this, RULETA_TERMINADA, 0.0f);

                // Juego pasa al minijuego en este mismo frame.
                Descargar();
            }

            break;
        }

        case RULETA_INACTIVA:
        case RULETA_TERMINADA:
            break;
    }

    if (
        estado == RULETA_INICIANDO ||
        estado == RULETA_GIRANDO ||
        estado == RULETA_FRENANDO
    )
    {
        int casilla = (int)std::floor(posicion + 0.5f);

        if (casilla != ultimaCasillaCentrada)
        {
            ultimaCasillaCentrada = casilla;

            // A maxima velocidad pasan 16 miniaturas por segundo;
            // se limita el tick para que no se vuelva un zumbido.
            if (tiempoDesdeUltimoTick >= SEPARACION_TICKS_RULETA)
            {
                tiempoDesdeUltimoTick = 0.0f;
                ReproducirSonidoRuleta(*this, SONIDO_RULETA_TICK);
            }
        }
    }
}


//==================================================
// DIBUJAR
//==================================================

// Tendedero de postales: el carrusel cuelga de una soga que cruza la
// pantalla; cada postal se sujeta con dos broches y se inclina segun
// la velocidad del giro. Todo en unidades de referencia (1280x720).

static const float X_CENTRO_RULETA = 640.0f;
static const float Y_SOGA_RULETA = 152.0f;          // en los bordes
static const float COMBA_SOGA_RULETA = 34.0f;       // caida en el centro
static const float ANCHO_POSTAL_RULETA = 196.0f;
static const float ALTO_POSTAL_RULETA = 232.0f;
static const float SEPARACION_POSTAL_RULETA = 232.0f;
static const float ALTO_NOMBRE_POSTAL_RULETA = 40.0f;
static const Rectangle CARTEL_NOMBRE_RULETA = { 360.0f, 446.0f, 560.0f, 56.0f };
static const Rectangle FICHA_RULETA = { 230.0f, 516.0f, 820.0f, 128.0f };

// Velo opaco: del violeta calido arriba a uno mas oscuro abajo.
static const Color SOMBRA_VELO_RULETA = { 30, 18, 38, 255 };
static const Color SOMBRA_VELO_RULETA_ABAJO = { 20, 12, 26, 255 };

// El velo cubre el tablero antes de que entren titulo y tendedero.
static const float DURACION_VELO_RULETA = 0.15f;


static float Limitar01Ruleta(float valor)
{
    if (valor < 0.0f) return 0.0f;
    if (valor > 1.0f) return 1.0f;
    return valor;
}


static float SuavizarRuleta(float t)
{
    float resto = 1.0f - Limitar01Ruleta(t);
    return 1.0f - resto * resto * resto;
}


static Vector2 PuntoRefRuleta(float x, float y)
{
    Rectangle rect = RectRef(x, y, 0.0f, 0.0f);
    return Vector2{ rect.x, rect.y };
}


// Altura de la soga (unidades de referencia) en x: parabola con la
// comba maxima en el centro.
static float AlturaSogaRuleta(float x)
{
    float u = (x - X_CENTRO_RULETA) / 700.0f;
    return Y_SOGA_RULETA + COMBA_SOGA_RULETA * (1.0f - u * u);
}


// Miniaturas por segundo que pasan bajo el marcador (para inclinar).
static float VelocidadVisualRuleta(
    const RuletaMinijuegos& ruleta
)
{
    float velocidad = 0.0f;

    switch (ruleta.estado)
    {
        case RULETA_INICIANDO:
            velocidad =
                VELOCIDAD_GIRO_RULETA *
                Limitar01Ruleta(ruleta.tiempoEstado / DURACION_INICIO_RULETA);
            break;

        case RULETA_GIRANDO:
            velocidad = VELOCIDAD_GIRO_RULETA;
            break;

        case RULETA_FRENANDO:
        {
            float resto =
                1.0f - Limitar01Ruleta(ruleta.tiempoEstado / DURACION_FRENADO_RULETA);
            velocidad = VELOCIDAD_GIRO_RULETA * resto * resto;
            break;
        }

        case RULETA_INACTIVA:
        case RULETA_SELECCIONADO:
        case RULETA_PREVIA:
        case RULETA_TERMINADA:
            break;
    }

    return ruleta.frenadoAcelerado
        ? velocidad * ESCALA_TIEMPO_ACELERADA_RULETA
        : velocidad;
}


// Como DibujarTextoMundo, pero la sombra tambien se funde con `alfa`
// (si no, al aparecer se ve la sombra sola sobre el tendedero).
static void DibujarTituloRuleta(
    const char* texto,
    float centroX,
    float y,
    float alfa
)
{
    if (alfa <= 0.0f)
    {
        return;
    }

    const int tamano = TamanoTexto(UI_TEXTO_TITULO);
    const int ancho = MeasureText(texto, tamano);
    const int sombra = tamano >= 30 ? 3 : 2;

    DrawText(texto, (int)centroX - ancho / 2 + sombra, (int)y + sombra, tamano, Color{ 0, 0, 0, (unsigned char)(120.0f * alfa) });
    DrawText(texto, (int)centroX - ancho / 2, (int)y, tamano, Fade(UI_TEXTO_MUNDO, alfa));
}


// Broche de madera que sujeta la postal a la soga.
static void DibujarBrocheRuleta(
    Vector2 centro
)
{
    Rectangle broche =
    {
        centro.x - Px(5.0f),
        centro.y - Px(12.0f),
        Px(10.0f),
        Px(24.0f)
    };

    DrawRectangleRec(broche, UI_MADERA_MEDIA);
    DrawRectangleLinesEx(broche, std::fmax(1.0f, Px(1.5f)), UI_TINTA);
    DrawLineEx(
        Vector2{ centro.x - Px(5.0f), centro.y },
        Vector2{ centro.x + Px(5.0f), centro.y },
        std::fmax(1.0f, Px(1.5f)),
        UI_TINTA
    );
}


// Postal colgada. (pivoteX, pivoteY) es el centro de su borde superior
// en pixeles; gira `angulo` grados alrededor de ese punto.
static void DibujarPostalRuleta(
    const EstiloUI& estilo,
    IdMinijuego id,
    float pivoteX,
    float pivoteY,
    float escala,
    float angulo,
    float oscurecer,
    bool destacada
)
{
    float ancho = Px(ANCHO_POSTAL_RULETA) * escala;
    float alto = Px(ALTO_POSTAL_RULETA) * escala;
    float escalaUI = EscalaUI() * escala;

    rlPushMatrix();
    rlTranslatef(pivoteX, pivoteY, 0.0f);
    rlRotatef(angulo, 0.0f, 0.0f, 1.0f);

    Rectangle local = { -ancho * 0.5f, 0.0f, ancho, alto };

    DrawRectangleRec(
        Rectangle{ local.x + Px(5.0f), local.y + Px(7.0f), local.width, local.height },
        Fade(BLACK, 0.30f)
    );

    DibujarNueveCortes(estilo.madera, 12, local, escalaUI * 0.5f, WHITE);

    Rectangle placa =
    {
        local.x + 5.0f * escalaUI,
        local.y + 5.0f * escalaUI,
        local.width - 10.0f * escalaUI,
        local.height - 10.0f * escalaUI
    };

    DibujarNueveCortes(estilo.placa, 8, placa, escalaUI, WHITE);

    float margen = 7.0f * escalaUI;
    float altoNombre = ALTO_NOMBRE_POSTAL_RULETA * escalaUI;

    Rectangle foto =
    {
        placa.x + margen,
        placa.y + margen,
        placa.width - margen * 2.0f,
        placa.height - margen - altoNombre
    };

    // Dentro de una matriz rotada el recorte no sirve: la ilustracion
    // se dibuja completa y el marco la tapa en el borde.
    DibujarIlustracionMinijuego(id, foto, false);
    DrawRectangleLinesEx(foto, std::fmax(1.0f, 2.0f * escalaUI), UI_MADERA_OSCURA);

    const char* nombre = ObtenerDatosMinijuego(id).nombre;
    int tamano = TamanoTextoAjustadoUI(nombre, 18, foto.width);
    if ((float)tamano > 18.0f * escalaUI) tamano = (int)(18.0f * escalaUI);
    if (tamano < 10) tamano = 10;

    DrawText(
        nombre,
        (int)(-MeasureText(nombre, tamano) * 0.5f),
        (int)(foto.y + foto.height + (altoNombre - tamano) * 0.5f),
        tamano,
        UI_TINTA
    );

    // Chinche del formato en la esquina, como en el tablon.
    Vector2 chinche = { placa.x + placa.width - Px(12.0f), placa.y + Px(12.0f) };
    DrawCircleV(chinche, Px(6.0f) * escala, ObtenerColorFormatoMinijuego(ObtenerFormatoMinijuego(id)));
    DrawCircleLinesV(chinche, Px(6.0f) * escala, UI_TINTA);

    if (oscurecer > 0.0f)
    {
        DrawRectangleRec(local, Fade(BLACK, oscurecer));
    }

    if (destacada)
    {
        DibujarMarcoFoco(local);
    }

    // Dos broches sobre la soga.
    DibujarBrocheRuleta(Vector2{ -ancho * 0.30f, Px(2.0f) });
    DibujarBrocheRuleta(Vector2{ ancho * 0.30f, Px(2.0f) });

    rlPopMatrix();
}


static float MedirSelloRuleta(
    const char* texto,
    bool conIcono
)
{
    const int tamano = TamanoTexto(UI_TEXTO_PEQUENO);
    const float alto = (float)tamano + Px(10.0f);

    return
        (float)MeasureText(texto, tamano) + Px(16.0f) +
        (conIcono ? alto * 1.9f : 0.0f);
}


// Sello: texto en tinta en un recuadro de color, con el icono del
// formato delante si formato >= 0.
static void DibujarSelloRuleta(
    const char* texto,
    float x,
    float y,
    Color borde,
    float alfa,
    int formato
)
{
    const int tamano = TamanoTexto(UI_TEXTO_PEQUENO);
    const float alto = (float)tamano + Px(10.0f);
    const float anchoIcono = formato >= 0 ? alto * 1.9f : 0.0f;

    Rectangle rect = { x, y, MedirSelloRuleta(texto, formato >= 0), alto };

    DrawRectangleRec(rect, Fade(borde, 0.22f * alfa));
    DrawRectangleLinesEx(rect, Px(2.0f), Fade(borde, alfa));

    if (formato >= 0)
    {
        DibujarIconoFormatoMinijuego(
            (FormatoMinijuego)formato,
            Vector2{ x + Px(5.0f) + anchoIcono * 0.5f, y + alto * 0.5f },
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
}


// Descripcion en hasta dos lineas centradas; reduce la fuente si no entra.
static void DibujarDescripcionRuleta(
    const char* texto,
    float centroX,
    float y,
    float ancho,
    Color color
)
{
    const int largo = (int)TextLength(texto);

    // Corte en el espacio que deja las dos mitades mas parejas.
    int corte = -1;

    for (int i = 0; i < largo; i++)
    {
        if (
            texto[i] == ' ' &&
            (corte < 0 || std::abs(i - largo / 2) < std::abs(corte - largo / 2))
        )
        {
            corte = i;
        }
    }

    if (
        MeasureText(texto, TamanoTexto(UI_TEXTO)) <= ancho ||
        corte < 0 ||
        corte >= 255 ||
        largo - corte >= 255
    )
    {
        DibujarTextoAjustadoUI(texto, centroX, y, UI_TEXTO, ancho, color, true);
        return;
    }

    char lineas[2][256] = {};
    TextCopy(lineas[0], TextSubtext(texto, 0, corte));
    TextCopy(lineas[1], TextSubtext(texto, corte + 1, largo - corte - 1));

    int tamanoA = TamanoTextoAjustadoUI(lineas[0], UI_TEXTO, ancho);
    int tamanoB = TamanoTextoAjustadoUI(lineas[1], UI_TEXTO, ancho);
    int tamano = tamanoA < tamanoB ? tamanoA : tamanoB;

    for (int i = 0; i < 2; i++)
    {
        DrawText(
            lineas[i],
            (int)(centroX - MeasureText(lineas[i], tamano) * 0.5f),
            (int)(y + i * (tamano + Px(6.0f))),
            tamano,
            color
        );
    }
}


void RuletaMinijuegos::DibujarTendedero() const
{

    const EstiloUI& estilo = ObtenerEstiloUI();
    const bool reducir = estilo.reducirMovimiento;

    // Primero el velo tapa por completo el tablero y su HUD (fichas,
    // ronda, avisos); recien entonces entran titulo y tendedero, asi
    // nunca se superponen. Con reducirMovimiento el velo es inmediato.
    const float velo = reducir
        ? 1.0f
        : Limitar01Ruleta(tiempoTotal / DURACION_VELO_RULETA);

    const float aparicion = reducir
        ? Limitar01Ruleta(tiempoTotal / DURACION_INICIO_RULETA)
        : Limitar01Ruleta((tiempoTotal - DURACION_VELO_RULETA) / DURACION_INICIO_RULETA);

    DrawRectangleGradientV(
        0,
        0,
        GetScreenWidth(),
        GetScreenHeight(),
        Fade(SOMBRA_VELO_RULETA, velo),
        Fade(SOMBRA_VELO_RULETA_ABAJO, velo)
    );

    // Hasta que el velo no tapa todo, no se dibuja nada encima.
    if (aparicion <= 0.0f)
    {
        return;
    }

    // Con la previa, su propio titulo reemplaza al de la ruleta (sin
    // fundido cruzado: los dos textos ocupan el mismo lugar).
    const bool enPrevia =
        estado == RULETA_PREVIA ||
        estado == RULETA_TERMINADA;

    if (!enPrevia)
    {
        // El titulo aparece recien cuando el tendedero (y su marcador)
        // ya bajo por debajo de el: al entrar no se cruzan.
        const float aparicionTitulo = reducir
            ? aparicion
            : Limitar01Ruleta((aparicion - 0.6f) / 0.4f);

        Vector2 titulo = PuntoRefRuleta(X_CENTRO_RULETA, 22.0f);

        DibujarTituloRuleta(
            "MINIJUEGO DE LA RONDA",
            titulo.x,
            titulo.y,
            aparicionTitulo
        );
    }

    if (!estilo.cargado)
    {
        return;
    }

    // El tendedero baja desde arriba al aparecer.
    const float caida = reducir ? 0.0f : -(1.0f - SuavizarRuleta(aparicion)) * 260.0f;

    const bool seleccionado =
        estado == RULETA_SELECCIONADO ||
        enPrevia;

    //------------------------------
    // SOGA
    //------------------------------

    const int TRAMOS_SOGA = 24;

    for (int i = 0; i < TRAMOS_SOGA; i++)
    {
        float x0 = -40.0f + 1360.0f * i / (float)TRAMOS_SOGA;
        float x1 = -40.0f + 1360.0f * (i + 1) / (float)TRAMOS_SOGA;

        DibujarCuerdaUI(
            estilo,
            PuntoRefRuleta(x0, AlturaSogaRuleta(x0) + caida),
            PuntoRefRuleta(x1, AlturaSogaRuleta(x1) + caida)
        );
    }

    //------------------------------
    // POSTALES
    //------------------------------

    // Las postales se inclinan hacia atras mientras corren y la elegida
    // se balancea al detenerse (nada de esto con reducirMovimiento).
    float inclinacion = 0.0f;

    if (!reducir)
    {
        inclinacion = -std::fmin(VelocidadVisualRuleta(*this) * 0.55f, 9.0f);
    }

    float balanceo = 0.0f;

    if (!reducir && estado == RULETA_SELECCIONADO)
    {
        balanceo =
            7.0f *
            std::exp(-3.2f * tiempoEstado) *
            std::sin(tiempoEstado * 10.0f);
    }

    float base = std::floor(posicion);
    float fraccion = posicion - base;

    // Se dibuja de los extremos hacia el centro para que la
    // postal mas cercana al marcador quede por encima.
    const int ALCANCE = 4;
    const int CANTIDAD_RANURAS = ALCANCE * 2 + 1;

    int ranuras[CANTIDAD_RANURAS]{};

    for (int i = 0; i < CANTIDAD_RANURAS; i++)
    {
        ranuras[i] = i - ALCANCE;
    }

    for (int i = 1; i < CANTIDAD_RANURAS; i++)
    {
        int actual = ranuras[i];
        float distanciaActual = std::fabs((float)actual - fraccion);
        int j = i - 1;

        while (
            j >= 0 &&
            std::fabs((float)ranuras[j] - fraccion) < distanciaActual
        )
        {
            ranuras[j + 1] = ranuras[j];
            j--;
        }

        ranuras[j + 1] = actual;
    }

    for (int i = 0; i < CANTIDAD_RANURAS; i++)
    {
        int k = ranuras[i];

        float desplazamiento = (float)k - fraccion;
        float distancia = std::fabs(desplazamiento);

        if (distancia > 3.4f)
        {
            continue;
        }

        bool destacada = seleccionado && distancia < 0.01f;

        float escala = 1.0f - 0.12f * std::fmin(distancia, 2.0f);
        float x = X_CENTRO_RULETA + desplazamiento * SEPARACION_POSTAL_RULETA;

        float oscurecer = std::fmin(0.16f * distancia, 0.50f);

        if (seleccionado && !destacada)
        {
            oscurecer += 0.20f;
        }

        Vector2 pivote = PuntoRefRuleta(x, AlturaSogaRuleta(x) + caida);

        DibujarPostalRuleta(
            estilo,
            ObtenerOpcionEnPosicion(*this, (int)base + k),
            pivote.x,
            pivote.y,
            escala,
            destacada ? balanceo : inclinacion,
            oscurecer,
            destacada
        );
    }

    //------------------------------
    // MARCADOR
    //------------------------------

    Vector2 punta = PuntoRefRuleta(X_CENTRO_RULETA, 150.0f + caida);
    float medio = Px(14.0f);

    Vector2 izquierda = { punta.x - medio, punta.y - medio * 1.4f };
    Vector2 derecha = { punta.x + medio, punta.y - medio * 1.4f };

    DrawTriangle(derecha, izquierda, punta, seleccionado ? UI_FOCO : UI_TEXTO_MUNDO);
    DrawTriangleLines(derecha, izquierda, punta, UI_TINTA);

    //------------------------------
    // NOMBRE DE LA POSTAL CENTRADA
    //------------------------------

    IdMinijuego centrado =
        ObtenerOpcionEnPosicion(
            *this,
            (int)std::floor(posicion + 0.5f)
        );

    const DatosMinijuegoCatalogo& datos = ObtenerDatosMinijuego(centrado);

    Rectangle cartel = RectRef(
        CARTEL_NOMBRE_RULETA.x,
        CARTEL_NOMBRE_RULETA.y,
        CARTEL_NOMBRE_RULETA.width,
        CARTEL_NOMBRE_RULETA.height
    );

    DrawRectangleRec(
        Rectangle{ cartel.x + Px(4.0f), cartel.y + Px(5.0f), cartel.width, cartel.height },
        Fade(BLACK, 0.28f * aparicion)
    );

    DibujarNueveCortes(estilo.cartel, 10, cartel, EscalaUI(), Fade(WHITE, aparicion));

    int tamanoNombre = TamanoTextoAjustadoUI(datos.nombre, UI_TEXTO_GRANDE, cartel.width - Px(28.0f));

    DrawText(
        datos.nombre,
        (int)(cartel.x + (cartel.width - MeasureText(datos.nombre, tamanoNombre)) * 0.5f),
        (int)(cartel.y + (cartel.height - tamanoNombre) * 0.5f),
        tamanoNombre,
        Fade(UI_TINTA, aparicion)
    );

    if (seleccionado)
    {
        DibujarMarcoFoco(cartel);
    }

    //------------------------------
    // FICHA DEL ELEGIDO
    //------------------------------

    if (seleccionado)
    {
        float t = estado == RULETA_SELECCIONADO ? tiempoEstado : 1.0f;
        float alfa = reducir ? 1.0f : Limitar01Ruleta(t / 0.20f);
        float bajada = reducir ? 0.0f : -(1.0f - SuavizarRuleta(t / 0.30f)) * 24.0f;

        Rectangle ficha = RectRef(
            FICHA_RULETA.x,
            FICHA_RULETA.y + bajada,
            FICHA_RULETA.width,
            FICHA_RULETA.height
        );

        DrawRectangleRec(
            Rectangle{ ficha.x + Px(4.0f), ficha.y + Px(5.0f), ficha.width, ficha.height },
            Fade(BLACK, 0.22f * alfa)
        );

        DibujarNueveCortes(estilo.madera, 12, ficha, EscalaUI() * 0.6f, Fade(WHITE, alfa));

        Rectangle placa =
        {
            ficha.x + Px(7.0f),
            ficha.y + Px(7.0f),
            ficha.width - Px(14.0f),
            ficha.height - Px(14.0f)
        };

        DibujarNueveCortes(estilo.placa, 8, placa, EscalaUI(), Fade(WHITE, alfa));

        FormatoMinijuego formato = ObtenerFormatoMinijuego(centrado);
        const char* textoFormato = ObtenerNombreFormatoMinijuego(formato);
        const char* textoJugadores = ObtenerTextoJugadoresMinijuego(centrado);

        float anchoSellos =
            MedirSelloRuleta(textoFormato, true) +
            Px(10.0f) +
            MedirSelloRuleta(textoJugadores, false);

        float centroX = placa.x + placa.width * 0.5f;
        float xSellos = centroX - anchoSellos * 0.5f;
        float ySellos = placa.y + Px(10.0f);

        DibujarSelloRuleta(
            textoFormato,
            xSellos,
            ySellos,
            ObtenerColorFormatoMinijuego(formato),
            alfa,
            formato
        );

        DibujarSelloRuleta(
            textoJugadores,
            xSellos + MedirSelloRuleta(textoFormato, true) + Px(10.0f),
            ySellos,
            UI_MADERA_MEDIA,
            alfa,
            -1
        );

        DibujarDescripcionRuleta(
            datos.descripcion,
            centroX,
            placa.y + Px(48.0f),
            placa.width - Px(32.0f),
            Fade(UI_TINTA, alfa)
        );
    }

    //------------------------------
    // AYUDA
    //------------------------------

    // En la previa la ayuda es la del cartel.
    if (enPrevia)
    {
        return;
    }

    // Cualquier humano frena con su boton de confirmar (ver
    // ConfirmarConParticipanteHumano): ESPACIO, ENTER o A segun su control.
    if (seleccionado)
    {
        DibujarAyudaUI(estilo, "PREPARANDO EL MINIJUEGO...");
    }
    else if (
        !frenadoAcelerado &&
        tiempoTotal >= TIEMPO_MINIMO_CONFIRMAR_RULETA
    )
    {
        DibujarAyudaUI(estilo, "ESPACIO / ENTER / A: FRENAR MÁS RÁPIDO");
    }
}


// Datos de la pasada fija de la previa para DibujarEnCacheMinijuegos.
struct PasadaFijaRuleta
{
    const RuletaMinijuegos* ruleta;
    const Participante* participantes;
    int cantidad;
};


static void DibujarPasadaFijaRuleta(
    const void* datos
)
{
    const PasadaFijaRuleta& pasada = *(const PasadaFijaRuleta*)datos;

    pasada.ruleta->DibujarTendedero();

    DibujarPreviaMinijuego(
        pasada.ruleta->previa,
        pasada.participantes,
        pasada.cantidad,
        CAPA_FIJA
    );
}


void RuletaMinijuegos::Dibujar(
    const Participante participantes[],
    int cantidad
) const
{
    if (estado == RULETA_INACTIVA)
    {
        return;
    }

    const bool enPrevia =
        estado == RULETA_PREVIA ||
        estado == RULETA_TERMINADA;

    if (!enPrevia)
    {
        DibujarTendedero();
        return;
    }

    // Previa quieta: velo, tendedero detenido y lo fijo del cartel salen
    // de la cache (una textura en vez de varias capas a pantalla
    // completa) y encima va lo vivo (banderas, sellos, ayuda).
    if (PreviaMinijuegoQuieta(previa))
    {
        const int clave[CANTIDAD_CLAVE_CACHE_RULETA] =
        {
            GetScreenWidth(),
            GetScreenHeight(),
            (int)previa.id,
            (int)std::lround(previa.panel.colgado.caida.valor * 100.0f),
            cantidad,
            (int)FirmaParticipantesMinijuegos(participantes, cantidad)
        };

        bool vigente = cacheValida && cachePrevia.id != 0;

        for (int i = 0; i < CANTIDAD_CLAVE_CACHE_RULETA && vigente; i++)
        {
            vigente = claveCache[i] == clave[i];
        }

        if (!vigente)
        {
            PasadaFijaRuleta pasada = { this, participantes, cantidad };

            cacheValida =
                DibujarEnCacheMinijuegos(cachePrevia, DibujarPasadaFijaRuleta, &pasada);

            for (int i = 0; i < CANTIDAD_CLAVE_CACHE_RULETA; i++)
            {
                claveCache[i] = clave[i];
            }
        }

        if (cacheValida)
        {
            ComponerCacheMinijuegos(cachePrevia);
            DibujarPreviaMinijuego(previa, participantes, cantidad, CAPA_VIVA);
            return;
        }
    }

    DibujarTendedero();
    DibujarPreviaMinijuego(previa, participantes, cantidad, CAPA_TODO);
}


void RuletaMinijuegos::Descargar()
{
    if (cachePrevia.id != 0)
    {
        UnloadRenderTexture(cachePrevia);
    }

    cachePrevia = RenderTexture2D{};
    cacheValida = false;
}


bool RuletaMinijuegos::Termino() const
{
    return estado == RULETA_TERMINADA;
}


IdMinijuego RuletaMinijuegos::ObtenerMinijuegoElegido() const
{
    return minijuegoElegido;
}
