# M015 — recordar el último juego al volver a la biblioteca

Fecha: 10 de noviembre de 2026. Rama: `feature/fangamenx-launcher-mvp`.
Solicitud del propietario: tras cerrar un juego con B, situar el cursor en el
último juego iniciado; conservar la selección al cerrar y abrir FangameNX.

## Prueba previa M014

El propietario confirmó en Switch dos ejecuciones correctas de `fnx017`
(Órbita doble), primero la escena B y después la A, con A/X/B y retorno a la
biblioteca. Snapshot MTP de solo lectura:
`local/m014/preupdate-20261110-110235/`, 14 archivos; se recalcularon sus
SHA-256 y coincidieron todos. Frente al snapshot anterior, los cuatro logs
compartidos conservan su prefijo exacto. `fnx017-selector.log` añade
`RUN 2 SELECT B` y `RUN 3 SELECT A`, ambos completados. Los `events.log` de
ambas escenas registran lectura/escritura OK, A, X, B y finalización Ruby;
cada bloque nuevo de `mkxp-boot.log` llega a `SDL_Quit OK`,
`[exit] 08 before return main` y `[launcher] retorno al menu programado`.
`launcher.log` añade dos selecciones de `fnx017`, dos nuevos inicios y la
salida final a hbmenu. Esta evidencia cubre dos ciclos físicos adicionales;
no cierra la incidencia intermitente de M012.

## Decisión e implementación

`config/launcher.json` admite ahora la clave opcional `last_game`, con el ID de
la carpeta del último juego iniciado. Se valida con las mismas reglas de ID
que usa el escáner. Al confirmar el lanzamiento de una carpeta revalidada y
preparada, el launcher guarda ese ID mediante escritura temporal y `fsync`,
preservando las demás claves del JSON. Un error de escritura queda
en `launcher.log` y no bloquea el arranque del juego. La configuración
malformada no se sobrescribe.

En el siguiente arranque, el escaneo busca ese ID en la biblioteca ordenada y
sitúa el foco allí. Si la carpeta ya no aparece, se selecciona el primer juego
disponible. Un reescaneo manual conserva la selección actual por ID siempre que
siga presente. El ajuste de lista compacta y los archivos de juego no cambian.
Se corrigieron también los textos de la interfaz que todavía indicaban M013 o
que el juego salía directamente a Homebrew Menu.

## Verificación antes de la prueba física

- Prueba host con el paquete de dos juegos: `launcher_host: OK`; valida lectura
  y escritura de `last_game`, rechazo de ID inseguro y conservación de claves
  desconocidas. Comando: `tests/run-launcher-host.cmd
  local/m013/package-bfa63f21d4`.
- Build incremental con `FNX_LAUNCHER=1` y libgcc Cortex-A57 aislada: código 0,
  NRO `0.2.4 b23`, SHA-256
  `d466e13275b293abbebbdcbb47ece808757f17f2e404cc52dd2fb65250f3df9a`.
  ELF SHA-256 `06e8cbc2c7fd54c4fbffe3639159aaf782f8cb9bb18d7da6b9f1fcfb86acdef4`.
  Log: `local/m015/build-final.log`.
- `auditar-armv8.py`: cero patrones conocidos incompatibles; 11 769 912 bytes
  ejecutables ELF/NRO idénticos. Los 25 hashes del paquete
  `local/m013/package-d466e13275/` coinciden con el manifiesto.
- Por MTP se archivó el NRO b20 anterior en `local/m015/old-nro/`, SHA-256
  `bfa63f21d44637984866b19180553c81fc6d76d40e4a59cdd281db1c9e64c049`.
  Se instaló b23 en `SD Card/switch/FangameNX/FangameNX.nro`, se leyó de vuelta
  y se verificó su SHA-256. El temporal remoto se retiró; el inventario final
  muestra solo una carpeta `FangameNX`. Log: `local/m015/mtp-update.log`,
  `UPDATE_VERIFIED FangameNX.nro`.

## Resultado físico de b23 y corrección b24

El propietario probó b23: tras iniciar Órbita doble y volver con B, el cursor
quedó en Pulso; tras + y nueva apertura, quedó de nuevo en Pulso. El snapshot
MTP `local/m014/preupdate-20261110-111149/` conserva 15 archivos, con 15
SHA-256 coincidentes. `config/launcher.json` seguía en
`{"schema":1,"compact":false}` y `launcher.log` registró dos veces
`config: no se pudo recordar el ultimo juego`. El juego sí ejecutó y cerró
normalmente. La verificación host no había reproducido esta escritura sobre
el sistema de archivos de Horizon.

La primera hipótesis es que el renombrado del temporal sobre
`launcher.json` existente falla en Horizon: el `fsdev_rename` de
[libnx](https://github.com/switchbrew/libnx/blob/master/nx/source/runtime/devices/fs_dev.c)
delega en `fsFsRenameFile`, sin sustitución explícita; la prueba física no
identificó aún la llamada concreta que falló. La b24 conserva el archivo
anterior como `launcher.json.bak` antes de renombrar el temporal y, si el
proceso se interrumpe entre renombres, las lecturas aceptan el respaldo.
La prueba host verifica recuperación desde ese respaldo. Un fallo nuevo
registra el valor `errno` para acotar el diagnóstico.

Por petición del propietario, b24 añade entrada táctil al launcher. Un toque
en una tarjeta no seleccionada mueve el foco; otro toque abre detalles. Hay
botones visibles para iniciar, volver, reescanear, ajustes y salir. Los toques
usan `SDL_FINGERUP` y coordenadas convertidas al tamaño lógico 1280×720; los
eventos de ratón sintetizados por el mismo toque se ignoran para evitar doble
acción. Esta parte compila, pero requiere prueba física. SDL documenta las
[coordenadas normalizadas de los dedos](https://wiki.libsdl.org/SDL2/SDL_TouchFingerEvent).

Build b24: `local/m015/build-touch-fix.log`, código 0; NRO SHA-256
`b4826253547d737b43a6b44fc61b34cb8f058e53c25422a7b6566e01911fa427`;
ELF SHA-256 `caf361bc9037e49acb5b4c1f063026de739439ec238b6770cd9932f0a3be5588`.
La auditoría halló cero patrones incompatibles conocidos y comparó 11 773 624
bytes ejecutables idénticos entre ELF y NRO. `launcher_host: OK`; los 25
archivos del paquete `local/m013/package-b482625354/` coinciden con su
manifiesto. Por MTP, b23 se archivó en `local/m015/old-nro-b23/` y se sustituyó
por b24, verificada por lectura de vuelta. Se retiró el temporal remoto;
`local/m015/mtp-update-b24.log` termina en `UPDATE_VERIFIED FangameNX.nro`.

## Primer ensayo físico de b24 y pánico dentro del juego

El propietario confirmó que la entrada táctil respondió en la biblioteca y
que logró entrar en Órbita doble. **Ya dentro de la escena B**, tocó la
pantalla por curiosidad y ocurrió un pánico de Atmosphère. No se estaba
probando entrada táctil del motor dentro del juego; esa ruta pertenece al
parche previo `mkxpz-tactil.patch`, no al manejo táctil nuevo del launcher.
Por ese pánico no pudo comprobar el foco tras B ni tras volver a abrir la app.

El snapshot MTP de solo lectura `local/m015/posttest-20261110-112400/`
contiene 15 archivos con 15 SHA-256 coincidentes. `config/launcher.json`
contiene `{"compact":false,"last_game":"fnx017","schema":1}`; no quedaron
`.bak` ni `.tmp`. Los logs del lanzamiento b24 conservan el prefijo previo:
`launcher.log` añade la selección de `fnx017` y la salida al motor;
`fnx017-selector.log` añade `RUN 6 SELECT B` sin `COMPLETADO`; el log de la
escena B registra lectura y escritura OK, pero no botones ni finalización Ruby;
`mkxp-boot.log` termina tras el pico de montón de 199 MB, sin marcas `[exit]`.
Esto acredita persistencia del ID y una interrupción durante el juego, no
prueba todavía que el cursor se restaure visualmente.

Se preservaron por MTP, sin borrar originales, tres informes AFE2 que no
estaban catalogados en los registros anteriores y dos logs recientes de
`atmosphere/crash_reports/`: `local/m015/panic-evidence-20261110-1124/`,
cinco hashes verificados en `manifest.json`. Uno de los AFE2,
`report_77bdef7113.bin`, contiene error `0xFFE` y Title ID
`0100000000000034` (`fatal`), igual que el pánico M012; no identifica la
causa inicial ni demuestra que el toque provocara una instrucción concreta
del motor. Los dos logs de `crash_reports` corresponden a procesos del
sistema (`pcv` y `Bus`); tampoco contienen una pila simbolizada de FangameNX.
La incidencia de tacto dentro del juego y la estabilidad M012 siguen abiertas.

## Segundo ensayo físico de b24

Después del reinicio, el propietario volvió a abrir FangameNX y **observó
Órbita doble seleccionado desde el inicio**. Esto confirma visualmente la
restauración del foco al abrir la app. Al intentar iniciar Órbita doble ocurrió
otro pánico de Atmosphère, esta vez **sin tocar la pantalla dentro del juego**.
No se llegó a B ni al retorno a la biblioteca, de modo que ese tramo sigue sin
verificarse en b24.

El snapshot MTP de solo lectura `local/m015/posttest-20261110-113200/`
contiene 15 archivos con 15 SHA-256 coincidentes. El nuevo bloque de
`launcher.log` llega a `seleccion: fnx017` y `salida: motor seleccionado`, pero
`fnx017-selector.log` no cambió. `mkxp-boot.log` registra arranque como applet,
montón de **248 MB** y `diagAbortWithResult res=0x2b59` antes de Ruby y de los
eventos del juego. La causa demostrada de este **segundo** fallo es iniciar el
motor con el presupuesto de memoria reducido; no se reproduce así el primer
pánico, que había ocurrido dentro de la escena B con memoria de aplicación.
Los dos informes nuevos de Atmosphère (`hbloader` y `qlaunch`) se copiaron sin
modificar los originales a `local/m015/panic-evidence-20261110-1132/`, con
dos SHA-256 verificados. No apareció un AFE2 nuevo en este segundo ensayo.

## Protección b25 y verificación pendiente

Antes de entregar el control al motor, el launcher comprueba el montón
concedido. Si el NRO tiene menos de 1 GB, permanece en detalles y muestra una
explicación para abrir Homebrew Menu con memoria de aplicación manteniendo R
al iniciar un juego instalado. Registra `arranque bloqueado: memoria de applet`.
El mismo criterio de montón ya se utilizaba para el aviso del motor. El
encabezado de la interfaz indica `MEMORIA LIMITADA` en este modo. No se ha
modificado la entrada táctil del motor dentro del juego.

Build incremental b25: `local/m015/build-applet-guard.log`, código 0; NRO
SHA-256 `bbf9d4fee998703903f10694c04b91a1e5dd01c0769e09cba79c119bd3b5e299`;
ELF SHA-256 `61f71eda7871ce62a8dc87daf85f29790d6ff7814a328b49e337b1e5c665f1c1`.
`auditar-armv8.py` encontró cero patrones incompatibles conocidos y comparó
11 774 200 bytes ejecutables ELF/NRO idénticos. El ejecutable de prueba host
dio `launcher_host: OK`; los 25 hashes del paquete
`local/m013/package-bbf9d4fee9/` coinciden con el manifiesto. El bloqueo de
applet aún requiere comprobación física, así como foco tras B y estabilidad
de Órbita doble en memoria de aplicación.

Con el MTP conectado, se leyó y archivó b24 en `local/m015/old-nro-b24/`
(SHA-256 `b4826253547d737b43a6b44fc61b34cb8f058e53c25422a7b6566e01911fa427`).
Se retiró ese NRO de `SD Card/switch/FangameNX/` y se instaló b25 como
`FangameNX.nro`. Tanto el temporal de instalación como el archivo final se
leyeron de vuelta y coincidieron con el hash b25. El temporal remoto se
eliminó; `local/m015/mtp-update-b25.log` termina en
`UPDATE_VERIFIED FangameNX.nro`. No se tocaron juegos ni configuración.

El informe de traspaso para ChatGPT se aplaza mientras queden pruebas del
encargo original.
