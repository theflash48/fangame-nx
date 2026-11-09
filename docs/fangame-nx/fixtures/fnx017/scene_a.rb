# SPDX-License-Identifier: GPL-2.0-or-later
# Fixture original A: azul, barra ambar y archivos propios.
carpeta = "fixtures/a"
log = File.open("#{carpeta}/events.log", "a")
log.sync = true
bitmap = nil
sprite = nil

begin
  log.puts("=== FNX017 A INICIO ===")
  esperado = "FNX017-A-LECTURA-OK\n"
  lectura_ok = File.binread("#{carpeta}/entrada.txt") == esperado
  log.puts("lectura=#{lectura_ok ? 'OK' : 'ERROR'}")

  marca = "FNX017-A-ESCRITURA-OK\n"
  File.open("#{carpeta}/escritura.txt", "ab") { |f| f.write(marca) }
  escritura_ok = File.binread("#{carpeta}/escritura.txt").end_with?(marca)
  log.puts("escritura=#{escritura_ok ? 'OK' : 'ERROR'}")

  bitmap = Bitmap.new(640, 480)
  sprite = Sprite.new
  sprite.bitmap = bitmap
  contador = 0
  alterno = false
  saliendo = false

  dibujar = lambda do
    bitmap.fill_rect(0, 0, 640, 480, alterno ? Color.new(5, 88, 92) : Color.new(12, 35, 80))
    bitmap.fill_rect(48, 36, 544, 10, Color.new(255, 180, 0))
    bitmap.fill_rect(48, 432, 544, 10, Color.new(255, 180, 0))
    bitmap.font.color = Color.new(255, 255, 255)
    bitmap.draw_text(48, 65, 544, 46, "FANGAMENX - FIXTURE A", 1)
    bitmap.draw_text(48, 130, 544, 46, lectura_ok ? "LECTURA A OK" : "LECTURA A ERROR", 1)
    bitmap.draw_text(48, 190, 544, 46, "CONTADOR A: #{contador}", 1)
    bitmap.draw_text(48, 250, 544, 46, alterno ? "FONDO: TURQUESA" : "FONDO: AZUL", 1)
    bitmap.draw_text(48, 315, 544, 46, "A: +1   X: COLOR   B: SALIR", 1)
    bitmap.draw_text(48, 372, 544, 40, saliendo ? "SALIENDO A..." : "ESCRITURA #{escritura_ok ? 'OK' : 'ERROR'}", 1)
  end

  dibujar.call
  loop do
    Input.update
    if Input.trigger?(Input::C)
      contador += 1
      log.puts("boton=A contador=#{contador}")
      dibujar.call
    end
    if Input.trigger?(Input::A)
      alterno = !alterno
      log.puts("boton=X color=#{alterno ? 'turquesa' : 'azul'}")
      dibujar.call
    end
    if Input.trigger?(Input::B)
      log.puts("boton=B salida_solicitada")
      saliendo = true
      dibujar.call
      Graphics.update
      break
    end
    Graphics.update
  end
  log.puts("FNX017 A RUBY COMPLETADO")
rescue Exception => e
  log.puts("FNX017 A ERROR #{e.class}: #{e.message}")
  raise
ensure
  sprite.dispose if sprite && !sprite.disposed?
  bitmap.dispose if bitmap && !bitmap.disposed?
  log.close
end
