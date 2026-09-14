Coloca el modelo de la montana de lava en esta carpeta con este nombre:

MontanaLava.glb

El juego lo carga automaticamente al iniciar la Zona de Pruebas. Si el
archivo no existe, conserva las montanas low-poly actuales como fallback.

Recomendaciones de exportacion:
- Formato GLB.
- Eje Y hacia arriba.
- Una sola montana centrada aproximadamente sobre X/Z.
- Materiales y texturas incluidos dentro del GLB.

El codigo centra la base y ajusta la escala automaticamente. Si el modelo
queda acostado, cambia ROTACION_X_MODELO_MONTANA_LAVA_3D en
Core/RecursosJuego.h.
