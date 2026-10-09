// Prueba del minijuego real mediante ZonaPruebas y un contexto OpenGL.
// Los wrappers observan recursos/dibujo y simulan entradas sin cambiar el juego.
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
static const char* rutaAusente = nullptr;
static Color originales[CANTIDAD_MODELOS_VETA_CRISTAL_3D][16]{};
static int cargas[CANTIDAD_MODELOS_VETA_CRISTAL_3D]{};
static int descargas[CANTIDAD_MODELOS_VETA_CRISTAL_3D]{};
static int cantidades[CANTIDAD_MODELOS_VETA_CRISTAL_3D]{};
static int cubos = 0, esferasEx = 0, circulos = 0, esferas = 0;

struct DibujoObservado
{
    Vector3 posicion{}, escala{};
    float angulo = 0;
    Color color{};
};
static DibujoObservado dibujos[CANTIDAD_MODELOS_VETA_CRISTAL_3D][48]{};

static void Comprobar(bool condicion, const char* detalle)
{
    if (!condicion)
    {
        if (errores < 20) std::fprintf(stderr, "FALLO: %s\n", detalle);
        errores++;
    }
}
static bool Cerca(float a, float b) { return std::fabs(a - b) < 0.002f; }
static bool Igual(Vector3 a, Vector3 b)
{
    return Cerca(a.x, b.x) && Cerca(a.y, b.y) && Cerca(a.z, b.z);
}
static int IndiceRuta(const char* ruta)
{
    for (int i = 0; i < CANTIDAD_MODELOS_VETA_CRISTAL_3D; i++)
        if (std::strcmp(ruta, RUTAS_MODELOS_VETA_CRISTAL_3D[i]) == 0) return i;
    return -1;
}
static void RegistrarPrueba(int nivel, const char* formato, va_list argumentos)
{
    char texto[2048];
    std::vsnprintf(texto, sizeof(texto), formato, argumentos);
    if (std::strstr(texto, "Modelo de escenario ausente")) avisosAusente++;
    if (nivel >= LOG_WARNING) std::fprintf(stderr, "%s\n", texto);
}
extern "C" bool __real_IsKeyPressed(int);
extern "C" bool __wrap_IsKeyPressed(int tecla)
{
    return tecla == teclaSimulada || __real_IsKeyPressed(tecla);
}
extern "C" bool __real_FileExists(const char*);
extern "C" bool __wrap_FileExists(const char* ruta)
{
    if (rutaAusente && std::strcmp(ruta, rutaAusente) == 0) return false;
    return __real_FileExists(ruta);
}
extern "C" Model __real_LoadModel(const char*);
extern "C" Model __wrap_LoadModel(const char* ruta)
{
    int i = IndiceRuta(ruta);
    if (i >= 0) cargas[i]++;
    return __real_LoadModel(ruta);
}
extern "C" void __real_UnloadModel(Model);
extern "C" void __wrap_UnloadModel(Model modelo)
{
    auto& recursos = ObtenerModelosEscenariosRetro3D().vetaCristal;
    for (int i = 0; i < CANTIDAD_MODELOS_VETA_CRISTAL_3D; i++)
        if (modelo.meshes && modelo.meshes == recursos[i].modelo.meshes) descargas[i]++;
    __real_UnloadModel(modelo);
}
extern "C" void __real_DrawModelEx(Model, Vector3, Vector3, float, Vector3, Color);
extern "C" void __wrap_DrawModelEx(Model modelo, Vector3 posicion, Vector3 eje,
    float angulo, Vector3 escala, Color tinte)
{
    auto& recursos = ObtenerModelosEscenariosRetro3D().vetaCristal;
    for (int i = 0; i < CANTIDAD_MODELOS_VETA_CRISTAL_3D; i++)
    {
        const auto& r = recursos[i];
        if (!r.cargado || modelo.meshes != r.modelo.meshes) continue;
        int n = cantidades[i]++;
        Comprobar(n < 48, "Limite de instancias observadas");
        Color color = r.materialColor < 0 ? WHITE :
            modelo.materials[r.materialColor].maps[MATERIAL_MAP_DIFFUSE].color;
        if (n < 48) dibujos[i][n] = { posicion, escala, angulo, color };
        Comprobar(ColorIsEqual(tinte, WHITE), "Conservar materiales/colores de vertice sin tinte global");
        Comprobar(Igual(eje, {0,1,0}), "Rotacion explicita alrededor de Y");
        Matrix identidad = MatrixIdentity();
        Comprobar(std::memcmp(&modelo.transform, &identidad, sizeof(Matrix)) == 0,
            "No normalizar pivotes ni duplicar las transformaciones importadas");
        for (int j = 0; j < modelo.materialCount && j < 16; j++)
            if (j != r.materialColor)
                Comprobar(ColorIsEqual(originales[i][j],
                    modelo.materials[j].maps[MATERIAL_MAP_DIFFUSE].color),
                    "No modificar materiales estaticos");
        break;
    }
    __real_DrawModelEx(modelo, posicion, eje, angulo, escala, tinte);
}
extern "C" void __real_DrawCube(Vector3, float, float, float, Color);
extern "C" void __wrap_DrawCube(Vector3 p, float x, float y, float z, Color c)
{
    cubos++;
    __real_DrawCube(p,x,y,z,c);
}
extern "C" void __real_DrawSphereEx(Vector3, float, int, int, Color);
extern "C" void __wrap_DrawSphereEx(Vector3 p, float r, int anillos, int lados, Color c)
{
    esferasEx++;
    __real_DrawSphereEx(p,r,anillos,lados,c);
}
extern "C" void __real_DrawSphere(Vector3, float, Color);
extern "C" void __wrap_DrawSphere(Vector3 p, float r, Color c)
{
    esferas++;
    __real_DrawSphere(p,r,c);
}
extern "C" void __real_DrawCircle3D(Vector3, float, Vector3, float, Color);
extern "C" void __wrap_DrawCircle3D(Vector3 p, float r, Vector3 eje, float a, Color c)
{
    circulos++;
    __real_DrawCircle3D(p,r,eje,a,c);
}

static void GuardarMateriales()
{
    auto& recursos = ObtenerModelosEscenariosRetro3D().vetaCristal;
    for (int i = 0; i < CANTIDAD_MODELOS_VETA_CRISTAL_3D; i++)
        for (int j = 0; j < recursos[i].modelo.materialCount && j < 16; j++)
            originales[i][j] = recursos[i].modelo.materials[j].maps[MATERIAL_MAP_DIFFUSE].color;
}
static void DibujarPrueba(ZonaPruebas& zona, const char* captura = nullptr)
{
    std::memset(cantidades, 0, sizeof(cantidades));
    cubos = esferasEx = circulos = esferas = 0;
    Vector3 posiciones[MAX_PARTICIPANTES]{}, tamanos[MAX_PARTICIPANTES]{};
    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        posiciones[i] = zona.jugadores[i].posicion;
        tamanos[i] = zona.jugadores[i].tamano;
    }
    auto& m = zona.gestorMinijuegos.minijuegoVetaCristal;
    Camera3D camara = m.camara;
    BeginDrawing();
    zona.Dibujar();
    if (captura)
    {
        rlDrawRenderBatchActive();
        Image imagen = LoadImageFromScreen();
        Comprobar(ExportImage(imagen, captura), "Captura del minijuego integrado");
        UnloadImage(imagen);
    }
    EndDrawing();
    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        Comprobar(Igual(posiciones[i], zona.jugadores[i].posicion), "Dibujo conserva posiciones");
        Comprobar(Igual(tamanos[i], zona.jugadores[i].tamano), "Dibujo conserva hitboxes");
    }
    Comprobar(Igual(camara.position, m.camara.position) && Igual(camara.target, m.camara.target),
        "Dibujo conserva camara");
    auto& recursos = ObtenerModelosEscenariosRetro3D().vetaCristal;
    for (int i = 0; i < CANTIDAD_MODELOS_VETA_CRISTAL_3D; i++)
        for (int j = 0; j < recursos[i].modelo.materialCount && j < 16; j++)
            Comprobar(ColorIsEqual(originales[i][j],
                recursos[i].modelo.materials[j].maps[MATERIAL_MAP_DIFFUSE].color),
                "Material restaurado despues de dibujar");
}
static void Avanzar(ZonaPruebas& zona, int frames)
{
    for (int i = 0; i < frames; i++) zona.Actualizar(1.0f / 60.0f);
}
static void VaciarGemas(MinijuegoVetaCristal& m)
{
    for (auto& g : m.gemas) g = {};
}
static void PrepararCabezazo(ZonaPruebas& zona, int jugador, int geoda)
{
    const auto& g = zona.gestorMinijuegos.minijuegoVetaCristal.geodas[geoda];
    auto& j = zona.jugadores[jugador];
    j.posicion = {g.x,1.49f,g.z};
    j.velocidad = {0,4,0};
    j.empuje = {};
    j.enSuelo = false;
}

int main()
{
    SetTraceLogCallback(RegistrarPrueba);
    SetConfigFlags(FLAG_WINDOW_HIDDEN);
    InitWindow(1280,800,"Verificacion integrada de Veta de Cristal");
    if (!IsWindowReady()) return 2;
    SetExitKey(KEY_NULL);
    SetRandomSeed(1701);
    static ZonaPruebas zona;
    Participante participantes[MAX_PARTICIPANTES]{};
    auto& almacen = ObtenerModelosEscenariosRetro3D();
    Mesh* mallas[CANTIDAD_MODELOS_VETA_CRISTAL_3D]{};
    const Color colores[] = {RED,BLUE,GREEN,YELLOW};
    for (int cantidad = 2; cantidad <= 4; cantidad++)
    {
        for (int i = 0; i < MAX_PARTICIPANTES; i++)
        {
            participantes[i] = {};
            participantes[i].activo = i < cantidad;
            participantes[i].conectado = true;
            participantes[i].control = i % 2 ? CONTROL_TECLADO_FLECHAS : CONTROL_TECLADO_WASD;
            participantes[i].numeroJugador = i+1;
            participantes[i].color = colores[i];
        }
        zona.Inicializar(participantes,cantidad);
        if (cantidad == 2)
            for (const auto& r : almacen.vetaCristal)
                Comprobar(!r.cargaIntentada, "No cargar la mina al arrancar");
        zona.modoCatalogo = true;
        zona.CambiarMinijuego(MINIJUEGO_VETA_CRISTAL);
        auto& m = zona.gestorMinijuegos.minijuegoVetaCristal;
        for (int i = 0; i < CANTIDAD_MODELOS_VETA_CRISTAL_3D; i++)
        {
            const auto& r = almacen.vetaCristal[i];
            Comprobar(r.cargado, RUTAS_MODELOS_VETA_CRISTAL_3D[i]);
            if (!r.cargado) { zona.Descargar(); DescargarModeloJugadorCompartido(); CloseWindow(); return 2; }
            if (cantidad == 2) mallas[i] = r.modelo.meshes;
            Comprobar(mallas[i] == r.modelo.meshes && cargas[i] == 1, "Carga unica entre participantes y entradas");
            Comprobar(r.modelo.materialCount <= 16, "Limite de materiales observados");
            Comprobar((i == MODELO_VETA_MARCA) == (r.materialColor >= 0), "Solo marca_geoda permite color dinamico");
            for (int j = 0; j < r.modelo.meshCount; j++)
                Comprobar(r.modelo.meshes[j].colors != nullptr, "Preservar colores de vertices del GLB");
        }
        GuardarMateriales();
        Comprobar(m.partidaValida && m.fase == FASE_VETA_PREPARACION, "Preparacion con 2/3/4 participantes");
        Comprobar(m.cantidadJugadoresEquipo[0] == (cantidad >= 3 ? 2 : 1) &&
            m.cantidadJugadoresEquipo[1] == cantidad - m.cantidadJugadoresEquipo[0], "1v1 / 2v1 / 2v2");
        DibujarPrueba(zona, cantidad == 4 ? "build/veta-preparacion.png" : nullptr);
        const int esperados[] = {1,1,34,1,6,6,1,0,8,2,0,0,0,0,12};
        for (int i = 0; i < CANTIDAD_MODELOS_VETA_CRISTAL_3D; i++)
            Comprobar(cantidades[i] == esperados[i], "Instancias iniciales sin primitivas duplicadas");
        Comprobar(cubos == 0 && esferasEx == 0, "Omitir suelo, paredes, geodas y vetas primitivas cargadas");
        Comprobar(Igual(dibujos[MODELO_VETA_SUELO][0].posicion,{0,0,0}) &&
            Igual(dibujos[MODELO_VETA_PORTICO][0].posicion,{0,0,0}), "Origen comun de piezas modulares");
        for (int g = 0, pequena = 0, grande = 0, marca = 0; g < MAX_GEODAS_VETA; g++)
        {
            const auto& geoda = m.geodas[g];
            int pieza = geoda.grande ? MODELO_VETA_GEODA_GRANDE : MODELO_VETA_GEODA_PEQUENA;
            int n = geoda.grande ? grande++ : pequena++;
            Comprobar(Igual(dibujos[pieza][n].posicion, {geoda.x,geoda.grande?3.10f:2.85f,geoda.z}),
                "Geoda centrada en su bloque logico");
            Color cristal = geoda.grande ? Color{255,200,70,255} : Color{90,220,255,255};
            Comprobar(ColorIsEqual(dibujos[MODELO_VETA_MARCA][marca].color,Fade(cristal,.8f)),
                "Color de marca segun tipo exclusivamente en COLOR_DINAMICO");
            marca += geoda.grande ? 2 : 1;
        }
        for (int i = 26; i < 34; i++)
            Comprobar(Cerca(dibujos[MODELO_VETA_MINERAL_PARED][i].angulo, i%2 ? -90.0f : 90.0f),
                "Vetas laterales orientadas hacia la arena");
        Avanzar(zona,182);
        Comprobar(m.fase == FASE_VETA_JUGANDO, "Fin real de la cuenta regresiva");
        int lado = m.estadosJugadores[0].equipo;
        int pequena = lado * 5, grande = pequena + 4;
        PrepararCabezazo(zona,0,pequena);
        Avanzar(zona,1);
        Comprobar(!m.geodas[pequena].cargada && m.geodas[pequena].tiempoRecarga > 4.9f,
            "Cabezazo descarga geoda pequena con las colisiones originales");
        Comprobar(m.geodas[pequena].tiempoSacudida > 0, "Sacudida tras golpe");
        VaciarGemas(m);
        int companero = -1;
        for (int j = 1; j < cantidad; j++)
            if (m.estadosJugadores[j].equipo == lado) companero = j;
        PrepararCabezazo(zona,0,grande);
        Avanzar(zona,1);
        if (companero >= 0)
        {
            Comprobar(m.geodas[grande].cargada && m.geodas[grande].ventanaGolpe > 0,
                "Primer golpe abre ventana de coordinacion");
            PrepararCabezazo(zona,companero,grande);
            Avanzar(zona,1);
        }
        Comprobar(!m.geodas[grande].cargada && m.gemas[0].activa &&
            m.gemas[0].valor == (companero >= 0 ? 6 : 4), "Tesoro de geoda grande segun equipo");
        VaciarGemas(m);
        // Ejercitar todos los GLB de estado manteniendo los datos y dibujo reales.
        for (int g = 0; g < MAX_GEODAS_VETA; g++)
        {
            m.geodas[g].cargada = g != 0 && g != 4;
            m.geodas[g].tiempoRecarga = m.geodas[g].cargada ? 0 : 5;
            m.geodas[g].tiempoSacudida = .2f;
        }
        m.tiempoAnimacion = .37f;
        const int valores[] = {1,3,4,6};
        for (int g = 0; g < 4; g++)
        {
            m.gemas[g].activa = true;
            m.gemas[g].x = -8.0f + g*4;
            m.gemas[g].z = 5;
            m.gemas[g].y = .4f + g*.1f;
            m.gemas[g].valor = valores[g];
        }
        m.vagonetaActiva = true;
        m.vagonetaZ = 2;
        m.direccionVagoneta = -1;
        DibujarPrueba(zona, cantidad == 4 ? "build/veta-estados.png" : nullptr);
        Comprobar(cantidades[MODELO_VETA_GEODA_AGOTADA] == 2 &&
            Cerca(dibujos[MODELO_VETA_GEODA_AGOTADA][1].escala.x,.9f/.65f), "Compartir agotada pequena/grande");
        Comprobar(Igual(dibujos[MODELO_VETA_VAGONETA][0].posicion,{0,0,2}) &&
            Cerca(dibujos[MODELO_VETA_VAGONETA][0].angulo,180), "Vagoneta sigue posicion y sentido");
        Comprobar(cubos == 1 && esferasEx == 0, "Solo conservar cubo de aviso, halos y efectos");
        for (int g = 0, violeta = 0; g < 4; g++)
        {
            int pieza = g == 0 ? MODELO_VETA_GEMA_AZUL : g == 1 ? MODELO_VETA_GEMA_DORADA : MODELO_VETA_GEMA_VIOLETA;
            int n = g >= 2 ? violeta++ : 0;
            const auto& gema = m.gemas[g];
            Comprobar(Igual(dibujos[pieza][n].posicion, {gema.x,
                gema.y+.08f*std::sin(m.tiempoAnimacion*5+gema.x*2),gema.z}), "Gemas conservan flotacion y radio visual");
        }
        // Sacudida conectada al mismo centro, sin mover la marca del suelo.
        Comprobar(Cerca(dibujos[MODELO_VETA_GEODA_AGOTADA][0].posicion.x,
            m.geodas[0].x+std::sin(m.tiempoAnimacion*60)*.07f*(.2f/.35f)), "Sacudida procedural del GLB");
        m.gemas[0].edad = 26;
        m.tiempoAnimacion = .25f; // seno negativo: parpadeo final.
        DibujarPrueba(zona);
        Comprobar(cantidades[MODELO_VETA_GEMA_AZUL] == 0, "Parpadeo antes de expirar");
        m.gemas[0].edad = 0;
        // Colision con vagoneta y posterior inmunidad, sin derivar hitboxes de GLB.
        VaciarGemas(m);
        zona.jugadores[0].posicion = {0,.7f,0};
        zona.jugadores[0].velocidad = zona.jugadores[0].empuje = {};
        zona.jugadores[0].enSuelo = true;
        m.estadosJugadores[0].gemas = 5;
        m.vagonetaZ = -1;
        m.direccionVagoneta = 1;
        Avanzar(zona,1);
        Comprobar(m.estadosJugadores[0].gemas == 3 && m.estadosJugadores[0].tiempoAturdido > 0 &&
            m.estadosJugadores[0].tiempoInmune > 2, "Vagoneta aturde y hace soltar dos gemas");
        DibujarPrueba(zona, cantidad == 4 ? "build/veta-choque.png" : nullptr);
        Avanzar(zona,1);
        Comprobar(m.estadosJugadores[0].gemas == 3, "Inmunidad evita un segundo golpe inmediato");
        m.vagonetaZ = 9.4f;
        Avanzar(zona,1);
        Comprobar(!m.vagonetaActiva, "Vagoneta sale del tunel");
        m.tiempoHastaVagoneta = 1;
        Avanzar(zona,1);
        Comprobar(m.avisoVagoneta && m.direccionVagoneta < 0, "Aviso y cambio de sentido");
        m.tiempoHastaVagoneta = .001f;
        Avanzar(zona,1);
        Comprobar(m.vagonetaActiva && Cerca(m.vagonetaZ,9.5f), "Nueva entrada desde extremo opuesto");
        Avanzar(zona,1);
        Comprobar(m.vagonetaZ < 9.5f, "Avance por Z en sentido inverso");
        m.vagonetaActiva = m.avisoVagoneta = false;
        m.tiempoHastaVagoneta = 10;
        m.estadosJugadores[0].tiempoAturdido = 0;
        zona.jugadores[0].posicion = {-2,.7f,4};
        zona.jugadores[0].velocidad = zona.jugadores[0].empuje = {};
        VaciarGemas(m);
        for (int g = 0; g < 4; g++)
        {
            m.gemas[g].activa = true;
            m.gemas[g].x = -2; m.gemas[g].z = 4; m.gemas[g].y = .4f;
            m.gemas[g].valor = valores[g];
        }
        int puntaje = m.estadosJugadores[0].gemas;
        Avanzar(zona,1);
        Comprobar(m.estadosJugadores[0].gemas == puntaje+14 && m.ultimaDoradaEquipo == lado,
            "Recogida y puntuacion de los cuatro valores");
        m.geodas[pequena].cargada = false;
        m.geodas[pequena].tiempoRecarga = .001f;
        Avanzar(zona,1);
        Comprobar(m.geodas[pequena].cargada, "Recarga real de geoda");
        // IA existente: comprobar que progresa una ronda real.
        participantes[0].esBot = true;
        m.estadosJugadores[0].tiempoDecision = 0;
        Vector3 antesBot = zona.jugadores[0].posicion;
        Avanzar(zona,90);
        Comprobar(!Igual(antesBot,zona.jugadores[0].posicion), "IA continua moviendo al participante");
        participantes[0].esBot = false;
        TipoControl controlPrevio = participantes[0].control;
        participantes[0].control = CONTROL_GAMEPAD;
        participantes[0].indiceGamepad = -1;
        participantes[0].conectado = false;
        m.estadosJugadores[0].tiempoDecision = 0;
        antesBot = zona.jugadores[0].posicion;
        Avanzar(zona,90);
        Comprobar(!participantes[0].conectado && !Igual(antesBot,zona.jugadores[0].posicion),
            "IA mantiene la ronda con un mando ausente");
        participantes[0].control = controlPrevio;
        m.tiempoRestante = .001f;
        Avanzar(zona,1);
        Comprobar(m.fase == FASE_VETA_TERMINADO && ResultadoMinijuegoFinalizado(m.resultado), "Resultado final real");
        DibujarPrueba(zona, cantidad == 4 ? "build/veta-final.png" : nullptr);
        teclaSimulada = KEY_R;
        Avanzar(zona,1);
        teclaSimulada = KEY_NULL;
        Comprobar(m.fase == FASE_VETA_PREPARACION, "R reinicia en ZonaPruebas");
        teclaSimulada = KEY_ESCAPE;
        Avanzar(zona,1);
        teclaSimulada = KEY_NULL;
        Comprobar(zona.volverAlMenu, "ESC vuelve al selector/menu");
        zona.Inicializar(participantes,cantidad);
        zona.modoCatalogo = zona.modoTablero = true;
        zona.CambiarMinijuego(MINIJUEGO_VETA_CRISTAL);
        teclaSimulada = KEY_R;
        Avanzar(zona,1);
        teclaSimulada = KEY_NULL;
        Comprobar(m.tiempoPreparacion < 3, "R no reinicia la ronda oficial de tablero");
        for (int i = 0; i < CANTIDAD_MODELOS_VETA_CRISTAL_3D; i++)
            Comprobar(mallas[i] == almacen.vetaCristal[i].modelo.meshes && cargas[i] == 1,
                "Reinicio, selector y tablero reutilizan el paquete");
        DibujarPrueba(zona);
    }
    // Fallos de carga reales simulados por ruta, sin mover ni editar los GLB.
    // Con todos los estados visibles verificar fallback independiente por pieza.
    auto& m = zona.gestorMinijuegos.minijuegoVetaCristal;
    m.vagonetaActiva = true;
    m.geodas[0].cargada = m.geodas[4].cargada = false;
    const int valores[] = {1,3,6};
    for (int g = 0; g < 3; g++)
    {
        m.gemas[g].activa = true; m.gemas[g].valor = valores[g];
        m.gemas[g].x = -4+g*4; m.gemas[g].y = .5f; m.gemas[g].z = 5;
    }
    DibujarPrueba(zona);
    int presentes[CANTIDAD_MODELOS_VETA_CRISTAL_3D];
    std::memcpy(presentes,cantidades,sizeof(presentes));
    int cubosBase=cubos, esferasBase=esferasEx, circulosBase=circulos, halosBase=esferas;
    for (int ausente = 0; ausente < CANTIDAD_MODELOS_VETA_CRISTAL_3D; ausente++)
    {
        auto& r = almacen.vetaCristal[ausente];
        DescargarSlotModeloEscenarioRetro3D(r);
        rutaAusente = RUTAS_MODELOS_VETA_CRISTAL_3D[ausente];
        for (int repeticion = 0; repeticion < 3; repeticion++)
            CargarPaqueteModelosEscenarioRetro3D(ObtenerPaqueteVetaCristalRetro3D());
        Comprobar(!r.cargado && r.cargaIntentada, "Fallo persistente sin reintentos por frame");
        DibujarPrueba(zona);
        for (int i = 0; i < CANTIDAD_MODELOS_VETA_CRISTAL_3D; i++)
            Comprobar(cantidades[i] == (i == ausente ? 0 : presentes[i]), "Fallback solo de la pieza ausente");
        Comprobar(cubos > cubosBase || esferasEx > esferasBase || circulos > circulosBase || esferas > halosBase,
            "La pieza fallida dibuja sus primitivas originales");
        rutaAusente = nullptr;
        r = {};
        CargarPaqueteModelosEscenarioRetro3D(ObtenerPaqueteVetaCristalRetro3D());
        GuardarMateriales();
    }
    Comprobar(avisosAusente == CANTIDAD_MODELOS_VETA_CRISTAL_3D, "Un solo diagnostico por recurso fallido");
    for (const auto& r : almacen.ultimoAsiento) Comprobar(!r.cargaIntentada, "No cargar Ultimo Asiento");
    for (const auto& r : almacen.cajasPuerto) Comprobar(!r.cargaIntentada, "No cargar Puerto");
    for (const auto& r : almacen.laberintoJade) Comprobar(!r.cargaIntentada, "No cargar Jade");
    zona.Descargar();
    zona.Descargar();
    for (int i = 0; i < CANTIDAD_MODELOS_VETA_CRISTAL_3D; i++)
    {
        const auto& r = almacen.vetaCristal[i];
        Comprobar(!r.cargado && !r.cargaIntentada && !r.modelo.meshes && cargas[i] == descargas[i],
            "Descarga simetrica e idempotente antes de CloseWindow");
    }
    DescargarModeloJugadorCompartido();
    CloseWindow();
    std::printf("Verificacion Veta de Cristal 2/3/4 participantes: %d errores\n",errores);
    return errores == 0 ? 0 : 1;
}
