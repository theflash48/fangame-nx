# SPDX-License-Identifier: GPL-2.0-or-later
# Caso sintetico propio: entrada, botones RGSS1, dibujo y salida desde Ruby.
log = File.open("fnx009-probe.log", "a")
log.sync = true
bitmap = nil
sprite = nil

begin
  log.puts("=== FNX009 INICIO ===")
  log.puts("pwd=#{Dir.pwd}")

  esperado = "FNX009-LECTURA-OK\n"
  begin
    recibido = File.binread("entrada.txt")
    lectura_ok = (recibido == esperado)
    log.puts("lectura=#{lectura_ok ? 'OK' : 'ERROR contenido inesperado'}")
  rescue StandardError => e
    lectura_ok = false
    log.puts("lectura=ERROR #{e.class}: #{e.message}")
  end

  marca = "FNX009-ESCRITURA-OK\n"
  File.open("fnx009-escritura.txt", "ab") { |archivo| archivo.write(marca) }
  escritura_ok = File.binread("fnx009-escritura.txt").end_with?(marca)
  log.puts("escritura=#{escritura_ok ? 'OK' : 'ERROR relectura'}")

  bitmap = Bitmap.new(640, 480)
  sprite = Sprite.new
  sprite.bitmap = bitmap
  contador = 0
  color_alterno = false
  saliendo = false

  dibujar = lambda do
    fondo = color_alterno ? Color.new(0, 85, 75) : Color.new(12, 28, 64)
    bitmap.fill_rect(0, 0, 640, 480, fondo)
    bitmap.fill_rect(48, 36, 544, 8, Color.new(255, 0, 180))
    bitmap.fill_rect(48, 432, 544, 8, Color.new(0, 255, 120))
    bitmap.font.color = Color.new(255, 255, 255)
    bitmap.draw_text(48, 58, 544, 48, "FangameNX - FNX-009", 1)
    bitmap.draw_text(48, 128, 544, 48, lectura_ok ? "LECTURA OK" : "LECTURA ERROR", 1)
    bitmap.draw_text(48, 190, 544, 48, "CONTADOR: #{contador}", 1)
    bitmap.draw_text(48, 245, 544, 48, color_alterno ? "COLOR: VERDE" : "COLOR: AZUL", 1)
    bitmap.draw_text(48, 315, 544, 48, "A: +1   X: COLOR   B: SALIR", 1)
    bitmap.draw_text(48, 370, 544, 40, saliendo ? "SALIENDO..." : "ESCRITURA #{escritura_ok ? 'OK' : 'ERROR'}", 1)
  end

  dibujar.call
  log.puts("pantalla=inicial")

  loop do
    Input.update
    if Input.trigger?(Input::C)
      contador += 1
      log.puts("boton=A contador=#{contador}")
      dibujar.call
    end
    if Input.trigger?(Input::A)
      color_alterno = !color_alterno
      log.puts("boton=X color=#{color_alterno ? 'verde' : 'azul'}")
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
  log.puts("FNX009 RUBY COMPLETADO; salida normal")
rescue Exception => e
  log.puts("FNX009 ERROR #{e.class}: #{e.message}")
  raise
ensure
  sprite.dispose if sprite && !sprite.disposed?
  bitmap.dispose if bitmap && !bitmap.disposed?
  log.close
end
