#!/usr/bin/ruby

require 'tempusmobs'

$change_count = 0
$skip_count = 0

def process_file(path)
  begin
    File.open(path, "r") { |inf|
      $linenum = 0
      until inf.eof?
        new_mob = Mobile.new
        new_mob.check if new_mob.load(inf)
      end
    }

  rescue RuntimeError => exc
    print exc, " at  ", path, ":", $linenum, "\n"
    print exc.backtrace.join("\n")
  end
end

ARGV.each { |path|
  STDERR.print "...checking mob file #{path}\n" if ARGV.length > 1
  process_file(path)
}
