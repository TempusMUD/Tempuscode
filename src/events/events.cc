#include <list>
using namespace std;

#include "structs.h"
#include "events.h"
#include "defs.h"
#include "iscript.h"
#include "utils.h"

eventQueue mobileQueue;

struct char_data;
extern list<CIScript *> scriptList;

void call_process(MobileEvent *e);

void send_to_char(const char *, struct char_data *);

MobileEvent::MobileEvent(char_data *ch, 
                         char_data *target, 
                         short val1, 
                         short val2, 
                         short val3, 
                         short val4) 
{
  this->ch = ch;
  this->target = target;
  val[0] = val1;
  val[1] = val2;
  val[2] = val3;
  val[3] = val4;
}

struct char_data *MobileEvent::getInit()
{
    return this->ch;
}

struct char_data *MobileEvent::getTarget()
{
    return this->target;
}

void MobileEvent::setInit(char_data *init)
{
    this->ch = ch;
}

void MobileEvent::setTarget(char_data *target)
{
    this->target = target;
}

int MobileEvent::getVal(int val)
{
    return this->val[val];
}

void MobileEvent::setVal(int val_num, int set_to)
{
    this->val[val_num] = set_to;
}

void EventPhysicalAttack::process()
{
    list<CIScript *>::iterator s;
    for(s = scriptList.begin(); s != scriptList.end(); s++) {
        if(GET_SCRIPT_VNUM(this->getTarget()) == (*s)->getVnum()) {
            (*s)->setTarget(this->getTarget());
            (*s)->setInit(this->getInit());
            (*s)->do_handler((*s)->handler_exists("EVT_PHYSICAL_ATTACK"));
        }
    }
}

void EventExamine::process()
{
    list<CIScript *>::iterator s;
    for(s = scriptList.begin(); s != scriptList.end(); s++) {
        if(GET_SCRIPT_VNUM(this->getTarget()) == (*s)->getVnum()) {
            (*s)->setTarget(this->getTarget());
            (*s)->setInit(this->getInit());
            (*s)->do_handler((*s)->handler_exists("EVT_EXAMINE"));
        }
    } 
}

void EventSteal::process()
{
    list<CIScript *>::iterator s;
    for(s = scriptList.begin(); s != scriptList.end(); s++) {
        if(GET_SCRIPT_VNUM(this->getTarget()) == (*s)->getVnum()) {
            (*s)->setTarget(this->getTarget());
            (*s)->setInit(this->getInit());
            (*s)->do_handler((*s)->handler_exists("EVT_STEAL"));
        }
    }
}

void send_to_queue(MobileEvent *e)
{
  mobileQueue.push_back(e);
}

void process_queue(void)
{
  while(!mobileQueue.empty()) {  
    call_process(mobileQueue.front());
    if(!mobileQueue.empty())
        mobileQueue.pop_front();
  }
}

void call_process(MobileEvent *e)
{
    if(e)    
        e->process();
}

