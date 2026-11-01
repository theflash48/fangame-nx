#!/bin/bash
# ==============================================================================
#  CRuby 3.1.7 -> Nintendo Switch (Horizon OS / libnx / devkitA64)
#  Objetivo: libruby-static.a enlazable contra libnx, base para mkxp-z.
#
#  PARCHES APLICADOS:
#   1. -std=gnu11      : GCC 16 usa C23 por defecto; Ruby 3.1.7 no es compatible
#                        (typedef de _Bool + declaracion K&R de getenv en dln.c)
#   2. shim sys/mman.h : Horizon no tiene memoria mapeada POSIX
#   3. libnxcompat.a   : newlib declara sysconf() pero no la implementa
# ==============================================================================
set -o pipefail

# DEVKITPRO debe estar en forma WINDOWS: switch.specs contiene
#   -T %:getenv(DEVKITPRO /libnx/switch.ld)
# y GCC expande ese getenv internamente. Con una ruta POSIX el enlazador no
# encuentra switch.ld, sigue con el script por defecto y las secciones se
# solapan: ".tls.align LMA overlaps .text LMA".
export DEVKITPRO=C:/devkitPro
export DEVKITA64=$DEVKITPRO/devkitA64
export PORTLIBS=$DEVKITPRO/portlibs/switch
# El PATH si necesita forma POSIX para el shell
export PATH=/c/devkitPro/devkitA64/bin:/c/devkitPro/portlibs/switch/bin:$PATH

ROOT=$(cd "$(dirname "$0")" && pwd -P)  # forma POSIX: shell y ficheros
ROOT_W=$(cygpath -m "$ROOT")              # forma Windows: GCC y Ruby anfitrion.
                        # mkmf.rb lanza sus conftest desde el Ruby de Windows,
                        # SIN la traduccion de rutas de MSYS2, asi que un
                        # -L/c/anilswitch/... se buscaria en C:\cnilswitch\...
SRC=$ROOT/src/ruby-3.1.7
BUILD=$ROOT/build/ruby-switch
PREFIX=$ROOT/out/ruby-switch
BASERUBY=$ROOT/src/rubyinstaller-3.1.4-1-x64/bin/ruby.exe
SHIM=$ROOT/patches/include
[ ! -L "$ROOT/build" ] && [ ! -L "$ROOT/out" ] || { echo "build/ y out/ no pueden ser enlaces simbolicos" >&2; exit 1; }
mkdir -p "$ROOT/build" "$ROOT/out/nx/lib"
LOGDIR="$ROOT/src/build-logs"
[ ! -L "$LOGDIR" ] || { echo "src/build-logs/ no puede ser enlace simbolico" >&2; exit 1; }
mkdir -p "$LOGDIR"
CONFIG_LOG=$(mktemp "$LOGDIR/ruby-configure.XXXXXX")
MAKE_LOG=$(mktemp "$LOGDIR/ruby-make.XXXXXX")
echo "configure log: $CONFIG_LOG"
echo "make log: $MAKE_LOG"

ARCH="-march=armv8-a+crc+crypto -mtune=cortex-a57 -mtp=soft -fPIE"

export CC=aarch64-none-elf-gcc
export CXX=aarch64-none-elf-g++
export AR=aarch64-none-elf-ar
export RANLIB=aarch64-none-elf-ranlib
export STRIP=aarch64-none-elf-strip
export NM=aarch64-none-elf-nm
export OBJCOPY=aarch64-none-elf-objcopy
export LD=aarch64-none-elf-ld   # OMITIRLO hace que el Makefile invoque 'ld' pelado -> Error 127

INC="-I$ROOT_W/patches/include -I$DEVKITPRO/libnx/include -I$PORTLIBS/include"
export CFLAGS="-g -O2 -std=gnu11 $ARCH -D__SWITCH__ -include $ROOT_W/patches/include/nxshim.h -ffunction-sections -fdata-sections $INC"
export CPPFLAGS="-D__SWITCH__ -include $ROOT_W/patches/include/nxshim.h $INC"
export LDFLAGS="-specs=$DEVKITPRO/libnx/switch.specs $ARCH -L$ROOT_W/out/lib -L$DEVKITPRO/libnx/lib -L$PORTLIBS/lib"
export LIBS="-lnxcompat -lnx"

echo "=================== SANITY ==================="
aarch64-none-elf-gcc --version | head -1
echo "BASERUBY: $($BASERUBY --version 2>&1)"
[ -f "$ROOT/out/lib/libnxcompat.a" ] && echo "libnxcompat.a: presente" || { echo "FALTA libnxcompat.a"; exit 1; }
[ -f "$SHIM/sys/mman.h" ] && echo "shim sys/mman.h: presente" || { echo "FALTA shim mman"; exit 1; }

echo ""
echo "=================== CONFIGURE ==================="
rm -rf "$BUILD"; mkdir -p "$BUILD"; cd "$BUILD"

# NOTA sobre ac_cv_header_sys_mman_h=no:
# El shim existe en el include path (cont.c lo incluye sin proteccion y lo
# necesita), pero le decimos a autoconf que NO esta. Asi gc.c -que si protege
# con #ifdef HAVE_SYS_MMAN_H- evita las rutas de mmap y usa posix_memalign,
# que en Horizon es mas eficiente que nuestro mmap respaldado por malloc.
"$SRC/configure" \
  --host=aarch64-none-elf --target=aarch64-none-elf \
  --prefix="$PREFIX" \
  --disable-shared --disable-install-doc --disable-install-rdoc \
  --disable-dtrace --disable-jit-support --disable-yjit \
  --with-static-linked-ext \
  --with-ext=zlib,stringio,strscan \
  --with-baseruby="$BASERUBY" \
  --without-gmp \
  --with-out-ext=openssl,fiddle,pty,syslog,readline,dbm,gdbm,win32,win32ole,socket,etc,-test-/* \
  ac_cv_func_fork=no ac_cv_func_vfork=no ac_cv_func_daemon=no ac_cv_func_waitpid=yes \
  ac_cv_func_execv=no ac_cv_func_execve=no \
  ac_cv_func_dlopen=no ac_cv_lib_dl_dlopen=no \
  ac_cv_exeext=.exe \
  ac_cv_func_mmap_fixed_mapped=no \
  ac_cv_func_getcontext=no ac_cv_func_setcontext=no ac_cv_func_makecontext=no \
  ac_cv_header_ucontext_h=no ac_cv_header_sys_mman_h=no ac_cv_header_dlfcn_h=no \
  rb_cv_binary_elf=yes \
  > "$CONFIG_LOG" 2>&1

CONFIG_RC=$?
if [ "$CONFIG_RC" -ne 0 ] || [ ! -f Makefile ]; then
  echo ">>> CONFIGURE FALLO"; tail -30 "$CONFIG_LOG"; exit 1
fi
echo ">>> Configure OK"

echo ""
echo "=================== MAKE ==================="
# extmk.rb copia flags de LDFLAGS a EXTLDFLAGS. Si el make recursivo
# conserva ambos, GCC pasa dos veces switch.ld a ld y solapa .tls.align/.text.
# El Makefile principal ya contiene esos flags. Conservamos la unica opcion
# adicional generada por extmk.rb: compresion de secciones de depuracion.
make -j6 LD=aarch64-none-elf-ld EXTLDFLAGS=-Wl,--compress-debug-sections=zlib > "$MAKE_LOG" 2>&1
RC=$?
echo "codigo de salida: $RC"
echo ""
echo "--- primeros errores ---"
grep -nE "error:|undefined reference|Error [0-9]" "$MAKE_LOG" | head -20
echo ""
echo "=================== ARTEFACTOS ==================="
echo "objetos .o : $(ls "$BUILD"/*.o 2>/dev/null | wc -l)"
if [ "$RC" -eq 0 ] && [ -s "$BUILD/libruby-static.a" ]; then
  ls -la "$BUILD/libruby-static.a"
  echo ">>> libruby-static.a: make completo OK"
elif [ -s "$BUILD/libruby-static.a" ]; then
  ls -la "$BUILD/libruby-static.a"
  echo ">>> libruby-static.a parcial: make fallo; no usar como compilacion validada"
else
  echo ">>> falta libruby-static.a"
  [ "$RC" -ne 0 ] || RC=1
fi
exit "$RC"
