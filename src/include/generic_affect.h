#ifndef _GENERIC_AFFECT_H_
#define _GENERIC_AFFECT_H_

#include <signal.h>
#include <string>
#include <map>
#include <typeinfo>
#include "executable_object.h"
#include "safe_list.h"
#include "xml_utils.h"
#include "room_data.h"


class Creature;
class GenericAffect;

using namespace std;

typedef map<string, GenericAffect *> AffectMap;

class GenericAffect {
    public:
        // Constructor & destructor
        GenericAffect();
        virtual ~GenericAffect();

        // Operators
        bool operator==(GenericAffect c) {
            if (_skillNum != c._skillNum)
                return false;

            return true;
        }

        static GenericAffect *constructAffect(string affectName);
        static void insertAffect(GenericAffect *affect);
        
        // Accessors
        void setSkillNum(int skillNum) { _skillNum = skillNum; }
        void setDuration(int duration) { _duration = duration; }
        void setUpdateInterval(int interval) { _updateInterval = interval; }
        void setLevel(short level) { _level = level; }
        void setAccumDuration(bool cumulative) { _accumDuration = cumulative; }
        void setAccumAffect(bool accumAffect) { _accumAffect = accumAffect; }
        void setAverageDuration(bool average) { _averageDuration = average; }
        void setAverageAffect(bool average) { _averageAffect = average; }
        void setOwner(long id) { _ownerID = id; }
        void setIsPermenant(bool perm=true) { _isPermenant = perm; };
        void setID(long long new_id) { _ID = new_id; };
        void setActive(bool active) { _active = active; }; 
        
        int getSkillNum() { return _skillNum; }
        int getDuration() { return _duration; }
        int getUpdateInterval() { return _updateInterval; }
        short getLevel() { return _level; }
        bool getAccumDuration() { return _accumDuration; }
        bool getAccumAffect() { return _accumAffect; }
        bool getAverageDuration() { return _averageDuration; }
        bool getAverageAffect() { return _averageAffect; }
        bool getIsPermenant() { return _isPermenant; }
        bool getActive() { return _active; }
        long getOwner() { return _ownerID; };
        long long getID() { return _ID; };
        string getClassName();


        // I can't think of a good implentation for these in the
        // superclass so we'll just leave them empty.
        virtual void handleTick() {};
        virtual void handleDeath() {};
        virtual void performUpdate() {};
        virtual bool canJoin() { return false; };
        virtual bool affectJoin() { return false; };
        virtual void affectRemove() {};
        virtual void affectModify(bool add = true) {}; 
        virtual void modifyEO(ExecutableObject *eo);
        
        virtual GenericAffect* createNewInstance();
        
    protected:
        int _skillNum;
        int _duration;
        int _updateInterval;
        short _level;
        bool _accumDuration;
        bool _accumAffect;
        bool _averageDuration;
        bool _averageAffect;
        bool _isPermenant;
        bool _active;
        long _ownerID;
        long _ID;

    private:
        static AffectMap* _getAffectMap() {
            static AffectMap affects;
            return &affects;
        }
};

class ObjectAffect : public GenericAffect {
    public:
        struct locModType {
            short location;
            short modifier;
        };

        ObjectAffect();
        ObjectAffect(ObjectAffect &c);
        ObjectAffect(Creature *ch);
        virtual ~ObjectAffect();

        // Operators
        bool operator==(ObjectAffect c) {
            if (_skillNum != c._skillNum)
                return false;

            return true;
        }

        // Accessors
        // The Add functions are mostly for internal use.  Feel free to use them, but make
        // sure you understand exactly what they do first.  Chances are, you want the SET functions.
        void addApply(short location, short modifier);
        void addValue(short location, short modifier);
        void addWeight(short modifier);
        void addDam(short modifier);
        void addMaxDam(short modifier);
        void setApply(short location, short modifier);
        void setBitvector(short which, long v) { _bitvector[which] = v; };
        void setWearoffToChar(string message) { _wearoffToChar = message; };
        void setWearoffToRoom(string message) { _wearoffToRoom = message; };
        void setTarget(obj_data *vict) { _target = vict; };
        void setDamMod(int mod) { _damMod = mod; };
        void setMaxDamMod(int mod) { _maxDamMod = mod; };
        void setTypeMod(char mod) { _typeMod = mod; };
        void setWornMod(int mod) { _wornMod = mod; };
        void setWeightMod(int mod) { _weightMod = mod; };
        void setValMod(int idx, char value) { _valMod[idx] = value; };
        
        short getApplyMod(short location);
        long getBitvector(short which) { return _bitvector[which]; }
        obj_data* getTarget() { return _target; }
        string getWearoffToChar() { return _wearoffToChar; }
        string getWearoffToRoom() { return _wearoffToRoom; }
        vector<locModType> &getApplyList() { return _apply; };
        vector<long> &getVectorList() { return _bitvector; };
        int getDamMod() { return _damMod; };
        int getMaxDamMod() { return _maxDamMod; };
        int getTypeMod() { return _typeMod; };
        int getOldType() { return _oldType; };
        int getWornMod() { return _wornMod; };
        int getWeightMod() { return _weightMod; };
        char getValMod(int idx) { return _valMod[idx]; };

        // Bit vector manipulation
        void addBitsToVector(short which, long bitvector) { 
            _bitvector[which] |= bitvector; 
        }
        void removeBitsFromVector(short which, long bitvector) { 
            _bitvector[which] &= ~bitvector;
        }
        bool bitIsSet(short which, long bitvector) { 
            return _bitvector[which] & bitvector; 
        }
       
        virtual bool canJoin();
        virtual bool affectJoin();
        virtual void affectRemove();
        virtual void affectModify(bool add = true); 
        virtual void handleTick();
        virtual void handleDeath();
        virtual void performUpdate();
        
        virtual void saveToXML(FILE *ouf);
        virtual void loadFromXML(xmlNodePtr node);

    protected:
        int _damMod;                // Add this value to the objects current damage
        int _maxDamMod;             // Add this value to the objects maximum damage 
        int _wornMod;               // Add these bits to the wear positions
        int _weightMod;             // Add this value to the weight
        char _valMod[4];            // Add this value to the objects weight
        char _typeMod;              // Change the type of the obj to this value
        char _oldType;              // Original type of obj (stored for wearoff)
        vector<long> _bitvector;    // Which extra bits to set
        vector<locModType> _apply;  // Which applies to change
        string _wearoffToChar;
        string _wearoffToRoom;

        obj_data *_target;
        virtual GenericAffect* createNewInstance();
};

class RoomAffect : public GenericAffect {
    public:

        enum {
            North,
            East,
            South, 
            West,
            Up,
            Down,
            Future,
            Past
        };

        struct exitFlags {
            int direction;
            int flags;
        };

        RoomAffect();
        RoomAffect(RoomAffect &c);
        RoomAffect(struct Creature *);
        virtual ~RoomAffect();

        bool operator==(RoomAffect &c) {
            if (_ID == c._ID)
                return true;

            if (_skillNum != c._skillNum)
                return false;

            if (_description != c._description)
                return false;

            if (_name != c._name)
                return false;

            for (int i = 0; i < 8; i++) {
                if (_exitInfo[i] != c._exitInfo[i])
                    return false;
            }

            if (_roomFlags != c._roomFlags)
                return false;

            if (_target != c._target)
                return false;
            return true;
        }

        // Accessors
        string getDescription() { return _description; };
        string getName() { return _name; };
        string getWearoffToRoom() { return _wearoffToRoom; }
        room_data *getTarget() { return _target; };
        long getExitInfo(short which) { return _exitInfo[which]; };
        long getRoomFlags() { return _roomFlags; };

        // Manipulators
        void setDescription(string d) { _description = d; };
        void setName(string n) { _name = n; };
        void setTarget(room_data *r) { _target = r; };
        void setExitInfo(short which, long bits) { _exitInfo[which] = bits; };
        void setRoomFlags(long f) { _roomFlags = f; };
        void setWearoffToRoom(string message) { _wearoffToRoom = message; };

        // Bit vector manipulation
        void addBitsToExitInfo(short which, long bitvector) { 
            _exitInfo[which] |= bitvector; 
        }
        void removeBitsFromExitInfo(short which, long bitvector) { 
            _exitInfo[which] &= ~bitvector;
        }
        bool exitBitIsSet(short which, long bitvector) { 
            return _exitInfo[which] & bitvector; 
        }

        // Members
        virtual bool canJoin();
        virtual bool affectJoin();
        virtual void affectRemove();
        virtual void affectModify(bool add = true); 
        virtual void handleTick();
        virtual void handleDeath();
        virtual void performUpdate();

    protected:
        string _description;        // Appends to the room description
        string _name;               // Appends to the room name
        vector<long> _exitInfo;     // Index into this to get or set exit flags for dir == index
        long _roomFlags;            // Add these bits to the room flags 
        string _wearoffToRoom;      // Wearoff message to the entire room

        room_data *_target;
        virtual GenericAffect *createNewInstance();
};

class CreatureAffect : public GenericAffect {
    public:
        struct locModType {
            short location;
            short modifier;
        };

        CreatureAffect();
        CreatureAffect(CreatureAffect &c);
        CreatureAffect(Creature *ch);
        virtual ~CreatureAffect();

        // Operators
        bool operator==(CreatureAffect c) {
            if (_ID == c._ID)
                return true;

            if (_skillNum != c._skillNum)
                return false;

            return true;
        }

        // Accessors
        void addApply(short location, short modifier);
        void addSkill(short location, short modifier);
        void setApply(short location, short modifier);
        void setBitvector(short which, long v) { _bitvector[which] = v; };
        void setWearoffToChar(string message) { _wearoffToChar = message; };
        void setWearoffToRoom(string message) { _wearoffToRoom = message; };
        void setTarget(Creature *vict) { _target = vict; };
        void setPosition(int position) { _position = position; };
        
        short getApplyMod(short location);
        short getSkillMod(short location);
        long getBitvector(short which) { return _bitvector[which]; }
        Creature* getTarget() { return _target; }
        string getWearoffToChar() { return _wearoffToChar; }
        string getWearoffToRoom() { return _wearoffToRoom; }
        vector<locModType> &getApplyList() { return _apply; };
        vector<long> &getVectorList() { return _bitvector; };
        int getPosition() { return _position; };

        // Bit vector manipulation
        void addBitsToVector(short which, long bitvector) { 
            _bitvector[which] |= bitvector; 
        }
        void removeBitsFromVector(short which, long bitvector) { 
            _bitvector[which] &= ~bitvector;
        }
        bool bitIsSet(short which, long bitvector) { 
            return _bitvector[which] & bitvector; 
        }
       
        virtual bool canJoin();
        virtual bool affectJoin();
        virtual void affectRemove();
        virtual void affectModify(bool add = true); 
        virtual void handleTick();
        virtual void handleDeath();
        virtual void performUpdate();
        
        virtual void saveToXML(FILE *ouf);
        virtual void loadFromXML(xmlNodePtr node);

    protected:
        // Which bits to set
        vector<long> _bitvector; 
        // Which skill to change
        vector<locModType> _skill;
        // Which apply to change
        vector<locModType> _apply;
        // Set the target to this position
        int _position;
        string _wearoffToChar;
        string _wearoffToRoom;
        
        Creature *_target;
        virtual GenericAffect* createNewInstance();
};

class MobAffect : public CreatureAffect {
    public:
        
        MobAffect() : CreatureAffect() {
            _isPermenant = true;
            _skillNum = -1;
        };

        MobAffect(MobAffect &c) : CreatureAffect(c) {
            _isPermenant = true;
            _skillNum = -1;
        };
        
        MobAffect(Creature *ch) : CreatureAffect(ch) {
            _isPermenant = true;
            _skillNum = -1;
        };
        
        virtual ~MobAffect() {};
};

class EquipAffect : public CreatureAffect {
    public:
        EquipAffect() : CreatureAffect() {};
        virtual ~EquipAffect() {};

        virtual bool canJoin();
        virtual bool affectJoin();
        virtual void handleTick();

    protected:
        virtual GenericAffect* createNewInstance();
};

class CreatureAffectList : public SafeList <CreatureAffect *> {
    public:
        CreatureAffectList(bool prepend = false) : 
            SafeList <CreatureAffect*>(prepend) {
        }
        ~CreatureAffectList() {
        }

        void clear() {
            while (size() > 0)
                remove (*(begin()));
        }

        void add_front(CreatureAffect *c) {
            push_front(c);
        }

        void add_back(CreatureAffect *c) {
            push_back(c);
        }

    private:
        inline operator int() {
            raise(SIGSEGV);
            return -1;
        }
};

class ObjectAffectList : public SafeList <ObjectAffect *> {
public:
    ObjectAffectList(bool prepend = false) : 
        SafeList <ObjectAffect*>(prepend) {
    }
    ~ObjectAffectList() {
    }

    void clear() {
        while (size() > 0)
            remove (*(begin()));
    }

    void add_front(ObjectAffect *c) {
        push_front(c);
    }

    void add_back(ObjectAffect *c) {
        push_back(c);
    }

private:
    inline operator int() {
        raise(SIGSEGV);
        return -1;
    }
};

class RoomAffectList : public SafeList <RoomAffect *> {
public:
    RoomAffectList(bool prepend = false) : 
        SafeList <RoomAffect*>(prepend) {
    }
    ~RoomAffectList() {
    }

    void clear() {
        while (size() > 0)
            remove (*(begin()));
    }

    void add_front(RoomAffect *c) {
        push_front(c);
    }

    void add_back(RoomAffect *c) {
        push_back(c);
    }

private:
    inline operator int() {
        raise(SIGSEGV);
        return -1;
    }
};

#endif
