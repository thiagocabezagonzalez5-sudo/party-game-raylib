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

static constexpr int N = CANTIDAD_MODELOS_TESORERO_CERCADO_3D;
static int errores = 0, tecla = KEY_NULL, avisos = 0;
static int cargas[N]{}, descargas[N]{}, cantidades[N]{};
static int cubos = 0, esferas = 0, cilindros = 0, segmentos = 0, circulos = 0;
static int triangulosSombra = 0;
static int teclaMantenida = KEY_NULL, planos = 0, indicadores = 0;
static const char* rutaAusente = nullptr;
static Color originales[N][16]{};
static const Color colores[] = {{235,80,80,255},{80,140,240,255},
    {90,205,115,255},{245,205,70,255}};
struct DibujoTesorero
{
    Vector3 posicion{}, eje{}, escala{};
    float angulo = 0;
    Color color{};
    int malla = -1;
};
static DibujoTesorero dibujos[N][64]{};
static auto& Recursos() { return ObtenerModelosEscenariosRetro3D().tesoreroCercado; }
static void Comprobar(bool valor, const char* mensaje)
{
    if (!valor) { if (errores < 20) std::fprintf(stderr,"FALLO: %s\n",mensaje); errores++; }
}
static bool Cerca(float a, float b) { return std::fabs(a-b) < .002f; }
static bool Igual(Vector3 a, Vector3 b)
{ return Cerca(a.x,b.x) && Cerca(a.y,b.y) && Cerca(a.z,b.z); }
static int Indice(const char* ruta)
{
    for (int i=0;i<N;i++) if (std::strcmp(ruta,RUTAS_MODELOS_TESORERO_CERCADO_3D[i])==0) return i;
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
        bool dinamico=(i==MODELO_TESORERO_REJA && (malla==0 || malla==2)) || i==MODELO_TESORERO_AVISO;
        for (int j=0;j<m.materialCount && j<16;j++) if (!dinamico || j!=materialActual)
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
    auto& m=zona.gestorMinijuegos.minijuegoTesoreroAcorralado;
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

static void ComprobarEscena(const MinijuegoTesoreroAcorralado& m)
{
    const int fijos[]={1,1,1,2,1,2,2,1,1,18,10,30,4,16};
    for (int i=0;i<14;i++) Comprobar(cantidades[i]==fijos[i],"Piezas completas o vistas compartidas sin duplicados");
    int avisosReja=0;
    for (int k=0;k<4;k++)
    {
        const auto& r=m.rejas[k]; float rotacion=k<2 ? 90.f : 0;
        const auto& marco=dibujos[MODELO_TESORERO_MARCO_REJA][k];
        Comprobar(Igual(marco.posicion,{r.x,0,r.z}) && Cerca(marco.angulo,rotacion),"Marco fijo en centro original, sin doblar transformaciones");
        for (int j=0;j<4;j++)
        {
            const auto& d=dibujos[MODELO_TESORERO_REJA][k*4+j];
            Comprobar(d.malla==j && Igual(d.posicion,{r.x,(1-r.altura)*2.6f,r.z}) && Cerca(d.angulo,rotacion),"Hoja movil sigue altura y orientacion de reja");
            Color esperado=originales[MODELO_TESORERO_REJA][Recursos()[MODELO_TESORERO_REJA].modelo.meshMaterial[j]];
            if (r.estado==REJA_TESORERO_AVISO && std::sin(m.tiempoAnimacion*18)>0 && (j==0 || j==2)) esperado={230,60,50,255};
            Comprobar(ColorIsEqual(d.color,esperado),"Solo barras y travesanos parpadean; herrajes intactos");
        }
        if (r.estado!=REJA_TESORERO_AVISO) continue;
        for (int j=0;j<3;j++)
        {
            const auto& d=dibujos[MODELO_TESORERO_AVISO][avisosReja*3+j];
            Color original=originales[MODELO_TESORERO_AVISO][Recursos()[MODELO_TESORERO_AVISO].modelo.meshMaterial[j]];
            Comprobar(Igual(d.posicion,{r.x,.03f,r.z}) && Cerca(d.angulo,rotacion) &&
                ColorIsEqual(d.color,Fade(original,.25f+.3f*(.5f+.5f*std::sin(m.tiempoAnimacion*18)))),"Aviso conserva RGB y pulsa solo alpha");
        }
        avisosReja++;
    }
    Comprobar(cantidades[MODELO_TESORERO_AVISO]==avisosReja*3,"Avisos solo durante estado AVISO");
    int rojo=0,dorado=0;
    for (int k=0;k<5;k++)
    {
        int pieza=k%2 ? MODELO_TESORERO_ESTANDARTE_DORADO : MODELO_TESORERO_ESTANDARTE_ROJO;
        int& instancia=k%2 ? dorado : rojo;
        for (int j=0;j<Recursos()[pieza].modelo.meshCount;j++)
        {
            bool movil=j>=3 || (k%2 && j==1);
            float x=-7.2f+k*3.6f+(movil ? std::sin(m.tiempoAnimacion*2+k)*.12f : 0);
            Comprobar(Igual(dibujos[pieza][instancia++].posicion,{x,2.8f,-7.2f}),"Vaiven de tela con asta y travesano fijos");
        }
    }
    for (int k=0;k<6;k++) for (int j=0;j<5;j++)
    {
        float escala=j>=3 ? (.17f+.04f*std::sin(m.tiempoAnimacion*12+k*2))/.17f : 1;
        const auto& d=dibujos[MODELO_TESORERO_ANTORCHA][k*5+j];
        Comprobar(Igual(d.escala,{escala,escala,escala}) && Igual(d.posicion,{k%2 ? 8.8f : -8.8f,1.84f*(1-escala),-4.5f+(k/2)*4.5f}),"Soporte fijo y llama pulsa alrededor de su centro local");
    }
    int visibles=0;
    for (const auto& moneda:m.monedas)
    {
        if (!moneda.activa || (moneda.edad>7 && std::sin(m.tiempoAnimacion*18)<0)) continue;
        const auto& d=dibujos[MODELO_TESORERO_MONEDA][visibles++];
        Comprobar(Igual(d.posicion,{moneda.x,moneda.y+.12f,moneda.z}),"Moneda sigue posicion y centro originales");
        float angulo=m.tiempoAnimacion*6+moneda.x*3;
        Vector3 normal=Vector3Transform({0,0,1},MatrixRotateY(d.angulo*DEG2RAD));
        Comprobar(Igual(normal,{std::cos(angulo),0,std::sin(angulo)}) && Igual(d.escala,{1,1,1}),"Giro original y radio de moneda .2 sin normalizar pivote");
    }
    Comprobar(cantidades[MODELO_TESORERO_MONEDA]==visibles,"Solo monedas activas y visibles por expiracion");
}
static void AlejarJugadores(ZonaPruebas& zona,int cantidad)
{
    const Vector3 posiciones[]={{-7,.7f,-5},{7,.7f,5},{-7,.7f,5},{7,.7f,-5}};
    for (int i=0;i<cantidad;i++)
    {
        auto& j=zona.jugadores[i]; j.posicion=posiciones[i]; j.velocidad=j.empuje={};
        j.enSuelo=true; j.aplastado=j.cayendo=j.golpeando=false; j.tiempoInmunidad=0;
    }
}
static int MonedasActivas(const MinijuegoTesoreroAcorralado& m)
{ int n=0; for (const auto& moneda:m.monedas) if (moneda.activa) n++; return n; }
int main()
{
    SetTraceLogCallback(Registrar); SetTraceLogLevel(LOG_WARNING);
    SetConfigFlags(FLAG_WINDOW_HIDDEN); InitWindow(1280,800,"Verificacion integrada Tesorero Acorralado");
    if (!IsWindowReady()) return 2;
    SetExitKey(KEY_NULL); SetRandomSeed(973);
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
            for (const auto& r:Recursos()) Comprobar(!r.cargaIntentada,"No cargar paquete al arrancar");
            participantes[1].activo=false; zona.CambiarMinijuego(MINIJUEGO_TESORERO_ACORRALADO);
            Comprobar(zona.gestorMinijuegos.minijuegoTesoreroAcorralado.resultado.estado==RESULTADO_MINIJUEGO_CANCELADO,"Conservar rechazo de un solo participante");
            for (const auto& r:Recursos()) Comprobar(!r.cargaIntentada,"No cargar paquete para ronda invalida");
            participantes[1].activo=true;
        }
        zona.CambiarMinijuego(MINIJUEGO_TESORERO_ACORRALADO);
        auto& m=zona.gestorMinijuegos.minijuegoTesoreroAcorralado;
        for (int i=0;i<N;i++)
        {
            auto& r=Recursos()[i]; Comprobar(r.cargado,RUTAS_MODELOS_TESORERO_CERCADO_3D[i]);
            if (!r.cargado) { zona.Descargar(); DescargarModeloJugadorCompartido(); CloseWindow(); return 2; }
            if (cantidad==2) mallas[i]=r.modelo.meshes;
            Comprobar(cargas[i]==1 && r.modelo.meshes==mallas[i],"Carga unica y mallas compartidas");
            for (int j=0;j<r.modelo.meshCount;j++) Comprobar(r.modelo.meshes[j].colors!=nullptr,"Conservar colores de vertices");
        }
        GuardarMateriales(); Dibujar(zona,cantidad==4 ? "build/tesorero-preparacion.png" : nullptr);
        ComprobarEscena(m);
        Comprobar(esferas==8 && cilindros==cantidad && segmentos==0 && triangulosSombra==8*cantidad,"Halos y bolsa conservados sin sombras falsas del escenario");
        Avanzar(zona,182); Comprobar(m.fase==FASE_TESORERO_JUGANDO,"Preparacion y comienzo reales");
        AlejarJugadores(zona,cantidad);
        m.rejas[0].estado=REJA_TESORERO_ABIERTA; m.rejas[0].tiempoEstado=.001f;
        Avanzar(zona,1); Comprobar(m.rejas[0].estado==REJA_TESORERO_AVISO,"Aviso real antes del descenso");
        Dibujar(zona,cantidad==4 ? "build/tesorero-aviso.png" : nullptr); ComprobarEscena(m);
        Avanzar(zona,60); Comprobar(m.rejas[0].estado==REJA_TESORERO_CERRADA && m.rejas[0].altura>0 && m.rejas[0].altura<1,"Descenso gradual real");
        Dibujar(zona); ComprobarEscena(m);
        Avanzar(zona,24); Comprobar(m.rejas[0].altura==1 && m.bloques[2].activaColision,"Reja baja y hitbox original activa");
        Dibujar(zona,cantidad==4 ? "build/tesorero-rejas.png" : nullptr); ComprobarEscena(m);
        m.rejas[1].estado=REJA_TESORERO_AVISO; m.rejas[1].tiempoEstado=.001f; m.rejas[1].altura=0;
        zona.jugadores[0].posicion={5,.7f,0}; Avanzar(zona,1);
        Comprobar(m.rejas[1].estado==REJA_TESORERO_AVISO && !m.bloques[3].activaColision,"Reja no baja sobre jugador");
        AlejarJugadores(zona,cantidad); Avanzar(zona,1);
        Comprobar(m.rejas[1].estado==REJA_TESORERO_CERRADA,"Reja baja al despejar el paso");
        Avanzar(zona,200); Comprobar(m.rejas[0].estado==REJA_TESORERO_ABIERTA && m.rejas[0].altura==0 && !m.bloques[2].activaColision,"Reapertura y liberacion de hitbox");
        for (int k=0;k<4;k++) { m.rejas[k].estado=REJA_TESORERO_ABIERTA; m.rejas[k].tiempoEstado=100; m.rejas[k].altura=0; m.bloques[2+k].activaColision=false; }
        AlejarJugadores(zona,cantidad); int tesorero=m.indiceTesorero, atacante=tesorero==0 ? 1 : 0;
        zona.jugadores[tesorero].posicion={4,.7f,3}; zona.jugadores[atacante].posicion={3,.7f,3};
        zona.jugadores[atacante].direccionMirada={1,0,0}; Pulsar(zona,KEY_E);
        Comprobar(m.monedasTesorero==95 && m.monedasPerdidas==5 && MonedasActivas(m)==5,"Golpe real suelta cinco monedas y conserva puntuacion");
        Dibujar(zona,cantidad==4 ? "build/tesorero-golpe.png" : nullptr); ComprobarEscena(m);
        MonedaTesorero moneda{}; moneda.activa=true; moneda.y=.25f; moneda.edad=1;
        moneda.x=zona.jugadores[atacante].posicion.x; moneda.z=zona.jugadores[atacante].posicion.z;
        m.monedas[0]=moneda; Avanzar(zona,1);
        Comprobar(!m.monedas[0].activa && m.estadosJugadores[atacante].monedasRobadas==1,"Trio recoge moneda");
        moneda.x=zona.jugadores[tesorero].posicion.x; moneda.z=zona.jugadores[tesorero].posicion.z;
        m.monedas[0]=moneda; Avanzar(zona,1);
        Comprobar(!m.monedas[0].activa && m.monedasTesorero==96 && m.monedasPerdidas==4,"Tesorero recupera moneda");
        // Fixture de caida real: el motor de fisica genera el impacto al aterrizar.
        AlejarJugadores(zona,cantidad); auto& objetivo=zona.jugadores[tesorero]; auto& golpeador=zona.jugadores[atacante];
        objetivo.posicion={4,.7f,3}; golpeador.posicion={3.2f,.85f,3}; golpeador.enSuelo=false;
        golpeador.golpeSueloActivo=true; golpeador.velocidad.y=-15; Avanzar(zona,2);
        Comprobar(objetivo.aplastado && m.monedasTesorero==84,"Golpe al suelo real suelta doce monedas y aplasta");
        Dibujar(zona,cantidad==4 ? "build/tesorero-pound.png" : nullptr); ComprobarEscena(m);
        m.monedas[0]={}; m.monedas[0].activa=true; m.monedas[0].x=-7; m.monedas[0].z=0; m.monedas[0].y=.25f; m.monedas[0].edad=8;
        m.tiempoAnimacion=.1f; Dibujar(zona); ComprobarEscena(m);
        m.tiempoAnimacion=.25f; Dibujar(zona); ComprobarEscena(m);
        m.monedas[0].edad=8.99f; Avanzar(zona,1); Comprobar(!m.monedas[0].activa,"Moneda expira a nueve segundos");
        Pulsar(zona,KEY_R); Avanzar(zona,182);
        for (int i=0;i<cantidad;i++) participantes[i].esBot=true;
        Vector3 antesIA=zona.jugadores[0].posicion; Avanzar(zona,60);
        Comprobar(!Igual(antesIA,zona.jugadores[0].posicion),"IA de tesorero y perseguidores activa");
        participantes[0].esBot=false; participantes[0].control=CONTROL_GAMEPAD; participantes[0].indiceGamepad=-1;
        Avanzar(zona,2); Comprobar(!participantes[0].conectado && m.fase==FASE_TESORERO_JUGANDO,"Mando ausente mantiene ronda con IA");
        participantes[0].control=CONTROL_TECLADO_WASD;
        for (int i=0;i<cantidad;i++) participantes[i].esBot=false;
        for (auto& monedaActual:m.monedas) monedaActual.activa=false;
        m.monedasTesorero=70; m.tiempoRestante=.001f; Avanzar(zona,1);
        Comprobar(m.fase==FASE_TESORERO_TERMINADO && m.ganaTesorero && ResultadoMinijuegoFinalizado(m.resultado),"Final por tiempo con victoria del tesorero");
        Dibujar(zona,cantidad==4 ? "build/tesorero-final-tesorero.png" : nullptr);
        Pulsar(zona,KEY_R); Avanzar(zona,182); m.monedasTesorero=50; Avanzar(zona,1);
        Comprobar(m.fase==FASE_TESORERO_TERMINADO && !m.ganaTesorero,"Final anticipado del trio al perder la mitad");
        Dibujar(zona,cantidad==4 ? "build/tesorero-final-trio.png" : nullptr);
        Pulsar(zona,KEY_R); Comprobar(m.fase==FASE_TESORERO_PREPARACION && m.monedasTesorero==100 && MonedasActivas(m)==0,"Reinicio limpia estados y monedas");
        Pulsar(zona,KEY_ESCAPE); Comprobar(zona.volverAlMenu,"Salida al catalogo/menu");
        zona.Inicializar(participantes,cantidad); zona.modoCatalogo=zona.modoTablero=true;
        zona.CambiarMinijuego(MINIJUEGO_TESORERO_ACORRALADO); Pulsar(zona,KEY_R); Pulsar(zona,KEY_ESCAPE);
        Comprobar(!zona.volverAlMenu && m.tiempoPreparacion<3,"Ronda de tablero conserva reglas de salida y reinicio");
        for (int i=0;i<N;i++) Comprobar(cargas[i]==1 && mallas[i]==Recursos()[i].modelo.meshes,"Reinicio y reentrada sin duplicar cargas");
    }
    auto& m=zona.gestorMinijuegos.minijuegoTesoreroAcorralado;
    m.rejas[0].estado=REJA_TESORERO_AVISO; m.tiempoAnimacion=.1f;
    m.monedas[0].activa=true; m.monedas[0].x=7; m.monedas[0].y=.25f; m.monedas[0].z=4;
    Dibujar(zona); ComprobarEscena(m);
    int presentes[N]; std::memcpy(presentes,cantidades,sizeof(presentes));
    int baseCubos=cubos,baseEsferas=esferas,baseCilindros=cilindros,baseSegmentos=segmentos;
    for (int ausente=0;ausente<N;ausente++)
    {
        DescargarSlotModeloEscenarioRetro3D(Recursos()[ausente]); rutaAusente=RUTAS_MODELOS_TESORERO_CERCADO_3D[ausente];
        for (int k=0;k<3;k++) CargarPaqueteTesoreroCercadoRetro3D();
        Dibujar(zona);
        for (int i=0;i<N;i++) Comprobar(cantidades[i]==(i==ausente ? 0 : presentes[i]),"Fallback por pieza sin duplicar otras");
        Comprobar(cubos>baseCubos || esferas>baseEsferas || cilindros>baseCilindros || segmentos>baseSegmentos,"Primitivas de respaldo visibles");
        rutaAusente=nullptr; Recursos()[ausente]={}; CargarPaqueteTesoreroCercadoRetro3D(); GuardarMateriales();
    }
    Comprobar(avisos==N,"Un diagnostico por modelo ausente");
    const auto& almacen=ObtenerModelosEscenariosRetro3D();
    for (const auto& r:almacen.ultimoAsiento) Comprobar(!r.cargaIntentada,"No cargar Asiento");
    for (const auto& r:almacen.cajasPuerto) Comprobar(!r.cargaIntentada,"No cargar Puerto");
    for (const auto& r:almacen.laberintoJade) Comprobar(!r.cargaIntentada,"No cargar Jade");
    for (const auto& r:almacen.vetaCristal) Comprobar(!r.cargaIntentada,"No cargar Veta");
    for (const auto& r:almacen.capsulasBarajadas) Comprobar(!r.cargaIntentada,"No cargar Capsulas");
    for (const auto& r:almacen.bateoMeteorico) Comprobar(!r.cargaIntentada,"No cargar Bateo");
    for (const auto& r:almacen.racimoToxico) Comprobar(!r.cargaIntentada,"No cargar Racimo");
    zona.Descargar(); zona.Descargar();
    for (int i=0;i<N;i++) Comprobar(cargas[i]==descargas[i] && !Recursos()[i].modelo.meshes,"Descarga simetrica antes de CloseWindow");
    DescargarModeloJugadorCompartido(); CloseWindow();
    std::printf("Verificacion Tesorero Acorralado 2/3/4 participantes: %d errores\n",errores);
    return errores==0 ? 0 : 1;
}
