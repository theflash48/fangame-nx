# Informe para ChatGPT · FangameNX M012 / FNX-017

Fecha: 10 de noviembre de 2026. Rama `test/fnx-017-five-cycles`, creada desde M011 `83b5491`. El hash del commit final se comunica junto a este informe.

## Resultado ejecutivo

**La prueba de cinco ciclos no se completó ni se superó.** Dos ciclos controlados consecutivos cumplieron la pauta de pantalla, lectura, A, X, B y retorno autónomo al Homebrew Menu; los cuatro logs de cada uno confirman lectura/escritura, eventos, terminación Ruby y cierre SDL. En el ciclo 3, el propietario vio que X no cambió el color y el log Ruby confirma que no se registró el evento X; A, B, lectura, escritura y cierre funcionaron. La serie se detuvo. A petición posterior del propietario se intentó un cuarto ciclo **diagnóstico**: la consola mostró `Atmosphere panic occurred!`, `Tittle ID: 0100000000000034` y `Error: std::abort (0xFFE)`. El propietario tuvo que forzar el reinicio, ya realizado. La pantalla indicó `Report Saved to atmosphere/fatal_errors/report_9b251d017c.bin`. **No se ha leído ese informe ni se recuperaron logs posteriores al pánico.** Se desconoce el momento y la causa. El ciclo 5 no se ejecutó.

## Evidencia conservada

Se identificó un único dispositivo WPD/MTP `Nintendo Switch` (VID `057E`, PID `201D`) y se accedió solo a `SD Card/switch/fnx016-sdl-fix/`, sin letras de unidad. El procedimiento fue exclusivamente de lectura: cuatro logs copiados a carpetas locales nuevas, con manifest de tamaños y SHA-256. No se alteraron archivos de la Switch ni se accedió al paquete dual M011. Las copias completas, excluidas de Git, están en `local/fnx017-stability-20261110-serie/`:

- `baseline/`: copia local de la captura mixta inicial; se preserva el original en `local/fnx017-stability-20261110/ciclo-01/`.
- `ciclo-01/`, `ciclo-02/`, `ciclo-03/`: cuatro logs y `manifest.json` en cada carpeta.
- El primer intento de línea base registró además dos ejecuciones Ruby y un arranque *applet* que abortó antes de Ruby. El propietario confirmó varios lanzamientos y pulsaciones; esta captura **no** cuenta como ciclo controlado.

El detalle por ciclo, los doce hashes y resultados del verificador están en [`estabilidad-cinco-ciclos-fnx-017.md`](estabilidad-cinco-ciclos-fnx-017.md). Para los ciclos 1 y 2, `verificar-ciclos-fnx-017.py --expected 1` devolvió 0 y confirmó un solo bloque nuevo de cada registro. Para el ciclo 3 devolvió 1 por ausencia de `boton=X color=verde`; los demás hitos, incluidos `SDL_DestroyWindow OK` y `[exit] 08 before return main`, están presentes. Del cuarto intento solo consta la pantalla fatal comunicada por el propietario; el archivo `.bin` indicado y los logs del fixture siguen sin recuperar.

## Interpretación y límites

El X físico se mapea en el código de Switch a `SDL_CONTROLLER_BUTTON_Y` y luego a `Input::A`; `Input.trigger?` exige un flanco. Funcionó en los ciclos 1 y 2 y no produjo evento Ruby en el ciclo 3. Los logs no contienen eventos SDL crudos, así que no permiten distinguir entre pulsación no recibida, sondeo perdido, estado previo de Input u otra causa. En el debug del ciclo 3 faltan líneas informativas de GL presentes en los dos anteriores, aunque boot marca `initGL OK`; no hay evidencia de relación con X. Tampoco hay evidencia para relacionar el crash del cuarto intento con X o con el cierre SDL.

FNX-016 conserva la evidencia de su primera ejecución satisfactoria. FNX-017 **no** acredita estabilidad tras cinco ciclos ni compatibilidad con fangames. M011 preparó un paquete dual local, pero **no se instaló ni se probó** en esta misión. No se modificó el runtime, NRO, fixture instalado, otros proyectos ni almacenamiento externo convencional. El `__pycache__` preexistente quedó fuera de Git.

## Continuación recomendada

1. El reinicio forzado ya se hizo. Esperar a que el propietario confirme que quiere retomar la investigación. No iniciar nuevos ciclos ni modificar la consola.
2. Con su indicación, leer por MTP **solo** los cuatro logs del fixture y compararlos con `ciclo-03/` para determinar si el cuarto intento añadió Ruby, entrada o cierre. Preservar copias y hashes; no sobrescribir las existentes. El informe `atmosphere/fatal_errors/report_9b251d017c.bin` está **fuera** de la autorización MTP de M012 y necesita autorización separada antes de recuperarlo.
3. Diseñar después una prueba de entrada mínima que registre evento SDL y estado/flanco RGSS de X con tiempos. Mantener las evidencias actuales intactas y no atribuir la causa del crash antes de ver datos.
