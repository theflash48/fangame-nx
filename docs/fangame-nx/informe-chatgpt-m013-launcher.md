# Informe M013 para el propietario

Fecha: 10 de noviembre de 2026. Rama local:
`feature/fangamenx-launcher-mvp`. Commits de implementación: `277be0d`,
`aa4c8f2`; primer commit de documentación: `1b4f3bc`.
No se hizo `push`, release, instalación, MTP ni acceso a ninguna microSD.

## Resultado general

Existe un primer NRO FangameNX con biblioteca externa, interfaz de mando,
detalles, ajustes, reescaneo, preferencia persistente y traspaso de la carpeta
seleccionada al runtime existente. El mismo binario puede apuntar a distintos
juegos preparados en `games/`; se inicia un solo juego por proceso. Esto está
**compilado e integrado en código**, pero aún no ejecutado en Switch.

| Estado | Funcionalidad |
|---|---|
| Implementada y probada en PC | Escaneo, ordenación, metadatos, rechazos de rutas, configuración, paquete local, reconstrucción de parches y compilación |
| Implementada sin prueba física | UI SDL, mando, portadas diferidas, selección y traspaso al motor, salida limpia del selector |
| Parcial | Validación de contenido del juego, escaneo síncrono, diagnóstico en pantalla de fallos después de iniciar el motor |
| Pendiente | Capturas fieles, emulador, prueba Switch, pruebas de compatibilidad de fangames y aislamiento de guardados de terceros |
| Bloqueada por evidencia | Resolución de FNX-017, fallo intermitente X y pánico Atmosphère |

## Interfaz y arquitectura

Tema azul noche con foco turquesa, icono original F/X, tarjeta y portada
geométrica por defecto, ficha de detalles, ajustes, escaneando, lista vacía y mensajes.
Admite `cover.png`/JPEG opcional por juego. No se generaron capturas: no había
una ejecución de la UI en PC que reutilizara su renderer SDL de Switch.

El escáner solo visita hijos inmediatos de `switch/FangameNX/games/`, ignora
enlaces y nombres con separadores, y nunca ejecuta scripts. `fangamenx.json`
es opcional; `Game.ini` da un título de reserva. El preflight exige piezas
mínimas del runtime. Al confirmar se revalida la carpeta, se destruye SDL del
selector, se hace `chdir` y `nx_carpeta_juego_fija`, y continúa el `main` ya
existente. El juego sale a Homebrew Menu. El contrato y esquema están en
[`desarrollo-launcher-m013.md`](desarrollo-launcher-m013.md).

## Compilación y evidencias

- NRO: `mkxp-z-nx/FangameNX.nro`, 17 812 681 bytes, SHA-256
  `b4b93af91e3bf6ce49fa6c111e55eb7d2ae49becb6eba77f0beaced56276b6a6`.
- ELF: `mkxp-z-nx/FangameNX.elf`, 90 221 960 bytes, SHA-256
  `00a1d61470466b32a5181df60c52c6f9e75bdd6c74ceaba319a46c5efd2580e6`.
- Build `0.2.4 b19` con devkitA64/libnx/SDL2, Ruby 3.1.7, mkxp-z 2.4.2 y
  libgcc Cortex-A57 aislada. `local/m013/build-delivery-v2.log` termina con NRO.
- El build genérico de regresión, sin `FNX_LAUNCHER`, también terminó con
  código 0 (`local/m013/build-generic-regression.log`).
- Auditoría: cero patrones ARMv9 conocidos, ELF/NRO idénticos en 11 767 736
  bytes ejecutables y cero símbolos indefinidos. No sustituye ejecución real.
- Reconstrucción: 40 parches aplicables, cero desviaciones. Incluye la
  corrección `mkxpz-sdl-shutdown.patch` de FNX-016.
- Prueba host C++: `launcher_host: OK`, incluidos ambos juegos del paquete.
- Paquete local: `local/m013/package-b4b93af91e/`, 25 archivos, SHA-256 del
  manifiesto `8eb80569390e8fb5dafb1e0ca572cf665bf8d5f4b94df10dc90e2cc325fd027f`.
  Incluye dos fixtures propios (`fnx009`, `fnx017`), configuración y licencias.
  No contiene fangames ajenos ni claves. No se copió a dispositivo alguno.

## Estabilidad

M012 sigue abierto. En cinco ciclos anteriores, X faltó en Ruby en el ciclo 3
y el ciclo 4 terminó en pánico Atmosphère; el pico heap comunicado fue 199 MB
en cada intento. El reporte AFE2 de `fatal` no determina la causa raíz. La
UI nueva tiene su propio mapeo SDL de botones, pero no hay prueba de que
resuelva ninguno de estos dos problemas. El cierre SDL corregido se conserva,
sin prometer que sea seguro tras todos los ciclos del launcher.

## Próxima prueba del propietario

1. Revisar el paquete local y su manifiesto. Copiarlo solo a una microSD
   propia cuando decida realizar la prueba; el paquete no se ha instalado.
2. Abrir el NRO desde Homebrew Menu con memoria de aplicación. Confirmar
   biblioteca de dos juegos, foco, detalles, ajustes, reescaneo y salida con B.
3. Lanzar `fnx009`, probar A/X/B y verificar retorno al Homebrew Menu.
4. Abrir el mismo NRO de nuevo y lanzar `fnx017`; verificar la otra carpeta
   y su log. Conservar `logs/launcher.log`, `mkxp-boot.log`, `mkxp-debug.log` y
   los logs de fixtures sin modificar los datos originales.
5. Comunicar cualquier bloqueo, fallo de X o pánico con el paso exacto y
   los logs. No inferir estabilidad de un solo ciclo.

## Próxima fase

Con una prueba física reproducible: corregir los fallos observados, obtener
capturas reales, probar portadas, mejorar los mensajes de arranque y ampliar
el preflight sin confundir estructura con compatibilidad. La ejecución de
fangames completos y el aislamiento de guardados requieren casos autorizados
y evidencia separada.
