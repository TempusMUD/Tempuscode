
#ifndef SETTING
#error This file meant to be included from act.setting.c, not compiled on its own.
#endif

SETTING(allow_clanmail, NULL, ONOFF(!PLR_FLAGGED(ch, PLR_NOCLANMAIL)), set_pflag_invbit(ch, val, 1, PLR_NOCLANMAIL),
        "Allow mail to the clan to be sent to you.")

SETTING(allow_hassle, "OLC", ONOFF(!PRF_FLAGGED(ch, PRF_NOHASSLE)), set_pref_invbit(ch, val, 1, PRF_NOHASSLE),
        "Allow mobs to attack you unprovoked.")

SETTING(allow_pkilling, NULL, ONOFF(PRF2_FLAGGED(ch, PRF2_PKILLER)), set_pref_bit(ch, val, 2, PRF2_PKILLER),
        "Allow yourself to attack other players.")

SETTING(allow_snoop, "WizardFull", ONOFF(!PRF_FLAGGED(ch, PRF_NOSNOOP)), set_pref_invbit(ch, val, 1, PRF_NOSNOOP),
        "Allow yourself to be snooped by gods.")

SETTING(channel_auction, NULL, ONOFF(!PRF_FLAGGED(ch, PRF_NOAUCT)), set_pref_invbit(ch, val, 1, PRF_NOAUCT),
        "Hear the auction channel.")

SETTING(channel_clan, NULL, ONOFF(!PRF_FLAGGED(ch, PRF_NOCLANSAY)), set_pref_invbit(ch, val, 1, PRF_NOCLANSAY),
        "Hear the channel of your clan (if any)")

SETTING(channel_dream, NULL, ONOFF(!PRF_FLAGGED(ch, PRF_NODREAM)), set_pref_invbit(ch, val, 1, PRF_NODREAM),
        "Hear the dreams of others")

SETTING(channel_gossip, NULL, ONOFF(!PRF_FLAGGED(ch, PRF_NOGOSS)), set_pref_invbit(ch, val, 1, PRF_NOGOSS),
        "Hear the player gossip")

SETTING(channel_grats, NULL, ONOFF(!PRF_FLAGGED(ch, PRF_NOGRATZ)), set_pref_invbit(ch, val, 1, PRF_NOGRATZ),
        "Hear the congratulations of players")

SETTING(channel_guild, NULL, ONOFF(!PRF2_FLAGGED(ch, PRF2_NOGUILDSAY)), set_pref_invbit(ch, val, 2, PRF2_NOGUILDSAY),
        "Hear the rumors of your guild")

SETTING(channel_haggle, NULL, ONOFF(!PRF_FLAGGED(ch, PRF_NOHAGGLE)), set_pref_invbit(ch, val, 1, PRF_NOHAGGLE),
        "Hear the haggling of players")

SETTING(channel_holler, "OLC", ONOFF(!PRF_FLAGGED(ch, PRF2_NOHOLLER)), set_pref_invbit(ch, val, 2, PRF2_NOHOLLER),
        "Hear the hollers of players")

SETTING(channel_imm, "OLC", ONOFF(!PRF2_FLAGGED(ch, PRF2_NOIMMCHAT)), set_pref_invbit(ch, val, 2, PRF2_NOIMMCHAT),
        "Hear the chat of the immortals")

SETTING(channel_sing, NULL, ONOFF(!PRF_FLAGGED(ch, PRF_NOMUSIC)), set_pref_invbit(ch, val, 1, PRF_NOMUSIC),
        "Hear the songs of your people")

SETTING(channel_newbie, NULL, ONOFF(PRF2_FLAGGED(ch, PRF2_NEWBIE_HELPER)), set_pref_bit(ch, val, 2, PRF2_NEWBIE_HELPER),
        "Hear the questions of the newbies")

SETTING(channel_petition, "OLC", ONOFF(!PRF_FLAGGED(ch, PRF_NOPETITION)), set_pref_invbit(ch, val, 1, PRF_NOPETITION),
        "Hear the petitions of the mortals")

SETTING(channel_plug, NULL, ONOFF(!PRF_FLAGGED(ch, PRF_NOPLUG)), set_pref_invbit(ch, val, 1, PRF_NOPLUG),
        "Hear the criers plug TempusMUD")

SETTING(channel_project, NULL, ONOFF(!PRF_FLAGGED(ch, PRF_NOPROJECT)), set_pref_invbit(ch, val, 1, PRF_NOPROJECT),
        "Hear the projections of remorts")

SETTING(channel_spew, NULL, ONOFF(!PRF_FLAGGED(ch, PRF_NOSPEW)), set_pref_invbit(ch, val, 1, PRF_NOSPEW),
        "Hear the spewing of other players")

SETTING(channel_wiz, "WizardBasic", ONOFF(!PRF_FLAGGED(ch, PRF_NOWIZ)), set_pref_invbit(ch, val, 1, PRF_NOWIZ),
        "Hear the chat of the wizards")

SETTING(display_autoexits, NULL, ONOFF(PRF_FLAGGED(ch, PRF_AUTOEXIT)), set_pref_bit(ch, val, 1, PRF_AUTOEXIT),
        "Display obvious exits in rooms")

SETTING(display_verbose, NULL, ONOFF(!PRF_FLAGGED(ch, PRF_BRIEF)), set_pref_invbit(ch, val, 1, PRF_BRIEF),
        "Display room descriptions on entrance")

SETTING(display_channels, NULL, ONOFF(!PRF_FLAGGED(ch, PRF_DEAF)), set_pref_invbit(ch, val, 1, PRF_DEAF),
        "Display or hide all channels")

SETTING(display_damage, NULL, ONOFF(PRF2_FLAGGED(ch, PRF2_SHOW_DAM)), set_pref_bit(ch, val, 2, PRF2_SHOW_DAM),
        "Display numeric damage during combat")

SETTING(display_units, NULL,
        get_pref_bool(USE_METRIC(ch), "metric", "imperial"),
        set_pref_bool(&ch->account->metric_units, val, "metric", "imperial"),
        "Display measurement units (account setting)")

SETTING(display_misses, NULL, ONOFF(!PRF_FLAGGED(ch, PRF_GAGMISS)), set_pref_invbit(ch, val, 1, PRF_GAGMISS),
        "Display misses in combat")

SETTING(display_nasty, NULL, ONOFF(PRF_FLAGGED(ch, PRF_NASTY)), set_pref_bit(ch, val, 1, PRF_NASTY),
        "Display the nasty words of other players")

SETTING(display_newbies, NULL, ONOFF(PRF2_FLAGGED(ch, PRF2_NEWBIE_HELPER)), set_pref_bit(ch, val, 2, PRF2_NEWBIE_HELPER),
        "Display when new characters enter the game")

SETTING(display_roomflags, "OLC", ONOFF(PRF_FLAGGED(ch, PRF_ROOMFLAGS)), set_pref_bit(ch, val, 1, PRF_ROOMFLAGS),
        "Display room flags")

SETTING(display_scoreaffects, NULL, ONOFF(PRF2_FLAGGED(ch, PRF2_NOAFFECTS)), set_pref_bit(ch, val, 2, PRF2_NOAFFECTS),
        "Display affects in score")

SETTING(display_syslog, "OLC", get_pref_syslog(ch), set_pref_syslog(ch, val), "Display syslog")

SETTING(display_tells, NULL, ONOFF(!PRF_FLAGGED(ch, PRF_NOTELL)), set_pref_invbit(ch, val, 1, PRF_NOTELL),
        "Display tells from other players")

SETTING(display_trailers, NULL, ONOFF(!PRF2_FLAGGED(ch, PRF2_NOTRAILERS)), set_pref_invbit(ch, val, 2, PRF2_NOTRAILERS),
        "Display affect trailers in room description")

SETTING(display_vnums, "OLC", ONOFF(PRF2_FLAGGED(ch, PRF2_DISP_VNUMS)), set_pref_bit(ch, val, 2, PRF2_DISP_VNUMS),
        "Display object and mob vnums")

SETTING(display_gechos, "OLC", ONOFF(!PRF2_FLAGGED(ch, PRF2_NOGECHO)), set_pref_bit(ch, val, 2, PRF2_NOGECHO),
        "Display gecho silliness")

SETTING(pref_autoloot, NULL, ONOFF(PRF2_FLAGGED(ch, PRF2_AUTOLOOT)), set_pref_bit(ch, val, 2, PRF2_AUTOLOOT),
        "Automatically loot money from what you kill")

SETTING(pref_autosplit, NULL, ONOFF(PRF2_FLAGGED(ch, PRF2_AUTOSPLIT)), set_pref_bit(ch, val, 2, PRF2_AUTOSPLIT),
        "Automatically split money across the group")

SETTING(pref_debug, "OLC", ONOFF(PRF2_FLAGGED(ch, PRF2_DEBUG)), set_pref_bit(ch, val, 2, PRF2_DEBUG),
        "Turn on debug mode")

SETTING(pref_holylight, "OLC", ONOFF(PRF_FLAGGED(ch, PRF_HOLYLIGHT)), set_pref_bit(ch, val, 1, PRF_HOLYLIGHT),
        "Everything is visible to you")

SETTING(pref_mortalized, "WizardFull", ONOFF(PLR_FLAGGED(ch, PLR_MORTALIZED)), set_pflag_bit(ch, val, 1, PLR_MORTALIZED),
        "Allow yourself to be a mere mortal.")

SETTING(pref_poofin, "OLC", get_pref_str(POOFIN(ch)), set_pref_str(ch, val, &POOFIN(ch)),
        "Message emitted when you transport yourself into a room.")

SETTING(pref_poofout, "OLC", get_pref_str(POOFOUT(ch)), set_pref_str(ch, val, &POOFOUT(ch)),
        "Message emitted when you transport yourself out of a room.")

SETTING(pref_wimpy, NULL,
        get_pref_int(ch->player_specials->saved.wimp_level),
        set_pref_int(&ch->player_specials->saved.wimp_level, val, 0, GET_MAX_HIT(ch)/2),
        "Automatically flee if damaged below these hitpoints")

SETTING(pref_worldwrite, "OLCWorldWrite", ONOFF(PRF2_FLAGGED(ch, PRF2_WORLDWRITE)), set_pref_bit(ch, val, 2, PRF2_WORLDWRITE),
        "Enable worldwrite")

SETTING(prompt_autodiagnose, NULL, ONOFF(PRF2_FLAGGED(ch, PRF2_AUTO_DIAGNOSE)), set_pref_bit(ch, val, 2, PRF2_AUTO_DIAGNOSE),
        "Display wounded status of opponents")

SETTING(prompt_autoprompt, NULL, ONOFF(PRF_FLAGGED(ch, PRF2_AUTOPROMPT)), set_pref_bit(ch, val, 2, PRF2_AUTOPROMPT),
        "Show prompt after any output")

SETTING(prompt_hitpoints, NULL, ONOFF(PRF_FLAGGED(ch, PRF_DISPHP)), set_pref_bit(ch, val, 1, PRF_DISPHP),
        "Show hitpoints on prompt")

SETTING(prompt_mana, NULL, ONOFF(PRF_FLAGGED(ch, PRF_DISPMANA)), set_pref_bit(ch, val, 1, PRF_DISPMANA),
        "Show mana on prompt")

SETTING(prompt_move, NULL, ONOFF(PRF_FLAGGED(ch, PRF_DISPMOVE)), set_pref_bit(ch, val, 1, PRF_DISPMOVE),
        "Show move on prompt")

SETTING(prompt_alignment, NULL, ONOFF(PRF2_FLAGGED(ch, PRF2_DISPALIGN)), set_pref_bit(ch, val, 2, PRF2_DISPALIGN),
        "Show alignment on prompt")

SETTING(prompt_localtime, NULL, ONOFF(PRF2_FLAGGED(ch, PRF2_DISPTIME)), set_pref_bit(ch, val, 2, PRF2_DISPTIME),
        "Show local game time on prompt")

SETTING(screen_color, NULL,
        get_pref_enum(ch->account->ansi_level, ansi_levels),
        set_pref_enum(&ch->account->ansi_level, val, ansi_levels),
        "Color level (account setting)")

SETTING(screen_compact, NULL,
        get_pref_enum(ch->account->compact_level, compact_levels),
        set_pref_enum(&ch->account->compact_level, val, compact_levels),
        "Compact level (account setting)")

SETTING(screen_height, NULL,
        get_pref_uint(ch->account->term_height),
        set_pref_uint(&ch->account->term_height, val, 1, 1000),
        "Height of terminal used for paging (account setting)")

SETTING(screen_width, NULL,
        get_pref_uint(ch->account->term_width),
        set_pref_uint(&ch->account->term_width, val, 1, 1000),
        "Width of terminal used for wordwrap (account setting)")

SETTING(wholist_anonymous, NULL, ONOFF(PRF2_FLAGGED(ch, PRF2_ANONYMOUS)), set_pref_bit(ch, val, 2, PRF2_ANONYMOUS),
        "Hide level and class from who list")

SETTING(wholist_clanhide, NULL, ONOFF(PRF2_FLAGGED(ch, PRF2_CLAN_HIDE)), set_pref_bit(ch, val, 2, PRF2_CLAN_HIDE),
        "Hide your clan on the wholist")
