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

static constexpr int N = CANTIDAD_MODELOS_ESFERAS_CANON_3D;
static int errores = 0, tecla = KEY_NULL, avisos = 0;
static int cargas[N]{}, descargas[N]{}, cantidades[N]{};
static int cubos = 0, esferas = 0, cilindros = 0, segmentos = 0, circulos = 0;
static int triangulosSombra = 0;
static int teclaMantenida = KEY_NULL, planos = 0, indicadores = 0;
static const char* rutaAusente = nullptr;
static Color originales[N][16]{};
static const Color colores[] = {{235,80,80,255},{80,140,240,255},
    {90,205,115,255},{245,205,70,255}};
struct DibujoEsferas
{
    Vector3 posicion{}, eje{}, escala{};
    float angulo = 0;
    Color color{};
    int malla = -1;
};
static DibujoEsferas dibujos[N][64]{};
static auto& Recursos() { return ObtenerModelosEscenariosRetro3D().esferasCanon; }
static void Comprobar(bool valor, const char* mensaje)
{
    if (!valor) { if (errores < 20) std::fprintf(stderr,"FALLO: %s\n",mensaje); errores++; }
}
static bool Cerca(float a, float b) { return std::fabs(a-b) < .002f; }
static bool Igual(Vector3 a, Vector3 b)
{ return Cerca(a.x,b.x) && Cerca(a.y,b.y) && Cerca(a.z,b.z); }
static int Indice(const char* ruta)
{
    for (int i=0;i<N;i++) if (std::strcmp(ruta,RUTAS_MODELOS_ESFERAS_CANON_3D[i])==0) return i;
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
        int dinamico=r.materialColor;
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
    auto& m=zona.gestorMinijuegos.minijuegoEsferasCanon;
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

extern "C" void __real_DrawSphereEx(Vector3,float,int,int,Color);
extern "C" void __wrap_DrawSphereEx(Vector3 p,float r,int a,int c,Color color)
{ esferas++; __real_DrawSphereEx(p,r,a,c,color); }

// Fixtures sobre la curva logica existente; Actualizar conserva toda la fisica.
static Vector3 Punto(const MinijuegoEsferasCanon& m,float s,float l=0,Vector3* tangente=nullptr)
{
    int i=0; while (i<PUNTOS_PISTA_ESFERAS-2 && m.puntosS[i+1]<s) i++;
    float dx=m.puntosX[i+1]-m.puntosX[i], dz=m.puntosZ[i+1]-m.puntosZ[i];
    float largo=std::sqrt(dx*dx+dz*dz); float t=(s-m.puntosS[i])/largo;
    if (tangente) *tangente={dx/largo,0,dz/largo};
    return {m.puntosX[i]+dx*t-dz/largo*l,-.12f*s,m.puntosZ[i]+dz*t+dx/largo*l};
}
static void Colocar(ZonaPruebas& zona,int i,float s,float lateral)
{
    auto& m=zona.gestorMinijuegos.minijuegoEsferasCanon; Vector3 p=Punto(m,s,lateral);
    auto& e=m.estadosJugadores[i]; e={}; e.participa=true;
    e.x=p.x; e.z=p.z; e.s=e.progresoMaximo=s; e.lateral=lateral;
    zona.jugadores[i].posicion={p.x,p.y+2.7f,p.z};
}
static void ComprobarModelos(ZonaPruebas& zona)
{
    const auto& m=zona.gestorMinijuegos.minijuegoEsferasCanon;
    for (int i=0;i<N;i++) Comprobar(cantidades[i]>0,"Todos los 26 GLB tienen uso");
    for (int i=0;i<=MODELO_ESFERAS_ARCO_NATURAL;i++)
        Comprobar(cantidades[i]==1 && Igual(dibujos[i][0].posicion,{0,0,0}) &&
            Igual(dibujos[i][0].escala,{1,1,1}) && Cerca(dibujos[i][0].angulo,0),
            "Tramos, paredes, puentes, rampa y arco globales sin transformar dos veces");
    Comprobar(cantidades[MODELO_ESFERAS_SUELO]==1 && Igual(dibujos[MODELO_ESFERAS_SUELO][0].posicion,{0,0,0}),"Suelo conserva origen global");
    Comprobar(cantidades[MODELO_ESFERAS_MESA]==10 && cantidades[MODELO_ESFERAS_CACTUS]==28 && cantidades[MODELO_ESFERAS_ROCA]==10,
        "Instancias comparten modelos y conservan las cantidades reales");
    for(int i=0;i<10;i++)
    {
        const auto& d=dibujos[MODELO_ESFERAS_MESA][i];
        Comprobar(Cerca(d.posicion.y,-.12f*(-d.posicion.z)-2) && d.escala.y>=.8f && d.escala.y<=1.6f,
            "Mesas conservan base y altura procedural");
        const auto& o=m.obstaculos[i]; const auto& r=dibujos[MODELO_ESFERAS_ROCA][i];
        Comprobar(Igual(r.posicion,{o.x,-.12f*o.s,o.z}) && Cerca(r.escala.x,o.radio/.9f),"Rocas siguen los obstaculos logicos sin cambiar hitboxes");
    }
    Comprobar(cantidades[MODELO_ESFERAS_CHECKPOINT]==6,"Dos banderas por checkpoint");
    for(int i=0;i<6;i++)
    {
        Vector3 t; int lado=i%2==0 ? -1 : 1;
        Vector3 p=Punto(m,28+30*(i/2),lado*3.9f,&t);
        const auto& d=dibujos[MODELO_ESFERAS_CHECKPOINT][i];
        Comprobar(Igual(p,d.posicion),"Banderas mantienen S y lateral actuales");
        Vector3 x=Vector3Transform({1,0,0},MatrixRotateY(d.angulo*DEG2RAD));
        Comprobar(Igual(x,{-t.z*(-lado),0,t.x*(-lado)}),"Yaw de raylib coloca banderas hacia el interior");
    }
    for (int id:{MODELO_ESFERAS_SALIDA,MODELO_ESFERAS_META})
    {
        Vector3 t; Vector3 p=Punto(m,id==MODELO_ESFERAS_SALIDA ? .8f : 120,0,&t);
        const auto& d=dibujos[id][0];
        Vector3 x=Vector3Transform({1,0,0},MatrixRotateY(d.angulo*DEG2RAD));
        Comprobar(Igual(p,d.posicion) && Igual(x,{-t.z,0,t.x}),"Arcos al pie y perpendiculares a la pista");
    }
    int visible=0;
    for(int i=0;i<zona.cantidadParticipantes;i++)
    {
        const auto& e=m.estadosJugadores[i];
        if(e.penalizacion>0 && e.penalizacion<=1.4f && std::sin(m.tiempoAnimacion*24)<0) continue;
        const auto& b=dibujos[MODELO_ESFERAS_ESFERA][visible];
        const auto& a=dibujos[MODELO_ESFERAS_ARO][visible++]; const Vector3 p=zona.jugadores[i].posicion;
        Comprobar(Igual(b.posicion,{p.x,p.y-1.7f,p.z}) && Cerca(b.angulo,e.giro*RAD2DEG) &&
            Igual(b.eje,{e.ejeX,0,e.ejeZ}) && Igual(b.escala,{1,1,1}),"Centro, radio y giro de esfera siguen el estado real");
        Comprobar(Igual(a.posicion,{p.x,p.y-2.7f,p.z}) && ColorIsEqual(a.color,Fade(zona.participantes[i].color,.7f)),
            "Aro sigue salto/caida y colorea solo COLOR_DINAMICO");
    }
    Comprobar(cantidades[MODELO_ESFERAS_ESFERA]==visible && cantidades[MODELO_ESFERAS_ARO]==visible,"Parpadeo mantiene esfera, aro y jugador juntos");
}
int main()
{
    SetTraceLogCallback(Registrar); SetTraceLogLevel(LOG_WARNING);
    SetConfigFlags(FLAG_WINDOW_HIDDEN); InitWindow(1280,800,"Verificacion del juego integrado Esferas del Canon");
    if (!IsWindowReady()) return 2;
    SetExitKey(KEY_NULL); SetRandomSeed(976);
    static ZonaPruebas zona; Participante participantes[MAX_PARTICIPANTES]{}; Mesh* mallas[N]{};
    for(int cantidad=2;cantidad<=4;cantidad++)
    {
        for(int i=0;i<MAX_PARTICIPANTES;i++)
        {
            participantes[i]={}; participantes[i].activo=i<cantidad; participantes[i].conectado=true;
            participantes[i].control=CONTROL_TECLADO_WASD; participantes[i].numeroJugador=i+1; participantes[i].color=colores[i];
        }
        zona.Inicializar(participantes,cantidad); zona.modoCatalogo=true;
        if(cantidad==2)
        {
            for(const auto& r:Recursos()) Comprobar(!r.cargaIntentada,"No cargar Esferas al iniciar catalogo");
            participantes[1].activo=false; zona.CambiarMinijuego(MINIJUEGO_ESFERAS_CANON);
            Comprobar(zona.gestorMinijuegos.minijuegoEsferasCanon.resultado.estado==RESULTADO_MINIJUEGO_CANCELADO,"Cancelar un solo participante");
            for(const auto& r:Recursos()) Comprobar(!r.cargaIntentada,"Ronda invalida no carga recursos");
            participantes[1].activo=true;
        }
        zona.CambiarMinijuego(MINIJUEGO_ESFERAS_CANON); auto& m=zona.gestorMinijuegos.minijuegoEsferasCanon;
        for(int i=0;i<N;i++)
        {
            const auto& r=Recursos()[i]; Comprobar(r.cargado,RUTAS_MODELOS_ESFERAS_CANON_3D[i]);
            if(!r.cargado) { zona.Descargar(); DescargarModeloJugadorCompartido(); CloseWindow(); return 2; }
            if(cantidad==2) mallas[i]=r.modelo.meshes;
            Comprobar(cargas[i]==1 && r.modelo.meshes==mallas[i],"Carga unica entre rondas y participantes");
            for(int j=0;j<r.modelo.meshCount;j++) Comprobar(r.modelo.meshes[j].colors!=nullptr,"Colores de vertice conservados");
        }
        GuardarMateriales(); Dibujar(zona,cantidad==4 ? "build/esferas-preparacion.png" : nullptr); ComprobarModelos(zona);
        // El renderizador compartido mantiene un cilindro de sombra por jugador.
        Comprobar(cubos==0 && cilindros==cantidad && esferas==0,"Solo sombras de jugadores, sin primitivas reemplazadas");
        Avanzar(zona,182); Comprobar(m.fase==FASE_ESFERAS_JUGANDO,"Preparacion de tres segundos");
        teclaMantenida=KEY_W; Pulsar(zona,KEY_E); teclaMantenida=KEY_NULL;
        Comprobar(m.estadosJugadores[0].recargaImpulso>2.9f && m.estadosJugadores[0].giro>0,"Controles reales activan impulso y rotacion");
        Dibujar(zona); ComprobarModelos(zona);
        // Pared, obstaculo y choques elasticos conservan la fisica existente.
        Pulsar(zona,KEY_R); Avanzar(zona,182);
        Colocar(zona,0,5,3.3f); m.estadosJugadores[0].velocidadX=12;
        Avanzar(zona,3); Comprobar(m.estadosJugadores[0].lateral<=3.402f && m.estadosJugadores[0].velocidadX<0,"Rebote de pared con limite logico");
        Colocar(zona,0,10,-.6f); m.estadosJugadores[0].velocidadX=-5;
        Avanzar(zona,1); Comprobar(m.estadosJugadores[0].velocidadX>0,"Rebote de roca conserva radio logico");
        Colocar(zona,0,5,-.4f); Colocar(zona,1,5,.4f);
        m.estadosJugadores[0].velocidadX=3; m.estadosJugadores[1].velocidadX=-3;
        Avanzar(zona,1); Comprobar(m.estadosJugadores[0].velocidadX<0 && m.estadosJugadores[1].velocidadX>0,"Choque entre esferas conserva restitucion");
        // Checkpoints, puente seguro, caida, reaparicion y parpadeo.
        const float grietas[]={41,75,98}, seguros[]={1.5f,-1.5f,0}, puntos[]={28,58,88};
        for(int g=0;g<3;g++)
        {
            Pulsar(zona,KEY_R); Avanzar(zona,182);
            // Acercar participantes y camara usando la actualizacion real.
            for(int i=0;i<cantidad;i++) Colocar(zona,i,grietas[g]-8-i*.5f,(i-1.5f)*1.5f);
            Avanzar(zona,60);
            Colocar(zona,0,puntos[g]+.1f,0); m.estadosJugadores[0].checkpoints=g;
            Avanzar(zona,1); Comprobar(m.estadosJugadores[0].checkpoints==g+1,"Activar checkpoint logico");
            Colocar(zona,0,grietas[g],seguros[g]); m.estadosJugadores[0].checkpoints=g+1;
            Avanzar(zona,1); Comprobar(m.estadosJugadores[0].penalizacion==0,"Paso seguro del puente");
            Colocar(zona,0,grietas[g],seguros[g]+2); m.estadosJugadores[0].checkpoints=g+1;
            Avanzar(zona,1); Comprobar(m.estadosJugadores[0].penalizacion>1.9f,"Grieta activa penalizacion original");
            Avanzar(zona,15); Dibujar(zona,cantidad==4 && g==0 ? "build/esferas-caida.png" : nullptr); ComprobarModelos(zona);
            const auto& e=m.estadosJugadores[0]; float t=2-e.penalizacion;
            Comprobar(Cerca(dibujos[MODELO_ESFERAS_ESFERA][0].posicion.y,-.12f*e.s+1-14*t*t),"Animacion de caida mantiene parabola original");
            Avanzar(zona,24); Comprobar(Cerca(m.estadosJugadores[0].s,puntos[g]),"Reaparecer en el ultimo checkpoint");
            m.tiempoAnimacion=3.14159265f/16; Dibujar(zona);
            Comprobar(cantidades[MODELO_ESFERAS_ESFERA]==cantidad-1 && cantidades[MODELO_ESFERAS_ARO]==cantidad-1,"Ocultar GLB durante parpadeo");
            Avanzar(zona,85); Comprobar(m.estadosJugadores[0].penalizacion==0,"Termina penalizacion de dos segundos");
        }
        Pulsar(zona,KEY_R); Avanzar(zona,182);
        for(int i=0;i<cantidad;i++) Colocar(zona,i,88.5f+i*.02f,(i-(cantidad-1)*.5f)*2.2f);
        Avanzar(zona,60);
        for(int i=0;i<cantidad;i++) Colocar(zona,i,90.5f+i*.02f,i==0 ? 2.8f : -2.8f+i*.1f);
        Vector3 tangente; Punto(m,90.5f,0,&tangente);
        m.estadosJugadores[0].velocidadX=tangente.x*3; m.estadosJugadores[0].velocidadZ=tangente.z*3;
        Avanzar(zona,1); Comprobar(m.estadosJugadores[0].tiempoAire>1,"Rampa activa salto procedural");
        Avanzar(zona,30); Dibujar(zona,cantidad==4 ? "build/esferas-rampa.png" : nullptr); ComprobarModelos(zona);
        const auto& e=m.estadosJugadores[0];
        Comprobar(Cerca(dibujos[MODELO_ESFERAS_ESFERA][0].posicion.y,-.12f*e.s+1+2.4f*std::sin(PI*(1-e.tiempoAire/1.1f))),"Esfera conserva arco de salto");
        // Meta real, cierre de doce segundos y clasificacion por progreso/tiempo.
        Pulsar(zona,KEY_R); Avanzar(zona,182); Colocar(zona,0,120.2f,0); Avanzar(zona,1);
        Comprobar(m.estadosJugadores[0].terminado && m.hayGanadorPorMeta && m.tiempoCierre>11.9f,"Meta abre cierre original");
        Avanzar(zona,721); Comprobar(m.fase==FASE_ESFERAS_TERMINADO && m.resultado.participantes[0].posicionFinal==1,"Final conserva ganador por meta");
        Dibujar(zona,cantidad==4 ? "build/esferas-final.png" : nullptr);
        Pulsar(zona,KEY_R); Avanzar(zona,182); m.estadosJugadores[0].progresoMaximo=50; m.tiempoRestante=.001f;
        Avanzar(zona,1); Comprobar(m.fase==FASE_ESFERAS_TERMINADO && m.resultado.participantes[0].posicionFinal==1,"Final por tiempo conserva progreso");
        Pulsar(zona,KEY_R); for(int i=0;i<cantidad;i++) participantes[i].esBot=true;
        Avanzar(zona,6000); Comprobar(m.fase==FASE_ESFERAS_TERMINADO,"IA termina ronda completa");
        for(int i=0;i<cantidad;i++) participantes[i].esBot=false;
        Pulsar(zona,KEY_ESCAPE); Comprobar(zona.volverAlMenu,"Salida al menu de pruebas");
        zona.Inicializar(participantes,cantidad); zona.modoCatalogo=zona.modoTablero=true;
        zona.CambiarMinijuego(MINIJUEGO_ESFERAS_CANON); Pulsar(zona,KEY_R); Pulsar(zona,KEY_ESCAPE);
        Comprobar(!zona.volverAlMenu && m.tiempoPreparacion<3,"Tablero bloquea reinicio y abandono");
        for(int i=0;i<N;i++) Comprobar(cargas[i]==1 && Recursos()[i].modelo.meshes==mallas[i],"Reinicio, selector y tablero conservan cache");
    }
    // Archivo ausente por pieza, sin mover ni editar ningun GLB.
    Pulsar(zona,KEY_NULL); Dibujar(zona); ComprobarModelos(zona);
    for(int ausente=0;ausente<N;ausente++)
    {
        Dibujar(zona); int presentes[N]; std::memcpy(presentes,cantidades,sizeof(presentes));
        int base=cubos+esferas+cilindros+segmentos+circulos+triangulosSombra;
        Comprobar(presentes[ausente]>0,"Modelo con uso antes de probar fallback");
        DescargarSlotModeloEscenarioRetro3D(Recursos()[ausente]); rutaAusente=RUTAS_MODELOS_ESFERAS_CANON_3D[ausente];
        for(int k=0;k<3;k++) CargarPaqueteModelosEscenarioRetro3D(ObtenerPaqueteEsferasCanonRetro3D());
        Dibujar(zona);
        for(int i=0;i<N;i++) Comprobar(cantidades[i]==(i==ausente ? 0 : presentes[i]),"Fallback conserva las otras piezas GLB");
        Comprobar(cubos+esferas+cilindros+segmentos+circulos+triangulosSombra>base,"Primitivas aparecen solo para la pieza fallida");
        rutaAusente=nullptr; Recursos()[ausente]={}; CargarPaqueteModelosEscenarioRetro3D(ObtenerPaqueteEsferasCanonRetro3D()); GuardarMateriales();
    }
    Comprobar(avisos==N,"Un solo diagnostico por recurso ausente");
    const auto& almacen=ObtenerModelosEscenariosRetro3D();
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
    DescargarModeloJugadorCompartido(); CloseWindow();
    std::printf("Verificacion Esferas del Canon 2/3/4 participantes: %d errores\n",errores);
    return errores==0 ? 0 : 1;
}
