#ifndef _SCREEN_H_
#define _SCREEN_H_

/* ************************************************************************
*   File: screen.h                                      Part of CircleMUD *
*  Usage: header file with ANSI color codes for online color              *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

//
// File: screen.h                      -- Part of TempusMUD
//
// All modifications and additions are
// Copyright 1998 by John Watson, all rights reserved.
//

enum {
    TERM_CLEAR = 0,
    TERM_NRM,

    TERM_BLA,
    TERM_RED,
    TERM_GRN,
    TERM_YEL,
    TERM_BLU,
    TERM_MAG,
    TERM_CYN,
    TERM_WHT,

    TERM_BLD,
    TERM_UND,
    TERM_BLK,
    TERM_REV,

    TERM_NRM_BLD,
    TERM_BLA_BLD,
    TERM_RED_BLD,
    TERM_GRN_BLD,
    TERM_YEL_BLD,
    TERM_BLU_BLD,
    TERM_MAG_BLD,
    TERM_CYN_BLD,

    TERM_BLA_REV,
    TERM_RED_REV,
    TERM_GRN_REV,
    TERM_YEL_REV,
    TERM_BLU_REV,
    TERM_MAG_REV,
    TERM_CYN_REV,

    TERM_NRM_BLA,
    TERM_NRM_RED,
    TERM_NRM_GRN,
    TERM_NRM_YEL,
    TERM_NRM_BLU,
    TERM_NRM_MAG,
    TERM_NRM_CYN,
    NUM_COLORS
};

/* conditional color.  pass it a pointer to a struct creature and a color level. */
enum {
    C_OFF = 0,
    C_SPR = 1,
    C_NRM = 2,
    C_CMP = 3,
};
typedef unsigned char color_level_t;

#define COLOR_LEV(ch) (_clrlevel(ch))
#define DISPLAY_MODE(ch) ((ch == NULL || ch->desc == NULL) ? NORMAL:ch->desc->display)

color_level_t _clrlevel(struct creature *ch);
bool clr(struct creature *ch,color_level_t lvl);
const char *termcode(enum display_mode mode, color_level_t setting, color_level_t lvl, int index);
const char *dtermcode(struct descriptor_data *d, color_level_t lvl, int color_index);

const char *CCNRM(struct creature *ch, color_level_t lvl);
const char *CCBLA(struct creature *ch, color_level_t lvl);
const char *CCRED(struct creature *ch, color_level_t lvl);
const char *CCGRN(struct creature *ch, color_level_t lvl);
const char *CCYEL(struct creature *ch, color_level_t lvl);
const char *CCBLU(struct creature *ch, color_level_t lvl);
const char *CCMAG(struct creature *ch, color_level_t lvl);
const char *CCCYN(struct creature *ch, color_level_t lvl);
const char *CCWHT(struct creature *ch, color_level_t lvl);
const char *CCBLD(struct creature *ch, color_level_t lvl);
const char *CCUND(struct creature *ch, color_level_t lvl);
const char *CCBLD(struct creature *ch, color_level_t lvl);
const char *CCBLK(struct creature *ch, color_level_t lvl);
const char *CCREV(struct creature *ch, color_level_t lvl);
const char *CCBLA_BLD(struct creature *ch, color_level_t lvl);
const char *CCRED_BLD(struct creature *ch, color_level_t lvl);
const char *CCGRN_BLD(struct creature *ch, color_level_t lvl);
const char *CCYEL_BLD(struct creature *ch, color_level_t lvl);
const char *CCBLU_BLD(struct creature *ch, color_level_t lvl);
const char *CCMAG_BLD(struct creature *ch, color_level_t lvl);
const char *CCCYN_BLD(struct creature *ch, color_level_t lvl);
const char *CCNRM_BLD(struct creature *ch, color_level_t lvl);
const char *CCBLA_REV(struct creature *ch, color_level_t lvl);
const char *CCRED_REV(struct creature *ch, color_level_t lvl);
const char *CCGRN_REV(struct creature *ch, color_level_t lvl);
const char *CCYEL_REV(struct creature *ch, color_level_t lvl);
const char *CCBLU_REV(struct creature *ch, color_level_t lvl);
const char *CCMAG_REV(struct creature *ch, color_level_t lvl);
const char *CCCYN_REV(struct creature *ch, color_level_t lvl);
const char *CCNRM_BLA(struct creature *ch, color_level_t lvl);
const char *CCNRM_RED(struct creature *ch, color_level_t lvl);
const char *CCNRM_GRN(struct creature *ch, color_level_t lvl);
const char *CCNRM_YEL(struct creature *ch, color_level_t lvl);
const char *CCNRM_BLU(struct creature *ch, color_level_t lvl);
const char *CCNRM_MAG(struct creature *ch, color_level_t lvl);
const char *CCNRM_CYN(struct creature *ch, color_level_t lvl);

#endif
