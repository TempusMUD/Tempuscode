#include <list>
using namespace std;

#include "structs.h"
#include "events.h"
#include "defs.h"
#include "utils.h"
#include "creature.h"

// This list brought to you by lame STL containers that don't
// know what polymorphism means
// See http://pages.cpsc.ucalgary.ca/~kremer/STL/1024x768/ref2.html
Event *Event::_scheduled_evts;

void
Event::Queue(Event *new_evt)
{
	Event *cur_evt;

	if (!_scheduled_evts || new_evt->less_than(_scheduled_evts)) {
		new_evt->_next = _scheduled_evts;
		_scheduled_evts = new_evt;
		return;
	}

	cur_evt = _scheduled_evts;
	while (cur_evt->_next && cur_evt->less_than(new_evt))
		cur_evt = cur_evt->_next;
	
	new_evt->_next = cur_evt->_next;
	cur_evt->_next = new_evt;
}

void
Event::ProcessScheduled(void)
{
	Event *cur_evt, *next_evt;

	if (!_scheduled_evts)
		return;

	while (_scheduled_evts && _scheduled_evts->is_ready()) {
		next_evt = _scheduled_evts->_next;
		if (_scheduled_evts->is_enabled())
			_scheduled_evts->trigger();
		delete _scheduled_evts;
		_scheduled_evts = next_evt;
	}

	for (cur_evt = _scheduled_evts;cur_evt;cur_evt = cur_evt->_next)
		cur_evt->pulse();
}

void
DeathEvent::trigger(void)
{
	Event *cur_evt;

	slog("Extract: %s", GET_NAME(_doomed));

	// Disable all the events
	for (cur_evt = _scheduled_evts;cur_evt;cur_evt = cur_evt->_next)
		if (cur_evt->involves(_doomed))
			cur_evt->disable();

	if (_arena)
		_doomed->arena_die();
	else
		_doomed->die();
	
}
