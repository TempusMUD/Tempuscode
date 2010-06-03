#ifndef _MOBACT_H_
#define _MOBACT_H_

//
// File: mobact.h                      -- Part of TempusMUD
//
// All modifications and additions are
// Copyright 1998 by John Watson, all rights reserved.
//

struct creature *choose_opponent(struct creature *ch,
	struct creature *ignore_vict);
int mobile_battle_activity(struct creature *ch,
	struct creature *previous_vict);

int mob_fight_slaad(struct creature *ch, struct creature *precious_vict);
int mob_fight_devil(struct creature *ch, struct creature *precious_vict);
int mob_fight_celestial(struct creature *ch, struct creature *precious_vict);
int mob_fight_guardinal(struct creature *ch, struct creature *precious_vict);
int mob_fight_demon(struct creature *ch, struct creature *precious_vict);
void knight_activity(struct creature * ch);
bool knight_battle_activity(struct creature * ch, struct creature * precious_vict);
void ranger_activity(struct creature *ch);
bool ranger_battle_activity(struct creature * ch, struct creature *precious_vict);
void barbarian_activity(struct creature *ch);
bool barbarian_battle_activity(struct creature * ch, struct creature *precious_vict);

void psionic_activity(struct creature *ch);
void psionic_best_attack(struct creature *ch, struct creature *vict);
int psionic_mob_fight(struct creature *ch, struct creature *precious_vict);

void mage_best_attack(struct creature *ch, struct creature *vict);
void mage_activity(struct creature *ch);
bool mage_mob_fight(struct creature *ch, struct creature *precious_vict);
#endif
