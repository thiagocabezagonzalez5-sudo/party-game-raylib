// Visor independiente para Cajas del Puerto; C++17 y raylib.
// Ejecutar desde esta carpeta, que contiene GLB/ y Vistas/.
#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include <cmath>
#include <cstdio>
#include <string>

static const char* NOMBRES[]={"muelle","grua_portico","grua_carro","grua_cabina","grua_brazo",
 "grua_cable_unidad","grua_gancho","contenedor_cuerpo","contenedor_puerta_izquierda",
 "contenedor_puerta_derecha","contenedor_decoracion","farol","bolardo","ancla_dorada",
 "barco_fondo","marca_golpe"};
static Model modelos[16];
static const Color COLORES[8]={{168,62,52,255},{52,110,166,255},{70,140,84,255},{196,150,56,255},
 {130,76,150,255},{60,150,156,255},{190,98,52,255},{112,118,130,255}};

static void DibujarPieza(int id,Vector3 posicion, Color color=WHITE,
 float angulo=0,Vector3 eje={0,1,0},Vector3 escala={1,1,1})
{
 int primitiva=-1;
 if(id==7 || id==10)primitiva=1;
 else if(id==8 || id==9)primitiva=0;
 if(primitiva>=0){
  int material=modelos[id].meshMaterial[primitiva];
  Color anterior=modelos[id].materials[material].maps[MATERIAL_MAP_DIFFUSE].color;
  modelos[id].materials[material].maps[MATERIAL_MAP_DIFFUSE].color=color;
  DrawModelEx(modelos[id],posicion,eje,angulo,escala,WHITE);
  modelos[id].materials[material].maps[MATERIAL_MAP_DIFFUSE].color=anterior;
 }else DrawModelEx(modelos[id],posicion,eje,angulo,escala,color);
}
static void DibujarContenedores(float abertura)
{
 for(int i=0;i<8;i++){
  float x=(i-3.5f)*2.1f;
  DibujarPieza(7,{x,0,-2.5f},COLORES[i]);
  float escala=1-.88f*abertura;
  DibujarPieza(8,{x-.85f,1.1f,-1.42f},COLORES[i],0,{0,1,0},{escala,1,1});
  DibujarPieza(9,{x+.85f,1.1f,-1.42f},COLORES[i],0,{0,1,0},{escala,1,1});
 }
 DibujarPieza(13,{(-3.5f)*2.1f,2.3f,-2.5f});
 DibujarPieza(15,{(+2.5f)*2.1f,2.33f,-2.5f});
}
static void DibujarGrua(float ganchoX,float tiempo)
{
 DibujarPieza(1,{0,0,-5});
 DibujarPieza(2,{ganchoX,5.7f,-5});
 DibujarPieza(3,{ganchoX,5.7f,-5});
 DibujarPieza(4,{ganchoX,5.55f,-5});
 float y=4.2f+.06f*sinf(tiempo*3);
 DibujarPieza(5,{ganchoX,5.55f,-2.5f},WHITE,0,{0,1,0},{1,5.55f-y,1});
 DibujarPieza(6,{ganchoX,y,-2.5f});
}
static void DibujarPuerto(float tiempo,float abertura)
{
 DrawPlane({0,-.92f,-4},{120,90},{10,26,44,255});
 DibujarPieza(0,{0,0,.75f});
 DibujarPieza(14,{0,-.9f,-17});
 for(int i=0;i<4;i++){
  float x=i%2==0?-10.5f:10.5f;float z=i<2?1.0f:6.5f;
  DibujarPieza(11,{x,0,z});
 }
 for(int i=0;i<6;i++)DibujarPieza(12,{-10.0f+4.0f*i,0,7.8f});
 for(int x=-1;x<=1;x+=2){
  DibujarPieza(10,{x*12.5f,0,-5},COLORES[x<0?2:1]);
  DibujarPieza(10,{x*12.5f,2.2f,-5},COLORES[x<0?0:3]);
 }
 DibujarContenedores(abertura);
 DibujarGrua(sinf(tiempo*.6f)*5.7f,tiempo);
}
static void Renderizar(Camera3D cam,int modo,float tiempo,bool mostrarControles=false)
{
 BeginDrawing();ClearBackground({5,12,25,255});BeginMode3D(cam);
 if(modo==1)DibujarPuerto(tiempo,.75f);
 else if(modo==2){
  DrawPlane({0,-.92f,-4},{90,70},{10,26,44,255});
  DibujarPieza(0,{0,0,.75f});DibujarGrua(sinf(tiempo*.6f)*3.2f,tiempo);
  DibujarContenedores(.15f);
 }
 else if(modo==3){DibujarContenedores(.9f);}
 else if(modo==4){DrawPlane({0,-.92f,-4},{80,65},{10,26,44,255});DibujarPieza(14,{0,-.9f,-17});}
 else if(modo>=5 && modo<21){int i=modo-5;DibujarPieza(i,{0,0,0},i==7||i==8||i==9||i==10?COLORES[1]:WHITE);}
 EndMode3D();
 if(mostrarControles){
  DrawRectangle(0,0,1280,62,{14,24,40,235});
  DrawText("CAJAS DEL PUERTO / Modelos GLB",18,8,23,RAYWHITE);
  DrawText("1 Puerto | 2 Grua | 3 Contenedores | 4 Barco | Flechas: girar",18,37,16,LIGHTGRAY);
 }
 EndDrawing();
}
static Camera3D CamaraModelo(int id){
 BoundingBox b=GetModelBoundingBox(modelos[id]);
 Vector3 centro=Vector3Scale(Vector3Add(b.min,b.max),.5f);
 Vector3 size=Vector3Subtract(b.max,b.min);
 float radio=fmaxf(size.x,fmaxf(size.y,size.z));
 Vector3 vector=Vector3Normalize({1.0f,.73f,1.65f});
 if(id==1||id==14)vector=Vector3Normalize({.55f,.48f,2.1f});
 if(id==8||id==9)vector=Vector3Normalize({.9f,.55f,1.8f});
 return {Vector3Add(centro,Vector3Scale(vector,radio*2.1f)),centro,{0,1,0},36,CAMERA_PERSPECTIVE};
}
static void Capturar(const char* nombre,Camera3D cam,int modo,float tiempo)
{
 Renderizar(cam,modo,tiempo);
 std::string destino=std::string("Vistas/")+nombre+".png";
 TakeScreenshot(destino.c_str());
}
int main(int argc,char** argv)
{
 std::string carpetaPaquete=GetWorkingDirectory();
 bool capturar=argc>1 && std::string(argv[1])=="--capturar";
 bool verificar=argc>1 && std::string(argv[1])=="--verificar";
 SetTraceLogLevel(LOG_WARNING);SetConfigFlags(FLAG_MSAA_4X_HINT);
 InitWindow(capturar?1600:1280,capturar?1000:800,"Cajas del Puerto - modelos GLB");
 if(!IsWindowReady())return 1;
 ChangeDirectory(carpetaPaquete.c_str());
 rlSetClipPlanes(.2,220);
 int cargados=0;
 for(int i=0;i<16;i++){
  std::string ruta=std::string("GLB/")+NOMBRES[i]+".glb";
  if(!FileExists(ruta.c_str())){
   std::fprintf(stderr,"Falta %s. Ejecuta el visor desde la carpeta del paquete.\n",ruta.c_str());
   for(int j=0;j<cargados;j++)UnloadModel(modelos[j]);CloseWindow();return 2;
  }
  modelos[i]=LoadModel(ruta.c_str());cargados++;
  if(modelos[i].meshCount==0){for(int j=0;j<cargados;j++)UnloadModel(modelos[j]);CloseWindow();return 3;}
  std::printf("VALIDADO %s meshes=%d\n",NOMBRES[i],modelos[i].meshCount);
 }
 if(capturar){
  Camera3D general={{27,18,27},{0,2,-5},{0,1,0},46,CAMERA_PERSPECTIVE};
  Capturar("Escena_general",general,1,2);
  Camera3D grua={{17,12,13},{0,3,-4},{0,1,0},48,CAMERA_PERSPECTIVE};
  Capturar("Grua_y_contenedores",grua,2,2);
  SetWindowSize(1280,720);
  for(int i=0;i<3;i++){BeginDrawing();ClearBackground(BLACK);EndDrawing();}
  Camera3D juego={{0,10.5f,12.5f},{0,3.2f,-1.5f},{0,1,0},54,CAMERA_PERSPECTIVE};
  Capturar("Camara_del_juego",juego,1,2);
  SetWindowSize(1600,1000);
  for(int i=0;i<3;i++){BeginDrawing();ClearBackground(BLACK);EndDrawing();}
  for(int i=0;i<16;i++)Capturar(NOMBRES[i],CamaraModelo(i),i+5,2);
 }else{
  SetTargetFPS(60);int modo=1,frames=0;float azimut=.65f;
  while(!WindowShouldClose() && (!verificar || frames<10)){
   frames++;
   if(IsKeyPressed(KEY_ONE))modo=1;
   if(IsKeyPressed(KEY_TWO))modo=2;
   if(IsKeyPressed(KEY_THREE))modo=3;
   if(IsKeyPressed(KEY_FOUR))modo=4;
   if(IsKeyDown(KEY_LEFT))azimut-=GetFrameTime();
   if(IsKeyDown(KEY_RIGHT))azimut+=GetFrameTime();
   Camera3D cam{};cam.up={0,1,0};cam.fovy=46;cam.projection=CAMERA_PERSPECTIVE;
   float radio=38,altura=19;cam.target={0,2,-5};
   if(modo==2){radio=25;altura=15;cam.target={0,3,-4};}
   if(modo==3){radio=18;altura=7;cam.target={0,1,-2.5f};}
   if(modo==4){radio=38;altura=14;cam.target={0,2,-17};}
   cam.position={cam.target.x+radio*sinf(azimut),altura,cam.target.z+radio*cosf(azimut)};
   Renderizar(cam,modo,(float)GetTime(),true);
  }
 }
 for(int i=0;i<cargados;i++)UnloadModel(modelos[i]);CloseWindow();return 0;
}
