#include "Gameplay/TiendaTablero.h"

#include "Entities/PersonajesBloque.h"
#include "Gameplay/PartidaTablero.h"
#include "UI/EstiloUI.h"

#include "rlgl.h"
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
    tienda.tiempoDesdeMensaje = 0.0f;
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
        DecirTienda(tienda, "¡PASE, PASE! MIRE SIN COMPROMISO", 0);
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

        // El vendedor se sorprende (solo presentacion).
        tienda.reaccion = InventarioLleno(partida.jugadores[jugador].inventario)
            ? REACCION_VENDEDOR_MOCHILA_LLENA
            : REACCION_VENDEDOR_SIN_MONEDAS;
        tienda.tiempoReaccion = 0.0f;
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
    tienda.reaccion = REACCION_VENDEDOR_COMPRA;
    tienda.tiempoReaccion = 0.0f;

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
    tienda.tiempoReaccion += deltaTime;
    tienda.tiempoDesdeMensaje += deltaTime;
    tienda.tiempoSeleccion += deltaTime;
    tienda.tiempoTotal += deltaTime;

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
        tienda.tiempoSeleccion = 0.0f;
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
//
// El puesto vive en el tablero: un carrito de madera con toldo a rayas al
// costado de la casilla, el mostrador con los tres objetos de la oferta y
// Don Cambalache detras. La interfaz 2D se reduce a una columna de ofertas
// a la izquierda y el globo con la frase del vendedor.

static const float ESCALA_VENDEDOR = 0.33f;     // las fichas usan 0.18
static const float ESCALA_PUESTO = 1.3f;        // carrito algo mayor que una casilla
static const float ALTURA_PISO_PUESTO = 0.47f;  // igual que los pies de las fichas


static float SuaveTienda(float u)
{
    u = Limitar01Tienda(u);
    return u * u * (3.0f - 2.0f * u);
}


// Direccion horizontal hacia la camara de partida (fija por tablero: el
// puesto no se mueve aunque la camara se acerque).
static Vector3 VistaTienda(const PartidaTablero& partida)
{
    const DefinicionTablero& def = *partida.definicion;
    Vector3 v = { def.camaraPosicion.x - def.camaraObjetivo.x, 0.0f, def.camaraPosicion.z - def.camaraObjetivo.z };
    float largo = std::sqrt(v.x * v.x + v.z * v.z);

    if (largo < 0.001f)
    {
        return { 0.0f, 0.0f, 1.0f };
    }

    return { v.x / largo, 0.0f, v.z / largo };
}


// Hacia donde mira el puesto: al jugador y un poco a la camara.
static Vector3 FrentePuesto(const PartidaTablero& partida)
{
    Vector3 v = VistaTienda(partida);
    Vector3 derecha = { v.z, 0.0f, -v.x };
    Vector3 f = { -derecha.x * 0.55f + v.x * 0.85f, 0.0f, -derecha.z * 0.55f + v.z * 0.85f };
    float largo = std::sqrt(f.x * f.x + f.z * f.z);

    return { f.x / largo, 0.0f, f.z / largo };
}


Vector3 PosicionPuestoTienda(const PartidaTablero& partida, int jugador)
{
    if (partida.definicion == nullptr || jugador < 0 || jugador >= MAX_PARTICIPANTES)
    {
        return Vector3{};
    }

    const Casilla* casilla = partida.tablero.ObtenerCasilla(partida.jugadores[jugador].casillaActual);
    Vector3 base = casilla != nullptr ? casilla->posicion : partida.jugadores[jugador].posicionVisual;
    Vector3 v = VistaTienda(partida);
    Vector3 derecha = { v.z, 0.0f, -v.x };

    return {
        base.x + derecha.x * 2.3f - v.x * 0.35f,
        base.y + ALTURA_PISO_PUESTO,
        base.z + derecha.z * 2.3f - v.z * 0.35f
    };
}


Vector3 PosicionVendedorTienda(const PartidaTablero& partida, int jugador)
{
    Vector3 puesto = PosicionPuestoTienda(partida, jugador);
    Vector3 f = FrentePuesto(partida);

    const float atras = 0.32f * ESCALA_PUESTO;

    return { puesto.x - f.x * atras, puesto.y, puesto.z - f.z * atras };
}


void DibujarPuestoTienda3D(
    const PartidaTablero& partida,
    const EstadoTienda& tienda,
    int jugador
)
{
    if (partida.definicion == nullptr || jugador < 0)
    {
        return;
    }

    const bool reducir = ObtenerEstiloUI().reducirMovimiento;
    const float t = partida.tiempoTablero;
    const Vector3 puesto = PosicionPuestoTienda(partida, jugador);
    const Vector3 frente = FrentePuesto(partida);
    const float giro = std::atan2(frente.x, frente.z) * RAD2DEG;

    // Aparicion: el carrito crece con un rebote y el vendedor asoma.
    float aparece = reducir ? 1.0f : SuaveTienda(tienda.tiempoTotal / 0.35f);
    float rebote = reducir ? 1.0f : aparece + 0.12f * std::sin(Limitar01Tienda(tienda.tiempoTotal / 0.5f) * PI) ;

    const Color madera = { 150, 98, 58, 255 };
    const Color maderaOscura = { 104, 66, 40, 255 };
    const Color rojo = { 214, 58, 52, 255 };
    const Color crema = { 250, 244, 232, 255 };

    rlPushMatrix();
    rlTranslatef(puesto.x, puesto.y, puesto.z);
    rlRotatef(giro, 0.0f, 1.0f, 0.0f);
    rlScalef(rebote * ESCALA_PUESTO, rebote * ESCALA_PUESTO, rebote * ESCALA_PUESTO);

    // Sombra, plataforma y ruedas (el carrito trae su propio piso).
    DrawCylinder({ 0.0f, -0.02f, 0.0f }, 1.05f, 1.05f, 0.01f, 16, Color{ 20, 20, 30, 90 });
    DrawCube({ 0.0f, -0.09f, -0.05f }, 1.75f, 0.16f, 1.15f, madera);
    DrawCubeWires({ 0.0f, -0.09f, -0.05f }, 1.75f, 0.16f, 1.15f, maderaOscura);

    for (int lado = -1; lado <= 1; lado += 2)
    {
        DrawCylinderEx({ lado * 0.90f, -0.18f, 0.20f }, { lado * 0.98f, -0.18f, 0.20f }, 0.26f, 0.26f, 12, maderaOscura);
        DrawCylinderEx({ lado * 0.97f, -0.18f, 0.20f }, { lado * 1.00f, -0.18f, 0.20f }, 0.09f, 0.09f, 8, Color{ 230, 190, 80, 255 });
    }

    // Postes y toldo a rayas.
    for (int lado = -1; lado <= 1; lado += 2)
    {
        DrawCube({ lado * 0.78f, 1.05f, -0.52f }, 0.08f, 2.10f, 0.08f, maderaOscura);
    }

    for (int k = 0; k < 6; k++)
    {
        float x = -0.80f + 0.32f * ((float)k + 0.5f);

        rlPushMatrix();
        // Toldo alto y hacia atras: deja ver la cara del vendedor.
        rlTranslatef(x, 2.10f, -0.62f);
        rlRotatef(-10.0f, 1.0f, 0.0f, 0.0f);
        DrawCube({ 0.0f, 0.0f, 0.0f }, 0.32f, 0.06f, 0.55f, k % 2 == 0 ? rojo : crema);
        rlPopMatrix();
    }

    // Mostrador con tabla clara y cartel de moneda.
    DrawCube({ 0.0f, 0.30f, 0.18f }, 1.40f, 0.60f, 0.40f, madera);
    DrawCube({ 0.0f, 0.62f, 0.18f }, 1.52f, 0.06f, 0.50f, Color{ 196, 140, 88, 255 });
    DrawCube({ 0.0f, 0.30f, 0.385f }, 0.36f, 0.36f, 0.02f, Color{ 255, 206, 60, 255 });
    DrawCube({ 0.0f, 0.30f, 0.392f }, 0.20f, 0.20f, 0.02f, Color{ 230, 160, 40, 255 });

    // La oferta sobre el mostrador; lo que se acaba de comprar salta.
    for (int i = 0; i < tienda.cantidadOferta; i++)
    {
        int id = tienda.oferta[i];

        if (id < 0)
        {
            continue;
        }

        float x = -0.45f + 0.45f * (float)i;
        float salto = 0.0f;

        if (!reducir && tienda.ultimoComprado == id && tienda.tiempoCompra < 0.6f)
        {
            salto = 0.35f * std::sin(tienda.tiempoCompra / 0.6f * PI);
        }
        else if (!reducir)
        {
            salto = 0.02f * std::sin(t * 2.5f + (float)i);
        }

        Color color = ObtenerDefinicionObjeto(id).color;
        DrawCube({ x, 0.77f + salto, 0.18f }, 0.22f, 0.22f, 0.22f, color);
        DrawCubeWires({ x, 0.77f + salto, 0.18f }, 0.22f, 0.22f, 0.22f, Fade(BLACK, 0.35f));
    }

    rlPopMatrix();

    //------------------------------
    // DON CAMBALACHE
    //------------------------------

    Vector3 pies = PosicionVendedorTienda(partida, jugador);
    pies.y -= (1.0f - aparece) * 1.0f;

    PosePersonajeBloque pose;
    pose.tiempo = t;
    pose.hablando = tienda.tiempoDesdeMensaje < 1.4f;
    pose.giroCabeza = reducir ? 0.0f : 0.18f * std::sin(t * 0.9f);

    const bool reaccionando = tienda.tiempoReaccion < 1.3f;

    if (reaccionando && tienda.reaccion == REACCION_VENDEDOR_COMPRA)
    {
        // Festeja la venta con saltitos.
        pose.gesto = GESTO_FESTEJAR;
        pose.pesoGesto = SuaveTienda(tienda.tiempoReaccion / 0.15f);

        if (!reducir)
        {
            pies.y += std::fabs(std::sin(tienda.tiempoReaccion * 9.0f)) * 0.22f * (1.0f - tienda.tiempoReaccion / 1.3f);
        }
    }
    else if (
        reaccionando &&
        (tienda.reaccion == REACCION_VENDEDOR_SIN_MONEDAS || tienda.reaccion == REACCION_VENDEDOR_MOCHILA_LLENA)
    )
    {
        // Sorpresa: ojos grandes, boca en O y un respingo.
        pose.gesto = GESTO_SORPRESA;
        pose.pesoGesto = SuaveTienda(tienda.tiempoReaccion / 0.12f);
        pose.susto = 1.0f - SuaveTienda((tienda.tiempoReaccion - 0.9f) / 0.4f);
        pose.tambaleo = reducir ? 0.0f : 0.5f * std::sin(Limitar01Tienda(tienda.tiempoReaccion / 0.4f) * PI);
    }
    else if (tienda.tiempoTotal < 1.9f)
    {
        // Saluda al llegar el jugador.
        pose.gesto = GESTO_SALUDAR;
        pose.pesoGesto = SuaveTienda((tienda.tiempoTotal - 0.25f) / 0.25f) * (1.0f - SuaveTienda((tienda.tiempoTotal - 1.6f) / 0.3f));
    }
    else if (tienda.tiempoSeleccion < 0.9f)
    {
        // Muestra la mercaderia cuando el jugador cambia de producto.
        pose.gesto = GESTO_PRESENTAR;
        pose.pesoGesto = SuaveTienda(tienda.tiempoSeleccion / 0.15f) * (1.0f - SuaveTienda((tienda.tiempoSeleccion - 0.6f) / 0.3f));
    }

    // Mira al jugador (que esta del lado de la casilla).
    DibujarVendedorBloque(pies, giro - 18.0f, ESCALA_VENDEDOR, pose);
}


// Carta compacta de una oferta (icono, nombre, precio). La seleccionada
// muestra ademas la descripcion del objeto.
static float DibujarCartaOferta(
    float x,
    float y,
    float ancho,
    int id,
    bool resaltar,
    bool apagada,
    float rebote,
    float alfa
)
{
    const DefinicionObjetoTablero& objeto = ObtenerDefinicionObjeto(id);
    const float alto = resaltar ? Px(96.0f) : Px(62.0f);

    Rectangle carta = { x + rebote * Px(8.0f), y, ancho, alto };

    DrawRectangleRounded(Rectangle{ carta.x + Px(3.0f), carta.y + Px(4.0f), carta.width, carta.height }, 0.18f, 6, Fade(BLACK, 0.30f * alfa));
    DrawRectangleRounded(carta, 0.18f, 6, Fade(UI_TINTA, alfa));
    DrawRectangleRounded(
        Rectangle{ carta.x + Px(3.0f), carta.y + Px(3.0f), carta.width - Px(6.0f), carta.height - Px(6.0f) },
        0.18f, 6, Fade(Color{ 255, 248, 232, 255 }, alfa)
    );

    // Franja de color del objeto e icono.
    DrawRectangleRounded(Rectangle{ carta.x + Px(6.0f), carta.y + Px(6.0f), Px(6.0f), carta.height - Px(12.0f) }, 0.5f, 4, Fade(objeto.color, alfa));
    DibujarIconoObjeto(id, Vector2{ carta.x + Px(40.0f), carta.y + Px(31.0f) }, Px(19.0f), alfa);

    float xTexto = carta.x + Px(68.0f);
    float anchoTexto = carta.width - Px(76.0f);

    DibujarTextoAjustadoUI(objeto.nombre, xTexto + anchoTexto * 0.5f, carta.y + Px(9.0f), UI_TEXTO, anchoTexto, Fade(UI_TINTA, alfa), true);

    // Precio: moneda + numero (rojo si no alcanza).
    const char* texto = TextFormat("%d", objeto.precio);
    int tamano = TamanoTexto(UI_TEXTO);
    float anchoPrecio = (float)MeasureText(texto, tamano) + Px(22.0f);
    float xPrecio = xTexto + (anchoTexto - anchoPrecio) * 0.5f;
    float yPrecio = carta.y + Px(33.0f);

    DibujarMonedaUI(Vector2{ xPrecio + Px(8.0f), yPrecio + (float)tamano * 0.5f }, 8.0f, alfa);
    DrawText(texto, (int)(xPrecio + Px(22.0f)), (int)yPrecio, tamano, apagada ? Fade(COLOR_TINTA_PRECIO_TIENDA, alfa) : Fade(UI_TINTA, alfa));

    if (resaltar)
    {
        DibujarParrafoUI(
            objeto.descripcion,
            carta.x + Px(12.0f), carta.y + Px(60.0f),
            UI_TEXTO_PEQUENO - 3, carta.width - Px(24.0f), Fade(UI_TINTA_SUAVE, alfa),
            UI_ALINEAR_CENTRO, Px(30.0f)
        );

        float halo = Px(4.0f);

        DrawRectangleRoundedLinesEx(
            Rectangle{ carta.x - halo, carta.y - halo, carta.width + halo * 2.0f, carta.height + halo * 2.0f },
            0.2f, 6, Px(UI_BORDE_FOCO), Fade(UI_FOCO, alfa)
        );
    }

    if (apagada)
    {
        DrawRectangleRounded(carta, 0.18f, 6, Fade(UI_VELO, 0.28f * alfa));
    }

    return alto;
}


// Globo de historieta con la frase del vendedor, sobre su cabeza.
static void DibujarGloboVendedor(
    const EstadoTienda& tienda,
    Vector2 cabeza,
    float alfa
)
{
    if (tienda.mensaje[0] == '\0' || alfa <= 0.0f)
    {
        return;
    }

    const float ancho = Px(330.0f);
    MedidaParrafoUI medida = MedirParrafoUI(tienda.mensaje, UI_TEXTO, ancho - Px(28.0f), Px(70.0f));
    const float alto = medida.alto + Px(24.0f);

    // Al costado de la cabeza (a la derecha si entra), por debajo de los
    // carteles del HUD y sin tapar al vendedor.
    float x = cabeza.x + Px(46.0f);
    float y = cabeza.y - alto * 0.5f - Px(30.0f);
    float maximoX = (float)GetScreenWidth() - ancho - Px(16.0f);

    if (x > maximoX) x = cabeza.x - Px(46.0f) - ancho;
    if (x < Px(280.0f)) x = Px(280.0f);
    if (y < Px(178.0f)) y = Px(178.0f);
    if (y > Px(470.0f) - alto) y = Px(470.0f) - alto;

    // Entra con un pequeno salto en cada frase nueva.
    float pop = tienda.tiempoDesdeMensaje < 0.15f ? 0.92f + 0.08f * (tienda.tiempoDesdeMensaje / 0.15f) : 1.0f;
    Rectangle r = { x + ancho * (1.0f - pop) * 0.5f, y + alto * (1.0f - pop), ancho * pop, alto * pop };
    Color papel = { 255, 248, 232, 255 };
    Color tinta = tienda.tipoMensaje == 2 ? COLOR_TINTA_ERROR_TIENDA : UI_TINTA;

    // Cola desde el borde del globo mas cercano a la cabeza.
    const bool aLaDerecha = r.x > cabeza.x;
    const float borde = aLaDerecha ? r.x + Px(4.0f) : r.x + r.width - Px(4.0f);
    const float yCola = r.y + r.height * 0.62f;
    const float sentido = aLaDerecha ? -1.0f : 1.0f;
    Vector2 punta = { borde + sentido * Px(26.0f), yCola + Px(10.0f) };
    Vector2 arriba = { borde, yCola - Px(12.0f) };
    Vector2 abajo = { borde, yCola + Px(12.0f) };

    if (aLaDerecha)
    {
        DrawTriangle(arriba, punta, abajo, Fade(UI_TINTA, alfa));
    }
    else
    {
        DrawTriangle(arriba, abajo, punta, Fade(UI_TINTA, alfa));
    }

    DrawRectangleRounded({ r.x + Px(3.0f), r.y + Px(5.0f), r.width, r.height }, 0.35f, 8, Fade(BLACK, 0.28f * alfa));
    DrawRectangleRounded({ r.x - Px(3.0f), r.y - Px(3.0f), r.width + Px(6.0f), r.height + Px(6.0f) }, 0.35f, 8, Fade(UI_TINTA, alfa));
    DrawRectangleRounded(r, 0.35f, 8, Fade(papel, alfa));

    Vector2 puntaClara = { punta.x - sentido * Px(7.0f), punta.y - Px(3.0f) };
    Vector2 arribaClara = { borde - sentido * Px(5.0f), yCola - Px(7.0f) };
    Vector2 abajoClara = { borde - sentido * Px(5.0f), yCola + Px(7.0f) };

    if (aLaDerecha)
    {
        DrawTriangle(arribaClara, puntaClara, abajoClara, Fade(papel, alfa));
    }
    else
    {
        DrawTriangle(arribaClara, abajoClara, puntaClara, Fade(papel, alfa));
    }

    DibujarParrafoUI(
        tienda.mensaje, r.x + Px(14.0f), r.y + (r.height - medida.alto) * 0.5f,
        UI_TEXTO, r.width - Px(28.0f), Fade(tinta, alfa), UI_ALINEAR_CENTRO, Px(70.0f)
    );
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
    const float entrada = estilo.reducirMovimiento ? 1.0f : Limitar01Tienda(tiempoTotal / 0.25f);
    const bool humano = partida.participantes != nullptr && !partida.participantes[jugador].esBot;
    const float deslizar = estilo.reducirMovimiento ? 0.0f : (1.0f - SuaveTienda(tiempoTotal / 0.3f)) * Px(-260.0f);

    // Frase del vendedor sobre su cabeza.
    Vector3 cabeza3D = PosicionVendedorTienda(partida, jugador);
    cabeza3D.y += 2.1f;

    DibujarGloboVendedor(tienda, GetWorldToScreen(cabeza3D, partida.camara), entrada);

    // Columna de ofertas a la izquierda (el puesto queda a la vista).
    const float x = Px(28.0f) + deslizar;
    const float ancho = Px(236.0f);
    float y = Px(118.0f);

    for (int i = 0; i < tienda.cantidadOferta; i++)
    {
        int id = tienda.oferta[i];
        bool apagada = MotivoNoCompra(partida, jugador, id) != nullptr;
        float rebote = 0.0f;

        if (tienda.ultimoComprado == id && tienda.tiempoCompra < 0.3f && !estilo.reducirMovimiento)
        {
            rebote = 1.0f - tienda.tiempoCompra / 0.3f;
        }

        y += DibujarCartaOferta(x, y, ancho, id, humano && tienda.seleccion == i, apagada, rebote, entrada) + Px(10.0f);
    }

    // Salir.
    {
        Rectangle salir = { x, y, ancho, Px(40.0f) };
        bool resaltar = humano && tienda.seleccion >= tienda.cantidadOferta;

        DrawRectangleRounded(Rectangle{ salir.x + Px(3.0f), salir.y + Px(4.0f), salir.width, salir.height }, 0.3f, 6, Fade(BLACK, 0.3f * entrada));
        DrawRectangleRounded(salir, 0.3f, 6, Fade(UI_TINTA, entrada));
        DrawRectangleRounded(
            Rectangle{ salir.x + Px(3.0f), salir.y + Px(3.0f), salir.width - Px(6.0f), salir.height - Px(6.0f) },
            0.3f, 6, Fade(Color{ 255, 248, 232, 255 }, entrada)
        );

        DibujarFlechaUI(salir.x + Px(26.0f), salir.y + salir.height * 0.5f, Px(10.0f), 1, Fade(UI_TINTA, entrada));
        DibujarTextoAjustadoUI("SALIR", salir.x + salir.width * 0.5f, salir.y + (salir.height - (float)TamanoTexto(UI_TEXTO)) * 0.5f, UI_TEXTO, salir.width * 0.6f, Fade(UI_TINTA, entrada), true);

        if (resaltar)
        {
            float halo = Px(4.0f);

            DrawRectangleRoundedLinesEx(
                Rectangle{ salir.x - halo, salir.y - halo, salir.width + halo * 2.0f, salir.height + halo * 2.0f },
                0.32f, 6, Px(UI_BORDE_FOCO), Fade(UI_FOCO, entrada)
            );
        }

        y += salir.height + Px(12.0f);
    }

    // Mochila y monedas de quien compra, bajo la columna.
    for (int e = 0; e < MAX_OBJETOS_INVENTARIO; e++)
    {
        DibujarEspacioObjeto(Rectangle{ x + Px(46.0f) * (float)e, y, Px(38.0f), Px(38.0f) }, estado.inventario[e], false, entrada);
    }

    const char* monedas = TextFormat("%d", estado.monedas);
    int tamanoMonedas = TamanoTexto(UI_TEXTO);
    Vector2 posMonedas = { x + Px(148.0f), y + Px(19.0f) - (float)tamanoMonedas * 0.5f };

    DibujarMonedaUI(Vector2{ posMonedas.x + Px(8.0f), posMonedas.y + (float)tamanoMonedas * 0.5f }, 8.0f, entrada);
    DibujarTextoMundoContornoUI(monedas, posMonedas.x + Px(34.0f), posMonedas.y - Px(3.0f), UI_TEXTO, UI_TEXTO_MUNDO, entrada);
}
