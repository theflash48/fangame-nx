# Informe para ChatGPT · FangameNX M016 · Pokémon Z y portadas

Fecha: 2026-11-10. Rama: `feature/fangamenx-launcher-mvp`. Instalación de
Pokémon Z 2.18 documentada en los commits `5ddbe4c`, `7232606`, `6f44087` y
`4a182bd`; automatización de portadas en `a85032a`. Todo quedó en Git local,
sin `push`. El informe técnico detallado es
[`informe-chatgpt-m016-pokemon-z.md`](informe-chatgpt-m016-pokemon-z.md).

## Resultado ejecutivo

**Pokémon Z 2.18 está instalado en la biblioteca FangameNX de la Nintendo
Switch y el propietario confirmó que funciona.** Lo comunicó primero con
«funciona» y después con «va de puta madre». Es una observación del usuario
en hardware, no una conclusión deducida del escáner o de los logs. No se han
registrado aún hitos separados de audio, controles, mapa, guardado/carga,
salida ni sesiones largas. La portada propia se copió y se vinculó a la ficha;
su visualización en la interfaz de Switch no ha sido confirmada expresamente.

Para las **próximas instalaciones preparadas**, el alta local mediante
`packaging/install-launcher-game.py` detecta una imagen de título o logo del
juego, la copia como `cover.png` y declara la portada en `fangamenx.json`.
Esa salida funciona con el NRO b25 ya instalado. El instalador no convierte
por sí solo una carpeta cruda de RPG Maker XP en una instalación ejecutable.

## Causa y corrección

No se identificó un fallo nuevo del motor que exigiera cambiar el NRO. El
juego requería preparar sus recursos y bibliotecas para el contrato actual
del lanzador: `datos.zip` sin compresión interna para Data/Graphics/Audio/Fonts,
`Game.rgssad` de ocho bytes, `mkxp.json`, `lib/` y `fangamenx.json`. La primera
copia MTP terminó antes de completar todos los ficheros pequeños; se rellenó
el inventario y se verificó por lectura de vuelta. La subcarpeta
`lib/benchmark`, creada en esta instalación, tuvo que recrearse tras comprobar
su contenido exacto. No se alteraron otros juegos ni sus guardados.

La portada de Pokémon Z salió de `Graphics/Pictures/titulo.png` dentro del
contenedor del propio juego. Se copió sin transformación a `cover.png` y el
manifiesto declaró `"cover": "cover.png"`. El lanzador b25 necesitaba ese
campo: la presencia del archivo por sí sola no bastaba. El cambio de código
posterior permite detectar `cover.png`, `cover.jpg` o `cover.jpeg` en la raíz
sin ese campo, pero **ese cambio aún no está compilado ni instalado en la
Switch**. El instalador nuevo sí escribe el campo requerido por b25.

## Build, paquete y evidencias

- Fuente autorizada: `D:\Descargas\POKEMON Z V2.18.zip`, 826 877 555 bytes,
  SHA-256 `f642df4dcdaf5e4e9746a82297dd8d9bd32ffc5f48f1ea5756a16d8f0f55f133`.
  Sus 19 118 entradas pasaron la comprobación ZIP y los controles de rutas,
  duplicados, cifrado y enlaces. El original permaneció intacto.
- `datos.zip`: 19 022 recursos, método ZIP `STORED`, 888 144 816 bytes,
  SHA-256 `0608df2428808edce149068f043bf2d6f88eeabafabac8ba1d04269adcd07a52`.
  Las 255 secciones Ruby se comprobaron sintácticamente: 3 transformadas
  por `NXFuente`, 0 errores de compilación. Esto no acredita todas las rutas
  de juego.
- NRO b25 activo: SHA-256
  `bbf9d4fee998703903f10694c04b91a1e5dd01c0769e09cba79c119bd3b5e299`,
  leído por MTP antes de la instalación y de la portada. No se recompiló ni
  sustituyó. Los 114 archivos de la instalación inicial se verificaron por
  SHA-256 tras leerlos de vuelta. Después se comprobaron por SHA-256 el nuevo
  PNG y `fangamenx.json`; el inventario MTP final encontró los 115 nombres
  previstos. Se conservaron
  cuatro ficheros generados al ejecutar (`mkxp-boot.log`, `mkxp-debug.log`,
  `mkxp-ruby.log` y `LastSave.dat`); su mera presencia no prueba guardado/carga.
- Portada instalada: PNG 512×384, SHA-256
  `d9dcf8c08ad8d19014cc49dd6f633fb8d3ac79c168f06299cb862f9f07d20459`.
  El escáner host devolvió `ready=1` y la ruta de portada. La lectura MTP
  confirmó el PNG y el manifiesto actualizado.
- Automatización posterior: `python -m py_compile` pasó; el selector halló
  `Graphics/Pictures/titulo.png` en el `datos.zip` privado; una instalación
  local de prueba creó `cover.png` y el campo del manifiesto con el SHA
  esperado. Un ZIP con dos candidatas de igual prioridad quedó sin selección.
  `tests/run-launcher-host.cmd local/m013/package-bbf9d4fee9` terminó en
  `launcher_host: OK`, incluida la prioridad de portada explícita. Detalles
  en [`portadas-automaticas.md`](portadas-automaticas.md).

La preparación, los recursos del juego, la imagen y las copias de lectura
están en `local/m016/`, excluido de Git. No se incorporaron scripts, recursos
ni distribución de Pokémon Z al repositorio.

## Alcance y siguiente tarea

La confirmación física acredita el funcionamiento general comunicado por el
propietario. Si se quiere cerrar compatibilidad funcional más amplia, registrar
una sesión con mapa, sonido, mando, guardado de partida nueva, carga tras
reiniciar y salida; comprobar también si la portada aparece en la ficha de la
Switch. No deducir esos hitos de `LastSave.dat` ni del preflight `ready=1`.

Para nuevos juegos, usar el instalador sobre una **carpeta ya preparada** y
verificar el resultado antes de copiarlo por MTP. Si un juego compone su
título con varias capas o no tiene una imagen candidata única, conservar la
portada geométrica hasta elegir un recurso adecuado. La detección directa de
`cover.png` sin manifiesto necesitará construir y probar otro NRO antes de
llevarla a la consola. La importación de juegos crudos y las actualizaciones
con conservación automática de partidas quedan fuera de esta misión.
