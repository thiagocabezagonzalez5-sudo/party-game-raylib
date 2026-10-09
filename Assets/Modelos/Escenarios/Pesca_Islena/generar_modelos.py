"""GLB procedurales originales para Pesca Isleña. Python 3 estándar."""
from geometria_glb import *
import geometria_glb as glb

glb.PALETTE.update({'oceano':'#126a92','oceano_luz':'#1888ab','arena':'#e8cf95','arena_clara':'#f4e0ab','arena_sombra':'#c6ae77','fondo':'#b5a684','laguna':'#209eaf','agua':'#58d3d4','agua_luz':'#b1f3e4','bambu':'#dfbc6e','bambu_claro':'#efd18a','bambu_oscuro':'#96a45e','soga':'#a67e57','madera':'#a46342','madera_oscura':'#603e36','lona':'#faf4de','hoja':'#2fa45f','hoja_luz':'#65bb6a','tronco':'#93664a','coco':'#805039','volcan':'#585562','volcan_claro':'#8b7774','lava':'#f47437','humo':'#a5a5af','coral':'#ed80a3','coral_naranja':'#f1a26c','coral_violeta':'#b684ca','pez_azul':'#63b8d7','pez_oscuro':'#286b9c','pez_verde':'#6ccb97','pez_dorado':'#efc94f','pez_oro':'#fff2a3','aleta':'#2f7ea5','ojo':'#293d4d','blanco':'#fff7e3','rojo':'#e75253','metal':'#d7e7e6','bota':'#76533e','bota_sombra':'#4f3c35','COLOR_DINAMICO':'#ffffff'})

def both(m,a,b,c,d,mat):quad(m,a,b,c,d,mat);quad(m,d,c,b,a,mat)
def sea():
 m=mesh()
 for i in range(64):
  a=TAU*i/64;b=TAU*(i+1)/64
  both(m,(15.0*math.cos(a),-.095,15.0*math.sin(a)),(85*math.cos(a),-.095,85*math.sin(a)),(85*math.cos(b),-.095,85*math.sin(b)),(15.0*math.cos(b),-.095,15.0*math.sin(b)),'oceano')
 for i in range(25):
  x=(i*29%110)-55;z=(i*37%112)-56
  if x*x+z*z<280:continue
  torus(m,(x,-.087,z),1.0+.15*(i%3),.025,'oceano_luz',14,4)
 return m

def island():
 m=mesh();n=48
 for i in range(n):
  a=TAU*i/n;b=TAU*(i+1)/n
  for r0,r1,y0,y1,mat in [(8.5,15,-.035,-.035,'arena'),(15,15.35,-.09,-.09,'arena_sombra')]:
   both(m,(r0*math.cos(a),y0,r0*math.sin(a)),(r1*math.cos(a),y1,r1*math.sin(a)),(r1*math.cos(b),y1,r1*math.sin(b)),(r0*math.cos(b),y0,r0*math.sin(b)),mat)
  both(m,(8.5*math.cos(a),-1.8,8.5*math.sin(a)),(8.5*math.cos(b),-1.8,8.5*math.sin(b)),(8.5*math.cos(b),0,8.5*math.sin(b)),(8.5*math.cos(a),0,8.5*math.sin(a)),'laguna')
  for r0,r1,mat in [(8.5,8.72,'arena_clara'),(14.1,14.3,'arena_clara')]:
   both(m,(r0*math.cos(a),.008,r0*math.sin(a)),(r1*math.cos(a),.008,r1*math.sin(a)),(r1*math.cos(b),.008,r1*math.sin(b)),(r0*math.cos(b),.008,r0*math.sin(b)),mat)
 cylinder(m,(0,-1.82,0),8.5,.035,'fondo',48)
 return m

def lagoon():
 m=mesh();cylinder(m,(0,-1.78,0),8.48,.035,'laguna',48)
 for r in (2.6,4.3,6.8):torus(m,(0,.01,0),r,.018,'agua_luz',48,4)
 return m

def dock():
 m=mesh();box(m,(0,.25,0),(3.4,.2,3.0),'bambu_oscuro')
 for k in range(6):
  x=(k-2.5)*.55
  box(m,(x,.375,0),(.5,.055,3),'bambu_claro' if k%2==0 else 'bambu')
  for z in (-1.25,1.25):cylinder(m,(x,.41,z),.065,.025,'bambu_oscuro',7)
 for x in (-1.48,1.48):
  for z in (-1.3,1.3):
   cylinder(m,(x,-1.8,z),.13,2.1,'bambu_oscuro',8,r_top=.11)
   torus(m,(x,.06,z),.12,.025,'soga',8,4)
 for z in (-1.5,1.5):beam(m,(-1.6,.19,z),(1.6,.19,z),.09,'bambu_oscuro',6)
 return m

def rod():
 m=mesh();beam(m,(0,0,0),(0,1.05,-.54),.065,'madera',8,r_end=.055)
 beam(m,(0,1.05,-.54),(0,1.6,-1.4),.054,'bambu_oscuro',8,r_end=.024)
 cylinder(m,(0,.23,0),.105,.21,'madera_oscura',9)
 torus(m,(0,1.32,-.91),.13,.023,'metal',9,4,plane='xy')
 ellipsoid(m,(0,.36,-.09),(.22,.22,.12),'metal',10,5)
 return m

def bobber():
 m=mesh();ellipsoid(m,(0,0,0),(.17,.17,.17),'blanco',12,6)
 cylinder(m,(0,.0,0),.11,.17,'rojo',10,r_top=.07)
 beam(m,(0,.13,0),(0,.32,0),.022,'metal',6)
 return m

def palm():
 m=mesh()
 for i in range(6):
  a=(0,4.2*i/6,0);b=(.12*math.sin((i+1)*.6),4.2*(i+1)/6,0)
  beam(m,a,b,.28-.014*i,'tronco',8,r_end=.27-.014*i)
  if i<5:torus(m,(b[0],b[1],0),.225-.012*i,.035,'madera',10,4)
 top=(.12*math.sin(3.6),4.2,0)
 for h in range(7):
  a=TAU*h/7;d=(math.cos(a),math.sin(a))
  # Hoja acintada, seis segmentos y caída en el extremo.
  for j in range(6):
   t0=j/6;t1=(j+1)/6
   def v(t,side):return (top[0]+d[0]*2.2*t-side*d[1]*.31*math.sin(math.pi*t),top[1]+.45*math.sin(math.pi*t)-.75*t*t,top[2]+d[1]*2.2*t+side*d[0]*.31*math.sin(math.pi*t))
   both(m,v(t0,-1),v(t0,1),v(t1,1),v(t1,-1),'hoja' if h%2 else 'hoja_luz')
 for x,z in [(-.22,.02),(.1,-.24),(.27,.12)]:ellipsoid(m,(top[0]+x,3.9,top[2]+z),(.17,.21,.17),'coco',9,5)
 return m

def boat():
 m=mesh()
 # Proa apuntando a -X, casco escalonado hueco visualmente.
 box(m,(0,.16,0),(3.2,.54,1.25),'madera')
 box(m,(0,.49,0),(2.6,.06,.9),'madera_oscura')
 for z in (-.63,.63):beam(m,(-1.6,.5,z),(1.25,.5,z),.12,'madera_oscura',7)
 cylinder(m,(-.15,.48,0),.07,2.2,'madera_oscura',7)
 both(m,(-.09,.65,.03),(.04,.65,.03),(.04,2.35,.03),(-.09,2.35,.03),'lona')
 both(m,(0,.7,.04),(1.12,.7,.04),(1.12,2.3,.04),(0,2.3,.04),'lona')
 beam(m,(1.34,.38,-.15),(2.5,.58,-.5),.045,'madera',6)
 return m

def gull():
 m=mesh();ellipsoid(m,(0,0,0),(.2,.14,.14),'blanco',9,5)
 ellipsoid(m,(0,.12,-.1),(.10,.11,.10),'blanco',8,4)
 for side in (-1,1):
  beam(m,(0,.06,0),(side*.55,.23,0),.075,'blanco',7,r_end=.035)
  beam(m,(side*.55,.23,0),(side*.9,.06,0),.045,'blanco',7,r_end=.012)
 ellipsoid(m,(0,.1,-.23),(.05,.035,.09),'rojo',7,4)
 return m

def volcano():
 m=mesh();cylinder(m,(0,0,0),4.2,5.6,'volcan',12,r_top=1.45)
 cylinder(m,(0,5.6,0),1.45,.85,'volcan_claro',12,r_top=1.25)
 cylinder(m,(0,6.43,0),1.15,.08,'lava',12)
 for j in range(6):
  a=TAU*j/6;beam(m,(1.15*math.cos(a),6.48,1.15*math.sin(a)),(1.55*math.cos(a),5.9,1.55*math.sin(a)),.09,'lava',5)
 return m

def smoke():
 m=mesh()
 for i,(x,y,r) in enumerate([(0,0,.58),(.28,.65,.78),(-.16,1.6,.98),(.19,2.55,1.2)]):
  ellipsoid(m,(x,y,0),(r,r*.65,r),'humo',10,5)
 return m

def coral(variant):
 m=mesh();c='coral_naranja' if variant else 'coral';c2='coral_violeta'
 cylinder(m,(0,0,0),.34,.22,'arena_sombra',9,r_top=.27)
 for j in range(5):
  a=TAU*j/5;d=(math.cos(a),math.sin(a));h=.65+.13*(j%3)
  beam(m,(d[0]*.13,.15,d[1]*.13),(d[0]*.37,h,d[1]*.37),.12,c if j%2 else c2,7,r_end=.065)
  ellipsoid(m,(d[0]*.37,h,d[1]*.37),(.20,.18,.20),c if j%2 else c2,8,4)
  beam(m,(d[0]*.30,h*.65,d[1]*.30),(d[0]*.65,h*.82,d[1]*.65),.07,c,6)
 return m

def fish(size,color,fin):
 m=mesh();r=size
 # Nariz apunta a +X; pivote en el centro para giro con dirX,dirZ.
 ellipsoid(m,(0,0,0),(r*1.15,r*.55,r*.53),color,14,7)
 for z in (-1,1):
  both(m,(-r*.86,0,z*r*.07),(-r*1.62,r*.52,z*r*.13),(-r*1.62,-r*.52,z*r*.13),(-r*.86,0,z*r*.07),fin)
 both(m,(-r*.25,r*.35,0),(-r*.62,r*.92,0),(r*.2,r*.55,0),(r*.4,r*.27,0),fin)
 for z in (-1,1):
  ellipsoid(m,(r*.62,r*.13,z*r*.46),(r*.09,r*.09,r*.05),'blanco',8,4)
  ellipsoid(m,(r*.64,r*.13,z*r*.51),(r*.05,r*.05,r*.028),'ojo',8,4)
 for x in (-.35,0,.35):
  for z in (-1,1):ellipsoid(m,(r*x,-r*.08,z*r*.51),(r*.06,r*.09,r*.027),fin,6,4)
 return m

def boot():
 m=mesh()
 box(m,(-.08,.08,0),(.33,.18,.52),'bota_sombra')
 box(m,(.09,.14,.19),(.61,.21,.27),'bota')
 box(m,(-.14,.36,-.09),(.31,.58,.35),'bota')
 box(m,(-.14,.67,-.09),(.38,.09,.4),'bota_sombra')
 cylinder(m,(-.14,.70,-.09),.115,.025,'arena_sombra',9)
 return m

def cursor():
 m=mesh();torus(m,(0,.02,0),.48,.03,'COLOR_DINAMICO',24,4)
 for a in (0,math.pi/2):
  beam(m,(-.68*math.cos(a),.025,-.68*math.sin(a)),(.68*math.cos(a),.025,.68*math.sin(a)),.018,'blanco',6)
 return m

def main():
 specs=[('oceano',sea(),'origen global','Plano marino 120x120; centro (0,0,0).'),('isla_laguna',island(),'origen global','Arena exterior radio 15; laguna radio 8.5 y fondo Y=-1.8.'),('superficie_laguna',lagoon(),'origen global','Fondo turquesa a Y=-1.78 y ondas a Y=.01; peces visibles a Y=-.9.'),('muelle_bambu',dock(),'centro en agua','Instanciar cuatro veces en X/Z=±10.5; tablero Y=.35.'),('cana_pescar',rod(),'mano','Mango en mano Y=1.45 del muelle, punta elevada hacia centro.'),('corcho',bobber(),'centro','En anzuelo (x,y,z); tintar tapa por jugador si hace falta.'),('palmera',palm(),'pie central','8 unidades en anillo radio 13.2, ángulos 22.5+45*k.'),('barca',boat(),'centro del casco','Posición (-12,0,-11).'),('gaviota',gull(),'centro','Cuatro en vuelo alrededor de radio 11.'),('volcan',volcano(),'pie central','Posición (0,0,-21).'),('humo_volcan',smoke(),'base de columna','Añadir a (0,7,-21), variar escala/altura visual.')]
 for i in range(2):specs.append((f'coral_{i+1}',coral(i),'pie central','Distribuir 16 en laguna; base Y=-1.8, radio 2..7.8.'))
 specs += [('pez_pequeno',fish(.28,'pez_azul','aleta'),'centro','Tipo 0, valor 1; centro Y=-.9.'),('pez_mediano',fish(.42,'pez_verde','pez_oscuro'),'centro','Tipo 1, valor 2; centro Y=-.9.'),('pez_dorado',fish(.5,'pez_dorado','pez_oro'),'centro','Tipo raro 2, valor 5; centro Y=-.9.'),('bota_vieja',boot(),'centro','Tipo 3, valor 0; centro Y=-.9.'),('cursor_lanzamiento',cursor(),'centro','En cursor (x,.09,z), tintar por jugador.')]
 for name,m,pivot,usage in specs:save(name,m,pivot,usage)
 (ROOT/'manifest.json').write_text(json.dumps({'minijuego':'Pesca Isleña','base':'claude/expansion-party ea5472c','radio_laguna':8.5,'distancia_muelle':10.5,'modelos':MANIFEST},ensure_ascii=False,indent=2))
 print(len(specs),'GLB;',sum(v['triangulos'] for v in MANIFEST),'triángulos')
if __name__=='__main__':main()
