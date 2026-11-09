# SPDX-License-Identifier: GPL-2.0-or-later
# Arnes sintetico: un solo NRO, A y B en procesos consecutivos.
seleccion = File.binread("siguiente.txt").strip
raise "seleccion invalida: #{seleccion.inspect}" unless ["A", "B"].include?(seleccion)

numero = if File.exist?("fnx017-selector.log")
  File.readlines("fnx017-selector.log").count { |linea| linea.start_with?("=== FNX017 RUN ") } + 1
else
  1
end

log = File.open("fnx017-selector.log", "a")
log.sync = true
log.puts("=== FNX017 RUN #{numero} SELECT #{seleccion} ===")
log.puts("pwd=#{Dir.pwd}")

begin
  load "./fixtures/#{seleccion.downcase}/scene.rb"
  proxima = seleccion == "A" ? "B" : "A"
  File.open("siguiente.txt", "wb") { |archivo| archivo.write("#{proxima}\n") }
  log.puts("FNX017 RUN #{numero} COMPLETADO; siguiente=#{proxima}")
rescue Exception => e
  log.puts("FNX017 RUN #{numero} ERROR #{e.class}: #{e.message}")
  raise
ensure
  log.close
end
