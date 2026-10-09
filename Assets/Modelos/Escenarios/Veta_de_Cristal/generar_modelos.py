"""Modelos originales GLB 2.0 para Veta de Cristal."""
from geometria_glb import *
import geometria_glb as glb

glb.PALETTE.update({
 'roca':'#514641','roca_clara':'#72645b','roca_oscura':'#342e30',
 'suelo':'#64534a','arena':'#8e7562','fisura':'#2a2629',
 'madera':'#9c6a43','madera_clara':'#bc8551','madera_oscura':'#593b31',
 'hierro':'#9ba7ad','hierro_oscuro':'#404d5c','oxido':'#a85839',
 'azul':'#55c4ee','azul_claro':'#a0edff','azul_oscuro':'#246e98',
 'violeta':'#ad73cf','violeta_claro':'#e2afff','violeta_oscuro':'#654d78',
 'oro':'#f1c251','oro_claro':'#ffe393','oro_oscuro':'#a67b36',
 'COLOR_DINAMICO':'#ffffff','BOMBILLAS':'#fff0b9',
 'negro':'#171b25','rojo':'#ce5146'
})

def cristal(m,x,y,z,radio,alto,color,n=6):
 # Prisma facetado y punta; plano de asiento en y.
 for i in range(n):
  a=i*TAU/n;b=(i+1)*TAU/n
  pa=(x+radio*math.cos(a),y,z+radio*math.sin(a))
  pb=(x+radio*math.cos(b),y,z+radio*math.sin(b))
  qa=(pa[0],y+alto*.68,pa[2]);qb=(pb[0],y+alto*.68,pb[2])
  quad(m,pa,pb,qb,qa,color)
  tri(m,qa,qb,(x,y+alto,z),color)
  tri(m,(x,y,z),pb,pa,color)

def suelo():
 m=mesh();box(m,(0,-.51,0),(24,1.02,16),'suelo')
 for x in range(-11,12,2):
  for z in range(-7,8,2):
   a=((x*5+z*11)%7)/7
   box(m,(x+a*.3,.016,z-a*.2),(1.0+a*.5,.023,.85+a*.25),'roca' if (x+z)%3 else 'roca_clara')
   if (x*2+z*3)%5==0:
    beam(m,(x-.4,.032,z-.28),(x+.1,.033,z+.17),.008,'fisura',5)
 for side,color in [(-1,'rojo'),(1,'azul')]:
  x=side*6.6
  box(m,(x,.036,0),(.055,.012,15.5),color)
  for z in range(-7,8,2):
   box(m,(x,.043,z),(.26,.025,.14),'hierro')
 return m

def paredes():
 m=mesh()
 box(m,(0,2.8,-8.6),(26,5.6,1.2),'roca')
 for x in [-12.6,12.6]:
  box(m,(x,2.2,0),(1.2,4.4,17),'roca')
 box(m,(0,.25,8.5),(26,.5,.6),'roca_oscura')
 for y in [.6,1.65,2.7,3.75,4.8]:
  box(m,(0,y,-7.985),(25.4,.06,.03),'roca_oscura')
  for x in [-12.1,12.1]:
   box(m,(x,y,0),(.03,.06,16.4),'roca_oscura')
 for x in range(-12,13,3):
  for y in [1.1,3.1]:
   box(m,(x,y,-7.975),(2.2,.18,.035),'roca_clara')
 for z in range(-7,8,3):
  for y in [.9,2.8]:
   for x in [-11.98,11.98]:
    box(m,(x,y,z),(.035,.12,2.0),'roca_clara')
 for x in [-12.8,12.8]:
  beam(m,(x,0,-8.5),(x,4.5,-8.5),.075,'hierro_oscuro',6)
 return m

def veta():
 m=mesh()
 # Grupo de cristales para repetir en muros, frontal hacia +Z.
 box(m,(0,0,-.09),(.86,.72,.12),'roca_clara')
 for x,y,radio,h,color in [(-.24,-.17,.13,.43,'azul'),(.02,-.21,.15,.55,'violeta'),
                           (.25,-.12,.105,.33,'oro'),(-.03,.22,.10,.33,'azul_claro')]:
  cristal(m,x,y-.25,.10,radio,h,color,5)
 for x,y in [(-.38,.24),(.34,-.29)]:
  ellipsoid(m,(x,y,.025),(.055,.055,.025),'BOMBILLAS',8,4)
 return m

def portico():
 m=mesh()
 for x in [-9,-3,3,9]:
  box(m,(x,2.4,-7.8),(.50,4.8,.50),'madera')
  for y in [1.35,2.65,3.95]:
   box(m,(x,y,-7.5),(.54,.08,.06),'madera_clara')
  box(m,(x,.16,-7.8),(.74,.32,.66),'hierro_oscuro')
  beam(m,(x-1,4.65,-7.8),(x,3.55,-7.8),.08,'madera_oscura',8)
  beam(m,(x+1,4.65,-7.8),(x,3.55,-7.8),.08,'madera_oscura',8)
 box(m,(0,4.8,-7.8),(19,.50,.50),'madera_oscura')
 for x in range(-9,10,3):
  box(m,(x,4.78,-7.51),(.13,.22,.055),'hierro')
  ellipsoid(m,(x,4.77,-7.46),(.033,.034,.033),'oro',8,4)
 return m

def poste():
 m=mesh()
 box(m,(0,2,0),(.40,4,.40),'madera')
 box(m,(0,4.10,0),(.50,.30,1.4),'madera_oscura')
 for y in [.2,2.3,3.8]:
  box(m,(0,y,0),(.48,.065,.48),'hierro_oscuro')
 for z in [-.50,.50]:
  beam(m,(0,4.02,z),(0,3.16,z),.038,'hierro',6)
 box(m,(0,.13,0),(.66,.26,.66),'roca_oscura')
 return m

def lampara():
 m=mesh()
 # Centro Y=3.7 de la lampara actual.
 cylinder(m,(0,-.27,0),.32,.08,'hierro_oscuro',12)
 ellipsoid(m,(0,0,0),(.25,.26,.25),'BOMBILLAS',12,7)
 for x in [-.21,.21]:
  for z in [-.21,.21]:
   beam(m,(x,-.18,z),(x,.22,z),.025,'hierro',5)
 cylinder(m,(0,.20,0),.30,.10,'madera_oscura',12,r_top=.15)
 torus(m,(0,-.21,0),.26,.026,'oro',20,5)
 beam(m,(0,.27,0),(0,.56,0),.035,'hierro',8)
 return m

def riel():
 m=mesh()
 box(m,(0,.01,0),(2.38,.03,16),'roca_oscura')
 for z in range(-7,8):
  box(m,(0,.072,z),(1.90,.09,.28),'madera_oscura')
  for x in [-.62,.62]:
   box(m,(x,.13,z),(.14,.016,.08),'hierro_oscuro')
 for x in [-.55,.55]:
  box(m,(x,.16,0),(.115,.115,16),'hierro')
  box(m,(x,.215,0),(.19,.024,16),'hierro_oscuro')
 for z in [-7.8,7.8]:
  box(m,(0,.16,z),(2.28,.14,.07),'hierro_oscuro')
 return m

def vagoneta():
 m=mesh()
 # Pivote a suelo Y=0, largo exterior 2.2 y ancho 1.84.
 box(m,(0,.42,0),(1.77,.17,2.14),'hierro_oscuro')
 box(m,(0,.54,0),(1.59,.09,1.96),'madera_oscura')
 for x in [-.78,.78]:
  box(m,(x,.87,0),(.12,.71,2.12),'madera')
  for z in [-.75,-.25,.25,.75]:
   box(m,(x*1.052,.83,z),(.045,.45,.055),'hierro_oscuro')
 for z in [-1.01,1.01]:
  box(m,(0,.87,z),(1.68,.71,.12),'madera')
  box(m,(0,1.25,z),(1.79,.10,.15),'hierro')
 for x in [-.79,.79]:
  for z in [-.73,.73]:
   ellipsoid(m,(x,.25,z),(.13,.23,.23),'negro',12,7)
   ellipsoid(m,(x*1.035,.25,z),(.026,.08,.08),'hierro',10,5)
 for x,z,c in [(-.45,-.44,'azul'),(.21,-.34,'oro'),(.39,.30,'violeta'),(-.18,.40,'azul_claro')]:
  cristal(m,x,.66,z,.15,.47,c,6)
 for z in [-1.06,1.06]:
  ellipsoid(m,(0,.96,z),(.15,.15,.035),'BOMBILLAS',10,5)
 return m

def geoda(size,big=False,empty=False):
 m=mesh()
 # Mitad 0.65 o 0.90; superficies dentro de cubo de colision.
 extent=size*.96
 box(m,(0,-.15*size,0),(extent*2,(1.62 if empty else 1.42)*size,extent*2),'roca_oscura' if empty else 'violeta')
 for x in [-1,1]:
  for z in [-1,1]:
   box(m,(x*size*.87,0,z*size*.87),(.07,size*1.78,.07),'hierro_oscuro')
 if empty:
  box(m,(0,0,extent+.005),(size*1.39,size*1.35,.012),'roca')
  for x in [-.34,.34]:
   beam(m,(x*size,-size*.35,extent+.015),(x*size*.40,size*.28,extent+.016),.018,'fisura',6)
  return m
 for x,y,w,h,col in [
  (-.43,-.15,.22,.60,'oro' if big else 'azul'),
  (-.12,-.18,.24,.73,'oro_claro' if big else 'azul_claro'),
  (.25,-.10,.20,.58,'oro' if big else 'azul'),
  (.46,.12,.13,.38,'violeta_claro')]:
  # Rombos facetados en la cara frontal; profundidad dentro del bloque logico.
  z=extent+.004; cx=x*size;cy=y*size;w*=size;h*=size
  top=(cx,cy+h/2,z);bottom=(cx,cy-h/2,z)
  left=(cx-w/2,cy,z);right=(cx+w/2,cy,z)
  mid=(cx,cy,z+.004)
  for a,b in [(top,right),(right,bottom),(bottom,left),(left,top)]:tri(m,mid,b,a,col)
  beam(m,top,bottom,.008*size,'oro_claro' if big else 'azul_claro',5)
 # Coronas minerales sobre la cara superior, visibles desde arriba.
 for x,z,rr,hh,col in [(-.38,-.20,.14,.34,'oro' if big else 'azul'),
                        (.10,.15,.17,.50,'oro_claro' if big else 'azul_claro'),
                        (.41,-.22,.11,.27,'violeta_claro')]:
  cristal(m,x*size,.53*size,z*size,rr*size,hh*size*.74,col,5)
 return m

def gema(color,radio):
 m=mesh()
 # Centro del coleccionable en el origen.
 top=(0,radio,0);bottom=(0,-radio,0)
 n=8
 for i in range(n):
  a=i*TAU/n;b=(i+1)*TAU/n
  p=(radio*math.cos(a),radio*.08,radio*math.sin(a))
  q=(radio*math.cos(b),radio*.08,radio*math.sin(b))
  tri(m,top,p,q,color);tri(m,bottom,q,p,color)
 torus(m,(0,radio*.08,0),radio*.94,radio*.025,'oro_claro' if color=='oro' else 'azul_claro',n,5)
 return m

def marca():
 m=mesh();torus(m,(0,.02,0),.55,.035,'COLOR_DINAMICO',28,5)
 for i in range(8):
  a=i*TAU/8
  beam(m,(.46*math.cos(a),.025,.46*math.sin(a)),(.56*math.cos(a),.025,.56*math.sin(a)),.016,'COLOR_DINAMICO',5)
 return m

def main():
 specs=[
 ('suelo_mina',suelo(),'centro de arena, suelo Y=0','Mundo (0,0,0); 24x16; limite logico jugadores X+-10.6 Z+-7.2.'),
 ('paredes_tunel',paredes(),'centro del tunel','Mundo (0,0,0); fondo Z=-8.6, laterales X+-12.6.'),
 ('veta_pared',veta(),'centro de grupo de minerales','Repetir en fondo y laterales, adaptar rotacion en Y. Solo visual.'),
 ('portico_madera',portico(),'centro de mina','Mundo (0,0,0), postes X+-9,+-3 y traviesa Y=4.8.'),
 ('poste_lateral',poste(),'pie de poste','Mundo (X+-11.7,0,Z=-5/0/5).'),
 ('lampara_colgante',lampara(),'centro de luz','Mundo (X+-11.0,3.7,Z=-5/0/5). Sin iluminacion real.'),
 ('riel_central',riel(),'centro de riel a suelo','Mundo (0,0,0), direccion Z, longitud 16.'),
 ('vagoneta',vagoneta(),'centro a suelo de vagoneta','Mundo (0,0,vagonetaZ); visible solo si vagonetaActiva.'),
 ('geoda_pequena',geoda(.65),'centro de bloque logico','8 posiciones X+-3.8/+-7.6 y Z+-3.6/+3.6, centro Y=2.85.'),
 ('geoda_grande',geoda(.90,True),'centro de bloque logico','2 posiciones X+-6, Z=0, centro Y=3.10.'),
 ('geoda_agotada',geoda(.65,empty=True),'centro de bloque logico','Para pequena descargada; escalar 0.9/0.65 para grande descargada.'),
 ('gema_azul',gema('azul',.20),'centro del coleccionable','Seguir GemaVeta (x,y,z), valor 1.'),
 ('gema_dorada',gema('oro',.28),'centro del coleccionable','Seguir GemaVeta (x,y,z), valor 3.'),
 ('gema_violeta',gema('violeta',.38),'centro del coleccionable','Seguir GemaVeta (x,y,z), valor 4 o 6.'),
 ('marca_geoda',marca(),'centro de aro al suelo','Mundo (geoda.x,.04,geoda.z), tintar segun tipo.'),
 ]
 for name,model,pivot,usage in specs:save(name,model,pivot,usage)
 (glb.ROOT/'manifest.json').write_text(json.dumps({
  'version':1,'rama_referencia':'claude/expansion-party','commit_referencia':'ea5472c',
  'unidades':'1 unidad GLB = 1 unidad raylib','ejes':'Y arriba, riel orientado en Z',
  'licencia':'Arte original procedural sin recursos externos',
  'nota':'Las mallas no alteran los bloques logicos, golpes, puntuacion ni fisica.',
  'modelos':glb.MANIFEST},ensure_ascii=False,indent=2),encoding='utf8')
 print(len(glb.MANIFEST),'GLB;',sum(x['triangulos'] for x in glb.MANIFEST),'triangulos')

if __name__=='__main__':main()
