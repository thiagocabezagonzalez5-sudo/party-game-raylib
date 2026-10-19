#include "Minigames/MinijuegoVagonetasDuelo.h"

#include "Minigames/AudioMinijuegos.h"
#include "Minigames/EfectosVisualesMinijuegos.h"
#include "Minigames/HudMinijuego.h"
#include "Minigames/ModeloJugadorCompartido.h"
#include "Minigames/MecanicasJugador.h"
#include "Minigames/UtilidadesMinijuegos.h"
#include "Systems/CalidadGrafica.h"
#include "Systems/Input.h"
#include "UI/EstiloUI.h"

#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"

#include <algorithm>
#include <cmath>
#include <cstdio>


//==================================================
// VAGONETAS DE MINA
//==================================================
//
// Carrera 1v1 en pantalla dividida sobre una pista procedural de 560 m con
// tres rieles. La pista es una lista de elementos ordenados por distancia
// (rocas, huecos, vigas, monedas, placas) generada con una semilla: los dos
// carros leen la MISMA lista. El mundo se dibuja a partir de una curva
// x = f(s): cada carro vive en su propio sistema (s, lateral), y la
// camara de cada mitad lo sigue desde atras y arriba.
//==================================================


static const float LARGO_PISTA_VAGONETAS = 560.0f;
static const float DURACION_PREPARACION_VAGONETAS = 3.0f;
static const float TOPE_TIEMPO_VAGONETAS = 45.0f;
static const float DURACION_POSCARRERA_VAGONETAS = 6.0f;

static const float SEPARACION_RIELES_VAGONETAS = 1.7f;
static const float VELOCIDAD_CRUCERO_VAGONETAS = 15.5f;
static const float BONO_TURBO_VAGONETAS = 9.5f;
static const float BONO_PLACA_VAGONETAS = 7.0f;
static const float DURACION_TURBO_VAGONETAS = 0.85f;
static const float DURACION_PLACA_VAGONETAS = 0.8f;
static const float COSTO_TURBO_VAGONETAS = 0.34f;
static const float RECARGA_ENERGIA_VAGONETAS = 0.15f;
static const float ESPERA_ENTRE_TURBOS_VAGONETAS = 0.30f;
static const float ACELERACION_VAGONETAS = 24.0f;
static const float FRENADO_VAGONETAS = 34.0f;
static const float IMPULSO_SALTO_VAGONETAS = 5.4f;
static const float GRAVEDAD_VAGONETAS = 17.0f;
static const float AYUDA_GOMA_VAGONETAS = 0.05f;
static const float DISTANCIA_GOMA_VAGONETAS = 60.0f;

// Alturas de la via (metros sobre el suelo de roca).
static const float Y_BALASTO_VAGONETAS = 0.12f;
static const float Y_DURMIENTE_VAGONETAS = 0.22f;
static const float Y_RIEL_VAGONETAS = 0.34f;
static const float SEPARACION_RUEDAS_VAGONETAS = 0.38f;


//==================================================
// ALEATORIO Y PISTA
//==================================================


static unsigned int SiguienteAleatorioVagonetas(unsigned int& semilla)
{
    if (semilla == 0) semilla = 0x9E3779B9u;
    semilla ^= semilla << 13;
    semilla ^= semilla >> 17;
    semilla ^= semilla << 5;
    return semilla;
}


static float Aleatorio01Vagonetas(unsigned int& semilla)
{
    return (float)(SiguienteAleatorioVagonetas(semilla) & 0xFFFFFFu) / 16777216.0f;
}


static float RangoAleatorioVagonetas(unsigned int& semilla, float minimo, float maximo)
{
    return minimo + (maximo - minimo) * Aleatorio01Vagonetas(semilla);
}


static float CurvaXVagonetas(float s)
{
    if (s <= 0.0f) return 0.0f;

    float rampa = s >= 60.0f ? 1.0f : s / 60.0f;
    rampa = rampa * rampa * (3.0f - 2.0f * rampa);

    return rampa * (
        7.0f * std::sin(s * 0.021f) +
        3.4f * std::sin(s * 0.049f + 1.1f)
    );
}


static float RumboPistaVagonetas(float s)
{
    float pendiente = (CurvaXVagonetas(s + 0.6f) - CurvaXVagonetas(s - 0.6f)) / 1.2f;
    return std::atan(pendiente);
}


// Punto de la pista a distancia s y desplazamiento lateral lat (derecha > 0).
// El carro avanza hacia -Z: asi +X queda a la derecha de la camara.
static Vector3 PosicionPistaVagonetas(float s, float lateral)
{
    float rumbo = RumboPistaVagonetas(s);

    return {
        CurvaXVagonetas(s) + lateral * std::cos(rumbo),
        0.0f,
        -s + lateral * std::sin(rumbo)
    };
}


static bool EsPeligroVagonetas(TipoElementoVagonetas tipo)
{
    return
        tipo == ELEMENTO_VAGONETAS_ROCA ||
        tipo == ELEMENTO_VAGONETAS_HUECO ||
        tipo == ELEMENTO_VAGONETAS_VIGA;
}


static bool CarrilEnMascaraVagonetas(int mascara, int carril)
{
    return (mascara & (1 << carril)) != 0;
}


static float LateralCarrilVagonetas(int carril)
{
    return (float)(carril - 1) * SEPARACION_RIELES_VAGONETAS;
}


static void AgregarElementoVagonetas(
    MinijuegoVagonetasDuelo& minijuego,
    TipoElementoVagonetas tipo,
    int mascara,
    float distancia,
    float largo = 1.0f
)
{
    if (minijuego.cantidadElementos >= MAX_ELEMENTOS_VAGONETAS) return;

    ElementoPistaVagonetas& elemento =
        minijuego.elementos[minijuego.cantidadElementos++];
    elemento.tipo = tipo;
    elemento.carriles = mascara;
    elemento.distancia = distancia;
    elemento.largo = largo;
}


struct PatronVagonetas
{
    float distancia = 0.0f;
    TipoElementoVagonetas tipo = ELEMENTO_VAGONETAS_ROCA;
    int mascara = 0;
    float largo = 1.0f;
    int carrilLibre = 1;        // un carril sin peligro (guia para monedas)
};


static int CarrilAleatorioVagonetas(unsigned int& semilla)
{
    return (int)(Aleatorio01Vagonetas(semilla) * 3.0f) % 3;
}


static void GenerarPistaVagonetas(MinijuegoVagonetasDuelo& minijuego)
{
    unsigned int semilla = minijuego.semillaPista;
    minijuego.cantidadElementos = 0;

    PatronVagonetas patrones[48];
    int cantidadPatrones = 0;
    float z = 46.0f;
    int anterior = -1;

    while (z < LARGO_PISTA_VAGONETAS - 40.0f && cantidadPatrones < 44)
    {
        // Sin repetir el mismo tipo dos veces seguidas.
        int tipo = 0;
        do
        {
            float r = Aleatorio01Vagonetas(semilla);
            tipo = r < 0.28f ? 0 : r < 0.42f ? 1 : r < 0.56f ? 2 : r < 0.66f ? 3
                : r < 0.80f ? 4 : 5;
        } while (tipo == anterior && tipo != 0);
        anterior = tipo;

        PatronVagonetas& p = patrones[cantidadPatrones];
        p = PatronVagonetas{};
        p.distancia = z;
        int libre = CarrilAleatorioVagonetas(semilla);
        p.carrilLibre = libre;
        float avance = RangoAleatorioVagonetas(semilla, 21.0f, 30.0f);

        switch (tipo)
        {
        case 0:     // roca en un riel
            p.tipo = ELEMENTO_VAGONETAS_ROCA;
            p.mascara = 1 << CarrilAleatorioVagonetas(semilla);
            p.carrilLibre = (p.mascara & (1 << libre)) ? (libre + 1) % 3 : libre;
            break;

        case 1:     // roca doble: un solo riel libre
            p.tipo = ELEMENTO_VAGONETAS_ROCA;
            p.mascara = 7 ^ (1 << libre);
            break;

        case 2:     // hueco en un riel
            p.tipo = ELEMENTO_VAGONETAS_HUECO;
            p.mascara = 1 << CarrilAleatorioVagonetas(semilla);
            p.largo = 4.4f;
            p.carrilLibre = (p.mascara & (1 << libre)) ? (libre + 1) % 3 : libre;
            break;

        case 3:     // hueco doble
            p.tipo = ELEMENTO_VAGONETAS_HUECO;
            p.mascara = 7 ^ (1 << libre);
            p.largo = 4.4f;
            break;

        case 4:     // viga a todo lo ancho: saltar
            p.tipo = ELEMENTO_VAGONETAS_VIGA;
            p.mascara = 7;
            p.carrilLibre = 1;
            break;

        default:    // eslalon: dos rocas dobles, hueco libre contiguo
        {
            p.tipo = ELEMENTO_VAGONETAS_ROCA;
            p.mascara = 7 ^ (1 << libre);
            int siguiente = libre == 1
                ? (Aleatorio01Vagonetas(semilla) < 0.5f ? 0 : 2)
                : 1;

            if (cantidadPatrones + 1 < 44)
            {
                PatronVagonetas& segundo = patrones[cantidadPatrones + 1];
                segundo = PatronVagonetas{};
                segundo.distancia = z + 11.0f;
                segundo.tipo = ELEMENTO_VAGONETAS_ROCA;
                segundo.mascara = 7 ^ (1 << siguiente);
                segundo.carrilLibre = siguiente;
                cantidadPatrones++;
                avance = 11.0f + RangoAleatorioVagonetas(semilla, 20.0f, 26.0f);
            }
            break;
        }
        }

        cantidadPatrones++;
        z += avance;
    }

    // Peligros y guias: monedas y placas entre un peligro y el siguiente.
    for (int i = 0; i < cantidadPatrones; i++)
    {
        const PatronVagonetas& p = patrones[i];
        AgregarElementoVagonetas(minijuego, p.tipo, p.mascara, p.distancia, p.largo);

        // Arco de monedas altas sobre huecos y vigas: premio al salto.
        if (p.tipo != ELEMENTO_VAGONETAS_ROCA)
        {
            int carrilArco = p.mascara == 7 ? 1 : (p.mascara & 1) ? 0 : (p.mascara & 4) ? 2 : 1;
            if (p.tipo == ELEMENTO_VAGONETAS_HUECO && p.mascara != 7 && p.mascara != (1 << carrilArco))
            {
                carrilArco = 0;
                while (!CarrilEnMascaraVagonetas(p.mascara, carrilArco)) carrilArco++;
            }

            for (int k = -1; k <= 1; k++)
            {
                AgregarElementoVagonetas(
                    minijuego,
                    ELEMENTO_VAGONETAS_MONEDA_ALTA,
                    1 << carrilArco,
                    p.distancia + k * 2.1f
                );
            }
        }

        if (i + 1 >= cantidadPatrones) continue;

        const PatronVagonetas& siguiente = patrones[i + 1];
        float hueco = siguiente.distancia - p.distancia;
        if (hueco < 16.0f) continue;

        // Fila de monedas hacia el carril libre del siguiente peligro.
        int carrilMonedas = siguiente.carrilLibre;
        float inicio = p.distancia + 7.0f;
        int cantidadMonedas = 3 + (int)(Aleatorio01Vagonetas(semilla) * 3.0f);

        for (int k = 0; k < cantidadMonedas; k++)
        {
            float d = inicio + k * 2.2f;
            if (d > siguiente.distancia - 6.0f) break;
            AgregarElementoVagonetas(minijuego, ELEMENTO_VAGONETAS_MONEDA, 1 << carrilMonedas, d);
        }

        // Placa de impulso: casi siempre en un carril seguro; a veces en
        // el carril del peligro (premio con riesgo: hay que esquivar).
        if (Aleatorio01Vagonetas(semilla) < 0.34f)
        {
            bool arriesgada = Aleatorio01Vagonetas(semilla) < 0.30f;
            int carrilPlaca = carrilMonedas;

            if (arriesgada)
            {
                carrilPlaca = 0;
                while (carrilPlaca < 2 && !CarrilEnMascaraVagonetas(siguiente.mascara, carrilPlaca)) carrilPlaca++;
            }

            float d = siguiente.distancia - (arriesgada ? 12.0f : 10.0f);
            if (d > p.distancia + 6.0f)
            {
                AgregarElementoVagonetas(minijuego, ELEMENTO_VAGONETAS_PLACA, 1 << carrilPlaca, d);
            }
        }
    }

    // Una placa de regalo cerca de la salida y monedas de recibimiento.
    AgregarElementoVagonetas(minijuego, ELEMENTO_VAGONETAS_PLACA, 1 << 1, 24.0f);

    std::stable_sort(
        minijuego.elementos,
        minijuego.elementos + minijuego.cantidadElementos,
        [](const ElementoPistaVagonetas& a, const ElementoPistaVagonetas& b)
        {
            return a.distancia < b.distancia;
        }
    );
}


//==================================================
// EFECTOS Y SONIDO
//==================================================


struct EntradaVagonetas
{
    bool izquierda = false;
    bool derecha = false;
    bool saltar = false;
    bool turbo = false;
};


static void SonarVagonetas(
    const MinijuegoVagonetasDuelo& minijuego,
    const CarroVagonetas& carro,
    TipoSonidoJuego sonido,
    bool soloHumanos
)
{
    if (soloHumanos && carro.controlIA) return;
    ReproducirSonidoMinijuego(minijuego.audio, sonido);
}


static void PonerAvisoVagonetas(CarroVagonetas& carro, AvisoVagonetas tipo)
{
    int mejor = 0;
    for (int i = 1; i < MAX_AVISOS_VAGONETAS; i++)
    {
        if (carro.avisos[i].tiempo < carro.avisos[mejor].tiempo) mejor = i;
    }

    carro.avisos[mejor].tipo = tipo;
    carro.avisos[mejor].tiempo = 1.1f;
}


static void EmitirParticulaVagonetas(
    MinijuegoVagonetasDuelo& minijuego,
    int carro,
    Vector3 posicion,
    Vector3 velocidad,
    float vida,
    float tamano,
    Color color,
    bool polvo
)
{
    for (int i = 0; i < MAX_PARTICULAS_VAGONETAS; i++)
    {
        ParticulaVagonetas& p = minijuego.particulas[carro][i];
        if (p.activa) continue;

        p.activa = true;
        p.polvo = polvo;
        p.posicion = posicion;
        p.velocidad = velocidad;
        p.vida = vida;
        p.vidaMaxima = vida;
        p.tamano = tamano;
        p.color = color;
        return;
    }
}


// Cantidad ajustada por la calidad de particulas (el azar de efectos usa su
// propia semilla: la calidad nunca cambia la jugabilidad).
static int CantidadEfectoVagonetas(int cantidad)
{
    float factor = FactorCalidadGrafica(CalidadParticulas());
    int ajustada = (int)std::lround((float)cantidad * factor);
    return ajustada < 1 && cantidad > 0 ? 1 : ajustada;
}


static void ChispasVagonetas(
    MinijuegoVagonetasDuelo& minijuego,
    int carro,
    Vector3 origen,
    int cantidad,
    float fuerza,
    bool azules
)
{
    unsigned int& azar = minijuego.semillaEfectos;
    int total = CantidadEfectoVagonetas(cantidad);

    for (int i = 0; i < total; i++)
    {
        Vector3 velocidad = {
            RangoAleatorioVagonetas(azar, -1.0f, 1.0f) * fuerza,
            RangoAleatorioVagonetas(azar, 0.5f, 1.4f) * fuerza,
            RangoAleatorioVagonetas(azar, -1.0f, 1.0f) * fuerza
        };

        float mezcla = Aleatorio01Vagonetas(azar);
        Color color = azules
            ? Color{ (unsigned char)(150 + 90 * mezcla), (unsigned char)(220 + 30 * mezcla), 255, 255 }
            : Color{ 255, (unsigned char)(170 + 70 * mezcla), (unsigned char)(40 + 90 * mezcla), 255 };

        EmitirParticulaVagonetas(
            minijuego, carro, origen, velocidad,
            RangoAleatorioVagonetas(azar, 0.25f, 0.6f),
            RangoAleatorioVagonetas(azar, 0.05f, 0.11f),
            color, false
        );
    }
}


static void PolvoVagonetas(
    MinijuegoVagonetasDuelo& minijuego,
    int carro,
    Vector3 origen,
    int cantidad,
    float fuerza
)
{
    unsigned int& azar = minijuego.semillaEfectos;
    int total = CantidadEfectoVagonetas(cantidad);

    for (int i = 0; i < total; i++)
    {
        Vector3 velocidad = {
            RangoAleatorioVagonetas(azar, -1.0f, 1.0f) * fuerza,
            RangoAleatorioVagonetas(azar, 0.2f, 0.9f) * fuerza,
            RangoAleatorioVagonetas(azar, -1.0f, 1.0f) * fuerza
        };

        EmitirParticulaVagonetas(
            minijuego, carro, origen, velocidad,
            RangoAleatorioVagonetas(azar, 0.45f, 0.8f),
            RangoAleatorioVagonetas(azar, 0.16f, 0.30f),
            Color{ 150, 124, 112, 255 }, true
        );
    }
}


static void ConfetiVagonetas(
    MinijuegoVagonetasDuelo& minijuego,
    int carro,
    Vector3 origen,
    Color base
)
{
    static const Color paleta[5] = {
        { 255, 214, 90, 255 }, { 90, 230, 255, 255 }, { 255, 110, 190, 255 },
        { 140, 255, 150, 255 }, { 255, 255, 255, 255 }
    };

    unsigned int& azar = minijuego.semillaEfectos;
    int total = CantidadEfectoVagonetas(3);

    for (int i = 0; i < total; i++)
    {
        Vector3 velocidad = {
            RangoAleatorioVagonetas(azar, -2.6f, 2.6f),
            RangoAleatorioVagonetas(azar, 4.0f, 7.5f),
            RangoAleatorioVagonetas(azar, -2.6f, 2.6f)
        };

        Color color = i % 3 == 0 ? base : paleta[(int)(Aleatorio01Vagonetas(azar) * 5.0f) % 5];
        EmitirParticulaVagonetas(
            minijuego, carro, origen, velocidad,
            RangoAleatorioVagonetas(azar, 1.0f, 1.6f), 0.11f, color, false
        );
    }
}


static void ActualizarParticulasVagonetas(MinijuegoVagonetasDuelo& minijuego, float dt)
{
    for (int k = 0; k < 2; k++)
    {
        for (int i = 0; i < MAX_PARTICULAS_VAGONETAS; i++)
        {
            ParticulaVagonetas& p = minijuego.particulas[k][i];
            if (!p.activa) continue;

            p.vida -= dt;
            if (p.vida <= 0.0f)
            {
                p.activa = false;
                continue;
            }

            if (p.polvo)
            {
                p.velocidad.y -= 0.6f * dt;
                p.velocidad.x *= 1.0f - 2.0f * dt;
                p.velocidad.z *= 1.0f - 2.0f * dt;
            }
            else
            {
                p.velocidad.y -= 13.0f * dt;
            }

            p.posicion.x += p.velocidad.x * dt;
            p.posicion.y += p.velocidad.y * dt;
            p.posicion.z += p.velocidad.z * dt;

            if (!p.polvo && p.posicion.y < 0.14f)
            {
                p.posicion.y = 0.14f;
                p.velocidad.y = -p.velocidad.y * 0.3f;
            }
        }
    }
}


//==================================================
// IA (bots y humanos desconectados)
//==================================================
//
// Un bot ve los peligros a unos 48 m, planifica cuando uno amenaza su riel
// y ejecuta la maniobra en un momento con variacion humana. Con probabilidad
// "error" la maniobra sale tarde, equivocada o se olvida. Nunca se atasca: el
// carro avanza solo, la IA solo decide rieles, saltos y turbo.
//==================================================


static int ElegirCarrilLibreVagonetas(
    const MinijuegoVagonetasDuelo& minijuego,
    const CarroVagonetas& carro,
    int indicePeligro,
    unsigned int& azar
)
{
    const ElementoPistaVagonetas& peligro = minijuego.elementos[indicePeligro];
    int mejor = carro.carrilObjetivo;
    float mejorPuntaje = -1000.0f;

    for (int carril = 0; carril < CARRILES_VAGONETAS; carril++)
    {
        if (CarrilEnMascaraVagonetas(peligro.carriles, carril)) continue;

        float puntaje = -std::fabs((float)(carril - carro.carrilObjetivo)) * 0.9f;
        puntaje += RangoAleatorioVagonetas(azar, 0.0f, 0.35f);

        for (int i = indicePeligro + 1; i < minijuego.cantidadElementos; i++)
        {
            const ElementoPistaVagonetas& e = minijuego.elementos[i];
            if (e.distancia - peligro.distancia > 30.0f) break;
            if (!CarrilEnMascaraVagonetas(e.carriles, carril)) continue;

            if (EsPeligroVagonetas(e.tipo))
            {
                // Un peligro seguido en el mismo riel: mejor otro, salvo que
                // se pueda saltar.
                puntaje -= e.tipo == ELEMENTO_VAGONETAS_ROCA ? 4.0f : 1.2f;
            }
            else if (e.tipo == ELEMENTO_VAGONETAS_PLACA)
            {
                puntaje += 1.4f;
            }
            else
            {
                puntaje += 0.25f;
            }
        }

        if (puntaje > mejorPuntaje)
        {
            mejorPuntaje = puntaje;
            mejor = carril;
        }
    }

    return mejor;
}


static void PlanificarPeligroVagonetas(
    const MinijuegoVagonetasDuelo& minijuego,
    CarroVagonetas& carro,
    int indice,
    unsigned int& azar
)
{
    const ElementoPistaVagonetas& peligro = minijuego.elementos[indice];
    float v = std::max(carro.velocidad, 10.0f);

    carro.elementoPlaneado = indice;
    carro.accionPlaneada = 0;

    bool error = Aleatorio01Vagonetas(azar) < carro.probabilidadError;
    float tipoError = Aleatorio01Vagonetas(azar);

    bool puedeEsquivar = (peligro.carriles & 7) != 7;
    bool esquivar =
        peligro.tipo == ELEMENTO_VAGONETAS_ROCA ||
        (peligro.tipo == ELEMENTO_VAGONETAS_HUECO && puedeEsquivar && Aleatorio01Vagonetas(azar) < 0.40f);

    if (esquivar)
    {
        carro.accionPlaneada = 1;
        carro.carrilDestino = ElegirCarrilLibreVagonetas(minijuego, carro, indice, azar);
        float margen = RangoAleatorioVagonetas(azar, 0.19f, 0.52f);
        carro.distanciaDisparo = 2.2f + v * margen;

        if (error)
        {
            if (tipoError < 0.55f)
            {
                carro.distanciaDisparo = 0.4f + v * 0.02f;   // reaccion tardia
            }
            else if (tipoError < 0.80f)
            {
                carro.accionPlaneada = 0;                    // se equivoca y no se mueve
            }
            else
            {
                carro.carrilDestino = carro.carrilObjetivo;
                carro.accionPlaneada = 0;                    // se distrae
            }
        }
    }
    else
    {
        // Saltar: el momento ideal es ~0.3 s antes del peligro.
        carro.accionPlaneada = 2;
        float ruido = (Aleatorio01Vagonetas(azar) - 0.5f) * 0.16f * (1.4f - carro.habilidad);
        carro.distanciaDisparo = v * (0.30f + ruido);

        if (peligro.tipo == ELEMENTO_VAGONETAS_HUECO)
        {
            carro.distanciaDisparo = v * (0.31f + ruido);
        }

        if (error)
        {
            if (tipoError < 0.5f) carro.distanciaDisparo = v * 0.10f;       // tarde
            else if (tipoError < 0.8f) carro.distanciaDisparo = v * 0.62f;  // temprano
            else carro.accionPlaneada = 0;                                  // olvido
        }
    }
}


static EntradaVagonetas DecidirIAVagonetas(
    MinijuegoVagonetasDuelo& minijuego,
    CarroVagonetas& carro,
    float dt
)
{
    EntradaVagonetas entrada;
    unsigned int& azar = minijuego.semillaIA;
    float v = std::max(carro.velocidad, 10.0f);

    // Pulsaciones encoladas (cambios de mas de un riel).
    if (carro.pulsacionesPendientes > 0)
    {
        carro.esperaPulsacion -= dt;
        if (carro.esperaPulsacion <= 0.0f)
        {
            if (carro.direccionPulsacion < 0) entrada.izquierda = true;
            else entrada.derecha = true;

            carro.pulsacionesPendientes--;
            carro.esperaPulsacion = RangoAleatorioVagonetas(azar, 0.06f, 0.11f);
        }
    }

    // Peligro mas cercano que amenaza el riel al que va el carro.
    int amenaza = -1;
    bool hayPeligroCerca = false;

    for (int i = carro.cursor; i < minijuego.cantidadElementos; i++)
    {
        const ElementoPistaVagonetas& e = minijuego.elementos[i];
        float distancia = e.distancia - carro.distancia;
        if (distancia > 48.0f) break;
        if (distancia < -1.0f || !EsPeligroVagonetas(e.tipo)) continue;

        if (CarrilEnMascaraVagonetas(e.carriles, carro.carrilObjetivo))
        {
            if (distancia < 26.0f) hayPeligroCerca = true;
            if (amenaza < 0) amenaza = i;
        }
    }

    if (amenaza >= 0 && carro.elementoPlaneado != amenaza)
    {
        PlanificarPeligroVagonetas(minijuego, carro, amenaza, azar);
    }

    if (amenaza >= 0 && carro.accionPlaneada != 0 && carro.elementoPlaneado == amenaza)
    {
        float distancia = minijuego.elementos[amenaza].distancia - carro.distancia;

        if (distancia <= carro.distanciaDisparo)
        {
            if (carro.accionPlaneada == 2)
            {
                if (carro.altura <= 0.001f) entrada.saltar = true;
            }
            else
            {
                int pasos = carro.carrilDestino - carro.carrilObjetivo;
                if (pasos != 0)
                {
                    if (pasos < 0) entrada.izquierda = true;
                    else entrada.derecha = true;

                    carro.direccionPulsacion = pasos < 0 ? -1 : 1;
                    carro.pulsacionesPendientes = std::abs(pasos) - 1;
                    carro.esperaPulsacion = RangoAleatorioVagonetas(azar, 0.06f, 0.11f);
                }
            }

            carro.accionPlaneada = 0;
        }
    }

    // Sin peligro a la vista: elegir riel por monedas y placas.
    carro.tiempoDecision -= dt;

    if (amenaza < 0 && carro.pulsacionesPendientes == 0 && carro.tiempoDecision <= 0.0f)
    {
        carro.tiempoDecision = RangoAleatorioVagonetas(azar, 0.35f, 0.6f);

        float puntaje[CARRILES_VAGONETAS] = { 0.0f, 0.0f, 0.0f };
        puntaje[carro.carrilObjetivo] += 0.9f;

        for (int i = carro.cursor; i < minijuego.cantidadElementos; i++)
        {
            const ElementoPistaVagonetas& e = minijuego.elementos[i];
            float distancia = e.distancia - carro.distancia;
            if (distancia > 40.0f) break;
            if (distancia < 6.0f) continue;

            for (int carril = 0; carril < CARRILES_VAGONETAS; carril++)
            {
                if (!CarrilEnMascaraVagonetas(e.carriles, carril)) continue;

                if (EsPeligroVagonetas(e.tipo))
                {
                    puntaje[carril] -= e.tipo == ELEMENTO_VAGONETAS_ROCA ? 12.0f : 2.5f;
                }
                else if (e.tipo == ELEMENTO_VAGONETAS_PLACA)
                {
                    puntaje[carril] += 2.2f;
                }
                else if (e.tipo == ELEMENTO_VAGONETAS_MONEDA)
                {
                    puntaje[carril] += 0.45f;
                }
            }
        }

        int mejor = carro.carrilObjetivo;
        for (int carril = 0; carril < CARRILES_VAGONETAS; carril++)
        {
            if (puntaje[carril] > puntaje[mejor] + 0.4f) mejor = carril;
        }

        // Los pasos de mas de un riel solo se dan de a uno por decision.
        if (mejor != carro.carrilObjetivo && Aleatorio01Vagonetas(azar) < 0.5f + 0.45f * carro.habilidad)
        {
            if (mejor < carro.carrilObjetivo) entrada.izquierda = true;
            else entrada.derecha = true;
        }
    }

    // Turbo: cuando hay energia y el camino inmediato esta libre.
    carro.tiempoTurbo -= dt;

    if (
        carro.tiempoTurbo <= 0.0f &&
        carro.energia >= COSTO_TURBO_VAGONETAS + carro.reservaTurbo &&
        carro.recargaTurbo <= 0.0f &&
        carro.turbo <= 0.05f &&
        carro.aturdido <= 0.2f
    )
    {
        bool libre = !hayPeligroCerca;
        if (!libre && Aleatorio01Vagonetas(azar) < carro.probabilidadError * 0.8f)
        {
            libre = true;   // se anima aunque venga un peligro
        }

        if (libre)
        {
            entrada.turbo = true;
            carro.tiempoTurbo = RangoAleatorioVagonetas(azar, 0.05f, 0.45f);
        }
        else
        {
            carro.tiempoTurbo = 0.12f;
        }
    }

    (void)v;
    return entrada;
}


//==================================================
// FISICA DE UN CARRO
//==================================================


static void AplicarChoqueVagonetas(
    MinijuegoVagonetasDuelo& minijuego,
    int indiceCarro,
    bool hueco
)
{
    CarroVagonetas& carro = minijuego.carros[indiceCarro];
    bool conTurbo = carro.turbo > 0.0f;

    carro.choques++;
    carro.velocidad *= conTurbo ? 0.30f : 0.45f;
    carro.turbo = 0.0f;
    carro.impulsoPlaca = 0.0f;
    carro.aturdido = conTurbo ? 1.1f : 0.8f;
    carro.inmune = 0.75f;
    carro.energia = std::max(0.0f, carro.energia - (conTurbo ? 0.25f : 0.10f));
    carro.bonoMonedas = 0.0f;
    carro.sacudida = 1.0f;
    carro.destelloChoque = 1.0f;
    PonerAvisoVagonetas(carro, AVISO_VAGONETAS_CHOQUE);

    if (hueco)
    {
        carro.hundido = 0.5f;
        carro.velocidadVertical = 3.2f;
        carro.altura = 0.01f;
    }

    Vector3 origen = PosicionPistaVagonetas(carro.distancia, carro.lateral);
    origen.y = 0.6f;
    ChispasVagonetas(minijuego, indiceCarro, origen, 18, 4.2f, false);
    PolvoVagonetas(minijuego, indiceCarro, origen, 6, 1.6f);
    SonarVagonetas(minijuego, carro, SONIDO_IMPACTO, false);
}


static bool SolapaCarrilVagonetas(const CarroVagonetas& carro, int mascara, float umbral)
{
    for (int carril = 0; carril < CARRILES_VAGONETAS; carril++)
    {
        if (!CarrilEnMascaraVagonetas(mascara, carril)) continue;
        if (std::fabs(carro.lateral - LateralCarrilVagonetas(carril)) < umbral) return true;
    }
    return false;
}


static void ProcesarElementosVagonetas(MinijuegoVagonetasDuelo& minijuego, int indiceCarro)
{
    CarroVagonetas& carro = minijuego.carros[indiceCarro];

    while (
        carro.cursor < minijuego.cantidadElementos &&
        minijuego.elementos[carro.cursor].distancia < carro.distancia - 4.0f
    )
    {
        carro.cursor++;
    }

    for (int i = carro.cursor; i < minijuego.cantidadElementos; i++)
    {
        const ElementoPistaVagonetas& e = minijuego.elementos[i];
        if (e.distancia - 3.0f > carro.distancia) break;
        if (carro.tomado[i]) continue;

        float delta = std::fabs(carro.distancia - e.distancia);

        switch (e.tipo)
        {
        case ELEMENTO_VAGONETAS_ROCA:
            if (
                delta < 1.45f && carro.inmune <= 0.0f && carro.altura < 1.3f &&
                SolapaCarrilVagonetas(carro, e.carriles, 0.9f)
            )
            {
                carro.tomado[i] = true;
                AplicarChoqueVagonetas(minijuego, indiceCarro, false);
            }
            break;

        case ELEMENTO_VAGONETAS_VIGA:
            if (
                delta < 1.05f && carro.inmune <= 0.0f && carro.altura < 0.5f &&
                SolapaCarrilVagonetas(carro, e.carriles, 1.6f)
            )
            {
                carro.tomado[i] = true;
                AplicarChoqueVagonetas(minijuego, indiceCarro, false);
            }
            break;

        case ELEMENTO_VAGONETAS_HUECO:
            if (
                delta < e.largo * 0.5f && carro.inmune <= 0.0f && carro.altura < 0.26f &&
                SolapaCarrilVagonetas(carro, e.carriles, 0.85f)
            )
            {
                carro.tomado[i] = true;
                AplicarChoqueVagonetas(minijuego, indiceCarro, true);
            }
            break;

        case ELEMENTO_VAGONETAS_MONEDA:
        case ELEMENTO_VAGONETAS_MONEDA_ALTA:
        {
            bool alta = e.tipo == ELEMENTO_VAGONETAS_MONEDA_ALTA;
            bool alcanza = alta ? carro.altura > 0.28f : carro.altura < 0.6f;

            if (delta < 1.15f && alcanza && SolapaCarrilVagonetas(carro, e.carriles, 0.9f))
            {
                carro.tomado[i] = true;
                carro.monedas++;
                carro.bonoMonedas = std::min(3.0f, carro.bonoMonedas + 0.6f);
                carro.energia = std::min(1.0f, carro.energia + 0.035f);

                Vector3 origen = PosicionPistaVagonetas(e.distancia, LateralCarrilVagonetas(
                    e.carriles & 1 ? 0 : e.carriles & 2 ? 1 : 2));
                origen.y = alta ? 1.5f : 1.0f;
                ChispasVagonetas(minijuego, indiceCarro, origen, 5, 1.8f, false);
                SonarVagonetas(minijuego, carro, SONIDO_RECOGER_OBJETO, true);
            }
            break;
        }

        case ELEMENTO_VAGONETAS_PLACA:
            if (delta < 1.7f && SolapaCarrilVagonetas(carro, e.carriles, 0.9f))
            {
                carro.tomado[i] = true;
                carro.impulsoPlaca = DURACION_PLACA_VAGONETAS;
                PonerAvisoVagonetas(carro, AVISO_VAGONETAS_IMPULSO);

                Vector3 origen = PosicionPistaVagonetas(carro.distancia, carro.lateral);
                origen.y = 0.5f;
                ChispasVagonetas(minijuego, indiceCarro, origen, 12, 3.4f, true);
                SonarVagonetas(minijuego, carro, SONIDO_ACIERTO, false);
            }
            break;
        }
    }
}


static void AvanzarCarroVagonetas(
    MinijuegoVagonetasDuelo& minijuego,
    int indiceCarro,
    float dt,
    const EntradaVagonetas& entrada,
    float distanciaRival,
    bool poscarrera
)
{
    CarroVagonetas& carro = minijuego.carros[indiceCarro];

    carro.turbo = std::max(0.0f, carro.turbo - dt);
    carro.recargaTurbo = std::max(0.0f, carro.recargaTurbo - dt);
    carro.impulsoPlaca = std::max(0.0f, carro.impulsoPlaca - dt);
    carro.aturdido = std::max(0.0f, carro.aturdido - dt);
    carro.inmune = std::max(0.0f, carro.inmune - dt);
    carro.hundido = std::max(0.0f, carro.hundido - dt);
    carro.sacudida = std::max(0.0f, carro.sacudida - dt * 2.4f);
    carro.destelloChoque = std::max(0.0f, carro.destelloChoque - dt * 3.0f);
    carro.bonoMonedas = std::max(0.0f, carro.bonoMonedas - 1.2f * dt);

    for (int i = 0; i < MAX_AVISOS_VAGONETAS; i++)
    {
        carro.avisos[i].tiempo = std::max(0.0f, carro.avisos[i].tiempo - dt);
    }

    if (poscarrera)
    {
        carro.turbo = 0.0f;
        carro.impulsoPlaca = 0.0f;
    }

    // Entrada: rieles, salto y turbo.
    if (!poscarrera)
    {
        int anterior = carro.carrilObjetivo;
        if (entrada.izquierda) carro.carrilObjetivo = std::max(0, carro.carrilObjetivo - 1);
        if (entrada.derecha) carro.carrilObjetivo = std::min(CARRILES_VAGONETAS - 1, carro.carrilObjetivo + 1);

        if (anterior != carro.carrilObjetivo && carro.altura < 0.05f)
        {
            Vector3 origen = PosicionPistaVagonetas(carro.distancia - 0.7f, carro.lateral);
            origen.y = 0.3f;
            PolvoVagonetas(minijuego, indiceCarro, origen, 2, 0.8f);
            ChispasVagonetas(minijuego, indiceCarro, origen, 4, 2.0f, false);
        }

        if (entrada.saltar && carro.altura <= 0.001f && carro.velocidadVertical == 0.0f)
        {
            carro.velocidadVertical = IMPULSO_SALTO_VAGONETAS;
            carro.tiempoSalto = 0.0f;
            SonarVagonetas(minijuego, carro, SONIDO_SALTO, true);
        }

        if (
            entrada.turbo &&
            carro.energia >= COSTO_TURBO_VAGONETAS &&
            carro.recargaTurbo <= 0.0f
        )
        {
            carro.energia -= COSTO_TURBO_VAGONETAS;
            carro.turbo = DURACION_TURBO_VAGONETAS;
            carro.recargaTurbo = ESPERA_ENTRE_TURBOS_VAGONETAS;
            PonerAvisoVagonetas(carro, AVISO_VAGONETAS_TURBO);

            Vector3 origen = PosicionPistaVagonetas(carro.distancia - 1.0f, carro.lateral);
            origen.y = 0.8f;
            ChispasVagonetas(minijuego, indiceCarro, origen, 10, 3.0f, true);
            SonarVagonetas(minijuego, carro, SONIDO_DISPARO, false);
        }
    }

    // Lateral: aproximacion exponencial al riel elegido.
    float objetivoLateral = LateralCarrilVagonetas(carro.carrilObjetivo);
    float diferencia = objetivoLateral - carro.lateral;
    carro.lateral += diferencia * std::min(1.0f, 15.0f * dt);
    carro.inclinacion += (Clamp(diferencia * 9.0f, -13.0f, 13.0f) - carro.inclinacion) * std::min(1.0f, 11.0f * dt);
    carro.guinada += (Clamp(diferencia * 6.0f, -9.0f, 9.0f) - carro.guinada) * std::min(1.0f, 9.0f * dt);

    // Vertical.
    if (carro.altura > 0.0f || carro.velocidadVertical != 0.0f)
    {
        carro.tiempoSalto += dt;
        carro.velocidadVertical -= GRAVEDAD_VAGONETAS * dt;
        carro.altura += carro.velocidadVertical * dt;

        if (carro.altura <= 0.0f)
        {
            if (carro.velocidadVertical < -2.5f)
            {
                Vector3 origen = PosicionPistaVagonetas(carro.distancia, carro.lateral);
                origen.y = 0.3f;
                PolvoVagonetas(minijuego, indiceCarro, origen, 3, 1.2f);
                ChispasVagonetas(minijuego, indiceCarro, origen, 6, 2.2f, false);
                SonarVagonetas(minijuego, carro, SONIDO_ATERRIZAJE, true);
            }

            carro.altura = 0.0f;
            carro.velocidadVertical = 0.0f;
        }
    }

    // Velocidad.
    float goma = 0.0f;
    if (!poscarrera && distanciaRival > carro.distancia)
    {
        goma = AYUDA_GOMA_VAGONETAS * Clamp(
            (distanciaRival - carro.distancia) / DISTANCIA_GOMA_VAGONETAS, 0.0f, 1.0f
        );
    }

    float bonoTurbo = 0.0f;
    if (carro.turbo > 0.0f)
    {
        bonoTurbo = BONO_TURBO_VAGONETAS * std::min(1.0f, carro.turbo / 0.2f);
    }

    float bonoPlaca = 0.0f;
    if (carro.impulsoPlaca > 0.0f)
    {
        bonoPlaca = BONO_PLACA_VAGONETAS * std::min(1.0f, carro.impulsoPlaca / 0.2f);
    }

    float factorAturdido = 1.0f;
    if (carro.aturdido > 0.0f)
    {
        factorAturdido = carro.aturdido > 0.25f
            ? 0.60f
            : 0.60f + 0.40f * (1.0f - carro.aturdido / 0.25f);
    }

    float objetivo = VELOCIDAD_CRUCERO_VAGONETAS * (1.0f + goma) * factorAturdido
        + std::max(bonoTurbo, bonoPlaca) + carro.bonoMonedas;

    if (poscarrera)
    {
        objetivo = carro.distancia > LARGO_PISTA_VAGONETAS + 9.0f
            ? 0.0f
            : VELOCIDAD_CRUCERO_VAGONETAS * 0.8f;
    }

    float cambio = Clamp(
        objetivo - carro.velocidad,
        -FRENADO_VAGONETAS * dt,
        ACELERACION_VAGONETAS * dt
    );
    carro.velocidad += cambio;

    float anterior = carro.distancia;
    carro.distancia += carro.velocidad * dt;
    carro.giroRuedas += carro.velocidad * dt / 0.18f;

    carro.energia = std::min(1.0f, carro.energia + RECARGA_ENERGIA_VAGONETAS * dt);

    if (!poscarrera)
    {
        ProcesarElementosVagonetas(minijuego, indiceCarro);
    }

    // Cruce de meta con tiempo exacto dentro del cuadro.
    if (!carro.llego && carro.distancia >= LARGO_PISTA_VAGONETAS)
    {
        float fraccion = carro.distancia > anterior
            ? (LARGO_PISTA_VAGONETAS - anterior) / (carro.distancia - anterior)
            : 0.0f;

        carro.llego = true;
        carro.tiempoLlegada = minijuego.tiempoCarrera + Clamp(fraccion, 0.0f, 1.0f) * dt;
    }

    // Chispas de las ruedas y llama del turbo.
    unsigned int& azar = minijuego.semillaEfectos;
    float ritmo = carro.velocidad * 0.55f + (carro.turbo > 0.0f ? 14.0f : 0.0f);
    carro.tiempoChispas += dt * ritmo;

    while (carro.tiempoChispas >= 1.0f && carro.altura < 0.05f)
    {
        carro.tiempoChispas -= 1.0f;

        float lado = Aleatorio01Vagonetas(azar) < 0.5f ? -1.0f : 1.0f;
        Vector3 origen = PosicionPistaVagonetas(
            carro.distancia - 0.8f,
            carro.lateral + lado * SEPARACION_RUEDAS_VAGONETAS
        );
        origen.y = Y_RIEL_VAGONETAS;

        if (Aleatorio01Vagonetas(azar) < FactorCalidadGrafica(CalidadParticulas()))
        {
            EmitirParticulaVagonetas(
                minijuego, indiceCarro, origen,
                {
                    RangoAleatorioVagonetas(azar, -0.8f, 0.8f),
                    RangoAleatorioVagonetas(azar, 1.2f, 3.2f),
                    RangoAleatorioVagonetas(azar, 0.5f, 2.2f)
                },
                RangoAleatorioVagonetas(azar, 0.2f, 0.42f),
                RangoAleatorioVagonetas(azar, 0.04f, 0.08f),
                Color{ 255, (unsigned char)(150 + 90 * Aleatorio01Vagonetas(azar)), 60, 255 },
                false
            );
        }
    }

    if (carro.turbo > 0.0f || carro.impulsoPlaca > 0.0f)
    {
        carro.tiempoPolvo += dt * 40.0f;
        while (carro.tiempoPolvo >= 1.0f)
        {
            carro.tiempoPolvo -= 1.0f;
            Vector3 origen = PosicionPistaVagonetas(
                carro.distancia - 1.35f,
                carro.lateral + RangoAleatorioVagonetas(azar, -0.25f, 0.25f)
            );
            origen.y = carro.altura + 0.85f;

            if (Aleatorio01Vagonetas(azar) < FactorCalidadGrafica(CalidadParticulas()))
            {
                bool placa = carro.turbo <= 0.0f;
                EmitirParticulaVagonetas(
                    minijuego, indiceCarro, origen,
                    {
                        RangoAleatorioVagonetas(azar, -0.5f, 0.5f),
                        RangoAleatorioVagonetas(azar, -0.2f, 0.8f),
                        RangoAleatorioVagonetas(azar, 2.0f, 5.0f)
                    },
                    RangoAleatorioVagonetas(azar, 0.22f, 0.4f),
                    RangoAleatorioVagonetas(azar, 0.07f, 0.12f),
                    placa
                        ? Color{ 120, 235, 255, 255 }
                        : Color{ 255, (unsigned char)(120 + 100 * Aleatorio01Vagonetas(azar)), 40, 255 },
                    false
                );
            }
        }
    }
}


//==================================================
// RESULTADO
//==================================================


static int ElegirGanadorVagonetas(const MinijuegoVagonetasDuelo& minijuego, bool porLlegada)
{
    const CarroVagonetas& a = minijuego.carros[0];
    const CarroVagonetas& b = minijuego.carros[1];

    if (porLlegada)
    {
        if (a.llego && !b.llego) return 0;
        if (b.llego && !a.llego) return 1;

        if (a.llego && b.llego && std::fabs(a.tiempoLlegada - b.tiempoLlegada) > 1.0e-6f)
        {
            return a.tiempoLlegada < b.tiempoLlegada ? 0 : 1;
        }
    }
    else if (std::fabs(a.distancia - b.distancia) > 1.0e-3f)
    {
        return a.distancia > b.distancia ? 0 : 1;
    }

    // Desempate determinista: menos choques, mas monedas, primer participante.
    if (a.choques != b.choques) return a.choques < b.choques ? 0 : 1;
    if (a.monedas != b.monedas) return a.monedas > b.monedas ? 0 : 1;
    return 0;
}


static void FinalizarCarreraVagonetas(MinijuegoVagonetasDuelo& minijuego, bool porLlegada)
{
    if (minijuego.fase == FASE_VAGONETAS_TERMINADO) return;

    int ganador = ElegirGanadorVagonetas(minijuego, porLlegada);
    minijuego.ganador = ganador;
    minijuego.fase = FASE_VAGONETAS_TERMINADO;
    minijuego.tiempoFinal = 0.0f;
    minijuego.partidaValida = true;

    ResultadoMinijuego& resultado = minijuego.resultado;
    resultado.estado = RESULTADO_MINIJUEGO_FINALIZADO;
    resultado.desenlace = DESENLACE_CON_GANADOR;

    for (int k = 0; k < 2; k++)
    {
        ResultadoParticipante& r = resultado.participantes[minijuego.carros[k].participante];
        r.posicionFinal = k == ganador ? 1 : 2;
        r.puntuacionMinijuego = (int)std::min(LARGO_PISTA_VAGONETAS, minijuego.carros[k].distancia);
    }

    ReproducirSonidoMinijuego(minijuego.audio, SONIDO_RESULTADO);
}


//==================================================
// ESTADO
//==================================================


void MinijuegoVagonetasDuelo::Inicializar()
{
    resultado = {};
    cantidadCarros = 0;
    cantidadElementos = 0;
    ganador = -1;
    lider = -1;
    partidaValida = false;
    fase = FASE_VAGONETAS_PREPARACION;
    tiempoPreparacion = DURACION_PREPARACION_VAGONETAS;
    tiempoRestante = TOPE_TIEMPO_VAGONETAS;
    tiempoCarrera = 0.0f;
    tiempoFinal = 0.0f;
    tiempoAnimacion = 0.0f;

    for (int k = 0; k < 2; k++)
    {
        carros[k] = CarroVagonetas{};
        for (int i = 0; i < MAX_PARTICULAS_VAGONETAS; i++) particulas[k][i] = ParticulaVagonetas{};
    }
}


void MinijuegoVagonetasDuelo::Reiniciar(
    JugadorPrueba jugadores[],
    Participante participantes[],
    int cantidadMaxima
)
{
    (void)jugadores;

    Inicializar();
    InicializarResultadoMinijuego(resultado, participantes, FORMATO_MINIJUEGO_INDIVIDUAL);

    // La semilla sale del azar global: cada duelo es una pista distinta, pero
    // los dos carros comparten la misma.
    semillaPista = (unsigned int)GetRandomValue(1, 0x3FFFFFFF);
    semillaEfectos = semillaPista ^ 0xA5A5A5A5u;
    semillaIA = semillaPista * 2654435761u + 12345u;

    int limite = cantidadMaxima < MAX_PARTICIPANTES ? cantidadMaxima : MAX_PARTICIPANTES;
    int indices[2] = { -1, -1 };
    int cantidad = 0;

    for (int i = 0; i < limite; i++)
    {
        if (!participantes[i].activo) continue;

        if (cantidad < 2) indices[cantidad++] = i;
        else resultado.participantes[i].participo = false;
    }

    if (cantidad < 2)
    {
        resultado.estado = RESULTADO_MINIJUEGO_CANCELADO;
        fase = FASE_VAGONETAS_TERMINADO;
        return;
    }

    for (int i = limite; i < MAX_PARTICIPANTES; i++) resultado.participantes[i].participo = false;
    resultado.cantidadParticipantes = 2;

    GenerarPistaVagonetas(*this);
    cantidadCarros = 2;

    for (int k = 0; k < 2; k++)
    {
        CarroVagonetas& carro = carros[k];
        carro.participante = indices[k];
        carro.controlIA = participantes[indices[k]].esBot || !participantes[indices[k]].conectado;
        carro.carrilObjetivo = 1;
        carro.lateral = 0.0f;
        carro.energia = 1.0f;
        carro.distancia = 0.0f;

        // Habilidad humana de bot: variada pero con el mismo rango para ambos.
        carro.habilidad = RangoAleatorioVagonetas(semillaIA, 0.72f, 1.0f);
        carro.probabilidadError = 0.03f + (1.0f - carro.habilidad) * 0.30f;
        carro.reservaTurbo = RangoAleatorioVagonetas(semillaIA, 0.0f, 0.25f);
        carro.tiempoDecision = RangoAleatorioVagonetas(semillaIA, 0.4f, 1.0f);
        carro.tiempoTurbo = RangoAleatorioVagonetas(semillaIA, 0.2f, 0.9f);
    }

    SeleccionarTemaVisualMinijuego(TEMA_VISUAL_NINGUNO);
}


void MinijuegoVagonetasDuelo::Actualizar(
    float deltaTime,
    JugadorPrueba jugadores[],
    int cantidadMaxima,
    Participante participantes[]
)
{
    (void)jugadores;
    (void)cantidadMaxima;

    tiempoAnimacion += deltaTime;

    if (resultado.estado == RESULTADO_MINIJUEGO_CANCELADO || cantidadCarros < 2)
    {
        return;
    }

    ActualizarParticulasVagonetas(*this, deltaTime);

    if (fase == FASE_VAGONETAS_PREPARACION)
    {
        float antes = tiempoPreparacion;
        tiempoPreparacion -= deltaTime;
        ActualizarAudioCuentaRegresiva(audio, antes, tiempoPreparacion);

        // Humanos que se desconectan durante la cuenta pasan a bot.
        for (int k = 0; k < 2; k++)
        {
            const Participante& p = participantes[carros[k].participante];
            carros[k].controlIA = p.esBot || !p.conectado;
        }

        if (tiempoPreparacion <= 0.0f)
        {
            tiempoPreparacion = 0.0f;
            fase = FASE_VAGONETAS_CARRERA;
        }

        return;
    }

    if (fase == FASE_VAGONETAS_CARRERA)
    {
        float antes = tiempoRestante;
        tiempoRestante -= deltaTime;
        ActualizarAudioAlertaTiempo(audio, antes, tiempoRestante);

        EntradaVagonetas entradas[2];

        for (int k = 0; k < 2; k++)
        {
            CarroVagonetas& carro = carros[k];
            const Participante& p = participantes[carro.participante];
            carro.controlIA = p.esBot || !p.conectado;

            if (carro.controlIA)
            {
                entradas[k] = DecidirIAVagonetas(*this, carro, deltaTime);
                continue;
            }

            InputMinijuegoParticipante lectura = LeerInputMinijuegoParticipante(p);
            entradas[k].izquierda = lectura.izquierda && !carro.previoIzquierda;
            entradas[k].derecha = lectura.derecha && !carro.previoDerecha;
            entradas[k].saltar = lectura.saltar && !carro.previoSaltar;
            entradas[k].turbo = lectura.golpear && !carro.previoGolpear;
            carro.previoIzquierda = lectura.izquierda;
            carro.previoDerecha = lectura.derecha;
            carro.previoSaltar = lectura.saltar;
            carro.previoGolpear = lectura.golpear;
        }

        float distanciasAntes[2] = { carros[0].distancia, carros[1].distancia };

        for (int k = 0; k < 2; k++)
        {
            AvanzarCarroVagonetas(
                *this, k, deltaTime, entradas[k], distanciasAntes[1 - k], false
            );
        }

        tiempoCarrera += deltaTime;

        // Cambio de liderazgo con aviso para quien pasa adelante.
        float diferencia = carros[0].distancia - carros[1].distancia;
        int nuevoLider = diferencia > 2.0f ? 0 : diferencia < -2.0f ? 1 : lider;

        if (nuevoLider != lider)
        {
            if (lider >= 0 && nuevoLider >= 0 && tiempoCarrera > 3.0f)
            {
                PonerAvisoVagonetas(carros[nuevoLider], AVISO_VAGONETAS_ADELANTE);
            }
            lider = nuevoLider;
        }

        if (carros[0].llego || carros[1].llego)
        {
            FinalizarCarreraVagonetas(*this, true);
        }
        else if (tiempoRestante <= 0.0f)
        {
            tiempoRestante = 0.0f;
            FinalizarCarreraVagonetas(*this, false);
        }

        return;
    }

    // Terminado: los carros siguen rodando y frenan; el ganador festeja.
    tiempoFinal += deltaTime;

    for (int k = 0; k < 2; k++)
    {
        CarroVagonetas& carro = carros[k];

        if (tiempoFinal < DURACION_POSCARRERA_VAGONETAS)
        {
            AvanzarCarroVagonetas(*this, k, deltaTime, EntradaVagonetas{}, 0.0f, true);
        }
        else
        {
            carro.velocidad = 0.0f;
        }

        if (k == ganador && tiempoFinal < 2.5f)
        {
            Vector3 origen = PosicionPistaVagonetas(carro.distancia, carro.lateral);
            origen.y = 1.6f;
            ConfetiVagonetas(*this, k, origen, participantes[carro.participante].color);
        }
    }
}


const ResultadoMinijuego& MinijuegoVagonetasDuelo::ObtenerResultado() const
{
    return resultado;
}


//==================================================
// DIBUJO: MUNDO
//==================================================


static const Color NIEBLA_VAGONETAS = { 18, 9, 26, 255 };


struct MarcoVagonetas
{
    Vector3 p{};
    float cosR = 1.0f;
    float sinR = 0.0f;
    float guinadaGrados = 0.0f;   // para rlRotatef(.., 0, 1, 0)
    float rumbo = 0.0f;
};


static MarcoVagonetas MarcoPistaVagonetas(float s)
{
    MarcoVagonetas m;
    m.rumbo = RumboPistaVagonetas(s);
    m.cosR = std::cos(m.rumbo);
    m.sinR = std::sin(m.rumbo);
    m.p = { CurvaXVagonetas(s), 0.0f, -s };
    m.guinadaGrados = -m.rumbo * RAD2DEG;
    return m;
}


static float HashVagonetas(int a, int b = 0)
{
    unsigned int h = (unsigned int)(a * 73856093) ^ (unsigned int)(b * 19349663) ^ 0x9E3779B9u;
    h ^= h >> 15; h *= 2246822519u; h ^= h >> 13; h *= 3266489917u; h ^= h >> 16;
    return (float)(h & 0xFFFFFFu) / 16777216.0f;
}


// Aleja el color con la distancia y lo calienta cerca de la linterna del
// carro. emisivo (0..1): lo que brilla casi no se apaga.
static Color TinteVagonetas(Color c, float distancia, float emisivo)
{
    float t = Clamp((distancia - 4.0f) / 85.0f, 0.0f, 1.0f);
    float luz = 1.0f - 0.5f * t * (1.0f - emisivo);
    if (distancia < 16.0f) luz += 0.22f * (1.0f - distancia / 16.0f);

    float mezcla = t * 0.8f * (1.0f - emisivo);
    float r = c.r * luz, g = c.g * luz, b = c.b * luz;
    r = r + (NIEBLA_VAGONETAS.r - r) * mezcla;
    g = g + (NIEBLA_VAGONETAS.g - g) * mezcla;
    b = b + (NIEBLA_VAGONETAS.b - b) * mezcla;

    return {
        (unsigned char)Clamp(r, 0.0f, 255.0f),
        (unsigned char)Clamp(g, 0.0f, 255.0f),
        (unsigned char)Clamp(b, 0.0f, 255.0f),
        c.a
    };
}


// Cubo alineado con la pista: lat a la derecha, y arriba, desp hacia la meta.
static void CuboPistaVagonetas(
    const MarcoVagonetas& m, float lat, float y, float desp,
    float ancho, float alto, float largo, Color color, float giroExtra = 0.0f
)
{
    rlPushMatrix();
    rlTranslatef(
        m.p.x + lat * m.cosR + desp * m.sinR,
        y,
        m.p.z + lat * m.sinR - desp * m.cosR
    );
    rlRotatef(m.guinadaGrados + giroExtra, 0.0f, 1.0f, 0.0f);
    (DrawCube)({ 0.0f, 0.0f, 0.0f }, ancho, alto, largo, color);
    rlPopMatrix();
}


static Vector3 PuntoPistaVagonetas(const MarcoVagonetas& m, float lat, float y, float desp)
{
    return {
        m.p.x + lat * m.cosR + desp * m.sinR,
        y,
        m.p.z + lat * m.sinR - desp * m.cosR
    };
}


struct VistaVagonetas
{
    const MinijuegoVagonetasDuelo* minijuego = nullptr;
    int k = 0;
    float distanciaCamara = 0.0f;
    float tiempo = 0.0f;
};


static Color LavaVagonetas(float tiempo, float fase)
{
    float pulso = 0.5f + 0.5f * std::sin(tiempo * 2.2f + fase);
    return { 255, (unsigned char)(88 + 80 * pulso), (unsigned char)(18 + 30 * pulso), 255 };
}


static void DibujarSegmentoVagonetas(const VistaVagonetas& v, int seg)
{
    float s = seg * 3.0f + 1.5f;
    float d = std::max(0.0f, s - v.distanciaCamara);
    MarcoVagonetas m = MarcoPistaVagonetas(s);
    bool lejos = d > 46.0f;
    bool decorar = CalidadDecoracion() != CALIDAD_BAJA;
    float h0 = HashVagonetas(seg, 1);
    float h1 = HashVagonetas(seg, 2);
    float h2 = HashVagonetas(seg, 3);

    // Paredes y techo.
    for (int lado = -1; lado <= 1; lado += 2)
    {
        float hh = HashVagonetas(seg, 10 + lado);
        float alto = 5.5f + hh * 4.0f;
        float lat = lado * (10.6f + HashVagonetas(seg, 20 + lado) * 1.8f);
        Color roca = hh < 0.5f ? Color{ 58, 42, 58, 255 } : Color{ 74, 50, 62, 255 };

        CuboPistaVagonetas(m, lat + lado * 4.5f, alto * 0.5f + 1.5f, 0.0f, 6.0f, alto + 3.0f, 3.1f, TinteVagonetas({ 40, 28, 44, 255 }, d + 4.0f, 0.0f));
        CuboPistaVagonetas(m, lat, alto * 0.5f - 0.3f, 0.0f, 3.2f, alto, 3.1f, TinteVagonetas(roca, d, 0.0f));
        CuboPistaVagonetas(m, lat - lado * 1.9f, 0.9f + hh, 0.0f, 1.4f, 2.0f + hh * 1.5f, 3.1f, TinteVagonetas({ 82, 58, 68, 255 }, d, 0.0f));

        if (decorar && !lejos && HashVagonetas(seg, 30 + lado) < 0.45f)
        {
            float cx = lat - lado * 2.9f;
            Color cristal = HashVagonetas(seg, 40 + lado) < 0.5f ? Color{ 90, 235, 255, 255 } : Color{ 235, 100, 255, 255 };
            float pulso = 0.8f + 0.2f * std::sin(v.tiempo * 3.0f + seg);

            for (int i = 0; i < 3; i++)
            {
                float altoC = (0.9f + HashVagonetas(seg, 50 + i + lado) * 1.5f) * (i == 1 ? 1.4f : 1.0f);
                CuboPistaVagonetas(
                    m, cx - lado * i * 0.45f, altoC * 0.5f + 0.1f, (i - 1) * 0.7f,
                    0.38f, altoC, 0.38f, TinteVagonetas(cristal, d, 0.9f), 20.0f * i
                );
            }

            Color halo = cristal;
            halo.a = (unsigned char)(34 * pulso);
            CuboPistaVagonetas(m, cx - lado * 0.45f, 1.0f, 0.0f, 2.6f, 2.2f, 2.6f, halo);
        }
    }

    CuboPistaVagonetas(m, 0.0f, 10.2f, 0.0f, 30.0f, 2.4f, 3.1f, TinteVagonetas({ 36, 26, 42, 255 }, d + 6.0f, 0.0f));

    // Estalactitas.
    if (decorar && !lejos && h0 < 0.55f)
    {
        for (int i = 0; i < 2; i++)
        {
            float lat = (HashVagonetas(seg, 60 + i) - 0.5f) * 14.0f;
            float largo = 1.4f + HashVagonetas(seg, 70 + i) * 2.4f;
            Vector3 techo = PuntoPistaVagonetas(m, lat, 9.0f, (i - 0.5f) * 1.6f);
            DrawCylinderEx(
                techo, { techo.x, 9.0f - largo, techo.z }, 0.55f, 0.0f, 5,
                TinteVagonetas({ 66, 46, 62, 255 }, d + 2.0f, 0.0f)
            );
        }
    }

    // Suelo de roca, canales de lava y guijarros.
    Color suelo = (seg & 1) ? Color{ 64, 48, 60, 255 } : Color{ 72, 54, 66, 255 };
    CuboPistaVagonetas(m, 0.0f, -0.15f, 0.0f, 8.8f, 0.3f, 3.06f, TinteVagonetas(suelo, d, 0.0f));

    for (int lado = -1; lado <= 1; lado += 2)
    {
        Color lava = LavaVagonetas(v.tiempo, seg * 0.7f + lado);
        CuboPistaVagonetas(m, lado * 7.0f, -0.42f, 0.0f, 5.2f, 0.2f, 3.06f, TinteVagonetas(lava, d, 0.85f));

        if (decorar && !lejos && HashVagonetas(seg, 80 + lado) < 0.5f)
        {
            Color brillo = { 255, 210, 90, 255 };
            CuboPistaVagonetas(m, lado * (5.2f + HashVagonetas(seg, 90 + lado) * 3.5f), -0.3f, (h1 - 0.5f) * 2.0f, 0.7f, 0.05f, 0.35f, TinteVagonetas(brillo, d, 0.9f));
        }

        if (decorar && !lejos && h2 < 0.6f)
        {
            float lat = lado * (2.9f + HashVagonetas(seg, 100 + lado) * 1.2f);
            CuboPistaVagonetas(m, lat, 0.12f, (h0 - 0.5f) * 2.0f, 0.3f, 0.24f, 0.3f, TinteVagonetas({ 96, 78, 84, 255 }, d, 0.0f), h1 * 90.0f);
        }
    }

    // Via: balasto, durmientes y rieles.
    CuboPistaVagonetas(m, 0.0f, Y_BALASTO_VAGONETAS * 0.5f, 0.0f, 5.2f, Y_BALASTO_VAGONETAS, 3.06f, TinteVagonetas({ 112, 98, 92, 255 }, d, 0.0f));

    for (int i = 0; i < 2; i++)
    {
        float desp = (i - 0.5f) * 1.5f;
        CuboPistaVagonetas(m, 0.0f, (Y_BALASTO_VAGONETAS + Y_DURMIENTE_VAGONETAS) * 0.5f, desp, 4.9f, Y_DURMIENTE_VAGONETAS - Y_BALASTO_VAGONETAS, 0.36f, TinteVagonetas({ 126, 80, 46, 255 }, d, 0.0f));
    }

    for (int carril = 0; carril < CARRILES_VAGONETAS; carril++)
    {
        for (int rail = -1; rail <= 1; rail += 2)
        {
            CuboPistaVagonetas(
                m, LateralCarrilVagonetas(carril) + rail * SEPARACION_RUEDAS_VAGONETAS,
                (Y_DURMIENTE_VAGONETAS + Y_RIEL_VAGONETAS) * 0.5f, 0.0f,
                0.1f, Y_RIEL_VAGONETAS - Y_DURMIENTE_VAGONETAS, 3.25f,
                TinteVagonetas({ 176, 182, 196, 255 }, d, 0.0f)
            );
        }
    }

    // Marcos de madera cada 15 m con farol.
    if (seg % 5 == 0)
    {
        Color madera = TinteVagonetas({ 118, 76, 44, 255 }, d, 0.0f);
        for (int lado = -1; lado <= 1; lado += 2)
        {
            CuboPistaVagonetas(m, lado * 3.7f, 2.7f, 0.0f, 0.5f, 5.4f, 0.5f, madera);
        }
        CuboPistaVagonetas(m, 0.0f, 5.5f, 0.0f, 8.4f, 0.5f, 0.55f, madera);

        if (!lejos && d > 9.0f)
        {
            CuboPistaVagonetas(m, 0.0f, 4.95f, 0.0f, 0.05f, 0.6f, 0.05f, TinteVagonetas({ 60, 50, 44, 255 }, d, 0.0f));
            CuboPistaVagonetas(m, 0.0f, 4.45f, 0.0f, 0.34f, 0.34f, 0.34f, TinteVagonetas({ 255, 214, 120, 255 }, d, 0.95f));
            CuboPistaVagonetas(m, 0.0f, 4.45f, 0.0f, 1.4f, 1.4f, 1.4f, { 255, 190, 80, (unsigned char)(26 + 8 * std::sin(v.tiempo * 9.0f + seg)) });
        }
    }

    // Antorchas junto a la via.
    if (!lejos && seg % 7 == 3)
    {
        float lado = (seg & 1) ? 1.0f : -1.0f;
        float parpadeo = 0.75f + 0.25f * std::sin(v.tiempo * 17.0f + seg * 1.7f);
        CuboPistaVagonetas(m, lado * 4.0f, 0.55f, 0.0f, 0.14f, 1.1f, 0.14f, TinteVagonetas({ 92, 60, 36, 255 }, d, 0.0f));
        CuboPistaVagonetas(m, lado * 4.0f, 1.3f, 0.0f, 0.34f * parpadeo + 0.1f, 0.42f, 0.34f * parpadeo + 0.1f, TinteVagonetas({ 255, 150, 40, 255 }, d, 1.0f));
        CuboPistaVagonetas(m, lado * 4.0f, 1.45f, 0.0f, 0.18f, 0.3f, 0.18f, TinteVagonetas({ 255, 230, 120, 255 }, d, 1.0f));
        CuboPistaVagonetas(m, lado * 4.0f, 1.4f, 0.0f, 2.0f, 2.0f, 2.0f, { 255, 150, 50, (unsigned char)(24 * parpadeo) });
        CuboPistaVagonetas(m, lado * 3.3f, 0.02f, 0.0f, 3.2f, 0.02f, 3.0f, { 255, 160, 60, (unsigned char)(30 * parpadeo) });
    }
}


static void DibujarElementoVagonetas(
    const VistaVagonetas& v,
    const ElementoPistaVagonetas& e,
    int indice
)
{
    const CarroVagonetas& carro = v.minijuego->carros[v.k];
    if (carro.tomado[indice] && e.tipo != ELEMENTO_VAGONETAS_HUECO) return;

    // Nada pegado a la camara ni premios que ya quedaron atras.
    if (e.distancia - e.largo * 0.5f < v.distanciaCamara + 2.5f) return;
    if (!EsPeligroVagonetas(e.tipo) && e.distancia < carro.distancia - 1.2f) return;

    float d = std::max(0.0f, e.distancia - v.distanciaCamara);
    MarcoVagonetas m = MarcoPistaVagonetas(e.distancia);

    for (int carril = 0; carril < CARRILES_VAGONETAS; carril++)
    {
        if (!CarrilEnMascaraVagonetas(e.carriles, carril)) continue;
        float lat = LateralCarrilVagonetas(carril);

        if (e.tipo == ELEMENTO_VAGONETAS_ROCA)
        {
            float h = HashVagonetas((int)e.distancia, carril);
            float giro = (h - 0.5f) * 40.0f;
            CuboPistaVagonetas(m, lat, 0.62f, 0.0f, 1.45f, 1.0f, 1.25f, TinteVagonetas({ 86, 66, 74, 255 }, d, 0.0f), giro);
            CuboPistaVagonetas(m, lat + 0.1f, 1.28f, 0.0f, 1.0f, 0.5f, 0.9f, TinteVagonetas({ 112, 88, 92, 255 }, d, 0.0f), giro + 18.0f);
            CuboPistaVagonetas(m, lat - 0.45f, 0.32f, -0.2f, 0.6f, 0.5f, 0.8f, TinteVagonetas({ 70, 54, 62, 255 }, d, 0.0f), giro - 20.0f);
            Color grieta = LavaVagonetas(v.tiempo, h * 6.0f);
            CuboPistaVagonetas(m, lat - 0.2f, 0.7f, -0.64f, 0.12f, 0.7f, 0.04f, TinteVagonetas(grieta, d, 0.95f), giro);
            CuboPistaVagonetas(m, lat + 0.3f, 0.45f, -0.64f, 0.35f, 0.1f, 0.04f, TinteVagonetas(grieta, d, 0.95f), giro);
        }
        else if (e.tipo == ELEMENTO_VAGONETAS_HUECO)
        {
            float pulso = 0.5f + 0.5f * std::sin(v.tiempo * 3.0f + e.distancia);
            CuboPistaVagonetas(m, lat, 0.36f, 0.0f, 1.8f, 0.08f, e.largo, TinteVagonetas({ 12, 6, 14, 255 }, d, 0.0f));
            CuboPistaVagonetas(m, lat, 0.412f, 0.0f, 1.2f, 0.012f, e.largo - 1.0f,
                TinteVagonetas({ 255, (unsigned char)(70 + 70 * pulso), 20, 255 }, d, 0.9f));

            for (int borde = -1; borde <= 1; borde += 2)
            {
                CuboPistaVagonetas(m, lat, 0.42f, borde * (e.largo * 0.5f), 1.8f, 0.06f, 0.14f, TinteVagonetas({ 255, 190, 60, 255 }, d, 0.9f));
                CuboPistaVagonetas(m, lat + borde * 0.5f, 0.55f, borde * (e.largo * 0.5f + 0.1f), 0.1f, 0.3f, 0.1f, TinteVagonetas({ 126, 80, 46, 255 }, d, 0.0f), borde * 25.0f);
            }
        }
    }

    if (e.tipo == ELEMENTO_VAGONETAS_VIGA)
    {
        Color madera = TinteVagonetas({ 128, 84, 50, 255 }, d, 0.0f);
        CuboPistaVagonetas(m, 0.0f, 0.4f, 0.0f, 5.4f, 0.4f, 0.5f, madera);

        for (int lado = -1; lado <= 1; lado += 2)
        {
            CuboPistaVagonetas(m, lado * 2.85f, 0.45f, 0.0f, 0.4f, 0.9f, 0.5f, madera);
        }

        for (int i = 0; i < 7; i++)
        {
            Color franja = (i & 1) ? Color{ 40, 36, 44, 255 } : Color{ 255, 196, 40, 255 };
            CuboPistaVagonetas(m, (i - 3) * 0.74f, 0.4f, -0.27f, 0.74f, 0.3f, 0.04f, TinteVagonetas(franja, d, 0.6f));
        }
    }
    else if (e.tipo == ELEMENTO_VAGONETAS_MONEDA || e.tipo == ELEMENTO_VAGONETAS_MONEDA_ALTA)
    {
        int carril = e.carriles & 1 ? 0 : e.carriles & 2 ? 1 : 2;
        float lat = LateralCarrilVagonetas(carril);
        float y = (e.tipo == ELEMENTO_VAGONETAS_MONEDA_ALTA ? 1.75f : 1.0f)
            + 0.08f * std::sin(v.tiempo * 4.0f + e.distancia);
        float giro = v.tiempo * 200.0f + e.distancia * 30.0f;

        CuboPistaVagonetas(m, lat, y, 0.0f, 0.46f, 0.46f, 0.07f, TinteVagonetas({ 255, 200, 50, 255 }, d, 0.9f), giro);
        CuboPistaVagonetas(m, lat, y, 0.0f, 0.32f, 0.32f, 0.1f, TinteVagonetas({ 255, 238, 150, 255 }, d, 0.95f), giro);
        CuboPistaVagonetas(m, lat, y, 0.0f, 0.62f, 0.12f, 0.06f, TinteVagonetas({ 255, 200, 50, 255 }, d, 0.9f), giro + 45.0f);
    }
    else if (e.tipo == ELEMENTO_VAGONETAS_PLACA)
    {
        int carril = e.carriles & 1 ? 0 : e.carriles & 2 ? 1 : 2;
        float lat = LateralCarrilVagonetas(carril);
        float pulso = 0.5f + 0.5f * std::sin(v.tiempo * 8.0f - e.distancia);

        CuboPistaVagonetas(m, lat, 0.236f, 0.0f, 1.4f, 0.012f, 3.8f, { 80, 255, 220, (unsigned char)(50 + 40 * pulso) });

        for (int i = -1; i <= 1; i++)
        {
            Color color = pulso > (i + 1) * 0.3f ? Color{ 120, 255, 235, 255 } : Color{ 40, 190, 190, 255 };
            color = TinteVagonetas(color, d, 1.0f);
            CuboPistaVagonetas(m, lat - 0.3f, 0.262f, i * 1.0f - 0.18f, 0.7f, 0.03f, 0.2f, color, 38.0f);
            CuboPistaVagonetas(m, lat + 0.3f, 0.262f, i * 1.0f - 0.18f, 0.7f, 0.03f, 0.2f, color, -38.0f);
        }
    }
}


static void DibujarSalidaMetaVagonetas(const VistaVagonetas& v)
{
    const MinijuegoVagonetasDuelo& mj = *v.minijuego;
    float distanciaCarro = mj.carros[v.k].distancia;

    // Meta.
    if (distanciaCarro > LARGO_PISTA_VAGONETAS - 85.0f)
    {
        MarcoVagonetas m = MarcoPistaVagonetas(LARGO_PISTA_VAGONETAS);
        float d = std::max(0.0f, LARGO_PISTA_VAGONETAS - v.distanciaCamara);
        Color poste = TinteVagonetas({ 230, 230, 238, 255 }, d, 0.3f);

        for (int lado = -1; lado <= 1; lado += 2)
        {
            CuboPistaVagonetas(m, lado * 3.6f, 2.6f, 0.0f, 0.5f, 5.2f, 0.5f, poste);
        }

        CuboPistaVagonetas(m, 0.0f, 5.5f, 0.0f, 7.8f, 0.9f, 0.5f, TinteVagonetas({ 30, 30, 38, 255 }, d, 0.2f));

        for (int i = 0; i < 14; i++)
        {
            for (int fila = 0; fila < 2; fila++)
            {
                Color c = ((i + fila) & 1) ? Color{ 245, 245, 250, 255 } : Color{ 28, 28, 34, 255 };
                CuboPistaVagonetas(m, (i - 6.5f) * 0.55f, 5.28f + fila * 0.44f, -0.27f, 0.55f, 0.44f, 0.04f, TinteVagonetas(c, d, 0.5f));
            }
        }

        // Linea cuadriculada sobre los durmientes.
        for (int i = 0; i < 12; i++)
        {
            for (int fila = 0; fila < 2; fila++)
            {
                Color c = ((i + fila) & 1) ? Color{ 245, 245, 250, 255 } : Color{ 28, 28, 34, 255 };
                CuboPistaVagonetas(m, (i - 5.5f) * 0.5f, 0.236f, (fila - 0.5f) * 0.5f, 0.5f, 0.012f, 0.5f, TinteVagonetas(c, d, 0.5f));
            }
        }
    }

    // Salida: semaforo de tres luces que se enciende con la cuenta.
    if (distanciaCarro < 30.0f)
    {
        MarcoVagonetas m = MarcoPistaVagonetas(2.5f);
        float d = std::max(0.0f, 2.5f - v.distanciaCamara);
        Color poste = TinteVagonetas({ 120, 78, 46, 255 }, d, 0.0f);

        for (int lado = -1; lado <= 1; lado += 2)
        {
            CuboPistaVagonetas(m, lado * 3.4f, 2.4f, 0.0f, 0.5f, 4.8f, 0.5f, poste);
        }

        CuboPistaVagonetas(m, 0.0f, 4.9f, 0.0f, 7.2f, 0.7f, 0.55f, poste);

        bool verde = mj.fase != FASE_VAGONETAS_PREPARACION;
        int encendidas = 3;
        if (!verde)
        {
            encendidas = 4 - (int)std::ceil(std::max(0.001f, mj.tiempoPreparacion));
            encendidas = std::max(0, std::min(3, encendidas));
        }

        for (int i = 0; i < 3; i++)
        {
            bool prendida = verde || i < encendidas;
            Color c = verde ? Color{ 90, 255, 120, 255 } : (prendida ? Color{ 255, 70, 60, 255 } : Color{ 70, 30, 34, 255 });
            CuboPistaVagonetas(m, (i - 1) * 1.3f, 4.9f, -0.3f, 0.7f, 0.5f, 0.1f, TinteVagonetas(c, d, prendida ? 1.0f : 0.1f));
            if (prendida)
            {
                c.a = 40;
                CuboPistaVagonetas(m, (i - 1) * 1.3f, 4.9f, -0.35f, 1.5f, 1.2f, 0.2f, c);
            }
        }
    }
}


static void DibujarRuedaVagonetas(float x, float z, float giro)
{
    rlPushMatrix();
    rlTranslatef(x, Y_RIEL_VAGONETAS + 0.17f, z);
    rlRotatef(giro * RAD2DEG, 1.0f, 0.0f, 0.0f);
    DrawCylinderEx({ -0.06f, 0.0f, 0.0f }, { 0.06f, 0.0f, 0.0f }, 0.17f, 0.17f, 8, Color{ 40, 42, 52, 255 });
    (DrawCube)({ 0.0f, 0.0f, 0.0f }, 0.14f, 0.3f, 0.05f, Color{ 190, 196, 210, 255 });
    (DrawCube)({ 0.0f, 0.0f, 0.0f }, 0.14f, 0.05f, 0.3f, Color{ 190, 196, 210, 255 });
    rlPopMatrix();
}


static void DibujarCarroVagonetas(
    const VistaVagonetas& v,
    int k,
    const Participante participantes[]
)
{
    const MinijuegoVagonetasDuelo& mj = *v.minijuego;
    const CarroVagonetas& c = mj.carros[k];
    MarcoVagonetas m = MarcoPistaVagonetas(c.distancia);
    Color acento = participantes[c.participante].color;

    float vibracion = c.altura < 0.01f && c.velocidad > 1.0f
        ? std::sin(v.tiempo * 47.0f + k) * 0.012f
        : 0.0f;
    float hundimiento = c.hundido > 0.0f ? -0.22f * std::sin(c.hundido / 0.5f * 3.14159f) : 0.0f;
    float y0 = std::max(0.0f, c.altura) + vibracion + hundimiento;

    Vector3 base = PuntoPistaVagonetas(m, c.lateral, 0.0f, 0.0f);

    // Sombra de contacto sobre los durmientes.
    DibujarSombraRetroRectangular(
        { base.x, 0.235f + c.altura, base.z }, 1.2f, 1.9f, m.rumbo, 0.235f, 0.30f
    );

    rlPushMatrix();
    rlTranslatef(base.x, y0, base.z);
    rlRotatef(m.guinadaGrados - c.guinada, 0.0f, 1.0f, 0.0f);
    rlRotatef(c.inclinacion * 0.6f, 0.0f, 0.0f, 1.0f);
    float cabeceo = c.altura > 0.02f ? Clamp(-c.velocidadVertical * 1.2f, -8.0f, 8.0f) : 0.0f;
    rlRotatef(cabeceo, 1.0f, 0.0f, 0.0f);

    Color hierro = { 96, 104, 124, 255 };
    Color oscuro = { 50, 52, 66, 255 };
    Color borde = { 176, 184, 200, 255 };
    if (c.destelloChoque > 0.0f)
    {
        float f = c.destelloChoque;
        hierro = { (unsigned char)(96 + 150 * f), (unsigned char)(104 - 40 * f), (unsigned char)(124 - 60 * f), 255 };
    }

    (DrawCube)({ 0.0f, 0.60f, 0.0f }, 1.02f, 0.1f, 1.75f, oscuro);
    (DrawCube)({ -0.5f, 0.88f, 0.0f }, 0.1f, 0.56f, 1.75f, hierro);
    (DrawCube)({ 0.5f, 0.88f, 0.0f }, 0.1f, 0.56f, 1.75f, hierro);
    (DrawCube)({ 0.0f, 0.88f, 0.84f }, 1.02f, 0.56f, 0.1f, hierro);
    (DrawCube)({ 0.0f, 0.82f, -0.84f }, 1.02f, 0.46f, 0.1f, hierro);
    (DrawCube)({ -0.5f, 1.17f, 0.0f }, 0.18f, 0.08f, 1.85f, borde);
    (DrawCube)({ 0.5f, 1.17f, 0.0f }, 0.18f, 0.08f, 1.85f, borde);
    (DrawCube)({ 0.0f, 1.12f, 0.88f }, 1.1f, 0.08f, 0.16f, borde);
    (DrawCube)({ 0.0f, 1.07f, -0.88f }, 1.1f, 0.08f, 0.16f, borde);
    // Franja del color del jugador y remaches.
    (DrawCube)({ -0.555f, 0.9f, 0.0f }, 0.03f, 0.14f, 1.4f, acento);
    (DrawCube)({ 0.555f, 0.9f, 0.0f }, 0.03f, 0.14f, 1.4f, acento);
    (DrawCube)({ 0.0f, 0.82f, -0.895f }, 0.7f, 0.14f, 0.03f, acento);
    (DrawCube)({ 0.0f, 0.88f, 0.895f }, 0.7f, 0.14f, 0.03f, acento);
    for (int i = -1; i <= 1; i += 2)
    {
        (DrawCube)({ i * 0.56f, 1.02f, -0.6f }, 0.04f, 0.06f, 0.06f, borde);
        (DrawCube)({ i * 0.56f, 1.02f, 0.6f }, 0.04f, 0.06f, 0.06f, borde);
    }

    // Farol delantero.
    (DrawCube)({ 0.0f, 0.98f, -0.95f }, 0.2f, 0.16f, 0.1f, Color{ 255, 240, 170, 255 });

    // Ruedas con rayos que giran.
    DibujarRuedaVagonetas(-SEPARACION_RUEDAS_VAGONETAS, -0.55f, c.giroRuedas);
    DibujarRuedaVagonetas(SEPARACION_RUEDAS_VAGONETAS, -0.55f, c.giroRuedas);
    DibujarRuedaVagonetas(-SEPARACION_RUEDAS_VAGONETAS, 0.55f, c.giroRuedas);
    DibujarRuedaVagonetas(SEPARACION_RUEDAS_VAGONETAS, 0.55f, c.giroRuedas);

    // Llama de turbo detras.
    if (c.turbo > 0.0f || c.impulsoPlaca > 0.0f)
    {
        float f = 0.7f + 0.3f * std::sin(v.tiempo * 60.0f);
        bool placa = c.turbo <= 0.0f;
        Color nucleo = placa ? Color{ 150, 255, 245, 255 } : Color{ 255, 220, 90, 255 };
        Color externo = placa ? Color{ 60, 210, 230, 150 } : Color{ 255, 110, 30, 150 };
        (DrawCube)({ 0.0f, 0.75f, 1.2f + 0.4f * f }, 0.3f, 0.3f, 0.7f * f, nucleo);
        (DrawCube)({ 0.0f, 0.75f, 1.45f + 0.7f * f }, 0.52f, 0.5f, 1.3f * f, externo);
    }

    rlPopMatrix();

    // Personaje dentro de la vagoneta.
    JugadorPrueba visual{};
    visual.direccionMirada = { std::sin(m.rumbo), 0.0f, -std::cos(m.rumbo) };
    // MODELO FUTURO: personaje agachado dentro de la vagoneta.
    DibujarModeloJugadorEnPosicion(
        { base.x, y0 + 0.64f, base.z },
        ObtenerAnguloModeloJugadorCompartido(visual),
        participantes[c.participante].color,
        ESCALA_MODELO_JUGADOR_3D * 0.55f
    );
}


static void DibujarParticulasVagonetas(const MinijuegoVagonetasDuelo& mj, int k)
{
    for (int i = 0; i < MAX_PARTICULAS_VAGONETAS; i++)
    {
        const ParticulaVagonetas& p = mj.particulas[k][i];
        if (!p.activa) continue;

        float f = p.vida / p.vidaMaxima;
        Color c = p.color;

        if (p.polvo)
        {
            c.a = (unsigned char)(120.0f * f);
            float tam = p.tamano * (2.2f - f);
            (DrawCube)(p.posicion, tam, tam, tam, c);
        }
        else
        {
            c.a = (unsigned char)(255.0f * std::min(1.0f, f * 2.0f));
            float tam = p.tamano * (0.5f + 0.5f * f);
            (DrawCube)(p.posicion, tam, tam, tam, c);
        }
    }
}


//==================================================
// DIBUJO: VISTA DE UN JUGADOR
//==================================================


static float EnvolventeTurboVagonetas(const CarroVagonetas& c)
{
    float turbo = c.turbo > 0.0f
        ? std::min(std::min(1.0f, (DURACION_TURBO_VAGONETAS - c.turbo) / 0.12f), std::min(1.0f, c.turbo / 0.25f))
        : 0.0f;
    float placa = c.impulsoPlaca > 0.0f
        ? 0.7f * std::min(std::min(1.0f, (DURACION_PLACA_VAGONETAS - c.impulsoPlaca) / 0.12f), std::min(1.0f, c.impulsoPlaca / 0.25f))
        : 0.0f;
    return std::max(0.0f, std::max(turbo, placa));
}


static const char* TextoAvisoVagonetas(AvisoVagonetas tipo)
{
    switch (tipo)
    {
    case AVISO_VAGONETAS_TURBO: return "¡TURBO!";
    case AVISO_VAGONETAS_IMPULSO: return "¡IMPULSO!";
    case AVISO_VAGONETAS_CHOQUE: return "¡CHOQUE!";
    case AVISO_VAGONETAS_ADELANTE: return "¡ADELANTE!";
    default: return "";
    }
}


static Color ColorAvisoVagonetas(AvisoVagonetas tipo)
{
    switch (tipo)
    {
    case AVISO_VAGONETAS_TURBO: return { 255, 214, 90, 255 };
    case AVISO_VAGONETAS_IMPULSO: return { 120, 255, 235, 255 };
    case AVISO_VAGONETAS_CHOQUE: return { 255, 96, 80, 255 };
    default: return { 140, 255, 150, 255 };
    }
}


static void DibujarVistaVagonetas(
    const MinijuegoVagonetasDuelo& mj,
    int k,
    Rectangle vista,
    const Participante participantes[]
)
{
    const CarroVagonetas& c = mj.carros[k];
    float factorMovimiento = ObtenerFactorMovimientoCamaraMinijuegos();
    float env = EnvolventeTurboVagonetas(c);
    float t = mj.tiempoAnimacion;

    // Camara detras y arriba; en la cuenta regresiva baja desde mas lejos.
    float intro = mj.fase == FASE_VAGONETAS_PREPARACION
        ? Clamp(mj.tiempoPreparacion / DURACION_PREPARACION_VAGONETAS, 0.0f, 1.0f) : 0.0f;
    intro = intro * intro * factorMovimiento;
    float atras = 6.4f + 2.4f * intro + 0.9f * env * factorMovimiento;
    float altura = 3.3f + 1.5f * intro;
    float distanciaCamara = c.distancia - atras;

    Camera3D camara{};
    camara.position = PosicionPistaVagonetas(distanciaCamara, c.lateral * 0.55f);
    camara.position.y = altura;
    camara.target = PosicionPistaVagonetas(c.distancia + 10.0f, c.lateral * 0.35f);
    camara.target.y = 0.95f;
    camara.up = { 0.0f, 1.0f, 0.0f };
    camara.fovy = 58.0f + 9.0f * env * factorMovimiento;
    camara.projection = CAMERA_PERSPECTIVE;

    float temblor = (c.sacudida * 0.20f + env * 0.035f) * factorMovimiento;
    camara.position.x += std::sin(t * 53.0f + k) * temblor;
    camara.position.y += std::cos(t * 61.0f) * temblor;
    camara.target.x += std::sin(t * 47.0f) * temblor * 0.4f;

    // Escena 3D en la mitad de pantalla (viewport y proyeccion propios).
    float escalaX = (float)rlGetFramebufferWidth() / (float)GetScreenWidth();
    float escalaY = (float)rlGetFramebufferHeight() / (float)GetScreenHeight();

    BeginScissorMode((int)vista.x, (int)vista.y, (int)vista.width, (int)vista.height);
    ClearBackground(NIEBLA_VAGONETAS);
    DrawRectangleGradientV(
        (int)vista.x, (int)vista.y, (int)vista.width, (int)vista.height,
        Color{ 10, 6, 20, 255 }, Color{ 96, 34, 30, 255 }
    );

    BeginMode3D(camara);
    rlViewport(
        (int)(vista.x * escalaX),
        (int)((float)rlGetFramebufferHeight() - (vista.y + vista.height) * escalaY),
        (int)(vista.width * escalaX),
        (int)(vista.height * escalaY)
    );
    rlSetMatrixProjection(MatrixPerspective(
        camara.fovy * DEG2RAD, vista.width / vista.height, 0.3, 230.0
    ));

    VistaVagonetas contexto{ &mj, k, distanciaCamara, t };
    int segDesde = (int)std::floor((c.distancia - 14.0f) / 3.0f);
    int segHasta = (int)std::floor((c.distancia + 80.0f) / 3.0f);
    int primerElemento = std::max(0, c.cursor - 4);

    // De lejos a cerca: los brillos translucidos quedan bien ordenados.
    int elementoActual = mj.cantidadElementos - 1;
    while (elementoActual >= 0 && mj.elementos[elementoActual].distancia > c.distancia + 80.0f)
    {
        elementoActual--;
    }

    DibujarSalidaMetaVagonetas(contexto);

    for (int seg = segHasta; seg >= segDesde; seg--)
    {
        DibujarSegmentoVagonetas(contexto, seg);

        float finSeg = seg * 3.0f;
        while (elementoActual >= primerElemento && mj.elementos[elementoActual].distancia >= finSeg)
        {
            DibujarElementoVagonetas(contexto, mj.elementos[elementoActual], elementoActual);
            elementoActual--;
        }
    }

    DibujarCarroVagonetas(contexto, k, participantes);
    DibujarParticulasVagonetas(mj, k);
    EndMode3D();
    rlViewport(0, 0, rlGetFramebufferWidth(), rlGetFramebufferHeight());

    // Superposiciones 2D de la mitad.
    int x0 = (int)vista.x;
    int y0 = (int)vista.y;
    int ancho = (int)vista.width;
    int alto = (int)vista.height;
    float cx = vista.x + vista.width * 0.5f;

    if (env > 0.01f && factorMovimiento > 0.0f)
    {
        unsigned char alfa = (unsigned char)(70.0f * env);
        Color tinte = c.turbo > 0.0f ? Color{ 255, 190, 60, alfa } : Color{ 90, 230, 255, alfa };
        Color claro = { tinte.r, tinte.g, tinte.b, 0 };
        int borde = alto / 4;
        DrawRectangleGradientV(x0, y0, ancho, borde, tinte, claro);
        DrawRectangleGradientV(x0, y0 + alto - borde, ancho, borde, claro, tinte);
        DrawRectangleGradientH(x0, y0, ancho / 5, alto, tinte, claro);
        DrawRectangleGradientH(x0 + ancho - ancho / 5, y0, ancho / 5, alto, claro, tinte);

        // Lineas de velocidad radiales.
        float cy = vista.y + vista.height * 0.52f;
        for (int i = 0; i < 22; i++)
        {
            float fase = std::fmod(t * 2.6f + HashVagonetas(i, k + 7), 1.0f);
            float ang = HashVagonetas(i, 3) * 6.2832f;
            float r0 = vista.height * (0.30f + 0.5f * fase);
            float r1 = r0 + vista.height * (0.08f + 0.22f * fase);
            unsigned char a = (unsigned char)(150.0f * env * (1.0f - fase));
            Vector2 p0 = { cx + std::cos(ang) * r0 * 0.9f, cy + std::sin(ang) * r0 * 0.7f };
            Vector2 p1 = { cx + std::cos(ang) * r1 * 0.9f, cy + std::sin(ang) * r1 * 0.7f };
            if (p1.x < vista.x || p1.x > vista.x + vista.width) continue;
            DrawLineEx(p0, p1, 2.0f, Color{ 255, 255, 255, a });
        }
    }

    if (c.destelloChoque > 0.0f)
    {
        unsigned char alfa = (unsigned char)(110.0f * c.destelloChoque);
        Color rojo = { 255, 40, 30, alfa };
        Color nada = { 255, 40, 30, 0 };
        int borde = alto / 5;
        DrawRectangleGradientV(x0, y0, ancho, borde, rojo, nada);
        DrawRectangleGradientV(x0, y0 + alto - borde, ancho, borde, nada, rojo);
        DrawRectangleGradientH(x0, y0, ancho / 7, alto, rojo, nada);
        DrawRectangleGradientH(x0 + ancho - ancho / 7, y0, ancho / 7, alto, nada, rojo);
    }

    // Avisos flotantes.
    for (int i = 0; i < MAX_AVISOS_VAGONETAS; i++)
    {
        const AvisoFlotanteVagonetas& aviso = c.avisos[i];
        if (aviso.tiempo <= 0.0f) continue;

        float edad = 1.1f - aviso.tiempo;
        Color color = ColorAvisoVagonetas(aviso.tipo);
        color.a = (unsigned char)(255.0f * Clamp(aviso.tiempo * 3.0f, 0.0f, 1.0f));
        float subida = factorMovimiento > 0.0f ? edad * 36.0f : 0.0f;
        DibujarTextoMundo(
            TextoAvisoVagonetas(aviso.tipo), cx,
            vista.y + vista.height * 0.30f - subida + i * Px(34.0f), UI_TEXTO_GRANDE, color
        );
    }

    // Medidor de turbo: tres celdas (cada una es un turbo).
    {
        float anchoCelda = std::min(Px(70.0f), vista.width * 0.12f);
        float altoCelda = Px(16.0f);
        float separacion = Px(6.0f);
        float total = anchoCelda * 3.0f + separacion * 2.0f;
        float x = cx - total * 0.5f;
        float y = vista.y + vista.height - Px(52.0f);

        DibujarTextoMundo("TURBO", cx, y - Px(30.0f), UI_TEXTO_PEQUENO, Color{ 255, 240, 210, 255 });
        DrawRectangle((int)x - 5, (int)y - 5, (int)total + 10, (int)altoCelda + 10, Fade(BLACK, 0.6f));

        for (int i = 0; i < 3; i++)
        {
            float llenado = Clamp((c.energia - i * COSTO_TURBO_VAGONETAS) / COSTO_TURBO_VAGONETAS, 0.0f, 1.0f);
            float xCelda = x + i * (anchoCelda + separacion);
            DrawRectangle((int)xCelda, (int)y, (int)anchoCelda, (int)altoCelda, Color{ 40, 34, 50, 255 });

            bool lista = llenado >= 1.0f;
            Color relleno = c.turbo > 0.0f ? Color{ 255, 214, 90, 255 }
                : lista ? Color{ 90, 235, 255, 255 } : Color{ 60, 150, 190, 255 };
            DrawRectangle((int)xCelda, (int)y, (int)(anchoCelda * llenado), (int)altoCelda, relleno);
            DrawRectangleLines((int)xCelda, (int)y, (int)anchoCelda, (int)altoCelda, lista ? WHITE : Color{ 150, 150, 170, 255 });
        }
    }

    // Puesto y ventaja.
    {
        const CarroVagonetas& rival = mj.carros[1 - k];
        float diferencia = c.distancia - rival.distancia;
        bool delante = diferencia >= 0.0f;
        bool parejo = std::fabs(diferencia) < 1.0f;
        const char* puesto = delante ? "1RO" : "2DO";
        Color color = parejo ? Color{ 255, 240, 210, 255 } : delante ? Color{ 140, 255, 150, 255 } : Color{ 255, 150, 120, 255 };

        DibujarTextoMundo(puesto, vista.x + Px(86.0f), vista.y + vista.height - Px(150.0f), UI_TEXTO_TITULO, color);

        if (!parejo)
        {
            DibujarTextoMundo(
                TextFormat("%s%d M", delante ? "+" : "-", (int)std::fabs(diferencia)),
                vista.x + Px(86.0f), vista.y + vista.height - Px(86.0f), UI_TEXTO, color
            );
        }
    }

    EndScissorMode();
}


//==================================================
// DIBUJO: ESCENA COMPLETA
//==================================================


static void DibujarBarraProgresoVagonetas(const MinijuegoVagonetasDuelo& mj, const Participante participantes[])
{
    int ancho = GetScreenWidth();
    float anchoBarra = Clamp(ancho * 0.30f, 300.0f, 560.0f);
    float x = ancho * 0.5f - anchoBarra * 0.5f;
    float y = Px(112.0f);
    float altoBarra = Px(12.0f);

    DrawRectangle((int)x - 14, (int)y - 14, (int)anchoBarra + 28, (int)altoBarra + 28, Fade(Color{ 22, 14, 30, 255 }, 0.82f));
    DrawRectangle((int)x, (int)y, (int)anchoBarra, (int)altoBarra, Color{ 60, 44, 70, 255 });

    for (int i = 1; i < 8; i++)
    {
        DrawRectangle((int)(x + anchoBarra * i / 8.0f), (int)y, 1, (int)altoBarra, Color{ 120, 100, 130, 255 });
    }

    // Bandera de meta.
    for (int i = 0; i < 4; i++)
    {
        Color c = (i & 1) ? Color{ 245, 245, 250, 255 } : Color{ 28, 28, 34, 255 };
        DrawRectangle((int)(x + anchoBarra) - 4, (int)(y - 8 + i * (altoBarra + 16) / 4), 8, (int)((altoBarra + 16) / 4) + 1, c);
    }

    // Un marcador por carro: el primero arriba, el segundo abajo.
    for (int k = 0; k < 2; k++)
    {
        const CarroVagonetas& c = mj.carros[k];
        float f = Clamp(c.distancia / LARGO_PISTA_VAGONETAS, 0.0f, 1.0f);
        float px = x + anchoBarra * f;
        Color color = participantes[c.participante].color;
        int py = (int)(k == 0 ? y - 7.0f : y + altoBarra - 2.0f);
        DrawRectangle((int)px - 8, py, 16, 9, BLACK);
        DrawRectangle((int)px - 7, py + 1, 14, 7, color);
    }
}


void MinijuegoVagonetasDuelo::Dibujar(
    const JugadorPrueba jugadores[],
    int cantidadMaxima,
    const Participante participantes[],
    bool mostrarDebug
) const
{
    (void)jugadores;
    (void)cantidadMaxima;
    (void)mostrarDebug;

    int ancho = GetScreenWidth();
    int alto = GetScreenHeight();

    SeleccionarTemaVisualMinijuego(TEMA_VISUAL_NINGUNO);

    if (cantidadCarros >= 2)
    {
        int mitad = ancho / 2;
        DibujarVistaVagonetas(*this, 0, { 0.0f, 0.0f, (float)(mitad - 3), (float)alto }, participantes);
        DibujarVistaVagonetas(*this, 1, { (float)(mitad + 3), 0.0f, (float)(ancho - mitad - 3), (float)alto }, participantes);

        // Divisoria con brillo de lava.
        DrawRectangle(mitad - 3, 0, 6, alto, Color{ 14, 8, 20, 255 });
        DrawRectangle(mitad - 1, 0, 2, alto, Color{ 255, 150, 50, 255 });
    }
    else
    {
        ClearBackground(NIEBLA_VAGONETAS);
    }

    DatosHudMinijuego hud;
    PrepararDatosHudMinijuego(
        hud,
        "VAGONETAS DE MINA",
        "CRUZÁ LA META PRIMERO: CAMBIÁ DE RIEL, SALTÁ LAS VÍAS ROTAS Y USÁ EL TURBO CON CABEZA.",
        resultado
    );

    hud.fase = fase == FASE_VAGONETAS_PREPARACION
        ? HUD_MINIJUEGO_CUENTA
        : (fase == FASE_VAGONETAS_CARRERA ? HUD_MINIJUEGO_JUGANDO : HUD_MINIJUEGO_FIN);
    hud.cuentaRestante = tiempoPreparacion;
    hud.reloj = HUD_RELOJ_CUENTA_ATRAS;
    hud.tiempoRestante = tiempoRestante;
    hud.tiempoTotal = TOPE_TIEMPO_VAGONETAS;
    hud.segundosAlerta = 10.0f;
    hud.finPorTiempo = fase == FASE_VAGONETAS_TERMINADO && ganador >= 0 &&
        !carros[0].llego && !carros[1].llego && tiempoRestante <= 0.0f;

    for (int k = 0; k < cantidadCarros; k++)
    {
        const CarroVagonetas& c = carros[k];
        JugadorHudMinijuego& jugador = hud.jugadores[c.participante];
        if (!jugador.visible) continue;

        std::snprintf(jugador.valor, sizeof(jugador.valor), "%d", c.monedas);
        jugador.valorNumerico = c.monedas;

        if (c.aturdido > 0.0f) std::snprintf(jugador.estado, sizeof(jugador.estado), "CHOQUE");
        else if (c.turbo > 0.0f) std::snprintf(jugador.estado, sizeof(jugador.estado), "TURBO");

        if (fase == FASE_VAGONETAS_PREPARACION && !c.controlIA)
        {
            std::snprintf(
                jugador.controles, sizeof(jugador.controles), "%s: RIEL\nSALTO / %s: TURBO",
                TextoMoverHud(participantes[c.participante]),
                TextoBotonGolpeHud(participantes[c.participante])
            );
        }

        if (fase == FASE_VAGONETAS_TERMINADO && resultado.estado == RESULTADO_MINIJUEGO_FINALIZADO)
        {
            if (c.llego)
            {
                int centesimas = (int)std::lround(c.tiempoLlegada * 100.0f);
                std::snprintf(jugador.detalleFinal, sizeof(jugador.detalleFinal), "%d,%02d S", centesimas / 100, centesimas % 100);
            }
            else
            {
                std::snprintf(jugador.detalleFinal, sizeof(jugador.detalleFinal), "A %d M",
                    (int)std::ceil(LARGO_PISTA_VAGONETAS - c.distancia));
            }
        }
    }

    DibujarHudMinijuego(hud, participantes);

    if (cantidadCarros >= 2 && fase != FASE_VAGONETAS_PREPARACION)
    {
        DibujarBarraProgresoVagonetas(*this, participantes);
    }
}
