#ifndef _MOBACT_H_
#define _MOBACT_H_

//
// File: mobact.h                      -- Part of TempusMUD
//
// All modifications and additions are
// Copyright 1998 by John Watson, all rights reserved.
//

struct Creature *choose_opponent(struct Creature *ch,
	struct Creature *ignore_vict);
int mobile_battle_activity(struct Creature *ch,
	struct Creature *previous_vict);

int mob_fight_slaad(struct Creature *ch, struct Creature *precious_vict);
int mob_fight_devil(struct Creature *ch, struct Creature *precious_vict);
int mob_fight_celestial(struct Creature *ch, struct Creature *precious_vict);
int mob_fight_guardinal(struct Creature *ch, struct Creature *precious_vict);
int mob_fight_demon(struct Creature *ch, struct Creature *precious_vict);
void knight_activity(Creature * ch);
bool knight_battle_activity(Creature * ch, Creature * precious_vict);
void ranger_activity(Creature *ch);
bool ranger_battle_activity(Creature * ch, Creature *precious_vict);
void barbarian_activity(Creature *ch);
bool barbarian_battle_activity(Creature * ch, Creature *precious_vict);

void psionic_activity(struct Creature *ch);
void psionic_best_attack(struct Creature *ch, struct Creature *vict);
int psionic_mob_fight(struct Creature *ch, struct Creature *precious_vict);

void mage_best_attack(struct Creature *ch, struct Creature *vict);
#endif
