# FNX-003 — registro de comprobaciones

Fecha: 30/10/2026. Directorio: `C:/Users/pablo/Documents/GitHub/FangameNX`. Rama: `build/fnx-003`; base: `fd3c2cd94c98abc2623cc1c0c756e6e032d5a9a7`. Las comprobaciones de compilación son de lectura o se detienen antes de producir artefactos; únicamente se descargaron y extrajeron dos fuentes públicas dentro de `src/`, que está ignorado por Git. Las salidas se transcriben sin abreviar salvo donde se indica explícitamente.

## Estado y versiones

```text
> git branch --show-current
build/fnx-003
> git log -1 --format='%H %s'
fd3c2cd94c98abc2623cc1c0c756e6e032d5a9a7 Documentar auditoría técnica y viabilidad multijuego
> git status --short
(sin salida)
> git --version
git version 2.53.0.windows.2
> gh --version | Select-Object -First 1
gh version 2.100.0 (2026-09-03)
> python --version
Python 3.13.1
> pwsh --version
PowerShell 7.6.5
> docker --version
Docker version 29.7.2, build a7dcaa6
> bash --version | head -1   # Git Bash
GNU bash, version 5.2.37(1)-release (x86_64-pc-msys)
```

`git diff --check` y `git submodule status` no produjeron salida al inicio. `git ls-files .gitmodules` no produjo salida. `git ls-files --stage` no mostró entradas `160000`. Al comienzo `src/`, `out/`, `release/`, `C:/devkitPro` y `C:/msys64` no existían. `command -v` en Git Bash encontró `patch` y `xxd`; no encontró `make`, `cmake`, `ninja`, `autoreconf`, `gcc` ni `ruby`. En Windows no se encontró `aarch64-none-elf-g++`, `elf2nro`, `elf2nso`, `nacptool` ni `npdmtool`.

```text
> wsl -d Ubuntu-24.04 --exec sh -c 'cat /etc/os-release | head -n 3; which dkp-pacman aarch64-none-elf-g++ elf2nro make cmake ninja autoconf automake libtool ruby gcc g++ git; ls -ld /opt/devkitpro /opt/devkitpro/devkitA64 /opt/devkitpro/libnx 2>&1'
PRETTY_NAME="Ubuntu 24.04.4 LTS"
NAME="Ubuntu"
VERSION_ID="24.04"
/usr/bin/git
ls: cannot access '/opt/devkitpro': No such file or directory
ls: cannot access '/opt/devkitpro/devkitA64': No such file or directory
ls: cannot access '/opt/devkitpro/libnx': No such file or directory
[código de salida: 1]
```

`docker version --format '{{json .}}'` informó cliente 29.7.2, `Server: null` y ausencia de la canalización `dockerDesktopLinuxEngine`. No se arrancó Docker Desktop.

## Sintaxis y primera ejecución segura

```text
> bash -n build-deps.sh build-deps2.sh build-ruby-switch.sh finalize-ruby.sh gen-xxd.sh construir.sh && echo bash-syntax-OK
bash-syntax-OK
[código de salida: 0]

> ./construir.sh

=== 1. Herramientas y bibliotecas ===
falta aarch64-none-elf-g++: instala devkitPro con devkitA64 y switch-tools (https://devkitpro.org/wiki/Getting_Started)
[código de salida: 1]
```

La ejecución se detuvo en `construir.sh:40-42`, antes de `gen-xxd.sh`, `make` y la limpieza de `release/`. No se ejecutaron `build-deps*.sh`, `build-ruby-switch.sh` ni `finalize-ruby.sh`: sus rutas fijas apuntan a `C:/anilswitch` y faltan fuentes y SDK. No hay logs de compilación ni artefactos nuevos que inspeccionar.

## Descarga de fuentes públicas al directorio ignorado `src/`

```text
> curl.exe -L --fail --retry 2 --output src\mkxpz.tar.gz https://github.com/mkxp-z/mkxp-z/archive/refs/tags/v2.4.2.tar.gz
[código de salida: 0; 10,04 MiB recibidos]
> curl.exe -L --fail --retry 2 --output src\ruby-3.1.7.tar.gz https://cache.ruby-lang.org/pub/ruby/3.1/ruby-3.1.7.tar.gz
[código de salida: 0; 19,84 MiB recibidos]
> (Get-FileHash src\mkxpz.tar.gz -Algorithm SHA256).Hash
4C2DF25E30139678307A8D072C325DC94C01EBC024C47D51EEE17DD6F2EE1AAC
> (Get-FileHash src\ruby-3.1.7.tar.gz -Algorithm SHA256).Hash
0556ACD69F141DDACE03FA5DD8D76E7EA0D8F5232EDF012429579BCDAAB30E7B
```

El SHA-256 de Ruby coincide con el publicado por [Ruby upstream](https://www.ruby-lang.org/en/news/2025/03/26/ruby-3-1-7-released/). La etiqueta de mkxp-z resuelve al commit `290ffe1135142d91a40b8f295a7b8e284f432c9a` (`gh api repos/mkxp-z/mkxp-z/git/ref/tags/v2.4.2`). El tarball se extrajo con `tar -xzf ... --strip-components=1` a `src/mkxp-z` y `src/ruby-3.1.7`; `src/mkxp-z/src/main.cpp` y `src/ruby-3.1.7/configure` existen. Todavía no se han aplicado parches. `git check-ignore -v` confirma que ambos tarballs están ignorados por la regla `/*` y `git status --short` solo muestra los documentos de esta misión y la modificación de `.gitignore`.

## Continuación autorizada: instalador y fuentes

El usuario autorizó la instalación Windows de devkitPro, paquetes Switch y utilidades MSYS2. Se descargó `devkitProUpdater-3.0.3.exe` desde la [publicación oficial](https://github.com/devkitPro/installer/releases/tag/v3.0.3). SHA-256 local: `038a99dc84f1ca0b52e9e0e074a94a3b0672e6d7bf0988563f0ab0812dcbb38d`. `Get-AuthenticodeSignature` devolvió `Valid`, firmante `Open Source Developer, David Murphy`. La ventana `devkitProUpdater 3.0.3` se abrió, pero no aceptó clics ni teclado desde el control de Windows; el proceso aparece elevado y el agente corre con nivel de integridad medio. Se solicitó al usuario completar la interfaz seleccionando solo «Minimal System» y «Switch Development». **El instalador todavía no ha instalado nada según `Test-Path C:/devkitPro`: `False`.**

Se descargaron los siguientes archivos a `src/` y se extrajeron en las rutas que esperan los scripts. `tar -tf` confirmó las entradas de build indicadas.

```text
libiconv-1.16.tar.gz       e6a1b1b589654277ee790cce3734f07876ac4ccfaecbee8afa0b649cf529cc04  -> src/libiconv-1.16 (configure)
uchardet-0.0.8.tar.xz     e97a60cfc00a1c147a674b097bb1422abd9fa78a2d9ce3f3fdcc2e78a34ac5f0  -> src/uchardet (CMakeLists.txt)
SDL2_sound-2.0.6.tar.gz   63be57cc80bcd29aeed6c29700759871c44c90251a7508b8d0eacd8d9cece30c  -> src/SDL_sound (CMakeLists.txt)
pixman-0.42.2.tar.gz      ea1480efada2fd948bc75366f7c349e1c96d3297d09a3fe62626e38e234a625e  -> src/pixman (configure)
rubyinstaller-3.1.4-1-x64.7z 6701088607ea4b587a31af76d75cb3fe9f7bcd75fc175cffcca22369ebb6331d
> src/rubyinstaller-3.1.4-1-x64/bin/ruby.exe --version
ruby 3.1.4p223 (2023-03-30 revision 957bb7cb81) [x64-mingw-ucrt]
```

Los hashes de uchardet y pixman coinciden con los manifiestos `.sha256` y `.sha512` de sus distribuidores; el de SDL_sound coincide con el digest publicado en GitHub. Las versiones de pixman, uchardet y SDL_sound se eligieron para esta reproducción porque el repositorio heredado no fija sus revisiones; aún no se han compilado.

## Parches y rutas

`patch -p1` sobre el primer parche mkxp-z falló con `No file to patch` y 24 hunks ignorados; no alteró `src/mkxp-z`. Registro íntegro: [log-mkxp-intento-p1.txt](log-mkxp-intento-p1.txt). `python reconstruir-src.py` falló primero porque `patch` no estaba en PATH de Python ([log-reconstruir-sin-patch.txt](log-reconstruir-sin-patch.txt)). Con Git `usr/bin` en PATH dedujo que los 37 parches aplican al tarball, pero la comparación con el árbol aún original produjo 39 diferencias ([log-reconstruir-preparacion.txt](log-reconstruir-preparacion.txt)).

Se añadió `aplicar-parches-mkxp.py`, que utiliza `donde_aplicar` de `reconstruir-src.py` para elegir directorio y nivel `-p` por parche. Su ejecución terminó con código 0; `reconstruir-src.py` informó `39 fichero(s) salen identicos, 0 se desvian, 0 sin parche`. Registro íntegro: [log-aplicar-mkxp.txt](log-aplicar-mkxp.txt). `aplicar-parches-ruby.py` aplicó los 45 parches numerados con código 0 y apartó 10 generados obsoletos a `src/ruby-generados-originales`; registro íntegro: [log-aplicar-ruby.txt](log-aplicar-ruby.txt). No se modificaron archivos versionados del runtime.

Los scripts `build-deps.sh`, `build-deps2.sh`, `build-ruby-switch.sh`, `finalize-ruby.sh`, `gen-xxd.sh` y `make-resume.sh` ahora derivan la raíz de su propia ubicación. `mkxp-z-nx/Makefile` ya no usa `C:/rgss-switch` como destino de respaldo. `construir.sh` ahora guarda `make` completo en `src/build-logs/` y respeta su código de salida. Antes de ejecutar los scripts, se comprobó su sintaxis:

```text
> bash -n build-deps.sh build-deps2.sh build-ruby-switch.sh finalize-ruby.sh gen-xxd.sh make-resume.sh construir.sh && echo syntax-OK
syntax-OK
[código de salida: 0]
> cygpath -m "$(pwd)"
C:/Users/pablo/Documents/GitHub/FangameNX
```

Estas comprobaciones no prueban aún el enlace ni el arranque del motor.
