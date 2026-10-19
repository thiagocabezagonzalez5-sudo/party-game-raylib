# Reglas de los tableros

Reglas que todo tablero (actual y futuro) debe cumplir. La decisión de diseño es del usuario; el código que las comprueba está en `Board/CatalogoTableros.*` (`ValidarCasillasObligatoriasTablero`).

## Regla obligatoria: casillas especiales

**Todo tablero debe tener al menos una casilla de cada uno de estos seis tipos**, además de las básicas (azul, +, −, azar):

| Tipo | Símbolo | Qué hace |
|---|---|---|
| REGALO | caja con lazo | La mitad de las veces +4 monedas; la otra mitad un objeto barato (Botas o Escudo). Con la mochila llena, +4 monedas. |
| CAOS | rayo | Ruleta de castigos para quien cae: pierde 1 trofeo, 20 monedas o todas las monedas; o **caos global** (−20 monedas a todos). Sin trofeos, la opción de trofeo pasa a una pérdida de monedas. Topes en 0. |
| INTERCAMBIO | flechas opuestas | Tres dados: 1) primera persona, 2) *da / recibe / intercambia* con 5–15 monedas o 1 trofeo, 3) segunda persona (distinta). *Intercambia* permuta los totales del recurso sorteado. Nadie queda en negativo. |
| DUELO | espadas cruzadas | El jugador elige rival y apuesta (5, 10 o 20 monedas con tope del más pobre, o 1 trofeo si ambos tienen). Se sortea uno de los minijuegos de duelo 1v1 y el ganador se lleva la apuesta. El Escudo no protege (apuesta voluntaria). Empate, ronda cancelada o sin minijuegos de duelo: se devuelve la apuesta. |
| TIENDA | signo $ | Un comerciante ofrece 3 objetos al azar; se compran con monedas hasta llenar la mochila (3 espacios). |
| EVENTO | exclamación | Evento propio del tablero (ver abajo). Es obligatorio definir el hook `ResolverCasilla` de la `DefinicionTablero`. |

Un tablero que no cumpla la regla **sigue jugándose**, pero registra un aviso en el log una sola vez por tablero (`Tablero '...' incumple la regla de casillas: falta X`). Un tablero nuevo no se da por terminado mientras muestre esos avisos.

### Estado actual

| Tablero | REGALO | CAOS | INTERCAMBIO | DUELO | TIENDA | EVENTO | Estado |
|---|---|---|---|---|---|---|---|
| Isla Arboleda | 5 | 2 | 3 | 2 | 2 | 4 | Cumple |
| Forja del Volcán | 2 | 1 | 1 | 1 | 1 | 4 | Cumple |
| Mercado Flotante | 2 | 1 | 2 | 1 | 2 | 2 | Cumple |
| Torre del Reloj | 2 | 1 | 1 | 1 | 2 | 2 | Cumple |
| Feria Nocturna | 2 | 1 | 1 | 1 | 1 | 2 | Cumple |

En Forja del Volcán el Escudo también cubre sus pérdidas propias: la llama de la erupción y el robo del duende (protege a la víctima).

## Evento propio de cada tablero (casilla EVENTO)

Cada tablero define SU evento en el hook `ResolverCasilla`; para uno con presentación e interacción usa `IniciarEventoInteractivo()` (`Gameplay/EventosCasillas.h`).

- **Isla Arboleda** (al azar entre dos): el Gran Roble tira dos objetos, uno bueno (+3 casillas) y uno malo (−5 monedas), y el jugador elige cuál atrapa sin saber cuál es cuál; o unas abejas lo persiguen y lo llevan al inicio del tablero.
- **Forja del Volcán** (al azar entre tres): veta de oro (+7 monedas), duende ladrón (roba 5 monedas a un rival al azar; si nadie tiene monedas, deja la veta) o géiser (lanza al jugador a otra casilla del anillo). Gimmick: el puente de basalto se levanta en las rondas pares y cierra el atajo; cada ronda una llama cae en una casilla básica del anillo (−5, o −10 en la última ronda: la gran erupción).
- **Mercado Flotante** (la Campana del Mercado, al azar entre dos): día de feria (cada dueño cobra 1 moneda por puesto, máximo 5; si nadie tiene puestos, quien cayó recibe 2) o inspección (quien tiene más puestos pierde uno, que queda libre; el Escudo no lo cubre porque no es una pérdida de monedas ni de trofeos). Gimmick: las casillas azules son puestos; caer en uno libre lo abre por 3 monedas, en el de un rival se pagan 3 de alquiler (el Escudo lo bloquea) y en el propio se cobran 2.
- **Torre del Reloj** (la Campanada, al azar entre dos): cuerda al reloj (la esfera gira 1 paso ya mismo y +3 monedas) o marcha atrás (el jugador retrocede 3 casillas por la esfera; desde una aguja vuelve a la casilla de donde sale). Gimmick: desde la ronda 2, al empezar cada ronda las casillas verdes, rojas y azules de la esfera giran de 1 a 3 posiciones en sentido horario; las especiales y la casilla inicial quedan fijas.
- **Feria Nocturna** (la Gran Rueda): 40 % te llevás todo el pozo (vuelve a 5), 40 % la mitad del pozo, 20 % la rueda se traba y pagás 3 al pozo (el Escudo lo bloquea). Gimmick: las monedas perdidas en las casillas rojas caen al pozo, y desde la ronda 2 el trofeo se remata a 15, 20 o 25 monedas (en la última ronda nunca menos de 20).

## Objetos (inventario de 3 espacios por jugador)

| Objeto | Precio | Efecto |
|---|---|---|
| Botas | 5 | +3 casillas en la próxima tirada |
| Dado doble | 8 | Tira dos dados de 6 y los suma |
| Escudo | 7 | Anula automáticamente la próxima pérdida de monedas o trofeo (casilla negativa, azar, CAOS, evento malo, intercambio en que se pierde, imán) |
| Imán | 9 | Roba 5 monedas a un rival elegido |

Se usan al empezar el turno propio (tecla de cancelar abre el menú). Los bots compran sin gastar lo que necesitan para el trofeo y usan Botas, Dado doble e Imán con reglas simples.

## Minijuegos de duelo

Marcados `esDuelo` en `Core/CatalogoMinijuegos.cpp`: exclusivos del duelo (2 jugadores, fuera de las rondas normales, visibles en el catálogo libre).

1. **Cables Cruzados** (puzzle de circuitos, mejor de 3)
2. **Vagonetas de Mina** (carrera por rieles con turbo)
3. **Gemas Deslizantes** (puzzle deslizante, mejor de 3)

## Reglas generales de partida

- Trofeo: 20 monedas. 5 rondas por partida.
- Cada ronda termina con un minijuego para todos (premio 10 al ganador, 3 de participación).
- Las bifurcaciones nunca bloquean el recorrido.

## Lista de comprobación para un tablero nuevo

1. Las seis casillas obligatorias colocadas en zonas con sentido (por ejemplo la tienda en un muelle o plaza).
2. Hook `ResolverCasilla` con el evento propio y `TextoLeyendaCasilla` si cambia algún valor común.
3. `DecidirRutaBot` y valor de bot para **todos** los tipos (sin `default` que oculte alguno).
4. Camino físico visible entre casillas, bifurcaciones que se dividan en rutas visibles y rendimiento estable (mallas horneadas en GPU, respetar `Systems/CalidadGrafica.h`).
5. Partidas con 2, 3 y 4 bots que terminen siempre, y capturas revisadas a 1280x720 y 1920x1080.
6. Sin avisos de la regla obligatoria al cargar el tablero.
