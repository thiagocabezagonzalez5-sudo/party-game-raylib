"""Modelos GLB originales y modulares para Autos de Globo. Python estándar."""
from geometria_glb import *
import geometria_glb as glb

glb.PALETTE.update({
 'cielo':'#9dd8ec','plaza':'#283455','plaza_luz':'#394770','plaza_sombra':'#1a253e',
 'metal':'#697a9e','metal_luz':'#b7c8df','metal_oscuro':'#343d61',
 'cian':'#67dff7','cian_luz':'#b9f6ff','rosa':'#f369c5','rosa_luz':'#ffb9df',
 'verde':'#70efad','verde_oscuro':'#3ba981','naranja':'#ffb05a',
 'ventana':'#ffeaa8','vidrio':'#517bae','vidrio_oscuro':'#34527e',
 'asfalto':'#414b6c','jardin':'#438c6c','jardin_luz':'#65bc77',
 'blanco':'#f4f9ff','rojo':'#eb6472','negro':'#232b40',
 'COLOR_DINAMICO':'#ffffff','BOMBILLAS':'#ffffff'})

def plaza():
 m=mesh();box(m,(0,-1,0),(19.9,2,14.5),'plaza_sombra')
 box(m,(0,-.05,0),(18.3,.10,12.9),'plaza')
 for x in range(-8,9,2):box(m,(x,.008,0),(.04,.012,12.86),'cian')
 for z in range(-6,7,2):box(m,(0,.009,z),(18.26,.012,.04),'cian')
 for x in (-9.15,9.15):box(m,(x,-.13,0),(.13,.16,12.9),'metal')
 for z in (-6.45,6.45):box(m,(0,-.13,z),(18.3,.16,.13),'metal')
 return m

def center_ring():
 m=mesh();torus(m,(0,.025,0),1.6,.04,'cian',32,5)
 torus(m,(0,.03,0),2.6,.035,'rosa',36,5)
 for i in range(12):
  a=TAU*i/12
  ellipsoid(m,(2.6*math.cos(a),.055,2.6*math.sin(a)),(.055,.023,.055),'cian_luz',7,4)
 return m

def barrier(length):
 m=mesh();box(m,(0,.24,0),(length,.48,.3),'metal_oscuro')
 box(m,(0,.52,0),(length,.06,.11),'rosa')
 for i in range(int(length/1.5)+1):
  x=-length/2+i*length/int(length/1.5)
  box(m,(x,.7,0),(.08,.36,.09),'metal')
  ellipsoid(m,(x,.91,0),(.09,.08,.09),'rosa_luz',8,4)
 for i in range(int(length/2.0)):
  x=-length/2+1+i*2
  box(m,(x,.24,.16),(.45,.09,.018),'cian')
 return m

def building(w,h,shade):
 m=mesh();box(m,(0,h/2,0),(w,h,w),shade)
 box(m,(0,h+.08,0),(w+.2,.16,w+.2),'metal_luz')
 for y in range(1,int(h)-1,2):
  for z in (-w/2-.018,w/2+.018):
   box(m,(0,y,z),(w*.77,.12,.035),'ventana' if y%4==1 else 'cian')
  for x in (-w/2-.018,w/2+.018):box(m,(x,y,0),(.035,.12,w*.77),'vidrio')
 for x in (-w/2,w/2):
  for z in (-w/2,w/2):box(m,(x,h/2,z),(.07,h,.07),'metal_luz')
 for z in (-w/2+.4,w/2-.4):box(m,(0,h-.1,z),(w-.45,.08,.27),'jardin')
 return m

def green_wall():
 m=mesh();box(m,(0,2,0),(1.35,4,.18),'jardin')
 for i in range(16):
  x=-.58+(i*7%12)/10;y=.3+(i*11%34)/10
  ellipsoid(m,(x,y,.14),(.17,.13,.075),'jardin_luz' if i%3 else 'verde_oscuro',8,4)
 return m

def highway(length,w):
 m=mesh();box(m,(0,-.40,0),(length,.3,w),'asfalto')
 for x in (-length/2+.2,length/2-.2):box(m,(x,-.22,0),(.10,.08,w),'metal')
 if length>w:
  for x in range(-int(length/2)+2,int(length/2),3):box(m,(x,-.218,0),(1.0,.025,.07),'ventana')
 else:
  for z in range(-int(w/2)+2,int(w/2),3):box(m,(0,-.218,z),(.07,.025,1.0),'ventana')
 return m

def traffic():
 m=mesh();box(m,(0,.12,0),(.80,.20,.4),'COLOR_DINAMICO')
 box(m,(0,.24,0),(.38,.07,.31),'vidrio_oscuro')
 for x in (-.22,.22):
  for z in (-.22,.22):cylinder(m,(x,0,z),.095,.07,'cian',8)
 return m

def holo_base():
 m=mesh();cylinder(m,(0,0,0),.43,.45,'metal_oscuro',10,r_top=.30)
 torus(m,(0,.44,0),.31,.045,'cian',16,4)
 beam(m,(0,.46,0),(0,2.35,0),.04,'cian_luz',7)
 return m

def holo_rings():
 m=mesh()
 torus(m,(0,0,0),.8,.038,'COLOR_DINAMICO',24,5,plane='xz')
 torus(m,(0,0,0),.8,.038,'COLOR_DINAMICO',24,5,plane='xy')
 torus(m,(0,0,0),.8,.038,'cian_luz',24,5,plane='yz')
 ellipsoid(m,(0,0,0),(.13,.13,.13),'BOMBILLAS',10,5)
 return m

def drone():
 m=mesh();ellipsoid(m,(0,0,0),(.25,.17,.25),'metal_luz',10,6)
 for i in range(4):
  a=TAU*i/4;x=.38*math.cos(a);z=.38*math.sin(a)
  beam(m,(0,.04,0),(x,.1,z),.055,'metal',6)
  cylinder(m,(x,.1,z),.13,.06,'metal_oscuro',10)
 ellipsoid(m,(0,-.17,0),(.07,.07,.07),'rojo',8,4)
 return m

def rotor():
 m=mesh();box(m,(0,0,0),(.75,.025,.09),'metal_luz')
 box(m,(0,.018,0),(.09,.025,.75),'metal_luz')
 cylinder(m,(0,-.04,0),.07,.07,'cian',8)
 return m

def plate(warning):
 m=mesh();cylinder(m,(0,.01,0),1.1,.065,'naranja' if warning else 'verde',24)
 torus(m,(0,.085,0),1.1,.046,'naranja' if warning else 'verde',28,5)
 torus(m,(0,.092,0),.65,.032,'ventana' if warning else 'cian_luz',24,4)
 for i in range(6):
  a=TAU*i/6;ellipsoid(m,(.85*math.cos(a),.09,.85*math.sin(a)),(.055,.026,.055),'BOMBILLAS',7,4)
 return m

def recharge_orb():
 m=mesh();ellipsoid(m,(0,0,0),(.25,.25,.25),'verde',13,7)
 torus(m,(0,0,0),.34,.027,'cian_luz',20,4,plane='xy')
 ellipsoid(m,(-.08,.09,.18),(.075,.06,.035),'BOMBILLAS',8,4)
 return m

def car():
 m=mesh();box(m,(0,.3,0),(2,.3,1.2),'COLOR_DINAMICO')
 box(m,(.8,.27,0),(.6,.20,1),'metal_luz')
 box(m,(1.10,.3,0),(.08,.22,1.10),'cian')
 box(m,(-.1,.55,0),(.8,.22,.9),'vidrio')
 box(m,(-1.03,.32,0),(.06,.16,1),'rojo')
 for z in (-.5,.5):box(m,(-.9,.6,z),(.3,.22,.07),'COLOR_DINAMICO')
 for x in (-.65,.65):
  for z in (-.5,.5):
   cylinder(m,(x,.025,z),.17,.10,'cian',10)
   torus(m,(x,.13,z),.17,.023,'cian_luz',14,4)
 box(m,(-.86,.58,0),(.14,.16,.85),'metal_oscuro')
 return m

def balloon():
 m=mesh();ellipsoid(m,(0,0,0),(.34,.38,.34),'COLOR_DINAMICO',16,9)
 torus(m,(0,-.08,0),.33,.028,'cian_luz',22,4)
 ellipsoid(m,(-.11,.12,.26),(.07,.09,.035),'BOMBILLAS',8,4)
 cylinder(m,(0,-.43,0),.07,.08,'metal',8,r_top=.045)
 return m

def tether():
 m=mesh();beam(m,(0,0,0),(.2,1.0,0),.015,'metal_luz',6)
 return m

def turbo():
 m=mesh();beam(m,(0,0,0),(-1.15,0,0),.26,'naranja',8,r_end=0)
 beam(m,(0,0,0),(-.72,0,0),.15,'ventana',8,r_end=0)
 return m

def halo():
 m=mesh();torus(m,(0,.035,0),.92,.035,'cian',28,5)
 return m

def main():
 specs=[
 ('plaza_elevada',plaza(),'origen global','Arena 18.3×12.9, límites de centros X=±8.3, Z=±5.6.'),
 ('anillo_central',center_ring(),'centro','Colocar en origen sobre suelo.'),
 ('barrera_frontal',barrier(18.7),'centro al pie','Dos copias en Z=±6.45.'),
 ('barrera_lateral',barrier(12.9),'centro al pie','Dos copias en X=±9.15 con giro Y=90°.'),
 ('rascacielos_bajo',building(2.8,10,'vidrio'),'pie central','Instanciar fuera de plaza; Y=-6.'),
 ('rascacielos_medio',building(3.4,16,'vidrio_oscuro'),'pie central','Instanciar fuera de plaza; Y=-6.'),
 ('rascacielos_alto',building(4.1,22,'vidrio'),'pie central','Instanciar fuera de plaza; Y=-6.'),
 ('jardin_vertical',green_wall(),'pie central','Colocar en fachadas de edificios; orientar hacia la cámara.'),
 ('autopista_trasera',highway(50,2.2),'centro de tramo','Centro (0,0,-9.6), detrás de plaza.'),
 ('autopista_lateral',highway(2,40),'centro de tramo','Centro (12.4,0,0), fuera de plaza.'),
 ('vehiculo_trafico',traffic(),'centro','Decoración animable en autopistas; teñir COLOR_DINAMICO.'),
 ('holograma_pedestal',holo_base(),'pie central','Centros (±10.6,0,±7.6).'),
 ('holograma_anillos',holo_rings(),'centro','Animar rotación Y en Y≈2.4 sobre pedestal; tintar COLOR_DINAMICO.'),
 ('dron_cuerpo',drone(),'centro','Cuatro drones en órbita; posición Y≈3.5..5.3.'),
 ('dron_helice',rotor(),'centro','Cuatro instancias por dron, girar Y individualmente.'),
 ('placa_aviso',plate(True),'centro al pie','Radio lógico 1.1; visible durante aviso.'),
 ('placa_recarga',plate(False),'centro al pie','Radio lógico 1.1; visible durante fase activa.'),
 ('orbe_recarga',recharge_orb(),'centro','Flotar en Y≈.9 sobre placa activa.'),
 ('auto_flotante',car(),'centro de auto','Local +X es frente; trasladar a (a.x,0,a.z), rotar −a.angulo; teñir COLOR_DINAMICO.'),
 ('globo_energia',balloon(),'centro de globo','Instanciar 0..3 por auto según a.globos, en altura≈1.9..2.1; teñir COLOR_DINAMICO.'),
 ('cuerda_globo',tether(),'anclaje inferior','Opcional, trasladar/rotar/escalar entre amarre y globo; el cable también puede ser línea dinámica.'),
 ('llama_turbo',turbo(),'base de escape','Solo si turboActivo>0, en X local=-1.05,Y=.35.'),
 ('halo_auto',halo(),'centro sobre suelo','A nivel del suelo debajo de cada auto, radio .92.'),
 ]
 for name,m,pivot,usage in specs:save(name,m,pivot,usage)
 (ROOT/'manifest.json').write_text(json.dumps({'minijuego':'Autos de Globo','base':'claude/expansion-party f290cb2','coordenadas':'Y arriba; frente del auto +X; unidades raylib','modelos':MANIFEST},ensure_ascii=False,indent=2))
 print(len(specs),'GLB;',sum(v['triangulos'] for v in MANIFEST),'triángulos')

if __name__=='__main__':main()
