// Ejecuta ZonaPruebas y el minijuego reales con OpenGL y entradas simuladas.
#include "Gameplay/ZonaPruebas.h"
#include "Minigames/ModelosEscenariosRetro3D.h"
#include "Minigames/ModeloJugadorCompartido.h"
#include "rlgl.h"
#include <cmath>
#include <cstdio>
#include <cstring>
#include <cstdarg>

static constexpr int N=CANTIDAD_MODELOS_BOLAS_AZUCAR_3D;
static int errores=0, tecla=KEY_NULL, avisos=0;
static int cargas[N]{}, descargas[N]{}, cantidades[N]{}, usos[N]{};
static int cubos=0, esferas=0, cilindros=0, planos=0, aros=0;
static const char* ausente=nullptr;
static Color originales[N][16]{};
static unsigned long long firmas[N]{};
static float alphasBolas[4]{};
static const Color colores[]={{235,80,80,255},{80,140,240,255},{90,205,115,255},{245,205,70,255}};
struct InstanciaAzucar { Vector3 posicion{},eje{},escala{}; float angulo=0; Color color{}; int malla=-1; };
static InstanciaAzucar dibujos[N][64]{};
static auto& Recursos() { return ObtenerModelosEscenariosRetro3D().bolasAzucar; }
static void Comprobar(bool ok,const char* texto)
{ if(!ok) { if(errores<30) std::fprintf(stderr,"FALLO: %s\n",texto); errores++; } }
static bool Cerca(float a,float b) { return std::fabs(a-b)<.002f; }
static bool Igual(Vector3 a,Vector3 b) { return Cerca(a.x,b.x) && Cerca(a.y,b.y) && Cerca(a.z,b.z); }
static int Indice(const char* ruta)
{ for(int i=0;i<N;i++) if(std::strcmp(ruta,RUTAS_MODELOS_BOLAS_AZUCAR_3D[i])==0) return i; return -1; }
static unsigned long long Firma(Model modelo)
{
    unsigned long long h=1469598103934665603ULL;
    for(int i=0;i<modelo.meshCount;i++)
    {
        Mesh& m=modelo.meshes[i];
        const unsigned char* vertices=reinterpret_cast<const unsigned char*>(m.vertices);
        for(int b=0;b<m.vertexCount*3*(int)sizeof(float);b++) h=(h^vertices[b])*1099511628211ULL;
        if(m.colors) for(int b=0;b<m.vertexCount*4;b++) h=(h^m.colors[b])*1099511628211ULL;
    }
    return h;
}
static void Registrar(int nivel,const char* formato,va_list args)
{
    char texto[2048]; std::vsnprintf(texto,sizeof(texto),formato,args);
    if(std::strstr(texto,"Modelo de escenario ausente")) avisos++;
    if(nivel>=LOG_WARNING) std::fprintf(stderr,"%s\n",texto);
}
extern "C" bool __real_IsKeyPressed(int);
extern "C" bool __wrap_IsKeyPressed(int k) { return k==tecla || __real_IsKeyPressed(k); }
extern "C" bool __real_IsKeyDown(int);
extern "C" bool __wrap_IsKeyDown(int k) { return k==tecla || __real_IsKeyDown(k); }
extern "C" bool __real_FileExists(const char*);
extern "C" bool __wrap_FileExists(const char* p)
{ return !(ausente && std::strcmp(p,ausente)==0) && __real_FileExists(p); }
extern "C" Model __real_LoadModel(const char*);
extern "C" Model __wrap_LoadModel(const char* p)
{ int i=Indice(p); if(i>=0) cargas[i]++; return __real_LoadModel(p); }
extern "C" void __real_UnloadModel(Model);
extern "C" void __wrap_UnloadModel(Model modelo)
{ for(int i=0;i<N;i++) if(modelo.meshes && modelo.meshes==Recursos()[i].modelo.meshes) descargas[i]++; __real_UnloadModel(modelo); }
extern "C" void __real_DrawModelEx(Model,Vector3,Vector3,float,Vector3,Color);
extern "C" void __wrap_DrawModelEx(Model modelo,Vector3 p,Vector3 eje,float giro,Vector3 escala,Color tinte)
{
    for(int i=0;i<N;i++) if(Recursos()[i].cargado)
    {
        auto& r=Recursos()[i]; int malla=-1;
        for(int j=0;j<r.modelo.meshCount;j++) if(modelo.meshes==&r.modelo.meshes[j]) malla=j;
        if(malla<0) continue;
        int n=cantidades[i]++; usos[i]++;
        int material=r.materialColor>=0?r.materialColor:modelo.meshMaterial[0];
        if(n<64) dibujos[i][n]={Vector3Transform(p,rlGetMatrixTransform()),eje,escala,giro,
            modelo.materials[material].maps[MATERIAL_MAP_DIFFUSE].color,malla};
        Comprobar(ColorIsEqual(tinte,WHITE),"Sin tinte global sobre materiales y vertices");
        Matrix identidad=MatrixIdentity();
        Comprobar(std::memcmp(&modelo.transform,&identidad,sizeof(Matrix))==0,"Pivote original sin centrado");
        for(int j=0;j<modelo.materialCount;j++)
        {
            Color c=modelo.materials[j].maps[MATERIAL_MAP_DIFFUSE].color;
            if(i==MODELO_AZUCAR_BOLA)
            {
                if(j==r.materialColor) continue;
                Comprobar(c.r==originales[i][j].r && c.g==originales[i][j].g && c.b==originales[i][j].b,"Cuerpo conserva color original durante desvanecimiento");
                bool cuerpo=j==r.modelo.meshMaterial[0] || j==r.modelo.meshMaterial[1];
                unsigned char alpha=cuerpo?(unsigned char)(originales[i][j].a*alphasBolas[n]):originales[i][j].a;
                Comprobar(c.a==alpha,"Reposo desvanece cuerpo y detalles sin tinte global");
            }
            else Comprobar(ColorIsEqual(c,originales[i][j]),"Materiales de escenario intactos");
        }
        if(i==MODELO_AZUCAR_SUELO) Comprobar(malla==0 || malla==1 || malla==4,"Pepitas incorporadas omitidas para evitar duplicacion");
        break;
    }
    __real_DrawModelEx(modelo,p,eje,giro,escala,tinte);
}
extern "C" void __real_DrawCube(Vector3,float,float,float,Color);
extern "C" void __wrap_DrawCube(Vector3 p,float x,float y,float z,Color c)
{ cubos++; __real_DrawCube(p,x,y,z,c); }
extern "C" void __real_DrawSphere(Vector3,float,Color);
extern "C" void __wrap_DrawSphere(Vector3 p,float r,Color c)
{ esferas++; __real_DrawSphere(p,r,c); }
extern "C" void __real_DrawCylinder(Vector3,float,float,float,int,Color);
extern "C" void __wrap_DrawCylinder(Vector3 p,float a,float b,float h,int n,Color c)
{ cilindros++; __real_DrawCylinder(p,a,b,h,n,c); }
extern "C" void __real_DrawPlane(Vector3,Vector2,Color);
extern "C" void __wrap_DrawPlane(Vector3 p,Vector2 s,Color c)
{ planos++; __real_DrawPlane(p,s,c); }
extern "C" void __real_DrawSphereWires(Vector3,float,int,int,Color);
extern "C" void __wrap_DrawSphereWires(Vector3 p,float r,int x,int y,Color c)
{ aros++; __real_DrawSphereWires(p,r,x,y,c); }
static void GuardarRecursos()
{
    for(int i=0;i<N;i++)
    {
        firmas[i]=Firma(Recursos()[i].modelo);
        for(int j=0;j<Recursos()[i].modelo.materialCount;j++) originales[i][j]=Recursos()[i].modelo.materials[j].maps[MATERIAL_MAP_DIFFUSE].color;
    }
}
static void Dibujar(ZonaPruebas& zona,const char* captura=nullptr)
{
    auto& m=zona.gestorMinijuegos.minijuegoBolasAzucar;
    unsigned char estado[sizeof(m)], jugadores[sizeof(zona.jugadores)];
    std::memcpy(estado,&m,sizeof(m)); std::memcpy(jugadores,zona.jugadores,sizeof(jugadores));
    std::memset(cantidades,0,sizeof(cantidades)); cubos=esferas=cilindros=planos=aros=0;
    int ordenBola=0;
    for(const auto& b:m.bolas) if(b.activa) alphasBolas[ordenBola++]=b.estado==BOLA_AZUCAR_REPOSO?1-b.inactiva/3*.6f:1;
    BeginDrawing(); zona.Dibujar();
    if(captura) { rlDrawRenderBatchActive(); Image img=LoadImageFromScreen(); Comprobar(ExportImage(img,captura),"Captura del juego integrado"); UnloadImage(img); }
    EndDrawing();
    Comprobar(std::memcmp(estado,&m,sizeof(m))==0,"El dibujo conserva fisica, arena, camara y reglas");
    Comprobar(std::memcmp(jugadores,zona.jugadores,sizeof(jugadores))==0,"Jugadores y hitboxes no cambian al dibujar");
    for(int i=0;i<N;i++) if(Recursos()[i].cargado)
    {
        Comprobar(firmas[i]==Firma(Recursos()[i].modelo),"Vertices y colores de vertice originales");
        for(int j=0;j<Recursos()[i].modelo.materialCount;j++) Comprobar(ColorIsEqual(originales[i][j],Recursos()[i].modelo.materials[j].maps[MATERIAL_MAP_DIFFUSE].color),"Material restaurado tras cada instancia");
    }
    int visibles=0;
    for(int i=0;i<4;i++) if(m.bolas[i].activa && Recursos()[MODELO_AZUCAR_BOLA].cargado)
    {
        auto& b=m.bolas[i]; const auto& d=dibujos[MODELO_AZUCAR_BOLA][visibles++];
        float alpha=b.estado==BOLA_AZUCAR_REPOSO?1-b.inactiva/3*.6f:1;
        Comprobar(Igual(d.posicion,{b.x,b.radio,b.z}) && Igual(d.escala,{b.radio,b.radio,b.radio}) && Igual(d.eje,{0,0,1}) && Cerca(d.angulo,b.giro*RAD2DEG),"Centro, radio y giro procedural de cada bola");
        Comprobar(ColorIsEqual(d.color,Fade(zona.participantes[i].color,.7f*alpha)),"Aro del duenio y desvanecimiento segun reposo");
    }
    Comprobar(cantidades[MODELO_AZUCAR_BOLA]==visibles,"Visibilidad usa bola.activa");
}
static void Avanzar(ZonaPruebas& zona,int frames)
{ for(int i=0;i<frames;i++) zona.Actualizar(1.0f/60); }
static void Preparar(ZonaPruebas& zona)
{ tecla=KEY_NULL; zona.CambiarMinijuego(MINIJUEGO_BOLAS_AZUCAR); Avanzar(zona,181); }
int main()
{
    SetTraceLogCallback(Registrar); SetTraceLogLevel(LOG_WARNING);
    SetConfigFlags(FLAG_WINDOW_HIDDEN); InitWindow(1280,800,"Verificacion integrada Bolas de Azucar");
    if(!IsWindowReady()) return 2;
    Participante p[4]{}; ZonaPruebas zona; zona.Inicializar(p,4); zona.modoCatalogo=true;
    auto& m=zona.gestorMinijuegos.minijuegoBolasAzucar;
    p[0].activo=true; m.Reiniciar(zona.jugadores,p,4);
    Comprobar(m.resultado.estado==RESULTADO_MINIJUEGO_CANCELADO,"Un participante cancela la partida");
    for(int i=0;i<N;i++) Comprobar(cargas[i]==0,"El catalogo y una partida invalida no cargan paquetes");
    for(int cantidad=2;cantidad<=4;cantidad++)
    {
        for(int i=0;i<4;i++) { p[i].activo=i<cantidad; p[i].conectado=true; p[i].esBot=false; p[i].numeroJugador=i+1; p[i].color=colores[i]; p[i].control=i==0?CONTROL_TECLADO_WASD:CONTROL_TECLADO_FLECHAS; }
        zona.cantidadParticipantes=cantidad; zona.CambiarMinijuego(MINIJUEGO_BOLAS_AZUCAR);
        for(int i=0;i<N;i++) Comprobar(Recursos()[i].cargado && cargas[i]==1,"20 modelos cargados una vez entre reinicios y participantes");
        GuardarRecursos();
        char ruta[100]; std::snprintf(ruta,sizeof(ruta),"build/azucar-preparacion-%d.png",cantidad); Dibujar(zona,ruta);
        Comprobar(cubos==0 && esferas==0 && planos==0 && cantidades[MODELO_AZUCAR_SUELO]==3,"Sin primitivas duplicadas; suelo usa vistas compartidas");
        Comprobar(cantidades[MODELO_AZUCAR_MURO]==4 && cantidades[MODELO_AZUCAR_AZUCAR_LATERAL]==2 && cantidades[MODELO_AZUCAR_AZUCAR_EXTREMO]==2 && cantidades[MODELO_AZUCAR_CHOCOLATE_PEQUENO]==2,"Instancias compartidas de muros y zonas");
        const Vector3 muros[]={{-9.2f,0,0},{9.2f,0,0},{0,0,-9.2f},{0,0,9.2f}};
        for(int i=0;i<4;i++) Comprobar(Igual(dibujos[MODELO_AZUCAR_MURO][i].posicion,muros[i]) && Cerca(dibujos[MODELO_AZUCAR_MURO][i].angulo,i<2?90:0),"Pivotes y orientacion de muros sin cambiar colisiones");
        Avanzar(zona,181); Comprobar(m.fase==FASE_BOLAS_JUGANDO,"Cuenta regresiva inicia la partida");
        tecla=KEY_E; Avanzar(zona,1); tecla=KEY_NULL;
        Comprobar(m.estadosJugadores[0].creando>0 && !m.bolas[0].activa,"Crear requiere medio segundo");
        Avanzar(zona,31); Comprobar(m.bolas[0].activa && m.bolas[0].estado==BOLA_AZUCAR_EMPUJADA,"Creacion real con radio inicial");
        Dibujar(zona,cantidad==4?"build/azucar-creacion.png":nullptr);
        zona.jugadores[0].posicion={5,.7f,0}; zona.jugadores[0].velocidad={}; zona.jugadores[0].direccionMirada={1,0,0};
        m.bolas[0].x=5.75f; m.bolas[0].z=0; float radio=m.bolas[0].radio;
        tecla=KEY_D; Avanzar(zona,40); tecla=KEY_NULL;
        Comprobar(m.bolas[0].radio>radio && m.bolas[0].giro>0,"Mover y empujar por azucar aumenta radio y giro");
        Dibujar(zona,cantidad==4?"build/azucar-crecimiento.png":nullptr);
        // Coloca el lanzamiento lejos del muro: cerca del borde la fisica
        // original rebota la bola en el mismo frame y reduce su velocidad.
        zona.jugadores[0].posicion={6,.7f,0}; zona.jugadores[0].velocidad={}; zona.jugadores[0].direccionMirada={1,0,0};
        m.bolas[0].x=6+.45f+m.bolas[0].radio; m.bolas[0].z=0;
        tecla=KEY_E; Avanzar(zona,1); tecla=KEY_NULL;
        Comprobar(m.bolas[0].estado==BOLA_AZUCAR_LANZADA && m.bolas[0].velocidadX>9,"Accion lanza usando estado real");
        // Reposo y transparencia del cuerpo, sin colorearlo por jugador.
        m.bolas[0]={true,BOLA_AZUCAR_REPOSO,0,4,0,0,1.4f,1.2f,1.5f};
        for(int i=1;i<cantidad;i++) m.bolas[i]={true,BOLA_AZUCAR_REPOSO,-6+4.0f*i,6.75f,0,0,.3f+.2f*i,.4f*i,1.5f};
        Dibujar(zona,cantidad==4?"build/azucar-reposo.png":nullptr);
        Avanzar(zona,100); Comprobar(!m.bolas[0].activa,"Reposo desaparece tras tres segundos");
        Preparar(zona); m.bolas[0]={true,BOLA_AZUCAR_LANZADA,0,0,3,0,.6f,0,0};
        Avanzar(zona,10); Comprobar(m.bolas[0].radio<.6f && m.bolas[0].velocidadX<3,"Chocolate derrite y frena la bola");
        m.bolas[0]={true,BOLA_AZUCAR_REPOSO,0,0,0,0,.23f,0,0}; Avanzar(zona,2);
        bool polvo=false; for(auto& a:m.particulas) polvo|=a.activa;
        Comprobar(!m.bolas[0].activa && polvo,"Derretimiento mantiene particulas");
        Preparar(zona); m.bolas[0]={true,BOLA_AZUCAR_LANZADA,-4.4f,-3.2f,6,0,.5f,0,0}; Avanzar(zona,1);
        Comprobar(m.bolas[0].velocidadX<0 && m.bolas[0].estado==BOLA_AZUCAR_LANZADA,"Rebote logico contra gominola");
        m.bolas[0]={true,BOLA_AZUCAR_LANZADA,8.6f,0,6,0,.5f,0,0}; Avanzar(zona,1);
        Comprobar(m.bolas[0].velocidadX<0 && m.bolas[0].x<=8.5f,"Rebote contra limite de arena");
        Preparar(zona); zona.jugadores[1].posicion={0,.7f,4}; zona.jugadores[1].velocidad={};
        m.bolas[0]={true,BOLA_AZUCAR_LANZADA,0,4,3,0,.5f,0,0}; Avanzar(zona,1);
        Comprobar(m.estadosJugadores[1].vidas==3 && m.estadosJugadores[1].aturdido>0 && m.mensajeTipo==2,"Bola pequena aturde sin quitar vida");
        Preparar(zona); zona.jugadores[1].posicion={0,.7f,4}; zona.jugadores[1].velocidad={};
        m.bolas[0]={true,BOLA_AZUCAR_LANZADA,0,4,3,0,1,0,0}; Avanzar(zona,1);
        Comprobar(m.estadosJugadores[1].vidas==2 && m.estadosJugadores[1].inmunidad>0 && m.mensajeTipo==1,"Bola grande quita vida y aplica inmunidad");
        Dibujar(zona,cantidad==4?"build/azucar-impacto.png":nullptr);
        m.estadosJugadores[1].inmunidad=0; m.estadosJugadores[1].vidas=1; zona.jugadores[1].velocidad={};
        m.bolas[0]={true,BOLA_AZUCAR_LANZADA,zona.jugadores[1].posicion.x,zona.jugadores[1].posicion.z,3,0,1,0,0}; Avanzar(zona,1);
        Comprobar(!m.estadosJugadores[1].vivo && zona.jugadores[1].posicion.y==-30 && !m.bolas[1].activa && m.mensajeTipo==3,"Eliminacion conserva orden y ocultacion");
        if(cantidad>2) { m.tiempoRestante=.001f; Avanzar(zona,1); }
        Comprobar(m.fase==FASE_BOLAS_TERMINADO && m.resultado.estado==RESULTADO_MINIJUEGO_FINALIZADO,"Final por supervivencia o tiempo");
        Dibujar(zona,cantidad==4?"build/azucar-final.png":nullptr);
        tecla=KEY_R; Avanzar(zona,1); tecla=KEY_NULL;
        Comprobar(m.fase==FASE_BOLAS_PREPARACION && m.estadosJugadores[1].vidas==3 && !m.bolas[0].activa,"R reinicia vidas y bolas sin recargar");
        zona.modoTablero=true; tecla=KEY_R; Avanzar(zona,1); tecla=KEY_ESCAPE; Avanzar(zona,1); tecla=KEY_NULL;
        Comprobar(!zona.volverAlMenu && m.tiempoPreparacion<3,"Ronda oficial protege reinicio y salida");
        zona.modoTablero=false; tecla=KEY_ESCAPE; Avanzar(zona,1); tecla=KEY_NULL;
        Comprobar(zona.volverAlMenu,"Salida al selector en pruebas"); zona.volverAlMenu=false;
        for(int i=0;i<cantidad;i++) p[i].esBot=true;
        Preparar(zona); Avanzar(zona,3700);
        Comprobar(m.fase==FASE_BOLAS_TERMINADO,"IA completa partida sin bloquearse");
    }
    Preparar(zona); m.bolas[0]={true,BOLA_AZUCAR_REPOSO,0,4,0,0,.8f,.7f,1};
    for(int pieza=0;pieza<N;pieza++)
    {
        Dibujar(zona); int base=cubos+esferas+cilindros+planos+aros, previo[N]; std::memcpy(previo,cantidades,sizeof(previo));
        DescargarSlotModeloEscenarioRetro3D(Recursos()[pieza]); ausente=RUTAS_MODELOS_BOLAS_AZUCAR_3D[pieza]; int avisosAntes=avisos;
        CargarPaqueteBolasAzucarRetro3D(); CargarPaqueteBolasAzucarRetro3D(); CargarPaqueteBolasAzucarRetro3D();
        Comprobar(avisos==avisosAntes+1,"Un diagnostico por archivo ausente");
        Dibujar(zona); Comprobar(cantidades[pieza]==0 && cubos+esferas+cilindros+planos+aros>base,"Fallback local por pieza sin GLB simultaneo");
        for(int i=0;i<N;i++) if(i!=pieza) Comprobar(cantidades[i]==previo[i],"Fallo no oculta otras piezas");
        ausente=nullptr; Recursos()[pieza]={}; CargarPaqueteBolasAzucarRetro3D(); GuardarRecursos();
    }
    for(int i=0;i<N;i++) Comprobar(usos[i]>0,"Los 20 modelos se usan en el juego integrado");
    zona.Descargar(); zona.Descargar();
    for(int i=0;i<N;i++) Comprobar(cargas[i]==descargas[i] && !Recursos()[i].modelo.meshes,"Descarga simetrica antes de CloseWindow");
    DescargarModeloJugadorCompartido(); CloseWindow();
    std::printf("Verificacion Bolas de Azucar 2/3/4 participantes: %d errores\n",errores);
    return errores?1:0;
}
