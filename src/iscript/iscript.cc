using namespace std;

#include <iscript.h>
#include <algorithm>
#include <iterator>
#include <structs.h>
//#include <defs.h>
#include <utils.h>

struct Creature;
void command_interpreter(struct Creature *ch, char *argument);

list <CIScript *> scriptList;

CIScript::CIScript(ifstream & ifile, string line)
{
	string str;

	line = line.erase(0, 1);
	vnum = atoi(line.c_str());

	while (str != "__end script__") {
		if (str.substr(0, 5) == "Name:") {
			str.erase(0, str.find_first_of(":", 0) + 1);
			while (str.substr(0, 1) == " " || str.substr(0, 1) == "\t") {
				str.erase(0, 1);
			}
			name = str;
		}
		if (str.substr(0, 3) == "EVT") {
			CHandler *h = new CHandler(str, ifile);
			theScript.push_back(h);
		}
		getline(ifile, str);
	}

	target = NULL;
	ch = NULL;
}

CIScript::CIScript(int svnum)
{
	vnum = svnum;
	name = "A Fresh New Script";
	target = NULL;
	ch = NULL;
}

CHandler::CHandler(const CHandler & chandler)
{
	*this = chandler;
}

CHandler::CHandler(string event, ifstream & ifile)
{
	string str;

	event_type = event;
	while (getline(ifile, str)) {
		if (str == "__end event__") {
			return;
		} else {
			theLines.push_back(str);
		}
	}
}

CHandler::CHandler(string event)
{
	event_type = event;
}

CHandler *
CIScript::handler_exists(const string & event_type)
{
	list <CHandler *>::iterator h;

	for (h = theScript.begin(); h != theScript.end(); h++) {
		if ((*h)->getEventType() == event_type)
			return *h;
	}
	return NULL;
}

void
CIScript::do_handler(CHandler * theHandler)
{
	if (theHandler != NULL) {
		theHandler->setTarget(target);
		theHandler->process_handler(theHandler);
	}

}

void
CHandler::process_handler(CHandler * theHandler)
{
	list <string>::iterator l;
	char buf[MAX_INPUT_LENGTH];

	for (l = theHandler->theLines.begin(); l != theHandler->theLines.end();
		l++) {
		//it sucks but l->c_str() is a const and arg 2 of command_interpreter is not...
		//this is the only way I know of to get rid of the compiler error that generates
		strcpy(buf, l->c_str());
		command_interpreter(target, buf);
	}
}

void
CIScript::setTarget(Creature * target)
{
	this->target = target;
}

void
CIScript::setInit(Creature * init)
{
	this->ch = init;
}

void
CHandler::setTarget(Creature * target)
{
	this->target = target;
}

void
CHandler::setInit(Creature * init)
{
	this->ch = init;
}

string
CHandler::getEventType()
{
	return this->event_type;
}

list <string> &CHandler::getTheLines()
{
	return this->theLines;
}

string
CIScript::getName()
{
	return this->name;
}

void
CIScript::setName(string name)
{
	this->name = name;
}

int
CIScript::getVnum()
{
	return this->vnum;
}

void
CIScript::setVnum(int vnum)
{
	this->vnum = vnum;
}

bool
CIScript::deleteHandler(string event_type)
{
	list <CHandler *>::iterator hi;

	for (hi = theScript.begin(); hi != theScript.end(); hi++) {
		if ((*hi)->getEventType() == event_type) {
			theScript.erase(hi);
			return true;
		}
	}
	return false;
}
