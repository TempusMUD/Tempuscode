#!/usr/bin/ruby

require_relative 'tempusrooms'

$change_count = 0
$skip_count = 0

def process_file(path)
  begin
    File.open(path, "r") { |inf|
      $linenum = 0
      until inf.eof?
        new_room = Room.new
        new_room.check if new_room.load(inf)
      end
    }

  rescue RuntimeError => exc
    print exc, " at  ", path, ":", $linenum, "\n"
    print exc.backtrace.join("\n")
  end
end

ARGV.each { |path|
  STDERR.print "...checking room file #{path}\n" if ARGV.length > 1
  process_file(path)
}
