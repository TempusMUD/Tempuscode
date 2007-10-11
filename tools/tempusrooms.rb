#!/usr/bin/ruby

$linenum = 0

class Roomsearch
  COMMANDS = [ "NONE", "DOOR", "MOBILE", "OBJECT", "REMOVE",
               "GIVE", "EQUIP", "TRANS", "SPELL", "DAMAGE",
               "SPAWN", "LDROOM" ]

  FLAGS = [ "REPEATABLE", "TRIPPED", "IGNORE", "CLANPASSWD",
            "TRIG_ENTER", "TRIG_FALL", "NOTRIG_FLY", "NOMOB",
            "NEWBIE_ONLY", "NOMSG", "NOEVIL", "NONEU", "NOGOOD",
            "NOMAGE", "NOCLER", "NOTHI", "NOBARB", "NORANG", "NOKNI",
            "NOMONK", "NOPSI", "NOPHY", "NOMERC", "NOBARD",
            "NOABBREV", "NOAFFMOB", "NOPLAYER", "REMORT_ONLY",
            "MATCH_ALL", "NOLOOK", "FAIL_TRIP" ]

  
  attr_accessor :triggers, :keywords, :to_vict, :to_room, :to_remote,
                :command, :arg0, :arg1, :arg2, :flags, :failure
  def initialize
    @triggers = ''
    @keywords = ''
    @to_vict = ''
    @to_room = ''
    @to_remote = ''
    @command = @arg0 = @arg1 = @arg2 = -1
    @flags = 0
    @failure = nil
  end

  def inspect
    result = "Search triggers: #{@triggers}, keywords: #{@keywords}\n"
    result += "To_vict  : #{@to_vict}\n"
    result += "To_room  : #{@to_room}\n"
    result += "To_remote: #{@to_remote}\n"
    result += "Fail chance: #{@failure}\n"
    result += "%14s%12d%12d%12d\n" %
      [ COMMANDS[@command], @arg0, @arg1, @arg2 ]
    flags = []
    (0..32).each { |bit_idx|
      flags.push FLAGS[bit_idx] unless (@flags & (1 << bit_idx)) == 0
    }
    result += "Flags: #{flags.join(" ")}\n"
    return result
  end

  def to_s
    result = <<EOF
Z
#{@triggers}~
#{@keywords}~
#{@to_vict}~
#{@to_room}~
#{@to_remote}~
EOF
    result += "#{@command} #{@arg0} #{@arg1} #{@arg2} #{@flags}"
    result += " #{@failure}" if @failure
    result += "\n"
    return result
  end

end

class Roomflow
  attr_accessor :direction, :speed, :type
  def initialize(direction, speed, type)
    @direction = direction
    @speed = speed
    @type = type
  end

  def inspect
    return "Flow (Direction: #{Roomexit::DIRECTIONS[@direction]}, Speed: #{@speed}, Type: #{@type})"
  end

  def to_s
    return "F\n#{@direction} #{@speed} #{@type}\n"
  end
end

class Roomexit

  DIRECTIONS = [ "north", "east", "south", "west", "up", "down",
                 "future", "past" ]

  FLAGS = [ "DOOR", "CLOSED", "LOCKED", "PICKPROOF", "HEAVY",
            "HARD-PICK", "!MOB", "HIDDEN", "!SCAN", "TECH", "ONE-WAY",
            "NOPASS", "THORNS", "THORNS_NOPASS", "STONE", "ICE",
            "FIRE", "FIRE_NOPASS", "FLESH", "IRON", "ENERGY_F",
            "ENERGY_F_NOPASS", "FORCE", "SPECIAL", "REINF", "SECRET" ]

  attr_accessor :direction, :description, :keywords, :exit_info, :key, :to_room
  def initialize(direction)
    @direction = direction
    @description = ""
    @to_room = -1
    @key = -1
    @keywords = ""
    @exit_info = ""
  end

  def inspect
    flags = ""
    if @exit_info == "0"
      flags = " NONE"
    else
      @exit_info.each_byte { |flag|
        flags += " " + FLAGS[flag - ?a]
      }
    end
    result = "Exit %-6s:  To: [%5d], Key: [%5d], Keywrd: %s, Type:%s\n" %
      [ DIRECTIONS[@direction], @to_room, @key, @keywords, flags ]
    result += "Description:\n#{@description}" unless @description.empty?
    return result
  end

  def to_s
    return <<EOF
D#{@direction}
#{@description}~
#{@keywords}~
#{@exit_info} #{@key} #{@to_room}
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

class Room

  SECTORS = [ "Inside", "City", "Field", "Forest", "Hills",
              "Mountains", "Water (Swim)", "Water (No Swim)",
              "Underwater", "Open Air", "Notime", "Climbing",
              "Outer Space", "Road", "Vehicle", "Farmland", "Swamp",
              "Desert", "Fire River", "Jungle", "Pitch Surface",
              "Pitch Submerged", "Beach", "Astral", "Elemental Fire",
              "Elemental Earth", "Elemental Air", "Elemental Water",
              "Elemental Positive", "Elemental Negative",
              "Elemental Smoke", "Elemental Ice", "Elemental Ooze",
              "Elemental Magma", "Elemental Lightning",
              "Elemental Steam", "Elemental Radiance",
              "Elemental Minerals", "Elemental Vacuum",
              "Elemental Salt", "Elemental Ash", "Elemental Dust",
              "Blood", "Rock", "Muddy", "Trail", "Tundra",
              "Catacombs", "Cracked Road", "Deep Ocean" ]

  FLAGS = [ "DRK", "DTH", "!MOB", "IND", "NV", "SDPF", "!TRK", "!MAG",
            "TNL", "!TEL", "GDR", "HAS", "HCR", "COMFORT", "SMOKE",
            "!FLEE", "!PSI", "!SCI", "!RCL", "CLAN", "ARENA", "DOCK",
            "BURN", "FREEZ", "NULLMAG", "HOLYO", "RAD", "SLEEP",
            "EXPLOD", "POISON", "VACUUM" ]

  attr_accessor :idnum, :title, :description, :zone, :flags, :sector,
                :extradescs, :directions, :searches, :flows, :sound

  def initialize()
    @description = ""
    @sound = ""
    @prog = ""
    @special_param = ""
    @extradescs = Array.new
    @searches = Array.new
    @flows = Array.new
    @directions = Hash.new
	@flags = "0"
	@sector = 0
	@occupancy = 255
  end

  def inspect
    # For displaying the contents of a room
    flags = ""
    if @flags == "0"
      flags = " NONE"
    else
      @flags.each_byte { |flag|
        flags += " " + FLAGS[flag - ?a]
      }
    end

    result = <<EOF
Room name: #{@title}
Zone: [#{@zone}], VNum: [#{@idnum}], Type: #{SECTORS[@sector]}, Max: [#{@occupancy}]
Flags: #{flags}
Description:
#{@description}
EOF

    @flows.each { |flow| result += flow.inspect }
    @extradescs.each { |exd| result += exd.inspect }

    (0..5).each { |dir|
      result += @directions[dir].inspect if @directions[dir]
    }

    @searches.each { |search| result += search.inspect }

    result += "Specparam:\n#{@special_param}\n" unless @special_param.empty?
    result += "Prog:\n#{@prog}\n" unless @prog.empty?
    return result
  end

  def to_s
    # Outputs a room in the circle format
    result = <<EOF
##{@idnum}
#{@title}~
#{@description}~
#{@zone} #{@flags} #{@sector}
EOF
    @directions.keys.sort.each { |idx| result += @directions[idx].to_s }
    @extradescs.each { |exd| result += exd.to_s }
    result += "R\n#{@prog}~\n" unless @prog.empty?
    @searches.each { |search| result += search.to_s }
    result += "L\n#{@sound}~\n" unless @sound.empty?
    @flows.each { |flow| result += flow.to_s }
    result += "O #{@occupancy}\n" if @occupancy
    result += "P\n#{@special_param}~\n" unless @special_param.empty?
    result += "S\n"
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

  def load(inf)
    state = :roomnum
    flow = search = extradesc = direction = match = nil
    inf.each { |line|
#      print "Room ", @idnum, " State ", state, "\n"
      line.chomp!
      $linenum += 1
      case state
      when :roomnum
        match = line.match(/^([#\$])(\d+)?/)
        raise "Invalid room number format" unless match
        return false if match[1] == "$"
        @idnum = match[2].to_i
        state = :title
      when :title
        @title = read_tilde_str(line, inf)
        if line.match(/\n/)
          $stderr.print "WARNING: multi-line title '#{line}\n"
        end
        state = :description
      when :description
        @description = read_tilde_str(line, inf)
        state = :flags
      when :flags
        match = line.match(/^(\d+) (\S+) (\d+)/)
        raise "Invalid flags field" unless match
        @zone = match[1].to_i
        @flags = match[2]
        @sector = match[3].to_i
        state = :optional
      when :optional
        case line
        when /^S/
          # End of room
          return true
        when /^O (\d+)/
          @occupancy = $1.to_i
        when /^D(\d+)/
          direction = $1.to_i
          @directions[direction] = Roomexit.new(direction)
          state = :direction_desc
        when /^E/
          state = :extradesc_keywords
        when /^L/
          state = :sound
        when /^F/
          state = :flow
        when /^Z/
          state = :search_command
        when /^R/
          state = :prog
        when /^P/
          state = :special_param
        else
          raise "Invalid optional directive #{line}"
        end
      when :direction_desc
        @directions[direction].description = read_tilde_str(line, inf)
        state = :direction_keywords
      when :direction_keywords
        @directions[direction].keywords = read_tilde_str(line, inf)
        state = :direction_flags
      when :direction_flags
        match = line.match(/^(\S+)\s+([-0-9]+)\s+([-0-9]+)/)
        raise "Dir flags didn't match" unless match
        @directions[direction].exit_info = match[1]
        @directions[direction].key = match[2].to_i
        @directions[direction].to_room = match[3].to_i
        state = :optional
      when :extradesc_keywords
        extradesc = Extradesc.new
        extradesc.keywords = read_tilde_str(line, inf)
        state = :extradesc_descrip
      when :extradesc_descrip
        extradesc.description = read_tilde_str(line, inf)
        @extradescs.push(extradesc)
        state = :optional
      when :sound
        @sound = read_tilde_str(line, inf)
        state = :optional
      when :flow
        match = line.match(/([-0-9]+) ([-0-9]+) ([-0-9]+)/)
        @flows.push(Roomflow.new(match[1].to_i, match[2].to_i, match[3].to_i))
        state = :optional
      when :search_command
        search = Roomsearch.new
        search.triggers = read_tilde_str(line, inf)
        state = :search_keywords
      when :search_keywords
        search.keywords = read_tilde_str(line, inf)
        state = :search_vict
      when :search_vict
        search.to_vict = read_tilde_str(line, inf)
        state = :search_room
      when :search_room
        search.to_room = read_tilde_str(line, inf)
        state = :search_remote
      when :search_remote
        search.to_remote = read_tilde_str(line, inf)
        state = :search_flags
      when :search_flags
        match = line.match(/([-0-9]+) ([-0-9]+) ([-0-9]+) ([-0-9]+) ([-0-9]+)(?: ([-0-9]+))?/)
        search.command = match[1].to_i
        search.arg0 = match[2].to_i
        search.arg1 = match[3].to_i
        search.arg2 = match[4].to_i
        search.flags = match[5].to_i
        search.failure = match[6] if match[6]

        @searches.push(search)
        state = :optional
      when :prog
        @prog = read_tilde_str(line, inf)
        state = :optional
      when :special_param
        @special_param = read_tilde_str(line, inf)
        state = :optional
      end
    }
  end
end
