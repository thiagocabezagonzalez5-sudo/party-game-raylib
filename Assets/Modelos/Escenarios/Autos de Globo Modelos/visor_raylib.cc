#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include <cmath>
#include <cstdio>
#include <string>
static const char* N[]={"plaza_elevada","anillo_central","barrera_frontal","barrera_lateral","rascacielos_bajo","rascacielos_medio","rascacielos_alto","jardin_vertical","autopista_trasera","autopista_lateral","vehiculo_trafico","holograma_pedestal","holograma_anillos","dron_cuerpo","dron_helice","placa_aviso","placa_recarga","orbe_recarga","auto_flotante","globo_energia","cuerda_globo","llama_turbo","halo_auto"};
static Model M[23]{};
static Color team[]={{241,91,84,255},{72,142,242,255},{85,210,135,255},{247,206,76,255}};
static void Part(int i,Vector3 p={0,0,0},float yaw=0,Vector3 s={1,1,1},Color tint=WHITE){DrawModelEx(M[i],p,{0,1,0},yaw,s,tint);}
static void Car(int i,Vector3 p,float angle,int balls,bool boost){
 Color c=team[i];Part(22,p,0,{1,1,1},c);Part(18,p,angle,{1,1,1},c);
 float a=angle*DEG2RAD,co=cosf(a),si=sinf(a);
 for(int k=0;k<balls;k++){
  float lat=(k-1)*.5f,back=.7f+.15f*fabsf((float)k-1),y=1.9f+.2f*(k%2);
  Vector3 b={p.x-back*co+lat*si,y,p.z+back*si+lat*co};Part(19,b,0,{1,1,1},c);
  DrawLine3D({p.x-.9f*co,.6f,p.z+.9f*si},{b.x,b.y-.35f,b.z},{220,237,250,255});
 }
 if(boost){Vector3 f={p.x-1.05f*co,.35f,p.z+1.05f*si};Part(21,f,angle);}
}
static void Scene(int mode){
 if(mode>=10){Part(mode-10);return;}
 Part(8,{0,0,-9.6f});Part(9,{12.4f,0,0});
 for(int i=0;i<9;i++){
  float x=-24.f+6.f*i,z=-20.f-(i%3)*1.6f;int b=4+i%3;
  Part(b,{x,-6,z});if(i%3==0)Part(7,{x,-2.8f,z+2.1f});
 }
 for(int i=0;i<7;i++){
  float x=i%2?17.5f:-17.5f,z=-11.f+(i/2)*6.f;
  Part(4+i%3,{x,-6,z});
 }
 for(int i=0;i<8;i++)Part(10,{-22.5f+5.6f*i,-.1f,-9.6f+(i%2?-.5f:.5f)},0,{1,1,1},i%2?Color{110,230,255,255}:Color{255,125,130,255});
 Part(0);Part(1);
 for(int side:{-1,1}){Part(2,{0,0,side*6.45f});Part(3,{side*9.15f,0,0},90);}
 for(int k=0;k<4;k++){
  float x=k%2?10.6f:-10.6f,z=k<2?-7.6f:7.6f;
  Part(11,{x,0,z});Part(12,{x,2.4f,z},k*32, {1,1,1},k%2?Color{255,130,220,255}:Color{90,255,225,255});
  float dx=k%2?13.f:-13.f,dz=k<2?-4.1f:4.1f;
  Part(13,{dx,3.5f+k*.55f,dz});for(int s=0;s<4;s++){
   float a=s*PI/2;Part(14,{dx+.38f*cosf(a),3.66f+k*.55f,dz+.38f*sinf(a)},s*30.f);
  }
 }
 if(mode==3)return;
 Part(15,{-5.5f,0,-3.2f});Part(16,{5.5f,0,3.2f});Part(17,{5.5f,1.f,3.2f});
 Vector3 p[]={{-5.1f,0,2.8f},{5.0f,0,-2.6f},{4.6f,0,2.7f},{-4.5f,0,-2.7f}};
 for(int i=0;i<(mode==4?2:4);i++){
  float angle=-atan2f(-p[i].z,-p[i].x)*RAD2DEG;
  Car(i,p[i],angle,mode==4?3:(i==1?2:3),i==0&&mode==2);
 }
}
static Camera3D ModelCam(int i){BoundingBox b=GetModelBoundingBox(M[i]);Vector3 c=Vector3Scale(Vector3Add(b.min,b.max),.5f),v=Vector3Subtract(b.max,b.min);float r=fmaxf(v.x,fmaxf(v.y,v.z));Vector3 dir=Vector3Normalize({.8f,1.25f,1.6f});if(i==0||i==1||i==15||i==16||i==22)dir=Vector3Normalize({.5f,1.9f,1.25f});return {Vector3Add(c,Vector3Scale(dir,r*1.65f+1)),c,{0,1,0},39,CAMERA_PERSPECTIVE};}
static void Render(Camera3D cam,int mode,bool hud){BeginDrawing();ClearBackground({98,138,177,255});BeginMode3D(cam);Scene(mode);EndMode3D();if(hud){DrawRectangle(0,0,1280,66,{31,38,72,230});DrawText("AUTOS DE GLOBO / Modelos GLB",20,9,23,RAYWHITE);DrawText("1 Arena | 2 Turbo | 3 Ciudad | 4 Dos autos",20,41,16,RAYWHITE);}EndDrawing();}
static void Capture(const char* name,Camera3D cam,int mode){Render(cam,mode,false);TakeScreenshot((std::string("Vistas/")+name+".png").c_str());}
int main(int argc,char**argv){
 std::string cwd=GetWorkingDirectory();bool capture=argc>1&&std::string(argv[1])=="--capturar",verify=argc>1&&std::string(argv[1])=="--verificar";
 SetTraceLogLevel(LOG_WARNING);SetConfigFlags(FLAG_MSAA_4X_HINT);InitWindow(capture?1600:1280,capture?1000:800,"Autos de Globo - GLB");if(!IsWindowReady())return 1;
 ChangeDirectory(cwd.c_str());rlSetClipPlanes(.2,240);
 int loaded=0;for(int i=0;i<23;i++){
  std::string path=std::string("GLB/")+N[i]+".glb";if(!FileExists(path.c_str())){fprintf(stderr,"Falta %s\n",path.c_str());break;}
  M[i]=LoadModel(path.c_str());if(!M[i].meshCount)break;loaded++;printf("VALIDADO %s meshes=%d\n",N[i],M[i].meshCount);
 }
 if(loaded!=23){for(int i=0;i<loaded;i++)UnloadModel(M[i]);CloseWindow();return 2;}
 if(capture){
  Capture("Camara_del_juego",{{0,17.5f,9.5f},{0,0,.4f},{0,1,0},50,CAMERA_PERSPECTIVE},1);
  Capture("Turbo_y_placas",{{0,17.5f,9.5f},{0,0,.4f},{0,1,0},50,CAMERA_PERSPECTIVE},2);
  Capture("Ciudad_futurista",{{17,15,19},{0,2,-6},{0,1,0},54,CAMERA_PERSPECTIVE},3);
  Capture("Dos_autos",{{0,14,11},{0,0,0},{0,1,0},43,CAMERA_PERSPECTIVE},4);
  Capture("Detalle_autos",{{6,7,7},{1,.5,0},{0,1,0},45,CAMERA_PERSPECTIVE},1);
  for(int i=0;i<23;i++)Capture(N[i],ModelCam(i),i+10);
 }else{SetTargetFPS(60);int mode=1,frames=0;while(!WindowShouldClose()&&(!verify||frames++<10)){
  if(IsKeyPressed(KEY_ONE))mode=1;if(IsKeyPressed(KEY_TWO))mode=2;if(IsKeyPressed(KEY_THREE))mode=3;if(IsKeyPressed(KEY_FOUR))mode=4;
  Render({{0,17.5f,9.5f},{0,0,.4f},{0,1,0},50,CAMERA_PERSPECTIVE},mode,true);
 }}for(int i=0;i<loaded;i++)UnloadModel(M[i]);CloseWindow();return 0;
}
