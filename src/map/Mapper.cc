#include <string>
using namespace std;
#include "Mapper.h"
#include <signal.h>

bool CAN_EDIT_ZONE(CHAR *ch, struct zone_data *zone);

ACMD(do_map) {
    int rows;
    int columns;
    if(IS_NPC(ch)) {
        send_to_char("You scribble out a map on the ground.\r\n",ch);
        return;
    }
    if(GET_LEVEL(ch) < LVL_DEMI && !CAN_EDIT_ZONE(ch,ch->in_room->zone)) {
        send_to_char("You can't map this zone.\r\n",ch);
        return;
    }
    
    argument = one_argument(argument, buf);
    if(!strcmp(buf,"small"))  {
        columns = rows = GET_PAGE_LENGTH(ch)/4;
    } else if(!strcmp(buf,"medium"))  {
        columns = rows = GET_PAGE_LENGTH(ch)/2;
    } else if(!strcmp(buf,"large"))  {
        columns = rows = GET_PAGE_LENGTH(ch);
    } else {
        columns = rows = GET_PAGE_LENGTH(ch)/2;
    }
    
    Mapper theMap(ch,rows,columns);
    if(theMap.build()) {
        theMap.display(rows,columns);
        if(theMap.full) {
            send_to_char("Room mapping limit reached. Some rooms not mapped.\r\n",ch);
        }
        sprintf(buf,"%d room's processed.\r\n",theMap.processed);
        send_to_char(buf,ch);
    }
    WAIT_STATE(ch,1 RL_SEC);
}
MapToken::MapToken( int d, int r, int c, room_data *s, room_data *t ) {
    direction = d; row = r; column = c; source = s; target = t;
    targetID = t->number;
    if( source == NULL || target == NULL ) {
        fprintf(stderr,"Error creating token.\r\n");
        raise(666);
    }
    next = NULL;
}
Mapper::Mapper(char_data *ch,int rows, int columns) {
    mapDisplay = new MapPixel[rows * columns];
    mapStack = NULL;
    this->rows = rows;
    this->columns = columns;
    this->ch = ch;
    processed = 0;
    size = 0;
    maxSize = 0;
    full = false;
}
Mapper::~Mapper() {
    if(mapDisplay != NULL) {
        delete [] mapDisplay;
        mapDisplay = NULL;
    }
}
void Mapper::display(int bRows,int bCols) {
    string line;
    char pen;
    int exits;
    char terrain;
    int row,col;
    row = col = 0;
    MapPixel *pixel;
    for ( row = 0;row < bRows;row++) {
        for (col = 0;col < bCols;col++) {
            if(!(validRow(row) && validColumn(col))) {
                fprintf(stderr,"Mapper::display - Invalid row/col [%d,%d]\r\n",row,col);
                continue;
            }
            pixel = mapDisplay + (row * columns + col);
            terrain = (*pixel).terrain;
            exits = (*pixel).exits;
            switch(terrain) {
                case -1:
                    pen = ' ';
                    break;
                 case SECT_INSIDE: pen = 'I'; break;
                 case SECT_CITY: pen = 'C'; break;
                 case SECT_FIELD: pen = 'F'; break;
                 case SECT_FOREST: pen = 'F'; break;
                 case SECT_HILLS: pen = 'H'; break;
                 case SECT_MOUNTAIN: pen = 'M'; break;
                 case SECT_WATER_SWIM: pen = 'W'; break;
                 case SECT_WATER_NOSWIM: pen = 'W'; break;
                 case SECT_UNDERWATER: pen = 'U'; break;
                 case SECT_FLYING: pen = 'A'; break;
                 case SECT_NOTIME: pen = '?'; break;
                 case SECT_CLIMBING: pen = 'M'; break;
                 case SECT_FREESPACE: pen = 'O'; break;
                 case SECT_ROAD: pen = 'R'; break;
                 case SECT_VEHICLE: pen = 'I'; break;
                 case SECT_CORNFIELD: pen = 'F'; break;
                 case SECT_SWAMP: pen = 'S'; break;
                 case SECT_DESERT: pen = 'D'; break;
                 case SECT_FIRE_RIVER: pen = 'R'; break;
                 case SECT_JUNGLE: pen = 'J'; break;
                 case SECT_PITCH_PIT: pen = 'P'; break;
                 case SECT_PITCH_SUB: pen = 'P'; break;
                 case SECT_BEACH: pen = 'B'; break;
                 case SECT_ASTRAL: pen = 'A'; break;
                 case SECT_ELEMENTAL_FIRE: pen = 'F'; break;
                 case SECT_ELEMENTAL_EARTH: pen = 'E'; break;
                 case SECT_ELEMENTAL_AIR: pen = 'A'; break;
                 case SECT_ELEMENTAL_WATER: pen = 'W'; break;
                 case SECT_ELEMENTAL_POSITIVE: pen = 'P'; break;
                 case SECT_ELEMENTAL_NEGATIVE: pen = 'N'; break;
                 case SECT_ELEMENTAL_SMOKE: pen = 'S'; break;
                 case SECT_ELEMENTAL_ICE: pen = 'I'; break;
                 case SECT_ELEMENTAL_OOZE: pen = 'O'; break;
                 case SECT_ELEMENTAL_MAGMA: pen = 'M'; break;
                 case SECT_ELEMENTAL_LIGHTNING: pen = 'L'; break;
                 case SECT_ELEMENTAL_STEAM: pen = 'S'; break;
                 case SECT_ELEMENTAL_RADIANCE: pen = 'R'; break;
                 case SECT_ELEMENTAL_MINERALS: pen = 'M'; break;
                 case SECT_ELEMENTAL_VACUUM: pen = 'V'; break;
                 case SECT_ELEMENTAL_SALT: pen = 'S'; break;
                 case SECT_ELEMENTAL_ASH: pen = 'A'; break;
                 case SECT_ELEMENTAL_DUST: pen = 'D'; break;
                 case SECT_BLOOD: pen = 'B'; break;
                case 125://    // Current Position
                    pen = '*';
                    exits = 5;
                    break;
                case 126://    // |
                    pen = '|';
                    break;
                case 127://    // -
                    pen = '-';
                    break;
                default:
                    pen = '?';
                    break;
            }
            switch(exits) {   // 0 - 6
                case 0:
                    line += pen;
                    break;
                case 1:        // Up
                    line += CCCYN(ch,C_NRM);
                    line += pen;
                    line += CCNRM(ch,C_NRM);
                    break;
                case 2:        // Down
                    line += CCGRN(ch,C_NRM);
                    line += pen;
                    line += CCNRM(ch,C_NRM);
                    break;
                case 3:        // Up and Down
                    line += CCYEL(ch,C_NRM);
                    line += pen;
                    line += CCNRM(ch,C_NRM);
                    break;
                case 4:        // Zone border
                    line += CCRED(ch,C_NRM);
                    line += pen;
                    line += CCNRM(ch,C_NRM);
                    break;
                case 5:        // Current position or invalid zone
                    line += CCMAG(ch,C_NRM);
                    line += pen;
                    line += CCNRM(ch,C_NRM);
                    break;
                default:
                    line += ' ';
                    break;
            }
        }
        strcpy(buf,line.c_str());
        strcat(buf,"\r\n");
        send_to_char(buf, ch);
        line.erase();
    }
}
    
bool Mapper::drawRoom( room_data *s,room_data *t,long row, long col) {
    int position = (row * columns) + col;
    short exits = 0;
    // if this is an obvious loop room, drop it
    if (t == s)
        return false;
    // If we are out of bounds, drop out of this room.
    if(!validRow(row) || !validColumn(col)) 
        return false;

    // Check for existing map symbol
    // if there is one, just return.
    if  ( !( mapDisplay[position].mapped ) ) {
        if(t->zone != curZone) {
            exits = 4;
        } else {
            // Check for links up and down.
            if (t->dir_option[Up] && t->dir_option[Up]->to_room)
                exits += 1;
            if (t->dir_option[Down] && t->dir_option[Down]->to_room)
                exits += 2;
        }

        mapDisplay[position].terrain = t->sector_type;
        mapDisplay[position].exits = exits;
        return true;
    }
    return false;
}

void Mapper::drawLink    (
                room_data *s, // Source Room
                room_data *t, // Target Room
                int row,      // Target's row
                int col,      // Target's col
                bool justLink //are we drawing only the link?
                )
    {
    // Check for doors (when doors exist) adding + instead of | or -
    short i = 0;            // loop counters
    short sExit,tExit;    // which exits actually connect
    sExit = tExit = -1;
    // if this is an obvious loop room, drop it
    if ( s == t )
        return;    
    
    while(i < 4) {
        if (s && s->dir_option[i] && s->dir_option[i]->to_room == t)// Find the source->target link
            sExit = i;
        if (t->dir_option[i] && t->dir_option[i]->to_room == s)// Find the target->source link
            tExit = i;
        i++;
    }
    int r;
    int c;
    // Write in the link characters. remember that +1 is 1 lower on the page.
    // 29 is | 30 is -
    if (sExit == North && tExit == South) {    // Normal north/south links
        r = row + 1;
        c = col;
        if( validRow( r ) && validColumn( c )) {
            mapDisplay[ (r * columns) + col].terrain = 126;
            mapDisplay[ (r * columns) + col].exits = 0;
        }
    } else if(sExit == South && tExit == North) {
        r = row - 1;
        c = col;
        if( validRow( r ) && validColumn( col )) {
            mapDisplay[ (r * columns) + col].terrain = 126;
            mapDisplay[ (r * columns) + col].exits = 0;        
        }
    } else if(sExit == East && tExit == West)  {   // Normal east/west links
        r = row;
        c = col - 1;
        if( validRow( r ) && validColumn( c )) {
            mapDisplay[ (r * columns) + c ].terrain = 127;
            mapDisplay[ (r * columns) + c ].exits = 0;
         }
    } else if(sExit == West && tExit == East) {
        r = row;
        c = col + 1;
        if( validRow( r ) && validColumn( c )) {
            mapDisplay[ (r * columns) + c ].terrain = 127;
            mapDisplay[ (r * columns) + c ].exits = 0;
        }
    }
   // Here starts the wierd links. (round rooms and the like)
   //    **    One way exits with no return to any room    **
   //
   // Note : This does not apply to a room that has a
   //            return link to a different room
   //
    else if( tExit == -1 && t->dir_option[sExit] && t->dir_option[sExit]->to_room == NULL ) {
        // north-> link
        if( sExit == North ) {
            r = row + 1;
            c = col;
            if( validRow( r ) && validColumn( c )) {
                mapDisplay[ ( r )* columns + ( c )].terrain = 126;
                mapDisplay[ ( r )* columns + ( c )].exits = 0;
            }
        // south-> link
        } else if    ( sExit == South ) {
            r = row - 1;
            c = col;
            if( validRow( r ) && validColumn( c )) {
                mapDisplay[ ( r )* columns + ( c )].terrain = 126;
                mapDisplay[ ( r )* columns + ( c )].exits = 0;
            }
        // east -> link
        } else if    ( sExit == East ) {
            r = row;
            c = col - 1;
            if( validRow( r ) && validColumn( c )) {
                mapDisplay[ ( r )* columns + ( c )].terrain = 126;
                mapDisplay[ ( r )* columns + ( c )].exits = 0;
            }
        // west -> link
        } else if    ( sExit == West ) {
            r = row;
            c = col + 1;
            if( validRow( r ) && validColumn( c )) {
                mapDisplay[ ( r )* columns + ( c )].terrain = 126;
                mapDisplay[ ( r )* columns + ( c )].exits = 0;
            }
        }
    }
    
    // top left corner of a round room
    //else if (sExit == exitN && tExit == exitW && InBounds(buf,row + 1, col)) { }
    // top right corner of a round room
    //else if (sExit == exitN && tExit == exitE && InBounds(buf,row + 1, col + 2)) { }
    // bottom left corner of a round room
    //else if (sExit == exitS && tExit == exitW && InBounds(buf,row, col)) { }
    // bottom right corner of a round room
    //else if (sExit == exitS && tExit == exitE && InBounds(buf,row, col + 2)) { }
    
    // What other special cases are there? loop rooms?

    return;
}
const static int mapBits [] = { 1, 2, 4, 8, 16, 32, 64, 128, 256, 512 };

static inline bool MAPPED(room_data *mappedRoom, int mappedDirection) {
    return (mappedRoom->find_first_step_index & mapBits[mappedDirection]);
}
static inline bool MAP(room_data *mappedRoom, int mappedDirection) {
    return (mappedRoom->find_first_step_index |= mapBits[mappedDirection]);
}
bool Mapper::build() {
    int i;
    long row,col;
    room_data *curRoom;
    MapToken *token = NULL;
    MapToken *curToken;
    

    // Make sure we are really going to map.

    curRoom = ch->in_room;
    curZone = curRoom->zone;
    // Is there at least one mappable exit?
    for(i = 0;i < 4;i++) {
        if(curRoom->dir_option[i] && curRoom->dir_option[i]->to_room != NULL) 
            break;
    }
    if(i >= 4) { // no exits
        send_to_char("You glance around and take note of your vast surroundings.\r\n",ch);
        return false;
    }

    // Actually start maping

    // Set up local environ
    row = rows/2;//kRTerm / 2;
    col = columns/2;//kCTerm / 2;
    
	for (zone_data *zone = zone_table; zone; zone = zone->next)
	    for (curRoom = zone->world; curRoom; curRoom = curRoom->next)
		    curRoom->find_first_step_index = 0;
    

    // Queue up the first room
    //         MapToken( int d, int r, int c, room_data *s, room_data *t ); 

    token = new MapToken(Up,row,col,ch->in_room,ch->in_room);
    push(token);
    // Process the queue
    while (! empty() ) {
        curToken = pop();

        // Check for a mark in the current direction
        // If it's marked, drop the current room and continue.
        if (MAPPED(curToken->getTarget(),curToken->direction)) {
            curToken->clear();
            delete curToken;
            continue;
        }
        // Mark the target room
        MAP(curToken->getTarget(),curToken->direction);
        
        // Draw the room and the link
        drawRoom( curToken->getSource(),curToken->getTarget(),curToken->row,curToken->column);
        drawLink( curToken->getSource(),curToken->getTarget(),curToken->row,curToken->column);
        

        if( ! full ) {
            room_direction_data *exit = NULL;
            // Queue the exits
            token = NULL;
            exit = curToken->getTarget()->dir_option[North];
            if( exit != NULL && exit->to_room != NULL && !MAPPED(exit->to_room,North)) {
                token = (new MapToken(North,curToken->row - 2,curToken->column,curToken->getTarget(), exit->to_room));
                if(token != NULL)
                    push(token);
            }                
            token = NULL;
            exit = curToken->getTarget()->dir_option[South];//&& exit->to_room->zone == curZone) {
            if( exit != NULL && exit->to_room != NULL && !MAPPED(exit->to_room,South)) {
                token = (new MapToken( South,curToken->row + 2,curToken->column,curToken->getTarget(), exit->to_room));
                if(token != NULL)
                    push(token);
            }                
            token = NULL;
            exit = curToken->getTarget()->dir_option[East];
            if( exit != NULL && exit->to_room != NULL && !MAPPED(exit->to_room,East)) {
                token = (new MapToken( East, curToken->row, curToken->column + 2,curToken->getTarget(), exit->to_room));
                if(token != NULL)
                    push(token);
            }                
            token = NULL;
            exit = curToken->getTarget()->dir_option[West];
            if( exit != NULL && exit->to_room != NULL && !MAPPED(exit->to_room,West)) {
                token = (new MapToken( West,curToken->row, curToken->column - 2,curToken->getTarget(), exit->to_room));
                if(token != NULL)
                    push(token);
            }
        }
        curToken->clear();
        delete curToken;
    }
    

    // Set the current position pixel
    // terrain 125 (*), exits 5 ( magenta )
    mapDisplay[ (row)* columns + (col)].terrain = 125;
    mapDisplay[ (row)* columns + (col)].exits = 5;
    return true;
}


