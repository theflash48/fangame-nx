# SPDX-License-Identifier: GPL-2.0-or-later
# Host-only flow check. Each scene is run in a fresh Ruby subprocess.
require "fileutils"
require "open3"
require "rbconfig"

if ARGV[0] == "--once"
  class Color
    def initialize(*); end
  end
  class Bitmap
    def initialize(*); end
    def fill_rect(*); end
    def font; @font ||= Struct.new(:color).new; end
    def draw_text(*args); ($texts ||= []) << args[4].to_s; end
    def dispose; @disposed = true; end
    def disposed?; !!@disposed; end
  end
  class Sprite
    attr_accessor :bitmap
    def dispose; @disposed = true; end
    def disposed?; !!@disposed; end
  end
  module Graphics
    def self.update; end
  end
  module Input
    C = :c
    A = :a
    B = :b
    def self.update; @tick = (@tick || 0) + 1; end
    def self.trigger?(key); {1 => C, 2 => A, 3 => B}[@tick] == key; end
  end
  Dir.chdir(ARGV.fetch(1)) { load "./selector.rb" }
  puts(($texts || []).grep(/FIXTURE|LECTURA|CONTADOR|SALIENDO/).join(" | "))
  exit
end

source = File.expand_path(ARGV.fetch(0))
target = File.expand_path(ARGV.fetch(1))
raise "target already exists: #{target}" if File.exist?(target)
FileUtils.mkdir_p(File.dirname(target))
FileUtils.cp_r(source, target)

["A", "B"].each do |letter|
  stdout, stderr, status = Open3.capture3(RbConfig.ruby, __FILE__, "--once", target)
  raise "host run #{letter} failed: #{stderr}" unless status.success?
  ["FIXTURE #{letter}", "LECTURA #{letter} OK", "CONTADOR #{letter}: 1", "SALIENDO #{letter}"].each do |expected|
    raise "host run #{letter} missing #{expected}: #{stdout}" unless stdout.include?(expected)
  end
  puts "HOST_#{letter}_OK #{stdout.strip}"
end

raise "selection did not cycle" unless File.binread(File.join(target, "siguiente.txt")) == "A\n"
selector = File.read(File.join(target, "fnx017-selector.log"))
raise "selection log mismatch" unless selector.include?("RUN 1 SELECT A") && selector.include?("RUN 2 SELECT B")
%w[a b].each do |letter|
  base = File.join(target, "fixtures", letter)
  events = File.read(File.join(base, "events.log"))
  written = File.binread(File.join(base, "escritura.txt"))
  upper = letter.upcase
  raise "event log mismatch #{upper}" unless events.include?("lectura=OK") && events.include?("escritura=OK") && events.include?("boton=A contador=1") && events.include?("boton=X color=") && events.include?("FNX017 #{upper} RUBY COMPLETADO")
  raise "write mismatch #{upper}" unless written == "FNX017-#{upper}-ESCRITURA-OK\n"
end
puts "HOST_DUAL_OK"
