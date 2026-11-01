# Misión 004 — primera prueba en hardware: preparación y bloqueo estático

Fecha: 1 de noviembre de 2026. Rama `test/fnx-004-hardware`, creada desde `fix/fnx-003-cruby-link` (`50bd4d4`). No se ha escrito en microSD, iniciado el NRO en Switch ni modificado el runtime. Los resultados de consola de esta misión son **ninguno**.

## Puerta de seguridad de la fase A

**Estado: no autorizar todavía el arranque en Switch 1.** La comparación controlada tomó un ELF sin postproceso, aplicó `fixup-arm64.py` a una copia y examinó cada byte. El guion recorre solo segmentos `PT_LOAD` ejecutables y cambió exactamente **15 palabras AArch64 de 4 bytes**: cinco `chkfeat x16` a `nop`, dos `mrs gcspr_el0` a `movz`, tres `gcspopm` a `nop`, dos `mrs tpidr2_el0` a `movz`, un `msr tpidr2_el0` a `nop` y dos `smstart/smstop` a `nop`. Tamaño, cabecera ELF, tabla de segmentos y tabla de secciones quedaron idénticos. Por tanto cambia instrucciones en `.text`, sin eliminar secciones TLS ni cambiar posiciones o metadatos de carga. La nota GNU Build ID quedó idéntica (`de57cd2ee3ede24755da4a7d5566859102ee798e`) pese al cambio de bytes: sirve como identificador del enlace original, pero no como hash del ELF posprocesado. Los hashes de ambos ELF son distintos. Véase [`log-m004-auditoria-estatica.txt`](log-m004-auditoria-estatica.txt).

La desensambladura del ELF **ya parcheado** aún contiene **23 instrucciones** de familias posteriores al Cortex-A57: seis `cntd`, dieciséis `str za[...]` y un `addsvl`. `__aarch64_have_sme` es una constante de un byte con valor cero en `.rodata` y protege la rutina `__arm_tpidr2_save`; esto explica por qué sus instrucciones SME pueden no ejecutarse en esta configuración. Sin embargo, hay cinco `cntd` adicionales en rutinas de desenrollado (`execute_stack_op`, `uw_update_context_1` y otras). No se ha demostrado que todas esas ramas sean inalcanzables en una ejecución real. La comparación exacta de 15 cambios no equivale a una auditoría exhaustiva de todo el código enlazado.

Arm clasifica el [Cortex-A57 como Armv8.0-A](https://developer.arm.com/-/media/Arm%20Developer%20Community/PDF/Learn%20the%20Architecture/Understanding%20the%20Armv8.x%20extensions.pdf); [SVE se introdujo en Armv8.2-A](https://developer.arm.com/-/media/Arm%20Developer%20Community/PDF/Learn%20the%20Architecture/102131_0100_01_SVE_and_Neon_coding_compared.pdf?revision=feaaf72e-a941-461c-bd92-0d960d0f8615) y [STR ZA pertenece a SME](https://developer.arm.com/community/arm-community-blogs/b/architectures-and-processors-blog/posts/arm-scalable-matrix-extension-introduction-p2). Aplicamos la puerta exigida por esta misión: mientras queden instrucciones incompatibles sin una justificación completa de sus rutas, **no se empieza una prueba de hardware**. Esto no demuestra que el NRO vaya a fallar; demuestra que su compatibilidad de ejecución aún no está confirmada. No se ampliará `fixup-arm64.py` con NOP adicionales ni se modificará libgcc a ciegas.

## Archivo y contrato de arranque examinados

El `mkxp-z-nx/rgss-switch.nro` de la Misión 003 mide **17 750 457 bytes**, SHA-256 `9bec33460205b3989bf405740c596c1b370311938bd7df586fb42e2e0ad7b496`. `comprobar-nro.py` encontró `NRO0`, `ASET` versión 0, icono de 12 993 bytes, NACP de 16 384 bytes y RomFS de 1 728 bytes, con todos los rangos dentro del archivo. El RomFS contiene solo `LEEME.txt`; no hay `juego.txt` ni rutas de un fangame. El ELF emparejado es ELF64 AArch64 PIE, SHA-256 `430be0c747f31c909537cf910a1dcdee334c7c9463894926b35eb13211c19a0c`. Ambos hashes se volvieron a verificar después de la auditoría; el ELF original se restauró sin cambios.

El código de `src/mkxp-z/src/main.cpp` elige primero el directorio de `argv[0]` si allí existe `mkxp.json`; solo después prueba `romfs:/juego.txt`. Por eso el NRO y `mkxp.json` del ensayo deben estar juntos en `SD:/switch/fnx004-sintetico/`. `nx_revision_arranque()` se ejecuta antes de SDL/Ruby y exige `mkxp.json`, `lib/`, `lib/nx-compat.rb`, `lib/socket.rb` y, en modo de carpetas sueltas, `Data/Scripts.rxdata`, `Graphics/` y `Audio/`. `src/mkxp-z/src/config.cpp` acepta `customScript`; `binding-mri.cpp` lee y evalúa ese archivo en lugar de cargar `Scripts.rxdata`. Es un contrato inferido del código y comprobado por archivos locales, **no una ejecución validada en Switch**.

## Caso sintético de la fase B

Fuentes propias versionadas: [`fixtures/fnx004/mkxp.json`](fixtures/fnx004/mkxp.json), [`probe.rb`](fixtures/fnx004/probe.rb), [`entrada.txt`](fixtures/fnx004/entrada.txt) y [`Game.ini`](fixtures/fnx004/Game.ini). `customScript` apunta a `probe.rb`. El script registra el directorio actual, lee `entrada.txt`, escribe y relee `fnx004-escritura.txt`, dibuja dos bandas magenta y verde y, si funciona la fuente ya incorporada al motor, muestra `FNX-004 RUBY OK`. Tras 480 llamadas a `Graphics.update`, registra `FNX004 RUBY COMPLETADO` y devuelve el control para una salida normal. En caso de excepción, escribe la clase y mensaje en `fnx004-probe.log` y vuelve a lanzar el error.

`Data/Scripts.rxdata` es un **marcador propio de cero bytes**, necesario únicamente para la revisión previa; `customScript` evita leerlo. `Graphics/` y `Audio/` son directorios vacíos. `lib/nx-compat.rb` y `lib/socket.rb` se copian del código propio del port para satisfacer la revisión; no se cargan mediante `preloadScript` en este ensayo. No hay recursos, scripts ni marcas de Pokémon ni de otros fangames. `ruby -c` devolvió `Syntax OK`, `mkxp.json` se analizó como JSON y se verificó la presencia de todos los archivos exigidos. **No se ha ejecutado Ruby dentro del NRO.**

## Paquete local de la fase C — bloqueado para microSD

Está preparado en `local/fnx004-hardware-probe/`, ignorado por Git, con un manifiesto SHA-256 en `MANIFEST.sha256.txt`. La subcarpeta `switch/fnx004-sintetico/` contiene el NRO sin alterar de la Misión 003, los cuatro archivos sintéticos, las dos bibliotecas Ruby propias, el marcador vacío y los directorios `Graphics/` y `Audio/`. El NRO copiado conserva el SHA-256 indicado arriba. No se generó NSP ni se distribuyó el paquete. El NRO incluye el icono heredado y dependencias estáticas; las condiciones de distribución siguen pendientes de la revisión indicada en la Misión 003.

Como no se rehizo el NACP, hbmenu puede mostrar el nombre heredado `rgss-switch` aunque el archivo se llame `fnx004-sintetico.nro`. Para identificarlo durante una prueba futura se usará la ruta del archivo y su hash, no solo el nombre visible.

**Procedimiento reservado para cuando la auditoría de CPU se cierre y el usuario autorice expresamente escribir en la microSD:**

1. Comprobar que el paquete y el NRO nuevo que se vaya a probar tengan hashes registrados, y que `SD:/switch/fnx004-sintetico/` no exista. Si existe, detenerse para evitar sobrescrituras.
2. Copiar únicamente `local/fnx004-hardware-probe/switch/fnx004-sintetico/` a `SD:/switch/fnx004-sintetico/`. Verificar de nuevo en la tarjeta el SHA-256 del NRO y la presencia de `mkxp.json`, `probe.rb`, `entrada.txt`, `Data/Scripts.rxdata`, `Graphics/`, `Audio/` y `lib/`.
3. Abrir hbmenu bajo Atmosphère con memoria de aplicación mediante title override, seleccionar `fnx004-sintetico.nro` y observar por separado: llegada a `main()`, revisión previa, inicio de Ruby, bandas en pantalla y retorno al menú al finalizar. No instalar NSP ni tocar sysNAND, emuMMC, Atmosphère, Hekate u otras aplicaciones.
4. Conservar `mkxp-boot.log`, `mkxp-debug.log`, `mkxp-ruby.log`, `errorlog.txt` si aparecen, y los archivos `fnx004-probe.log` y `fnx004-escritura.txt`. Registrar también pantalla, tiempo aproximado, modo de hbmenu, resultado de salida y cualquier error. Si no aparece pantalla, consultar primero `mkxp-boot.log`; si llega a Ruby, comprobar las marcas `lectura=OK`, `escritura=OK`, `pantalla=...` y `FNX004 RUBY COMPLETADO`.

Estos pasos son una guía condicional, **no una autorización para ejecutarlos ahora**. No se ha solicitado acceso a la microSD porque la puerta estática no se ha superado.

## Resultados reales y próximo paso

| Nivel | Resultado de esta misión |
|---|---|
| Compilación y formato | Verificados localmente en FNX-003; hashes reconfirmados. |
| Contrato y fixture sintético | Revisados estáticamente; sintaxis Ruby y estructura local correctas. |
| Arranque en Switch 1 | No intentado: bloqueo de instrucciones SVE/SME. |
| Ejecución de Ruby y lectura/escritura en microSD | No intentadas. |
| Juego RGSS completo | No intentado; el marcador `Scripts.rxdata` no es un juego. |

**FNX-003** mantiene su resultado de compilación reproducida, pero sigue sin validación de ejecución. **FNX-004** queda detenido en la fase A para hardware. Siguiente tarea recomendada: obtener o construir de forma aislada una variante de `libgcc` cuya ruta de desenrollado sea verificablemente compatible con Armv8.0-A, o demostrar con análisis de flujo y CFI que las instrucciones restantes no pueden alcanzarse en este binario. Ello puede requerir una toolchain distinta o cambios profundos y necesita una propuesta separada antes de modificar SDK o runtime. Tras generar un NRO nuevo, repetir esta auditoría estática, reconstruir el paquete y solicitar autorización explícita para escribir en la tarjeta.
