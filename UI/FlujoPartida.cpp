#include "UI/FlujoPartida.h"

#include "Systems/Input.h"

#include <cmath>
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


static void DibujarCentrado(
    const char* texto,
    int centroX,
    int y,
    int tamano,
    Color color
)
{
    DrawText(
        texto,
        centroX - MeasureText(texto, tamano) / 2,
        y,
        tamano,
        color
    );
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


static void DibujarAyuda(
    const char* texto
)
{
    DibujarCentrado(
        texto,
        GetScreenWidth() / 2,
        GetScreenHeight() - 34,
        18,
        LIGHTGRAY
    );
}


static void DibujarTitulo(
    const char* texto,
    Color color
)
{
    DibujarCentrado(
        texto,
        GetScreenWidth() / 2,
        28,
        40,
        color
    );
}


static void DibujarVelo(
    float opacidad
)
{
    DrawRectangle(
        0,
        0,
        GetScreenWidth(),
        GetScreenHeight(),
        Fade(Color{ 10, 12, 18, 255 }, opacidad)
    );
}


static void DibujarBoton(
    Rectangle rect,
    const char* texto,
    bool seleccionado,
    Color acento,
    float alfa
)
{
    DrawRectangleRec(
        rect,
        Fade(
            seleccionado ? acento : Color{ 34, 38, 50, 255 },
            (seleccionado ? 0.95f : 0.85f) * alfa
        )
    );

    DrawRectangleLinesEx(
        rect,
        seleccionado ? 4.0f : 2.0f,
        Fade(seleccionado ? RAYWHITE : GRAY, alfa)
    );

    DibujarCentrado(
        texto,
        (int)(rect.x + rect.width / 2.0f),
        (int)(rect.y + rect.height / 2.0f - 11.0f),
        22,
        Fade(seleccionado ? BLACK : RAYWHITE, alfa)
    );
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
// SELECCION DE TABLERO
//==================================================

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
        audio.ReproducirSonido(SONIDO_UI_MOVER);
    }
}


void SeleccionTablero::Dibujar() const
{
    const int ancho = GetScreenWidth();
    const int alto = GetScreenHeight();

    const IdTablero id = ObtenerIdElegido();
    const Color tema = ObtenerColorTemaTablero(id);
    const float aparicion = Aparicion(tiempo, 0.0f, 0.30f);

    DibujarVelo(0.80f * aparicion);

    DibujarTitulo("ELEGI EL TABLERO", Fade(RAYWHITE, aparicion));

    Rectangle vista =
    {
        40.0f,
        100.0f,
        ancho * 0.56f,
        (float)(alto - 100 - 100)
    };

    DrawRectangleRec(vista, Fade(BLACK, 0.6f * aparicion));

    DibujarVistaPreviaTablero(id, vista, tiempo);

    DrawRectangleLinesEx(vista, 4.0f, Fade(tema, aparicion));

    const int total = ObtenerCantidadTableros();

    if (total > 1)
    {
        const int centroY = (int)(vista.y + vista.height / 2.0f);

        DrawText("<", (int)vista.x + 14, centroY - 20, 40, Fade(RAYWHITE, 0.9f));
        DrawText(
            ">",
            (int)(vista.x + vista.width) - 34,
            centroY - 20,
            40,
            Fade(RAYWHITE, 0.9f)
        );
    }

    // Panel de informacion.
    const int xInfo = (int)(vista.x + vista.width) + 30;
    const int anchoInfo = ancho - xInfo - 40;
    int y = 108;

    DrawText(ObtenerNombreTablero(id), xInfo, y, 36, tema);
    y += 56;

    y = DibujarParrafo(
        ObtenerHistoriaTablero(id),
        xInfo,
        y,
        anchoInfo,
        20,
        RAYWHITE
    );

    y += 14;
    DrawText("OBJETIVO", xInfo, y, 16, GRAY);
    y += 22;

    y = DibujarParrafo(
        ObtenerObjetivoTablero(id),
        xInfo,
        y,
        anchoInfo,
        20,
        GOLD
    );

    y += 14;
    DrawText("GIMMICK", xInfo, y, 16, GRAY);
    y += 22;

    y = DibujarParrafo(
        ObtenerGimmickTablero(id),
        xInfo,
        y,
        anchoInfo,
        20,
        RAYWHITE
    );

    y += 14;
    DrawText("DIFICULTAD", xInfo, y, 16, GRAY);
    y += 22;

    DrawText(ObtenerDificultadTablero(id), xInfo, y, 22, tema);

    // Indicador de pagina.
    const int separacionPuntos = 26;
    const int xPuntos =
        ancho / 2 - (total - 1) * separacionPuntos / 2;

    for (int i = 0; i < total; i++)
    {
        DrawCircle(
            xPuntos + i * separacionPuntos,
            alto - 66,
            i == indice ? 8.0f : 5.0f,
            i == indice ? tema : GRAY
        );
    }

    DibujarAyuda("IZQ / DER: CAMBIAR   |   ESPACIO / A: ELEGIR   |   ESC / B: ATRAS");
}


//==================================================
// CONFIGURACION DE PARTIDA
//==================================================

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

        audio.ReproducirSonido(SONIDO_UI_MOVER);
    }
}


void ConfiguracionPartida::Dibujar(
    const Participante participantes[],
    int cantidad
) const
{
    const int ancho = GetScreenWidth();
    const int alto = GetScreenHeight();
    const Color tema = ObtenerColorTemaTablero(tablero);
    const float aparicion = Aparicion(tiempo, 0.0f, 0.30f);

    DibujarVelo(0.80f * aparicion);

    DibujarTitulo("CANTIDAD DE RONDAS", Fade(RAYWHITE, aparicion));

    DibujarCentrado(
        ObtenerNombreTablero(tablero),
        ancho / 2,
        78,
        24,
        Fade(tema, aparicion)
    );

    static const char* nombres[CANTIDAD_OPCIONES_RONDAS] =
    {
        "CORTA",
        "MEDIA",
        "LARGA"
    };

    const float anchoTarjeta = 210.0f;
    const float altoTarjeta = 190.0f;
    const float separacion = 28.0f;

    const float total =
        CANTIDAD_OPCIONES_RONDAS * anchoTarjeta +
        (CANTIDAD_OPCIONES_RONDAS - 1) * separacion;

    const float x0 = (ancho - total) / 2.0f;
    const float yTarjetas = alto / 2.0f - altoTarjeta / 2.0f - 20.0f;

    for (int i = 0; i < CANTIDAD_OPCIONES_RONDAS; i++)
    {
        const bool elegida = i == opcionRondas;

        Rectangle tarjeta =
        {
            x0 + i * (anchoTarjeta + separacion),
            yTarjetas - (elegida ? 10.0f : 0.0f),
            anchoTarjeta,
            altoTarjeta + (elegida ? 20.0f : 0.0f)
        };

        DrawRectangleRec(
            tarjeta,
            Fade(elegida ? Color{ 40, 46, 62, 255 } : Color{ 26, 29, 40, 255 }, aparicion)
        );

        DrawRectangleLinesEx(
            tarjeta,
            elegida ? 5.0f : 2.0f,
            Fade(elegida ? tema : GRAY, aparicion)
        );

        DibujarCentrado(
            TextFormat("%d", OPCIONES_RONDAS_PARTIDA[i]),
            (int)(tarjeta.x + tarjeta.width / 2.0f),
            (int)tarjeta.y + 34,
            72,
            Fade(elegida ? RAYWHITE : LIGHTGRAY, aparicion)
        );

        DibujarCentrado(
            "RONDAS",
            (int)(tarjeta.x + tarjeta.width / 2.0f),
            (int)tarjeta.y + 118,
            20,
            Fade(GRAY, aparicion)
        );

        DibujarCentrado(
            nombres[i],
            (int)(tarjeta.x + tarjeta.width / 2.0f),
            (int)tarjeta.y + 148,
            22,
            Fade(elegida ? tema : LIGHTGRAY, aparicion)
        );
    }

    // Resumen de jugadores: humanos activos y bots que ocuparan el resto.
    const float anchoChip = 150.0f;
    const float totalChips =
        MAX_PARTICIPANTES * anchoChip + (MAX_PARTICIPANTES - 1) * 12.0f;

    const float xChips = (ancho - totalChips) / 2.0f;
    const float yChips = yTarjetas + altoTarjeta + 52.0f;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        const bool humano = i < cantidad && participantes[i].activo;

        Rectangle chip =
        {
            xChips + i * (anchoChip + 12.0f),
            yChips,
            anchoChip,
            52.0f
        };

        const Color color =
            humano ? participantes[i].color : GRAY;

        DrawRectangleRec(chip, Fade(color, 0.28f * aparicion));
        DrawRectangleLinesEx(chip, 2.0f, Fade(color, aparicion));

        DrawText(
            TextFormat("J%d", i + 1),
            (int)chip.x + 10,
            (int)chip.y + 8,
            22,
            Fade(RAYWHITE, aparicion)
        );

        DrawText(
            humano ? "HUMANO" : "BOT",
            (int)chip.x + 10,
            (int)chip.y + 32,
            16,
            Fade(humano ? RAYWHITE : LIGHTGRAY, aparicion)
        );
    }

    DibujarAyuda("IZQ / DER: CAMBIAR   |   ESPACIO / A: COMENZAR   |   ESC / B: ATRAS");
}


//==================================================
// INTRO DEL TABLERO
//==================================================

void IntroTablero::Inicializar(
    IdTablero idTablero,
    int cantidadRondas
)
{
    tablero = idTablero;
    rondas = cantidadRondas;
    tiempo = 0.0f;
    terminada = false;

    lector.Reiniciar();
}


void IntroTablero::Actualizar(
    float deltaTime,
    const Participante participantes[],
    int cantidad
)
{
    tiempo += deltaTime;

    EntradaFlujo entrada = lector.Leer(participantes, cantidad);

    if (tiempo >= DURACION)
    {
        terminada = true;
        return;
    }

    // Saltar solo despues del bloqueo, para que una pulsacion
    // heredada no se salte la presentacion.
    if (
        tiempo >= BLOQUEO_ENTRADA_FLUJO + 0.2f &&
        (entrada.confirmar || entrada.cancelar)
    )
    {
        terminada = true;
    }
}


void IntroTablero::Dibujar() const
{
    const int ancho = GetScreenWidth();
    const int alto = GetScreenHeight();
    const Color tema = ObtenerColorTemaTablero(tablero);

    // Vista del mapa a pantalla completa.
    DibujarVistaPreviaTablero(
        tablero,
        Rectangle{ 0.0f, 0.0f, (float)ancho, (float)alto },
        tiempo
    );

    // Degradado inferior para que el texto sea legible sobre el mapa.
    DrawRectangleGradientV(
        0,
        alto / 2 - 40,
        ancho,
        alto / 2 + 40,
        Fade(BLACK, 0.0f),
        Fade(BLACK, 0.88f)
    );

    const int x = 70;
    const int anchoTexto = ancho - 140;
    int y = alto / 2 + 10;

    const float aNombre = Aparicion(tiempo, 0.3f);
    const float aHistoria = Aparicion(tiempo, 1.0f);
    const float aObjetivo = Aparicion(tiempo, 2.2f);
    const float aGimmick = Aparicion(tiempo, 3.2f);

    DrawText(
        ObtenerNombreTablero(tablero),
        x,
        y,
        52,
        Fade(tema, aNombre)
    );

    DrawText(
        TextFormat("%d RONDAS", rondas),
        ancho - 70 - MeasureText(TextFormat("%d RONDAS", rondas), 24),
        y + 18,
        24,
        Fade(RAYWHITE, aNombre)
    );

    y += 70;

    y = DibujarParrafo(
        ObtenerHistoriaTablero(tablero),
        x,
        y,
        anchoTexto,
        22,
        Fade(RAYWHITE, aHistoria)
    );

    y += 10;

    DrawText(
        TextFormat("OBJETIVO: %s", ObtenerObjetivoTablero(tablero)),
        x,
        y,
        22,
        Fade(GOLD, aObjetivo)
    );

    y += 34;

    DrawText(
        TextFormat("GIMMICK: %s", ObtenerGimmickTablero(tablero)),
        x,
        y,
        22,
        Fade(SKYBLUE, aGimmick)
    );

    // Barra de progreso y ayuda.
    DrawRectangle(0, alto - 8, ancho, 8, Fade(BLACK, 0.6f));
    DrawRectangle(0, alto - 8, (int)(ancho * Limitar01(tiempo / DURACION)), 8, tema);

    DrawText(
        "ESPACIO / A: SALTAR",
        ancho - 70 - MeasureText("ESPACIO / A: SALTAR", 18),
        alto - 38,
        18,
        Fade(LIGHTGRAY, Aparicion(tiempo, 0.8f))
    );
}


//==================================================
// ORDEN DE TURNO
//==================================================

static const float INICIO_TIRADA_ORDEN = 0.7f;

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


void OrdenTurno::Dibujar(
    const Participante participantes[]
) const
{
    const int ancho = GetScreenWidth();
    const int alto = GetScreenHeight();
    const Color tema = ObtenerColorTemaTablero(tablero);

    DibujarVistaPreviaTablero(
        tablero,
        Rectangle{ 0.0f, 0.0f, (float)ancho, (float)alto },
        tiempo
    );

    DibujarVelo(0.72f);

    DibujarTitulo("ORDEN DE TURNO", tema);

    DibujarCentrado(
        revelado ? "EL DADO MAS ALTO JUEGA PRIMERO" : "TIRANDO LOS DADOS...",
        ancho / 2,
        78,
        22,
        LIGHTGRAY
    );

    const float tiempoRevelar = ObtenerTiempoRevelarOrden(cantidad);

    const float anchoTarjeta = 210.0f;
    const float altoTarjeta = 250.0f;
    const float separacion = 24.0f;

    const float total =
        cantidad * anchoTarjeta + (cantidad - 1) * separacion;

    const float x0 = (ancho - total) / 2.0f;
    const float y0 = alto / 2.0f - altoTarjeta / 2.0f + 10.0f;

    const float u =
        revelado
        ? Suavizar((tiempo - tiempoRevelar) / 0.6f)
        : 0.0f;

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

        const float posicion =
            (float)j + ((float)posicionFinal - (float)j) * u;

        const bool primero = posicionFinal == 0;

        Rectangle tarjeta =
        {
            x0 + posicion * (anchoTarjeta + separacion),
            y0,
            anchoTarjeta,
            altoTarjeta
        };

        const Participante& datos = participantes[participante];

        DrawRectangleRec(tarjeta, Color{ 24, 27, 36, 240 });

        DrawRectangle(
            (int)tarjeta.x,
            (int)tarjeta.y,
            (int)tarjeta.width,
            62,
            Fade(datos.color, 0.9f)
        );

        DrawText(
            TextFormat("J%d", participante + 1),
            (int)tarjeta.x + 12,
            (int)tarjeta.y + 8,
            30,
            BLACK
        );

        DrawText(
            ObtenerEtiquetaControl(datos),
            (int)tarjeta.x + 12,
            (int)tarjeta.y + 40,
            16,
            BLACK
        );

        // Dado.
        const float lado = 92.0f;

        Rectangle dadoRect =
        {
            tarjeta.x + tarjeta.width / 2.0f - lado / 2.0f,
            tarjeta.y + 84.0f,
            lado,
            lado
        };

        int valor = 0;

        if (tiempo >= ObtenerTiempoDetencionDado(j))
        {
            valor = dado[participante];
        }
        else if (tiempo >= INICIO_TIRADA_ORDEN)
        {
            valor = ((int)(tiempo * 22.0f) + participante * 3) % 10 + 1;
        }

        DrawRectangleRec(dadoRect, RAYWHITE);
        DrawRectangleLinesEx(dadoRect, 4.0f, BLACK);

        DibujarCentrado(
            valor > 0 ? TextFormat("%d", valor) : "?",
            (int)(dadoRect.x + lado / 2.0f),
            (int)dadoRect.y + 22,
            50,
            BLACK
        );

        if (u > 0.7f)
        {
            DibujarCentrado(
                TextFormat("TURNO %d", posicionFinal + 1),
                (int)(tarjeta.x + tarjeta.width / 2.0f),
                (int)tarjeta.y + 196,
                28,
                primero ? GOLD : RAYWHITE
            );
        }

        DrawRectangleLinesEx(
            tarjeta,
            primero && u > 0.7f ? 6.0f : 2.0f,
            primero && u > 0.7f ? GOLD : GRAY
        );
    }

    DibujarAyuda(
        revelado
        ? "ESPACIO / A: COMENZAR LA PARTIDA"
        : "ESPACIO / A: VER EL RESULTADO"
    );
}


//==================================================
// RESULTADOS DE LA PARTIDA
//==================================================

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

    const float tiempoMenu = ObtenerTiempoMenuResultados(cantidad);

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


static void DibujarCorona(
    float centroX,
    float yBase,
    float escala
)
{
    const float ancho = 46.0f * escala;
    const float alto = 26.0f * escala;
    const float izquierda = centroX - ancho / 2.0f;

    DrawRectangle(
        (int)izquierda,
        (int)(yBase - alto * 0.35f),
        (int)ancho,
        (int)(alto * 0.35f),
        GOLD
    );

    for (int i = 0; i < 3; i++)
    {
        const float x = izquierda + ancho * (0.17f + 0.33f * (float)i);

        DrawTriangle(
            Vector2{ x - ancho * 0.17f, yBase - alto * 0.35f },
            Vector2{ x + ancho * 0.17f, yBase - alto * 0.35f },
            Vector2{ x, yBase - alto },
            GOLD
        );
    }
}


void ResultadosPartida::Dibujar(
    const Participante participantes[]
) const
{
    const int ancho = GetScreenWidth();
    const int alto = GetScreenHeight();
    const Color tema = ObtenerColorTemaTablero(tablero);

    DibujarVistaPreviaTablero(
        tablero,
        Rectangle{ 0.0f, 0.0f, (float)ancho, (float)alto },
        tiempo * 0.5f
    );

    DibujarVelo(0.78f);

    DibujarTitulo("FIN DE LA PARTIDA", GOLD);

    const float tiempoGanador = ObtenerTiempoRevelarPuesto(0, cantidad);
    const float tiempoMenu = ObtenerTiempoMenuResultados(cantidad);

    // Ganador (o empate).
    if (tiempo >= tiempoGanador)
    {
        int empatados = 0;

        for (int k = 0; k < cantidad; k++)
        {
            if (puesto[k] == 1)
            {
                empatados++;
            }
        }

        const float pulso =
            1.0f + 0.04f * std::sin(tiempo * 5.0f);

        const char* texto =
            empatados > 1
            ? "EMPATE EN EL PRIMER PUESTO"
            : TextFormat("GANA JUGADOR %d", ranking[0] + 1);

        const int tamano = (int)(34.0f * pulso);

        DibujarCentrado(texto, ancho / 2, 84, tamano, RAYWHITE);
    }
    else
    {
        DibujarCentrado(
            ObtenerNombreTablero(tablero),
            ancho / 2,
            84,
            24,
            Fade(tema, 0.9f)
        );
    }

    // Podio: 2.o, 1.o, 3.o, 4.o de izquierda a derecha.
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

    const float anchoColumna = 180.0f;
    const float separacion = 28.0f;

    const float total =
        cantidadColumnas * anchoColumna +
        (cantidadColumnas - 1) * separacion;

    const float x0 = (ancho - total) / 2.0f;
    const float yBase = (float)alto - 170.0f;

    for (int pos = 0; pos < cantidadColumnas; pos++)
    {
        const int slot = columnas[pos];
        const int participante = ranking[slot];
        const Participante& datos = participantes[participante];

        const float inicio = ObtenerTiempoRevelarPuesto(slot, cantidad);
        const float crecimiento = Suavizar((tiempo - inicio) / 0.5f);

        if (crecimiento <= 0.0f)
        {
            continue;
        }

        const float alturaFinal =
            120.0f + (float)(cantidad - 1 - slot) * 40.0f;

        const float altura = alturaFinal * crecimiento;
        const float x = x0 + pos * (anchoColumna + separacion);
        const bool campeon = puesto[slot] == 1;

        Rectangle columna = { x, yBase - altura, anchoColumna, altura };

        DrawRectangleRec(columna, Fade(datos.color, 0.88f));
        DrawRectangleLinesEx(columna, campeon ? 5.0f : 2.0f, campeon ? GOLD : BLACK);

        const float conteo = Limitar01((tiempo - inicio) / 0.8f);

        if (crecimiento > 0.9f)
        {
            const int centro = (int)(x + anchoColumna / 2.0f);
            const int arriba = (int)columna.y;

            DibujarCentrado(TextFormat("%d", puesto[slot]), centro, arriba + 8, 48, BLACK);

            DrawText(
                TextFormat("TROFEOS %d", (int)((float)trofeos[participante] * conteo + 0.5f)),
                (int)x + 12,
                arriba + 62,
                18,
                BLACK
            );

            DrawText(
                TextFormat("MONEDAS %d", (int)((float)monedas[participante] * conteo + 0.5f)),
                (int)x + 12,
                arriba + 86,
                18,
                BLACK
            );

            DrawText(
                ObtenerEtiquetaControl(datos),
                (int)x + 12,
                arriba + 108,
                14,
                Fade(BLACK, 0.75f)
            );
        }

        // Ficha del jugador sobre la columna.
        const float salto =
            campeon && tiempo >= tiempoGanador
            ? std::fabs(std::sin(tiempo * 4.0f)) * 10.0f
            : 0.0f;

        const float centroX = x + anchoColumna / 2.0f;
        const float centroY = columna.y - 36.0f - salto;

        DrawCircle((int)centroX, (int)centroY, 30.0f, datos.color);
        DrawCircleLines((int)centroX, (int)centroY, 30.0f, campeon ? GOLD : RAYWHITE);

        DibujarCentrado(
            TextFormat("J%d", participante + 1),
            (int)centroX,
            (int)centroY - 12,
            24,
            BLACK
        );

        if (campeon && tiempo >= tiempoGanador)
        {
            DibujarCorona(centroX, centroY - 34.0f, 1.0f);
        }
    }

    // Confeti del ganador.
    if (tiempo >= tiempoGanador)
    {
        const float tc = tiempo - tiempoGanador;

        const Color paleta[5] = { GOLD, SKYBLUE, PINK, LIME, ORANGE };

        for (int i = 0; i < 90; i++)
        {
            const float rx = std::fmod((float)i * 0.6180339f, 1.0f);
            const float velocidad = 90.0f + std::fmod((float)i * 37.0f, 120.0f);
            const float y = std::fmod(tc * velocidad + (float)i * 53.0f, (float)alto + 40.0f) - 20.0f;
            const float x = rx * (float)ancho + std::sin(tc * 2.0f + (float)i) * 22.0f;

            DrawRectanglePro(
                Rectangle{ x, y, 9.0f, 5.0f },
                Vector2{ 4.5f, 2.5f },
                tc * 180.0f + (float)i * 40.0f,
                Fade(paleta[i % 5], 0.9f)
            );
        }
    }

    // Menu de opciones.
    if (tiempo >= tiempoMenu)
    {
        const float aparicion = Aparicion(tiempo, tiempoMenu, 0.4f);

        static const char* textos[CANTIDAD_OPCIONES] =
        {
            "REVANCHA",
            "NUEVA PARTIDA",
            "VOLVER AL HUB"
        };

        static const char* descripciones[CANTIDAD_OPCIONES] =
        {
            "MISMO TABLERO, JUGADORES Y RONDAS",
            "ELEGIR JUGADORES Y TABLERO DE NUEVO",
            "VOLVER AL MENU PRINCIPAL"
        };

        const float anchoBoton = 250.0f;
        const float altoBoton = 52.0f;
        const float espacio = 20.0f;

        const float totalBotones =
            CANTIDAD_OPCIONES * anchoBoton + (CANTIDAD_OPCIONES - 1) * espacio;

        const float xBotones = (ancho - totalBotones) / 2.0f;

        for (int i = 0; i < CANTIDAD_OPCIONES; i++)
        {
            DibujarBoton(
                Rectangle
                {
                    xBotones + i * (anchoBoton + espacio),
                    (float)alto - 118.0f,
                    anchoBoton,
                    altoBoton
                },
                textos[i],
                i == opcion,
                tema,
                aparicion
            );
        }

        DibujarCentrado(
            descripciones[opcion],
            ancho / 2,
            alto - 56,
            18,
            Fade(LIGHTGRAY, aparicion)
        );
    }
    else
    {
        DibujarAyuda("ESPACIO / A: SALTAR");
    }
}
