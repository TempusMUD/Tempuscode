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

#define VT_CLEAR       "\x1B[2J"
#define VT_SVPOS       "\x1B[7"
#define VT_RTPOS       "\x1B[8"

#define KNRM  "\x1B[0m"
#define KBLA  "\x1B[30m"
#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KYEL  "\x1B[33m"
#define KBLU  "\x1B[34m"
#define KMAG  "\x1B[35m"
#define KCYN  "\x1B[36m"
#define KWHT  "\x1B[37m"
#define KNUL  ""

#define KBLD  "\x1B[1m"
#define KUND  "\x1B[4m"
#define KBLK  "\x1B[5m"
#define KREV  "\x1B[7m"

#define KBLA_BLD    "\x1B[30m\x1B[1m"
#define KRED_BLD    "\x1B[31m\x1B[1m"
#define KGRN_BLD    "\x1B[32m\x1B[1m"
#define KYEL_BLD    "\x1B[33m\x1B[1m"
#define KBLU_BLD    "\x1B[34m\x1B[1m"
#define KMAG_BLD    "\x1B[35m\x1B[1m"
#define KCYN_BLD    "\x1B[36m\x1B[1m"
#define KNRM_BLD    "\x1B[0m\x1B[1m"

#define KGRN_BLD_BLK    "\x1B[32m\x1B[1m\x1B[5m"
#define KYEL_BLD_BLK    "\x1B[33m\x1B[1m\x1B[5m"
#define KCYN_BLD_BLK    "\x1B[36m\x1B[1m\x1B[5m"

#define KBLA_REV    "\x1B[30m\x1B[7m"
#define KRED_REV    "\x1B[31m\x1B[7m"
#define KGRN_REV    "\x1B[32m\x1B[7m"
#define KYEL_REV    "\x1B[33m\x1B[7m"
#define KBLU_REV    "\x1B[34m\x1B[7m"
#define KMAG_REV    "\x1B[35m\x1B[7m"
#define KCYN_REV    "\x1B[36m\x1B[7m"
#define KNRM_REV    "\x1B[0m\x1B[7m"

#define KNRM_BLA    "\x1B[0m\x1B[30m"
#define KNRM_RED    "\x1B[0m\x1B[31m"
#define KNRM_GRN    "\x1B[0m\x1B[32m"
#define KNRM_YEL    "\x1B[0m\x1B[33m"
#define KNRM_BLU    "\x1B[0m\x1B[34m"
#define KNRM_MAG    "\x1B[0m\x1B[35m"
#define KNRM_CYN    "\x1B[0m\x1B[36m"

/* conditional color.  pass it a pointer to a struct creature and a color level. */
enum {
    C_OFF = 0,
    C_SPR = 1,
    C_NRM = 2,
    C_CMP = 3,
};
typedef unsigned char color_level_t;

static inline color_level_t
_clrlevel(struct creature *ch)
{
    if (!ch->desc || ch->desc->is_blind) {
        return 0;
    }
    return ch->desc->account->ansi_level;
}

static inline bool
clr(struct creature *ch,color_level_t lvl)
{
    return _clrlevel(ch) >= (lvl);
}

#define CNRM(disp,lvl)  ((disp >= lvl) ? KNRM : KNUL)
#define CBLA(disp,lvl)  ((disp >= lvl) ? KBLA : KNUL)
#define CRED(disp,lvl)  ((disp >= lvl) ? KRED : KNUL)
#define CGRN(disp,lvl)  ((disp >= lvl) ? KGRN : KNUL)
#define CYEL(disp,lvl)  ((disp >= lvl) ? KYEL : KNUL)
#define CBLU(disp,lvl)  ((disp >= lvl) ? KBLU : KNUL)
#define CMAG(disp,lvl)  ((disp >= lvl) ? KMAG : KNUL)
#define CCYN(disp,lvl)  ((disp >= lvl) ? KCYN : KNUL)
#define CWHT(disp,lvl)  ((disp >= lvl) ? KWHT : KNUL)

#define CBLD(disp,lvl)  ((disp >= lvl) ? KBLD : KNUL)
#define CUND(disp,lvl)  ((disp >= lvl) ? KUND : KNUL)
#define CBLK(disp,lvl)  ((disp >= lvl) ? KBLK : KNUL)
#define CREV(disp,lvl)  ((disp >= lvl) ? KREV : KNUL)

#define CBLA_BLD(disp,lvl)  ((disp >= lvl) ? KBLA_BLD : KNUL)
#define CRED_BLD(disp,lvl)  ((disp >= lvl) ? KRED_BLD : KNUL)
#define CGRN_BLD(disp,lvl)  ((disp >= lvl) ? KGRN_BLD : KNUL)
#define CYEL_BLD(disp,lvl)  ((disp >= lvl) ? KYEL_BLD : KNUL)
#define CBLU_BLD(disp,lvl)  ((disp >= lvl) ? KBLU_BLD : KNUL)
#define CMAG_BLD(disp,lvl)  ((disp >= lvl) ? KMAG_BLD : KNUL)
#define CCYN_BLD(disp,lvl)  ((disp >= lvl) ? KCYN_BLD : KNUL)
#define CNRM_BLD(disp,lvl)  ((disp >= lvl) ? KNRM_BLD : KNUL)

#define CBLA_REV(disp,lvl)  ((disp >= lvl) ? KBLA_REV : KNUL)
#define CRED_REV(disp,lvl)  ((disp >= lvl) ? KRED_REV : KNUL)
#define CGRN_REV(disp,lvl)  ((disp >= lvl) ? KGRN_REV : KNUL)
#define CYEL_REV(disp,lvl)  ((disp >= lvl) ? KYEL_REV : KNUL)
#define CBLU_REV(disp,lvl)  ((disp >= lvl) ? KBLU_REV : KNUL)
#define CMAG_REV(disp,lvl)  ((disp >= lvl) ? KMAG_REV : KNUL)
#define CCYN_REV(disp,lvl)  ((disp >= lvl) ? KCYN_REV : KNUL)

#define CNRM_BLA(disp,lvl)  ((disp >= lvl) ? KNRM_BLA : KNUL)
#define CNRM_RED(disp,lvl)  ((disp >= lvl) ? KNRM_RED : KNUL)
#define CNRM_GRN(disp,lvl)  ((disp >= lvl) ? KNRM_GRN : KNUL)
#define CNRM_YEL(disp,lvl)  ((disp >= lvl) ? KNRM_YEL : KNUL)
#define CNRM_BLU(disp,lvl)  ((disp >= lvl) ? KNRM_BLU : KNUL)
#define CNRM_MAG(disp,lvl)  ((disp >= lvl) ? KNRM_MAG : KNUL)
#define CNRM_CYN(disp,lvl)  ((disp >= lvl) ? KNRM_CYN : KNUL)


#define CCNRM(ch,lvl)  CNRM(_clrlevel(ch),lvl)
#define CCBLA(ch,lvl)  CBLA(_clrlevel(ch),lvl)
#define CCRED(ch,lvl)  CRED(_clrlevel(ch),lvl)
#define CCGRN(ch,lvl)  CGRN(_clrlevel(ch),lvl)
#define CCYEL(ch,lvl)  CYEL(_clrlevel(ch),lvl)
#define CCBLU(ch,lvl)  CBLU(_clrlevel(ch),lvl)
#define CCMAG(ch,lvl)  CMAG(_clrlevel(ch),lvl)
#define CCCYN(ch,lvl)  CCYN(_clrlevel(ch),lvl)
#define CCWHT(ch,lvl)  CWHT(_clrlevel(ch),lvl)

#define CCBLD(ch,lvl)  CBLD(_clrlevel(ch),lvl)
#define CCUND(ch,lvl)  CUND(_clrlevel(ch),lvl)
#define CCBLK(ch,lvl)  CBLK(_clrlevel(ch),lvl)
#define CCREV(ch,lvl)  CREV(_clrlevel(ch),lvl)

#define CCBLA_BLD(ch,lvl)  CBLA_BLD(_clrlevel(ch),lvl)
#define CCRED_BLD(ch,lvl)  CRED_BLD(_clrlevel(ch),lvl)
#define CCGRN_BLD(ch,lvl)  CGRN_BLD(_clrlevel(ch),lvl)
#define CCYEL_BLD(ch,lvl)  CYEL_BLD(_clrlevel(ch),lvl)
#define CCBLU_BLD(ch,lvl)  CBLU_BLD(_clrlevel(ch),lvl)
#define CCMAG_BLD(ch,lvl)  CMAG_BLD(_clrlevel(ch),lvl)
#define CCCYN_BLD(ch,lvl)  CCYN_BLD(_clrlevel(ch),lvl)
#define CCNRM_BLD(ch,lvl)  CNRM_BLD(_clrlevel(ch),lvl)

#define CCBLA_REV(ch,lvl)  CBLA_REV(_clrlevel(ch),lvl)
#define CCRED_REV(ch,lvl)  CRED_REV(_clrlevel(ch),lvl)
#define CCGRN_REV(ch,lvl)  CGRN_REV(_clrlevel(ch),lvl)
#define CCYEL_REV(ch,lvl)  CYEL_REV(_clrlevel(ch),lvl)
#define CCBLU_REV(ch,lvl)  CBLU_REV(_clrlevel(ch),lvl)
#define CCMAG_REV(ch,lvl)  CMAG_REV(_clrlevel(ch),lvl)
#define CCCYN_REV(ch,lvl)  CCYN_REV(_clrlevel(ch),lvl)

#define CCNRM_BLA(ch,lvl)  CNRM_BLA(_clrlevel(ch),lvl)
#define CCNRM_RED(ch,lvl)  CNRM_RED(_clrlevel(ch),lvl)
#define CCNRM_GRN(ch,lvl)  CNRM_GRN(_clrlevel(ch),lvl)
#define CCNRM_YEL(ch,lvl)  CNRM_YEL(_clrlevel(ch),lvl)
#define CCNRM_BLU(ch,lvl)  CNRM_BLU(_clrlevel(ch),lvl)
#define CCNRM_MAG(ch,lvl)  CNRM_MAG(_clrlevel(ch),lvl)
#define CCNRM_CYN(ch,lvl)  CNRM_CYN(_clrlevel(ch),lvl)

#define COLOR_LEV(ch) (_clrlevel(ch))

#define QNRM CCNRM(ch,C_SPR)
#define QRED CCRED(ch,C_SPR)
#define QGRN CCGRN(ch,C_SPR)
#define QYEL CCYEL(ch,C_SPR)
#define QBLU CCBLU(ch,C_SPR)
#define QMAG CCMAG(ch,C_SPR)
#define QCYN CCCYN(ch,C_SPR)
#define QWHT CCWHT(ch,C_SPR)
#endif
