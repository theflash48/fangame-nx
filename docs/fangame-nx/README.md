# FangameNX: documentación de trabajo

## Misión 001

- [`auditoria-001.md`](auditoria-001.md): inventario, arquitectura heredada, compatibilidad, licencias, riesgos y comprobaciones.
- [`ADR-004-lanzador.md`](ADR-004-lanzador.md): viabilidad multijuego, arquitectura propuesta, alternativas y primer prototipo.

## Misión 002

- [`entorno-fnx-003.md`](entorno-fnx-003.md): guía de entorno, dependencias, procedimiento y bloqueos de compilación.
- [`log-fnx-003.md`](log-fnx-003.md): comandos, versiones y resultados observados en esta misión.

## Misión 003

- [`diagnostico-cruby-fnx-003.md`](diagnostico-cruby-fnx-003.md): causa del enlace fallido, correcciones del build, comprobación del NRO y límites.
- [`log-m003-compilacion.txt`](log-m003-compilacion.txt): salidas completas de los intentos y de las compilaciones limpias, con la ruta de usuario sustituida.
- [`log-m003-verificacion.txt`](log-m003-verificacion.txt): herramientas, formatos, símbolos, contenido del NRO y hashes.

## Misión 004

- [`prueba-hardware-fnx-004.md`](prueba-hardware-fnx-004.md): revisión ARMv8-A, fixture sintético, paquete local y puerta de seguridad para la primera prueba en Switch.
- [`log-m004-auditoria-estatica.txt`](log-m004-auditoria-estatica.txt): comparación exacta del postproceso, instrucciones restantes, formato y comprobaciones del paquete.

## Misión 005

- [`compatibilidad-armv8-fnx-015.md`](compatibilidad-armv8-fnx-015.md): procedencia y alcanzabilidad de las 23 instrucciones SVE/SME, diagnóstico de `libgcc`, resultado y alternativas.
- [`log-m005-compilacion.txt`](log-m005-compilacion.txt): salida completa de la recompilación limpia del motor.
- [`log-m005-verificacion.txt`](log-m005-verificacion.txt): comandos, inventario de instrucciones, comprobaciones estáticas y hashes.

## Misión 006

- [`libgcc-aislada-fnx-015.md`](libgcc-aislada-fnx-015.md): procedencia, decisión, integración y resultados de la biblioteca Cortex-A57.
- [`log-libgcc-aislada-fnx-015.md`](log-libgcc-aislada-fnx-015.md): comandos, versiones, errores y verificaciones.
- [`patches/libgcc-cortex-a57.patch`](patches/libgcc-cortex-a57.patch): ajuste local reversible de siete fuentes de GCC 16.1.0.
- [`auditar-armv8.py`](auditar-armv8.py): detector de los opcodes conocidos de M004/M005 en secciones ejecutables y comparación del código ELF/NRO.
- [`log-m006-libgcc-build.txt`](log-m006-libgcc-build.txt), [`log-m006-cruby-configure.txt`](log-m006-cruby-configure.txt), [`log-m006-cruby-make.txt`](log-m006-cruby-make.txt), [`log-m006-cruby-summary.txt`](log-m006-cruby-summary.txt), [`log-m006-cruby-finalize.txt`](log-m006-cruby-finalize.txt) y [`log-m006-motor-build.txt`](log-m006-motor-build.txt): logs completos de los pasos finales, con rutas personales sustituidas.
- [`log-m006-libgcc-repro-configure.txt`](log-m006-libgcc-repro-configure.txt) y [`log-m006-libgcc-repro-make.txt`](log-m006-libgcc-repro-make.txt): segunda construcción de la biblioteca en otro directorio, con rutas personales sustituidas.
- [`log-m006-verificacion.txt`](log-m006-verificacion.txt): mapa resumido, auditoría de CPU con dos desensambladores, estructura, símbolos, fixture y hashes.

## Misión 007

- [`preparacion-primer-arranque-fnx-016.md`](preparacion-primer-arranque-fnx-016.md): paquete sintético local, inventario, verificaciones y guía reservada para instalación manual.
- [`log-m007-verificacion.txt`](log-m007-verificacion.txt): comandos, hashes y resultados estáticos del NRO de ensayo.
- [`fixtures/fnx016/icono.svg`](fixtures/fnx016/icono.svg): fuente vectorial original del icono neutro, sin recursos de juegos.

Este directorio reúne las decisiones y comprobaciones propias del fork. El repositorio original ya documenta un motor compartido, un perfil genérico y perfiles específicos. FangameNX parte de esa base; la compatibilidad real y el alcance de los perfiles deben verificarse antes de diseñar cambios.

## Documentos de referencia

- [`CONVENCIONES.md`](../../CONVENCIONES.md): contrato actual entre motor, empaquetador, shims y perfiles.
- [`README.md`](../../README.md): arquitectura, estado declarado y límites conocidos por el proyecto original.
- [`LICENCIAS.md`](../../LICENCIAS.md) y [`LICENSE`](../../LICENSE): licencias y atribuciones existentes.
- [`juegos/README.md`](../../juegos/README.md): formato y uso de los perfiles.
- [`TRIAJE.md`](../../TRIAJE.md): registros y diagnóstico.

## Primer trabajo: auditoría técnica

1. **Inventario reproducible:** anotar versiones, dependencias, herramientas de compilación y procedencia de los binarios o fuentes externos.
2. **Arranque genérico:** seguir desde la selección de carpeta hasta la carga de `mkxp.json`; comprobar rutas, errores y ausencia de nombres de juegos dentro del binario.
3. **Compatibilidad RGSS:** identificar qué cubren mkxp-z, Ruby y los shims; separar soporte general de ajustes ligados a un juego concreto.
4. **Empaquetado:** comprobar la composición de configuración, el perfil `generico`, la generación de NRO/NSP y los casos sin perfil.
5. **Datos y licencias:** rastrear qué entra en el repositorio y en cada entrega; verificar avisos, código fuente y licencias de dependencias.
6. **Pruebas:** definir una matriz mínima con casos sintéticos o material propio/autorizado, PC, emulador y hardware cuando estén disponibles. Registrar evidencia y límites por caso.

## Registro de resultados

Para cada hallazgo, anotar archivo y línea o comando, entorno, resultado observado, impacto y propuesta. Marcar las hipótesis como tales. No añadir capturas ni archivos de juegos de terceros a este directorio.

El historial heredado contiene imágenes y perfiles relacionados con juegos concretos, entre ellos `juegos/pokemon-anil/icono.jpg` y capturas en `docs/capturas/`. Antes de redistribuir una entrega de FangameNX, revisar su procedencia y autorización. Esta preparación no añade ni elimina esos archivos heredados.
