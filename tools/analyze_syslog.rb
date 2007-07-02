#!/usr/local/bin/ruby

SEARCHES = [
	/transferred [A-Z]/,
	/forced [A-Z]/,
	/:: security: /
]

if ARGV.length > 0
	inf_name = ARGV[0]
else
	inf_name = '/usr/tempus/TempusMUD/syslog'
end

addr_cache = Hash.new
backtrace_cache = Hash.new
error = ""

IO.foreach(inf_name) { |line|
	SEARCHES.each {|exp|
		if exp.match(line)
			print line
			break
		end
	}
	if /SYSERR: /.match(line) or /CHECK: /.match(line)
		error = line
	end
	if /TRACE: /.match(line)
		addrs = line.scan(/0x[A-Za-z0-9]+/).join(" ")
		if !addr_cache.has_key?(addrs)
			addr_cache[addrs] = `addr2line -e /usr/tempus/TempusMUD/bin/circle #{addrs}`
		end
		backtrace = addr_cache[addrs]
		problem_key = error.gsub(/.*?:: /, '') + "\n" + backtrace
		if !backtrace_cache.has_key?(problem_key)
			backtrace_cache[problem_key] = true
			print error + backtrace
		end
	end
}
