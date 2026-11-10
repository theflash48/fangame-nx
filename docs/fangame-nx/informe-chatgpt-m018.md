# Informe para ChatGPT · FangameNX M018

Fecha: 2026-11-10. Rama: `feature/fangamenx-launcher-mvp`.

## Procedencia del segundo juego (registrada antes de la descarga)

El segundo candidato real es **Pokémon Ópalo**, ofrecido sin pago por su autor EricLostie en su página [Lostie Fangames · Pokémon Ópalo](https://lostiefangames.blogspot.com/p/pokemon-opalo.html), enlace «DESCARGA DEL JUEGO». El enlace del autor apunta a `https://mega.nz/file/OvQFESAY#ZfOyD3l9ABwOZ0XgYx28a0yj1N9-oJar4AOJx0IZlZU`. La página lo presenta como «COMPLETO», pero no especifica una versión numérica ni una licencia de redistribución. Esta procedencia se registró antes de descargarlo. La descarga fue solo para preparación local y uso personal: ningún recurso ni script del juego entró en Git.

## Resultado ejecutivo

La autorización MTP continua quedó escrita en `AGENTS.md`: identificación de la Switch correcta, copia recuperable antes de sustituir, lectura de vuelta y protección de partidas, otras instalaciones y archivos de sistema. No habrá que pedir permiso MTP en cada misión mientras el propietario no revoque esta autorización.

El NRO **b27 está instalado**, y el segundo juego real **Pokémon Ópalo 2.11 está copiado a una carpeta nueva**. Ambos resultados están verificados por lectura MTP y SHA-256. La ejecución de b27 y Ópalo 2.11 **todavía no está probada por el propietario en consola**; el perfil histórico `juegos/pokemon-opalo/perfil.json` documenta una prueba de otra instalación anterior, que no sustituye a esta prueba.

## b27: despliegue y recuperación

- Se identificó un único dispositivo portátil Nintendo Switch presente y correcto por WPD. La unidad fija `D:` no se confundió con MTP; `D:\Descargas` solo contenía Pokémon Z como fangame RPG Maker XP utilizable. No se alteró esa fuente.
- Se comprobaron los 25 archivos del paquete b27 `local/m013/package-fe34daaf94/` contra su manifiesto. Antes del cambio se leyó de vuelta el b25 activo a `local/m018/old-nro-b25/FangameNX.nro`, SHA-256 `bbf9d4fee998703903f10694c04b91a1e5dd01c0769e09cba79c119bd3b5e299`.
- Se copió b27 con nombre temporal, se leyó de vuelta y se verificó su SHA. Solo después se sustituyó `SD Card/switch/FangameNX/FangameNX.nro`; el archivo final se leyó de vuelta con SHA-256 `fe34daaf94e7522a19fb04f46f2da8fa3276a8da1a5cd8ae0d00e9d7b017a3bd`. El temporal se retiró. Evidencia privada: `local/m018/mtp-update-b27.log` y las carpetas de lectura `local/m018/stage-readback-b27/`, `final-readback-b27/`.
- La copia b25 permanece recuperable en PC. No se modificaron Pokémon Z, sus partidas, otros juegos, Atmosphère ni archivos de sistema. La prueba de arranque, navegación y retorno de b27 en Switch sigue pendiente.

## Segundo juego: diagnóstico, paquete e instalación

- Descarga desde el enlace del autor mediante MEGAcmd oficial 2.7.0, extraído en `local/m018/tools/`, sin incorporar sus binarios a Git. El instalador oficial tenía firma Authenticode válida. Archivo `local/m018/official-opalo/OPALO V2.11.zip`, SHA-256 `036564c290fd4aa6a8727f9a9e458c37e625a5e50d3995385c1bc268c4dac71f`. El nombre del ZIP aporta la versión **2.11**; la página del autor no la indica.
- `python packaging/prepare-launcher-game.py 'local/m018/official-opalo/OPALO V2.11.zip' --diagnose`: perfil `pokemon-opalo`, familia `ruby18`, scripts `Data/Scripts.rxdata`, **16 607** archivos fuente, **cero problemas** y dos advertencias: scripts antiguos susceptibles de transformación NXFuente y nombres no ASCII que exigen comprobar MTP.
- Se preparó con `--profile pokemon-opalo --id pokemon-opalo --name 'Pokémon Ópalo' --author EricLostie --version 2.11 --output local/m018/prepared/pokemon-opalo`. El staging contiene 114 archivos, 16 557 recursos dentro de `datos.zip` y `cover.png` derivada de `Graphics/Pictures/titulo.png`. El manifiesto privado de staging enumera hashes. `packaging/install-launcher-game.py` verificó por hashes la copia a `local/m018/library/switch/FangameNX/games/pokemon-opalo/` y dejó `fangamenx.json` con `cover: cover.png`. `datos.zip` mide 666 186 609 bytes y tiene SHA-256 `5782bfbf607044e9473395b8a78f92ed7ca63bddbc8475619fc55e176cbf9263`.
- MTP copió el juego a la **nueva** ruta `SD Card/switch/FangameNX/games/pokemon-opalo/`. La primera pasada mostró 113/114: omitió el TXT `GUÍA SI EL JUEGO TE VA MAL O NO ABRE.txt` por su nombre con tilde. Se conservó el staging original y se renombró únicamente ese TXT en la copia local de instalación como `GUIA SI EL JUEGO TE VA MAL O NO ABRE.txt`; se transfirió el archivo faltante. La lectura MTP completa terminó con **114/114 archivos y 667 389 757 bytes**, y los 114 SHA-256 coincidieron con la copia local. Evidencia privada: `local/m018/mtp-install-opalo.log`, `mtp-install-opalo-resume.log` y `mtp-readback-opalo/`.
- El escáner del launcher descubre automáticamente los subdirectorios seguros de `games/`; `pokemon-opalo` contiene `fangamenx.json`, `mkxp.json`, `lib/nx-compat.rb`, `lib/socket.rb`, `datos.zip` y `Game.rgssad`. La portada está declarada y leída del archivo local del juego. Esto acredita la estructura de instalación, no que la portada se haya visto ya en pantalla.

## Verificación, límites y siguiente prueba

Se usaron PowerShell, Python 3.13.1, MEGAcmd 2.7.0 y MTP/WPD de Windows. Los diagnósticos, manifiestos, hashes locales y lecturas MTP anteriores son las pruebas de esta misión. No se recompiló el motor: b27 ya estaba construido y auditado en M017. No se añadieron recursos, scripts, ZIP, binarios de MEGA ni datos personales al repositorio. No se hizo `push`.

Queda que el propietario inicie **FangameNX b27** en la consola, compruebe que aparecen **Pokémon Z y Pokémon Ópalo con sus portadas**, abra Ópalo 2.11, pruebe movimiento/sonido/guardado/carga/retorno y comunique los registros si surge un fallo. La X intermitente y el pánico tras uso táctil siguen abiertos; esta instalación no demuestra que estén corregidos. La ejecución física de Ópalo 2.11 y la compatibilidad de su versión concreta también quedan pendientes.
