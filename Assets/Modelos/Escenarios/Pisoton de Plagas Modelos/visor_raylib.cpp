#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include <cmath>
#include <cstdio>
#include <string>
static const char* N[]={"suelo_jardin","brizna_gigante","flor_fondo_rosa","flor_fondo_amarilla","flor_fondo_violeta","flor_brote","flor_abierta","aro_flor","seta","regadera","piedra","cerca_fondo","cerca_lateral","madriguera","aro_aviso","escarabajo","oruga_cabeza","oruga_segmento","babosa_dorada","avispa_cuerpo","avispa_ala","gota_rocio"};
static Model M[22]{};
static void Part(int i,Vector3 p={0,0,0},float yaw=0,Vector3 s={1,1,1},Color tint=WHITE){DrawModelEx(M[i],p,{0,1,0},yaw,s,tint);}
static void Scene(int mode){
 if(mode>=10){Part(mode-10);return;}
 Part(0);Part(11,{0,0,-6.9f});Part(12,{-10.4f,0,0},90);Part(12,{10.4f,0,0},90);
 for(int k=0;k<20;k++)Part(1,{-19.f+2.f*k,0,-11.7f-(k%3)*.45f},0,{.75f,1.0f+.1f*(k%3),.75f});
 for(int k=0;k<12;k++)Part(1,{k%2?11.9f:-11.9f,0,-6.4f+1.1f*k},0,{.8f,.85f,.8f});
 Part(2,{-7,0,-9.4f});Part(3,{1.5f,0,-10.6f});Part(4,{8,0,-9.4f});
 float sx[]={-10.8f,10.9f,10.6f},sz[]={3.8f,2.6f,-1.2f};for(int s=0;s<3;s++)Part(8,{sx[s],0,sz[s]});
 Part(9,{-11.2f,0,-3});float rx[]={10.4f,-10.5f,6.5f,-5.5f,11.4f},rz[]={5.2f,-.2f,7.2f,7.4f,-4.5f};for(int k=0;k<5;k++)Part(10,{rx[k],0,rz[k]});
 float hx[]={-7.2f,0,7.2f,-7.2f,0,7.2f},hz[]={-4.3f,-4.7f,-4.3f,4.3f,4.7f,4.3f};
 for(int k=0;k<6;k++)Part(13,{hx[k],0,hz[k]});
 if(mode==3){Part(5,{-.2f,0,.5f});Part(6,{3,0,.4f});Part(7,{3,0,.4f});return;}
 for(int k=0;k<10;k++)Part(21,{-8.f+(k*19%160)/10.f,0,-4.2f+(k*31%90)/10.f});
 Part(6,{2.7f,0,.3f});Part(7,{2.7f,0,.3f});
 if(mode==4)return;
 Part(14,{-7.2f,0,-4.3f},0,{1,1,1},ORANGE);
 Part(15,{-3.6f,0,-2.1f},22);Part(15,{4.8f,0,3.5f},135);
 Part(16,{-.3f,0,2.7f},-15);Part(17,{-1.0f,0,2.5f},-15);Part(17,{-1.6f,0,2.3f},-15);
 Part(18,{5.9f,0,-2.0f},-40);
 Vector3 p={-.5f,1.02f,-1.9f};Part(19,p,20);Part(20,{p.x,p.y+.19f,p.z-.23f},20);Part(20,{p.x,p.y+.19f,p.z+.23f},200);
 if(mode==2){Part(18,{-4.5f,0,1.3f});Part(19,{3.6f,1.04f,2.6f});}
}
static Camera3D ModelCam(int i){
 BoundingBox b=GetModelBoundingBox(M[i]);Vector3 center=Vector3Scale(Vector3Add(b.min,b.max),.5f),size=Vector3Subtract(b.max,b.min);
 float r=fmaxf(size.x,fmaxf(size.y,size.z));Vector3 dir=Vector3Normalize({.9f,1.1f,1.65f});
 if(i==0||i==7||i==14)dir=Vector3Normalize({.7f,1.9f,1.3f});
 return {Vector3Add(center,Vector3Scale(dir,r*1.65f+1)),center,{0,1,0},39,CAMERA_PERSPECTIVE};
}
static void Render(Camera3D cam,int mode,bool hud){BeginDrawing();ClearBackground({163,202,139,255});BeginMode3D(cam);Scene(mode);EndMode3D();if(hud){DrawRectangle(0,0,1280,66,{37,85,53,233});DrawText("PISOTON DE PLAGAS / Modelos GLB",20,9,23,RAYWHITE);DrawText("1 Escena | 2 Plagas | 3 Flor y jardin | 4 Escenario",20,41,16,RAYWHITE);}EndDrawing();}
static void Capture(const char* name,Camera3D cam,int mode){Render(cam,mode,false);TakeScreenshot((std::string("Vistas/")+name+".png").c_str());}
int main(int argc,char**argv){
 std::string cwd=GetWorkingDirectory();bool capture=argc>1&&std::string(argv[1])=="--capturar",verify=argc>1&&std::string(argv[1])=="--verificar";
 SetTraceLogLevel(LOG_WARNING);SetConfigFlags(FLAG_MSAA_4X_HINT);InitWindow(capture?1600:1280,capture?1000:800,"Pisoton de Plagas - GLB");if(!IsWindowReady())return 1;
 ChangeDirectory(cwd.c_str());rlSetClipPlanes(.2,240);
 int loaded=0;for(int i=0;i<22;i++){
  std::string path=std::string("GLB/")+N[i]+".glb";if(!FileExists(path.c_str())){fprintf(stderr,"Falta %s\n",path.c_str());break;}
  M[i]=LoadModel(path.c_str());if(!M[i].meshCount)break;loaded++;printf("VALIDADO %s meshes=%d\n",N[i],M[i].meshCount);
 }
 if(loaded!=22){for(int i=0;i<loaded;i++)UnloadModel(M[i]);CloseWindow();return 2;}
 if(capture){
  Capture("Camara_del_juego",{{0,15,10.5f},{0,0,.6f},{0,1,0},50,CAMERA_PERSPECTIVE},1);
  Capture("Plagas_en_jardin",{{0,15,10.5f},{0,0,.6f},{0,1,0},50,CAMERA_PERSPECTIVE},2);
  Capture("Flor_y_jardin",{{14,13,12},{0,2,-2},{0,1,0},58,CAMERA_PERSPECTIVE},3);
  Capture("Escenario",{{0,15,10.5f},{0,0,.6f},{0,1,0},50,CAMERA_PERSPECTIVE},4);
  Capture("Detalle_plagas",{{5,6,7},{0,.3,0},{0,1,0},46,CAMERA_PERSPECTIVE},1);
  for(int i=0;i<22;i++)Capture(N[i],ModelCam(i),i+10);
 }else{SetTargetFPS(60);int mode=1,frames=0;while(!WindowShouldClose()&&(!verify||frames++<10)){
  if(IsKeyPressed(KEY_ONE))mode=1;if(IsKeyPressed(KEY_TWO))mode=2;if(IsKeyPressed(KEY_THREE))mode=3;if(IsKeyPressed(KEY_FOUR))mode=4;
  Render({{0,15,10.5f},{0,0,.6f},{0,1,0},50,CAMERA_PERSPECTIVE},mode,true);
 }}for(int i=0;i<loaded;i++)UnloadModel(M[i]);CloseWindow();return 0;
}
