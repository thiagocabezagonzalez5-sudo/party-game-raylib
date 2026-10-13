#include "UI/DemoEstiloUI.h"

#include <cmath>


//==================================================
// DATOS DE LA DEMO
//==================================================

enum TipoFilaDemo
{
    FILA_SELECTOR = 0,
    FILA_PALANCA,
    FILA_INTERRUPTOR
};

// Por pestana: tipo de fila y su indice dentro del arreglo del tipo.
static const int FILAS_DEMO[DEMO_PESTANAS][DEMO_FILAS][2] =
{
    { { FILA_SELECTOR, 0 }, { FILA_PALANCA, 0 }, { FILA_INTERRUPTOR, 0 } },
    { { FILA_PALANCA, 1 }, { FILA_PALANCA, 2 }, { FILA_INTERRUPTOR, 1 } },
    { { FILA_SELECTOR, 1 }, { FILA_INTERRUPTOR, 2 }, { FILA_INTERRUPTOR, 3 } }
};

static const char* NOMBRES_PESTANA[DEMO_PESTANAS] =
{
    "VIDEO",
    "AUDIO",
    "JUEGO"
};

static const char* OPCIONES_SELECTOR[2][3] =
{
    { "VENTANA", "COMPLETA", "SIN BORDES" },
    { "FACIL", "NORMAL", "DIFICIL" }
};

static const float REPOSO_PESTANA[DEMO_PESTANAS] = { 1.2f, -1.0f, 1.4f };

// Layout en unidades de referencia (1280x720).
static const Rectangle REF_PANEL = { 240.0f, 100.0f, 800.0f, 520.0f };
static const Rectangle REF_TITULO = { 450.0f, 22.0f, 380.0f, 60.0f };
static const float Y_FILAS = 232.0f;
static const float PASO_FILAS = 66.0f;
static const float Y_BOTONES = 468.0f;


static Rectangle RectFila(int fila)
{
    return Rectangle{ 290.0f, Y_FILAS + fila * PASO_FILAS, 700.0f, UI_ALTO_FILA + 6.0f };
}


static void IniciarFilasPestana(DemoEstiloUI& demo, float retrasoBase)
{
    for (int fila = 0; fila < DEMO_FILAS; fila++)
    {
        int tipo = FILAS_DEMO[demo.pestanaActiva][fila][0];
        int indice = FILAS_DEMO[demo.pestanaActiva][fila][1];
        float retraso = retrasoBase + fila * UI_T_ESCALONADO;
        Rectangle rect = RectFila(fila);

        if (tipo == FILA_SELECTOR)
        {
            const char* etiqueta = indice == 0 ? "Modo de pantalla" : "Dificultad";
            demo.selectores[indice].Iniciar(rect, etiqueta, retraso);
        }
        else if (tipo == FILA_PALANCA)
        {
            const char* etiquetas[3] = { "Brillo", "Musica", "Efectos" };
            float valores[3] = { 0.7f, 0.45f, 0.8f };
            demo.palancas[indice].Iniciar(rect, etiquetas[indice], valores[indice], retraso);
        }
        else
        {
            const char* etiquetas[4] = { "VSync", "Silenciar todo", "Reducir movimiento", "Mostrar ayudas" };
            bool valores[4] = { true, false, demo.estilo.reducirMovimiento, true };
            demo.interruptores[indice].Iniciar(rect, etiquetas[indice], valores[indice], retraso);
        }
    }
}


//==================================================
// INICIALIZAR / DESCARGAR
//==================================================

void DemoEstiloUI::Inicializar()
{
    CargarEstiloUI(estilo);

    salir = false;
    tiempo = 0.0f;
    pestanaActiva = 0;
    botonConFoco = 0;
    enfoque.indice = 0;
    enfoque.cantidad = DEMO_FILAS + 1;
    opcionSelector[0] = 1;
    opcionSelector[1] = 1;

    panel.Iniciar(REF_PANEL, 0.0f);
    colgadoTitulo.Iniciar(-1.0f, 0.45f, 120.0f, 1.0f);

    for (int i = 0; i < DEMO_PESTANAS; i++)
    {
        pestanas[i].Iniciar(
            Rectangle{ 300.0f + i * 225.0f, 144.0f, 205.0f, 46.0f },
            NOMBRES_PESTANA[i],
            REPOSO_PESTANA[i],
            0.65f + i * UI_T_ESCALONADO
        );
        pestanas[i].activo = i == pestanaActiva;
    }

    botonVolver.Iniciar(Rectangle{ 380.0f, Y_BOTONES, 220.0f, UI_ALTO_BOTON }, "VOLVER", -1.2f, 1.0f);
    botonAplicar.Iniciar(Rectangle{ 680.0f, Y_BOTONES, 220.0f, UI_ALTO_BOTON }, "APLICAR", 1.0f, 1.07f);

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
    demo.filasListas = false;
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
    int fila = demo.enfoque.indice;
    int tipo = FILAS_DEMO[demo.pestanaActiva][fila][0];
    int indice = FILAS_DEMO[demo.pestanaActiva][fila][1];

    if (tipo == FILA_SELECTOR)
    {
        demo.opcionSelector[indice] = (demo.opcionSelector[indice] + direccion + 3) % 3;
        GolpearSelectorUI(demo.selectores[indice], direccion);
    }
    else if (tipo == FILA_PALANCA)
    {
        AjustarPalancaUI(demo.palancas[indice], direccion);
    }
    else
    {
        demo.interruptores[indice].activo = direccion > 0;
    }
}


void DemoEstiloUI::Actualizar(float deltaTime, const EntradaUI& entrada)
{
    tiempo += deltaTime;
    ActualizarEstiloUI(estilo, deltaTime);

    // Los interruptores de movimiento reducido gobiernan el estilo.
    estilo.reducirMovimiento = interruptores[2].activo;

    if (entrada.volver)
    {
        salir = true;
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

    bool enBotones = enfoque.indice == DEMO_FILAS;

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
                salir = true;
            }
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

    ActualizarBotonUI(botonVolver, estilo, deltaTime, enBotones && botonConFoco == 0);
    ActualizarBotonUI(botonAplicar, estilo, deltaTime, enBotones && botonConFoco == 1);

    for (int i = 0; i < 2; i++)
    {
        ActualizarSelectorUI(selectores[i], estilo, deltaTime);
    }

    for (int i = 0; i < 3; i++)
    {
        ActualizarPalancaUI(palancas[i], estilo, deltaTime);
    }

    for (int i = 0; i < 4; i++)
    {
        ActualizarInterruptorUI(interruptores[i], estilo, deltaTime);
    }
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


void DemoEstiloUI::Dibujar() const
{
    DibujarFondoDemo();

    DibujarTituloUI(estilo, colgadoTitulo, "CONFIGURACION", REF_TITULO);
    DibujarPanelUI(estilo, panel);

    for (int i = 0; i < DEMO_PESTANAS; i++)
    {
        DibujarBotonUI(estilo, pestanas[i], false);
    }

    for (int fila = 0; fila < (filasListas ? DEMO_FILAS : 0); fila++)
    {
        int tipo = FILAS_DEMO[pestanaActiva][fila][0];
        int indice = FILAS_DEMO[pestanaActiva][fila][1];
        bool foco = enfoque.indice == fila;

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
            DibujarPalancaUI(estilo, palancas[indice], foco);
        }
        else
        {
            DibujarInterruptorUI(estilo, interruptores[indice], foco);
        }
    }

    bool enBotones = enfoque.indice == DEMO_FILAS;

    DibujarBotonUI(estilo, botonVolver, enBotones && botonConFoco == 0);
    DibujarBotonUI(estilo, botonAplicar, enBotones && botonConFoco == 1);

    DibujarAyudaUI(
        estilo,
        "FLECHAS/WASD O CRUCETA: MOVER   ENTER/A: ACEPTAR   Q-E / LB-RB: PESTANAS   ESC/B: SALIR"
    );
}
