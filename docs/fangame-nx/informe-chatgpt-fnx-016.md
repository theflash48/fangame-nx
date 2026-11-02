# Informe para ChatGPT · FangameNX FNX-016

Fecha: 2026-11-02. Rama: `fix/fnx-016-sdl-shutdown`. Compilación y paquete: commit `2a759c5`. La documentación del resultado físico se confirma en el commit siguiente de esta rama (`git log -1`).

## Resultado ejecutivo

**FNX-016 superado para el fixture sintético probado una vez en Nintendo Switch 1 con Atmosphère.** El propietario confirmó que al pulsar B la aplicación volvió **por sí sola** al Homebrew Menu, sin usar HOME → X → Cerrar. Los logs obtenidos después por MTP confirman lectura y escritura de archivo, eventos A/X/B, salida normal del script Ruby, terminación del hilo RGSS, cierre de `SDL_DestroyWindow`, `SDL_Quit` y llegada al punto previo al retorno de `main`. La vuelta efectiva al Homebrew Menu es observación del propietario, no una inferencia a partir del log.

## Causa y corrección

EXIT TRACE llegó a `alcCloseDevice OK` y se detuvo dentro de `SDL_DestroyWindow`. El contexto GL se creó y utilizó en el hilo RGSS y se destruía desde el hilo principal sin desasociarlo previamente del hilo que lo usó. El parche mínimo `patches/mkxpz-sdl-shutdown.patch` invoca `SDL_GL_MakeCurrent(window, NULL)` en el hilo RGSS antes de terminarlo y añade marcas alrededor de esa llamada y de `SDL_GL_DeleteContext`. En la nueva prueba aparecen `RGSS after GL unbind`, `main after GL delete` y `SDL_DestroyWindow OK` en ese orden. Esto respalda la hipótesis de un problema de ciclo de vida de EGL entre hilos; no hay traza interna que identifique la subllamada exacta que bloqueaba.

## Build, paquete y evidencias

- Construcción **incremental** del motor y generación de NRO con `libgcc` aislada para Cortex-A57. `reconstruir-src.py --orden`: 39 archivos idénticos y cero desviaciones. Auditoría estática: cero patrones incompatibles conocidos, cero instrucciones `<unknown>` con `--mcpu=cortex-a57`, cero símbolos indefinidos. Estas comprobaciones no sustituyen pruebas físicas amplias.
- NRO instalado: `SD Card/switch/fnx016-sdl-fix/fnx016-sdl-fix.nro`, 17 749 502 bytes, SHA-256 `726e86074266e609aad6db91d570f3397f0f17752369ac0c72534dae5fe8dd6d`. Los ocho archivos del paquete se leyeron de vuelta por MTP y coincidieron con el manifiesto local.
- Logs de la prueba recuperados **solo en lectura** por WPD/MTP a `local/fnx016-sdl-shutdown/postboot-evidence-20261102-114055/`, excluido de Git. Sus tamaños y hashes están en `docs/fangame-nx/cierre-sdl-fnx-016.md`. `fnx009-probe.log` registra `lectura=OK`, `escritura=OK`, cinco A, cuatro X, petición B y `FNX009 RUBY COMPLETADO; salida normal`. `mkxp-boot.log` llega a `[exit] 08 before return main`.
- Las tres instalaciones anteriores (`fnx004-sintetico`, `fnx009-interactivo`, `fnx016-exit-trace`) se archivaron localmente, 36 archivos incluidos logs, antes de retirarlas. La comprobación final del directorio `SD Card/switch/` encontró **solo `fnx016-sdl-fix`** entre las carpetas `fnx*`.

## Alcance y siguiente tarea

Esto valida un caso **sintético**, no la ejecución de fangames de RPG Maker XP o Pokémon Essentials ni la biblioteca multijuego. Tampoco establece estabilidad tras ciclos repetidos. Próxima tarea recomendada: repetir el ciclo de arranque y salida controlada varias veces con fixtures sintéticos y comprobar que los logs de cada ciclo terminan en `[exit] 08`, luego preparar una prueba de selección entre **dos fixtures propios** desde un mismo NRO, preservando evidencias y sin introducir recursos de terceros. Mantener cualquier selector o cambio de contrato como misión separada; aún no está implementado.
