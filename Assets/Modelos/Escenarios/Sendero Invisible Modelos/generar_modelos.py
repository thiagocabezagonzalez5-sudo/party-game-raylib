"""Modelos GLB modulares para Sendero Invisible. Solo Python estándar."""
from geometria_glb import *
import geometria_glb as glb

glb.PALETTE.update({
 'noche':'#192131','terreno':'#29313a','terreno_luz':'#39424d',
 'losa':'#515a69','losa_luz':'#687483','losa_sombra':'#353d4c',
 'salida':'#4a667c','meta':'#60845c','grieta':'#c477e5','grieta_sombra':'#3e3053',
 'piedra':'#6e7582','piedra_luz':'#929ba4','piedra_sombra':'#4a5260',
 'hierro':'#232936','hierro_luz':'#566474','tronco':'#3d333a','tronco_luz':'#59454a',
 'verde':'#6ef0a6','verde_luz':'#b4ffd2','verde_sombra':'#3b9e7d',
 'niebla':'#708398','niebla_oscura':'#39465b','luna':'#ebecdc','luna_sombra':'#c4c9c0',
 'blanco':'#e6edf5','negro':'#161923','COLOR_DINAMICO':'#ffffff','BOMBILLAS':'#d2ffe2'})

def tile(kind):
 m=mesh();base={'normal':'losa','crack':'losa','start':'salida','finish':'meta'}[kind]
 box(m,(0,-.2,0),(1.36,.4,1.36),base)
 box(m,(0,-.004,0),(1.25,.012,1.25),'losa_luz' if kind=='normal' else base)
 for x,z in [(-.5,-.5),(.5,-.5),(-.5,.5),(.5,.5)]:
  ellipsoid(m,(x,.014,z),(.055,.02,.055),'losa_sombra',7,4)
 if kind=='crack':
  pts=[(-.58,-.25),(-.10,.12),(.17,-.25),(.54,.18)]
  for i in range(3):
   a,b=pts[i],pts[i+1]
   beam(m,(a[0],.023,a[1]),(b[0],.023,b[1]),.027,'grieta_sombra',5)
   beam(m,(a[0],.036,a[1]),(b[0],.036,b[1]),.013,'grieta',5)
  beam(m,(-.10,.033,.12),(-.24,.033,.55),.014,'grieta',5)
 if kind=='start':
  box(m,(0,.023,-.49),(1.02,.018,.07),'COLOR_DINAMICO')
  box(m,(0,.023,.49),(1.02,.018,.07),'COLOR_DINAMICO')
 if kind=='finish':
  for i in range(4):
   x=-.45+i*.30
   box(m,(x,.022,-.45),(.15,.018,.10),'verde_luz')
 return m

def fragment():
 m=mesh();box(m,(0,0,0),(.55,.35,.55),'losa')
 box(m,(0,.18,0),(.5,.014,.5),'losa_luz')
 return m

def glow():
 m=mesh();box(m,(0,.016,0),(1.23,.015,1.23),'verde_sombra')
 torus(m,(0,.038,0),.47,.029,'verde',24,4)
 return m

def cemetery():
 m=mesh();box(m,(0,-1,0),(120,1,36),'terreno')
 for i in range(90):
  x=-59+(i*83%1180)/10;z=-17+(i*47%340)/10
  box(m,(x,-.49,z),(.38,.018,.23),'terreno_luz' if i%3 else 'noche')
 return m

def ledge():
 m=mesh();box(m,(0,-1,0),(120,1,.8),'terreno_luz')
 box(m,(0,-.49,.12),(120,.07,.10),'piedra_sombra')
 return m

def headstone(cross=False):
 m=mesh();box(m,(0,.52,0),(.8,1.04,.25),'piedra')
 ellipsoid(m,(0,1.04,0),(.4,.24,.13),'piedra_luz',10,5)
 box(m,(0,.13,.16),(1.03,.26,.52),'piedra_sombra')
 if cross:
  box(m,(0,1.38,0),(.15,.73,.15),'piedra_luz')
  box(m,(0,1.54,0),(.52,.13,.15),'piedra_luz')
 else:
  box(m,(0,.64,.137),(.29,.07,.016),'piedra_sombra')
  box(m,(0,.45,.137),(.36,.035,.016),'piedra_sombra')
 return m

def mausoleum():
 m=mesh();box(m,(0,1.6,0),(4,3.2,3),'piedra')
 box(m,(0,3.22,0),(4.3,.3,3.3),'piedra_sombra')
 # Pirámide baja de tejado.
 cylinder(m,(0,3.35,0),2.15,1.35,'hierro_luz',4,r_top=0)
 box(m,(0,1.12,1.52),(1.2,2.0,.1),'negro')
 for x in (-1.5,1.5):
  box(m,(x,1.4,1.56),(.25,2.4,.12),'piedra_luz')
  ellipsoid(m,(x,2.7,1.57),(.15,.15,.08),'verde_sombra',8,4)
 for z in (-1.25,1.25):
  for x in (-1.8,1.8):box(m,(x,2.2,z),(.12,1.1,.12),'piedra_sombra')
 return m

def tree():
 m=mesh();cylinder(m,(0,0,0),.44,5.1,'tronco',7,r_top=.23)
 for a,b,r in [((0,3.8,0),(-1.8,6.4,0),.17),((0,4.3,0),(1.6,6.9,.4),.15),((0,2.8,0),(1.3,4.3,-.55),.13)]:
  beam(m,a,b,r,'tronco',7,r_end=.07)
  x,y,z=b;beam(m,b,(x-.28,y+.55,z+.08),.08,'tronco_luz',6,r_end=.02)
 for a in (-.5,.5):ellipsoid(m,(a,.21,.2),(.6,.18,.4),'tronco',9,5)
 return m

def fence():
 m=mesh()
 for i in range(8):
  x=-5.6+i*1.6
  beam(m,(x,0,0),(x,1.8,0),.055,'hierro',5)
  cylinder(m,(x,1.8,0),.11,.30,'hierro_luz',5,r_top=0)
 for y in (.4,1.0):box(m,(0,y,0),(11.4,.08,.08),'hierro')
 for x in (-5.6,5.6):box(m,(x,.75,0),(.18,1.5,.18),'hierro_luz')
 return m

def lantern():
 m=mesh();cylinder(m,(0,0,0),.10,2.6,'hierro',6,r_top=.065)
 box(m,(0,2.31,0),(.43,.43,.43),'hierro_luz')
 for x in (-.2,.2):
  for z in (-.2,.2):box(m,(x,2.31,z),(.06,.44,.06),'hierro')
 cylinder(m,(0,2.52,0),.32,.18,'hierro',8,r_top=.08)
 ellipsoid(m,(0,2.33,0),(.16,.22,.16),'BOMBILLAS',11,6)
 ellipsoid(m,(0,2.55,0),(.09,.14,.09),'verde',9,5)
 return m

def hand_lantern():
 m=mesh();box(m,(0,0,0),(.34,.40,.34),'hierro_luz')
 for x in (-.16,.16):
  for z in (-.16,.16):box(m,(x,0,z),(.04,.40,.04),'hierro')
 ellipsoid(m,(0,0,0),(.14,.18,.14),'BOMBILLAS',10,5)
 beam(m,(0,.2,0),(0,.47,0),.035,'hierro',5)
 torus(m,(0,.49,0),.14,.025,'hierro',12,4,plane='xy')
 return m

def wisp():
 m=mesh();ellipsoid(m,(0,0,0),(.23,.27,.23),'BOMBILLAS',12,7)
 ellipsoid(m,(0,-.22,.18),(.12,.14,.11),'verde_sombra',9,5)
 for i in range(4):
  a=TAU*i/4
  ellipsoid(m,(.36*math.cos(a),-.12,.36*math.sin(a)),(.055,.06,.055),'verde',7,4)
 return m

def goal():
 m=mesh()
 for x in (-1.6,1.6):
  box(m,(x,1.4,0),(.18,2.8,.18),'hierro')
  ellipsoid(m,(x,2.96,0),(.16,.16,.16),'BOMBILLAS',10,5)
 box(m,(0,2.8,0),(3.4,.14,.14),'hierro_luz')
 torus(m,(0,2.77,.07),.46,.035,'verde',20,4,plane='xy')
 return m

def moon():
 m=mesh();ellipsoid(m,(0,0,0),(17,17,17),'luna',22,11)
 for x,y,z,r in [(-4,6,15,2),(-8,-2,14,3),(6,-5,15,2.3)]:
  ellipsoid(m,(x,y,z),(r,r*.7,.14),'luna_sombra',10,5)
 return m

def fog():
 m=mesh();box(m,(0,-8,-1),(90,.05,40),'noche')
 for i in range(32):
  x=-40+(i*31%81);z=-13+(i*23%28)
  ellipsoid(m,(x,-3.5-(i%3)*.45,z),(2.5+(i%4)*.35,.06,.7+(i%3)*.25),'niebla' if i%3 else 'niebla_oscura',9,4)
 return m

def main():
 specs=[
 ('losa_normal',tile('normal'),'centro de casilla','Cuadrícula 6×10, paso 1.5; centro Y=0, tamaño 1.36×.4×1.36.'),
 ('losa_agrietada',tile('crack'),'centro de casilla','Solo al marcar m.grieta[indice]; no anticipar ruta segura.'),
 ('losa_salida',tile('start'),'centro de casilla','Fila 0, tinte COLOR_DINAMICO del jugador.'),
 ('losa_meta',tile('finish'),'centro de casilla','Fila 9; extremo Z=-7.5.'),
 ('fragmento_losa',fragment(),'centro de fragmento','Cuatro trozos temporales durante caída, seguir física visual existente.'),
 ('brillo_losa',glow(),'centro de casilla','Solo al iluminar ruta durante memorización o farol.'),
 ('terreno_cementerio',cemetery(),'centro de terraza','Centro (0,0,-28), suelo de fondo.'),
 ('borde_cementerio',ledge(),'centro','Centro (0,0,-9.6).'),
 ('lapida',headstone(),'pie central','Varias detrás de verja; escalas/giros discretos.'),
 ('lapida_cruz',headstone(True),'pie central','Variante de tumba con cruz.'),
 ('mausoleo',mausoleum(),'pie central','Centros aprox (-14,-24),(12,-27),(30,-22); escala 1..1.3.'),
 ('arbol_seco',tree(),'pie central','Al fondo Z≈-19..-26.'),
 ('verja_tramo',fence(),'centro al pie','Cinco segmentos en Z=-9.4, centros X=-22.4,-11.2,0,11.2,22.4.'),
 ('farol_verde',lantern(),'pie central','Siete faroles en X=-27+9*k,Z=-9.'),
 ('farol_recogible',hand_lantern(),'centro','En losa del índice m.farolIndice, altura≈.55.'),
 ('fuego_fatuo',wisp(),'centro','Durante memorización, recorrer m.secuencia a Y≈.9.'),
 ('arco_meta',goal(),'centro al pie','Uno por carril sobre la fila 9, centro Z=-7.9.'),
 ('luna',moon(),'centro geométrico','Colocar relativa a cámara en dirección superior izquierda, a distancia≈150.'),
 ('niebla_abismo',fog(),'centro','Bajo la cuadrícula, niveles Y=-2.6,-5,-8; ajustar alpha en motor si procede.'),
 ]
 for name,m,pivot,usage in specs:save(name,m,pivot,usage)
 (ROOT/'manifest.json').write_text(json.dumps({'minijuego':'Sendero Invisible','base':'claude/expansion-party 22af9d0','coordenadas':'Y arriba; paso de losas 1.5; unidades raylib','modelos':MANIFEST},ensure_ascii=False,indent=2))
 print(len(specs),'GLB;',sum(v['triangulos'] for v in MANIFEST),'triángulos')

if __name__=='__main__':main()
