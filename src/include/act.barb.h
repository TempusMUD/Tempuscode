//
// File: act.barb.h                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

//
// act.barb.h, header file for barb functions
// included through char_class.h only
//


int perform_barb_beserk( struct char_data *ch,
                         struct char_data **who_was_attacked,
                         struct char_data *precious_ch,
                         int * return_flags );
