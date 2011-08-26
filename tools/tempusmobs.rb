#!/usr/bin/ruby

require 'stringutils'

$linenum = 0

class Mobile

  SEXES = [ "NEUTER", "MALE", "FEMALE" ]

  MOB_FLAGS = [ "SPEC", "SENTINEL", "SCAVENGER", "ISNPC", "AWARE",
                "AGGR", "STAY-ZONE", "WIMPY", "AGGR_EVIL",
                "AGGR_GOOD", "AGGR_NEUTRAL", "MEMORY", "HELPER",
                "!CHARM", "!SUMMN", "!SLEEP", "!BASH", "!BLIND",
                "!TURN", "!PETRI", "PET", "SOULL ESS", "SPRT_HNTR",
                "UTILITY" ]

  MOB2_FLAGS = [ "SCRIPT", "MOUNT", "STAY_SECT", "ATK_MOBS", "HUNT",
                 "LOOT", "NOSTUN", "SELLR", "!WEAR", "SILENT",
                 "FAMILIAR", "NO_FLOW", "!APPROVE", "RENAMED",
                 "!AGGR_RACE", "15", "16", "17", "18", "19" ]

  TONGUES = [ "common", "arcanum", "modriatic", "elvish", "dwarven",
              "orcish", "klingon", "hobbish", "sylvan", "underdark",
              "mordorian", "daedalus", "trollish", "ogrish",
              "infernum", "trogish", "manticora", "ghennian",
              "draconian", "inconnu", "abyssal", "astral", "sibilant",
              "gish", "centralian", "kobolas", "kalerrian",
              "rakshasian", "gryphus", "rotarial", "celestial",
              "elysian", "greek", "deadite", "elemental" ]

  AFF_FLAGS = [ "BLiND", "INViS", "DT-ALN", "DT-INV", "DT-MAG",
                "SNSE-L", "WaTWLK", "SNCT", "GRP", "CRSE", "NFRa",
                "PoIS", "PRT-E", "PRT-G", "SLeP", "!TRK", "NFLT",
                "TiM_W", "SNK", "HiD", "H2O-BR", "CHRM", "CoNFu",
                "!PaIN", "ReTNa", "ADReN", "CoNFi", "ReJV", "ReGN",
                "GlWL", "BlR", "31" ]

  AFF2_FLAGS = [ "FLUOR", "TRANSP", "SLOW", "HASTE", "MOUNTED",
                 "FIRESHLD", "BERSERK", "INTIMD", "TRUE_SEE",
                 "DIV_ILL", "PROT_UND", "INVS_UND", "ANML_KIN",
                 "END_COLD", "PARALYZ", "PROT_LGTN", "PROT_FIRE",
                 "TELEK", "PROT_RAD", "BURNING!", "!BEHEAD",
                 "DISPLACR", "PROT_DEVILS", "MEDITATE", "EVADE",
                 "BLADE BARRIER", "OBLIVITY", "NRG_FIELD", "PETRI",
                 "VERTIGO", "PROT_DEMONS" ]

  AFF3_FLAGS = [ "ATTR-FIELD", "ENERGY_LEAK", "POISON-2", "POISON-3",
                 "SICK", "S-DEST!", "DAMCON", "STASIS", "P-SPHERE",
                 "RAD", "DET_POISON", "MANA_TAP", "ENERGY_TAP",
                 "SONIC_IMAGERY", "SHR_OBSC", "!BREATHE", "PROT_HEAT",
                 "PSISHIELD", "PSY CRUSH", "2xDAM", "ACID",
                 "HAMSTRING", "GRV WELL", "SMBL PAIN", "EMP_SHLD",
                 "IAFF", "!UNUSED!", "TAINTED", "INFIL", "DivPwR",
                 "MANA_LEAK" ]

  CLASSES = [ "Mage", "Cleric", "Thief", "Warrior", "Barbarian",
              "Psionic", "Physic", "Cyborg", "Knight", "Ranger",
              "Bard", "Monk", "Vampire", "Mercenary", "Spare1",
              "Spare2", "Spare3", "ILL", "ILL", "ILL", "ILL", "ILL",
              "ILL", "ILL", "ILL", "ILL", "ILL", "ILL", "ILL", "ILL",
              "ILL", "ILL", "ILL", "ILL", "ILL", "ILL", "ILL", "ILL",
              "ILL", "ILL", "ILL", "ILL", "ILL", "ILL", "ILL", "ILL",
              "ILL", "ILL", "ILL", "ILL", "Mobile", "Bird",
              "Predator", "Snake", "Horse", "Small", "Medium",
              "Large", "Scientist", "ILL", "Skeleton", "Ghoul",
              "Shadow", "Wight", "Wraith", "Mummy", "Spectre",
              "Vampire", "Ghost", "Lich", "Zombie", "ILL", "ILL",
              "ILL", "ILL", "ILL", "ILL", "ILL", "ILL", "ILL", "ILL",
              "Earth", "Fire", "Water", "Air", "Lighting", "ILL",
              "ILL", "ILL", "ILL", "ILL", "Green", "White", "Black",
              "Blue", "Red", "Silver", "Shadow_D", "Deep", "Turtle",
              "ILL", "Least", "Lesser", "Greater", "Duke", "Arch",
              "ILL", "ILL", "ILL", "ILL", "ILL", "Hill", "Stone",
              "Frost", "Fire", "Cloud", "Storm", "ILL", "ILL", "ILL",
              "Red", "Blue", "Green", "Grey", "Death", "Lord", "ILL",
              "ILL", "ILL", "ILL", "Type I", "Type II", "Type III",
              "Type IV", "Type V", "Type VI", "Semi", "Minor",
              "Major", "Lord", "Prince", "ILL", "ILL", "ILL", "ILL",
              "ILL", "ILL", "ILL", "ILL", "ILL", "Astral", "Monadic",
              "Movanic", "ILL", "ILL", "ILL", "ILL", "ILL", "ILL",
              "ILL", "Fire", "Lava", "Smoke", "Steam", "ILL", "ILL",
              "ILL", "ILL", "ILL", "ILL", "Arcana", "Charona",
              "Dergho", "Hydro", "Pisco", "Ultro", "Yagno", "Pyro",
              "Godling", "Deity" ]

  RACES = [ "Human", "Elf", "Dwarf", "Half Orc", "Klingon",
            "Halfling", "Tabaxi", "Drow", "ILL", "ILL", "Mobile",
            "Undead", "Humanoid", "Animal", "Dragon", "Giant", "Orc",
            "Goblin", "Hafling", "Minotaur", "Troll", "Golem",
            "Elemental", "Ogre", "Devil", "Trog", "Manticore",
            "Bugbear", "Draconian", "Duergar", "Slaad", "Robot",
            "Demon", "Deva", "Plant", "Archon", "Pudding", "Alien 1",
            "Predator Alien", "Slime", "Illithid", "Fish", "Beholder",
            "Gaseous", "Githyanki", "Insect", "Daemon", "Mephit",
            "Kobold", "Umber Hulk", "Wemic", "Rakshasa", "Spider",
            "Griffin", "Rotarian", "Half Elf", "Celestial",
            "Guardinal", "Olympian", "Yugoloth", "Rowlahr",
            "Githzerai", "Faerie" ]

  POSITIONS = [ "Dead", "Mortally wounded", "Incapacitated",
                "Stunned", "Sleeping", "Resting", "Sitting",
                "Fighting", "Standing", "Flying", "Mounted",
                "Swimming" ]

  ATTACKS = [ "hit", "sting", "whip", "slash", "bite", "bludgeon",
              "crush", "pound", "claw", "maul", "thrash", "pierce",
              "blast", "punch", "stab", "zap", "rip", "chop",
              "shoot" ]

  attr_accessor :vnum, :alignment, :aliases, :name, :ldesc, :desc,
  :mob_flags, :mob2_flags, :aff_flags, :aff2_flags, :aff3_flags,
  :cur_tongue, :known_tongues, :hitnodice, :hitdice, :hitplus, :gold,
  :cash, :hitroll, :damroll, :morale, :str, :int, :wis, :con, :dex, :cha

  def initialize
    @aff_flags = ""
    @aff2_flags = ""
    @aff3_flags = ""
    @aliases = ""
    @alignment = 0
    @armor = 100
    @cash = 0
    @cha = 11
    @class = 0
    @con = 11
    @cur_tongue = 0
    @damnodice = 0
    @damsizedice = 0
    @damroll = 0
    @desc = ""
    @dex = 11
    @gen = 0
    @gold = 0
    @height = 198
    @hitpbonus = 0
    @hitpnodice = 0
    @hitpsizedice = 0
    @hitroll = 0
    @int = 11
    @lair = -1
    @ldesc = ""
    @leader = -1
    @mob_flags = ""
    @mob2_flags = ""
    @movebuf = ""
    @name = ""
    @race = 0
    @remort_class = -1
    @sex = 0
    @str = 11
    @stradd = 0
    @tongues = []
    @vnum = 0
    @voice = 0
    @weight = 200
    @wis = 11
  end

  def pbits(bits, desc)
    return "NIL" if !bits
    flags = ""
    if bits == "0"
      flags = " NONE"
    else
      bits.each_byte { |flag|
        if flag > ?Z
          flags += " " + desc[flag - ?a]
        else
          flags += " " + desc[26 + flag - ?A]
        end
      }
    end
    return flags
  end

  def inspect
    result = <<EOF
#{SEXES[@sex]} MOB '#{@name}'
Alias: #{@aliases}, VNum: #{@vnum}
L-Des: #{@ldesc}
Race: #{RACES[@race]}, Class: #{CLASSES[@class]}/#{(@gen > 0) ? CLASSES[@remort_class]:"None"} Gen: #{@gen}
Lev: #{@level}, Exp, #{@exp}, Align: #{@alignment}
Height:  #{@height} centimeters , Weight: #{@weight} pounds.
Str: #{@str}/#{@stradd}  Int: #{@int}  Wis: #{@wis}  Dex: #{@dex}  Con: #{@con}  Cha: #{@cha}
Hit p.:#{@hitpnodice}d#{@hitpsizedice}+#{@hitpbonus}  Mana p.:#{@maxmana}  Move p.:#{@maxmove}
AC: #{@armor}/10, Hitroll: #{@hitroll}, Damroll: #{@damroll}
Gold: #{@gold}, Cash: #{@cash}, (Total: #{@gold + @cash})
Pos: #{POSITIONS[@pos]}, Dpos: #{POSITIONS[@dpos]}, Attack: #{ATTACKS[@attack]}
NPC flags: #{pbits(@mob_flags, MOB_FLAGS)}
NPC flags(2): #{pbits(@mob2_flags, MOB2_FLAGS)}
NPC Dam: #{@damnodice}d#{@damsizedice}, Morale: #{@morale}, Lair: #{@lair}, Ldr: #{@leader}
EOF
    result += "AFF: #{pbits(@aff_flags, AFF_FLAGS)}\n" if @aff_flags != '0'
    result += "AFF2: #{pbits(@aff2_flags, AFF2_FLAGS)}\n" if @aff2_flags != '0'
    result += "AFF3: #{pbits(@aff3_flags, AFF3_FLAGS)}\n" if @aff3_flags != '0'

    result += <<EOF
Currently speaking: #{TONGUES[@cur_tongue]}
Known Languages: #{@tongues.map { |id| TONGUES[id] }.join(", ")}
EOF
    return result
  end

  def to_s
    result = <<EOF
##{@vnum}
#{@aliases}~
#{@name}~
#{@ldesc}
~
#{@desc}~
#{@mob_flags} #{@mob2_flags} #{@aff_flags} #{@aff2_flags} #{@aff3_flags} #{@alignment} E
#{@level} #{20 - @hitroll} #{@armor / 10} #{@hitpnodice}d#{@hitpsizedice}+#{@hitpbonus} #{@damnodice}d#{@damsizedice}+#{@damroll}
#{@gold} #{@exp} #{@race} #{@class}
#{@pos} #{@dpos} #{@sex} #{@attack}
EOF
    result += "Str: #{@str}\n" unless @str == 11
    result += "StrAdd: #{@stradd}\n" unless @stradd == 0
    result += "Int: #{@int}\n" unless @int == 11
    result += "Wis: #{@wis}\n" unless @wis == 11
    result += "Dex: #{@dex}\n" unless @dex == 11
    result += "Con: #{@con}\n" unless @con == 11
    result += "Cha: #{@cha}\n" unless @cha == 11
    result += "MaxMana: #{@maxmana}\n" unless @maxmana == 10
    result += "MaxMove: #{@maxmove}\n" unless @maxmove == 50
    result += "Weight: #{@weight}\n" unless @weight == 200
    result += "Height: #{@height}\n" unless @height == 198
    result += "CurTongue: #{@cur_tongue}\n" unless @cur_tongue == 0
    @tongues.each {|id| result += "KnownTongue: #{id}\n" }
    result += "Cash: #{@cash}\n" unless @cash == 0
    result += "Morale: #{@morale}\n" unless @morale == 100
    result += "Lair: #{@lair}\n" unless @lair < 0
    result += "Leader: #{@leader}\n" unless @leader < 0
    result += "RemortClass: #{@remort_class}\n" unless @remort_class == -1
    result += "Move_buf: #{@movebuf}\n" if @movebuf
    result += "SpecParam:\n#{@specparam}~\n" if @specparam
    result += "LoadParam:\n#{@loadparam}~\n" if @loadparam
    result += "Prog:\n#{@prog}~\n" if @prog
    result += "Generation: #{@gen}\n" unless @gen == 0
    result += "E\n"
    return result
  end

  def read_tilde_str(initial_line, inf)
    result = initial_line.chomp
    if !result.match(/~/)
      inf.each { |line|
        line.chomp!
        result += "\n"
        result += line
        break if line.match(/~/)
      }
    end
    return result.gsub(/~/,'')
  end

  def load(inf)
    state = :vnum
    mob_type = nil
    inf.each { |line|
      case state
      when :vnum
        match = line.match(/^([#\$])(\d+)?/)
        raise "Invalid mobile vnum format: #{line}" unless match
        return false if match[1] == "$"
        @vnum = match[2].to_i
        state = :aliases
      when :aliases
        @aliases = read_tilde_str(line, inf)
        @aliases.gsub!(/[\r\n].*/, '')
        state = :name
      when :name
        @name = read_tilde_str(line, inf)
        @name.gsub!(/[\r\n].*/, '')
        state = :ldesc
      when :ldesc
        @ldesc = read_tilde_str(line, inf)
        @ldesc.gsub!(/[\r\n].*/, '')
        state = :desc
      when :desc
        @desc = read_tilde_str(line, inf)
        state = :flags
      when :flags
        match = line.match(/^(\S+)\s+(\S+)\s+(\S+)\s+(\S+)\s+(\S+)\s+([\d-]+)\s+(.)/)
        raise "Invalid flag line: #{line}" unless match
        @mob_flags = match[1]
        @mob2_flags = match[2]
        @aff_flags = match[3]
        @aff2_flags = match[4]
        @aff3_flags = match[5]
        @alignment = match[6].to_i

        mob_type = match[7]
        raise "Invalid mob type #{mob_type}" unless ['S','E'].index(mob_type)

        state = :numbers1
      when :numbers1
        match = line.match(/^(\d+)\s+([\d-]+)\s+([\d-]+)\s+(\d+)d(\d+)\+(\d+)\s+(\d+)d(\d+)\+([-0-9]+)/)
        raise "Invalid first numeric line: #{line}" unless match
        @level = match[1].to_i
        @hitroll= 20 - match[2].to_i
        @armor = 10 * match[3].to_i
        @hitpnodice = match[4].to_i
        @hitpsizedice = match[5].to_i
        @hitpbonus = match[6].to_i
        @damnodice = match[7].to_i
        @damsizedice = match[8].to_i
        @damroll = match[9].to_i
        state = :numbers2
      when :numbers2
        match = line.match(/^(\d+)\s+(\d+)(?:\s+(\d+)\s+(\d+))?/)
        raise "Invalid second numeric line: #{line}" unless match
        @gold = match[1].to_i
        @exp = match[2].to_i
        @race = match[3].to_i if match[3]
        @class = match[4].to_i if match[4]
        state = :numbers3
      when :numbers3
        match = line.match(/^(\d+)\s+(\d+)\s+(\d+)(?:\s+(\d+))?/)
        raise "Invalid third numeric line: #{line}" unless match
        @pos = match[1].to_i
        @dpos = match[2].to_i
        @sex = match[3].to_i
        @attack = match[4].to_i

        state = (mob_type == 'E') ? (:optional):(:replies)
      when :optional
        case line
        when /^#/
            raise "Unterminated E section in mob @vnum"
        when /^E$/
          state = :replies
        when /SpecParam:/
          state = :specparam
        when /LoadParam:/
          state = :loadparam
        when /Prog:/
          state = :prog
        when /BareHandAttack: (\d+)/
          @attack = $1.to_i
        when /Move_buf: (.+)/
          @movebuf = $1
        when /Str: (\d+)/
          @str = $1.to_i
        when /StrAdd: (\d+)/
          @stradd = $1.to_i
        when /Int: (\d+)/
          @int = $1.to_i
        when /Wis: (\d+)/
          @wis = $1.to_i
        when /Dex: (\d+)/
          @dex = $1.to_i
        when /Con: (\d+)/
          @con = $1.to_i
        when /Cha: (\d+)/
          @cha = $1.to_i
        when /MaxMana: (\d+)/
          @maxmana = $1.to_i
        when /MaxMove: (\d+)/
          @maxmove = $1.to_i
        when /Height: (\d+)/
          @height = $1.to_i
        when /Weight: (\d+)/
          @weight = $1.to_i
        when /RemortClass: (\d+)/
          @remort_class = $1.to_i
        when /Class: (\d+)/
          @class = $1.to_i
        when /Race: (\d+)/
          @race = $1.to_i
        when /Credits: (\d+)/
          @credits = $1.to_i
        when /Cash: (\d+)/
          @cash = $1.to_i
        when /Morale: (\d+)/
          @morale = $1.to_i
        when /Lair: (\d+)/
          @lair = $1.to_i
        when /Leader: (\d+)/
          @leader = $1.to_i
        when /Generation: (\d+)/
          @gen = $1.to_i
        when /CurTongue: (\d+)/
          @cur_tongue = $1.to_i
        when /KnownTongue: (\d+)/
          @tongues.push($1.to_i)
        when /CurLang: (\d+)/
          @cur_tongue = $1.to_i - 1
        when /KnownLang: (\d+)/
          num = $1
          0.upto(32) { |bit|
            if ((num >> bit) & 1)
              @tongues.push(bit)
            end
          }
        when /Voice: (\d+)/
          @voice = $1.to_i
        else
          raise "Unrecognized espec keyword in mobile #{@vnum}"
        end
      when :specparam
        @specparam = read_tilde_str(line, inf)
        state = :optional
      when :loadparam
        @loadparam = read_tilde_str(line, inf)
        state = :optional
      when :prog
        @prog = read_tilde_str(line, inf)
        state = :optional
      when :replies
        case line
        when /^R/
          read_tilde_str('', inf)
          read_tilde_str('', inf)
        when /^[\$#]/
          inf.seek(-(line.length), IO::SEEK_CUR)
          break
        end
      else
          raise "Invalid state reached in mob #{@vnum}"
      end
    }
    @morale = (/h/.match(@mob_flags)) ? ([30, @level].max):100;
    return true
  end

  def check
    check_desc(@desc, "mobile #{@vnum} desc")
    check_text(@movebuf, "mobile #{@vnum} move_buf")
    check_desc(@ldesc, "mobile #{@vnum} ldesc")
    if match = /[^a-zA-Z ]/.match(@aliases)
      print "mobile #{vnum} alias contains invalid character '#{match[0]}'\n"
    end
  end
end
