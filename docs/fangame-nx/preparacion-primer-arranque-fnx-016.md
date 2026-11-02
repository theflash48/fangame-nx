# Misión 007 · paquete para el primer arranque sintético

Fecha: 2 de noviembre de 2026. Rama `test/fnx-016-first-boot`, creada desde `fix/fnx-015-isolated-libgcc` (`f762847`). Esta misión termina con **preparación local**: no se ha copiado nada a la microSD ni ejecutado el NRO en Switch. No se ha modificado el runtime, el SDK, Atmosphère, Hekate, sysNAND o emuMMC.

## Resultado y procedencia

El paquete está en `C:\Users\pablo\Documents\GitHub\FangameNX\local\fnx016-first-boot\switch\fnx004-sintetico\`. Es local y está excluido de Git. Se recuperaron los cuatro ficheros propios de [`fixtures/fnx004/`](fixtures/fnx004/) y los dos shims propios de `runtime/lib/`, sin cambiar sus bytes. El marcador `Data/Scripts.rxdata` continúa vacío. El único NRO dentro del paquete se generó desde el ELF auditado en M006, con NACP de prueba y un icono geométrico nuevo creado en [`fixtures/fnx016/icono.svg`](fixtures/fnx016/icono.svg). La fuente SVG tiene licencia GPLv2 o posterior como el código propio del port; no usa imágenes ni fuentes externas.

El mapa de M006, SHA-256 `77f64254eb2f743cefcfc0fb993cec7e5803c493f910e506d3e47371fdf6d7e5`, registra **ocho** miembros de la `libgcc` local Cortex-A57, incluido `unwind-dw2.o`, y **cero** de la biblioteca global. El ELF conserva SHA-256 `cf52e3b7bc5747dff78c3d4339886c1d27e3de1cb145ccc25a920c25fdfe30ce`. Los primeros **17 719 296 bytes** del NRO nuevo son idénticos al cuerpo del NRO de M006: solo se rehizo ASET (icono y NACP). No se ejecutó `fixup-arm64.py`.

El NRO de prueba mide **17 749 502 bytes**, SHA-256 **`33bccb3327406509efb13583ac0cc9ad86ee4f874ccc433fdb7df559193e05c7`**. Su icono incrustado es un JPEG baseline 256×256 de 12 038 bytes, SHA-256 `775a84b79c4f158b6846215a3edccbb96c28c95c6767de5ff3647939055d030d`, distinto del icono heredado. El NACP muestra **FangameNX FNX-016**, autor **FangameNX**, versión **0.0.1**. La pantalla de carga que ya existe en el cuerpo del motor sigue mostrando «RGSS switch»; pertenece al port y no se modificó en esta misión.

### Construcción repetible del NRO de prueba

Con el ELF M006 cuyo hash figura arriba, `nacptool` y `elf2nro` de `C:\devkitPro\tools\bin`, ImageMagick 7.1.2-30 y una carpeta de salida nueva, se ejecutó desde la raíz del repositorio:

```powershell
$repo = (Resolve-Path '.').Path
$paquete = Join-Path $repo 'local/fnx016-repeticion'
$icono = Join-Path $paquete 'work/icono.jpg'
$nacp = Join-Path $paquete 'work/control.nacp'
$nro = Join-Path $paquete 'switch/fnx004-sintetico/fnx004-sintetico.nro'
if (Test-Path -LiteralPath $paquete) { throw 'La salida ya existe; elija otra carpeta' }
New-Item -ItemType Directory -Path (Split-Path $icono), (Split-Path $nro) -Force | Out-Null
magick (Join-Path $repo 'docs/fangame-nx/fixtures/fnx016/icono.svg') -background '#102438' -alpha off -resize 256x256! -colorspace sRGB -sampling-factor 4:2:0 -interlace none -quality 92 $icono
& 'C:\devkitPro\tools\bin\nacptool.exe' --create 'FangameNX FNX-016' 'FangameNX' '0.0.1' $nacp
& 'C:\devkitPro\tools\bin\elf2nro.exe' (Join-Path $repo 'mkxp-z-nx/rgss-switch.elf') $nro "--icon=$icono" "--nacp=$nacp" "--romfsdir=$(Join-Path $repo 'mkxp-z-nx/romfs')"
```

El bloque usa rutas absolutas para las herramientas y crea una **salida distinta**, para conservar la evidencia original. Se ejecutó tal cual en `local/fnx016-repeticion/`: el segundo NRO tuvo el **mismo SHA-256** `33bccb3327406509efb13583ac0cc9ad86ee4f874ccc433fdb7df559193e05c7`. Después se copian los cuatro archivos de `fixtures/fnx004/` y los dos shims `runtime/lib/`, y se crean el marcador vacío y los directorios vacíos según el inventario siguiente. El SVG convertido dos veces produjo el mismo JPEG por SHA-256. El NACP y el JPEG intermedios están en `work/`, fuera de la carpeta que se copiaría a la tarjeta.

## Contenido exacto del payload

`MANIFEST.sha256.txt` está en `local/fnx016-first-boot/`, fuera de `switch/` (1 075 bytes; SHA-256 `da7eb4e9ea0e29e5fbe62fcc3bf2cd781a212f7cd60d8ebc5c3d8e2fb463b7fa`). Sus ocho entradas se recalcularon: ocho archivos reales, **17 770 204 bytes en total**, tamaños y hashes coincidentes. Los intermedios locales son `work/icono.jpg` (12 038 bytes; hash indicado arriba) y `work/control.nacp` (16 384 bytes; SHA-256 `be60d9f7b888cbf7439f209370aad844f68e5f0d2b959213b0c20fec330e8a28`). La estructura que debe quedar en la tarjeta es:

```text
SD:/switch/fnx004-sintetico/
  fnx004-sintetico.nro
  mkxp.json
  probe.rb
  entrada.txt
  Game.ini
  Data/Scripts.rxdata       (0 bytes)
  Graphics/                 (vacío)
  Audio/                    (vacío)
  lib/nx-compat.rb
  lib/socket.rb
```

| Archivo relativo a `fnx004-sintetico/` | Bytes | SHA-256 |
|---|---:|---|
| `fnx004-sintetico.nro` | 17 749 502 | `33bccb3327406509efb13583ac0cc9ad86ee4f874ccc433fdb7df559193e05c7` |
| `mkxp.json` | 252 | `12c04116640619d5114211d6da4ecc5de4233268fb42b0746b1acf8a21fffe18` |
| `probe.rb` | 1 384 | `0fe0736d897111a1b09a6096cc46deef7f9001380620c2cf24c8b55db2315c61` |
| `entrada.txt` | 18 | `dc8c883dd3884925ab5b566edc10c7e80309f1240f0ce2610fc2318c2dda0364` |
| `Game.ini` | 69 | `e0ecc9428801674c88b950cf7467e9741a13402410a38606f684b6c460f1751a` |
| `Data/Scripts.rxdata` | 0 | `e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855` |
| `lib/nx-compat.rb` | 17 068 | `ed6c8e2cb0b50929de40de5343f73ed2c3f47409afefe2767ceea065a34cdd61` |
| `lib/socket.rb` | 1 911 | `0554868f1d1f4211c63492e6436cdac8407ce856a33a3355dc9f326657bc6fc7` |

El NRO incluye únicamente el motor y sus dependencias estáticas, el icono propio, NACP y un RomFS con `/LEEME.txt`; no incluye datos, scripts o imágenes de fangames. El NRO es un artefacto **local de prueba**, no una release ni una distribución de dependencias. `Audio/` y `Graphics/` deben conservarse aunque estén vacíos: la revisión previa del motor los exige en modo de carpetas sueltas. `lib/socket.rb` es exigido por esa misma revisión aunque el fixture no invoque el plugin. `Game.ini` identifica el caso sintético y el archivo vacío `Scripts.rxdata` satisface la comprobación de existencia; `customScript` hace que el motor evalúe `probe.rb` en su lugar.

## Verificaciones estáticas y límite de la prueba

[`log-m007-verificacion.txt`](log-m007-verificacion.txt) registra comandos y resultados. `comprobar-nro.py` confirmó `NRO0`, `ASET` versión 0, rangos válidos, icono propio, NACP y RomFS. [`auditar-armv8.py`](auditar-armv8.py) comparó **11 713 656 bytes** de código del ELF con el NRO nuevo sin diferencias y encontró **0** instrucciones incompatibles conocidas. `llvm-objdump 21.1.8 -d --mcpu=cortex-a57` produjo **0** instrucciones `<unknown>` en el ELF; `nm -u` produjo **0** símbolos indefinidos. La auditoría GNU completa de M006 se aplica al mismo cuerpo binario y se conserva en [`log-m006-verificacion.txt`](log-m006-verificacion.txt).

`mkxp.json` se analizó como JSON: `customScript=probe.rb`, `rgssVersion=1`, `rubyLoadpath=[lib]`, sin preloads. Los cuatro ficheros sintéticos copiados son idénticos por hash a los de M004. El Ruby anfitrión no estaba en `PATH` en esta sesión; `probe.rb` pasó `ruby -c` en M004/M006, pero **no** se repitió esa prueba de sintaxis aquí. Ni el arranque, ni la ejecución de Ruby, ni el dibujo, ni la salida al menú están verificados en Switch.

## Guía reservada para una autorización posterior

**No ejecutar estos pasos hasta recibir autorización explícita para escribir en la microSD y realizar la prueba física.** Solo se necesita homebrew NRO mediante Atmosphère; no se genera ni instala NSP.

1. Apagar la consola correctamente antes de retirar la microSD y abrir la tarjeta en el PC. Identificar su letra de unidad; esta guía la representa por `X:`. Comprobar que `X:\switch\fnx004-sintetico\` **no exista**. Si ya existe, detenerse: no borrar ni sobrescribir la instalación previa. Conservar íntegros `X:\atmosphere\`, `X:\bootloader\`, `X:\emuMMC\`, los demás directorios de `X:\switch\` y cualquier otro archivo de la tarjeta.
2. Copiar **solo** `C:\Users\pablo\Documents\GitHub\FangameNX\local\fnx016-first-boot\switch\fnx004-sintetico\` a `X:\switch\fnx004-sintetico\`. No copiar `work/` ni el manifiesto. Confirmar que `Graphics/` y `Audio/` vacíos siguen presentes. Tras la copia, calcular `Get-FileHash -LiteralPath 'X:\switch\fnx004-sintetico\fnx004-sintetico.nro' -Algorithm SHA256` y exigir `33bccb3327406509efb13583ac0cc9ad86ee4f874ccc433fdb7df559193e05c7` antes de encender.
3. Expulsar la unidad de forma segura y volver a iniciar la configuración de Atmosphère que el propietario ya utiliza. Desde HOME, mantener **R** al abrir un juego o aplicación existente y mantenerlo hasta que aparezca Homebrew Menu con memoria de aplicación. Es el comportamiento por defecto descrito por [Atmosphère](https://github.com/Atmosphere-NX/Atmosphere/blob/master/docs/features/configurations.md); si la instalación usa otra tecla, consultar su configuración existente sin cambiarla en esta prueba. Evitar el hbmenu abierto como applet desde Álbum.
4. En Homebrew Menu, abrir `switch/fnx004-sintetico/` y seleccionar `fnx004-sintetico.nro`, identificable como **FangameNX FNX-016**. Registrar si aparece la pantalla de revisión, la carga «RGSS switch», las bandas **magenta y verde**, y, si la fuente funciona, el texto «FNX-004 RUBY OK». El script ejecuta 480 `Graphics.update` y después intenta terminar normalmente; la duración y el retorno real al menú siguen sin comprobarse.
5. Si la aplicación responde, probar HOME y cerrar el homebrew desde el menú. Si la consola deja de responder, mantener POWER durante **al menos 20 segundos** para apagarla, según la [guía de Nintendo para una Switch bloqueada](https://en-americas-support.nintendo.com/app/answers/detail/a_id/22523/p/897/c/990). No retirar la tarjeta mientras la consola esté encendida.

### Registros que recoger después de la prueba

En `X:\switch\fnx004-sintetico\`, buscar `mkxp-boot.log` (llegada a `main()` y carpeta seleccionada), `mkxp-debug.log`, `mkxp-ruby.log` y `errorlog.txt` **solo si existen**. El script de prueba intenta crear `fnx004-probe.log`: sus marcas `FNX004 RUBY INICIADO`, `lectura=OK`, `escritura=OK`, `pantalla=bandas magenta y verde` y `FNX004 RUBY COMPLETADO` distinguen las fases. `fnx004-escritura.txt` con `FNX004-ESCRITURA-OK` verifica la escritura y relectura. Si no se reconoció la carpeta, el log temprano puede estar en `X:\rgss-switch.log`. Si Atmosphère muestra un fallo, conservar los informes que **aparezcan** en `X:\atmosphere\crash_reports\` o `fatal_errors\`, sin modificar los existentes. Registrar foto o descripción de pantalla, punto de fallo, modo de hbmenu y resultado de salida. La ausencia de un log **no** demuestra que esa fase haya funcionado ni que se haya alcanzado.

## Pendiente

La compatibilidad estática con Cortex-A57 es favorable, pero el funcionamiento en Switch 1, la selección real de `argv[0]`, la apertura de `probe.rb`, el dibujo, la escritura en microSD y el retorno al menú siguen pendientes de la primera prueba física. La reproducción completa del build desde cero sigue limitada por la fase auxiliar de GCC descrita en M006. Esta misión no declara FNX-003 ni FNX-004 finalizados. La próxima tarea es **instalación guiada y observación en hardware**, únicamente tras autorización expresa.
