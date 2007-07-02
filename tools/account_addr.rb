#!/usr/local/bin/ruby

require 'postgres'

sql = PGconn.connect("", -1, "", "", "tempus", "realm")

if ARGV.length != 1
	$stderr.print "Usage: account_addr.rb <netaddr>\n"
	exit 1
end

accounts = sql.query("select idnum,name, creation_time, login_time from accounts where login_addr like '#{ARGV[0]}%' or creation_addr like '#{ARGV[0]}%'")

accounts.each { |account|
	printf("%10d %s\n", account[0], account[1])
}
