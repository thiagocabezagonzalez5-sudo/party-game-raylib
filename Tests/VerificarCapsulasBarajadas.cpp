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
#include <initializer_list>

static int errores = 0;
static int avisosAusente = 0;
static int teclaSimulada = KEY_NULL;
static int teclaMantenida = KEY_NULL;
static const char* rutaAusente = nullptr;
static Color originales[CANTIDAD_MODELOS_CAPSULAS_BARAJADAS_3D][16]{};
static int cargas[CANTIDAD_MODELOS_CAPSULAS_BARAJADAS_3D]{};
static int descargas[CANTIDAD_MODELOS_CAPSULAS_BARAJADAS_3D]{};
static int cantidades[CANTIDAD_MODELOS_CAPSULAS_BARAJADAS_3D]{};
static int cubos = 0, esferasEx = 0, circulos = 0, esferas = 0;
static int cilindros = 0, segmentos = 0;
static int barras = 0, burbujas = 0;
static float alturasBarras[24]{}, alturasBurbujas[16]{};

struct DibujoObservado
{
    Vector3 posicion{}, escala{};
    Vector3 eje{};
    float angulo = 0;
    Color color{};
};
static DibujoObservado dibujos[CANTIDAD_MODELOS_CAPSULAS_BARAJADAS_3D][48]{};

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
    for (int i = 0; i < CANTIDAD_MODELOS_CAPSULAS_BARAJADAS_3D; i++)
        if (std::strcmp(ruta, RUTAS_MODELOS_CAPSULAS_BARAJADAS_3D[i]) == 0) return i;
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
extern "C" bool __real_IsKeyDown(int);
extern "C" bool __wrap_IsKeyDown(int tecla)
{
    return tecla == teclaMantenida || __real_IsKeyDown(tecla);
}
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
    auto& recursos = ObtenerModelosEscenariosRetro3D().capsulasBarajadas;
    for (int i = 0; i < CANTIDAD_MODELOS_CAPSULAS_BARAJADAS_3D; i++)
        if (modelo.meshes && modelo.meshes == recursos[i].modelo.meshes) descargas[i]++;
    __real_UnloadModel(modelo);
}
extern "C" void __real_DrawModelEx(Model, Vector3, Vector3, float, Vector3, Color);
extern "C" void __wrap_DrawModelEx(Model modelo, Vector3 posicion, Vector3 eje,
    float angulo, Vector3 escala, Color tinte)
{
    auto& recursos = ObtenerModelosEscenariosRetro3D().capsulasBarajadas;
    for (int i = 0; i < CANTIDAD_MODELOS_CAPSULAS_BARAJADAS_3D; i++)
    {
        const auto& r = recursos[i];
        if (!r.cargado) continue;
        int malla = -1;
        for (int j = 0; j < r.modelo.meshCount; j++)
            if (modelo.meshes == &r.modelo.meshes[j]) malla = j;
        if (malla < 0) continue;
        int n = cantidades[i]++;
        Comprobar(n < 48, "Limite de instancias observadas");
        Color color = r.materialColor < 0 ? WHITE :
            modelo.materials[r.materialColor].maps[MATERIAL_MAP_DIFFUSE].color;
        if (n < 48) dibujos[i][n] = { posicion, escala, eje, angulo, color };
        Comprobar(ColorIsEqual(tinte, WHITE), "Conservar materiales/colores de vertice sin tinte global");
        if (i == MODELO_CAPSULAS_MONITOR || i == MODELO_CAPSULAS_TUBO)
        {
            Comprobar(modelo.meshCount == 1 &&
                malla != (i == MODELO_CAPSULAS_MONITOR ? 2 : 4), "Omitir barras/burbujas estaticas del GLB");
            Comprobar(modelo.meshMaterial == &r.modelo.meshMaterial[malla], "Vista comparte malla y material sin copiarlos");
        }
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
    if (Cerca(x,.3f) && Cerca(z,.04f) && barras < 24) alturasBarras[barras++] = y;
    __real_DrawCube(p,x,y,z,c);
}
extern "C" void __real_DrawSphereEx(Vector3, float, int, int, Color);
extern "C" void __wrap_DrawSphereEx(Vector3 p, float r, int anillos, int lados, Color c)
{
    esferasEx++;
    if (Cerca(r,.07f) && burbujas < 16) alturasBurbujas[burbujas++] = p.y;
    __real_DrawSphereEx(p,r,anillos,lados,c);
}
extern "C" void __real_DrawSphere(Vector3, float, Color);
extern "C" void __real_DrawCylinder(Vector3, float, float, float, int, Color);
extern "C" void __wrap_DrawCylinder(Vector3 p, float superior, float inferior, float h, int lados, Color c)
{
    cilindros++;
    __real_DrawCylinder(p,superior,inferior,h,lados,c);
}
extern "C" void __real_DrawCylinderEx(Vector3, Vector3, float, float, int, Color);
extern "C" void __wrap_DrawCylinderEx(Vector3 a, Vector3 b, float ra, float rb, int lados, Color c)
{
    segmentos++;
    __real_DrawCylinderEx(a,b,ra,rb,lados,c);
}
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
    auto& recursos = ObtenerModelosEscenariosRetro3D().capsulasBarajadas;
    for (int i = 0; i < CANTIDAD_MODELOS_CAPSULAS_BARAJADAS_3D; i++)
        for (int j = 0; j < recursos[i].modelo.materialCount && j < 16; j++)
            originales[i][j] = recursos[i].modelo.materials[j].maps[MATERIAL_MAP_DIFFUSE].color;
}
static void DibujarPrueba(ZonaPruebas& zona, const char* captura = nullptr)
{
    std::memset(cantidades, 0, sizeof(cantidades));
    cubos = esferasEx = circulos = esferas = 0;
    cilindros = segmentos = barras = burbujas = 0;
    Vector3 posiciones[MAX_PARTICIPANTES]{}, tamanos[MAX_PARTICIPANTES]{};
    for (int i = 0; i < MAX_PARTICIPANTES; i++)
    {
        posiciones[i] = zona.jugadores[i].posicion;
        tamanos[i] = zona.jugadores[i].tamano;
    }
    auto& m = zona.gestorMinijuegos.minijuegoCapsulasBarajadas;
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
    auto& recursos = ObtenerModelosEscenariosRetro3D().capsulasBarajadas;
    for (int i = 0; i < CANTIDAD_MODELOS_CAPSULAS_BARAJADAS_3D; i++)
        for (int j = 0; j < recursos[i].modelo.materialCount && j < 16; j++)
            Comprobar(ColorIsEqual(originales[i][j],
                recursos[i].modelo.materials[j].maps[MATERIAL_MAP_DIFFUSE].color),
                "Material restaurado despues de dibujar");
}
static void Avanzar(ZonaPruebas& zona, int frames)
{
    for (int i = 0; i < frames; i++) zona.Actualizar(1.0f / 60.0f);
}

static void EsperarSubfase(ZonaPruebas& zona, SubfaseCapsulas subfase)
{
    auto& m = zona.gestorMinijuegos.minijuegoCapsulasBarajadas;
    int frames = 0;
    while (m.subfase != subfase && frames++ < 1500) Avanzar(zona,1);
    Comprobar(m.subfase == subfase, "Transicion real de subfase");
}
static float XSlot(int slot, int cantidad)
{
    float separacion = cantidad == 3 ? 2.8f : cantidad == 4 ? 2.5f : 2.1f;
    return (slot-(cantidad-1)*.5f)*separacion;
}
static Vector3 ExtremoSegmento(const DibujoObservado& d)
{
    Vector3 fin = Vector3Transform({0,d.escala.y,0},MatrixRotate(d.eje,d.angulo*DEG2RAD));
    return Vector3Add(d.posicion,fin);
}
static void ComprobarBrazo()
{
    const auto& a = dibujos[MODELO_CAPSULAS_BRAZO_SEGMENTO][0];
    const auto& b = dibujos[MODELO_CAPSULAS_BRAZO_SEGMENTO][1];
    Vector3 mano = dibujos[MODELO_CAPSULAS_BRAZO_PINZA][0].posicion;
    Vector3 codo = dibujos[MODELO_CAPSULAS_BRAZO_ARTICULACION][0].posicion;
    Comprobar(cantidades[MODELO_CAPSULAS_BRAZO_SEGMENTO] == 2, "Dos uniones comparten una malla");
    Comprobar(Igual(a.posicion,{0,4.2f,-3.4f}) && Igual(ExtremoSegmento(a),codo), "Hombro conectado al codo");
    Comprobar(Igual(b.posicion,codo) && Igual(ExtremoSegmento(b),mano), "Codo conectado a la pinza");
    Comprobar(Cerca(a.escala.x,1) && Cerca(a.escala.z,1) &&
        Cerca(b.escala.x,1) && Cerca(b.escala.z,1), "Segmentos escalan solamente en Y");
}
static void ComprobarCapsulas(const MinijuegoCapsulasBarajadas& m, float apertura)
{
    for (int c = 0; c < m.cantidadCapsulas; c++)
    {
        const auto& cuerpo = dibujos[MODELO_CAPSULAS_CUERPO][c];
        const auto& banda = dibujos[MODELO_CAPSULAS_BANDA][c];
        const auto& tapa = dibujos[MODELO_CAPSULAS_TAPA][c];
        float elevacion = m.revelado*m.revelado*(3-2*m.revelado)*1.7f;
        Comprobar(Cerca(cuerpo.posicion.y,1.01f+elevacion) && Igual(banda.posicion,cuerpo.posicion),
            "Cuerpo y banda comparten pivote y elevacion");
        Comprobar(Igual(tapa.posicion,{cuerpo.posicion.x,cuerpo.posicion.y+1.5f+apertura*1.3f,cuerpo.posicion.z}),
            "Tapa conserva apertura y elevacion sin repetir transformaciones");
        Color color = ColorFromHSV(c*68.0f,.75f,.95f);
        Comprobar(ColorIsEqual(banda.color,color) && ColorIsEqual(tapa.color,color), "Banda y tapa segun capsula");
        if (m.swapA < 0)
            Comprobar(Cerca(cuerpo.posicion.x,XSlot(m.slotDe[c],m.cantidadCapsulas)) && Cerca(cuerpo.posicion.z,0),
                "Posicion segun slot logico de la ronda");
    }
}
static void ComprobarMarcadores(const MinijuegoCapsulasBarajadas& m, int cantidad)
{
    Comprobar(cantidades[MODELO_CAPSULAS_MARCADOR] == cantidad, "Un marcador por participante");
    for (int i = 0; i < cantidad; i++)
    {
        const auto& e = m.estados[i];
        int slot = m.subfase == SUBFASE_CAPSULAS_REVELAR ? e.slotElegido : e.marcador;
        const auto& d = dibujos[MODELO_CAPSULAS_MARCADOR][i];
        Comprobar(Igual(d.posicion,{XSlot(slot,m.cantidadCapsulas)+(i-(cantidad-1)*.5f)*.42f,
            3.5f+.08f*std::sin(m.tiempoAnimacion*4+i),1.5f}), "Marcadores conservan separacion, flotacion y posicion real");
        Color color = m.subfase == SUBFASE_CAPSULAS_REVELAR
            ? (e.acerto ? Color{90,255,130,255} : Color{255,80,80,255}) : m.coloresJugadores[i];
        Comprobar(ColorIsEqual(d.color,color), "Marcador toma color de jugador o resultado solo en COLOR_DINAMICO");
    }
}
static void Pulsar(ZonaPruebas& zona, int tecla)
{
    teclaMantenida = teclaSimulada = tecla;
    Avanzar(zona,1);
    teclaMantenida = teclaSimulada = KEY_NULL;
    Avanzar(zona,1);
}

int main()
{
    SetTraceLogCallback(RegistrarPrueba);
    SetConfigFlags(FLAG_WINDOW_HIDDEN);
    InitWindow(1280,800,"Verificacion integrada de Capsulas Barajadas");
    if (!IsWindowReady()) return 2;
    SetExitKey(KEY_NULL);
    SetRandomSeed(1802);
    static ZonaPruebas zona;
    Participante participantes[MAX_PARTICIPANTES]{};
    auto& almacen = ObtenerModelosEscenariosRetro3D();
    auto& recursos = almacen.capsulasBarajadas;
    Mesh* mallas[CANTIDAD_MODELOS_CAPSULAS_BARAJADAS_3D]{};
    const Color colores[] = {RED,BLUE,GREEN,YELLOW};
    for (int cantidad = 2; cantidad <= 4; cantidad++)
    {
        for (int i = 0; i < MAX_PARTICIPANTES; i++)
        {
            participantes[i] = {};
            participantes[i].activo = i < cantidad;
            participantes[i].conectado = true;
            participantes[i].numeroJugador = i+1;
            participantes[i].color = colores[i];
            participantes[i].control = i%2 ? CONTROL_TECLADO_FLECHAS : CONTROL_TECLADO_WASD;
        }
        zona.Inicializar(participantes,cantidad);
        if (cantidad == 2)
            for (const auto& r : recursos) Comprobar(!r.cargaIntentada, "No cargar laboratorio al arrancar");
        zona.modoCatalogo = true;
        zona.CambiarMinijuego(MINIJUEGO_CAPSULAS_BARAJADAS);
        auto& m = zona.gestorMinijuegos.minijuegoCapsulasBarajadas;
        for (int i = 0; i < CANTIDAD_MODELOS_CAPSULAS_BARAJADAS_3D; i++)
        {
            const auto& r = recursos[i];
            Comprobar(r.cargado,RUTAS_MODELOS_CAPSULAS_BARAJADAS_3D[i]);
            if (!r.cargado) { zona.Descargar(); DescargarModeloJugadorCompartido(); CloseWindow(); return 2; }
            if (cantidad == 2) mallas[i] = r.modelo.meshes;
            Comprobar(cargas[i] == 1 && mallas[i] == r.modelo.meshes, "Entrada y participantes comparten las mallas");
            bool dinamico = i == MODELO_CAPSULAS_TUBO || i == MODELO_CAPSULAS_BALIZA ||
                i == MODELO_CAPSULAS_BANDA || i == MODELO_CAPSULAS_TAPA || i == MODELO_CAPSULAS_MARCADOR;
            Comprobar(dinamico == (r.materialColor >= 0), "Solo los cinco materiales dinamicos son tintables");
            for (int j = 0; j < r.modelo.meshCount; j++)
                Comprobar(r.modelo.meshes[j].colors != nullptr, "Colores de vertices preservados");
        }
        GuardarMateriales();
        DibujarPrueba(zona,cantidad == 4 ? "build/capsulas-preparacion.png" : nullptr);
        const int esperados[] = {1,16,1,32,2,1,1,1,2,1,1,3,3,3,1,0};
        for (int i = 0; i < CANTIDAD_MODELOS_CAPSULAS_BARAJADAS_3D; i++)
            Comprobar(cantidades[i] == esperados[i], "Todos los modelos iniciales, sin duplicar piezas");
        Comprobar(cubos == 24 && esferasEx == 16 && cilindros == 0 && segmentos == 0,
            "Solo barras y burbujas procedurales; primitivas reemplazadas omitidas");
        Comprobar(Igual(dibujos[MODELO_CAPSULAS_MESA][0].posicion,{0,0,0}) &&
            Igual(dibujos[MODELO_CAPSULAS_PASARELA][0].posicion,{0,0,4.8f}), "Mesa y pasarela conservan posicion");
        ComprobarBrazo();
        ComprobarCapsulas(m,0);
        for (int k = 0; k < 8; k++)
            Comprobar(ColorIsEqual(dibujos[MODELO_CAPSULAS_TUBO][k*4].color,
                Fade(ColorFromHSV(k*47.0f+100,.7f,.95f),.85f)), "Solo liquido recibe color del tubo");
        float barraAntes = alturasBarras[0], burbujaAntes = alturasBurbujas[0];
        float tiempoAntes = m.tiempoAnimacion;
        m.tiempoAnimacion += .37f;
        DibujarPrueba(zona);
        Comprobar(barras == 24 && burbujas == 16 && !Cerca(barraAntes,alturasBarras[0]) &&
            !Cerca(burbujaAntes,alturasBurbujas[0]), "Conservar animaciones de barras y burbujas");
        m.tiempoAnimacion = tiempoAntes;
        Avanzar(zona,182);
        Comprobar(m.fase == FASE_CAPSULAS_JUGANDO, "Fin de cuenta regresiva");
        const int cantidadesRonda[] = {3,3,4,4,5};
        for (int ronda = 0; ronda < RONDAS_CAPSULAS; ronda++)
        {
            Comprobar(m.ronda == ronda && m.cantidadCapsulas == cantidadesRonda[ronda], "3/3/4/4/5 capsulas por ronda");
            // Posar las muestras en la fase real sin avanzar logica de eleccion.
            float anterior = m.tiempoSubfase;
            for (float u : {0.0f,.5f,.65f,.85f,.95f})
            {
                m.tiempoSubfase = u*2.4f;
                DibujarPrueba(zona,cantidad == 4 && ronda == 4 && Cerca(u,.5f) ? "build/capsulas-brazo.png" : nullptr);
                ComprobarBrazo();
                ComprobarCapsulas(m,u < .8f ? 1 : 1-(u-.8f)/.2f);
                const auto& pinza = dibujos[MODELO_CAPSULAS_BRAZO_PINZA][0];
                const auto& nucleo = dibujos[MODELO_CAPSULAS_NUCLEO][0];
                if (u < .6f) Comprobar(Igual(nucleo.posicion,Vector3Add(pinza.posicion,{0,-.5f,0})), "Nucleo sigue mano antes de entrega");
                else Comprobar(Igual(nucleo.posicion,{XSlot(m.slotDe[m.premio],m.cantidadCapsulas),1.41f,0}), "Nucleo entregado bajo capsula premiada");
                Comprobar(Cerca(nucleo.escala.x,1+.12f*std::sin(m.tiempoAnimacion*8)), "Pulso original del nucleo");
            }
            m.tiempoSubfase = anterior;
            EsperarSubfase(zona,SUBFASE_CAPSULAS_BARAJAR);
            int esperaSwap = 0;
            while (m.swapA < 0 && m.subfase == SUBFASE_CAPSULAS_BARAJAR && esperaSwap++ < 1500) Avanzar(zona,1);
            Comprobar(m.swapA >= 0, "Intercambio real iniciado");
            if (m.swapA < 0) break;
            float anteriorSwap = m.swapTiempo;
            m.swapTiempo = (.62f-.08f*ronda)*.5f;
            DibujarPrueba(zona,cantidad == 4 && ronda == 4 ? "build/capsulas-barajado.png" : nullptr);
            const auto& a = dibujos[MODELO_CAPSULAS_CUERPO][m.swapA];
            const auto& b = dibujos[MODELO_CAPSULAS_CUERPO][m.swapB];
            Comprobar(Cerca(a.posicion.x,b.posicion.x) && Cerca(a.posicion.z,1.3f) && Cerca(b.posicion.z,-1.3f),
                "Arcos de barajado separados en Z a mitad del intercambio");
            ComprobarCapsulas(m,0);
            float tiempoBarajado = m.tiempoAnimacion;
            m.tiempoAnimacion = 0;
            DibujarPrueba(zona);
            Comprobar(ColorIsEqual(dibujos[MODELO_CAPSULAS_BALIZA][0].color,{255,150,30,255}), "Baliza naranja al barajar");
            m.tiempoAnimacion = .2f;
            DibujarPrueba(zona);
            Comprobar(ColorIsEqual(dibujos[MODELO_CAPSULAS_BALIZA][0].color,{70,70,80,255}), "Parpadeo de baliza");
            m.tiempoAnimacion = tiempoBarajado;
            m.swapTiempo = anteriorSwap;
            EsperarSubfase(zona,SUBFASE_CAPSULAS_ELEGIR);
            bool vistos[MAX_CAPSULAS]{};
            for (int c = 0; c < m.cantidadCapsulas; c++)
            {
                Comprobar(m.slotDe[c] >= 0 && m.slotDe[c] < m.cantidadCapsulas && !vistos[m.slotDe[c]], "Barajado conserva permutacion de slots");
                vistos[m.slotDe[c]] = true;
            }
            DibujarPrueba(zona,cantidad == 4 && ronda == 4 ? "build/capsulas-eleccion.png" : nullptr);
            ComprobarMarcadores(m,cantidad);
            Comprobar(ColorIsEqual(dibujos[MODELO_CAPSULAS_BALIZA][0].color,{80,255,140,255}), "Baliza verde al elegir");
            int puntosAntes = m.estados[0].puntos;
            // Estimulos deterministas para probar apuesta, error, timeout e IA.
            // La confirmacion y la asignacion de puntos pasan por Actualizar real.
            if (ronda != 2)
                for (int i = 1; i < cantidad; i++)
                {
                    m.estados[i].confirmado = true;
                    m.estados[i].slotElegido = ronda == 0 && i == 1 ? m.slotDe[m.premio] : (m.slotDe[m.premio]+1)%m.cantidadCapsulas;
                    m.estados[i].tiempoConfirmacion = 1.2f;
                }
            if (ronda <= 1)
            {
                int pasos = 0;
                while (m.estados[0].marcador != m.slotDe[m.premio] && pasos++ < 10)
                    Pulsar(zona,m.estados[0].marcador < m.slotDe[m.premio] ? KEY_D : KEY_A);
                Comprobar(m.estados[0].marcador == m.slotDe[m.premio], "Mover seleccion por entrada real de teclado");
                if (ronda == 1) Avanzar(zona,72);
                Pulsar(zona,KEY_SPACE);
            }
            else if (ronda == 3 || ronda == 4)
            {
                participantes[0].esBot = ronda == 3;
                if (ronda == 4)
                {
                    participantes[0].control = CONTROL_GAMEPAD;
                    participantes[0].indiceGamepad = -1;
                    participantes[0].conectado = false;
                }
                m.estados[0].botObjetivo = m.slotDe[m.premio];
                m.estados[0].botConfirmar = ronda == 3 ? .4f : 1.3f;
            }
            EsperarSubfase(zona,SUBFASE_CAPSULAS_REVELAR);
            int puntosRonda = ronda == 0 || ronda == 3 ? 2 : ronda == 2 ? 0 : 1;
            Comprobar(m.estados[0].puntos == puntosAntes+puntosRonda, "Puntos reales por rapidez, timeout e IA");
            if (ronda == 0) Comprobar(m.estados[1].puntosRonda == 1, "Acierto tardio vale uno");
            if (ronda == 2) Comprobar(!m.estados[0].confirmado && !m.estados[0].acerto, "Timeout sin confirmar");
            participantes[0].esBot = false;
            participantes[0].control = CONTROL_TECLADO_WASD;
            Avanzar(zona,15);
            DibujarPrueba(zona,cantidad == 4 && ronda == 4 ? "build/capsulas-revelado.png" : nullptr);
            ComprobarCapsulas(m,0);
            // En timeout no hay slot elegido: el marcador desaparece.
            if (ronda != 2) ComprobarMarcadores(m,cantidad);
            else Comprobar(cantidades[MODELO_CAPSULAS_MARCADOR] == 0, "Ocultar marcadores no confirmados al revelar");
            if (ronda != 2) Comprobar(zona.jugadores[0].posicion.y > 1.02f, "Conservar rebote del jugador que acierta");
            Avanzar(zona,145);
        }
        Comprobar(m.fase == FASE_CAPSULAS_TERMINADO && ResultadoMinijuegoFinalizado(m.resultado) &&
            m.resultado.participantes[0].posicionFinal == 1 && m.estados[0].puntos == 6, "Final de cinco rondas y ganador");
        DibujarPrueba(zona,cantidad == 4 ? "build/capsulas-final.png" : nullptr);
        // Desempate original por suma del tiempo de aciertos.
        m.fase = FASE_CAPSULAS_JUGANDO;
        m.ronda = 4; m.subfase = SUBFASE_CAPSULAS_REVELAR; m.tiempoSubfase = 2.61f;
        m.estados[0].puntos = m.estados[1].puntos = 10;
        m.estados[0].tiempoAciertos = 2; m.estados[1].tiempoAciertos = 1;
        Avanzar(zona,1);
        Comprobar(m.resultado.participantes[1].posicionFinal == 1 && m.resultado.participantes[0].posicionFinal == 2,
            "Desempate por tiempo conservado");
        Pulsar(zona,KEY_R);
        Comprobar(m.fase == FASE_CAPSULAS_PREPARACION && m.ronda == 0, "R reinicia el minijuego");
        Pulsar(zona,KEY_ESCAPE);
        Comprobar(zona.volverAlMenu, "Salida al menu/selector");
        zona.Inicializar(participantes,cantidad);
        zona.modoCatalogo = zona.modoTablero = true;
        zona.CambiarMinijuego(MINIJUEGO_CAPSULAS_BARAJADAS);
        Pulsar(zona,KEY_R);
        Comprobar(m.tiempoPreparacion < 3, "R no reinicia ronda oficial del tablero");
        for (int i = 0; i < CANTIDAD_MODELOS_CAPSULAS_BARAJADAS_3D; i++)
            Comprobar(cargas[i] == 1 && mallas[i] == recursos[i].modelo.meshes, "Rondas, reinicio y tablero sin cargas duplicadas");
    }
    // Fallo independiente de cada archivo, sin modificar los recursos del disco.
    auto& m = zona.gestorMinijuegos.minijuegoCapsulasBarajadas;
    m.fase = FASE_CAPSULAS_JUGANDO;
    m.subfase = SUBFASE_CAPSULAS_ELEGIR;
    DibujarPrueba(zona);
    int presentes[CANTIDAD_MODELOS_CAPSULAS_BARAJADAS_3D];
    std::memcpy(presentes,cantidades,sizeof(presentes));
    int cubosBase=cubos, esferasBase=esferasEx, cilindrosBase=cilindros, segmentosBase=segmentos;
    for (int ausente = 0; ausente < CANTIDAD_MODELOS_CAPSULAS_BARAJADAS_3D; ausente++)
    {
        DescargarSlotModeloEscenarioRetro3D(recursos[ausente]);
        rutaAusente = RUTAS_MODELOS_CAPSULAS_BARAJADAS_3D[ausente];
        for (int intento = 0; intento < 3; intento++)
            CargarPaqueteModelosEscenarioRetro3D(ObtenerPaqueteCapsulasBarajadasRetro3D());
        Comprobar(!recursos[ausente].cargado && recursos[ausente].cargaIntentada, "Fallo almacenado sin cargas por frame");
        DibujarPrueba(zona);
        for (int i = 0; i < CANTIDAD_MODELOS_CAPSULAS_BARAJADAS_3D; i++)
            Comprobar(cantidades[i] == (i == ausente ? 0 : presentes[i]), "Fallback solo de la pieza ausente");
        Comprobar(cubos > cubosBase || esferasEx > esferasBase || cilindros > cilindrosBase || segmentos > segmentosBase,
            "Conservar primitivas de la pieza fallida");
        rutaAusente = nullptr;
        recursos[ausente] = {};
        CargarPaqueteModelosEscenarioRetro3D(ObtenerPaqueteCapsulasBarajadasRetro3D());
        GuardarMateriales();
    }
    Comprobar(avisosAusente == 16, "Un diagnostico por recurso fallido");
    for (const auto& r : almacen.ultimoAsiento) Comprobar(!r.cargaIntentada, "No cargar Asiento");
    for (const auto& r : almacen.cajasPuerto) Comprobar(!r.cargaIntentada, "No cargar Puerto");
    for (const auto& r : almacen.laberintoJade) Comprobar(!r.cargaIntentada, "No cargar Jade");
    for (const auto& r : almacen.vetaCristal) Comprobar(!r.cargaIntentada, "No cargar Veta");
    zona.Descargar();
    zona.Descargar();
    for (int i = 0; i < CANTIDAD_MODELOS_CAPSULAS_BARAJADAS_3D; i++)
        Comprobar(!recursos[i].cargado && !recursos[i].cargaIntentada && !recursos[i].modelo.meshes && cargas[i] == descargas[i],
            "Descarga simetrica e idempotente antes de cerrar OpenGL");
    DescargarModeloJugadorCompartido();
    CloseWindow();
    std::printf("Verificacion Capsulas Barajadas 2/3/4 participantes: %d errores\n",errores);
    return errores == 0 ? 0 : 1;
}
