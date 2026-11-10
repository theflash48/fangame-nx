# FangameNX M013 — desarrollo del launcher

Fecha: 10 de noviembre de 2026. Rama: `feature/fangamenx-launcher-mvp`.
Código: `277be0d` y ajuste de escaneo `aa4c8f2`. Esta nota fija el contrato
nuevo que continúa ADR-004.

## Decisión integrada

La variante `FNX_LAUNCHER=1` produce `FangameNX.nro` con un selector SDL previo
a RGSS. La instalación se localiza en el directorio del propio NRO (`argv[0]`);
si hbmenu no entrega esa ruta, se usa `sdmc:/switch/FangameNX`. La biblioteca
vive en `games/` bajo esa instalación. Tras seleccionar, se cierra por completo
la ventana, el renderer, las fuentes y SDL; se cambia el directorio de trabajo,
se fija la carpeta de juego para los logs y se inicia el motor una sola vez.
La salida del juego termina el proceso y vuelve a Homebrew Menu. No se intenta
reinicializar Ruby, GL ni audio. El build normal `rgss-switch` mantiene el
contrato anterior. La corrección SDL de FNX-016 sigue en el árbol y en el
parche de reconstrucción.

## Instalación y manifestación

```text
switch/FangameNX/
  FangameNX.nro
  config/launcher.json
  logs/launcher.log                 (se crea al arrancar)
  games/<id>/
    fangamenx.json                 (opcional)
    mkxp.json, lib/, Data/ ...     (preparados para el motor)
```

`<id>` es el nombre de la carpeta: entre 1 y 80 caracteres ASCII
`[A-Za-z0-9_-]`. El escáner solo visita hijos inmediatos y rechaza enlaces
simbólicos en carpetas y archivos críticos. Los juegos se ordenan por ID. No
lee ni ejecuta Ruby durante el escaneo. El manifiesto opcional es JSON estricto
UTF-8 de hasta 32 KiB:

```json
{
  "id": "mi-juego",
  "name": "Mi juego",
  "description": "Descripción breve",
  "author": "Autor",
  "version": "1.0",
  "cover": "cover.png"
}
```

`id`, si aparece, debe coincidir con la carpeta. `cover` acepta solo un nombre
local `.png`, `.jpg` o `.jpeg`, sin rutas ni `..`; la portada se carga solo al
mostrar el juego. Si falta el campo `cover`, el escáner busca, en este orden,
`cover.png`, `cover.jpg` y `cover.jpeg` en la raíz del juego. La ruta explícita
del manifiesto tiene prioridad. Si no existe una imagen utilizable, se dibuja
una imagen geométrica original. Si
falta el manifiesto, se usa `Title=` de `Game.ini` y luego el ID. Metadatos
ilegibles o inválidos se muestran como instalación no ejecutable. La portada
ausente no bloquea. Este esquema no declara compatibilidad con XP o Essentials.

El preflight solo considera preparadas las carpetas que tienen `mkxp.json`,
`lib/nx-compat.rb`, `lib/socket.rb` y **o bien** `Data/Scripts.rxdata` con
`Graphics/` y `Audio/` **o bien** `datos.zip` con `Game.rgssad`. Es una
comprobación de estructura, no de contenido ni compatibilidad. El motor conserva
su propia revisión de arranque. Justo antes de lanzar se reescanea y se valida
otra vez el ID seleccionado. No se toca ningún archivo del juego al escanear.

`config/launcher.json` admite `{"schema":1,"compact":false}`. La lista
compacta es la única preferencia editable por ahora. Una configuración ausente
usa el valor por defecto; una malformada no se sobrescribe. Al guardar se
conservan las claves desconocidas y se escribe un temporal, `fflush`, `fsync`
y `rename`. El launcher escribe trazas breves de inicio, escaneo, selección y
salida en `logs/launcher.log`; los logs y guardados del motor siguen dentro de
la carpeta de cada juego. El código no migra partidas existentes.

Para añadir una instalación **ya preparada** a una biblioteca local se puede
usar `python packaging/install-launcher-game.py <carpeta-preparada>
<carpeta-switch/FangameNX/games>`. El destino debe ser nuevo. Durante la copia,
`packaging/auto_cover.py` conserva una portada explícita o busca un único PNG
válido de título/logo en `Graphics/Pictures` o `Graphics/Titles*`, tanto suelto
como dentro de `datos.zip`. Copia los bytes a `cover.png` y declara el campo en
`fangamenx.json`. Si no encuentra una elección única, conserva la portada
geométrica de reserva. Esta herramienta prepara la biblioteca en el PC: no
convierte una carpeta cruda de RPG Maker XP en una instalación ejecutable y no
escribe por sí sola en la SD ni en MTP.

## Interfaz y controles

Paleta azul noche, turquesa y violeta, icono original, tarjetas con foco
turquesa, detalles, ajustes, biblioteca vacía, pantalla de escaneo, avisos y portadas diferidas.
El renderer usa el tipo Liberation Sans ya empotrado en mkxp-z. Redibuja por
eventos, sin bucle continuo de creación de texturas cuando está inactivo.
El escaneo es síncrono: la pantalla informa de la operación, pero la entrada
queda suspendida mientras recorre las carpetas.

| Botón Switch | Evento SDL | Acción |
|---|---|---|
| Cruceta arriba/abajo | `SDL_CONTROLLER_BUTTON_DPAD_*` | Mover foco |
| A | `SDL_CONTROLLER_BUTTON_B` | Abrir detalles, iniciar o activar ajuste |
| B | `SDL_CONTROLLER_BUTTON_A` | Volver o salir |
| X | `SDL_CONTROLLER_BUTTON_Y` | Reescanear en biblioteca |
| + | `SDL_CONTROLLER_BUTTON_START` | Abrir ajustes |

Teclado de prueba: flechas, Enter, Esc, X y F1. Se ignoran repeticiones de
`SDL_KEYDOWN` y hay un intervalo mínimo de 140 ms entre eventos de navegación.
La incidencia de X en M012 sigue abierta; este mapeo no demuestra su corrección.

## Verificado en PC

- `python reconstruir-src.py --orden`: 40 parches aplicables, 39 archivos
  idénticos a la fuente reconstruida, cero desviaciones y cero huérfanos. El
  parche M013 depende del cierre SDL y se aplica después de él.
- `make -C mkxp-z-nx -j6 FNX_LAUNCHER=1 FNX_LIBGCC_PREFIX=<prefijo aislado>`
  desde MSYS2 de devkitPro: código 0; ELF y NRO generados. El log está en
  `local/m013/build-delivery-v2.log`. No se modificó el SDK global.
- La compilación de regresión sin `FNX_LAUNCHER` también terminó con código 0
  y produjo `rgss-switch.nro` (`local/m013/build-generic-regression.log`).
- `tests/run-launcher-host.cmd <paquete>` con Visual Studio C++: código 0;
  biblioteca vacía, dos instalaciones externas, JSON inválido, nombre largo,
  portada con ruta prohibida, ID discordante, configuración persistente y
  conservación de claves desconocidas. La prueba lee el paquete M013 real.
- `python docs/fangame-nx/auditar-armv8.py <elf> <nro>`: cero patrones conocidos
  incompatibles y 11 767 736 bytes de código ELF/NRO idénticos. `nm -u` no
  lista símbolos indefinidos. El manifiesto del paquete pasa sus 25 hashes.

## Pendiente y límites

No se ejecutó el launcher en Switch ni en emulador. Por tanto, el render SDL
previo al motor, la entrada física, la lectura desde microSD, el salto a Ruby,
el cierre y la estabilidad aún requieren prueba. No hay capturas fieles de la
interfaz; el icono sí se generó y revisó localmente. Tampoco se ha probado la
ejecución de un fangame real o el aislamiento de guardados para juegos que
usen rutas absolutas. El preflight de `datos.zip` no inspecciona su registro
interno: la validación definitiva permanece en el motor. FNX-017 y el pánico
Atmosphère de M012 siguen abiertos, sin causa raíz confirmada.

Siguiente paso: verificar en hardware la biblioteca y dos lanzamientos
separados del mismo NRO con los ejemplos propios, y conservar los logs antes
de ampliar la compatibilidad.
