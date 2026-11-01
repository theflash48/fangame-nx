# Caso sintético propio. Se ejecuta mediante customScript, sin Scripts.rxdata.
log = File.open("fnx004-probe.log", "w")
log.sync = true
bitmap = nil
sprite = nil

begin
  log.puts("FNX004 RUBY INICIADO")
  log.puts("pwd=#{Dir.pwd}")

  entrada = File.binread("entrada.txt")
  raise "lectura inesperada" unless entrada == "FNX004-LECTURA-OK\n"
  log.puts("lectura=OK")

  File.write("fnx004-escritura.txt", "FNX004-ESCRITURA-OK\n")
  raise "escritura inesperada" unless File.binread("fnx004-escritura.txt") == "FNX004-ESCRITURA-OK\n"
  log.puts("escritura=OK")

  bitmap = Bitmap.new(640, 480)
  bitmap.fill_rect(0, 0, 640, 480, Color.new(12, 28, 64))
  bitmap.fill_rect(80, 100, 480, 80, Color.new(255, 0, 180))
  bitmap.fill_rect(80, 300, 480, 40, Color.new(0, 255, 120))
  sprite = Sprite.new
  sprite.bitmap = bitmap
  log.puts("pantalla=bandas magenta y verde")

  begin
    bitmap.font.color = Color.new(255, 255, 255)
    bitmap.draw_text(80, 210, 480, 60, "FNX-004 RUBY OK", 1)
    log.puts("texto=OK")
  rescue StandardError => e
    log.puts("texto=ERROR #{e.class}: #{e.message}")
  end

  480.times { Graphics.update }
  log.puts("FNX004 RUBY COMPLETADO; salida normal")
rescue Exception => e
  log.puts("FNX004 ERROR #{e.class}: #{e.message}")
  raise
ensure
  sprite.dispose if sprite && !sprite.disposed?
  bitmap.dispose if bitmap && !bitmap.disposed?
  log.close
end
