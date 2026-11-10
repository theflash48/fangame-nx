# Informe para ChatGPT · FangameNX M021

Fecha: 2026-11-10. Rama: `feature/fangamenx-launcher-mvp`. Base: `c8df114` (M020). La implementación y este informe están en el commit que los contiene (`git log -1`). Sin `push`.

## Resultado y evidencia de b34

El propietario confirmó físicamente en Switch 1 que el menú de b34 se abre, el eje Y ya está bien y el eje X sigue reflejado; al seleccionar la salida a biblioteca todavía ocurre un crash. La velocidad nominal de Ópalo se había confirmado en b32. No se ha probado aún el remapeo.

Antes de volver a abrir el launcher, se copiaron por MTP cinco registros a `local/m021/evidence-20261110-222112/`; `manifest.json` conserva tamaños y SHA-256. `launcher.log` acaba en `seleccion: pokemon-opalo`. La última sesión de `mkxp-boot.log` identifica **b34**, llega a la carga de Ópalo y se corta después de `[mem] pico del monton 479 MB`, sin ninguna marca `[quick]`, excepción Ruby ni limpieza `[exit]`. `mkxp-debug.log` tampoco registra el cierre. Se comprobó por MTP que no había otros archivos de error o crash en la carpeta de Ópalo ni en `logs/` de FangameNX. El logger de arranque mantiene el descriptor abierto y no sincroniza cada línea; una caída puede perder las marcas recientes. Por ello **los registros no localizan el último paso del overlay ni prueban el punto exacto de la caída**. HOME sigue siendo la observación del propietario, no un diagnóstico de Atmosphère.

## Causa de código y cambio de ciclo de vida

La ruta de b34 seguía haciendo `SharedState::checkShutdown()` desde `Graphics::update()`, llamado en `graphicsUpdate` mediante `rb_thread_call_without_gvl`. `checkShutdown()` llama `scriptBinding->terminate()`, que en el binding MRI es `rb_raise(rb_eSystemExit, " ")`. Se invocaba así una excepción de Ruby **sin el GVL**. Mover la llamada del intercambio GL al comienzo de `Graphics.update` en M020 no cambió esa condición. Es una infracción demostrable por el código; los logs perdidos no permiten demostrar que fuese la única causa física del crash.

Ahora la confirmación deja una solicitud atómica y cierra el overlay. No publica `SDL_QUIT`, no espera `rqTerm` y no finaliza Ruby en el bucle de dibujo. El binding de `Graphics.update` (y los otros métodos gráficos que pueden presentar fotogramas) comprueba la solicitud **después** de volver de `rb_thread_call_without_gvl` y de liberar `GFX_LOCK`; entonces emite `SystemExit` con el GVL recuperado. `mriBindingExecute` limpia Ruby y `rgssThreadFun` solicita el fin del hilo de eventos, igual que en la salida voluntaria de los fixtures que regresó a biblioteca en las pruebas anteriores. Se conservan el diálogo de confirmación, el cierre de GL en su hilo, SDL y `envSetNextLoad` posterior. `Graphics.wait` deja de iterar tras la solicitud.

Se añadieron marcas sincronizadas a disco en la confirmación, tras cerrar el overlay y justo antes de `SystemExit`, para que un fallo posterior permita distinguir esas tres transiciones sin activar el diagnóstico lento para toda la partida. No se cambiaron juegos ni guardados.

## Corrección horizontal

El quad de b34 usaba UV `(w,h,-w,-h)`. La prueba física mostró V correcto y U reflejado. b35 usa `(0,h,+w,-h)`: **solo U cambia de signo; V permanece igual**. El cambio afecta exclusivamente a la textura del menú, no al framebuffer del juego ni al launcher principal. Se generó un patrón host asimétrico en `local/m021/overlay-uv-correcto.png` y su referencia reflejada en `overlay-uv-b34-reflejado.png`: texto «IZQUIERDA»/«DERECHA» y barras roja/azul en extremos opuestos. La inspección visual confirma la geometría buscada; el resultado OpenGL en Switch sigue pendiente de comprobación.

## Verificación y despliegue

- `tests/run-launcher-host.cmd local/m013/package-8763807865`: pasan `launcher_host` y `quick_menu_host`, incluido el estado de solicitud de salida, velocidad, atajo y controles por juego.
- `python reconstruir-src.py --orden`: 41 archivos idénticos, cero desvíos y cero archivos sin parche. El cambio del motor está en `patches/mkxpz-m021-menu-exit.patch`; `git diff --check` pasa.
- `make -C mkxp-z-nx -j6 FNX_LAUNCHER=1 FNX_LIBGCC_PREFIX=.../src/build-logs/fnx016-isolated-prefix FNX_LINK_MAP=.../local/m021/FangameNX-b35.map`: genera **0.2.4 b35**. Auditoría: cero patrones ARM incompatibles conocidos, cero `<unknown>` o extensiones buscadas por GNU objdump, cero símbolos indefinidos y 11 788 792 bytes de código ELF/NRO idénticos. `python comprobar-nro.py` valida NRO, NACP, icono y RomFS.
- NRO b35: **17 837 257 bytes**, SHA-256 `c947ac9a9d351512ed01c8a9200af6ecb25295b9ed27c253d165074c3666b2bd`. Paquete privado `local/m013/package-c947ac9a9d/`: 25 archivos verificados contra el manifiesto, cuyo SHA-256 es `90c22116ab2cca66c85d7fbb756baf9d55b47c2b292369f674f05585df1c3f7a`.
- MTP identificó una sola Nintendo Switch WPD presente y OK. El NRO b34 activo se leyó y respaldó en `local/m021/old-nro-b34/` con su hash esperado. El temporal b35 y el archivo final `SD Card/switch/FangameNX/FangameNX.nro` se recuperaron por MTP con el hash b35 exacto; se retiró el temporal remoto. Scripts y evidencias están en `local/m021/mtp-update-b35.ps1`, `mtp-update-b35.log`, `stage-readback-b35/` y `final-readback-b35/`. No se escribieron carpetas de Pokémon Ópalo/Z, configuración, partidas ni otros archivos de la consola.

## Prueba física pendiente

En b35: abrir Ópalo, comprobar texto y barras del menú sin reflejo horizontal y con Y correcto, confirmar «Volver a la biblioteca» y comprobar si aparece la biblioteca FangameNX. Luego verificar el atajo y el remapeo individual, y que Ópalo mantiene la velocidad normal. **La salida no se declara corregida en hardware todavía.** Si falla, copiar `mkxp-boot.log` antes de otro arranque: las marcas `[quick]` sincronizadas delimitan confirmación, cierre del overlay y llegada al binding Ruby. Siguen abiertas la X intermitente y la incidencia anterior de pánico táctil.
