#ifndef __EVENTS_H
#define __EVENTS_H

#define NUM_EVENTS 3 

#include <list>
using namespace std;

// prototype char_data to avoid including char_data.h
struct char_data;

class MobileEvent {
  public:
      MobileEvent(char_data *ch, char_data *target, short val1, short val2, short val3, short val4, string event_type); 
    
      // the =0 here makes this a "pure virual" function
      /*virtual*/ void process();//=0;
      struct char_data *getInit();
      struct char_data *getTarget();
      void setInit(char_data *ch);
      void setTarget(char_data *target);
      int getVal(int val);
      void setVal(int val_num, int set_to);
  private:    
      char_data *ch;
      char_data *target;
      short val[4];
      string event_type;            
};

/*class EventPhysicalAttack : public MobileEvent 
{
    public:
        EventPhysicalAttack(char_data *ch, char_data *target, 
                            short val1, short val2, short val3, short val4) 
                            : MobileEvent(ch, target, val1, val2, val3, val4) {}
  
        void process(); 
};  

class EventExamine : public MobileEvent
{
    public:
        EventExamine(char_data *ch, char_data *target,
                     short val1, short val2, short val3, short val4)
                     : MobileEvent(ch, target, val1, val2, val3, val4) {}

        void process();
};

class EventSteal : public MobileEvent
{
    public:
        EventSteal(char_data *ch, char_data *target,
                   short val1, short val2, short val3, short val4)
                   : MobileEvent(ch, target, val1, val2, val3, val4) {}
        
        void process();
};*/

typedef list<MobileEvent *> eventQueue;

#endif
