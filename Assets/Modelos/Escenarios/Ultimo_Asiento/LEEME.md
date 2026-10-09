# Último Asiento — paquete de modelos v1

17 archivos GLB originales creados para la rama `claude/expansion-party`, referencia `ea5472c`.
Este paquete contiene arte listo para cargar y un visor independiente. **Todavía no está integrado en el minijuego.**

## Contenido

- `GLB/`: modelos modulares, con materiales y geometría dentro de cada archivo.
- `Vistas/`: capturas reales de raylib de cada pieza, del carrusel y la noria armados y de la escena de comprobación.
- `Vista_previa.png`: lámina de presentación.
- `manifest.json`: triángulos, límites, materiales, pivotes y ubicación sugerida de cada pieza.
- `generar_modelos.py`: generador completo y reproducible; Python 3, sin dependencias externas.
- `visor_raylib.cpp`: visor completo en C++17, raylib y raymath; sin clases propias.
- `VALIDACION.txt`: resultados de comprobación de los GLB.

## Escala y orientación

Una unidad de modelo equivale a una unidad del juego. Y es arriba. Frente de caballito y carro: +X. Frente del puesto: +Z.
Usar escala 1; las medidas provienen del código de Último Asiento. El suelo del juego está en Y=-0.05.
El carrusel conserva su radio físico de exclusión de 2.5. La base tiene radio 2.2 y el techo llega a 2.48.
La taza queda dentro de un radio de 1.25, incluido su asa. El platillo tiene radio 1.2 y el borde está a Y=0.737.
La taza es hueca y baja para permitir ver al jugador. **Las colisiones siguen siendo las actuales del juego**; no derivarlas de la malla de la taza.
La grada conserva una superficie a Y=0.7, compatible con los puestos de espectadores existentes.

## Piezas

| Archivo | Triángulos | Pivote |
| --- | ---: | --- |
| `carrusel_base.glb` | 2,064 | Centro a nivel del suelo |
| `carrusel_columna.glb` | 360 | Mismo origen que base |
| `carrusel_techo.glb` | 2,328 | Mismo origen que base |
| `caballito.glb` | 1,368 | Centro del cuerpo; frente +X |
| `poste_caballito.glb` | 32 | Pie a nivel del carrusel |
| `bombilla.glb` | 120 | Centro de la esfera |
| `taza.glb` | 1,616 | Centro del platillo a nivel del suelo |
| `valla_tramo.glb` | 616 | Centro del tramo, suelo |
| `noria_soporte.glb` | 352 | Centro de la base, suelo |
| `noria_rueda.glb` | 3,776 | Centro del eje de giro |
| `noria_cabina.glb` | 244 | Punto superior de suspension |
| `montana_rusa_vias.glb` | 8,732 | Centro XZ, suelo |
| `montana_rusa_carro.glb` | 256 | Centro de ruedas al nivel de los rieles |
| `puesto_feria.glb` | 526 | Centro de la base, frente +Z |
| `grada.glb` | 488 | Centro XZ, suelo |
| `globo.glb` | 232 | Centro del globo |
| `arena.glb` | 2,112 | Centro de la superficie de juego |

## Movimientos

Los GLB son mallas estáticas modulares, sin esqueletos ni animaciones incrustadas. El movimiento se aplica desde C++, como en el código actual:

- Carrusel: base, columna y techo comparten origen. Caballitos y postes se instancian a radio 1.55. Girar sus posiciones con `anguloCarrusel`. El caballito tiene pivote en el cuerpo y se coloca a altura 1.2 más el vaivén existente.
- Bombillas: 12 instancias a radio 2.4, Y=3.25 más el nivel del suelo; material blanco para teñir con los colores de la ronda.
- Noria: soporte en `(15,0,-12.5)`. Rueda en `(15,6.4,-12.5)`, rotación sobre Z. Las ocho cabinas se desplazan alrededor del eje a radio 5 y se mantienen verticales.
- Montaña rusa: vías desplazadas a Z=-17. Carro a X=-21+42*t, Y=5+3.2*sin(9*t)+1.6*sin(23*t), más el contacto de las ruedas; orientar según la pendiente.
- Valla: módulos tangentes al círculo de radio 8.65. Los extremos coinciden aproximadamente; no crear colisiones nuevas para ellos.

## Colores durante la partida

Materiales `COLOR_DINAMICO` y `BOMBILLAS` blancos permiten recolorear sin perder los adornos.
`DrawModel(..., tint)` tiñe todo el modelo: para conservar los bordes dorados y la porcelana, cambiar únicamente el material dinámico.
El visor incluido muestra el procedimiento con `meshMaterial` y restaura el color después de dibujar cada instancia.

| Modelo | Primitive del material dinámico (desde 0) |
| --- | ---: |
| taza | 2 |
| noria_cabina | 3 |
| globo | 0 |
| valla_tramo, bombillas | 2 |
| bombilla | 0 |

Los índices anteriores corresponden exactamente a esta versión del paquete. Verificar nuevamente si se regenera modificando materiales.
No duplicar modelos por jugador: cargar una vez y dibujar instancias. Restaurar el material tras cada cambio de color.

## Abrir el visor en Windows / MSYS2 UCRT64

Desde la carpeta extraída, que debe contener `GLB/`, ejecutar en PowerShell:

```powershell
& 'C:/msys64/ucrt64/bin/g++.exe' visor_raylib.cpp -std=c++17 -o VisorUltimoAsiento.exe -lraylib -lopengl32 -lgdi32 -lwinmm
./VisorUltimoAsiento.exe
```

Usa la misma instalación de raylib que tu juego. Si el compilador está en otra ruta, ajustar únicamente ese comando.
Teclas 1 a 5 cambian la vista; flechas izquierda/derecha orbitan; Escape cierra. Todos los modelos se descargan al salir.
El visor y los GLB se comprobaron en Linux con raylib 6.1-dev y SDL/OpenGL. No se ejecutó el binario Windows.

## Integración posterior

Copiar `GLB/` a `Assets/Modelos/Escenarios/UltimoAsiento/`. Sustituir únicamente el dibujo de las primitivas existentes.
Cargar recursos una sola vez y descargar cada `Model` exactamente una vez. Evitar recargarlos al reiniciar una ronda.
Conservar cámaras, tiempos, reglas, posiciones de espectadores, colores de trampa y condiciones de ocupación.
La noria y buena parte de la montaña rusa quedan fuera del encuadre original durante la partida: la vista general incluida las muestra para inspeccionar el arte.
Las capturas de escena son del visor de comprobación, no una integración ya aplicada al juego.

## Aspecto y licencia

Arte original low-poly con cereza, azul, crema, dorado y violeta. No contiene modelos, texturas ni marcas de Nintendo ni recursos descargados de terceros.
No necesita texturas externas. Usa color de material y color de vértice para un sombreado discreto; no requiere un shader PBR.
Los materiales tienen normales planas. Las bombillas son geometría coloreada; el paquete no agrega bloom ni iluminación dinámica.

El visor fija los planos de recorte en 0.1 y 200 para reducir artefactos de profundidad durante la inspección.
