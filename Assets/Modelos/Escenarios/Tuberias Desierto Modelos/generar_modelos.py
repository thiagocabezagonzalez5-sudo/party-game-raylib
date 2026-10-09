"""GLB modulares del oasis y sus acueductos, generados con Python estándar."""
from geometria_glb import *
import geometria_glb as glb

glb.PALETTE.update({
 'arena':'#e2be80','arena_clara':'#f4d396','arena_sombra':'#b68a56',
 'piedra':'#d6b276','piedra_luz':'#f2d49e','piedra_sombra':'#aa7d4f',
 'barro':'#c47f52','barro_sombra':'#905936','ceramica':'#dc9862',
 'agua':'#3caae8','agua_luz':'#9ce8ee','agua_sombra':'#267db4',
 'oro':'#ffd25a','oro_sombra':'#bd812f','miraje':'#eee0a3',
 'madera':'#795337','hierro':'#56566a','verde':'#3f9a53',
 'verde_luz':'#71b85f','verde_sombra':'#317648','palma':'#96683e',
 'sol':'#ffe598','sol_sombra':'#ffbf6b','COLOR_JUGADOR':'#ffffff'
})

def suelo():
 m=mesh();box(m,(0,-.6,-30),(220,1,160),'arena')
 for k in range(56):
  x=-52+(k*43%105);z=-65+(k*59%112)
  ellipsoid(m,(x,-.08,z),(1+(k%4)*.55,.07,.35+(k%3)*.22),
            'arena_clara' if k%3 else 'arena_sombra',8,4)
 return m

def pared():
 m=mesh();box(m,(0,7.5,0),(30,15,1.2),'piedra')
 box(m,(0,15.2,0),(31,.6,1.8),'piedra_sombra')
 box(m,(0,13.9,.65),(28,.2,.12),'piedra_sombra')
 box(m,(0,.4,.65),(28,.2,.12),'piedra_sombra')
 for lado in (-1,1):box(m,(lado*14.2,7.5,.7),(1,14,.4),'piedra_sombra')
 # Relieves esculpidos poco profundos; dejan la pared libre para las tuberías.
 for k in range(8):
  x=-13+k*3.7
  box(m,(x,14.57,.67),(.72,.5,.09),'arena_sombra')
  torus(m,(x,14.56,.74),.17,.026,'piedra_luz',10,4,plane='xy')
 for y in (2.0,6.1,10.1):
  for k in range(7):
   x=-12+(k+.5*(int(y)%2))*3.9
   box(m,(x,y,-.61),(3.7,.07,.04),'arena_sombra')
 return m

def cisterna():
 m=mesh()
 cylinder(m,(0,0,0),4.2,1.25,'piedra_sombra',24,r_top=4.35)
 cylinder(m,(0,1.25,0),4.35,.25,'piedra_luz',24)
 cylinder(m,(0,1.51,0),3.6,.04,'agua',24)
 for k in range(20):
  a=k*TAU/20
  ellipsoid(m,(4.22*math.cos(a),.73,4.22*math.sin(a)),(.13,.18,.13),
            'piedra' if k%2 else 'arena_sombra',7,4)
 return m

def agua_cisterna():
 m=mesh();cylinder(m,(0,0,0),3.1,.025,'agua_luz',24)
 for r in (.9,1.75,2.55):torus(m,(0,.03,0),r,.03,'agua_sombra',32,4)
 return m

def colector():
 m=mesh()
 beam(m,(-8.3,0,0),(8.3,0,0),.34,'piedra_sombra',12)
 beam(m,(0,1.3,-.7),(0,0,0),.41,'piedra',10)
 for x in (-8.2,-5.45,-2.72,0,2.72,5.45,8.2):
  torus(m,(x,0,0),.35,.055,'piedra_luz',16,5,plane='yz')
 return m

def tubo_piedra():
 # 1 unidad a lo largo de Y; en el motor, rotar desde +Y hasta B-A y escalar Y.
 # Canal frontal abierto para que se vea el agua durante el flujo.
 m=mesh()
 for k in range(24):
  a=k*TAU/24;b=(k+1)*TAU/24
  if math.sin((a+b)/2)>.69:continue
  def p(r,y,t):return (r*math.cos(t),y,r*math.sin(t))
  quad(m,p(.36,0,a),p(.36,1,a),p(.36,1,b),p(.36,0,b),'barro')
  quad(m,p(.29,0,b),p(.29,1,b),p(.29,1,a),p(.29,0,a),'barro_sombra')
  for y in (0,1):quad(m,p(.29,y,a),p(.36,y,a),p(.36,y,b),p(.29,y,b),'ceramica')
 for y in (.06,.94):cylinder(m,(0,y,0),.41,.075,'ceramica',12)
 for y in (.24,.76):torus(m,(0,y,0),.365,.02,'barro_sombra',14,4)
 return m

def tubo_agua():
 m=mesh();cylinder(m,(0,0,0),.24,1,'agua',12)
 for y in (.21,.54,.83):
  box(m,(0,y,.232),(.20,.045,.013),'agua_luz')
 return m

def codo_tuberia():
 m=mesh();ellipsoid(m,(0,0,0),(.37,.37,.37),'barro',14,7)
 torus(m,(0,0,0),.38,.06,'ceramica',16,5,plane='xy')
 return m

def columna():
 m=mesh();cylinder(m,(0,0,0),.8,6,'piedra',12,r_top=.75)
 cylinder(m,(0,0,0),1,.36,'piedra_sombra',12)
 cylinder(m,(0,5.9,0),1.05,.35,'piedra_luz',12)
 for y in (1.0,2.3,3.6,4.9):torus(m,(0,y,0),.8,.035,'arena_sombra',20,4)
 for x in (-.34,.34):box(m,(x,3,.69),(.07,5.3,.025),'piedra_luz')
 return m

def repisa():
 m=mesh();box(m,(0,.9,0),(28,1.8,2.4),'piedra_sombra')
 box(m,(0,1.78,.15),(28.5,.08,2.5),'piedra_luz')
 for x in (-12,-8,-4,0,4,8,12):
  box(m,(x,.8,1.22),(.06,1.55,.04),'arena_sombra')
 return m

def plataforma():
 m=mesh();box(m,(0,-.25,0),(26,.5,6.5),'arena')
 box(m,(0,-.24,3.3),(26,.5,.2),'piedra_sombra')
 for x in (-10,-5,0,5,10):box(m,(x,.01,0),(.04,.02,6),'arena_sombra')
 return m

def compuerta_base():
 m=mesh();cylinder(m,(0,-.3,0),.5,.4,'piedra_sombra',12)
 torus(m,(0,0,0),.49,.08,'piedra_luz',16,5)
 for x in (-.62,.62):box(m,(x,.15,.47),(.11,.8,.1),'hierro')
 return m

def compuerta_hoja():
 m=mesh();box(m,(0,0,0),(1,.5,.15),'madera')
 for x in (-.37,0,.37):box(m,(x,0,.08),(.06,.49,.035),'hierro')
 for y in (-.18,.18):box(m,(0,y,.09),(.94,.045,.03),'hierro')
 return m

def cantaro(kind):
 m=mesh();base={'barro':'barro','oro':'oro','miraje':'miraje'}[kind]
 ellipsoid(m,(0,.75,0),(.75,.75,.75),base,16,9)
 cylinder(m,(0,1.22,0),.48,.46,base,16,r_top=.34)
 cylinder(m,(0,1.66,0),.51,.14,'barro_sombra' if kind=='barro' else ('oro_sombra' if kind=='oro' else 'agua_luz'),16,r_top=.34)
 cylinder(m,(0,1.80,0),.31,.018,'agua_sombra' if kind=='barro' else ('oro' if kind=='oro' else 'agua_luz'),16)
 torus(m,(0,.75,0),.75,.045,'ceramica' if kind=='barro' else ('oro_sombra' if kind=='oro' else 'agua_luz'),20,5)
 for a in (-1,1):
  x=a*.57
  torus(m,(x,1.05,0),.29,.07,base,14,5,plane='xy')
 return m

def palmera():
 m=mesh();cylinder(m,(0,0,0),.36,4,'palma',9,r_top=.24)
 for y in (1,2,3):torus(m,(0,y,0),.33-y*.02,.035,'arena_sombra',12,4)
 ellipsoid(m,(0,4,0),(.52,.45,.52),'verde_sombra',10,6)
 for k in range(7):
  a=k*TAU/7;c=math.cos(a);s=math.sin(a)
  beam(m,(0,4.1,0),(.95*c,4.28,.95*s),.13,'verde',7,r_end=.10)
  beam(m,(.95*c,4.28,.95*s),(2.25*c,3.34,2.25*s),.19,'verde_luz',7,r_end=.02)
  for j in (1,2,3):
   d=.45+j*.38
   ellipsoid(m,(d*c,4.4-j*.25,d*s),(.18,.07,.38),'verde',7,4)
 for k in range(4):
  a=k*TAU/4
  ellipsoid(m,(.3*math.cos(a),3.87,.3*math.sin(a)),(.19,.23,.16),'palma',8,5)
 return m

def cactus():
 m=mesh();cylinder(m,(0,0,0),.35,2.65,'verde',10,r_top=.32)
 ellipsoid(m,(0,2.65,0),(.32,.28,.32),'verde_luz',10,5)
 for s,y in ((-1,1.05),(1,1.5)):
  beam(m,(s*.15,y,0),(s*.8,y,0),.15,'verde',8)
  cylinder(m,(s*.8,y,0),.18,.95,'verde',8)
  ellipsoid(m,(s*.8,y+.94,0),(.18,.2,.18),'verde_luz',8,5)
 for a in range(8):
  ang=a*TAU/8
  box(m,(math.cos(ang)*.34,1.3,math.sin(ang)*.34),(.025,.7,.025),'verde_luz')
 return m

def duna():
 m=mesh();ellipsoid(m,(0,-2.1,0),(12,2.8,6.4),'arena',20,8)
 ellipsoid(m,(3,-1.8,.3),(5,1.7,2.4),'arena_clara',16,7)
 return m

def sol():
 m=mesh();ellipsoid(m,(0,0,0),(11,11,11),'sol',22,12)
 torus(m,(0,0,0),12,.35,'sol_sombra',32,5,plane='xy')
 return m

def main():
 specs=[
  ('suelo_desierto',suelo(),'centro del suelo','Origen: suelo y=0; extiende la arena alrededor del muro.'),
  ('pared_arenisca',pared(),'pie centro','Centro (0,0,-3.2), pared 30×15; cara frontal z≈-2.6.'),
  ('cisterna_oasis',cisterna(),'base centro','Centro (0,15.2,-3.2); borde y≈16.7.'),
  ('agua_cisterna',agua_cisterna(),'centro superficie','Centro (0,16.74,-3.2), movimiento suave en Y.'),
  ('colector',colector(),'centro colector','Centro (0,13.7,-2.3); escala X para cubrir entradas 4..6.'),
  ('tubo_piedra',tubo_piedra(),'extremo inferior local','Tramo unitario +Y entre dos puntos de PuntoTuberia; escala longitudinal.'),
  ('tubo_agua',tubo_agua(),'extremo inferior local','Tubo interior +Y; mostrar solo donde progresoAgua lo permite.'),
  ('codo_tuberia',codo_tuberia(),'centro','Uniones de tramos al seguir una curva; no cambia carriles.'),
  ('columna_ruina',columna(),'pie centro','Varias en x≈±18 y ±21, delante o a los lados de la pared.'),
  ('repisa_cantaros',repisa(),'pie centro','Centro (0,0,-1.9); tapa y=1.8.'),
  ('plataforma_jugadores',plataforma(),'centro','Centro (0,0,3), jugador en z=2.8.'),
  ('compuerta_base',compuerta_base(),'centro del eje','Una por entrada en (XCarril,12.8,-2.3).'),
  ('compuerta_hoja',compuerta_hoja(),'centro hoja','Centro (XCarril,13.0+0.9*compuerta,-1.8).'),
  ('cantaro_barro',cantaro('barro'),'base centro','En cada salida salvo oro o miraje, (XCarril,1.8,-1.9).'),
  ('cantaro_oro',cantaro('oro'),'base centro','Salida oro; +3 puntos, identidad oculta hasta revelación según reglas.'),
  ('cantaro_miraje',cantaro('miraje'),'base centro','Salida espejismo; ocultar o sustituir durante disipación al revelar.'),
  ('palmera_oasis',palmera(),'pie centro','Tres sobre el borde superior, escala .8..1.1.'),
  ('cactus',cactus(),'pie centro','Decoración a ambos lados x≈±11.5,z≈4.5.'),
  ('duna',duna(),'centro bajo arena','Fondo detrás de la pared, centrar por ejemplo (-34,0,-14).'),
  ('sol',sol(),'centro','Fondo (-26,22,-60), diámetro 22.'),
 ]
 for name,m,pivot,usage in specs:save(name,m,pivot,usage)
 (ROOT/'manifest.json').write_text(json.dumps({'minijuego':'Tuberías Desierto','base':'claude/expansion-party e167c89','coordenadas':'Y arriba; unidades raylib; tubería Z=-2.3; entradas y=12.8; cántaros y=1.8','modelos':MANIFEST},ensure_ascii=False,indent=2)+'\n')
 print(len(specs),'GLB;',sum(v['triangulos'] for v in MANIFEST),'triángulos')

if __name__=='__main__':main()
