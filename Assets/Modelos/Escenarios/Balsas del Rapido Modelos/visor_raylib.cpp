#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include <cmath>
#include <cstdio>
#include <string>
static const char* N[]={"rio_tramo","isla_tramo","orilla_tramo","arbol_selva","liana","palmera_isla","ruina_musgosa","cascada_orilla","espuma_rapido","roca_obstaculo","tronco_flotante","divisor_rapido","remolino","banana_boost","loro_rojo","loro_azul","loro_amarillo","loro_ala_roja","loro_ala_azul","loro_ala_amarilla","balsa","remo","arco_meta","linea_meta"};
static Model M[24]{};
static void Part(int i,Vector3 p={0,0,0},float yaw=0,Vector3 s={1,1,1},Color tint=WHITE){DrawModelEx(M[i],p,{0,1,0},yaw,s,tint);}
static void Scene(int mode){
 if(mode>=10){Part(mode-10);return;}
 float focus=mode==3?120.f:(mode==2?67.f:20.f);
 for(int k=0;k<7;k++){
  float p=10+20*k,z=-p;if(fabsf(p-focus)>60)continue;
  for(int side:{-1,1}){
   Part(0,{side*8.f,0,z});Part(2,{side*17.6f,0,z},side==-1?180:0);
  }Part(1,{0,0,z});
 }
 for(int k=0;k<20;k++){
  float p=3+k*7;if(fabsf(p-focus)>42)continue;
  for(int side:{-1,1})Part(3,{side*(18.f+k%3),1.3f,-p},0,{.7f,.76f,.7f});
 }
 for(int k=0;k<13;k++){
  float p=7+k*11;if(fabsf(p-focus)>43)continue;
  Part(5,{k%2?.9f:-.9f,0,-p});
 }
 for(float p:{18.f,41.f,63.f,88.f,106.f})if(fabsf(p-focus)<42)Part(6,{0,0,-p});
 for(float p:{30.f,95.f,113.f})if(fabsf(p-focus)<39)for(int side:{-1,1})Part(7,{side*13.8f,0,-p});
 if(focus>42&&focus<100){
  for(int side:{-1,1}){
   Part(11,{side*8.f,0,-67});
   for(int i=0;i<6;i++)Part(8,{side*8.f-3.2f,0,-56.f-4.8f*i});
   for(int i=0;i<4;i++)Part(9,{side*8.f-2.5f-(i%2)*1.2f,0,-57.f-i*6},0,{.65f,.65f,.65f});
   Part(13,{side*8.f+3.1f,.26f,-60});Part(13,{side*8.f+3.1f,.26f,-72});
  }
 }
 if(focus<45){
  for(int side:{-1,1}){
   float c=side*8.f;Part(9,{c-2.4f,0,-14});Part(9,{c+2.8f,0,-17});
   Part(13,{c+.4f,.25f,-20});Part(10,{c+1.4f,0,-25});Part(12,{c-2.2f,0,-30});
  }
 }
 if(focus>88){
  for(int side:{-1,1}){
   float c=side*8.f;Part(9,{c-2.5f,0,-90});Part(10,{c+1.8f,0,-94});Part(12,{c-1.8f,0,-98});
   Part(13,{c+2.4f,.28f,-104});Part(10,{c-1.5f,0,-108});Part(9,{c+.9f,0,-112});
  }
 }
 if(focus>65){Part(22,{0,0,-120});for(int side:{-1,1})Part(23,{side*8.f,0,-120});}
 float p0=focus<45?15:(focus<100?64:115),p1=p0+1.8f;
 for(int side:{-1,1}){
  float p=side==-1?p0:p1,x=side*8.f+(focus>45&&focus<100?(side==-1?-3.2f:3.1f):0);
  Part(20,{x,.12f,-p},side==-1?-6.f:5.f,{1,1,1},side==-1?Color{255,150,40,255}:Color{240,80,170,255});
  for(int s:{-1,1})Part(21,{x+s*.9f,.62f,-p},s==-1?180:0);
 }
 for(int k=0;k<6;k++){
  float p=focus-12.f+k*7;if(p<0||p>130)continue;
  float x=(k%2?1.f:-1.f)*15.5f,y=5.f+.3f*(k%3);
  int ci=k%3;Part(14+ci,{x,y,-p});Part(17+ci,{x-.38f,y+.14f,-p});Part(17+ci,{x+.38f,y+.14f,-p},180);
 }
}
static Camera3D CameraFocus(float f){return {{0,11,f*-1+15.5f},{0,.4f,-f-5.5f},{0,1,0},56,CAMERA_PERSPECTIVE};}
static Camera3D ModelCam(int i){BoundingBox b=GetModelBoundingBox(M[i]);Vector3 center=Vector3Scale(Vector3Add(b.min,b.max),.5f),size=Vector3Subtract(b.max,b.min);float r=fmaxf(size.x,fmaxf(size.y,size.z));Vector3 dir=Vector3Normalize({.8f,1.2f,1.55f});return {Vector3Add(center,Vector3Scale(dir,r*1.65f+1)),center,{0,1,0},39,CAMERA_PERSPECTIVE};}
static void Render(Camera3D cam,int mode,bool hud){BeginDrawing();ClearBackground({126,183,174,255});BeginMode3D(cam);Scene(mode);EndMode3D();if(hud){DrawRectangle(0,0,1280,66,{28,72,77,230});DrawText("BALSAS DEL RAPIDO / Modelos GLB",20,9,23,RAYWHITE);DrawText("1 Inicio | 2 Rapidos | 3 Meta",20,41,16,RAYWHITE);}EndDrawing();}
static void Capture(const char* name,Camera3D cam,int mode){Render(cam,mode,false);TakeScreenshot((std::string("Vistas/")+name+".png").c_str());}
int main(int argc,char**argv){
 std::string cwd=GetWorkingDirectory();bool capture=argc>1&&std::string(argv[1])=="--capturar",verify=argc>1&&std::string(argv[1])=="--verificar";
 SetTraceLogLevel(LOG_WARNING);SetConfigFlags(FLAG_MSAA_4X_HINT);InitWindow(capture?1600:1280,capture?1000:800,"Balsas del Rapido - GLB");if(!IsWindowReady())return 1;
 ChangeDirectory(cwd.c_str());rlSetClipPlanes(.2,260);
 int loaded=0;for(int i=0;i<24;i++){
  std::string path=std::string("GLB/")+N[i]+".glb";if(!FileExists(path.c_str())){fprintf(stderr,"Falta %s\n",path.c_str());break;}
  M[i]=LoadModel(path.c_str());if(!M[i].meshCount)break;loaded++;printf("VALIDADO %s meshes=%d\n",N[i],M[i].meshCount);
 }
 if(loaded!=24){for(int i=0;i<loaded;i++)UnloadModel(M[i]);CloseWindow();return 2;}
 if(capture){
  Capture("Inicio_camara_del_juego",CameraFocus(20),1);
  Capture("Rapidos_y_divisor",CameraFocus(67),2);
  Capture("Meta",CameraFocus(120),3);
  Capture("Balsas_de_cerca",{{13,9,-6},{0,.4f,-18},{0,1,0},48,CAMERA_PERSPECTIVE},1);
  Capture("Vista_general_rapidos",{{22,21,-40},{0,0,-67},{0,1,0},54,CAMERA_PERSPECTIVE},2);
  for(int i=0;i<24;i++)Capture(N[i],ModelCam(i),i+10);
 }else{SetTargetFPS(60);int mode=1,frames=0;while(!WindowShouldClose()&&(!verify||frames++<10)){
  if(IsKeyPressed(KEY_ONE))mode=1;if(IsKeyPressed(KEY_TWO))mode=2;if(IsKeyPressed(KEY_THREE))mode=3;
  Render(CameraFocus(mode==3?120.f:(mode==2?67.f:20.f)),mode,true);
 }}for(int i=0;i<loaded;i++)UnloadModel(M[i]);CloseWindow();return 0;
}
