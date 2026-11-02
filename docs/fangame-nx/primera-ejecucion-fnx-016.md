# Misión 008 · primera ejecución en Switch 1

Fecha: 2 de noviembre de 2026. Rama `test/fnx-016-first-boot`, desde M007 (`0bd7393`). Estado actual: **paquete copiado y verificado en microSD; arranque físico pendiente del propietario**. No se ha ejecutado el NRO ni se ha observado todavía Ruby, gráficos, logs o salida al menú.

## Identificación y comprobación previa de la microSD

Windows mostró una única unidad de tipo extraíble: `E:`, etiqueta `SWITCH SD`, FAT32, 172 083 904 512 bytes totales y 8 036 286 464 bytes libres antes de la copia. `Get-Partition -DriveLetter E | Get-Disk` indicó disco 2 por USB, en línea, saludable y sin protección de escritura. `E:\switch\`, `E:\atmosphere\`, `E:\bootloader\` y `E:\emuMMC\` existían. `E:\switch\fnx004-sintetico\` **no existía**. Las otras unidades visibles eran discos fijos o unidades virtuales, no otra microSD extraíble. No se formateó, reparó ni particionó la unidad.

Se validó el origen `C:\Users\pablo\Documents\GitHub\FangameNX\local\fnx016-first-boot\switch\fnx004-sintetico\` contra `MANIFEST.sha256.txt` (SHA-256 `da7eb4e9ea0e29e5fbe62fcc3bf2cd781a212f7cd60d8ebc5c3d8e2fb463b7fa`). Había exactamente los ocho archivos, las carpetas `Audio/`, `Data/`, `Graphics/`, `lib/`, ningún archivo adicional ni enlace de sistema de archivos. Los ocho tamaños y SHA-256 coincidieron; total 17 770 204 bytes. El NRO tenía SHA-256 `33bccb3327406509efb13583ac0cc9ad86ee4f874ccc433fdb7df559193e05c7`. El [inventario completo](preparacion-primer-arranque-fnx-016.md#contenido-exacto-del-payload) es el de M007.

## Copia autorizada y verificación en destino

Se volvió a comprobar que `E:` conservaba etiqueta, tipo, sistema de archivos, disco, espacio y capacidad de escritura, y que el destino seguía ausente. Se creó **solo** `E:\switch\fnx004-sintetico\` y, dentro, `Audio/`, `Data/`, `Graphics/` y `lib/`. Los ocho archivos se copiaron individualmente con `System.IO.File.Copy(origen, destino, false)`, que rechaza la sobrescritura. No se copió `work/`, el manifiesto, `__pycache__`, logs de desarrollo, NSP ni material de juegos.

La lectura posterior desde `E:` encontró **8 archivos y 4 directorios**, sin elementos inesperados. Se recalcularon allí los ocho SHA-256 y tamaños: **0 diferencias** frente al manifiesto. El NRO instalado conserva SHA-256 `33bccb3327406509efb13583ac0cc9ad86ee4f874ccc433fdb7df559193e05c7`. Quedaron 8 017 805 312 bytes libres. Se compararon, antes y después, los metadatos de las demás entradas directas de `E:\switch\` y de las carpetas principales `atmosphere`, `bootloader` y `emuMMC`: no cambiaron. Esta comprobación de metadatos no es una auditoría recursiva de todo el contenido de la tarjeta; el comando de copia solo abrió destinos dentro de la nueva carpeta.

No se modificaron archivos de configuración global, Atmosphère, Hekate, sysNAND, emuMMC ni otras aplicaciones. No se generó ni instaló NSP. No se ha publicado el binario.

## Arranque manual pendiente

La instalación está en `E:\switch\fnx004-sintetico\` **en este PC**; al volver a insertar la tarjeta en la consola equivale a `SD:/switch/fnx004-sintetico/`. El propietario debe expulsar `E:` desde Windows, introducir la microSD con la Switch apagada y arrancar su configuración habitual de Atmosphère. Desde HOME, abrir Homebrew Menu mediante *title override* con memoria de aplicación (normalmente mantener **R** al abrir un juego o aplicación ya existente hasta que aparezca hbmenu, según la [configuración oficial de Atmosphère](https://github.com/Atmosphere-NX/Atmosphere/blob/master/docs/features/configurations.md); si la configuración propia usa otra tecla, respetarla). En hbmenu, seleccionar `switch/fnx004-sintetico/fnx004-sintetico.nro`, visible como **FangameNX FNX-016**. No usar el modo applet de Álbum para esta prueba.

Expectativa del fixture, todavía **no observada**: posible carga «RGSS switch», bandas magenta y verde, texto opcional «FNX-004 RUBY OK», lectura de `entrada.txt`, creación de `fnx004-probe.log` y `fnx004-escritura.txt`, y salida tras 480 llamadas a `Graphics.update`. Si se bloquea, el propietario puede intentar HOME y cerrar el programa; si la consola no responde, apagarla manteniendo POWER al menos 20 segundos, conforme a la [guía de Nintendo](https://en-americas-support.nintendo.com/app/answers/detail/a_id/22523/p/897/c/990). No retirar la microSD con la consola encendida.

## Evidencias de ejecución por completar

| Nivel | Resultado real |
|---|---|
| Copia y SHA-256 en microSD | **Verificado**: ocho archivos coinciden. |
| Aparición y selección en Homebrew Menu | Pendiente de observación del propietario. |
| Llegada a `main()` y revisión previa | Pendiente; buscar `mkxp-boot.log`. |
| Evaluación de `probe.rb` | Pendiente; buscar `FNX004 RUBY INICIADO` en `fnx004-probe.log`. |
| Lectura de `entrada.txt` | Pendiente; buscar `lectura=OK`. |
| Escritura y relectura | Pendiente; buscar `escritura=OK` y `fnx004-escritura.txt`. |
| Dibujo, salida o recuperación | Pendiente de descripción o foto del usuario. |
| Errores observados | Ninguno observado aún: no se ha arrancado el NRO. |

Si el arranque falla, solicitar el mensaje, una foto cuando ayude y los archivos **que existan** en `SD:/switch/fnx004-sintetico/`: `mkxp-boot.log`, `mkxp-debug.log`, `mkxp-ruby.log`, `errorlog.txt`, `fnx004-probe.log` y `fnx004-escritura.txt`. Si no se reconoció la carpeta, revisar `SD:/rgss-switch.log`; si aparece un panel de Atmosphère, conservar los informes disponibles en `atmosphere/crash_reports/` o `fatal_errors/`. No sobrescribir otros registros ni repetir el ensayo sin diagnosticar un fallo.

**Estado de FNX-016:** instalación y verificación de integridad completas; primera ejecución en Switch 1 **pendiente**. No se declara la prueba superada.
