#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include <cmath>
#include <cstdio>
#include <string>
static const char* N[]={"suelo_arcade","pared_arcade","gabinete_arcade","marco_jugador","boton_detener","tambor_rodillo","simbolo_triangulo","simbolo_circulo","simbolo_cuadrado","simbolo_rombo","simbolo_estrella","linea_comodin","columna_led_cian","columna_led_rosa","holograma_cubo","holograma_esfera","holograma_piramide","letrero_cian","letrero_rosa","letrero_verde","letrero_naranja"};
static Model M[21]{};static Color PLAYER[]={{235,80,80,255},{80,140,240,255},{90,205,115,255},{245,205,70,255}};
static void Part(int i,Vector3 p={0,0,0},Color tint=WHITE){DrawModel(M[i],p,1,tint);}
static void Reel(float cx,float angle,int machine,int r){
 rlPushMatrix();rlTranslatef(cx,3,0);rlRotatef(-angle*RAD2DEG,1,0,0);Part(5);
 for(int k=0;k<10;k++){
  float a=k*6.2831853f/10;int symbol=(k*3+r+machine)%5;
  rlPushMatrix();rlTranslatef(0,1.73f*sinf(a),1.73f*cosf(a));rlRotatef(-a*RAD2DEG,1,0,0);Part(6+symbol);rlPopMatrix();
 }
 rlPopMatrix();
}
static void Machine(int n,int count,bool spin){
 float x=(n-(count-1)*.5f)*5.8f;Part(2,{x,0,0});Part(3,{x,0,0},PLAYER[n]);Part(4,{x,1.2f,1.9f});
 for(int r=0;r<3;r++)Reel(x+(r-1)*1.05f,spin?.28f*(r+n):0.0f,n,r);
 Part(11,{x,3,1.84f});
 DrawCube({x,.72f,3.7f},.65f,1.4f,.65f,PLAYER[n]);
 DrawCube({x,1.45f,3.7f},.58f,.16f,.58f,WHITE);
}
static void Scene(int mode){
 if(mode>=10){Part(mode-10);return;}
 Part(0);Part(1);
 for(int c=0;c<9;c++)Part(c%2?13:12,{-24.f+c*6.f,0,-8.5f});
 for(int i=0;i<3;i++)Part(14+i,{-12.f+i*12.f,9.5f,-6});
 for(int i=0;i<4;i++)Part(17+i,{-18.f+i*12.f,12.3f,-8.4f});
 if(mode==4)return;
 int count=mode==3?2:4;
 for(int i=0;i<count;i++)Machine(i,count,mode==2);
}
static Camera3D CamModel(int i){
 BoundingBox b=GetModelBoundingBox(M[i]);Vector3 c=Vector3Scale(Vector3Add(b.min,b.max),.5f),v=Vector3Subtract(b.max,b.min);
 float r=fmaxf(v.x,fmaxf(v.y,v.z));Vector3 dir=Vector3Normalize({.7f,.5f,1.55f});
 if(i==0||i==1)dir=Vector3Normalize({.35f,1.0f,1.7f});
 return {Vector3Add(c,Vector3Scale(dir,r*1.7f+1.1f)),c,{0,1,0},38,CAMERA_PERSPECTIVE};
}
static void Render(Camera3D cam,int mode,bool hud){BeginDrawing();ClearBackground({6,4,16,255});BeginMode3D(cam);Scene(mode);EndMode3D();if(hud){DrawRectangle(0,0,1280,64,{26,18,51,240});DrawText("RODILLOS NEON / Modelos GLB",20,8,23,RAYWHITE);DrawText("1 Cuatro maquinas | 2 Giro | 3 Dos maquinas | 4 Sala",20,39,16,LIGHTGRAY);}EndDrawing();}
static void Capture(const char* n,Camera3D cam,int mode){Render(cam,mode,false);TakeScreenshot((std::string("Vistas/")+n+".png").c_str());}
int main(int argc,char** argv){
 std::string cwd=GetWorkingDirectory();bool capture=argc>1&&std::string(argv[1])=="--capturar",verify=argc>1&&std::string(argv[1])=="--verificar";
 SetTraceLogLevel(LOG_WARNING);SetConfigFlags(FLAG_MSAA_4X_HINT);InitWindow(capture?1600:1280,capture?1000:800,"Rodillos Neon - modelos GLB");
 if(!IsWindowReady())return 1;ChangeDirectory(cwd.c_str());rlSetClipPlanes(.2,220);
 int loaded=0;for(int i=0;i<21;i++){
  std::string path=std::string("GLB/")+N[i]+".glb";if(!FileExists(path.c_str())){fprintf(stderr,"Falta %s\n",path.c_str());break;}
  M[i]=LoadModel(path.c_str());loaded++;if(!M[i].meshCount)break;printf("VALIDADO %s meshes=%d\n",N[i],M[i].meshCount);
 }
 if(loaded!=21){for(int i=0;i<loaded;i++)UnloadModel(M[i]);CloseWindow();return 2;}
 if(capture){
  Capture("Cuatro_maquinas",{{0,6.5f,21.5f},{0,2.4f,1},{0,1,0},45,CAMERA_PERSPECTIVE},1);
  Capture("Rodillos_girando",{{0,6.5f,21.5f},{0,2.4f,1},{0,1,0},45,CAMERA_PERSPECTIVE},2);
  Capture("Dos_maquinas",{{0,5.4f,14.5f},{0,2.4f,1},{0,1,0},45,CAMERA_PERSPECTIVE},3);
  Capture("Sala_arcade",{{19,15,25},{0,7,-6},{0,1,0},58,CAMERA_PERSPECTIVE},4);
  Capture("Detalle_maquina",{{4.8f,5.1f,8.5f},{2.9f,3,0},{0,1,0},48,CAMERA_PERSPECTIVE},3);
  for(int i=0;i<21;i++)Capture(N[i],CamModel(i),i+10);
 }else{SetTargetFPS(60);int mode=1,frames=0;while(!WindowShouldClose()&&(!verify||frames++<10)){
  if(IsKeyPressed(KEY_ONE))mode=1;if(IsKeyPressed(KEY_TWO))mode=2;if(IsKeyPressed(KEY_THREE))mode=3;if(IsKeyPressed(KEY_FOUR))mode=4;
  Camera3D cam={{0,6.5f,21.5f},{0,2.4f,1},{0,1,0},45,CAMERA_PERSPECTIVE};
  if(mode==3)cam={{0,5.4f,14.5f},{0,2.4f,1},{0,1,0},45,CAMERA_PERSPECTIVE};
  Render(cam,mode,true);
 }}for(int i=0;i<loaded;i++)UnloadModel(M[i]);CloseWindow();return 0;
}
