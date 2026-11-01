# Misión 005 — compatibilidad Armv8.0-A del NRO

Fecha: 2 de noviembre de 2026. Rama `fix/fnx-015-armv8-compat`, creada desde `test/fnx-004-hardware` (`b99bcf3`). Alcance: auditoría estática y recompilación limpia del motor con el SDK ya instalado. No se ha ejecutado nada en Switch, copiado a microSD, creado un NSP ni modificado el runtime, `fixup-arm64.py`, el SDK o una biblioteca.

## Resultado y criterio de aceptación

El NRO nuevo **contiene todavía 23 instrucciones SVE/SME reales en `.text`**, todas conservadas también en el NRO. No son datos interpretados erróneamente como código. Su procedencia es la `pic/libgcc.a` de devkitA64 r30-1 / GCC 16.1.0. No se ha logrado un binario sin esas instrucciones: las tres variantes de `libgcc.a` instaladas las contienen. Añadir `-march=armv8-a` al proyecto no recompilaría esa biblioteca.

La evidencia de flujo es más fuerte que en M004: las 18 instrucciones de `__arm_tpidr2_save` están tras una lectura de `__aarch64_have_sme`, constante de solo lectura con valor **0** en el ELF y el NRO. Las cinco `cntd` restantes del desenrollador dependen de procesar el registro DWARF 46 (`VG`); el volcado completo de `.eh_frame` contiene una sola regla para `r46`, dentro del FDE de `__arm_tpidr2_save`, y empieza **después** de su `cntd` en una ruta que ya quedó bloqueada por la misma constante. `frame_dummy` registra ese `.eh_frame` estático. Este resultado permite clasificar las 23 instrucciones como **sin ruta ejecutable identificada en el binario y sus tablas estáticas actuales**, incluso al contemplar el salto indirecto de `execute_stack_op` y las llamadas al desenrollador. No equivale a una prueba de ejecución: no se ha demostrado la ausencia de registros CFI creados dinámicamente por un cambio futuro ni auditado formalmente todo el grafo de llamadas indirectas del programa.

**Puerta para hardware:** la auditoría ya justifica que estas 23 instrucciones no se ejecutan en las rutas normales cubiertas por el binario actual. Aun así, el criterio más fuerte de obtener un ejecutable sin instrucciones incompatibles **no se cumple**. Como el NRO conserva opcodes incompatibles y `fixup-arm64.py` aún transforma otras 15 instrucciones, recomendamos mantener el paquete de M004 bloqueado y preparar primero una biblioteca `libgcc` Armv8.0-A aislada. Si se decide probar antes en hardware, debe ser una decisión explícita basada en esta limitación, con el fixture sintético y un NRO nuevo; esta misión no autoriza la copia ni la ejecución.

## Causa y cadena de compilación

El `Makefile` fija `-march=armv8-a+crc+crypto -mtune=cortex-a57 -mtp=soft -fPIE` y `switch.specs`. `aarch64-none-elf-gcc -print-multi-lib` enumera `.` / `large` / `pic`; con esos flags, `-print-libgcc-file-name` selecciona `C:/devkitPro/devkitA64/lib/gcc/aarch64-none-elf/16.1.0/pic/libgcc.a`. En las tres variantes, el objeto `unwind-dw2.o` contiene seis `cntd` y `__arm_tpidr2_save.o` contiene 18 instrucciones SVE/SME. El enlazador conserva cinco de las seis `cntd` del primer objeto; la sexta queda eliminada por secciones. Las 23 restantes están en el segmento `PT_LOAD` con permiso de ejecución, dentro de `.text`.

El origen concreto se confirmó con `aarch64-none-elf-nm -A`, `readelf -SW` y el desensamblado de los miembros extraídos del archivo `pic/libgcc.a`. Los bytes son instrucciones originales de los objetos, no una lectura accidental de constantes. La dirección de NRO coincide con la VMA del ELF para estas 23 entradas; `elf2nro` solo cambió 38 bytes de cabecera entre `0x10` y `0x53`, y el segmento de código coincide byte por byte a partir de `0x80`.

### Inventario exacto de las 23 instrucciones

Todos los bytes se muestran en orden de palabra AArch64 tal como `objdump` los imprime; en el archivo little endian aparecen invertidos. La sección de salida es `.text` en los 23 casos. `uw` significa `unwind-dw2.o` de `pic/libgcc.a`; `save` significa `__arm_tpidr2_save.o` del mismo archivo. El símbolo de función coincide con la función indicada, y las direcciones son VMA del ELF y desplazamientos de texto del NRO.

| Dirección | Palabra | Instrucción | Función / símbolo | Sección de entrada; objeto |
|---|---|---|---|---|
| `af3994` | `04e0e3e0` | `cntd x0` | `execute_stack_op` | `.text.execute_stack_op`; uw |
| `af39a8` | `04e0e3e0` | `cntd x0` | `execute_stack_op` | `.text.execute_stack_op`; uw |
| `af3e38` | `04e0e3e0` | `cntd x0` | `uw_update_context_1` | `.text.uw_update_context_1`; uw |
| `af3e9c` | `04e0e3e0` | `cntd x0` | `uw_update_context_1` | `.text.uw_update_context_1`; uw |
| `af3fc0` | `04e0e3e0` | `cntd x0` | `uw_update_context` | `.text.uw_update_context`; uw |
| `af7b04` | `e1206200` | `str za[w15, 0], [x16]` | `__arm_tpidr2_save` | `.text`; save |
| `af7b08` | `e1206201` | `str za[w15, 1], [x16, #1, mul vl]` | `__arm_tpidr2_save` | `.text`; save |
| `af7b0c` | `e1206202` | `str za[w15, 2], [x16, #2, mul vl]` | `__arm_tpidr2_save` | `.text`; save |
| `af7b10` | `e1206203` | `str za[w15, 3], [x16, #3, mul vl]` | `__arm_tpidr2_save` | `.text`; save |
| `af7b14` | `e1206204` | `str za[w15, 4], [x16, #4, mul vl]` | `__arm_tpidr2_save` | `.text`; save |
| `af7b18` | `e1206205` | `str za[w15, 5], [x16, #5, mul vl]` | `__arm_tpidr2_save` | `.text`; save |
| `af7b1c` | `e1206206` | `str za[w15, 6], [x16, #6, mul vl]` | `__arm_tpidr2_save` | `.text`; save |
| `af7b20` | `e1206207` | `str za[w15, 7], [x16, #7, mul vl]` | `__arm_tpidr2_save` | `.text`; save |
| `af7b24` | `e1206208` | `str za[w15, 8], [x16, #8, mul vl]` | `__arm_tpidr2_save` | `.text`; save |
| `af7b28` | `e1206209` | `str za[w15, 9], [x16, #9, mul vl]` | `__arm_tpidr2_save` | `.text`; save |
| `af7b2c` | `e120620a` | `str za[w15, 10], [x16, #10, mul vl]` | `__arm_tpidr2_save` | `.text`; save |
| `af7b30` | `e120620b` | `str za[w15, 11], [x16, #11, mul vl]` | `__arm_tpidr2_save` | `.text`; save |
| `af7b34` | `e120620c` | `str za[w15, 12], [x16, #12, mul vl]` | `__arm_tpidr2_save` | `.text`; save |
| `af7b38` | `e120620d` | `str za[w15, 13], [x16, #13, mul vl]` | `__arm_tpidr2_save` | `.text`; save |
| `af7b3c` | `e120620e` | `str za[w15, 14], [x16, #14, mul vl]` | `__arm_tpidr2_save` | `.text`; save |
| `af7b40` | `e120620f` | `str za[w15, 15], [x16, #15, mul vl]` | `__arm_tpidr2_save` | `.text`; save |
| `af7b48` | `04305a10` | `addsvl x16, x16, #16` | `__arm_tpidr2_save` | `.text`; save |
| `af7b60` | `04e0e3f0` | `cntd x16` | `__arm_tpidr2_save` | `.text`; save |

## Alcanzabilidad y otras extensiones

- El único `bl` directo a `__arm_tpidr2_save` visto en el desensamblado está en `__arm_za_disable`; **ambas** funciones leen `__aarch64_have_sme` y saltan fuera antes de llegar a SME cuando vale cero. La constante está en `.rodata`, símbolo `GLOBAL HIDDEN` de un byte (`0xe34a10`), sin modificación identificada; valor `00` en ELF y NRO. La comprobación no depende únicamente de buscar llamadas directas.
- Las `cntd` del desenrollador están en funciones utilizadas por excepciones. Sus bifurcaciones comprueban `0x2e` = 46 (`VG`). `execute_stack_op` también contiene una tabla de saltos; se revisó por tanto la entrada CFI que decide ese registro. `readelf -wf` produjo un único `DW_CFA_offset: r46 (vg)`, para FDE `pc=af7ad0..af7b70`; la regla se activa a `af7b68`, después del `cntd` en `af7b60` y del guardia SME. No aparecieron reglas `DW_CFA_expression` ni `DW_CFA_val_expression`. `frame_dummy` pasa `__EH_FRAME_BEGIN__` a `__register_frame_info`; Ruby se construyó sin JIT/YJIT y con `dlopen` deshabilitado. Esto cubre las tablas de desenrollado estáticas presentes, no un CFI que una versión futura pudiera registrar por otra vía.
- La búsqueda dirigida en todo el desensamblado halló, además de las 23, `bti c` ×14, `xpaclri` ×6 y `autia1716`/`autib1716` ×1 cada uno. Estas 22 codificaciones pertenecen al espacio HINT y se comportan como NOP en Armv8.0-A; no se contaron como incompatibles. No se encontraron mnemónicos LSE (`cas`, `ldadd`, `swp`, `ldapr`), MTE, `smstart`, `smstop`, GCS o lecturas de `tpidr2` tras el postproceso. El barrido por mnemónicos no sustituye un verificador formal de todos los opcodes. Los flags `+crc+crypto` siguen el [Makefile de libnx](https://github.com/switchbrew/libnx/blob/master/nx/Makefile); la presencia de esas extensiones en la consola concreta no se ha medido aquí.
- Arm describe [Cortex-A57 como Armv8.0-A](https://developer.arm.com/-/media/Arm%20Developer%20Community/PDF/Learn%20the%20Architecture/Understanding%20the%20Armv8.x%20extensions.pdf), [SVE como extensión posterior](https://developer.arm.com/-/media/Arm%20Developer%20Community/PDF/Learn%20the%20Architecture/102131_0100_01_SVE_and_Neon_coding_compared.pdf?revision=feaaf72e-a941-461c-bd92-0d960d0f8615) y [STR ZA como SME](https://developer.arm.com/community/arm-community-blogs/b/architectures-and-processors-blog/posts/arm-scalable-matrix-extension-introduction-p2). Arm también documenta la [compatibilidad de PAC y BTI codificados como HINT](https://developer.arm.com/community/arm-community-blogs/b/tools-software-ides-blog/posts/code-reuse-attacks-the-compiler-story?CommentId=c2c6d455-3b74-4dea-b7f3-bb49fad77076%3FCommentId%3Dc2c6d455-3b74-4dea-b7f3-bb49fad77076). La explicación de los ayudantes SME está en [GCC](https://gcc.gnu.org/pipermail/gcc-cvs/2023-December/394915.html).

## Las 15 sustituciones de `fixup-arm64.py`

El enlace original se posprocesa en sitio; M004 comparó byte a byte el ELF antes y después y registró las 15 VMA en `log-m004-auditoria-estatica.txt`. La compilación limpia de M005 volvió a informar exactamente cinco `chkfeat x16`, dos `mrs gcspr_el0`, tres `gcspopm`, dos `mrs tpidr2_el0`, un `msr tpidr2_el0` y dos `smstart/smstop`. No se cambió el guion ni se añadieron reglas para las 23 instrucciones. `chkfeat` es HINT en A57 y los bloques GCS se saltan; las modificaciones SME están tras el guardia de cero. Por ello las sustituciones son equivalentes **para las rutas Armv8.0-A identificadas**, no para una CPU con GCS/SME. La nota GNU Build ID se calculó antes de editar instrucciones y no representa el contenido final; para identificar el NRO se usa SHA-256.

## Compilación, artefactos y fixture

Antes de compilar se conservaron `build/`, ELF, NRO y NACP anteriores con sufijo `.m005-base` dentro de `mkxp-z-nx/`. Se ejecutó `make -C mkxp-z-nx -j6` en MSYS2 de devkitPro con devkitA64 en `PATH`; código **0**. El primer intento de invocar `bash -c` sin su entorno de login no encontró `make`; se repitió con `bash -lc`. [Log completo de compilación](log-m005-compilacion.txt) y [verificación estática](log-m005-verificacion.txt). Se reutilizaron Ruby, nxcompat y portlibs ya preparados en M003; es una compilación limpia **del motor**, no una reconstrucción completa de todos los componentes desde fuentes. No se instalaron herramientas.

| Artefacto local, ignorado por Git | Bytes | SHA-256 |
|---|---:|---|
| `mkxp-z-nx/rgss-switch.elf` | 88 867 944 | `4fb1a54a2906d17966821401c63174756a5ef12f74900926d8a2f1ecfab25172` |
| `mkxp-z-nx/rgss-switch.nro` | 17 750 457 | `c97902c7d89f8fe94aefb255008ea99bec127f06d25278d5c7ddfa5d1ac878b5` |
| `mkxp-z-nx/rgss-switch.nacp` | 16 384 | `80bf8703ca7a305b8d5aaaa33ca212094b8b0c0bfda29349a2ef28f492eb095e` |

`readelf -h` informa ELF64 AArch64 `DYN` (PIE); `nm -u` no devuelve símbolos indefinidos. `comprobar-nro.py` confirmó `NRO0`, `ASET v0`, icono de 12 993 bytes, NACP de 16 384, RomFS de 1 728 y rangos válidos. El RomFS solo lleva `LEEME.txt`. El fixture sintético de M004 sigue con `mkxp.json` junto al script propio `probe.rb`, `Data/Scripts.rxdata` vacío, `Graphics/`, `Audio/` y `lib/`; su JSON se analizó y los cuatro archivos fuente coinciden con el paquete local por SHA-256. **El paquete M004 contiene el NRO anterior** (`9bec…496`) y sigue bloqueado; no se ha sustituido por este NRO nuevo. El contrato de arranque del código no cambió en esta rama.

## Alternativas y siguiente paso

1. **Recomendado:** construir en un directorio aislado una variante de `libgcc` compatible con Armv8.0-A, con los mismos ABI y versión de GCC que el SDK, enlazarla solo en el proyecto y auditar de nuevo ELF/NRO, excepciones y CFI. Hay que evaluar si la compilación de `libgcc` elimina el código SME o solo lo conserva protegido. Esto supone trabajo profundo de toolchain: presentar diseño y riesgos y obtener autorización antes de ejecutarlo. No reemplazar `C:/devkitPro`.
2. Usar una versión aislada de devkitA64 anterior a la inclusión de SME, reconstruyendo y comprobando todas las dependencias vinculadas. Mayor riesgo de mezcla de ABI y pérdida de correcciones; no cambiar el SDK global.
3. Mantener el binario actual y aceptar la prueba controlada solo por el argumento de guardias/CFI. Es razonable como **hipótesis de ejecución en hardware**, pero conserva las 23 instrucciones y puede seguir fallando en traductores que analicen ramas muertas. Requiere autorización expresa para microSD y ejecución; no satisface la preferencia por un binario limpio.
4. Extender `fixup-arm64.py` con NOP para `cntd`/`str za`/`addsvl`: descartado. Alteraría semántica de desenrollado y guardado SME; silenciar opcodes no demuestra corrección.

**Estado:** FNX-003 conserva compilación reproducible del motor, sin validación en Switch. FNX-004 sigue sin prueba de hardware. FNX-015 identifica la causa y las 23 rutas estáticas, pero no elimina instrucciones del ejecutable; la compatibilidad real sigue pendiente de un ensayo controlado o de una `libgcc` aislada sin ellas. No se fusionó con `main` ni se publicó release.
