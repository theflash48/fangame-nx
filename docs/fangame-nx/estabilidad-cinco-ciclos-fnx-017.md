# Misión 012 · cinco ciclos de estabilidad FNX-017

Rama `test/fnx-017-five-cycles`, desde M011 `83b5491`. El propietario autorizó únicamente la **lectura** de logs del fixture ya instalado `SD Card/switch/fnx016-sdl-fix/` mediante WPD/MTP. No se usan letras de unidad ni se accede a otros dispositivos. La prueba física la realiza el propietario.

## Preparación y línea base

El protocolo de M011 está en [`estabilidad-multifixture-fnx-017.md`](estabilidad-multifixture-fnx-017.md). El script Ruby del fixture abre `fnx009-probe.log` en modo `a` y `fnx009-escritura.txt` en `ab`. El código de `mkxp-boot.log` usa `O_APPEND` con rotación a 1 MiB; `mkxp-debug.log` usa `fopen(..., "a")`. Los cuatro logs se acumulan entre arranques. Cada copia local se conserva en una carpeta diferente, sin sobrescribir la evidencia M016.

El 10 de noviembre de 2026 se identificó **un único** dispositivo WPD `Nintendo Switch`, VID `057E`, PID `201D`, y dentro de él `SD Card/switch/fnx016-sdl-fix/` con los cuatro registros esperados. Se copiaron en modo lectura a `local/fnx017-stability-20261110/baseline/`, con `manifest.json` local. Los hashes locales coincidieron con la evidencia M016 del 2 de noviembre; esto comprueba la igualdad de las **copias obtenidas**, no constituye una lectura byte a byte del remoto original.

| Registro inicial | Bytes | SHA-256 local |
|---|---:|---|
| `fnx009-probe.log` | 332 | `39c7caf1601c176c2e7b1cade8ef551f74689786a63f5b0632ae32204f501ac9` |
| `fnx009-escritura.txt` | 20 | `f715634853244f18c30bfb378193bf828ab65b41c45c7eeff61b27aa1a7341d3` |
| `mkxp-boot.log` | 4 043 | `42e12661cc0a3f99d15cc9eeccec1dda2bf19e96112238344e22c400dbcd01da` |
| `mkxp-debug.log` | 888 | `76095ba9983a4b8e8e9338bddc3459093a5dbaaf0eb6afc6a8b650d68e06a8a1` |

El procedimiento local `local/fnx017-stability-20261110/copy-mtp-logs.ps1` exige el mismo dispositivo WPD y la ruta exacta; copia solo estos cuatro archivos, espera a que cada copia local estabilice su tamaño, calcula SHA-256 y rechaza carpetas locales preexistentes. No invoca operaciones de escritura sobre el objeto MTP. El script y las copias están excluidos de Git.

## Captura inicial que exige aclaración

El propietario comunicó `LECTURA OK`, A +1, X con cambio azul/verde y B con retorno rápido y autónomo al Homebrew Menu. Se recuperaron cuatro registros en `local/fnx017-stability-20261110/ciclo-01/`, con hashes en su `manifest.json`. Sin embargo, el sufijo desde la línea base contiene **tres** marcas `[build]`, **dos** bloques Ruby completos y **dos** cierres con `SDL_DestroyWindow OK` y `[exit] 08 before return main`. La primera marca de arranque indica modo *applet* con memoria reducida (248 MB de montón) y aborta durante la revisión de arranque, antes de Ruby. Los dos arranques siguientes indican memoria de aplicación y terminan normalmente. El primer bloque Ruby registra siete A y ninguna X; el segundo registra cuatro X y cuatro A. Ambos incluyen lectura/escritura OK y salida por B.

| Copia local mixta | Bytes | SHA-256 local |
|---|---:|---|
| `fnx009-probe.log` | 937 | `02fd3e93ecafec1af372d8c6d0238f97c6fef8f8bff68ea4312b70902526d9c2` |
| `fnx009-escritura.txt` | 60 | `60a38dbaecdc5660c04b3ae611bdab9da8bb5e80f5e6fdb84b7214ca72da4b07` |
| `mkxp-boot.log` | 13 386 | `39d14673933fa848113681f0e66d232ac6d0fcfac283916af3d6f70a91484e6c` |
| `mkxp-debug.log` | 3 064 | `208ba353df2b7352b828a11ff19c3610c44d6fd621ae0afa642645f2b0509692` |

Por tanto, esta carpeta es una **captura mixta de varios intentos**, no una evidencia aislada de un único ciclo A→X→B. `verificar-ciclos-fnx-017.py --expected 1` rechazó la comparación: `bloques Ruby/boot/debug=(2, 3, 3), esperados=1`. El propietario confirmó que abrió la aplicación más de una vez y pulsó A/X varias veces. No se atribuye sin más un bloque concreto al resultado visual comunicado. La captura permanece íntegra, sin alterar los originales MTP ni las copias locales.

Para empezar una serie contable se creó `local/fnx017-stability-20261110-serie/` y se copiaron **solo localmente** los cuatro archivos de la captura mixta a su `baseline/`. Los cuatro hashes coinciden con la captura anterior. `copy-mtp-logs.ps1` se copió a la raíz de esta nueva serie; guardará `ciclo-01` a `ciclo-05` sin sobrescribirlos. La nueva línea base no requiere otra lectura MTP ni modifica la Switch.

## Resultados por ciclo

Para cada ciclo de la **serie controlada**, el propietario abre la aplicación **una sola vez con memoria de aplicación**, observa `LECTURA OK`, pulsa A, X y B **una vez cada uno** y comunica si vuelve al Homebrew Menu sin HOME. Después abre USB File Transfer; se guardan cuatro copias locales en `local/fnx017-stability-20261110-serie/ciclo-NN/`. La verificación compara cada instantánea con la anterior mediante [`verificar-ciclos-fnx-017.py`](verificar-ciclos-fnx-017.py) y exige exactamente un nuevo bloque Ruby, boot y debug, los eventos A/X/B, lectura/escritura OK, `SDL_DestroyWindow OK` y la marca final de `main`. El resultado visual no se infiere de los logs.

| Ciclo | Pantalla y lectura observadas | A / X observados | B y regreso propio | Logs, hashes y validación | Anomalías |
|---:|---|---|---|---|---|
| 1 | `LECTURA OK`; pantalla sin anomalías comunicadas | A +1; X azul→verde | B volvió por sí solo al Homebrew Menu | `ciclo-01/`, cuatro copias locales; verificador `--expected 1` OK | Ningún error visible comunicado |
| 2 | `LECTURA OK`, según confirmación general del propietario | A +1; X azul→verde, según confirmación | B volvió solo al Homebrew Menu, según confirmación | `ciclo-02/`, cuatro copias locales; verificador `--expected 1` OK | Ningún error visible comunicado |
| 3 | `LECTURA OK`, según la pauta realizada y el log | A +1; **X no cambió el color** | B volvió solo al Homebrew Menu | `ciclo-03/`, cuatro copias locales; verificador rechazó la ausencia de X | Fallo de entrada X observado y confirmado por log |
| 4 (diagnóstico adicional) | Ruby registró `pantalla=inicial`; resultado visual no comunicado | Sin eventos A/X en el log | Sin evento B ni marcas de salida | Cuatro logs y AFE2 recuperados por MTP | Pánico de Atmosphère; reinicio forzado; causa sin determinar |
| 5 (prueba final tras reinicio) | `LECTURA OK` y espera inicial sin fallo | A +1; X azul→verde tras pulsación mantenida ~1 s | B volvió solo al Homebrew Menu | `ciclo-05/`, cuatro copias locales; verificador `--expected 1` OK | Ningún error visible comunicado |

La serie de cinco ciclos **consecutivos correctos** se detuvo tras el ciclo 3 por el fallo de X. A petición posterior del propietario se intentó un cuarto ciclo diagnóstico, que acabó en un pánico de Atmosphère y reinicio forzado. Tras volver a conectar la Switch, se recuperaron los cuatro logs del cuarto intento y el informe fatal por MTP, sin modificar los originales. El propietario pidió **una sola prueba final tras reiniciar**, que funcionó y cuyos logs se conservaron como `ciclo-05/`. El balance es **tres pruebas satisfactorias (1, 2 y 5), una con fallo de X (3) y una interrumpida por pánico (4)**. No se acredita estabilidad de cinco ciclos seguidos. El paquete dual M011 no se instaló ni probó.

### Evidencia del ciclo 1 controlado

El propietario confirmó exactamente **un** lanzamiento con memoria de aplicación, A/X/B una vez cada uno, retorno autónomo y cero errores visibles. Las cuatro copias MTP en `local/fnx017-stability-20261110-serie/ciclo-01/` son prefijos extendidos de la nueva línea base. El sufijo contiene una marca de inicio Ruby, `lectura=OK`, `escritura=OK`, `boton=A contador=1`, `boton=X color=verde`, `boton=B salida_solicitada`, `FNX009 RUBY COMPLETADO; salida normal`, un bloque de arranque y cierre con `SDL_DestroyWindow OK` y `[exit] 08 before return main`, y un bloque debug terminado en `Shutting down.`. El marcador de escritura creció 20 bytes. `verificar-ciclos-fnx-017.py baseline ciclo-01 --expected 1` terminó con código 0.

| Archivo en `ciclo-01/` | Bytes | SHA-256 local |
|---|---:|---|
| `fnx009-probe.log` | 1 135 | `30a4ee1fc9b11614bfc17120f01ad2666d119397d201b5b1423f7dc29ea9e57c` |
| `fnx009-escritura.txt` | 80 | `029bc3081246af581c2d5fcaa08d4fd1bfbf77334a3ca28a33ff0fc9ccedf149` |
| `mkxp-boot.log` | 17 429 | `875009f85754222bad350392731ce8e4dd0f9b48073e8ec29a5f3f71da334ace` |
| `mkxp-debug.log` | 4 007 | `89f4cee6e060a908d5920b3f6cf1d785d98ec29e24b0378091babd95f8d43fd0` |

### Evidencia del ciclo 2 controlado

El propietario respondió «todo perfe» a la pauta completa de un lanzamiento, lectura, A/X/B y regreso autónomo. El sufijo de `ciclo-02/` respecto a `ciclo-01/` contiene exactamente una ejecución Ruby con `lectura=OK`, `escritura=OK`, `boton=A contador=1`, `boton=X color=verde`, B y finalización normal; el boot contiene `SDL_DestroyWindow OK` y `[exit] 08 before return main`, y debug termina en `Shutting down.`. Los cuatro prefijos se conservaron y el marcador de escritura creció 20 bytes. El verificador terminó con código 0.

| Archivo en `ciclo-02/` | Bytes | SHA-256 local |
|---|---:|---|
| `fnx009-probe.log` | 1 333 | `ccd923a7aa3f9cd3afb46b232811e95765e5ce7a1e76ec735cd0fc102b35efe8` |
| `fnx009-escritura.txt` | 100 | `9b611712e0a5b046abea810b7920d1df26dd92d36bf277fff1c737ad29aaff82` |
| `mkxp-boot.log` | 21 472 | `fcf0fa87e05814d2ca63e62697b35226961068dd22ac72ecac41a214d8306772` |
| `mkxp-debug.log` | 4 950 | `9fcd362e1d5ba7bb1a3e7a1eadcfa155c1d549fdcca8f641e7d3d9728a7ee219` |

### Evidencia del ciclo 3 y parada

El propietario indicó que X **no cambió el color**; A, B y el resto funcionaron, con regreso al Homebrew Menu. Se copiaron los cuatro registros a `local/fnx017-stability-20261110-serie/ciclo-03/`. Frente a `ciclo-02/` hay exactamente un nuevo inicio Ruby, un nuevo arranque con memoria de aplicación y un bloque debug. `fnx009-probe.log` registra `lectura=OK`, `escritura=OK`, `boton=A contador=1`, `boton=B salida_solicitada` y `FNX009 RUBY COMPLETADO; salida normal`; **no registra `boton=X`**. `fnx009-escritura.txt` creció 20 bytes. El boot llega a `SDL_DestroyWindow OK` y `[exit] 08 before return main`; debug llega a `Shutting down.`. Los cuatro registros posteriores conservan íntegros los anteriores como prefijo.

`verificar-ciclos-fnx-017.py ciclo-02 ciclo-03 --expected 1` devolvió código 1: `Ruby ciclo 1: falta 'boton=X color=verde'`. Esa es una **detección correcta** de un criterio incumplido, no un fallo del analizador. La traza Ruby solo demuestra que el flanco de X no llegó a la rama `Input.trigger?(Input::A)`; no registra eventos SDL crudos y no permite atribuir la causa a mando, duración de la pulsación, sondeo de `Input` o mapeo. El código local mapea X físico de Switch a `SDL_CONTROLLER_BUTTON_Y` y `Input::A`; el mismo mapeo funcionó en los ciclos 1 y 2. `Input.trigger?` requiere una transición de no pulsado a pulsado. No se cambia el runtime durante esta misión.

Anomalía secundaria: el sufijo de `mkxp-debug.log` del ciclo 3 tiene 752 bytes frente a 943 en cada uno de los ciclos 1 y 2; faltan las líneas informativas `Backend`, datos GL y viewport, aunque `mkxp-boot.log` sí marca `initGL OK` y el propietario vio la pantalla. No hay evidencia para relacionar esta omisión con la entrada X. El aviso de FluidSynth ausente figura también en los ciclos 1 y 2 y no impidió su ejecución.

| Archivo en `ciclo-03/` | Bytes | SHA-256 local |
|---|---:|---|
| `fnx009-probe.log` | 1 511 | `b2e619854174714c7404763b99adfba34abaac636447fe8594a18dfbe9e0a907` |
| `fnx009-escritura.txt` | 120 | `4ead19fa1649a91adc4b8b2a60fa10c0f1540690ac3fc1e8b6023637160be6f8` |
| `mkxp-boot.log` | 26 283 | `677d0f3df0c5f838421f5dc783b52a1f1e2a99d3c7f6201a4220fdc6b77c8df8` |
| `mkxp-debug.log` | 5 702 | `3ca15d7a9dccb3449ccd7783833ee7cab01574cf27f56f5731bfc3f3e9558830` |

### Cuarto intento y parada por crash

Tras conocer el fallo de X, el propietario pidió un último ciclo diagnóstico con una pulsación de X mantenida aproximadamente un segundo. Comunicó un crash y después precisó que tuvo que **forzar el reinicio**, ya realizado. La pantalla mostró literalmente:

```text
Atmosphere panic occurred!
Tittle ID: 0100000000000034
Error: std::abort (0xFFE)
Report Saved to atmosphere/fatal_errors/report_9b251d017c.bin
```

Estas cadenas proceden de la observación del propietario. En la sesión siguiente indicó que ya había reconectado la Switch y pidió investigar. Una copia local del `.bin` y de los cuatro logs quedó en `local/fnx017-stability-20261110-serie/`, sin escribir en MTP. **No recuerda en qué momento apareció el pánico respecto a A, X o B.** El Title ID mostrado no identifica por sí solo la causa de la interrupción. No se atribuye el pánico al problema de X ni al cierre SDL sin evidencia.

Los cuatro logs de `ciclo-04/` conservan los de `ciclo-03/` como prefijo. El sufijo nuevo de `fnx009-probe.log` contiene `FNX009 INICIO`, `lectura=OK`, `escritura=OK` y `pantalla=inicial`, pero **ningún evento A/X/B ni `FNX009 RUBY COMPLETADO`**. `fnx009-escritura.txt` creció 20 bytes. `mkxp-boot.log` marca inicio con memoria de aplicación, `SharedState OK` y Ruby, y termina en `[mem] pico del monton 199 MB`, sin ninguna marca `[exit]`; `mkxp-debug.log` termina tras iniciar el motor y tampoco registra `Shutting down.`. `verificar-ciclos-fnx-017.py ciclo-03 ciclo-04 --expected 1` devuelve código 1: falta `boton=A contador=1`. Los logs sitúan la interrupción **después de dibujar la pantalla inicial en Ruby y antes de cualquier entrada registrada**, sin demostrar cuándo pulsó el propietario.

| Archivo recuperado después del pánico | Bytes | SHA-256 de la copia local |
|---|---:|---|
| `ciclo-04/fnx009-probe.log` | 1 606 | `8521fa4eebc1bd51700903cc290b94a230e4e54e0b57362d5d60f0b1d3d93d30` |
| `ciclo-04/fnx009-escritura.txt` | 140 | `a7010fd06bbf463db32862c9669cc7de7a5c9f974e2e662879f9cb36838e17ce` |
| `ciclo-04/mkxp-boot.log` | 29 776 | `8efc62774321fb8569d6661ce1d833c88974fc53ac4e5cf85945f4a58047cf3e` |
| `ciclo-04/mkxp-debug.log` | 6 556 | `f21099b0ecf73acea94edc3acab27593df05712f12b1b0c407aba4c3d3cf34be` |
| `fatal-report/report_9b251d017c.bin` | 1 104 | `806ee5b429f57174d42d48f202946c175dbca73b3a9219b70571bdb12ea1bf06` |

La copia AFE2 empieza con `AFE2`, código `0xFFE` y Title ID `0100000000000034`; el identificador interno coincide con `9b251d017c` del nombre. Hay PC `0x580b020598`, base de módulo `0x580b000000` y cuatro entradas de traza declaradas, pero no se dispone de símbolos del módulo para interpretarlas. El formato se contrastó con la [estructura de Hekate](https://github.com/CTCaer/hekate/blob/master/bootloader/hos/secmon_exo.c). El [árbol oficial de Atmosphère](https://github.com/Atmosphere-NX/Atmosphere/blob/master/atmosphere.mk) asigna `0100000000000034` a su módulo `fatal`; un [comentario de un colaborador de Atmosphère](https://github.com/Atmosphere-NX/Atmosphere/issues/816) describe ese ID en otro caso como un fallo durante la presentación de un error fatal. **Inferencia:** este reporte puede ser secundario y no revelar el error inicial de FangameNX o del sistema. No prueba que FangameNX haya llamado a `std::abort` ni señala una corrección del runtime.

### Prueba final después del reinicio

El propietario realizó un último lanzamiento del **mismo NRO sin cambios**: esperó cinco segundos en `LECTURA OK`, pulsó A una vez, mantuvo X aproximadamente un segundo, pulsó B y comunicó que todo funcionó y volvió al Homebrew Menu. Los cuatro logs de `ciclo-05/` conservan los de `ciclo-04/` como prefijo. El sufijo registra exactamente una ejecución con `lectura=OK`, `escritura=OK`, `boton=A contador=1`, `boton=X color=verde`, `boton=B salida_solicitada`, finalización Ruby, `SDL_DestroyWindow OK` y `[exit] 08 before return main`. `verificar-ciclos-fnx-017.py ciclo-04 ciclo-05 --expected 1` terminó con código 0.

| Archivo en `ciclo-05/` | Bytes | SHA-256 de la copia local |
|---|---:|---|
| `fnx009-probe.log` | 1 804 | `28ab6cdfcceec10199039e2ffb458da0805b17b698f200f72dde92df4814a62e` |
| `fnx009-escritura.txt` | 160 | `0bfa66e7b8ba3ad9ad2b8e050b7aa42f89b5237cfb8a3a270b997f2bdfceb34d` |
| `mkxp-boot.log` | 34 001 | `487da8849ba5e3710342914e8f5e20ee995bd5978aeb180956d03e0cdc5079e5` |
| `mkxp-debug.log` | 7 499 | `b7f5081e6c65dedb0277f354189d0146514ec32f7c99300ad18089918725ff1f` |

`mkxp-boot.log` informa un pico de montón de **199 MB en cada uno de los cinco lanzamientos** de esta serie, incluidos el tercero y el interrumpido. No se observa una tendencia creciente en esa métrica de cada proceso. No mide toda la memoria del sistema ni permite descartar otras fugas o fallos de Atmosphère. El origen del evento X omitido y del pánico sigue sin confirmar.

**Siguiente tarea recomendada:** avanzar hacia un primer lanzador genérico de juegos externos según [`ADR-004-lanzador.md`](ADR-004-lanzador.md), usando estos resultados como límite de confianza. El propietario pidió dejar de repetir mini pruebas; una nueva implementación deberá definir criterios de verificación relevantes para su arquitectura y conservar el diagnóstico pendiente de X/pánico como riesgo, sin atribuirles una corrección improvisada.
