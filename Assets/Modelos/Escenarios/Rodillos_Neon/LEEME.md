# Rodillos Neón · modelos GLB v1

21 GLB 2.0 originales para `Minigames/MinijuegoRodillosNeon.cpp` de `claude/expansion-party`, referencia `ea5472c`. Cargados y comprobados en raylib. **Todavía no integrados en el juego.**

`GLB/` contiene las piezas; `manifest.json`, sus dimensiones, pivotes y uso. `Vistas/` tiene 26 capturas y `Vista_previa.png` muestra la colección. `generar_modelos.py` y `geometria_glb.py` regeneran los GLB con Python 3 sin dependencias externas. `visor_raylib.cpp` carga y libera las 21 mallas.

## Escala y montaje

Una unidad GLB equivale a una unidad raylib. Eje Y hacia arriba y frente de las máquinas hacia +Z. `suelo_arcade` y `pared_arcade` se colocan en el origen. Para N participantes, centro X de cada máquina `((i-(N-1)/2)*5.8)`, Z=0. `gabinete_arcade` y `marco_jugador` comparten ese origen; el marco se puede teñir por participante. `boton_detener` se ubica en `(X,1.2,1.9)`.

Cada máquina lleva tres instancias de `tambor_rodillo`, centradas en `(X-1.05,3,0)`, `(X,3,0)` y `(X+1.05,3,0)`. Radio de tambor 1,7; gira sobre X en grados `-rodillo.angulo*RAD2DEG`. Para cada uno de los diez valores `k` de la tira aleatoria, colocar el `simbolo_*` de tipo `rodillo.tira[k]` en el espacio local del tambor: `Y=1.73*sin(k*2π/10)`, `Z=1.73*cos(k*2π/10)` y orientación X=`-k*36°`. El giro del tambor debe heredarse por sus símbolos. El visor muestra esa jerarquía y una tira de ejemplo; no guarda la baraja ni el estado del juego en un GLB. La `linea_comodin` va en `(X,3,1.84)`; conservar la ventana temporal y puntuación del C++.

Instanciar `columna_led_cian`/`columna_led_rosa` alternadas en `X=-24+6*c`, Z=−8,5. Colocar tres hologramas en `X=-12+12*k`, Y≈9,5, Z=−6 y rotarlos si se desea. Los cuatro `letrero_*` van en `X=-18+12*k`, Y=12,3, Z=−8,4. El brillo se representa mediante materiales de color intenso; para un halo real se requiere el posprocesado de iluminación del proyecto.

Las mallas son estáticas y no traen rig ni animaciones GLB. Conservar el giro, el frenado, la línea de comodín, los glitches, las cinco rondas, la IA y la cámara del juego. Cargar cada archivo una vez y descargarlo al cerrar. Esta entrega no modifica el repositorio.

## Visor independiente

En Windows con MSYS2 UCRT64 y raylib, desde la carpeta descomprimida:

```powershell
& 'C:/msys64/ucrt64/bin/g++.exe' visor_raylib.cpp -std=c++17 -o VisorRodillosNeon.exe -lraylib -lopengl32 -lgdi32 -lwinmm
./VisorRodillosNeon.exe
```

Teclas 1 a 4: cuatro máquinas, giro de ejemplo, dos máquinas y sala. Escape: salir. `--capturar` regenera las vistas. Los cubos de colores del visor representan jugadores temporales.
