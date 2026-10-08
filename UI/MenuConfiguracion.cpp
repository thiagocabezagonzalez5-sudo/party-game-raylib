#include "UI/MenuConfiguracion.h"
#include "Systems/Input.h"

#include <cmath>
#include <cstdio>

#include "Minigames/EfectosVisualesMinijuegos.h"


//==================================================
// COLORES (mismos paneles que HUB y FlujoPartida)
//==================================================

static const Color COLOR_ACENTO = { 255, 120, 20, 255 };
static const Color COLOR_VELO = { 10, 12, 18, 255 };
static const Color COLOR_PANEL = { 24, 27, 36, 240 };
static const Color COLOR_FILA = { 34, 38, 50, 255 };
static const Color COLOR_FILA_SEL = { 48, 54, 74, 255 };
static const Color COLOR_BARRA = { 14, 16, 22, 255 };


//==================================================
// TABLAS DE OPCIONES
//==================================================

static const int MAX_FILAS = 5;

static const char* NOMBRES_CATEGORIAS[CANTIDAD_CATEGORIAS_CONFIGURACION] =
{
    "VIDEO",
    "GRAFICOS",
    "AUDIO",
    "CONTROLES",
    "JUEGO",
    "ACCESIBILIDAD"
};

static const OpcionConfiguracion TABLA_OPCIONES[CANTIDAD_CATEGORIAS_CONFIGURACION][MAX_FILAS] =
{
    {
        OPCION_VIDEO_MODO,
        OPCION_VIDEO_RESOLUCION,
        OPCION_VIDEO_FPS,
        OPCION_VIDEO_MOSTRAR_FPS,
        OPCION_VIDEO_VSYNC
    },
    {
        OPCION_GRAFICO_PRESET,
        OPCION_GRAFICO_PARTICULAS,
        OPCION_GRAFICO_DECORACION,
        OPCION_GRAFICO_EFECTOS,
        OPCION_GRAFICO_SOMBRAS
    },
    {
        OPCION_AUDIO_SONIDOS,
        OPCION_AUDIO_MUSICA,
        OPCION_NINGUNA,
        OPCION_NINGUNA,
        OPCION_NINGUNA
    },
    {
        OPCION_CONTROL_MODO_TECLADO,
        OPCION_CONTROL_ZONA_MUERTA,
        OPCION_NINGUNA,
        OPCION_NINGUNA,
        OPCION_NINGUNA
    },
    {
        OPCION_JUEGO_RESTAURAR,
        OPCION_NINGUNA,
        OPCION_NINGUNA,
        OPCION_NINGUNA,
        OPCION_NINGUNA
    },
    {
        OPCION_ACCESIBILIDAD_MOVIMIENTO,
        OPCION_NINGUNA,
        OPCION_NINGUNA,
        OPCION_NINGUNA,
        OPCION_NINGUNA
    }
};


static int CantidadOpciones(
    int categoria
)
{
    int cantidad = 0;

    while (
        cantidad < MAX_FILAS &&
        TABLA_OPCIONES[categoria][cantidad] != OPCION_NINGUNA
    )
    {
        cantidad++;
    }

    return cantidad;
}


static const char* EtiquetaOpcion(
    int opcion
)
{
    switch (opcion)
    {
        case OPCION_VIDEO_MODO: return "Modo de pantalla";
        case OPCION_VIDEO_RESOLUCION: return "Resolucion";
        case OPCION_VIDEO_FPS: return "Limite de FPS";
        case OPCION_VIDEO_MOSTRAR_FPS: return "Mostrar FPS";
        case OPCION_VIDEO_VSYNC: return "VSync";
        case OPCION_GRAFICO_PRESET: return "Preset grafico";
        case OPCION_GRAFICO_PARTICULAS: return "Particulas";
        case OPCION_GRAFICO_DECORACION: return "Decoracion";
        case OPCION_GRAFICO_EFECTOS: return "Efectos";
        case OPCION_GRAFICO_SOMBRAS: return "Sombras";
        case OPCION_AUDIO_SONIDOS: return "Efectos de sonido";
        case OPCION_AUDIO_MUSICA: return "Musica";
        case OPCION_CONTROL_MODO_TECLADO: return "Modo de teclado";
        case OPCION_CONTROL_ZONA_MUERTA: return "Zona muerta del stick";
        case OPCION_JUEGO_RESTAURAR: return "Restaurar predeterminados";
        case OPCION_ACCESIBILIDAD_MOVIMIENTO: return "Reducir movimiento de camara";
    }

    return "";
}


static const char* DescripcionOpcion(
    int opcion
)
{
    switch (opcion)
    {
        case OPCION_VIDEO_MODO:
            return "Ventana, pantalla completa o ventana sin bordes. Pide confirmacion al cambiar.";

        case OPCION_VIDEO_RESOLUCION:
            return "Tamano de la imagen. En ventana sin bordes se usa la del monitor (bloqueada).";

        case OPCION_VIDEO_FPS:
            return "Tope de cuadros por segundo. Con VSync activo el monitor tambien limita.";

        case OPCION_VIDEO_MOSTRAR_FPS:
            return "Muestra un contador de FPS en la esquina durante todo el juego.";

        case OPCION_VIDEO_VSYNC:
            return "Sincroniza con el monitor para evitar cortes en la imagen.";

        case OPCION_GRAFICO_PRESET:
            return "Fija las cuatro opciones a la vez. Cambiar una individual pasa a PERSONALIZADO.";

        case OPCION_GRAFICO_PARTICULAS:
            return "Menos particulas ambientales (petalos, brasas, humo, chispas) en tableros y minijuegos.";

        case OPCION_GRAFICO_DECORACION:
            return "Menos props decorativos y elementos lejanos fuera del area jugable.";

        case OPCION_GRAFICO_EFECTOS:
            return "Menos efectos secundarios: agua animada, destellos y haces de luz.";

        case OPCION_GRAFICO_SOMBRAS:
            return "Menos manchas de sombra simulada bajo personajes y objetos.";

        case OPCION_AUDIO_SONIDOS:
            return "Volumen de los efectos. Al cambiarlo suena una muestra.";

        case OPCION_AUDIO_MUSICA:
            return "Volumen de la musica de fondo. 0% la silencia.";

        case OPCION_CONTROL_MODO_TECLADO:
            return "1 jugador: un teclado completo. 2 jugadores: WASD (J1) y flechas (J2).";

        case OPCION_CONTROL_ZONA_MUERTA:
            return "Inclinacion del stick que se ignora (todos los mandos). Subela si el personaje se mueve solo.";

        case OPCION_ACCESIBILIDAD_MOVIMIENTO:
            return "Quita los temblores de camara en minijuegos.";

        case OPCION_JUEGO_RESTAURAR:
            return "Devuelve video, graficos, audio y controles a sus valores originales (pide confirmar).";
    }

    return "";
}


static bool EsSlider(
    int opcion
)
{
    return
        opcion == OPCION_AUDIO_SONIDOS ||
        opcion == OPCION_AUDIO_MUSICA ||
        opcion == OPCION_CONTROL_ZONA_MUERTA;
}


// Posicion 0..1 del slider. La zona muerta recorre su rango en pasos.
static float ValorSlider(
    const ConfiguracionJuego& config,
    int opcion
)
{
    if (opcion == OPCION_CONTROL_ZONA_MUERTA)
    {
        return
            (float)(config.zonaMuertaStick - ZONA_MUERTA_STICK_MINIMA) /
            (float)(ZONA_MUERTA_STICK_MAXIMA - ZONA_MUERTA_STICK_MINIMA);
    }

    return opcion == OPCION_AUDIO_SONIDOS
        ? config.volumenSonidos
        : config.volumenMusica;
}


static int PorcentajeSlider(
    const ConfiguracionJuego& config,
    int opcion
)
{
    if (opcion == OPCION_CONTROL_ZONA_MUERTA)
    {
        return config.zonaMuertaStick;
    }

    return (int)std::lround(ValorSlider(config, opcion) * 100.0f);
}


static bool EsInterruptor(
    int opcion
)
{
    return
        opcion == OPCION_VIDEO_MOSTRAR_FPS ||
        opcion == OPCION_VIDEO_VSYNC ||
        opcion == OPCION_ACCESIBILIDAD_MOVIMIENTO;
}


static bool EsAccion(
    int opcion
)
{
    return opcion == OPCION_JUEGO_RESTAURAR;
}


//==================================================
// UTILIDADES
//==================================================

static int Envolver(
    int valor,
    int cantidad
)
{
    if (cantidad <= 0)
    {
        return 0;
    }

    while (valor < 0)
    {
        valor += cantidad;
    }

    return valor % cantidad;
}


static float LimitarFloat(
    float valor,
    float minimo,
    float maximo
)
{
    if (valor < minimo)
    {
        return minimo;
    }

    if (valor > maximo)
    {
        return maximo;
    }

    return valor;
}


static int TamanoFuente(
    float base,
    float escala
)
{
    int tamano = (int)(base * escala);

    return tamano < 13 ? 13 : tamano;
}


// Dibuja el texto reduciendo la fuente hasta que quepa en maxAncho.
static void DibujarTextoAjustado(
    const char* texto,
    float x,
    float y,
    int tamano,
    float maxAncho,
    Color color,
    bool centrado
)
{
    while (tamano > 11 && MeasureText(texto, tamano) > maxAncho)
    {
        tamano--;
    }

    float ancho = (float)MeasureText(texto, tamano);

    if (centrado)
    {
        x -= ancho / 2.0f;
    }

    DrawText(texto, (int)x, (int)y, tamano, color);
}


static bool MandoPulsado(
    int boton
)
{
    for (int g = 0; g < 4; g++)
    {
        if (
            IsGamepadAvailable(g) &&
            IsGamepadButtonPressed(g, boton)
        )
        {
            return true;
        }
    }

    return false;
}


static bool MandoAbajo(
    int boton
)
{
    for (int g = 0; g < 4; g++)
    {
        if (
            IsGamepadAvailable(g) &&
            IsGamepadButtonDown(g, boton)
        )
        {
            return true;
        }
    }

    return false;
}


static float EjeMando(
    int eje
)
{
    float mejor = 0.0f;

    for (int g = 0; g < 4; g++)
    {
        if (!IsGamepadAvailable(g))
        {
            continue;
        }

        // Stick ya filtrado por la zona muerta configurada.
        Vector2 stick = LeerStickIzquierdo(g);
        float valor = eje == GAMEPAD_AXIS_LEFT_X ? stick.x : stick.y;

        if (std::fabs(valor) > std::fabs(mejor))
        {
            mejor = valor;
        }
    }

    return mejor;
}


static void Sonido(
    AudioJuego& audio,
    TipoSonidoJuego tipo
)
{
    audio.ReproducirSonido(tipo);
}


//==================================================
// DISPOSICION (compartida por Actualizar y Dibujar)
//==================================================

struct DisposicionConfig
{
    float escala;
    Rectangle panel;
    Rectangle pestanas[CANTIDAD_CATEGORIAS_CONFIGURACION];
    Rectangle botonLB;
    Rectangle botonRB;
    Rectangle filas[MAX_FILAS];
    Rectangle zonaExtra;
    Rectangle info;
    Rectangle botonVolver;
    Rectangle ayuda;
    Rectangle modal;
    Rectangle modalMantener;
    Rectangle modalRevertir;
};


static DisposicionConfig CalcularDisposicion(
    float desplazamientoY
)
{
    DisposicionConfig d{};

    const float w = (float)GetScreenWidth();
    const float h = (float)GetScreenHeight();

    float s = w / 1280.0f;

    if (h / 720.0f < s)
    {
        s = h / 720.0f;
    }

    s = LimitarFloat(s, 0.55f, 2.2f);
    d.escala = s;

    float anchoPanel = 1040.0f * s;

    if (anchoPanel > w - 24.0f)
    {
        anchoPanel = w - 24.0f;
    }

    const float altoPanel = 600.0f * s;

    d.panel =
    {
        (w - anchoPanel) / 2.0f,
        (h - altoPanel) / 2.0f + desplazamientoY,
        anchoPanel,
        altoPanel
    };

    const float margen = 28.0f * s;
    const float interior = d.panel.width - margen * 2.0f;

    // Pestanas con los botones LB / RB a los costados.
    const float yPestanas = d.panel.y + 78.0f * s;
    const float altoPestana = 44.0f * s;
    const float anchoBoton = 46.0f * s;
    const float hueco = 6.0f * s;

    d.botonLB = { d.panel.x + margen, yPestanas, anchoBoton, altoPestana };
    d.botonRB =
    {
        d.panel.x + d.panel.width - margen - anchoBoton,
        yPestanas,
        anchoBoton,
        altoPestana
    };

    const float xInicio = d.botonLB.x + anchoBoton + hueco;
    const float xFin = d.botonRB.x - hueco;
    const float anchoPestana =
        (xFin - xInicio - hueco * (CANTIDAD_CATEGORIAS_CONFIGURACION - 1)) /
        CANTIDAD_CATEGORIAS_CONFIGURACION;

    for (int i = 0; i < CANTIDAD_CATEGORIAS_CONFIGURACION; i++)
    {
        d.pestanas[i] =
        {
            xInicio + i * (anchoPestana + hueco),
            yPestanas,
            anchoPestana,
            altoPestana
        };
    }

    const float yFilas = d.panel.y + 142.0f * s;
    const float altoFila = 54.0f * s;
    const float pasoFila = 64.0f * s;

    for (int i = 0; i < MAX_FILAS; i++)
    {
        d.filas[i] =
        {
            d.panel.x + margen,
            yFilas + i * pasoFila,
            interior,
            altoFila
        };
    }

    // Zona bajo la primera fila (tarjetas de controles, resumen, notas).
    d.zonaExtra =
    {
        d.panel.x + margen,
        yFilas + pasoFila,
        interior,
        d.panel.y + 456.0f * s - (yFilas + pasoFila)
    };

    d.info =
    {
        d.panel.x + margen,
        d.panel.y + 462.0f * s,
        interior,
        38.0f * s
    };

    d.botonVolver =
    {
        d.panel.x + d.panel.width - margen - 150.0f * s,
        d.panel.y + 514.0f * s,
        150.0f * s,
        40.0f * s
    };

    d.ayuda =
    {
        d.panel.x + margen,
        d.panel.y + 512.0f * s,
        interior - 170.0f * s,
        60.0f * s
    };

    d.modal =
    {
        (w - 560.0f * s) / 2.0f,
        (h - 240.0f * s) / 2.0f,
        560.0f * s,
        240.0f * s
    };

    d.modalMantener =
    {
        d.modal.x + 30.0f * s,
        d.modal.y + d.modal.height - 70.0f * s,
        240.0f * s,
        44.0f * s
    };

    d.modalRevertir =
    {
        d.modal.x + d.modal.width - 30.0f * s - 240.0f * s,
        d.modal.y + d.modal.height - 70.0f * s,
        240.0f * s,
        44.0f * s
    };

    return d;
}


static Rectangle ControlDeFila(
    Rectangle fila
)
{
    return
    {
        fila.x + fila.width * 0.46f,
        fila.y,
        fila.width * 0.54f - 14.0f,
        fila.height
    };
}


static Rectangle BarraDeFila(
    Rectangle fila,
    float escala
)
{
    Rectangle control = ControlDeFila(fila);

    return
    {
        control.x,
        fila.y + fila.height / 2.0f - 8.0f * escala,
        control.width - 78.0f * escala,
        16.0f * escala
    };
}


//==================================================
// TEXTOS DE VALORES
//==================================================

static bool EsResolucionNativa(
    Resolucion resolucion
)
{
    int monitor = GetCurrentMonitor();

    return
        resolucion.ancho == GetMonitorWidth(monitor) &&
        resolucion.alto == GetMonitorHeight(monitor);
}


static const char* TextoResolucion(
    const ConfiguracionJuego& config,
    Resolucion resoluciones[]
)
{
    Resolucion r = resoluciones[config.indiceResolucion];

    if (config.modoVentana == MODO_SIN_BORDES)
    {
        int monitor = GetCurrentMonitor();

        return TextFormat(
            "Monitor %d x %d",
            GetMonitorWidth(monitor),
            GetMonitorHeight(monitor)
        );
    }

    return TextFormat(
        EsResolucionNativa(r) ? "%d x %d (monitor)" : "%d x %d",
        r.ancho,
        r.alto
    );
}


static const char* NombreModoTeclado(
    ModoTeclado modo
)
{
    return modo == TECLADO_DIVIDIDO
        ? "2 JUGADORES"
        : "1 JUGADOR";
}


static const char* NombreNivelCalidad(
    int nivel
)
{
    switch (nivel)
    {
        case CALIDAD_BAJA: return "BAJO";
        case CALIDAD_MEDIA: return "MEDIO";
        default: return "ALTO";
    }
}


static const char* NombrePresetGrafico(
    int preset
)
{
    return preset == PRESET_GRAFICO_PERSONALIZADO
        ? "PERSONALIZADO"
        : NombreNivelCalidad(preset);
}


static int* CampoCalidad(
    ConfiguracionJuego& config,
    int opcion
)
{
    switch (opcion)
    {
        case OPCION_GRAFICO_PARTICULAS: return &config.calidadParticulas;
        case OPCION_GRAFICO_DECORACION: return &config.calidadDecoracion;
        case OPCION_GRAFICO_EFECTOS: return &config.calidadEfectos;
        case OPCION_GRAFICO_SOMBRAS: return &config.calidadSombras;
    }

    return nullptr;
}


static void AplicarCalidadGrafica(
    MenuConfiguracion& menu,
    ConfiguracionJuego& config
)
{
    ActualizarPresetGrafico(config);
    EstablecerOpcionesCalidadGrafica(ObtenerOpcionesCalidadDesdeConfiguracion(config));
    menu.configuracionCambiada = true;
}


static int IndiceNativo(
    Resolucion resoluciones[],
    int cantidad
)
{
    for (int i = 0; i < cantidad; i++)
    {
        if (EsResolucionNativa(resoluciones[i]))
        {
            return i;
        }
    }

    return cantidad > 0 ? cantidad - 1 : 0;
}


//==================================================
// VIDEO: CAMBIO CON CONFIRMACION
//==================================================

static void IniciarCambioVideo(
    MenuConfiguracion& menu,
    ConfiguracionJuego& config,
    Resolucion resoluciones[],
    ModoVentana nuevoModo,
    int nuevoIndice
)
{
    if (
        nuevoModo == config.modoVentana &&
        nuevoIndice == config.indiceResolucion
    )
    {
        return;
    }

    menu.modoPrevio = config.modoVentana;
    menu.resolucionPrevia = config.indiceResolucion;

    ModoVentana actual = config.modoVentana;

    config.indiceResolucion = nuevoIndice;

    AplicarModoVentana(
        actual,
        nuevoModo,
        resoluciones[nuevoIndice]
    );

    config.modoVentana = actual;

    menu.confirmandoVideo = true;
    menu.tiempoConfirmacion = menu.DURACION_CONFIRMACION;
    menu.opcionArrastrada = OPCION_NINGUNA;
}


static void RevertirVideo(
    MenuConfiguracion& menu,
    ConfiguracionJuego& config,
    Resolucion resoluciones[]
)
{
    ModoVentana actual = config.modoVentana;

    AplicarModoVentana(
        actual,
        menu.modoPrevio,
        resoluciones[menu.resolucionPrevia]
    );

    config.modoVentana = actual;
    config.indiceResolucion = menu.resolucionPrevia;
    menu.confirmandoVideo = false;
}


//==================================================
// VOLUMENES (con preview de sonido)
//==================================================

static void CambiarVolumenSonidos(
    MenuConfiguracion& menu,
    ConfiguracionJuego& config,
    AudioJuego& audio,
    float nuevo
)
{
    nuevo = LimitarFloat(nuevo, 0.0f, 1.0f);

    if (std::fabs(nuevo - config.volumenSonidos) < 0.001f)
    {
        return;
    }

    config.volumenSonidos = nuevo;
    audio.AplicarVolumenSonidos(nuevo);
    menu.configuracionCambiada = true;

    if (menu.enfriamientoPreview <= 0.0f)
    {
        Sonido(audio, SONIDO_UI_MOVER);
        menu.enfriamientoPreview = 0.12f;
    }
}


static void CambiarVolumenMusica(
    MenuConfiguracion& menu,
    ConfiguracionJuego& config,
    AudioJuego& audio,
    float nuevo
)
{
    nuevo = LimitarFloat(nuevo, 0.0f, 1.0f);

    if (std::fabs(nuevo - config.volumenMusica) < 0.001f)
    {
        return;
    }

    config.volumenMusica = nuevo;
    audio.AplicarVolumenMusica(nuevo);
    menu.configuracionCambiada = true;
}


static void CambiarZonaMuerta(
    MenuConfiguracion& menu,
    ConfiguracionJuego& config,
    AudioJuego& audio,
    int nuevo
)
{
    nuevo = LimitarZonaMuertaStick(nuevo);

    if (nuevo == config.zonaMuertaStick)
    {
        return;
    }

    config.zonaMuertaStick = nuevo;
    EstablecerZonaMuertaStick(nuevo / 100.0f);
    menu.configuracionCambiada = true;
    audio.ReproducirSonido(SONIDO_UI_MOVER);
}


//==================================================
// ENTRADA
//==================================================

struct EntradaConfig
{
    int vertical = 0;
    int horizontalPulso = 0;
    int horizontalRepetido = 0;
    int categoria = 0;
    bool confirmar = false;
    bool cancelar = false;
};


static EntradaConfig LeerEntrada(
    MenuConfiguracion& menu,
    float deltaTime
)
{
    EntradaConfig e{};

    // Ejes del stick con histeresis simple (solo flancos).
    float ejeX = EjeMando(GAMEPAD_AXIS_LEFT_X);
    float ejeY = EjeMando(GAMEPAD_AXIS_LEFT_Y);

    int stickH = ejeX > 0.4f ? 1 : (ejeX < -0.4f ? -1 : 0);
    int stickV = ejeY > 0.4f ? 1 : (ejeY < -0.4f ? -1 : 0);

    // Vertical: solo flancos.
    if (
        IsKeyPressed(KEY_UP) ||
        IsKeyPressed(KEY_W) ||
        IsKeyPressedRepeat(KEY_UP) ||
        MandoPulsado(GAMEPAD_BUTTON_LEFT_FACE_UP) ||
        (stickV == -1 && menu.ejeVerticalPrevio != -1)
    )
    {
        e.vertical = -1;
    }
    else if (
        IsKeyPressed(KEY_DOWN) ||
        IsKeyPressed(KEY_S) ||
        IsKeyPressedRepeat(KEY_DOWN) ||
        MandoPulsado(GAMEPAD_BUTTON_LEFT_FACE_DOWN) ||
        (stickV == 1 && menu.ejeVerticalPrevio != 1)
    )
    {
        e.vertical = 1;
    }

    menu.ejeVerticalPrevio = stickV;
    menu.ejeHorizontalPrevio = stickH;

    // Horizontal: direccion mantenida con repeticion (para volumenes).
    int izquierda =
        IsKeyDown(KEY_LEFT) ||
        IsKeyDown(KEY_A) ||
        MandoAbajo(GAMEPAD_BUTTON_LEFT_FACE_LEFT) ||
        stickH == -1;

    int derecha =
        IsKeyDown(KEY_RIGHT) ||
        IsKeyDown(KEY_D) ||
        MandoAbajo(GAMEPAD_BUTTON_LEFT_FACE_RIGHT) ||
        stickH == 1;

    int direccion = 0;

    if (derecha && !izquierda)
    {
        direccion = 1;
    }
    else if (izquierda && !derecha)
    {
        direccion = -1;
    }

    if (direccion == 0)
    {
        menu.direccionPrevia = 0;
        menu.tiempoDireccion = 0.0f;
    }
    else if (direccion != menu.direccionPrevia)
    {
        e.horizontalPulso = direccion;
        e.horizontalRepetido = direccion;
        menu.direccionPrevia = direccion;
        menu.tiempoDireccion = 0.0f;
        menu.siguienteRepeticion = 0.35f;
    }
    else
    {
        menu.tiempoDireccion += deltaTime;

        if (menu.tiempoDireccion >= menu.siguienteRepeticion)
        {
            e.horizontalRepetido = direccion;
            menu.siguienteRepeticion += 0.07f;
        }
    }

    if (IsKeyPressed(KEY_Q) || MandoPulsado(GAMEPAD_BUTTON_LEFT_TRIGGER_1))
    {
        e.categoria = -1;
    }
    else if (IsKeyPressed(KEY_E) || MandoPulsado(GAMEPAD_BUTTON_RIGHT_TRIGGER_1))
    {
        e.categoria = 1;
    }

    e.confirmar =
        IsKeyPressed(KEY_ENTER) ||
        IsKeyPressed(KEY_KP_ENTER) ||
        IsKeyPressed(KEY_SPACE) ||
        MandoPulsado(GAMEPAD_BUTTON_RIGHT_FACE_DOWN);

    e.cancelar =
        IsKeyPressed(KEY_ESCAPE) ||
        IsKeyPressed(KEY_BACKSPACE) ||
        MandoPulsado(GAMEPAD_BUTTON_RIGHT_FACE_RIGHT);

    return e;
}


//==================================================
// ACCIONES
//==================================================

static void IniciarSalida(
    MenuConfiguracion& menu
)
{
    menu.saliendo = true;
    menu.opcionArrastrada = OPCION_NINGUNA;
}


static void IrACategoria(
    MenuConfiguracion& menu,
    int categoria
)
{
    categoria = Envolver(categoria, CANTIDAD_CATEGORIAS_CONFIGURACION);

    if (categoria == menu.categoriaActual)
    {
        return;
    }

    menu.categoriaDestino = categoria;
    menu.cambiandoCategoria = true;
    menu.opcionArrastrada = OPCION_NINGUNA;
    menu.restaurarArmado = false;
}


static void RestaurarPredeterminados(
    MenuConfiguracion& menu,
    ConfiguracionJuego& config,
    Resolucion resoluciones[],
    int cantidadResoluciones,
    int opcionesFPS[],
    int cantidadOpcionesFPS,
    AudioJuego& audio
)
{
    ConfiguracionJuego base{};

    config.indiceFPS = cantidadOpcionesFPS > 1 ? 1 : 0;
    config.mostrarFPS = base.mostrarFPS;
    config.vsync = base.vsync;
    config.reducirMovimiento = base.reducirMovimiento;
    EstablecerReducirMovimientoMinijuegos(config.reducirMovimiento);
    config.volumenMusica = base.volumenMusica;
    config.volumenSonidos = base.volumenSonidos;
    config.modoTeclado = base.modoTeclado;
    config.zonaMuertaStick = base.zonaMuertaStick;
    EstablecerZonaMuertaStick(config.zonaMuertaStick / 100.0f);

    config.calidadParticulas = base.calidadParticulas;
    config.calidadDecoracion = base.calidadDecoracion;
    config.calidadEfectos = base.calidadEfectos;
    config.calidadSombras = base.calidadSombras;
    AplicarCalidadGrafica(menu, config);

    SetTargetFPS(opcionesFPS[config.indiceFPS]);
    AplicarVSync(config.vsync);
    audio.AplicarVolumenMusica(config.volumenMusica);
    audio.AplicarVolumenSonidos(config.volumenSonidos);

    menu.configuracionCambiada = true;

    IniciarCambioVideo(
        menu,
        config,
        resoluciones,
        MODO_PANTALLA_COMPLETA,
        IndiceNativo(resoluciones, cantidadResoluciones)
    );
}


// Cambia la opcion indicada. paso: -1 / +1. Devuelve true si se hizo algo.
static void AplicarPaso(
    MenuConfiguracion& menu,
    ConfiguracionJuego& config,
    int opcion,
    int paso,
    Resolucion resoluciones[],
    int cantidadResoluciones,
    int opcionesFPS[],
    int cantidadOpcionesFPS,
    AudioJuego& audio
)
{
    switch (opcion)
    {
        case OPCION_VIDEO_MODO:
        {
            int nuevo = Envolver((int)config.modoVentana + paso, 3);

            IniciarCambioVideo(
                menu,
                config,
                resoluciones,
                (ModoVentana)nuevo,
                config.indiceResolucion
            );

            Sonido(audio, SONIDO_UI_MOVER);
            break;
        }

        case OPCION_VIDEO_RESOLUCION:
        {
            if (config.modoVentana == MODO_SIN_BORDES)
            {
                menu.aviso = "La ventana sin bordes usa la resolucion del monitor.";
                menu.tiempoAviso = 2.5f;
                Sonido(audio, SONIDO_UI_CANCELAR);
                break;
            }

            int nuevo = Envolver(config.indiceResolucion + paso, cantidadResoluciones);

            IniciarCambioVideo(
                menu,
                config,
                resoluciones,
                config.modoVentana,
                nuevo
            );

            Sonido(audio, SONIDO_UI_MOVER);
            break;
        }

        case OPCION_VIDEO_FPS:
        {
            config.indiceFPS = Envolver(config.indiceFPS + paso, cantidadOpcionesFPS);
            SetTargetFPS(opcionesFPS[config.indiceFPS]);
            menu.configuracionCambiada = true;
            Sonido(audio, SONIDO_UI_MOVER);
            break;
        }

        case OPCION_VIDEO_MOSTRAR_FPS:
        {
            config.mostrarFPS = !config.mostrarFPS;
            menu.configuracionCambiada = true;
            Sonido(audio, SONIDO_UI_MOVER);
            break;
        }

        case OPCION_VIDEO_VSYNC:
        {
            config.vsync = !config.vsync;
            AplicarVSync(config.vsync);
            menu.configuracionCambiada = true;
            Sonido(audio, SONIDO_UI_MOVER);
            break;
        }

        case OPCION_GRAFICO_PRESET:
        {
            // Cicla BAJO / MEDIO / ALTO; desde PERSONALIZADO entra por un extremo.
            int actual = config.presetGrafico;

            if (actual == PRESET_GRAFICO_PERSONALIZADO)
            {
                actual = paso > 0 ? -1 : 3;
            }

            int nuevo = Envolver(actual + paso, 3);
            OpcionesCalidadGrafica preset = CrearPresetCalidadGrafica((NivelCalidadGrafica)nuevo);

            config.calidadParticulas = preset.particulas;
            config.calidadDecoracion = preset.decoracion;
            config.calidadEfectos = preset.efectos;
            config.calidadSombras = preset.sombras;
            AplicarCalidadGrafica(menu, config);
            Sonido(audio, SONIDO_UI_MOVER);
            break;
        }

        case OPCION_GRAFICO_PARTICULAS:
        case OPCION_GRAFICO_DECORACION:
        case OPCION_GRAFICO_EFECTOS:
        case OPCION_GRAFICO_SOMBRAS:
        {
            int* campo = CampoCalidad(config, opcion);

            *campo = Envolver(*campo + paso, CANTIDAD_NIVELES_CALIDAD);
            AplicarCalidadGrafica(menu, config);
            Sonido(audio, SONIDO_UI_MOVER);
            break;
        }

        case OPCION_AUDIO_SONIDOS:
        {
            CambiarVolumenSonidos(menu, config, audio, config.volumenSonidos + 0.05f * paso);
            break;
        }

        case OPCION_AUDIO_MUSICA:
        {
            CambiarVolumenMusica(menu, config, audio, config.volumenMusica + 0.05f * paso);
            break;
        }

        case OPCION_CONTROL_ZONA_MUERTA:
        {
            CambiarZonaMuerta(
                menu,
                config,
                audio,
                config.zonaMuertaStick + ZONA_MUERTA_STICK_PASO * paso
            );
            break;
        }

        case OPCION_CONTROL_MODO_TECLADO:
        {
            config.modoTeclado =
                config.modoTeclado == TECLADO_COMPLETO
                    ? TECLADO_DIVIDIDO
                    : TECLADO_COMPLETO;

            menu.configuracionCambiada = true;
            Sonido(audio, SONIDO_UI_MOVER);
            break;
        }

        case OPCION_ACCESIBILIDAD_MOVIMIENTO:
        {
            config.reducirMovimiento = !config.reducirMovimiento;
            EstablecerReducirMovimientoMinijuegos(config.reducirMovimiento);
            menu.configuracionCambiada = true;
            Sonido(audio, SONIDO_UI_MOVER);
            break;
        }

        case OPCION_JUEGO_RESTAURAR:
        {
            if (!menu.restaurarArmado)
            {
                menu.restaurarArmado = true;
                menu.tiempoRestaurar = 4.0f;
                Sonido(audio, SONIDO_UI_MOVER);
            }
            else
            {
                menu.restaurarArmado = false;

                RestaurarPredeterminados(
                    menu,
                    config,
                    resoluciones,
                    cantidadResoluciones,
                    opcionesFPS,
                    cantidadOpcionesFPS,
                    audio
                );

                Sonido(audio, SONIDO_UI_CONFIRMAR);
            }

            break;
        }
    }
}


//==================================================
// INICIALIZAR
//==================================================

void MenuConfiguracion::Inicializar()
{
    categoriaActual = CATEGORIA_VIDEO;

    for (int i = 0; i < CANTIDAD_CATEGORIAS_CONFIGURACION; i++)
    {
        opcionSeleccionada[i] = 0;
    }

    categoriaHover = -1;
    volver = false;
    configuracionCambiada = false;

    alphaGeneral = 0.0f;
    alphaContenido = 1.0f;
    saliendo = false;
    cambiandoCategoria = false;
    categoriaDestino = CATEGORIA_VIDEO;

    bloqueoEntrada = 0.15f;
    direccionPrevia = 0;
    tiempoDireccion = 0.0f;
    siguienteRepeticion = 0.0f;
    ejeHorizontalPrevio = 0;
    ejeVerticalPrevio = 0;
    mouseAnterior = GetMousePosition();

    opcionArrastrada = OPCION_NINGUNA;
    enfriamientoPreview = 0.0f;

    confirmandoVideo = false;
    tiempoConfirmacion = 0.0f;
    restaurarArmado = false;
    tiempoRestaurar = 0.0f;
    aviso = "";
    tiempoAviso = 0.0f;
    vsyncSincronizado = false;
}


//==================================================
// ACTUALIZAR
//==================================================

void MenuConfiguracion::Actualizar(
    ConfiguracionJuego& config,
    Resolucion resoluciones[],
    int cantidadResoluciones,
    int opcionesFPS[],
    int cantidadOpcionesFPS,
    AudioJuego& audio
)
{
    const float deltaTime = GetFrameTime();

    // Valores siempre validos, aunque config venga de un archivo editado.
    NormalizarConfiguracion(
        config,
        cantidadResoluciones,
        cantidadOpcionesFPS,
        IndiceNativo(resoluciones, cantidadResoluciones)
    );

    if (!vsyncSincronizado)
    {
        vsyncSincronizado = true;

        if (IsWindowState(FLAG_VSYNC_HINT) != config.vsync)
        {
            AplicarVSync(config.vsync);
        }
    }

    if (enfriamientoPreview > 0.0f)
    {
        enfriamientoPreview -= deltaTime;
    }

    if (tiempoAviso > 0.0f)
    {
        tiempoAviso -= deltaTime;
    }

    if (restaurarArmado)
    {
        tiempoRestaurar -= deltaTime;

        if (tiempoRestaurar <= 0.0f)
        {
            restaurarArmado = false;
        }
    }

    //------------------------------
    // TRANSICION GENERAL
    //------------------------------

    if (saliendo)
    {
        alphaGeneral -= deltaTime / DURACION_TRANSICION;

        if (alphaGeneral <= 0.0f)
        {
            alphaGeneral = 0.0f;
            volver = true;
        }

        return;
    }

    if (alphaGeneral < 1.0f)
    {
        alphaGeneral += deltaTime / DURACION_TRANSICION;

        if (alphaGeneral > 1.0f)
        {
            alphaGeneral = 1.0f;
        }
    }

    if (bloqueoEntrada > 0.0f)
    {
        bloqueoEntrada -= deltaTime;
    }

    const DisposicionConfig d = CalcularDisposicion(0.0f);
    const Vector2 mouse = GetMousePosition();
    const bool click = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    const bool mouseMovido =
        std::fabs(mouse.x - mouseAnterior.x) +
        std::fabs(mouse.y - mouseAnterior.y) > 0.5f;

    mouseAnterior = mouse;

    EntradaConfig entrada = LeerEntrada(*this, deltaTime);

    if (bloqueoEntrada > 0.0f)
    {
        entrada = EntradaConfig{};
    }

    //------------------------------
    // CONFIRMACION DE VIDEO
    //------------------------------

    if (confirmandoVideo)
    {
        tiempoConfirmacion -= deltaTime;

        bool mantener =
            entrada.confirmar ||
            (click && CheckCollisionPointRec(mouse, d.modalMantener));

        bool revertir =
            entrada.cancelar ||
            tiempoConfirmacion <= 0.0f ||
            (click && CheckCollisionPointRec(mouse, d.modalRevertir));

        if (mantener)
        {
            confirmandoVideo = false;
            configuracionCambiada = true;
            Sonido(audio, SONIDO_UI_CONFIRMAR);
        }
        else if (revertir)
        {
            RevertirVideo(*this, config, resoluciones);
            Sonido(audio, SONIDO_UI_CANCELAR);
        }

        return;
    }

    //------------------------------
    // CAMBIO DE CATEGORIA (fundido)
    //------------------------------

    if (cambiandoCategoria)
    {
        // Las pulsaciones durante el fundido se acumulan en el destino.
        if (entrada.categoria != 0)
        {
            categoriaDestino = Envolver(
                categoriaDestino + entrada.categoria,
                CANTIDAD_CATEGORIAS_CONFIGURACION
            );
        }

        alphaContenido -= deltaTime / DURACION_TRANSICION_CONTENIDO;

        if (alphaContenido <= 0.0f)
        {
            alphaContenido = 0.0f;
            categoriaActual = categoriaDestino;
            cambiandoCategoria = false;
        }

        return;
    }

    if (alphaContenido < 1.0f)
    {
        alphaContenido += deltaTime / DURACION_TRANSICION_CONTENIDO;

        if (alphaContenido > 1.0f)
        {
            alphaContenido = 1.0f;
        }
    }

    //------------------------------
    // SALIR
    //------------------------------

    if (
        entrada.cancelar ||
        (click && CheckCollisionPointRec(mouse, d.botonVolver))
    )
    {
        IniciarSalida(*this);
        return;
    }

    //------------------------------
    // CATEGORIAS
    //------------------------------

    categoriaHover = -1;

    for (int i = 0; i < CANTIDAD_CATEGORIAS_CONFIGURACION; i++)
    {
        if (CheckCollisionPointRec(mouse, d.pestanas[i]))
        {
            categoriaHover = i;
        }
    }

    if (entrada.categoria != 0)
    {
        IrACategoria(*this, categoriaActual + entrada.categoria);
        Sonido(audio, SONIDO_UI_MOVER);
        return;
    }

    if (click)
    {
        if (CheckCollisionPointRec(mouse, d.botonLB))
        {
            IrACategoria(*this, categoriaActual - 1);
            Sonido(audio, SONIDO_UI_MOVER);
            return;
        }

        if (CheckCollisionPointRec(mouse, d.botonRB))
        {
            IrACategoria(*this, categoriaActual + 1);
            Sonido(audio, SONIDO_UI_MOVER);
            return;
        }

        if (categoriaHover >= 0 && categoriaHover != categoriaActual)
        {
            IrACategoria(*this, categoriaHover);
            Sonido(audio, SONIDO_UI_MOVER);
            return;
        }
    }

    //------------------------------
    // OPCIONES DE LA CATEGORIA
    //------------------------------

    const int cantidad = CantidadOpciones(categoriaActual);

    if (cantidad == 0)
    {
        return;
    }

    int& seleccion = opcionSeleccionada[categoriaActual];

    if (entrada.vertical != 0)
    {
        seleccion = Envolver(seleccion + entrada.vertical, cantidad);
        restaurarArmado = false;
        Sonido(audio, SONIDO_UI_MOVER);
    }

    int filaHover = -1;

    for (int i = 0; i < cantidad; i++)
    {
        if (CheckCollisionPointRec(mouse, d.filas[i]))
        {
            filaHover = i;
        }
    }

    if (
        filaHover >= 0 &&
        filaHover != seleccion &&
        (mouseMovido || click) &&
        opcionArrastrada == OPCION_NINGUNA
    )
    {
        seleccion = filaHover;
        restaurarArmado = false;
    }

    const int opcion = TABLA_OPCIONES[categoriaActual][seleccion];

    // Arrastre de sliders con el mouse.
    if (!IsMouseButtonDown(MOUSE_BUTTON_LEFT))
    {
        opcionArrastrada = OPCION_NINGUNA;
    }

    if (click)
    {
        for (int i = 0; i < cantidad; i++)
        {
            int op = TABLA_OPCIONES[categoriaActual][i];

            if (!EsSlider(op))
            {
                continue;
            }

            Rectangle barra = BarraDeFila(d.filas[i], d.escala);
            barra.y -= 10.0f * d.escala;
            barra.height += 20.0f * d.escala;

            if (CheckCollisionPointRec(mouse, barra))
            {
                opcionArrastrada = op;
                seleccion = i;
            }
        }
    }

    if (opcionArrastrada != OPCION_NINGUNA)
    {
        int fila = 0;

        for (int i = 0; i < cantidad; i++)
        {
            if (TABLA_OPCIONES[categoriaActual][i] == opcionArrastrada)
            {
                fila = i;
            }
        }

        Rectangle barra = BarraDeFila(d.filas[fila], d.escala);
        float valor = LimitarFloat((mouse.x - barra.x) / barra.width, 0.0f, 1.0f);

        valor = std::round(valor * 100.0f) / 100.0f;

        if (opcionArrastrada == OPCION_CONTROL_ZONA_MUERTA)
        {
            int pasos = (int)std::lround(
                valor *
                (ZONA_MUERTA_STICK_MAXIMA - ZONA_MUERTA_STICK_MINIMA) /
                ZONA_MUERTA_STICK_PASO
            );

            CambiarZonaMuerta(
                *this,
                config,
                audio,
                ZONA_MUERTA_STICK_MINIMA + pasos * ZONA_MUERTA_STICK_PASO
            );
        }
        else if (opcionArrastrada == OPCION_AUDIO_SONIDOS)
        {
            CambiarVolumenSonidos(*this, config, audio, valor);
        }
        else
        {
            CambiarVolumenMusica(*this, config, audio, valor);
        }

        return;
    }

    // Teclado / mando: izquierda-derecha cambia el valor.
    int paso = EsSlider(opcion) ? entrada.horizontalRepetido : entrada.horizontalPulso;

    bool activar = entrada.confirmar;

    if (click && filaHover == seleccion)
    {
        Rectangle control = ControlDeFila(d.filas[seleccion]);

        if (EsSlider(opcion))
        {
            // El click sobre la barra ya se resolvio arriba.
        }
        else if (
            !EsInterruptor(opcion) &&
            !EsAccion(opcion) &&
            CheckCollisionPointRec(mouse, control) &&
            mouse.x < control.x + control.width / 2.0f
        )
        {
            paso = -1;
        }
        else
        {
            activar = true;
        }
    }

    if (paso != 0 && (EsSlider(opcion) || !EsAccion(opcion)))
    {
        AplicarPaso(
            *this, config, opcion, paso,
            resoluciones, cantidadResoluciones,
            opcionesFPS, cantidadOpcionesFPS, audio
        );
    }
    else if (activar && !EsSlider(opcion))
    {
        AplicarPaso(
            *this, config, opcion, 1,
            resoluciones, cantidadResoluciones,
            opcionesFPS, cantidadOpcionesFPS, audio
        );
    }
}


//==================================================
// DIBUJO: PIEZAS
//==================================================

static void DibujarFlecha(
    float centroX,
    float centroY,
    float tamano,
    int sentido,
    Color color
)
{
    Vector2 a, b, c;

    if (sentido < 0)
    {
        a = { centroX + tamano / 2.0f, centroY - tamano };
        b = { centroX - tamano / 2.0f, centroY };
        c = { centroX + tamano / 2.0f, centroY + tamano };
    }
    else
    {
        a = { centroX - tamano / 2.0f, centroY - tamano };
        b = { centroX - tamano / 2.0f, centroY + tamano };
        c = { centroX + tamano / 2.0f, centroY };
    }

    DrawTriangle(a, b, c, color);
}


static void DibujarFila(
    const DisposicionConfig& d,
    Rectangle fila,
    int opcion,
    bool seleccionada,
    bool bloqueada,
    const ConfiguracionJuego& config,
    Resolucion resoluciones[],
    int opcionesFPS[],
    const MenuConfiguracion& menu,
    float alfa
)
{
    const float s = d.escala;
    const float pulso = seleccionada ? 1.0f + 0.5f * std::sin((float)GetTime() * 6.0f) : 0.0f;

    Rectangle r = fila;

    if (seleccionada)
    {
        float crece = (3.0f + pulso) * s;
        r.x -= crece;
        r.y -= crece / 2.0f;
        r.width += crece * 2.0f;
        r.height += crece;
    }

    DrawRectangleRec(r, Fade(seleccionada ? COLOR_FILA_SEL : COLOR_FILA, 0.92f * alfa));
    DrawRectangleLinesEx(
        r,
        seleccionada ? 3.0f * s : 1.5f,
        Fade(seleccionada ? COLOR_ACENTO : GRAY, (seleccionada ? 1.0f : 0.5f) * alfa)
    );

    if (seleccionada)
    {
        DrawRectangle((int)r.x, (int)r.y, (int)(6.0f * s), (int)r.height, Fade(COLOR_ACENTO, alfa));
    }

    const Color colorTexto = bloqueada ? Fade(GRAY, alfa) : Fade(RAYWHITE, alfa);
    const int fuente = TamanoFuente(24.0f, s);

    DibujarTextoAjustado(
        EtiquetaOpcion(opcion),
        fila.x + 22.0f * s,
        fila.y + fila.height / 2.0f - fuente / 2.0f,
        fuente,
        fila.width * 0.42f - 22.0f * s,
        colorTexto,
        false
    );

    const Rectangle control = ControlDeFila(fila);
    const Color acento = Fade(seleccionada ? COLOR_ACENTO : LIGHTGRAY, alfa);

    if (EsSlider(opcion))
    {
        float valor = ValorSlider(config, opcion);
        Rectangle barra = BarraDeFila(fila, s);

        DrawRectangleRec(barra, Fade(COLOR_BARRA, alfa));
        DrawRectangle(
            (int)barra.x + 2,
            (int)barra.y + 2,
            (int)((barra.width - 4.0f) * valor),
            (int)barra.height - 4,
            Fade(seleccionada ? COLOR_ACENTO : RAYWHITE, alfa)
        );
        DrawRectangleLinesEx(barra, 2.0f, Fade(seleccionada ? COLOR_ACENTO : GRAY, alfa));

        DrawCircle(
            (int)(barra.x + barra.width * valor),
            (int)(barra.y + barra.height / 2.0f),
            9.0f * s,
            Fade(RAYWHITE, alfa)
        );

        const char* texto = TextFormat("%d%%", PorcentajeSlider(config, opcion));

        DrawText(
            texto,
            (int)(barra.x + barra.width + 16.0f * s),
            (int)(fila.y + fila.height / 2.0f - fuente / 2.0f),
            fuente,
            colorTexto
        );

        return;
    }

    if (EsInterruptor(opcion))
    {
        bool activo =
            opcion == OPCION_VIDEO_MOSTRAR_FPS ? config.mostrarFPS :
            (opcion == OPCION_VIDEO_VSYNC ? config.vsync : config.reducirMovimiento);

        Rectangle pastilla =
        {
            control.x,
            fila.y + fila.height / 2.0f - 14.0f * s,
            64.0f * s,
            28.0f * s
        };

        DrawRectangleRounded(
            pastilla,
            1.0f,
            8,
            Fade(activo ? Color{ 70, 190, 90, 255 } : Color{ 70, 74, 90, 255 }, alfa)
        );

        DrawCircle(
            (int)(activo ? pastilla.x + pastilla.width - 14.0f * s : pastilla.x + 14.0f * s),
            (int)(pastilla.y + pastilla.height / 2.0f),
            11.0f * s,
            Fade(RAYWHITE, alfa)
        );

        DrawText(
            activo ? "SI" : "NO",
            (int)(pastilla.x + pastilla.width + 16.0f * s),
            (int)(fila.y + fila.height / 2.0f - fuente / 2.0f),
            fuente,
            colorTexto
        );

        return;
    }

    if (EsAccion(opcion))
    {
        const char* texto = menu.restaurarArmado ? "PULSA OTRA VEZ: RESTAURA TODO" : "RESTAURAR";
        Rectangle boton =
        {
            control.x,
            fila.y + 8.0f * s,
            control.width * 0.8f,
            fila.height - 16.0f * s
        };

        DrawRectangleRec(
            boton,
            Fade(menu.restaurarArmado ? Color{ 200, 60, 50, 255 } : Color{ 60, 66, 88, 255 }, alfa)
        );
        DrawRectangleLinesEx(boton, 2.0f, acento);

        DibujarTextoAjustado(
            texto,
            boton.x + boton.width / 2.0f,
            boton.y + boton.height / 2.0f - fuente / 2.0f,
            fuente,
            boton.width - 16.0f * s,
            Fade(RAYWHITE, alfa),
            true
        );

        return;
    }

    // Selector con flechas.
    const char* texto = "";

    switch (opcion)
    {
        case OPCION_VIDEO_MODO:
            texto = NombreModoVentana(config.modoVentana);
            break;

        case OPCION_VIDEO_RESOLUCION:
            texto = TextoResolucion(config, resoluciones);
            break;

        case OPCION_VIDEO_FPS:
            texto = TextFormat("%d", opcionesFPS[config.indiceFPS]);
            break;

        case OPCION_GRAFICO_PRESET:
            texto = NombrePresetGrafico(config.presetGrafico);
            break;

        case OPCION_GRAFICO_PARTICULAS:
            texto = NombreNivelCalidad(config.calidadParticulas);
            break;

        case OPCION_GRAFICO_DECORACION:
            texto = NombreNivelCalidad(config.calidadDecoracion);
            break;

        case OPCION_GRAFICO_EFECTOS:
            texto = NombreNivelCalidad(config.calidadEfectos);
            break;

        case OPCION_GRAFICO_SOMBRAS:
            texto = NombreNivelCalidad(config.calidadSombras);
            break;

        case OPCION_CONTROL_MODO_TECLADO:
            texto = NombreModoTeclado(config.modoTeclado);
            break;
    }

    const Color colorFlecha = bloqueada ? Fade(DARKGRAY, alfa) : acento;
    const float centroY = fila.y + fila.height / 2.0f;
    const float izq = control.x + 14.0f * s;
    const float der = control.x + control.width * 0.9f - 14.0f * s;

    DibujarFlecha(izq, centroY, 8.0f * s, -1, colorFlecha);
    DibujarFlecha(der, centroY, 8.0f * s, 1, colorFlecha);

    DibujarTextoAjustado(
        texto,
        (izq + der) / 2.0f,
        centroY - fuente / 2.0f,
        fuente,
        der - izq - 40.0f * s,
        colorTexto,
        true
    );
}


static void DibujarTarjeta(
    Rectangle r,
    const char* titulo,
    float s,
    float alfa
)
{
    DrawRectangleRec(r, Fade(COLOR_FILA, 0.85f * alfa));
    DrawRectangleLinesEx(r, 1.5f, Fade(GRAY, 0.5f * alfa));

    DrawText(
        titulo,
        (int)(r.x + 16.0f * s),
        (int)(r.y + 12.0f * s),
        TamanoFuente(20.0f, s),
        Fade(COLOR_ACENTO, alfa)
    );
}


static float DibujarLinea(
    const char* etiqueta,
    const char* valor,
    Rectangle r,
    float y,
    float s,
    float alfa
)
{
    const int fuente = TamanoFuente(16.0f, s);
    const float ancho = r.width - 32.0f * s;

    DibujarTextoAjustado(etiqueta, r.x + 16.0f * s, y, fuente, ancho * 0.40f, Fade(LIGHTGRAY, alfa), false);
    DibujarTextoAjustado(valor, r.x + 16.0f * s + ancho * 0.44f, y, fuente, ancho * 0.56f, Fade(RAYWHITE, alfa), false);

    return y + fuente + 7.0f * s;
}


static void DibujarContenidoExtra(
    const DisposicionConfig& d,
    const ConfiguracionJuego& config,
    Resolucion resoluciones[],
    int opcionesFPS[],
    int categoria,
    float alfa
)
{
    const float s = d.escala;
    const float hueco = 14.0f * s;

    // La zona extra empieza bajo la ULTIMA fila de opciones de la categoria
    // (CalcularDisposicion la deja bajo la primera; las filas avanzan de a
    // 64 px en escala 1).
    Rectangle z = d.zonaExtra;
    const float filasExtra = (float)(CantidadOpciones(categoria) - 1) * 64.0f * s;

    if (filasExtra > 0.0f)
    {
        z.y += filasExtra;
        z.height -= filasExtra;
    }

    if (categoria == CATEGORIA_CONTROLES)
    {
        const float mitad = (z.width - hueco) / 2.0f;
        Rectangle teclado = { z.x, z.y + 6.0f * s, mitad, z.height - 6.0f * s };
        Rectangle mando = { z.x + mitad + hueco, teclado.y, mitad, teclado.height };

        DibujarTarjeta(teclado, "TECLADO", s, alfa);
        DibujarTarjeta(mando, "MANDO (GAMEPAD)", s, alfa);

        float y = teclado.y + 40.0f * s;

        if (config.modoTeclado == TECLADO_COMPLETO)
        {
            y = DibujarLinea("Jugador 1", "Teclado completo", teclado, y, s, alfa);
            y = DibujarLinea("Mover", "WASD o Flechas", teclado, y, s, alfa);
            y = DibujarLinea("Saltar / Aceptar", "ESPACIO", teclado, y, s, alfa);
            y = DibujarLinea("Accion", "E", teclado, y, s, alfa);
            y = DibujarLinea("Cancelar", "RETROCESO", teclado, y, s, alfa);
            y = DibujarLinea("Pausa / Salir", "ESC", teclado, y, s, alfa);
        }
        else
        {
            y = DibujarLinea("J1 Mover", "WASD", teclado, y, s, alfa);
            y = DibujarLinea("J1 Saltar / Accion", "ESPACIO / E", teclado, y, s, alfa);
            y = DibujarLinea("J2 Mover", "Flechas", teclado, y, s, alfa);
            y = DibujarLinea("J2 Saltar / Accion", "ENTER / SHIFT DER.", teclado, y, s, alfa);
            y = DibujarLinea("Cancelar", "J1 RETROCESO, J2 SHIFT DER.", teclado, y, s, alfa);
            y = DibujarLinea("Pausa / Salir", "ESC", teclado, y, s, alfa);
        }

        int conectados = 0;

        for (int g = 0; g < 4; g++)
        {
            if (IsGamepadAvailable(g))
            {
                conectados++;
            }
        }

        // Columna izquierda: botones. Columna derecha: vista previa del stick.
        Rectangle columnaLineas = mando;
        columnaLineas.width = mando.width * 0.62f;

        float ym = mando.y + 40.0f * s;

        ym = DibujarLinea("Mover", "Stick o cruceta", columnaLineas, ym, s, alfa);
        ym = DibujarLinea("Saltar, aceptar", "A", columnaLineas, ym, s, alfa);
        ym = DibujarLinea("Accion", "X", columnaLineas, ym, s, alfa);
        ym = DibujarLinea("Volver", "B", columnaLineas, ym, s, alfa);
        ym = DibujarLinea("Pausa", "START", columnaLineas, ym, s, alfa);
        DibujarLinea("Conectados", TextFormat("%d", conectados), columnaLineas, ym, s, alfa);

        // Vista previa del stick: circulo exterior = inclinacion maxima,
        // disco interior = zona muerta. El punto (stick sin filtrar) se
        // pinta con el acento cuando sale de la zona: ese es el momento en
        // que el juego empieza a recibir movimiento.
        int gamepad = -1;

        for (int g = 0; g < 4 && gamepad < 0; g++)
        {
            if (IsGamepadAvailable(g))
            {
                gamepad = g;
            }
        }

        const int fuentePrevia = TamanoFuente(14.0f, s);
        const Rectangle previa = {
            mando.x + columnaLineas.width,
            mando.y + 36.0f * s,
            mando.width - columnaLineas.width - 12.0f * s,
            mando.height - 36.0f * s - 8.0f * s
        };

        float radio = std::fmin(
            previa.width / 2.0f - 8.0f * s,
            (previa.height - fuentePrevia * 2.0f - 14.0f * s) / 2.0f
        );

        if (radio > 16.0f * s)
        {
            const float zona = ObtenerZonaMuertaStick();
            const Vector2 centro = {
                previa.x + previa.width / 2.0f,
                previa.y + radio + 4.0f * s
            };

            DrawCircleV(centro, radio, Fade(COLOR_BARRA, alfa));
            DrawCircleLinesV(centro, radio, Fade(GRAY, alfa));
            DrawLineV(Vector2{ centro.x - radio, centro.y }, Vector2{ centro.x + radio, centro.y }, Fade(GRAY, 0.35f * alfa));
            DrawLineV(Vector2{ centro.x, centro.y - radio }, Vector2{ centro.x, centro.y + radio }, Fade(GRAY, 0.35f * alfa));
            DrawCircleV(centro, radio * zona, Fade(COLOR_ACENTO, 0.22f * alfa));
            DrawCircleLinesV(centro, radio * zona, Fade(COLOR_ACENTO, 0.9f * alfa));

            Vector2 crudo = LeerStickIzquierdoCrudo(gamepad);
            Vector2 filtrado = LeerStickIzquierdo(gamepad);
            bool activo = filtrado.x != 0.0f || filtrado.y != 0.0f;

            DrawCircleV(
                Vector2{ centro.x + crudo.x * radio, centro.y + crudo.y * radio },
                5.0f * s,
                Fade(activo ? COLOR_ACENTO : RAYWHITE, alfa)
            );

            const char* estadoStick =
                gamepad < 0
                    ? "Conecta un mando"
                    : (activo ? "Moviendo" : "En zona muerta");

            DibujarTextoAjustado(
                gamepad < 0 ? "SIN MANDO" : TextFormat("MANDO %d", gamepad + 1),
                centro.x,
                centro.y + radio + 6.0f * s,
                fuentePrevia,
                previa.width,
                Fade(gamepad < 0 ? DARKGRAY : LIGHTGRAY, alfa),
                true
            );

            DibujarTextoAjustado(
                estadoStick,
                centro.x,
                centro.y + radio + 8.0f * s + fuentePrevia,
                fuentePrevia,
                previa.width,
                Fade(activo ? COLOR_ACENTO : LIGHTGRAY, alfa),
                true
            );
        }

        return;
    }

    if (categoria == CATEGORIA_JUEGO)
    {
        Rectangle tarjeta = { z.x, z.y + 6.0f * s, z.width, z.height - 6.0f * s };

        DibujarTarjeta(tarjeta, "RESUMEN ACTUAL", s, alfa);

        float y = tarjeta.y + 44.0f * s;

        y = DibujarLinea("Pantalla", NombreModoVentana(config.modoVentana), tarjeta, y, s, alfa);
        y = DibujarLinea("Resolucion", TextoResolucion(config, resoluciones), tarjeta, y, s, alfa);
        y = DibujarLinea("FPS", TextFormat("%d%s", opcionesFPS[config.indiceFPS], config.vsync ? " (VSync)" : ""), tarjeta, y, s, alfa);
        y = DibujarLinea("Musica / Efectos", TextFormat("%d%% / %d%%", (int)std::lround(config.volumenMusica * 100.0f), (int)std::lround(config.volumenSonidos * 100.0f)), tarjeta, y, s, alfa);
        y = DibujarLinea("Teclado", NombreModoTeclado(config.modoTeclado), tarjeta, y, s, alfa);
        DibujarLinea("Guardado", "Automatico al aplicar cada cambio", tarjeta, y, s, alfa);

        return;
    }
}


//==================================================
// DIBUJAR
//==================================================

void MenuConfiguracion::Dibujar(
    const ConfiguracionJuego& config,
    Resolucion resoluciones[],
    int opcionesFPS[]
)
{
    const float alfa = alphaGeneral;
    const float alfaContenidoActual = alfa * alphaContenido;

    const DisposicionConfig d = CalcularDisposicion((1.0f - alfa) * 18.0f);
    const float s = d.escala;

    // Velo sobre el HUB 3D que ya se dibujo de fondo.
    DrawRectangle(
        0, 0, GetScreenWidth(), GetScreenHeight(),
        Fade(COLOR_VELO, 0.72f * alfa)
    );

    DrawRectangleRec(d.panel, Fade(COLOR_PANEL, alfa));
    DrawRectangleLinesEx(d.panel, 2.0f, Fade(GRAY, 0.6f * alfa));
    DrawRectangle(
        (int)d.panel.x, (int)d.panel.y,
        (int)d.panel.width, (int)(6.0f * s),
        Fade(COLOR_ACENTO, alfa)
    );

    DrawText(
        "CONFIGURACION",
        (int)(d.panel.x + 28.0f * s),
        (int)(d.panel.y + 22.0f * s),
        TamanoFuente(38.0f, s),
        Fade(RAYWHITE, alfa)
    );

    //------------------------------
    // PESTANAS
    //------------------------------

    const int fuenteHint = TamanoFuente(18.0f, s);

    Rectangle botones[2] = { d.botonLB, d.botonRB };
    const char* textosBoton[2] = { "LB", "RB" };

    for (int i = 0; i < 2; i++)
    {
        DrawRectangleRec(botones[i], Fade(COLOR_FILA, alfa));
        DrawRectangleLinesEx(botones[i], 1.5f, Fade(GRAY, 0.7f * alfa));
        DibujarTextoAjustado(
            textosBoton[i],
            botones[i].x + botones[i].width / 2.0f,
            botones[i].y + botones[i].height / 2.0f - fuenteHint / 2.0f,
            fuenteHint,
            botones[i].width - 6.0f,
            Fade(LIGHTGRAY, alfa),
            true
        );
    }

    for (int i = 0; i < CANTIDAD_CATEGORIAS_CONFIGURACION; i++)
    {
        const bool activa = (cambiandoCategoria ? categoriaDestino : categoriaActual) == i;
        const bool hover = categoriaHover == i;
        Rectangle r = d.pestanas[i];

        DrawRectangleRec(r, Fade(activa ? COLOR_ACENTO : COLOR_FILA, (activa ? 0.95f : 0.85f) * alfa));
        DrawRectangleLinesEx(
            r,
            activa || hover ? 2.5f : 1.5f,
            Fade(activa ? RAYWHITE : (hover ? COLOR_ACENTO : GRAY), alfa)
        );

        const int fuente = TamanoFuente(20.0f, s);

        DibujarTextoAjustado(
            NOMBRES_CATEGORIAS[i],
            r.x + r.width / 2.0f,
            r.y + r.height / 2.0f - fuente / 2.0f,
            fuente,
            r.width - 12.0f * s,
            Fade(activa ? BLACK : RAYWHITE, alfa),
            true
        );
    }

    //------------------------------
    // FILAS Y EXTRA
    //------------------------------

    const int cantidad = CantidadOpciones(categoriaActual);

    for (int i = 0; i < cantidad; i++)
    {
        const int opcion = TABLA_OPCIONES[categoriaActual][i];
        const bool bloqueada =
            opcion == OPCION_VIDEO_RESOLUCION &&
            config.modoVentana == MODO_SIN_BORDES;

        DibujarFila(
            d,
            d.filas[i],
            opcion,
            opcionSeleccionada[categoriaActual] == i,
            bloqueada,
            config,
            resoluciones,
            opcionesFPS,
            *this,
            alfaContenidoActual
        );
    }

    DibujarContenidoExtra(d, config, resoluciones, opcionesFPS, categoriaActual, alfaContenidoActual);

    //------------------------------
    // INFO / AVISO
    //------------------------------

    DrawRectangleRec(d.info, Fade(COLOR_FILA, 0.7f * alfa));

    const char* textoInfo = "";

    if (tiempoAviso > 0.0f)
    {
        textoInfo = aviso;
    }
    else if (restaurarArmado)
    {
        textoInfo = "Restaura TODO, incluida la pantalla (modo y resolucion). Pulsa otra vez para confirmar.";
    }
    else if (cantidad > 0)
    {
        textoInfo = DescripcionOpcion(TABLA_OPCIONES[categoriaActual][opcionSeleccionada[categoriaActual]]);
    }

    DibujarTextoAjustado(
        textoInfo,
        d.info.x + 14.0f * s,
        d.info.y + d.info.height / 2.0f - TamanoFuente(18.0f, s) / 2.0f,
        TamanoFuente(18.0f, s),
        d.info.width - 28.0f * s,
        Fade(tiempoAviso > 0.0f ? COLOR_ACENTO : LIGHTGRAY, alfa),
        false
    );

    //------------------------------
    // AYUDA Y BOTON VOLVER
    //------------------------------

    const int fuenteAyuda = TamanoFuente(15.0f, s);

    DibujarTextoAjustado(
        "TECLADO: Flechas/WASD elegir y cambiar | Q/E categoria | ENTER aceptar | ESC volver",
        d.ayuda.x,
        d.ayuda.y + 4.0f * s,
        fuenteAyuda,
        d.ayuda.width,
        Fade(LIGHTGRAY, alfa),
        false
    );

    DibujarTextoAjustado(
        "MANDO: Cruceta/stick elegir y cambiar | LB/RB categoria | A aceptar | B volver",
        d.ayuda.x,
        d.ayuda.y + 6.0f * s + fuenteAyuda + 4.0f * s,
        fuenteAyuda,
        d.ayuda.width,
        Fade(GRAY, alfa),
        false
    );

    DibujarTextoAjustado(
        "Los cambios se guardan automaticamente.",
        d.ayuda.x,
        d.ayuda.y + 8.0f * s + (fuenteAyuda + 4.0f * s) * 2.0f,
        fuenteAyuda,
        d.ayuda.width,
        Fade(Color{ 110, 160, 120, 255 }, alfa),
        false
    );

    {
        const bool hover = CheckCollisionPointRec(GetMousePosition(), d.botonVolver);
        const int fuente = TamanoFuente(22.0f, s);

        DrawRectangleRec(d.botonVolver, Fade(hover ? COLOR_ACENTO : COLOR_FILA, alfa));
        DrawRectangleLinesEx(d.botonVolver, 2.0f, Fade(hover ? RAYWHITE : GRAY, alfa));
        DibujarTextoAjustado(
            "VOLVER",
            d.botonVolver.x + d.botonVolver.width / 2.0f,
            d.botonVolver.y + d.botonVolver.height / 2.0f - fuente / 2.0f,
            fuente,
            d.botonVolver.width - 12.0f * s,
            Fade(hover ? BLACK : RAYWHITE, alfa),
            true
        );
    }

    //------------------------------
    // MODAL DE CONFIRMACION
    //------------------------------

    if (confirmandoVideo)
    {
        DrawRectangle(
            0, 0, GetScreenWidth(), GetScreenHeight(),
            Fade(COLOR_VELO, 0.6f)
        );

        DrawRectangleRec(d.modal, COLOR_PANEL);
        DrawRectangleLinesEx(d.modal, 3.0f, COLOR_ACENTO);

        DibujarTextoAjustado(
            "Mantener esta configuracion de video?",
            d.modal.x + d.modal.width / 2.0f,
            d.modal.y + 28.0f * s,
            TamanoFuente(28.0f, s),
            d.modal.width - 40.0f * s,
            RAYWHITE,
            true
        );

        DibujarTextoAjustado(
            TextFormat("Se revertira automaticamente en %d s", (int)std::ceil(tiempoConfirmacion)),
            d.modal.x + d.modal.width / 2.0f,
            d.modal.y + 84.0f * s,
            TamanoFuente(20.0f, s),
            d.modal.width - 40.0f * s,
            LIGHTGRAY,
            true
        );

        Rectangle barra =
        {
            d.modal.x + 30.0f * s,
            d.modal.y + 124.0f * s,
            d.modal.width - 60.0f * s,
            14.0f * s
        };

        DrawRectangleRec(barra, COLOR_BARRA);
        DrawRectangle(
            (int)barra.x + 2,
            (int)barra.y + 2,
            (int)((barra.width - 4.0f) * LimitarFloat(tiempoConfirmacion / DURACION_CONFIRMACION, 0.0f, 1.0f)),
            (int)barra.height - 4,
            COLOR_ACENTO
        );

        const Vector2 mouse = GetMousePosition();
        const bool hoverMantener = CheckCollisionPointRec(mouse, d.modalMantener);
        const bool hoverRevertir = CheckCollisionPointRec(mouse, d.modalRevertir);
        const int fuente = TamanoFuente(20.0f, s);

        DrawRectangleRec(d.modalMantener, hoverMantener ? COLOR_ACENTO : Color{ 60, 150, 80, 255 });
        DrawRectangleLinesEx(d.modalMantener, 2.0f, RAYWHITE);
        DibujarTextoAjustado(
            "MANTENER (ENTER / A)",
            d.modalMantener.x + d.modalMantener.width / 2.0f,
            d.modalMantener.y + d.modalMantener.height / 2.0f - fuente / 2.0f,
            fuente,
            d.modalMantener.width - 12.0f * s,
            BLACK,
            true
        );

        DrawRectangleRec(d.modalRevertir, hoverRevertir ? COLOR_ACENTO : COLOR_FILA);
        DrawRectangleLinesEx(d.modalRevertir, 2.0f, GRAY);
        DibujarTextoAjustado(
            "REVERTIR (ESC / B)",
            d.modalRevertir.x + d.modalRevertir.width / 2.0f,
            d.modalRevertir.y + d.modalRevertir.height / 2.0f - fuente / 2.0f,
            fuente,
            d.modalRevertir.width - 12.0f * s,
            hoverRevertir ? BLACK : RAYWHITE,
            true
        );
    }
}
