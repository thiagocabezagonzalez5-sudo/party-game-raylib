#include "UI/RuletaOpciones.h"

#include "UI/EstiloUI.h"

#include <cmath>
#include <cstdio>


//==================================================
// CONSTANTES
//==================================================

static const float DURACION_GIRO_RULETA_OPC = 3.6f;
static const int VUELTAS_RULETA_OPC = 4;
static const float ESCALA_TIEMPO_ACELERADA_RULETA_OPC = 2.4f;
static const float ANGULO_MARCADOR_RULETA_OPC = -90.0f;     // arriba
static const float PI_RULETA_OPC = 3.14159265f;


//==================================================
// UTILIDADES
//==================================================

static float ModuloAnguloRuletaOpc(float angulo)
{
    float resultado = std::fmod(angulo, 360.0f);

    return resultado < 0.0f
        ? resultado + 360.0f
        : resultado;
}


// Frenado cubico: arranca rapido y llega a velocidad cero en u = 1.
static float CurvaGiroRuletaOpc(float u)
{
    if (u >= 1.0f)
    {
        return 1.0f;
    }

    float resto = 1.0f - u;

    return 1.0f - resto * resto * resto;
}


static void ReproducirSonidoRuletaOpc(
    const RuletaOpciones& ruleta,
    TipoSonidoJuego tipo
)
{
    if (ruleta.audio != nullptr)
    {
        ruleta.audio->ReproducirSonido(tipo);
    }
}


//==================================================
// LOGICA
//==================================================

void RuletaOpciones::Iniciar(
    const SectorRuletaOpciones sectoresNuevos[],
    int cantidadSectores,
    int sectorElegido,
    AudioJuego* audioJuego
)
{
    audio = audioJuego;

    cantidad = cantidadSectores;

    if (cantidad > MAX_SECTORES_RULETA_OPCIONES)
    {
        cantidad = MAX_SECTORES_RULETA_OPCIONES;
    }

    if (cantidad < 1)
    {
        cantidad = 1;
    }

    for (int i = 0; i < cantidad; i++)
    {
        sectores[i] = i < cantidadSectores
            ? sectoresNuevos[i]
            : SectorRuletaOpciones{};
    }

    sectorGanador =
        sectorElegido >= 0 && sectorElegido < cantidad
        ? sectorElegido
        : 0;

    // Arranque visual al azar: no influye en el resultado.
    anguloInicial = (float)GetRandomValue(0, 359);
    angulo = anguloInicial;

    // El marcador debe quedar dentro del sector ganador, lejos de los
    // bordes (entre el 25 % y el 75 % de su ancho).
    float ancho = 360.0f / (float)cantidad;
    float fraccion = 0.25f + 0.5f * (float)GetRandomValue(0, 100) / 100.0f;
    float objetivo =
        ANGULO_MARCADOR_RULETA_OPC -
        ((float)sectorGanador + fraccion) * ancho;

    anguloFinal =
        anguloInicial +
        360.0f * (float)VUELTAS_RULETA_OPC +
        ModuloAnguloRuletaOpc(objetivo - anguloInicial);

    tiempo = 0.0f;
    tiempoDetenida = 0.0f;
    acelerada = false;
    ultimoSector = SectorBajoMarcador();
    estado = RULETA_OPC_ESPERANDO;
}


void RuletaOpciones::Girar()
{
    if (estado != RULETA_OPC_ESPERANDO)
    {
        return;
    }

    tiempo = 0.0f;
    estado = RULETA_OPC_GIRANDO;
}


int RuletaOpciones::SectorBajoMarcador() const
{
    if (cantidad <= 0)
    {
        return 0;
    }

    float ancho = 360.0f / (float)cantidad;
    float relativo =
        ModuloAnguloRuletaOpc(ANGULO_MARCADOR_RULETA_OPC - angulo);

    int sector = (int)std::floor(relativo / ancho);

    if (sector >= cantidad)
    {
        sector = cantidad - 1;
    }

    return sector;
}


void RuletaOpciones::Actualizar(
    float deltaTime,
    bool acelerar
)
{
    if (estado == RULETA_OPC_DETENIDA)
    {
        tiempoDetenida += deltaTime;
        return;
    }

    if (estado != RULETA_OPC_GIRANDO)
    {
        return;
    }

    // Un pico de lag solo salta animacion; el destino no cambia.
    if (deltaTime > 0.10f)
    {
        deltaTime = 0.10f;
    }

    if (acelerar)
    {
        acelerada = true;
    }

    tiempo +=
        acelerada
        ? deltaTime * ESCALA_TIEMPO_ACELERADA_RULETA_OPC
        : deltaTime;

    float u = tiempo / DURACION_GIRO_RULETA_OPC;

    angulo =
        anguloInicial +
        (anguloFinal - anguloInicial) * CurvaGiroRuletaOpc(u);

    int sector = SectorBajoMarcador();

    if (sector != ultimoSector)
    {
        ultimoSector = sector;
        ReproducirSonidoRuletaOpc(*this, SONIDO_RULETA_TICK);
    }

    if (u >= 1.0f)
    {
        angulo = anguloFinal;
        tiempoDetenida = 0.0f;
        estado = RULETA_OPC_DETENIDA;
    }
}


bool RuletaOpciones::EstaGirando() const
{
    return estado == RULETA_OPC_GIRANDO;
}


bool RuletaOpciones::Termino() const
{
    return estado == RULETA_OPC_DETENIDA;
}


//==================================================
// DIBUJO
//==================================================

static void DibujarTextoRadialRuletaOpc(
    const char* texto,
    Vector2 punto,
    float anguloGrados,
    float anchoMaximo,
    int tamanoBase,
    Color color
)
{
    Font fuente = GetFontDefault();
    int tamano = tamanoBase;

    while (
        tamano > 10 &&
        MeasureTextEx(fuente, texto, (float)tamano, (float)(tamano / 10)).x > anchoMaximo
    )
    {
        tamano--;
    }

    float espaciado = (float)(tamano / 10);
    Vector2 medida = MeasureTextEx(fuente, texto, (float)tamano, espaciado);

    // La mitad izquierda se lee del reves: se gira media vuelta para que
    // el texto quede siempre de pie.
    float rotacion = anguloGrados;
    float radianes = anguloGrados * PI_RULETA_OPC / 180.0f;

    if (std::cos(radianes) < 0.0f)
    {
        rotacion += 180.0f;
    }

    DrawTextPro(
        fuente, texto, punto,
        Vector2{ medida.x * 0.5f, medida.y * 0.5f },
        rotacion, (float)tamano, espaciado, color
    );
}


void RuletaOpciones::Dibujar(
    Vector2 centro,
    float radio,
    float tiempoReloj
) const
{
    if (estado == RULETA_OPC_INACTIVA || cantidad <= 0)
    {
        return;
    }

    float ancho = 360.0f / (float)cantidad;
    int segmentos = (int)std::ceil(ancho / 5.0f) + 2;
    float borde = radio * 0.075f;

    // Sombra y aro de madera.
    DrawCircleV(Vector2{ centro.x + radio * 0.04f, centro.y + radio * 0.06f }, radio + borde, Fade(BLACK, 0.35f));
    DrawCircleV(centro, radio + borde, UI_TINTA);
    DrawCircleV(centro, radio + borde - Px(2.0f), UI_MADERA_OSCURA);
    DrawCircleV(centro, radio + Px(1.0f), UI_TINTA);

    // Sectores.
    for (int i = 0; i < cantidad; i++)
    {
        float inicio = angulo + (float)i * ancho;

        DrawCircleSector(centro, radio, inicio, inicio + ancho, segmentos, sectores[i].color);
    }

    // Separadores.
    for (int i = 0; i < cantidad; i++)
    {
        float radianes = (angulo + (float)i * ancho) * PI_RULETA_OPC / 180.0f;
        Vector2 punta =
        {
            centro.x + std::cos(radianes) * radio,
            centro.y + std::sin(radianes) * radio
        };

        DrawLineEx(centro, punta, Px(2.0f), Fade(UI_TINTA, 0.85f));
    }

    // Textos radiales.
    int tamanoBase = (int)(radio * 0.115f);

    if (tamanoBase < 10)
    {
        tamanoBase = 10;
    }

    for (int i = 0; i < cantidad; i++)
    {
        float medio = angulo + ((float)i + 0.5f) * ancho;
        float radianes = medio * PI_RULETA_OPC / 180.0f;
        float distancia = radio * 0.60f;
        Vector2 punto =
        {
            centro.x + std::cos(radianes) * distancia,
            centro.y + std::sin(radianes) * distancia
        };

        DibujarTextoRadialRuletaOpc(
            sectores[i].texto, punto, medio,
            radio * 0.62f, tamanoBase, UI_PLACA_CLARA
        );
    }

    // Clavos del aro.
    for (int i = 0; i < cantidad; i++)
    {
        float radianes = (angulo + (float)i * ancho) * PI_RULETA_OPC / 180.0f;
        Vector2 clavo =
        {
            centro.x + std::cos(radianes) * (radio + borde * 0.55f),
            centro.y + std::sin(radianes) * (radio + borde * 0.55f)
        };

        DrawCircleV(clavo, borde * 0.28f, UI_CUERDA);
    }

    // Sector ganador con la ruleta quieta: aro dorado que late.
    if (estado == RULETA_OPC_DETENIDA)
    {
        float inicio = angulo + (float)sectorGanador * ancho;
        float pulso = 0.5f + 0.5f * std::sin(tiempoReloj * 7.0f);

        DrawCircleSector(centro, radio, inicio, inicio + ancho, segmentos, Fade(WHITE, 0.10f + 0.16f * pulso));
        DrawRing(centro, radio * 0.90f, radio * 0.985f, inicio, inicio + ancho, segmentos, UI_FOCO);
    }

    // Eje central.
    DrawCircleV(centro, radio * 0.15f, UI_TINTA);
    DrawCircleV(centro, radio * 0.125f, UI_MADERA_MEDIA);
    DrawCircleV(centro, radio * 0.055f, UI_FOCO);

    // Marcador fijo arriba, apuntando hacia abajo.
    float mitad = radio * 0.11f;
    float yArriba = centro.y - radio - borde * 1.25f;
    float yPunta = centro.y - radio + radio * 0.10f;
    float contorno = Px(2.5f);

    DrawTriangle(
        Vector2{ centro.x, yPunta + contorno },
        Vector2{ centro.x + mitad + contorno, yArriba - contorno },
        Vector2{ centro.x - mitad - contorno, yArriba - contorno },
        UI_TINTA
    );
    DrawTriangle(
        Vector2{ centro.x, yPunta },
        Vector2{ centro.x + mitad, yArriba },
        Vector2{ centro.x - mitad, yArriba },
        UI_FOCO
    );
}
