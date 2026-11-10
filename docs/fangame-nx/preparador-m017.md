# M017 · Preparación genérica local de juegos para FangameNX

Fecha: 2026-11-10. El preparador es `packaging/prepare-launcher-game.py`.
Trabaja en el PC y no accede a la Switch. Reutiliza `juegos/*/perfil.json`,
`runtime/mkxp.base.json`, el catálogo de shims, `runtime/lib/`, la lista de
stdlib Ruby 3.1.7 y `packaging/auto_cover.py`. No incorpora juegos al código
fuente ni presupone que una estructura válida sea jugable.

## Auditoría de la preparación M016

| Paso | Decisión M017 |
|---|---|
| Validar ZIP, empaquetar recursos, componer `mkxp.json`, copiar runtime y generar hashes | Regla reutilizable del preparador |
| Consultar perfil y shims, conservar scripts sueltos cuando el contrato lo requiere | Regla reutilizable del contrato existente |
| Seleccionar una única imagen de título y declarar `cover.png` | Regla reutilizable con reserva ante ambigüedad |
| Prefijo `Pokemon Z V2.18/`, nombre `pokemon-z`, autor EricLostie, 255 scripts | Evidencia específica de Pokémon Z; no codificada como regla general |
| Renombrar `Créditos.txt` a `Creditos.txt` y recrear `lib/benchmark` por anomalías MTP | Correcciones específicas de aquella transferencia; no se aplican a otros juegos |

El staging privado `local/m016/` y el ZIP de origen no se tocaron en M017.

## Uso

```powershell
python packaging/prepare-launcher-game.py local/mi-juego.zip --diagnose
python packaging/prepare-launcher-game.py local/mi-juego.zip --family rgss1 --id mi-juego --output local/m017/mi-juego
python packaging/install-launcher-game.py local/m017/mi-juego local/mi-biblioteca/switch/FangameNX/games
```

El primer comando imprime un informe JSON y no crea staging. El segundo
requiere una fuente ZIP o carpeta existente; `--output` debe ser un destino
nuevo dentro de `local/`. Si el perfil y `Game.ini` no aclaran la familia Ruby,
se exige `--family ruby31`, `ruby18` o `rgss1`. `--profile` permite elegir un
perfil existente; `--name`, `--author` y `--version` fijan metadatos sin
atribuir al juego el autor del port. El tercer comando añade la carpeta
preparada a una biblioteca **local** nueva, verifica hashes de los archivos
copiados, completa la portada y publica el directorio con el ID del
manifiesto. Ni el preparador ni el instalador escriben en MTP.

## Contrato y límites

El diagnóstico exige `Game.ini` con `Title=`, `Data/`, `Graphics/`, `Audio/`,
scripts indicados por `Scripts=` o `Data/Scripts/`, familia Ruby determinada y
dependencias locales del runtime. Un ZIP puede tener un único directorio raíz
que contenga el juego. Se rechazan rutas absolutas, `..`, separadores
Windows, enlaces, entradas cifradas, nombres duplicados sin distinguir
mayúsculas y conflictos archivo/directorio. La preparación valida los CRC del
ZIP. Se aplican límites de 100 000 archivos y 8 GiB de contenido de origen.
El contenedor de recursos debe quedar por debajo de 4 GiB con margen para
metadatos ZIP: no se activa ZIP64 hasta comprobarlo con PhysFS en el runtime.

Los recursos de `Data/`, `Graphics/`, `Audio/`, `Fonts/` y `Movies/` van a
`datos.zip` con método `STORED`; `Data/Scripts/` permanece suelta cuando
existe. Se descartan binarios de Windows, archivos de escritorio, partidas
`Game*.rxdata` de la raíz y carpetas de desarrollo conocidas. Se conserva un
`Game.rgssad` original o se crea la llave vacía de ocho bytes. La configuración
se compone con las claves permitidas del juego, el perfil y los shims, según
`CONVENCIONES.md`. Los cambios específicos del juego viven en su perfil; no
se añaden reglas de Pokémon Z al motor o al preparador. Un archivo cifrado con
otro nombre, scripts no visibles, bibliotecas nativas o adaptaciones especiales
pueden requerir trabajo manual y prueba real. El resultado no se declara
compatible por pasar el diagnóstico.

Un archivo RGSS con otro nombre o un perfil que fija `execName` distinto de
`Game` bloquean el staging automático: sus alias no se adivinan. Los nombres
no ASCII se conservan y se avisa de que la copia MTP debe verificarse; el
preparador no aplica a todos los juegos el cambio de `Créditos.txt` que fue
específico de Pokémon Z.

El staging se construye primero en un directorio parcial privado y solo se
renombra al destino final tras generar los archivos. El manifiesto vecino
`<nombre-del-staging>.manifest.json` contiene fuente SHA-256, decisiones, advertencias,
rutas, tamaños y SHA-256 de cada archivo. Una repetición verifica fuente,
opciones y todos los archivos antes de reutilizar el resultado; si hay una
diferencia, conserva el staging existente y exige otro destino. Un parcial
interrumpido también se conserva para diagnóstico. Se comprueba el espacio
libre antes de copiar. Este flujo evita destruir un resultado válido; no
transfiere ni reanuda bytes en la Switch.

## Portadas y verificación

Se conserva el `cover` explícito. Los PNG se validan por estructura de chunks,
CRC, dimensiones de 128 a 2048 píxeles y tamaño máximo de 8 MiB. Si no hay
portada, se busca una imagen de título inequívoca en `Graphics/Pictures` o
`Graphics/Titles*`, suelta o empaquetada, y se declara como `cover.png`.
Empates o imágenes inválidas dejan la reserva geométrica. En el NRO b27 local,
la UI comprueba tamaño y dimensiones de PNG/JPEG antes de SDL_image y solo
carga la portada del juego mostrado. El b25 instalado conserva su contrato
anterior; el instalador escribe el campo `cover` que ese NRO necesita.

Pruebas locales: `python tests/test_prepare_launcher_game.py` terminó en
`prepare_launcher_game: OK` con carpeta y ZIP sintéticos, staging, manifiesto,
reutilización, cambio de fuente, ZIP inseguro, colisión de nombres, PNG
truncado, dimensiones excesivas, estructura incompleta y detección de un perfil cuyos indicadores son
directorios. `python -m py_compile` pasó. El escáner host
con Pulso y Órbita doble terminó en `launcher_host: OK`. Estas pruebas no
ejecutan un fangame ni la UI SDL en la Switch.
