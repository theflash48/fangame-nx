# Portadas automáticas al añadir juegos al launcher

Fecha: 10 de noviembre de 2026. Cambio posterior a M016.

## Contrato

Una carpeta de juego ya preparada se añade con
`python packaging/install-launcher-game.py <preparado> <biblioteca>/games`.
El instalador exige un ID válido, un destino nuevo, `mkxp.json`, los dos shims
obligatorios y una de las estructuras de datos que admite el launcher. Copia
esa carpeta a la biblioteca local y elige la portada sin intervención manual.
No prepara directamente una carpeta cruda de RPG Maker XP ni escribe en la SD.

Si la instalación declara `cover` en `fangamenx.json`, se respeta. Si ya tiene
`cover.png` válido, se declara ese archivo. En otro caso se buscan imágenes PNG
de al menos 128×128, como máximo 2048×2048 y 8 MiB, con nombres reconocibles
(`titulo`, `title`, `portada`, `cover`, `titlelogo`, `logotitulo`, `logo`) dentro
de `Graphics/Pictures` o `Graphics/Titles*`, sueltas o en `datos.zip`. Se
priorizan `titulo` y `title`. Solo se elige una candidata si la prioridad más
alta es única. Se copia sin transformar como `cover.png` y se añade
`"cover": "cover.png"` al manifiesto. Si no hay una elección única, el
launcher usa su dibujo de reserva: no presenta una imagen arbitraria como
portada oficial del juego.

El escáner del launcher también reconoce un `cover.png`, `cover.jpg` o
`cover.jpeg` en la raíz aunque no haya campo `cover`, con el manifiesto
explícito por encima. Esta parte requiere un NRO reconstruido; **el NRO b25
instalado durante M016 no se sustituyó**. Las portadas generadas por el
instalador ya funcionan con b25 porque llevan el campo explícito.

## Verificación

Entorno: Windows, Python 3.13.1 y Visual Studio C++ mediante
`tests/run-launcher-host.cmd`.

| Comprobación | Resultado observado |
|---|---|
| `python -m py_compile packaging/auto_cover.py packaging/install-launcher-game.py` | Sin errores |
| `choose(local/m016/stage-pokemon-z-2.18)` | Encuentra `Graphics/Pictures/titulo.png`, 200 880 bytes, dentro de `datos.zip` |
| Alta local de una fixture preparada con ese PNG dentro de ZIP | Crea `games/juego-prueba/cover.png` y `fangamenx.json`; SHA-256 de la portada coincide con el recurso original |
| ZIP sintético con `titulo.png` y `title.png` de igual prioridad | No elige ninguna portada (`None`) |
| `tests/run-launcher-host.cmd local/m013/package-bbf9d4fee9` | `launcher_host: OK`; cubre portada automática de raíz y prioridad de manifiesto |

La imagen usada en la prueba local procede de la copia privada del propietario
y quedó bajo `local/`, excluido de Git. No se incorporó material del juego al
repositorio. La detección sobre otros fangames y la alternativa de raíz en un
NRO nuevo todavía requieren pruebas con otros juegos y en hardware.
