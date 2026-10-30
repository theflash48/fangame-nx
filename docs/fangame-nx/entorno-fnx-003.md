# FNX-003 — entorno de compilación del motor heredado

Estado a 30/10/2026: **bloqueado antes del build**. Esta guía registra el estado observado en `build/fnx-003`, derivada de `audit/mision-001` (`fd3c2cd`). No hay un NRO de FangameNX producido en esta misión y FNX-003 no se considera terminado.

## Alcance y entorno elegido

El flujo documentado por el repositorio usa Windows, la shell MSYS2 de devkitPro y un SDK en `C:/devkitPro`. El Makefile pasa rutas Windows a GCC y `build-ruby-switch.sh` exige un Ruby anfitrión `.exe` de RubyInstaller. Por ello, la primera reproducción debe usar ese entorno. Ubuntu 24.04 en WSL existe pero no tiene devkitPro ni herramientas de compilación; Docker CLI existe, pero su daemon no responde. La CI Linux (`.github/workflows/nro.yml`) declara que nunca se ha ejecutado de extremo a extremo y usa una imagen `latest` sin digest. Ninguna de las alternativas ofrece hoy una versión reproducible demostrada.

**Plan de instalación autorizado por el usuario, pendiente de completar en la interfaz de Windows:** instalador oficial devkitProUpdater 3.0.3 en `C:/devkitPro`, selección exclusiva de «Minimal System» y «Switch Development» (devkitA64, libnx y switch-tools), portlibs que satisfagan el enlace del Makefile y utilidades MSYS2 `make`, `cmake`, `autoconf`, `automake`, `libtool`, `pkg-config`, `bison`, `zip` y `xxd`. El instalador se descargó y su firma Authenticode es válida; la ventana elevada no aceptó control automático, así que el usuario debe completar esa selección. Consultar paquetes disponibles antes de instalar los restantes y guardar `pacman -Q`, versiones y hashes. [Instalador oficial](https://github.com/devkitPro/installer/releases), [paquetes oficiales](https://github.com/devkitPro/pacman-packages), [guía Switchbrew](https://www.switchbrew.org/wiki/Setting_up_Development_Environment).

## Inventario de dependencias

| Componente | Requisito observado | Estado / versión local |
| --- | --- | --- |
| Git, GitHub CLI, Python, PowerShell | Gestión y herramientas del repositorio | Git 2.53.0.windows.2; gh 2.100.0; Python 3.13.1; PowerShell 7.6.5. |
| Bash, `patch`, `xxd` | Shell y generación de shaders | Git Bash 5.2.37; `patch` y `xxd` presentes en Git Bash. |
| devkitPro / devkitA64 | `aarch64-none-elf-gcc/g++`, `ar`, `nm`, `ld` | No instalado. Versión pendiente. |
| libnx / switch-tools | `switch.specs`, `elf2nro`, `elf2nso`, `nacptool`, `npdmtool`, `build_romfs` | No instalados. Versiones pendientes. |
| Portlibs de Switch | SDL2, SDL2_image, SDL2_ttf, PhysFS, OpenAL, Theora/Vorbis/Ogg, mpg123, modplug, FreeType, HarfBuzz, curl, PNG/JPEG/WebP, zlib, bzip2, EGL/glapi/drm_nouveau | No instalados. El Makefile enumera bibliotecas, pero no versiones de paquete. |
| mkxp-z | Fuentes 2.4.2, bajo `src/mkxp-z`; `reconstruir-src.py` exige `src/mkxpz.tar.gz` | Tag `v2.4.2`, commit `290ffe1135142d91a40b8f295a7b8e284f432c9a`; SHA-256 del tarball `4c2df25e30139678307a8d072c325dc94c01ebc024c47d51eee17dd6f2ee1aac`. Los 37 parches aplicaron; la reconstrucción compara 39 archivos idénticos, cero desviaciones. |
| CRuby | Fuentes 3.1.7 y 45 parches | Tarball SHA-256 `0556acd69f141ddace03fa5dd8d76e7ea0d8f5232edf012429579bcdaab30e7b`. Aplicaron los 45 parches; se apartaron 10 archivos generados obsoletos para que se regeneren al compilar. No hay compilación Ruby todavía. |
| RubyInstaller anfitrión | `src/rubyinstaller-3.1.4-1-x64/bin/ruby.exe` | Archivo 3.1.4-1 x64 de RubyInstaller, SHA-256 `6701088607ea4b587a31af76d75cb3fe9f7bcd75fc175cffcca22369ebb6331d`; ejecutable comprobado: `ruby 3.1.4p223 [x64-mingw-ucrt]`. |
| Dependencias portadas localmente | libiconv 1.16, pixman, uchardet, SDL_sound; `libnxcompat.a`, `libruby-static.a` | Fuentes descargadas y extraídas: libiconv 1.16, pixman 0.42.2, uchardet 0.0.8, SDL_sound 2.0.6. Archivos `.a` ausentes. |
| Build host | `make`, `cmake`, `autoconf`, `automake`, `libtool`, `pkg-config`, `bison`, `zip` | No encontrados en Git Bash. Bison 3.8.2 se menciona en el historial como herramienta de generación, pero no está instalado aquí. |

No hay submódulos Git en este repositorio. El fichero `.gitmodules` de mkxp-z en `v2.4.2` está vacío. Todas las fuentes se guardan en `src/`, ignorado por Git; `out/` y `release/` siguen ausentes. Los parches están versionados (37 `mkxpz-*.patch` y 45 parches numerados de Ruby). El listado estático de bibliotecas procede de `mkxp-z-nx/Makefile:143-152`; la presencia de una biblioteca en la lista no demuestra que una versión actual de portlibs la provea ni que enlace correctamente.

| Fuente adicional elegida para esta reproducción | Procedencia y SHA-256 local |
| --- | --- |
| libiconv 1.16 | [GNU](https://ftp.gnu.org/pub/gnu/libiconv/libiconv-1.16.tar.gz) — `e6a1b1b589654277ee790cce3734f07876ac4ccfaecbee8afa0b649cf529cc04` |
| pixman 0.42.2 | [Cairo/freedesktop](https://www.cairographics.org/releases/pixman-0.42.2.tar.gz) — `ea1480efada2fd948bc75366f7c349e1c96d3297d09a3fe62626e38e234a625e`; SHA-512 contrastado con el publicado por upstream. Esta versión contiene `configure`, que exige `build-deps.sh`. |
| uchardet 0.0.8 | [freedesktop](https://www.freedesktop.org/software/uchardet/releases/uchardet-0.0.8.tar.xz) — `e97a60cfc00a1c147a674b097bb1422abd9fa78a2d9ce3f3fdcc2e78a34ac5f0`, contrastado con el `.sha256` upstream. |
| SDL_sound 2.0.6 | [icculus](https://github.com/icculus/SDL_sound/releases/tag/v2.0.6) — `63be57cc80bcd29aeed6c29700759871c44c90251a7508b8d0eacd8d9cece30c`, igual al digest de GitHub; esta rama usa SDL2. |

Las versiones de pixman, uchardet y SDL_sound son una **selección nueva y fijada para la reproducción**: el repositorio heredado no anotó sus versiones originales. Aún hay que demostrar que compilan con este port.

## Procedimiento de reproducción propuesto

1. Verificar rama, limpieza del checkout, espacio libre y que `C:/devkitPro` no contiene una instalación previa. Instalar solo el plan autorizado. Ejecutar `pacman -Q`, `aarch64-none-elf-g++ --version` y versiones de herramientas desde la shell MSYS2 de devkitPro; guardar la salida íntegra en un log local y el inventario versionado, sin datos personales ni claves.
2. Las seis fuentes externas y RubyInstaller están descargados y extraídos en `src/`, con los hashes anteriores; mantenerlos fuera de Git. Desde un árbol limpio, ejecutar `python aplicar-parches-mkxp.py` y `python aplicar-parches-ruby.py` con GNU `patch` en PATH. Estos guiones no son idempotentes: se ejecutan una vez sobre los tarballs originales. El primero invoca `reconstruir-src.py` y exige comparación idéntica; el segundo aparta los archivos generados obsoletos de Ruby sin borrarlos.
3. Los scripts de build de esta rama ya derivan la raíz de su propia ubicación en vez de `C:/anilswitch`. El Makefile exige `cygpath` y no cae a `C:/rgss-switch`. `construir.sh` guarda la salida completa de `make` en un nombre único bajo `src/build-logs/` y respeta su código de salida. `bash -n` pasa; falta probarlos en el entorno devkitPro real. Antes de ejecutar `build-deps*.sh`, confirmar que `build/` y `out/` son directorios reales dentro de FangameNX.
4. Verificar que `patches/nxcompat.c` produce `out/lib/libnxcompat.a` y que las extensiones Ruby llegan a `out/nx/extobj/`. Los scripts documentados no incluyen un paso explícito que cree estos prerrequisitos; identificar y registrar el comando exacto antes de lanzar `build-ruby-switch.sh` o `finalize-ruby.sh`.
5. En la shell MSYS2, ejecutar en orden y registrar código de salida y logs completos: `./build-deps.sh`, `./build-deps2.sh`, `./build-ruby-switch.sh`, `./finalize-ruby.sh`, `./construir.sh`. El README propone los dos primeros con `&&`; no aceptar un `echo OK` como éxito sin comprobar archivos instalados y códigos de salida. `build-ruby-switch.sh` escribe `docs/configure-log.txt` y `docs/make-log.txt` (ignorados), por lo que hay que preservarlos para diagnóstico sin publicarlos por defecto.
6. Si sale un `.nro`, comprobar `rgss-switch.elf`, formato AArch64, tamaños y SHA-256; inspeccionar el ZIP, `motor/`, `tools/`, `lib/` y `juegos/` para excluir juegos, scripts extraídos, claves, iconos o capturas sin derechos claros. Conservar el ELF correspondiente a ese NRO. Una compilación y validación de formato **no equivalen** a una prueba en hardware.

## Bloqueos y riesgos observados

- El primer comando seguro `./construir.sh` termina con código 1 por ausencia de `aarch64-none-elf-g++`; log exacto en `log-fnx-003.md`.
- Faltan las bibliotecas compiladas y el SDK; la instalación autorizada está pendiente de terminar en Windows. Tampoco hay manifiesto de versiones de portlibs hasta disponer de `pacman -Q`.
- Las rutas antiguas `C:/anilswitch` se han eliminado de los scripts principales en esta rama; aún hay que verificar su ejecución. `build-deps*.sh` recrea subdirectorios de `build/`; `construir.sh` vacía la carpeta de entrega. Todos esos destinos deben permanecer dentro de FangameNX.
- El script heredado filtraba `make` con `grep ... || true`, ocultando su código de salida. Esta rama registra la salida completa y falla si `make` falla; el cambio todavía no se ha ejecutado con el SDK.
- `LICENCIAS.md` documenta las licencias principales; faltan versiones y avisos por cada portlib enlazado. OpenAL LGPLv2+ y el enlace estático requieren revisión específica antes de redistribuir. El ZIP heredado copia iconos/perfiles y herramientas de terceros; cada archivo debe inventariarse antes de publicar. Véase `auditoria-001.md` FNX-002.

## Relación con el Roadmap

FNX-003 permanece abierto hasta una compilación limpia reproducible. FNX-001 aporta el inventario del motor y los parches que deben reconstruirse. FNX-002 condiciona cualquier redistribución de artefactos. FNX-004 y FNX-005 (selector y arquitectura) no se implementan en esta misión.
