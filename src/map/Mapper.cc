#include <vector>
#include <fstream>
#include <string>
using namespace std;
#include "structs.h"
#include "char_data.h"
#include "utils.h"
#include "interpreter.h"
#include "comm.h"
//#include "Mapper.h"
#include <signal.h>

//static fstream debug;

const static int mapBits [] = { 1, 2, 4, 8, 16, 32, 64, 128, 256, 512 };

#define MAPPED(mappedRoom, mappedDirection) (mappedRoom->find_first_step_index & mapBits[mappedDirection])
#define MAP(mappedRoom, mappedDirection) (mappedRoom->find_first_step_index |= mapBits[mappedDirection])
ACMD(do_map) {
    send_to_char("Map removed until further notice.\r\n",ch);
    /*
    int rows = GET_PAGE_LENGTH(ch);
    int columns = 80;
    char mapBuf[rows * columns];
    Mapper theMap(ch,rows,columns);
    theMap.build();
    theMap.display(mapBuf,rows,columns);
    */
}
/*
MapToken::MapToken() {
        direction = -1;
        row = 0;
        column = 0;
        target = NULL;
        source = NULL;
        targetID = -1;
        //fprintf(stderr,"BLANK MapToken\r\n");
}
*/
/*
void MapToken::set(int d, int r, int c, room_data *s, room_data *t ) {
    direction = d; row = r; column = c; source = s; target = t;
    targetID = target->number;
    //printf(stderr,"SET MapToken d=%d r=%d c=%d id=%ld s=%s t=%s\r\n",
    //        d,r,c,targetID, (s ? "X" : "NULL"),( t ? "X" : "NULL") );
}
*/
/*
MapToken::MapToken( const MapToken & token ) {
    *this = token;
}
*/
/*
MapToken &MapToken::operator=(const MapToken &token) {
    this->direction = token.direction;
    this->row = token.row;
    this->column = token.column;
    this->source = token.source;
    this->target = token.target;
    this->targetID = token.targetID;
    //fprintf(stderr,"SET MapToken d=%d r=%d c=%d id=%ld s=%s t=%s\r\n",
    //        direction,row,column,targetID, (source ? "X" : "NULL"),( target ? "X" : "NULL") );
    if(target == NULL) {
        raise(SIGSEGV);
    }
    return *this;
}
*/
/*
Mapper::Mapper(char_data *ch,int rows, int columns) : mapDisplay(rows * columns), mapQueue() {
    this->rows = rows;
    this->columns = columns;
    this->ch = ch;
}
*/

/*
void Mapper::display(char *buf,int bRows,int bCols) {
    string line;
    char *r,pen;
    int exits;
    char terrain;
    long row,col;
    long wrtLen;
    row = col = 0;
    r = buf;
    vector<MapPixel>::iterator pixel;
    //fstream mapFile ("map.out",ios::out);
    for ( row = 0;row < bRows;row++) {
        for (col = 0;col < bCols;col++) {
            pixel = mapDisplay.begin() + row * columns + col;
            terrain = (*pixel).terrain;
            exits = (*pixel).exits;
            wrtLen = 0;
            switch(terrain) {// 0 - 30
                case 0:
                    pen = ' ';
                    break;
                case 1:        // V Void
                    pen = 'V';
                    break;
                case 2:        // R Roads
                    pen = 'R';
                    break;
                case 3:        // F Forrest
                    pen = 'F';
                    break;
                case 4:        // D Flat+hot
                    pen = 'D';
                    break;
                case 5:        // P Plains + Stuff
                    pen = 'P';
                    break;
                case 6:        // I Inside/cave
                    pen = 'I';
                    break;
                case 7:        // M Mountain/cliff/hill/steep
                    pen = 'M';
                    break;
                case 8:        // S Swamp
                    pen = 'S';
                    break;
                case 9:        // A Air
                    pen = 'A';
                    break;
                case 10:    // B Beach
                    pen = 'B';
                    break;
                case 11:    // U Underwater
                    pen = 'U';
                    break;
                case 12:    // W Water
                    pen = 'W';
                    break;
                case 13:    // C City
                    pen = 'C';
                    break;
                case 14:
                    pen = 'J';
                    break;
                case 28:    // Current Position
                    pen = '*';
                    exits = 5;
                    break;
                case 29:    // |
                    pen = '|';
                    break;
                case 30:    // -
                    pen = '-';
                    break;
                default:
                    pen = ' ';
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
                    // *w++ = ' ';
                    break;
            }
        }
        // *w = '\0';
        //w = line;
        strcpy(buf,line.c_str());
        strcat(buf,"\r\n");
        send_to_char(buf, ch);
        //mapFile << line << endl;
        //mapFile.flush();
        // *w = '\0';
        line = "";
    }
}*/
    
/*
bool Mapper::drawRoom( room_data *s,room_data *t,long row, long col) {
    //struct zone_data *currentZone = ch->in_room->zone;
    bool drawn;     // did we draw?
    short terrain, exits;
 
    // if this is an obvious loop room, drop it
    if (t == s)
        return false;
    // If we are out of bounds, drop out of this room.
    if ( row < 0 || row > rows || col < 0 || col > columns)
        return false;

    terrain = 0;
    exits = 0;

    // Check for existing map symbol
    // if there is one, just return.
    if  ( !( mapDisplay[row * columns + col].mapped ) ) {
        // Check terrain type
        int x;
        //x = t->mTerrain;
        x = 27;
        if(x <= 1)
            terrain = 1;    // Void
        else if(x <= 5)
            terrain = 2;    // Roads
        else if(x <= 7)
            terrain = 3;    // Forrest
        else if(x <=9)
            terrain = 4;    // Flat+hot or beach
        else if(x <=14)
            terrain = 5;    // Plains + Stuff
        else if(x <=16)
            terrain = 6;    // Inside/cave
        else if(x <=20)
            terrain = 7;    // Mountain/Cliff/Hill/Steep
        else if(x ==21)
            terrain = 8;    // Swamp
        else if(x <=23)
            terrain = 9;    // Air
        else if(x ==24)
            terrain = 10;    // Beach
        else if(x ==25)
            terrain = 11;    // Underwater
        else if(x <=29)
            terrain = 12;    // Water
        else if(x == 30)
            terrain = 13;    // City
        else if(x == 31)
            terrain = 14;
        else
            terrain = 27;    // Question Mark

        // Check for links up and down.
        if (s->zone != t->zone) {
            exits = 4;
            //debug << "Crossing zone border from " << s->zone->number << " to " << t->zone->number << "."<< endl;
            //debug.flush();
        } else {
            if (t->dir_option[Up] && t->dir_option[Up]->to_room)
                exits += 1;
            if (t->dir_option[Down] && t->dir_option[Down]->to_room)
                exits += 2;
        }

        mapDisplay[row * columns + col].terrain = terrain;
        mapDisplay[row * columns + col].exits = exits;
    }
    return drawn;
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

    // Write in the link characters. remember that +1 is 1 lower on the page.
    // 29 is | 30 is -
    if (sExit == North && tExit == South) {    // Normal north/south links
        if( validRow( row + 1 ) && validColumn( col )) {
            mapDisplay[ (row + 1)* columns + col].terrain = 29;
            mapDisplay[ (row + 1)* columns + col].exits = 0;
        }
    } else if(sExit == South && tExit == North) {
        if( validRow( row - 1 ) && validColumn( col )) {
            mapDisplay[ (row - 1)* columns + col].terrain = 29;
            mapDisplay[ (row - 1)* columns + col].exits = 0;        
        }
    } else if(sExit == East && tExit == West)  {   // Normal east/west links
        if( validRow( row ) && validColumn( col - 1 )) {
            mapDisplay[ (row)* columns + (col -1)].terrain = 30;
            mapDisplay[ (row)* columns + (col -1)].exits = 0;
         }
    } else if(sExit == West && tExit == East) {
        if( validRow( row ) && validColumn( col + 1 )) {
            mapDisplay[ (row)* columns + (col +1)].terrain = 30;
            mapDisplay[ (row)* columns + (col +1)].exits = 0;
        }
    }
    // Here starts the wierd links. (round rooms and the like)
    // Round rooms put off till sometime next centry.

   //    **    One way exits with no return to any room    **
   //
   // Note : This does not apply to a room that has a
   //            return link to a different room
   //
    else if( tExit == -1 && t->dir_option[sExit] && t->dir_option[sExit]->to_room == NULL ) {
        // north-> link
        if( sExit == North ) {
            if( validRow( row + 1 ) && validColumn( col )) {
                mapDisplay[ (row +1)* columns + (col)].terrain = 29;
                mapDisplay[ (row +1)* columns + (col)].exits = 0;
            }
        // south-> link
        } else if    ( sExit == South ) {
            if( validRow( row - 1 ) && validColumn( col )) {
                mapDisplay[ (row -1)* columns + (col)].terrain = 29;
                mapDisplay[ (row -1)* columns + (col)].exits = 0;
            }
        // east -> link
        } else if    ( sExit == East ) {
            if( validRow( row ) && validColumn( col - 1 )) {
                mapDisplay[ (row)* columns + (col -1)].terrain = 29;
                mapDisplay[ (row)* columns + (col -1)].exits = 0;
            }
        // west -> link
        } else if    ( sExit == West ) {
            if( validRow( row ) && validColumn( col + 1 )) {
                mapDisplay[ (row)* columns + (col +1)].terrain = 29;
                mapDisplay[ (row)* columns + (col +1)].exits = 0;
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
*/
/*
bool Mapper::build() {
    int i;
    long row,col;
    room_data *curRoom;
    MapToken temp;
    MapToken current;
    

    // Make sure we are really going to map.

    curRoom = ch->in_room;
    // Is there at least one mappable exit?
    for(i = 0;i < 4;i++) {
        if(curRoom->dir_option[i] && curRoom->dir_option[i]->to_room != NULL) 
            break;
    }
    if(i > 4) { // no exits
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

    temp.set(0,row,col,ch->in_room,ch->in_room);
    mapQueue.push_back( temp );
    // Process the queue
    while (! mapQueue.empty()) {
        current = mapQueue.front();
        mapQueue.pop_front();

        // Check for a mark in the current direction
        // If it's marked, drop the current room and continue.
        if (MAPPED(current.target,getOppDir(current.direction))) {
            continue;
        }
        // Mark the target room
        //current->trg->mMapped[SwapDir(current->dir)] = 1;
        MAP(current.target,getOppDir(current.direction));
        
        // Draw the room and the link
        drawRoom( current.source,current.target,current.row,current.column);
        drawLink( current.source,current.target,current.row,current.column);
        

        room_direction_data *op = NULL;
        // Queue the exits
        op = current.target->dir_option[North];
        if( op != NULL && op->to_room != NULL) {
            temp.set(North,current.row - 2,current.column,current.target, op->to_room);
            mapQueue.push_back( temp );
        }                
        op = current.target->dir_option[South];
        if( op != NULL && op->to_room != NULL) {
            temp.set( South,current.row + 2,current.column,current.target, op->to_room);
            mapQueue.push_back( temp );
        }                
        op = current.target->dir_option[East];
        if( op != NULL && op->to_room != NULL) {
            temp.set( East, current.row, current.column + 2,current.target, op->to_room);
            mapQueue.push_back( temp );
        }                
        op = current.target->dir_option[West];
        if( op != NULL && op->to_room != NULL) {
            temp.set( West,current.row, current.column - 2,current.target, op->to_room);
            mapQueue.push_back( temp );
        }
    }
    

    // Set the current position pixel
    // terrain 28 (*), exits 5 ( magenta )
    mapDisplay[ (row)* columns + (col)].terrain = 28;
    mapDisplay[ (row)* columns + (col)].exits = 5;
    return true;
}
*/

/*
 Algorithm by D. Lowe - 2001


# Separate rooms into list of unset vertices and list of edges
recurse:
        add room
        for each exit
                if edge does not alread exist
                        if exit is symmetrical
                                add edge
                                recurse
                        else
                                add corner
                                add edge from this room to corner
                                add edge from corner to other room
                                recurse
                        fi
                fi
        rof


Set canonical position of vertex of location (aka set center)

# Equalize edges
list of set vertices
current vertex = location

begin:
for each edge connected to current vertex
        if horizontal edge
                if other vertex is set
                        if Y(current) != Y(other)
                                Y(current) = Y(other)
                                check horizontal constraints( other ) until no violations
                if other vertex is not set
                        set other vertex +/- 2
                        put vertex on edge checking stack
        if vertical edge
                if other vertex is set
                        if X(current) != X(other)
                                X(current) = X(other)
                                check vertical constraints(other) until no violations
                if other vertex is not set
                        set other vertex +/- 2
                        put vertex on edge checking stack
        if no more edges
                pop current vertex from edge checking stack
        if no more vertexes
                done
*/

