## Trabajo autónomo en equipos

Cuando la rama actual comience por `claude/`:

- El líder puede realizar commits locales automáticamente después de:
  1. compilar correctamente;
  2. revisar git diff;
  3. comprobar que el commit contiene solo la tarea terminada.

- Los teammates NO deben realizar commits.
- Solo el líder integra y crea commits.
- Nunca ejecutar push, merge, rebase, cambio de rama, reset destructivo o eliminación no autorizada.
- Si existe una decisión de arquitectura que cambie comportamiento global, detenerse y consultar al usuario.
- Para decisiones locales compatibles con la arquitectura existente, continuar sin pedir autorización.