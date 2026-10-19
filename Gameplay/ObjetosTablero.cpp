#include "Gameplay/ObjetosTablero.h"

#include "Gameplay/PartidaTablero.h"
#include "UI/EstiloUI.h"
#include "UI/EstiloUIGlobal.h"

#include <cmath>
#include <cstdio>
#include <cstring>


//==================================================
// CATALOGO
//==================================================

static const DefinicionObjetoTablero CATALOGO_OBJETOS[CANTIDAD_OBJETOS_TABLERO] =
{
    // BOTAS
    { "BOTAS", "+3 CASILLAS EN TU PRÓXIMA TIRADA", 5, true, true,
      Color{ 176, 110, 60, 255 } },

    // DADO DOBLE
    { "DADO DOBLE", "TIRÁS DOS DADOS DE 6 Y SE SUMAN", 8, false, true,
      Color{ 235, 200, 90, 255 } },

    // ESCUDO
    { "ESCUDO", "BLOQUEA TU PRÓXIMA PÉRDIDA (AUTOMÁTICO)", 7, true, false,
      Color{ 70, 120, 200, 255 } },

    // IMAN
    { "IMÁN", "ROBA 5 MONEDAS A UN RIVAL", 9, false, true,
      Color{ 200, 60, 55, 255 } }
};


const DefinicionObjetoTablero& ObtenerDefinicionObjeto(int id)
{
    if (id < 0 || id >= CANTIDAD_OBJETOS_TABLERO)
    {
        id = 0;
    }

    return CATALOGO_OBJETOS[id];
}


//==================================================
// INVENTARIO
//==================================================

int ContarObjetosInventario(const int inventario[])
{
    int cantidad = 0;

    for (int i = 0; i < MAX_OBJETOS_INVENTARIO; i++)
    {
        if (inventario[i] != OBJETO_VACIO)
        {
            cantidad++;
        }
    }

    return cantidad;
}


bool InventarioLleno(const int inventario[])
{
    return ContarObjetosInventario(inventario) >= MAX_OBJETOS_INVENTARIO;
}


bool TieneObjetoInventario(const int inventario[], int id)
{
    for (int i = 0; i < MAX_OBJETOS_INVENTARIO; i++)
    {
        if (inventario[i] == id)
        {
            return true;
        }
    }

    return false;
}


bool AgregarObjetoInventario(int inventario[], int id)
{
    if (id < 0 || id >= CANTIDAD_OBJETOS_TABLERO)
    {
        return false;
    }

    for (int i = 0; i < MAX_OBJETOS_INVENTARIO; i++)
    {
        if (inventario[i] == OBJETO_VACIO)
        {
            inventario[i] = id;
            return true;
        }
    }

    return false;
}


int QuitarObjetoInventario(int inventario[], int espacio)
{
    if (espacio < 0 || espacio >= MAX_OBJETOS_INVENTARIO)
    {
        return OBJETO_VACIO;
    }

    int id = inventario[espacio];
    inventario[espacio] = OBJETO_VACIO;

    return id;
}


//==================================================
// REGLAS
//==================================================

int ListarRivalesImanes(
    const PartidaTablero& partida,
    int jugador,
    int rivales[]
)
{
    int cantidad = 0;

    for (int i = 0; i < partida.cantidadJugadores; i++)
    {
        int indice = partida.ordenParticipantes[i];

        if (
            indice >= 0 &&
            indice < MAX_PARTICIPANTES &&
            indice != jugador &&
            partida.jugadores[indice].monedas > 0
        )
        {
            rivales[cantidad++] = indice;
        }
    }

    // Orden por numero de jugador: la lista del menu no depende del turno.
    for (int a = 0; a < cantidad; a++)
    {
        for (int b = a + 1; b < cantidad; b++)
        {
            if (rivales[b] < rivales[a])
            {
                int temporal = rivales[a];
                rivales[a] = rivales[b];
                rivales[b] = temporal;
            }
        }
    }

    return cantidad;
}


bool ObjetoUsableAhora(
    const PartidaTablero& partida,
    int jugador,
    int espacio
)
{
    if (
        jugador < 0 ||
        jugador >= MAX_PARTICIPANTES ||
        espacio < 0 ||
        espacio >= MAX_OBJETOS_INVENTARIO
    )
    {
        return false;
    }

    const EstadoJugadorPartidaTablero& estado = partida.jugadores[jugador];
    int id = estado.inventario[espacio];

    if (id == OBJETO_VACIO || !ObtenerDefinicionObjeto(id).usoManual)
    {
        return false;
    }

    switch (id)
    {
        case OBJETO_BOTAS:
            return !estado.botasActivas;

        case OBJETO_DADO_DOBLE:
            return !estado.dadoDobleActivo;

        case OBJETO_IMAN:
        {
            int rivales[MAX_PARTICIPANTES];

            return ListarRivalesImanes(partida, jugador, rivales) > 0;
        }

        default:
            break;
    }

    return false;
}


bool HayObjetoUsableAhora(const PartidaTablero& partida, int jugador)
{
    for (int i = 0; i < MAX_OBJETOS_INVENTARIO; i++)
    {
        if (ObjetoUsableAhora(partida, jugador, i))
        {
            return true;
        }
    }

    return false;
}


void AvisarObjeto(PartidaTablero& partida, const char* texto)
{
    std::snprintf(
        partida.avisoObjeto,
        sizeof(partida.avisoObjeto),
        "%s",
        texto
    );

    partida.tiempoAvisoObjeto = 2.8f;
}


bool EscudoAbsorbePerdida(PartidaTablero& partida, int jugador)
{
    if (jugador < 0 || jugador >= MAX_PARTICIPANTES)
    {
        return false;
    }

    EstadoJugadorPartidaTablero& estado = partida.jugadores[jugador];

    for (int i = 0; i < MAX_OBJETOS_INVENTARIO; i++)
    {
        if (estado.inventario[i] == OBJETO_ESCUDO)
        {
            QuitarObjetoInventario(estado.inventario, i);

            char texto[96];
            std::snprintf(
                texto, sizeof(texto),
                "¡EL ESCUDO DE J%d BLOQUEA LA PÉRDIDA!",
                jugador + 1
            );

            AvisarObjeto(partida, texto);
            ReproducirSonidoTablero(partida, SONIDO_IMPACTO);

            return true;
        }
    }

    return false;
}


bool UsarObjeto(
    PartidaTablero& partida,
    int jugador,
    int espacio,
    int rival,
    char* resumen,
    int capacidad
)
{
    resumen[0] = '\0';

    if (!ObjetoUsableAhora(partida, jugador, espacio))
    {
        return false;
    }

    EstadoJugadorPartidaTablero& estado = partida.jugadores[jugador];
    int id = estado.inventario[espacio];

    switch (id)
    {
        case OBJETO_BOTAS:
            QuitarObjetoInventario(estado.inventario, espacio);
            estado.botasActivas = true;

            std::snprintf(
                resumen, capacidad,
                "J%d USA LAS BOTAS: +%d EN LA TIRADA",
                jugador + 1, BOTAS_CASILLAS_EXTRA
            );
            break;

        case OBJETO_DADO_DOBLE:
            QuitarObjetoInventario(estado.inventario, espacio);
            estado.dadoDobleActivo = true;

            std::snprintf(
                resumen, capacidad,
                "J%d USA EL DADO DOBLE: DOS DADOS EN LA TIRADA",
                jugador + 1
            );
            break;

        case OBJETO_IMAN:
        {
            int rivales[MAX_PARTICIPANTES];
            int cantidad = ListarRivalesImanes(partida, jugador, rivales);
            bool valido = false;

            for (int i = 0; i < cantidad; i++)
            {
                if (rivales[i] == rival)
                {
                    valido = true;
                }
            }

            if (!valido)
            {
                return false;
            }

            QuitarObjetoInventario(estado.inventario, espacio);

            if (EscudoAbsorbePerdida(partida, rival))
            {
                std::snprintf(
                    resumen, capacidad,
                    "¡EL ESCUDO DE J%d BLOQUEA EL IMÁN DE J%d!",
                    rival + 1, jugador + 1
                );
            }
            else
            {
                EstadoJugadorPartidaTablero& victima = partida.jugadores[rival];
                int robo =
                    victima.monedas < IMAN_MONEDAS_ROBADAS
                    ? victima.monedas
                    : IMAN_MONEDAS_ROBADAS;

                victima.monedas -= robo;
                estado.monedas += robo;

                std::snprintf(
                    resumen, capacidad,
                    "¡EL IMÁN DE J%d ROBA %d MONEDAS A J%d!",
                    jugador + 1, robo, rival + 1
                );

                ReproducirSonidoTablero(partida, SONIDO_MONEDA);
            }
            break;
        }

        default:
            return false;
    }

    ReproducirSonidoTablero(partida, SONIDO_RECOGER_OBJETO);
    AvisarObjeto(partida, resumen);

    return true;
}


int TirarDadoDeJugador(
    PartidaTablero& partida,
    int jugador,
    char* detalle,
    int capacidad
)
{
    EstadoJugadorPartidaTablero& estado = partida.jugadores[jugador];
    int total = 0;

    detalle[0] = '\0';

    if (estado.dadoDobleActivo)
    {
        int a = GetRandomValue(1, DADO_DOBLE_CARAS);
        int b = GetRandomValue(1, DADO_DOBLE_CARAS);

        total = a + b;
        std::snprintf(detalle, capacidad, "DADO DOBLE: %d + %d", a, b);
    }
    else
    {
        total = GetRandomValue(1, 10);
    }

    if (estado.botasActivas)
    {
        total += BOTAS_CASILLAS_EXTRA;

        if (detalle[0] != '\0')
        {
            int largo = (int)std::strlen(detalle);
            std::snprintf(detalle + largo, capacidad - largo, " + BOTAS %d", BOTAS_CASILLAS_EXTRA);
        }
        else
        {
            std::snprintf(detalle, capacidad, "%d + BOTAS %d", total - BOTAS_CASILLAS_EXTRA, BOTAS_CASILLAS_EXTRA);
        }
    }

    estado.dadoDobleActivo = false;
    estado.botasActivas = false;

    return total;
}


//==================================================
// BOTS
//==================================================

int DistanciaEntreCasillas(
    const PartidaTablero& partida,
    int desde,
    int hasta
)
{
    const Tablero& tablero = partida.tablero;
    int distancia[MAX_CASILLAS_TABLERO];
    int cola[MAX_CASILLAS_TABLERO];
    int inicio = 0;
    int fin = 0;

    for (int i = 0; i < MAX_CASILLAS_TABLERO; i++)
    {
        distancia[i] = -1;
    }

    if (
        desde < 0 || desde >= tablero.cantidadCasillas ||
        hasta < 0 || hasta >= tablero.cantidadCasillas
    )
    {
        return -1;
    }

    distancia[desde] = 0;
    cola[fin++] = desde;

    while (inicio < fin)
    {
        int actual = cola[inicio++];

        if (actual == hasta)
        {
            return distancia[actual];
        }

        for (int c = 0; c < tablero.casillas[actual].cantidadConexiones; c++)
        {
            int destino = tablero.casillas[actual].conexiones[c].destino;

            if (destino >= 0 && destino < tablero.cantidadCasillas && distancia[destino] < 0)
            {
                distancia[destino] = distancia[actual] + 1;
                cola[fin++] = destino;
            }
        }
    }

    return -1;
}


int DecidirObjetoBot(
    const PartidaTablero& partida,
    int jugador,
    int& rival
)
{
    const EstadoJugadorPartidaTablero& estado = partida.jugadores[jugador];
    int espacioBotas = -1;
    int espacioDoble = -1;

    rival = -1;

    for (int i = 0; i < MAX_OBJETOS_INVENTARIO; i++)
    {
        if (!ObjetoUsableAhora(partida, jugador, i))
        {
            continue;
        }

        int id = estado.inventario[i];

        if (id == OBJETO_IMAN)
        {
            // Iman: si algun rival tiene 8 monedas o mas, va al mas rico.
            int rivales[MAX_PARTICIPANTES];
            int cantidad = ListarRivalesImanes(partida, jugador, rivales);
            int mejor = -1;

            for (int r = 0; r < cantidad; r++)
            {
                if (
                    partida.jugadores[rivales[r]].monedas >= 8 &&
                    (
                        mejor < 0 ||
                        partida.jugadores[rivales[r]].monedas > partida.jugadores[mejor].monedas
                    )
                )
                {
                    mejor = rivales[r];
                }
            }

            if (mejor >= 0)
            {
                rival = mejor;
                return i;
            }
        }
        else if (id == OBJETO_BOTAS && espacioBotas < 0)
        {
            espacioBotas = i;
        }
        else if (id == OBJETO_DADO_DOBLE && espacioDoble < 0)
        {
            espacioDoble = i;
        }
    }

    if (espacioBotas < 0 && espacioDoble < 0)
    {
        return -1;
    }

    // Con las monedas del trofeo, los objetos de movimiento se usan si
    // acercan la tirada a la casilla del trofeo (tirada media: 5,5; con
    // botas 8,5; con dado doble 7).
    if (
        estado.monedas >= partida.costoTrofeo &&
        partida.casillaTrofeo >= 0
    )
    {
        int distancia =
            DistanciaEntreCasillas(partida, estado.casillaActual, partida.casillaTrofeo);

        if (distancia >= 6 && distancia <= 12 && espacioDoble >= 0)
        {
            return espacioDoble;
        }

        if (distancia >= 5 && distancia <= 11 && espacioBotas >= 0)
        {
            return espacioBotas;
        }

        return -1;
    }

    // Sin trofeo a la vista: no los guarda para siempre.
    if (GetRandomValue(0, 99) < 35)
    {
        return espacioBotas >= 0 ? espacioBotas : espacioDoble;
    }

    return -1;
}


//==================================================
// MENU DE USO
//==================================================

void IniciarMenuObjetos(PartidaTablero& partida, int jugador)
{
    MenuObjetosTablero& menu = partida.menuObjetos;

    menu = MenuObjetosTablero{};
    menu.activo = true;

    const EstadoJugadorPartidaTablero& estado = partida.jugadores[jugador];

    // Arranca sobre el primer objeto que se pueda usar.
    for (int i = 0; i < MAX_OBJETOS_INVENTARIO; i++)
    {
        if (estado.inventario[i] != OBJETO_VACIO)
        {
            menu.seleccion = i;

            if (ObjetoUsableAhora(partida, jugador, i))
            {
                break;
            }
        }
    }
}


static void MensajeMenuObjetos(MenuObjetosTablero& menu, const char* texto)
{
    std::snprintf(menu.mensaje, sizeof(menu.mensaje), "%s", texto);
    menu.tiempoMensaje = 2.2f;
}


void ActualizarMenuObjetos(
    PartidaTablero& partida,
    float deltaTime,
    bool esAutomatico,
    bool puedeDecidir,
    const InputSeleccionParticipante& entrada
)
{
    MenuObjetosTablero& menu = partida.menuObjetos;
    int jugador = ObtenerParticipanteTurnoTableroFinal(partida);

    if (!menu.activo || menu.terminado)
    {
        return;
    }

    menu.tiempo += deltaTime;

    if (menu.tiempoMensaje > 0.0f)
    {
        menu.tiempoMensaje -= deltaTime;
    }

    // Un bot o un humano desconectado no usa el menu; sin actividad el
    // humano lo cierra solo (la partida nunca queda esperando aqui).
    if (jugador < 0 || esAutomatico || menu.tiempo > 25.0f)
    {
        menu.terminado = true;
        return;
    }

    if (!puedeDecidir)
    {
        return;
    }

    if (menu.paso == 0)
    {
        int anterior = menu.seleccion;

        if (entrada.izquierda || entrada.arriba)
        {
            menu.seleccion = (menu.seleccion + MAX_OBJETOS_INVENTARIO - 1) % MAX_OBJETOS_INVENTARIO;
        }
        else if (entrada.derecha || entrada.abajo)
        {
            menu.seleccion = (menu.seleccion + 1) % MAX_OBJETOS_INVENTARIO;
        }

        if (menu.seleccion != anterior)
        {
            ReproducirSonidoTablero(partida, SONIDO_UI_MOVER);
        }

        if (entrada.cancelar)
        {
            ReproducirSonidoTablero(partida, SONIDO_UI_CANCELAR);
            menu.terminado = true;
            return;
        }

        if (!entrada.confirmar || menu.tiempo < 0.15f)
        {
            return;
        }

        int id = partida.jugadores[jugador].inventario[menu.seleccion];

        if (id == OBJETO_VACIO)
        {
            MensajeMenuObjetos(menu, "ESE ESPACIO ESTÁ VACÍO");
            ReproducirSonidoTablero(partida, SONIDO_ERROR);
        }
        else if (!ObtenerDefinicionObjeto(id).usoManual)
        {
            MensajeMenuObjetos(menu, "EL ESCUDO SE USA SOLO AL RECIBIR UNA PÉRDIDA");
            ReproducirSonidoTablero(partida, SONIDO_ERROR);
        }
        else if (!ObjetoUsableAhora(partida, jugador, menu.seleccion))
        {
            MensajeMenuObjetos(
                menu,
                id == OBJETO_IMAN
                    ? "NINGÚN RIVAL TIENE MONEDAS PARA ROBAR"
                    : "ESE EFECTO YA ESTÁ ACTIVO EN ESTA TIRADA"
            );
            ReproducirSonidoTablero(partida, SONIDO_ERROR);
        }
        else if (id == OBJETO_IMAN)
        {
            menu.cantidadRivales = ListarRivalesImanes(partida, jugador, menu.rivales);
            menu.rivalSeleccion = 0;
            menu.espacioIman = menu.seleccion;
            menu.paso = 1;
            menu.tiempo = 0.0f;
            ReproducirSonidoTablero(partida, SONIDO_UI_CONFIRMAR);
        }
        else
        {
            char resumen[96];

            UsarObjeto(partida, jugador, menu.seleccion, -1, resumen, (int)sizeof(resumen));
            menu.terminado = true;
        }

        return;
    }

    // Paso 1: a quien robarle con el iman.
    int anterior = menu.rivalSeleccion;

    if (menu.cantidadRivales > 0)
    {
        if (entrada.izquierda || entrada.arriba)
        {
            menu.rivalSeleccion = (menu.rivalSeleccion + menu.cantidadRivales - 1) % menu.cantidadRivales;
        }
        else if (entrada.derecha || entrada.abajo)
        {
            menu.rivalSeleccion = (menu.rivalSeleccion + 1) % menu.cantidadRivales;
        }
    }

    if (menu.rivalSeleccion != anterior)
    {
        ReproducirSonidoTablero(partida, SONIDO_UI_MOVER);
    }

    if (entrada.cancelar)
    {
        ReproducirSonidoTablero(partida, SONIDO_UI_CANCELAR);
        menu.paso = 0;
        menu.tiempo = 0.0f;
        return;
    }

    if (entrada.confirmar && menu.tiempo >= 0.15f && menu.cantidadRivales > 0)
    {
        char resumen[96];

        ReproducirSonidoTablero(partida, SONIDO_UI_CONFIRMAR);
        UsarObjeto(
            partida, jugador, menu.espacioIman,
            menu.rivales[menu.rivalSeleccion],
            resumen, (int)sizeof(resumen)
        );
        menu.terminado = true;
    }
}


void TextoMenuObjetos(
    const PartidaTablero& partida,
    char* destino,
    int capacidad
)
{
    const MenuObjetosTablero& menu = partida.menuObjetos;

    destino[0] = '\0';

    if (menu.tiempoMensaje > 0.0f && menu.mensaje[0] != '\0')
    {
        std::snprintf(destino, capacidad, "%s", menu.mensaje);
    }
    else if (menu.paso == 1)
    {
        std::snprintf(destino, capacidad, "ELEGÍ A QUIÉN ROBARLE CON EL IMÁN");
    }
    else
    {
        std::snprintf(destino, capacidad, "ELEGÍ UN OBJETO PARA USAR ANTES DE TIRAR");
    }
}


//==================================================
// ICONOS (primitivas 2D)
//==================================================

static float Limitar01Objetos(float v)
{
    return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
}


static void DibujarPuntosDado(Vector2 centro, float lado, int caras, Color tinta)
{
    // Pips de 1, 2, 3 o 4 en una cara.
    float d = lado * 0.24f;
    float r = lado * 0.075f;

    if (caras == 1 || caras == 3)
    {
        DrawCircleV(centro, r, tinta);
    }

    if (caras >= 2)
    {
        DrawCircleV(Vector2{ centro.x - d, centro.y - d }, r, tinta);
        DrawCircleV(Vector2{ centro.x + d, centro.y + d }, r, tinta);
    }

    if (caras >= 4)
    {
        DrawCircleV(Vector2{ centro.x + d, centro.y - d }, r, tinta);
        DrawCircleV(Vector2{ centro.x - d, centro.y + d }, r, tinta);
    }
}


void DibujarIconoObjeto(int id, Vector2 c, float r, float alfa)
{
    const Color tinta = Fade(UI_TINTA, alfa);

    switch (id)
    {
        case OBJETO_BOTAS:
        {
            Color cuero = Fade(Color{ 176, 110, 60, 255 }, alfa);
            Color puno = Fade(Color{ 84, 138, 70, 255 }, alfa);

            // Caña y pie, con contorno de tinta.
            Rectangle cana = { c.x - r * 0.52f, c.y - r * 0.82f, r * 0.62f, r * 1.05f };
            Rectangle pie = { c.x - r * 0.52f, c.y + r * 0.02f, r * 1.18f, r * 0.62f };
            float borde = r * 0.09f;

            DrawRectangleRounded(Rectangle{ cana.x - borde, cana.y - borde, cana.width + borde * 2.0f, cana.height + borde * 2.0f }, 0.25f, 4, tinta);
            DrawRectangleRounded(Rectangle{ pie.x - borde, pie.y - borde, pie.width + borde * 2.0f, pie.height + borde * 2.0f }, 0.45f, 6, tinta);
            DrawRectangleRounded(cana, 0.25f, 4, cuero);
            DrawRectangleRounded(pie, 0.45f, 6, cuero);
            DrawRectangleRec(Rectangle{ cana.x, cana.y, cana.width, r * 0.26f }, puno);
            DrawRectangleRec(Rectangle{ pie.x + r * 0.05f, pie.y + pie.height * 0.74f, pie.width - r * 0.1f, pie.height * 0.2f }, Fade(UI_TINTA, 0.55f * alfa));
            break;
        }

        case OBJETO_DADO_DOBLE:
        {
            float lado = r * 1.05f;
            Color cara = Fade(Color{ 255, 248, 232, 255 }, alfa);
            Rectangle a = { c.x - r * 0.88f, c.y - r * 0.62f, lado, lado };
            Rectangle b = { c.x - r * 0.12f, c.y - r * 0.40f, lado, lado };
            float borde = r * 0.09f;

            DrawRectangleRounded(Rectangle{ a.x - borde, a.y - borde, a.width + borde * 2.0f, a.height + borde * 2.0f }, 0.25f, 6, tinta);
            DrawRectangleRounded(a, 0.22f, 6, cara);
            DibujarPuntosDado(Vector2{ a.x + lado * 0.5f, a.y + lado * 0.5f }, lado, 3, tinta);

            DrawRectangleRounded(Rectangle{ b.x - borde, b.y - borde, b.width + borde * 2.0f, b.height + borde * 2.0f }, 0.25f, 6, tinta);
            DrawRectangleRounded(b, 0.22f, 6, Fade(Color{ 235, 200, 90, 255 }, alfa));
            DibujarPuntosDado(Vector2{ b.x + lado * 0.5f, b.y + lado * 0.5f }, lado, 4, tinta);
            break;
        }

        case OBJETO_ESCUDO:
        {
            Color azul = Fade(Color{ 70, 120, 200, 255 }, alfa);
            Color borde = Fade(Color{ 245, 232, 200, 255 }, alfa);
            float ancho = r * 0.84f;
            float alto = r * 0.95f;
            float grosor = r * 0.10f;

            // Contorno de tinta, borde claro y cuerpo azul.
            for (int capa = 0; capa < 3; capa++)
            {
                float extra = capa == 0 ? grosor * 1.8f : (capa == 1 ? grosor * 0.9f : 0.0f);
                Color color = capa == 0 ? tinta : (capa == 1 ? borde : azul);

                DrawRectangleRec(Rectangle{ c.x - ancho - extra, c.y - r * 0.85f - extra, (ancho + extra) * 2.0f, alto * 0.62f + extra }, color);
                DrawTriangle(
                    Vector2{ c.x, c.y + r * 0.95f + extra * 1.2f },
                    Vector2{ c.x + ancho + extra, c.y - r * 0.85f + alto * 0.62f },
                    Vector2{ c.x - ancho - extra, c.y - r * 0.85f + alto * 0.62f },
                    color
                );
            }

            // Emblema: estrella simple dorada.
            DrawPoly(Vector2{ c.x, c.y - r * 0.12f }, 5, r * 0.34f, -90.0f, Fade(Color{ 255, 214, 64, 255 }, alfa));
            break;
        }

        case OBJETO_IMAN:
        {
            Color rojo = Fade(Color{ 200, 60, 55, 255 }, alfa);
            Color plata = Fade(Color{ 232, 232, 238, 255 }, alfa);
            float interior = r * 0.36f;
            float exterior = r * 0.82f;
            float borde = r * 0.09f;

            // Herradura: arco superior y dos patas con puntas plateadas.
            DrawRing(c, interior - borde, exterior + borde, 180.0f, 360.0f, 24, tinta);
            DrawRing(c, interior, exterior, 180.0f, 360.0f, 24, rojo);

            for (int lado = -1; lado <= 1; lado += 2)
            {
                float centroPata = c.x + (float)lado * (interior + exterior) * 0.5f;
                float ancho = exterior - interior;
                Rectangle pata = { centroPata - ancho * 0.5f, c.y, ancho, r * 0.72f };

                DrawRectangleRec(Rectangle{ pata.x - borde, pata.y, pata.width + borde * 2.0f, pata.height + borde }, tinta);
                DrawRectangleRec(pata, rojo);
                DrawRectangleRec(Rectangle{ pata.x, pata.y + pata.height * 0.62f, pata.width, pata.height * 0.38f }, plata);
            }
            break;
        }

        default:
            break;
    }
}


void DibujarEspacioObjeto(Rectangle rect, int id, bool resaltar, float alfa)
{
    float borde = Px(2.0f);
    float radio = 0.22f;

    DrawRectangleRounded(
        Rectangle{ rect.x + Px(2.0f), rect.y + Px(3.0f), rect.width, rect.height },
        radio, 6, Fade(BLACK, 0.25f * alfa)
    );
    DrawRectangleRounded(rect, radio, 6, Fade(UI_TINTA, alfa));
    DrawRectangleRounded(
        Rectangle{ rect.x + borde, rect.y + borde, rect.width - borde * 2.0f, rect.height - borde * 2.0f },
        radio, 6,
        Fade(id == OBJETO_VACIO ? Color{ 214, 190, 148, 255 } : Color{ 255, 248, 232, 255 }, alfa)
    );

    if (id != OBJETO_VACIO)
    {
        float lado = rect.width < rect.height ? rect.width : rect.height;

        DibujarIconoObjeto(
            id,
            Vector2{ rect.x + rect.width * 0.5f, rect.y + rect.height * 0.5f },
            lado * 0.40f,
            alfa
        );
    }

    if (resaltar)
    {
        float halo = Px(3.0f);

        DrawRectangleRoundedLinesEx(
            Rectangle{ rect.x - halo, rect.y - halo, rect.width + halo * 2.0f, rect.height + halo * 2.0f },
            radio + 0.03f, 6, Px(UI_BORDE_FOCO), Fade(UI_FOCO, alfa)
        );
    }
}


//==================================================
// HUD: INVENTARIOS
//==================================================

void DibujarInventariosHud(const PartidaTablero& partida)
{
    const EstiloUI& estilo = ObtenerEstiloUI();

    if (!estilo.cargado || partida.cantidadJugadores <= 0)
    {
        return;
    }

    int turno = ObtenerParticipanteTurnoTableroFinal(partida);

    // Orden fijo J1..J4 para que las filas no salten con el turno.
    int filas[MAX_PARTICIPANTES];
    int cantidad = 0;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        if (partida.jugadores[i].participa)
        {
            filas[cantidad++] = i;
        }
    }

    if (cantidad == 0)
    {
        return;
    }

    const float altoFila = 27.0f;
    const float lado = 22.0f;
    const float ancho = 178.0f;
    float alto = altoFila * (float)cantidad + 8.0f;

    Rectangle panel = RectRef(14.0f, 664.0f - alto, ancho, alto);

    DrawRectangleRec(
        Rectangle{ panel.x + Px(2.0f), panel.y + Px(3.0f), panel.width, panel.height },
        Fade(BLACK, 0.22f)
    );
    DibujarNueveCortes(estilo.placa, 8, panel, EscalaUI(), WHITE);
    DrawRectangleLinesEx(panel, Px(1.0f) < 1.0f ? 1.0f : Px(1.0f), Fade(UI_MADERA_OSCURA, 0.6f));

    int tamano = TamanoTexto(UI_TEXTO_PEQUENO - 2);

    for (int f = 0; f < cantidad; f++)
    {
        int indice = filas[f];
        const EstadoJugadorPartidaTablero& estado = partida.jugadores[indice];
        bool activo = indice == turno;
        float y = panel.y + Px(4.0f) + Px(altoFila) * (float)f;
        float cy = y + Px(altoFila) * 0.5f;

        if (activo)
        {
            DrawRectangleRounded(
                Rectangle{ panel.x + Px(3.0f), y + Px(1.0f), panel.width - Px(6.0f), Px(altoFila) - Px(2.0f) },
                0.25f, 4, Fade(UI_FOCO, 0.35f)
            );
        }

        // Color del jugador y numero.
        Color color = partida.participantes != nullptr ? partida.participantes[indice].color : LIGHTGRAY;

        DrawRectangleRec(Rectangle{ panel.x + Px(8.0f), cy - Px(8.0f), Px(5.0f), Px(16.0f) }, color);
        DrawText(TextFormat("J%d", indice + 1), (int)(panel.x + Px(17.0f)), (int)(cy - (float)tamano * 0.5f), tamano, UI_TINTA);

        for (int e = 0; e < MAX_OBJETOS_INVENTARIO; e++)
        {
            Rectangle espacio =
            {
                panel.x + Px(44.0f) + Px(lado + 4.0f) * (float)e,
                cy - Px(lado) * 0.5f,
                Px(lado),
                Px(lado)
            };

            DibujarEspacioObjeto(espacio, estado.inventario[e], false, 1.0f);
        }

        // Efectos ya armados para la proxima tirada.
        float x = panel.x + Px(44.0f) + Px(lado + 4.0f) * (float)MAX_OBJETOS_INVENTARIO + Px(4.0f);

        if (estado.botasActivas)
        {
            const char* texto = TextFormat("+%d", BOTAS_CASILLAS_EXTRA);

            DrawText(texto, (int)x, (int)(cy - (float)tamano * 0.5f), tamano, Color{ 150, 90, 20, 255 });
            x += (float)MeasureText(texto, tamano) + Px(5.0f);
        }

        if (estado.dadoDobleActivo)
        {
            DrawText("x2", (int)x, (int)(cy - (float)tamano * 0.5f), tamano, Color{ 150, 90, 20, 255 });
        }
    }
}


void DibujarAvisoObjeto(const PartidaTablero& partida)
{
    if (
        partida.tiempoAvisoObjeto <= 0.0f ||
        partida.avisoObjeto[0] == '\0' ||
        partida.fase == FASE_PARTIDA_TABLERO_EVENTO_INTERACTIVO ||
        partida.fase == FASE_PARTIDA_TABLERO_TERMINADA
    )
    {
        return;
    }

    float alfa = Limitar01Objetos(partida.tiempoAvisoObjeto / 0.4f);
    float cx = (float)GetScreenWidth() * 0.5f;
    float y = RectRef(0.0f, 500.0f, 0.0f, 0.0f).y;

    DibujarTextoMundoContornoUI(partida.avisoObjeto, cx, y, UI_TEXTO_GRANDE, UI_FOCO, alfa);
}


//==================================================
// MENU DE USO: DIBUJO
//==================================================

void DibujarMenuObjetos2D(const PartidaTablero& partida)
{
    const MenuObjetosTablero& menu = partida.menuObjetos;

    if (!menu.activo || partida.fase != FASE_PARTIDA_TABLERO_MENU_OBJETOS)
    {
        return;
    }

    const EstiloUI& estilo = ObtenerEstiloUI();
    int jugador = ObtenerParticipanteTurnoTableroFinal(partida);

    if (!estilo.cargado || jugador < 0)
    {
        return;
    }

    const EstadoJugadorPartidaTablero& estado = partida.jugadores[jugador];
    float entrada = estilo.reducirMovimiento ? 1.0f : Limitar01Objetos(menu.tiempo / 0.2f);
    float cx = (float)GetScreenWidth() * 0.5f;

    DibujarVeloUI(0.55f);

    DibujarTextoMundoContornoUI(
        TextFormat("OBJETOS DE J%d", jugador + 1),
        cx, RectRef(0.0f, 166.0f, 0.0f, 0.0f).y, UI_TEXTO_GRANDE, UI_FOCO, 1.0f
    );

    if (menu.paso == 0)
    {
        float centros[MAX_OBJETOS_INVENTARIO] = { 470.0f, 640.0f, 810.0f };

        for (int e = 0; e < MAX_OBJETOS_INVENTARIO; e++)
        {
            Rectangle espacio = RectRef(centros[e] - 64.0f, 250.0f, 128.0f, 128.0f);
            int id = estado.inventario[e];
            bool usable = ObjetoUsableAhora(partida, jugador, e);

            DibujarEspacioObjeto(espacio, id, e == menu.seleccion, entrada);

            // Los que no se pueden usar ahora se apagan.
            if (id != OBJETO_VACIO && !usable)
            {
                DrawRectangleRounded(espacio, 0.22f, 6, Fade(UI_VELO, 0.35f));
            }

            const char* nombre = id == OBJETO_VACIO ? "VACÍO" : ObtenerDefinicionObjeto(id).nombre;

            DibujarTextoAjustadoUI(
                nombre, espacio.x + espacio.width * 0.5f, espacio.y + espacio.height + Px(10.0f),
                UI_TEXTO, espacio.width, UI_TEXTO_MUNDO, true
            );
        }

        // Descripcion del objeto elegido.
        int id = estado.inventario[menu.seleccion];
        const char* descripcion = "NO HAY NADA EN ESTE ESPACIO";

        if (id != OBJETO_VACIO)
        {
            descripcion = ObtenerDefinicionObjeto(id).descripcion;
        }

        Rectangle banda = RectRef(290.0f, 448.0f, 700.0f, 52.0f);
        Rectangle placa = DibujarPlacaConMarcoUI(estilo, banda, 6.0f, entrada);
        int tamano = TamanoTextoAjustadoUI(descripcion, UI_TEXTO, placa.width - Px(24.0f));

        DibujarTextoAjustadoUI(
            descripcion, placa.x + placa.width * 0.5f, placa.y + (placa.height - (float)tamano) * 0.5f,
            UI_TEXTO, placa.width - Px(24.0f), UI_TINTA, true
        );
    }
    else
    {
        DibujarTextoMundoContornoUI(
            "¿A QUIÉN LE ROBÁS CON EL IMÁN?",
            cx, RectRef(0.0f, 202.0f, 0.0f, 0.0f).y, UI_TEXTO, UI_TEXTO_MUNDO, 1.0f
        );

        int n = menu.cantidadRivales;
        float separacion = 170.0f;
        float inicio = 640.0f - separacion * (float)(n - 1) * 0.5f;

        for (int r = 0; r < n; r++)
        {
            int rival = menu.rivales[r];
            Rectangle carta = RectRef(inicio + separacion * (float)r - 70.0f, 262.0f, 140.0f, 130.0f);
            Color color = partida.participantes != nullptr ? partida.participantes[rival].color : LIGHTGRAY;
            bool elegido = r == menu.rivalSeleccion;

            DrawRectangleRounded(Rectangle{ carta.x + Px(3.0f), carta.y + Px(5.0f), carta.width, carta.height }, 0.16f, 6, Fade(BLACK, 0.3f));
            DrawRectangleRounded(carta, 0.16f, 6, UI_TINTA);
            DrawRectangleRounded(Rectangle{ carta.x + Px(3.0f), carta.y + Px(3.0f), carta.width - Px(6.0f), carta.height - Px(6.0f) }, 0.16f, 6, Color{ 255, 248, 232, 255 });
            DrawRectangleRounded(Rectangle{ carta.x + carta.width * 0.08f, carta.y + carta.height * 0.08f, carta.width * 0.84f, carta.height * 0.16f }, 0.5f, 6, color);

            DibujarTextoAjustadoUI(
                TextFormat("J%d", rival + 1), carta.x + carta.width * 0.5f, carta.y + carta.height * 0.30f,
                UI_TEXTO_GRANDE, carta.width * 0.8f, UI_TINTA, true
            );

            // Monedas del rival (moneda + numero).
            const char* monedas = TextFormat("%d", partida.jugadores[rival].monedas);
            int tamanoMonedas = TamanoTexto(UI_TEXTO);
            float anchoMonedas = (float)MeasureText(monedas, tamanoMonedas);
            float izquierda = carta.x + (carta.width - anchoMonedas - Px(22.0f)) * 0.5f;
            float yMonedas = carta.y + carta.height * 0.68f;

            DibujarMonedaUI(Vector2{ izquierda + Px(8.0f), yMonedas + (float)tamanoMonedas * 0.5f }, 8.0f, 1.0f);
            DrawText(monedas, (int)(izquierda + Px(22.0f)), (int)yMonedas, tamanoMonedas, UI_TINTA);

            if (elegido)
            {
                float halo = Px(4.0f);

                DrawRectangleRoundedLinesEx(
                    Rectangle{ carta.x - halo, carta.y - halo, carta.width + halo * 2.0f, carta.height + halo * 2.0f },
                    0.18f, 6, Px(UI_BORDE_FOCO), UI_FOCO
                );
            }
        }
    }

    if (menu.tiempoMensaje > 0.0f && menu.mensaje[0] != '\0')
    {
        DibujarTextoMundoContornoUI(
            menu.mensaje, cx, RectRef(0.0f, 520.0f, 0.0f, 0.0f).y, UI_TEXTO, Color{ 255, 120, 100, 255 },
            Limitar01Objetos(menu.tiempoMensaje / 0.3f)
        );
    }
}
