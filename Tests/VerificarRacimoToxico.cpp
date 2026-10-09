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

static constexpr int N = CANTIDAD_MODELOS_RACIMO_TOXICO_3D;
static int errores = 0, tecla = KEY_NULL, avisos = 0;
static int cargas[N]{}, descargas[N]{}, cantidades[N]{};
static int cubos = 0, esferas = 0, cilindros = 0, segmentos = 0, circulos = 0;
static int triangulosSombra = 0;
static int teclaMantenida = KEY_NULL, planos = 0, indicadores = 0;
static const char* rutaAusente = nullptr;
static Color originales[N][16]{};
static const Color colores[] = {{235,80,80,255},{80,140,240,255},
    {90,205,115,255},{245,205,70,255}};
struct DibujoRacimo
{
    Vector3 posicion{}, eje{}, escala{};
    float angulo = 0;
    Color color{};
    int malla = -1;
};
static DibujoRacimo dibujos[N][24]{};
static auto& Recursos() { return ObtenerModelosEscenariosRetro3D().racimoToxico; }
static void Comprobar(bool valor, const char* mensaje)
{
    if (!valor) { if (errores < 20) std::fprintf(stderr,"FALLO: %s\n",mensaje); errores++; }
}
static bool Cerca(float a, float b) { return std::fabs(a-b) < .002f; }
static bool Igual(Vector3 a, Vector3 b)
{ return Cerca(a.x,b.x) && Cerca(a.y,b.y) && Cerca(a.z,b.z); }
static int Indice(const char* ruta)
{
    for (int i=0;i<N;i++) if (std::strcmp(ruta,RUTAS_MODELOS_RACIMO_TOXICO_3D[i])==0) return i;
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
        Color color=r.materialColor<0 ? WHITE : m.materials[r.materialColor].maps[MATERIAL_MAP_DIFFUSE].color;
        Comprobar(n<24,"Limite de instancias");
        if (n<24) dibujos[i][n]={p,eje,escala,angulo,color,malla};
        Comprobar(ColorIsEqual(tinte,WHITE),"Sin tinte global de materiales y vertices");
        Matrix identidad=MatrixIdentity();
        Comprobar(std::memcmp(&m.transform,&identidad,sizeof(Matrix))==0,"Pivotes originales sin centrado");
        for (int j=0;j<m.materialCount && j<16;j++) if (j!=r.materialColor)
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
    auto& m=zona.gestorMinijuegos.minijuegoRacimoToxico;
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
static Vector3 PosicionFruto(int i,float tiempo)
{ return {(i%2==0 ? -.36f : .36f)+std::sin(tiempo*1.5f+i)*.03f,1.5f+i*.42f,0}; }
static ModeloRacimoToxico3D PiezaFruto(int tipo)
{
    return tipo==FRUTA_RACIMO_TOXICA ? MODELO_RACIMO_FRUTO_TOXICO :
        tipo==FRUTA_RACIMO_DORADA ? MODELO_RACIMO_FRUTO_DORADO : MODELO_RACIMO_FRUTO_NORMAL;
}
static bool Dibujado(ModeloRacimoToxico3D pieza,Vector3 p)
{
    for (int i=0;i<cantidades[pieza] && i<24;i++) if (Igual(p,dibujos[pieza][i].posicion)) return true;
    return false;
}
static void TiposDeterministas(MinijuegoRacimoToxico& m)
{
    for (int i=0;i<TOTAL_FRUTAS_RACIMO;i++) m.tipoFruta[i]=FRUTA_RACIMO_NORMAL;
    for (int i : {5,11,17}) m.tipoFruta[i]=FRUTA_RACIMO_TOXICA;
    m.tipoFruta[14]=FRUTA_RACIMO_DORADA;
}
static void ComprobarFrutos(const MinijuegoRacimoToxico& m)
{
    int esperados[3]{};
    for (int i=m.frente;i<TOTAL_FRUTAS_RACIMO;i++)
    {
        esperados[m.tipoFruta[i]]++;
        Comprobar(Dibujado(PiezaFruto(m.tipoFruta[i]),PosicionFruto(i,m.tiempoAnimacion)),"Fruto visible segun frente, tipo y vaiven");
    }
    for (const auto& v:m.vuelos) if (v.activo)
    {
        esperados[v.tipo]++;
        Vector3 destino={m.posicionBalsaX[v.jugador],2,3.5f};
        Vector3 p=Vector3Lerp(v.origen,destino,v.progreso);
        p.y+=std::sin(v.progreso*PI)*1.2f;
        Comprobar(Dibujado(PiezaFruto(v.tipo),p),"Fruto en vuelo sigue arco y destinatario reales");
    }
    for (int tipo=0;tipo<3;tipo++)
        Comprobar(cantidades[PiezaFruto(tipo)]==esperados[tipo],"Solo frutos restantes y vuelos, sin duplicar superficies");
}
static void ComprobarBalsas(const MinijuegoRacimoToxico& m,int cantidad)
{
    Comprobar(cantidades[MODELO_RACIMO_BALSA]==cantidad,"Una balsa por participante, incluida la hundida");
    for (int i=0;i<cantidad;i++)
    {
        float x=(i-(cantidad-1)*.5f)*5;
        float temblor=m.estadosJugadores[i].sacudida>0 ? std::sin(m.tiempoAnimacion*50)*.12f : 0;
        float y=.08f+(m.estadosJugadores[i].vivo ? 0 : -.55f)+std::sin(m.tiempoAnimacion*2+x)*.04f;
        Comprobar(Cerca(m.posicionBalsaX[i],x) && Igual(dibujos[MODELO_RACIMO_BALSA][i].posicion,{x+temblor,y,3.5f}),"Separacion, temblor, bamboleo y hundimiento originales");
        Comprobar(ColorIsEqual(dibujos[MODELO_RACIMO_BALSA][i].color,WHITE),"Madera estatica sin tinte de estado");
    }
    bool turno=m.fase==FASE_RACIMO_TURNO;
    Comprobar(cantidades[MODELO_RACIMO_ARO]==(turno ? 1 : 0),"Aro visible exclusivamente durante turno");
    if (turno)
    {
        const auto& d=dibujos[MODELO_RACIMO_ARO][0];
        Comprobar(Igual(d.posicion,{m.posicionBalsaX[m.turno],.02f,3.5f}) &&
            ColorIsEqual(d.color,Fade(WHITE,.5f+.3f*std::sin(m.tiempoAnimacion*6))),"Aro del jugador actual con pulso en BOMBILLAS");
    }
    for (int i=0;i<16;i++)
    {
        float t=m.tiempoAnimacion;
        Vector3 p={-12.0f+(i*37%24)+std::sin(t*.7f+i)*.8f,
            .9f+(i*7%40)/5.f+std::sin(t*1.1f+i*2)*.4f,-7.0f+(i*13%12)};
        Comprobar(Igual(dibujos[MODELO_RACIMO_LUCIERNAGA][i].posicion,p) &&
            ColorIsEqual(dibujos[MODELO_RACIMO_LUCIERNAGA][i].color,Fade(WHITE,.4f+.6f*(.5f+.5f*std::sin(t*2.5f+i*1.7f)))),"Luciernagas conservan vuelo y brillo por instancia");
    }
}
static void EsperarTurno(ZonaPruebas& zona)
{
    auto& m=zona.gestorMinijuegos.minijuegoRacimoToxico;
    int frames=0; while (m.fase==FASE_RACIMO_ANIMACION && frames++<100) Avanzar(zona,1);
    Comprobar(m.fase==FASE_RACIMO_TURNO || m.fase==FASE_RACIMO_TERMINADO,"Animacion termina en turno o final");
}
static void Tomar(ZonaPruebas& zona,int jugador,int frente,int cantidad)
{
    auto& m=zona.gestorMinijuegos.minijuegoRacimoToxico;
    m.turno=jugador; m.frente=frente; m.fase=FASE_RACIMO_TURNO; m.tiempoTurno=5;
    teclaMantenida=cantidad==2 ? KEY_D : KEY_A; Avanzar(zona,1); teclaMantenida=KEY_NULL;
    Comprobar(m.seleccion==cantidad,"Seleccion mediante entrada real de teclado");
    Dibujar(zona); Comprobar(indicadores==cantidad,"Conservar indicadores de cantidad seleccionada");
    Pulsar(zona,KEY_E);
    Comprobar(m.fase==FASE_RACIMO_ANIMACION && m.frente==frente+cantidad,"Confirmacion real toma uno o dos frutos");
}
int main()
{
    SetTraceLogCallback(Registrar); SetTraceLogLevel(LOG_WARNING);
    SetConfigFlags(FLAG_WINDOW_HIDDEN); InitWindow(1280,800,"Verificacion integrada Racimo Toxico");
    if (!IsWindowReady()) return 2;
    SetExitKey(KEY_NULL); SetRandomSeed(821);
    static ZonaPruebas zona; Participante participantes[MAX_PARTICIPANTES]{};
    Mesh* mallas[N]{};
    for (int cantidad=2;cantidad<=4;cantidad++)
    {
        for (int i=0;i<MAX_PARTICIPANTES;i++)
        {
            participantes[i]={}; participantes[i].activo=i<cantidad;
            participantes[i].conectado=true; participantes[i].control=CONTROL_TECLADO_WASD;
            participantes[i].numeroJugador=i+1; participantes[i].color=colores[i];
        }
        zona.Inicializar(participantes,cantidad); zona.modoCatalogo=true;
        if (cantidad==2) for (const auto& r:Recursos()) Comprobar(!r.cargaIntentada,"Carga diferida hasta activar Racimo");
        zona.CambiarMinijuego(MINIJUEGO_RACIMO_TOXICO);
        auto& m=zona.gestorMinijuegos.minijuegoRacimoToxico;
        for (int i=0;i<N;i++)
        {
            auto& r=Recursos()[i]; Comprobar(r.cargado,RUTAS_MODELOS_RACIMO_TOXICO_3D[i]);
            if (!r.cargado) { zona.Descargar(); DescargarModeloJugadorCompartido(); CloseWindow(); return 2; }
            if (cantidad==2) mallas[i]=r.modelo.meshes;
            Comprobar(cargas[i]==1 && r.modelo.meshes==mallas[i],"Instancias comparten recursos");
            bool bombillas=i==MODELO_RACIMO_ARO || i==MODELO_RACIMO_LUCIERNAGA;
            Comprobar((r.materialColor>=0)==bombillas,"Solo las dos BOMBILLAS de estado son dinamicas");
            for (int j=0;j<r.modelo.meshCount;j++) Comprobar(r.modelo.meshes[j].colors!=nullptr,"Colores de vertices conservados");
        }
        GuardarMateriales(); TiposDeterministas(m);
        Dibujar(zona,cantidad==4 ? "build/racimo-preparacion.png" : nullptr);
        const int esperados[]={1,1,1,18,3,1,cantidad,3,2,7,16,4,4,0};
        for (int i=0;i<N;i++) Comprobar(cantidades[i]==esperados[i],"Todos los modelos sin primitivas duplicadas");
        // Racimo mantiene enSuelo=false: la sombra existente viene del
        // wrapper del modelo de jugador, ocho triangulos y ningun cilindro.
        Comprobar(cubos==2 && planos==0 && esferas==20 && segmentos==6 && cilindros==0 && triangulosSombra==8*cantidad,
            "Conservar niebla, halos, lianas animadas y sombras de jugadores");
        int sombrasJugadores=triangulosSombra;
        ComprobarFrutos(m); ComprobarBalsas(m,cantidad);
        const Vector3 inicios[]={{-9,.1f,0},{6.5f,.1f,-2},{2,.1f,-5}};
        const Vector3 finales[]={{-6,.1f,-1},{10,.1f,-1},{5,.1f,-6}};
        for (int i=0;i<3;i++)
        {
            const auto& d=dibujos[MODELO_RACIMO_TRONCO][i];
            Vector3 extremo=Vector3Add(d.posicion,Vector3Transform({0,0,d.escala.z},MatrixRotate(d.eje,d.angulo*DEG2RAD)));
            Comprobar(Igual(d.posicion,inicios[i]) && Igual(extremo,finales[i]) && Cerca(d.escala.x,1) && Cerca(d.escala.y,1),"Tronco conserva extremos, pivote +Z y escala solo longitudinal");
        }
        Comprobar(Igual(dibujos[MODELO_RACIMO_ARBOL_PODRIDO][0].posicion,{-4.8f,0,-1}) &&
            Igual(dibujos[MODELO_RACIMO_CABANA][0].posicion,{-10.5f,0,-8}) &&
            Igual(dibujos[MODELO_RACIMO_CABANA][1].posicion,{9.5f,0,-9}),"Posiciones del pantano original");
        Avanzar(zona,182); Comprobar(m.fase==FASE_RACIMO_TURNO,"Preparacion real completa");
        Tomar(zona,0,0,2);
        Comprobar(m.estadosJugadores[0].frutasSeguras==2 && m.ultimoEvento==EVENTO_RACIMO_SEGURO && m.vuelos[0].activo && m.vuelos[1].activo,"Dos frutos seguros crean dos vuelos");
        Avanzar(zona,27); Dibujar(zona,cantidad==4 ? "build/racimo-vuelos.png" : nullptr);
        ComprobarFrutos(m); ComprobarBalsas(m,cantidad);
        Comprobar(triangulosSombra==sombrasJugadores,"Halos, niebla y GLB no agregan manchas de personaje sobre el agua");
        EsperarTurno(zona); Comprobar(m.turno==1 && !m.vuelos[0].activo && !m.vuelos[1].activo,"Rotacion de turno y fin del vuelo");
        Tomar(zona,0,5,1);
        Comprobar(m.estadosJugadores[0].vidas==1 && m.ultimoEvento==EVENTO_RACIMO_TOXICA && m.estadosJugadores[0].sacudida>0,"Veneno resta una vida y sacude la balsa");
        Avanzar(zona,10); Dibujar(zona,cantidad==4 ? "build/racimo-veneno.png" : nullptr);
        ComprobarFrutos(m); ComprobarBalsas(m,cantidad); EsperarTurno(zona);
        Tomar(zona,0,14,1);
        Comprobar(m.ultimoEvento==EVENTO_RACIMO_DORADA && m.saltarSiguiente,"Dorado conserva salto de turno");
        Avanzar(zona,27); Dibujar(zona,cantidad==4 ? "build/racimo-dorado.png" : nullptr); ComprobarFrutos(m);
        EsperarTurno(zona); Comprobar(m.jugadorSaltado==1 && m.turno==(cantidad==2 ? 0 : 2),"Saltar al siguiente jugador vivo");
        // Timeout en el ultimo fruto, aunque la seleccion anterior fuera dos.
        m.turno=1; m.frente=21; m.tipoFruta[21]=FRUTA_RACIMO_NORMAL; m.seleccion=2; m.tiempoTurno=.001f;
        Avanzar(zona,1); Comprobar(m.frente==22 && m.vuelos[0].activo && !m.vuelos[1].activo,"Timeout toma solo el ultimo fruto");
        EsperarTurno(zona); Comprobar(m.frente==0 && m.racimoRepuesto,"Reposicion real del racimo");
        Dibujar(zona); ComprobarFrutos(m);
        participantes[1].esBot=true; m.turno=1; m.tiempoBot=.001f;
        Avanzar(zona,1); Comprobar(m.fase==FASE_RACIMO_ANIMACION && m.frente>=1 && m.frente<=2,"Bot toma segun IA original");
        EsperarTurno(zona);
        participantes[1].esBot=false; participantes[1].control=CONTROL_GAMEPAD; participantes[1].indiceGamepad=-1;
        m.turno=1; m.tiempoBot=.001f; int frenteAntes=m.frente;
        Avanzar(zona,1); Comprobar(!participantes[1].conectado && m.fase==FASE_RACIMO_ANIMACION && m.frente>frenteAntes,"Mando ausente conserva IA de respaldo");
        EsperarTurno(zona); participantes[1].control=CONTROL_TECLADO_WASD;
        Tomar(zona,0,11,1);
        Comprobar(!m.estadosJugadores[0].vivo && m.estadosJugadores[0].vidas==0 &&
            m.estadosJugadores[0].posicionEliminacion==cantidad && m.ultimoEvento==EVENTO_RACIMO_ELIMINADO,"Segunda baya elimina y conserva clasificacion");
        Dibujar(zona,cantidad==4 ? "build/racimo-eliminacion.png" : nullptr); ComprobarBalsas(m,cantidad);
        EsperarTurno(zona);
        for (int i=1;i<cantidad;i++) participantes[i].esBot=true;
        int frames=0; while (m.fase!=FASE_RACIMO_TERMINADO && frames++<5000) Avanzar(zona,1);
        Comprobar(m.fase==FASE_RACIMO_TERMINADO && ResultadoMinijuegoFinalizado(m.resultado),"Partida real termina por vidas o tiempo");
        Comprobar(m.resultado.participantes[0].posicionFinal==cantidad,"Posicion del eliminado preservada al final");
        Dibujar(zona,cantidad==4 ? "build/racimo-final.png" : nullptr);
        Pulsar(zona,KEY_R); Comprobar(m.fase==FASE_RACIMO_PREPARACION && m.frente==0 && m.estadosJugadores[0].vivo,"R reinicia estado y conserva modelos");
        Pulsar(zona,KEY_ESCAPE); Comprobar(zona.volverAlMenu,"Salida al selector/menu");
        zona.Inicializar(participantes,cantidad); zona.modoCatalogo=zona.modoTablero=true;
        zona.CambiarMinijuego(MINIJUEGO_RACIMO_TOXICO); Pulsar(zona,KEY_R); Pulsar(zona,KEY_ESCAPE);
        Comprobar(m.tiempoPreparacion<3 && !zona.volverAlMenu,"R y Escape no interrumpen ronda de tablero");
        for (int i=0;i<N;i++) Comprobar(cargas[i]==1 && mallas[i]==Recursos()[i].modelo.meshes,"Reinicio, reposicion y reentrada sin duplicar recursos");
    }
    auto& m=zona.gestorMinijuegos.minijuegoRacimoToxico;
    m.fase=FASE_RACIMO_TURNO; m.turno=0; m.frente=0; TiposDeterministas(m);
    Dibujar(zona); int presentes[N]; std::memcpy(presentes,cantidades,sizeof(presentes));
    int baseCubos=cubos,baseEsferas=esferas,baseCilindros=cilindros,baseSegmentos=segmentos,basePlanos=planos;
    for (int ausente=0;ausente<N;ausente++)
    {
        DescargarSlotModeloEscenarioRetro3D(Recursos()[ausente]); rutaAusente=RUTAS_MODELOS_RACIMO_TOXICO_3D[ausente];
        for (int j=0;j<3;j++) CargarPaqueteModelosEscenarioRetro3D(ObtenerPaqueteRacimoToxicoRetro3D());
        Dibujar(zona);
        for (int i=0;i<N;i++) Comprobar(cantidades[i]==(i==ausente ? 0 : presentes[i]),"Fallback independiente por pieza");
        Comprobar(cubos>baseCubos || esferas>baseEsferas || cilindros>baseCilindros || segmentos>baseSegmentos || planos>basePlanos,"Primitivas reemplazan la pieza fallida");
        Comprobar(Recursos()[ausente].cargaIntentada && !Recursos()[ausente].cargado,"Fallo recordado sin cargas por frame");
        rutaAusente=nullptr; Recursos()[ausente]={};
        CargarPaqueteModelosEscenarioRetro3D(ObtenerPaqueteRacimoToxicoRetro3D()); GuardarMateriales();
    }
    Comprobar(avisos==N,"Un diagnostico por recurso ausente");
    const auto& almacen=ObtenerModelosEscenariosRetro3D();
    for (const auto& r:almacen.ultimoAsiento) Comprobar(!r.cargaIntentada,"No cargar Asiento");
    for (const auto& r:almacen.cajasPuerto) Comprobar(!r.cargaIntentada,"No cargar Puerto");
    for (const auto& r:almacen.laberintoJade) Comprobar(!r.cargaIntentada,"No cargar Jade");
    for (const auto& r:almacen.vetaCristal) Comprobar(!r.cargaIntentada,"No cargar Veta");
    for (const auto& r:almacen.capsulasBarajadas) Comprobar(!r.cargaIntentada,"No cargar Capsulas");
    for (const auto& r:almacen.bateoMeteorico) Comprobar(!r.cargaIntentada,"No cargar Bateo");
    zona.Descargar(); zona.Descargar();
    for (int i=0;i<N;i++) Comprobar(cargas[i]==descargas[i] && !Recursos()[i].modelo.meshes,"Descarga simetrica antes de cerrar contexto grafico");
    DescargarModeloJugadorCompartido(); CloseWindow();
    std::printf("Verificacion Racimo Toxico 2/3/4 participantes: %d errores\n",errores);
    return errores==0 ? 0 : 1;
}
