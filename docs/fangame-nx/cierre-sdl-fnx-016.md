# FNX-016 · cierre de SDL después de EXIT TRACE

Rama `fix/fnx-016-sdl-shutdown`. Trabajo limitado al cierre del motor y a los fixtures propios.

## Evidencia de la prueba física

El usuario observa de nuevo `SALIENDO...` inmóvil al pulsar B. Las copias de cuatro registros obtenidas por WPD/MTP de `SD Card/switch/fnx016-exit-trace/` están en `local/exit-trace-evidencias-mtp-20261102-090424/` (ignorado por Git); no se alteraron los originales. `fnx009-probe.log` registra `lectura=OK`, `escritura=OK`, petición B y `FNX009 RUBY COMPLETADO; salida normal`. `mkxp-debug.log` llega a `Shutting down.`. `mkxp-boot.log` marca `[exit] 02 alcCloseDevice OK`, pero no `[exit] 03 SDL_DestroyWindow OK`. Queda demostrado que la llamada a `SDL_DestroyWindow` no ha retornado; aún no se conoce la subllamada exacta que se detiene.

## Diagnóstico previo a la modificación

El contexto GL se crea y usa en el hilo RGSS (`rgssThreadFun`), que termina sin desasociarlo. El hilo principal espera a RGSS, llama `SDL_GL_DeleteContext` desde otro hilo y luego `SDL_DestroyWindow`. La implementación SDL 2.28.5 consulta el contexto **del hilo actual** en `SDL_GL_DeleteContext`: solo invoca `SDL_GL_MakeCurrent(NULL, NULL)` si ese contexto es actual en el hilo llamante. Así, en nuestro orden de cierre no se garantiza liberar el vínculo EGL del hilo RGSS antes de destruir el contexto y la superficie. El backend Switch de SDL llama `SDL_EGL_MakeCurrent(NULL, NULL)` y `SDL_EGL_DestroySurface` al destruir la ventana. Esto ofrece una causa plausible del bloqueo, todavía **no confirmada** por traza interna de EGL.

Fuentes primarias inspeccionadas: [SDL_video.c 2.28.5](https://github.com/libsdl-org/SDL/blob/release-2.28.5/src/video/SDL_video.c), [SDL_egl.c 2.28.5](https://github.com/libsdl-org/SDL/blob/release-2.28.5/src/video/SDL_egl.c) y [parche Switch de devkitPro SDL2 2.28.5](https://github.com/devkitPro/pacman-packages/blob/master/switch/SDL2/SDL2-2.28.5.patch).

## Corrección mínima propuesta

Después de `SharedState::finiInstance()` y antes de terminar el hilo RGSS, desasociar el contexto GL con `SDL_GL_MakeCurrent(window, NULL)` **en el propio hilo que lo utilizó**. Conservar el cierre de OpenAL y el orden restante. Añadir trazas justo alrededor de la desasociación y de `SDL_GL_DeleteContext` para comprobar el punto alcanzado en la próxima prueba. Si falla la desasociación, registrar `SDL_GetError()`; no ocultar el error ni saltarse `SDL_DestroyWindow`. El cambio se conserva en un parche separable y reversible.

## Resultados

Se conservó el `main.cpp`, ELF y NRO de EXIT TRACE en `local/fnx016-sdl-shutdown/before/` antes de reconstruir. La corrección está en [`mkxpz-sdl-shutdown.patch`](../../patches/mkxpz-sdl-shutdown.patch). `reconstruir-src.py --orden` dedujo que debe aplicarse después de `mkxpz-shutdown-traza.patch`: **39 archivos reconstruidos idénticos**, cero desviaciones y cero cambios sin parche (`local/fnx016-sdl-shutdown/reconstruccion-final.log`). Una primera versión del parche tenía demasiado poco contexto y se aplicó prematuramente en la deducción automática; se amplió el contexto, se volvió a comprobar y no se usó esa versión para construir el paquete.

El comando ejecutado en MSYS2 de devkitPro fue `make -C mkxp-z-nx -j6 FNX_LIBGCC_PREFIX=C:/Users/pablo/Documents/GitHub/FangameNX/src/build-logs/fnx016-isolated-prefix FNX_LINK_MAP=C:/Users/pablo/Documents/GitHub/FangameNX/local/fnx016-sdl-shutdown/motor.map`. Resultado **0**; compilación **incremental** de `main.cpp` y `nx_boot.c`, enlace ELF y generación NRO, sin `fixup-arm64.py`. Log íntegro: `local/fnx016-sdl-shutdown/build.log`. El mapa incluye los ocho miembros requeridos de la `libgcc` aislada (`libgcc.a` SHA-256 `b7bca10ee17cae2295533e793c513ab144095e37b2e21142817c5c9811c837dc`). No se modificó devkitPro.

| Artefacto local | Bytes | SHA-256 |
|---|---:|---|
| `mkxp-z-nx/rgss-switch.elf` | 88 855 200 | `28dea29da5df2884074ec049bdf1dda8f4e5d06f1afa6e421a7cae5aaa79ee57` |
| `mkxp-z-nx/rgss-switch.nro` | 17 750 457 | `c50c0b4ad5793a2b2fda2f02f17db7cbe62bc70faab0f14b706c3ba68932ba4f` |
| `local/fnx016-sdl-shutdown/switch/fnx016-sdl-fix/fnx016-sdl-fix.nro` | 17 749 502 | `726e86074266e609aad6db91d570f3397f0f17752369ac0c72534dae5fe8dd6d` |
| `local/fnx016-sdl-shutdown/MANIFEST.sha256.txt` | — | `8b255c25a45c4cec6b85e5a4a09b79a24bf36d49c69fc1b193b528b429b0a9c5` |

El NRO empaquetado se creó con el icono neutro existente y NACP `FangameNX SDL FIX`, versión `0.0.3`, sin cambiar el script sintético M009 ni su contrato de arranque. Contiene **8 archivos** y cuatro directorios, entre ellos `Audio/` y `Graphics/` vacíos. `comprobar-nro.py` verificó NRO0/ASET/NACP/RomFS. `auditar-armv8.py` dio **0 patrones incompatibles conocidos** y cero diferencias entre 11 713 592 bytes de código ELF/NRO, tanto en el NRO del motor como en el empaquetado. `llvm-objdump -d --mcpu=cortex-a57` dio **0 `<unknown>`** y `aarch64-none-elf-nm -u` **0 símbolos indefinidos** (`local/fnx016-sdl-shutdown/checks.log`). Estas verificaciones son estáticas.

## Conservación y limpieza de la Switch

Se accedió solo al dispositivo USB WPD/MTP `Nintendo Switch` (VID `057E`, PID `201D`), a `SD Card/switch/` y a las carpetas propias de FangameNX. No se enumeraron ni tocaron unidades extraíbles convencionales. Antes de retirar versiones, se copiaron **todos los archivos** de `fnx004-sintetico/`, `fnx009-interactivo/` y `fnx016-exit-trace/` a `local/fnx016-sdl-shutdown/mtp-archive/`, carpeta ignorada por Git: **12 por instalación, 36 en total**, incluidos los logs y marcadores. Se comprobó de nuevo cada SHA-256 local contra `archive.json` (hash de ese inventario: `9cb9e4fda3a8f4177f9949debababc58fc98dc1e8aaa3088dc4f6f372aec3a3f`). Los NRO archivados coinciden con sus paquetes locales conocidos. MTP no proporciona comparación directa de bytes remotos; los hashes del archivo son de las copias locales legibles.

Se creó `SD Card/switch/fnx016-sdl-fix/`, se copiaron sus ocho archivos y cuatro carpetas, y se leyeron por MTP los ocho archivos de vuelta a `local/fnx016-sdl-shutdown/readback/`: **8/8 hashes coincidentes** con el manifiesto, incluido el NRO. Después se eliminaron **solo** las tres carpetas antiguas indicadas mediante la API WPD, comprobando la desaparición de cada una. Una invocación inicial al verbo de Shell abrió un diálogo y se canceló sin borrar; se usó la API WPD con identificadores resueltos por nombre y conteo de hijos. La comprobación final de las carpetas de prueba de FangameNX encuentra **únicamente `fnx016-sdl-fix`**. Ninguna otra aplicación ni configuración de la Switch fue modificada.

## Plan del ensayo físico

Se indicó al propietario salir de USB File Transfer de la forma habitual, abrir Homebrew Menu con memoria de aplicación y ejecutar **FangameNX SDL FIX**. Es el fixture visual FNX009: A incrementa por pulsación, X alterna color y B muestra `SALIENDO...`. La prueba debía observar si B devolvía la aplicación al menú por sí misma y, en caso de bloqueo, cerrar con HOME. Los registros se esperaban solo en `SD Card/switch/fnx016-sdl-fix/`. Antes de recibir el resultado físico, la salida y FNX-016 seguían sin validar.

## Resultado físico posterior

El propietario confirmó que, al pulsar **B**, `FangameNX SDL FIX` **volvió por sí solo al Homebrew Menu**, sin usar HOME → X → Cerrar. Es una observación de una ejecución física en Nintendo Switch 1; no equivale a una prueba de estabilidad prolongada ni de compatibilidad con fangames.

La Switch seguía accesible por WPD/MTP. Se copiaron **solo** `mkxp-boot.log`, `mkxp-debug.log`, `fnx009-probe.log` y `fnx009-escritura.txt` desde `SD Card/switch/fnx016-sdl-fix/` a `local/fnx016-sdl-shutdown/postboot-evidence-20261102-114055/` (ignorado por Git), sin modificar los originales. Copias legibles:

| Archivo | Bytes locales | SHA-256 de la copia local |
|---|---:|---|
| `mkxp-boot.log` | 4 043 | `42e12661cc0a3f99d15cc9eeccec1dda2bf19e96112238344e22c400dbcd01da` |
| `mkxp-debug.log` | 888 | `76095ba9983a4b8e8e9338bddc3459093a5dbaaf0eb6afc6a8b650d68e06a8a1` |
| `fnx009-probe.log` | 332 | `39c7caf1601c176c2e7b1cade8ef551f74689786a63f5b0632ae32204f501ac9` |
| `fnx009-escritura.txt` | 20 | `f715634853244f18c30bfb378193bf828ab65b41c45c7eeff61b27aa1a7341d3` |

`fnx009-probe.log` registra `lectura=OK`, `escritura=OK`, cinco pulsaciones A, cuatro cambios X, `boton=B salida_solicitada` y `FNX009 RUBY COMPLETADO; salida normal`. `mkxp-debug.log` registra la terminación del hilo RGSS y `Shutting down.`. `mkxp-boot.log` registra, en orden, `RGSS after GL unbind`, `main after GL delete`, `02 alcCloseDevice OK`, `03 SDL_DestroyWindow OK`, `04 Sound_Quit OK`, `05 TTF_Quit OK`, `06 IMG_Quit OK`, `07 SDL_Quit OK` y `08 before return main`. La llamada a `SDL_DestroyWindow` pasó de no devolver el control en EXIT TRACE a completarse en esta ejecución. La explicación de que el vínculo EGL entre hilos causaba el bloqueo sigue siendo una **inferencia fuerte**, no una traza interna de EGL.

**Estado FNX-016:** superados en una prueba sintética física el arranque, render, entradas A/X/B, lectura, escritura, ejecución y finalización de Ruby, y retorno autónomo al Homebrew Menu. Pendientes pruebas repetidas, estabilidad, cambio de juego y compatibilidad con juegos reales. No se ha probado ningún fangame ni se ha implementado el lanzador multijuego.
