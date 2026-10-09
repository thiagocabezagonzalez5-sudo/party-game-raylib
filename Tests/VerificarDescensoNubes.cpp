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

static constexpr int N = CANTIDAD_MODELOS_DESCENSO_NUBES_3D;
static int errores = 0, tecla = KEY_NULL, avisos = 0;
static int cargas[N]{}, descargas[N]{}, cantidades[N]{};
static int cubos = 0, esferas = 0, cilindros = 0, segmentos = 0, circulos = 0;
static int triangulosSombra = 0;
static int teclaMantenida = KEY_NULL, planos = 0, indicadores = 0;
static int esferasEx=0, bandas=0, lineas=0, avesAnimadas=0;
static const char* rutaAusente = nullptr;
static Color originales[N][16]{};
static const Color colores[] = {{235,80,80,255},{80,140,240,255},
    {90,205,115,255},{245,205,70,255}};
struct DibujoNubes
{
    Vector3 posicion{}, eje{}, escala{};
    float angulo = 0;
    Color color{};
    int malla = -1;
};
static DibujoNubes dibujos[N][256]{};
static auto& Recursos() { return ObtenerModelosEscenariosRetro3D().descensoNubes; }
static void Comprobar(bool valor, const char* mensaje)
{
    if (!valor) { if (errores < 20) std::fprintf(stderr,"FALLO: %s\n",mensaje); errores++; }
}
static bool Cerca(float a, float b) { return std::fabs(a-b) < .002f; }
static bool Igual(Vector3 a, Vector3 b)
{ return Cerca(a.x,b.x) && Cerca(a.y,b.y) && Cerca(a.z,b.z); }
static int Indice(const char* ruta)
{
    for (int i=0;i<N;i++) if (std::strcmp(ruta,RUTAS_MODELOS_DESCENSO_NUBES_3D[i])==0) return i;
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
        Comprobar(n<256,"Limite de instancias");
        if (n<256) dibujos[i][n]={p,eje,escala,angulo,color,malla};
        Comprobar(ColorIsEqual(tinte,WHITE),"Sin tinte global de materiales y vertices");
        Matrix identidad=MatrixIdentity();
        Comprobar(std::memcmp(&m.transform,&identidad,sizeof(Matrix))==0,"Pivotes originales sin centrado");
        bool dinamico=i==MODELO_NUBES_PLANEADOR || i==MODELO_NUBES_PLANEADOR_FRENADO || i==MODELO_NUBES_BANDA;
        for (int j=0;j<m.materialCount && j<16;j++) if (!dinamico || j!=materialActual)
            Comprobar(ColorIsEqual(originales[i][j],m.materials[j].maps[MATERIAL_MAP_DIFFUSE].color),"Materiales estaticos intactos");
        if (i==MODELO_NUBES_AVE)
        {
            const auto& alas=ObtenerModelosEscenariosRetro3D().alasNubes;
            Comprobar(std::memcmp(m.meshes[0].vertices,alas.vertices[0],alas.inicioAlas*3*sizeof(float))==0,"Aleteo deja el cuerpo fijo");
            if (std::memcmp(m.meshes[1].vertices,alas.vertices[1],m.meshes[1].vertexCount*3*sizeof(float))!=0) avesAnimadas++;
        }
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
extern "C" void __real_DrawSphereEx(Vector3,float,int,int,Color);
extern "C" void __wrap_DrawSphereEx(Vector3 p,float r,int a,int b,Color c)
{ esferasEx++; __real_DrawSphereEx(p,r,a,b,c); }
extern "C" void __real_DrawCylinderWires(Vector3,float,float,float,int,Color);
extern "C" void __wrap_DrawCylinderWires(Vector3 p,float a,float b,float h,int l,Color c)
{ bandas++; __real_DrawCylinderWires(p,a,b,h,l,c); }
extern "C" void __real_DrawLine3D(Vector3,Vector3,Color);
extern "C" void __wrap_DrawLine3D(Vector3 a,Vector3 b,Color c)
{ lineas++; __real_DrawLine3D(a,b,c); }
static void Dibujar(ZonaPruebas& zona,const char* captura=nullptr)
{
    std::memset(cantidades,0,sizeof(cantidades));
    cubos=esferas=cilindros=segmentos=circulos=triangulosSombra=planos=indicadores=0;
    esferasEx=bandas=lineas=0;
    auto& m=zona.gestorMinijuegos.minijuegoDescensoNubes;
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
    if (Recursos()[MODELO_NUBES_AVE].cargado)
    {
        const auto& alas=ObtenerModelosEscenariosRetro3D().alasNubes;
        for (int i=0;i<2;i++)
        {
            const auto& mesh=Recursos()[MODELO_NUBES_AVE].modelo.meshes[i];
            Comprobar(std::memcmp(mesh.vertices,alas.vertices[i],mesh.vertexCount*3*sizeof(float))==0 &&
                std::memcmp(mesh.normals,alas.normales[i],mesh.vertexCount*3*sizeof(float))==0,"Vertices y normales del ave restaurados entre instancias");
        }
    }
}
static void Avanzar(ZonaPruebas& zona,int frames)
{ for (int i=0;i<frames;i++) zona.Actualizar(1.0f/60); }
static void Pulsar(ZonaPruebas& zona,int k)
{ tecla=k; Avanzar(zona,1); tecla=KEY_NULL; }

static int Primitivas()
{ return cubos+esferas+esferasEx+cilindros+segmentos+circulos+planos+bandas+lineas; }
static void EscenaControlada(ZonaPruebas& zona,float altura,bool frenado=false)
{
    auto& m=zona.gestorMinijuegos.minijuegoDescensoNubes;
    m.alturaCamara=altura; m.tiempoAnimacion=.07f;
    m.cantidadAnillos=2; m.anillos[0]={{-2,altura,0},1,0}; m.anillos[1]={{2,altura,0},3,0};
    m.cantidadTormentas=1; m.tormentas[0]={{0,altura,-3},1.6f,0};
    m.cantidadVientos=1; m.vientos[0]={altura,1,0};
    for (int i=0;i<zona.cantidadParticipantes;i++)
    {
        m.estados[i].altura=altura; m.estados[i].aterrizo=false;
        m.estados[i].frenando=frenado ? 1.f : 0.f; m.estados[i].aturdido=i==0 ? .5f : 0.f;
        zona.jugadores[i].posicion={-3.f+2*i,altura+.72f,2};
    }
}
static void ComprobarEscena(ZonaPruebas& zona)
{
    const auto& m=zona.gestorMinijuegos.minijuegoDescensoNubes;
    int planeadores=0, frenados=0, estrellas=0;
    for (int i=0;i<zona.cantidadParticipantes;i++)
    {
        if (m.estados[i].aterrizo) continue;
        int pieza=m.estados[i].frenando>0 ? MODELO_NUBES_PLANEADOR_FRENADO : MODELO_NUBES_PLANEADOR;
        int n=pieza==MODELO_NUBES_PLANEADOR ? planeadores++ : frenados++;
        const auto& d=dibujos[pieza][n];
        Comprobar(Igual(d.posicion,zona.jugadores[i].posicion) && Igual(d.escala,{1,1,1}) && Cerca(d.angulo,0),"Planeador conserva pivote del jugador y escala");
        Comprobar(ColorIsEqual(d.color,m.coloresJugadores[i]),"Solo COLOR_DINAMICO recibe el color del jugador");
        if (m.estados[i].aturdido>0) estrellas+=3;
    }
    Comprobar(cantidades[MODELO_NUBES_PLANEADOR]==planeadores && cantidades[MODELO_NUBES_PLANEADOR_FRENADO]==frenados && cantidades[MODELO_NUBES_ESTRELLA]==estrellas,"Variantes y estrellas segun estado real");
    if (m.alturaCamara<=30)
    {
        for (int pieza:{MODELO_NUBES_MAR,MODELO_NUBES_ISLA,MODELO_NUBES_DIANA})
            Comprobar(cantidades[pieza]==1 && Igual(dibujos[pieza][0].posicion,{0,0,0}),"Aterrizaje con origen compartido Y=0");
    }
    else for (int pieza:{MODELO_NUBES_MAR,MODELO_NUBES_ISLA,MODELO_NUBES_DIANA})
        Comprobar(cantidades[pieza]==0,"Isla oculta segun alturaCamara");
    for (int i=0;i<cantidades[MODELO_NUBES_ASPAS];i++)
    {
        const auto& d=dibujos[MODELO_NUBES_ASPAS][i];
        Comprobar(Igual(d.eje,{0,0,1}) && Cerca(d.angulo,m.tiempoAnimacion*1.2f*RAD2DEG),"Giro procedural de aspas sobre su eje Z");
        bool conectado=false;
        for (int j=0;j<cantidades[MODELO_NUBES_MOLINO];j++)
        {
            const auto& torre=dibujos[MODELO_NUBES_MOLINO][j]; float s=torre.escala.x;
            if (Igual(d.posicion,{torre.posicion.x,torre.posicion.y+4.15f*s,torre.posicion.z+.88f*s}) && Igual(d.escala,torre.escala)) conectado=true;
        }
        Comprobar(conectado,"Aspas conectadas a torre con offset escalado una vez");
    }
    unsigned todos=(1u<<zona.cantidadParticipantes)-1; int blancos=0,dorados=0;
    for (int i=0;i<m.cantidadAnillos;i++)
    {
        const auto& a=m.anillos[i];
        if (a.posicion.y<m.alturaCamara-17 || a.posicion.y>m.alturaCamara+9 || (a.recogidoPor&todos)==todos) continue;
        if (a.valor>=3)
        {
            int n=dorados++*3;
            Comprobar(Igual(dibujos[MODELO_NUBES_ANILLO_DORADO][n].posicion,a.posicion),"Anillo dorado en posicion logica");
            float s=(.28f+.05f*std::sin(m.tiempoAnimacion*6))/.252f;
            const auto& d=dibujos[MODELO_NUBES_ANILLO_DORADO][n+2];
            Comprobar(Igual(d.escala,{s,s,s}) && Cerca(d.posicion.y,a.posicion.y+.12f*(1-s)),"Pulso dorado conserva centro de BOMBILLAS");
        }
        else Comprobar(Igual(dibujos[MODELO_NUBES_ANILLO_BLANCO][blancos++].posicion,a.posicion),"Anillo blanco en posicion logica");
    }
    Comprobar(cantidades[MODELO_NUBES_ANILLO_BLANCO]==blancos && cantidades[MODELO_NUBES_ANILLO_DORADO]==dorados*3,"Recogida por participante y filtros de altura");
    int tormentas=0;
    for (int i=0;i<m.cantidadTormentas;i++)
    {
        const auto& t=m.tormentas[i];
        if (t.posicion.y<m.alturaCamara-17 || t.posicion.y>m.alturaCamara+9) continue;
        bool rayo=std::fmod(m.tiempoAnimacion*3+i,1.f)<.3f;
        for (int j=0;j<(rayo ? 4 : 3);j++)
        {
            const auto& d=dibujos[MODELO_NUBES_TORMENTA][tormentas++]; float s=t.radio/1.6f;
            Comprobar(d.malla==j && Igual(d.posicion,t.posicion) && Igual(d.escala,{s,s,s}),"Rayos parpadean y aro del GLB omitido en favor del indicador");
        }
    }
    Comprobar(cantidades[MODELO_NUBES_TORMENTA]==tormentas,"Sin rayos permanentes");
    int flechas=0,vientos=0;
    for (int v=0;v<m.cantidadVientos;v++)
    {
        const auto& viento=m.vientos[v];
        if (viento.altura<m.alturaCamara-17 || viento.altura>m.alturaCamara+9) continue;
        for (int j=0;j<2;j++)
        {
            const auto& d=dibujos[MODELO_NUBES_BANDA][vientos*2+j];
            Comprobar(Igual(d.posicion,{0,viento.altura,0}) && d.color.a==(unsigned char)(255*.22f),"Banda centrada con opacidad original del aviso");
        }
        vientos++;
        for (int a=0;a<5;a++)
        {
            float lateral=(a-2.f)*2.2f, avance=std::fmod(m.tiempoAnimacion*2+a*.4f,3.f)-1.5f;
            Vector3 centro={viento.dirZ!=0 ? lateral : avance*2*viento.dirX,viento.altura,viento.dirZ!=0 ? avance*2*viento.dirZ : lateral};
            const auto& d=dibujos[MODELO_NUBES_FLECHA][flechas++];
            Vector3 direccion=Vector3Transform({1,0,0},MatrixRotateY(d.angulo*DEG2RAD));
            Comprobar(Igual(d.posicion,centro) && Igual(direccion,{viento.dirX,0,viento.dirZ}),"Avance de flechas y orientacion hacia la fuerza real");
        }
    }
    Comprobar(cantidades[MODELO_NUBES_BANDA]==vientos*2 && cantidades[MODELO_NUBES_FLECHA]==flechas,"Instancias compartidas del viento");
    bool arco=62>m.alturaCamara-20 && 62<m.alturaCamara+24;
    Comprobar(cantidades[MODELO_NUBES_ARCOIRIS]==(arco ? 1 : 0),"Visibilidad original del arcoiris");
    Comprobar(cubos==0 && cilindros==0 && segmentos==0 && planos==0 && bandas==0 && esferasEx==0,"No superponer primitivas reemplazadas por GLB");
    Comprobar(triangulosSombra==8*zona.cantidadParticipantes,"Solo sombras de personajes, sin manchas del escenario");
}

int main()
{
    SetTraceLogCallback(Registrar); SetTraceLogLevel(LOG_WARNING);
    SetConfigFlags(FLAG_WINDOW_HIDDEN); InitWindow(1280,800,"Verificacion integrada Descenso en Nubes");
    if (!IsWindowReady()) return 2;
    SetExitKey(KEY_NULL); SetRandomSeed(974);
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
            for (const auto& r:Recursos()) Comprobar(!r.cargaIntentada,"No cargar al arrancar");
            participantes[1].activo=false; zona.CambiarMinijuego(MINIJUEGO_DESCENSO_NUBES);
            Comprobar(zona.gestorMinijuegos.minijuegoDescensoNubes.resultado.estado==RESULTADO_MINIJUEGO_CANCELADO,"Ronda invalida con un jugador");
            for (const auto& r:Recursos()) Comprobar(!r.cargaIntentada,"No cargar una ronda cancelada");
            participantes[1].activo=true;
        }
        zona.CambiarMinijuego(MINIJUEGO_DESCENSO_NUBES);
        auto& m=zona.gestorMinijuegos.minijuegoDescensoNubes;
        for (int i=0;i<N;i++)
        {
            const auto& r=Recursos()[i]; Comprobar(r.cargado,RUTAS_MODELOS_DESCENSO_NUBES_3D[i]);
            if (!r.cargado) { zona.Descargar(); DescargarModeloJugadorCompartido(); CloseWindow(); return 2; }
            if (cantidad==2) mallas[i]=r.modelo.meshes;
            Comprobar(cargas[i]==1 && r.modelo.meshes==mallas[i],"Carga unica compartida por jugadores y reinicios");
            for (int j=0;j<r.modelo.meshCount;j++) Comprobar(r.modelo.meshes[j].colors!=nullptr,"Colores de vertices conservados");
        }
        GuardarMateriales(); Dibujar(zona,cantidad==4 ? "build/nubes-preparacion.png" : nullptr); ComprobarEscena(zona);
        Avanzar(zona,182); Comprobar(m.fase==FASE_NUBES_JUGANDO,"Cuenta regresiva y comienzo reales");
        m.cantidadTormentas=m.cantidadVientos=m.cantidadAnillos=0;
        float antes=m.estados[0].altura; Pulsar(zona,KEY_E);
        Comprobar(m.estados[0].frenando>0 && Cerca(antes-m.estados[0].altura,1.25f/60),"Accion real frena a mitad de velocidad");
        Dibujar(zona,cantidad==4 ? "build/nubes-frenado.png" : nullptr); ComprobarEscena(zona);
        Avanzar(zona,62); Comprobar(m.estados[0].frenando==0 && m.estados[0].recargaFreno>0,"Freno dura un segundo y entra en recarga");
        Pulsar(zona,KEY_E); Comprobar(m.estados[0].frenando==0,"No frenar durante recarga");
        Avanzar(zona,122); Pulsar(zona,KEY_SPACE); Comprobar(m.estados[0].frenando>0,"Salto tambien activa el freno");
        teclaMantenida=KEY_D; float x=zona.jugadores[0].posicion.x; Avanzar(zona,30); teclaMantenida=KEY_NULL;
        Comprobar(zona.jugadores[0].posicion.x>x,"Movimiento horizontal con controles existentes");
        Pulsar(zona,KEY_R); Avanzar(zona,182);
        m.cantidadAnillos=1; m.cantidadTormentas=m.cantidadVientos=0;
        m.anillos[0]={zona.jugadores[0].posicion,3,0}; m.anillos[0].posicion.y=m.estados[0].altura;
        Avanzar(zona,1); Comprobar(m.estados[0].puntos==3 && m.anillos[0].recogidoPor==1,"Recogida dorada real y mascara individual");
        Dibujar(zona); ComprobarEscena(zona); Comprobar(cantidades[MODELO_NUBES_ANILLO_DORADO]==3,"Anillo visible para los demas");
        for (int i=1;i<cantidad;i++) { zona.jugadores[i].posicion=zona.jugadores[0].posicion; m.estados[i].altura=m.anillos[0].posicion.y; }
        Avanzar(zona,1); Dibujar(zona); ComprobarEscena(zona);
        Comprobar(cantidades[MODELO_NUBES_ANILLO_DORADO]==0,"Ocultar anillo recogido por todos");
        m.anillos[0]={zona.jugadores[0].posicion,1,0}; m.anillos[0].posicion.y=m.estados[0].altura;
        Avanzar(zona,1); Comprobar(m.estados[0].puntos==4,"Anillo blanco da un punto");
        m.cantidadAnillos=0; m.cantidadTormentas=1; m.tormentas[0]={zona.jugadores[0].posicion,1.6f,0};
        m.tormentas[0].posicion.y=m.estados[0].altura; Avanzar(zona,1);
        Comprobar(m.estados[0].puntos==2 && m.estados[0].golpes==1 && m.estados[0].aturdido>0,"Tormenta real resta dos y aturde");
        Dibujar(zona,cantidad==4 ? "build/nubes-tormenta.png" : nullptr); ComprobarEscena(zona);
        Avanzar(zona,1); Comprobar(m.estados[0].golpes==1,"Tormenta no repite impacto");
        m.cantidadTormentas=0; m.cantidadVientos=1;
        m.vientos[0]={m.estados[0].altura,0,-1}; m.estados[0].velX=m.estados[0].velZ=0;
        Avanzar(zona,1); Comprobar(m.estados[0].velZ<0,"Viento conserva fuerza original");
        for (float h:{100.f,62.f,26.f,0.f})
        {
            EscenaControlada(zona,h); Dibujar(zona,cantidad==4 && h==62 ? "build/nubes-arcoiris.png" : nullptr); ComprobarEscena(zona);
            m.tiempoAnimacion=.22f; m.tormentas[0].radio=2.1f;
            for (Vector3 dir: {Vector3{1,0,0},Vector3{-1,0,0},Vector3{0,0,1},Vector3{0,0,-1}})
            { m.vientos[0].dirX=dir.x; m.vientos[0].dirZ=dir.z; Dibujar(zona); ComprobarEscena(zona); }
        }
        Pulsar(zona,KEY_R); Avanzar(zona,182);
        m.cantidadAnillos=m.cantidadTormentas=m.cantidadVientos=0;
        for (int i=0;i<cantidad;i++)
        { m.estados[i].altura=.01f; zona.jugadores[i].posicion={i==0 ? 0.f : 4.f,.73f,0}; }
        Avanzar(zona,1);
        Comprobar(m.fase==FASE_NUBES_TERMINADO && m.estados[0].bonusCentro && m.estados[0].puntos==5 && !m.estados[1].bonusCentro,"Aterrizaje real: bonus central y final");
        m.alturaCamara=0; Dibujar(zona,cantidad==4 ? "build/nubes-aterrizaje-final.png" : nullptr); ComprobarEscena(zona);
        Pulsar(zona,KEY_R); Comprobar(m.fase==FASE_NUBES_PREPARACION && m.estados[0].puntos==0 && !m.estados[0].aterrizo,"Reinicio de estados y nivel");
        for (int i=0;i<cantidad;i++) participantes[i].esBot=true;
        Avanzar(zona,3300); Comprobar(m.fase==FASE_NUBES_TERMINADO && m.resultado.estado==RESULTADO_MINIJUEGO_FINALIZADO,"Ronda completa con IA y resultado");
        for (int i=0;i<cantidad;i++) { Comprobar(m.estados[i].aterrizo,"Todos aterrizan"); participantes[i].esBot=false; }
        Pulsar(zona,KEY_R); Avanzar(zona,182); m.tiempoJuego=51.99f; Avanzar(zona,1);
        Comprobar(m.fase==FASE_NUBES_TERMINADO,"Final por limite de 52 segundos");
        Pulsar(zona,KEY_ESCAPE); Comprobar(zona.volverAlMenu,"Regreso al menu/catalogo");
        zona.Inicializar(participantes,cantidad); zona.modoCatalogo=zona.modoTablero=true;
        zona.CambiarMinijuego(MINIJUEGO_DESCENSO_NUBES); Pulsar(zona,KEY_R); Pulsar(zona,KEY_ESCAPE);
        Comprobar(!zona.volverAlMenu && m.tiempoPreparacion<3,"Modo tablero conserva bloqueo de R y Escape");
        for (int i=0;i<N;i++) Comprobar(cargas[i]==1 && Recursos()[i].modelo.meshes==mallas[i],"Reentrada sin nuevas cargas");
    }
    Comprobar(avesAnimadas>0,"Aleteo procedural observado en dibujo real");
    for (int ausente=0;ausente<N;ausente++)
    {
        // Dos alturas cubren fondo alto y aterrizaje; el frenado elige su variante.
        float h=(ausente==MODELO_NUBES_MAR || ausente==MODELO_NUBES_ISLA || ausente==MODELO_NUBES_DIANA) ? 20.f : 62.f;
        EscenaControlada(zona,h,ausente==MODELO_NUBES_PLANEADOR_FRENADO); Dibujar(zona);
        int presentes[N]; std::memcpy(presentes,cantidades,sizeof(presentes)); int base=Primitivas();
        Comprobar(presentes[ausente]>0,"Cada GLB tiene un uso visible integrado");
        if (ausente==MODELO_NUBES_AVE) DescargarAlasDescensoNubesRetro3D();
        DescargarSlotModeloEscenarioRetro3D(Recursos()[ausente]); rutaAusente=RUTAS_MODELOS_DESCENSO_NUBES_3D[ausente];
        for (int k=0;k<3;k++) CargarPaqueteDescensoNubesRetro3D();
        Dibujar(zona);
        for (int i=0;i<N;i++) Comprobar(cantidades[i]==(i==ausente ? 0 : presentes[i]),"Fallback por pieza conserva las demas");
        Comprobar(Primitivas()>base,"Fallback primitivo visible");
        rutaAusente=nullptr; Recursos()[ausente]={}; CargarPaqueteDescensoNubesRetro3D(); GuardarMateriales();
    }
    Comprobar(avisos==N,"Un unico diagnostico por GLB ausente");
    const auto& almacen=ObtenerModelosEscenariosRetro3D();
    for (const auto& r:almacen.ultimoAsiento) Comprobar(!r.cargaIntentada,"No cargar Asiento");
    for (const auto& r:almacen.cajasPuerto) Comprobar(!r.cargaIntentada,"No cargar Puerto");
    for (const auto& r:almacen.laberintoJade) Comprobar(!r.cargaIntentada,"No cargar Jade");
    for (const auto& r:almacen.vetaCristal) Comprobar(!r.cargaIntentada,"No cargar Veta");
    for (const auto& r:almacen.capsulasBarajadas) Comprobar(!r.cargaIntentada,"No cargar Capsulas");
    for (const auto& r:almacen.bateoMeteorico) Comprobar(!r.cargaIntentada,"No cargar Bateo");
    for (const auto& r:almacen.racimoToxico) Comprobar(!r.cargaIntentada,"No cargar Racimo");
    for (const auto& r:almacen.tesoreroCercado) Comprobar(!r.cargaIntentada,"No cargar Tesorero");
    zona.Descargar(); zona.Descargar();
    for (int i=0;i<N;i++) Comprobar(cargas[i]==descargas[i] && !Recursos()[i].modelo.meshes,"Descarga simetrica antes de CloseWindow");
    Comprobar(!almacen.alasNubes.vertices[0] && !almacen.alasNubes.normales[1],"Liberar tambien copias CPU de alas");
    DescargarModeloJugadorCompartido(); CloseWindow();
    std::printf("Verificacion Descenso en Nubes 2/3/4 participantes: %d errores\n",errores);
    return errores==0 ? 0 : 1;
}
