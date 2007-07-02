#!/usr/local/bin/ruby

if ARGV.size < 2
	$stderr.print "Usage: analyze_memusage.rb <memusage file> <executable>\n"
	exit 1
end

block_sums = Hash.new
block_sums.default = 0
block_counts = Hash.new
block_counts.default = 0
sources = Array.new
functions = Array.new
total_bytes = 0
total_blocks = 0

$stderr.print "Analyzing file...\n"

IO.foreach(ARGV[0]) { |line|
	(num, size, addr, alloced) = line.strip.split(/\s+/)
	if alloced
		block_sums[alloced] += size.to_i
		block_counts[alloced] += 1
		total_bytes += size.to_i
		total_blocks += 1
	end
}

addresses = block_sums.keys.sort { |a, b| block_sums[b] <=> block_sums[a] }

$stderr.print "Getting file/line numbers...\n"

ext = IO.popen("-", "r+") { |ext|
	if !ext
		exec("addr2line -C -f -s -e #{ARGV[1]}")
	end

	addresses.each {|addr|
		ext.print addr, "\n"
		func = ext.gets.chomp
		if func == '??'
			functions.unshift addr
		else
			functions.unshift func
		end
		sources.unshift ext.gets.chomp
	}

}

$stderr.print "Outputting file...\n"

addresses.each {|addr|
	print functions.pop, " (", sources.pop, ")\n"
	print "    ", block_counts[addr], " blocks, ",
		block_sums[addr], " bytes, ",
		block_sums[addr] / block_counts[addr], " bytes/block\n\n"
}
print "Total bytes: #{total_bytes}\n"
print "Total blocks: #{total_blocks}\n"
