#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include <cmath>
#include <cstdio>
#include <string>

static const char* N[]={"lago_lava","cancha_obsidiana","borde_cancha",
 "poste_red","red_cadenas","volcan_menor","volcan_mayor",
 "columna_basalto","columna_con_llama","roca_magma","roca_sobrecalentada",
 "charco_lava","sombra_pelota","indicador_caida","burbuja_lava",
 "ceniza","estela_ascua"};
static Model M[17]{};
static void Pieza(int i,Vector3 p,Vector3 s={1,1,1}){
 DrawModelEx(M[i],p,{0,1,0},0,s,WHITE);
}
static void Paisaje(float t){
 Pieza(0,{0,0,0});
 Pieza(5,{-28,-1.5f,-38});Pieza(6,{26,-1.5f,-44});
 for(int i=0;i<10;i++){
  float x=-15.f+i*3.4f;float z=-9.f-(i*11%20)/10.f;
  float h=2.5f+(i*7%40)/10.f;
  Pieza(7,{x,-1.5f,z},{1,h/4,1});
 }
 for(int i=0;i<4;i++){
  float x=i%2?-12.2f:12.2f;float z=i<2?-3.5f:3.5f;
  float h=2.4f+(i*13%24)/10.f;
  Pieza(8,{x,-1.5f,z},{1,h/4,1});
 }
 for(int i=0;i<15;i++){
  float x=-26+(i*37%52),z=-26+(i*23%36);
  if(fabsf(x)<11&&fabsf(z)<7)continue;
  Pieza(14,{x,-1.47f+.12f*sinf(t+i),z});
 }
 for(int i=0;i<25;i++){
  float x=-15+(i*17%30),z=-12+(i*29%23),y=4+(i*19%100)/10.f;
  Pieza(15,{x,y,z});
 }
}
static void Cancha(float t,int modo){
 Pieza(1,{0,0,0});Pieza(2,{0,0,0});
 Pieza(3,{0,0,-5.4f});Pieza(3,{0,0,5.4f});Pieza(4,{0,0,0});
 for(int i=0;i<4;i++){
  float x=i<2?-5.2f:5.2f;float z=i%2?-2.f:2.f;
  Color c=i<2?Color{249,145,58,255}:Color{78,190,239,255};
  DrawCube({x,.7f,z},.72f,1.4f,.72f,c);
  DrawCube({x,1.44f,z},.65f,.16f,.65f,RAYWHITE);
 }
 Vector3 pelota=modo==3?Vector3{2.5f,1.2f,-1.2f}:Vector3{-.3f,3.1f,0};
 Pieza(modo==3?10:9,pelota);Pieza(12,{pelota.x,0,pelota.z});
 if(modo==3){
  Pieza(11,{2.5f,0,-1.2f});Pieza(11,{-4.1f,0,2.0f});
  Pieza(13,{3.5f,0,-1.7f});
 }else Pieza(13,{4.2f,0,1.5f});
 for(int k=1;k<7;k++){
  float f=1.f-k/8.f;
  Pieza(16,{pelota.x-k*.31f,pelota.y+k*.12f,pelota.z},{f,f,f});
 }
}
static void Escena(int modo,float t){
 if(modo>=5&&modo<22){Pieza(modo-5,{0,0,0});return;}
 if(modo!=4)Paisaje(t);
 Cancha(t,modo);
}
static Camera3D CamaraModelo(int id){
 BoundingBox b=GetModelBoundingBox(M[id]);
 Vector3 centro=Vector3Scale(Vector3Add(b.min,b.max),.5f);
 Vector3 size=Vector3Subtract(b.max,b.min);
 float radius=fmaxf(size.x,fmaxf(size.y,size.z));
 Vector3 dir=Vector3Normalize({.9f,.7f,1.50f});
 if(id==1||id==2||id==4)dir=Vector3Normalize({.48f,.9f,1.55f});
 if(id==5||id==6)dir=Vector3Normalize({.75f,.40f,1.50f});
 return {Vector3Add(centro,Vector3Scale(dir,radius*1.8f+1.4f)),centro,{0,1,0},36,CAMERA_PERSPECTIVE};
}
static void Dibujar(Camera3D cam,int modo,float t,bool hud=false){
 BeginDrawing();ClearBackground({43,29,38,255});BeginMode3D(cam);
 Escena(modo,t);EndMode3D();
 if(hud){
  DrawRectangle(0,0,1280,64,{44,30,38,235});
  DrawText("VOLEA DE MAGMA / Modelos GLB",18,8,23,RAYWHITE);
  DrawText("1 Escena | 2 Camara del juego | 3 Sobrecalentada | 4 Cancha | Flechas: girar",18,39,16,LIGHTGRAY);
 }
 EndDrawing();
}
static void Capturar(const char* nombre,Camera3D cam,int modo){
 Dibujar(cam,modo,1.3f);TakeScreenshot((std::string("Vistas/")+nombre+".png").c_str());
}
int main(int argc,char** argv){
 std::string carpeta=GetWorkingDirectory();
 bool capturar=argc>1&&std::string(argv[1])=="--capturar";
 bool verificar=argc>1&&std::string(argv[1])=="--verificar";
 SetTraceLogLevel(LOG_WARNING);SetConfigFlags(FLAG_MSAA_4X_HINT);
 InitWindow(capturar?1600:1280,capturar?1000:800,"Volea de Magma - modelos GLB");
 if(!IsWindowReady())return 1;
 ChangeDirectory(carpeta.c_str());rlSetClipPlanes(.2,260);
 int cargados=0;
 for(int i=0;i<17;i++){
  std::string ruta=std::string("GLB/")+N[i]+".glb";
  if(!FileExists(ruta.c_str())){std::fprintf(stderr,"Falta %s\n",ruta.c_str());break;}
  M[i]=LoadModel(ruta.c_str());cargados++;
  if(!M[i].meshCount)break;
  std::printf("VALIDADO %s meshes=%d\n",N[i],M[i].meshCount);
 }
 if(cargados!=17){for(int i=0;i<cargados;i++)UnloadModel(M[i]);CloseWindow();return 2;}
 if(capturar){
  Capturar("Escena_general",{{23,17,23},{0,3,-8},{0,1,0},55,CAMERA_PERSPECTIVE},1);
  Capturar("Camara_del_juego",{{0,9,14.5f},{0,1.6f,0},{0,1,0},50,CAMERA_PERSPECTIVE},2);
  Capturar("Roca_sobrecalentada",{{0,9,14.5f},{0,1.6f,0},{0,1,0},50,CAMERA_PERSPECTIVE},3);
  Capturar("Cancha_y_red",{{16,12,16},{0,1,0},{0,1,0},50,CAMERA_PERSPECTIVE},4);
  for(int i=0;i<17;i++)Capturar(N[i],CamaraModelo(i),i+5);
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
   Camera3D cam={{28*sinf(azimut),14,28*cosf(azimut)},{0,1,-4},{0,1,0},53,CAMERA_PERSPECTIVE};
   if(modo==2||modo==3)cam={{0,9,14.5f},{0,1.6f,0},{0,1,0},50,CAMERA_PERSPECTIVE};
   Dibujar(cam,modo,(float)GetTime(),true);
  }
 }
 for(int i=0;i<cargados;i++)UnloadModel(M[i]);CloseWindow();return 0;
}
