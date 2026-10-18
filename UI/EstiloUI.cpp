#include "UI/EstiloUI.h"

#include "rlgl.h"

#include <cmath>
#include <cstdio>


//==================================================
// UTILIDADES
//==================================================

static float Limitar(float v, float minimo, float maximo)
{
    return v < minimo ? minimo : (v > maximo ? maximo : v);
}


static float Hash01(int semilla)
{
    unsigned int x = (unsigned int)semilla * 2654435761u;
    x ^= x >> 15;
    x *= 2246822519u;
    x ^= x >> 13;

    return (float)(x & 0xFFFFu) / 65535.0f;
}


static Color Escalar(Color c, float factor)
{
    float r = c.r * factor;
    float g = c.g * factor;
    float b = c.b * factor;

    return Color{
        (unsigned char)Limitar(r, 0.0f, 255.0f),
        (unsigned char)Limitar(g, 0.0f, 255.0f),
        (unsigned char)Limitar(b, 0.0f, 255.0f),
        c.a
    };
}


// Multiplica el alfa propio del color (a diferencia de Fade, que lo
// reemplaza): con alfa = 1 devuelve el color sin cambios.
static Color MultiplicarAlfa(Color c, float alfa)
{
    if (alfa >= 1.0f)
    {
        return c;
    }

    c.a = (unsigned char)(c.a * Limitar(alfa, 0.0f, 1.0f));

    return c;
}


//==================================================
// ESCALA Y LAYOUT
//==================================================

float EscalaUI()
{
    float ancho = (float)GetScreenWidth() / UI_REF_ANCHO;
    float alto = (float)GetScreenHeight() / UI_REF_ALTO;

    return ancho < alto ? ancho : alto;
}


float Px(float unidades)
{
    return unidades * EscalaUI();
}


Rectangle RectRef(float x, float y, float ancho, float alto)
{
    float escala = EscalaUI();
    float origenX = ((float)GetScreenWidth() - UI_REF_ANCHO * escala) * 0.5f;
    float origenY = ((float)GetScreenHeight() - UI_REF_ALTO * escala) * 0.5f;

    return Rectangle{
        origenX + x * escala,
        origenY + y * escala,
        ancho * escala,
        alto * escala
    };
}


int TamanoTexto(int tokenTexto)
{
    int tamano = (int)(tokenTexto * EscalaUI() + 0.5f);

    return tamano < 10 ? 10 : tamano;
}


//==================================================
// TEXTURAS GENERADAS (madera, placa, cartel, cuerda)
//==================================================

static const Color CONTORNO_MADERA = { 58, 36, 24, 255 };


static void PintarPixel(Image& imagen, int x, int y, Color color)
{
    ImageDrawPixel(&imagen, x, y, color);
}


// Marco de madera oscura con veta horizontal y clavos en las esquinas.
static Image GenerarImagenMadera()
{
    const int lado = 48;
    Image imagen = GenImageColor(lado, lado, UI_MADERA_OSCURA);

    for (int y = 0; y < lado; y++)
    {
        for (int x = 0; x < lado; x++)
        {
            float veta =
                0.5f + 0.5f * std::sin(x * 0.07f + y * 0.9f + 1.8f * std::sin(y * 0.31f));
            float tono = 0.88f + 0.14f * veta + 0.03f * Hash01(y * lado + x);

            Color color = Escalar(UI_MADERA_OSCURA, tono);

            if (x < 2 || y < 2 || x >= lado - 2 || y >= lado - 2)
            {
                color = CONTORNO_MADERA;
            }
            else if (x == 2 || y == 2)
            {
                color = Escalar(UI_MADERA_OSCURA, 1.25f);
            }

            PintarPixel(imagen, x, y, color);
        }
    }

    const int nx[2] = { 6, lado - 7 };

    for (int i = 0; i < 2; i++)
    {
        for (int j = 0; j < 2; j++)
        {
            ImageDrawCircle(&imagen, nx[i], nx[j], 2, CONTORNO_MADERA);
            PintarPixel(imagen, nx[i] - 1, nx[j] - 1, Color{ 200, 170, 130, 255 });
        }
    }

    return imagen;
}


// Placa clara (zona de texto): color plano (sin grano) para no restar
// contraste a la tinta ni formar bloques al estirarse en 9-slice.
static Image GenerarImagenPlaca()
{
    const int lado = 32;
    Image imagen = GenImageColor(lado, lado, UI_PLACA_CLARA);

    for (int y = 0; y < lado; y++)
    {
        for (int x = 0; x < lado; x++)
        {
            float tono = 1.0f;
            Color color = Escalar(UI_PLACA_CLARA, tono);

            if (x == 0 || y == 0 || x == lado - 1 || y == lado - 1)
            {
                color = Color{ 120, 84, 58, 255 };
            }

            PintarPixel(imagen, x, y, color);
        }
    }

    return imagen;
}


// Cartel/boton: borde de madera media con clavos y cara clara.
static Image GenerarImagenCartel()
{
    const int ancho = 48;
    const int alto = 40;
    const int marco = 8;
    Image imagen = GenImageColor(ancho, alto, UI_MADERA_MEDIA);

    for (int y = 0; y < alto; y++)
    {
        for (int x = 0; x < ancho; x++)
        {
            float veta = 0.5f + 0.5f * std::sin(x * 0.2f + y * 1.1f);
            Color color = Escalar(UI_MADERA_MEDIA, 0.90f + 0.14f * veta);

            bool cara =
                x >= marco && x < ancho - marco &&
                y >= marco && y < alto - marco;

            if (cara)
            {
                float tono = 1.0f;
                color = Escalar(UI_PLACA_CLARA, tono);

                if (x == marco || y == marco || x == ancho - marco - 1 || y == alto - marco - 1)
                {
                    color = Color{ 138, 98, 68, 255 };
                }
            }

            if (x < 2 || y < 2 || x >= ancho - 2 || y >= alto - 2)
            {
                color = CONTORNO_MADERA;
            }

            PintarPixel(imagen, x, y, color);
        }
    }

    const int xs[2] = { 5, ancho - 6 };
    const int ys[2] = { 5, alto - 6 };

    for (int i = 0; i < 2; i++)
    {
        for (int j = 0; j < 2; j++)
        {
            ImageDrawCircle(&imagen, xs[i], ys[j], 1, CONTORNO_MADERA);
        }
    }

    return imagen;
}


// Cuerda trenzada vertical: se repite a lo largo.
static Image GenerarImagenCuerda()
{
    const int ancho = 8;
    const int alto = 16;
    Image imagen = GenImageColor(ancho, alto, UI_CUERDA);

    for (int y = 0; y < alto; y++)
    {
        for (int x = 0; x < ancho; x++)
        {
            bool clara = ((x * 2 + y) % 8) < 4;
            Color color = clara ? UI_CUERDA : Escalar(UI_CUERDA, 0.68f);

            if (x == 0 || x == ancho - 1)
            {
                color = Escalar(UI_CUERDA, 0.42f);
            }

            PintarPixel(imagen, x, y, color);
        }
    }

    return imagen;
}


static Texture2D CrearTextura(Image imagen, bool repetir)
{
    Texture2D textura = LoadTextureFromImage(imagen);
    UnloadImage(imagen);

    SetTextureFilter(textura, TEXTURE_FILTER_POINT);
    SetTextureWrap(textura, repetir ? TEXTURE_WRAP_REPEAT : TEXTURE_WRAP_CLAMP);

    return textura;
}


void CargarEstiloUI(EstiloUI& estilo)
{
    if (estilo.cargado)
    {
        return;
    }

    estilo.madera = CrearTextura(GenerarImagenMadera(), false);
    estilo.placa = CrearTextura(GenerarImagenPlaca(), false);
    estilo.cartel = CrearTextura(GenerarImagenCartel(), false);
    estilo.cuerda = CrearTextura(GenerarImagenCuerda(), true);
    estilo.cargado = true;
}


void DescargarEstiloUI(EstiloUI& estilo)
{
    if (!estilo.cargado)
    {
        return;
    }

    UnloadTexture(estilo.madera);
    UnloadTexture(estilo.placa);
    UnloadTexture(estilo.cartel);
    UnloadTexture(estilo.cuerda);

    estilo.madera = {};
    estilo.placa = {};
    estilo.cartel = {};
    estilo.cuerda = {};
    estilo.cargado = false;
}


void ActualizarEstiloUI(EstiloUI& estilo, float deltaTime)
{
    estilo.tiempo += deltaTime;
}


//==================================================
// ENTRADA
//==================================================

EntradaUI LeerEntradaUI(LectorEntradaUI& lector)
{
    EntradaUI entrada;

    entrada.izquierda = IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A);
    entrada.derecha = IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D);
    entrada.arriba = IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W);
    entrada.abajo = IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S);
    entrada.aceptar = IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE);
    entrada.volver = IsKeyPressed(KEY_ESCAPE);
    entrada.pestanaAnterior = IsKeyPressed(KEY_Q);
    entrada.pestanaSiguiente = IsKeyPressed(KEY_E);

    for (int mando = 0; mando < 4; mando++)
    {
        if (!IsGamepadAvailable(mando))
        {
            for (int d = 0; d < 4; d++)
            {
                lector.stickPrevio[mando][d] = false;
            }

            continue;
        }

        float x = GetGamepadAxisMovement(mando, GAMEPAD_AXIS_LEFT_X);
        float y = GetGamepadAxisMovement(mando, GAMEPAD_AXIS_LEFT_Y);

        // Flanco de subida del stick: un empuje = un movimiento.
        bool nivel[4] = { x < -0.55f, x > 0.55f, y < -0.55f, y > 0.55f };

        entrada.izquierda = entrada.izquierda ||
            IsGamepadButtonPressed(mando, GAMEPAD_BUTTON_LEFT_FACE_LEFT) ||
            (nivel[0] && !lector.stickPrevio[mando][0]);
        entrada.derecha = entrada.derecha ||
            IsGamepadButtonPressed(mando, GAMEPAD_BUTTON_LEFT_FACE_RIGHT) ||
            (nivel[1] && !lector.stickPrevio[mando][1]);
        entrada.arriba = entrada.arriba ||
            IsGamepadButtonPressed(mando, GAMEPAD_BUTTON_LEFT_FACE_UP) ||
            (nivel[2] && !lector.stickPrevio[mando][2]);
        entrada.abajo = entrada.abajo ||
            IsGamepadButtonPressed(mando, GAMEPAD_BUTTON_LEFT_FACE_DOWN) ||
            (nivel[3] && !lector.stickPrevio[mando][3]);

        for (int d = 0; d < 4; d++)
        {
            lector.stickPrevio[mando][d] = nivel[d];
        }

        entrada.aceptar = entrada.aceptar ||
            IsGamepadButtonPressed(mando, GAMEPAD_BUTTON_RIGHT_FACE_DOWN);
        entrada.volver = entrada.volver ||
            IsGamepadButtonPressed(mando, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT);
        entrada.pestanaAnterior = entrada.pestanaAnterior ||
            IsGamepadButtonPressed(mando, GAMEPAD_BUTTON_LEFT_TRIGGER_1);
        entrada.pestanaSiguiente = entrada.pestanaSiguiente ||
            IsGamepadButtonPressed(mando, GAMEPAD_BUTTON_RIGHT_TRIGGER_1);
    }

    return entrada;
}


void EnfoqueUI::Mover(int delta)
{
    if (cantidad <= 0)
    {
        indice = 0;
        return;
    }

    indice = ((indice + delta) % cantidad + cantidad) % cantidad;
}


//==================================================
// ANIMACION
//==================================================

void ResorteUI::Actualizar(
    float objetivo,
    float deltaTime,
    float rigidez,
    float amortiguacion
)
{
    // Subpasos: estable aunque un frame dure mucho.
    float restante = Limitar(deltaTime, 0.0f, 0.1f);

    while (restante > 0.0f)
    {
        float paso = restante > (1.0f / 120.0f) ? (1.0f / 120.0f) : restante;

        float aceleracion =
            -rigidez * (valor - objetivo) - amortiguacion * velocidad;

        velocidad += aceleracion * paso;
        valor += velocidad * paso;
        restante -= paso;
    }
}


void ColgadoUI::Iniciar(
    float reposoGrados,
    float retrasoEntrada,
    float distanciaCaidaRef,
    float intensidadBalanceo
)
{
    *this = ColgadoUI{};

    reposo = reposoGrados;
    angulo = reposoGrados;
    retraso = retrasoEntrada;
    distanciaCaida = distanciaCaidaRef;
    intensidad = intensidadBalanceo;
    caida.valor = -distanciaCaidaRef;
    signo = reposoGrados >= 0.0f ? 1.0f : -1.0f;
}


void ColgadoUI::Pulsar()
{
    hundimiento = 1.0f;
}


bool ColgadoUI::EnMovimiento() const
{
    return
        retraso > 0.0f ||
        !aterrizo ||
        std::fabs(angulo - reposo) > 0.03f ||
        std::fabs(velocidad) > 0.3f ||
        std::fabs(caida.valor) > 0.05f;
}


void ColgadoUI::Actualizar(
    const EstiloUI& estilo,
    float deltaTime,
    bool foco
)
{
    hundimiento = Limitar(hundimiento - deltaTime / UI_T_PULSACION, 0.0f, 1.0f);

    // Accesibilidad: sin caida ni balanceo, todo queda en su sitio.
    if (estilo.reducirMovimiento)
    {
        angulo = reposo;
        velocidad = 0.0f;
        caida = ResorteUI{};
        retraso = 0.0f;
        aterrizo = true;
        focoPrevio = foco;
        return;
    }

    if (retraso > 0.0f)
    {
        retraso -= deltaTime;
        focoPrevio = foco;

        if (retraso > 0.0f)
        {
            return;
        }

        retraso = 0.0f;
    }

    // Caida de entrada (muelle subamortiguado: rebota un poco).
    float caidaAnterior = caida.valor;
    caida.Actualizar(0.0f, deltaTime, 36.0f, 8.0f);

    if (!aterrizo && caidaAnterior < 0.0f && caida.valor >= 0.0f)
    {
        aterrizo = true;
        velocidad += 70.0f * signo * intensidad;
    }

    // Impulso al recibir foco: solo entonces se balancea.
    if (foco && !focoPrevio)
    {
        velocidad += 120.0f * signo * intensidad;
        signo = -signo;
    }

    focoPrevio = foco;

    // Pendulo amortiguado (~1,2 Hz): vuelve solo al angulo de reposo.
    float restante = Limitar(deltaTime, 0.0f, 0.1f);
    const float rigidez = 56.8f;
    const float amortiguacion = 5.0f;

    while (restante > 0.0f)
    {
        float paso = restante > (1.0f / 120.0f) ? (1.0f / 120.0f) : restante;

        float aceleracion =
            -rigidez * (angulo - reposo) - amortiguacion * velocidad;

        velocidad += aceleracion * paso;
        angulo += velocidad * paso;
        restante -= paso;
    }

    if (
        aterrizo &&
        std::fabs(angulo - reposo) < 0.03f &&
        std::fabs(velocidad) < 0.3f
    )
    {
        angulo = reposo;
        velocidad = 0.0f;
    }

    if (aterrizo && std::fabs(caida.valor) < 0.05f && std::fabs(caida.velocidad) < 0.5f)
    {
        caida = ResorteUI{};
    }
}


//==================================================
// DIBUJO BASE
//==================================================

void DibujarNueveCortes(
    Texture2D textura,
    int borde,
    Rectangle destino,
    float escala,
    Color tinte
)
{
    float b = (float)borde;
    float bordeDestino = b * escala;

    // Si el destino es menor que dos bordes, los bordes se encogen.
    float maxBorde = (destino.width < destino.height ? destino.width : destino.height) * 0.5f;

    if (bordeDestino > maxBorde)
    {
        bordeDestino = maxBorde;
    }

    float ancho = (float)textura.width;
    float alto = (float)textura.height;

    float xs[4] = { 0.0f, b, ancho - b, ancho };
    float ys[4] = { 0.0f, b, alto - b, alto };

    // Cortes en pixeles enteros y 0,5 px de solape: sin rendijas.
    float dx[4] = {
        std::floor(destino.x),
        std::floor(destino.x + bordeDestino + 0.5f),
        std::floor(destino.x + destino.width - bordeDestino + 0.5f),
        std::floor(destino.x + destino.width + 0.5f)
    };

    float dy[4] = {
        std::floor(destino.y),
        std::floor(destino.y + bordeDestino + 0.5f),
        std::floor(destino.y + destino.height - bordeDestino + 0.5f),
        std::floor(destino.y + destino.height + 0.5f)
    };

    for (int fila = 0; fila < 3; fila++)
    {
        for (int columna = 0; columna < 3; columna++)
        {
            Rectangle origen = {
                xs[columna], ys[fila],
                xs[columna + 1] - xs[columna],
                ys[fila + 1] - ys[fila]
            };

            Rectangle dest = {
                dx[columna], dy[fila],
                dx[columna + 1] - dx[columna] + (columna < 2 ? 0.5f : 0.0f),
                dy[fila + 1] - dy[fila] + (fila < 2 ? 0.5f : 0.0f)
            };

            if (dest.width <= 0.0f || dest.height <= 0.0f)
            {
                continue;
            }

            DrawTexturePro(textura, origen, dest, Vector2{ 0.0f, 0.0f }, 0.0f, tinte);
        }
    }
}


void DibujarCuerdaUI(
    const EstiloUI& estilo,
    Vector2 a,
    Vector2 b
)
{
    float dx = b.x - a.x;
    float dy = b.y - a.y;
    float largo = std::sqrt(dx * dx + dy * dy);

    if (largo < 1.0f)
    {
        return;
    }

    float grosor = Px(5.0f);
    float texel = grosor / 8.0f;
    float rotacion = std::atan2(-dx, dy) * RAD2DEG;

    // La altura de origen mayor que la textura hace que se repita (trenzado).
    Rectangle origen = { 0.0f, 0.0f, 8.0f, largo / texel };
    Rectangle destino = { a.x, a.y, grosor, largo };

    DrawTexturePro(
        estilo.cuerda,
        origen,
        destino,
        Vector2{ grosor * 0.5f, 0.0f },
        rotacion,
        WHITE
    );
}


void DibujarTextoTinta(
    const char* texto,
    float x,
    float y,
    int tokenTexto,
    Color color
)
{
    DrawText(texto, (int)x, (int)y, TamanoTexto(tokenTexto), color);
}


void DibujarTextoMundo(
    const char* texto,
    float centroX,
    float y,
    int tokenTexto,
    Color color
)
{
    int tamano = TamanoTexto(tokenTexto);
    int ancho = MeasureText(texto, tamano);
    int sombra = tamano >= 30 ? 3 : 2;

    DrawText(texto, (int)centroX - ancho / 2 + sombra, (int)y + sombra, tamano, Color{ 0, 0, 0, 120 });
    DrawText(texto, (int)centroX - ancho / 2, (int)y, tamano, color);
}


void DibujarMarcoFoco(Rectangle rect)
{
    float grosor = Px(UI_BORDE_FOCO);
    float halo = Px(4.0f);

    DrawRectangleLinesEx(
        Rectangle{ rect.x - halo, rect.y - halo, rect.width + halo * 2.0f, rect.height + halo * 2.0f },
        Px(2.0f),
        Fade(UI_FOCO, 0.35f)
    );

    DrawRectangleLinesEx(rect, grosor, UI_FOCO);
}


void DibujarFlechaUI(
    float cx,
    float cy,
    float medio,
    int direccion,
    Color color
)
{
    Vector2 punta = { cx + direccion * medio, cy };
    Vector2 a = { cx - direccion * medio, cy - medio * 1.2f };
    Vector2 b = { cx - direccion * medio, cy + medio * 1.2f };

    // DrawTriangle exige sentido antihorario.
    if (direccion > 0)
    {
        DrawTriangle(a, b, punta, color);
    }
    else
    {
        DrawTriangle(b, a, punta, color);
    }
}


//==================================================
// TEXTO AJUSTADO AL ANCHO
//==================================================

// raylib no dibuja la fuente por defecto por debajo de 10 px.
static const int TAMANO_TEXTO_MINIMO = 10;


int TamanoTextoAjustadoUI(
    const char* texto,
    int tokenTexto,
    float anchoMaximo
)
{
    int tamano = TamanoTexto(tokenTexto);

    while (tamano > TAMANO_TEXTO_MINIMO && (float)MeasureText(texto, tamano) > anchoMaximo)
    {
        tamano--;
    }

    return tamano;
}


void DibujarTextoAjustadoUI(
    const char* texto,
    float x,
    float y,
    int tokenTexto,
    float anchoMaximo,
    Color color,
    bool centrado
)
{
    int tamano = TamanoTextoAjustadoUI(texto, tokenTexto, anchoMaximo);

    if (centrado)
    {
        x -= (float)MeasureText(texto, tamano) * 0.5f;
    }

    DrawText(texto, (int)x, (int)y, tamano, color);
}


// Margen horizontal de un cartel a cada lado del texto: 10 de marco de
// madera + 6 de aire (unidades de referencia).
static const float MARGEN_TEXTO_CARTEL = 16.0f;


float AnchoNecesarioBotonUI(
    const char* texto,
    int tokenTexto
)
{
    // La fuente pixel no escala de forma exactamente lineal (el espaciado
    // es entero): se mide a escala 1 y a la escala actual y se toma el
    // mayor, para que el texto quepa en ambas.
    float aEscala1 = (float)MeasureText(texto, tokenTexto);
    float aEscalaActual = (float)MeasureText(texto, TamanoTexto(tokenTexto)) / EscalaUI();
    float anchoTexto = aEscala1 > aEscalaActual ? aEscala1 : aEscalaActual;

    return std::ceil(anchoTexto + MARGEN_TEXTO_CARTEL * 2.0f);
}


//==================================================
// CARTEL COLGADO (base de boton, pestana y titulo)
//==================================================

// Dibuja un cartel que cuelga de dos cuerdas en V desde un ancla situada
// UI_LARGO_CUERDA por encima. Todo (cuerdas, madera, texto) rota junto.
static void DibujarCartelColgado(
    const EstiloUI& estilo,
    const ColgadoUI& colgado,
    Rectangle rect,
    const char* texto,
    int tokenTexto,
    bool foco,
    bool activo,
    bool habilitado
)
{
    if (!estilo.cargado || colgado.retraso > 0.0f)
    {
        return;
    }

    float escala = EscalaUI();
    float largoCuerda = UI_LARGO_CUERDA * escala;
    float desplazamiento = colgado.caida.valor * escala;
    float hundido = colgado.hundimiento * Px(4.0f);
    float bajada = activo ? Px(6.0f) : 0.0f;

    Vector2 ancla = {
        rect.x + rect.width * 0.5f,
        rect.y - largoCuerda + desplazamiento + bajada + hundido
    };

    rlPushMatrix();
    rlTranslatef(ancla.x, ancla.y, 0.0f);
    rlRotatef(colgado.angulo, 0.0f, 0.0f, 1.0f);

    // Coordenadas locales: el ancla es el origen.
    Rectangle local = { -rect.width * 0.5f, largoCuerda, rect.width, rect.height };
    float inset = Px(16.0f);

    DibujarCuerdaUI(estilo, Vector2{ 0.0f, 0.0f }, Vector2{ local.x + inset, local.y + Px(3.0f) });
    DibujarCuerdaUI(estilo, Vector2{ 0.0f, 0.0f }, Vector2{ local.x + local.width - inset, local.y + Px(3.0f) });

    Color tinte = habilitado ? WHITE : Color{ 190, 190, 190, 255 };

    // Sombra proyectada.
    DrawRectangleRec(
        Rectangle{ local.x + Px(4.0f), local.y + Px(5.0f), local.width, local.height },
        Fade(BLACK, 0.28f)
    );

    DibujarNueveCortes(estilo.cartel, 10, local, escala, tinte);

    if (activo)
    {
        // Pestana activa: cara mas calida.
        DrawRectangleRec(
            Rectangle{ local.x + Px(9.0f), local.y + Px(9.0f), local.width - Px(18.0f), local.height - Px(18.0f) },
            Fade(UI_FOCO, 0.18f)
        );
    }

    if (foco)
    {
        DibujarMarcoFoco(local);
    }

    // Si el texto no cabe dentro del marco (10 px de madera por lado), la
    // fuente se reduce; si cabe, queda exactamente igual.
    int tamano = TamanoTextoAjustadoUI(texto, tokenTexto, local.width - Px(20.0f));
    int anchoTexto = MeasureText(texto, tamano);

    DrawText(
        texto,
        (int)(local.x + (local.width - anchoTexto) * 0.5f),
        (int)(local.y + (local.height - tamano) * 0.5f),
        tamano,
        habilitado ? UI_TINTA : UI_TINTA_SUAVE
    );

    rlPopMatrix();
}


//==================================================
// PANEL
//==================================================

void PanelUI::Iniciar(Rectangle rectRef, float retraso)
{
    ref = rectRef;

    // Solo cae y rebota: el contenido no se balancea con el.
    colgado.Iniciar(0.0f, retraso, ref.y + ref.height + 20.0f, 0.0f);
}


void ActualizarPanelUI(PanelUI& panel, const EstiloUI& estilo, float dt)
{
    panel.colgado.Actualizar(estilo, dt, false);
}


Rectangle AreaInteriorPanelUI(const PanelUI& panel)
{
    return Rectangle{
        panel.ref.x + 14.0f,
        panel.ref.y + 14.0f,
        panel.ref.width - 28.0f,
        panel.ref.height - 28.0f
    };
}


void DibujarPanelUI(const EstiloUI& estilo, const PanelUI& panel)
{
    if (!estilo.cargado || panel.colgado.retraso > 0.0f)
    {
        return;
    }

    float escala = EscalaUI();
    Rectangle rect = RectRef(panel.ref.x, panel.ref.y, panel.ref.width, panel.ref.height);
    rect.y += panel.colgado.caida.valor * escala;

    // Dos cuerdas verticales desde el borde superior de la pantalla.
    float inset = Px(46.0f);

    for (int lado = 0; lado < 2; lado++)
    {
        float x = lado == 0 ? rect.x + inset : rect.x + rect.width - inset;

        if (rect.y > 0.0f)
        {
            DibujarCuerdaUI(estilo, Vector2{ x, 0.0f }, Vector2{ x, rect.y + Px(4.0f) });
        }
    }

    DrawRectangleRec(
        Rectangle{ rect.x + Px(6.0f), rect.y + Px(8.0f), rect.width, rect.height },
        Fade(BLACK, 0.30f)
    );

    DibujarNueveCortes(estilo.madera, 12, rect, escala, WHITE);

    float margen = Px(14.0f);
    Rectangle interior = {
        rect.x + margen, rect.y + margen,
        rect.width - margen * 2.0f, rect.height - margen * 2.0f
    };

    DibujarNueveCortes(estilo.placa, 8, interior, escala, WHITE);
}


//==================================================
// TITULO / BOTON
//==================================================

void DibujarTituloUI(
    const EstiloUI& estilo,
    const ColgadoUI& colgado,
    const char* texto,
    Rectangle ref
)
{
    Rectangle rect = RectRef(ref.x, ref.y, ref.width, ref.height);

    DibujarCartelColgado(estilo, colgado, rect, texto, UI_TEXTO_GRANDE, false, false, true);
}


void BotonUI::Iniciar(
    Rectangle rectRef,
    const char* textoBoton,
    float reposoGrados,
    float retraso
)
{
    ref = rectRef;
    texto = textoBoton;
    habilitado = true;
    activo = false;
    colgado.Iniciar(reposoGrados, retraso, 60.0f, 1.0f);
}


void ActualizarBotonUI(
    BotonUI& boton,
    const EstiloUI& estilo,
    float dt,
    bool foco
)
{
    boton.colgado.Actualizar(estilo, dt, foco && boton.habilitado);
}


void DibujarBotonUI(
    const EstiloUI& estilo,
    const BotonUI& boton,
    bool foco
)
{
    Rectangle rect = RectRef(boton.ref.x, boton.ref.y, boton.ref.width, boton.ref.height);

    DibujarCartelColgado(
        estilo,
        boton.colgado,
        rect,
        boton.texto,
        UI_TEXTO,
        foco && boton.habilitado,
        boton.activo,
        boton.habilitado
    );
}


//==================================================
// FILAS: BASE COMUN
//==================================================

// Placa clara de una fila con su sombra y marco de foco. Devuelve el
// rectangulo en pixeles ya desplazado por la animacion de entrada.
static bool DibujarBaseFila(
    const EstiloUI& estilo,
    const ColgadoUI& colgado,
    Rectangle ref,
    bool foco,
    Rectangle& rect
)
{
    if (!estilo.cargado || colgado.retraso > 0.0f)
    {
        return false;
    }

    rect = RectRef(ref.x, ref.y, ref.width, ref.height);
    rect.y += colgado.caida.valor * EscalaUI();

    DrawRectangleRec(
        Rectangle{ rect.x + Px(3.0f), rect.y + Px(4.0f), rect.width, rect.height },
        Fade(BLACK, 0.22f)
    );

    DibujarNueveCortes(estilo.placa, 8, rect, EscalaUI(), WHITE);

    if (foco)
    {
        DibujarMarcoFoco(rect);
    }

    return true;
}


static void DibujarEtiquetaFila(const char* etiqueta, Rectangle rect)
{
    int tamano = TamanoTexto(UI_TEXTO);

    DrawText(
        etiqueta,
        (int)(rect.x + Px(UI_RELLENO + 4.0f)),
        (int)(rect.y + (rect.height - tamano) * 0.5f),
        tamano,
        UI_TINTA
    );
}


//==================================================
// SELECTOR
//==================================================

void SelectorUI::Iniciar(Rectangle rectRef, const char* textoEtiqueta, float retraso)
{
    ref = rectRef;
    etiqueta = textoEtiqueta;
    golpeIzquierda = 0.0f;
    golpeDerecha = 0.0f;
    habilitado = true;
    colgado.Iniciar(0.0f, retraso, 40.0f, 0.0f);
}


void ActualizarSelectorUI(SelectorUI& selector, const EstiloUI& estilo, float dt)
{
    selector.colgado.Actualizar(estilo, dt, false);

    selector.golpeIzquierda = Limitar(selector.golpeIzquierda - dt / UI_T_PULSACION, 0.0f, 1.0f);
    selector.golpeDerecha = Limitar(selector.golpeDerecha - dt / UI_T_PULSACION, 0.0f, 1.0f);
}


void GolpearSelectorUI(SelectorUI& selector, int direccion)
{
    if (!selector.habilitado)
    {
        return;
    }

    if (direccion < 0)
    {
        selector.golpeIzquierda = 1.0f;
    }
    else
    {
        selector.golpeDerecha = 1.0f;
    }
}


// Flecha solo contorneada: indica que el selector no se puede cambiar.
static void DibujarFlechaInactiva(float cx, float cy, float medio, int direccion, Color color)
{
    Vector2 punta = { cx + direccion * medio, cy };
    Vector2 a = { cx - direccion * medio, cy - medio * 1.2f };
    Vector2 b = { cx - direccion * medio, cy + medio * 1.2f };
    float grosor = Px(1.5f);

    DrawLineEx(a, b, grosor, color);
    DrawLineEx(b, punta, grosor, color);
    DrawLineEx(punta, a, grosor, color);
}


void DibujarSelectorUI(
    const EstiloUI& estilo,
    const SelectorUI& selector,
    const char* valorTexto,
    bool foco
)
{
    Rectangle rect;

    if (!DibujarBaseFila(estilo, selector.colgado, selector.ref, foco, rect))
    {
        return;
    }

    float centroY = rect.y + rect.height * 0.5f;
    float medio = Px(7.0f);
    float xIzq = rect.x + rect.width * 0.55f;
    float xDer = rect.x + rect.width - Px(28.0f);
    float centroValor = (xIzq + xDer) * 0.5f;
    int tamano = TamanoTexto(UI_TEXTO);
    int ancho = MeasureText(valorTexto, tamano);

    if (!selector.habilitado)
    {
        // Atenuado: rayado diagonal muy suave (sin texto encima), flechas
        // solo contorneadas y texto en tinta suave (~4,7:1 sobre la placa).
        // Cada raya va de (x, abajo) a (x + alto, arriba) a 45 grados; se
        // recorta a mano a los bordes izquierdo y derecho (sin scissor,
        // para no pisar uno que tenga activo la pantalla).
        float pasoRayas = Px(14.0f);
        float inset = Px(2.0f);
        float izquierda = rect.x + inset;
        float derecha = rect.x + rect.width - inset;
        float abajo = rect.y + rect.height - inset;
        float alto = rect.height - inset * 2.0f;

        for (float x = izquierda - alto; x < derecha; x += pasoRayas)
        {
            float t0 = Limitar((izquierda - x) / alto, 0.0f, 1.0f);
            float t1 = Limitar((derecha - x) / alto, 0.0f, 1.0f);

            if (t1 <= t0)
            {
                continue;
            }

            DrawLineEx(
                Vector2{ x + alto * t0, abajo - alto * t0 },
                Vector2{ x + alto * t1, abajo - alto * t1 },
                Px(2.0f),
                Fade(UI_MADERA_OSCURA, 0.08f)
            );
        }

        // El rayado no pasa por debajo de etiqueta y valor: se repinta la
        // placa lisa detras de cada texto.
        int tamanoEtiqueta = TamanoTexto(UI_TEXTO);
        float xEtiqueta = rect.x + Px(UI_RELLENO + 4.0f);

        DrawRectangleRec(
            Rectangle{
                xEtiqueta - Px(4.0f),
                centroY - tamanoEtiqueta * 0.5f - Px(3.0f),
                (float)MeasureText(selector.etiqueta, tamanoEtiqueta) + Px(8.0f),
                tamanoEtiqueta + Px(6.0f)
            },
            UI_PLACA_CLARA
        );

        DrawRectangleRec(
            Rectangle{
                centroValor - ancho * 0.5f - Px(4.0f),
                centroY - tamano * 0.5f - Px(3.0f),
                ancho + Px(8.0f),
                tamano + Px(6.0f)
            },
            UI_PLACA_CLARA
        );

        DrawText(
            selector.etiqueta,
            (int)xEtiqueta,
            (int)(rect.y + (rect.height - tamanoEtiqueta) * 0.5f),
            tamanoEtiqueta,
            UI_TINTA_SUAVE
        );

        DibujarFlechaInactiva(xIzq, centroY, medio, -1, Fade(UI_TINTA_SUAVE, 0.55f));
        DibujarFlechaInactiva(xDer, centroY, medio, 1, Fade(UI_TINTA_SUAVE, 0.55f));

        DrawText(
            valorTexto,
            (int)(centroValor - ancho * 0.5f),
            (int)(centroY - tamano * 0.5f),
            tamano,
            UI_TINTA_SUAVE
        );

        return;
    }

    DibujarEtiquetaFila(selector.etiqueta, rect);

    float golpeI = selector.golpeIzquierda * Px(6.0f);
    float golpeD = selector.golpeDerecha * Px(6.0f);

    DibujarFlechaUI(xIzq - golpeI, centroY, medio, -1, UI_MADERA_OSCURA);
    DibujarFlechaUI(xDer + golpeD, centroY, medio, 1, UI_MADERA_OSCURA);

    DrawText(
        valorTexto,
        (int)(centroValor - ancho * 0.5f),
        (int)(centroY - tamano * 0.5f),
        tamano,
        UI_TINTA
    );
}


//==================================================
// PALANCA (slider)
//==================================================

void PalancaUI::Iniciar(
    Rectangle rectRef,
    const char* textoEtiqueta,
    float valorInicial,
    float retraso
)
{
    ref = rectRef;
    etiqueta = textoEtiqueta;
    valor = Limitar(valorInicial, 0.0f, 1.0f);
    valorMostrado = valor;
    paso = 0.05f;
    colgado.Iniciar(0.0f, retraso, 40.0f, 0.0f);
}


void ActualizarPalancaUI(PalancaUI& palanca, const EstiloUI& estilo, float dt)
{
    palanca.colgado.Actualizar(estilo, dt, false);

    // La perilla llega suave (o directo con movimiento reducido).
    if (estilo.reducirMovimiento)
    {
        palanca.valorMostrado = palanca.valor;
        return;
    }

    float diferencia = palanca.valor - palanca.valorMostrado;
    float avance = diferencia * Limitar(dt / UI_T_ASENTAR_PALANCA, 0.0f, 1.0f);

    palanca.valorMostrado += avance;

    if (std::fabs(palanca.valor - palanca.valorMostrado) < 0.001f)
    {
        palanca.valorMostrado = palanca.valor;
    }
}


void AjustarPalancaUI(PalancaUI& palanca, int direccion)
{
    palanca.valor = Limitar(palanca.valor + direccion * palanca.paso, 0.0f, 1.0f);
}


float ValorRealPalancaUI(const PalancaUI& palanca)
{
    return palanca.minimo + palanca.valor * (palanca.maximo - palanca.minimo);
}


void RielPalancaUI(
    const PalancaUI& palanca,
    float& x0,
    float& x1
)
{
    Rectangle rect = RectRef(palanca.ref.x, palanca.ref.y, palanca.ref.width, palanca.ref.height);

    x0 = rect.x + rect.width * 0.42f;
    x1 = rect.x + rect.width - Px(palanca.anchoValor);
}


void DibujarPalancaUI(
    const EstiloUI& estilo,
    const PalancaUI& palanca,
    bool foco,
    const char* textoValor
)
{
    Rectangle rect;

    if (!DibujarBaseFila(estilo, palanca.colgado, palanca.ref, foco, rect))
    {
        return;
    }

    DibujarEtiquetaFila(palanca.etiqueta, rect);

    float x0 = rect.x + rect.width * 0.42f;
    float x1 = rect.x + rect.width - Px(palanca.anchoValor);
    float yRiel = rect.y + rect.height * 0.62f;
    float grosor = Px(8.0f);

    // Riel: surco oscuro con relleno de madera hasta la perilla.
    DrawRectangleRec(Rectangle{ x0, yRiel - grosor * 0.5f, x1 - x0, grosor }, UI_TINTA);

    float xPerilla = x0 + (x1 - x0) * palanca.valorMostrado;

    DrawRectangleRec(
        Rectangle{ x0 + Px(1.0f), yRiel - grosor * 0.5f + Px(1.0f), xPerilla - x0, grosor - Px(2.0f) },
        UI_MADERA_MEDIA
    );

    // Marcas.
    for (int i = 0; i <= 10; i++)
    {
        float x = x0 + (x1 - x0) * i / 10.0f;
        DrawRectangleRec(Rectangle{ x - Px(1.0f), yRiel + Px(7.0f), Px(2.0f), Px(5.0f) }, UI_TINTA_SUAVE);
    }

    // Palanca: vastago + bola roja con brillo.
    DrawLineEx(
        Vector2{ xPerilla, yRiel },
        Vector2{ xPerilla, yRiel - Px(15.0f) },
        Px(5.0f),
        UI_MADERA_OSCURA
    );

    DrawCircleV(Vector2{ xPerilla, yRiel - Px(18.0f) }, Px(9.0f), UI_PELIGRO);
    DrawCircleLinesV(Vector2{ xPerilla, yRiel - Px(18.0f) }, Px(9.0f), CONTORNO_MADERA);
    DrawCircleV(Vector2{ xPerilla - Px(3.0f), yRiel - Px(21.0f) }, Px(2.5f), Fade(WHITE, 0.8f));

    char texto[48];
    int tamano = TamanoTexto(UI_TEXTO);

    if (textoValor != nullptr || palanca.formatoValor != nullptr)
    {
        if (textoValor != nullptr)
        {
            snprintf(texto, sizeof texto, "%s", textoValor);
        }
        else
        {
            snprintf(texto, sizeof texto, palanca.formatoValor, ValorRealPalancaUI(palanca));
        }

        // Cabe entre la bola de la perilla (en x1) y el borde derecho.
        float anchoMaximo = Px(palanca.anchoValor - UI_RELLENO - 4.0f - 12.0f);
        tamano = TamanoTextoAjustadoUI(texto, UI_TEXTO, anchoMaximo);
    }
    else
    {
        // Por defecto: 0..100 % (igual que siempre).
        snprintf(texto, sizeof texto, "%d%%", (int)(palanca.valor * 100.0f + 0.5f));
    }

    int ancho = MeasureText(texto, tamano);

    DrawText(
        texto,
        (int)(rect.x + rect.width - Px(UI_RELLENO + 4.0f) - ancho),
        (int)(rect.y + (rect.height - tamano) * 0.5f),
        tamano,
        UI_TINTA
    );
}


//==================================================
// INTERRUPTOR (toggle)
//==================================================

void InterruptorUI::Iniciar(
    Rectangle rectRef,
    const char* textoEtiqueta,
    bool valorInicial,
    float retraso
)
{
    ref = rectRef;
    etiqueta = textoEtiqueta;
    activo = valorInicial;
    posicion = valorInicial ? 1.0f : 0.0f;
    colgado.Iniciar(0.0f, retraso, 40.0f, 0.0f);
}


void ActualizarInterruptorUI(
    InterruptorUI& interruptor,
    const EstiloUI& estilo,
    float dt
)
{
    interruptor.colgado.Actualizar(estilo, dt, false);

    float objetivo = interruptor.activo ? 1.0f : 0.0f;

    if (estilo.reducirMovimiento)
    {
        interruptor.posicion = objetivo;
        return;
    }

    float diferencia = objetivo - interruptor.posicion;
    float avance = Limitar(dt / UI_T_ASENTAR_PALANCA, 0.0f, 1.0f);

    interruptor.posicion += diferencia * avance;

    if (std::fabs(diferencia) < 0.01f)
    {
        interruptor.posicion = objetivo;
    }
}


void DibujarInterruptorUI(
    const EstiloUI& estilo,
    const InterruptorUI& interruptor,
    bool foco
)
{
    Rectangle rect;

    if (!DibujarBaseFila(estilo, interruptor.colgado, interruptor.ref, foco, rect))
    {
        return;
    }

    DibujarEtiquetaFila(interruptor.etiqueta, rect);

    float anchoRiel = Px(62.0f);
    float altoRiel = Px(26.0f);
    float x = rect.x + rect.width * 0.55f;
    float y = rect.y + (rect.height - altoRiel) * 0.5f;

    Color riel = interruptor.activo ? UI_POSITIVO : UI_MADERA_OSCURA;

    DrawRectangleRounded(Rectangle{ x, y, anchoRiel, altoRiel }, 0.5f, 8, CONTORNO_MADERA);
    DrawRectangleRounded(
        Rectangle{ x + Px(2.0f), y + Px(2.0f), anchoRiel - Px(4.0f), altoRiel - Px(4.0f) },
        0.5f,
        8,
        riel
    );

    float recorrido = anchoRiel - altoRiel;
    float cx = x + altoRiel * 0.5f + recorrido * interruptor.posicion;

    DrawCircleV(Vector2{ cx, y + altoRiel * 0.5f }, altoRiel * 0.5f - Px(3.0f), UI_PLACA_CLARA);
    DrawCircleLinesV(Vector2{ cx, y + altoRiel * 0.5f }, altoRiel * 0.5f - Px(3.0f), CONTORNO_MADERA);

    const char* texto = interruptor.activo ? "SI" : "NO";
    int tamano = TamanoTexto(UI_TEXTO);

    DrawText(
        texto,
        (int)(x + anchoRiel + Px(14.0f)),
        (int)(rect.y + (rect.height - tamano) * 0.5f),
        tamano,
        UI_TINTA
    );
}


//==================================================
// TARJETA
//==================================================

// Medidas de la tarjeta en unidades de referencia.
static const float MARCO_TARJETA = 7.0f;            // madera visible
static const float RELLENO_TARJETA = 12.0f;
static const float ALTO_TITULO_TARJETA = 34.0f;     // texto + aire + divisor
static const float PASO_LINEA_TARJETA = 24.0f;


void TarjetaUI::Iniciar(Rectangle rectRef, const char* textoTitulo, float retraso)
{
    ref = rectRef;
    titulo = textoTitulo;
    colgado.Iniciar(0.0f, retraso, 40.0f, 0.0f);
}


void ActualizarTarjetaUI(TarjetaUI& tarjeta, const EstiloUI& estilo, float dt)
{
    tarjeta.colgado.Actualizar(estilo, dt, false);
}


Rectangle AreaContenidoTarjetaUI(const TarjetaUI& tarjeta)
{
    float margen = MARCO_TARJETA + RELLENO_TARJETA;
    float arriba = margen - 4.0f;

    if (tarjeta.titulo != nullptr)
    {
        arriba += ALTO_TITULO_TARJETA;
    }

    return Rectangle{
        tarjeta.ref.x + margen,
        tarjeta.ref.y + arriba,
        tarjeta.ref.width - margen * 2.0f,
        tarjeta.ref.height - arriba - margen + 4.0f
    };
}


void DibujarTarjetaUI(const EstiloUI& estilo, const TarjetaUI& tarjeta, float alfa)
{
    if (!estilo.cargado || tarjeta.colgado.retraso > 0.0f || alfa <= 0.0f)
    {
        return;
    }

    float escala = EscalaUI();
    Rectangle rect = RectRef(tarjeta.ref.x, tarjeta.ref.y, tarjeta.ref.width, tarjeta.ref.height);
    rect.y += tarjeta.colgado.caida.valor * escala;

    DrawRectangleRec(
        Rectangle{ rect.x + Px(4.0f), rect.y + Px(5.0f), rect.width, rect.height },
        MultiplicarAlfa(Fade(BLACK, 0.22f), alfa)
    );

    // Marco de madera fino (la misma textura del panel, a media escala)
    // y la placa clara encima: el texto solo va sobre la placa.
    DibujarNueveCortes(estilo.madera, 12, rect, escala * 0.6f, MultiplicarAlfa(WHITE, alfa));

    float marco = Px(MARCO_TARJETA);
    Rectangle placa = {
        rect.x + marco, rect.y + marco,
        rect.width - marco * 2.0f, rect.height - marco * 2.0f
    };

    DibujarNueveCortes(estilo.placa, 8, placa, escala, MultiplicarAlfa(WHITE, alfa));

    if (tarjeta.titulo == nullptr)
    {
        return;
    }

    float relleno = Px(RELLENO_TARJETA);
    float anchoUtil = placa.width - relleno * 2.0f;

    DibujarTextoAjustadoUI(
        tarjeta.titulo,
        placa.x + relleno,
        placa.y + Px(8.0f),
        UI_TEXTO,
        anchoUtil,
        MultiplicarAlfa(UI_TINTA, alfa),
        false
    );

    DrawRectangleRec(
        Rectangle{ placa.x + relleno, placa.y + Px(32.0f), anchoUtil, Px(2.0f) },
        MultiplicarAlfa(Fade(UI_MADERA_OSCURA, 0.45f), alfa)
    );
}


void DibujarLineaTarjetaUI(
    const TarjetaUI& tarjeta,
    int linea,
    const char* etiqueta,
    const char* valor,
    float alfa
)
{
    if (tarjeta.colgado.retraso > 0.0f || alfa <= 0.0f)
    {
        return;
    }

    Rectangle area = AreaContenidoTarjetaUI(tarjeta);
    Rectangle rect = RectRef(area.x, area.y + linea * PASO_LINEA_TARJETA, area.width, PASO_LINEA_TARJETA);
    rect.y += tarjeta.colgado.caida.valor * EscalaUI();

    float y = rect.y + (rect.height - TamanoTexto(UI_TEXTO_PEQUENO)) * 0.5f;

    DibujarTextoAjustadoUI(etiqueta, rect.x, y, UI_TEXTO_PEQUENO, rect.width * 0.42f, MultiplicarAlfa(UI_TINTA_SUAVE, alfa), false);
    DibujarTextoAjustadoUI(valor, rect.x + rect.width * 0.45f, y, UI_TEXTO_PEQUENO, rect.width * 0.55f, MultiplicarAlfa(UI_TINTA, alfa), false);
}


//==================================================
// MODAL / AVISO
//==================================================

// Velo oscuro sobre la escena (el mismo de Configuracion).
static const Color COLOR_VELO_MODAL = { 10, 12, 18, 255 };

static const float T_APARICION_VELO = 0.18f;
static const float MARGEN_MODAL = 14.0f;            // madera del cartel
static const float HUECO_BOTONES_MODAL = 30.0f;


void ModalUI::Abrir(
    Rectangle rectRef,
    const char* textoMensaje,
    const char* textoBoton0,
    const char* textoBoton1,
    int focoInicial
)
{
    ref = rectRef;
    mensaje = textoMensaje;
    foco = focoInicial == 1 ? 1 : 0;
    abierto = true;
    aparicion = 0.0f;

    // Cae poco (desde apenas encima de los ganchos) y no se balancea.
    colgado.Iniciar(0.0f, 0.0f, largoSogas + 18.0f, 0.0f);

    // Botones al pie, con el ancho que pidan sus textos (+40 de aire,
    // minimo 180) sin pasarse del cartel; si aun asi no cabe, el texto se
    // reduce solo.
    const char* textos[2] = { textoBoton0, textoBoton1 };
    float ancho = 180.0f;

    for (int i = 0; i < 2; i++)
    {
        float necesario = AnchoNecesarioBotonUI(textos[i]) + 40.0f;
        ancho = necesario > ancho ? necesario : ancho;
    }

    float anchoMaximo = (ref.width - MARGEN_MODAL * 2.0f - HUECO_BOTONES_MODAL * 3.0f) * 0.5f;
    ancho = ancho < anchoMaximo ? ancho : anchoMaximo;

    float x0 = ref.x + (ref.width - ancho * 2.0f - HUECO_BOTONES_MODAL) * 0.5f;
    float y = ref.y + ref.height - MARGEN_MODAL - 18.0f - UI_ALTO_BOTON;
    const float reposos[2] = { -1.0f, 1.0f };

    for (int i = 0; i < 2; i++)
    {
        botones[i].Iniciar(
            Rectangle{ x0 + i * (ancho + HUECO_BOTONES_MODAL), y, ancho, UI_ALTO_BOTON },
            textos[i],
            reposos[i],
            0.12f + i * UI_T_ESCALONADO
        );

        // Caida corta: acompanan al cartel en lugar de bajar desde arriba.
        botones[i].colgado.Iniciar(reposos[i], 0.12f + i * UI_T_ESCALONADO, 24.0f, 1.0f);
    }
}


void ModalUI::Cerrar()
{
    abierto = false;
    aparicion = 0.0f;
}


int ActualizarModalUI(
    ModalUI& modal,
    const EstiloUI& estilo,
    float dt,
    const EntradaUI& entrada
)
{
    if (!modal.abierto)
    {
        return -1;
    }

    modal.aparicion = Limitar(modal.aparicion + dt / T_APARICION_VELO, 0.0f, 1.0f);

    // Los botones estan uno al lado del otro: izquierda/derecha van al de
    // ese lado; arriba/abajo alternan (para mandos que solo usan el stick
    // vertical en los menus).
    if (entrada.izquierda)
    {
        modal.foco = 0;
    }
    else if (entrada.derecha)
    {
        modal.foco = 1;
    }
    else if (entrada.arriba || entrada.abajo)
    {
        modal.foco = 1 - modal.foco;
    }

    modal.colgado.Actualizar(estilo, dt, false);

    for (int i = 0; i < 2; i++)
    {
        ActualizarBotonUI(modal.botones[i], estilo, dt, modal.foco == i);
    }

    int respuesta = -1;

    if (entrada.aceptar)
    {
        respuesta = modal.foco;
    }
    else if (entrada.volver)
    {
        respuesta = modal.botonCancelar == 0 ? 0 : 1;
    }

    if (respuesta >= 0)
    {
        modal.botones[respuesta].colgado.Pulsar();
        modal.Cerrar();
    }

    return respuesta;
}


// Parte `texto` en lineas que quepan en anchoMaximo y las dibuja
// centradas. Devuelve la y siguiente a la ultima linea.
static float DibujarParrafoCentrado(
    const char* texto,
    float centroX,
    float y,
    int tamano,
    float anchoMaximo,
    Color color
)
{
    char linea[256];
    char prueba[256];
    int largoLinea = 0;
    const char* p = texto;

    linea[0] = '\0';

    while (*p != '\0')
    {
        // Siguiente palabra (hasta el espacio).
        const char* fin = p;

        while (*fin != '\0' && *fin != ' ')
        {
            fin++;
        }

        int largoPalabra = (int)(fin - p);

        snprintf(
            prueba,
            sizeof prueba,
            "%s%s%.*s",
            linea,
            largoLinea > 0 ? " " : "",
            largoPalabra,
            p
        );

        if (largoLinea > 0 && (float)MeasureText(prueba, tamano) > anchoMaximo)
        {
            DrawText(linea, (int)(centroX - MeasureText(linea, tamano) * 0.5f), (int)y, tamano, color);
            y += tamano + Px(6.0f);

            snprintf(linea, sizeof linea, "%.*s", largoPalabra, p);
        }
        else
        {
            snprintf(linea, sizeof linea, "%s", prueba);
        }

        largoLinea = (int)TextLength(linea);
        p = *fin == ' ' ? fin + 1 : fin;
    }

    if (largoLinea > 0)
    {
        DrawText(linea, (int)(centroX - MeasureText(linea, tamano) * 0.5f), (int)y, tamano, color);
        y += tamano + Px(6.0f);
    }

    return y;
}


void DibujarModalUI(
    const EstiloUI& estilo,
    const ModalUI& modal,
    const char* detalle,
    float progreso
)
{
    if (!modal.abierto || !estilo.cargado)
    {
        return;
    }

    DrawRectangle(
        0,
        0,
        GetScreenWidth(),
        GetScreenHeight(),
        Fade(COLOR_VELO_MODAL, 0.55f * modal.aparicion)
    );

    float escala = EscalaUI();
    Rectangle fijo = RectRef(modal.ref.x, modal.ref.y, modal.ref.width, modal.ref.height);
    Rectangle rect = fijo;
    rect.y += modal.colgado.caida.valor * escala;

    // Ganchos clavados por encima del cartel (no se mueven) y sogas
    // cortas hasta el borde superior.
    if (modal.largoSogas > 0.0f)
    {
        float yGancho = fijo.y - Px(modal.largoSogas);
        float inset = Px(modal.ref.width * 0.2f < 46.0f ? modal.ref.width * 0.2f : 46.0f);

        for (int lado = 0; lado < 2; lado++)
        {
            float x = lado == 0 ? rect.x + inset : rect.x + rect.width - inset;

            if (rect.y > yGancho)
            {
                DibujarCuerdaUI(estilo, Vector2{ x, yGancho }, Vector2{ x, rect.y + Px(4.0f) });
            }

            DrawCircleV(Vector2{ x, yGancho }, Px(4.0f), UI_TINTA);
            DrawCircleV(Vector2{ x, yGancho }, Px(2.5f), UI_MADERA_OSCURA);
        }
    }

    DrawRectangleRec(
        Rectangle{ rect.x + Px(6.0f), rect.y + Px(8.0f), rect.width, rect.height },
        Fade(BLACK, 0.30f)
    );

    DibujarNueveCortes(estilo.madera, 12, rect, escala, WHITE);

    float margen = Px(MARGEN_MODAL);
    Rectangle interior = {
        rect.x + margen, rect.y + margen,
        rect.width - margen * 2.0f, rect.height - margen * 2.0f
    };

    DibujarNueveCortes(estilo.placa, 8, interior, escala, WHITE);

    // Mensaje (grande) y detalle (partido en lineas).
    float centroX = interior.x + interior.width * 0.5f;
    float anchoTexto = interior.width - Px(40.0f);
    float y = interior.y + Px(20.0f);

    DibujarTextoAjustadoUI(modal.mensaje, centroX, y, UI_TEXTO_GRANDE, anchoTexto, UI_TINTA, true);
    y += TamanoTexto(UI_TEXTO_GRANDE) + Px(14.0f);

    if (detalle != nullptr && detalle[0] != '\0')
    {
        y = DibujarParrafoCentrado(detalle, centroX, y, TamanoTexto(UI_TEXTO), anchoTexto, UI_TINTA_SUAVE);
    }

    // Riel de cuenta atras (mismo aspecto que el de las palancas).
    if (progreso >= 0.0f)
    {
        Rectangle riel = {
            interior.x + Px(40.0f),
            y + Px(4.0f),
            interior.width - Px(80.0f),
            Px(10.0f)
        };

        DrawRectangleRec(riel, UI_TINTA);
        DrawRectangleRec(
            Rectangle{
                riel.x + Px(1.0f),
                riel.y + Px(1.0f),
                (riel.width - Px(2.0f)) * Limitar(progreso, 0.0f, 1.0f),
                riel.height - Px(2.0f)
            },
            UI_MADERA_MEDIA
        );
    }

    // Los botones acompanan la caida del cartel.
    for (int i = 0; i < 2; i++)
    {
        BotonUI boton = modal.botones[i];
        boton.ref.y += modal.colgado.caida.valor;

        DibujarBotonUI(estilo, boton, modal.foco == i);
    }
}


int BotonModalBajoMouseUI(const ModalUI& modal)
{
    if (!modal.abierto)
    {
        return -1;
    }

    Vector2 mouse = GetMousePosition();

    for (int i = 0; i < 2; i++)
    {
        const Rectangle& r = modal.botones[i].ref;

        if (CheckCollisionPointRec(mouse, RectRef(r.x, r.y, r.width, r.height)))
        {
            return i;
        }
    }

    return -1;
}


//==================================================
// AYUDA
//==================================================

void DibujarAyudaUI(const EstiloUI& estilo, const char* texto)
{
    if (!estilo.cargado)
    {
        return;
    }

    Rectangle rect = RectRef(140.0f, 674.0f, 1000.0f, 32.0f);
    int tamano = TamanoTexto(UI_TEXTO_PEQUENO);
    int ancho = MeasureText(texto, tamano);

    DibujarNueveCortes(estilo.placa, 8, rect, EscalaUI(), WHITE);

    DrawText(
        texto,
        (int)(rect.x + (rect.width - ancho) * 0.5f),
        (int)(rect.y + (rect.height - tamano) * 0.5f),
        tamano,
        UI_TINTA
    );
}


//==================================================
// SEGUNDA TANDA: UTILIDADES
//==================================================

// Triangulo relleno en cualquier orden de vertices: raylib descarta los
// que no van en sentido antihorario, asi que se corrige el orden aqui.
static void DibujarTrianguloUI(Vector2 a, Vector2 b, Vector2 c, Color color)
{
    // Area con signo en pantalla (y hacia abajo): > 0 = horario a la vista.
    float area = (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);

    if (area > 0.0f)
    {
        DrawTriangle(a, c, b, color);
    }
    else
    {
        DrawTriangle(a, b, c, color);
    }
}


//==================================================
// TEXTO SOBRE EL MUNDO CON CONTORNO
//==================================================

void DibujarTextoMundoContornoUI(
    const char* texto,
    float centroX,
    float y,
    int tokenTexto,
    Color color,
    float alfa,
    Color contorno
)
{
    alfa = Limitar(alfa, 0.0f, 1.0f);

    if (alfa <= 0.0f)
    {
        return;
    }

    int tamano = TamanoTexto(tokenTexto);
    int ancho = MeasureText(texto, tamano);

    // Proporcionales al tamano: ~1/16 de contorno y ~1/10 de sombra
    // (16 px -> 1 y 2; 30 px -> 2 y 3; 54 px -> 3 y 5).
    int grosor = (int)(tamano / 16.0f + 0.5f);
    int sombra = (int)(tamano / 10.0f + 0.5f);
    grosor = grosor < 1 ? 1 : grosor;
    sombra = sombra < 2 ? 2 : sombra;

    int x = (int)centroX - ancho / 2;
    int yTexto = (int)y;

    // 8 copias desplazadas forman el contorno (el trazo de la fuente
    // pixel siempre es mas ancho que el desplazamiento: no quedan huecos).
    const int dx[8] = { -1, 0, 1, -1, 1, -1, 0, 1 };
    const int dy[8] = { -1, -1, -1, 0, 0, 1, 1, 1 };

    // Sombra dura: la silueta contorneada, desplazada abajo a la derecha.
    Color colorSombra = Fade(BLACK, 0.45f * alfa);

    for (int i = 0; i < 8; i++)
    {
        DrawText(texto, x + sombra + dx[i] * grosor, yTexto + sombra + dy[i] * grosor, tamano, colorSombra);
    }

    Color colorContorno = Fade(contorno, alfa);

    for (int i = 0; i < 8; i++)
    {
        DrawText(texto, x + dx[i] * grosor, yTexto + dy[i] * grosor, tamano, colorContorno);
    }

    DrawText(texto, x, yTexto, tamano, Fade(color, alfa));
}


//==================================================
// ETIQUETA COMPACTA
//==================================================

Rectangle DibujarEtiquetaUI(
    const char* texto,
    Vector2 centro,
    float rotacionGrados,
    const OpcionesEtiquetaUI& opciones
)
{
    float alfa = Limitar(opciones.alfa, 0.0f, 1.0f);
    int tamano = TamanoTexto(opciones.tokenTexto);
    int anchoTexto = MeasureText(texto, tamano);

    float borde = Px(2.5f);
    float ancho = anchoTexto + Px(8.0f) * 2.0f + borde * 2.0f;
    float alto = tamano + Px(4.0f) * 2.0f + borde * 2.0f;

    Rectangle resultado = { centro.x - ancho * 0.5f, centro.y - alto * 0.5f, ancho, alto };

    if (alfa <= 0.0f)
    {
        return resultado;
    }

    rlPushMatrix();
    rlTranslatef(centro.x, centro.y, 0.0f);
    rlRotatef(rotacionGrados, 0.0f, 0.0f, 1.0f);

    // Coordenadas locales: el centro de la placa es el origen.
    Rectangle local = { -ancho * 0.5f, -alto * 0.5f, ancho, alto };
    float pico = Px(7.0f);

    DrawRectangleRec(
        Rectangle{ local.x + Px(2.0f), local.y + Px(3.0f), local.width, local.height },
        Fade(BLACK, 0.25f * alfa)
    );

    if (opciones.pico)
    {
        DibujarTrianguloUI(
            Vector2{ -pico - Px(1.5f), local.y + local.height - Px(1.0f) },
            Vector2{ pico + Px(1.5f), local.y + local.height - Px(1.0f) },
            Vector2{ 0.0f, local.y + local.height + pico + Px(1.5f) },
            Fade(CONTORNO_MADERA, alfa)
        );

        DibujarTrianguloUI(
            Vector2{ -pico, local.y + local.height - Px(1.0f) },
            Vector2{ pico, local.y + local.height - Px(1.0f) },
            Vector2{ 0.0f, local.y + local.height + pico - Px(1.0f) },
            Fade(opciones.borde, alfa)
        );
    }

    DrawRectangleRec(local, Fade(opciones.borde, alfa));
    DrawRectangleRec(
        Rectangle{ local.x + borde, local.y + borde, local.width - borde * 2.0f, local.height - borde * 2.0f },
        Fade(UI_PLACA_CLARA, alfa)
    );
    DrawRectangleLinesEx(local, Px(1.0f), Fade(CONTORNO_MADERA, alfa));

    DrawText(
        texto,
        (int)(-anchoTexto * 0.5f),
        (int)(-tamano * 0.5f),
        tamano,
        Fade(UI_TINTA, alfa)
    );

    rlPopMatrix();

    return resultado;
}


//==================================================
// PARRAFO CON AJUSTE DE LINEA
//==================================================

// Espacio entre lineas, proporcional a la fuente (al reducirla, se
// reduce tambien).
static float InterlineadoParrafo(int tamano)
{
    float interlineado = tamano * 0.35f;

    return interlineado < 2.0f ? 2.0f : interlineado;
}


// Recorre el texto linea por linea con la fuente `tamano`. Si dibujar es
// true, dibuja cada linea alineada en [x, x + anchoMaximo] a partir de y.
// Devuelve la cantidad de lineas.
static int RecorrerParrafo(
    const char* texto,
    int tamano,
    float anchoMaximo,
    bool dibujar,
    float x,
    float y,
    AlineacionUI alineacion,
    Color color
)
{
    if (texto == nullptr || texto[0] == '\0')
    {
        return 0;
    }

    char linea[512];
    int lineas = 0;
    float paso = tamano + InterlineadoParrafo(tamano);
    const char* p = texto;

    while (true)
    {
        // La linea es [inicio, fin): palabras enteras que caben.
        const char* inicio = p;
        const char* fin = p;
        const char* q = p;

        while (true)
        {
            const char* palabra = q;

            while (*palabra == ' ')
            {
                palabra++;
            }

            if (*palabra == '\0' || *palabra == '\n')
            {
                q = palabra;
                break;
            }

            const char* finPalabra = palabra;

            while (*finPalabra != '\0' && *finPalabra != ' ' && *finPalabra != '\n')
            {
                finPalabra++;
            }

            int largo = (int)(finPalabra - inicio);
            largo = largo < (int)sizeof linea - 1 ? largo : (int)sizeof linea - 1;
            snprintf(linea, sizeof linea, "%.*s", largo, inicio);

            // La primera palabra siempre entra (nunca se corta una palabra).
            if (fin == inicio || (float)MeasureText(linea, tamano) <= anchoMaximo)
            {
                fin = finPalabra;
                q = finPalabra;
                continue;
            }

            // No cabe: la linea termina antes y la palabra abre la siguiente.
            q = palabra;
            break;
        }

        if (dibujar)
        {
            int largo = (int)(fin - inicio);
            largo = largo < (int)sizeof linea - 1 ? largo : (int)sizeof linea - 1;
            snprintf(linea, sizeof linea, "%.*s", largo, inicio);

            float anchoLinea = (float)MeasureText(linea, tamano);
            float xLinea = x;

            if (alineacion == UI_ALINEAR_CENTRO)
            {
                xLinea = x + (anchoMaximo - anchoLinea) * 0.5f;
            }
            else if (alineacion == UI_ALINEAR_DERECHA)
            {
                xLinea = x + anchoMaximo - anchoLinea;
            }

            DrawText(linea, (int)xLinea, (int)(y + lineas * paso), tamano, color);
        }

        lineas++;

        if (*q == '\0')
        {
            break;
        }

        // Salto de linea explicito: se consume; un ajuste por ancho no
        // consume nada (q ya apunta a la palabra siguiente).
        p = *q == '\n' ? q + 1 : q;
    }

    return lineas;
}


// Elige la fuente (reduciendo si hace falta) y mide el bloque.
static MedidaParrafoUI CalcularParrafo(
    const char* texto,
    int tokenTexto,
    float anchoMaximo,
    float altoMaximo
)
{
    MedidaParrafoUI medida;
    int tamano = TamanoTexto(tokenTexto);

    while (true)
    {
        int lineas = RecorrerParrafo(texto, tamano, anchoMaximo, false, 0.0f, 0.0f, UI_ALINEAR_IZQUIERDA, BLANK);

        medida.lineas = lineas;
        medida.tamano = tamano;
        medida.alto = lineas > 0 ? lineas * (float)tamano + (lineas - 1) * InterlineadoParrafo(tamano) : 0.0f;
        medida.cabe = altoMaximo <= 0.0f || medida.alto <= altoMaximo;

        if (medida.cabe || tamano <= TAMANO_TEXTO_MINIMO)
        {
            return medida;
        }

        tamano--;
    }
}


MedidaParrafoUI MedirParrafoUI(
    const char* texto,
    int tokenTexto,
    float anchoMaximo,
    float altoMaximo
)
{
    return CalcularParrafo(texto, tokenTexto, anchoMaximo, altoMaximo);
}


MedidaParrafoUI DibujarParrafoUI(
    const char* texto,
    float x,
    float y,
    int tokenTexto,
    float anchoMaximo,
    Color color,
    AlineacionUI alineacion,
    float altoMaximo
)
{
    MedidaParrafoUI medida = CalcularParrafo(texto, tokenTexto, anchoMaximo, altoMaximo);

    RecorrerParrafo(texto, medida.tamano, anchoMaximo, true, x, y, alineacion, color);

    return medida;
}


//==================================================
// SELLO / INSIGNIA
//==================================================

// Estrella de `puntas` puntas centrada en el origen local.
static void DibujarEstrellaSello(float radio, float radioInterior, int puntas, Color color)
{
    int vertices = puntas * 2;

    for (int i = 0; i < vertices; i++)
    {
        float a0 = (float)i / vertices * 2.0f * PI - PI * 0.5f;
        float a1 = (float)(i + 1) / vertices * 2.0f * PI - PI * 0.5f;
        float r0 = i % 2 == 0 ? radio : radioInterior;
        float r1 = i % 2 == 0 ? radioInterior : radio;

        DibujarTrianguloUI(
            Vector2{ 0.0f, 0.0f },
            Vector2{ std::cos(a0) * r0, std::sin(a0) * r0 },
            Vector2{ std::cos(a1) * r1, std::sin(a1) * r1 },
            color
        );
    }
}


void DibujarSelloUI(
    const char* texto,
    Vector2 centro,
    float radioRef,
    FormaSelloUI forma,
    Color color,
    float rotacionGrados,
    float alfa
)
{
    alfa = Limitar(alfa, 0.0f, 1.0f);

    if (alfa <= 0.0f)
    {
        return;
    }

    float radio = Px(radioRef);
    float contorno = Px(2.0f);
    float radioDisco = radio * 0.70f;
    const int puntas = 12;

    rlPushMatrix();
    rlTranslatef(centro.x, centro.y, 0.0f);
    rlRotatef(rotacionGrados, 0.0f, 0.0f, 1.0f);

    Vector2 origen = { 0.0f, 0.0f };
    Vector2 sombra = { Px(3.0f), Px(4.0f) };

    if (forma == UI_SELLO_ESTRELLA)
    {
        rlPushMatrix();
        rlTranslatef(sombra.x, sombra.y, 0.0f);
        DibujarEstrellaSello(radio, radio * 0.80f, puntas, Fade(BLACK, 0.25f * alfa));
        rlPopMatrix();

        DibujarEstrellaSello(radio + contorno, (radio + contorno) * 0.80f, puntas, Fade(CONTORNO_MADERA, alfa));
        DibujarEstrellaSello(radio, radio * 0.80f, puntas, Fade(color, alfa));
    }
    else
    {
        DrawCircleV(sombra, radio, Fade(BLACK, 0.25f * alfa));
        DrawCircleV(origen, radio + contorno, Fade(CONTORNO_MADERA, alfa));
        DrawCircleV(origen, radio, Fade(color, alfa));

        // Aro claro fino de sello de goma.
        DrawRing(origen, radio * 0.84f, radio * 0.84f + Px(1.5f), 0.0f, 360.0f, 48, Fade(UI_PLACA_CLARA, 0.7f * alfa));
    }

    // Disco claro con el texto en tinta (contraste del estilo).
    DrawCircleV(origen, radioDisco + Px(1.0f), Fade(CONTORNO_MADERA, alfa));
    DrawCircleV(origen, radioDisco, Fade(UI_PLACA_CLARA, alfa));

    // Fuente a la medida del disco: arranca en ~0,6 del radio y se reduce
    // hasta que el texto entre en la cuerda central.
    int tamano = (int)(radio * 0.62f);
    float anchoMaximo = radioDisco * 1.70f;

    tamano = tamano < TAMANO_TEXTO_MINIMO ? TAMANO_TEXTO_MINIMO : tamano;

    while (tamano > TAMANO_TEXTO_MINIMO && (float)MeasureText(texto, tamano) > anchoMaximo)
    {
        tamano--;
    }

    int anchoTexto = MeasureText(texto, tamano);

    DrawText(
        texto,
        (int)(-anchoTexto * 0.5f),
        (int)(-tamano * 0.5f),
        tamano,
        Fade(UI_TINTA, alfa)
    );

    rlPopMatrix();
}


//==================================================
// VELO
//==================================================

void VeloUI::Iniciar(bool visibleInicial)
{
    visible = visibleInicial;
    aparicion = visibleInicial ? 1.0f : 0.0f;
}


void ActualizarVeloUI(VeloUI& velo, const EstiloUI& estilo, float dt)
{
    float objetivo = velo.visible ? 1.0f : 0.0f;

    // Sin fundido con movimiento reducido (o duracion nula).
    if (estilo.reducirMovimiento || velo.duracion <= 0.0f)
    {
        velo.aparicion = objetivo;
        return;
    }

    float paso = dt / velo.duracion;

    if (velo.aparicion < objetivo)
    {
        velo.aparicion = Limitar(velo.aparicion + paso, 0.0f, objetivo);
    }
    else
    {
        velo.aparicion = Limitar(velo.aparicion - paso, objetivo, 1.0f);
    }
}


void DibujarVeloUI(float alfa, Color color)
{
    alfa = Limitar(alfa, 0.0f, 1.0f);

    if (alfa <= 0.0f)
    {
        return;
    }

    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(color, alfa));
}


void DibujarVeloUI(const VeloUI& velo)
{
    DibujarVeloUI(velo.opacidad * velo.aparicion, velo.color);
}


//==================================================
// BANDERIN CON MASTIL
//==================================================

static const int SEGMENTOS_BANDERIN = 12;


// Desplazamiento vertical de la tela en la posicion x (0 = mastil, 1 =
// punta): el borde pegado al mastil no se mueve.
static float OndaBanderin(float x01, float tiempo, float fase, float amplitud)
{
    return amplitud * x01 * std::sin(x01 * 2.0f * PI * 1.1f - tiempo * 4.2f + fase);
}


void DibujarBanderinUI(
    const EstiloUI& estilo,
    const char* texto,
    Rectangle refBandera,
    float altoMastilRef,
    Color color,
    bool ondear,
    float fase,
    float alfa
)
{
    if (!estilo.cargado || alfa <= 0.0f)
    {
        return;
    }

    Rectangle tela = RectRef(refBandera.x, refBandera.y, refBandera.width, refBandera.height);
    bool mover = ondear && !estilo.reducirMovimiento;
    float amplitud = mover ? Px(5.0f) : 0.0f;
    float cola = tela.height * 0.32f;               // largo de la cola en V
    float cuerpo = tela.width - cola;

    // Mastil: palo de madera con contorno y una perilla dorada arriba.
    float anchoMastil = Px(8.0f);
    Rectangle mastil = {
        tela.x - anchoMastil,
        tela.y - Px(12.0f),
        anchoMastil,
        Px(altoMastilRef) + Px(12.0f)
    };

    DrawRectangleRec(
        Rectangle{ mastil.x + Px(3.0f), mastil.y + Px(4.0f), mastil.width, mastil.height },
        MultiplicarAlfa(Fade(BLACK, 0.25f), alfa)
    );
    DrawRectangleRec(mastil, MultiplicarAlfa(UI_MADERA_MEDIA, alfa));
    DrawRectangleRec(Rectangle{ mastil.x + Px(1.5f), mastil.y, Px(2.0f), mastil.height }, MultiplicarAlfa(Escalar(UI_MADERA_MEDIA, 1.18f), alfa));
    DrawRectangleLinesEx(mastil, Px(1.5f), MultiplicarAlfa(CONTORNO_MADERA, alfa));
    DrawCircleV(Vector2{ mastil.x + anchoMastil * 0.5f, mastil.y }, Px(6.5f), MultiplicarAlfa(CONTORNO_MADERA, alfa));
    DrawCircleV(Vector2{ mastil.x + anchoMastil * 0.5f, mastil.y }, Px(5.0f), MultiplicarAlfa(UI_FOCO, alfa));

    // Bordes de la tela: arriba y abajo por columna (+ la cola).
    Vector2 arriba[SEGMENTOS_BANDERIN + 1];
    Vector2 abajo[SEGMENTOS_BANDERIN + 1];

    for (int i = 0; i <= SEGMENTOS_BANDERIN; i++)
    {
        float t = (float)i / SEGMENTOS_BANDERIN;
        float x = tela.x + cuerpo * t;
        float onda = OndaBanderin((x - tela.x) / tela.width, estilo.tiempo, fase, amplitud);

        arriba[i] = Vector2{ x, tela.y + onda };
        abajo[i] = Vector2{ x, tela.y + tela.height + onda };
    }

    float ondaPunta = OndaBanderin(1.0f, estilo.tiempo, fase, amplitud);
    float xMuesca = tela.x + cuerpo + cola * 0.45f;
    float ondaMuesca = OndaBanderin((xMuesca - tela.x) / tela.width, estilo.tiempo, fase, amplitud);

    Vector2 puntaArriba = { tela.x + tela.width, tela.y + ondaPunta };
    Vector2 puntaAbajo = { tela.x + tela.width, tela.y + tela.height + ondaPunta };
    Vector2 muesca = { xMuesca, tela.y + tela.height * 0.5f + ondaMuesca };

    // Sombra, contorno (la misma tela un poco mas grande) y tela.
    for (int capa = 0; capa < 3; capa++)
    {
        Vector2 d = { 0.0f, 0.0f };
        float crecer = 0.0f;
        Color c = MultiplicarAlfa(color, alfa);

        if (capa == 0)
        {
            d = Vector2{ Px(4.0f), Px(5.0f) };
            c = MultiplicarAlfa(Fade(BLACK, 0.25f), alfa);
        }
        else if (capa == 1)
        {
            crecer = Px(1.5f);
            c = MultiplicarAlfa(CONTORNO_MADERA, alfa);
        }

        for (int i = 0; i < SEGMENTOS_BANDERIN; i++)
        {
            Vector2 a0 = { arriba[i].x + d.x, arriba[i].y + d.y - crecer };
            Vector2 a1 = { arriba[i + 1].x + d.x, arriba[i + 1].y + d.y - crecer };
            Vector2 b0 = { abajo[i].x + d.x, abajo[i].y + d.y + crecer };
            Vector2 b1 = { abajo[i + 1].x + d.x, abajo[i + 1].y + d.y + crecer };

            DibujarTrianguloUI(a0, b0, b1, c);
            DibujarTrianguloUI(a0, b1, a1, c);
        }

        Vector2 aN = { arriba[SEGMENTOS_BANDERIN].x + d.x, arriba[SEGMENTOS_BANDERIN].y + d.y - crecer };
        Vector2 bN = { abajo[SEGMENTOS_BANDERIN].x + d.x, abajo[SEGMENTOS_BANDERIN].y + d.y + crecer };
        Vector2 pa = { puntaArriba.x + d.x + crecer, puntaArriba.y + d.y - crecer };
        Vector2 pb = { puntaAbajo.x + d.x + crecer, puntaAbajo.y + d.y + crecer };
        Vector2 m = { muesca.x + d.x - crecer, muesca.y + d.y };

        DibujarTrianguloUI(aN, m, pa, c);
        DibujarTrianguloUI(bN, pb, m, c);
        DibujarTrianguloUI(aN, bN, m, c);
    }

    // Franja clara para el texto (sigue la onda) y el texto en tinta,
    // inclinado segun la pendiente de la tela en su centro.
    float margenFranja = tela.height * 0.20f;
    int desde = 1;
    int hasta = SEGMENTOS_BANDERIN - 1;

    for (int i = desde; i < hasta; i++)
    {
        Vector2 a0 = { arriba[i].x, arriba[i].y + margenFranja };
        Vector2 a1 = { arriba[i + 1].x, arriba[i + 1].y + margenFranja };
        Vector2 b0 = { abajo[i].x, abajo[i].y - margenFranja };
        Vector2 b1 = { abajo[i + 1].x, abajo[i + 1].y - margenFranja };

        DibujarTrianguloUI(a0, b0, b1, MultiplicarAlfa(UI_PLACA_CLARA, alfa));
        DibujarTrianguloUI(a0, b1, a1, MultiplicarAlfa(UI_PLACA_CLARA, alfa));
    }

    float anchoFranja = arriba[hasta].x - arriba[desde].x;
    float altoFranja = tela.height - margenFranja * 2.0f;
    float centroX = (arriba[desde].x + arriba[hasta].x) * 0.5f;
    float x01 = (centroX - tela.x) / tela.width;
    float onda = OndaBanderin(x01, estilo.tiempo, fase, amplitud);
    float delta = 0.01f;
    float pendiente =
        (OndaBanderin(x01 + delta, estilo.tiempo, fase, amplitud) - onda) / (delta * tela.width);

    int tamano = TamanoTextoAjustadoUI(texto, UI_TEXTO, anchoFranja - Px(8.0f));

    // Si la franja es baja, la fuente tampoco puede pasarse de su alto.
    while (tamano > TAMANO_TEXTO_MINIMO && tamano > altoFranja - Px(4.0f))
    {
        tamano--;
    }

    int anchoTexto = MeasureText(texto, tamano);

    rlPushMatrix();
    rlTranslatef(centroX, tela.y + tela.height * 0.5f + onda, 0.0f);
    rlRotatef(std::atan(pendiente) * RAD2DEG, 0.0f, 0.0f, 1.0f);
    DrawText(texto, (int)(-anchoTexto * 0.5f), (int)(-tamano * 0.5f), tamano, MultiplicarAlfa(UI_TINTA, alfa));
    rlPopMatrix();
}


//==================================================
// MARCO DE ILUSTRACION CON RECORTE
//==================================================

Rectangle AreaMarcoIlustracionUI(Rectangle ref, float grosorMarcoRef)
{
    Rectangle rect = RectRef(ref.x, ref.y, ref.width, ref.height);
    float grosor = Px(grosorMarcoRef);

    // Pixeles enteros hacia adentro: el recorte nunca pisa el marco.
    float x0 = std::ceil(rect.x + grosor);
    float y0 = std::ceil(rect.y + grosor);
    float x1 = std::floor(rect.x + rect.width - grosor);
    float y1 = std::floor(rect.y + rect.height - grosor);

    return Rectangle{
        x0,
        y0,
        x1 > x0 ? x1 - x0 : 0.0f,
        y1 > y0 ? y1 - y0 : 0.0f
    };
}


Rectangle ComenzarMarcoIlustracionUI(
    const EstiloUI& estilo,
    Rectangle ref,
    float grosorMarcoRef,
    Color fondo
)
{
    Rectangle area = AreaMarcoIlustracionUI(ref, grosorMarcoRef);

    if (estilo.cargado)
    {
        Rectangle rect = RectRef(ref.x, ref.y, ref.width, ref.height);

        DrawRectangleRec(
            Rectangle{ rect.x + Px(5.0f), rect.y + Px(6.0f), rect.width, rect.height },
            Fade(BLACK, 0.28f)
        );

        // El 9-slice de madera tiene 12 texeles de borde: se escala para
        // que el borde mida grosorMarcoRef.
        DibujarNueveCortes(estilo.madera, 12, rect, EscalaUI() * grosorMarcoRef / 12.0f, WHITE);

        if (fondo.a > 0)
        {
            DrawRectangleRec(area, fondo);
        }
    }

    // Siempre se activa (aunque no haya estilo): asi Terminar es simetrico.
    BeginScissorMode((int)area.x, (int)area.y, (int)area.width, (int)area.height);

    return area;
}


void TerminarMarcoIlustracionUI(
    const EstiloUI& estilo,
    Rectangle ref,
    float grosorMarcoRef
)
{
    EndScissorMode();

    if (!estilo.cargado)
    {
        return;
    }

    Rectangle area = AreaMarcoIlustracionUI(ref, grosorMarcoRef);

    // Canto interior: sombra arriba (el contenido queda "hundido" en el
    // marco) y filete oscuro alrededor.
    float bisel = Px(6.0f);

    DrawRectangleGradientV(
        (int)area.x,
        (int)area.y,
        (int)area.width,
        (int)bisel,
        Fade(BLACK, 0.30f),
        Fade(BLACK, 0.0f)
    );

    DrawRectangleLinesEx(area, Px(1.5f), CONTORNO_MADERA);
}


//==================================================
// TERCERA TANDA: PIEZAS DEL HUD
//==================================================
//
// Copias fieles de los helpers static de Minigames/HudMinijuego.cpp y
// Gameplay/PartidaTablero.cpp (mismas medidas y colores): con alfa = 1
// el resultado es identico, para poder migrarlos sin cambio visual.

// Dorados de la moneda y el trofeo (los del HUD del tablero).
static const Color ORO_ICONO = { 255, 200, 60, 255 };
static const Color ORO_OSCURO_ICONO = { 200, 140, 30, 255 };
static const Color ORO_CLARO_ICONO = { 255, 230, 130, 255 };


// Grosor de linea en pixeles: nunca menos de 1 px.
static float GrosorLineaUI(float unidades)
{
    float grosor = Px(unidades);

    return grosor < 1.0f ? 1.0f : grosor;
}


//==================================================
// PINZA
//==================================================

void DibujarPinzaUI(Vector2 puntoSoga, float alfa)
{
    if (alfa <= 0.0f)
    {
        return;
    }

    Rectangle pinza = { puntoSoga.x - Px(4.0f), puntoSoga.y - Px(6.0f), Px(8.0f), Px(18.0f) };

    DrawRectangleRec(pinza, MultiplicarAlfa(UI_MADERA_MEDIA, alfa));
    DrawRectangleLinesEx(pinza, GrosorLineaUI(1.0f), MultiplicarAlfa(UI_TINTA, alfa));
    DrawRectangleRec(
        Rectangle{ pinza.x, pinza.y + Px(7.0f), pinza.width, Px(2.0f) },
        MultiplicarAlfa(UI_MADERA_OSCURA, alfa)
    );
}


//==================================================
// BANDERIN COLGANTE
//==================================================

// Tela con cola en V. (x, y) esquina superior izquierda, todo en pixeles.
static void DibujarFormaBanderinColgante(float x, float y, float ancho, float alto, Color color)
{
    float cola = alto * 0.26f;
    float cuerpo = alto - cola;
    float medio = x + ancho * 0.5f;

    DrawRectangleRec(Rectangle{ x, y, ancho, cuerpo }, color);

    // DrawTriangle: vertices en sentido antihorario en pantalla.
    DrawTriangle(Vector2{ x, y + cuerpo - 0.5f }, Vector2{ x, y + alto }, Vector2{ medio, y + cuerpo - 0.5f }, color);
    DrawTriangle(Vector2{ medio, y + cuerpo - 0.5f }, Vector2{ x + ancho, y + alto }, Vector2{ x + ancho, y + cuerpo - 0.5f }, color);
}


void DibujarBanderinColganteUI(
    Vector2 esquina,
    Color color,
    float anchoRef,
    float altoRef,
    float alfa
)
{
    if (alfa <= 0.0f)
    {
        return;
    }

    float ancho = Px(anchoRef);
    float alto = Px(altoRef);
    float borde = GrosorLineaUI(1.5f);

    DibujarFormaBanderinColgante(
        esquina.x - borde,
        esquina.y - borde,
        ancho + borde * 2.0f,
        alto + borde * 2.5f,
        MultiplicarAlfa(UI_TINTA, alfa)
    );
    DibujarFormaBanderinColgante(esquina.x, esquina.y, ancho, alto, MultiplicarAlfa(color, alfa));
}


//==================================================
// PLACA CON MARCO
//==================================================

Rectangle DibujarPlacaConMarcoUI(
    const EstiloUI& estilo,
    Rectangle rect,
    float marcoRef,
    float alfa
)
{
    float marco = Px(marcoRef);
    Rectangle placa = {
        rect.x + marco, rect.y + marco,
        rect.width - marco * 2.0f, rect.height - marco * 2.0f
    };

    if (!estilo.cargado || alfa <= 0.0f)
    {
        return placa;
    }

    DrawRectangleRec(
        Rectangle{ rect.x + Px(3.0f), rect.y + Px(4.0f), rect.width, rect.height },
        MultiplicarAlfa(Fade(BLACK, 0.25f), alfa)
    );

    DibujarNueveCortes(estilo.madera, 12, rect, EscalaUI() * 0.5f, MultiplicarAlfa(WHITE, alfa));
    DibujarNueveCortes(estilo.placa, 8, placa, EscalaUI(), MultiplicarAlfa(WHITE, alfa));

    return placa;
}


//==================================================
// TEXTO ALINEADO
//==================================================

int DibujarTextoAlineadoUI(
    const char* texto,
    float x,
    float y,
    int tokenTexto,
    float anchoMaximo,
    Color color,
    AlineacionUI alineacion
)
{
    int tamano = TamanoTextoAjustadoUI(texto, tokenTexto, anchoMaximo);
    float ancho = (float)MeasureText(texto, tamano);

    if (alineacion == UI_ALINEAR_CENTRO)
    {
        x -= ancho * 0.5f;
    }
    else if (alineacion == UI_ALINEAR_DERECHA)
    {
        x -= ancho;
    }

    DrawText(texto, (int)x, (int)y, tamano, color);

    return tamano;
}


int DibujarTextoDerechaUI(
    const char* texto,
    float derecha,
    float y,
    int tokenTexto,
    float anchoMaximo,
    Color color
)
{
    return DibujarTextoAlineadoUI(texto, derecha, y, tokenTexto, anchoMaximo, color, UI_ALINEAR_DERECHA);
}


//==================================================
// ICONOS: MONEDA Y TROFEO
//==================================================

void DibujarMonedaUI(Vector2 centro, float radioRef, float alfa)
{
    if (alfa <= 0.0f)
    {
        return;
    }

    float radio = Px(radioRef);

    DrawCircleV(centro, radio + GrosorLineaUI(1.5f), MultiplicarAlfa(UI_TINTA, alfa));
    DrawCircleV(centro, radio, MultiplicarAlfa(ORO_ICONO, alfa));
    DrawCircleV(centro, radio * 0.62f, MultiplicarAlfa(ORO_OSCURO_ICONO, alfa));
    DrawCircleV(centro, radio * 0.42f, MultiplicarAlfa(ORO_CLARO_ICONO, alfa));
}


// Piezas de la copa (alto t en pixeles), agrandadas `extra` pixeles para
// el contorno.
static void DibujarPiezasTrofeo(float cx, float cy, float t, float extra, Color color, Color colorPie)
{
    // Asas: anillos a cada lado de la copa.
    float radioAsa = t * 0.17f;
    float grosorAsa = t * 0.07f + extra;

    DrawRing(Vector2{ cx - t * 0.34f, cy - t * 0.26f }, radioAsa - grosorAsa, radioAsa + extra, 0.0f, 360.0f, 12, color);
    DrawRing(Vector2{ cx + t * 0.34f, cy - t * 0.26f }, radioAsa - grosorAsa, radioAsa + extra, 0.0f, 360.0f, 12, color);

    // Copa: borde recto arriba y fondo redondo.
    DrawRectangleRec(
        Rectangle{ cx - t * 0.34f - extra, cy - t * 0.50f - extra, t * 0.68f + extra * 2.0f, t * 0.26f + extra * 2.0f },
        color
    );
    DrawCircleSector(Vector2{ cx, cy - t * 0.25f }, t * 0.34f + extra, 0.0f, 180.0f, 14, color);

    // Tallo y pie.
    DrawRectangleRec(
        Rectangle{ cx - t * 0.07f - extra, cy - extra, t * 0.14f + extra * 2.0f, t * 0.34f + extra * 2.0f },
        color
    );
    DrawRectangleRec(
        Rectangle{ cx - t * 0.28f - extra, cy + t * 0.34f - extra, t * 0.56f + extra * 2.0f, t * 0.16f + extra * 2.0f },
        colorPie
    );
}


void DibujarTrofeoUI(Vector2 centro, float altoRef, float alfa)
{
    if (alfa <= 0.0f)
    {
        return;
    }

    float alto = Px(altoRef);
    Color contorno = MultiplicarAlfa(UI_TINTA, alfa);

    DibujarPiezasTrofeo(centro.x, centro.y, alto, GrosorLineaUI(1.5f), contorno, contorno);
    DibujarPiezasTrofeo(
        centro.x, centro.y, alto, 0.0f,
        MultiplicarAlfa(ORO_ICONO, alfa),
        MultiplicarAlfa(ORO_OSCURO_ICONO, alfa)
    );

    // Brillo.
    DrawRectangleRec(
        Rectangle{ centro.x - alto * 0.22f, centro.y - alto * 0.44f, alto * 0.08f, alto * 0.30f },
        MultiplicarAlfa(ORO_CLARO_ICONO, alfa)
    );
}
