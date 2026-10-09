#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include <cmath>
#include <cstdio>
#include <string>
static const char* N[]={"losa_normal","losa_agrietada","losa_salida","losa_meta","fragmento_losa","brillo_losa","terreno_cementerio","borde_cementerio","lapida","lapida_cruz","mausoleo","arbol_seco","verja_tramo","farol_verde","farol_recogible","fuego_fatuo","arco_meta","luna","niebla_abismo"};
static Model M[19]{};
static Color players[]={{234,92,89,255},{86,151,242,255},{92,212,137,255},{247,207,76,255}};
static void Part(int i,Vector3 p={0,0,0},float yaw=0,Vector3 s={1,1,1},Color tint=WHITE){DrawModelEx(M[i],p,{0,1,0},yaw,s,tint);}
static Camera3D GameCam(int lanes){float dist=(.5f*fmaxf(2,lanes)*10+2)/.6f;if(dist<15)dist=15;return {{0,.7f*dist,-.8f+.75f*dist},{0,0,-.8f},{0,1,0},50,CAMERA_PERSPECTIVE};}
static void Scene(int mode){
 if(mode>=10){Part(mode-10);return;}
 int lanes=mode==3?2:4;
 Part(18);Part(6,{0,0,-28});Part(7,{0,0,-9.6f});
 for(int k=0;k<5;k++)Part(12,{-22.4f+11.2f*k,0,-9.4f});
 for(int k=0;k<7;k++)Part(13,{-27.f+9*k,0,-9.f});
 for(int k=0;k<34;k++){
  float x=-30.f+6.f*(k%11)+(k%3)*.3f,z=-14.f-5.f*(k/11);
  if(fabsf(x+14)<3&&z<-20)continue;
  Part(k%3==0?9:8,{x,-.5f,z},(k*29)%35-18);
 }
 Part(10,{-14,-.5f,-24},0,{1.2f,1.2f,1.2f});Part(10,{12,-.5f,-27});Part(10,{30,-.5f,-22},0,{1.3f,1.3f,1.3f});
 for(int k=0;k<6;k++)Part(11,{-32.f+k*13.f,-.5f,-19.f-(k%3)*2},k*20,{1,1+(k%3)*.1f,1});
 for(int lane=0;lane<lanes;lane++){
  float c=(lane-.5f*(lanes-1))*10;
  for(int row=0;row<10;row++)for(int col=0;col<6;col++){
   float x=c+(col-2.5f)*1.5f,z=6.f-row*1.5f;
   bool crack=(row==2&&col==lane%2+2)||(row==5&&col==4)||(row==7&&col==1);
   if(mode==4&&row==6&&col==3)continue;
   int which=row==0?2:(row==9?3:(crack?1:0));
   Part(which,{x,0,z},0,{1,1,1},row==0?players[lane]:WHITE);
   if(mode==2&&((col==2&&row<5)||(col==3&&row>=5&&row<9)))Part(5,{x,0,z});
  }
  float gx=c+(3-2.5f)*1.5f;
  Part(16,{gx,0,-7.9f});
  float lx=c+(3-2.5f)*1.5f,lz=6.f-5*1.5f;
  Part(14,{lx,.58f,lz});
  if(mode==2)Part(15,{lx,.95f,lz+1.5f});
  if(mode!=3){
   float px=c+(2-2.5f)*1.5f,pz=6.f-2*1.5f;
   DrawCube({px,.75f,pz},.55f,1.3f,.55f,players[lane]);
  }
 }
 if(mode==4){for(int i=0;i<4;i++)Part(4,{.75f+(i%2?1:-1)*(.3f+i*.1f),-1.0f-i*.15f,-3.f+(i/2?-.5f:.5f)});}
}
static Camera3D ModelCam(int i){BoundingBox b=GetModelBoundingBox(M[i]);Vector3 c=Vector3Scale(Vector3Add(b.min,b.max),.5f),v=Vector3Subtract(b.max,b.min);float r=fmaxf(v.x,fmaxf(v.y,v.z));Vector3 dir=Vector3Normalize({.8f,1.1f,1.55f});if(i<=5||i==18)dir=Vector3Normalize({.5f,1.8f,1.3f});return {Vector3Add(c,Vector3Scale(dir,r*1.65f+1)),c,{0,1,0},39,CAMERA_PERSPECTIVE};}
static void Render(Camera3D cam,int mode,bool hud){
 BeginDrawing();ClearBackground({24,30,47,255});BeginMode3D(cam);
 Scene(mode);EndMode3D();if(hud){DrawRectangle(0,0,1280,66,{24,33,44,230});DrawText("SENDERO INVISIBLE / Modelos GLB",20,9,23,RAYWHITE);DrawText("1 Cuatro carriles | 2 Ruta revelada | 3 Dos carriles | 4 Losa rota",20,41,16,RAYWHITE);}EndDrawing();
}
static void Capture(const char* name,Camera3D cam,int mode){Render(cam,mode,false);TakeScreenshot((std::string("Vistas/")+name+".png").c_str());}
int main(int argc,char**argv){
 std::string cwd=GetWorkingDirectory();bool capture=argc>1&&std::string(argv[1])=="--capturar",verify=argc>1&&std::string(argv[1])=="--verificar";
 SetTraceLogLevel(LOG_WARNING);SetConfigFlags(FLAG_MSAA_4X_HINT);InitWindow(capture?1600:1280,capture?1000:800,"Sendero Invisible - GLB");if(!IsWindowReady())return 1;
 ChangeDirectory(cwd.c_str());rlSetClipPlanes(.2,240);
 int loaded=0;for(int i=0;i<19;i++){
  std::string path=std::string("GLB/")+N[i]+".glb";if(!FileExists(path.c_str())){fprintf(stderr,"Falta %s\n",path.c_str());break;}
  M[i]=LoadModel(path.c_str());if(!M[i].meshCount)break;loaded++;printf("VALIDADO %s meshes=%d\n",N[i],M[i].meshCount);
 }
 if(loaded!=19){for(int i=0;i<loaded;i++)UnloadModel(M[i]);CloseWindow();return 2;}
 if(capture){
  Capture("Camara_cuatro_carriles",GameCam(4),1);
  Capture("Ruta_memorizacion",GameCam(4),2);
  Capture("Camara_dos_carriles",GameCam(2),3);
  Capture("Losa_rota",GameCam(4),4);
  Capture("Vista_del_cementerio",{{25,18,20},{0,0,-9},{0,1,0},52,CAMERA_PERSPECTIVE},1);
  for(int i=0;i<19;i++)Capture(N[i],ModelCam(i),i+10);
 }else{SetTargetFPS(60);int mode=1,frames=0;while(!WindowShouldClose()&&(!verify||frames++<10)){
  if(IsKeyPressed(KEY_ONE))mode=1;if(IsKeyPressed(KEY_TWO))mode=2;if(IsKeyPressed(KEY_THREE))mode=3;if(IsKeyPressed(KEY_FOUR))mode=4;
  Render(GameCam(mode==3?2:4),mode,true);
 }}for(int i=0;i<loaded;i++)UnloadModel(M[i]);CloseWindow();return 0;
}
