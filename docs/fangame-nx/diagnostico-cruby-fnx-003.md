# Misión 003 — enlace de CRuby para FNX-003

Fecha: 1 de noviembre de 2026. Rama: `fix/fnx-003-cruby-link`, creada desde `build/fnx-003` (`4e0d7d0`). Alcance: sistema de compilación, objetos y logs locales. Ninguna observación de esta misión prueba ejecución en Switch.

## Estado inicial y dos rutas de enlace

El cuarto intento de la Misión 002 generó `libruby-static.a` y `ext/stringio/stringio.a`, `ext/strscan/strscan.a`, `ext/zlib/zlib.a` y `ext/extinit.o`, pero `make` acabó con código 2 al enlazar `ruby.exe`. El objetivo `ruby.exe` del `Makefile` principal toma `$(EXTOBJS)` y `$(EXTLIBS)`. `exts.mk`, generado por `extmk.rb`, define ambos y lanza un `make` recursivo con esas variables. La invocación manual anterior `make V=1 ruby.exe` saltó ese nivel: el comando resultante solo pasaba `ext/extinit.o`, por lo que faltaban `Init_stringio`, `Init_strscan` e `Init_zlib`. Las tres funciones sí están definidas en sus respectivas bibliotecas, comprobado con `aarch64-none-elf-nm`. Esas referencias indefinidas son un efecto de la invocación diagnóstica incompleta, no la causa del fallo original de `make`.

La reproducción correcta del enlace original es `make -f exts.mk V=1 LD=aarch64-none-elf-ld ruby.exe` desde `build/ruby-switch` con `DEVKITPRO=C:/devkitPro`. Acabó con código 2 y mostró en la línea de GCC `main.o`, `ext/extinit.o`, las tres bibliotecas de extensión y `-lz`, seguidos de `libruby-static.a` y las bibliotecas de plataforma. El linker real es `C:/devkitPro/devkitA64/.../aarch64-none-elf/bin/ld.exe`, invocado por `aarch64-none-elf-gcc` mediante `collect2.exe`; no se llamó a `ld` del anfitrión. El error fue `section .tls.align LMA [0,7] overlaps section .text LMA [0,0x23e337]`.

## Causa confirmada del solapamiento

`build/ruby-switch/Makefile` incluye `-specs=C:/devkitPro/libnx/switch.specs` en `LDFLAGS`. `build/ruby-switch/exts.mk` vuelve a incluir la misma opción en `EXTLDFLAGS`, que pasa al `make` recursivo. La línea fallida contiene **dos** `-specs` idénticos. `switch.specs` expande cada uno a `-T C:/devkitPro/libnx/switch.ld`; `aarch64-none-elf-gcc -###` mostró dos `-T` en la invocación efectiva de `collect2.exe`. El script `switch.ld` sitúa `.text` desde `0` y `.tls.align` en la región de datos de solo lectura tras `.text`; aplicarlo dos veces reinicia el contador de ubicación y produce secciones solapadas. No es necesario suprimir TLS ni alterar el script del SDK.

Prueba causal acotada: `make -f exts.mk V=1 LD=aarch64-none-elf-ld EXTLDFLAGS= ruby.exe` conservó `EXTOBJS` y `EXTLIBS` y eliminó únicamente la segunda colección de flags del enlace. Terminó con código **0**. `readelf -h` identifica el resultado como ELF64 AArch64 PIE; `objdump -h` muestra `.text` en LMA `0`, `.tls.align` en `0x2f4e10` y `.main.tls` en `0x3629c8`, sin solapamiento. `nm` encuentra `Init_ext`, `Init_stringio`, `Init_strscan`, `Init_zlib` y `ruby_init` en el ELF. Este archivo no es un NRO ni se ha ejecutado.

## Extensiones y postproceso: problema independiente

`exts.mk` integra las extensiones durante el enlace del ejecutable temporal mediante `EXTOBJS="ext/extinit.o ext/stringio/stringio.a ext/strscan/strscan.a ext/zlib/zlib.a"`. Cada archivo `.a` contiene un único objeto (`stringio.o`, `strscan.o`, `zlib.o`). El `ext/extinit.c` generado registra los tres nombres con `ruby_init_ext`; el `patches/extinit.c` heredado hace lo mismo en otro orden. `libruby-static.a` aún contiene `dmyext.o` y no contiene esas extensiones. `finalize-ruby.sh` espera objetos bajo `out/nx/extobj/`, pero ningún paso documentado los copia allí. Es un problema de postproceso independiente de los dos `-T` del enlace temporal. Hay que adaptar ese guion a los objetos generados y verificar con `nm` que el archivo final contiene los cuatro `Init_*` sin `dmyext.o`.

## Plan de verificación ejecutado

Aplicar una corrección mínima al comando de `make` que evite la segunda expansión de `switch.specs`; repetir la compilación completa desde un árbol de build nuevo conservando el anterior. Después verificar las extensiones, el archivo Ruby final y el enlace del motor. Los logs y hashes de cada paso se añadirán aquí.

## Compilación limpia de CRuby y postproceso

El árbol anterior se conservó como `build/ruby-switch-m002-4e0d7d0/` antes de ejecutar `./build-ruby-switch.sh` desde un directorio nuevo. El guion ahora escribe logs nuevos bajo `src/build-logs/` para no sobrescribir `docs/configure-log.txt` ni `docs/make-log.txt`. La primera prueba limpia empleó `make -j6 LD=aarch64-none-elf-ld EXTLDFLAGS=`: `configure` y `make` terminaron ambos con código **0**. El ELF temporal era PIE ELF64 AArch64; `.text` comenzaba en LMA `0`, `.tls.align` en `0x59a460` y `.main.tls` en `0x6139c8`. `nm` encontró `Init_ext`, `Init_stringio`, `Init_strscan`, `Init_zlib` y `ruby_init`.

La inspección posterior de `exts.mk` mostró que `EXTLDFLAGS` contiene, además de las opciones ya presentes en `LDFLAGS`, `-Wl,--compress-debug-sections=zlib`. El valor vacío también la retiraba. Para conservarla, la corrección final del guion establece `EXTLDFLAGS=-Wl,--compress-debug-sections=zlib`: así queda un único `-specs`/`-T` y se mantiene la opción adicional generada por `extmk.rb`. Esta selección se basa en los flags exactos de CRuby 3.1.7 generados en este entorno; debe revisarse si cambia la versión o la configuración.

`./finalize-ruby.sh` terminó con código **0** después de tomar `ext/extinit.o`, `ext/stringio/stringio.o`, `ext/strscan/strscan.o` y `ext/zlib/zlib.o` del árbol generado. El guion ya no depende del directorio inexistente `out/nx/extobj/`. Eliminó `dmyext.o`, añadió los cuatro objetos a `out/nx/lib/libruby-static.a` y verificó `Init_ext`, `Init_zlib`, `Init_stringio`, `Init_strscan`, `ruby_init_ext` y `ruby_init`. El archivo final mide 45 983 716 bytes y tiene SHA-256 `a01bc053e064d97c74e5b7f533164c3c312d6d83c1e86d4fe453307531cfebf6`. El ejecutable temporal `ruby.exe` tiene SHA-256 `83756f7fc133522c0eb3742cd9611a910fa2415f34da1ec2ee9e0cebe20944f5`. Estas son comprobaciones de enlace y formato, no de ejecución en consola.

## Primer intento del motor

`./gen-xxd.sh` terminó con código 0 y creó 29 archivos `.xxd` ignorados por Git. `make -C mkxp-z-nx -j6` terminó con código 2 durante `config.cpp`: `src/mkxp-z/src/util/encoding.h` incluye `<uchardet.h>`, pero la versión de uchardet instalada por CMake coloca la cabecera en `out/nx/include/uchardet/uchardet.h`. El Makefile solo añade `-Iout/nx/include`. Se añadirá ese subdirectorio al include path. No se copiará ni modificará la cabecera de terceros.

La repetición con la ruta de uchardet compiló más fuentes y se detuvo en `binding-mri.cpp` por `<ruby.h>` ausente. El postproceso instala `libruby-static.a`, pero no cabeceras. La cabecera pública existe en `src/ruby-3.1.7/include/ruby.h` y la configuración AArch64 generada en `build/ruby-switch/.ext/include/aarch64-elf/ruby/config.h`; ninguna está en `out/nx/include`. Se añadirán al Makefile ambos directorios de inclusión, sin copiar o modificar fuentes de Ruby.

La tercera ejecución del Makefile compiló todos los objetos y enlazó `rgss-switch.elf` (88 867 944 bytes), pero acabó con código 127 en la receta posterior `python fixup-arm64.py`: la shell MSYS2 de devkitPro no encuentra `python`. Windows sí tiene `C:/Python313/python.exe` y `py -3` accesible desde MSYS2; `py -3 --version` informó Python 3.13.14. El intento `py -3 fixup-arm64.py --help` terminó con `FileNotFoundError` porque ese guion no implementa ayuda y trató `--help` como nombre de archivo; no cambió el ELF. Se usará una variable `PYTHON ?= py -3` en el Makefile y se conservará el ELF previo antes de repetir la receta. `fixup-arm64.py` es parte del build heredado y modifica el ELF en sitio para neutralizar instrucciones ARMv9 de libgcc; no se alterará su código en esta misión.

## Correcciones del build y resultado de los intentos

| Cambio versionado | Causa verificada | Alcance |
|---|---|---|
| `build-ruby-switch.sh`: logs únicos y `EXTLDFLAGS=-Wl,--compress-debug-sections=zlib` | El `make` recursivo añadía una segunda copia de `switch.specs`; el primer valor vacío resolvió el solapamiento, pero omitía la compresión de depuración. El valor final conserva la única opción adicional de `exts.mk`. | Solo flags y salida de logs; no se toca `switch.ld`, TLS ni fuentes de CRuby. |
| `finalize-ruby.sh`: cuatro rutas reales de objetos y comprobación con `nm`/`ar` | `out/nx/extobj/` no existía; los objetos estaban en `build/ruby-switch/ext/`. | Archivo estático generado localmente; no cambia código del intérprete. |
| `mkxp-z-nx/Makefile`: rutas de uchardet y de las dos cabeceras Ruby; `PYTHON ?= py -3` | Fallos observados de cabeceras ausentes y de `python` fuera de PATH. | Solo inclusión y ejecución del postproceso heredado. |
| `.gitignore` y documentación | La lista blanca requiere rutas explícitas; los binarios y dependencias siguen ignorados. | Solo registros versionados. |

La cuarta ejecución de `make -C mkxp-z-nx -j6` con `py -3` terminó con código **0**: el guion heredado neutralizó 15 instrucciones ARMv9 en el ELF y `elf2nro` generó un NRO. Después se conservó ese resultado, se recompiló el motor desde un directorio de objetos nuevo y volvió a terminar con código **0**. El Makefile incrementa su número de build, de modo que estos dos NRO no son idénticos byte a byte.

Para cerrar la corrección final se conservó el primer árbol Ruby de esta misión, se repitió `./build-ruby-switch.sh` desde cero con el flag de compresión mantenido y se ejecutó otra vez `./finalize-ruby.sh`: **ambos terminaron con código 0**. El nuevo `ruby.exe` tiene `.text` en LMA `0`, `.tls.align` en `0x59a460` y `.main.tls` en `0x6139c8`; contiene `Init_stringio`, `Init_strscan` e `Init_zlib`. El archivo final contiene `extinit.o`, `stringio.o`, `strscan.o`, `zlib.o` y los seis símbolos comprobados por el guion; `dmyext.o` no aparece. Se conservó el NRO anterior y se hizo una tercera compilación limpia del motor, ahora enlazada contra este archivo Ruby final: **código 0**. La salida `fixup-arm64.py` informa de 15 instrucciones neutralizadas; eso debe evaluarse en una prueba de ejecución, ya que terminar el enlace no demuestra el comportamiento de ese código.

Los comandos de la cadena final, desde la raíz del repositorio y con las fuentes y portlibs preparados según [`entorno-fnx-003.md`](entorno-fnx-003.md), fueron:

```sh
./build-ruby-switch.sh
./finalize-ruby.sh
./gen-xxd.sh                 # 29 archivos .xxd; ya generados antes del enlace final
make -C mkxp-z-nx -j6       # desde la shell MSYS2 de devkitPro
```

Antes de cada compilación limpia se trasladaron los directorios de objetos y artefactos previos a nombres únicos **dentro del repositorio**, sin borrarlos. Los logs originales están ignorados en `src/build-logs/`; [`log-m003-compilacion.txt`](log-m003-compilacion.txt) reúne las salidas completas, incluida la línea de enlace fallida, los comandos de aislamiento y los dos builds finales. Solo se sustituyó el nombre de usuario en esa copia versionada.

## Artefacto final y comprobaciones

Entorno observado: devkitA64 `r30-1`, GCC `16.1.0`, libnx `4.12.0-1`, switch-tools `1.13.1-1`, GNU Make `4.4.1` y Python `3.13.14`. No se instalaron ni modificaron herramientas globales durante esta misión.

| Archivo local ignorado por Git | Tamaño | SHA-256 |
|---|---:|---|
| `build/ruby-switch/ruby.exe` | 14 495 648 | `b89a90640741f35c681cee92abb2d6b63fa519facbf9da41f06fa172f1e2a9bb` |
| `out/nx/lib/libruby-static.a` | 45 983 716 | `dcfb9f54da3c0d1573ae1fac5a257f5c5334edc081d3a8588bcb0b3636b940cb` |
| `mkxp-z-nx/rgss-switch.elf` | 88 867 944 | `430be0c747f31c909537cf910a1dcdee334c7c9463894926b35eb13211c19a0c` |
| `mkxp-z-nx/rgss-switch.nro` | 17 750 457 | `9bec33460205b3989bf405740c596c1b370311938bd7df586fb42e2e0ad7b496` |

`readelf -h` identifica el ELF final como **ELF64 AArch64 PIE** y `nm -u` no muestra símbolos indefinidos. `objdump -h` sitúa `.text` en LMA `0`, `.tls.align` en `0xe394e0` y `.main.tls` en `0x11cc8a0`. `comprobar-nro.py` encuentra las cabeceras `NRO0` y `ASET`; el icono, NACP y RomFS están dentro del tamaño del archivo. El RomFS local contiene solo `LEEME.txt`. El icono ASET coincide byte a byte con `nsp/icono.jpg` (SHA-256 `849c5c07167301a40121fb8ddddc8f809e281eb9a37ed7c52dc20e690b9453eb`). La arquitectura del NRO se infiere del ELF AArch64 que `elf2nro` convirtió; el NRO no se ha ejecutado en consola. [`log-m003-verificacion.txt`](log-m003-verificacion.txt) conserva los comandos y resultados.

## Límites, licencias y estado de FNX-003

La compilación completa de Ruby y del motor **se reprodujo en este entorno preparado**, con dos builds limpios del motor y uno adicional de Ruby tras ajustar el flag final. No se ha repetido la instalación del SDK ni la obtención de las fuentes desde un equipo vacío. No se han ejecutado pruebas en Switch ni con fangames; el NRO está comprobado por formato, secciones, símbolos y contenido empaquetado, no por funcionamiento. El build final conserva advertencias de GCC (`-Wclass-memaccess`, `-Wformat-truncation` y flags de C++ usados al compilar C); ninguna se corrigió con cambios funcionales al runtime.

`LICENCIAS.md` declara GPLv2+ para el port y mkxp-z, licencia Ruby/BSD-2 para CRuby y varias licencias de dependencias enlazadas; OpenAL-soft es LGPLv2+ y requiere revisar condiciones de redistribución con enlace estático. El NRO incluye el icono heredado del motor y recursos empotrados según esa documentación. No se creó ZIP ni release, ni se añadió un juego, clave, captura o binario a Git. Antes de distribuir, hay que verificar de forma específica los avisos, fuentes correspondientes y permisos de todos los recursos que entren en la entrega.

**FNX-003:** desbloqueado el enlace de CRuby y obtenido un NRO reproducible a nivel de procedimiento en el entorno descrito. La siguiente tarea recomendada es una prueba de arranque en hardware con un caso sintético o propio, registrar logs de ejecución y revisar la neutralización ARMv9 y las advertencias que puedan afectar al runtime. No iniciar el selector multi-juego hasta separar ese resultado de la mera generación del archivo.
