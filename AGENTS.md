# Instrucciones para Codex — FangameNX

## Alcance

- Trabaja únicamente dentro de este repositorio. `origin` es el fork `theflash48/fangame-nx` y `upstream` es `randygil/rgss-switch`; no publiques cambios en `upstream`.
- FangameNX busca una aplicación homebrew genérica y extensible para ejecutar juegos de RPG Maker XP y Pokémon Essentials compatibles con el runtime existente. Conserva la separación entre motor, configuración, empaquetado y perfiles.
- Antes de modificar un contrato entre piezas, lee `CONVENCIONES.md`, `README.md` y la documentación relevante en `docs/`. Registra las decisiones nuevas en `docs/fangame-nx/`.

## Límites

- No incluyas recursos, scripts extraídos ni distribuciones de fangames; tampoco claves de consola ni datos personales. El material de juego debe permanecer fuera del repositorio.
- No elimines ni sobrescribas archivos existentes sin autorización. Conserva el historial, las atribuciones y las cabeceras de licencia.
- No reescribas el motor ni añadas funciones nuevas sin una tarea explícita. Prefiere cambios pequeños y comprobables.
- La `.gitignore` es una lista blanca. Al añadir archivos, permite solo rutas concretas y comprueba `git status --short` antes de confirmar cambios.

## Autonomía de desarrollo y MTP

- El propietario autoriza de forma continua el uso de MTP en su Nintendo Switch para leer, instalar y actualizar exclusivamente FangameNX, sus juegos preparados y sus registros. No pidas confirmación por cada operación; esta autorización sigue vigente hasta que el propietario la revoque.
- Identifica una vez por sesión el dispositivo portátil Nintendo Switch correcto antes de escribir. No confundas su acceso MTP con una microSD conectada a un lector ni toques otra Switch.
- Antes de sustituir ejecutables o configuraciones, conserva copias locales recuperables, verifica la versión activa y comprueba por lectura de vuelta los archivos nuevos críticos. Conserva partidas, instalaciones ajenas y versiones anteriores.
- No modifiques sysNAND, Atmosphère, bootloader, datos de otras aplicaciones ni archivos fuera de la instalación FangameNX. La autorización MTP no permite destruir guardados.
- Prioriza compatibilidad y funciones reales sobre micropruebas o documentación redundante. Si un paso se bloquea, continúa con otra tarea útil autorizada y registra el bloqueo concreto.

## Verificación

- Distingue lo observado, lo inferido y lo pendiente. No afirmes compatibilidad con un juego, emulador o consola sin pruebas reproducibles.
- Documenta comandos, entorno y resultados de las pruebas pertinentes. Si falta el SDK o el hardware, indícalo como límite de verificación.
- Revisa `LICENSE` y `LICENCIAS.md` antes de incorporar código o dependencias. Mantén las obligaciones de GPLv2 o posterior y las licencias de cada componente.
