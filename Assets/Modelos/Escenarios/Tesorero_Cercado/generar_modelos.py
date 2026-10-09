"""Modelos originales GLB 2.0 para Tesorero Cercado, sin dependencias externas."""
from geometria_glb import *
import geometria_glb as glb

glb.PALETTE.update({
 'agua':'#366c92','agua_clara':'#5d9bad','espuma':'#9bc8cf',
 'piedra':'#898880','piedra_clara':'#aca99b','piedra_oscura':'#615f5e',
 'junta':'#4c4b4a','arena':'#b2a184','madera':'#775038',
 'madera_clara':'#ac7645','madera_oscura':'#3c2d2a',
 'hierro':'#454a53','hierro_claro':'#80858a','hierro_oscuro':'#292e38',
 'rojo':'#b64149','rojo_claro':'#df7770','rojo_oscuro':'#782c3e',
 'oro':'#e6b94c','oro_claro':'#fbe19a','oro_oscuro':'#9d7735',
 'fuego':'#f69b40','fuego_claro':'#ffe087','verde':'#667e68',
 'COLOR_DINAMICO':'#ffffff','BOMBILLAS':'#fff4bb'
})

def foso():
 m=mesh();box(m,(0,-.94,0),(36,1.5,28),'agua')
 # La plataforma jugable tapa el centro, dejando visible la lámina alrededor.
 for x in range(-15,16,3):
  for z in (-11,-9,9,11):
   if (x+z)%4==0:torus(m,(x,-.179,z),.65,.024,'agua_clara',16,4)
 for z in range(-12,13,3):
  for x in (-16,-13,13,16):
   if (x-z)%4==0:torus(m,(x,-.179,z),.56,.017,'espuma',16,4)
 for x,z in [(-11,-8),(11,-8),(-11,8),(11,8)]:
  ellipsoid(m,(x,-.08,z),(.65,.1,.46),'piedra_oscura',10,5)
 return m

def suelo():
 m=mesh();box(m,(0,-.5,0),(18,1,14),'piedra_oscura')
 # Losa completa dentro del borde lógico 18 x 14, sin resaltes de colisión.
 for ix in range(9):
  for iz in range(7):
   x=-8+2*ix;z=-6+2*iz
   box(m,(x,.006,z),(1.93,.025,1.93),'piedra_clara' if (ix+iz)%3==0 else 'piedra')
   if (ix*7+iz*11)%9==0:
    beam(m,(x-.45,.027,z-.35),(x+.22,.029,z+.18),.013,'junta',5)
 for x in [-8.9,8.9]:
  box(m,(x,.065,0),(.13,.13,14),'arena')
 for z in [-6.9,6.9]:
  box(m,(0,.065,z),(18,.13,.13),'arena')
 return m

def ladrillos(m,largo,alto,fondo,eje='x'):
 # Juntas en la cara interior para que el muro conserve una silueta simple.
 for y in (.68,1.37,2.05):
  if y>=alto-.13:continue
  if eje=='x':box(m,(0,y,fondo/2+.013),(largo,.023,.028),'junta')
  else:box(m,(-fondo/2-.013,y,0),(.028,.023,largo),'junta')
 for fila in range(int(alto/.68)):
  y=.34+fila*.68
  for i in range(-12,13):
   u=i*1.35+(fila%2)*.65
   if abs(u)>largo/2-.4:continue
   if eje=='x':box(m,(u,y,fondo/2+.015),(.025,.53,.03),'junta')
   else:box(m,(-fondo/2-.015,y,u),(.03,.53,.025),'junta')

def muro_fondo():
 m=mesh();box(m,(0,1.3,0),(20.4,2.6,1.2),'piedra')
 box(m,(0,2.61,0),(20.6,.17,1.38),'piedra_clara')
 ladrillos(m,20.4,2.6,1.2)
 for i in range(17):
  x=-9.6+1.2*i
  box(m,(x,2.98,0),(.72,.54,1.26),'piedra_oscura')
  box(m,(x,3.26,0),(.76,.06,1.3),'piedra_clara')
 for x in (-8.4,-4.8,4.8,8.4):
  box(m,(x,1.48,.64),(.20,2.15,.12),'piedra_oscura')
 return m

def muro_lateral():
 m=mesh();box(m,(0,1.1,0),(1.2,2.2,15.2),'piedra')
 box(m,(0,2.23,0),(1.38,.14,15.35),'piedra_clara')
 ladrillos(m,15.2,2.2,1.2,'z')
 for i in range(12):
  z=-6.6+1.2*i
  box(m,(0,2.46,z),(1.2,.50,.70),'piedra_oscura')
  box(m,(0,2.74,z),(1.25,.06,.74),'piedra_clara')
 return m

def parapeto():
 m=mesh();box(m,(0,.25,0),(20.4,.5,1.2),'piedra')
 box(m,(0,.51,0),(20.5,.08,1.32),'piedra_clara')
 for x in range(-9,10,2):box(m,(x,.32,.62),(.024,.32,.025),'junta')
 for x in (-9.6,9.6):box(m,(x,.62,0),(.65,.25,1.35),'piedra_oscura')
 return m

def torre_esquina(alta):
 m=mesh();h=4.4 if alta else 2.4
 cylinder(m,(0,0,0),1.5,h,'piedra',12)
 cylinder(m,(0,h-.20,0),1.61,.25,'piedra_clara',12)
 for y in (.8,1.6,2.4,3.2):
  if y<h-.4:torus(m,(0,y,0),1.506,.025,'piedra_oscura',12,4)
 for i in range(12):
  a=TAU*i/12
  if i%2:continue
  box(m,(1.51*math.cos(a),h-.47,1.51*math.sin(a)),(.14,.44,.24),'piedra_oscura')
 cylinder(m,(0,h+.045,0),1.8,1.55,'rojo',12,r_top=0)
 torus(m,(0,h+.06,0),1.81,.055,'oro_oscuro',12,5)
 cylinder(m,(0,h+1.47,0),.065,.35,'oro',8,r_top=.015)
 # Troneras orientadas al patio.
 for a in (math.pi/3,math.pi,5*math.pi/3):
  x,z=1.504*math.cos(a),1.504*math.sin(a)
  box(m,(x,h*.56,z),(.17,.55,.08),'hierro_oscuro')
 return m

def torre_homenaje():
 m=mesh();box(m,(0,1.10,0),(2.60,2.20,2.60),'piedra')
 for y in (.54,1.08,1.62,2.16):
  box(m,(0,y,1.316),(2.6,.028,.04),'junta')
 for x in (-1.12,1.12):
  box(m,(x,1.1,1.34),(.10,2.1,.08),'piedra_clara')
 box(m,(0,1.21,1.35),(.68,1.25,.07),'madera_oscura')
 box(m,(0,1.99,1.36),(.82,.10,.12),'piedra_oscura')
 box(m,(0,2.24,0),(2.75,.12,2.75),'piedra_clara')
 for x in (-1.12,0,1.12):
  for z in (-1.12,1.12):box(m,(x,2.48,z),(.40,.44,.40),'piedra_oscura')
 for x in (-1.12,1.12):box(m,(x,2.48,0),(.40,.44,.40),'piedra_oscura')
 for z in (-1.12,1.12):box(m,(0,2.48,z),(.40,.44,.40),'piedra_oscura')
 # Estandarte de remate, compactado en el mismo módulo.
 cylinder(m,(0,2.56,0),.045,1.58,'madera',7)
 quad(m,(0,3.97,.02),(.72,3.88,.02),(.66,3.55,.02),(0,3.59,.02),'oro')
 quad(m,(0,3.59,.02),(.66,3.55,.02),(.72,3.88,.02),(0,3.97,.02),'oro')
 return m

def portal():
 m=mesh()
 # Panel oscuro del portón sobre la cara que mira al jugador (+Z).
 box(m,(0,1.11,0),(2.42,2.20,.15),'madera_oscura')
 for x in (-.95,-.53,-.11,.31,.73,1.13):
  box(m,(x,1.1,.09),(.034,2.16,.035),'madera_clara')
  for y in (.32,1.0,1.75):ellipsoid(m,(x,y,.12),(.035,.035,.02),'hierro',7,4)
 for x in (-1.43,1.43):box(m,(x,1.3,.02),(.38,2.6,.41),'piedra_oscura')
 box(m,(0,2.62,.02),(3.25,.39,.43),'piedra_oscura')
 box(m,(0,2.84,.02),(3.35,.08,.48),'piedra_clara')
 torus(m,(0,1.1,.15),.20,.04,'oro_oscuro',14,5,plane='xy')
 return m

def estandarte(rojo):
 m=mesh();c='rojo' if rojo else 'oro';d='rojo_claro' if rojo else 'oro_claro'
 cylinder(m,(0,0,0),.05,1.8,'madera',7)
 cylinder(m,(0,1.72,0),.13,.12,'oro',9,r_top=0)
 beam(m,(0,1.54,0),(.96,1.54,0),.026,'madera_clara',7)
 a=(.05,1.50,0);b=(.88,1.50,.04);c1=(.81,.30,.03);d1=(.49,.48,.12);e=(.08,.28,0)
 tri(m,a,b,c1,c);tri(m,a,c1,d1,c);tri(m,a,d1,e,c)
 tri(m,c1,b,a,c);tri(m,d1,c1,a,c);tri(m,e,d1,a,c)
 beam(m,(.17,1.39,.055),(.17,.52,.06),.018,d,5)
 ellipsoid(m,(.44,1.10,.065),(.14,.14,.025),'oro_claro' if rojo else 'rojo',10,5)
 return m

def antorcha():
 m=mesh()
 box(m,(0,1.02,0),(.14,1.30,.14),'madera')
 cylinder(m,(0,1.45,0),.23,.18,'hierro',9,r_top=.18)
 torus(m,(0,1.59,0),.17,.025,'hierro_claro',12,4)
 ellipsoid(m,(0,1.84,0),(.16,.28,.16),'fuego',12,6)
 ellipsoid(m,(0,1.91,.06),(.075,.18,.09),'fuego_claro',10,5)
 return m

def marco_reja():
 m=mesh()
 # Pivote al centro; marco fijo en Y=0. Para X=±5, girar Y=90 grados.
 for x in (-2.15,2.15):
  box(m,(x,2.7,0),(.50,5.4,.50),'piedra')
  for y in (.25,1.55,2.85,4.15,5.27):
   box(m,(x,y,.255),(.50,.09,.045),'piedra_clara')
  box(m,(x,5.48,0),(.64,.18,.64),'piedra_oscura')
  # Polea, eje hacia la cámara en +Z.
  torus(m,(x,4.92,.40),.30,.055,'oro_oscuro',20,5,plane='xy')
  torus(m,(x,4.92,.43),.18,.03,'hierro_claro',16,4,plane='xy')
  cylinder(m,(x,5.30,0),.17,.18,'piedra_oscura',8)
 box(m,(0,5.39,0),(4.75,.22,.43),'piedra_oscura')
 for x in (-1.98,1.98):beam(m,(x,5.13,.46),(x,3.83,.46),.018,'oro_oscuro',5)
 return m

def reja():
 m=mesh()
 # Solo la hoja móvil. Dibujar en Y=(1-altura)*2.6; baja del todo en Y=0.
 for i in range(6):
  x=((i+.5)/6-.5)*4
  box(m,(x,1.19,0),(.115,2.38,.13),'hierro')
  cylinder(m,(x,2.38,0),.078,.17,'hierro_claro',6,r_top=0)
 for y in (.61,1.94):box(m,(0,y,.01),(4.03,.14,.17),'hierro_oscuro')
 for x in (-2.01,2.01):box(m,(x,1.15,0),(.11,2.3,.18),'hierro_claro')
 for x in (-1.5,0,1.5):
  ellipsoid(m,(x,1.94,.11),(.047,.047,.033),'oro_oscuro',8,4)
 return m

def aviso_reja():
 m=mesh();box(m,(0,.025,0),(4.58,.045,1.02),'rojo_oscuro')
 for x in (-1.93,-.96,0,.96,1.93):
  box(m,(x,.054,0),(.14,.015,1.02),'rojo')
 box(m,(0,.054,-.45),(4.56,.015,.09),'rojo_claro')
 box(m,(0,.054,.45),(4.56,.015,.09),'rojo_claro')
 return m

def moneda():
 m=mesh()
 # Cara vertical +Z, gira alrededor de Y como en el minijuego.
 beam(m,(0,0,-.045),(0,0,.045),.20,'oro',12)
 torus(m,(0,0,.052),.158,.015,'oro_claro',16,4,plane='xy')
 torus(m,(0,0,.058),.12,.012,'oro_oscuro',16,4,plane='xy')
 star(m,(0,0,.068),.085,'oro_claro')
 return m

def main():
 specs=[
  ('foso_agua',foso(),'centro del patio','(0,0,0); agua alrededor del suelo jugable.'),
  ('suelo_losas',suelo(),'centro del patio','(0,0,0); huella 18 x 14, superficie Y=0.'),
  ('muro_fondo',muro_fondo(),'base central','(0,0,-7.6); almenas al fondo.'),
  ('muro_lateral',muro_lateral(),'base central','Repetir en (±9.6,0,0); muro a lo largo de Z.'),
  ('parapeto_frontal',parapeto(),'base central','(0,0,7.6); altura .5 para no tapar jugadores.'),
  ('torre_esquina_alta',torre_esquina(True),'base central','(±10.4,0,-8.2); altura 4.4 + tejado 1.65.'),
  ('torre_esquina_baja',torre_esquina(False),'base central','(±10.4,0,8.2); altura 2.4 + tejado 1.65.'),
  ('torre_homenaje',torre_homenaje(),'centro del obstaculo','(0,0,0); cuerpo 2.6 x 2.6 y altura de colision 2.2.'),
  ('portal_fondo',portal(),'centro del porton','(0,0,-6.95); montaje en muro del fondo.'),
  ('estandarte_rojo',estandarte(True),'pie del asta','(x,2.8,-7.2), x=-7.2+3.6*k para k par.'),
  ('estandarte_dorado',estandarte(False),'pie del asta','(x,2.8,-7.2), x=-7.2+3.6*k para k impar.'),
  ('antorcha',antorcha(),'base del soporte','(±8.8,0,-4.5+4.5*j), tres por lado.'),
  ('marco_reja',marco_reja(),'centro de reja al nivel del suelo','Centros (±5,0,0),(0,0,±4.2); girar 90 grados para las dos de X.'),
  ('reja_levadiza',reja(),'centro de reja al nivel del suelo','Centro x,z del estado; Y=(1-reja.altura)*2.6; girar 90 grados si mitadZ>mitadX.'),
  ('aviso_reja',aviso_reja(),'centro de aviso en suelo','Centro x,z; visible solo durante estado AVISO; girar 90 grados si mitadZ>mitadX.'),
  ('moneda_tesoro',moneda(),'centro de moneda','Centro de moneda.y+.12; conservar vuelo, giro y recogida en C++.'),
 ]
 for name,model,pivot,usage in specs:save(name,model,pivot,usage)
 (glb.ROOT/'manifest.json').write_text(json.dumps({
  'version':1,'rama_referencia':'claude/expansion-party','commit_referencia':'ea5472c',
  'unidades':'1 unidad GLB = 1 unidad raylib','ejes':'Y arriba, cámara hacia Z negativo',
  'licencia':'Arte original procedural sin recursos externos',
  'nota':'La altura/estado de las rejas, colisiones, monedas y puntaje siguen en C++.',
  'modelos':glb.MANIFEST},ensure_ascii=False,indent=2),encoding='utf8')
 print(len(glb.MANIFEST),'GLB;',sum(x['triangulos'] for x in glb.MANIFEST),'triangulos')

if __name__=='__main__':main()
