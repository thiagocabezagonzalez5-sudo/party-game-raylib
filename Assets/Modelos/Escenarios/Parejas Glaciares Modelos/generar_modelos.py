"""Arte glacial modular para Parejas Glaciares. Python 3 estándar."""
from geometria_glb import *
import geometria_glb as glb

glb.PALETTE.update({
 'noche':'#0d2446','agua':'#173d68','hielo':'#a8daf2',
 'hielo_luz':'#e6f8fb','hielo_sombra':'#6ea4d0','hielo_profundo':'#376b9b',
 'nieve':'#f4f8f8','montana':'#536b92','cueva':'#163d76',
 'rojo':'#ef666c','azul':'#6395ed','amarillo':'#f3d864',
 'verde':'#6ddca0','violeta':'#af7ce4','naranja':'#f3ad60',
 'turquesa':'#61ded4','aurora':'#8beecd','negro':'#2c3546',
 'piel':'#8b99a8','pico':'#edab55','AURORA_VERDE':'#55f4af',
 'AURORA_VIOLETA':'#d185f2','AURORA_CIAN':'#7cdeef',
 'COLOR_JUGADOR':'#ffffff'
})

def mar():
 m=mesh();box(m,(0,-.63,-6),(120,.11,90),'agua')
 for k in range(48):
  x=-50+(k*31%100);z=-40+(k*61%80)
  ellipsoid(m,(x,-.55,z),(1.1+(k%4)*.5,.018,.23),'hielo_profundo',8,4)
 return m

def lago():
 m=mesh();box(m,(0,-.35,0),(20,.5,11),'hielo')
 box(m,(0,-.065,0),(19.5,.04,10.4),'hielo_luz')
 for k in range(14):
  x=-9+(k*17%180)/10;z=-4.5+(k*13%90)/10
  beam(m,(x,-.037,z),(x+.4+(k%3)*.2,-.037,z+.2),.018,'hielo_sombra',5)
 return m

def tablero():
 m=mesh();box(m,(0,-.12,0),(8.2,.28,8.2),'hielo_sombra')
 box(m,(0,.025,0),(8.05,.01,8.05),'hielo')
 for i in range(-2,3):
  x=i*1.9
  box(m,(x,.035,0),(.035,.016,7.85),'hielo_profundo')
  box(m,(0,.035,x),(7.85,.016,.035),'hielo_profundo')
 return m

def bloque(matched=False):
 m=mesh();h=.9 if not matched else .26
 box(m,(0,h*.5,0),(1.65,h,1.65),'hielo' if not matched else 'hielo_sombra')
 box(m,(0,h+.006,0),(1.52,.012,1.52),'hielo_luz' if not matched else 'hielo')
 for x in (-.8,.8):box(m,(x,h*.5,0),(.027,h,1.65),'hielo_profundo')
 for z in (-.8,.8):box(m,(0,h*.5,z),(1.65,h,.027),'hielo_profundo')
 if not matched:
  for k in range(6):
   a=k*math.pi/3
   beam(m,(0,h+.03,0),(.54*math.cos(a),h+.03,.54*math.sin(a)),.022,'hielo_sombra',5)
 else:
  torus(m,(0,h+.03,0),.6,.035,'COLOR_JUGADOR',24,5)
 return m

def cursor():
 m=mesh()
 for s in (-1,1):
  box(m,(s*.91,.58,0),(.055,1.12,1.86),'amarillo')
  box(m,(0,.58,s*.91),(1.86,1.12,.055),'amarillo')
  ellipsoid(m,(s*.91,1.15,s*.91),(.075,.075,.075),'nieve',8,4)
 return m

def simbolo(i):
 m=mesh();mat=['rojo','azul','amarillo','verde','violeta','naranja','turquesa','aurora'][i]
 if i==0:ellipsoid(m,(0,.35,0),(.35,.35,.35),mat,12,7)
 elif i==1:box(m,(0,.34,0),(.62,.62,.62),mat)
 elif i==2:cylinder(m,(0,0,0),.37,.8,mat,10,r_top=.0)
 elif i==3:cylinder(m,(0,0,0),.28,.7,mat,12)
 elif i==4:
  cylinder(m,(0,0,0),.02,.4,mat,4,r_top=.35)
  cylinder(m,(0,.4,0),.35,.4,mat,4,r_top=.02)
 elif i==5:
  ellipsoid(m,(0,.29,0),(.3,.3,.3),mat,10,6)
  ellipsoid(m,(0,.76,0),(.2,.2,.2),mat,10,6)
 elif i==6:
  for k in range(6):
   a=k*TAU/6
   ellipsoid(m,(.33*math.cos(a),.15,.33*math.sin(a)),(.11,.11,.11),mat,8,4)
 else:
  # Aurora, pareja especial: estrella tridimensional de seis radios.
  for axis in ((1,0,0),(0,1,0),(0,0,1)):
   a=(0,.4,0);b=add(a,mul(axis,.45));c=add(a,mul(axis,-.45))
   beam(m,a,b,.08,mat,7,r_end=.02);beam(m,a,c,.08,mat,7,r_end=.02)
  ellipsoid(m,(0,.4,0),(.21,.21,.21),'nieve',10,6)
 return m

def tempano():
 m=mesh();cylinder(m,(0,-.45,0),2.35,.52,'hielo',16,r_top=2.1)
 cylinder(m,(0,.075,0),1.95,.04,'hielo_luz',16)
 for k in range(10):
  a=k*TAU/10
  ellipsoid(m,(2.2*math.cos(a),-.1,2.2*math.sin(a)),(.24,.13,.27),'hielo_sombra',7,4)
 return m

def iceberg():
 m=mesh();cylinder(m,(0,-.5,0),3.2,4.5,'hielo',5,r_top=0)
 cylinder(m,(0,1.9,0),1.58,2.1,'hielo_luz',5,r_top=0)
 for k in range(5):
  a=k*TAU/5;x=2.2*math.cos(a);z=2.2*math.sin(a)
  cylinder(m,(x,-.5,z),.5,1.6+(k%2)*.5,'hielo_sombra',4,r_top=0)
 return m

def montana():
 m=mesh();cylinder(m,(0,-.5,0),6,11,'montana',6,r_top=0)
 cylinder(m,(0,6.1,0),2.45,4.4,'nieve',6,r_top=0)
 for k in range(5):
  a=k*TAU/5
  ellipsoid(m,(4.8*math.cos(a),.1,4.8*math.sin(a)),(.35,.2,.46),'hielo_profundo',8,4)
 return m

def cueva():
 m=mesh()
 box(m,(0,1.8,-.2),(9.2,3.6,2),'cueva')
 for x in (-4,4):
  box(m,(x,1.8,0),(1.2,3.6,1.5),'hielo_sombra')
  for y in (.7,2.6):ellipsoid(m,(x,y,.84),(.28,.18,.15),'hielo_luz',9,5)
 box(m,(0,3.9,0),(9.2,1.2,1.5),'hielo')
 box(m,(0,2,0.83),(6.8,3.6,.4),'cueva')
 for x in (-3.2,-2.1,0,2.3,3.1):
  cylinder(m,(x,3.15,.78),.26,.85,'hielo_luz',5,r_top=0)
 return m

def pinguino():
 m=mesh();cylinder(m,(0,0,0),.32,.8,'negro',10,r_top=.27)
 ellipsoid(m,(0,.55,.21),(.27,.36,.17),'nieve',11,6)
 ellipsoid(m,(0,.98,0),(.25,.25,.25),'negro',10,6)
 box(m,(0,.96,.27),(.13,.065,.18),'pico')
 for x in (-.11,.11):ellipsoid(m,(x,1.03,.22),(.035,.04,.03),'nieve',7,4)
 for s in (-1,1):
  beam(m,(s*.27,.62,0),(s*.54,.27,-.08),.10,'negro',7,r_end=.03)
  ellipsoid(m,(s*.17,.06,.17),(.17,.055,.21),'pico',8,4)
 return m

def foca():
 m=mesh();beam(m,(-.7,.4,0),(.5,.4,0),.40,'piel',12,r_end=.34)
 ellipsoid(m,(.72,.56,0),(.34,.30,.29),'piel',12,7)
 ellipsoid(m,(-.75,.38,0),(.42,.27,.32),'piel',10,6)
 for z in (-.14,.14):ellipsoid(m,(.94,.63,z),(.055,.055,.045),'negro',8,5)
 for s in (-1,1):
  ellipsoid(m,(-.15,.1,s*.4),(.46,.10,.2),'piel',10,5)
  ellipsoid(m,(-.94,.36,s*.22),(.34,.09,.18),'piel',9,5)
 return m

def aurora(mat):
 m=mesh()
 for k in range(12):
  x=-2.5+k*.45;y=12+2.1*math.sin(k*.38)
  w=.48
  a=(x,y-4,-.06);b=(x+w,y-4,0);c=(x+w,y+4,.08);d=(x,y+4,.04)
  quad(m,a,b,c,d,mat);quad(m,d,c,b,a,mat)
  beam(m,(x,y-3,.1),(x+w,y-3,.1),.025,'nieve',5)
 return m

def fragmento():
 m=mesh();cylinder(m,(0,0,0),.32,.7,'hielo_luz',5,r_top=0)
 ellipsoid(m,(0,.05,0),(.35,.05,.32),'hielo_sombra',8,4)
 return m

def main():
 specs=[
  ('mar_frio',mar(),'centro global','Origen, plano oscuro bajo toda la escena.'),
  ('lago_helado',lago(),'centro lago','Origen; borde de hielo para tablero y témpanos.'),
  ('tablero_4x4',tablero(),'centro tablero','Origen; centros de bloque x/z=(índice%4−1.5)*1.9.'),
  ('bloque_oculto',bloque(),'pie centro','Todos los bloques sin revelar, 1.65×.9×1.65.'),
  ('bloque_emparejado',bloque(True),'pie centro','Variante baja tras pareja; símbolo solo visible cuando procede.'),
  ('cursor_seleccion',cursor(),'pie centro','Posición del cursor activo, entorno del bloque sin revelar solución.'),
  ('simbolo_0_esfera',simbolo(0),'base centro','Solo mostrar al revelar un bloque con símbolo 0.'),
  ('simbolo_1_cubo',simbolo(1),'base centro','Símbolo 1, pieza separada.'),
  ('simbolo_2_cono',simbolo(2),'base centro','Símbolo 2, pieza separada.'),
  ('simbolo_3_cilindro',simbolo(3),'base centro','Símbolo 3, pieza separada.'),
  ('simbolo_4_rombo',simbolo(4),'base centro','Símbolo 4, pieza separada.'),
  ('simbolo_5_nieve',simbolo(5),'base centro','Símbolo 5, pieza separada.'),
  ('simbolo_6_copo',simbolo(6),'base centro','Símbolo 6, pieza separada.'),
  ('simbolo_7_aurora',simbolo(7),'base centro','Pareja especial aurora, 3 puntos.'),
  ('tempano_jugador',tempano(),'pie centro','Uno por jugador en posición de témpano de minijuego.'),
  ('iceberg',iceberg(),'base central','Al fondo (-12,-.5,-9),(11,-.5,-10),(15,-.5,1).'),
  ('montana_nevada',montana(),'base central','Repetir en X=-22+8.5*i,Z=-20.'),
  ('cueva_hielo',cueva(),'pie centro','Centro (0,0,-11).'),
  ('pinguino',pinguino(),'pie centro','Decoración sobre icebergs, fuera del tablero.'),
  ('foca',foca(),'pie centro','Decoración aprox (12.5,-.4,-3).'),
  ('aurora_verde',aurora('AURORA_VERDE'),'centro de banda','Fondo Z=-26, material translúcido; repetir.'),
  ('aurora_violeta',aurora('AURORA_VIOLETA'),'centro de banda','Segunda banda del cielo; independiente.'),
  ('aurora_cian',aurora('AURORA_CIAN'),'centro de banda','Tercera banda, animar desplazando el nodo.'),
  ('fragmento_hielo',fragmento(),'base centro','Partículas visuales al crujir; no altera símbolos.'),
 ]
 for name,m,pivot,usage in specs:save(name,m,pivot,usage)
 (ROOT/'manifest.json').write_text(json.dumps({'minijuego':'Parejas Glaciares','base':'claude/expansion-party f9f6713','coordenadas':'Y arriba; tablero 4x4; paso 1.9; bloque y=0..0.9','modelos':MANIFEST},ensure_ascii=False,indent=2)+'\n')
 print(len(specs),'GLB;',sum(x['triangulos'] for x in MANIFEST),'triángulos')

if __name__=='__main__':main()
