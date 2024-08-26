#!/usr/bin/ruby

require_relative 'tempusobjs'

def process_file(path)
  begin
    objs = read_objects(path)
    objs.each {|obj|
      obj.check
    }
  rescue RuntimeError => exc
    print exc, " at  ", path, ":", $linenum, "\n"
    print exc.backtrace.join("\n")
  end
end

ARGV.each { |path| process_file(path) }
