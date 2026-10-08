#include "UI/RuletaMinijuegos.h"

#include "UI/MiniaturasMinijuegos.h"

#include <cmath>


//==================================================
// CONSTANTES
//==================================================

// Duracion aproximada total: 0.4 + (0.6 a 2.0) + 1.9 + 1.1 = 4 a 5.4 s.
static const float DURACION_INICIO_RULETA = 0.40f;
static const float VELOCIDAD_GIRO_RULETA = 16.0f;
static const float GIRO_MINIMO_RULETA = 0.60f;
static const float DURACION_FRENADO_RULETA = 1.90f;
static const float PAUSA_SELECCIONADO_RULETA = 1.10f;

// Confirmar se acepta despues del primer tramo y solo acelera el reloj.
static const float TIEMPO_MINIMO_CONFIRMAR_RULETA = 0.80f;
static const float ESCALA_TIEMPO_ACELERADA_RULETA = 2.6f;
static const float TIEMPO_MINIMO_SALTAR_PAUSA_RULETA = 0.30f;

static const float SEPARACION_TICKS_RULETA = 0.045f;
static const float ALTO_FRANJA_NOMBRE_RULETA = 36.0f;


//==================================================
// UTILIDADES
//==================================================

static int ModuloPositivo(
    int valor,
    int divisor
)
{
    if (divisor <= 0)
    {
        return 0;
    }

    int resultado = valor % divisor;

    return resultado < 0
        ? resultado + divisor
        : resultado;
}


static IdMinijuego ObtenerOpcionEnPosicion(
    const RuletaMinijuegos& ruleta,
    int posicion
)
{
    if (ruleta.cantidadOpciones <= 0)
    {
        return ruleta.minijuegoElegido;
    }

    return ruleta.opciones[
        ModuloPositivo(posicion, ruleta.cantidadOpciones)
    ];
}


// Frenado con curva cubica: arranca a la velocidad de giro
// (derivada inicial 3) y llega a velocidad cero en u = 1.
static float CurvaFrenadoRuleta(float u)
{
    float resto = 1.0f - u;
    return 1.0f - resto * resto * resto;
}


static float DistanciaFrenadoRuleta()
{
    return VELOCIDAD_GIRO_RULETA * DURACION_FRENADO_RULETA / 3.0f;
}


static void ReproducirSonidoRuleta(
    RuletaMinijuegos& ruleta,
    TipoSonidoJuego tipo
)
{
    if (ruleta.audio != nullptr)
    {
        ruleta.audio->ReproducirSonido(tipo);
    }
}


static void CambiarEstadoRuleta(
    RuletaMinijuegos& ruleta,
    EstadoRuletaMinijuegos nuevoEstado,
    float tiempoInicial
)
{
    ruleta.estado = nuevoEstado;
    ruleta.tiempoEstado = tiempoInicial;
}


//==================================================
// INICIAR
//==================================================

void RuletaMinijuegos::Iniciar(
    IdMinijuego elegido,
    AudioJuego* audioJuego
)
{
    audio = audioJuego;
    minijuegoElegido = elegido;

    cantidadOpciones = 0;
    indiceElegido = -1;

    int disponibles = ObtenerCantidadMinijuegosDisponiblesTablero();

    for (
        int i = 0;
        i < disponibles && cantidadOpciones < CANTIDAD_MINIJUEGOS;
        i++
    )
    {
        IdMinijuego id = ObtenerMinijuegoDisponibleTablero(i);

        if (id == elegido)
        {
            indiceElegido = cantidadOpciones;
        }

        opciones[cantidadOpciones] = id;
        cantidadOpciones++;
    }

    // El elegido siempre debe poder mostrarse. Si no estaba entre los
    // disponibles se agrega para que lo visto coincida con lo que se juega.
    if (indiceElegido < 0)
    {
        TraceLog(
            LOG_WARNING,
            "Ruleta: el minijuego elegido no figura como disponible para tablero."
        );

        if (cantidadOpciones >= CANTIDAD_MINIJUEGOS)
        {
            cantidadOpciones = CANTIDAD_MINIJUEGOS - 1;
        }

        indiceElegido = cantidadOpciones;
        opciones[cantidadOpciones] = elegido;
        cantidadOpciones++;
    }

    // Arranque visual aleatorio: no influye en el resultado.
    posicionInicial =
        (float)GetRandomValue(0, cantidadOpciones - 1);

    float finInicio =
        posicionInicial +
        VELOCIDAD_GIRO_RULETA * DURACION_INICIO_RULETA * 0.5f;

    float minimoInicioFrenado =
        finInicio +
        VELOCIDAD_GIRO_RULETA * GIRO_MINIMO_RULETA;

    float distanciaFrenado = DistanciaFrenadoRuleta();

    // Primera posicion entera, alcanzable tras el giro minimo,
    // en la que queda centrado exactamente el minijuego elegido.
    int destino =
        (int)std::ceil(minimoInicioFrenado + distanciaFrenado);

    destino +=
        ModuloPositivo(
            indiceElegido - destino,
            cantidadOpciones
        );

    posicionFinal = (float)destino;
    posicionInicioFrenado = posicionFinal - distanciaFrenado;

    posicion = posicionInicial;
    tiempoTotal = 0.0f;
    frenadoAcelerado = false;
    ultimaCasillaCentrada = (int)std::floor(posicion + 0.5f);
    tiempoDesdeUltimoTick = 0.0f;

    CambiarEstadoRuleta(*this, RULETA_INICIANDO, 0.0f);
}


//==================================================
// ACTUALIZAR
//==================================================

void RuletaMinijuegos::Actualizar(
    float deltaTime,
    bool confirmar
)
{
    if (
        estado == RULETA_INACTIVA ||
        estado == RULETA_TERMINADA
    )
    {
        return;
    }

    // Un pico de lag solo salta animacion; el destino no cambia.
    if (deltaTime > 0.10f)
    {
        deltaTime = 0.10f;
    }

    tiempoTotal += deltaTime;
    tiempoDesdeUltimoTick += deltaTime;

    bool puedeAcelerar =
        (
            estado == RULETA_GIRANDO ||
            estado == RULETA_FRENANDO
        ) &&
        tiempoTotal >= TIEMPO_MINIMO_CONFIRMAR_RULETA;

    if (confirmar && puedeAcelerar)
    {
        frenadoAcelerado = true;
    }

    float deltaAnimacion =
        frenadoAcelerado
        ? deltaTime * ESCALA_TIEMPO_ACELERADA_RULETA
        : deltaTime;

    switch (estado)
    {
        case RULETA_INICIANDO:
        {
            tiempoEstado += deltaAnimacion;

            if (tiempoEstado < DURACION_INICIO_RULETA)
            {
                // Aceleracion constante desde cero hasta la velocidad de giro.
                posicion =
                    posicionInicial +
                    VELOCIDAD_GIRO_RULETA *
                    tiempoEstado * tiempoEstado /
                    (2.0f * DURACION_INICIO_RULETA);

                break;
            }

            float exceso = tiempoEstado - DURACION_INICIO_RULETA;

            posicion =
                posicionInicial +
                VELOCIDAD_GIRO_RULETA * DURACION_INICIO_RULETA * 0.5f +
                VELOCIDAD_GIRO_RULETA * exceso;

            CambiarEstadoRuleta(*this, RULETA_GIRANDO, exceso);
            break;
        }

        case RULETA_GIRANDO:
        {
            tiempoEstado += deltaAnimacion;
            posicion += VELOCIDAD_GIRO_RULETA * deltaAnimacion;

            if (posicion >= posicionInicioFrenado)
            {
                // El tramo recorrido de mas se convierte en tiempo
                // de frenado para no perder continuidad.
                float exceso =
                    (posicion - posicionInicioFrenado) /
                    VELOCIDAD_GIRO_RULETA;

                CambiarEstadoRuleta(*this, RULETA_FRENANDO, exceso);

                posicion =
                    posicionInicioFrenado +
                    (posicionFinal - posicionInicioFrenado) *
                    CurvaFrenadoRuleta(
                        tiempoEstado / DURACION_FRENADO_RULETA
                    );
            }

            break;
        }

        case RULETA_FRENANDO:
        {
            tiempoEstado += deltaAnimacion;

            float progreso = tiempoEstado / DURACION_FRENADO_RULETA;

            if (progreso >= 1.0f)
            {
                posicion = posicionFinal;
                CambiarEstadoRuleta(*this, RULETA_SELECCIONADO, 0.0f);
                ReproducirSonidoRuleta(*this, SONIDO_RESULTADO);
                break;
            }

            posicion =
                posicionInicioFrenado +
                (posicionFinal - posicionInicioFrenado) *
                CurvaFrenadoRuleta(progreso);

            break;
        }

        case RULETA_SELECCIONADO:
        {
            tiempoEstado += deltaTime;

            bool saltarPausa =
                confirmar &&
                tiempoEstado >= TIEMPO_MINIMO_SALTAR_PAUSA_RULETA;

            if (
                saltarPausa ||
                tiempoEstado >= PAUSA_SELECCIONADO_RULETA
            )
            {
                CambiarEstadoRuleta(*this, RULETA_TERMINADA, 0.0f);
            }

            break;
        }

        case RULETA_INACTIVA:
        case RULETA_TERMINADA:
            break;
    }

    if (
        estado == RULETA_INICIANDO ||
        estado == RULETA_GIRANDO ||
        estado == RULETA_FRENANDO
    )
    {
        int casilla = (int)std::floor(posicion + 0.5f);

        if (casilla != ultimaCasillaCentrada)
        {
            ultimaCasillaCentrada = casilla;

            // A maxima velocidad pasan 16 miniaturas por segundo;
            // se limita el tick para que no se vuelva un zumbido.
            if (tiempoDesdeUltimoTick >= SEPARACION_TICKS_RULETA)
            {
                tiempoDesdeUltimoTick = 0.0f;
                ReproducirSonidoRuleta(*this, SONIDO_RULETA_TICK);
            }
        }
    }
}


//==================================================
// DIBUJAR
//==================================================

static int AjustarTamanoTexto(
    const char* texto,
    int tamanoMaximo,
    int tamanoMinimo,
    float anchoDisponible
)
{
    int tamano = tamanoMaximo;

    while (
        tamano > tamanoMinimo &&
        MeasureText(texto, tamano) > anchoDisponible
    )
    {
        tamano--;
    }

    return tamano;
}


static void DibujarTextoCentrado(
    const char* texto,
    float centroX,
    float y,
    int tamano,
    Color color
)
{
    DrawText(
        texto,
        (int)(centroX - MeasureText(texto, tamano) / 2.0f),
        (int)y,
        tamano,
        color
    );
}


static void DibujarTarjetaRuleta(
    IdMinijuego id,
    Rectangle rect,
    float oscurecer,
    bool destacada,
    float pulso
)
{
    DrawRectangleRec(rect, Color{ 22, 24, 32, 255 });

    DibujarMiniaturaMinijuego(id, rect, 2.6f);

    float altoFranja = ALTO_FRANJA_NOMBRE_RULETA;

    DrawRectangle(
        (int)rect.x,
        (int)(rect.y + rect.height - altoFranja),
        (int)rect.width,
        (int)altoFranja,
        Fade(BLACK, 0.74f)
    );

    const char* nombre = ObtenerDatosMinijuego(id).nombre;

    int tamanoNombre =
        AjustarTamanoTexto(
            nombre,
            20,
            8,
            rect.width - 10.0f
        );

    DibujarTextoCentrado(
        nombre,
        rect.x + rect.width / 2.0f,
        rect.y + rect.height - altoFranja / 2.0f - tamanoNombre / 2.0f,
        tamanoNombre,
        RAYWHITE
    );

    if (destacada)
    {
        float grosor = 5.0f + 3.0f * pulso;

        DrawRectangleLinesEx(
            Rectangle
            {
                rect.x - 8.0f,
                rect.y - 8.0f,
                rect.width + 16.0f,
                rect.height + 16.0f
            },
            2.0f,
            Fade(GOLD, 0.35f + 0.35f * pulso)
        );

        DrawRectangleLinesEx(rect, grosor, GOLD);
    }
    else
    {
        DrawRectangleLinesEx(rect, 3.0f, Fade(RAYWHITE, 0.55f));
    }

    if (oscurecer > 0.0f)
    {
        DrawRectangleRec(rect, Fade(BLACK, oscurecer));
    }
}


void RuletaMinijuegos::Dibujar() const
{
    if (estado == RULETA_INACTIVA)
    {
        return;
    }

    float ancho = (float)GetScreenWidth();
    float alto = (float)GetScreenHeight();
    float centroX = ancho / 2.0f;

    float aparicion = 1.0f;

    if (estado == RULETA_INICIANDO)
    {
        aparicion = tiempoEstado / DURACION_INICIO_RULETA;

        if (aparicion > 1.0f)
        {
            aparicion = 1.0f;
        }
    }

    DrawRectangle(
        0,
        0,
        (int)ancho,
        (int)alto,
        Fade(Color{ 10, 12, 18, 255 }, 0.82f * aparicion)
    );

    int tamanoTitulo = (int)(alto / 17.0f);
    if (tamanoTitulo < 26) tamanoTitulo = 26;
    if (tamanoTitulo > 58) tamanoTitulo = 58;

    DibujarTextoCentrado(
        "MINIJUEGO DE LA RONDA",
        centroX,
        alto * 0.09f,
        tamanoTitulo,
        Fade(RAYWHITE, aparicion)
    );

    // Tamano de tarjeta: al menos cinco visibles en cualquier resolucion.
    // Con la separacion elegida, las cinco centrales ocupan 4.1 anchos.
    float anchoTarjeta = (ancho - 32.0f) / 4.1f;

    if (anchoTarjeta > alto * 0.34f) anchoTarjeta = alto * 0.34f;
    if (anchoTarjeta > 380.0f) anchoTarjeta = 380.0f;

    float altoTarjeta = anchoTarjeta * 1.12f;
    float separacion = anchoTarjeta * 0.86f;
    float centroY = alto * 0.46f;

    bool seleccionado =
        estado == RULETA_SELECCIONADO ||
        estado == RULETA_TERMINADA;

    float pulso =
        seleccionado
        ? 0.5f + 0.5f * std::sin(tiempoEstado * 12.0f)
        : 0.0f;

    float base = std::floor(posicion);
    float fraccion = posicion - base;

    // Se dibuja de los extremos hacia el centro para que la
    // miniatura mas cercana al marcador quede por encima.
    const int ALCANCE = 4;
    const int CANTIDAD_RANURAS = ALCANCE * 2 + 1;

    int ranuras[CANTIDAD_RANURAS]{};

    for (int i = 0; i < CANTIDAD_RANURAS; i++)
    {
        ranuras[i] = i - ALCANCE;
    }

    for (int i = 1; i < CANTIDAD_RANURAS; i++)
    {
        int actual = ranuras[i];
        float distanciaActual = std::fabs((float)actual - fraccion);
        int j = i - 1;

        while (
            j >= 0 &&
            std::fabs((float)ranuras[j] - fraccion) < distanciaActual
        )
        {
            ranuras[j + 1] = ranuras[j];
            j--;
        }

        ranuras[j + 1] = actual;
    }

    for (int i = 0; i < CANTIDAD_RANURAS; i++)
    {
        int k = ranuras[i];

        float desplazamiento = (float)k - fraccion;
        float distanciaReal = std::fabs(desplazamiento);

        if (distanciaReal > 3.6f)
        {
            continue;
        }

        bool destacada = seleccionado && distanciaReal < 0.01f;

        float distanciaEscala =
            distanciaReal > 3.0f ? 3.0f : distanciaReal;

        float escala = 1.0f - 0.17f * distanciaEscala;

        if (destacada)
        {
            escala += 0.05f * pulso;
        }

        float x = centroX + desplazamiento * separacion;

        Rectangle rect =
        {
            x - anchoTarjeta * escala / 2.0f,
            centroY - altoTarjeta * escala / 2.0f,
            anchoTarjeta * escala,
            altoTarjeta * escala
        };

        float oscurecer = 0.30f * distanciaReal;
        if (oscurecer > 0.75f) oscurecer = 0.75f;

        if (seleccionado && !destacada)
        {
            oscurecer += 0.15f;
        }

        oscurecer = 1.0f - (1.0f - oscurecer) * aparicion;

        DibujarTarjetaRuleta(
            ObtenerOpcionEnPosicion(*this, (int)base + k),
            rect,
            oscurecer,
            destacada,
            pulso
        );
    }

    // Marcadores del centro.
    float margenMarcador = 14.0f;
    float yArriba = centroY - altoTarjeta / 2.0f - margenMarcador - 18.0f;
    float yAbajo = centroY + altoTarjeta / 2.0f + margenMarcador + 18.0f;

    Color colorMarcador = Fade(seleccionado ? GOLD : RAYWHITE, aparicion);

    DrawTriangle(
        { centroX + 16.0f, yArriba },
        { centroX, yArriba + 18.0f },
        { centroX - 16.0f, yArriba },
        colorMarcador
    );

    DrawTriangle(
        { centroX - 16.0f, yAbajo },
        { centroX, yAbajo - 18.0f },
        { centroX + 16.0f, yAbajo },
        colorMarcador
    );

    // Nombre de la miniatura centrada.
    IdMinijuego centrado =
        ObtenerOpcionEnPosicion(
            *this,
            (int)std::floor(posicion + 0.5f)
        );

    const DatosMinijuegoCatalogo& datos = ObtenerDatosMinijuego(centrado);

    int tamanoNombre = (int)(alto / 15.0f);
    if (tamanoNombre < 24) tamanoNombre = 24;
    if (tamanoNombre > 64) tamanoNombre = 64;

    tamanoNombre =
        AjustarTamanoTexto(
            datos.nombre,
            tamanoNombre,
            16,
            ancho - 40.0f
        );

    float yNombre = yAbajo + 18.0f;

    DibujarTextoCentrado(
        datos.nombre,
        centroX,
        yNombre,
        tamanoNombre,
        Fade(seleccionado ? GOLD : RAYWHITE, aparicion)
    );

    float yAyuda = alto - 46.0f;

    if (seleccionado)
    {
        int tamanoDescripcion =
            AjustarTamanoTexto(
                datos.descripcion,
                20,
                11,
                ancho - 40.0f
            );

        DibujarTextoCentrado(
            datos.descripcion,
            centroX,
            yNombre + tamanoNombre + 14.0f,
            tamanoDescripcion,
            LIGHTGRAY
        );

        DibujarTextoCentrado(
            "PREPARANDO MINIJUEGO...",
            centroX,
            yAyuda,
            18,
            Fade(RAYWHITE, 0.6f + 0.4f * pulso)
        );
    }
    else if (
        !frenadoAcelerado &&
        tiempoTotal >= TIEMPO_MINIMO_CONFIRMAR_RULETA
    )
    {
        DibujarTextoCentrado(
            "CONFIRMAR: FRENAR MAS RAPIDO",
            centroX,
            yAyuda,
            18,
            LIGHTGRAY
        );
    }
}


bool RuletaMinijuegos::Termino() const
{
    return estado == RULETA_TERMINADA;
}


IdMinijuego RuletaMinijuegos::ObtenerMinijuegoElegido() const
{
    return minijuegoElegido;
}
