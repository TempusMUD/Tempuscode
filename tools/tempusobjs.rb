#!/usr/bin/ruby

$linenum = 0

class Apply
  attr_accessor :key, :value

  PLACES = ["NONE", "STR", "DEX", "INT", "WIS", "CON", "CHA", "CLASS",
            "LEVEL", "AGE", "WEIGHT", "HEIGHT", "MAXMANA", "MAXHIT",
            "MAXMOVE", "GOLD", "EXP", "ARMOR", "HITROLL", "DAMROLL",
            "SAV_PARA", "SAV_ROD", "SAV_PETRI", "SAV_BREATH",
            "SAV_SPELL", "SNEAK", "HIDE", "RACE", "SEX", "BACKST",
            "PICK", "PUNCH", "SHOOT", "KICK", "TRACK", "IMPALE",
            "BEHEAD", "THROW", "RIDING", "TURN", "SAV_CHEM",
            "SAV_PSI", "ALIGN", "SAV_PHY", "CASTER", "WEAP_SPEED",
            "DISGUISE", "NOTHIRST", "NOHUNGER", "NODRUNK", "SPEED"]

  def initialize(key, value)
    @key = key
    @value = value
  end
  
  def inspect
    "Apply: #{@value} to #{PLACES[@key]}"
  end

  def to_s
    <<EOF
A
#{@key} #{@value}
EOF
  end
end

class Extradesc
  attr_accessor :keywords, :description
  def initialize
  end

  def inspect
    "Extradesc: #{@keywords}\n#{@description}"
  end

  def to_s
    return <<EOF
E
#{@keywords}~
#{@description}~
EOF
  end
end

class Item

  TYPES = ["LIGHT", "SCROLL", "WAND", "STAFF", "WEAPON", "CAMERA",
           "MISSILE", "TREASURE", "ARMOR", "POTION", "WORN", "OTHER",
           "TRASH", "TRAP", "CONTAINER", "NOTE", "DRINKCON", "KEY",
           "FOOD", "MONEY", "PEN", "BOAT", "FOUNTAIN", "WINGS",
           "VR_ARCADE", "SCUBA_MASK", "DEVICE", "INTERFACE",
           "HOLY_SYMB", "VEHICLE", "ENGINE", "BATTERY", "ENERGY_GUN",
           "WINDOW", "PORTAL", "TOBACCO", "CIGARETTE", "METAL",
           "VSTONE", "PIPE", "TRANSPORTER", "SYRINGE", "CHIT",
           "SCUBA_TANK", "TATTOO", "TOOL", "BOMB", "DETONATOR",
           "FUSE", "PODIUM", "PILL", "ENERGY_CELL", "V_WINDOW",
           "V_DOOR", "V_CONSOLE", "GUN", "BULLET", "CLIP",
           "MICROCHIP", "COMMUNICATOR", "SCRIPT", "INSTRUMENT",
           "BOOK"]

  INSTRUMENT_TYPES = ["Percussion", "String", "Wind"]

  WEAR_FLAGS = ["TAKE", "FINGER", "NECK", "BODY", "HEAD", "LEGS",
                "FEET", "HANDS", "ARMS", "SHIELD", "ABOUT", "WAIST",
                "WRIST", "WIELD", "HOLD", "CROTCH", "EYES", "BACK",
                "BELT", "FACE", "EAR", "ASS"]


  EXTRA_FLAGS = ["GLOW", "HUM", "NORENT", "NODONATE", "NOINVIS",
                 "INVISIBLE", "MAGIC", "NODROP", "BLESS", "ANTI_GOOD",
                 "ANTI_EVIL", "ANTI_NEUTRAL", "ANTI_MAGIC_USER",
                 "ANTI_CLERIC", "ANTI_THIEF", "ANTI_WARRIOR",
                 "NOSELL", "ANTI_BARB", "ANTI_PSYCHIC", "ANTI_PHYSIC",
                 "ANTI_CYBORG", "ANTI_KNIGHT", "ANTI_RANGER",
                 "ANTI_BARD", "ANTI_MONK", "BLURRED",
                 "MAGIC_NODISPEL", "UNUSED", "REPULSION_FIELD",
                 "TRANSPARENT", "DAMNED"]

  EXTRA2_FLAGS = ["RADIOACTIVE", "ANTI_MERC", "ANTI_SPARE1",
                  "ANTI_SPARE2", "ANTI_SPARE3", "HIDDEN", "TRAPPED",
                  "SINGULAR", "NOLOCATE", "NOSOIL", "CAST_WEAPON",
                  "TWO_HANDED", "BODY_PART", "ABLAZE", "CURSED_PERM",
                  "NOREMOVE", "THROWN_WEAPON", "GODEQ", "NO_MORT",
                  "BROKEN", "IMPLANT", "REINFORCED", "ENHANCED",
                  "REQ_MORT", "PROTECTED_HUNT", "RENAMED",
                  "UNAPPROVED"]

  EXTRA3_FLAGS = ["REQ_MAGE", "REQ_CLERIC", "REQ_THIEF",
                  "REQ_WARRIOR", "REQ_BARB", "REQ_PSIONIC",
                  "REQ_PHYSIC", "REQ_CYBORG", "REQ_KNIGHT",
                  "REQ_RANGER", "REQ_BARD", "REQ_MONK", "REQ_VAMPIRE",
                  "REQ_MERCENARY", "REQ_SPARE1", "REQ_SPARE2",
                  "REQ_SPARE3", "LATTICE_HARDENED", "STAY_ZONE",
                  "HUNTED", "NOMAG", "NOSCI"]

  CONTAINER_FLAGS = ["CLOSEABLE", "PICKPROOF", "CLOSED", "LOCKED"]


  LIQUIDS = ["WATER", "BEER", "WINE", "ALE", "DARKALE", "WHISKY",
             "LEMONADE", "FIREBRT", "LOCALSPC", "SLIME", "MILK",
             "TEA", "COFFEE", "BLOOD", "SALTWATER", "CLEARWATER",
             "COKE", "FIRETALON", "SOUP", "MUD", "HOLY_WATER", "OJ",
             "GOATS_MILK", "MUCUS", "PUS", "SPRITE", "DIET_COKE",
             "ROOT_BEER", "VODKA", "CITY_BEER", "URINE", "STOUT",
             "SOULS", "CHAMPAGNE", "CAPPUCINO", "RUM", "SAKE",
             "CHOCOLATE_MILK", "JUICE"]

  MATERIALS = [ "indeterminate", "water", "fire", "shadow", "gelatin",
                "light", "dreams", "*", "*", "*", "paper", "papyrus",
                "cardboard", "hemp", "parchment", "*", "*", "*", "*",
                "*", "cloth", "silk", "cotton", "polyester", "vinyl",
                "wool", "satin", "denim", "carpet", "velvet", "nylon",
                "canvas", "sponge", "*", "*", "*", "*", "*", "*", "*",
                "leather", "suede", "hard leather", "skin", "fur",
                "scales", "hair", "ivory", "*", "*", "flesh", "bone",
                "tissue", "cooked meat", "raw meat", "cheese", "egg",
                "*", "*", "*", "vegetable", "leaf", "grain", "bread",
                "fruit", "nut", "flower petal", "fungus", "slime",
                "*", "candy", "chocolate", "pudding", "*", "*", "*",
                "*", "*", "*", "*", "wood", "oak", "pine", "maple",
                "birch", "mahogony", "teak", "rattan", "ebony",
                "bamboo", "*", "*", "*", "*", "*", "*", "*", "*", "*",
                "*", "metal", "iron", "bronze", "steel", "copper",
                "brass", "silver", "gold", "platinum", "electrum",
                "lead", "tin", "chrome", "aluminum", "silicon",
                "titanium", "adamantium", "cadmium", "nickel",
                "mithril", "pewter", "plutonium", "uranium", "rust",
                "orichalcum", "*", "*", "*", "*", "*", "*", "*", "*",
                "*", "*", "*", "*", "*", "*", "*", "plastic",
                "kevlar", "rubber", "fiberglass", "asphalt",
                "concrete", "wax", "phenolic", "latex", "enamel", "*",
                "*", "*", "*", "*", "*", "*", "*", "*", "*", "glass",
                "crystal", "lucite", "porcelain", "ice", "shell",
                "earthenware", "pottery", "ceramic", "*", "*", "*",
                "*", "*", "*", "*", "*", "*", "*", "*", "stone",
                "azurite", "agate", "moss Agate", "banded Agate", "eye
                Agate", "tiger Eye Agate", "quartz", "rose Quartz",
                "smoky Quartz", "quartz 2", "hematite", "lapis
                lazuli", "malachite", "obsidian", "rhodochorsite",
                "tiger eye", "turquoise", "bloodstone", "carnelian",
                "chalcedony", "chysoprase", "citrine", "jasper",
                "moonstone", "onyx", "zircon", "amber", "alexandrite",
                "amethyst", "oriental amethyst", "aquamarine",
                "chrysoberyl", "coral", "garnet", "jade", "jet",
                "pearl", "peridot", "spinel", "topaz", "oriental
                topaz", "tourmaline", "sapphire", "black sapphire",
                "star sapphire", "ruby", "star ruby", "opal", "fire
                opal", "diamond", "sandstone", "marble", "emerald",
                "mud", "clay", "labradorite", "iolite", "spectrolite",
                "charolite", "basalt", "ash", "ink"]


  attr_accessor :vnum, :name, :aliases, :extradescs, :type

  def initialize()
    @vnum = 0
    @type = 0
    @cost = 0
    @rent = 0
    @owner = nil
    @material = 0
    @values = [ 0, 0, 0, 0 ]
    @name = ""
    @aliases = ""
    @line_desc = ""
    @action_desc = ""
    @soilage = ""
    @prog = ""
    @special_param = ""
    @extradescs = Array.new
    @applies = Array.new
	@bitvector = Array.new
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
Name: '#{@name}', Aliases: #{@aliases}
Vnum: #{@vnum}, Type: #{TYPES[@type - 1]}
L-Des: #{@line_desc}
Can be worn on: #{pbits(@worn, WEAR_FLAGS)}
Extra flags : #{pbits(@extra, EXTRA_FLAGS)}
Extra2 flags: #{pbits(@extra2, EXTRA2_FLAGS)}
Extra3 flags: #{pbits(@extra3, EXTRA3_FLAGS)}
Weight: #{@weight}, Cost: #{@cost}, Rent: #{@rent}, Timer: #{@timer}
Material: #{MATERIALS[@material]}, Maxdamage: #{@max_dam}, Damage: #{@damage}
Values 0-3: [#{@values[0]}] [#{@values[1]}] [#{@values[2]}] [#{@values[3]}]
EOF

    @extradescs.each { |exd| result += exd.inspect }
    @applies.each { |app| result += app.inspect }
    result += "Specparam:\n#{@special_param}\n" unless @special_param.empty?
    return result
  end

  def to_s
    # Outputs an object in the circle format
    result = <<EOF
##{@vnum}
#{@aliases}~
#{@name}~
#{@line_desc}~
#{@action_desc}~
#{@type} #{@extra} #{@extra2} #{@worn} #{@extra3}
#{@values.join(' ')}
#{@material} #{@max_dam} #{@damage} #{@weight}
#{@weight} #{@cost} #{@rent} #{@timer}
EOF
    @extradescs.each { |exd| result += exd.to_s }
    @applies.each { |app| result += app.to_s }
    0.upto(2) { |bitv|
      result += "V\n#{bitv} #{@bitvector[bitv]}\n" if @bitvector[bitv]
    }
    result += "O #{@owner}\n" if @owner
    result += "P\n#{@special_param}~\n" unless @special_param.empty?

    return result
  end

  def read_tilde_str(initial_line, inf)
    result = initial_line
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

  def load(inf, last_vnum)
    if last_vnum
      state = :aliases
      @vnum = last_vnum
    else
      state = :vnum
    end
    extradesc = match = nil
    inf.each { |line|
      line.chomp!
      $linenum += 1
      case state
      when :vnum
        match = line.match(/^([#\$])(\d+)?/)
        raise "Invalid object number format" unless match
        return false if match[1] == "$"
        @vnum = match[2].to_i
        state = :aliases
      when :aliases
        @aliases = read_tilde_str(line, inf)
        if @aliases.match(/\n/)
          $stderr.print "WARNING: multi-line aliases '#{line}\n"
        end
        state = :name
      when :name
        @name = read_tilde_str(line, inf)
        state = :line_desc
      when :line_desc
        @line_desc = read_tilde_str(line, inf)
        state = :action_desc
      when :action_desc
        @action_desc = read_tilde_str(line, inf)
        state = :numbers1
      when :numbers1
        match = line.match(/^(\d+) (\S+) (\S+) (\S+)( (\S+))?/)
        raise "Invalid first numeric line: #{line}" unless match
        @type = match[1].to_i
        @extra = match[2]
        @extra2 = match[3]
        @worn = match[4]
        @extra3 = match[6]
        state = :values
      when :values
        match = line.match(/^([\d-]+) ([\d-]+) ([\d-]+)(?: (\d+))?/)
        raise "Invalid second numeric line: #{line}" unless match
        @values = match[1..4].collect { |i| i.to_i }
        state = :numbers2
      when :numbers2
        match = line.match(/^(\d+) ([-0-9]+) ([-0-9]+)/)
        raise "Invalid third numeric line" unless match
        @material = match[1].to_i
        @max_dam = match[2].to_i
        @damage = match[3].to_i
        state = :numbers3
      when :numbers3
        match = line.match(/^([\d-]+) ([\d-]+) ([\d-]+)( ([\d-]+))?/)
        raise "Invalid fourth numeric line: #{line}" unless match
        @weight = match[1].to_i
        @cost = match[2].to_i
        @rent = match[3].to_i
        @timer = match[5].to_i if match[5]
        state = :optional
      when :optional
        case line
        when /^\$/
          return nil
        when /^#(\d+)/
          # End of object
          return $1.to_i
        when /^E/
          state = :extradesc_keywords
        when /^A/
          state = :apply
        when /^O/
          match = line.match(/^O (\d+)/)
          @owner = match[1].to_i
        when /^V/
          state = :bitvector
        when /^P/
          state = :special_param
        else
          raise "Invalid optional directive #{line}"
        end
      when :extradesc_keywords
        extradesc = Extradesc.new
        extradesc.keywords = read_tilde_str(line, inf)
        state = :extradesc_descrip
      when :extradesc_descrip
        extradesc.description = read_tilde_str(line, inf)
        @extradescs.push(extradesc)
        state = :optional
      when :apply
        match = line.match(/^(\d+) ([0-9-]+)/)
        raise "Invalid apply line '#{line}'" unless match
        @applies.push(Apply.new(match[1].to_i, match[2].to_i))
        state = :optional
      when :bitvector
        match = line.match(/(\d+) (\S+)/)
        raise "Invalid bitvector #{line}" unless match
        @bitvector[match[1].to_i - 1] = match[2]
        state = :optional
      when :special_param
        @special_param = read_tilde_str(line, inf)
        state = :optional
      end
    }
  end
end

def read_objects(path)
  result = Array.new
  item = vnum = nil
  
  File.open(path) { |inf|
    until !(line = inf.gets) || (match = line.match(/^\#(\d+)/))
    end

    if line
      vnum = match[1].to_i

      begin
        item = Item.new
        vnum = item.load(inf, vnum)
        result.push item
      end while vnum
    end
  }
  return result
end

def check_objects
  fnames = IO.read('lib/world/obj/index').collect { |fname| fname.chomp }
  fnames.each { |fname|
    if fname != '$'
      path = 'lib/world/obj/' + fname
#      print "Checking #{path}\n"
      objs = read_objects(path)
      objs.each {|obj|
        print "Object ##{obj.vnum} has key in its alias, and is of UNDEFINED type\n" if obj.aliases.match(/\bkey\b/) && obj.type == 0
    }
    end
  }
  nil
end
