#ifndef _EVENTS_H_
#define _EVENTS_H_

#include <list>

using namespace std;

// Events only have one thing in common - a trigger time.  Trigger
// times are the number of pulses (tenths of a second) that are left to go.
class Event {
	public:
		static void ProcessScheduled(void);
		static void Queue(Event *new_evt);

		Event(long trigger_time)
		{
			_trigger_time = trigger_time;
			_enabled = true;
		}
		Event(Event &evt) { _trigger_time = evt._trigger_time; }
		virtual ~Event(void) {}

		void disable(void) { _enabled = false; }
		bool is_enabled(void) { return _enabled; }

		// These two check to make sure there aren't any pointers to the
		// given object.
		virtual bool involves(Creature *ch) = 0;
		virtual bool involves(obj_data *ch) = 0;

		Event *_next;

	private:
		static Event *_scheduled_evts;

		bool less_than(Event *other)
			{ return _trigger_time < other->_trigger_time; }


		// Trigger handles the actual mechanics of carrying out the event
		virtual void trigger(void) = 0;
		bool is_ready(void)
			{ return _trigger_time <= 0; }
		void pulse(void)
			{ _trigger_time--; }

		long int _trigger_time;
		bool _enabled;
};

class DeathEvent : public Event {
	public:
		DeathEvent(long trigger_time, Creature *doomed, bool arena)
			: Event(trigger_time), _doomed(doomed), _arena(arena) {}
		DeathEvent(DeathEvent &evt)
			: Event(evt) { _doomed = evt._doomed; }
		virtual ~DeathEvent(void) {}

	private:
		// Trigger handles the actual mechanics of carrying out the event
		virtual void trigger(void);
		// These two check to make sure there aren't any pointers to the
		// given object.
		virtual bool involves(Creature *ch)
			{ return _doomed == ch; }
		virtual bool involves(obj_data *ch)
			{ return false; }

		Creature *_doomed;
		bool _arena;
};

#endif
