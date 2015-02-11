#include <stdio.h>
#include <stdbool.h>
#include <ctype.h>
#include <inttypes.h>
#include <glib.h>

#include "interpreter.h"
#include "structs.h"
#include "utils.h"
#include "constants.h"
#include "comm.h"
#include "security.h"
#include "handler.h"
#include "defs.h"
#include "desc_data.h"
#include "macros.h"
#include "room_data.h"
#include "zone_data.h"
#include "race.h"
#include "creature.h"
#include "account.h"
#include "screen.h"

const char *ANSI_CODES[NUM_COLORS] = {
    // Terminal clear
    "\x1B[2J",
    // Terminal reset
    "\x1B[0m",
    // Colors
    "\x1B[30m",
    "\x1B[31m",
    "\x1B[32m",
    "\x1B[33m",
    "\x1B[34m",
    "\x1B[35m",
    "\x1B[36m",
    "\x1B[37m",
    // Other attributes
    "\x1B[0m",
    "\x1B[1m",
    "\x1B[7m",
    "\x1B[8m",
    // Bold colors
    "\x1B[0m\x1B[1m",
    "\x1B[30m\x1B[1m",
    "\x1B[31m\x1B[1m",
    "\x1B[32m\x1B[1m",
    "\x1B[33m\x1B[1m",
    "\x1B[34m\x1B[1m",
    "\x1B[35m\x1B[1m",
    "\x1B[36m\x1B[1m",
    // Reversed colors
    "\x1B[30m\x1B[7m",
    "\x1B[31m\x1B[7m",
    "\x1B[32m\x1B[7m",
    "\x1B[33m\x1B[7m",
    "\x1B[34m\x1B[7m",
    "\x1B[35m\x1B[7m",
    "\x1B[36m\x1B[7m",
    // Forced normal colors
    "\x1B[0m\x1B[30m",
    "\x1B[0m\x1B[31m",
    "\x1B[0m\x1B[32m",
    "\x1B[0m\x1B[33m",
    "\x1B[0m\x1B[34m",
    "\x1B[0m\x1B[35m",
    "\x1B[0m\x1B[36m",
};

const char *IRC_CODES[NUM_COLORS] = {
    // Terminal clear
    "",
    // Terminal reset
    "\x0f",
    // Colors
    "\x03" "01",
    "\x03" "04",
    "\x03" "03",
    "\x03" "08",
    "\x03" "02",
    "\x03" "05",
    "\x03" "11",
    "\x03" "00",
    // Other attributes
    "\x02",
    "\x1f",
    "\x1d",
    "\x16",
    // Bold colors
    "\x0f\x02",
    "\x02\x03" "01",
    "\x02\x03" "04",
    "\x02\x03" "03",
    "\x02\x03" "08",
    "\x02\x03" "02",
    "\x02\x03" "05",
    "\x02\x03" "11",
    // Reversed colors
    "\x16\x03" "01",
    "\x16\x03" "04",
    "\x16\x03" "03",
    "\x16\x03" "08",
    "\x16\x03" "02",
    "\x16\x03" "05",
    "\x16\x03" "11",
    // Forced normal colors
    "\x0f\x03" "01",
    "\x0f\x03" "04",
    "\x0f\x03" "03",
    "\x0f\x03" "08",
    "\x0f\x03" "02",
    "\x0f\x03" "05",
    "\x0f\x03" "11",
};

color_level_t
_clrlevel(struct creature *ch)
{
    if (!ch->desc || ch->desc->display == BLIND) {
        return 0;
    }
    return ch->desc->account->ansi_level;
}

bool
clr(struct creature *ch,color_level_t lvl)
{
    return _clrlevel(ch) >= (lvl);
}

const char *
termcode(enum display_mode mode, color_level_t setting, color_level_t lvl, int index)
{
    if (mode == BLIND || setting < lvl) {
        return "";
    }
    if (mode == IRC) {
        return IRC_CODES[index];
    }
    return ANSI_CODES[index];
}

const char *
dtermcode(struct descriptor_data *d, color_level_t lvl, int color_index) {
    if (d == NULL || d->account == NULL) {
        return "";
    }
    return termcode(d->display, d->account->ansi_level, lvl, color_index);
}

const char *CCCLEAR(struct creature *ch, color_level_t lvl) {
    return dtermcode(ch->desc, lvl, TERM_CLEAR);
}
const char *CCNRM(struct creature *ch, color_level_t lvl) {
    return dtermcode(ch->desc, lvl, TERM_NRM);
}
const char *CCBLA(struct creature *ch, color_level_t lvl) {
    return dtermcode(ch->desc, lvl, TERM_BLA);
}
const char *CCRED(struct creature *ch, color_level_t lvl) {
    return dtermcode(ch->desc, lvl, TERM_RED);
}
const char *CCGRN(struct creature *ch, color_level_t lvl) {
    return dtermcode(ch->desc, lvl, TERM_GRN);
}
const char *CCYEL(struct creature *ch, color_level_t lvl) {
    return dtermcode(ch->desc, lvl, TERM_YEL);
}
const char *CCBLU(struct creature *ch, color_level_t lvl) {
    return dtermcode(ch->desc, lvl, TERM_BLU);
}
const char *CCMAG(struct creature *ch, color_level_t lvl) {
    return dtermcode(ch->desc, lvl, TERM_MAG);
}
const char *CCCYN(struct creature *ch, color_level_t lvl) {
    return dtermcode(ch->desc, lvl, TERM_CYN);
}
const char *CCWHT(struct creature *ch, color_level_t lvl) {
    return dtermcode(ch->desc, lvl, TERM_WHT);
}

const char *CCBLD(struct creature *ch, color_level_t lvl) {
    return dtermcode(ch->desc, lvl, TERM_BLD);
}
const char *CCUND(struct creature *ch, color_level_t lvl) {
    return dtermcode(ch->desc, lvl, TERM_UND);
}
const char *CCBLK(struct creature *ch, color_level_t lvl) {
    return dtermcode(ch->desc, lvl, TERM_BLK);
}
const char *CCREV(struct creature *ch, color_level_t lvl) {
    return dtermcode(ch->desc, lvl, TERM_REV);
}

const char *CCBLA_BLD(struct creature *ch, color_level_t lvl) {
    return dtermcode(ch->desc, lvl, TERM_BLA_BLD);
}
const char *CCRED_BLD(struct creature *ch, color_level_t lvl) {
    return dtermcode(ch->desc, lvl, TERM_RED_BLD);
}
const char *CCGRN_BLD(struct creature *ch, color_level_t lvl) {
    return dtermcode(ch->desc, lvl, TERM_GRN_BLD);
}
const char *CCYEL_BLD(struct creature *ch, color_level_t lvl) {
    return dtermcode(ch->desc, lvl, TERM_YEL_BLD);
}
const char *CCBLU_BLD(struct creature *ch, color_level_t lvl) {
    return dtermcode(ch->desc, lvl, TERM_BLU_BLD);
}
const char *CCMAG_BLD(struct creature *ch, color_level_t lvl) {
    return dtermcode(ch->desc, lvl, TERM_MAG_BLD);
}
const char *CCCYN_BLD(struct creature *ch, color_level_t lvl) {
    return dtermcode(ch->desc, lvl, TERM_CYN_BLD);
}
const char *CCNRM_BLD(struct creature *ch, color_level_t lvl) {
    return dtermcode(ch->desc, lvl, TERM_NRM_BLD);
}
const char *CCBLA_REV(struct creature *ch, color_level_t lvl) {
    return dtermcode(ch->desc, lvl, TERM_BLA_REV);
}
const char *CCRED_REV(struct creature *ch, color_level_t lvl) {
    return dtermcode(ch->desc, lvl, TERM_RED_REV);
}
const char *CCGRN_REV(struct creature *ch, color_level_t lvl) {
    return dtermcode(ch->desc, lvl, TERM_GRN_REV);
}
const char *CCYEL_REV(struct creature *ch, color_level_t lvl) {
    return dtermcode(ch->desc, lvl, TERM_YEL_REV);
}
const char *CCBLU_REV(struct creature *ch, color_level_t lvl) {
    return dtermcode(ch->desc, lvl, TERM_BLU_REV);
}
const char *CCMAG_REV(struct creature *ch, color_level_t lvl) {
    return dtermcode(ch->desc, lvl, TERM_MAG_REV);
}
const char *CCCYN_REV(struct creature *ch, color_level_t lvl) {
    return dtermcode(ch->desc, lvl, TERM_CYN_REV);
}
const char *CCNRM_BLA(struct creature *ch, color_level_t lvl) {
    return dtermcode(ch->desc, lvl, TERM_NRM_BLA);
}
const char *CCNRM_RED(struct creature *ch, color_level_t lvl) {
    return dtermcode(ch->desc, lvl, TERM_NRM_RED);
}
const char *CCNRM_GRN(struct creature *ch, color_level_t lvl) {
    return dtermcode(ch->desc, lvl, TERM_NRM_GRN);
}
const char *CCNRM_YEL(struct creature *ch, color_level_t lvl) {
    return dtermcode(ch->desc, lvl, TERM_NRM_YEL);
}
const char *CCNRM_BLU(struct creature *ch, color_level_t lvl) {
    return dtermcode(ch->desc, lvl, TERM_NRM_BLU);
}
const char *CCNRM_MAG(struct creature *ch, color_level_t lvl) {
    return dtermcode(ch->desc, lvl, TERM_NRM_MAG);
}
const char *CCNRM_CYN(struct creature *ch, color_level_t lvl) {
    return dtermcode(ch->desc, lvl, TERM_NRM_CYN);
}
