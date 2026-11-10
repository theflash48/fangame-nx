# Informe M014 — retorno a FangameNX y limpieza de versión anterior

Fecha: 10 de noviembre de 2026. Rama: `feature/fangamenx-launcher-mvp`.
Alcance: prueba física ya realizada, lectura posterior de logs por MTP y
retirada del prototipo antiguo de la SD. La implementación y compilación de
M014 se describen en [retorno-menu-m014.md](retorno-menu-m014.md).

## Resultado observado

- El propietario abrió `fnx009` en Switch, pulsó B dentro del juego y confirmó
  el retorno a la biblioteca. Después pulsó + en FangameNX y confirmó la
  salida a Homebrew Menu: **«Sí, funcionan B y +»**. Consta **un ciclo físico**.
- La lectura MTP posterior conserva los ocho logs previos y añade una ejecución
  de `fnx009` y una nueva apertura del launcher. `fnx009-probe.log` termina en
  `boton=B salida_solicitada` y `FNX009 RUBY COMPLETADO; salida normal`.
  `mkxp-boot.log` registra `SDL_DestroyWindow OK`, `SDL_Quit OK`,
  `[exit] 08 before return main` y `[launcher] retorno al menu programado`.
  `mkxp-debug.log` añade `Shutting down.`. `launcher.log` registra otro
  `inicio: launcher`, `scan: terminado` y `salida: hbmenu`.
- Estas trazas y el relato del propietario concuerdan con B → cierre normal del
  juego → nueva ejecución del NRO → biblioteca → + → Homebrew Menu. El log
  confirma la programación del siguiente NRO; la visualización de la
  biblioteca y la salida con + proceden de la prueba comunicada por el
  propietario.

## Evidencia preservada y limpieza

- Snapshot MTP final: `local/m014/preupdate-20261110-104428/`, nueve archivos
  con SHA-256 en `manifest.json`; se recalcularon los nueve hashes, cero
  diferencias. Incluye `logs/launcher.log`, los logs de `fnx009` y `fnx017`, y
  el `mkxp-debug.log` de la raíz. El log raíz solo registra el arranque del
  launcher (`argc = 1` y ruta del NRO), sin diagnóstico adicional. El snapshot
  anterior al cambio sigue en `local/m014/preupdate-20261110-102547/`.
- Instalación activa: `SD Card/switch/FangameNX/FangameNX.nro`, build `0.2.4
  b20`, SHA-256 comprobado por lectura MTP tras instalar:
  `bfa63f21d44637984866b19180553c81fc6d76d40e4a59cdd281db1c9e64c049`.
  El NRO anterior, SHA-256
  `b4b93af91e3bf6ce49fa6c111e55eb7d2ae49becb6eba77f0beaced56276b6a6`,
  está conservado en `local/m014/old-nro/`. El archivo temporal de instalación
  se eliminó tras verificar la sustitución.
- Se inventarió `SD Card/switch/fnx016-sdl-fix`, prototipo antiguo ajeno a la
  instalación actual. Sus 12 archivos se copiaron íntegros a
  `local/m014/obsolete-fnx016-20261110-104254/`; los 12 hashes del
  `manifest.json` coinciden con los archivos archivados. Se eliminó después
  **esa carpeta exacta** por MTP. La nueva enumeración filtrada de
  `SD Card/switch/` solo muestra `FangameNX` (cinco elementos: `config/`,
  `games/`, `logs/`, `FangameNX.nro` y `mkxp-debug.log`). No se tocaron otros
  directorios de Switch.
- Comandos/registros locales: `local/m014/mtp-final-snapshot.log`,
  `local/m014/archive-remove-fnx016.log`, `local/m014/mtp-snapshot.ps1`,
  `local/m014/archive-remove-fnx016.ps1`. Entorno: Windows PowerShell, MTP
  `Nintendo Switch`, consola física del propietario. Esos archivos quedan
  fuera del repositorio público por la lista blanca de `.gitignore`.

## Límites y siguiente paso

- Los logs de `fnx017` no crecieron tras instalar M014; falta repetir en la
  build actual la secuencia juego → B → biblioteca → + con ese fixture.
- La nueva traza de `fnx009` no registra A ni X. La incidencia intermitente de
  entrada X y el pánico de M012 siguen pendientes; esta prueba no los resuelve.
  Tampoco se ha probado la ruta sin `envHasNextLoad` ni una serie de ciclos
  repetidos.
- Próxima comprobación aconsejada: ejecutar `fnx017` y varios ciclos de
  `fnx009`/`fnx017`, leer de nuevo logs por MTP y contrastar que cada cierre
  normal programa el retorno y cada launcher vuelve a iniciar. Después abordar
  la incidencia de entrada M012 con una reproducción acotada.

Para las siguientes versiones: antes de sustituir el NRO, leer los logs y
archivar la versión activa con SHA-256; instalar y verificar el NRO nuevo por
lectura MTP; retirar el archivo temporal y cualquier instalación obsoleta
identificada, tras archivar su contenido. Mantener un único NRO activo de
FangameNX en la SD y documentar por separado lo probado en hardware.
