#include "UI/MenuConfiguracion.h"
#include "UI/EstiloUIGlobal.h"
#include "Systems/Input.h"

#include <cmath>
#include <cstdio>

#include "Minigames/EfectosVisualesMinijuegos.h"


//==================================================
// COLORES
//==================================================

// Velo sobre el HUB 3D; el resto de colores son tokens de UI/EstiloUI.h.
static const Color COLOR_VELO = { 10, 12, 18, 255 };


//==================================================
// TABLAS DE OPCIONES
//==================================================

static const int MAX_FILAS = 5;

static const char* NOMBRES_CATEGORIAS[CANTIDAD_CATEGORIAS_CONFIGURACION] =
{
    "VIDEO",
    "GRÁFICOS",
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
        case OPCION_VIDEO_RESOLUCION: return "Resolución";
        case OPCION_VIDEO_FPS: return "Límite de FPS";
        case OPCION_VIDEO_MOSTRAR_FPS: return "Mostrar FPS";
        case OPCION_VIDEO_VSYNC: return "VSync";
        case OPCION_GRAFICO_PRESET: return "Preset gráfico";
        case OPCION_GRAFICO_PARTICULAS: return "Partículas";
        case OPCION_GRAFICO_DECORACION: return "Decoración";
        case OPCION_GRAFICO_EFECTOS: return "Efectos";
        case OPCION_GRAFICO_SOMBRAS: return "Sombras";
        case OPCION_AUDIO_SONIDOS: return "Efectos de sonido";
        case OPCION_AUDIO_MUSICA: return "Música";
        case OPCION_CONTROL_MODO_TECLADO: return "Modo de teclado";
        case OPCION_CONTROL_ZONA_MUERTA: return "Zona muerta del stick";
        case OPCION_JUEGO_RESTAURAR: return "Restaurar predeterminados";
        case OPCION_ACCESIBILIDAD_MOVIMIENTO: return "Reducir movimiento de cámara";
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
            return "Ventana, pantalla completa o ventana sin bordes. Pide confirmación al cambiar.";

        case OPCION_VIDEO_RESOLUCION:
            return "Tamaño de la imagen. En ventana sin bordes se usa la del monitor (bloqueada).";

        case OPCION_VIDEO_FPS:
            return "Tope de cuadros por segundo. Con VSync activo el monitor también limita.";

        case OPCION_VIDEO_MOSTRAR_FPS:
            return "Muestra un contador de FPS en la esquina durante todo el juego.";

        case OPCION_VIDEO_VSYNC:
            return "Sincroniza con el monitor para evitar cortes en la imagen.";

        case OPCION_GRAFICO_PRESET:
            return "Fija las cuatro opciones a la vez. Cambiar una individual pasa a PERSONALIZADO.";

        case OPCION_GRAFICO_PARTICULAS:
            return "Menos partículas ambientales (pétalos, brasas, humo, chispas) en tableros y minijuegos.";

        case OPCION_GRAFICO_DECORACION:
            return "Menos props decorativos y elementos lejanos fuera del área jugable.";

        case OPCION_GRAFICO_EFECTOS:
            return "Menos efectos secundarios: agua animada, destellos y haces de luz.";

        case OPCION_GRAFICO_SOMBRAS:
            return "Menos manchas de sombra simulada bajo personajes y objetos.";

        case OPCION_AUDIO_SONIDOS:
            return "Volumen de los efectos. Al cambiarlo suena una muestra.";

        case OPCION_AUDIO_MUSICA:
            return "Volumen de la música de fondo. 0% la silencia.";

        case OPCION_CONTROL_MODO_TECLADO:
            return "1 jugador: un teclado completo. 2 jugadores: WASD (J1) y flechas (J2).";

        case OPCION_CONTROL_ZONA_MUERTA:
            return "Inclinación del stick que se ignora (todos los mandos). Subila si el personaje se mueve solo.";

        case OPCION_ACCESIBILIDAD_MOVIMIENTO:
            return "Quita los temblores de cámara en minijuegos y el balanceo de los carteles del menú.";

        case OPCION_JUEGO_RESTAURAR:
            return "Devuelve video, gráficos, audio y controles a sus valores originales (pide confirmar).";
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
    ObtenerEstiloUI().reducirMovimiento = config.reducirMovimiento;
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
                menu.aviso = "La ventana sin bordes usa la resolución del monitor.";
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
            ObtenerEstiloUI().reducirMovimiento = config.reducirMovimiento;
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
// DISPOSICION (unidades de referencia 1280x720)
//==================================================
//
// Todo se ubica en unidades de referencia y se pasa a pixeles con
// RectRef (UI/EstiloUI): asi escala igual que la demo del estilo.

static const Rectangle REF_TITULO = { 450.0f, 22.0f, 380.0f, 60.0f };
static const Rectangle REF_PANEL = { 110.0f, 100.0f, 1060.0f, 560.0f };

// Pestanas: ancho = texto (fuente UI_TEXTO a escala 1) + 32 de marco.
static const float ANCHO_PESTANA[CANTIDAD_CATEGORIAS_CONFIGURACION] =
{
    96.0f, 136.0f, 94.0f, 156.0f, 98.0f, 192.0f
};

static const float REPOSO_PESTANA[CANTIDAD_CATEGORIAS_CONFIGURACION] =
{
    1.2f, -1.0f, 1.4f, -1.2f, 1.0f, -1.4f
};

static const float Y_PESTANAS = 144.0f;
static const float ALTO_PESTANA = 46.0f;
static const float ANCHO_BOTON_LB = 56.0f;
static const float HUECO_PESTANAS = 12.0f;

static const float X_FILAS = 160.0f;
static const float ANCHO_FILAS = 960.0f;
static const float Y_FILAS = 214.0f;
static const float PASO_FILAS = 58.0f;
static const float Y_FIN_CONTENIDO = 494.0f;
static const float Y_INFO = 508.0f;

static const Rectangle REF_RESTAURAR = { 430.0f, 240.0f, 420.0f, 52.0f };
static const Rectangle REF_VOLVER = { 530.0f, 572.0f, 220.0f, UI_ALTO_BOTON };

static const Rectangle REF_MODAL = { 300.0f, 220.0f, 680.0f, 280.0f };
static const Rectangle REF_MANTENER = { 335.0f, 386.0f, 290.0f, UI_ALTO_BOTON };
static const Rectangle REF_REVERTIR = { 655.0f, 386.0f, 290.0f, UI_ALTO_BOTON };


// x inicial de la tira LB + pestanas + RB, centrada en 1280.
static float InicioTiraPestanas()
{
    float total = ANCHO_BOTON_LB * 2.0f + HUECO_PESTANAS * 2.0f;

    for (int i = 0; i < CANTIDAD_CATEGORIAS_CONFIGURACION; i++)
    {
        total += ANCHO_PESTANA[i];
    }

    total += HUECO_PESTANAS * (CANTIDAD_CATEGORIAS_CONFIGURACION - 1);

    return (UI_REF_ANCHO - total) * 0.5f;
}


static Rectangle RefPestana(
    int categoria
)
{
    float x = InicioTiraPestanas() + ANCHO_BOTON_LB + HUECO_PESTANAS;

    for (int i = 0; i < categoria; i++)
    {
        x += ANCHO_PESTANA[i] + HUECO_PESTANAS;
    }

    return Rectangle{ x, Y_PESTANAS, ANCHO_PESTANA[categoria], ALTO_PESTANA };
}


static Rectangle RefBotonAnterior()
{
    return Rectangle{ InicioTiraPestanas(), Y_PESTANAS, ANCHO_BOTON_LB, ALTO_PESTANA };
}


static Rectangle RefBotonSiguiente()
{
    Rectangle ultima = RefPestana(CANTIDAD_CATEGORIAS_CONFIGURACION - 1);

    return Rectangle{
        ultima.x + ultima.width + HUECO_PESTANAS,
        Y_PESTANAS,
        ANCHO_BOTON_LB,
        ALTO_PESTANA
    };
}


static Rectangle RefFila(
    int fila
)
{
    return Rectangle{ X_FILAS, Y_FILAS + fila * PASO_FILAS, ANCHO_FILAS, UI_ALTO_FILA };
}


// Zona de las filas o del boton de la opcion (en referencia).
static Rectangle RefControlOpcion(
    int categoria,
    int fila
)
{
    if (EsAccion(TABLA_OPCIONES[categoria][fila]))
    {
        return REF_RESTAURAR;
    }

    return RefFila(fila);
}


static Rectangle EnPantalla(
    Rectangle ref
)
{
    return RectRef(ref.x, ref.y, ref.width, ref.height);
}


// Rectangulo en pixeles de un elemento con su desplazamiento de caida.
static Rectangle EnPantallaColgado(
    Rectangle ref,
    const ColgadoUI& colgado
)
{
    Rectangle r = EnPantalla(ref);
    r.y += colgado.caida.valor * EscalaUI();

    return r;
}


// Riel de una palanca en pixeles (mismas proporciones que DibujarPalancaUI).
static void RielPalanca(
    Rectangle fila,
    float& x0,
    float& x1
)
{
    x0 = fila.x + fila.width * 0.42f;
    x1 = fila.x + fila.width - Px(84.0f);
}


//==================================================
// TEXTO DEL SELECTOR
//==================================================

static const char* TextoValorSelector(
    const ConfiguracionJuego& config,
    int opcion,
    Resolucion resoluciones[],
    int opcionesFPS[]
)
{
    switch (opcion)
    {
        case OPCION_VIDEO_MODO:
            return NombreModoVentana(config.modoVentana);

        case OPCION_VIDEO_RESOLUCION:
            return TextoResolucion(config, resoluciones);

        case OPCION_VIDEO_FPS:
            return TextFormat("%d", opcionesFPS[config.indiceFPS]);

        case OPCION_GRAFICO_PRESET:
            return NombrePresetGrafico(config.presetGrafico);

        case OPCION_GRAFICO_PARTICULAS:
            return NombreNivelCalidad(config.calidadParticulas);

        case OPCION_GRAFICO_DECORACION:
            return NombreNivelCalidad(config.calidadDecoracion);

        case OPCION_GRAFICO_EFECTOS:
            return NombreNivelCalidad(config.calidadEfectos);

        case OPCION_GRAFICO_SOMBRAS:
            return NombreNivelCalidad(config.calidadSombras);

        case OPCION_CONTROL_MODO_TECLADO:
            return NombreModoTeclado(config.modoTeclado);
    }

    return "";
}


static bool ValorInterruptor(
    const ConfiguracionJuego& config,
    int opcion
)
{
    if (opcion == OPCION_VIDEO_MOSTRAR_FPS)
    {
        return config.mostrarFPS;
    }

    if (opcion == OPCION_VIDEO_VSYNC)
    {
        return config.vsync;
    }

    return config.reducirMovimiento;
}


//==================================================
// COMPONENTES: INICIO Y ANIMACION
//==================================================

// Al cambiar de pestana las filas caen enseguida y desde poco: con esta
// distancia la primera fila nunca sube hasta la tira de pestanas.
static const float PASO_FILAS_PESTANA = 0.035f;
static const float CAIDA_FILAS_PESTANA = 14.0f;


// Acorta la caida de una fila recien iniciada (unidades de referencia).
static void AcortarCaida(
    ColgadoUI& colgado,
    float distancia
)
{
    colgado.distanciaCaida = distancia;
    colgado.caida.valor = -distancia;
}


// Crea las filas de la categoria actual; caen escalonadas (paso) desde
// retrasoBase, cada una desde distanciaCaida unidades mas arriba. Los
// valores se copian de config.
static void IniciarFilasCategoria(
    MenuConfiguracion& menu,
    const ConfiguracionJuego& config,
    float retrasoBase,
    float paso = UI_T_ESCALONADO,
    float distanciaCaida = 40.0f
)
{
    const int cantidad = CantidadOpciones(menu.categoriaActual);

    for (int fila = 0; fila < cantidad; fila++)
    {
        const int opcion = TABLA_OPCIONES[menu.categoriaActual][fila];
        const float retraso = retrasoBase + fila * paso;
        const Rectangle ref = RefFila(fila);

        if (EsSlider(opcion))
        {
            menu.palancas[opcion].Iniciar(ref, EtiquetaOpcion(opcion), ValorSlider(config, opcion), retraso);
            AcortarCaida(menu.palancas[opcion].colgado, distanciaCaida);
        }
        else if (EsInterruptor(opcion))
        {
            menu.interruptores[opcion].Iniciar(ref, EtiquetaOpcion(opcion), ValorInterruptor(config, opcion), retraso);
            AcortarCaida(menu.interruptores[opcion].colgado, distanciaCaida);
        }
        else if (EsAccion(opcion))
        {
            menu.botonRestaurar.Iniciar(REF_RESTAURAR, "RESTAURAR TODO", -1.0f, retraso);
        }
        else
        {
            menu.selectores[opcion].Iniciar(ref, EtiquetaOpcion(opcion), retraso);
            AcortarCaida(menu.selectores[opcion].colgado, distanciaCaida);
        }
    }

    menu.colgadoExtra.Iniciar(0.0f, retrasoBase + cantidad * paso, distanciaCaida, 0.0f);
}


// true mientras alguna pestana aun cae o se balancea mucho.
static bool PestanasAgitadas(
    const MenuConfiguracion& menu
)
{
    for (int i = 0; i < CANTIDAD_CATEGORIAS_CONFIGURACION; i++)
    {
        const ColgadoUI& c = menu.pestanas[i].colgado;

        if (
            c.retraso > 0.0f ||
            !c.aterrizo ||
            std::fabs(c.angulo - c.reposo) > 2.5f ||
            std::fabs(c.velocidad) > 15.0f
        )
        {
            return true;
        }
    }

    return false;
}


static void IniciarModal(
    MenuConfiguracion& menu
)
{
    menu.panelModal.Iniciar(REF_MODAL, 0.0f);
    menu.botonMantener.Iniciar(REF_MANTENER, "MANTENER (ENTER / A)", -1.0f, 0.30f);
    menu.botonRevertir.Iniciar(REF_REVERTIR, "REVERTIR (ESC / B)", 1.0f, 0.37f);
    menu.modalIniciado = true;
    menu.focoModal = 0;
}


static void ActualizarAnimaciones(
    MenuConfiguracion& menu,
    const ConfiguracionJuego& config,
    float deltaTime
)
{
    const EstiloUI& estilo = ObtenerEstiloUI();
    const int cantidad = CantidadOpciones(menu.categoriaActual);
    const int seleccion = menu.opcionSeleccionada[menu.categoriaActual];

    ActualizarPanelUI(menu.panel, estilo, deltaTime);
    menu.colgadoTitulo.Actualizar(estilo, deltaTime, false);

    // La pestana activa recibe el "foco": se balancea al activarse.
    for (int i = 0; i < CANTIDAD_CATEGORIAS_CONFIGURACION; i++)
    {
        menu.pestanas[i].activo = i == menu.categoriaActual;
        ActualizarBotonUI(menu.pestanas[i], estilo, deltaTime, i == menu.categoriaActual);
    }

    ActualizarBotonUI(menu.botonAnterior, estilo, deltaTime, false);
    ActualizarBotonUI(menu.botonSiguiente, estilo, deltaTime, false);

    if (!menu.filasListas && !PestanasAgitadas(menu))
    {
        IniciarFilasCategoria(menu, config, 0.0f);
        menu.filasListas = true;
    }

    // Los componentes siguen a config (restaurar, arrastre, otra pestana).
    for (int i = 0; i < cantidad; i++)
    {
        const int opcion = TABLA_OPCIONES[menu.categoriaActual][i];

        if (EsSlider(opcion))
        {
            menu.palancas[opcion].valor = ValorSlider(config, opcion);
            ActualizarPalancaUI(menu.palancas[opcion], estilo, deltaTime);
        }
        else if (EsInterruptor(opcion))
        {
            menu.interruptores[opcion].activo = ValorInterruptor(config, opcion);
            ActualizarInterruptorUI(menu.interruptores[opcion], estilo, deltaTime);
        }
        else if (EsAccion(opcion))
        {
            menu.botonRestaurar.texto =
                menu.restaurarArmado
                    ? "PULSÁ OTRA VEZ PARA CONFIRMAR"
                    : "RESTAURAR TODO";

            ActualizarBotonUI(
                menu.botonRestaurar,
                estilo,
                deltaTime,
                menu.filasListas && seleccion == i && !menu.confirmandoVideo
            );
        }
        else
        {
            ActualizarSelectorUI(menu.selectores[opcion], estilo, deltaTime);
        }
    }

    menu.colgadoExtra.Actualizar(estilo, deltaTime, false);

    ActualizarBotonUI(
        menu.botonVolver,
        estilo,
        deltaTime,
        seleccion == cantidad && !menu.confirmandoVideo
    );

    // Cartel de confirmacion de video.
    if (menu.confirmandoVideo && !menu.modalIniciado)
    {
        IniciarModal(menu);
    }
    else if (!menu.confirmandoVideo)
    {
        menu.modalIniciado = false;
    }

    if (menu.modalIniciado)
    {
        ActualizarPanelUI(menu.panelModal, estilo, deltaTime);
        ActualizarBotonUI(menu.botonMantener, estilo, deltaTime, menu.focoModal == 0);
        ActualizarBotonUI(menu.botonRevertir, estilo, deltaTime, menu.focoModal == 1);
    }
}


// Salida: todos los carteles suben juntos (se "recogen"). desplazamiento
// en unidades de referencia (negativo = hacia arriba).
static void RecogerCarteles(
    MenuConfiguracion& menu,
    float desplazamiento
)
{
    ColgadoUI* colgados[] =
    {
        &menu.panel.colgado,
        &menu.colgadoTitulo,
        &menu.botonAnterior.colgado,
        &menu.botonSiguiente.colgado,
        &menu.botonVolver.colgado,
        &menu.botonRestaurar.colgado,
        &menu.colgadoExtra
    };

    for (ColgadoUI* c : colgados)
    {
        c->caida.valor = desplazamiento;
        c->caida.velocidad = 0.0f;
    }

    for (int i = 0; i < CANTIDAD_CATEGORIAS_CONFIGURACION; i++)
    {
        menu.pestanas[i].colgado.caida.valor = desplazamiento;
    }

    for (int i = 0; i < CANTIDAD_OPCIONES_CONFIGURACION; i++)
    {
        menu.selectores[i].colgado.caida.valor = desplazamiento;
        menu.palancas[i].colgado.caida.valor = desplazamiento;
        menu.interruptores[i].colgado.caida.valor = desplazamiento;
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

    saliendo = false;
    tiempoSalida = 0.0f;
    filasListas = false;

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
    modalIniciado = false;
    focoModal = 0;
    restaurarArmado = false;
    tiempoRestaurar = 0.0f;
    aviso = "";
    tiempoAviso = 0.0f;
    vsyncSincronizado = false;

    // Entrada escalonada: panel, titulo, pestanas y boton volver. Las
    // filas se crean cuando las pestanas se asientan.
    panel.Iniciar(REF_PANEL, 0.0f);
    colgadoTitulo.Iniciar(-1.0f, 0.30f, 120.0f, 1.0f);

    botonAnterior.Iniciar(RefBotonAnterior(), "LB", 1.5f, 0.30f);
    botonSiguiente.Iniciar(RefBotonSiguiente(), "RB", -1.5f, 0.30f + CANTIDAD_CATEGORIAS_CONFIGURACION * 0.05f);

    for (int i = 0; i < CANTIDAD_CATEGORIAS_CONFIGURACION; i++)
    {
        pestanas[i].Iniciar(
            RefPestana(i),
            NOMBRES_CATEGORIAS[i],
            REPOSO_PESTANA[i],
            0.35f + i * 0.05f
        );
        pestanas[i].activo = i == categoriaActual;
    }

    botonVolver.Iniciar(REF_VOLVER, "VOLVER", -1.2f, 0.70f);
}


//==================================================
// ACTUALIZAR
//==================================================

static void IrACategoria(
    MenuConfiguracion& menu,
    const ConfiguracionJuego& config,
    int categoria
)
{
    categoria = Envolver(categoria, CANTIDAD_CATEGORIAS_CONFIGURACION);

    if (categoria == menu.categoriaActual)
    {
        return;
    }

    menu.categoriaActual = categoria;
    menu.opcionArrastrada = OPCION_NINGUNA;
    menu.restaurarArmado = false;

    // Las filas nuevas caen ya, escalonadas y desde poco, sin esperar a
    // que la pestana deje de balancearse. Durante la entrada (filas aun
    // no listas) se crean cuando las pestanas se asientan, como antes.
    if (menu.filasListas)
    {
        IniciarFilasCategoria(menu, config, 0.0f, PASO_FILAS_PESTANA, CAIDA_FILAS_PESTANA);
    }
}


static void IniciarSalida(
    MenuConfiguracion& menu
)
{
    menu.saliendo = true;
    menu.tiempoSalida = 0.0f;
    menu.opcionArrastrada = OPCION_NINGUNA;
}


// Teclado, mando y mouse. Las animaciones se actualizan despues.
static void ProcesarEntrada(
    MenuConfiguracion& menu,
    ConfiguracionJuego& config,
    Resolucion resoluciones[],
    int cantidadResoluciones,
    int opcionesFPS[],
    int cantidadOpcionesFPS,
    AudioJuego& audio,
    float deltaTime
)
{
    const Vector2 mouse = GetMousePosition();
    const bool click = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    const bool mouseMovido =
        std::fabs(mouse.x - menu.mouseAnterior.x) +
        std::fabs(mouse.y - menu.mouseAnterior.y) > 0.5f;

    menu.mouseAnterior = mouse;

    EntradaConfig entrada = LeerEntrada(menu, deltaTime);

    if (menu.bloqueoEntrada > 0.0f)
    {
        entrada = EntradaConfig{};
    }

    //------------------------------
    // CONFIRMACION DE VIDEO
    //------------------------------

    if (menu.confirmandoVideo)
    {
        menu.tiempoConfirmacion -= deltaTime;

        // Foco entre MANTENER y REVERTIR: flechas/cruceta o mouse.
        int focoNuevo = menu.focoModal;

        if (entrada.horizontalPulso != 0)
        {
            focoNuevo = entrada.horizontalPulso < 0 ? 0 : 1;
        }
        else if (entrada.vertical != 0)
        {
            focoNuevo = 1 - menu.focoModal;
        }
        else if (mouseMovido && CheckCollisionPointRec(mouse, EnPantalla(REF_MANTENER)))
        {
            focoNuevo = 0;
        }
        else if (mouseMovido && CheckCollisionPointRec(mouse, EnPantalla(REF_REVERTIR)))
        {
            focoNuevo = 1;
        }

        if (focoNuevo != menu.focoModal)
        {
            menu.focoModal = focoNuevo;
            Sonido(audio, SONIDO_UI_MOVER);
        }

        // Aceptar ejecuta el boton con foco; ESC/B siempre revierte.
        bool mantener =
            (entrada.confirmar && menu.focoModal == 0) ||
            (click && CheckCollisionPointRec(mouse, EnPantalla(REF_MANTENER)));

        bool revertir =
            (entrada.confirmar && menu.focoModal == 1) ||
            entrada.cancelar ||
            menu.tiempoConfirmacion <= 0.0f ||
            (click && CheckCollisionPointRec(mouse, EnPantalla(REF_REVERTIR)));

        if (mantener)
        {
            menu.confirmandoVideo = false;
            menu.configuracionCambiada = true;
            Sonido(audio, SONIDO_UI_CONFIRMAR);
        }
        else if (revertir)
        {
            RevertirVideo(menu, config, resoluciones);
            Sonido(audio, SONIDO_UI_CANCELAR);
        }

        return;
    }

    //------------------------------
    // SALIR
    //------------------------------

    if (
        entrada.cancelar ||
        (click && CheckCollisionPointRec(mouse, EnPantalla(REF_VOLVER)))
    )
    {
        menu.botonVolver.colgado.Pulsar();
        IniciarSalida(menu);
        return;
    }

    //------------------------------
    // CATEGORIAS
    //------------------------------

    menu.categoriaHover = -1;

    for (int i = 0; i < CANTIDAD_CATEGORIAS_CONFIGURACION; i++)
    {
        if (CheckCollisionPointRec(mouse, EnPantalla(RefPestana(i))))
        {
            menu.categoriaHover = i;
        }
    }

    if (entrada.categoria != 0)
    {
        IrACategoria(menu, config, menu.categoriaActual + entrada.categoria);
        Sonido(audio, SONIDO_UI_MOVER);
        return;
    }

    if (click)
    {
        if (CheckCollisionPointRec(mouse, EnPantalla(RefBotonAnterior())))
        {
            menu.botonAnterior.colgado.Pulsar();
            IrACategoria(menu, config, menu.categoriaActual - 1);
            Sonido(audio, SONIDO_UI_MOVER);
            return;
        }

        if (CheckCollisionPointRec(mouse, EnPantalla(RefBotonSiguiente())))
        {
            menu.botonSiguiente.colgado.Pulsar();
            IrACategoria(menu, config, menu.categoriaActual + 1);
            Sonido(audio, SONIDO_UI_MOVER);
            return;
        }

        if (menu.categoriaHover >= 0 && menu.categoriaHover != menu.categoriaActual)
        {
            IrACategoria(menu, config, menu.categoriaHover);
            Sonido(audio, SONIDO_UI_MOVER);
            return;
        }
    }

    //------------------------------
    // FOCO (opciones + VOLVER)
    //------------------------------

    const int categoria = menu.categoriaActual;
    const int cantidad = CantidadOpciones(categoria);
    int& seleccion = menu.opcionSeleccionada[categoria];

    if (entrada.vertical != 0)
    {
        seleccion = Envolver(seleccion + entrada.vertical, cantidad + 1);
        menu.restaurarArmado = false;
        Sonido(audio, SONIDO_UI_MOVER);
    }

    int filaHover = -1;

    if (menu.filasListas)
    {
        for (int i = 0; i < cantidad; i++)
        {
            if (CheckCollisionPointRec(mouse, EnPantalla(RefControlOpcion(categoria, i))))
            {
                filaHover = i;
            }
        }
    }

    if (CheckCollisionPointRec(mouse, EnPantalla(REF_VOLVER)))
    {
        filaHover = cantidad;
    }

    if (
        filaHover >= 0 &&
        filaHover != seleccion &&
        (mouseMovido || click) &&
        menu.opcionArrastrada == OPCION_NINGUNA
    )
    {
        seleccion = filaHover;
        menu.restaurarArmado = false;
    }

    if (seleccion == cantidad)
    {
        if (entrada.confirmar)
        {
            menu.botonVolver.colgado.Pulsar();
            IniciarSalida(menu);
        }

        return;
    }

    // Las opciones solo cambian cuando sus filas estan a la vista.
    if (!menu.filasListas)
    {
        return;
    }

    const int opcion = TABLA_OPCIONES[categoria][seleccion];

    //------------------------------
    // ARRASTRE DE PALANCAS (mouse)
    //------------------------------

    if (!IsMouseButtonDown(MOUSE_BUTTON_LEFT))
    {
        menu.opcionArrastrada = OPCION_NINGUNA;
    }

    if (click)
    {
        for (int i = 0; i < cantidad; i++)
        {
            int op = TABLA_OPCIONES[categoria][i];

            if (!EsSlider(op))
            {
                continue;
            }

            Rectangle fila = EnPantalla(RefFila(i));
            float x0, x1;
            RielPalanca(fila, x0, x1);

            Rectangle zona = { x0 - Px(12.0f), fila.y, x1 - x0 + Px(24.0f), fila.height };

            if (CheckCollisionPointRec(mouse, zona))
            {
                menu.opcionArrastrada = op;
                seleccion = i;
            }
        }
    }

    if (menu.opcionArrastrada != OPCION_NINGUNA)
    {
        int fila = 0;

        for (int i = 0; i < cantidad; i++)
        {
            if (TABLA_OPCIONES[categoria][i] == menu.opcionArrastrada)
            {
                fila = i;
            }
        }

        float x0, x1;
        RielPalanca(EnPantalla(RefFila(fila)), x0, x1);

        float valor = LimitarFloat((mouse.x - x0) / (x1 - x0), 0.0f, 1.0f);

        valor = std::round(valor * 100.0f) / 100.0f;

        if (menu.opcionArrastrada == OPCION_CONTROL_ZONA_MUERTA)
        {
            int pasos = (int)std::lround(
                valor *
                (ZONA_MUERTA_STICK_MAXIMA - ZONA_MUERTA_STICK_MINIMA) /
                ZONA_MUERTA_STICK_PASO
            );

            CambiarZonaMuerta(
                menu,
                config,
                audio,
                ZONA_MUERTA_STICK_MINIMA + pasos * ZONA_MUERTA_STICK_PASO
            );
        }
        else if (menu.opcionArrastrada == OPCION_AUDIO_SONIDOS)
        {
            CambiarVolumenSonidos(menu, config, audio, valor);
        }
        else
        {
            CambiarVolumenMusica(menu, config, audio, valor);
        }

        return;
    }

    //------------------------------
    // CAMBIAR EL VALOR
    //------------------------------

    // Teclado / mando: izquierda-derecha cambia el valor.
    int paso = EsSlider(opcion) ? entrada.horizontalRepetido : entrada.horizontalPulso;

    bool activar = entrada.confirmar;

    if (click && filaHover == seleccion)
    {
        Rectangle fila = EnPantalla(RefControlOpcion(categoria, seleccion));

        // Selector: click en la mitad izquierda del valor = anterior.
        float xIzq = fila.x + fila.width * 0.55f;
        float xDer = fila.x + fila.width - Px(28.0f);

        if (EsSlider(opcion))
        {
            // El click sobre el riel ya se resolvio arriba.
        }
        else if (
            !EsInterruptor(opcion) &&
            !EsAccion(opcion) &&
            mouse.x >= xIzq - Px(20.0f) &&
            mouse.x < (xIzq + xDer) * 0.5f
        )
        {
            paso = -1;
        }
        else
        {
            activar = true;
        }
    }

    // La accion (restaurar) solo responde a aceptar.
    if (EsAccion(opcion))
    {
        paso = 0;
    }

    if (paso == 0 && activar && !EsSlider(opcion))
    {
        paso = 1;
    }

    if (paso == 0)
    {
        return;
    }

    AplicarPaso(
        menu, config, opcion, paso,
        resoluciones, cantidadResoluciones,
        opcionesFPS, cantidadOpcionesFPS, audio
    );

    if (EsAccion(opcion))
    {
        menu.botonRestaurar.colgado.Pulsar();
    }
    else if (!EsSlider(opcion) && !EsInterruptor(opcion))
    {
        GolpearSelectorUI(menu.selectores[opcion], paso);
    }
}


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
    // SALIDA: los carteles se recogen
    //------------------------------

    if (saliendo)
    {
        tiempoSalida += deltaTime;

        float t = LimitarFloat(tiempoSalida / DURACION_TRANSICION, 0.0f, 1.0f);

        // Con movimiento reducido se sale sin animacion.
        if (ObtenerEstiloUI().reducirMovimiento)
        {
            t = 1.0f;
        }

        RecogerCarteles(*this, -t * t * UI_REF_ALTO);

        if (t >= 1.0f)
        {
            volver = true;
        }

        return;
    }

    if (bloqueoEntrada > 0.0f)
    {
        bloqueoEntrada -= deltaTime;
    }

    ProcesarEntrada(
        *this,
        config,
        resoluciones,
        cantidadResoluciones,
        opcionesFPS,
        cantidadOpcionesFPS,
        audio,
        deltaTime
    );

    if (!saliendo)
    {
        ActualizarAnimaciones(*this, config, deltaTime);
    }
}


//==================================================
// DIBUJO: PIEZAS
//==================================================

// La palanca de la zona muerta recorre 5..35 %: se tapa el porcentaje
// 0..100 que dibuja DibujarPalancaUI y se escribe el valor real. La placa
// es de color plano, asi que el parche no se nota.
static void DibujarValorPalanca(
    const PalancaUI& palanca,
    const char* texto
)
{
    if (palanca.colgado.retraso > 0.0f)
    {
        return;
    }

    Rectangle fila = EnPantallaColgado(palanca.ref, palanca.colgado);

    DrawRectangleRec(
        Rectangle{
            fila.x + fila.width - Px(72.0f),
            fila.y + Px(6.0f),
            Px(66.0f),
            fila.height - Px(12.0f)
        },
        UI_PLACA_CLARA
    );

    const int tamano = TamanoTexto(UI_TEXTO);
    const int ancho = MeasureText(texto, tamano);

    DrawText(
        texto,
        (int)(fila.x + fila.width - Px(UI_RELLENO + 4.0f) - ancho),
        (int)(fila.y + (fila.height - tamano) * 0.5f),
        tamano,
        UI_TINTA
    );
}


// Tarjeta clara (placa con sombra) con titulo en tinta.
static void DibujarTarjeta(
    const EstiloUI& estilo,
    Rectangle r,
    const char* titulo
)
{
    DrawRectangleRec(
        Rectangle{ r.x + Px(3.0f), r.y + Px(4.0f), r.width, r.height },
        Fade(BLACK, 0.18f)
    );

    DibujarNueveCortes(estilo.placa, 8, r, EscalaUI(), WHITE);

    DrawText(
        titulo,
        (int)(r.x + Px(14.0f)),
        (int)(r.y + Px(9.0f)),
        TamanoTexto(UI_TEXTO_PEQUENO),
        UI_TINTA
    );

    DrawRectangleRec(
        Rectangle{ r.x + Px(14.0f), r.y + Px(28.0f), r.width - Px(28.0f), Px(2.0f) },
        Fade(UI_MADERA_OSCURA, 0.45f)
    );
}


// Linea "etiqueta  valor" dentro de una tarjeta. Devuelve la y siguiente.
static float DibujarLinea(
    const char* etiqueta,
    const char* valor,
    Rectangle r,
    float y
)
{
    const int fuente = TamanoTexto(UI_TEXTO_PEQUENO);
    const float ancho = r.width - Px(28.0f);

    DibujarTextoAjustado(etiqueta, r.x + Px(14.0f), y, fuente, ancho * 0.42f, UI_TINTA_SUAVE, false);
    DibujarTextoAjustado(valor, r.x + Px(14.0f) + ancho * 0.45f, y, fuente, ancho * 0.55f, UI_TINTA, false);

    return y + fuente + Px(5.0f);
}


// Vista previa en vivo del stick: circulo = inclinacion maxima, disco
// sombreado = zona muerta, punto = stick sin filtrar del primer mando.
static void DibujarVistaStick(
    Rectangle zona
)
{
    int gamepad = -1;

    for (int g = 0; g < 4 && gamepad < 0; g++)
    {
        if (IsGamepadAvailable(g))
        {
            gamepad = g;
        }
    }

    const int fuente = TamanoTexto(UI_TEXTO_PEQUENO);
    const float radio = std::fmin(
        zona.width * 0.5f - Px(8.0f),
        (zona.height - fuente - Px(12.0f)) * 0.5f
    );

    if (radio < Px(16.0f))
    {
        return;
    }

    const Vector2 centro = { zona.x + zona.width * 0.5f, zona.y + radio + Px(2.0f) };
    const float zonaMuerta = ObtenerZonaMuertaStick();

    DrawCircleV(centro, radio, Color{ 214, 186, 136, 255 });
    DrawLineEx(Vector2{ centro.x - radio, centro.y }, Vector2{ centro.x + radio, centro.y }, Px(1.0f), Fade(UI_TINTA_SUAVE, 0.45f));
    DrawLineEx(Vector2{ centro.x, centro.y - radio }, Vector2{ centro.x, centro.y + radio }, Px(1.0f), Fade(UI_TINTA_SUAVE, 0.45f));
    DrawRing(centro, radio - Px(2.0f), radio, 0.0f, 360.0f, 48, UI_TINTA);

    // Zona muerta: el juego ignora toda inclinacion dentro de este disco.
    DrawCircleV(centro, radio * zonaMuerta, Fade(UI_MADERA_OSCURA, 0.45f));
    DrawRing(centro, radio * zonaMuerta - Px(1.5f), radio * zonaMuerta, 0.0f, 360.0f, 36, UI_MADERA_OSCURA);

    const Vector2 crudo = LeerStickIzquierdoCrudo(gamepad);
    const Vector2 filtrado = LeerStickIzquierdo(gamepad);
    const bool moviendo = filtrado.x != 0.0f || filtrado.y != 0.0f;
    const Vector2 punto = { centro.x + crudo.x * radio, centro.y + crudo.y * radio };

    if (moviendo)
    {
        DrawLineEx(centro, punto, Px(2.0f), UI_PELIGRO);
    }

    DrawCircleV(punto, Px(6.0f), moviendo ? UI_PELIGRO : UI_TINTA);
    DrawCircleLinesV(punto, Px(6.0f), UI_TINTA);

    const char* estado =
        gamepad < 0
            ? "SIN MANDO: en reposo"
            : TextFormat("MANDO %d: %s", gamepad + 1, moviendo ? "se mueve" : "zona muerta");

    DibujarTextoAjustado(estado, centro.x, centro.y + radio + Px(6.0f), fuente, zona.width, UI_TINTA, true);
}


static void DibujarContenidoExtra(
    const MenuConfiguracion& menu,
    const ConfiguracionJuego& config,
    Resolucion resoluciones[],
    int opcionesFPS[]
)
{
    const EstiloUI& estilo = ObtenerEstiloUI();
    const ColgadoUI& colgado = menu.colgadoExtra;
    const int categoria = menu.categoriaActual;

    if (colgado.retraso > 0.0f)
    {
        return;
    }

    if (categoria == CATEGORIA_CONTROLES)
    {
        // Bajo las dos filas: tarjeta de teclado y tarjeta de mando.
        const float yRef = Y_FILAS + CantidadOpciones(categoria) * PASO_FILAS;
        const float altoRef = Y_FIN_CONTENIDO - yRef;

        Rectangle teclado = EnPantallaColgado(Rectangle{ X_FILAS, yRef, 440.0f, altoRef }, colgado);
        Rectangle mando = EnPantallaColgado(Rectangle{ X_FILAS + 456.0f, yRef, ANCHO_FILAS - 456.0f, altoRef }, colgado);

        DibujarTarjeta(estilo, teclado, "TECLADO");
        DibujarTarjeta(estilo, mando, "MANDO (GAMEPAD)");

        float y = teclado.y + Px(36.0f);

        if (config.modoTeclado == TECLADO_COMPLETO)
        {
            y = DibujarLinea("Jugador 1", "Teclado completo", teclado, y);
            y = DibujarLinea("Mover", "WASD o Flechas", teclado, y);
            y = DibujarLinea("Saltar / Aceptar", "ESPACIO", teclado, y);
            y = DibujarLinea("Acción", "E", teclado, y);
            y = DibujarLinea("Cancelar", "RETROCESO", teclado, y);
            DibujarLinea("Pausa / Salir", "ESC", teclado, y);
        }
        else
        {
            y = DibujarLinea("J1 Mover", "WASD", teclado, y);
            y = DibujarLinea("J1 Saltar / Acción", "ESPACIO / E", teclado, y);
            y = DibujarLinea("J2 Mover", "Flechas", teclado, y);
            y = DibujarLinea("J2 Saltar / Acción", "ENTER / SHIFT DER.", teclado, y);
            y = DibujarLinea("Cancelar", "J1 RETROCESO, J2 SHIFT DER.", teclado, y);
            DibujarLinea("Pausa / Salir", "ESC", teclado, y);
        }

        int conectados = 0;

        for (int g = 0; g < 4; g++)
        {
            if (IsGamepadAvailable(g))
            {
                conectados++;
            }
        }

        // Columna izquierda: botones. Columna derecha: vista del stick.
        Rectangle columna = mando;
        columna.width = mando.width * 0.58f;

        float ym = mando.y + Px(36.0f);

        ym = DibujarLinea("Mover", "Stick o cruceta", columna, ym);
        ym = DibujarLinea("Saltar, aceptar", "A", columna, ym);
        ym = DibujarLinea("Acción", "X", columna, ym);
        ym = DibujarLinea("Volver", "B", columna, ym);
        ym = DibujarLinea("Pausa", "START", columna, ym);
        DibujarLinea("Conectados", TextFormat("%d", conectados), columna, ym);

        DibujarVistaStick(
            Rectangle{
                mando.x + columna.width,
                mando.y + Px(36.0f),
                mando.width - columna.width - Px(10.0f),
                mando.height - Px(40.0f)
            }
        );

        return;
    }

    if (categoria == CATEGORIA_JUEGO)
    {
        const float yRef = REF_RESTAURAR.y + REF_RESTAURAR.height + 26.0f;
        Rectangle tarjeta = EnPantallaColgado(Rectangle{ 300.0f, yRef, 680.0f, Y_FIN_CONTENIDO - yRef }, colgado);

        DibujarTarjeta(estilo, tarjeta, "RESUMEN ACTUAL");

        float y = tarjeta.y + Px(36.0f);

        y = DibujarLinea("Pantalla", NombreModoVentana(config.modoVentana), tarjeta, y);
        y = DibujarLinea("Resolución", TextoResolucion(config, resoluciones), tarjeta, y);
        y = DibujarLinea("FPS", TextFormat("%d%s", opcionesFPS[config.indiceFPS], config.vsync ? " (VSync)" : ""), tarjeta, y);
        y = DibujarLinea("Música / Efectos", TextFormat("%d%% / %d%%", (int)std::lround(config.volumenMusica * 100.0f), (int)std::lround(config.volumenSonidos * 100.0f)), tarjeta, y);
        y = DibujarLinea("Teclado", NombreModoTeclado(config.modoTeclado), tarjeta, y);
        DibujarLinea("Guardado", "Automático al aplicar cada cambio", tarjeta, y);

        return;
    }

    if (categoria == CATEGORIA_ACCESIBILIDAD)
    {
        const float yRef = Y_FILAS + PASO_FILAS + 10.0f;
        Rectangle tarjeta = EnPantallaColgado(Rectangle{ 300.0f, yRef, 680.0f, 100.0f }, colgado);

        DibujarTarjeta(estilo, tarjeta, "QUÉ CAMBIA");

        float y = tarjeta.y + Px(36.0f);

        y = DibujarLinea("Minijuegos", "Sin temblores de cámara", tarjeta, y);
        y = DibujarLinea("Menús", "Carteles quietos, sin caídas ni balanceo", tarjeta, y);
        DibujarLinea("Estado", config.reducirMovimiento ? "ACTIVADO" : "Desactivado", tarjeta, y);
    }
}


// Altura (referencia) de los ganchos del cartel de confirmacion: entre la
// tira de pestanas y la primera fila, asi sus sogas no cruzan ningun texto.
static const float Y_GANCHOS_MODAL = 206.0f;


// Como DibujarPanelUI, pero las sogas salen de dos ganchos clavados en el
// panel bajo las pestanas en lugar del borde superior de la pantalla.
static void DibujarPanelModal(
    const EstiloUI& estilo,
    const MenuConfiguracion& menu
)
{
    const PanelUI& panel = menu.panelModal;

    if (!estilo.cargado || panel.colgado.retraso > 0.0f)
    {
        return;
    }

    const float escala = EscalaUI();
    Rectangle rect = EnPantallaColgado(panel.ref, panel.colgado);

    const float yGancho =
        EnPantalla(Rectangle{ 0.0f, Y_GANCHOS_MODAL, 0.0f, 0.0f }).y +
        menu.panel.colgado.caida.valor * escala;
    const float inset = Px(46.0f);

    for (int lado = 0; lado < 2; lado++)
    {
        const float x = lado == 0 ? rect.x + inset : rect.x + rect.width - inset;

        if (rect.y > yGancho)
        {
            DibujarCuerdaUI(estilo, Vector2{ x, yGancho }, Vector2{ x, rect.y + Px(4.0f) });
        }

        DrawCircleV(Vector2{ x, yGancho }, Px(4.0f), UI_TINTA);
        DrawCircleV(Vector2{ x, yGancho }, Px(2.5f), UI_MADERA_OSCURA);
    }

    DrawRectangleRec(
        Rectangle{ rect.x + Px(6.0f), rect.y + Px(8.0f), rect.width, rect.height },
        Fade(BLACK, 0.30f)
    );

    DibujarNueveCortes(estilo.madera, 12, rect, escala, WHITE);

    const float margen = Px(14.0f);
    Rectangle interior = {
        rect.x + margen, rect.y + margen,
        rect.width - margen * 2.0f, rect.height - margen * 2.0f
    };

    DibujarNueveCortes(estilo.placa, 8, interior, escala, WHITE);
}


static void DibujarModal(
    const MenuConfiguracion& menu
)
{
    const EstiloUI& estilo = ObtenerEstiloUI();

    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(COLOR_VELO, 0.5f));

    DibujarPanelModal(estilo, menu);

    Rectangle interior = EnPantallaColgado(AreaInteriorPanelUI(menu.panelModal), menu.panelModal.colgado);
    const float centroX = interior.x + interior.width * 0.5f;

    DibujarTextoAjustado(
        "¿Mantener esta configuración de video?",
        centroX,
        interior.y + Px(22.0f),
        TamanoTexto(UI_TEXTO_GRANDE),
        interior.width - Px(40.0f),
        UI_TINTA,
        true
    );

    DibujarTextoAjustado(
        TextFormat("Se revertirá automáticamente en %d s", (int)std::ceil(menu.tiempoConfirmacion)),
        centroX,
        interior.y + Px(66.0f),
        TamanoTexto(UI_TEXTO),
        interior.width - Px(40.0f),
        UI_TINTA_SUAVE,
        true
    );

    // Cuenta atras: mismo riel que las palancas.
    Rectangle riel = {
        interior.x + Px(40.0f),
        interior.y + Px(100.0f),
        interior.width - Px(80.0f),
        Px(10.0f)
    };

    float fraccion = LimitarFloat(menu.tiempoConfirmacion / menu.DURACION_CONFIRMACION, 0.0f, 1.0f);

    DrawRectangleRec(riel, UI_TINTA);
    DrawRectangleRec(
        Rectangle{ riel.x + Px(1.0f), riel.y + Px(1.0f), (riel.width - Px(2.0f)) * fraccion, riel.height - Px(2.0f) },
        UI_MADERA_MEDIA
    );

    DibujarBotonUI(estilo, menu.botonMantener, menu.focoModal == 0);
    DibujarBotonUI(estilo, menu.botonRevertir, menu.focoModal == 1);
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
    const EstiloUI& estilo = ObtenerEstiloUI();
    const float salida = saliendo ? LimitarFloat(tiempoSalida / DURACION_TRANSICION, 0.0f, 1.0f) : 0.0f;

    // Velo suave sobre el HUB 3D que ya se dibujo de fondo.
    DrawRectangle(
        0, 0, GetScreenWidth(), GetScreenHeight(),
        Fade(COLOR_VELO, 0.35f * (1.0f - salida))
    );

    DibujarTituloUI(estilo, colgadoTitulo, "CONFIGURACIÓN", REF_TITULO);
    DibujarPanelUI(estilo, panel);

    //------------------------------
    // PESTANAS
    //------------------------------

    const bool hoverAnterior = CheckCollisionPointRec(GetMousePosition(), EnPantalla(RefBotonAnterior()));
    const bool hoverSiguiente = CheckCollisionPointRec(GetMousePosition(), EnPantalla(RefBotonSiguiente()));

    DibujarBotonUI(estilo, botonAnterior, hoverAnterior && !confirmandoVideo);
    DibujarBotonUI(estilo, botonSiguiente, hoverSiguiente && !confirmandoVideo);

    for (int i = 0; i < CANTIDAD_CATEGORIAS_CONFIGURACION; i++)
    {
        DibujarBotonUI(
            estilo,
            pestanas[i],
            categoriaHover == i && i != categoriaActual && !confirmandoVideo
        );
    }

    //------------------------------
    // FILAS Y EXTRA
    //------------------------------

    const int cantidad = CantidadOpciones(categoriaActual);
    const int seleccion = opcionSeleccionada[categoriaActual];

    if (filasListas)
    {
        for (int i = 0; i < cantidad; i++)
        {
            const int opcion = TABLA_OPCIONES[categoriaActual][i];
            const bool foco = seleccion == i && !confirmandoVideo;

            if (EsSlider(opcion))
            {
                DibujarPalancaUI(estilo, palancas[opcion], foco);

                if (opcion == OPCION_CONTROL_ZONA_MUERTA)
                {
                    DibujarValorPalanca(palancas[opcion], TextFormat("%d%%", PorcentajeSlider(config, opcion)));
                }
            }
            else if (EsInterruptor(opcion))
            {
                DibujarInterruptorUI(estilo, interruptores[opcion], foco);
            }
            else if (EsAccion(opcion))
            {
                DibujarBotonUI(estilo, botonRestaurar, foco);
            }
            else
            {
                DibujarSelectorUI(
                    estilo,
                    selectores[opcion],
                    TextoValorSelector(config, opcion, resoluciones, opcionesFPS),
                    foco
                );
            }
        }

        DibujarContenidoExtra(*this, config, resoluciones, opcionesFPS);
    }

    //------------------------------
    // INFO / AVISO (sobre la placa del panel)
    //------------------------------

    const float desplazamientoPanel = panel.colgado.caida.valor * EscalaUI();
    const int fuenteInfo = TamanoTexto(UI_TEXTO_PEQUENO);

    if (filasListas)
    {
        const char* textoInfo = "";
        bool esAviso = false;

        if (tiempoAviso > 0.0f)
        {
            textoInfo = aviso;
            esAviso = true;
        }
        else if (restaurarArmado)
        {
            textoInfo = "Restaura TODO, incluida la pantalla (modo y resolución). Pulsá otra vez para confirmar.";
            esAviso = true;
        }
        else if (seleccion == cantidad)
        {
            textoInfo = "Vuelve al menú principal.";
        }
        else
        {
            textoInfo = DescripcionOpcion(TABLA_OPCIONES[categoriaActual][seleccion]);
        }

        Rectangle info = EnPantalla(Rectangle{ X_FILAS, Y_INFO, ANCHO_FILAS, 20.0f });
        info.y += desplazamientoPanel;

        // Aviso: marca roja a la izquierda; el texto sigue en tinta.
        if (esAviso)
        {
            DrawCircleV(
                Vector2{ info.x + Px(6.0f), info.y + fuenteInfo * 0.5f },
                Px(5.0f),
                UI_PELIGRO
            );
        }

        DibujarTextoAjustado(
            textoInfo,
            info.x + Px(20.0f),
            info.y,
            fuenteInfo,
            info.width - Px(20.0f),
            esAviso ? UI_TINTA : UI_TINTA_SUAVE,
            false
        );
    }

    {
        Rectangle nota = EnPantalla(Rectangle{ X_FILAS, 592.0f, 330.0f, 20.0f });
        nota.y += desplazamientoPanel;

        DibujarTextoAjustado(
            "Los cambios se guardan automáticamente.",
            nota.x,
            nota.y,
            fuenteInfo,
            nota.width,
            UI_TINTA_SUAVE,
            false
        );
    }

    DibujarBotonUI(estilo, botonVolver, seleccion == cantidad && !confirmandoVideo);

    DibujarAyudaUI(
        estilo,
        "FLECHAS/WASD O CRUCETA: ELEGIR Y CAMBIAR   ENTER/A: ACEPTAR   Q-E / LB-RB: PESTAÑAS   ESC/B: VOLVER"
    );

    if (confirmandoVideo && modalIniciado)
    {
        DibujarModal(*this);
    }
}
