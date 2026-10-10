// Prueba de integracion: ZonaPruebas real, contexto OpenGL y entradas simuladas.
// Los wrappers observan el dibujo sin reemplazar el renderizador por un visor.
#include "Gameplay/ZonaPruebas.h"
#include "Minigames/ModelosEscenariosRetro3D.h"
#include "Minigames/ModeloJugadorCompartido.h"
#include "Minigames/SombrasRetro.h"
#include "rlgl.h"
#include <cmath>
#include <cstdio>
#include <cstring>
#include <cstdarg>
#include <initializer_list>

static constexpr int N = CANTIDAD_MODELOS_PAREJAS_GLACIAR_3D;
static int errores = 0, tecla = KEY_NULL, avisos = 0;
static int cargas[N]{}, descargas[N]{}, cantidades[N]{};
static int cubos = 0, esferas = 0, cilindros = 0, segmentos = 0, circulos = 0;
static int triangulosSombra = 0;
static int teclaMantenida = KEY_NULL, planos = 0, indicadores = 0;
static const char* rutaAusente = nullptr;
static Color originales[N][16]{};
static const Color colores[] = {{235,80,80,255},{80,140,240,255},
    {90,205,115,255},{245,205,70,255}};
struct DibujoGlaciar
{
    Vector3 posicion{}, eje{}, escala{};
    float angulo = 0;
    Color color{};
    int malla = -1;
};
static DibujoGlaciar dibujos[N][64]{};
static auto& Recursos() { return ObtenerModelosEscenariosRetro3D().parejasGlaciar; }
static void Comprobar(bool valor, const char* mensaje)
{
    if (!valor) { if (errores < 20) std::fprintf(stderr,"FALLO: %s\n",mensaje); errores++; }
}
static bool Cerca(float a, float b) { return std::fabs(a-b) < .002f; }
static bool Igual(Vector3 a, Vector3 b)
{ return Cerca(a.x,b.x) && Cerca(a.y,b.y) && Cerca(a.z,b.z); }
static int Indice(const char* ruta)
{
    for (int i=0;i<N;i++) if (std::strcmp(ruta,RUTAS_MODELOS_PAREJAS_GLACIAR_3D[i])==0) return i;
    return -1;
}
static void Registrar(int nivel, const char* formato, va_list args)
{
    char texto[2048]; std::vsnprintf(texto,sizeof(texto),formato,args);
    if (std::strstr(texto,"Modelo de escenario ausente")) avisos++;
    if (nivel >= LOG_WARNING) std::fprintf(stderr,"%s\n",texto);
}
extern "C" bool __real_IsKeyPressed(int);
extern "C" bool __wrap_IsKeyPressed(int k) { return k==tecla || __real_IsKeyPressed(k); }
extern "C" bool __real_IsKeyDown(int);
extern "C" bool __wrap_IsKeyDown(int k) { return k==teclaMantenida || __real_IsKeyDown(k); }
extern "C" bool __real_FileExists(const char*);
extern "C" bool __wrap_FileExists(const char* ruta)
{ return !(rutaAusente && std::strcmp(ruta,rutaAusente)==0) && __real_FileExists(ruta); }
extern "C" Model __real_LoadModel(const char*);
extern "C" Model __wrap_LoadModel(const char* ruta)
{ int i=Indice(ruta); if (i>=0) cargas[i]++; return __real_LoadModel(ruta); }
extern "C" void __real_UnloadModel(Model);
extern "C" void __wrap_UnloadModel(Model m)
{
    for (int i=0;i<N;i++) if (m.meshes && m.meshes==Recursos()[i].modelo.meshes) descargas[i]++;
    __real_UnloadModel(m);
}
extern "C" void __real_DrawModelEx(Model,Vector3,Vector3,float,Vector3,Color);
extern "C" void __wrap_DrawModelEx(Model m,Vector3 p,Vector3 eje,float angulo,Vector3 escala,Color tinte)
{
    for (int i=0;i<N;i++)
    {
        auto& r=Recursos()[i]; if (!r.cargado) continue;
        int malla=-1;
        for (int j=0;j<r.modelo.meshCount;j++) if (m.meshes==&r.modelo.meshes[j]) malla=j;
        if (malla<0) continue;
        int n=cantidades[i]++;
        int materialActual=m.meshMaterial[0];
        int materialRegistro=m.meshCount>1 && r.materialColor>=0 ? r.materialColor : materialActual;
        Color color=m.materials[materialRegistro].maps[MATERIAL_MAP_DIFFUSE].color;
        Comprobar(n<64,"Limite de instancias");
        if (n<64) dibujos[i][n]={p,eje,escala,angulo,color,malla};
        Comprobar(ColorIsEqual(tinte,WHITE),"Sin tinte global de materiales y vertices");
        Matrix identidad=MatrixIdentity();
        Comprobar(std::memcmp(&m.transform,&identidad,sizeof(Matrix))==0,"Pivotes originales sin centrado");
        int dinamico=(i>=MODELO_GLACIAR_AURORA_VERDE || i==MODELO_GLACIAR_FRAGMENTO) ? materialActual : r.materialColor;
        for (int j=0;j<m.materialCount && j<16;j++) if (j!=dinamico)
            Comprobar(ColorIsEqual(originales[i][j],m.materials[j].maps[MATERIAL_MAP_DIFFUSE].color),"Materiales estaticos intactos");
        break;
    }
    __real_DrawModelEx(m,p,eje,angulo,escala,tinte);
}
extern "C" void __real_DrawCube(Vector3,float,float,float,Color);
extern "C" void __wrap_DrawCube(Vector3 p,float x,float y,float z,Color c)
{ cubos++; __real_DrawCube(p,x,y,z,c); }
extern "C" void __real_DrawSphere(Vector3,float,Color);
extern "C" void __wrap_DrawSphere(Vector3 p,float r,Color c)
{ esferas++; __real_DrawSphere(p,r,c); }
extern "C" void __real_DrawCylinder(Vector3,float,float,float,int,Color);
extern "C" void __wrap_DrawCylinder(Vector3 p,float a,float b,float h,int l,Color c)
{ cilindros++; __real_DrawCylinder(p,a,b,h,l,c); }
extern "C" void __real_DrawCylinderEx(Vector3,Vector3,float,float,int,Color);
extern "C" void __wrap_DrawCylinderEx(Vector3 a,Vector3 b,float x,float y,int l,Color c)
{ segmentos++; __real_DrawCylinderEx(a,b,x,y,l,c); }
extern "C" void __real_DrawCircle3D(Vector3,float,Vector3,float,Color);
extern "C" void __wrap_DrawCircle3D(Vector3 p,float r,Vector3 eje,float a,Color c)
{ circulos++; __real_DrawCircle3D(p,r,eje,a,c); }
extern "C" void __real_DrawTriangle3D(Vector3,Vector3,Vector3,Color);
extern "C" void __wrap_DrawTriangle3D(Vector3 a,Vector3 b,Vector3 c,Color color)
{ triangulosSombra++; __real_DrawTriangle3D(a,b,c,color); }
extern "C" void __real_DrawPlane(Vector3,Vector2,Color);
extern "C" void __wrap_DrawPlane(Vector3 p,Vector2 s,Color c)
{ planos++; __real_DrawPlane(p,s,c); }
extern "C" void __real_DrawSphereWires(Vector3,float,int,int,Color);
extern "C" void __wrap_DrawSphereWires(Vector3 p,float r,int anillos,int cortes,Color c)
{ indicadores++; __real_DrawSphereWires(p,r,anillos,cortes,c); }
static void GuardarMateriales()
{
    for (int i=0;i<N;i++) for (int j=0;j<Recursos()[i].modelo.materialCount && j<16;j++)
        originales[i][j]=Recursos()[i].modelo.materials[j].maps[MATERIAL_MAP_DIFFUSE].color;
}
static void Dibujar(ZonaPruebas& zona,const char* captura=nullptr)
{
    std::memset(cantidades,0,sizeof(cantidades));
    cubos=esferas=cilindros=segmentos=circulos=triangulosSombra=planos=indicadores=0;
    auto& m=zona.gestorMinijuegos.minijuegoParejasGlaciar;
    unsigned char antes[sizeof(m)]; std::memcpy(antes,&m,sizeof(m));
    Vector3 posiciones[MAX_PARTICIPANTES], tamanos[MAX_PARTICIPANTES];
    for (int i=0;i<MAX_PARTICIPANTES;i++)
    { posiciones[i]=zona.jugadores[i].posicion; tamanos[i]=zona.jugadores[i].tamano; }
    BeginDrawing(); zona.Dibujar();
    if (captura)
    {
        rlDrawRenderBatchActive(); Image imagen=LoadImageFromScreen();
        Comprobar(ExportImage(imagen,captura),"Captura del juego integrado"); UnloadImage(imagen);
    }
    EndDrawing();
    Comprobar(std::memcmp(antes,&m,sizeof(m))==0,"El dibujo no cambia reglas, estado ni camara");
    for (int i=0;i<MAX_PARTICIPANTES;i++)
        Comprobar(Igual(posiciones[i],zona.jugadores[i].posicion) && Igual(tamanos[i],zona.jugadores[i].tamano),"Posiciones e hitboxes intactos");
    for (int i=0;i<N;i++) for (int j=0;j<Recursos()[i].modelo.materialCount && j<16;j++)
        Comprobar(ColorIsEqual(originales[i][j],Recursos()[i].modelo.materials[j].maps[MATERIAL_MAP_DIFFUSE].color),"Restaurar materiales despues del dibujo");
}
static void Avanzar(ZonaPruebas& zona,int frames)
{ for (int i=0;i<frames;i++) zona.Actualizar(1.0f/60); }
static void Pulsar(ZonaPruebas& zona,int k)
{ tecla=k; Avanzar(zona,1); tecla=KEY_NULL; }

static float X(int i) { return (i%4-1.5f)*1.9f; }
static float Z(int i) { return (i/4-1.5f)*1.9f; }
static void Elegir(ZonaPruebas& zona,int indice)
{
    auto& m=zona.gestorMinijuegos.minijuegoParejasGlaciar;
    m.cursor=indice; m.accionPrevia=false; Pulsar(zona,KEY_E);
}
static void PrepararTableroVisual(ZonaPruebas& zona)
{
    auto& m=zona.gestorMinijuegos.minijuegoParejasGlaciar;
    // Ocho identidades reales, estados de revelado, pareja y temblor juntos.
    for (int i=0;i<16;i++)
    {
        m.bloques[i]={}; m.bloques[i].simbolo=i/2;
        if (i%2==0) { m.bloques[i].revelado=true; m.bloques[i].derretido=1; }
    }
    m.bloques[14].emparejado=true; m.bloques[14].duenio=1;
    m.bloques[3].temblor=m.bloques[5].temblor=1;
    m.fase=FASE_GLACIAR_ELEGIR; m.turno=0; m.cursor=3; m.tiempoAnimacion=1.23f;
}
static void ComprobarModelos(ZonaPruebas& zona)
{
    const auto& m=zona.gestorMinijuegos.minijuegoParejasGlaciar;
    for (int i:{MODELO_GLACIAR_MAR,MODELO_GLACIAR_LAGO,MODELO_GLACIAR_TABLERO})
        Comprobar(cantidades[i]==1 && Igual(dibujos[i][0].posicion,{0,0,0}) && Igual(dibujos[i][0].escala,{1,1,1}),
            "Piezas modulares conservan origen y unidades");
    Comprobar(cantidades[MODELO_GLACIAR_TEMPANO]==zona.cantidadParticipantes,"Un modelo compartido para los tempanos");
    for (int i=0;i<zona.cantidadParticipantes;i++)
        Comprobar(Igual(dibujos[MODELO_GLACIAR_TEMPANO][i].posicion,{m.posicionTempanoX[i],0,m.posicionTempanoZ[i]}),
            "Tempano sigue las posiciones reales por participante");
    Comprobar(cantidades[MODELO_GLACIAR_PINGUINO]==3 && cantidades[MODELO_GLACIAR_FOCA]==1 &&
        cantidades[MODELO_GLACIAR_ICEBERG]==3 && cantidades[MODELO_GLACIAR_MONTANA]==6,"Fauna y paisaje sin duplicar cargas");
    for (int i=0;i<6;i++)
    {
        const auto& d=dibujos[MODELO_GLACIAR_MONTANA][i]; float h=9+(i*7)%5;
        Comprobar(Cerca(d.posicion.y-.5f*d.escala.y,-.5f) && Cerca(11*d.escala.y,h),
            "Montanas conservan pie y altura procedural");
    }
    int n=0;
    for (int i=MODELO_GLACIAR_SIMBOLO_0;i<=MODELO_GLACIAR_SIMBOLO_7;i++) n+=cantidades[i];
    int visibles=0;
    for (const auto& b:m.bloques) if(b.derretido>=.5f) visibles++;
    Comprobar(n==visibles,"No filtrar simbolos ocultos ni duplicar los revelados");
    Comprobar(cantidades[MODELO_GLACIAR_CURSOR]==(m.fase==FASE_GLACIAR_ELEGIR ? 1 : 0),
        "Cursor visible solo en eleccion");
}
int main()
{
    SetTraceLogCallback(Registrar); SetTraceLogLevel(LOG_WARNING);
    SetConfigFlags(FLAG_WINDOW_HIDDEN); InitWindow(1280,800,"Verificacion integrada Parejas Glaciares");
    if (!IsWindowReady()) return 2;
    SetExitKey(KEY_NULL); SetRandomSeed(975);
    static ZonaPruebas zona; Participante participantes[MAX_PARTICIPANTES]{}; Mesh* mallas[N]{};
    for (int cantidad=2;cantidad<=4;cantidad++)
    {
        for (int i=0;i<MAX_PARTICIPANTES;i++)
        {
            participantes[i]={}; participantes[i].activo=i<cantidad; participantes[i].conectado=true;
            participantes[i].control=CONTROL_TECLADO_WASD; participantes[i].numeroJugador=i+1; participantes[i].color=colores[i];
        }
        zona.Inicializar(participantes,cantidad); zona.modoCatalogo=true;
        if (cantidad==2)
        {
            for (const auto& r:Recursos()) Comprobar(!r.cargaIntentada,"No cargar Glaciar al arrancar");
            participantes[1].activo=false; zona.CambiarMinijuego(MINIJUEGO_PAREJAS_GLACIAR);
            Comprobar(zona.gestorMinijuegos.minijuegoParejasGlaciar.resultado.estado==RESULTADO_MINIJUEGO_CANCELADO,"Cancelar un solo participante");
            for (const auto& r:Recursos()) Comprobar(!r.cargaIntentada,"No cargar ronda invalida");
            participantes[1].activo=true;
        }
        zona.CambiarMinijuego(MINIJUEGO_PAREJAS_GLACIAR); auto& m=zona.gestorMinijuegos.minijuegoParejasGlaciar;
        for (int i=0;i<N;i++)
        {
            const auto& r=Recursos()[i]; Comprobar(r.cargado,RUTAS_MODELOS_PAREJAS_GLACIAR_3D[i]);
            if (!r.cargado) { zona.Descargar(); DescargarModeloJugadorCompartido(); CloseWindow(); return 2; }
            if (cantidad==2) mallas[i]=r.modelo.meshes;
            Comprobar(cargas[i]==1 && r.modelo.meshes==mallas[i],"Carga unica entre rondas y participantes");
            for (int j=0;j<r.modelo.meshCount;j++) Comprobar(r.modelo.meshes[j].colors!=nullptr,"Conservar colores de vertice");
        }
        GuardarMateriales(); Dibujar(zona,cantidad==4 ? "build/glaciar-preparacion.png" : nullptr); ComprobarModelos(zona);
        Comprobar(cantidades[MODELO_GLACIAR_BLOQUE_OCULTO]==16 && cantidades[MODELO_GLACIAR_BLOQUE_EMPAREJADO]==0,
            "Preparacion oculta los dieciseis bloques");
        Avanzar(zona,182); Comprobar(m.fase==FASE_GLACIAR_ELEGIR,"Preparacion termina a tres segundos");
        int cursorAntes=m.cursor; teclaMantenida=KEY_D; Avanzar(zona,1); teclaMantenida=KEY_NULL;
        Comprobar(m.cursor==cursorAntes+1,"Mover cursor con controles reales");
        m.turno=0;
        // Pareja especial con confirmaciones reales y animacion de derretido.
        int a=-1,b=-1;
        for(int i=0;i<16;i++) if(m.bloques[i].simbolo==7) { if(a<0) a=i; else b=i; }
        Elegir(zona,a); Avanzar(zona,8); Dibujar(zona);
        Comprobar(cantidades[MODELO_GLACIAR_SIMBOLO_7]==0,"No revelar antes del umbral de derretido");
        Avanzar(zona,4); Dibujar(zona);
        Comprobar(cantidades[MODELO_GLACIAR_SIMBOLO_7]==1,"Revelar al superar .5 sin esperar el segundo bloque");
        Elegir(zona,b); Avanzar(zona,20); Dibujar(zona,cantidad==4 ? "build/glaciar-pareja-aurora.png" : nullptr);
        Comprobar(m.aciertoActual && m.estadosJugadores[0].puntos==3 && m.estadosJugadores[0].parejas==1,
            "Pareja aurora conserva tres puntos y propietario");
        Comprobar(cantidades[MODELO_GLACIAR_BLOQUE_EMPAREJADO]==2,"Variante emparejada reemplaza solo esos bloques");
        Comprobar(ColorIsEqual(dibujos[MODELO_GLACIAR_BLOQUE_EMPAREJADO][0].color,participantes[0].color),
            "Color de propietario exclusivamente en COLOR_JUGADOR");
        Avanzar(zona,44); Comprobar(m.fase==FASE_GLACIAR_ELEGIR && m.turno==0,"Acertar repite turno");
        // Cuatro fallos activan el intercambio real, sin alterar emparejados.
        for (int fallo=0;fallo<4;fallo++)
        {
            a=b=-1;
            for(int i=0;i<16;i++) if(!m.bloques[i].emparejado) {a=i;break;}
            for(int i=0;i<16;i++) if(!m.bloques[i].emparejado && m.bloques[i].simbolo!=m.bloques[a].simbolo) {b=i;break;}
            Elegir(zona,a); Elegir(zona,b); Avanzar(zona,62);
            Comprobar(!m.bloques[a].revelado && !m.bloques[b].revelado,"Fallo oculta ambas identidades");
        }
        Comprobar(m.fase==FASE_GLACIAR_CRUJIDO,"Cuatro fallos inician crujido");
        a=m.crujidoA; b=m.crujidoB; int simboloA=m.bloques[a].simbolo,simboloB=m.bloques[b].simbolo;
        Avanzar(zona,2); Dibujar(zona,cantidad==4 ? "build/glaciar-crujido.png" : nullptr);
        Comprobar(cantidades[MODELO_GLACIAR_FRAGMENTO]==12,"Solo los dos bloques que tiemblan producen fragmentos");
        Avanzar(zona,43);
        Comprobar(m.crujidoIntercambiado && m.bloques[a].simbolo==simboloB && m.bloques[b].simbolo==simboloA,
            "Intercambio y tiempo del crujido reales");
        Avanzar(zona,40); Comprobar(m.fase==FASE_GLACIAR_ELEGIR,"Crujido retorna a eleccion");
        Dibujar(zona); Comprobar(cantidades[MODELO_GLACIAR_FRAGMENTO]==0,"No dejar fragmentos al terminar crujido");
        Pulsar(zona,KEY_R); Avanzar(zona,182); m.tiempoTurno=.001f; Avanzar(zona,1);
        Comprobar(m.primera>=0,"Tiempo de turno revela un bloque automaticamente");
        m.tiempoRestante=.001f; Avanzar(zona,1);
        Comprobar(m.fase==FASE_GLACIAR_TERMINADO && m.resultado.desenlace==DESENLACE_EMPATE,"Final por tiempo y empate");
        Pulsar(zona,KEY_R); Avanzar(zona,182); m.estadosJugadores[0].puntos=3; m.tiempoRestante=.001f; Avanzar(zona,1);
        Comprobar(m.resultado.desenlace==DESENLACE_CON_GANADOR && m.resultado.participantes[0].posicionFinal==1,
            "Resultado conserva ganador por puntuacion");
        Dibujar(zona,cantidad==4 ? "build/glaciar-final.png" : nullptr);
        Pulsar(zona,KEY_R);
        for (int i=0;i<cantidad;i++) participantes[i].esBot=true;
        Avanzar(zona,6000); Comprobar(m.fase==FASE_GLACIAR_TERMINADO,"Ronda completa de IA finaliza");
        for (int i=0;i<cantidad;i++) participantes[i].esBot=false;
        Pulsar(zona,KEY_ESCAPE); Comprobar(zona.volverAlMenu,"Salir al menu");
        zona.Inicializar(participantes,cantidad); zona.modoCatalogo=zona.modoTablero=true;
        zona.CambiarMinijuego(MINIJUEGO_PAREJAS_GLACIAR); Pulsar(zona,KEY_R); Pulsar(zona,KEY_ESCAPE);
        Comprobar(!zona.volverAlMenu && m.tiempoPreparacion<3,"Tablero bloquea reinicio y abandono");
        for (int i=0;i<N;i++) Comprobar(cargas[i]==1 && Recursos()[i].modelo.meshes==mallas[i],"Reinicio y reentrada conservan recursos");
        PrepararTableroVisual(zona); Dibujar(zona,cantidad==4 ? "build/glaciar-simbolos.png" : nullptr); ComprobarModelos(zona);
        Comprobar(cubos==0 && esferas==1 && segmentos==0 && planos==0,"Solo halo procedural: ninguna primitiva reemplazada superpuesta");
        Comprobar(triangulosSombra==8*cantidad,"Sin sombras humanas para GLB ni sombras en suelo incorrecto");
        for(int s=0;s<8;s++)
        {
            int modelo=MODELO_GLACIAR_SIMBOLO_0+s;
            Comprobar(cantidades[modelo]==1 && Igual(dibujos[modelo][0].posicion,{X(s*2),.23f,Z(s*2)}),
                "Identidad y posicion del simbolo dependen de bloques[]");
        }
    }
    // Simular archivos ausentes sin modificar ni mover los recursos reales.
    PrepararTableroVisual(zona);
    for (int ausente=0;ausente<N;ausente++)
    {
        Dibujar(zona); int presentes[N]; std::memcpy(presentes,cantidades,sizeof(presentes));
        int base=cubos+esferas+cilindros+segmentos+planos;
        Comprobar(presentes[ausente]>0,"Los veinticuatro modelos tienen uso");
        DescargarSlotModeloEscenarioRetro3D(Recursos()[ausente]); rutaAusente=RUTAS_MODELOS_PAREJAS_GLACIAR_3D[ausente];
        for(int k=0;k<3;k++) CargarPaqueteParejasGlaciarRetro3D();
        Dibujar(zona);
        for (int i=0;i<N;i++) Comprobar(cantidades[i]==(i==ausente ? 0 : presentes[i]),"Fallback por pieza conserva los otros GLB");
        // El cursor usa wires; no pertenece a los contadores de primitivas solidas.
        if(ausente!=MODELO_GLACIAR_CURSOR)
            Comprobar(cubos+esferas+cilindros+segmentos+planos>base,"Fallback visible por pieza");
        rutaAusente=nullptr; Recursos()[ausente]={}; CargarPaqueteParejasGlaciarRetro3D(); GuardarMateriales();
    }
    Comprobar(avisos==N,"Diagnostico de carga una sola vez por recurso");
    const auto& almacen=ObtenerModelosEscenariosRetro3D();
    for (const auto& r:almacen.voleaMagma) Comprobar(!r.cargaIntentada,"No cargar Volea");
    for (const auto& r:almacen.ultimoAsiento) Comprobar(!r.cargaIntentada,"No cargar Asiento");
    for (const auto& r:almacen.cajasPuerto) Comprobar(!r.cargaIntentada,"No cargar Puerto");
    for (const auto& r:almacen.laberintoJade) Comprobar(!r.cargaIntentada,"No cargar Jade");
    for (const auto& r:almacen.vetaCristal) Comprobar(!r.cargaIntentada,"No cargar Veta");
    for (const auto& r:almacen.capsulasBarajadas) Comprobar(!r.cargaIntentada,"No cargar Capsulas");
    for (const auto& r:almacen.bateoMeteorico) Comprobar(!r.cargaIntentada,"No cargar Bateo");
    for (const auto& r:almacen.racimoToxico) Comprobar(!r.cargaIntentada,"No cargar Racimo");
    for (const auto& r:almacen.tesoreroCercado) Comprobar(!r.cargaIntentada,"No cargar Tesorero");
    for (const auto& r:almacen.descensoNubes) Comprobar(!r.cargaIntentada,"No cargar Nubes");
    zona.Descargar(); zona.Descargar();
    for (int i=0;i<N;i++) Comprobar(cargas[i]==descargas[i] && !Recursos()[i].modelo.meshes,"Descarga simetrica antes de CloseWindow");
    DescargarModeloJugadorCompartido(); CloseWindow();
    std::printf("Verificacion Parejas Glaciares 2/3/4 participantes: %d errores\n",errores);
    return errores==0 ? 0 : 1;
}
