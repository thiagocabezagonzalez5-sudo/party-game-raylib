#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include <cmath>
#include <cstdio>
#include <string>
static const char* N[]={"fondo_rosa","suelo_galleta","azucar_lateral","azucar_extremo","chocolate_grande","chocolate_pequeno","muro_galleta","gominola_roja","gominola_verde","gominola_amarilla","gominola_azul","piruleta_rosa","piruleta_azul","piruleta_amarilla","piruleta_violeta","piruleta_verde","columna_caramelo","montana_nata","bola_azucar","pepita_chocolate"};
static Model M[20]{};static Color PLAYER[]={{235,80,80,255},{80,140,240,255},{90,205,115,255},{245,205,70,255}};
static void Part(int i,Vector3 p={0,0,0},float yaw=0,Vector3 scale={1,1,1},Color tint=WHITE){DrawModelEx(M[i],p,{0,1,0},yaw,scale,tint);}
static void Scene(int mode){
 if(mode>=10){Part(mode-10);return;}
 Part(0);Part(1);
 Part(2,{-6,0,0});Part(2,{6,0,0});Part(3,{0,0,-6.75f});Part(3,{0,0,6.75f});
 Part(4);Part(5,{-5.5f,0,-6.5f});Part(5,{5.5f,0,6.5f});
 for(int sx:{-1,1}){Part(6,{0,0,sx*9.2f});Part(6,{sx*9.2f,0,0},90);}
 for(int i=0;i<4;i++)Part(7+i,{i%2?3.2f:-3.2f,0,i<2?-3.2f:3.2f});
 for(int i=0;i<28;i++){
  float x=-8.4f+((i*37)%168)/10.f,z=-8.4f+((i*53)%168)/10.f;
  if((fabsf(x)>4&&fabsf(x)<8&&fabsf(z)<4.5)||fabsf(x)<3&&fabsf(z)>5.5)continue;
  Part(19,{x,0,z});
 }
 for(int i=0;i<7;i++)Part(11+i%5,{-20.f+6.5f*i,0,-16});
 Part(12,{-16,0,4});Part(14,{16,0,-3});Part(11,{17,0,9});
 for(int i=0;i<4;i++)Part(16,{i%2?12.5f:-12.5f,0,i<2?-12.f:12.f});
 for(int i=0;i<3;i++)Part(17,{-22.f+20*i,-.5f,-30});
 if(mode!=4){
  Vector3 places[]={{-6.7f,.55f,-2.5f},{6.7f,.75f,2.5f},{-2.0f,1.15f,6.6f},{2.0f,.30f,-6.6f}};
  for(int i=0;i<(mode==3?2:4);i++){
   float r=places[i].y;Part(18,{places[i].x,r,places[i].z},0,{r,r,r});
   DrawCircle3D({places[i].x,.10f,places[i].z},r+.08f,{1,0,0},90,PLAYER[i]);
   DrawCube({places[i].x+.8f,.85f,places[i].z+.65f},.55f,1.5f,.55f,PLAYER[i]);
  }
 }
}
static Camera3D CamModel(int i){
 BoundingBox b=GetModelBoundingBox(M[i]);Vector3 c=Vector3Scale(Vector3Add(b.min,b.max),.5f),v=Vector3Subtract(b.max,b.min);
 float r=fmaxf(v.x,fmaxf(v.y,v.z));Vector3 dir=Vector3Normalize({.8f,1.f,1.45f});
 if(i==0||i==1||i==2||i==3||i==4||i==5)dir=Vector3Normalize({.5f,1.7f,1.4f});
 return {Vector3Add(c,Vector3Scale(dir,r*1.7f+1.1f)),c,{0,1,0},37,CAMERA_PERSPECTIVE};
}
static void Render(Camera3D cam,int mode,bool hud){BeginDrawing();ClearBackground({255,206,225,255});BeginMode3D(cam);Scene(mode);EndMode3D();if(hud){DrawRectangle(0,0,1280,64,{104,56,85,225});DrawText("BOLAS DE AZUCAR / Modelos GLB",20,8,23,RAYWHITE);DrawText("1 Cuatro jugadores | 2 Arena | 3 Dos jugadores | 4 Paisaje",20,39,16,RAYWHITE);}EndDrawing();}
static void Capture(const char* n,Camera3D cam,int mode){Render(cam,mode,false);TakeScreenshot((std::string("Vistas/")+n+".png").c_str());}
int main(int argc,char** argv){
 std::string cwd=GetWorkingDirectory();bool capture=argc>1&&std::string(argv[1])=="--capturar",verify=argc>1&&std::string(argv[1])=="--verificar";
 SetTraceLogLevel(LOG_WARNING);SetConfigFlags(FLAG_MSAA_4X_HINT);InitWindow(capture?1600:1280,capture?1000:800,"Bolas de Azucar - modelos GLB");
 if(!IsWindowReady())return 1;ChangeDirectory(cwd.c_str());rlSetClipPlanes(.2,230);
 int loaded=0;for(int i=0;i<20;i++){
  std::string path=std::string("GLB/")+N[i]+".glb";if(!FileExists(path.c_str())){fprintf(stderr,"Falta %s\n",path.c_str());break;}
  M[i]=LoadModel(path.c_str());loaded++;if(!M[i].meshCount)break;printf("VALIDADO %s meshes=%d\n",N[i],M[i].meshCount);
 }
 if(loaded!=20){for(int i=0;i<loaded;i++)UnloadModel(M[i]);CloseWindow();return 2;}
 if(capture){
  Capture("Camara_del_juego",{{0,20,11},{0,0,.5f},{0,1,0},46,CAMERA_PERSPECTIVE},1);
  Capture("Arena_dulce",{{16,20,19},{0,0,0},{0,1,0},57,CAMERA_PERSPECTIVE},2);
  Capture("Dos_jugadores",{{0,20,11},{0,0,.5f},{0,1,0},46,CAMERA_PERSPECTIVE},3);
  Capture("Paisaje_azucarado",{{24,17,22},{0,2,-11},{0,1,0},58,CAMERA_PERSPECTIVE},4);
  Capture("Detalle_obstaculos",{{9,10,9},{0,0,0},{0,1,0},53,CAMERA_PERSPECTIVE},1);
  for(int i=0;i<20;i++)Capture(N[i],CamModel(i),i+10);
 }else{SetTargetFPS(60);int mode=1,frames=0;while(!WindowShouldClose()&&(!verify||frames++<10)){
  if(IsKeyPressed(KEY_ONE))mode=1;if(IsKeyPressed(KEY_TWO))mode=2;if(IsKeyPressed(KEY_THREE))mode=3;if(IsKeyPressed(KEY_FOUR))mode=4;
  Render({{0,20,11},{0,0,.5f},{0,1,0},46,CAMERA_PERSPECTIVE},mode,true);
 }}for(int i=0;i<loaded;i++)UnloadModel(M[i]);CloseWindow();return 0;
}
