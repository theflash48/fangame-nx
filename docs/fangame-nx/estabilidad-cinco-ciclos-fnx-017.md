# Misión 012 · cinco ciclos de estabilidad FNX-017

Rama `test/fnx-017-five-cycles`, desde M011 `83b5491`. El propietario autorizó únicamente la **lectura** de logs del fixture ya instalado `SD Card/switch/fnx016-sdl-fix/` mediante WPD/MTP. No se usan letras de unidad ni se accede a otros dispositivos. La prueba física la realiza el propietario.

## Preparación y línea base

El protocolo de M011 está en [`estabilidad-multifixture-fnx-017.md`](estabilidad-multifixture-fnx-017.md). El script Ruby del fixture abre `fnx009-probe.log` en modo `a` y `fnx009-escritura.txt` en `ab`. El código de `mkxp-boot.log` usa `O_APPEND` con rotación a 1 MiB; `mkxp-debug.log` usa `fopen(..., "a")`. Los cuatro logs se acumulan entre arranques. Cada copia local se conserva en una carpeta diferente, sin sobrescribir la evidencia M016.

El 10 de noviembre de 2026 se identificó **un único** dispositivo WPD `Nintendo Switch`, VID `057E`, PID `201D`, y dentro de él `SD Card/switch/fnx016-sdl-fix/` con los cuatro registros esperados. Se copiaron en modo lectura a `local/fnx017-stability-20261110/baseline/`, con `manifest.json` local. Los hashes locales coincidieron con la evidencia M016 del 2 de noviembre; esto comprueba la igualdad de las **copias obtenidas**, no constituye una lectura byte a byte del remoto original.

| Registro inicial | Bytes | SHA-256 local |
|---|---:|---|
| `fnx009-probe.log` | 332 | `39c7caf1601c176c2e7b1cade8ef551f74689786a63f5b0632ae32204f501ac9` |
| `fnx009-escritura.txt` | 20 | `f715634853244f18c30bfb378193bf828ab65b41c45c7eeff61b27aa1a7341d3` |
| `mkxp-boot.log` | 4 043 | `42e12661cc0a3f99d15cc9eeccec1dda2bf19e96112238344e22c400dbcd01da` |
| `mkxp-debug.log` | 888 | `76095ba9983a4b8e8e9338bddc3459093a5dbaaf0eb6afc6a8b650d68e06a8a1` |

El procedimiento local `local/fnx017-stability-20261110/copy-mtp-logs.ps1` exige el mismo dispositivo WPD y la ruta exacta; copia solo estos cuatro archivos, espera a que cada copia local estabilice su tamaño, calcula SHA-256 y rechaza carpetas locales preexistentes. No invoca operaciones de escritura sobre el objeto MTP. El script y las copias están excluidos de Git.

## Captura inicial que exige aclaración

El propietario comunicó `LECTURA OK`, A +1, X con cambio azul/verde y B con retorno rápido y autónomo al Homebrew Menu. Se recuperaron cuatro registros en `local/fnx017-stability-20261110/ciclo-01/`, con hashes en su `manifest.json`. Sin embargo, el sufijo desde la línea base contiene **tres** marcas `[build]`, **dos** bloques Ruby completos y **dos** cierres con `SDL_DestroyWindow OK` y `[exit] 08 before return main`. La primera marca de arranque indica modo *applet* con memoria reducida (248 MB de montón) y aborta durante la revisión de arranque, antes de Ruby. Los dos arranques siguientes indican memoria de aplicación y terminan normalmente. El primer bloque Ruby registra siete A y ninguna X; el segundo registra cuatro X y cuatro A. Ambos incluyen lectura/escritura OK y salida por B.

| Copia local mixta | Bytes | SHA-256 local |
|---|---:|---|
| `fnx009-probe.log` | 937 | `02fd3e93ecafec1af372d8c6d0238f97c6fef8f8bff68ea4312b70902526d9c2` |
| `fnx009-escritura.txt` | 60 | `60a38dbaecdc5660c04b3ae611bdab9da8bb5e80f5e6fdb84b7214ca72da4b07` |
| `mkxp-boot.log` | 13 386 | `39d14673933fa848113681f0e66d232ac6d0fcfac283916af3d6f70a91484e6c` |
| `mkxp-debug.log` | 3 064 | `208ba353df2b7352b828a11ff19c3610c44d6fd621ae0afa642645f2b0509692` |

Por tanto, esta carpeta es una **captura mixta de varios intentos**, no una evidencia aislada de un único ciclo A→X→B. `verificar-ciclos-fnx-017.py --expected 1` rechazó la comparación: `bloques Ruby/boot/debug=(2, 3, 3), esperados=1`. El propietario confirmó que abrió la aplicación más de una vez y pulsó A/X varias veces. No se atribuye sin más un bloque concreto al resultado visual comunicado. La captura permanece íntegra, sin alterar los originales MTP ni las copias locales.

Para empezar una serie contable se creó `local/fnx017-stability-20261110-serie/` y se copiaron **solo localmente** los cuatro archivos de la captura mixta a su `baseline/`. Los cuatro hashes coinciden con la captura anterior. `copy-mtp-logs.ps1` se copió a la raíz de esta nueva serie; guardará `ciclo-01` a `ciclo-05` sin sobrescribirlos. La nueva línea base no requiere otra lectura MTP ni modifica la Switch.

## Resultados por ciclo

Para cada ciclo de la **serie controlada**, el propietario abre la aplicación **una sola vez con memoria de aplicación**, observa `LECTURA OK`, pulsa A, X y B **una vez cada uno** y comunica si vuelve al Homebrew Menu sin HOME. Después abre USB File Transfer; se guardan cuatro copias locales en `local/fnx017-stability-20261110-serie/ciclo-NN/`. La verificación compara cada instantánea con la anterior mediante [`verificar-ciclos-fnx-017.py`](verificar-ciclos-fnx-017.py) y exige exactamente un nuevo bloque Ruby, boot y debug, los eventos A/X/B, lectura/escritura OK, `SDL_DestroyWindow OK` y la marca final de `main`. El resultado visual no se infiere de los logs.

| Ciclo | Pantalla y lectura observadas | A / X observados | B y regreso propio | Logs, hashes y validación | Anomalías |
|---:|---|---|---|---|---|
| 1 | Pendiente | Pendiente | Pendiente | Pendiente | — |
| 2 | Pendiente | Pendiente | Pendiente | Pendiente | — |
| 3 | Pendiente | Pendiente | Pendiente | Pendiente | — |
| 4 | Pendiente | Pendiente | Pendiente | Pendiente | — |
| 5 | Pendiente | Pendiente | Pendiente | Pendiente | — |

La evaluación de estabilidad queda pendiente de completar los cinco ciclos. El paquete dual M011 no se instala ni se prueba en esta misión.
