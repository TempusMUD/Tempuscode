SCREEN_WIDTH = 72

NOCAP_WORDS = [ "a","an","is","if","or","the","of","in","into","to",
                "and","your","on","off","by","out","no","it","at",
                "as","for" ]

def check_text(str, where)
  return if str.match(/^\s*$/m)

  linenum = 0
  str.each { |line|
    linenum += 1
    if line.length > SCREEN_WIDTH
      print "#{where}: Line #{linenum} is #{line.length} characters\n"
      break
    end
  }

  print "#{where}: Two spaces missing after punctuation\n" if /[.?!;]+["']?[^ \n][^ ][A-Za-z]/.match(str)
  print "#{where}: Use of the word 'you'\n" if /\byou\b/.match(str) and !/^vict emit/.match(str)
  print "#{where}: Use of the word 'your'\n" if /\byour\b/.match(str) and !/^vict emit/.match(str)
  print "#{where}: Use of the word 'you're'\n" if /\byou're\b/.match(str) and !/^vict emit/.match(str)
  print "#{where}: Use of the word 'very'\n" if /\bvery\b/.match(str)
  print "#{where}: Use of the word 'whatever'\n" if /\bwhatever\b/.match(str)
  print "#{where}: Duplicated word '#{$1}'\n" if /\b(\w+)\s+\1\b/mi.match(str)
  File.open("/tmp/desc.txt", "w") { |ouf|
    ouf.print str
  }
  IO.popen("aspell --add-extra-dicts=./tempus.dict list </tmp/desc.txt", "r") { |ext|
    ext.each { |line|
      print "#{where}: Possible misspelling of #{line}"
    }
  }

  if $?.exitstatus >= 128
    print "### segfault detected\n"
    Kernel.exit(0)
  end

  File.unlink('/tmp/desc.txt')
end

def check_desc(str, where)
  if str.match(/^\s*$/m)
    print "#{where}: no desc\n"
    return
  end

  print "#{where}: Orphaned word\n" if /^\S+\Z/m.match(str)

  check_text(str, where)
end

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

    word.gsub!(/^Mc(.)/) { |match| "Mc#{$1.upcase}" }

    result += word
  }

  if original != result
    $change_count += 1
  else
    $skip_count += 1
  end

  return result
end

