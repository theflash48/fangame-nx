# FNX-003 — entorno de compilación del motor heredado

Estado a 30/10/2026: **bloqueado antes del build**. Esta guía registra el estado observado en `build/fnx-003`, derivada de `audit/mision-001` (`fd3c2cd`). No hay un NRO de FangameNX producido en esta misión y FNX-003 no se considera terminado.

## Alcance y entorno elegido

El flujo documentado por el repositorio usa Windows, la shell MSYS2 de devkitPro y un SDK en `C:/devkitPro`. El Makefile pasa rutas Windows a GCC y `build-ruby-switch.sh` exige un Ruby anfitrión `.exe` de RubyInstaller. Por ello, la primera reproducción debe usar ese entorno. Ubuntu 24.04 en WSL existe pero no tiene devkitPro ni herramientas de compilación; Docker CLI existe, pero su daemon no responde. La CI Linux (`.github/workflows/nro.yml`) declara que nunca se ha ejecutado de extremo a extremo y usa una imagen `latest` sin digest. Ninguna de las alternativas ofrece hoy una versión reproducible demostrada.

**Plan de instalación, pendiente de autorización del usuario:** instalador oficial de devkitPro para Windows en `C:/devkitPro`, selección «Switch Development» (devkitA64, libnx y switch-tools), portlibs que satisfagan el enlace del Makefile y utilidades MSYS2 `make`, `cmake`, `autoconf`, `automake`, `libtool`, `pkg-config`, `zip` y `xxd`. Consultar los paquetes exactos disponibles antes de instalarlos, guardar versión de instalador, lista de paquetes (`pacman -Q`), URL de procedencia y SHA-256 de descargas. No se ha ejecutado este plan. [Instalador oficial](https://github.com/devkitPro/installer/releases), [paquetes oficiales](https://github.com/devkitPro/pacman-packages), [guía Switchbrew](https://www.switchbrew.org/wiki/Setting_up_Development_Environment).

## Inventario de dependencias

| Componente | Requisito observado | Estado / versión local |
| --- | --- | --- |
| Git, GitHub CLI, Python, PowerShell | Gestión y herramientas del repositorio | Git 2.53.0.windows.2; gh 2.100.0; Python 3.13.1; PowerShell 7.6.5. |
| Bash, `patch`, `xxd` | Shell y generación de shaders | Git Bash 5.2.37; `patch` y `xxd` presentes en Git Bash. |
| devkitPro / devkitA64 | `aarch64-none-elf-gcc/g++`, `ar`, `nm`, `ld` | No instalado. Versión pendiente. |
| libnx / switch-tools | `switch.specs`, `elf2nro`, `elf2nso`, `nacptool`, `npdmtool`, `build_romfs` | No instalados. Versiones pendientes. |
| Portlibs de Switch | SDL2, SDL2_image, SDL2_ttf, PhysFS, OpenAL, Theora/Vorbis/Ogg, mpg123, modplug, FreeType, HarfBuzz, curl, PNG/JPEG/WebP, zlib, bzip2, EGL/glapi/drm_nouveau | No instalados. El Makefile enumera bibliotecas, pero no versiones de paquete. |
| mkxp-z | Fuentes 2.4.2, bajo `src/mkxp-z`; `reconstruir-src.py` exige `src/mkxpz.tar.gz` y aplica parches | Descargados y extraídos sin parches. Tag `v2.4.2`, commit `290ffe1135142d91a40b8f295a7b8e284f432c9a`; SHA-256 del tarball `4c2df25e30139678307a8d072c325dc94c01ebc024c47d51eee17dd6f2ee1aac`. |
| CRuby | Fuentes 3.1.7 y parches; RubyInstaller anfitrión 3.1.4-1 x64 | Fuentes descargadas y extraídas sin parches; SHA-256 `0556acd69f141ddace03fa5dd8d76e7ea0d8f5232edf012429579bcdaab30e7b`. RubyInstaller ausente. |
| Dependencias portadas localmente | libiconv 1.16, pixman, uchardet, SDL_sound; `libnxcompat.a`, `libruby-static.a` | Fuentes y archivos `.a` ausentes. Versiones/revisiones de pixman, uchardet y SDL_sound no fijadas en los scripts. |
| Build host | `make`, `cmake`, `autoconf`, `automake`, `libtool`, `pkg-config`, `zip` | No encontrados en Git Bash. |

No hay submódulos Git en este repositorio. El fichero `.gitmodules` de mkxp-z en `v2.4.2` está vacío. `src/` contiene ahora únicamente los dos tarballs y sus árboles extraídos, todo ignorado por Git; `out/` y `release/` siguen ausentes. Los parches están versionados (37 `mkxpz-*.patch` y 45 parches numerados de Ruby). El listado estático de bibliotecas procede de `mkxp-z-nx/Makefile:143-152`; la presencia de una biblioteca en la lista no demuestra que una versión actual de portlibs la provea ni que enlace correctamente.

## Procedimiento de reproducción propuesto

1. Verificar rama, limpieza del checkout, espacio libre y que `C:/devkitPro` no contiene una instalación previa. Instalar solo el plan autorizado. Ejecutar `pacman -Q`, `aarch64-none-elf-g++ --version` y versiones de herramientas desde la shell MSYS2 de devkitPro; guardar la salida íntegra en un log local y el inventario versionado, sin datos personales ni claves.
2. Las fuentes de mkxp-z 2.4.2 y CRuby 3.1.7 ya se obtuvieron de [GitHub upstream](https://github.com/mkxp-z/mkxp-z/releases/tag/v2.4.2) y [Ruby upstream](https://www.ruby-lang.org/en/news/2025/03/26/ruby-3-1-7-released/), respectivamente, y se verificaron por SHA-256. Obtener libiconv 1.16, pixman, uchardet, SDL_sound y RubyInstaller 3.1.4-1 de sus distribuidores oficiales. Fijar URLs, etiquetas/commits y SHA-256 antes de usarlas. Mantener `src/` fuera de Git. No usar `master` o `latest` como sustituto de una revisión fijada. Aplicar los parches sobre las fuentes extraídas y comprobar que `reconstruir-src.py` reconstruye exactamente `src/mkxp-z`; no dar por correctos los parches sin esa comparación.
3. Resolver de forma documentada las rutas fijas `C:/anilswitch` en `build-deps.sh`, `build-deps2.sh`, `build-ruby-switch.sh`, `finalize-ruby.sh` y `gen-xxd.sh`. **No ejecutar esos scripts sin corregir su destino:** contienen `rm -rf` y pueden escribir en otro proyecto. La solución debe ser portable y limitada a la cadena de build; no requiere cambiar el runtime. Antes de cualquier cambio en archivos existentes se dejará registrada la causa y el alcance.
4. Verificar que `patches/nxcompat.c` produce `out/lib/libnxcompat.a` y que las extensiones Ruby llegan a `out/nx/extobj/`. Los scripts documentados no incluyen un paso explícito que cree estos prerrequisitos; identificar y registrar el comando exacto antes de lanzar `build-ruby-switch.sh` o `finalize-ruby.sh`.
5. En la shell MSYS2, ejecutar en orden y registrar código de salida y logs completos: `./build-deps.sh`, `./build-deps2.sh`, `./build-ruby-switch.sh`, `./finalize-ruby.sh`, `./construir.sh`. El README propone los dos primeros con `&&`; no aceptar un `echo OK` como éxito sin comprobar archivos instalados y códigos de salida. `build-ruby-switch.sh` escribe `docs/configure-log.txt` y `docs/make-log.txt` (ignorados), por lo que hay que preservarlos para diagnóstico sin publicarlos por defecto.
6. Si sale un `.nro`, comprobar `rgss-switch.elf`, formato AArch64, tamaños y SHA-256; inspeccionar el ZIP, `motor/`, `tools/`, `lib/` y `juegos/` para excluir juegos, scripts extraídos, claves, iconos o capturas sin derechos claros. Conservar el ELF correspondiente a ese NRO. Una compilación y validación de formato **no equivalen** a una prueba en hardware.

## Bloqueos y riesgos observados

- El primer comando seguro `./construir.sh` termina con código 1 por ausencia de `aarch64-none-elf-g++`; log exacto en `log-fnx-003.md`.
- Siguen faltando libiconv, pixman, uchardet, SDL_sound, RubyInstaller, las bibliotecas locales y el SDK. Las fuentes presentes de mkxp-z y CRuby todavía son originales sin parches. El repositorio no fija revisiones de pixman, uchardet ni SDL_sound. Tampoco hay manifiesto de versiones de portlibs. La reproducibilidad exige fijarlas y guardar checksums.
- Los scripts de dependencias y Ruby apuntan a `C:/anilswitch`, fuera de este proyecto. `build-deps*.sh` recrea subdirectorios de `build/`; `construir.sh` vacía la carpeta de entrega. Ejecutarlos sin aislar rutas vulneraría el alcance de esta misión.
- `construir.sh` filtra la salida de `make` con `grep ... || true`; el código final del pipeline no refleja el fallo de `make`. La presencia de NRO/ELF se comprueba luego, pero es necesario conservar la salida completa de `make` para un diagnóstico confiable.
- `LICENCIAS.md` documenta las licencias principales; faltan versiones y avisos por cada portlib enlazado. OpenAL LGPLv2+ y el enlace estático requieren revisión específica antes de redistribuir. El ZIP heredado copia iconos/perfiles y herramientas de terceros; cada archivo debe inventariarse antes de publicar. Véase `auditoria-001.md` FNX-002.

## Relación con el Roadmap

FNX-003 permanece abierto hasta una compilación limpia reproducible. FNX-001 aporta el inventario del motor y los parches que deben reconstruirse. FNX-002 condiciona cualquier redistribución de artefactos. FNX-004 y FNX-005 (selector y arquitectura) no se implementan en esta misión.
