#ifndef _MAP_H_
#define _MAP_H_

#include <vector>
#include <queue>
#include <string>

// Tempus Includes
#include "screen.h"
#include "desc_data.h"
#include "comm.h"
#include "db.h"
#include "utils.h"
#include "interpreter.h"
#include "room_data.h"
#include <signal.h>

using namespace std;

enum Directions {
    North,
    East,
    South,
    West,
    Up,
    Down,
    NorthWest,
    NorthEast,
    SouthWest,
    SouthEast
};

struct _sectToPen {
    int sectType;
    string penColor;
    string vNoExitColor;
    string hNoExitColor;
    string pen;
    string vNoExitPen;
    string hSouthNoExitPen;
    string hNorthNoExitPen;
};

class MapPixel {
    public:
        MapPixel() {
            _doorPen = "#";
            _upExitPen = "\\";
            _downExitPen = "/ ";
            _xCoord = -1;
            _yCoord = -1;
            _r = NULL;
            _isNorthEdge = false;
            _isSouthEdge = false;
            _isEastEdge = false;
            _isWestEdge = false;
            _dontDraw = false;
        };

        MapPixel(int x, int y, room_data *r) {
            _doorPen = "#";
            _upExitPen = "\\";
            _downExitPen = "/";
            _xCoord = x;
            _yCoord = y;
            _r = r;
            _isNorthEdge = false;
            _isSouthEdge = false;
            _isEastEdge = false;
            _isWestEdge = false;
            _dontDraw = false;
        };

        ~MapPixel() {};

        MapPixel &operator=(const MapPixel &m) {
            _doorPen = m._doorPen;
            _upExitPen = m._upExitPen;
            _downExitPen = m._downExitPen;
            _xCoord = m._xCoord;
            _yCoord = m._yCoord;
            _r = m._r;
            _isNorthEdge = m._isNorthEdge;
            _isSouthEdge = m._isSouthEdge;
            _isEastEdge = m._isEastEdge;
            _isWestEdge = m._isWestEdge;;
            _dontDraw = m._dontDraw;

            return *this;
        };

        int getXCoord() { return _xCoord; };
        int getYCoord() { return _yCoord; };
        room_data *getRoomPointer() { return _r; };
        bool dontDraw() { return _dontDraw; };
        
        void setXCoord(int s) { _xCoord = s; };
        void setYCoord(int s) { _yCoord = s; };
        void setRoomPointer(room_data *r) { _r = r; };
        void setDontDraw(bool b) { _dontDraw = b; };

        bool draw(vector<string> &_map);

        // Drawing helper functions
        bool drawDoor(int d, vector<string> &_map, string pen = "");
        void drawTerrain(int d, vector<string> &_map);
        void drawOneCorner(int d, vector<string> &_map);

        room_data *validEdge(int d, room_data *room = NULL) {
            room_data *dest = NULL;
            room_data *org = NULL;

            if (!room)
                org = _r;
            else
                org = room;

            if (!org)
                return false;

            if (org->dir_option[d])
                dest = org->dir_option[d]->to_room;

            return dest;
        };

        void setEdge(int d) {
            if (d == North)
                _isNorthEdge = true;
            if (d == South)
                _isSouthEdge = true;
            if (d == East)
                _isEastEdge = true;
            if (d == West)
                _isWestEdge = true;
        }
        static int centerX;
        static int centerY;
        static int hSize;
        static int vSize;

    private:
        int _xCoord;
        int _yCoord;
        string _doorPen;
        string _upExitPen;
        string _downExitPen;
        room_data *_r;
        bool _isNorthEdge;
        bool _isSouthEdge;
        bool _isEastEdge;
        bool _isWestEdge;
        bool _dontDraw;
};

class Map : vector<MapPixel> {
    public:
        friend class MapPixel;

        Map(Creature *ch, int hsize, int vsize);
        ~Map() {};

        bool draw();

        Map::iterator find(int x, int y);

        bool build();
        void display();

        static struct _sectToPen sectToPen[NUM_SECT_TYPES];

    private:
        int _hsize, _vsize;
        queue<MapPixel> _roomQueue;
        vector<string> _map;
        static Creature * _ch;
};
#endif
