#!/usr/local/bin/ruby

require 'rexml/document'
require 'postgres'

# Convert the account entries in the database and the character files
# into a player file usable by older versions of Tempus.

CCLASS_TABLE = { 'Mage' => 0,
				'Cleric' => 1,
				'Thief' => 2,
				'Warrior' => 3,
				'Barbarian' => 4,
				'Psionic' => 5,
				'Cyborg' => 6,
				'Knight' => 7,
				'Ranger' => 8,
				'Bard' => 9,
				'Monk' => 10,
				'Vampire' => 11,
				'Mercenary' => 12 }
RACE_TABLE = { 'Human' => 0,
				'Elf' => 1,
				'Dwarf' => 2,
				'Half Orc' => 3,
				'Halfling' => 5,
				'Tabaxi' => 6,
				'Drow' => 7,
				'Orc' => 16,
				'Minotaur' => 19 }
SEX_TABLE = { 'Male' => 1,
				'Female' => 2,
				'Neuter' => 0 }


sql = PGconn.connect("", -1, "", "", "tempus", "realm")

players = sql.query("select accounts.password, players.idnum from accounts,players where accounts.idnum=players.account")

File.open("new_pfile", "wb") { |ouf|

	players.each { |tuple|
		password = tuple[0]
		idnum = tuple[1].to_i

		File.open("/usr/tempus/TempusMUD/lib/players/character/#{idnum % 10}/#{idnum}.dat", "r") { |inf|
			doc = REXML::Document.new(inf)
			root = doc.root
			level = root.elements['stats'].attributes['level'].to_i
			next if level < 50

			name = root.attributes['name']

			print "Converting #{name}...\n"
			title = ''
			poofin = ''
			poofout = ''
			desc = ''
			sex = SEX_TABLE[root.elements['stats'].attributes['sex']] or 0
			race = RACE_TABLE[root.elements['stats'].attributes['race']] or 0
			height = root.elements['stats'].attributes['height'].to_i
			weight = root.elements['stats'].attributes['weight'].to_i
			align = root.elements['stats'].attributes['align'].to_i
			cclass = CCLASS_TABLE[root.elements['stats'].attributes['class']]
			title = root.elements['title'].text if root.elements['title']
			poofin = root.elements['poofin'].text if root.elements['poofin']
			poofout = root.elements['poofout'].text if root.elements['poofout']
			desc = root.elements['description'].text if root.elements['description']

			now = Time.now.to_i
			precord = [ name, desc, title, poofin, poofout, cclass, 0, weight,
						height, 0, sex, race, level, 0, now, now, 0, password,
						align, idnum, '', 25, 0, 25, 25, 25, 25, 25, 
						100, 100, 100, 100, 100, 100, ''
				].pack("Z21 Z1024 Z61 Z256 Z256 sssss CCCC LLL Z41 llZ911 ccccccc ssssss Z2401")
			p precord.length
			ouf.print precord
		}
	}
}
