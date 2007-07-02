#!/usr/local/bin/ruby

require 'postgres'

$sql = PGconn.connect('', -1, '', '', 'tempus', 'realm', 'tarrasque');

# First we need to get a hash of all the players stored in the account
accountIdx = Hash.new
accountQPs = Hash.new(0)
$sql.query("select idnum, account from players").each { |tuple|
	accountIdx[tuple[0].to_i] = tuple[1].to_i
}

# Now we can iterate through the player files, looking for the magic
# quest points line
Dir['/usr/tempus/TempusMUD/lib/players/character/**/*.dat'].each {|inf_path|
	idnum = %r#/(\d+)\.dat#.match(inf_path)[1].to_i
	next if !accountIdx[idnum]
	is_immort = false
	qp = 0
	IO.foreach(inf_path) {|line|
		case line
		when /<quest /
			res = / points="(\d+)"/.match(line)
			qp = res[1].to_i if res
		when /<stats level="(\d+)"/
			is_immort = true if $1.to_i > 49
		end
	}

	if !is_immort and qp > 0
		accountQPs[accountIdx[idnum]] += qp
	end
}

accountQPs.each { |acct, qps|
	print acct, " - ", qps, "\n"
#	$sql.exec("update accounts set quest_points=#{qps} where idnum=#{acct}")
}
