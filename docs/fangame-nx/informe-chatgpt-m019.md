# Informe para ChatGPT · FangameNX M019

Fecha: 2026-11-10. Rama: `feature/fangamenx-launcher-mvp`. Base: `56cf8f1`; la implementación M019 está en el commit que contiene este informe (`git log -1`). Sin `push`.

## Resultado y evidencia física previa

El propietario confirmó que **Pokémon Ópalo 2.11 funciona en Switch 1 con b27** y observó una velocidad excesiva. También confirmó que **B abre el menú interno de Ópalo**. Esta misión no volvió a probar físicamente esos hechos. La versión nueva **0.2.4 b32** está instalada y verificada por lectura MTP, pero su ejecución, interfaz y salida en consola siguen pendientes de prueba del propietario.

## Causa y control de velocidad

La configuración base y los dos juegos instalados declaraban `syncToRefreshrate: true`. En mkxp-z ese ajuste fija `Graphics.frame_rate` al refresco del panel y **desactiva `FPSLimiter`**. Con un panel de 60 Hz, RGSS1 arranca así por encima de su cadencia nominal de 40 Hz. `vsync: true` únicamente sincroniza `SDL_GL_SwapWindow`; no impide que la lógica Ruby avance con una frecuencia indebida. Es una causa demostrada por código, coherente con la observación del propietario; la frecuencia efectiva anterior no se midió en la Switch.

El launcher b32 fuerza `syncToRefreshrate=false` al iniciar un juego, sin editar el `mkxp.json` ya instalado. Los paquetes futuros llevan ese valor en `runtime/mkxp.base.json`. Se conserva `vsync` tal como lo declara cada juego; no se modificó Pokémon Z. El limitador usa `SDL_GetPerformanceCounter`, plazos absolutos, espera con `nanosleep`/SDL y reinicio de fase tras retrasos grandes o pausas. No ejecuta ráfagas de ticks para recuperar tiempo. La cadencia sale de `Graphics.frame_rate` (40 Hz iniciales en RGSS1, modificables por script) o de `fixedFramerate` si el perfil lo fija. El render puede seguir condicionado por VSync y por la carga de la GPU; el menú distingue FPS medidos, Hz de simulación y multiplicador.

El menú ofrece **Automático, 0,5×, 1×, 1,5× y 2×**. Automático y 1× usan hoy la misma cadencia nominal; Automático es el valor predeterminado. La preferencia se guarda por ID como `speed_<id>` en `switch/FangameNX/config/launcher.json`, con escritura temporal, respaldo y recuperación ya usados por el launcher. Los scripts, recursos y partidas de los juegos no se modifican.

## Menú rápido y salida

**L+R+PLUS durante 0,8 s** abre el menú nativo solo durante un juego; PLUS conserva su función en la biblioteca. El hilo de eventos detecta la combinación con reloj monotónico, consume PLUS y el segundo hombro, limpia el estado de entrada al abrir y consume la navegación mientras el menú está activo. El primer hombro puede haber llegado al juego antes de completarse la combinación: es un límite conocido del atajo. Una pulsación corta no abre el menú y la misma pulsación mantenida no lo reabre repetidamente. B no es una salida universal.

El menú se dibuja sobre el último fotograma en el hilo RGSS, dueño del contexto GL: fondo oscuro translúcido, selección turquesa, texto grande. Ofrece Reanudar, Velocidad, Diagnóstico y Volver a la biblioteca. Diagnóstico muestra juego, build, Hz nominal/efectivos, FPS medidos y estado real de VSync. No se ofrece un interruptor de VSync porque no se verificó que cambiarlo en caliente sea seguro en esta plataforma.

Mientras está abierto, Ruby no avanza. El reloj monotónico compartido del juego se congela y descuenta el tiempo de pausa al reanudar; el limitador reinicia su plazo. Si OpenAL expone `ALC_SOFT_pause_device`, el audio también se pausa y reanuda. La salida a la biblioteca exige confirmación por posible progreso sin guardar. Tras confirmar, se solicita `SDL_QUIT` al hilo de eventos y se usa el contrato existente `rqTerm`/`rqTermAck` y `scriptBinding->terminate`; la limpieza Ruby, GL y SDL sigue en sus hilos y orden existentes. No se simula B ni se mata el proceso. El atajo requiere que el bucle gráfico siga avanzando; no resuelve un bloqueo total de Ruby fuera de `Graphics.update`/dibujo.

## Compilación, paquete y despliegue

Compilación incremental con devkitA64 y `FNX_LIBGCC_PREFIX=src/build-logs/fnx016-isolated-prefix`, NRO b32 de **17 829 065 bytes**, SHA-256 `8763807865a3a2c1008decc12f1f00be2c4411b94495114ca2476afcfb40f2a3`. Auditoría ARMv8: cero instrucciones incompatibles conocidas, cero símbolos indefinidos, cero `<unknown>`/patrones ARMv9 buscados y **11 783 928 bytes** de código ELF/NRO idénticos. `reconstruir-src.py --orden`: 40 archivos reconstruidos idénticos, cero desvíos y cero archivos sin parche.

Paquete privado `local/m013/package-8763807865/`: 25/25 archivos verificados, manifiesto SHA-256 `d97ceae938eebe526629f9100ff0ebdc578dd5c2f288c51eb53bbe44b1f76d28`. Se conservaron b27 y las compilaciones intermedias b30/b31 en `local/`. MTP mostró una sola Nintendo Switch portátil; la disponibilidad de MTP indicó que el launcher no estaba en primer plano, aunque no expone una lista de procesos para certificarlo. Se respaldó cada NRO activo antes de sustituirlo y se leyó de vuelta el temporal y el definitivo. El archivo activo `SD Card/switch/FangameNX/FangameNX.nro` devolvió el SHA-256 b32 exacto. Evidencia privada: `local/m019/mtp-update-b32.log`, `old-nro-b31/`, `stage-readback-b32/` y `final-readback-b32/`. No se tocaron las carpetas, configuraciones ni guardados de Pokémon Z y Ópalo.

## Pruebas y siguientes pasos

`tests/run-launcher-host.cmd local/m013/package-fe34daaf94` pasó: escáner, preferencias existentes, pulsación corta/prolongada, supresión del atajo, comandos de navegación, velocidad individual y cadencia con retraso/pausa. `python tests/test_prepare_launcher_game.py` pasó tras cambiar la base. `git diff --check` y la reconstrucción del parche pasaron. No hay prueba visual automatizada del overlay ni prueba física de b32.

Próxima prueba de mayor valor: abrir b32 en la Switch, iniciar Ópalo y Z, comparar la velocidad normal con la observada en b27, mantener L+R+PLUS, cambiar velocidad, reanudar, comprobar B dentro de Ópalo y volver a biblioteca tras la confirmación. Registrar si el panel se dibuja bien y si el audio se pausa. Continúan abiertas la X intermitente, el pánico anterior tras interacción táctil, guardado/carga, sesiones largas y compatibilidad de la salida con scripts que no atiendan el bucle gráfico.
