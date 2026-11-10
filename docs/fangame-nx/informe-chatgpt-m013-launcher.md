# Informe M013 para el propietario

Fecha: 10 de noviembre de 2026. Rama local:
`feature/fangamenx-launcher-mvp`. Commit de implementación: `277be0d`.
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
| Parcial | Validación de contenido del juego, diagnóstico en pantalla de fallos después de iniciar el motor |
| Pendiente | Capturas fieles, emulador, prueba Switch, pruebas de compatibilidad de fangames y aislamiento de guardados de terceros |
| Bloqueada por evidencia | Resolución de FNX-017, fallo intermitente X y pánico Atmosphère |

## Interfaz y arquitectura

Tema azul noche con foco turquesa, icono original F/X, tarjeta y portada
geométrica por defecto, ficha de detalles, ajustes, lista vacía y mensajes.
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

- NRO: `mkxp-z-nx/FangameNX.nro`, 17 808 585 bytes, SHA-256
  `21a987d7d55248ea050ebeea60b863567aa62bfc4413e181ebe0d6c6af852af3`.
- ELF: `mkxp-z-nx/FangameNX.elf`, 90 210 024 bytes, SHA-256
  `1603edda489827332f98bd4397543207115892f882e7219f95e81133d9650576`.
- Build `0.2.4 b17` con devkitA64/libnx/SDL2, Ruby 3.1.7, mkxp-z 2.4.2 y
  libgcc Cortex-A57 aislada. `local/m013/build-delivery.log` termina con NRO.
- Auditoría: cero patrones ARMv9 conocidos, ELF/NRO idénticos en 11 767 608
  bytes ejecutables y cero símbolos indefinidos. No sustituye ejecución real.
- Reconstrucción: 40 parches aplicables, cero desviaciones. Incluye la
  corrección `mkxpz-sdl-shutdown.patch` de FNX-016.
- Prueba host C++: `launcher_host: OK`, incluidos ambos juegos del paquete.
- Paquete local: `local/m013/package-21a987d7d5/`, 25 archivos, SHA-256 del
  manifiesto `b5ece2c3431e9f946231314fbb6f187aa4745288357fb4b95d5f9414e1c93402`.
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
