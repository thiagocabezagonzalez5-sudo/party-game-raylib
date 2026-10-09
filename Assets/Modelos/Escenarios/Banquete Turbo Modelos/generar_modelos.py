"""Arte orbital original, GLB 2.0 modular. Python 3 sin dependencias."""
from geometria_glb import *
import geometria_glb as glb

glb.PALETTE.update({
    'suelo':'#263047', 'suelo_claro':'#39465f', 'pared':'#323c59',
    'panel':'#4c6082', 'metal':'#9baec4', 'metal_luz':'#dce8ee',
    'metal_oscuro':'#2d3951', 'cian':'#55d8ee', 'cian_sombra':'#278baa',
    'pantalla':'#122436', 'azul_tierra':'#2468b6', 'mar_tierra':'#3e92cf',
    'continente':'#72ad73', 'nube':'#e8f1ee', 'espacio':'#0d1630',
    'blanco':'#eaf7ff', 'rojo':'#e3535c', 'naranja':'#ffae5b',
    'oro':'#ffcf65', 'verde':'#6ed7aa', 'violeta':'#b587d7',
    'comida':'#89c9d9', 'COLOR_JUGADOR':'#ffffff', 'EMISION':'#8beeff'
})

def comedor():
    m=mesh()
    box(m,(0,-.25,-1),(40,.5,22),'suelo')
    for i in range(-6,7):
        box(m,(i*3,.012,-1),(.045,.017,21.4),'cian_sombra')
    for j in range(-3,4):
        box(m,(0,.012,j*3-1),(39,.017,.045),'panel')
    for z in (-11,9):
        box(m,(0,.45,z),(40,.9,.36),'pared')
        box(m,(0,.89,z),(39.7,.05,.16),'cian')
    for x in (-19.9,19.9):
        box(m,(x,.9,-1),(.28,1.8,22),'metal_oscuro')
    return m

def ventanal():
    m=mesh()
    for y,h in ((.75,1.5),(10.2,3.4)):
        box(m,(0,y,0),(34,h,.6),'pared')
    for x in (-13.5,13.5):box(m,(x,6,0),(7,9,.6),'pared')
    box(m,(0,1.54,.32),(20,.1,.08),'cian')
    box(m,(0,8.48,.32),(20,.1,.08),'cian_sombra')
    for x in (-9,-4.5,0,4.5,9):
        box(m,(x,6,.32),(.25,9,.29),'metal_oscuro')
        for y in (2.1,8):ellipsoid(m,(x,y,.52),(.11,.11,.06),'metal_luz',9,5)
    for x in (-10.1,10.1):
        box(m,(x,5,.35),(.1,7,.12),'metal')
    return m

def earth():
    m=mesh()
    ellipsoid(m,(0,0,0),(10,10,10),'azul_tierra',28,16)
    # Continentes estilizados: manchas curvadas sobre la cara visible +Z.
    for x,y,r in [(-4,3,2.1),(-2,1,2.5),(-1,-3,1.5),
                  (3,4,2.2),(5,1,1.6),(4,-3,2.0)]:
        z=math.sqrt(max(1,100-x*x-y*y))
        ellipsoid(m,(x,y,z+.05),(r,r*.75,.22),'continente',12,7)
    for x,y,r in [(-6,5,1.2),(-4,-2,1.2),(1,5,1.0),(4,0,1.0)]:
        z=math.sqrt(max(1,100-x*x-y*y))
        ellipsoid(m,(x,y,z+.26),(r,.18,r*.37),'nube',12,5)
    torus(m,(0,0,0),10.35,.12,'cian_sombra',44,6,plane='xy')
    return m

def sky():
    m=mesh()
    # Placa opaca: no ocupa el hueco del ventanal, va detrás de la Tierra.
    box(m,(0,0,-.04),(90,42,.08),'espacio')
    for k in range(90):
        x=-43+(k*157%860)/10; y=-19+(k*89%390)/10
        r=.045+(k%5)*.017
        ellipsoid(m,(x,y,.07),(r,r,.035),'blanco' if k%6 else 'cian',6,4)
    return m

def techo():
    m=mesh();box(m,(0,0,0),(40,.35,20),'pared')
    for x in (-15,-9,-3,3,9,15):
        box(m,(x,-.23,0),(.12,.08,18),'metal_oscuro')
    for z in (-7,0,7):box(m,(0,-.24,z),(38,.04,.08),'cian_sombra')
    return m

def tuberia():
    m=mesh()
    for i,z in enumerate((-.8,0,.8)):
        beam(m,(-18,0,z),(18,0,z),.18+(i%2)*.05,'metal',10)
        for x in (-13,-5,3,11):
            torus(m,(x,0,z),.22,.037,'metal_oscuro',16,5,plane='yz')
    beam(m,(-18,-.42,1.58),(18,-.42,1.58),.055,'cian',8)
    return m

def lampara():
    m=mesh()
    cylinder(m,(0,-1,0),.03,1,'metal',8)
    cylinder(m,(0,-1.2,0),.56,.25,'metal_oscuro',12,r_top=.18)
    ellipsoid(m,(0,-1.21,0),(.36,.10,.36),'EMISION',12,6)
    torus(m,(0,-1.07,0),.48,.055,'cian',20,5)
    return m

def mesa():
    m=mesh()
    cylinder(m,(0,0,0),.35,.58,'metal_oscuro',12,r_top=.5)
    cylinder(m,(0,.58,0),.10,.23,'cian_sombra',12)
    box(m,(0,.86,0),(3.2,.12,1.6),'panel')
    box(m,(0,.94,-.77),(3.1,.03,.05),'cian')
    box(m,(0,.94,.77),(3.1,.03,.05),'COLOR_JUGADOR')
    for x in (-1.55,1.55):box(m,(x,.94,0),(.04,.03,1.5),'cian')
    for x in (-1.32,1.32):
        for z in (-.62,.62):ellipsoid(m,(x,.93,z),(.055,.02,.055),'metal_luz',8,4)
    return m

def bandeja():
    m=mesh();box(m,(0,0,0),(2.6,.055,1),'metal_luz')
    for z in (-.46,.46):box(m,(0,.04,z),(2.55,.045,.07),'metal')
    for x in (-1.23,1.23):box(m,(x,.04,0),(.07,.045,.88),'metal')
    box(m,(0,.03,0),(2.28,.009,.74),'panel')
    return m

def taburete():
    m=mesh();cylinder(m,(0,0,0),.45,.32,'metal_oscuro',12,r_top=.5)
    torus(m,(0,.33,0),.47,.04,'COLOR_JUGADOR',24,5)
    cylinder(m,(0,.33,0),.39,.07,'panel',12)
    for a in range(4):
        x=.34*math.cos(a*math.pi/2);z=.34*math.sin(a*math.pi/2)
        ellipsoid(m,(x,.4,z),(.038,.012,.038),'cian',7,4)
    return m

def robot():
    m=mesh();box(m,(0,0,0),(1,1.1,.7),'metal_luz')
    box(m,(0,.11,.375),(.55,.42,.045),'cian')
    ellipsoid(m,(0,.96,0),(.43,.42,.4),'metal_luz',16,8)
    box(m,(0,.98,.32),(.53,.23,.16),'pantalla')
    for x in (-.13,.13):ellipsoid(m,(x,.99,.415),(.06,.07,.025),'EMISION',8,5)
    for s in (-1,1):
        beam(m,(s*.54,.27,0),(s*.86,-.36,.42),.10,'metal',8)
        ellipsoid(m,(s*.86,-.36,.42),(.14,.13,.13),'cian',9,5)
        box(m,(s*.23,-.57,0),(.12,.10,.32),'metal_oscuro')
    beam(m,(0,1.33,0),(0,1.7,0),.035,'metal_oscuro',6)
    ellipsoid(m,(0,1.72,0),(.085,.085,.085),'rojo',9,5)
    cylinder(m,(0,-1.15,0),.06,.65,'cian',10,r_top=.31)
    torus(m,(0,-.61,0),.32,.045,'metal',16,5)
    return m

def tubo(kind):
    m=mesh()
    color={'normal':'comida','picante':'rojo','dorada':'oro'}[kind]
    beam(m,(-.78,0,0),(.78,0,0),.205,color,14)
    ellipsoid(m,(-.78,0,0),(.205,.205,.205),color,11,6)
    ellipsoid(m,(.78,0,0),(.205,.205,.205),color,11,6)
    for x in (-.52,.05,.54):torus(m,(x,0,0),.22,.029,'metal_luz',14,5,plane='yz')
    if kind=='picante':
        for x in (-.37,0,.37):
            cylinder(m,(x,.19,0),.09,.21,'naranja',6,r_top=0)
        box(m,(0,-.205,.01),(.65,.025,.05),'naranja')
    elif kind=='dorada':
        for x in (-.45,0,.45):ellipsoid(m,(x,.19,.03),(.09,.07,.09),'blanco',8,5)
    else:
        for x in (-.38,.15):box(m,(x,.205,0),(.2,.018,.08),'verde')
    return m

def consola():
    m=mesh();box(m,(0,.65,0),(1.9,1.3,.65),'metal_oscuro')
    box(m,(0,.95,.34),(1.55,.63,.03),'pantalla')
    for i in range(5):box(m,(-.55+i*.25,.98,.36),(.13,.08,.01),'cian' if i%2 else 'verde')
    for x in (-.65,.65):ellipsoid(m,(x,.39,.38),(.10,.10,.04),'oro',9,5)
    box(m,(0,1.35,0),(2,.12,.82),'metal')
    return m

def main():
    specs=[
        ('comedor_orbital',comedor(),'centro suelo','Centro (0,0,-1), suelo a Y=0; malla generada ya desplazada Z=-1.'),
        ('ventanal_tierra',ventanal(),'centro del muro','Centro (0,0,-8), abertura de 20 por 7.'),
        ('tierra',earth(),'centro de planeta','Centro (-7,7,-24), radio 10; cara continental hacia +Z.'),
        ('cielo_estrellado',sky(),'centro de fondo','Centro (0,0,-36); fondo visible a través del ventanal.'),
        ('techo_comedor',techo(),'centro del techo','Centro (0,10,-1).'),
        ('tuberias_techo',tuberia(),'centro tramo','Centro (0,9,-4), 36 unidades a lo largo de X.'),
        ('lampara_colgante',lampara(),'enganche al techo','Repetir en X; pivote en Y=0, luz queda a Y=-1.2.'),
        ('mesa_magnetica',mesa(),'pie centro','Centro (posicionX[i],0,1.3), tapa Y=.86.'),
        ('bandeja',bandeja(),'centro bandeja','Centro (posicionX[i],.95,1.3).'),
        ('taburete',taburete(),'pie centro','Centro (posicionX[i],0,-.3); COLOR_JUGADOR según participante.'),
        ('robot_camarero',robot(),'torso centro','Centro (robotX,2.5,-3); desplazar y animar el brazo aparte.'),
        ('tubo_comida_normal',tubo('normal'),'centro de ración','Ración normal; disminuir escala X según bocados restantes.'),
        ('tubo_comida_picante',tubo('picante'),'centro de ración','Ración roja; alternar tinte de material para aviso.'),
        ('tubo_comida_dorada',tubo('dorada'),'centro de ración','Ración que vale doble; escala y animación externas.'),
        ('consola_lateral',consola(),'pie centro','Decoración a los costados X≈±17; fuera de los puestos.'),
    ]
    for name,m,pivot,usage in specs:save(name,m,pivot,usage)
    (ROOT/'manifest.json').write_text(json.dumps({'minijuego':'Banquete Turbo','base':'claude/expansion-party 6958b26','coordenadas':'Y arriba; unidades raylib; separación puestos 4.2','modelos':MANIFEST},ensure_ascii=False,indent=2)+'\n')
    print(len(specs),'GLB;',sum(v['triangulos'] for v in MANIFEST),'triángulos')

if __name__=='__main__':main()
