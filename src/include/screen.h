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

#define KBLA_BLD	"\x1B[30m\x1B[1m"
#define KRED_BLD	"\x1B[31m\x1B[1m"
#define KGRN_BLD  	"\x1B[32m\x1B[1m"
#define KYEL_BLD  	"\x1B[33m\x1B[1m"
#define KBLU_BLD	"\x1B[34m\x1B[1m"
#define KMAG_BLD  	"\x1B[35m\x1B[1m"
#define KCYN_BLD  	"\x1B[36m\x1B[1m"
#define KNRM_BLD  	"\x1B[0m\x1B[1m"

#define KGRN_BLD_BLK  	"\x1B[32m\x1B[1m\x1B[5m"
#define KYEL_BLD_BLK  	"\x1B[33m\x1B[1m\x1B[5m"
#define KCYN_BLD_BLK  	"\x1B[36m\x1B[1m\x1B[5m"

#define KBLA_REV	"\x1B[30m\x1B[7m"
#define KRED_REV	"\x1B[31m\x1B[7m"
#define KGRN_REV  	"\x1B[32m\x1B[7m"
#define KYEL_REV  	"\x1B[33m\x1B[7m"
#define KBLU_REV	"\x1B[34m\x1B[7m"
#define KMAG_REV  	"\x1B[35m\x1B[7m"
#define KCYN_REV  	"\x1B[36m\x1B[7m"
#define KNRM_REV  	"\x1B[0m\x1B[7m"

#define KNRM_BLA  	"\x1B[0m\x1B[30m"
#define KNRM_RED  	"\x1B[0m\x1B[31m"
#define KNRM_GRN  	"\x1B[0m\x1B[32m"
#define KNRM_YEL  	"\x1B[0m\x1B[33m"
#define KNRM_BLU  	"\x1B[0m\x1B[34m"
#define KNRM_MAG  	"\x1B[0m\x1B[35m"
#define KNRM_CYN  	"\x1B[0m\x1B[36m"


/* conditional color.  pass it a pointer to a Creature and a color level. */
#define C_OFF	0
#define C_SPR	1
#define C_NRM	2
#define C_CMP	3
#define _clrlevel(ch) ((ch->desc && ch->desc->original) ? \
		       ((PRF_FLAGGED((ch->desc->original),PRF_COLOR_1)?1:0)+\
			(PRF_FLAGGED((ch->desc->original),PRF_COLOR_2)?2:0)):\
		       (PRF_FLAGGED((ch), PRF_COLOR_1) ? 1 : 0) + \
		       (PRF_FLAGGED((ch), PRF_COLOR_2) ? 2 : 0))
#define clr(ch,lvl) (_clrlevel(ch) >= (lvl))
#define CCNRM(ch,lvl)  (clr((ch),(lvl))?KNRM:KNUL)
#define CCBLA(ch,lvl)  (clr((ch),(lvl))?KBLA:KNUL)
#define CCRED(ch,lvl)  (clr((ch),(lvl))?KRED:KNUL)
#define CCGRN(ch,lvl)  (clr((ch),(lvl))?KGRN:KNUL)
#define CCYEL(ch,lvl)  (clr((ch),(lvl))?KYEL:KNUL)
#define CCBLU(ch,lvl)  (clr((ch),(lvl))?KBLU:KNUL)
#define CCMAG(ch,lvl)  (clr((ch),(lvl))?KMAG:KNUL)
#define CCCYN(ch,lvl)  (clr((ch),(lvl))?KCYN:KNUL)
#define CCWHT(ch,lvl)  (clr((ch),(lvl))?KWHT:KNUL)

#define CCBLD(ch,lvl)  (clr((ch),(lvl))?KBLD:KNUL)
#define CCUND(ch,lvl)  (clr((ch),(lvl))?KUND:KNUL)
#define CCBLK(ch,lvl)  (clr((ch),(lvl))?KBLK:KNUL)
#define CCREV(ch,lvl)  (clr((ch),(lvl))?KREV:KNUL)

#define CCBLA_BLD(ch,lvl)  (clr((ch),(lvl))?KBLA_BLD:KNUL)
#define CCRED_BLD(ch,lvl)  (clr((ch),(lvl))?KRED_BLD:KNUL)
#define CCGRN_BLD(ch,lvl)  (clr((ch),(lvl))?KGRN_BLD:KNUL)
#define CCYEL_BLD(ch,lvl)  (clr((ch),(lvl))?KYEL_BLD:KNUL)
#define CCBLU_BLD(ch,lvl)  (clr((ch),(lvl))?KBLU_BLD:KNUL)
#define CCMAG_BLD(ch,lvl)  (clr((ch),(lvl))?KMAG_BLD:KNUL)
#define CCCYN_BLD(ch,lvl)  (clr((ch),(lvl))?KCYN_BLD:KNUL)
#define CCNRM_BLD(ch,lvl)  (clr((ch),(lvl))?KNRM_BLD:KNUL)

#define CCBLA_REV(ch,lvl)  (clr((ch),(lvl))?KBLA_REV:KNUL)
#define CCRED_REV(ch,lvl)  (clr((ch),(lvl))?KRED_REV:KNUL)
#define CCGRN_REV(ch,lvl)  (clr((ch),(lvl))?KGRN_REV:KNUL)
#define CCYEL_REV(ch,lvl)  (clr((ch),(lvl))?KYEL_REV:KNUL)
#define CCBLU_REV(ch,lvl)  (clr((ch),(lvl))?KBLU_REV:KNUL)
#define CCMAG_REV(ch,lvl)  (clr((ch),(lvl))?KMAG_REV:KNUL)
#define CCCYN_REV(ch,lvl)  (clr((ch),(lvl))?KCYN_REV:KNUL)

#define CCNRM_BLA(ch,lvl)  (clr((ch),(lvl))?KNRM_BLA:KNUL)
#define CCNRM_RED(ch,lvl)  (clr((ch),(lvl))?KNRM_RED:KNUL)
#define CCNRM_GRN(ch,lvl)  (clr((ch),(lvl))?KNRM_GRN:KNUL)
#define CCNRM_YEL(ch,lvl)  (clr((ch),(lvl))?KNRM_YEL:KNUL)
#define CCNRM_BLU(ch,lvl)  (clr((ch),(lvl))?KNRM_BLU:KNUL)
#define CCNRM_MAG(ch,lvl)  (clr((ch),(lvl))?KNRM_MAG:KNUL)
#define CCNRM_CYN(ch,lvl)  (clr((ch),(lvl))?KNRM_CYN:KNUL)

#define COLOR_LEV(ch) (_clrlevel(ch))

#define QNRM CCNRM(ch,C_SPR)
#define QRED CCRED(ch,C_SPR)
#define QGRN CCGRN(ch,C_SPR)
#define QYEL CCYEL(ch,C_SPR)
#define QBLU CCBLU(ch,C_SPR)
#define QMAG CCMAG(ch,C_SPR)
#define QCYN CCCYN(ch,C_SPR)
#define QWHT CCWHT(ch,C_SPR)
