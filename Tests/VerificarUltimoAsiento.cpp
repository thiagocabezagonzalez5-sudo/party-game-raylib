// Prueba de integracion: ejecuta ZonaPruebas y el minijuego real con OpenGL.
// No usa el visor del paquete ni modifica los GLB.
#include "Gameplay/ZonaPruebas.h"
#include "Minigames/ModelosEscenariosRetro3D.h"
#include "Minigames/ModeloJugadorCompartido.h"
#include "rlgl.h"

#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstring>

static int errores = 0;
static int avisosAusente = 0;

static void Comprobar(bool condicion, const char* detalle)
{
    if (!condicion)
    {
        std::fprintf(stderr, "FALLO: %s\n", detalle);
        errores++;
    }
}

static void RegistrarPrueba(int nivel, const char* formato, va_list argumentos)
{
    char texto[2048];
    std::vsnprintf(texto, sizeof(texto), formato, argumentos);
    if (std::strstr(texto, "Modelo de escenario ausente")) avisosAusente++;
    if (nivel >= LOG_WARNING) std::fprintf(stderr, "%s\n", texto);
}

static bool Cerca(float a, float b)
{
    return std::fabs(a - b) < 0.002f;
}

static void DibujarPrueba(ZonaPruebas& zona, const char* captura = nullptr)
{
    BeginDrawing();
    zona.Dibujar();
    if (captura != nullptr)
    {
        // Leer el buffer actual antes del intercambio de EndDrawing. En una
        // ventana oculta, despues del intercambio se obtiene el frame previo.
        rlDrawRenderBatchActive();
        Image imagen = LoadImageFromScreen();
        Comprobar(ExportImage(imagen, captura), "Exportar captura del minijuego integrado");
        UnloadImage(imagen);
    }
    EndDrawing();
}

int main()
{
    SetTraceLogCallback(RegistrarPrueba);
    SetConfigFlags(FLAG_WINDOW_HIDDEN);
    InitWindow(1280, 800, "Verificacion del minijuego Ultimo Asiento");
    if (!IsWindowReady()) return 2;
    SetExitKey(KEY_NULL);

    // Objetos grandes fuera de la pila de Windows, como en el juego.
    static ZonaPruebas zona;
    Participante participantes[MAX_PARTICIPANTES]{};
    const Color colores[] = { RED, BLUE, GREEN, YELLOW };
    ModelosEscenariosRetro3D& almacen = ObtenerModelosEscenariosRetro3D();

    for (int cantidad = 2; cantidad <= 4; cantidad++)
    {
        for (int i = 0; i < MAX_PARTICIPANTES; i++)
        {
            participantes[i] = {};
            participantes[i].activo = i < cantidad;
            participantes[i].conectado = true;
            participantes[i].control = i % 2 == 0 ? CONTROL_TECLADO_WASD : CONTROL_TECLADO_FLECHAS;
            participantes[i].numeroJugador = i + 1;
            participantes[i].color = colores[i];
        }
        zona.Inicializar(participantes, cantidad);
        if (cantidad == 2)
        {
            for (const auto& r : almacen.ultimoAsiento)
                Comprobar(!r.cargaIntentada, "ZonaPruebas no carga el paquete al arrancar");
            Comprobar(!almacen.montanaLava.cargaIntentada, "La montana tambien se carga a demanda");
        }
        zona.modoCatalogo = true;
        zona.CambiarMinijuego(MINIJUEGO_ULTIMO_ASIENTO);
        auto& m = zona.gestorMinijuegos.minijuegoUltimoAsiento;

        Mesh* mallas[CANTIDAD_MODELOS_ULTIMO_ASIENTO_3D]{};
        Color originales[CANTIDAD_MODELOS_ULTIMO_ASIENTO_3D][16]{};
        for (int i = 0; i < CANTIDAD_MODELOS_ULTIMO_ASIENTO_3D; i++)
        {
            auto& r = almacen.ultimoAsiento[i];
            Comprobar(r.cargado, RUTAS_MODELOS_ULTIMO_ASIENTO_3D[i]);
            if (!r.cargado) continue;
            mallas[i] = r.modelo.meshes;
            for (int j = 0; j < r.modelo.meshCount; j++)
                Comprobar(r.modelo.meshes[j].colors != nullptr, "Conservar colores de vertice");
            Comprobar(r.modelo.materialCount <= 16, "Limite de materiales de esta prueba");
            for (int j = 0; j < r.modelo.materialCount && j < 16; j++)
                originales[i][j] = r.modelo.materials[j].maps[MATERIAL_MAP_DIFFUSE].color;
        }

        Comprobar(Cerca(GetModelBoundingBox(almacen.ultimoAsiento[MODELO_ASIENTO_CARRUSEL_COLUMNA].modelo).min.y, 0.53f), "Origen modular de la columna");
        Comprobar(Cerca(GetModelBoundingBox(almacen.ultimoAsiento[MODELO_ASIENTO_CARRUSEL_TECHO].modelo).min.y, 3.03f), "Origen modular del techo");
        Comprobar(Cerca(GetModelBoundingBox(almacen.ultimoAsiento[MODELO_ASIENTO_NORIA_CABINA].modelo).min.y, -0.925f), "Pivote superior de cabina");
        Comprobar(m.fase == FASE_ULTIMO_ASIENTO_PREPARACION, "Cuenta regresiva inicial");
        DibujarPrueba(zona, cantidad == 4 ? "build/asiento-preparacion.png" : nullptr);
        for (int f = 0; f < 182; f++) zona.Actualizar(1.0f / 60.0f);
        Comprobar(m.fase == FASE_ULTIMO_ASIENTO_JUGANDO, "Fin de preparacion");
        Comprobar(m.subfase == SUBFASE_ULTIMO_ASIENTO_MUSICA, "Subfase de musica");
        DibujarPrueba(zona, cantidad == 4 ? "build/asiento-musica.png" : nullptr);

        m.tiempoSubfase = 0.001f;
        zona.Actualizar(1.0f / 60.0f);
        Comprobar(m.cantidadTazas == cantidad - 1, "Una taza menos que jugadores vivos");
        // Colocar jugadores sin alterar reglas ni colisiones. La ventana
        // oculta no recibe teclas y los teclados evitan activar la IA de mando.
        for (int c = 0; c < m.cantidadTazas; c++)
        {
            zona.jugadores[c].posicion = m.tazas[c].posicion;
            zona.jugadores[c].posicion.y += 0.72f;
        }
        zona.jugadores[cantidad - 1].posicion = { 0.0f, 0.67f, 7.8f };
        m.indiceTrampa = cantidad >= 3 ? 0 : -1;
        for (int c = 0; c < m.cantidadTazas; c++) m.tazas[c].trampa = c == m.indiceTrampa;
        zona.Actualizar(1.0f / 60.0f);
        for (int c = 0; c < m.cantidadTazas; c++)
            Comprobar(m.tazas[c].ocupante == c, "Ocupacion de taza");
        DibujarPrueba(zona, cantidad == 4 ? "build/asiento-ocupacion.png" : nullptr);
        m.tiempoSubfase = 0.5f;
        DibujarPrueba(zona, cantidad == 4 ? "build/asiento-trampa.png" : nullptr);

        // Cada pieza ausente debe poder caer a primitivas sin afectar a otras.
        if (cantidad == 4)
        {
            for (auto& r : almacen.ultimoAsiento)
            {
                bool cargado = r.cargado;
                r.cargado = false;
                DibujarPrueba(zona);
                r.cargado = cargado;
            }
            // La misma escena con reloj avanzado comprueba el dibujo animado.
            m.tiempoAnimacion = 11.5f;
            m.anguloCarrusel = 137.0f;
            DibujarPrueba(zona, "build/asiento-animacion.png");
        }

        for (int i = 0; i < CANTIDAD_MODELOS_ULTIMO_ASIENTO_3D; i++)
        {
            const auto& r = almacen.ultimoAsiento[i];
            for (int j = 0; j < r.modelo.materialCount && j < 16; j++)
                Comprobar(ColorIsEqual(originales[i][j], r.modelo.materials[j].maps[MATERIAL_MAP_DIFFUSE].color), "Restaurar todos los materiales tras dibujar");
        }

        m.tiempoSubfase = 0.001f;
        zona.Actualizar(1.0f / 60.0f);
        Comprobar(!m.vivo[cantidad - 1], "Eliminacion del jugador sin taza");
        if (cantidad >= 3) Comprobar(!m.vivo[0], "Eliminacion por taza trampa");
        DibujarPrueba(zona, cantidad == 4 ? "build/asiento-eliminacion.png" : nullptr);
        m.tiempoRestante = 0.001f;
        zona.Actualizar(1.0f / 60.0f);
        Comprobar(m.fase == FASE_ULTIMO_ASIENTO_TERMINADO, "Pantalla final");
        DibujarPrueba(zona, cantidad == 4 ? "build/asiento-final.png" : nullptr);

        zona.gestorMinijuegos.ReiniciarActivo(zona.contextoMinijuego);
        Comprobar(m.fase == FASE_ULTIMO_ASIENTO_PREPARACION, "Reinicio de ronda (dispatch de R)");
        zona.CambiarModo(PRUEBA_ZONA_PRINCIPAL);
        zona.Inicializar(participantes, cantidad); // Reentrada desde selector/tablero.
        zona.modoTablero = true;
        zona.modoCatalogo = true;
        zona.CambiarMinijuego(MINIJUEGO_ULTIMO_ASIENTO);
        DibujarPrueba(zona);
        for (int i = 0; i < CANTIDAD_MODELOS_ULTIMO_ASIENTO_3D; i++)
            Comprobar(almacen.ultimoAsiento[i].modelo.meshes == mallas[i], "Reinicio/reentrada reutilizan las mismas mallas");
        Comprobar(!almacen.montanaLava.cargaIntentada, "Ultimo Asiento no carga otros paquetes");
    }

    RecursoModeloEscenarioRetro3D ausente;
    for (int i = 0; i < 5; i++)
        PrepararSlotModeloEscenarioRetro3D(ausente, "build/modelo-inexistente.glb", 0.0f);
    Comprobar(avisosAusente == 1 && !ausente.cargado, "Registrar un archivo ausente una sola vez");
    RecursoModeloEscenarioRetro3D incompatible;
    PrepararSlotModeloEscenarioRetro3D(incompatible,
        RUTAS_MODELOS_ULTIMO_ASIENTO_3D[MODELO_ASIENTO_TAZA], 0.0f,
        PIVOTE_ESCENARIO_ORIGINAL, 999);
    Comprobar(!incompatible.cargado, "Rechazar y liberar un GLB incompatible con el color de estado");

    zona.Descargar();
    zona.Descargar(); // Descarga idempotente antes de cerrar el contexto.
    for (const auto& r : almacen.ultimoAsiento)
        Comprobar(!r.cargado && !r.cargaIntentada && r.modelo.meshes == nullptr, "Descarga simetrica del paquete");
    DescargarModeloJugadorCompartido();
    CloseWindow();
    std::printf("Verificacion integrada 2/3/4 jugadores: %d errores\n", errores);
    return errores == 0 ? 0 : 1;
}
