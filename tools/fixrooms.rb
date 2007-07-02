#!/usr/bin/ruby

require 'tempusrooms'

SCREEN_WIDTH = 70

$change_count = 0
$skip_count = 0

def format_desc(original, indent)
  str = original.strip.dup
  lines = []
  line = indent ? "  ":""

  # Insure at least one space after punctuation
  str.gsub!(/([.?!;]+["']?)/, "\\1 ")
  str.split(/\s+/m).each do |word|
    next if word.match(/^very/)
    if line.size + word.size >= SCREEN_WIDTH
      lines << line.rstrip
      line = word
    elsif line.empty?
      line = word
    elsif line.match(/[.?!]+["']?$/)
      line << "  " << word
    else
      line << " " << word
    end
  end

  lines << line.rstrip unless line.empty?

  result = lines.join("\n") + "\n"

  if original != result
    $change_count += 1
  else
    $skip_count += 1
  end

  return result
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

def fix_room!(room)
  if room.flags.match(/[lm]/) # Skip houses
    p room
  end
  room.title = capitalize(room.title)
  return if room.flags.match(/[b]/) # Skip DTs
  room.description = format_desc(room.description, true)
  room.sound = format_desc(room.sound, false) unless room.sound.empty?
  room.extradescs.each { |exd|
    # ASCII art detect
    if !exd.description.match(/(---|___)/)
      exd.description = format_desc(exd.description, false)
    end
  }
  room.directions.each { |ignore, dir|
    dir.description = format_desc(dir.description, false)
  }
  
end

def process_file(path)
  begin
    $change_count = 0
    $skip_count = 0

    File.open(path + ".new", "w") { |ouf|
      File.open(path, "r") { |inf|
        $linenum = 0
        until inf.eof?
          new_room = Room.new
          if new_room.load(inf)
            fix_room!(new_room)
            ouf.print new_room.to_s
          end
        end
        ouf.print "$~\n"
      }
    }
  rescue RuntimeError => exc
    print exc, " at  ", path, ":", $linenum, "\n"
    print exc.backtrace.join("\n")
  end
end

process_file(ARGV[0])
