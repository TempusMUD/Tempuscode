//
// File: reimb.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//


SPECIAL(reimb)
{
	extern struct title_type titles[NUM_CLASSES][LVL_GRIMP + 1];
	struct Creature *reimber = (struct Creature *)me;
	int i;
	typedef struct {
		int level;
		char *name;
		char *pwd;
		int gold;
		int bank;
		int str;
		int str_add;
		int intel;
		int wis;
		int dex;
		int con;
		int cha;
	} reimb_array;
	reimb_array data[] = {

		{46, "Elric", "ElEAvJpYWb", 760146, 8840579, 18, 40, 18, 16, 18, 18,
				8},
		{14, "Jstraw", "Jsz.UWB/y/", 150315, 8840579, 16, 0, 13, 16, 16, 17,
				10},
		{16, "Halfstep", "HaPYbxley1", 113112, 8790579, 15, 0, 14, 17, 18, 14,
				11},
		{18, "Repeater", "Rebj8uWJsu", 833280, 8330579, 11, 0, 18, 18, 18, 17,
				10},
		{46, "Striker", "StWPcaWI7.", 0, 8840579, 18, 20, 19, 18, 16, 16, 18},
		{43, "Doom", "DoJKjDNIe4", 92788, 8840579, 18, 100, 18, 18, 15, 16,
				11},
		{45, "Death", "De2y5nFmBc", 93988, 8840579, 18, 60, 16, 14, 17, 16,
				12},
		{25, "Muse", "MuAqK/Zolq", 74779, 8840579, 18, 0, 18, 18, 18, 18, 15},
		{11, "Mo", "Mo.9Q7j3JF", 93988, 8840579, 18, 0, 18, 18, 18, 16, 10},
		{19, "Pagan", "PaXByUieXA", 19310, 8916579, 17, 0, 11, 11, 14, 16, 6},
		{35, "Dirk", "Dij4TyxDAu", 93988, 8840579, 12, 0, 18, 15, 19, 11, 13},
		{18, "Rachael", "RaeytuRy/S", 205324, 8840579, 16, 0, 18, 14, 12, 13,
				10},
		{2, "Traxus", "TrMSYKGHhz", 332, 0, 17, 0, 10, 11, 15, 16, 5},
		{39, "Wizard", "WiCfRNsNHQ", 105651, 8840579, 18, 0, 16, 18, 18, 17,
				7},
		{21, "Ironwolf", "IrnZqu2QRX", 147306, 8840579, 18, 87, 15, 16, 18, 16,
				9},
		{2, "Halo", "HajBjXyELW", 10069, 0, 12, 0, 17, 16, 12, 16, 9},
		{49, "ClERic", "ClI3zYEZ/N", 9400, 8840579, 18, 50, 18, 18, 18, 17,
				10},
		{8, "Lars", "LaUELejVVP", 4165, 9010279, 18, 10, 15, 11, 18, 14, 9},
		{5, "Rex", "Rebd/dFmKL", 10000, 0, 16, 0, 17, 18, 15, 18, 14},
		{42, "Rad", "RawP/AWhvt", 57286, 8840579, 17, 0, 18, 16, 18, 13, 9},
		{5, "Eldritch", "ElcrImH7Av", 487, 0, 17, 0, 17, 18, 16, 18, 12},
		{45, "Brian", "Br.Bl7j84M", 692725, 8840579, 18, 60, 18, 16, 18, 16,
				11},
		{4, "Mindcrime", "MiXyv9cf/a", 93949, 8840579, 18, 60, 14, 14, 14, 16,
				6},
		{7, "Desami", "DetZSWOFnw", 102365, 0, 18, 20, 17, 18, 16, 18, 17},
		{37, "Jaberdawn", "Jaj6jT/Tet", 124811, 8740579, 17, 0, 11, 13, 14, 16,
				6},
		{12, "Conn", "CoauJ4c5LF", 93980, 8840579, 18, 30, 16, 16, 17, 18, 7},
		{16, "Kalarr", "Ka4k93kfNh", 247186, 8840579, 12, 0, 18, 16, 18, 14,
				12},
		{41, "Canon", "CaeHtKIavU", 93988, 8840579, 18, 20, 18, 18, 18, 18, 9},
		{40, "Rikus", "RirtdZ8nLS", 93705, 8840579, 18, 100, 17, 16, 17, 18,
				1},
		{5, "Yes", "YePFOaHhgh", 0, 0, 14, 0, 18, 18, 19, 12, 10},
		{19, "Pete", "Pe5vriHxXh", 100437, 8840579, 18, 20, 9, 8, 16, 16, 6},
		{45, "Brax", "BrNlvxRk7e", 93988, 8840579, 18, 100, 9, 16, 19, 18, 9},
		{40, "Raven", "RaD7h8spab", 360881, 9200000, 11, 0, 15, 16, 19, 17,
				11},
		{35, "Dirk", "Dij4TyxDAu", 229875, 1000000, 12, 0, 18, 15, 19, 11, 13},
		{17, "Jason", "JaIcyB1Z27", 667330, 7375208, 18, 0, 13, 15, 19, 17, 9},
		{46, "Elric", "ElEAvJpYWb", 1666041, 32560600, 18, 40, 18, 16, 18, 18,
				8},
		{40, "Canon", "CaeHtKIavU", 1000000, 8500492, 18, 20, 18, 18, 18, 18,
				9},
		{47, "SPAWN", "SPTfNE5QY3", 1016862, 12250000, 18, 0, 14, 18, 18, 18,
				9},
		{40, "Sune", "SuOdaeuvm3", 95685, 7200278, 18, 30, 10, 14, 13, 17, 3},
		{42, "Rad", "RawP/AWhvt", 145271, 9750000, 17, 0, 18, 16, 18, 13, 9},
		{3, "Worf", "WoqS6rLl5G", 108, 0, 16, 0, 8, 9, 11, 15, 7},
		{40, "Rikus", "RirtdZ8nLS", 537266, 9000000, 18, 100, 17, 16, 17, 18,
				1},
		{3, "Beast", "Bes6XbHjFH", 153, 0, 17, 0, 18, 18, 16, 18, 12},
		{41, "Blackstaff", "BlR0kAfCTD", 9991692, 0, 9, 0, 18, 18, 14, 18, 11},
		{40, "Gymll", "Gy.6Gyh5I0", 0, 0, 18, 60, 11, 18, 17, 14, 7},
		{42, "Snarl", "SnGMkvRP7n", 8511, 50, 18, 70, 9, 15, 16, 15, 8},
		{42, "Bharran", "BhtsreehxC", 0, 0, 14, 0, 11, 18, 17, 18, 9},
		{3, "Tym", "TyRGOs4kgi", 0, 0, 13, 0, 16, 14, 14, 9, 12},
		{32, "Talon", "TaIJUe2xM5", 0, 10000000, 18, 0, 12, 18, 18, 13, 10},
		{17, "Rachael", "RaeytuRy/S", 184010, 4700000, 16, 0, 18, 14, 12, 13,
				10},
		{7, "Dahrinn", "DaM7SW35B1", 25026, 200000, 18, 10, 16, 19, 16, 13, 9},
		{47, "KIX", "KIY1Sg4C9U", 79474, 6939950, 17, 0, 16, 16, 17, 18, 13},
		{40, "Disco", "DiqOcCkQ7G", 210731, 2615000, 14, 0, 16, 18, 17, 18, 4},
		{45, "DarkAngel", "Datw0rHgsj", 2080689, 0, 18, 100, 18, 18, 19, 18,
				8},
		{35, "Shadow", "Sh7D2.Us7I", 77165220, 0, 18, 100, 18, 18, 19, 18, 11},
		{47, "Stone", "St6icV8zA/", 131800, 9271614, 18, 95, 18, 18, 17, 15,
				13},
		{21, "Ironwolf", "IrnZqu2QRX", 239852, 9965404, 18, 87, 15, 16, 18, 16,
				9},
		{47, "Grim", "GrTKq93oLz", 3195205, 24500000, 18, 100, 11, 16, 17, 17,
				6},
		{39, "Wizard", "WiCfRNsNHQ", 5021740, 100000, 18, 0, 16, 18, 18, 17,
				7},
		{47, "MEDEVAC", "MEkCZE1eEd", 1522425, 13000000, 18, 40, 18, 18, 18,
				18, 6},
		{46, "Striker", "StWPcaWI7.", 981189, 29000000, 18, 20, 19, 18, 16, 16,
				18},
		{11, "Britnae", "BrCn/QbFvj", 46665, 500, 7, 0, 17, 15, 12, 9, 9},
		{4, "Skogin", "SkhUbCvGXZ", 0, 0, 18, 40, 11, 16, 14, 18, 10},
		{47, "Vengeance", "VefaM.WulX", 53622, 7500000, 18, 20, 12, 18, 19, 13,
				11},
		{11, "Mystra", "MyZ88u5ciu", 291911, 5450000, 11, 0, 15, 16, 16, 18,
				12},
		{49, "Djiin", "DjD7jedGvt", 0, 75000, 18, 20, 18, 18, 16, 18, 9},
		{16, "Argon", "ArSAF9Fmo/", 184189, 9700000, 18, 20, 18, 18, 18, 16,
				14},
		{12, "Havoc", "Ha/ZRf1WOS", 180670, 150000, 18, 100, 14, 18, 18, 18,
				11},
		{4, "Logan", "LoDtvXxRHw", 100000000, 0, 18, 30, 16, 18, 18, 18, 13},
		{18, "Demolition", "DePdQpLj5M", 9969330, 0, 18, 100, 18, 18, 18, 18,
				9},
		{45, "Rolent", "RoEKHygDHG", 0, 1700000, 17, 0, 11, 9, 18, 18, 8},
		{40, "Turtle", "TuCGDYQe/V", 120858, 1325000, 13, 0, 19, 18, 15, 18,
				14},
		{42, "Keoria", "Ke4KUj8s2n", 128679, 6500000, 17, 0, 15, 11, 15, 18,
				10},
		{3, "Laertes", "LaDaApZgQ7", 100000000, 0, 9, 0, 18, 18, 16, 18, 12},
		{10, "Yip", "YijCZjNT2s", 167, 0, 10, 0, 18, 14, 15, 12, 13},
		{19, "Pagan", "PaXByUieXA", 9999655, 0, 17, 0, 11, 11, 14, 16, 6},
		{4, "Perrin", "PeDmZCtPf8", 4405, 60000, 18, 0, 17, 16, 14, 10, 10},
		{9, "SMILEY", "SM9Jm3SYaC", 1985562, 312791, 18, 70, 13, 13, 18, 18,
				10},
		{26, "Convict", "CoohMKJNQF", 10000678, 0, 18, 10, 16, 16, 19, 17, 9},
		{44, "Gossamer", "Goi3AzLh.J", 1553409, 5000000, 18, 20, 13, 15, 18,
				14, 10},
		{2, "Desami", "DetZSWOFnw", 72, 0, 14, 0, 17, 18, 15, 16, 11},
		{17, "Whisper", "Whu4//qrSE", 10005794, 0, 18, 10, 15, 15, 17, 18, 4},
		{24, "Tori", "To7QcnKvq.", 153491, 9900000, 18, 0, 16, 16, 18, 14, 9},
		{5, "Bones", "Bo/Whvuigh", 759706, 0, 18, 0, 13, 18, 16, 12, 8},
		{23, "Gabriel", "Gaey7CRxPF", 700240, 1360000, 18, 20, 12, 14, 19, 18,
				8},
		{8, "Odin", "OdxhpgDH5W", 967283, 3000000, 18, 50, 14, 14, 17, 18, 9},
		{3, "Niamh", "NisS2IhJeI", 561540, 0, 18, 50, 16, 16, 18, 18, 9},
		{20, "Crasher", "CrGpBAGKnS", 151, 0, 8, 0, 16, 15, 14, 11, 12},
		{45, "Brian", "Br.Bl7j84M", 138544, 4500000, 18, 60, 18, 16, 18, 16,
				11},
		{3, "Lorki", "Lo5vIyq2ke", 20, 0, 18, 20, 11, 11, 14, 18, 8},
		{9, "Concor", "CohDTKb5IP", 762870, 0, 18, 0, 18, 19, 18, 18, 11},
		{19, "Pete", "Pe5vriHxXh", 154566, 1768000, 18, 20, 9, 8, 16, 16, 6},
		{45, "Rasta", "RaeaPqGLR9", 9967343, 0, 18, 10, 19, 18, 18, 18, 15},
		{2, "Arthur", "ArCCwYztCD", 113, 0, 18, 40, 18, 13, 14, 15, 12},
		{14, "Hawk", "HaFnvkNDw6", 532571, 300000, 11, 0, 17, 14, 14, 11, 12},
		{3, "Shirah", "Sh9gZ4z0ka", 200878, 0, 15, 0, 18, 14, 13, 14, 11},
		{41, "An", "AnvvZIhiCP", 1267366, 2900000, 18, 20, 14, 15, 18, 17, 10},
		{5, "Ugh", "Ug4PrV.YgV", 120, 0, 18, 60, 10, 13, 15, 16, 9},
		{32, "Simon", "SiFl1TjFMx", 9710850, 0, 18, 100, 18, 18, 19, 18, 11},
		{35, "Bhaal", "BhtsreehxC", 9970597, 0, 10, 0, 18, 17, 14, 18, 12},
		{25, "Muse", "MuAqK/Zolq", 7808669, 0, 18, 0, 18, 18, 18, 18, 15},
		{3, "Lionheart", "LiIhmzcClK", 143, 0, 15, 0, 15, 15, 18, 14, 12},
		{10, "Prometheus", "Pr3zY.5e10", 119350, 180000, 18, 0, 14, 14, 12, 16,
				4},
		{22, "Seren", "Se/wna04mZ", 208453, 0, 18, 0, 16, 16, 15, 16, 13},
		{12, "Conn", "CoauJ4c5LF", 217105, 21000000, 18, 30, 16, 16, 17, 18,
				7},
		{44, "Pitt", "PiKjNL.NvL", 90000, 9910000, 18, 60, 11, 15, 16, 17, 7},
		{9, "Caine", "Cae4PwJ03a", 32068, 8105000, 18, 50, 18, 18, 14, 18, 8},
		{3, "MistyNyte", "MixNq1OqM9", 0, 0, 11, 0, 13, 14, 11, 8, 7},
		{5, "Jean", "JetpN4LTPj", 10167, 0, 12, 0, 15, 14, 11, 8, 9},
		{7, "Snail", "SnlGWKI2/T", 95794, 9900000, 18, 73, 18, 18, 18, 18, 16},
		{3, "Life", "Liz.h3pU1U", 0, 0, 16, 0, 13, 14, 16, 14, 11},
		{3, "Maquab", "MabdCJCNGK", 100000, 0, 6, 0, 17, 15, 12, 8, 10},
		{11, "Blade", "BlqTRfvvG7", 0, 100000, 18, 20, 18, 18, 17, 18, 7},
		{37, "Jaberdawn", "Jaj6jT/Tet", 50169, 100000, 17, 0, 11, 13, 14, 16,
				6},
		{7, "Ping", "PiD1l8bhPg", 193280, 0, 13, 0, 16, 18, 12, 12, 5},
		{13, "Repeater", "Rebj8uWJsu", 26456, 776194, 11, 0, 18, 18, 18, 17,
				10},
		{8, "Kelle", "KefH.N4Pjp", 0, 30000, 9, 0, 18, 18, 13, 9, 11},
		{4, "Help", "HeyBtuGpfO", 206723, 750000, 18, 0, 12, 11, 18, 16, 6},
		{10, "Photon", "Ph7M7Wp0JR", 966098, 0, 18, 0, 14, 10, 19, 14, 9},
		{11, "Kalarr", "Ka4k93kfNh", 638179, 24900000, 12, 0, 18, 16, 18, 14,
				12},
		{33, "BlackDeath", "BlDbTZPJmI", 9925165, 0, 18, 100, 18, 18, 18, 18,
				5},
		{11, "JStraw", "JS.iLwzmhk", 13516, 0, 16, 0, 13, 16, 16, 17, 10},
		{7, "Fatsnaps", "Fa5jS5t3d4", 1054, 0, 18, 20, 11, 17, 14, 18, 9},
		{9, "Sparrow", "SpSUEPYQCl", 56042, 0, 18, 30, 15, 17, 18, 12, 11},
		{5, "Mobius", "MowqKAT9DF", 1619, 0, 18, 20, 18, 18, 13, 12, 12},
		{4, "Vinitar", "ViVX/519tb", 1000200, 0, 18, 10, 18, 18, 12, 11, 9},
		{5, "Kheldar", "KhXJ7CAbLY", 1005145, 0, 18, 20, 18, 17, 19, 16, 7},
		{3, "Jarek", "Ja001vTw6G", 1998137, 0, 14, 0, 18, 17, 18, 15, 5},
		{5, "Zodiac", "ZoD0RvW4Z6", 115666, 0, 17, 0, 18, 18, 19, 18, 7},
		{3, "Mindcrime", "MiXyv9cf/a", 432, 0, 18, 20, 11, 12, 14, 16, 6},
		{9, "DarkHeart", "DaPQNri1Lk", 88282, 0, 18, 30, 11, 14, 16, 17, 9},
		{5, "Lars", "LaUELejVVP", 16861, 6400, 18, 10, 15, 11, 18, 14, 9},
		{41, "Cull", "CugBJdZjWI", 125, 0, 11, 0, 18, 18, 16, 18, 15},
		{6, "Fuzzy", "FupA.6ZNKD", 2215, 3000, 16, 0, 12, 18, 13, 18, 8},
		{41, "Urg", "UrbtJ2z8J5", 0, 0, 18, 20, 11, 16, 16, 18, 9},
		{6, "DarkBlade", "DaFrAvSt7g", 10899, 100000, 18, 0, 9, 10, 12, 17, 8},
		{42, "Fee", "FevKfizYvc", 8816895, 0, 18, 50, 18, 18, 18, 18, 10},
		{2, "Jimbalaya", "Ji/ssKHpIU", 97818, 0, 11, 0, 11, 13, 17, 13, 8},
		{3, "Joe", "JoVZvtMmG9", 0, 0, 15, 0, 18, 14, 16, 17, 9},
		{5, "Aragorn", "ArXF.C6tmN", 2466, 0, 16, 0, 18, 18, 16, 17, 10},
		{3, "Kip", "KifKenFju2", 0, 0, 18, 10, 14, 18, 12, 15, 10},
		{6, "Phallus", "PhFB1gCW8a", 0, 0, 17, 0, 15, 16, 18, 13, 12},
		{7, "StokerAce", "St61OTsY5C", 83598, 0, 15, 0, 18, 18, 18, 14, 7},
		{11, "Mo", "MoJJsSQq6X", 72770, 30000, 18, 0, 18, 18, 18, 16, 10},
		{12, "Halfstep", "HaPYbxley1", 17098, 55000, 15, 0, 14, 17, 18, 14,
				11},
		{8, "Targa", "TagYS64oIB", 116699, 0, 17, 0, 9, 11, 18, 18, 5},
		{4, "Blackbird", "BlEW1wWBoK", 330712, 0, 18, 20, 15, 15, 14, 14, 11},
		{5, "Tor", "TouR0ZEbG.", 1467, 0, 18, 0, 18, 18, 16, 16, 3},
		{5, "Said", "Sa8OAKWARq", 10099, 0, 18, 20, 18, 18, 17, 18, 7},
		{6, "Jeremy", "Je0h9.Bx9d", 6616, 0, 18, 0, 18, 18, 17, 18, 8},
		{49, "ClERic", "ClI3zYEZ/N", 1341500, 0, 18, 50, 18, 18, 18, 17, 10},
		{43, "Doom", "DoJKjDNIe4", 144895, 0, 18, 100, 18, 18, 15, 16, 11},
		{45, "Death", "De2y5nFmBc", 0, 0, 18, 60, 16, 14, 17, 16, 12},
		{13, "NaTaLiE", "Nama9rlhpi", 100000, 0, 18, 60, 18, 18, 19, 18, 16},
		{12, "Kv", "KvOkDWaFSs", 2971, 100000, 18, 20, 18, 18, 18, 18, 11},
		{-1, "", "", 0, 0, 0, 0, 0, 0, 0, 0, 0}
	};

	if (!CMD_IS("help"))
		return 0;

	one_argument(argument, arg);

	for (i = 0; data[i].level > 0; i++) {
		if (!strncasecmp(data[i].name, GET_NAME(ch), strlen(data[i].name))) {
			if (GET_LEVEL(ch) >= data[i].level)
				act("$n says, 'Piss off, $N.'", FALSE, reimber, 0, ch,
					TO_ROOM);
			else if (!*arg)
				send_to_char(ch,
					"Type HELP <old password> for reimburse.\r\n");
			else if (strncmp(CRYPT(arg, data[i].pwd), data[i].pwd,
					MAX_PWD_LENGTH)) {
				send_to_char(ch, "Bad password.\r\n");
				slog("Bad REIMB pwd attempt for %s from %s.", GET_NAME(ch),
					ch->desc->host);
			} else {
				act("$n whaps you upside the head!", FALSE, reimber, 0, ch,
					TO_VICT);
				act("$n whaps $N upside the head!", FALSE, reimber, 0, ch,
					TO_NOTVICT);
				ch->real_abils.str = data[i].str;
				ch->real_abils.str_add = data[i].str_add;
				ch->real_abils.intel = data[i].intel;
				ch->real_abils.wis = data[i].wis;
				ch->real_abils.dex = data[i].dex;
				ch->real_abils.con = data[i].con;
				ch->real_abils.cha = data[i].cha;
				if (!(data[i].level > 49 || data[i].level < 1))
					gain_exp_regardless(ch,
						titles[(int)GET_CLASS(ch)][data[i].level].exp -
						GET_EXP(ch));
				save_char(ch, NULL);
				GET_GOLD(ch) = data->gold;
				GET_BANK_GOLD(ch) = data->bank;
				GET_HIT(ch) = GET_MAX_HIT(ch);
				GET_MANA(ch) = GET_MAX_MANA(ch);
				GET_MOVE(ch) = GET_MAX_MOVE(ch);
			}
			return 1;
		}
	}
	send_to_char(ch,
		"You are not in the file.  All players with level less than 2\r\n"
		"are lost.  Sorry.\r\n");
	return 1;
}
