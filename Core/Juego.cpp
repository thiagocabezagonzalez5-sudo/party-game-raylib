#include "Core/Juego.h"

#include "raylib.h"
#include "Systems/Input.h"


//==================================================
// RECURSOS
//==================================================

static void PrepararDirectorioDeRecursos()
{
    if (DirectoryExists("Assets"))
    {
        return;
    }

    const char* directorioAplicacion =
        GetApplicationDirectory();

    if (
        directorioAplicacion != nullptr &&
        DirectoryExists(
            TextFormat(
                "%s../Assets",
                directorioAplicacion
            )
        )
    )
    {
        bool directorioCambiado =
            ChangeDirectory(
                TextFormat(
                    "%s..",
                    directorioAplicacion
                )
            );

        if (directorioCambiado)
        {
            TraceLog(
                LOG_INFO,
                "Directorio de recursos preparado: %s",
                GetWorkingDirectory()
            );
        }
        else
        {
            TraceLog(
                LOG_WARNING,
                "No se pudo acceder a la carpeta del proyecto"
            );
        }
    }
    else
    {
        TraceLog(
            LOG_WARNING,
            "No se encontro la carpeta Assets junto al ejecutable ni un nivel arriba"
        );
    }
}


//==================================================
// FLUJO FINAL
//==================================================

static void PrepararSeleccionDePersonajes(
    Juego& juego
)
{
    juego.cantidadParticipantes = 0;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        juego.participantes[i] = Participante{};
        juego.participantes[i].numeroJugador = i + 1;
    }

    ConfigurarControlesParticipantes(
        juego.participantes,
        MAX_PARTICIPANTES,
        juego.config.modoTeclado
    );

    juego.seleccionPersonajes.Inicializar(
        juego.participantes,
        MAX_PARTICIPANTES
    );
}


static ModoZonaPruebas ConvertirCatalogoAModo(
    int idCatalogo
)
{
    switch (idCatalogo)
    {
        case CATALOGO_COLOR_SEGURO:
            return PRUEBA_COLOR_SEGURO;

        case CATALOGO_PELOTAS:
            return PRUEBA_PELOTAS_EMPUJON;

        case CATALOGO_TRONCO:
            return PRUEBA_TRONCO_COORDINADO;

        case CATALOGO_FABRICA_67:
            return PRUEBA_FABRICA_67;

        case CATALOGO_ISLA_FUEGO:
            return PRUEBA_ISLA_FUEGO;

        case CATALOGO_CAPITAN_MANDA:
            return PRUEBA_CAPITAN_MANDA;

        case CATALOGO_BARRA_GIRATORIA:
            return PRUEBA_BARRA_GIRATORIA;
    }

    return PRUEBA_COLOR_SEGURO;
}


static bool CancelarPantallaConMando()
{
    for (int i = 0; i < 4; i++)
    {
        if (
            IsGamepadAvailable(i) &&
            IsGamepadButtonPressed(
                i,
                GAMEPAD_BUTTON_RIGHT_FACE_RIGHT
            )
        )
        {
            return true;
        }
    }

    return false;
}


static void DibujarTableroVacio()
{
    DrawRectangle(
        0,
        0,
        GetScreenWidth(),
        GetScreenHeight(),
        Fade(Color{ 14, 16, 22, 255 }, 0.66f)
    );

    Rectangle panel =
    {
        GetScreenWidth() / 2.0f - 330.0f,
        GetScreenHeight() / 2.0f - 145.0f,
        660.0f,
        290.0f
    };

    DrawRectangle(
        (int)panel.x,
        (int)panel.y,
        (int)panel.width,
        (int)panel.height,
        Fade(BLACK, 0.82f)
    );

    DrawRectangleLinesEx(
        panel,
        4.0f,
        ORANGE
    );

    const char* titulo = "TABLERO";
    const char* estado = "EN CONSTRUCCION";
    const char* ayuda = "ESC / B PARA VOLVER";

    DrawText(
        titulo,
        GetScreenWidth() / 2 - MeasureText(titulo, 44) / 2,
        (int)panel.y + 52,
        44,
        RAYWHITE
    );

    DrawText(
        estado,
        GetScreenWidth() / 2 - MeasureText(estado, 28) / 2,
        (int)panel.y + 128,
        28,
        ORANGE
    );

    DrawText(
        ayuda,
        GetScreenWidth() / 2 - MeasureText(ayuda, 18) / 2,
        (int)panel.y + 218,
        18,
        LIGHTGRAY
    );
}


//==================================================
// INICIALIZAR
//==================================================

void Juego::Inicializar()
{
    InitWindow(
        1280,
        720,
        "Juego de Party"
    );

    PrepararDirectorioDeRecursos();
    SetExitKey(KEY_NULL);

    InicializarResoluciones();

    int monitor = GetCurrentMonitor();
    int anchoMonitor = GetMonitorWidth(monitor);
    int altoMonitor = GetMonitorHeight(monitor);
    int indiceNativo = 0;

    for (int i = 0; i < cantidadResoluciones; i++)
    {
        if (
            resoluciones[i].ancho == anchoMonitor &&
            resoluciones[i].alto == altoMonitor
        )
        {
            indiceNativo = i;
            break;
        }
    }

    config = ConfiguracionJuego{};

    bool configEncontrada =
        CargarConfiguracion(
            rutaConfiguracion,
            config
        );

    if (!configEncontrada)
    {
        config.modoVentana = MODO_PANTALLA_COMPLETA;
        config.indiceResolucion = indiceNativo;
        config.indiceFPS = 1;
    }

    if (
        config.indiceResolucion < 0 ||
        config.indiceResolucion >= cantidadResoluciones
    )
    {
        config.indiceResolucion = indiceNativo;
    }

    if (
        config.indiceFPS < 0 ||
        config.indiceFPS >= CANTIDAD_OPCIONES_FPS
    )
    {
        config.indiceFPS = 1;
    }

    if (
        config.modoTeclado < TECLADO_COMPLETO ||
        config.modoTeclado > TECLADO_DIVIDIDO
    )
    {
        config.modoTeclado = TECLADO_DIVIDIDO;
    }

    if (config.volumenMusica < 0.0f) config.volumenMusica = 0.0f;
    if (config.volumenMusica > 1.0f) config.volumenMusica = 1.0f;
    if (config.volumenSonidos < 0.0f) config.volumenSonidos = 0.0f;
    if (config.volumenSonidos > 1.0f) config.volumenSonidos = 1.0f;

    cantidadParticipantes = 0;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        participantes[i] = Participante{};
        participantes[i].numeroJugador = i + 1;
    }

    ConfigurarControlesParticipantes(
        participantes,
        MAX_PARTICIPANTES,
        config.modoTeclado
    );

    SetTargetFPS(
        opcionesFPS[config.indiceFPS]
    );

    ModoVentana modoActual = MODO_VENTANA;

    AplicarModoVentana(
        modoActual,
        config.modoVentana,
        resoluciones[config.indiceResolucion]
    );

    config.modoVentana = modoActual;

    audio.Inicializar();
    audio.AplicarVolumenMusica(config.volumenMusica);
    audio.AplicarVolumenSonidos(config.volumenSonidos);

    menuConfiguracion.Inicializar();
    menuModoJuego.Inicializar();
    seleccionMinijuegos.Inicializar();

    seleccionPersonajes.Inicializar(
        participantes,
        MAX_PARTICIPANTES
    );

    zonaPruebas.Inicializar(
        participantes,
        cantidadParticipantes
    );

    pantallaLogo.Inicializar(
        "Assets/UI/LogoCreador.png"
    );

    menuPreparado = false;
    cargaMenuSolicitada = false;
    estado = ESTADO_LOGO;
    cerrarJuego = false;
}


//==================================================
// RESOLUCIONES
//==================================================

void Juego::InicializarResoluciones()
{
    cantidadResoluciones = 0;

    int monitor = GetCurrentMonitor();
    int anchoMonitor = GetMonitorWidth(monitor);
    int altoMonitor = GetMonitorHeight(monitor);

    Resolucion candidatas[] =
    {
        { 800, 600 },
        { 1024, 576 },
        { 1280, 720 },
        { 1366, 768 },
        { 1600, 900 },
        { 1920, 1080 },
        { 2560, 1440 },
        { 3840, 2160 }
    };

    const int cantidadCandidatas =
        sizeof(candidatas) / sizeof(candidatas[0]);

    for (int i = 0; i < cantidadCandidatas; i++)
    {
        if (
            candidatas[i].ancho <= anchoMonitor &&
            candidatas[i].alto <= altoMonitor
        )
        {
            AgregarResolucion(
                resoluciones,
                cantidadResoluciones,
                MAX_RESOLUCIONES,
                candidatas[i].ancho,
                candidatas[i].alto
            );
        }
    }

    AgregarResolucion(
        resoluciones,
        cantidadResoluciones,
        MAX_RESOLUCIONES,
        anchoMonitor,
        altoMonitor
    );
}


//==================================================
// ACTUALIZAR
//==================================================

void Juego::Actualizar(
    float deltaTime
)
{
    audio.Actualizar();

    switch (estado)
    {
        case ESTADO_LOGO:
        {
            pantallaLogo.Actualizar(deltaTime);

            if (
                !menuPreparado &&
                !cargaMenuSolicitada &&
                pantallaLogo.tiempo >= 1.0f
            )
            {
                cargaMenuSolicitada = true;
                break;
            }

            if (cargaMenuSolicitada && !menuPreparado)
            {
                menuPrincipal.Inicializar();

                audio.CargarMusicaMenu(
                    "Assets/Audio/MusicaMenu.mp3"
                );

                audio.AplicarVolumenMusica(
                    config.volumenMusica
                );

                menuPreparado = true;
                cargaMenuSolicitada = false;
                pantallaLogo.tiempo = 1.5f;
            }

            if (
                menuPreparado &&
                pantallaLogo.Termino()
            )
            {
                menuPrincipal.PrepararEntrada(true);
                audio.ReproducirMusicaMenu();
                estado = ESTADO_MENU;
            }

            break;
        }

        case ESTADO_MENU:
        {
            menuPrincipal.Actualizar(deltaTime);

            if (menuPrincipal.empezarJuego)
            {
                menuPrincipal.empezarJuego = false;
                menuModoJuego.Inicializar();
                estado = ESTADO_SELECCION_MODO;
            }

            if (menuPrincipal.abrirConfiguracion)
            {
                menuPrincipal.abrirConfiguracion = false;
                menuConfiguracion.Inicializar();
                estado = ESTADO_CONFIGURACION;
            }

            if (menuPrincipal.salir)
            {
                GuardarConfiguracion(
                    rutaConfiguracion,
                    config
                );

                cerrarJuego = true;
            }

            break;
        }

        case ESTADO_CONFIGURACION:
        {
            menuPrincipal.fondo.Actualizar(deltaTime);

            menuConfiguracion.Actualizar(
                config,
                resoluciones,
                cantidadResoluciones,
                opcionesFPS,
                CANTIDAD_OPCIONES_FPS,
                audio
            );

            if (menuConfiguracion.configuracionCambiada)
            {
                GuardarConfiguracion(
                    rutaConfiguracion,
                    config
                );

                menuConfiguracion.configuracionCambiada = false;
            }

            if (menuConfiguracion.volver)
            {
                menuConfiguracion.volver = false;

                GuardarConfiguracion(
                    rutaConfiguracion,
                    config
                );

                menuPrincipal.PrepararEntrada(false);
                estado = ESTADO_MENU;
            }

            break;
        }

        case ESTADO_SELECCION_MODO:
        {
            menuPrincipal.fondo.Actualizar(deltaTime);
            menuModoJuego.Actualizar(deltaTime);

            if (menuModoJuego.volver)
            {
                menuPrincipal.PrepararEntrada(false);
                estado = ESTADO_MENU;
                break;
            }

            if (menuModoJuego.confirmar)
            {
                if (
                    menuModoJuego.opcionSeleccionada ==
                    MODO_JUEGO_MINIJUEGOS
                )
                {
                    PrepararSeleccionDePersonajes(*this);
                    estado = ESTADO_SELECCION_JUGADORES;
                }
                else
                {
                    estado = ESTADO_TABLERO_VACIO;
                }
            }

            break;
        }

        case ESTADO_TABLERO_VACIO:
        {
            menuPrincipal.fondo.Actualizar(deltaTime);

            if (
                IsKeyPressed(KEY_ESCAPE) ||
                CancelarPantallaConMando()
            )
            {
                menuModoJuego.Inicializar();
                estado = ESTADO_SELECCION_MODO;
            }

            break;
        }

        case ESTADO_SELECCION_JUGADORES:
        {
            menuPrincipal.fondo.Actualizar(deltaTime);

            seleccionPersonajes.Actualizar(
                deltaTime,
                participantes,
                MAX_PARTICIPANTES
            );

            int cantidadHumana = 0;
            bool activosPreparados = true;

            for (int i = 0; i < MAX_PARTICIPANTES; i++)
            {
                if (!participantes[i].activo)
                {
                    continue;
                }

                cantidadHumana++;

                if (
                    !participantes[i].conectado ||
                    !seleccionPersonajes.jugadores[i].listo
                )
                {
                    activosPreparados = false;
                }
            }

            // J1 tiene que ser humano porque es quien elige el
            // minijuego en el catalogo de la siguiente pantalla.
            bool cantidadValida =
                cantidadHumana >= 1 &&
                cantidadHumana <= MAX_PARTICIPANTES &&
                participantes[0].activo;

            seleccionPersonajes.todosListos =
                cantidadValida && activosPreparados;

            seleccionPersonajes.iniciarPartida =
                seleccionPersonajes.todosListos;

            if (seleccionPersonajes.iniciarPartida)
            {
                seleccionPersonajes.iniciarPartida = false;

                CompletarParticipantesConBots(
                    participantes,
                    MAX_PARTICIPANTES
                );

                cantidadParticipantes = MAX_PARTICIPANTES;
                seleccionMinijuegos.Inicializar();
                estado = ESTADO_SELECCION_MINIJUEGO;
                break;
            }

            if (seleccionPersonajes.volverAlMenu)
            {
                seleccionPersonajes.volverAlMenu = false;
                menuModoJuego.Inicializar();
                estado = ESTADO_SELECCION_MODO;
            }

            break;
        }

        case ESTADO_SELECCION_MINIJUEGO:
        {
            menuPrincipal.fondo.Actualizar(deltaTime);

            seleccionMinijuegos.Actualizar(
                deltaTime,
                participantes[0]
            );

            if (seleccionMinijuegos.volver)
            {
                PrepararSeleccionDePersonajes(*this);
                estado = ESTADO_SELECCION_JUGADORES;
                break;
            }

            if (seleccionMinijuegos.confirmado)
            {
                ModoZonaPruebas modoElegido =
                    ConvertirCatalogoAModo(
                        seleccionMinijuegos.indiceSeleccionado
                    );

                zonaPruebas.Inicializar(
                    participantes,
                    MAX_PARTICIPANTES
                );

                zonaPruebas.modoCatalogo = true;
                zonaPruebas.CambiarModo(modoElegido);
                estado = ESTADO_ZONA_PRUEBAS;
            }

            break;
        }

        case ESTADO_ZONA_PRUEBAS:
        {
            zonaPruebas.Actualizar(deltaTime);

            if (zonaPruebas.volverAlMenu)
            {
                zonaPruebas.volverAlMenu = false;

                if (zonaPruebas.modoCatalogo)
                {
                    zonaPruebas.modoCatalogo = false;
                    estado = ESTADO_SELECCION_MINIJUEGO;
                }
                else
                {
                    menuPrincipal.PrepararEntrada(false);
                    estado = ESTADO_MENU;
                }
            }

            break;
        }

        case ESTADO_PARTIDA:
        case ESTADO_MINIJUEGO:
        case ESTADO_RESULTADO:
        {
            break;
        }
    }
}


//==================================================
// DIBUJAR
//==================================================

void Juego::Dibujar()
{
    switch (estado)
    {
        case ESTADO_LOGO:
        {
            pantallaLogo.Dibujar();

            if (
                cargaMenuSolicitada &&
                !menuPreparado
            )
            {
                const char* texto = "CARGANDO...";

                DrawText(
                    texto,
                    GetScreenWidth() / 2 - MeasureText(texto, 24) / 2,
                    GetScreenHeight() - 100,
                    24,
                    BLACK
                );
            }

            break;
        }

        case ESTADO_MENU:
        {
            menuPrincipal.Dibujar();
            break;
        }

        case ESTADO_CONFIGURACION:
        {
            menuPrincipal.fondo.DibujarPantallaCompleta();

            menuConfiguracion.Dibujar(
                config,
                resoluciones,
                opcionesFPS
            );

            break;
        }

        case ESTADO_SELECCION_MODO:
        {
            menuPrincipal.fondo.DibujarPantallaCompleta();
            menuModoJuego.Dibujar();
            break;
        }

        case ESTADO_TABLERO_VACIO:
        {
            menuPrincipal.fondo.DibujarPantallaCompleta();
            DibujarTableroVacio();
            break;
        }

        case ESTADO_SELECCION_JUGADORES:
        {
            menuPrincipal.fondo.DibujarPantallaCompleta();

            seleccionPersonajes.Dibujar(
                participantes,
                MAX_PARTICIPANTES
            );

            if (!participantes[0].activo)
            {
                const char* aviso =
                    "JUGADOR 1 DEBE UNIRSE PARA ELEGIR EL MINIJUEGO";

                DrawText(
                    aviso,
                    GetScreenWidth() / 2 - MeasureText(aviso, 18) / 2,
                    GetScreenHeight() - 76,
                    18,
                    ORANGE
                );
            }

            break;
        }

        case ESTADO_SELECCION_MINIJUEGO:
        {
            menuPrincipal.fondo.DibujarPantallaCompleta();
            seleccionMinijuegos.Dibujar(participantes[0]);
            break;
        }

        case ESTADO_ZONA_PRUEBAS:
        {
            zonaPruebas.Dibujar();
            break;
        }

        case ESTADO_PARTIDA:
        case ESTADO_MINIJUEGO:
        {
            ClearBackground(SKYBLUE);
            break;
        }

        case ESTADO_RESULTADO:
        {
            ClearBackground(BLACK);
            break;
        }
    }

    if (
        config.mostrarFPS &&
        estado != ESTADO_LOGO
    )
    {
        const char* texto =
            TextFormat("FPS: %d", GetFPS());

        DrawText(
            texto,
            GetScreenWidth() - MeasureText(texto, 20) - 20,
            20,
            20,
            DARKGREEN
        );
    }
}


bool Juego::DebeCerrar()
{
    return cerrarJuego;
}


void Juego::Descargar()
{
    GuardarConfiguracion(
        rutaConfiguracion,
        config
    );

    zonaPruebas.Descargar();
    seleccionPersonajes.Descargar();
    audio.Descargar();
    pantallaLogo.Descargar();
    menuPrincipal.Descargar();
}
