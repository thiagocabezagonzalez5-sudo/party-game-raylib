#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include <cmath>
#include <cstdio>
#include <string>

static const char* N[]={"agua_pantano","arbol_podrido","enredadera_central",
 "fruto_normal","fruto_toxico","fruto_dorado","balsa","tronco_flotante",
 "cabana_pilotes","arbol_fondo","luciernaga","nenufar","roca_pantano","aro_turno"};
static Model M[14]{};
static Color COLORES[]={{235,80,80,255},{80,140,240,255},{90,205,115,255},{245,205,70,255}};
static void Pieza(int i,Vector3 p,Color color=WHITE,float escala=1){
 DrawModel(M[i],p,escala,color);
}
static void Tronco(Vector3 a,Vector3 b){
 Vector3 dir=Vector3Subtract(b,a);float l=Vector3Length(dir);
 if(l<.001f)return;
 Vector3 u=Vector3Scale(dir,1/l);
 Vector3 axis=Vector3CrossProduct({0,0,1},u);
 float dot=fmaxf(-1.f,fminf(1.f,Vector3DotProduct({0,0,1},u)));
 if(Vector3Length(axis)<.001f)axis={0,1,0};
 DrawModelEx(M[7],a,axis,acosf(dot)*RAD2DEG,{1,1,l},WHITE);
}
static void Pantano(float t){
 Pieza(0,{0,0,0});Pieza(1,{-4.8f,0,-1});Pieza(2,{0,0,0});
 for(int i=0;i<7;i++)Pieza(9,{-16.f+5.5f*i,0,-15});
 Pieza(8,{-10.5f,0,-8});Pieza(8,{9.5f,0,-9});
 Tronco({-9,.1f,0},{-6,.1f,-1});
 Tronco({6.5f,.1f,-2},{10,.1f,-1});
 Tronco({2,.1f,-5},{5,.1f,-6});
 for(int i=0;i<22;i++){
  int mod=i==5||i==11||i==17?4:i==14?5:3;
  float x=(i%2==0?-.36f:.36f)+.03f*sinf(t*1.5f+i);
  Pieza(mod,{x,1.5f+i*.42f,0});
 }
 for(int i=0;i<16;i++){
  float x=-12+(i*37%24)+.8f*sinf(t*.7f+i);
  float y=.9f+(i*7%40)/5.f+.4f*sinf(t*1.1f+i*2);
  float z=-7+(i*13%12);
  Pieza(10,{x,y,z});
 }
 for(int x:{-14,-6,6,14}){
  Pieza(11,{(float)x,-.08f,-4.f+(x%3)});
  Pieza(12,{(float)x,-.15f,-10.f+(x%4)});
 }
}
static void Balsas(float t,int count){
 for(int i=0;i<count;i++){
  float x=(i-(count-1)*.5f)*5.f;
  Pieza(6,{x,.08f+.04f*sinf(t*2+x),3.5f});
  if(i==1)Pieza(13,{x,.02f,3.5f});
  // Personaje provisional solo para comprobar escala y visibilidad.
  DrawCube({x,1.05f,3.5f},.7f,1.7f,.7f,COLORES[i]);
  DrawCube({x,1.93f,3.5f},.65f,.2f,.65f,{240,238,222,255});
 }
}
static void Escena(int modo,float t){
 if(modo>=5&&modo<19){Pieza(modo-5,{0,0,0});return;}
 Pantano(t);
 if(modo==4)return;
 Balsas(t,modo==3?2:4);
}
static Camera3D CamaraModelo(int id){
 BoundingBox b=GetModelBoundingBox(M[id]);
 Vector3 centro=Vector3Scale(Vector3Add(b.min,b.max),.5f);
 Vector3 size=Vector3Subtract(b.max,b.min);
 float radius=fmaxf(size.x,fmaxf(size.y,size.z));
 Vector3 dir=Vector3Normalize({1.12f,.82f,1.55f});
 if(id==1||id==8||id==9)dir=Vector3Normalize({.8f,.51f,1.6f});
 return {Vector3Add(centro,Vector3Scale(dir,radius*1.9f+1.25f)),centro,{0,1,0},36,CAMERA_PERSPECTIVE};
}
static void Dibujar(Camera3D cam,int modo,float t,bool hud=false){
 BeginDrawing();ClearBackground({10,26,22,255});BeginMode3D(cam);
 Escena(modo,t);EndMode3D();
 if(hud){
  DrawRectangle(0,0,1280,62,{18,39,31,240});
  DrawText("RACIMO TOXICO / Modelos GLB",18,8,23,RAYWHITE);
  DrawText("1 Cuatro balsas | 2 Camara del juego | 3 Dos balsas | 4 Pantano | Flechas: girar",18,37,16,LIGHTGRAY);
 }
 EndDrawing();
}
static void Capturar(const char* nombre,Camera3D cam,int modo){
 Dibujar(cam,modo,1.25f);
 TakeScreenshot((std::string("Vistas/")+nombre+".png").c_str());
}
int main(int argc,char** argv){
 std::string carpeta=GetWorkingDirectory();
 bool capturar=argc>1&&std::string(argv[1])=="--capturar";
 bool verificar=argc>1&&std::string(argv[1])=="--verificar";
 SetTraceLogLevel(LOG_WARNING);SetConfigFlags(FLAG_MSAA_4X_HINT);
 InitWindow(capturar?1600:1280,capturar?1000:800,"Racimo Toxico - modelos GLB");
 if(!IsWindowReady())return 1;
 ChangeDirectory(carpeta.c_str());rlSetClipPlanes(.2,160);
 int cargados=0;
 for(int i=0;i<14;i++){
  std::string ruta=std::string("GLB/")+N[i]+".glb";
  if(!FileExists(ruta.c_str())){std::fprintf(stderr,"Falta %s\n",ruta.c_str());break;}
  M[i]=LoadModel(ruta.c_str());cargados++;
  if(!M[i].meshCount)break;
  std::printf("VALIDADO %s meshes=%d\n",N[i],M[i].meshCount);
 }
 if(cargados!=14){for(int i=0;i<cargados;i++)UnloadModel(M[i]);CloseWindow();return 2;}
 if(capturar){
  Capturar("Escena_general",{{19,13,24},{0,4,-2},{0,1,0},55,CAMERA_PERSPECTIVE},1);
  Capturar("Camara_del_juego",{{0,7.5f,17},{0,4.8f,0},{0,1,0},50,CAMERA_PERSPECTIVE},2);
  Capturar("Dos_balsas",{{0,7.5f,17},{0,4.8f,0},{0,1,0},50,CAMERA_PERSPECTIVE},3);
  Capturar("Pantano_y_cabanas",{{24,14,20},{0,4,-4},{0,1,0},51,CAMERA_PERSPECTIVE},4);
  for(int i=0;i<14;i++)Capturar(N[i],CamaraModelo(i),i+5);
 }else{
  SetTargetFPS(60);int modo=1,frames=0;float azimut=.5f;
  while(!WindowShouldClose()&&(!verificar||frames<10)){
   frames++;
   if(IsKeyPressed(KEY_ONE))modo=1;
   if(IsKeyPressed(KEY_TWO))modo=2;
   if(IsKeyPressed(KEY_THREE))modo=3;
   if(IsKeyPressed(KEY_FOUR))modo=4;
   if(IsKeyDown(KEY_LEFT))azimut-=GetFrameTime();
   if(IsKeyDown(KEY_RIGHT))azimut+=GetFrameTime();
   Camera3D cam={{24*sinf(azimut),10,24*cosf(azimut)},{0,4,0},{0,1,0},52,CAMERA_PERSPECTIVE};
   if(modo==2)cam={{0,7.5f,17},{0,4.8f,0},{0,1,0},50,CAMERA_PERSPECTIVE};
   Dibujar(cam,modo,(float)GetTime(),true);
  }
 }
 for(int i=0;i<cargados;i++)UnloadModel(M[i]);CloseWindow();return 0;
}
