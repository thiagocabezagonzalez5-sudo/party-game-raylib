#include "Gameplay/TiendaTablero.h"

#include "Gameplay/PartidaTablero.h"
#include "UI/EstiloUI.h"
#include "UI/EstiloUIGlobal.h"

#include <cmath>
#include <cstdio>


//==================================================
// CONSTANTES
//==================================================

static const float TIEMPO_INACTIVIDAD_TIENDA = 20.0f;
static const float RESERVA_MONEDAS_BOT = 2.0f;
static const Color COLOR_TINTA_PRECIO_TIENDA = { 160, 30, 24, 255 };
static const Color COLOR_TINTA_ERROR_TIENDA = { 160, 30, 24, 255 };


static float Limitar01Tienda(float v)
{
    return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
}


static void DecirTienda(EstadoTienda& tienda, const char* texto, int tipo)
{
    std::snprintf(tienda.mensaje, sizeof(tienda.mensaje), "%s", texto);
    tienda.tipoMensaje = tipo;
    tienda.tiempoMensaje = 3.0f;
}


//==================================================
// REGLAS
//==================================================

void IniciarTienda(PartidaTablero& partida, int jugador, EstadoTienda& tienda)
{
    tienda = EstadoTienda{};

    // Oferta: 3 objetos distintos al azar entre los 4 del catalogo.
    int bolsa[CANTIDAD_OBJETOS_TABLERO];

    for (int i = 0; i < CANTIDAD_OBJETOS_TABLERO; i++)
    {
        bolsa[i] = i;
    }

    for (int i = CANTIDAD_OBJETOS_TABLERO - 1; i > 0; i--)
    {
        int otro = GetRandomValue(0, i);
        int temporal = bolsa[i];
        bolsa[i] = bolsa[otro];
        bolsa[otro] = temporal;
    }

    tienda.cantidadOferta =
        OBJETOS_POR_OFERTA_TIENDA < CANTIDAD_OBJETOS_TABLERO
        ? OBJETOS_POR_OFERTA_TIENDA
        : CANTIDAD_OBJETOS_TABLERO;

    for (int i = 0; i < tienda.cantidadOferta; i++)
    {
        tienda.oferta[i] = bolsa[i];
    }

    tienda.proximaAccionBot = 1.0f + 0.6f * (float)GetRandomValue(0, 100) / 100.0f;

    bool bot = partida.participantes != nullptr && partida.participantes[jugador].esBot;

    if (bot)
    {
        char texto[96];

        std::snprintf(texto, sizeof(texto), "J%d MIRA LA MERCADERÍA...", jugador + 1);
        DecirTienda(tienda, texto, 0);
    }
    else
    {
        DecirTienda(tienda, "¡BIENVENIDO! ¿QUÉ TE LLEVÁS HOY?", 0);
    }

    tienda.tiempoMensaje = 99.0f;
}


const char* MotivoNoCompra(const PartidaTablero& partida, int jugador, int id)
{
    if (id < 0 || id >= CANTIDAD_OBJETOS_TABLERO)
    {
        return "ESE OBJETO NO EXISTE";
    }

    const EstadoJugadorPartidaTablero& estado = partida.jugadores[jugador];

    if (InventarioLleno(estado.inventario))
    {
        return "TU MOCHILA ESTÁ LLENA";
    }

    if (estado.monedas < ObtenerDefinicionObjeto(id).precio)
    {
        return "NO TE ALCANZAN LAS MONEDAS";
    }

    return nullptr;
}


bool ComprarObjetoTienda(
    PartidaTablero& partida,
    int jugador,
    int id,
    EstadoTienda& tienda
)
{
    const char* motivo = MotivoNoCompra(partida, jugador, id);

    if (motivo != nullptr)
    {
        DecirTienda(tienda, motivo, 2);
        ReproducirSonidoTablero(partida, SONIDO_ERROR);
        return false;
    }

    EstadoJugadorPartidaTablero& estado = partida.jugadores[jugador];
    const DefinicionObjetoTablero& objeto = ObtenerDefinicionObjeto(id);

    estado.monedas -= objeto.precio;
    AgregarObjetoInventario(estado.inventario, id);

    tienda.compras++;
    tienda.ultimoComprado = id;
    tienda.tiempoCompra = 0.0f;

    char texto[96];

    std::snprintf(texto, sizeof(texto), "¡GRACIAS! SE LLEVA: %s", objeto.nombre);
    DecirTienda(tienda, texto, 1);

    ReproducirSonidoTablero(partida, SONIDO_COMPRA);

    return true;
}


int DecidirCompraBot(
    const PartidaTablero& partida,
    int jugador,
    const EstadoTienda& tienda
)
{
    const EstadoJugadorPartidaTablero& estado = partida.jugadores[jugador];

    if (
        tienda.compras >= COMPRAS_MAXIMAS_BOT ||
        InventarioLleno(estado.inventario)
    )
    {
        return -1;
    }

    // Cerca del trofeo, las monedas son para el trofeo.
    if (estado.monedas >= partida.costoTrofeo - 6)
    {
        return -1;
    }

    // El mas util que pueda pagar dejando una reserva.
    int mejor = -1;
    float mejorPuntos = 0.0f;

    int rivales[MAX_PARTICIPANTES];
    bool hayRivalRico = false;
    int cantidadRivales = ListarRivalesImanes(partida, jugador, rivales);

    for (int r = 0; r < cantidadRivales; r++)
    {
        if (partida.jugadores[rivales[r]].monedas >= 8)
        {
            hayRivalRico = true;
        }
    }

    for (int i = 0; i < tienda.cantidadOferta; i++)
    {
        int id = tienda.oferta[i];

        if (
            id < 0 ||
            (float)ObtenerDefinicionObjeto(id).precio > (float)estado.monedas - RESERVA_MONEDAS_BOT ||
            TieneObjetoInventario(estado.inventario, id)
        )
        {
            continue;
        }

        float puntos = 0.0f;

        switch (id)
        {
            case OBJETO_ESCUDO:
                puntos = (estado.trofeos > 0 || estado.monedas >= 10) ? 3.0f : 2.0f;
                break;

            case OBJETO_IMAN:
                puntos = hayRivalRico ? 3.5f : 1.0f;
                break;

            case OBJETO_BOTAS:
                puntos = 2.5f;
                break;

            case OBJETO_DADO_DOBLE:
                puntos = 2.0f;
                break;

            default:
                break;
        }

        // Pequeno desempate al azar.
        puntos += 0.1f * (float)GetRandomValue(0, 9) / 9.0f;

        if (puntos > mejorPuntos)
        {
            mejorPuntos = puntos;
            mejor = i;
        }
    }

    return mejor;
}


//==================================================
// ACTUALIZACION
//==================================================

void ActualizarTienda(
    PartidaTablero& partida,
    int jugador,
    EstadoTienda& tienda,
    float deltaTime,
    bool esAutomatico,
    bool puedeDecidir,
    const InputSeleccionParticipante& entrada
)
{
    if (tienda.terminado)
    {
        return;
    }

    tienda.tiempoSinAccion += deltaTime;
    tienda.tiempoCompra += deltaTime;

    if (tienda.tiempoMensaje < 90.0f)
    {
        tienda.tiempoMensaje -= deltaTime;
    }

    if (esAutomatico)
    {
        // Bot (o humano desconectado): compra con la regla simple y sale.
        if (tienda.tiempoSinAccion >= tienda.proximaAccionBot)
        {
            int indice = DecidirCompraBot(partida, jugador, tienda);

            if (indice >= 0)
            {
                ComprarObjetoTienda(partida, jugador, tienda.oferta[indice], tienda);
                tienda.proximaAccionBot = tienda.tiempoSinAccion + 1.1f;
            }
            else
            {
                tienda.terminado = true;
            }
        }

        return;
    }

    if (tienda.tiempoSinAccion >= TIEMPO_INACTIVIDAD_TIENDA)
    {
        tienda.terminado = true;
        return;
    }

    if (!puedeDecidir)
    {
        return;
    }

    int opciones = tienda.cantidadOferta + 1;
    int anterior = tienda.seleccion;

    if (entrada.izquierda || entrada.arriba)
    {
        tienda.seleccion = (tienda.seleccion + opciones - 1) % opciones;
    }
    else if (entrada.derecha || entrada.abajo)
    {
        tienda.seleccion = (tienda.seleccion + 1) % opciones;
    }

    if (tienda.seleccion != anterior)
    {
        ReproducirSonidoTablero(partida, SONIDO_UI_MOVER);
        tienda.tiempoSinAccion = 0.0f;
    }

    if (entrada.cancelar)
    {
        ReproducirSonidoTablero(partida, SONIDO_UI_CANCELAR);
        tienda.terminado = true;
        return;
    }

    if (entrada.confirmar)
    {
        tienda.tiempoSinAccion = 0.0f;

        if (tienda.seleccion >= tienda.cantidadOferta)
        {
            ReproducirSonidoTablero(partida, SONIDO_UI_CONFIRMAR);
            tienda.terminado = true;
        }
        else
        {
            ComprarObjetoTienda(partida, jugador, tienda.oferta[tienda.seleccion], tienda);
        }
    }
}


//==================================================
// TEXTOS
//==================================================

void TextoTienda(
    const PartidaTablero& partida,
    const EstadoTienda& tienda,
    int jugador,
    char* destino,
    int capacidad
)
{
    bool bot = partida.participantes != nullptr && partida.participantes[jugador].esBot;

    if (tienda.compras > 0 && tienda.tipoMensaje == 1 && tienda.tiempoMensaje > 0.0f)
    {
        std::snprintf(destino, capacidad, "%s", tienda.mensaje);
    }
    else if (bot)
    {
        std::snprintf(destino, capacidad, "J%d ESTÁ DE COMPRAS EN LA TIENDA", jugador + 1);
    }
    else
    {
        std::snprintf(destino, capacidad, "TIENDA: ELEGÍ QUÉ COMPRAR O SALÍ");
    }
}


//==================================================
// DIBUJO
//==================================================

static void DibujarComerciante(Vector2 c, float escala, float tiempo, bool reducir)
{
    float salto = reducir ? 0.0f : std::sin(tiempo * 2.4f) * 3.0f * escala;
    float x = c.x;
    float y = c.y + salto;

    Color tunica = Color{ 120, 78, 52, 255 };
    Color piel = Color{ 236, 190, 150, 255 };
    Color sombrero = Color{ 84, 138, 70, 255 };
    Color tinta = UI_TINTA;

    // Sombra.
    DrawEllipse((int)x, (int)(c.y + 112.0f * escala), 50.0f * escala, 9.0f * escala, Fade(BLACK, 0.3f));

    // Mochila a la espalda y tunica.
    DrawCircleV(Vector2{ x + 34.0f * escala, y + 40.0f * escala }, 26.0f * escala + 2.0f, tinta);
    DrawCircleV(Vector2{ x + 34.0f * escala, y + 40.0f * escala }, 26.0f * escala, Color{ 190, 140, 70, 255 });
    DrawEllipse((int)x, (int)(y + 62.0f * escala), 44.0f * escala + 2.0f, 54.0f * escala + 2.0f, tinta);
    DrawEllipse((int)x, (int)(y + 62.0f * escala), 44.0f * escala, 54.0f * escala, tunica);
    DrawRectangle((int)(x - 44.0f * escala), (int)(y + 70.0f * escala), (int)(88.0f * escala), (int)(8.0f * escala), Fade(UI_CUERDA, 0.9f));

    // Cabeza.
    DrawCircleV(Vector2{ x, y - 6.0f * escala }, 30.0f * escala + 2.0f, tinta);
    DrawCircleV(Vector2{ x, y - 6.0f * escala }, 30.0f * escala, piel);
    DrawCircleV(Vector2{ x - 10.0f * escala, y - 10.0f * escala }, 3.6f * escala, tinta);
    DrawCircleV(Vector2{ x + 10.0f * escala, y - 10.0f * escala }, 3.6f * escala, tinta);
    DrawEllipse((int)(x - 10.0f * escala), (int)(y + 8.0f * escala), 11.0f * escala, 5.0f * escala, Color{ 90, 58, 40, 255 });
    DrawEllipse((int)(x + 10.0f * escala), (int)(y + 8.0f * escala), 11.0f * escala, 5.0f * escala, Color{ 90, 58, 40, 255 });

    // Sombrero de ala ancha.
    DrawEllipse((int)x, (int)(y - 28.0f * escala), 44.0f * escala + 2.0f, 10.0f * escala + 2.0f, tinta);
    DrawEllipse((int)x, (int)(y - 28.0f * escala), 44.0f * escala, 10.0f * escala, sombrero);
    DrawTriangle(
        Vector2{ x, y - 70.0f * escala },
        Vector2{ x + 24.0f * escala, y - 30.0f * escala },
        Vector2{ x - 24.0f * escala, y - 30.0f * escala },
        sombrero
    );
    DrawRectangle((int)(x - 24.0f * escala), (int)(y - 38.0f * escala), (int)(48.0f * escala), (int)(7.0f * escala), Color{ 255, 214, 64, 255 });
}


static void DibujarCartaOferta(
    Rectangle carta,
    int id,
    int precio,
    bool resaltar,
    bool apagada,
    float rebote,
    float alfa
)
{
    float desplazar = rebote * Px(6.0f);

    carta.y -= desplazar;

    DrawRectangleRounded(Rectangle{ carta.x + Px(3.0f), carta.y + Px(5.0f), carta.width, carta.height }, 0.10f, 6, Fade(BLACK, 0.3f * alfa));
    DrawRectangleRounded(carta, 0.10f, 6, Fade(UI_TINTA, alfa));
    DrawRectangleRounded(
        Rectangle{ carta.x + Px(3.0f), carta.y + Px(3.0f), carta.width - Px(6.0f), carta.height - Px(6.0f) },
        0.10f, 6, Fade(Color{ 255, 248, 232, 255 }, alfa)
    );

    const DefinicionObjetoTablero& objeto = ObtenerDefinicionObjeto(id);

    // Banda de color del objeto y su icono.
    DrawRectangleRounded(
        Rectangle{ carta.x + carta.width * 0.07f, carta.y + carta.height * 0.05f, carta.width * 0.86f, carta.height * 0.07f },
        0.5f, 6, Fade(objeto.color, alfa)
    );
    DibujarIconoObjeto(id, Vector2{ carta.x + carta.width * 0.5f, carta.y + carta.height * 0.30f }, carta.width * 0.23f, alfa);

    DibujarTextoAjustadoUI(
        objeto.nombre, carta.x + carta.width * 0.5f, carta.y + carta.height * 0.50f,
        UI_TEXTO, carta.width * 0.88f, Fade(UI_TINTA, alfa), true
    );

    // Precio: moneda + numero.
    const char* texto = TextFormat("%d", precio);
    int tamano = TamanoTexto(UI_TEXTO);
    float anchoTexto = (float)MeasureText(texto, tamano);
    float izquierda = carta.x + (carta.width - anchoTexto - Px(22.0f)) * 0.5f;
    float yPrecio = carta.y + carta.height * 0.62f;

    DibujarMonedaUI(Vector2{ izquierda + Px(8.0f), yPrecio + (float)tamano * 0.5f }, 8.0f, alfa);
    DrawText(texto, (int)(izquierda + Px(22.0f)), (int)yPrecio, tamano, apagada ? Fade(COLOR_TINTA_PRECIO_TIENDA, alfa) : Fade(UI_TINTA, alfa));

    DibujarParrafoUI(
        objeto.descripcion,
        carta.x + carta.width * 0.07f, carta.y + carta.height * 0.74f,
        UI_TEXTO_PEQUENO - 3, carta.width * 0.86f, Fade(UI_TINTA_SUAVE, alfa),
        UI_ALINEAR_CENTRO, carta.height * 0.21f
    );

    if (apagada)
    {
        DrawRectangleRounded(carta, 0.10f, 6, Fade(UI_VELO, 0.30f * alfa));
    }

    if (resaltar)
    {
        float halo = Px(4.0f);

        DrawRectangleRoundedLinesEx(
            Rectangle{ carta.x - halo, carta.y - halo, carta.width + halo * 2.0f, carta.height + halo * 2.0f },
            0.12f, 6, Px(UI_BORDE_FOCO), Fade(UI_FOCO, alfa)
        );
    }
}


void DibujarTienda2D(
    const PartidaTablero& partida,
    const EstadoTienda& tienda,
    int jugador,
    float tiempoTotal
)
{
    const EstiloUI& estilo = ObtenerEstiloUI();

    if (!estilo.cargado || jugador < 0)
    {
        return;
    }

    const EstadoJugadorPartidaTablero& estado = partida.jugadores[jugador];
    float entrada = estilo.reducirMovimiento ? 1.0f : Limitar01Tienda(tiempoTotal / 0.25f);
    float cx = (float)GetScreenWidth() * 0.5f;
    bool humano = partida.participantes != nullptr && !partida.participantes[jugador].esBot;

    DibujarVeloUI(0.58f * entrada);

    DibujarTextoMundoContornoUI("¡TIENDA!", cx, RectRef(0.0f, 166.0f, 0.0f, 0.0f).y, UI_TEXTO_GRANDE, UI_FOCO, entrada);
    DibujarTextoMundoContornoUI(
        "UN COMERCIANTE DEL BOSQUE OFRECE TRES OBJETOS",
        cx, RectRef(0.0f, 202.0f, 0.0f, 0.0f).y, UI_TEXTO, UI_TEXTO_MUNDO, entrada
    );

    // Comerciante.
    Rectangle ref = RectRef(190.0f, 330.0f, 0.0f, 0.0f);
    DibujarComerciante(Vector2{ ref.x, ref.y }, EscalaUI(), estilo.tiempo, estilo.reducirMovimiento);

    // Ofertas.
    float centros[OBJETOS_POR_OFERTA_TIENDA] = { 430.0f, 620.0f, 810.0f };

    for (int i = 0; i < tienda.cantidadOferta; i++)
    {
        int id = tienda.oferta[i];
        Rectangle carta = RectRef(centros[i] - 80.0f, 245.0f, 160.0f, 215.0f);
        bool apagada = MotivoNoCompra(partida, jugador, id) != nullptr;
        float rebote = 0.0f;

        if (tienda.ultimoComprado == id && tienda.tiempoCompra < 0.3f && !estilo.reducirMovimiento)
        {
            rebote = 1.0f - tienda.tiempoCompra / 0.3f;
        }

        DibujarCartaOferta(
            carta, id, ObtenerDefinicionObjeto(id).precio,
            humano && tienda.seleccion == i, apagada, rebote, entrada
        );
    }

    // Salir.
    {
        Rectangle salir = RectRef(960.0f, 290.0f, 110.0f, 120.0f);
        bool resaltar = humano && tienda.seleccion >= tienda.cantidadOferta;

        DrawRectangleRounded(Rectangle{ salir.x + Px(3.0f), salir.y + Px(5.0f), salir.width, salir.height }, 0.14f, 6, Fade(BLACK, 0.3f * entrada));
        DrawRectangleRounded(salir, 0.14f, 6, Fade(UI_TINTA, entrada));
        DrawRectangleRounded(
            Rectangle{ salir.x + Px(3.0f), salir.y + Px(3.0f), salir.width - Px(6.0f), salir.height - Px(6.0f) },
            0.14f, 6, Fade(Color{ 255, 248, 232, 255 }, entrada)
        );

        DibujarFlechaUI(salir.x + salir.width * 0.5f, salir.y + salir.height * 0.34f, Px(14.0f), 1, Fade(UI_TINTA, entrada));
        DibujarTextoAjustadoUI("SALIR", salir.x + salir.width * 0.5f, salir.y + salir.height * 0.62f, UI_TEXTO, salir.width * 0.84f, Fade(UI_TINTA, entrada), true);

        if (resaltar)
        {
            float halo = Px(4.0f);

            DrawRectangleRoundedLinesEx(
                Rectangle{ salir.x - halo, salir.y - halo, salir.width + halo * 2.0f, salir.height + halo * 2.0f },
                0.16f, 6, Px(UI_BORDE_FOCO), UI_FOCO
            );
        }
    }

    // Frase del comerciante.
    if (tienda.mensaje[0] != '\0')
    {
        Rectangle banda = RectRef(350.0f, 478.0f, 720.0f, 46.0f);
        Rectangle placa = DibujarPlacaConMarcoUI(estilo, banda, 6.0f, entrada);
        Color color = tienda.tipoMensaje == 2 ? COLOR_TINTA_ERROR_TIENDA : UI_TINTA;
        int tamano = TamanoTextoAjustadoUI(tienda.mensaje, UI_TEXTO, placa.width - Px(20.0f));

        DibujarTextoAjustadoUI(
            tienda.mensaje, placa.x + placa.width * 0.5f, placa.y + (placa.height - (float)tamano) * 0.5f,
            UI_TEXTO, placa.width - Px(20.0f), Fade(color, entrada), true
        );
    }

    // Mochila del jugador y sus monedas.
    DibujarTextoDerechaUI(
        "TU MOCHILA:", RectRef(438.0f, 0.0f, 0.0f, 0.0f).x, RectRef(0.0f, 539.0f, 0.0f, 0.0f).y,
        UI_TEXTO_PEQUENO, Px(110.0f), UI_TEXTO_MUNDO
    );

    for (int e = 0; e < MAX_OBJETOS_INVENTARIO; e++)
    {
        DibujarEspacioObjeto(RectRef(446.0f + 46.0f * (float)e, 528.0f, 38.0f, 38.0f), estado.inventario[e], false, entrada);
    }

    const char* monedas = TextFormat("%d", estado.monedas);
    int tamanoMonedas = TamanoTexto(UI_TEXTO);
    Vector2 posMonedas = { RectRef(612.0f, 0.0f, 0.0f, 0.0f).x, RectRef(0.0f, 547.0f, 0.0f, 0.0f).y };

    DibujarMonedaUI(Vector2{ posMonedas.x + Px(8.0f), posMonedas.y + (float)tamanoMonedas * 0.5f }, 8.0f, entrada);
    DibujarTextoMundoContornoUI(monedas, posMonedas.x + Px(34.0f), posMonedas.y - Px(3.0f), UI_TEXTO, UI_TEXTO_MUNDO, entrada);
}
