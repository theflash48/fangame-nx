# FangameNX: documentación de trabajo

## Misión 001

- [`auditoria-001.md`](auditoria-001.md): inventario, arquitectura heredada, compatibilidad, licencias, riesgos y comprobaciones.
- [`ADR-004-lanzador.md`](ADR-004-lanzador.md): viabilidad multijuego, arquitectura propuesta, alternativas y primer prototipo.

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
