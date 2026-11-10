# Informe para ChatGPT · M016 · Pokémon Z 2.18

Fecha: 10 de noviembre de 2026. Rama local: `feature/fangamenx-launcher-mvp`.
HEAD al iniciar: `ba10711`. No se modificó el motor ni se construyó o instaló
otro NRO. No se hizo `push` ni publicación.

**Estado posterior comunicado por el propietario:** Pokémon Z 2.18 arranca y
funciona en su Nintendo Switch con esta instalación. El propietario lo confirmó
primero con «funciona» y después con «va de puta madre». Es una validación real
del uso general, aunque no sustituye pruebas registradas por separado de
guardado/carga, audio, controles, salida o sesiones prolongadas. Los apartados
de la misión original conservan el estado observado antes de esa prueba.

## Resultado observado

Se preparó Pokémon Z 2.18 en `local/m016/stage-pokemon-z-2.18/`, excluido de
Git, y se copió por MTP a `Nintendo Switch → SD Card → switch → FangameNX →
games → pokemon-z`. El inventario final tiene **114 de 114 archivos** y las
lecturas de vuelta coinciden con el staging final en SHA-256. La instalación
de archivos queda verificada; **no se ha iniciado el juego en la Switch y no
se afirma compatibilidad de 2.18**.

Antes de la primera escritura se identificó un único dispositivo portátil MTP
`Nintendo Switch`, se accedió a `SD Card`, se observaron `fnx009` y `fnx017`
en la biblioteca y se recuperó el NRO activo. Su SHA-256 fue
`bbf9d4fee998703903f10694c04b91a1e5dd01c0769e09cba79c119bd3b5e299`,
igual al exigido. La carpeta `pokemon-z` no existía. El código b25 confirma
`switch/FangameNX/games/<id>/` como ruta de descubrimiento. MTP informó
7,42 GB libres antes de copiar; el staging mide 889 208 850 bytes. No se
tocaron `FangameNX.nro`, los ejemplos, `config/launcher.json`, otros juegos o
guardados.

La primera orden `Shell.Application.CopyHere` creó la carpeta y copió solo
parte de los archivos antes de terminar el proceso de PowerShell. Su
`datos.zip` se leyó de vuelta: tamaño 888 144 816 bytes y SHA-256 idéntico al
local. Se completaron los archivos restantes sin volver a copiar el
contenedor. Dos elementos se resistieron inicialmente; el fichero de créditos
se copió con nombre ASCII `Creditos.txt`, conservando su contenido. El archivo
`benchmark/version.rb` no apareció tras la copia individual, la copia de la
carpeta `benchmark` y un pegado en el Explorador. Se comprobó mediante WPD
que `lib/benchmark`, creada en esta misión, contenía exactamente un archivo
propio (`benchmark.gemspec`); se eliminó **solo esa subcarpeta** y se recreó.
La copia secuencial de sus dos archivos con lectura MTP inmediata verificó
ambos hashes. El inventario final remoto enumera 114 archivos, sin diferencias
de nombres con el staging. No se borró ningún contenido preexistente.

## Fuente, inventario y preparación privada

Fuente autorizada: `D:\Descargas\POKEMON Z V2.18.zip`, 826 877 555 bytes,
SHA-256 `f642df4dcdaf5e4e9746a82297dd8d9bd32ffc5f48f1ea5756a16d8f0f55f133`.
`zipfile.testzip()` pasó las 19 118 entradas: un solo directorio raíz, sin
cifrado, duplicados, colisiones sin distinción de mayúsculas, rutas de escape
ni enlaces simbólicos. Total descomprimido: 915 879 001 bytes. No se ejecutó
ningún EXE. El original descargado permanece intacto.

| Componente del ZIP | Observado |
|---|---|
| `Game.ini` | `Title=Pokemon Z`, `Scripts=Data\\Scripts.rxdata`, `Library=RGSS104E.dll` |
| `Data/` | 558 entradas, 50 061 306 bytes; `Scripts.rxdata` 1 017 941 bytes y mapas `.rxdata` |
| `Graphics/` | 15 828 entradas, 287 820 001 bytes |
| `Audio/` | 2 674 entradas, 559 226 353 bytes |
| `Fonts/` | 9 entradas, 222 668 bytes |
| Raíz | `preload.rb`, `configuration.json`, textos; también EXE, DLL, PDF y LNK de Windows |
| Guardados | No hay `Game.rxdata` ni partidas en el ZIP; coincidencias de “save” son recursos gráficos y de audio |

El staging privado contiene 19 022 recursos dentro de `datos.zip` **almacenado
sin compresión**, `Game.rgssad` de ocho bytes, `Game.ini`, `preload.rb`,
`configuration.json`, los textos de la raíz, `mkxp.json`, `fangamenx.json` y
105 ficheros en `lib/`. Se excluyeron `PBS/` (material de desarrollo),
`Data/Game.exe` duplicado, EXE, DLL, PDF y LNK de la raíz. El manifiesto
privado final está en `local/m016/stage-manifest-final.sha256.txt` (SHA-256
`188e3e5858e2a7fefc083fa2559a002d013dcd29eb10864230b868128e8d73aa`).
El contenedor tiene 19 022 entradas, 885 608 074 bytes de contenido, ZIP
válido con todas las entradas de método 0, y SHA-256
`0608df2428808edce149068f043bf2d6f88eeabafabac8ba1d04269adcd07a52`.
No se incorporaron archivos, recursos ni scripts de Pokémon Z a Git.

El `mkxp.json` compuesto sigue las capas de `CONVENCIONES.md`: base de Switch,
claves permitidas del archivo del juego, perfil histórico vacío y preloads
`nx-compat.rb`, `nx-teclado.rb`, `nx-rutas.rb`, `nx-ruby18.rb`, `preload.rb`
en ese orden. El juego declara `subImageFix=false` y `enableBlitting=true`;
se conservaron según la lista blanca del empaquetador, sin afirmar que sean
óptimos en Tegra. `rubyLoadpath=["lib"]`. El manifiesto `fangamenx.json`
declara ID `pokemon-z`, nombre Pokémon Z, autor EricLostie y versión 2.18.
Los guardados que cree el juego deben permanecer en su carpeta propia; no se
trasladó ninguna partida previa ni se mezclaron partidas de prueba.

## Comparación con 2.13 y compatibilidad pendiente

El perfil `juegos/pokemon-z/perfil.json` y `docs/pruebas/pokemon-z.md`
registran la versión 2.13 con `motor=ruby18`: 235 scripts, título, introducción,
mapa 66, menú y guardado `Game.rxdata` en Ryujinx con el empaquetado antiguo.
La 2.18 contiene **255 secciones**, 20 más, y usa `RGSS104E.dll` en
`Game.ini`. Esta diferencia basta para impedir trasladar el estado `jugable`
del perfil 2.13 a 2.18 o a la biblioteca b25 en hardware. Se conservó el
perfil histórico sin editarlo.

La 2.18 conserva llamadas `Win32API`, entrada `Input` redefinida, el lector
Ogg, `StringInput`, `LargePlane` y la lógica de guardado de la familia Ruby
1.8. El shim existente cubre los casos documentados en 2.13; no hay prueba
de que cubra todas las rutas nuevas. La búsqueda de guardado muestra
`RTP.getSaveFileName` y nombres `Game*.rxdata`. La carga real, gráficos,
audio, mapas, memoria, mando, tacto, guardado/carga y salida al launcher
siguen pendientes de ejecución física. La salida con B de Pulso y Órbita no
demuestra que este juego tenga la misma salida.

No se identificó un defecto genérico del runtime que pudiera corregirse de
forma comprobable sin ejecutar 2.18. Se dejaron abiertas las incidencias de
X intermitente y pánico tras tacto. El NRO b25 permanece activo y sin cambios.

## Verificaciones locales reproducibles

Entorno: Windows, PowerShell 7.6.9, Python 3.13.1, Ruby 3.1.4 del árbol y
Visual Studio C++ para el test host. Los guiones de preparación y las copias
de lectura residen en `local/m016/` y están ignorados por Git.

| Comando o comprobación | Resultado |
|---|---|
| `Get-FileHash -Algorithm SHA256 D:\Descargas\POKEMON Z V2.18.zip` | Hash fuente anterior |
| `zipfile.testzip()` más controles de rutas, cifrado y colisiones | 19 118 entradas válidas |
| `python local/m016/prepare.py` | 114 archivos de staging; 19 022 recursos empaquetados |
| `zipfile.testzip()` y método de cada entrada de `datos.zip` | Pasa; 19 022 entradas almacenadas |
| `ruby local/m016/check-syntax.rb` | 255 secciones, 3 transformadas por `NXFuente`, 0 fallos de compilación |
| `tests/run-launcher-host.cmd local/m013/package-bbf9d4fee9` | `launcher_host: OK` con Pulso y Órbita |
| `local/m016/check-stage.cmd` | Escáner real: `pokemon-z`, versión 2.18, `ready=1` |
| SHA-256 de archivos leídos de vuelta por MTP | 113 coincidencias en lectura completa; los 2 de `lib/benchmark` se leyeron de nuevo tras recrear esa subcarpeta: 114/114 hashes finales verificados |

El preflight `ready=1` solo valida estructura. No ejecuta Ruby ni el juego.
No se compilaron fuentes de Switch ni se ejecutó el juego en Ryujinx o en
hardware durante M016. El test sintáctico no prueba comportamiento. La SD
real muestra nueve archivos en la raíz de `pokemon-z`, 105 en `lib/` y 20
subdirectorios, como el manifiesto.

## Recuperación y siguiente prueba

1. La copia está lista para la primera prueba. Cerrar USB File Transfer y
   abrir FangameNX con memoria de aplicación.
2. Si se sospecha corrupción de la copia, volver a MTP, verificar el NRO b25
   y comparar la carpeta `pokemon-z` con el manifiesto privado de 114 hashes.
   No fusionar otra versión ni borrar guardados.
3. En el primer arranque, medir por separado: aparición en biblioteca, ficha,
   lanzamiento, primera
   pantalla, mando, gráficos/audio, mapa, guardado y carga de **partida nueva**
   y salida. No usar partidas previas para esta prueba.
4. Ante un fallo, no repetir a ciegas. Volver a USB File Transfer y recuperar
   `mkxp-boot.log`, `mkxp-debug.log`, `mkxp-ruby.log`, `errorlog.txt` de
   `games/pokemon-z/` si existen, más `logs/launcher.log`, antes de cambiar
   configuración o motor. Conservar el ZIP original, staging y manifiesto
   para reconstruir. No modificar el NRO activo en esta misión.

Propuesta futura **FNX-022**: asociar cada juego a una fuente autorizada,
comprobar versiones y verificar descargas, con actualización que conserve
partidas; también valorar actualización de FangameNX al inicio con copia de
seguridad, recuperación y reinicio seguro. No se implementó ni activó.

## Anexo posterior: portada del propio juego

Después del cierre de M016, el propietario comunicó que el juego «funciona»
en la Switch. No especificó hitos de mapa, audio, controles o guardado; esa
comunicación no verifica por separado esas funciones. Al volver a inspeccionar
la carpeta por MTP había cuatro archivos adicionales generados por la
ejecución: `mkxp-boot.log`, `mkxp-debug.log`, `mkxp-ruby.log` y `LastSave.dat`.
Se conservaron intactos. La presencia de `LastSave.dat` por sí sola no
demuestra una partida guardada y cargada correctamente.

Para dar portada a la ficha, `local/m016/add-cover.py` extrae directamente
`Graphics/Pictures/titulo.png` del `datos.zip` preparado, valida su cabecera
PNG y dimensiones (512×384, bajo el límite de 2048×2048 del launcher) y
copia sus bytes sin transformarlos a `cover.png` en la raíz de la instalación.
El script añade `"cover": "cover.png"` a `fangamenx.json` y conserva el
manifiesto anterior en local. El recurso del juego no entra en Git. Esta
selección es específica de Pokémon Z 2.18: otros juegos pueden componer la
pantalla de título a partir de varias capas y requerir otra elección.

Antes de escribir, se recuperó nuevamente el NRO activo y su SHA-256 siguió
siendo `bbf9d4fee998703903f10694c04b91a1e5dd01c0769e09cba79c119bd3b5e299`.
Se verificó por lectura MTP el manifiesto anterior; después se copió
`cover.png` y se verificó su SHA-256
`d9dcf8c08ad8d19014cc49dd6f633fb8d3ac79c168f06299cb862f9f07d20459`,
idéntico al recurso dentro de `datos.zip`. Solo entonces se sustituyó el
`fangamenx.json` propio de esta instalación; la lectura MTP del nuevo
manifiesto coincide con SHA-256
`4bf41063ebaed053b81b3ecd1a26ea1013be7f06269f078a4afc0a902b02f33f`.
El escáner host del launcher devuelve `ready=1` y una ruta local de portada
válida. El manifiesto privado actualizado está en
`local/m016/stage-manifest-cover.sha256.txt` (SHA-256
`be2102d02501237dd1c8e935089ed1ce7c6c9033619a0733387592b263524f23`).
La enumeración MTP posterior encontró los 115 archivos de ese manifiesto,
más los cuatro archivos de ejecución anteriores, sin nombres ausentes ni
inesperados. La visualización de la portada en la Switch todavía requiere
reabrir FangameNX o reescanear su biblioteca.

El lanzador no descubre `cover.png` solo por su nombre. Lee el campo `cover`
de `fangamenx.json`, valida que sea un nombre local PNG/JPEG, comprueba que el
archivo exista y lo carga al mostrar la ficha; si no puede cargarlo, usa la
imagen geométrica de reserva. Esta instalación ya declara
`"cover": "cover.png"` y tiene ese archivo en la raíz de `pokemon-z`. La
prueba host confirma que el escáner asocia la ruta, pero el propietario aún
no ha confirmado expresamente que la imagen se vea en la interfaz de Switch.
