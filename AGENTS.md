# Proyecto Party 3D con raylib

## Objetivo

Este repositorio contiene un juego party 3D local inspirado en el ritmo y la estructura de Mario Party. Debe admitir 2 o 4 jugadores locales, teclado y gamepads, tableros por turnos y minijuegos cortos. Las reglas pueden tomar referencias de juegos del género, pero los nombres, escenarios, personajes, arte, audio y código deben ser originales.

## Restricciones académicas y técnicas

- Lenguaje: C++ con raylib.
- No usar `class`. Usar `struct`, funciones libres y enums.
- Evitar dependencias externas nuevas salvo autorización expresa.
- Mantener compatibilidad con el compilador UCRT64 de MSYS2 configurado en `.vscode/tasks.json`.
- Antes de usar una función o campo de raylib, comprobar que existe en la versión instalada del proyecto.
- En esta versión, `LoadModelAnimations()` recibe `int*` y `ModelAnimation` usa `keyframeCount`.
- Usar `/` en rutas de recursos, por ejemplo `Assets/Modelos/modelo.glb`.
- No modificar `.vscode/tasks.json` sin autorización expresa.
- No cambiar nombres, controles o comportamiento ya existente sin indicarlo primero.

## Estructura y arquitectura

- Leer el proyecto real antes de proponer cambios; no inventar archivos, métodos ni firmas.
- Mantener cada minijuego separado en `Minigames/Nombre.h` y `Minigames/Nombre.cpp`.
- Reutilizar `TiposMinijuegos` y `UtilidadesMinijuegos` para estados, entradas, física, colisiones, partículas y dibujo compartido.
- Integrar prototipos mediante `Gameplay/ZonaPruebas.h` y `Gameplay/ZonaPruebas.cpp`.
- La lógica de tableros debe estar separada de la lógica de minijuegos. Antes de crear el primer tablero, proponer una arquitectura pequeña basada en `struct` y esperar aprobación.
- Todo recurso cargado debe tener una descarga simétrica: modelo, textura, sonido, música y animaciones.
- Si se agrega `Descargar()` en un `.cpp`, declararlo también en el `struct` correspondiente del `.h`.
- Evitar duplicar modelos o texturas en memoria cuando puedan compartirse de forma segura.

## Jugadores y controles

- Soportar hasta cuatro jugadores locales.
- Respetar el modo de teclado configurado y la asignación actual de gamepads.
- No asumir que todos los controles están conectados.
- Manejar conexión y desconexión de gamepads sin dejar una ronda imposible.
- Mostrar en pantalla el control requerido cuando una mecánica dependa de una tecla o botón.

## Diseño de minijuegos

Antes de implementar un minijuego, presentar brevemente:

1. Objetivo y condición de victoria.
2. Duración y fases de la ronda.
3. Controles de teclado y gamepad.
4. Estados necesarios.
5. Colisiones, cámara y física.
6. Recursos necesarios y alternativas temporales con primitivas 3D.
7. Archivos que se crearán o modificarán.

Luego implementar un solo minijuego por tarea. Cada minijuego debe incluir:

- Cuenta regresiva inicial.
- Estado jugando y pantalla final.
- Reinicio con `R` dentro de Zona de Pruebas.
- Salida segura al menú o al selector de pruebas.
- Soporte para 2 y 4 jugadores cuando corresponda.
- Cámara legible para multijugador local.
- Tiempo delta para movimiento y animaciones.
- Límites de velocidad y resolución estable de colisiones.
- Recursos liberados al cerrar.

Si existe `Docs/Wiki-de-mario-party.txt`, usarlo únicamente como referencia de reglas y explicar qué minijuego inspira la propuesta. No copiar código, nombres, música, modelos ni recursos de Nintendo.

## Diseño de tableros

- Construir primero un tablero prototipo pequeño antes de uno definitivo.
- Separar datos de casillas, conexiones, eventos y presentación 3D.
- Representar el recorrido como nodos y conexiones, no como una lista rígida, para permitir bifurcaciones.
- Validar que todas las rutas sean alcanzables y que ninguna bifurcación bloquee la partida.
- Mantener separados: turno, dado, movimiento, evento de casilla, tienda/decisión y fin de ronda.
- Usar primitivas 3D como recurso temporal hasta que existan modelos definitivos.
- No implementar simultáneamente tablero, economía, objetos y todos los eventos. Trabajar por etapas compilables.

## Forma de trabajo obligatoria

1. Ejecutar `git status` antes de modificar archivos.
2. Leer completos los archivos relacionados y sus encabezados.
3. Explicar el plan y señalar cualquier dato faltante; no adivinar requisitos importantes.
4. Hacer cambios directamente en los archivos del repositorio.
5. Compilar usando la tarea existente de `.vscode/tasks.json` o su comando equivalente.
6. Corregir todos los errores provocados por el cambio.
7. Revisar el diff y comprobar declaraciones/implementaciones de todos los métodos nuevos.
8. Informar qué se modificó, cómo probarlo y qué limitaciones quedan.
9. Dar los comandos exactos de Git para guardar el cambio, pero no ejecutar `commit` ni `push` sin autorización.

## Seguridad de cambios

- No usar `git reset --hard`, `git clean`, `push --force` ni eliminar archivos sin autorización expresa.
- No sobrescribir cambios existentes del usuario.
- Si el árbol de trabajo ya está modificado, identificar esos cambios antes de editar.
- Realizar cambios pequeños y verificables; no reescribir todo el proyecto por una función puntual.
- Si una tarea es demasiado grande, dividirla en etapas y completar una etapa compilable por vez.

## Comunicación

- Responder y comentar el código en español.
- Explicar los conceptos de forma didáctica para un desarrollador principiante de raylib con conocimientos junior de C++.
- Cuando se cree un archivo nuevo, mostrar su propósito y ubicación.
- Cuando se detecte un error previo no relacionado, informarlo por separado y no modificarlo sin autorización.
