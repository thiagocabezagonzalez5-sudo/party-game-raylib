#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include <cstdio>
#include <cmath>
#include <string>
const char* root="./";
const char* names[]={"carrusel_base","carrusel_columna","carrusel_techo","caballito","poste_caballito","bombilla","taza","valla_tramo","noria_soporte","noria_rueda","noria_cabina","montana_rusa_vias","montana_rusa_carro","puesto_feria","grada","globo","arena"};
Model models[17];
// Indices de primitive segun manifest.json. Usar meshMaterial evita asumir
// el indice interno de material que asigna la version de raylib.
void draw(int i,Vector3 p={0,0,0},float angle=0,Vector3 axis={0,1,0},Color tint=WHITE){
 int primitive=-1;
 if(i==6)primitive=2; // Taza: COLOR_DINAMICO
 if(i==10)primitive=3; // Cabina: COLOR_DINAMICO
 if(i==15)primitive=0; // Globo: COLOR_DINAMICO
 if(primitive>=0){
  int material=models[i].meshMaterial[primitive];
  Color previo=models[i].materials[material].maps[MATERIAL_MAP_DIFFUSE].color;
  models[i].materials[material].maps[MATERIAL_MAP_DIFFUSE].color=tint;
  DrawModelEx(models[i],p,axis,angle,{1,1,1},WHITE);
  models[i].materials[material].maps[MATERIAL_MAP_DIFFUSE].color=previo;
 }else DrawModelEx(models[i],p,axis,angle,{1,1,1},tint);
}
void carousel(Vector3 p,float a=0){
 draw(0,p); draw(1,p);draw(2,p,a);
 for(int i=0;i<6;i++){
  float ang=(a+i*60)*DEG2RAD; Vector3 q={p.x+1.55f*cosf(ang),p.y,p.z+1.55f*sinf(ang)};
  draw(4,q);q.y+=1.2f+.10f*sinf((float)GetTime()*4+i*1.5f);draw(3,q,-a-i*60-90);
 }
 for(int i=0;i<12;i++){float ang=(a+i*30)*DEG2RAD;draw(5,{p.x+2.4f*cosf(ang),p.y+3.25f,p.z+2.4f*sinf(ang)},0,{0,1,0},ColorFromHSV(i*30,.5,1));}
}
void ferris(Vector3 p,float a=12){
 draw(8,p);Vector3 center={p.x,p.y+6.4f,p.z};draw(9,center,a,{0,0,1});
 for(int i=0;i<8;i++){
  float t=(a+i*45)*DEG2RAD;
  draw(10,{center.x+5*cosf(t),center.y+5*sinf(t),center.z+.02f},0,{0,1,0},ColorFromHSV(i*45,.35,1));
 }
}
void scene(){
 draw(16,{0,-.05f,0});carousel({0,-.05f,0},fmodf((float)GetTime()*25,360));
 for(int i=0;i<32;i++){float a=(i+.5f)*2*PI/32;draw(7,{8.65f*cosf(a),-.05f,8.65f*sinf(a)},-a*RAD2DEG-90);}
 for(int i=0;i<3;i++){float a=i*2*PI/3+.4f;draw(6,{5.3f*cosf(a),-.05f,5.3f*sinf(a)},i*90,{0,1,0},ColorFromHSV(i*120+25,.30,1));}
 draw(14,{11.1f,0,.3f});draw(14,{-11.1f,0,.3f},180);
 for(int i=0;i<3;i++)draw(13,{-15+i*5.2f,0,-11});
 ferris({15,0,-12.5f},(float)GetTime()*8);draw(11,{0,0,-17});
 float t=.22f;float y=5+3.2f*sinf(t*9)+1.6f*sinf(t*23);float slope=(28.8f*cosf(t*9)+36.8f*cosf(t*23))/42;
 draw(12,{-21+42*t,y+.09f,-17},atanf(slope)*RAD2DEG,{0,0,1});
 for(int i=0;i<10;i++){float a=PI+.25f+i*(PI-.5f)/9;draw(15,{10.2f*cosf(a),3.0f+i%3*.35f,10.2f*sinf(a)},0,{0,1,0},ColorFromHSV(i*36,.4,1));}
}

// Visor independiente. Ejecutarlo desde la carpeta que contiene GLB/.
// 1: escena. 2: carrusel. 3: caballito. 4: taza. 5: noria.
// Izquierda/derecha: girar vista. Escape: cerrar.
int main(int argc, char** argv){
 SetConfigFlags(FLAG_MSAA_4X_HINT); InitWindow(1280,800,"Ultimo Asiento - Modelos GLB");
 if(!IsWindowReady()) return 1;
 rlSetClipPlanes(0.1,200); // Rango suficiente para el visor y mejor precision de profundidad.
 SetTargetFPS(60);
 int cargados=0;
 for(int i=0;i<17;i++){
  std::string path=std::string(root)+"GLB/"+names[i]+".glb";
  if(!FileExists(path.c_str())){
   for(int j=0;j<cargados;j++)UnloadModel(models[j]);CloseWindow();
   std::fprintf(stderr,"No se encontro %s. Ejecutar desde la carpeta del paquete.\n",path.c_str());return 2;
  }
  models[i]=LoadModel(path.c_str());cargados++;
  if(models[i].meshCount<1){for(int j=0;j<cargados;j++)UnloadModel(models[j]);CloseWindow();return 3;}
 }
 int modo=1;float azimut=.42f;
 int fotogramas=0;bool verificar=argc>1 && std::string(argv[1])=="--verificar";
 while(!WindowShouldClose() && (!verificar || fotogramas<10)){
  fotogramas++;
  for(int i=1;i<=5;i++)if(IsKeyPressed(KEY_ONE+i-1))modo=i;
  if(IsKeyDown(KEY_LEFT))azimut-=GetFrameTime();
  if(IsKeyDown(KEY_RIGHT))azimut+=GetFrameTime();
  float t=(float)GetTime();Camera3D cam{};
  cam.up={0,1,0};cam.fovy=43;cam.projection=CAMERA_PERSPECTIVE;
  float radio=45,altura=25;cam.target={0,2,-5};
  if(modo==2){radio=9;altura=5;cam.target={0,2.1f,0};}
  if(modo==3){radio=2.7f;altura=1.0f;cam.target={0,.1f,0};}
  if(modo==4){radio=4.8f;altura=2.4f;cam.target={0,.3f,0};}
  if(modo==5){radio=23;altura=11;cam.target={0,5.6f,0};}
  cam.position={cam.target.x+radio*sinf(azimut),altura,cam.target.z+radio*cosf(azimut)};
  BeginDrawing();ClearBackground({26,31,49,255});BeginMode3D(cam);
  if(modo==1){DrawPlane({0,-.54f,0},{140,140},{35,39,58,255});scene();}
  if(modo==2)carousel({0,0,0},fmodf(t*25,360));
  if(modo==3)draw(3);
  if(modo==4)draw(6,{0,0,0},0,{0,1,0},ColorFromHSV(fmodf(t*35,360),.5f,1));
  if(modo==5)ferris({0,0,0},t*8);
  EndMode3D();DrawRectangle(0,0,1280,64,{17,22,35,230});
  DrawText("ULTIMO ASIENTO / Modelos originales",22,10,22,RAYWHITE);
  DrawText("1 Escena | 2 Carrusel | 3 Caballito | 4 Taza | 5 Noria | Flechas: girar",22,39,16,LIGHTGRAY);
  EndDrawing();
 }
 for(int i=0;i<cargados;i++)UnloadModel(models[i]);CloseWindow();return 0;
}
