"""GLB procedurales originales para Rodillos Neón; Python 3 estándar."""
from geometria_glb import *
import geometria_glb as glb

glb.PALETTE.update({'suelo':'#110f26','cuadricula_azul':'#2b5dcb','cuadricula_rosa':'#9b4aba','pared':'#17132f','pared_panel':'#242041','gabinete':'#28213f','gabinete_claro':'#3e345c','gabinete_borde':'#101327','metal':'#677290','metal_claro':'#9eaac2','tambor':'#1b182d','tambor_cara':'#242139','tambor_raya':'#554775','cian':'#12dcec','cian_luz':'#b7fbff','rosa':'#f34ac3','rosa_luz':'#ffc0ed','verde':'#80f25a','verde_luz':'#cdffa5','naranja':'#ffa652','amarillo':'#ffe478','amarillo_luz':'#fff9b2','negro':'#11101f','blanco':'#e9f6f7','COLOR_DINAMICO':'#ffffff'})

def both(m,a,b,c,d,mat):quad(m,a,b,c,d,mat);quad(m,d,c,b,a,mat)
def outlined_box(m,c,size,mat,trim):
 box(m,c,size,mat);x,y,z=c;w,h,d=size
 for sx in (-1,1):
  for sy in (-1,1):beam(m,(x-sx*w/2,y+sy*h/2,z+d/2+.012),(x+sx*w/2,y+sy*h/2,z+d/2+.012),.022,trim,6)

def floor():
 m=mesh();box(m,(0,-.2,0),(60,.4,40),'suelo')
 for i in range(-15,16):box(m,(2*i,.021,-2),(0.04,.04,28),'cuadricula_azul' if i%2 else 'cuadricula_rosa')
 for i in range(-8,7):box(m,(0,.022,i*2),(60,.04,.04),'cuadricula_azul')
 for x in (-22,-11,0,11,22):
  torus(m,(x,.03,4.3),1.4,.025,'rosa',20,4)
 return m

def backdrop():
 m=mesh();box(m,(0,7,-9),(60,16,.4),'pared')
 for x in range(-28,29,2):box(m,(x,7,-8.75),(.04,14.8,.04),'pared_panel')
 for y in range(2,15,2):box(m,(0,y,-8.74),(59,.035,.04),'pared_panel')
 box(m,(0,14.2,-8.7),(59,.11,.1),'rosa')
 return m

def cabinet():
 m=mesh()
 box(m,(0,.75,.3),(4.4,1.5,2.8),'gabinete')
 box(m,(-1.95,3,.2),(.5,3.2,3.4),'gabinete_claro')
 box(m,(1.95,3,.2),(.5,3.2,3.4),'gabinete_claro')
 box(m,(0,4.95,.2),(4.4,1.2,3.4),'gabinete')
 box(m,(0,1.55,1.95),(3.4,.4,.3),'gabinete_borde')
 box(m,(0,4.5,1.95),(3.4,.55,.3),'gabinete_borde')
 box(m,(0,1.18,1.59),(2.75,.22,.9),'gabinete_claro')
 for x in (-1.7,1.7):
  box(m,(x,3,1.94),(.08,2.8,.09),'metal')
  box(m,(x,5.46,1.9),(.08,.10,.12),'metal')
 for x,c in [(-1.2,'rosa'),(0,'cian'),(1.2,'amarillo')]:
  torus(m,(x,5.06,1.98),.29,.036,c,18,4,plane='xy')
  ellipsoid(m,(x,5.06,2),(.08,.08,.04),c,9,5)
 for x in (-1.6,1.6):
  for y in (.45,.95):box(m,(x,y,1.84),(.14,.07,.08),'metal')
 return m

def frame():
 m=mesh()
 for x in (-2.2,2.2):box(m,(x,2.9,1.96),(.07,4.4,.075),'COLOR_DINAMICO')
 box(m,(0,5.55,1.96),(4.4,.07,.075),'COLOR_DINAMICO')
 box(m,(0,.04,1.75),(4.4,.07,.075),'COLOR_DINAMICO')
 for x in (-2.2,2.2):ellipsoid(m,(x,5.55,1.96),(.09,.09,.09),'blanco',9,5)
 return m

def button():
 m=mesh();box(m,(0,0,0),(1.5,.24,.8),'gabinete_claro')
 cylinder(m,(0,.13,0),.34,.18,'metal',18,r_top=.3)
 ellipsoid(m,(0,.32,0),(.28,.12,.28),'COLOR_DINAMICO',18,6)
 torus(m,(0,.15,0),.34,.03,'blanco',20,5)
 return m

def drum():
 m=mesh()
 # Eje horizontal X, radio 1.7, ancho .8, origen en centro.
 n=30;w=.8;r=1.7
 for i in range(n):
  a=TAU*i/n;b=TAU*(i+1)/n
  v0=(-w/2,r*math.sin(a),r*math.cos(a));v1=(w/2,r*math.sin(a),r*math.cos(a))
  v2=(w/2,r*math.sin(b),r*math.cos(b));v3=(-w/2,r*math.sin(b),r*math.cos(b))
  both(m,v0,v1,v2,v3,'tambor' if i%3 else 'tambor_cara')
 for x in (-.42,.42):
  torus(m,(x,0,0),r,.055,'metal',36,5,plane='yz')
 for k in range(10):
  a=k*TAU/10
  for x in (-.38,.38):ellipsoid(m,(x,1.72*math.sin(a),1.72*math.cos(a)),(.045,.045,.045),'tambor_raya',7,4)
 beam(m,(-.6,0,0),(.6,0,0),.13,'metal',12)
 return m

def glyph(i):
 m=mesh();colors=['cian','rosa','verde','naranja','amarillo'];lights=['cian_luz','rosa_luz','verde_luz','amarillo_luz','amarillo_luz']
 sides=[3,16,4,4,10][i];angle=math.pi/2 if i in (0,4) else math.pi/4 if i==2 else 0
 pts=[]
 for k in range(sides):
  t=angle+k*TAU/sides;r=.43 if i==4 and k%2==0 else .20 if i==4 else .42
  pts.append((r*math.cos(t),r*math.sin(t)))
 for k,(x,y) in enumerate(pts):
  xx,yy=pts[(k+1)%sides]
  both(m,(0,0,.025),(x,y,.025),(xx,yy,.025),(0,0,.025),colors[i])
  beam(m,(x,y,.04),(xx,yy,.04),.028,lights[i],6)
 if i==1:torus(m,(0,0,.042),.24,.025,'blanco',22,5,plane='xy')
 if i==4:ellipsoid(m,(0,0,.04),(.08,.08,.045),'amarillo_luz',8,4)
 return m

def payline():
 m=mesh();box(m,(0,0,0),(3.5,.035,.035),'blanco')
 for x,side in [(-1.9,1),(1.9,-1)]:
  both(m,(x,-.18,.04),(x,.18,.04),(x+side*.25,0,.04),(x,-.18,.04),'amarillo')
 box(m,(0,.23,-.015),(3.2,.025,.025),'amarillo')
 box(m,(0,-.23,-.015),(3.2,.025,.025),'amarillo')
 return m

def led(pink=False):
 m=mesh();box(m,(0,6.5,0),(.5,13,.4),'pared_panel')
 for k in range(10):
  color=('rosa' if k%3 else 'rosa_luz') if pink else ('cian' if k%3 else 'cian_luz')
  box(m,(0,1.2+1.25*k,.27),(.3,.52,.11),color)
 for y in (.2,12.85):box(m,(0,y,.03),(.6,.12,.48),'metal')
 return m

def hologram(i):
 m=mesh();c=['cian','rosa','amarillo'][i]
 if i==0:
  for x in (-.9,.9):
   for y in (-.9,.9):beam(m,(x,y,-.9),(x,y,.9),.025,c,5)
  for z in (-.9,.9):
   for x in (-.9,.9):beam(m,(x,-.9,z),(x,.9,z),.025,c,5)
   for y in (-.9,.9):beam(m,(-.9,y,z),(.9,y,z),.025,c,5)
 elif i==1:
  for y in (-.65,-.25,.25,.65):
   r=math.sqrt(1.1**2-y*y);torus(m,(0,y,0),r,.026,c,24,5)
  for a in range(8):
   t=TAU*a/8;beam(m,(0,1.1,0),(1.03*math.cos(t),.35,1.03*math.sin(t)),.018,c,5)
   beam(m,(0,-1.1,0),(1.03*math.cos(t),-.35,1.03*math.sin(t)),.018,c,5)
 else:
  for k in range(4):
   a=TAU*k/4;b=TAU*(k+1)/4
   beam(m,(0,.95,0),(.95*math.cos(a),-.85,.95*math.sin(a)),.025,c,5)
   beam(m,(.95*math.cos(a),-.85,.95*math.sin(a)),(.95*math.cos(b),-.85,.95*math.sin(b)),.025,c,5)
 return m

def sign(i):
 m=mesh();colors=['cian','rosa','verde','naranja'];c=colors[i]
 box(m,(0,0,0),(4.9,2,.20),'gabinete_borde')
 for y in (-.86,.86):box(m,(0,y,.13),(4.7,.09,.1),c)
 for x in (-2.3,2.3):box(m,(x,0,.13),(.09,1.8,.1),c)
 g=transform(glyph(i),p=(0,0,.18));merge(m,g)
 for x in (-1.6,1.6):torus(m,(x,0,.16),.24,.025,'amarillo',16,4,plane='xy')
 return m

def main():
 specs=[('suelo_arcade',floor(),'origen global','Rejilla 60x40 en (0,0,0).'),('pared_arcade',backdrop(),'origen global','Pared trasera en Z=-9, Y hasta 15.'),('gabinete_arcade',cabinet(),'pie central','Instanciar en X=(i-(N-1)/2)*5.8, Z=0.'),('marco_jugador',frame(),'pie central','Mismo origen que gabinete; teñir por jugador.'),('boton_detener',button(),'centro de base','Gabinete (0,1.2,1.9); teñir cúpula o todo al color de jugador.'),('tambor_rodillo',drum(),'centro del eje','Gabinete X=-1.05,0,1.05; Y=3, Z=0. Girar sobre X según rodillo.angulo.')]
 for i,name in enumerate(['triangulo','circulo','cuadrado','rombo','estrella']):specs.append(('simbolo_'+name,glyph(i),'centro cara XY','Tipo %d; adjuntar a cada k del tambor en radio 1.73 y rotar sobre X.'%i))
 specs.append(('linea_comodin',payline(),'centro','Poner en gabinete (0,3,1.84); banda visual sin lógica.'))
 specs.extend([('columna_led_cian',led(False),'pie central','Pared X=-24+6*c, Z=-8.5.'),('columna_led_rosa',led(True),'pie central','Alternar en pared X=-24+6*c, Z=-8.5.')])
 for i,name in enumerate(['cubo','esfera','piramide']):specs.append(('holograma_'+name,hologram(i),'centro','Sobre pared, X=-12+12*k, Y=9.5, Z=-6; animar giro.'))
 for i,name in enumerate(['cian','rosa','verde','naranja']):specs.append(('letrero_'+name,sign(i),'centro','Mundo X=-18+12*k, Y=12.3, Z=-8.4.'))
 for name,m,pivot,usage in specs:save(name,m,pivot,usage)
 (ROOT/'manifest.json').write_text(json.dumps({'minijuego':'Rodillos Neón','base':'claude/expansion-party ea5472c','radio_rodillo':1.7,'altura_rodillos':3.0,'separacion_maquinas':5.8,'modelos':MANIFEST},ensure_ascii=False,indent=2))
 print(len(specs),'GLB;',sum(v['triangulos'] for v in MANIFEST),'triángulos')
if __name__=='__main__':main()
