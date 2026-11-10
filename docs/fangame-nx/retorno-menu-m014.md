# M014 — retorno al menú tras cerrar el juego

Fecha: 10 de noviembre de 2026. Rama:
`feature/fangamenx-launcher-mvp`. Solicitud del propietario después de la
primera prueba física de M013: B cierra el fixture y debe volver al menú de
FangameNX; + debe cerrar la aplicación.

## Evidencia previa conservada

La lectura MTP de los logs, anterior al cambio, está en
`local/m014/preupdate-20261110-102547/` y su `manifest.json` contiene ocho
archivos con SHA-256. `launcher.log` registra la selección de `fnx009` y
`fnx017`; cada uno informa el directorio correcto. `fnx009-probe.log` registra
lectura, escritura, B y finalización normal; `fnx017-selector.log` registra
una ejecución completa de la escena A. Ambos `mkxp-boot.log` llegan a
`SDL_DestroyWindow OK` y `[exit] 08 before return main`, y los dos debug
terminan en `Shutting down.`. Esto demuestra esas dos salidas, no la estabilidad
de ciclos repetidos. La traza de `fnx009` **no contiene A ni X** en esta
prueba; no se da por resuelta la incidencia de entrada M012.

## Decisión

Se conserva el modelo de ADR-004: **un juego por ejecución del runtime**. Tras
una salida normal, después de `SDL_Quit`, el NRO llama a `envSetNextLoad` con
la ruta de `FangameNX.nro`. El cargador homebrew inicia una ejecución nueva del
mismo NRO, que muestra otra vez la biblioteca. Ruby, SDL, OpenAL, PhysFS, sus
hilos y estados globales no se reinicializan dentro del proceso anterior.

La [API oficial de libnx](https://switchbrew.github.io/libnx/env_8h.html)
define `envHasNextLoad` y `envSetNextLoad`. El
[código de libnx](https://github.com/switchbrew/libnx/blob/master/nx/source/runtime/env.c)
devuelve fallo si el cargador no aporta el búfer de siguiente NRO. El
[cargador de switchbrew](https://github.com/switchbrew/nx-hbloader/blob/master/source/main.c)
aporta esa ruta y dispone de un búfer de 512 bytes. Por eso se comprueba
`envHasNextLoad`, el límite de longitud y la existencia del NRO antes de
programar el retorno. Si no está disponible, falla la llamada, hubo un aviso
de error o el hilo RGSS no confirmó su cierre, el proceso sigue la salida
conocida a Homebrew Menu. No se encadena después de un fallo de arranque ni
se oculta la pantalla de error.

En el launcher, + sale a Homebrew Menu desde cualquier pantalla; B vuelve
atrás en detalles y ajustes, y en la biblioteca no cierra. Los ajustes pasan a
Y. X sigue reescaneando en la biblioteca. El teclado de desarrollo usa F10
para salir y F1 para ajustes. El runtime del juego conserva sus propias
acciones de botones: en el fixture `fnx009`, B termina el script Ruby.

## Verificación local

- `python reconstruir-src.py`: 40 parches aplicables, 39 archivos idénticos,
  cero desviaciones y huérfanos. `mkxpz-sdl-shutdown.patch` sigue antes de
  `mkxpz-launcher.patch`.
- Build `FNX_LAUNCHER=1` con libgcc Cortex-A57 aislada: código 0, NRO
  `0.2.4 b20`, SHA-256
  `bfa63f21d44637984866b19180553c81fc6d76d40e4a59cdd281db1c9e64c049`.
  ELF SHA-256 `b9ef9f85250605f97c13c8331f1c7d04ceab76528e6ffc61aae25610a1b35fb0`.
  Log: `local/m014/build.log`.
- `auditar-armv8.py`: cero patrones conocidos incompatibles; 11 768 248 bytes
  de código ELF/NRO idénticos. Build genérico sin `FNX_LAUNCHER`: código 0,
  log `local/m014/build-generic.log`.
- Prueba host C++ sobre el nuevo paquete: `launcher_host: OK`. El retorno
  `envSetNextLoad` y los botones SDL **no pueden ejecutarse en esa prueba**.
- Paquete local `local/m013/package-bfa63f21d4/`: 25 archivos, manifiesto
  SHA-256 `5af0bdb02aad717b25aec6de7957321f970be3af9aef177a4472831b6ca1c9eb`.

## Prueba física pendiente

La compilación y el contrato del cargador hacen viable la técnica, pero no
demuestran aún que el retorno funcione en esta Switch. Tras instalar el NRO
nuevo: probar + desde la biblioteca, abrir de nuevo, ejecutar `fnx009`, salir
con B y observar si reaparece la biblioteca sin pasar por Homebrew Menu. Luego
probar + para salir de verdad. Si funciona, repetir una vez con `fnx017` y
recoger los logs. Un fallo de X o un pánico se registra aparte: M012 sigue
abierto.
