# Informe para ChatGPT · FangameNX M012 / FNX-017

Fecha: 10 de noviembre de 2026. Rama `test/fnx-017-five-cycles`, creada desde M011 `83b5491`. El hash del commit final se comunica junto a este informe.

## Resultado ejecutivo

**No se superó el criterio de cinco ciclos consecutivos correctos.** En la serie controlada, los ciclos 1 y 2 pasaron: pantalla, lectura, A, X, B y retorno autónomo al Homebrew Menu, confirmados por el propietario y por logs de Ruby/SDL. En el ciclo 3, X no cambió el color y el log Ruby no registra su evento; A, B y el cierre funcionaron. La serie se detuvo. A petición del propietario, un cuarto intento diagnóstico terminó en `Atmosphere panic occurred!`, `Tittle ID: 0100000000000034`, `Error: std::abort (0xFFE)` y reinicio forzado. Después del reinicio, el propietario hizo **una única prueba final** con el mismo NRO: esperó cinco segundos, pulsó A, mantuvo X alrededor de un segundo y salió con B. Comunicó que todo funcionó. Los logs de esa quinta ejecución confirman lectura/escritura, A/X/B, Ruby y cierre SDL. Balance: **tres ejecuciones correctas (1, 2 y 5), una con fallo de X (3) y una interrumpida por pánico (4)**.

## Evidencia conservada

Se identificó un único dispositivo WPD/MTP `Nintendo Switch` (VID `057E`, PID `201D`) y se accedió solo a `SD Card/switch/fnx016-sdl-fix/` y al informe fatal indicado por el propietario en `SD Card/atmosphere/fatal_errors/`, sin letras de unidad. El procedimiento fue exclusivamente de lectura: cuatro logs por instantánea y una copia del informe AFE2, con tamaños y SHA-256 locales. No se alteraron archivos de la Switch ni se accedió al paquete dual M011. Las copias completas, excluidas de Git, están en `local/fnx017-stability-20261110-serie/`:

- `baseline/`: copia local de la captura mixta inicial; se preserva el original en `local/fnx017-stability-20261110/ciclo-01/`.
- `ciclo-01/` a `ciclo-05/`: cuatro logs y `manifest.json` en cada carpeta. El quinto corresponde a la prueba final tras el reinicio.
- `fatal-report/report_9b251d017c.bin`: 1 104 bytes, SHA-256 local `806ee5b429f57174d42d48f202946c175dbca73b3a9219b70571bdb12ea1bf06`. No se confirma igualdad byte a byte con el remoto porque MTP no aportó una suma remota.
- El primer intento de línea base registró además dos ejecuciones Ruby y un arranque *applet* que abortó antes de Ruby. El propietario confirmó varios lanzamientos y pulsaciones; esta captura **no** cuenta como ciclo controlado.

El detalle por ciclo, los veinte hashes de logs y resultados del verificador están en [`estabilidad-cinco-ciclos-fnx-017.md`](estabilidad-cinco-ciclos-fnx-017.md). Para los ciclos 1, 2 y 5, `verificar-ciclos-fnx-017.py --expected 1` devolvió 0 y confirmó un solo bloque nuevo de cada registro. Para el ciclo 3 devolvió 1 por ausencia de `boton=X color=verde`; el cierre sí llegó a `[exit] 08 before return main`. Para el ciclo 4 devolvió 1 por ausencia de A: `fnx009-probe.log` llegó a `pantalla=inicial` sin eventos A/X/B ni finalización Ruby, y `mkxp-boot.log` no contiene ninguna marca de salida. El marcador de escritura avanzó en todos los lanzamientos.

## Interpretación y límites

El X físico se mapea en el código de Switch a `SDL_CONTROLLER_BUTTON_Y` y luego a `Input::A`; `Input.trigger?` exige un flanco. Funcionó en los ciclos 1, 2 y 5, y no produjo evento Ruby en el 3. Los logs no contienen eventos SDL crudos, así que no permiten distinguir entre pulsación no recibida, sondeo perdido, estado previo de Input u otra causa. En el debug del ciclo 3 faltan líneas informativas de GL presentes en los otros, aunque boot marca `initGL OK`; no hay evidencia de relación con X.

El `.bin` comienza con `AFE2`, error `0xFFE` y Title ID `0100000000000034`. La [configuración oficial de Atmosphère](https://github.com/Atmosphere-NX/Atmosphere/blob/master/atmosphere.mk) asigna ese ID a su módulo `fatal`. Un [colaborador del proyecto](https://github.com/Atmosphere-NX/Atmosphere/issues/816) describe un caso con ese ID como un error durante la presentación de otro fatal. **Inferencia, no diagnóstico confirmado:** el AFE2 puede describir un fallo secundario de `fatal`, sin revelar el desencadenante inicial. No prueba que FangameNX ejecutara `std::abort`. El propietario no recuerda el instante exacto del pánico. El pico de montón registrado por FangameNX es **199 MB en cada uno de los cinco lanzamientos**; no se ve crecimiento en esa métrica por proceso, pero no se descartan fugas no medidas ni otros fallos del sistema.

FNX-016 conserva la evidencia de su primera ejecución satisfactoria. FNX-017 **no** acredita cinco ciclos correctos seguidos ni compatibilidad con fangames. M011 preparó un paquete dual local, pero **no se instaló ni se probó** en esta misión. No se modificó el runtime, NRO, fixture instalado, otros proyectos ni almacenamiento externo convencional. El `__pycache__` preexistente quedó fuera de Git.

## Continuación recomendada

1. El propietario quiere **pasar al desarrollo del lanzador genérico real** y dejar de repetir mini pruebas. La siguiente misión debería implementar un primer corte del selector de directorios externos conforme a [`ADR-004-lanzador.md`](ADR-004-lanzador.md), con aislamiento de configuración y logs y sin incluir juegos de terceros. Mantener el alcance mínimo y documentar el contrato antes de tocar el runtime.
2. Conservar esta anomalía de X y el pánico como riesgos abiertos; no adjudicarles una causa ni parchear botones, Atmosphère o el cierre SDL sin evidencia adicional. Una prueba futura debe responder a un riesgo concreto de la arquitectura real, no repetir la serie actual por inercia.
3. Antes de escribir en la Switch para instalar un nuevo lanzador o fixture, preparar un paquete revisable, preservar la instalación y logs actuales y obtener la autorización pertinente. Ninguna instalación nueva se hizo en M012.
