#!/usr/local/bin/ruby

require 'tempusrooms'

SCREEN_WIDTH = 70

$change_count = 0
$skip_count = 0

def check_desc(str, where)
    if str == ""
        print "#{where}: no desc\n"
    end

    linenum = 0
    str.each { |line|
        linenum += 1
        if line.length > SCREEN_WIDTH
            print "#{where}: Line #{linenum} is #{line.length} characters\n"
            break
        end
    }

    print "#{where}: Orphaned word\n" if /^\S+\Z/m.match(str)
    print "#{where}: Two spaces missing after punctuation\n" if /[.?!;]+["']?[^ ][^ ][A-Za-z]/m.match(str)
    print "#{where}: Use of the word 'you'\n" if /you/.match(str) and !/^vict emit/.match(str)
    print "#{where}: Use of the word 'very'\n" if /\bvery\b/.match(str)
    print "#{where}: Duplicated word '#{$1}'\n" if /\b(\w+)\s+\1\b/i.match(str)
    File.open("/tmp/desc.txt", "w") { |ouf|
        ouf.print str
    }
    IO.popen("/home/httpd/lib/dspell -d /usr/dict/ds.words -d ./tempus.words /tmp/desc.txt", "r") { |ext|
        ext.each { |line|
            print "#{where}: Possible misspelling of #{$1}\n" if /^M \d+ \d+ \[([^\]]+)\]$/.match(line)
        }
    }

    File.unlink('/tmp/desc.txt')
end

NOCAP_WORDS = [ "a","an","is","if","or","the","of","in","into","to",
                "and","your","on","off","by","out","no","it","at",
                "as","for" ]

def capitalize(original)
  str = original.dup
  str.gsub!(/\.+$/, '')
  result = ""
  cap_next = true
  str.split(/([^A-Za-z']+)/).each { |word|
    if cap_next
      word.capitalize!
      cap_next = false
    elsif !word.match(/[^A-Za-z']/) && NOCAP_WORDS.index(word.downcase)
      word.downcase!
    else
      cap_next = word.match(/[.?!]/)
      word.capitalize!
    end
    
    word.gsub!(/^Mc(.)/) { |match| "Mc#{match[1].downcase}" }

    result += word
  }

  if original != result
    $change_count += 1
  else
    $skip_count += 1
  end

  return result
end

def check_room(room)
  return if room.flags.match(/[lm]/) # Skip houses
  return if room.title.match(/(unused|reuse)/i) # Skip unused rooms
  title = capitalize(room.title)
  print "room #{room.idnum}: Title capitalization should be '#{title}' in room #{room.idnum}\n" if title != room.title
  return if room.flags.match(/[b]/) # Skip DTs
  room.description = check_desc(room.description, "room #{room.idnum}")
  room.sound = check_desc(room.sound, "room #{room.idnum} sound") unless room.sound.empty?
  room.extradescs.each { |exd|
    # ASCII art detect
    if !exd.description.match(/(---|___)/)
      exd.description = check_desc(exd.description, "room #{room.idnum} extradesc #{exd.keywords}")
    end
  }
  room.directions.each { |ignore, dir|
    dir.description = check_desc(dir.description, "room #{room.idnum} #{Roomexit::DIRECTIONS[dir.direction]} exit")
  }
  
end

def process_file(path)
  begin
    $change_count = 0
    $skip_count = 0

    File.open(path, "r") { |inf|
      $linenum = 0
      until inf.eof?
        new_room = Room.new
        check_room(new_room) if new_room.load(inf)
      end
    }
    
  rescue RuntimeError => exc
    print exc, " at  ", path, ":", $linenum, "\n"
    print exc.backtrace.join("\n")
  end
end

process_file(ARGV[0])
