# FangameNX M024 · compatibilidad y adaptación de juegos

Fecha: 2026-11-11. Esta auditoría parte del repositorio en b42 (`e26477d`), no de la base b39 citada en el encargo recibido después. El remapeo no forma parte de este bloque; la incidencia física de b40 y el fallo de Atmosphère permanecen documentados en `informe-chatgpt-m024.md`.

## Contratos observados

El launcher lee carpetas bajo `switch/FangameNX/games/<id>`, valida `fangamenx.json` y exige `mkxp.json`, shims mínimos y datos preparados. No contiene nombres de Z u Ópalo en el escáner. El preparador M017 toma una carpeta o ZIP, escoge un perfil por señales, copia `runtime/lib`, stdlib y shims, conserva scripts sueltos, crea `datos.zip` sin compresión y un manifiesto de hashes. `src/mkxp-z` no está versionado; el motor se reconstruye mediante `patches/ORDEN.txt`. La configuración base fija RGSS 1 y CRuby 3.1.7. `ruby18` y `rgss1` dependen de `NXFuente` y shims; no equivalen a ejecutar Ruby 1.8 nativo.

| Capacidad | Verificado | Límite o incertidumbre |
|---|---|---|
| Biblioteca sin juegos, con uno, dos y 80 entradas sintéticas | `launcher_host` inspecciona las cuatro cardinalidades; Z/Ópalo probados físicamente en b39 | El host comprueba escaneo, no render SDL ni estabilidad del catálogo grande en Switch |
| Portadas | Selección automática y reserva en tests; Z/Ópalo con portadas en consola | PNG/JPEG corruptos pasan por reserva en la UI; la ruta de render no se ejecuta en el host |
| Favoritos y orden | Persistencia host; uso físico comunicado en Z/Ópalo | 80 favoritos no probados físicamente |
| RPG Maker XP / RGSS 1, `Scripts.rxdata` | Cabecera Ruby Marshal, `Game.ini`, perfil y recursos diagnosticados; Z/Ópalo ejecutados | La cabecera no valida todo el Marshal ni garantiza ejecución Ruby |
| Scripts Ruby sueltos | `Data/Scripts/*.rb` reconocidos; `Scripts/` de Reborn se conserva con el cargador RXDATA | La carga depende del bootstrap de cada juego; no se ejecuta en diagnóstico |
| Ruby 3.x / Essentials modernos | DLL Ruby 3 detectada; perfil Reborn y staging 19.5 preparados | Reborn 19.5 aún no se ha iniciado físicamente; perfil previo se probó con otra versión |
| Ruby 1.8 / Essentials antiguos | Z/Ópalo y perfiles históricos; `NXFuente` transforma al ejecutar | Cada uso de sintaxis/API antigua puede requerir shim; no existe garantía universal |
| RGSS2/3 y `.rvdata` | Diagnóstico rechaza `Library=RGSS20*/30*`, `rgssVersion != 1` o script no `.rxdata` | No se ofrece preparación para VX/Ace |
| `datos.zip` y `Game.rgssad` | ZIP `STORED` y llave de ocho bytes probados; fuente ZIP con CRC verificado | Archivo RGSS cifrado real se bloquea; ZIP64 y recursos >4 GiB no verificados |
| Pantallas 512×384 y 640×480 | Configuración 512×384 conservada en fixture; motor escala por aspecto | Otras resoluciones requieren prueba de render, no solo metadatos |
| Fuentes, gráficos, audio | Carpetas inspeccionadas y empaquetadas; Z/Ópalo han mostrado imagen y audio | Códecs, fuentes particulares, MIDI y referencias a extensiones se prueban al ejecutar |
| Rutas Unicode | Fixture conserva nombre con `ñ` en ZIP y manifiesto | Copia MTP y apertura por cada juego aún requieren verificación |
| Guardado y salida | Z/Ópalo han vuelto a biblioteca en consola; fixtures históricos de guardado | Ubicación y formato de guardado del tercer juego no probados; un error fatal del runtime puede cerrar el proceso |
| Bibliotecas nativas de PC | Diagnóstico enumera DLL/SO; staging las excluye | Los shims solo cubren APIs conocidas; hay que inspeccionar llamadas y ejecutar el juego |

## Dependencias concretas

Z y Ópalo declaran perfiles `ruby18` con nombres y estado propios; sus juegos no están dentro de Git. Z necesitó reparaciones puntuales durante la instalación M016, pero el preparador M017 no las aplica a otros juegos. Ópalo sirvió para descubrir ajustes generales del motor y de `NXFuente`; su perfil actual no inyecta shims propios. Reborn tiene un perfil `ruby31` con señal `Scripts/ScriptLoader.rb`, `nx-reborn.rb`, tres shims de servicios y rutas `Scripts`, `patch`, `stdlib`, `gems`. Es una configuración específica y visible, sin excepciones nuevas añadidas al motor en esta misión.

Antes de este bloque, el preparador seleccionaba un perfil que exigía título y fichero aunque solo coincidiera el título. Ahora exige ambas señales. Deduce Ruby 3.1 cuando hay DLL Ruby 3 identificable y RGSS1 cuando coexisten `RGSS10*.dll` y un `Game.exe` pequeño; otros casos requieren `--family`. Un valor explícito que contradiga la DLL detectada falla. El diagnóstico comprueba `Scripts.rxdata` por cabecera, `mkxp.json` y sus carpetas de parches, RGSS 1, dependencias nativas y `Game.rgssad` de llave vacía; registra la fase `preparacion` y `runtime_verified=false`. Las bibliotecas `.so`, `.dylib` y `.bundle` se excluyen igual que las DLL/EXE. Una carpeta de parches vacía se preserva en el staging.

No se reescribe ningún script original. El staging es privado, se crea en parcial y se publica con manifiesto. El diagnóstico de dependencias nativas es conservador: presencia no implica que se llamen obligatoriamente, y ausencia no demuestra que el juego no use una API no soportada.

## Pruebas sintéticas y fases

`python tests/test_prepare_launcher_game.py` cubre carpeta y ZIP, CRC, colisión, rutas inseguras, manifiesto y reutilización, fuente alterada, RGSS2, script sin cabecera Marshal, `mkxp.json` de RGSS distinto o dimensión inválida, archivo RGSS no vacío, perfil con señal incompleta, DLL/SO excluidos, carpeta `patch` vacía, nombre Unicode, 512×384 y 640×480 en la configuración y falta de portada. `tests/run-launcher-host.cmd <fixture>` cubre biblioteca vacía, una/dos entradas y 80 entradas sintéticas, nombres de 96 bytes, metadatos inválidos, portada corrupta como ruta que la UI debe degradar, persistencia de orden/favoritos y menú rápido. Estas pruebas verifican **preparación** y **escaneo**. El arranque, la ejecución de scripts, gráficos/audio, guardado y salida de un juego nuevo siguen pendientes de consola. Los logs existentes distinguen arranque del launcher, carga del motor, Ruby y salida, pero un fallo fatal externo puede no dejar última transición.

## Tercer juego: Pokémon Reborn 19.5.0

La [página oficial del equipo](https://www.rebornevo.com/pr/download/) ofrece Reborn 19.5 y enlaza su ZIP Windows mediante `https://pkmnfan.games/reborn-win`, que redirigió a `https://www.rebornevo.com/downloads/rebornremote/Reborn_19.5/Reborn-19.5.0-windows.zip`. La [presentación oficial](https://www.rebornevo.com/pr/index.html/) identifica RPG Maker XP y Essentials. Se eligió por su familia Ruby 3.x, cargador `Scripts.rxdata` que usa scripts externos, carpetas `patch`/`stdlib`/`gems` y dependencias nativas de PC: ejerce rutas distintas de Z y Ópalo. El perfil Reborn heredado corresponde a pruebas previas de otra versión; no acredita 19.5.

ZIP descargado solo a `local/m026/Reborn-19.5.0-windows.zip`: 656 291 383 bytes, SHA-256 `56dd8aa431d7d16605b621df654b26eafc9e80ec7437a3b8031b3fbd4b68ba08`. Contiene 12 634 archivos: 7 684 de Graphics, 2 637 de Audio, 932 de Data, 9 de Fonts y scripts externos. `Data/Scripts.rxdata` tiene cabecera Marshal; `mkxp.json` declara 512×384 y `patch`, carpeta vacía que ahora se conserva. Ocho bibliotecas nativas de PC se enumeran para revisión y no se distribuyen en el staging. El diagnóstico no encontró bloqueo estructural. El staging final `local/m026/reborn-stage-v3/` tiene 1 474 archivos y 11 262 recursos en `datos.zip`; `local/m026/reborn-stage-v3.manifest.json` contiene hashes y el formato 2 del preparador. Su lista de archivos y hashes coincide exactamente con el staging v2 instalado. La portada queda en reserva por ausencia de una candidata inequívoca. Una instalación en `local/m026/library/games/pokemon-reborn` pasó verificación de hashes; el staging v2 también pasó una reutilización tras comprobarlo antes de activar el formato 2.

**Estado MTP:** `SD Card/switch/FangameNX/games/pokemon-reborn` no existía antes de la transferencia. Se copió solo esa carpeta; Z, Ópalo, configuraciones y guardados no se tocaron. La lectura de vuelta a `local/m026/mtp-readback/pokemon-reborn/` reprodujo los 1 474 archivos y la carpeta `patch` vacía. Cada archivo se contrastó con tamaño y SHA-256 del manifiesto; `datos.zip` dio `6d592614b43e17a046c69ebeded9a00f701aafa88100ceaf196c12ce8c44dd3e`. No hay prueba física de arranque, ejecución, guardado ni retorno de Reborn 19.5. Ningún recurso del juego se incorpora al repositorio. El NRO b42 instalado, SHA-256 `8c140969ede7af283e15c0c89cfbf55398383b3ce27bf1383adf6315eb93930b`, permanece igual; no hay cambios del motor que exijan un NRO nuevo.
