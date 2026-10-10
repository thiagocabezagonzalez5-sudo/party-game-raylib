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

static constexpr int N = CANTIDAD_MODELOS_VOLEA_MAGMA_3D;
static int errores = 0, tecla = KEY_NULL, avisos = 0;
static int cargas[N]{}, descargas[N]{}, cantidades[N]{};
static int cubos = 0, esferas = 0, cilindros = 0, segmentos = 0, circulos = 0;
static int triangulosSombra = 0;
static int teclaMantenida = KEY_NULL, planos = 0, indicadores = 0;
static const char* rutaAusente = nullptr;
static Color originales[N][16]{};
static const Color colores[] = {{235,80,80,255},{80,140,240,255},
    {90,205,115,255},{245,205,70,255}};
struct DibujoVolea
{
    Vector3 posicion{}, eje{}, escala{};
    float angulo = 0;
    Color color{};
    int malla = -1;
};
static DibujoVolea dibujos[N][64]{};
static auto& Recursos() { return ObtenerModelosEscenariosRetro3D().voleaMagma; }
static void Comprobar(bool valor, const char* mensaje)
{
    if (!valor) { if (errores < 20) std::fprintf(stderr,"FALLO: %s\n",mensaje); errores++; }
}
static bool Cerca(float a, float b) { return std::fabs(a-b) < .002f; }
static bool Igual(Vector3 a, Vector3 b)
{ return Cerca(a.x,b.x) && Cerca(a.y,b.y) && Cerca(a.z,b.z); }
static int Indice(const char* ruta)
{
    for (int i=0;i<N;i++) if (std::strcmp(ruta,RUTAS_MODELOS_VOLEA_MAGMA_3D[i])==0) return i;
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
        int materialActual=m.meshMaterial[0]; Color color=m.materials[materialActual].maps[MATERIAL_MAP_DIFFUSE].color;
        Comprobar(n<64,"Limite de instancias");
        if (n<64) dibujos[i][n]={p,eje,escala,angulo,color,malla};
        Comprobar(ColorIsEqual(tinte,WHITE),"Sin tinte global de materiales y vertices");
        Matrix identidad=MatrixIdentity();
        Comprobar(std::memcmp(&m.transform,&identidad,sizeof(Matrix))==0,"Pivotes originales sin centrado");
        int dinamico=m.meshCount>1 ? r.materialColor : ((i==MODELO_VOLEA_CHARCO || i==MODELO_VOLEA_ESTELA || i==MODELO_VOLEA_CENIZA) ? materialActual : -1);
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
    auto& m=zona.gestorMinijuegos.minijuegoVoleaMagma;
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

static float Hash(int n)
{ float f=std::sin(n*12.9898f)*43758.5453f; return f-std::floor(f); }
static Color ColorRocaPrueba(float temperatura,float t)
{
    if (temperatura>=.85f)
    { float p=.5f+.5f*std::sin(t*10); return {255,(unsigned char)(200+40*p),(unsigned char)(70+60*p),255}; }
    float k=Clamp(temperatura/.85f,0,1);
    return {(unsigned char)(70+185*k),(unsigned char)(58+62*k),(unsigned char)(58-38*k),255};
}
static void ComprobarEscena(ZonaPruebas& zona)
{
    const auto& m=zona.gestorMinijuegos.minijuegoVoleaMagma; float t=m.tiempoAnimacion;
    for (int i:{MODELO_VOLEA_LAGO,MODELO_VOLEA_CANCHA,MODELO_VOLEA_BORDE,MODELO_VOLEA_RED})
        Comprobar(cantidades[i]==1 && Igual(dibujos[i][0].posicion,{0,0,0}) && Igual(dibujos[i][0].escala,{1,1,1}),"Escenario modular con origen comun, sin normalizar");
    Comprobar(cantidades[MODELO_VOLEA_POSTE]==2 && Igual(dibujos[MODELO_VOLEA_POSTE][0].posicion,{0,0,-5.4f}) &&
        Igual(dibujos[MODELO_VOLEA_POSTE][1].posicion,{0,0,5.4f}),"Postes en extremos originales de red X=0");
    Color cadena={255,(unsigned char)(110+60*(.5f+.5f*std::sin(t*4))),40,255};
    Comprobar(ColorIsEqual(dibujos[MODELO_VOLEA_RED][0].color,cadena),"Pulso de cadena conserva animacion");
    for (int i:{MODELO_VOLEA_VOLCAN_MENOR,MODELO_VOLEA_VOLCAN_MAYOR})
        Comprobar(cantidades[i]==1 && Igual(dibujos[i][0].posicion,i==MODELO_VOLEA_VOLCAN_MENOR ? Vector3{-28,-1.5f,-38} : Vector3{26,-1.5f,-44}),"Volcanes conservan ubicacion y escala documentada");
    Comprobar(cantidades[MODELO_VOLEA_COLUMNA]==10 && cantidades[MODELO_VOLEA_COLUMNA_LLAMA]==4,"Mallas compartidas por catorce columnas");
    for (int i=0;i<10;i++)
    {
        const auto& d=dibujos[MODELO_VOLEA_COLUMNA][i]; float h=2.5f+4.5f*Hash(i*3+7);
        Comprobar(Igual(d.posicion,{-15.f+3.4f*i,-1.5f,-9.f-Hash(i+40)*2}) && Igual(d.escala,{1,h/4,1}),"Columna escala exclusivamente Y desde su pie");
    }
    for (int i=0;i<4;i++)
    {
        const auto& d=dibujos[MODELO_VOLEA_COLUMNA_LLAMA][i]; float h=2+2.5f*Hash(i+90);
        Comprobar(Igual(d.posicion,{i%2 ? 12.2f : -12.2f,-1.5f,i<2 ? -3.5f : 3.5f}) && Igual(d.escala,{1,h/4,1}),"Columnas con llama conservan alturas y posiciones");
    }
    int burbujas=0;
    for (int i=0;i<16;i++)
    {
        float x=-26+52*Hash(i*5+1),z=-26+36*Hash(i*5+2);
        if (std::fabs(x)<11 && std::fabs(z)<7) continue;
        float fase=std::sin(t*(.8f+Hash(i)*1.2f)+i*1.7f),s=(.5f+.15f*fase)/.48f;
        const auto& d=dibujos[MODELO_VOLEA_BURBUJA][burbujas++];
        Comprobar(Igual(d.posicion,{x,-1.45f+.18f*fase,z}) && Igual(d.escala,{s,s,s}),"Burbujas animadas fuera de cancha");
        Comprobar(ColorIsEqual(d.color,{255,(unsigned char)(150+60*fase),40,255}),"Pulso solo en lava_clara de burbuja");
    }
    Comprobar(cantidades[MODELO_VOLEA_BURBUJA]==burbujas && cantidades[MODELO_VOLEA_CENIZA]==40,"Decoracion compartida sin cargas por instancia");
    for (int i=0;i<40;i++)
    {
        float v=.5f+.7f*Hash(i*3+1),f=std::fmod(t*v*.35f+Hash(i*3+2),1.f);
        const auto& d=dibujos[MODELO_VOLEA_CENIZA][i];
        Comprobar(Igual(d.posicion,{-16+32*Hash(i*3+3)+1.5f*std::sin(t+i),14-15*f,-12+22*Hash(i*7+5)}) && d.color.a==200,"Ceniza conserva trayectoria y opacidad");
    }
    bool caliente=m.pelota.temperatura>=.85f; int pieza=caliente ? MODELO_VOLEA_ROCA_CALIENTE : MODELO_VOLEA_ROCA;
    Comprobar(cantidades[pieza]==1 && cantidades[caliente ? MODELO_VOLEA_ROCA : MODELO_VOLEA_ROCA_CALIENTE]==0,"Variante de roca segun temperatura");
    Comprobar(Igual(dibujos[pieza][0].posicion,m.pelota.posicion) && Igual(dibujos[pieza][0].escala,{1,1,1}) &&
        ColorIsEqual(dibujos[pieza][0].color,ColorRocaPrueba(m.pelota.temperatura,t)),"Pelota conserva pivote central, radio .45 y temperatura");
    float h=Clamp(m.pelota.posicion.y,0,9),radio=std::fmax(.3f,.45f*(1.25f-.08f*h)),s=radio/.48f;
    Comprobar(cantidades[MODELO_VOLEA_SOMBRA]==1 && Igual(dibujos[MODELO_VOLEA_SOMBRA][0].posicion,{m.pelota.posicion.x,.03f,m.pelota.posicion.z}) &&
        Igual(dibujos[MODELO_VOLEA_SOMBRA][0].escala,{s,1,s}),"Sombra unica de pelota escala en XZ con altura");
    BoundingBox disco=GetMeshBoundingBox(Recursos()[MODELO_VOLEA_SOMBRA].modelo.meshes[0]);
    Comprobar(disco.min.y+dibujos[MODELO_VOLEA_SOMBRA][0].posicion.y>.031f,"Disco de sombra por encima de las losas GLB");
    float discriminante=m.pelota.velocidad.y*m.pelota.velocidad.y+28*(m.pelota.posicion.y-.45f);
    float vuelo=m.pelota.estado==PELOTA_VOLEA_EN_JUEGO && discriminante>0 ? std::fmax(0.f,(m.pelota.velocidad.y+std::sqrt(discriminante))/14) : 0;
    float x=m.pelota.posicion.x+m.pelota.velocidad.x*vuelo,z=m.pelota.posicion.z+m.pelota.velocidad.z*vuelo;
    bool visible=m.pelota.estado==PELOTA_VOLEA_EN_JUEGO && std::fabs(x)<12 && std::fabs(z)<8;
    Comprobar(cantidades[MODELO_VOLEA_INDICADOR]==(visible ? 3 : 0),"Indicador solo para vuelo con prediccion visible");
    if (visible)
    {
        float escala=(.55f+.1f*(.5f+.5f*std::sin(t*9)))/.6f;
        for (int j=0;j<3;j++)
            Comprobar(Igual(dibujos[MODELO_VOLEA_INDICADOR][j].posicion,{x,0,z}) && Igual(dibujos[MODELO_VOLEA_INDICADOR][j].escala,j==1 ? Vector3{1,1,1} : Vector3{escala,1,escala}),"Prediccion balistica real y aro interior fijo");
    }
    Comprobar(cantidades[MODELO_VOLEA_ESTELA]==14,"Siete ascuas con dos mallas compartidas");
    for (int k=7;k>=1;k--) for (int j=0;j<2;j++)
    {
        float f=1-k/8.f,s=.45f*(.25f+.55f*f)/.18f; const auto& d=dibujos[MODELO_VOLEA_ESTELA][(7-k)*2+j];
        Comprobar(Igual(d.posicion,m.pelota.estela[k]) && Igual(d.escala,{s,s,s}) && d.color.a==Fade(WHITE,.5f*f).a,"Estela usa historial, radio y desvanecimiento originales");
    }
    int charcos=0;
    for (int i=0;i<MAX_CHARCOS_VOLEA;i++)
    {
        const auto& c=m.charcos[i]; if (!c.activo) continue;
        float alfa=std::fmin(1.f,4-c.tiempo);
        for (int j=0;j<4;j++)
        {
            const auto& d=dibujos[MODELO_VOLEA_CHARCO][charcos*4+j];
            Comprobar(Igual(d.posicion,{c.x,0,c.z}) && Igual(d.escala,{1,1,1}) && d.color.a==Fade(WHITE,alfa).a,"Charco activo conserva radio 1.8 y desvanece todas sus mallas");
        }
        charcos++;
    }
    Comprobar(cantidades[MODELO_VOLEA_CHARCO]==charcos*4,"Charcos visibles exclusivamente si activos");
    int golpes=0,enSuelo=0,particulas=0,sombrasParticulas=0;
    for (int i=0;i<zona.cantidadParticipantes;i++)
    {
        const auto& j=zona.jugadores[i];
        if (j.golpeando && !j.aplastado) golpes++;
        if (j.enSuelo && !j.cayendo) enSuelo++;
    }
    for (const auto& p:m.particulas)
    {
        if (!p.activa) continue;
        particulas++;
        float vida=p.vidaMaxima>0 ? Clamp(p.vida/p.vidaMaxima,0,1) : 0;
        float tamano=p.tamano*(.35f+.65f*vida);
        if (tamano>=.2f*EscalaUmbralSombraPropRetro()) sombrasParticulas+=2;
    }
    Comprobar(cubos==2+particulas && esferas==12+(caliente ? 1 : 0)+golpes && cilindros==enSuelo && segmentos==0,
        "Conservar marcas, humo, halo, punos, particulas y sombra cilindrica del jugador sin superponer geometria del escenario");
    int sombrasGolpe=.18f>=.1f*EscalaUmbralSombraPropRetro() ? golpes*LadosSombraPropRetro() : 0;
    Comprobar(triangulosSombra==8*zona.cantidadParticipantes+sombrasGolpe+sombrasParticulas,
        "Sombras de personajes y sus efectos intactas, sin manchas del escenario");
}
static void PrepararGolpe(ZonaPruebas& zona,int indice,bool remate=false)
{
    auto& m=zona.gestorMinijuegos.minijuegoVoleaMagma; int equipo=m.estadosJugadores[indice].equipo;
    for (int i=0;i<zona.cantidadParticipantes;i++)
    {
        auto& j=zona.jugadores[i]; j.posicion={m.estadosJugadores[i].equipo==0 ? -8.f : 8.f,.7f,i%2 ? 4.f : -4.f};
        j.velocidad=j.empuje={}; j.enSuelo=true; m.estadosJugadores[i].recargaGolpe=0;
    }
    auto& j=zona.jugadores[indice]; j.posicion={equipo==0 ? -3.5f : 3.5f,remate ? 1.8f : .7f,0}; j.enSuelo=!remate;
    m.pelota.posicion={j.posicion.x,remate ? 2.7f : 2.4f,0}; m.pelota.velocidad={}; m.pelota.enfriamientoToque=0;
    m.pelota.estado=remate ? PELOTA_VOLEA_EN_JUEGO : PELOTA_VOLEA_SAQUE; m.equipoSaque=equipo;
    m.toques[0]=m.toques[1]=0; m.equipoUltimoToque=m.ultimoJugador=-1;
}
static void ForzarCaida(ZonaPruebas& zona,float temperatura=.6f)
{
    auto& m=zona.gestorMinijuegos.minijuegoVoleaMagma;
    m.pelota.estado=PELOTA_VOLEA_EN_JUEGO; m.pelota.posicion={3,.46f,1}; m.pelota.velocidad={0,-1,0};
    m.pelota.temperatura=temperatura; m.pelota.enfriamientoToque=0;
    Avanzar(zona,1);
}
int main()
{
    SetTraceLogCallback(Registrar); SetTraceLogLevel(LOG_WARNING);
    SetConfigFlags(FLAG_WINDOW_HIDDEN); InitWindow(1280,800,"Verificacion integrada Volea de Magma");
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
            for (const auto& r:Recursos()) Comprobar(!r.cargaIntentada,"No cargar Volea al arrancar");
            participantes[1].activo=false; zona.CambiarMinijuego(MINIJUEGO_VOLEA_MAGMA);
            Comprobar(zona.gestorMinijuegos.minijuegoVoleaMagma.resultado.estado==RESULTADO_MINIJUEGO_CANCELADO,"Cancelar un solo participante");
            for (const auto& r:Recursos()) Comprobar(!r.cargaIntentada,"No cargar ronda invalida");
            participantes[1].activo=true;
        }
        zona.CambiarMinijuego(MINIJUEGO_VOLEA_MAGMA); auto& m=zona.gestorMinijuegos.minijuegoVoleaMagma;
        for (int i=0;i<N;i++)
        {
            const auto& r=Recursos()[i]; Comprobar(r.cargado,RUTAS_MODELOS_VOLEA_MAGMA_3D[i]);
            if (!r.cargado) { zona.Descargar(); DescargarModeloJugadorCompartido(); CloseWindow(); return 2; }
            if (cantidad==2) mallas[i]=r.modelo.meshes;
            Comprobar(cargas[i]==1 && r.modelo.meshes==mallas[i],"Carga unica, compartida por todas las instancias");
            for (int j=0;j<r.modelo.meshCount;j++) Comprobar(r.modelo.meshes[j].colors!=nullptr,"Colores de vertices originales");
        }
        Comprobar(m.cantidadJugadoresEquipo[0]==(cantidad>=3 ? 2 : 1) && m.cantidadJugadoresEquipo[1]==cantidad-m.cantidadJugadoresEquipo[0],"Equipos 1vs1, 2vs1 y 2vs2 conservados");
        GuardarMateriales(); Dibujar(zona,cantidad==4 ? "build/volea-preparacion.png" : nullptr); ComprobarEscena(zona);
        Avanzar(zona,182); Comprobar(m.fase==FASE_VOLEA_JUGANDO,"Cuenta regresiva y fase jugando reales");
        int sacador=0; while (m.estadosJugadores[sacador].equipo!=m.equipoSaque) sacador++;
        PrepararGolpe(zona,sacador); int equipo=m.estadosJugadores[sacador].equipo;
        Pulsar(zona,KEY_E);
        Comprobar(m.pelota.estado==PELOTA_VOLEA_EN_JUEGO && m.toques[equipo]==1 && m.estadosJugadores[sacador].tiempoSwing>0,"Saque con golpe real");
        Comprobar(m.pelota.velocidad.x*(equipo==0 ? 1 : -1)>0,"Golpe dirige la roca a mitad rival");
        Dibujar(zona,cantidad==4 ? "build/volea-saque.png" : nullptr); ComprobarEscena(zona);
        PrepararGolpe(zona,sacador,true); Pulsar(zona,KEY_E);
        Comprobar(m.toques[equipo]==1 && !zona.jugadores[sacador].enSuelo,"Salto y golpe producen remate");
        Dibujar(zona); ComprobarEscena(zona);
        PrepararGolpe(zona,sacador); m.toques[equipo]=2; Pulsar(zona,KEY_E);
        Comprobar(m.toques[equipo]==2 && m.avisoToques>0,"Limite de dos toques conservado");
        if (m.cantidadJugadoresEquipo[equipo]>1)
        {
            PrepararGolpe(zona,sacador); m.toques[equipo]=1; m.equipoUltimoToque=equipo; m.ultimoJugador=sacador;
            Pulsar(zona,KEY_E); Comprobar(m.toques[equipo]==1 && m.avisoToques>0,"Companeros no permiten dos toques consecutivos del mismo jugador");
        }
        m.pelota.estado=PELOTA_VOLEA_EN_JUEGO; m.pelota.posicion={-.01f,1.5f,0}; m.pelota.velocidad={3,0,0};
        Avanzar(zona,1); Comprobar(m.pelota.velocidad.x<0,"Rebote bajo la red mantiene colision logica");
        m.pelota.posicion={-.01f,4,0}; m.pelota.velocidad={3,0,0}; m.toques[1]=2;
        Avanzar(zona,1); Comprobar(m.toques[1]==0,"Cruce alto reinicia toques rivales");
        m.pelota.velocidad={20,2,0}; m.pelota.posicion={8,4,0}; Dibujar(zona); ComprobarEscena(zona);
        Comprobar(cantidades[MODELO_VOLEA_INDICADOR]==0,"Ocultar prediccion fuera de limites visuales");
        Pulsar(zona,KEY_R); Avanzar(zona,182); int puntosAntes=m.puntos[0]; ForzarCaida(zona,.9f);
        Comprobar(m.puntos[0]==puntosAntes+1 && m.pelota.estado==PELOTA_VOLEA_PUNTO && m.charcos[0].activo,"Caida sobrecalentada anota y crea charco real");
        Dibujar(zona,cantidad==4 ? "build/volea-sobrecalentada.png" : nullptr); ComprobarEscena(zona);
        int pisador=0; while (m.estadosJugadores[pisador].equipo!=1) pisador++;
        zona.jugadores[pisador].posicion={m.charcos[0].x,.7f,m.charcos[0].z}; zona.jugadores[pisador].enSuelo=true;
        Avanzar(zona,1); Comprobar(zona.jugadores[pisador].tiempoRalentizado>0 && Cerca(zona.jugadores[pisador].multiplicadorRalentizacion,.45f),"Charco ralentiza con hitbox original");
        Avanzar(zona,104); Comprobar(m.pelota.estado==PELOTA_VOLEA_SAQUE && m.equipoSaque==0,"Pausa de punto y nuevo saque");
        m.charcos[0].tiempo=3.5f; Dibujar(zona,cantidad==4 ? "build/volea-charco-expira.png" : nullptr); ComprobarEscena(zona);
        Avanzar(zona,32); Comprobar(!m.charcos[0].activo,"Charco expira a cuatro segundos");
        m.puntosTotales=2; ForzarCaida(zona,.9f);
        Comprobar(m.puntosTotales==3 && m.pelota.temperatura<.85f && m.charcos[0].activo,"Cada tres puntos enfria despues de crear charco");
        Dibujar(zona); ComprobarEscena(zona); Avanzar(zona,104);
        Comprobar(m.mensaje==MENSAJE_VOLEA_ENFRIA,"Mensaje de enfriamiento al nuevo saque");
        for (float temperatura:{0.f,.6f,.849f,.85f,1.f})
        {
            m.pelota.temperatura=temperatura; m.tiempoAnimacion=1.23f; Dibujar(zona); ComprobarEscena(zona);
        }
        Pulsar(zona,KEY_R); Avanzar(zona,182); m.puntos[0]=6; ForzarCaida(zona);
        Comprobar(m.fase==FASE_VOLEA_TERMINADO && m.equipoGanador==0 && m.puntos[0]==7,"Final real a siete puntos");
        Dibujar(zona,cantidad==4 ? "build/volea-final.png" : nullptr); ComprobarEscena(zona);
        Pulsar(zona,KEY_R); Avanzar(zona,182); m.puntos[0]=2; m.puntos[1]=1; m.tiempoRestante=.001f; Avanzar(zona,1);
        Comprobar(m.fase==FASE_VOLEA_TERMINADO && m.equipoGanador==0,"Victoria por tiempo con ventaja");
        Pulsar(zona,KEY_R); Avanzar(zona,182); m.tiempoRestante=.001f; Avanzar(zona,1);
        Comprobar(m.modoOro && m.fase==FASE_VOLEA_JUGANDO && Cerca(m.tiempoRestante,20),"Empate abre punto de oro de veinte segundos");
        Dibujar(zona,cantidad==4 ? "build/volea-oro.png" : nullptr); ComprobarEscena(zona);
        ForzarCaida(zona); Comprobar(m.fase==FASE_VOLEA_TERMINADO && m.equipoGanador==0,"Primer punto de oro termina la ronda");
        Pulsar(zona,KEY_R); Avanzar(zona,182); m.tiempoRestante=.001f; Avanzar(zona,1);
        m.tiempoRestante=.001f; Avanzar(zona,1); Comprobar(m.fase==FASE_VOLEA_TERMINADO && m.empate,"Punto de oro agotado mantiene empate");
        Pulsar(zona,KEY_R);
        Comprobar(m.fase==FASE_VOLEA_PREPARACION && m.puntos[0]==0 && !m.charcos[0].activo && !m.modoOro,"Reinicio limpia fases, charcos y puntos");
        for (int i=0;i<cantidad;i++) participantes[i].esBot=true;
        Avanzar(zona,6000); Comprobar(m.fase==FASE_VOLEA_TERMINADO && m.resultado.estado==RESULTADO_MINIJUEGO_FINALIZADO,"Ronda completa de IA con resultado");
        for (int i=0;i<cantidad;i++) participantes[i].esBot=false;
        Pulsar(zona,KEY_ESCAPE); Comprobar(zona.volverAlMenu,"Regreso al catalogo/menu");
        zona.Inicializar(participantes,cantidad); zona.modoCatalogo=zona.modoTablero=true;
        zona.CambiarMinijuego(MINIJUEGO_VOLEA_MAGMA); Pulsar(zona,KEY_R); Pulsar(zona,KEY_ESCAPE);
        Comprobar(!zona.volverAlMenu && m.tiempoPreparacion<3,"Modo tablero bloquea reinicio y abandono");
        for (int i=0;i<N;i++) Comprobar(cargas[i]==1 && Recursos()[i].modelo.meshes==mallas[i],"Reinicio y reentrada sin duplicar mallas");
    }
    auto& m=zona.gestorMinijuegos.minijuegoVoleaMagma;
    m.pelota.estado=PELOTA_VOLEA_EN_JUEGO; m.pelota.posicion={-2,3,0}; m.pelota.velocidad={3,1,0};
    m.charcos[0]={true,3,1,3.5f}; m.tiempoAnimacion=1.2f;
    for (int ausente=0;ausente<N;ausente++)
    {
        m.pelota.temperatura=ausente==MODELO_VOLEA_ROCA_CALIENTE ? .9f : .6f; Dibujar(zona); ComprobarEscena(zona);
        int presentes[N]; std::memcpy(presentes,cantidades,sizeof(presentes)); int base=cubos+esferas+cilindros+segmentos+circulos;
        Comprobar(presentes[ausente]>0,"Los diecisiete GLB tienen uso en el juego");
        DescargarSlotModeloEscenarioRetro3D(Recursos()[ausente]); rutaAusente=RUTAS_MODELOS_VOLEA_MAGMA_3D[ausente];
        for (int k=0;k<3;k++) CargarPaqueteVoleaMagmaRetro3D();
        Dibujar(zona);
        for (int i=0;i<N;i++) Comprobar(cantidades[i]==(i==ausente ? 0 : presentes[i]),"Fallback individual no duplica otros modelos");
        Comprobar(cubos+esferas+cilindros+segmentos+circulos>base,"Fallback procedural visible para cada pieza");
        rutaAusente=nullptr; Recursos()[ausente]={}; CargarPaqueteVoleaMagmaRetro3D(); GuardarMateriales();
    }
    Comprobar(avisos==N,"Solo un diagnostico por recurso ausente");
    const auto& almacen=ObtenerModelosEscenariosRetro3D();
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
    std::printf("Verificacion Volea de Magma 2/3/4 participantes: %d errores\n",errores);
    return errores==0 ? 0 : 1;
}
