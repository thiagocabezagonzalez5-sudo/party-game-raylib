#include "Minigames/MinijuegoLaberintoInclinado.h"

#include "Minigames/AudioMinijuegos.h"
#include "Systems/Input.h"

#include "raylib.h"
#include "rlgl.h"

#include <cmath>


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


static const char* NombreJugadorLaberinto(const Participante& participante, int indice)
{
    return TextFormat(
        "J%d%s",
        participante.numeroJugador > 0 ? participante.numeroJugador : indice + 1,
        participante.esBot ? " BOT" : ""
    );
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

    if (m.cantidadTableros <= 2)
    {
        m.camara.position = { 0.0f, 21.0f, 9.0f };
        m.camara.target = { 0.0f, 0.0f, 0.4f };
    }
    else
    {
        m.camara.position = { 0.0f, 32.0f, 13.5f };
        m.camara.target = { 0.0f, 0.0f, 0.8f };
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
// VISUAL (MODELO FUTURO)
//==================================================
// MODELO FUTURO: reemplazar por GLB la losa/marco de piedra, los bloques de
// muro, el altar de la meta, los discos de checkpoint, la esfera de jade,
// las columnas y las antorchas del templo. La logica (celdas, muros,
// agujeros, trampas) no depende de esta decoracion.
//==================================================

static void DibujarEscenaTemplo(float t)
{
    // Suelo del templo y muro del fondo.
    DrawCube({ 0.0f, -1.2f, 0.0f }, 54.0f, 0.4f, 46.0f, Color{ 52, 46, 40, 255 });
    DrawCube({ 0.0f, 5.0f, -22.0f }, 54.0f, 12.0f, 1.2f, Color{ 66, 58, 48, 255 });

    // Friso dorado con glifos en el muro del fondo.
    DrawCube({ 0.0f, 8.4f, -21.3f }, 54.0f, 0.5f, 0.3f, Color{ 190, 150, 60, 255 });

    for (int i = -6; i <= 6; i++)
    {
        float x = (float)i * 4.0f;
        DrawCube({ x, 6.6f, -21.3f }, 1.4f, 0.35f, 0.2f, Color{ 190, 150, 60, 255 });
        DrawCube({ x, 6.0f, -21.3f }, 0.35f, 1.2f, 0.2f, Color{ 52, 150, 110, 255 });
    }

    // Columnas laterales y antorchas.
    const float columnasX[4] = { -23.0f, -23.0f, 23.0f, 23.0f };
    const float columnasZ[4] = { -14.0f, 12.0f, -14.0f, 12.0f };

    for (int i = 0; i < 4; i++)
    {
        float x = columnasX[i];
        float z = columnasZ[i];

        DrawCylinder({ x, 0.0f, z }, 1.5f, 1.5f, 0.6f, 12, Color{ 110, 100, 84, 255 });
        DrawCylinder({ x, 0.6f, z }, 1.0f, 1.0f, 9.0f, 12, Color{ 150, 138, 112, 255 });
        DrawCylinder({ x, 9.6f, z }, 1.4f, 1.4f, 0.6f, 12, Color{ 110, 100, 84, 255 });
        DrawCube({ x, 4.6f, z }, 2.1f, 0.18f, 2.1f, Color{ 52, 150, 110, 255 });
    }

    // Antorchas junto al fondo: soporte, llama parpadeante y brillo.
    for (int i = 0; i < 6; i++)
    {
        float x = -15.0f + (float)i * 6.0f;
        float parpadeo = 0.5f + 0.5f * std::sin(t * 11.0f + (float)i * 1.9f);

        DrawCylinder({ x, 2.4f, -21.0f }, 0.12f, 0.2f, 1.6f, 8, Color{ 70, 52, 36, 255 });
        DrawSphere(
            { x, 4.3f + 0.08f * parpadeo, -20.9f },
            0.34f + 0.08f * parpadeo,
            Color{ 255, (unsigned char)(130 + 60 * parpadeo), 30, 255 }
        );
        DrawSphere({ x, 4.3f, -20.9f }, 0.2f, Color{ 255, 230, 140, 255 });
    }
}


static Vector3 PosicionLocal(float x, float z, float y)
{
    return { x - MITAD_TABLERO, y, z - MITAD_TABLERO };
}


static void DibujarTrampaVisual(
    const MinijuegoLaberintoInclinado& m,
    const TrampaDardosLaberinto& trampa,
    float t
)
{
    float avance = 0.0f;
    int estado = EstadoTrampa(trampa, m.tiempoJuego, avance);
    bool horizontal = std::fabs(trampa.direccionX) > 0.5f;

    // Chevrones grabados en el suelo; se encienden de rojo en el aviso.
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
            DrawCube(
                centro,
                horizontal ? 0.55f : 0.14f,
                0.03f,
                horizontal ? 0.14f : 0.55f,
                color
            );
        }
    }

    // Boquillas en los muros laterales del pasillo (brillan en el aviso).
    int medioC = (trampa.columnaInicio + trampa.columnaFin) / 2;
    int medioR = (trampa.filaInicio + trampa.filaFin) / 2;
    Color boquilla = estado == 0 ? Color{ 40, 34, 30, 255 } : Color{ 255, 70, 40, 255 };

    if (horizontal)
    {
        DrawCube(PosicionLocal((float)medioC + 0.5f, (float)trampa.filaInicio - 0.02f, 0.35f), 0.4f, 0.22f, 0.08f, boquilla);
        DrawCube(PosicionLocal((float)medioC + 0.5f, (float)trampa.filaFin + 1.02f, 0.35f), 0.4f, 0.22f, 0.08f, boquilla);
    }
    else
    {
        DrawCube(PosicionLocal((float)trampa.columnaInicio - 0.02f, (float)medioR + 0.5f, 0.35f), 0.08f, 0.22f, 0.4f, boquilla);
        DrawCube(PosicionLocal((float)trampa.columnaFin + 1.02f, (float)medioR + 0.5f, 0.35f), 0.08f, 0.22f, 0.4f, boquilla);
    }

    // Dardo en vuelo durante el disparo.
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

        DrawCube(
            PosicionLocal(dx, dz, 0.35f),
            horizontal ? 0.7f : 0.08f,
            0.08f,
            horizontal ? 0.08f : 0.7f,
            Color{ 255, 190, 60, 255 }
        );
    }
}


// Dibuja una losa completa en coordenadas locales (origen en su centro).
static void DibujarTableroVisual(
    const MinijuegoLaberintoInclinado& m,
    int indiceJugador,
    const Participante& participante,
    bool mostrarDebug,
    float t
)
{
    const EstadoJugadorLaberinto& e = m.estadosJugadores[indiceJugador];
    Color colorJugador = COLORES_JUGADORES_LABERINTO[indiceJugador % MAX_PARTICIPANTES];

    (void)participante;

    // Base de piedra con borde de jade.
    DrawCube({ 0.0f, -0.3f, 0.0f }, 12.0f, 0.6f, 12.0f, COLOR_PIEDRA_OSCURA);
    DrawCube({ 0.0f, 0.01f, -5.8f }, 12.0f, 0.04f, 0.4f, COLOR_JADE);
    DrawCube({ 0.0f, 0.01f, 5.8f }, 12.0f, 0.04f, 0.4f, COLOR_JADE);
    DrawCube({ -5.8f, 0.01f, 0.0f }, 0.4f, 0.04f, 12.0f, COLOR_JADE);
    DrawCube({ 5.8f, 0.01f, 0.0f }, 0.4f, 0.04f, 12.0f, COLOR_JADE);

    // Banda del color del jugador en el frente de la losa.
    DrawCube({ 0.0f, -0.3f, 6.02f }, 12.0f, 0.3f, 0.06f, colorJugador);

    // Suelo del laberinto con baldosas alternadas.
    DrawCube({ 0.0f, 0.0f, 0.0f }, 11.0f, 0.05f, 11.0f, COLOR_LOSA);

    for (int r = 0; r < FILAS_LABERINTO; r++)
    {
        for (int c = 0; c < COLUMNAS_LABERINTO; c++)
        {
            TipoCeldaLaberinto tipo = m.celdas[r][c];
            Vector3 centro = PosicionLocal((float)c + 0.5f, (float)r + 0.5f, 0.0f);

            if (tipo == CELDA_LABERINTO_MURO)
            {
                DrawCube({ centro.x, ALTO_MURO * 0.5f, centro.z }, 1.0f, ALTO_MURO, 1.0f, COLOR_PIEDRA);
                DrawCubeWires({ centro.x, ALTO_MURO * 0.5f, centro.z }, 1.0f, ALTO_MURO, 1.0f, COLOR_PIEDRA_OSCURA);

                // Glifos dorados sobre algunos bloques.
                if ((c * 7 + r * 3) % 5 == 0)
                {
                    DrawCube({ centro.x, ALTO_MURO + 0.015f, centro.z }, 0.36f, 0.03f, 0.36f, COLOR_ORO);
                    DrawCube({ centro.x, ALTO_MURO + 0.03f, centro.z }, 0.12f, 0.03f, 0.12f, COLOR_PIEDRA_OSCURA);
                }
                else if ((c + r * 5) % 7 == 0)
                {
                    DrawCube({ centro.x, ALTO_MURO + 0.015f, centro.z }, 0.7f, 0.03f, 0.1f, COLOR_JADE);
                }
            }
            else if (tipo == CELDA_LABERINTO_AGUJERO)
            {
                DrawCylinder({ centro.x, 0.03f, centro.z }, 0.44f, 0.44f, 0.02f, 12, Color{ 170, 70, 40, 255 });
                DrawCylinder({ centro.x, 0.045f, centro.z }, RADIO_AGUJERO + 0.02f, RADIO_AGUJERO + 0.02f, 0.02f, 12, Color{ 8, 6, 12, 255 });
            }
            else if ((c + r) % 2 == 0)
            {
                DrawCube({ centro.x, 0.03f, centro.z }, 0.94f, 0.03f, 0.94f, COLOR_LOSA_CLARA);
            }
        }
    }

    // Salida.
    Vector3 salida = PosicionLocal((float)m.inicioColumna + 0.5f, (float)m.inicioFila + 0.5f, 0.05f);
    DrawCylinder(salida, 0.4f, 0.4f, 0.02f, 20, Color{ 40, 120, 90, 255 });

    // Checkpoints: disco dorado que se vuelve jade al activarse.
    for (int k = 0; k < MAX_PUNTOS_CONTROL_LABERINTO; k++)
    {
        bool activo = e.puntosControl > k;
        Color color = activo ? COLOR_JADE : COLOR_ORO;
        Vector3 centro = PosicionLocal((float)m.controlColumna[k] + 0.5f, (float)m.controlFila[k] + 0.5f, 0.05f);

        DrawCylinder(centro, 0.42f, 0.42f, 0.03f, 20, color);
        DrawCylinder({ centro.x, 0.06f, centro.z }, 0.26f, 0.26f, 0.03f, 20, COLOR_PIEDRA_OSCURA);
        DrawCylinder({ centro.x, 0.3f, centro.z }, 0.05f, 0.05f, 0.5f, 8, COLOR_PIEDRA_CLARA);
        DrawSphere(
            { centro.x, 0.6f + (activo ? 0.04f * std::sin(t * 6.0f) : 0.0f), centro.z },
            0.11f,
            color
        );
    }

    // Altar de la meta.
    Vector3 meta = PosicionLocal((float)m.metaColumna + 0.5f, (float)m.metaFila + 0.5f, 0.0f);
    float brillo = 0.5f + 0.5f * std::sin(t * 4.0f);

    DrawCube({ meta.x, 0.1f, meta.z }, 0.85f, 0.2f, 0.85f, COLOR_PIEDRA_CLARA);
    DrawCube({ meta.x, 0.3f, meta.z }, 0.55f, 0.2f, 0.55f, COLOR_ORO);
    DrawSphere(
        { meta.x, 0.62f + 0.05f * brillo, meta.z },
        0.16f + 0.04f * brillo,
        Color{ 255, (unsigned char)(210 + 40 * brillo), 120, 255 }
    );

    // Trampas de dardos.
    for (int i = 0; i < m.cantidadTrampas; i++)
    {
        DibujarTrampaVisual(m, m.trampas[i], t);
    }

    // Esfera de jade, con sombra del color del jugador.
    float escala = 1.0f;
    float px = e.x;
    float pz = e.z;

    if (e.tiempoCaida > 0.0f)
    {
        escala = Acotar(e.tiempoCaida / DURACION_CAIDA, 0.0f, 1.0f);
    }

    Vector3 esfera = PosicionLocal(px, pz, RADIO_ESFERA * escala + 0.03f);
    DrawCylinder({ esfera.x, 0.045f, esfera.z }, 0.34f * escala, 0.34f * escala, 0.012f, 16, colorJugador);
    DrawSphere(esfera, RADIO_ESFERA * escala, e.llego ? COLOR_ORO : COLOR_JADE);
    DrawSphere(
        { esfera.x - 0.08f, esfera.y + 0.1f, esfera.z - 0.08f },
        0.07f * escala,
        Color{ 200, 255, 225, 255 }
    );

    if (mostrarDebug)
    {
        DrawSphereWires(esfera, RADIO_ESFERA, 8, 8, RED);
    }
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

    int limite = cantidadMaxima < MAX_PARTICIPANTES
        ? cantidadMaxima
        : MAX_PARTICIPANTES;

    ClearBackground(Color{ 24, 20, 18, 255 });
    BeginMode3D(camara);

    DibujarEscenaTemplo(tiempoAnimacion);

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
        DibujarTableroVisual(*this, i, participantes[i], mostrarDebug, tiempoAnimacion);
        rlPopMatrix();
    }

    EndMode3D();

    int anchoPantalla = GetScreenWidth();
    int altoPantalla = GetScreenHeight();

    // Rotulos sobre cada tablero.
    int activos = 0;

    for (int i = 0; i < limite; i++)
    {
        if (!resultado.participantes[i].participo)
        {
            continue;
        }

        activos++;

        const EstadoJugadorLaberinto& e = estadosJugadores[i];
        Vector3 centro = centrosTableros[e.tablero < 0 ? 0 : e.tablero];
        Vector2 pantalla = GetWorldToScreen({ centro.x, 0.5f, centro.z - 6.6f }, camara);
        Color colorJugador = COLORES_JUGADORES_LABERINTO[i % MAX_PARTICIPANTES];

        const char* nombre = NombreJugadorLaberinto(participantes[i], i);
        int anchoNombre = MeasureText(nombre, 20);
        int baseX = (int)pantalla.x;
        int baseY = (int)pantalla.y;

        // Fondo oscuro para que el rotulo se lea sobre el borde de otras losas.
        int anchoPanel = (anchoNombre > 150 ? anchoNombre : 150) + 16;

        // El rotulo va al costado exterior de la losa: asi no tapa el laberinto de la fila superior.
        float signo = centro.x < -0.1f ? -1.0f : (centro.x > 0.1f ? 1.0f : 0.0f);

        if (signo != 0.0f)
        {
            Vector2 borde = GetWorldToScreen({ centro.x + signo * 7.0f, 0.5f, centro.z }, camara);
            baseX = (int)borde.x + (int)signo * (anchoPanel / 2 + 10);
            baseY = (int)borde.y + 20;
            if (baseX < anchoPanel / 2 + 4) baseX = anchoPanel / 2 + 4;
            if (baseX > anchoPantalla - anchoPanel / 2 - 4) baseX = anchoPantalla - anchoPanel / 2 - 4;
        }
        else if (baseY < 128) baseY = 128;
        else baseY += 12;

        DrawRectangle(baseX - anchoPanel / 2, baseY - 50, anchoPanel, 56, Fade(BLACK, 0.62f));

        DrawText(nombre, baseX - anchoNombre / 2, baseY - 46, 20, colorJugador);

        const char* estadoTexto = e.llego
            ? TextFormat("META  -  %.1f s", e.tiempoLlegada)
            : TextFormat("CP %d/%d   %.0f%%", e.puntosControl, MAX_PUNTOS_CONTROL_LABERINTO, PorcentajeProgreso(*this, e));
        int anchoEstado = MeasureText(estadoTexto, 16);
        DrawText(estadoTexto, baseX - anchoEstado / 2, baseY - 24, 16, e.llego ? GOLD : RAYWHITE);

        // Barra de progreso.
        int anchoBarra = 120;
        float fraccion = Acotar(e.progreso / (float)distanciaInicio, 0.0f, 1.0f);
        DrawRectangle(baseX - anchoBarra / 2, baseY - 6, anchoBarra, 6, Fade(BLACK, 0.7f));
        DrawRectangle(baseX - anchoBarra / 2, baseY - 6, (int)((float)anchoBarra * fraccion), 6, colorJugador);

        if (fase == FASE_LABERINTO_PREPARACION && !JugadorEsBot(participantes[i]))
        {
            const char* control = TextFormat("CONTROL: %s", ObtenerNombreControlParticipante(participantes[i]));
            int anchoControl = MeasureText(control, 14);
            DrawText(control, baseX - anchoControl / 2, baseY + 2, 14, LIGHTGRAY);
        }
    }

    // Encabezado y tiempo.
    DrawText("LABERINTO JADE", 28, 20, 28, GOLD);
    DrawText("TEMPLO ANTIGUO: INCLINA TU LOSA Y LLEGA AL ALTAR DORADO", 28, 52, 16, Color{ 120, 220, 170, 255 });

    const char* textoTiempo = TextFormat("TIEMPO %02d", (int)std::ceil(tiempoRestante));
    DrawText(
        textoTiempo,
        anchoPantalla - MeasureText(textoTiempo, 26) - 28,
        22,
        26,
        tiempoRestante <= 6.0f ? RED : GOLD
    );

    if (plazoAcortado && fase == FASE_LABERINTO_JUGANDO)
    {
        const char* aviso = "ALGUIEN LLEGO AL ALTAR: SE ACABA EL TIEMPO";
        DrawText(aviso, anchoPantalla / 2 - MeasureText(aviso, 20) / 2, 56, 20, ORANGE);
    }

    const char* ayuda = "MOVER: inclina la losa (WASD / FLECHAS / STICK)   -   Evita los agujeros y los dardos";
    DrawText(ayuda, anchoPantalla / 2 - MeasureText(ayuda, 16) / 2, altoPantalla - 28, 16, LIGHTGRAY);

    if (fase == FASE_LABERINTO_PREPARACION)
    {
        int numero = (int)std::ceil(tiempoPreparacion);

        if (numero < 1) numero = 1;

        const char* texto = TextFormat("%d", numero);
        DrawText(
            texto,
            anchoPantalla / 2 - MeasureText(texto, 96) / 2,
            altoPantalla / 2 - 60,
            96,
            GOLD
        );
    }
    else if (
        fase == FASE_LABERINTO_TERMINADO &&
        resultado.estado == RESULTADO_MINIJUEGO_FINALIZADO
    )
    {
        int panelAncho = 560;
        int panelAlto = 130 + 30 * activos;
        int px = anchoPantalla / 2 - panelAncho / 2;
        int py = altoPantalla / 2 - panelAlto / 2;

        DrawRectangle(px, py, panelAncho, panelAlto, Fade(BLACK, 0.9f));

        int ganadores[MAX_PARTICIPANTES]{};
        int cantidadGanadores = ObtenerIndicesGanadores(resultado, ganadores, MAX_PARTICIPANTES);
        const char* titulo = "EMPATE EN EL TEMPLO";

        if (resultado.desenlace == DESENLACE_CON_GANADOR && cantidadGanadores == 1)
        {
            titulo = TextFormat("GANA %s", NombreJugadorLaberinto(participantes[ganadores[0]], ganadores[0]));
        }

        DrawText(titulo, anchoPantalla / 2 - MeasureText(titulo, 32) / 2, py + 14, 32, GOLD);

        int fila = 0;

        for (int posicion = 1; posicion <= MAX_PARTICIPANTES; posicion++)
        {
            for (int i = 0; i < limite; i++)
            {
                if (
                    !resultado.participantes[i].participo ||
                    resultado.participantes[i].posicionFinal != posicion
                )
                {
                    continue;
                }

                const EstadoJugadorLaberinto& e = estadosJugadores[i];
                const char* detalle = e.llego
                    ? TextFormat("META %.1f s", e.tiempoLlegada)
                    : TextFormat("%.0f%% del camino", PorcentajeProgreso(*this, e));

                DrawText(
                    TextFormat("%d.  %s   %s", posicion, NombreJugadorLaberinto(participantes[i], i), detalle),
                    px + 40,
                    py + 62 + fila * 30,
                    22,
                    COLORES_JUGADORES_LABERINTO[i % MAX_PARTICIPANTES]
                );
                fila++;
            }
        }

        DrawText(
            TextoReinicioMinijuego(),
            anchoPantalla / 2 - MeasureText(TextoReinicioMinijuego(), 18) / 2,
            py + panelAlto - 30,
            18,
            RAYWHITE
        );
    }
}


const ResultadoMinijuego& MinijuegoLaberintoInclinado::ObtenerResultado() const
{
    return resultado;
}
