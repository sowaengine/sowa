#!/usr/bin/ruby

require 'yaml'

#  https://github.com/Lexographics/bully

#  MIT License
#  
#  Copyright (c) 2021-2023 Lexographics
#  
#  Permission is hereby granted, free of charge, to any person obtaining a copy
#  of this software and associated documentation files (the "Software"), to deal
#  in the Software without restriction, including without limitation the rights
#  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
#  copies of the Software, and to permit persons to whom the Software is
#  furnished to do so, subject to the following conditions:
#  
#  The above copyright notice and this permission notice shall be included in all
#  copies or substantial portions of the Software.
#  
#  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
#  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
#  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
#  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
#  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
#  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
#  SOFTWARE.

$version = "2"

class Config
  attr_accessor :extension
  attr_accessor :namespace
  attr_accessor :dir

  def version
    puts "Bully build generator version #{$version}"
  end

  def usage
    version
    puts "Usage:"
    puts "  ruby bully.rb   --help, -h                prints usage"
    puts "  ruby bully.rb   --version                 prints version information"
    puts "  ruby bully.rb   --ext {extension}         extension for generated files (default: h)"
    puts "  ruby bully.rb   --namespace {namespace}   namespace for generated files (default: )"
    puts "  ruby bully.rb   --dir {directory}         directory to lookup resources (default: ./)"
    
    exit
  end

  def load_config
    return if not File.exists?("./bully_config.yml")
    data = YAML.load_file("./bully_config.yml")

    if data["extension"]
      @extension = data["extension"]
    end
    if data["namespace"]
      @namespace = data["namespace"]
    end
    if data["dir"]
      @dir = data["dir"]
    end
  end

  def initialize
    @extension = "h"
    @namespace = ""
    @dir = "./"

    load_config

    ARGV.each_with_index do |arg, index|
      case arg
      when "--help", "-h"
        usage
      when "--version"
        version; exit
      when "--ext"
        @extension = ARGV[index + 1]
      when "--namespace"
        @namespace = ARGV[index + 1]
      when "--dir"
        @dir = ARGV[index + 1]
      end
    end
  end
end

$cfg = Config.new()

def main
  if not File.directory?($cfg.dir)
    puts "Invalid data directory!"
    exit 1
  end

  visit_dir $cfg.dir
end

def visit_dir dir
  Dir.foreach(dir) do |filename|
    next if filename == '.' or filename == '..'
    next if ["." + $cfg.extension].include? File.extname filename

    path = File.join(dir, filename)

    if File.directory?(path)
      visit_dir path
      next
    end
  
    if File.file?(path)
      convert_file path
    end
  end
end


def convert_file(filename)
  out_filename = filename + "." + $cfg.extension
  header_guard = out_filename.tr(".", "_").tr("*", "_").tr("-", "_").tr("/", "_") # nice
  var_name = header_guard + "_data"

  puts "Generating #{out_filename}"

  data = nil
  File.open(filename, "rb") { |file|
    data = file.read
  }
  data.force_encoding("ASCII-8BIT")
  bytes = data.unpack("C*")
  
  File.open(out_filename, "w") { |file|
    file.write "#ifndef #{header_guard}\n"
    file.write "#define #{header_guard}\n"
    file.write "#pragma once\n"

    if $cfg.namespace != ""
      file.write "\nnamespace #{$cfg.namespace} {\n"
    end

    
    file.write "static unsigned char #{var_name}[#{bytes.size}] = {"
    bytes.each_with_index { |char, index|
      if index % 8 == 0
        file.write "\n\t"
      end

      file.write("#{" " * (3 - char.to_s.size)}#{char}, ")
    }
    file.write "\n};\n"

    if $cfg.namespace != ""
      file.write "} // namespace #{$cfg.namespace}\n"
    end

    file.write "\n"
    file.write "#endif // #{header_guard}\n"
  }
end


main