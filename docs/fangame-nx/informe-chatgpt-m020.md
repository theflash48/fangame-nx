# Informe para ChatGPT · FangameNX M020

Fecha: 2026-11-10. Rama: `feature/fangamenx-launcher-mvp`. Base: `3e9a57e` (M019). La implementación y este informe están en el commit que los contiene (`git log -1`). Sin `push`.

## Resultado

El propietario confirmó en Switch 1 que b32 ejecuta Pokémon Ópalo 2.11 a velocidad normal y abre el menú rápido. Comunicó dos defectos: panel invertido horizontal y verticalmente, y salida a HOME al elegir la biblioteca. **b34 está instalada y su SHA-256 se verificó por lectura MTP**. La orientación y el retorno de b34 aún requieren ejecución física; la instalación verificada no demuestra esas funciones.

## Evidencia del fallo de b32

Antes de tocar el NRO se copiaron por MTP los cinco logs de FangameNX y Ópalo a `local/m020/evidence-20261110-192812/`, con SHA-256 y tamaños en `manifest.json`. `mkxp-boot.log` termina en métricas de la partida, sin entrada al menú ni trazas `[exit]`. `mkxp-debug.log`, `mkxp-ruby.log` y `launcher.log` tampoco contienen la selección de salida, una excepción Ruby/C++/SDL/EGL ni la vuelta al launcher. Por tanto, **no se puede demostrar con esos logs la causa concreta del salto a HOME ni distinguir en la traza un retorno normal a Homebrew Menu**. HOME es observación del propietario. No hay evidencia para atribuirlo a Atmosphère.

El código de b32 sí mostraba una ruta anómala: al confirmar la salida, `runQuickMenu()` publicaba `SDL_QUIT`, esperaba `rqTerm` y `swapGLBuffer()` llamaba inmediatamente a `checkShutDownReset()`, que termina Ruby dentro del intercambio de pantalla. En b34 se deja que ese intercambio y el overlay regresen a Ruby y se atiende `rqTerm` en el punto habitual de `Graphics.update`. Se añadieron trazas `[quick]` antes de `SDL_QUIT` y al recibir `rqTerm`. Se conservan la confirmación, el cierre cooperativo Ruby, la propiedad GL del hilo RGSS, el orden SDL y `envSetNextLoad` tras el cierre. **Es una corrección fundada en el flujo de código, no una causa física probada todavía.**

## Orientación del panel

El panel es un `Bitmap` dibujado con un `Quad` sobre el framebuffer final. En b32 el pase directo usaba el rectángulo UV `(0,0,w,h)`; el propietario observó el contenido completo girado 180 grados. b34 invierte ambos ejes del rectángulo UV del `Bitmap` en ese pase. No transforma el framebuffer ni el render RGSS del juego. La correspondencia exacta entre textura, proyección y pantalla queda pendiente de confirmar visualmente en Switch; no se dispone de una captura del framebuffer de b34.

## Controles por juego

El menú rápido añade **Controles del juego**. Muestra las cuatro entradas lógicas A/B/X/Y del motor y el botón físico asignado. Tras elegir una entrada, espera una pulsación física A/B/X/Y; PLUS cancela la captura. Si el botón ya pertenece a otra entrada, intercambia las dos asignaciones y muestra el conflicto. Hay opción de restaurar el valor predeterminado y de volver. Una captura incompleta no se guarda.

La configuración se guarda como `controls_<id>` en `config/launcher.json`, cadena permutada `ABXY`; la ausencia del campo mantiene la distribución b32. Se reutiliza la escritura temporal y recuperación de respaldo, sin alterar `speed_<id>`. Los cuatro botones son una permutación: cada pulsación y liberación física se traduce a una única entrada lógica, sin duplicados ni estados perdidos por conflictos. El hilo de eventos ejecuta el atajo y los controles del menú **antes** de remapear A/B/X/Y para el juego. Tanto los bindings RGSS como las consultas directas del estado de mando leen el `controllerState` ya remapeado. L, R y PLUS quedan físicos y no editables; por ello siempre permiten abrir el menú. No se cambió la distribución de Ópalo ni Z en la tarjeta: el propietario puede elegirla desde el editor.

## Construcción y comprobaciones

- `tests/run-launcher-host.cmd local/m013/package-8763807865`: pasa `launcher_host` y `quick_menu_host`. Incluye persistencia separada entre Ópalo y Z, intercambio A↔Y, restauración, captura física, atajo y cadencia.
- `python reconstruir-src.py --orden`: 40 archivos reconstruidos idénticos, cero desvíos y cero archivos sin parche. El cambio del motor queda en `patches/mkxpz-m020-menu-controles.patch`.
- `make -C mkxp-z-nx -j6 FNX_LAUNCHER=1 FNX_LIBGCC_PREFIX=.../src/build-logs/fnx016-isolated-prefix FNX_LINK_MAP=.../local/m020/FangameNX-b34.map`: construye **0.2.4 b34** con devkitA64. `auditar-armv8.py`: cero patrones incompatibles conocidos y 11 788 408 bytes de código ELF/NRO idénticos. `objdump -d -M no-aliases`: cero `<unknown>` o extensiones buscadas; `nm -u`: cero símbolos indefinidos; ELF64 AArch64 PIE. `python comprobar-nro.py`: cabeceras, rangos, NACP, icono y RomFS válidos. `git diff --check` pasa.
- NRO b34: **17 837 257 bytes**, SHA-256 `dcb52f751dcf2bdb5a27903adf913d6186caccb8a5a60a18de3b7cbb48a48da1`. Paquete privado `local/m013/package-dcb52f751d/`, 25 archivos, manifiesto SHA-256 `bc078d74ecbc2dad1c8298f8c25bfb714536e17f3ed0ef5d21f57c7ce96fbc7e`.

## Instalación MTP

Se detectó una sola Nintendo Switch WPD presente y en estado OK. Antes de reemplazar el NRO activo se leyó y verificó b32 (`8763807865a3a2c1008decc12f1f00be2c4411b94495114ca2476afcfb40f2a3`) en `local/m020/old-nro-b32/`. Se instaló b33 provisional, se leyó de vuelta, y se respaldó antes de pasar a b34. El temporal b34 y el archivo final `SD Card/switch/FangameNX/FangameNX.nro` se leyeron por MTP y ambos devolvieron el hash b34 exacto. Los temporales remotos se retiraron. Evidencia local: `local/m020/mtp-update-b33.ps1`, `mtp-update-b34.ps1`, sus logs y carpetas `stage-readback-*`, `final-readback-*`, `old-nro-*`. No se escribieron carpetas de juegos, scripts, `LastSave.dat`, configuración ni Atmosphère.

## Próxima prueba física y riesgos abiertos

En b34: iniciar Ópalo, abrir L+R+PLUS, leer el panel y navegar, entrar a Controles, asignar una entrada a otro botón, reanudar y probar la acción del juego, reabrir y restaurar, luego confirmar Volver a la biblioteca. Comprobar que se llega a FangameNX y que Z conserva controles predeterminados; confirmar que Ópalo sigue a velocidad normal. Si vuelve a fallar, recuperar `mkxp-boot.log` y `mkxp-debug.log` **antes** de otro arranque: las dos trazas `[quick]` separan la solicitud de cierre de la terminación Ruby. Siguen abiertas la X intermitente y la antigua incidencia de pánico táctil. La auditoría estática y el host no sustituyen una ejecución en consola.
