"""Modelos originales modulares del barco explorador. Python estándar."""
from geometria_glb import *
import geometria_glb as glb

glb.PALETTE.update({
 'madera':'#9a643e','madera_clara':'#c18b58','madera_oscura':'#613e31',
 'madera_dorada':'#ddab69','hierro':'#3a4853','hierro_luz':'#7c9298',
 'oro':'#edc464','crema':'#f1e6c8','crema_sombra':'#d1c19f',
 'cuerda':'#d6c190','cuerda_sombra':'#9a865f',
 'azul_mar':'#175a97','mar_claro':'#3584bc','espuma':'#d5f1ef',
 'arena':'#dec189','palma':'#6b4d36','verde':'#4d9b57',
 'verde_luz':'#71b777','cuervo':'#252a38','ala':'#343d4d',
 'pico':'#f2ad4c','ojo':'#e5685d','gaviota':'#f4f4e9',
 'rojo':'#eb6060','azul':'#4e99df','verde_jugador':'#65ce84','amarillo':'#f1d35c'
})

def casco():
 m=mesh()
 # Costados inclinados, proa levemente puntiaguda en +Z.
 top=[(-9,0,-2.3),(9,0,-2.3),(9,0,2.15),(0,0,3.05),(-9,0,2.15)]
 bot=[(-7.7,-3.15,-1.65),(7.7,-3.15,-1.65),(7.7,-3.15,1.45),(0,-3.15,2.1),(-7.7,-3.15,1.45)]
 for i in range(5):
  j=(i+1)%5;quad(m,top[i],bot[i],bot[j],top[j],'madera_oscura')
  # Franja brillante sobre la línea de flotación.
  a=add(top[i],(0,-.42,0));b=add(top[j],(0,-.42,0))
  c=add(bot[j],mul(sub(top[j],bot[j]),.75));d=add(bot[i],mul(sub(top[i],bot[i]),.75))
  quad(m,a,d,c,b,'madera')
  beam(m,top[i],top[j],.08,'madera_dorada',7)
 for y in (-.9,-2.0):
  for x in (-7,-4,0,4,7):
   ellipsoid(m,(x,y,2.18-.08*abs(x)),(.11,.10,.03),'oro',7,4)
 for z in (-1.85,1.85):
  for x in (-7.8,7.8):box(m,(x,-1.0,z),(.16,1.7,.12),'madera_clara')
 return m

def cubierta():
 m=mesh();box(m,(0,-.04,0),(18,.12,4.6),'madera_clara')
 for k in range(-4,5):
  box(m,(0,.03,k*.5),(17.9,.023,.035),'madera_oscura')
 for x in (-8.8,8.8):box(m,(x,.04,0),(.11,.05,4.6),'oro')
 for x in (-6.6,-2.2,2.2,6.6):
  torus(m,(x,.03,0),.18,.023,'hierro',12,4)
 return m

def barandilla():
 m=mesh()
 for x in range(-9,10):
  box(m,(float(x),.45,0),(.10,.9,.10),'madera')
  ellipsoid(m,(float(x),.94,0),(.09,.09,.09),'oro',7,4)
 box(m,(0,.87,0),(18.1,.12,.16),'madera_clara')
 box(m,(0,.28,0),(18,.08,.12),'madera_oscura')
 return m

def mastil():
 m=mesh();cylinder(m,(0,0,0),.28,11.6,'madera',12,r_top=.18)
 for y in (1,2,3,4,5,6,7,8,9):
  lado=-1 if y%2==0 else 1
  box(m,(lado*.28,y,.12),(.24,.08,.15),'madera_oscura')
  torus(m,(0,y+.15,0),.25,.025,'cuerda',12,4)
 box(m,(0,8.8,-.1),(3.05,.14,.14),'madera_oscura')
 cylinder(m,(0,11.55,0),.2,.32,'oro',12,r_top=0)
 return m

def vela():
 m=mesh()
 # Tejido ligeramente hinchado, frente visible hacia cámara (+Z).
 for row in range(8):
  ya=3.2+row*.7;yb=ya+.7
  for col in range(8):
   xa=-1.6+col*.4;xb=xa+.4
   def point(x,y):
    u=x/1.6;v=(y-6)/2.8
    return (x,y,-.75+.18*(1-u*u)*(1-v*v))
   a,b,c,d=point(xa,ya),point(xb,ya),point(xb,yb),point(xa,yb)
   quad(m,a,d,c,b,'crema' if (row+col)%4 else 'crema_sombra')
   quad(m,b,c,d,a,'crema' if (row+col)%4 else 'crema_sombra')
 for y in (3.2,8.8):beam(m,(-1.7,y,-.69),(1.7,y,-.69),.065,'madera',8)
 box(m,(0,6,-.52),(3.15,.43,.034),'crema_sombra')
 return m

def franja(mat):
 m=mesh();box(m,(0,6,-.487),(3.13,.47,.045),mat)
 return m

def cofa():
 m=mesh();cylinder(m,(0,10,0),.86,.22,'madera_clara',16)
 torus(m,(0,10.24,0),.87,.065,'oro',24,5)
 for k in range(12):
  a=k*TAU/12;x=.83*math.cos(a);z=.83*math.sin(a)
  beam(m,(x,10.18,z),(x,10.66,z),.043,'madera_oscura',6)
 torus(m,(0,10.67,0),.83,.04,'cuerda',24,5)
 return m

def bandera(mat):
 m=mesh();beam(m,(0,10.2,0),(0,11.7,0),.045,'madera_oscura',7)
 # Lámina ondulante de dos caras que nace en la cofa.
 for k in range(5):
  x0=.07+k*.25;x1=x0+.25
  for y0,y1 in ((10.95,11.4),):
   z0=.08*math.sin(k*.7);z1=.08*math.sin((k+1)*.7)
   quad(m,(x0,y0,z0),(x1,y0,z1),(x1,y1,z1),(x0,y1,z0),mat)
   quad(m,(x0,y1,z0),(x1,y1,z1),(x1,y0,z1),(x0,y0,z0),mat)
 star(m,(.59,11.18,.08),.11,'oro')
 return m

def jarcias():
 m=mesh()
 for x in (-1.9,1.9):
  for z in (-1.2,1.4):
   beam(m,(0,9.4,0),(x,.1,z),.027,'cuerda',6)
   ellipsoid(m,(x,.1,z),(.08,.06,.08),'hierro',8,4)
 for k in range(1,9):
  y=k*1.05
  half=.3+1.5*(1-y/9.45)
  beam(m,(-half,y,1.15),(half,y,1.15),.018,'cuerda_sombra',5)
 return m

def cofre():
 m=mesh();box(m,(0,.35,0),(1.2,.7,.8),'madera_oscura')
 box(m,(0,.78,0),(1.28,.18,.88),'madera_clara')
 for x in (-.5,.5):box(m,(x,.39,.405),(.09,.65,.02),'hierro')
 for z in (-.38,.38):box(m,(0,.82,z),(1.26,.05,.045),'oro')
 box(m,(0,.54,.455),(.18,.23,.045),'oro')
 ellipsoid(m,(0,.51,.49),(.046,.046,.016),'hierro',8,5)
 return m

def barril():
 m=mesh();cylinder(m,(0,0,0),.32,.85,'madera',12,r_top=.32)
 cylinder(m,(0,.18,0),.41,.47,'madera_clara',12,r_top=.41)
 for y in (.08,.72):torus(m,(0,y,0),.405,.044,'hierro',18,5)
 cylinder(m,(0,.85,0),.31,.04,'madera_oscura',12)
 return m

def cuervo():
 m=mesh();ellipsoid(m,(0,0,0),(.34,.25,.25),'cuervo',12,7)
 ellipsoid(m,(.28,.12,0),(.2,.17,.18),'cuervo',10,6)
 beam(m,(.39,.1,0),(.70,.02,0),.083,'pico',6,r_end=0)
 for z in (-.12,.12):ellipsoid(m,(.39,.17,z),(.038,.04,.02),'ojo',8,4)
 for z in (-.22,.22):
  beam(m,(-.07,.12,z),(-.5,.19,z*2.4),.14,'ala',8,r_end=.018)
  beam(m,(-.34,-.04,z),(-.69,-.15,z*1.5),.07,'ala',6,r_end=0)
 return m

def ala_cuervo():
 m=mesh()
 for sign in (-1,1):
  for k in range(5):
   z=sign*(.15+k*.1)
   beam(m,(0,0,0),(-.34,.25,z),.08,'ala',6,r_end=.014)
 return m

def gaviota():
 m=mesh();ellipsoid(m,(0,0,0),(.26,.13,.16),'gaviota',10,5)
 ellipsoid(m,(.22,.1,0),(.12,.1,.1),'gaviota',8,5)
 beam(m,(.3,.07,0),(.45,.04,0),.045,'pico',5,r_end=0)
 for s in (-1,1):
  beam(m,(-.03,0,s*.07),(-.46,.17,s*.84),.09,'gaviota',6,r_end=.012)
  beam(m,(-.42,.16,s*.79),(-.55,.3,s*1.02),.045,'crema_sombra',5,r_end=0)
 return m

def isla():
 m=mesh();cylinder(m,(0,-.05,0),8.5,.65,'arena',16,r_top=6.4)
 ellipsoid(m,(0,.45,0),(4.2,.6,2.8),'arena',14,7)
 cylinder(m,(-1,.5,0),.24,3.2,'palma',8,r_top=.17)
 ellipsoid(m,(-1,3.7,0),(.42,.38,.42),'verde',10,5)
 for k in range(6):
  a=k*TAU/6
  beam(m,(-1,3.7,0),(-1+2*math.cos(a),3.1,2*math.sin(a)),.13,'verde_luz',7,r_end=.02)
 return m

def mar():
 m=mesh();box(m,(0,-4.6,-20),(120,.2,90),'azul_mar')
 for k in range(52):
  x=-50+(k*37%101);z=-57+(k*29%81)
  ellipsoid(m,(x,-4.45,z),(1.5+(k%4)*.32,.035,.12),'mar_claro' if k%3 else 'espuma',8,4)
 return m

def espuma():
 m=mesh()
 for i in range(9):
  x=-1.8+i*.45
  ellipsoid(m,(x,0,.18*math.sin(i)),(.36,.045,.12),'espuma',8,4)
 return m

def ancla():
 m=mesh();beam(m,(0,0,0),(0,1.4,0),.10,'hierro',8)
 torus(m,(0,1.5,0),.16,.04,'hierro',14,5,plane='xy')
 beam(m,(-.65,.28,0),(.65,.28,0),.1,'hierro',8)
 for x in (-.65,.65):beam(m,(x,.28,0),(x,0,0),.08,'hierro',7,r_end=.04)
 return m

def timon():
 m=mesh();torus(m,(0,1,0),.6,.065,'madera_clara',20,5,plane='xy')
 for k in range(8):
  a=k*TAU/8;x=.78*math.cos(a);y=1+.78*math.sin(a)
  beam(m,(0,1,0),(x,y,0),.045,'madera',6)
  ellipsoid(m,(x,y,0),(.085,.085,.085),'oro',8,4)
 cylinder(m,(0,.96,0),.14,.08,'hierro',12)
 return m

def main():
 specs=[
  ('casco_barco',casco(),'centro en cubierta','Colocar origen del barco; casco de Y=0 a -3.15.'),
  ('cubierta',cubierta(),'centro en cubierta','Origen del barco, ancho 18, largo 4.6, superficie Y≈0.'),
  ('barandilla',barandilla(),'centro en base','En borde delantero Z=2.25; duplicar atrás con orientación.'),
  ('mastil',mastil(),'pie centro','Una instancia por jugador, X=posicionX[i], z=0; meta a Y=10.'),
  ('vela',vela(),'pie centro del mástil','Una por mástil; situada entre Y=3.2 y 8.8, z≈-.7.'),
  ('franja_roja',franja('rojo'),'pie centro del mástil','Superponer a vela para jugador rojo.'),
  ('franja_azul',franja('azul'),'pie centro del mástil','Superponer a vela para jugador azul.'),
  ('franja_verde',franja('verde_jugador'),'pie centro del mástil','Superponer a vela para jugador verde.'),
  ('franja_amarilla',franja('amarillo'),'pie centro del mástil','Superponer a vela para jugador amarillo.'),
  ('cofa',cofa(),'pie centro del mástil','Una por mástil, plato en Y=10.'),
  ('jarcias',jarcias(),'pie centro del mástil','Una por mástil, tensores a bordes de cubierta.'),
  ('bandera_roja',bandera('rojo'),'pie centro del mástil','Elegir bandera según color de jugador; asta Y=10.2..11.7.'),
  ('bandera_azul',bandera('azul'),'pie centro del mástil','Alternativa azul.'),
  ('bandera_verde',bandera('verde_jugador'),'pie centro del mástil','Alternativa verde.'),
  ('bandera_amarilla',bandera('amarillo'),'pie centro del mástil','Alternativa amarilla.'),
  ('cofre',cofre(),'pie centro','Dos en X≈±8.2, Z=.6.'),
  ('barril',barril(),'pie centro','Varios en los extremos de cubierta.'),
  ('cuervo',cuervo(),'centro del cuerpo','Dinámico: X=posicionX[i]+c.x, Y=c.altura, Z=.75.'),
  ('ala_cuervo',ala_cuervo(),'centro del cuerpo','Pieza opcional para animar aleteo sin mover el cuerpo.'),
  ('gaviota',gaviota(),'centro del cuerpo','Decoración móvil al fondo.'),
  ('isla_lejana',isla(),'centro en superficie del mar','Colocar (-22,-4.5,-42).'),
  ('mar',mar(),'centro global','Origen del mundo; agua a Y≈-4.5, no rota con barco.'),
  ('espuma_ola',espuma(),'centro superficial','Instancias de espuma móviles alrededor del barco.'),
  ('ancla',ancla(),'extremo inferior','Decoración de costados, fuera de rutas de escalada.'),
  ('timon',timon(),'pie centro','Decoración en popa detrás de los mástiles.'),
 ]
 for name,m,pivot,usage in specs:save(name,m,pivot,usage)
 (ROOT/'manifest.json').write_text(json.dumps({'minijuego':'Trepa el Mástil','base':'claude/expansion-party 27f2519','coordenadas':'Y arriba; cubierta y=0; separación mástiles 4.4; cofa y=10','modelos':MANIFEST},ensure_ascii=False,indent=2)+'\n')
 print(len(specs),'GLB;',sum(x['triangulos'] for x in MANIFEST),'triángulos')

if __name__=='__main__':main()
