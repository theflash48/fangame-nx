# Registro de M006

Las rutas relativas se entienden desde la raíz de FangameNX. Los logs íntegros de GCC y MSYS2 están en `src/build-logs/fnx016-*`, excluidos de Git por su tamaño. Este registro conserva los comandos y los resultados relevantes; las rutas completas se encuentran en esos logs locales.

| Paso | Comando o comprobación | Resultado |
| --- | --- | --- |
| Rama | `git switch -c fix/fnx-015-isolated-libgcc` desde `52ce014` | Rama aislada; `main` intacta. |
| SDK | `pacman -Qi devkita64-gcc`; `aarch64-none-elf-gcc -v`; `-print-multi-lib`; `-print-libgcc-file-name` | GCC 16.1.0-1, objetivo `aarch64-none-elf`, `pic/libgcc.a` seleccionada. |
| Fuentes | Descargar `gcc-16.1.0.tar.xz` y comparar SHA-512 con `sha512.sum` de GCC | Coincidencia; 102 456 900 bytes. |
| Parche SDK | `patch --dry-run -p1` y `patch -p1` con `gcc-16.1.0-1.patch` de devkitPro | Ambos código 0; log local `devkitpro-patch-*.log`. |
| Prerrequisitos GCC | `./contrib/download_prerequisites --no-isl` | Código 0; cuatro SHA-512 OK, `prerequisites.log`. |
| Host local | MSYS2 base 20260927; `pacman -Sy --needed --noconfirm gcc make autoconf automake bison flex texinfo` | Primer espejo HTTP 500; reintento completó instalación. GCC 16.2.0, Make 4.4.1, Bison 3.8.2 ejecutados. |
| Configuración GCC | `configure --target=aarch64-none-elf --with-newlib --with-march=armv8 --enable-multilib --enable-threads=posix --enable-languages=c --disable-bootstrap --disable-shared --disable-nls ...` en `src/build-logs/fnx016-gcc-build/full` | Código 0; `configure.log`. Prefijo de salida local ignorado. |
| Intento de libgcc independiente | `libgcc/configure` y `make unwind-dw2.o` con el compilador objetivo existente | La configuración generó Makefile, pero la compilación falló por `tconfig.h` ausente: es un encabezado generado por la fase GCC. Se pasó a construir `all-gcc` localmente en vez de inventar el encabezado. Logs `configure-pristine.log` y `make-pristine-object.log`. |
| Parche A57 | `patch -R --dry-run -p1 -i docs/fangame-nx/patches/libgcc-cortex-a57.patch` sobre el árbol local ya parcheado | Código 0; siete fuentes coinciden. |
| Ensamblador A57 | `aarch64-none-elf-gcc -x assembler-with-cpp -DFNX_CORTEX_A57_ONLY -march=armv8-a -fPIC -I<fuente> -I<auto-target.h> -c` para cinco fuentes; `objdump -d` y `readelf -sW` | Cinco objetos ELF AArch64 compilados; opcodes y símbolos inspeccionados en `src/build-logs/fnx016-asm-probe/`. El primer intento con rutas `-I` relativas falló por `auto-target.h` no encontrado; las rutas absolutas resolvieron la prueba. |
| GCC auxiliar | `make -j4 all-gcc` en `src/build-logs/fnx016-gcc-build/full` | Generó `tconfig.h`, `tm.h`, `auto-host.h` y otros encabezados reales. Se interrumpió tras construir `libgcc.a` con el compilador objetivo ya instalado; no se instaló un GCC anfitrión nuevo. Log íntegro `make-all-gcc.log`. |
| Prueba de desenrollador | `make gcc_objdir=../gcc unwind-dw2.o` | Primer intento falló por macros de configuración ausentes en un `libgcc.mvars` provisional; tras añadir `-DIN_GCC` y copiar los encabezados generados, terminó con código 0. Logs `make-a57-unwind-probe*.log`. |
| Archivo completo | `make -j2 gcc_objdir=../gcc libgcc.a` | Código 0, log `make-a57-libgcc-a.log`. Un `make all` previo intentó multilib `large` y falló por ruta relativa `libgcc.mvars`; se invocó el objetivo preciso `libgcc.a` para la variante PIC requerida. |
| Símbolos y miembros | `ar t`, `nm -g --defined-only`, `Compare-Object` | 409 miembros y 727 símbolos globales definidos con tipos idénticos a la biblioteca instalada; sin diferencias. |
| Selección aislada | Copia a `src/build-logs/fnx016-isolated-prefix/pic/libgcc.a`; `aarch64-none-elf-gcc -B<prefijo>/ -fPIE -print-libgcc-file-name` | Selecciona archivo local. SHA-256 `b7bca10ee17cae2295533e793c513ab144095e37b2e21142817c5c9811c837dc`. |
| CRuby limpio | `FNX_LIBGCC_PREFIX=/c/.../fnx016-isolated-prefix ./build-ruby-switch.sh` | Código 0. Configuración `ruby-configure.Xmhdbv`, compilación `ruby-make.lDBB77`, salida `fnx016-cruby-build.log`. `DLDFLAGS` muestra `-B` local; 82 objetos raíz y biblioteca de 44 648 078 bytes. |
| Postproceso Ruby | `./finalize-ruby.sh` | Código 0; `fnx016-finalize-ruby.log`. Se eliminó `dmyext.o`; seis símbolos críticos presentes en biblioteca final de 45 983 716 bytes. |
| Segunda construcción | En otro directorio limpio, repetir `libgcc/configure` y `make -j2 gcc_objdir=../gcc libgcc.a` con las mismas fuentes, encabezados y flags | Código 0; 409 miembros, 727 símbolos iguales. Archivo completo SHA-256 `d9232064dfef2705dd6c8e123ca52ab5a484ad6137d2aeab057bc0c04fbb04dd`; difiere por información de depuración ligada a la ruta del build. `objcopy --strip-debug` sobre cada uno de los 409 miembros deja 0 diferencias byte a byte. Logs versionados `log-m006-libgcc-repro-configure.txt`, `log-m006-libgcc-repro-make.txt`. |
| Motor limpio | `make -C mkxp-z-nx -j6 FNX_LIBGCC_PREFIX=<ruta-local-Windows> FNX_LINK_MAP=<mapa-local>` | Código 0; `fixup-arm64.py` omitido; ELF y NRO producidos. Salida completa en `log-m006-motor-build.txt`. |
| Mapa y símbolos | Inspeccionar `FNX_LINK_MAP`, `nm -u` y `readelf -wf` | Ocho miembros de la libgcc local, ninguno de la global; 0 símbolos indefinidos; rutinas de excepción y desenrollado presentes. Mapa local SHA-256 `77f64254eb2f743cefcfc0fb993cec7e5803c493f910e506d3e47371fdf6d7e5`. |
| Auditoría CPU | `py -3 docs/fangame-nx/auditar-armv8.py <ELF> <NRO>`; `llvm-objdump -d --mcpu=cortex-a57 <ELF>`; `aarch64-none-elf-objdump -d -M no-aliases <ELF>` | M005: 23 instrucciones conocidas/`<unknown>`; M006: 0/0. El código ELF/NRO coincide fuera de la cabecera. Desensamblado completo y control positivo LSE registrados en `log-m006-verificacion.txt`. |
| Estructura y hash | `py -3 comprobar-nro.py <NRO>`; `Get-FileHash <NRO> -Algorithm SHA256` | NRO0/ASET y rangos válidos; 17 750 457 bytes, SHA-256 `9416eaaf5ea40351f0bf991ba2b43d09cde1540c8c3a19cb590f27f38f384ea7`. Incluye icono heredado cuya redistribución está pendiente de revisión. |
| Fixture | `ruby -c docs/fangame-nx/fixtures/fnx004/probe.rb`; analizar `mkxp.json`; copiar solo a `local/fnx006-hardware-probe/` ignorado | Sintaxis OK; paquete local con NRO M006, sin escritura en microSD ni ejecución en consola. |

Para repetir **la biblioteca aislada**, el árbol `fnx016-gcc-source/gcc-16.1.0` contiene el tar oficial, primero con el parche de devkitPro y después con `patch -p1 -i docs/fangame-nx/patches/libgcc-cortex-a57.patch`. Desde `fnx016-gcc-build/full`, la configuración auxiliar fue (con `<FANGAMENX>` sustituido por la ruta MSYS local):

```sh
<FANGAMENX>/src/build-logs/fnx016-gcc-source/gcc-16.1.0/configure \
  --target=aarch64-none-elf --prefix=<FANGAMENX>/src/build-logs/fnx016-gcc-build/install \
  --with-as=/c/devkitPro/devkitA64/bin/aarch64-none-elf-as \
  --with-ld=/c/devkitPro/devkitA64/bin/aarch64-none-elf-ld \
  --with-newlib --with-march=armv8 --enable-multilib --enable-threads=posix \
  --enable-languages=c --disable-bootstrap --disable-shared --disable-nls \
  --disable-libsanitizer --disable-libstdcxx --disable-libgomp --disable-libssp \
  --disable-libquadmath --disable-libatomic --disable-libfortran --disable-libvtv --disable-lto
make -j4 all-gcc
```

`make all-gcc` se detuvo tras generar los encabezados de `gcc/`; no se instaló un GCC anfitrión nuevo. Se copiaron `tconfig.h`, `tm.h`, `auto-host.h`, `options.h` e `insn-*.h` a `fnx016-libgcc-build/gcc/`. Esa fase auxiliar todavía exige intervención manual y no se ha demostrado una reconstrucción íntegra de GCC desde cero. El archivo `gcc/libgcc.mvars` local contiene exactamente:

```make
GCC_CFLAGS = -g -O2 -fPIC -march=armv8-a+crc+crypto -mtune=cortex-a57 -mtp=soft -DFNX_CORTEX_A57_ONLY -DIN_GCC
INHIBIT_LIBC_CFLAGS =
```

Desde `fnx016-libgcc-build/pristine-pic`, con `aarch64-none-elf-gcc`, `ar` y `ranlib` de devkitA64 en `PATH`, se ejecutó:

```sh
../../fnx016-gcc-source/gcc-16.1.0/libgcc/configure --build=x86_64-pc-msys --host=aarch64-none-elf --target=aarch64-none-elf --with-newlib --enable-threads=posix --disable-shared
make -j2 gcc_objdir=../gcc libgcc.a
```

La copia de `libgcc.a` a `fnx016-isolated-prefix/pic/` es la única entrada binaria nueva para los enlaces; el SDK original permanece intacto. Para CRuby, `FNX_LIBGCC_PREFIX` debe usar la ruta MSYS (`/c/...`); para el Makefile del motor, una ruta Windows con barras `/`. El valor se comprueba antes del enlace. La repetición del archivo en otro directorio mostró equivalencia de código y símbolos, pero no identidad del SHA-256 de la biblioteca con depuración. El NRO de esta misión solo se construyó una vez.
