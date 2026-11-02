# FNX-016 · diagnóstico de la salida con B

Rama `debug/fnx-016-shutdown`, creada desde `test/fnx-016-interactive` (`bff1003`). La instalación M009 y su NRO quedan conservados. Esta investigación usa exclusivamente el dispositivo WPD/MTP `Nintendo Switch` de Nintendo por USB y la carpeta `SD Card/switch/fnx009-interactivo/`; no se enumeraron ni abrieron volúmenes convencionales ni la tarjeta del lector ajeno.

## Evidencia recuperada

Tras la autorización expresa del propietario se copiaron, sin modificar los originales, cuatro archivos a `local/m009-evidencias-mtp-20261102-045741/`, carpeta ignorada por Git. Las copias son legibles. MTP no ofrece un tamaño remoto fiable; los hashes siguientes corresponden a las copias locales y no constituyen una comparación directa con los originales.

| Copia local | Bytes | SHA-256 |
|---|---:|---|
| `fnx009-probe.log` | 825 | `5b552b1d5247f174a25b768c2873260ddb00e96c21d5dd5532dad91b0df57caf` |
| `fnx009-escritura.txt` | 40 | `77e323838545eb255d35cdaca605d962009ed580d6d6af95aa70665cc3ebbe67` |
| `mkxp-debug.log` | 2 224 | `30c07a322aa70862d677f3b3ec8a94bfabe1f2d4808d8cf83e237c8ccacb8f68` |
| `mkxp-boot.log` | 8 706 | `c0992763447da3b67ddf335281af8f05ec037c947c9c3204d83a91368b3e448e` |

`fnx009-probe.log` contiene **dos ejecuciones**. Cada una registra `lectura=OK`, `escritura=OK`, pulsaciones A y X, `boton=B salida_solicitada` y después `FNX009 RUBY COMPLETADO; salida normal`. Por tanto, en ambas el último `Graphics.update` de la rama B devolvió el control al script y se alcanzó la línea posterior al bucle. La presencia de dos marcadores `FNX009-ESCRITURA-OK\n` en `fnx009-escritura.txt` demuestra persistencia de ese archivo entre ejecuciones; no se deduce de ello la terminación correcta del proceso.

`mkxp-debug.log` registra dos veces `EventThread termination requested`, `RGSS thread ack'd request after 0 ms` y `Shutting down.`. Esta última marca está en `src/mkxp-z/src/main.cpp` **después** de `SDL_WaitThread(rgssThread, 0)`, de liberar el contexto GL y de `eventThread.cleanup()`, pero **antes** de `alcCloseDevice`, `SDL_DestroyWindow`, `Sound_Quit`, `TTF_Quit`, `IMG_Quit`, `SDL_Quit` y el retorno de `main`. Así, el bloqueo visual observado por el propietario no se atribuye ya al `Input`, al último `Graphics.update`, a la salida del script ni a `ruby_cleanup(0)`: el motor avanzó más allá de ellos. Los registros actuales no distinguen qué llamada posterior se detiene ni si `main` retorna y falla el retorno al Homebrew Menu. No se ha inspeccionado una traza de pila.

`mkxp-boot.log` confirma la selección de `sdmc:/switch/fnx009-interactivo` mediante `argv[0]`, carga de RGSS1 y dos arranques. No contiene marcadores de las llamadas de cierre porque la compilación M006/M009 no los emite. El mensaje `Failed to load libfluidsynth.so. Midi playback is disabled.` aparece en ambos arranques, pero no hay evidencia de que intervenga en el bloqueo de salida.

## Próximo diagnóstico mínimo

Se propone **solo instrumentar** `src/mkxp-z/src/main.cpp` con marcadores `nx_boot_trace` inmediatamente antes del primer paso y después de cada función del tramo de cierre enumerado arriba, incluido el punto previo al retorno de `main`. `nx_boot_trace` ya está usado en ese archivo y escribe en `mkxp-boot.log`; la instrumentación no altera el orden ni la semántica de las llamadas. El objetivo es que una única ejecución futura deje constancia del último paso completado. Se conservará el cambio como parche versionado, al final de `patches/ORDEN.txt`, para poder reproducirlo y revertirlo.

Antes de recompilar se preservarán el ELF, NRO y mapa de la compilación M006. El nuevo enlace seguirá usando la `libgcc` aislada Cortex-A57, con auditoría de opcodes y de símbolos. Se preparará un paquete sintético nuevo y separado; no se sobrescribirá M009 ni sus logs. No se declarará resuelto el cierre por el mero hecho de compilar o de ver las marcas previas a la llamada que falla.

## Instrumentación y verificación local

Se añadió [`mkxpz-shutdown-traza.patch`](../../patches/mkxpz-shutdown-traza.patch) al final de `patches/ORDEN.txt`. El parche deja intactas las llamadas de cierre y escribe ocho marcas en `mkxp-boot.log`: antes de `alcCloseDevice`, después de cada una de `alcCloseDevice`, `SDL_DestroyWindow`, `Sound_Quit`, `TTF_Quit`, `IMG_Quit`, `SDL_Quit`, y justo antes de retornar de `main`. No altera Ruby, RGSS, SDL, OpenAL ni el comportamiento del script. Se aplicó sobre la fuente local y una segunda aplicación de ensayo sobre la copia preservada produjo el **mismo SHA-256** de `main.cpp` (`d33438ca799784ddf3660b9d97448d541172bc18152a1475e60f9d9688624fe0`). El original (`f3d30457bd3850b05b062d390c892d1e04e653b1df4699bc0ebb2a60db190050`) permanece en `local/fnx016-shutdown-diagnostic/before/`.

Antes de compilar se preservaron, con hashes coincidentes, el ELF M006 (`cf52e3b7bc5747dff78c3d4339886c1d27e3de1cb145ccc25a920c25fdfe30ce`), su NRO (`9416eaaf5ea40351f0bf991ba2b43d09cde1540c8c3a19cb590f27f38f384ea7`) y el mapa (`77f64254eb2f743cefcfc0fb993cec7e5803c493f910e506d3e47371fdf6d7e5`). El nuevo `make -C mkxp-z-nx -j6 FNX_LIBGCC_PREFIX=<prefijo local Windows> FNX_LINK_MAP=<mapa local>` terminó con código **0**. Fue una **compilación incremental** de `main.cpp` y `nx_boot.c`, seguida de enlace y generación NRO; no se presenta como build limpio. `fixup-arm64.py` se omitió como en M006. Salida íntegra: `local/fnx016-shutdown-diagnostic/build.log`.

| Artefacto diagnóstico local | Bytes | SHA-256 |
|---|---:|---|
| `mkxp-z-nx/rgss-switch.elf` | 88 855 112 | `5daa3262e5ca760efbbdfae1deebd5430ed3ae72d4427184d7f9693f0df50af5` |
| `mkxp-z-nx/rgss-switch.nro` (NACP del motor) | 17 750 457 | `2da036d8d00183cf7d631ee4ac27d600275183f2127f5e91029a4c8fda82c410` |
| `local/fnx016-shutdown-diagnostic/shutdown.map` | 16 343 993 | `71f9e3ac7b7703f4f38557e8d8c2e77c984e01375336425940b87164dc98dfa1` |
| `local/fnx016-shutdown-diagnostic/switch/fnx016-exit-trace/fnx016-exit-trace.nro` | 17 749 502 | `6756fa918b326369497ca969d0425a601fdad1f60db6984137e6f0818b602557` |
| `local/fnx016-shutdown-diagnostic/MANIFEST.sha256.txt` | 647 | `63d3237078d00f974eb2ddfc3b2517aae02d38117c97a83e0f1b5f88d26fdf87` |

El mapa incorpora **ocho** miembros de la `libgcc` aislada Cortex-A57 y ninguno de la global. `nm -u` da **0** símbolos indefinidos. `auditar-armv8.py` encuentra **0** patrones incompatibles conocidos y compara 11 713 720 bytes de código ELF/NRO sin diferencias; LLVM `llvm-objdump 21.1.8 -d --mcpu=cortex-a57` informa **0** instrucciones `<unknown>`. `comprobar-nro.py` confirma `NRO0`, ASET v0, NACP «FangameNX EXIT TRACE», icono neutro y RomFS `/LEEME.txt`. El cuerpo de 17 719 296 bytes del NRO empaquetado es idéntico al del NRO recién enlazado: solo cambia ASET. El `interactive.rb` reutilizado pasó `ruby -c` (`Syntax OK`). Estas verificaciones son estáticas; la nueva ruta de cierre no se ha ejecutado en Switch.

## Instalación de diagnóstico y siguiente ensayo

Con la autorización amplia posterior del propietario, se identificó otra vez **solo** la Switch de clase WPD/MTP por USB y se confirmó la presencia de la instalación M009. Se comprobó que `SD Card/switch/fnx016-exit-trace/` no existía, se creó únicamente esa carpeta y se copiaron los ocho archivos del payload local y cuatro subdirectorios (`Audio/` y `Graphics/` vacíos). La enumeración de esa carpeta nueva confirmó la estructura exacta. Se recuperó por MTP cada uno de los ocho archivos a `local/fnx016-shutdown-diagnostic/readback/`: **8/8 SHA-256 coinciden** con `MANIFEST.sha256.txt`, incluido el NRO `6756fa918b326369497ca969d0425a601fdad1f60db6984137e6f0818b602557`. No se accedió a unidades extraíbles convencionales ni a la tarjeta del lector ajeno; M009 y M008 no se tocaron. No se generó NSP ni se ejecutó esta variante.

Para el ensayo manual, salir normalmente de USB File Transfer, iniciar Homebrew Menu con memoria de aplicación según la configuración habitual y seleccionar **FangameNX EXIT TRACE** en `switch/fnx016-exit-trace/`. El script sigue mostrando el texto `FangameNX - FNX-009` porque el código de prueba se reutilizó sin cambios; los logs que genere estarán **dentro de la carpeta nueva**, no en la instalación M009. Comprobar pantalla inicial y pulsar B **una sola vez**. Esperar unos segundos y anotar si vuelve al menú o permanece en `SALIENDO...`; si queda inmóvil, usar HOME para cerrar como en la prueba anterior. No repetir antes de conservar el log de esta ejecución. La última marca `[exit] NN ...` en `mkxp-boot.log` permitirá acotar la llamada de cierre; si aparece `08 before return main` y la pantalla sigue inmóvil, habrá que estudiar la salida posterior de `main` en libnx/homebrew.

**Estado:** salida Ruby y terminación del hilo RGSS demostradas por los dos registros M009; bloqueo visible situado después de `Shutting down.` pero sin función culpable identificada. Variante diagnóstica construida, auditada, instalada y verificada; **prueba física diagnóstica pendiente**. No se declara resuelto FNX-016.
