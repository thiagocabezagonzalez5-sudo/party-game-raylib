// Ejecuta ZonaPruebas y el minijuego integrado, con el contexto OpenGL real.
// El linker observa DrawModelEx y las teclas R/ESC sin cambiar el juego.
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
static int teclaSimulada = KEY_NULL;
static bool observar = false;
static Color originales[CANTIDAD_MODELOS_CAJAS_PUERTO_3D][16]{};

struct DibujoObservado
{
    Vector3 posicion{};
    Vector3 escala{};
    Color color{};
};
static DibujoObservado dibujos[CANTIDAD_MODELOS_CAJAS_PUERTO_3D][8]{};
static int cantidades[CANTIDAD_MODELOS_CAJAS_PUERTO_3D]{};

static void Comprobar(bool condicion, const char* detalle)
{
    if (!condicion)
    {
        std::fprintf(stderr, "FALLO: %s\n", detalle);
        errores++;
    }
}

static bool Cerca(float a, float b) { return std::fabs(a - b) < 0.002f; }
static bool Igual(Vector3 a, Vector3 b)
{
    return Cerca(a.x, b.x) && Cerca(a.y, b.y) && Cerca(a.z, b.z);
}

static void RegistrarPrueba(int nivel, const char* formato, va_list argumentos)
{
    char texto[2048];
    std::vsnprintf(texto, sizeof(texto), formato, argumentos);
    if (std::strstr(texto, "Modelo de escenario ausente")) avisosAusente++;
    if (nivel >= LOG_WARNING) std::fprintf(stderr, "%s\n", texto);
}

extern "C" bool __real_IsKeyPressed(int tecla);
extern "C" bool __wrap_IsKeyPressed(int tecla)
{
    return tecla == teclaSimulada || __real_IsKeyPressed(tecla);
}

extern "C" void __real_DrawModelEx(Model, Vector3, Vector3, float, Vector3, Color);
extern "C" void __wrap_DrawModelEx(Model modelo, Vector3 posicion, Vector3 eje,
    float angulo, Vector3 escala, Color tinte)
{
    auto& recursos = ObtenerModelosEscenariosRetro3D().cajasPuerto;
    for (int i = 0; observar && i < CANTIDAD_MODELOS_CAJAS_PUERTO_3D; i++)
    {
        const auto& r = recursos[i];
        if (modelo.meshes != r.modelo.meshes) continue;
        int n = cantidades[i]++;
        Comprobar(n < 8, "No duplicar instancias de una pieza");
        if (n < 8)
        {
            Color color = r.materialColor < 0 ? WHITE :
                modelo.materials[r.materialColor].maps[MATERIAL_MAP_DIFFUSE].color;
            dibujos[i][n] = { posicion, escala, color };
        }
        Comprobar(ColorIsEqual(tinte, WHITE), "DrawModelEx conserva el color del resto de materiales");
        Comprobar(Cerca(angulo, 0.0f), "No duplicar la transformacion del paquete");
        for (int j = 0; j < modelo.materialCount && j < 16; j++)
            if (j != r.materialColor)
                Comprobar(ColorIsEqual(originales[i][j],
                    modelo.materials[j].maps[MATERIAL_MAP_DIFFUSE].color),
                    "No tenir refuerzos/herrajes durante el dibujo");
        break;
    }
    __real_DrawModelEx(modelo, posicion, eje, angulo, escala, tinte);
}

static void DibujarPrueba(ZonaPruebas& zona, const char* captura = nullptr)
{
    std::memset(cantidades, 0, sizeof(cantidades));
    Vector3 posiciones[MAX_PARTICIPANTES]{};
    Vector3 tamanos[MAX_PARTICIPANTES]{};
    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        posiciones[i] = zona.jugadores[i].posicion;
        tamanos[i] = zona.jugadores[i].tamano;
    }
    auto& m = zona.gestorMinijuegos.minijuegoCajasPuerto;
    Camera3D camara = m.camara;
    BeginDrawing();
    zona.Dibujar();
    if (captura)
    {
        // Capturar el frame actual antes del intercambio de buffers.
        rlDrawRenderBatchActive();
        Image imagen = LoadImageFromScreen();
        Comprobar(ExportImage(imagen, captura), "Exportar captura del juego integrado");
        UnloadImage(imagen);
    }
    EndDrawing();
    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        Comprobar(Igual(posiciones[i], zona.jugadores[i].posicion), "Dibujar no mueve jugadores");
        Comprobar(Igual(tamanos[i], zona.jugadores[i].tamano), "Dibujar conserva hitboxes");
    }
    Comprobar(Igual(camara.position, m.camara.position) && Igual(camara.target, m.camara.target),
        "Dibujar conserva la camara");
    auto& recursos = ObtenerModelosEscenariosRetro3D().cajasPuerto;
    for (int i = 0; i < CANTIDAD_MODELOS_CAJAS_PUERTO_3D; i++)
        for (int j = 0; j < recursos[i].modelo.materialCount && j < 16; j++)
            Comprobar(ColorIsEqual(originales[i][j],
                recursos[i].modelo.materials[j].maps[MATERIAL_MAP_DIFFUSE].color),
                "Restaurar los materiales despues de cada dibujo");
}

static void Avanzar(ZonaPruebas& zona, int frames)
{
    for (int i = 0; i < frames; i++) zona.Actualizar(1.0f / 60.0f);
}

static void ComprobarGrua(const MinijuegoCajasPuerto& m)
{
    const auto& carro = dibujos[MODELO_CAJAS_GRUA_CARRO][0];
    const auto& cabina = dibujos[MODELO_CAJAS_GRUA_CABINA][0];
    const auto& brazo = dibujos[MODELO_CAJAS_GRUA_BRAZO][0];
    const auto& cable = dibujos[MODELO_CAJAS_GRUA_CABLE][0];
    const auto& gancho = dibujos[MODELO_CAJAS_GRUA_GANCHO][0];
    Comprobar(Igual(carro.posicion, { m.ganchoX, 5.7f, -5.0f }) &&
        Igual(cabina.posicion, carro.posicion), "Carro y cabina siguen ganchoX");
    Comprobar(Igual(brazo.posicion, { m.ganchoX, 5.55f, -5.0f }), "Brazo sigue ganchoX");
    Comprobar(Igual(cable.posicion, { m.ganchoX, 5.55f, -2.5f }), "Cable unido a la punta del brazo");
    Comprobar(Cerca(cable.escala.x, 1.0f) && Cerca(cable.escala.z, 1.0f), "Cable escala solo en Y");
    Comprobar(Cerca(cable.posicion.y - cable.escala.y, gancho.posicion.y), "Cable conectado al gancho");
    Comprobar(Igual(gancho.posicion,
        { m.ganchoX, 4.2f + std::sin(m.tiempoAnimacion * 3.0f) * 0.06f, -2.5f }),
        "Gancho conserva el vaiven del estado actual");
}

int main()
{
    SetTraceLogCallback(RegistrarPrueba);
    SetConfigFlags(FLAG_WINDOW_HIDDEN);
    InitWindow(1280, 800, "Verificacion integrada de Cajas del Puerto");
    if (!IsWindowReady()) return 2;
    SetExitKey(KEY_NULL);
    static ZonaPruebas zona;
    Participante participantes[MAX_PARTICIPANTES]{};
    auto& almacen = ObtenerModelosEscenariosRetro3D();
    Mesh* mallas[CANTIDAD_MODELOS_CAJAS_PUERTO_3D]{};
    const Color colores[] = { RED, BLUE, GREEN, YELLOW };
    const Color coloresCajas[] = { {168,62,52,255}, {52,110,166,255}, {70,140,84,255},
        {196,150,56,255}, {130,76,150,255}, {60,150,156,255}, {190,98,52,255}, {112,118,130,255} };

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
            for (const auto& r : almacen.cajasPuerto)
                Comprobar(!r.cargaIntentada, "No cargar Cajas del Puerto al arrancar");
        zona.modoCatalogo = true;
        zona.CambiarMinijuego(MINIJUEGO_CAJAS_PUERTO);
        auto& m = zona.gestorMinijuegos.minijuegoCajasPuerto;
        for (int i = 0; i < CANTIDAD_MODELOS_CAJAS_PUERTO_3D; i++)
        {
            auto& r = almacen.cajasPuerto[i];
            Comprobar(r.cargado, RUTAS_MODELOS_CAJAS_PUERTO_3D[i]);
            if (!r.cargado) { zona.Descargar(); CloseWindow(); return 2; }
            if (cantidad == 2) mallas[i] = r.modelo.meshes;
            Comprobar(mallas[i] == r.modelo.meshes, "Reentrada reutiliza las mismas mallas");
            Comprobar(r.modelo.materialCount <= 16, "Limite de materiales de la prueba");
            for (int j = 0; j < r.modelo.materialCount && j < 16; j++)
                originales[i][j] = r.modelo.materials[j].maps[MATERIAL_MAP_DIFFUSE].color;
            for (int j = 0; j < r.modelo.meshCount; j++)
                Comprobar(r.modelo.meshes[j].colors != nullptr, "Conservar colores de vertices");
        }
        observar = true;
        Comprobar(m.fase == FASE_CAJAS_PREPARACION, "Cuenta regresiva inicial");
        DibujarPrueba(zona, cantidad == 4 ? "build/cajas-preparacion.png" : nullptr);
        const int esperados[] = { 1,1,1,1,1,1,1,8,8,8,4,4,6,1,1,0 };
        for (int i = 0; i < CANTIDAD_MODELOS_CAJAS_PUERTO_3D; i++)
            Comprobar(cantidades[i] == esperados[i], "Todas las piezas se dibujan sin duplicar mallas");
        Comprobar(Igual(dibujos[MODELO_CAJAS_MUELLE][0].posicion, {0,0,0.75f}), "Posicion del muelle");
        Comprobar(Igual(dibujos[MODELO_CAJAS_BARCO][0].posicion, {0,-0.9f,-17}), "Posicion del barco");
        for (int c = 0; c < 8; c++)
        {
            Comprobar(Igual(dibujos[MODELO_CAJAS_CONTENEDOR_CUERPO][c].posicion,
                { (c-3.5f)*2.1f, 0, -2.5f }), "Posicion de los ocho contenedores");
            Comprobar(ColorIsEqual(dibujos[MODELO_CAJAS_CONTENEDOR_CUERPO][c].color, coloresCajas[c]),
                "Color de cada instancia en COLOR_DINAMICO");
        }
        ComprobarGrua(m);
        Avanzar(zona, 182);
        Comprobar(m.fase == FASE_CAJAS_ESCONDER, "Fase de esconder jugadores");
        int escondidos[3]{};
        int n = 0;
        for (int j = 0; j < cantidad; j++)
            if (j != m.indiceSolo)
            {
                escondidos[n] = j;
                zona.jugadores[j].posicion.x = m.contenedores[2 + n * 2].posicion.x;
                n++;
            }
        m.tiempoFase = 0.001f;
        Avanzar(zona, 1);
        Comprobar(m.fase == FASE_CAJAS_CIERRE, "Cierre tras asignacion de ocupantes");
        for (int j = 0; j < n; j++)
            Comprobar(m.contenedores[2+j*2].ocupante == escondidos[j], "Ocupacion del contenedor");
        // Elegir estimulos deterministas; la resolucion y eliminacion son las reales.
        for (int c = 0; c < 8; c++) m.contenedores[c].reforzado = c == 4;
        Avanzar(zona, 62);
        Comprobar(m.fase == FASE_CAJAS_ELEGIR, "Elegir tras cierre");
        DibujarPrueba(zona, cantidad == 4 ? "build/cajas-puertas-cerradas.png" : nullptr);
        for (int c = 0; c < 8; c++)
        {
            const auto& izquierda = dibujos[MODELO_CAJAS_PUERTA_DERECHA][c];
            const auto& derecha = dibujos[MODELO_CAJAS_PUERTA_IZQUIERDA][c];
            const auto& p = m.contenedores[c].posicion;
            Comprobar(Igual(izquierda.posicion, {p.x-.85f,p.y,p.z+1.08f}) &&
                Igual(derecha.posicion, {p.x+.85f,p.y,p.z+1.08f}), "Bisagras originales de las puertas");
            BoundingBox bi = GetModelBoundingBox(almacen.cajasPuerto[MODELO_CAJAS_PUERTA_DERECHA].modelo);
            BoundingBox bd = GetModelBoundingBox(almacen.cajasPuerto[MODELO_CAJAS_PUERTA_IZQUIERDA].modelo);
            Comprobar(Cerca(izquierda.posicion.x + bi.max.x * izquierda.escala.x, p.x) &&
                Cerca(derecha.posicion.x + bd.min.x * derecha.escala.x, p.x), "Puertas cerradas llegan al centro");
        }
        m.cursor = 7;
        Avanzar(zona, 30);
        DibujarPrueba(zona, cantidad == 4 ? "build/cajas-grua.png" : nullptr);
        Comprobar(m.ganchoX > 7.0f, "Movimiento real de la grua hacia el cursor");
        ComprobarGrua(m);
        m.fase = FASE_CAJAS_RESOLVER;
        m.cantidadElegidos = n >= 2 ? 2 : 1;
        m.ordenElegidos[0] = 2;
        m.ordenElegidos[1] = 4;
        m.contenedores[2].elegido = true;
        m.contenedores[4].elegido = n >= 2;
        Avanzar(zona, 30);
        Comprobar(m.caidaIniciada && m.contenedores[2].caida > 0, "Caida antes del golpe");
        DibujarPrueba(zona, cantidad == 4 ? "build/cajas-caida.png" : nullptr);
        Avanzar(zona, 18);
        Comprobar(!m.estadosJugadores[escondidos[0]].vivo, "Golpe elimina ocupante sin refuerzo");
        Comprobar(m.contenedores[2].marcas == 1 && m.contenedores[2].humo > 0,
            "Golpe conserva marcas y humo");
        DibujarPrueba(zona, cantidad == 4 ? "build/cajas-golpe.png" : nullptr);
        Comprobar(cantidades[MODELO_CAJAS_MARCA_GOLPE] == 1, "Marca visible solo despues del golpe");
        if (n >= 2)
        {
            Avanzar(zona, 67);
            Comprobar(m.contenedores[4].marcas == 1 && m.estadosJugadores[escondidos[1]].vivo,
                "Ancla protege al ocupante del segundo golpe");
            DibujarPrueba(zona, cantidad == 4 ? "build/cajas-refuerzo.png" : nullptr);
            Comprobar(cantidades[MODELO_CAJAS_ANCLA] == 1 && cantidades[MODELO_CAJAS_MARCA_GOLPE] == 2,
                "Ancla y marcas siguen el estado");
        }
        if (cantidad == 4)
        {
            int presentes[CANTIDAD_MODELOS_CAJAS_PUERTO_3D]{};
            std::memcpy(presentes, cantidades, sizeof(presentes));
            for (int ausente = 0; ausente < CANTIDAD_MODELOS_CAJAS_PUERTO_3D; ausente++)
            {
                almacen.cajasPuerto[ausente].cargado = false;
                DibujarPrueba(zona);
                for (int i = 0; i < CANTIDAD_MODELOS_CAJAS_PUERTO_3D; i++)
                    Comprobar(cantidades[i] == (i == ausente ? 0 : presentes[i]),
                        "Fallback por pieza conserva los demas GLB");
                almacen.cajasPuerto[ausente].cargado = true;
            }
        }
        Avanzar(zona, 160);
        if (n >= 2)
        {
            Comprobar(m.ronda == 2 && m.fase == FASE_CAJAS_ESCONDER, "Nueva ronda conserva recursos");
            // Ultima ronda, sin blancos pendientes: comprobar la finalizacion real.
            m.ronda = RONDAS_CAJAS;
            m.fase = FASE_CAJAS_RESOLVER;
            m.cantidadElegidos = 0;
            m.pasoResolucion = 0;
            m.tiempoFinResolucion = 0;
            Avanzar(zona, 122);
        }
        Comprobar(m.fase == FASE_CAJAS_TERMINADO && ResultadoMinijuegoFinalizado(m.resultado), "Resultado final");
        DibujarPrueba(zona, cantidad == 4 ? "build/cajas-final.png" : nullptr);
        teclaSimulada = KEY_R;
        Avanzar(zona, 1);
        teclaSimulada = KEY_NULL;
        Comprobar(m.fase == FASE_CAJAS_PREPARACION, "R reinicia desde ZonaPruebas");
        teclaSimulada = KEY_ESCAPE;
        Avanzar(zona, 1);
        teclaSimulada = KEY_NULL;
        Comprobar(zona.volverAlMenu, "ESC solicita regresar al menu");
        zona.Inicializar(participantes, cantidad);
        zona.modoCatalogo = true;
        zona.modoTablero = true;
        zona.CambiarMinijuego(MINIJUEGO_CAJAS_PUERTO);
        DibujarPrueba(zona);
        teclaSimulada = KEY_R;
        Avanzar(zona, 1);
        teclaSimulada = KEY_NULL;
        Comprobar(m.tiempoPreparacion < 3.0f, "Partida oficial ignora reinicio con R");
        for (int i = 0; i < CANTIDAD_MODELOS_CAJAS_PUERTO_3D; i++)
            Comprobar(mallas[i] == almacen.cajasPuerto[i].modelo.meshes, "Reinicio/selector/tablero comparten recursos");
        for (const auto& r : almacen.ultimoAsiento)
            Comprobar(!r.cargaIntentada, "Puerto no carga otros paquetes");
        Comprobar(!almacen.montanaLava.cargaIntentada, "Puerto no carga montana");
    }
    RecursoModeloEscenarioRetro3D ausente;
    for (int i = 0; i < 5; i++)
        PrepararSlotModeloEscenarioRetro3D(ausente, "build/modelo-cajas-inexistente.glb", 0);
    Comprobar(avisosAusente == 1 && !ausente.cargado, "Registrar carga fallida una sola vez");
    observar = false;
    zona.Descargar();
    zona.Descargar();
    for (const auto& r : almacen.cajasPuerto)
        Comprobar(!r.cargado && !r.cargaIntentada && !r.modelo.meshes, "Descarga simetrica e idempotente");
    DescargarModeloJugadorCompartido();
    CloseWindow();
    std::printf("Verificacion Cajas del Puerto 2/3/4 jugadores: %d errores\n", errores);
    return errores == 0 ? 0 : 1;
}
