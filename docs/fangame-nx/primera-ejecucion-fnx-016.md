# Misión 008 · primera ejecución en Switch 1

Fecha: 2 de noviembre de 2026. Rama `test/fnx-016-first-boot`, desde M007 (`0bd7393`). Estado actual: **copia verificada y primera ejecución en Switch 1 comunicada por el propietario**. Hay observación visual de Ruby y renderizado; faltan los logs, la persistencia de archivos y una salida iniciada por el runtime. Esta actualización se basa en el relato del propietario, sin fotografía ni lectura de la microSD. Desde esta actualización la microSD **no está disponible y su acceso está prohibido hasta una nueva autorización explícita**; no se ha buscado, detectado ni accedido a ninguna unidad extraíble.

## Identificación y comprobación previa de la microSD

Windows mostró una única unidad de tipo extraíble: `E:`, etiqueta `SWITCH SD`, FAT32, 172 083 904 512 bytes totales y 8 036 286 464 bytes libres antes de la copia. `Get-Partition -DriveLetter E | Get-Disk` indicó disco 2 por USB, en línea, saludable y sin protección de escritura. `E:\switch\`, `E:\atmosphere\`, `E:\bootloader\` y `E:\emuMMC\` existían. `E:\switch\fnx004-sintetico\` **no existía**. Las otras unidades visibles eran discos fijos o unidades virtuales, no otra microSD extraíble. No se formateó, reparó ni particionó la unidad.

Se validó el origen `C:\Users\pablo\Documents\GitHub\FangameNX\local\fnx016-first-boot\switch\fnx004-sintetico\` contra `MANIFEST.sha256.txt` (SHA-256 `da7eb4e9ea0e29e5fbe62fcc3bf2cd781a212f7cd60d8ebc5c3d8e2fb463b7fa`). Había exactamente los ocho archivos, las carpetas `Audio/`, `Data/`, `Graphics/`, `lib/`, ningún archivo adicional ni enlace de sistema de archivos. Los ocho tamaños y SHA-256 coincidieron; total 17 770 204 bytes. El NRO tenía SHA-256 `33bccb3327406509efb13583ac0cc9ad86ee4f874ccc433fdb7df559193e05c7`. El [inventario completo](preparacion-primer-arranque-fnx-016.md#contenido-exacto-del-payload) es el de M007.

## Copia autorizada y verificación en destino

Se volvió a comprobar que `E:` conservaba etiqueta, tipo, sistema de archivos, disco, espacio y capacidad de escritura, y que el destino seguía ausente. Se creó **solo** `E:\switch\fnx004-sintetico\` y, dentro, `Audio/`, `Data/`, `Graphics/` y `lib/`. Los ocho archivos se copiaron individualmente con `System.IO.File.Copy(origen, destino, false)`, que rechaza la sobrescritura. No se copió `work/`, el manifiesto, `__pycache__`, logs de desarrollo, NSP ni material de juegos.

La lectura posterior desde `E:` encontró **8 archivos y 4 directorios**, sin elementos inesperados. Se recalcularon allí los ocho SHA-256 y tamaños: **0 diferencias** frente al manifiesto. El NRO instalado conserva SHA-256 `33bccb3327406509efb13583ac0cc9ad86ee4f874ccc433fdb7df559193e05c7`. Quedaron 8 017 805 312 bytes libres. Se compararon, antes y después, los metadatos de las demás entradas directas de `E:\switch\` y de las carpetas principales `atmosphere`, `bootloader` y `emuMMC`: no cambiaron. Esta comprobación de metadatos no es una auditoría recursiva de todo el contenido de la tarjeta; el comando de copia solo abrió destinos dentro de la nueva carpeta.

No se modificaron archivos de configuración global, Atmosphère, Hekate, sysNAND, emuMMC ni otras aplicaciones. No se generó ni instaló NSP. No se ha publicado el binario.

## Preparación para el arranque manual (registro anterior)

En el momento de la copia, Windows identificó la microSD como `E:` y el paquete quedó en `E:\switch\fnx004-sintetico\`, equivalente entonces a `SD:/switch/fnx004-sintetico/`. **No se presupone que esa letra siga asignada a la tarjeta.** La guía anterior indicaba expulsarla de Windows, insertarla con la Switch apagada, arrancar la configuración habitual de Atmosphère y abrir Homebrew Menu mediante *title override* con memoria de aplicación (normalmente mantener **R** al abrir un juego o aplicación ya existente hasta que aparezca hbmenu, según la [configuración oficial de Atmosphère](https://github.com/Atmosphere-NX/Atmosphere/blob/master/docs/features/configurations.md)). En hbmenu se debía seleccionar `switch/fnx004-sintetico/fnx004-sintetico.nro`, visible como **FangameNX FNX-016**, sin usar el modo applet de Álbum. Este párrafo registra las instrucciones dadas antes de la prueba; **no solicita conectar la microSD ahora**.

Antes del ensayo se esperaban la carga «RGSS switch», las bandas magenta y verde, el texto opcional «FNX-004 RUBY OK», lectura de `entrada.txt`, creación de `fnx004-probe.log` y `fnx004-escritura.txt`, y salida tras 480 llamadas a `Graphics.update`. El párrafo siguiente distingue lo que el propietario vio de lo que aún requiere logs. La guía anterior también describía HOME y, ante un bloqueo del sistema, mantener POWER al menos 20 segundos conforme a la [guía de Nintendo](https://en-americas-support.nintendo.com/app/answers/detail/a_id/22523/p/897/c/990).

## Primera ejecución comunicada por el propietario

El propietario informa que FangameNX **arrancó en Nintendo Switch 1 con Atmosphère**. Vio las bandas magenta y verde y el mensaje esperado centrado. Describe un área de renderizado aproximadamente 4:3, fondo azul marino y bandas negras laterales. Los botones que probó no produjeron cambios visibles. HOME respondió; HOME → X → Cerrar terminó inmediatamente sin error visible. No se ha recibido fotografía, duración del ensayo ni contenido de archivos de log.

El `mkxp.json` del fixture fija `customScript: "probe.rb"`, `rgssVersion: 1` y `fixedAspectRatio: true`. El motor selecciona `runCustomScript(customScript)` en `src/mkxp-z/binding/binding-mri.cpp`. Para RGSS 1, `src/mkxp-z/src/config.cpp` usa 640×480 cuando no se fijan otras dimensiones. `probe.rb` crea un `Bitmap` de 640×480, pinta fondo azul marino y dos bandas, las presenta mediante `Sprite` y dibuja `FNX-004 RUBY OK`. Lo observado es coherente con esa ruta, incluido el aspecto 4:3; no se midieron píxeles en la consola.

En el flujo del `probe.rb` **copiado y verificado por hash antes del ensayo**, el texto solo se intenta dibujar después de que `File.open("fnx004-probe.log", "w")` haya devuelto un archivo, `File.binread("entrada.txt")` haya devuelto exactamente `FNX004-LECTURA-OK\n`, y `File.write` seguido de `File.binread("fnx004-escritura.txt")` haya devuelto `FNX004-ESCRITURA-OK\n`. Por ello, si la pantalla procede de ese paquete, verla constituye **evidencia funcional indirecta** de ejecución Ruby, lectura del fixture y ciclo de escritura/relectura dentro del proceso. No prueba por sí sola que los datos persistieran después del cierre ni qué rutas absolutas se usaron. El texto visible también indica que `Bitmap.draw_text` y la fuente funcionaron en ese intento. No demuestra que se completaran las 480 llamadas a `Graphics.update`.

`probe.rb` no invoca `Input.update`, `Input.press?` ni `Input.trigger?`, y no contiene condiciones de botones: solo repite `Graphics.update` 480 veces. Por tanto, **no se esperaba una reacción visual** a los botones probados; este ensayo no evalúa el mapeo del mando. `lib/nx-compat.rb` y `lib/socket.rb` están presentes porque la revisión previa los exige, pero `preloadScript` está vacío y el fixture no hace `require`; su presencia no demuestra que se hayan ejecutado. `rubyLoadpath: ["lib"]` solo hace accesible la ruta de búsqueda. El shim `socket.rb` ofrece clases de sustitución y no participa en la prueba visual.

HOME → X → Cerrar es una acción del sistema que terminó el proceso sin error visible. **No es evidencia de que Ruby terminara el bucle, ejecutara `ensure` o devolviera el control de forma natural al Homebrew Menu.**

### Estado de las evidencias

| Nivel | Resultado real |
|---|---|
| Copia y SHA-256 en microSD | **Verificado**: ocho archivos coinciden. |
| Arranque en Switch 1 | **Observado por el propietario** bajo Atmosphère; no hay logs o foto todavía. |
| Llegada a `main()` y revisión previa | Inferida del dibujo del fixture; pendiente de `mkxp-boot.log`. |
| Evaluación de `probe.rb` | **Evidencia visual fuerte**: bandas y texto exclusivos del script; pendiente de `fnx004-probe.log`. |
| Lectura de `entrada.txt` | Inferida del orden del script; pendiente de marca `lectura=OK` en log. |
| Escritura y relectura | Inferidas del orden del script; persistencia tras cierre pendiente de log y archivo. |
| Renderizado | **Observado**: fondo azul marino, bandas magenta y verde, texto centrado; aspecto aproximadamente 4:3 con barras laterales. |
| Mando | Botones sin efecto visible; el fixture no los consulta. |
| Salida o recuperación | HOME respondió y HOME → X cerró sin error visible; salida propia del runtime **no verificada**. |
| Errores observados | Ninguno visible según el propietario; registros aún no consultados. |

**Rutas esperadas, no comprobadas ahora:** el directorio de trabajo elegido por el motor debería situar `fnx004-probe.log` y `fnx004-escritura.txt` en `SD:/switch/fnx004-sintetico/`; allí también podrían estar `mkxp-boot.log`, `mkxp-debug.log` y `errorlog.txt`. Si se hubiera cargado `nx-compat.rb`, `NX.log` intentaría `mkxp-ruby.log` allí y, si fallara, `SD:/rgss-switch-ruby.log`. En este fixture no hay una carga explícita del shim, así que **la ausencia de `mkxp-ruby.log` no sería un fallo por sí misma**. La reserva del log de arranque cuando no se reconoce la carpeta es `SD:/rgss-switch.log`, aunque el dibujo del fixture hace improbable ese caso. Un fallo del sistema podría dejar informes en `SD:/atmosphere/crash_reports/` o `fatal_errors/`. No se ha intentado recuperar, leer ni comprobar ninguno de estos archivos.

Cuando vuelva a existir autorización de acceso a la microSD, `fnx004-probe.log` permitiría buscar `lectura=OK`, `escritura=OK`, `texto=OK` y `FNX004 RUBY COMPLETADO`. El archivo `fnx004-escritura.txt` permitiría comprobar la persistencia y su contenido exacto tras cerrar. **No repetir el fixture actual antes de preservar esos dos archivos**: `File.open(..., "w")` y `File.write` los sobrescribirían en una nueva ejecución. Para una prueba futura más directa, un fixture propio podría mostrar **LECTURA OK** en pantalla tras comparar `entrada.txt`, mostrar un contador/color que cambie al detectar `Input::C` con `Input.update` y terminar al detectar `Input::B`, registrando las transiciones. La correspondencia A→`Input::C` y B→`Input::B` está documentada en `GUIA.md`, pero habría que verificarla en este caso. Esta es solo una **propuesta**: no se ha cambiado `probe.rb`, el runtime ni el paquete, y no se ha repetido el ensayo.

**Estado de FNX-016:** instalación verificada; **primer arranque, Ruby y dibujo observados por el propietario**. Persistencia de archivos, contenido de logs, controles y salida natural pendientes. No se declara la prueba integral superada.
