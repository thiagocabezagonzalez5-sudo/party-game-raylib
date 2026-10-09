"""Modelos originales de ruinas y estados dinámicos. Python 3 estándar."""
from geometria_glb import *
import geometria_glb as glb

glb.PALETTE.update({
 'piedra':'#858d7d','piedra_luz':'#abb5a4','piedra_sombra':'#59685a',
 'junta':'#47564b','musgo':'#4b9257','musgo_oscuro':'#2f7149',
 'suelo':'#67725f','tierra':'#2a583c','arbol':'#594738',
 'hoja':'#367b48','hoja_luz':'#5aa15f','runa':'#89e8d8',
 'luz':'#bdfcf2','agua':'#44bad1','orbe_azul':'#b9f8ff',
 'orbe_naranja':'#ffb469','halo_azul':'#63c3e1','halo_naranja':'#e97549',
 'escudo':'#61c6e4','escudo_fuerte':'#ffc06c',
 'alerta':'#e5795e','VELO_PORTAL':'#59dcca',
 'CRISTAL_ESCUDO':'#6cdded','CRISTAL_CARGADO':'#ffd284'
})

def jungla():
 m=mesh();box(m,(0,-.8,-10),(120,1,90),'tierra')
 for i in range(60):
  x=-55+(i*29%111);z=-53+(i*47%88)
  ellipsoid(m,(x,-.27,z),(.65,.10,.34),'hoja' if i%3 else 'musgo',8,4)
 return m

def plaza():
 m=mesh();box(m,(0,-.2,-2),(17.2,.4,24),'suelo')
 for k in range(9):box(m,(-8.6+k*17.2/8,.006,-2),(.053,.012,23.95),'junta')
 for k in range(12):box(m,(0,.006,-13.95+k*2),(17.16,.012,.055),'junta')
 for k in range(16):
  x=-7.9+(k*11%158)/10;z=-11+(k*19%196)/10
  ellipsoid(m,(x,.01,z),(.45+(k%3)*.13,.01,.27),'musgo_oscuro' if k%2 else 'musgo',9,4)
 return m

def muro_lateral():
 m=mesh();box(m,(0,.8,0),(1,1.6,2.4),'piedra_sombra')
 box(m,(0,1.63,0),(1.12,.13,2.5),'musgo')
 for z in (-.85,.15,.95):box(m,(.52,.82,z),(.035,.043,.72),'junta')
 return m

def escalinata():
 m=mesh()
 for k in range(4):
  z=-k*1.0;y=.15+k*.3
  box(m,(0,y,z),(11-k*.6,.3+k*.6,1.0),'piedra')
  box(m,(0,.31+k*.6,z),(10.7-k*.6,.025,.88),'piedra_luz')
  for s in (-1,1):ellipsoid(m,(s*(5.4-k*.3),.3+k*.6,z),(.15,.04,.16),'musgo',8,4)
 return m

def templo():
 m=mesh();box(m,(0,5,0),(24,10,2),'piedra_sombra')
 box(m,(0,10.2,0),(25,.5,2.4),'musgo')
 for x in (-11,-8,8,11):
  box(m,(x,7,1.04),(.06,5,.04),'piedra_luz')
  box(m,(x,4,1.03),(.06,1.2,.04),'musgo_oscuro')
 for k in range(11):box(m,(-11+k*2.2,9.85,1.1),(.75,.25,.10),'piedra')
 return m

def muro_fondo():
 m=mesh();box(m,(0,1.4,0),(6.2,2.8,.8),'piedra')
 box(m,(0,2.86,0),(6.3,.12,.9),'musgo')
 for x in (-2.4,0,2.4):box(m,(x,1.2,.42),(.045,1.9,.035),'junta')
 return m

def portal_marco():
 m=mesh()
 for x in (-3,3):
  box(m,(x,2.6,0),(1,5.2,1),'piedra')
  box(m,(x,5.3,0),(1.3,.4,1.3),'piedra_sombra')
  for y in (.8,1.8,2.8,3.8,4.8):
   box(m,(x,y,.53),(.78,.038,.035),'junta')
 box(m,(0,5.5,0),(7,.9,1.1),'piedra')
 box(m,(0,6,0),(7,.12,1.2),'musgo')
 for x in (-3.35,3.35):
  for y in (1.2,2.5,3.8):ellipsoid(m,(x,y,.58),(.07,.08,.03),'musgo',7,4)
 return m

def runa():
 m=mesh()
 # Glifo independiente para pulso, pivot en centro de símbolo.
 beam(m,(-.13,-.17,0),(0,.18,0),.027,'runa',6)
 beam(m,(0,.18,0),(.14,-.17,0),.027,'runa',6)
 beam(m,(-.08,-.03,0),(.09,-.03,0),.026,'luz',6)
 ellipsoid(m,(0,.19,0),(.045,.05,.04),'luz',8,4)
 return m

def velo():
 m=mesh();box(m,(0,2.5,0),(4.9,4.95,.055),'VELO_PORTAL')
 for k in range(5):
  y=.65+k*.93
  box(m,(0,y,.045),(1.4-.14*k,.04,.027),'luz')
  ellipsoid(m,(-1.7+k*.2,y+.3,.04),(.08,.09,.04),'agua',8,4)
 torus(m,(0,2.52,.06),2.08,.035,'runa',30,4,plane='xy')
 return m

def linea_meta():
 m=mesh();box(m,(0,.03,0),(5,.04,.3),'runa')
 for x in (-2.35,2.35):ellipsoid(m,(x,.06,0),(.10,.08,.12),'luz',9,5)
 return m

def columna():
 m=mesh();box(m,(0,.15,0),(1.5,.3,1.5),'piedra_sombra')
 cylinder(m,(0,.3,0),.6,3.7,'piedra',12,r_top=.54)
 for k in range(3):torus(m,(0,1+k*1.15,0),.59-.02*k,.03,'piedra_luz',18,4)
 box(m,(0,4.15,0),(1.6,.3,1.6),'piedra_sombra')
 cylinder(m,(0,2.95,0),.63,.15,'musgo',12)
 for a in (.15,2.2,4.25):
  pts=[]
  for j in range(9):
   y=4.03-j*.32;ang=a+j*.19
   pts.append((.64*math.cos(ang),y,.64*math.sin(ang)))
  for i in range(8):beam(m,pts[i],pts[i+1],.034,'musgo_oscuro',5)
  for j in (2,5):ellipsoid(m,pts[j],(.15,.08,.10),'hoja',8,4)
 return m

def aviso_columna():
 m=mesh();torus(m,(0,.045,0),1.15,.06,'alerta',28,5)
 torus(m,(0,.055,0),.87,.035,'orbe_naranja',26,4)
 for a in range(8):
  t=a*TAU/8
  ellipsoid(m,(1.16*math.cos(t),.07,1.16*math.sin(t)),(.055,.04,.055),'alerta',7,4)
 return m

def columna_caida():
 m=mesh();box(m,(0,.15,0),(1.4,.3,1.4),'piedra_sombra')
 beam(m,(-.8,.45,0),(.9,.45,0),.5,'piedra',12)
 beam(m,(.85,.43,.15),(1.65,.43,.15),.43,'piedra_sombra',12)
 for i in range(7):
  x=-1.5+i*.4
  box(m,(x,.07,(i%3-1)*.45),(.15,.11,.15),'piedra_luz' if i%2 else 'piedra_sombra')
 return m

def estatua():
 m=mesh();box(m,(0,.18,0),(2.5,.35,1.0),'piedra')
 ellipsoid(m,(1.38,.5,0),(.52,.50,.50),'piedra_luz',12,7)
 box(m,(-.85,.48,.08),(.48,.62,.4),'piedra_sombra')
 for x in (-1,.3):box(m,(x,.55,.48),(.38,.05,.04),'musgo')
 return m

def arbol():
 m=mesh();cylinder(m,(0,0,0),.62,5.9,'arbol',9,r_top=.39)
 for x,y,z,r in ((0,6.4,0,2.6),(1.7,5.8,.8,1.9),(-1.5,5.5,-.55,1.8)):
  ellipsoid(m,(x,y,z),(r,r*.8,r*.85),'hoja',12,7)
  ellipsoid(m,(x-.25,y+.3,z+.2),(r*.54,r*.4,r*.56),'hoja_luz',10,6)
 for s in (-1,1):beam(m,(0,3.6,0),(s*1.6,5.8,s*.3),.18,'arbol',8,r_end=.07)
 return m

def arbusto():
 m=mesh()
 for x,y,z,r in ((0,.45,0,.55),(-.4,.38,.18,.37),(.4,.48,-.1,.42)):
  ellipsoid(m,(x,y,z),(r,r*.85,r),'hoja',11,6)
  ellipsoid(m,(x-.1,y+.16,z+.2),(r*.35,r*.24,r*.3),'hoja_luz',9,5)
 return m

def orbe(returned=False):
 m=mesh();base='orbe_naranja' if returned else 'orbe_azul';halo='halo_naranja' if returned else 'halo_azul'
 ellipsoid(m,(0,0,0),(.35,.35,.35),base,16,8)
 torus(m,(0,0,0),.46,.035,halo,24,5,plane='xy')
 torus(m,(0,0,0),.46,.027,'luz',24,5,plane='xz')
 for i in range(4):
  a=i*TAU/4
  ellipsoid(m,(.49*math.cos(a),.09,.49*math.sin(a)),(.055,.06,.055),'luz',8,4)
 return m

def escudo(fuerte=False):
 m=mesh();body='CRISTAL_CARGADO' if fuerte else 'CRISTAL_ESCUDO'
 border='escudo_fuerte' if fuerte else 'escudo'
 box(m,(0,0,0),(3.7,1.45,.12),body)
 for x in (-1.9,1.9):box(m,(x,0,0),(.11,1.55,.24),border)
 for y in (-.76,.76):box(m,(0,y,0),(3.87,.10,.24),border)
 box(m,(0,-.8,0),(4,.2,.4),'piedra_sombra')
 for x in (-1.2,0,1.2):ellipsoid(m,(x,0,.13),(.11,.11,.03),'luz' if not fuerte else 'orbe_naranja',9,5)
 return m

def main():
 specs=[
  ('terreno_selva',jungla(),'centro global','Origen; suelo exterior a Y≈-.3.'),
  ('plaza_losas',plaza(),'centro de plaza','Origen; suelo de juego X±8.6, Z=-14..10.'),
  ('muro_lateral',muro_lateral(),'base centro','16 tramos X=±9.1, Z=-11+2.6*k; colisiones siguen X±8.6.'),
  ('escalinata',escalinata(),'centro primer peldaño','Centro (0,0,-13.3), detrás de la línea de gol.'),
  ('templo_fondo',templo(),'pie centro','Centro (0,0,-18).'),
  ('muro_fondo',muro_fondo(),'pie centro','Centros X=±6.4,Z=-12.7.'),
  ('portal_marco',portal_marco(),'pie central','Centro (0,0,-12.7), hueco ancho 5.'),
  ('runa',runa(),'centro símbolo','Instanciar sobre jambas frontales en X≈±3,Y=1.2,2.5,3.8.'),
  ('velo_portal',velo(),'pie central','Centro (0,0,-12.5); material translúcido independiente.'),
  ('linea_meta',linea_meta(),'centro','Centro (0,0,-12.1), barra de gol.'),
  ('columna_entera',columna(),'pie central','Seis posiciones POSICIONES_COLUMNAS; colisión radio=.6.'),
  ('aviso_columna',aviso_columna(),'centro en suelo','Visible solo mientras estado COLUMNA_GUARDIAN_AVISO.'),
  ('columna_caida',columna_caida(),'centro en suelo','Reemplaza columna_entera tras derrumbe; deja de ser sólida.'),
  ('estatua_caida',estatua(),'centro en suelo','Decoración X≈±7.4,Z≈7.6.'),
  ('arbol_selva',arbol(),'pie central','Varios fuera de límites de plaza.'),
  ('arbusto',arbusto(),'pie central','Decoración lateral X≈±8.4.'),
  ('orbe_azul',orbe(),'centro','En (orbe.x,.9,orbe.z), escala pulso sin cambiar colisión.'),
  ('orbe_devuelto',orbe(True),'centro','Sustituir solo si orbe.devuelto; color naranja.'),
  ('escudo_guardian',escudo(),'centro geométrico','Centro (guardianX,1,ZEscudo); ancho 3.8, altura 1.5.'),
  ('escudo_embestida',escudo(True),'centro geométrico','Sustituir durante embestida, mismo pivote y colisión.'),
 ]
 for name,m,pivot,usage in specs:save(name,m,pivot,usage)
 (ROOT/'manifest.json').write_text(json.dumps({'minijuego':'Guardián de Ruinas','base':'claude/expansion-party d17f4ea','coordenadas':'Y arriba; X plaza ±8.6; portal Z=-12.3; radio columna .6','modelos':MANIFEST},ensure_ascii=False,indent=2)+'\n')
 print(len(specs),'GLB;',sum(x['triangulos'] for x in MANIFEST),'triángulos')

if __name__=='__main__':main()
