#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include <cmath>
#include <cstdio>
#include <string>

static const char* N[]={"foso_agua","suelo_losas","muro_fondo","muro_lateral",
 "parapeto_frontal","torre_esquina_alta","torre_esquina_baja","torre_homenaje",
 "portal_fondo","estandarte_rojo","estandarte_dorado","antorcha",
 "marco_reja","reja_levadiza","aviso_reja","moneda_tesoro"};
static Model M[16]{};
static void Pieza(int id,Vector3 p,float rot=0,float escala=1){
 DrawModelEx(M[id],p,{0,1,0},rot,{escala,escala,escala},WHITE);
}
static void Escena(int modo,float t){
 if(modo>=5&&modo<21){Pieza(modo-5,{0,0,0});return;}
 Pieza(0,{0,0,0});Pieza(1,{0,0,0});Pieza(2,{0,0,-7.6f});
 Pieza(3,{-9.6f,0,0});Pieza(3,{9.6f,0,0});Pieza(4,{0,0,7.6f});
 for(int x:{-1,1}){
  Pieza(5,{x*10.4f,0,-8.2f});Pieza(6,{x*10.4f,0,8.2f});
 }
 Pieza(7,{0,0,0});Pieza(8,{0,0,-6.95f});
 for(int i=0;i<5;i++)Pieza(i%2?10:9,{-7.2f+i*3.6f,2.8f,-7.2f});
 for(int i=0;i<6;i++){
  float x=i%2?-8.8f:8.8f;float z=-4.5f+(i/2)*4.5f;
  Pieza(11,{x,0,z});
 }
 for(int i=0;i<4;i++){
  float x=i==0?-5.f:i==1?5.f:0;
  float z=i==2?-4.2f:i==3?4.2f:0;
  float rot=i<2?90:0;
  Pieza(12,{x,0,z},rot);
  if(modo!=4){
   float cerrada=modo==3?1.f:(i==1||i==2?1.f:0.f);
   Pieza(13,{x,(1-cerrada)*2.6f,z},rot);
   if(i==0&&modo==1)Pieza(14,{x,.03f,z},rot);
  }
 }
 if(modo!=4){
  for(int i=0;i<12;i++){
   float x=-7.f+(i*29%140)/10.f;float z=-5.f+(i*31%100)/10.f;
   if(std::abs(x)<2&&std::abs(z)<2)continue;
   Pieza(15,{x,.22f+.04f*sinf(t+i),z},t*65.f+i*15.f);
  }
  // Marcadores de escala de los jugadores, ajenos al paquete GLB.
  for(int i=0;i<4;i++){
   float x=i==0?7.6f:-7.6f;float z=i==0?0:(i-2)*2.1f;
   Color c=i==0?Color{250,206,69,255}:Color{222,63,65,255};
   DrawCube({x,.7f,z},.7f,1.4f,.7f,c);
   DrawCube({x,1.44f,z},.65f,.15f,.65f,RAYWHITE);
  }
 }
}
static Camera3D CamaraModelo(int id){
 BoundingBox b=GetModelBoundingBox(M[id]);
 Vector3 centro=Vector3Scale(Vector3Add(b.min,b.max),.5f);
 Vector3 size=Vector3Subtract(b.max,b.min);
 float radius=fmaxf(size.x,fmaxf(size.y,size.z));
 Vector3 dir=Vector3Normalize({1.f,.7f,1.35f});
 if(id==2||id==3||id==4)dir=Vector3Normalize({.25f,.65f,1.5f});
 return {Vector3Add(centro,Vector3Scale(dir,radius*1.9f+1.25f)),centro,{0,1,0},36,CAMERA_PERSPECTIVE};
}
static void Dibujar(Camera3D cam,int modo,float t,bool hud=false){
 BeginDrawing();ClearBackground({24,32,46,255});BeginMode3D(cam);
 Escena(modo,t);EndMode3D();
 if(hud){
  DrawRectangle(0,0,1280,64,{31,38,48,240});
  DrawText("TESORERO CERCADO / Modelos GLB",18,8,23,RAYWHITE);
  DrawText("1 Patio | 2 Camara del juego | 3 Rejas bajas | 4 Castillo | Flechas: girar",18,39,16,LIGHTGRAY);
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
 InitWindow(capturar?1600:1280,capturar?1000:800,"Tesorero Cercado - modelos GLB");
 if(!IsWindowReady())return 1;
 ChangeDirectory(carpeta.c_str());rlSetClipPlanes(.2,170);
 int cargados=0;
 for(int i=0;i<16;i++){
  std::string ruta=std::string("GLB/")+N[i]+".glb";
  if(!FileExists(ruta.c_str())){std::fprintf(stderr,"Falta %s\n",ruta.c_str());break;}
  M[i]=LoadModel(ruta.c_str());cargados++;
  if(!M[i].meshCount)break;
  std::printf("VALIDADO %s meshes=%d\n",N[i],M[i].meshCount);
 }
 if(cargados!=16){for(int i=0;i<cargados;i++)UnloadModel(M[i]);CloseWindow();return 2;}
 if(capturar){
  Capturar("Escena_general",{{22,18,22},{0,1.3f,0},{0,1,0},55,CAMERA_PERSPECTIVE},1);
  Capturar("Camara_del_juego",{{0,17.5f,13.5f},{0,.4f,.6f},{0,1,0},50,CAMERA_PERSPECTIVE},2);
  Capturar("Rejas_cerradas",{{0,17.5f,13.5f},{0,.4f,.6f},{0,1,0},50,CAMERA_PERSPECTIVE},3);
  Capturar("Castillo",{{25,15,20},{0,1,0},{0,1,0},52,CAMERA_PERSPECTIVE},4);
  for(int i=0;i<16;i++)Capturar(N[i],CamaraModelo(i),i+5);
 }else{
  SetTargetFPS(60);int modo=1,frames=0;float azimut=.7f;
  while(!WindowShouldClose()&&(!verificar||frames<10)){
   frames++;
   if(IsKeyPressed(KEY_ONE))modo=1;
   if(IsKeyPressed(KEY_TWO))modo=2;
   if(IsKeyPressed(KEY_THREE))modo=3;
   if(IsKeyPressed(KEY_FOUR))modo=4;
   if(IsKeyDown(KEY_LEFT))azimut-=GetFrameTime();
   if(IsKeyDown(KEY_RIGHT))azimut+=GetFrameTime();
   Camera3D cam={{27*sinf(azimut),17,27*cosf(azimut)},{0,1,0},{0,1,0},53,CAMERA_PERSPECTIVE};
   if(modo==2||modo==3)cam={{0,17.5f,13.5f},{0,.4f,.6f},{0,1,0},50,CAMERA_PERSPECTIVE};
   Dibujar(cam,modo,(float)GetTime(),true);
  }
 }
 for(int i=0;i<cargados;i++)UnloadModel(M[i]);CloseWindow();return 0;
}
