#ifndef _ISCRIPT_H_
#define _ISCRIPT_H_

#include <string>
#include <list>
#include <fstream>

using namespace std;

struct Creature;

class CHandler {
  public:

	CHandler(string event, ifstream & ifile);
	CHandler(const CHandler & chandler);
	 CHandler(string event);

	const CHandler & operator = (const CHandler & chandler){
		if (&chandler != this) {
			event_type = chandler.event_type;
				theLines = chandler.theLines; target = chandler.target;}
  return *this;}; void process_handler(CHandler * theHandler); void setTarget(Creature * target); void setInit(Creature * ch); string getEventType(); list < string > &getTheLines(); private:
	Creature * target;
		Creature * ch;
		string event_type; list < string > theLines;}; class CIScript {
  public:
  CIScript(ifstream & ifile, string line); CIScript(int svnum); CHandler * handler_exists(const string & event_type); void do_handler(CHandler * theHandler); void setTarget(Creature * target); void setInit(Creature * ch); bool deleteHandler(string event_type); string getName(); void setName(string name); int getVnum(); void setVnum(int vnum); list < CHandler * >theScript; private:
	int vnum; string name; Creature * target; Creature * ch;};
#endif
