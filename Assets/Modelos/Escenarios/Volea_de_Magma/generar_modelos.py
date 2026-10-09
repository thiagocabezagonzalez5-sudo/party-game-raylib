"""Modelos originales GLB 2.0 para Volea de Magma, Python estándar."""
from geometria_glb import *
import geometria_glb as glb

glb.PALETTE.update({
 'obsidiana':'#2b2939','obsidiana_clara':'#4e485d','obsidiana_oscura':'#171923',
 'grieta':'#885147','lava':'#e95a25','lava_roja':'#b63429',
 'lava_clara':'#ff9837','lava_amarilla':'#ffce63','lava_brillo':'#fff1a6',
 'basalto':'#353340','basalto_claro':'#676474','ceniza':'#9e989b',
 'roca':'#5c4b47','roca_clara':'#9a6951',
 'hierro':'#24222d','hierro_claro':'#69636e',
 'cadena':'#b46840','cadena_clara':'#ffb65c',
 'naranja':'#f3923e','celeste':'#62bad7','ocre':'#d5aa72',
 'COLOR_DINAMICO':'#ffffff','BOMBILLAS':'#fff0b0',
})

def lago():
 m=mesh();box(m,(0,-1.7,-20),(120,.4,90),'lava_roja')
 # Vetado irregular visible sin interferir con el rectángulo jugable.
 for ix in range(-5,6):
  for iz in range(-5,4):
   x=ix*10+((ix*7+iz*11)%5)-2
   z=-20+iz*9+((ix*13-iz*3)%5)-2
   if abs(x)<12 and abs(z)<8:continue
   ellipsoid(m,(x,-1.485,z),(2.0,.02,1.15),'lava',10,4)
 for x,z in [(-16,7),(-11,-16),(18,-5),(27,4),(-30,-7),(38,-19),(-5,-27)]:
  torus(m,(x,-1.47,z),1.2,.045,'lava_clara',20,4)
 return m

def cancha():
 m=mesh()
 # Bloque de colisión 20x12, centro Y=-.5; juego efectivo 18x10.
 box(m,(0,-.5,0),(20,1,12),'obsidiana_oscura')
 box(m,(0,.011,0),(18,.022,10),'obsidiana')
 for ix in range(-8,9,2):
  for iz in range(-4,5,2):
   if (ix*3+iz*7)%4==0:
    box(m,(ix,.025,iz),(1.82,.012,1.83),'obsidiana_clara')
   if (ix*11+iz*3)%13==0:
    beam(m,(ix-.40,.032,iz-.24),(ix+.38,.033,iz+.27),.016,'grieta',5)
 for z in (-5,5):box(m,(0,.044,z),(18,.035,.14),'lava_clara')
 for x in (-9,9):box(m,(x,.044,0),(.14,.035,10),'lava_clara')
 box(m,(0,.051,0),(.14,.04,10),'lava_amarilla')
 for x,mat in [(-4.5,'naranja'),(4.5,'celeste')]:
  for z in (-3,0,3):
   torus(m,(x,.043,z),.53,.018,mat,20,4)
 return m

def borde():
 m=mesh()
 for z in (-5.90,5.90):
  box(m,(0,.16,z),(20,.30,.22),'basalto')
  box(m,(0,.31,z),(20,.035,.12),'lava_clara')
 for x in (-9.90,9.90):
  box(m,(x,.16,0),(.22,.30,12),'basalto')
  box(m,(x,.31,0),(.12,.035,12),'lava_clara')
 for x in (-9.8,9.8):
  for z in (-5.8,5.8):
   ellipsoid(m,(x,.37,z),(.28,.15,.28),'lava_amarilla',9,4)
 return m

def poste():
 m=mesh()
 cylinder(m,(0,0,0),.20,2.89,'hierro',8,r_top=.15)
 cylinder(m,(0,-.08,0),.30,.18,'basalto',8)
 for y in (.35,1.05,1.75,2.42):torus(m,(0,y,0),.18,.022,'cadena',12,4)
 ellipsoid(m,(0,3.00,0),(.22,.22,.22),'lava_clara',10,5)
 beam(m,(0,2.86,0),(0,3.21,0),.065,'lava_amarilla',7,r_end=.015)
 return m

def red():
 m=mesh()
 # Hoja fija, centrada en X=0, ancho Z=10.8 y dintel a Y=2.4.
 for fila in range(5):
  y=.9+fila*.375
  beam(m,(0,y,-5.4),(0,y,5.4),.048,'cadena',7)
  for z in range(-5,6,2):
   torus(m,(0,y,z),.12,.018,'cadena_clara',10,4,plane='yz')
 for k in range(13):
  z=-5.4+k*.9
  beam(m,(0,.9,z),(0,2.4,z),.035,'cadena',6)
 for z in (-5.4,5.4):
  torus(m,(0,2.4,z),.16,.034,'hierro_claro',12,5,plane='yz')
 box(m,(0,2.44,0),(.12,.09,10.8),'lava_amarilla')
 return m

def volcan(grande=False):
 m=mesh();r=20 if grande else 16;h=26 if grande else 20
 cylinder(m,(0,0,0),r,h,'roca',18,r_top=r*.12)
 cylinder(m,(0,h-.16,0),r*.125,.18,'obsidiana_oscura',18)
 cylinder(m,(0,h+.005,0),r*.094,.07,'lava_clara',18)
 torus(m,(0,h+.02,0),r*.108,.055,'lava_amarilla',18,5)
 for k in range(6):
  a=TAU*k/6+.33
  x,z=r*math.cos(a),r*math.sin(a)
  beam(m,(r*.11*math.cos(a),h-.1,r*.11*math.sin(a)),
       (x*.85,h*.20,z*.85),.30 if grande else .25,'lava',8,r_end=.48)
  beam(m,(x*.85,h*.20,z*.85),(x*.98,h*.09,z*.98),.24,'lava_amarilla',7,r_end=.10)
 for i in range(8):
  a=TAU*i/8
  ellipsoid(m,(r*.72*math.cos(a),h*.09,r*.72*math.sin(a)),
            (1.25,.35,1.0),'obsidiana',9,4)
 return m

def columna():
 m=mesh()
 # Altura base 4. Escalado Y independiente para conservar radio y ancho.
 cylinder(m,(0,0,0),1.18,4,'basalto',6)
 cylinder(m,(0,4,0),1.21,.16,'basalto_claro',6)
 for i in range(6):
  a=TAU*i/6
  beam(m,(1.16*math.cos(a),.40,1.16*math.sin(a)),
       (1.16*math.cos(a),3.75,1.16*math.sin(a)),.018,'obsidiana_oscura',5)
 return m

def columna_fuego():
 m=columna()
 ellipsoid(m,(0,4.36,0),(.27,.32,.27),'lava_clara',10,5)
 ellipsoid(m,(0,4.52,.04),(.12,.23,.13),'lava_amarilla',9,5)
 return m

def roca_magma(caliente=False):
 m=mesh();base='lava' if caliente else 'roca'
 ellipsoid(m,(0,0,0),(.45,.44,.45),base,14,8)
 for i in range(6):
  a=TAU*i/6
  ellipsoid(m,(.32*math.cos(a),.18*math.sin(a),.32*math.sin(a)),
            (.095,.058,.08),'lava_amarilla' if caliente else 'lava_roja',8,4)
 for y,r in [(-.14,.37),(.15,.38)]:
  torus(m,(0,y,0),r,.025,'lava_clara' if caliente else 'grieta',18,5)
 ellipsoid(m,(.18,.21,.27),(.12,.10,.07),'roca' if caliente else 'roca_clara',9,5)
 return m

def charco():
 m=mesh()
 cylinder(m,(0,.02,0),1.8,.035,'lava_roja',26)
 cylinder(m,(0,.057,0),1.12,.020,'lava',24)
 torus(m,(0,.07,0),1.67,.047,'lava_clara',26,5)
 for x,z in [(-.58,.43),(.69,-.33),(.15,.67)]:
  ellipsoid(m,(x,.09,z),(.16,.047,.13),'lava_amarilla',9,4)
 return m

def sombra():
 m=mesh();cylinder(m,(0,.015,0),.48,.012,'obsidiana_oscura',18)
 torus(m,(0,.03,0),.50,.015,'grieta',18,4)
 return m

def indicador():
 m=mesh()
 torus(m,(0,.075,0),.60,.04,'lava_amarilla',24,5)
 torus(m,(0,.075,0),.31,.027,'lava',20,4)
 for i in range(4):
  a=TAU*i/4
  ellipsoid(m,(.60*math.cos(a),.09,.60*math.sin(a)),(.06,.04,.06),'BOMBILLAS',8,4)
 return m

def burbuja():
 m=mesh();ellipsoid(m,(0,0,0),(.48,.43,.48),'lava_clara',12,6)
 torus(m,(0,.09,0),.41,.023,'lava_amarilla',16,4)
 ellipsoid(m,(-.13,.31,.15),(.10,.07,.07),'BOMBILLAS',8,4)
 return m

def ceniza():
 m=mesh();box(m,(0,0,0),(.09,.09,.09),'ceniza');return m

def estela():
 m=mesh();ellipsoid(m,(0,0,0),(.18,.18,.18),'lava_clara',10,5)
 ellipsoid(m,(0,.04,.03),(.07,.06,.07),'lava_amarilla',8,4)
 return m

def main():
 specs=[
  ('lago_lava',lago(),'centro en Y de la planicie','Mundo (0,0,0); extiende X±60, Z=-65..25, superficie Y=-1.5.'),
  ('cancha_obsidiana',cancha(),'centro del campo','Mundo (0,0,0); plataforma 20x12 y zona válida 18x10.'),
  ('borde_cancha',borde(),'centro del campo','Mundo (0,0,0); borde exterior a X±9.9, Z±5.9.'),
  ('poste_red',poste(),'pie al suelo','Instanciar en (0,0,-5.4) y (0,0,5.4).'),
  ('red_cadenas',red(),'centro del campo','Mundo (0,0,0); plano X=0, Z±5.4, altura principal 2.4.'),
  ('volcan_menor',volcan(),'pie del volcán','Mundo (-28,-1.5,-38); altura 20, radio de base 16.'),
  ('volcan_mayor',volcan(True),'pie del volcán','Mundo (26,-1.5,-44); altura 26, radio de base 20.'),
  ('columna_basalto',columna(),'pie de columna','Repetir sobre Y=-1.5, escalar solo Y=altura/4 (altura 2.5..7).'),
  ('columna_con_llama',columna_fuego(),'pie de columna','Repetir en X=±12.2, Z=±3.5; escalar solo Y=altura/4.'),
  ('roca_magma',roca_magma(),'centro de pelota','pelota.posicion; radio visual .45, temp<.85.'),
  ('roca_sobrecalentada',roca_magma(True),'centro de pelota','pelota.posicion; temp≥.85.'),
  ('charco_lava',charco(),'centro en suelo','(charco.x,0,charco.z); radio 1.8; dibujar solo si activo.'),
  ('sombra_pelota',sombra(),'centro sobre suelo','(pelota.x,0,pelota.z); escalar con altura si interesa.'),
  ('indicador_caida',indicador(),'centro sobre suelo','(prediccion.x,0,prediccion.z) cuando pelota en juego.'),
  ('burbuja_lava',burbuja(),'centro de burbuja','Fuera de la cancha, Y≈-1.45; animar seno.'),
  ('ceniza',ceniza(),'centro de partícula','Instanciar con posiciones animadas, sin colisión.'),
  ('estela_ascua',estela(),'centro de estela','Repetir sobre trayectoria de la roca, escalando tamaño.'),
 ]
 for name,model,pivot,usage in specs:save(name,model,pivot,usage)
 (glb.ROOT/'manifest.json').write_text(json.dumps({
  'version':1,'rama_referencia':'claude/expansion-party','commit_referencia':'ea5472c',
  'unidades':'1 unidad GLB = 1 unidad raylib','ejes':'Y arriba; red en X=0',
  'licencia':'Arte original procedural sin recursos externos',
  'nota':'Pelota, reglas de toques, puntos, charcos y colisiones siguen en C++.',
  'modelos':glb.MANIFEST},ensure_ascii=False,indent=2),encoding='utf8')
 print(len(glb.MANIFEST),'GLB;',sum(x['triangulos'] for x in glb.MANIFEST),'triangulos')

if __name__=='__main__':main()
