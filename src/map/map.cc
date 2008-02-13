#include "map.h"
#include <math.h>

using namespace std;

int MapPixel::centerX;
int MapPixel::centerY;
int MapPixel::hSize;
int MapPixel::vSize;

Creature * Map::_ch;

struct _sectToPen Map::sectToPen[NUM_SECT_TYPES] = {
    { SECT_INSIDE, "", "", "", ".", "#", "#", "#" },
    { SECT_CITY, "", "", "", ".",  "#", "#", "#" },
    { SECT_FIELD, "", "", "", ".", "^", "^", "^" },
    { SECT_FOREST, "", "", "", "@", "^", "^", "^" },
    { SECT_HILLS, "", "", "", "^", "^", "^", "^" },
    { SECT_MOUNTAIN, "", "", "", ".", "^", "^", "^" },
    { SECT_WATER_SWIM, "", "", "", "~", "^", "^", "^" },
    { SECT_WATER_NOSWIM, "", "", "", "~", "^", "^", "^" },
    { SECT_UNDERWATER, "", "", "", "~", "^", "^", "^" },
    { SECT_FLYING, "", "", "", ".", "^", "^", "^" }, 
    { SECT_NOTIME, "", "", "", ",", "^", "^", "^" },
    { SECT_CLIMBING, "", "", "", ".", "^", "^", "^" },
    { SECT_FREESPACE, "", "", "", "O", "^", "^", "^" },
    { SECT_ROAD, "", "", "", "#", "^", "^", "^" },
    { SECT_VEHICLE, "", "", "", "`", "^", "^", "^" },
    { SECT_FARMLAND, "", "", "", "^", "^", "^", "^" },
    { SECT_SWAMP, "", "", "", "$", "^", "^", "^" },
    { SECT_DESERT, "", "", "", "~", "^", "^", "^" },
    { SECT_FIRE_RIVER, "", "", "", "~", "^", "^", "^" },
    { SECT_JUNGLE, "", "", "", "@", "^", "^", "^" },
    { SECT_PITCH_PIT, "", "", "", "!", "^", "^", "^" },
    { SECT_PITCH_SUB, "", "", "", "!", "^", "^", "^" },
    { SECT_BEACH, "", "", "", ".", "^", "^", "^" },
    { SECT_ASTRAL, "", "", "", "O", "^", "^", "^" },
    { SECT_ELEMENTAL_FIRE, "", "", "", "$", "^", "^", "^" },
    { SECT_ELEMENTAL_EARTH, "", "", "", "-", "^", "^", "^" },
    { SECT_ELEMENTAL_AIR, "", "", "", "~", "^", "^", "^" },
    { SECT_ELEMENTAL_WATER, "", "", "", "~", "^", "^", "^" },
    { SECT_ELEMENTAL_POSITIVE, "", "", "", "+", "^", "^", "^" },
    { SECT_ELEMENTAL_NEGATIVE, "", "", "", "-", "^", "^", "^" },
    { SECT_ELEMENTAL_SMOKE, "", "", "", "$", "^", "^", "^" },
    { SECT_ELEMENTAL_ICE, "", "", "", ".", "^", "^", "^" },
    { SECT_ELEMENTAL_OOZE, "", "", "", "\"", "^", "^", "^" },
    { SECT_ELEMENTAL_MAGMA, "", "", "", "~", "^", "^", "^" },
    { SECT_ELEMENTAL_LIGHTNING, "", "", "", "^", "^", "^", "^" },
    { SECT_ELEMENTAL_STEAM, "", "", "", "$", "^", "^", "^" },
    { SECT_ELEMENTAL_RADIANCE, "", "", "", "U", "^", "^", "^" },
    { SECT_ELEMENTAL_MINERALS, "", "", "", "U", "^", "^", "^" },
    { SECT_ELEMENTAL_VACUUM, "", "", "", "U", "^", "^", "^" },
    { SECT_ELEMENTAL_SALT, "", "", "", "U", "^", "^", "^" },
    { SECT_ELEMENTAL_ASH, "", "", "", "U", "^", "^", "^" },
    { SECT_ELEMENTAL_DUST, "", "", "", "U", "^", "^", "^" },
    { SECT_BLOOD, "", "", "", "~", "^", "^", "^" },
    { SECT_ROCK, "", "", "", "<", "^", "^", "^" },
    { SECT_MUDDY, "", "", "", ".", "^", "^", "^" },
    { SECT_TRAIL, "", "", "", "#", "^", "^", "^" },
    { SECT_TUNDRA, "", "", "", ".", "^", "^", "^" },
    { SECT_CATACOMBS, "", "", "", ".", "#", "#", "#" },
    { SECT_CRACKED_ROAD, "", "", "", "%", "^", "^", "^" },
    { SECT_DEEP_OCEAN, "", "", "", "~", "^", "^", "^" }
};

ACMD(do_map)
{
    char *arg1, *ptr;
    int rows = 0, cols = 0;

	if (ch->isNPC()) {
		send_to_char(ch, "You scribble out a map on the ground.\r\n");
		return;
	}

    arg1 = tmp_getword(&argument);
    if (*arg1) {
        ptr = arg1;
        while (*ptr != 'x' && *ptr !='X' && *ptr != 0)
            ptr++;

        if (*ptr != 0) {
            *ptr = 0;
            ptr++;
        } 

        rows = atoi(arg1);
        if (*ptr)
            cols = atoi(ptr);
    }

    if (is_abbrev(arg1, "on")) {
        ch->player_specials->_mapRows = 5;
        ch->player_specials->_mapCols = 7;
        send_to_char(ch, "Ok, The map will now be displayed.\r\n ");
        return;
    }
    else if (is_abbrev(arg1, "off")) {
        ch->player_specials->_mapRows = 0;
        ch->player_specials->_mapCols = 0;
        send_to_char(ch, "Ok, The map will no longer be displayed.\r\n ");
        return;
    }
    else if (is_abbrev(arg1, "small")) {
        ch->player_specials->_mapRows = 3;
        ch->player_specials->_mapCols = 5;
        send_to_char(ch, "Ok, your map size will now be small.\r\n ");
        return;
    }
    else if (is_abbrev(arg1, "medium")) {
        ch->player_specials->_mapRows = 5;
        ch->player_specials->_mapCols = 7;
        send_to_char(ch, "Ok, your map size will now be medium.\r\n ");
        return;
    }
    else if (is_abbrev(arg1, "large")) {
        ch->player_specials->_mapRows = 7;
        ch->player_specials->_mapCols = 9;
        send_to_char(ch, "Ok, your map size will now be large.\r\n ");
        return;
    }
    else if (rows && cols) {
        if (rows < 0 || cols < 0) {
            send_to_char(ch, "Why don't you just turn it off?\r\n");
            return;
        }

        if ((rows > 15 || cols > 19) && !ch->isImmortal()) {
            send_to_char(ch, "Come now, let's be reasonable, shall we?\r\n");
            return;
        }
        else {
            send_to_char(ch, "CAUTION:  Generating maps this large will slow down the mud!\r\n"
                    "Do not make it permenant!\r\n");
        }

        rows = MIN(rows, 125);
        cols = MIN(cols, 125);

        ch->player_specials->_mapRows = (char)rows;
        ch->player_specials->_mapCols = (char)cols;

        send_to_char(ch, "Ok, your map size will now be %dx%d.\r\n", rows, cols);

        return;
    }

    send_to_char(ch, "usage:  map [ [on | off] [ <rows>x<columns> | "
            "[ small | medium | large ] ]\r\n");
}

Map::Map(Creature *ch, int hsize, int vsize) {
    _hsize = hsize;
    _vsize = vsize;

    _map.resize((_hsize * 3) * (_vsize * 3));
    _map.resize((_hsize * 3) * (_vsize * 3));

    vector<string>::iterator vi = _map.begin();
    for (; vi != _map.end(); ++vi) {
        *vi = " ";
    }

    resize(hsize * vsize);
    Map::_ch = ch;

    Map::sectToPen[SECT_INSIDE].penColor = CCYEL_BLD(_ch, C_SPR);
    Map::sectToPen[SECT_INSIDE].vNoExitColor = CCNRM_BLD(_ch, C_SPR);
    Map::sectToPen[SECT_INSIDE].hNoExitColor = CCNRM_BLD(_ch, C_SPR); 

    Map::sectToPen[SECT_CITY].penColor = CCYEL_BLD(_ch, C_SPR);
    Map::sectToPen[SECT_CITY].vNoExitColor = CCNRM_BLD(_ch, C_SPR);
    Map::sectToPen[SECT_CITY].hNoExitColor = CCNRM_BLD(_ch, C_SPR); 

    Map::sectToPen[SECT_FIELD].penColor = CCYEL(_ch, C_SPR);
    Map::sectToPen[SECT_FIELD].vNoExitColor = CCYEL(_ch, C_SPR);
    Map::sectToPen[SECT_FIELD].hNoExitColor = CCYEL(_ch, C_SPR); 

    Map::sectToPen[SECT_FOREST].penColor = CCGRN(_ch, C_SPR);
    Map::sectToPen[SECT_FOREST].vNoExitColor = CCGRN(_ch, C_SPR);
    Map::sectToPen[SECT_FOREST].hNoExitColor = CCGRN(_ch, C_SPR); 

    Map::sectToPen[SECT_HILLS].penColor = CCYEL(_ch, C_SPR);
    Map::sectToPen[SECT_HILLS].vNoExitColor = CCYEL(_ch, C_SPR);
    Map::sectToPen[SECT_HILLS].hNoExitColor = CCGRN(_ch, C_SPR); 

    Map::sectToPen[SECT_MOUNTAIN].penColor = CCNRM_BLD(_ch, C_SPR);
    Map::sectToPen[SECT_MOUNTAIN].vNoExitColor = CCNRM_BLD(_ch, C_SPR);
    Map::sectToPen[SECT_MOUNTAIN].hNoExitColor = CCNRM_BLD(_ch, C_SPR); 

    Map::sectToPen[SECT_WATER_SWIM].penColor = CCBLU_BLD(_ch, C_SPR);
    Map::sectToPen[SECT_WATER_SWIM].vNoExitColor = CCNRM(_ch, C_SPR);
    Map::sectToPen[SECT_WATER_SWIM].hNoExitColor = CCNRM(_ch, C_SPR); 

    Map::sectToPen[SECT_WATER_NOSWIM].penColor = CCBLU(_ch, C_SPR);
    Map::sectToPen[SECT_WATER_NOSWIM].vNoExitColor = CCNRM(_ch, C_SPR);
    Map::sectToPen[SECT_WATER_NOSWIM].hNoExitColor = CCNRM(_ch, C_SPR); 

    Map::sectToPen[SECT_UNDERWATER].penColor = CCBLU(_ch, C_SPR);
    Map::sectToPen[SECT_UNDERWATER].vNoExitColor = CCBLU(_ch, C_SPR);
    Map::sectToPen[SECT_UNDERWATER].hNoExitColor = CCBLU(_ch, C_SPR); 

    Map::sectToPen[SECT_FLYING].penColor = CCCYN_BLD(_ch, C_SPR);
    Map::sectToPen[SECT_FLYING].vNoExitColor = CCCYN_BLD(_ch, C_SPR);
    Map::sectToPen[SECT_FLYING].hNoExitColor = CCCYN_BLD(_ch, C_SPR); 

    Map::sectToPen[SECT_NOTIME].penColor = CCMAG(_ch, C_SPR);
    Map::sectToPen[SECT_NOTIME].vNoExitColor = CCMAG(_ch, C_SPR);
    Map::sectToPen[SECT_NOTIME].hNoExitColor = CCMAG(_ch, C_SPR); 

    Map::sectToPen[SECT_CLIMBING].penColor = CCWHT(_ch, C_SPR);
    Map::sectToPen[SECT_CLIMBING].vNoExitColor = CCWHT(_ch, C_SPR);
    Map::sectToPen[SECT_CLIMBING].hNoExitColor = CCWHT(_ch, C_SPR); 

    Map::sectToPen[SECT_FREESPACE].penColor = CCMAG(_ch, C_SPR);
    Map::sectToPen[SECT_FREESPACE].vNoExitColor = CCMAG(_ch, C_SPR);
    Map::sectToPen[SECT_FREESPACE].hNoExitColor = CCMAG(_ch, C_SPR); 

    Map::sectToPen[SECT_ROAD].penColor = CCBLA_BLD(_ch, C_SPR);
    Map::sectToPen[SECT_ROAD].vNoExitColor = CCNRM(_ch, C_SPR);
    Map::sectToPen[SECT_ROAD].hNoExitColor = CCNRM(_ch, C_SPR); 

    Map::sectToPen[SECT_VEHICLE].penColor = CCWHT(_ch, C_SPR);
    Map::sectToPen[SECT_VEHICLE].vNoExitColor = CCWHT(_ch, C_SPR);
    Map::sectToPen[SECT_VEHICLE].hNoExitColor = CCWHT(_ch, C_SPR); 

    Map::sectToPen[SECT_FARMLAND].penColor = CCGRN(_ch, C_SPR);
    Map::sectToPen[SECT_FARMLAND].vNoExitColor = CCGRN(_ch, C_SPR);
    Map::sectToPen[SECT_FARMLAND].hNoExitColor = CCGRN(_ch, C_SPR); 

    Map::sectToPen[SECT_SWAMP].penColor = CCGRN(_ch, C_SPR);
    Map::sectToPen[SECT_SWAMP].vNoExitColor = CCGRN(_ch, C_SPR);
    Map::sectToPen[SECT_SWAMP].hNoExitColor = CCGRN(_ch, C_SPR); 

    Map::sectToPen[SECT_DESERT].penColor = CCYEL_BLD(_ch, C_SPR);
    Map::sectToPen[SECT_DESERT].vNoExitColor = CCYEL_BLD(_ch, C_SPR);
    Map::sectToPen[SECT_DESERT].hNoExitColor = CCYEL_BLD(_ch, C_SPR); 

    Map::sectToPen[SECT_FIRE_RIVER].penColor = CCRED(_ch, C_SPR);
    Map::sectToPen[SECT_FIRE_RIVER].vNoExitColor = CCRED(_ch, C_SPR);
    Map::sectToPen[SECT_FIRE_RIVER].hNoExitColor = CCRED(_ch, C_SPR); 

    Map::sectToPen[SECT_JUNGLE].penColor = CCGRN_BLD(_ch, C_SPR);
    Map::sectToPen[SECT_JUNGLE].vNoExitColor = CCGRN_BLD(_ch, C_SPR);
    Map::sectToPen[SECT_JUNGLE].hNoExitColor = CCGRN_BLD(_ch, C_SPR); 

    Map::sectToPen[SECT_PITCH_PIT].penColor = CCNRM(_ch, C_SPR);
    Map::sectToPen[SECT_PITCH_PIT].vNoExitColor = CCNRM(_ch, C_SPR);
    Map::sectToPen[SECT_PITCH_PIT].hNoExitColor = CCNRM(_ch, C_SPR); 
    
    Map::sectToPen[SECT_PITCH_SUB].penColor = CCBLA_BLD(_ch, C_SPR);
    Map::sectToPen[SECT_PITCH_SUB].vNoExitColor = CCBLA_BLD(_ch, C_SPR);
    Map::sectToPen[SECT_PITCH_SUB].hNoExitColor = CCBLA_BLD(_ch, C_SPR); 

    Map::sectToPen[SECT_BEACH].penColor = CCYEL_BLD(_ch, C_SPR);
    Map::sectToPen[SECT_BEACH].vNoExitColor = CCYEL_BLD(_ch, C_SPR);
    Map::sectToPen[SECT_BEACH].hNoExitColor = CCYEL_BLD(_ch, C_SPR); 

    Map::sectToPen[SECT_ASTRAL].penColor = CCBLA_BLD(_ch, C_SPR);
    Map::sectToPen[SECT_ASTRAL].vNoExitColor = CCBLA_BLD(_ch, C_SPR);
    Map::sectToPen[SECT_ASTRAL].hNoExitColor = CCBLA_BLD(_ch, C_SPR); 

    Map::sectToPen[SECT_ELEMENTAL_FIRE].penColor = CCRED_BLD(_ch, C_SPR);
    Map::sectToPen[SECT_ELEMENTAL_FIRE].vNoExitColor = CCRED_BLD(_ch, C_SPR);
    Map::sectToPen[SECT_ELEMENTAL_FIRE].hNoExitColor = CCRED_BLD(_ch, C_SPR); 
    
    Map::sectToPen[SECT_ELEMENTAL_EARTH].penColor = CCYEL(_ch, C_SPR);
    Map::sectToPen[SECT_ELEMENTAL_EARTH].vNoExitColor = CCYEL(_ch, C_SPR);
    Map::sectToPen[SECT_ELEMENTAL_EARTH].hNoExitColor = CCYEL(_ch, C_SPR); 

    Map::sectToPen[SECT_ELEMENTAL_AIR].penColor = CCCYN_BLD(_ch, C_SPR);
    Map::sectToPen[SECT_ELEMENTAL_AIR].vNoExitColor = CCCYN_BLD(_ch, C_SPR);
    Map::sectToPen[SECT_ELEMENTAL_AIR].hNoExitColor = CCCYN_BLD(_ch, C_SPR); 

    Map::sectToPen[SECT_ELEMENTAL_WATER].penColor = CCBLU_BLD(_ch, C_SPR);
    Map::sectToPen[SECT_ELEMENTAL_WATER].vNoExitColor = CCBLU_BLD(_ch, C_SPR);
    Map::sectToPen[SECT_ELEMENTAL_WATER].hNoExitColor = CCBLU_BLD(_ch, C_SPR); 

    Map::sectToPen[SECT_ELEMENTAL_POSITIVE].penColor = CCWHT(_ch, C_SPR);
    Map::sectToPen[SECT_ELEMENTAL_POSITIVE].vNoExitColor = CCWHT(_ch, C_SPR);
    Map::sectToPen[SECT_ELEMENTAL_POSITIVE].hNoExitColor = CCWHT(_ch, C_SPR); 

    Map::sectToPen[SECT_ELEMENTAL_NEGATIVE].penColor = CCBLA_BLD(_ch, C_SPR);
    Map::sectToPen[SECT_ELEMENTAL_NEGATIVE].vNoExitColor = CCBLA_BLD(_ch, C_SPR);
    Map::sectToPen[SECT_ELEMENTAL_NEGATIVE].hNoExitColor = CCBLA_BLD(_ch, C_SPR); 

    Map::sectToPen[SECT_ELEMENTAL_SMOKE].penColor = CCBLA_BLD(_ch, C_SPR);
    Map::sectToPen[SECT_ELEMENTAL_SMOKE].vNoExitColor = CCBLA_BLD(_ch, C_SPR);
    Map::sectToPen[SECT_ELEMENTAL_SMOKE].hNoExitColor = CCBLA_BLD(_ch, C_SPR); 

    Map::sectToPen[SECT_ELEMENTAL_ICE].penColor = CCCYN_BLD(_ch, C_SPR);
    Map::sectToPen[SECT_ELEMENTAL_ICE].vNoExitColor = CCCYN_BLD(_ch, C_SPR);
    Map::sectToPen[SECT_ELEMENTAL_ICE].hNoExitColor = CCCYN_BLD(_ch, C_SPR); 

    Map::sectToPen[SECT_ELEMENTAL_OOZE].penColor = CCGRN(_ch, C_SPR);
    Map::sectToPen[SECT_ELEMENTAL_OOZE].vNoExitColor = CCGRN(_ch, C_SPR);
    Map::sectToPen[SECT_ELEMENTAL_OOZE].hNoExitColor = CCGRN(_ch, C_SPR); 

    Map::sectToPen[SECT_ELEMENTAL_MAGMA].penColor = CCRED_BLD(_ch, C_SPR);
    Map::sectToPen[SECT_ELEMENTAL_MAGMA].vNoExitColor = CCRED_BLD(_ch, C_SPR);
    Map::sectToPen[SECT_ELEMENTAL_MAGMA].hNoExitColor = CCRED_BLD(_ch, C_SPR); 

    Map::sectToPen[SECT_ELEMENTAL_LIGHTNING].penColor = CCBLU(_ch, C_SPR);
    Map::sectToPen[SECT_ELEMENTAL_LIGHTNING].vNoExitColor = CCBLU(_ch, C_SPR);
    Map::sectToPen[SECT_ELEMENTAL_LIGHTNING].hNoExitColor = CCBLU(_ch, C_SPR); 

    Map::sectToPen[SECT_ELEMENTAL_STEAM].penColor = CCNRM(_ch, C_SPR);
    Map::sectToPen[SECT_ELEMENTAL_STEAM].vNoExitColor = CCNRM(_ch, C_SPR);
    Map::sectToPen[SECT_ELEMENTAL_STEAM].hNoExitColor = CCNRM(_ch, C_SPR); 

    Map::sectToPen[SECT_ELEMENTAL_RADIANCE].penColor = CCNRM(_ch, C_SPR);
    Map::sectToPen[SECT_ELEMENTAL_RADIANCE].vNoExitColor = CCNRM(_ch, C_SPR);
    Map::sectToPen[SECT_ELEMENTAL_RADIANCE].hNoExitColor = CCNRM(_ch, C_SPR); 

    Map::sectToPen[SECT_ELEMENTAL_MINERALS].penColor = CCNRM(_ch, C_SPR);
    Map::sectToPen[SECT_ELEMENTAL_MINERALS].vNoExitColor = CCNRM(_ch, C_SPR);
    Map::sectToPen[SECT_ELEMENTAL_MINERALS].hNoExitColor = CCNRM(_ch, C_SPR); 

    Map::sectToPen[SECT_ELEMENTAL_VACUUM].penColor = CCNRM(_ch, C_SPR);
    Map::sectToPen[SECT_ELEMENTAL_VACUUM].vNoExitColor = CCNRM(_ch, C_SPR);
    Map::sectToPen[SECT_ELEMENTAL_VACUUM].hNoExitColor = CCNRM(_ch, C_SPR); 

    Map::sectToPen[SECT_ELEMENTAL_SALT].penColor = CCNRM(_ch, C_SPR);
    Map::sectToPen[SECT_ELEMENTAL_SALT].vNoExitColor = CCNRM(_ch, C_SPR);
    Map::sectToPen[SECT_ELEMENTAL_SALT].hNoExitColor = CCNRM(_ch, C_SPR); 

    Map::sectToPen[SECT_ELEMENTAL_ASH].penColor = CCNRM(_ch, C_SPR);
    Map::sectToPen[SECT_ELEMENTAL_ASH].vNoExitColor = CCNRM(_ch, C_SPR);
    Map::sectToPen[SECT_ELEMENTAL_ASH].hNoExitColor = CCNRM(_ch, C_SPR); 
    
    Map::sectToPen[SECT_ELEMENTAL_DUST].penColor = CCNRM(_ch, C_SPR);
    Map::sectToPen[SECT_ELEMENTAL_DUST].vNoExitColor = CCNRM(_ch, C_SPR);
    Map::sectToPen[SECT_ELEMENTAL_DUST].hNoExitColor = CCNRM(_ch, C_SPR); 
    
    Map::sectToPen[SECT_BLOOD].penColor = CCRED(_ch, C_SPR);
    Map::sectToPen[SECT_BLOOD].vNoExitColor = CCRED(_ch, C_SPR);
    Map::sectToPen[SECT_BLOOD].hNoExitColor = CCRED(_ch, C_SPR); 

    Map::sectToPen[SECT_ROCK].penColor = CCWHT(_ch, C_SPR);
    Map::sectToPen[SECT_ROCK].vNoExitColor = CCWHT(_ch, C_SPR);
    Map::sectToPen[SECT_ROCK].hNoExitColor = CCWHT(_ch, C_SPR); 
    
    Map::sectToPen[SECT_MUDDY].penColor = CCYEL(_ch, C_SPR);
    Map::sectToPen[SECT_MUDDY].vNoExitColor = CCYEL(_ch, C_SPR);
    Map::sectToPen[SECT_MUDDY].hNoExitColor = CCYEL(_ch, C_SPR); 

    Map::sectToPen[SECT_TRAIL].penColor = CCYEL(_ch, C_SPR);
    Map::sectToPen[SECT_TRAIL].vNoExitColor = CCYEL(_ch, C_SPR);
    Map::sectToPen[SECT_TRAIL].hNoExitColor = CCYEL(_ch, C_SPR); 

    Map::sectToPen[SECT_TUNDRA].penColor = CCYEL(_ch, C_SPR);
    Map::sectToPen[SECT_TUNDRA].vNoExitColor = CCYEL(_ch, C_SPR);
    Map::sectToPen[SECT_TUNDRA].hNoExitColor = CCYEL(_ch, C_SPR); 

    Map::sectToPen[SECT_CATACOMBS].penColor = CCNRM(_ch, C_SPR);
    Map::sectToPen[SECT_CATACOMBS].vNoExitColor = CCBLA_BLD(_ch, C_SPR);
    Map::sectToPen[SECT_CATACOMBS].hNoExitColor = CCBLA_BLD(_ch, C_SPR); 

    Map::sectToPen[SECT_CRACKED_ROAD].penColor = CCNRM(_ch, C_SPR);
    Map::sectToPen[SECT_CRACKED_ROAD].vNoExitColor = CCNRM(_ch, C_SPR);
    Map::sectToPen[SECT_CRACKED_ROAD].hNoExitColor = CCNRM(_ch, C_SPR); 

    Map::sectToPen[SECT_DEEP_OCEAN].penColor = CCBLU(_ch, C_SPR);
    Map::sectToPen[SECT_DEEP_OCEAN].vNoExitColor = CCBLU(_ch, C_SPR);
    Map::sectToPen[SECT_DEEP_OCEAN].hNoExitColor = CCBLU(_ch, C_SPR); 
};

#define MARK(room) ( room->find_first_step_index = find_first_step_index )
#define UNMARK(room) ( room->find_first_step_index = 0 )
#define IS_MARKED(room) ( room->find_first_step_index == find_first_step_index )
extern unsigned char find_first_step_index;

bool
Map::build()
{
    int x, y, i;
    MapPixel pix2;
    room_direction_data *theExit = NULL;

    if (!_ch->in_room)
        return false;

    if (!_hsize || !_vsize)
        return false;

    // Is there at least one mappable exit?
    for (i = 0; i < 6; i++) {
        if (_ch->in_room->dir_option[i] && _ch->in_room->dir_option[i]->to_room != NULL)
            break;
    }
    if (i >= 6) {				// no exits
        return false;
    }

    ++find_first_step_index;

    if (!find_first_step_index) {
        ++find_first_step_index;

        room_data *curr_room;
        zone_data *zone;
        for (zone = zone_table; zone; zone = zone->next)
            for (curr_room = zone->world; curr_room; curr_room = curr_room->next)
                UNMARK(curr_room);
    }

    // Find the center
    x = (int)floorf((float)(_hsize / 2));
    y = (int)floorf((float)(_vsize / 2));
    MapPixel::centerX = x;
    MapPixel::centerY = y;
    MapPixel::hSize = _hsize;
    MapPixel::vSize = _vsize;

    // Queue up the first room
    MapPixel pix(x, y, _ch->in_room);
    pix.setRoomPointer(_ch->in_room);
    _roomQueue.push(pix);

    // Process the queue
    while (!_roomQueue.empty()) {
        pix = _roomQueue.front(); 
		_roomQueue.pop();

        // Do nothing with this for the moment
        if ((*this)[(pix.getYCoord() * _hsize) + pix.getXCoord()].getRoomPointer())
            continue;

        if (!IS_MARKED(pix.getRoomPointer())) {
            (*this)[(pix.getYCoord() * _hsize) + pix.getXCoord()] = pix;
            MARK(pix.getRoomPointer());
        }
        else
            continue;

        // Push all the theExits of this room onto the room queue, assuming that they are in bounds
        theExit = pix.getRoomPointer()->dir_option[North];
        if (theExit && theExit->to_room && pix.getYCoord() - 1 >= 0) {
            if (!IS_SET(theExit->exit_info, EX_CLOSED)) {
                pix2.setXCoord(pix.getXCoord());
                pix2.setYCoord(pix.getYCoord() - 1);
                pix2.setRoomPointer(theExit->to_room);
                _roomQueue.push(pix2);
            }
        }

        theExit = pix.getRoomPointer()->dir_option[South];
        if (theExit && theExit->to_room && pix.getYCoord() + 1 < _vsize) {
            if (!IS_SET(theExit->exit_info, EX_CLOSED)) {
                pix2.setXCoord(pix.getXCoord());
                pix2.setYCoord(pix.getYCoord() + 1);
                pix2.setRoomPointer(theExit->to_room);
                _roomQueue.push(pix2);
            }
        }

        theExit = pix.getRoomPointer()->dir_option[East];
        if (theExit && theExit->to_room && pix.getXCoord() + 1 < _hsize) {
            if (!IS_SET(theExit->exit_info, EX_CLOSED)) {
                pix2.setXCoord(pix.getXCoord() + 1);
                pix2.setYCoord(pix.getYCoord());
                pix2.setRoomPointer(theExit->to_room);
                _roomQueue.push(pix2);
            }
        }

        theExit = pix.getRoomPointer()->dir_option[West];
        if (theExit && theExit->to_room && pix.getXCoord() - 1 >= 0) {
            if (!IS_SET(theExit->exit_info, EX_CLOSED)) {
                pix2.setXCoord(pix.getXCoord() - 1);
                pix2.setYCoord(pix.getYCoord());
                pix2.setRoomPointer(theExit->to_room);
                _roomQueue.push(pix2);
            }
        }
    }

    Map::iterator it;
    for (int i = 0; i < _vsize * 2 - 1; i++) {
        for (int j = 0; j < (_hsize * 2) + 1; j++) {
            it = find(j, i);
            if (it != end()) {
                if (find(j, i - 1) == end())
                    it->setEdge(North);
                if (find(j, i + 1) == end())
                    it->setEdge(South);
                if (find(j - 1, i) == end())
                    it->setEdge(West);
                if (find(j + 1, i) == end())
                    it->setEdge(East);
            }
        }
    }

    return draw();
}

void
Map::display() {
    string line;
    const char *indent = "    ";
    send_to_char(_ch, "%s%s.%s.\r\n", CCNRM(_ch, C_SPR), indent, tmp_pad('-', (_hsize * 2) + 1));
    for (int i = 0; i < _vsize * 2 - 1; i++) {
        for (int j = 0; j < (_hsize * 2) + 1; j++) {
            line += _map[i * ((_hsize * 2) + 1) + j];
        }
        send_to_char(_ch, "%s%s|%s%s|\r\n", CCNRM(_ch, C_SPR), indent, line.c_str(), 
                CCNRM(_ch, C_SPR));
        line.clear();
    }
    send_to_char(_ch, "%s%s`%s'\r\n", CCNRM(_ch, C_SPR), indent, tmp_pad('-', (_hsize * 2) + 1));
}

Map::iterator
Map::find(int x, int y) {
    Map::iterator vi;

    for (vi = begin(); vi != end(); vi++) {
        if (vi->getXCoord() == x && vi->getYCoord() == y)
            return vi;
    }

    return this->end();
}

bool
Map::draw() {
    Map::iterator it;
    // Moving left to right
    for (int i = 0; i < _vsize; i++) {
        for (int j = 0; j < _hsize; j++) {
            if ((it = find(j, i)) != end()) {
                if (!it->draw(_map))
                    return false;
            }
        }
    }

    return true;
};

bool 
MapPixel::drawDoor(int d, vector<string> &_map, string pen) {
    int hsize = (MapPixel::hSize * 2) + 1;
    int index = int((float)((_yCoord * 2) * (hsize)) + (float)(_xCoord * 2) + 1);
    room_direction_data *exit = NULL;
    string doorPen;

    if (!_r)
        return false;

    if (pen == "")
        doorPen = _doorPen;
    else
        doorPen = pen;

    _sectToPen *sectInfo = &Map::sectToPen[SECT_TYPE(_r)];

    if (d == North && _yCoord > 0) {
        if (validEdge(North)) {
            exit = _r->dir_option[North];
            if (IS_SET(exit->exit_info, EX_HIDDEN) || IS_SET(exit->exit_info, EX_SECRET)) {
                if (!Map::_ch->isImmortal()) {
                    _map[index - hsize] = sectInfo->hNoExitColor + sectInfo->hNorthNoExitPen + KNRM;
                    return false;
                }
                _map[index - hsize] = CCYEL(Map::_ch, C_SPR);
                _map[index - hsize] += CCBLD(Map::_ch, C_CMP) + doorPen + KNRM;
                return true;
            }
            if (IS_SET(exit->exit_info, EX_ISDOOR) && IS_SET(exit->exit_info, EX_CLOSED)) {
                _map[index - hsize] = CCBLU(Map::_ch, C_SPR);
                _map[index - hsize] += CCBLD(Map::_ch, C_CMP) +  doorPen + KNRM;
                return true;
            }
        }
    }
    else if (d == South && _yCoord != MapPixel::vSize) {
        if (validEdge(South)) {
            exit = _r->dir_option[South];
            if (IS_SET(exit->exit_info, EX_HIDDEN) || IS_SET(exit->exit_info, EX_SECRET)) {
                if (!Map::_ch->isImmortal()) {
                    _map[index + hsize] = sectInfo->hNoExitColor + sectInfo->hSouthNoExitPen + KNRM;
                    return false;
                }

                _map[index + hsize] = CCYEL(Map::_ch, C_SPR);
                _map[index + hsize] += CCBLD(Map::_ch, C_CMP) + doorPen + KNRM;
                return true;
            }
            if (IS_SET(exit->exit_info, EX_ISDOOR) && IS_SET(exit->exit_info, EX_CLOSED)) {
                _map[index + hsize] = CCBLU(Map::_ch, C_SPR);
                _map[index + hsize] += CCBLD(Map::_ch, C_CMP) +  doorPen + KNRM;
                return true;
            }
        }
    }
    else if (d == East) {
        if (validEdge(East)) {
            exit = _r->dir_option[East];
            if (IS_SET(exit->exit_info, EX_HIDDEN) || IS_SET(exit->exit_info, EX_SECRET)) {
                if (!Map::_ch->isImmortal()) {
                    _map[index + 1] = sectInfo->hNoExitColor + sectInfo->vNoExitPen + KNRM;
                    return false;
                }
                _map[index + 1] = CCYEL(Map::_ch, C_SPR);
                _map[index + 1] += CCBLD(Map::_ch, C_CMP) + doorPen + KNRM;
                return true;
            }
            if (IS_SET(exit->exit_info, EX_ISDOOR) && IS_SET(exit->exit_info, EX_CLOSED)) {
                _map[index + 1] = CCBLU(Map::_ch, C_SPR);
                _map[index + 1] += CCBLD(Map::_ch, C_SPR) + doorPen + KNRM;
                return true;
            }
        }
    }
    else if (d == West) {
        if (validEdge(West)) {
            exit = _r->dir_option[West];
            if (IS_SET(exit->exit_info, EX_HIDDEN) || IS_SET(exit->exit_info, EX_SECRET)) {
                if (!Map::_ch->isImmortal()) {
                    _map[index - 1] = sectInfo->hNoExitColor + sectInfo->vNoExitPen + KNRM;
                    return false;
                }

                _map[index - 1] = CCYEL(Map::_ch, C_SPR);
                _map[index - 1] += CCBLD(Map::_ch, C_CMP) + doorPen + KNRM;
                return true;
            }
            if (IS_SET(exit->exit_info, EX_ISDOOR) && IS_SET(exit->exit_info, EX_CLOSED)) {
                _map[index - 1] = CCBLU(Map::_ch, C_SPR);
                _map[index - 1] += CCBLD(Map::_ch, C_CMP) + doorPen + KNRM;
                return true;
            }
        }
    }
    else if (d == Up) {
        if (validEdge(Up)) {
            exit = _r->dir_option[Up];
            if (IS_SET(exit->exit_info, EX_HIDDEN) || IS_SET(exit->exit_info, EX_SECRET)) {
                if (Map::_ch->isImmortal()) {
                    _map[index] = CCYEL(Map::_ch, C_SPR);
                    _map[index] = CCBLD(Map::_ch, C_CMP) + doorPen + KNRM;
                    return true;
                }
                return false;
            }

            _map[index] = CCRED(Map::_ch, C_SPR);
            _map[index] += CCBLD(Map::_ch, C_CMP) + doorPen + KNRM;
            return true;
        }
    }
    else if (d == Down) {
        if (validEdge(Down)) {
            exit = _r->dir_option[Down];
            if (IS_SET(exit->exit_info, EX_HIDDEN) || IS_SET(exit->exit_info, EX_SECRET)) {
                if (Map::_ch->isImmortal()) {
                    _map[index] = CCYEL(Map::_ch, C_SPR);
                    _map[index] += CCBLD(Map::_ch, C_CMP) + doorPen + KNRM;
                    return true;
                }
                return false;
            }

            _map[index] = CCMAG(Map::_ch, C_SPR);
            _map[index] += CCBLD(Map::_ch, C_CMP) + doorPen + KNRM;
            return true;
        }
    }

    return false;
}

void 
MapPixel::drawTerrain(int d, vector<string> &_map) {
    int hsize = (MapPixel::hSize * 2) + 1;
    int index = int((float)((_yCoord * 2) * (hsize)) + (float)(_xCoord * 2) + 1);
    _sectToPen *sectInfo = &Map::sectToPen[SECT_TYPE(_r)];

    if (!_r)
        return;

    if (d == North && _yCoord > 0) {
        if (validEdge(North))
            _map[index - hsize] = sectInfo->penColor + sectInfo->pen + KNRM;
        else
            _map[index - hsize] = sectInfo->hNoExitColor + sectInfo->hNorthNoExitPen + KNRM;
    }
    else if (d == South && _yCoord < MapPixel::vSize) {
        if (validEdge(South))
            _map[(index + hsize)] = sectInfo->penColor + sectInfo->pen + KNRM;
        else
            _map[(index + hsize)] = sectInfo->hNoExitColor + sectInfo->hSouthNoExitPen + KNRM;
    }
    else if (d == East) {
        if (validEdge(East))
            _map[index + 1] = sectInfo->penColor + sectInfo->pen + KNRM;
        else
            _map[index + 1] = sectInfo->vNoExitColor + sectInfo->vNoExitPen + KNRM;
    }
    else if (d == West) {
        if (validEdge(West))
            _map[index - 1] = sectInfo->penColor + sectInfo->pen + KNRM;
        else
            _map[index - 1] = sectInfo->vNoExitColor + sectInfo->vNoExitPen + KNRM;
    }
}

void
MapPixel::drawOneCorner(int d, vector<string> &_map) {
    struct room_data *theRoom, *theRoom2;
    int hsize = (MapPixel::hSize * 2) + 1;
    int index = int((float)((_yCoord * 2) * (hsize)) + (float)(_xCoord * 2) + 1);
    _sectToPen *sectInfo = &Map::sectToPen[SECT_TYPE(_r)];

    if (!_r)
        return;

    if (d == NorthWest && _yCoord > 0) {
        if ((theRoom = validEdge(North)) && (theRoom2 = validEdge(West)) &&
             validEdge(West, theRoom) && validEdge(North, theRoom2)) {
            _map[index - hsize - 1] = sectInfo->penColor + sectInfo->pen + KNRM;
        }
        else if (validEdge(North)){
            _map[index - hsize - 1] = sectInfo->vNoExitColor + sectInfo->vNoExitPen + KNRM;
        }
        else
            _map[index - hsize - 1] = sectInfo->hNoExitColor + sectInfo->hNorthNoExitPen + KNRM;
    }

    if (d == NorthEast && _yCoord > 0) {
        if ((theRoom = validEdge(North)) && (theRoom2 = validEdge(East)) &&
             validEdge(East, theRoom) && validEdge(North, theRoom2)) {
            _map[index - hsize + 1] = sectInfo->penColor + sectInfo->pen + KNRM;
        }
        else if (validEdge(North)){
            _map[index - hsize + 1] = sectInfo->vNoExitColor + sectInfo->vNoExitPen + KNRM;
        }
        else
            _map[index - hsize + 1] = sectInfo->hNoExitColor + sectInfo->hNorthNoExitPen + KNRM;
    }

    if (d == SouthWest && _yCoord < MapPixel::vSize) {
        if ((theRoom = validEdge(South)) && (theRoom2 = validEdge(West)) &&
             validEdge(West, theRoom) && validEdge(South, theRoom2)) {
            _map[index + hsize - 1] = sectInfo->penColor + sectInfo->pen + KNRM;
        }
        else if (validEdge(South)){
            _map[index + hsize - 1] = sectInfo->vNoExitColor + sectInfo->vNoExitPen + KNRM;
        }
        else
            _map[index + hsize - 1] = sectInfo->hNoExitColor + sectInfo->hSouthNoExitPen + KNRM;
    }

    if (d == SouthEast && _yCoord < MapPixel::vSize) {
        if ((theRoom = validEdge(South)) && (theRoom2 = validEdge(East)) && 
             validEdge(East, theRoom) && validEdge(South, theRoom2)) {
            _map[index + hsize + 1] = sectInfo->penColor + sectInfo->pen + KNRM;
        }
        else if (validEdge(South)){
            _map[index + hsize + 1] = sectInfo->vNoExitColor + sectInfo->vNoExitPen + KNRM;
        }
        else
            _map[index + hsize + 1] = sectInfo->hNoExitColor + sectInfo->hSouthNoExitPen + KNRM;
    }
}

bool
MapPixel::draw(vector<string> &_map) {
    if (!_r)
        return true;

    int index = int((float)((_yCoord * 2) * ((MapPixel::hSize * 2) + 1)) + (float)(_xCoord * 2) + 1);
    _sectToPen *sectInfo = &Map::sectToPen[SECT_TYPE(_r)];

    // Currently, the rooms will arrive to this function starting in the Northwest and ending
    // in the Southeast.
    
    // Center room.  It wants to draw everything in every direction
    if (_xCoord == MapPixel::centerX && _yCoord == MapPixel::centerY) {
        drawOneCorner(NorthWest, _map);
        drawOneCorner(NorthEast, _map);
        drawOneCorner(SouthWest, _map);
        drawOneCorner(SouthEast, _map);

        for (int x = 0; x < 4; x++) {
            drawTerrain(x, _map);
            drawDoor(x, _map);
        }

        if (!drawDoor(Up, _map, "*") && !drawDoor(Down, _map, "*")) {
            _map[index] = CCGRN(Map::_ch, C_SPR);
            _map[index] += CCBLD(Map::_ch, C_CMP);
            _map[index] += "*";
            _map[index] += KNRM;
        }
    }
    else  {
        // Northwest
        if (_xCoord < MapPixel::centerX && _yCoord < MapPixel::centerY) {
            drawOneCorner(NorthWest, _map);
            drawTerrain(North, _map);
            drawTerrain(West, _map);
            drawDoor(North, _map);
            drawDoor(West, _map);

            if (_isEastEdge) {
                drawTerrain(East, _map);
                drawDoor(East, _map);
                drawOneCorner(NorthEast, _map);
                drawOneCorner(SouthEast, _map);
            }

            if (_isSouthEdge) {
                drawTerrain(South, _map);
                drawDoor(South, _map);
                drawOneCorner(SouthWest, _map);
                drawOneCorner(SouthEast, _map);
            }
        }

        // North
        else if (_xCoord == MapPixel::centerX && _yCoord < MapPixel::centerY) {
            drawOneCorner(NorthWest, _map);
            drawOneCorner(NorthEast, _map);
            drawTerrain(North, _map);
            drawTerrain(East, _map);
            drawTerrain(West, _map);
            drawDoor(North, _map);
            drawDoor(East, _map);
            drawDoor(West, _map);

            if (_isSouthEdge) {
                drawTerrain(South, _map);
                drawDoor(South, _map);
                drawOneCorner(SouthWest, _map);
                drawOneCorner(SouthEast, _map);
            }
        }
        // Northeast
        else if (_xCoord > MapPixel::centerX && _yCoord < MapPixel::centerY) {
            drawOneCorner(NorthEast, _map);
            drawTerrain(North, _map);
            drawTerrain(East, _map);
            drawDoor(North, _map);
            drawDoor(East, _map);

            if (_isWestEdge) {
                drawTerrain(West, _map);
                drawDoor(West, _map);
                drawOneCorner(NorthWest, _map);
                drawOneCorner(SouthWest, _map);
            }
            if (_isSouthEdge) {
                drawTerrain(South, _map);
                drawDoor(South, _map);
                drawOneCorner(SouthWest, _map);
                drawOneCorner(SouthEast, _map);
            }
        }
        // West
        else if (_yCoord == MapPixel::centerY && _xCoord < MapPixel::centerX) {
            drawOneCorner(SouthWest, _map);
            drawOneCorner(NorthWest, _map);
            drawTerrain(North, _map);
            drawTerrain(West, _map);
            drawTerrain(South, _map);
            drawDoor(North, _map);
            drawDoor(West, _map);
            drawDoor(South, _map);

            if (_isEastEdge) {
                drawTerrain(East, _map);
                drawDoor(East, _map);
                drawOneCorner(NorthEast, _map);
                drawOneCorner(SouthEast, _map);
            }
        }
        // East
        else if (_yCoord == MapPixel::centerY && _xCoord > MapPixel::centerX) {
            drawOneCorner(NorthEast, _map);
            drawOneCorner(SouthEast, _map);
            drawTerrain(North, _map);
            drawTerrain(East, _map);
            drawTerrain(South, _map);
            drawDoor(North, _map);
            drawDoor(East, _map);
            drawDoor(South, _map);

            if (_isWestEdge) {
                drawTerrain(West, _map);
                drawDoor(West, _map);
                drawOneCorner(NorthWest, _map);
                drawOneCorner(SouthWest, _map);
            }
        }
        // SouthWest
        else if (_xCoord < MapPixel::centerX && _yCoord > MapPixel::centerY) {
            drawOneCorner(SouthWest, _map);
            drawTerrain(South, _map);
            drawTerrain(West, _map);
            drawDoor(South, _map);
            drawDoor(West, _map);

            if (_isEastEdge) {
                drawTerrain(East, _map);
                drawDoor(East, _map);
                drawOneCorner(NorthEast, _map);
                drawOneCorner(SouthEast, _map);
            }
            if (_isNorthEdge) {
                drawTerrain(North, _map);
                drawDoor(North, _map);
                drawOneCorner(NorthEast, _map);
                drawOneCorner(NorthWest, _map);
            }
        }
        // South
        else if (_xCoord == MapPixel::centerX && _yCoord > MapPixel::centerY) {
            drawOneCorner(SouthWest, _map);
            drawOneCorner(SouthEast, _map);
            drawTerrain(South, _map);
            drawTerrain(East, _map);
            drawTerrain(West, _map);
            drawDoor(South, _map);
            drawDoor(East, _map);
            drawDoor(West, _map);

            if (_isNorthEdge) {
                drawTerrain(North, _map);
                drawDoor(North, _map);
                drawOneCorner(NorthEast, _map);
                drawOneCorner(NorthWest, _map);
            }
        }
        // SouthEast
        else if (_xCoord > MapPixel::centerX && _yCoord > MapPixel::centerY) {
            drawOneCorner(SouthEast, _map);
            drawTerrain(South, _map);
            drawTerrain(East, _map);
            drawDoor(South, _map);
            drawDoor(East, _map);

            if (_isNorthEdge) {
                drawTerrain(North, _map);
                drawDoor(North, _map);
                drawOneCorner(NorthEast, _map);
                drawOneCorner(NorthWest, _map);
            }
            if (_isWestEdge) {
                drawTerrain(West, _map);
                drawDoor(West, _map);
                drawOneCorner(NorthWest, _map);
                drawOneCorner(SouthWest, _map);
            }
        }

        if (!drawDoor(Up, _map, sectInfo->pen) && !drawDoor(Down, _map, sectInfo->pen))
            _map[index] = sectInfo->penColor + sectInfo->pen + KNRM;
    }

    return true;
}


