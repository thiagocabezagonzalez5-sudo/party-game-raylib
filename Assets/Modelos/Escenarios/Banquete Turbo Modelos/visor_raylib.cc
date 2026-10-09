#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include <cstdio>
#include <string>
static const char* N[]={"comedor_orbital","ventanal_tierra","tierra","cielo_estrellado","techo_comedor","tuberias_techo","lampara_colgante","mesa_magnetica","bandeja","taburete","robot_camarero","tubo_comida_normal","tubo_comida_picante","tubo_comida_dorada","consola_lateral"};
static Model M[15]{};
static std::string ROOT;
static void Part(int i,Vector3 p={0,0,0},Vector3 s={1,1,1},Color tint=WHITE){DrawModelEx(M[i],p,{0,1,0},0,s,tint);}
static Camera3D Cam(int mode){
 if(mode==1)return {{0,4.6f,12.5f},{0,1.5f,0},{0,1,0},45,CAMERA_PERSPECTIVE};
 if(mode==2)return {{8,5,9},{0,1,-.5f},{0,1,0},43,CAMERA_PERSPECTIVE};
 return {{0,12,23},{0,2,-3},{0,1,0},50,CAMERA_PERSPECTIVE};
}
static void Scene(int mode){
 if(mode>=10){Part(mode-10);return;}
 Part(3,{0,6,-36});Part(2,{-7,7,-24});Part(0);Part(1,{0,0,-8});
 Part(4,{0,11,-1});Part(5,{0,9,-4});
 for(int i=-2;i<=2;i++)Part(6,{i*7.f,10,-1});
 for(int i=0;i<4;i++){
  float x=(i-1.5f)*4.2f;
  Part(7,{x,0,1.3f});Part(8,{x,.99f,1.3f});
  Part(9,{x,0,-.3f},{1,1,1},i==0?RED:(i==1?BLUE:(i==2?GREEN:GOLD)));
  Part(mode==2?12:(i==3?13:11),{x,1.23f,1.3f});
  DrawCube({x,1,-.3f},.55f,1.6f,.55f,i==0?RED:(i==1?BLUE:(i==2?GREEN:GOLD)));
 }
 Part(10,{0,2.5f,-3});Part(14,{-17,0,-3});Part(14,{17,0,-3});
}
static Camera3D ModelCam(int i){
 BoundingBox b=GetModelBoundingBox(M[i]);Vector3 c=Vector3Scale(Vector3Add(b.min,b.max),.5f);
 Vector3 d=Vector3Subtract(b.max,b.min);float r=fmaxf(d.x,fmaxf(d.y,d.z));
 Vector3 dir=Vector3Normalize({.7f,1.05f,1.6f});
 if(i==2||i==3||i==1)dir=Vector3Normalize({.2f,.15f,1.6f});
 if(i==0||i==4)dir=Vector3Normalize({.25f,1.8f,1.2f});
 return {Vector3Add(c,Vector3Scale(dir,r*1.45f+1)),c,{0,1,0},42,CAMERA_PERSPECTIVE};
}
static void Render(Camera3D camera,int mode,bool hud){
 BeginDrawing();ClearBackground({13,20,39,255});BeginMode3D(camera);Scene(mode);EndMode3D();
 if(hud){DrawRectangle(0,0,1280,68,{12,22,36,238});DrawText("BANQUETE TURBO / Modelos GLB",20,8,25,RAYWHITE);DrawText("1 Camara del juego | 2 Raciones picantes | 3 Comedor orbital",20,42,16,RAYWHITE);}EndDrawing();
}
static void Shot(const char* name,Camera3D camera,int mode){Render(camera,mode,false);TakeScreenshot((ROOT+"/Vistas/"+name+".png").c_str());}
int main(int argc,char** argv){
 std::string cwd=argc>2?argv[2]:GetWorkingDirectory();
 ROOT=cwd;
 bool capture=argc>1&&std::string(argv[1])=="--capturar",verify=argc>1&&std::string(argv[1])=="--verificar";
 SetTraceLogLevel(LOG_WARNING);SetConfigFlags(FLAG_MSAA_4X_HINT);InitWindow(capture?1600:1280,capture?1000:800,"Banquete Turbo - GLB");
 if(!IsWindowReady())return 1;ChangeDirectory(cwd.c_str());rlSetClipPlanes(.2,240);
 int loaded=0;for(int i=0;i<15;i++){
  std::string path=std::string("GLB/")+N[i]+".glb";
  if(!FileExists(path.c_str())){fprintf(stderr,"Falta %s\n",path.c_str());break;}
  M[i]=LoadModel(path.c_str());if(!M[i].meshCount)break;
  loaded++;printf("VALIDADO %s meshes=%d\n",N[i],M[i].meshCount);
 }
 if(loaded!=15){for(int i=0;i<loaded;i++)UnloadModel(M[i]);CloseWindow();return 2;}
 if(capture){
  Shot("Camara_del_juego",Cam(1),1);Shot("Raciones_picantes",Cam(2),2);Shot("Comedor_orbital",Cam(3),3);
  for(int i=0;i<15;i++)Shot(N[i],ModelCam(i),i+10);
 }else{SetTargetFPS(60);int mode=1,frames=0;while(!WindowShouldClose()&&(!verify||frames++<10)){
  if(IsKeyPressed(KEY_ONE))mode=1;if(IsKeyPressed(KEY_TWO))mode=2;if(IsKeyPressed(KEY_THREE))mode=3;
  Render(Cam(mode),mode,true);
 }}for(int i=0;i<loaded;i++)UnloadModel(M[i]);CloseWindow();return 0;
}
