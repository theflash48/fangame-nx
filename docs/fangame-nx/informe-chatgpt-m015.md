# Informe para ChatGPT · FangameNX M015

Fecha: 10 de noviembre de 2026. Rama local: `feature/fangamenx-launcher-mvp`.
Último commit de código: `58c609a`. No se hizo `push` ni publicación.

## Resultado ejecutivo

El primer launcher real de FangameNX está instalado y probado en Nintendo
Switch 1 con **dos ejemplos sintéticos**, Pulso (`fnx009`) y Órbita doble
(`fnx017`). Un único NRO descubre juegos externos, muestra biblioteca,
detalles y ajustes, inicia el motor con la carpeta elegida y vuelve a la
biblioteca tras el cierre normal de esos ejemplos. El propietario confirmó
que B dentro de los ejemplos vuelve a la biblioteca y + en FangameNX sale a
Homebrew Menu. El foco queda en el último juego iniciado después de B y
después de cerrar y reabrir la aplicación. La biblioteca responde al tacto.
Estas pruebas **no** establecen que B cierre cualquier fangame real: los
ejemplos implementan expresamente esa salida en Ruby.

El propietario eligió como primera integración de un juego real **Pokémon Z de
Eric Lostie**. Todavía no se ha instalado ni probado mediante este launcher.
No se han incorporado archivos del juego al repositorio.

## Versión activa y verificación

- NRO activo: `SD Card/switch/FangameNX/FangameNX.nro`, `0.2.4 b25`, SHA-256
  `bbf9d4fee998703903f10694c04b91a1e5dd01c0769e09cba79c119bd3b5e299`.
  La lectura de vuelta por MTP coincide. El NRO b24 anterior quedó archivado
  localmente antes de retirarlo de la Switch; se eliminó el temporal remoto.
- Build con devkitPro, `FNX_LAUNCHER=1` y libgcc aislada Cortex-A57:
  `local/m015/build-applet-guard.log`, código 0. ELF SHA-256
  `61f71eda7871ce62a8dc87daf85f29790d6ff7814a328b49e337b1e5c665f1c1`.
  La auditoría encontró cero patrones incompatibles conocidos y comparó
  11 774 200 bytes ejecutables ELF/NRO idénticos. La prueba host terminó
  `launcher_host: OK`; los 25 archivos del paquete local
  `local/m013/package-bbf9d4fee9/` coinciden con su manifiesto.
- Prueba física con memoria de aplicación: el propietario observó Órbita
  doble seleccionado, lo abrió, volvió con B y confirmó que seguía
  seleccionado. Tras + y una nueva apertura seguía seleccionado. El snapshot
  MTP `local/m015/posttest-20261110-114332/` tiene 15 hashes verificados.
  `fnx017-selector.log` añade `RUN 7 SELECT B` completado; el motor registra
  montón de 3262 MB, `SDL_Quit OK` y retorno programado al launcher.
- Prueba física como applet: el propietario confirmó aviso recuperable al
  intentar iniciar Órbita doble, retorno con B y salida con + sin pánico.
  El snapshot `local/m015/posttest-20261110-114828/` tiene 15 hashes
  verificados. `launcher.log` registra
  `arranque bloqueado: memoria de applet`; los logs del motor y del juego
  permanecen idénticos al snapshot anterior.
- `config/launcher.json` conserva `last_game: fnx017`; no hay `.bak` ni `.tmp`
  pendientes. El archivo de seguimiento detallado es
  `docs/fangame-nx/ultimo-juego-m015.md`.

## Incidencias y límites

La primera caída reciente de Atmosphère sucedió **dentro** de Órbita doble,
después de tocar la pantalla en el juego. El orden temporal está observado;
la causa no está demostrada. Ese toque no ejercitaba la nueva entrada táctil
del launcher, sino la ruta táctil previa del motor. La segunda caída ocurrió
al intentar iniciar el juego con solo 248 MB de montón en modo applet; los
logs terminan antes de Ruby. La b25 bloquea ahora ese lanzamiento y su
comportamiento recuperable quedó probado. La segunda causa no explica la
primera. La incidencia M012 de X perdido en uno de cinco ciclos y el pánico
anterior de Atmosphère siguen abiertos. No se afirma estabilidad general ni
compatibilidad con fangames completos en hardware.

## Base existente para Pokémon Z

Ya existe `juegos/pokemon-z/perfil.json` para **Pokémon Z 2.13**, familia
`ruby18`, y `docs/pruebas/pokemon-z.md` documenta una ejecución anterior en
Ryujinx con el empaquetado antiguo: título, introducción, mapa 66, menú y
guardado. `README.md` lo resume como jugable **en ese entorno y alcance**;
no se probó el combate. Esa evidencia es útil para reutilizar shims y
diagnóstico, pero no valida la ruta nueva
`sdmc:/switch/FangameNX/games/pokemon-z/` ni esta build en Switch física.
El perfil es una instrucción para el empaquetador heredado; el launcher lee
`fangamenx.json`, `Game.ini` y la estructura preparada de la carpeta, no el
perfil directamente. `docs/fangame-nx/desarrollo-launcher-m013.md` describe el
contrato actual. Ningún juego real está dentro del paquete M015.

## Encargo recomendado para el siguiente chat

Integrar una **copia legítima y autorizada** de Pokémon Z 2.13 en la
biblioteca privada de FangameNX, sin publicar ni incorporar recursos o
scripts extraídos al repositorio. Identificar la ubicación de esa copia y
confirmar su versión antes de preparar archivos. Reutilizar el perfil y las
correcciones documentadas; comprobar cómo adaptar la salida del empaquetador
antiguo al contrato `games/<id>/` del launcher, conservando guardados y logs
anteriores. Preparar la carpeta fuera de Git, validar su estructura y metadatos
con el escáner, y ensayar primero el arranque con memoria de aplicación. Medir
por separado título, controles, audio, cambio de mapa, guardado/carga y salida
segura. Si hay un fallo, preservar el estado y leer los logs por MTP antes de
cambiar otra cosa. No atribuir al juego una salida con B que no haya
implementado ni probado.

Antes de instalar cualquier versión nueva del NRO: leer logs, archivar el NRO
activo con SHA-256, retirarlo, instalar el nuevo, verificarlo por lectura MTP
y eliminar temporales, manteniendo un único NRO activo. Si la integración se
resuelve solo con archivos externos del juego, no recompilar el motor por
inercia. Mantener separados lo observado, lo inferido y lo pendiente.
