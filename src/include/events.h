#ifndef __EVENTS_H
#define __EVENTS_H

#define NUM_EVENTS 3

#include <list>
using namespace std;

// prototype Creature to avoid including creature.h
struct Creature;

class MobileEvent {
  public:
	MobileEvent(Creature * ch, Creature * target, short val1, short val2,
		short val3, short val4, string event_type);

	// the =0 here makes this a "pure virual" function
									/*virtual */ void process();
									//=0;
	struct Creature *getInit();
	struct Creature *getTarget();
	void setInit(Creature * ch);
	void setTarget(Creature * target);
	int getVal(int val);
	void setVal(int val_num, int set_to);
  private:
	 Creature * ch;
	Creature *target;
	short val[4];
	string event_type;
};

/*class EventPhysicalAttack : public MobileEvent 
{
    public:
        EventPhysicalAttack(Creature *ch, Creature *target, 
                            short val1, short val2, short val3, short val4) 
                            : MobileEvent(ch, target, val1, val2, val3, val4) {}
  
        void process(); 
};  

class EventExamine : public MobileEvent
{
    public:
        EventExamine(Creature *ch, Creature *target,
                     short val1, short val2, short val3, short val4)
                     : MobileEvent(ch, target, val1, val2, val3, val4) {}

        void process();
};

class EventSteal : public MobileEvent
{
    public:
        EventSteal(Creature *ch, Creature *target,
                   short val1, short val2, short val3, short val4)
                   : MobileEvent(ch, target, val1, val2, val3, val4) {}
        
        void process();
};*/

typedef list < MobileEvent * >eventQueue;

#endif
