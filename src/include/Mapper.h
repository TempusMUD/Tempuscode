using namespace std;
// Tempus Includes
#include "screen.h"
#include "desc_data.h"
#include "comm.h"
#include "db.h"
#include "utils.h"
#include "interpreter.h"

class MapToken {
    public:
        MapToken();
        MapToken(const MapToken &token);
        MapToken &operator=(const MapToken &token);
        MapToken( int d, int r, int c, room_data *s, room_data *t )  {
            MapToken();
            set(d,r,c,s,t);
        }
        void set( int d, int r, int c, room_data *s, room_data *t ); 
        room_data *getSource() {return source;}
        void setSource(room_data *s){source = s;}

        room_data *getTarget() {return target;}
        void setTarget(room_data *t){target = t;}

        int getDirection() {return direction;}
        void setDirection(int d) {direction = d;}

        void setRow(int r) {row = r;}
        int getRow() {return row;}

        void setColumn(int c) {column = c;}
        int getColumn() {return column;}

        int direction;
        int row;
        int column;
        long targetID;
        struct room_data *target;
        struct room_data *source;

        MapToken *next;
};
class MapPixel {
    public:
        MapPixel() {
            terrain = -1;
            exits = 0;
            mapped = false;
        }
        char terrain;
        int exits;
        bool mapped;
};



class Mapper {
    public:
        Mapper(char_data *ch,int rows, int columns);
        ~Mapper();
        bool build();
        void display(char *buf,int bRows,int bCols);

    private:
        char_data *ch; // character doing the mapping
        int rows,columns; // size of the desired map
        MapPixel *mapDisplay;
        MapToken *mapStack;
        void drawLink (room_data *s,room_data *t,int row,int col,bool justLink = false);
        bool drawRoom( room_data *s,room_data *t,long row, long col);
        int getOppDir(int dir) {
            switch (dir) {
                case 0: return 2;
                case 1: return 3;
                case 2: return 0;
                case 3: return 1;
                case 4: return 5;
                case 5: return 4;
                default:
                    return -1;
            }
        }
        inline bool validRow(int row) {
            return (row < rows && row >= 0);
        }
        inline bool validColumn(int col) {
            return (col < columns && col >= 0);
        }
        inline bool empty() {
            return mapStack == NULL;
        }
        inline MapToken *pop() {
            if(mapStack == NULL)
                return NULL;
            MapToken *t = mapStack;
            mapStack = t->next;
            return t;
        }
        inline void push(MapToken *t) {
            t->next = mapStack;
            mapStack = t;
        }
};
const int North = 0;
const int East = 1;
const int South = 2;
const int West = 3;
const int Up = 4;
const int Down = 5;
