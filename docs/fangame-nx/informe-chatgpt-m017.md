# Informe para ChatGPT · FangameNX M017

Fecha: 2026-11-10. Rama: `feature/fangamenx-launcher-mvp`. Punto de partida:
`6edf558`. Implementación y documentación de M017 en el commit que contiene
este informe (`git log -1`). No se hizo `push`. No se accedió a MTP, microSD
ni otras unidades extraíbles; no se modificó Pokémon Z instalado ni el NRO
b25 de la Switch.

## Resultado ejecutivo

M016 quedó consolidada como el **primer juego real cuyo funcionamiento general
confirmó el propietario en Switch**: Pokémon Z 2.18. M017 no repitió esa
instalación ni reinterpretó como verificadas funciones específicas que aún no
se han probado por separado. El staging y las evidencias privadas de M016 se
conservaron.

Hay ahora un **preparador local reutilizable** para una carpeta o ZIP de un
juego de RPG Maker XP, más un modo de diagnóstico sin staging. Produce
configuración, recursos empaquetados, metadatos, portada cuando hay una
candidata clara y un manifiesto de hashes. No promete compatibilidad por
estructura y detiene los casos que necesitan una adaptación específica.

## Cambios y diferencias respecto a M016

- `packaging/prepare-launcher-game.py` toma una fuente local y consulta los
  perfiles, la base `mkxp`, el catálogo de shims y la stdlib existentes. El
  diagnóstico identifica `Game.ini`, scripts, Data/Graphics/Audio, familia
  Ruby, perfil, dependencias, problemas y advertencias. `--family`, `--profile`,
  `--id`, `--name`, `--author` y `--version` permiten resolver datos que no
  deben adivinarse.
- El staging privado empaqueta Data/Graphics/Audio/Fonts/Movies en
  `datos.zip` con método `STORED`, deja `Data/Scripts/` suelta cuando existe,
  compone `mkxp.json`, copia `lib/`, crea o conserva `Game.rgssad` y genera
  `fangamenx.json`. La portada usa `packaging/auto_cover.py`; no hay rutas ni
  excepciones de Pokémon Z en la nueva herramienta.
- Se rechazan entradas ZIP peligrosas, duplicados sin distinción de
  mayúsculas, conflictos archivo/directorio, enlaces, cifrado, rutas de
  escape y fuentes que exceden los límites. La salida nueva se publica tras
  construirla en un parcial privado. El manifiesto vecino enumera rutas,
  tamaños y SHA-256; repetir con la misma fuente y opciones verifica todos
  los archivos antes de reutilizar. `packaging/install-launcher-game.py`
  verifica por hash la copia a una biblioteca local antes de publicarla.
- La selección de portada comprueba la estructura y CRC del PNG, además de
  límites de tamaño y dimensiones. Un empate conserva el placeholder. La UI
  del **NRO local nuevo** comprueba cabeceras PNG/JPEG y dimensiones antes de
  SDL_image. Sigue cargando solo la portada del juego mostrado.

La herramienta puede analizar carpetas o ZIP con una sola raíz de juego que
contengan `Game.ini`, Data, Graphics, Audio y scripts accesibles. Para crear
staging exige una familia Ruby determinada y las dependencias locales. Un
archivo RGSS con nombre no estándar, `execName` distinto de `Game`, scripts
no visibles, bibliotecas nativas o necesidades de perfil no cubiertas
requieren revisión específica. `datos.zip` se limita a menos de 4 GiB sin
ZIP64, pendiente de validar con el runtime. El perfil histórico de un juego
no demuestra compatibilidad de una versión nueva.

## Build, paquete y evidencias

Entorno: Windows, PowerShell 7.6.9, Python 3.13.1, devkitA64 GCC 16.1.0,
Visual Studio C++ para el test host y `libgcc` Cortex-A57 aislada con SHA-256
`b7bca10ee17cae2295533e793c513ab144095e37b2e21142817c5c9811c837dc`.

- Prueba sintética: `python tests/test_prepare_launcher_game.py` terminó en
  `prepare_launcher_game: OK`. Cubre carpeta y ZIP, diagnóstico sin paquete,
  `datos.zip`, configuración, portada, manifiesto, reutilización, cambio de
  fuente, estructura incompleta, entrada ZIP insegura, colisión de nombres,
  PNG inválido y detección
  de un perfil cuyos indicadores son directorios. `python -m py_compile` pasó.
- Regresión host: `tests/run-launcher-host.cmd local/m013/package-bbf9d4fee9`
  y `tests/run-launcher-host.cmd local/m013/package-fe34daaf94` terminaron en
  `launcher_host: OK` con el paquete b25 y con el paquete nuevo. Cubre
  descubrimiento, metadatos, portada, configuración persistente y errores del
  escáner con Pulso y Órbita doble. Navegación SDL, modo applet, lanzamiento
  y retorno se revisaron en el código existente; M017 no añade una prueba
  física de esos recorridos.
- Compilación **incremental** con
  `make -C mkxp-z-nx -j6 FNX_LAUNCHER=1 FNX_LIBGCC_PREFIX=<repo>/src/build-logs/fnx016-isolated-prefix FNX_LINK_MAP=<repo>/local/m017/FangameNX-b27.map`:
  NRO `0.2.4 b27`, 17 820 873 bytes, SHA-256
  `fe34daaf94e7522a19fb04f46f2da8fa3276a8da1a5cd8ae0d00e9d7b017a3bd`.
  ELF SHA-256
  `73ef9ce4e376b5019f48b3abd6ae86b5d4feb2caddac6228cb255b99bd09d0cf`.
  Logs y mapa: `local/m017/build-b27.log` y `local/m017/FangameNX-b27.map`.
- Auditoría `docs/fangame-nx/auditar-armv8.py`: cero patrones conocidos
  incompatibles, 11 776 120 bytes ejecutables ELF/NRO coincidentes y cero
  diferencias. `objdump -d -M no-aliases` no encontró `<unknown>` ni los
  patrones ARMv9 buscados; `nm -u` dio cero símbolos indefinidos. El mapa
  referencia la `libgcc` aislada en `src/build-logs/fnx016-isolated-prefix/`.
- Paquete de desarrollo **solo local**:
  `local/m013/package-fe34daaf94/`, 25 archivos, SHA-256 del manifiesto
  `e733f9ce824e8038f7f647004436be72ad1a20e61ca2e80eb83c71d0f550e547`;
  los 25 hashes se verificaron. El b25 conocido permanece en
  `local/m013/package-bbf9d4fee9/` y en `local/m017/FangameNX-b25.nro`,
  SHA-256 `bbf9d4fee998703903f10694c04b91a1e5dd01c0769e09cba79c119bd3b5e299`.

No se incorporaron recursos, scripts ni distribuciones de fangames a Git.
Las pruebas del preparador usan datos sintéticos creados en `local/`.

## Alcance y siguiente tarea

El NRO b27 **no se instaló**; la detección de `cover.png` sin manifiesto y
la comprobación previa de dimensiones siguen sin prueba en hardware. No se
afirma corregida la X intermitente ni el pánico tras tacto. Una portada
generada por el nuevo preparador sí declara el campo que entiende el b25.

La matriz [`matriz-pokemon-z-m017.md`](matriz-pokemon-z-m017.md) deja pendientes
portada visible, controles, audio, mapas, guardado/carga, retorno y estabilidad
de Pokémon Z hasta recibir resultados concretos del propietario. Próximo
objetivo recomendado: diagnosticar una **segunda copia legítima** de un juego
distinto con el preparador, revisar sus advertencias y preparar staging local;
solo después planificar una prueba controlada de ese paquete y del NRO nuevo
en hardware. No sobrescribir partidas ni sustituir el b25 durante esta misión.

Contrato y comandos detallados: [`preparador-m017.md`](preparador-m017.md).
