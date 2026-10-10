// ZonaPruebas y minijuego reales, con OpenGL y entradas simuladas.
#include "Gameplay/ZonaPruebas.h"
#include "Minigames/ModelosEscenariosRetro3D.h"
#include "Minigames/ModeloJugadorCompartido.h"
#include "rlgl.h"
#include <cmath>
#include <cstdio>
#include <cstring>
#include <cstdarg>

static constexpr int N = CANTIDAD_MODELOS_GRUA_CHATARRA_3D;
static int errores = 0, tecla = KEY_NULL, tecla2 = KEY_NULL, avisos = 0;
static int cargas[N]{}, descargas[N]{}, cantidades[N]{}, usos[N]{};
static int primitivas = 0, cubos = 0, cables = 0;
static const char* ausente = nullptr;
static Color originales[N][16]{};
static unsigned long long firmas[N]{};
static float tiempoDibujo = 0;
static const Color colores[] = {{235,80,80,255},{80,140,240,255},{90,205,115,255},{245,205,70,255}};
struct InstanciaGrua { Vector3 posicion{}, eje{}, escala{}; float angulo = 0; Color color{}; int malla = -1; };
static InstanciaGrua dibujos[N][128]{};
static Vector3 cableDesde[4]{}, cableHasta[4]{};
static auto& Recursos() { return ObtenerModelosEscenariosRetro3D().gruaChatarra; }
static void Comprobar(bool ok, const char* texto)
{ if (!ok) { if (errores < 30) std::fprintf(stderr,"FALLO: %s\n",texto); errores++; } }
static bool Cerca(float a, float b) { return std::fabs(a-b) < .002f; }
static bool Igual(Vector3 a, Vector3 b) { return Cerca(a.x,b.x) && Cerca(a.y,b.y) && Cerca(a.z,b.z); }
static int Indice(const char* ruta)
{ for (int i=0;i<N;i++) if (std::strcmp(ruta,RUTAS_MODELOS_GRUA_CHATARRA_3D[i])==0) return i; return -1; }
static unsigned long long Firma(Model modelo)
{
    unsigned long long h = 1469598103934665603ULL;
    for (int i=0;i<modelo.meshCount;i++)
    {
        Mesh& m = modelo.meshes[i];
        const unsigned char* vertices = reinterpret_cast<const unsigned char*>(m.vertices);
        for (int b=0;b<m.vertexCount*3*(int)sizeof(float);b++) h=(h^vertices[b])*1099511628211ULL;
        if (m.colors) for (int b=0;b<m.vertexCount*4;b++) h=(h^m.colors[b])*1099511628211ULL;
    }
    return h;
}
static void Registrar(int nivel, const char* formato, va_list args)
{
    char texto[2048]; std::vsnprintf(texto,sizeof(texto),formato,args);
    if (std::strstr(texto,"Modelo de escenario ausente")) avisos++;
    if (nivel>=LOG_WARNING) std::fprintf(stderr,"%s\n",texto);
}
extern "C" bool __real_IsKeyPressed(int);
extern "C" bool __wrap_IsKeyPressed(int k) { return k==tecla || k==tecla2 || __real_IsKeyPressed(k); }
extern "C" bool __real_IsKeyDown(int);
extern "C" bool __wrap_IsKeyDown(int k) { return k==tecla || k==tecla2 || __real_IsKeyDown(k); }
extern "C" bool __real_FileExists(const char*);
extern "C" bool __wrap_FileExists(const char* p)
{ return !(ausente && std::strcmp(p,ausente)==0) && __real_FileExists(p); }
extern "C" Model __real_LoadModel(const char*);
extern "C" Model __wrap_LoadModel(const char* p)
{ int i=Indice(p); if (i>=0) cargas[i]++; return __real_LoadModel(p); }
extern "C" void __real_UnloadModel(Model);
extern "C" void __wrap_UnloadModel(Model modelo)
{ for (int i=0;i<N;i++) if (modelo.meshes && modelo.meshes==Recursos()[i].modelo.meshes) descargas[i]++; __real_UnloadModel(modelo); }
extern "C" void __real_DrawModelEx(Model,Vector3,Vector3,float,Vector3,Color);
extern "C" void __wrap_DrawModelEx(Model modelo,Vector3 p,Vector3 eje,float giro,Vector3 escala,Color tinte)
{
    for (int i=0;i<N;i++) if (Recursos()[i].cargado)
    {
        auto& r=Recursos()[i]; int malla=-1;
        for (int j=0;j<r.modelo.meshCount;j++) if (modelo.meshes==&r.modelo.meshes[j]) malla=j;
        if (malla<0) continue;
        int n=cantidades[i]++; usos[i]++;
        int material=r.materialColor>=0 ? r.materialColor : modelo.meshMaterial[0];
        if (n<128) dibujos[i][n]={Vector3Transform(p,rlGetMatrixTransform()),eje,escala,giro,
            modelo.materials[material].maps[MATERIAL_MAP_DIFFUSE].color,malla};
        Comprobar(ColorIsEqual(tinte,WHITE),"Sin tinte global ni cambio de colores de vertice");
        Matrix identidad=MatrixIdentity();
        Comprobar(std::memcmp(&modelo.transform,&identidad,sizeof(Matrix))==0,"Pivotes originales, sin normalizacion");
        for (int j=0;j<modelo.materialCount;j++) if (j!=r.materialColor)
            Comprobar(ColorIsEqual(modelo.materials[j].maps[MATERIAL_MAP_DIFFUSE].color,originales[i][j]),"Herrajes y materiales originales intactos durante el dibujo");
        if (i==MODELO_GRUA_SUELO) Comprobar(malla<3,"Sin escombro incorporado superpuesto");
        if (i==MODELO_GRUA_CINTA)
        {
            int lado=p.x<0 ? -1 : 1;
            const auto& mesh=r.modelo.meshes[3];
            const float* reposo=ObtenerModelosEscenariosRetro3D().franjasCintaGrua;
            for (int v=0;v<mesh.vertexCount;v++)
            {
                int k=v/36;
                float z=-2.2f+std::fmod(k+tiempoDibujo*1.2f*lado,8.0f);
                Comprobar(Cerca(mesh.vertices[v*3+2]+p.z,z+reposo[v*3+2]-(-3.5f+k)),"Franjas siguen el recorrido procedural por lado");
                Comprobar(Cerca(mesh.vertices[v*3],reposo[v*3]) && Cerca(mesh.vertices[v*3+1],reposo[v*3+1]),"La cinta anima solo Z de las franjas");
            }
        }
        break;
    }
    __real_DrawModelEx(modelo,p,eje,giro,escala,tinte);
}
extern "C" void __real_DrawCube(Vector3,float,float,float,Color);
extern "C" void __wrap_DrawCube(Vector3 p,float x,float y,float z,Color c)
{ primitivas++; cubos++; __real_DrawCube(p,x,y,z,c); }
extern "C" void __real_DrawSphere(Vector3,float,Color);
extern "C" void __wrap_DrawSphere(Vector3 p,float r,Color c)
{ primitivas++; __real_DrawSphere(p,r,c); }
extern "C" void __real_DrawCylinder(Vector3,float,float,float,int,Color);
extern "C" void __wrap_DrawCylinder(Vector3 p,float a,float b,float h,int n,Color c)
{ primitivas++; __real_DrawCylinder(p,a,b,h,n,c); }
extern "C" void __real_DrawCylinderEx(Vector3,Vector3,float,float,int,Color);
extern "C" void __wrap_DrawCylinderEx(Vector3 p,Vector3 q,float a,float b,int n,Color c)
{ primitivas++; __real_DrawCylinderEx(p,q,a,b,n,c); }
extern "C" void __real_DrawCircle3D(Vector3,float,Vector3,float,Color);
extern "C" void __wrap_DrawCircle3D(Vector3 p,float r,Vector3 eje,float giro,Color c)
{ primitivas++; __real_DrawCircle3D(p,r,eje,giro,c); }
extern "C" void __real_DrawLine3D(Vector3,Vector3,Color);
extern "C" void __wrap_DrawLine3D(Vector3 p,Vector3 q,Color c)
{ if (cables<4) { cableDesde[cables]=p; cableHasta[cables]=q; } cables++; __real_DrawLine3D(p,q,c); }
static void GuardarRecursos()
{
    for (int i=0;i<N;i++)
    {
        firmas[i]=Firma(Recursos()[i].modelo);
        for (int j=0;j<Recursos()[i].modelo.materialCount;j++) originales[i][j]=Recursos()[i].modelo.materials[j].maps[MATERIAL_MAP_DIFFUSE].color;
    }
}
static void Dibujar(ZonaPruebas& zona, const char* captura=nullptr)
{
    auto& m=zona.gestorMinijuegos.minijuegoGruaChatarra;
    unsigned char estado[sizeof(m)], jugadores[sizeof(zona.jugadores)];
    std::memcpy(estado,&m,sizeof(m)); std::memcpy(jugadores,zona.jugadores,sizeof(jugadores));
    std::memset(cantidades,0,sizeof(cantidades)); primitivas=cubos=cables=0; tiempoDibujo=m.tiempoAnimacion;
    BeginDrawing(); zona.Dibujar();
    if (captura) { rlDrawRenderBatchActive(); Image img=LoadImageFromScreen(); Comprobar(ExportImage(img,captura),"Captura del juego integrado"); UnloadImage(img); }
    EndDrawing();
    Comprobar(std::memcmp(estado,&m,sizeof(m))==0,"Dibujo conserva estados, tiempos, puntuacion y camara");
    Comprobar(std::memcmp(jugadores,zona.jugadores,sizeof(jugadores))==0,"Jugadores y hitboxes intactos");
    for (int i=0;i<N;i++) if (Recursos()[i].cargado)
    {
        Comprobar(firmas[i]==Firma(Recursos()[i].modelo),"Vertices restaurados y colores originales");
        for (int j=0;j<Recursos()[i].modelo.materialCount;j++)
            Comprobar(ColorIsEqual(originales[i][j],Recursos()[i].modelo.materials[j].maps[MATERIAL_MAP_DIFFUSE].color),"Material restaurado tras cada instancia");
    }
    int garra=0;
    for (int i=0;i<4;i++) if (m.estadosJugadores[i].participa)
    {
        auto& e=m.estadosJugadores[i];
        if (Recursos()[MODELO_GRUA_TOLVA].cargado)
            Comprobar(Igual(dibujos[MODELO_GRUA_TOLVA][garra].posicion,{(e.puesto-(m.cantidadPuestos-1)*.5f)*4.6f,0,-6.3f}) && ColorIsEqual(dibujos[MODELO_GRUA_TOLVA][garra].color,zona.participantes[i].color),"Tolva sigue puesto y color de participante");
        if (Recursos()[MODELO_GRUA_IMAN].cargado)
        {
            const auto& d=dibujos[MODELO_GRUA_IMAN][garra];
            Comprobar(Cerca(d.posicion.x,e.x) && Cerca(d.posicion.z,e.z) && ColorIsEqual(d.color,zona.participantes[i].color),"Iman sigue estado y color del jugador");
            Comprobar(Igual(cableDesde[garra],{e.x,8,e.z}) && Igual(cableHasta[garra],{e.x,d.posicion.y+.35f,e.z}),"Cable conectado a carro e iman durante toda la secuencia");
            if (Recursos()[MODELO_GRUA_PINZA].cargado)
            {
                float apertura=.6f;
                if (e.estado==GARRA_GRUA_ACCION && e.tiempo>=.55f)
                    apertura=e.tiempo<.8f ? .6f-.35f*(e.tiempo-.55f)/.25f : (e.tipoCapturado>=0 ? .25f : .25f+.35f*std::fmin((e.tiempo-.8f)/.45f,1.0f));
                for (int k=0;k<4;k++)
                {
                    float a=k*PI/2+PI/4; const auto& pinza=dibujos[MODELO_GRUA_PINZA][garra*4+k];
                    Comprobar(Igual(pinza.posicion,{e.x+std::cos(a)*apertura*.7f,d.posicion.y-.02f,e.z+std::sin(a)*apertura*.7f}) && Cerca(pinza.angulo,90-a*RAD2DEG),"Pinzas mantienen apertura, pivote superior y punta hacia dentro");
                }
            }
        }
        if (Recursos()[MODELO_GRUA_CARRO].cargado)
            Comprobar(Igual(dibujos[MODELO_GRUA_CARRO][garra].posicion,{e.x,8,e.z}),"Carro sigue X/Z durante la entrega");
        if (Recursos()[MODELO_GRUA_MARCA].cargado)
            Comprobar(Igual(dibujos[MODELO_GRUA_MARCA][garra].posicion,{e.x,0,e.z}) && ColorIsEqual(dibujos[MODELO_GRUA_MARCA][garra].color,Fade(zona.participantes[i].color,.9f)),"Marca y alpha siguen al jugador");
        garra++;
    }
    Comprobar(cables==garra,"Un cable por garra, sin duplicados");
}
static void Avanzar(ZonaPruebas& zona, int frames)
{ for (int i=0;i<frames;i++) zona.Actualizar(1.0f/60); }
static void Preparar(ZonaPruebas& zona)
{ tecla=tecla2=KEY_NULL; zona.CambiarMinijuego(MINIJUEGO_GRUA_CHATARRA); Avanzar(zona,181); }
static void Objetivo(ZonaPruebas& zona, int tipo)
{
    auto& m=zona.gestorMinijuegos.minijuegoGruaChatarra;
    for (auto& o:m.objetos) o={};
    m.objetos[0]={true,tipo,0,0,37,0};
    auto& e=m.estadosJugadores[0]; e.x=e.z=e.velocidadX=e.velocidadZ=0;
    tecla=KEY_E; Avanzar(zona,1); tecla=KEY_NULL;
}
int main()
{
    SetTraceLogCallback(Registrar); SetTraceLogLevel(LOG_WARNING);
    SetConfigFlags(FLAG_WINDOW_HIDDEN); InitWindow(1280,800,"Verificacion integrada Grua de Chatarra");
    if (!IsWindowReady()) return 2;
    Participante p[4]{}; ZonaPruebas zona; zona.Inicializar(p,4); zona.modoCatalogo=true;
    auto& m=zona.gestorMinijuegos.minijuegoGruaChatarra;
    p[0].activo=true; m.Reiniciar(zona.jugadores,p,4);
    Comprobar(m.resultado.estado==RESULTADO_MINIJUEGO_CANCELADO,"Un participante cancela la ronda");
    for (int i=0;i<N;i++) Comprobar(cargas[i]==0,"Inicializacion del catalogo y ronda invalida no cargan modelos");
    for (int cantidad=2;cantidad<=4;cantidad++)
    {
        for (int i=0;i<4;i++) { p[i].activo=i<cantidad; p[i].conectado=true; p[i].esBot=false; p[i].numeroJugador=i+1; p[i].color=colores[i]; p[i].control=i==0 ? CONTROL_TECLADO_WASD : CONTROL_TECLADO_FLECHAS; }
        zona.cantidadParticipantes=cantidad; zona.CambiarMinijuego(MINIJUEGO_GRUA_CHATARRA);
        for (int i=0;i<N;i++) Comprobar(Recursos()[i].cargado && cargas[i]==1,"22 modelos cargados una vez entre instancias y reinicios");
        GuardarRecursos();
        char ruta[100]; std::snprintf(ruta,sizeof(ruta),"build/grua-preparacion-%d.png",cantidad); Dibujar(zona,ruta);
        Comprobar(cubos==0 && cantidades[MODELO_GRUA_SUELO]==3 && cantidades[MODELO_GRUA_ESCOMBROS]==46,"Sin primitivas duplicadas ni escombros superpuestos");
        Comprobar(cantidades[MODELO_GRUA_CINTA]==2 && cantidades[MODELO_GRUA_PILA]==9 && cantidades[MODELO_GRUA_FOCO]==4 && cantidades[MODELO_GRUA_PINZA]==cantidad*4,"Mallas compartidas entre instancias");
        Avanzar(zona,181); Comprobar(m.fase==FASE_GRUA_JUGANDO,"Cuenta regresiva inicia ronda");
        float x=m.estadosJugadores[0].x; tecla=KEY_D; Avanzar(zona,20); tecla=KEY_NULL;
        Comprobar(m.estadosJugadores[0].x>x && m.estadosJugadores[0].velocidadX<=6,"Movimiento e inercia con controles reales");
        for (int tipo=0;tipo<CANTIDAD_TIPOS_OBJETO_GRUA;tipo++)
        {
            Preparar(zona); Objetivo(zona,tipo);
            Avanzar(zona,15); Dibujar(zona,cantidad==4 && tipo==OBJETO_GRUA_MOTOR ? "build/grua-bajada.png" : nullptr);
            Comprobar(Cerca(dibujos[MODELO_GRUA_IMAN][0].posicion.y,3.2f+( .75f-3.2f)*std::pow(m.estadosJugadores[0].tiempo/.55f,2)),"Altura sigue bajada cuadratica original");
            Avanzar(zona,35); auto& e=m.estadosJugadores[0];
            Comprobar(e.tipoCapturado==tipo && !m.objetos[0].activo,"Captura real retira objetivo del pozo");
            Dibujar(zona,cantidad==4 && tipo==OBJETO_GRUA_MOTOR ? "build/grua-captura.png" : nullptr);
            Comprobar(cantidades[MODELO_GRUA_TUERCA+tipo]==1,"Solo se dibuja el objeto capturado, sin copia en el pozo");
            Comprobar(Igual(dibujos[MODELO_GRUA_TUERCA+tipo][0].posicion,{e.x,dibujos[MODELO_GRUA_IMAN][0].posicion.y-.75f,e.z}) && Cerca(dibujos[MODELO_GRUA_TUERCA+tipo][0].angulo,m.tiempoAnimacion*40),"Objeto transportado sigue pivote, altura y giro procedural");
            if (tipo==OBJETO_GRUA_CARTUCHO)
            {
                Comprobar(!e.exito && e.mensaje==3 && e.puntos==0,"Cartucho penaliza sin puntaje negativo");
                Avanzar(zona,40); Dibujar(zona);
                Comprobar(cantidades[MODELO_GRUA_CARTUCHO]==0,"Cartucho deja de mostrarse al terminar accion");
            }
            else
            {
                Avanzar(zona,42); Dibujar(zona,cantidad==4 && tipo==OBJETO_GRUA_MOTOR ? "build/grua-transporte.png" : nullptr);
                Comprobar(e.exito && e.z<0 && !e.depositado,"Entrega viaja a la tolva usando el estado actual");
                Avanzar(zona,23); Dibujar(zona,cantidad==4 && tipo==OBJETO_GRUA_MOTOR ? "build/grua-entrega.png" : nullptr);
                const int valores[]={1,3,5,8};
                Comprobar(e.depositado && e.puntos==valores[tipo] && e.mejorObjeto==valores[tipo] && cantidades[MODELO_GRUA_TUERCA+tipo]==0,"Deposito puntua una sola vez y oculta el objeto");
                Avanzar(zona,45); Comprobar(e.estado==GARRA_GRUA_LIBRE && Cerca(e.x,0) && Cerca(e.z,0),"Retorno al origen al completar accion");
            }
            Avanzar(zona,200); Comprobar(m.objetos[0].activo,"Reaparicion del objetivo conserva temporizador");
        }
        Preparar(zona); for (auto& o:m.objetos) o={};
        tecla=KEY_E; Avanzar(zona,1); tecla=KEY_NULL; Avanzar(zona,50); Dibujar(zona);
        Comprobar(m.estadosJugadores[0].mensaje==-1 && !m.estadosJugadores[0].exito,"Intento vacio conserva fallo y apertura");
        Preparar(zona); for (auto& o:m.objetos) o={}; m.objetos[0]={true,OBJETO_GRUA_MOTOR,0,0,0,0};
        m.estadosJugadores[0].x=m.estadosJugadores[0].z=0;
        m.estadosJugadores[1].x=.4f; m.estadosJugadores[1].z=0;
        tecla=KEY_E; tecla2=KEY_RIGHT_SHIFT; Avanzar(zona,1); tecla=tecla2=KEY_NULL; Avanzar(zona,35); Dibujar(zona,cantidad==4 ? "build/grua-disputa.png" : nullptr);
        Comprobar(m.estadosJugadores[0].exito && m.estadosJugadores[1].disputaPerdida && m.estadosJugadores[1].mensaje==2,"Disputa favorece centrado y bloquea rival");
        m.tiempoRestante=.001f; Avanzar(zona,1); Dibujar(zona,cantidad==4 ? "build/grua-final.png" : nullptr);
        Comprobar(m.fase==FASE_GRUA_TERMINADO && m.resultado.estado==RESULTADO_MINIJUEGO_FINALIZADO,"Tiempo finaliza ronda y conserva resultado");
        tecla=KEY_R; Avanzar(zona,1); tecla=KEY_NULL;
        Comprobar(m.fase==FASE_GRUA_PREPARACION && m.estadosJugadores[0].puntos==0,"R reinicia puntuacion y secuencias");
        zona.modoTablero=true; tecla=KEY_R; Avanzar(zona,1); tecla=KEY_ESCAPE; Avanzar(zona,1); tecla=KEY_NULL;
        Comprobar(!zona.volverAlMenu && m.tiempoPreparacion<3,"Ronda oficial protege reinicio y salida");
        zona.modoTablero=false; tecla=KEY_ESCAPE; Avanzar(zona,1); tecla=KEY_NULL;
        Comprobar(zona.volverAlMenu,"Salida al selector en pruebas"); zona.volverAlMenu=false;
        for (int i=0;i<cantidad;i++) p[i].esBot=true;
        Preparar(zona); Avanzar(zona,3100);
        Comprobar(m.fase==FASE_GRUA_TERMINADO,"IA completa la partida sin bloquearse");
        int puntos=0; for (auto& e:m.estadosJugadores) puntos+=e.puntos;
        Comprobar(puntos>0,"Bots capturan y entregan objetos");
    }
    for (auto& participante:p) participante.esBot=false;
    Preparar(zona);
    for (int pieza=0;pieza<N;pieza++)
    {
        Dibujar(zona); int base=primitivas, previo[N]; std::memcpy(previo,cantidades,sizeof(previo));
        DescargarSlotModeloEscenarioRetro3D(Recursos()[pieza]); ausente=RUTAS_MODELOS_GRUA_CHATARRA_3D[pieza]; int avisosAntes=avisos;
        CargarPaqueteGruaChatarraRetro3D(); CargarPaqueteGruaChatarraRetro3D(); CargarPaqueteGruaChatarraRetro3D();
        Comprobar(avisos==avisosAntes+1,"Un diagnostico por archivo ausente, sin repetir por frame");
        Dibujar(zona); Comprobar(cantidades[pieza]==0 && primitivas>base,"Fallback por pieza, sin GLB simultaneo");
        for (int i=0;i<N;i++) if (i!=pieza) Comprobar(cantidades[i]==previo[i],"Un fallo conserva las otras piezas");
        ausente=nullptr; Recursos()[pieza]={}; CargarPaqueteGruaChatarraRetro3D(); GuardarRecursos();
    }
    for (int i=0;i<N;i++) Comprobar(usos[i]>0,"Los 22 modelos se usan en el juego real");
    zona.Descargar(); zona.Descargar();
    for (int i=0;i<N;i++) Comprobar(cargas[i]==descargas[i] && !Recursos()[i].modelo.meshes,"Descarga simetrica antes de CloseWindow");
    Comprobar(!ObtenerModelosEscenariosRetro3D().franjasCintaGrua,"Reposo CPU de la cinta liberado");
    DescargarModeloJugadorCompartido(); CloseWindow();
    std::printf("Verificacion Grua de Chatarra 2/3/4 participantes: %d errores\n",errores);
    return errores ? 1 : 0;
}
