#include "UI/DemoEstiloUI.h"

#include <cmath>


//==================================================
// DATOS DE LA DEMO
//==================================================

enum TipoFilaDemo
{
    FILA_SELECTOR = 0,
    FILA_PALANCA,
    FILA_INTERRUPTOR,
    FILA_TARJETA            // sin foco; ocupa el lugar de la ultima fila
};

// La ultima pestana (EXTRAS) no tiene filas: muestra la segunda tanda de
// componentes y tiene un unico elemento con foco (el parrafo).
static const int PESTANA_EXTRAS = 4;

// Por pestana con filas: tipo de fila y su indice dentro del arreglo del
// tipo.
static const int FILAS_DEMO[PESTANA_EXTRAS][DEMO_FILAS][2] =
{
    { { FILA_SELECTOR, 0 }, { FILA_PALANCA, 0 }, { FILA_INTERRUPTOR, 0 } },
    { { FILA_PALANCA, 1 }, { FILA_PALANCA, 2 }, { FILA_INTERRUPTOR, 1 } },
    { { FILA_SELECTOR, 1 }, { FILA_INTERRUPTOR, 2 }, { FILA_INTERRUPTOR, 3 } },
    { { FILA_PALANCA, 3 }, { FILA_SELECTOR, 2 }, { FILA_TARJETA, 0 } }
};

static const char* NOMBRES_PESTANA[DEMO_PESTANAS] =
{
    "VIDEO",
    "AUDIO",
    "JUEGO",
    "CONTROLES",
    "EXTRAS"
};

static const char* OPCIONES_SELECTOR[3][3] =
{
    { "VENTANA", "COMPLETA", "SIN BORDES" },
    { "FACIL", "NORMAL", "DIFICIL" },
    { "SOLO MANDO", "SOLO MANDO", "SOLO MANDO" }
};

// Palanca con texto propio: limite de FPS en 5 pasos.
static const int OPCIONES_FPS[5] = { 30, 60, 90, 120, 144 };

static const float REPOSO_PESTANA[DEMO_PESTANAS] = { 1.2f, -1.0f, 1.4f, -1.2f, 1.0f };

// Interruptor que gobierna EstiloUI::reducirMovimiento.
static const int INTERRUPTOR_REDUCIR = 2;

// Layout en unidades de referencia (1280x720).
static const Rectangle REF_PANEL = { 240.0f, 100.0f, 800.0f, 520.0f };
static const Rectangle REF_TITULO = { 450.0f, 22.0f, 380.0f, 60.0f };
static const float Y_FILAS = 232.0f;
static const float PASO_FILAS = 66.0f;
static const float Y_BOTONES = 510.0f;

// Con 5 pestanas, minimo 126 de ancho y 14 de hueco para que entren en
// el panel.
static const float HUECO_PESTANAS = 14.0f;
static const float ANCHO_MINIMO_PESTANA = 126.0f;

// EXTRAS: marco con recorte, tarjeta del parrafo, sellos, etiquetas y, a
// los costados del panel (sobre el mundo), textos con contorno y
// banderines.
static const Rectangle REF_MARCO = { 282.0f, 222.0f, 250.0f, 172.0f };
static const Rectangle REF_TARJETA_PARRAFO = { 552.0f, 216.0f, 262.0f, 184.0f };
static const Rectangle REF_BANDERIN_IZQ = { 70.0f, 268.0f, 150.0f, 58.0f };
static const Rectangle REF_BANDERIN_DER = { 1080.0f, 268.0f, 150.0f, 58.0f };
static const float ALTO_MASTIL = 330.0f;

static const char* TEXTO_PARRAFO =
    "Elegí un minijuego y confirmá con ACEPTAR. Si el texto no entra, "
    "la letra se achica sola y nunca corta palabras.";

static const char* NOMBRES_ALINEACION[3] = { "IZQUIERDA", "CENTRO", "DERECHA" };

static const Rectangle REF_MODAL_SALIR = { 340.0f, 230.0f, 600.0f, 250.0f };
static const Rectangle REF_MODAL_APLICAR = { 340.0f, 220.0f, 600.0f, 270.0f };
static const float DURACION_CONFIRMACION = 10.0f;


static Rectangle RectFila(int fila)
{
    return Rectangle{ 290.0f, Y_FILAS + fila * PASO_FILAS, 700.0f, UI_ALTO_FILA + 6.0f };
}


// Filas que reciben foco en una pestana (la tarjeta no).
static int FilasConFoco(int pestana)
{
    if (pestana == PESTANA_EXTRAS)
    {
        return 1;
    }

    int cantidad = 0;

    for (int fila = 0; fila < DEMO_FILAS; fila++)
    {
        if (FILAS_DEMO[pestana][fila][0] != FILA_TARJETA)
        {
            cantidad++;
        }
    }

    return cantidad;
}


static int IndiceFps(const PalancaUI& palanca)
{
    return (int)(palanca.valor * 4.0f + 0.5f);
}


static void IniciarFilasPestana(DemoEstiloUI& demo, float retrasoBase)
{
    if (demo.pestanaActiva == PESTANA_EXTRAS)
    {
        demo.tarjetaParrafo.Iniciar(REF_TARJETA_PARRAFO, "Párrafo con ajuste", retrasoBase);
        return;
    }

    for (int fila = 0; fila < DEMO_FILAS; fila++)
    {
        int tipo = FILAS_DEMO[demo.pestanaActiva][fila][0];
        int indice = FILAS_DEMO[demo.pestanaActiva][fila][1];
        float retraso = retrasoBase + fila * UI_T_ESCALONADO;
        Rectangle rect = RectFila(fila);

        if (tipo == FILA_SELECTOR)
        {
            const char* etiquetas[3] = { "Modo de pantalla", "Dificultad", "Distribucion" };
            demo.selectores[indice].Iniciar(rect, etiquetas[indice], retraso);

            // La distribucion no se puede cambiar sin teclado: deshabilitado.
            demo.selectores[indice].habilitado = indice != 2;
        }
        else if (tipo == FILA_PALANCA)
        {
            // Se conserva el valor ajustado al volver a la pestana.
            const char* etiquetas[4] = { "Limite de FPS", "Musica", "Efectos", "Zona muerta" };
            float valor = demo.palancas[indice].valor;
            demo.palancas[indice].Iniciar(rect, etiquetas[indice], valor, retraso);

            // Iniciar repone paso = 0,05: el limite de FPS va en 5 pasos y
            // la zona muerta de a 1 % (rango 5..35).
            if (indice == 0)
            {
                demo.palancas[indice].paso = 0.25f;
            }
            else if (indice == 3)
            {
                demo.palancas[indice].paso = 1.0f / 30.0f;
            }
        }
        else if (tipo == FILA_INTERRUPTOR)
        {
            const char* etiquetas[4] = { "VSync", "Silenciar todo", "Reducir movimiento", "Mostrar ayudas" };
            bool valor = demo.interruptores[indice].activo;
            demo.interruptores[indice].Iniciar(rect, etiquetas[indice], valor, retraso);
        }
        else
        {
            demo.tarjeta.Iniciar(
                Rectangle{ rect.x, rect.y, rect.width, 114.0f },
                "Mandos conectados",
                retraso
            );
        }
    }
}


//==================================================
// INICIALIZAR / DESCARGAR
//==================================================

void DemoEstiloUI::Inicializar(bool reducirMovimiento)
{
    CargarEstiloUI(estilo);

    salir = false;
    tiempo = 0.0f;
    pestanaActiva = 0;
    botonConFoco = 0;
    enfoque.indice = 0;
    enfoque.cantidad = FilasConFoco(pestanaActiva) + 1;
    opcionSelector[0] = 1;
    opcionSelector[1] = 1;
    opcionSelector[2] = 0;
    alineacionParrafo = UI_ALINEAR_IZQUIERDA;
    velo.Iniciar(false);

    estilo.reducirMovimiento = reducirMovimiento;

    // Valores iniciales (las filas los conservan al cambiar de pestana).
    const float valoresPalanca[4] = { 0.25f, 0.45f, 0.8f, 10.0f / 30.0f };
    const bool valoresInterruptor[4] = { true, false, reducirMovimiento, true };

    for (int i = 0; i < 4; i++)
    {
        palancas[i].valor = valoresPalanca[i];
        interruptores[i].activo = valoresInterruptor[i];
    }

    // Limite de FPS: texto propio ("60 FPS") y zona para el valor mas
    // ancha que la de 0..100 %.
    palancas[0].anchoValor = 110.0f;

    // Zona muerta: rango real 5..35 % con formato propio.
    palancas[3].minimo = 5.0f;
    palancas[3].maximo = 35.0f;
    palancas[3].formatoValor = "%.0f %%";

    panel.Iniciar(REF_PANEL, 0.0f);
    colgadoTitulo.Iniciar(-1.0f, 0.45f, 120.0f, 1.0f);

    // Pestanas con el ancho que pide su texto, centradas en el panel.
    float anchos[DEMO_PESTANAS];
    float total = HUECO_PESTANAS * (DEMO_PESTANAS - 1);

    for (int i = 0; i < DEMO_PESTANAS; i++)
    {
        float necesario = AnchoNecesarioBotonUI(NOMBRES_PESTANA[i]) + 40.0f;
        anchos[i] = necesario > ANCHO_MINIMO_PESTANA ? necesario : ANCHO_MINIMO_PESTANA;
        total += anchos[i];
    }

    float x = REF_PANEL.x + (REF_PANEL.width - total) * 0.5f;

    for (int i = 0; i < DEMO_PESTANAS; i++)
    {
        pestanas[i].Iniciar(
            Rectangle{ x, 144.0f, anchos[i], 46.0f },
            NOMBRES_PESTANA[i],
            REPOSO_PESTANA[i],
            0.65f + i * UI_T_ESCALONADO
        );
        pestanas[i].activo = i == pestanaActiva;
        x += anchos[i] + HUECO_PESTANAS;
    }

    botonVolver.Iniciar(Rectangle{ 380.0f, Y_BOTONES, 220.0f, UI_ALTO_BOTON }, "VOLVER", -1.2f, 1.0f);
    botonAplicar.Iniciar(Rectangle{ 680.0f, Y_BOTONES, 220.0f, UI_ALTO_BOTON }, "APLICAR", 1.0f, 1.07f);

    modalSalir.Cerrar();
    modalAplicar.Cerrar();

    // Las filas se crean cuando las pestanas se asientan (ver Actualizar).
    filasListas = false;
}


void DemoEstiloUI::Descargar()
{
    DescargarEstiloUI(estilo);
}


//==================================================
// ACTUALIZAR
//==================================================

static void CambiarPestana(DemoEstiloUI& demo, int delta)
{
    int nueva = ((demo.pestanaActiva + delta) % DEMO_PESTANAS + DEMO_PESTANAS) % DEMO_PESTANAS;

    if (nueva == demo.pestanaActiva)
    {
        return;
    }

    demo.pestanaActiva = nueva;

    for (int i = 0; i < DEMO_PESTANAS; i++)
    {
        demo.pestanas[i].activo = i == nueva;
    }

    demo.enfoque.indice = 0;
    demo.enfoque.cantidad = FilasConFoco(nueva) + 1;
    demo.filasListas = false;

    // El velo es de EXTRAS: al cambiar de pestana se funde.
    demo.velo.visible = false;
}


// true mientras alguna pestana aun cae o se balancea (mas de 2,5 grados o
// con velocidad angular alta).
static bool PestanasAgitadas(const DemoEstiloUI& demo)
{
    for (int i = 0; i < DEMO_PESTANAS; i++)
    {
        const ColgadoUI& c = demo.pestanas[i].colgado;

        if (c.retraso > 0.0f || !c.aterrizo || std::fabs(c.angulo - c.reposo) > 2.5f || std::fabs(c.velocidad) > 15.0f)
        {
            return true;
        }
    }

    return false;
}


static void AjustarFilaEnfocada(DemoEstiloUI& demo, int direccion)
{
    if (demo.pestanaActiva == PESTANA_EXTRAS)
    {
        demo.alineacionParrafo = (demo.alineacionParrafo + direccion + 3) % 3;
        return;
    }

    int fila = demo.enfoque.indice;
    int tipo = FILAS_DEMO[demo.pestanaActiva][fila][0];
    int indice = FILAS_DEMO[demo.pestanaActiva][fila][1];

    if (tipo == FILA_SELECTOR)
    {
        // Deshabilitado: ni cambia el valor ni golpean las flechas.
        if (!demo.selectores[indice].habilitado)
        {
            return;
        }

        demo.opcionSelector[indice] = (demo.opcionSelector[indice] + direccion + 3) % 3;
        GolpearSelectorUI(demo.selectores[indice], direccion);
    }
    else if (tipo == FILA_PALANCA)
    {
        AjustarPalancaUI(demo.palancas[indice], direccion);
    }
    else if (tipo == FILA_INTERRUPTOR)
    {
        demo.interruptores[indice].activo = direccion > 0;
    }
}


static void AbrirModalSalir(DemoEstiloUI& demo)
{
    // Foco inicial en la opcion segura (seguir editando).
    demo.modalSalir.Abrir(REF_MODAL_SALIR, "¿Salir sin aplicar?", "SALIR", "SEGUIR EDITANDO", 1);
    demo.modalSalir.botonCancelar = 1;
}


static void AbrirModalAplicar(DemoEstiloUI& demo)
{
    demo.modalAplicar.Abrir(REF_MODAL_APLICAR, "¿Mantener estos ajustes?", "MANTENER", "REVERTIR", 0);
    demo.modalAplicar.botonCancelar = 1;
    demo.tiempoConfirmacion = DURACION_CONFIRMACION;
}


static void ActualizarFilas(DemoEstiloUI& demo, float deltaTime)
{
    for (int i = 0; i < 3; i++)
    {
        ActualizarSelectorUI(demo.selectores[i], demo.estilo, deltaTime);
    }

    for (int i = 0; i < 4; i++)
    {
        ActualizarPalancaUI(demo.palancas[i], demo.estilo, deltaTime);
        ActualizarInterruptorUI(demo.interruptores[i], demo.estilo, deltaTime);
    }

    ActualizarTarjetaUI(demo.tarjeta, demo.estilo, deltaTime);
    ActualizarTarjetaUI(demo.tarjetaParrafo, demo.estilo, deltaTime);
    ActualizarVeloUI(demo.velo, demo.estilo, deltaTime);
}


void DemoEstiloUI::Actualizar(float deltaTime, const EntradaUI& entradaReal)
{
    tiempo += deltaTime;
    ActualizarEstiloUI(estilo, deltaTime);

    // El interruptor de movimiento reducido gobierna el estilo.
    estilo.reducirMovimiento = interruptores[INTERRUPTOR_REDUCIR].activo;

    // Con un aviso abierto, la entrada es solo suya.
    EntradaUI entrada = entradaReal;
    bool hayModal = modalSalir.abierto || modalAplicar.abierto;

    if (hayModal)
    {
        entrada = EntradaUI{};
    }

    if (modalSalir.abierto)
    {
        if (ActualizarModalUI(modalSalir, estilo, deltaTime, entradaReal) == 0)
        {
            salir = true;
        }
    }

    if (modalAplicar.abierto)
    {
        tiempoConfirmacion -= deltaTime;

        int respuesta = ActualizarModalUI(modalAplicar, estilo, deltaTime, entradaReal);

        // Sin respuesta a tiempo se revierte solo.
        if (respuesta < 0 && tiempoConfirmacion <= 0.0f)
        {
            modalAplicar.Cerrar();
        }
    }

    if (entrada.volver)
    {
        AbrirModalSalir(*this);
    }

    if (entrada.pestanaAnterior)
    {
        CambiarPestana(*this, -1);
    }

    if (entrada.pestanaSiguiente)
    {
        CambiarPestana(*this, 1);
    }

    if (entrada.arriba)
    {
        enfoque.Mover(-1);
    }

    if (entrada.abajo)
    {
        enfoque.Mover(1);
    }

    bool enBotones = enfoque.indice == FilasConFoco(pestanaActiva);

    if (entrada.izquierda || entrada.derecha)
    {
        int direccion = entrada.derecha ? 1 : -1;

        if (enBotones)
        {
            botonConFoco = direccion > 0 ? 1 : 0;
        }
        else if (filasListas)
        {
            AjustarFilaEnfocada(*this, direccion);
        }
    }

    if (entrada.aceptar)
    {
        if (enBotones)
        {
            BotonUI& boton = botonConFoco == 0 ? botonVolver : botonAplicar;
            boton.colgado.Pulsar();

            if (botonConFoco == 0)
            {
                AbrirModalSalir(*this);
            }
            else
            {
                AbrirModalAplicar(*this);
            }
        }
        else if (filasListas && pestanaActiva == PESTANA_EXTRAS)
        {
            velo.visible = !velo.visible;
        }
        else if (filasListas)
        {
            int tipo = FILAS_DEMO[pestanaActiva][enfoque.indice][0];
            int indice = FILAS_DEMO[pestanaActiva][enfoque.indice][1];

            if (tipo == FILA_INTERRUPTOR)
            {
                interruptores[indice].activo = !interruptores[indice].activo;
            }
            else if (tipo == FILA_SELECTOR)
            {
                AjustarFilaEnfocada(*this, 1);
            }
        }
    }

    // Animaciones.
    ActualizarPanelUI(panel, estilo, deltaTime);
    colgadoTitulo.Actualizar(estilo, deltaTime, false);

    for (int i = 0; i < DEMO_PESTANAS; i++)
    {
        ActualizarBotonUI(pestanas[i], estilo, deltaTime, i == pestanaActiva);
    }

    // Las filas aparecen (escalonadas) cuando las pestanas se asientan.
    if (!filasListas && !PestanasAgitadas(*this))
    {
        IniciarFilasPestana(*this, 0.0f);
        filasListas = true;
    }

    bool focoBotones = enBotones && !hayModal;

    ActualizarBotonUI(botonVolver, estilo, deltaTime, focoBotones && botonConFoco == 0);
    ActualizarBotonUI(botonAplicar, estilo, deltaTime, focoBotones && botonConFoco == 1);

    ActualizarFilas(*this, deltaTime);
}


//==================================================
// DIBUJAR
//==================================================

static void DibujarFondoDemo()
{
    int ancho = GetScreenWidth();
    int alto = GetScreenHeight();
    int horizonte = alto * 46 / 100;

    DrawRectangleGradientV(0, 0, ancho, alto, Color{ 52, 36, 112, 255 }, Color{ 255, 150, 104, 255 });
    DrawCircle(ancho * 70 / 100, horizonte, (float)alto / 11.0f, Color{ 255, 224, 172, 255 });
    DrawRectangleGradientV(0, horizonte, ancho, alto - horizonte, Color{ 70, 150, 205, 255 }, Color{ 36, 84, 168, 255 });
    DrawRectangle(0, horizonte, ancho, 2, Color{ 255, 200, 150, 255 });
}


// Punto en unidades de referencia -> pixeles.
static Vector2 PuntoRef(float x, float y)
{
    Rectangle r = RectRef(x, y, 0.0f, 0.0f);

    return Vector2{ r.x, r.y };
}


// Escena de playa mas grande que el marco: se desplaza de lado a lado
// (quieta con movimiento reducido) para que se vea el recorte.
static void DibujarEscenaMarco(const DemoEstiloUI& demo, Rectangle area)
{
    float desplazamiento = demo.estilo.reducirMovimiento ? 0.0f : std::sin(demo.tiempo * 0.7f) * Px(36.0f);
    float margen = Px(80.0f);
    float horizonte = area.y + area.height * 0.55f;

    DrawRectangleGradientV(
        (int)(area.x - margen),
        (int)area.y,
        (int)(area.width + margen * 2.0f),
        (int)(horizonte - area.y) + 1,
        Color{ 52, 36, 112, 255 },
        Color{ 255, 150, 104, 255 }
    );

    // Sol grande: se sale por arriba y por la derecha.
    DrawCircleV(Vector2{ area.x + area.width * 0.85f + desplazamiento, area.y + Px(14.0f) }, Px(52.0f), Color{ 255, 224, 172, 255 });

    DrawRectangleGradientV(
        (int)(area.x - margen),
        (int)horizonte,
        (int)(area.width + margen * 2.0f),
        (int)(area.y + area.height - horizonte) + 1,
        Color{ 70, 150, 205, 255 },
        Color{ 36, 84, 168, 255 }
    );

    // Isla de arena que se corta por abajo.
    Vector2 isla = { area.x + area.width * 0.38f + desplazamiento, area.y + area.height + Px(52.0f) };
    DrawCircleV(isla, Px(100.0f), Color{ 236, 206, 140, 255 });

    // Dos jugadores con su etiqueta (la de J2 sale del marco y se recorta).
    const Color colores[2] = { UI_PELIGRO, Color{ 70, 130, 210, 255 } };
    const char* nombres[2] = { "J1", "J2" };
    const float xs[2] = { 0.30f, 0.95f };

    for (int i = 0; i < 2; i++)
    {
        Vector2 cuerpo = { area.x + area.width * xs[i] + desplazamiento, area.y + area.height * 0.78f };

        DrawCircleV(cuerpo, Px(13.0f), UI_TINTA);
        DrawCircleV(cuerpo, Px(11.0f), colores[i]);

        OpcionesEtiquetaUI opciones;
        opciones.borde = colores[i];
        opciones.pico = true;

        DibujarEtiquetaUI(nombres[i], Vector2{ cuerpo.x, cuerpo.y - Px(36.0f) }, i == 0 ? -5.0f : 6.0f, opciones);
    }
}


static void DibujarExtras(const DemoEstiloUI& demo, bool foco)
{
    const EstiloUI& estilo = demo.estilo;

    // Sobre el mundo, a los costados del panel: textos con contorno
    // proporcional (54, 30 y 16 de token) y banderines.
    Vector2 izquierda = PuntoRef(140.0f, 118.0f);
    Vector2 derecha = PuntoRef(1160.0f, 140.0f);

    DibujarTextoMundoContornoUI("¡YA!", izquierda.x, izquierda.y, UI_TEXTO_TITULO, UI_FOCO);
    DibujarTextoMundoContornoUI("RONDA 3", derecha.x, derecha.y, UI_TEXTO_GRANDE, UI_TEXTO_MUNDO);
    DibujarTextoMundoContornoUI("+3 MONEDAS", derecha.x, derecha.y + Px(44.0f), UI_TEXTO_PEQUENO, UI_FOCO);

    DibujarBanderinUI(estilo, "SALIDA", REF_BANDERIN_IZQ, ALTO_MASTIL, UI_PELIGRO, true, 0.0f);
    DibujarBanderinUI(estilo, "META", REF_BANDERIN_DER, ALTO_MASTIL, UI_POSITIVO, true, 1.7f);

    // Marco con recorte: todo lo que se dibuja entre Comenzar y Terminar
    // queda dentro del area.
    Rectangle area = ComenzarMarcoIlustracionUI(estilo, REF_MARCO);
    DibujarEscenaMarco(demo, area);
    TerminarMarcoIlustracionUI(estilo, REF_MARCO);

    // Etiqueta fuera del recorte: puede pisar el marco.
    OpcionesEtiquetaUI opcionesMarco;
    opcionesMarco.borde = UI_MADERA_OSCURA;
    DibujarEtiquetaUI("RECORTE", PuntoRef(REF_MARCO.x + 46.0f, REF_MARCO.y + 4.0f), -7.0f, opcionesMarco);

    // Parrafo con ajuste de linea dentro de la tarjeta, con alto maximo:
    // la fuente baja de 20 px hasta que entra.
    DibujarTarjetaUI(estilo, demo.tarjetaParrafo);

    if (demo.tarjetaParrafo.colgado.retraso <= 0.0f)
    {
        Rectangle ref = AreaContenidoTarjetaUI(demo.tarjetaParrafo);
        Rectangle rect = RectRef(ref.x, ref.y, ref.width, ref.height);
        rect.y += demo.tarjetaParrafo.colgado.caida.valor * EscalaUI();

        float altoInfo = (float)TamanoTexto(UI_TEXTO_PEQUENO) + Px(8.0f);

        MedidaParrafoUI medida = DibujarParrafoUI(
            TEXTO_PARRAFO,
            rect.x,
            rect.y,
            UI_TEXTO,
            rect.width,
            UI_TINTA,
            (AlineacionUI)demo.alineacionParrafo,
            rect.height - altoInfo
        );

        DibujarTextoAjustadoUI(
            TextFormat(
                "%s - %d líneas a %d px, alto %d px",
                NOMBRES_ALINEACION[demo.alineacionParrafo],
                medida.lineas,
                medida.tamano,
                (int)medida.alto
            ),
            rect.x,
            rect.y + rect.height - (float)TamanoTexto(UI_TEXTO_PEQUENO),
            UI_TEXTO_PEQUENO,
            rect.width,
            UI_TINTA_SUAVE,
            false
        );

        if (foco)
        {
            Rectangle tarjeta = RectRef(
                REF_TARJETA_PARRAFO.x,
                REF_TARJETA_PARRAFO.y,
                REF_TARJETA_PARRAFO.width,
                REF_TARJETA_PARRAFO.height
            );
            tarjeta.y += demo.tarjetaParrafo.colgado.caida.valor * EscalaUI();

            DibujarMarcoFoco(tarjeta);
        }
    }

    // Sellos: circulares y estrellados, con texto corto o numero.
    DibujarSelloUI("x2", PuntoRef(874.0f, 262.0f), 34.0f, UI_SELLO_CIRCULO, UI_PELIGRO, -10.0f);
    DibujarSelloUI("NUEVO", PuntoRef(964.0f, 268.0f), 42.0f, UI_SELLO_ESTRELLA, UI_FOCO, 8.0f);
    DibujarSelloUI("1º", PuntoRef(876.0f, 354.0f), 32.0f, UI_SELLO_ESTRELLA, UI_POSITIVO);
    DibujarSelloUI("x3", PuntoRef(964.0f, 360.0f), 30.0f, UI_SELLO_CIRCULO, Color{ 70, 130, 210, 255 }, 12.0f);

    // Etiquetas compactas rotadas (colores de jugador) y con pico.
    const char* textos[6] = { "J1", "J2", "J3", "J4", "¡AHORA!", "EN RACHA" };
    const Color bordes[6] = {
        UI_PELIGRO, Color{ 70, 130, 210, 255 }, UI_POSITIVO, UI_FOCO, UI_PELIGRO, UI_MADERA_MEDIA
    };
    const float xs[6] = { 316.0f, 376.0f, 436.0f, 496.0f, 640.0f, 860.0f };
    const float rotaciones[6] = { -6.0f, 4.0f, -3.0f, 7.0f, -8.0f, 5.0f };

    for (int i = 0; i < 6; i++)
    {
        OpcionesEtiquetaUI opciones;
        opciones.borde = bordes[i];
        opciones.tokenTexto = i >= 4 ? UI_TEXTO : UI_TEXTO_PEQUENO;
        opciones.pico = i == 4;

        DibujarEtiquetaUI(textos[i], PuntoRef(xs[i], 440.0f), rotaciones[i], opciones);
    }
}


// Velo encima de todo (menos los avisos) con un texto sobre el mundo.
static void DibujarVeloExtras(const DemoEstiloUI& demo)
{
    if (demo.velo.aparicion <= 0.0f)
    {
        return;
    }

    DibujarVeloUI(demo.velo);

    Vector2 centro = PuntoRef(640.0f, 300.0f);

    DibujarTextoMundoContornoUI("VELO", centro.x, centro.y, UI_TEXTO_TITULO, UI_TEXTO_MUNDO, demo.velo.aparicion);
    DibujarTextoMundoContornoUI(
        "ACEPTÁ PARA QUITARLO",
        centro.x,
        centro.y + Px(70.0f),
        UI_TEXTO_GRANDE,
        UI_FOCO,
        demo.velo.aparicion
    );
}


void DemoEstiloUI::Dibujar() const
{
    DibujarFondoDemo();

    bool hayModal = modalSalir.abierto || modalAplicar.abierto;

    DibujarTituloUI(estilo, colgadoTitulo, "CONFIGURACION", REF_TITULO);
    DibujarPanelUI(estilo, panel);

    for (int i = 0; i < DEMO_PESTANAS; i++)
    {
        DibujarBotonUI(estilo, pestanas[i], false);
    }

    bool extras = pestanaActiva == PESTANA_EXTRAS;

    if (extras && filasListas)
    {
        DibujarExtras(*this, enfoque.indice == 0 && !hayModal);
    }

    for (int fila = 0; fila < (filasListas && !extras ? DEMO_FILAS : 0); fila++)
    {
        int tipo = FILAS_DEMO[pestanaActiva][fila][0];
        int indice = FILAS_DEMO[pestanaActiva][fila][1];
        bool foco = enfoque.indice == fila && !hayModal;

        if (tipo == FILA_SELECTOR)
        {
            DibujarSelectorUI(
                estilo,
                selectores[indice],
                OPCIONES_SELECTOR[indice][opcionSelector[indice]],
                foco
            );
        }
        else if (tipo == FILA_PALANCA)
        {
            if (indice == 0)
            {
                DibujarPalancaUI(
                    estilo,
                    palancas[indice],
                    foco,
                    TextFormat("%d FPS", OPCIONES_FPS[IndiceFps(palancas[indice])])
                );
            }
            else
            {
                DibujarPalancaUI(estilo, palancas[indice], foco);
            }
        }
        else if (tipo == FILA_INTERRUPTOR)
        {
            DibujarInterruptorUI(estilo, interruptores[indice], foco);
        }
        else
        {
            DibujarTarjetaUI(estilo, tarjeta);
            DibujarLineaTarjetaUI(tarjeta, 0, "Mando 1", "Conectado (cruceta y stick)");
            DibujarLineaTarjetaUI(tarjeta, 1, "Teclado", "Flechas o WASD, Enter y Esc para volver");
        }
    }

    bool enBotones = enfoque.indice == FilasConFoco(pestanaActiva) && !hayModal;

    DibujarBotonUI(estilo, botonVolver, enBotones && botonConFoco == 0);
    DibujarBotonUI(estilo, botonAplicar, enBotones && botonConFoco == 1);

    DibujarAyudaUI(
        estilo,
        extras
            ? "IZQ/DER: ALINEACIÓN   ENTER/A: VELO   ARRIBA/ABAJO: BOTONES   Q-E / LB-RB: PESTANAS   ESC/B: VOLVER"
            : "FLECHAS/WASD O CRUCETA: MOVER   ENTER/A: ACEPTAR   Q-E / LB-RB: PESTANAS   ESC/B: VOLVER"
    );

    DibujarVeloExtras(*this);

    DibujarModalUI(
        estilo,
        modalSalir,
        "Los cambios que no aplicaste se van a perder. Elegí SEGUIR EDITANDO para quedarte."
    );

    float fraccion = tiempoConfirmacion / DURACION_CONFIRMACION;

    DibujarModalUI(
        estilo,
        modalAplicar,
        TextFormat("Si no confirmás, se revierten en %d s.", (int)std::ceil(tiempoConfirmacion)),
        fraccion < 0.0f ? 0.0f : fraccion
    );
}
