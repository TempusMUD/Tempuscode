if DEBUG
  DBGFLAG = -g -O0
else
  DBGFLAG = -O3
endif
bin_PROGRAMS = @top_srcdir@/bin/circle
INCLUDES = -I@top_srcdir@/src/include @PQINCPATH@ @XML_CFLAGS@ @GLIB_CFLAGS@
AM_CFLAGS = -Wall -Wcast-qual ${DBGFLAG}
AM_LDFLAGS = @XML_LIBS@ @GLIB_LIBS@
ETAGSFLAGS = --regex='/^ASPELL(\\([a-zA-Z0-9_]+\\))$/\\1/' --regex='/^ACMD(\\([a-zA-Z0-9_]+\\))$/\\1/' --regex='/^DEFPROGHANDLER(\\([a-zA-Z0-9_]+\\),[^)]+)$/prog_do_\\1/' --regex='/^SPECIAL(\\([a-zA-Z0-9_]+\\))$/\\1/'

@top_srcdir@_bin_circle_SOURCES = \
				 clan/clan.c \
				 classes/act.barb.c \
                 classes/act.cleric.c \
                 classes/act.bard.c \
                 classes/act.cyborg.c \
                 classes/act.hood.c \
                 classes/act.knight.c \
                 classes/act.mage.c \
                 classes/act.merc.c \
                 classes/act.monk.c \
                 classes/act.physic.c \
                 classes/act.psionic.c \
                 classes/act.ranger.c \
                 classes/act.thief.c \
                 classes/char_class.c \
                 combat/act.offensive.c \
                 combat/combat_messages.c \
                 combat/combat_utils.c \
                 combat/fight.c \
                 combat/pvp.c \
                 db/account.c \
                 db/obj_matcher.c \
                 db/analysis.c \
                 db/constants.c \
                 db/db.c \
                 db/objsave.c \
                 db/xml.c \
                 dyntext/dyntext.c \
                 editor/boardeditor.c \
                 editor/fileeditor.c \
                 editor/editor.c \
                 editor/maileditor.c \
                 editor/polleditor.c \
                 editor/progeditor.c \
                 editor/texteditor.c \
                 flow_room/flow_room.c \
                 help/help_item.c \
                 help/help_collection.c \
                 house/house.c \
                 interpreter/interpreter.c \
                 interpreter/role.c \
                 interpreter/security.c \
                 magic/magic.c \
                 magic/spell_parser.c \
                 magic/spells.c \
                 misc/act.comm.c \
                 misc/act.informative.c \
                 misc/act.movement.c \
                 misc/act.obj.c \
                 misc/act.other.c \
                 misc/act.wizard.c \
                 misc/config.c \
                 misc/limits.c \
                 misc/login.c \
                 mobiles/artisan.c \
                 mobiles/cityguard.c \
                 mobiles/guard.c \
                 mobiles/mail.c \
                 mobiles/mobact.c \
                 mobiles/tattooist.c \
                 mobiles/vendor.c \
                 mobiles/voice.c \
                 net/gnetsys.c \
                 net/ban.c \
                 net/comm.c \
                 net/nanny.c \
                 objects/boards.c \
                 objects/smokes.c \
                 objects/bomb.c \
                 objects/guns.c \
                 objects/paths.c \
                 objects/vehicle.c \
                 olc/olc_mob.c \
                 olc/olc.c \
                 olc/olc_obj.c \
                 olc/olc_srch.c \
                 olc/olc_wld.c \
                 olc/olc_zon.c \
                 quest/quest.c \
                 search/prog_compile.c \
                 search/prog.c \
                 search/search.c \
                 social/act.social.c \
                 social/language.c \
                 specials/demonic_guard.c \
                 specials/remorter.c \
                 specials/duke.c \
                 specials/hell_hunter_spec.c \
                 specials/spec_assign.c \
                 specials/spec_procs.c \
                 specials/styx.c \
                 structs/creature.c \
                 structs/creature_io.c \
                 structs/obj_data.c \
                 structs/players.c \
                 structs/room_data.c \
                 util/accstr.c \
                 util/handler.c \
                 util/graph.c \
                 util/modify.c \
                 util/random.c \
                 util/sight.c \
                 util/tmpstr.c \
                 util/utils.c \
                 weather/weather.c
