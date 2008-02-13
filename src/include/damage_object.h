#ifndef _DAMAGE_OBJECT_H_
#define _DAMAGE_OBJECT_H_

#include <vector>
#include <string>

#include "executable_object.h"

struct Creature;
struct obj_data;

using namespace std;

class DamageObject : public ExecutableObject {
    public:
        // Normal constructor & destructor
        DamageObject();
        ~DamageObject();

        // Copy constructor
        DamageObject(const DamageObject &c);

        // Operators
        DamageObject &operator =(DamageObject &c);
        bool operator >(DamageObject &c) const;
        bool operator <(DamageObject &c) const;
        bool operator ==(DamageObject &c) const;

        // Accessors
        void setWeapon(obj_data *weap);
        void setDamage(int damage);
        void setDamageType(int type);
        void setDamageLocation(int location);
        void setVictimKilled(bool victimKilled);
        void setDamageFailed(bool damageFailed);
        void setGenMessages(bool gen = true);
        
        Creature *getAttacker() const;
        obj_data *getWeapon() const;
        int getDamage() const;
        int getDamageType() const;
        int getDamageLocation() const;
        bool getDamageFailed() const;
        bool getGenMessages() const;
        
        // Normal members
        void execute();
        void cap();
        void generateDamageMessages();
        bool cannotDamage(struct Creature *vict);

        // This is just temporary, if I include spells.h here we have big problems
        bool isWeaponType() {
            return (_damageType >= 800 && _damageType < 820);
        }

    private:
        int _damage;
        int _damageType;
        int _damageLocation;
        bool _damageFailed;
        bool _genMessages;        
};

#endif
