//
// File: old_reimb.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//


SPECIAL(old_reimb)
{
	extern struct title_type titles[NUM_CLASSES][LVL_GRIMP + 1];
	struct Creature *reimber = (struct Creature *)me;
	int i;
	typedef struct {
		int level;
		char *name;
	} reimb_array;
	reimb_array data[90] = {
		{32, "Phalanx"},
		{42, "Keoria"},
		{32, "Hempy"},
		{35, "Bhaal"},
		{42, "Fee"},
		{29, "Shadow"},
		{45, "DarkAngel"},
		{42, "Bharran"},
		{47, "SPAWN"},
		{43, "Doom"},
		{34, "Cur"},
		{38, "Merlin"},
		{47, "Grim"},
		{46, "Striker"},
		{44, "Pitt"},
		{24, "Tori"},
		{47, "Stone"},
		{50, "Cogline"},
		{45, "Death"},
		{30, "Oaken"},
		{30, "Charon"},
		{47, "KIX"},
		{46, "Nino"},
		{20, "Jojimba"},
		{37, "Stash"},
		{40, "Sune"},
		{27, "Wizard"},
		{52, "Sarflin"},
		{32, "Talon"},
		{40, "Raven"},
		{47, "MEDEVAC"},
		{41, "Blackstaff"},
		{35, "Dark"},
		{33, "Natas"},
		{47, "Vengeance"},
		{40, "Gymll"},
		{33, "Black"},
		{42, "Snarl"},
		{45, "Elric"},
		{44, "Gossamer"},
		{49, "Djiin"},
		{21, "Torous"},
		{27, "Mama"},
		{43, "Brax"},
		{31, "Rad"},
		{40, "BlackDeath"},
		{30, "HomeSkillet"},
		{40, "Rasta"},
		{26, "Convict"},
		{38, "Artemis"},
		{23, "Grawg"},
		{40, "Morgul"},
		{25, "Havoc"},
		{24, "Rikus"},
		{45, "Rolent"},
		{20, "TicK"},
		{20, "Ren"},
		{20, "Corloth"},
		{21, "Walker"},
		{40, "Canon"},
		{17, "Jason"},
		{11, "Rachael"},
		{14, "Freya"},
		{17, "Loric"},
		{12, "Kiodiege"},
		{11, "Mystra"},
		{17, "WHISPER"},
		{12, "Jarovin"},
		{10, "Gwangi"},
		{19, "Pagan"},
		{12, "Cryonax"},
		{10, "HUSKY"},
		{18, "Ke"},
		{10, "Pope"},
		{18, "Demolition"},
		{10, "Simon"},
		{11, "Storm"},
		{15, "Norwil"},
		{14, "Reg"},
		{15, "Ravel"},
		{13, "Kono"},
		{12, "Ironwolf"},
		{10, "Thelin"},
		{15, "Shela"},
		{10, "Sabu"},
		{14, "DarkLord"},
		{19, "Trial"},
		{16, "Argon"},
		{7, "Jaberdawn"},
		{7, "Thud"},
	};

	if (!CMD_IS("help"))
		return 0;

	for (i = 0; i < 90; i++) {
		if (!strncasecmp(data[i].name, GET_NAME(ch), strlen(data[i].name))) {
			if (GET_LEVEL(ch) < 3)
				send_to_char(ch,
					"You'd better train your stats up and get to level 3 first.\r\n");
			else if (GET_LEVEL(ch) >= data[i].level)
				act("$n says, 'Piss off, $N.", FALSE, reimber, 0, ch, TO_ROOM);
			else {
				act("$n whaps you upside the head!", FALSE, reimber, 0, ch,
					TO_VICT);
				act("$n whaps $N upside the head!", FALSE, reimber, 0, ch,
					TO_NOTVICT);
				if (!(data[i].level > 54 || data[i].level < 1))
					gain_exp_regardless(ch,
						titles[(int)GET_CLASS(ch)][data[i].level].exp -
						GET_EXP(ch));
				ch->saveToXML();
				GET_GOLD(ch) = 10000000;
				GET_HIT(ch) = GET_MAX_HIT(ch);
				GET_MANA(ch) = GET_MAX_MANA(ch);
				GET_MOVE(ch) = GET_MAX_MOVE(ch);
			}
			return 1;
		}
	}
	send_to_char(ch,
		"You are not in the file.  All players with level less than 20\r\n"
		"are lost.  Sorry.\r\n");
	return 1;
}
