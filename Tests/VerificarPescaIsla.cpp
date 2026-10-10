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

static constexpr int N = CANTIDAD_MODELOS_PESCA_ISLENA_3D;
static int errores = 0, tecla = KEY_NULL, avisos = 0;
static int cargas[N]{}, descargas[N]{}, cantidades[N]{};
static int cubos = 0, esferas = 0, cilindros = 0, segmentos = 0, circulos = 0;
static int triangulosSombra = 0, lineas = 0; static float tiempoVisual = 0;
static int teclaMantenida = KEY_NULL, planos = 0, indicadores = 0;
static const char* rutaAusente = nullptr;
static Color originales[N][16]{};
static const Color colores[] = {{235,80,80,255},{80,140,240,255},
    {90,205,115,255},{245,205,70,255}};
struct DibujoPesca
{
    Vector3 posicion{}, eje{}, escala{};
    float angulo = 0;
    Color color{};
    int malla = -1;
};
static DibujoPesca dibujos[N][64]{};
static auto& Recursos() { return ObtenerModelosEscenariosRetro3D().pescaIslena; }
static void Comprobar(bool valor, const char* mensaje)
{
    if (!valor) { if (errores < 20) std::fprintf(stderr,"FALLO: %s\n",mensaje); errores++; }
}
static bool Cerca(float a, float b) { return std::fabs(a-b) < .002f; }
static bool Igual(Vector3 a, Vector3 b)
{ return Cerca(a.x,b.x) && Cerca(a.y,b.y) && Cerca(a.z,b.z); }
static int Indice(const char* ruta)
{
    for (int i=0;i<N;i++) if (std::strcmp(ruta,RUTAS_MODELOS_PESCA_ISLENA_3D[i])==0) return i;
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
static void ComprobarAnimacion(int pieza,int instancia);
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
        int dinamico=r.materialColor;
        for (int j=0;j<m.materialCount && j<16;j++) if (j!=dinamico)
            Comprobar(ColorIsEqual(originales[i][j],m.materials[j].maps[MATERIAL_MAP_DIFFUSE].color),"Materiales estaticos intactos");
        ComprobarAnimacion(i,n);
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
    auto& m=zona.gestorMinijuegos.minijuegoPescaIsla;
    unsigned char antes[sizeof(m)]; std::memcpy(antes,&m,sizeof(m));
    Vector3 posiciones[MAX_PARTICIPANTES], tamanos[MAX_PARTICIPANTES];
    for (int i=0;i<MAX_PARTICIPANTES;i++)
    { posiciones[i]=zona.jugadores[i].posicion; tamanos[i]=zona.jugadores[i].tamano; }
    tiempoVisual=m.tiempoAnimacion; lineas=0; BeginDrawing(); zona.Dibujar();
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

extern "C" void __real_DrawSphereEx(Vector3,float,int,int,Color);
extern "C" void __wrap_DrawSphereEx(Vector3 p,float r,int a,int c,Color color)
{ esferas++; __real_DrawSphereEx(p,r,a,c,color); }

extern "C" void __real_DrawLine3D(Vector3,Vector3,Color);
extern "C" void __wrap_DrawLine3D(Vector3 a,Vector3 b,Color c)
{ lineas++; __real_DrawLine3D(a,b,c); }
static void ComprobarAnimacion(int pieza,int instancia)
{
    int indice=pieza==MODELO_PESCA_GAVIOTA ? 0 : pieza==MODELO_PESCA_HUMO ? 1 :
        pieza>=MODELO_PESCA_PEQUENO && pieza<=MODELO_PESCA_DORADO ? 2+pieza-MODELO_PESCA_PEQUENO : -1;
    if(indice<0) return;
    auto& a=ObtenerModelosEscenariosRetro3D().animacionesPesca[indice];
    const auto& m=Recursos()[pieza].modelo.meshes[indice<2 ? 0 : 1];
    Comprobar(a.vertices!=nullptr,"Animacion preparada una vez por recurso");
    if(!a.vertices) return;
    for(int v=0;v<m.vertexCount;v++)
    {
        int p=v*3; Vector3 normal={m.normals[p],m.normals[p+1],m.normals[p+2]};
        Comprobar(std::isfinite(m.vertices[p]) && std::isfinite(m.vertices[p+1]) &&
            std::isfinite(m.vertices[p+2]) && Cerca(Vector3Length(normal),1),"Vertices y normales animados validos");
        if(indice==0)
        {
            float peso=std::fmin(1.0f,std::fmax(0.0f,(std::fabs(a.vertices[p])-.24f)/.66f));
            Comprobar(Cerca(m.vertices[p+1],a.vertices[p+1]+peso*std::sin(tiempoVisual*8+instancia)*.3f),"Aleteo conserva fase y amplitud");
        }
        if(indice==1)
        {
            int h=v/240; float fase=std::fmod(tiempoVisual*.25f+h*.25f,1.0f);
            Comprobar(m.colors[v*4+3]==(unsigned char)(a.colores[v*4+3]*.7f*(1-fase)),"Humo conserva desvanecimiento por particula");
            for(int c=0;c<3;c++) Comprobar(m.colors[v*4+c]==a.colores[v*4+c],"Humo conserva RGB de vertices");
        }
    }
}
static void ComprobarReposo()
{
    const int piezas[]={MODELO_PESCA_GAVIOTA,MODELO_PESCA_HUMO,MODELO_PESCA_PEQUENO,MODELO_PESCA_MEDIANO,MODELO_PESCA_DORADO};
    for(int i=0;i<5;i++)
    {
        const auto& a=ObtenerModelosEscenariosRetro3D().animacionesPesca[i];
        const auto& r=Recursos()[piezas[i]]; if(!r.cargado) continue;
        const auto& m=r.modelo.meshes[i<2 ? 0 : 1]; int bytes=m.vertexCount*3*sizeof(float);
        Comprobar(a.vertices && std::memcmp(a.vertices,m.vertices,bytes)==0 && std::memcmp(a.normales,m.normals,bytes)==0,
            "Restaurar geometria y normales entre instancias y frames");
        if(a.colores) Comprobar(std::memcmp(a.colores,m.colors,m.vertexCount*4)==0,"Restaurar alpha del humo");
    }
}
static Vector3 Muelle(int i)
{ return i==0 ? Vector3{0,0,10.5f} : i==1 ? Vector3{0,0,-10.5f} : i==2 ? Vector3{-10.5f,0,0} : Vector3{10.5f,0,0}; }
static Vector3 Punta(int i)
{ Vector3 p=Muelle(i); return {p.x-p.x/10.5f*1.4f,2.7f,p.z-p.z/10.5f*1.4f}; }
static void ComprobarModelos(ZonaPruebas& zona)
{
    const auto& m=zona.gestorMinijuegos.minijuegoPescaIsla;
    for(int i:{MODELO_PESCA_OCEANO,MODELO_PESCA_ISLA,MODELO_PESCA_AGUA})
        Comprobar(cantidades[i]==1 && Igual(dibujos[i][0].posicion,{0,0,0}) && Igual(dibujos[i][0].escala,{1,1,1}),"Isla y agua conservan pivote global y unidades");
    Comprobar(cantidades[MODELO_PESCA_MUELLE]==4 && cantidades[MODELO_PESCA_PALMERA]==8 && cantidades[MODELO_PESCA_GAVIOTA]==4 &&
        cantidades[MODELO_PESCA_CORAL_1]==8 && cantidades[MODELO_PESCA_CORAL_2]==8,"Mallas compartidas entre todas las instancias");
    const float giros[]={0,180,-90,90};
    for(int i=0;i<4;i++) Comprobar(Igual(dibujos[MODELO_PESCA_MUELLE][i].posicion,Muelle(i)) &&
        Cerca(dibujos[MODELO_PESCA_MUELLE][i].angulo,giros[i]),"Cuatro muelles mantienen posicion y orientacion");
    for(int i=0;i<zona.cantidadParticipantes;i++)
    {
        const auto& d=dibujos[MODELO_PESCA_CANA][i];
        Vector3 tip={0,1.6f*d.escala.y,-1.4f};
        tip=Vector3Add(Vector3Transform(tip,MatrixRotateY(d.angulo*DEG2RAD)),d.posicion);
        Comprobar(Igual(tip,Punta(m.muelleDe[i])) && Igual(zona.jugadores[i].posicion,
            Vector3Add(Muelle(m.muelleDe[i]),{0,1.07f,0})),"Cana conecta mano y punta sin mover jugador ni hitbox");
    }
    int visibles[4]{}, libres=0, anzuelos=0;
    for(const auto& p:m.peces)
    {
        if(p.estado==ESTADO_PEZ_AUSENTE) continue;
        int modelo=MODELO_PESCA_PEQUENO+p.tipo; const auto& d=dibujos[modelo][visibles[p.tipo]++];
        Vector3 frente=Vector3Transform({1,0,0},MatrixRotateY(d.angulo*DEG2RAD));
        Comprobar(Igual(d.posicion,{p.x,-.9f,p.z}) && Igual(frente,{p.dirX,0,p.dirZ}),"Tipo, posicion y rumbo de peces dependen de peces[]");
    }
    for(int t=0;t<4;t++) Comprobar(cantidades[MODELO_PESCA_PEQUENO+t]==visibles[t],"No dibujar peces capturados ni ausentes");
    for(int i=0;i<zona.cantidadParticipantes;i++)
    {
        const auto& e=m.estados[i];
        if(e.estado==PESCA_LIBRE)
        {
            const auto& d=dibujos[MODELO_PESCA_CURSOR][libres++];
            Comprobar(Igual(d.posicion,{e.cursorX,.09f,e.cursorZ}) && ColorIsEqual(d.color,m.coloresJugadores[i]),"Cursor visible solo al apuntar y color solo en COLOR_DINAMICO");
        }
        else
        {
            const auto& d=dibujos[MODELO_PESCA_CORCHO][anzuelos++]; Vector3 p={e.anzueloX,.12f,e.anzueloZ};
            if(e.estado==PESCA_LANZANDO)
            {
                float u=std::fmin(1.0f,e.tiempoEstado/.45f); Vector3 tip=Punta(m.muelleDe[i]);
                p={tip.x+(p.x-tip.x)*u,tip.y*(1-u)+.12f*u+std::sin(PI*u)*1.5f,tip.z+(p.z-tip.z)*u};
            }
            else if(e.estado==PESCA_PICADA) p.y=-.08f;
            else if(e.estado==PESCA_TENSION)
            {
                Vector3 tip=Punta(m.muelleDe[i]); float u=e.progreso*.5f;
                p.x+=(tip.x-p.x)*u; p.z+=(tip.z-p.z)*u; p.y=.05f;
            }
            else p.y+=.05f*std::sin(m.tiempoAnimacion*3+i);
            Comprobar(Igual(d.posicion,p) && ColorIsEqual(d.color,m.coloresJugadores[i]),"Corcho sigue lanzamiento, espera, pique y tension; solo la tapa recibe color");
        }
    }
    Comprobar(cantidades[MODELO_PESCA_CURSOR]==libres && cantidades[MODELO_PESCA_CORCHO]==anzuelos,"Visibilidad por estado de pesca");
    ComprobarReposo();
}
static void PrepararPicada(ZonaPruebas& zona,int f,bool compartida=false)
{
    auto& m=zona.gestorMinijuegos.minijuegoPescaIsla;
    for(auto& p:m.peces) { p.estado=ESTADO_PEZ_AUSENTE; p.ausente=1000; }
    auto& p=m.peces[f]; p.estado=ESTADO_PEZ_NADANDO; p.x=p.z=0; p.temporizadorMorder=.001f; p.ignorar=0;
    for(int i=0;i<zona.cantidadParticipantes;i++)
    {
        m.estados[i]={}; m.estados[i].cursorX=4; m.estados[i].cursorZ=0;
        if(i==0 || (compartida && i==1))
        { m.estados[i].estado=PESCA_ESPERANDO; m.estados[i].tiempoEstado=1; }
    }
    Avanzar(zona,1); Comprobar(m.estados[0].estado==PESCA_PICADA,"Pique real desde atraccion del pez");
}
int main()
{
    SetTraceLogCallback(Registrar); SetTraceLogLevel(LOG_WARNING);
    SetConfigFlags(FLAG_WINDOW_HIDDEN); InitWindow(1280,800,"Verificacion del juego integrado Pesca Islena");
    if(!IsWindowReady()) return 2;
    SetExitKey(KEY_NULL); SetRandomSeed(977);
    static ZonaPruebas zona; Participante participantes[MAX_PARTICIPANTES]{}; Mesh* mallas[N]{};
    for(int cantidad=2;cantidad<=4;cantidad++)
    {
        for(int i=0;i<MAX_PARTICIPANTES;i++)
        {
            participantes[i]={}; participantes[i].activo=i<cantidad; participantes[i].conectado=true;
            participantes[i].control=i==0 ? CONTROL_TECLADO_WASD : CONTROL_TECLADO_FLECHAS;
            participantes[i].numeroJugador=i+1; participantes[i].color=colores[i];
        }
        zona.Inicializar(participantes,cantidad); zona.modoCatalogo=true;
        if(cantidad==2)
        {
            for(const auto& r:Recursos()) Comprobar(!r.cargaIntentada,"No cargar Pesca al iniciar catalogo");
            participantes[1].activo=false; zona.CambiarMinijuego(MINIJUEGO_PESCA_ISLA);
            Comprobar(zona.gestorMinijuegos.minijuegoPescaIsla.resultado.estado==RESULTADO_MINIJUEGO_CANCELADO,"Cancelar un solo participante");
            for(const auto& r:Recursos()) Comprobar(!r.cargaIntentada,"Ronda invalida no carga recursos");
            participantes[1].activo=true;
        }
        zona.CambiarMinijuego(MINIJUEGO_PESCA_ISLA); auto& m=zona.gestorMinijuegos.minijuegoPescaIsla;
        for(int i=0;i<N;i++)
        {
            const auto& r=Recursos()[i]; Comprobar(r.cargado,RUTAS_MODELOS_PESCA_ISLENA_3D[i]);
            if(!r.cargado) { zona.Descargar(); DescargarModeloJugadorCompartido(); CloseWindow(); return 2; }
            if(cantidad==2) mallas[i]=r.modelo.meshes;
            Comprobar(cargas[i]==1 && r.modelo.meshes==mallas[i],"Carga unica entre rondas y participantes");
            for(int j=0;j<r.modelo.meshCount;j++) Comprobar(r.modelo.meshes[j].colors!=nullptr,"Colores de vertice conservados");
        }
        GuardarMateriales(); Dibujar(zona,cantidad==4 ? "build/pesca-preparacion.png" : nullptr); ComprobarModelos(zona);
        Comprobar(cubos==0 && cilindros==0 && esferas==6 && segmentos==0 && planos==0,
            "Solo destellos de peces raros: sin primitivas reemplazadas");
        Avanzar(zona,182); Comprobar(m.fase==FASE_PESCA_JUGANDO,"Preparacion de tres segundos");
        float cursor=m.estados[0].cursorX; teclaMantenida=KEY_D; Avanzar(zona,1); teclaMantenida=KEY_NULL;
        Comprobar(m.estados[0].cursorX>cursor,"Cursor usa controles reales");
        Pulsar(zona,KEY_E); Avanzar(zona,12); Dibujar(zona,cantidad==4 ? "build/pesca-lanzamiento.png" : nullptr); ComprobarModelos(zona);
        Comprobar(m.estados[0].estado==PESCA_LANZANDO,"Lanzamiento de .45 segundos");
        Avanzar(zona,16); Comprobar(m.estados[0].estado==PESCA_ESPERANDO,"Lanzamiento pasa a espera");
        Dibujar(zona); ComprobarModelos(zona); Pulsar(zona,KEY_E);
        Comprobar(m.estados[0].estado==PESCA_LIBRE && m.estados[0].bloqueo>0,"Retirar carnada con accion");
        // Ventana compartida: solo el primero engancha; captura con tira/afloja.
        const int indices[]={0,6,10}; const int valores[]={1,2,5};
        for(int tipo=0;tipo<3;tipo++)
        {
            PrepararPicada(zona,indices[tipo],tipo==0); Dibujar(zona,cantidad==4 && tipo==2 ? "build/pesca-pique.png" : nullptr); ComprobarModelos(zona);
            Pulsar(zona,KEY_E); Comprobar(m.estados[0].estado==PESCA_TENSION,"Enganchar dentro de ventana");
            if(tipo==0) Comprobar(m.estados[1].estado==PESCA_LIBRE && m.estados[1].bloqueo>1.9f,"Competencia conserva perdida de carnada");
            Avanzar(zona,15); Dibujar(zona,cantidad==4 && tipo==2 ? "build/pesca-tension.png" : nullptr); ComprobarModelos(zona);
            for(int k=0;k<480 && m.estados[0].estado==PESCA_TENSION;k++)
            {
                const auto& e=m.estados[0];
                if((e.recogiendo && e.tension>.7f) || (!e.recogiendo && e.tension<.25f)) Pulsar(zona,KEY_E);
                else Avanzar(zona,1);
            }
            Comprobar(m.estados[0].puntos==valores[tipo] && m.estados[0].peces==1 && m.estados[0].mejorValor==valores[tipo] &&
                m.peces[indices[tipo]].estado==ESTADO_PEZ_AUSENTE,"Captura conserva valor, ausencia y popup");
            Dibujar(zona,cantidad==4 && tipo==2 ? "build/pesca-captura.png" : nullptr); ComprobarModelos(zona);
        }
        PrepararPicada(zona,12); Pulsar(zona,KEY_E);
        Comprobar(m.estados[0].estado==PESCA_LIBRE && m.estados[0].bloqueo>1.9f && m.estados[0].puntos==0,"Bota bloquea dos segundos sin sumar puntos");
        Avanzar(zona,241); Comprobar(m.peces[12].estado==ESTADO_PEZ_NADANDO,"Bota reaparece tras cuatro segundos");
        PrepararPicada(zona,0); Avanzar(zona,28);
        Comprobar(m.estados[0].estado==PESCA_LIBRE && m.estados[0].bloqueo>0,"Fallar ventana de pique devuelve carnada");
        PrepararPicada(zona,6); Pulsar(zona,KEY_E); m.estados[0].tension=.999f; Avanzar(zona,1);
        Comprobar(m.estados[0].estado==PESCA_LIBRE && m.peces[6].estado==ESTADO_PEZ_NADANDO,"Exceso de tension pierde pez");
        PrepararPicada(zona,6); Pulsar(zona,KEY_E); m.estados[0].tiempoPelea=8.01f; Avanzar(zona,1);
        Comprobar(m.estados[0].estado==PESCA_LIBRE,"Limite de ocho segundos conserva perdida de pez");
        m.tiempoRestante=.001f; Avanzar(zona,1);
        Comprobar(m.fase==FASE_PESCA_TERMINADO && m.resultado.desenlace==DESENLACE_EMPATE,"Final por tiempo y empate");
        Pulsar(zona,KEY_R); Avanzar(zona,182); m.estados[0].puntos=m.estados[1].puntos=5; m.estados[0].mejorValor=5; m.estados[1].mejorValor=2;
        m.tiempoRestante=.001f; Avanzar(zona,1);
        Comprobar(m.resultado.desenlace==DESENLACE_CON_GANADOR && m.resultado.participantes[0].posicionFinal==1,"Desempate conserva mejor pez");
        Dibujar(zona,cantidad==4 ? "build/pesca-final.png" : nullptr);
        Pulsar(zona,KEY_R); for(int i=0;i<cantidad;i++) participantes[i].esBot=true;
        Avanzar(zona,3300); Comprobar(m.fase==FASE_PESCA_TERMINADO,"IA completa ronda de cincuenta segundos");
        for(int i=0;i<cantidad;i++) participantes[i].esBot=false;
        Pulsar(zona,KEY_ESCAPE); Comprobar(zona.volverAlMenu,"Salida al menu de pruebas");
        zona.Inicializar(participantes,cantidad); zona.modoCatalogo=zona.modoTablero=true;
        zona.CambiarMinijuego(MINIJUEGO_PESCA_ISLA); Pulsar(zona,KEY_R); Pulsar(zona,KEY_ESCAPE);
        Comprobar(!zona.volverAlMenu && m.tiempoPreparacion<3,"Tablero conserva bloqueo de reinicio y abandono");
        for(int i=0;i<N;i++) Comprobar(cargas[i]==1 && Recursos()[i].modelo.meshes==mallas[i],"Reinicio, selector y tablero conservan cache");
    }
    auto& m=zona.gestorMinijuegos.minijuegoPescaIsla; m.tiempoAnimacion=1.234f;
    m.estados[1].estado=PESCA_ESPERANDO; m.estados[1].anzueloX=0; m.estados[1].anzueloZ=-4;
    for(int ausente=0;ausente<N;ausente++)
    {
        Dibujar(zona); int presentes[N]; std::memcpy(presentes,cantidades,sizeof(presentes));
        int base=cubos+esferas+cilindros+segmentos+circulos+planos+lineas;
        Comprobar(presentes[ausente]>0,"Los dieciocho GLB tienen uso");
        DescargarAnimacionesPescaIslenaRetro3D(); DescargarSlotModeloEscenarioRetro3D(Recursos()[ausente]);
        rutaAusente=RUTAS_MODELOS_PESCA_ISLENA_3D[ausente];
        for(int k=0;k<3;k++) CargarPaquetePescaIslenaRetro3D();
        Dibujar(zona); ComprobarReposo();
        for(int i=0;i<N;i++) Comprobar(cantidades[i]==(i==ausente ? 0 : presentes[i]),"Fallback conserva las otras piezas GLB");
        Comprobar(cubos+esferas+cilindros+segmentos+circulos+planos+lineas>base,"Solo la pieza ausente vuelve a primitivas");
        rutaAusente=nullptr; Recursos()[ausente]={}; CargarPaquetePescaIslenaRetro3D(); GuardarMateriales();
    }
    Comprobar(avisos==N,"Un solo diagnostico por recurso ausente");
    const auto& almacen=ObtenerModelosEscenariosRetro3D();
    for(const auto& r:almacen.esferasCanon) Comprobar(!r.cargaIntentada,"No cargar Esferas");
    for(const auto& r:almacen.parejasGlaciar) Comprobar(!r.cargaIntentada,"No cargar Glaciar");
    for(const auto& r:almacen.voleaMagma) Comprobar(!r.cargaIntentada,"No cargar Volea");
    for(const auto& r:almacen.ultimoAsiento) Comprobar(!r.cargaIntentada,"No cargar Asiento");
    for(const auto& r:almacen.cajasPuerto) Comprobar(!r.cargaIntentada,"No cargar Puerto");
    for(const auto& r:almacen.laberintoJade) Comprobar(!r.cargaIntentada,"No cargar Jade");
    for(const auto& r:almacen.vetaCristal) Comprobar(!r.cargaIntentada,"No cargar Veta");
    for(const auto& r:almacen.capsulasBarajadas) Comprobar(!r.cargaIntentada,"No cargar Capsulas");
    for(const auto& r:almacen.bateoMeteorico) Comprobar(!r.cargaIntentada,"No cargar Bateo");
    for(const auto& r:almacen.racimoToxico) Comprobar(!r.cargaIntentada,"No cargar Racimo");
    for(const auto& r:almacen.tesoreroCercado) Comprobar(!r.cargaIntentada,"No cargar Tesorero");
    for(const auto& r:almacen.descensoNubes) Comprobar(!r.cargaIntentada,"No cargar Nubes");
    zona.Descargar(); zona.Descargar();
    for(int i=0;i<N;i++) Comprobar(cargas[i]==descargas[i] && !Recursos()[i].modelo.meshes,"Descarga simetrica antes de CloseWindow");
    for(const auto& a:ObtenerModelosEscenariosRetro3D().animacionesPesca)
        Comprobar(!a.vertices && !a.normales && !a.colores,"Liberar copias CPU de animacion");
    DescargarModeloJugadorCompartido(); CloseWindow();
    std::printf("Verificacion Pesca Islena 2/3/4 participantes: %d errores\n",errores);
    return errores==0 ? 0 : 1;
}
