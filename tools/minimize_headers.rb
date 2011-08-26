#!/usr/bin/ruby

def build(path)
  return Kernel.system('chdir ' + File.dirname(path) + ';gcc -std=gnu99 -DHAVE_CONFIG_H -I. -I../../src -I.. -I../../src/include -I/usr/include/postgresql -I/usr/include/libxml2 -I/usr/include/glib-2.0 -I/usr/lib/glib-2.0/include -Wall -Wcast-qual -g -O -DUSE_IPV6 -Werror -Wfatal-errors -S -o /dev/null' + File.basename(path))
  # return Kernel.system('make CFLAGS="-Werror -Wfatal-errors"')# >/dev/null 2>&1')
end

path = ARGV[0]
system('cp', path, path + '~')

lines = IO.read(path + "~").split(/\n/)

# retrieve the lines included with the associated line numbers
includes = []
lines.each_with_index {|line, num|
  if line.match(/^\s*#include/)
    includes.push(num)
  end
}

if includes.empty?
  print "No includes in this path!\n"
  exit(0)
end

includes.reverse!

if not build(path)
  print "Didn't build initially!\n"
  exit(1)
end

# Find a single include file that can be removed without breaking the
# build (if one exists).  Repeat from beginning until no include files
# can be removed.
unnecessaries = []
includes.each {|exclude_num|
  next if unnecessaries.include?(exclude_num)
  print "Attempting ", path, " without ", lines[exclude_num], "\n"
  File.open(path, "w") {|ouf|
    lines.each_with_index { |line, num|
      if num != exclude_num and not unnecessaries.include?(num)
        ouf.print line, "\n"
      end
    }
  }

  if build(path)
    unnecessaries.push(exclude_num)
    print "PASSED... excluding ", lines[exclude_num], "\n"
  end
}

# write out final version
File.open(path, "w") {|ouf|
  lines.each_with_index { |line, num|
    if not unnecessaries.include?(num)
      ouf.print line, "\n"
    end
  }
}

if unnecessaries.empty?
  print "All includes were necessary.\n"
else
  unnecessaries.each {|num|
    print "Removed ", lines[num], "\n"
  }
end

if build(path)
  print "Done.\n"
else
  print "Didn't build on final file!\n"
end
