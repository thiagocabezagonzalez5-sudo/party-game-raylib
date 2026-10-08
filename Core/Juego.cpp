#include "Core/Juego.h"
#include "Core/CatalogoMinijuegos.h"

#include "raylib.h"

#include <cmath>
#include <algorithm>
#include "Systems/Input.h"
#include "Minigames/EfectosVisualesMinijuegos.h"
#include "Systems/CalidadGrafica.h"


//==================================================
// RECURSOS
//==================================================

static void PrepararDirectorioDeRecursos()
{
    if (DirectoryExists(DIRECTORIO_RECURSOS))
    {
        return;
    }

    const char* directorioAplicacion = GetApplicationDirectory();

    if (
        directorioAplicacion != nullptr &&
        DirectoryExists(
            TextFormat(
                "%s../%s",
                directorioAplicacion,
                DIRECTORIO_RECURSOS
            )
        )
    )
    {
        bool directorioCambiado =
            ChangeDirectory(
                TextFormat("%s..", directorioAplicacion)
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


static IdMinijuego ElegirMinijuegoAleatorioTablero(
    const Participante participantes[]
)
{
    int indices[MAX_PARTICIPANTES]{};
    int jugadores = ObtenerIndicesParticipantesActivos(
        participantes,
        indices,
        MAX_PARTICIPANTES
    );

    // Solo se sortean minijuegos jugables con esta cantidad de participantes
    // (p.ej. los 2 vs 2 estrictos quedan fuera con 3 jugadores).
    IdMinijuego candidatos[CANTIDAD_MINIJUEGOS]{};
    int cantidadCandidatos = 0;
    const int disponibles = ObtenerCantidadMinijuegosDisponiblesTablero();

    for (int i = 0; i < disponibles; i++)
    {
        IdMinijuego id = ObtenerMinijuegoDisponibleTablero(i);

        if (MinijuegoAdmiteCantidadJugadores(id, jugadores))
        {
            candidatos[cantidadCandidatos] = id;
            cantidadCandidatos++;
        }
    }

    if (cantidadCandidatos <= 0)
    {
        return MINIJUEGO_COLOR_SEGURO;
    }

    return candidatos[GetRandomValue(0, cantidadCandidatos - 1)];
}


// Arma una partida de tablero nueva (monedas, trofeos, ronda, gimmicks y
// fichas vuelven a su estado inicial). En la primera partida los puestos
// libres se completan con bots; en una revancha los bots ya existen y
// CompletarParticipantesConBots los convertiria en humanos.
static void PrepararPartidaTablero(
    Juego& juego,
    IdTablero tablero,
    int rondas,
    bool completarBots
)
{
    if (completarBots)
    {
        CompletarParticipantesConBots(
            juego.participantes,
            MAX_PARTICIPANTES
        );
    }

    juego.cantidadParticipantes = MAX_PARTICIPANTES;

    juego.partidaTablero.Inicializar(
        juego.participantes,
        MAX_PARTICIPANTES,
        &juego.audio,
        tablero
    );

    juego.partidaTablero.cantidadRondas = rondas;

    juego.zonaPruebas.modoCatalogo = false;
    juego.zonaPruebas.modoTablero = false;
    juego.tiempoResultadoMinijuegoTablero = 0.0f;
    juego.tiempoRondaMinijuegoTablero = 0.0f;
    juego.confirmandoSalida = false;
}


static void VolverAlHub(
    Juego& juego
)
{
    juego.confirmandoSalida = false;
    juego.menuModoJuego.Inicializar();
    juego.menuPrincipal.PrepararEntrada(false);
    juego.estado = ESTADO_MENU;
}


static void DibujarConfirmacionSalida(
    int opcion,
    float tiempo
)
{
    const int ancho = GetScreenWidth();
    const int alto = GetScreenHeight();
    const float aparicion =
        tiempo >= 0.2f ? 1.0f : tiempo / 0.2f;

    DrawRectangle(0, 0, ancho, alto, Fade(BLACK, 0.62f * aparicion));

    Rectangle panel =
    {
        ancho / 2.0f - 300.0f,
        alto / 2.0f - 130.0f,
        600.0f,
        260.0f
    };

    DrawRectangleRec(panel, Fade(Color{ 24, 27, 35, 255 }, aparicion));
    DrawRectangleLinesEx(panel, 4.0f, Fade(ORANGE, aparicion));

    const char* titulo = "SALIR DE LA PARTIDA?";
    const char* aviso = "SE PIERDE EL PROGRESO DE ESTA PARTIDA";

    DrawText(
        titulo,
        ancho / 2 - MeasureText(titulo, 34) / 2,
        (int)panel.y + 34,
        34,
        Fade(RAYWHITE, aparicion)
    );

    DrawText(
        aviso,
        ancho / 2 - MeasureText(aviso, 18) / 2,
        (int)panel.y + 84,
        18,
        Fade(LIGHTGRAY, aparicion)
    );

    const char* textos[2] = { "CONTINUAR", "SALIR AL HUB" };

    for (int i = 0; i < 2; i++)
    {
        Rectangle boton =
        {
            panel.x + 40.0f + i * 270.0f,
            panel.y + 130.0f,
            250.0f,
            56.0f
        };

        const bool elegido = i == opcion;

        DrawRectangleRec(
            boton,
            Fade(elegido ? ORANGE : Color{ 40, 44, 58, 255 }, aparicion)
        );

        DrawRectangleLinesEx(
            boton,
            elegido ? 4.0f : 2.0f,
            Fade(elegido ? RAYWHITE : GRAY, aparicion)
        );

        DrawText(
            textos[i],
            (int)(boton.x + boton.width / 2.0f) - MeasureText(textos[i], 22) / 2,
            (int)boton.y + 17,
            22,
            Fade(elegido ? BLACK : RAYWHITE, aparicion)
        );
    }

    const char* ayuda = "IZQ / DER: ELEGIR   |   ESPACIO / A: ACEPTAR   |   ESC / B: CONTINUAR";

    DrawText(
        ayuda,
        ancho / 2 - MeasureText(ayuda, 16) / 2,
        (int)panel.y + 216,
        16,
        Fade(GRAY, aparicion)
    );
}


static bool ConfirmarConParticipanteHumano(
    const Participante participantes[]
)
{
    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (
            !participantes[i].activo ||
            participantes[i].esBot ||
            !participantes[i].conectado
        )
        {
            continue;
        }

        InputSeleccionParticipante entrada =
            LeerInputSeleccionParticipante(participantes[i]);

        if (entrada.confirmar)
        {
            return true;
        }
    }

    return false;
}


static bool CancelarPantallaConMando()
{
    for (int i = 0; i < MAX_PARTICIPANTES; i++)
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

    DrawRectangleLinesEx(panel, 4.0f, ORANGE);

    const char* titulo = "TABLERO";
    const char* estadoTexto = "ESTADO ANTIGUO";
    const char* ayuda = "ESC / B PARA VOLVER";

    DrawText(
        titulo,
        GetScreenWidth() / 2 - MeasureText(titulo, 44) / 2,
        (int)panel.y + 52,
        44,
        RAYWHITE
    );

    DrawText(
        estadoTexto,
        GetScreenWidth() / 2 - MeasureText(estadoTexto, 28) / 2,
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
        ANCHO_INICIAL,
        ALTO_INICIAL,
        TITULO_JUEGO
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
        CargarConfiguracion(RUTA_CONFIGURACION_JUEGO, config);

    if (!configEncontrada)
    {
        config.modoVentana = MODO_PANTALLA_COMPLETA;
        config.indiceResolucion = indiceNativo;
        config.indiceFPS = 1;
    }

    // Rangos, indices y NaN se corrigen en un solo lugar (ConfiguracionJuego).
    NormalizarConfiguracion(
        config,
        cantidadResoluciones,
        CANTIDAD_OPCIONES_FPS,
        indiceNativo
    );

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

    SetTargetFPS(opcionesFPS[config.indiceFPS]);

    ModoVentana modoActual = MODO_VENTANA;

    AplicarModoVentana(
        modoActual,
        config.modoVentana,
        resoluciones[config.indiceResolucion]
    );

    config.modoVentana = modoActual;
    AplicarVSync(config.vsync);
    EstablecerReducirMovimientoMinijuegos(config.reducirMovimiento);
    EstablecerZonaMuertaStick(config.zonaMuertaStick / 100.0f);
    EstablecerOpcionesCalidadGrafica(ObtenerOpcionesCalidadDesdeConfiguracion(config));

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
        cantidadParticipantes,
        &audio
    );

    pantallaLogo.Inicializar(RUTA_LOGO_CREADOR);

    menuPreparado = false;
    cargaMenuSolicitada = false;
    tiempoResultadoMinijuegoTablero = 0.0f;
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

// Categoria de musica para cada estado. Si la categoria no tiene pista,
// AudioJuego mantiene la que ya suena (hoy solo existe la del menu).
static CategoriaMusica ObtenerCategoriaMusicaEstado(
    EstadoJuego estado
)
{
    switch (estado)
    {
        case ESTADO_PARTIDA:
        case ESTADO_RULETA_MINIJUEGO:
        case ESTADO_TABLERO_VACIO:
        case ESTADO_INTRO_TABLERO:
        case ESTADO_ORDEN_TURNO:
        case ESTADO_RESULTADOS_PARTIDA:
            return MUSICA_TABLERO;

        case ESTADO_ZONA_PRUEBAS:
        case ESTADO_MINIJUEGO:
            return MUSICA_MINIJUEGO;

        default:
            return MUSICA_MENU;
    }
}


void Juego::Actualizar(
    float deltaTime
)
{
    const EstadoJuego estadoAlEntrar = estado;

    audio.SeleccionarMusica(ObtenerCategoriaMusicaEstado(estado));
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
                    RUTA_MUSICA_MENU
                );

                audio.AplicarVolumenMusica(config.volumenMusica);

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
            int opcionAnterior = menuPrincipal.opcionSeleccionada;
            menuPrincipal.Actualizar(deltaTime);

            if (opcionAnterior != menuPrincipal.opcionSeleccionada)
            {
                audio.ReproducirSonido(SONIDO_UI_MOVER);
            }

            // El HUB elige el modo directamente con sus objetos 3D
            // (dirigible = partida de tablero, barco = minijuegos libres).
            if (menuPrincipal.empezarTablero || menuPrincipal.empezarMinijuegos)
            {
                audio.ReproducirSonido(SONIDO_UI_CONFIRMAR);
                menuModoJuego.Inicializar();
                menuModoJuego.opcionSeleccionada =
                    menuPrincipal.empezarTablero
                        ? MODO_JUEGO_TABLERO
                        : MODO_JUEGO_MINIJUEGOS;
                menuPrincipal.empezarTablero = false;
                menuPrincipal.empezarMinijuegos = false;
                PrepararSeleccionDePersonajes(*this);
                estado = ESTADO_SELECCION_JUGADORES;
            }

            if (menuPrincipal.abrirConfiguracion)
            {
                audio.ReproducirSonido(SONIDO_UI_CONFIRMAR);
                menuPrincipal.abrirConfiguracion = false;
                menuConfiguracion.Inicializar();
                estado = ESTADO_CONFIGURACION;
            }

            if (menuPrincipal.salir)
            {
                audio.ReproducirSonido(SONIDO_UI_CONFIRMAR);
                GuardarConfiguracion(RUTA_CONFIGURACION_JUEGO, config);
                cerrarJuego = true;
            }

            break;
        }

        case ESTADO_CONFIGURACION:
        {
            menuPrincipal.ActualizarFondo(deltaTime);

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
                GuardarConfiguracion(RUTA_CONFIGURACION_JUEGO, config);
                menuConfiguracion.configuracionCambiada = false;
            }

            if (menuConfiguracion.volver)
            {
                menuConfiguracion.volver = false;
                audio.ReproducirSonido(SONIDO_UI_CANCELAR);
                GuardarConfiguracion(RUTA_CONFIGURACION_JUEGO, config);
                menuPrincipal.PrepararEntrada(false);
                estado = ESTADO_MENU;
            }

            break;
        }

        case ESTADO_SELECCION_MODO:
        {
            menuPrincipal.ActualizarFondo(deltaTime);

            int opcionAnterior = menuModoJuego.opcionSeleccionada;
            menuModoJuego.Actualizar(deltaTime);

            if (opcionAnterior != menuModoJuego.opcionSeleccionada)
            {
                audio.ReproducirSonido(SONIDO_UI_MOVER);
            }

            if (menuModoJuego.volver)
            {
                audio.ReproducirSonido(SONIDO_UI_CANCELAR);
                menuPrincipal.PrepararEntrada(false);
                estado = ESTADO_MENU;
                break;
            }

            if (menuModoJuego.confirmar)
            {
                audio.ReproducirSonido(SONIDO_UI_CONFIRMAR);
                PrepararSeleccionDePersonajes(*this);
                estado = ESTADO_SELECCION_JUGADORES;
            }

            break;
        }

        case ESTADO_TABLERO_VACIO:
        {
            menuPrincipal.ActualizarFondo(deltaTime);

            if (
                IsKeyPressed(KEY_ESCAPE) ||
                CancelarPantallaConMando()
            )
            {
                menuModoJuego.Inicializar();
                menuPrincipal.PrepararEntrada(false);
                estado = ESTADO_MENU;
            }

            break;
        }

        case ESTADO_SELECCION_JUGADORES:
        {

            seleccionPersonajes.Actualizar(
                deltaTime,
                participantes,
                MAX_PARTICIPANTES
            );

            // La pantalla avisa con banderas; el audio vive en Juego.
            if (seleccionPersonajes.sonidoMover) audio.ReproducirSonido(SONIDO_UI_MOVER);
            if (seleccionPersonajes.sonidoConfirmar) audio.ReproducirSonido(SONIDO_UI_CONFIRMAR);
            if (seleccionPersonajes.sonidoCancelar) audio.ReproducirSonido(SONIDO_UI_CANCELAR);

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

            bool modoMinijuegos =
                menuModoJuego.opcionSeleccionada ==
                MODO_JUEGO_MINIJUEGOS;

            // Basta un humano: los puestos libres se completan con bots.
            bool cantidadValida =
                cantidadHumana >= 1 &&
                cantidadHumana <= MAX_PARTICIPANTES &&
                (!modoMinijuegos || participantes[0].activo);

            seleccionPersonajes.todosListos =
                cantidadValida && activosPreparados;

            // Con todos listos se muestra el cartel y hace falta una nueva
            // confirmacion (no se avanza solo ni con la misma pulsacion).
            if (seleccionPersonajes.todosListos)
            {
                tiempoTodosListos += deltaTime;
            }
            else
            {
                tiempoTodosListos = 0.0f;
            }

            seleccionPersonajes.iniciarPartida =
                seleccionPersonajes.todosListos &&
                tiempoTodosListos >= 0.4f &&
                ConfirmarConParticipanteHumano(participantes);

            if (seleccionPersonajes.iniciarPartida)
            {
                seleccionPersonajes.iniciarPartida = false;
                tiempoTodosListos = 0.0f;
                audio.ReproducirSonido(SONIDO_UI_CONFIRMAR);

                if (modoMinijuegos)
                {
                    CompletarParticipantesConBots(
                        participantes,
                        MAX_PARTICIPANTES
                    );

                    cantidadParticipantes = MAX_PARTICIPANTES;

                    seleccionMinijuegos.Inicializar();
                    estado = ESTADO_SELECCION_MINIJUEGO;
                }
                else
                {
                    // Los bots se completan recien al comenzar la partida:
                    // asi se puede volver a elegir jugadores sin perder
                    // la seleccion.
                    seleccionTablero.Inicializar();
                    estado = ESTADO_SELECCION_TABLERO;
                }

                break;
            }

            if (seleccionPersonajes.volverAlMenu)
            {
                seleccionPersonajes.volverAlMenu = false;
                menuModoJuego.Inicializar();
                menuPrincipal.PrepararEntrada(false);
                estado = ESTADO_MENU;
            }

            break;
        }

        case ESTADO_SELECCION_TABLERO:
        {
            menuPrincipal.ActualizarFondo(deltaTime);

            ActualizarConexionesParticipantes(
                participantes,
                MAX_PARTICIPANTES
            );

            seleccionTablero.Actualizar(
                deltaTime,
                participantes,
                MAX_PARTICIPANTES,
                audio
            );

            if (seleccionTablero.volver)
            {
                // Vuelve a jugadores conservando la seleccion hecha.
                seleccionTablero.volver = false;
                seleccionPersonajes.alphaEntrada = 0.0f;
                seleccionPersonajes.iniciarPartida =
                    SolicitudInicioSeleccion{};
                estado = ESTADO_SELECCION_JUGADORES;
            }
            else if (seleccionTablero.confirmado)
            {
                seleccionTablero.confirmado = false;
                configuracionPartida.Inicializar(
                    seleccionTablero.ObtenerIdElegido()
                );
                estado = ESTADO_CONFIGURACION_PARTIDA;
            }

            break;
        }

        case ESTADO_CONFIGURACION_PARTIDA:
        {
            menuPrincipal.ActualizarFondo(deltaTime);

            ActualizarConexionesParticipantes(
                participantes,
                MAX_PARTICIPANTES
            );

            configuracionPartida.Actualizar(
                deltaTime,
                participantes,
                MAX_PARTICIPANTES,
                audio
            );

            if (configuracionPartida.volver)
            {
                configuracionPartida.volver = false;
                seleccionTablero.Inicializar();
                estado = ESTADO_SELECCION_TABLERO;
            }
            else if (configuracionPartida.confirmado)
            {
                configuracionPartida.confirmado = false;

                // Un mando que se desconecto durante la eleccion libera su
                // puesto (lo ocupa un bot). Debe quedar al menos un humano.
                int humanosConectados = 0;

                for (int i = 0; i < MAX_PARTICIPANTES; i++)
                {
                    if (!participantes[i].activo)
                    {
                        continue;
                    }

                    if (participantes[i].conectado)
                    {
                        humanosConectados++;
                    }
                    else
                    {
                        participantes[i].activo = false;
                        participantes[i].personajeSeleccionado = -1;
                        seleccionPersonajes.jugadores[i].listo = false;
                    }
                }

                if (humanosConectados < 1)
                {
                    seleccionPersonajes.alphaEntrada = 0.0f;
                    seleccionPersonajes.iniciarPartida =
                        SolicitudInicioSeleccion{};
                    estado = ESTADO_SELECCION_JUGADORES;
                    break;
                }

                PrepararPartidaTablero(
                    *this,
                    configuracionPartida.tablero,
                    configuracionPartida.ObtenerRondas(),
                    true
                );

                introTablero.Inicializar(
                    configuracionPartida.tablero,
                    configuracionPartida.ObtenerRondas()
                );

                estado = ESTADO_INTRO_TABLERO;
            }

            break;
        }

        case ESTADO_INTRO_TABLERO:
        {
            ActualizarConexionesParticipantes(
                participantes,
                MAX_PARTICIPANTES
            );

            introTablero.Actualizar(
                deltaTime,
                participantes,
                MAX_PARTICIPANTES
            );

            if (introTablero.terminada)
            {
                ordenTurno.Inicializar(partidaTablero);
                estado = ESTADO_ORDEN_TURNO;
            }

            break;
        }

        case ESTADO_ORDEN_TURNO:
        {
            ActualizarConexionesParticipantes(
                participantes,
                MAX_PARTICIPANTES
            );

            ordenTurno.Actualizar(
                deltaTime,
                participantes,
                MAX_PARTICIPANTES,
                audio
            );

            if (ordenTurno.terminado)
            {
                partidaTablero.tiempoFase = 0.0f;
                estado = ESTADO_PARTIDA;
            }

            break;
        }

        case ESTADO_RESULTADOS_PARTIDA:
        {
            ActualizarConexionesParticipantes(
                participantes,
                MAX_PARTICIPANTES
            );

            resultadosPartida.Actualizar(
                deltaTime,
                participantes,
                MAX_PARTICIPANTES,
                audio
            );

            const AccionResultados accion = resultadosPartida.accion;
            resultadosPartida.accion = RESULTADOS_NINGUNA;

            if (accion == RESULTADOS_REVANCHA)
            {
                // Mismo tablero, jugadores y rondas; todo el estado de la
                // partida (monedas, trofeos, ronda, gimmicks, fichas y
                // orden de turno) se reconstruye.
                PrepararPartidaTablero(
                    *this,
                    partidaTablero.idTablero,
                    partidaTablero.cantidadRondas,
                    false
                );

                ordenTurno.Inicializar(partidaTablero);
                estado = ESTADO_ORDEN_TURNO;
            }
            else if (accion == RESULTADOS_NUEVA_PARTIDA)
            {
                PrepararSeleccionDePersonajes(*this);
                estado = ESTADO_SELECCION_JUGADORES;
            }
            else if (accion == RESULTADOS_VOLVER_HUB)
            {
                VolverAlHub(*this);
            }

            break;
        }

        case ESTADO_SELECCION_MINIJUEGO:
        {
            menuPrincipal.ActualizarFondo(deltaTime);

            int indiceAnterior = seleccionMinijuegos.indiceSeleccionado;

            seleccionMinijuegos.Actualizar(
                deltaTime,
                participantes[0]
            );

            if (indiceAnterior != seleccionMinijuegos.indiceSeleccionado)
            {
                audio.ReproducirSonido(SONIDO_UI_MOVER);
            }

            if (seleccionMinijuegos.volver)
            {
                audio.ReproducirSonido(SONIDO_UI_CANCELAR);
                PrepararSeleccionDePersonajes(*this);
                estado = ESTADO_SELECCION_JUGADORES;
                break;
            }

            if (seleccionMinijuegos.confirmado)
            {
                audio.ReproducirSonido(SONIDO_UI_CONFIRMAR);

                IdMinijuego minijuegoElegido =
                    ObtenerIdMinijuegoPorIndice(
                        seleccionMinijuegos.indiceSeleccionado
                    );

                zonaPruebas.Inicializar(
                    participantes,
                    MAX_PARTICIPANTES,
                    &audio
                );

                zonaPruebas.modoCatalogo = true;
                zonaPruebas.CambiarMinijuego(minijuegoElegido);
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
        {
            ActualizarConexionesParticipantes(
                participantes,
                MAX_PARTICIPANTES
            );

            if (partidaTablero.fase == FASE_PARTIDA_TABLERO_TERMINADA)
            {
                // Partida invalida (sin jugadores o recorrido roto): no hay
                // nada que mostrar ni forma de jugarla.
                if (
                    partidaTablero.cantidadJugadores <= 0 ||
                    !partidaTablero.tablero.recorridoValido
                )
                {
                    VolverAlHub(*this);
                    break;
                }

                resultadosPartida.Inicializar(partidaTablero);
                estado = ESTADO_RESULTADOS_PARTIDA;
                break;
            }

            if (confirmandoSalida)
            {
                tiempoSalida += deltaTime;

                EntradaFlujo entrada =
                    lectorSalida.Leer(participantes, MAX_PARTICIPANTES);

                if (tiempoSalida < BLOQUEO_ENTRADA_FLUJO)
                {
                    break;
                }

                if (entrada.cancelar)
                {
                    confirmandoSalida = false;
                    audio.ReproducirSonido(SONIDO_UI_CANCELAR);
                }
                else if (entrada.confirmar)
                {
                    audio.ReproducirSonido(SONIDO_UI_CONFIRMAR);

                    if (opcionSalida == 0)
                    {
                        confirmandoSalida = false;
                    }
                    else
                    {
                        VolverAlHub(*this);
                    }
                }
                else if (
                    entrada.izquierda ||
                    entrada.derecha ||
                    entrada.arriba ||
                    entrada.abajo
                )
                {
                    opcionSalida = 1 - opcionSalida;
                    audio.ReproducirSonido(SONIDO_UI_MOVER);
                }

                break;
            }

            partidaTablero.Actualizar(deltaTime);

            if (partidaTablero.SolicitaSalida())
            {
                // ESC pide confirmacion: no se pierde la partida por error.
                partidaTablero.salidaSolicitada = false;
                confirmandoSalida = true;
                opcionSalida = 0;
                tiempoSalida = 0.0f;
                lectorSalida.Reiniciar();
                break;
            }

            if (partidaTablero.SolicitaMinijuego())
            {
                // El resultado se decide aca, antes de la animacion.
                // La ruleta solo lo presenta.
                ruletaMinijuegos.Iniciar(
                    ElegirMinijuegoAleatorioTablero(participantes),
                    &audio
                );

                estado = ESTADO_RULETA_MINIJUEGO;
            }

            break;
        }

        case ESTADO_RULETA_MINIJUEGO:
        {
            ActualizarConexionesParticipantes(
                participantes,
                MAX_PARTICIPANTES
            );

            ruletaMinijuegos.Actualizar(
                deltaTime,
                ConfirmarConParticipanteHumano(participantes)
            );

            if (ruletaMinijuegos.Termino())
            {
                IdMinijuego minijuegoElegido =
                    ruletaMinijuegos.ObtenerMinijuegoElegido();

                zonaPruebas.Inicializar(
                    participantes,
                    MAX_PARTICIPANTES,
                    &audio
                );

                zonaPruebas.modoCatalogo = true;
                zonaPruebas.modoTablero = true;
                zonaPruebas.CambiarMinijuego(minijuegoElegido);

                tiempoResultadoMinijuegoTablero = 0.0f;
                tiempoRondaMinijuegoTablero = 0.0f;
                estado = ESTADO_MINIJUEGO;
            }

            break;
        }

        case ESTADO_MINIJUEGO:
        {
            zonaPruebas.Actualizar(deltaTime);

            // En ronda de tablero ZonaPruebas nunca pide salir (modoTablero),
            // pero se conserva como red de seguridad.
            if (zonaPruebas.volverAlMenu)
            {
                zonaPruebas.volverAlMenu = false;
                zonaPruebas.modoCatalogo = false;
                zonaPruebas.modoTablero = false;
                partidaTablero.ContinuarTrasMinijuego();
                estado = ESTADO_PARTIDA;
                break;
            }

            const ResultadoMinijuego* resultado =
                zonaPruebas.ObtenerResultadoMinijuego();

            tiempoRondaMinijuegoTablero += deltaTime;

            if (tiempoRondaMinijuegoTablero >= 240.0f)
            {
                TraceLog(LOG_WARNING, "Ronda de tablero sin final tras 240 s: se vuelve al tablero sin premios.");
                partidaTablero.ContinuarTrasMinijuego();
                zonaPruebas.modoCatalogo = false;
                zonaPruebas.modoTablero = false;
                tiempoResultadoMinijuegoTablero = 0.0f;
                estado = ESTADO_PARTIDA;
                break;
            }

            // Sin ESC en la ronda oficial, una ronda cancelada (por ejemplo,
            // menos de 2 participantes) debe volver sola al tablero.
            if (
                resultado != nullptr &&
                resultado->estado == RESULTADO_MINIJUEGO_CANCELADO
            )
            {
                tiempoResultadoMinijuegoTablero += deltaTime;

                if (tiempoResultadoMinijuegoTablero >= 2.0f)
                {
                    partidaTablero.ContinuarTrasMinijuego();
                    zonaPruebas.modoCatalogo = false;
                    zonaPruebas.modoTablero = false;
                    tiempoResultadoMinijuegoTablero = 0.0f;
                    estado = ESTADO_PARTIDA;
                }

                break;
            }

            if (
                resultado == nullptr ||
                !ResultadoMinijuegoFinalizado(*resultado)
            )
            {
                tiempoResultadoMinijuegoTablero = 0.0f;
                break;
            }

            tiempoResultadoMinijuegoTablero += deltaTime;

            bool confirmar =
                tiempoResultadoMinijuegoTablero >= 1.5f &&
                ConfirmarConParticipanteHumano(participantes);

            bool continuarAutomaticamente =
                tiempoResultadoMinijuegoTablero >= 7.0f;

            if (confirmar || continuarAutomaticamente)
            {
                // Un resultado inconsistente no se premia; la partida
                // continua igual que al salir del minijuego sin resultado.
                if (ValidarResultadoMinijuego(*resultado, participantes))
                {
                    partidaTablero.AplicarResultadoMinijuego(*resultado);
                }
                else
                {
                    TraceLog(
                        LOG_WARNING,
                        "Resultado de minijuego invalido: se descarto sin aplicar recompensas."
                    );
                }

                partidaTablero.ContinuarTrasMinijuego();
                zonaPruebas.modoCatalogo = false;
                zonaPruebas.modoTablero = false;
                tiempoResultadoMinijuegoTablero = 0.0f;
                estado = ESTADO_PARTIDA;
            }

            break;
        }

        case ESTADO_RESULTADO:
            break;
    }

    // Fundido corto desde negro al cambiar de pantalla. Tablero y ruleta
    // comparten escena, y el logo ya tiene su propia transicion.
    if (fundidoEntrada > 0.0f)
    {
        fundidoEntrada -= deltaTime / 0.28f;

        if (fundidoEntrada < 0.0f)
        {
            fundidoEntrada = 0.0f;
        }
    }

    if (estado != estadoAlEntrar)
    {
        const bool escenaCompartida =
            (estado == ESTADO_PARTIDA && estadoAlEntrar == ESTADO_RULETA_MINIJUEGO) ||
            (estado == ESTADO_RULETA_MINIJUEGO && estadoAlEntrar == ESTADO_PARTIDA);

        if (!escenaCompartida && estadoAlEntrar != ESTADO_LOGO)
        {
            fundidoEntrada = 1.0f;
        }
    }

    // Al dejar un minijuego (ronda de tablero o pruebas) se limpia el estado
    // visual global: si no, el tablero dibujaria el tema del ultimo
    // minijuego (por ejemplo la montana nevada de Pelotas).
    if (
        estado != estadoAlEntrar &&
        (estadoAlEntrar == ESTADO_MINIJUEGO ||
         estadoAlEntrar == ESTADO_ZONA_PRUEBAS)
    )
    {
        ReiniciarEstadoVisualMinijuegos();
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

            if (cargaMenuSolicitada && !menuPreparado)
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
            menuPrincipal.Dibujar();
            break;

        case ESTADO_CONFIGURACION:
            menuPrincipal.DibujarFondo();
            menuConfiguracion.Dibujar(
                config,
                resoluciones,
                opcionesFPS
            );
            break;

        case ESTADO_SELECCION_MODO:
            menuPrincipal.DibujarFondo();
            menuModoJuego.Dibujar();
            break;

        case ESTADO_TABLERO_VACIO:
            menuPrincipal.DibujarFondo();
            DibujarTableroVacio();
            break;

        case ESTADO_SELECCION_JUGADORES:
        {
            // La seleccion 3D dibuja su propia escena completa.

            seleccionPersonajes.Dibujar(
                participantes,
                MAX_PARTICIPANTES
            );

            bool modoMinijuegos =
                menuModoJuego.opcionSeleccionada ==
                MODO_JUEGO_MINIJUEGOS;

            if (modoMinijuegos && !participantes[0].activo)
            {
                const char* aviso =
                    "JUGADOR 1 DEBE UNIRSE PARA ELEGIR EL MINIJUEGO";

                // Entre las etiquetas de los personajes y los paneles de
                // jugador (que empiezan a 124 px de abajo en escala 720p).
                float escala = std::fmin(
                    GetScreenHeight() / 720.0f,
                    GetScreenWidth() / 1280.0f
                );
                int tamano = std::max(14, (int)(12.0f * escala));
                int anchoTexto = MeasureText(aviso, tamano);
                int x = GetScreenWidth() / 2 - anchoTexto / 2;
                int y = GetScreenHeight() - (int)(150.0f * escala);

                DrawRectangle(
                    x - 10,
                    y - 4,
                    anchoTexto + 20,
                    tamano + 8,
                    Fade(BLACK, 0.70f)
                );

                DrawText(
                    aviso,
                    x,
                    y,
                    tamano,
                    ORANGE
                );
            }

            break;
        }

        case ESTADO_SELECCION_TABLERO:
            menuPrincipal.DibujarFondo();
            seleccionTablero.Dibujar();
            break;

        case ESTADO_CONFIGURACION_PARTIDA:
            menuPrincipal.DibujarFondo();
            configuracionPartida.Dibujar(
                participantes,
                MAX_PARTICIPANTES
            );
            break;

        case ESTADO_INTRO_TABLERO:
            introTablero.Dibujar();
            break;

        case ESTADO_ORDEN_TURNO:
            ordenTurno.Dibujar(participantes);
            break;

        case ESTADO_RESULTADOS_PARTIDA:
            resultadosPartida.Dibujar(participantes);
            break;

        case ESTADO_SELECCION_MINIJUEGO:
            menuPrincipal.DibujarFondo();
            seleccionMinijuegos.Dibujar(participantes[0]);
            break;

        case ESTADO_ZONA_PRUEBAS:
            zonaPruebas.Dibujar();
            break;

        case ESTADO_PARTIDA:
            partidaTablero.Dibujar();

            if (confirmandoSalida)
            {
                DibujarConfirmacionSalida(opcionSalida, tiempoSalida);
            }

            break;

        case ESTADO_RULETA_MINIJUEGO:
            partidaTablero.Dibujar();
            ruletaMinijuegos.Dibujar();
            break;

        case ESTADO_MINIJUEGO:
        {
            zonaPruebas.Dibujar();

            const ResultadoMinijuego* resultado =
                zonaPruebas.ObtenerResultadoMinijuego();

            bool terminado =
                resultado != nullptr &&
                ResultadoMinijuegoFinalizado(*resultado);

            DrawRectangle(
                0,
                GetScreenHeight() - 58,
                GetScreenWidth(),
                58,
                Fade(BLACK, 0.82f)
            );

            const char* texto =
                terminado
                    ? TextFormat(
                        "RESULTADO | VUELTA AL TABLERO EN %d s (CONFIRMAR PARA SEGUIR YA)",
                        (int)std::ceil(std::fmax(0.0f, 7.0f - tiempoResultadoMinijuegoTablero))
                    )
                    : "MINIJUEGO DE RONDA | JUEGA HASTA EL FINAL PARA VOLVER AL TABLERO";

            DrawText(
                texto,
                GetScreenWidth() / 2 - MeasureText(texto, 18) / 2,
                GetScreenHeight() - 39,
                18,
                RAYWHITE
            );

            break;
        }

        case ESTADO_RESULTADO:
            ClearBackground(BLACK);
            break;
    }

    if (fundidoEntrada > 0.0f)
    {
        DrawRectangle(
            0,
            0,
            GetScreenWidth(),
            GetScreenHeight(),
            Fade(BLACK, fundidoEntrada)
        );
    }

    if (
        config.mostrarFPS &&
        estado != ESTADO_LOGO
    )
    {
        const char* texto = TextFormat("FPS: %d", GetFPS());

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
    GuardarConfiguracion(RUTA_CONFIGURACION_JUEGO, config);

    zonaPruebas.Descargar();
    DescargarVistaPreviaTableros();
    seleccionPersonajes.Descargar();
    audio.Descargar();
    pantallaLogo.Descargar();
    menuPrincipal.Descargar();
}
