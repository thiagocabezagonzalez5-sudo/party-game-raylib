#include "Minigames/MinijuegoLaberintoInclinado.h"

#include "Minigames/AudioMinijuegos.h"
#include "Minigames/EfectosVisualesMinijuegos.h"
#include "Minigames/HudMinijuego.h"
#include "Minigames/ModeloJugadorCompartido.h"
#include "Minigames/ModelosEscenariosRetro3D.h"
#include "Minigames/SombrasRetro.h"
#include "Systems/CalidadGrafica.h"
#include "Systems/Input.h"
#include "UI/EstiloUI.h"

#include "raylib.h"
#include "rlgl.h"

#include <cmath>
#include <cstdio>


//==================================================
// CONSTANTES
//==================================================

static const float DURACION_PREPARACION_LABERINTO = 3.0f;
static const float DURACION_PARTIDA_LABERINTO = 60.0f;
// Cuando alguien llega a la meta, el resto tiene este tiempo para terminar.
static const float PLAZO_TRAS_PRIMERA_LLEGADA = 14.0f;

static const float RADIO_ESFERA = 0.28f;
static const float RADIO_AGUJERO = 0.34f;
static const float ACELERACION_MAXIMA = 9.5f;
static const float VELOCIDAD_MAXIMA = 4.8f;
static const float VELOCIDAD_MAXIMA_EMPUJE = 6.4f;
static const float FRICCION_ESFERA = 1.1f;
static const float REBOTE_ESFERA = 0.38f;
static const float IMPACTO_FUERTE = 2.6f;
static const float INCLINACION_MAXIMA_GRADOS = 12.0f;
static const float DURACION_CAIDA = 0.7f;
static const float RADIO_META = 0.42f;

static const float PERIODO_TRAMPA = 3.6f;
static const float DURACION_AVISO_TRAMPA = 1.0f;
static const float DURACION_PULSO_TRAMPA = 0.4f;
static const float ACELERACION_TRAMPA = 13.0f;

static const float MITAD_TABLERO = 5.5f;
static const float ALTO_MURO = 0.6f;

static const Color COLOR_PIEDRA_CLARA = { 168, 154, 128, 255 };
static const Color COLOR_PIEDRA = { 132, 119, 98, 255 };
static const Color COLOR_PIEDRA_OSCURA = { 82, 74, 62, 255 };
static const Color COLOR_LOSA = { 108, 99, 83, 255 };
static const Color COLOR_LOSA_CLARA = { 122, 112, 94, 255 };
static const Color COLOR_JADE = { 52, 176, 128, 255 };
static const Color COLOR_ORO = { 240, 196, 70, 255 };

static const Color COLORES_JUGADORES_LABERINTO[MAX_PARTICIPANTES] =
{
    { 235, 80, 80, 255 },
    { 80, 140, 240, 255 },
    { 90, 205, 115, 255 },
    { 245, 205, 70, 255 }
};


//==================================================
// DISENO LOGICO DEL LABERINTO
//==================================================
//
// '#' muro, ' ' suelo, 'o' agujero, 'S' salida, 'A' y 'B' checkpoints,
// 'M' meta. Los otros disenos son este mismo mapa transpuesto o girado, por
// lo que todos son jugables por construccion.
//==================================================

static const char* const DISENO_BASE[FILAS_LABERINTO] =
{
    "###########",
    "#S        #",
    "######### #",
    "#   o     #",
    "#     o   #",
    "# #########",
    "#A        #",
    "######### #",
    "#M   o   B#",
    "#      o  #",
    "###########"
};

// Trampas del diseno base: columna/fila inicio, fin y direccion del empuje.
struct TrampaBaseLaberinto
{
    int c0;
    int r0;
    int c1;
    int r1;
    float dx;
    float dz;
    float desfase;
};

static const TrampaBaseLaberinto TRAMPAS_BASE[MAX_TRAMPAS_LABERINTO] =
{
    { 3, 1, 7, 1, -1.0f, 0.0f, 1.0f },
    { 3, 6, 7, 6, -1.0f, 0.0f, 2.3f }
};


static void MapearCeldaDiseno(int diseno, int c, int r, int& salidaC, int& salidaR)
{
    if (diseno == 1)
    {
        salidaC = r;
        salidaR = c;
    }
    else if (diseno == 2)
    {
        salidaC = FILAS_LABERINTO - 1 - c;
        salidaR = FILAS_LABERINTO - 1 - r;
    }
    else
    {
        salidaC = c;
        salidaR = r;
    }
}


static void MapearDireccionDiseno(int diseno, float dx, float dz, float& salidaX, float& salidaZ)
{
    if (diseno == 1)
    {
        salidaX = dz;
        salidaZ = dx;
    }
    else if (diseno == 2)
    {
        salidaX = -dx;
        salidaZ = -dz;
    }
    else
    {
        salidaX = dx;
        salidaZ = dz;
    }
}


static void CalcularDistanciasMeta(MinijuegoLaberintoInclinado& m)
{
    for (int r = 0; r < FILAS_LABERINTO; r++)
    {
        for (int c = 0; c < COLUMNAS_LABERINTO; c++)
        {
            m.distanciaMeta[r][c] = -1;
        }
    }

    int cola[FILAS_LABERINTO * COLUMNAS_LABERINTO]{};
    int cabeza = 0;
    int final = 0;

    m.distanciaMeta[m.metaFila][m.metaColumna] = 0;
    cola[final++] = m.metaFila * COLUMNAS_LABERINTO + m.metaColumna;

    const int saltoC[4] = { 1, -1, 0, 0 };
    const int saltoR[4] = { 0, 0, 1, -1 };

    while (cabeza < final)
    {
        int actual = cola[cabeza++];
        int c = actual % COLUMNAS_LABERINTO;
        int r = actual / COLUMNAS_LABERINTO;

        for (int k = 0; k < 4; k++)
        {
            int nc = c + saltoC[k];
            int nr = r + saltoR[k];

            if (nc < 0 || nr < 0 || nc >= COLUMNAS_LABERINTO || nr >= FILAS_LABERINTO)
            {
                continue;
            }

            if (m.celdas[nr][nc] != CELDA_LABERINTO_LIBRE || m.distanciaMeta[nr][nc] >= 0)
            {
                continue;
            }

            m.distanciaMeta[nr][nc] = m.distanciaMeta[r][c] + 1;
            cola[final++] = nr * COLUMNAS_LABERINTO + nc;
        }
    }

    int d = m.distanciaMeta[m.inicioFila][m.inicioColumna];
    m.distanciaInicio = d > 0 ? d : 1;
}


static void ConstruirDisenoLaberinto(MinijuegoLaberintoInclinado& m, int diseno)
{
    for (int r = 0; r < FILAS_LABERINTO; r++)
    {
        for (int c = 0; c < COLUMNAS_LABERINTO; c++)
        {
            int dc = 0;
            int dr = 0;
            MapearCeldaDiseno(diseno, c, r, dc, dr);

            char simbolo = DISENO_BASE[r][c];
            TipoCeldaLaberinto tipo = CELDA_LABERINTO_LIBRE;

            if (simbolo == '#')
            {
                tipo = CELDA_LABERINTO_MURO;
            }
            else if (simbolo == 'o')
            {
                tipo = CELDA_LABERINTO_AGUJERO;
            }
            else if (simbolo == 'S')
            {
                m.inicioColumna = dc;
                m.inicioFila = dr;
            }
            else if (simbolo == 'M')
            {
                m.metaColumna = dc;
                m.metaFila = dr;
            }
            else if (simbolo == 'A')
            {
                m.controlColumna[0] = dc;
                m.controlFila[0] = dr;
            }
            else if (simbolo == 'B')
            {
                m.controlColumna[1] = dc;
                m.controlFila[1] = dr;
            }

            m.celdas[dr][dc] = tipo;
        }
    }

    m.cantidadTrampas = MAX_TRAMPAS_LABERINTO;

    for (int i = 0; i < MAX_TRAMPAS_LABERINTO; i++)
    {
        const TrampaBaseLaberinto& base = TRAMPAS_BASE[i];
        int ac = 0;
        int ar = 0;
        int bc = 0;
        int br = 0;
        MapearCeldaDiseno(diseno, base.c0, base.r0, ac, ar);
        MapearCeldaDiseno(diseno, base.c1, base.r1, bc, br);

        TrampaDardosLaberinto& trampa = m.trampas[i];
        trampa.columnaInicio = ac < bc ? ac : bc;
        trampa.columnaFin = ac < bc ? bc : ac;
        trampa.filaInicio = ar < br ? ar : br;
        trampa.filaFin = ar < br ? br : ar;
        MapearDireccionDiseno(diseno, base.dx, base.dz, trampa.direccionX, trampa.direccionZ);
        trampa.desfase = base.desfase;
        m.pulsoPrevio[i] = false;
    }

    CalcularDistanciasMeta(m);
}


//==================================================
// UTILIDADES
//==================================================

static int ColumnaDe(float x)
{
    int c = (int)std::floor(x);

    if (c < 0) c = 0;
    if (c >= COLUMNAS_LABERINTO) c = COLUMNAS_LABERINTO - 1;

    return c;
}


static int FilaDe(float z)
{
    int r = (int)std::floor(z);

    if (r < 0) r = 0;
    if (r >= FILAS_LABERINTO) r = FILAS_LABERINTO - 1;

    return r;
}


static float Acotar(float valor, float minimo, float maximo)
{
    if (valor < minimo) return minimo;
    if (valor > maximo) return maximo;
    return valor;
}


static float AleatorioSigno()
{
    return (float)GetRandomValue(-1000, 1000) / 1000.0f;
}


static bool EsMuro(const MinijuegoLaberintoInclinado& m, int c, int r)
{
    if (c < 0 || r < 0 || c >= COLUMNAS_LABERINTO || r >= FILAS_LABERINTO)
    {
        return true;
    }

    return m.celdas[r][c] == CELDA_LABERINTO_MURO;
}


static bool JugadorEsBot(const Participante& participante)
{
    return participante.esBot || !participante.conectado;
}


// 0 = reposo, 1 = aviso (se ve la senal roja), 2 = disparo.
static int EstadoTrampa(const TrampaDardosLaberinto& trampa, float tiempoJuego, float& avance)
{
    float t = std::fmod(tiempoJuego + trampa.desfase, PERIODO_TRAMPA);
    float inicioPulso = PERIODO_TRAMPA - DURACION_PULSO_TRAMPA;
    float inicioAviso = inicioPulso - DURACION_AVISO_TRAMPA;

    avance = 0.0f;

    if (t >= inicioPulso)
    {
        avance = (t - inicioPulso) / DURACION_PULSO_TRAMPA;
        return 2;
    }

    if (t >= inicioAviso)
    {
        avance = (t - inicioAviso) / DURACION_AVISO_TRAMPA;
        return 1;
    }

    return 0;
}


static bool EsferaEnTrampa(const TrampaDardosLaberinto& trampa, float x, float z)
{
    return
        x >= (float)trampa.columnaInicio &&
        x <= (float)(trampa.columnaFin + 1) &&
        z >= (float)trampa.filaInicio &&
        z <= (float)(trampa.filaFin + 1);
}


// Progreso por el camino: distancia recorrida desde la salida, con parte
// fraccionaria dentro de la celda para desempatar.
static float CalcularProgreso(const MinijuegoLaberintoInclinado& m, const EstadoJugadorLaberinto& e)
{
    if (e.llego)
    {
        return (float)m.distanciaInicio;
    }

    int c = ColumnaDe(e.x);
    int r = FilaDe(e.z);
    int d = m.distanciaMeta[r][c];

    if (d < 0)
    {
        return e.progreso;
    }

    float fraccion = 0.0f;
    const int saltoC[4] = { 1, -1, 0, 0 };
    const int saltoR[4] = { 0, 0, 1, -1 };

    for (int k = 0; k < 4 && d > 0; k++)
    {
        int nc = c + saltoC[k];
        int nr = r + saltoR[k];

        if (
            nc >= 0 && nr >= 0 && nc < COLUMNAS_LABERINTO && nr < FILAS_LABERINTO &&
            m.distanciaMeta[nr][nc] == d - 1
        )
        {
            float proyeccion =
                (e.x - ((float)c + 0.5f)) * (float)saltoC[k] +
                (e.z - ((float)r + 0.5f)) * (float)saltoR[k] + 0.5f;
            fraccion = Acotar(proyeccion, 0.0f, 1.0f);
            break;
        }
    }

    float progreso = (float)(m.distanciaInicio - d) + fraccion;

    return Acotar(progreso, 0.0f, (float)m.distanciaInicio);
}


static float PorcentajeProgreso(const MinijuegoLaberintoInclinado& m, const EstadoJugadorLaberinto& e)
{
    return 100.0f * Acotar(e.progreso / (float)m.distanciaInicio, 0.0f, 1.0f);
}


//==================================================
// FISICA DE LA ESFERA (2D EN EL PLANO DE LA LOSA)
//==================================================

// Resuelve choques circulo contra celdas-muro (rectangulos AABB). Devuelve la
// mayor velocidad de impacto contra un muro en esta llamada.
static float ResolverMurosEsfera(const MinijuegoLaberintoInclinado& m, EstadoJugadorLaberinto& e)
{
    float impactoMaximo = 0.0f;

    for (int pasada = 0; pasada < 2; pasada++)
    {
        int c0 = (int)std::floor(e.x) - 1;
        int r0 = (int)std::floor(e.z) - 1;

        for (int r = r0; r <= r0 + 2; r++)
        {
            for (int c = c0; c <= c0 + 2; c++)
            {
                if (!EsMuro(m, c, r))
                {
                    continue;
                }

                float cercanoX = Acotar(e.x, (float)c, (float)(c + 1));
                float cercanoZ = Acotar(e.z, (float)r, (float)(r + 1));
                float dx = e.x - cercanoX;
                float dz = e.z - cercanoZ;
                float distancia2 = dx * dx + dz * dz;

                if (distancia2 >= RADIO_ESFERA * RADIO_ESFERA)
                {
                    continue;
                }

                float nx = 0.0f;
                float nz = 0.0f;
                float penetracion = 0.0f;

                if (distancia2 > 0.000001f)
                {
                    float distancia = std::sqrt(distancia2);
                    nx = dx / distancia;
                    nz = dz / distancia;
                    penetracion = RADIO_ESFERA - distancia;
                }
                else
                {
                    // Centro dentro del muro: sale por el lado mas cercano.
                    float izquierda = e.x - (float)c;
                    float derecha = (float)(c + 1) - e.x;
                    float arriba = e.z - (float)r;
                    float abajo = (float)(r + 1) - e.z;
                    float menor = izquierda;
                    nx = -1.0f;

                    if (derecha < menor) { menor = derecha; nx = 1.0f; nz = 0.0f; }
                    if (arriba < menor) { menor = arriba; nx = 0.0f; nz = -1.0f; }
                    if (abajo < menor) { menor = abajo; nx = 0.0f; nz = 1.0f; }

                    penetracion = menor + RADIO_ESFERA;
                }

                e.x += nx * penetracion;
                e.z += nz * penetracion;

                float velocidadNormal = e.velocidadX * nx + e.velocidadZ * nz;

                if (velocidadNormal < 0.0f)
                {
                    if (-velocidadNormal > impactoMaximo)
                    {
                        impactoMaximo = -velocidadNormal;
                    }

                    float factor = (1.0f + REBOTE_ESFERA) * velocidadNormal;
                    e.velocidadX -= factor * nx;
                    e.velocidadZ -= factor * nz;
                }
            }
        }
    }

    return impactoMaximo;
}


static float MoverEsfera(
    const MinijuegoLaberintoInclinado& m,
    EstadoJugadorLaberinto& e,
    float deltaTime
)
{
    int subpasos = (int)std::ceil(deltaTime / 0.008f);

    if (subpasos < 1) subpasos = 1;
    if (subpasos > 10) subpasos = 10;

    float h = deltaTime / (float)subpasos;
    float impactoMaximo = 0.0f;

    for (int paso = 0; paso < subpasos; paso++)
    {
        float aceleracionX = e.inclinacionX * ACELERACION_MAXIMA;
        float aceleracionZ = e.inclinacionZ * ACELERACION_MAXIMA;

        for (int i = 0; i < m.cantidadTrampas; i++)
        {
            float avance = 0.0f;

            if (
                EstadoTrampa(m.trampas[i], m.tiempoJuego, avance) == 2 &&
                EsferaEnTrampa(m.trampas[i], e.x, e.z)
            )
            {
                aceleracionX += m.trampas[i].direccionX * ACELERACION_TRAMPA;
                aceleracionZ += m.trampas[i].direccionZ * ACELERACION_TRAMPA;
            }
        }

        e.velocidadX += aceleracionX * h;
        e.velocidadZ += aceleracionZ * h;

        float amortiguacion = 1.0f - FRICCION_ESFERA * h;

        if (amortiguacion < 0.0f) amortiguacion = 0.0f;

        e.velocidadX *= amortiguacion;
        e.velocidadZ *= amortiguacion;

        float velocidad = std::sqrt(e.velocidadX * e.velocidadX + e.velocidadZ * e.velocidadZ);

        if (velocidad > VELOCIDAD_MAXIMA_EMPUJE)
        {
            float escala = VELOCIDAD_MAXIMA_EMPUJE / velocidad;
            e.velocidadX *= escala;
            e.velocidadZ *= escala;
        }

        e.x += e.velocidadX * h;
        e.z += e.velocidadZ * h;

        float impacto = ResolverMurosEsfera(m, e);

        if (impacto > impactoMaximo)
        {
            impactoMaximo = impacto;
        }
    }

    return impactoMaximo;
}


// Devuelve true si el centro de la esfera esta sobre un agujero.
static bool BuscarAgujero(
    const MinijuegoLaberintoInclinado& m,
    const EstadoJugadorLaberinto& e,
    float& centroX,
    float& centroZ
)
{
    int c0 = (int)std::floor(e.x) - 1;
    int r0 = (int)std::floor(e.z) - 1;

    for (int r = r0; r <= r0 + 2; r++)
    {
        for (int c = c0; c <= c0 + 2; c++)
        {
            if (
                c < 0 || r < 0 || c >= COLUMNAS_LABERINTO || r >= FILAS_LABERINTO ||
                m.celdas[r][c] != CELDA_LABERINTO_AGUJERO
            )
            {
                continue;
            }

            float dx = e.x - ((float)c + 0.5f);
            float dz = e.z - ((float)r + 0.5f);

            if (dx * dx + dz * dz < RADIO_AGUJERO * RADIO_AGUJERO)
            {
                centroX = (float)c + 0.5f;
                centroZ = (float)r + 0.5f;
                return true;
            }
        }
    }

    return false;
}


//==================================================
// IA DE BOTS
//==================================================
//
// El bot calcula, con el campo de distancias hacia la meta, el siguiente
// tramo recto del recorrido (centro de pasillos, sin pasar por agujeros) y
// regula su velocidad deseada: rapido en recta, frenando antes de la esquina.
// Hay imprecision lateral y una sacudida si no avanza durante 3 segundos.
//==================================================

static void CalcularInclinacionBot(
    const MinijuegoLaberintoInclinado& m,
    EstadoJugadorLaberinto& e,
    float deltaTime,
    float& salidaX,
    float& salidaZ
)
{
    salidaX = 0.0f;
    salidaZ = 0.0f;

    e.tiempoRuido -= deltaTime;

    if (e.tiempoRuido <= 0.0f)
    {
        float amplitud = 0.16f * (1.45f - e.habilidad);
        e.tiempoRuido = 0.5f + 0.1f * (float)GetRandomValue(0, 6);
        e.ruidoX = AleatorioSigno() * amplitud;
        e.ruidoZ = AleatorioSigno() * amplitud;
    }

    if (e.tiempoSacudida > 0.0f)
    {
        e.tiempoSacudida -= deltaTime;
        salidaX = e.sacudidaX;
        salidaZ = e.sacudidaZ;
        return;
    }

    int c = ColumnaDe(e.x);
    int r = FilaDe(e.z);
    int actual = m.distanciaMeta[r][c];

    const int saltoC[4] = { 1, -1, 0, 0 };
    const int saltoR[4] = { 0, 0, 1, -1 };
    int mejor = -1;
    int mejorDistancia = 100000;

    for (int k = 0; k < 4; k++)
    {
        int nc = c + saltoC[k];
        int nr = r + saltoR[k];

        if (nc < 0 || nr < 0 || nc >= COLUMNAS_LABERINTO || nr >= FILAS_LABERINTO)
        {
            continue;
        }

        int d = m.distanciaMeta[nr][nc];

        if (d >= 0 && d < mejorDistancia)
        {
            mejorDistancia = d;
            mejor = k;
        }
    }

    int dirC = 0;
    int dirR = 0;
    int destinoC = c;
    int destinoR = r;

    if (mejor >= 0 && (actual < 0 || mejorDistancia < actual))
    {
        dirC = saltoC[mejor];
        dirR = saltoR[mejor];
        destinoC = c + dirC;
        destinoR = r + dirR;

        // Extiende el tramo recto hasta 3 celdas mientras siga la ruta.
        for (int extra = 0; extra < 3; extra++)
        {
            int siguienteC = destinoC + dirC;
            int siguienteR = destinoR + dirR;

            if (
                siguienteC < 0 || siguienteR < 0 ||
                siguienteC >= COLUMNAS_LABERINTO || siguienteR >= FILAS_LABERINTO
            )
            {
                break;
            }

            if (
                m.distanciaMeta[siguienteR][siguienteC] < 0 ||
                m.distanciaMeta[siguienteR][siguienteC] !=
                    m.distanciaMeta[destinoR][destinoC] - 1
            )
            {
                break;
            }

            destinoC = siguienteC;
            destinoR = siguienteR;
        }
    }

    float objetivoX = (float)destinoC + 0.5f;
    float objetivoZ = (float)destinoR + 0.5f;
    float velocidadDeseadaX = 0.0f;
    float velocidadDeseadaZ = 0.0f;

    if (dirC == 0 && dirR == 0)
    {
        velocidadDeseadaX = Acotar((objetivoX - e.x) * 2.0f, -1.5f, 1.5f);
        velocidadDeseadaZ = Acotar((objetivoZ - e.z) * 2.0f, -1.5f, 1.5f);
    }
    else
    {
        float restante = (objetivoX - e.x) * (float)dirC + (objetivoZ - e.z) * (float)dirR;
        float velocidadMaxima = 3.3f * e.habilidad;
        float velocidadAvance = 0.0f;

        if (restante > 0.0f)
        {
            velocidadAvance = 2.6f * std::sqrt(restante);

            if (velocidadAvance > velocidadMaxima) velocidadAvance = velocidadMaxima;
            if (velocidadAvance < 0.9f && restante > 0.1f) velocidadAvance = 0.9f;
        }

        // Correccion lateral hacia el centro del pasillo, con imprecision.
        float lateralX = dirC == 0 ? 1.0f : 0.0f;
        float lateralZ = dirR == 0 ? 1.0f : 0.0f;
        float errorLateral =
            lateralX * (objetivoX - e.x + e.ruidoX) +
            lateralZ * (objetivoZ - e.z + e.ruidoZ);
        float velocidadLateral = Acotar(errorLateral * 3.0f, -1.6f, 1.6f);

        velocidadDeseadaX = (float)dirC * velocidadAvance + lateralX * velocidadLateral;
        velocidadDeseadaZ = (float)dirR * velocidadAvance + lateralZ * velocidadLateral;
    }

    salidaX = Acotar((velocidadDeseadaX - e.velocidadX) * 1.3f, -1.0f, 1.0f);
    salidaZ = Acotar((velocidadDeseadaZ - e.velocidadZ) * 1.3f, -1.0f, 1.0f);
}


// Antiatasco: si el bot no mejora su progreso en 3 s, sacude y se centra.
static void VigilarAtascoBot(EstadoJugadorLaberinto& e, float deltaTime)
{
    if (e.progreso > e.mejorProgresoBot + 0.04f)
    {
        e.mejorProgresoBot = e.progreso;
        e.tiempoSinProgreso = 0.0f;
        return;
    }

    e.tiempoSinProgreso += deltaTime;

    if (e.tiempoSinProgreso > 3.0f)
    {
        e.tiempoSinProgreso = 1.5f;
        e.tiempoSacudida = 0.5f;
        e.sacudidaX = AleatorioSigno();
        e.sacudidaZ = AleatorioSigno();

        float centroX = std::floor(e.x) + 0.5f;
        float centroZ = std::floor(e.z) + 0.5f;
        e.x += (centroX - e.x) * 0.5f;
        e.z += (centroZ - e.z) * 0.5f;
        e.velocidadX = 0.0f;
        e.velocidadZ = 0.0f;
    }
}


//==================================================
// RESULTADO
//==================================================

// true si el jugador a "va por delante" del jugador b.
static bool VaPorDelante(
    const EstadoJugadorLaberinto& a,
    const EstadoJugadorLaberinto& b
)
{
    if (a.llego && !b.llego) return true;
    if (!a.llego && b.llego) return false;

    if (a.llego && b.llego)
    {
        return a.tiempoLlegada < b.tiempoLlegada - 0.0001f;
    }

    return a.progreso > b.progreso + 0.0001f;
}


static void FinalizarLaberinto(MinijuegoLaberintoInclinado& m)
{
    if (m.resultado.estado == RESULTADO_MINIJUEGO_FINALIZADO)
    {
        return;
    }

    int cantidadPrimeros = 0;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        ResultadoParticipante& resultadoJugador = m.resultado.participantes[i];

        if (!resultadoJugador.participo)
        {
            continue;
        }

        int posicion = 1;

        for (int j = 0; j < MAX_PARTICIPANTES; j++)
        {
            if (
                j != i &&
                m.resultado.participantes[j].participo &&
                VaPorDelante(m.estadosJugadores[j], m.estadosJugadores[i])
            )
            {
                posicion++;
            }
        }

        resultadoJugador.posicionFinal = posicion;
        resultadoJugador.numeroEquipo = -1;
        resultadoJugador.puntuacionMinijuego =
            m.estadosJugadores[i].llego
            ? 100
            : (int)PorcentajeProgreso(m, m.estadosJugadores[i]);
        resultadoJugador.puntosObtenidos = 0;

        if (posicion == 1)
        {
            cantidadPrimeros++;
        }
    }

    m.resultado.estado = RESULTADO_MINIJUEGO_FINALIZADO;
    m.resultado.desenlace =
        cantidadPrimeros == 1 ? DESENLACE_CON_GANADOR : DESENLACE_EMPATE;
    m.fase = FASE_LABERINTO_TERMINADO;
    ReproducirSonidoMinijuego(m.audio, SONIDO_RESULTADO);
}


//==================================================
// INICIALIZACION Y REINICIO
//==================================================

static void ConfigurarCamara(MinijuegoLaberintoInclinado& m)
{
    m.camara.up = { 0.0f, 1.0f, 0.0f };
    m.camara.projection = CAMERA_PERSPECTIVE;
    m.camara.fovy = 45.0f;

    // Algo menos cenital que una vista en planta: las losas quedan bajo el
    // tendedero del HUD y alrededor se ve la camara del templo.
    if (m.cantidadTableros <= 2)
    {
        m.camara.position = { 0.0f, 24.0f, 15.0f };
        m.camara.target = { 0.0f, 0.0f, 1.2f };
    }
    else
    {
        m.camara.position = { 0.0f, 33.0f, 19.0f };
        m.camara.target = { 0.0f, 0.0f, 1.6f };
    }
}


static void ColocarTablero(MinijuegoLaberintoInclinado& m, int slot, int total)
{
    const float separacion = 7.0f;

    if (total <= 2)
    {
        m.centrosTableros[slot] = { slot == 0 ? -separacion : separacion, 0.0f, 0.0f };
        return;
    }

    float x = (slot % 2 == 0) ? -separacion : separacion;
    float z = (slot < 2) ? -separacion : separacion;

    // Con 3 jugadores el tercero queda centrado en la fila de abajo.
    if (total == 3 && slot == 2)
    {
        x = 0.0f;
    }

    m.centrosTableros[slot] = { x, 0.0f, z };
}


void MinijuegoLaberintoInclinado::Inicializar()
{
    // El diseno anterior se conserva para no repetirlo dos veces seguidas.
    int disenoPrevio = disenoActual;

    resultado = {};
    resultado.formato = FORMATO_MINIJUEGO_INDIVIDUAL;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        estadosJugadores[i] = {};
        centrosTableros[i] = {};
    }

    for (int r = 0; r < FILAS_LABERINTO; r++)
    {
        for (int c = 0; c < COLUMNAS_LABERINTO; c++)
        {
            celdas[r][c] = CELDA_LABERINTO_MURO;
            distanciaMeta[r][c] = -1;
        }
    }

    cantidadTableros = 0;
    cantidadTrampas = 0;
    llegadas = 0;
    plazoAcortado = false;
    ultimoDiseno = disenoPrevio;

    fase = FASE_LABERINTO_PREPARACION;
    tiempoPreparacion = DURACION_PREPARACION_LABERINTO;
    tiempoRestante = DURACION_PARTIDA_LABERINTO;
    tiempoJuego = 0.0f;
    tiempoAnimacion = 0.0f;
    enfriamientoImpacto = 0.0f;

    for (int i = 0; i < MAX_PARTICULAS_LABERINTO; i++)
    {
        particulas[i] = {};
    }

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        visuales[i] = {};
    }

    for (int i = 0; i < MAX_TRAMPAS_LABERINTO; i++)
    {
        pulsoVisualPrevio[i] = false;
    }

    tiempoAnuncio = -1.0f;
    jugadorAnuncio = -1;
    anuncioConPlazo = false;
    confetiLanzado = false;

    // Elige un diseno distinto al anterior.
    int nuevo = GetRandomValue(0, CANTIDAD_DISENOS_LABERINTO - 1);

    if (nuevo == disenoPrevio)
    {
        nuevo = (nuevo + 1) % CANTIDAD_DISENOS_LABERINTO;
    }

    disenoActual = nuevo;
    ConstruirDisenoLaberinto(*this, disenoActual);
    ConfigurarCamara(*this);
}


void MinijuegoLaberintoInclinado::Reiniciar(
    JugadorPrueba jugadores[],
    Participante participantes[],
    int cantidadMaxima
)
{
    Inicializar();
    CargarPaqueteModelosEscenarioRetro3D(ObtenerPaqueteLaberintoJadeRetro3D());
    InicializarResultadoMinijuego(
        resultado,
        participantes,
        FORMATO_MINIJUEGO_INDIVIDUAL
    );

    int limite = cantidadMaxima < MAX_PARTICIPANTES
        ? cantidadMaxima
        : MAX_PARTICIPANTES;

    int total = 0;

    for (int i = 0; i < limite; i++)
    {
        if (participantes[i].activo)
        {
            total++;
        }
    }

    if (total < 2)
    {
        resultado.estado = RESULTADO_MINIJUEGO_CANCELADO;
        fase = FASE_LABERINTO_TERMINADO;
        return;
    }

    cantidadTableros = total;
    ConfigurarCamara(*this);

    int slot = 0;

    for (int i = 0; i < limite; i++)
    {
        if (!participantes[i].activo)
        {
            continue;
        }

        ColocarTablero(*this, slot, total);

        EstadoJugadorLaberinto& e = estadosJugadores[i];
        e.tablero = slot;
        e.x = (float)inicioColumna + 0.5f;
        e.z = (float)inicioFila + 0.5f;
        e.puntoRespawnX = e.x;
        e.puntoRespawnZ = e.z;
        e.habilidad = 0.6f + 0.35f * (float)GetRandomValue(0, 100) / 100.0f;
        e.tiempoRuido = 0.0f;
        e.progreso = 0.0f;
        e.mejorProgresoBot = 0.0f;

        jugadores[i].posicion = centrosTableros[slot];
        jugadores[i].posicionSpawn = centrosTableros[slot];

        slot++;
    }
}


//==================================================
// ACTUALIZACION
//==================================================

// Particulas, destellos y anuncios (definida con el dibujo, al final).
static void ActualizarVisualLaberinto(
    MinijuegoLaberintoInclinado& m,
    float deltaTime,
    const Participante participantes[],
    int cantidadMaxima
);


static void ReaparecerEsfera(EstadoJugadorLaberinto& e)
{
    e.x = e.puntoRespawnX;
    e.z = e.puntoRespawnZ;
    e.velocidadX = 0.0f;
    e.velocidadZ = 0.0f;
    e.tiempoCaida = 0.0f;
    e.mejorProgresoBot = 0.0f;
    e.tiempoSinProgreso = 0.0f;
}


void MinijuegoLaberintoInclinado::Actualizar(
    float deltaTime,
    JugadorPrueba jugadores[],
    int cantidadMaxima,
    Participante participantes[]
)
{
    (void)jugadores;

    // Efectos del frame anterior (solo visual; sigue animando en el final).
    ActualizarVisualLaberinto(
        *this,
        deltaTime > 0.05f ? 0.05f : deltaTime,
        participantes,
        cantidadMaxima
    );

    if (resultado.estado != RESULTADO_MINIJUEGO_EN_CURSO)
    {
        return;
    }

    if (deltaTime > 0.05f)
    {
        deltaTime = 0.05f;
    }

    int limite = cantidadMaxima < MAX_PARTICIPANTES
        ? cantidadMaxima
        : MAX_PARTICIPANTES;

    tiempoAnimacion += deltaTime;

    if (fase == FASE_LABERINTO_PREPARACION)
    {
        float preparacionAntes = tiempoPreparacion;
        tiempoPreparacion -= deltaTime;
        ActualizarAudioCuentaRegresiva(audio, preparacionAntes, tiempoPreparacion);

        if (tiempoPreparacion <= 0.0f)
        {
            tiempoPreparacion = 0.0f;
            fase = FASE_LABERINTO_JUGANDO;
        }

        return;
    }

    if (fase != FASE_LABERINTO_JUGANDO)
    {
        return;
    }

    float restanteAntes = tiempoRestante;
    tiempoRestante -= deltaTime;
    ActualizarAudioAlertaTiempo(audio, restanteAntes, tiempoRestante);
    tiempoJuego += deltaTime;
    enfriamientoImpacto -= deltaTime;

    if (tiempoRestante < 0.0f)
    {
        tiempoRestante = 0.0f;
    }

    bool humanoEnTrampa[MAX_TRAMPAS_LABERINTO]{};

    for (int i = 0; i < limite; i++)
    {
        if (!resultado.participantes[i].participo)
        {
            continue;
        }

        EstadoJugadorLaberinto& e = estadosJugadores[i];
        bool esBot = JugadorEsBot(participantes[i]);

        // Esfera ya en la meta: queda quieta sobre el altar.
        if (e.llego)
        {
            e.objetivoX = 0.0f;
            e.objetivoZ = 0.0f;
            e.inclinacionX += (0.0f - e.inclinacionX) * Acotar(deltaTime * 6.0f, 0.0f, 1.0f);
            e.inclinacionZ += (0.0f - e.inclinacionZ) * Acotar(deltaTime * 6.0f, 0.0f, 1.0f);
            continue;
        }

        // Cayendo por un agujero: sin control hasta reaparecer.
        if (e.tiempoCaida > 0.0f)
        {
            e.tiempoCaida -= deltaTime;
            e.x += (e.agujeroX - e.x) * Acotar(deltaTime * 8.0f, 0.0f, 1.0f);
            e.z += (e.agujeroZ - e.z) * Acotar(deltaTime * 8.0f, 0.0f, 1.0f);

            if (e.tiempoCaida <= 0.0f)
            {
                ReaparecerEsfera(e);
                e.progreso = CalcularProgreso(*this, e);
            }

            continue;
        }

        // Inclinacion deseada: humano (stick/teclas) o bot (IA).
        float deseadoX = 0.0f;
        float deseadoZ = 0.0f;

        if (esBot)
        {
            CalcularInclinacionBot(*this, e, deltaTime, deseadoX, deseadoZ);
        }
        else
        {
            InputJugador entrada = LeerInputParticipante(participantes[i]);
            deseadoX = entrada.moverX;
            deseadoZ = entrada.moverZ;

            float longitud = std::sqrt(deseadoX * deseadoX + deseadoZ * deseadoZ);

            if (longitud > 1.0f)
            {
                deseadoX /= longitud;
                deseadoZ /= longitud;
            }
        }

        e.objetivoX = deseadoX;
        e.objetivoZ = deseadoZ;

        float suavizado = Acotar(deltaTime * 9.0f, 0.0f, 1.0f);
        e.inclinacionX += (e.objetivoX - e.inclinacionX) * suavizado;
        e.inclinacionZ += (e.objetivoZ - e.inclinacionZ) * suavizado;

        float impacto = MoverEsfera(*this, e, deltaTime);

        if (impacto > IMPACTO_FUERTE && enfriamientoImpacto <= 0.0f && !esBot)
        {
            enfriamientoImpacto = 0.18f;
            ReproducirSonidoMinijuego(audio, SONIDO_IMPACTO);
        }

        // Agujeros: vuelve al ultimo checkpoint.
        float centroAgujeroX = 0.0f;
        float centroAgujeroZ = 0.0f;

        if (BuscarAgujero(*this, e, centroAgujeroX, centroAgujeroZ))
        {
            e.tiempoCaida = DURACION_CAIDA;
            e.agujeroX = centroAgujeroX;
            e.agujeroZ = centroAgujeroZ;
            e.velocidadX = 0.0f;
            e.velocidadZ = 0.0f;
            e.caidas++;

            if (!esBot)
            {
                ReproducirSonidoMinijuego(audio, SONIDO_CAIDA);
            }

            continue;
        }

        // Checkpoints en orden.
        if (e.puntosControl < MAX_PUNTOS_CONTROL_LABERINTO)
        {
            float cx = (float)controlColumna[e.puntosControl] + 0.5f;
            float cz = (float)controlFila[e.puntosControl] + 0.5f;
            float dx = e.x - cx;
            float dz = e.z - cz;

            if (dx * dx + dz * dz < 0.36f)
            {
                e.puntosControl++;
                e.puntoRespawnX = cx;
                e.puntoRespawnZ = cz;
                ReproducirSonidoMinijuego(audio, SONIDO_ACIERTO);
            }
        }

        // Meta: al entrar en el altar la esfera queda fijada y se registra la llegada.
        float mx = (float)metaColumna + 0.5f;
        float mz = (float)metaFila + 0.5f;
        float mdx = e.x - mx;
        float mdz = e.z - mz;

        if (mdx * mdx + mdz * mdz < RADIO_META * RADIO_META)
        {
            e.llego = true;
            e.tiempoLlegada = tiempoJuego;
            e.velocidadX = 0.0f;
            e.velocidadZ = 0.0f;
            llegadas++;
            ReproducirSonidoMinijuego(audio, SONIDO_RECOGER_OBJETO);

            if (!plazoAcortado)
            {
                plazoAcortado = true;

                if (tiempoRestante > PLAZO_TRAS_PRIMERA_LLEGADA)
                {
                    tiempoRestante = PLAZO_TRAS_PRIMERA_LLEGADA;
                }
            }
        }

        e.progreso = CalcularProgreso(*this, e);

        if (esBot && !e.llego)
        {
            VigilarAtascoBot(e, deltaTime);
        }

        for (int t = 0; t < cantidadTrampas; t++)
        {
            if (!esBot && EsferaEnTrampa(trampas[t], e.x, e.z))
            {
                humanoEnTrampa[t] = true;
            }
        }
    }

    // Sonido del disparo una sola vez por descarga, si afecta a un humano.
    for (int t = 0; t < cantidadTrampas; t++)
    {
        float avance = 0.0f;
        bool pulso = EstadoTrampa(trampas[t], tiempoJuego, avance) == 2;

        if (pulso && !pulsoPrevio[t] && humanoEnTrampa[t])
        {
            ReproducirSonidoMinijuego(audio, SONIDO_DISPARO);
        }

        pulsoPrevio[t] = pulso;
    }

    int participantesTotales = 0;

    for (int i = 0; i < limite; i++)
    {
        if (resultado.participantes[i].participo)
        {
            participantesTotales++;
        }
    }

    if (llegadas >= participantesTotales || tiempoRestante <= 0.0f)
    {
        FinalizarLaberinto(*this);
    }
}


//==================================================
// VISUAL: CAMARA DEL TEMPLO DE JADE
//==================================================
//
// Capas, de adelante hacia atras:
//  1. Las losas inclinables (piezas GLB del paquete, misma cantidad y
//     posicion) sobre pedestales tallados en un estanque sagrado.
//  2. El borde del estanque con sus operadores y mandos, braseros,
//     serpientes que vierten agua, estatuas, jaguares, vasijas, estelas
//     y ofrendas.
//  3. Columnas con relieves, muros de sillares con pilastras, friso de
//     glifos y hornacinas, y el altar del idolo de jade.
//  4. Las hornacinas en penumbra y el casco del templo (GLB) detras.
//  5. Techo en sombra con incensarios colgados y un haz de luz del oculo.
// La niebla verde oscura funde lo lejano y lo hondo. Todo el azar visual
// sale de un hash o de semillaVisual, nunca de GetRandomValue.
//==================================================

static const float NIVEL_SUELO_JADE = -1.0f;
static const float NIVEL_AGUA_JADE = -3.0f;
static const float FONDO_ESTANQUE_JADE = -4.4f;
static const float MITAD_ESTANQUE_X_JADE = 14.6f;
static const float ANCHO_BORDE_JADE = 0.8f;
static const float MURO_LATERAL_JADE = 23.0f;
static const float ALTO_CAMARA_JADE = 14.0f;
static const float DURACION_ANUNCIO_JADE = 2.6f;
static const float DURACION_TEXTO_JADE = 1.2f;

static const Color COLOR_NIEBLA_JADE = { 14, 34, 30, 255 };
static const Color COLOR_LOSA_PISO_JADE = { 128, 126, 104, 255 };
static const Color COLOR_JUNTA_JADE = { 52, 54, 44, 255 };
static const Color COLOR_MURO_CAMARA_JADE = { 116, 116, 96, 255 };
static const Color COLOR_TALLA_JADE = { 158, 152, 124, 255 };
static const Color COLOR_JADE_OSCURO = { 30, 110, 86, 255 };
static const Color COLOR_JADE_CLARO = { 130, 236, 186, 255 };
static const Color COLOR_BRONCE_JADE = { 158, 110, 56, 255 };
static const Color COLOR_TERRACOTA_JADE = { 164, 88, 58, 255 };
static const Color COLOR_AGUA_JADE = { 24, 106, 92, 255 };
static const Color COLOR_ESTATUA_JADE = { 112, 124, 106, 255 };

// Plano de los halos: perpendicular a la vista (se recalcula al dibujar).
static Vector3 vFrenteJade = { 0.0f, 0.54f, -0.84f };


//==================================================
// UTILIDADES DE DIBUJO
//==================================================

static float Limitar01Jade(float v)
{
    if (v < 0.0f) return 0.0f;
    if (v > 1.0f) return 1.0f;
    return v;
}


// Hash determinista para la decoracion (misma escena en cada partida).
static float Hash01Jade(int a, int b)
{
    unsigned int h = (unsigned int)a * 374761393u + (unsigned int)b * 668265263u;
    h = (h ^ (h >> 13)) * 1274126177u;
    h ^= h >> 16;
    return (float)(h & 0xFFFFu) / 65535.0f;
}


// Azar propio de las particulas: no consume el GetRandomValue de la partida.
static float AzarJade(MinijuegoLaberintoInclinado& m, float minimo, float maximo)
{
    unsigned int& s = m.semillaVisual;
    s ^= s << 13;
    s ^= s >> 17;
    s ^= s << 5;
    return minimo + (maximo - minimo) * (float)(s & 0xFFFFFFu) / 16777215.0f;
}


static unsigned char CanalJade(float v)
{
    if (v < 0.0f) return 0;
    if (v > 255.0f) return 255;
    return (unsigned char)v;
}


static Color TonoJade(Color c, float f)
{
    return Color{ CanalJade((float)c.r * f), CanalJade((float)c.g * f), CanalJade((float)c.b * f), c.a };
}


static Color MezclaJade(Color a, Color b, float t)
{
    t = Limitar01Jade(t);

    return Color{
        CanalJade((float)a.r + ((float)b.r - (float)a.r) * t),
        CanalJade((float)a.g + ((float)b.g - (float)a.g) * t),
        CanalJade((float)a.b + ((float)b.b - (float)a.b) * t),
        CanalJade((float)a.a + ((float)b.a - (float)a.a) * t)
    };
}


static Color ConAlfaJade(Color c, float alfa)
{
    c.a = CanalJade(255.0f * alfa);
    return c;
}


// Con dos losas el estanque es una sola fila; con tres o cuatro, dos.
static float MitadEstanqueZJade(int tableros)
{
    return tableros <= 2 ? 7.6f : 14.6f;
}


// Penumbra del templo: lo lejano del centro, lo hondo del estanque y lo
// alto (el techo) se funden con el verde oscuro del aire.
static float CantidadNieblaJade(Vector3 p)
{
    float d = std::sqrt(p.x * p.x + p.z * p.z);
    float n = (d - 17.0f) / 22.0f;

    if (p.y < -1.3f) n += (-1.3f - p.y) * 0.09f;
    if (p.y > 4.0f) n += (p.y - 4.0f) * 0.065f;

    if (n < 0.0f) return 0.0f;
    return n > 0.86f ? 0.86f : n;
}


static Color TinteJade(Color c, Vector3 p)
{
    Color niebla = COLOR_NIEBLA_JADE;
    niebla.a = c.a;
    return MezclaJade(c, niebla, CantidadNieblaJade(p));
}


static void VerticeJade(Vector3 p, Color c)
{
    rlColor4ub(c.r, c.g, c.b, c.a);
    rlVertex3f(p.x, p.y, p.z);
}


static void QuadJade(Vector3 a, Vector3 b, Vector3 c, Vector3 d, Color color)
{
    VerticeJade(a, color);
    VerticeJade(b, color);
    VerticeJade(c, color);
    VerticeJade(d, color);
}


// Quad con la niebla por vertice (pisos y muros grandes).
static void QuadNieblaJade(Vector3 a, Vector3 b, Vector3 c, Vector3 d, Color color)
{
    VerticeJade(a, TinteJade(color, a));
    VerticeJade(b, TinteJade(color, b));
    VerticeJade(c, TinteJade(color, c));
    VerticeJade(d, TinteJade(color, d));
}


// Caja con caras sombreadas (arriba 1.0, frente 0.78, izquierda 0.70,
// derecha 0.62, fondo 0.56, abajo 0.45). Respeta la matriz actual.
static void CuboJade(Vector3 c, Vector3 t, Color color)
{
    float x0 = c.x - t.x * 0.5f, x1 = c.x + t.x * 0.5f;
    float y0 = c.y - t.y * 0.5f, y1 = c.y + t.y * 0.5f;
    float z0 = c.z - t.z * 0.5f, z1 = c.z + t.z * 0.5f;

    rlBegin(RL_QUADS);
    QuadJade({ x0, y1, z1 }, { x1, y1, z1 }, { x1, y1, z0 }, { x0, y1, z0 }, color);
    QuadJade({ x0, y0, z1 }, { x1, y0, z1 }, { x1, y1, z1 }, { x0, y1, z1 }, TonoJade(color, 0.78f));
    QuadJade({ x1, y0, z0 }, { x0, y0, z0 }, { x0, y1, z0 }, { x1, y1, z0 }, TonoJade(color, 0.56f));
    QuadJade({ x1, y0, z1 }, { x1, y0, z0 }, { x1, y1, z0 }, { x1, y1, z1 }, TonoJade(color, 0.62f));
    QuadJade({ x0, y0, z0 }, { x0, y0, z1 }, { x0, y1, z1 }, { x0, y1, z0 }, TonoJade(color, 0.70f));
    QuadJade({ x0, y0, z0 }, { x1, y0, z0 }, { x1, y0, z1 }, { x0, y0, z1 }, TonoJade(color, 0.45f));
    rlEnd();
}


static void CuboNieblaJade(Vector3 c, Vector3 t, Color color)
{
    CuboJade(c, t, TinteJade(color, c));
}


// Luz de las caras laterales segun su orientacion (angulo en XZ).
static float LuzLateralJade(float angulo)
{
    return 0.70f + 0.13f * std::sin(angulo) - 0.07f * std::cos(angulo + 0.6f);
}


// Prisma o tronco de cono de base.y (radio r0) a base.y + alto (r1).
static void PrismaJade(
    Vector3 base,
    float r0,
    float r1,
    float alto,
    int lados,
    Color color,
    float giro = 0.0f
)
{
    float yArriba = base.y + alto;
    float paso = 6.2832f / (float)lados;
    Color tapaArriba = color;
    Color tapaAbajo = TonoJade(color, 0.45f);

    rlBegin(RL_QUADS);

    for (int k = 0; k < lados; k++)
    {
        float a0 = giro + paso * (float)k;
        float a1 = a0 + paso;
        Color cara = TonoJade(color, LuzLateralJade(a0 + paso * 0.5f));
        float c0 = std::cos(a0), s0 = std::sin(a0);
        float c1 = std::cos(a1), s1 = std::sin(a1);

        QuadJade(
            { base.x + c0 * r0, base.y, base.z + s0 * r0 },
            { base.x + c1 * r0, base.y, base.z + s1 * r0 },
            { base.x + c1 * r1, yArriba, base.z + s1 * r1 },
            { base.x + c0 * r1, yArriba, base.z + s0 * r1 },
            cara
        );

        if (r1 > 0.001f)
        {
            Vector3 centro = { base.x, yArriba, base.z };
            QuadJade(centro, { base.x + c1 * r1, yArriba, base.z + s1 * r1 }, { base.x + c0 * r1, yArriba, base.z + s0 * r1 }, centro, tapaArriba);
        }

        if (r0 > 0.001f)
        {
            Vector3 centro = { base.x, base.y, base.z };
            QuadJade(centro, { base.x + c0 * r0, base.y, base.z + s0 * r0 }, { base.x + c1 * r0, base.y, base.z + s1 * r0 }, centro, tapaAbajo);
        }
    }

    rlEnd();
}


// Corona plana (r0..r1) a la altura c.y.
static void AnilloJade(Vector3 c, float r0, float r1, Color color, int lados = 24)
{
    rlBegin(RL_QUADS);

    for (int k = 0; k < lados; k++)
    {
        float a0 = 6.2832f * (float)k / (float)lados;
        float a1 = 6.2832f * (float)(k + 1) / (float)lados;

        QuadJade(
            { c.x + std::cos(a0) * r1, c.y, c.z + std::sin(a0) * r1 },
            { c.x + std::cos(a0) * r0, c.y, c.z + std::sin(a0) * r0 },
            { c.x + std::cos(a1) * r0, c.y, c.z + std::sin(a1) * r0 },
            { c.x + std::cos(a1) * r1, c.y, c.z + std::sin(a1) * r1 },
            color
        );
    }

    rlEnd();
}


static void DiscoJade(Vector3 c, float r, Color color, int lados = 20)
{
    AnilloJade(c, 0.0f, r, color, lados);
}


// Gema tallada (dos piramides): pivotes, ojos, joyas de las ofrendas.
static void GemaJade(Vector3 c, float r, float alto, Color color, int lados = 6)
{
    PrismaJade({ c.x, c.y - alto, c.z }, 0.0f, r, alto, lados, TonoJade(color, 0.86f));
    PrismaJade(c, r, 0.0f, alto, lados, color);
}


// Disco con borde que se apaga (u y v definen el plano).
static void ResplandorJade(Vector3 centro, Vector3 u, Vector3 v, float radio, Color color)
{
    const int LADOS = 18;
    rlBegin(RL_TRIANGLES);

    for (int k = 0; k < LADOS; k++)
    {
        float a0 = 6.2832f * (float)k / (float)LADOS;
        float a1 = 6.2832f * (float)(k + 1) / (float)LADOS;
        float c0 = std::cos(a0) * radio, s0 = std::sin(a0) * radio;
        float c1 = std::cos(a1) * radio, s1 = std::sin(a1) * radio;

        rlColor4ub(color.r, color.g, color.b, color.a);
        rlVertex3f(centro.x, centro.y, centro.z);
        rlColor4ub(color.r, color.g, color.b, 0);
        rlVertex3f(centro.x + u.x * c0 + v.x * s0, centro.y + u.y * c0 + v.y * s0, centro.z + u.z * c0 + v.z * s0);
        rlColor4ub(color.r, color.g, color.b, 0);
        rlVertex3f(centro.x + u.x * c1 + v.x * s1, centro.y + u.y * c1 + v.y * s1, centro.z + u.z * c1 + v.z * s1);
    }

    rlEnd();
}


static void ResplandorSueloJade(Vector3 c, float radio, Color color)
{
    ResplandorJade(c, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, radio, color);
}


static void ResplandorFrenteJade(Vector3 c, float radio, Color color)
{
    ResplandorJade(c, { 1.0f, 0.0f, 0.0f }, vFrenteJade, radio, color);
}


// Charco de luz en el piso: discos concentricos muy tenues.
static void CharcoLuzJade(Vector3 c, float radio, Color color)
{
    for (int r = 0; r < 3; r++)
    {
        ResplandorSueloJade({ c.x, c.y + 0.002f * (float)r, c.z }, radio * (0.5f + 0.32f * (float)r), color);
    }
}


// Cono de luz de "arriba" (radio r0, transparente) a "abajo" (r1, color).
static void ConoLuzJade(Vector3 arriba, Vector3 abajo, float r0, float r1, Color color)
{
    const int LADOS = 14;
    Vector3 eje = { abajo.x - arriba.x, abajo.y - arriba.y, abajo.z - arriba.z };
    float largo = std::sqrt(eje.x * eje.x + eje.y * eje.y + eje.z * eje.z);
    if (largo < 0.001f) return;
    eje = { eje.x / largo, eje.y / largo, eje.z / largo };

    Vector3 ref = std::fabs(eje.y) < 0.9f ? Vector3{ 0.0f, 1.0f, 0.0f } : Vector3{ 1.0f, 0.0f, 0.0f };
    Vector3 u = { eje.y * ref.z - eje.z * ref.y, eje.z * ref.x - eje.x * ref.z, eje.x * ref.y - eje.y * ref.x };
    float lu = std::sqrt(u.x * u.x + u.y * u.y + u.z * u.z);
    u = { u.x / lu, u.y / lu, u.z / lu };
    Vector3 v = { eje.y * u.z - eje.z * u.y, eje.z * u.x - eje.x * u.z, eje.x * u.y - eje.y * u.x };

    Color c0 = color;
    c0.a = 0;

    rlBegin(RL_QUADS);

    for (int k = 0; k < LADOS; k++)
    {
        float a0 = 6.2832f * (float)k / (float)LADOS;
        float a1 = 6.2832f * (float)(k + 1) / (float)LADOS;
        Vector3 d0 = { u.x * std::cos(a0) + v.x * std::sin(a0), u.y * std::cos(a0) + v.y * std::sin(a0), u.z * std::cos(a0) + v.z * std::sin(a0) };
        Vector3 d1 = { u.x * std::cos(a1) + v.x * std::sin(a1), u.y * std::cos(a1) + v.y * std::sin(a1), u.z * std::cos(a1) + v.z * std::sin(a1) };

        VerticeJade({ arriba.x + d0.x * r0, arriba.y + d0.y * r0, arriba.z + d0.z * r0 }, c0);
        VerticeJade({ arriba.x + d1.x * r0, arriba.y + d1.y * r0, arriba.z + d1.z * r0 }, c0);
        VerticeJade({ abajo.x + d1.x * r1, abajo.y + d1.y * r1, abajo.z + d1.z * r1 }, color);
        VerticeJade({ abajo.x + d0.x * r1, abajo.y + d0.y * r1, abajo.z + d0.z * r1 }, color);
    }

    rlEnd();
}


// Punto local de una losa (origen en su centro) llevado al mundo con la
// misma inclinacion que Dibujar: primero el giro en Z, despues en X.
static Vector3 PuntoTableroJade(const MinijuegoLaberintoInclinado& m, int jugador, float lx, float ly, float lz)
{
    const EstadoJugadorLaberinto& e = m.estadosJugadores[jugador];
    Vector3 c = m.centrosTableros[e.tablero < 0 ? 0 : e.tablero];
    float ax = e.inclinacionZ * INCLINACION_MAXIMA_GRADOS * DEG2RAD;
    float az = -e.inclinacionX * INCLINACION_MAXIMA_GRADOS * DEG2RAD;

    float x1 = lx * std::cos(az) - ly * std::sin(az);
    float y1 = lx * std::sin(az) + ly * std::cos(az);
    float y2 = y1 * std::cos(ax) - lz * std::sin(ax);
    float z2 = y1 * std::sin(ax) + lz * std::cos(ax);

    return { c.x + x1, c.y + y2, c.z + z2 };
}


static Vector3 PosicionLocal(float x, float z, float y)
{
    return { x - MITAD_TABLERO, y, z - MITAD_TABLERO };
}


// Donde se para el operador de cada losa (al costado exterior, mirando
// su losa y algo hacia la camara) y donde va su mando de piedra.
static void PosicionOperadorJade(
    const MinijuegoLaberintoInclinado& m,
    int jugador,
    Vector3& pies,
    Vector3& consola,
    float& angulo
)
{
    const EstadoJugadorLaberinto& e = m.estadosJugadores[jugador];
    Vector3 c = m.centrosTableros[e.tablero < 0 ? 0 : e.tablero];
    float lado = c.x < -0.1f ? -1.0f : (c.x > 0.1f ? 1.0f : 0.0f);

    if (lado == 0.0f)
    {
        // Losa central (tres jugadores): el operador va en la isleta izquierda.
        pies = { -8.9f, NIVEL_SUELO_JADE, c.z };
        consola = { -7.9f, NIVEL_SUELO_JADE, c.z };
        angulo = 60.0f;
        return;
    }

    pies = { c.x + lado * 8.9f, NIVEL_SUELO_JADE, c.z };
    consola = { c.x + lado * 7.95f, NIVEL_SUELO_JADE, c.z };
    angulo = lado < 0.0f ? 60.0f : -60.0f;
}


//==================================================
// EFECTOS (solo visual)
//==================================================

static int CantidadParticulasJade(int base)
{
    int cantidad = (int)std::lround((float)base * FactorCalidadGrafica(CalidadParticulas()));
    return cantidad < 1 ? 1 : cantidad;
}


static void EmitirParticulaJade(
    MinijuegoLaberintoInclinado& m,
    Vector3 posicion,
    Vector3 velocidad,
    float vida,
    float tamano,
    float gravedad,
    Color color,
    bool brillo
)
{
    for (int i = 0; i < MAX_PARTICULAS_LABERINTO; i++)
    {
        ParticulaLaberinto& p = m.particulas[i];

        if (p.activa)
        {
            continue;
        }

        p.activa = true;
        p.brillo = brillo;
        p.posicion = posicion;
        p.velocidad = velocidad;
        p.vida = vida;
        p.vidaMaxima = vida;
        p.tamano = tamano;
        p.gravedad = gravedad;
        p.color = color;
        return;
    }
}


// Estallido radial: chispas que suben y se abren.
static void EstallidoJade(
    MinijuegoLaberintoInclinado& m,
    Vector3 centro,
    int base,
    float fuerza,
    float subida,
    Color color,
    bool brillo,
    float tamano
)
{
    int cantidad = CantidadParticulasJade(base);

    for (int k = 0; k < cantidad; k++)
    {
        float a = AzarJade(m, 0.0f, 6.2832f);
        float r = AzarJade(m, 0.4f, 1.0f) * fuerza;

        EmitirParticulaJade(
            m,
            centro,
            { std::cos(a) * r, AzarJade(m, 0.5f, 1.0f) * subida, std::sin(a) * r },
            AzarJade(m, 0.45f, 0.9f),
            tamano * AzarJade(m, 0.7f, 1.3f),
            6.0f,
            color,
            brillo
        );
    }
}


static Color ColorJugadorJade(const Participante participantes[], int i)
{
    Color c = participantes[i].color;
    if (c.a == 0) c = COLORES_JUGADORES_LABERINTO[i % MAX_PARTICIPANTES];
    c.a = 255;
    return c;
}


static void ActualizarVisualLaberinto(
    MinijuegoLaberintoInclinado& m,
    float deltaTime,
    const Participante participantes[],
    int cantidadMaxima
)
{
    m.relojVisual += deltaTime;

    for (int i = 0; i < MAX_PARTICULAS_LABERINTO; i++)
    {
        ParticulaLaberinto& p = m.particulas[i];

        if (!p.activa)
        {
            continue;
        }

        p.vida -= deltaTime;

        if (p.vida <= 0.0f)
        {
            p.activa = false;
            continue;
        }

        float freno = 1.0f - deltaTime * 1.6f;
        if (freno < 0.0f) freno = 0.0f;
        p.velocidad.x *= freno;
        p.velocidad.z *= freno;
        p.velocidad.y -= p.gravedad * deltaTime;
        p.posicion.x += p.velocidad.x * deltaTime;
        p.posicion.y += p.velocidad.y * deltaTime;
        p.posicion.z += p.velocidad.z * deltaTime;
    }

    if (m.tiempoAnuncio >= 0.0f)
    {
        m.tiempoAnuncio += deltaTime;
        if (m.tiempoAnuncio > DURACION_ANUNCIO_JADE) m.tiempoAnuncio = -1.0f;
    }

    int limite = cantidadMaxima < MAX_PARTICIPANTES ? cantidadMaxima : MAX_PARTICIPANTES;
    bool jugando = m.fase == FASE_LABERINTO_JUGANDO || m.fase == FASE_LABERINTO_TERMINADO;

    for (int i = 0; i < limite; i++)
    {
        if (!m.resultado.participantes[i].participo)
        {
            continue;
        }

        const EstadoJugadorLaberinto& e = m.estadosJugadores[i];
        VisualJugadorLaberinto& v = m.visuales[i];
        Color color = ColorJugadorJade(participantes, i);

        v.destelloControl = v.destelloControl > 0.0f ? v.destelloControl - deltaTime * 1.5f : 0.0f;
        v.destelloCaida = v.destelloCaida > 0.0f ? v.destelloCaida - deltaTime * 1.4f : 0.0f;
        v.destelloMeta = v.destelloMeta > 0.0f ? v.destelloMeta - deltaTime * 0.6f : 0.0f;
        v.destelloGolpe = v.destelloGolpe > 0.0f ? v.destelloGolpe - deltaTime * 4.0f : 0.0f;
        if (v.tiempoTexto > 0.0f) v.tiempoTexto -= deltaTime;

        if (jugando && e.puntosControl > v.puntosControlPrevios)
        {
            Vector3 p = PuntoTableroJade(m, i, e.x - MITAD_TABLERO, 0.4f, e.z - MITAD_TABLERO);
            EstallidoJade(m, p, 14, 2.6f, 3.6f, color, false, 0.09f);
            EstallidoJade(m, p, 12, 2.0f, 4.2f, COLOR_JADE_CLARO, true, 0.12f);
            v.destelloControl = 1.0f;
            v.tiempoTexto = DURACION_TEXTO_JADE;
            v.tipoTexto = 1;
        }

        if (jugando && e.caidas > v.caidasPrevias)
        {
            Vector3 p = PuntoTableroJade(m, i, e.agujeroX - MITAD_TABLERO, 0.12f, e.agujeroZ - MITAD_TABLERO);
            EstallidoJade(m, p, 12, 1.8f, 1.6f, Color{ 150, 146, 128, 255 }, false, 0.08f);
            EstallidoJade(m, p, 6, 0.9f, 1.0f, Color{ 70, 64, 56, 255 }, false, 0.06f);
            v.destelloCaida = 1.0f;
            v.tiempoTexto = DURACION_TEXTO_JADE;
            v.tipoTexto = 2;
        }

        if (jugando && e.llego && !v.llegoPrevio)
        {
            Vector3 p = PuntoTableroJade(m, i, (float)m.metaColumna + 0.5f - MITAD_TABLERO, 0.7f, (float)m.metaFila + 0.5f - MITAD_TABLERO);
            EstallidoJade(m, p, 26, 2.4f, 6.5f, COLOR_ORO, true, 0.14f);
            EstallidoJade(m, p, 14, 2.0f, 4.5f, color, false, 0.1f);
            v.destelloMeta = 1.0f;

            if (m.fase == FASE_LABERINTO_JUGANDO)
            {
                // Solo la primera llegada acorta el plazo de los demas.
                m.anuncioConPlazo = m.jugadorAnuncio < 0;
                m.jugadorAnuncio = i;
                m.tiempoAnuncio = 0.0f;
            }
        }

        // Golpe fuerte contra un muro: la velocidad cambia de golpe.
        float dvx = e.velocidadX - v.velocidadXPrevia;
        float dvz = e.velocidadZ - v.velocidadZPrevia;

        if (
            m.fase == FASE_LABERINTO_JUGANDO &&
            e.caidas == v.caidasPrevias &&
            e.tiempoCaida <= 0.0f &&
            !e.llego &&
            dvx * dvx + dvz * dvz > 9.0f &&
            v.destelloGolpe <= 0.0f
        )
        {
            Vector3 p = PuntoTableroJade(m, i, e.x - MITAD_TABLERO, 0.12f, e.z - MITAD_TABLERO);
            EstallidoJade(m, p, 6, 1.2f, 1.4f, Color{ 196, 186, 156, 255 }, false, 0.06f);
            v.destelloGolpe = 1.0f;
        }

        v.puntosControlPrevios = e.puntosControl;
        v.caidasPrevias = e.caidas;
        v.llegoPrevio = e.llego;
        v.velocidadXPrevia = e.velocidadX;
        v.velocidadZPrevia = e.velocidadZ;
    }

    // Disparo de cada trampa: astillas de piedra en las boquillas.
    for (int t = 0; t < m.cantidadTrampas; t++)
    {
        float avance = 0.0f;
        bool pulso = m.fase == FASE_LABERINTO_JUGANDO && EstadoTrampa(m.trampas[t], m.tiempoJuego, avance) == 2;

        if (pulso && !m.pulsoVisualPrevio[t])
        {
            const TrampaDardosLaberinto& trampa = m.trampas[t];
            float bx = trampa.direccionX > 0.5f ? (float)trampa.columnaInicio : (trampa.direccionX < -0.5f ? (float)(trampa.columnaFin + 1) : ((float)trampa.columnaInicio + 0.5f));
            float bz = trampa.direccionZ > 0.5f ? (float)trampa.filaInicio : (trampa.direccionZ < -0.5f ? (float)(trampa.filaFin + 1) : ((float)trampa.filaInicio + 0.5f));

            for (int i = 0; i < limite; i++)
            {
                if (!m.resultado.participantes[i].participo) continue;

                Vector3 p = PuntoTableroJade(m, i, bx - MITAD_TABLERO, 0.35f, bz - MITAD_TABLERO);
                EstallidoJade(m, p, 5, 1.0f, 1.2f, Color{ 186, 172, 140, 255 }, false, 0.05f);
                EstallidoJade(m, p, 4, 0.8f, 1.0f, Color{ 255, 170, 70, 255 }, true, 0.07f);
            }
        }

        m.pulsoVisualPrevio[t] = pulso;
    }

    // Final: confeti sobre la losa de cada ganador.
    if (
        m.fase == FASE_LABERINTO_TERMINADO &&
        m.resultado.estado == RESULTADO_MINIJUEGO_FINALIZADO &&
        !m.confetiLanzado
    )
    {
        m.confetiLanzado = true;
        const Color confeti[5] =
        {
            COLOR_ORO, COLOR_JADE_CLARO, { 236, 96, 86, 255 }, { 96, 156, 240, 255 }, { 250, 246, 230, 255 }
        };

        for (int i = 0; i < limite; i++)
        {
            if (!m.resultado.participantes[i].participo || m.resultado.participantes[i].posicionFinal != 1)
            {
                continue;
            }

            const EstadoJugadorLaberinto& e = m.estadosJugadores[i];
            Vector3 c = m.centrosTableros[e.tablero < 0 ? 0 : e.tablero];
            int cantidad = CantidadParticulasJade(60);

            for (int k = 0; k < cantidad; k++)
            {
                float a = AzarJade(m, 0.0f, 6.2832f);
                float r = AzarJade(m, 1.0f, 4.0f);

                EmitirParticulaJade(
                    m,
                    { c.x + AzarJade(m, -2.0f, 2.0f), 2.0f, c.z + AzarJade(m, -2.0f, 2.0f) },
                    { std::cos(a) * r, AzarJade(m, 5.0f, 9.0f), std::sin(a) * r },
                    AzarJade(m, 1.8f, 3.0f),
                    AzarJade(m, 0.12f, 0.2f),
                    5.0f,
                    confeti[k % 5],
                    false
                );
            }
        }
    }
}


//==================================================
// PISO, ESTANQUE Y MUROS
//==================================================

// Rectangulo de losas talladas (x0..x1, z0..z1) a la altura y: junta
// oscura, cada losa con su tono, filo de luz y, algunas, un cuadro
// rehundido con un rombo de jade o una grieta.
static void LosasPisoJade(float x0, float x1, float z0, float z1, float y, float lado, int semilla)
{
    if (x1 - x0 < 0.05f || z1 - z0 < 0.05f) return;

    int nx = (int)std::ceil((x1 - x0) / lado - 0.01f);
    int nz = (int)std::ceil((z1 - z0) / lado - 0.01f);
    if (nx < 1) nx = 1;
    if (nz < 1) nz = 1;

    float px = (x1 - x0) / (float)nx;
    float pz = (z1 - z0) / (float)nz;
    const float J = 0.05f;
    float yl = y + 0.012f;

    rlBegin(RL_QUADS);
    QuadNieblaJade({ x0, y, z1 }, { x1, y, z1 }, { x1, y, z0 }, { x0, y, z0 }, COLOR_JUNTA_JADE);

    for (int i = 0; i < nx; i++)
    {
        for (int j = 0; j < nz; j++)
        {
            float a0 = x0 + px * (float)i + J, a1 = x0 + px * (float)(i + 1) - J;
            float b0 = z0 + pz * (float)j + J, b1 = z0 + pz * (float)(j + 1) - J;
            float h = Hash01Jade(i * 13 + semilla, j * 7 + semilla * 3);
            float h2 = Hash01Jade(i * 5 + semilla * 11, j * 17 + 91);
            Color base = TonoJade(COLOR_LOSA_PISO_JADE, 0.88f + 0.2f * h);
            if (h2 < 0.12f) base = MezclaJade(base, COLOR_JADE_OSCURO, 0.2f);

            QuadNieblaJade({ a0, yl, b1 }, { a1, yl, b1 }, { a1, yl, b0 }, { a0, yl, b0 }, base);

            // Filo claro del lado del fondo: la losa tiene canto.
            float filo = (b1 - b0) * 0.08f;
            QuadNieblaJade({ a0, yl + 0.002f, b0 + filo }, { a1, yl + 0.002f, b0 + filo }, { a1, yl + 0.002f, b0 }, { a0, yl + 0.002f, b0 }, TonoJade(base, 1.14f));

            float cx = (a0 + a1) * 0.5f, cz = (b0 + b1) * 0.5f;
            float rx = (a1 - a0) * 0.5f, rz = (b1 - b0) * 0.5f;

            if (h2 > 0.8f)
            {
                Color hondo = TonoJade(base, 0.72f);
                Color centro = TonoJade(base, 0.94f);
                float r = (rx < rz ? rx : rz) * 0.24f;
                QuadNieblaJade({ cx - rx * 0.62f, yl + 0.004f, cz + rz * 0.62f }, { cx + rx * 0.62f, yl + 0.004f, cz + rz * 0.62f }, { cx + rx * 0.62f, yl + 0.004f, cz - rz * 0.62f }, { cx - rx * 0.62f, yl + 0.004f, cz - rz * 0.62f }, hondo);
                QuadNieblaJade({ cx - rx * 0.48f, yl + 0.006f, cz + rz * 0.48f }, { cx + rx * 0.48f, yl + 0.006f, cz + rz * 0.48f }, { cx + rx * 0.48f, yl + 0.006f, cz - rz * 0.48f }, { cx - rx * 0.48f, yl + 0.006f, cz - rz * 0.48f }, centro);
                QuadNieblaJade({ cx - r, yl + 0.008f, cz }, { cx, yl + 0.008f, cz + r }, { cx + r, yl + 0.008f, cz }, { cx, yl + 0.008f, cz - r }, COLOR_JADE);
            }
            else if (h2 > 0.66f)
            {
                Color grieta = TonoJade(base, 0.55f);
                float g = 0.025f;
                QuadNieblaJade({ a0 + rx * 0.3f, yl + 0.004f, b0 + rz * 0.4f }, { a0 + rx * 0.3f + g, yl + 0.004f, b0 + rz * 0.4f - g }, { cx + rx * 0.2f + g, yl + 0.004f, cz + rz * 0.3f - g }, { cx + rx * 0.2f, yl + 0.004f, cz + rz * 0.3f }, grieta);
                QuadNieblaJade({ cx + rx * 0.2f, yl + 0.004f, cz + rz * 0.3f }, { cx + rx * 0.2f + g, yl + 0.004f, cz + rz * 0.3f - g }, { a1 - rx * 0.1f + g, yl + 0.004f, cz + rz * 0.1f - g }, { a1 - rx * 0.1f, yl + 0.004f, cz + rz * 0.1f }, grieta);
            }
        }
    }

    rlEnd();
}


// Cara de sillares: rectangulo vertical desde "origen" que corre "largo"
// por "eje" entre y0 e y1, con relieve hacia "normal". Junta oscura,
// cada sillar con su tono y un filo de luz arriba.
static void SillaresJade(
    Vector3 origen,
    Vector3 eje,
    float largo,
    float y0,
    float y1,
    Vector3 normal,
    float ancho,
    float alto,
    Color piedra,
    float luz,
    int semilla
)
{
    if (largo <= 0.01f || y1 - y0 <= 0.01f) return;

    auto Punto = [&](float s, float y, float relieve) -> Vector3
    {
        return { origen.x + eje.x * s + normal.x * relieve, y, origen.z + eje.z * s + normal.z * relieve };
    };

    const float J = 0.05f;

    rlBegin(RL_QUADS);
    QuadNieblaJade(Punto(0.0f, y0, 0.0f), Punto(largo, y0, 0.0f), Punto(largo, y1, 0.0f), Punto(0.0f, y1, 0.0f), TonoJade(COLOR_JUNTA_JADE, luz));

    int filas = (int)std::ceil((y1 - y0) / alto - 0.01f);

    for (int f = 0; f < filas; f++)
    {
        float fy0 = y0 + alto * (float)f;
        float fy1 = fy0 + alto;
        if (fy1 > y1) fy1 = y1;
        if (fy1 - fy0 < J * 2.0f) continue;

        float desfase = (f & 1) ? ancho * 0.5f : 0.0f;

        for (int col = -1; ; col++)
        {
            float s0 = (float)col * ancho + desfase;
            float s1 = s0 + ancho * (0.8f + 0.4f * Hash01Jade(col + semilla, f * 3 + semilla));
            if (s0 >= largo) break;
            if (s0 < 0.0f) s0 = 0.0f;
            if (s1 > largo) s1 = largo;
            if (s1 - s0 < J * 2.0f) continue;

            float h = Hash01Jade(col * 7 + semilla, f * 13 + semilla * 3);
            Color c = TonoJade(piedra, (0.86f + 0.22f * h) * luz);
            if (Hash01Jade(col + semilla * 5, f + 77) > 0.9f) c = MezclaJade(c, COLOR_JADE_OSCURO, 0.25f);

            Vector3 a = Punto(s0 + J, fy0 + J, 0.012f);
            Vector3 b = Punto(s1 - J, fy0 + J, 0.012f);
            Vector3 cc = Punto(s1 - J, fy1 - J, 0.012f);
            Vector3 d = Punto(s0 + J, fy1 - J, 0.012f);
            QuadNieblaJade(a, b, cc, d, c);

            float filo = (fy1 - fy0) * 0.16f;
            QuadNieblaJade(Punto(s0 + J, fy1 - J - filo, 0.014f), Punto(s1 - J, fy1 - J - filo, 0.014f), cc, d, TonoJade(c, 1.14f));
        }
    }

    rlEnd();
}


// Friso con greca escalonada sobre una cara vertical (mismo sistema que
// SillaresJade): fondo de jade oscuro y bloques de piedra en relieve.
static void FrisoGrecaJade(
    Vector3 origen,
    Vector3 eje,
    float largo,
    float y0,
    float y1,
    Vector3 normal,
    float luz
)
{
    auto Punto = [&](float s, float y, float relieve) -> Vector3
    {
        return { origen.x + eje.x * s + normal.x * relieve, y, origen.z + eje.z * s + normal.z * relieve };
    };

    float h = y1 - y0;
    Color fondo = TonoJade(COLOR_JADE_OSCURO, luz * 0.8f);
    Color talla = TonoJade(COLOR_TALLA_JADE, luz);
    Color oro = TonoJade(COLOR_ORO, luz);

    rlBegin(RL_QUADS);
    QuadNieblaJade(Punto(0.0f, y0, 0.01f), Punto(largo, y0, 0.01f), Punto(largo, y1, 0.01f), Punto(0.0f, y1, 0.01f), fondo);

    // Listeles arriba y abajo.
    QuadNieblaJade(Punto(0.0f, y1 - h * 0.12f, 0.03f), Punto(largo, y1 - h * 0.12f, 0.03f), Punto(largo, y1, 0.03f), Punto(0.0f, y1, 0.03f), TonoJade(talla, 1.08f));
    QuadNieblaJade(Punto(0.0f, y0, 0.03f), Punto(largo, y0, 0.03f), Punto(largo, y0 + h * 0.12f, 0.03f), Punto(0.0f, y0 + h * 0.12f, 0.03f), TonoJade(talla, 0.86f));

    float paso = h * 1.4f;
    float m0 = y0 + h * 0.18f, m1 = y1 - h * 0.18f, mm = (m0 + m1) * 0.5f;

    for (float s = 0.0f; s + paso <= largo + 0.01f; s += paso)
    {
        // Escalon: bloque bajo a la izquierda, alto a la derecha, y un clavo de oro.
        QuadNieblaJade(Punto(s + paso * 0.06f, m0, 0.04f), Punto(s + paso * 0.5f, m0, 0.04f), Punto(s + paso * 0.5f, mm, 0.04f), Punto(s + paso * 0.06f, mm, 0.04f), talla);
        QuadNieblaJade(Punto(s + paso * 0.5f, mm, 0.04f), Punto(s + paso * 0.94f, mm, 0.04f), Punto(s + paso * 0.94f, m1, 0.04f), Punto(s + paso * 0.5f, m1, 0.04f), TonoJade(talla, 1.06f));
        QuadNieblaJade(Punto(s + paso * 0.62f, m0 + 0.02f, 0.05f), Punto(s + paso * 0.8f, m0 + 0.02f, 0.05f), Punto(s + paso * 0.8f, mm - 0.04f, 0.05f), Punto(s + paso * 0.62f, mm - 0.04f, 0.05f), oro);
    }

    rlEnd();
}


static void DibujarPisoCamaraJade(float H)
{
    float W = MITAD_ESTANQUE_X_JADE + ANCHO_BORDE_JADE;
    float Hb = H + ANCHO_BORDE_JADE;
    float X = MURO_LATERAL_JADE;
    float zFondo = -H - 5.0f;
    float zFrente = H + 16.0f;

    // Cuatro rectangulos alrededor del estanque (sin recortar losas).
    LosasPisoJade(-X, X, zFondo, -Hb, NIVEL_SUELO_JADE, 2.0f, 11);
    LosasPisoJade(-X, X, Hb, zFrente, NIVEL_SUELO_JADE, 2.0f, 23);
    LosasPisoJade(-X, -W, -Hb, Hb, NIVEL_SUELO_JADE, 2.0f, 37);
    LosasPisoJade(W, X, -Hb, Hb, NIVEL_SUELO_JADE, 2.0f, 53);

    // Camino de losas claras hacia el altar (fondo) y la entrada (frente).
    rlBegin(RL_QUADS);
    Color camino = TonoJade(COLOR_TALLA_JADE, 0.92f);
    float y = NIVEL_SUELO_JADE + 0.016f;

    for (int s = -1; s <= 1; s += 2)
    {
        QuadNieblaJade({ -6.4f, y, (float)s * Hb }, { -6.2f, y, (float)s * Hb }, { -6.2f, y, s < 0 ? zFondo : zFrente }, { -6.4f, y, s < 0 ? zFondo : zFrente }, camino);
        QuadNieblaJade({ 6.2f, y, (float)s * Hb }, { 6.4f, y, (float)s * Hb }, { 6.4f, y, s < 0 ? zFondo : zFrente }, { 6.2f, y, s < 0 ? zFondo : zFrente }, camino);
    }

    rlEnd();
}


// Bordillo del estanque: piedra clara con incrustacion de jade y tachas
// de oro; esquinas con bloque tallado y gema.
static void DibujarBordeEstanqueJade(float H)
{
    float W = MITAD_ESTANQUE_X_JADE;
    float b = ANCHO_BORDE_JADE;
    float yc = NIVEL_SUELO_JADE + 0.14f;
    float alto = 0.28f;
    Color piedra = COLOR_TALLA_JADE;

    CuboNieblaJade({ 0.0f, yc, -H - b * 0.5f }, { 2.0f * W + 2.0f * b, alto, b }, piedra);
    CuboNieblaJade({ 0.0f, yc, H + b * 0.5f }, { 2.0f * W + 2.0f * b, alto, b }, piedra);
    CuboNieblaJade({ -W - b * 0.5f, yc, 0.0f }, { b, alto, 2.0f * H }, piedra);
    CuboNieblaJade({ W + b * 0.5f, yc, 0.0f }, { b, alto, 2.0f * H }, piedra);

    // Incrustacion de jade y tachas de oro sobre el bordillo.
    float y = yc + alto * 0.5f + 0.004f;
    float m = b * 0.5f;

    rlBegin(RL_QUADS);
    Color jade = COLOR_JADE_OSCURO;
    QuadNieblaJade({ -W - m, y, -H - m - 0.06f }, { W + m, y, -H - m - 0.06f }, { W + m, y, -H - m + 0.06f }, { -W - m, y, -H - m + 0.06f }, jade);
    QuadNieblaJade({ -W - m, y, H + m - 0.06f }, { W + m, y, H + m - 0.06f }, { W + m, y, H + m + 0.06f }, { -W - m, y, H + m + 0.06f }, jade);
    QuadNieblaJade({ -W - m - 0.06f, y, -H - m }, { -W - m + 0.06f, y, -H - m }, { -W - m + 0.06f, y, H + m }, { -W - m - 0.06f, y, H + m }, jade);
    QuadNieblaJade({ W + m - 0.06f, y, -H - m }, { W + m + 0.06f, y, -H - m }, { W + m + 0.06f, y, H + m }, { W + m - 0.06f, y, H + m }, jade);

    Color oro = COLOR_ORO;
    const float t = 0.09f;

    for (float x = -W + 1.0f; x < W; x += 2.0f)
    {
        for (int s = -1; s <= 1; s += 2)
        {
            float z = (float)s * (H + m);
            QuadNieblaJade({ x - t, y + 0.002f, z }, { x, y + 0.002f, z + t }, { x + t, y + 0.002f, z }, { x, y + 0.002f, z - t }, oro);
        }
    }

    for (float z = -H + 1.0f; z < H; z += 2.0f)
    {
        for (int s = -1; s <= 1; s += 2)
        {
            float x = (float)s * (W + m);
            QuadNieblaJade({ x - t, y + 0.002f, z }, { x, y + 0.002f, z + t }, { x + t, y + 0.002f, z }, { x, y + 0.002f, z - t }, oro);
        }
    }

    rlEnd();

    // Esquinas.
    for (int sx = -1; sx <= 1; sx += 2)
    {
        for (int sz = -1; sz <= 1; sz += 2)
        {
            Vector3 c = { (float)sx * (W + m), NIVEL_SUELO_JADE + 0.25f, (float)sz * (H + m) };
            CuboNieblaJade(c, { 1.2f, 0.5f, 1.2f }, TonoJade(piedra, 0.9f));
            CuboNieblaJade({ c.x, c.y + 0.3f, c.z }, { 0.9f, 0.1f, 0.9f }, piedra);
            GemaJade({ c.x, c.y + 0.5f, c.z }, 0.22f, 0.16f, TinteJade(COLOR_JADE, c));
        }
    }
}


// Estanque: fondo de losas hundidas, paredes de sillares con friso de
// greca, isletas (tres jugadores) y los pedestales que sostienen las losas.
static void DibujarEstanqueJade(const MinijuegoLaberintoInclinado& m, float H)
{
    float W = MITAD_ESTANQUE_X_JADE;

    LosasPisoJade(-W, W, -H, H, FONDO_ESTANQUE_JADE, 2.4f, 71);

    float yFriso = NIVEL_SUELO_JADE - 0.9f;
    SillaresJade({ -W, 0.0f, -H }, { 1.0f, 0.0f, 0.0f }, 2.0f * W, FONDO_ESTANQUE_JADE, yFriso, { 0.0f, 0.0f, 1.0f }, 1.6f, 0.62f, COLOR_MURO_CAMARA_JADE, 0.7f, 3);
    SillaresJade({ -W, 0.0f, H }, { 0.0f, 0.0f, -1.0f }, 2.0f * H, FONDO_ESTANQUE_JADE, yFriso, { 1.0f, 0.0f, 0.0f }, 1.6f, 0.62f, COLOR_MURO_CAMARA_JADE, 0.8f, 5);
    SillaresJade({ W, 0.0f, -H }, { 0.0f, 0.0f, 1.0f }, 2.0f * H, FONDO_ESTANQUE_JADE, yFriso, { -1.0f, 0.0f, 0.0f }, 1.6f, 0.62f, COLOR_MURO_CAMARA_JADE, 0.62f, 7);
    SillaresJade({ W, 0.0f, H }, { -1.0f, 0.0f, 0.0f }, 2.0f * W, FONDO_ESTANQUE_JADE, yFriso, { 0.0f, 0.0f, -1.0f }, 1.6f, 0.62f, COLOR_MURO_CAMARA_JADE, 0.55f, 9);

    FrisoGrecaJade({ -W, 0.0f, -H }, { 1.0f, 0.0f, 0.0f }, 2.0f * W, yFriso, NIVEL_SUELO_JADE, { 0.0f, 0.0f, 1.0f }, 0.85f);
    FrisoGrecaJade({ -W, 0.0f, H }, { 0.0f, 0.0f, -1.0f }, 2.0f * H, yFriso, NIVEL_SUELO_JADE, { 1.0f, 0.0f, 0.0f }, 0.95f);
    FrisoGrecaJade({ W, 0.0f, -H }, { 0.0f, 0.0f, 1.0f }, 2.0f * H, yFriso, NIVEL_SUELO_JADE, { -1.0f, 0.0f, 0.0f }, 0.75f);

    // Tres jugadores: dos isletas de piedra a los lados de la losa central.
    if (m.cantidadTableros == 3)
    {
        for (int s = -1; s <= 1; s += 2)
        {
            float x0 = (float)s * 7.3f;
            float x1 = (float)s * W;
            float xa = x0 < x1 ? x0 : x1;
            float xb = x0 < x1 ? x1 : x0;
            float alto = NIVEL_SUELO_JADE - FONDO_ESTANQUE_JADE;
            Vector3 c = { (xa + xb) * 0.5f, FONDO_ESTANQUE_JADE + alto * 0.5f, (1.5f + H) * 0.5f };
            CuboNieblaJade(c, { xb - xa, alto, H - 1.5f }, COLOR_MURO_CAMARA_JADE);
            LosasPisoJade(xa, xb, 1.5f, H, NIVEL_SUELO_JADE, 2.0f, 80 + s);
            CuboNieblaJade({ x0, NIVEL_SUELO_JADE + 0.12f, c.z }, { 0.5f, 0.24f, H - 1.5f }, COLOR_TALLA_JADE);
            CuboNieblaJade({ c.x, NIVEL_SUELO_JADE + 0.12f, 1.5f }, { xb - xa, 0.24f, 0.5f }, COLOR_TALLA_JADE);
        }
    }

    // Pedestales: plinto sumergido, fuste con anillo de jade, capitel y
    // una gema-pivote de jade sobre la que se inclina la losa.
    for (int k = 0; k < m.cantidadTableros && k < MAX_PARTICIPANTES; k++)
    {
        Vector3 c = m.centrosTableros[k];
        Color piedra = TinteJade(COLOR_TALLA_JADE, { c.x, -2.0f, c.z });
        Color oscura = TonoJade(piedra, 0.75f);

        PrismaJade({ c.x, FONDO_ESTANQUE_JADE, c.z }, 1.9f, 1.7f, 0.6f, 8, oscura, 0.39f);
        PrismaJade({ c.x, FONDO_ESTANQUE_JADE + 0.6f, c.z }, 1.15f, 1.0f, -1.5f - (FONDO_ESTANQUE_JADE + 0.6f), 8, piedra, 0.39f);
        PrismaJade({ c.x, -2.35f, c.z }, 1.12f, 1.1f, 0.28f, 8, TinteJade(COLOR_JADE_OSCURO, c), 0.39f);
        PrismaJade({ c.x, -1.5f, c.z }, 1.0f, 1.45f, 0.42f, 8, piedra, 0.39f);
        AnilloJade({ c.x, -1.07f, c.z }, 0.42f, 0.66f, COLOR_ORO, 16);
        GemaJade({ c.x, -0.86f, c.z }, 0.42f, 0.2f, COLOR_JADE, 8);
    }
}


// Columna con relieves: basa escalonada, tambores octogonales que alternan
// liso y tallado (placas de jade con clavo de oro), capitel con abaco.
static void ColumnaRelieveJade(Vector3 base, float alto, int semilla)
{
    Color piedra = TinteJade(COLOR_TALLA_JADE, { base.x, base.y + 2.0f, base.z });
    Color oscura = TonoJade(piedra, 0.72f);
    Color jade = TinteJade(COLOR_JADE_OSCURO, base);
    Color oro = TinteJade(COLOR_ORO, base);

    CuboJade({ base.x, base.y + 0.2f, base.z }, { 1.7f, 0.4f, 1.7f }, oscura);
    CuboJade({ base.x, base.y + 0.5f, base.z }, { 1.4f, 0.2f, 1.4f }, piedra);

    float tope = base.y + alto - 0.8f;
    float y = base.y + 0.6f;
    int k = 0;

    while (y < tope - 0.05f)
    {
        float h = tope - y < 0.9f ? tope - y : 0.9f;
        Color tambor = TonoJade(piedra, 0.94f + 0.1f * Hash01Jade(k, semilla));
        PrismaJade({ base.x, y, base.z }, 0.56f, 0.56f, h - 0.06f, 8, tambor, 0.39f);
        PrismaJade({ base.x, y + h - 0.06f, base.z }, 0.5f, 0.5f, 0.06f, 8, oscura, 0.39f);

        if (k % 2 == 1)
        {
            float ym = y + h * 0.5f;
            CuboJade({ base.x + 0.56f, ym, base.z }, { 0.06f, h * 0.62f, 0.34f }, jade);
            CuboJade({ base.x - 0.56f, ym, base.z }, { 0.06f, h * 0.62f, 0.34f }, jade);
            CuboJade({ base.x, ym, base.z + 0.56f }, { 0.34f, h * 0.62f, 0.06f }, jade);
            CuboJade({ base.x, ym, base.z - 0.56f }, { 0.34f, h * 0.62f, 0.06f }, jade);
            CuboJade({ base.x + 0.6f, ym, base.z }, { 0.04f, 0.1f, 0.1f }, oro);
            CuboJade({ base.x - 0.6f, ym, base.z }, { 0.04f, 0.1f, 0.1f }, oro);
            CuboJade({ base.x, ym, base.z + 0.6f }, { 0.1f, 0.1f, 0.04f }, oro);
        }

        y += h;
        k++;
    }

    PrismaJade({ base.x, tope, base.z }, 0.56f, 0.88f, 0.4f, 8, piedra, 0.39f);
    PrismaJade({ base.x, tope + 0.16f, base.z }, 0.8f, 0.8f, 0.12f, 8, jade, 0.39f);
    CuboJade({ base.x, tope + 0.6f, base.z }, { 2.0f, 0.4f, 2.0f }, oscura);
}


// Pilastra adosada a un muro (sobresale hacia "normal"), con bandas
// talladas y una placa de jade.
static void PilastraJade(Vector3 base, Vector3 normal, float alto)
{
    Color piedra = TinteJade(COLOR_TALLA_JADE, { base.x, base.y + 2.0f, base.z });
    Color oscura = TonoJade(piedra, 0.74f);
    bool enX = std::fabs(normal.x) > 0.5f;
    Vector3 t = enX ? Vector3{ 0.8f, alto, 1.3f } : Vector3{ 1.3f, alto, 0.8f };
    Vector3 c = { base.x + normal.x * 0.4f, base.y + alto * 0.5f, base.z + normal.z * 0.4f };

    CuboJade(c, t, TonoJade(piedra, 0.92f));

    for (float y = base.y + 1.0f; y < base.y + alto - 0.6f; y += 1.6f)
    {
        Vector3 tb = enX ? Vector3{ 0.9f, 0.18f, 1.45f } : Vector3{ 1.45f, 0.18f, 0.9f };
        CuboJade({ c.x, y, c.z }, tb, oscura);
    }

    Vector3 placa = { c.x + normal.x * 0.42f, base.y + 2.6f, c.z + normal.z * 0.42f };
    CuboJade(placa, enX ? Vector3{ 0.04f, 0.8f, 0.6f } : Vector3{ 0.6f, 0.8f, 0.04f }, TinteJade(COLOR_JADE_OSCURO, placa));
    CuboJade({ placa.x + normal.x * 0.02f, placa.y, placa.z + normal.z * 0.02f }, enX ? Vector3{ 0.04f, 0.2f, 0.2f } : Vector3{ 0.2f, 0.2f, 0.04f }, TinteJade(COLOR_ORO, placa));

    Vector3 tc = enX ? Vector3{ 1.1f, 0.4f, 1.7f } : Vector3{ 1.7f, 0.4f, 1.1f };
    CuboJade({ c.x, base.y + alto - 0.2f, c.z }, tc, oscura);
    CuboJade({ c.x, base.y + 0.25f, c.z }, tc, oscura);
}


// Hornacina: marco de piedra sobre el muro, interior en penumbra con un
// idolito; deja ver la profundidad del templo detras del muro.
static void HornacinaJade(Vector3 base, Vector3 normal, Vector3 eje)
{
    Color piedra = TinteJade(COLOR_TALLA_JADE, { base.x, base.y + 1.5f, base.z });
    Color oscura = TonoJade(piedra, 0.7f);
    Color hueco = TinteJade(Color{ 18, 30, 26, 255 }, base);
    const float ancho = 1.8f;
    const float alto = 2.6f;

    auto P = [&](float s, float y, float r) -> Vector3
    {
        return { base.x + eje.x * s + normal.x * r, y, base.z + eje.z * s + normal.z * r };
    };

    rlBegin(RL_QUADS);
    QuadJade(P(-ancho * 0.5f, base.y, 0.03f), P(ancho * 0.5f, base.y, 0.03f), P(ancho * 0.5f, base.y + alto, 0.03f), P(-ancho * 0.5f, base.y + alto, 0.03f), hueco);
    rlEnd();

    // Jambas, dintel y repisa.
    bool enX = std::fabs(normal.x) > 0.5f;
    for (int s = -1; s <= 1; s += 2)
    {
        Vector3 j = P((float)s * (ancho * 0.5f + 0.18f), base.y + alto * 0.5f, 0.2f);
        CuboJade(j, enX ? Vector3{ 0.4f, alto + 0.2f, 0.36f } : Vector3{ 0.36f, alto + 0.2f, 0.4f }, piedra);
    }

    Vector3 d = P(0.0f, base.y + alto + 0.2f, 0.24f);
    CuboJade(d, enX ? Vector3{ 0.48f, 0.4f, ancho + 0.9f } : Vector3{ ancho + 0.9f, 0.4f, 0.48f }, oscura);
    Vector3 r = P(0.0f, base.y + 0.1f, 0.26f);
    CuboJade(r, enX ? Vector3{ 0.52f, 0.2f, ancho + 0.5f } : Vector3{ ancho + 0.5f, 0.2f, 0.52f }, oscura);

    // Idolito sentado en la penumbra.
    Color idolo = TinteJade(TonoJade(COLOR_JADE_OSCURO, 0.8f), base);
    Vector3 i0 = P(0.0f, base.y + 0.5f, 0.25f);
    CuboJade(i0, { 0.5f, 0.6f, 0.5f }, idolo);
    CuboJade({ i0.x, i0.y + 0.52f, i0.z }, { 0.4f, 0.42f, 0.4f }, idolo);
    CuboJade({ i0.x, i0.y + 0.82f, i0.z }, { 0.56f, 0.12f, 0.3f }, TinteJade(COLOR_ORO, base));
}


// Muros de la camara: sillares de piso a techo, zocalo, pilastras con
// hornacinas entre ellas y friso de glifos; muro del fondo con el altar.
static void DibujarMurosCamaraJade(float H, int paso)
{
    float X = MURO_LATERAL_JADE;
    float zFondo = -H - 5.0f;
    float zFrente = H + 16.0f;

    for (int s = -1; s <= 1; s += 2)
    {
        float sx = (float)s;
        Vector3 normal = { -sx, 0.0f, 0.0f };
        Vector3 eje = { 0.0f, 0.0f, s < 0 ? -1.0f : 1.0f };
        Vector3 origen = { sx * X, 0.0f, s < 0 ? zFrente : zFondo };

        SillaresJade(origen, eje, zFrente - zFondo, NIVEL_SUELO_JADE, ALTO_CAMARA_JADE, normal, 2.0f, 0.9f, COLOR_MURO_CAMARA_JADE, s < 0 ? 0.86f : 0.7f, 20 + s);
        CuboNieblaJade({ sx * (X - 0.2f), NIVEL_SUELO_JADE + 0.35f, (zFondo + zFrente) * 0.5f }, { 0.4f, 0.7f, zFrente - zFondo }, TonoJade(COLOR_MURO_CAMARA_JADE, 0.7f));
        FrisoGrecaJade({ sx * (X - 0.02f), 0.0f, origen.z }, eje, zFrente - zFondo, 4.4f, 5.4f, normal, s < 0 ? 0.9f : 0.74f);

        int k = 0;
        for (float z = zFondo + 3.0f; z < zFrente - 1.0f; z += 6.0f, k++)
        {
            PilastraJade({ sx * X, NIVEL_SUELO_JADE, z }, normal, 9.0f);

            if (paso == 1 || k % 2 == 0)
            {
                HornacinaJade({ sx * X, NIVEL_SUELO_JADE + 0.6f, z + 3.0f }, normal, { 0.0f, 0.0f, 1.0f });
            }
        }
    }

    SillaresJade({ -X, 0.0f, zFondo }, { 1.0f, 0.0f, 0.0f }, 2.0f * X, NIVEL_SUELO_JADE, ALTO_CAMARA_JADE, { 0.0f, 0.0f, 1.0f }, 2.0f, 0.9f, COLOR_MURO_CAMARA_JADE, 0.8f, 31);
    CuboNieblaJade({ 0.0f, NIVEL_SUELO_JADE + 0.35f, zFondo + 0.2f }, { 2.0f * X, 0.7f, 0.4f }, TonoJade(COLOR_MURO_CAMARA_JADE, 0.7f));
    FrisoGrecaJade({ -X, 0.0f, zFondo + 0.02f }, { 1.0f, 0.0f, 0.0f }, 2.0f * X, 7.6f, 8.6f, { 0.0f, 0.0f, 1.0f }, 0.85f);

    const float pilastras[4] = { 8.0f, 12.0f, 16.5f, 21.0f };
    for (int k = 0; k < 4; k++)
    {
        for (int s = -1; s <= 1; s += 2)
        {
            PilastraJade({ (float)s * pilastras[k], NIVEL_SUELO_JADE, zFondo }, { 0.0f, 0.0f, 1.0f }, 9.0f);
        }
    }

    for (int s = -1; s <= 1; s += 2)
    {
        HornacinaJade({ (float)s * 14.25f, NIVEL_SUELO_JADE + 0.6f, zFondo }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f, 0.0f });
        if (paso == 1) HornacinaJade({ (float)s * 18.75f, NIVEL_SUELO_JADE + 0.6f, zFondo }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f, 0.0f });
    }
}


// Altar del fondo: gradas, idolo-mascara de jade con ojos de oro, orejeras
// y tocado de plumas, columnas talladas a los lados y ofrenda.
static void DibujarAltarIdoloJade(float H)
{
    float z0 = -H - 5.0f;
    Color piedra = COLOR_TALLA_JADE;

    for (int k = 0; k < 3; k++)
    {
        float ancho = 12.0f - 2.0f * (float)k;
        float fondo = 3.4f - 0.8f * (float)k;
        CuboNieblaJade({ 0.0f, NIVEL_SUELO_JADE + 0.2f + 0.4f * (float)k, z0 + fondo * 0.5f }, { ancho, 0.4f, fondo }, TonoJade(piedra, 0.95f - 0.05f * (float)k));
        CuboNieblaJade({ 0.0f, NIVEL_SUELO_JADE + 0.39f + 0.4f * (float)k, z0 + fondo - 0.08f }, { ancho, 0.04f, 0.16f }, TonoJade(COLOR_JADE_OSCURO, 1.1f));
    }

    float yb = NIVEL_SUELO_JADE + 1.2f;
    Vector3 ref = { 0.0f, yb + 2.0f, z0 + 1.0f };
    Color jade = TinteJade(MezclaJade(COLOR_JADE, COLOR_JADE_OSCURO, 0.45f), ref);
    Color jadeClaro = TinteJade(COLOR_JADE, ref);
    Color oro = TinteJade(COLOR_ORO, ref);
    Color oscuro = TinteJade(Color{ 20, 40, 34, 255 }, ref);

    CuboJade({ 0.0f, yb + 1.8f, z0 + 0.9f }, { 3.6f, 3.6f, 1.5f }, jade);
    CuboJade({ 0.0f, yb + 3.1f, z0 + 1.7f }, { 3.2f, 0.32f, 0.2f }, jadeClaro);
    CuboJade({ -0.8f, yb + 2.65f, z0 + 1.68f }, { 0.8f, 0.4f, 0.14f }, oro);
    CuboJade({ 0.8f, yb + 2.65f, z0 + 1.68f }, { 0.8f, 0.4f, 0.14f }, oro);
    CuboJade({ -0.8f, yb + 2.65f, z0 + 1.76f }, { 0.3f, 0.26f, 0.04f }, oscuro);
    CuboJade({ 0.8f, yb + 2.65f, z0 + 1.76f }, { 0.3f, 0.26f, 0.04f }, oscuro);
    CuboJade({ 0.0f, yb + 2.0f, z0 + 1.82f }, { 0.5f, 0.9f, 0.36f }, jadeClaro);
    CuboJade({ 0.0f, yb + 1.15f, z0 + 1.7f }, { 1.8f, 0.46f, 0.18f }, oscuro);

    for (int k = -3; k <= 3; k++)
    {
        CuboJade({ (float)k * 0.22f, yb + 1.3f, z0 + 1.78f }, { 0.12f, 0.14f, 0.06f }, oro);
    }

    for (int s = -1; s <= 1; s += 2)
    {
        CuboJade({ (float)s * 2.05f, yb + 2.2f, z0 + 0.9f }, { 0.5f, 1.0f, 1.0f }, oro);
        CuboJade({ (float)s * 2.32f, yb + 2.2f, z0 + 0.9f }, { 0.06f, 0.5f, 0.5f }, jadeClaro);
    }

    // Tocado: abanico de plumas que alternan jade y jade claro con puntas de oro.
    for (int k = -4; k <= 4; k++)
    {
        rlPushMatrix();
        rlTranslatef(0.0f, yb + 3.5f, z0 + 0.55f);
        rlRotatef((float)k * 15.0f, 0.0f, 0.0f, 1.0f);
        CuboJade({ 0.0f, 1.3f, 0.0f }, { 0.5f, 2.6f, 0.22f }, (k & 1) ? jade : jadeClaro);
        CuboJade({ 0.0f, 2.7f, 0.0f }, { 0.44f, 0.3f, 0.26f }, oro);
        rlPopMatrix();
    }

    CuboJade({ 0.0f, yb + 3.75f, z0 + 1.0f }, { 4.0f, 0.5f, 1.3f }, oro);

    for (int s = -1; s <= 1; s += 2)
    {
        ColumnaRelieveJade({ (float)s * 4.6f, yb, z0 + 1.1f }, 7.0f, 5 + s);
    }
}


//==================================================
// UTILERIA DEL TEMPLO
//==================================================

static void BraseroJade(Vector3 base, float t, int semilla)
{
    Color piedra = TinteJade(COLOR_TALLA_JADE, base);
    Color bronce = TinteJade(COLOR_BRONCE_JADE, base);
    Color oscuro = TonoJade(bronce, 0.6f);

    PrismaJade(base, 0.5f, 0.38f, 0.5f, 8, piedra, 0.39f);
    PrismaJade({ base.x, base.y + 0.5f, base.z }, 0.12f, 0.12f, 0.45f, 6, oscuro);
    PrismaJade({ base.x, base.y + 0.95f, base.z }, 0.22f, 0.64f, 0.38f, 10, bronce);
    AnilloJade({ base.x, base.y + 1.33f, base.z }, 0.52f, 0.66f, TonoJade(bronce, 1.2f), 14);
    DiscoJade({ base.x, base.y + 1.3f, base.z }, 0.54f, Color{ 150, 42, 20, 255 }, 12);

    for (int k = 0; k < 3; k++)
    {
        float f = 0.78f + 0.22f * std::sin(t * 11.0f + (float)k * 2.1f + (float)semilla);
        float ox = (k == 1 ? 0.16f : (k == 2 ? -0.14f : 0.0f));
        float oz = (k == 1 ? 0.08f : (k == 2 ? -0.1f : 0.0f));
        Color llama = k == 0 ? Color{ 255, 150, 50, 255 } : Color{ 255, 206, 90, 255 };
        PrismaJade({ base.x + ox, base.y + 1.3f, base.z + oz }, k == 0 ? 0.3f : 0.18f, 0.0f, (k == 0 ? 0.9f : 0.6f) * f, 6, llama, t * 2.0f + (float)k);
    }
}


// Guardian de piedra: plinto con placa, faldellin de jade, pectoral de oro,
// lanza y tocado de plumas. anguloY 0 mira hacia +Z.
static void EstatuaGuardianJade(Vector3 base, float anguloY, float escala)
{
    Color piedra = TinteJade(COLOR_ESTATUA_JADE, { base.x, base.y + 2.0f, base.z });
    Color oscura = TonoJade(piedra, 0.72f);
    Color jade = TinteJade(COLOR_JADE_OSCURO, base);
    Color jadeClaro = TinteJade(COLOR_JADE, base);
    Color oro = TinteJade(COLOR_ORO, base);
    Color madera = TinteJade(Color{ 92, 66, 44, 255 }, base);

    rlPushMatrix();
    rlTranslatef(base.x, base.y, base.z);
    rlRotatef(anguloY, 0.0f, 1.0f, 0.0f);
    rlScalef(escala, escala, escala);

    CuboJade({ 0.0f, 0.35f, 0.0f }, { 1.4f, 0.7f, 1.4f }, oscura);
    CuboJade({ 0.0f, 0.72f, 0.0f }, { 1.5f, 0.08f, 1.5f }, piedra);
    CuboJade({ 0.0f, 0.36f, 0.71f }, { 0.7f, 0.4f, 0.03f }, jade);
    CuboJade({ 0.0f, 0.36f, 0.73f }, { 0.16f, 0.16f, 0.02f }, oro);

    CuboJade({ -0.22f, 1.1f, 0.0f }, { 0.3f, 0.72f, 0.36f }, piedra);
    CuboJade({ 0.22f, 1.1f, 0.0f }, { 0.3f, 0.72f, 0.36f }, piedra);
    CuboJade({ 0.0f, 1.55f, 0.02f }, { 0.82f, 0.32f, 0.52f }, jade);
    CuboJade({ 0.0f, 2.05f, 0.0f }, { 0.86f, 0.74f, 0.5f }, piedra);
    CuboJade({ 0.0f, 2.12f, 0.26f }, { 0.52f, 0.3f, 0.05f }, oro);
    CuboJade({ -0.56f, 1.98f, 0.04f }, { 0.24f, 0.72f, 0.28f }, piedra);
    CuboJade({ 0.56f, 2.02f, 0.14f }, { 0.24f, 0.62f, 0.28f }, piedra);

    CuboJade({ 0.66f, 2.1f, 0.3f }, { 0.08f, 3.0f, 0.08f }, madera);
    GemaJade({ 0.66f, 3.75f, 0.3f }, 0.12f, 0.2f, oro, 4);

    CuboJade({ 0.0f, 2.68f, 0.0f }, { 0.52f, 0.52f, 0.5f }, piedra);
    CuboJade({ 0.0f, 2.72f, 0.26f }, { 0.4f, 0.1f, 0.03f }, oscura);
    CuboJade({ -0.11f, 2.73f, 0.27f }, { 0.1f, 0.06f, 0.02f }, oro);
    CuboJade({ 0.11f, 2.73f, 0.27f }, { 0.1f, 0.06f, 0.02f }, oro);
    CuboJade({ 0.0f, 3.06f, -0.04f }, { 0.72f, 0.28f, 0.52f }, jade);

    for (int k = -2; k <= 2; k++)
    {
        float alto = 0.5f + 0.1f * (float)(2 - (k < 0 ? -k : k));
        CuboJade({ (float)k * 0.16f, 3.2f + alto * 0.5f, -0.16f }, { 0.13f, alto, 0.1f }, (k & 1) ? jadeClaro : jade);
    }

    rlPopMatrix();
}


// Jaguar sentado de piedra dorada con manchas y ojos de jade.
static void JaguarJade(Vector3 base, float anguloY, float escala)
{
    Color piedra = TinteJade(Color{ 168, 146, 98, 255 }, { base.x, base.y + 1.5f, base.z });
    Color mancha = TonoJade(piedra, 0.5f);
    Color oscura = TinteJade(TonoJade(COLOR_MURO_CAMARA_JADE, 0.8f), base);
    Color ojo = COLOR_JADE_CLARO;

    rlPushMatrix();
    rlTranslatef(base.x, base.y, base.z);
    rlRotatef(anguloY, 0.0f, 1.0f, 0.0f);
    rlScalef(escala, escala, escala);

    CuboJade({ 0.0f, 0.3f, 0.0f }, { 1.3f, 0.6f, 1.8f }, oscura);
    CuboJade({ 0.0f, 0.62f, 0.0f }, { 1.4f, 0.06f, 1.9f }, TonoJade(oscura, 1.2f));
    CuboJade({ 0.0f, 1.05f, -0.35f }, { 0.95f, 0.8f, 1.0f }, piedra);
    CuboJade({ 0.0f, 1.55f, 0.15f }, { 0.78f, 1.0f, 0.62f }, piedra);
    CuboJade({ -0.24f, 1.0f, 0.52f }, { 0.2f, 0.76f, 0.2f }, piedra);
    CuboJade({ 0.24f, 1.0f, 0.52f }, { 0.2f, 0.76f, 0.2f }, piedra);
    CuboJade({ -0.24f, 0.7f, 0.62f }, { 0.24f, 0.12f, 0.3f }, piedra);
    CuboJade({ 0.24f, 0.7f, 0.62f }, { 0.24f, 0.12f, 0.3f }, piedra);
    CuboJade({ 0.0f, 2.2f, 0.32f }, { 0.66f, 0.54f, 0.6f }, piedra);
    CuboJade({ 0.0f, 2.1f, 0.7f }, { 0.4f, 0.3f, 0.3f }, TonoJade(piedra, 1.08f));
    CuboJade({ 0.0f, 2.16f, 0.86f }, { 0.12f, 0.08f, 0.02f }, mancha);
    CuboJade({ -0.22f, 2.54f, 0.24f }, { 0.14f, 0.18f, 0.12f }, piedra);
    CuboJade({ 0.22f, 2.54f, 0.24f }, { 0.14f, 0.18f, 0.12f }, piedra);
    CuboJade({ -0.16f, 2.3f, 0.63f }, { 0.12f, 0.07f, 0.02f }, ojo);
    CuboJade({ 0.16f, 2.3f, 0.63f }, { 0.12f, 0.07f, 0.02f }, ojo);

    // Manchas.
    const float manchas[6][3] =
    {
        { -0.48f, 1.1f, -0.5f }, { 0.48f, 1.25f, -0.2f }, { -0.4f, 1.7f, 0.1f },
        { 0.4f, 1.5f, 0.25f }, { 0.48f, 0.9f, -0.75f }, { -0.48f, 0.85f, -0.05f }
    };
    for (int k = 0; k < 6; k++)
    {
        CuboJade({ manchas[k][0], manchas[k][1], manchas[k][2] }, { 0.03f, 0.12f, 0.12f }, mancha);
    }

    // Cola enroscada sobre el plinto.
    CuboJade({ 0.45f, 0.72f, -0.6f }, { 0.16f, 0.14f, 0.6f }, piedra);
    CuboJade({ 0.5f, 0.72f, -0.05f }, { 0.16f, 0.14f, 0.5f }, piedra);
    CuboJade({ 0.5f, 0.72f, 0.3f }, { 0.16f, 0.14f, 0.2f }, mancha);

    rlPopMatrix();
}


// Vasija de barro con banda pintada.
static void VasijaJade(Vector3 base, float escala, Color barro, int semilla)
{
    Color c = TinteJade(barro, base);
    Color banda = TinteJade((semilla & 1) ? COLOR_JADE_OSCURO : Color{ 60, 36, 26, 255 }, base);
    float e = escala;

    PrismaJade(base, 0.16f * e, 0.2f * e, 0.08f * e, 10, TonoJade(c, 0.8f));
    PrismaJade({ base.x, base.y + 0.08f * e, base.z }, 0.2f * e, 0.38f * e, 0.3f * e, 10, c);
    PrismaJade({ base.x, base.y + 0.38f * e, base.z }, 0.385f * e, 0.38f * e, 0.08f * e, 10, banda);
    PrismaJade({ base.x, base.y + 0.46f * e, base.z }, 0.38f * e, 0.24f * e, 0.22f * e, 10, c);
    PrismaJade({ base.x, base.y + 0.68f * e, base.z }, 0.2f * e, 0.17f * e, 0.12f * e, 10, TonoJade(c, 0.9f));
    PrismaJade({ base.x, base.y + 0.8f * e, base.z }, 0.2f * e, 0.25f * e, 0.05f * e, 10, TonoJade(c, 1.08f));
}


// Estela tallada: losa alta con cartuchos de glifos. anguloY 0 mira a +Z.
static void EstelaJade(Vector3 base, float anguloY, float alto, int semilla)
{
    Color piedra = TinteJade(COLOR_TALLA_JADE, { base.x, base.y + alto * 0.5f, base.z });
    Color hondo = TonoJade(piedra, 0.66f);
    Color jade = TinteJade(COLOR_JADE_OSCURO, base);

    rlPushMatrix();
    rlTranslatef(base.x, base.y, base.z);
    rlRotatef(anguloY, 0.0f, 1.0f, 0.0f);

    CuboJade({ 0.0f, 0.15f, 0.0f }, { 1.5f, 0.3f, 0.8f }, TonoJade(piedra, 0.8f));
    CuboJade({ 0.0f, 0.3f + alto * 0.5f, 0.0f }, { 1.1f, alto, 0.36f }, piedra);
    CuboJade({ 0.0f, 0.3f + alto + 0.12f, 0.0f }, { 0.8f, 0.24f, 0.34f }, piedra);

    int filas = (int)(alto / 0.5f);
    rlBegin(RL_QUADS);
    for (int f = 0; f < filas; f++)
    {
        float y0 = 0.45f + (float)f * 0.5f;
        for (int c = 0; c < 2; c++)
        {
            float x0 = -0.44f + (float)c * 0.46f;
            float z = 0.185f;
            QuadJade({ x0, y0, z }, { x0 + 0.42f, y0, z }, { x0 + 0.42f, y0 + 0.42f, z }, { x0, y0 + 0.42f, z }, hondo);
            Color glifo = Hash01Jade(f * 2 + c, semilla) > 0.6f ? jade : TonoJade(piedra, 0.94f);
            float h = Hash01Jade(f + semilla, c + 3);
            QuadJade({ x0 + 0.08f, y0 + 0.08f, z + 0.005f }, { x0 + 0.34f, y0 + 0.08f, z + 0.005f }, { x0 + 0.34f, y0 + 0.2f + 0.12f * h, z + 0.005f }, { x0 + 0.08f, y0 + 0.2f + 0.12f * h, z + 0.005f }, glifo);
            QuadJade({ x0 + 0.16f, y0 + 0.26f, z + 0.006f }, { x0 + 0.26f, y0 + 0.26f, z + 0.006f }, { x0 + 0.26f, y0 + 0.36f, z + 0.006f }, { x0 + 0.16f, y0 + 0.36f, z + 0.006f }, TonoJade(glifo, 0.8f));
        }
    }
    rlEnd();

    rlPopMatrix();
}


// Ofrenda: cuenco de piedra con gemas de jade, monedas de oro y velas.
static void OfrendaJade(Vector3 base, float t)
{
    Color piedra = TinteJade(COLOR_TALLA_JADE, base);
    Color oro = TinteJade(COLOR_ORO, base);
    Color jade = TinteJade(COLOR_JADE, base);

    PrismaJade(base, 0.3f, 0.36f, 0.2f, 8, TonoJade(piedra, 0.8f), 0.39f);
    PrismaJade({ base.x, base.y + 0.2f, base.z }, 0.36f, 0.6f, 0.22f, 12, piedra);
    DiscoJade({ base.x, base.y + 0.4f, base.z }, 0.54f, TonoJade(piedra, 0.6f), 12);
    GemaJade({ base.x - 0.16f, base.y + 0.5f, base.z + 0.05f }, 0.13f, 0.1f, jade);
    GemaJade({ base.x + 0.15f, base.y + 0.48f, base.z - 0.1f }, 0.11f, 0.09f, jade);
    GemaJade({ base.x + 0.05f, base.y + 0.5f, base.z + 0.2f }, 0.1f, 0.08f, TonoJade(jade, 1.2f));

    for (int k = 0; k < 4; k++)
    {
        float a = (float)k * 1.7f;
        PrismaJade({ base.x + std::cos(a) * 0.32f, base.y + 0.41f, base.z + std::sin(a) * 0.32f }, 0.08f, 0.08f, 0.025f, 8, oro);
    }

    for (int s = -1; s <= 1; s += 2)
    {
        Vector3 v = { base.x + (float)s * 0.75f, base.y, base.z + 0.15f };
        PrismaJade(v, 0.08f, 0.08f, 0.32f, 6, TinteJade(Color{ 236, 226, 196, 255 }, v));
        float f = 0.85f + 0.15f * std::sin(t * 13.0f + (float)s);
        PrismaJade({ v.x, v.y + 0.32f, v.z }, 0.05f, 0.0f, 0.14f * f, 5, Color{ 255, 196, 90, 255 });
    }
}


// Cabeza de serpiente emplumada sobre el bordillo; vierte agua (el chorro
// va con los translucidos). anguloY 0 mira hacia +Z.
static void SerpienteJade(Vector3 base, float anguloY)
{
    Color piedra = TinteJade(COLOR_ESTATUA_JADE, base);
    Color jade = TinteJade(COLOR_JADE, base);
    Color jadeOscuro = TinteJade(COLOR_JADE_OSCURO, base);
    Color oro = TinteJade(COLOR_ORO, base);
    Color colmillo = TinteJade(Color{ 236, 230, 210, 255 }, base);

    rlPushMatrix();
    rlTranslatef(base.x, base.y, base.z);
    rlRotatef(anguloY, 0.0f, 1.0f, 0.0f);

    CuboJade({ 0.0f, 0.3f, -0.3f }, { 1.0f, 0.6f, 1.0f }, TonoJade(piedra, 0.8f));
    CuboJade({ 0.0f, 0.95f, -0.25f }, { 0.8f, 0.8f, 0.9f }, piedra);

    for (int k = 0; k < 8; k++)
    {
        float a = -1.3f + (float)k * 0.37f;
        CuboJade({ std::sin(a) * 0.62f, 1.05f + std::cos(a) * 0.5f, -0.45f }, { 0.26f, 0.26f, 0.14f }, (k & 1) ? jade : jadeOscuro);
    }

    CuboJade({ 0.0f, 1.25f, 0.35f }, { 0.74f, 0.38f, 0.8f }, piedra);
    CuboJade({ 0.0f, 0.82f, 0.38f }, { 0.66f, 0.18f, 0.7f }, TonoJade(piedra, 0.9f));
    CuboJade({ 0.0f, 1.03f, 0.42f }, { 0.5f, 0.2f, 0.6f }, TinteJade(Color{ 24, 30, 26, 255 }, base));
    CuboJade({ -0.18f, 0.97f, 0.7f }, { 0.07f, 0.16f, 0.07f }, colmillo);
    CuboJade({ 0.18f, 0.97f, 0.7f }, { 0.07f, 0.16f, 0.07f }, colmillo);
    CuboJade({ -0.24f, 1.4f, 0.48f }, { 0.12f, 0.1f, 0.14f }, oro);
    CuboJade({ 0.24f, 1.4f, 0.48f }, { 0.12f, 0.1f, 0.14f }, oro);
    CuboJade({ 0.0f, 1.48f, 0.15f }, { 0.5f, 0.1f, 0.4f }, jade);

    rlPopMatrix();
}


// Incensario de bronce colgado de una cadena que baja del techo.
static Vector3 PosicionIncensarioJade(Vector3 anclaje, float largo, float t, float fase)
{
    float a = 0.1f * std::sin(t * 0.9f + fase) * ObtenerFactorMovimientoCamaraMinijuegos();
    return { anclaje.x + std::sin(a) * largo, anclaje.y - std::cos(a) * largo, anclaje.z };
}


static void IncensarioJade(Vector3 anclaje, float largo, float t, float fase)
{
    Vector3 p = PosicionIncensarioJade(anclaje, largo, t, fase);
    Color bronce = TinteJade(COLOR_BRONCE_JADE, p);
    Color oscuro = TonoJade(bronce, 0.55f);

    DrawCylinderEx({ p.x, p.y + 0.55f, p.z }, anclaje, 0.035f, 0.035f, 4, oscuro);
    PrismaJade({ p.x, p.y - 0.1f, p.z }, 0.14f, 0.32f, 0.32f, 8, bronce);
    PrismaJade({ p.x, p.y + 0.22f, p.z }, 0.32f, 0.06f, 0.3f, 8, TonoJade(bronce, 1.1f));
    PrismaJade({ p.x, p.y - 0.2f, p.z }, 0.06f, 0.14f, 0.1f, 8, oscuro);
}


static void EscombrosJade(Vector3 base, int semilla)
{
    Color piedra = TinteJade(COLOR_MURO_CAMARA_JADE, base);

    for (int k = 0; k < 3; k++)
    {
        float h = Hash01Jade(k, semilla);
        float s = 0.3f + 0.35f * h;
        rlPushMatrix();
        rlTranslatef(base.x + (float)(k - 1) * 0.55f, base.y + s * 0.4f, base.z + (h - 0.5f) * 0.6f);
        rlRotatef(40.0f * h + (float)k * 25.0f, 0.0f, 1.0f, 0.0f);
        CuboJade({ 0.0f, 0.0f, 0.0f }, { s * 1.2f, s * 0.8f, s }, TonoJade(piedra, 0.85f + 0.2f * h));
        rlPopMatrix();
    }
}


//==================================================
// POSICIONES DE LA UTILERIA (dependen del tamano del estanque)
//==================================================

static Vector3 PosicionAntorchaJade(int k, float H)
{
    if (k < 4)
    {
        float s = k < 2 ? -1.0f : 1.0f;
        float z = (k % 2 == 0 ? -1.0f : 1.0f) * 0.7f * H;
        return { s * 17.6f, NIVEL_SUELO_JADE, z };
    }

    return { (k == 4 ? -1.0f : 1.0f) * 12.5f, NIVEL_SUELO_JADE, -H - 2.0f };
}


static Vector3 PosicionBraseroJade(int k, float H)
{
    float s = (k & 1) ? 1.0f : -1.0f;

    if (k < 2) return { s * 18.6f, NIVEL_SUELO_JADE, -0.28f * H };
    if (k < 4) return { s * 18.6f, NIVEL_SUELO_JADE, 0.28f * H };
    if (k < 6) return { s * 6.9f, NIVEL_SUELO_JADE, -H - 1.9f };
    if (k < 8) return { s * 6.5f, NIVEL_SUELO_JADE, H + 2.4f };

    // Isletas de la partida de tres.
    return { s * 12.9f, NIVEL_SUELO_JADE, 3.4f };
}


// Con tres jugadores las isletas suman dos braseros.
static int CantidadBraserosJade(int tableros)
{
    return tableros == 3 ? 10 : 8;
}


// Serpientes que vierten agua: con dos losas en las esquinas del fondo;
// con tres o cuatro, en la mitad de los lados (por la calle entre losas).
static void PosicionSerpienteJade(int k, float H, int tableros, Vector3& base, float& angulo, Vector3& boca, Vector3& caida)
{
    float s = k == 0 ? -1.0f : 1.0f;
    float W = MITAD_ESTANQUE_X_JADE;

    if (tableros <= 2)
    {
        base = { s * (W + 1.0f), NIVEL_SUELO_JADE, -H - 1.0f };
        angulo = s < 0.0f ? 45.0f : -45.0f;
        float d = 0.7071f;
        boca = { base.x - s * d * 0.75f, base.y + 1.0f, base.z + d * 0.75f };
        caida = { base.x - s * d * 2.3f, NIVEL_AGUA_JADE, base.z + d * 2.3f };
        return;
    }

    base = { s * (W + 0.55f), NIVEL_SUELO_JADE, 0.0f };
    angulo = s < 0.0f ? 90.0f : -90.0f;
    boca = { base.x - s * 0.75f, base.y + 1.0f, 0.0f };
    caida = { base.x - s * 2.1f, NIVEL_AGUA_JADE, 0.0f };
}


//==================================================
// ESCENARIO
//==================================================

static void DibujarEscenaTemploJade(const MinijuegoLaberintoInclinado& m, float H, float t)
{
    bool decoracion = CalidadDecoracion() != CALIDAD_BAJA;
    int paso = CalidadDecoracion() == CALIDAD_ALTA ? 1 : 2;

    // Casco del templo (GLB): queda detras de los muros nuevos y su piso
    // bajo el fondo del estanque.
    DibujarModeloLaberintoJadeRetro3D(MODELO_JADE_TEMPLO, { 0.0f, -4.5f, 0.0f }, 0.0f, { 1.5f, 1.5f, 1.5f });

    DibujarPisoCamaraJade(H);
    DibujarEstanqueJade(m, H);
    DibujarBordeEstanqueJade(H);
    DibujarMurosCamaraJade(H, paso);
    DibujarAltarIdoloJade(H);

    // Columnas del templo (GLB) en las esquinas del paseo.
    for (int k = 0; k < 4; k++)
    {
        float x = (k & 1) ? 20.4f : -20.4f;
        float z = k < 2 ? -H - 1.6f : H + 1.8f;

        if (!DibujarModeloLaberintoJadeRetro3D(MODELO_JADE_COLUMNA, { x, NIVEL_SUELO_JADE + 0.2f, z }))
        {
            ColumnaRelieveJade({ x, NIVEL_SUELO_JADE, z }, 10.0f, k);
        }
    }

    // Antorchas (GLB) con su llama que titila.
    for (int k = 0; k < 6; k++)
    {
        Vector3 p = PosicionAntorchaJade(k, H);
        float parpadeo = 0.5f + 0.5f * std::sin(t * 11.0f + (float)k * 1.9f);

        if (!DibujarModeloLaberintoJadeRetro3D(MODELO_JADE_ANTORCHA, p))
        {
            PrismaJade(p, 0.2f, 0.12f, 1.6f, 8, TinteJade(Color{ 70, 52, 36, 255 }, p));
            PrismaJade({ p.x, p.y + 1.6f, p.z }, 0.12f, 0.28f, 0.2f, 8, TinteJade(COLOR_BRONCE_JADE, p));
        }

        float escalaLlama = (0.34f + 0.08f * parpadeo) / 0.34f;
        Vector3 llama = { p.x, p.y + 1.9f + 0.08f * parpadeo, p.z };

        if (!DibujarModeloLaberintoJadeRetro3D(MODELO_JADE_LLAMA, llama, 0.0f, { escalaLlama, escalaLlama, escalaLlama }))
        {
            PrismaJade({ llama.x, llama.y - 0.3f, llama.z }, 0.26f * escalaLlama, 0.0f, 0.8f * escalaLlama, 6, Color{ 255, (unsigned char)(140 + 60 * parpadeo), 40, 255 });
        }
    }

    if (!decoracion)
    {
        return;
    }

    // Columnas con relieves a los lados del estanque.
    for (int s = -1; s <= 1; s += 2)
    {
        for (int z = -1; z <= 1; z += 2)
        {
            ColumnaRelieveJade({ (float)s * 20.8f, NIVEL_SUELO_JADE, (float)z * 0.5f * H }, 10.0f, s * 3 + z);
        }
    }

    for (int k = 0; k < CantidadBraserosJade(m.cantidadTableros); k += paso)
    {
        BraseroJade(PosicionBraseroJade(k, H), t, k);
    }

    // Serpientes que vierten agua al estanque.
    for (int k = 0; k < 2; k++)
    {
        Vector3 base, boca, caida;
        float angulo = 0.0f;
        PosicionSerpienteJade(k, H, m.cantidadTableros, base, angulo, boca, caida);
        SerpienteJade(base, angulo);
    }

    // Guardianes: al pie del altar y en la entrada, mirando al estanque.
    for (int s = -1; s <= 1; s += 2)
    {
        EstatuaGuardianJade({ (float)s * 9.2f, NIVEL_SUELO_JADE, -H - 3.3f }, 0.0f, 1.15f);
        EstatuaGuardianJade({ (float)s * 11.0f, NIVEL_SUELO_JADE, H + 3.4f }, 0.0f, 1.15f);
        JaguarJade({ (float)s * 21.0f, NIVEL_SUELO_JADE, 0.85f * H }, s < 0 ? 90.0f : -90.0f, 1.1f);
        JaguarJade({ (float)s * 21.0f, NIVEL_SUELO_JADE, -0.85f * H }, s < 0 ? 90.0f : -90.0f, 1.1f);
    }

    // Estelas contra los muros.
    for (int s = -1; s <= 1; s += 2)
    {
        EstelaJade({ (float)s * 22.0f, NIVEL_SUELO_JADE, 0.0f }, s < 0 ? 90.0f : -90.0f, 2.6f, 3 + s);
        EstelaJade({ (float)s * 18.8f, NIVEL_SUELO_JADE, -H - 4.2f }, 0.0f, 2.2f, 7 + s);
        EstelaJade({ (float)s * 18.5f, NIVEL_SUELO_JADE, H + 3.6f }, 0.0f, 2.2f, 11 + s);
    }

    // Vasijas en grupos.
    const float vasijas[12][3] =
    {
        { -21.6f, 0.0f, 0.36f }, { -21.9f, 0.0f, 0.44f }, { 21.7f, 0.0f, -0.38f }, { 21.5f, 0.0f, -0.46f },
        { -15.4f, 0.0f, -1.0f }, { -16.1f, 0.0f, -1.0f }, { 15.6f, 0.0f, -1.0f },
        { -3.0f, 0.0f, 1.0f }, { 3.0f, 0.0f, 1.0f }, { -14.8f, 0.0f, 1.0f }, { 15.6f, 0.0f, 1.0f }, { 14.9f, 0.0f, 1.0f }
    };
    // Las de la tercera columna son fraccion de H (lado) o +/-1 (fondo/frente).
    for (int k = 0; k < 12; k += paso)
    {
        float z;
        if (k < 4) z = vasijas[k][2] * H;
        else if (k < 7) z = -H - 3.4f - 0.5f * Hash01Jade(k, 3);
        else z = H + 1.9f + 0.7f * Hash01Jade(k, 4);

        Vector3 p = { vasijas[k][0], NIVEL_SUELO_JADE, z };
        Color barro = (k % 3 == 0) ? COLOR_TERRACOTA_JADE : ((k % 3 == 1) ? Color{ 196, 156, 104, 255 } : Color{ 128, 82, 58, 255 });
        VasijaJade(p, 1.0f + 0.5f * Hash01Jade(k, 9), barro, k);
    }

    // Ofrendas: en la grada del altar, en la entrada y junto a cada mando.
    OfrendaJade({ 0.0f, NIVEL_SUELO_JADE + 1.2f, -H - 2.6f }, t);
    OfrendaJade({ 0.0f, NIVEL_SUELO_JADE, H + 2.2f }, t);

    for (int s = -1; s <= 1; s += 2)
    {
        EscombrosJade({ (float)s * 9.0f, NIVEL_SUELO_JADE, H + 5.4f }, 4 + s);
        EscombrosJade({ (float)s * 21.4f, NIVEL_SUELO_JADE, -0.6f * H }, 8 + s);
    }

    // Tres jugadores: las isletas a los lados de la losa central tienen su
    // propio santuario (el operador de esa losa va en la izquierda).
    if (m.cantidadTableros == 3)
    {
        EstatuaGuardianJade({ -12.7f, NIVEL_SUELO_JADE, H - 2.2f }, 90.0f, 1.1f);
        VasijaJade({ -13.6f, NIVEL_SUELO_JADE, 8.6f }, 1.3f, COLOR_TERRACOTA_JADE, 1);
        VasijaJade({ -13.3f, NIVEL_SUELO_JADE, 9.5f }, 1.0f, Color{ 196, 156, 104, 255 }, 2);
        OfrendaJade({ -10.6f, NIVEL_SUELO_JADE, H - 1.6f }, t);
        EscombrosJade({ -12.4f, NIVEL_SUELO_JADE, 5.6f }, 13);

        JaguarJade({ 11.6f, NIVEL_SUELO_JADE, 7.4f }, -90.0f, 1.1f);
        EstatuaGuardianJade({ 12.7f, NIVEL_SUELO_JADE, H - 2.2f }, -90.0f, 1.1f);
        EstelaJade({ 9.4f, NIVEL_SUELO_JADE, H - 1.4f }, 0.0f, 2.2f, 17);
        VasijaJade({ 9.0f, NIVEL_SUELO_JADE, 4.2f }, 1.2f, Color{ 128, 82, 58, 255 }, 3);
        OfrendaJade({ 10.2f, NIVEL_SUELO_JADE, 10.6f }, t);
    }

    // Incensarios colgados del techo en sombra.
    for (int s = -1; s <= 1; s += 2)
    {
        IncensarioJade({ (float)s * 18.6f, 16.0f, 0.0f }, 12.6f, t, (float)s);
        if (paso == 1) IncensarioJade({ (float)s * 9.0f, 16.0f, H + 4.2f }, 13.0f, t, (float)s * 2.0f);
    }
}


// Operadores: dais con anillo de su color, mando de piedra con palanca
// que se inclina como su losa y el personaje, que acompana la inclinacion,
// salta al llegar al altar y se sacude al caer la esfera.
static void DibujarOperadoresJade(
    const MinijuegoLaberintoInclinado& m,
    int limite,
    const Participante participantes[],
    float t
)
{
    float escala = m.cantidadTableros <= 2 ? 1.1f : 1.2f;

    for (int i = 0; i < limite; i++)
    {
        if (!m.resultado.participantes[i].participo)
        {
            continue;
        }

        const EstadoJugadorLaberinto& e = m.estadosJugadores[i];
        const VisualJugadorLaberinto& v = m.visuales[i];
        Color color = ColorJugadorJade(participantes, i);
        Vector3 pies, consola;
        float angulo = 0.0f;
        PosicionOperadorJade(m, i, pies, consola, angulo);

        Color piedra = TinteJade(COLOR_TALLA_JADE, pies);
        PrismaJade(pies, 0.85f, 0.78f, 0.16f, 12, TonoJade(piedra, 0.85f));
        AnilloJade({ pies.x, pies.y + 0.165f, pies.z }, 0.6f, 0.76f, color, 20);

        // Mando: pedestal con banda del jugador y palanca con pomo de jade.
        CuboJade({ consola.x, consola.y + 0.5f, consola.z }, { 0.6f, 1.0f, 0.6f }, piedra);
        CuboJade({ consola.x, consola.y + 0.78f, consola.z }, { 0.64f, 0.14f, 0.64f }, color);
        CuboJade({ consola.x, consola.y + 1.04f, consola.z }, { 0.74f, 0.08f, 0.74f }, TonoJade(piedra, 1.1f));
        Vector3 rotula = { consola.x, consola.y + 1.1f, consola.z };
        Vector3 pomo = { rotula.x + e.inclinacionX * 0.32f, rotula.y + 0.5f, rotula.z + e.inclinacionZ * 0.32f };
        DrawCylinderEx(rotula, pomo, 0.05f, 0.05f, 6, TinteJade(COLOR_BRONCE_JADE, rotula));
        GemaJade(pomo, 0.12f, 0.1f, COLOR_JADE, 6);

        if (CalidadSombras() != CALIDAD_BAJA)
        {
            DibujarSombraRetroCircular({ pies.x, pies.y + 0.17f, pies.z }, 0.5f, 0.5f, pies.y + 0.18f, 0.3f, 10);
        }

        float salto = 0.0f;
        float sacudida = 0.0f;

        if (e.llego)
        {
            salto = 0.45f * std::fabs(std::sin(t * 7.0f + (float)i)) * ObtenerFactorMovimientoCamaraMinijuegos();
        }

        if (v.destelloCaida > 0.0f)
        {
            sacudida = std::sin(t * 60.0f) * 0.07f * v.destelloCaida * ObtenerFactorMovimientoCamaraMinijuegos();
        }

        rlPushMatrix();
        rlTranslatef(pies.x + sacudida, pies.y + 0.17f + salto, pies.z);
        rlRotatef(-e.inclinacionX * 7.0f, 0.0f, 0.0f, 1.0f);
        rlRotatef(e.inclinacionZ * 7.0f, 1.0f, 0.0f, 0.0f);
        DibujarModeloJugadorEnPosicion(ObtenerPersonajeParticipante(participantes[i]), { 0.0f, 0.0f, 0.0f }, angulo, color, ESCALA_MODELO_JUGADOR_3D * escala);
        rlPopMatrix();
    }
}


//==================================================
// LOSAS
//==================================================

static void DibujarTrampaVisual(
    const MinijuegoLaberintoInclinado& m,
    const TrampaDardosLaberinto& trampa,
    float t
)
{
    float avance = 0.0f;
    int estado = EstadoTrampa(trampa, m.tiempoJuego, avance);
    bool horizontal = std::fabs(trampa.direccionX) > 0.5f;
    // La flecha GLB apunta a +X. La direccion ya esta transformada por el
    // diseno logico (base, transpuesto o girado); no rotar el mapa otra vez.
    float anguloFlecha = std::atan2(-trampa.direccionZ, trampa.direccionX) * RAD2DEG;

    // Flechas grabadas en el suelo del pasillo: rojas en el aviso, naranjas al disparar.
    for (int r = trampa.filaInicio; r <= trampa.filaFin; r++)
    {
        for (int c = trampa.columnaInicio; c <= trampa.columnaFin; c++)
        {
            Color color = Color{ 92, 70, 52, 255 };

            if (estado == 1)
            {
                float parpadeo = 0.5f + 0.5f * std::sin(t * 18.0f);
                color = Color{ 235, (unsigned char)(50 + 40 * parpadeo), 30, 255 };
            }
            else if (estado == 2)
            {
                color = Color{ 255, 150, 40, 255 };
            }

            Vector3 centro = PosicionLocal((float)c + 0.5f, (float)r + 0.5f, 0.05f);

            if (!DibujarModeloLaberintoJadeRetro3D(MODELO_JADE_FLECHA, centro, anguloFlecha, { 1.0f, 1.0f, 1.0f }, color))
            {
                rlPushMatrix();
                rlTranslatef(centro.x, centro.y, centro.z);
                rlRotatef(anguloFlecha, 0.0f, 1.0f, 0.0f);
                rlBegin(RL_TRIANGLES);
                VerticeJade({ 0.3f, 0.0f, 0.0f }, color);
                VerticeJade({ -0.1f, 0.0f, -0.22f }, color);
                VerticeJade({ -0.1f, 0.0f, 0.22f }, color);
                rlEnd();
                CuboJade({ -0.2f, 0.0f, 0.0f }, { 0.24f, 0.01f, 0.1f }, color);
                rlPopMatrix();
            }
        }
    }

    // Boquillas en los muros del pasillo (brillan en el aviso).
    int medioC = (trampa.columnaInicio + trampa.columnaFin) / 2;
    int medioR = (trampa.filaInicio + trampa.filaFin) / 2;
    Color boquilla = estado == 0 ? Color{ 40, 34, 30, 255 } : Color{ 255, 70, 40, 255 };

    Vector3 posiciones[2];
    float angulos[2];

    if (horizontal)
    {
        // Frente GLB +Z: ambas boquillas miran hacia el interior del pasillo.
        posiciones[0] = PosicionLocal((float)medioC + 0.5f, (float)trampa.filaInicio - 0.02f, 0.02f);
        posiciones[1] = PosicionLocal((float)medioC + 0.5f, (float)trampa.filaFin + 1.02f, 0.02f);
        angulos[0] = 0.0f;
        angulos[1] = 180.0f;
    }
    else
    {
        posiciones[0] = PosicionLocal((float)trampa.columnaInicio - 0.02f, (float)medioR + 0.5f, 0.02f);
        posiciones[1] = PosicionLocal((float)trampa.columnaFin + 1.02f, (float)medioR + 0.5f, 0.02f);
        angulos[0] = 90.0f;
        angulos[1] = -90.0f;
    }

    for (int k = 0; k < 2; k++)
    {
        if (!DibujarModeloLaberintoJadeRetro3D(MODELO_JADE_BOQUILLA, posiciones[k], angulos[k], { 1.0f, 1.0f, 1.0f }, boquilla))
        {
            Vector3 p = posiciones[k];
            CuboJade({ p.x, 0.3f, p.z }, horizontal ? Vector3{ 0.4f, 0.26f, 0.1f } : Vector3{ 0.1f, 0.26f, 0.4f }, boquilla);
        }
    }

    // Dardo en vuelo durante el disparo: asta de madera, punta de bronce y
    // plumas rojas, con la cola apuntando al muro de donde salio.
    if (estado == 2)
    {
        float inicioX = trampa.direccionX > 0.0f ? (float)trampa.columnaInicio : (float)(trampa.columnaFin + 1);
        float inicioZ = trampa.direccionZ > 0.0f ? (float)trampa.filaInicio : (float)(trampa.filaFin + 1);
        float finX = trampa.direccionX > 0.0f ? (float)(trampa.columnaFin + 1) : (float)trampa.columnaInicio;
        float finZ = trampa.direccionZ > 0.0f ? (float)(trampa.filaFin + 1) : (float)trampa.filaInicio;

        if (horizontal)
        {
            inicioZ = (float)trampa.filaInicio + 0.5f;
            finZ = inicioZ;
        }
        else
        {
            inicioX = (float)trampa.columnaInicio + 0.5f;
            finX = inicioX;
        }

        float dx = inicioX + (finX - inicioX) * avance;
        float dz = inicioZ + (finZ - inicioZ) * avance;
        Vector3 p = PosicionLocal(dx, dz, 0.36f);

        rlPushMatrix();
        rlTranslatef(p.x, p.y, p.z);
        rlRotatef(anguloFlecha, 0.0f, 1.0f, 0.0f);
        CuboJade({ -0.1f, 0.0f, 0.0f }, { 0.62f, 0.07f, 0.07f }, Color{ 150, 108, 66, 255 });
        CuboJade({ 0.26f, 0.0f, 0.0f }, { 0.14f, 0.12f, 0.12f }, Color{ 255, 206, 96, 255 });
        CuboJade({ -0.36f, 0.06f, 0.0f }, { 0.16f, 0.1f, 0.02f }, Color{ 220, 60, 46, 255 });
        CuboJade({ -0.36f, -0.0f, 0.06f }, { 0.16f, 0.02f, 0.1f }, Color{ 200, 50, 40, 255 });
        rlPopMatrix();
    }
}


// Losa completa en coordenadas locales (origen en su centro).
static void DibujarTableroVisual(
    const MinijuegoLaberintoInclinado& m,
    int indiceJugador,
    bool mostrarDebug,
    float t
)
{
    const EstadoJugadorLaberinto& e = m.estadosJugadores[indiceJugador];
    Color colorJugador = COLORES_JUGADORES_LABERINTO[indiceJugador % MAX_PARTICIPANTES];

    // Marco de piedra con borde de jade (GLB) o su equivalente tallado.
    bool losaGLB = DibujarModeloLaberintoJadeRetro3D(MODELO_JADE_LOSA, { 0.0f, 0.0f, 0.0f });

    if (!losaGLB)
    {
        CuboJade({ 0.0f, -0.3f, 0.0f }, { 12.0f, 0.6f, 12.0f }, COLOR_PIEDRA_OSCURA);
        CuboJade({ 0.0f, 0.12f, -5.75f }, { 12.0f, 0.24f, 0.5f }, COLOR_JADE);
        CuboJade({ 0.0f, 0.12f, 5.75f }, { 12.0f, 0.24f, 0.5f }, COLOR_JADE);
        CuboJade({ -5.75f, 0.12f, 0.0f }, { 0.5f, 0.24f, 11.0f }, COLOR_JADE);
        CuboJade({ 5.75f, 0.12f, 0.0f }, { 0.5f, 0.24f, 11.0f }, COLOR_JADE);
    }

    // Banda del color del jugador en el frente de la losa.
    if (!DibujarModeloLaberintoJadeRetro3D(MODELO_JADE_BANDA, { 0.0f, -0.3f, 6.02f }, 0.0f, { 1.0f, 1.0f, 1.0f }, colorJugador))
    {
        CuboJade({ 0.0f, -0.3f, 6.02f }, { 12.0f, 0.3f, 0.06f }, colorJugador);
    }

    for (int r = 0; r < FILAS_LABERINTO; r++)
    {
        for (int c = 0; c < COLUMNAS_LABERINTO; c++)
        {
            TipoCeldaLaberinto tipo = m.celdas[r][c];
            Vector3 centro = PosicionLocal((float)c + 0.5f, (float)r + 0.5f, 0.0f);

            if (tipo == CELDA_LABERINTO_MURO)
            {
                if (!DibujarModeloLaberintoJadeRetro3D(MODELO_JADE_MURO, centro))
                {
                    CuboJade({ centro.x, ALTO_MURO * 0.5f, centro.z }, { 0.98f, ALTO_MURO, 0.98f }, COLOR_PIEDRA);
                    CuboJade({ centro.x, ALTO_MURO - 0.04f, centro.z }, { 1.0f, 0.08f, 1.0f }, COLOR_PIEDRA_CLARA);
                }

                // Glifos dorados sobre algunos bloques.
                if ((c * 7 + r * 3) % 5 == 0)
                {
                    if (!DibujarModeloLaberintoJadeRetro3D(MODELO_JADE_GLIFO, { centro.x, 0.61f, centro.z }))
                    {
                        CuboJade({ centro.x, ALTO_MURO + 0.015f, centro.z }, { 0.36f, 0.03f, 0.36f }, COLOR_ORO);
                        CuboJade({ centro.x, ALTO_MURO + 0.03f, centro.z }, { 0.12f, 0.03f, 0.12f }, COLOR_PIEDRA_OSCURA);
                    }
                }
            }
            else if (tipo == CELDA_LABERINTO_AGUJERO)
            {
                if (!DibujarModeloLaberintoJadeRetro3D(MODELO_JADE_AGUJERO, centro))
                {
                    PrismaJade({ centro.x, 0.0f, centro.z }, 0.5f, 0.46f, 0.06f, 16, Color{ 150, 96, 54, 255 });
                    DiscoJade({ centro.x, 0.065f, centro.z }, RADIO_AGUJERO + 0.02f, Color{ 8, 6, 12, 255 }, 16);
                }
            }
            else if (!losaGLB)
            {
                Color losa = (c + r) % 2 == 0 ? COLOR_LOSA_CLARA : COLOR_LOSA;
                CuboJade({ centro.x, 0.015f, centro.z }, { 0.96f, 0.03f, 0.96f }, losa);
            }
        }
    }

    // Salida.
    Vector3 salida = PosicionLocal((float)m.inicioColumna + 0.5f, (float)m.inicioFila + 0.5f, 0.05f);
    if (!DibujarModeloLaberintoJadeRetro3D(MODELO_JADE_SALIDA, salida))
    {
        AnilloJade({ salida.x, 0.04f, salida.z }, 0.28f, 0.4f, Color{ 40, 120, 90, 255 }, 20);
    }

    // Checkpoints: disco dorado que se vuelve jade al activarse.
    for (int k = 0; k < MAX_PUNTOS_CONTROL_LABERINTO; k++)
    {
        bool activo = e.puntosControl > k;
        Color color = activo ? COLOR_JADE : COLOR_ORO;
        Vector3 centro = PosicionLocal((float)m.controlColumna[k] + 0.5f, (float)m.controlFila[k] + 0.5f, 0.05f);

        if (!DibujarModeloLaberintoJadeRetro3D(MODELO_JADE_CHECKPOINT, centro, 0.0f, { 1.0f, 1.0f, 1.0f }, color))
        {
            AnilloJade({ centro.x, 0.04f, centro.z }, 0.26f, 0.42f, color, 20);
            PrismaJade({ centro.x, 0.05f, centro.z }, 0.05f, 0.05f, 0.5f, 6, COLOR_PIEDRA_CLARA);
            GemaJade({ centro.x, 0.62f, centro.z }, 0.12f, 0.1f, color);
        }
    }

    // Altar de la meta.
    Vector3 meta = PosicionLocal((float)m.metaColumna + 0.5f, (float)m.metaFila + 0.5f, 0.0f);
    if (!DibujarModeloLaberintoJadeRetro3D(MODELO_JADE_ALTAR, meta))
    {
        CuboJade({ meta.x, 0.1f, meta.z }, { 0.85f, 0.2f, 0.85f }, COLOR_PIEDRA_CLARA);
        CuboJade({ meta.x, 0.3f, meta.z }, { 0.55f, 0.2f, 0.55f }, COLOR_ORO);
        GemaJade({ meta.x, 0.62f, meta.z }, 0.18f, 0.14f, COLOR_ORO);
    }

    for (int i = 0; i < m.cantidadTrampas; i++)
    {
        DibujarTrampaVisual(m, m.trampas[i], t);
    }

    // Esfera de jade: sombra suave, anillo del color del jugador y la esfera.
    float escala = 1.0f;

    if (e.tiempoCaida > 0.0f)
    {
        escala = Acotar(e.tiempoCaida / DURACION_CAIDA, 0.0f, 1.0f);
    }

    Vector3 esfera = PosicionLocal(e.x, e.z, RADIO_ESFERA * escala + 0.03f);

    if (e.tiempoCaida <= 0.0f)
    {
        rlDrawRenderBatchActive();
        rlDisableDepthMask();
        DiscoJade({ esfera.x + 0.06f, 0.07f, esfera.z + 0.06f }, 0.34f, Color{ 0, 0, 0, 80 }, 16);
        rlDrawRenderBatchActive();
        rlEnableDepthMask();
    }

    AnilloJade({ esfera.x, 0.08f, esfera.z }, 0.3f * escala, 0.47f * escala, colorJugador, 20);

    // La esfera GLB ya mide radio .28 y tiene origen central. Su escala
    // uniforme sigue la caida.
    if (!DibujarModeloLaberintoJadeRetro3D(MODELO_JADE_ESFERA, esfera, 0.0f, { escala, escala, escala }))
    {
        DrawSphere(esfera, RADIO_ESFERA * escala, e.llego ? COLOR_ORO : COLOR_JADE);
        DrawSphere({ esfera.x - 0.08f, esfera.y + 0.1f, esfera.z - 0.08f }, 0.07f * escala, Color{ 200, 255, 225, 255 });
    }

    if (mostrarDebug)
    {
        DrawSphereWires(esfera, RADIO_ESFERA, 8, 8, RED);
    }
}


// Luces aditivas de una losa (dentro de su matriz): halo y estela de la
// esfera, columna dorada del altar, checkpoints, aviso de los dardos.
static void DibujarLucesTableroJade(const MinijuegoLaberintoInclinado& m, int i, float t)
{
    const EstadoJugadorLaberinto& e = m.estadosJugadores[i];
    const VisualJugadorLaberinto& v = m.visuales[i];

    // Altar: charco y columna de luz dorada (el acento de la escena).
    Vector3 meta = PosicionLocal((float)m.metaColumna + 0.5f, (float)m.metaFila + 0.5f, 0.0f);
    float pulso = 0.8f + 0.2f * std::sin(t * 3.0f);
    float fuerza = 1.0f + 1.5f * v.destelloMeta + (e.llego ? 0.6f : 0.0f);
    ResplandorSueloJade({ meta.x, 0.06f, meta.z }, 1.3f * pulso, Color{ 255, 206, 90, (unsigned char)(70.0f * fuerza > 255.0f ? 255.0f : 70.0f * fuerza) });
    ConoLuzJade({ meta.x, 3.4f, meta.z }, { meta.x, 0.7f, meta.z }, 0.6f, 0.34f, Color{ 255, 214, 110, (unsigned char)(36.0f * fuerza * pulso) });
    ResplandorFrenteJade({ meta.x, 0.95f, meta.z }, 0.7f * pulso, Color{ 255, 226, 140, (unsigned char)(90.0f * pulso) });

    // Checkpoints: el siguiente late en oro; los activos quedan en jade.
    for (int k = 0; k < MAX_PUNTOS_CONTROL_LABERINTO; k++)
    {
        Vector3 c = PosicionLocal((float)m.controlColumna[k] + 0.5f, (float)m.controlFila[k] + 0.5f, 0.0f);

        if (e.puntosControl > k)
        {
            ResplandorSueloJade({ c.x, 0.08f, c.z }, 0.9f, Color{ 90, 255, 180, 60 });
            ResplandorFrenteJade({ c.x, 0.7f, c.z }, 0.45f + 0.5f * v.destelloControl, Color{ 140, 255, 200, (unsigned char)(70.0f + 120.0f * v.destelloControl) });
        }
        else if (e.puntosControl == k && !e.llego)
        {
            float late = 0.5f + 0.5f * std::sin(t * 5.0f);
            AnilloJade({ c.x, 0.09f, c.z }, 0.46f + 0.1f * late, 0.56f + 0.14f * late, Color{ 255, 210, 100, (unsigned char)(60.0f + 70.0f * late) }, 20);
        }
    }

    // Dardos: el pasillo se enciende en rojo durante el aviso y la estela
    // del dardo cruza al disparar.
    for (int k = 0; k < m.cantidadTrampas; k++)
    {
        const TrampaDardosLaberinto& trampa = m.trampas[k];
        float avance = 0.0f;
        int estado = EstadoTrampa(trampa, m.tiempoJuego, avance);
        if (estado == 0) continue;

        float x0 = (float)trampa.columnaInicio - MITAD_TABLERO, x1 = (float)(trampa.columnaFin + 1) - MITAD_TABLERO;
        float z0 = (float)trampa.filaInicio - MITAD_TABLERO, z1 = (float)(trampa.filaFin + 1) - MITAD_TABLERO;
        float late = estado == 1 ? 0.5f + 0.5f * std::sin(t * 18.0f) : 1.0f;
        Color rojo = estado == 1 ? Color{ 255, 60, 30, (unsigned char)(26.0f + 40.0f * late) } : Color{ 255, 150, 50, 70 };

        rlBegin(RL_QUADS);
        VerticeJade({ x0 + 0.06f, 0.09f, z0 + 0.06f }, rojo);
        VerticeJade({ x0 + 0.06f, 0.09f, z1 - 0.06f }, rojo);
        VerticeJade({ x1 - 0.06f, 0.09f, z1 - 0.06f }, rojo);
        VerticeJade({ x1 - 0.06f, 0.09f, z0 + 0.06f }, rojo);
        rlEnd();

        int medioC = (trampa.columnaInicio + trampa.columnaFin) / 2;
        int medioR = (trampa.filaInicio + trampa.filaFin) / 2;
        bool horizontal = std::fabs(trampa.direccionX) > 0.5f;
        Vector3 b0 = horizontal ? PosicionLocal((float)medioC + 0.5f, (float)trampa.filaInicio, 0.32f) : PosicionLocal((float)trampa.columnaInicio, (float)medioR + 0.5f, 0.32f);
        Vector3 b1 = horizontal ? PosicionLocal((float)medioC + 0.5f, (float)trampa.filaFin + 1.0f, 0.32f) : PosicionLocal((float)trampa.columnaFin + 1.0f, (float)medioR + 0.5f, 0.32f);
        ResplandorFrenteJade(b0, 0.45f * late + 0.15f, Color{ 255, 80, 40, (unsigned char)(150.0f * late) });
        ResplandorFrenteJade(b1, 0.45f * late + 0.15f, Color{ 255, 80, 40, (unsigned char)(150.0f * late) });

        if (estado == 2)
        {
            float dx = trampa.direccionX, dz = trampa.direccionZ;
            float sx = horizontal ? (dx > 0.0f ? x0 : x1) : ((float)trampa.columnaInicio + 0.5f - MITAD_TABLERO);
            float sz = horizontal ? ((float)trampa.filaInicio + 0.5f - MITAD_TABLERO) : (dz > 0.0f ? z0 : z1);
            float largo = horizontal ? (x1 - x0) : (z1 - z0);
            float px = sx + dx * largo * avance;
            float pz = sz + dz * largo * avance;
            float cola = 1.4f;
            Color estela = { 255, 200, 110, 140 };
            Color nada = { 255, 200, 110, 0 };

            rlBegin(RL_QUADS);
            VerticeJade({ px - dx * cola - dz * 0.06f, 0.36f, pz - dz * cola - dx * 0.06f }, nada);
            VerticeJade({ px - dx * cola + dz * 0.06f, 0.36f, pz - dz * cola + dx * 0.06f }, nada);
            VerticeJade({ px + dz * 0.06f, 0.36f, pz + dx * 0.06f }, estela);
            VerticeJade({ px - dz * 0.06f, 0.36f, pz - dx * 0.06f }, estela);
            rlEnd();
            ResplandorFrenteJade({ px, 0.36f, pz }, 0.35f, Color{ 255, 220, 140, 160 });
        }
    }

    // Esfera: halo de jade (oro al llegar) y estela si va rapido.
    if (e.tiempoCaida > 0.0f)
    {
        return;
    }

    Vector3 esfera = PosicionLocal(e.x, e.z, RADIO_ESFERA + 0.03f);
    Color halo = e.llego ? Color{ 255, 214, 110, 110 } : Color{ 120, 255, 190, 110 };
    ResplandorFrenteJade(esfera, 0.9f + 0.3f * v.destelloControl, halo);
    ResplandorSueloJade({ esfera.x, 0.08f, esfera.z }, 0.7f, Color{ halo.r, halo.g, halo.b, 40 });

    float rapidez = std::sqrt(e.velocidadX * e.velocidadX + e.velocidadZ * e.velocidadZ);

    if (rapidez > 1.2f && !e.llego)
    {
        float ux = e.velocidadX / rapidez, uz = e.velocidadZ / rapidez;
        float largo = rapidez * 0.16f;
        Color c0 = { 140, 255, 200, 90 };
        Color c1 = { 140, 255, 200, 0 };

        rlBegin(RL_QUADS);
        VerticeJade({ esfera.x - uz * 0.2f, esfera.y, esfera.z + ux * 0.2f }, c0);
        VerticeJade({ esfera.x - ux * largo - uz * 0.05f, esfera.y, esfera.z - uz * largo + ux * 0.05f }, c1);
        VerticeJade({ esfera.x - ux * largo + uz * 0.05f, esfera.y, esfera.z - uz * largo - ux * 0.05f }, c1);
        VerticeJade({ esfera.x + uz * 0.2f, esfera.y, esfera.z - ux * 0.2f }, c0);
        rlEnd();
    }
}


//==================================================
// AGUA, HUMO Y LUCES
//==================================================

// Lotos, flores y velas flotantes en el agua libre (no bajo las losas).
static int PosicionesLotosJade(const MinijuegoLaberintoInclinado& m, float H, Vector3 salida[], int tipos[], int maximo)
{
    float W = MITAD_ESTANQUE_X_JADE;
    int n = 0;

    for (int k = 0; k < 90 && n < maximo; k++)
    {
        float x = -W + 0.5f + (2.0f * W - 1.0f) * Hash01Jade(k, 401);
        float z = -H + 0.5f + (2.0f * H - 1.0f) * Hash01Jade(k, 402);
        bool libre = true;

        for (int b = 0; b < m.cantidadTableros && b < MAX_PARTICIPANTES; b++)
        {
            Vector3 c = m.centrosTableros[b];
            if (std::fabs(x - c.x) < 6.6f && std::fabs(z - c.z) < 6.6f) libre = false;
        }

        if (m.cantidadTableros == 3 && std::fabs(x) > 6.9f && z > 1.1f) libre = false;
        if (!libre) continue;

        salida[n] = { x, NIVEL_AGUA_JADE + 0.03f, z };
        tipos[n] = k % 4;
        n++;
    }

    return n;
}


static void DibujarLotosJade(const MinijuegoLaberintoInclinado& m, float H, float t)
{
    Vector3 lotos[24];
    int tipos[24];
    int n = PosicionesLotosJade(m, H, lotos, tipos, 24);

    for (int k = 0; k < n; k++)
    {
        Vector3 p = lotos[k];
        p.y += 0.02f * std::sin(t * 1.3f + (float)k);
        float r = 0.32f + 0.18f * Hash01Jade(k, 403);
        Color hoja = TinteJade(Color{ 52, 128, 64, 255 }, p);

        DiscoJade(p, r, hoja, 12);
        AnilloJade({ p.x, p.y + 0.004f, p.z }, r * 0.82f, r * 0.9f, TonoJade(hoja, 0.75f), 12);

        if (tipos[k] == 0)
        {
            PrismaJade({ p.x + 0.05f, p.y, p.z }, 0.08f, 0.18f, 0.12f, 6, TinteJade(Color{ 240, 150, 180, 255 }, p), 0.3f);
            DiscoJade({ p.x + 0.05f, p.y + 0.125f, p.z }, 0.06f, Color{ 255, 220, 90, 255 }, 6);
        }
        else if (tipos[k] == 1)
        {
            PrismaJade({ p.x, p.y, p.z }, 0.07f, 0.07f, 0.18f, 6, TinteJade(Color{ 236, 226, 196, 255 }, p));
            float f = 0.85f + 0.15f * std::sin(t * 12.0f + (float)k);
            PrismaJade({ p.x, p.y + 0.18f, p.z }, 0.045f, 0.0f, 0.12f * f, 5, Color{ 255, 196, 90, 255 });
        }
    }
}


static void DibujarAguaJade(float H, float t)
{
    float W = MITAD_ESTANQUE_X_JADE;
    bool animada = CalidadEfectos() != CALIDAD_BAJA;
    float lado = CalidadEfectos() == CALIDAD_ALTA ? 1.2f : 2.4f;
    int nx = (int)std::ceil(2.0f * W / lado);
    int nz = (int)std::ceil(2.0f * H / lado);
    float px = 2.0f * W / (float)nx;
    float pz = 2.0f * H / (float)nz;
    float y = NIVEL_AGUA_JADE;

    auto ColorAgua = [&](float x, float z) -> Color
    {
        float f = 0.92f;
        if (animada) f = 0.86f + 0.16f * std::sin(x * 0.8f + t * 1.3f) * std::cos(z * 0.65f - t * 1.0f);
        Color c = TonoJade(COLOR_AGUA_JADE, f);
        c.a = 200;
        return c;
    };

    rlDrawRenderBatchActive();
    rlDisableDepthMask();
    rlBegin(RL_QUADS);

    for (int i = 0; i < nx; i++)
    {
        for (int j = 0; j < nz; j++)
        {
            float x0 = -W + px * (float)i, x1 = x0 + px;
            float z0 = -H + pz * (float)j, z1 = z0 + pz;
            VerticeJade({ x0, y, z1 }, ColorAgua(x0, z1));
            VerticeJade({ x1, y, z1 }, ColorAgua(x1, z1));
            VerticeJade({ x1, y, z0 }, ColorAgua(x1, z0));
            VerticeJade({ x0, y, z0 }, ColorAgua(x0, z0));
        }
    }

    rlEnd();
    rlDrawRenderBatchActive();
    rlEnableDepthMask();
}


// Translucidos: chorros de las serpientes y humo de incensarios y braseros.
static void DibujarTranslucidosJade(const MinijuegoLaberintoInclinado& m, float H, float t)
{
    rlDrawRenderBatchActive();
    rlDisableDepthMask();

    for (int k = 0; k < 2; k++)
    {
        Vector3 base, boca, caida;
        float angulo = 0.0f;
        PosicionSerpienteJade(k, H, m.cantidadTableros, base, angulo, boca, caida);

        // Chorro en parabola con bandas de brillo que bajan.
        const int TRAMOS = 10;
        rlBegin(RL_QUADS);

        for (int s = 0; s < TRAMOS; s++)
        {
            float u0 = (float)s / (float)TRAMOS;
            float u1 = (float)(s + 1) / (float)TRAMOS;
            auto Punto = [&](float u) -> Vector3
            {
                return {
                    boca.x + (caida.x - boca.x) * u,
                    boca.y + (caida.y - boca.y) * u * u + 0.35f * u * (1.0f - u),
                    boca.z + (caida.z - boca.z) * u
                };
            };
            Vector3 a = Punto(u0), b = Punto(u1);
            float banda = 0.5f + 0.5f * std::sin(u0 * 14.0f - t * 12.0f);
            Color c = { (unsigned char)(150 + 60 * banda), 236, 214, (unsigned char)(120 + 60 * banda) };
            float w0 = 0.12f + 0.06f * u0, w1 = 0.12f + 0.06f * u1;
            QuadJade({ a.x - w0, a.y, a.z }, { a.x + w0, a.y, a.z }, { b.x + w1, b.y, b.z }, { b.x - w1, b.y, b.z }, c);
            QuadJade({ a.x, a.y, a.z - w0 }, { a.x, a.y, a.z + w0 }, { b.x, b.y, b.z + w1 }, { b.x, b.y, b.z - w1 }, c);
        }

        rlEnd();

        // Espuma donde cae.
        AnilloJade({ caida.x, NIVEL_AGUA_JADE + 0.02f, caida.z }, 0.0f, 0.4f, Color{ 220, 250, 240, 150 }, 12);
    }

    // Humo: bocanadas redondas que suben, se abren y se desvanecen.
    if (CalidadDecoracion() != CALIDAD_BAJA)
    {
        int paso = CalidadDecoracion() == CALIDAD_ALTA ? 1 : 2;

        for (int s = -1; s <= 1; s += 2)
        {
            Vector3 fuente = PosicionIncensarioJade({ (float)s * 18.6f, 16.0f, 0.0f }, 12.6f, t, (float)s);

            for (int k = 0; k < 8; k += paso)
            {
                float u = std::fmod(t * 0.22f + Hash01Jade(k, 500 + s), 1.0f);
                float r = 0.2f + 0.7f * u;
                Vector3 p = { fuente.x + std::sin(t * 0.7f + (float)k) * 0.3f * u, fuente.y + 0.4f + 2.6f * u, fuente.z + 0.3f * u };
                ResplandorFrenteJade(p, r, Color{ 196, 210, 200, (unsigned char)(46.0f * std::sin(u * 3.1416f)) });
            }
        }
    }

    rlDrawRenderBatchActive();
    rlEnableDepthMask();
}


static void DibujarParticulasJade(const MinijuegoLaberintoInclinado& m, bool brillo)
{
    for (int i = 0; i < MAX_PARTICULAS_LABERINTO; i++)
    {
        const ParticulaLaberinto& p = m.particulas[i];

        if (!p.activa || p.brillo != brillo)
        {
            continue;
        }

        float vida = p.vidaMaxima > 0.0f ? p.vida / p.vidaMaxima : 0.0f;

        if (brillo)
        {
            ResplandorFrenteJade(p.posicion, p.tamano * (0.6f + 0.8f * vida), ConAlfaJade(p.color, 0.9f * vida));
        }
        else
        {
            float tam = p.tamano * (0.5f + 0.5f * vida);
            CuboJade(p.posicion, { tam, tam, tam }, p.color);
        }
    }
}


static void DibujarLucesJade(
    const MinijuegoLaberintoInclinado& m,
    float H,
    int limite,
    float t
)
{
    if (CalidadEfectos() == CALIDAD_BAJA)
    {
        return;
    }

    rlDrawRenderBatchActive();
    rlDisableDepthMask();
    BeginBlendMode(BLEND_ADDITIVE);

    // Antorchas: halo en la llama y charco calido en el piso.
    for (int k = 0; k < 6; k++)
    {
        Vector3 p = PosicionAntorchaJade(k, H);
        float pulso = 0.85f + 0.15f * std::sin(t * 9.0f + (float)k * 2.0f) + 0.05f * std::sin(t * 23.0f + (float)k);
        ResplandorFrenteJade({ p.x, p.y + 2.0f, p.z }, 1.25f * pulso, Color{ 255, 150, 60, 90 });
        ResplandorFrenteJade({ p.x, p.y + 1.95f, p.z }, 0.6f * pulso, Color{ 255, 220, 140, 150 });
        CharcoLuzJade({ p.x, NIVEL_SUELO_JADE + 0.03f, p.z }, 4.2f * pulso, Color{ 255, 150, 70, 34 });
    }

    // Braseros, con chispas que suben.
    int paso = CalidadDecoracion() == CALIDAD_ALTA ? 1 : 2;
    bool decoracion = CalidadDecoracion() != CALIDAD_BAJA;

    for (int k = 0; decoracion && k < CantidadBraserosJade(m.cantidadTableros); k += paso)
    {
        Vector3 b = PosicionBraseroJade(k, H);
        float pulso = 0.85f + 0.15f * std::sin(t * 8.0f + (float)k * 1.3f) + 0.05f * std::sin(t * 21.0f + (float)k);
        ResplandorFrenteJade({ b.x, b.y + 1.75f, b.z }, 1.15f * pulso, Color{ 255, 140, 50, 90 });
        ResplandorFrenteJade({ b.x, b.y + 1.65f, b.z }, 0.5f * pulso, Color{ 255, 220, 140, 130 });
        CharcoLuzJade({ b.x, NIVEL_SUELO_JADE + 0.035f, b.z }, 4.2f * pulso, Color{ 255, 140, 60, 36 });

        for (int c = 0; c < 5; c++)
        {
            float ciclo = 1.2f + 0.6f * Hash01Jade(c, 210 + k);
            float u = std::fmod(t / ciclo + Hash01Jade(c, 212 + k), 1.0f);
            Vector3 p = { b.x + std::sin(t * 3.0f + (float)c) * 0.25f * u, b.y + 1.6f + 2.2f * u, b.z + std::cos(t * 2.0f + (float)c) * 0.2f * u };
            ResplandorFrenteJade(p, 0.08f, Color{ 255, 190, 90, (unsigned char)(220.0f * (1.0f - u)) });
        }
    }

    // El estanque brilla desde abajo: resplandor de jade en el agua libre y
    // destellos que corren por la superficie.
    float W = MITAD_ESTANQUE_X_JADE;
    for (int k = 0; k < 10; k++)
    {
        float x = (k % 2 == 0 ? -1.0f : 1.0f) * (W - 0.8f);
        float z = -H + 1.5f + (2.0f * H - 3.0f) * (float)(k / 2) / 4.0f;
        float late = 0.75f + 0.25f * std::sin(t * 1.4f + (float)k);
        ResplandorSueloJade({ x, NIVEL_AGUA_JADE + 0.05f, z }, 3.2f, Color{ 60, 220, 170, (unsigned char)(22.0f * late) });
    }

    ResplandorSueloJade({ 0.0f, NIVEL_AGUA_JADE + 0.05f, 0.0f }, 4.0f, Color{ 60, 220, 170, 26 });

    int destellos = (int)(46.0f * FactorCalidadGrafica(CalidadParticulas()));
    for (int k = 0; k < destellos; k++)
    {
        float u = std::fmod(t * (0.25f + 0.2f * Hash01Jade(k, 600)) + Hash01Jade(k, 601), 1.0f);
        float x = -W + 0.3f + (2.0f * W - 0.6f) * Hash01Jade(k, 602);
        float z = -H + 0.3f + (2.0f * H - 0.6f) * Hash01Jade(k, 603);
        float a = std::sin(u * 3.1416f);
        ResplandorSueloJade({ x + u * 0.6f, NIVEL_AGUA_JADE + 0.06f, z }, 0.22f, Color{ 190, 255, 230, (unsigned char)(110.0f * a) });
    }

    // Velas sobre los lotos.
    {
        Vector3 lotos[24];
        int tipos[24];
        int n = PosicionesLotosJade(m, H, lotos, tipos, 24);

        for (int k = 0; k < n; k++)
        {
            if (tipos[k] != 1) continue;
            Vector3 p = lotos[k];
            ResplandorFrenteJade({ p.x, p.y + 0.3f, p.z }, 0.45f, Color{ 255, 190, 90, 120 });
            ResplandorSueloJade({ p.x, NIVEL_AGUA_JADE + 0.05f, p.z }, 1.1f, Color{ 255, 170, 80, 40 });
        }
    }

    // Ofrendas: velas.
    {
        Vector3 ofrendas[2] = { { 0.0f, NIVEL_SUELO_JADE + 1.2f, -H - 2.6f }, { 0.0f, NIVEL_SUELO_JADE, H + 2.2f } };
        for (int k = 0; k < 2; k++)
        {
            for (int s = -1; s <= 1; s += 2)
            {
                Vector3 v = { ofrendas[k].x + (float)s * 0.75f, ofrendas[k].y + 0.42f, ofrendas[k].z + 0.15f };
                ResplandorFrenteJade(v, 0.4f, Color{ 255, 190, 90, 110 });
            }
            CharcoLuzJade({ ofrendas[k].x, ofrendas[k].y + 0.02f, ofrendas[k].z }, 2.4f, Color{ 255, 170, 80, 30 });
        }
    }

    // Idolo: ojos de oro encendidos y haz de luz del oculo sobre el altar.
    {
        float z0 = -H - 5.0f;
        float yb = NIVEL_SUELO_JADE + 1.2f;
        float late = 0.75f + 0.25f * std::sin(t * 1.6f);
        ResplandorFrenteJade({ -0.8f, yb + 2.65f, z0 + 1.8f }, 0.55f * late, Color{ 255, 210, 100, 150 });
        ResplandorFrenteJade({ 0.8f, yb + 2.65f, z0 + 1.8f }, 0.55f * late, Color{ 255, 210, 100, 150 });
        ConoLuzJade({ 2.5f, 15.0f, z0 + 3.0f }, { 0.0f, yb, z0 + 2.2f }, 1.2f, 2.6f, Color{ 255, 236, 190, 26 });
        ResplandorSueloJade({ 0.0f, yb + 0.03f, z0 + 2.4f }, 2.6f, Color{ 255, 230, 170, 40 });
    }

    // Hornacinas: brillo de jade tenue en el fondo de cada una.
    for (int s = -1; s <= 1; s += 2)
    {
        for (float z = -H - 5.0f + 6.0f; z < H + 15.0f; z += 6.0f)
        {
            Vector3 p = { (float)s * (MURO_LATERAL_JADE - 0.05f), NIVEL_SUELO_JADE + 1.9f, z };
            ResplandorJade(p, { 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f, 0.0f }, 1.1f, Color{ 60, 200, 150, 26 });
        }
    }

    // Losas: luces locales con la misma matriz que su dibujo.
    for (int i = 0; i < limite; i++)
    {
        if (!m.resultado.participantes[i].participo) continue;

        const EstadoJugadorLaberinto& e = m.estadosJugadores[i];
        Vector3 centro = m.centrosTableros[e.tablero < 0 ? 0 : e.tablero];

        rlPushMatrix();
        rlTranslatef(centro.x, centro.y, centro.z);
        rlRotatef(e.inclinacionZ * INCLINACION_MAXIMA_GRADOS, 1.0f, 0.0f, 0.0f);
        rlRotatef(-e.inclinacionX * INCLINACION_MAXIMA_GRADOS, 0.0f, 0.0f, 1.0f);
        DibujarLucesTableroJade(m, i, t);
        rlPopMatrix();
    }

    DibujarParticulasJade(m, true);

    // Motas de polvo dorado que flotan en la luz.
    int motas = (int)(36.0f * FactorCalidadGrafica(CalidadParticulas()));
    for (int k = 0; k < motas; k++)
    {
        float ciclo = 12.0f + 6.0f * Hash01Jade(k, 220);
        float u = std::fmod(t / ciclo + Hash01Jade(k, 221), 1.0f);
        float lado = Hash01Jade(k, 224) < 0.5f ? -1.0f : 1.0f;
        Vector3 p =
        {
            lado * (14.0f + 7.0f * Hash01Jade(k, 222)) + std::sin(t * 0.6f + (float)k) * 0.4f,
            -0.5f + 4.0f * u,
            -H - 3.0f + (2.0f * H + 7.0f) * Hash01Jade(k, 223)
        };
        float alfa = std::sin(u * 3.1416f);
        ResplandorFrenteJade(p, 0.06f, Color{ 255, 230, 170, (unsigned char)(180.0f * alfa) });
    }

    EndBlendMode();
    rlDrawRenderBatchActive();
    rlEnableDepthMask();
}


//==================================================
// DIBUJO
//==================================================

void MinijuegoLaberintoInclinado::Dibujar(
    const JugadorPrueba jugadores[],
    int cantidadMaxima,
    const Participante participantes[],
    bool mostrarDebug
) const
{
    (void)jugadores;


    int limite = cantidadMaxima < MAX_PARTICIPANTES ? cantidadMaxima : MAX_PARTICIPANTES;
    const float t = relojVisual;
    float H = MitadEstanqueZJade(cantidadTableros);
    int ancho = GetScreenWidth();
    int alto = GetScreenHeight();

    // Entrada: durante la cuenta la camara arranca baja, mirando el altar
    // del fondo, y sube hasta la vista de juego.
    Camera3D vista = camara;

    if (fase == FASE_LABERINTO_PREPARACION)
    {
        float u = Limitar01Jade(tiempoPreparacion / DURACION_PREPARACION_LABERINTO);
        float suave = u * u * (3.0f - 2.0f * u) * ObtenerFactorMovimientoCamaraMinijuegos();
        vista.position.y -= 10.0f * suave;
        vista.position.z += 13.0f * suave;
        vista.target.z -= 6.0f * suave;
    }

    {
        float dy = vista.target.y - vista.position.y;
        float dz = vista.target.z - vista.position.z;
        float l = std::sqrt(dy * dy + dz * dz);
        if (l > 0.001f) vFrenteJade = { 0.0f, -dz / l, dy / l };
    }

    // Fondo: penumbra del techo del templo.
    DrawRectangleGradientV(0, 0, ancho, alto, Color{ 12, 28, 25, 255 }, Color{ 4, 10, 9, 255 });

    BeginMode3D(vista);
    rlDisableBackfaceCulling();

    DibujarEscenaTemploJade(*this, H, t);
    DibujarOperadoresJade(*this, limite, participantes, t);

    for (int i = 0; i < limite; i++)
    {
        if (!resultado.participantes[i].participo)
        {
            continue;
        }

        const EstadoJugadorLaberinto& e = estadosJugadores[i];
        Vector3 centro = centrosTableros[e.tablero < 0 ? 0 : e.tablero];

        // Inclinacion visual limitada de la losa.
        float anguloX = e.inclinacionZ * INCLINACION_MAXIMA_GRADOS;
        float anguloZ = -e.inclinacionX * INCLINACION_MAXIMA_GRADOS;

        rlPushMatrix();
        rlTranslatef(centro.x, centro.y, centro.z);
        rlRotatef(anguloX, 1.0f, 0.0f, 0.0f);
        rlRotatef(anguloZ, 0.0f, 0.0f, 1.0f);
        DibujarTableroVisual(*this, i, mostrarDebug, t);
        rlPopMatrix();
    }

    DibujarAguaJade(H, t);

    if (CalidadDecoracion() != CALIDAD_BAJA)
    {
        DibujarLotosJade(*this, H, t);
    }

    DibujarTranslucidosJade(*this, H, t);
    DibujarParticulasJade(*this, false);
    DibujarLucesJade(*this, H, limite, t);

    rlEnableBackfaceCulling();
    EndMode3D();

    // Cartelito J# sobre cada operador.
    float escalaOperador = cantidadTableros <= 2 ? 1.1f : 1.2f;

    for (int i = 0; i < limite; i++)
    {
        if (!resultado.participantes[i].participo || !participantes[i].activo)
        {
            continue;
        }

        Vector3 pies, consola;
        float angulo = 0.0f;
        PosicionOperadorJade(*this, i, pies, consola, angulo);
        Vector2 p = GetWorldToScreen({ pies.x, pies.y + 0.2f + 2.3f * escalaOperador, pies.z }, vista);

        char rotulo[8];
        std::snprintf(rotulo, sizeof(rotulo), "J%d", participantes[i].numeroJugador > 0 ? participantes[i].numeroJugador : i + 1);

        OpcionesEtiquetaUI op;
        op.borde = ColorJugadorJade(participantes, i);
        op.pico = true;
        DibujarEtiquetaUI(rotulo, p, 0.0f, op);
    }

    // Textos flotantes sobre la esfera: checkpoint o caida al pozo.
    for (int i = 0; i < limite; i++)
    {
        const VisualJugadorLaberinto& v = visuales[i];

        if (!resultado.participantes[i].participo || v.tiempoTexto <= 0.0f || fase != FASE_LABERINTO_JUGANDO)
        {
            continue;
        }

        const EstadoJugadorLaberinto& e = estadosJugadores[i];
        float u = 1.0f - v.tiempoTexto / DURACION_TEXTO_JADE;
        float lx = v.tipoTexto == 2 ? e.agujeroX : e.x;
        float lz = v.tipoTexto == 2 ? e.agujeroZ : e.z;
        Vector3 w = PuntoTableroJade(*this, i, lx - MITAD_TABLERO, 1.0f + 1.4f * u, lz - MITAD_TABLERO);
        Vector2 s = GetWorldToScreen(w, vista);
        float alfa = v.tiempoTexto < 0.35f ? v.tiempoTexto / 0.35f : 1.0f;

        if (v.tipoTexto == 1)
        {
            char texto[24];
            std::snprintf(texto, sizeof(texto), "¡CONTROL %d/%d!", estadosJugadores[i].puntosControl, MAX_PUNTOS_CONTROL_LABERINTO);
            DibujarTextoMundoContornoUI(texto, s.x, s.y, UI_TEXTO, COLOR_JADE_CLARO, alfa);
        }
        else
        {
            DibujarTextoMundoContornoUI("¡AL POZO!", s.x, s.y, UI_TEXTO, Color{ 226, 220, 204, 255 }, alfa);
        }
    }

    // Anuncio de llegada al altar.
    if (fase == FASE_LABERINTO_JUGANDO && tiempoAnuncio >= 0.0f && jugadorAnuncio >= 0)
    {
        float u = tiempoAnuncio;
        float alfa = u < 0.15f ? u / 0.15f : (u > DURACION_ANUNCIO_JADE - 0.4f ? (DURACION_ANUNCIO_JADE - u) / 0.4f : 1.0f);
        float y = (float)alto * 0.42f;
        char texto[40];
        std::snprintf(texto, sizeof(texto), "¡J%d LLEGÓ AL ALTAR!", participantes[jugadorAnuncio].numeroJugador > 0 ? participantes[jugadorAnuncio].numeroJugador : jugadorAnuncio + 1);
        DibujarTextoMundoContornoUI(texto, (float)ancho * 0.5f, y, UI_TEXTO_GRANDE, Color{ 255, 214, 110, 255 }, alfa);

        if (anuncioConPlazo)
        {
            char plazo[48];
            std::snprintf(plazo, sizeof(plazo), "¡LOS DEMÁS TIENEN %d SEGUNDOS!", (int)PLAZO_TRAS_PRIMERA_LLEGADA);
            DibujarTextoMundoContornoUI(plazo, (float)ancho * 0.5f, y + Px(40.0f), UI_TEXTO, UI_TEXTO_MUNDO, alfa);
        }
    }

    // HUD comun.
    DatosHudMinijuego hud;
    PrepararDatosHudMinijuego(
        hud,
        "LABERINTO JADE",
        "INCLINÁ TU LOSA Y LLEVÁ LA ESFERA DE JADE AL ALTAR DORADO. ¡OJO CON LOS POZOS Y LOS DARDOS!",
        resultado
    );

    hud.fase = fase == FASE_LABERINTO_PREPARACION
        ? HUD_MINIJUEGO_CUENTA
        : (fase == FASE_LABERINTO_TERMINADO ? HUD_MINIJUEGO_FIN : HUD_MINIJUEGO_JUGANDO);
    hud.cuentaRestante = tiempoPreparacion;
    hud.reloj = HUD_RELOJ_CUENTA_ATRAS;
    hud.tiempoRestante = tiempoRestante;
    hud.tiempoTotal = DURACION_PARTIDA_LABERINTO;
    hud.segundosAlerta = 10.0f;

    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        JugadorHudMinijuego& j = hud.jugadores[i];
        if (!j.visible || i >= limite) continue;

        const EstadoJugadorLaberinto& e = estadosJugadores[i];
        int porcentaje = (int)PorcentajeProgreso(*this, e);
        j.valorNumerico = e.puntosControl + (e.llego ? 10 : 0);

        if (e.llego)
        {
            int decimas = (int)(e.tiempoLlegada * 10.0f);
            std::snprintf(j.valor, sizeof(j.valor), "%d,%d S", decimas / 10, decimas % 10);
            std::snprintf(j.estado, sizeof(j.estado), "¡EN EL ALTAR!");
            std::snprintf(j.detalleFinal, sizeof(j.detalleFinal), "ALTAR: %d,%d S", decimas / 10, decimas % 10);
            j.progreso = 1.0f;
        }
        else
        {
            std::snprintf(j.valor, sizeof(j.valor), "%d %%", porcentaje);
            std::snprintf(j.detalleFinal, sizeof(j.detalleFinal), "%d %% DEL CAMINO", porcentaje);
            j.progreso = Limitar01Jade((float)porcentaje / 100.0f);

            if (fase == FASE_LABERINTO_JUGANDO && e.tiempoCaida > 0.0f)
            {
                std::snprintf(j.estado, sizeof(j.estado), "¡AL POZO!");
            }
        }

        if (!participantes[i].esBot)
        {
            std::snprintf(j.controles, sizeof(j.controles), "INCLINÁ: %s", TextoMoverHud(participantes[i]));
        }
    }

    if (fase == FASE_LABERINTO_TERMINADO && resultado.estado == RESULTADO_MINIJUEGO_FINALIZADO)
    {
        hud.finPorTiempo = tiempoRestante <= 0.0f;

        if (llegadas == 0)
        {
            hud.notaFinal = "NADIE LLEGÓ AL ALTAR: GANA QUIEN AVANZÓ MÁS";
        }
    }

    DibujarHudMinijuego(hud, participantes);
}


const ResultadoMinijuego& MinijuegoLaberintoInclinado::ObtenerResultado() const
{
    return resultado;
}
