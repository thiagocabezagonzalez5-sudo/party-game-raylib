#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include <cmath>
#include <cstdio>
#include <string>
static const char* N[]={"suelo_desguace","escombros","muro_fondo","engranaje_gigante","prensa_estructura","prensa_plato","auto_aplastado_rojo","auto_aplastado_azul","auto_aplastado_amarillo","cinta_transportadora","pila_chatarra","foco_industrial","tolva_equipo","garra_iman","garra_pinza","carro_grua","marca_garra","objeto_tuerca","objeto_engranaje","objeto_motor","objeto_bateria","objeto_cartucho"};
static Model M[22]{};
static Color colors[]={{235,82,79,255},{76,145,239,255},{80,207,132,255},{238,205,75,255}};
static void Part(int i,Vector3 p={0,0,0},float yaw=0,Vector3 s={1,1,1},Color tint=WHITE){DrawModelEx(M[i],p,{0,1,0},yaw,s,tint);}
static void Scene(int mode){
 if(mode>=10){Part(mode-10);return;}
 Part(0);Part(2,{0,0,-9.2f});
 for(int i=0;i<2;i++){
  float x=i?7.5f:-7.f;
  rlPushMatrix();rlTranslatef(x,3.4f,-8.0f);rlRotatef(i?18.f:-20.f,0,0,1);DrawModel(M[3],{0,0,0},1,WHITE);rlPopMatrix();
 }
 Part(4,{10.6f,0,-3.5f});Part(5,{10.6f,3.1f,-3.5f});
 for(int i=0;i<3;i++)Part(6+i,{-10.8f,i*.5f+.08f,-3.5f+(i%2)*.2f});
 for(int side:{-1,1})Part(9,{side*9.9f,0,1.6f});
 for(int i=0;i<9;i++)Part(10,{-10.f+2.5f*i,0,6.6f},0,{1,.8f+(i*7%9)/9.f,1});
 for(int i=0;i<4;i++)Part(11,{-8.f+5.4f*i,0,8});
 for(int i=0;i<4;i++)Part(12,{-6.9f+4.6f*i,0,-6.3f},0,{1,1,1},colors[i]);
 if(mode==3)return;
 for(int i=0;i<13;i++){
  int kind=i%5;float x=-7.7f+(i*19%155)/10.f,z=-4.4f+(i*37%95)/10.f;
  Part(17+kind,{x,.08f,z});
 }
 if(mode==4)return;
 Vector3 c={-2.2f,2.7f,1.5f};
 Part(13,c,0,{1,1,1},colors[0]);Part(15,{c.x,8,c.z});Part(16,{c.x,0,c.z},0,{1,1,1},colors[0]);
 for(int k=0;k<4;k++){float a=(k+.5f)*PI/2.f;Part(14,{c.x+cosf(a)*.42f,c.y,c.z+sinf(a)*.42f});}
 DrawLine3D({c.x,7.83f,c.z},{c.x,c.y+.35f,c.z},{40,40,46,255});
 if(mode==2){Part(19,{c.x,c.y-.8f,c.z});}
}
static Camera3D ModelCam(int i){
 BoundingBox b=GetModelBoundingBox(M[i]);Vector3 center=Vector3Scale(Vector3Add(b.min,b.max),.5f),size=Vector3Subtract(b.max,b.min);
 float r=fmaxf(size.x,fmaxf(size.y,size.z));Vector3 dir=Vector3Normalize({1.2f,1.0f,1.65f});
 if(i==0)dir=Vector3Normalize({.7f,2.0f,1.4f});
 if(i==2||i==3)dir=Vector3Normalize({.65f,.55f,1.4f});
 return {Vector3Add(center,Vector3Scale(dir,r*1.7f+1)),center,{0,1,0},38,CAMERA_PERSPECTIVE};
}
static void Render(Camera3D cam,int mode,bool hud){
 BeginDrawing();ClearBackground({188,169,140,255});BeginMode3D(cam);Scene(mode);EndMode3D();
 if(hud){DrawRectangle(0,0,1280,66,{35,40,47,230});DrawText("GRUA DE CHATARRA / Modelos GLB",20,9,24,RAYWHITE);DrawText("1 Escena | 2 Objeto capturado | 3 Maquinaria | 4 Objetos",20,40,16,RAYWHITE);}EndDrawing();
}
static void Capture(const char* name,Camera3D cam,int mode){Render(cam,mode,false);TakeScreenshot((std::string("Vistas/")+name+".png").c_str());}
int main(int argc,char**argv){
 std::string cwd=GetWorkingDirectory();bool capture=argc>1&&std::string(argv[1])=="--capturar",verify=argc>1&&std::string(argv[1])=="--verificar";
 SetTraceLogLevel(LOG_WARNING);SetConfigFlags(FLAG_MSAA_4X_HINT);InitWindow(capture?1600:1280,capture?1000:800,"Grua de Chatarra - GLB");if(!IsWindowReady())return 1;
 ChangeDirectory(cwd.c_str());rlSetClipPlanes(.2,240);
 int loaded=0;for(int i=0;i<22;i++){
  std::string path=std::string("GLB/")+N[i]+".glb";if(!FileExists(path.c_str())){fprintf(stderr,"Falta %s\n",path.c_str());break;}
  M[i]=LoadModel(path.c_str());if(!M[i].meshCount)break;loaded++;printf("VALIDADO %s meshes=%d\n",N[i],M[i].meshCount);
 }
 if(loaded!=22){for(int i=0;i<loaded;i++)UnloadModel(M[i]);CloseWindow();return 2;}
 if(capture){
  Capture("Camara_del_juego",{{0,16,11.5f},{0,0,-.3f},{0,1,0},50,CAMERA_PERSPECTIVE},1);
  Capture("Objeto_capturado",{{0,16,11.5f},{0,0,-.3f},{0,1,0},50,CAMERA_PERSPECTIVE},2);
  Capture("Maquinaria",{{15,13,17},{0,1,-2},{0,1,0},54,CAMERA_PERSPECTIVE},3);
  Capture("Objetos_en_pozo",{{-2,13,14},{0,0,0},{0,1,0},45,CAMERA_PERSPECTIVE},4);
  Capture("Garra_detalle",{{2,6,6},{-2.2f,2.7f,1.5f},{0,1,0},45,CAMERA_PERSPECTIVE},1);
  for(int i=0;i<22;i++)Capture(N[i],ModelCam(i),i+10);
 }else{SetTargetFPS(60);int mode=1,frames=0;while(!WindowShouldClose()&&(!verify||frames++<10)){
  if(IsKeyPressed(KEY_ONE))mode=1;if(IsKeyPressed(KEY_TWO))mode=2;if(IsKeyPressed(KEY_THREE))mode=3;if(IsKeyPressed(KEY_FOUR))mode=4;
  Render({{0,16,11.5f},{0,0,-.3f},{0,1,0},50,CAMERA_PERSPECTIVE},mode,true);
 }}for(int i=0;i<loaded;i++)UnloadModel(M[i]);CloseWindow();return 0;
}
