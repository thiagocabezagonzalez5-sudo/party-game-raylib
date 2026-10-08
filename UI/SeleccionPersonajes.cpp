#include "UI/SeleccionPersonajes.h"

#include "UI/SeleccionPersonajes3D.h"
#include "Systems/Input.h"

#include <cmath>
#include <cstdio>
#include <cstring>


//==================================================
// COLORES DE JUGADORES
//==================================================

static Color ObtenerColorJugador(
    int indice
)
{
    switch (indice)
    {
        case 0:
            return RED;

        case 1:
            return BLUE;

        case 2:
            return GREEN;

        case 3:
            return GOLD;
    }

    return WHITE;
}


//==================================================
// UTILIDADES
//==================================================

static int LimitarCircular(
    int valor,
    int minimo,
    int maximo
)
{
    if (valor < minimo)
    {
        return maximo;
    }

    if (valor > maximo)
    {
        return minimo;
    }

    return valor;
}


static void MoverCursor(
    JugadorSeleccion& jugador,
    int delta
)
{
    jugador.cursorPersonaje =
        LimitarCircular(
            jugador.cursorPersonaje + delta,
            0,
            MAX_PERSONAJES_SELECCION - 1
        );
}


static float Suavizar(
    float actual,
    float objetivo,
    float velocidad,
    float deltaTime
)
{
    float paso = velocidad * deltaTime;

    if (paso > 1.0f)
    {
        paso = 1.0f;
    }

    return actual + (objetivo - actual) * paso;
}


// Retardo (segundos) antes de que un bot "elija" su personaje.
static float RetardoBot(
    int indice
)
{
    return 0.45f + 0.32f * indice;
}


static float RevelacionBot(
    float tiempoComoBot,
    int indice
)
{
    float t = (tiempoComoBot - RetardoBot(indice)) / 0.25f;

    if (t < 0.0f)
    {
        return 0.0f;
    }

    return t > 1.0f ? 1.0f : t;
}


//==================================================
// CONEXIONES
//==================================================

static int ContarParticipantesActivos(
    const Participante participantes[],
    int cantidadMaxima
)
{
    int cantidad = 0;

    for (
        int i = 0;
        i < cantidadMaxima;
        i++
    )
    {
        if (participantes[i].activo)
        {
            cantidad++;
        }
    }

    return cantidad;
}


//==================================================
// TODOS LISTOS
//==================================================

static void ActualizarTodosListos(
    SeleccionPersonajes& seleccion,
    const Participante participantes[],
    int cantidadMaxima
)
{
    int activos = ContarParticipantesActivos(
        participantes,
        cantidadMaxima
    );

    bool activosPreparados =
        EsCantidadParticipantesValida(activos);

    for (
        int i = 0;
        i < cantidadMaxima;
        i++
    )
    {
        if (
            participantes[i].activo &&
            (
                !participantes[i].conectado ||
                !seleccion.jugadores[i].listo
            )
        )
        {
            activosPreparados = false;
        }
    }

    seleccion.todosListos =
        activosPreparados;
}


//==================================================
// INICIALIZAR
//==================================================

void SeleccionPersonajes::Inicializar(
    Participante participantes[],
    int cantidadMaxima
)
{
    //------------------------------
    // PERSONAJES REALES DEL JUEGO
    //------------------------------

    personajes[0].nombre = "TUNG TUNG";
    personajes[0].color = ORANGE;

    personajes[1].nombre = "PERSONAJE 2";
    personajes[1].color = SKYBLUE;

    personajes[2].nombre = "PERSONAJE 3";
    personajes[2].color = LIME;

    personajes[3].nombre = "PERSONAJE 4";
    personajes[3].color = VIOLET;

    // El modelo 3D es el compartido de main.cpp: no hay nada que cargar.
    recursosCargados =
        true;


    //------------------------------
    // REINICIAR JUGADORES
    //------------------------------

    for (
        int i = 0;
        i < cantidadMaxima;
        i++
    )
    {
        participantes[i].activo =
            false;

        participantes[i].personajeSeleccionado =
            -1;

        participantes[i].color =
            ObtenerColorJugador(i);

        jugadores[i].cursorPersonaje =
            i %
            MAX_PERSONAJES_SELECCION;

        jugadores[i].listo =
            false;

        jugadores[i].bloqueoHorizontal =
            false;

        jugadores[i].bloqueoVertical =
            false;

        tiempoComoBot[i] =
            0.0f;

        avisoDesconexion[i] =
            0.0f;
    }

    for (
        int i = 0;
        i < MAX_PERSONAJES_SELECCION;
        i++
    )
    {
        foco[i] =
            0.0f;

        tiempoReaccion[i] =
            DURACION_REACCION;
    }

    volverAlMenu =
        false;

    todosListos =
        false;

    iniciarPartida =
        false;

    alphaEntrada =
        0.0f;

    tiempoEscena =
        0.0f;

    tiempoTodosListos =
        0.0f;

    sonidoMover =
        false;

    sonidoConfirmar =
        false;

    sonidoCancelar =
        false;
}


//==================================================
// ACTUALIZAR
//==================================================

void SeleccionPersonajes::Actualizar(
    float deltaTime,
    Participante participantes[],
    int cantidadMaxima
)
{
    sonidoMover =
        false;

    sonidoConfirmar =
        false;

    sonidoCancelar =
        false;

    tiempoEscena +=
        deltaTime;


    //------------------------------
    // FADE
    //------------------------------

    if (
        alphaEntrada <
        1.0f
    )
    {
        alphaEntrada +=
            deltaTime /
            DURACION_ENTRADA;

        if (
            alphaEntrada >
            1.0f
        )
        {
            alphaEntrada =
                1.0f;
        }
    }


    //------------------------------
    // CONEXIONES
    //------------------------------

    ActualizarConexionesParticipantes(
        participantes,
        cantidadMaxima
    );


    //------------------------------
    // ANIMACION (estado visual)
    //------------------------------

    for (
        int c = 0;
        c < MAX_PERSONAJES_SELECCION;
        c++
    )
    {
        bool apuntado = false;

        for (
            int i = 0;
            i < cantidadMaxima;
            i++
        )
        {
            if (
                participantes[i].activo &&
                participantes[i].conectado &&
                jugadores[i].cursorPersonaje == c
            )
            {
                apuntado = true;
            }
        }

        foco[c] =
            Suavizar(
                foco[c],
                apuntado ? 1.0f : 0.0f,
                9.0f,
                deltaTime
            );

        if (tiempoReaccion[c] < DURACION_REACCION)
        {
            tiempoReaccion[c] +=
                deltaTime;
        }
    }

    for (
        int i = 0;
        i < cantidadMaxima;
        i++
    )
    {
        if (participantes[i].activo)
        {
            tiempoComoBot[i] =
                0.0f;
        }
        else
        {
            tiempoComoBot[i] +=
                deltaTime;
        }

        if (avisoDesconexion[i] > 0.0f)
        {
            avisoDesconexion[i] -=
                deltaTime;
        }
    }

    if (todosListos)
    {
        tiempoTodosListos +=
            deltaTime;
    }
    else
    {
        tiempoTodosListos =
            0.0f;
    }


    //------------------------------
    // BLOQUEO DE ENTRADA AL ENTRAR
    //------------------------------
    //
    // Mientras dura el fundido de entrada se ignora todo: la pulsacion
    // que abrio esta pantalla (o un boton mantenido) no debe unir a un
    // jugador ni saltar a la pantalla siguiente.

    if (alphaEntrada < 1.0f)
    {
        return;
    }


    //------------------------------
    // CONTROLES DESCONECTADOS
    //------------------------------
    //
    // Un mando que se desconecta libera su puesto: no queda ningun
    // jugador "listo" sin control que bloquee el inicio.

    int cantidadActivos =
        0;

    for (
        int i = 0;
        i < cantidadMaxima;
        i++
    )
    {
        if (
            participantes[i].activo &&
            !participantes[i].conectado
        )
        {
            participantes[i].activo =
                false;

            participantes[i].personajeSeleccionado =
                -1;

            jugadores[i].listo =
                false;

            jugadores[i].bloqueoHorizontal =
                false;

            jugadores[i].bloqueoVertical =
                false;

            avisoDesconexion[i] =
                3.5f;
        }

        if (participantes[i].activo)
        {
            cantidadActivos++;
        }
    }


    //------------------------------
    // VOLVER
    //------------------------------

    if (IsKeyPressed(KEY_ESCAPE))
    {
        volverAlMenu =
            true;

        return;
    }


    //==================================================
    // JUGADORES
    //==================================================

    for (
        int i = 0;
        i < cantidadMaxima;
        i++
    )
    {
        JugadorSeleccion& jugador =
            jugadores[i];

        Participante& participante =
            participantes[i];

        if (!participante.conectado)
        {
            continue;
        }

        InputSeleccionParticipante entrada =
            LeerInputSeleccionParticipante(
                participante
            );

        //------------------------------
        // UNIRSE / VOLVER
        //------------------------------

        if (!participante.activo)
        {
            if (entrada.confirmar)
            {
                participante.activo =
                    true;

                jugador.listo =
                    false;

                // El stick mantenido al unirse no mueve el cursor.
                jugador.bloqueoHorizontal =
                    true;

                jugador.bloqueoVertical =
                    true;

                avisoDesconexion[i] =
                    0.0f;

                sonidoConfirmar =
                    true;
            }
            else if (
                entrada.cancelar &&
                cantidadActivos == 0
            )
            {
                // Nadie se unio: cancelar vuelve al HUB.
                volverAlMenu =
                    true;

                return;
            }

            continue;
        }


        //------------------------------
        // MOVER CURSOR
        //------------------------------
        //
        // El teclado dispara una vez por pulsacion; el stick de un
        // gamepad usa bloqueos para que un empuje sea un solo paso.

        bool esGamepad =
            participante.control == CONTROL_GAMEPAD;

        if (!jugador.listo)
        {
            int paso = 0;

            if (entrada.izquierda && (!esGamepad || !jugador.bloqueoHorizontal))
            {
                paso = -1;
                jugador.bloqueoHorizontal = true;
            }
            else if (entrada.derecha && (!esGamepad || !jugador.bloqueoHorizontal))
            {
                paso = 1;
                jugador.bloqueoHorizontal = true;
            }
            else if (entrada.arriba && (!esGamepad || !jugador.bloqueoVertical))
            {
                paso = -1;
                jugador.bloqueoVertical = true;
            }
            else if (entrada.abajo && (!esGamepad || !jugador.bloqueoVertical))
            {
                paso = 1;
                jugador.bloqueoVertical = true;
            }

            if (paso != 0)
            {
                MoverCursor(
                    jugador,
                    paso
                );

                sonidoMover =
                    true;
            }
        }

        if (!entrada.izquierda && !entrada.derecha)
        {
            jugador.bloqueoHorizontal =
                false;
        }

        if (!entrada.arriba && !entrada.abajo)
        {
            jugador.bloqueoVertical =
                false;
        }


        //------------------------------
        // CONFIRMAR
        //------------------------------

        if (entrada.confirmar)
        {
            if (!jugador.listo)
            {
                tiempoReaccion[jugador.cursorPersonaje] =
                    0.0f;

                sonidoConfirmar =
                    true;
            }

            jugador.listo =
                true;

            participante.personajeSeleccionado =
                jugador.cursorPersonaje;
        }


        //------------------------------
        // CANCELAR
        //------------------------------

        if (entrada.cancelar)
        {
            sonidoCancelar =
                true;

            if (jugador.listo)
            {
                jugador.listo =
                    false;
            }
            else
            {
                participante.activo =
                    false;

                participante.personajeSeleccionado =
                    -1;
            }
        }
    }


    ActualizarTodosListos(
        *this,
        participantes,
        cantidadMaxima
    );

    iniciarPartida =
        todosListos;
}


//==================================================
// DIBUJO 2D: AYUDAS
//==================================================

// Escala unica de la UI: sigue la ventana real (alto y ancho), de modo que
// nada se sale de pantalla ni en ventanas bajas o estrechas.
static float EscalaUI()
{
    float porAlto = GetScreenHeight() / 720.0f;
    float porAncho = GetScreenWidth() / 1280.0f;

    return porAlto < porAncho ? porAlto : porAncho;
}


static int Px(
    float valor
)
{
    return (int)(valor * EscalaUI() + 0.5f);
}


static void DibujarTextoSombra(
    const char* texto,
    int x,
    int y,
    int tamano,
    Color color
)
{
    int sombra = tamano / 14 + 1;

    DrawText(
        texto,
        x + sombra,
        y + sombra,
        tamano,
        Fade(BLACK, 0.55f * (color.a / 255.0f))
    );

    DrawText(
        texto,
        x,
        y,
        tamano,
        color
    );
}


static void DibujarTextoCentrado(
    const char* texto,
    int centroX,
    int y,
    int tamano,
    Color color
)
{
    DibujarTextoSombra(
        texto,
        centroX - MeasureText(texto, tamano) / 2,
        y,
        tamano,
        color
    );
}


// Etiqueta redondeada con texto centrado; devuelve su ancho.
static int DibujarEtiqueta(
    const char* texto,
    int x,
    int y,
    int tamano,
    Color fondo,
    Color colorTexto
)
{
    int margen = tamano / 2;
    int ancho = MeasureText(texto, tamano) + margen * 2;
    int alto = tamano + tamano / 2;

    DrawRectangleRounded(
        Rectangle{ (float)x, (float)y, (float)ancho, (float)alto },
        0.45f,
        8,
        fondo
    );

    DrawText(
        texto,
        x + margen,
        y + tamano / 4,
        tamano,
        colorTexto
    );

    return ancho;
}


//==================================================
// DIBUJO 2D: ETIQUETAS SOBRE LA ESCENA
//==================================================

static void DibujarEtiquetasEscena(
    const SeleccionPersonajes& seleccion,
    const Participante participantes[],
    int cantidadMaxima,
    const PersonajeEscena3D escena[],
    Camera3D camara
)
{
    for (
        int c = 0;
        c < MAX_PERSONAJES_SELECCION;
        c++
    )
    {
        Vector3 pedestal =
            ObtenerPosicionPedestalSeleccion(c);

        float foco =
            seleccion.foco[c];

        //------------------------------
        // NOMBRE (placa bajo el pedestal)
        //------------------------------

        Vector2 ancla =
            GetWorldToScreen(
                Vector3{ pedestal.x, 0.12f, 1.9f },
                camara
            );

        int tamanoNombre =
            Px(15.0f + 3.0f * foco);

        const char* nombre =
            seleccion.personajes[c].nombre;

        int anchoNombre =
            MeasureText(nombre, tamanoNombre);

        int tamanoBot = Px(11.0f);

        int anchoBot =
            escena[c].bot
            ? MeasureText("BOT", tamanoBot) + tamanoBot + Px(6.0f)
            : 0;

        int placaAncho =
            anchoNombre + Px(24.0f) + anchoBot;

        int placaAlto =
            tamanoNombre + Px(10.0f);

        Rectangle placa =
        {
            ancla.x - placaAncho * 0.5f,
            ancla.y - placaAlto * 0.5f,
            (float)placaAncho,
            (float)placaAlto
        };

        DrawRectangleRounded(
            placa,
            0.4f,
            8,
            Fade(Color{ 22, 14, 40, 255 }, 0.60f + 0.25f * foco)
        );

        DrawRectangleRoundedLines(
            placa,
            0.4f,
            8,
            Fade(
                seleccion.personajes[c].color,
                0.55f + 0.45f * foco
            )
        );

        DrawText(
            nombre,
            (int)placa.x + Px(12.0f),
            (int)(placa.y + (placa.height - tamanoNombre) * 0.5f),
            tamanoNombre,
            Fade(RAYWHITE, 0.80f + 0.20f * foco)
        );

        if (escena[c].bot)
        {
            DibujarEtiqueta(
                "BOT",
                (int)placa.x + Px(12.0f) + anchoNombre + Px(6.0f),
                (int)(placa.y + (placa.height - tamanoBot * 1.5f) * 0.5f),
                tamanoBot,
                Fade(Color{ 120, 122, 138, 255 }, 0.95f),
                RAYWHITE
            );
        }

        //------------------------------
        // CHIPS J# SOBRE LA CABEZA
        //------------------------------

        int cursores[MAX_JUGADORES_SELECCION];
        int cantidadCursores = 0;

        for (
            int i = 0;
            i < cantidadMaxima;
            i++
        )
        {
            if (
                participantes[i].activo &&
                participantes[i].conectado &&
                seleccion.jugadores[i].cursorPersonaje == c
            )
            {
                cursores[cantidadCursores] = i;
                cantidadCursores++;
            }
        }

        if (cantidadCursores == 0)
        {
            continue;
        }

        float altura =
            ObtenerAlturaCabezaSeleccion(escena[c]);

        Vector2 cabeza =
            GetWorldToScreen(
                Vector3{ pedestal.x, altura + 0.35f, pedestal.z },
                camara
            );

        int chipTamano = Px(15.0f);
        int chipAlto = chipTamano + Px(8.0f);
        int separacion = Px(4.0f);

        int anchos[MAX_JUGADORES_SELECCION];
        int total = 0;

        for (
            int k = 0;
            k < cantidadCursores;
            k++
        )
        {
            anchos[k] =
                seleccion.jugadores[cursores[k]].listo
                ? Px(84.0f)
                : Px(36.0f);

            total += anchos[k] + (k > 0 ? separacion : 0);
        }

        int x0 =
            (int)cabeza.x - total / 2;

        int y0 =
            (int)cabeza.y - chipAlto - Px(8.0f);

        for (
            int k = 0;
            k < cantidadCursores;
            k++
        )
        {
            int jugadorIndice = cursores[k];

            bool listo =
                seleccion.jugadores[jugadorIndice].listo;

            Color color =
                ObtenerColorJugador(jugadorIndice);

            int xChip = x0;

            for (int previo = 0; previo < k; previo++)
            {
                xChip += anchos[previo] + separacion;
            }

            Rectangle chip =
            {
                (float)xChip,
                (float)y0,
                (float)anchos[k],
                (float)chipAlto
            };

            DrawRectangleRounded(
                chip,
                0.5f,
                8,
                listo ? color : Fade(Color{ 20, 14, 36, 255 }, 0.88f)
            );

            DrawRectangleRoundedLines(
                chip,
                0.5f,
                8,
                listo ? WHITE : color
            );

            // Flechita hacia el personaje.
            float centroChip = chip.x + chip.width * 0.5f;
            float base = chip.y + chip.height - 1.0f;

            DrawTriangle(
                Vector2{ centroChip - Px(5.0f), base },
                Vector2{ centroChip, base + Px(7.0f) },
                Vector2{ centroChip + Px(5.0f), base },
                color
            );

            const char* texto =
                listo
                ? TextFormat("J%d LISTO", jugadorIndice + 1)
                : TextFormat("J%d", jugadorIndice + 1);

            DrawText(
                texto,
                (int)centroChip - MeasureText(texto, chipTamano) / 2,
                (int)(chip.y + (chip.height - chipTamano) * 0.5f),
                chipTamano,
                listo ? Color{ 255, 255, 255, 255 } : color
            );
        }
    }
}


//==================================================
// DIBUJO 2D: PANELES DE JUGADOR
//==================================================

static Color AclararColor(Color c, float t)
{
    return Color{
        (unsigned char)(c.r + (255 - c.r) * t),
        (unsigned char)(c.g + (255 - c.g) * t),
        (unsigned char)(c.b + (255 - c.b) * t),
        255
    };
}


static void DibujarPanelJugador(
    const SeleccionPersonajes& seleccion,
    const Participante& participante,
    int indice,
    Rectangle panel
)
{
    const JugadorSeleccion& jugador =
        seleccion.jugadores[indice];

    Color color =
        ObtenerColorJugador(indice);

    float t =
        seleccion.tiempoEscena;

    int xTexto = (int)panel.x + Px(14.0f);
    int xDerecha = (int)(panel.x + panel.width) - Px(10.0f);

    bool conectado = participante.conectado;
    bool activo = participante.activo && conectado;

    float opacidad =
        conectado ? 1.0f : 0.60f;

    DrawRectangleRounded(
        panel,
        0.14f,
        8,
        Fade(Color{ 20, 12, 38, 255 }, 0.84f * opacidad)
    );

    if (activo)
    {
        DrawRectangleRounded(
            panel,
            0.14f,
            8,
            Fade(color, 0.14f)
        );
    }

    // Banda de color a la izquierda.
    DrawRectangleRounded(
        Rectangle{ panel.x, panel.y, (float)Px(7.0f), panel.height },
        0.8f,
        6,
        Fade(color, opacidad)
    );

    DrawRectangleRoundedLines(
        panel,
        0.14f,
        8,
        Fade(
            activo && jugador.listo ? GOLD : color,
            (activo ? 1.0f : 0.45f) * opacidad
        )
    );

    //------------------------------
    // FILA 1: J# Y HUMANO / BOT
    //------------------------------

    DibujarTextoSombra(
        TextFormat("J%d", indice + 1),
        xTexto,
        (int)panel.y + Px(7.0f),
        Px(24.0f),
        Fade(AclararColor(color, 0.35f), opacidad)
    );

    int tamanoTag = Px(12.0f);
    const char* tag = activo ? "HUMANO" : "BOT";

    int anchoTag =
        MeasureText(tag, tamanoTag) + tamanoTag;

    DibujarEtiqueta(
        tag,
        xDerecha - anchoTag,
        (int)panel.y + Px(9.0f),
        tamanoTag,
        activo
            ? Fade(color, 0.85f)
            : Fade(Color{ 110, 112, 128, 255 }, 0.9f * opacidad),
        activo && indice == 3 ? BLACK : RAYWHITE
    );

    // Dispositivo, junto al J#.
    const char* dispositivo =
        conectado
        ? ObtenerNombreControlParticipante(participante)
        : "SIN CONTROL";

    DrawText(
        dispositivo,
        xTexto + Px(38.0f),
        (int)panel.y + Px(14.0f),
        Px(12.0f),
        Fade(LIGHTGRAY, 0.9f * opacidad)
    );

    int yFila3 = (int)panel.y + Px(42.0f);

    //------------------------------
    // SIN CONTROL
    //------------------------------

    if (!conectado)
    {
        bool aviso =
            seleccion.avisoDesconexion[indice] > 0.0f;

        DrawText(
            aviso ? "CONTROL DESCONECTADO" : "NO CONECTADO",
            xTexto,
            yFila3,
            Px(15.0f),
            aviso
                ? Fade(Color{ 255, 120, 100, 255 }, 0.65f + 0.35f * std::sin(t * 9.0f))
                : Fade(LIGHTGRAY, 0.75f)
        );

        float revelado =
            RevelacionBot(seleccion.tiempoComoBot[indice], indice);

        DrawText(
            TextFormat("SERA BOT: %s", seleccion.personajes[indice].nombre),
            xTexto,
            yFila3 + Px(22.0f),
            Px(12.0f),
            Fade(GRAY, revelado)
        );

        return;
    }

    //------------------------------
    // SIN UNIRSE: INVITACION
    //------------------------------

    if (!participante.activo)
    {
        float pulso =
            0.5f + 0.5f * std::sin(t * 4.5f + indice);

        DrawText(
            TextFormat("%s PARA UNIRTE", ObtenerTextoBotonPrincipal(participante)),
            xTexto,
            yFila3 - Px(2.0f),
            Px(16.0f),
            Fade(RAYWHITE, 0.7f + 0.3f * pulso)
        );

        float revelado =
            RevelacionBot(seleccion.tiempoComoBot[indice], indice);

        DrawText(
            TextFormat("MIENTRAS: BOT %s", seleccion.personajes[indice].nombre),
            xTexto,
            yFila3 + Px(22.0f),
            Px(12.0f),
            Fade(GRAY, revelado)
        );

        return;
    }

    //------------------------------
    // UNIDO: PERSONAJE Y ESTADO
    //------------------------------

    int cursor =
        jugador.cursorPersonaje;

    DrawCircle(
        xTexto + Px(6.0f),
        yFila3 + Px(10.0f),
        (float)Px(6.0f),
        seleccion.personajes[cursor].color
    );

    DrawText(
        seleccion.personajes[cursor].nombre,
        xTexto + Px(20.0f),
        yFila3,
        Px(19.0f),
        RAYWHITE
    );

    DrawText(
        jugador.listo ? "LISTO!" : "ELIGIENDO...",
        xTexto,
        yFila3 + Px(27.0f),
        Px(15.0f),
        jugador.listo
            ? GOLD
            : Fade(LIGHTGRAY, 0.9f)
    );
}


//==================================================
// DIBUJO 2D: AYUDA SEGUN LOS CONTROLES EN USO
//==================================================

static void AgregarAyuda(
    char* destino,
    int capacidad,
    const char* texto
)
{
    int usado = (int)std::strlen(destino);

    std::snprintf(
        destino + usado,
        (size_t)(capacidad - usado),
        "%s%s",
        usado > 0 ? "   |   " : "",
        texto
    );
}


static void ComponerAyuda(
    const Participante participantes[],
    int cantidadMaxima,
    char* destino,
    int capacidad
)
{
    bool completo = false;
    bool wasd = false;
    bool flechas = false;
    bool mando = false;

    for (
        int i = 0;
        i < cantidadMaxima;
        i++
    )
    {
        if (!participantes[i].conectado)
        {
            continue;
        }

        switch (participantes[i].control)
        {
            case CONTROL_TECLADO_COMPLETO:
                completo = true;
                break;

            case CONTROL_TECLADO_WASD:
                wasd = true;
                break;

            case CONTROL_TECLADO_FLECHAS:
                flechas = true;
                break;

            case CONTROL_GAMEPAD:
                mando = true;
                break;

            case CONTROL_NINGUNO:
                break;
        }
    }

    destino[0] = '\0';

    if (completo)
    {
        AgregarAyuda(destino, capacidad, "TECLADO: WASD o FLECHAS, ESPACIO o ENTER = OK, RETROCESO o SHIFT DER = ATRAS");
    }

    if (wasd)
    {
        AgregarAyuda(destino, capacidad, "WASD + ESPACIO (ATRAS: RETROCESO)");
    }

    if (flechas)
    {
        AgregarAyuda(destino, capacidad, "FLECHAS + ENTER (ATRAS: SHIFT DER)");
    }

    if (mando)
    {
        AgregarAyuda(destino, capacidad, "MANDO: STICK/CRUZ + A (ATRAS: B)");
    }

    if (destino[0] == '\0')
    {
        AgregarAyuda(destino, capacidad, "CONFIRMA CON TU TECLA O BOTON PARA UNIRTE");
    }

    AgregarAyuda(destino, capacidad, "ESC: HUB");
}


//==================================================
// DIBUJAR
//==================================================

void SeleccionPersonajes::Dibujar(
    const Participante participantes[],
    int cantidadMaxima
) const
{
    int ancho = GetScreenWidth();
    int alto = GetScreenHeight();

    //------------------------------
    // DATOS PARA LA ESCENA
    //------------------------------

    PersonajeEscena3D escena[MAX_PERSONAJES_SELECCION];

    for (
        int c = 0;
        c < MAX_PERSONAJES_SELECCION;
        c++
    )
    {
        PersonajeEscena3D& visual = escena[c];

        visual.color =
            personajes[c].color;

        visual.foco =
            foco[c];

        float progreso =
            tiempoReaccion[c] / DURACION_REACCION;

        if (progreso < 1.0f)
        {
            visual.progresoSalto =
                progreso;

            float salida =
                1.0f - (1.0f - progreso) * (1.0f - progreso);

            visual.giroExtra =
                360.0f * salida;
        }

        for (
            int i = 0;
            i < cantidadMaxima;
            i++
        )
        {
            if (
                participantes[i].activo &&
                participantes[i].conectado &&
                jugadores[i].cursorPersonaje == c &&
                visual.cantidadAros < ESCENA_SELECCION_MAX_AROS
            )
            {
                visual.aros[visual.cantidadAros] =
                    ObtenerColorJugador(i);

                visual.cantidadAros++;

                if (jugadores[i].listo)
                {
                    visual.listo = true;
                }
            }
        }

        // Un bot ocupa el personaje con el mismo numero que su puesto.
        if (
            c < cantidadMaxima &&
            !participantes[c].activo &&
            RevelacionBot(tiempoComoBot[c], c) >= 0.5f
        )
        {
            visual.bot = true;
        }
    }

    float confeti =
        todosListos
        ? (tiempoTodosListos < 3.0f ? 1.0f : 0.5f)
        : 0.0f;

    DibujarEscenaSeleccion(
        tiempoEscena,
        escena,
        MAX_PERSONAJES_SELECCION,
        confeti
    );

    Camera3D camara =
        ObtenerCamaraEscenaSeleccion(tiempoEscena);

    DibujarEtiquetasEscena(
        *this,
        participantes,
        cantidadMaxima,
        escena,
        camara
    );


    //------------------------------
    // DEGRADADOS (contraste para texto, sin tapar la escena)
    //------------------------------

    DrawRectangleGradientV(
        0,
        0,
        ancho,
        Px(86.0f),
        Fade(Color{ 18, 10, 40, 255 }, 0.55f),
        Fade(Color{ 18, 10, 40, 255 }, 0.0f)
    );

    DrawRectangleGradientV(
        0,
        alto - Px(130.0f),
        ancho,
        Px(130.0f),
        Fade(Color{ 18, 10, 40, 255 }, 0.0f),
        Fade(Color{ 18, 10, 40, 255 }, 0.60f)
    );


    //------------------------------
    // TITULO
    //------------------------------

    DibujarTextoCentrado(
        "ELIGE TU PERSONAJE",
        ancho / 2,
        Px(12.0f),
        Px(34.0f),
        RAYWHITE
    );

    int humanos = 0;

    for (
        int i = 0;
        i < cantidadMaxima;
        i++
    )
    {
        if (participantes[i].activo && participantes[i].conectado)
        {
            humanos++;
        }
    }

    const char* subtitulo =
        humanos < 1
        ? "UNETE PARA EMPEZAR (LOS PUESTOS LIBRES SERAN BOTS)"
        : TextFormat(
            "%d HUMANOS + %d BOTS",
            humanos,
            MAX_PARTICIPANTES - humanos
        );

    DibujarTextoCentrado(
        subtitulo,
        ancho / 2,
        Px(52.0f),
        Px(16.0f),
        humanos < 2 ? Color{ 255, 190, 90, 255 } : Color{ 235, 225, 245, 255 }
    );


    //------------------------------
    // PANELES (calculados con el tamano real de la ventana)
    //------------------------------

    float margen = (float)Px(16.0f);
    float hueco = (float)Px(10.0f);
    float panelAncho = (ancho - margen * 2.0f - hueco * 3.0f) / 4.0f;
    float panelAlto = (float)Px(92.0f);
    float panelY = alto - panelAlto - (float)Px(32.0f);

    for (
        int i = 0;
        i < MAX_JUGADORES_SELECCION && i < cantidadMaxima;
        i++
    )
    {
        DibujarPanelJugador(
            *this,
            participantes[i],
            i,
            Rectangle{
                margen + i * (panelAncho + hueco),
                panelY,
                panelAncho,
                panelAlto
            }
        );
    }


    //------------------------------
    // TODOS LISTOS (bajo el titulo: no tapa personajes ni paneles)
    //------------------------------

    if (todosListos)
    {
        float pulso =
            0.5f + 0.5f * std::sin(tiempoEscena * 6.0f);

        float entrada =
            tiempoTodosListos < 0.25f ? tiempoTodosListos / 0.25f : 1.0f;

        int tamano =
            Px((20.0f + 1.5f * pulso) * (0.6f + 0.4f * entrada));

        const char* mensaje =
            "TODOS LISTOS!  CONFIRMA PARA SEGUIR";

        int anchoMensaje =
            MeasureText(mensaje, tamano) + Px(40.0f);

        int altoMensaje =
            tamano + Px(12.0f);

        Rectangle caja =
        {
            ancho * 0.5f - anchoMensaje * 0.5f,
            (float)Px(74.0f),
            (float)anchoMensaje,
            (float)altoMensaje
        };

        DrawRectangleRounded(
            caja,
            0.5f,
            10,
            Fade(Color{ 20, 12, 38, 255 }, 0.85f * entrada)
        );

        DrawRectangleRoundedLines(
            caja,
            0.5f,
            10,
            Fade(GOLD, entrada)
        );

        DrawText(
            mensaje,
            (int)(caja.x + (caja.width - MeasureText(mensaje, tamano)) * 0.5f),
            (int)(caja.y + (caja.height - tamano) * 0.5f),
            tamano,
            Fade(GOLD, entrada)
        );
    }


    //------------------------------
    // AYUDA INFERIOR
    //------------------------------

    char ayuda[256];

    ComponerAyuda(
        participantes,
        cantidadMaxima,
        ayuda,
        (int)sizeof(ayuda)
    );

    int tamanoAyuda = Px(13.0f);

    // Si la ayuda no cabe en el ancho, se reduce el texto (nunca se corta).
    while (
        tamanoAyuda > 9 &&
        MeasureText(ayuda, tamanoAyuda) > ancho - Px(24.0f)
    )
    {
        tamanoAyuda--;
    }

    DibujarTextoCentrado(
        ayuda,
        ancho / 2,
        alto - Px(24.0f),
        tamanoAyuda,
        Fade(Color{ 240, 232, 250, 255 }, 0.88f)
    );


    //------------------------------
    // FUNDIDO DE ENTRADA
    //------------------------------

    if (alphaEntrada < 1.0f)
    {
        DrawRectangle(
            0,
            0,
            ancho,
            alto,
            Fade(Color{ 12, 8, 24, 255 }, 1.0f - alphaEntrada)
        );
    }
}


//==================================================
// DESCARGAR
//==================================================

void SeleccionPersonajes::Descargar()
{
    // No hay recursos propios: el modelo 3D es el compartido, que
    // descarga main.cpp con DescargarModeloJugadorCompartido().
    recursosCargados =
        false;
}
