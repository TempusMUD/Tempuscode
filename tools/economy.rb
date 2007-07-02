#!/usr/local/bin/ruby

require 'postgres'

ROOT_DIR = "/usr/tempus/TempusMUD/lib/players"

total_gold = 0
total_cash = 0
total_p_bank = 0
total_f_bank = 0
char_count = 0
account_count = 0
char_ids = Array.new
accounts_rejected = 0

$sql = PGconn.connect("", -1, "", "", "tempus", "realm")

ACCOUNT_CLAUSE = " where age(login_time) < '1 month' and age(login_time, creation_time) > interval '1 day' and idnum not in (select distinct account from players where idnum in (select player from sgroup_members where sgroup=20))"

accounts = $sql.query("select bank_past, bank_future from accounts" + ACCOUNT_CLAUSE)
accounts.each {|acct|
	total_p_bank += acct[0].to_i
	total_f_bank += acct[1].to_i
	account_count += 1
}

char_ids = $sql.query("select idnum from players where account in (select idnum from accounts" + ACCOUNT_CLAUSE + ")").collect {|char| char[0].to_i }

char_ids.each { |idnum|
	IO.foreach("#{ROOT_DIR}/character/#{idnum % 10}/#{idnum}.dat") { |line|
		result = /<stats level=\"([^"]+)\" /.match(line)
		break if result && result[1].to_i > 49
		result = /<money gold=\"([^"]+)\" cash=\"([^"]+)\"/.match(line)
		next if !result
		total_gold += result[1].to_i
		total_cash += result[2].to_i
		char_count += 1
		break
	}
}
	
total_onhand = total_gold + total_cash
total_inbank = total_p_bank + total_f_bank
total_money = total_onhand + total_inbank

printf("Total gold: %15d         Total cash: %d\n", total_gold, total_cash);
printf("Total past bank: %15d    Total future bank: %d\n", total_p_bank, total_f_bank);
printf("Total on hand: %15d       Total in bank: %d\n", total_onhand, total_inbank);
printf("Total money: %15d\n", total_money);
printf("Number of chars: %15d   Number of accounts: %d\n", char_count, account_count)
printf("On-hand/char: %15d         Bank/account: %d\n", total_onhand/char_count, total_inbank/account_count)
