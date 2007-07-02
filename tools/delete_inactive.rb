#!/usr/local/bin/ruby

require 'postgres'

def delete_account(sql, account)
	# Don't delete the disabled account
	return if account == 1819

	# delete players from account
	players = sql.query("select idnum from players where account=#{account}").collect {|tuple| tuple.first.to_i}.sort
	players.each { |player|
		sql.exec("update clans set owner=null where owner=#{player}")
		sql.exec("delete from clan_members where player=#{player}")
		sql.exec("delete from sgroup_members where player=#{player}")
		sql.exec("delete from trusted where player=#{player}")
		sql.exec("delete from bounty_hunters where idnum=#{player} or victim=#{player}")
		sql.exec("update board_messages set author=null where author=#{player}")
		sql.exec("delete from players where idnum=#{player}")
	}
	
	# delete account
	sql.exec("update forum_messages set author=null where author=#{account}")
	sql.exec("delete from voting_accounts where account=#{account}")
	sql.exec("delete from trusted where account=#{account}")
	sql.exec("delete from accounts where idnum=#{account}")
end

sql = PGconn.connect("", -1, "", "", "tempus", "realm", "tarrasque")
accounts = sql.query("select idnum from accounts where age(login_time, creation_time) < '1 week' and age(creation_time) > '4 weeks'").collect {|tuple| tuple.first.to_i}.sort
accounts.each { |account| delete_account(sql, account) }
print accounts.length, " accounts deleted due to oneshottedness.\n"

accounts = sql.query("select idnum from accounts where age(login_time) > '2 years'").collect {|tuple| tuple.first.to_i}.sort
accounts.each { |account| delete_account(sql, account) }
print accounts.length, " accounts deleted to >2 year inactivity.\n"

accounts = sql.query("select idnum from accounts where age(login_time, creation_time) < '4 weeks' and age(creation_time) > '1 year'").collect {|tuple| tuple.first.to_i}.sort
accounts.each { |account| delete_account(sql, account) }
print accounts.length, " accounts deleted due to notcomingbackedness.\n"
